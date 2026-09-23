# Annexe de l'audit C du 23 septembre 2026 — lectures, scripts, vérifications

Pièces justificatives de
[AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md](../AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md).
Hors produit, hors registre, `public_status=not_claimed`, GCP non utilisé.
Aucune donnée SemanticKITTI n'est versionnée ici (seulement des nombres
recalculés depuis les reçus publiés et de petites fixtures synthétiques).

## `lectures/`

Neuf lectures indépendantes, menées en parallèle sur `origin/main` `0125dc18`
(puis recoupées jusqu'aux commits cités dans chaque rapport) :

| fichier | lentille |
| --- | --- |
| `L1_rapport.md` | objet mathématique, statuts, écarts à la définition normative |
| `L2_rapport.md` | générateur q2 (front WSPD, témoins, Pool, census) |
| `L3_rapport.md` | générateur q3/q4 (filtres, cœur, cover, voies mortes, atlas, induction q3) |
| `L4_rapport.md` | chaîne, catalogue, `complete_relative`, porte T2 |
| `L5_rapport.md` | reconstruction de la tour FULL |
| `L6_rapport.md` | portes, oracles, mutants, CI |
| `L7_rapport.md` | mesures G4 R1 → R7b, écart au contrat, chronomètre |
| `L9_rapport.md` | pistes fermées, invariants, parallélisme et GPU |

Chaque `*_constats.json` donne les constats bruts de la lecture (gravité,
preuves fichier:ligne, action). **Ce sont des constats avant vérification** :
le document principal ne retient que les constats revérifiés par l'auditeur C
ou confirmés par la vérification adverse, et signale les autres comme en
cours. La lecture L8 (hygiène du dossier) alimente l'index
`morsehgp3D_v9/audits/README.md`, publié séparément.

## `scripts/`

Scripts de contrôle écrits par les lectures et les vérificateurs, tous
exécutés sous `nice -n 19` avec au plus deux fils : oracles Python exacts
(fractions ou entiers), petits harnais C++ liés à la bibliothèque v9 publiée,
recalculs sur les sorties brutes des reçus G4. Quelques-uns référencent les
chemins du poste de l'auditeur ; ils se relancent en adaptant ces chemins.

| dossier | contenu |
| --- | --- |
| `lentille1/` | `lower_link_pi0.py` (lien inférieur, u = 2, 3, 4), `e5_gabriel_only.py` (fixture E5), `hgp_block_oracle.py` (extension non régulière, 33 nuages dégénérés) |
| `lentille2/` | `check_q2_bounds.py` (7 315 contrôles de bornes), `check_knn_invariant.py`, `q2costs.py` |
| `lentille3/` | `bounds_u18.py` (23 majorants q3/q4 à 18 bits) |
| `lentille4/` | `oracle_catalogue.py`, `chain_dump.cpp`, `compare.py`, fixtures (200 appels, 24 472 lignes égales à l'oracle) |
| `lentille5/` | `check_level_filter.py` (filtre flottant des niveaux), `sizes.cpp` |
| `lentille6/` | `diam_depth.py` (vacuité de l'élagage à K = 10 sur T2), liste des tests CI |
| `lentille7/` | `table_g4.py`, `analyse.py`, `scaling_laws.py` et leurs sorties (109 exécutions G4) |
| `lentille9/` | `spawn.cpp` (coût de création d'un fil) |
| `verif_*`, `l4-*`, `L4-*` | contrôles des vérificateurs adverses (sondes d'omission, mesures d'allocation, fixture u13 de A) |
