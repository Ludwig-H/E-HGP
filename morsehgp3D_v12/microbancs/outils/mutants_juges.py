#!/usr/bin/env python3
"""Mutants causaux des juges et pilotes durcis (CST-0018, CST-0213, CST-0214, CST-0215) : chaque garde retiree doit
etre detectee par sa porte. Juge les portes, pas les microbancs : aucune donnee, aucun GPU.

Chaque mutant est une substitution textuelle unique (refusee si elle ne s'applique pas exactement une fois) dans une
copie temporaire de microbancs/ ; la porte concernee est rejouee sur la copie et doit echouer (code 1). Les mutants
natifs de mhgp12_mes_m4 sont compiles (g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror) et joues par
tests/test_m4_preuves.py. Deux gardes doublees (effacement de la cible et jeton de session de MES-M2) ne sont tuees
qu'ensemble : retirees seules, l'autre refuse encore ; ce mutant est donc combine.

Usage : python3 -S -O outils/mutants_juges.py --recu-g4-m2 DOSSIER_m2 --recu-g4-tour DOSSIER_g4_t0b
          [--binaires-tour DIR] [--cxx g++]
Codes : 0 tous les mutants tues, 1 un mutant vivant ou non applicable, 2 usage.
"""
import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

sys.dont_write_bytecode = True
ICI = Path(__file__).resolve().parent
MICROBANCS = ICI.parent
M2 = 'mes_m2_feuille/scripts/g4_leaf_bench.py'
PILOTE = 'mes_m3_m4_tour/pilote.py'
M4 = 'mes_m3_m4_tour/mes_m4/mes_m4.cpp'

MUTANTS = [
    # (nom, fichier, [(avant, apres)], porte)
    ('m2_identite_hote_ignoree', M2, [("                s.refuse('identite hote : ' + '; '.join(problems[:6]))",
                                       "                pass")], 'm2'),
    ('m2_prises_perimees_admises', M2, [("    if result.get('nonce') != nonce:", "    if False:"),
                                         ("        if target.exists():\n            target.unlink()\n    except OSError as e:\n"
                                          "        return None, '', 'cible perimee non effacable : %s' % e",
                                          "        pass\n    except OSError as e:\n"
                                          "        return None, '', 'cible perimee non effacable : %s' % e")], 'm2'),
    ('m2_cas_decisif_non_exige', M2, [("        if not any(deciding(name, decide) for name in expected):", "        if False:"),
                                       ("    if any(c not in configs for c in CONTRACT['configs']):", "    if False:"),
                                       ("            s.refuse('cas qui decide du contrat sans prise : ' + name)",
                                        "            pass")], 'm2'),
    ('m2_durees_non_finies_admises', M2, [(" or not math.isfinite(x) or x <= 0:", ":")], 'm2'),
    ('m2_isolation_apres_ignoree', M2, [("    isolation = isolation_start and isolation_before is not None and "
                                         "isolation_before[0] and \\\n        isolation_after is not None and "
                                         "isolation_after[0]", "    isolation = isolation_start")], 'm2'),
    ('m2_sanitizer_absent_admis', M2, [("            s.refuse('compute-sanitizer introuvable')", "            pass")],
     'm2'),
    ('m2_temoin_faux_admis', M2, [("                if taken['forms']['witness']['identity'] is not True:",
                                   "                if False:")], 'm2'),
    ('m2_admission_ignoree', M2, [("        admitted = code == 0 and not bad", "        admitted = True")], 'm2'),
    ('m2_processus_minimum_ignore', M2, [("    if args.processes < CONTRACT['processes_min']:", "    if False:")], 'm2'),
    ('m2_binaire_modifie_ignore', M2, [("    if end_binaries != binaries:", "    if False:")], 'm2'),
    ('pilote_lem_t6_non_exige', PILOTE, [('        if k >= 2 and (t6["naissances_jugees"] != l.get("naissances") or '
                                          'not t6["naissances_jugees"]):', '        if False:')], 'tour'),
    ('pilote_flower_facultative', PILOTE, [('    exigees = set(SECTIONS[genre]) - ({"FLOWER"} if genre == 3 and '
                                            'ordre < 2 else set())', '    exigees = set()')], 'tour'),
    ('pilote_prise_non_rattachee', PILOTE, [('            if reecrits != ref:', '            if False:')], 'tour'),
    ('pilote_une_prise_suffit', PILOTE, [('len(prises) < exigees or', 'len(prises) < 1 or')], 'tour'),
    ('pilote_campagne_ecrasee', PILOTE, [('        bloc["campagnes"][args.campagne] = campagne',
                                          '        bloc["campagnes"] = {args.campagne: campagne}')], 'tour'),
    ('pilote_portes_au_code_seul', PILOTE, [('        problemes += valider_porte(nom, code, lignes)',
                                             '        problemes += [] if code == code_attendu else ["code"]')],
     'tour'),
    ('pilote_sortie_silencieuse_admise', PILOTE, [('def valider_m3(lignes, code, trame, k_max, fichiers):\n',
                                                   'def valider_m3(lignes, code, trame, k_max, fichiers):\n'
                                                   '    return [], code == 0\n')], 'tour'),
    ('pilote_binaire_sans_provenance', PILOTE, [('def problemes_provenance(prov):\n',
                                                 'def problemes_provenance(prov):\n    return []\n')], 'tour'),
    ('pilote_vidages_non_reverifies', PILOTE, [('        if sha256_ou_rien(os.path.join(dossier, nom)) != attendu:',
                                                '        if False:')], 'tour'),
    ('pilote_mutant_tue_par_code', PILOTE, [('def valider_mutant_m4(code, lignes):\n',
                                             'def valider_mutant_m4(code, lignes):\n'
                                             '    return code == 1, True, True\n')], 'tour'),
    ('m4_flower_optionnelle', M4, [('    if (k >= 2 && !forest.has("FLOWER")) return refuse(k, "section_absente FLOWER '
                                    '(verticales de l\'ordre k)");\n', '')], 'm4'),
    ('m4_cles_hors_domaine_admises', M4, [('      if (births_s.first[i].key >= key_domain || (i > 0 && ',
                                           '      if (key_domain == ~u64{0} || (i > 0 && ')], 'm4'),
]


def run(cmd, cwd=None):
    proc = subprocess.run([str(c) for c in cmd], cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return proc.returncode, proc.stdout, proc.stderr


def main(argv):
    options = {}
    i = 1
    while i < len(argv):
        if argv[i] in ('--recu-g4-m2', '--recu-g4-tour', '--binaires-tour', '--cxx') and i + 1 < len(argv):
            options[argv[i]] = argv[i + 1]
            i += 2
        else:
            print(__doc__)
            return 2
    if '--recu-g4-m2' not in options or '--recu-g4-tour' not in options:
        print(__doc__)
        return 2
    cxx = options.get('--cxx', 'g++')
    python = sys.executable
    resultats, vivants = [], []
    with tempfile.TemporaryDirectory(prefix='mutants-juges-') as tmp:
        tmp = Path(tmp)
        for nom, fichier, substitutions, porte in MUTANTS:
            copie = tmp / nom
            shutil.copytree(MICROBANCS, copie / 'microbancs', ignore=shutil.ignore_patterns('build', 'out', '__pycache__'))
            cible = copie / 'microbancs' / fichier
            texte = cible.read_text()
            applicable = True
            for avant, apres in substitutions:
                if texte.count(avant) != 1:
                    applicable = False
                    break
                texte = texte.replace(avant, apres)
            if not applicable:
                resultats.append({'mutant': nom, 'verdict': 'NON_APPLICABLE'})
                vivants.append(nom)
                continue
            cible.write_text(texte)
            racine = copie / 'microbancs'
            if porte == 'm2':
                code, out, err = run([python, '-S', '-O', racine / 'mes_m2_feuille/tests/test_juge_m2.py',
                                      '--recu-g4', options['--recu-g4-m2']])
            elif porte == 'tour':
                cmd = [python, '-S', '-O', racine / 'mes_m3_m4_tour/tests/test_pilote.py', '--recu-g4',
                       options['--recu-g4-tour']]
                if '--binaires-tour' in options:
                    cmd += ['--binaires', options['--binaires-tour']]
                code, out, err = run(cmd)
            else:
                exe = copie / 'mhgp12_mes_m4_mutant'
                c, _, e = run([cxx, '-std=c++20', '-O1', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread', cible,
                               '-o', exe])
                if c != 0:
                    resultats.append({'mutant': nom, 'verdict': 'NON_COMPILE', 'erreur': e[-300:]})
                    vivants.append(nom)
                    continue
                code, out, err = run([python, '-S', '-O', racine / 'mes_m3_m4_tour/tests/test_m4_preuves.py',
                                      '--binaire', exe])
            ecarts = []
            for ligne in out.splitlines():
                if ligne.startswith('{') and '"ecarts"' in ligne:
                    try:
                        ecarts = json.loads(ligne).get('ecarts', [])
                    except ValueError:
                        pass
            tue = code == 1 and bool(ecarts)
            resultats.append({'mutant': nom, 'porte': porte, 'code_porte': code, 'verdict': 'tue' if tue else 'VIVANT',
                              'detection': ecarts[0][:160] if ecarts else (err.strip()[-160:] or None)})
            if not tue:
                vivants.append(nom)
    for r in resultats:
        print(json.dumps(r, ensure_ascii=False, sort_keys=True))
    print(json.dumps({'mutants': len(resultats), 'vivants': vivants, 'optimise': sys.flags.optimize},
                     ensure_ascii=False))
    return 0 if not vivants else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
