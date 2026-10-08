#!/usr/bin/env python3
"""T1-b sur G4 : voie appareil du catalogue de la v12 (CONTRAT_CATALOGUE.md, paragraphe 10), jugee par une regle ecrite
avant la session. Bibliotheque standard seulement (Python 3.10 nu de la VM) ; aucune commande GCP ; aucune donnee
SemanticKITTI ecrite dans --out (empreintes, comptes et durees seulement ; les entrees restent dans --data).

Etapes (journal par etape dans <out>/logs/, rapport <out>/report.json reecrit apres chaque etape) :
  1. environnement : nvcc, cmake, compilateur, nvidia-smi (GPU, pilote, persistance, processus de calcul), charge ;
  2. construction Release du produit au profil 21 avec MHGP12_ENABLE_CUDA=ON (sm_120) ; empreintes des binaires ;
  3. portes rapides (ctest -LE long) de cette construction ; la porte device_open de mhgp12_catalogue_device_unit y
     joue la VRAIE voie appareil contre la voie CPU sur neuf temoins (feuilles non resolues comprises) ;
  4. identite a l'octet : ng00, ng01, ng02 a K5 et K10 et uniformes de 8 000, 16 000 et 32 000 sites a K5, feuille
     24 : empreinte catalogue_digest (export MHGP12DP), grand livre et comptes de la voie appareil (3 passes d'un meme
     processus : determinisme d'un appel a l'autre) egaux a ceux de la voie CPU (48 fils) ; diagnostics physiques
     (paliers, etendue, reecritures) egaux, reprises sur l'hote de la voie hybride comptees par cause et reecritures
     par lieu ; empreintes de la voie CPU egales a celles de la session F2 (receipts/g4_t1f_20261007 : voie CPU d'avant
     la fin d'etage partagee) ;
  5. temps de la voie CPU apres la fin d'etage parallele : les douze cas de F2, 48 fils, 10 passes (publie, compare a
     F2, ne decide rien) ;
  6. temps de l'etage C a chaud sur l'appareil (MESURE.md, paragraphe 5) : K5 sur ng00, ng01, ng02, 5 processus par
     trame en ordre tournant, 10 passes par processus, passes 2 a 10 ; K10 publie (3 processus, 5 passes) ; etapes
     publiees a part, mediane et maximum (CST-0235) : parcours, feuilles, emission, fin d'etage, transferts du raccord
     complet (toute copie hote <-> appareil de l'appel), publication, total et non ventile ;
  7. mutants de bench/g4_catalogue_mutants.json : copie mutee construite avec CUDA, critere du mutant ;
  8. juge et rapport.

Regle (ecrite le 7 octobre 2026, avant toute session, completee le meme jour vers 22 h UTC apres la lecture de
l'auditeur : diagnostics physiques et reprises comptes, etapes disjointes ; CONTRAT_CATALOGUE.md, paragraphes 6, 7
et 10) :
  << refuse >> si une preuve manque ou est incoherente : outil absent (nvcc, GPU), construction en echec, execution
     d'une sonde en echec (code inattendu, delai, sortie illisible), cas ou passe manquants, GPU non isole avant
     et apres les temps, porte device_open sans marqueur positif, contrat de mesure non respecte
     (moins de 5 processus ou de 10 passes, mutants sautes), etapes d'une passe
     chaude dont la somme depasse le total (durees non disjointes) ;
  << rejete >> si les preuves sont completes mais qu'une condition tombe : une porte rapide en echec ; une identite (empreinte, grand livre, comptes, diagnostics physiques, reprises et
     reecritures comptees) ou le determinisme en defaut sur un cas ; une empreinte de la voie CPU differente de F2 ; un
     mutant non tue ; le budget non tenu ;
  << adopte >> seulement si tout tient et que le budget est tenu : pour CHACUNE de ng00, ng01, ng02 a K5, la mediane
     des passes chaudes de la cohorte configuree (processes * (passes - 1), soit 45 valeurs pour le plan 5 x 10)
     ET le maximum des medianes par processus sont au
     plus 45 ms (borne haute du budget de 35 a 45 ms, CONTRAT_CATALOGUE.md, paragraphe 7).
  Les temps a K10, de la voie CPU et a froid sont publies sans decider.

Usage :
  python3 g4_catalogue_device.py --src DEPOT --data DONNEES --work TRAVAIL --out SORTIE [--jobs N] [--nvcc NVCC]
          [--cmake CMAKE] [--ctest CTEST] [--processes 5] [--passes 10] [--threads 48] [--skip-mutants]
  python3 g4_catalogue_device.py --selftest-judge      (auto-test du juge par injections, sans outil ni donnee)
  python3 g4_catalogue_device.py --check-mutants RACINE_V12   (motifs des mutants appareil presents une fois)
DEPOT contient morsehgp3D_v12/. Codes : 0 rapport ecrit (quel que soit le verdict) ou auto-test conforme ; 1 auto-test
en echec ; 2 refus avant toute mesure (arguments, dossiers, --work dans --out).
"""
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import g4_catalogue_judge as judge_module  # noqa: E402

BUDGET_NS = judge_module.BUDGET_NS
CONTRACT = judge_module.CONTRACT
DATA_FILES = {'ng00': 'lidar_ng00', 'ng01': 'lidar_ng01', 'ng02': 'lidar_ng02', 'u8000': 'uniform_u18_n8000',
              'u16000': 'uniform_u18_n16000', 'u32000': 'uniform_u18_n32000'}


def now():
    return time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime())


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, 'rb') as handle:
        for block in iter(lambda: handle.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


class Session:
    def __init__(self, out):
        self.out = Path(out)
        self.logs = self.out / 'logs'
        self.logs.mkdir(parents=True, exist_ok=True)
        self.report = {'schema': 'mhgp12_g4_catalogue_device_v1', 'started_utc': now(), 'contract': CONTRACT,
                       'budget_ns': BUDGET_NS, 'steps': {}}

    def run(self, name, argv, timeout, cwd=None, env=None):
        start = time.time()
        log = self.logs / (name + '.log')
        try:
            proc = subprocess.run([str(a) for a in argv], cwd=cwd, env=env, timeout=timeout, capture_output=True,
                                  text=True, errors='replace')
            code, out, err, timed_out = proc.returncode, proc.stdout, proc.stderr, False
        except subprocess.TimeoutExpired as error:
            code, timed_out = None, True
            out = error.stdout if isinstance(error.stdout, str) else ''
            err = error.stderr if isinstance(error.stderr, str) else ''
        except OSError as error:
            code, out, err, timed_out = None, '', str(error), False
        seconds = time.time() - start
        with open(log, 'w', encoding='utf-8') as handle:
            handle.write('$ ' + ' '.join(str(a) for a in argv) + '\n' + out + '\n--- stderr ---\n' + err)
        return {'name': name, 'code': code, 'timeout': timed_out, 'seconds': round(seconds, 3), 'stdout': out,
                'stderr_tail': err[-2000:]}

    def step(self, name, value):
        self.report['steps'][name] = value
        self.save()

    def save(self):
        path = self.out / 'report.json'
        tmp = path.with_suffix('.tmp')
        with open(tmp, 'w', encoding='utf-8') as handle:
            json.dump(self.report, handle, indent=1, sort_keys=True)
        os.replace(tmp, path)


def find_tool(explicit, name, extra):
    candidates = [explicit] if explicit else []
    candidates.append(shutil.which(name))
    if os.environ.get('CUDA_HOME'):
        candidates.append(os.path.join(os.environ['CUDA_HOME'], 'bin', name))
    candidates += extra + sorted(str(p) for p in Path('/usr/local').glob('cuda-12*/bin/' + name))
    for c in candidates:
        if c and os.path.isfile(c) and os.access(c, os.X_OK):
            return c
    return None


def environment(s, nvcc, cmake):
    env = {'date_utc': now(), 'cpu_count': os.cpu_count()}
    queries = {'nvcc': [nvcc, '--version'] if nvcc else None, 'cmake': [cmake, '--version'], 'cxx': ['c++', '--version'],
               'gpu': ['nvidia-smi', '--query-gpu=name,driver_version,persistence_mode,clocks.sm,clocks.max.sm,'
                       'temperature.gpu,power.draw,memory.used,memory.total', '--format=csv,noheader'],
               'gpu_apps': ['nvidia-smi', '--query-compute-apps=pid,process_name,used_memory', '--format=csv,noheader'],
               'uptime': ['uptime']}
    for key, cmd in queries.items():
        if cmd is None:
            env[key] = None
            continue
        r = s.run('env_' + key, cmd, 60)
        env[key] = r['stdout'].strip() if r['code'] == 0 else None
    return env


def gpu_quiet(s, label):
    r = s.run('gpu_apps_' + label, ['nvidia-smi', '--query-compute-apps=pid', '--format=csv,noheader'], 60)
    return r['code'] == 0 and r['stdout'].strip() == ''


def build(s, src, bdir, cmake, nvcc, jobs, name, targets=None):
    configure = [cmake, '-S', Path(src) / 'morsehgp3D_v12', '-B', bdir, '-DCMAKE_BUILD_TYPE=Release',
                 '-DMHGP12_COORD_BITS=21', '-DMHGP12_ENABLE_CUDA=ON', '-DCMAKE_CUDA_COMPILER=' + nvcc]
    c = s.run(name + '_configure', configure, 900)
    if c['code'] != 0:
        return {'ok': False, 'configure': c['code'], 'seconds': c['seconds']}
    cmd = [cmake, '--build', bdir, '-j', str(jobs)]
    for t in targets or []:
        cmd += ['--target', t]
    b = s.run(name + '_build', cmd, 2400)
    return {'ok': b['code'] == 0, 'configure': c['code'], 'build': b['code'], 'seconds': c['seconds'] + b['seconds']}


def unique_object(pairs):
    obj = {}
    for key, value in pairs:
        if key in obj:
            raise ValueError('cle JSON dupliquee')
        obj[key] = value
    return obj


def reject_constant(value):
    raise ValueError('constante JSON non standard : ' + value)


def parse_probe(text):
    out = {'passes': [], 'digests': [], 'open': None, 'exit': None, 'unreadable': 0}
    if not text.isascii():  # emetteur natif ASCII ; inclut le refus du remplacement UTF-8 U+FFFD
        out['unreadable'] = 1
        return out
    for line in text.splitlines():
        line = line.strip()
        if not line:
            continue
        try:
            obj = json.loads(line, object_pairs_hook=unique_object, parse_constant=reject_constant)
        except (ValueError, RecursionError):
            out['unreadable'] += 1
            continue
        if not isinstance(obj, dict) or not json.dumps(obj, ensure_ascii=False).isascii():
            out['unreadable'] += 1
            continue
        phase = obj.get('phase')
        if out['exit'] is not None:
            out['unreadable'] += 1
        if phase == 'catalogue':
            if out['digests'] and len(out['digests']) != len(out['passes']):
                out['unreadable'] += 1
            out['passes'].append(obj)
        elif phase == 'digest':
            if len(out['digests']) != len(out['passes']) - 1:
                out['unreadable'] += 1
            out['digests'].append(obj.get('catalogue_sha256'))
        elif phase in ('open', 'exit'):
            if out[phase] is not None or (phase == 'open' and out['passes']):
                out['unreadable'] += 1
            out[phase] = obj
        else:
            out['unreadable'] += 1
    return out


def probe_args(probe, data, case, k, leaf, threads, extra):
    stem = Path(data) / DATA_FILES[case]
    return [probe, str(stem) + '.u32le', str(stem) + '.ids.u32le', '--k=%d' % k, '--leaf=%d' % leaf,
            '--threads=%d' % threads] + extra


STAGE_KEYS = ('traversal_ns', 'count_ns', 'fill_ns', 'levels_ns', 'sort_ns', 'assemble_ns', 'table_ns')
DEVICE_STAGE_KEYS = ('transfer_ns', 'publish_ns')


def summary(parsed):
    """Comptes, grand livre, duree et durees par etape (diagnostics disjoints) de chaque passe ; diagnostics complets
    de chaque passe (copie finale conservee pour compatibilite)."""
    rows = []
    for p in parsed['passes']:
        diag = p.get('diagnostics') if isinstance(p.get('diagnostics'), dict) else {}
        dev = p.get('device') if isinstance(p.get('device'), dict) else {}
        stages = {k: diag.get(k) for k in STAGE_KEYS}
        stages.update({k: dev.get(k) for k in DEVICE_STAGE_KEYS})
        row = dict(p)  # metadonnees et diagnostics de CHAQUE passe, sans perte avant le juge
        row['stages'] = stages
        rows.append(row)
    last = parsed['passes'][-1] if parsed['passes'] else {}
    return {'passes': rows, 'digests': parsed['digests'], 'diagnostics': last.get('diagnostics'),
            'device': last.get('device'), 'open': parsed['open'], 'exit': parsed['exit'],
            'unreadable': parsed['unreadable']}


def identity(s, probe, data, threads):
    cases = []
    for case, k, leaf in CONTRACT['identity_cases']:
        cpu = s.run('id_%s_k%d_cpu' % (case, k), probe_args(probe, data, case, k, leaf, threads, ['--digest']), 1200)
        dev = s.run('id_%s_k%d_dev' % (case, k), probe_args(probe, data, case, k, leaf, threads,
                                                            ['--digest', '--device', '--passes=%d' %
                                                             CONTRACT['identity_passes']]), 1200)
        cases.append({'case': case, 'k': k, 'leaf': leaf, 'cpu_code': cpu['code'], 'device_code': dev['code'],
                      'cpu': summary(parse_probe(cpu['stdout'])), 'device': summary(parse_probe(dev['stdout']))})
        s.step('identity', cases)
    return cases


def cpu_timing(s, probe, data, threads, passes):
    cases = []
    for case, k, leaf in CONTRACT['f2_cases']:
        r = s.run('cpu_%s_k%d_f%d' % (case, k, leaf), probe_args(probe, data, case, k, leaf, threads,
                                                                  ['--digest', '--passes=%d' % passes]), 1800)
        cases.append({'case': case, 'k': k, 'leaf': leaf, 'code': r['code'], 'run': summary(parse_probe(r['stdout']))})
        s.step('cpu_timing', cases)
    return cases


def device_timing(s, probe, data, k, processes, passes, threads):
    frames = list(CONTRACT['frames'])
    runs = []
    for p in range(processes):
        order = frames[p % len(frames):] + frames[:p % len(frames)]  # ordre tournant
        for frame in order:
            r = s.run('dev_k%d_%s_p%d' % (k, frame, p), probe_args(probe, data, frame, k, 24, threads,
                                                                   ['--device', '--passes=%d' % passes]), 1200)
            runs.append({'frame': frame, 'process': p, 'code': r['code'], 'run': summary(parse_probe(r['stdout']))})
            s.step('device_timing_k%d' % k, runs)
    return runs


def apply_mutant(root, mutant):
    """Correctif d'un mutant sur une copie (texte cherche present une seule fois, remplacements dans l'ordre)."""
    path = Path(root) / mutant['fichier']
    text = path.read_text(encoding='utf-8')
    for item in [mutant] + list(mutant.get('aussi', [])):
        if text.count(item['cherche']) != 1:
            return 'motif absent ou multiple : ' + item['cherche'][:60]
        text = text.replace(item['cherche'], item['remplace'])
    path.write_text(text, encoding='utf-8')
    return None


def mutant_run(s, args, nvcc, cmake, mutant, reference_digest):
    work = Path(args.work) / 'mutants' / mutant['id']
    shutil.rmtree(work, ignore_errors=True)
    shutil.copytree(Path(args.src) / 'morsehgp3D_v12', work / 'morsehgp3D_v12',
                    ignore=shutil.ignore_patterns('__pycache__', '.git'))
    problem = apply_mutant(work / 'morsehgp3D_v12', mutant)
    out = {'id': mutant['id'], 'critere': mutant['critere'], 'applied': problem is None, 'problem': problem}
    if problem is not None:
        return out
    built = build(s, work, work / 'b', cmake, nvcc, args.jobs, 'mutant_' + mutant['id'],
                  ['mhgp12_catalogue_probe', 'mhgp12_catalogue_device_unit'])
    out['build'] = built
    if not built['ok']:
        return out
    probe = work / 'b' / 'mhgp12_catalogue_probe'
    if mutant['critere'] == 'identite':
        unit = s.run('mutant_%s_unit' % mutant['id'], [work / 'b' / 'mhgp12_catalogue_device_unit'], 1200)
        dev = s.run('mutant_%s_ng00' % mutant['id'], probe_args(probe, args.data, 'ng00', 5, 24, args.threads,
                                                                 ['--digest', '--device']), 1200)
        parsed = parse_probe(dev['stdout'])
        out.update({'unit_code': unit['code'], 'unit_timeout': unit['timeout'], 'ng00_code': dev['code'],
                    'ng00_digest': parsed['digests'][0] if parsed['digests'] else None, 'ng00': summary(parsed),
                    'reference_digest': reference_digest})
    else:
        dev = s.run('mutant_%s_temps' % mutant['id'], probe_args(probe, args.data, 'ng00', 5, 24, args.threads,
                                                                  ['--digest', '--device', '--passes=4']), 2400)
        parsed = parse_probe(dev['stdout'])
        out.update({'code': dev['code'], 'timeout': dev['timeout'], 'run': summary(parsed),
                    'reference_digest': reference_digest})
    return out


def check_mutants(root):
    """Chaque motif du manifeste des mutants appareil est present une seule fois dans l'arbre (porte locale)."""
    manifest = json.loads((HERE / 'g4_catalogue_mutants.json').read_text(encoding='utf-8'))
    ids = [m.get('id') for m in manifest.get('mutants', [])]
    if ids != judge_module.CONTRACT['mutants']:
        print('mutants_appareil_ecart : identifiants %s, contrat %s' % (ids, judge_module.CONTRACT['mutants']))
        return 1
    for m in manifest['mutants']:
        text = (Path(root) / m['fichier']).read_text(encoding='utf-8')
        for item in [m] + list(m.get('aussi', [])):
            if text.count(item['cherche']) != 1:
                print('mutants_appareil_ecart : %s : motif absent ou multiple : %s' % (m['id'], item['cherche'][:60]))
                return 1
            text = text.replace(item['cherche'], item['remplace'])
    print('mutants_appareil_ok mutants=%d' % len(ids))
    return 0


def main_run(args):
    for folder in (args.src, args.data):
        if not Path(folder).is_dir():
            print('refus : dossier absent : %s' % folder)
            return 2
    out, work = Path(args.out).resolve(), Path(args.work).resolve()
    if out == work or out in work.parents or work in out.parents:
        print('refus : --work et --out doivent etre disjoints')
        return 2
    for stem in DATA_FILES.values():
        for suffix in ('.u32le', '.ids.u32le'):
            if not (Path(args.data) / (stem + suffix)).is_file():
                print('refus : donnee absente : %s%s' % (stem, suffix))
                return 2
    work.mkdir(parents=True, exist_ok=True)
    s = Session(out)
    nvcc = find_tool(args.nvcc, 'nvcc', ['/usr/local/cuda/bin/nvcc'])
    cmake = args.cmake or shutil.which('cmake') or 'cmake'
    ctest = args.ctest or shutil.which('ctest') or 'ctest'
    s.report['options'] = {'processes': args.processes, 'passes': args.passes, 'threads': args.threads,
                           'jobs': args.jobs, 'skip_mutants': args.skip_mutants}
    s.step('environment', environment(s, nvcc, cmake))
    if nvcc is None:
        s.report['verdict'] = judge_module.judge(s.report)
        s.save()
        return 0
    bdir = work / 'b21'
    built = build(s, args.src, bdir, cmake, nvcc, args.jobs, 'produit')
    if built['ok']:
        built['binaries'] = {name: sha256_file(bdir / name) for name in
                             ('mhgp12_catalogue_probe', 'mhgp12_catalogue_device_unit') if (bdir / name).is_file()}
    s.step('build', built)
    if built['ok']:
        gates = s.run('ctest', [ctest, '--no-tests=error', '-LE', 'long', '-j', str(args.jobs), '--output-on-failure'],
                      2400, cwd=bdir)
        unit = s.run('device_open', [bdir / 'mhgp12_catalogue_device_unit', 'device_open'], 1200)
        s.step('gates', {'code': gates['code'], 'timeout': gates['timeout'], 'tail': gates['stdout'][-1500:],
                         'device_open_code': unit['code'], 'device_open_stdout': unit['stdout'][-1000:]})
        probe = bdir / 'mhgp12_catalogue_probe'
        identity(s, probe, args.data, args.threads)
        cpu_timing(s, probe, args.data, args.threads, CONTRACT['cpu_passes'])
        s.step('gpu_quiet_before', gpu_quiet(s, 'avant'))
        device_timing(s, probe, args.data, 5, args.processes, args.passes, args.threads)
        device_timing(s, probe, args.data, 10, CONTRACT['k10_processes'], CONTRACT['k10_passes'], args.threads)
        s.step('gpu_quiet_after', gpu_quiet(s, 'apres'))
        if not args.skip_mutants:
            manifest = json.loads((HERE / 'g4_catalogue_mutants.json').read_text(encoding='utf-8'))
            reference = judge_module.reference_digest(s.report)
            results = []
            for mutant in manifest['mutants']:
                results.append(mutant_run(s, args, nvcc, cmake, mutant, reference))
                s.step('mutants', results)
    s.report['finished_utc'] = now()
    s.report['verdict'] = judge_module.judge(s.report)
    s.save()
    print('verdict : %s' % s.report['verdict']['verdict'])
    return 0


def main():
    parser = argparse.ArgumentParser(description='T1-b sur G4 : voie appareil du catalogue, portes, temps, juge.')
    parser.add_argument('--src')
    parser.add_argument('--data')
    parser.add_argument('--work')
    parser.add_argument('--out')
    parser.add_argument('--jobs', type=int, default=44)
    parser.add_argument('--nvcc')
    parser.add_argument('--cmake')
    parser.add_argument('--ctest')
    parser.add_argument('--processes', type=int, default=CONTRACT['processes'])
    parser.add_argument('--passes', type=int, default=CONTRACT['passes'])
    parser.add_argument('--threads', type=int, default=CONTRACT['threads'])
    parser.add_argument('--skip-mutants', action='store_true')
    parser.add_argument('--selftest-judge', action='store_true')
    parser.add_argument('--check-mutants', metavar='RACINE_V12')
    args = parser.parse_args()
    if args.selftest_judge:
        return judge_module.selftest()
    if args.check_mutants:
        return check_mutants(args.check_mutants)
    if not (args.src and args.data and args.work and args.out):
        print('refus : --src, --data, --work et --out sont exiges')
        return 2
    return main_run(args)


if __name__ == '__main__':
    sys.exit(main())
