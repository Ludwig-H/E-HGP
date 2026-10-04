#!/usr/bin/env python3
"""Banc G4 de la voie GPU du catalogue : CPU contre GPU, a froid et a chaud, dumps identiques exiges.

    python3 bench/gpu_ab.py (--bench BUILD/mhgp11_full_bench | --src SRC --work DIR) --data DIR --out DIR
        [--modes cpu=16379,gpu=81915] [--reps 5] [--workers 48] [--warm-passes 5] [--kmax 5] [--leaf 16]

Un seul binaire (construit avec MHGP11_ENABLE_CUDA) joue chaque mode : 16379 est la voie CPU de reference, 81915
ajoute le bit 65536 (feuilles en lot sur le GPU), 49147 le bit 32768 (meme lot sur le Pool de l'hote). Avec --src, le binaire est construit (ou repris s'il existe) dans
--work/b_cuda : Release, MHGP11_COORD_BITS=21, MHGP11_ENABLE_CUDA=ON, nvcc trouve comme les sessions G4 precedentes
(PATH, CUDA_HOME, /usr/local/cuda, /usr/local/cuda-12.9), cible mhgp11_full_bench. A froid : un processus neuf par prise, modes ordonnes par un carre de
Williams (bench/ab_g4.py), sur lidar_ng00/01/02 ; chaque dump est hache puis efface et doit egaler celui de la premiere prise CPU
de sa trame, et le registre du catalogue (catalogue_work) doit egaler le sien. A chaud : un processus par (trame, mode) qui enchaine --warm-passes passes FULL (Pool, memoire et contexte
GPU vivants) ; on garde chaque ligne "pass" et le dump de la derniere passe, lui aussi compare. Le rapport publie les
medianes par trame, mode et regime (mur, domaine, passe unique, executeur du lot, forets) et le detail du lot GPU
(contexte, envoi, comptage, ecriture, retour). Bibliotheque standard seule. Codes : 0 conforme ; 1 refus.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ab_g4 import williams  # noqa: E402  (meme carre equilibre que le banc A/B)

FRAMES = ('lidar_ng00', 'lidar_ng01', 'lidar_ng02')


def sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 22), b''):
            h.update(chunk)
    return h.hexdigest()


def phases(text):
    out, passes = {}, []
    for line in text.splitlines():
        try:
            j = json.loads(line)
        except ValueError:
            continue
        if isinstance(j, dict) and 'phase' in j:
            if j['phase'] == 'pass':
                passes.append(j)
            else:
                out[j['phase']] = j
    return out, passes


def run(cmd, timeout):
    t0 = time.monotonic()
    done = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    return done.returncode, done.stdout, done.stderr, round(time.monotonic() - t0, 3)


def median(values):
    v = sorted(x for x in values if x is not None)
    return v[len(v) // 2] if v else None


def take_summary(got):
    full, domain = got.get('full', {}), got.get('domain', {})
    batch = domain.get('leaf_batch') or {}
    ms = lambda v: None if v is None else round(v / 1e6, 3)  # noqa: E731
    return dict(status=full.get('status'), exit=got.get('exit', {}).get('status'), wall_ms=ms(full.get('wall_ns')),
                domain_ms=ms(full.get('domain_ns')), forest_ms=ms(full.get('forest_ns')),
                single_pass_ms=ms(domain.get('single_pass_ns')), prefix_ms=ms(domain.get('prefix_ns')),
                sort_ms=ms(domain.get('sort_ns')), cpu_seconds=full.get('cpu_seconds'),
                batch=dict(batch))


def find_nvcc():
    """nvcc comme les sessions G4 precedentes : PATH, CUDA_HOME, /usr/local/cuda, /usr/local/cuda-12.9, autres 12.x."""
    candidates = [shutil.which('nvcc')]
    if os.environ.get('CUDA_HOME'):
        candidates.append(os.path.join(os.environ['CUDA_HOME'], 'bin', 'nvcc'))
    candidates += ['/usr/local/cuda/bin/nvcc', '/usr/local/cuda-12.9/bin/nvcc']
    candidates += sorted(str(p) for p in Path('/usr/local').glob('cuda-12*/bin/nvcc'))
    for candidate in candidates:
        if candidate and os.path.isfile(candidate) and os.access(candidate, os.X_OK):
            return candidate
    return None


def build(src, work, out, log):
    """Construit (ou reprend) le banc CUDA ; journaux dans out/build_*.log ; rend le chemin ou None."""
    bdir = work / 'b_cuda'
    exe = bdir / 'mhgp11_full_bench'
    if exe.is_file():
        log.append(dict(step='reuse', sha256=sha256(exe)))
        return exe
    work.mkdir(parents=True, exist_ok=True)
    nvcc = find_nvcc()
    log.append(dict(step='nvcc', path=nvcc))
    if nvcc is None:
        return None
    for name, cmd in (('versions_nvcc', [nvcc, '--version']), ('versions_cmake', ['cmake', '--version'])):
        code, stdout, stderr, seconds = run(cmd, 60)
        log.append(dict(step=name, code=code, text=(stdout + stderr)[-600:]))
    steps = [('configure', ['cmake', '-S', str(src / 'morsehgp3D_v11'), '-B', str(bdir), '-DCMAKE_BUILD_TYPE=Release',
                            '-DMHGP11_COORD_BITS=21', '-DMHGP11_ENABLE_CUDA=ON', '-DCMAKE_CUDA_COMPILER=' + nvcc], 600),
             ('build', ['cmake', '--build', str(bdir), '-j', str(os.cpu_count() or 8), '--target', 'mhgp11_full_bench'],
              1800)]
    for name, cmd, timeout in steps:
        code, stdout, stderr, seconds = run(cmd, timeout)
        (out / ('build_%s.log' % name)).write_text(stdout[-200000:] + '\n--- stderr ---\n' + stderr)
        log.append(dict(step=name, code=code, seconds=seconds))
        if code != 0:
            return None
    log.append(dict(step='built', sha256=sha256(exe) if exe.is_file() else None))
    return exe if exe.is_file() else None


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--bench', type=Path)
    ap.add_argument('--src', type=Path)
    ap.add_argument('--work', type=Path)
    ap.add_argument('--data', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--modes', default='cpu=16379,gpu=81915')
    ap.add_argument('--reps', type=int, default=5)
    ap.add_argument('--workers', default='48')
    ap.add_argument('--warm-passes', type=int, default=5)
    ap.add_argument('--kmax', type=int, default=5)
    ap.add_argument('--leaf', type=int, default=16)
    args = ap.parse_args()
    modes = [tuple(m.split('=')) for m in args.modes.split(',')]
    args.out.mkdir(parents=True, exist_ok=True)
    build_log = []
    if args.bench is None and args.src is not None and args.work is not None:
        args.bench = build(args.src, args.work, args.out, build_log)
    if args.bench is None or not args.bench.is_file() or any(len(m) != 2 for m in modes):
        print('refus : banc absent ou --modes invalide', file=sys.stderr)
        return 1
    report = dict(schema='ehgp.v11.gpu_ab.v1', build=build_log, modes=dict(modes), reps=args.reps, workers=args.workers,
                  warm_passes=args.warm_passes, kmax=args.kmax, leaf=args.leaf, bench_sha256=sha256(args.bench),
                  cold=[], warm=[], identity={}, ledger={}, refusals=[])
    tail = [str(args.kmax), str(args.leaf), '256', '0', '4294967295', '8589934592']
    dump = args.out / 'dump.tmp'

    def save():
        (args.out / 'gpu_ab_report.json').write_text(json.dumps(report, indent=1) + '\n')

    def one(frame, name, mode, workers, passes):
        cmd = [str(args.bench), str(args.data / (frame + '.u32le')), str(args.data / (frame + '.ids.u32le')), str(dump)]
        cmd += tail + [workers, mode] + ([str(passes)] if passes > 1 else [])
        if dump.exists():
            dump.unlink()
        code, out, err, seconds = run(cmd, 900)
        got, passes_seen = phases(out)
        work = got.get('domain', {}).get('catalogue_work')
        digest = sha256(dump) if dump.exists() else None
        if dump.exists():
            dump.unlink()
        row = dict(frame=frame, mode=name, workers=workers, code=code, seconds=seconds, dump_sha256=digest,
                   summary=take_summary(got), passes=[{k: v for k, v in p.items() if k != 'phase'} for p in passes_seen],
                   stderr=err[-2000:] if code != 0 else '')
        reference = report['identity'].get(frame)
        if digest is not None and reference is None and name == modes[0][0]:
            report['identity'][frame] = digest
            report['ledger'][frame] = work
            reference = digest
        ok = (code == 0 and row['summary']['status'] == 'ok' and digest is not None and digest == reference and
              work is not None and work == report['ledger'].get(frame))
        if not ok:
            report['refusals'].append('%s %s w%s passes%d code %s dump %s' % (frame, name, workers, passes, code,
                                                                             (digest or '')[:12]))
        return row

    orders = williams(len(modes))
    report['orders'] = [[modes[i][0] for i in o] for o in orders]
    for workers in args.workers.split(','):
        for rep in range(args.reps):
            for frame in FRAMES:
                order = [modes[i] for i in orders[rep % len(orders)]]
                for name, mode in order:
                    row = one(frame, name, mode, workers, 1)
                    row['rep'] = rep
                    report['cold'].append(row)
                    save()
        for frame in FRAMES:
            for name, mode in modes:
                report['warm'].append(one(frame, name, mode, workers, args.warm_passes))
                save()
    # Medianes : a froid par prise ; a chaud sur les passes 2..P (la premiere ouvre contexte et memoire).
    table = {}
    for row in report['cold']:
        if row['code'] == 0:
            table.setdefault('cold|%s|w%s|%s' % (row['frame'], row['workers'], row['mode']), []).append(row['summary'])
    report['cold_medians_ms'] = {k: {f: median([r[f] for r in rows]) for f in ('wall_ms', 'domain_ms',
                                                                                'single_pass_ms', 'forest_ms')}
                                 for k, rows in sorted(table.items())}
    warm = {}
    for row in report['warm']:
        later = [p for p in row['passes'] if p.get('pass', 0) >= 2 and p.get('status') == 'ok']
        warm['warm|%s|w%s|%s' % (row['frame'], row['workers'], row['mode'])] = dict(
            wall_ms=median([p['wall_ns'] / 1e6 for p in later]), domain_ms=median([p['domain_ns'] / 1e6 for p in later]),
            forest_ms=median([p['forest_ns'] / 1e6 for p in later]),
            batch_executor_ms=median([p['batch_executor_ns'] / 1e6 for p in later]),
            best_wall_ms=min([p['wall_ns'] / 1e6 for p in later], default=None),
            first_pass_wall_ms=(row['passes'][0]['wall_ns'] / 1e6) if row['passes'] else None,
            first_pass_device_init_ms=(row['passes'][0]['batch_device_init_ns'] / 1e6) if row['passes'] else None)
    report['warm_medians_ms'] = warm
    report['verdict'] = 'conforme' if not report['refusals'] else 'refus'
    save()
    for key, value in report['cold_medians_ms'].items():
        print(key, value)
    for key, value in warm.items():
        print(key, value)
    print('gpu_ab_verdict %s refus %d' % (report['verdict'], len(report['refusals'])))
    return 0 if not report['refusals'] else 1


if __name__ == '__main__':
    sys.exit(main())
