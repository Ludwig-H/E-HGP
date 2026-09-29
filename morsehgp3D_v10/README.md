# Morse HGP 3D v10

Ouverte le 28 septembre 2026, sur `main`. Base de code **neuve** : la v9 est un sujet différentiel et une
source de fixtures, jamais une base de code.

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
public_status=not_claimed
```

## Objet et objectifs

- **Objet (inchangé)** : pour k = 1..Kmax, la tour FULL de π0(L_k(a)), L_k(a) = région couverte par au moins
  k boules de rayon √a (hiérarchie K-NN : naissances, multifusions, continuations datées, plateaux,
  extension non régulière, verticales), niveaux = rayons carrés rationnels exacts.
- **Complexité** : jugée d'abord sur le régime LiDAR (trames SemanticKITTI sans sol, 30–60 k sites, grille
  1 mm = 18 bits, K5 puis K10, 1 s puis 100 ms sur G4) ; tailles d'intérêt 8k/16k/32k pour les pentes.
- **Clustering** : tirer de la tour une hiérarchie de points, la condenser comme HDBSCAN et la sélectionner, puis la
  confronter à `sklearn.cluster.HDBSCAN` (adversaire utilisé tel quel, jamais réimplémenté) à K = `min_samples`, sur
  des bancs synthétiques préenregistrés.

## Décisions d'ouverture (audit critique de la v9)

- Générateur par **boîtes de centres** à certificats de gardes et de dominateurs (lentille L13) : exact,
  linéaire sur toutes les familles mesurées, là où le front WSPD v9 est quasi quadratique sur amas et
  cubique sur coquilles. Le WSPD et le paramètre s disparaissent.
- Tour par **minima fixes, morceaux locaux (Gordan) et descente**, validée contre l'oracle Γ_k
  (dégénérescences comprises) par la référence exacte `reference/hgp10_ref.py`.
- Doublons : le banc les retire (`quantize18`) ; la tour refuse encore explicitement une entrée pondérée
  (multiplicités), sous une raison de refus à corriger (audit du 29 septembre, IMP-16).
- Un seul chemin par défaut, pas de levier sans ablation, pas de mutant dans le code produit.

## État

| Couche | Fichiers | Porte |
| --- | --- | --- |
| fondations | `src/core`, `src/sched` (dont le tri parallèle), `src/arith` | `mhgp10_unit` |
| nuage | `src/cloud` (sites Morton, index `SiteTree`) | `mhgp10_unit` |
| catalogue | `src/catalogue` (boîtes de centres, frontière pilotée par la charge, assemblage parallèle) | `mhgp10_catalogue_oracle` |
| tour | `src/tower` (tous ordres, verticales, entrées `core` et `cover`) | `mhgp10_tower_oracle`, `mhgp10_points_cover`, `mhgp10_regression_*` |
| tête | `src/points/dendrogram.*`, `src/head` (condensation exacte, EOM, feuilles, λ = r^(−z)) | `mhgp10_head_condensation_vs_sklearn` |
| banc | `bench/synthetic` (préenregistrements, `run_test.py`, `decide.py`), `bench/scaling`, `bench/g4` | `mhgp10_regression_batch_equivalence`, `mhgp10_regression_mreach_border` |
| référence | `reference/hgp10_ref.py` (Γ_k, catalogue, tour, C∩X en Fraction) | `reference/test_ref.py` |

## Résultats (29 septembre 2026, `public_status=not_claimed`)

- **Clustering, tests préenregistrés** : à K = `min_samples`, la tour bat sklearn HDBSCAN.
  - Lot A, tête C∩X, 960 scènes de 8 000 à 32 000 points : K = 1, 2, 3 (reçu `receipts/test_kmatch_A_20260929`).
  - Lot C, tête v10-b (entrée par première couverture, EOM avec λ = r^(−z)) : **tous les K de 1 à 10**, Δ de +0,031
    à +0,091, et de +0,034 à +0,105 sans remplissage (reçu `receipts/test_cover_C_20260929`).
- **Audit** (`audits/audit_hierarchie_knn_20260929`) : la tour est l'arbre plug-in exact de l'estimateur K-NN.
  - À même entrée et même tête, la hiérarchie d'HDBSCAN fait jeu égal ; le lot C le confirme (famille « objet »).
  - L'avantage vient donc de l'entrée des amas discrets et de la tête. L'axe K de la tour reste inexploité.
- **Performance LiDAR** (G4, CPU seul, 48 fils ; reçu `receipts/g4_session2_perf_20260929`) :
  - catalogue et tour à K = 5 en 0,31 à 0,37 s par trame sans sol ;
  - à K = 10, en 1,2 à 1,5 s ;
  - chaîne jusqu'aux étiquettes à K = 5, en 0,46 à 0,56 s.

## Construction

```bash
cmake -S morsehgp3D_v10 -B build/v10 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v10 --parallel
ctest --test-dir build/v10 --output-on-failure
python3 -m unittest discover -s morsehgp3D_v10/reference -p 'test_*.py'   # référence exacte (≈ 6 min)
```

Aucune dépendance externe dans le produit (C++20, `-Werror`) ; les portes Python utilisent NumPy et scikit-learn.
Sessions G4 : `gcp-migration/v10_session.py` (voir `gcp-migration/README_V10.md`). La VM n'a ni pip ni numpy :
`bench/g4/lot_runner.py` et `bench/g4/pyenv_run.py` y exécutent les campagnes Python avec un Python portable et des
binaires liés statiquement, envoyés comme données.
