"""Porte des aides de cmake/gates.cmake : proprietes des portes qu'elles enregistrent, lues dans la liste de CTest.

Le projet factice tests/support/gate_fixture est configure (sans compilateur), puis `ctest --show-only=json-v1` donne
pour chaque porte sa commande et ses proprietes. Sont juges : PYTHONDONTWRITEBYTECODE partout, delais par defaut (300 s,
3600 s sous long) et explicites, portes serie (scale*, lidar, mutant), saut des portes lidar sans donnees, jumelle
sous python3 -O des portes Python (sauf long), ligne attendue, arguments passes un a un.

    python3 test_gate_properties.py <cmake> <ctest> <racine v11> <dossier de travail>   -> 0, 1 ou 3
"""
import json
import os
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mhgp11_gate  # noqa: E402

FLOOR = 49


def definitions(command):
    """Definitions -DNOM=valeur d'une commande cmake -P run_expect.cmake."""
    found = {}
    for word in command:
        if word.startswith('-D') and '=' in word:
            name, value = word[2:].split('=', 1)
            found[name] = value
    return found


def main():
    if len(sys.argv) != 5:
        print('usage : test_gate_properties.py <cmake> <ctest> <racine v11> <dossier de travail>')
        return 2
    cmake, ctest, root, work = sys.argv[1], sys.argv[2], os.path.abspath(sys.argv[3]), os.path.abspath(sys.argv[4])
    gate = mhgp11_gate.Gate('gate_properties')
    shutil.rmtree(work, ignore_errors=True)
    fixture = os.path.join(root, 'tests', 'support', 'gate_fixture')
    configured = mhgp11_gate.run([cmake, '-S', fixture, '-B', work, '-DMHGP11_ROOT=' + root], timeout=120)
    if not gate.check(configured.code == 0, 'configuration du projet factice : %s\n%s%s'
                      % (configured.describe(), configured.stdout, configured.stderr)):
        return gate.finish(FLOOR)
    listed = mhgp11_gate.run([ctest, '--show-only=json-v1'], cwd=work, timeout=120)
    try:
        tests = {test['name']: test for test in json.loads(listed.stdout)['tests']}
    except (ValueError, KeyError):
        gate.check(False, 'liste de CTest illisible : %s' % listed.describe())
        return gate.finish(FLOOR)

    def properties(name):
        return {item['name']: item['value'] for item in tests[name].get('properties', [])}

    expected_names = ['mhgp11_fixture_lidar', 'mhgp11_fixture_long', 'mhgp11_fixture_plain', 'mhgp11_fixture_python',
                      'mhgp11_fixture_python_long', 'mhgp11_fixture_python_opt', 'mhgp11_fixture_refusal',
                      'mhgp11_fixture_scale', 'mhgp11_fixture_timeout']
    gate.check_eq(sorted(tests), expected_names, 'portes enregistrees (jumelle _opt sauf sous long)')
    if sorted(tests) != expected_names:
        return gate.finish(FLOOR)

    for name in expected_names:  # 9 x 3 = 27
        props, command = properties(name), tests[name]['command']
        gate.check('PYTHONDONTWRITEBYTECODE=1' in props.get('ENVIRONMENT', []), '%s : PYTHONDONTWRITEBYTECODE' % name)
        gate.check(command[-1].endswith('cmake/run_expect.cmake') and command[-2] == '-P',
                   '%s : jouee par run_expect.cmake' % name)
        gate.check(definitions(command).get('EXPECTED') in ('0', '1'), '%s : code attendu passe au script' % name)

    plain = properties('mhgp11_fixture_plain')
    gate.check_eq(sorted(plain.get('LABELS', [])), ['fast', 'unit'], 'labels')
    gate.check_eq(plain.get('TIMEOUT'), 300, 'delai par defaut')
    gate.check_eq(plain.get('RUN_SERIAL', False), False, 'porte ordinaire : pas serie')
    gate.check('SKIP_REGULAR_EXPRESSION' not in plain, 'porte ordinaire : jamais sautee')
    command = definitions(tests['mhgp11_fixture_plain']['command'])
    gate.check_eq((command.get('NARGS'), command.get('ARG0'), command.get('ARG1')), ('2', '-E', 'true'), 'arguments')
    gate.check('REQUIRE_DIR_ENV' not in command and 'EXPECT_LINE' not in command, 'ni donnees ni ligne exigees')

    timed = properties('mhgp11_fixture_timeout')
    gate.check_eq(timed.get('TIMEOUT'), 42, 'delai explicite')
    gate.check('MHGP11_FIXTURE=1' in timed.get('ENVIRONMENT', []), 'variable ENV de la porte')
    gate.check_eq(definitions(tests['mhgp11_fixture_timeout']['command']).get('EXPECT_LINE'), 'ligne attendue',
                  'ligne attendue passee au script')

    gate.check_eq(properties('mhgp11_fixture_long').get('TIMEOUT'), 3600, 'delai par defaut sous long')
    gate.check_eq(properties('mhgp11_fixture_scale').get('RUN_SERIAL'), True, 'porte scale : serie')

    lidar = properties('mhgp11_fixture_lidar')
    gate.check_eq(lidar.get('RUN_SERIAL'), True, 'porte lidar : serie')
    gate.check_eq(lidar.get('SKIP_REGULAR_EXPRESSION'), ['mhgp11_porte_sautee MHGP11_DATA_DIR'], 'porte lidar : saut')
    gate.check_eq(definitions(tests['mhgp11_fixture_lidar']['command']).get('REQUIRE_DIR_ENV'), 'MHGP11_DATA_DIR',
                  'porte lidar : dossier de donnees exige')

    twin = properties('mhgp11_fixture_python_opt')
    gate.check('PYTHONOPTIMIZE=1' in twin.get('ENVIRONMENT', []), 'jumelle : sous python3 -O')
    gate.check('PYTHONOPTIMIZE=1' not in properties('mhgp11_fixture_python').get('ENVIRONMENT', []),
               'porte Python : sans -O')
    python = definitions(tests['mhgp11_fixture_python']['command'])
    gate.check(python.get('ARG0', '').endswith('tests/support/test_gate_helper.py') and python.get('NARGS') == '1',
               'porte Python : le script est le premier argument')
    long_python = definitions(tests['mhgp11_fixture_python_long']['command'])
    gate.check_eq((long_python.get('NARGS'), long_python.get('ARG1'), long_python.get('ARG2'),
                   long_python.get('EXPECTED')), ('3', 'un', 'deux mots', '1'), 'porte Python : arguments et code')

    refusal = definitions(tests['mhgp11_fixture_refusal']['command'])
    gate.check_eq(refusal.get('EXPECT_LINE'), 'expect_refusal_verdict conforme jeton', 'porte de refus : verdict exige')
    words = [refusal.get('ARG%d' % index) for index in range(int(refusal.get('NARGS', '0')))]
    gate.check(words[-1].endswith('cmake/expect_refusal.cmake') and '-DTOKEN=jeton' in words
               and '-DREFUSAL0=faux' in words and '-DNREFUSAL=1' in words, 'porte de refus : script et options')
    shutil.rmtree(work, ignore_errors=True)
    return gate.finish(FLOOR)


if __name__ == '__main__':
    sys.exit(main())
