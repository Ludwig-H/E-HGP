#!/usr/bin/env python3
"""MES-M2 sur G4 : feuille du catalogue sur le GPU (microbanc hors produit de la v12).

Une seule entree pour la session : construit, vide les feuilles de la v11, verifie l'identite sur l'hote, mesure
l'etendue locale (MES-S), joue le banc CUDA en plusieurs processus et juge, puis ecrit un rapport JSON. Bibliotheque
standard seulement (Python 3.10 de la VM, sans numpy) ; aucune commande GCP.

Etapes (journal par etape dans <out>/logs/) :
  1. versions et environnement (nvcc, cmake, compilateur, nvidia-smi, uptime, processus GPU) ;
  2. si les vidages ne sont pas fournis (--dumps) : construction de la bibliotheque v11 (Release, u21, cible mhgp11)
     et de l'outil de vidage, puis vidage des trames (--data) en parallele ;
  3. construction du banc (CUDA sm_120) et des outils hote ; empreintes sha256 des binaires et des sources ;
  4. identite sur l'hote des deux formes contre leaf.cpp (toutes les feuilles), auto-test de la verification d'arene,
     MES-S, puis Compute Sanitizer (memcheck, racecheck, synccheck) sur les premieres feuilles d'un cas ;
  5. banc : --processes processus par cas, ordre des cas tournant d'un processus a l'autre, formes entrelacees dans
     chaque repetition (dans le banc) ;
  6. juge (regle ecrite d'avance, README.md § 6) et rapport <out>/report.json.

Regle d'adoption (PLAN.md, MES-M2) : comptage <= 1/3 du temoin (noyau un-fil de la v11) et identite avec la feuille de
reference. Decident les cas a feuilles 24 (K5/24 et K10/24, configuration GPU de la v11) ; les cas K5/16 sont mesures et
publies (choix de la taille de feuille), l'identite y est exigee aussi. Trois verdicts par forme : « adopte », « rejete » (mesure ou identite), « refuse » (banc invalide : prise ou
donnee manquante, binaire non hache, temoin faux, isolation GPU non certifiee) ; seul « adopte » permet l'adoption.

Usage :
  python3 scripts/g4_leaf_bench.py --out OUT --data DONNEES [--repo DEPOT] [--dumps VIDAGES] [--processes 5]
          [--reps 15] [--warmup 3] [--frames ng00,ng01,ng02] [--configs 5:16,5:24,10:24] [--forms ...]
          [--jobs N] [--no-cuda] [--nvcc NVCC] [--cmake CMAKE]
Codes : 0 rapport ecrit (quel que soit le verdict), 2 refus avant toute mesure (arguments, depot introuvable).
"""

import argparse
import hashlib
import json
import math
import os
import random
import shutil
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent.parent  # racine du microbanc (v12_feuille)
DEFAULT_FORMS = ['witness', 'j3', 'j3_r168', 'j3_r128', 'coherent', 'coherent_r168', 'coherent_r128']
THRESHOLD = 1.0 / 3.0  # regle d'adoption : comptage <= 1/3 du temoin
BOOTSTRAP = 10000
SEED = 20261007


def now():
    return time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


class Session:
    def __init__(self, out):
        self.out = Path(out)
        self.logs = self.out / 'logs'
        self.logs.mkdir(parents=True, exist_ok=True)
        self.steps = []
        self.refusals = []

    def run(self, name, cmd, timeout, cwd=None, env=None, capture=False):
        """Lance une commande, journal dans logs/<name>.log ; rend (code, sortie standard)."""
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


def environment(s, nvcc, cmake):
    env = {'date_utc': now(), 'python': sys.version.split()[0], 'cpu_count': os.cpu_count()}
    for name, cmd in (('nvcc', [nvcc, '--version'] if nvcc else None), ('cmake', [cmake, '--version']),
                      ('cxx', ['c++', '--version']), ('uptime', ['uptime', '-s']),
                      ('gpu', ['nvidia-smi', '--query-gpu=name,driver_version,persistence_mode,clocks.sm,'
                               'clocks.max.sm,temperature.gpu,power.draw,memory.used,memory.total',
                               '--format=csv,noheader']),
                      ('gpu_apps', ['nvidia-smi', '--query-compute-apps=pid,process_name,used_memory',
                                    '--format=csv,noheader'])):
        if cmd is None:
            env[name] = None
            continue
        code, out, err = s.run('env_' + name, cmd, 60)
        env[name] = out.strip() if code == 0 else {'code': code, 'stderr': err.strip()[-400:]}
    return env


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
    """Vidages en parallele (un processus par vidage, au plus jobs a la fois)."""
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
            xyz, ids = data / ('lidar_%s.u32le' % frame), data / ('lidar_%s.ids.u32le' % frame)
            out = open(s.logs / ('dump_%s.log' % name), 'wb')
            p = subprocess.Popen([str(tool), str(xyz), str(ids), str(k), str(leaf), str(dumps / (name + '.bin'))],
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


def deciding(name, decide):
    """Vrai si le cas (nom de vidage ngXX_kK_lL) appartient aux configurations qui decident."""
    parts = name.split('_')
    if len(parts) != 3:
        return True
    return (int(parts[1][1:]), int(parts[2][1:])) in decide


def judge(cases, forms, processes_required, decide=((5, 24), (10, 24))):
    """Rapports de medianes par processus, moyenne geometrique, IC 95 % par bootstrap sur les processus.

    Seuls les cas des configurations `decide` (feuilles 24 : configuration GPU de la v11) decident ; les autres sont
    publies (champ deciding faux) et ne changent pas le verdict."""
    rng = random.Random(SEED)
    verdicts = {}
    witness_ok = all(all(run['witness_identity'] for run in c['runs']) for c in cases.values()) and bool(cases)
    for form in forms:
        if form == 'witness':
            continue
        per_case, reasons_refused, reasons_rejected = {}, [], []
        for name, c in sorted(cases.items()):
            decides = deciding(name, decide)
            logs, identity, unresolved = [], True, 0
            for run in c['runs']:
                f = run['forms'].get(form)
                w = run['forms'].get('witness')
                if f is None or w is None or not f['ms'] or not w['ms']:
                    continue
                logs.append(math.log(median(f['ms']) / median(w['ms'])))
                identity = identity and f['identity']
                unresolved += f['unresolved']
            if len(logs) < processes_required:
                if decides:
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
    adopted = [f for f, v in verdicts.items() if v['verdict'] == 'adopte']
    choice = min(adopted, key=lambda f: verdicts[f]['ratio_gm_all_cases']) if adopted else None
    return verdicts, choice


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--out', required=True)
    ap.add_argument('--data', help='dossier des trames lidar_ngXX.u32le / .ids.u32le (hors depot)')
    ap.add_argument('--dumps', help='vidages MHGP12LF deja faits (sinon construits depuis --data)')
    ap.add_argument('--repo', help='racine du depot (contient morsehgp3D_v11)')
    ap.add_argument('--frames', default='ng00,ng01,ng02')
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
    frames = [f for f in args.frames.split(',') if f]
    configs = []
    for c in args.configs.split(','):
        k, leaf = c.split(':')
        configs.append((int(k), int(leaf)))
    forms = [f for f in args.forms.split(',') if f]
    if 'witness' not in forms:
        forms.insert(0, 'witness')
    if args.dumps is None and args.data is None:
        print('--data ou --dumps requis', file=sys.stderr)
        return 2

    s = Session(args.out)
    work = s.out / 'work'
    work.mkdir(parents=True, exist_ok=True)
    report = {'bench': 'MES-M2 feuille GPU (v12, hors produit)', 'started_utc': now(),
              'frame': {'phase': 'exploration_v12_hors_registre', 'backend': 'cuda_g4 (microbanc)',
                        'quantification': 'quantized_u21_input_only', 'public_status': 'not_claimed'},
              'rule': {'threshold': THRESHOLD, 'statistic': 'moyenne geometrique des rapports de medianes par '
                       'processus (forme / temoin), IC 95 % bootstrap sur les processus, par cas',
                       'adopt': 'identite sur tous les cas et borne haute <= 1/3 sur chaque cas qui decide',
                       'deciding_configs': args.decide, 'processes_required': args.processes},
              'args': vars(args), 'repo': str(repo), 'feuille_dir': str(HERE)}
    nvcc = None if args.no_cuda else find_nvcc(args.nvcc)
    report['environment'] = environment(s, nvcc, args.cmake)
    gpu_apps = report['environment'].get('gpu_apps')
    isolation = isinstance(gpu_apps, str) and gpu_apps.strip() == ''
    report['gpu_isolation'] = isolation
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
            s.refusals.append('construction de la bibliotheque v11 en echec')
    bdir = build_feuille(s, repo, work, args.cmake, jobs, nvcc, v11_lib, nvcc is not None)
    if bdir is None:
        s.refusals.append('construction du microbanc en echec')
    if nvcc is None and not args.no_cuda:
        s.refusals.append('nvcc introuvable')

    # Empreintes des binaires et des sources du microbanc.
    hashes = {}
    if bdir is not None:
        for b in ('mhgp12_leaf_dump', 'mhgp12_leaf_identity', 'mhgp12_mes_s', 'mhgp12_arena_selftest',
                  'mhgp12_leaf_bench'):
            p = bdir / b
            if p.is_file():
                hashes[b] = sha256_file(p)
    for p in sorted(HERE.rglob('*')):
        if p.is_file() and p.suffix in ('.hpp', '.cpp', '.cu', '.py', '.txt') and 'build' not in p.parts \
                and 'dumps' not in p.parts and 'results' not in p.parts:
            hashes['src/' + str(p.relative_to(HERE))] = sha256_file(p)
    for rel in ('src/catalogue/leaf.cpp', 'src/catalogue/leaf_device.hpp', 'src/catalogue/leaf_device_predicates.hpp',
                'src/catalogue/leaf_batch.hpp', 'src/catalogue/leaf_batch_cuda.cu'):
        p = repo / 'morsehgp3D_v11' / rel
        hashes['v11/' + rel] = sha256_file(p) if p.is_file() else None
    report['hashes'] = hashes

    # Vidages.
    if args.dumps:
        dumps = Path(args.dumps)
    else:
        dumps = s.out / 'dumps'
        tool = bdir / 'mhgp12_leaf_dump' if bdir else None
        if tool is not None and tool.is_file():
            report['dumps_made'] = make_dumps(s, tool, Path(args.data), frames, configs, dumps, jobs)
        else:
            s.refusals.append('outil de vidage absent')
    cases = []
    for frame in frames:
        for k, leaf in configs:
            p = dumps / ('%s_k%d_l%d.bin' % (frame, k, leaf))
            if p.is_file():
                cases.append(p)
            else:
                s.refusals.append('vidage absent : ' + p.name)
    report['dump_hashes'] = {p.name: sha256_file(p) for p in cases}

    # Identite hote, auto-test de la verification d'arene, MES-S.
    if bdir is not None and cases:
        if not args.skip_identity:
            code, out, _ = s.run('identity_host', [bdir / 'mhgp12_leaf_identity', '--threads', str(min(jobs, 64))]
                                 + cases, 7200)
            report['identity_host'] = {'code': code, 'results': jsonl(out)}
        code, out, _ = s.run('arena_selftest', [bdir / 'mhgp12_arena_selftest', cases[0], '20000'], 1800)
        report['arena_selftest'] = {'code': code, 'results': jsonl(out)}
        if code != 0:
            s.refusals.append('auto-test de la verification d arene en echec')
        code, out, _ = s.run('mes_s', [bdir / 'mhgp12_mes_s'] + cases, 1800)
        report['mes_s'] = {'code': code, 'results': jsonl(out)}

    # Compute Sanitizer (memcheck, racecheck, synccheck) sur les premieres feuilles d'un cas, formes v12 seulement
    # (contrat CUDA de l'auditeur v11 : voies m = 1..32, votes et __syncwarp). Informatif : un defaut detecte refuse.
    sanitizer = None
    if nvcc is not None:
        cand = Path(nvcc).parent / 'compute-sanitizer'
        sanitizer = cand if cand.is_file() else shutil.which('compute-sanitizer')
    bench_bin = bdir / 'mhgp12_leaf_bench' if bdir else None
    if not args.skip_sanitizer and sanitizer and bench_bin is not None and bench_bin.is_file() and cases:
        small = [c for c in cases if c.name.endswith('_k5_l24.bin')] or cases
        report['sanitizer'] = {}
        warp_forms = ','.join(f for f in forms if f != 'witness')
        for tool in ('memcheck', 'racecheck', 'synccheck'):
            code, out, err = s.run('sanitizer_' + tool,
                                   [sanitizer, '--tool', tool, '--error-exitcode', '9', bench_bin, '--dump', small[0],
                                    '--forms', warp_forms, '--reps', '1', '--warmup', '0', '--leaves',
                                    str(args.sanitizer_leaves), '--json', s.out / ('sanitizer_%s.json' % tool)], 3600)
            report['sanitizer'][tool] = {'code': code, 'tail': (out + err)[-600:]}
            if code != 0:
                s.refusals.append('compute-sanitizer %s : code %d' % (tool, code))

    # Banc : processus x cas, ordre des cas tournant.
    bench_cases = {}
    bench = bdir / 'mhgp12_leaf_bench' if bdir else None
    if nvcc is not None and bench is not None and bench.is_file() and cases:
        runs_dir = s.out / 'runs'
        runs_dir.mkdir(exist_ok=True)
        # Prise d'echauffement jetee (MESURE.md § 5 : premier processus GPU jete), sur le plus petit vidage.
        smallest = min(cases, key=lambda c: c.stat().st_size)
        s.run('bench_discarded', [bench, '--dump', smallest, '--forms', ','.join(forms), '--reps', '1', '--warmup', '1',
                                  '--json', runs_dir / 'discarded.json'], 1800)
        for p in range(args.processes):
            order = cases[p % len(cases):] + cases[:p % len(cases)]
            for case in order:
                name = case.stem
                target = runs_dir / ('%s_p%d.json' % (name, p))
                code, _, err = s.run('bench_%s_p%d' % (name, p),
                                     [bench, '--dump', case, '--forms', ','.join(forms), '--reps', str(args.reps),
                                      '--warmup', str(args.warmup), '--json', target], 1800)
                entry = bench_cases.setdefault(name, {'runs': [], 'failures': []})
                if not target.is_file() or code not in (0, 1):
                    entry['failures'].append({'process': p, 'code': code, 'stderr': err[-400:]})
                    continue
                result = json.loads(target.read_text())
                c = result['cases'][0]
                forms_out = {}
                for f in c['forms']:
                    forms_out[f['form']] = f
                entry['runs'].append({'process': p, 'code': code, 'device': result.get('device'),
                                      'context_ms': result.get('context_ms'), 'forms': forms_out,
                                      'witness_identity': forms_out.get('witness', {}).get('identity', False)})
    elif nvcc is None:
        s.refusals.append('banc CUDA non joue')
    report['bench'] = bench_cases
    for name, entry in bench_cases.items():
        if entry['failures']:
            s.refusals.append('%s : %d processus en echec' % (name, len(entry['failures'])))
    if not isolation:
        s.refusals.append('isolation GPU non certifiee (processus de calcul presents ou nvidia-smi illisible)')
    decide = []
    for c in args.decide.split(','):
        k, leaf = c.split(':')
        decide.append((int(k), int(leaf)))
    verdicts, choice = judge(bench_cases, forms, args.processes, tuple(decide))
    if s.refusals:
        for v in verdicts.values():
            if v['verdict'] != 'refuse':
                v['verdict'] = 'refuse'
            v['refused'] = sorted(set(v['refused'] + s.refusals))
        choice = None
    if not verdicts:
        verdicts = {f: {'verdict': 'refuse', 'refused': list(s.refusals)} for f in forms if f != 'witness'}
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
