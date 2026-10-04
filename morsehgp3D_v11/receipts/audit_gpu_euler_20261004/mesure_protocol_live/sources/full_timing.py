#!/usr/bin/env python3
"""Chronometrage FULL sur des scenes LiDAR preparees (cible du contrat : temps en fonction du nombre de sites).

    python3 bench/full_timing.py --bench BUILD/mhgp11_full_bench --scenes DIR --work DIR --out FICHIER.json
        [--extra NOM=XYZ:IDS ...] [--reps 1] [--workers 48] [--mode 16379] [--kmax 5] [--leaf 16]
        [--taskset 0-23]

DIR contient points_manifest.json et <nom>_sites.u32le (triplets u32, ecrits par bench/points_lidar_prepare.py) ;
les PointId sont 0..n-1, ecrits une fois dans WORK. --extra ajoute des trames deja codees (par exemple les trois
trames du contrat). Chaque prise est un processus neuf, dump vers /dev/null, memes arguments que bench/ab_g4.py
(K, feuille 16 par defaut, capacite 256, budget 8 Gio) ; --taskset epingle le processus (par exemple un fil par coeur
physique a W24). Publie par scene : sites, statut, mur, domaine, passe unique, forets,
boules ; puis, par tranche de sites (30-40k, 40-50k, 50-60k, 60k+), mediane et maximum du mur median par scene.
Chaque entree porte sa provenance (entree du manifeste ou fichier --extra, empreinte sha256 des sites) : une tranche
de sites decrit des scenes, elle ne qualifie pas une trame entiere. Le rapport est ecrit apres chaque prise (statut
running, puis complete) ; une prise hors delai ou en echec est publiee avec son statut et la fin de ses sorties, et
n'arrete pas le banc. Aucune coordonnee n'est ecrite dans la sortie. Bibliotheque standard seule. Codes : 0 toutes
les prises ok ; 1 une prise en echec (publiee) ; 2 entree refusee.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import time

BINS = ((30000, 40000), (40000, 50000), (50000, 60000), (60000, 10 ** 9))


def phases(text):
    out = {}
    for line in text.splitlines():
        try:
            j = json.loads(line)
        except ValueError:
            continue
        if isinstance(j, dict) and 'phase' in j:
            out[j['phase']] = j
    return out


def run(bench, xyz, ids, args):
    cmd = [str(bench), str(xyz), str(ids), '/dev/null', str(args.kmax), str(args.leaf), '256', '0', '4294967295',
           '8589934592', str(args.workers), str(args.mode)]
    if args.taskset:
        cmd = ['taskset', '-c', args.taskset] + cmd
    t0 = time.monotonic()
    try:
        done = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    except subprocess.TimeoutExpired as expired:
        tail = lambda v: (v.decode(errors='replace') if isinstance(v, bytes) else (v or ''))[-2000:]  # noqa: E731
        return dict(ok=False, code=None, status='timeout', seconds=round(time.monotonic() - t0, 3),
                    stdout_tail=tail(expired.stdout), stderr_tail=tail(expired.stderr))
    got = phases(done.stdout)
    full, domain = got.get('full', {}), got.get('domain', {})
    ok = done.returncode == 0 and full.get('status') == 'ok' and got.get('exit', {}).get('status') == 'ok'
    ms = lambda v: None if v is None else round(v / 1e6, 2)  # noqa: E731
    failed = {} if ok else dict(stdout_tail=done.stdout[-2000:], stderr_tail=done.stderr[-2000:])
    return dict(ok=ok, code=done.returncode, status='ok' if ok else 'failed', **failed,
                seconds=round(time.monotonic() - t0, 3), wall_ms=ms(full.get('wall_ns')),
                domain_ms=ms(full.get('domain_ns')), forest_ms=ms(full.get('forest_ns')),
                single_pass_ms=ms(domain.get('single_pass_ns')), balls=domain.get('catalogue_balls'),
                cpu_seconds=full.get('cpu_seconds'))


def sha256(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for chunk in iter(lambda: f.read(1 << 22), b''):
            h.update(chunk)
    return h.hexdigest()


def median(values):
    v = sorted(values)
    return v[len(v) // 2] if v else None


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--bench', type=Path, required=True)
    ap.add_argument('--scenes', type=Path)
    ap.add_argument('--work', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--extra', action='append', default=[])
    ap.add_argument('--reps', type=int, default=1)
    ap.add_argument('--workers', type=int, default=48)
    ap.add_argument('--mode', type=int, default=16379)
    ap.add_argument('--kmax', type=int, default=5)
    ap.add_argument('--leaf', type=int, default=16)
    ap.add_argument('--taskset', default='')
    args = ap.parse_args()
    if not args.bench.is_file() or args.reps < 1:
        print('refus : banc absent ou reps < 1', file=sys.stderr)
        return 2
    args.work.mkdir(parents=True, exist_ok=True)
    cases = []
    if args.scenes is not None:
        manifest = json.loads((args.scenes / 'points_manifest.json').read_text())
        for scene in manifest['scenes']:
            xyz = args.scenes / (scene['name'] + '_sites.u32le')
            n = xyz.stat().st_size // 12
            ids = args.work / (scene['name'] + '.ids.u32le')
            ids.write_bytes(struct.pack('<%dI' % n, *range(n)))
            provenance = {k: v for k, v in scene.items() if isinstance(v, (str, int, float, bool)) or v is None}
            provenance.update(source='manifeste ' + str(args.scenes / 'points_manifest.json'), sites_sha256=sha256(xyz))
            cases.append(dict(name=scene['name'], sites=n, sequence=scene.get('sequence'), frame=scene.get('frame'),
                              xyz=xyz, ids=ids, provenance=provenance))
    for spec in args.extra:
        name, _, paths = spec.partition('=')
        xyz, _, ids = paths.partition(':')
        xyz, ids = Path(xyz), Path(ids)
        if not name or not xyz.is_file() or not ids.is_file():
            print('refus : --extra ' + spec, file=sys.stderr)
            return 2
        cases.append(dict(name=name, sites=xyz.stat().st_size // 12, sequence=None, frame=None, xyz=xyz, ids=ids,
                          provenance=dict(source='--extra ' + xyz.name, sites_sha256=sha256(xyz),
                                          ids_sha256=sha256(ids))))
    rows, failures = [], 0
    report = dict(schema='ehgp.v11.full_timing.v2', status='running', kmax=args.kmax, leaf=args.leaf,
                  workers=args.workers, mode=args.mode, taskset=args.taskset or None, reps=args.reps,
                  planned_cases=len(cases), rows=rows, bins={}, failures=0)
    args.out.parent.mkdir(parents=True, exist_ok=True)

    def checkpoint():
        report['failures'] = failures
        args.out.write_text(json.dumps(report, indent=1) + '\n')

    for case in cases:
        row = dict(name=case['name'], sites=case['sites'], sequence=case['sequence'], frame=case['frame'],
                   provenance=case['provenance'], wall_median_ms=None, takes=[])
        rows.append(row)
        for _ in range(args.reps):
            take = run(args.bench, case['xyz'], case['ids'], args)
            row['takes'].append(take)
            failures += not take['ok']
            row['wall_median_ms'] = median([t['wall_ms'] for t in row['takes'] if t['ok']])
            checkpoint()  # une prise finie n'est jamais perdue
        print('%-16s %6d sites  mur %s ms' % (case['name'], case['sites'], row['wall_median_ms']), flush=True)
    bins = {}
    for lo, hi in BINS:
        walls = [r['wall_median_ms'] for r in rows if lo <= r['sites'] < hi and r['wall_median_ms'] is not None]
        bins['%d-%s' % (lo, hi if hi < 10 ** 9 else 'plus')] = dict(scenes=len(walls), median_ms=median(walls),
                                                                     max_ms=max(walls) if walls else None)
    report.update(status='complete', bins=bins,
                  bins_note='tranches descriptives de scenes : aucune ne qualifie une trame entiere')
    checkpoint()
    for key, value in bins.items():
        print('tranche %-12s scenes %3d  mediane %s ms  max %s ms' % (key, value['scenes'], value['median_ms'],
                                                                    value['max_ms']))
    return 0 if failures == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
