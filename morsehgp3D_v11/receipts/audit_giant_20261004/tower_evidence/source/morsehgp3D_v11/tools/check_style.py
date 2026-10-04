#!/usr/bin/env python3
"""Controle des regles de proprete de la v11 qui se lisent dans le texte (docs/ARCHITECTURE.md, paragraphe 1).

    python3 tools/check_style.py [--root <morsehgp3D_v11>] [--units <unite>...]

Sans --units : tout l'arbre. Avec --units (modules de la table, cli, reference) : src/<unite>/ et tests/<unite>/ de
chaque unite, cli/ pour cli, reference/ pour reference ; l'unite core entraine le socle (cmake/, tests/support/,
tests/mutants/, tools/, CMakeLists.txt). Code de sortie : 0 conforme, 1 sinon (une ligne par ecart :
chemin:ligne: [regle] message). Python 3.10 nu, aucun assert.

Regles (nom entre crochets dans la sortie) :
  [table]            cmake/modules.cmake est la copie exacte de la table du paragraphe 2 (modules, ordre, dependances)
  [module]           tout dossier de src/ est un module de la table, avec src/<m>/<m>.hpp et src/<m>/module.cmake
  [taille_fichier]   un fichier de src/ fait au plus 500 lignes
  [taille_fonction]  une fonction de src/ fait au plus 100 lignes (heuristique ci-dessous)
  [accolades]        les accolades d'un fichier de src/ sont equilibrees (sinon l'heuristique ne peut pas le lire)
  [assert]           ni assert(...) ni <cassert> ni <assert.h> dans le C++ de src/, cli/ et tests/ (static_assert reste
                     permis) : sous NDEBUG un assert ne controle rien
  [ascii]            aucun octet non ASCII dans src/ et cli/ (commentaires en francais sans accents)
  [tabulation]       aucune tabulation dans le C++ (deux espaces)
  [entete]           un fichier de src/ ou de cli/ commence par un commentaire qui dit son role
  [namespace]        dans src/, tout le code est dans namespace mhgp11 (fichiers .def exceptes)
  [macro]            dans src/ et cli/, toute macro definie s'appelle MHGP11_...
  [mhgp10]           aucun identifiant de la v10 : MHGP10 partout (C++, CMake), namespace mhgp10 et mhgp10:: dans le C++
  [inclusion]        dans src/ et cli/, une inclusion entre guillemets a la forme "module/fichier" ; un module n'inclut
                     d'un AUTRE module que son en-tete public "module/module.hpp"
  [dependance]       un module n'inclut que lui-meme et les modules dont il depend, directement ou non, d'apres la table
  [python_assert]    aucune instruction assert dans un script Python (une porte tient sous python3 -O) ; un script que
                     l'interprete courant ne sait pas lire est aussi un ecart
  [raison_morte]     chaque raison de src/core/reasons.def nomme un module de la table ; si ce module est present,
                     une emission Reason::<nom> (ou MHGP11_CHECK(..., <nom>)) existe dans src/<module>/
  [raison_sans_porte] si ce module est present, le nom de la raison apparait dans un fichier de tests/<module>/
                     (une raison n'entre dans la table qu'avec la porte qui la provoque)

Heuristique de [taille_fonction]. Commentaires, chaines, caracteres et lignes de preprocesseur sont d'abord effaces.
Une accolade ouvrante rencontree dans la portee d'un namespace ou d'une classe ouvre :
  - un namespace ou une classe (struct, union) si le texte qui la precede depuis la derniere limite (';', '}', '{',
    public: / private: / protected:) le dit : la portee est parcourue a son tour ;
  - un enum ou un initialiseur (aucune parenthese avant l'accolade) : le bloc est saute ;
  - sinon un corps de fonction (le texte precedent contient une parenthese), y compris une lambda de portee
    namespace. Dans une liste d'initialisation de constructeur (un ':' seul apres la liste des parametres), une
    accolade collee a un identifiant initialise un membre : elle est sautee, le corps est l'accolade suivante.
La longueur va de la premiere ligne de la signature a l'accolade fermante, lignes vides et commentaires compris.
Limites connues : une accolade ouverte dans une branche de preprocesseur et fermee dans une autre n'est pas lisible
(ecart [accolades]) ; une macro qui cache une accolade n'est pas vue.
"""
import argparse
import ast
import os
import re
import sys

MAX_FILE_LINES = 500
MAX_FUNCTION_LINES = 100
CPP_SUFFIXES = ('.hpp', '.cpp', '.def')
EXTRA_UNITS = ('cli', 'reference')


# ---------------------------------------------------------------- table des modules
def read_architecture_table(root, problems):
    """Table du paragraphe 2 : liste ordonnee des modules et dependances directes ('tous' : les modules precedents)."""
    path = os.path.join(root, 'docs', 'ARCHITECTURE.md')
    try:
        with open(path, encoding='utf-8') as handle:
            lines = handle.read().splitlines()
    except OSError as error:
        problems.append((path, 0, 'table', 'document illisible : %s' % error))
        return [], {}
    order, deps, in_table = [], {}, False
    for line in lines:
        if re.match(r'^\|\s*Module\s*\|', line):
            in_table = True
            continue
        if not in_table:
            continue
        if not line.startswith('|'):
            break
        cells = [cell.strip() for cell in line.strip().strip('|').split('|')]
        if len(cells) != 3 or set(cells[0]) <= set('- '):
            continue
        name = cells[0].strip('`')
        listed = re.findall(r'`([a-z_0-9]+)`', cells[2])
        deps[name] = list(order) if cells[2].strip().lower() == 'tous' else listed
        order.append(name)
    if len(order) < 2 or 'core' not in order:
        problems.append((path, 0, 'table', 'table des modules du paragraphe 2 introuvable'))
        return [], {}
    for name in order:
        for dep in deps[name]:
            if dep not in order or order.index(dep) >= order.index(name):
                problems.append((path, 0, 'table',
                                 'module %s : dependance %s absente ou placee apres lui' % (name, dep)))
    return order, deps


def check_cmake_table(root, order, deps, problems):
    path = os.path.join(root, 'cmake', 'modules.cmake')
    try:
        with open(path, encoding='utf-8') as handle:
            text = handle.read()
    except OSError as error:
        problems.append((path, 0, 'table', 'fichier illisible : %s' % error))
        return
    found = re.search(r'^set\(MHGP11_ALL_MODULES([^)]*)\)', text, re.M)
    cmake_order = found.group(1).split() if found else []
    if cmake_order != order:
        problems.append((path, 0, 'table', 'MHGP11_ALL_MODULES = %s, document = %s' % (cmake_order, order)))
    for name in order:
        found = re.search(r'^set\(MHGP11_DEPS_%s\b([^)]*)\)' % re.escape(name), text, re.M)
        cmake_deps = found.group(1).split() if found else None
        if cmake_deps is None or sorted(cmake_deps) != sorted(deps[name]):
            problems.append((path, 0, 'table', 'MHGP11_DEPS_%s = %s, document = %s' % (name, cmake_deps, deps[name])))
    for name in re.findall(r'^set\(MHGP11_DEPS_([a-z_0-9]+)', text, re.M):
        if name not in order:
            problems.append((path, 0, 'table', 'MHGP11_DEPS_%s : module absent du document' % name))


def closure(name, deps):
    seen, pending = set(), [name]
    while pending:
        current = pending.pop()
        if current not in seen:
            seen.add(current)
            pending.extend(deps.get(current, []))
    return seen


# ---------------------------------------------------------------- lecture du C++
def strip_code(text):
    """Efface commentaires, chaines, caracteres et lignes de preprocesseur (fins de ligne conservees)."""
    out, i, n = [], 0, len(text)
    line_start = True

    def blank(upto):
        nonlocal i
        while i < upto:
            out.append('\n' if text[i] == '\n' else ' ')
            i += 1

    while i < n:
        c = text[i]
        if c == '\n':
            out.append(c)
            i += 1
            line_start = True
        elif c in ' \t':
            out.append(c)
            i += 1
        elif line_start and c == '#':
            end = i
            while end < n and not (text[end] == '\n' and text[end - 1] != '\\'):
                end += 1
            blank(end)
        elif text.startswith('//', i):
            end = text.find('\n', i)
            blank(n if end < 0 else end)
        elif text.startswith('/*', i):
            end = text.find('*/', i + 2)
            blank(n if end < 0 else end + 2)
        elif c == '"':
            line_start = False
            raw = re.search(r'(?<![A-Za-z0-9_])(?:u8|u|U|L)?R$', text[max(0, i - 3):i])
            if raw:
                delimiter = re.match(r'"([^()\\ ]{0,16})\(', text[i:])
                close = text.find(')' + delimiter.group(1) + '"', i) if delimiter else -1
                blank(n if close < 0 else close + len(delimiter.group(1)) + 2)
            else:
                end = i + 1
                while end < n and text[end] != '"' and text[end] != '\n':
                    end += 2 if text[end] == '\\' else 1
                blank(min(n, end + 1))
        elif c == "'":
            line_start = False
            start = i
            while start > 0 and (text[start - 1].isalnum() or text[start - 1] in '_.'):
                start -= 1
            if start < i and text[start].isdigit():  # separateur de chiffres : 1'000'000
                out.append(' ')
                i += 1
            else:
                end = i + 1
                while end < n and text[end] != "'" and text[end] != '\n':
                    end += 2 if text[end] == '\\' else 1
                blank(min(n, end + 1))
        else:
            line_start = False
            out.append(c)
            i += 1
    return ''.join(out)


def match_brace(code, opening):
    """Indice de l'accolade fermante du bloc ouvert en `opening`, ou -1."""
    depth = 0
    for position in range(opening, len(code)):
        if code[position] == '{':
            depth += 1
        elif code[position] == '}':
            depth -= 1
            if depth == 0:
                return position
    return -1


def has_initializer_list(head):
    """Vrai si un ':' seul (ni '::') suit la premiere liste de parametres : liste d'initialisation de constructeur."""
    opening = head.find('(')
    depth, position = 0, opening
    while position < len(head):
        if head[position] == '(':
            depth += 1
        elif head[position] == ')':
            depth -= 1
            if depth == 0:
                break
        position += 1
    depth = 0
    for index in range(position + 1, len(head)):
        char = head[index]
        if char in '([':
            depth += 1
        elif char in ')]':
            depth -= 1
        elif char == ':' and depth == 0:
            if head[index - 1] != ':' and (index + 1 >= len(head) or head[index + 1] != ':'):
                return True
    return False


def classify(head):
    """Nature du bloc qu'ouvre une accolade precedee de `head` : namespace, class, skip (enum, initialiseur),
    member (initialisation d'un membre) ou function."""
    text = head.strip()
    if re.fullmatch(r'extern', text):
        return 'namespace'
    if '(' not in text:
        if re.search(r'\bnamespace\b', text):
            return 'namespace'
        if re.search(r'\benum\b', text):
            return 'skip'
        if re.search(r'\b(class|struct|union)\b', text) and not text.endswith('='):
            return 'class'
        return 'skip'
    if re.search(r'\benum\b', text) and not re.search(r'\)\s*$', text):
        return 'skip'
    if re.match(r'^(template\s*<.*>\s*)?(\[\[.*?\]\]\s*)?(class|struct|union)\b', text, re.S):
        return 'class'
    if has_initializer_list(text) and re.search(r'[A-Za-z0-9_>]$', text):
        return 'member'
    return 'function'


def scan_blocks(code, top_level_namespace):
    """Parcourt le code efface. Rend (fonctions, ecarts) : fonctions = [(premiere ligne, derniere ligne)] ; ecarts =
    [(ligne, regle, message)] pour le code hors namespace (si top_level_namespace) et les accolades."""
    functions, problems = [], []
    line_of = [1] * (len(code) + 1)
    for index, char in enumerate(code):
        line_of[index + 1] = line_of[index] + (1 if char == '\n' else 0)
    scopes = []  # 'namespace' ou 'class' ; vide : portee globale
    head, head_start, parens, i = '', -1, 0, 0
    outside_reported = False

    def outside(position, what):
        nonlocal outside_reported
        if top_level_namespace and not scopes and not outside_reported:
            outside_reported = True
            problems.append((line_of[position], 'namespace', '%s hors de namespace mhgp11' % what))

    while i < len(code):
        char = code[i]
        if char in '([':
            parens += 1
        elif char in ')]':
            parens -= 1
        if char == '{' and parens > 0:  # accolade dans une parenthese : lambda ou liste en argument
            close = match_brace(code, i)
            if close < 0:
                problems.append((line_of[i], 'accolades', 'accolade ouvrante sans fermante'))
                return functions, problems
            head += '{}'
            i = close + 1
            continue
        if char == '{':
            kind = classify(head)
            start = head_start if head_start >= 0 else i
            if kind == 'namespace':
                name = re.search(r'\bnamespace\s+([A-Za-z_][\w:]*)', head)
                if top_level_namespace and not scopes:
                    if not name or not re.fullmatch(r'mhgp11(::\w+)*', name.group(1)):
                        problems.append((line_of[i], 'namespace', 'namespace de premier niveau autre que mhgp11'))
                scopes.append('namespace')
                head, head_start = '', -1
                i += 1
                continue
            if kind == 'class':
                outside(start, 'type')
                scopes.append('class')
                head, head_start = '', -1
                i += 1
                continue
            close = match_brace(code, i)
            if close < 0:
                problems.append((line_of[i], 'accolades', 'accolade ouvrante sans fermante'))
                return functions, problems
            if kind == 'member':
                head += '{}'
            else:
                outside(start, 'code')
                if kind == 'function':
                    functions.append((line_of[start], line_of[close]))
                head, head_start = '', -1
            i = close + 1
            continue
        if char == '}':
            if not scopes:
                problems.append((line_of[i], 'accolades', 'accolade fermante sans ouvrante'))
                return functions, problems
            scopes.pop()
            head, head_start = '', -1
        elif char == ';' and parens == 0:
            if head.strip():
                outside(head_start, 'declaration')
            head, head_start = '', -1
        elif char == ':' and scopes and scopes[-1] == 'class' and head.strip() in ('public', 'private', 'protected'):
            head, head_start = '', -1
        else:
            if head_start < 0 and not char.isspace():
                head_start = i
            head += char
        i += 1
    if scopes:
        problems.append((line_of[len(code)], 'accolades', 'portee non fermee en fin de fichier'))
    return functions, problems


# ---------------------------------------------------------------- regles par fichier
def check_cpp(root, relative, area, module, order, deps, problems):
    """area : 'src', 'cli' ou 'tests'."""
    path = os.path.join(root, relative)
    with open(path, 'rb') as handle:
        data = handle.read()

    def report(line, rule, message):
        problems.append((relative, line, rule, message))

    product = area in ('src', 'cli')
    if product:
        for number, raw_line in enumerate(data.split(b'\n'), 1):
            if any(byte > 0x7F for byte in raw_line):
                report(number, 'ascii', 'octet non ASCII')
                break
    text = data.decode('utf-8', errors='replace')
    lines = text.split('\n')
    count = len(lines) - 1 if text.endswith('\n') else len(lines)
    for number, line in enumerate(lines, 1):
        if '\t' in line:
            report(number, 'tabulation', 'tabulation (deux espaces attendus)')
            break
    if product and not text.lstrip().startswith('//'):
        report(1, 'entete', 'le fichier ne commence pas par un commentaire qui dit son role')
    if area == 'src' and count > MAX_FILE_LINES:
        report(count, 'taille_fichier', '%d lignes, au plus %d' % (count, MAX_FILE_LINES))
    for number, line in enumerate(lines, 1):
        if 'MHGP10' in line:
            report(number, 'mhgp10', 'identifiant de la v10 : MHGP10')
        if re.match(r'\s*#\s*include\s*[<"](cassert|assert\.h)[>"]', line):
            report(number, 'assert', 'inclusion de %s' % re.search(r'[<"](.*)[>"]', line).group(1))
        define = re.match(r'\s*#\s*define\s+([A-Za-z_]\w*)', line)
        if product and define and not define.group(1).startswith('MHGP11_'):
            report(number, 'macro', 'macro %s : le nom doit commencer par MHGP11_' % define.group(1))
        include = re.match(r'\s*#\s*include\s*"([^"]*)"', line)
        if product and include:
            check_include(include.group(1), area, module, order, deps, number, report)

    code = strip_code(text)
    for number, line in enumerate(code.split('\n'), 1):
        if re.search(r'(?<![A-Za-z0-9_])assert\s*\(', line):
            report(number, 'assert', 'assert(...) : une precondition violee rend un refus, jamais assert')
        if re.search(r'\bnamespace\s+mhgp10\b|\bmhgp10::', line):
            report(number, 'mhgp10', 'namespace de la v10')
    if area == 'src' and not relative.endswith('.def'):
        functions, found = scan_blocks(code, True)
        for line, rule, message in found:
            report(line, rule, message)
        for first, last in functions:
            if last - first + 1 > MAX_FUNCTION_LINES:
                report(first, 'taille_fonction', '%d lignes, au plus %d' % (last - first + 1, MAX_FUNCTION_LINES))


def check_include(target, area, module, order, deps, number, report):
    parts = target.split('/')
    if len(parts) != 2 or parts[0] not in order:
        if area == 'src':
            report(number, 'inclusion', '"%s" : forme "module/fichier" attendue' % target)
        return
    other = parts[0]
    if area == 'src' and other == module:
        return
    if parts[1] != other + '.hpp':
        report(number, 'inclusion', '"%s" : seul l\'en-tete public "%s/%s.hpp" s\'inclut hors du module %s'
               % (target, other, other, other))
    if area == 'src' and other not in closure(module, deps):
        report(number, 'dependance', 'le module %s ne depend pas de %s (table du paragraphe 2)' % (module, other))


def check_python(root, relative, problems):
    path = os.path.join(root, relative)
    try:
        with open(path, encoding='utf-8') as handle:
            tree = ast.parse(handle.read(), filename=relative)
    except (OSError, SyntaxError, ValueError) as error:
        problems.append((relative, getattr(error, 'lineno', 0) or 0, 'python_assert',
                         'script illisible par python %d.%d : %s' % (sys.version_info[0], sys.version_info[1], error)))
        return
    for node in ast.walk(tree):
        if isinstance(node, ast.Assert):
            problems.append((relative, node.lineno, 'python_assert', 'instruction assert'))


def check_cmake(root, relative, problems):
    with open(os.path.join(root, relative), encoding='utf-8', errors='replace') as handle:
        for number, line in enumerate(handle, 1):
            if 'MHGP10' in line:
                problems.append((relative, number, 'mhgp10', 'identifiant de la v10 : MHGP10'))


def module_code(root, module):
    """Code C++ de src/<module>/ (hors .def), commentaires et chaines effaces."""
    parts = []
    for relative in walk(root, os.path.join('src', module)):
        if relative.endswith(('.hpp', '.cpp')):
            with open(os.path.join(root, relative), encoding='utf-8', errors='replace') as handle:
                parts.append(strip_code(handle.read()))
    return '\n'.join(parts)


def tests_text(root, module):
    """Texte de tous les fichiers de tests/<module>/."""
    parts = []
    folder = os.path.join('tests', module)
    if os.path.isdir(os.path.join(root, folder)):
        for relative in walk(root, folder):
            with open(os.path.join(root, relative), encoding='utf-8', errors='replace') as handle:
                parts.append(handle.read())
    return '\n'.join(parts)


def check_reasons(root, order, units, everything, problems):
    """Table des raisons : chaque raison est emise par le module qu'elle nomme et citee par ses portes."""
    table = os.path.join('src', 'core', 'reasons.def')
    try:
        with open(os.path.join(root, table), encoding='utf-8') as handle:
            lines = handle.read().splitlines()
    except OSError:
        return
    code, tests = {}, {}
    for number, line in enumerate(lines, 1):
        found = re.match(r'MHGP11_REASON\((\w+),\s*(\w+)(?:,\s*(\w+))?\)', line)
        if not found:
            continue
        name, module = found.group(1), found.group(3)
        if module is None or module not in order:
            problems.append((table, number, 'raison_morte', 'raison %s : module emetteur absent de la table' % name))
            continue
        judged = everything or 'core' in units or module in units
        if name == 'none' or not judged or not os.path.isdir(os.path.join(root, 'src', module)):
            continue
        if module not in code:
            code[module], tests[module] = module_code(root, module), tests_text(root, module)
        emission = r'\bReason::%s\b|MHGP11_CHECK\([^;]*,\s*%s\s*\)' % (name, name)
        if not re.search(emission, code[module]):
            problems.append((table, number, 'raison_morte', 'raison %s : jamais emise dans src/%s/' % (name, module)))
        if not re.search(r'\b%s\b' % name, tests[module]):
            problems.append((table, number, 'raison_sans_porte',
                             'raison %s : citee par aucun fichier de tests/%s/' % (name, module)))


# ---------------------------------------------------------------- parcours
def walk(root, top):
    """Fichiers sous root/top, chemins relatifs a root, tries ; dossiers caches et __pycache__ exclus."""
    found = []
    for folder, folders, files in os.walk(os.path.join(root, top)):
        folders[:] = sorted(name for name in folders if not name.startswith('.') and name != '__pycache__')
        for name in sorted(files):
            found.append(os.path.relpath(os.path.join(folder, name), root))
    return found


def main():
    parser = argparse.ArgumentParser(description='Regles de proprete de la v11 lisibles dans le texte.')
    parser.add_argument('--root', default=os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
    parser.add_argument('--units', nargs='*', default=[])
    args = parser.parse_args()
    root = os.path.abspath(args.root)
    problems = []
    order, deps = read_architecture_table(root, problems)
    if not order:
        return finish(problems, 0)
    for unit in args.units:
        if unit not in order and unit not in EXTRA_UNITS:
            print('usage : unite inconnue %s (admis : %s)' % (unit, ' '.join(order + list(EXTRA_UNITS))))
            return 1
    everything = not args.units
    socle = everything or 'core' in args.units

    if socle:
        check_cmake_table(root, order, deps, problems)
    check_reasons(root, order, args.units, everything, problems)
    src = os.path.join(root, 'src')
    present = sorted(name for name in os.listdir(src) if os.path.isdir(os.path.join(src, name))) \
        if os.path.isdir(src) else []
    checked = 0
    for module in present:
        if not everything and module not in args.units:
            continue
        if module not in order:
            problems.append((os.path.join('src', module), 0, 'module', 'dossier de src/ hors de la table des modules'))
            continue
        for required in (module + '.hpp', 'module.cmake'):
            if not os.path.isfile(os.path.join(src, module, required)):
                problems.append((os.path.join('src', module), 0, 'module', 'fichier requis absent : %s' % required))
        for relative in walk(root, os.path.join('src', module)):
            if relative.endswith(CPP_SUFFIXES):
                check_cpp(root, relative, 'src', module, order, deps, problems)
                checked += 1
            elif relative.endswith('.cmake'):
                check_cmake(root, relative, problems)
    tops = []
    if everything or 'cli' in args.units:
        tops.append(('cli', 'cli'))
    for unit in (present + ['cli'] if everything else args.units):
        tops.append((os.path.join('tests', unit), 'tests'))
    if everything or 'reference' in args.units:
        tops.append(('reference', 'tests'))
    if socle:
        tops += [(os.path.join('tests', 'support'), 'tests'), (os.path.join('tests', 'mutants'), 'tests'),
                 ('tools', 'tests'), ('cmake', 'tests')]
    if everything:
        tops += [('tests', 'tests'), ('bench', 'tests')]
    seen = set()
    for top, area in tops:
        if not os.path.isdir(os.path.join(root, top)):
            continue
        for relative in walk(root, top):
            if relative in seen:
                continue
            seen.add(relative)
            if relative.endswith(('.hpp', '.cpp')):
                check_cpp(root, relative, area, None, order, deps, problems)
                checked += 1
            elif relative.endswith('.py'):
                check_python(root, relative, problems)
                checked += 1
            elif relative.endswith('.cmake'):
                check_cmake(root, relative, problems)
    if socle and os.path.isfile(os.path.join(root, 'CMakeLists.txt')):
        check_cmake(root, 'CMakeLists.txt', problems)
    return finish(problems, checked)


def finish(problems, checked):
    for path, line, rule, message in sorted(problems, key=lambda item: (str(item[0]), item[1], item[2])):
        print('%s:%d: [%s] %s' % (path, line, rule, message))
    if problems:
        print('style_ecarts %d (fichiers controles : %d)' % (len(problems), checked))
        return 1
    if checked == 0:
        print('style_ecarts 1 : aucun fichier controle (porte vide)')
        return 1
    print('style_ok fichiers=%d' % checked)
    return 0


if __name__ == '__main__':
    sys.exit(main())
