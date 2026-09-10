# Tour complète avec cache : local et G4

10 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
**Les tours 50k terminent ; ni 1 seconde ni 100 ms ne sont atteintes.**
L'autorité reste relative aux census exacts complets fournis : pas de
certificat général de complétude WSPD ni d'archive industrielle FULL.

## 50k sur G4, tour et verticales retenues

Le [reçu clos](../receipts/full_ball_scale_gpu_20260910/README.md) conserve
les commandes, sources, sorties et arrêts. Uniforme u16, graine 3, s WSPD=8,
48 fils pour les phases parallèles ; le constructeur FULL reste mono-thread.
`cuda_census_cpu_full` accélère seulement prefilter/census. Les coûts de
transfert froid, génération, tri, construction et digest sont inclus dans
`total_s` ; le temps externe du processus est conservé séparément.

| Tour | Route | Total (s) | Constructeur FULL (s) | Nœuds retenus |
| --- | --- | ---: | ---: | ---: |
| K=1..10 | CPU | 418,873 | 389,668 | 27 273 218 |
| K=1..10 | CUDA census + FULL CPU | 418,921 | 390,481 | 27 273 218 |
| K=1..5 | CPU | 33,853 | 27,228 | 4 209 792 |
| K=1..5 | CUDA census + FULL CPU | 33,569 | 26,983 | 4 209 792 |

Les signatures CPU/hybride sont identiques dans chaque paire, verticales et
contributions comprises. K10 traite 21 468 368 boules, dont quatre extra-shells ;
K5 en traite 4 010 348, dont trois. Ce sont les premières complétions de ces
tours dans cet instrument, pas un réétiquetage des refus historiques.
Une observation par configuration ne qualifie pas un gain temporel robuste.

À K10, les kernels census totalisent **189,346 ms**, mais la phase avec
préparation/reconstruction coûte 4,540 s : préparation 904,948 ms et
reconstruction hôte 2 931,852 ms. La construction FULL prend encore 390,481 s,
avec 41 986 201 MEB, 3 898 856 828 supports exacts et 39 922 832 hits du cache.
Un chrono kernel n'est donc pas le contrat de tour. À K5 : 29,799 ms de kernels,
0,846 s pour la phase et 26,983 s FULL ; 4 383 525 MEB / 44 413 779 supports.

La vraie gate Blackwell SM 12.0 passe 16 627 contrôles sur 4 116 boules
(q2=846, q3=2 085, q4=1 185), trois extra-shells et 17 rejets.
Les paires s=10/12 sur G4 sont **non exécutées** : la durée observée des
quatre processus (907,494 s) ne permettait plus de les terminer dans la
fenêtre de clôture. Leur plan conservé n'est pas une mesure.

## Croissance locale, un thread

Même snapshot de 68 sources que la session G4 optimisée, même entrée uniforme
et graine, K=1..10 avec verticales. Machine partagée, une observation par
configuration ; des compilations concurrentes ont notamment perturbé le
premier passage 8k. Aucun gain de temps apparié n'est déduit de ce triplet.

| n, s=8 | Total (s) | FULL (s) | MEB | Supports exacts | Nœuds | RSS max (KiB) |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 8 000 | 235,724 | 89,571 | 6 227 265 | 573 011 617 | 3 976 472 | 2 136 656 |
| 16 000 | 354,144 | 155,883 | 13 252 780 | 1 227 441 406 | 8 310 399 | 4 427 688 |
| 32 000 | 736,819 | 352,009 | 27 711 509 | 2 577 959 005 | 17 166 975 | 9 108 756 |

Les MEB sont multipliées par 2,128 puis 2,091, les sorties par 2,090 puis
2,066. Cela décrit ce régime uniforme, pas une borne pour tous les nuages.
Les digests coïncident avec le [triplet précédent](RESULTATS_TOUR_BOULES_20260910.md),
dont les temps ne sont pas des bras synchronisés de cette campagne.

| n=8 000 | Total (s) | FULL (s) | Candidats bruts |
| --- | ---: | ---: | ---: |
| s=8 | 235,724 | 89,571 | 3 144 017 |
| s=10 | 149,179 | 69,348 | 3 129 992 |
| s=12 | 157,080 | 75,519 | 3 123 497 |

Les trois sorties ont le même digest `cdd77e30…`, 3 976 472 nœuds et
6 227 265 MEB. La charge hôte change entre passages : **ce tableau ne
désigne pas s=10 comme optimum**. Il vérifie ici l'invariance de la sortie.
La comparaison s=10/12 à 16k/32k reste à mesurer séparément.

## Traçabilité, coût et suite

Snapshot optimisé `e8ef188d…`, manifeste sources `e1399d80…` ; constructeur
`910f45ba…`, journal `7608e70e…`, sonde `306e5a9e…`. Les digests complets
et sources sont dans le reçu. Le front WSPD optionnel a changé ensuite,
sans être consommé par cette sonde. Vingt CTests CPU passent sur sources
stables ; compilation/lien CMake CUDA locaux réussis, sans device local.
Une capture CTest initiale est conservée en échec pour dérive du seul test
d'injection ; le rejeu stable est distinct. Le premier essai G4 a échoué
en compilation NVCC, avant tout kernel ; l'adaptateur strict le corrige.

Les deux générations `2026-09-10T13:11:21.894-07:00` et
`2026-09-10T13:41:50.482-07:00` de la cible exacte
`devpod-gpu-exploration/us-central1-b/ehgp-v7-4fa0e0789a7d5bb06b787d35`
sont certifiées **TERMINATED**. Modèle SPOT, GCE STOP/3 600 s et arrêt invité
30 minutes vérifiés avant travail. Aucune autre VM n'a été modifiée ;
aucune autre VM étiquetée E-HGP active détectée à la clôture.

Les [optimisations intégrées](OPTIMISATIONS_CACHE_ET_GPU_20260910.md) réduisent
le travail et la résidence, mais le prochain levier majeur est le traitement
statique dédoublonné des résolutions, puis leur exécution CPU/GPU par lots.
Le journal incrémental et le front de témoins WSPD visent deux autres coûts.
Le contrôle des [quatre blocs nommés](../receipts/full_ball_named_blocks_20260910/README.md)
reste **NOT_EXECUTED_50K** : les digests globaux ne le remplacent pas.
Streaming, grands plateaux, résidence de sortie et identifiants massifs
restent à traiter avant plusieurs dizaines de millions sur G4. La
[borne de sortie](CROISSANCE_ET_BORNE_DE_SORTIE.md) interdit de promettre
une sortie FULL explicite universellement sous-quadratique.
