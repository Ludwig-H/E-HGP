# Passation v10

État historique au 29 septembre 2026, complété le 30 septembre. Ce document décrit l'état ; l'historique est dans `git log`.

## Reprise développeur et précision au 30 septembre

Lire [le développement frontière et précision](docs/DEVELOPPEMENT_FRONTIERE_ET_PRECISION_20260930.md).
Le correctif de recherche de rang est raccordé au moteur ; un bras exact
par bande de couverture K3/K5 et des quotas de scènes sont ajoutés hors tête
de production. Six régressions natives bornées passent, pas une campagne
de clustering ni une qualification G4.

Nouvelle instruction utilisateur : dépasser u18. Le moteur ci-dessous est
encore u18 ; les chronos historiques ne qualifient pas la future précision.
Profil proposé : grille paramétrable à 0,1 mm, contenant u32, palier géométrique
u24 puis u32 complet. Le choix de grille ou float32 sans perte reste à confirmer.
Premières primitives u32 isolées livrées : distance u128 et Morton96,
212 684 contrôles. Le domaine du moteur complet et son refus au-delà de u18
sont inchangés.

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
   [audit critique de la v9](receipts/audit_v9_20260928/AUDIT_V9_CRITIQUE_20260928.md).

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

1. **Clustering (tête v10-b).** Le bras expérimental du 30 septembre et le port
   de précision sont décrits en tête ; les résultats suivants restent historiques.
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
     (+0,0046). Sur 416 composantes gaussiennes séparables, v10-b en retient 91 % (99 % sous un col à 30 % du pic) ;
     l'écart restant à la référence MAP supervisée porte sur l'affectation de la masse sous le col.
   - **Affectation sous le col** (dev sur G4, reçu `receipts/bench_dev_alloc_20260929`) : la montée de densité
     lissée et bornée (`asc20_b2`) est le meilleur remplissage de la tour à chaque K, de +0,003 à +0,009 (sous la
     marge de 0,02). Elle nuit à sklearn en feuilles : le levier est propre à la tête EOM. Sur les 192 scènes
     gaussiennes à K = 10 :
     - scores de référence (diagnostics supervisés, pas des plafonds d'ARI, audit continu du 29 septembre) : MAP à
       paramètres estimés sur les labels 0,895 ; montée discrète (k = 20) sur la densité ajustée avec les labels
       0,847 ; tour 0,816, puis 0,830 avec `asc20_b2` ;
     - l'écart entre les deux références n'est pas une impossibilité démontrée pour les méthodes de densité : il
       compare deux procédures ;
     - la tour reste à 0,017 de la seconde, surtout sur `anisotropic`, où sklearn en feuilles fait mieux (0,892
       contre 0,859).
   - **Sélection sur `anisotropic`** (dev local, reçu `receipts/bench_dev_shrink_20260929`) : rétrécir les amas EOM à
     leurs cœurs ne répare rien. La tour en feuilles égale sklearn sur `anisotropic` (0,887 contre 0,883), mais
     s'effondre sur `shells` (0,60 contre 0,94). Chaque famille veut sa sélection, et aucune règle fixe essayée
     (z, feuilles, prominence à la ToMATo, antichaîne mêlée) ne gagne 0,02 en moyenne.
   - **Ordres K plus grands** (dev sur G4, reçu `receipts/bench_dev_bigk_20260929`) : c'est le témoin MR₂-bord qui a
     été mesuré à K = 10 à 48, la tour exacte seulement à K = 10.
     - Avec la tête z = 6, le recul moyen vient presque entièrement de `shells`, qui s'effondre dès K = 16.
     - D'autres réglages progressent : MR₂ à z = 3 sans remplissage gagne +0,046 sur `anisotropic` à K = 48.
     - Cette grille ne justifie pas à elle seule un chantier immédiat du moteur au-delà de K = 10. Elle ne démontre
       pas que de grands K ne peuvent pas aider (addendum GPU et grands K de l'audit continu).
   - **Prochaine recherche** (dev) : un critère de sélection par scène (conception en cours), un a priori de taille
     autre que √n.
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
   - **Session G4 4** (reçu `receipts/g4_session4_j2c_20260929`, CPU seul, 24 cœurs et 48 fils, trois trames sans sol
     d'une seule séquence, 08) :
     - catalogue + tour **FULL** 1..K sans attaches : 0,204 à 0,254 s à K = 5, 0,86 à 1,12 s à K = 10, hors
       préparation ;
     - `mhgp10_cluster` (catalogue + **un seul ordre** K = 5 + tête, pas FULL) : 0,218 à 0,263 s, dont 0,02 s de
       tête ;
     - ni 100 ms ni plusieurs séquences ne sont qualifiés. Pour un budget FULL, la tour FULL seule prend 67 à 89 ms
       à K = 5.
   - Suite du plan ordonné du juge des conceptions GPU (hors dépôt, `v10-persist/gpu_design/juge/PLAN.md`) : la
     feuille en en-tête commun CPU/GPU (J3a), puis les feuilles sur GPU, puis l'arbre entier sur GPU. Cibles :
     100 ms à K = 5, 1 s à K = 10.
   - La VM n'a ni pip ni numpy : Python portable et binaires statiques envoyés comme données (`bench/g4/`).
3. **Échelle au-delà du LiDAR** (reçu `receipts/g4_session5_scale_20260929`, jusqu'à 1 024 000 sites, corrigé par
   l'audit continu, voir `receipts/ERRATA.md`) :
   - croissance empirique sous-quadratique. Ce n'est pas un coût constant par boule : sur `clusters` K = 10, la tour
     passe de 86 à 174 ns par boule de 8 000 à 1 024 000 sites ;
   - le nombre de boules par site (≈ 460 en 3D, 65 à 120 sur une surface, à K = 10) est un ordre de grandeur
     empirique, pas une borne ;
   - **la mémoire est le mur** : 303,6 à 314,9 octets par boule au pic ; un million de sites 3D à K = 10 prend
     135,28 Gio, soit 145,26 Go ;
   - aucune capacité LiDAR n'est qualifiée (le seuil « 1,4 M sites » était faux). Les dizaines de millions de
     points demanderont un catalogue qui ne réside pas tout entier ;
   - le rapport tour/catalogue varie de 1,1 (`terrain`) à 2 (`clusters`, `uniform` denses).

   GPU de la VM : RTX PRO 6000 Blackwell (97 Go, sm_120), CUDA 12.9 sous `/usr/local/cuda-12.9`. La sonde
   (`receipts/g4_session7_cuda_probe_20260929`) qualifie seulement un comparateur de produits i64 × i64 en `__int128`,
   exact sur 16,8 M paires. Ni les prédicats complets ni l'arithmétique I192 du moteur ne sont qualifiés. Le rapport
   de débit ×2,3 entre 128 et 64 bits n'est pas fiable : les boucles de débit débordaient en entiers signés.
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
