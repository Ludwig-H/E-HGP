#!/usr/bin/env python3
"""Mutants du pilote MES-B (verdicts, empreintes, etiquettes, delai, raccord au lecteur partage) : chaque mutant est
applique a une copie de pilote_b.py dans un dossier temporaire qui reproduit la disposition du depot
(mes_b_scenes/ et outils/lecteur_full.py), la porte test_pilote_b.py y est rejouee et doit echouer (mutant tue). Les
mutants du lecteur sont dans outils/mutants_lecteur_full.py. Un mutant dont le texte d'origine n'apparait pas exactement
une fois est invalide (ecart). Python 3.10 nu, aucun assert. Codes : 0 tous tues ; 1 un mutant survit ou est invalide.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
MUTANTS = {
    'b1_seuil': ("for n, v in rows if v > 2.0]", "for n, v in rows if v > 2.5]"),
    'b2_tous': ("small = [r for r in k5 if r['sites'] < 10_000_000]", "small = list(k5)"),
    'b3_seuil': ("        if s > 1.1:", "        if s > 1.5:"),
    'empreinte_voies': ("        if len(everything) != 1:",
                        "        if any(len(v) != 1 for v in by_voie.values()):"),
    'empreinte_passes': ("        if len(everything) != 1:", "        if len(by_voie) > 1 and len(everything) != 1:"),
    'etiquette_tronquee': ("        label = prefix + name[-(23 - len(prefix)):]",
                           "        label = (prefix + name)[-23:]\n        break"),
    'delai_ignore': ("        if forecast > remaining:", "        if False:"),
    'b1_sans_echec': ("            if r['etat'] == 'echec' or (r['etat'] == 'refus' and r['sites'] < 10_000_000)]",
                      "            if r['etat'] == 'refus' and r['sites'] < 10_000_000]"),
    'b1_refus_partout': ("            if r['etat'] == 'echec' or (r['etat'] == 'refus' and r['sites'] < 10_000_000)]",
                         "            if r['etat'] in ('echec', 'refus')]"),
    'b4_sans_refus': ("for r in k10 if r['etat'] != 'ok']", "for r in k10 if r['etat'] == 'echec']"),
    'pente_sans_garde': ("    if any(x <= 0 or y <= 0 for x, y in points):\n        return None\n", ""),
    'budget_partage_attendu': ("trames=[(label, sites)], budget_appareil='separe', bits=BITS_EXPECTED,",
                               "trames=[(label, sites)], budget_appareil='partage', bits=BITS_EXPECTED,"),
    'schema_sequentiel_attendu': ("bits=BITS_EXPECTED, schema=MODE['schema'])",
                                  "bits=BITS_EXPECTED, schema='sequentiel')"),
    'drapeau_perdu': ("(['--digest'] if c['empreinte'] else []) + \\\n            MODE['flags']",
                      "(['--digest'] if c['empreinte'] else [])"),
    'mode_colle': ("    MODE.update(dict(schema='sequentiel', flags=['--sequentiel']) if args.sequentiel else\n"
                   "                dict(schema='recouvert', flags=[]))",
                   "    if args.sequentiel:\n        MODE.update(schema='sequentiel', flags=['--sequentiel'])"),
    'colonnes_sequentielles': ("    tail = ('queue',) if overlapped else ('T', 'M', 'V', 'R')",
                               "    tail = ('T', 'M', 'V', 'R')"),
    'memoire_sequentielle': ("    stages = lf.MEM_OVERLAP if overlapped else lf.MEM_STAGES",
                             "    stages = lf.MEM_STAGES"),
}


def main():
    with open(os.path.join(HERE, 'pilote_b.py'), encoding='utf-8') as handle:
        original = handle.read()
    bad = []
    for name, (old, new) in MUTANTS.items():
        if original.count(old) != 1:
            bad.append('%s : mutant invalide (%d occurrences)' % (name, original.count(old)))
            continue
        with tempfile.TemporaryDirectory() as root:
            folder, tools = os.path.join(root, 'mes_b_scenes'), os.path.join(root, 'outils')
            os.makedirs(folder)
            os.makedirs(tools)
            with open(os.path.join(folder, 'pilote_b.py'), 'w', encoding='utf-8') as out:
                out.write(original.replace(old, new))
            shutil.copy(os.path.join(HERE, 'test_pilote_b.py'), folder)
            shutil.copy(os.path.join(HERE, '..', 'outils', 'lecteur_full.py'), tools)
            done = subprocess.run([sys.executable, '-S', os.path.join(folder, 'test_pilote_b.py')],
                                  stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=600)
        if done.returncode == 0:
            bad.append('%s : survit' % name)
    for line in bad:
        print(line, file=sys.stderr)
    if bad:
        return 1
    print('mutants_pilote_b_ok tues=%d' % len(MUTANTS))
    return 0


if __name__ == '__main__':
    sys.exit(main())
