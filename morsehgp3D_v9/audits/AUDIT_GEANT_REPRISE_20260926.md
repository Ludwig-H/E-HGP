# Reprise indépendante de la v9 après R20 : objet, code, preuves et coût

26 septembre 2026 — audit transversal B. Base gelée pour cette note :
`c529c82bb` (`origin/main` au début de la reprise), intervalle de **67 commits**
depuis `a22dc9657`. Cadre : `exploration_v9_hors_registre`, profil de temps
`quantized_u18_input_only` à 1 mm, `public_status=not_claimed`. Aucun code du
moteur n'est modifié par cette reprise. GCP **non utilisé par cet audit** ;
les mesures G4 citées viennent des reçus R21 et R22. Le WIP du développeur
dans `build/v9-open-worktree` (sonde v29, lots compacts) n'est pas un résultat
publié de la base gelée. La session R23-C annoncée par C n'a pas de reçu dans
cette base.

## Verdict exécutif

La v9 est désormais une **chaîne FULL réellement exécutée sur G4**, avec
génération q2/q3/q4, catalogue canonique, recensement exact des boules
présentées, tour explicite K=1..Kmax et plusieurs portes d'égalité. Les
optimisations récentes ne paraissent pas avoir changé l'objet calculé dans les
cas testés. R22 donne **0,760–0,983 s** pour K1..5 sans sol sur trois trames
de la séquence 08, **2,269–2,994 s** pour K1..10 ; avec sol, K1..5 prend
**1,810–2,027 s** et K1..10 **5,311–6,010 s**. Il s'agit du temps de
`chain_total`, nuage déjà préparé, session d'appareil ouverte ; le mur du
processus est plus long. [Reçu R22](../receipts/g4_tower_r22_20260926/README.md),
[contre-lecture C](CONTRE_AUDIT_C_R22_20260926.md).

Le jalon exploratoire **1 s, K5, sans sol, trois trames** est donc franchi,
mais **ni le contrat principal brut entier**, ni K10 à 1 s, ni K5 à 100 ms,
ni une qualification multi-séquences ne le sont. Les trois trames restent
une seule séquence, et 08/000200 n'a qu'un passage R22 à 0,983 s : marge de
17 ms, insuffisante pour une promesse robuste. Les deux parcours 08/000000
valent 0,926 et 0,940 s. Le projet a raison de garder `not_claimed`.

L'obstacle aux 100 ms n'est **pas seulement le tri de Kruskal**. À K5 sans
sol, R22 mesure q3/q4 à 378–513 ms, le recensement à 86–109 ms et la tour
FULL à 246–300 ms. Chacun des deux premiers grands postes, pris seul, peut
dépasser le budget total. À K5 brut, q3/q4 vaut 861–1 018 ms et FULL
609–695 ms. Une simple micro-optimisation de q2 ou du tri ne peut combler
ces écarts ; il faut simultanément raccourcir le travail géométrique et
les passages hôte/mémoire, rendre la phase A de **tous** les ordres plus
économique, et supprimer les dépendances qui sérialisent la chaîne.

## 1. Ce qui a effectivement changé depuis la précédente reprise

Les commits produit sur l'intervalle se regroupent en quatre familles ;
les nombreux commits de reçus, de contre-audits, de présentation et des
projets voisins ne doivent pas être comptés comme des gains moteur.

| famille | changements publiés | preuve spécifique |
| --- | --- | --- |
| FULL E4 / E2 | Queue populations–images en pipeline, banque dense, pool persistant d'aides, options nommées ; correction/retrait du dimensionnement post-phase 0 ; priorité des erreurs et jointure avant destruction des minuteries | portes `tower_tail`, `task_pool`, `order_failure_priority`, huit combinaisons de leviers ; R21 compare tour et `tower_work` |
| Phase 0 FULL | Classes par hachage **exact** des requêtes et index de graines sans tri dominant ; collisions forcées dans les portes | `static_grouping_gate`, trois mutants d'identité/ordinal tués ; R21 mesure 300 ms pour FULL à 00/K5 contre 424 ms témoin apparié |
| Exactitude du catalogue | Positivité q3/q4 certifiée côté chaîne avant sceau, refus déterministe par plus petite clé ; `SealedCatalogue` lie le recensement à l'index et n'échantillonne la passe 1 de la tour qu'à 1/64 | `sealed_catalogue_gate`, T2 enrichi ; R22 égale les condensés et le travail de la tour ; résidu de corruption mémoire hors échantillon déclaré |
| Flux hôte/GPU | q2 recensé pendant les appels d'appareil, bassin d'enregistrements épinglés et réutilisable, transfert ventilé | `chain_q2_early_census_gate`, `record_pool_gate`, R22 et ablations |

Le [reçu R21](../receipts/g4_tower_r21_20260925/README.md) est le premier
test intégré de ces leviers FULL sur G4 : 34 cas complets, 22 comparaisons
égales, 12 épingles reproduites, dont les six trames brutes à K5/K10. Le
[reçu R22](../receipts/g4_tower_r22_20260926/README.md) en ajoute trois :
36 cas complets, 24 comparaisons égales, les 12 épingles reproduites.
Les fermetures SHA256 ont été revérifiées : **534/534 R21, 560/560 R22**.
Le GPU exécute réellement filtre/certificats/voies, mais le front WSPD,
la fusion, le recensement et FULL restent au moins en partie sur CPU.
Le cœur mathématique du générateur q3/q4 n'a pas reçu, sur cet intervalle,
de nouveau certificat géométrique majeur : dans
`src/gen/pipeline/wspd_q34.cpp`, le changement de fond relevé est une
borne de sûreté sur les IDs de support. Les gains R21/R22 viennent surtout
de l'ordonnancement FULL, du déplacement du recensement q2 et des
transferts/validations ; ils ne résolvent pas encore le volume d'aval q3/q4.

En plus de la lecture des reçus, le [volet mathématique frais](AUDIT_REPRISE_20260926_MATH_EXACTITUDE.md)
a reconstruit la base Release `c529c82bb` hors des builds épinglés et
rejoué **3/3 portes ciblées** : oracle T2 spatial q3/q4, recensement q2
précoce et catalogue scellé. La fixture cosphérique du juge de supports
recompilé sur ce HEAD retrouve **1 155/1 155** clés, huit coquilles
étendues, et tue deux mutants causaux. Les six trames brutes du reçu
historique n'ont **pas** été recalculées lors de cette reprise.

## 2. Quel objet est prouvé, et à quel niveau ?

La tour demandée est **explicite** : les nœuds, parents, relations verticales,
images et extensions non régulières doivent être disponibles dans le temps
annoncé. R21/R22 suivent bien ce chemin produit, et non une simple liste
compacte décodable plus tard. Les condensés, les présentations et
`tower_work` égaux entre bras rendent une régression d'objet récente peu
probable sur les cas joués. Les douze épingles comprennent brut et sans sol.

La nuance est majeure : `complete_relative` signifie **complet relativement
au catalogue recoupé**. Le moteur et un autre chemin de la même famille
peuvent omettre la même boule. Le juge T2 compare exhaustivement la tour au
modèle Γ sur de petits nuages (n ≤ 14) et le
[juge indépendant des supports bruts](c_raw_support_judge_20260925/README.md)
cherche adversarialement 675–1 024 clés admissibles par cas dans six cas
bruts, avec coquilles littérales ; il les trouve toutes et tue 14 mutants.
Ce sont de bonnes preuves **bornées**, pas une preuve globale de complétude
des trames LiDAR ou une borne de croissance. Euler est une condition
nécessaire sur les degrés vérifiés, pas un certificat de toutes les clés ;
les reçus le vérifient jusqu'à K−2, non jusqu'à K.

Le sceau R-29 n'est pas une suppression aveugle de contrôle : les supports
réguliers sont certifiés avant, par prédicats exacts, puis la tour conserve
la validation structurelle de sa passe 2 et un échantillon déterministe de
la passe 1. Cela conserve l'objet en exécution normale. Son **modèle de
faute** est toutefois plus faible que celui de la voie publique sans sceau :
une corruption isolée après recensement et hors indice 0 modulo 64 n'est
pas garantie détectée et pourrait même rendre une lecture hors bornes.
Le code actuel n'offre pas d'écriture intermédiaire vers ce catalogue
déplacé, mais la documentation et les futurs ports doivent préserver cet
invariant. Une faute périodique évitant l'échantillon n'est jamais vue ;
la formule « toute faute systématique est détectée » serait trop large.

La grille 1 mm est le **profil choisi pour le contrat temps** ; le float32
brut est secondaire et non qualifié par ces reçus. La segmentation du sol
est un prétraitement approximatif dont le masque est figé ; l'exactitude
HGP s'entend sur le sous-nuage déclaré. Les temps de masque, lecture et
quantification sont séparés de `chain_total`, et cette frontière doit rester
visible dans toute revendication de latence.

## 3. Relecture indépendante des temps G4

Toutes les valeurs ci-dessous sont celles de la première répétition du bras
GPU R22, `s=8`, `W=48`. Elles sont en **ms**, non des promesses statistiques.
Les phases du front, filtre, certificats et voies peuvent se recouvrir en
partie : ne pas les soustraire naïvement à `q34`.

| trame | sites | K | chaîne | q3/q4 | front CPU | filtre | certificats | voies | census | FULL | mur externe |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 08/000000 sans sol | 39 885 | 5 | 926 | 482 | 98 | 96 | 119 | 81 | 102 | 289 | 1 718 |
| 08/000100 sans sol | 35 551 | 5 | 760 | 378 | 75 | 67 | 99 | 66 | 86 | 246 | 1 519 |
| 08/000200 sans sol | 45 845 | 5 | 983 | 513 | 98 | 98 | 132 | 94 | 109 | 300 | 1 874 |
| 08/000000 brut | 123 389 | 5 | 1 991 | 1 018 | 288 | 149 | 211 | 185 | 210 | 632 | 3 478 |
| 08/000100 brut | 124 479 | 5 | 1 810 | 861 | 238 | 135 | 179 | 141 | 207 | 609 | 3 280 |
| 08/000200 brut | 125 526 | 5 | 2 027 | 970 | 256 | 160 | 202 | 166 | 229 | 695 | 3 678 |

La comparaison **appariée et entrelacée** R22 avec trois leviers coupés
n'existe que pour la seconde répétition de 08/000000 : gain total environ
62–68 ms à K5 et 224–232 ms à K10. La fourchette plus large publiée
62–82 / 224–266 ms utilise aussi une répétition jouée une minute plus tôt.
Les ablations individuelles n'ont qu'un passage chacune. Il faut donc
créditer fortement le mécanisme interne observé — passe 1 50→34 ms à
K5, copie 30,6→4,8 ms, par exemple — tout en qualifiant prudemment son
effet sur la latence complète.

Le bassin épinglé déplace son coût : 256 Mo à K5, 1,28 Go à K10, réservés
hors chaîne (environ 39 ms / 190–201 ms). Sur un **processus d'une trame**,
le mur externe ne gagne pratiquement rien à K5 et régresse d'environ
104–151 ms imputables au bassin à K10. Le crédit industriel exige une
boucle de plusieurs trames dans la même session, avec allocation amortie,
`chain_total` **et** mur externe par bras. La sonde v29 en développement
va dans cette direction ; aucun résultat v29 n'est encore intégré ici.

## 4. Pourquoi 100 ms n'est pas une extrapolation des gains R22

À 08/000000/K5, il faut retirer **826 ms** de la chaîne R22 pour atteindre
100 ms ; même la meilleure trame demande −660 ms. À 08/000200, q3/q4
seul vaut 513 ms, FULL 300 ms et census 109 ms. Le budget de 100 ms
réclame donc une refonte de la **structure du travail et de ses
dépendances**. Les 234–282 Mo estimés de tableaux explicites pour
08/000000/K5 ne démontrent pas, eux, un plancher d'écriture >100 ms ;
il faut mesurer débit et allocations, non invoquer la mémoire comme
impossibilité mathématique.

La phase A de la tour est un exemple de piège d'attribution. La fenêtre
FULL vaut `max_K(prêt(K)+A(K))`, la phase 0 préparant les ordres du haut
vers le bas. À K5 R22, A(5) dure environ 148 ms sans sol, mais sa seule
accélération ferait gagner **14–21 ms**, parce que A(4) finit presque
au même moment (**37–53 ms** sur les brutes K5). La recommandation
initiale « A(Kmax) d'abord » est donc
retirée : il faut alléger **tous** les A(K), puis la phase 0. À K10,
les ordres qui bornent varient (K6–K9 selon trame/sol) ; A(10) seule ne
borne pas aujourd'hui. La [contre-lecture C de R22](CONTRE_AUDIT_C_R22_20260926.md)
le démontre avec le modèle de fenêtre recalculé sur les mesures.

Le [profil local de phase A](c_phase_a_profil_20260926/README.md) indique
une direction utile au chantier `tower_compact_lots` : environ 39–41 %
du temps **échantillonné des lots** est imputé au segment par facette
(borne haute, pas uniquement les chasses de racine), 16 % au brouillon
plat dont environ la moitié des octets sont les niveaux, et les lots
groupés coûtent 5–9 fois un singleton. L'hôte était chargé et les
frontières `rdtsc` non sérialisées ; ce profil motive des ablations G4,
pas un gain prédit. L'allègement des niveaux seuls ne peut plausiblement
diviser A(K) par quatre. Le code compact en cours doit conserver
`tower_work` même en échec, les niveaux exacts non réduits, et les
priorités d'erreur déterministes.

Pour q3/q4, le front WSPD reste CPU et ses tâches traînardes allongent
le mur. Dans le brut K5 R22, le front est déjà 238–288 ms. La « glu »
hôte autour des appels GPU, les conversions/libérations et les transferts
ne sont pas des noyaux ; les noyaux ne saturent pas à eux seuls la chaîne.
Le GPU ne peut raccourcir la tour CPU en aval tant que le catalogue complet
est une barrière. Un recouvrement recensement/tour avec les appels de
l'appareil demanderait **une preuve de dépendances et d'identité**, pas
seulement des fils supplémentaires.

## 5. Complexité, séparation WSPD, et régimes LiDAR

Le [reçu de croissance locale](../receipts/lidar_scaling_local_20260923/README.md)
mesure, pour trois géométries sans sol et K5/K10, des nuages emboîtés
8k→16k→32k puis trame entière. Les temps de chaîne ont des exposants
empiriques 0,73–1,49 par doublement : **aucun temps quadratique mesuré**
sur ces six séries. Le nombre de boules a des exposants 0,75–1,03.
Mais certains compteurs internes du cœur diamétral montent à
**p=2,51–3,05** sur un des deux doublements selon la trame, avant
les certificats plus récents. Deux doublements et des entrées
sélectionnées ne constituent ni une borne asymptotique ni une
qualification sous-quadratique des versions R21/R22. Il faut refaire
une campagne déterministe avec les compteurs **du binaire intégré
actuel**, sans remplacer les trames brutes par des quarts, et conserver
les coupes géométriques par plans capteur comme diagnostics distincts.

À `s=8/10/12`, le [test local K5 sur un seul quart](b_s8_s10_s12_k5_quarter_20260924/README.md)
trouve le même objet et 46 218→40 728→37 843 paires étendues, mais
37 459→42 686→47 158 rectangles. Le front plus séparé baisse des
paires tout en créant des tâches ; sous forte contention locale, il
n'élit pas un `s` optimal. **R22 G4 n'exerce que s=8.** Le contrôle
d'invariance et les mesures appariées s8/s10/s12 sur trames entières
restent dus. Le plan R23-C annoncé par C les vise, sans résultat reçu.

Les dizaines de millions de sites ne sont pas qualifiées : coût des
structures, arènes et transferts, croissance des catalogues et sortie
explicite doivent avoir leur propre protocole. Le seuil de 100 ms sur
40k sites ne prouverait rien pour ce régime, et réciproquement.

Les essais historiques bruts décimés sont plus inquiétants que le seul
temps du sous-nuage sans sol : sur 08/000000 brut, 30 847→61 694→123 389
sites, le dernier doublement de `dead_core_form_sites` (formes chargées
hors les deux extrémités par cœur) a un
exposant empirique **2,136 à K5** et **1,936 à K10** ; au plein K10,
il y a **1 238 630 455** de ces évaluations et **11 387 391**
de boules. Ces chiffres sont des reçus CPU locaux v12 sur une trame,
[K5](lidar_raw_physical_scaling_20260923/README.md) et
[K10](lidar_raw_k10_density_20260923/README.md), **pas** des compteurs
du binaire R22/G4. Cela ne prouve pas une loi quadratique, mais interdit
de déclarer le régime brut sous-quadratique. À R22 b02/K10, le tampon
d'événements q4 atteint 4 091 sur 4 096 et huit voies sont reportées ;
le même cas cumule environ **1,299 milliard** d'incidences site–cœur,
**1,082 milliard** site–cover et **9,056 millions** d'enregistrements
de voies. Capacité, mémoire et repli sont à garder dans les campagnes
plus grandes. Voir le [volet q2/q3/q4](AUDIT_REPRISE_20260926_Q234_GPU.md).

Les projets voisins ne changent pas le statut v9. Le dossier
`Zoltan/FoundationModel` propose de consommer la tour, non de la calculer
plus vite. Sa phrase sur la commutation des rotations/translations avec
la tour vaut pour **l'objet géométrique continu**, pas pour l'exécutable
sur grille 1 mm : son propre README reconnaît que la quantification casse
cette commutation et demande un recalcul. `E-HGP` a un registre d'audit
distinct ; aucune de ses affirmations ne qualifie la v9.

## 6. Risques d'implémentation et lacunes des portes

Les audits récents ne signalent **aucun défaut d'objet avéré** dans
l'intégration R21/R22. Ils laissent des risques circonscrits :

1. **Observabilité/lecteur.** Sous sceau, le lecteur accepte encore un
   nombre nul de supports déclarés contrôlés ; le repli à zéro clé q2
   précocement recensée n'est pas visible au résumé, et certains champs
   de travail ne sont pas bornés. Une ligne peut rester `complete_relative`
   sans les planchers de chemin attendus. Voir R22-2/5 et R21-9/10/11.
2. **Portes négatives.** Le prédicat q3 copié côté chaîne n'a été forcé
   obtus qu'à une position ; erreur d'image d'un ordre bas sous une
   `Failure` d'ordre haut, voie q2 du point d'essai, bail du bassin rendu
   avant conversion et allocation du bassin sur `--device` demandent
   encore les fixtures indiquées par C (R22-1/4/9/10).
3. **Concurrence et durée de vie.** TSan a été exercé sur une coupe
   intégrée R21 sans rapport, mais sa couverture reste partielle et
   ne constitue pas une porte générale pour les huit combinaisons des
   leviers, `--unwind` et les prochaines continuations du front.
4. **Métrologie.** R21/R22 possèdent leurs hashes et arrêts gardés ;
   l'affirmation supplémentaire « TERMINATED relu en lecture seule »
   n'a pas de pièce jointe dans ces reçus. Les durées individuelles de
   certains leviers n'ont qu'une répétition. Le mur externe et le
   coût résident doivent être publiés côte à côte.
5. **Documentation vivante.** La suite de `PASSATION.md` conserve
   « A(Kmax) d'abord » alors que C a réfuté son rendement au K5 ;
   le README R22 immuable crédite la copie « divisée par 14 » sans
   qualifier son amortissement hors chaîne. Corriger dans la prochaine
   passation/reçu, pas en réécrivant les pièces historiques.
6. **Sonde v29 encore WIP.** Au commit `3cf62b8ca` du worktree de
   développement, `--frames=N` retient seulement le condensé de tour
   et les temps des répétitions après la première. `same_object` ne
   compare donc ni catalogue, ni présentations, ni `tower_work` par
   trame. Un refus ultérieur est normalement vu par son digest nul,
   mais un désaccord de catalogue donnant le même digest de tour ne
   l'est pas. Avant de créditer une boucle résidente, publier et juger
   les trois condensés et le statut **pour chaque trame** ; les trames
   répétées identiques mesurent d'abord amortissement/cache, pas un flux
   de scènes variées. Ce constat ne vise pas le reçu R22.
7. **Lecteur s8/10/12 historique.** Les sorties du quart gardent leurs
   hashes, leurs condensés d'objet concordants et leurs compteurs
   publiés, mais le script de vérification
   reconstruit un chemin d'entrée absolu courant et refuse `command
   argv` lorsqu'on relocalise l'archive. C'est un défaut de
   portabilité du lecteur, non une divergence de l'objet.

## 7. Ordre de travail recommandé au développeur

**P0, sans nouvelle campagne G4 lourde :** finir les portes
`tower_compact_lots` sur **tous** les ordres, avec comparaison
bit-à-bit du payload FULL, `tower_work`, priorité des erreurs et
épingles ; instrumenter la v29 pour séparer allocation/initialisation,
parcours et libérations du census, et chaque A(K), sur un témoin non
instrumenté. Ajouter les fixtures négatives prioritaires R22-10,
R22-2/5 puis R22-1/4/9. Le chantier des continuations du front doit
garder le même travail exact et les masques hérités, sans recréer les
ancêtres ou ajouter un quota caché.

**P1, une session G4 gardée :** bras entrelacés avec témoin non
instrumenté, s8/10/12 à objet comparé, `tower_overlap_static` 0/1,
mur externe et boucle résidente multi-trames pour le bassin. Publier
les temps d'étapes et comptes déterministes, pas seulement
`chain_total`. Ne pas multiplier les sessions si R23-C puis R23
répondent déjà à ces questions ; reprendre leurs reçus et arrêter la VM
ciblée avec preuve.

**P2, trajectoire 100 ms :** si les lots compacts ne divisent pas
fortement A(K), mesurer le coût des lots groupés/DSU, de l'initialisation
par boule et des passages explicites avant une nouvelle variante.
Réduire le front CPU et la glu hôte q3/q4 avant d'optimiser de quelques
pourcents un noyau q2 déjà caché. Tester une architecture plus massive
pour les tâches/certificats q3/q4 et la construction FULL seulement avec
une preuve de conservation des clés et de l'ordre des fusions. La
projection 1 s brut de C reste elle-même optimiste sur b02 même avec
plusieurs leviers réunis ; **100 ms** nécessite davantage qu'un
réarrangement de la phase A.

**P3, qualification :** plusieurs séquences SemanticKITTI, trames
entières sans et avec sol, K5 puis K10, sortie FULL explicite,
float32 séparé, s=8/10/12, réplications appariées, croissance physique
8k/16k/32k et coupes capteur, puis plusieurs dizaines de millions de
sites. Distinguer strictement « exact sur le sous-nuage déclaré »,
« complet relatif au catalogue », « oracle borné », et « contrat
de production acquis ».

## Sources complémentaires et portée

- [Passation produit](../PASSATION.md) et
  [provenance détaillée](../docs/PROVENANCE.md) : déroulé des leviers,
  limites des portes et mesures locales.
- [Contre-audit C R21](CONTRE_AUDIT_C_R21_TOUR_INTEGREE_20260926.md) et
  [R22](CONTRE_AUDIT_C_R22_20260926.md) : recalculs des reçus et constats
  de code ; le présent audit adopte leurs corrections chiffrées, pas
  leurs projections comme des faits.
- [Audit mathématique frais](AUDIT_REPRISE_20260926_MATH_EXACTITUDE.md) et
  [audit q2/q3/q4–G4](AUDIT_REPRISE_20260926_Q234_GPU.md) : portes
  rejouées, recoupement brut des 70 sorties R21/R22 et limites de
  complexité/séparation.
- [Audit FULL/phase A](AUDIT_REPRISE_20260926_FULL.md) : 17 portes
  locales et cinq cas T2 rejoués, trois mutations de lecteur acceptées, chronos
  chaîne/mur et portée du graphe événementiel.
- [Proposition C pour la phase A](PROPOSITION_C_PHASE_A_20260926.md)
  et [profil local](c_phase_a_profil_20260926/README.md) : vérifications
  utiles au code compact en cours ; aucune mesure G4 du nouveau code.
- [Architecture GPU 100 ms](ARCHITECTURE_GPU_100MS_Q34_FULL_20260924.md),
  [plan critique](PLAN_CRITIQUE_100MS_FULL_20260924.md),
  [octets FULL](CONTRE_AUDIT_FULL_R20_OCTETS_100MS_20260924.md) :
  scénarios expérimentaux, pas preuves de vitesse.

Ce document est un **instantané auditeur**. Toute modification de la v9
après `c529c82bb` exige un différentiel et un nouveau reçu avant de
changer le verdict.
