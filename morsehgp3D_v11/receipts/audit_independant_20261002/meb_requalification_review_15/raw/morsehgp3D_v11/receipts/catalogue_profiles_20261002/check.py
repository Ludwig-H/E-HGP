"""LIVE profile-campaign reader: original local receipt + one original archive required.

Code0 means evidence coherence, including failed campaigns. Canonical files were removed remotely;
semantic/raw digests are recorded results, not hashes recomputed from archived canonical payloads.
"""
import importlib.util
import math
from pathlib import Path
import sys
import tarfile

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('catalogue_receipt_reader', HERE.parent / 'catalogue_20261002/check.py')
old = importlib.util.module_from_spec(spec)
spec.loader.exec_module(old)
need, js, sha, fields, BASE, HEX = old.need, old.js, old.sha, old.fields, old.BASE, old.HEX
BENCH = 'results/cmd/001_profiles/'
PROFILES = {18: 'gcc_release', 21: 'bits21', 24: 'bits24'}
CONFIG_BITS = dict(gcc_release=18, mutants=18, gcc_asan_ubsan=24, gcc_tsan=21, clang_release=21,
                   bits21=21, bits24=24, poison=21, style=21)
LOGICAL = {'nodes', 'leaves', 'filter_tests', 'dominance_tests', 'prefixes', 'judged', 'census_tests',
           'max_leaf', 'max_depth'}


def unit(row):
    return row['case'], row['coord_bits'], row['kmax'], row['repetition']


def finite(value):
    return type(value) in (int, float) and math.isfinite(value) and value >= 0


def workload(row):
    event = row['events'][1]
    logical = event['logical']
    need(set(logical) == LOGICAL and all(type(v) is int and v >= 0 for v in logical.values()), 'compteurs')
    need(type(event['generation_passes']) is int and event['generation_passes'] == 2, 'passes')
    return (2, *(logical[name] for name in sorted(LOGICAL)))


def compare(rows):
    out = []
    for name in sorted(old.CASE_COUNTS):
        for kmax in (5, 10):
            matches = [r for r in rows if r['case'] == name and r['kmax'] == kmax and r['status'] == 'ok']
            equal = len({r['semantic']['sha256'] for r in matches}) <= 1
            same_work = len({workload(r) for r in matches}) <= 1
            out.append({'case': name, 'kmax': kmax, 'successful_profiles': [r['coord_bits'] for r in matches],
                        'semantic_equal': equal, 'geometric_work_equal': same_work,
                        'status': 'different' if not equal or not same_work else
                        'equal' if len(matches) == 3 else 'incomplete'})
    return out


def attempt(row, case, build, complete, last):
    _, bits, kmax, repetition = unit(row)
    argv = row['argv']
    need(repetition == 0 and row['whole_input'] is True and row['count'] == case['count'] and
         row['timeout_seconds'] == 30 and len(argv) == 10 and argv[0] == build['path'] and
         Path(argv[1]).name == case['coordinates'] and Path(argv[2]).name == case['point_ids'] and
         Path(argv[3]).name == '%s_b%d_k%d.bin' % (case['name'], bits, kmax) and
         argv[4:] == [str(kmax), '16', '256', '0', str(2**32 - 1), str(8 * 1024**3)], 'commande')
    need(finite(row['process_wall_seconds']), 'duree_processus')
    state = row['status']
    if state == 'pending_semantic':
        need(not complete and last and row['exit_code'] == 0 and not row['errors'] and
             not {'semantic', 'canonical_sha256', 'canonical_bytes'} & set(row), 'checkpoint')
        parsed = old.attempt_events(dict(row, status='ok'), True)
    else:
        parsed = old.attempt_events(row, True)
    need(parsed == row['events'], 'evenements')
    if state != 'ok':
        need(state in ('pending_semantic', 'timeout', 'refused', 'failed', 'launch_error',
                       'invalid_output', 'artifact_error'), 'statut')
        need(state == 'artifact_error' or not {'semantic', 'canonical_sha256', 'canonical_bytes'} & set(row),
             'artefact_apres_echec')
        if 'canonical_sha256' in row:
            need(HEX.fullmatch(row['canonical_sha256']), 'empreinte_partielle')
        if 'canonical_bytes' in row:
            need(type(row['canonical_bytes']) is int and row['canonical_bytes'] >= 0, 'taille_partielle')
        return
    need(row['exit_code'] == 0 and [e['phase'] for e in parsed] == ['cloud', 'catalogue', 'exit'] and
         all(e.get('status', 'ok') == 'ok' for e in parsed), 'faux_succes')
    cloud, cat, _ = parsed
    need(cloud['points'] == cloud['sites'] == case['count'] and cat['coord_bits'] == bits and cat['kmax'] == kmax,
         'identite_calcul')
    need(all(type(obj[key]) is int and obj[key] >= 0 for obj, keys in
             [(cloud, ('points', 'sites', 'read_ns', 'cloud_ns', 'cloud_peak_bytes')),
              (cat, ('balls', 'levels', 'incidences', 'wall_ns', 'peak_reserved_bytes', 'reserved_after_bytes'))]
             for key in keys) and cat['balls'] > 0 and cat['reserved_after_bytes'] <= cat['peak_reserved_bytes'] <=
         8 * 1024**3 and cloud['cloud_peak_bytes'] <= 8 * 1024**3 and finite(cat['cpu_seconds']), 'metriques')
    workload(row)
    need(row['catalogue_ms'] == cat['wall_ns'] / 1e6 and row['cloud_ms'] == cloud['cloud_ns'] / 1e6 and
         row['read_ms'] == cloud['read_ns'] / 1e6 and
         row['catalogue_within_100ms'] is (cat['wall_ns'] < 100000000), 'temps_API')
    need(HEX.fullmatch(row['canonical_sha256']) and type(row['canonical_bytes']) is int and
         row['canonical_bytes'] > 34 and finite(row['semantic_wall_seconds']), 'empreinte_brute')
    value = row['semantic']
    need(set(value) == {'schema', 'sha256', 'coord_bits', 'kmax', 'sites', 'levels', 'balls', 'incidences'} and
         value['schema'] == 'ehgp.v11.catalogue_semantic.v1' and HEX.fullmatch(value['sha256']) and
         value['coord_bits'] == bits and value['kmax'] == kmax and value['sites'] == case['count'] and
         all(value[k] == cat[k] for k in ('levels', 'balls', 'incidences')), 'empreinte_semantique')


def judge_report(report, manifest, manifest_hash, qualification_hash, builds, provenance_hashes):
    need(report['schema'] == 'ehgp.v11.catalogue_profiles.v1' and
         report['attempt_schema'] == 'ehgp.v11.catalogue_attempt.v2' and report['manifest'] == manifest and
         report['manifest_sha256'] == manifest_hash and report['qualification_sha256'] == qualification_hash,
         'identite_rapport')
    need(report['requested_runs'] == 36 and report['repetitions_requested'] == 1 and report['leaf_size'] == 16 and
         report['timeout_seconds'] == 30 and report['native_schedule_bound_seconds'] == 1080, 'protocole')
    need(type(report['complete']) is bool and type(report['full_schedule_completed']) is bool, 'cloture_rapport')
    records = report['builds']
    need(len(records) == 3 and [r['coord_bits'] for r in records] == list(PROFILES), 'profils_inventaire')
    pins = {r['coord_bits']: r for r in records}
    for bits, name in PROFILES.items():
        record = pins[bits]
        source = builds[name]['mhgp11_catalogue_bench']
        need(record['configuration'] == name and record['sha256'] == source['sha256'] and
             record['bytes'] == source['size'] and record['provenance_sha256'] == provenance_hashes[name] and
             record['cache_sha256'] == builds[name]['CMakeCache.txt']['sha256'] and
             Path(record['path']).parts[-3:] == (name, 'build', 'mhgp11_catalogue_bench'), 'binaire_qualifie')
    cases = {c['name']: c for c in manifest['cases']}
    need(len(manifest['cases']) == 6 and {n: c['count'] for n, c in cases.items()} == old.CASE_COUNTS and
         all(c['profile'] == 'quantized_u18_input_only' and c['unit_site_weights'] is True and
             c['duplicate_sites'] == 0 for c in cases.values()), 'entrees_communes')
    expected = [(c['name'], bits, k, 0) for c in sorted(cases.values(), key=lambda c:
                (c['name'].startswith('lidar'), c['count'])) for bits in PROFILES for k in (5, 10)]
    runs, omissions = report['runs'], report['not_run']
    observed, omitted = [unit(r) for r in runs], [unit(r) for r in omissions]
    need(len(observed) == len(set(observed)) and len(omitted) == len(set(omitted)) and
         not set(observed) & set(omitted) and set(observed + omitted) <= set(expected), 'unites')
    cursor, justified = 0, set()
    for i, row in enumerate(runs):
        need(cursor < len(expected) and unit(row) == expected[cursor], 'ordre_tentatives')
        attempt(row, cases[row['case']], pins[row['coord_bits']], report['complete'], i == len(runs) - 1)
        cursor += 1
        if row['status'] not in ('ok', 'pending_semantic') and row['kmax'] == 5:
            justified.add(expected[cursor])
            cursor += 1
    need(set(omitted) <= justified and all(r['reason'] == 'same_profile_K5_failed' for r in omissions),
         'omissions_non_causales')
    schedule = len(runs) == 36 and all(r['status'] == 'ok' for r in runs)
    comparisons = compare(runs)
    need(report['comparisons'] == comparisons or (not runs and report['comparisons'] == []), 'comparaisons')
    conforming = schedule and all(c['status'] == 'equal' for c in comparisons)
    if report['complete']:
        need(cursor == 36 and set(omitted) == justified and set(observed + omitted) == set(expected), 'calendrier_incomplet')
        need(report['all_attempted_ok'] is all(r['status'] == 'ok' for r in runs) and
             report['full_schedule_completed'] is schedule and report['conforming'] is conforming, 'verdict')
    else:
        need(report['full_schedule_completed'] is False and report.get('conforming', False) is False, 'partiel_verdi')
        conforming = False
    counts = {bits: {'attempted': sum(r['coord_bits'] == bits for r in runs),
                     'ok': sum(r['coord_bits'] == bits and r['status'] == 'ok' for r in runs)} for bits in PROFILES}
    return dict(conforming=conforming, attempted=len(runs), unplayed=36 - len(runs), profiles=counts,
                complete=report['complete'], different=sum(c['status'] == 'different' for c in comparisons),
                equal=sum(c['status'] == 'equal' for c in comparisons))


def bench_group_closed(meta, has_report):
    if has_report or 'group_closed' in meta:
        need(meta.get('group_closed') == '1', 'fermeture_banc')


def check(folder):
    receipt, worker, data = old.read_capture(folder)
    need(all(receipt[k] is True for k in ('private_key_deleted', 'oslogin_key_removed', 'reserve_released')),
         'nettoyage_session')
    summary, counts, builds, matrix_code = old.matrix(data, folder)
    for name, records in builds.items():
        if 'CMakeCache.txt' in records:
            lines = [line.split('=', 1)[1] for line in records['CMakeCache.txt']['text'].splitlines()
                     if line.startswith('MHGP11_COORD_BITS:')]
            need(lines == [str(CONFIG_BITS[name])], 'profil_configuration')
    manifest, manifest_hash = old.inputs(folder, receipt)
    metas = [fields(data[p + 'meta.txt']) for p in ('results/cmd/000_matrice/', BENCH)]
    need(int(metas[0]['exit_code']) == matrix_code and metas[0]['group_closed'] == '1' and
         metas[0]['status'] == ('ok' if matrix_code == 0 else 'failed'), 'commande_matrice')
    result_path = BENCH + 'files/profiles.json'
    if result_path in data:
        source = data[result_path]
        need(source == (folder / 'profiles.json').read_bytes() and matrix_code == 0, 'copie_banc_qualifie')
        report = js(source)
        hashes = {name: sha(data[BASE + name + '/build_provenance.json']) for name in PROFILES.values()}
        verdict = judge_report(report, manifest, manifest_hash, sha(data[BASE + 'summary.json']), builds, hashes)
        need((metas[1]['status'] == 'ok') is verdict['conforming'] and
             (int(metas[1]['exit_code']) == 0) is verdict['conforming'], 'commande_banc_verdict')
        if report['complete']:
            need(int(metas[1]['exit_code']) == (0 if verdict['conforming'] else 1), 'commande_banc_code')
    else:
        need(metas[1]['status'] != 'ok' and int(metas[1]['exit_code']) != 0 and
             not (folder / 'profiles.json').exists(), 'banc_absent_verdi')
        verdict = dict(conforming=False, attempted=0, unplayed=36, profiles={}, complete=False, different=0, equal=0)
    bench_group_closed(metas[1], result_path in data)
    command_ok = sum(m['status'] == 'ok' for m in metas)
    need(worker['commands_total'] == '2' and int(worker['commands_ok']) == command_ok and
         worker['status'] == ('completed' if command_ok == 2 else 'failed') and
         receipt['status'] == ('completed' if command_ok == 2 else 'failed_remote') and
         receipt['worker_exit_code'] == (0 if command_ok == 2 else 1), 'verdict_session')
    print('%s coherence=ok campagne=%s commit=%s' % (folder.name, 'CONFORME' if command_ok == 2 else 'ECHEC', receipt['commit']))
    print('  essais=%d non_joues=%d comparaisons_egales=%d divergentes=%d' %
          (verdict['attempted'], verdict['unplayed'], verdict['equal'], verdict['different']))
    for bits, values in verdict['profiles'].items():
        print('  B%d essais=%d succes=%d' % (bits, values['attempted'], values['ok']))
    print('  portes=%d/%d' % (sum(c[1] for c in counts.values()), sum(c[0] for c in counts.values())))
    return command_ok != 2


def main():
    folders = [Path(p) for p in sys.argv[1:]] or sorted(p for p in HERE.glob('profiles[0-9]*') if p.is_dir())
    need(folders, 'aucune_capture_close')
    failed = sum(check(folder) for folder in folders)
    print('captures_coherentes=%d campagnes_echouees=%d' % (len(folders), failed))


if __name__ == '__main__':
    try:
        main()
    except (old.foundation.Refusal, OSError, ValueError, KeyError, TypeError, IndexError,
            old.foundation.ET.ParseError, tarfile.TarError) as error:
        print('REFUS ' + (str(error) if isinstance(error, old.foundation.Refusal) else type(error).__name__))
        sys.exit(1)
