# Relecture de MES-M6 : changements fichier par fichier (lot 3)

7 octobre 2026. Base HEAD de `main` `58761b36d` ; correctif `../patch_lot3.diff` (chemins `a/morsehgp3D_v12/...`,
`b/morsehgp3D_v12/...`, applicable par `git apply` à la racine du dépôt). Seule la relecture `--rejuger` de MES-M6
change ; le passage (`run`), le banc et les autres juges sont inchangés.

| Fichier | Changement | Constat | Test |
| --- | --- | --- | --- |
| `microbancs/mes_m6_session/run_m6.py` | `is_int` strict (`type(x) is int`) ; liste fermée `KNOWN_SCHEMAS` (`v1` historique nommé, `v2`), tout autre schéma refusé ; rapport v1 aux champs exacts `V1_FIELDS` et prises aux champs exacts `V1_RUN_FIELDS` ; identifiants de prise typés et dans la plage avant toute comparaison, entrée illisible ou en double refusée ; effectif validé : ensemble des prises lues et conformes égal aux `3 × processus` attendues ; codes de compilation et de prise, nombre de lignes en entiers stricts ; schéma lu publié dans la sortie ; docstring | `CST-0018` (reçu `audit_reprise_20261007/juges`) | `tests/test_juge_m6.py` 68/0 (3.10, 3.12, normal, `-O`) ; pilote du HEAD : 10 cas nouveaux relus code 0 |
| `microbancs/mes_m6_session/tests/test_juge_m6.py` | 14 cas : les 6 injections de l'auditeur (indice `false`, indice `0.0`, indices flottants sans fichiers ni médianes, schéma `v999` avec refus publié, code de compilation `false`, code de prise `false`) et 8 gardes seules (prise retirée, en double, indice hors plage, schéma absent, effectif flottant ; session A au schéma `v0`, avec refus publié, avec une prise au champ inconnu) ; `FALSIFICATIONS_V2` accepte une action sur le dossier publié | idem | 68 cas, 0 écart |
| `microbancs/outils/mutants_juges.py` | 9 mutants nouveaux, un par garde (`m6_entiers_laxistes`, `m6_rejuge_identifiants_non_types`, `m6_rejuge_doublons_admis`, `m6_rejuge_effectif_non_valide`, `m6_rejuge_schema_ouvert`, `m6_rejuge_v1_champs_libres`, `m6_rejuge_v1_prises_libres`, `m6_rejuge_code_compilation_laxiste`, `m6_rejuge_code_de_prise_laxiste`) ; `m6_rejuge_code_ignore` remis au nouveau code ; 90 au total | idem | voir `RAPPORT.md` § 4 |
| `microbancs/README.md` | état de MES-M6 (relecture) et de l'outil de mutants (90) | — | règles de `tools/check_docs.py` rejouées |
