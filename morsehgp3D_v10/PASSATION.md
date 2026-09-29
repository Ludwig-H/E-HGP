# Passation v10

État au 29 septembre 2026. Ce document décrit l'état ; l'historique est dans `git log`.

```text
phase=exploration_v10_hors_registre
backend=cpu_reference
profile=quantized_u18_input_only
public_status=not_claimed
GCP : session 1 le 29 septembre (CPU seul, arrêt certifié TERMINATED), reçu `receipts/g4_session1_20260929`
```

## Lire d'abord

1. [README](README.md) : état court et commandes.
2. [Spécification](docs/SPEC_V10.md) : l'objet tel qu'implémenté, les portes, ce qui n'est pas établi.
3. [Clustering depuis la tour](docs/CLUSTERING_DEPUIS_LA_TOUR_20260929.md) : lecture critique des parties I et II de
   la thèse, diagnostic de la tête du 28 septembre, tête v10-b.
4. [Conception intégrée](docs/conception/CONCEPTION_V10.md), et
   [audit critique de la v9](audits/audit_v9_20260928/AUDIT_V9_CRITIQUE_20260928.md).

## Acquis (théorème ou oracle → code → porte)

| Acquis | Code | Porte |
| --- | --- | --- |
| Catalogue critique exact par boîtes de centres (lemmes G, D, C), feuille v2 à masques de dominance (lemmes M, S) | `src/catalogue/generator.cpp` | `mhgp10_catalogue_oracle` ; égalité à la v9 sur 08/000200 K5 et K10 |
| Tour FULL par morceaux locaux (Gordan) et descente, construite tous ordres ensemble par étages, verticales à pointeurs de saut | `src/tower/tower.cpp` | `mhgp10_tower_oracle` ; dumps identiques au binaire figé `4a3d09d8a` sur 8 entrées ; 1 fil = 4 fils |
| Hiérarchie de points, entrée `core` (C∩X) ou `cover` (première couverture, amas discrets du théorème 2) ; en `cover`, seules les premières boules couvrantes sont résolues (au plus n par ordre) | `src/tower/tower.cpp` (attaches) | `mhgp10_tower_oracle`, `mhgp10_points_cover`, `mhgp10_regression_level_collision`, `mhgp10_regression_batch_equivalence` ; reçu `receipts/cover_attach_first_balls_20260929` |
| Appel groupé du banc : un catalogue par scène pour tous les K, les deux entrées et toutes les têtes | `cli/mhgp10_cluster.cpp` (`--k-list`, `--entry=core,cover`, `--configs`), `bench/synthetic/run_test.py` | `mhgp10_regression_batch_equivalence` (groupé = séparé, vote = arbre, 1 fil = 4 fils) |
| Condensation HDBSCAN exacte, EOM, feuilles ; vote de couverture | `src/head/head.cpp`, `cli/mhgp10_cluster.cpp` | `mhgp10_head_condensation_vs_sklearn` |
| Référence exacte Python | `reference/hgp10_ref.py` | `reference/test_ref.py` |

## Carte

- `src/core`, `src/sched` : statuts, tampons comptés, ordonnanceur unique.
- `src/arith` : entiers larges, prédicats géométriques exacts (bornes en tête de `geometry.hpp`).
- `src/cloud` : sites (Morton, multiplicités), arbre k-d des sites (k-NN et boules fermées à centre rationnel).
- `src/catalogue`, `src/tower`, `src/points`, `src/head` : objet, tour, hiérarchie de points, tête.
- `cli/` : `mhgp10_catalogue`, `mhgp10_tower`, `mhgp10_cluster` (`--entry=core|cover`, `--label=vote`).
- `bench/synthetic` : banc contre `sklearn.cluster.HDBSCAN` appelé tel quel, graines dev et test disjointes ;
  `prereg/` : préenregistrements gelés ; `run_test.py` et `decide.py` exécutent et décident.
- `bench/scaling` : mise à l'échelle, en régime spatial et en régime de densité (synthétique), et par secteurs coupés
  au capteur (LiDAR).

## Chantiers ouverts, par priorité

1. **Clustering (tête v10-b).**
   - Sur dev, l'entrée par première couverture corrige le défaut de rappel de la tête du 28 septembre (sémantique
     des cœurs). À K apparié avec `min_samples`, la tour bat sklearn à K = 1, 2, 3 et 5 (+0,061, +0,050, +0,035,
     +0,013) et fait jeu égal à K = 8. Son meilleur réglage est K = 3, ẑ-EOM, b(1,5) : 0,7831 (reçu
     `receipts/bench_dev_cover_20260929`).
   - Il reste deux faiblesses : `filaments` et `anisotropic` aux niveaux difficiles, où EOM fusionne des groupes
     proches, et la décroissance de l'avantage quand K grandit. Piste : juger une scission par sa persistance à
     travers K, grâce aux verticales.
   - Campagne de test du lot A (tête C∩X, préenregistrée `e8dd36a91`) : relancée en entier après un redémarrage de
     la machine. Le run interrompu (160/960 scènes) est conservé et doit concorder avec la relance.
   - La tête v10-b exigera un nouveau préenregistrement, sur un nouvel espace de graines. Le banc est prêt :
     l'appel groupé partage le catalogue entre tous les K, et `run_test.py --resume` reprend une exécution
     interrompue sans recalculer une scène.
2. **Performance LiDAR** (priorité de complexité).
   - La tour est 8 à 14 fois plus rapide. Mesures à 4 fils : K5 de 0,40 à 0,54 s, K10 de 2,8 à 3,9 s, RSS K10 de
     1,8 Go.
   - Le catalogue domine désormais la chaîne : K5 de 2,7 à 3,8 s, K10 de 10 à 14 s à 4 fils.
   - En entrée `cover`, l'attache ne résout plus que les premières boules couvrantes : la tour est 2 à 3,5 fois
     plus rapide dans ce mode, avec des dumps identiques.
   - Session G4 1, CPU seul (le GPU n'a pas servi, la v10 n'a pas de voie CUDA), 48 fils :
     - K = 5 : de 0,58 à 1,19 s par trame (catalogue et tour) ; chaîne complète jusqu'aux étiquettes en entrée
       `cover`, de 0,75 à 1,39 s ;
     - K = 10 : de 2,5 à 4,6 s ;
     - compteurs linéaires en croissance spatiale (exposant 1,00 à 1,05), de 1,03 à 1,31 en croissance de
       densité, proches de 1 sur les secteurs LiDAR coupés au capteur.
   - Le catalogue fait 85 à 93 % du temps et ne passe pas l'échelle : ×7,7 de 1 à 48 fils. Son assemblage final
     était séquentiel ; il est désormais parallèle, avec un catalogue identique octet pour octet (reçu
     `receipts/catalogue_parallel_assembly_20260929`).
   - Prochain levier : l'énumération des boîtes. Le filtrage des nœuds internes compte 592 M tests de gardes et de
     dominance, contre 133 M dans les feuilles. Deux voies : réduire ce travail sur CPU, ou porter l'arbre de boîtes
     sur GPU.
   - La VM n'a ni `pip` ni `numpy` : les portes Python ne tournent qu'en local, et une campagne sklearn ne peut pas
     y tourner en l'état.
3. **Multiplicités** dans la tour, Euler pondéré, juges d'échelle (K = 1 contre EMST, Euler à kmax + 2).
4. Verticales publiées : calculées par la tour, non exposées à la tête.

## Pièges

- Ne jamais reconstruire un binaire utilisé par une campagne en cours. Figer un build depuis `git archive`, ou
  copier le binaire à part avant l'expérience.
- Un redémarrage de la machine vide `/tmp` et le scratchpad, et tue les agents d'arrière-plan : garder sous
  `/workspaces` tout ce qui doit survivre.
- `pkill -f` : toujours un motif à crochets (`'run_campa[i]gn'`), sinon le shell se tue lui-même.
- Aucun octet de nuage LiDAR n'est ajouté au dépôt ; les secteurs du v8 déjà suivis sont réutilisés en place.
