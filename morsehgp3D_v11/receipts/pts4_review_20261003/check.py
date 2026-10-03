#!/usr/bin/env python3
"""Contrelecture autonome de métadonnées déjà produites ; aucun fit, moteur ou GCP."""
from pathlib import Path
import argparse
import ast
import collections
import gzip
import hashlib
import json
from fractions import Fraction

BASE = Path(__file__).resolve().parent
TIME_KEYS = frozenset({'seconds', 'wall_seconds', 'full_ns', 'export_ns'})
CHECKS = 0


def need(condition, message):
    global CHECKS
    CHECKS += 1
    if not condition:
        raise ValueError(message)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def load(name):
    return json.loads((BASE / name).read_text())


def stripped(value):
    if isinstance(value, dict):
        return {k: stripped(v) for k, v in value.items() if k not in TIME_KEYS}
    if isinstance(value, list):
        return [stripped(v) for v in value]
    return value


def manifest(name):
    result = {}
    for line in (BASE / name).read_text().splitlines():
        digest, path = line.split('  ', 1)
        path = 'results/' + path.removeprefix('./')
        need(path not in result, 'duplicate manifest path')
        result[path] = digest
    return result


def cohort(rows):
    result = {'scenes': len(rows), 'instances': sum(row['objects'] for row in rows), 'orders': {}}
    for k in ['2', '3', '5', '10']:
        a = [x for row in rows for x in row['orders'][k]['margin_r']['best']]
        b = [x for row in rows for x in row['orders'][k]['hdbscan']['best']]
        need(len(a) == len(b) == result['instances'], 'instances / scores mismatch')
        aa, bb = sum(Fraction(str(x)) for x in a), sum(Fraction(str(x)) for x in b)
        result['orders'][k] = {
            'margin_r_sum': sum(a), 'hdbscan_sum': sum(b),
            'margin_r_mean': sum(a) / len(a), 'hdbscan_mean': sum(b) / len(b),
            'margin_r_mean_exact_from_persisted_scores': str(aa / len(a)),
            'hdbscan_mean_exact_from_persisted_scores': str(bb / len(b)),
            'rescues': sum(y <= .5 < x for x, y in zip(a, b)),
            'losses': sum(x <= .5 < y for x, y in zip(a, b)),
            'unit': 'one retained labelled instance in one scene, equally weighted',
        }
    result['sites_minmax'] = [min(r['sites'] for r in rows), max(r['sites'] for r in rows)]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path)
    args = parser.parse_args()
    source = load('SOURCE_BEFORE.json')
    raw = json.loads(gzip.decompress((BASE / 'case_metadata.json.gz').read_bytes()))
    need(set(raw) == {'claudepts3', 'claudepts4'}, 'sessions')
    data = {}
    phases = {}
    counts = {}
    gates = {}
    for session, expected in [('claudepts3', 258), ('claudepts4', 260)]:
        receipt = load(session + '/receipt_scope.json')
        need(receipt['status'] == 'failed_remote' and receipt['worker_exit_code'] == 1, 'overall status')
        need(receipt['targeted_shutdown_certified'] and receipt['observed_after']['status'] == 'TERMINATED', 'closure')
        need([x['status'] for x in receipt['commands']] == ['ok'] * 4 + ['deadline_cut'], 'command statuses')
        need(receipt['commands'][-1]['exit_code'] == '124', 'deadline code')
        m = manifest(session + '/RESULTS_MANIFEST.sha256')
        need(len(m) == (336 if session == 'claudepts3' else 338), 'closed inventory count')
        d = {}
        p = {}
        for record in raw[session]:
            need(sha(record['json_text'].encode()) == record['sha256'] == m[record['member']], 'case payload hash')
            row = json.loads(record['json_text'])
            need(row['name'] not in d and row['status'] == 'ok', 'duplicate / failed case')
            need(set(row['orders']) == {'2', '3', '5', '10'}, 'orders')
            need(row['export']['coord_bits'] == 21, 'compiled numeric profile')
            for order in row['orders'].values():
                need(len(order['margin_r']['best']) == len(order['hdbscan']['best']) == row['objects'], 'objects')
                need(all(0 <= x <= 1 for name in ['margin_r', 'hdbscan'] for x in order[name]['best']), 'IoU domain')
            d[row['name']] = row
            p[row['name']] = record['member'].split('/')[2]
        need(len(d) == expected, 'case count')
        data[session] = d
        phases[session] = p
        counts[session] = dict(collections.Counter(p.values()))
        gates[session] = load(session + '/raw/results/cmd/001_gate/files/gate.json')
        need(gates[session]['verdict'] == 'conforme' and not gates[session]['disagreements'], 'old gate')
        need(all(f['ok'] for f in gates[session]['fixtures']), 'old fixtures')
    common = sorted(data['claudepts3'].keys() & data['claudepts4'].keys())
    need(len(common) == 258, 'intersection')
    different = [name for name in common if stripped(data['claudepts3'][name]) != stripped(data['claudepts4'][name])]
    need(not different, 'non-time output differences')
    added = sorted(data['claudepts4'].keys() - data['claudepts3'].keys())
    need(added == ['c08_001189', 'c08_001190'], 'added scenes')
    inventory = load('points_manifest.json')
    need(sha((BASE / 'points_manifest.json').read_bytes()) == source['input_inventory_sha256'], 'input inventory')
    roles = {row['name']: row for row in inventory['scenes']}
    latest = data['claudepts4']
    lidar_a = [r for n, r in latest.items() if phases['claudepts4'][n] == '003_lidar-a']
    ranked = sorted(lidar_a, key=lambda r: ({'demo': 0, 'echec': 1, 'temoin': 2}[roles[r['name']]['role']], r['name']))
    selected = {'demo': [], 'echec': [], 'temoin': []}
    seen = {}
    duplicates = []
    for row in ranked:
        spec = roles[row['name']]
        need(row['meta']['sites_sha256'] == spec['sites_sha256'], 'input XYZ signature')
        key = (spec['sites_sha256'], spec['labels_sha256'])
        if key in seen:
            duplicates.append({'removed': row['name'], 'retained': seen[key]})
        else:
            seen[key] = row['name']
            selected[spec['role']].append(row)
    neighbours = [r for n, r in latest.items() if phases['claudepts4'][n] == '004_lidar-b']
    cohorts = {role: cohort(rows) for role, rows in selected.items()}
    cohorts['voisin'] = cohort(neighbours)
    need([(cohorts[x]['scenes'], cohorts[x]['instances']) for x in ['demo', 'echec', 'temoin', 'voisin']] ==
         [(5, 72), (37, 425), (20, 204), (68, 824)], 'cohort counts')
    need(len(duplicates) == 2, 'duplicate count')
    prepared = load('claudepts4/raw/results/cmd/000_prepare/files/prepare.json')
    need(prepared['status'] == 0 and prepared['scenes'] == len(prepared['frames']) == 72, 'prepared neighbourhood')
    need(all(x['replay']['identical'] and not x['replay']['gaps'] for x in prepared['frames']), 'mask replay')
    missing = sorted({x['name'] for x in prepared['frames']} - {x['name'] for x in neighbours})
    need(len(missing) == 4, 'missing persistent results')
    # A module-docstring-only edit is not a different mathematical implementation.
    played = ast.parse((BASE / 'claudepts4/played/morsehgp3D_v11/bench/points_radius.py').read_text())
    live = ast.parse((BASE / 'live/morsehgp3D_v11/bench/points_radius.py').read_text())
    played.body.pop(0)
    live.body.pop(0)
    need(ast.dump(played) == ast.dump(live), 'radius code changed beyond docstring')
    result = {
        'status': 'review_conforming_metadata', 'checks': CHECKS,
        'counts': counts, 'common_identical_non_time': len(common), 'different_non_time': different,
        'time_keys_ignored_recursively': sorted(TIME_KEYS), 'added_in_pts4': added,
        'cohorts_pts4_deduplicated': cohorts, 'duplicates_removed': duplicates,
        'missing_persistent_neighbour_results_pts4': missing,
        'missing_launch_status': 'unknown; absence of result does not prove not launched',
        'quality_scope': 'Exact equality of persisted JSON observables. IoU best values are rounded to six decimals; no complete date/owner/FULL binary equality claim.',
        'numeric_scope': 'CPU reference on G4, compiled u21; not GPU or whole-domain u21 qualification.',
        'gate_scope': 'Played eb467 old gate only; LIVE stricter judge and mutants are not inherited.',
        'cohort_unit_limit': 'Neighbour instance observations are correlated across nearby frames; not 824 independent physical objects.',
        'score_limit': 'Best block IoU; no condensation, selection or universal superiority qualification.',
        'native_execution': False, 'gcp_reads_or_mutations': False,
    }
    text = json.dumps(result, sort_keys=True, ensure_ascii=False, indent=2) + '\n'
    if args.out:
        args.out.write_text(text)
    else:
        print(text, end='')


if __name__ == '__main__':
    main()
