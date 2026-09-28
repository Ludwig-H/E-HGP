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
| Catalogue critique exact par boîtes de centres (lemmes G, D, C), feuille v2 à masques de dominance (lemmes M, S) | `src/catalogue/generator.cpp` | `mhgp10_catalogue_oracle` ; égalité à la v9 sur 08/000200 K5 et K10 ; dumps de la feuille v2 identiques à `93710d076` |
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

1. **Clustering** : campagne de développement terminée (reçu `receipts/bench_dev_20260928/`) : parité entre la
   meilleure configuration unique de la tour et le meilleur HDBSCAN réglé sur les mêmes graines, large victoire sur
   HDBSCAN par défaut. Sélection par scène par DBCV : gain symétrique pour les deux méthodes ; grille complète
   (K ≤ 10, `min_samples` ≤ 20, α ∈ {1, 2}) en cours en local à n = 2 000. Ensuite : préenregistrement, puis
   campagne de test 8k/16k/32k (G4).
2. **Performance LiDAR** (priorité de complexité) : la feuille v2 a divisé le catalogue par ≈ 1,6 ; la tour est
   désormais le goulot (mesure locale à 4 fils : K5 catalogue 3–4 s contre tour 4–6 s ; K10 11–15 s contre
   32–56 s, RSS 2,4 Go), loin de l'estimation de conception (≈ 2 CPU·s à K5). Instrumentation par étage et
   accélération de la tour en cours. Protocole G4 v10 (`gcp-migration/v10_*`) en correction après revue adverse,
   non commité : aucune session G4 v10 à ce jour.
3. **Multiplicités** dans la tour, Euler pondéré, juges d'échelle (K = 1 contre EMST, Euler à kmax + 2).
4. Verticales publiées (calculables par descente, non exposées).

## Pièges

- Ne jamais reconstruire un binaire utilisé par une campagne en cours : figer un build depuis `git archive`.
- `pkill -f` : toujours un motif à crochets (`'run_campa[i]gn'`), sinon le shell se tue lui-même.
- Aucun octet de nuage LiDAR dans le dépôt ; les entrées se régénèrent depuis leurs empreintes.
