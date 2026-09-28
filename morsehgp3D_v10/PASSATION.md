# Passation v10

État au 28 septembre 2026. Ce document décrit l'état ; l'historique est dans `git log`.

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
public_status=not_claimed
GCP : non utilisé à ce jour
```

## Lire d'abord

1. [README](README.md) : état court et commandes.
2. [Spécification](docs/SPEC_V10.md) : l'objet tel qu'implémenté, les portes, ce qui n'est pas établi.
3. [Audit critique de la v9](audits/audit_v9_20260928/AUDIT_V9_CRITIQUE_20260928.md) et la
   [base de connaissances](audits/audit_v9_20260928/CONNAISSANCES_V10_20260928.md).

## Acquis (théorème ou oracle → code → porte)

| Acquis | Code | Porte |
| --- | --- | --- |
| Catalogue critique exact par boîtes de centres (lemmes G, D, C) | `src/catalogue/generator.cpp` | `mhgp10_catalogue_oracle` ; égalité à la v9 sur 08/000200 K5 et K10 |
| Tour FULL par morceaux locaux (Gordan) et descente | `src/tower/tower.cpp` | `mhgp10_tower_oracle` (7 600 coupes, E5) |
| Hiérarchie de points C∩X | `src/tower/tower.cpp` (attaches) | `mhgp10_tower_oracle` (partitions de points) |
| Condensation HDBSCAN exacte, EOM, feuilles | `src/head/head.cpp` | `mhgp10_head_condensation_vs_sklearn` |
| Référence exacte Python | `reference/hgp10_ref.py` | `reference/test_ref.py` |

## Carte

- `src/core`, `src/sched` : statuts, tampons comptés, ordonnanceur unique.
- `src/arith` : entiers larges, prédicats géométriques exacts (bornes en tête de `geometry.hpp`).
- `src/cloud` : sites (Morton, multiplicités), index `SiteTree` (k-NN et boules fermées à centre rationnel).
- `src/catalogue`, `src/tower`, `src/points`, `src/head` : objet, tour, hiérarchie de points, tête.
- `cli/` : `mhgp10_catalogue`, `mhgp10_tower`, `mhgp10_cluster`.
- `bench/synthetic` : banc contre `sklearn.cluster.HDBSCAN` (appelé tel quel), graines dev et test disjointes.

## Chantiers ouverts, par priorité

1. **Clustering** : campagne de développement en cours (8 familles × 4 niveaux × bruit 0/0,1 × n = 2k, 8k).
   À mi-parcours, la meilleure configuration unique de la tour égale le meilleur HDBSCAN réglé sur les mêmes graines
   (parité, conforme à la prévision CLUSTER_v2). Expérience de choix par scène sans étiquettes en cours. Ensuite :
   préenregistrement, puis campagne de test 8k/16k/32k (G4).
2. **Performance LiDAR** (priorité de complexité) : accélération de la feuille du générateur en cours ; mesures
   G4 à faire (protocole v10 en préparation, scripts gardés réutilisés).
3. **Multiplicités** dans la tour, Euler pondéré, juges d'échelle (K = 1 contre EMST, Euler à kmax + 2).
4. Verticales publiées (calculables par descente, non exposées).

## Pièges

- Ne jamais reconstruire un binaire utilisé par une campagne en cours : figer un build depuis `git archive`.
- `pkill -f` : toujours un motif à crochets (`'run_campa[i]gn'`), sinon le shell se tue lui-même.
- Aucun octet de nuage LiDAR dans le dépôt ; les entrées se régénèrent depuis leurs empreintes.
