#!/usr/bin/env python3
"""Pente LiDAR locale de la chaine v9 : sous-nuages emboites et sept morceaux.

Entrees : les trames sans sol a 1 mm du recu v8 lidar_ground_20260921 (deja
dans le depot, jamais recopiees ici). Pour chaque trame demandee :
- sous-nuages EMBOITES de 8 000, 16 000 et 32 000 sites : sites tries par
  distance horizontale au centre median (x, y), puis par rang d'entree ; ce
  sont des disques croissants autour du centre, PAS des coupes par plans du
  capteur ;
- les sept morceaux figes du recu v8 (entiere, deux moities, quatre quarts,
  coupes par plans du capteur), controles contre les empreintes de son
  MANIFEST.json.
Chaque nuage derive est ecrit dans un repertoire de travail hors depot (octets
KITTI derives, jamais versionnes) ; ses identifiants de sites retenus sont
epingles (SHA-256) et l'emboitement 8k < 16k < 32k est verifie. Chaque cas
passe par mhgp9_tower_probe (K, W, s, leviers par defaut) et n'est accepte
que complet : statut complete_relative, K_effective = K, ordres 1..K,
options conformes, schema de sonde attendu. Un echec laisse un recu type
(commande, code, mur, raison, stdout/stderr) et arrete la campagne.

  run_lidar_scaling.py --probe build/v9-dev/mhgp9_tower_probe --scene 00 \\
      --k 5 --workers 8 --repeat 0 --work DIR --out DIR

La sortie `--out` doit etre vide (jamais d'ecrasement silencieux) ; l'identite
de chaque cas porte trame, K, s, W et repetition. Diagnostic de croissance
seulement (CLAUDE.md, TEST_PLAN 3.1) : ni le contrat de trame entiere, ni une
mesure G4. Aucun assert : tient sous python3 -O.
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
PROBE_SCHEMA = 'mhgp9_tower_probe_v12'
PIECES = ('full', 'half_x_neg', 'half_x_nonneg', 'quarter_x_neg_y_neg', 'quarter_x_neg_y_nonneg',
          'quarter_x_nonneg_y_neg', 'quarter_x_nonneg_y_nonneg')
NESTED = (8000, 16000, 32000)
DEFAULT_LEVERS = dict(atlas_saturate_deep=True, q3_leaf_census=True, q34_dead_lanes=True, q34_witness_cache=True,
                      q34_dead_core=True, tower_meb_proposal=True)
# Travail publie : front, filtres (visites des DFS de temoins), covers et
# noyaux, preuves, graines et DFS q3/q4, atlas, balayages q4, catalogue, tour.
WORK_KEYS = (
    ('generator', 'q2_candidate_pairs'), ('generator', 'q2_accepted_pairs'),
    ('ledger', 'q34_input_rectangles'), ('ledger', 'witness_input_pair_mass'), ('ledger', 'witness_rect_node_visits'),
    ('ledger', 'expanded_pairs'), ('ledger', 'witness_pair_queries'), ('ledger', 'witness_pair_node_visits'),
    ('ledger', 'witness_cache_node_tests'), ('ledger', 'core_builds'), ('ledger', 'core_sites'),
    ('ledger', 'core_cover_node_visits'), ('ledger', 'cover_builds'), ('ledger', 'cover_sites'),
    ('ledger', 'cover_node_visits'), ('ledger', 'dead_core_uniform_tests'), ('ledger', 'dead_uniform_tests'),
    ('ledger', 'q3_edges'), ('ledger', 'q4_edges'), ('ledger', 'q3_seed_node_visits'), ('ledger', 'q3_seeds'),
    ('ledger', 'q3_census_point_tests'), ('ledger', 'q3_leaf_point_tests'), ('ledger', 'q4_domain_node_visits'),
    ('ledger', 'q4_cover_decomposition_node_visits'), ('ledger', 'q4_seed_node_visits'), ('ledger', 'q4_seeds'),
    ('ledger', 'q4_sweep_active_sites'), ('ledger', 'q4_sweep_events'), ('ledger', 'atlas_node_visits'),
    ('ledger', 'atlas_point_tests'), ('generator', 'q3_emitted'), ('generator', 'q4_emitted'),
    ('catalogue', 'balls'), ('tower_work', 'meb_calls'), ('tower_work', 'intruder_nodes'),
    ('tower_work', 'contributions'))


class Refusal(Exception):
    pass


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def read_u32(path, width):
    raw = path.read_bytes()
    if len(raw) % (4 * width):
        raise Refusal('not a u32le file of width %d: %s' % (width, path))
    return raw, [struct.unpack_from('<%dI' % width, raw, i) for i in range(0, len(raw), 4 * width)]


def nested_indices(points, size):
    xs = sorted(p[0] for p in points)
    ys = sorted(p[1] for p in points)
    cx, cy = xs[len(xs) // 2], ys[len(ys) // 2]
    order = sorted(range(len(points)), key=lambda i: ((points[i][0] - cx) ** 2 + (points[i][1] - cy) ** 2, i))
    return sorted(order[:size])


def validate_probe(value, args, sites):
    need = [value.get('schema') == PROBE_SCHEMA, value.get('status') == 'complete_relative',
            value.get('input', {}).get('sites') == sites]
    options = value.get('options', {})
    need += [options.get('K') == args.k, options.get('K_effective') == min(args.k, sites), options.get('s') == args.s,
             options.get('workers') == args.workers, options.get('run_tower') is True,
             options.get('levers') == DEFAULT_LEVERS]
    need += [[order.get('K') for order in value.get('orders', [])] == list(range(1, min(args.k, sites) + 1))]
    need += [all(type(value.get(section, {}).get(key)) is int for section, key in WORK_KEYS)]
    return all(need)


def run_case(args, name, path, provenance, out_dir, sites):
    argv = [str(args.probe), str(path), str(args.k), str(args.workers), '--s=' + str(args.s)]
    started = time.monotonic()
    completed = subprocess.run(argv, capture_output=True, text=True)
    elapsed = time.monotonic() - started
    text = completed.stdout
    begin, end = text.find('{'), text.rfind('}')
    record = dict(case=name, argv=argv[1:], exit_code=completed.returncode, external_wall_s=round(elapsed, 3),
                  input=provenance)
    try:
        if completed.returncode != 0 or begin < 0:
            raise Refusal('probe exit code %d' % completed.returncode)
        value = json.loads(text[begin:end + 1])
        if not validate_probe(value, args, sites):
            raise Refusal('probe output not a complete tower with the expected options/schema')
    except (Refusal, ValueError) as error:
        record.update(outcome='refused', reason=str(error), stdout=text[-4000:], stderr=completed.stderr[-4000:])
        (out_dir / (name + '.failure.json')).write_text(json.dumps(record, indent=1, sort_keys=True) + '\n')
        raise Refusal(name + ': ' + str(error))
    record.update(outcome='complete_relative', probe=value)
    (out_dir / (name + '.json')).write_text(json.dumps(record, indent=1, sort_keys=True) + '\n')
    return value


def slopes(rows, key):
    values = [(r['sites'], r[key]) for r in rows]
    return [round(math.log(b[1] / a[1]) / math.log(b[0] / a[0]), 3) if a[1] > 0 and b[1] > 0 else None
            for a, b in zip(values, values[1:])]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--scene', choices=('00', '01', '02'), required=True)
    parser.add_argument('--k', type=int, choices=(5, 10), required=True)
    parser.add_argument('--workers', type=int, default=8)
    parser.add_argument('--s', type=int, default=8)
    parser.add_argument('--repeat', type=int, default=0)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    try:
        if args.s < 8:
            raise Refusal('s < 8 is never measured')
        if args.out.exists() and any(args.out.iterdir()):
            raise Refusal('output directory is not empty: ' + str(args.out))
        args.work.mkdir(parents=True, exist_ok=True)
        args.out.mkdir(parents=True, exist_ok=True)
        grid = V8 / ('scene_' + args.scene + '_grid')
        manifest_raw = (grid / 'MANIFEST.json').read_bytes()
        manifest = json.loads(manifest_raw)
        full_raw, full = read_u32(grid / 'full.u32le', 3)
        ids_raw, ids = read_u32(grid / 'full.site_ids.u32le', 1)
        entry = manifest['datasets']['full']
        if sha(full_raw) != entry['points_sha256'] or sha(ids_raw) != entry['site_ids_sha256'] or \
                len(full) != entry['sites'] or len(ids) != len(full):
            raise Refusal('full frame differs from its v8 manifest')
        probe_sha = sha(args.probe.read_bytes())
        head = subprocess.run(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
        trees = {name: subprocess.run(['git', '-C', str(ROOT), 'rev-parse', 'HEAD:morsehgp3D_v9/' + name],
                                      capture_output=True, text=True).stdout.strip() for name in ('src', 'bench')}
        dirty = subprocess.run(['git', '-C', str(ROOT), 'status', '--porcelain', '--', 'morsehgp3D_v9/src',
                                'morsehgp3D_v9/bench'], capture_output=True, text=True).stdout
        tag = 's%s_k%d_s%d_w%d_r%d' % (args.scene, args.k, args.s, args.workers, args.repeat)
        cases = []
        previous = None
        for size in NESTED:
            if size > len(full):
                continue
            chosen = nested_indices(full, size)
            if previous is not None and not set(previous) <= set(chosen):
                raise Refusal('nested subsets are not nested')
            previous = chosen
            name = tag + '_nested_%d' % size
            path = args.work / (name + '.u32le')
            path.write_bytes(b''.join(struct.pack('<3I', *full[i]) for i in chosen))
            id_raw = b''.join(struct.pack('<I', ids[i][0]) for i in chosen)
            cases.append((name, size, path, dict(kind='nested_disc', parent_points_sha256=entry['points_sha256'],
                                                  points_sha256=sha(path.read_bytes()), site_ids_sha256=sha(id_raw),
                                                  sites=size)))
        for piece in PIECES:
            data = manifest['datasets'][piece]
            path = grid / data['points_file']
            raw = path.read_bytes()
            if sha(raw) != data['points_sha256'] or len(raw) // 12 != data['sites']:
                raise Refusal('piece differs from its v8 manifest: ' + piece)
            cases.append((tag + '_piece_' + piece, data['sites'], path,
                          dict(kind='v8_sensor_piece', piece=piece, points_sha256=data['points_sha256'],
                               site_ids_sha256=data['site_ids_sha256'], sites=data['sites'])))
        rows = []
        for name, size, path, provenance in cases:
            value = run_case(args, name, path, provenance, args.out, size)
            row = dict(case=name, sites=size, digest=value['tower_digest'],
                       chain_s=round(value['times_ms']['chain_total'] / 1000, 3),
                       digest_s=round(value['times_ms']['digest'] / 1000, 3), cpu_s=value['chain_cpu_s'],
                       peak_rss_kb=value['peak_rss_kb'])
            for section, key in WORK_KEYS:
                row[key] = value[section][key]
            rows.append(row)
        nested_rows = sorted((r for r in rows if '_nested_' in r['case']), key=lambda r: r['sites'])
        summary = dict(schema='mhgp9_lidar_scaling_v2', scene=args.scene, k=args.k, workers=args.workers, s=args.s,
                       repeat=args.repeat, public_status='not_claimed', probe_sha256=probe_sha, git_head=head,
                       git_trees=trees, worktree_dirty_src_or_bench=bool(dirty.strip()),
                       v8_manifest_sha256=sha(manifest_raw), mask=manifest.get('mask'), profile=manifest.get('profile'),
                       rows=rows, nested_slopes={key: slopes(nested_rows, key)
                                                 for key in [k for _, k in WORK_KEYS] + ['cpu_s', 'chain_s']})
        (args.out / ('SUMMARY_' + tag + '.json')).write_text(json.dumps(summary, indent=1) + '\n')
        print(json.dumps(dict(case=tag, nested_slopes=summary['nested_slopes']), sort_keys=True))
        return 0
    except Refusal as error:
        print('refused: ' + str(error), file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
