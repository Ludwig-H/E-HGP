#!/usr/bin/env python3
"""Mutants causaux des juges et pilotes durcis (CST-0018, CST-0213, CST-0214, CST-0215, CST-0222, CST-0223) : chaque
garde retiree doit etre detectee par sa porte. Juge les portes, pas les microbancs : aucune donnee, aucun GPU.

Chaque mutant est une substitution textuelle unique (refusee si elle ne s'applique pas exactement une fois ; toutes
sont verifiees avant la premiere porte) dans une copie temporaire de microbancs/ ; la porte concernee est rejouee sur
la copie et doit echouer (code 1 et au moins un ecart). Avant les mutants, chaque porte utilisee est jouee sur une
copie NON mutee et doit etre conforme (code 0) : sans ce temoin, un recu absent ou un binaire manquant tuerait tous
les mutants de la porte par vacuite ; un temoin en echec fait echouer l'outil (TEMOIN_EN_ECHEC).

Portes : m2 (mes_m2_feuille/tests/test_juge_m2.py, sorties reelles de la session A) ; tour (mes_m3_m4_tour/tests/
test_pilote.py, sessions B et D) ; m4 (mutants natifs de mhgp12_mes_m4, compiles par g++ -std=c++20 -Wall -Wextra
-Wpedantic -Werror et joues par tests/test_m4_preuves.py) ; m5 (mes_m5_parcours/tests/test_juge_m5.py, session C) ;
m5_format (porte du lecteur strict host/format_selftest.cpp, CST-0223, compilee) ; m5_unit (portes unitaires de
host/traversal_identity.cpp --unit, CST-0222 : code 3 et "ok":false = tue) ; m6 (mes_m6_session/tests/
test_juge_m6.py, session A).

Gardes doublees, tuees seulement ensemble (retiree seule, l'autre refuse encore ; mutant combine) : effacement de la
cible et jeton de session (MES-M2, MES-M5) ; passes v11 exigees et repli sur la passe froide, garde de contrat du juge
et du collecteur (MES-M5). Sans mutant : la somme controlee des tests G1 dans le lecteur de MES-M5 (la deborder sans
violer la borne par noeud exige plus de 10^8 noeuds ; seule checked_add est jouee a la borne de u64).

Usage : python3 -S -O outils/mutants_juges.py --recu-g4-m2 DOSSIER_m2 --recu-g4-tour DOSSIER_g4_t0b
          --recu-g4-tour-d DOSSIER_g4_t0d --recu-g4-m5 DOSSIER_m5 --recu-g4-m6 DOSSIER_m6
          [--binaires-tour DIR] [--cxx g++] [--seulement PREFIXE]
Codes : 0 tous les temoins conformes et tous les mutants tues ; 1 un temoin en echec, ou un mutant vivant, non
applicable ou non compile ; 2 usage.
"""
import json
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
M5 = 'mes_m5_parcours/scripts/g4_traversal_bench.py'
M5_FORMAT = 'mes_m5_parcours/include/mhgp12/traversal/format.hpp'
M5_BFS = 'mes_m5_parcours/include/mhgp12/traversal/bfs.hpp'
M6 = 'mes_m6_session/run_m6.py'
RECUS = ('--recu-g4-m2', '--recu-g4-tour', '--recu-g4-tour-d', '--recu-g4-m5', '--recu-g4-m6')

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
    ('m2_temoin_faux_a_l_echauffement_admis', M2, [("        elif taken is not None and taken['forms']['witness']"
                                                    "['identity'] is not True:", "        elif False:")], 'm2'),
    ('m2_admission_ignoree', M2, [("        admitted = code == 0 and not bad", "        admitted = True")], 'm2'),
    ('m2_processus_minimum_ignore', M2, [("    if args.processes < CONTRACT['processes_min']:", "    if False:")], 'm2'),
    ('m2_binaire_modifie_ignore', M2, [("    if end_binaries != binaries:", "    if False:")], 'm2'),
    # MES-M2 : preuves contradictoires du recu audit_cd_corrections_20261007/m2 (CST-0018), chaque garde seule.
    ('m2_compteurs_de_forme_ignores', M2, [("        why = form_counters_consistent(r, records, population)\n",
                                            "        why = None\n")], 'm2'),
    ('m2_formes_du_sanitizer_ignorees', M2, [("    if not list(forms) or not isinstance(rows, list) or \\\n"
                                              "            [r.get('form') if isinstance(r, dict) else None for r in "
                                              "rows] != list(forms):", "    if not isinstance(rows, list):")], 'm2'),
    ('m2_couverture_du_sanitizer_ignoree', M2, [("    if covered < 1 or c.get('leaves') != covered:",
                                                 "    if covered < 1:")], 'm2'),
    ('m2_echauffement_discordant_admis', M2, [("        if taken is not None and (code == 0) != taken['identity']:\n"
                                               "            taken, why = None, 'code %s et identite discordants' % code\n"
                                               "        elif", "        if False:\n"
                                               "            taken, why = None, 'code %s et identite discordants' % code\n"
                                               "        elif")], 'm2'),
    ('m2_sanitizer_code_discordant_admis', M2, [("                if taken is not None and (code == 0) != "
                                                 "taken['identity']:\n                    taken, why = None, 'code %s "
                                                 "et identite discordants' % code\n                ok = taken is not None",
                                                 "                if False:\n                    taken, why = None, "
                                                 "'code %s et identite discordants' % code\n                ok = taken is "
                                                 "not None")], 'm2'),
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
    # MES-M4 : mutant rattache a son cas (recu audit_cd_corrections_20261007/m34, CST-0018), chaque garde seule.
    ('pilote_mutant_tue_par_code', PILOTE, [('def valider_mutant_m4(code, lignes, trame, k_max, fichiers):\n',
                                             'def valider_mutant_m4(code, lignes, trame, k_max, fichiers):\n'
                                             '    return [], code == 1, True, True\n')], 'tour'),
    ('pilote_mutant_d_une_autre_trame', PILOTE, [('entrees[0].get("trame") != trame or \\\n            entrees[0].get('
                                                  '"mutant_sans_contraction") is not True:', '\\\n            '
                                                  'entrees[0].get("mutant_sans_contraction") is not True:')], 'tour'),
    ('pilote_mutant_tronque_admis', PILOTE, [('    problemes += p\n    geometrique = False',
                                              '    geometrique = False')], 'tour'),
    ('pilote_mutant_comptes_non_lies', PILOTE, [('            if l.get(cle) != attendu or attendu is None:\n'
                                                 '                problemes.append("ordre %d : %s %s, vidage %s" % (k, '
                                                 'cle, l.get(cle), attendu))\n        ident = l.get("identite")',
                                                 '            pass\n        ident = l.get("identite")')], 'tour'),
    ('pilote_juge_m4_mutant_incomplet_admis', PILOTE, [('b.get("sortie_complete") is True and not b.get("refus") and ',
                                                        '')], 'tour'),
    ('m4_flower_optionnelle', M4, [('    if (k >= 2 && !forest.has("FLOWER")) return refuse(k, "section_absente FLOWER '
                                    '(verticales de l\'ordre k)");\n', '')], 'm4'),
    ('m4_cles_hors_domaine_admises', M4, [('      if (births_s.first[i].key >= key_domain || (i > 0 && ',
                                           '      if (key_domain == ~u64{0} || (i > 0 && ')], 'm4'),
    # MES-M5 : juge et pilote (CST-0018).
    ('m5_mediane_declaree_crue', M5, [("  if not same_median(declared.get('total'), gpu_ms) or not "
                                       "same_median(declared.get('resident'), resident_ms):", '  if False:')], 'm5'),
    ('m5_code_d_outil_ignore', M5, [('  elif code not in (0, 1):', '  elif False:')], 'm5'),
    ('m5_passe_froide_admise', M5, [("  if r.get('v11_passes') != passes or not isinstance(ns, list) or len(ns) != "
                                     "passes or passes < 2:", '  if not isinstance(ns, list) or not ns:'),
                                    ("'v11_ms': median(ns[1:]) / 1e6", "'v11_ms': median(ns[1:] or ns) / 1e6")],
     'm5'),
    ('m5_grille_reduite_admise', M5, [('  missing = [c for c in contract_cases() if c not in cases]',
                                       '  missing = []'),
                                      ('  if sorted(deciding) != sorted(contract_deciding()) or any(c not in cases for c '
                                       'in deciding):', '  if False:'),
                                      ("  if not is_int(need) or need < CONTRACT['processes_min']:", '  if False:'),
                                      ("  for reason in deviations:\n    s.refuse('hors contrat : ' + reason)",
                                       '  for reason in deviations:\n    pass')], 'm5'),
    ('m5_prises_perimees_admises', M5, [("  if nonce is not None and g.get('nonce') != nonce:", '  if False:'),
                                        ('def clear(target):\n', 'def clear(target):\n  return True\n')], 'm5'),
    ('m5_isolation_de_debut_seule', M5, [('  isolation = isolation_start and isolation_before[0] and '
                                          'isolation_after[0]', '  isolation = isolation_start')], 'm5'),
    ('m5_sanitizer_sans_prise_admis', M5, [('    elif proof.get(tool) is not True:', '    elif False:')], 'm5'),
    ('m5_binaire_modifie_ignore', M5, [('  if end_binaries != binaries:', '  if False:')], 'm5'),
    ('m5_sources_non_rehachees', M5, [('  if end_sources != sources:', '  if False:')], 'm5'),
    ('m5_porte_du_lecteur_ignoree', M5, [('  if not format_ok:', '  if False:')], 'm5'),
    ('m5_temoin_cst0222_non_exige', M5, [(" and \\\n        units[0].get('emit_tasks_u64') is True", '')], 'm5'),
    ('m5_reservation_admise', M5, [("  if r.get('timed_allocations') != 0:", '  if False:')], 'm5'),
    ('m5_code_et_identite_discordants', M5, [("        if gpu is not None and (code == 0) != (gpu['gpu_identity'] is "
                                              "True):", '        if False:')], 'm5'),
    # MES-M5 : lecteur strict (CST-0223) et emission des taches (CST-0222), natifs.
    ('m5_lecteur_tests_g1_non_bornes', M5_FORMAT, [('    if (!tests_in_bounds(n.tests, n.candidates, h.kmax))',
                                                    '    if (false)')], 'm5_format'),
    ('m5_lecteur_racine_non_controlee', M5_FORMAT, [('        if (n.lo[a] != cloud_lo[a] || n.hi[a] != cloud_hi[a] + 1)',
                                                     '        if (false)')], 'm5_format'),
    ('m5_lecteur_boite_de_feuille_libre', M5_FORMAT, [('      if (l.lo[a] != lo || l.hi[a] != hi)',
                                                       '      if (lo > hi)')], 'm5_format'),
    ('m5_lecteur_feuille_non_terminale', M5_FORMAT, [('    if (l.m > h.leaf_size && width > 1) return fail',
                                                      '    if (false) return fail')], 'm5_format'),
    ('m5_emission_taches_u32', M5_BFS, [('      q.tasks = static_cast<u32>(tasks_of(o.count));',
                                         '      q.tasks = (o.count + kChunk - 1) / kChunk;')], 'm5_unit'),
    # MES-M6 : pilote (CST-0018, complement de pilote du recu audit_session_t1_20261007/m6).
    ('m6_prises_non_lues', M6, [('def check_take(text, mode, reps):\n',
                                 'def check_take(text, mode, reps):\n  return []\n')], 'm6'),
    ('m6_binaire_non_exige', M6, [("    elif report['binary_sha256'] is None:", '    elif False:')], 'm6'),
    ('m6_effectifs_ignores', M6, [("  if not is_int(row['n']) or row['n'] != n:", "  if not is_int(row['n']):")], 'm6'),
    ('m6_quantiles_non_ordonnes', M6, [('  if values != sorted(values):', '  if False:')], 'm6'),
    ('m6_isolation_par_prise_ignoree', M6, [('        if not before or not after:', '        if False:')], 'm6'),
    ('m6_isolation_de_debut_ignoree', M6, [("  if not quiet:\n    refusals.append('GPU non isole au debut",
                                            "  if False:\n    refusals.append('GPU non isole au debut")], 'm6'),
    ('m6_binaire_non_rehache', M6, [("    if sha256(binary) != report['binary_sha256']:", '    if False:')], 'm6'),
    ('m6_sources_non_rehachees', M6, [("  if {'run_m6.py': sha256(DRIVER), 'mes_m6_session_cost.cu': sha256(SOURCE)} "
                                       "!= sources:", '  if False:')], 'm6'),
    ('m6_mode_ignore', M6, [("    if row['sync'] != mode:", '    if False:')], 'm6'),
    ('m6_doublons_admis', M6, [('    if key in seen:', '    if False:')], 'm6'),
    ('m6_sorties_perimees_gardees', M6, [('def clear_outputs(out):\n', 'def clear_outputs(out):\n  return [], None\n')],
     'm6'),
    ('m6_appareils_melanges', M6, [('  if len(devices) > 1:', '  if False:')], 'm6'),
    ('m6_code_non_nul_admis', M6, [("        problems = (['code %d' % run_code] if run_code != 0 else []) + "
                                    "check_take(run_out, mode, args.reps)",
                                    '        problems = check_take(run_out, mode, args.reps)')], 'm6'),
    ('m6_rejuge_empreintes_ignorees', M6, [("        if r.get('sha256') != hashlib.sha256(data).hexdigest():",
                                            '        if False:')], 'm6'),
    ('m6_rejuge_code_ignore', M6, [("      found = (['code %r' % r.get('code')] if r.get('code') != 0 else []) + "
                                    "check_take(text, mode, reps)", '      found = check_take(text, mode, reps)')], 'm6'),
    ('m6_rejuge_isolation_ignoree', M6, [("        if not all(isinstance(r.get(side), dict) and r[side].get('quiet') "
                                          "is True\n                   for side in ('isolation_before', "
                                          "'isolation_after')):", '        if False:')], 'm6'),
]


def run(cmd, cwd=None):
    proc = subprocess.run([str(c) for c in cmd], cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return proc.returncode, proc.stdout, proc.stderr


def compiler(cxx, sources, exe, include=()):
    cmd = [cxx, '-std=c++20', '-O1', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread']
    for chemin in include:
        cmd += ['-I', chemin]
    code, _, err = run(cmd + list(sources) + ['-o', exe])
    return None if code == 0 else err[-300:]


def jouer_porte(porte, racine, copie, options, cxx, python):
    """Joue une porte sur une copie de microbancs/ (racine). Rend (code, sortie, erreur, erreur de compilation)."""
    if porte == 'm2':
        return run([python, '-S', '-O', racine / 'mes_m2_feuille/tests/test_juge_m2.py', '--recu-g4',
                    options['--recu-g4-m2']]) + (None,)
    if porte == 'tour':
        cmd = [python, '-S', '-O', racine / 'mes_m3_m4_tour/tests/test_pilote.py', '--recu-g4', options['--recu-g4-tour'],
               '--recu-g4-d', options['--recu-g4-tour-d']]
        if '--binaires-tour' in options:
            cmd += ['--binaires', options['--binaires-tour']]
        return run(cmd) + (None,)
    if porte == 'm5':
        return run([python, '-S', '-O', racine / 'mes_m5_parcours/tests/test_juge_m5.py', '--recu-g4',
                    options['--recu-g4-m5']]) + (None,)
    if porte == 'm6':
        return run([python, '-S', '-O', racine / 'mes_m6_session/tests/test_juge_m6.py', '--recu-g4',
                    options['--recu-g4-m6']]) + (None,)
    if porte == 'm4':
        exe = copie / 'mhgp12_mes_m4_porte'
        erreur = compiler(cxx, [racine / M4], exe)
        if erreur is not None:
            return None, '', '', erreur
        return run([python, '-S', '-O', racine / 'mes_m3_m4_tour/tests/test_m4_preuves.py', '--binaire', exe]) + (None,)
    source = 'host/format_selftest.cpp' if porte == 'm5_format' else 'host/traversal_identity.cpp'
    exe = copie / ('mhgp12_' + porte)
    erreur = compiler(cxx, ['-DMHGP12_COORD_BITS=21', racine / 'mes_m5_parcours' / source], exe,
                      [racine / 'mes_m5_parcours/include', racine / 'mes_m2_feuille/include'])
    if erreur is not None:
        return None, '', '', erreur
    if porte == 'm5_format':
        dossier = copie / 'vidages'
        dossier.mkdir(exist_ok=True)
        code, out, err = run([exe, dossier])
        bilan = [json.loads(l) for l in out.splitlines() if l.startswith('{"porte"')]
        ecarts = bilan[-1].get('ecarts') if bilan else None
        if isinstance(ecarts, int):  # bilan natif : nombre d'ecarts
            out += '\n' + json.dumps({'ecarts': ['%d cas du lecteur differents de l attendu' % ecarts] if ecarts else []})
        return code, out, err, None
    code, out, err = run([exe, '--unit'])
    if code == 3 and '"ok":false' in out:  # porte unitaire en echec : le mutant est tue
        return 1, '{"ecarts": ["porte unitaire CST-0222 en echec"]}', err, None
    return code, out, err, None


def ecarts_de(out):
    ecarts = []
    for ligne in out.splitlines():
        if ligne.startswith('{') and '"ecarts"' in ligne:
            try:
                ecarts = json.loads(ligne).get('ecarts', [])
            except ValueError:
                pass
    return ecarts if isinstance(ecarts, list) else []


def copier(tmp, nom):
    copie = tmp / nom
    shutil.copytree(MICROBANCS, copie / 'microbancs', ignore=shutil.ignore_patterns('build', 'out', '__pycache__'))
    return copie


def main(argv):
    options = {}
    i = 1
    while i < len(argv):
        if argv[i] in RECUS + ('--binaires-tour', '--cxx', '--seulement') and i + 1 < len(argv):
            options[argv[i]] = argv[i + 1]
            i += 2
        else:
            print(__doc__)
            return 2
    if any(r not in options for r in RECUS):
        print(__doc__)
        return 2
    cxx = options.get('--cxx', 'g++')
    python = sys.executable
    choisis = [m for m in MUTANTS if m[0].startswith(options.get('--seulement', ''))]
    resultats, vivants, textes = [], [], {}
    for nom, fichier, substitutions, porte in choisis:  # toutes les substitutions verifiees avant toute porte
        texte = (MICROBANCS / fichier).read_text()
        for avant, apres in substitutions:
            if texte.count(avant) != 1:
                texte = None
                break
            texte = texte.replace(avant, apres)
        if texte is None:
            resultats.append({'mutant': nom, 'verdict': 'NON_APPLICABLE'})
            vivants.append(nom)
        else:
            textes[nom] = texte
    with tempfile.TemporaryDirectory(prefix='mutants-juges-') as tmp:
        tmp = Path(tmp)
        temoins = {}
        for porte in sorted({m[3] for m in choisis}):
            copie = copier(tmp, 'temoin_' + porte)
            code, out, err, erreur = jouer_porte(porte, copie / 'microbancs', copie, options, cxx, python)
            temoins[porte] = code == 0 and erreur is None
            resultats.append({'temoin': porte, 'code_porte': code, 'verdict': 'conforme' if temoins[porte] else
                              'TEMOIN_EN_ECHEC', 'detail': None if temoins[porte] else
                              (erreur or (ecarts_de(out) or [err.strip()[-200:]])[0][:200])})
            if not temoins[porte]:
                vivants.append('temoin_' + porte)
        for nom, fichier, substitutions, porte in choisis:
            if nom not in textes:
                continue
            if not temoins[porte]:
                resultats.append({'mutant': nom, 'porte': porte, 'verdict': 'NON_JOUE (temoin en echec)'})
                vivants.append(nom)
                continue
            copie = copier(tmp, nom)
            (copie / 'microbancs' / fichier).write_text(textes[nom])
            code, out, err, erreur = jouer_porte(porte, copie / 'microbancs', copie, options, cxx, python)
            if erreur is not None:
                resultats.append({'mutant': nom, 'porte': porte, 'verdict': 'NON_COMPILE', 'erreur': erreur})
                vivants.append(nom)
                continue
            ecarts = ecarts_de(out)
            tue = code == 1 and bool(ecarts)
            resultats.append({'mutant': nom, 'porte': porte, 'code_porte': code, 'verdict': 'tue' if tue else 'VIVANT',
                              'detection': ecarts[0][:160] if ecarts else (err.strip()[-160:] or None)})
            if not tue:
                vivants.append(nom)
    for r in resultats:
        print(json.dumps(r, ensure_ascii=False, sort_keys=True))
    print(json.dumps({'mutants': len(choisis), 'temoins': len(temoins), 'vivants': vivants,
                      'optimise': sys.flags.optimize}, ensure_ascii=False))
    return 0 if not vivants else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
