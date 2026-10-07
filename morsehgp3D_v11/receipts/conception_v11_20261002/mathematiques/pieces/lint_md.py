#!/usr/bin/env python3
"""Controle de forme d'un document Markdown de la v11 : regles de tools/check_docs.py (equations sur une ligne
physique, accolades explicites, jetons LaTeX bannis, espaces finaux, tabulations) et controles supplementaires
(nombre pair de $ par ligne hors code, pas de lettre accentuee dans une formule hors \\text{...}).

Usage : python3 -B lint_md.py FICHIER.md [...]
"""
import re
import sys

INLINE_CODE = re.compile(r"`[^`]*`")
EXPLICIT_BRACES = re.compile(r"\\(?:mathbb|mathbf|frac|sqrt)(?!\{)")
BANNED = (r"\operatorname", r"\left\|", r"\right\|", r"\left\{", r"\right\}")
AMPUTATED = (re.compile(r"(?<!\\)\bqquad\b"), re.compile(r"(?<!\\)\bpi_0\b"), re.compile(r"(?<!\\)\bmathrm\b"))
TEXT = re.compile(r"\\text\{[^{}]*\}")
ACCENT = re.compile(r"[àâäçéèêëîïôöùûüÿœÀÂÄÇÉÈÊËÎÏÔÖÙÛÜŸŒ]")


def lint(path):
    errors = []
    in_fence = False
    for number, line in enumerate(open(path, encoding='utf-8').read().splitlines(), start=1):
        if line.strip().startswith('```'):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        if line.endswith((' ', '\t')):
            errors.append('%d: espace final' % number)
        if '\t' in line or any(ord(ch) < 32 for ch in line):
            errors.append('%d: tabulation ou caractere de controle' % number)
        body = INLINE_CODE.sub('', line)
        if body.count('$$') not in (0, 2):
            errors.append('%d: bloc $$ non ferme sur la ligne' % number)
        for token in BANNED:
            if token in body:
                errors.append('%d: jeton banni %s' % (number, token))
        for pattern in AMPUTATED:
            if pattern.search(body):
                errors.append('%d: contre-oblique manquante devant %s' % (number, pattern.search(body).group(0)))
        if EXPLICIT_BRACES.search(body):
            errors.append('%d: accolades exigees apres mathbb, mathbf, frac, sqrt' % number)
        single = body.replace('$$', '')
        if single.count('$') % 2:
            errors.append('%d: nombre impair de $' % number)
        # formules : segments entre $ ... $ (apres retrait des blocs $$ ... $$ traites a part)
        segments = []
        for block in re.findall(r"\$\$(.*?)\$\$", body):
            segments.append(block)
        parts = re.sub(r"\$\$.*?\$\$", '', body).split('$')
        segments += parts[1::2]
        for seg in segments:
            bare = TEXT.sub('', seg)
            if ACCENT.search(bare):
                errors.append('%d: lettre accentuee dans une formule hors \\text{} : %s' % (number, seg[:60]))
            if '|' in bare:
                errors.append('%d: barre verticale dans une formule (tableaux) : %s' % (number, seg[:60]))
    if in_fence:
        errors.append('bloc de code non ferme')
    return errors


def main():
    bad = 0
    for path in sys.argv[1:]:
        errors = lint(path)
        bad += len(errors)
        print('%s : %d ligne(s), %d erreur(s)' % (path, sum(1 for _ in open(path, encoding='utf-8')), len(errors)))
        for e in errors:
            print('  ' + e)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
