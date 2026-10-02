"""Lecteur LIVE : coherence des captures, jamais qualification d'une campagne echouee.

Usage : python3 [-O] check.py [reprise1/ reprise2/ ...]. Sans arguments : reprises presentes a cote du lecteur.
Le brut local epingle reste obligatoire ; aucune archive autonome ni nouvelle execution n'est revendiquee.
Code 0 : captures coherentes, y compris echecs historiques affiches ; code 1 : capture refusee.
"""
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import re
import sys
import tarfile
import xml.etree.ElementTree as ET

NAMES = set('gcc_release mutants gcc_asan_ubsan gcc_tsan clang_release bits21 bits24 poison style'.split())
BASE = 'results/cmd/000_matrice/files/matrix/'
CAP = 64 * 1024 * 1024


class Refusal(Exception):
    pass


def need(condition, reason):
    if not condition:
        raise Refusal(reason)


def unique(pairs):
    result = {}
    for key, value in pairs:
        need(key not in result, 'cle_dupliquee')
        result[key] = value
    return result


def nonfinite(_):
    raise Refusal('JSON_non_fini')


def js(data):
    return json.loads(data, object_pairs_hook=unique, parse_constant=nonfinite)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def fields(data):
    return unique(line.split('=', 1) for line in data.decode().splitlines() if line)


def judge_config(config, data):
    name, status = config['name'], config['status']
    prefix = BASE + name + '/'
    need(js(data[prefix + 'result.json']) == config, 'resultat_resume_different')
    need(config['conforming'] is (status == 'ok'), 'conformite_configuration')
    if status == 'absent':
        need(name == 'clang_release' and config['optional'] is True and not config['steps'] and
             'tests' not in config and prefix + 'junit.xml' not in data, 'absence_non_facultative')
        return (0, 0, 0, 0)
    selected = js(data[prefix + 'tests.json'])
    names = [test['name'] for test in selected]
    need(names and len(names) == len(set(names)) and all(isinstance(n, str) for n in names), 'selection_vide_double')
    need(all(test['disabled'] is False for test in selected), 'selection_desactivee')
    root = ET.fromstring(data[prefix + 'junit.xml'])
    cases = list(root.iter('testcase'))
    found = [case.get('name') for case in cases]
    need(root.tag == 'testsuite' and len(found) == len(set(found)) and set(found) == set(names), 'inventaire_JUnit')
    passed, failed, not_run, labels = 0, 0, 0, {}
    by_name = {test['name']: test for test in selected}
    for case in cases:
        state, bad, skip = case.get('status'), case.find('failure') is not None, case.find('skipped') is not None
        need(state in ('run', 'fail', 'notrun', 'disabled') and not (bad and skip), 'statut_JUnit')
        if bad or state == 'fail':
            failed += 1
        elif skip or state in ('notrun', 'disabled'):
            not_run += 1
        else:
            passed += 1
            for label in by_name[case.get('name')]['labels']:
                labels[label] = labels.get(label, 0) + 1
    need(passed + failed > 0, 'campagne_vide')
    need(int(root.get('tests')) == len(names) and int(root.get('failures')) == failed and
         int(root.get('disabled', 0)) + int(root.get('skipped', 0)) == not_run, 'totaux_JUnit')
    want = dict(selected=len(names), passed=passed, failed=failed, not_run=not_run,
                ctest_total=len(names), ctest_failed=failed)
    need(all(type(v) is int for v in config['tests'].values()) and config['tests'] == want and
         config['passed_labels'] == labels, 'compteurs_configuration')
    need(status in ('ok', 'failed', 'build_failed', 'incomplete', 'floor_violated'), 'statut_configuration')
    steps = unique((step['name'], step) for step in config['steps'])
    need(set(steps) == ({'configure', 'list', 'test'} if name == 'style' else {'configure', 'build', 'list', 'test'}),
         'etapes_configuration')
    if status == 'ok':
        need(not failed and not not_run and all(s['status'] == 'ok' and s['exit_code'] == 0 for s in steps.values()),
             'configuration_faussement_conforme')
    elif status == 'build_failed':
        need(steps['build']['status'] == 'failed' and steps['build']['exit_code'] != 0, 'construction_non_echouee')
    elif status == 'failed':
        need(failed > 0 or steps['test']['exit_code'] != 0, 'echec_sans_cause')
    return len(names), passed, failed, not_run


def check(folder):
    receipt = js((folder / 'receipt.json').read_bytes())
    raw_bytes = Path(receipt['raw_receipt_local']).read_bytes()
    need(sha(raw_bytes) == receipt['original_receipt_sha256'], 'hash_brut_local')
    raw = js(raw_bytes)
    need(all(json.dumps(raw[key], sort_keys=True) == json.dumps(value, sort_keys=True)
             for key, value in receipt.items() if key in raw), 'compact_different_du_brut')
    need(receipt['schema'] == 'ehgp.v11.session_receipt.v1' and receipt['source_kind'] == 'commit' and
         receipt['evidence_grade'] == 'pushed_commit' and re.fullmatch('[0-9a-f]{40}', receipt['commit']), 'source')
    need(receipt['closure'] == 'stopped' and receipt['targeted_shutdown_certified'] is True and
         receipt['results_verified'] is True and receipt['stop_exit_code'] == 0 and not receipt['errors'], 'cloture')
    need(receipt['generation'] == receipt['closing_generation'] == receipt['observed_after']['lastStartTimestamp']
         and receipt['observed_after']['status'] == 'TERMINATED', 'generation')
    blob = (folder / 'results.tar.gz').read_bytes()
    need(sha(blob) == receipt['results_sha256'] and len(blob) == receipt['results_bytes'], 'hash_archive')
    data, seen, size = {}, set(), 0
    with tarfile.open(fileobj=io.BytesIO(blob), mode='r:gz') as archive:
        for member in archive:
            path = PurePosixPath(member.name)
            need(member.name not in seen and not path.is_absolute() and '..' not in path.parts and
                 (member.isfile() or member.isdir()), 'membre_tar')
            seen.add(member.name)
            size += member.size
            need(0 <= member.size <= CAP and size <= CAP, 'taille_tar')
            if member.isfile():
                data[member.name] = archive.extractfile(member).read()
    need(data[BASE + 'summary.json'] == (folder / 'matrix.json').read_bytes(), 'copie_matrice')
    summary = js(data[BASE + 'summary.json'])
    need(summary['schema'] == 'ehgp.v11.g4_matrix_summary.v1', 'schema_matrice')
    configs = summary['configurations']
    names = [config['name'] for config in configs]
    need(len(names) == len(NAMES) and set(names) == NAMES and summary['complete'] is True and
         len(summary['requested']) == len(NAMES) and set(summary['requested']) == NAMES, 'configurations')
    need({p[len(BASE):].split('/')[0] for p in data if p.startswith(BASE) and '/' in p[len(BASE):]} == NAMES,
         'dossiers_configuration')
    statuses = {config['name']: config['status'] for config in configs}
    need(summary['statuses'] == statuses, 'statuts_resume')
    counts = {config['name']: judge_config(config, data) for config in configs}
    failed = [name for name, status in statuses.items() if status not in ('ok', 'absent')]
    code = 1 if summary.get('signals') or any(statuses[n] in ('failed', 'build_failed') for n in failed) else 3 if failed else 0
    need(type(summary['exit_code']) is int and summary['exit_code'] == code and
         summary['conforming'] is (code == 0), 'verdict_global')
    worker, meta = fields(data['results/worker.txt']), fields(data['results/cmd/000_matrice/meta.txt'])
    need(worker['source'] == 'commit:' + receipt['commit'] and worker['generation'] == receipt['generation'] and
         worker['package_sha256'] == receipt['package_sha256'], 'source_archive')
    need(int(meta['exit_code']) == code and meta['status'] == ('ok' if code == 0 else 'failed') and
         meta['group_closed'] == '1' and receipt['status'] == ('completed' if code == 0 else 'failed_remote') and
         receipt['worker_exit_code'] == (0 if code == 0 else 1), 'issue_session')
    need(worker['status'] == ('completed' if code == 0 else 'failed') and worker['commands_total'] == '1' and
         worker['commands_ok'] == ('1' if code == 0 else '0'), 'issue_worker')
    print('%s coherence=ok campagne=%s commit=%s' % (folder.name, 'ECHEC' if code else 'CONFORME', receipt['commit']))
    for name in sorted(counts):
        print('  %s %s selection/passes/echecs/non_joues=%s' % (name, statuses[name], '/'.join(map(str, counts[name]))))
    return bool(code)


def main():
    folders = [Path(p) for p in sys.argv[1:]] or sorted(Path(__file__).parent.glob('reprise[0-9]*'))
    need(folders, 'aucune_capture')
    failed = sum(check(folder) for folder in folders)
    print('captures_coherentes=%d campagnes_echouees=%d' % (len(folders), failed))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (Refusal, OSError, ValueError, KeyError, TypeError, ET.ParseError, tarfile.TarError) as error:
        print('REFUS ' + (str(error) if isinstance(error, Refusal) else type(error).__name__))
        sys.exit(1)
