#!/usr/bin/env python3
"""Relecture metadata locale de finmesure ; aucun build, test natif ou cloud."""

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


SESSION_NAME = 'v11.20261005.claudefinmesure'
SOURCE_PIN = '38b76701b9b0198fc1c37afe16e1480e638e513c'
STAGES = ('cloud', 'index', 'domain', 'tree', 'attach', 'output', 'write', 'total')


def compact(session, repo):
    data = read_session(session, repo)
    need(data['state'] == 'closed' and data['source_commit'] == SOURCE_PIN, 'session/pin attendu')
    need(data['receipt_status'] == 'completed' and data['worker_exit_code'] == 0 and data['done'] == '0',
         'session non conforme')
    result = {key: data[key] for key in ('session', 'state', 'source_commit', 'receipt_status', 'done',
        'worker_exit_code', 'targeted_closure_verified', 'receipt_sha256', 'archive_sha256',
        'package_sha256', 'plan_sha256', 'source_tree_verification', 'data_verified_remote',
        'data_manifest_sha256', 'delivered_data_names')}
    result.update(schema='ehgp.v11.audit.finmesure_observed_read.v1',
        phase='exploration_v11_hors_registre', backend='cpu_reference',
        profile='quantized_u21_input_only', public_status='not_claimed',
        observed_measurement_scope_valid=True, global_qualification_complete=False,
        audit_native_or_cloud_runs=0)
    contracts = {}
    for path in ('morsehgp3D_v11/bench/sorties_g4.py', 'morsehgp3D_v11/tests/cli/cli_supports_scale.py',
                 'morsehgp3D_v11/src/api/compute.cpp', 'morsehgp3D_v11/src/api/internal.hpp',
                 'morsehgp3D_v11/cli/mhgp11.cpp', 'morsehgp3D_v11/CMakeLists.txt'):
        raw = subprocess.check_output(['git', '-C', str(repo), 'show', SOURCE_PIN + ':' + path])
        contracts[path] = dict(sha256=sha(raw), text=raw.decode())
    need('set(MHGP11_COORD_BITS 21 CACHE STRING' in contracts['morsehgp3D_v11/CMakeLists.txt']['text'],
         'profil par defaut change')
    need('kSupportsRoute = SupportsRoute::full_tower;' in contracts['morsehgp3D_v11/src/api/internal.hpp']['text']
         and 'kEngineMask = 16379;' in contracts['morsehgp3D_v11/src/api/internal.hpp']['text'],
         'route supports/masque changes')
    compute = contracts['morsehgp3D_v11/src/api/compute.cpp']['text']
    need('wanted->k, api_detail::kSupportsRoute, report' in compute and
         'build_order_full(std::move(domain.value()), k, budget, api_detail::full_params()' in compute and
         'build_full(std::move(domain.value()), budget, nullptr, api_detail::full_params()' in compute,
         'chemins FULL/supports changes')
    bench = contracts['morsehgp3D_v11/bench/sorties_g4.py']['text']
    identity_namespace = {'__name__': 'audit_identity_only', '__file__': str(repo / 'morsehgp3D_v11/bench/sorties_g4.py')}
    exec(compile(bench, identity_namespace['__file__'], 'exec'), identity_namespace)
    need(len(data['commands']) == 3, 'nombre de commandes different')
    with tarfile.open(session / 'results/results.tar.gz', 'r:gz') as archive:
        plan = json.loads((session / 'package/plan.json').read_text())
        need(plan['python_packages'] == 'none' and plan['default_build'], 'plan de construction different')
        configure = read_member(archive, 'results/build/configure/argv.txt').decode()
        need('-DCMAKE_BUILD_TYPE=Release' in configure and '-DMHGP11_COORD_BITS' not in configure,
             'profil/construction differents')
        commands = []
        for index, c in enumerate(plan['commands']):
            prefix = 'results/cmd/%03d_%s/' % (index, c['name'])
            meta = kv(read_member(archive, prefix + 'meta.txt'))
            need(meta['status'] == 'ok' and meta['exit_code'] == '0', 'commande non conforme : ' + c['name'])
            commands.append(dict(name=c['name'], status=meta['status'], exit_code=0,
                stdout_sha256=sha(read_member(archive, prefix + 'stdout')),
                stderr_sha256=sha(read_member(archive, prefix + 'stderr'))))
        need([c['name'] for c in commands] == ['mesure_l2', 'supports_w48_ng02', 'supports_w48_ng00'],
             'commandes inattendues')
        raw = read_member(archive, 'results/cmd/000_mesure_l2/files/sorties_g4.json')
        document = json.loads(raw)
        config = document['configuration']
        need(config['frames'] == ['ng00', 'ng01', 'ng02'] and config['k'] == 5 and
             config['workers'] == [1, 48] and config['prises'] == 3 and config['k10'] == 'ng00',
             'configuration de mesure differente')
        calls = document['calls']
        need(document['schema'] == 'ehgp.v11.sorties_g4.v1' and document['defects'] == [] and
             identity_namespace['identity'](calls) == [], 'mesure invalide / identites divergentes')
        expected = {(f, 5, w, o, phase, prise) for f in config['frames'] for w in [1, 48]
                    for o in ['full', 'supports'] for phase, prises in [('froid', [0]), ('chaud', [1, 2, 3])]
                    for prise in prises}
        expected |= {('ng00', 10, 48, o, phase, prise) for o in ['full', 'supports']
                     for phase, prise in [('froid', 0), ('chaud', 1)]}
        key = lambda c: (c['frame'], c['k'], c['workers'], c['output'], c['phase'], c['prise'])
        need(len(calls) == len({key(c) for c in calls}) == 52 and {key(c) for c in calls} == expected and
             all(c['ok'] for c in calls), 'appels absents, dupliques ou en echec')
        need(document['engine_masks'] == {'full': 16379, 'supports': 16379}, 'masques differents')
        binary_sha = document['provenance']['cli_sha256']
        binary_rows = read_member(archive, 'results/provenance/binaries.sha256').decode().splitlines()
        need(any(line.split()[0] == binary_sha and line.split()[-1].rsplit('/', 1)[-1] == 'mhgp11'
                 for line in binary_rows), 'binaire mesure different de celui du worker')
        need(document['provenance']['commit'] == 'b319efc84', 'annotation obsolete differente')
        need(document['decision']['decision'] == 'build_order_par_defaut', 'decision historique differente')
        result['measure'] = dict(json_member='results/cmd/000_mesure_l2/files/sorties_g4.json',
            json_sha256=sha(raw), configuration=config, binary_sha256=binary_sha,
            masks_confirmed_by_pinned_route={'full': 16379, 'supports': 16379},
            all_calls_ok=True, total_calls=52, k5_calls=48, descriptive_k10_calls=4,
            defects=[], official_identity_check='conforme')
        result['obsolete_annotations'] = dict(declared_commit=document['provenance']['commit'],
            authoritative_source_commit=SOURCE_PIN, declared_commit_is_execution_source=False,
            declared_decision=document['decision']['decision'], decision_applicable=False,
            reason='Les deux voies mesurees construisent FULL/16379 ; aucun build_order/7035 mesure.')
        result['identity_groups'] = []
        for f, k, o in sorted({(c['frame'], c['k'], c['output']) for c in calls}):
            group = [c for c in calls if (c['frame'], c['k'], c['output']) == (f, k, o)]
            first = group[0]
            need(len({(c['file_sha256'], c['manifest_sha256'], c['tree_k_sha256'], c['file_bytes'])
                      for c in group}) == 1, 'identite de groupe differente')
            result['identity_groups'].append(dict(frame=f, k=k, output=o, calls=len(group),
                workers=sorted({c['workers'] for c in group}), file_sha256=first['file_sha256'],
                manifest_sha256=first['manifest_sha256'], tree_k_sha256=first['tree_k_sha256'],
                file_bytes=first['file_bytes']))
        result['calls'] = []
        for c in calls:
            need(set(c['stages_ns']) == set(STAGES) and
                 all(type(v) is int and v >= 0 for v in c['stages_ns'].values()), 'durees invalides')
            result['calls'].append({key: c[key] for key in ('frame', 'k', 'workers', 'output', 'phase', 'prise',
                'sites', 'wall_ns', 'stages_ns', 'peaks_bytes', 'counts')})
        hot = [c for c in calls if c['k'] == 5 and c['workers'] == 48 and c['phase'] == 'chaud']
        need(len(hot) == 18 and all(c['stages_ns']['tree'] > 100000000 and
             c['stages_ns']['total'] > 100000000 for c in hot), 'fait borne 100ms different')
        result['time_contract'] = dict(threshold_ns=100000000, k5_w48_hot_calls=18,
            tree_exceeds_threshold_on_every_observed_call=True,
            total_exceeds_threshold_on_every_observed_call=True,
            full_100ms_contract_acquired=False, k10_multi_frame_contract_acquired=False,
            scope='Temps observes par appel sur trois trames de la sequence08 ; aucune inference depuis medianes/ratios.')
        result['supports_w48_checks'] = []
        for index, frame in [(1, 'ng02'), (2, 'ng00')]:
            prefix = 'results/cmd/%03d_supports_w48_%s/' % (index, frame)
            text = read_member(archive, prefix + 'stdout').decode()
            lines = text.splitlines()
            line = next(x for x in lines if x.startswith('cli_supports_scale_verdict conforme '))
            fields = {k: int(v) for k, v in (word.split('=') for word in line.split()[2:])}
            controls = next(x for x in lines if x.startswith('cli_supports_scale_ok controles='))
            need(fields['k'] == 5 and fields['appels'] == 14 and
                 fields['sites'] == (45845 if frame == 'ng02' else 39885) and controls.endswith('=52'),
                 'controle supports W48 different')
            av = plan['commands'][index]['argv']
            need('--fils=1,4,48' in av and '--bits' in av and av[av.index('--bits') + 1] == '21',
                 'portee supports W48 differente')
            result['supports_w48_checks'].append(dict(frame=frame, workers=[1, 4, 48], profile='u21',
                verdict='conforme', observed_counters=fields, controls=52,
                covers_full_frame_and_cospherical_fixture=True, official_reader=True,
                repetitions_permutation_relabeling_and_FULL_tree_signature=True))
        result['commands'] = commands
    result['source_contracts'] = {path: dict(sha256=value['sha256']) for path, value in contracts.items()}
    result['limits'] = [
        'Valeurs conservees attribuees au commit du recu 38b76701b ; annotation b319efc84 et decision build_order_par_defaut obsoletees, non utilisees comme conclusions.',
        'Mesure FULL versus supports/L2b : les deux voies construisent FULL/16379, pas order_tree/7035, points ou plat.',
        '52 appels observes : 48 K5 sur trois trames a W1/W48, puis 4 K10 descriptifs sur ng00 W48 seulement.',
        'Identite du fichier et manifeste entre prises et W pour une meme sortie ; seule tree_k_sha256 est commune entre FULL et supports.',
        'Premier appel dit froid : entree fraichement ouverte, cache non vide ; aucune purge de cache certifiee.',
        '18 prises chaudes K5/W48 au-dessus de 100ms pour tree et total ; aucun contrat100ms acquis ni comparaison HDBSCAN/GPU.',
        'Qualification globale ouverte : 41 occurrences ordinaires manquantes, attentes supports_route u18/u24 et campagnes mutants API/CLI restent distinctes.',
        'Rejeu dependant des archives locales et du commit Git ; aucun journal brut, binaire natif ou octet LiDAR copie.',
    ]
    return result


def main():
    parser = argparse.ArgumentParser(description='Relecture metadata seule de la mesure G4 close.')
    parser.add_argument('--session', type=Path, default=Path('/workspaces/.ehgp-sessions') / SESSION_NAME)
    parser.add_argument('--repo', type=Path, default=Path('/workspaces/E-HGP'))
    args = parser.parse_args()
    answer = compact(args.session.resolve(), args.repo.resolve())
    expected = json.loads(Path(__file__).with_name('summary.json').read_text())
    need(answer == expected, 'relecture divergente du resume fige')
    print(json.dumps(dict(session=answer['session'], source_commit=answer['source_commit'],
        archive_sha256=answer['archive_sha256'], targeted_closure_verified=True,
        observed_calls=52, identity='conforme', supports_w48_calls_per_check=14,
        obsolete_decision_applied=False, full_100ms_contract_acquired=False,
        global_qualification_complete=False, replay='conforme'),
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
