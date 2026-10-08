#!/usr/bin/env python3
"""Mutants du pilote MES-C : chaque mutant est applique a une copie de pilote_c.py dans un dossier temporaire qui
reproduit la disposition du depot (mes_c_petits/ et outils/), la porte test_pilote_c.py y est rejouee et doit echouer
(mutant tue). Un mutant dont le texte d'origine n'apparait pas exactement une fois est invalide (ecart). Python 3.10
nu, aucun assert. Codes : 0 tous tues ; 1 un mutant survit ou est invalide.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
MUTANTS = {
    'c1_seuil': ("FIXED_LIMIT_NS = 2e6", "FIXED_LIMIT_NS = 0.5e6"),
    'c2_seuil': ("MAIN_REGIME_NS_PER_SITE = 241.3e6 / 64740", "MAIN_REGIME_NS_PER_SITE = 241.3e6 / 164740"),
    'c3_refus_ignores': ("for h in k5 if h['etat'] != 'ok']", "for h in k5 if h['etat'] == 'echec']"),
    'droite_melangee': ("            groups.setdefault(v['groupe'], []).append((v['sites'], v['chaud_ns']))",
                        "            groups.setdefault('reel', []).append((v['sites'], v['chaud_ns']))"),
    'chaud_avec_froid': ("        warm = [p['wall_ns'] for p in mine[1:]]",
                         "        warm = [p['wall_ns'] for p in mine]"),
    'empreinte_non_controlee': ("            if len(v['empreintes']) != 1:",
                                "            if len(v['empreintes']) > 2:"),
    'difficile_dans_session': ("    session = [c for c in clouds if c['groupe'] not in HARD]",
                               "    session = list(clouds)"),
    'archive_admise': ("        print('pilote_c : archive ou manifeste refuses', file=sys.stderr)\n        return 2",
                       "        clouds = []"),
}


def main():
    with open(os.path.join(HERE, 'pilote_c.py'), encoding='utf-8') as handle:
        original = handle.read()
    bad = []
    for name, (old, new) in MUTANTS.items():
        if original.count(old) != 1:
            bad.append('%s : mutant invalide (%d occurrences)' % (name, original.count(old)))
            continue
        with tempfile.TemporaryDirectory() as root:
            folder, tools = os.path.join(root, 'mes_c_petits'), os.path.join(root, 'outils')
            os.makedirs(folder)
            os.makedirs(tools)
            with open(os.path.join(folder, 'pilote_c.py'), 'w', encoding='utf-8') as out:
                out.write(original.replace(old, new))
            shutil.copy(os.path.join(HERE, 'test_pilote_c.py'), folder)
            for module in ('lecteur_full.py', 'banc_full.py'):
                shutil.copy(os.path.join(HERE, '..', 'outils', module), tools)
            done = subprocess.run([sys.executable, '-S', os.path.join(folder, 'test_pilote_c.py')],
                                  stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=900)
        if done.returncode == 0:
            bad.append('%s : survit' % name)
    for line in bad:
        print(line, file=sys.stderr)
    if bad:
        return 1
    print('mutants_pilote_c_ok tues=%d' % len(MUTANTS))
    return 0


if __name__ == '__main__':
    sys.exit(main())
