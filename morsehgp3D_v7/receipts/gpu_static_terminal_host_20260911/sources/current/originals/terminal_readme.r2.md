# Terminal géométrique privé, premier raccord hôte

Cadre : exploration v7 hors registre, profil quantifié u16, backend `HOST_STUB`, `public_status=not_claimed`. Ce chantier reprend c03f6be8 et deux briques privées explicitement épinglées, pas les nouvelles modifications post-seed. Aucun code actif, audit, Git ou GCP n'est modifié. `o2_r1` puis la révision ordinal u64 `o2_r2` sont fermées PASS ; SAN ROOT et T2 sont encore à qualifier séparément.

## Une seule source composée

`prepare.py` a créé le snapshot `source/` à partir de la clôture MEB/clé `2e26fa2d592f3455a8fddfd7aa3e9a98370d24bf537c22fe07d20eb9500414a9`, puis ajouté le juge FULL et les oracles du commit `c03f6be8488453486b112811071827a96303ec86`. L'intrus est explicitement importé au SHA `165fd5964c9ea2394ca29747b236713d74528db4fd4149429f9080dd423cc75a`, avec son propriétaire durci, son décodeur et sa division. Les chemins de ces includes consomment **la même** base arithmétique `source/morsehgp3D_v7/src`, pas deux namespaces concurrents issus de snapshots différents. Les originales et pins demeurent lisibles ; aucun résultat précédent n'est automatiquement transféré au raccord.

## Helper sans état temporel

`terminal.cuh` calcule un terminal pour K=2..10, un thread/facette à terme : sélection MEB, clé primitive, niveau brut, lookup avant intrus, remplacement du premier slot de support par le premier intrus Morton strict, puis nouvelle MEB. Il exige niveau strictement inférieur au consommateur à chaque itération ; la fenêtre terminale est `n_interior + arity - 1 <= K <= n_interior + n_shell`. Un hit hors fenêtre poursuit la recherche d'intrus. À rayon égal, la clé doit rester identique et la coquille sélectionnée perdre exactement un point.

Les niveaux sont bruts q2 distance carrée/4, q3_level_raw et q4_level_raw, comparés via U320. Aucun PGCD de niveau ni formule de rayon depuis la clé primitive n'est ajouté. Leur représentation peut différer du CPU tout en étant sémantiquement égale. Le niveau franchit l'ABI comme cinq mots u64 ; la clé comme dix mots u64, sans i128 natif. Le comparateur de clé respecte l'ordre **signé** des coefficients, pas l'ordre brut des octets.

Les positions u16 résidentes sont chargées en K P3 locaux dans le même ordre que les indices triés. Les slots de support sont donc conservés. L'indice géométrique, pas le PointId, est utilisé pour le remplacement et l'exclusion. Les bornes de pile 49 et d'évaluation u16 proviennent du helper intrus qualifié ; aucun quota de chaîne n'est ajouté. Tous les compteurs longs du helper sont contrôlés contre le débordement.

La révision après la première O2 transporte l'ordinal d'occurrence en **u64** dans Request et Result. L'ancien ordinal u32 du helper MEB est fixé à zéro, local à son unique appel interne ; il ne porte pas l'identité globale. Deux fixtures transportent 2^32+7 et UINT64_MAX sans modifier sélection, clé ou travail. `o2_r1` conserve la première API u32 ; seule une capture nouvelle peut qualifier la révision.

`o2_r2` qualifie cette révision sur static1 et static4 : 20 851 contrôles unitaires, 523 MEB K≥2, 605 refus K1/coupe stricte, 483 niveaux bruts de représentation différente et deux grands ordinaux. Le juge FULL effectue 256 672 contrôles sur 30 nuages, 124 ordres et 4 498 comparaisons physiques ; 88 terminaux et 106 traces sont appariés au CPU, avec 14 descentes strictes et quatre à rayon égal. Les deux modes rendent les mêmes résultats.

La trace facultative est un diagnostic : un buffer trop court signale une trace incomplète, **sans arrêter ni tronquer le terminal**. La gate répète chaque résolution appariée avec capacité de trace zéro et confronte encore le terminal et les compteurs. Cette limite diagnostique ne devient ni une limite de points ni un garde-fou algorithmique.

## Propriétaire et limites

`terminal_owner.hpp` possède ensemble index compact et catalogue, avec une nouvelle génération commune. Les vues expirent avec lui ; mélanger deux propriétaires de même géométrie n'est pas admis. Le propriétaire admet les métadonnées, les bornes et les clés distinctes ; il ne certifie pas la géométrie ou la complétude d'un catalogue externe. Le raccord FULL est appelé après la validation du catalogue par le producteur c03. L'index ne contient pas de doublons géométriques, comme le FULL actif.

Il n'y a pas encore de propriétaire de buffers CUDA ni de wrapper transactionnel GPU. La publication FULL hôte reste celle de c03 : toute erreur vide le résultat entier ; aucun préfixe d'ordre n'est exporté. Le helper ne connaît ni ancres, ni DSU, ni cartes verticales. Il retourne uniquement le BallId du catalogue ; le code chronologique hôte résout ensuite cette ancre fermée vers sa composante. K=1 reste la résolution directe vers une feuille, sans faux singleton de catalogue.

## Porte de preuve, pas benchmark

`terminal_bridge.hpp` est exclusivement un juge de raccord : il exécute la méthode statique CPU c03 conservée, puis le nouveau helper, compare BallId, tous les compteurs géométriques et chaque trace (sélection/clé/niveau/intrus), et répète une fois le helper sans stockage de trace. Seul le travail d'un helper est reporté dans les compteurs historiques du résultat FULL ; les deux autres calculs appartiennent au juge. Ces exécutions ne doivent donc jamais servir à annoncer du travail réellement payé ou une accélération du produit.

La modification privée de `full_ball_tower.hpp` est conditionnée par `MHGP7_PRIVATE_TERMINAL_STUB`. Elle crée le propriétaire avant le parallélisme, garde K=1/les seeds/le DSU/la verticalité inchangés, puis substitue seulement le terminal statique. Le scratch de la référence reste le scratch historique pour préserver ses mesures structurelles ; les copies, traces et allocations du juge sont du surcoût **hors** de ces compteurs, sans prétention RSS.

`gate.cpp` commence par les 605 fixtures Gram de MEB : les 523 cas K≥2 éprouvent clé/niveau brut/support/compteurs dans l'ordre Morton ; les 82 singletons vérifient que K=1 reste hors API terminale. Le catalogue unitaire sert uniquement à une requête de hit immédiat, pas à une reconstruction FULL complète. La gate historique indépendante Gram/Gamma reconstruit ensuite 30 nuages bornés et compare physiquement le raccord au resolver CPU séquentiel. Des captures nouvelles doivent qualifier ces sorties ; les succès antérieurs de ce juge ne sont pas hérités.

```bash
python3 -B build/v7_gpu_static_terminal_20260911/record.py --out o2_r1 --mode stub
python3 -B build/v7_gpu_static_terminal_20260911/record.py --out san_root_r1 --mode san
```

Chaque capture est create-only et compile son `source_snapshot`, avec C++20, O2 ou ASan/UBSan/LSan et les quatre diagnostics stricts. Le Boost déjà disponible localement est consommé via `-isystem`, sans installation réseau. Les exécutions static1/static4 sont séquentielles dans le recorder. Aucun résultat CUDA exécuté, aucune mesure GPU/FULL 50k ou multi-millions n'est revendiqué.
