#!/usr/bin/env python3
"""Contre-lecture locale des preuves L/mesure et du reçu Git publié, sans lancer le produit."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import tarfile

HERE = Path(__file__).resolve().parent
RESULT = re.compile(r'^\s*\d+/\d+ Test\s+#\d+:\s+(\S+)\s+\.+\s+'
                    r'(Passed|\*\*\*[^\n]+?)\s+(\d+(?:\.\d+)?) sec\s*$', re.M)
STAGES = ('cloud', 'index', 'domain', 'tree', 'attach', 'output', 'write', 'total')


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read_member(tar, name):
    with tar.extractfile(name) as handle:
        return handle.read()


def closed(entry):
    base = Path(entry['local_session_path'])
    raw = (base / 'receipt.json').read_bytes()
    require(sha(raw) == entry['receipt_sha256'], 'receipt SHA ' + entry['session'])
    receipt = json.loads(raw)
    require(all(receipt.get(k) == v for k, v in entry['receipt_fields'].items()), 'safe receipt fields')
    require((base / 'DONE').read_text().strip() == entry['done'], 'DONE status')
    require(receipt['closure'] == 'stopped' and receipt['targeted_shutdown_certified']
            and receipt['start_certified'] and receipt['observed_after']['status'] == 'TERMINATED'
            and receipt['target']['instance'] == receipt['observed_after']['name']
            and receipt['generation'] == receipt['closing_generation']
            == receipt['observed_after']['lastStartTimestamp'], 'targeted closure')
    require(receipt['private_key_deleted'] and receipt['reserve_released'] and receipt['oslogin_key_removed'],
            'cleanup')
    require(receipt['results_verified'] and not receipt['results_skipped_members']
            and not receipt['overflow']['evicted'] and not receipt['overflow']['truncated_streams'],
            'archive completeness')
    archive = base / 'results/results.tar.gz'
    require(sha(archive.read_bytes()) == receipt['results_sha256'], 'archive SHA')
    require(sha((base / 'package/package.tar.gz').read_bytes()) == receipt['package_sha256'], 'package SHA')
    require(sha((base / 'package/plan.json').read_bytes()) == receipt['plan_sha256'], 'plan SHA')
    require((base / 'package/plan.json').read_text() == entry['exact_plan'], 'exact consumed plan')
    return receipt, archive


def measure_summary(data):
    configuration = data['configuration']
    require(configuration['frames'] == ['ng00', 'ng01', 'ng02'] and configuration['k'] == 5
            and configuration['workers'] == [1, 48] and configuration['prises'] == 3
            and configuration['k10'] == 'ng00', 'measurement configuration')
    require(data['engine_masks'] == {'full': 16379, 'supports': 7035}, 'engine masks')
    calls = data['calls']
    expected = set()
    for frame in configuration['frames']:
        for workers in (1, 48):
            for output in ('full', 'supports'):
                expected.add((frame, 5, workers, output, 'froid', 0))
                for take in (1, 2, 3):
                    expected.add((frame, 5, workers, output, 'chaud', take))
    for output in ('full', 'supports'):
        expected.add(('ng00', 10, 48, output, 'froid', 0))
        expected.add(('ng00', 10, 48, output, 'chaud', 1))
    actual = [(c['frame'], c['k'], c['workers'], c['output'], c['phase'], c['prise']) for c in calls]
    require(len(actual) == len(set(actual)) == 52 and set(actual) == expected, 'complete unique 52-call coverage')
    require(all(c['ok'] is True for c in calls) and not data['defects'], 'all calls ok')
    files, trees = {}, {}
    sites = {'ng00': 39885, 'ng01': 35551, 'ng02': 45845}
    for call in calls:
        require(call['sites'] == sites[call['frame']], 'whole declared nonground frame')
        require(set(call['stages_ns']) == set(STAGES)
                and all(type(v) is int and v >= 0 for v in call['stages_ns'].values()), 'all measured stages')
        for field in ('file_sha256', 'manifest_sha256', 'tree_k_sha256'):
            require(re.fullmatch('[0-9a-f]{64}', call[field]) is not None, 'hash shape')
        files.setdefault((call['frame'], call['k'], call['output']), set()).add(
            (call['file_sha256'], call['manifest_sha256'], call['file_bytes']))
        trees.setdefault((call['frame'], call['k']), set()).add(call['tree_k_sha256'])
    require(all(len(group) == 1 for group in files.values()), 'file/manifest identity between W and takes')
    require(all(len(group) == 1 for group in trees.values()), 'FULL/supports tree identity')
    held = []
    basis = []
    for frame in configuration['frames']:
        values = {output: sorted(c['stages_ns']['tree'] for c in calls
                                 if c['frame'] == frame and c['k'] == 5 and c['workers'] == 48
                                 and c['phase'] == 'chaud' and c['output'] == output)
                  for output in ('full', 'supports')}
        require(all(len(v) == 3 for v in values.values()), 'rule coverage')
        within = 10 * values['supports'][1] <= 11 * values['full'][1]
        if within:
            held.append(frame)
        basis.append({'frame': frame, 'stage': 'tree', 'workers': 48, 'k': 5, 'within_rule': within})
    decision = 'build_order_par_defaut' if len(held) >= 2 else 'livrer_L2b'
    require(data['decision']['decision'] == decision and data['decision']['frames_within_rule'] == held,
            'published decision matches predeclared tree rule')
    return {'calls': 52, 'k5_calls': 48, 'k10_descriptive_calls': 4, 'all_ok': True,
            'unique_complete_coverage': True, 'files_manifests_identical_between_workers_and_takes': True,
            'tree_identity_full_supports': True, 'all_stages_present': list(STAGES),
            'configuration': configuration, 'engine_masks': data['engine_masks'],
            'declared_cli_sha256': data['provenance']['cli_sha256'],
            'declared_commit': data['provenance']['commit'], 'decision': decision,
            'frames_within_rule': held, 'rule_basis': basis}


def derive(repo):
    manifest = json.loads((HERE / 'manifest.json').read_bytes())
    result = {'schema': 'ehgp.v11.audit.g4_long_measure_summary.v1',
              'source_commit': manifest['source_commit'], 'native_runs': 0, 'gcp_calls': 0,
              'full_sanitizer_qualification': False}
    for entry in manifest['sessions']:
        receipt, archive = closed(entry)
        require(receipt['commit'] == manifest['source_commit'], 'executed source pin')
        with tarfile.open(archive) as tar:
            if entry['kind'] == 'long':
                raw = read_member(tar, entry['summary_member'])
                require(sha(raw) == entry['summary_sha256'], 'long summary SHA')
                summary = json.loads(raw)
                config = summary['configurations'][0]
                prefix = 'results/cmd/000_matrice/files/matrix/release_long/'
                inventory = json.loads(read_member(tar, prefix + 'tests.json'))
                outcomes = {name: (state, seconds) for name, state, seconds in
                            RESULT.findall(read_member(tar, prefix + 'ctest.log').decode())}
                passed = sorted(name for name, (state, _) in outcomes.items() if state == 'Passed')
                missing = sorted(x['test'] for x in config['not_run'])
                require(len(passed) == config['tests']['passed'] == 34 and len(inventory) == 38,
                        'long Passed inventory count')
                require(set(missing) == {x['name'] for x in inventory} - set(outcomes), 'long missing exact set')
                require(missing == sorted(['mhgp11_mutants_' + module
                                          for module in ('tower', 'supports', 'api', 'cli')]), 'long remaining mutants')
                require(not config['failures'], 'long individual failures')
                important = ('mhgp11_cli_full_identity_scale32000_k10', 'mhgp11_cli_full_identity_lidar_ng00_k10')
                require(all(outcomes.get(name, ('',))[0] == 'Passed' for name in important), 'K10 identity passes')
                result['long'] = {'session': entry['session'], 'targeted_closure_verified': True,
                                  'matrix_complete': summary['complete'], 'matrix_conforming': summary['conforming'],
                                  'receipt_status': receipt['status'], 'worker_exit_code': receipt['worker_exit_code'],
                                  'tests': config['tests'], 'ctest_args': config['ctest_args'],
                                  'passed_tests': passed, 'missing_tests': missing,
                                  'non_mutant_passed_count': sum(not n.startswith('mhgp11_mutants_') for n in passed),
                                  'k10_identity_passed': list(important)}
            else:
                raw = read_member(tar, entry['summary_member'])
                require(sha(raw) == entry['summary_sha256'], 'paired measure SHA')
                data = json.loads(raw)
                require(manifest['source_commit'].startswith(data['provenance']['commit']), 'measure declared source')
                result['measure'] = measure_summary(data)
                result['measure'].update(session=entry['session'], targeted_closure_verified=True,
                                         receipt_status=receipt['status'], worker_exit_code=receipt['worker_exit_code'])
                result['supports_w48'] = []
                for item in entry['w48']:
                    text = read_member(tar, item['member']).decode()
                    require(sha(text.encode()) == item['sha256'] and text == item['exact_stdout'], 'W48 exact stdout')
                    require('appels=14\ncli_supports_scale_ok controles=52\n' in text
                            and text.startswith('cli_supports_scale_verdict conforme '), 'W48 native gate verdict')
                    result['supports_w48'].append({'frame': item['frame'], 'workers': [1, 4, 48],
                                                  'calls': 14, 'controls': 52, 'verdict': 'conforme'})
    published = manifest['published']
    def show(name):
        return subprocess.check_output(['git', '-C', str(repo), 'show', published['commit'] + ':'
                                        + published['base'] + str(PurePosixPath(name))])
    sums = show('SHA256SUMS')
    require(sha(sums) == published['sha256sums_sha256'], 'published SHA manifest')
    count = 0
    for line in sums.decode().splitlines():
        expected, name = line.split('  ', 1)
        require(sha(show(name)) == expected, 'published file SHA ' + name)
        count += 1
    for entry in manifest['sessions']:
        short = entry['session'].split('.')[-1]
        original = Path(entry['local_session_path'])
        require(show(short + '/receipt.json') == (original / 'receipt.json').read_bytes(), 'exact published receipt')
        with tarfile.open(original / 'results/results.tar.gz') as tar:
            require(show(short + '/' + entry['published_summary_name']) == read_member(tar, entry['summary_member']),
                    'exact published summary')
    a = json.loads(show('claudequalA/receipt.json'))
    a_raw = Path(published['a_original_receipt_path']).read_bytes()
    require(show('claudequalA/receipt.json') == a_raw, 'A published receipt exactly original')
    require(a['commit'].startswith('00bd979ac'), 'A actual source')
    readme = show('README.md').decode()
    row = next(line for line in readme.splitlines() if '| `claudequalA` ' in line)
    require('b319efc84' in row, 'captured README A source mismatch')
    b = json.loads(show('claudequalb/matrix_summary.json'))
    b_missing = {c['name']: c['tests']['not_run'] for c in b['configurations']}
    require(b_missing == {'gcc_asan_ubsan': 50, 'gcc_tsan': 65} and not b['conforming'], 'B remains partial')
    result['published'] = {'commit': published['commit'], 'sha_entries_verified': count,
                           'L_measure_receipts_summaries_exact_archive_copies': True,
                           'A_source_in_receipt': a['commit'], 'A_source_in_README': 'b319efc84',
                           'A_receipt_exact_original': True, 'B_missing': b_missing}
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path('/workspaces/E-HGP'))
    args = parser.parse_args()
    for line in (HERE / 'SHA256SUMS').read_text().splitlines():
        digest, filename = line.split('  ', 1)
        require(sha((HERE / filename).read_bytes()) == digest, 'capsule SHA ' + filename)
    result = derive(args.repo)
    require(result == json.loads((HERE / 'summary.json').read_bytes()), 'derived summary differs')
    print('long: Passed34/38 non_mutants27 K10_identity2 archive_sha=ok targeted_closure=ok')
    print('measure: calls52 complete identity=ok decision=livrer_L2b W48_ng00_ng02=conforme')
    print('published: SHA41/41 A_source_README_mismatch B_missing_ASan50_TSan65')
    print('audit_long_measure_verdict conforme sourceb319 hors_produit_natif hors_cloud')


if __name__ == '__main__':
    main()
