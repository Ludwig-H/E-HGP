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
   - **Audit du 29 septembre** (`audits/audit_hierarchie_knn_20260929`) : 3 lectures, 4 audits, 4
     contre-vérifications, 52 constats, dont aucun réfuté.
     - La tour est exactement l'arbre plug-in de l'estimateur K-NN, et elle identifie les modes des mélanges
       séparables : aux niveaux faciles, sa meilleure coupe atteint le plafond de Bayes.
     - À même entrée et même tête, elle égale la hiérarchie d'HDBSCAN (MR-bord, à 0,01 près sur dev). L'avantage
       sur HDBSCAN vient de l'entrée des amas discrets et de la tête (z). À entrée cœur, MR₁ bat la tour : c'est la
       loi de la demi-lacune.
     - L'axe K (verticales) reste inexploité : c'est la voie d'un avantage structurel.
   - **Tête v10-b** : entrée `cover`, EOM avec λ = r^(−z), remplissage borné.
     - ẑ est abandonné : il fait s'effondrer `shells` à K = 10.
     - L'optimum de z dépend de n (≈ 3–4 à 2 000 points, ≈ 6 à 8 000).
     - Sans remplissage, la tour domine sklearn de +0,04 à +0,115 sur dev ; avec, de +0,013 à +0,070 (scènes de
       8 000 points).
   - **Lot A** (tête C∩X du 28 septembre, test préenregistré) : la tour bat HDBSCAN à K = 1, 2 et 3, avec
     Δ = +0,053, +0,092 et +0,049, p_Holm = 3e-5 (reçu `receipts/test_kmatch_A_20260929`). À K = 1, c'est un effet
     de tête.
   - **Lot C** (préenregistrement `bc413ff56`, amendé `65ea6b222`, exécuté sur G4 en deux sessions, reçu
     `receipts/test_cover_C_20260929`) : **la tête v10-b bat HDBSCAN à tous les K** :
     - Δ = +0,075, +0,073, +0,091, +0,064, +0,040 et +0,031 à K = 1, 2, 3, 5, 8 et 10 ; sans remplissage, de
       +0,034 à +0,105 ;
     - lot B (tête C∩X) : parité à K = 5, défaite à K = 8 et 10 ;
     - famille « objet » : parité de la tour et de MR₂-bord à même entrée et même tête à K = 2, 3, 5 et 8, et
       +0,010 pour la tour à K = 10 (sous la marge). L'avantage vient de l'entrée et de la tête.
   - **Tout calcul long passe sur G4** (directive du 29 septembre). La VM n'a ni pip ni numpy : un Python portable
     et des binaires statiques y sont envoyés comme données (`bench/g4/lot_runner.py`, `bench/g4/pyenv_run.py`,
     fusion `bench/g4/merge_sessions.py`).
   - **Têtes multi-K** (`audits/tete_multik_20260929`, dev seulement) : tranche oblique, persistance à travers K,
     antichaîne à ordres mêlés. Aucune ne bat v10-b de 0,02 ; seule l'antichaîne se reproduit hors échantillon
     (+0,0046). Sur 416 composantes gaussiennes séparables, v10-b en retient 91 % (99 % sous un col à 30 % du pic) :
     l'écart restant au plafond de Bayes est l'affectation de la masse sous le col.
   - **Affectation sous le col** (dev sur G4, reçu `receipts/bench_dev_alloc_20260929`) : la montée de densité
     lissée et bornée (`asc20_b2`) est le meilleur remplissage de la tour à chaque K, de +0,003 à +0,009 (sous la
     marge de 0,02). Elle nuit à sklearn en feuilles : le levier est propre à la tête EOM. Sur les 192 scènes
     gaussiennes à K = 10 :
     - Bayes 0,895, bassins de la vraie densité (Morse) 0,847, tour 0,816 puis 0,830 avec `asc20_b2` ;
     - le prix du modèle (0,048) est hors de portée de toute méthode de densité ;
     - il reste 0,017 à la tour, surtout sur `anisotropic`, où sklearn en feuilles fait mieux (0,892 contre 0,859).
   - **Sélection sur `anisotropic`** (dev local, reçu `receipts/bench_dev_shrink_20260929`) : rétrécir les amas EOM à
     leurs cœurs ne répare rien. La tour en feuilles égale sklearn sur `anisotropic` (0,887 contre 0,883), mais
     s'effondre sur `shells` (0,60 contre 0,94). Chaque famille veut sa sélection, et aucune règle fixe essayée
     (z, feuilles, prominence à la ToMATo, antichaîne mêlée) ne gagne 0,02 en moyenne.
   - **Prochaine recherche** (dev) : un critère de sélection par scène ; un a priori de taille autre que √n ; des
     ordres K plus grands, hors du moteur exact actuel (`kMaxOrder = 10`), à évaluer d'abord sur le témoin MR₂-bord.
   - **Corrections de l'audit livrées** : raison de refus propre aux multiplicités (IMP-16, porte
     `mhgp10_regression_multiplicity_refusal`) ; α ≠ 1 refusé hors de `kd_tree`/`ball_tree` dans
     `methods.hdbscan_labels` (IMP-04, α est ignoré en brute dans sklearn 1.9.1) ; refus dev écrits à ARI_s = 0
     (D8) ; EVAL_v2 (k du remplissage), CLUSTER_v2 § 3.2, A9-67 inversé, errata des reçus (`receipts/ERRATA.md`).
     Reste, pour l'auteur : la phrase de la thèse « la seule différence est le type de connexité », qui doit aussi
     mentionner l'appartenance.
2. **Performance LiDAR** (priorité de complexité). Mesures G4 en CPU seul, 48 fils (reçus
   `receipts/g4_session1_20260929` puis `receipts/g4_session2_perf_20260929`) :
   - après l'assemblage parallèle et la frontière pilotée par la charge (J1), catalogue et tour à K = 5 en **0,31 à
     0,37 s** par trame (0,58 à 1,19 s avant) et en 1,2 à 1,5 s à K = 10 (2,5 à 4,6 s) ;
   - chaîne complète jusqu'aux étiquettes à K = 5 : 0,46 à 0,56 s ;
   - le catalogue passe maintenant de 8,5 s à 1 fil à 0,31 s à 48 fils (×27, contre ×7,7).
   - **J2 livré** (`5565f94fb`, reçu `receipts/catalogue_filter_j2_20260929`) : filtre des nœuds en forme D-loc
     exacte, garde incluse dans la dominance, pré-ignorance par l'enveloppe parente. Le catalogue est identique octet
     pour octet (10 entrées, 1 et 4 fils), les 9 portes sont vertes. Sur G4 (reçu
     `receipts/g4_session3_j2_20260929`), le catalogue de la trame 02 à K = 5 passe de 8,51 à 4,82 s à 1 fil, et
     de 0,314 à 0,235 s à 48 fils. Trames entières à 48 fils : 0,23 à 0,29 s à K = 5, 1,02 à 1,29 s à K = 10. La
     chaîne complète jusqu'aux étiquettes prend 0,38 à 0,48 s, dont 0,15 à 0,18 s de tête. `MHGP10_MARCH` n'est
     permis sur G4 qu'après la porte ISA v4.
   - **Assemblage** (`7eee86c53`) : tableaux du catalogue non initialisés, remplis par les boucles parallèles ;
     identique octet pour octet en build normal et empoisonné ; ordre + assemblage −29 à −32 % à 4 fils.
   - **Course du pool corrigée** (`8e3b76245`, reçu `receipts/pool_race_fix_20260929`) : un ouvrier en retard
     pouvait exécuter une tranche du travail suivant deux fois ou avec la mauvaise fonction. Les tests préenregistrés
     tournent à 1 fil et n'ont pas pu être touchés. Nouvelle porte de stress du pool dans `mhgp10_unit` ;
     ThreadSanitizer propre (`-DMHGP10_TSAN=ON`, lancé par `setarch -R`).
   - **Tête** (reçu `receipts/head_point_dendrogram_20260929`) : `point_dendrogram` trie par dénombrement des rangs du
     catalogue au lieu d'un tri général à approximations recalculées : ×17 à K = 5 (0,272 → 0,016 s en local).
   - **J2c** (reçu `receipts/catalogue_fitted_split_j2c_20260929`) : boîtes ajustées à l'enveloppe de leur liste,
     coupées au milieu du plus long côté (arbre binaire). Catalogue identique ; étage des boîtes ×1,12 à ×1,30 en
     local ; `t_frontier` +60 à 100 %, à surveiller sur G4. Les conceptions GPU « arbre » supposaient un octree : à
     revoir.
   - **Session G4 4** (reçu `receipts/g4_session4_j2c_20260929`, CPU seul, 48 fils) : chaîne complète jusqu'aux
     étiquettes à K = 5 en 0,22 à 0,26 s (0,46 à 0,56 s le matin), dont 0,02 s de tête. Catalogue et tour : 0,20 à
     0,25 s à K = 5, 0,86 à 1,13 s à K = 10. Restent pour 100 ms : boîtes (0,10 s), tour (0,05 s), frontière
     (0,027 s), ordre et assemblage.
   - Suite du plan ordonné du juge des conceptions GPU (hors dépôt, `v10-persist/gpu_design/juge/PLAN.md`) : la
     feuille en en-tête commun CPU/GPU (J3a), puis les feuilles sur GPU, puis l'arbre entier sur GPU. Cibles :
     100 ms à K = 5, 1 s à K = 10.
   - La VM n'a ni pip ni numpy : Python portable et binaires statiques envoyés comme données (`bench/g4/`).
3. **Échelle au-delà du LiDAR** (reçu `receipts/g4_session5_scale_20260929`, jusqu'à 1 024 000 sites) : temps
   linéaire en nombre de boules. Les boules par site sont bornées par la géométrie : ≈ 460 en 3D, 65 à 120 sur une
   surface, à K = 10. **La mémoire est le mur** : ≈ 280 octets par boule au pic, soit 135 Go pour un million de sites
   3D à K = 10. La tour coûte deux fois le catalogue à grande échelle. Au-delà d'environ 1,4 M sites LiDAR, il
   faudra un catalogue qui ne réside pas tout entier. GPU de la VM : RTX PRO 6000 Blackwell (97 Go, sm_120),
   CUDA 12.9 sous `/usr/local/cuda-12.9`.
4. **Multiplicités** dans la tour, Euler pondéré, juges d'échelle (K = 1 contre EMST, Euler à kmax + 2).
5. Verticales publiées : calculées par la tour, non exposées à la tête.

## Pièges

- Ne jamais reconstruire un binaire utilisé par une campagne en cours. Figer un build depuis `git archive`, ou
  copier le binaire à part avant l'expérience.
- Un redémarrage de la machine vide `/tmp` et le scratchpad, et tue les agents d'arrière-plan : garder sous
  `/workspaces` tout ce qui doit survivre.
- `pkill -f` : toujours un motif à crochets (`'run_campa[i]gn'`), sinon le shell se tue lui-même.
- Tout changement du code parallèle passe aussi sous ThreadSanitizer (`-DMHGP10_TSAN=ON`, dans `/tmp`). Sur ce
  noyau, TSan exige `setarch $(uname -m) -R`, sinon « unexpected memory mapping ». Un échec d'oracle non
  reproductible en isolé se stresse sous charge avant d'être attribué au dernier changement.
- Aucun octet de nuage LiDAR n'est ajouté au dépôt ; les secteurs du v8 déjà suivis sont réutilisés en place.
