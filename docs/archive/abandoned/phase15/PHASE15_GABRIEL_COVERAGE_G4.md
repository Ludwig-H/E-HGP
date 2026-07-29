# Phase 15 — gate massif de couverture Gabriel sur Delaunay ordinaire (archive)

> [!NOTE]
> Rapport historique scellé. Cette voie PDEL/Gabriel reste un oracle hors ligne; elle n'est ni une dépendance, ni un repli, ni une correction du pipeline produit centré sur le catalogue exact des paires diamétrales.

## Verdict

Le diagnostic `ordinary_delaunay_wedge_plus_cuda_g4_aabb_grid / hgp_reduced / gabriel_necessary_batch` implémente le critère demandé : pour chaque triangle classé `gabriel_binary64` au niveau carré $a$, soit le triangle reste explicite dans le lot nécessaire, soit ses trois arêtes appartiennent déjà à une même composante construite à des niveaux strictement inférieurs à $a$. Les unions d'un même plateau sont appliquées seulement après toutes les décisions du plateau; elles ne peuvent donc pas justifier une omission. Les triangles ambigus sont tous conservés explicitement et ne servent jamais à connecter un autre triangle.

Les campagnes G4 ferment ce gate binary64 à 50 000, 10 000 001 et 30 000 001 points. Les deux découpages indépendants à 10 000 001 points produisent les mêmes comptes et les mêmes SHA-256 pour l'univers reconstructible, les sources Gabriel binary64 et le lot explicite logique. Le validateur Python borné rejoue intégralement la DSU, recalcule l'univers, le catalogue Gabriel exact et les engagements sur les fixtures émises. Pour les rapports massifs sans records, il ferme seulement le schéma, les compteurs et les invariants annoncés; les SHA-256 restent des engagements du producteur dont seule l'égalité entre découpages est vérifiable depuis les artefacts archivés.

Ce verdict reste `diagnostic_sidecar`, `public_status=not_claimed`. Les prédicats Gabriel et les niveaux sont binary64, la topologie SoS de Geogram n'est pas recertifiée, et des incidences non-wedge restent nécessaires à Gamma$_2$ exact. Le champ `coverage_violation_count=0` ferme la partition construite `necessary` ou `strictly_lower_connected`; il n'est pas un oracle indépendant des niveaux ou de la complétude Gabriel massive. En particulier, deux niveaux exactement égaux peuvent être séparés par leur calcul binary64 : le lot réduit ne certifie donc pas la stricte antériorité exacte. Tant que cette recertification manque, le fallback conditionnellement sûr sous position générale est l'univers wedge entier, pas le lot réduit. Aucun de ces lots n'est encore raccordé au flux Morse terminal; ce diagnostic ne prouve ni l'exactitude de `hgp_reduced`, ni M.1, ni le pipeline $K=10$.

## Architecture et coût intermédiaire

Le générateur ne parcourt jamais « arête fois tous les points ». Il extrait le 1-squelette de la Delaunay ordinaire, construit son CSR et énumère uniquement les wedges canoniques par plages bornées de sommets. Sous position générale et pour le 1-squelette complet du nerf de Voronoï, tout triangle de Gabriel possède au moins deux arêtes de Delaunay, y compris lorsque sa miniboule a un support minimal de cardinal deux.

Le mode spécialisé supprime l'arène globale de tous les wedges valides de la tentative Gamma$_2$ restreinte. Il conserve encore l'arène globale des propositions Gabriel ou ambiguës afin de les trier par niveau, puis construit les facettes et la DSU sur hôte. À 30 000 001 points, la télémétrie observée pendant le run donne environ 42 GiB utilisés sur 176 GiB, zéro swap, tandis que le rapport compte 6 256 917 032 octets device au maximum. Cette résidence convient au diagnostic massif G4, mais elle n'est pas l'architecture produit finale; les runs triés segmentés et le merge externe restent à implémenter.

Les SHA-256 engagent séparément le nuage binary64, les arêtes Delaunay triées, l'univers implicite des wedges reconstructible depuis la règle de propriété canonique, les sources acceptées, les nécessaires acceptés, les ambiguïtés de sécurité et le manifeste composite du lot explicite.

Deux lots ne doivent pas être confondus. Sous les hypothèses génériques du lemme d'inclusion, l'univers wedge entier est le sur-ensemble conservateur de tous les triangles exactement Gabriel : il contient 4 405 823 candidats sur le nouveau 50 k, 887 005 885 à 10 000 001 points et 2 661 333 712 à 30 000 001 points. Le lot plus petit `necessary_accepted + ambiguous` résulte du classifieur et de l'ordre binary64; ses comptes 325 585, 67 858 286 et 204 114 412 ne sont pas un remplacement exact du grand lot tant que vacuité, ordre strict et égalités de niveaux ne sont pas recertifiés.

## Résultats

Les deux lignes 50 k portent la graine décimale `5570761781678721000` (`0x4d4f5253454847e8`) et le SHA-256 de nuage `70fdd798a382ee8ca17feb178ae7fc590b7f4dd1664f101200217a3b82541fbe`. Ce n'est pas la graine par défaut et canonique historique `5570761781678720848` (`0x4d4f525345484750`). Les deux découpages 50 k se recertifient mutuellement, mais ils ne recertifient ni les comptes 4 396 699/417 839 du nuage historique, ni son timing.

| points | sommets/chunk | chunks | wedges canoniques | Gabriel binary64 | nécessaires acceptés | reliés strictement plus bas | ambigus conservés | invalides | froid |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 5, fixture E5 | 2 | 3 | 10 | 5 | 5 | 0 | 0 | 0 | 198,603 ms |
| 5, fixture E5 | 3 | 2 | 10 | 5 | 5 | 0 | 0 | 0 | 160,853 ms |
| 50 000 | 10 000 | 5 | 4 405 823 | 417 869 | 325 585 | 92 284 | 0 | 0 | 744,955 ms |
| 50 000 | 50 000 | 1 | 4 405 823 | 417 869 | 325 585 | 92 284 | 0 | 0 | 670,080 ms |
| 10 000 001 | 50 000 | 201 | 887 005 885 | 87 631 258 | 67 858 274 | 19 772 984 | 12 | 0 | 244,228 s |
| 10 000 001 | 100 000 | 101 | 887 005 885 | 87 631 258 | 67 858 274 | 19 772 984 | 12 | 0 | 243,432 s |
| 30 000 001 | 150 000 | 201 | 2 661 333 712 | 263 693 761 | 204 114 328 | 59 579 433 | 84 | 0 | 862,603 s |

À 30 000 001 points, les supports de cardinal deux et trois comptent respectivement 119 304 632 et 144 389 129 triangles. Le lot explicite logique de sécurité contient 204 114 412 triangles après ajout des 84 ambiguïtés. Son SHA-256 est `56bcb074d94c6690965de232365afeae377c23f787c799407e19b61e2ff47b5e`; le SHA-256 des sources acceptées est `d57e19c7bb4ffb6620675e0e8e799b62ef2f120a07f08effa99bd30594eba5ae`. Les runs massifs ont `necessary_records=null` et `ambiguous_safety_records=null` : ils n'archivent pas les identifiants du lot, seulement ses comptes et engagements. Le lot n'est donc pas encore consommable ni recertifiable record par record hors du processus qui l'a construit.

Le profil 30 M attribue 777,691 s sur 862,603 s à la réduction hôte et 20,063 s au tri global hôte. Delaunay, extraction des arêtes et classification GPU prennent respectivement 8,330 s, 15,175 s et 6,257 s. Le prochain travail de performance est donc la segmentation et la réduction parallèle des facettes, sans modifier le critère scientifique.

## Validation et provenance

Le binaire CUDA 12.9 `sm_120` compile avec les avertissements stricts. La base locale est `b7ee14087e36bbd0fa3133f8f12f63e650d70f9e`; l'overlay de travail a le SHA-256 `0dcdb25eaf8390779907d3629be17930f42d83dff80cd8398156f6f38d0f81da` et le commit détaché temporaire embarqué par le binaire est `1b08121332bedf65d8434733110f5b75b6dea86d`. Aucun de ces changements n'a été poussé depuis la VM.

Geogram est épinglé à `v1.10.0`, commit `c8529bb00838186938ab31d96008a59b6a892dee`. Sur Ubuntu 22.04, la construction exige de transmettre explicitement `libtbb.so` dans `CMAKE_CXX_STANDARD_LIBRARIES`; le helper gardé est corrigé en conséquence.

La session utile cible `devpod-gpu-exploration / europe-west4-ai1a / ehgp-blackwell-spot-ai1a`, machine `g4-standard-48`, provisioning `SPOT`, `instanceTerminationAction=STOP`, `maxRunDuration=3600`, arrêt invité relu à 45 minutes et génération `2026-07-27T13:39:44.510-07:00`. Elle est arrêtée et certifiée `TERMINATED` à `2026-07-27T14:15:26.414-07:00`; aucune autre VM `project=e-hgp` n'est active et la clé OS Login éphémère est révoquée. Deux tentatives préalables ont échoué fermé avant benchmark : une échéance invitée de 55 minutes trop tardive sur la cible AI et un `terminationTimestamp` absent sur la cible `europe-west4-a`; leurs générations exactes ont aussi été certifiées `TERMINATED`.

Les sept sorties brutes et leurs sept rapports de contrôle, soit quatorze fichiers, sont archivés sous les noms `phase15_gabriel_coverage_*_g4*.json` dans ce répertoire. L'archive de transfert avait le SHA-256 `9110dbde931d738c0039f4358fe378361e46883f49e6eed81a26da7e916f8e55`.

## Portes restantes

- sérialiser transactionnellement le lot massif `necessary + ambiguous`, avec index, compte et SHA vérifiables, puis le raccorder à `ExactSparseDirectH0Candidate` ou à la façade terminale sans lui attribuer une autorité exacte non recertifiée;
- externaliser les runs triés afin de supprimer les arènes hôte globales observées à 30 M;
- recertifier exactement les rejets, les ambiguïtés, l'ordre et les égalités de niveaux ainsi que la complétude du 1-squelette en présence de dégénérescences avant de substituer le lot réduit à l'univers wedge conservateur;
- comparer l'inclusion bidirectionnelle au flux Morse borné puis à un vrai flux massif terminal;
- fermer séparément le SLO `warm_e2e` sous 100 ms à 50 000 points et $K=10$; le diagnostic présent vaut 670,080 ms à froid pour le seul gate $k=2$ et ne satisfait pas ce SLO.
