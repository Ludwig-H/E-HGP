#!/usr/bin/env python3
"""Relecture metadata locale de P9 ; aucun build, test natif ou cloud."""

import argparse

from collections import Counter

import hashlib

import io

import json

from pathlib import Path

import re

import subprocess

import tarfile

import xml.etree.ElementTree as ET

RESULT = re.compile(r'^\s*\d+/\d+ Test\s+#\d+:\s+(\S+)\s+\.+\s*'
                    r'(Passed|\*\*\*[^\n]+?)\s+(\d+(?:\.\d+)?) sec\s*$', re.M)

SAFE_STEP = ('name', 'status', 'exit_code', 'seconds', 'timed_out', 'timeout_seconds')

SAFE_TESTS = ('selected', 'passed', 'failed', 'not_run', 'ctest_total', 'ctest_failed')

def need(ok, detail):
    if not ok:
        raise RuntimeError(detail)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def sha_file(path):
    out = hashlib.sha256()
    with path.open('rb') as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b''):
            out.update(block)
    return out.hexdigest()

def read_member(archive, path, maximum=16 * 1024 * 1024):
    member = archive.getmember(path)
    need(member.isfile() and 0 <= member.size <= maximum, 'membre non borne : ' + path)
    return archive.extractfile(member).read()

def kv(data):
    return dict(line.split('=', 1) for line in data.decode().splitlines() if '=' in line)

def verdicts(data):
    entries = RESULT.findall(data.decode())
    out = {name: dict(test=name, state=state, seconds=seconds) for name, state, seconds in entries}
    need(len(entries) == len(out), 'verdicts termines dupliques')
    return out

def junit(data):
    root = ET.fromstring(data)
    out = {}
    for case in root.iter('testcase'):
        name = case.get('name')
        need(name and name not in out, 'JUnit : nom absent/duplique')
        state = ('failed' if case.find('failure') is not None or case.find('error') is not None
                 else 'skipped' if case.find('skipped') is not None else 'passed')
        out[name] = state
    return out

def important(name):
    return (name.startswith(('mhgp11_api_', 'mhgp11_io_', 'mhgp11_head_', 'mhgp11_points_', 'mhgp11_plat_')) or
            name.startswith(('mhgp11_num_roots', 'mhgp11_num_unit_roots')) or
            name in ('mhgp11_cli_contract', 'mhgp11_cli_contract_opt',
                     'mhgp11_cli_plat', 'mhgp11_cli_plat_opt'))

def source_tree(package, repo, pin):
    """Une archive Git selective ; egalite exacte de tous les fichiers utiles du paquet."""
    prefixes = ('morsehgp3D_v11/src/', 'morsehgp3D_v11/include/', 'morsehgp3D_v11/tests/',
                'morsehgp3D_v11/cmake/', 'morsehgp3D_v11/bench/', 'morsehgp3D_v11/reference/',
                'morsehgp3D_v11/tools/', 'morsehgp3D_v11/cli/', 'morsehgp3D_v11/python/')
    # Le controleur et les gardes s'executent localement ; seul le worker est livre au paquet.
    exact = {'morsehgp3D_v11/CMakeLists.txt', 'gcp-migration/v11_worker.sh'}
    tracked = subprocess.check_output(['git', '-C', str(repo), 'ls-tree', '-r', '--name-only', pin],
                                      text=True).splitlines()
    selected = {name for name in tracked if name in exact or name.startswith(prefixes)}
    # Un repertoire absent (include/, python/, etc.) n'est pas transmis comme pathspec inexistant.
    paths = sorted(exact & selected) + [prefix.rstrip('/') for prefix in prefixes
                                      if any(name.startswith(prefix) for name in selected)]
    git_tar = subprocess.check_output(['git', '-C', str(repo), 'archive', '--format=tar', pin, '--', *paths])
    manifest = {}
    with tarfile.open(fileobj=io.BytesIO(git_tar), mode='r:') as expected, tarfile.open(package, 'r:gz') as actual:
        wanted = {entry.name for entry in expected.getmembers() if entry.isfile()}
        need(wanted == selected, 'source Git selective incomplete ou non reguliere')
        # L'archive inclut egalement ses entrees de repertoires.
        actual_files = {entry.name for entry in actual.getmembers() if entry.isfile() and
                        (entry.name in exact or entry.name.startswith(prefixes))}
        need(actual_files == wanted, 'sources utiles ajoutees/manquantes dans le paquet')
        for name in sorted(wanted):
            original = read_member(expected, name)
            delivered = read_member(actual, name)
            need(original == delivered, 'source du paquet/Git divergent : ' + name)
            manifest[name] = sha(original)
    manifest_bytes = ''.join(digest + '  ' + name + '\n' for name, digest in sorted(manifest.items())).encode()
    return dict(method='git_archive_selectif_egalite_exacte', source_commit=pin,
                files_verified=len(manifest), manifest_sha256=sha(manifest_bytes),
                paths=paths, extra_or_missing_files=False,
                includes_docs_or_receipts=False)

def matrix(archive, prefix, all_members):
    summary_bytes = read_member(archive, prefix + 'summary.json')
    source = json.loads(summary_bytes)
    answer = {key: source[key] for key in ('schema', 'complete', 'conforming', 'exit_code', 'requested',
              'statuses', 'budget_seconds', 'thread_budget', 'signals', 'started_utc', 'ended_utc') if key in source}
    answer.update(summary_sha256=sha(summary_bytes), configurations=[])
    for entry in source['configurations']:
        folder = prefix + entry['name'] + '/'
        result_bytes = read_member(archive, folder + 'result.json')
        result = json.loads(result_bytes)
        need(result['status'] == entry['status'] and result['tests'] == entry['tests'], 'resume/result divergent')
        row = {key: result[key] for key in ('name', 'status', 'conforming', 'reason', 'seconds', 'cmake_options',
                                             'ctest_args') if key in result}
        recorded_tests = result.get('tests') or {}
        row['tests'] = {key: recorded_tests[key] for key in SAFE_TESTS if key in recorded_tests}
        row['steps'] = [{key: step[key] for key in SAFE_STEP if key in step} for step in result.get('steps', [])]
        row['result_sha256'] = sha(result_bytes)
        row['missing_tests'] = sorted(item['test'] for item in result.get('not_run', []))
        row['failed_tests'] = sorted(item['test'] for item in result.get('failures', []))
        if recorded_tests and folder + 'tests.json' in all_members and folder + 'ctest.log' in all_members:
            tests_bytes = read_member(archive, folder + 'tests.json')
            log_bytes = read_member(archive, folder + 'ctest.log')
            tests = json.loads(tests_bytes)
            names = {test['name'] for test in tests}
            need(len(names) == len(tests), 'inventaire : noms de tests dupliques')
            out = verdicts(log_bytes)
            need(set(out) <= names, 'verdict termine inconnu de l inventaire')
            passed = {name for name, value in out.items() if value['state'] == 'Passed'}
            failed = set(out) - passed
            missing = names - set(out)
            need(len(passed) == result['tests']['passed'] and len(tests) == result['tests']['selected'],
                 'comptes Passed explicites divergents : ' + result['name'])
            need(len(failed) == result['tests']['failed'] and len(missing) == result['tests']['not_run'],
                 'comptes failed/missing divergents : ' + result['name'])
            need(set(row['failed_tests']) <= failed and set(row['missing_tests']) <= missing,
                 'noms failed/missing divergents')
            # Les listes du runner peuvent etre plafonnees ; garder les ensembles complets observes.
            row['failed_tests'] = sorted(failed)
            row['missing_tests'] = sorted(missing)
            row['completed_passed_count'] = len(passed)
            row['inventory_sha256'] = sha(tests_bytes)
            row['ctest_log_sha256'] = sha(log_bytes)
            row['important_verdicts'] = [out.get(name, dict(test=name, state='no_result', seconds=None))
                                         for name in sorted(names) if important(name)]
            if folder + 'junit.xml' in all_members:
                xml_bytes = read_member(archive, folder + 'junit.xml')
                try:
                    xml = junit(xml_bytes)
                except ET.ParseError:
                    row['junit_state'] = 'interrompu_ou_incomplet'
                else:
                    row['junit_counts'] = dict(Counter(xml.values()))
                    row['junit_sha256'] = sha(xml_bytes)
                    row['junit_state'] = 'concordant' if set(xml) == set(out) and all(
                        xml[name] == ('passed' if value['state'] == 'Passed' else 'failed')
                        for name, value in out.items()) else 'partiel_ou_autre_etat'
        answer['configurations'].append(row)
    return answer


SESSION_NAME = 'v11.20261005.claudefinp9'
SOURCE_PIN = '38b76701b9b0198fc1c37afe16e1480e638e513c'
CASES = [
    ('mhgp11_points_vs_python', dict(min_clouds=407, min_sites=100000, min_delayed=50000),
     dict(random_clouds=400, fixed_fixtures=4, uniform_sizes=[300, 2000, 8000],
          k=[1, 2, 3, 4, 5], m_all=True, workers=2)),
    ('mhgp11_points_vs_python_lidar_ng00_k5', dict(min_clouds=0, min_sites=30000, min_delayed=20000),
     dict(data='lidar_ng00', full_frame=True, fixed_fixtures_listed=4,
          fixed_fixtures_compared_at_k5=2, k=[5], m_all=False, workers=8)),
    ('mhgp11_points_vs_python_lidar_ng01_k5', dict(min_clouds=0, min_sites=30000, min_delayed=20000),
     dict(data='lidar_ng01', full_frame=True, fixed_fixtures_listed=4,
          fixed_fixtures_compared_at_k5=2, k=[5], m_all=False, workers=8)),
    ('mhgp11_points_vs_python_lidar_ng02_k5', dict(min_clouds=0, min_sites=30000, min_delayed=20000),
     dict(data='lidar_ng02', full_frame=True, fixed_fixtures_listed=4,
          fixed_fixtures_compared_at_k5=2, k=[5], m_all=False, workers=8)),
]


def compact(session, repo):
    data = read_session(session, repo)
    need(data['state'] == 'closed' and data['source_commit'] == SOURCE_PIN, 'session/pin attendu')
    need(data['receipt_status'] == 'completed' and data['worker_exit_code'] == 0 and data['done'] == '0',
         'session non conforme')
    result = {key: data[key] for key in ('session', 'state', 'source_commit', 'receipt_status', 'done',
        'worker_exit_code', 'targeted_closure_verified', 'receipt_sha256', 'archive_sha256',
        'package_sha256', 'plan_sha256', 'source_tree_verification', 'data_verified_remote',
        'data_manifest_sha256', 'delivered_data_names')}
    result.update(schema='ehgp.v11.audit.finp9_differential_read.v1',
        phase='exploration_v11_hors_registre', backend='cpu_reference',
        profile='quantized_u21_input_only', public_status='not_claimed',
        differential_scope_conforming=True, global_qualification_complete=False,
        audit_native_or_cloud_runs=0)
    contracts = {}
    for path in ('morsehgp3D_v11/tests/points/tests.cmake', 'morsehgp3D_v11/tests/points/points_vs_python.py',
                 'morsehgp3D_v11/bench/points_radius.py', 'morsehgp3D_v11/cmake/gates.cmake',
                 'morsehgp3D_v11/cmake/run_expect.cmake', 'morsehgp3D_v11/CMakeLists.txt'):
        raw = subprocess.check_output(['git', '-C', str(repo), 'show', SOURCE_PIN + ':' + path])
        contracts[path] = dict(sha256=sha(raw), text=raw.decode())
    cmake = contracts['morsehgp3D_v11/tests/points/tests.cmake']['text']
    script = contracts['morsehgp3D_v11/tests/points/points_vs_python.py']['text']
    section = cmake[cmake.index('mhgp11_python_gate(mhgp11_points_vs_python '):cmake.index('# Echelle et trames')]
    need('LINE ' not in section, 'absence de contrat LINE devenue fausse')
    need('--clouds 400' in section and '--m-all --uniform=300,2000,8000 --k=1,2,3,4,5' in section and
         '--min-clouds 407 --min-sites 100000' in section and '--min-delayed 50000' in section,
         'contrat synthetique different')
    need('--clouds 0 --data=lidar_${frame} --k=5' in section and
         '--workers 8 --min-sites 30000 --min-delayed 20000' in section, 'contrat LiDAR different')
    need('set(MHGP11_COORD_BITS 21 CACHE STRING' in contracts['morsehgp3D_v11/CMakeLists.txt']['text'],
         'profil par defaut change')
    need('for s in range(n):' in script and "stats['orders'] += 1" in script and
         'except (Refusal, OSError, ValueError) as error:' in script and "verdict, code = 'refus', 2" in script,
         'traitement des ecarts/refus different')
    need('return 1 if k == 1 else k + 1' in contracts['morsehgp3D_v11/bench/points_radius.py']['text'],
         'qualification m(k) differente')
    need(len(data['commands']) == len(CASES), 'nombre de commandes different')
    result['tests'], result['counter_evidence'] = [], []
    with tarfile.open(session / 'results/results.tar.gz', 'r:gz') as archive:
        all_members = archive.getnames()
        plan = json.loads((session / 'package/plan.json').read_text())
        need(plan['python_packages'] == 'pinned' and
             plan['build_targets'] == ['mhgp11_points_probe', 'mhgp11_points_export'],
             'plan de construction/dependances different')
        configure = read_member(archive, 'results/build/configure/argv.txt').decode()
        need('-DCMAKE_BUILD_TYPE=Release' in configure and '-DMHGP11_COORD_BITS' not in configure,
             'profil/construction differents')
        versions_raw = read_member(archive, 'results/env/python_packages.txt')
        versions = dict(item.split('=', 1) for item in versions_raw.decode().strip().split(','))
        need(versions == {'hdbscan': '0.8.44', 'numpy': '2.2.6', 'scikit-learn': '1.7.2', 'scipy': '1.15.3'},
             'versions Python non epinglees')
        result['python_environment'] = dict(policy='pinned', observed_versions=versions,
                                          observed_versions_sha256=sha(versions_raw))
        preserved_counters = []
        for index, (row, case) in enumerate(zip(data['commands'], CASES)):
            name, floors, parameters = case
            need(row['kind'] == 'direct_ctest' and row['expected_single_test'] == name and
                 row['metadata']['status'] == 'ok' and row['metadata']['exit_code'] == '0' and
                 row['completed_passed_count'] == 1 and not row['failed_tests'] and not row['missing_tests'] and
                 row['verdicts'][0]['state'] == 'Passed', 'porte non conforme : ' + name)
            prefix = 'results/cmd/%03d_%s/' % (index, row['name'])
            for member in all_members:
                if member.startswith(prefix) and member.endswith(('stdout', 'stderr', 'LastTest.log', 'junit.xml')):
                    text = read_member(archive, member).decode('utf-8', 'replace')
                    if 'points_vs_python_verdict' in text or 'python_refusals' in text:
                        preserved_counters.append(member)
            result['tests'].append(row)
            result['counter_evidence'].append(dict(test=name, parameters=parameters,
                lower_bounds_implied_by_passed_gate=floors,
                exact_business_counts_not_observed=True,
                business_verdict_line_directly_preserved=False,
                python_refusals_counter_not_instrumented=True,
                refusal_skipping_not_present_in_judge=True))
    need(not preserved_counters, 'hypothese de conservation devenue fausse')
    result['source_contracts'] = {path: dict(sha256=value['sha256']) for path, value in contracts.items()}
    result['coverage'] = dict(direct_passed_ctests=4, failed_ctests=0, missing_ctests=0,
        budget_interruptions=0, native_gate_python_opt_qualified=False,
        business_verdict_line_directly_preserved=False, exact_counts_claimed=False,
        synthetic_qualification_k1_m1_and_k2to5_m1_kplus1=True,
        lidar_k5_m6=True, complete_frame_inputs=True,
        every_site_and_all_point_tree_records_compared=True,
        common_full_tower_catalogue_comparison=True,
        expected_fields=['Morton_order_PointIds', 'forest_parent_rank', 'exact_fraction_t_M_Q',
                         'owner', 'floor', 'strict', 'referenced_rank_levels',
                         'plateau_levels', 'block_plateau_parent', 'site_block_plateau'])
    result['limits'] = [
        'Les quatre PASS CTest sont observes ; les totaux nuages/ordres/sites/retardes/plateaux ne sont pas conserves dans un log metier.',
        'P9 impose des planchers, aucune LINE de totaux exacts ; seuls ces minima et le parcours programme sont qualifiables ici.',
        'Le juge ne possede pas de compteur python_refusals ni de voie comptant puis sautant un refus ; aucun total de refus Python mesure nest affirme.',
        'Chaque commande LiDAR liste aussi quatre fixtures ; K5 en compare deux, les deux autres etant hors domaine. Ce ne sont pas cinq nuages effectivement pendus.',
        'Differentiel points sur le meme catalogue/tour ; ne constitue pas un nouvel oracle independant de construction de FULL.',
        'Portes longues sans jumelles -O ; les rejeux -O de cet auditeur ne sont pas une execution native des portes sous Python optimise.',
        'Qualification bornee a Release u21 ; aucun contrat temps FULL ni qualification globale u18/u24/sanitizers/mutants.',
        'Rejeu dependant des archives locales et du commit Git ; aucun journal brut, binaire natif ou octet LiDAR copie.',
    ]
    return result


def main():
    parser = argparse.ArgumentParser(description='Relecture locale metadata seule du differentiel S9 P9 clos.')
    parser.add_argument('--session', type=Path, default=Path('/workspaces/.ehgp-sessions') / SESSION_NAME)
    parser.add_argument('--repo', type=Path, default=Path('/workspaces/E-HGP'))
    args = parser.parse_args()
    answer = compact(args.session.resolve(), args.repo.resolve())
    expected = json.loads(Path(__file__).with_name('summary.json').read_text())
    need(answer == expected, 'relecture divergente du resume fige')
    print(json.dumps(dict(session=answer['session'], source_commit=answer['source_commit'],
        archive_sha256=answer['archive_sha256'], targeted_closure_verified=True,
        direct_passed_ctests=4, business_counts_preserved=False, exact_counts_claimed=False,
        differential_scope_conforming=True, global_qualification_complete=False, replay='conforme'),
        ensure_ascii=False, sort_keys=True, indent=2))

def direct_ctest(archive, prefix, command, all_members):
    args = command['argv']
    pattern = args[args.index('-R') + 1] if '-R' in args else None
    expected = pattern[1:-1] if pattern and re.fullmatch(r'\^mhgp11_[a-zA-Z0-9_]+\$', pattern) else None
    meta = kv(read_member(archive, prefix + 'meta.txt'))
    row = dict(name=command['name'], kind='direct_ctest', expected_single_test=expected,
               timeout_seconds=command['timeout_seconds'],
               metadata={key: meta[key] for key in ('status', 'exit_code', 'wall_seconds', 'timeout_seconds') if key in meta})
    if prefix + 'stdout' in all_members:
        log = read_member(archive, prefix + 'stdout')
        out = verdicts(log)
        row['stdout_sha256'] = sha(log)
        row['verdicts'] = list(out.values())
        row['completed_passed_count'] = sum(value['state'] == 'Passed' for value in out.values())
        row['failed_tests'] = sorted(name for name, value in out.items() if value['state'] != 'Passed')
        row['missing_tests'] = [expected] if expected and expected not in out else []
        if expected:
            need(set(out) <= {expected}, 'selection directe inattendue')
        if meta.get('status') == 'ok':
            need(row['completed_passed_count'] > 0 and not row['failed_tests'] and not row['missing_tests'],
                 'succès ctest sans verdict termine')
    return row

def read_session(session, repo):
    required = [session / 'DONE', session / 'receipt.json', session / 'results/results.tar.gz']
    if not all(path.is_file() for path in required):
        return dict(schema='ehgp.v11.audit.local_read.v1', session=session.name, state='open',
                    final_receipt_present=(session / 'receipt.json').is_file(), qualification_claimed=False)
    receipt_bytes = (session / 'receipt.json').read_bytes()
    receipt = json.loads(receipt_bytes)
    need(receipt['source_kind'] == 'commit' and receipt['evidence_grade'] == 'pushed_commit', 'source non publiee')
    pin = receipt['commit']
    need(receipt['worker_source'] == 'commit:' + pin, 'identite worker/source')
    archive_path = session / 'results/results.tar.gz'
    need(sha_file(archive_path) == receipt['results_sha256'], 'archive SHA')
    package = session / 'package/package.tar.gz'
    need(sha_file(package) == receipt['package_sha256'], 'paquet source SHA')
    plan_bytes = (session / 'package/plan.json').read_bytes()
    need(sha(plan_bytes) == receipt['plan_sha256'], 'plan SHA')
    closed = (receipt['closure'] == 'stopped' and receipt['targeted_shutdown_certified'] and
              receipt['observed_after']['status'] == 'TERMINATED' and
              receipt['observed_after']['name'] == receipt['target']['instance'] and
              receipt['generation'] == receipt['closing_generation'] ==
              receipt['observed_after']['lastStartTimestamp'] and receipt['start_certified'] and
              receipt['private_key_deleted'] and receipt['reserve_released'] and receipt['oslogin_key_removed'])
    need(closed, 'fermeture ciblee non certifiee')
    need(receipt['results_verified'] and not receipt['results_skipped_members'] and
         not receipt['overflow']['evicted'] and not receipt['overflow']['truncated_streams'], 'resultats incomplets')
    need(receipt['data_verified_remote'], 'donnees non verifiees cote worker')
    source_proof = source_tree(package, repo, pin)
    answer = dict(schema='ehgp.v11.audit.local_read.v1', session=session.name, state='closed', source_commit=pin,
        receipt_status=receipt['status'], done=(session / 'DONE').read_text().strip(),
        worker_exit_code=receipt['worker_exit_code'], targeted_closure_verified=True,
        receipt_sha256=sha(receipt_bytes), archive_sha256=receipt['results_sha256'],
        package_sha256=receipt['package_sha256'], plan_sha256=receipt['plan_sha256'],
        source_tree_verification=source_proof,
        data_verified_remote=True, data_manifest_sha256=receipt['data_manifest_sha256'],
        delivered_data_names=sorted(item['name'] for item in receipt['data_files']),
        commands=[], native_cloud_runs=0, qualification_claimed=False)
    plan = json.loads(plan_bytes)
    with tarfile.open(archive_path, 'r:gz') as archive:
        all_members = set(archive.getnames())
        for index, command in enumerate(plan['commands']):
            prefix = 'results/cmd/%03d_%s/' % (index, command['name'])
            summary_path = prefix + 'files/matrix/summary.json'
            if summary_path in all_members:
                row = dict(name=command['name'], kind='matrix', matrix=matrix(archive, prefix + 'files/matrix/', all_members))
                if prefix + 'meta.txt' in all_members:
                    meta = kv(read_member(archive, prefix + 'meta.txt'))
                    row['metadata'] = {key: meta[key] for key in ('status', 'exit_code', 'wall_seconds') if key in meta}
            elif command['argv'][0] == 'ctest' and prefix + 'meta.txt' in all_members:
                row = direct_ctest(archive, prefix, command, all_members)
            else:
                row = dict(name=command['name'], kind='other_or_no_result', result_classified=False)
            answer['commands'].append(row)
    return answer


if __name__ == '__main__':
    main()
