"""LIVE bounded-MEB evidence reader; explicit transport/provenance port from index_20261002. Code0 means coherent evidence, including explicit failures.

Original local receipt and archive remain mandatory. Canonical outputs were deleted remotely: their published
hashes are compared, not recomputed. No native execution, MEB performance extrapolation or FULL qualification.
"""
import importlib.util
import math
from math import comb
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


previous = load_module('meb_previous_index', HERE.parent / 'index_20261002/check.py')
q4, old, profiles, asan = previous.q4, previous.old, previous.profiles, previous.asan
need, js, sha, fields, BASE, HEX = old.need, old.js, old.sha, old.fields, old.BASE, old.HEX
# Private imported instance: transport/provenance ported explicitly from the index receipt reader.
# Historical files remain unchanged. No import of the current benchmark validator.
asan.CACHE = dict(asan.CACHE, MHGP11_MODULES='num;index;tower')
EXTRA_TARGETS = {'mhgp11_tower_probe', 'mhgp11_meb_bench', 'mhgp11_tower_unit', 'mhgp11_tower_fault'}
asan.BINARIES |= EXTRA_TARGETS
asan.TARGETS |= EXTRA_TARGETS
asan.BUILD_FILES = {'CMakeCache.txt'} | {'CMakeFiles/%s.dir/%s' % (target, filename)
    for target in asan.TARGETS for filename in ('flags.make', 'link.txt')}
SUPPLEMENT = 'results/cmd/001_asan18/files/matrix/'
BENCH = 'results/cmd/002_meb/'
COMMANDS = ('000_matrice', '001_asan18', '002_meb')
SOURCE_COMMIT = None  # Set only to the actually captured pushed commit, after source freeze.
SCHEMA = 'ehgp.v11.meb_benchmark.v1'
LOGICAL = {'nodes', 'bounds', 'point_tests', 'inside_blocks', 'outside_blocks', 'passes'}
MEB_LOGICAL = {'presentations', 'nondegenerate', 'positive', 'containing', 'comparisons', 'point_tests'}
TIMES = ('meb_ns', 'census_ns', 'wrapper_ns', 'reference_ns')
# Counts below are derived from the frozen tests; actual G4 outputs must confirm them.
UNIT_GATES = {'mhgp11_tower_unit_' + group: (group, count, floor) for group, count, floor in
    [('geometry', 196, 175), ('local_support', 20, 20), ('refusals', 25, 24), ('ownership', 24, 22),
     ('wrapper', 35, 30), ('capacity', 18, 18), ('shell', 10, 10), ('concurrency', 11, 10)]}
UNIT_GATES['mhgp11_tower_fault_starvation'] = ('starvation', 33, 30)
FRACTION = ('mhgp11_tower_fraction', 'mhgp11_tower_judge')
BENCH_GATES = {name + suffix: line for name, line in {
    'mhgp11_tower_bench_collector': 'meb_collector_verdict conforme attempts11 corruptions27 provenance20 schedules6 native0',
    'mhgp11_tower_bench_io': 'meb_io_verdict conforme attempts8 queries144 refusals5'
}.items() for suffix in ('', '_opt')}
REQUIRED = set(UNIT_GATES) | set(BENCH_GATES) | {name + suffix for name in FRACTION for suffix in ('', '_opt')}
BUDGET = 8 * 1024**3
MUTANTS = {'tower': ['q3_non_strict_admis', 'egalite_remplace_canonique', 'dernier_site_non_inclus',
    'q4_prefixe_obtus_coupe', 'census_sur_point_ancre', 'support_positions_locales', 'doublon_partie_admis',
    'cardinal_douze_refuse', 'priorite_seuil_perdue', 'multiplicites_refusees']}


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
            if name.removesuffix('_opt') == FRACTION[0]:
                expected = dict(bits=bits, fixtures=31, requests=350, checks=14631, refusals=18, permutations=31)
                need(value['modes'] == {'0': 62, '1': 270} and all(type(v) is int for v in value['modes'].values())
                     and HEX.fullmatch(value['input_sha256']), 'Fraction_MEB_modes')
            else:
                expected = dict(checks=43800, corruptions=27, fixed_facts_per_profile=20, fixtures=31,
                                malformed=3, native=0, profiles=3, requests_per_profile=350)
            need(all(type(value[k]) is int and value[k] == v for k, v in expected.items()), 'Fraction_compteurs')
            values[name] = value
        passed += 1
    for name in FRACTION:
        if name in values and name + '_opt' in values:
            need(values[name] == values[name + '_opt'], 'Fraction_normal_opt')
    if config['status'] == 'ok':
        need(passed == len(REQUIRED), 'portes_non_passees')
    return passed


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
        lines = previous.full_test_output(data, BASE + config['name'] + '/', name, case.findtext('system-out') or '')
        expected = 'mutants_ok module=%s mutants=%d tues=%d dont_signal=0 dont_delai=0 dont_construction=0 plancher=%d'
        n = len(identities)
        need(expected % (module, n, n, n) in lines and 'run_expect_verdict conforme' in lines,
             'mutants_non_causaux')
        for identity in identities:
            need(sum(bool(re.fullmatch(re.escape(identity) + r'\s+TUE\s+(code|ligne)', line)) for line in lines) == 1,
                 'mutant_mort_par_juge_' + identity)


def judge_matrix(data):
    counts, builds, code = previous.judge_matrix(data)
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
        need(counts == (55, 55, 0, 0), 'inventaire_supplement55')
    passed = previous.gates(config, data, 18) + gates(config, data, 18)
    state = config['status']
    code = 1 if summary.get('signals') or state not in ('ok', 'vacuous', 'incomplete', 'floor_violated') else (
        0 if state == 'ok' else 3)
    need(type(summary['exit_code']) is int and summary['exit_code'] == code and
         summary['conforming'] is (code == 0), 'verdict_supplement')
    return counts, passed, code


def events(row):
    values, n, bits = row['events'], row['count'], row['coord_bits']
    need(n >= 12 and len(values) == 52 and [e['phase'] for e in values] ==
         ['cloud', 'index'] + ['query'] * 48 + ['summary', 'exit'], 'phases_MEB')
    cloud, index, total, end = values[0], values[1], values[-2], values[-1]
    need(all(type(cloud[k]) is int and cloud[k] == n for k in ('sites', 'points')), 'entree_entiere')
    for key in ('read_ns', 'cloud_ns', 'peak_reserved_bytes', 'reserved_after_bytes'):
        integer(cloud[key], 'cloud_' + key)
    need(cloud['reserved_after_bytes'] == 28*n + 8 <= cloud['peak_reserved_bytes'] <= BUDGET, 'memoire_cloud')
    need(index['status'] == end['status'] == 'ok' and index['reason'] == end['reason'] == 'none' and
         type(index['coord_bits']) is int and index['coord_bits'] == bits and
         type(index['leaf_size']) is int and index['leaf_size'] == 8, 'identite_index')
    for key in ('wall_ns', 'peak_reserved_bytes', 'reserved_after_bytes', 'nodes', 'max_depth', 'node_bytes'):
        integer(index[key], 'index_' + key)
    width = 1
    while n // width > 8:
        width *= 2
    nodes = 2*(width + (n % width if n // width == 8 else 0)) - 1
    depth = ((n + 7)//8 - 1).bit_length() + 1
    base = cloud['reserved_after_bytes'] + 40*nodes
    need(index['nodes'] == nodes and index['max_depth'] == depth and index['node_bytes'] == 40 and
         index['reserved_after_bytes'] == index['peak_reserved_bytes'] == base <= BUDGET, 'construction_exacte')
    expected = dict(queries=48, complete=0, saturated=0, reserved_after_bytes=base, **dict.fromkeys(TIMES, 0))
    sizes = [0]*4
    for i, event in enumerate(values[2:-2]):
        size, threshold = 1 + i % 12, (5, 10, 13)[i % 3]
        need(all(type(event[k]) is int and event[k] == v for k, v in
                 [('ordinal', i), ('size', size), ('threshold', threshold)]), 'identite_requete')
        need(event['status'] == 'ok' and event['reason'] == 'none' and event['reference_ok'] is True, 'MEB_scan_wrapper')
        q = integer(event['support_size'], 'support_size', 5)
        need(1 <= q <= size and (q == 1) is (size == 1), 'support_strict_local')
        sizes[q - 1] += 1
        for key in TIMES:
            expected[key] += integer(event[key], key)
        p, u = integer(event['interior'], 'interieur', n+1), integer(event['shell'], 'coquille', n+1)
        kind = event['kind']
        need(kind in ('complete', 'saturated') and p+u <= n and
             ((kind == 'complete' and p < threshold and u >= q) or
              (kind == 'saturated' and p == threshold and u == 0)), 'population')
        expected[kind] += 1
        for stage, copies in (('meb', 0), ('census', 1), ('wrapper', 2)):
            need(integer(event[stage+'_after_bytes'], stage+'_after_bytes') ==
                 integer(event[stage+'_peak_bytes'], stage+'_peak_bytes') == base + copies*4*(p+u) <= BUDGET,
                 'memoire_exacte_' + stage)
        m, c = event['meb_logical'], event['census_logical']
        for ledger, keys in ((m, MEB_LOGICAL), (c, LOGICAL)):
            need(set(ledger) == keys and all(type(v) is int and 0 <= v < 2**64 for v in ledger.values()), 'travail_entier')
        need(m['presentations'] == sum(comb(size, q) for q in range(1, min(size, 4)+1)) and
             1 <= m['containing'] <= m['positive'] <= m['nondegenerate'] <= m['presentations'] and
             m['positive'] >= size + comb(size, 2) and m['comparisons'] == m['containing']-1 and
             m['positive'] <= m['point_tests'] <= size*m['positive'], 'enumeration_MEB')
        need(c['passes'] == 2 and c['nodes'] > 0, 'census_deux_passes')
    need(all(type(total[k]) is int and total[k] == v for k, v in expected.items()) and
         total['support_sizes'] == sizes and all(type(v) is int for v in total['support_sizes']), 'resume')
    need(sizes[0] == 4 and expected['complete'] >= 4 and (n < 8000 or expected['saturated'] > 0), 'planchers_MEB')
    return dict(expected, support_sizes=sizes)


def workload(row):
    return (row['events'][1]['nodes'], row['events'][1]['max_depth'], tuple(
        (tuple(e['meb_logical'][k] for k in sorted(MEB_LOGICAL)),
         tuple(e['census_logical'][k] for k in sorted(LOGICAL))) for e in row['events'][2:-2]))


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
         all(type(value[k]) is int and value[k] == v for k, v in expected.items() if k != 'support_sizes') and
         value['support_sizes'] == expected['support_sizes'] and
         all(type(v) is int for v in value['support_sizes']) and HEX.fullmatch(value['sha256']) and HEX.fullmatch(value['raw_sha256']) and
         value['raw_sha256'] == row['canonical_sha256'] and type(value['bytes']) is int and
         value['bytes'] == row['canonical_bytes'] > 34 and finite(row['semantic_wall_seconds']), 'semantic')


def judge_report(report, manifest, manifest_hash, qualification_hash, builds, provenance_hashes,
                 supplement_hash, supplement_provenance_hash):
    need(report['schema'] == SCHEMA and report['manifest'] == manifest and report['manifest_sha256'] == manifest_hash and
         report['qualification_sha256'] == qualification_hash and report['supplement_sha256'] == supplement_hash and
         report['supplement_provenance_sha256'] == supplement_provenance_hash, 'identite_rapport')
    for key, value in dict(requested_runs=18, repetitions_requested=1, leaf_size=8, queries_per_run=48,
                           timeout_seconds=30, native_schedule_bound_seconds=540).items():
        need(type(report[key]) is int and report[key] == value, 'protocole_' + key)
    need(report['thresholds'] == [5, 10, 13] and all(type(v) is int for v in report['thresholds']) and
         all(type(report[k]) is bool for k in ('complete', 'conforming', 'full_schedule_completed')), 'protocole')
    records = report['builds']
    need(len(records) == 3 and [r['coord_bits'] for r in records] == list(profiles.PROFILES) and
         all(type(r['coord_bits']) is int for r in records), 'profils')
    pins = {r['coord_bits']: r for r in records}
    for bits, name in profiles.PROFILES.items():
        r, source = pins[bits], builds[name]['mhgp11_meb_bench']
        need(r['configuration'] == name and r['sha256'] == source['sha256'] and type(r['bytes']) is int and
             r['bytes'] == source['size'] and r['provenance_sha256'] == provenance_hashes[name] and
             Path(r['path']).parts[-3:] == (name, 'build', 'mhgp11_meb_bench'), 'binaire_qualifie')
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
    need(SOURCE_COMMIT is not None and receipt['commit'] == SOURCE_COMMIT, 'source_MEB_qualifiee')
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
    result_path = BENCH + 'files/meb.json'
    if result_path in data:
        need(codes == [0, 0], 'banc_sans_qualification')
        raw = data[result_path]
        need((folder / 'meb.json').read_bytes() == raw and metas[2].get('group_closed') == '1', 'copie_fermeture_banc_MEB')
        report = js(raw)
        provenance = {name: sha(data[BASE + name + '/build_provenance.json']) for name in profiles.PROFILES.values()}
        supplement_pin = sha(data[SUPPLEMENT + asan.NAME + '/build_provenance.json'])
        verdict = judge_report(report, manifest, manifest_hash, hashes[0], builds, provenance, hashes[1], supplement_pin)
        q4.command(metas[2], (0 if verdict['conforming'] else 1) if report['complete'] else None)
    else:
        need(not (folder / 'meb.json').exists(), 'banc_absent')
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
