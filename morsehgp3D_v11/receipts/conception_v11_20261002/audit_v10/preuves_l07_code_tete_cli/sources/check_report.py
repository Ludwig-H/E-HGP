"""Controle local du rapport selon les regles de tools/check_docs.py (reprises telles quelles), plus : dollars
equilibres par ligne physique, aucun lien Markdown relatif."""
import re
import sys

MARKDOWN_LINK = re.compile(r"!?\[[^\]]*\]\(([^)\s]+)")
INLINE_CODE = re.compile(r"`[^`]*`")
EXPLICIT_BRACES = re.compile(r"\\(?:mathbb|mathbf|frac|sqrt)(?!\{)")
BANNED = (r"\operatorname", r"\left\|", r"\right\|", r"\left\{", r"\right\}")
AMPUTATED = (re.compile(r"(?<!\\)\bqquad\b"), re.compile(r"(?<!\\)\bpi_0\b"), re.compile(r"(?<!\\)\bmathrm\b"))
errors = []
in_fence = False
for n, line in enumerate(open(sys.argv[1], encoding='utf-8').read().splitlines(), 1):
    if line.strip().startswith('```'):
        in_fence = not in_fence
        continue
    if in_fence:
        continue
    if line.endswith((' ', '\t')):
        errors.append('%d: espace en fin de ligne' % n)
    if '\t' in line or any(ord(c) < 32 for c in line):
        errors.append('%d: tabulation ou caractere de controle' % n)
    lint = INLINE_CODE.sub('', line)
    if lint.count('$$') not in (0, 2):
        errors.append('%d: $$ non ferme sur la ligne' % n)
    if lint.replace('$$', '').count('$') % 2:
        errors.append('%d: $ non equilibre sur la ligne' % n)
    for t in BANNED:
        if t in lint:
            errors.append('%d: jeton banni %s' % (n, t))
    for p in AMPUTATED:
        if p.search(lint):
            errors.append('%d: commande LaTeX amputee %s' % (n, p.pattern))
    if EXPLICIT_BRACES.search(lint):
        errors.append('%d: accolades explicites exigees' % n)
    for m in MARKDOWN_LINK.finditer(line):
        errors.append('%d: lien Markdown %s' % (n, m.group(1)))
if in_fence:
    errors.append('bloc de code non ferme')
print('\n'.join(errors) if errors else 'rapport conforme aux regles Markdown')
sys.exit(1 if errors else 0)
