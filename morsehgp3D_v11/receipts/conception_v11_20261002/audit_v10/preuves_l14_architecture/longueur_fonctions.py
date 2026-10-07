#!/usr/bin/env python3
"""Longueur des fonctions C++ (heuristique par appariement d'accolades, commentaires et chaines retires).

Usage : longueur_fonctions.py RACINE [seuil]
Sortie : une ligne « lignes<TAB>fichier:debut-fin<TAB>signature » par bloc de fonction de longueur >= seuil,
triee par longueur decroissante, puis un resume par fichier. Un bloc est compte comme fonction si son en-tete
contient une parenthese et ne commence pas par namespace/struct/class/enum/union ; les lambdas affectees a une
variable locale sont comptees dans la fonction qui les contient (elles ne sont pas des blocs de premier niveau).
"""
import os
import re
import sys


def strip(src):
    """Remplace commentaires et litteraux par des espaces, en gardant les sauts de ligne."""
    out = []
    i, n = 0, len(src)
    while i < n:
        c = src[i]
        if src.startswith('//', i):
            j = src.find('\n', i)
            j = n if j < 0 else j
            out.append(' ' * (j - i))
            i = j
        elif src.startswith('/*', i):
            j = src.find('*/', i + 2)
            j = n if j < 0 else j + 2
            out.append(''.join(ch if ch == '\n' else ' ' for ch in src[i:j]))
            i = j
        elif c == '"' or c == "'":
            j = i + 1
            while j < n and src[j] != c:
                j += 2 if src[j] == '\\' else 1
            j = min(n, j + 1)
            out.append(c + ' ' * (j - i - 2) + c if j - i >= 2 else c)
            i = j
        else:
            out.append(c)
            i += 1
    return ''.join(out)


CONTAINER = re.compile(r'^\s*(?:template\s*<[^{}]*>\s*)?(?:namespace|struct|class|enum|union|extern)\b')


def functions(path):
    src = open(path, encoding='utf-8', errors='replace').read()
    s = strip(src)
    line_of = [1]
    for ch in s:
        line_of.append(line_of[-1] + (ch == '\n'))
    res = []
    stack = []  # (kind, start_index, header)
    last_boundary = 0
    for i, ch in enumerate(s):
        if ch == '{':
            header = s[last_boundary:i]
            head = ' '.join(header.split())
            inside_function = any(k == 'fn' for k, _, _ in stack)
            if inside_function:
                kind = 'inner'
            elif CONTAINER.match(header) and '(' not in head.split('{')[0].split(':')[0][:0] and not re.search(r'\)\s*(const)?\s*(noexcept)?\s*(->[^{]*)?$', head):
                kind = 'container'
            elif '(' in head:
                kind = 'fn'
            else:
                kind = 'container'
            stack.append((kind, i, head))
            last_boundary = i + 1
        elif ch == '}':
            if stack:
                kind, start, head = stack.pop()
                if kind == 'fn':
                    a, b = line_of[start], line_of[i]
                    # debut reel : premiere ligne de l'en-tete
                    hstart = line_of[start - len(s[:start]) + s.rfind(head.split(' ')[0], 0, start)] if head else a
                    res.append((b - min(a, hstart) + 1, min(a, hstart), b, head[-110:]))
            last_boundary = i + 1
        elif ch == ';':
            if not any(k in ('fn', 'inner') for k, _, _ in stack):
                last_boundary = i + 1
    return res, src.count('\n') + (0 if src.endswith('\n') else 1)


def main():
    root = sys.argv[1]
    seuil = int(sys.argv[2]) if len(sys.argv) > 2 else 60
    rows, per_file = [], []
    for d, _, fs in sorted(os.walk(root)):
        for f in sorted(fs):
            if not f.endswith(('.cpp', '.hpp', '.h')):
                continue
            p = os.path.join(d, f)
            fns, total = functions(p)
            rel = os.path.relpath(p, root)
            per_file.append((total, rel, len(fns), max([x[0] for x in fns], default=0)))
            for ln, a, b, head in fns:
                if ln >= seuil:
                    rows.append((ln, '%s:%d-%d' % (rel, a, b), head))
    for ln, where, head in sorted(rows, reverse=True):
        print('%d\t%s\t%s' % (ln, where, head))
    print('--- par fichier : lignes, fichier, fonctions, plus longue fonction')
    for total, rel, nf, mx in sorted(per_file, reverse=True):
        print('%d\t%s\t%d\t%d' % (total, rel, nf, mx))


if __name__ == '__main__':
    main()
