"""Assemble le rapport L08 depuis ses fragments, remplit les mesures, controle la forme Markdown."""
import json
import re
import sys

D = '/tmp/v11-audit/l08_tests_portes/rapport/'
ORDER = ['A_entete.md', 'B_execution.md', 'C_portes.md', 'C2_modules.md', 'D_oracles.md', 'E_juge.md', 'F_extensions.md',
         'F2_echelle.md', 'G_tete_process.md', 'H_solide.md', 'I_pas_refaire.md', 'J_questions.md', 'K_plan.md',
         'L_fin.md']
vals = json.load(open(D + 'valeurs.json'))
text = ''.join(open(D + f).read() for f in ORDER)
for k, v in vals.items():
    text = text.replace('@@%s@@' % k, v)
# renumerotation des constats dans l'ordre du texte (15 -> 10, 10 -> 11, 11 -> 12, 16 -> 13, 12 -> 14, 13 -> 15, 14 -> 16)
REN = {'15': '10', '10': '11', '11': '12', '16': '13', '12': '14', '13': '15', '14': '16'}
for prefix in ('L08_TESTS_PORTES-', 'L08-'):
    for old in REN:
        text = text.replace(prefix + old, prefix + '<<' + old + '>>')
    for old, new in REN.items():
        text = text.replace(prefix + '<<' + old + '>>', prefix + new)
left = sorted(set(re.findall(r'@@[A-Z0-9_]+@@', text)))
problems = []
if left:
    problems.append('marqueurs restants : %s' % left)
for i, line in enumerate(text.split('\n'), 1):
    if line.count('$') % 2:
        problems.append('ligne %d : nombre impair de $' % i)
    for bad in ('\\operatorname', '\\left\\|', '\\left\\{', '\\right\\|', '\\right\\}'):
        if bad in line:
            problems.append('ligne %d : %s interdit' % (i, bad))
    if re.search(r'\]\((?!https?://)', line):
        problems.append('ligne %d : lien relatif ou prive' % i)
if problems:
    print('\n'.join(problems[:40]))
out = sys.argv[1]
open(out, 'w').write(text)
print('ecrit', out, len(text), 'octets', len(text.split('\n')), 'lignes ; problemes', len(problems))
