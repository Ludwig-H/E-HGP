"""Porte du lanceur de mutants lui-meme (tests/mutants/run_mutants.py), sans aucune compilation.

Premiere partie, doublures : cmake et ctest sont remplaces par des scripts qui lisent la copie mutee et rendent une
issue d'apres des mots-cles (code, rapport JUnit, verdict de run_expect.cmake). Chaque verdict du lanceur est joue :
tue (code, signal, delai, construction), survivant, invalide, temoin rouge, plancher, motif absent ou multiple,
schema.

Seconde partie, vrais cmake et ctest sur le projet sans compilateur tests/mutants/fixture : un mutant reellement tue
par sa porte, un equivalent qui survit, et les trois issues qu'une doublure ne prouve pas (audits du 2 octobre 2026) :
temoin saute, erreur de chargement de CTest, programme impossible a lancer (absent, ou script dont l'interprete
manque). Aucune ne doit rendre 0.

    python3 test_run_mutants.py <run_mutants.py> <cmake> <ctest> <racine v12>     -> 0 conforme, 1, 3 plancher
"""
import hashlib
import json
import os
import sys
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'support'))
import mhgp12_gate  # noqa: E402

FLOOR = 92

STUB = r'''
import os, sys
argv = sys.argv[2:]
role = sys.argv[1]
def source_text(build):
    with open(os.path.join(build, 'source_path')) as handle:
        root = handle.read()
    text = ''
    for folder, _, files in os.walk(root):
        for name in sorted(files):
            with open(os.path.join(folder, name), encoding='utf-8') as handle:
                text += handle.read()
    return text
if role == 'cmake' and '--build' in argv:
    build = argv[argv.index('--build') + 1]
    with open(os.path.join(build, 'build_args'), 'w') as handle:
        handle.write('\n'.join(argv))
    if 'NE_COMPILE_PAS' in source_text(build):
        print('erreur : jeton_de_construction')
        sys.exit(2)
    sys.exit(0)
if role == 'cmake':
    source, build = argv[argv.index('-S') + 1], argv[argv.index('-B') + 1]
    os.makedirs(build, exist_ok=True)
    with open(os.path.join(build, 'source_path'), 'w') as handle:
        handle.write(source)
    with open(os.path.join(build, 'configure_args'), 'w') as handle:
        handle.write('\n'.join(argv))
    sys.exit(0)
build = argv[argv.index('--test-dir') + 1]
name = argv[argv.index('-R') + 1].strip('^$')
report = argv[argv.index('--output-junit') + 1]
text = source_text(build)
def finish(status, lines, code):
    for line in lines:
        print(line)
    if status is not None:
        with open(report, 'w') as handle:
            handle.write('<testsuite><testcase name="%s" status="%s"/></testsuite>' % (name, status))
    sys.exit(code)
if name == 'mhgp12_porte_absente':
    finish(None, ['No tests were found!!!'], 8)
if 'TEMOIN_ROUGE' in text:
    finish('fail', ['run_expect_verdict code'], 8)
if 'TEMOIN_SAUTE' in text or 'MUTATION_SAUTEE' in text:
    finish('notrun', [], 0)
if 'MUTATION_SIGNAL' in text:
    finish('fail', ['run_expect_verdict arret_anormal'], 8)
if 'MUTATION_DELAI' in text:
    finish('fail', ['1/1 Test #1: %s ...........***Timeout   1.00 sec' % name], 8)
if 'MUTATION_LIGNE' in text:
    finish('fail', ['run_expect_verdict conforme', 'run_expect_verdict ligne_absente'], 8)
if 'MUTATION_SANS_JUGE' in text:
    finish('fail', ['Parse error'], 8)
if 'MUTATION_LANCEMENT' in text:
    finish('fail', ['run_expect_verdict lancement_impossible'], 8)
if 'MUTATION_INCOHERENTE' in text:
    finish('fail', ['run_expect_verdict conforme'], 8)
if 'MUTATION_AUTRE_NOM' in text:
    name = 'mhgp12_autre_porte'
    finish('fail', ['run_expect_verdict code'], 8)
if 'MUTATION_TUEUSE' in text:
    finish('fail', ['run_expect_verdict code'], 8)
finish('run', ['run_expect_verdict conforme'], 0)
'''


def tree_digest(root):
    digest = hashlib.sha256()
    for folder, folders, files in os.walk(root):
        folders.sort()
        for name in sorted(files):
            path = os.path.join(folder, name)
            digest.update(os.path.relpath(path, root).encode())
            with open(path, 'rb') as handle:
                digest.update(handle.read())
    return digest.hexdigest()


class Bench:
    """Arbre source factice, doublures de cmake et de ctest, et lancement du lanceur."""

    def __init__(self, folder, runner):
        self.folder = folder
        self.runner = runner
        self.source = os.path.join(folder, 'source')
        os.makedirs(os.path.join(self.source, 'src'))
        self.write('CMakeLists.txt', 'projet factice\n')
        self.write('src/a.cpp', 'ligne un\nvaleur = 1;\nligne trois\nrepete\nrepete\n')
        stub = os.path.join(folder, 'stub.py')
        with open(stub, 'w', encoding='utf-8') as handle:
            handle.write(STUB)
        self.tools = {}
        for role in ('cmake', 'ctest'):
            path = os.path.join(folder, role)
            with open(path, 'w', encoding='utf-8') as handle:
                handle.write('#!/bin/sh\nexec "%s" "%s" %s "$@"\n' % (sys.executable, stub, role))
            os.chmod(path, 0o755)
            self.tools[role] = path
        self.count = 0

    def write(self, relative, text):
        with open(os.path.join(self.source, relative), 'w', encoding='utf-8') as handle:
            handle.write(text)

    def mutant(self, name, replace, **more):
        entry = {'id': name, 'fichier': 'src/a.cpp', 'cherche': 'valeur = 1;', 'remplace': replace}
        if more.get('attendu') != 'construction':
            entry['porte'] = 'mhgp12_porte'
        entry.update(more)
        return entry

    def run(self, manifest, *extra, raw=None):
        self.count += 1
        path = os.path.join(self.folder, 'manifeste_%d.json' % self.count)
        with open(path, 'w', encoding='utf-8') as handle:
            handle.write(raw if raw is not None else json.dumps(manifest))
        argv = [sys.executable, self.runner, '--manifest', path, '--source', self.source,
                '--work', os.path.join(self.folder, 'travail'), '--jobs', '2',
                '--cmake', self.tools['cmake'], '--ctest', self.tools['ctest']] + list(extra)
        return mhgp12_gate.run(argv, timeout=120)


def manifest(mutants, floor=1, **more):
    content = {'module': 'essai', 'plancher': floor, 'mutants': mutants}
    content.update(more)
    return content


def expect(gate, what, result, code, line=None):
    gate.check(result.code == code, '%s : %s, attendu code %d\n%s' % (what, result.describe(), code, result.stdout))
    if line is not None:
        gate.check(line in result.stdout.splitlines(), '%s : ligne absente : %s\n%s' % (what, line, result.stdout))


def with_stubs(gate, folder, runner):
    bench = Bench(folder, runner)
    before = tree_digest(bench.source)
    killer = bench.mutant('tueur', 'valeur = MUTATION_TUEUSE;')
    by_signal = bench.mutant('par_signal', 'valeur = MUTATION_SIGNAL;')
    by_timeout = bench.mutant('par_delai', 'valeur = MUTATION_DELAI;')
    by_line = bench.mutant('par_ligne', 'valeur = MUTATION_LIGNE;')
    equivalent = bench.mutant('equivalent', 'valeur = 1 ;')
    broken = bench.mutant('ne_compile_pas', 'valeur = NE_COMPILE_PAS;')
    refused = bench.mutant('refuse_a_la_construction', 'valeur = NE_COMPILE_PAS;', attendu='construction',
                           jeton='jeton_de_construction')

    # tous tues : code 0, causes comptees a part ; compte rendu ecrit
    report = os.path.join(folder, 'compte_rendu.json')
    expect(gate, 'tous tues', bench.run(manifest([killer, by_signal, by_timeout, by_line, refused], floor=5),
                                        '--report', report), 0,
           'mutants_ok module=essai mutants=5 tues=5 dont_signal=1 dont_delai=1 dont_construction=1 plancher=5')
    with open(report, encoding='utf-8') as handle:
        written = json.load(handle)
    gate.check_eq((written['code'], written['temoin'], written['module'], len(written['manifeste_sha256']),
                   len(written['sources_sha256'])), (0, 'vert', 'essai', 64, 64), 'compte rendu : en-tete')
    gate.check_eq([(m['id'], m['verdict'], m['detail'], m['juge']) for m in written['mutants']],
                  [('tueur', 'TUE', 'code', 'mhgp12_porte'), ('par_signal', 'TUE', 'signal', 'mhgp12_porte'),
                   ('par_delai', 'TUE', 'delai', 'mhgp12_porte'), ('par_ligne', 'TUE', 'ligne', 'mhgp12_porte'),
                   ('refuse_a_la_construction', 'TUE', 'construction', 'construction:jeton_de_construction')],
                  'compte rendu : verdict et cause de chaque mutant')
    # un survivant : code 1, meme si les autres sont tues
    result = bench.run(manifest([killer, equivalent]), '--report', report)
    expect(gate, 'survivant', result, 1, 'SURVIVANTS module=essai : 1 sur 2')
    gate.check('mutants_ok' not in result.stdout, 'survivant : aucune ligne mutants_ok')
    with open(report, encoding='utf-8') as handle:
        gate.check_eq(json.load(handle)['code'], 1, 'compte rendu : code du survivant')
    # un mutant qui ne se construit pas alors qu'une porte devait le tuer : invalide, code 3
    expect(gate, 'ne compile pas', bench.run(manifest([killer, broken])), 3, 'INVALIDES module=essai : 1 sur 2')
    # un survivant prime sur un invalide
    expect(gate, 'survivant et invalide', bench.run(manifest([equivalent, broken])), 1)
    # refus attendu a la construction : la copie se construit -> survivant ; mauvais jeton -> invalide
    expect(gate, 'construction sans refus', bench.run(manifest([bench.mutant(
        'se_construit', 'valeur = 2;', attendu='construction', jeton='jeton_de_construction')])), 1)
    expect(gate, 'construction, autre jeton', bench.run(manifest([bench.mutant(
        'autre_jeton', 'valeur = NE_COMPILE_PAS;', attendu='construction', jeton='jeton_absent')])), 3)

    # une porte en echec ne tue que si le juge a dit pourquoi ; sinon le mutant est invalide, jamais tue
    for name, word in (('sans_juge', 'MUTATION_SANS_JUGE'), ('lancement', 'MUTATION_LANCEMENT'),
                       ('incoherente', 'MUTATION_INCOHERENTE'), ('sautee', 'MUTATION_SAUTEE'),
                       ('autre_nom', 'MUTATION_AUTRE_NOM')):
        result = bench.run(manifest([killer, bench.mutant(name, 'valeur = %s;' % word)]))
        expect(gate, 'porte %s' % name, result, 3, 'INVALIDES module=essai : 1 sur 2')
        gate.check('mutants_ok' not in result.stdout, 'porte %s : aucune ligne mutants_ok' % name)

    # temoin : la porte doit etre executee et passee sans mutation
    expect(gate, 'porte absente', bench.run(manifest([bench.mutant('x', 'valeur = 2;', porte='mhgp12_porte_absente')])),
           1, 'TEMOIN ROUGE module=essai : aucun mutant juge')
    bench.write('src/b.cpp', 'TEMOIN_ROUGE\n')
    expect(gate, 'temoin rouge', bench.run(manifest([killer]), '--report', report), 1,
           'TEMOIN ROUGE module=essai : aucun mutant juge')
    with open(report, encoding='utf-8') as handle:
        written = json.load(handle)
    gate.check_eq((written['code'], written['temoin'], written['mutants']), (1, 'rouge', []), 'compte rendu : temoin')
    bench.write('src/b.cpp', 'TEMOIN_SAUTE\n')
    expect(gate, 'temoin saute', bench.run(manifest([killer])), 1, 'TEMOIN ROUGE module=essai : aucun mutant juge')
    os.remove(os.path.join(bench.source, 'src', 'b.cpp'))

    # plancher : manifeste trop court, selection trop courte, plancher baisse explicitement
    expect(gate, 'plancher du manifeste', bench.run(manifest([killer], floor=2)), 3)
    expect(gate, 'selection sous le plancher', bench.run(manifest([killer, by_signal], floor=2), '--only', 'tueur'), 3)
    expect(gate, 'plancher baisse', bench.run(manifest([killer, by_signal], floor=2), '--only', 'tueur',
                                             '--floor', '1'), 0,
           'mutants_ok module=essai mutants=1 tues=1 dont_signal=0 dont_delai=0 dont_construction=0 plancher=1')
    expect(gate, 'only inconnu', bench.run(manifest([killer]), '--only', 'absent'), 2)

    # motif absent, motif present deux fois : code 3, avant toute construction
    expect(gate, 'motif absent', bench.run(manifest([dict(killer, cherche='valeur = 9;')])), 3)
    expect(gate, 'motif multiple', bench.run(manifest([dict(killer, cherche='repete')])), 3)
    expect(gate, 'fichier absent', bench.run(manifest([dict(killer, fichier='src/absent.cpp')])), 3)

    # plusieurs remplacements dans un mutant : tous appliques, chacun present une seule fois a son tour
    both = dict(bench.mutant('deux_remplacements', 'valeur = 2;'),
                aussi=[{'fichier': 'src/a.cpp', 'cherche': 'ligne trois', 'remplace': 'MUTATION_TUEUSE'}])
    expect(gate, 'aussi applique', bench.run(manifest([both])), 0)
    expect(gate, 'aussi absent', bench.run(manifest([dict(both, aussi=[
        {'fichier': 'src/a.cpp', 'cherche': 'ligne neuf', 'remplace': 'x'}])])), 3)
    expect(gate, 'aussi defait par le premier', bench.run(manifest([dict(both, aussi=[
        {'fichier': 'src/a.cpp', 'cherche': 'valeur = 1;', 'remplace': 'x'}])])), 3)
    expect(gate, 'aussi hors schema', bench.run(manifest([dict(both, aussi=[{'fichier': 'src/a.cpp'}])])), 2)
    expect(gate, 'aussi hors arbre', bench.run(manifest([dict(both, aussi=[
        {'fichier': '../a.cpp', 'cherche': 'a', 'remplace': 'b'}])])), 2)

    # schema : code 2
    expect(gate, 'cle inconnue', bench.run(manifest([dict(killer, surprise=1)])), 2)
    expect(gate, 'cle absente', bench.run(manifest([{k: v for k, v in killer.items() if k != 'cherche'}])), 2)
    expect(gate, 'identifiant en double', bench.run(manifest([killer, killer])), 2)
    expect(gate, 'remplacement identique', bench.run(manifest([dict(killer, remplace='valeur = 1;')])), 2)
    expect(gate, 'fichier hors arbre', bench.run(manifest([dict(killer, fichier='../a.cpp')])), 2)
    expect(gate, 'porte sans prefixe', bench.run(manifest([dict(killer, porte='porte')])), 2)
    expect(gate, 'construction sans jeton', bench.run(manifest([bench.mutant('x', 'y', attendu='construction')])), 2)
    expect(gate, 'plancher nul', bench.run(manifest([killer], floor=0)), 2)
    expect(gate, 'cle de tete inconnue', bench.run(manifest([killer], surprise=1)), 2)
    expect(gate, 'JSON illisible', bench.run(None, raw='{"module": '), 2)
    expect(gate, 'cle JSON en double',
           bench.run(None, raw='{"module": "a", "module": "b", "plancher": 1, "mutants": []}'), 2)

    # --check : manifeste juge sans construction (les doublures ne sont pas appelees : aucun dossier de travail)
    expect(gate, 'check', bench.run(manifest([killer, equivalent], floor=2), '--check'), 0,
           'manifeste_ok module=essai mutants=2 plancher=2')
    gate.check(not os.path.exists(os.path.join(folder, 'travail')), 'check : aucun dossier de travail cree')
    expect(gate, 'check, motif absent', bench.run(manifest([dict(killer, cherche='valeur = 9;')]), '--check'), 3)
    # --list : mutants a porte et mutants de construction
    result = bench.run(manifest([killer, refused]), '--list')
    expect(gate, 'list', result, 0, 'tueur src/a.cpp mhgp12_porte')
    gate.check('refuse_a_la_construction src/a.cpp construction:jeton_de_construction' in result.stdout.splitlines(),
               'list : mutant de construction')

    # options d'un mutant, unites de construction et fils passes a la configuration et a la construction de la copie
    result = bench.run(manifest([dict(killer, options=['-DOPTION_DU_MUTANT=ON'])], construction=['core', 'num']),
                       '--cmake-arg=-DPROFIL=1', '--keep')
    expect(gate, 'options', result, 0)
    with open(os.path.join(folder, 'travail', 'tueur', 'build', 'configure_args'), encoding='utf-8') as handle:
        configured = handle.read().splitlines()
    gate.check_eq(configured[-3:], ['-DMHGP12_MODULES=core;num', '-DPROFIL=1', '-DOPTION_DU_MUTANT=ON'],
                  'arguments de configuration de la copie')
    with open(os.path.join(folder, 'travail', 'tueur', 'build', 'build_args'), encoding='utf-8') as handle:
        gate.check_eq(handle.read().splitlines()[-2:], ['--parallel', '1'], 'un fil par construction de mutant')
    with open(os.path.join(folder, 'travail', 'temoin_0', 'build', 'build_args'), encoding='utf-8') as handle:
        gate.check_eq(handle.read().splitlines()[-2:], ['--parallel', '2'], 'tous les fils pour le temoin')
    with open(os.path.join(folder, 'travail', 'tueur', 'src', 'src', 'a.cpp'), encoding='utf-8') as handle:
        gate.check('MUTATION_TUEUSE' in handle.read(), 'la copie gardee porte la mutation')

    # un dossier de travail etranger n'est jamais efface : refus, contenu intact
    foreign = os.path.join(folder, 'etranger')
    os.makedirs(foreign)
    with open(os.path.join(foreign, 'fichier'), 'w', encoding='utf-8') as handle:
        handle.write('a garder\n')
    argv = [sys.executable, bench.runner, '--manifest', os.path.join(folder, 'manifeste_1.json'), '--source',
            bench.source, '--work', foreign, '--cmake', bench.tools['cmake'], '--ctest', bench.tools['ctest']]
    gate.check_eq(mhgp12_gate.run(argv, timeout=120).code, 2, 'dossier de travail etranger refuse')
    gate.check(os.path.isfile(os.path.join(foreign, 'fichier')), 'dossier etranger intact')

    # les sources d'origine ne sont jamais modifiees
    gate.check_eq(tree_digest(bench.source), before, 'sources d origine intactes')


def with_real_ctest(gate, folder, runner, cmake, ctest, root):
    source = os.path.join(root, 'tests', 'mutants', 'fixture')
    before = tree_digest(source)
    count = [0]

    def run(mutants, env=None):
        count[0] += 1
        path = os.path.join(folder, 'reel_%d.json' % count[0])
        with open(path, 'w', encoding='utf-8') as handle:
            json.dump(manifest(mutants), handle)
        argv = [sys.executable, runner, '--manifest', path, '--source', source, '--work',
                os.path.join(folder, 'reel'), '--jobs', '1', '--cmake', cmake, '--ctest', ctest,
                '--cmake-arg=-DMHGP12_ROOT=' + root]
        return mhgp12_gate.run(argv, timeout=300, env=env)

    without_data = {key: value for key, value in os.environ.items() if key != mhgp12_gate.DATA_ENV}
    with_data = dict(without_data)
    with_data[mhgp12_gate.DATA_ENV] = folder
    hook = '# point d\'insertion des mutants de chargement\n'

    def mutant(name, **more):
        entry = {'id': name, 'fichier': 'src/value.txt', 'cherche': 'valeur 1', 'remplace': 'valeur 2',
                 'porte': 'mhgp12_fixture_value'}
        entry.update(more)
        return entry

    # un mutant reellement tue par sa porte : verdict code de run_expect.cmake
    expect(gate, 'reel : tue', run([mutant('valeur_changee')], without_data), 0,
           'mutants_ok module=essai mutants=1 tues=1 dont_signal=0 dont_delai=0 dont_construction=0 plancher=1')
    # un equivalent survit
    expect(gate, 'reel : survivant', run([mutant('commentaire', fichier='CMakeLists.txt', cherche=hook,
                                                 remplace='# commentaire modifie\n')], without_data), 1)
    # porte temoin sautee (label lidar, donnees absentes) : jamais un temoin vert
    result = run([mutant('porte_sautee', porte='mhgp12_fixture_lidar_value')], without_data)
    expect(gate, 'reel : temoin saute', result, 1, 'TEMOIN ROUGE module=essai : aucun mutant juge')
    gate.check('sautee sans mutation' in result.stdout, 'reel : temoin saute, issue lue dans le rapport de CTest')
    # la meme porte avec ses donnees : executee, donc temoin vert et mutant tue
    expect(gate, 'reel : porte lidar avec donnees', run([mutant('porte_jouee', porte='mhgp12_fixture_lidar_value')],
                                                       with_data), 0)
    # erreur de chargement de CTest : aucune porte n'est jouee, le mutant n'est pas tue
    load_error = mutant('chargement_casse', fichier='CMakeLists.txt', cherche=hook, remplace=(
        'set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES ${PROJECT_SOURCE_DIR}/src/casse.cmake)\n'))
    result = run([load_error], without_data)
    expect(gate, 'reel : erreur de chargement', result, 3, 'INVALIDES module=essai : 1 sur 1')
    gate.check('mutants_ok' not in result.stdout, 'reel : erreur de chargement, aucune ligne mutants_ok')
    # programme impossible a lancer : aucun juge n'a tourne, le mutant n'est pas tue
    no_launch = mutant('programme_absent', fichier='CMakeLists.txt', cherche='set(judge ${CMAKE_COMMAND})',
                       remplace='set(judge /mhgp12/programme/absent)')
    result = run([no_launch], without_data)
    expect(gate, 'reel : lancement impossible', result, 3, 'INVALIDES module=essai : 1 sur 1')
    gate.check('lancement_impossible' in result.stdout, 'reel : verdict de lancement impossible')
    # script juge existant et executable dont la ligne #! nomme un interprete absent : le lancement echoue a
    # l'execution, apres tout controle d'existence ; ce n'est pas un signal, et le mutant n'est pas tue
    script = {'fichier': 'src/judge.sh.in', 'porte': 'mhgp12_fixture_script'}
    result = run([mutant('interprete_absent', cherche='#!/bin/sh', remplace='#!/mhgp12/interprete/absent',
                         **script)], without_data)
    expect(gate, 'reel : interprete absent', result, 3, 'INVALIDES module=essai : 1 sur 1')
    gate.check('lancement_impossible' in result.stdout and 'TUE' not in result.stdout,
               'reel : interprete absent, jamais tue par signal')
    # le meme script juge reellement : un code de sortie change le tue
    expect(gate, 'reel : script tue', run([mutant('code_du_script', cherche='exit 0', remplace='exit 1', **script)],
                                         without_data), 0)
    gate.check_eq(tree_digest(source), before, 'reel : projet factice intact')


def main():
    if len(sys.argv) != 5:
        print('usage : test_run_mutants.py <run_mutants.py> <cmake> <ctest> <racine v12>')
        return 2
    runner, cmake, ctest, root = os.path.abspath(sys.argv[1]), sys.argv[2], sys.argv[3], os.path.abspath(sys.argv[4])
    gate = mhgp12_gate.Gate('run_mutants')
    with tempfile.TemporaryDirectory() as folder:
        with_stubs(gate, os.path.join(folder, 'doublures'), runner)
    with tempfile.TemporaryDirectory() as folder:
        with_real_ctest(gate, folder, runner, cmake, ctest, root)
    return gate.finish(FLOOR)


if __name__ == '__main__':
    sys.exit(main())
