#!/usr/bin/env python3
"""Modèle borné du matcher EXPECT_LINE ; aucune exécution CMake ou native."""
import hashlib
import json
from pathlib import Path


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def matches(output, expected):
    normalized = output.replace('\r\n', '\n')
    return '\n' + expected + '\n' in '\n' + normalized + '\n'


def changed(value):
    return ('0' if value[0] != '0' else '1') + value[1:]


def main():
    root = Path(__file__).resolve().parent
    review = json.loads((root / 'review.json').read_text())
    model = json.loads((root / 'expect_line_model.json').read_text())
    for path, digest in review['captured_sha256'].items():
        need(hashlib.sha256((root / 'source' / path).read_bytes()).hexdigest() == digest,
             'source différente : ' + path)
    mutant = review['mutant_cli']
    need(mutant['new_target'] == 'mhgp11_cli_points' and mutant['floor'] == 28 and
         mutant['mutants'] == 28 and all(mutant['mutation_unchanged'].values()),
         'raccord ou plancher différent')
    checks = 0
    for row in model['rows']:
        current = row['new_expected_line']
        journal = row['former_common_journal']
        common = current.replace(' fils=', ' journal=' + journal + ' fils=')
        fingerprints = ('supports_route_empreintes fichier=' + row['former_u21_file'] +
                        ' manifeste=' + row['former_u21_manifest'] + ' journal=' + journal)
        pair = fingerprints + '\n' + common
        output = '{"route":"compute"}\n' + pair + '\n'
        probes = [
            (matches(output, common), True),
            (matches(output, pair), True),
            (matches(output.replace('\n', '\r\n'), pair), True),
            (matches(common + '\n', pair), False),
            (matches(common + '\n' + fingerprints + '\n', pair), False),
        ]
        for field, original in [('fichier', row['former_u21_file']),
                                ('manifeste', row['former_u21_manifest'])]:
            altered = output.replace(field + '=' + original, field + '=' + changed(original))
            probes.extend([(matches(altered, pair), False), (matches(altered, common), True)])
        altered_journal = output.replace('journal=' + journal, 'journal=' + changed(journal))
        probes.extend([(matches(altered_journal, common), False),
                       (matches(altered_journal, pair), False)])
        altered_count = output.replace('boules=', 'boules=9', 1)
        probes.extend([(matches(altered_count, common), False),
                       (matches(altered_count, pair), False)])
        # Le verdict WIP courant laisse passer des diagnostics différents à comptes constants.
        lost_guard_output = fingerprints.replace(journal, changed(journal)) + '\n' + current + '\n'
        probes.append((matches(lost_guard_output, current), True))
        for actual, expected in probes:
            need(actual == expected, 'matcher différent : ' + row['name'])
            checks += 1
    print(json.dumps({'verdict': 'conforme', 'modele': 'EXPECT_LINE_only',
                      'cas': len(model['rows']), 'verifications': checks,
                      'mutants_cli': mutant['mutants'], 'plancher_cli': mutant['floor'],
                      'native_or_cloud_actions': 0}, sort_keys=True))


if __name__ == '__main__':
    main()
