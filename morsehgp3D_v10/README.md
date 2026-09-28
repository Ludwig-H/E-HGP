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
- **Clustering** : tirer de la tour la hiérarchie de points C∩X (x entre à D_K(x), composante de L_K qui le
  contient), la condenser comme HDBSCAN, et la confronter à `sklearn.cluster.HDBSCAN` (adversaire utilisé
  tel quel, jamais réimplémenté) sur des bancs synthétiques préenregistrés.

## Décisions d'ouverture (audit critique de la v9)

- Générateur par **boîtes de centres** à certificats de gardes et de dominateurs (lentille L13) : exact,
  linéaire sur toutes les familles mesurées, là où le front WSPD v9 est quasi quadratique sur amas et
  cubique sur coquilles. Le WSPD et le paramètre s disparaissent.
- Tour par **minima fixes, morceaux locaux (Gordan) et descente**, validée contre l'oracle Γ_k
  (dégénérescences comprises) par la référence exacte `reference/hgp10_ref.py`.
- Doublons = **multiplicités** (poids), jamais un refus.
- Un seul chemin par défaut, pas de levier sans ablation, pas de mutant dans le code produit.

## État

| Couche | Fichiers | Porte |
| --- | --- | --- |
| fondations | `src/core`, `src/sched`, `src/arith/wide.hpp` | `mhgp10_unit` (entiers larges contre un juge décimal, ordonnanceur, statuts, tampons) |
| nuage | `src/cloud` (sites Morton, multiplicités, index `SiteTree`) | `mhgp10_unit` (permutation, D_K exact) |
| tête | `src/points/dendrogram.*`, `src/head` (condensation HDBSCAN exacte, EOM, feuilles, λ = r^(−z)) | `mhgp10_head_condensation_vs_sklearn` (K = 1, 2 : égalité avec sklearn) |
| référence | `reference/hgp10_ref.py` (Γ_k, catalogue, tour, C∩X en Fraction) | `reference/test_ref.py` (E5, génériques, grilles) |

À venir : catalogue C++ (générateur par boîtes), tour FULL, producteur C∩X, banc contre HDBSCAN.

## Construction

```bash
cmake -S morsehgp3D_v10 -B build/v10 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v10 --parallel
ctest --test-dir build/v10 --output-on-failure
python3 -m unittest discover -s morsehgp3D_v10/reference -p 'test_*.py'   # référence exacte (≈ 6 min)
```

Aucune dépendance externe dans le produit (C++20, `-Werror`) ; la porte de la tête utilise Python 3,
NumPy et scikit-learn. GCP non utilisé à ce stade.
