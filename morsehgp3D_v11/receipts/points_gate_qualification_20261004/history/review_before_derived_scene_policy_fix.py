#!/usr/bin/env python3
"""Read only preserved G4 metadata: no native executable, import of the engine, fit or cloud call."""
from pathlib import Path
import argparse
import collections
from fractions import Fraction
import gzip
import hashlib
import io
import json
import tarfile

ROOT = Path(__file__).resolve().parent
CHECKS = 0
TIME = {'seconds', 'wall_seconds', 'full_ns', 'export_ns'}


def need(ok, why):
    global CHECKS
    CHECKS += 1
    if not ok:
        raise ValueError(why)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def load(path):
    return json.loads((ROOT / path).read_text())


def no_time(obj):
    if isinstance(obj, dict):
        return {k: no_time(v) for k, v in obj.items() if k not in TIME}
    if isinstance(obj, list):
        return [no_time(v) for v in obj]
    return obj


def read_archive(session):
    receipt = load(session + '/receipt_scope.json')
    raw = (ROOT / session / 'results.tar.gz').read_bytes()
    need(sha(raw) == receipt['results_sha256'], 'archive SHA')
    need(receipt['targeted_shutdown_certified'] is True, 'shutdown certificate')
    need(receipt['observed_after']['status'] == 'TERMINATED', 'stopped target')
    need(receipt['observed_after']['lastStartTimestamp'] == receipt['generation'], 'same generation at closure')
    files = {}
    with tarfile.open(fileobj=io.BytesIO(raw)) as archive:
        for member in archive:
            need(not member.name.startswith('/') and '..' not in Path(member.name).parts, 'unsafe member')
            if member.isfile():
                need(member.name not in files, 'duplicate archive member')
                files[member.name] = archive.extractfile(member).read()
    manifest = files['results/MANIFEST.sha256'].decode()
    expected = {}
    for line in manifest.splitlines():
        digest, name = line.split('  ', 1)
        name = 'results/' + name.removeprefix('./')
        need(name not in expected, 'duplicate manifest entry')
        need(name in files and sha(files[name]) == digest, 'payload SHA')
        expected[name] = digest
    need(set(files) - {'results/MANIFEST.sha256'} == set(expected), 'complete inventory')
    cases = {}
    phases = {}
    for path, raw in files.items():
        if path.endswith('.json') and '/files/' in path and ('/lidar/' in path or '/synthetic/' in path):
            case = json.loads(raw)
            need(case['name'] not in cases, 'duplicate case name')
            need(case['status'] == 'ok' and case['export']['coord_bits'] == 21, 'case / numeric profile')
            need(set(case['orders']) == {'2', '3', '5', '10'}, 'measured orders')
            cases[case['name']] = case
            phases[case['name']] = path.split('/')[2]
    return receipt, files, cases, phases, len(expected)


def source_check(session):
    records = json.loads(gzip.decompress((ROOT / session / 'package_members.json.gz').read_bytes()))
    metadata = load(session + '/package_check.json')
    need(metadata['all_regular_files_match_Git'] and len(records) == 5385, 'source package snapshot')
    by_name = {r['path']: r for r in records}
    need(len(by_name) == len(records), 'duplicate package source')
    for path in (ROOT / session / 'played').rglob('*'):
        if path.is_file():
            name = str(path.relative_to(ROOT / session / 'played'))
            raw = path.read_bytes()
            need(sha(raw) == by_name[name]['sha256'], 'preserved played source SHA')
            need(hashlib.sha1(('blob ' + str(len(raw)) + '\0').encode() + raw).hexdigest() ==
                 by_name[name]['git_blob'], 'preserved source Git blob')
    return by_name


def scores(rows):
    result = {'scenes': len(rows), 'instance_observations': sum(r['objects'] for r in rows), 'orders': {}}
    for k in ['2', '3', '5', '10']:
        aa = [x for r in rows for x in r['orders'][k]['margin_r']['best']]
        bb = [x for r in rows for x in r['orders'][k]['hdbscan']['best']]
        need(len(aa) == len(bb) == result['instance_observations'], 'score counts')
        a, b = sum(Fraction(str(x)) for x in aa), sum(Fraction(str(x)) for x in bb)
        result['orders'][k] = {
            'margin_r_mean_of_persisted_scores': str(a / len(aa)),
            'hdbscan_mean_of_persisted_scores': str(b / len(bb)),
            'rescues': sum(y <= .5 < x for x, y in zip(aa, bb)),
            'losses': sum(x <= .5 < y for x, y in zip(aa, bb)),
        }
    result['sites_minmax'] = [min(r['sites'] for r in rows), max(r['sites'] for r in rows)]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path)
    args = parser.parse_args()
    sessions = {}
    for name in ['claudepts4', 'claudepts5', 'claudepts6']:
        sessions[name] = read_archive(name)
    e, efiles, ecases, _, _ = sessions['claudepts5']
    f, ffiles, fcases, fphases, _ = sessions['claudepts6']
    need(e['commit'] == '6c88fe0ed95541d787943fb351cede73033eeef2' and e['source_kind'] == 'commit', 'E source')
    need(f['commit'] == 'f02f91c7ece65598eefef1ad14d0c73a8c539466' and f['source_kind'] == 'commit', 'F source')
    need(e['status'] == 'failed_remote' and e['worker_exit_code'] == 1, 'E failure preserved')
    need(f['status'] == 'completed' and f['worker_exit_code'] == 0, 'F complete')
    need([c['exit_code'] for c in e['commands']] == ['0', '1', '2', '2', '2'], 'E exact command codes')
    need(not ecases, 'E has no measured cases')
    trace = b''.join(raw for name, raw in efiles.items() if '_gate/' in name and name.endswith('/stderr'))
    need(b'FileNotFoundError' in trace and b'/gate/mutants/qualification_decalee/' in trace, 'E exact cause')
    need(all(c['status'] == 'ok' and c['exit_code'] == '0' for c in f['commands']), 'F all commands pass')
    need(len(fcases) == 205, 'F 205 persisted cases')
    phase_counts = dict(collections.Counter(fphases.values()))
    need(phase_counts == {'004_synthetic': 128, '002_lidar-demo': 5, '003_lidar-b': 72}, 'F phase scope')
    gate = json.loads(ffiles['results/cmd/001_gate/files/gate.json'])
    need(gate['verdict'] == 'conforme' and not gate['disagreements'], 'strict gate verdict')
    need((gate['clouds'], gate['comparisons'], gate['radius_sites']) == (2854, 194520, 215974), 'strict gate counts')
    need(len(gate['fixtures']) == 12 and all(q['ok'] for q in gate['fixtures']), 'fixtures')
    mutants = gate['mutants']
    need(set(mutants) == {'coupe_ouverte', 'marge_carree', 'qualification_decalee', 'sans_marge'}, 'mutant inventory')
    need(all(q['killed'] for q in mutants.values()), 'mutants killed')
    need(mutants['coupe_ouverte']['how'] == ['invariant:proprietaire_non_vivant:margin_r'], 'open cut exact cause')
    for name in ['marge_carree', 'qualification_decalee', 'sans_marge']:
        need(any(h.startswith('fixtures:') for h in mutants[name]['how']) and
             'desaccord:m000' in mutants[name]['how'], 'causal result mismatch')
    d, _, dcases, _, _ = sessions['claudepts4']
    common = sorted(dcases.keys() & fcases.keys())
    need(len(common) == 201, 'D/F intersection')
    different = [name for name in common if no_time(dcases[name]) != no_time(fcases[name])]
    need(not different, 'D/F non-time output differences')
    need(d['binary_hashes'] == e['binary_hashes'] == f['binary_hashes'], 'same declared exporter ELF')
    emap, fmap = source_check('claudepts5'), source_check('claudepts6')
    changed = [name for name in emap if emap[name]['sha256'] != fmap[name]['sha256']]
    need(changed == ['morsehgp3D_v11/bench/points_gate.py'], 'E/F source delta only gate mkdir')
    prep = json.loads(ffiles['results/cmd/000_prepare/files/prepare.json'])
    need(prep['status'] == 0 and prep['probe']['ok'] and prep['scenes'] == 72, 'prepared neighbours')
    need(all(x['replay']['identical'] and not x['replay']['gaps'] for x in prep['frames']), 'mask replay')
    published = ROOT / 'published/morsehgp3D_v11/receipts/developpement_20261003/points_g4'
    manifests = [json.loads((published / name).read_text()) for name in
                 ['data_manifest_pts1.json', 'data_manifest_pts2.json', 'data_manifest_pts2_roles.json',
                  'data_manifest_pts3.json']]
    by_name = {}
    for inventory in manifests:
        for row in inventory['scenes']:
            by_name.setdefault(row['name'], {}).update(row)
    demos = [r for name, r in fcases.items() if fphases[name] == '002_lidar-demo']
    neighbours = [r for name, r in fcases.items() if fphases[name] == '003_lidar-b']
    seen = {}
    unique_neighbours = []
    duplicates = []
    for row in demos + neighbours:
        spec = by_name[row['name']]
        need(row['meta']['sites_sha256'] == spec['sites_sha256'], 'matched input XYZ digest')
        key = (spec['sites_sha256'], spec['labels_sha256'])
        if key in seen:
            duplicates.append({'removed': row['name'], 'retained': seen[key]})
        else:
            seen[key] = row['name']
            if row in neighbours:
                unique_neighbours.append(row)
    need(duplicates == [{'removed': 'c08_000882', 'retained': 'zoltan_02_velos_contre_facade'}], 'one duplicate input')
    need(len(unique_neighbours) == 71, 'unique neighbour scope')
    result = {
        'status': 'conforming_closed_metadata_review', 'checks': CHECKS,
        'publication_pin': 'ab1a739d17f801823a66d74609209696152c8705',
        'executed_F_pin': f['commit'], 'gate': {k: gate[k] for k in ['clouds', 'comparisons', 'radius_sites', 'mutants']},
        'F_case_counts': phase_counts, 'common_D_F': len(common), 'different_non_time': different,
        'ignored_time_keys': sorted(TIME), 'duplicate_inputs': duplicates,
        'F_demo': scores(demos), 'F_unique_neighbours': scores(unique_neighbours),
        'actual_F_gate_scope': 'At most nine sites, k<=4; m in {1,k+1,k+2} when m<=n. radius_sites counts repeated site comparisons.',
        'actual_F_mutant_scope': 'Four Python consumer mutations evaluated with the unchanged C++ exporter. Three result mismatches; one consumer invariant. No C++ mutant or native PointRadiusDate port.',
        'quality_scope': 'Persisted best-block IoU rounded to six decimals. No condensation/selection, full date-owner dumps, universal improvement or 100ms qualification.',
        'numeric_scope': 'CPU reference, compiled u21 on G4. No GPU or whole-domain u21/u24 qualification.',
        'E_failure_scope': 'Missing directory while writing mutant XYZ input; campaigns refuse absent gate. Not a native arithmetic failure.',
        'native_execution': False, 'fit': False, 'gcp_operations': False,
    }
    # scores() adds independent count checks; record the final total.
    result['checks'] = CHECKS
    text = json.dumps(result, indent=2, ensure_ascii=False, sort_keys=True) + '\n'
    if args.out:
        args.out.write_text(text)
    else:
        print(text, end='')


if __name__ == '__main__':
    main()
