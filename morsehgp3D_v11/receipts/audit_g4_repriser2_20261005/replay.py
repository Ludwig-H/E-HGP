#!/usr/bin/env python3
"""Lecture locale de recus G4 clos ; aucun build, test natif ou acces reseau.

Usage : python3 -B read_closed_g4.py SESSION [--repo DEPOT]
Un JSON sur stdout ; aucune capsule ecrite et aucun resultat partiel promu.
"""
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


SESSION_NAME = 'v11.20261005.clauderepriser2'
SOURCE_PIN = '98a00955083d483306c4f92b9031e382e81b0e59'
OLD_SESSION = 'v11.20261005.claudefina2'
OLD_PIN = '38b76701b9b0198fc1c37afe16e1480e638e513c'
OLD_ARCHIVE_SHA = '42425a0969d728688672785f34f3302b698cf2daea83ca6c62891d6ff9b28631'
OLD_MISSING = {'gcc_release': ['mhgp11_cli_full_identity_lidar_ng01_k5_opt', 'mhgp11_cli_full_identity_lidar_ng02_k5_opt', 'mhgp11_cli_supports_lidar_ng00_k5_opt', 'mhgp11_cli_supports_lidar_ng01_k5_opt', 'mhgp11_cli_supports_lidar_ng02_k5_opt', 'mhgp11_cli_supports_scale16000_opt', 'mhgp11_cli_supports_scale32000_opt', 'mhgp11_cli_supports_scale8000_opt'], 'bits21': ['mhgp11_cli_full_determinism_scale8000_opt', 'mhgp11_cli_full_identity_lidar_ng00_k5_opt', 'mhgp11_cli_full_identity_lidar_ng01_k5_opt', 'mhgp11_cli_full_identity_lidar_ng02_k5_opt', 'mhgp11_cli_full_identity_scale32000_opt', 'mhgp11_cli_supports_lidar_ng00_k5_opt', 'mhgp11_cli_supports_lidar_ng01_k5_opt', 'mhgp11_cli_supports_lidar_ng02_k5_opt', 'mhgp11_cli_supports_scale16000_opt', 'mhgp11_cli_supports_scale32000_opt', 'mhgp11_cli_supports_scale8000_opt'], 'bits24': ['mhgp11_cli_full_determinism_scale8000_opt', 'mhgp11_cli_full_identity_lidar_ng00_k5_opt', 'mhgp11_cli_full_identity_lidar_ng01_k5_opt', 'mhgp11_cli_full_identity_lidar_ng02_k5_opt', 'mhgp11_cli_full_identity_scale32000_opt', 'mhgp11_cli_supports_lidar_ng00_k5_opt', 'mhgp11_cli_supports_lidar_ng01_k5_opt', 'mhgp11_cli_supports_lidar_ng02_k5_opt', 'mhgp11_cli_supports_scale16000_opt', 'mhgp11_cli_supports_scale32000_opt', 'mhgp11_cli_supports_scale8000_opt'], 'poison': ['mhgp11_cli_full_determinism_scale8000_opt', 'mhgp11_cli_full_identity_lidar_ng00_k5_opt', 'mhgp11_cli_full_identity_lidar_ng01_k5_opt', 'mhgp11_cli_full_identity_lidar_ng02_k5_opt', 'mhgp11_cli_full_identity_scale32000_opt', 'mhgp11_cli_supports_lidar_ng00_k5_opt', 'mhgp11_cli_supports_lidar_ng01_k5_opt', 'mhgp11_cli_supports_lidar_ng02_k5_opt', 'mhgp11_cli_supports_scale16000_opt', 'mhgp11_cli_supports_scale32000_opt', 'mhgp11_cli_supports_scale8000_opt']}
SUPPORTS_ROUTES = ['mhgp11_api_supports_route_lidar_ng00_k5', 'mhgp11_api_supports_route_lidar_ng01_k5', 'mhgp11_api_supports_route_lidar_ng02_k5', 'mhgp11_api_supports_route_scale16000', 'mhgp11_api_supports_route_scale32000', 'mhgp11_api_supports_route_scale8000']
PROFILE_BITS = {'gcc_release': 18, 'bits21': 21, 'bits24': 24, 'poison': 21}

def compact(session, repo, baseline_session):
    data = read_session(session, repo)
    need(data['state'] == 'closed' and data['source_commit'] == SOURCE_PIN, 'session R2/pin attendu')
    need(len(data['commands']) == 1 and data['commands'][0]['kind'] == 'matrix', 'commande matrice unique attendue')
    base = read_session(baseline_session, repo)
    need(base['state'] == 'closed' and base['source_commit'] == OLD_PIN and base['archive_sha256'] == OLD_ARCHIVE_SHA,
         'baseline fina2 differente')
    base_rows = base['commands'][0]['matrix']['configurations']
    need({row['name']: row['missing_tests'] for row in base_rows} == OLD_MISSING, 'anciennes 41 absences divergentes')
    result = {key: data[key] for key in ('session', 'state', 'source_commit', 'receipt_status', 'done',
        'worker_exit_code', 'targeted_closure_verified', 'receipt_sha256', 'archive_sha256',
        'package_sha256', 'plan_sha256', 'source_tree_verification', 'data_verified_remote',
        'data_manifest_sha256', 'delivered_data_names')}
    result.update(schema='ehgp.v11.audit.repriser2_scale_inventory_read.v1',
        phase='exploration_v11_hors_registre', backend='cpu_reference',
        profile='quantized_u21_input_only', public_status='not_claimed',
        global_qualification_complete=False, audit_native_or_cloud_runs=0)
    result['baseline'] = {key: base[key] for key in ('session', 'source_commit', 'receipt_sha256', 'archive_sha256')}
    result['baseline']['configurations'] = [{key: row[key] for key in
        ('name', 'inventory_sha256', 'ctest_log_sha256', 'result_sha256', 'missing_tests')} for row in base_rows]
    source = data['commands'][0]['matrix']
    result['matrix'] = {key: source[key] for key in ('summary_sha256', 'complete', 'conforming',
        'exit_code', 'budget_seconds', 'thread_budget', 'requested', 'statuses', 'started_utc', 'ended_utc')}
    matrix_raw = subprocess.check_output(['git', '-C', str(repo), 'show', SOURCE_PIN + ':morsehgp3D_v11/tools/g4_matrix.json'])
    result['source_matrix_sha256'] = sha(matrix_raw)
    result['configurations'], inventories, outcomes = [], {}, {}
    expected = {name + '_echelle_' + suffix for name in PROFILE_BITS for suffix in ('gros', 'reste')}
    with tarfile.open(session / 'results/results.tar.gz', 'r:gz') as archive:
        members = set(archive.getnames())
        for row in source['configurations']:
            name = row['name']
            need(name in expected and name not in inventories, 'configuration inconnue/dupliquee')
            profile_name = name.split('_echelle_')[0]
            bits = PROFILE_BITS[profile_name]
            need('-DMHGP11_COORD_BITS=' + str(bits) in row['cmake_options'] and
                 '-DCMAKE_BUILD_TYPE=Release' in row['cmake_options'], 'profil attendu')
            need(('-DMHGP11_POISON=ON' in row['cmake_options']) == (profile_name == 'poison'), 'profil poison attendu')
            prefix = 'results/cmd/000_matrice/files/matrix/' + name + '/'
            selected = {key: row[key] for key in ('name', 'status', 'conforming', 'tests', 'steps',
                'cmake_options', 'ctest_args', 'result_sha256', 'failed_tests', 'missing_tests') if key in row}
            selected.update(coord_bits=bits, ordinary_profile=profile_name)
            names, out = set(), {}
            if prefix + 'tests.json' in members:
                entries = json.loads(read_member(archive, prefix + 'tests.json'))
                names = {item['name'] for item in entries}
                need(len(names) == len(entries), 'inventaire duplique')
                need(not any(item['disabled'] for item in entries), 'porte desactivee')
                labels = set().union(*(set(item['labels']) for item in entries))
                need(not labels & {'mutant','long'}, 'porte longue/mutant hors objectif R2')
                selected.update(labels_selected=sorted(labels), inventory_unique=True,
                    inventory_sha256=sha(read_member(archive, prefix + 'tests.json')))
                if prefix + 'ctest.log' in members:
                    out = verdicts(read_member(archive, prefix + 'ctest.log'))
                    need(set(out) <= names, 'verdict inconnu de l inventaire')
                    selected['ctest_log_sha256'] = sha(read_member(archive, prefix + 'ctest.log'))
                for key in ('junit_state','junit_counts','junit_sha256'):
                    if key in row: selected[key] = row[key]
            selected.update(inventory_count=len(names), terminal_verdicts_count=len(out),
                all_selected_passed=bool(names) and set(out) == names and all(v['state']=='Passed' for v in out.values()))
            inventories[name], outcomes[name] = names, out
            result['configurations'].append(selected)
    need(set(inventories) == expected, 'configuration de R2 absente')
    profiles = []
    for profile_name, bits in PROFILE_BITS.items():
        a, b = profile_name + '_echelle_gros', profile_name + '_echelle_reste'
        need(not inventories[a] & inventories[b], 'lots R2 superposes dans un profil')
        names = inventories[a] | inventories[b]
        out = dict(outcomes[a], **outcomes[b])
        def state(test):
            return out[test]['state'] if test in out else 'no_result' if test in names else 'not_selected'
        missing_verdicts = {test: state(test) for test in OLD_MISSING[profile_name]}
        route_verdicts = {test: state(test) for test in SUPPORTS_ROUTES}
        profiles.append(dict(profile=profile_name, coord_bits=bits, lots=[a,b],
            union_inventory_count=len(names), disjoint_lots=True,
            union_names_sha256=sha(('\n'.join(sorted(names)) + '\n').encode()),
            old_missing_count=len(missing_verdicts), old_missing_verdicts=missing_verdicts,
            old_missing_all_passed=all(v=='Passed' for v in missing_verdicts.values()),
            supports_route_verdicts=route_verdicts,
            supports_routes_all_passed=all(v=='Passed' for v in route_verdicts.values())))
    all_41 = sum(v=='Passed' for row in profiles for v in row['old_missing_verdicts'].values())
    all_routes = sum(v=='Passed' for row in profiles for v in row['supports_route_verdicts'].values())
    result['coverage'] = dict(inventories_unique=True, profile_lots_disjoint=True,
        profiles=profiles, historical_missing_occurrences=41,
        historical_missing_completed_passed=all_41, historical_missing_all_passed=(all_41==41),
        supports_route_occurrences=24, supports_route_completed_passed=all_routes,
        supports_routes_all_passed=(all_routes==24),
        selected_total=sum(len(v) for v in inventories.values()),
        all_selected_have_terminal_pass=all(row['all_selected_passed'] for row in result['configurations']))
    result['ordinary_scale_scope_conforming'] = bool(source['complete'] and source['conforming'] and source['exit_code'] == 0 and result['coverage']['all_selected_have_terminal_pass'])
    result['coverage']['profile_union_inventories_identical'] = len({row['union_names_sha256'] for row in profiles}) == 1
    result['coverage']['u18_u24_repaired_routes_passed'] = sum(v == 'Passed' for row in profiles if row['profile'] in ('gcc_release', 'bits24') for v in row['supports_route_verdicts'].values())
    result['limits'] = [
        'Reprise ordinaire echelle/LiDAR des profils u18, u21, u24 et poison au pin 98a009550 ; les profils restent distincts.',
        'Les 41 absences historiques sont comparees nominativement aux inventaires et journaux de fina2 puis aux verdicts R2, sans transfert des sanitizers.',
        'Les 24 routes API comptent six noms dans chacun des quatre profils ordinaires ; les douze anciennement rouges u18/u24 sont identifiees separement par profil.',
        'R3 mutants et R4 ASan/UBSan gardent leurs propres resultats ; aucun transfert ni anticipation.',
        'Rejeu dependant des archives locales R2 et fina2 et des commits Git ; aucun journal brut, binaire natif ou octet LiDAR copie.',
    ]
    return result


def main():
    parser = argparse.ArgumentParser(description='Relecture locale metadata seule de R2 et couverture des 41 absences.')
    parser.add_argument('--session', type=Path, default=Path('/workspaces/.ehgp-sessions') / SESSION_NAME)
    parser.add_argument('--baseline-session', type=Path, default=Path('/workspaces/.ehgp-sessions') / OLD_SESSION)
    parser.add_argument('--repo', type=Path, default=Path('/workspaces/E-HGP'))
    parser.add_argument('--summary', type=Path, default=Path(__file__).with_name('summary.json'))
    args = parser.parse_args()
    answer = compact(args.session.resolve(), args.repo.resolve(), args.baseline_session.resolve())
    if args.summary:
        expected = json.loads(args.summary.read_text())
        need(answer == expected, 'relecture divergente du resume fige')
    print(json.dumps(dict(session=answer['session'], source_commit=answer['source_commit'], archive_sha256=answer['archive_sha256'], targeted_closure_verified=True, configurations={row['name']: row['tests'] for row in answer['configurations']}, historical_missing_completed_passed=answer['coverage']['historical_missing_completed_passed'], supports_route_completed_passed=answer['coverage']['supports_route_completed_passed'], ordinary_scale_scope_conforming=answer['ordinary_scale_scope_conforming'], global_qualification_complete=False, replay='conforme'), ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
