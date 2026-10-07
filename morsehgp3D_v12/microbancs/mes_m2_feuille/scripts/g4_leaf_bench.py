#!/usr/bin/env python3
"""MES-M2 sur G4 : feuille du catalogue sur le GPU (microbanc hors produit de la v12).

Une seule entree pour la session : construit, vide les feuilles de la v11, admet les vidages, verifie l'identite sur
l'hote, mesure l'etendue locale (MES-S), joue le banc CUDA en plusieurs processus et juge, puis ecrit un rapport JSON.
Bibliotheque standard seulement (Python 3.10 de la VM, sans numpy) ; aucune commande GCP.

Etapes (journal par etape dans <out>/logs/) :
  1. versions et environnement (nvcc, cmake, compilateur, nvidia-smi, uptime, processus GPU) ;
  2. si les vidages ne sont pas fournis (--dumps) : construction de la bibliotheque v11 (Release, u21, cible mhgp11)
     et de l'outil de vidage, puis vidage des trames (--data) en parallele (cible effacee avant chaque vidage) ;
  3. construction du banc (CUDA sm_120) et des outils hote ; empreintes sha256 des binaires et des sources ;
  4. admission : porte du lecteur (mhgp12_dump_admission_selftest), identite de chaque vidage (sha256, en-tete,
     empreinte FNV-1a finale) et admission de tous les vidages par le lecteur C++ (mhgp12_leaf_identity --admission),
     AVANT tout noyau ; un refus arrete la mesure ;
  5. identite sur l'hote des formes de base contre leaf.cpp (toutes les feuilles), auto-test de la verification
     d'arene, MES-S, puis Compute Sanitizer (memcheck, racecheck, synccheck) sur les premieres feuilles d'un cas ;
  6. banc : un processus jete puis --processes processus par cas, ordre des cas tournant ; chaque prise est un fichier
     neuf (cible effacee avant le lancement) qui doit repeter le jeton de la session (--nonce), citer l'empreinte du
     vidage lu et ses comptes, et porter exactement les formes et repetitions demandees ;
  7. isolation du GPU relevee avant et apres le banc ; binaires, sources et vidages rehaches en fin de session ;
  8. juge (regle ecrite d'avance, README.md § 9) et rapport <out>/report.json.

Regle d'adoption (PLAN.md, MES-M2 ; README.md § 9, CONTRAT ci-dessous) : comptage <= 1/3 du temoin (noyau un-fil de la
v11) et identite avec la feuille de reference. Decident les cas a feuilles 24 (K5/24 et K10/24) de ng00, ng01 et ng02 ;
les cas K5/16 sont mesures et publies, l'identite y est exigee aussi. Trois verdicts par forme : « adopte », « rejete »
(mesure ou identite du banc), « refuse » (banc invalide ou preuve absente : vidage, prise, processus, binaire,
identite hote de code non nul, auto-tests, sanitizers, isolation GPU, options hors contrat) ; seul « adopte » permet
l'adoption. Constats CST-0018 et CST-0215 : toute preuve manquante, perimee ou non rattachee a son vidage rend
« refuse », jamais « adopte » ni « rejete ».

Usage :
  python3 scripts/g4_leaf_bench.py --out OUT --data DONNEES [--repo DEPOT] [--dumps VIDAGES] [--processes 5]
          [--reps 15] [--warmup 3] [--frames ng00,ng01,ng02] [--configs 5:16,5:24,10:24] [--forms ...]
          [--jobs N] [--no-cuda] [--nvcc NVCC] [--cmake CMAKE]
Les options qui s'ecartent du contrat (trames, configurations, cas qui decident, moins de 5 processus, sans identite
ou sans sanitizers) restent permises pour l'exploration : le rapport publie les mesures, le verdict est « refuse ».
Codes : 0 rapport ecrit (quel que soit le verdict), 2 refus avant toute mesure (arguments, depot introuvable).
"""

import argparse
import hashlib
import json
import math
import os
import random
import shutil
import struct
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent  # racine du microbanc (mes_m2_feuille)
DEFAULT_FORMS = ['witness', 'j3', 'j3_r168', 'j3_r128', 'coherent', 'coherent_r168', 'coherent_r128']
THRESHOLD = 1.0 / 3.0  # regle d'adoption : comptage <= 1/3 du temoin
BOOTSTRAP = 10000
SEED = 20261007
PROFILE_BITS = 21  # profil de la construction (MHGP12_COORD_BITS par defaut du CMakeLists)
SANITIZERS = ('memcheck', 'racecheck', 'synccheck')
# Contrat ecrit avant la mesure (README.md § 9) : trames, configurations, cas qui decident, processus minimum.
CONTRACT = {'frames': ('ng00', 'ng01', 'ng02'), 'configs': ((5, 16), (5, 24), (10, 24)),
            'decide': ((5, 24), (10, 24)), 'processes_min': 5}
REQUIRED_BINARIES = ('mhgp12_leaf_identity', 'mhgp12_mes_s', 'mhgp12_arena_selftest', 'mhgp12_dump_admission_selftest',
                     'mhgp12_leaf_bench')
HEADER = struct.Struct('<8s15Q')  # en-tete MHGP12LF v1 (dump_format.hpp, 128 octets)
HEADER_FIELDS = ('version', 'coord_bits', 'kmax', 'leaf_size', 'max_leaf', 'flags', 'n_sites', 'n_leaves',
                 'n_leaf_sites', 'n_records', 'n_population', 'n_counters', 'walk_leaves', 'walk_inline_leaves',
                 'reserved')


def now():
    return time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def compiled_dependencies(bdir, roots):
    """Dependances locales effectivement compilees : fichiers .d du compilateur sous <bdir>/CMakeFiles (CMake >= 3.20),
    chemins sous les racines donnees (sources du microbanc, v11), haches. Les en-tetes systeme sont hors du releve."""
    found = set()
    roots = [os.path.realpath(str(r)) for r in roots]
    for folder, _, names in os.walk(str(Path(bdir) / 'CMakeFiles')):
        for name in names:
            if not name.endswith('.d'):
                continue
            try:
                text = Path(folder, name).read_text(errors='replace').replace('\\\n', ' ')
            except OSError:
                continue
            for token in text.split():
                if token.endswith(':'):
                    continue
                path = os.path.realpath(token)
                if any(path.startswith(r + os.sep) for r in roots) and os.path.isfile(path):
                    found.add(path)
    out = {}
    for path in sorted(found):
        label = next(('%d:%s' % (i, os.path.relpath(path, r)) for i, r in enumerate(roots)
                      if path.startswith(r + os.sep)), path)
        out[label] = sha256_file(path)
    return out


def dump_identity(path):
    """Identite d'un vidage MHGP12LF lue en une passe : sha256, taille, champs de l'en-tete, empreinte FNV-1a finale
    (8 derniers octets, verifiee par le lecteur C++ a l'admission et citee par chaque outil). None si illisible."""
    h = hashlib.sha256()
    size, head, tail = 0, b'', b''
    try:
        with open(path, 'rb') as f:
            for block in iter(lambda: f.read(1 << 20), b''):
                if len(head) < HEADER.size:
                    head += block[:HEADER.size - len(head)]
                tail = (tail + block)[-8:]
                size += len(block)
                h.update(block)
    except OSError:
        return None
    if len(head) < HEADER.size or len(tail) < 8:
        return None
    values = HEADER.unpack(head)
    out = {'sha256': h.hexdigest(), 'octets': size, 'magic': values[0].decode('ascii', 'replace'),
           'fnv1a': '%016x' % int.from_bytes(tail, 'little')}
    out.update(dict(zip(HEADER_FIELDS, values[1:])))
    return out


class Session:
    def __init__(self, out):
        self.out = Path(out)
        self.logs = self.out / 'logs'
        self.logs.mkdir(parents=True, exist_ok=True)
        self.steps = []
        self.refusals = []

    def run(self, name, cmd, timeout, cwd=None, env=None, capture=False):
        """Lance une commande, journal dans logs/<name>.log ; rend (code, sortie standard, erreur standard)."""
        start = time.time()
        log = self.logs / (name + '.log')
        try:
            proc = subprocess.run([str(c) for c in cmd], cwd=cwd, env=env, timeout=timeout,
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            code, stdout, stderr = proc.returncode, proc.stdout, proc.stderr
        except subprocess.TimeoutExpired as e:
            code, stdout, stderr = 124, e.stdout or b'', (e.stderr or b'') + b'\ndelai depasse\n'
        except OSError as e:
            code, stdout, stderr = 127, b'', str(e).encode()
        elapsed = time.time() - start
        with open(log, 'wb') as f:
            f.write(('$ ' + ' '.join(str(c) for c in cmd) + '\n').encode())
            f.write(stdout)
            f.write(b'\n--- stderr ---\n')
            f.write(stderr)
        self.steps.append({'step': name, 'code': code, 'seconds': round(elapsed, 3), 'log': str(log.name)})
        return code, stdout.decode(errors='replace'), stderr.decode(errors='replace')

    def refuse(self, reason):
        if reason not in self.refusals:
            self.refusals.append(reason)


def find_repo(start):
    for p in [start] + list(start.parents):
        if (p / 'morsehgp3D_v11' / 'src' / 'catalogue' / 'leaf.cpp').is_file():
            return p
    return None


def find_nvcc(explicit):
    cands = [explicit] if explicit else []
    cands.append(shutil.which('nvcc'))
    if os.environ.get('CUDA_HOME'):
        cands.append(os.path.join(os.environ['CUDA_HOME'], 'bin', 'nvcc'))
    cands += ['/usr/local/cuda/bin/nvcc', '/usr/local/cuda-12.9/bin/nvcc']
    cands += sorted(str(p) for p in Path('/usr/local').glob('cuda-12*/bin/nvcc'))
    for c in cands:
        if c and os.path.isfile(c) and os.access(c, os.X_OK):
            return c
    return None


GPU_APPS = ['nvidia-smi', '--query-compute-apps=pid,process_name,used_memory', '--format=csv,noheader']


def environment(s, nvcc, cmake):
    env = {'date_utc': now(), 'python': sys.version.split()[0], 'cpu_count': os.cpu_count()}
    for name, cmd in (('nvcc', [nvcc, '--version'] if nvcc else None), ('cmake', [cmake, '--version']),
                      ('cxx', ['c++', '--version']), ('uptime', ['uptime', '-s']),
                      ('gpu', ['nvidia-smi', '--query-gpu=name,driver_version,persistence_mode,clocks.sm,'
                               'clocks.max.sm,temperature.gpu,power.draw,memory.used,memory.total',
                               '--format=csv,noheader']),
                      ('gpu_apps', GPU_APPS)):
        if cmd is None:
            env[name] = None
            continue
        code, out, err = s.run('env_' + name, cmd, 60)
        env[name] = out.strip() if code == 0 else {'code': code, 'stderr': err.strip()[-400:]}
    return env


def gpu_quiet(s, label):
    """Isolation du GPU : nvidia-smi lisible (code 0) et aucun processus de calcul. Rend (certifiee, releve)."""
    code, out, err = s.run('gpu_apps_' + label, GPU_APPS, 60)
    quiet = code == 0 and out.strip() == ''
    return quiet, {'code': code, 'processes': out.strip()[-400:], 'stderr': err.strip()[-200:], 'quiet': quiet}


def build_v11(s, repo, work, cmake, jobs):
    bdir = work / 'b_v11'
    code, _, _ = s.run('v11_configure', [cmake, '-S', repo / 'morsehgp3D_v11', '-B', bdir,
                                         '-DCMAKE_BUILD_TYPE=Release', '-DMHGP11_COORD_BITS=21',
                                         '-DMHGP11_MODULES=catalogue'], 900)
    if code != 0:
        return None
    code, _, _ = s.run('v11_build', [cmake, '--build', bdir, '-j', str(jobs), '--target', 'mhgp11'], 1800)
    lib = bdir / 'libmhgp11.a'
    return lib if code == 0 and lib.is_file() else None


def build_feuille(s, repo, work, cmake, jobs, nvcc, v11_lib, cuda):
    bdir = work / 'b_feuille'
    cmd = [cmake, '-S', HERE, '-B', bdir, '-DCMAKE_BUILD_TYPE=Release',
           '-DMHGP11_SOURCE_DIR=' + str(repo / 'morsehgp3D_v11')]
    if v11_lib:
        cmd.append('-DMHGP11_LIBRARY=' + str(v11_lib))
    if cuda:
        cmd += ['-DMHGP12_FEUILLE_CUDA=ON', '-DCMAKE_CUDA_COMPILER=' + nvcc]
    code, _, _ = s.run('feuille_configure', cmd, 900)
    if code != 0:
        return None
    code, _, _ = s.run('feuille_build', [cmake, '--build', bdir, '-j', str(jobs)], 3600)
    return bdir if code == 0 else None


def make_dumps(s, tool, data, frames, configs, dumps, jobs):
    """Vidages en parallele (un processus par vidage, au plus jobs a la fois). Chaque cible est effacee avant le
    lancement : un vidage perime ne peut jamais remplacer un vidage en echec."""
    dumps.mkdir(parents=True, exist_ok=True)
    pending = []
    for frame in frames:
        for k, leaf in configs:
            pending.append((frame, k, leaf))
    running, results = [], {}
    while pending or running:
        while pending and len(running) < jobs:
            frame, k, leaf = pending.pop(0)
            name = '%s_k%d_l%d' % (frame, k, leaf)
            target = dumps / (name + '.bin')
            try:
                if target.exists():
                    target.unlink()
            except OSError as e:
                results[name] = {'code': None, 'seconds': 0.0, 'summary': None, 'error': 'cible perimee : %s' % e}
                continue
            xyz, ids = data / ('lidar_%s.u32le' % frame), data / ('lidar_%s.ids.u32le' % frame)
            out = open(s.logs / ('dump_%s.log' % name), 'wb')
            p = subprocess.Popen([str(tool), str(xyz), str(ids), str(k), str(leaf), str(target)],
                                 stdout=subprocess.PIPE, stderr=out)
            running.append((name, p, out, time.time()))
        still = []
        for name, p, out, t0 in running:
            if p.poll() is None:
                still.append((name, p, out, t0))
                continue
            stdout = p.stdout.read().decode(errors='replace')
            out.write(stdout.encode())
            out.close()
            info = None
            for line in stdout.splitlines():
                try:
                    info = json.loads(line)
                except ValueError:
                    pass
            results[name] = {'code': p.returncode, 'seconds': round(time.time() - t0, 3), 'summary': info}
            s.steps.append({'step': 'dump_' + name, 'code': p.returncode, 'seconds': results[name]['seconds']})
        running = still
        time.sleep(0.2)
    return results


def jsonl(text):
    out = []
    for line in text.splitlines():
        line = line.strip()
        if line.startswith('{'):
            try:
                out.append(json.loads(line))
            except ValueError:
                pass
    return out


def median(v):
    v = sorted(v)
    n = len(v)
    if n == 0:
        return float('nan')
    return v[n // 2] if n % 2 else 0.5 * (v[n // 2 - 1] + v[n // 2])


def is_int(x):
    return isinstance(x, int) and not isinstance(x, bool)


def timings(f):
    """Durees d'une forme : liste non vide de reels finis strictement positifs, sinon None (prise invalide)."""
    if not isinstance(f, dict) or not isinstance(f.get('ms'), list) or not f['ms']:
        return None
    values = []
    for x in f['ms']:
        if isinstance(x, bool) or not isinstance(x, (int, float)) or not math.isfinite(x) or x <= 0:
            return None
        values.append(float(x))
    return values


def deciding(name, decide):
    """Vrai si le cas (nom de vidage ngXX_kK_lL) appartient aux configurations qui decident."""
    parts = name.split('_')
    if len(parts) != 3:
        return True
    try:
        return (int(parts[1][1:]), int(parts[2][1:])) in decide
    except ValueError:
        return True


def form_base(form):
    """Forme de base d'une variante (les bornes de registres partagent la source et l'identite hote)."""
    for base in ('j3', 'coherent'):
        if form == base or form.startswith(base + '_'):
            return base
    return None


def judge(cases, forms, processes_required, decide=CONTRACT['decide'], expected=None):
    """Rapports de medianes par processus, moyenne geometrique, IC 95 % par bootstrap sur les processus.

    Seuls les cas des configurations `decide` (feuilles 24 : configuration GPU de la v11) decident ; les autres sont
    publies (champ deciding faux) et ne changent que l'identite exigee partout. Refus (jamais adoption ni rejet) :
    cas attendu absent, aucun cas qui decide, prise invalide ou duree non finie (moins de `processes_required`
    processus valides sur un cas, quel qu'il soit), temoin faux. Statistique inchangee (meme generateur, meme ordre
    de tirage) : sur des prises completes, les nombres sont ceux du juge ecrit avant la mesure."""
    rng = random.Random(SEED)
    verdicts = {}
    names = sorted(cases)
    expected = sorted(set(expected)) if expected is not None else names
    missing = [n for n in expected if n not in cases]
    witness_ok = bool(cases) and all(
        c.get('runs') and all(run.get('witness_identity') is True for run in c['runs']) for c in cases.values())
    for form in forms:
        if form == 'witness':
            continue
        per_case, reasons_refused, reasons_rejected = {}, [], []
        for name in missing:
            reasons_refused.append('%s : cas attendu absent' % name)
        if not any(deciding(name, decide) for name in expected):
            reasons_refused.append('aucun cas qui decide parmi les cas joues')
        for name in names:
            c = cases[name]
            decides = deciding(name, decide)
            logs, identity, unresolved = [], True, 0
            for run in c.get('runs', []):
                forms_run = run.get('forms', {})
                f, w = forms_run.get(form), forms_run.get('witness')
                mf, mw = timings(f), timings(w)
                if mf is None or mw is None:
                    continue
                logs.append(math.log(median(mf) / median(mw)))
                identity = identity and f.get('identity') is True
                value = f.get('unresolved', 0)
                unresolved += value if is_int(value) else 0
            if len(logs) < processes_required:
                reasons_refused.append('%s : %d processus valides sur %d exiges' % (name, len(logs),
                                                                                   processes_required))
                continue
            gm = math.exp(sum(logs) / len(logs))
            boot = []
            for _ in range(BOOTSTRAP):
                sample = [logs[rng.randrange(len(logs))] for _ in logs]
                boot.append(sum(sample) / len(sample))
            boot.sort()
            lo, hi = math.exp(boot[int(0.025 * BOOTSTRAP)]), math.exp(boot[int(0.975 * BOOTSTRAP) - 1])
            per_case[name] = {'ratio_gm': gm, 'ci95': [lo, hi], 'processes': len(logs), 'identity': identity,
                              'unresolved': unresolved, 'ratios': [math.exp(x) for x in logs], 'deciding': decides}
            if not identity:
                reasons_rejected.append('%s : identite en defaut' % name)  # l'identite est exigee partout
            elif decides and hi > THRESHOLD:
                reasons_rejected.append('%s : borne haute %.3f > 1/3' % (name, hi))
        if not witness_ok:
            reasons_refused.append('temoin : totaux differents de la reference ou prise manquante')
        if reasons_refused:
            verdict = 'refuse'
        elif reasons_rejected:
            verdict = 'rejete'
        else:
            verdict = 'adopte'
        overall, informative = None, None
        decisive = [v for v in per_case.values() if v['deciding']]
        if decisive:
            overall = math.exp(sum(math.log(v['ratio_gm']) for v in decisive) / len(decisive))
        others = [v for v in per_case.values() if not v['deciding']]
        if others:
            informative = {'ratio_gm': math.exp(sum(math.log(v['ratio_gm']) for v in others) / len(others)),
                           'all_upper_bounds_below_threshold': all(v['ci95'][1] <= THRESHOLD for v in others)}
        verdicts[form] = {'verdict': verdict, 'refused': reasons_refused, 'rejected': reasons_rejected,
                          'ratio_gm_all_cases': overall, 'informative_cases': informative, 'cases': per_case}
    adopted = [f for f, v in verdicts.items() if v['verdict'] == 'adopte' and v['ratio_gm_all_cases'] is not None
               and math.isfinite(v['ratio_gm_all_cases'])]
    choice = min(adopted, key=lambda f: verdicts[f]['ratio_gm_all_cases']) if adopted else None
    return verdicts, choice


def form_counters_consistent(row, reference_records, reference_population):
    """Compteurs d'une forme coherents avec son identite (definitions du banc, leaf_bench.cu : identite du temoin =
    aucun ecart de compteurs ni d'emissions ; identite d'une forme warp = pas de debordement, aucun ecart, et, sans
    feuille non resolue, arene de la taille exacte de reference). Rend None ou la raison du desaccord (CST-0018)."""
    for key in ('mismatched_counts', 'mismatched_emissions', 'records', 'population'):
        if not is_int(row.get(key)) or row[key] < 0:
            return '%s illisible' % key
    if not isinstance(row.get('overflow'), bool):
        return 'debordement illisible'
    if row['identity'] is True:
        if row['mismatched_counts'] != 0 or row['mismatched_emissions'] != 0 or row['overflow']:
            return 'identite declaree malgre des ecarts de compteurs ou un debordement'
        if row['unresolved'] == 0 and (row['records'] != reference_records or
                                       row['population'] != reference_population):
            return 'identite declaree avec une arene de taille differente de la reference'
    return None


def check_bench_json(path, nonce, case, ident, forms, reps, warmup, leaves=None):
    """Prise du banc : fichier neuf de la session (jeton), rattache a son vidage (empreinte, profil, comptes), formes et
    repetitions exactement celles de la commande, durees finies positives, compteurs coherents avec l'identite de chaque
    forme. leaves : couverture demandee par --leaves (Compute Sanitizer : premieres feuilles) ; None ou 0 = toutes les
    feuilles, comptes de reference egaux a ceux du vidage. Rend (prise, None) ou (None, raison)."""
    if not path.is_file():
        return None, 'fichier de prise absent'
    try:
        result = json.loads(path.read_text())
    except (OSError, ValueError) as e:
        return None, 'prise illisible : %s' % e
    if not isinstance(result, dict) or result.get('bench') != 'mhgp12_leaf_bench':
        return None, 'prise d un autre banc'
    if result.get('nonce') != nonce:
        return None, 'jeton de session absent ou different (prise perimee)'
    if result.get('reps') != reps or result.get('warmup') != warmup:
        return None, 'repetitions ou echauffement differents de la commande'
    cases = result.get('cases')
    if not isinstance(cases, list) or len(cases) != 1 or not isinstance(cases[0], dict):
        return None, 'prise sans exactement un cas'
    c = cases[0]
    if c.get('dump') != str(case):
        return None, 'prise d un autre vidage (chemin)'
    expect = {'dump_fnv1a': ident['fnv1a'], 'coord_bits': ident['coord_bits'], 'kmax': ident['kmax'],
              'leaf_size': ident['leaf_size'], 'dump_leaves': ident['n_leaves'], 'sites': ident['n_sites']}
    for key, value in expect.items():
        if c.get(key) != value:
            return None, 'prise non rattachee a son vidage (%s)' % key
    covered = ident['n_leaves'] if not leaves else min(leaves, ident['n_leaves'])
    records, population = c.get('reference_records'), c.get('reference_population')
    if covered < 1 or c.get('leaves') != covered:
        return None, 'couverture de la prise (%r feuilles) differente de la commande (%d)' % (c.get('leaves'), covered)
    if covered == ident['n_leaves']:
        if records != ident['n_records'] or population != ident['n_population']:
            return None, 'prise partielle ou comptes differents du vidage'
    elif not (is_int(records) and is_int(population) and 0 <= records <= ident['n_records'] and
              0 <= population <= ident['n_population']):
        return None, 'comptes de reference de la couverture hors du vidage'
    rows = c.get('forms')
    if not list(forms) or not isinstance(rows, list) or \
            [r.get('form') if isinstance(r, dict) else None for r in rows] != list(forms):
        return None, 'formes absentes ou differentes de la commande'
    out = {}
    for r in rows:
        if timings(r) is None or len(r['ms']) != reps:
            return None, '%s : durees absentes, non finies, non positives ou en nombre faux' % r['form']
        if not isinstance(r.get('identity'), bool) or not is_int(r.get('unresolved')) or r['unresolved'] < 0:
            return None, '%s : identite ou feuilles non resolues illisibles' % r['form']
        why = form_counters_consistent(r, records, population)
        if why is not None:
            return None, '%s : %s' % (r['form'], why)
        out[r['form']] = r
    return {'result': result, 'forms': out, 'identity': all(r['identity'] for r in rows)}, None


def run_json(s, name, cmd, target, timeout):
    """Lance une commande qui ecrit target : la cible est effacee avant (aucune prise perimee ne peut survivre)."""
    try:
        if target.exists():
            target.unlink()
    except OSError as e:
        return None, '', 'cible perimee non effacable : %s' % e
    return s.run(name, cmd, timeout)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--out', required=True)
    ap.add_argument('--data', help='dossier des trames lidar_ngXX.u32le / .ids.u32le (hors depot)')
    ap.add_argument('--dumps', help='vidages MHGP12LF deja faits (sinon construits depuis --data)')
    ap.add_argument('--repo', help='racine du depot (contient morsehgp3D_v11)')
    ap.add_argument('--frames', default=','.join(CONTRACT['frames']))
    ap.add_argument('--configs', default='5:16,5:24,10:24')
    ap.add_argument('--decide', default='5:24,10:24',
                    help='configurations qui decident (feuilles 24 : configuration GPU de la v11) ; les autres sont publiees')
    ap.add_argument('--forms', default=','.join(DEFAULT_FORMS))
    ap.add_argument('--processes', type=int, default=5)
    ap.add_argument('--reps', type=int, default=15)
    ap.add_argument('--warmup', type=int, default=3)
    ap.add_argument('--jobs', type=int, default=max(1, (os.cpu_count() or 8) - 2))
    ap.add_argument('--no-cuda', action='store_true', help='outils hote seulement (essai local sans GPU)')
    ap.add_argument('--skip-identity', action='store_true')
    ap.add_argument('--skip-sanitizer', action='store_true')
    ap.add_argument('--sanitizer-leaves', type=int, default=3000)
    ap.add_argument('--nvcc')
    ap.add_argument('--cmake', default=shutil.which('cmake') or 'cmake')
    args = ap.parse_args()

    repo = Path(args.repo).resolve() if args.repo else find_repo(HERE)
    if repo is None or not (repo / 'morsehgp3D_v11' / 'src' / 'catalogue' / 'leaf.cpp').is_file():
        print('depot introuvable : --repo (racine contenant morsehgp3D_v11)', file=sys.stderr)
        return 2
    try:
        frames = [f for f in args.frames.split(',') if f]
        configs = [tuple(int(x) for x in c.split(':')) for c in args.configs.split(',') if c]
        decide = [tuple(int(x) for x in c.split(':')) for c in args.decide.split(',') if c]
    except ValueError:
        print('--configs et --decide : K:feuille separes par des virgules', file=sys.stderr)
        return 2
    if any(len(c) != 2 for c in configs + decide) or args.processes < 1 or args.reps < 1 or args.warmup < 0:
        print('arguments invalides', file=sys.stderr)
        return 2
    forms = [f for f in args.forms.split(',') if f]
    if 'witness' not in forms:
        forms.insert(0, 'witness')
    if len(set(forms)) != len(forms) or any(f != 'witness' and form_base(f) is None for f in forms):
        print('--forms : formes inconnues ou repetees', file=sys.stderr)
        return 2
    if args.dumps is None and args.data is None:
        print('--data ou --dumps requis', file=sys.stderr)
        return 2

    s = Session(args.out)
    work = s.out / 'work'
    work.mkdir(parents=True, exist_ok=True)
    nonce = 'm2-%s-%s' % (time.strftime('%Y%m%dT%H%M%SZ', time.gmtime()), os.urandom(6).hex())
    report = {'bench': 'MES-M2 feuille GPU (v12, hors produit)', 'started_utc': now(), 'nonce': nonce,
              'frame': {'phase': 'exploration_v12_hors_registre', 'backend': 'cuda_g4 (microbanc)',
                        'quantification': 'quantized_u21_input_only', 'public_status': 'not_claimed'},
              'rule': {'threshold': THRESHOLD, 'statistic': 'moyenne geometrique des rapports de medianes par '
                       'processus (forme / temoin), IC 95 % bootstrap sur les processus, par cas',
                       'adopt': 'identite sur tous les cas et borne haute <= 1/3 sur chaque cas qui decide ; toutes '
                       'les preuves presentes et fraiches (CST-0018, CST-0215)',
                       'deciding_configs': args.decide, 'processes_required': args.processes,
                       'contract': {'frames': list(CONTRACT['frames']),
                                    'configs': ['%d:%d' % c for c in CONTRACT['configs']],
                                    'decide': ['%d:%d' % c for c in CONTRACT['decide']],
                                    'processes_min': CONTRACT['processes_min'], 'sanitizers': list(SANITIZERS)}},
              'args': vars(args), 'repo': str(repo), 'feuille_dir': str(HERE)}
    # Ecarts au contrat ecrit d'avance : mesure publiee, adoption interdite.
    contract = []
    if any(f not in frames for f in CONTRACT['frames']):
        contract.append('trames %s : le contrat exige %s' % (','.join(frames), ','.join(CONTRACT['frames'])))
    if any(c not in configs for c in CONTRACT['configs']):
        contract.append('configurations %s : le contrat exige 5:16,5:24,10:24' % args.configs)
    if sorted(set(decide)) != sorted(CONTRACT['decide']):
        contract.append('cas qui decident %s : le contrat fixe 5:24,10:24' % args.decide)
    if args.processes < CONTRACT['processes_min']:
        contract.append('%d processus : le contrat en exige au moins %d' % (args.processes,
                                                                          CONTRACT['processes_min']))
    if args.skip_identity:
        contract.append('identite hote non jouee (--skip-identity)')
    if args.skip_sanitizer:
        contract.append('sanitizers non joues (--skip-sanitizer)')
    if args.no_cuda:
        contract.append('banc CUDA non joue (--no-cuda)')
    for reason in contract:
        s.refuse('hors contrat : ' + reason)
    report['contract_deviations'] = contract

    nvcc = None if args.no_cuda else find_nvcc(args.nvcc)
    report['environment'] = environment(s, nvcc, args.cmake)
    gpu_apps = report['environment'].get('gpu_apps')
    isolation_start = isinstance(gpu_apps, str) and gpu_apps.strip() == ''
    code, out, _ = s.run('git_head', ['git', '-C', repo, 'rev-parse', 'HEAD'], 60)
    report['repo_head'] = out.strip() if code == 0 else None

    # Donnees : empreintes des trames (aucune coordonnee dans le rapport).
    if args.data:
        data = Path(args.data)
        report['data'] = {}
        for frame in frames:
            for suffix in ('.u32le', '.ids.u32le'):
                p = data / ('lidar_%s%s' % (frame, suffix))
                report['data'][p.name] = sha256_file(p) if p.is_file() else None

    jobs = max(1, args.jobs)
    v11_lib = None
    if args.dumps is None:
        v11_lib = build_v11(s, repo, work, args.cmake, jobs)
        if v11_lib is None:
            s.refuse('construction de la bibliotheque v11 en echec')
    bdir = build_feuille(s, repo, work, args.cmake, jobs, nvcc, v11_lib, nvcc is not None)
    if bdir is None:
        s.refuse('construction du microbanc en echec')
    if nvcc is None and not args.no_cuda:
        s.refuse('nvcc introuvable')

    # Empreintes des binaires et des sources du microbanc (rehachees en fin de session).
    def binary_hashes():
        out = {}
        if bdir is not None:
            for b in ('mhgp12_leaf_dump',) + REQUIRED_BINARIES:
                p = bdir / b
                if p.is_file():
                    out[b] = sha256_file(p)
        return out

    out_dir = s.out.resolve()

    def source_hashes():
        out = {}
        for p in sorted(HERE.rglob('*')):
            if p.is_file() and p.suffix in ('.hpp', '.cpp', '.cu', '.py', '.txt') and 'build' not in p.parts \
                    and 'dumps' not in p.parts and 'results' not in p.parts and '__pycache__' not in p.parts \
                    and out_dir not in p.resolve().parents:
                out['src/' + str(p.relative_to(HERE))] = sha256_file(p)
        for rel in ('src/catalogue/leaf.cpp', 'src/catalogue/leaf_device.hpp',
                    'src/catalogue/leaf_device_predicates.hpp', 'src/catalogue/leaf_batch.hpp',
                    'src/catalogue/leaf_batch_cuda.cu'):
            p = repo / 'morsehgp3D_v11' / rel
            out['v11/' + rel] = sha256_file(p) if p.is_file() else None
        return out

    binaries = binary_hashes()
    hashes = dict(binaries)
    hashes.update(source_hashes())
    report['hashes'] = hashes
    dep_roots = [HERE, repo / 'morsehgp3D_v11']
    dependencies = compiled_dependencies(bdir, dep_roots) if bdir is not None else {}
    report['compiled_dependencies'] = {'roots': ['0: microbanc', '1: morsehgp3D_v11'], 'files': dependencies,
                                       'note': 'releve des fichiers .d du compilateur ; vide si le generateur '
                                               'n\'en ecrit pas'}
    for b in REQUIRED_BINARIES:
        if b == 'mhgp12_leaf_bench' and args.no_cuda:
            continue
        if b not in binaries:
            s.refuse('binaire non construit ou non hache : ' + b)
    if args.dumps is None and 'mhgp12_leaf_dump' not in binaries:
        s.refuse('binaire non construit ou non hache : mhgp12_leaf_dump')

    # Porte du lecteur : vidage valide admis, mutants a empreinte juste refuses (binaire de cette session).
    admission_ok = False
    if bdir is not None and 'mhgp12_dump_admission_selftest' in binaries:
        folder = work / 'admission_selftest'
        shutil.rmtree(folder, ignore_errors=True)
        folder.mkdir(parents=True)
        code, out, _ = s.run('admission_selftest', [bdir / 'mhgp12_dump_admission_selftest', folder], 600)
        rows = jsonl(out)
        valid = [r for r in rows if r.get('cas') == 'valide']
        mutants = [r for r in rows if r.get('cas') != 'valide']
        admission_ok = (code == 0 and len(valid) == 1 and valid[0].get('admis') is True and len(mutants) >= 40 and
                        all(r.get('admis') is False for r in mutants))
        report['admission_selftest'] = {'code': code, 'cases': len(rows), 'mutants_refused':
                                        sum(r.get('admis') is False for r in mutants), 'conform': admission_ok}
        shutil.rmtree(folder, ignore_errors=True)
    if not admission_ok:
        s.refuse('porte d admission des vidages absente ou en echec')

    # Vidages : construits dans la session (cible effacee avant, code 0 exige) ou fournis (--dumps).
    if args.dumps:
        dumps = Path(args.dumps)
        report['dumps_provenance'] = 'fournis (--dumps), admis et haches dans la session'
    else:
        dumps = s.out / 'dumps'
        report['dumps_provenance'] = 'construits dans la session'
        tool = bdir / 'mhgp12_leaf_dump' if bdir else None
        if tool is not None and tool.is_file() and args.data:
            report['dumps_made'] = make_dumps(s, tool, Path(args.data), frames, configs, dumps, jobs)
            for name, made in sorted(report['dumps_made'].items()):
                if made.get('code') != 0:
                    s.refuse('vidage en echec : %s (code %s)' % (name, made.get('code')))
        else:
            s.refuse('outil de vidage absent')
    cases, idents = [], {}
    for frame in frames:
        for k, leaf in configs:
            p = dumps / ('%s_k%d_l%d.bin' % (frame, k, leaf))
            made = report.get('dumps_made', {}).get(p.stem)
            if made is not None and made.get('code') != 0:
                continue
            ident = dump_identity(p) if p.is_file() else None
            if ident is None:
                s.refuse('vidage absent ou illisible : ' + p.name)
                continue
            if ident['magic'] != 'MHGP12LF' or ident['version'] != 1 or ident['coord_bits'] != PROFILE_BITS or \
                    ident['kmax'] != k or ident['leaf_size'] != leaf:
                s.refuse('vidage %s : en-tete (profil, K, feuille) different du cas' % p.name)
                continue
            cases.append(p)
            idents[p.name] = ident
    report['dump_hashes'] = {name: ident['sha256'] for name, ident in idents.items()}
    report['dump_identity'] = idents

    # Admission de tous les vidages par le lecteur C++ avant tout noyau.
    admitted = False
    if bdir is not None and 'mhgp12_leaf_identity' in binaries and cases:
        code, out, _ = s.run('admission_vidages', [bdir / 'mhgp12_leaf_identity', '--admission'] + cases, 1800)
        rows = {r.get('dump'): r for r in jsonl(out)}
        bad = []
        for p in cases:
            r = rows.get(str(p))
            if r is None or r.get('admis') is not True or r.get('dump_fnv1a') != idents[p.name]['fnv1a']:
                bad.append(p.name)
        admitted = code == 0 and not bad
        report['admission'] = {'code': code, 'admitted': len(cases) - len(bad), 'refused': bad,
                               'reasons': [r.get('raison') for r in rows.values() if r.get('admis') is False]}
    if not admitted:
        s.refuse('admission des vidages en echec ou non jouee : aucun noyau sur un vidage non admis')
        cases_run = []
    else:
        cases_run = cases

    # Identite hote (formes de base), auto-test de la verification d'arene, MES-S.
    bases = sorted({form_base(f) for f in forms if f != 'witness'})
    if bdir is not None and cases_run:
        if not args.skip_identity:
            code, out, _ = s.run('identity_host', [bdir / 'mhgp12_leaf_identity', '--forms', ','.join(bases),
                                                   '--threads', str(min(jobs, 64))] + cases_run, 7200)
            rows = jsonl(out)
            report['identity_host'] = {'code': code, 'results': rows}
            problems = [] if code == 0 else ['code %d' % code]
            for p in cases_run:
                ident = idents[p.name]
                for base in bases:
                    found = [r for r in rows if r.get('dump') == str(p) and r.get('form') == base]
                    if len(found) != 1:
                        problems.append('%s/%s : ligne absente' % (p.name, base))
                        continue
                    r = found[0]
                    if r.get('identity') is not True or r.get('mismatched_counts') != 0 or \
                            r.get('mismatched_emissions') != 0 or r.get('leaves') != ident['n_leaves'] or \
                            r.get('dump_leaves') != ident['n_leaves'] or r.get('dump_fnv1a') != ident['fnv1a']:
                        problems.append('%s/%s : identite, couverture ou empreinte en defaut' % (p.name, base))
            report['identity_host']['problems'] = problems
            if problems:
                s.refuse('identite hote : ' + '; '.join(problems[:6]))
        code, out, _ = s.run('arena_selftest', [bdir / 'mhgp12_arena_selftest', cases_run[0], '20000'], 1800)
        rows = jsonl(out)
        report['arena_selftest'] = {'code': code, 'results': rows}
        arena_ok = code == 0 and len(rows) == 1 and rows[0].get('identity') is True and \
            rows[0].get('mutants_vivants') == 0 and rows[0].get('dump_fnv1a') == idents[cases_run[0].name]['fnv1a']
        if not arena_ok:
            s.refuse('auto-test de la verification d arene en echec ou sans preuve')
        code, out, _ = s.run('mes_s', [bdir / 'mhgp12_mes_s'] + cases_run, 1800)
        report['mes_s'] = {'code': code, 'results': jsonl(out)}  # publie (contrat numerique), hors regle d'adoption

    # Compute Sanitizer (memcheck, racecheck, synccheck) sur les premieres feuilles d'un cas, formes v12 seulement
    # (contrat CUDA de l'auditeur v11 : voies m = 1..32, votes et __syncwarp). Preuve exigee : un defaut ou une
    # absence refuse le banc.
    sanitizer = None
    if nvcc is not None:
        cand = Path(nvcc).parent / 'compute-sanitizer'
        sanitizer = cand if cand.is_file() else shutil.which('compute-sanitizer')
    bench_bin = bdir / 'mhgp12_leaf_bench' if bdir else None
    report['sanitizer'] = {}
    if not args.skip_sanitizer and not args.no_cuda:
        if sanitizer is None:
            s.refuse('compute-sanitizer introuvable')
        elif bench_bin is not None and bench_bin.is_file() and cases_run:
            small = [c for c in cases_run if c.name.endswith('_k5_l24.bin')] or cases_run
            warp_forms = [f for f in forms if f != 'witness']
            for tool in SANITIZERS:
                target = s.out / ('sanitizer_%s.json' % tool)
                code, out, err = run_json(s, 'sanitizer_' + tool,
                                          [sanitizer, '--tool', tool, '--error-exitcode', '9', bench_bin, '--dump',
                                           small[0], '--forms', ','.join(warp_forms), '--reps', '1', '--warmup',
                                           '0', '--leaves', str(args.sanitizer_leaves), '--json', target,
                                           '--nonce', nonce], target, 3600)
                # Preuve : aucun defaut signale (code 0, ou 1 si le banc lui-meme constate un ecart d'identite, juge
                # par les prises du banc ; 9 = defaut du sanitizer) et prise neuve de la session sur le bon vidage,
                # validee comme toute prise (CST-0018) : formes et repetitions de la commande, couverture des
                # --sanitizer-leaves premieres feuilles, compteurs coherents, code et identite concordants.
                taken, why = check_bench_json(target, nonce, small[0], idents[small[0].name], warp_forms, 1, 0,
                                              leaves=args.sanitizer_leaves) if code in (0, 1) \
                    else (None, 'code %s' % code)
                if taken is not None and (code == 0) != taken['identity']:
                    taken, why = None, 'code %s et identite discordants' % code
                ok = taken is not None
                report['sanitizer'][tool] = {'code': code, 'tail': ((out or '') + (err or ''))[-600:], 'proof': ok,
                                             'reason': why, 'identity': taken['identity'] if ok else None,
                                             'json_sha256': sha256_file(target) if target.is_file() else None}
                if not ok:
                    s.refuse('compute-sanitizer %s : code %s ou prise sans preuve (%s)' % (tool, code, why))

    # Banc : processus x cas, ordre des cas tournant ; prises neuves, rattachees a la session et a leur vidage.
    bench_cases = {}
    bench = bench_bin
    isolation_before, isolation_after = None, None
    if nvcc is not None and bench is not None and bench.is_file() and cases_run:
        runs_dir = s.out / 'runs'
        runs_dir.mkdir(exist_ok=True)
        isolation_before = gpu_quiet(s, 'avant_banc')
        # Prise d'echauffement jetee (MESURE.md § 5 : premier processus GPU jete), sur le plus petit vidage.
        smallest = min(cases_run, key=lambda c: idents[c.name]['octets'])
        target = runs_dir / 'discarded.json'
        code, _, err = run_json(s, 'bench_discarded', [bench, '--dump', smallest, '--forms', ','.join(forms),
                                                       '--reps', '1', '--warmup', '1', '--json', target,
                                                       '--nonce', nonce], target, 1800)
        taken, why = check_bench_json(target, nonce, smallest, idents[smallest.name], forms, 1, 1) \
            if code in (0, 1) else (None, 'code %s' % code)
        # Memes preuves qu'une prise normale (CST-0018) : code et identite concordants, temoin identique.
        if taken is not None and (code == 0) != taken['identity']:
            taken, why = None, 'code %s et identite discordants' % code
        elif taken is not None and taken['forms']['witness']['identity'] is not True:
            taken, why = None, 'temoin different de la reference'
        report['bench_discarded'] = {'code': code, 'proof': taken is not None, 'reason': why}
        if taken is None:
            s.refuse('prise d echauffement en echec : %s' % why)
        for p in range(args.processes):
            order = cases_run[p % len(cases_run):] + cases_run[:p % len(cases_run)]
            for case in order:
                name = case.stem
                target = runs_dir / ('%s_p%d.json' % (name, p))
                code, _, err = run_json(s, 'bench_%s_p%d' % (name, p),
                                        [bench, '--dump', case, '--forms', ','.join(forms), '--reps', str(args.reps),
                                         '--warmup', str(args.warmup), '--json', target, '--nonce', nonce],
                                        target, 1800)
                entry = bench_cases.setdefault(name, {'runs': [], 'failures': []})
                if code not in (0, 1):
                    entry['failures'].append({'process': p, 'code': code, 'reason': 'code de sortie',
                                              'stderr': (err or '')[-400:]})
                    continue
                taken, why = check_bench_json(target, nonce, case, idents[case.name], forms, args.reps, args.warmup)
                if taken is None:
                    entry['failures'].append({'process': p, 'code': code, 'reason': why})
                    continue
                if (code == 0) != taken['identity']:
                    entry['failures'].append({'process': p, 'code': code, 'reason': 'code et identite discordants'})
                    continue
                if taken['forms']['witness']['identity'] is not True:
                    entry['failures'].append({'process': p, 'code': code, 'reason': 'temoin different de la reference'})
                    continue
                result = taken['result']
                entry['runs'].append({'process': p, 'code': code, 'device': result.get('device'),
                                      'context_ms': result.get('context_ms'), 'forms': taken['forms'],
                                      'witness_identity': True, 'json': target.name,
                                      'json_sha256': sha256_file(target)})
        isolation_after = gpu_quiet(s, 'apres_banc')
    elif nvcc is None:
        s.refuse('banc CUDA non joue')
    report['bench'] = bench_cases
    for name, entry in sorted(bench_cases.items()):
        if entry['failures']:
            reasons = sorted({f['reason'] for f in entry['failures']})
            s.refuse('%s : %d processus en echec ou sans preuve (%s)' % (name, len(entry['failures']),
                                                                         '; '.join(reasons)))
    isolation = isolation_start and isolation_before is not None and isolation_before[0] and \
        isolation_after is not None and isolation_after[0]
    report['gpu_isolation'] = isolation
    report['gpu_isolation_detail'] = {'start': isolation_start,
                                      'before_bench': isolation_before[1] if isolation_before else None,
                                      'after_bench': isolation_after[1] if isolation_after else None}
    if not isolation:
        s.refuse('isolation GPU non certifiee (processus de calcul presents, nvidia-smi illisible ou releve absent)')

    # Fin de session : binaires, sources et vidages inchanges depuis leur empreinte.
    end_binaries = binary_hashes()
    if end_binaries != binaries:
        s.refuse('binaire modifie pendant la session')
    end_sources = source_hashes()
    if any(hashes.get(k) != v for k, v in end_sources.items()):
        s.refuse('sources modifiees pendant la session')
    if bdir is not None and any(compiled_dependencies(bdir, dep_roots).get(k) != v for k, v in dependencies.items()):
        s.refuse('dependance compilee modifiee pendant la session')
    changed = [p.name for p in cases if (sha256_file(p) if p.is_file() else None) != idents[p.name]['sha256']]
    if changed:
        s.refuse('vidages modifies pendant la session : ' + ', '.join(changed))

    decide_names = ['%s_k%d_l%d' % (f, k, l) for f in CONTRACT['frames'] for (k, l) in CONTRACT['decide']]
    expected = ['%s_k%d_l%d' % (f, k, l) for f in frames for (k, l) in configs]
    verdicts, choice = judge(bench_cases, forms, args.processes, tuple(decide), expected)
    for name in decide_names:
        if name not in bench_cases or not bench_cases[name]['runs']:
            s.refuse('cas qui decide du contrat sans prise : ' + name)
    if s.refusals:
        for v in verdicts.values():
            if v['verdict'] != 'refuse':
                v['verdict'] = 'refuse'
            v['refused'] = sorted(set(v['refused'] + s.refusals))
        choice = None
    if not verdicts:
        verdicts = {f: {'verdict': 'refuse', 'refused': list(s.refusals)} for f in forms if f != 'witness'}
    # Preuves citees par chaque verdict : vidages, prises, journaux (fichiers et empreintes).
    evidence = {'nonce': nonce,
                'dumps': {name: {'sha256': ident['sha256'], 'fnv1a': ident['fnv1a']} for name, ident in idents.items()},
                'runs': {'runs/' + run['json']: run['json_sha256'] for entry in bench_cases.values()
                         for run in entry['runs']},
                'logs': {}}
    for step in ('admission_vidages', 'identity_host', 'arena_selftest', 'admission_selftest'):
        log = s.logs / (step + '.log')
        if log.is_file():
            evidence['logs']['logs/' + log.name] = sha256_file(log)
    for tool, entry in report['sanitizer'].items():
        if entry.get('json_sha256'):
            evidence['logs']['sanitizer_%s.json' % tool] = entry['json_sha256']
    for v in verdicts.values():
        v['evidence'] = 'report.evidence (nonce %s, %d prises, %d vidages)' % (nonce, len(evidence['runs']),
                                                                              len(evidence['dumps']))
    report['evidence'] = evidence
    report['binaries_end'] = end_binaries
    report['verdicts'] = verdicts
    report['choice'] = choice
    report['refusals'] = s.refusals
    report['steps'] = s.steps
    report['finished_utc'] = now()
    (s.out / 'report.json').write_text(json.dumps(report, indent=1, sort_keys=True))
    summary = {f: (v['verdict'], v.get('ratio_gm_all_cases')) for f, v in verdicts.items()}
    print(json.dumps({'report': str(s.out / 'report.json'), 'choice': choice, 'verdicts': summary,
                      'refusals': s.refusals}))
    return 0


if __name__ == '__main__':
    sys.exit(main())
