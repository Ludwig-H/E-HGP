"""LIVE evidence reader for deferred q4 levels; no native execution or performance extrapolation.

Code0 means coherent evidence, including explicitly failed campaigns. Canonical payloads were removed remotely:
their recorded hashes are compared, not recomputed. Previous receipts and readers remain unchanged.
"""
import copy
import importlib.util
from pathlib import Path
import sys
import tarfile

HERE = Path(__file__).resolve().parent


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


profiles = load_module('q4_previous_profiles', HERE.parent / 'catalogue_profiles_20261002/check.py')
asan = load_module('q4_previous_asan', HERE.parent / 'catalogue_profiles_20261002/check_asan18.py')
old = profiles.old
need, js, sha, fields, BASE = old.need, old.js, old.sha, old.fields, old.BASE
SUPPLEMENT = 'results/cmd/001_asan18/files/matrix/'
BENCH = 'results/cmd/002_profiles/'
COMMANDS = ('000_matrice', '001_asan18', '002_profiles')
SCHEMA = 'ehgp.v11.catalogue_profiles.v2'
WORK_SCHEMA = 'ehgp.v11.catalogue_work.v1'
SOURCE_COMMIT = 'ffc2ff95f0ae7296bdc522df81df34c58c3fdf47'
BASELINE_SHA = 'bb9935eafb71680fac6fe80db5a6ea16d6b901b923bd46b302d28f5703dab197'
NUM_GATES = {'mhgp11_num_unit_power_paths': ('power_paths', 207, 205),
             'mhgp11_num_unit_candidate': ('candidate', 831, 800)}
FRACTION_GATES = {'mhgp11_num_fraction', 'mhgp11_num_fraction_opt'}


def q4_signature(row):
    work, counts = row['events'][1]['work'], row['qmin_counts']
    need(set(work) == {'q4_candidates', 'q4_levels'} and
         all(type(v) is int and 0 <= v < 2**64 for v in work.values()), 'compteurs_q4')
    need(set(counts) == {'2', '3', '4'} and all(type(v) is int and 0 <= v < 2**32 for v in counts.values()) and
         sum(counts.values()) == row['semantic']['balls'], 'comptes_qmin')
    need(work['q4_levels'] == counts['4'] <= work['q4_candidates'], 'niveaux_q4_par_passe')
    return work['q4_candidates'], work['q4_levels']


def q4_comparisons(rows):
    result = []
    for name in sorted(old.CASE_COUNTS):
        for kmax in (5, 10):
            matches = [r for r in rows if (r['case'], r['kmax'], r['status']) == (name, kmax, 'ok')]
            signatures = {q4_signature(row) for row in matches}
            result.append(dict(case=name, kmax=kmax, successful_profiles=[r['coord_bits'] for r in matches],
                               status='different' if len(signatures) > 1 else
                               'equal' if len(matches) == 3 else 'incomplete'))
    return result


def judge_report(report, manifest, manifest_hash, qualification_hash, builds, provenance_hashes, supplement_hash):
    need(report['schema'] == SCHEMA and report['work_schema'] == WORK_SCHEMA and
         report['supplement_sha256'] == supplement_hash, 'identite_q4_supplement')
    for row in report['runs']:
        if row['status'] == 'ok':
            q4_signature(row)
        elif row['status'] != 'artifact_error':
            need('qmin_counts' not in row, 'qmin_avant_decodage')
    comparisons = q4_comparisons(report['runs'])
    need(report['q4_comparisons'] == comparisons or (not report['runs'] and report['q4_comparisons'] == []),
         'comparaisons_q4')
    # Reuse the frozen v1 validator for all unchanged fields and causal omissions. Its conformance only concerns
    # the old semantic/work contract; q4 conformance is checked separately against the original v2 report below.
    legacy = copy.deepcopy(report)
    legacy['schema'] = 'ehgp.v11.catalogue_profiles.v1'
    if legacy['complete']:
        legacy['conforming'] = len(legacy['runs']) == 36 and all(r['status'] == 'ok' for r in legacy['runs']) and all(
            c['status'] == 'equal' for c in profiles.compare(legacy['runs']))
    result = profiles.judge_report(legacy, manifest, manifest_hash, qualification_hash, builds, provenance_hashes)
    conforming = result['conforming'] and all(c['status'] == 'equal' for c in comparisons)
    need(report.get('conforming', False) is conforming, 'verdict_q4')
    return dict(result, conforming=conforming, q4_different=sum(c['status'] == 'different' for c in comparisons))


def compare_baseline(report, baseline):
    need(report['manifest'] == baseline['manifest'] and report['manifest_sha256'] == baseline['manifest_sha256'],
         'entrees_baseline')
    previous = {profiles.unit(row): row for row in baseline['runs'] if row['status'] == 'ok'}
    need(len(previous) == 15, 'baseline_15_succes')
    current = {profiles.unit(row): row for row in report['runs'] if row['status'] == 'ok'}
    matched, missing = [], []
    for key, before in previous.items():
        if key not in current:
            missing.append(key)
            continue
        after = current[key]
        for field in ('canonical_sha256', 'canonical_bytes', 'semantic'):
            need(after[field] == before[field], 'regression_%s_%s' % (field, key))
        need(profiles.workload(after) == profiles.workload(before), 'regression_travail_%s' % (key,))
        for field in ('balls', 'levels', 'incidences', 'peak_reserved_bytes', 'reserved_after_bytes'):
            need(after['events'][1][field] == before['events'][1][field], 'regression_%s_%s' % (field, key))
        need(after['events'][0]['cloud_peak_bytes'] == before['events'][0]['cloud_peak_bytes'],
             'regression_pic_cloud_%s' % (key,))
        matched.append(key)
    return dict(matched=matched, missing=missing, complete=len(matched) == 15)


def judge_matrix(data):
    summary = js(data[BASE + 'summary.json'])
    configs = summary['configurations']
    names = [c['name'] for c in configs]
    need(summary['schema'] == 'ehgp.v11.g4_matrix_summary.v1' and summary['complete'] is True and
         len(names) == len(old.foundation.NAMES) and set(names) == old.foundation.NAMES and
         len(summary['requested']) == len(names) and set(summary['requested']) == set(names), 'inventaire_matrice')
    need({p[len(BASE):].split('/')[0] for p in data if p.startswith(BASE) and '/' in p[len(BASE):]} == set(names),
         'dossiers_matrice')
    statuses = {c['name']: c['status'] for c in configs}
    need(summary['statuses'] == statuses, 'statuts_matrice')
    counts, builds = {}, {}
    for config in configs:
        name = config['name']
        if 'tests' not in config and config['status'] in old.EARLY | {'build_failed'}:
            need(js(data[BASE + name + '/result.json']) == config and config['conforming'] is False,
                 'echec_precoce_matrice')
            counts[name] = (0, 0, 0, 0)
        else:
            counts[name] = old.foundation.judge_config(config, data)
        builds[name] = old.provenance(config, data)
        cache = builds[name].get('CMakeCache.txt')
        if cache:
            bits = [line.split('=', 1)[1] for line in cache['text'].splitlines()
                    if line.startswith('MHGP11_COORD_BITS:')]
            need(bits == [str(profiles.CONFIG_BITS[name])], 'profil_configuration')
    failed = [s for s in statuses.values() if s not in ('ok', 'absent')]
    code = 1 if summary.get('signals') or any(s not in ('vacuous', 'incomplete', 'floor_violated') for s in failed) else (
        3 if failed else 0)
    need(type(summary['exit_code']) is int and summary['exit_code'] == code and summary['conforming'] is (code == 0),
         'verdict_matrice')
    return counts, builds, code


def supplement_tests(config, data):
    if 'tests' not in config:
        need(config['status'] != 'ok', 'supplement_sans_tests')
        return 0
    root = old.foundation.ET.fromstring(data[asan.PREFIX + 'junit.xml'])
    cases = {case.get('name'): case for case in root.iter('testcase')}
    required = set(NUM_GATES) | FRACTION_GATES
    if config['status'] == 'ok':
        need(required <= set(cases), 'portes_num_q4_absentes')
    passed, fractions = 0, []
    for name in required & set(cases):
        case = cases[name]
        if case.get('status') != 'run' or case.find('failure') is not None or case.find('skipped') is not None:
            continue
        lines = (case.findtext('system-out') or '').splitlines()
        need('run_expect_verdict conforme' in lines, 'verdict_porte_num')
        if name in NUM_GATES:
            group, count, floor = NUM_GATES[name]
            need('test %s controles=%d echecs=0 plancher=%d' % (group, count, floor) in lines and
                 'mhgp11_test_ok tests=1 controles=%d' % count in lines, 'plancher_num_q4')
        else:
            values = [js(line) for line in lines if line.startswith('{')]
            need(len(values) == 1, 'sortie_Fraction_unique')
            value = values[0]
            need(all(type(value[key]) is int and value[key] == expected for key, expected in
                     dict(bits=18, checks=11838, geometry=528, degeneracies=50, integers=160).items()) and
                 old.HEX.fullmatch(value['input_sha256']), 'Fraction_q4_b18')
            fractions.append(value)
        passed += 1
    need(len(fractions) < 2 or fractions[0] == fractions[1], 'Fraction_normal_opt_different')
    if config['status'] == 'ok':
        need(passed == 4, 'portes_num_q4_non_passees')
    return passed


def judge_supplement(data):
    summary = js(data[BASE + 'summary.json'])
    configs = summary['configurations']
    need(summary['schema'] == 'ehgp.v11.g4_matrix_summary.v1' and summary['complete'] is True and
         summary['requested'] == [asan.NAME] and len(configs) == 1 and configs[0]['name'] == asan.NAME,
         'inventaire_supplement')
    config = configs[0]
    need(summary['statuses'] == {asan.NAME: config['status']} and
         {p[len(BASE):].split('/')[0] for p in data if p.startswith(BASE) and '/' in p[len(BASE):]} == {asan.NAME},
         'statut_dossier_supplement')
    if 'tests' not in config and config['status'] in old.EARLY | {'build_failed'}:
        need(js(data[asan.PREFIX + 'result.json']) == config and config['conforming'] is False, 'echec_supplement')
        counts = (0, 0, 0, 0)
    else:
        counts = old.foundation.judge_config(config, data)
    asan.provenance(config, data)
    passed = supplement_tests(config, data)
    state = config['status']
    code = 1 if summary.get('signals') or state not in ('ok', 'vacuous', 'incomplete', 'floor_violated') else (
        0 if state == 'ok' else 3)
    need(type(summary['exit_code']) is int and summary['exit_code'] == code and
         summary['conforming'] is (code == 0), 'verdict_supplement')
    return counts, passed, code


def command(meta, code=None):
    if code is not None:
        need(meta.get('group_closed') == '1' and int(meta['exit_code']) == code and
             meta['status'] == ('ok' if code == 0 else 'failed'), 'issue_commande')
    elif 'exit_code' in meta:
        need(meta.get('group_closed') == '1' and int(meta['exit_code']) != 0 and meta['status'] != 'ok',
             'commande_incomplete')
    else:
        need(meta['status'] in ('interrupted', 'skipped_deadline', 'skipped_build') and
             meta.get('group_closed', '1') == '1', 'commande_non_lancee')


def session(receipt, worker, metas):
    good = sum(m['status'] == 'ok' for m in metas)
    need(worker['commands_total'] == '3' and int(worker['commands_ok']) == good and
         worker['status'] == ('completed' if good == 3 else 'failed') and
         receipt['status'] == ('completed' if good == 3 else 'failed_remote') and
         receipt['worker_exit_code'] == (0 if good == 3 else 1), 'verdict_session')
    return good == 3


def check(folder, baseline):
    receipt, worker, data = old.read_capture(folder)
    need(receipt['commit'] == SOURCE_COMMIT, 'source_q4_qualifiee')
    need(all(receipt[k] is True for k in ('private_key_deleted', 'oslogin_key_removed', 'reserve_released')),
         'nettoyage_session')
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')} == set(COMMANDS), 'commandes_inventaire')
    metas = [fields(data['results/cmd/' + name + '/meta.txt']) for name in COMMANDS]
    manifest, manifest_hash = old.inputs(folder, receipt)
    matrix_hash = supplement_hash = None
    counts, builds, matrix_code, supplement_code = {}, {}, None, None
    if BASE + 'summary.json' in data:
        source = data[BASE + 'summary.json']
        need((folder / 'matrix.json').read_bytes() == source, 'copie_matrice')
        counts, builds, matrix_code = judge_matrix(data)
        matrix_hash = sha(source)
    else:
        need(not (folder / 'matrix.json').exists(), 'matrice_absente')
    command(metas[0], matrix_code)
    if SUPPLEMENT + 'summary.json' in data:
        source = data[SUPPLEMENT + 'summary.json']
        need((folder / 'asan18.json').read_bytes() == source, 'copie_supplement')
        mapped = {BASE + p[len(SUPPLEMENT):]: value for p, value in data.items() if p.startswith(SUPPLEMENT)}
        _, _, supplement_code = judge_supplement(mapped)
        supplement_hash = sha(source)
    else:
        need(not (folder / 'asan18.json').exists(), 'supplement_absent')
    command(metas[1], supplement_code)
    comparison = dict(matched=[], missing=list(range(15)), complete=False)
    result_path = BENCH + 'files/profiles.json'
    if result_path in data:
        need(matrix_code == supplement_code == 0, 'banc_sans_qualification')
        source = data[result_path]
        need((folder / 'profiles.json').read_bytes() == source, 'copie_banc')
        report = js(source)
        hashes = {name: sha(data[BASE + name + '/build_provenance.json']) for name in profiles.PROFILES.values()}
        verdict = judge_report(report, manifest, manifest_hash, matrix_hash, builds, hashes, supplement_hash)
        if report['complete']:
            command(metas[2], 0 if verdict['conforming'] else 1)
        else:
            command(metas[2])
        comparison = compare_baseline(report, baseline)
    else:
        need(not (folder / 'profiles.json').exists(), 'banc_absent')
        command(metas[2])
        verdict = dict(conforming=False, attempted=0, unplayed=36, different=0, q4_different=0)
    good = session(receipt, worker, metas)
    need(good is verdict['conforming'], 'qualification_et_banc')
    print('%s coherence=ok campagne=%s commit=%s' % (folder.name, 'CONFORME' if good else 'ECHEC', receipt['commit']))
    print('  essais=%d non_joues=%d divergences=%d q4_divergences=%d baseline=%d/15 non_compares=%d' %
          (verdict['attempted'], verdict['unplayed'], verdict['different'], verdict['q4_different'],
           len(comparison['matched']), len(comparison['missing'])))
    print('  matrice=%d/%d supplement_code=%s' %
          (sum(c[1] for c in counts.values()), sum(c[0] for c in counts.values()), supplement_code))
    return not good


def main():
    baseline_folder = HERE.parent / 'catalogue_profiles_20261002/profiles1'
    source = (baseline_folder / 'profiles.json').read_bytes()
    need(sha(source) == BASELINE_SHA, 'baseline_epinglee')
    profiles.check(baseline_folder)
    baseline = js(source)
    folders = [Path(p) for p in sys.argv[1:]] or sorted(p.parent for p in HERE.glob('*/receipt.json'))
    need(folders, 'aucune_capture_close')
    failed = sum(check(folder, baseline) for folder in folders)
    print('captures_coherentes=%d campagnes_echouees=%d' % (len(folders), failed))


if __name__ == '__main__':
    try:
        main()
    except (old.foundation.Refusal, OSError, ValueError, KeyError, TypeError, IndexError,
            old.foundation.ET.ParseError, tarfile.TarError) as error:
        print('REFUS ' + (str(error) if isinstance(error, old.foundation.Refusal) else type(error).__name__))
        sys.exit(1)
