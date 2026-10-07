#!/usr/bin/env python3
"""Memes regles que tools/check_docs.py (validate), appliquees a un fichier quelconque, plus : aucun lien Markdown."""
import re
import sys

MARKDOWN_LINK = re.compile(r"!?\[[^\]]*\]\(([^)\s]+)")
INLINE_CODE = re.compile(r"`[^`]*`")
EXPLICIT_BRACES = re.compile(r"\\(?:mathbb|mathbf|frac|sqrt)(?!\{)")
BANNED = (r"\operatorname", r"\left\|", r"\right\|", r"\left\{", r"\right\}")
AMPUTATED = (re.compile(r"(?<!\\)\bqquad\b"), re.compile(r"(?<!\\)\bpi_0\b"), re.compile(r"(?<!\\)\bmathrm\b"))
errors = []
in_fence = False
for number, line in enumerate(open(sys.argv[1], encoding='utf-8').read().splitlines(), start=1):
    if line.strip().startswith("```"):
        in_fence = not in_fence
        continue
    if in_fence:
        continue
    if line.endswith((" ", "\t")):
        errors.append("%d: espace en fin de ligne" % number)
    if "\t" in line or any(ord(ch) < 32 for ch in line):
        errors.append("%d: tabulation ou caractere de controle" % number)
    lint = INLINE_CODE.sub("", line)
    if lint.count("$$") not in (0, 2):
        errors.append("%d: equation $$ sur plusieurs lignes" % number)
    if lint.count("$") % 2:
        errors.append("%d: nombre impair de $" % number)
    for t in BANNED:
        if t in lint:
            errors.append("%d: jeton interdit %s" % (number, t))
    for p in AMPUTATED:
        if p.search(lint):
            errors.append("%d: barre oblique manquante" % number)
    if EXPLICIT_BRACES.search(lint):
        errors.append("%d: accolades explicites exigees" % number)
    for m in MARKDOWN_LINK.finditer(line):
        errors.append("%d: lien Markdown %r" % (number, m.group(1)))
if in_fence:
    errors.append("bloc de code non ferme")
print('lint', sys.argv[1], ':', len(errors), 'erreur(s)')
for e in errors:
    print(' ', e)
sys.exit(1 if errors else 0)
