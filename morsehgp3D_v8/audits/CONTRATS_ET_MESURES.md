# Audit des contrats et des mesures v7

13 septembre 2026. Audit en lecture seule de la v7 publiée à `dc57ffd5`,
dans le cadre [v8](../README.md) : aucun moteur v8, aucun nouveau benchmark,
aucune compilation C++, GCP non utilisé. Les modifications v7 locales non
committées sont distinguées, pas promues. Les résultats cités ci-dessous
restent ceux de leurs sources v7 ; ils ne sont pas des résultats v8.

## 1. Conclusion immédiate

**La v7 a obtenu des tours FULL complètes à 50 000 points, mais pas le
contrat de vitesse. La majorité de ses dernières optimisations ne possède
pas encore de nouvelle mesure 50k.** Il serait donc incorrect de présenter
419 secondes comme le temps de toutes les variantes actuelles, ou les
performances de petits kernels CUDA comme celles d'une tour GPU.

| Obligation | Constat vérifié | Verdict |
| --- | --- | --- |
| Toute la tour K1..10, 50k, moins de 1 s | Dernières complétions publiées : 418,873 s CPU et 418,921 s hybride, le 10 septembre | Non atteint ; environ 419 fois le délai |
| Repli : toute la tour K1..5, moins de 1 s | Processus distincts : 33,853 s CPU et 33,569 s hybride | Non atteint ; environ 34 fois le délai |
| Puis 100 ms, au même périmètre | Aucun run de tour à ce délai | Non atteint ; écarts observés d'environ 4 189 et 336 fois |
| Exactitude du payload FULL | Preuves conditionnelles et confrontations indépendantes bornées ; sorties retenues avec contributions et verticales | Pas un certificat industriel universel de toute la chaîne |
| Mono puis multi-CPU | Triplets uniformes complets 8k/16k/32k ; groupes de résolutions effectivement distribués sur quatre workers dans le prototype privé | Acquis sur ces captures ; pas une montée en charge générale établie |
| Tour GPU FULL | Census exécuté sur carte ; primitives géométriques distinctes sur carte ; terminal complet seulement qualifié localement | Aucune tour dont toutes les étapes coûteuses sont GPU |
| Plusieurs dizaines de millions sur G4 | Notes de résidence et frontières d'indices, pas de tour complète mesurée à ces tailles dans le corpus examiné | Non qualifié |
| Répétitions contractuelles et p95 | Une observation par configuration dans les grandes séries examinées | Protocole du contrat non exécuté |
| s WSPD = 8/10/12 | Égalités de sorties sur des campagnes locales ; pas les trois séparations sur G4 à 50k | Comparaison temporelle contractuelle incomplète |

Référence contractuelle : [CONTRAT_PERFORMANCE v7](../../morsehgp3D_v7/docs/CONTRAT_PERFORMANCE.md).
Le délai porte sur une tour, jamais sur K10 isolé, ni sur un composant.
`smax=11` désigne la fenêtre requise pour K1..10 ; ce n'est pas s WSPD.

## 2. Les quatre vrais runs 50k FULL

Même nuage uniforme u16, n=50 000, coordonnées dans 0..65535, graine 3,
s=8. La G4 expose 48 CPU logiques ; `--threads=48` distribue l'amont,
**pas le constructeur FULL alors mono-thread**. La route hybride ne met
sur GPU que préfiltre/census. Ces mesures incluent le digest diagnostique.
Le temps externe du processus inclut aussi sa fermeture.

| Tour et route | Total sonde (s) | Externe (s) | Constructeur FULL (s) | RSS externe (KiB) |
| --- | ---: | ---: | ---: | ---: |
| K1..10 CPU | 418,872898 | 419,809931 | 389,667561 | 16 206 376 |
| K1..10 CUDA census + FULL CPU | 418,920854 | 419,887634 | 390,480778 | 16 318 064 |
| K1..5 CPU | 33,852713 | 34,006404 | 27,227780 | 3 785 592 |
| K1..5 CUDA census + FULL CPU | 33,569245 | 33,790222 | 26,983111 | 3 949 772 |

Sources brutes :
[CPU K10](../../morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910/gcp/optimized/output/cpu_n50000_k10_s8.summary.json),
[hybride K10](../../morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910/gcp/optimized/output/gpu_n50000_k10_s8.summary.json),
[CPU K5](../../morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910/gcp/optimized/output/cpu_n50000_k5_s8.summary.json),
[hybride K5](../../morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910/gcp/optimized/output/gpu_n50000_k5_s8.summary.json).
Les RSS sont ceux des fichiers `.stderr` de mêmes noms, par exemple
[CPU K10 GNU time](../../morsehgp3D_v7/receipts/full_ball_scale_gpu_20260910/gcp/optimized/output/cpu_n50000_k10_s8.stderr).
La [note de campagne](../../morsehgp3D_v7/docs/RESULTATS_TOUR_CACHE_G4_20260910.md)
déclare autorité, sources et clôtures.

Les digests et les compteurs de la signature géométrique/structurelle
commune coïncident pour chaque Kmax ; les compteurs CUDA restent propres
à la route hybride.
Une égalité de digest ne constitue pas à elle seule un oracle géométrique
50k indépendant. Les deux générations de VM sont historiquement closes,
arrêts ciblés certifiés ; la présente relecture n'a pas contacté GCP.

### Où passe le temps à 50k ?

| Phase (s) | CPU K10 | Hybride K10 | CPU K5 | Hybride K5 |
| --- | ---: | ---: | ---: | ---: |
| Index | 0,008 | 0,008 | 0,008 | 0,008 |
| Génération WSPD + voies q2/q3/q4 + émission | 12,231 | 12,259 | 4,464 | 4,422 |
| Tri/RLE des candidats | 3,126 | 3,169 | 0,323 | 0,326 |
| Préfiltre CPU | 2,641 | inclus census | 0,410 | inclus census |
| Census, avec préparation/transfert sur la route hybride | 2,756 | 4,540 | 0,439 | 0,846 |
| Constructeur FULL | 389,668 | 390,481 | 27,228 | 26,983 |
| Digest | 8,371 | 8,391 | 0,959 | 0,961 |

Ces postes proviennent des quatre JSON précédents. **Le tri/RLE amont
représente environ 0,75 % du total CPU K10 ; FULL environ 93 %.**
Même rendre ce tri-là gratuit laisserait 415,747 s à travail restant
inchangé. Ce calcul est une soustraction de postes observés, pas une
prévision matérielle ni une démonstration que tous les tris internes sont
négligeables. Le chronomètre FULL de cette sonde ne les isole pas.

Le travail FULL mesuré n'est pas « trier quelques faces puis Kruskal » :

| Volume, cumulé sur la tour | K1..10 | K1..5 |
| --- | ---: | ---: |
| Boules recensées | 21 468 368 | 4 010 348 |
| Blocs boule/ordre | 37 913 587 | 6 451 762 |
| Occurrences de représentants | 72 307 407 | 11 920 957 |
| Appels MEB du résolveur | 41 986 201 | 4 383 525 |
| Supports géométriques testés | 3 898 856 828 | 44 413 779 |
| Nœuds FULL retenus | 27 273 218 | 4 209 792 |
| Références de parents | 27 273 208 | 4 209 787 |
| Contributions | 16 495 216 | 2 491 412 |
| Références verticales | 27 173 227 | 4 109 801 |

Une MEB est ici la plus petite boule contenant une facette ; un appel peut
essayer de nombreux supports. Une résolution de facette peut ensuite
enchaîner plusieurs appels MEB et recherches d'intrus. Les 3,899 milliards
de supports K10 ne sont ni 3,899 milliards de faces de sortie, ni autant
d'arêtes à trier. Le changement K10→K5 divise le nombre de nœuds par environ
6,48 mais les supports testés par environ 87,8 : le coût de recherche
géométrique ne se déduit pas du seul volume final.

### Pourquoi les kernels rapides ne donnent-ils pas une tour rapide ?

Le census hybride K10 mesure 189,346 ms de kernels, mais **4 540,114 ms
de phase**. Il paie notamment 904,948 ms de préparation hôte,
2 931,852 ms de reconstruction hôte, 202,693 ms de H2D, 89,800 ms de D2H
et 155,077 ms d'allocation. Les captures comptent 83 lots,
2 428 787 648 octets de boules envoyées, 2 168 560 400 octets de sentinelles
envoyées et autant de résultats reçus, plus l'index. Ces volumes sont
cumulés, pas tous simultanément résidents ; `cuda_peak_bytes=57 474 504`
ne décrit pas le RSS ni toute la VRAM du système.

À K5, les kernels ne prennent que 29,799 ms, mais la phase 846,324 ms et
FULL 26,983 s. Cette expérience démontre un raccord GPU réel et un coût
de transport/reconstruction important ; elle **ne mesure aucun gain de tour
robuste**. Il faut réduire les allers-retours et conserver les objets
utiles sur device, puis paralléliser aussi les étapes restantes.

## 3. Chronologie : ne pas mélanger les objets mesurés

| Famille de mesures | Résultat utile | Ce qu'elle ne mesure pas |
| --- | --- | --- |
| 4 septembre : v6/v7 50k `verified_events_only` | Quatre paires terminées, uniform/terrain, K1..10/K1..5 ; v7 50,120/18,283/10,117/5,432 s | Pas FULL, pas les futures tours retenues ; coordonnées uniformes par défaut différentes |
| 6 septembre : première sonde FULL G4 50k | K10 refuse en 21,372 s, K5 en 5,646 s, zéro ordre construit | Aucun temps de complétion FULL ; refus extra-shell |
| 10 septembre : première tour par boules retenue | 8k/16k/32k complets et verticales conservées | Pas seulement des ordres détruits successivement |
| 10 septembre : cache + G4 ci-dessus | Les quatre complétions 50k FULL | Pas de nouveau temps des optimisations ultérieures |
| 11 septembre : résolutions statiques, puis semis après échange | Réductions de travail vérifiées et triplets locaux | Pas encore une requalification 50k |
| 11 septembre : atlas, flux, contraction des pivots, workers, rangs | Prototypes privés complets confrontés au Builder et petits oracles | Pas intégrés automatiquement à la CLI active |
| Après `dc57ffd5` : marques dans le premier parcours | Fichiers/captures locaux présents | Delta non publié à la coupure de cet audit ; exclu des références ci-dessous |

Sources :
[paires réduites du 4](../../morsehgp3D_v7/docs/RESULTATS_G4_20260904.md),
[refus du 6](../../morsehgp3D_v7/docs/RESULTATS_G4_FULL_20260906.md),
[tour retenue initiale](../../morsehgp3D_v7/docs/RESULTATS_TOUR_BOULES_20260910.md),
[semis après échange](../../morsehgp3D_v7/docs/SEMIS_APRES_ECHANGE_20260911.md),
[raccord privé à rangs](../../morsehgp3D_v7/receipts/rank_guard_streaming_20260911/README.md).

Les résultats FULL réguliers antérieurs qui libéraient chaque ordre,
refusaient un plateau ou s'arrêtaient sur quota restent utiles pour
expliquer les corrections ; ils ne sont pas des concurrents chronométriques
de la tour complète conservée. Une censure ne fournit pas un temps final.

## 4. Triplets locaux complets : même famille, versions distinctes

Tous les triplets suivants portent sur uniforme u16, graine 3, s8,
K1..10. Les lignes sont des campagnes historiques distinctes, pas des bras
appariés : compilation, charge hôte et parfois périmètre temporel diffèrent.

| Version / threads amont-géométrie-consultations | Total 8k / 16k / 32k (s) | Interprétation |
| --- | --- | --- |
| Tour retenue initiale, mono | 215,169 / 417,627 / 965,053 | Digest inclus ; FULL conservé |
| Cache nominal, mono | 235,724 / 354,144 / 736,819 | Digest inclus ; première mesure 8k perturbée |
| Statique initial, amont1/statique4 | 466,761 / 686,049 / 802,278 | Hôte fortement variable ; baisse du travail établie, pas de scaling temporel attribuable |
| Statique semé après échange, amont1/statique1 | 141,366 / 318,968 / 694,459 | Triplet du binaire actif épinglé ; digest inclus |
| Privé, naissance dense + workers 4/4/4 | 92,963 / 215,381 / 489,601 | Synthèse/digests/comparaison exclus et séparés |
| Privé, mêmes objets + gardes par rangs 4/4/4 | 96,401 / 223,447 / 478,616 | Dernier triplet publié ; pas de gain temporel robuste contre le parent |

Références détaillées :
[tour initiale](../../morsehgp3D_v7/docs/RESULTATS_TOUR_BOULES_20260910.md),
[cache](../../morsehgp3D_v7/docs/RESULTATS_TOUR_CACHE_G4_20260910.md),
[statique initial](../../morsehgp3D_v7/docs/RESOLUTION_STATIQUE_CPU_20260911.md),
[semis](../../morsehgp3D_v7/docs/SEMIS_APRES_ECHANGE_20260911.md),
[workers](../../morsehgp3D_v7/receipts/parallel_birth_streaming_20260911/README.md),
[rangs](../../morsehgp3D_v7/receipts/rank_guard_streaming_20260911/README.md).

### Ce que la comparaison de threads démontre effectivement

Le paquet workers conserve à 8k trois processus successifs, sans autre
compilation/benchmark ROOT concurrent, mais sans exclusivité de l'hôte :

| Threads amont / géométrie / consultations | Total (s) | Atlas + géométrie + réduction (s) |
| --- | ---: | ---: |
| 1/1/1 | 187,214350 | 58,649086 |
| 1/4/1 | 164,702915 | 34,976049 |
| 4/4/4 | 92,963380 | 32,986996 |

La paire 1/1/1→1/4/1 garde les autres réglages et le travail identiques :
elle observe environ 1,68 fois moins de temps pour la phase composite et
1,14 fois pour la tour, sur une seule paire. Ce n'est pas un débit des seuls
kernels MEB, car atlas, tri, dispersion et réduction restent dans la phase.
La troisième ligne change aussi l'amont et les consultations ; son gain
ne doit pas être attribué au seul nouveau pool.

Le pool travaille réellement : les quatre workers de la deuxième ligne
traitent 786 996 / 786 878 / 790 469 / 762 842 tâches. La répartition
est dynamique. Les histoires elles-mêmes et les horizontales K restent
séquentielles ; quatre threads de consultations ne les parallélisent pas.

### Dernière décomposition publiée, à 32k

Le [JSON brut du run à rangs](../../morsehgp3D_v7/receipts/rank_guard_streaming_20260911/objects/8cc9dadd60995d03d02b8ec2ba52818b5b0321b948c05725667a18ea629a32d5)
est lié au nom logique
`build/v7_rank_guard_streaming_20260911/n32000_cpu4_r1/results.json`
par son [manifeste](../../morsehgp3D_v7/receipts/rank_guard_streaming_20260911/MANIFEST.json).

| Poste | Secondes | Part approximative des 478,616 s |
| --- | ---: | ---: |
| Génération WSPD/voies | 92,196 | 19,3 % |
| Tri/RLE amont | 4,370 | 0,9 % |
| Préfiltre | 21,523 | 4,5 % |
| Census | 16,176 | 3,4 % |
| Validation commune du catalogue | 42,562 | 8,9 % |
| Atlas + géométrie + réduction | 158,710 | 33,2 % |
| Reconstruction des histoires | 50,467 | 10,5 % |
| Export, avec consultations historiques | 92,044 | 19,2 % |

Index et libérations expliquent le reste ; arrondis non additifs.
Même supprimer totalement la phase composite atlas/géométrie/réduction
laisserait environ **319,905 s** sur cette exécution à travail restant
constant. Cette soustraction explique pourquoi accélérer les seules MEB
ne suffit pas ; ce n'est pas une borne de tout nouvel algorithme.

Cette même capture paie 19 784 213 MEB, 1 692 521 354 supports candidats
q2/q3/q4, 2 133 246 557 tests de puissance et 344 297 287 visites d'index
dans la recherche d'intrus. L'export fait 10 348 964 consultations
contributives, 17 102 987 consultations inférieures et 17 102 978 vérifications
de naturalité, avec 554 088 383 pas de recherche binaire. Beaucoup de
travail demeure donc après les terminaux géométriques.

## 5. Volumes, résidence et croissance

Dernier prototype privé publié à rangs :

| n | Boules | Occurrences | MEB | Naissances | Nœuds FULL | RSS (KiB) |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 8 000 | 3 113 381 | 10 456 312 | 4 359 540 | 2 404 646 | 3 976 472 | 2 874 208 |
| 16 000 | 6 526 716 | 21 948 186 | 9 364 101 | 5 026 402 | 8 310 399 | 5 807 080 |
| 32 000 | 13 502 432 | 45 453 599 | 19 784 213 | 10 380 964 | 17 166 975 | 11 607 552 |

Les boules forment le catalogue géométrique commun ; les autres volumes
sont cumulés sur les ordres. Le RSS est celui du processus.
Le 32k conserve aussi 23 851 396 marques. Les occurrences augmentent
par facteurs 2,099 puis 2,071 et les MEB par 2,148 puis 2,113 quand n
double ; le temps par 2,318 puis 2,142. Cela décrit un régime uniforme,
pas tous les régimes de nuages.

Les fenêtres réduisent la résidence des requêtes, **pas celle de tous les
catalogues ou de toute la sortie**. À 32k, la capture nomme 3 024 544 768
octets de capacité pour BallData, 1 963 990 944 pour les candidats après
RLE et 216 038 912 pour les survivants. Ce sont des capacités à des étapes
nommées : on ne doit pas les sommer avec le RSS ou des pics non simultanés.

Il existe une [borne mathématique de sortie](../../morsehgp3D_v7/docs/CROISSANCE_ET_BORNE_DE_SORTIE.md)
quadratique dès K2 pour des nuages 3D exacts à précision croissante :
au moins un minimum FULL par paire croisée dans la construction étudiée.
Elle ne prouve pas une asymptotique infinie dans le profil u16 fini ni un
temps matériel impossible à 50k. Elle interdit en revanche de promettre
l'énumération explicite FULL universellement sous-quadratique indépendamment
de la taille de sortie. Une représentation implicite doit annoncer ce
qu'elle restitue et le coût de son expansion.

Le contrat massif demande séparément 10 000 001, puis 30/50/100 millions
de points. Les [notes de résidence](../../morsehgp3D_v7/docs/RESIDENCE_MASSIVE.md)
analysent des largeurs d'indices et des layouts ; aucune de ces analyses
n'est un run complet. La revue des docs et READMEs de reçus n'a trouvé
aucune complétion FULL à ces paliers. Il reste notamment à qualifier
catalogues globaux, identifiants/offsets, grandes coquilles, résidence
de sortie, interruption/reprise et export industriel. Extrapoler le RSS
uniforme 50k jusqu'à 30 millions ne constituerait pas une qualification.

## 6. s WSPD, familles et couvertures expérimentales

| Campagne | Comparaison effectivement exécutée | Manque pour conclure sur la latence |
| --- | --- | --- |
| Cache nominal, mono 8k | s8/10/12 : même sortie, 3 144 017 / 3 129 992 / 3 123 497 candidats | Charge hôte différente ; 235,724/149,179/157,080 s ne choisissent pas s10 comme optimum |
| Statique initial, 8k, amont1/statique4 | s8/10/12 : même calendrier, 4 185 184 MEB, 364 590 166 supports | Pas trois triplets 8k/16k/32k comparables |
| Privé workers puis rangs, n800 | Trois comparaisons physiques s8/10/12 | Ce sont des micros, pas des 8 000 points ni des 50k |
| G4 FULL 50k | s8 seulement | s10/s12 planifiés mais non exécutés, fenêtre de clôture insuffisante |

Sources : [cache/G4](../../morsehgp3D_v7/docs/RESULTATS_TOUR_CACHE_G4_20260910.md),
[comparaison statique](../../morsehgp3D_v7/receipts/static_s_factors_20260911/README.md),
[comparaison à rangs](../../morsehgp3D_v7/receipts/rank_guard_streaming_20260911/README.md).

L'invariance de la sortie sous s et le meilleur compromis de coût sont
deux résultats différents. Les triplets FULL récents ne couvrent qu'uniforme,
une graine. Terrain du produit réduit, petits arcs adverses, plateaux et
tests structuraux ne remplacent pas une campagne FULL de croissance sur
chaque régime.

## 7. Ce qui a réellement tourné sur GPU

| État | Pièce probante | Limite |
| --- | --- | --- |
| Census exécuté dans une tour50k | Quatre runs de la section2, dont deux hybrides | FULL reste CPU |
| Sélection MEB par lots sur vraie G4 | 605 cas, 21 432 contrôles | Gate, pas benchmark de débit ni terminal complet |
| Clé/PGCD/division128 sur vraie G4 | 13 573 cas, 325 752 mots comparés | Gate de primitives |
| Terminal sans semis étendu K9/K10 | 1 577 requêtes hôte, compilation/lien SM120 | Aucun kernel de ce terminal exécuté |
| Terminal semé résident raccordé au Builder | Émulation hôte et NVCC strict ; 238 registres, pile 1 392 octets/thread annoncés à la compilation | Ressources compilateur, pas mesure d'occupation ou de débit |
| Tentatives G4 du terminal par lots | Deux préemptions US, un refus outils EU ; trois arrêts certifiés | Zéro compilation invitée, zéro kernel, zéro benchmark |

Sources : [primitives réelles](../../morsehgp3D_v7/docs/RESULTATS_PRIMITIVES_GPU_20260911.md),
[terminal K10](../../morsehgp3D_v7/receipts/gpu_static_terminal_k10_20260911/README.md),
[batch et ressources CUDA](../../morsehgp3D_v7/docs/PARALLELISATION_PAR_LOTS_20260911.md),
[tentatives G4](../../morsehgp3D_v7/receipts/terminal_batch_g4_20260911/README.md).
Compiler et lier un kernel ne qualifie ni son exécution ni ses performances.
Un émulateur hôte de transport n'est pas la carte.

## 8. Pourquoi il faut changer la manière de mesurer en v8

Les preuves existantes permettent déjà d'éviter plusieurs erreurs :
candidats/faces/naissances sont distingués, travail MEB réellement payé
compté, terminales comparées directement, lots égaux conservés, refus non
réétiquetés en succès. En revanche, les chronomètres sont souvent trop
agrégés pour arbitrer précisément la prochaine structure parallèle.

Pour la v8, demander un tableau de travail et de résidence par étape :

1. Front WSPD : rectangles visités/séparés/tués/subdivisés, masses de
   paires, visites pour trouver les témoins, tailles maximales et distribution
   des tâches, par voie q2/q3/q4.
2. Génération : coût q2, q3 et q4 séparé, covers visités, seeds, racines
   balayées et candidats émis ; ne pas appeler tout cela « construire WSPD ».
3. Géométrie : demandes totales/uniques, MEB/supports/puissances/visites,
   réemplois, tri des requêtes, dispersion et temps des workers séparés.
4. Graphes et forêts : arêtes avant/après réduction, pivots, naissances,
   coût de normalisation, tris exacts et tris entiers, reconstruction
   des plateaux ; séparer Kruskal, histoire et export.
5. Entrées/sorties : octets lus/écrits/copied par phase, pics simultanés,
   allocations et transferts froids ; distinguer payload logique, RAM,
   VRAM, export en mémoire et archive effectivement sérialisée.

Puis exécuter mono, quatre CPU et G4 sur les **mêmes objets et sources**,
avec paires à réglage unique, s8/10/12, les trois tailles locales et plusieurs
familles. Le protocole contractuel demande deux échauffements, dix nuages
frais par famille, p95 et mémoire ; aucune série ci-dessus ne le remplace.
N'engager une campagne 50k/G4 que sur la chaîne raccordée visée, en gardant
les compteurs complets et les arrêts gardés.

## 9. Vérifications réellement faites pour cet audit

Les trois lecteurs de paquets clos ont été exécutés le 13 septembre 2026,
en Python normal et `-O`, soit **six commandes, six codes0** :

- `full_ball_scale_gpu_20260910/verify.py` : 329 fichiers, quatre tours50k
  reconnues, deux arrêts ciblés certifiés ; `contract_qualified=false`.
- `rank_guard_streaming_20260911/verify.py` : 894 fichiers logiques,
  12 captures historiques, comparaisons parent et s vérifiées ; aucun
  nouveau calcul géométrique réalisé par le lecteur.
- `terminal_batch_g4_20260911/verify.py` : trois sessions, trois arrêts,
  zéro commande invitée de calcul, zéro benchmark et aucun device exécuté.

[Compte rendu JSON v8](../receipts/audit_v7_20260913/CONTRACT_CHECKS.json) :
commandes, dates d'observation, codes, résumés réellement retournés et
pins des manifestes/lecteurs. Les hashes sont observés après la relecture,
pas une nouvelle capture avant/après des vieux exécutables. Ce compte
rendu n'est ni un lecteur nouveau ni une requalification C++.

La v8 n'hérite d'aucun statut `exact`, d'aucun temps mesuré et d'aucune
qualification massive. Son apport à ce stade est cet audit vérifiable
et le découpage des questions à résoudre.
