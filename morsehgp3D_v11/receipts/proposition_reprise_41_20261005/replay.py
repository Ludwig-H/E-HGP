#!/usr/bin/env python3
"""Valide une proposition, sans build, test natif, acces cloud ni execution du plan."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tarfile


def need(ok, reason):
    if not ok:
        raise SystemExit('REFUS : ' + reason)


def digest(value):
    return hashlib.sha256(value).hexdigest()


def module_at(repo, pin, path, expected_sha):
    raw = subprocess.check_output(['git', 'show', pin + ':' + path], cwd=repo)
    need(digest(raw) == expected_sha, 'source du validateur differente : ' + path)
    namespace = {'__name__': 'audit_reprise_validator', '__file__': str(repo / path)}
    exec(compile(raw, str(repo / path), 'exec'), namespace)
    return namespace


def selected(config, tests):
    args = config['ctest_args']
    options = {args[i]: args[i + 1] for i in range(0, len(args), 2)}
    need(set(options) <= {'-R', '-E', '-L', '-LE'}, 'option de selection inattendue')
    def accept(test):
        name, labels = test['name'], test['labels']
        return (not test['disabled'] and
                ('-R' not in options or re.search(options['-R'], name) is not None) and
                ('-E' not in options or re.search(options['-E'], name) is None) and
                ('-L' not in options or any(re.search(options['-L'], x) for x in labels)) and
                ('-LE' not in options or not any(re.search(options['-LE'], x) for x in labels)))
    return [test for test in tests if accept(test)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path('/workspaces/E-HGP'))
    parser.add_argument('--session', type=Path,
                        default=Path('/workspaces/.ehgp-sessions/v11.20261005.claudefina2'))
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    scope = json.loads((here / 'scope.json').read_text())
    pin = scope['source_commit']
    validators = scope['validator_sources']
    matrix_module = module_at(args.repo, pin, 'morsehgp3D_v11/tools/g4_matrix.py',
                              validators['morsehgp3D_v11/tools/g4_matrix.py'])
    session_module = module_at(args.repo, pin, 'gcp-migration/v11_session.py',
                               validators['gcp-migration/v11_session.py'])
    matrix = matrix_module['load_matrix'](here / 'g4_reprise_41.json')
    raw_original = subprocess.check_output(
        ['git', 'show', pin + ':' + scope['source_matrix_path']], cwd=args.repo)
    need(digest(raw_original) == scope['source_matrix_sha256'], 'matrice source changee')
    original = json.loads(raw_original)
    originals = {c['name']: c for c in original['configurations']}
    plan = json.loads((here / 'plan_reprise_41.json').read_text())
    tracked = set(subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', pin],
                                          cwd=args.repo).decode().splitlines())
    session_module['validate_plan'](plan, tracked, set(scope['data_names']))
    target = scope['matrix_integration_path']
    need(target not in tracked, 'hypothese du pin actuel sans integration devenue fausse')
    need(plan['commands'][0]['argv'][plan['commands'][0]['argv'].index('--matrix') + 1]
         == '{src}/' + target, 'matrice du plan incorrecte')
    need(plan['commands'][0]['argv'][plan['commands'][0]['argv'].index('--only') + 1]
         == ','.join(c['name'] for c in matrix['configurations']), 'configuration omise du plan')
    archive = args.session / 'results/results.tar.gz'
    need(digest(archive.read_bytes()) == scope['fina2_archive_sha256'], 'archive fina2 changee')
    need(digest((args.session / 'receipt.json').read_bytes()) == scope['fina2_receipt_sha256'],
         'recu fina2 change')
    changed_keys = {'description', 'ctest_args', 'min_tests', 'require_labels', 'probes'}
    expected_by_name = {c['configuration']: c for c in scope['configurations']}
    counts = {}
    with tarfile.open(archive) as tar:
        for config in matrix['configurations']:
            name = config['name']
            expected = expected_by_name[name]
            original_config = matrix_module['validate_configuration'](originals[name])
            for key in set(config) - changed_keys:
                need(config[key] == original_config[key], 'contrainte du profil modifiee : ' + name + '/' + key)
            need(config['ctest_args'][:-2] == original_config['ctest_args'], 'filtres originaux modifies')
            need(config['ctest_args'][-2] == '-R', 'selection ciblee absente')
            need(config['probes'] == [], 'sonde hors reprise encore selectionnee')
            raw = tar.extractfile(expected['inventory_member']).read()
            need(digest(raw) == expected['inventory_sha256'], 'inventaire fina2 change')
            tests = json.loads(raw)
            names = {t['name'] for t in tests}
            need(len(names) == len(tests), 'inventaire avec doublons')
            result_path = expected['inventory_member'].rsplit('/', 1)[0]
            log = tar.extractfile(result_path + '/ctest.log').read().decode('utf-8', 'replace')
            outcomes = {}
            for line in log.splitlines():
                match = matrix_module['RESULT_RE'].match(line)
                if match:
                    need(match[1] not in outcomes, 'verdict double')
                    outcomes[match[1]] = match[2]
            need(set(outcomes) <= names, 'verdict hors inventaire')
            missing = names - set(outcomes)
            expected_names = set(expected['exact_tests'])
            need(missing == expected_names, 'restes fina2 differents du perimetre propose')
            picked = selected(config, tests)
            need({t['name'] for t in picked} == expected_names, 'selection ciblee differente des restes')
            need(config['min_tests'] == len(picked) == expected['expected_count'], 'plancher incorrect')
            labels = sorted({label for t in picked for label in t['labels']})
            need(config['require_labels'] == labels == expected['selected_labels'], 'labels non selectionnes exiges')
            need(set(config['require_labels_if_data']) <= set(labels), 'label conditionnel hors reprise')
            counts[name] = len(picked)
    need(sum(counts.values()) == scope['ordinary_occurrences'] == 41, 'total different de 41')
    need(len({n for c in scope['configurations'] for n in c['exact_tests']})
         == scope['distinct_test_names'] == 11, 'union des noms incorrecte')
    print(json.dumps({'plan_shape': 'PASS', 'matrix_shape': 'PASS', 'selection_against_fina2': counts,
                      'ordinary_occurrences': 41, 'distinct_names': 11,
                      'integration_required_at_pin': True, 'executed': False,
                      'native_or_cloud_runs': 0}, sort_keys=True))


if __name__ == '__main__':
    main()
