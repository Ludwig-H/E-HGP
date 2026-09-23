#!/usr/bin/env python3
"""Pente LiDAR locale de la chaine v9 : coupes emboitees et sept morceaux.

Entrees : les trames sans sol a 1 mm du recu v8 lidar_ground_20260921 (deja
dans le depot, jamais recopiees ici). Pour chaque trame demandee :
- sous-nuages EMBOITES de 8 000, 16 000 et 32 000 sites : les sites tries par
  distance horizontale au centre median (x, y), puis par rang d'entree ;
- les sept morceaux figes du recu v8 (entiere, deux moitiés, quatre quarts).
Chaque nuage est ecrit dans un repertoire de travail hors depot (octets
KITTI derives, jamais versionnes), puis passe a mhgp9_tower_probe (K, W, s,
leviers par defaut). La sortie est un JSON par cas avec l'empreinte SHA-256
de l'entree, et un resume des pentes log-log entre tailles emboitees.

  run_lidar_scaling.py --probe build/v9-dev/mhgp9_tower_probe --scene 00 \\
      --k 5 --workers 8 --work DIR --out DIR

Diagnostic de croissance seulement (CLAUDE.md, TEST_PLAN 3.1) : ni le contrat
de trame entiere ni une mesure G4. Aucun assert : tient sous python3 -O.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
V8 = ROOT / 'morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6'
PIECES = ('full', 'half_x_neg', 'half_x_nonneg', 'quarter_x_neg_y_neg', 'quarter_x_neg_y_nonneg',
          'quarter_x_nonneg_y_neg', 'quarter_x_nonneg_y_nonneg')
NESTED = (8000, 16000, 32000)
WORK_KEYS = (('ledger', 'witness_input_pair_mass'), ('ledger', 'expanded_pairs'), ('ledger', 'core_builds'),
             ('ledger', 'cover_builds'), ('ledger', 'cover_sites'), ('ledger', 'core_sites'),
             ('ledger', 'q3_seeds'), ('ledger', 'q4_seeds'), ('ledger', 'atlas_point_tests'),
             ('generator', 'q2_candidate_pairs'), ('generator', 'q3_emitted'), ('generator', 'q4_emitted'),
             ('catalogue', 'balls'), ('tower_work', 'meb_calls'), ('tower_work', 'intruder_nodes'),
             ('tower_work', 'contributions'))


def fail(message):
    print('refused: ' + message, file=sys.stderr)
    sys.exit(2)


def read_points(path):
    raw = path.read_bytes()
    if len(raw) % 12:
        fail('not a u32le triple file: ' + str(path))
    return [struct.unpack_from('<3I', raw, i) for i in range(0, len(raw), 12)]


def write_points(path, points):
    path.write_bytes(b''.join(struct.pack('<3I', *p) for p in points))
    return hashlib.sha256(path.read_bytes()).hexdigest()


def nested(points, size):
    xs = sorted(p[0] for p in points)
    ys = sorted(p[1] for p in points)
    cx, cy = xs[len(xs) // 2], ys[len(ys) // 2]
    order = sorted(range(len(points)), key=lambda i: ((points[i][0] - cx) ** 2 + (points[i][1] - cy) ** 2, i))
    return [points[i] for i in sorted(order[:size])]


def run_case(args, name, path, digest, out_dir):
    argv = [str(args.probe), str(path), str(args.k), str(args.workers), '--s=' + str(args.s)]
    started = time.monotonic()
    completed = subprocess.run(argv, capture_output=True, text=True)
    elapsed = time.monotonic() - started
    text = completed.stdout
    begin, end = text.find('{'), text.rfind('}')
    if completed.returncode != 0 or begin < 0:
        fail('probe failed on ' + name + ': ' + completed.stderr[-400:])
    value = json.loads(text[begin:end + 1])
    record = dict(case=name, input_sha256=digest, argv=argv[1:], exit_code=completed.returncode,
                  external_wall_s=round(elapsed, 3), probe=value)
    (out_dir / (name + '.json')).write_text(json.dumps(record, indent=1, sort_keys=True) + '\n')
    return value


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--scene', choices=('00', '01', '02'), required=True)
    parser.add_argument('--k', type=int, choices=(5, 10), required=True)
    parser.add_argument('--workers', type=int, default=8)
    parser.add_argument('--s', type=int, default=8)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    if args.s < 8:
        fail('s < 8 is never measured')
    grid = V8 / ('scene_' + args.scene + '_grid')
    args.work.mkdir(parents=True, exist_ok=True)
    args.out.mkdir(parents=True, exist_ok=True)
    full = read_points(grid / 'full.u32le')
    results = {}
    for size in NESTED:
        if size > len(full):
            continue
        name = 's%s_k%d_nested_%d' % (args.scene, args.k, size)
        path = args.work / (name + '.u32le')
        digest = write_points(path, nested(full, size))
        results[name] = (size, run_case(args, name, path, digest, args.out))
    for piece in PIECES:
        name = 's%s_k%d_piece_%s' % (args.scene, args.k, piece)
        path = grid / (piece + '.u32le')
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        results[name] = (len(path.read_bytes()) // 12, run_case(args, name, path, digest, args.out))
    rows = []
    for name, (size, value) in results.items():
        row = dict(case=name, sites=size, status=value['status'], digest=value['tower_digest'],
                   chain_s=round(value['times_ms']['chain_total'] / 1000, 3), cpu_s=value['chain_cpu_s'])
        for section, key in WORK_KEYS:
            row[key] = value[section][key]
        rows.append(row)
    slopes = {}
    nested_rows = [r for r in rows if '_nested_' in r['case']]
    nested_rows.sort(key=lambda r: r['sites'])
    for (section, key) in WORK_KEYS + (('', 'cpu_s'),):
        values = [(r['sites'], r[key]) for r in nested_rows]
        slopes[key] = [round(math.log(b[1] / a[1]) / math.log(b[0] / a[0]), 3) if a[1] > 0 and b[1] > 0 else None
                       for a, b in zip(values, values[1:])]
    summary = dict(schema='mhgp9_lidar_scaling_v1', scene=args.scene, k=args.k, workers=args.workers, s=args.s,
                   public_status='not_claimed', rows=rows, nested_slopes=slopes)
    (args.out / ('SUMMARY_s%s_k%d.json' % (args.scene, args.k))).write_text(json.dumps(summary, indent=1) + '\n')
    print(json.dumps(dict(scene=args.scene, k=args.k, slopes=slopes), sort_keys=True))
    return 0


if __name__ == '__main__':
    sys.exit(main())
