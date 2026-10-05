#!/usr/bin/env python3
"""Relecture de la session S close et projection du nouveau plan ; aucun natif/cloud."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tarfile

HERE = Path(__file__).resolve().parent
REPO = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path('/workspaces/E-HGP')
SESSION = Path(sys.argv[2]).resolve() if len(sys.argv) > 2 else Path(
    '/workspaces/.ehgp-sessions/v11.20261005.claudequals')
PIN = 'd26328fe2216133ba40385a9673325bbdcf03ea7'
NEXT_PIN = '8b2ca400ea215ffbf6821fe2a1a73b40d8095b2f'
ARCHIVE_SHA = '40327f6cc990e8b4231cf55cc15903cc1dd207b813e10d04b539bda3fcd64650'
PREFIX = 'results/cmd/000_matrice/files/matrix/'
VERDICT = re.compile(r'^\s*\d+/\d+ Test\s+#\d+:\s+(\S+)\s+\.+\s*'
                     r'(Passed|\*\*\*[^\n]+?)\s+(\d+(?:\.\d+)?) sec\s*$', re.M)


def need(ok, detail):
    if not ok:
        raise RuntimeError(detail)


def digest(value):
    return hashlib.sha256(value).hexdigest()


def git_source(pin, path):
    return subprocess.check_output(['git', '-C', str(REPO), 'show', pin + ':' + path])


def member(archive, path):
    return archive.extractfile(path).read()


def selected(test, args):
    for option, negate, labels in [('-R', False, False), ('-E', True, False),
                                  ('-L', False, True), ('-LE', True, True)]:
        if option in args:
            pattern = args[args.index(option) + 1]
            matches = any(re.search(pattern, value) for value in
                          (test['labels'] if labels else [test['name']]))
            if matches == negate:
                return False
    return True


def derive():
    receipt_bytes = (SESSION / 'receipt.json').read_bytes()
    receipt = json.loads(receipt_bytes)
    need((SESSION / 'DONE').read_text().strip() == '3', 'session failed_remote close')
    need(receipt['commit'] == PIN and receipt['source_kind'] == 'commit' and
         receipt['evidence_grade'] == 'pushed_commit' and receipt['worker_source'] == 'commit:' + PIN,
         'chaine source')
    archive_bytes = (SESSION / 'results/results.tar.gz').read_bytes()
    need(digest(archive_bytes) == ARCHIVE_SHA == receipt['results_sha256'], 'archive SHA')
    package_bytes = (SESSION / 'package/package.tar.gz').read_bytes()
    need(digest(package_bytes) == receipt['package_sha256'], 'paquet source SHA')
    plan_bytes = (SESSION / 'package/plan.json').read_bytes()
    need(digest(plan_bytes) == receipt['plan_sha256'], 'plan SHA')
    closed = (receipt['closure'] == 'stopped' and receipt['targeted_shutdown_certified'] and
              receipt['observed_after']['status'] == 'TERMINATED' and
              receipt['observed_after']['name'] == receipt['target']['instance'] and
              receipt['generation'] == receipt['closing_generation'] ==
              receipt['observed_after']['lastStartTimestamp'] and receipt['start_certified'] and
              receipt['private_key_deleted'] and receipt['reserve_released'] and
              receipt['oslogin_key_removed'] and receipt['guest_guard_intact'])
    need(closed, 'fermeture ciblee generation exacte')
    need(receipt['results_verified'] and not receipt['results_skipped_members'] and
         not receipt['overflow']['evicted'] and not receipt['overflow']['truncated_streams'],
         'resultats verifies/complets')
    source_paths = ['gcp-migration/v11_worker.sh', 'morsehgp3D_v11/tools/g4_matrix.json',
        'morsehgp3D_v11/tools/g4_matrix.py', 'morsehgp3D_v11/tests/num/tests.cmake',
        'morsehgp3D_v11/tests/points/tests.cmake', 'morsehgp3D_v11/src/points/point_tree.cpp',
        'morsehgp3D_v11/bench/roots_cost.cpp']
    source_sha = {}
    with tarfile.open(SESSION / 'package/package.tar.gz', 'r:gz') as package:
        for path in source_paths:
            content = member(package, path)
            need(content == git_source(PIN, path), 'paquet/source Git : ' + path)
            source_sha[path] = digest(content)
    answer = dict(schema='ehgp.v11.audit.g4_s_summary.v1',
        session='v11.20261005.claudequals', source_commit=PIN,
        receipt_sha256=digest(receipt_bytes), archive_sha256=ARCHIVE_SHA,
        archive_bytes=len(archive_bytes), package_sha256=receipt['package_sha256'],
        source_files_sha256=source_sha, plan_sha256=receipt['plan_sha256'],
        session_plan=json.loads(plan_bytes), receipt_status=receipt['status'],
        worker_exit_code=receipt['worker_exit_code'], targeted_closure_verified=closed,
        results_verified=True, matrix_configurations=[], qualification_complete=False,
        native_runs_by_auditor=0, cloud_calls_by_auditor=0)
    data_names = sorted(item['name'] for item in receipt['data_files'])
    answer['delivered_data_names'] = data_names
    answer['missing_uniform_input_names'] = [f'uniform_u18_n{size}{suffix}'
        for size in (8000, 16000, 32000) for suffix in ('.u32le', '.ids.u32le')]
    need(not set(answer['missing_uniform_input_names']) & set(data_names), 'uniformes livres')
    tests_by_family = {'gcc_asan': {}, 'gcc_tsan': {}}
    with tarfile.open(SESSION / 'results/results.tar.gz', 'r:gz') as archive:
        summary_bytes = member(archive, PREFIX + 'summary.json')
        summary = json.loads(summary_bytes)
        answer['matrix_summary_sha256'] = digest(summary_bytes)
        answer['matrix_summary'] = {key: summary[key] for key in
            ('complete', 'conforming', 'exit_code', 'requested', 'statuses', 'budget_seconds',
             'thread_budget', 'signals', 'started_utc', 'ended_utc')}
        total_passed_tsan_closed = 0
        for config in summary['configurations']:
            name = config['name']
            prefix = PREFIX + name + '/'
            result_bytes = member(archive, prefix + 'result.json')
            result = json.loads(result_bytes)
            need(result['tests'] == config['tests'] and result['status'] == config['status'] and
                 result['conforming'] == config['conforming'], 'resume/result concordants : ' + name)
            tests_bytes = member(archive, prefix + 'tests.json')
            tests = json.loads(tests_bytes)
            log_bytes = member(archive, prefix + 'ctest.log')
            outcomes = {n: (status, seconds) for n, status, seconds in VERDICT.findall(log_bytes.decode())}
            passed = {n for n, (status, seconds) in outcomes.items() if status == 'Passed'}
            failed = {n for n, (status, seconds) in outcomes.items() if status != 'Passed'}
            missing = {test['name'] for test in tests} - set(outcomes)
            need(len(passed) == result['tests']['passed'] and len(tests) == result['tests']['selected'],
                 'comptes Passed explicites : ' + name)
            need(failed == {test['test'] for test in result['failures']} and
                 missing == {test['test'] for test in result['not_run']}, 'ensembles result/log : ' + name)
            family = 'gcc_asan' if name.startswith('gcc_asan') else 'gcc_tsan'
            need(not set(tests_by_family[family]) & {test['name'] for test in tests}, 'lots S non disjoints')
            tests_by_family[family].update({test['name']: test for test in tests})
            last_test = member(archive, prefix + 'LastTest.log')
            if failed:
                need(len(failed) == 3 and all('_roots_cost_uniform_u18_' in n for n in failed),
                     'echec autre que donnees uniformes')
                if family == 'gcc_asan':
                    need(last_test.count(b'"reason":"input_unreadable"') == 3, 'cause des trois refus IO ASan')
            row = {key: result[key] for key in ('name', 'status', 'conforming', 'reason', 'seconds',
                                                'tests', 'cmake_options', 'ctest_args')}
            row.update(result_sha256=digest(result_bytes), inventory_sha256=digest(tests_bytes),
                ctest_log_sha256=digest(log_bytes), last_test_log_sha256=digest(last_test),
                completed_passed_count=len(passed), failed_tests=sorted(failed), missing_tests=sorted(missing),
                failure_reason='input_unreadable' if failed and family == 'gcc_asan' else None,
                failure_reason_inference='inputs_absents_memes_sondes_ASan' if failed and family == 'gcc_tsan' else None,
                s9_verdicts=[dict(test=test['name'], state=outcomes.get(test['name'], ('no_result', None))[0],
                                  seconds=outcomes.get(test['name'], ('no_result', None))[1])
                    for test in tests if test['name'].startswith('mhgp11_points_')],
                num_verdicts=[dict(test=test['name'], state=outcomes.get(test['name'], ('no_result', None))[0])
                    for test in tests if test['name'].startswith('mhgp11_num_')],
                steps=[{key: step[key] for key in ('name', 'status', 'exit_code', 'seconds',
                            'timed_out', 'timeout_seconds') if key in step} for step in result['steps']])
            answer['matrix_configurations'].append(row)
            if family == 'gcc_tsan' and result['conforming']:
                total_passed_tsan_closed += len(passed)
        need(total_passed_tsan_closed == 60, 'cinq lots TSan conformes / 60 portes')
        answer['completed_tsan_conforming_lots_passed'] = total_passed_tsan_closed
    answer['short_num_roots_and_sort_refusal_selected'] = {
        name: any(name in tests for tests in tests_by_family.values())
        for name in ('mhgp11_num_roots', 'mhgp11_points_unit_sort_refusal')}
    need(not any(answer['short_num_roots_and_sort_refusal_selected'].values()), 'porte courte selectionnee')
    matrix_bytes = git_source(NEXT_PIN, 'morsehgp3D_v11/tools/g4_matrix.json')
    matrix = json.loads(matrix_bytes)
    projected = []
    for family, tests in tests_by_family.items():
        need(len(tests) == 100, 'inventaire reel S : 100 portes par instrumentation')
        configs = [config for config in matrix['configurations'] if config['name'].startswith(family) and
                   ('_scale' in config['name'] or '_lidar' in config['name'])]
        sets = [{name for name, test in tests.items() if selected(test, config['ctest_args'])}
                for config in configs]
        union = set().union(*sets)
        expected = {name for name in tests if not name.endswith('_opt')}
        need(union == expected and sum(map(len, sets)) == len(union) == 68,
             'nouveaux lots : union exacte/disjointe des portes normales')
        need(all(len(names) >= config['min_tests'] for names, config in zip(sets, configs)), 'planchers')
        projected.append(dict(instrumentation=family, observed_S_inventory_count=100,
            projected_normal_union_count=len(union), projected_excluded_opt_count=32, disjoint=True,
            s9_selected_normal=sorted(name for name in union if name.startswith('mhgp11_points_')),
            lots=[dict(configuration=config['name'], count=len(names), min_tests=config['min_tests'])
                  for names, config in zip(sets, configs)]))
    answer['next_matrix_projection'] = dict(source_commit=NEXT_PIN, matrix_sha256=digest(matrix_bytes),
        shard_count=11, families=projected, execution_claimed=False)
    answer['limits'] = ['S ne qualifie pas les portes courtes de S8/S9 ni le refus de tri',
        'S ne joue aucun differentiel points_vs_python ni L2b',
        'matrice 8b projetee sur inventaires S observes ; pas une nouvelle session',
        '32 jumelles _opt par instrumentation exclues du nouveau perimetre',
        'TSan rest : trois Failed explicites, raison IO inferee ; LastTest.log interrompu sans details',
        'depend de la session locale close et des commits Git ; aucune donnee brute recopiee']
    return answer


if (HERE / 'SHA256SUMS').exists():
    for line in (HERE / 'SHA256SUMS').read_text().splitlines():
        sha, name = line.split('  ', 1)
        need(digest((HERE / name).read_bytes()) == sha, 'capsule SHA : ' + name)
result = derive()
if '--capture' in sys.argv:
    (HERE / 'summary.json').write_text(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2) + '\n')
else:
    need(result == json.loads((HERE / 'summary.json').read_text()), 'capture differe des preuves closes')
print(json.dumps(dict(session=result['session'], source_commit=PIN, archive_sha256=ARCHIVE_SHA,
    targeted_closure_verified=True, tsan_closed_lots=5, tsan_closed_passed=60,
    new_shards_normal_union_each=68, new_shards_excluded_opt_each=32,
    qualification_complete=False, native_cloud_runs=0), sort_keys=True))
