"""Porte du controle de style lui-meme (tools/check_style.py) : chaque regle sur un arbre factice, cas conforme et
cas en ecart, dont les frontieres (500 et 501 lignes, 100 et 101 lignes) et les pieges de l'heuristique des fonctions.

    python3 test_check_style.py <chemin de check_style.py>     -> 0 conforme, 1 desaccord, 3 plancher
"""
import os
import shutil
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mhgp12_gate  # noqa: E402

FLOOR = 142  # 133 dans la v11, plus les trois cas de la regle [mhgp11] et les trois de [recus]

HEADER = '// Role du fichier.\n'


def in_namespace(body):
    return HEADER + '#pragma once\nnamespace mhgp12 {\n' + body + '}  // namespace mhgp12\n'


def function(lines, signature='inline int f()'):
    """Fonction de `lines` lignes exactement, de la signature a l'accolade fermante."""
    return signature + ' {\n' + '  // ligne\n' * (lines - 2) + '}\n'


BASE = {
    'src/core/core.hpp': in_namespace('inline int one() { return 1; }\n'),
    'src/core/module.cmake': '# Module core.\n',
    'src/num/num.hpp': HEADER + '#pragma once\n#include "core/core.hpp"\nnamespace mhgp12::num {\n}\n',
    'src/num/module.cmake': '# Module num.\n',
}

TRICKY = in_namespace(
    'inline const char* a() { return "accolade { dans une chaine"; }\n'
    "inline char b() { return '{'; }\n"
    'inline int c() { return 1\'000\'000; }  // accolade { dans un commentaire\n'
    '/* bloc { de commentaire\n   sur deux lignes */\n'
    'inline const char* d() { return R"x(brut { ) " )x"; }\n'
    '#define MHGP12_BLOC(x) do { (void)(x); \\\n  } while (0)\n'
    'struct S {\n  S() : a_{1}, b_(2) {}\n  int a_;\n  int b_;\n};\n')

REASONS = ('// Table des raisons.\nMHGP12_REASON(none, ok, core)\n'
           'MHGP12_REASON(memory_budget, resource_exhausted, core)\n')
EMIT = 'inline int f() { return fail(Reason::memory_budget); }\n'

# (nom, fichiers ajoutes a la base, regle attendue ou None, nombre d'ecarts, arguments)
CASES = [
    ('base conforme', {}, None, 0, []),
    ('pieges de lecture', {'src/core/tricky.hpp': TRICKY}, None, 0, []),
    # taille des fichiers : 500 admis, 501 refuse
    ('fichier de 500 lignes', {'src/core/big.hpp': in_namespace('// ligne\n' * 496)}, None, 0, []),
    ('fichier de 501 lignes', {'src/core/big.hpp': in_namespace('// ligne\n' * 497)}, 'taille_fichier', 1, []),
    # taille des fonctions : 100 admis, 101 refuse, sous toutes les formes
    ('fonction de 100 lignes', {'src/core/f.hpp': in_namespace(function(100))}, None, 0, []),
    ('fonction de 101 lignes', {'src/core/f.hpp': in_namespace(function(101))}, 'taille_fonction', 1, []),
    ('methode de 101 lignes', {'src/core/f.hpp': in_namespace(
        'class C {\n public:\n' + function(101, '  int m() const') + '};\n')}, 'taille_fonction', 1, []),
    ('constructeur a liste de 101 lignes', {'src/core/f.hpp': in_namespace(
        'struct C {\n' + function(101, '  C() : a_{1}, b_(2)') + '  int a_;\n  int b_;\n};\n')},
     'taille_fonction', 1, []),
    ('lambda de 101 lignes', {'src/core/f.hpp': in_namespace(
        function(101, 'inline const auto g = [](int x)').rstrip('\n') + ';\n')}, 'taille_fonction', 1, []),
    ('gabarit de 101 lignes', {'src/core/f.hpp': in_namespace(
        function(101, 'template <class T>\nT h(T x)').replace('  // ligne\n', '', 1))}, 'taille_fonction', 1, []),
    ('fonction a type de retour suffixe', {'src/core/f.hpp': in_namespace(
        function(101, 'inline auto k() -> int'))}, 'taille_fonction', 1, []),
    ('deux fonctions de 60 lignes', {'src/core/f.hpp': in_namespace(
        function(60) + function(60, 'inline int f2()'))}, None, 0, []),
    ('initialiseur de 150 lignes', {'src/core/f.hpp': in_namespace(
        'inline constexpr int t[] = {\n' + '  1,\n' * 148 + '};\n')}, None, 0, []),
    ('enum de 150 lignes', {'src/core/f.hpp': in_namespace(
        'enum class E : int {\n' + ''.join('  v%d,\n' % i for i in range(148)) + '};\n')}, None, 0, []),
    ('classe de 150 lignes a methodes courtes', {'src/core/f.hpp': in_namespace(
        'class C {\n public:\n' + '  int m() const { return 1; }\n' * 147 + '};\n')}, None, 0, []),
    ('accolades non equilibrees', {'src/core/f.hpp': in_namespace('inline int f() {\n')}, 'accolades', 1, []),
    # assert
    ('assert dans src', {'src/core/a.cpp': in_namespace('inline void f(int x) { assert(x > 0); }\n')},
     'assert', 1, []),
    ('cassert dans src', {'src/core/a.cpp': HEADER + '#include <cassert>\nnamespace mhgp12 {\n}\n'}, 'assert', 1, []),
    ('assert.h dans src', {'src/core/a.cpp': HEADER + '#include <assert.h>\nnamespace mhgp12 {\n}\n'}, 'assert', 1, []),
    ('static_assert admis', {'src/core/a.cpp': in_namespace('static_assert(sizeof(int) == 4, "assert(");\n')},
     None, 0, []),
    ('assert en commentaire admis', {'src/core/a.cpp': in_namespace('// jamais assert(x)\n')}, None, 0, []),
    ('assert dans un test C++', {'tests/core/t.cpp': 'int main() { assert(1); }\n'}, 'assert', 1, []),
    # texte
    ('octet non ASCII', {'src/core/a.cpp': in_namespace('// caf\u00e9\n')}, 'ascii', 1, []),
    ('non ASCII admis dans les tests', {'tests/core/t.cpp': '// caf\u00e9\nint main() { return 0; }\n'}, None, 0, []),
    ('tabulation', {'src/core/a.cpp': in_namespace('\tinline int f() { return 1; }\n')}, 'tabulation', 1, []),
    ('en-tete absent', {'src/core/a.cpp': 'namespace mhgp12 {\n}\n'}, 'entete', 1, []),
    # namespace
    ('code hors namespace', {'src/core/a.cpp': HEADER + 'int f() { return 1; }\n'}, 'namespace', 1, []),
    ('declaration hors namespace', {'src/core/a.cpp': HEADER + 'extern int x;\n'}, 'namespace', 1, []),
    ('autre namespace', {'src/core/a.cpp': HEADER + 'namespace autre {\n}\n'}, 'namespace', 1, []),
    ('namespace anonyme de premier niveau', {'src/core/a.cpp': HEADER + 'namespace {\n}\n'}, 'namespace', 1, []),
    ('sous-namespace admis', {'src/core/a.cpp': HEADER + 'namespace mhgp12::detail {\nnamespace {\n}\n}\n'},
     None, 0, []),
    ('fichier .def hors regle namespace', {'src/core/t.def': '// Table.\nMHGP12_REASON(none, ok)\n'}, None, 0, []),
    # macros et identifiants de la v10
    ('macro sans prefixe', {'src/core/a.cpp': in_namespace('#define CHECK_X 1\n')}, 'macro', 1, []),
    ('macro a prefixe admise', {'src/core/a.cpp': in_namespace('#define MHGP12_X 1\n')}, None, 0, []),
    ('MHGP10 en commentaire', {'src/core/a.cpp': in_namespace('// port de MHGP10_CHECK\n')}, 'mhgp10', 1, []),
    ('namespace mhgp10', {'src/core/a.cpp': in_namespace('inline int f() { return mhgp10::g(); }\n')},
     'mhgp10', 1, []),
    ('MHGP10 dans un fichier CMake', {'src/core/module.cmake': '# Module.\noption(MHGP10_POISON "" OFF)\n'},
     'mhgp10', 1, []),
    # identifiants de la v11 (regle ajoutee au port de la v12, sur le modele de la precedente)
    ('MHGP11 en commentaire', {'src/core/a.cpp': in_namespace('// port de MHGP11_CHECK\n')}, 'mhgp11', 1, []),
    ('namespace mhgp11', {'src/core/a.cpp': in_namespace('inline int f() { return mhgp11::g(); }\n')},
     'mhgp11', 1, []),
    ('MHGP11 dans un fichier CMake', {'src/core/module.cmake': '# Module.\noption(MHGP11_POISON "" OFF)\n'},
     'mhgp11', 1, []),
    # une porte ne lit jamais un recu (CST-0014)
    ('receipts dans un module CMake',
     {'src/core/module.cmake': '# Module.\nset(X ${CMAKE_CURRENT_LIST_DIR}/receipts/a.json)\n'}, 'recus', 1, []),
    ('receipts dans un commentaire CMake', {'src/core/module.cmake': '# Module ; voir receipts/a/README.md.\n'},
     None, 0, []),
    ('receipts dans les portes d un test', {'tests/core/tests.cmake': 'add_test(NAME t COMMAND cat receipts/x.json)\n'},
     'recus', 1, []),
    # inclusions et dependances
    ('inclusion sans module', {'src/core/a.cpp': HEADER + '#include "buffer.hpp"\nnamespace mhgp12 {\n}\n'},
     'inclusion', 1, []),
    ('en-tete interne d un autre module', {'src/num/a.cpp': HEADER + '#include "core/status.hpp"\n'
                                           'namespace mhgp12 {\n}\n'}, 'inclusion', 1, []),
    ('en-tete interne du meme module', {'src/core/a.cpp': HEADER + '#include "core/status.hpp"\n'
                                        'namespace mhgp12 {\n}\n'}, None, 0, []),
    ('dependance inverse', {'src/core/a.cpp': HEADER + '#include "num/num.hpp"\nnamespace mhgp12 {\n}\n'},
     'dependance', 1, []),
    ('dependance hors table', {'src/sched/sched.hpp': HEADER + '#include "num/num.hpp"\nnamespace mhgp12 {\n}\n',
                               'src/sched/module.cmake': '# Module sched.\n'}, 'dependance', 1, []),
    ('dependance indirecte admise', {'src/index/index.hpp': HEADER + '#include "core/core.hpp"\n'
                                     '#include "num/num.hpp"\nnamespace mhgp12 {\n}\n',
                                     'src/index/module.cmake': '# Module index.\n'}, None, 0, []),
    ('cli : en-tete interne', {'cli/main.cpp': HEADER + '#include "core/status.hpp"\nint main() { return 0; }\n'},
     'inclusion', 1, []),
    ('cli : en-tetes publics', {'cli/main.cpp': HEADER + '#include "core/core.hpp"\n#include "num/num.hpp"\n'
                                'int main() { return 0; }\n'}, None, 0, []),
    # structure des modules et table
    ('dossier hors table', {'src/foo/foo.hpp': in_namespace(''), 'src/foo/module.cmake': '# Module.\n'},
     'module', 1, []),
    ('en-tete public absent', {'src/sched/pool.hpp': in_namespace(''), 'src/sched/module.cmake': '# Module.\n'},
     'module', 1, []),
    ('module.cmake absent', {'src/sched/sched.hpp': in_namespace('')}, 'module', 1, []),
    # Python
    ('assert Python', {'tests/core/test_x.py': 'x = 1\nassert x\n'}, 'python_assert', 1, []),
    ('Python illisible', {'tests/core/test_x.py': 'def f(:\n'}, 'python_assert', 1, []),
    ('Python conforme', {'tests/core/test_x.py': 'import sys\nsys.exit(0)\n'}, None, 0, []),
    # table des raisons : emise par son module, citee par ses portes
    ('raison emise et citee', {'src/core/reasons.def': REASONS, 'src/core/a.hpp': in_namespace(EMIT),
                               'tests/core/t.cpp': '// memory_budget\nint main() { return 0; }\n'}, None, 0, []),
    ('raison emise par MHGP12_CHECK', {'src/core/reasons.def': REASONS, 'src/core/a.hpp': in_namespace(
        'inline int f(bool c) { MHGP12_CHECK(c, memory_budget); return 0; }\n'),
        'tests/core/t.cpp': '// memory_budget\nint main() { return 0; }\n'}, None, 0, []),
    ('raison jamais emise', {'src/core/reasons.def': REASONS,
                             'tests/core/t.cpp': '// memory_budget\nint main() { return 0; }\n'},
     'raison_morte', 1, []),
    ('raison citee en commentaire seulement', {'src/core/reasons.def': REASONS, 'src/core/a.hpp': in_namespace(
        '// fail(Reason::memory_budget)\n'), 'tests/core/t.cpp': '// memory_budget\nint main() { return 0; }\n'},
     'raison_morte', 1, []),
    ('raison sans porte', {'src/core/reasons.def': REASONS, 'src/core/a.hpp': in_namespace(EMIT)},
     'raison_sans_porte', 1, []),
    ('raison d un module absent', {'src/core/reasons.def': REASONS
                                   + 'MHGP12_REASON(empty_input, invalid_input, cloud)\n',
                                   'src/core/a.hpp': in_namespace(EMIT),
                                   'tests/core/t.cpp': '// memory_budget\nint main() { return 0; }\n'}, None, 0, []),
    ('raison sans module', {'src/core/reasons.def': REASONS + 'MHGP12_REASON(empty_input, invalid_input)\n',
                            'src/core/a.hpp': in_namespace(EMIT),
                            'tests/core/t.cpp': '// memory_budget\nint main() { return 0; }\n'}, 'raison_morte', 1, []),
    ('raison d un module hors table', {'src/core/reasons.def': REASONS + 'MHGP12_REASON(x, invalid_input, foo)\n',
                                       'src/core/a.hpp': in_namespace(EMIT),
                                       'tests/core/t.cpp': '// memory_budget\nint main() { return 0; }\n'},
     'raison_morte', 1, []),
    # unites : un ecart de num n'est pas vu depuis core, il l'est depuis num
    ('unite core seule', {'src/num/a.cpp': 'int f() { return 1; }\n'}, None, 0, ['--units', 'core']),
    ('unite num', {'src/num/a.cpp': 'int f() { return 1; }\n'}, 'namespace', 2, ['--units', 'num']),
    ('unite inconnue', {}, 'usage', 0, ['--units', 'absente']),
]


def main():
    if len(sys.argv) != 2:
        print('usage : test_check_style.py <check_style.py>')
        return 2
    checker = os.path.abspath(sys.argv[1])
    real_root = os.path.dirname(os.path.dirname(checker))
    gate = mhgp12_gate.Gate('check_style')
    with tempfile.TemporaryDirectory() as folder:
        count = 0

        def tree(files, table=None):
            nonlocal count
            count += 1
            root = os.path.join(folder, 'arbre_%d' % count)
            os.makedirs(os.path.join(root, 'docs'))
            os.makedirs(os.path.join(root, 'cmake'))
            shutil.copy(os.path.join(real_root, 'docs', 'ARCHITECTURE.md'), os.path.join(root, 'docs'))
            shutil.copy(os.path.join(real_root, 'cmake', 'modules.cmake'), os.path.join(root, 'cmake'))
            content = dict(BASE)
            content.update(files)
            for relative, text in content.items():
                path = os.path.join(root, relative)
                os.makedirs(os.path.dirname(path), exist_ok=True)
                with open(path, 'w', encoding='utf-8') as handle:
                    handle.write(text)
            if table is not None:
                table(root)
            return root

        def judge(name, root, rule, problems, extra):
            result = mhgp12_gate.run([sys.executable, checker, '--root', root] + extra, timeout=60)
            lines = result.stdout.splitlines()
            if rule is None:
                gate.check(result.code == 0, '%s : %s, attendu code 0\n%s' % (name, result.describe(), result.stdout))
                gate.check(bool(lines) and lines[-1].startswith('style_ok fichiers='), '%s : ligne style_ok' % name)
                return
            gate.check(result.code == 1, '%s : %s, attendu code 1\n%s' % (name, result.describe(), result.stdout))
            if rule == 'usage':
                gate.check(bool(lines) and lines[0].startswith('usage :'), '%s : ligne usage' % name)
                return
            tagged = [line for line in lines if '[%s]' % rule in line]
            gate.check(len(tagged) >= 1 and len([line for line in lines if '] ' in line]) == problems,
                       '%s : %d ecart(s) attendu(s), dont [%s]\n%s' % (name, problems, rule, result.stdout))

        for name, files, rule, problems, extra in CASES:
            judge(name, tree(files), rule, problems, extra)

        # table : toute difference entre cmake/modules.cmake et le document est un ecart
        def drop_dependency(root):
            path = os.path.join(root, 'cmake', 'modules.cmake')
            with open(path, encoding='utf-8') as handle:
                text = handle.read()
            with open(path, 'w', encoding='utf-8') as handle:
                handle.write(text.replace('set(MHGP12_DEPS_num core)', 'set(MHGP12_DEPS_num)'))

        def swap_order(root):
            path = os.path.join(root, 'cmake', 'modules.cmake')
            with open(path, encoding='utf-8') as handle:
                text = handle.read()
            with open(path, 'w', encoding='utf-8') as handle:
                handle.write(text.replace('core num sched cloud', 'core sched num cloud'))

        def drop_document(root):
            os.remove(os.path.join(root, 'docs', 'ARCHITECTURE.md'))

        judge('table : dependance retiree', tree({}, drop_dependency), 'table', 1, [])
        judge('table : ordre change', tree({}, swap_order), 'table', 1, [])
        judge('table : document absent', tree({}, drop_document), 'table', 1, [])

        # arbre sans aucun fichier controle : la porte ne passe pas a vide
        empty = os.path.join(folder, 'vide')
        os.makedirs(os.path.join(empty, 'docs'))
        os.makedirs(os.path.join(empty, 'cmake'))
        shutil.copy(os.path.join(real_root, 'docs', 'ARCHITECTURE.md'), os.path.join(empty, 'docs'))
        shutil.copy(os.path.join(real_root, 'cmake', 'modules.cmake'), os.path.join(empty, 'cmake'))
        result = mhgp12_gate.run([sys.executable, checker, '--root', empty], timeout=60)
        gate.check_eq(result.code, 1, 'arbre vide : code 1')
        gate.check('aucun fichier controle' in result.stdout, 'arbre vide : message')

        # l'arbre reel (dont ce fichier et check_style.py) est conforme
        result = mhgp12_gate.run([sys.executable, checker, '--root', real_root, '--units', 'core'], timeout=120)
        gate.check(result.code == 0, 'arbre reel, unite core : %s\n%s' % (result.describe(), result.stdout))
    return gate.finish(FLOOR)


if __name__ == '__main__':
    sys.exit(main())
