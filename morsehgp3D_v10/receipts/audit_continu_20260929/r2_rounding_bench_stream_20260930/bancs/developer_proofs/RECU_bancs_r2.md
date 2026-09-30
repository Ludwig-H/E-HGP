# Reçu — bancs, second tour : signaux de `scale_run`, contrôles de `decide.py` et `merge_sessions.py` (30 septembre 2026)

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
mode=correctifs_r2
public_status=not_claimed
```

GCP non utilisé. Aucune graine `test` ni `test_v10b` : fixtures sur l'espace `dev` et lots A et C archivés relus
sans modification. Aucun source C++ touché : les binaires du build sont identiques octet pour octet à ceux de
`build/v10-wt` (`mhgp10_catalogue` `a3bbad50…`, `mhgp10_tower` `a2077628…`).

| Élément | Valeur |
| --- | --- |
| Base | `56020cab6` (extraction `git archive`, sans `receipts/` ni `audits/`) |
| Patch du premier tour | `bancs.patch`, sha256 `39a669b937e074e330322f2255ef1e0e5da8c4e7613c504132c969cd207b438f` |
| Patch du second tour | `bancs_r2.patch` (base → état final), série dans `series/` |
| Dépôt jetable | `/tmp/mhgp10-r2/bancs/src` : commits `base`, `r1`, puis deux commits de ce tour |
| Builds | `build/` (final) et `build-r1/` (état r1), Release, `-j3` |

Fichiers du dépôt touchés (état final contre base) : `bench/scaling/scale_run.py`, `bench/synthetic/decide.py`,
`bench/g4/merge_sessions.py`, `tests/regression/test_scale_run_timeout.py`,
`tests/regression/test_decide_completeness.py`, `CMakeLists.txt` (tous sous `morsehgp3D_v10/`). Pas de
`docs/math/` : aucun statut mathématique ne change.

## 0. Matériaux lus

- Rapport du vérificateur adverse du premier tour (bancs, « accepté avec réserves », P1 à P7) et ses sondes
  (`build/v10-fixes/bancs-verif/sondes/`, copiées dans [sondes_verif/](recu/sondes_verif/ORIGINE.txt), mêmes sha256).
- Contre-audit continu : `morsehgp3D_v10/audits/audit_continu_20260929/timeout/CONTRE_AUDIT_BANCS_CORRIGES_20260929.md`
  (ARI de 1,25 admis puis publié) et ses captures
  `morsehgp3D_v10/receipts/audit_continu_20260929/bench_corrected/` (`record.py`, `normal_r2/`, `optimized_r2/`,
  `closure.json`).
- Contre-audit indépendant : `morsehgp3D_v10/audits/audit_independant_20260929/CONTRE_AUDIT_TETE_BANCS.md`, mise à jour
  du 30 septembre (critères de réception du banc : table dev complète acceptée, chaque mutation isolée refusée sans
  décision, témoin `refused=1` à NaN et scores négatifs valides conservés, rejeu normal et `-O`, simulation
  déterministe de la course).
- Décision de raccord : `morsehgp3D_v10/audits/REPONSE_CLAUDE_CONTRE_AUDITS_ET_RACCORD_20260929.md`, § 2 « Bancs ».

## 1. B-a — course sur le chemin des signaux de `scale_run.py` (P1, P2)

### Reproduction sur r1

Simulation déterministe du vérificateur (le gestionnaire de `scale_run` est appelé au premier `pthread_sigmask` de
`close_group`) :

```bash
python3 -B sondes_verif/signal_dans_close_group.py src-r1/morsehgp3D_v10/bench/scaling/scale_run.py        # code 0
python3.10 -B sondes_verif/signal_dans_close_group.py src-r1/morsehgp3D_v10/bench/scaling/scale_run.py     # code 0
```

Sortie ([3.12](recu/preuves/a_signal/r1/simulation_python3.txt), [3.10](recu/preuves/a_signal/r1/simulation_python3.10.txt)) :
`SURVIVANTS pendant_appel=2 delai=2`. Enfant et petit-enfant survivent, pendant un appel comme après un délai. Les
vrais signaux sont re-mesurés plus bas (« Re-mesure avec les vrais signaux »).

### Cause

Le premier SIGTERM lève `Terminated` dans `communicate`. La clause de fermeture appelle `close_group`. Un second
SIGTERM traité avant `killpg` (au point de contrôle d'entrée de `close_group`, ou juste après `pthread_sigmask` pour
un signal déjà reçu au niveau C) lève un nouveau `Terminated`. Il sort de `close_group` avant le SIGKILL de groupe.
L'appel, dans sa propre session, survit ; `main` écrit pourtant « appel en cours tué et récolté » et rend 143. Le
masquage seul ne suffit pas : il bloque la délivrance, pas le gestionnaire Python d'un signal déjà reçu.

### Constat supplémentaire : Python 3.10 traite un signal déjà reçu à l'entrée d'une clause `except`

La VM G4 exécute `python3` 3.10.12 (`receipts/test_cover_C_20260929/sessions/c1/env/python3.txt`). Essai
[fenetre_clause.py](recu/essais/fenetre_clause.py) : un appel C long reçoit SIGALRM puis lève une exception qui n'est pas
un signal ; le gestionnaire lève `Boom`.

| Interpréteur | `Boom` levé à l'entrée de la clause, hors du `try` | Levé dans le corps ou plus tard |
| --- | ---: | ---: |
| 3.10.21 | 20 sur 20 | 0 |
| 3.12.1 | 0 | 20 sur 20 |

([sortie](recu/essais/fenetre_clause.txt), [point de contrôle observé](recu/essais/point_controle.txt) : en 3.10, le cadre est
encore sur la ligne fautive quand le gestionnaire tourne.) Conséquence : avec la seule correction minimale du
vérificateur, un signal reçu pendant que `communicate` lève `TimeoutExpired` peut sortir de la clause du délai avant
la pose du drapeau, sous Python 3.10. La fenêtre est de quelques instructions, mais elle est sur la VM qui exécute
`scale_run`.

### Correctif (`bench/scaling/scale_run.py`)

1. Correction minimale validée par le vérificateur : le drapeau de report `_LAUNCH['active']` est posé en première
   instruction des deux clauses de fermeture, remis à zéro après la fermeture (dans le `finally`, après la
   suppression du fichier de temps), et un signal différé pendant la fermeture après un délai est levé ensuite.
2. `stop(signum)` pose le drapeau AVANT de lever `Terminated` (gestionnaire et signaux différés) : dès qu'un signal
   agit, tout signal suivant est différé jusqu'au groupe fermé, quel que soit le point de contrôle de l'interpréteur.
3. La clause du délai est couverte par une clause de fermeture extérieure : une exception levée à son entrée (cas
   3.10) ferme quand même le groupe.
4. Docstring : la limite annoncée au premier tour était fausse ; elle est remplacée (§ 8, § 9).

Rien ne change sur le chemin normal : même instrument (`/usr/bin/time`), mêmes colonnes, même JSON natif.

### Porte (`mhgp10_regression_scale_run_timeout`, déjà enregistrée ; labels `gate;regression;fast`)

Contrôles ajoutés (les huit du premier tour sont gardés, chaque cas charge un module neuf) :

| Contrôle | Ce qu'il simule | Exigence |
| --- | --- | --- |
| `second_signal_pendant_appel` | proxy de `pthread_sigmask` (sonde du vérificateur) : second SIGTERM au premier point de contrôle de `close_group`, premier au lancement | `Terminated`, enfant et petit-enfant morts, injection faite une fois, signal suivant non différé |
| `second_signal_apres_delai` | même proxy après un délai | `Terminated` levé après la fermeture, aucun survivant |
| `second_signal_apres_exception` | même proxy après une exception qui n'est pas un signal (`OSError` simulée) | `OSError` propagée, aucun survivant, report levé |
| `balayage_lignes_delai`, `balayage_lignes_signal` | un signal traité à chaque frontière de ligne de `run_json` (fonction de trace, une exécution par ligne), chemins délai et signal ; couvre l'entrée des clauses `except` | `Terminated` et aucun survivant à chaque ligne ; au moins 10 lignes et une clause `except` couvertes |
| `close_group_vrai_signal_differe` | `close_group` seul, drapeau baissé, VRAI SIGTERM envoyé juste avant le SIGKILL de groupe | `Terminated` après la fermeture, aucun survivant |
| `signal_hors_appel_apres_delai` | signal traité hors appel après un délai sans signal | `Terminated` aussitôt (le report ne reste pas levé) |

Chaque simulation exige que son injection ait eu lieu (plancher contre le vert par vacuité).

Résultats (CTest exact, `run_expect.cmake`, `EXPECTED=0`, `EXPECT_LINE`) :

| Arbre | Résultat | Contrôles en échec |
| --- | --- | --- |
| r1 + porte finale (`build-r1`) | **Failed**, `code de sortie 1, attendu 0` | 5 contrôles : les trois `second_signal_*` (enfant et petit-enfant survivants) ; `balayage_lignes_delai` (5 lignes en échec, dont 150 à 152 de r1 : entrée de la clause du délai et appel de `close_group`) ; `balayage_lignes_signal` (6 lignes, dont 150, 158 et 159 : entrées des clauses) |
| final (`build`) | **Passed**, 22,3 à 22,6 s (charge 28 à 33) | aucun |

Preuves : [CTest r1](recu/preuves/e_ctest/ctest_r1_portes_finales.txt), [CTest final](recu/preuves/e_ctest/ctest_final_portes_V.txt),
[3.12](recu/preuves/a_signal/final/porte_delai_py312.txt), [3.10](recu/preuves/a_signal/final/porte_delai_py310.txt),
[interpréteurs](recu/preuves/f_interpretes/).

### Mutants

Porte figée, un mutant à la fois ([outil](recu/outils/mutants_scale_run.py)), Python 3.12 et 3.10
([3.12](recu/preuves/a_signal/mutants_python3/resume.jsonl.txt), [3.10](recu/preuves/a_signal/mutants_python3.10/resume.jsonl.txt)) :

| Variante | Ce qu'elle retire | 3.12 | 3.10 | Contrôles qui la tuent (3.12) |
| --- | --- | --- | --- | --- |
| `final` | code final (témoin) | survit | survit | - |
| `m1_sans_drapeau_clause_exterieure` | drapeau absent de la clause extérieure | tué | tué | `second_signal_apres_exception` |
| `m2_sans_drapeau_clause_delai` | drapeau absent de la clause du délai | tué | tué | `second_signal_apres_delai`, `balayage_lignes_delai` |
| `m3_stop_sans_drapeau` | `stop` lève sans poser le drapeau | tué | tué | `balayage_lignes_signal` |
| `m4_signal_differe_perdu_apres_delai` | signal différé jamais levé après un délai | tué | tué | `second_signal_apres_delai`, `balayage_lignes_delai` |
| `m5_drapeau_non_remis_apres_delai` | drapeau laissé levé après un délai | tué | tué | `signal_hors_appel_apres_delai`, `balayage_lignes_delai` |
| `m5b_drapeau_non_remis_apres_exception` | drapeau laissé levé après une exception | tué | tué | `second_signal_pendant_appel`, `second_signal_apres_exception` |
| `m6_sans_masque_close_group` | sans masquage dans `close_group` | tué | tué | `second_signal_pendant_appel`, `second_signal_apres_delai`, `second_signal_apres_exception`, `close_group_vrai_signal_differe` |
| `m7_clause_delai_plate` | clause du délai non couverte (structure plate) | tué | tué | `balayage_lignes_delai` |
| `r1_sans_report_des_signaux` | signal jamais différé (tour 1) | tué | non jouée | `signal_pendant_le_lancement`, `second_signal_pendant_appel`, `second_signal_apres_delai`, `second_signal_apres_exception`, `balayage_lignes_delai`, `balayage_lignes_signal` |
| `r1_sans_recolte_du_groupe` | groupe non récolté (tour 1) | tué | non jouée | `delai_enfant_et_petit_enfant`, `signal_pendant_le_lancement`, `second_signal_pendant_appel`, `second_signal_apres_delai`, `second_signal_apres_exception`, `balayage_lignes_delai`, `balayage_lignes_signal`, `close_group_vrai_signal_differe`, `run_delai_tour_tuee`, `run_sigterm_appel_tue`, `run_sigint_appel_tue` |
| `r1_sans_killpg` | `proc.kill` au lieu de `killpg` (tour 1) | tué | non jouée | `case_delay`, `signal_pendant_le_lancement`, `second_signal_pendant_appel`, `second_signal_apres_delai`, `second_signal_apres_exception`, `balayage_lignes_delai`, `balayage_lignes_signal`, `close_group_vrai_signal_differe`, `run_delai_tour_tuee`, `run_sigterm_appel_tue`, `run_sigint_appel_tue` |
| `r1_sans_nouvelle_session` | sans nouvelle session (tour 1) | tué | non jouée | `case_delay`, `signal_pendant_le_lancement`, `second_signal_pendant_appel`, `second_signal_apres_delai`, `second_signal_apres_exception`, `balayage_lignes_delai`, `balayage_lignes_signal`, `run_delai_tour_tuee`, `run_sigterm_appel_tue`, `run_sigint_appel_tue` |
| `r1_sans_controle_des_boules` | sans égalité des boules (tour 1) | tué | non jouée | `run_compteurs_de_boules_egaux` |
| `r1_sans_appels_natifs` | sans fichier d'appels (tour 1) | tué | non jouée | `run_ligne_ok_appels_natifs`, `run_compteurs_de_boules_egaux`, `run_delai_tour_tuee` |
| `r1_sans_gestionnaire_sigterm` | sans gestionnaire (tour 1) | tué | non jouée | `run_sigterm_appel_tue`, `run_sigint_appel_tue` |
| `v_sans_subreaper` | sans subreaper (vérificateur) | survit | non jouée | - |
| `v_delai_proc_kill` | délai fermé par `proc.kill` (vérificateur) | tué | non jouée | `delai_enfant_et_petit_enfant`, `second_signal_apres_delai`, `balayage_lignes_delai`, `run_delai_tour_tuee` |
| `etat_r1` | code du premier tour | tué | tué | `second_signal_pendant_appel`, `second_signal_apres_delai`, `second_signal_apres_exception`, `balayage_lignes_delai`, `balayage_lignes_signal` |
| `suggestion_verificateur` | correction minimale du vérificateur seule | tué | tué | `balayage_lignes_delai`, `balayage_lignes_signal` |
| `base_avant_r1` | base, avant le premier tour | tué | non jouée | `delai_enfant_et_petit_enfant`, `signal_hors_appel_apres_delai`, `signal_pendant_le_lancement`, `appel_normal_json_natif`, `second_signal_pendant_appel`, `second_signal_apres_delai`, `second_signal_apres_exception`, `balayage_lignes_delai`, `balayage_lignes_signal`, `case_close_group_real_signal`, `run_ligne_ok_appels_natifs`, `run_compteurs_de_boules_egaux`, `run_delai_tour_tuee`, `run_sigterm_appel_tue`, `run_sigint_appel_tue` |

Bilan : 17 mutants, 16 tués, 1 équivalent dans cet environnement (`v_sans_subreaper`, § 10). Les huit mutants du
correctif sont tués sous 3.12 et sous 3.10 ; le témoin `final` passe sous les deux. La passe 3.10 est restreinte aux
mutants du correctif et aux deux variantes (fichier [sous-ensemble](recu/outils/sous_ensemble_python3.10.txt)) : les
mutants de mécanique de groupe du premier tour ne dépendent pas de l'interpréteur. Le code r1 est tué par les trois
simulations et par les deux balayages. La correction minimale seule du vérificateur est tuée par les seuls balayages,
aux entrées de clause (lignes 150 et 151 du chemin délai, 150 et 162 du chemin signal) : c'est exactement la fenêtre
que Python 3.10 ouvre. `m6` (sans masquage) est tué par le plancher des trois simulations et, sur le comportement,
par le vrai SIGTERM de `close_group_vrai_signal_differe`.

### Re-mesure avec les vrais signaux (sondes du vérificateur)

Sondes du vérificateur, sans modification ([double_signal.py](recu/sondes_verif/double_signal.py),
[signal_dans_close_group.py](recu/sondes_verif/signal_dans_close_group.py)) : r1 contre final, CPython 3.12.1 et 3.10.21
(le Python de la sonde exécute aussi `scale_run` et les faux binaires). Deux vrais SIGTERM directs à un écart de
48 à 75 µs par pas de 3 (10 essais par écart) ; rafales de SIGTERM toutes les 20 µs pendant 3 ms ; enveloppe exacte
d'une étape du worker G4 (`setsid -w /usr/bin/time timeout --foreground --kill-after=10s`, puis `killpg` du groupe
de l'étape). Charge de la machine 12 à 22 ([script](recu/outils/vrais_signaux.sh.txt), [sorties](recu/preuves/g_vrais_signaux/charge_debut.txt)).

| Python | Code | Mode | Essais | Tours survivantes | Codes de sortie observés |
| --- | --- | --- | ---: | ---: | --- |
| 3.12 | r1 | bande 48–75 µs | 100 | **31** | 143 |
| 3.12 | r1 | rafale | 32 | **23** | −15, 1, 143 |
| 3.12 | final | bande 48–75 µs | 100 | 0 | 143 |
| 3.12 | final | rafale | 32 | 0 | −15, 143 |
| 3.12 | final | enveloppe G4 | 60 | 0 | −15 (code de `setsid`, tué avec le groupe de l'étape) |
| 3.10 | r1 | bande 48–75 µs | 100 | **19** | 143 |
| 3.10 | r1 | rafale | 32 | **11** | −15, 1, 143 |
| 3.10 | final | bande 48–75 µs | 100 | 0 | 143 |
| 3.10 | final | rafale | 32 | 0 | −15, 143 |
| 3.10 | final | enveloppe G4 | 60 | 0 | −15 (idem) |

Simulation déterministe du vérificateur : `SURVIVANTS pendant_appel=2 delai=2` pour r1, `pendant_appel=0 delai=0`
pour le final, sous 3.12 comme sous 3.10. Avec r1, chaque survie s'accompagne du code 143 et du message « tué et
récolté ». Avec le final, un −15 en rafale est un SIGTERM reçu pendant la finalisation de l'interpréteur, après la
fermeture de l'appel (§ 10). Ces sondes sont statistiques : elles confirment le défaut et son absence dans ces
conditions ; la preuve permanente est la porte déterministe.

## 2. B-b — quatre mutants non équivalents survivaient à la porte de complétude (P3)

Le code r1 refusait déjà ces quatre cas ; c'est la porte qui ne les isolait pas. Cas ajoutés, chacun arrêté par un
seul contrôle :

| Mutant | Contrôle retiré | Cas isolant | Pourquoi seul ce contrôle l'arrête |
| --- | --- | --- | --- |
| M09 | manifeste reconstruit comparé à l'épingle | `plan_altere_meme_nombre_sous_l_epingle` | tailles 8 000 et 32 000 au lieu de 8 000 et 16 000 : 32 scènes aussi ; lot cohérent avec ce plan, `run.json` déclare l'épingle d'origine |
| M10 | ligne de méthode inconnue | `methode_inconnue_ajoutee` | une ligne `inconnue` en plus, aucun couple connu retiré |
| N03 | plan de chaque session comparé à l'épingle | `fusion_session_unique_autre_plan` | une seule session : le contrôle entre sessions ne voit rien |
| N04 | manifeste reconstruit de la fusion comparé à l'épingle | `fusion_epingle_fausse` | épingle `f…f` ; la session déclare cette épingle et ce préenregistrement |

Les anciens cas restent (plan altéré à 48 scènes, méthode renommée, deux plans). N02 (fusion complète par compte)
est **équivalent** : après le refus explicite des plans dupliqués (§ 5), les noms du plan sont uniques, donc
`len(plan) = total` ; chaque scène fusionnée est dans le plan et fusionnée une seule fois, donc
`set(seen) ⊆ plan` et `|set(seen)| = len(seen)`. Alors `set(seen) == plan` équivaut à `len(seen) == total`.

Mutants de `decide.py` et `merge_sessions.py` ([outil](recu/outils/mutants_decide.py),
[résumé](recu/preuves/b_completude/mutants/resume.jsonl.txt)) :

| Mutant | Statut | Controles qui le tuent |
| --- | --- | --- |
| `final` | survit | - |
| `M01_plan_complet_ignore` | tue | `scene_manquante`, `scene_manquante_decision_nu` |
| `M02_hors_plan_ignore` | tue | `reprise_scene_hors_plan_en_plus` |
| `M03_doublon_ecrase` | tue | `doublon_autre_valeur`, `reprise_scene_entiere_recopiee` |
| `M04_metadonnees_ignorees` | tue | `metadonnee_famille`, `metadonnee_bruit_chaine`, `metadonnee_graine` |
| `M05_scores_non_verifies` | tue | `refus_hors_domaine` |
| `M06_run_incomplet_ignore` | tue | `run_declare_incomplet` |
| `M07_nombre_scenes_ignore` | tue | `run_autre_nombre_de_scenes` |
| `M08_plan_run_ignore` | tue | `run_autre_plan` |
| `M09_epingle_ignoree` | tue | `plan_altere_meme_nombre_sous_l_epingle` |
| `M10_methode_inconnue_ignoree` | tue | `methode_inconnue_ajoutee` |
| `M11_refus_hors_domaine` | tue | `refus_hors_domaine` |
| `M12_numpy_en_tete` | tue | `complet`, `complet_fusion_declare`, `sonde_audit_une_scene`, `sonde_audit_une_scene_decision_nu` (+55) |
| `M13_erreurs_non_bloquantes` | tue | `sonde_audit_une_scene`, `sonde_audit_une_scene_decision_nu`, `scene_manquante`, `scene_manquante_decision_nu` (+28) |
| `M14_colonnes_non_verifiees` | tue | `colonne_absente` |
| `N01_fusion_hors_plan` | tue | `fusion_scene_hors_plan` |
| `N02_fusion_complete_par_compte` | survit | - |
| `N03_fusion_plan_session_ignore` | tue | `fusion_session_unique_autre_plan` |
| `N04_fusion_epingle_ignoree` | tue | `fusion_epingle_fausse` |
| `D1_domaine_ignore` | tue | `ari_s_1_25_non_refuse`, `ari_s_au_dela_de_la_tolerance`, `ari_s_sous_moins_un_demi`, `ami_au_dessus_de_un` |
| `D2_borne_basse_ari_ignoree` | tue | `ari_s_sous_moins_un_demi` |
| `D3_borne_ami_ignoree` | tue | `ami_au_dessus_de_un` |
| `D4_domaine_sur_lignes_refusees` | tue | `temoin_refuse_nan_admis` |
| `D5_bornes_strictes` | tue | `bornes_exactes_acceptees`, `tolerance_declaree_acceptee`, `lot_A_archive`, `lot_C_archive` |
| `D6_sans_tolerance` | tue | `tolerance_declaree_acceptee` |
| `D7_tolerance_trop_large` | tue | `ari_s_au_dela_de_la_tolerance` |
| `D8_ami_negatif_refuse` | tue | `scores_negatifs_valides` |
| `E1_results_absent_plante` | tue | `results_csv_absent` |
| `E2_run_json_absent_plante` | tue | `run_json_absent`, `run_json_illisible`, `preenregistrement_absent` |
| `E3_plan_duplique_accepte` | tue | `plan_duplique_lot_dedoublonne` |
| `E4_json_non_objet_accepte` | tue | `run_json_non_objet` |
| `F1_fusion_pycache` | tue | `fusion_sans_B_aucun_pycache` |
| `F2_fusion_plan_duplique` | tue | `fusion_plan_duplique` |
| `F3_fusion_results_absent_plante` | tue | `fusion_results_csv_absent` |
| `F4_fusion_run_json_absent_plante` | tue | `fusion_run_json_absent` |
| `F5_fusion_colonnes_ignorees` | tue | `fusion_colonne_methode_absente` |
| `F6_fusion_doublon` | tue | `fusion_scene_deux_fois` |
| `F7_fusion_scene_incomplete` | tue | `fusion_scene_incomplete` |

Bilan : 38 mutants appliqués, 37 tués, 1 équivalent (N02, preuve ci-dessus). Les quatre survivants du premier tour
(M09, M10, N03, N04) sont tués chacun par son seul cas isolant. D5 (bornes strictes) est aussi tué par les lots A et C
archivés, qui contiennent des ARI_s égaux à 1 : les bornes exactes doivent être admises. Le témoin `final` passe.

## 3. B-c — score hors domaine admis et publié (P4)

### Reproduction sur r1

Cas `ari_s_1_25_non_refuse` de la porte finale joué sur `build-r1` : `nu code=0 : lot_conforme_au_plan`, puis
`numpy code=0 DECISION ECRITE` (`mean_ari_s.tour = 1,25`), comme dans la capture de l'audit continu. Même chose
pour ARI_s 1,000001, ARI_s −0,75 et AMI 1,5 ([CTest r1](recu/preuves/e_ctest/ctest_r1_portes_finales.txt)).

### Cause et correctif

`finite_scores` ne testait que la finitude. `decide.py` contrôle désormais le domaine des scores que la décision lit
réellement (ARI_s pour l'écart, AMI_nc pour la garde), sur les lignes non refusées, avec une tolérance déclarée :

| Score | Domaine | Source |
| --- | --- | --- |
| ARI_s | [−1/2, 1] | ARI de Hubert-Arabie ; maximum 1 (paires communes au plus la moyenne des paires internes) ; minimum −1/2 (Chacón et Rastrojo, 2023), documenté par scikit-learn 1.9.1, que `metrics.py` appelle |
| AMI_nc | ≤ 1, pas de borne basse | `adjusted_mutual_info_score` : « upperlimited by 1.0 », peut être négatif |
| tolérance | `TOL = 1e-9` | arrondi du calcul flottant ; `run_test.py` écrit 6 décimales, un ARI calculé ne dépasse 1 que de quelques ulp |

Une ligne refusée vaut 0 (EVAL_v2 D8) : ses scores ne sont pas lus, le témoin NaN reste admis. `ari_nc`, `coverage`
et `clusters` ne sont pas lus par la décision et ne sont pas contrôlés. Refus : code 2, ligne `REFUS : … lignes non
refusées à score hors domaine …`, rien n'est écrit.

### Porte

| Cas | r1 | final |
| --- | --- | --- |
| `ari_s_1_25_non_refuse`, `ari_s_au_dela_de_la_tolerance` (1,000001), `ari_s_sous_moins_un_demi` (−0,75), `ami_au_dessus_de_un` (1,5) | acceptés et décidés | refusés, sans décision, en Python nu et en décision complète |
| `bornes_exactes_acceptees` (1, −0,5, AMI 1,0), `tolerance_declaree_acceptee` (±5e−10 au-delà), `scores_negatifs_valides` (ARI −0,2, AMI −0,3), `temoin_refuse_nan_admis` | acceptés | acceptés et décidés |

Les mutants D1 à D8 (domaine ignoré, borne basse ignorée, borne AMI ignorée, domaine appliqué aux lignes refusées,
bornes strictes, tolérance nulle, tolérance 1e−3, AMI négatif refusé) sont au tableau du § 2.

## 4. B-d — échec causal de la porte de complétude sur l'ancien arbre (P5)

La porte juge maintenant chaque lot deux fois : en Python nu (`-S`, `--check-only`, contrat de la VM sans numpy) et,
si numpy est importable, en décision complète. Sur l'ancien `decide.py` (base, avant r1), l'échec n'est donc plus
seulement l'import de numpy sous `-S` : la décision complète écrit un `DECISION.json` sur des lots invalides.

Porte finale jouée sur l'arbre de base, numpy importable (`base_avant_r1` dans
[la campagne](recu/preuves/b_completude/mutants/resume.jsonl.txt), sortie complète
[ici](recu/preuves/b_completude/mutants/base_avant_r1.txt)) : code 1, 54 contrôles en échec.

| Lots invalides joués en décision complète (numpy) | Ancien `decide.py` |
| --- | ---: |
| décidés : code 0 et `DECISION.json` écrit | **27** |
| plantage, code 1 (colonne absente, fichiers absents ou illisibles, `run.json` non objet) | 6 |
| refusés, code 2 (couple manquant, méthode renommée, autre préenregistrement, lot C moins une ligne) | 4 |
| total | 37 |

Les 27 lots invalides décidés : `sonde_audit_une_scene` (et sa variante), `scene_manquante` (et sa variante),
`doublon_autre_valeur`, `reprise_scene_entiere_recopiee`, `scene_hors_plan_remplacante`,
`reprise_scene_hors_plan_en_plus`, `methode_inconnue_ajoutee`, les trois métadonnées, `score_non_fini`,
`refus_hors_domaine`, `run_declare_incomplet`, `run_autre_plan`, `run_autre_nombre_de_scenes`, les deux plans altérés,
les quatre scores hors domaine, les deux plans dupliqués, `fusion_partielle_refusee` et `lot_C_sonde_audit_decision_nu`.
L'échec de la porte sur l'ancien arbre est donc causal. Les lots valides n'y échouent qu'en mode 1 (`--check-only`
inconnu de l'ancien `decide.py`, code 2 d'argparse) ; en décision complète, l'ancien code les décide.

Même constat par le différentiel adverse du vérificateur, rejoué : l'ancien `decide.py` décide 12 lots invalides sur
16, le final les refuse tous ([diff_decide](recu/preuves/c_causalite/diff_decide_base_final.jsonl.txt)).

Sur r1 (variante `etat_r1`, et CTest au § 1), la porte finale échoue sur exactement 15 contrôles : les quatre scores
hors domaine, les cinq fichiers absents ou illisibles de `decide.py`, le plan dupliqué dédoublonné, et côté fusion le
plan dupliqué, les deux fichiers absents, la colonne absente et le `__pycache__`.

## 5. B-e — fichiers absents, `__pycache__`, spécifications dupliquées, reprise (P6, P7)

| Point | r1 | Correctif | Cas de la porte |
| --- | --- | --- | --- |
| `results.csv` absent | plantage, code 1 | `REFUS`, code 2 | `results_csv_absent` |
| `run.json` absent, illisible ou non objet ; préenregistrement absent | plantage, code 1 | `REFUS`, code 2 | `run_json_absent`, `run_json_illisible`, `run_json_non_objet`, `preenregistrement_absent` |
| fusion : `run.json`, `results.csv` ou colonne absents | plantage, code 1 | `REFUS`, code 2, rien d'écrit | `fusion_run_json_absent`, `fusion_results_csv_absent`, `fusion_colonne_methode_absente` |
| fusion lancée sans `-B` | écrit `bench/synthetic/__pycache__` | `sys.dont_write_bytecode = True` avant `import decide` | `fusion_sans_B_aucun_pycache` (copie jetable du banc ; témoin : un import ordinaire y écrit un cache) |
| plan aux spécifications dupliquées (tailles répétées) | lot dédoublonné **accepté et décidé** par `decide.py`, fusion écrite | refus explicite dans `decide.py` et `merge_sessions.py` | `plan_duplique_lot_dedoublonne`, `plan_duplique_lot_de_run_test`, `fusion_plan_duplique` |

Le rapport du vérificateur disait ce préenregistrement « désormais toujours refusé » : c'était vrai du lot tel que
`run_test.py` l'écrirait (doublons), faux d'un lot dédoublonné, que r1 décidait. Le refus est maintenant explicite et
documenté (docstrings de `decide.py` et `merge_sessions.py`). Les préenregistrements A et C n'ont aucun nom
dupliqué : leurs lots restent acceptés.

**P6, reprise.** `run_test.py` n'est pas modifié : il est épinglé par les préenregistrements A et C
(`pins.scripts_sha256`). `--resume` garde encore toute scène complète en méthodes, même hors plan ou recopiée ;
`decide.py` refuse le lot final qui en résulte (cas `reprise_scene_hors_plan_en_plus` et
`reprise_scene_entiere_recopiee`). Seul du calcul est perdu, aucune décision n'est possible. Sur G4,
`lot_runner.py` ne reprend que des scènes du plan et refuse une liste de scènes faites hors plan.

## 6. Portes et contrôles joués

Commandes exactes, depuis `/tmp/mhgp10-r2/bancs` (`S=src/morsehgp3D_v10/tests/regression`, `PY310` = CPython
3.10.21, même série que la VM G4 3.10.12, sans numpy ; `python3` = 3.12.1 avec numpy 2.5.3 et scikit-learn 1.9.1 ;
`/usr/bin/python3` = 3.12.3 sans numpy) :

| Commande | Résultat |
| --- | --- |
| `ctest --test-dir build-r1 --output-on-failure -R 'mhgp10_regression_(scale_run_timeout\|decide_completeness)$'` (portes finales copiées dans l'arbre r1, puis arbre restauré) | **0 % passés**, 2 échecs, `code de sortie 1, attendu 0` : 5 et 15 contrôles en échec ([sortie](recu/preuves/e_ctest/ctest_r1_portes_finales.txt)) |
| `ctest --test-dir build -V -R 'mhgp10_regression_(scale_run_timeout\|decide_completeness)$'` | **100 % passés** (2 sur 2) : 22,6 s et 34,6 s ; sortie de chaque contrôle en mode verbeux ([sortie](recu/preuves/e_ctest/ctest_final_portes_V.txt)) |
| `ctest --test-dir build --output-on-failure -L fast` | **4 sur 4** : `mhgp10_unit` 2,0 s, `mhgp10_regression_multiplicity_refusal` 0,5 s, `mhgp10_regression_scale_run_timeout` 22,3 s, `mhgp10_regression_decide_completeness` 31,0 s ([sortie](recu/preuves/e_ctest/ctest_final_fast.txt)) |
| `python3 $S/test_scale_run_timeout.py build`, puis `PY310`, `python3 -O`, `PY310 -O`, `/usr/bin/python3`, `python3 -X dev -W always` | code 0 et `scale_run_timeout_ok` sous les six interpréteurs ou modes, 19 à 25 s ; aucun avertissement sous `-X dev -W always` ([3.12](recu/preuves/a_signal/final/porte_delai_py312.txt), [3.10](recu/preuves/a_signal/final/porte_delai_py310.txt), [autres](recu/preuves/f_interpretes/delai_py310_O.txt)) |
| `python3 $S/test_decide_completeness.py build`, puis `python3 -O`, `/usr/bin/python3`, `/usr/bin/python3 -O`, `PY310`, `PY310 -O`, `python3 -X dev -W always` | code 0 et `decide_completeness_ok` partout ; avec numpy, 44 décisions complètes jouées (plancher 40), 30 à 43 s ; sans numpy, Python nu seul, 18 à 23 s ; aucun avertissement sous `-X dev -W always` ([3.12 numpy](recu/preuves/b_completude/final/porte_completude_py312_numpy.txt), [3.10](recu/preuves/b_completude/final/porte_completude_py310_sans_numpy.txt), [`-X dev`](recu/preuves/f_interpretes/completude_py312_Xdev.txt)) |
| `python3 -B outils/mutants_scale_run.py …` (3.12 : 17 mutants et 3 variantes ; 3.10 : 8 mutants du correctif et 2 variantes) | voir § 1 |
| `python3 -B outils/mutants_decide.py src gel preuves/b_completude/mutants etat_r1=src-r1 base_avant_r1=src-base` | voir § 2 et § 4 |
| `python3 -B sondes_verif/signal_dans_close_group.py <scale_run.py>`, r1 et final, 3.12 et 3.10 | voir § 1 |
| `python3 -B sondes_verif/double_signal.py <scale_run.py> <travail> direct2 10 <écart>` (48 à 75 µs par pas de 3), `rafale 32 20`, `groupe 60` | voir § 1 |
| `python3 -B sondes_verif/diff_decide.py src-base src <travail>` | 23 cas, 0 non conforme ; l'ancien décide 12 lots invalides sur 16 |
| `python3 -B sondes_verif/redecide_refusion.py src-base src <travail>` | § 7 |
| `outils/differentiels.sh.txt` (`scale_run.py run`, base, r1 et final, puis `compare_scale.py`) | § 7 |
| `git apply --check` puis `git apply` de `bancs_r2.patch` sur une extraction neuve de `56020cab6` ; `git am series/*.patch` sur une autre | arbres identiques entre eux et à l'arbre final ; `git apply --check` propre aussi sur `e9eab2754` |

Aucun `__pycache__` n'est écrit dans l'arbre source par les portes : après un dernier passage CTest des deux portes,
le contrôle est vide ([contrôle](recu/preuves/f_interpretes/pycache_apres_rejeu.txt), [passage](recu/preuves/f_interpretes/ctest_controle_pycache.txt)).
Un seul cache avait été trouvé avant ce passage : `tests/regression/__pycache__`, écrit à 03 h 49 par un de mes
contrôles ad hoc qui chargeait le module de la porte par `importlib` pour fabriquer une fixture ; supprimé. Aucun
processus dormeur ne reste : les portes tuent par PID ce qu'elles ont lancé.

## 7. Sorties sur entrées valides

Aucune sortie existante ne change sur une entrée valide. Les preuves ont été jouées avec les fichiers finaux
(copies figées `gel/`, sha256 au § 11).

| Différentiel | Résultat | Preuve |
| --- | --- | --- |
| Re-décision du lot C, décision complète (numpy) | base = final = archive, octet pour octet : `DECISION.json` `847a238d…`, `DECISION.md` `e92d3b62…`, même stdout | [redecide](recu/preuves/d_differentiels/redecide_refusion_base_final.jsonl.txt) |
| Re-décision du lot A | base = final : `DECISION.json` `286f86a1…`, `DECISION.md` `85d13cba…` (= archive) ; le JSON archivé diffère des deux par la seule clé `secondary: {}` ajoutée par `c764e121a`, avant le correctif | idem |
| `--check-only` en Python nu sur A et C | acceptés (960 × 8, 960 × 32) | idem, et porte (`lot_A_archive`, `lot_C_archive`) |
| Re-fusion des sessions c1 (338) et c2 (622) du lot C | base = final = final sous `-S` = archive : `results.csv` `313312ef…`, `run.json` `ab6df492…`, `done.u32le` `c55e01ff…` | idem |
| Différentiel adverse du vérificateur, décision complète | 5 lots valides (CRLF, LF, lignes mélangées, colonne en plus, fusion déclarée) : décisions identiques à l'ancien code (`150ba480…`) ; témoin `refused=1` à NaN : identique ; ARI 1,25 : maintenant refusé | [diff_decide](recu/preuves/c_causalite/diff_decide_base_final.jsonl.txt) |
| `scale_run.py run`, vrais binaires, 1 fil : K = 5 sur 5 entrées (8 000 à 16 000 sites synthétiques, deux quarts LiDAR ; 147 666 à 1 178 845 boules), K = 10 sur une entrée (3 106 098 boules) | base, r1 et final : 22 colonnes déterministes identiques, en-têtes identiques ; chaque ligne `ok` du final est cohérente avec ses deux JSON natifs | [K5 base/final](recu/preuves/d_differentiels/compare_k5_base_final.txt), [K5 r1/final](recu/preuves/d_differentiels/compare_k5_r1_final.txt), [K10](recu/preuves/d_differentiels/compare_k10_base_final.txt) |

C'est un différentiel de sorties, pas une mesure de coût : les temps varient avec la charge de la machine (charge 20
à 26). Les nouveaux refus ne portent que sur des entrées invalides : score hors domaine, fichier absent ou illisible,
plan dupliqué, colonnes de session absentes ou différentes. `decide.py` change encore de sha256 : un préenregistrement
futur épinglera la version intégrée ; les préenregistrements A et C, déjà exécutés, ne sont pas rejoués.

## 8. Errata du reçu du premier tour (`RECU_bancs.md`)

| Énoncé du premier tour | Correction |
| --- | --- |
| § 7 et docstring de `scale_run.py` : « Seul SIGKILL envoyé à `scale_run` lui-même, sans SIGTERM préalable, laisse l'appel en cours vivre dans sa propre session. » | Faux. Deux SIGTERM rapprochés le laissaient aussi vivre (vérificateur : 7 survies sur 204 de 0 à 150 µs, 21 sur 100 dans la bande de 48 à 75 µs, 18 sur 32 en rafale ; simulation déterministe : enfant et petit-enfant survivants), avec le code 143 et le message « tué et récolté ». Corrigé au § 1. |
| § 2 : « SIGINT/SIGTERM/SIGHUP sont bloqués pendant la fermeture puis délivrés après. » | Incomplet : le masque ne retient pas le gestionnaire Python d'un signal déjà reçu, ni un signal traité à l'entrée de `close_group`. |
| § 1 (c), tableau du § 3 : la porte de complétude échoue 29 fois sur 35 sur l'arbre non corrigé. | L'échec venait surtout de l'import de numpy sous `python3 -S` (code 1), pas de la logique E1 ; seul `fusion_scene_hors_plan` échouait pour la vraie cause. La preuve causale était à part (`c3`). La porte finale est causale d'elle-même (§ 4). |
| § 3 : « 18 variantes incohérentes refusées en 20 contrôles », présentées comme contrôles de chaque vérification. | Quatre vérifications n'étaient isolées par aucun cas (épingle du manifeste, méthode inconnue ajoutée, plan par session, épingle de la fusion) : les mutants M09, M10, N03, N04 survivaient. Le cas « plan altéré sous la même épingle » était arrêté par le nombre de scènes. |
| § 2 : `decide.py` exige « ARI_s et AMI finis ». | Vrai mais insuffisant : un ARI_s de 1,25 était accepté puis publié (`mean_ari_s.tour = 1,25`). Domaine contrôlé au § 3. |
| § 7 : « `decide.py`, lignes 266–267 préexistantes (`json.load(open(...))`) : deux ResourceWarning ». | Corrigé en passant (lecture par `with`). Il reste un avertissement préexistant dans `load()` (§ 10). |
| Vérificateur, P7 : préenregistrement aux spécifications dupliquées « désormais toujours refusé ». | Vrai pour le lot que `run_test.py` écrirait (doublons) ; faux pour un lot dédoublonné, que r1 décidait. Refus explicite au § 5. |

## 9. Lignes proposées pour l'intégrateur (B-f)

Aucun de ces fichiers n'est modifié par ce correctif. Lignes à ajouter par l'intégrateur, une par fichier ; le nom
du dossier de reçu est à fixer par lui (`<recu_bancs>` ci-dessous).

`receipts/ERRATA.md` (une ligne du tableau) :

```text
| `<recu_bancs>` (premier tour, `RECU_bancs.md`) | « Seul SIGKILL envoyé à scale_run lui-même, sans SIGTERM préalable, laisse l'appel en cours vivre » ; la porte de complétude échoue 29 fois sur 35 sur l'arbre non corrigé ; 18 variantes refusées présentées comme contrôles isolés. | Deux SIGTERM rapprochés faisaient aussi survivre l'appel, avec le code 143 et « tué et récolté » (vérificateur : 21 survies sur 100 dans la bande de 48 à 75 µs) ; corrigé au second tour, simulation gravée dans la porte. L'échec de la porte venait surtout de l'import de numpy sous -S ; quatre contrôles n'étaient isolés par aucun cas (M09, M10, N03, N04) ; un ARI_s de 1,25 était accepté et publié. | Vérificateur adverse des bancs (30 septembre 2026), P1 à P5 ; `RECU_bancs_r2.md` |
```

`PASSATION.md`, tableau « Acquis » (une ligne) :

```text
| Bancs sûrs : délai et signaux de `scale_run` (groupe de l'appel tué et récolté, signaux rapprochés différés jusqu'au groupe fermé, y compris sous Python 3.10) ; décision refusée hors du plan exact (complétude, épingle, domaine ARI_s [−1/2, 1] et AMI ≤ 1 à 1e−9, fichiers, plan dupliqué) ; fusion sans scène hors plan ni `__pycache__` | `bench/scaling/scale_run.py`, `bench/synthetic/decide.py`, `bench/g4/merge_sessions.py` | `mhgp10_regression_scale_run_timeout`, `mhgp10_regression_decide_completeness` (label `fast`) ; reçu `<recu_bancs>` (deux tours) |
```

## 10. Limites et points ouverts

- **SIGKILL de `scale_run` lui-même**, sans SIGTERM préalable : l'appel en cours vit dans sa propre session. Limite
  inchangée ; la fermer exigerait de remplacer `/usr/bin/time` (instrument de mesure) par `wait4` et
  `PR_SET_PDEATHSIG`.
- **Double faute** : une exception interne qui n'est ni un signal ni le délai (`OSError` de `communicate`,
  `MemoryError`), suivie d'un signal traité exactement à l'entrée de la clause de fermeture extérieure sous
  Python 3.10. Fenêtre de quelques instructions, non couverte ; écrite dans la docstring. Le second signal pendant
  `close_group` après une telle exception, lui, est couvert (`second_signal_apres_exception`).
- **Code de sortie en rafale** : un SIGTERM reçu pendant la finalisation de l'interpréteur, après la restauration des
  dispositions par défaut, termine `scale_run` par le signal (−15) au lieu de 143. L'appel est déjà fermé. Observé par
  le vérificateur sur sa variante ; voir la re-mesure (§ 1).
- **Mutant `v_sans_subreaper`** (pas de `PR_SET_CHILD_SUBREAPER`) : équivalent là où le PID 1 récolte les orphelins
  (`docker-init` ici, systemd sur la VM) ; `close_group` attend alors que le groupe soit vide. La porte ne peut pas
  le distinguer dans ces environnements.
- **Balayage** : injection aux frontières de ligne (événements `line`) du seul cadre de `run_json`, pas à chaque
  instruction ; `close_group` est couvert par le proxy de `pthread_sigmask` et par le vrai signal. Les sondes à vrais
  signaux sont statistiques : leur absence de survie n'est pas une preuve ; la porte repose sur les simulations
  déterministes.
- **Borne basse de l'ARI** : −1/2 est invoquée (Chacón et Rastrojo, 2023 ; documentation de scikit-learn 1.9.1), pas
  re-démontrée ici ; le dénombrement des paires donne seul −1. La tolérance 1e−9 couvre l'arrondi du calcul.
- **AMI** : aucune borne basse. Dans un cas dégénéré, le plancher du dénominateur de scikit-learn peut porter un AMI
  calculé au-delà de 1 + 1e−9 : le lot serait refusé, avec la raison écrite. C'est le côté sûr (aucune décision
  fausse), pas un cas observé.
- **`decide.py`, `load()`** : un `ResourceWarning` préexistant sous `-X dev` en décision complète (fichier ouvert sans
  `with`). La partie statistique n'est pas modifiée, pour que les décisions restent identiques octet pour octet.
- **`run_test.py --resume`** : inchangé (épinglé). Seul du calcul est perdu (§ 5).
- **Portes plus longues** : 20 à 30 s chacune sous charge 20 à 26 (7 s au premier tour), dans le délai CTest de
  300 s et le budget `fast` (5 min). Sur une VM sans numpy, la porte de complétude ne joue que le Python nu ; son
  plancher de décisions complètes (40) ne s'applique que si numpy est importable.
- **Non rejoué** : `mhgp10_catalogue_oracle` et `mhgp10_tower_oracle` (15 et 20 min), comme demandé ; aucun C++
  modifié, binaires identiques à `v10-wt` ; ASan, UBSan et TSan sans objet. L'intégration finale jouera la suite
  complète.
- **Non modifiés** : `README.md`, `PASSATION.md`, `receipts/ERRATA.md` (lignes proposées au § 9), `run_test.py`.

## 11. Fichiers

| Fichier | sha256 |
| --- | --- |
| `bancs_r2.patch` (base `56020cab6` → final, 6 fichiers, chemins `morsehgp3D_v10/…`) | `e47ab9231a47b05290e84de00eecf7d0c37fd6988dd62c3b8d1a1079edd16584` |
| `series/0001-r1.patch` (patch du premier tour tel que commité) | `24a16b1cc6e9905aacd35ed7a340fb91988cf21631818b0742ca9be79173c820` |
| `series/0002-bancs-r2-scale_run-signal-race-closed-and-engraved-i.patch` | `dbc6b56cab1ca612b3386a3581f425e8f908a023c5e23c2654902fa6c931fb2f` |
| `series/0003-bancs-r2-score-domain-missing-files-and-duplicated-p.patch` | `20de7c8c19f7faebaf60e592ce8b036dead4b6b5502815d5c1f2169a5cac0a67` |
| `morsehgp3D_v10/bench/scaling/scale_run.py` | `14f3915daef73088360cf5d90be1a76aab81666ddb22764dc2ea003822714dea` |
| `morsehgp3D_v10/bench/synthetic/decide.py` | `de5888f52d6b6f249bafc77c167ed3dea8c866b8642797c2cd0175dd2b5a5150` |
| `morsehgp3D_v10/bench/g4/merge_sessions.py` | `3ebd3a216b8fdf4a23be1a07964e57c5eab82ad750d8edbb9eefd88008a9ff1b` |
| `morsehgp3D_v10/tests/regression/test_scale_run_timeout.py` | `ebd58d604c801e8e41a1d0c27ef69a624a65954314c33b159860d77cc4d3f2ff` |
| `morsehgp3D_v10/tests/regression/test_decide_completeness.py` | `f55b2b61911cfefecd0f400b0e8a4d9aeae37d47cd9266e7c42d9a44cb5c1ad7` |
| `morsehgp3D_v10/CMakeLists.txt` | `6cee9dd092547593f7c7fab764e7be56c40cf4db902e57744404220d356b8f5e` |

Les deux portes gardent leur nom, leur code attendu (0), leur ligne finale et leurs labels (`gate;regression;fast`,
délai 300 s) ; seul le commentaire d'enregistrement de `CMakeLists.txt` change.

Contenu de `recu/` (texte seulement ; les `.jsonl`, `.csv`, `.log`, `.err` et `.sh` y portent le suffixe `.txt`,
octets inchangés ; pour les mutants, seules les lignes en échec et la dernière ligne de chaque sortie) :

- [RECU_bancs.md](recu/RECU_bancs.md) : reçu du premier tour, copie ;
- [essais/](recu/essais/fenetre_clause.py) : point de contrôle des signaux en 3.10 et 3.12, injection par trace ;
- [outils/](recu/outils/mutants_scale_run.py) : bancs de mutants, différentiels, re-mesure, assemblage ;
- [sondes_verif/](recu/sondes_verif/ORIGINE.txt) : sondes du vérificateur rejouées ;
- `preuves/` : `a_signal` (B-a), `b_completude` (B-b, B-c, B-e), `c_causalite` (B-d), `d_differentiels` (§ 7),
  `e_ctest`, `f_interpretes`, `g_vrais_signaux`.

`SHA256SUMS` (dossier de travail) couvre le patch, la série, les deux reçus et tout `recu/`.
