# Preuves de la lentille L01 (mathématiques du catalogue v10)

2 octobre 2026. Scripts, journaux, comptes et empreintes de l'audit `L01_MATH_CATALOGUE.md` (dossier parent). Aucune coordonnée KITTI. `GCP non utilisé`.

Les scripts ont tourné sous `/tmp/v11-audit/l01_math_catalogue/` contre un build Release de la copie des sources v10 (identiques au worktree). Ils gardent ces chemins : pour les rejouer, construire la v10 hors dépôt et adapter le dossier.

- Oracle indépendant : `oracle_indep.py`, `campagne_20261002_per10.log`.
- Euler à l'échelle : `euler_juge.py`, `euler_lidar00.txt`, `euler_lidar01.txt`, `euler_lidar02.txt`, `mutants_euler.txt`.
- Restriction : `jkm2.sh`, `jkm2_lidar.txt`.
- Juge d'échantillon : `j1_echantillon.sh`, `j1_juge.py`, `j1_lidar02_k10.txt`.
- Tour contre l'oracle à K = 10 : `tower_oracle_k10.py`, `tower_oracle_k10.log`.
- Coût : `amas_coins_14.u32le`, `petite_feuille_rejeu.sh`, `petite_feuille_rejeu.log`, `gen_amas.py`, `amas_rejeu.sh`, `amas_rejeu.log`.
- Dégénérescences : `degeneres_rejeu.sh`, `degeneres.txt`.
- Comptes : `run_lidar.sh`, `catalogue_lidar_comptes.txt`, `tour_lidar02_comptes.txt`, `doublons_kitti.txt`.
- Représentation des niveaux : `c9_niveau.py`, `c9_niveau.log`.
- Porte du dépôt rejouée : `e1_test_catalogue_oracle.log`.
- `empreintes_fichiers_restes_sous_tmp.txt` : sha256 et tailles des fichiers à coordonnées laissés sous `/tmp`.
- `SHA256SUMS` : empreintes de ce dossier.
