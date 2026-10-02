"""Porte des aides de cmake/gates.cmake : proprietes des portes qu'elles enregistrent, lues dans la liste de CTest.

Le projet factice tests/support/gate_fixture est configure (sans compilateur), puis `ctest --show-only=json-v1` donne
pour chaque porte sa commande et ses proprietes. Sont juges : PYTHONDONTWRITEBYTECODE partout, delais par defaut (300 s,
3600 s sous long) et explicites, portes serie (scale*, lidar, mutant), saut des portes lidar sans donnees, jumelle
sous python3 -O des portes Python (sauf long), ligne attendue, arguments passes un a un. Puis les portes lidar sont
jouees par le vrai CTest : sautee sans donnees, jouee avec ; un programme qui ecrit lui-meme le jeton de saut est en
echec, jamais saute.

    python3 test_gate_properties.py <cmake> <ctest> <racine v11> <dossier de travail>   -> 0, 1 ou 3
"""
import json
import os
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mhgp11_gate  # noqa: E402

FLOOR = 69


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
    cmake, ctest, root = sys.argv[1], sys.argv[2], os.path.abspath(sys.argv[3])
    work = os.path.join(os.path.abspath(sys.argv[4]), 'essai_%d' % os.getpid())  # propre a ce processus
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

    expected_names = ['mhgp11_fixture_abort', 'mhgp11_fixture_abort_forged', 'mhgp11_fixture_lidar',
                      'mhgp11_fixture_long', 'mhgp11_fixture_plain',
                      'mhgp11_fixture_python',
                      'mhgp11_fixture_python_long', 'mhgp11_fixture_python_opt', 'mhgp11_fixture_refusal',
                      'mhgp11_fixture_scale', 'mhgp11_fixture_timeout', 'mhgp11_fixture_usurper']
    gate.check_eq(sorted(tests), expected_names, 'portes enregistrees (jumelle _opt sauf sous long)')
    if sorted(tests) != expected_names:
        return gate.finish(FLOOR)

    for name in expected_names:  # 12 x 3 = 36
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
    gate.check_eq(twin.get('DEPENDS'), ['mhgp11_fixture_python'], 'jumelle : lancee apres la porte')
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

    abort = definitions(tests['mhgp11_fixture_abort']['command'])
    words = [abort.get('ARG%d' % index) for index in range(int(abort.get('NARGS', '0')))]
    gate.check_eq((abort.get('EXPECTED'), abort.get('EXPECT_LINE')), ('0', None),
                  'porte d arret anormal : le juge doit reussir, aucune ligne enfant ne fait foi')
    gate.check(words[0].endswith('tests/support/expect_abnormal_stop.py') and words[1:] == [cmake, '-E', 'true'],
               'porte d arret anormal : le juge recoit le programme et ses arguments')
    aborted = mhgp11_gate.run_ctest_gate(ctest, work, 'mhgp11_fixture_abort', 120)
    gate.check_eq((aborted.status, aborted.verdict), ('echec', 'code'),
                  'porte d arret anormal sur un programme qui reussit : en echec')
    forged_abort = mhgp11_gate.run_ctest_gate(ctest, work, 'mhgp11_fixture_abort_forged', 120)
    gate.check_eq((forged_abort.status, forged_abort.verdict), ('echec', 'code'),
                  'porte d arret anormal : une ligne imitee suivie du code 3 ne prouve aucun signal')

    # les portes lidar jouees par le vrai CTest, sans puis avec le dossier de donnees
    without_data = {key: value for key, value in os.environ.items() if key != mhgp11_gate.DATA_ENV}
    with_data = dict(without_data)
    with_data[mhgp11_gate.DATA_ENV] = work
    skipped = mhgp11_gate.run_ctest_gate(ctest, work, 'mhgp11_fixture_lidar', 120, without_data)
    gate.check_eq((skipped.status, skipped.verdict), ('sautee', ''), 'porte lidar sans donnees : sautee, non jouee')
    played = mhgp11_gate.run_ctest_gate(ctest, work, 'mhgp11_fixture_lidar', 120, with_data)
    gate.check_eq(played.status, 'passe', 'porte lidar avec donnees : jouee et passee')
    forged = mhgp11_gate.run_ctest_gate(ctest, work, 'mhgp11_fixture_usurper', 120, with_data)
    gate.check_eq((forged.status, forged.verdict), ('echec', 'jeton_usurpe'),
                  'programme qui ecrit le jeton de saut : en echec, jamais saute')
    gate.check('mhgp11_porte_sautee' not in forged.output, 'le jeton usurpe n atteint pas la sortie de la porte')
    absent = mhgp11_gate.run_ctest_gate(ctest, work, 'mhgp11_fixture_absente', 120, with_data)
    gate.check_eq(absent.status, 'absente', 'porte inconnue : absente, jamais passee')
    plain_run = mhgp11_gate.run_ctest_gate(ctest, work, 'mhgp11_fixture_plain', 120, without_data)
    gate.check_eq((plain_run.status, plain_run.timed_out), ('passe', False), 'porte ordinaire : passee')
    shutil.rmtree(work, ignore_errors=True)
    return gate.finish(FLOOR)


if __name__ == '__main__':
    sys.exit(main())
