#!/usr/bin/env python3
"""Banc G4 de la voie GPU du catalogue : CPU contre GPU, a froid et a chaud, dumps identiques exiges.

    python3 bench/gpu_ab.py (--bench BUILD/mhgp11_full_bench | --src SRC --work DIR) --data DIR --out DIR
        [--modes cpu=16379,gpu=81915,gpu_partage=344059:400] [--reps 5] [--workers 48] [--warm-passes 5] [--kmax 5]
        [--leaf 16]
        [--variants new,base,...]  (archives <nom>_src.tar.gz du dossier de donnees, voir --variants)

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
import tarfile
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


# Prereglages d'environnement d'un mode (suffixe @nom) : "tas" garde les grands blocs dans le tas de glibc (seuil mmap
# a 1 Gio, pas de rognage), donc ni munmap a la restitution ni nouvelles fautes de page a la passe suivante.
ENV_PRESETS = {'tas': {'GLIBC_TUNABLES': 'glibc.malloc.mmap_threshold=1073741824:glibc.malloc.trim_threshold=4294967296'}}


def run(cmd, timeout, env=None):
    t0 = time.monotonic()
    done = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout, env=env)
    return done.returncode, done.stdout, done.stderr, round(time.monotonic() - t0, 3)


def median(values):
    v = sorted(x for x in values if x is not None)
    return v[len(v) // 2] if v else None


def host_memory():
    """Politique des pages de 2 Mio (THP) et pages enormes anonymes de l'hote, pour lire les mesures (7 octobre)."""
    out = {}
    try:
        out['libc'] = os.confstr('CS_GNU_LIBC_VERSION')
    except (ValueError, OSError):
        out['libc'] = None
    for key, path in (('thp_enabled', '/sys/kernel/mm/transparent_hugepage/enabled'),
                      ('thp_defrag', '/sys/kernel/mm/transparent_hugepage/defrag'),
                      ('kernel', '/proc/sys/kernel/osrelease')):
        try:
            out[key] = Path(path).read_text().strip()
        except OSError:
            out[key] = None
    try:
        for line in Path('/proc/meminfo').read_text().splitlines():
            if line.startswith(('AnonHugePages:', 'MemTotal:', 'MemAvailable:')):
                out[line.split(':')[0]] = line.split(':')[1].strip()
    except OSError:
        pass
    return out


def take_summary(got):
    full, domain = got.get('full', {}), got.get('domain', {})
    batch = domain.get('leaf_batch') or {}
    ms = lambda v: None if v is None else round(v / 1e6, 3)  # noqa: E731
    return dict(status=full.get('status'), exit=got.get('exit', {}).get('status'), wall_ms=ms(full.get('wall_ns')),
                domain_ms=ms(full.get('domain_ns')), forest_ms=ms(full.get('forest_ns')),
                single_pass_ms=ms(domain.get('single_pass_ns')), prefix_ms=ms(domain.get('prefix_ns')),
                sort_ms=ms(domain.get('sort_ns')), level_scan_ms=ms(domain.get('level_scan_ns')),
                assembly_ms=ms(domain.get('assembly_ns')), compact_ms=ms(domain.get('compact_ns')),
                allocation_ms=ms(domain.get('allocation_ns')), release_ms=ms(domain.get('release_ns')),
                lookup_ms=ms(domain.get('lookup_ns')), cpu_seconds=full.get('cpu_seconds'),
                pipeline_placement_cores=(full.get('pipeline_tasks') or {}).get('placement_cores'),
                pipeline=pipeline_summary(full), batch=dict(batch))


def pipeline_summary(full):
    """Chronologie descriptive de l'etage des forets (ms depuis son debut) : phases, voies de resolution, publieurs et
    suiveurs verticaux par ordre. Absente (None) si la sonde ne l'emet pas ; aucune decision ne la lit."""
    tasks, phases = full.get('pipeline_tasks'), full.get('phases')
    if not isinstance(tasks, dict) or not isinstance(phases, dict):
        return None
    ms = lambda v: None if not isinstance(v, int) else round(v / 1e6, 3)  # noqa: E731
    keys = ('publish_start_ns', 'publish_end_ns', 'publish_cpu_ns', 'publish_wait_ns', 'vertical_start_ns',
            'vertical_end_ns', 'vertical_cpu_ns', 'vertical_wait_ns')
    return dict(phases={k: ms(v) for k, v in phases.items()},
                lanes={k: ms(tasks.get(k)) for k in ('lanes_last_start_ns', 'lanes_first_finish_ns',
                                                     'lanes_last_finish_ns', 'lanes_cpu_ns')},
                orders=[dict(k=o.get('k'), **{key[:-3] + '_ms': ms(o.get(key)) for key in keys})
                        for o in tasks.get('orders', []) if isinstance(o, dict)])


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


def build(src, work, out, log, suffix=''):
    """Construit (ou reprend) le banc CUDA ; journaux dans out/build_*.log ; rend le chemin ou None."""
    bdir = work / ('b_cuda' + suffix)
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
        (out / ('build%s_%s.log' % (suffix, name))).write_text(stdout[-200000:] + '\n--- stderr ---\n' + stderr)
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
    # Variantes de source (6 octobre 2026, workflow GPU) : archives <nom>_src.tar.gz du dossier de donnees (arbre
    # morsehgp3D_v11/ a la racine), chacune construite avec CUDA ; "new" designe --src. Chaque mode est joue par chaque
    # variante (nom variante:mode), dans l'ordre equilibre du carre de Williams ; tous les dumps d'une trame doivent
    # egaler la premiere prise.
    ap.add_argument('--variants', default='new')
    args = ap.parse_args()
    raw_modes = [tuple(m.split('=')) for m in args.modes.split(',')]
    variants = [v for v in args.variants.split(',') if v]
    args.out.mkdir(parents=True, exist_ok=True)
    build_log = []
    benches = {}
    if (any(len(m) != 2 for m in raw_modes) or not variants or len(set(variants)) != len(variants) or
            any(m[1].partition('@')[2] not in ('',) + tuple(ENV_PRESETS) for m in raw_modes if len(m) == 2)):
        print('refus : --modes ou --variants invalide', file=sys.stderr)
        return 1
    for variant in variants:
        if variant == 'new':
            if args.bench is None and args.src is not None and args.work is not None:
                args.bench = build(args.src, args.work, args.out, build_log)
            benches[variant] = args.bench
            continue
        archive = args.data / (variant + '_src.tar.gz')
        if args.work is None or not archive.is_file():
            print('refus : archive de variante absente : ' + str(archive), file=sys.stderr)
            return 1
        archive_hash = sha256(archive)
        root = args.work / ('src_' + variant + '_' + archive_hash)
        if not (root / 'morsehgp3D_v11').is_dir():
            root.mkdir(parents=True, exist_ok=True)
            with tarfile.open(archive) as tar:
                tar.extractall(root)
        build_log.append(dict(step='variant', name=variant, archive_sha256=archive_hash))
        benches[variant] = build(root, args.work, args.out, build_log, suffix='_' + variant + '_' + archive_hash)
    if any(b is None or not Path(b).is_file() for b in benches.values()):
        print('refus : banc absent', file=sys.stderr)
        return 1
    # Une seule variante "new" : noms de modes inchanges (compatibilite des rapports precedents).
    modes = [(n if variants == ['new'] else v + ':' + n, m, benches[v]) for v in variants for n, m in raw_modes]
    # Non-vacuite des deux regimes (audit du 4 octobre) : au moins une prise a froid, au moins deux passes a chaud.
    if args.reps < 1 or args.warm_passes < 2:
        print('refus : --reps >= 1 et --warm-passes >= 2 exiges', file=sys.stderr)
        return 1
    report = dict(schema='ehgp.v11.gpu_ab.v1', build=build_log, modes={n: m for n, m, _ in modes}, reps=args.reps,
                  workers=args.workers, warm_passes=args.warm_passes, kmax=args.kmax, leaf=args.leaf,
                  bench_sha256={v: sha256(b) for v, b in benches.items()}, variants=variants,
                  host=host_memory(), cold=[], warm=[], identity={}, ledger={}, refusals=[])
    tail = [str(args.kmax), str(args.leaf), '256', '0', '4294967295', '8589934592']
    dump = args.out / 'dump.tmp'

    def save():
        (args.out / 'gpu_ab_report.json').write_text(json.dumps(report, indent=1) + '\n')

    def one(frame, name, mode, bench, workers, passes):
        cmd = [str(bench), str(args.data / (frame + '.u32le')), str(args.data / (frame + '.ids.u32le')), str(dump)]
        # Mode MASQUE ou MASQUE:PART (executeur partage du lot, part de l'hote pour mille : argument final de la sonde).
        # Suffixe @nom : prereglage d'environnement du banc (ENV_PRESETS), le reste du mode inchange.
        bare, _, preset = mode.partition('@')
        mask, _, split = bare.partition(':')
        cmd += tail + [workers, mask] + ([str(passes)] if passes > 1 or split else []) + ([split] if split else [])
        if dump.exists():
            dump.unlink()
        env = None if not preset else dict(os.environ, **ENV_PRESETS[preset])
        code, out, err, seconds = run(cmd, 900, env)
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
        if passes > 1:  # passes exactement 1..P, toutes reussies ; l'identite porte sur le dernier dump seulement
            ok = ok and [p.get('pass') for p in passes_seen] == list(range(1, passes + 1)) and all(
                p.get('status') == 'ok' for p in passes_seen)
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
                for name, mode, bench in order:
                    row = one(frame, name, mode, bench, workers, 1)
                    row['rep'] = rep
                    report['cold'].append(row)
                    save()
        for frame in FRAMES:
            for name, mode, bench in modes:
                report['warm'].append(one(frame, name, mode, bench, workers, args.warm_passes))
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
            first_pass_device_init_ms=(row['passes'][0]['batch_device_init_ns'] / 1e6) if row['passes'] else None,
            release_ms=median([p['release_ns'] / 1e6 for p in later if 'release_ns' in p]),
            lookup_ms=median([p['lookup_ns'] / 1e6 for p in later if 'lookup_ns' in p]))
    report['warm_medians_ms'] = warm
    report['scope'] = ('a froid : dump et registre de chaque prise ; a chaud : passes 1..P toutes reussies, dump et '
                       'registre de la derniere passe P seulement (les passes 1..P-1 ne serialisent rien)')
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
