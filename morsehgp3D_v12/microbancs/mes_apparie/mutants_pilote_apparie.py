#!/usr/bin/env python3
"""Mutants du pilote apparie : chaque mutant est applique a une copie de pilote_apparie.py dans un dossier temporaire
qui reproduit la disposition du depot (mes_apparie/ et outils/), la porte test_pilote_apparie.py y est rejouee et doit
echouer (mutant tue). Un mutant dont le texte d'origine n'apparait pas exactement une fois est invalide (ecart).
Python 3.10 nu, aucun assert. Codes : 0 tous tues ; 1 un mutant survit ou est invalide.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
MUTANTS = {
    'sans_fermeture_binaire': ("            provenance.get('sonde_fin_sha256') != provenance['sonde_sha256']):",
                               "            False):"),
    'identite_non_relue': ("            one(report.get('identite', {}).get(label, {}).get(arm), path, attendu, "
                           "'identite %s %s' % (label, arm))", "            pass"),
    'resume_non_compare': ("        if any(dumped(take.get(key)) != dumped(value) for key, value in summary.items()):",
                           "        if False:"),
    'cohorte_vide_admise': ("    if type(labels) is not list or not labels or ", "    if type(labels) is not list or "),
    'borne_basse': ("        case['verdict'] = 'adopte' if all(v['ic95'][1] < rule['seuil_borne_haute']",
                    "        case['verdict'] = 'adopte' if all(v['ic95'][0] < rule['seuil_borne_haute']"),
    'aa_sans_fenetre': ("if abs(v['rapport'] - 1) > rule['fenetre_aa']]", "if False]"),
    'aa_adoptable': ("        if arm == aa:\n            case['verdict'] = 'controle A/A'\n            continue\n", ""),
    'identite_ignoree': ("        if len(digests) != 1:", "        if False:"),
    'tours_libres': ("    if any(type(tours) is not list or len(tours) != cfg['tours'] for tours in "
                     "report['campagne'].values()):", "    if False:"),
    'tour_incomplet_admis': ("            if type(row) is not dict or set(row) != set(arms) or \\\n",
                             "            if type(row) is not dict or \\\n"),
    'sans_rejeu_brut': ("    verdict = judge(report, out_dir)", "    verdict = judge(report)"),
    'schema_fixe': ("    return 'sequentiel' if '--sequentiel' in options else 'recouvert'", "    return 'recouvert'"),
    'premiere_passe_gardee': ("    return median([p['wall_ns'] for p in passes[1:]])",
                              "    return median([p['wall_ns'] for p in passes])"),
    'sans_rotation': ("    return [names[(t + i) % len(names)] for i in range(len(names))]", "    return list(names)"),
    'option_libre': ("any(not OPTION.fullmatch(o) for o in options)", "False"),
    'aa_options_libres': ("                                      arms[args.aa] != arms[args.reference])) or \\\n",
                          "                                      False)) or \\\n"),
    'rapport_inverse': ("math.log(row[arm]['mur_chaud_ns'] / row[reference]['mur_chaud_ns'])",
                        "math.log(row[reference]['mur_chaud_ns'] / row[arm]['mur_chaud_ns'])"),
}


def main():
    with open(os.path.join(HERE, 'pilote_apparie.py'), encoding='utf-8') as handle:
        original = handle.read()
    bad = []
    for name, (old, new) in MUTANTS.items():
        if original.count(old) != 1:
            bad.append('%s : mutant invalide (%d occurrences)' % (name, original.count(old)))
            continue
        with tempfile.TemporaryDirectory() as root:
            folder, tools = os.path.join(root, 'mes_apparie'), os.path.join(root, 'outils')
            os.makedirs(folder)
            os.makedirs(tools)
            with open(os.path.join(folder, 'pilote_apparie.py'), 'w', encoding='utf-8') as out:
                out.write(original.replace(old, new))
            shutil.copy(os.path.join(HERE, 'test_pilote_apparie.py'), folder)
            for module in ('lecteur_full.py', 'banc_full.py'):
                shutil.copy(os.path.join(HERE, '..', 'outils', module), tools)
            done = subprocess.run([sys.executable, '-S', os.path.join(folder, 'test_pilote_apparie.py')],
                                  stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=900)
        if done.returncode == 0:
            bad.append('%s : survit' % name)
    for line in bad:
        print(line, file=sys.stderr)
    if bad:
        return 1
    print('mutants_pilote_apparie_ok tues=%d' % len(MUTANTS))
    return 0


if __name__ == '__main__':
    sys.exit(main())
