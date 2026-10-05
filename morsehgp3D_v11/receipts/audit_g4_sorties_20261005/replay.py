#!/usr/bin/env python3
"""Relecture bornée des archives locales ; aucun lancement natif ou accès réseau."""
import hashlib
import json
from pathlib import Path
import re
import tarfile

HERE = Path(__file__).resolve().parent
PREFIX = 'results/cmd/000_matrice/files/matrix/'
RESULT = re.compile(
    r'^\s*\d+/\d+ Test\s+#\d+:\s+(\S+)\s+\.+\s+'
    r'(Passed|\*\*\*[^\n]+?)\s+(\d+(?:\.\d+)?) sec\s*$', re.M)


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def load(path):
    return json.loads(path.read_bytes())


def member(archive, name):
    with archive.extractfile(name) as stream:
        return stream.read()


def fields(data):
    """Valeurs exactes sélectionnées ; ni host, ni argv contenant un nom de compte."""
    main = ('schema', 'budget_seconds', 'complete', 'conforming', 'exit_code',
            'started_utc', 'ended_utc', 'requested', 'statuses', 'thread_budget', 'signals')
    per = ('name', 'status', 'conforming', 'optional', 'reason', 'seconds', 'started_utc',
           'tests', 'threads', 'passed_labels', 'cmake_options', 'ctest_args')
    steps = ('name', 'status', 'exit_code', 'seconds', 'timed_out', 'timeout_seconds')
    safe = {k: data[k] for k in main if k in data}
    safe['configurations'] = []
    for config in data['configurations']:
        value = {k: config[k] for k in per if k in config}
        value['missing_tests'] = [x['test'] for x in config.get('not_run', [])]
        value['failed_tests'] = [x['test'] for x in config.get('failures', [])]
        value['steps'] = [{k: x[k] for k in steps if k in x} for x in config.get('steps', [])]
        safe['configurations'].append(value)
    return safe


def critical(name):
    return (name.startswith(('mhgp11_api_', 'mhgp11_io_'))
            or name in ('mhgp11_cli_contract', 'mhgp11_cli_contract_opt'))


def derive():
    snapshot = load(HERE / 'manifest.json')
    answer = {'schema': 'ehgp.v11.audit.g4_partial_summary.v1',
              'source_commit': snapshot['source_commit'],
              'qualification_complete': False,
              'excluded_active_session': snapshot['excluded_active_session'],
              'native_runs_by_auditor': 0, 'gcp_calls_by_auditor': 0,
              'sessions': [], 'mutants': None}
    for saved in snapshot['sessions']:
        name = saved['session']
        local = HERE / name
        hashes = saved['hash_verification']
        receipt_path = Path(hashes['receipt_original_path'])
        receipt_bytes = receipt_path.read_bytes()
        require(digest(receipt_bytes) == hashes['receipt_original_sha256'], name + ': receipt SHA')
        receipt = json.loads(receipt_bytes)
        require(receipt['commit'] == snapshot['source_commit'], name + ': commit')
        receipt_fields = saved['receipt_fields']
        require(all(receipt.get(k) == v for k, v in receipt_fields.items()), name + ': receipt fields')
        package = Path(hashes['package_original_path'])
        archive_path = Path(hashes['archive_original_path'])
        require(digest(package.read_bytes()) == receipt['package_sha256'] == saved['package_sha256'],
                name + ': package SHA')
        require(digest(archive_path.read_bytes()) == receipt['results_sha256']
                == saved['results_archive_sha256'], name + ': results archive SHA')
        plan_path = receipt_path.parent / 'package' / 'plan.json'
        require(digest(plan_path.read_bytes()) == receipt['plan_sha256'], name + ': plan SHA')
        require(plan_path.read_bytes() == saved['session_plan_text'].encode('utf-8'), name + ': exact plan')
        closure = (receipt['closure'] == 'stopped' and receipt['targeted_shutdown_certified']
                   and receipt['observed_after']['status'] == 'TERMINATED'
                   and receipt['observed_after']['name'] == receipt['target']['instance']
                   and receipt['generation'] == receipt['closing_generation']
                   == receipt['observed_after']['lastStartTimestamp']
                   and receipt['start_certified'] and receipt['private_key_deleted']
                   and receipt['reserve_released'] and receipt['oslogin_key_removed'])
        require(closure, name + ': targeted closure')
        require(receipt['results_verified'] and not receipt['results_skipped_members']
                and not receipt['overflow']['evicted'] and not receipt['overflow']['truncated_streams'],
                name + ': archive completeness')
        session = {'session': name, 'source_commit': receipt['commit'],
                   'plan_sha256': receipt['plan_sha256'],
                   'archive_sha256': receipt['results_sha256'],
                   'archive_bytes': receipt['results_bytes'],
                   'package_sha256': receipt['package_sha256'],
                   'receipt_status': receipt['status'], 'worker_exit_code': receipt['worker_exit_code'],
                   'targeted_closure_verified': closure, 'configurations': []}
        with tarfile.open(archive_path) as archive:
            summary_bytes = member(archive, hashes['summary_archive_member'])
            require(digest(summary_bytes) == hashes['summary_sha256'], name + ': summary SHA')
            summary = json.loads(summary_bytes)
            require(fields(summary) == saved['matrix_summary_fields'], name + ': summary fields')
            session['matrix_complete'] = summary['complete']
            session['matrix_conforming'] = summary['conforming']
            session['budget_seconds'] = summary['budget_seconds']
            names = set(archive.getnames())
            for config in summary['configurations']:
                chosen = ('name', 'status', 'conforming', 'reason', 'seconds', 'tests')
                row = {k: config[k] for k in chosen if k in config}
                row['missing_tests'] = [x['test'] for x in config.get('not_run', [])]
                row['failed_tests'] = [x['test'] for x in config.get('failures', [])]
                start = PREFIX + config['name'] + '/'
                log_name = start + 'ctest.log'
                if log_name in names:
                    text = member(archive, log_name).decode('utf-8')
                    matches = RESULT.findall(text)
                    outcomes = {n: (s, sec) for n, s, sec in matches}
                    require(len(matches) == len(outcomes), name + ': duplicate completed verdict')
                    passed = {n for n, (s, sec) in outcomes.items() if s == 'Passed'}
                    row['completed_passed_count'] = len(passed)
                    if config.get('tests') is not None:
                        require(len(passed) == config['tests']['passed'], name + ': explicit Passed count')
                        selected = json.loads(member(archive, start + 'tests.json'))
                        row['critical_verdicts'] = []
                        for test in selected:
                            n = test['name']
                            if critical(n):
                                state, seconds = outcomes.get(n, ('no_result', None))
                                row['critical_verdicts'].append({'test': n, 'state': state, 'seconds': seconds})
                        require(len(selected) == config['tests']['selected'], name + ': selected count')
                        require(set(row['missing_tests']) == {x['name'] for x in selected if x['name'] not in outcomes},
                                name + ': missing exact set')
                session['configurations'].append(row)
            if name.endswith('claudequalmatrice'):
                mutant_bytes = member(archive, hashes['mutants_lines_archive_member'])
                lines = re.findall(hashes['mutants_lines_selection_regex'], mutant_bytes.decode('utf-8'))
                require('\n'.join(lines) + '\n' == saved['mutants_exact_lines'],
                        name + ': exact mutant lines')
                modules = []
                for line in lines:
                    entry = dict(re.findall(r'(\w+)=(\w+)', line))
                    modules.append({k: v if k == 'module' else int(v) for k, v in entry.items()})
                totals = {k: sum(x[k] for x in modules)
                          for k in ('mutants', 'tues', 'dont_signal', 'dont_delai', 'dont_construction')}
                totals['by_expected_return_code'] = totals['tues'] - totals['dont_construction']
                answer['mutants'] = {'modules': modules, 'totals': totals}
        answer['sessions'].append(session)
    for plan in snapshot['plans']:
        require(digest(plan['exact_text'].encode('utf-8')) == plan['sha256'],
                'captured plan SHA ' + plan['name'])
    return answer


def main():
    for line in (HERE / 'SHA256SUMS').read_text().splitlines():
        sha, name = line.split('  ', 1)
        require(digest((HERE / name).read_bytes()) == sha, 'capsule SHA ' + name)
    result = derive()
    require(result == load(HERE / 'summary.json'), 'summary differs from pinned archives')
    for session in result['sessions']:
        print(session['session'] + ': archive_sha=ok plan_sha=ok targeted_closure=ok qualification=partial')
    total = result['mutants']['totals']
    print('mutants: tues=%d code=%d construction=%d signal=%d delai=%d' %
          (total['tues'], total['by_expected_return_code'], total['dont_construction'],
           total['dont_signal'], total['dont_delai']))
    print('audit_g4_partial_verdict conforme archives2 source00bd hors_natif hors_cloud')


if __name__ == '__main__':
    main()
