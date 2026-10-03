"""LIVE compact index evidence reader. Code0 means coherent evidence, including explicit failures.

Original local receipt and archive remain mandatory. Canonical outputs were deleted remotely: their published
hashes are compared, not recomputed. No native execution, index performance extrapolation or FULL qualification.
"""
import importlib.util
import math
from pathlib import Path
import re
import sys
import tarfile

HERE = Path(__file__).resolve().parent


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


q4 = load_module('index_previous_q4', HERE.parent / 'catalogue_q4_20261002/check.py')
old, profiles = q4.old, q4.profiles
need, js, sha, fields, BASE, HEX = old.need, old.js, old.sha, old.fields, old.BASE, old.HEX
# Private imported instance; adapt its provenance contract, without editing any historical reader or receipt.
asan = load_module('index_supplement_provenance', HERE.parent / 'catalogue_profiles_20261002/check_asan18.py')
asan.CACHE = dict(asan.CACHE, MHGP11_MODULES='num;index')
asan.BINARIES |= {'mhgp11_num_bounds_probe', 'mhgp11_index_probe', 'mhgp11_index_bench',
                  'mhgp11_index_unit', 'mhgp11_index_fault'}
asan.TARGETS |= {'mhgp11_num_bounds_probe', 'mhgp11_index_probe', 'mhgp11_index_bench',
                 'mhgp11_index_unit', 'mhgp11_index_fault'}
asan.BUILD_FILES = {'CMakeCache.txt'} | {'CMakeFiles/%s.dir/%s' % (target, filename)
    for target in asan.TARGETS for filename in ('flags.make', 'link.txt')}
SUPPLEMENT = 'results/cmd/001_asan18/files/matrix/'
BENCH = 'results/cmd/002_index/'
COMMANDS = ('000_matrice', '001_asan18', '002_index')
SOURCE_COMMIT = 'e8520481d1745627e156723ad995ac5175a8163f'
SCHEMA = 'ehgp.v11.index_benchmark.v1'
LOGICAL = {'nodes', 'bounds', 'point_tests', 'inside_blocks', 'outside_blocks', 'passes'}
UNIT_GATES = dict(q4.NUM_GATES, mhgp11_num_unit_bounds=('bounds', 183, 180))
for group, count, floor in [('fixtures', 241, 190), ('structure', 468, 200), ('ownership', 40, 35),
                           ('budget', 35, 22), ('blocks', 16, 14), ('concurrency', 11, 10)]:
    UNIT_GATES['mhgp11_index_unit_' + group] = (group, count, floor)
UNIT_GATES['mhgp11_index_fault_starvation'] = ('starvation', 34, 20)
FRACTION = ('mhgp11_num_fraction', 'mhgp11_num_bounds_fraction', 'mhgp11_index_fraction')
BENCH_GATES = {name + suffix: line for name, line in {
    'mhgp11_index_bench_collector': 'index_collector_verdict conforme attempts11 corruptions17 provenance20 schedules6 native0',
    'mhgp11_index_bench_io': 'index_io_verdict conforme attempts8 queries192 refusals5'
}.items() for suffix in ('', '_opt')}
REQUIRED = set(UNIT_GATES) | set(BENCH_GATES) | {name + suffix for name in FRACTION for suffix in ('', '_opt')}
BUDGET = 8 * 1024**3
MUTANTS = {'index': ['contact_exterieur',
           'contact_interieur',
           'bloc_temoin_duplique',
           'coquille_limitee_a_k',
           'saturation_egalite_perdue',
           'compte_une_passe_omis',
           'allocation_noeuds_excedentaire',
           'nuage_consomme_sur_refus'],
 'num': ['bounds_anchor_inside_ignored',
         'bounds_linear_upper_inverted_u24',
         'bounds_wide_upper_omitted_u21',
         'bounds_zero_width_refused',
         'candidate_side_inverse_u24',
         'candidate_ancre_perdue_u21',
         'candidate_niveau_zero_u24',
         'presentation_q3_taguee_q4_u21',
         'power_natif_q4_signe_inverse_u24',
         'power_large_q3_termes_omis_u21',
         'side_natif_coquille_exterieure_u24',
         'retenue_addition_perdue',
         'emprunt_perdu',
         'produit_signe_perdu',
         'conversion_borne_large',
         'denominateur_nul_admis',
         'ordre_niveaux_inverse',
         'milieu_denominateur_un',
         'angle_droit_admis',
         'convexite_face_admise']}


def integer(value, label, upper=2**64):
    need(type(value) is int and 0 <= value < upper, label)
    return value


def finite(value):
    return type(value) in (int, float) and math.isfinite(value) and value >= 0


def gates(config, data, bits):
    if 'tests' not in config:
        need(config['status'] != 'ok', 'portes_absentes')
        return 0
    prefix = BASE + config['name'] + '/'
    root = old.foundation.ET.fromstring(data[prefix + 'junit.xml'])
    cases = {case.get('name'): case for case in root.iter('testcase')}
    if config['status'] == 'ok':
        need(REQUIRED <= set(cases), 'portes_num_index_absentes')
    passed, values = 0, {}
    for name in REQUIRED & set(cases):
        case = cases[name]
        if case.get('status') != 'run' or case.find('failure') is not None or case.find('skipped') is not None:
            continue
        lines = (case.findtext('system-out') or '').splitlines()
        need('run_expect_verdict conforme' in lines, 'verdict_porte')
        if name in BENCH_GATES:
            need(BENCH_GATES[name] in lines, 'porte_banc_' + name)
        elif name in UNIT_GATES:
            group, count, floor = UNIT_GATES[name]
            need('test %s controles=%d echecs=0 plancher=%d' % (group, count, floor) in lines and
                 'mhgp11_test_ok tests=1 controles=%d' % count in lines, 'controles_natifs_' + name)
        else:
            outputs = [js(line) for line in lines if line.startswith('{')]
            need(len(outputs) == 1, 'Fraction_unique')
            value = outputs[0]
            expected = dict(bits=bits)
            base_name = name.removesuffix('_opt')
            if base_name == FRACTION[0]:
                expected.update(checks=11838, geometry=528, degeneracies=50, integers=160)
            elif base_name == FRACTION[1]:
                expected.update(cases=391, checks=3400, valid=354, degenerate=34, refused=3,
                                contacts=87, inside=30, outside=137, loose=128, wide={18: 0, 21: 7, 24: 35}[bits])
            else:
                expected.update(requests=1010, checks=36020, permutations=47)
                need(value['kinds'] == dict(complete=302, saturated=702, refused=6) and
                     all(type(v) is int for v in value['kinds'].values()), 'Fraction_index_populations')
            need(all(type(value[k]) is int and value[k] == v for k, v in expected.items()) and
                 HEX.fullmatch(value['input_sha256']), 'Fraction_compteurs')
            values[name] = value
        passed += 1
    for name in FRACTION:
        if name in values and name + '_opt' in values:
            need(values[name] == values[name + '_opt'], 'Fraction_normal_opt')
    if config['status'] == 'ok':
        need(passed == len(REQUIRED), 'portes_non_passees')
    return passed


def full_test_output(data, prefix, name, junit):
    # CTest truncates successful JUnit output at 1024 bytes. LastTest keeps the full per-test section.
    source = data[prefix + 'LastTest.log'].decode('utf-8')
    headings = list(re.finditer(r'^\d+/\d+ Testing: ([^\n]+)\n', source, re.MULTILINE))
    matches = [i for i, heading in enumerate(headings) if heading.group(1) == name]
    need(len(matches) == 1, 'LastTest_identite_unique')
    i = matches[0]
    section = source[headings[i].end():headings[i + 1].start() if i + 1 < len(headings) else len(source)]
    need(len(re.findall(r'^\d+/\d+ Test: ' + re.escape(name) + '$', section, re.MULTILINE)) == 1 and
         section.count('\nTest Passed.\n') == 1 and '\nTest Failed.\n' not in section, 'LastTest_statut')
    markers = '\nOutput:\n----------------------------------------------------------\n'
    need(section.count(markers) == 1 and section.count('\n<end of output>\n') == 1, 'LastTest_delimiteurs')
    output = section.split(markers, 1)[1].split('\n<end of output>\n', 1)[0]
    marker = '...\nThe rest of the test output was removed since it exceeds the threshold of 1024 bytes.\n'
    if junit.endswith(marker):
        need(output.startswith(junit[:-len(marker)]), 'JUnit_LastTest_prefixe')
    else:
        need(output.strip() == junit.strip(), 'JUnit_LastTest_sortie')
    return output.splitlines()


def causal_mutants(config, data):
    if 'tests' not in config:
        need(config['status'] != 'ok', 'mutants_absents')
        return
    root = old.foundation.ET.fromstring(data[BASE + config['name'] + '/junit.xml'])
    cases = {c.get('name'): c for c in root.iter('testcase')}
    for module, identities in MUTANTS.items():
        name = 'mhgp11_mutants_' + module
        if name not in cases:
            need(config['status'] != 'ok', 'mutants_module_absent')
            continue
        case = cases[name]
        if case.get('status') != 'run' or case.find('failure') is not None or case.find('skipped') is not None:
            continue
        lines = full_test_output(data, BASE + config['name'] + '/', name, case.findtext('system-out') or '')
        expected = 'mutants_ok module=%s mutants=%d tues=%d dont_signal=0 dont_delai=0 dont_construction=0 plancher=%d'
        n = len(identities)
        need(expected % (module, n, n, n) in lines and 'run_expect_verdict conforme' in lines,
             'mutants_non_causaux')
        for identity in identities:
            need(sum(bool(re.fullmatch(re.escape(identity) + r'\s+TUE\s+(code|ligne)', line)) for line in lines) == 1,
                 'mutant_mort_par_juge_' + identity)


def judge_matrix(data):
    counts, builds, code = q4.judge_matrix(data)
    for config in js(data[BASE + 'summary.json'])['configurations']:
        name = config['name']
        if name == 'mutants':
            causal_mutants(config, data)
        elif name != 'style' and config['status'] != 'absent':
            gates(config, data, profiles.CONFIG_BITS[name])
    return counts, builds, code


def judge_supplement(data):
    summary = js(data[BASE + 'summary.json'])
    configs = summary['configurations']
    need(summary['schema'] == 'ehgp.v11.g4_matrix_summary.v1' and summary['complete'] is True and
         summary['requested'] == [asan.NAME] and len(configs) == 1 and configs[0]['name'] == asan.NAME,
         'inventaire_supplement')
    config = configs[0]
    need(summary['statuses'] == {asan.NAME: config['status']} and
         {p[len(BASE):].split('/')[0] for p in data if p.startswith(BASE) and '/' in p[len(BASE):]} == {asan.NAME},
         'dossiers_supplement')
    if 'tests' not in config and config['status'] in old.EARLY | {'build_failed'}:
        need(js(data[asan.PREFIX + 'result.json']) == config and config['conforming'] is False, 'echec_supplement')
        counts = (0, 0, 0, 0)
    else:
        counts = old.foundation.judge_config(config, data)
    asan.provenance(config, data)
    if config['status'] == 'ok':
        need(counts == (36, 36, 0, 0), 'inventaire_supplement36')
    passed = gates(config, data, 18)
    state = config['status']
    code = 1 if summary.get('signals') or state not in ('ok', 'vacuous', 'incomplete', 'floor_violated') else (
        0 if state == 'ok' else 3)
    need(type(summary['exit_code']) is int and summary['exit_code'] == code and
         summary['conforming'] is (code == 0), 'verdict_supplement')
    return counts, passed, code


def events(row):
    points, index, total, end = row['events'][0], row['events'][1], row['events'][-2], row['events'][-1]
    need([e['phase'] for e in row['events']] == ['cloud', 'index'] + ['query'] * 64 + ['summary', 'exit'], 'phases')
    n = row['count']
    need(type(points['sites']) is int and type(points['points']) is int and
         points['sites'] == points['points'] == n, 'entree_entiere')
    for key in ('read_ns', 'cloud_ns', 'peak_reserved_bytes'):
        integer(points[key], 'cloud_' + key)
    need(type(points['reserved_after_bytes']) is int and points['reserved_after_bytes'] == 28 * n + 8 <=
         points['peak_reserved_bytes'] <= BUDGET, 'budget_cloud')
    need(index['status'] == end['status'] == 'ok' and index['reason'] == end['reason'] == 'none' and
         type(index['coord_bits']) is int and index['coord_bits'] == row['coord_bits'] and
         type(index['leaf_size']) is int and index['leaf_size'] == 8, 'index_identite')
    for key in ('wall_ns', 'peak_reserved_bytes', 'reserved_after_bytes', 'nodes', 'max_depth'):
        integer(index[key], 'index_' + key)
    need(0 < index['reserved_after_bytes'] <= index['peak_reserved_bytes'] <= BUDGET and
         0 < index['nodes'] < 2 * n and 0 < index['max_depth'] <= (n - 1).bit_length() + 1, 'index_memoire')
    width = 1
    while n // width > 8:
        width *= 2
    nodes = 2 * (width + (n % width if n // width == 8 else 0)) - 1
    depth = ((n + 7) // 8 - 1).bit_length() + 1
    need(index['nodes'] == nodes and index['max_depth'] == depth and type(index['node_bytes']) is int and
         index['node_bytes'] == 40 and index['reserved_after_bytes'] == index['peak_reserved_bytes'] ==
         points['reserved_after_bytes'] + 40 * nodes, 'construction_exacte')
    arities, counts, times = [0] * 4, dict(complete=0, saturated=0, degenerate=0), dict(query_ns=0, reference_ns=0)
    for i, event in enumerate(row['events'][2:-2]):
        q, threshold = 1 + i % 4, (5, 10, 13)[i % 3]
        need(all(type(event[k]) is int and event[k] == v for k, v in
                 [('ordinal', i), ('arity', q), ('threshold', threshold)]), 'identite_requete')
        integer(event['factory_ns'], 'factory_ns')
        if event['status'] == 'degenerate':
            need(q in (3, 4), 'degenerescence_q1q2')
            counts['degenerate'] += 1
            continue
        need(event['status'] == 'ok' and event['reason'] == 'none' and event['reference_ok'] is True,
             'requete_scan')
        for key in ('wall_ns', 'reference_ns', 'peak_reserved_bytes', 'reserved_after_bytes', 'interior', 'shell'):
            integer(event[key], 'requete_' + key)
        need(index['reserved_after_bytes'] <= event['reserved_after_bytes'] <= event['peak_reserved_bytes'] <= BUDGET,
             'budget_requete')
        need(event['reserved_after_bytes'] == event['peak_reserved_bytes'] ==
             index['reserved_after_bytes'] + 4 * (event['interior'] + event['shell']), 'memoire_exacte_census')
        k = event['kind']
        need(k in ('complete', 'saturated') and event['interior'] + event['shell'] <= n and
             ((k == 'complete' and event['interior'] < threshold) or
              (k == 'saturated' and event['interior'] == threshold and event['shell'] == 0)), 'population')
        need(set(event['logical']) == LOGICAL and all(type(v) is int and 0 <= v < 2**64 for
             v in event['logical'].values()) and event['logical']['passes'] == 2 and
             event['logical']['nodes'] > 0, 'travail')
        arities[q - 1] += 1
        counts[k] += 1
        times['query_ns'] += event['wall_ns']
        times['reference_ns'] += event['reference_ns']
    expected = dict(queries=sum(arities), **counts, **times, reserved_after_bytes=index['reserved_after_bytes'])
    need(all(type(total[k]) is int and total[k] == v for k, v in expected.items()) and
         total['arities'] == arities and all(type(v) is int for v in total['arities']), 'resume_requetes')
    need(arities[:2] == [16, 16] and min(arities[2:]) > 0 and counts['complete'] >= 16 and counts['saturated'] > 0,
         'planchers_requetes')
    return expected


def workload(row):
    return (row['events'][1]['nodes'], row['events'][1]['max_depth'], tuple(
        ('degenerate',) if event['status'] == 'degenerate' else tuple(event['logical'][k] for k in sorted(LOGICAL))
        for event in row['events'][2:-2]))


def comparisons(rows):
    out = []
    for name in sorted(old.CASE_COUNTS):
        matches = [r for r in rows if r['case'] == name and r['status'] == 'ok']
        equal = len({r['semantic']['sha256'] for r in matches}) <= 1
        same_work = len({workload(r) for r in matches}) <= 1
        out.append(dict(case=name, successful_profiles=[r['coord_bits'] for r in matches], semantic_equal=equal,
                        geometric_work_equal=same_work, status='different' if not equal or not same_work else
                        'equal' if len(matches) == 3 else 'incomplete'))
    return out


def attempt(row, case, build, complete, last):
    bits, state, argv = row['coord_bits'], row['status'], row['argv']
    need(row['repetition'] == 0 and type(row['repetition']) is int and row['whole_input'] is True and
         type(row['count']) is int and row['count'] == case['count'] and row['timeout_seconds'] == 30 and
         len(argv) == 5 and argv[0] == build['path'] and Path(argv[1]).name == case['coordinates'] and
         Path(argv[2]).name == case['point_ids'] and Path(argv[3]).name == '%s_b%d.bin' % (case['name'], bits) and
         argv[4] == str(BUDGET), 'commande')
    artefacts = {'semantic', 'canonical_sha256', 'canonical_bytes'}
    if state == 'running':
        need(not complete and last and row['exit_code'] is None and row['stdout'] == row['stderr'] == '' and
             row['events'] == row['errors'] == [] and 'process_wall_seconds' not in row and
             not artefacts & set(row), 'checkpoint_running')
        return
    need(finite(row['process_wall_seconds']), 'temps_processus')
    if state == 'pending_semantic':
        need(not complete and last and row['exit_code'] == 0 and not row['errors'] and
             not artefacts & set(row), 'checkpoint_semantique')
        parsed = old.attempt_events(dict(row, status='ok'), True)
    else:
        parsed = old.attempt_events(row, True)
    need(parsed == row['events'], 'flux_evenements')
    if state != 'ok':
        need(state in ('pending_semantic', 'timeout', 'failed', 'refused', 'launch_error', 'invalid_output',
                       'artifact_error') and (state == 'artifact_error' or not artefacts & set(row)), 'echec_artefact')
        if 'canonical_sha256' in row:
            need(HEX.fullmatch(row['canonical_sha256']), 'hash_partiel')
        if 'canonical_bytes' in row:
            integer(row['canonical_bytes'], 'taille_partielle')
        return
    need(type(row['exit_code']) is int and row['exit_code'] == 0 and row['stderr'] == '', 'faux_succes')
    expected = events(row)
    value = row['semantic']
    need(set(value) == {'sha256', 'raw_sha256', 'bytes'} | set(expected) and
         all(type(value[k]) is int and value[k] == v for k, v in expected.items()) and
         HEX.fullmatch(value['sha256']) and HEX.fullmatch(value['raw_sha256']) and
         value['raw_sha256'] == row['canonical_sha256'] and type(value['bytes']) is int and
         value['bytes'] == row['canonical_bytes'] > 34 and finite(row['semantic_wall_seconds']), 'semantic')


def judge_report(report, manifest, manifest_hash, qualification_hash, builds, provenance_hashes,
                 supplement_hash, supplement_provenance_hash):
    need(report['schema'] == SCHEMA and report['manifest'] == manifest and report['manifest_sha256'] == manifest_hash and
         report['qualification_sha256'] == qualification_hash and report['supplement_sha256'] == supplement_hash and
         report['supplement_provenance_sha256'] == supplement_provenance_hash, 'identite_rapport')
    for key, value in dict(requested_runs=18, repetitions_requested=1, leaf_size=8, queries_per_run=64,
                           timeout_seconds=30, native_schedule_bound_seconds=540).items():
        need(type(report[key]) is int and report[key] == value, 'protocole_' + key)
    need(report['thresholds'] == [5, 10, 13] and all(type(v) is int for v in report['thresholds']) and
         all(type(report[k]) is bool for k in ('complete', 'conforming', 'full_schedule_completed')), 'protocole')
    records = report['builds']
    need(len(records) == 3 and [r['coord_bits'] for r in records] == list(profiles.PROFILES) and
         all(type(r['coord_bits']) is int for r in records), 'profils')
    pins = {r['coord_bits']: r for r in records}
    for bits, name in profiles.PROFILES.items():
        r, source = pins[bits], builds[name]['mhgp11_index_bench']
        need(r['configuration'] == name and r['sha256'] == source['sha256'] and type(r['bytes']) is int and
             r['bytes'] == source['size'] and r['provenance_sha256'] == provenance_hashes[name] and
             Path(r['path']).parts[-3:] == (name, 'build', 'mhgp11_index_bench'), 'binaire_qualifie')
        lines = builds[name]['CMakeCache.txt']['text'].splitlines()
        for key, expected in dict(MHGP11_COORD_BITS=str(bits), MHGP11_SANITIZE='OFF', MHGP11_TSAN='OFF',
                                  MHGP11_POISON='OFF').items():
            need([line.split('=', 1)[1] for line in lines if line.startswith(key + ':')] == [expected], 'cache_banc')
    cases = {c['name']: c for c in manifest['cases']}
    need(len(manifest['cases']) == 6 and {n: c['count'] for n, c in cases.items()} == old.CASE_COUNTS and
         all(c['profile'] == 'quantized_u18_input_only' and c['unit_site_weights'] is True and
             c['duplicate_sites'] == 0 for c in cases.values()), 'entrees_communes')
    expected = [(c['name'], bits, 0) for c in sorted(cases.values(), key=lambda c:
                (c['name'].startswith('lidar'), c['count'])) for bits in profiles.PROFILES]
    unit = lambda r: (r['case'], r['coord_bits'], r['repetition'])
    runs, omitted = report['runs'], report['not_run']
    need([unit(r) for r in runs] == expected[:len(runs)] and len(runs) <= 18 and
         [unit(r) for r in omitted] == expected[len(runs):] and all(r['reason'] == 'pending' for r in omitted),
         'calendrier_prefixe')
    for i, row in enumerate(runs):
        need(type(row['coord_bits']) is int, 'profil_entier')
        attempt(row, cases[row['case']], pins[row['coord_bits']], report['complete'], i == len(runs) - 1)
    compared = comparisons(runs)
    need(report['comparisons'] == compared or (not runs and report['comparisons'] == []), 'comparaisons')
    schedule = len(runs) == 18 and all(r['status'] == 'ok' for r in runs)
    conforming = schedule and all(c['status'] == 'equal' for c in compared)
    if report['complete']:
        need(not omitted and len(runs) == 18 and report['full_schedule_completed'] is schedule and
             report['conforming'] is conforming, 'verdict_complet')
    else:
        need(report['full_schedule_completed'] is False and report['conforming'] is False, 'verdict_partiel')
        conforming = False
    return dict(conforming=conforming, attempted=len(runs), unplayed=18 - len(runs),
                ok=sum(r['status'] == 'ok' for r in runs), different=sum(c['status'] == 'different' for c in compared))


def check(folder):
    receipt, worker, data = old.read_capture(folder)
    need(SOURCE_COMMIT is not None and receipt['commit'] == SOURCE_COMMIT, 'source_index_qualifiee')
    need(all(receipt[k] is True for k in ('private_key_deleted', 'oslogin_key_removed', 'reserve_released')),
         'nettoyage_session')
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')} == set(COMMANDS), 'commandes')
    metas = [fields(data['results/cmd/' + name + '/meta.txt']) for name in COMMANDS]
    manifest, manifest_hash = old.inputs(folder, receipt)
    counts, builds, codes, hashes = {}, {}, [None, None], [None, None]
    for i, (prefix, filename) in enumerate(((BASE, 'matrix.json'), (SUPPLEMENT, 'asan18.json'))):
        if prefix + 'summary.json' in data:
            raw = data[prefix + 'summary.json']
            need((folder / filename).read_bytes() == raw, 'copie_' + filename)
            if i == 0:
                counts, builds, codes[i] = judge_matrix(data)
            else:
                mapped = {BASE + p[len(prefix):]: v for p, v in data.items() if p.startswith(prefix)}
                _, _, codes[i] = judge_supplement(mapped)
            hashes[i] = sha(raw)
        else:
            need(not (folder / filename).exists(), 'resume_absent')
        q4.command(metas[i], codes[i])
    result_path = BENCH + 'files/index.json'
    if result_path in data:
        need(codes == [0, 0], 'banc_sans_qualification')
        raw = data[result_path]
        need((folder / 'index.json').read_bytes() == raw and metas[2].get('group_closed') == '1', 'copie_fermeture_banc')
        report = js(raw)
        provenance = {name: sha(data[BASE + name + '/build_provenance.json']) for name in profiles.PROFILES.values()}
        supplement_pin = sha(data[SUPPLEMENT + asan.NAME + '/build_provenance.json'])
        verdict = judge_report(report, manifest, manifest_hash, hashes[0], builds, provenance, hashes[1], supplement_pin)
        q4.command(metas[2], (0 if verdict['conforming'] else 1) if report['complete'] else None)
    else:
        need(not (folder / 'index.json').exists(), 'banc_absent')
        q4.command(metas[2])
        verdict = dict(conforming=False, attempted=0, ok=0, unplayed=18, different=0)
    good = q4.session(receipt, worker, metas)
    need(good is verdict['conforming'], 'verdict_session_banc')
    print('%s coherence=ok campagne=%s commit=%s' % (folder.name, 'CONFORME' if good else 'ECHEC', receipt['commit']))
    print('  essais=%d succes=%d non_joues=%d divergences=%d portes_matrice=%d/%d' %
          (verdict['attempted'], verdict['ok'], verdict['unplayed'], verdict['different'],
           sum(c[1] for c in counts.values()), sum(c[0] for c in counts.values())))
    return not good


def main():
    folders = [Path(p) for p in sys.argv[1:]] or sorted(p.parent for p in HERE.glob('*/receipt.json'))
    need(folders, 'aucune_capture_close')
    failed = sum(check(folder) for folder in folders)
    print('captures_coherentes=%d campagnes_echouees=%d' % (len(folders), failed))


if __name__ == '__main__':
    try:
        main()
    except (old.foundation.Refusal, asan.old.foundation.Refusal, OSError, ValueError, KeyError, TypeError, IndexError,
            old.foundation.ET.ParseError, tarfile.TarError) as error:
        print('REFUS ' + (str(error) if isinstance(error, old.foundation.Refusal) else type(error).__name__))
        sys.exit(1)
