Preuves de l'audit L06 (code de la tour de morsehgp3D_v10, HEAD origin/main afb081774), 2 octobre 2026.
Rapport : /workspaces/E-HGP/build/v11-persist/audit_v10/L06_CODE_TOUR.md
Calculs : /tmp/v11-audit/l06_code_tour/ (perdus au redemarrage ; les scripts ci-dessous les rejouent).
Binaire juge : build Release hors source du HEAD, /tmp/v11-audit/l06_code_tour/build-release.
Aucun fichier n'a ete cree dans l'arbre de lecture ; aucune commande GCP ; aucune commande git mutante.

mesures/          sorties JSON de mhgp10_tower (trame 00, K = 5 et 10, 1 et 4 fils, avec et sans attaches ; trame avec sol), pics RSS
g4/               extraction par etage des sorties brutes du recu receipts/g4_session4_j2c_20260929 (lecture seule)
instrumentation/  patch_instr.py, patch_tsc.py : compteurs ajoutes a une COPIE de tower.cpp (aucune decision changee) ;
                  instr_*.err, tsc_*.err : compteurs L06_* ; echelle_clusters_* : 8 000, 16 000, 32 000 sites ;
                  contraste_noyau_halo.txt ; werror_array_bounds.log
krbench/          krbench.cpp : Kruskal par lots (copie fidele) contre noyau sans lots, sur les entrees reelles ; resultats
oracles/          ctest_portes_tour.log ; oracle_k10.py (Gamma_k jusqu'a K = 10) ; oracle_sauts.py (nuages choisis pour sauter,
                  K = 6, 8, 10) ; oracle_amas.py ; oracle_cover.py (composante de l'entree cover) ; emst_judge.py (ordre 1
                  contre EMST) ; journaux
mutants/          make_mutants.py, run_mutants.sh, mutants_results.txt, mutant_point_open_validate.txt, mutants_sur_trame00.txt,
                  autres_portes_contre_mutants.txt, run_alt.sh et descente_alternative_resultats.txt (differentiel de descente)
meb/              welzl_harness.cpp, welzl_judge.py, welzl_judge.log (MEB de tower.cpp contre la force brute exacte)
sanitizers/       run_sanitizers.sh, sanitizers.txt (ASan, UBSan, TSan sur petites entrees)
determinisme/     sha256 des dumps a 1, 3 et 4 fils
isometrie/        iso_test.py (classes d'attache sous isometrie), tower_merkle.py (empreinte canonique), anchors.jsonl (ancres v10)
limites/          gen.py (cas limites), run_edge.sh, edge_results.txt, sphere24_k10.time
