"""Porte du lanceur de mutants lui-meme (tests/mutants/run_mutants.py), sans aucune compilation.

Les programmes cmake et ctest sont remplaces par des doublures ecrites dans un dossier temporaire : elles lisent la
copie que le lanceur a mutee et rendent un code d'apres des mots-cles. Chaque verdict du lanceur est ainsi joue :
tue (code, signal, construction), survivant, invalide, temoin rouge, plancher, motif absent ou multiple, schema.

    python3 test_run_mutants.py <chemin de run_mutants.py>     -> 0 conforme, 1 desaccord, 3 plancher
"""
import hashlib
import json
import os
import sys
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'support'))
import mhgp11_gate  # noqa: E402

FLOOR = 45

STUB = r'''
import os, sys
argv = sys.argv[1:]
role = argv[0]
argv = argv[1:]
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
gate = argv[argv.index('-R') + 1]
text = source_text(build)
if gate == '^mhgp11_porte_absente$':
    print('No tests were found!!!')
    sys.exit(8)
if 'TEMOIN_ROUGE' in text:
    print('run_expect_verdict code')
    sys.exit(8)
if 'MUTATION_SIGNAL' in text:
    print('run_expect_verdict arret_anormal')
    sys.exit(8)
if 'MUTATION_DELAI' in text:
    print('***Timeout')
    sys.exit(8)
if 'MUTATION_TUEUSE' in text:
    print('run_expect_verdict code')
    sys.exit(8)
sys.exit(0)
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
            entry['porte'] = 'mhgp11_porte'
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
        return mhgp11_gate.run(argv, timeout=120)


def main():
    if len(sys.argv) != 2:
        print('usage : test_run_mutants.py <run_mutants.py>')
        return 2
    gate = mhgp11_gate.Gate('run_mutants')
    with tempfile.TemporaryDirectory() as folder:
        bench = Bench(folder, os.path.abspath(sys.argv[1]))
        before = tree_digest(bench.source)

        def manifest(mutants, floor=1, **more):
            content = {'module': 'essai', 'plancher': floor, 'mutants': mutants}
            content.update(more)
            return content

        def expect(what, result, code, line=None):
            gate.check(result.code == code,
                       '%s : %s, attendu code %d\n%s' % (what, result.describe(), code, result.stdout))
            if line is not None:
                gate.check(line in result.stdout.splitlines(),
                           '%s : ligne absente : %s\n%s' % (what, line, result.stdout))

        killer = bench.mutant('tueur', 'valeur = MUTATION_TUEUSE;')
        by_signal = bench.mutant('par_signal', 'valeur = MUTATION_SIGNAL;')
        by_timeout = bench.mutant('par_delai', 'valeur = MUTATION_DELAI;')
        equivalent = bench.mutant('equivalent', 'valeur = 1 ;')
        broken = bench.mutant('ne_compile_pas', 'valeur = NE_COMPILE_PAS;')
        refused = bench.mutant('refuse_a_la_construction', 'valeur = NE_COMPILE_PAS;', attendu='construction',
                               jeton='jeton_de_construction')

        # tous tues : code 0, causes comptees a part
        expect('tous tues', bench.run(manifest([killer, by_signal, by_timeout, refused], floor=4)), 0,
               'mutants_ok module=essai mutants=4 tues=4 dont_signal=1 dont_delai=1 dont_construction=1 plancher=4')
        # un survivant : code 1, meme si les autres sont tues
        result = bench.run(manifest([killer, equivalent]))
        expect('survivant', result, 1, 'SURVIVANTS module=essai : 1 sur 2')
        gate.check('mutants_ok' not in result.stdout, 'survivant : aucune ligne mutants_ok')
        # un mutant qui ne se construit pas alors qu'une porte devait le tuer : invalide, code 3
        expect('ne compile pas', bench.run(manifest([killer, broken])), 3, 'INVALIDES module=essai : 1 sur 2')
        # un survivant prime sur un invalide
        expect('survivant et invalide', bench.run(manifest([equivalent, broken])), 1)
        # refus attendu a la construction : la copie se construit -> survivant ; mauvais jeton -> invalide
        expect('construction sans refus', bench.run(manifest([bench.mutant(
            'se_construit', 'valeur = 2;', attendu='construction', jeton='jeton_de_construction')])), 1)
        expect('construction, autre jeton', bench.run(manifest([bench.mutant(
            'autre_jeton', 'valeur = NE_COMPILE_PAS;', attendu='construction', jeton='jeton_absent')])), 3)
        # porte absente de la copie : temoin rouge
        expect('porte absente', bench.run(manifest([bench.mutant('x', 'valeur = 2;', porte='mhgp11_porte_absente')])),
               1, 'TEMOIN ROUGE module=essai : aucun mutant juge')

        # plancher : manifeste trop court, selection trop courte, plancher baisse explicitement
        expect('plancher du manifeste', bench.run(manifest([killer], floor=2)), 3)
        expect('selection sous le plancher', bench.run(manifest([killer, by_signal], floor=2), '--only', 'tueur'), 3)
        expect('plancher baisse', bench.run(manifest([killer, by_signal], floor=2), '--only', 'tueur', '--floor', '1'),
               0, 'mutants_ok module=essai mutants=1 tues=1 dont_signal=0 dont_delai=0 dont_construction=0 plancher=1')
        expect('only inconnu', bench.run(manifest([killer]), '--only', 'absent'), 2)

        # motif absent, motif present deux fois : code 3, avant toute construction
        expect('motif absent', bench.run(manifest([dict(killer, cherche='valeur = 9;')])), 3)
        expect('motif multiple', bench.run(manifest([dict(killer, cherche='repete')])), 3)
        expect('fichier absent', bench.run(manifest([dict(killer, fichier='src/absent.cpp')])), 3)

        # schema : code 2
        expect('cle inconnue', bench.run(manifest([dict(killer, surprise=1)])), 2)
        expect('cle absente', bench.run(manifest([{k: v for k, v in killer.items() if k != 'cherche'}])), 2)
        expect('identifiant en double', bench.run(manifest([killer, killer])), 2)
        expect('remplacement identique', bench.run(manifest([dict(killer, remplace='valeur = 1;')])), 2)
        expect('fichier hors arbre', bench.run(manifest([dict(killer, fichier='../a.cpp')])), 2)
        expect('porte sans prefixe', bench.run(manifest([dict(killer, porte='porte')])), 2)
        expect('construction sans jeton', bench.run(manifest([bench.mutant('x', 'y', attendu='construction')])), 2)
        expect('plancher nul', bench.run(manifest([killer], floor=0)), 2)
        expect('cle de tete inconnue', bench.run(manifest([killer], surprise=1)), 2)
        expect('JSON illisible', bench.run(None, raw='{"module": '), 2)
        expect('cle JSON en double',
               bench.run(None, raw='{"module": "a", "module": "b", "plancher": 1, "mutants": []}'), 2)

        # --check : manifeste juge sans construction (les doublures ne sont pas appelees : aucun dossier de travail)
        expect('check', bench.run(manifest([killer, equivalent], floor=2), '--check'), 0,
               'manifeste_ok module=essai mutants=2 plancher=2')
        gate.check(not os.path.exists(os.path.join(folder, 'travail')), 'check : aucun dossier de travail cree')
        expect('check, motif absent', bench.run(manifest([dict(killer, cherche='valeur = 9;')]), '--check'), 3)
        expect('list', bench.run(manifest([killer]), '--list'), 0, 'tueur src/a.cpp mhgp11_porte')

        # temoin rouge : la porte echoue sans mutation, aucun mutant n'est juge
        bench.write('src/b.cpp', 'TEMOIN_ROUGE\n')
        expect('temoin rouge', bench.run(manifest([killer])), 1, 'TEMOIN ROUGE module=essai : aucun mutant juge')
        os.remove(os.path.join(bench.source, 'src', 'b.cpp'))

        # options d'un mutant et unites de construction passees a la configuration de la copie
        result = bench.run(manifest([dict(killer, options=['-DOPTION_DU_MUTANT=ON'])], construction=['core', 'num']),
                           '--cmake-arg=-DPROFIL=1', '--keep')
        expect('options', result, 0)
        with open(os.path.join(folder, 'travail', 'tueur', 'build', 'configure_args'), encoding='utf-8') as handle:
            configured = handle.read().splitlines()
        gate.check_eq(configured[-3:], ['-DMHGP11_MODULES=core;num', '-DPROFIL=1', '-DOPTION_DU_MUTANT=ON'],
                      'arguments de configuration de la copie')
        with open(os.path.join(folder, 'travail', 'tueur', 'src', 'src', 'a.cpp'), encoding='utf-8') as handle:
            gate.check('MUTATION_TUEUSE' in handle.read(), 'la copie gardee porte la mutation')

        # un dossier de travail etranger n'est jamais efface : refus, contenu intact
        foreign = os.path.join(folder, 'etranger')
        os.makedirs(foreign)
        with open(os.path.join(foreign, 'fichier'), 'w', encoding='utf-8') as handle:
            handle.write('a garder\n')
        argv = [sys.executable, bench.runner, '--manifest', os.path.join(folder, 'manifeste_1.json'), '--source',
                bench.source, '--work', foreign, '--cmake', bench.tools['cmake'], '--ctest', bench.tools['ctest']]
        gate.check_eq(mhgp11_gate.run(argv, timeout=120).code, 2, 'dossier de travail etranger refuse')
        gate.check(os.path.isfile(os.path.join(foreign, 'fichier')), 'dossier etranger intact')

        # les sources d'origine ne sont jamais modifiees
        gate.check_eq(tree_digest(bench.source), before, 'sources d origine intactes')
    return gate.finish(FLOOR)


if __name__ == '__main__':
    sys.exit(main())
