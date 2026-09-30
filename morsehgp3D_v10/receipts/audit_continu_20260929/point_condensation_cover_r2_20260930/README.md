# R2 — condensation : témoin de couverture géométrique historique

La tête publiée calcule ici une stabilité incorrecte après le départ du point 0 : avec min_cluster_size=6, la masse restante devient 5 et doit sortir immédiatement à la même date. Elle continue pourtant jusqu'à la naissance géométrique de la racine. Le défaut est reproduit sur une traduction contrôlée d'un export natif historique à six points, pas seulement sur un objet API abstrait.

## Résultat borné

Pour K=3, le point 0 est couvert à β=25 et attaché à la racine née à β=169/9. Les cinq autres points sont couverts avant cette naissance. En parcourant l'arbre en sens décroissant de β, le premier départ fait donc passer la masse 6→5.

| expZ | Stabilité native de la racine | Condensation événementielle correcte |
| --- | --- | --- |
| 1 | 1/5 + 15/13 = 88/65 | 6/5 |
| 2 | 1/25 + 45/169 = 1294/4225 | 6/25 |

Les cinq dates de sortie incorrectes sont aussi observées. Ces différences valent pour allow_single_cluster=false et true. Sur ce petit nuage, la sélection EOM reste identique : aucune inversion EOM n'est revendiquée ici. Le paquet API R1 distinct montre séparément des inversions, y compris sous une racine exclue ; il n'est ni modifié ni fusionné avec cette preuve géométrique.

Par mode natif : 12 configurations (mcs=1/2/6 × expZ=1/2 × racine autorisée/non autorisée), huit contrôles positifs mcs=1/2, quatre désaccords mcs=6, zéro inversion EOM. mcs=1 est un contrôle de l'API C++ seulement : ce n'est pas un paramètre accepté par sklearn.HDBSCAN.

## Géométrie et traduction vérifiées

historical/internal_k3.u32le contient, dans l'ordre des IDs originaux :
(15,4,0), (5,4,0), (7,8,0), (7,0,0), (1,4,0), (0,4,1).
Le nuage n'est pas contenu dans un plan. L'export conserve l'ordre des sites [3,4,5,1,0,2], six nœuds et toutes les couvertures brutes ; aucune bande heuristique ni nouveau point n'est ajouté.

materialize.py utilise une référence MEB Fraction indépendante pour les 20 sommets Γ3 et les 15 unions à quatre points. Les composantes Γ3 et celles de la forêt exportée coïncident aux 28 coupes événementielles/intermédiaires contrôlées. Les six premières couvertures sont recoupées, et chaque propriétaire choisi est vivant et contient un sommet Γ3 réalisant cette première couverture. Le fichier materialized.hpp est régénéré à l'identique aux lectures normale et −O.

La traduction vers PointDendrogram conserve les rangs, parents, CSR, IDs/attaches/dates et les six poids unitaires. Les niveaux sont représentés par les double finis de cette API existante ; ceci ne qualifie pas un nouveau moteur d'ordre rationnel.

check.py est un oracle d'événements distinct du corps natif : ensemble de points encore présents, départs par cohortes atomiques, collapse immédiat si la masse résiduelle devient inférieure à mcs, puis divisions géométriques et EOM. Pour expZ=2, les calculs sont Fraction. Pour expZ=1, les sommes de racines sont encadrées par des rationnels calculés avec isqrt ; les décisions EOM utilisent des intervalles à 160/320/640 bits ou refusent l'ambiguïté. Aucun assert ni décision oracle en flottants.

## Deux autorités distinctes

Les sept fichiers source/head, source/points et source/core sont les blobs Git exacts du commit 8bb4618e5c6c7c2bc8a5d7a1c2e189a4249d0047, vérifiés avant compilation. Le gel des 17 sources/preuves est daté 2026-09-30T10:32:08.460790+00:00 ; la capture se clôt à 2026-09-30T10:32:16.240899+00:00 avec les mêmes empreintes avant/après.

L'export géométrique est ANCIEN : origine morsehgp3D_v10/receipts/development_frontier_precision_20260930/qualification/native_normal/internal_k3.json, engine_commit déclaré d679ae29d-plus-rank-fix-final. Son SHA-256 est a2af5e645bba1eb775a5ed6d659ccc14d1ddd17246ce4f0973f7170a10a47499. historical/receipt.json conserve son ancienne commande, rc0, stdout et les pins du nuage/export/références. Le pin du binaire historique n'est qu'un SHA post-build ; sa fermeture complète de sources reste externe. Cette copie ne rejoue pas le générateur ni ne relabellise l'ancien export sous 8bb.

Les nouvelles exécutions concernent UNIQUEMENT la tête C++ : deux compilations strictes, normale et UBSan, puis deux appels natifs rc0, sans diagnostics, avec stdout identiques. Les binaires sont externes au paquet et leurs empreintes avant/après sont conservées ; aucun binaire n'est versionné. Les en-têtes système et bibliothèques ne sont pas embarqués.

## Lecture du reçu

receipt.json conserve exactement 11 commandes : identification du compilateur, deux matérialisations normale/−O, deux compilations, deux appels natifs et quatre jugements normale/−O. Tous les rc sont 0 ; toutes les sorties d'erreur sont vides. Les temps de ces micro-commandes ne sont pas des chronos de performance HGP.

verify.py contrôle le manifeste complet de 44 fichiers avant tout import de code archivé, les 17 pins exacts, les commandes/arguments/flags/flux/codes, les deux autorités et les compteurs non vacants. Il recalcule la matérialisation et rejuge les deux stdout natifs sans appeler de compilateur, de moteur ou de générateur. Il est portable après copie du paquet : les chemins absolus conservés sont des arguments HISTORIQUES, pas des fichiers LIVE exigés.

Exécution en lecture seule : python3 -B verify.py ; puis python3 -B -O verify.py.
SHA256SUMS ferme les 44 fichiers hors manifeste lui-même ; ses 45 fichiers totaux n'incluent aucun binaire.

Aucun moteur partagé modifié, aucune nouvelle exécution du générateur, aucun GCP/GPU, aucun benchmark ARI ni nouveau contrat FULL/sous-quadratique/100 ms acquis. Ce témoin établit le défaut de condensation sur ce cas géométrique historique recoupé, pas une garantie statistique générale ni une dégradation chiffrée de benchmarks passés.

