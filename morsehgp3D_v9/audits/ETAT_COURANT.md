# État courant des audits v9

**Reprise transversale du 26 septembre** (base `c529c82bb`, avant les
WIP v29/R23) : [audit géant](AUDIT_GEANT_REPRISE_20260926.md),
[exactitude et juges](AUDIT_REPRISE_20260926_MATH_EXACTITUDE.md),
[q2/q3/q4 et G4](AUDIT_REPRISE_20260926_Q234_GPU.md). Les reçus R21/R22
et leurs empreintes tiennent ; les optimisations intégrées préservent
l'objet sur les cas jugés, sans nouvelle preuve globale de complétude.
R22 franchit 1 s pour K5 sans sol sur trois trames de la **seule**
séquence 08 (0,760–0,983 s de chaîne), mais ni le contrat brut,
ni K10 à 1 s, ni 100 ms, ni plusieurs séquences. Le bassin épinglé
réduit la chaîne, mais le coût d'une seule trame avec ouverture du
processus reste neutre à K5 et défavorable à K10 ; une boucle
résidente est à mesurer. Les anciennes courbes 8k/16k/32k ont des
temps sous-quadratiques mesurés sur leurs cas, **pas une borne** :
certains compteurs internes dépassent p=2, et R22 n'a pas de panneau
de croissance actuel. `s=8` seul sur R22 ; s10/12, trames brutes et
plusieurs séquences demandent des portes propres. Le modèle de fenêtre
FULL corrige la priorité : alléger A(K) pour **tous les ordres**, pas
A(Kmax) seule.

Le [reçu R22](../receipts/g4_tower_r22_20260926/README.md) est le
dernier chrono G4. Sans sol, K5 vaut 0,760 à 0,983 s et K10 2,27 à
2,99 s. Avec sol, K5 vaut 1,81 à 2,03 s et K10 5,31 à 6,01 s. Les douze
épingles sont reproduites, dont les 30 cas scellés. La [contre-lecture C
de R22](CONTRE_AUDIT_C_R22_20260926.md) tient le reçu et les trois
séries (sceau R-29, recensement précoce, bassin épinglé). Le gain du
bassin est payé hors de la chaîne : il est neutre à K5 et une perte à
K10 par processus d'une trame. C y corrige sa recommandation R21 : il
faut alléger la phase A de tous les ordres, pas A(Kmax) seule.

Le [reçu R21](../receipts/g4_tower_r21_20260925/README.md) était le
chrono G4 précédent. Sans sol, K5 vaut 0,805 à 1,026 s et K10 2,45 à
3,21 s. Avec sol, K5 vaut 1,91 à 2,18 s et K10 5,71 à 6,49 s. Les douze
épingles sont reproduites, dont les six brutes.

Sa [contre-lecture C](CONTRE_AUDIT_C_R21_TOUR_INTEGREE_20260926.md)
tient le reçu et ne trouve aucun défaut d'objet dans la tour intégrée.
Elle confirme 13 constats de portes, de lecteur ou de statut, tous de
gravité basse. ThreadSanitizer est sans rapport, mais sa couverture est
partielle. Deux constats de temps :
- à K5, la fenêtre de la tour est bornée par A(Kmax), pas par les
  ordres bas ;
- le plan annoncé ne ramène le brut K5 que vers 1,8 à 1,9 s
  (projection).

La [feuille de route auditée vers 100 ms explicites](PLAN_CRITIQUE_100MS_FULL_20260924.md)
pose une enveloppe **expérimentale** de 10/35/15/35/5 ms pour
préparation, génération concurrente, canonisation/census, FULL et marge.
Sur les trois trames sans sol R20 K5/s8 de la seule séquence 08, la chaîne
reste à **1,010/1,109/1,260 s** ; au pire, q3/q4, l'aval de
canonisation et FULL demanderaient respectivement environ ×17,6, ×10,2
et ×13,0 pour cette enveloppe. La sortie explicite et ses octets sont
dans le budget ; aucun de ces gains n'est acquis. Les autres profils
(K10, avec sol, s10/12, plusieurs séquences, coupes et densités,
plusieurs dizaines de millions de sites) restent des portes séparées.

La [contrelecture mathématique des boîtes BVH avant S3](CONTRELECTURE_FORMES_BVH_AVANT_S3_20260924.md)
prouve qu'un nœud spatial entier peut être crédité sur une cellule de
centres par huit maxima de coins exacts, à condition de maintenir une
antichaîne de sites **par cellule** ; un nœud qui échoue au test ne peut
pas être supprimé sans minorant distinct. Le [pilote borné K10](b_spatial_block_probe_plan_20260924/README.md)
termine ses 256 arêtes d'un quart de 1 288 sites : 136 fermetures
complètes contre 93 pour le cœur seul, mais 5,12 millions de coins et
2,93 millions de tests ponctuels pour 8 287 incidences de cœur seulement
*éligibles* à être évitées. Chaque nœud crédité ne regroupe en moyenne
que 1,78 site. Cette réalisation est un **signal de coût défavorable**,
non un gain du moteur ; K5 exige une trace S2 distincte. Le
[retest du WIP R21](CONTRE_AUDIT_R21_PREFLIGHT_20260924.md) échoue au
selftest nominal sur un plan 30 cas attendu encore comme 18 ; son lecteur
accepte sous Python normal/`-O` un mutant dont la session CUDA de 20 s
précède un mur externe de 10 ms. **À la date de ce retest**, G4 restait
en attente d'un préflight réparé et gelé ; R20 était alors le dernier
chrono reçu. R21 et R22 ont depuis clos cette attente, sous leurs
propres protocoles et limites.

Le [relevé audit-only S2→S3](b_s2_trace_20260924/README.md) fournit
maintenant, sur **un seul quart sans sol de 1 288 sites** (08/000200,
K10/s8/W8), les 55 657 arêtes survivantes avec ordinal, rectangle,
masques et `F_e`. Son lecteur normal/`-O` recoupe le reçu CPU historique :
52 842 rectangles, 95 830 paires étendues et `ΣF=1 151 766`. Les
segments couvrent aussi les rectangles vides ; un lecteur de jointure
refuse bits non demandés, doublons et ordinals hors bornes. C'est une
base de mesure, **pas** un gain GPU ni FULL. Une [palette de blocs
ponctuelle](b_moments_palette_20260924/README.md) a depuis joint de
vraies décisions avant cœur à cette trace : même 32 blocs candidats
par arête ne ferment qu'**une arête**, `F=66` (0,0057 % de `ΣF`). Son
autre essai sur 120 rectangles lourds K5 d'un quart de 11 461 sites
ferme zéro voie avec 128 blocs voisins. Ces résultats rejettent les
**choix de blocs naïfs** testés, pas tout certificat ni le régime K5
entier. Ils doivent empêcher un port GPU prématuré de cette variante.

La [trace K5 distincte](b_s2_trace_k5_20260924/README.md) sur ce même
quart fige 37 459 rectangles, 46 218 paires étendues, 27 099 survivantes
et `ΣF=298 205`. Le cœur ferme 7 020 arêtes, pesant 146 394 incidences
de sites ; S3 ferme finalement 9 447 arêtes. Lecteurs normal/`-O`/LIVE,
deux mutants et recoupement d'un **nouveau** run produit CPU local
passent. Ce run appartient à la même famille de code et ne remplace
pas un oracle géométrique indépendant ; son unique `chain_total=869,186
ms` est un chrono local sur 1 288 sites, pas une qualification G4. Cette
trace sert à mesurer une sonde pré-cœur **K5** avec ses vrais ordinals,
ses deux masques et le travail aval évitable, sans recycler K10.
La [sonde BVH K5](b_spatial_block_probe_k5_20260924/README.md) a depuis
achevé 256/256 arêtes stratifiées : 133 ferment tous leurs bits S2,
contre 87 par le core, avec 46 fermetures supplémentaires de masse
`F=1 214`. Mais elle dépense **6,842 millions** d'évaluations de forme
pour `ΣF=5 634` sur le même échantillon (dont 4,262 millions aux seuls
coins) et regroupe 1,46 site par nœud crédité. Cette implémentation
est économiquement **négative**, même avant un port GPU ; la sélection
stratifiée n'autorise aucune extrapolation à la trame entière.
Une [comparaison locale s=8/10/12](b_s8_s10_s12_k5_quarter_20260924/README.md)
sur le même quart K5 trouve les mêmes 27 099 survivantes et les mêmes
digests catalogue/tour ; les paires q3/q4 étendues descendent de 46 218
à 40 728 puis 37 843, tandis que les rectangles montent de 37 459 à
42 686 puis 47 158. Les murs d'un hôte partagé varient trop pour élire
un s. Ce test ne remplace pas l'appariement G4 multi-trames demandé.

L'[audit des octets explicites FULL](CONTRE_AUDIT_FULL_R20_OCTETS_100MS_20260924.md)
compte, à R20/08/000000/K5, 207,496 millions d'octets de tableaux à
tailles connues et 26,242–74,296 millions d'octets logiques dérivés pour
la banque partagée : environ 234–282 millions d'octets au total. Le reçu
ne donne pas la capacité physique exacte
de la banque. Ce volume **ne démontre aucun plancher d'écriture supérieur
à 100 ms** ; le verrou mesuré demeure le travail géométrique/phase 0 et
l'amont q3/q4. La [contrelecture de complétude des épingles brutes](b_full_raw_completeness_gap_20260924/README.md)
montre simultanément que moteur et lots CPU peuvent omettre la même
boule sans rompre leur égalité ni `complete_relative`. Elle propose une
porte indépendante et bornée de supports q2/q3/q4 tirés des coordonnées
brutes avec census global exact et IDs de coquille littéraux. Le
[juge de C](c_raw_support_judge_20260925/README.md) la réalise : **vert
sur les six cas** bruts, avec 675 à 1 024 clés admissibles distinctes
par cas, toutes présentes avec leurs IDs complets. Les quatre strates
sont non vacantes, 14 mutants sont tués et la fixture étendue est
exacte. C'est une recherche adverse bornée, pas une complétude, ni
GPU/G4.

Le [grand audit C](AUDIT_C_GRAND_AUDIT_V9_20260924.md) classe les postes
et projections vers 100 ms ; sa [contrelecture B](CONTRE_AUDIT_B_GRAND_AUDIT_C_100MS_20260924.md)
confirme l'écart mesuré, mais refuse de transformer les projections en
preuve d'« impossibilité ». Sur R20/08/000000/K5, la différence
arithmétique **78,871 ms** entre q3/q4 et ses sous-chronos (qui peuvent
se recouvrir) est déjà **dans** `q34_ms`/`chain_total`, non hors
chronomètre. L'ordonnancement décroissant de la phase statique est
optimal seulement sous hypothèses de durées fixes et ressources
indépendantes, non démontrées pour les 48 CPU partagés. Les 100 ms de
tour explicite restent l'objectif utilisateur ; aucun moteur ni reçu
G4 nouveau depuis R20 à cette lecture.
La [note d'architecture GPU/100 ms](ARCHITECTURE_GPU_100MS_Q34_FULL_20260924.md)
sépare deux refontes encore hypothétiques : tâches WSPD/masques sur
appareil avec baisse de travail **avant** paires et core, puis cibles
terminales batchées et graphe d'événements FULL avec sortie explicite.
Elle conserve census global, coquilles entières, ordinals et niveaux
stricts/fermés ; elle exige des portes littérales avant tout gain déclaré.
Le pilote BVH ponctuel K5/K10 ne justifie pas de port direct.

Le [shadow rectangle à moments](moments_rectangle_shadow_20260924/README.md)
donne un **résultat négatif utile** sur un quart 08/000200 sans sol :
parmi 120 rectangles ouverts lourds de masse cumulée 897 149 paires,
le bloc spatial naïf de ≤64 sites n'en ferme aucun uniformément aux 64
coins ; pourtant 14 rectangles ont une paire sondée fermable avec ce
même bloc. Le préfiltre d'intervalles entier est sûr, mais le verrou est
ici l'uniformité de la grosse boîte et le choix du bloc, pas seulement
l'arithmétique des coins. Même deux niveaux de tuilage disjoint ne
ferment au plus que 4 620/897 149 paires lourdes pré-S2 (0,515 %), et
le préfiltre ne ferme aucune des 840 tuiles. Aucun gain S2, FULL ou G4
ne s'en déduit ; prochaine porte : bloc mieux choisi ou certificat par
arête, joint à `ΣF` réellement évitée.

Le [préflight indépendant R21/v26](CONTRE_AUDIT_B_PREFLIGHT_R21_V26_20260924.md)
trouve un défaut du lecteur : il ne confronte pas les durées d'ouverture
du contexte et de réservation GPU au mur externe, et accepte causalement
deux millions de millisecondes de session pour 0,001875 s de processus.
Le WIP brut à 30 cas garde aussi des autotests de scénario à 18 cas ; la
porte doit être rejouée sur SHA figé avant G4. Aucun reçu R21 publié.
La sortie explicite FULL est bien dans `chain_total`, mais un bras « chaud »
par nouveau processus ne prouve pas encore un flux multi-trames persistant.
À K5, enlever gratuitement la phase statique FULL de R20 laisserait encore
environ 223 ms de FULL, en plus de 636–804 ms hors FULL : 100 ms exige
une refonte couplée de l'amont q3/q4 et de la tour, pas seulement le tri.

Le [pool E2](CONTRE_AUDIT_B_POOL_E2_WIP_20260924.md), maintenant au commit
local `89b977f34`, réemploie les fils du propriétaire de phase 0, mais ne borne
pas les K runners et leurs auxiliaires : jusqu'à 293 fils présents
à K5/W48 par le code, pic LiDAR non mesuré. Un reproducteur compilé
montre qu'une erreur ouvrière peut contaminer le job brut suivant si
propriétaire et ouvrier lèvent ensemble ; les wrappers du moteur capturent
normalement cette paire, donc aucun faux résultat R20 n'est déduit. Le
nouveau gate de jonction tardive paraît couvrir correctement les indices
dynamiques, mais ce **même défaut d'exception** est reproduit au nouveau
SHA. Les fils qui arrivent après fermeture peuvent ne jamais exécuter de
callback, alors que `static_lanes_used` et `static_workers_created`
comptent encore la largeur prévue. Aucun reçu
E2 G4 ni résultat TSan nouveau n'est acquis à cette lecture.
Le [groupement haché de phase 0](CONTRE_AUDIT_B_GROUP_HASH_WIP_20260924.md)
est un commit local frère, non intégré à E2 : son gate compare toutes
les cibles statiques hachées/triées, mais pas les tours explicites
littéralement. Le `252e6794e` renforce son contrôle de statut, de
premiers ordinaux et de chemin pris. Six portes synthétiques passent
sur un build Release existant, trois mutants tués causalement ; ni
reçu LiDAR, ni compilation fraîche épinglée, ni gain G4 publié.
Les deux WIP
occupent le même argument booléen de l'API avec des sens différents ;
leur assemblage ne peut garder cet argument positionnel unique.
Un troisième commit frère,
[E4](CONTRE_AUDIT_B_QUEUE_E4_WIP_20260924.md), ajoute lui aussi un
booléen au même emplacement pour recouvrir populations et images.
Il pré-calcule des offsets dans `validate_ms`, non dans
`populations_ms` : seul FULL total apparié jugera son effet. La sonde
G4 ne sérialise pas encore ce mode ni les nouveaux sous-chronos ; aucun
reçu E4 n'est disponible. L'intégration exige désormais **trois**
options distinctes et huit combinaisons de gate. Le pic de fils/RSS
peut augmenter et reste non mesuré.
La [contrelecture de la reprise E4](CONTRE_AUDIT_E4_TAIL_REPRISE_20260924.md)
constate que `5394a975d` annule dans le moteur l'essai de dimensionnement
tardif `fcf708d27`, après absence de gain RSS local et forte croissance
de la queue exposée. Aucune race, erreur de résultat ou interblocage
avéré n'a été trouvée sur ce chemin. Restent non couverts : exceptions
d'allocation/lancement concurrentes, pic K10/W48 de fils et mémoire,
et comptabilité additive des chronos en cas d'échec statique tardif.
`populations_ms` mesure seulement une queue visible, jamais tout le
travail B ; E4 n'a ni reçu G4 ni résultat FULL à 100 ms.

Les [épingles CPU brutes de C](CONTRE_AUDIT_B_EPINGLES_BRUTES_C_PREFLIGHT_20260924.md)
sont publiées au `5102ec2cc` sur trois trames entières **avec sol** de
la seule séquence 08. Les 29 fichiers du manifeste passent SHA-256 ;
les 12 sorties ont code nul et statut `complete_relative`, et les six
paires moteur/lots coïncident pour les digests et objets de tour. Le
script crée néanmoins `DONE` sans vérifier ces conditions ; un lecteur
indépendant [versionné](b_raw_pin_reader_20260924/README.md) ferme
maintenant 12/12 cas, 6/6 paires, 3/3 entrées et 29/29 empreintes,
en normal/`-O`, avec trois mutations causales rejetées. Les épingles
restent à intégrer au **protocole R21**. L'invariant Euler n'est contrôlé
que jusqu'à K−2, et les JSON
disent `grid=unspecified` malgré la provenance 1 mm indépendante.
Ces épingles ne sont ni chrono GPU/G4, ni qualification de plusieurs
séquences ou du float32 brut.

Le plan R21 à 30 cas reste bloqué avant G4 : attente nominale de 18 cas,
durées CUDA d'ouverture/réservation omises du garde-mur externe, et six
épingles brutes absentes de `PINNED_DIGESTS`. Le statut `partial` ne doit
pas dispenser les cas bruts. Son autotest nominal a été rejoué au
`61cfba666` : code 1 sur les labels GPU attendus des anciens cas, sans
aucun appel GCP. Le prochain test algorithmique à fort levier
est un [relevé complet **S2→S3**](CONTRE_AUDIT_B_MOMENTS_S2_SEAM_20260924.md)
des survivants/masques/ordinals et du
travail de cœur `F` par arête sans changer les sorties, puis un BVH exact
borné sur les groupes lourds. Le résultat positif actuel concerne un
groupe favorable d'une trame avec sol ; aucune sélectivité sans sol n'est
encore acquise. Le certificat multisite et son extension exacte aux
64 couples de coins sont recevables mathématiquement, mais aucun port
ni gain n'est validé. Neuf [fixtures entières de bord](b_moments_rectangle_edges_20260924/README.md)
passent en normal/`-O` (boîtes recouvrantes, contact d'extrémité,
égalités strictes et voies indépendantes) ; elles ne mesurent pas le
LiDAR. Le filtre après S2 ne peut pas réduire les paires
déjà développées ; le chemin GPU recherche encore deux fois le rectangle
de chaque paire, et ses plafonds `2³¹` imposeront une stratégie tuilée
pour le massif.

Le [contre-audit R20 et 100 ms](CONTRE_AUDIT_B_R20_ET_TRAJECTOIRE_100MS_20260924.md)
reçoit la session G4 publiée (`d1d038393`) : 326/326 empreintes et
lecteur normal/`-O` passent, 18 tours `complete_relative` et 12
comparaisons égales. Sur les trois trames **sans sol de la seule séquence
08**, s8/W48/u18, K1..5 prend **1,010 / 1,109 / 1,260 s** et K1..10
**3,147 / 3,873 / 3,860 s** (ordre 000100/000000/000200). Aucun
contrat d'une seconde n'est acquis. L'utilisateur a confirmé que les
**100 ms incluent la tour explicite** (tous nœuds, parents, liens et leur
matérialisation). La fusion L15 ralentit le noyau
apparié et reste désactivée ; les 30/151 ms « transfert des voies »
incluent aussi des opérations hôte. À K5, FULL seul coûte 374–456 ms,
et la chaîne **hors FULL** 636–804 ms : le préchauffage et l'épinglage
ne peuvent pas constituer une trajectoire de 100 ms. L'exactitude
publique reste relative au catalogue recoupé, les IDs de coquille ne
sont pas tous comparés littéralement, et trames brutes, autres séquences,
s10/s12 et croissance G4 restent à qualifier. Une preuve par concavité
séparée étend le certificat multisite de moments à un rectangle WSPD
entier via 64 couples de coins, **sans sélectivité ni vitesse LiDAR encore
mesurées** ; son shadow exact et la refonte parallèle de la tour FULL
sont les deux priorités architecturales proposées pour 100 ms.

Le [panneau CPU S3/S4a des secteurs physiques](s4a_cpu_scene02_physical_panel_20260924/README.md)
mesure maintenant **21 sous-nuages × deux bras** de 08/000200 sans sol,
K10/s8/W8 : scène entière, deux moitiés, quatre quarts, chacun aux densités
globales emboîtées 1/4, 1/2 et 1. Les 42 sorties et les 21 paires de
catalogues/ordres/compteurs communs sont validées ; 44 tentatives et 178
empreintes sont conservées. Pour `core_sites`, **7/18 pentes spatiales** et
**2/14 pentes de densité** atteignent 2 (maximum 3,296 et 2,077) ; les
pentes CPU·s de chaîne restent sous 2 dans les deux axes. À densité pleine,
la somme des cœurs des deux moitiés vaut **0,899** fois celui de la scène,
pour un repère quadratique de **0,508** calculé sur leurs tailles réelles.
Le quart chaud `x≥0,y<0`, **recalculé comme problème distinct**, traite
583,0 M formes contre 1 069,2 M pour la scène entière ; ces deux totaux
ne forment pas une partition des arêtes ni des formes du plein. Ses pentes
de densité sont 2,077 puis 1,916. Les murs sous garde sont
descriptifs, car la charge externe varie entre bras ; ce panneau d'une
seule scène et d'une seule graine n'est ni G4, ni FULL brut avec sol, ni
une borne asymptotique.

Un [certificat multisite par moments](moments_multisite_precore_20260924/NOTE.md)
donne une voie exacte pour éviter, sur certaines arêtes S2, la
matérialisation du cœur q3/q4 : un bloc de sites distincts et cinq moments
entiers suffisent à prouver **au moins K−1 intérieurs q3** ou **K−2
intérieurs q4** pour tous les centres admissibles de la voie. Il généralise
le test historique « au moins un intérieur par groupe » sans exiger une
partition en groupes ni un témoin individuel uniforme. Une fixture u18 K5
et un contrôle rationnel exercent la fermeture des deux voies et leur retrait
indépendant. C'est une **proposition non portée** : sélectivité, coût de
construction des blocs, mémoire et gain de chaîne restent à mesurer en
shadow sur les arêtes survivantes, aux densités emboîtées et dans les
secteurs physiques. Le filtre intervient après S2 ; il ne réduit pas
`expanded_pairs`.

Le [reçu G4 R15 S4a](CONTRELECTURE_G4_R15_S4A_20260923.md) est maintenant
publié : 326 empreintes relues, 18/18 tours `complete_relative`, six
condensés épinglés reproduits et 12 comparaisons appariées égales. Sur
08/000000, les paires S3/S4a de la même session donnent un gain réel de
**0,163–0,177 s à K5** et **0,374–0,409 s à K10**. Les meilleures
chaînes des trois trames sans sol de la séquence 08 restent **1,446 s à
K5** et **5,242 s à K10** ; le contrat de 1 s n'est pas atteint. Même
en retirant gratuitement toute la phase `edges_ms` CPU de R15, le reste
mesuré des trois K5 vaut 1,072–1,350 s si les autres postes restent
fixes : S4b et le chemin critique hors arêtes doivent progresser
ensemble. R15 G4 ne mesure ni demi-scènes, ni quarts, ni densités réduites ;
les pentes v12 CPU ne se transfèrent pas à S4a. Sur 08/000000, R14 et
R15 ont exactement les mêmes `core_sites` : **359,707 M à K5** et
**909,580 M à K10**. S4a accélère q3 en aval, sans réduire la
matérialisation des formes du cœur qui porte plusieurs pentes LiDAR
défavorables.

Le [préflight du prototype scratch S4b J8](AUDIT_S4B_J8_BIT_FINAL_20260924.md)
a décelé neuf bornes encodées dans `uint8_t` : le neuvième signe était
perdu et `lens[7]` restait zéro. Le constructeur a depuis élargi ces
masques à `uint16_t` dans son source local ; les rejeux K5/K10 à pas 20
sur 08/000000 donnent `design_compare=1` sur **7 771/85 491** objets
q4 des seules arêtes sondées. Cela corrige le préflight ponctuel, sans
source/binaire/reçu formellement reliés ni port produit.
Même après cette correction, une [famille u18 de seaux
vivants](AUDIT_S4B_J8_BIT_FINAL_20260924.md)
force `m²` comparaisons de racines par graine pour seulement `K−2`
groupes peu profonds ; `m=6554` donne 42 954 916 comparaisons.
L'oracle rationnel complet des neuf sites à `L=10` trouve exactement
trois/six supports q4 admissibles à K5/K10 ; le produit CPU v20 émet
trois/six présentations sur ce nuage dans un rejeu local, sans trace
nominative des supports ni port du DESIGN.
Prévoir un tri/balayage exact pour les gros seaux et mesurer la traîne
sur LiDAR avant toute affirmation de coût S4b.
Le [raffinement octaire scratch](AUDIT_S4B_J8_BIT_FINAL_20260924.md)
égale aussi le multiensemble q4 du produit sur **une trame sans sol
entière**, 158 496 objets à K5 et 1 732 548 à K10 ; ce n'est toujours
ni un port produit ni une chaîne G4. Son ledger omet la recherche linéaire
des candidats dans les groupes de racines égales : une [fixture u18
exacte](s4b_refined_group_fixture_20260924.py) force 1 036 080 tests
d'appartenance pour un groupe de 1 439, contre 45 pas déclarés. Il omet
aussi les rescans de tout le cover pour `Lsize` et chaque seau vivant :
au moins 146,5 M/1,486 Md visites supplémentaires à K5/K10 sur la
trame sondée. Marquer les candidats par position et rassembler les
groupes en un passage retire le carré local ; compter ces rescans et
mesurer la chaîne **S4b** appariée sur le panneau spatial/densité avant de
projeter la pente GPU.
Le [premier port HostGroup S4b WIP](AUDIT_S4B_J8_BIT_FINAL_20260924.md)
avait un défaut d'objet causal : `q4_valid_bit` survivait entre deux
groupes du même seau, et le [gate à cinq sites](s4b_valid_group_gate_20260924.cpp)
répétait le premier support. Le constructeur efface désormais le bit
pour tous les événements dans le passage de positivité ; le même gate
passe en `-O1`/`-O2` sur son source local non publié. Cette correction
ponctuelle reste à compléter par l'égalité nominative de tout le flux.
Le port à tampons évite les rescans de cover du simulateur, mais son
ledger ne compte pas encore la reconstruction de chaque liste de seau
ni les réductions min et de présentation par groupe ; mesurer ces
passes avant de projeter son temps GPU. Le plan de tâches parallèles
doit aussi raccorder ses sorties à la plage unique par arête du batch.
Pour chiffrer S4b sur les 21 entrées physiques/densité, apparier **S4a et
S4b avec le même binaire v21** : le panneau S3/S4a existant est épinglé au
v20. Ses entrées et digests servent de témoins de cohérence, pas de base
causale pour un écart de temps ou de compteurs entre versions.
La [nouvelle porte fichier S4b v21 en WIP](s4b_compare_gate_v21_20260924/README.md)
compare utilement les enregistrements q4 sur les arêtes qu'elle visite,
mais saute les arêtes reportées, ne borne pas le nombre de graines ou
d'émissions et ne compare que taille/empreintes de coquille. Deux points
font rendre `equal=1` avec zéro graine et zéro tétraèdre ; une fixture
tétraédrique à seaux de largeur nulle exerce au contraire une émission.
Publier les exclusions et un plancher positif sur LiDAR, puis juger les IDs
de coquille sur petites fixtures, avant de dire « toutes les arêtes ».

La [matrice de croissance LiDAR](CROISSANCE_LIDAR_PLANS_ET_DENSITE_20260923.md)
répond séparément aux deux variations demandées : trame entière → deux
moitiés → quatre quarts selon les plans capteur, et densités globales
emboîtées 1/4 → 1/2 → 1 dans **chaque** secteur. Sur trois trames sans
sol × K5/K10, le v12 CPU publie 126 cas ; 13/36 pentes spatiales
à densité entière dépassent 2 ; le croisement des trois densités donne
**5/36→10/36→13/36**, soit 28/108 liens spatiaux, et 10/84 liens de
densité des `core_sites` atteignent 2. Les pentes de temps CPU restent
inférieures à 2 sur ces liens. Le brut avec sol couvre les sept secteurs
et trois densités de 08/000000 à K5/K10. Ces pentes finies
isolent un verrou réel de formes calculées : `dead_core_loads` reste
sous 2 sur les 108 liens spatiaux et les 84 liens de densité sans sol,
mais la **taille moyenne du cœur chargé** fait monter leur produit.
Elles ne démontrent aucune borne asymptotique ; le nouveau panneau CPU
S4a sur 08/000200 utilise les côtés float32 physiques et une autre graine.
R15 G4 ne couvre toujours pas ces coupes.
Dans la matrice sans sol v12, les côtés utilisent le signe **quantifié** :
il coïncide avec le float32 physique sur 08/000000 et 08/000100, mais
déplace un retour de 08/000200. La contre-épreuve du quart chaud à K10
selon le signe physique maintient `p_core=2,035350`.

Le [reçu G4 R14](../receipts/g4_tower_r14_20260923/README.md),
[contrelu indépendamment](CONTRELECTURE_G4_R14_RECU_20260923.md), ferme
18/18 cas `complete_relative` sur trois trames **sans sol** de la seule
séquence 08, grille 1 mm, s8/W48. Les 326 empreintes et le lecteur épinglé
passent. Les meilleures chaînes S2+S3 valent **1,563 s à K5** et
**5,488 s à K10** ; aucune tour sous la seconde ni trame brute du contrat
principal n'est acquise. Sur 08/000000, les paires entrelacées attribuent
à S3 **0,221/0,230 s** de gain K5 et **0,794/0,785 s** K10. La
[lecture du budget restant](LECTURE_R14_G4_PLANCHER_CONDITIONNEL_20260923.md)
montre qu'un remplacement gratuit de la seule phase des survivants
laisserait **1,024–1,305 s** à K5 si les autres phases restaient fixes :
S4 et la tour doivent être mesurés ensemble. Le [reçu R13](../receipts/g4_tower_r13_20260923/README.md)
reste historique, pas une ablation des changements simultanés R13→R14.
Le [préflight B](CONTRE_AUDIT_B_PREFLIGHT_G4_R14_V19_20260923.md) et la
contrelecture du reçu confirment que la garde locale annoncée de
**2 Gio libres** n'est pas dans le chemin R14 publié. Les ventilations
GPU réelles ferment à l'arrondi près, mais le lecteur accepte des sommes
artificiellement sous-déclarées ; « kernel/transfert » désigne en outre
des intervalles mêlant calcul, copies et allocations. Le temps total de
chaîne reste exploitable sans cette attribution matérielle fine.

Le constructeur a créé le port S4a q3 sans atlas, nouveau levier de chaîne
et protocole R15, publié sur `main` par `5ceb4d219` puis corrigé par
`3765080cf`. R14 mesure S2+S3 ; R15 mesure le port S4a. Les portes livrées
comparent le port hôte à l'ancien moteur sur des fixtures synthétiques ;
`--file` publie des statistiques, pas un gate LiDAR. Les 146 M tests q3
annoncés après classement en anneaux sont des **tests logiques de scan** :
le warp exécute aussi les voies après le site d'arrêt dans son dernier
ballot. Les nouveaux tampons CUDA retéléversent l'index et reconstruisent
les covers ; la session résidente S4.0 du plan n'est pas encore intégrée.
Le panneau CPU couvre désormais les sept secteurs physiques de
08/000200/K10 à trois densités. Étendre la contre-épreuve à K5, à d'autres
trames et au chemin G4 ; les pentes v12 ne qualifient pas ces ports.

La [contrelecture S4](AUDIT_S4_RESIDENCE_ORDINALS_20260923.md) précise le
raccord du jalon hybride : si q3 s'exécute sur GPU et q4 sur CPU,
la résidence S2/S3 exige encore l'export des arêtes q4 ouvertes avec
leurs ordinaux et masques, plus copie/synchronisation comptées dans le mur.
Pour un éventuel filtre de groupe avant S3, garder l'ordinal S2 immuable,
des masques de voies monotones et un repli CPU sur le dernier masque
certifié. Le plan doit aussi pondérer le second passage q4 par les tailles
de cover et préciser ce que prouve l'empreinte des IDs de coquille.
La [contrelecture B du plan S4](CONTRE_AUDIT_B_PLAN_S4_LEDGER_ET_BUDGET_20260923.md)
ajoute une porte structurelle : une graine q4 produit **deux groupes et
deux émissions** dans une fixture u18 testée directement sur les lanes
v9, donc l'identité projetée « graines = émissions + rejets » est fausse.
Le ledger doit distinguer graines, groupes et enregistrements avant port
GPU. Les champs projetés occupent au moins **129 octets** avant alignement,
non les 112 annoncés ; les budgets d'arène de plages sont des estimations,
pas des bornes. R15 mesure S4a sur G4 ; ces corrections du ledger et le
DESIGN S4b n'y sont pas qualifiés.
Le jumeau CPU/GPU compare deux chemins issus des mêmes survivants : il
détecte une divergence, pas une clé omise par les deux. Conserver le juge
échantillonné de clés admissibles indépendantes en parallèle de la porte
différentielle S4, avec sa portée limitée déclarée.
L'[audit du couplage graines–cover S4a](AUDIT_S4_COUPLAGE_GRAINES_COVER_20260923.md)
montre que les agrégats R14 et LiDAR brut ne calculent pas le volume projeté
`Σ_e ceil(g_e/32)c_e` : ils séparent graines et covers, et le total des
covers mélange les voies. Le WIP donne un warp à chaque arête, traite les
graines successivement et les sites 32 par 32 : son nominal est
`Σ_e g_e ceil(c_e/32)`, son travail effectif dépend de l'arrêt **par
graine**. À `g_e,c_e` identiques, le nominal WIP ne dépasse celui du
plan que de `G=Σg_e` au plus ; pour R14/K5, `G=9,318 M`, mais les
`0,2 G` pas du plan restent une projection non mesurée. Le compteur
`ceil(census_point_tests/32)` par arête est correctement libellé minorant,
pas un nombre de ballots. Publier les couples par arête, les ballots
réels, quatre évaluations d'anneau par site, transferts et replis avant
d'attribuer un coût ou une croissance à S4a.
Le [préflight S4a](AUDIT_S4_WIP_EXCEPTIONS_WORKERS_20260923.md) avait
repéré trois allocations/insertions **hors capture d'exception** dans
`run_lanes_batch_host` du port `5ceb4d219` : un `bad_alloc` d'un worker
pouvait terminer le processus. Le correctif `3765080cf` englobe tout le
worker, réveille les attentes, joint avant relance et évite les slabs pour
zéro arête : ces fenêtres sont closes **à la lecture du source**. Sa porte
tente toutefois de créer environ **512 Gio d'enregistrements par worker**
pour provoquer `bad_alloc` ; sous overcommit, elle peut épuiser la mémoire
au lieu de livrer une exception contrôlée. Une injection bornée aux trois
endroits reste préférable.

`both_edges` est corrigé dans `5ceb4d219`. Le gate de chaîne de
`3765080cf` compare sept champs du ledger entre S3, S4a jugé et S4a à
ardoise réduite. Son exécution locale, 32 cas/code 0, donne
`asked=368886`, `tails=174420`, `both=281530` ; S3 CPU ne reporte aucune
arête, donc **au moins 87 064 occurrences** cumulent q3 reportée et q4
ouverte. Ce chemin est effectivement contrôlé ; `507580243` ajoute
le plancher `tails+both>asked` et compare aussi `q4_emitted` du bras
reporté. Aucune perte de boule n'était déduite du défaut de comptage
initial.

La réception v20 de `3765080cf` vérifie
`q3_edges − lanes_asked ≤ certificats_différés`. Le correctif
**`507580243`** accepte maintenant le report S4a par records ou arène
même si `n<65536` : le contre-exemple exact de 28 679 sites/4 097
arêtes est dans son gate direct. Il applique au jumeau moteur, dans le
worker **et** le contrôleur, les planchers de census q3 de feuille et
du cache ; R15 avait déjà ces comptes positifs. Une fixture équivalente
de 4 400 sites/40 arêtes réduirait fortement le coût de cette porte,
mais reste une proposition mathématique non exécutée. Un
[reçu d'audit CPU S3/S4a](s4a_ground_hot_quarter_20260923/README.md)
couvre maintenant le quart physique chaud de 08/000200/K10 aux trois
densités emboîtées : six sorties appariées vérifiées, une tentative
géométriquement valide écartée des temps pour contention. S4a garde les
mêmes **34,673→153,448→582,997 M** `core_sites` que S3, de pentes
**2,077/1,916** ; ses tests q3 logiques ont des pentes **1,542/1,507**.
Le CPU·s de chaîne S4a donne **1,393/1,406** sur ces deux liens, sans
preuve asymptotique ni transfert de chrono à G4. Le
[reçu local du développeur](../receipts/s4a_q3_lanes_local_20260923/README.md)
sur la trame entière 08/000000 est maintenant publié par `54f6249a4` :
six bras S3/S4a CPU/jugé K5/K10 donnent les mêmes condensés tour et
catalogue, mais aucune coupe ni densité. Ses temps sur hôte partagé
restent descriptifs.

Le [nouvel audit du coût S4a](AUDIT_S4A_VALIDATION_ET_ARENE_20260923.md)
montre que S2/S3/S4a rescannent chacun l'index par nœud et par point
(`26n` appartenances sur un arbre équilibré de `2^25` sites, au plus
`55n` pour le constructeur u18 à milieu géométrique), hors chronos CUDA
internes. Certifier une fois le propriétaire immuable ou calculer les
extrema par induction réduirait ce poste. Le plafond d'arène GPU permet
une traîne CPU **si l'arène allouée déborde**, mais une allocation des
autres buffers peut encore refuser toute la chaîne. Le commentaire a été
rectifié par `8b47a75a9` ; R15 confirme un gain S4a G4 sur trames sans
sol de 35–46 k sites sans report normal, mais ne mesure pas séparément
validation, capacité d'arène et régime massif.

23 septembre 2026. Ports v13 publiés : sonde **`c768e06a`**, porte Euler
8k **`a08378da`**, lecteur LiDAR **`50646eef`** puis **`1f048aae`**,
lecteur G4 **`515b3666`** puis **`1f048aae`**.
La sonde v14 **`67fce4e9`** et le correctif de réception/masse
**`fe1142b5`** sont publiés. Le [reçu G4 R9](../receipts/g4_tower_r9_20260923/README.md),
exécuté avec `fe1142b5`, est désormais versionné par **`76436d44`** ;
les deux empreintes relevées par la [contrelecture B](CONTRE_AUDIT_B_G4_R9_ORDONNANCEMENT_20260923.md)
avant publication sont inchangées. Le même commit publie la sonde v15.
Le [contre-audit B du recouvrement FULL](CONTRE_AUDIT_B_WIP_TOUR_V15_RECOUVREMENT_20260923.md)
et la [contrelecture par ordre](CONTRELEC_V15_CHRONO_ORDRE_20260923.md)
ont trouvé un faux refus K1 et un faux accord K5 dans les lecteurs
successifs. **`33d51efd`** corrige la fenêtre lancement→jointure et la
dépendance K≥2 ; **`c19e4b49`** corrige finalement K1 avec
`max(static, lots_K1)` et un test positif. Le
[reçu G4 R10](../receipts/g4_tower_r10_20260923/README.md) exécute le
paquet `33d51efd` sur **CPU G4** : recouvrement FULL ON/OFF, 24/24 cas
`complete_relative`, meilleure chaîne **2,736 s à K5** et **8,066 s à
K10**, sans sol/grille 1 mm/s8/une seule séquence. Son ancien lecteur
accepte les sorties réelles ; le lecteur corrigé les accepte aussi
([réception A](RECEPTION_G4_R10_20260923.md)). La
[contrelecture B de R10](CONTRE_AUDIT_B_G4_R10_ET_PASSE2_20260923.md)
sépare ce gain des modifications du moteur publiées **après** la capture
par `308ca2a1`. L'égalité exhaustive du payload ON/OFF, un stress TSan
du recouvrement et une nouvelle porte de refus/ledger restent ouverts.
R8/R9 ne mesurent pas le recouvrement ; R10 ne mesure aucun GPU.
La sonde **v16 `f685461a`** prépare les jobs q2 par masse avec
64 jobs/worker (contre 16 auparavant). Sa porte différentielle compare
sorties et travail sur plus de 400 cas, mais avec granularités 1/4/32,
**pas 64** ; le gain q2 local 1,711→1,048 s (un cas K5/W8) n'a pas de
reçu brut. La [contrelecture B](CONTRE_AUDIT_B_Q2_MASS_FIRST_V16_20260923.md)
calcule sur le meilleur R10 que supprimer entièrement q2 laisserait
2,505 s à K5 et 7,651 s à K10 : v16 n'est pas une voie suffisante vers
1 s. Les archives v15 conservent leur
lecteur épinglé ; le protocole **R11 historique** exigeait la sonde v16
et dix leviers, tandis que R13 emploie v18.
Le [reçu G4 R11](../receipts/g4_tower_r11_20260923/README.md),
[contrelu par B](CONTRE_AUDIT_B_G4_R11_ET_VOISINS_COEUR_20260923.md),
mesure ensuite le levier q2 ON/OFF dans le **même paquet v16** :
388/388 empreintes, 24/24 cas `complete_relative`, meilleur K5
**2,537 s** de chaîne et meilleur K10 **7,682 s** ; q2 décroît de
**2,84× à 4,38× en moyenne appariée** selon les six cas. Le reçu
annonce à tort 3,3–4,5× ; les répétitions individuelles donnent
2,82–4,43×. Ce levier change ensemble ordre par masse et grain
16→64 jobs/worker. R10→R11 ne mesure pas q2 seul, car le paquet de
la tour a aussi évolué ; l'ablation interne R11 est la preuve du gain.
Toujours **CPU G4**, s8, sans sol et seule séquence 08 : ni GPU ni
contrat brut multi-séquence ou sous-quadraticité nouvelle.
Le [budget B de la porte filtre GPU](CONTRE_AUDIT_B_PORTE_FILTRE_GPU_20260923.md)
montre que, dans R11, `chain_s−q34_s` vaut encore 0,874/1,062/1,106 s
sur les trois trames K5 et au moins 3,235 s à K10 : déporter **seulement**
les filtres ne peut qualifier la seconde si les autres phases restent
inchangées et séquentielles. La porte de débit 0,1 s pour tous les masques
sans cache demeure utile : la population R11/000000/K5 implique déjà
au moins **253,6 M visites de nœuds** (rectangles plus une racine par
paire), donc **>2,53 Md/s** pour la seule porte S1, transferts inclus ;
le nombre réel de visites de paires sans cache est inconnu. Le port
CUDA `0d5ad2e89`, le bench v2 `1c9c1e5d7` et le protocole G4 avec
garde brute `7565451fc` sont publiés. La
[contrelecture B du paquet](CONTRE_AUDIT_B_G4_S1_PUBLIE_20260923.md)
valide un snapshot strict commité de six cas ; les **10/10 selftests**
normaux et **10/10** sous `-O` passent sur faux GPU. La garde ferme
les contre-exemples A de débordement u18 et de boîte racine forgée.
La [tentative G4 1](../receipts/g4_gpu_s1_attempt1_20260923/README.md)
a échoué à la configuration avant compilation (`CUDA_STANDARD 20`
inconnu de CMake 3.22.1), sans calculer de masque GPU. Le correctif
`6e0e43a0d` passe l'unité CUDA à C++17 et rapporte un build local
avec cet outillage. Le [reçu G4 S1](../receipts/g4_gpu_s1_20260923/README.md),
[contre-audité par B](CONTRE_AUDIT_B_G4_GPU_S1_SESSIONS_20260923.md),
publie ensuite **six passages CUDA exacts** sur les trois trames **sans sol**
de la seule séquence 08, K5/K10 : masques égaux au CPU, visites égales,
préflight positif et mutant causal détecté. Ses 166 fichiers et ses
résumés ont été contre-vérifiés. Le seuil S1 fixé pour 08/000000/K5 est
franchi : **63,8 ms**, transferts aller/retour compris, soit **18,5×** plus
vite que le filtre CPU à 48 fils avec cache. Les autres K5 prennent
43,4/64,2 ms ; les K10 prennent 103,0/70,3/106,8 ms, dont deux au-dessus
de 100 ms. Ce sont les meilleurs temps de trois passes après contexte
et allocations, et excluent garde, index/front, cœur/certificat,
catalogue et FULL.
L'[audit B du coût de garde](CONTRE_AUDIT_B_VALIDATION_INDEX_GPU_S1_20260923.md)
montre `3Σ|plage(v)|` vérifications de rangs **hors** événements GPU ;
l'API brute ne contrôle pas l'unicité XYZ déjà certifiée par le
producteur. Le scan/probe gardent des buffers `O(R+P)` et le noyau
de paires paie `O(P log R)` ; la vitesse S1 est mesurée, mais la mémoire
massive n'est pas qualifiée ; le gain de la chaîne a été mesuré depuis
en R12/R13, avec les limites exposées plus bas. Une certification linéaire
réutilisable de l'index et le tuilage S2 sont les prochaines étapes ;
mesurer ensuite les temps de création et de consommation des requêtes
dans la chaîne. Aucun contrat de tour G4 n'en découle.
La [proposition S2](PROPOSITION_B_GPU_STREAMING_S2_20260923.md)
sépare le tuilage borné sans nouveau rejet (S2a) du certificat
bloc/ligne avant expansion (S2b), avec tests causaux et arrêt de la
**piste expérimentale**, jamais de l'exécution exacte. Le profil local
attribue environ 33 % du CPU
q3/q4 aux deux DFS ciblés avec leur ordre d'enfants ; 40 % comprend
aussi le front non porté. Un chemin GPU intégré doit inclure les coûts
de création/consommation des requêtes et une sortie bornée par lots.
Le [préflight B du raccord batch WIP](CONTRE_AUDIT_B_RACCORD_Q34_BATCH_WIP_20260923.md)
signale plusieurs portes avant de qualifier S2 : adapter de filtre fiable
et porte différentielle causale (les ledgers acceptent même un retour
tout zéro), tuilage borné au lieu de matérialiser tous rectangles et
survivants, et conservation des compteurs de travail du filtre.
Dans ce chemin, `q34_occupancy.cpu_sum_s` n'inclut pas la phase batch de
filtrage ; comparer cette occupation à celle du moteur sous-estimerait
le travail q3/q4 intégré. Le temps CPU du processus garde un périmètre
plus large.
Le préflight portait sur un diff mutable ; ses trois mécanismes restent
visibles dans le port S2 publié par **`a6d81f9ce`**. La réception GPU
de la chaîne sur G4 reste distincte.
La [porte causale A](q34_batch_duplicate_gate_20260923/README.md) exerce
le même point de confiance : à trois sites/K3, remplacer un survivant par
un doublon de même masque conserve cardinalité, masses et compteurs,
`validate_completion` réussit, mais l'unique clé q3 disparaît. Le reçu
Release porte sur le batch CPU du snapshot avant publication, avec mutation
injectée ; aucune erreur spontanée CUDA ni tour FULL n'en est déduite.
Il faut contrôler l'ordinal des paires et qualifier séparément leurs
masques géométriques.
Le garde structurel publié dans `2059189d` ferme ce contre-exemple local :
un [rejeu indépendant](q34_batch_duplicate_gate_20260923/README.md) à trois
sites/K3 refuse le doublon à cardinalité et comptes inchangés, puis la
paire hors rectangle, par les motifs propres au nouveau contrôle.
**`c265a5da` ferme aussi la porte de test** : le mutant intégré remplace
un survivant sans changer cardinalité ni comptes et exige le motif du
parcours structurel ; la paire étrangère exige son motif propre.
La [porte de réception partielle](s2_partial_twin_20260923/README.md)
avait produit sur faux G4 un cas GPU achevé, son jumeau coupé au budget
et un reçu `partial` sans comparaison. **`c265a5da` publie maintenant
`unpaired_batch_cases`** : worker et lecteur recalculent la liste des cas
par lots achevés sans jumeau moteur achevé de même fichier/K/s ; un reçu
qui la masque est refusé. Ces mesures restent brutes et non appariées.
R12, entièrement achevé, n'est pas touché.
Le [préflight B du protocole G4 S2 WIP](CONTRE_AUDIT_B_PROTOCOLE_G4_S2_WIP_20260923.md)
est désormais historique : **`f9e6a5527` ferme le faux marqueur GPU**
en séparant préflight et tours LiDAR GPU achevées, liste recalculée par
l'hôte et mutant mixte. Rejeu B local : 24/24 selftests normaux et
1/1 ciblé sous `-O` ; aucun nouveau reçu G4. La
[contrelecture B de la croissance aval](CONTRE_AUDIT_B_CROISSANCE_Q34_AVAL_S2_20260923.md)
mesure, sur le sans-sol 08/000200 de 16k à 32k, ×4,81 paires développées
et ×8,27 incidences site–cœur à K5, pour ×1,83 émissions q3+q4. Sur la trame
brute 08/000000/K10, le CPU local traite 37,87 M paires et compte
2,329 Md incidences site–cœur+cover, dont 2,304 Md formes chargées hors
extrémités. Le filtre GPU ne supprime pas cette masse aval ;
ces mesures ne prouvent ni une borne sous-quadratique ni le budget FULL.
Le [shadow des rectangles](Q34_BLOCS_LIDAR_SHADOW_20260923.md) donne un
critère d'ordonnancement S2a concret sur 08/000000/s8 : à K5, **1 081 123
des 1 128 166** rectangles ouverts ont moins de 16 paires, mais ne portent
que **8,3 %** des paires ; à K10, **1 962 111 des 2 034 440** en portent
**12,7 %**. Si un bloc GPU traite une tuile par rectangle, ces millions de
petites tuiles risquent de sous-remplir les blocs. Comparer un empaquetage
stable de plusieurs petits rectangles par bloc/warp et le tuilage séparé
des 47 043/72 329 grands rectangles, en conservant chaque ordinal et en
facturant le coût d'empaquetage ; **ce gain d'empaquetage** n'est pas
encore mesuré (le port S2 GPU est, lui, mesuré en R12/R13).
La [mesure complémentaire sur la trame **brute entière**](q34_raw_rectangle_mass_20260923/README.md)
08/000000/1 mm/s8 retrouve exactement cinq comptes du grand-livre v12
pour K5 et K10. Au K10, **198 169 des 4 308 768** rectangles ouverts
portent **74,62 %** des 37,87 M paires développables ; seulement
**2 337** rectangles (0,054 %) en portent **37,05 %**. Le port batch
matérialise avant filtrage **9,88 M** rectangles, soit 226 MiB pour un
seul vecteur de 24 octets. S2a a besoin de deux plafonds indépendants
sur les rectangles et les paires, puis de regrouper les petits produits
et de scinder les rares gros sans perdre les ordinals ni les masques.
Cette sonde CPU ne mesure ni le port S2 courant, ni GPU/G4, ni une
croissance sous-quadratique. À sa date, la matrice spatiale/densité
restait à rejouer avec le raccord exact ; les reçus S2 CPU K5
[quarts](edge_matched_core_20260923/README.md) et
[demis](s2_half_density_k5_20260923/README.md) couvrent depuis les
21 entrées distinctes, avec deux builds épinglés et trois entrées
« plein » communes ; ce n'est pas un reçu unique à binaire commun.
Le [reçu G4 R12](../receipts/g4_tower_r12_20260923/README.md) publie
maintenant le raccord S2 : 14/14 cas achevés, sept filtres GPU réels,
chaînes **2,01–2,69 s à K5** et **6,63–8,70 s à K10** sur trois trames
sans sol de la seule séquence 08. La [contrelecture A](CONTRELECTURE_G4_R12_S2_20260923.md)
vérifie le reçu et précise que ses huit accords comprennent six couples
GPU–moteur distincts, mais comparent des résumés, pas les catalogues clé
par clé. La campagne C compare ceux du lot **CPU** sur 08/000000/K5/K10,
pas ceux du GPU. R12 change simultanément architecture batch et CUDA :
le bras batch CPU `1/0` manque pour attribuer le gain de 15–25 %.
Sur le meilleur K5, survivants et tour coûtent déjà **1,426 s** ; supprimer
entièrement l'appel du filtre laisserait **1,786 s** de chaîne.
Priorités : différentiel catalogue GPU, ablation à trois bras, puis
réduction/accélération exacte des survivants et de la tour ; reprendre
les coupes physiques et densités sur S2 pour étudier la croissance.
La [décomposition par arête](ATTRIBUTION_COEUR_ARETES_COUPES_LIDAR_20260923.md)
a désormais un [reçu S2 CPU apparié](edge_matched_core_20260923/README.md)
sur la trame **brute** 08/000000/K5, plein et quatre quarts physiques aux
densités 1/4, 1/2 et entière. Quinze couples moteur/lot ont mêmes sorties,
charges et formes du cœur ; le second traçage du plein retrouve les mêmes
3 986 433 arêtes et le masque après preuve. Au dernier doublement de
densité, les formes du cœur croissent avec `p=2,119` : à pleine densité,
157 012 arêtes traversant les quarts portent **386,518 M / 559,662 M**
formes, alors que les arêtes communes ne gagnent que 0,317 M formes entre
plein et quarts. Les traversantes de `x=0` concentrent 383,619 M formes,
dont **380,106 M** sur des arêtes ensuite fermées au cœur. Le seuil
intrinsèque exploratoire `|ab|≥4 m` couvre 80,4 % des formes avec 8,5 %
des charges, mais n'est pas un certificat. Essayer un rejet exact avant
matérialisation, avec repli et coût total apparié.
Le [shadow du préfixe](lazy_prefix_dead_core_20260923/README.md) mesure
maintenant les formes réellement consultées aux trois densités du plein brut
K5 : dans l'ordre actuel, `Σ(n−h)` vaut 13,015/51,005/**242,982 M**
formes non consultées, soit 35,17/39,58/**43,42 %** des formes du cœur.
Au plein, 196,983 M de ce suffixe sont sur les arêtes traversant `x=0`,
sans que cet axe soit une hypothèse produit. Les trois sorties discrètes
coïncident avec le batch antérieur ; chaque arête recoupe la trace S2.
Le cover reste intact.
La pente finie de `Σh` au dernier doublement est encore **2,024** : ce
shadow borne un potentiel de fabrication évitable, sans mesurer un gain
CPU, sans conclure à une chaîne sous-quadratique et sans remplacer le
certificat exact avant cœur.
La [contrelecture B du préfixe](CONTRE_AUDIT_B_CROISSANCE_Q34_AVAL_S2_20260923.md)
confirme ses sommes mais relève que l'analyseur publie des champs
`post_closed_*` et `loads_full_prefix` faux dans ses seuls sous-groupes
`intersections` ; ne pas les utiliser. Le reçu compact n'embarque pas
les grosses traces nécessaires à la jointure autonome.
Le [reçu local du développeur](../receipts/q34_survivor_phases_20260923/README.md),
[contrelu par B](CONTRE_AUDIT_B_VENTILATION_SURVIVANTS_20260923.md),
ventile les survivants sur une trame **sans sol** 08/000000 : cœur et
cover occupent environ 35 % des **ticks TSC écoulés** à K5, 28 % à K10,
contre 65/72 % pour atlas et voies. La
[contrelecture A des compteurs](CONTRELECTURE_CYCLES_SURVIVANTS_Q34_20260923.md)
montre que `rest_cyc` recouvre le cœur des arêtes ouvertes ; les neuf
postes de l'autre patch ne se recouvrent pas dans l'instrumentation mais
suivent surtout le temps écoulé des fils désordonnancés, pas leur CPU
actif. Son « 1,3 % maximal » pour les formes paresseuses mélange fraction
de sites et temps de deux runs : c'est une projection, pas une borne ni
un gain G4. Même retirer idéalement **toute** la phase
des arêtes survivantes des trois lignes K5 de R12 laisserait 1,177 à
1,513 s de chaîne, à autres phases inchangées : S3 seul ne suffit pas.
Le [préflight B de S3](CONTRE_AUDIT_B_S3_CERTIFICAT_WIP_20260923.md)
et le [gate causal de frontière](s3_frontier_barrier_gate_20260923/README.md)
ont trouvé un réemploi WAW/WAR du frontier dans l'ancien noyau. Les
correctifs publiés `545c71799` (barrière), `942494362` (gardes d'entrée
et juge CPU par arête dans les deux préflights G4), `46c50432c`
(compte exact des covers reconstruits et classement GPU observé) et
`18d7c69c7` (six épingles de condensé de catalogue et mutants de
coquille) ferment ces défauts et préparent R13. Le lot CUDA **vide à
pointeurs nuls** reste sans essai device ciblé ; la porte hôte ne
simule pas les délais mémoire CUDA. Les preuves CPU et le reçu R13
ci-dessous comparent le condensé FNV-64 et des comptes publiés, sans
égalité littérale indépendante des catalogues GPU/CPU. Les commandes,
refus et portées intermédiaires restent dans la contrelecture liée.
L'[addendum de flux S2/S3](PROPOSITION_B_GPU_STREAMING_S2_20260923.md)
épingle les six populations R/P/S de R12 et les plafonds `2^31−1` du
port à appel entier. Sans tuilage exact, cette représentation n'a pas
de voie vers plusieurs dizaines de millions de sites, indépendamment
de la question mathématique de croissance globale.
Le [reçu R13 S3 sur G4](../receipts/g4_tower_r13_20260923/README.md)
est publié après ces préflights.
La [contrelecture B](CONTRE_AUDIT_B_G4_R13_S3_20260923.md) vérifie les
326 empreintes et rejoue le lecteur normal/`-O` : 18/18 cas achevés,
six épingles CPU tour/catalogue et 12 comparaisons égales. Le préflight
à 1 500 sites juge 55 523 décisions GPU, dont 3 404 reportées dans
la variante à ardoise 64 ; le juge par arête est désactivé sur LiDAR.
Sur trois trames **sans sol de la seule séquence 08**, s8/W48, la chaîne
S2+S3 GPU est à **1,738–2,347 s à K5** et **5,914–7,967 s à K10**.
L'ablation interne R13 de 08/000000 donne un gain net S3 de 138 ms à
K5 et 583 ms à K10, mais 611–750 k covers sont encore reconstruits
côté CPU à K5, 1,258–1,555 M à K10, puis la tour prend 0,584–0,784 s
et 2,260–3,039 s respectivement. Le `certificate_device_ms` publié
encadre allocations, initialisations, transferts et noyau, contrairement
au libellé « coût du noyau » du reçu ; le noyau seul n'est pas chronométré.
Une seule mesure par cas, aucun p95 ; aucun contrat brut multi-séquence
ni sous-quadraticité globale ne sont qualifiés. Le G4 SPOT a été arrêté
et relu `TERMINATED`.
La [contrelecture physique de B](CONTRE_AUDIT_B_G4_R13_S3_20260923.md)
précise que le GPU a déjà construit le cover complet des arêtes
`decided && mask!=0`, mais n'en livre pas les plages : le CPU le
reparcourt 0,611–1,555 M fois selon le cas. Ces visites et tests du
**second parcours** sont absents du ledger logique, dont l'égalité
CPU/GPU ne vaut donc pas égalité du travail physique. Le coût isolé du
rebuild et la distribution des plages ouvertes ne sont pas publiés ;
`edges_ms` englobe aussi tout l'aval q3/q4. La borne structurelle du
payload compact est large (4,89–675,15 Mo selon le cas) : mesurer
temps/visites du rebuild et plages ouvertes avant tout port de leur
export GPU→CPU. Même effacer fictivement tout `edges_ms` laisse
1,196–1,599 s à K5 dans R13 : cette optimisation seule ne ferme pas
le contrat sur la chaîne inchangée.
Le [correctif d'interprétation](../receipts/g4_tower_r13_20260923/ADDENDUM_20260923.md)
du développeur suit R13. La [mise à jour D5](CONTRE_AUDIT_B_D5_FULL_MAIGRE_20260923.md)
rappelle que phase statique et lots de la tour se recouvrent déjà :
leurs durées ne s'additionnent pas en temps mur. La vue exacte
`E×C` proposée par B, avec gardes conservés et repli par cellule,
reste à mesurer ; le premier crible `E_C` conservateur est mesuré plus bas.
La lecture initiale d'un **diff WIP** de FULL dans
[cette même note](CONTRE_AUDIT_B_D5_FULL_MAIGRE_20260923.md)
ne trouvait pas de contradiction logique à l'indexation des niveaux
exacts par runs. Le code est depuis **publié et restauré**
(`96a053805`, `293aa6d7b`) avec un
[reçu local](../receipts/tower_phaseA_lean_local_20260923/README.md).
La [contrelecture B](CONTRE_AUDIT_B_PHASE_A_ALLEGEE_20260923.md)
confirme les cinq paires de sorties et le gain local de phase A
(médiane K5 `784→534 ms`, K10 deux paires `3679/3119→2134/1965 ms`),
mais aucun gain de tour K5 stable, G4 ou brut. Le reçu n'épingle pas
SHA complets de binaires, entrée brute, commandes et log de ses 152
portes. `level_run` retient 4 octets/boule jusque dans `finish()` ;
mesurer RSS et les quatre arrondis avant d'étendre sa qualification.
L'ablation interne S2 seul→S2+S3 n'a qu'un passage : les témoins
moteur de ses deux bras dérivent aussi de **3,266 à 3,841 s** à K5
et de **10,498 à 11,023 s** à K10. Les postes internes sont
mesurés, mais des paires répétées et entrelacées sont nécessaires
pour attribuer un gain stable de chaîne à S3 seul.
Le commit produit **`0b41e4c86`**, postérieur à R13, remplace les
compteurs d'une arête S3 par des `u32` privés et déplace les totaux du
warp en mémoire partagée, écrits par son leader. Les champs `u32`
**par arête** ne débordent pas sous les gardes actuelles :
visites/rangs sont bornés par l'index, formes/sites par l'ardoise,
cellules par 5 461 et `failed_cells` par 10 922 ; les deux grands
produits cellules×sites restent `u64`. En revanche, la
[borne de B](CONTRE_AUDIT_B_G4_R13_S3_20260923.md) montre que les
**sommes `u64` globales** requièrent un contrôle pour une capacité
utilisateur proche de `2^32` ; la capacité par défaut de 65 536
reste dans leur domaine borné si `edges≤2^31−1`. Pour le contrat massif,
des lots bornés et une accumulation hôte exacte `u128` seraient une
voie ; un simple refus préserverait l'exactitude sans servir ce contrat.
La synchronisation du warp avant réemploi est conservée. `ptxas`
annonce 128 registres et 16 warps résidents par SM, contre 248/8 pour
R13 ; **aucun chrono G4 du nouveau code** ne valide encore un gain.
Le prochain essai doit alterner ancien/nouveau sur les mêmes entrées,
juger masques et compteurs et publier le temps du noyau S3 séparé de
l'intervalle upload+noyau+download.
La [preuve B de redondance d'une seule cellule de
centres](CERTIFICAT_B_REDONDANCE_CELLULE_UNIQUE_20260923.md) affine la
piste de rejet **avant** le cœur : si la cellule couvre le disque
nominal d'une arête encore ouverte, chaque garde qui la domine aurait
déjà été crédité par S2. Segmenter les arêtes sans **subdiviser les
centres** ne crée alors aucun rejet neuf. Une exception exacte vient
du clipping par la boîte réelle du nuage, bien plus petite que le cube
u18 en `z` sur 08/000000 ; l'autre solution couvre les centres par
2/4/8 cellules à gardes distincts. Un test entier O(S) identifie
d'abord les disques intérieurs qui rendent la cellule unique vaine.
Le [recomptage exact des traces S2 brutes 08/000000/K5](bbox_clipping_s2_20260923/README.md)
trouve seulement **57 677 / 3 986 433 arêtes (1,447 %) et
8 363 262 / 559 661 741 incidences cœur (1,494 %)** avec un disque
actif débordant de la boîte réelle du nuage. La cellule unique
globalement clippée ne peut donc concerner que cette faible masse sur
ce cas ; ce n'est ni un gain mesuré ni une conclusion pour K10/sans-sol.
Sur les sous-échantillons emboîtés 1/4 et 1/2 de cette même trame/K5,
ce plafond d'incidences est 2,823 % et 2,319 % : baisse finie, sans
borne de croissance ni transfert aux coupes spatiales.
Le [complément apparié par arête](precore_cell_screen_20260923/README.md)
retrouve les comptes q3/q4 de B sur une autre trace et resserre le
plafond du **chargement entier** : toutes les voies ouvertes doivent
déborder, soit **46 340 arêtes / 7 528 704 formes (1,345 %)** au plein.
Cette part vaut 2,429 % puis 2,050 % aux densités 1/4 et 1/2 ; dans
les quatre quarts spatiaux à densité entière, elle varie de 3,563 % à
7,568 % de leurs calculs séparés. Une borne exacte qui garde la
corrélation des deux extrémités des arêtes survivantes est démontrée
sur une fixture q4 positive pour les **sous-cellules**, mais son coût
`|E|×sommets` et son rendement LiDAR demandent la mesure ci-dessous.
La [proposition B de crédit par nœuds](CERTIFICAT_B_NOEUDS_CORRELES_AVANT_COEUR_20260923.md)
étend cette borne : si le **plus lointain point** d'une boîte d'index
satisfait l'inégalité stricte aux sommets d'une sous-cellule, toute
sa population distincte peut servir de gardes, sans énumérer ses sites.
Huit cellules AABB partagent au plus 27 sommets ; le budget de visites
porte sur la **preuve**, jamais sur les candidats. C'est un lemme
exact mais pas encore un gain LiDAR ou une solution à `S` développé.
Le premier shadow limite ses recherches et conserve le repli moteur ;
les autres régimes restent à mesurer.
La [jointure S2 par segment](s2_segment_mass_20260923/README.md) a
maintenant mesuré cette distribution sur la trame brute entière
08/000000/K5 : **11 174 segments d'au moins 16 survivantes**, soit
0,44 % des 2 548 453 rectangles ouverts, portent **478 635 662 / 559 661 741
formes (85,52 %)** avec seulement 396 481 / 3 986 433 arêtes S2.
La longueur du segment est connue **après le filtre S2 mais avant le cœur** ;
elle offre un sélecteur déterministe de shadow et sans axe LiDAR. Le produit de
rectangle ≥1 024 cible encore 71,00 % des formes, mais laisse 22 grands
rectangles ≥32 768 sans aucune survivante : le produit seul ne suffit pas.
Pour les segments ≥16, huit sous-cellules à 27 sommets demanderaient
`27×396 481 = 10 704 987` termes corrélés, soit 2,24 % de leurs formes
actuelles **avant** séparation q3/q4, gardes, repli et couverture. Ce rapport
arithmétique n'est ni un rejet ni un gain mesuré. La jointure recrée le
front et le filtre avec une archive distincte du binaire tracé ; elle
retrouve exactement chaque arête, son masque et les cinq comptes du front.
Le sélecteur est essayé ci-dessous sur le plein brut dense et un quart
clairsemé ; restent le sans-sol, le coût de couverture et la chaîne.
Le [panel de segments S2 sur les quatre quarts et trois densités](s2_segment_panel_20260923/README.md)
recoupe **15/15** cas avec les traces et les cinq comptes front/filtre.
Sur le plein brut, la part de F portée par les segments ≥16 monte de
**67,64 % à 76,52 %, puis 85,52 %** ; leur masse elle-même croît avec des
pentes finies **1,978 puis 2,279**. Mais le quart `x≥0,y≥0` ne couvre
que **7,20→17,66→35,50 %** de F par ce critère, et son budget
optimiste `27S16/F16` vaut **39,41→28,02→19,73 %**, avant gardes.
Sur le plein dense, les voies séparées q3/q4 relèvent le budget de
2,24 % à **3,69 %**. Le seuil 16 est donc une cible d'ablation, pas
un réglage universel ; mesurer un déclencheur adaptatif et toujours
laisser un repli exact. Les moitiés ont leur panel S2 de formes, pas
encore de distribution des segments.
Le [shadow exact par nœuds avant cœur](s2_precore_node_shadow_20260923/README.md)
teste alors huit cellules 3D fermées sur les segments S2 d'au moins
16 arêtes, avec `Q_E` entier et **64 ou 256 visites d'index par cellule**.
Sur le plein brut 08/000000/K5, ces tentatives ferment 36 puis
103 segments, correspondant à **458 001 (0,0818 %) puis 753 058
(0,1345 %) des 559 661 741 formes** du cœur potentiellement évitables,
pour 10,705 M termes corrélés et **4,595 puis 15,309 M visites**.
Le quart `x≥0,y≥0` à densité 1/4 ne ferme rien à budget64 ; même à
4 096 visites, une seule fermeture épargne 848 formes. Les
**39 arêtes** que le prouveur du cœur laisse ouvertes malgré la preuve
shadow ont huit cellules × quatre gardes chacune vérifiées par entiers ;
la voie exacte n'émet aucun candidat q3/q4 sur elles. Le reçu est
reproductible et les 17 sommes SHA passent, mais ces formes ne sont
**pas** un gain CPU/G4 ni un résultat FULL. Cette grille commune ne
justifie pas un port tel quel. L'[ablation `E_C` par cellule](s2_precell_incidence_20260923/README.md)
sur les mêmes deux sous-nuages teste le premier raffinement : un crible
entier boîte/projection/plan, sûr aux contacts, conserve encore
**86,32 % / 86,26 %** des incidences arête–cellule. À budget64 sur le
plein, il ferme 1 393 arêtes et rend **600 315 F** fermables
(0,1073 % du total), contre 974 arêtes/458 001 F pour `E` commun :
le gain n'est que **0,0254 % de F total** ; le quart reste à zéro.
Les 21,9 M termes `Q` du sidecar naïf sont mémoïsables, mais la faible
fermeture interdit d'en faire le prochain port produit. Tester plutôt
des domaines 2D ou adaptatifs et le seuil q4 seul `K−2`, avec budget,
repli exact et coût de chaîne mesuré.
Un [certificat complémentaire sans cellule](paired_guards_precore_20260923/README.md)
apparie des **sites distincts** : si la somme de leurs deux marges
d'intériorité est strictement positive sur tout le disque de centres
q3 ou q4, au moins un site de chaque paire est intérieur. Les tests
entiers sont `H>0`, `3H²>4X` (q3) ou `H²>2X` (q4), avec quatre/trois
paires disjointes à K5. Une fixture native laisse S2 ouvert sur les deux
voies, puis ferme exactement ces voies par quatre paires. Sur deux
échantillons stratifiés de **60 arêtes S2 lourdes** d'une même trame
brute, une palette de 16 sites au plus par quadrant ferme **27/60 puis
40/60** arêtes et couvre **111 883/260 032 puis 168 845/260 599**
formes de cœur échantillonnées. La palette oracle balaie toutefois
**7,403 M sites par échantillon** ; ces rapports ne décrivent ni le flux
complet ni un gain de temps. Prochaine épreuve : recherche bornée par
index, visites et coût de chaîne sur plein/moitiés/quarts à densités
appariées, avec repli exact pour tout échec.
La [contrelecture B](CONTRE_AUDIT_B_PAIRES_GARDES_PRECOEUR_20260923.md)
rejoue les SHA, les 299/383 preuves strictes et les 120 valeurs `F`.
Les deux tirages n'ont aucune arête commune, mais pas le même mélange
de voies : à palette16, q4 seul ferme 17/29 puis 30/38, tandis que
q3+q4 ferme 10/31 puis 7/19. Le 27→40/60 ne mesure donc pas une
amélioration ni un taux du flux. Le balayage oracle lit **×28,47** plus
de sites que les formes du cœur des 60 arêtes de la première graine ;
et l'échantillon « lourd » est choisi par `F` connu **après** le cœur.
Un chemin vraiment pré-cœur doit trouver un déclencheur bon marché et
ses gardes par index, sinon il déplace le coût vers `Ω(nE)`.
Le [shadow de palettes indexées](paired_guard_index_shadow_20260923/README.md)
retrouve, site par site, les palettes oracle des 120 arêtes sélectionnées
**après** connaissance de `F`. À 16 sites/quadrant, il ferme 67/120 arêtes
et 280 728/520 631 formes conditionnelles, pour 48 553 nœuds dépilés,
83 768 boîtes et 6 909 sites testés ; les 53 échecs paient aussi leur
recherche. Un [shadow par paires de nœuds](paired_guard_node_blocks_20260923/README.md)
certifie exactement chaque produit de blocs disjoints : à budget 64
visites/quadrant et cap4, 32/120 fermetures contre 21 pour les points,
avec 14 624 contre 11 751 tests de paires ; à budget 256, 67 fermetures
des deux côtés et seulement +1,85 % de `F` fermable net pour les blocs.
Les 758 certificats enregistrés passent le lecteur exact et LIVE. Le
déclencheur pré-cœur `|ab|²≥2²²` conserve les 67 fermables du panel mais
appellerait **909 278/3 986 433** survivantes S2 sur la trame brute :
aucun gain produit, FULL ou G4 n'est démontré. Chercher un routage moins
cher et une mutualisation par groupe, puis mesurer succès **et replis**
sur tout le flux avant tout port.
La [contrelecture du coût physique](CONTRE_AUDIT_B_COUT_NOEUDS_PAIRES_20260923.md)
montre que le shadow par nœuds recalcule les bornes après chaque
extraction de la file sans les compter dans `boxes` : cap4/budget64
paye **88 108** calculs contre 57 794 affichés (Top-B : 58 444),
et cap4/budget256 **129 753** contre 82 160 affichés (Top-B :
83 768). Les 758 preuves positives demeurent exactes ; la comparaison
de coût des « boîtes » doit être corrigée avant de juger un port.
Le [certificat B de rectangle entier](CERTIFICAT_B_PAIRES_GARDES_RECTANGLE_20260923.md)
relève la paire ponctuelle : les **64 couples de coins** de `box(A)×box(B)`
certifient la même paire de gardes pour tous les sites du rectangle,
par convexité d'un cône et affinité séparée des deux extrémités.
Quatre/trois paires disjointes ferment les voies K5. Des nœuds
témoins `U,V` peuvent fournir plusieurs paires par une borne uniforme
ou **4096** quadruplets de coins, jamais par un seul représentant.
Le déclencheur segment S2≥16 cible ici 85,52 % des formes du cœur
sur une seule trame brute, mais survient **après l'expansion S2** ;
un essai au front demande un déclencheur pré-S2 différent. À palette
64, l'essai exhaustif de paires et coins pourrait coûter **1,442 Md**
de tests sur ces segments : sélection bornée, crible et arrêt précoce
sont des conditions de viabilité, non des optimisations facultatives.
Le [shadow B sur les rectangles réels](rect_pair_shadow_b_20260923/README.md)
mesure maintenant **tous les 1 747** rectangles ouverts de produit
`|A||B|≥1 024` de cette trame brute K5/s8 : seuls 299 ont des
survivantes S2. Avec 32 gardes proposés au maximum et appariement
exact, **72** de ces 299 segments ferment complètement, soit
**2 175 arêtes et 5,060 M / 559,662 M formes du cœur global
(0,904 %)** potentiellement évitables. Les **979** autres fermetures
portent des segments déjà vides après S2 : elles pourraient réduire
S2 mais non le cœur. Le crible paie **26,845 M** couples de coins
sur hôte CPU partagé, sans gain de chaîne mesuré. Le glouton manque
trois fermetures positives ; l'amélioration de matching seule ne
renverse pas la sélectivité. C'est un résultat négatif pour **cette
palette uniforme et ce budget**, pas pour la preuve ni pour des
rectangles subdivisés ou des tuiles d'arêtes. Ni borne de croissance
ni vitesse G4 n'est mesurée.
Une [généralisation B à des groupes de r gardes](CERTIFICAT_B_GROUPES_GARDES_Q34_20260923.md)
conserve les **mêmes inégalités entières** et la preuve par 64 coins ;
des groupes disjoints donnent autant de sites intérieurs. Une fixture
u18 K5 a **quatre triples** qui ferment q3 et trois q4, tandis que
**les 66 paires** de gardes échouent toutes. Une seconde fixture teste
64 coins distincts et 120 paires : le [contrôle entier](check_group_guards.py)
passe. Le [premier shadow LiDAR des triples](rect_guard_triples_b_20260923/README.md)
mesure une proposition bornée sur ces **1 747** grands rectangles : après
la preuve par paires, un mix glouton de triples ferme seulement **deux
rectangles positifs supplémentaires**, 44 arêtes et **53 523 formes**,
soit **0,00956 %** du cœur global. Sur les 696 rectangles restants, la
sélection des triples paie encore **2,547 M énumérations**, 2,712 M
coins et environ **356,5 ms CPU locaux** ; ni gain de chaîne ni GPU
n'en découlent. Le certificat garde son pouvoir mathématique, mais cette
palette uniforme/top64 ne justifie pas un port ; tester des tuiles ou
groupes adaptatifs avec gardes/IDs archivés et repli exact.
Le [shadow des paires pondérées](weighted_guard_pairs_20260923/README.md)
donne un autre levier exact : pour une paire fixée, des poids rationnels
positifs `λ:μ` conservent le certificat par 64 coins si **le même
rapport** vaut sur tout le rectangle. Une fixture ferme q3/q4 au rapport
2:3, mais échoue à 1:1 ; la fixture des quatre triples reste impossible
à fermer par **toute** paire pondérée, même avec poids libres. Sur la
même trame brute K5 et la palette B, quatre rapports bornés ajoutent
**15 rectangles S2-positifs**, soit 1 712 arêtes et **5,634 M formes**
potentiellement évitables (1,007 % du cœur global). Les 15 preuves
archivées passent 6 720 tests entiers de coins, avec gardes disjoints
par voie. Mais tous les 1 747 grands rectangles paient déjà 2,243 M
rapports au représentant et 58,086 M coins supplémentaires ; même un
déclenchement après échec 1:1 laisserait 1,089 M rapports et 23,060 M
coins sur 696 replis. Aucun gain de chaîne n'est mesuré. L'étape utile
est une sélection de rapport dans l'intersection exacte des intervalles
des 64 coins, puis une porte conditionnelle dont le coût complet est
comparé aux formes et à l'aval réellement épargnés.
Le [diagnostic de routage pré-cœur](paired_guard_dispatch_grid_20260923/README.md)
lit les **3 986 433** survivantes S2 du plein brut 08/000000/K5.
`D≥2²³` et une cellule du milieu de 4,096 m occupée par au moins
1 024 sites routent **147 406** arêtes (3,70 %) ; parmi les gros cœurs
`F≥1 000`, ce sont 85 023/91 267 arêtes et **421,520/429,564 M**
formes, connues seulement après le cœur. Un groupe cellule/axe contient
67 827 arêtes, dont 66 869 grosses portant **367,510 M** formes ;
ses boîtes d'extrémités larges motivent une subdivision à mesurer.
Le préfiltre exact sur boîtes par **16 coins/axe pour Hmin** et **64
coins/composante pour Xmax** est vérifié sur 30 fixtures ; il peut
précéder les tests corrélés de B. Cellule/axe n'est pas un certificat :
subdiviser les couples d'extrémités, prouver la disjonction des sites,
facturer le groupage et les replis. Ce seul plein brut ne prédit pas
la sélectivité des demi-scènes, quarts ou densités.
Le [BVH de couples d'arêtes](paired_guard_group_bvh_20260923/README.md)
teste exactement cette subdivision sur le **plus grand groupe en nombre
d'arêtes**, critère disponible après S2 mais avant le cœur : 67 827
arêtes contre 9 548 dans le second groupe. Seize représentants choisis
par extrémités proposent 51 paires avec **1,974 M visites de sites** ;
à feuilles de 64, 187 790 tests uniformes et 4 107 matchings ferment
intégralement **24 537 arêtes**, dont `F=153,838 M` formes qui auraient
pu être évitées avant `load`. Les 43 290 autres arêtes gardent le chemin
exact ; les preuves positives sont recoupées point par point en Release
et sous ASan/UBSan. Le groupe n'est qu'un cas favorable d'une trame
brute/K5/s8, avec coût de dispatch, BVH, sélection, replis et aval non
apparié au moteur. Ce n'est ni un gain net ni une borne sous-quadratique.
Les [demi-scènes et quarts aux trois densités](lidar_raw_physical_scaling_20260923/README.md)
ont été mesurés avec v12, puis appariés au batch S2 CPU K5 par les
deux reçus ci-dessus. La somme de leurs tours ne reconstruit pas le
plein. Une scène/K5/CPU ne prouve ni sous-quadraticité, ni contrat G4.

Les deux meilleures lignes R11 (08/000100) bornent aussi le gain du
seul réordonnancement q3/q4 : les **48 workers logiques** consomment
78,151 CPU·s en 1,661 s de mur maximal à K5 et 211,744 CPU·s en
4,444 s à K10. À travail CPU inchangé et occupation idéale des 48 CPU
logiques (24 cœurs physiques avec SMT), les planchers sont
**1,628 s et 4,411 s** : environ **33 ms** de marge de scheduling
sur la portion workers dans chaque cas. Cette borne conditionnelle
n'inclut pas une réduction du travail ni un changement de coût par
opération ; elle oriente la suite vers moins de paires, formes et
sorties intermédiaires, avec coût aval complet.
La première **matrice complète** de densité vient du binaire v12
**`4530644b`** ; ses 21 entrées K5 ont depuis un appariement S2 CPU
dans les reçus des quarts et des demis ci-dessus. Un
[rejeu S2/v17 apparié](q34_batch_density_quarter_20260923/README.md)
sur le quart brut `x≥0,y<0` de 08/000000 à K5 et aux mêmes trois densités
trouve des tours/catalogues identiques moteur–batch CPU. Les formes cœur
restent 3,549→14,655→52,302 M (`p=2,002/1,823`) ; au plein du quart,
le batch sans cache passe de 97,76 à 162,48 M visites de paires témoins,
de 44,225 à 46,253 CPU·s et de 585 328 à 597 424 KiB RSS. Ce reçu
isolé ne qualifiait pas la croissance globale de S2 et ne qualifie
toujours pas CUDA/G4. Le runner LiDAR v17 constructeur force les leviers
batch/GPU
à `false` et n'échantillonne que des disques 8k/16k/32k ; il ne rejoue
pas cette matrice. Le reçu G4
[R8](../receipts/g4_tower_r8_20260923/README.md) exécute
`515b3666` sur CPU G4 et a sa
[contrelecture indépendante](CONTRE_AUDIT_B_G4_R8_20260923.md). Le
[reçu G4 R7b](../receipts/g4_tower_r7b_20260923/README.md) exécute
le paquet **`8e8b83a3`**, antérieur au Welzl move-to-front, aux
séparateurs pseudo-aléatoires du tri FULL et à la libération précoce de
l'index de clés. Le [reçu R6](../receipts/g4_tower_r6_20260923/README.md)
exécute `78ce9fd4` ; comparer ses temps avec R7b exige d'ajouter le
digest séparé au périmètre mural ancien. Cadre :
`exploration_v9_hors_registre`, `reference_cpu`,
`quantized_u18_input_only`, **`not_claimed`**. Ce fichier porte le
verdict mutable ; les notes et reçus gardent les preuves.

La [première tentative R7](../receipts/g4_tower_r7_stockout_20260923/README.md)
a subi un `STOCKOUT` avant démarrage, sans chrono. La rupture de schéma
de la sonde v10/plan v5 est [close](PROTOCOLE_TOUR_V10_V5_RUPTURE_20260923.md)
depuis `f55ea40c`. Le lecteur `8e8b83a3` impose l'identité MEB exacte
`proposals=verified_proposals+proposal_fallbacks` trouvée par la
[contrelecture](MEB_PROPOSITION_EXACTE_20260923.md) : **21/21** selftests
normal et `-O`, et **31/31** mutants tués dans chacune des deux portes
réelles. Le WIP v6 C6/tri est [contrelu à part](CONTRE_AUDIT_B_WIP_V6_C6_TRI_20260923.md)
et ne qualifie pas v9.

## Contrat et objet effectivement construit

Le jalon v9 vise toute la tour HGP **K=1..10 en moins de 1 s sur GCP G4**,
avec repli K=1..5, puis 100 ms. Le sans-sol LiDAR u18/grille 1 mm est le
premier régime de travail. Décision utilisateur ultérieure : **la grille
1 mm est le profil de qualification prioritaire de v9 ; le float32 exact
est secondaire**. Ce choix de précision ne remplace pas le contrat
principal sur **trames brutes entières** par le sous-nuage sans sol ;
aucune hypothèse d'alignement entre passages LiDAR n'est admise. Les trois
trames 08/000000, 000100 et 000200 sont d'une seule séquence, pas une
qualification multi-séquence. Les coupes capteur servent au diagnostic de
croissance, jamais à remplacer une trame entière.

Pour ce diagnostic LiDAR, les **deux moitiés** sont séparées par le plan
capteur `x=0` et les **quatre quarts** par `x=0` et `y=0` : deux plans
verticaux contenant le capteur, sans sous-échantillonnage ; les points sur
un plan vont au côté non négatif. Il faut publier ces découpes pour la
trame brute entière aussi bien que pour le régime sans sol.
Les sept fichiers chronométrés v9 découpent la **géométrie sur grille 1 mm** : leurs
IDs sont disjoints et reforment exactement chaque trame sans sol. Une
coordonnée float32 brute de 08/000200, négative à 0,09 mm du plan, devient
`x=0` sur grille et change de moitié ; la
[contrelecture des découpes](DECOUPES_CAPTEUR_LIDAR_BRUT_ET_GRILLE_20260923.md)
sépare donc explicitement coupe brute et coupe quantifiée. Les six
préparations float32 v8 des trames brutes et sans sol contiennent déjà des
coupes par les signes originaux, vérifiées par IDs, coordonnées et hashes ;
elles ne sont pas des chronos de tour v9. Les disques
emboîtés 8k/16k/32k du runner de pente sont un **autre** diagnostic ; ils
ne remplacent ni ces découpes par plans ni la trame entière.

La chaîne produit des candidats de miniballes k-Gabriel locales, recoupe
leurs `BallKey`, recense exactement chaque clé **émise**, puis construit
la tour FULL relative à ce catalogue. Elle vérifie le support minimal
`q_min` des coquilles étendues et refuse sans troncature une coquille de
plus de 12 sites. Les petits oracles T2 comparent inventaire de boules et
tour Γ publique, y compris W1/W4 ; ils ne prouvent pas qu'aucune clé
entièrement omise ne manque à une grande trame. `complete_relative`
signifie précisément cela, et un lecteur de contrat doit exiger
`run_tower=true` et les ordres K=1..Kmax. Voir les [contrelectures du
moteur](CONTRE_AUDIT_A_MATH_MOTEUR_20260922.md), de
[FULL](CONTRE_AUDIT_B_FULL_COUTS_ET_INTERFACES_20260922.md) et de la
[porte T2](CONTRE_AUDIT_B_PORTE_PUBLIQUE_T2_20260922.md).

La [note d'Euler par ordre](NOTE_C_INVARIANT_EULER_20260923.md),
[démontrée aussi par le nerf sans position générale](CONTRELEC_EULER_PAR_NERF_20260923.md),
donne un contrôle global **nécessaire** du catalogue : `E_K=1` pour
`K≤Kmax−2`. Les 18 coupes LiDAR 8k/16k/32k publiées le satisfont et neuf
mutants d'omission échappant à la chaîne le violent à K5. La formule ne
certifie pas les clés une à une : à K5 elle ne juge que K1..3, et le
contrat K10 exigerait un générateur K12 pour juger ses dix ordres. Le
protocole Kmax+2 ajoute des témoins, mais **Euler et la restriction des
clés seuls** peuvent manquer une omission commune aux deux exécutions :
le [contre-exemple exact de B](CONTRE_EXEMPLE_EULER_KPLUS2_20260923.md)
construit deux clés omises à 13 sites qui laissent `E_K=1` et la
restriction K7→K5 inchangées ; son extension à 23 sites fait de même
pour K12→K10. La [sonde de C](c_omission_20260923/README.md) montre
que **FULL refuse les omissions de la fixture 13 sites**. À 8k/K5–K7,
elle refuse 515/515 retraits isolés de clés avec `p+u≤Kmax`, mais
seulement 2/280 retraits de la classe régulière q2/q3 de fin de fenêtre
le sont par connexité finale. Ces clés étaient **déjà émises** et le
tirage est déterministe ; la campagne ne cherche aucune clé jamais
produite. Le seuil n'est pas un « si et seulement si » de détection.

La [contrelecture B](CONTRE_AUDIT_B_OMISSIONS_ET_PORTEE_REVISION_C_20260923.md)
montre que 9/12 retraits de couche haute acceptés sur de petites
fixtures changent pourtant le digest FULL (recherche locale non
épinglée), sans prouver que les 278 acceptations à 8k changent la tour.
Elle relève aussi la lacune de l'argument par racine finale : les nœuds
sont reconstruits sans la naissance retirée. Le
[lemme de la première cofacette](LEMME_PREMIERE_COFACETTE_OMISSION_20260923.md)
comble **conditionnellement** cette lacune, même pour plusieurs clés
retirées si elles sont toutes de rang haut au plus Kmax : il suppose un
catalogue autrement complet, accepté par FULL et un résolveur interne
exact. Il ne prouve pas la complétude du générateur. Comparer K5 et K6
**avec tour FULL et restriction clé par clé** renforcerait le diagnostic
des omissions isolées de fin de fenêtre ; K10 demande K11, hors domaine
actuel, et les suppressions mixtes restent ouvertes. La sonde C n'a pas
de preuve de complétude K10 ; son mode `b13` exige désormais les refus
dans le code de sortie. Ses 129 jugements adverses ne sont pas 129
tests exécutables. Les conclusions d'impossibilité K10/GPU ou de
nécessité d'un générateur par niveau restent des hypothèses.

La [campagne complémentaire de C](c_omission_20260923/README.md),
[contre-lue par B](CONTRE_AUDIT_B_JUGE_Q2_ET_DIGEST_C_20260923.md),
montre que supprimer artificiellement une clé de rang haut peut être
accepté par FULL tout en changeant son condensé : **10/68** retraits q2
acceptés et **26/67** q3 acceptés le font sur les quatre cas 8k de la
seconde campagne, dont un K10. Cela prouve une différence à la tour
témoin, pas une omission naturelle dans le générateur ni la complétude
du témoin. Son juge q2 indépendant teste tous les partenaires d'ancres
échantillonnées (première campagne) : **204 683 présentations admissibles
présentes**, dont
16 506 à `p=9`/K10. Ce ne sont pas des clés distinctes. La seule trame
entière est 08/000000 **sans sol** avec 200 ancres sur 39 885, non une
trame brute multi-séquence ; les entrées ne sont pas hachées dans ces
sorties. Le trou prioritaire de complétude demeure q3 à
`p=Kmax−2`, particulièrement les supports longs.
Le [juge q3 indépendant de C](c_omission_20260923/q3_sample_judge.cpp)
ferme depuis v4 (`e2fd68662`) les deux portes signalées par A et B :
`top_keys` et le mutant ciblent désormais les clés **régulières q3**
(`q_min=3`, trois sites de coquille, `p=Kmax−2`) ; les clés q2 rencontrées
par des triangles aigus sont comptées séparément. Le recoupement compare
les **ensembles exacts d'IDs de coquille**, et le mutant de coquille
corrompue est prévu. Le [certificat de B](CERTIFICAT_B_MARGE_JUGE_Q3_U18_20260923.md)
établit la sûreté du filtre flottant sous IEEE binary64 sans fast-math
pour ces triangles aigus u18. La source et la
[recette v5](c_omission_20260923/run_judges_v5.sh) de `c6042af2b`
ajoutent la comparaison indépendante des clés canoniques q2/q3, un
mutant de clé seule et des sites isolés choisis depuis les coordonnées.
La [contrelecture B](CONTRE_AUDIT_B_JUGES_C_V4_20260923.md) a trouvé
dans ce **runner v5** des faux succès de provenance et de code 1 par
redirection, ainsi qu'un mutant des sites longs seulement observé.
Le **nouveau** [runner v6](c_omission_20260923/run_judges_v6_gates.sh)
de `abf3c3827` vérifie substitutions et sorties, et exige un mutant
`drop-long` apparié avec au moins 50 incidences longues. Mais
l'[audit A des portes v6](AUDIT_A_JUGE_Q3_V6_LONGUES_INCIDENCES_20260923.md)
relève que ce plancher peut être atteint hors du rang q3 critique
`p=Kmax−2`, et qu'une observation `obs` peut accepter un refus du
juge de code 2. Le [reçu publié par `88f30372`](c_omission_20260923/README.md)
est positif dans sa portée : `STATUS=0`, **205 182 incidences q2** et
**286 706 incidences q3** échantillonnées toutes présentes en v5, dont
**55 297 clés q3 régulières** au rang `p=Kmax−2` ; portes
v6 avec mutants causaux tués. Les deux observations `obs` ont effectivement
rendu 0 : le défaut de code 2 est latent, sans fausser ces sorties.
En revanche, s02 passe le plancher de 50 avec 59 longues incidences tous
rangs et seulement **13** au rang critique ; le mutant ne classe pas ses
désaccords par rang. Le juge échantillonne des ancres et fixe
`run_tower=false` : même un code 0 ne contrôlerait que le catalogue
échantillonné, sans certifier la tour FULL ni la complétude globale.
Le [juge v7 et sa contrelecture B](CONTRE_AUDIT_B_JUGE_Q3_CRL_V7_20260923.md)
ferment la définition et la mutation de la **strate critique longue** :
92/179/17 triangles distincts sur huit ancres de chacune des trois coupes
LiDAR 8k/K10, aucun désaccord sain, `drop-crl` et `drop-long` tués dans
cette strate. Une clé retirée du catalogue est détectée par coupe.
L'échantillon ne couvre qu'environ 0,061/0,188/0,016 % des populations
q3 de tête publiées, populations elles-mêmes issues du produit ; ni
complétude générale ni transfert au GPU. À l'étape v7, la garde d'index
des juges devait encore vérifier la bijection et les bornes des IDs
avant tout accès ; le patch R-20 ci-dessous ajoute cette garde, sans être
encore intégré au produit.

La [proposition R-20 de C](c_omission_20260923/judges_product_gates.patch)
porte 21 `gate` et 13 `scale8000` en patch, **34/34** réussis
localement mais pas encore intégrés. La
[contrelecture B](CONTRE_AUDIT_B_PORTES_JUGES_R20_20260923.md) confirme
leur utilité de régression et borne leur portée : 50 ancres q2 ou
20 q3 sur 8 000 sites, aucun FULL (`run_tower=false`), aucun cas
LiDAR/u18 haut ni fixture dédiée aux coquilles étendues ; les
mutants `INDEX_*` n'altèrent que l'index reconstruit par le juge.
Une omission régulière dont la coquille ne touche pas l'échantillon
peut passer malgré tous les codes 0 et l'anti-vacuité. **34/34 n'est
donc ni un certificat du catalogue complet ni de la tour K10**.

La porte de **clés jamais émises** publiée par `683fa46e` change utilement
le sens du contrôle : elle recense des MEB de supports q2–q4 voisins
échantillonnés, indépendamment du générateur, et recherche leurs clés
dans le catalogue. Le développeur rapporte 77 051 puis 77 177
**présentations** admissibles présentes sur trois familles synthétiques
à 2k/8k, K5/K10 ; ce ne sont pas des clés distinctes. Les
[contrelectures A](CONTRELEC_JUGE_CLES_ABSENTES_20260923.md) et
[B](CONTRE_AUDIT_B_JUGE_CLES_ABSENTES_20260923.md) bornent la portée :
500 ancres fixes, 75 supports/ancre (q4 parmi six voisins), aucun
SemanticKITTI ni haut du domaine u18 ; le plancher de mutation cumule
seulement les cas K5 et des clés non ciblées par l'échantillon ; les
dépassements de coquille sont confondus avec les sorties de fenêtre.
Compter les clés distinctes, séparer les refus et planter une clé
admissible par famille, K et strate de rang renforcerait cette porte
unilatérale sans lui donner une portée globale ni FULL. Le statut reste
`complete_relative`.

Le port Euler v13 est publié en **`c768e06a`**. La sonde écrit v13 et le
lecteur G4 en vérifie la borne, la longueur du vecteur et les nouveaux
champs `q34_occupancy`/`tower_phases_ms`. Le lecteur LiDAR `50646eef`
rejetait déjà un vecteur Euler tronqué et une rétrogradation v13→v12,
mais acceptait encore l'absence de ces deux champs. Le correctif
**`1f048aae`** réutilise les juges v13 G4 et lie la revalidation à une
matrice de campagnes annoncée ; son selftest refuse **43/43** mutations
en Python normal et sous `-O` (rejeu local). Le lecteur G4 `515b3666`
reconnaissait déjà un refus réel antérieur au calcul d'Euler
(`chain_shell_above_12`, borne Euler 0) comme refus explicite. Aucune
nouvelle pente LiDAR v13 n'est publiée par ce seul correctif.
La [contrelecture et sa
suite](CONTRE_AUDIT_B_LECTEURS_1F048_20260923.md) confirment que
**`fe1142b5`** fait juger le schéma courant v14 par le validateur G4
**complet** (trois mutants anciennement survivants refusés ; 46/46
selftest normal/`-O`) et confine désormais l'argument des morceaux v8.
Le chemin archivé d'un **disque emboîté** reste lié par son seul nom :
un faux `/tmp/evil/<même nom>.u32le` passe avec matrice/hash/commit,
sans lire ce fichier ; les octets reconstruits et leur FNV restent
vérifiés. C'est une limite de fidélité de commande, non une clé omise.
Le lecteur courant n'a plus de compatibilité locale v13 ; le reçu G4
R8 reste lu par son lecteur épinglé.

Le calcul Euler reste inclus dans `census_ms` (`tower_chain.cpp:486,539–601`)
et un `E_K` faux refuse **avant** FULL (`:602–612`), alors que la décision
du constructeur prévoit son coût séparé et un refus après tour FULL réussie.
Les portes mathématiques passent sur leurs cas locaux, sans contre-exemple
trouvé à la formule ; la porte 8k est ajoutée en `a08378da`. **`1f048aae`**
impose aux deux mutants de chaîne la raison
`chain_catalogue_euler_violated`, et le lecteur G4 lie ce refus au statut
Euler `fails` dans les deux sens. La porte d'échelle de **`96bd6190`**
échantillonne une boule sur 64 pour son recensus brut : contrôle utile mais
déterministe et non exhaustif, sans couverture garantie de chaque famille
de coquilles. **`fe1142b5`** fait maintenant vérifier
`kInvariantViolated` et `kFails` par la porte C++ des mutants ; elle
ne recalcule pas encore sa borne ni une somme `by_k` fausse dans cette
branche. Pour `run_tower=false`, expliciter la positivité des
supports réguliers, que les fabriques exactes q2/q3/q4 imposent déjà.

Les certificats exacts actuellement raccordés comprennent la saturation
profonde de l'atlas, le census q3 sur fragment complet, la preuve de voies
q3/q4 mortes et le cache de nœuds témoins. Le propriétaire d'arête et les
tests stricts gardent les contacts dans la coquille. La porte propriétaire
de `84c74a5e` tue causalement l'ancien cache d'index par adresse nue et
vérifie la réutilisation après `bad_alloc`. `e5688680` exige désormais le
réemploi effectif de la même adresse hors ASan ; la quarantaine ASan
empêche ce plancher sans invalider les autres contrôles. Voir la
[contrelecture du chargement des
formes](CONTRE_AUDIT_B_CHARGEMENT_FORMES_Q34_WIP_20260923.md) et la
[preuve conjointe](CONTRE_AUDIT_B_Q34_PREUVE_CONJOINTE_WIP_20260923.md).

`a78664d4` tente le certificat de voies mortes d'abord sur les sites de la
boule diamétrale de l'arête, sous-ensemble du cover complet ; toute voie
ouverte repasse par celui-ci. Le sous-ensemble ne peut ajouter un faux
témoin intérieur. Les portes locales comparent les candidats aux petits
oracles et exercent fermeture puis repli ; R6 isole désormais ce levier
**ON par défaut** en ablation FULL sur G4. `e5688680` protège
aussi les consommateurs complets contre un cœur passé par erreur ;
`cc4664e5` place le même refus avant les retours q4 à K1/2. La porte
FULL actuelle
compare les cinq leviers ensemble, donc couvre le raccord sans attribuer
une égalité au seul cœur. Corriger aussi le commentaire de profondeur
« exacte » dans `q34_dead_lanes.cpp` : sur le cœur, le compte ponctuel est
un minorant et ne sert qu'à abandonner une preuve. Voir la
[contrelecture du cœur](CONTRE_AUDIT_B_NOYAU_DIAMETRAL_WIP_20260923.md).

## Ce que mesurent les reçus G4

Le [reçu R3](../receipts/g4_tower_r3_20260923/README.md) donne six paires
FULL CPU on/off du certificat de voies mortes, même entrée et sorties :
la chaîne passe de **12,93→8,89 / 10,12→6,19 / 19,71→10,93 s** à K5
et **48,33→35,56 / 36,29→25,96 / 62,99→37,75 s** à K10
(08/000000, 000100, 000200). Générateur, catalogue, ordres et digest
coïncident dans chaque paire ; les masses de travail expliquent le gain
aval. La [contrelecture indépendante](CONTRE_AUDIT_B_G4_R3_20260923.md)
vérifie les sorties brutes, le préflight et l'arrêt ciblé. Cette ablation
ne donne ni complétude exhaustive, ni borne sous-quadratique.

Le [reçu R4b](../receipts/g4_tower_r4b_20260923/README.md) ajoute six
paires cache de témoins ON/OFF : la chaîne gagne **0,12 à 0,89 s** selon
la paire, en une seule exécution par configuration. Les rejets par cache
représentent **53,36–69,76 % des paires résiduelles développées**, pas
de toutes les paires du nuage. Les sorties demeurent égales ; ni preuve
conjointe ni coût de croissance ne sont isolés par cette ablation.
La [contrelecture R4b](CONTRE_AUDIT_B_G4_R4B_CACHE_20260923.md) recoupe
le reçu. La première tentative R4, [arrêtée avant le
worker](../receipts/g4_tower_r4_preempted_20260923/README.md), ne donne
aucun chrono. Son README annonce `compute.instances.preempted`, mais ne
joint pas la trace GCE brute de cet événement ; les traces hôte suffisent
à prouver l'absence de sonde et l'arrêt ciblé, pas la cause exacte.
Le [contrôle d'antichaîne du cache](CACHE_TEMOINS_COUT_VALIDATION_20260923.md)
paie aussi du travail non inclus dans `node_tests` : les six cas R5
impliquent au moins **1,090 milliard** de tours de validation et
**424,330 millions** de comparaisons d'intervalles. Ce sont des
minorants de comptage, pas des durées ; un ticket interne possédé
pourrait réutiliser l'antichaîne certifiée sans assouplir l'API publique.

R5 exécute **13/13 cas FULL CPU `complete_relative`** : deux répétitions
W48 des trois trames sans sol à K5/K10, plus 000000/K10/W24. Les 245
hashes et sept comparaisons d'objet concordent ; l'arrêt ciblé
`TERMINATED` est attesté. À s8 avec les quatre leviers actifs, les
intervalles de mur entre les deux répétitions W48 sont :

| 08/ | K1..5 | K1..10 |
| --- | ---: | ---: |
| 000000 | 5,96–5,98 s | 17,00–17,37 s |
| 000100 | 4,26–4,33 s | 12,07–12,09 s |
| 000200 | 7,38–7,40 s | 19,28–19,57 s |

La tour seule prend 0,84–1,10 s à K5 et 4,01–5,37 s à K10 ; q3/q4
reste le premier poste (65–70 % du total à K5, 45–57 % à K10).
Face à R4b, le temps de tour K10 descend de 11,42–14,89 s à
4,15–5,37 s sur la même cible, mais plusieurs changements séparent les
sessions : ce n'est **pas** une ablation causale des seuls ordres K
concurrents. La [contrelecture R5](CONTRE_AUDIT_B_G4_R5_20260923.md)
détaille les limites. Le cas 000000/K10 à W24 prend 21,59 s contre
17,00–17,37 s à W48, pour moins de CPU·s ; un seul cas ne sépare pas
déséquilibre, contention et taille des lots.

R5 mesure **uniquement le CPU** d'un G4, sur u18/grille 1 mm sans sol de
la séquence 08. Le meilleur mur est encore 4,263 s à K5 et 12,067 s à
K10 : aucun contrat de 1 s, 100 ms, GPU ou croissance sous-quadratique
globale n'est acquis. Le [premier reçu local](../receipts/first_tower_20260922/README.md),
[R1](../receipts/g4_tower_r1_20260922/README.md) et
[R2 refusé par son validateur](CONTRE_AUDIT_B_G4_R2_PREFLIGHT_20260923.md)
restent des témoins historiques ; leurs temps ne remplacent pas R5.

Le [reçu R6](../receipts/g4_tower_r6_20260923/README.md),
[contrelu indépendamment](CONTRE_AUDIT_B_G4_R6_20260923.md), a exécuté
le snapshot `78ce9fd4`
sur G4 24 cas FULL CPU (trois trames entières sans sol, K5/K10,
W48/s8, deux répétitions ON/OFF du seul cœur), tous
`complete_relative` ; la relecture indépendante de la capture rend
`completed`, avec arrêt ciblé certifié et 390/390 empreintes intègres.
Dans les douze paires, émissions q2/q3/q4, résumés de catalogue,
ordres, travail FULL et digest coïncident ; R6 ne publie pas le flux
complet des clés/supports/coquilles. Le mur de chaîne gagne
**0,51–8,66 %** par paire (**1,8–7,7 %** sur les moyennes des deux
répétitions par scène/K) ; le CPU·s gagne **8,09–21,18 %** par paire.
Le cœur ferme **54,9–58,9 %**
des arêtes qui arrivent au cover et réduit les formes chargées de
**72,85–81,87 %**, mais augmente les visites d'index core+cover de
**21,0–45,6 %**. Meilleurs cas ON : **4,151 s à K5** et **11,726 s à
K10**, toujours hors contrat. L'[erratum
R6](../receipts/g4_tower_r6_20260923/ERRATUM.md) corrige « générateur
identique » : `q34_cover_builds` change par construction
(000000/K5 : 900 377 ON, 2 043 612 OFF), tandis que les émissions et
masses de candidats restent identiques. Il ne corrige pas encore les
plages arrondies du README, qui omettent 0,51 %, 8,66 % et 72,85 %.
Les trois compteurs du cache témoin varient légèrement entre
répétitions, sans changer l'objet.

À travail CPU mesuré inchangé, même une répartition idéale sur les
**48 fils logiques** du G4 laisserait, pour le meilleur cas R6 ON,
**1,89 s à K5** et **5,81 s à K10** de CPU·s/48 avant tout coût de
synchronisation. Ce calcul ne borne ni un nouvel algorithme ni une
voie GPU ; il montre que le seul ordonnancement CPU de l'existant ne
peut atteindre 1 s.

La reprise du cover complet à partir des nœuds terminaux du cœur est
exacte en principe, mais R6 borne son bénéfice en visites : sur
08/000100 et 000200, à K5 **et** K10, les visites du cœur seul excèdent
déjà les visites du cover OFF (de **0,2 à 12,0 %** selon le cas). Même
un second parcours gratuit ne ferait pas descendre ce compteur sous OFF.
Sur 08/000000, il faudrait économiser au moins **81,3 %** des visites
du cover ON à K5 et **72,6 %** à K10 pour atteindre seulement le niveau
OFF. Cette borne ne condamne pas la reprise : elle peut réduire CPU et
mur en conservant la baisse des formes chargées, mais impose de mesurer
son coût mémoire et de viser ensuite un certificat **avant** l'expansion.

Le [reçu R7b](../receipts/g4_tower_r7b_20260923/README.md) reprend sur
G4 le paquet `8e8b83a3` : **24/24 cas FULL CPU
`complete_relative`**, deux répétitions entrelacées ON/OFF du seul
MEB proposé pour trois trames 08, K5/K10, s8/W48, dix-huit comparaisons
de **projection d'objet** égales, arrêt ciblé certifié et **388/388**
empreintes du paquet vérifiées par la
[contrelecture indépendante](CONTRE_AUDIT_B_G4_R7B_20260923.md)
en Python normal et sous `-O`. Le reçu ne publie pas les flux
intégraux de clés/supports/coquilles/parents. Les trois compteurs du cache témoin
`witness_cache_queries/node_tests/rejected_pairs` varient légèrement
entre les paires malgré l'identité de l'objet ; la phrase du README
« travail hors MEB identique » doit être lue comme **travail de tour
hors MEB**, pas comme égalité intégrale du ledger générateur.
Toutes les propositions MEB sont
vérifiées. À K10, le MEB ON réduit la tour de **5 à 8,4 %** et la chaîne
de **2 à 4 %** ; à K5, le gain de tour reste dans le bruit et la chaîne
est parfois plus lente. Meilleurs cas ON : **3,67 s à K5** et **9,58 s
à K10** pour `chain_total`, plus respectivement **0,16 s** et
**0,80 s** de digest synchrone. Ce sont encore des secondes, sur CPU
seul et une seule séquence. R7b inclut d'autres changements depuis R6 :
leur effet ne se déduit pas de cette ablation. Il n'exécute ni Welzl
move-to-front, ni le nouvel échantillonnage FULL, ni la libération
précoce de `key_slots`, et ne prouve pas la complétude des clés omises.

Le [reçu R8](../receipts/g4_tower_r8_20260923/README.md) du paquet
`515b3666` ajoute Euler et l'occupation q3/q4 : **20/20 cas FULL CPU
`complete_relative`**, trois trames 08 **sans sol** à 1 mm, toutes les
empreintes archivées vérifiées, lecteur du commit épinglé positif en
Python normal et sous `-O`, arrêt ciblé `TERMINATED`. À W48/s8, la chaîne
vaut **3,657–6,395 s à K5** et **9,403–15,372 s à K10** sur les deux
répétitions, digest de contrôle exclu. Euler tient sur les ordres jugés
mais reste nécessaire seulement.
L'attente de file occupe **35–49 %** du temps des fils q3/q4 à K5, donc
l'ordonnancement mérite une ablation. La
[contrelecture R8](CONTRE_AUDIT_B_G4_R8_20260923.md) relève le verrou
supplémentaire : **chaîne hors q3/q4 = 1,147–1,685 s dès K5**, et tour
aval seule **2,972–3,895 s à K10**. Accélérer le seul q3/q4 ne peut faire
passer sous 1 s le chemin mesuré actuel. GPU, trames brutes avec sol,
diversité des séquences et complétude absolue restent ouverts.

Le [rectificatif d'ordonnancement](RECTIFICATIF_R8_Q34_ORDONNANCEMENT_20260923.md)
relève une erreur commune au reçu et à sa contrelecture : **tous les
rectangles survivants sont proposés à la file**, même sous 256 paires ;
256 ne règle que le découpage des plages. Sur 000000/K5, **2,006 M** des
**3,134 M** rectangles sont rejetés dans les jobs avant publication, puis
**1,196 M** plages sont publiées. Mesurer séparément la fin des jobs de
front et des plages est nécessaire avant d'attribuer l'attente. À travail
CPU q3/q4 R8 inchangé, le meilleur cas 000100/K5 requiert déjà au moins
**1,611 s** pour cette phase même sur 48 fils parfaitement occupés ; le
rééquilibrage seul ne clôt pas le contrat.

La sonde v14 **`67fce4e9`** active deux leviers d'ordonnancement q3/q4
distincts (jobs par masse, grain 64 au lieu de 16). Sa
[contrelecture](CONTRE_AUDIT_B_V14_ORDONNANCEMENT_20260923.md) confirme
des portes bornées d'exactitude nettement renforcées : plans mass-first
contre front oracle, candidats q3/q4 normalisés contre oracle rationnel.
Au moment de cette première contrelecture, le gain local annoncé sur
le plus long job et l'attente ne possédait **aucun reçu brut v14**.
La [contrelecture R9](CONTRE_AUDIT_B_G4_R9_ORDONNANCEMENT_20260923.md)
porte maintenant sur un reçu G4 CPU apparié figé dans Git :
24/24 `complete_relative`, 18 comparaisons égales, validateur épinglé
normal/`-O` positif. Avec les **deux** leviers ON, K5 passe de
3,69–6,30 s à **2,80–4,07 s** selon la trame/répétition ; K10 de
9,45–15,09 s à **8,60–11,89 s**. L'attente q3/q4 K5 tombe de
35–49 % à moins de 1 %, mais la phase reste 1,66–2,39 s. À travail
CPU inchangé, son mur n'est plus que 1,02–1,03 fois `cpu_sum/48` à K5 :
réduire le **travail** ou changer de backend devient la priorité.
Le README du reçu devrait encore préciser que ces trois entrées sont
**sans sol, à 1 mm, toutes de la séquence 08**, et que « chaîne » exclut
le digest (meilleur cas 2,799 s de chaîne, **3,021 s de mur externe**).
Ses q3/q4, attente et plus long job tabulés sont ceux de la répétition
0, tandis que les colonnes chaîne portent 0/1. `q2+fusion+recensement`
vaut **0,884–1,410 s à K10 ON**, au-delà de la borne « 0,4–0,8 s » du
README ; Euler `holds` ne contrôle que K1..3 à K5 et K1..8 à K10.
Les nouveaux chronos ne couvrent pas les plages publiées et les deux leviers
ne sont pas séparés. Une matrice 2×2 appariée sur une trame difficile
avec q3/q4 et chaîne complets est la prochaine porte d'attribution.
La [contrelecture des durées v14](RECEPTION_V14_CHRONOS_Q34_20260923.md)
montre que le lecteur accepte encore à **`fe1142b5`** un maximum de
job supérieur à leur somme, et une somme jobs+attente supérieure au
budget mural des fils. Ces identités doivent entrer dans la porte avant
d'utiliser les nouveaux chronos pour expliquer le gain.
Le débordement de masse diagonale de la version initiale v14 est
**fermé en source** par `fe1142b5` (calcul i128 exact sur le domaine
des plans admis), sans nouveau reçu de performance.

Le [reçu R10 contrelu](RECEPTION_G4_R10_20260923.md) isole ensuite le
recouvrement phase 0/phase A de FULL sur **12 paires G4 CPU** : 24/24
cas `complete_relative`, 388/388 SHA conformes, objets logiques ON/OFF
et par rapport à R9 ON identiques. La tour gagne **0,049–0,151 s à K5**
et **0,461–0,761 s à K10** ; meilleur `chain_s` ON **2,74 s à K5**
et **8,07 s à K10**, encore hors contrat. Le README du reçu réduit
à tort le gain K5 maximal à 0,12 s et le ralentissement maximal de la
phase 0 à 8 % : les maxima lus sont **0,151 s** et **9,94 %**.
Le paquet exécuté `33d51efd` précède la validation parallèle du
catalogue et le remplissage parallèle des programmes de `308ca2a1`,
ainsi que le lecteur K1 corrigé en `c19e4b49` ; ne pas leur attribuer
ces chronos. La phase q3/q4 du meilleur K5 reste **1,65–1,69 s**.
La [contrelecture de la passe 2](CONTRE_AUDIT_B_G4_R10_ET_PASSE2_20260923.md)
ne trouve pas de course dans les plages de dispersion, mais note que
`records` devient zéro sur tout refus tardif. Un essai local minimal
sur trois sites collinéaires, deux records et K1 reproduit
`full_ball_outside_rank_window` avec `records=0` après `308ca2a1`,
contre `records=1` à `33d51efd` ; statut et raison restent identiques.
Sur 08/000000 sans sol K5, un binaire local postérieur au port rend le
même digest R10 avec 1 306 696 boules, franchissant les seuils des deux
voies parallèles. Cela soutient l'identité des sorties sur ce cas, sans
remplacer une porte de payload complet ni une qualification G4 du port.
Le [shadow des voisins du cœur](../receipts/knn_core_probe_20260923/README.md)
mesurait 96 %/86 % de fermetures K5/K10 sur une coupe 16k, mais choisissait
des sites du cœur **déjà construit**. Le [reçu négatif publié](../receipts/near_sites_negative_20260923/README.md)
teste désormais les 16 voisins **globaux** : à 08/000000 sans sol, il enlève
62 % des recherches de paire K5 pour seulement −2 % de CPU q3/q4 et régresse
de +3,7 % à K10. Le port a été retiré. La preuve mathématique reste utile :
[tout sous-ensemble de sites distincts](CERTIFICAT_Q34_SOUS_ENSEMBLES_LOCAUX_20260923.md)
donne une fermeture sûre, mais sa sélection et son coût complet doivent
être payés. Ce reçu `--no-tower` ne compare pas les clés une à une.

Le [reçu micro q3/q4 publié](../receipts/q34_micro_levers_20260923/README.md)
mesure environ −4 % de CPU pour l'incrément `+1` non vérifié avec garde sur
trois entrées, digest FULL identique sur deux paires. Le changement global
brisait pourtant le contrat de dépassement d'autres API publiques
(`point_witness` rebouclait de `UINT64_MAX` à zéro dans notre fixture) :
**levier retiré du produit**. La variante sûre à compteurs DFS locaux ne
gagne que 0,9 %. Le profil attribue environ 40 % du CPU q3/q4 au filtrage,
sans poste unique supérieur à 16 % ; les raffinements de rectangles et le
second cache `b` régressent dans ce reçu. Ces résultats locaux K5 ne
qualifient ni K10, ni G4, ni une borne de croissance.

## Verrou q3/q4 : réduire le travail avant l'expansion

Le certificat de voies mortes a un vrai bénéfice aval, mais construit
encore le cover avant de charger les formes de tous ses sites. R5/K10
compte **4,15–9,28 milliards** de `dead_form_sites` sur 35–46 k sites ;
`load()` écrit en plus deux formes nulles par cover. Sur 000000/K10,
les 7,805 milliards d'incidences site–cover logiques montrent pourquoi
un certificat qui relit chaque cover ne ferme pas le verrou structurel.
Le [reçu exploratoire par
arête](CONTRE_AUDIT_B_RECU_VOIES_MORTES_20260923.md) a corrigé les
pourcentages et covers moyens mal définis dans la provenance initiale ;
ses prototypes sans FULL ne sont pas une mesure R5.

Une optimisation exacte de **constante** mérite une ablation ciblée : pour
une même arête et un même index, le cœur diamétral est inclus dans le cover
complet ; la forme affine de `Q34DeadLaneProver::load` dépend seulement de
l'arête et du site. Si une voie survit au cœur, les deux flux de rangs sont
triés : en gardant les plages du cœur jusqu'au cover, une fusion **à rebours**
peut déplacer ses formes dans le buffer complet et calculer seulement les
nouveaux sites, sans nouveau tableau de formes. Vérifier identité de l'index
et de l'arête, invalider le prover avant l'extension, puis mesurer
`full_forms_reused`, `full_forms_new`, CPU et
RSS. Les agrégats actuels n'isolent pas le cœur des seules arêtes ouvertes ;
la fusion parcourt encore tout le cover et ne change pas la borne globale.

Diagnostic local du commit `a78664d4` sur **un quart spatial seulement de
la trame brute** 08/000000 à 1 mm (sol conservé)
([entrée u32le](../../morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i/scene_00_000000_grid/quarter_x_neg_y_neg.u32le),
30 263 sites, K5/s8/W8, tour statique W8, quatre autres leviers ON) : deux paires
OFF/ON du cœur gardent catalogue, ordres, travail FULL et digest égaux.
Le cœur ferme 443 495 des 990 559 arêtes qui survivent au filtre ; les
incidences site–cover payées passent de 454,08 M à 120,56 M en additionnant
**cœur et cover complet**. Le CPU de chaîne baisse de 72,05/71,88 à
68,72/68,56 CPU·s, tandis que le mur de chaîne vaut 14,81/17,04 s OFF
et 16,08/24,06 s ON. Sur cet hôte partagé, aucune amélioration murale
stable n'est établie ; les sorties JSON et le binaire local ne constituent
pas un reçu G4. Entrée SHA256 `2632c86e…6c516e`, binaire
`267dbed7…b7734` ; les sept sources principales ont les mêmes empreintes
que le commit publié. Les mesures G4 doivent inclure les visites et tests
du **core_cover**, publiés par la sonde v8 de `e5688680` et maintenant
reçus sur G4 dans R6. Les temps et masses par worker restent nécessaires
pour diagnostiquer le chemin critique. R6 a réalisé l'ablation appariée
sur trames sans sol entières avec préflight ON ; l'expansion `A×B`
demeure entière.

Une [ablation locale complémentaire sur deux coupes **sans
sol**](CORE_LIDAR_LOCAL_20260923.md), K5/K10/s8/W8 et sans FULL,
compare aussi le **flux entier** des présentations q3/q4 : OFF et ON
coïncident sur 117 196, 565 007 puis 2 032 711 lignes, avec clé,
support, profondeur et coquille exacts. Le coût reste dépendant du
régime : à K10, les visites core+cover montent de 83,1 % sur 8 225
sites mais baissent de 10,1 % sur 13 055 sites. Le CPU de chaîne
diminue dans les deux cas ; le mur q3/q4 régresse sur la petite coupe
et gagne sur la grande, une répétition sur hôte partagé. Construire
le cover complet puis filtrer le cœur exigerait de scanner jusqu'à
1,63 milliard d'incidences sur la grande coupe, contre 115 millions
pour le cœur actuel : cette variante ne mérite pas de port sans
prototype favorable. Aucune paire développée n'est supprimée.

La priorité constructive est de prouver un masque q3/q4 **avant**
l'expansion de produits résiduels `A×B`, puis avant le cover pour les
arêtes restantes. Le filtre actuel de rectangle ne voit que des témoins
universels ; un filtre exact de ligne `a×B_node` peut écarter `|B|`
paires d'un coup, à condition de transporter le vrai nœud B et sa boîte.
Le sidecar trouve 79–88 % de masse développée dans des rectangles
éligibles, mais paie 15,7–29,8 visites DFS par paire évitable sur son
échantillon ; le prototype local de ligne indépendante régresse en CPU
total. Tester d'abord un ticket borné de nœuds témoins réemployés, un
seuil de déclenchement et le repli exact. Ne jamais compter deux fois
un témoin : le ticket porte masque, antichaîne, compte et curseur, et
le DFS ne visite que le complément de ses plages. Voir le
[contrat de coûts](CONTRAT_COUTS_ET_PARALLELISATION.md) et les
[certificats par rectangles](PISTE_B_Q34_RECTANGLES_AVANT_EXPANSION_20260923.md),
[nœuds avant cover](PISTE_B_Q34_NOEUDS_AVANT_COVER_20260923.md) et
[gardes par blocs](DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md).
Leurs bornes locales sont exactes ; aucun gain net LiDAR de ces
certificats par blocs ni majorant global sous-quadratique n'est démontré.
Une [fixture exacte de cellules de
centres](FIXTURE_CELLULES_CENTRES_Q34_20260923.md) tue à K5 un produit
non singleton q3/q4 sans témoin universel et évite potentiellement 36
formes de cœur ; elle ne garantit ni l'apparition de ce rectangle dans
le front réel ni une économie sur LiDAR. Les shadows de ligne/`h_a`
retirent des millions de paires mais **zéro arête qui aurait atteint le
cœur** dans l'échantillon publié : mesurer désormais les formes de cœur
réellement évitées par rectangle, pas seulement les paires filtrées.
La [mise en budget des cellules](PISTE_B_Q34_RECTANGLES_AVANT_EXPANSION_20260923.md)
ajoute un ciblage de **tentatives** sur produits lourds, sans quota de
candidats : à s8/08/000000/K10, 72 329 rectangles ouverts de masse ≥16
portent 87,3 % des paires résiduelles, mais quatre cellules×vingt IDs×huit
coins paient déjà 46,29 M comparaisons avant sélection/repli. À s10/s12,
les masses résiduelles baissent alors que les rectangles ouverts montent ;
leur nombre lourd n'est pas mesuré. Un certificat ne supprime que son bit
de voie q3 ou q4, jamais une `BallKey` globale qui pourrait avoir un
`q_min` plus petit ailleurs. Un shadow doit suivre les arêtes réellement
parvenues au cœur et l'état futur du cache avant toute ablation FULL.

Le [shadow LiDAR de la palette par ancre](SHADOW_HA_Q34_LIDAR_20260923.md)
isole une proposition plus légère qu'un nouveau DFS de ligne. À K10/s8
sur 08/000000 sans sol, il ferme **3 499 305 / 30 777 213** paires
résiduelles avant expansion ; les **30 777 213 masques de paire** du
replay mono restent identiques, comme les **4 507 278 covers potentiels**.
L'économie CPU locale indicative vaut environ **1,29 s** après préparation
et tests de palette, sur hôte partagé et sans FULL/G4. Le cache de paire
rejette déjà la plupart de ces paires à faible coût ; ne porter la palette
qu'après une ablation de chaîne ON/OFF, identités complètes et coût par
worker inclus. Une palette des seuls proches peut manquer les témoins
dans la direction de B, comme le montre la contre-fixture B.

Le [shadow orienté par octant](SHADOW_HA_OCTANT_Q34_LIDAR_20260923.md)
emploie les mêmes voisins proposés et le prédicat entier exact sur
l'octant pointant vers B. Sur 08/000000 sans sol K10/s8, il ferme
**5 177 835** paires avant filtre, contre **3 499 305** pour les
proches ; les **30 777 213 masques** et **4 507 278 covers** restent
identiques. Le replay mono gagne environ **3,85 CPU·s locaux nets**
après préparation et lignes, sur hôte partagé ; 08/000000 K5 gagne
**1,82 CPU·s**, mais 08/000100 K5 seulement **0,19 CPU·s** et régresse
en mur. Les passes K10/s10 et 08/000100 K10 ne jugent que les lignes,
pas le replay. Conserver l'octant en SHADOW jusqu'à une ablation de
chaîne entière, mémoire comprise, sur plusieurs séquences et sol brut ;
aucun cover ni travail aval n'est supprimé ici.

Le [contre-audit du grand-livre q3/q4](LEDGER_VISITES_CACHEES_Q34_20260923.md)
montre que six parcours d'index déjà comptés par le générateur et les
sites balayés par le sweep q4 n'étaient pas projetés dans la sonde
FULL v11 de R7b. Le port v12 `4530644b` les publie maintenant ; ses
identités/bornes sont cohérentes avec les options actuelles de chaîne.
Ils incluent les témoins par rectangle/paire, l'accès aux graines q3
par arête et trois parcours q4 par arête. Sur un index à `2n−1` nœuds,
leurs bornes par appel restent linéaires en `n` ; les comptes R7b publiés
ne permettent donc pas d'écarter un coût caché
`(rectangles + paires recherchées + arêtes q3/q4)×n`. Le reçu v12
ci-dessous publie leur croissance sur trois trames LiDAR appariées.

Le [reçu local v12](../receipts/lidar_scaling_local_20260923/README.md),
[contrelu indépendamment](CONTRE_AUDIT_LIDAR_SCALING_V12_LOCAL_20260923.md),
archive trois trames sans sol à K5/K10 : **60 cas de sonde et six résumés**,
soit 66 JSON ; la formule « 66 cas » du README compte aussi les résumés.
Sur 08/000200/K10, de 16k à 32k, les paires développées font ×4,27 et la
population logique des cœurs ×7,25, alors que le catalogue de boules fait
×1,86. C'est un signal local de coût caché à attaquer avant le cœur,
pas une borne asymptotique. Les temps viennent d'un hôte CPU partagé ; le
reçu n'est ni G4 ni une preuve de contrat ou de complétude absolue.
La [lecture des demi-scènes et quarts](CROISSANCE_LIDAR_PLANS_ET_DENSITE_20260923.md)
compare les 42 cas spatiaux du reçu : 0/36 pentes CPU parent→morceau
dépassent 2, mais **13/36** pentes de population logique du cœur le
dépassent. Les coupes changent la géométrie. Le
[premier reçu de densité](lidar_density_scene02_20260923/README.md)
garde chaque secteur de 08/000200 fixe et ajoute **28** sondes à 1/4 et
1/2 des sites ; au K10, `dead_core_form_sites` a une pente d'au moins 2
sur **4/28** relations adjacentes, concentrées dans `x≥0` et
`x≥0,y<0`, malgré des pentes de paires et CPU sous 2. Ce compteur omet
les deux extrémités par charge, que le code calcule aussi : le total exact
des formes du cœur est `core_sites = dead_core_form_sites + 2 × dead_core_loads`.
Il franchit 2 sur **3/28** relations de ce premier reçu.
L'extension aux [trames entières 000000/000100](lidar_density_full_3scenes_20260923/README.md)
et à [leurs six secteurs](lidar_density_sectors_00_01_20260923/README.md)
ajoute **56 sondes**. Sur les trois scènes et sept secteurs, **10/84**
relations de densité adjacentes atteignent `p_core_sites≥2` pour toutes
les formes réellement calculées, contre **14/84** pour le sous-total hors
extrémités ; le quart
`x≥0,y<0` le fait dans chacune des trois trames à K5 ou K10. Paires,
visites/bornes des nœuds et CPU restent sous 2 dans ces essais, sans
preuve asymptotique. Les trois trames appartiennent à une seule séquence,
sans sol et sans mesure G4. Une ablation appariée du seul site qui étend
fortement `z` dans le quart chaud de 000200 laisse la pente K10 du
sous-total de formes hors extrémités
à **2,042528** : cet extrême ne porte pas à lui seul le signal. Le secteur
est fixe, mais l'étendue des sites sélectionnés varie avec la densité.
Une [ablation à boîte exactement fixe](lidar_density_bbox_fixed_20260923/README.md)
sur ce quart échange seulement trois puis deux IDs aux densités 1/4 et 1/2 ;
la pente K10 de ce sous-total reste **2,058215 puis 2,042880**, pratiquement
inchangée. Ce signal ne vient donc pas seulement de l'étendue de la boîte.
Une [contre-épreuve de coupe physique](lidar_scene02_physical_cut_20260923/README.md)
reprend ensuite les deux quarts `y<0` entiers de 08/000200/K10 en
classant les **retours float32 avant grille**, avec la même grille u18
globale. Un seul retour brut (14826) change de côté ; il porte l'ID de
site **61939 en profil grille** et 61942 en profil float32, d'où une
jointure impérative par correspondance des retours. Le quart chaud
`x≥0,y<0` perd 2 980 formes sur 583 000 415 et sa pente 1/2→entière
passe de 2,035162 à **2,035350** : le franchissement 2 persiste.
Cette contre-épreuve ferme ce biais d'arrondi pour ce lien seulement ;
elle ne transforme pas toute la matrice sans sol en coupes physiques.
Le [premier reçu brut avec sol](lidar_raw_physical_scaling_20260923/README.md),
[dont les 21 cas ont été contre-lus indépendamment par B](CONTRE_AUDIT_B_LIDAR_BRUT_PHYSIQUE_20260923.md),
compte désormais **21 cas K5** sur une trame entière 08/000000 à 1 mm :
sept secteurs par plans **float32 physiques**, chacun aux trois densités
emboîtées. La sélection garde les 123 389 retours sans fusion ;
trois changeraient de secteur si le signe était pris après grille.
Sur 30 847→61 694→123 389 sites, toutes les formes calculées par
le cœur passent de 37,01→128,85→559,66 M, soit des pentes finies
**1,800 puis 2,119**, contre **1,218 puis 1,297** pour CPU·s. Le
sous-total hors extrémités publié initialement vaut 35,46→125,48→551,69 M.
Les deux moitiés cumulent 97,0 % des charges de cœur du plein, mais seulement
31,5 % de toutes ses formes : 45,68 contre 140,39 formes par charge.
C'est un verrou de **masse par cœur**, pas seulement de nombre de cœurs.
Dans les 14 comparaisons de densité à secteur fixe, **2** pentes de
toutes les formes dépassent 2 : la trame entière 1/2→entière (2,119) et
le quart `x≥0,y<0` 1/4→1/2 (2,002). Les pentes CPU sont 1,200–1,328.
La part des formes retrouvée en sommant les moitiés tombe de **0,567**
à densité 1/4 à **0,315** à densité entière ; la part des charges de
cœur reste autour de 0,97. Sept des 18 liens spatiaux K5 dépassent aussi
2 sur le total des formes. Les chronos muraux des nouvelles coupes sont
fortement perturbés par la contention de l'hôte partagé.
Le plein prend 48,36 s de chaîne locale W8 et 1,93 GiB RSS ; ni borne
asymptotique ni contrat G4 ne sont acquis.
Le [complément brut K10](lidar_raw_k10_density_20260923/README.md) reprend
**les mêmes trois ensembles d'IDs et les mêmes octets** de la trame entière :
30 847→61 694→123 389 sites, trois sorties `complete_relative`. Ses formes
du cœur, extrémités comprises, font 106,60→330,91→1 254,25 M, de pentes
finies **1,634 puis 1,922**, contre 1,800 puis 2,119 à K5. Les
chiffres initiaux 103,15→323,69→1 238,63 M comptaient seulement les
sites hors extrémités. Les CPU·s de chaîne font
173,008→383,704→905,514 (pentes 1,149 puis 1,239) ; le plein porte
11,387 M boules de catalogue et **8,219 GiB** de RSS. Les comptes des cinq
premiers ordres K10 sont égaux à ceux de K5 sur chaque entrée, sans
identité clé par clé ni preuve des clés jamais émises. Cette coupe K10 ne
franchit pas la pente 2 des formes, mais la masse absolue et la mémoire
restent des verrous ; les murs de l'hôte partagé ne qualifient pas G4.
La [contrelecture B](CONTRE_AUDIT_B_LIDAR_BRUT_K10_20260923.md)
confirme **10/10 hashes**, les IDs/coordonnées emboîtés, les dix ordres
et les pentes ; elle précise que les trois lignes historiques n'ont
pas de champ `validated` archivé et que le cas plein chevauche un autre
calcul CPU sur l'hôte partagé. Cœur **plus** couverture complète écrivent
**2,329 milliards** de formes au plein K10 : le sous-total de **2,304
milliards** de la contrelecture B omet leurs deux extrémités par charge,
soit 24,745 M formes au plein. Sur les trois densités, le total exact vaut
298,608→693,370→2 328,973 M (`p=1,215/1,748`) ; la croissance du nombre
moyen de sites par charge explique l'essentiel de sa pente.
Le [complément brut K10 par plans physiques](lidar_raw_k10_sectors_20260923/README.md)
ferme maintenant la matrice **sept secteurs × trois densités** : dix-huit
nouvelles sondes K10, six moitiés rejouées, et les trois pleins antérieurs
sur les **mêmes octets** que K5. Un des 14 liens de densité franchit la
pente 2 de toutes les formes du cœur : le quart `x≥0,y<0` à 1/2→entière
(**2,029**, contre 2,057 hors extrémités), malgré une pente CPU de 1,251.
Trois des 18 liens spatiaux franchissent 2 pour le total des formes,
contre quatre pour le sous-total ; à densité entière, plein→deux
demi-scènes donne **2,167/2,306** pour le total, contre 1,922 pour la
densité 1/2→entière du plein. Les deux axes ne sont pas interchangeables.
À densité entière, Σformes/plein vaut 0,425 pour les moitiés et 0,381
pour les quarts, tandis que Σcharges cœur/plein
vaut 0,969/0,958 et Σboules catalogue/plein 0,996/0,991. Les six rejeux
gardent formes, charges, paires, catalogue et ordres ; seules quelques
visites de cache/témoins varient, avec au plus 0,51 % de CPU. Chaque
morceau reconstruit sa propre tour : ces sommes ne dénombrent pas les
arêtes traversantes et ne prouvent aucune borne sous-quadratique globale.
Le repère `F/n²`, avec extrémités incluses, rend visible la différence
entre les axes : sur le plein K5 il vaut **0,038895→0,033854→0,036760**
aux densités 1/4→1/2→entière, contre **0,112026→0,086942→0,082382**
à K10. Sur les 14 liens de densité à secteur fixe, **2 à K5 et 1 à K10**
franchissent une pente de 2 ; sur les 18 liens spatiaux parent→enfant,
**7 à K5 et 3 à K10** la franchissent. À densité entière,
`ΣF(demis)/F(plein)` vaut **0,315/0,425** à K5/K10, sous le repère
homogène `Σ(n_demi/n)²≈0,500` ; pour les quarts, **0,309/0,381** sont
au-dessus de `Σ(n_quart/n)²≈0,250`. Le diagnostic varie donc avec la
façon de doubler les points ; aucun exposant unique ne résume ces coupes.
En suivant **la densité globale** dans chaque famille de tours recalculées,
les pentes finies de `ΣF` pour les deux moitiés sont **1,420/1,653**
à K5 et **1,667/1,434** à K10 ; celles des quatre quarts sont
**1,737/1,848** à K5 et **1,582/1,766** à K10. Les pentes du plein
sont respectivement **1,800/2,119** et **1,634/1,922**. Les effectifs
globaux sont les mêmes `30 847→61 694→123 389` à chaque comparaison ;
`F` inclut les deux extrémités par charge. Ces sommes mesurent des
calculs **séparés** et omettent les incidences entre secteurs : elles
montrent où étudier la croissance, sans fournir un algorithme exact
pour la trame entière ni une borne asymptotique.
La [contre-épreuve multi-graine du quart brut chaud](lidar_raw_hot_quarter_multiseed_20260923/README.md)
garde `x≥0,y<0` physique de 08/000000 et les mêmes octets u18 du plein.
Avec deux autres décimations globales emboîtées, `p_core` K10 sur le lien
1/2→plein vaut **2,029 / 1,971 / 2,095** pour les trois graines ; le
franchissement de 2 varie avec le tirage, la masse par cœur reste
sensible. À K5 sur le premier lien, **2,002 / 1,957 / 1,860**. Les huit
nouvelles sondes restent CPU/W8 locales, `complete_relative`, et ce
seul quart ne représente ni plusieurs séquences ni la trame entière.
Leur lecteur compare pour K5/K10 les **cinq compteurs agrégés** de
chaque ordre K1..5, pas les clés ni la topologie ; le reçu le précise
désormais explicitement.
La [contre-épreuve multi-graine sans sol](lidar_ground_hot_quarter_multiseed_20260923/README.md)
prend le quart **physique** `x≥0,y<0` de 08/000200 après masque entier
figé, sur la grille commune de 1 mm. À K10, pour trois décimations
globales emboîtées, les pentes de **toutes les formes calculées**
`1/4→1/2` valent **2,046 / 2,077 / 2,025**, puis `1/2→plein`
**2,035 / 1,916 / 2,048** ; le plein physique a 14 828 sites.
Les paires développées restent sous 1,75 et les pentes CPU de chaîne
sous 1,43. Le lecteur LIVE normal/`-O` et les 13 empreintes passent.
Le tirage sans sol porte sur les seuls sites retenus après masque, alors
que le tirage brut classe tous les retours avant masque : la même graine
ne rend pas leurs niveaux réduits appariés. Leurs pentes se comparent
comme deux régimes, pas comme une ablation causale du sol. Un quart de
chaque régime et trois graines ne prouvent ni une fréquence LiDAR ni une
borne asymptotique, et les chronos ne couvrent pas la segmentation.
La forme du cœur n'est pas le seul travail volumineux. Sur le plein brut
K5 aux trois densités, `dead_uniform_tests` compte
165,153→467,564→1 459,833 M tests (pentes 1,501/1,643),
`atlas_node_visits` 110,134→298,006→828,142 M visites
(1,436/1,475) et `atlas_point_tests` 91,532→241,258→654,215 M
(1,398/1,439), face aux 37,010→128,853→559,662 M formes du cœur.
Au plein K10, ces trois compteurs atteignent respectivement
4,376/3,529/2,857 milliards. Ce sont des unités de travail différentes,
parfois recouvrantes, et non des temps à additionner. Une réduction de
`core_sites` doit être évaluée avec ces visites, le cover, le catalogue et
la tour ; un profilage par étape est nécessaire pour classer les gains.
La contre-vérification B du complément retrouve **52/52 SHA**, les 18
cas K10 gardés sur 24 essais et les 21 lignes de la matrice ; les cinq
premiers ordres K10 égalent les **comptes** K5 sur chaque entrée, pas
les clés une à une. Sa mention de float32 comme défaut a été rectifiée :
la grille 1 mm reste le profil contractuel prioritaire de v9.
Le [panel S2 CPU des demi-scènes brutes K5](s2_half_density_k5_20260923/README.md)
apparie moteur et lot sur neuf entrées (plein et deux demis × trois
densités), 18/18 sorties `complete_relative`. Six compteurs, dont formes
réellement calculées, charges et paires développées, égalent exactement
le reçu v12 sur **9/9** entrées : S2 n'a pas réduit cette masse ici.
La somme des formes des deux demis vaut **0,567→0,435→0,315** de celle
du plein lorsque la densité passe de 1/4 à 1/2 puis entière, alors que
la part des charges reste vers **0,97**. Les temps muraux sont bruités ;
aucune pente K10 S2 ou borne sous-quadratique nouvelle n'est acquise.
Le plan G4 v18 exécuté en [R13](CONTRE_AUDIT_B_G4_R13_S3_20260923.md) comporte 18 cas
issus des trois trames **sans sol** 00/01/02 de la seule séquence 08,
aux K5/K10 et à quelques bras d'attribution. Il ne rejoue **aucune**
trame brute avec sol, demi-scène, quart ni fraction de densité : il ne
confirme donc pas sur G4 les deux axes de croissance ci-dessus. Les
préflights S3 device par arête sont passés ; garder ce diagnostic
spatial/densité comme campagne distincte du contrat de trame entière,
avec les mêmes IDs emboîtés et les coûts S2+cœur+cover+catalogue.
Le [crédit exact par nœuds du certificat de cœur](../receipts/dead_node_credit_negative_20260923/README.md)
a été essayé hors produit : mêmes voies et digest, mais CPU de chaîne
**+27 % à K5 et +32 % à K10** sur la coupe 16k de 000000 ; cette variante
est fermée. La réduction des paires longues avant le cœur reste ouverte.
Le [shadow du préfixe paresseux](lazy_prefix_dead_core_20260923/README.md)
quantifie désormais **43,42 % de suffixe non consulté** au plein brut K5,
sans gain CPU intégré. Pour choisir une autre voie sans déplacer le coût,
mesurer par worker taille du cœur, masques q3/q4 avant/après preuve,
formes réellement chargées, coût des tests et du cover aval.

Le [reçu local de pente LiDAR v11](CONTRE_AUDIT_PENTE_LIDAR_LOCALE_PARTIELLE_20260923.md)
est intègre (15/15 SHA et entrées vérifiées) mais partiel : une seule
trame sans sol, K5 aux tailles 8k/16k/32k et K10 seulement à 8k/16k.
Les chronos internes K5 croissent d'environ ×2 à chaque doublement,
mais `core_sites` croît ×7,66 au premier ; à K10, ×5,69 de 8k à 16k.
Ce signal ne qualifie ni le travail total sous-quadratique, ni G4/GPU.
Le lecteur de `06f71037` corrige les quatre omissions du runner v2 : FNV
recalculé sur les octets fournis, format, grille 1 mm et threads statiques
vérifiés ; 25/25 mutations sont refusées sous Python normal et `-O`.
L'[addendum de revalidation](../receipts/lidar_scaling_local_20260923_revalidation/README.md)
retrouve les 60 cas archivés, sans nouveau calcul HGP. Les anciens JSON
gardent `grid=unspecified` dans la sonde ; la grille 1 mm est attestée par
les manifestes. `4b6e3aa6` lie désormais le nom du résumé à scène/K/s/W/
répétition et K/s/W de chaque commande au résumé ; il réancre les chemins
historiques du selftest (27/27), sans nouvelle sonde HGP. `--revalidate` ne
recoupe pas encore `record.case`, le chemin d'entrée dans `argv[0]` ni les
empreintes/commit du résumé, et un sous-ensemble non vide de groupes peut
retourner code 0. Ne pas employer
ce seul code de sortie comme preuve de couverture de la campagne entière.

Le [shadow de scission des rectangles q3/q4](Q34_BLOCS_LIDAR_SHADOW_20260923.md)
sur 08/000000 sans sol ferme 3,68 M paires à K5 et 4,12 M à K10 avant
leur filtre individuel, mais paie 40,9 M et 62,5 M visites supplémentaires
de nœuds témoins. Ces paires auraient déjà été rejetées par le filtre
individuel : aucun cover aval n'est économisé. Ne pas porter cette scission
naïve sans bilan apparié du travail complet ; le cache de témoins par boîte
reste une hypothèse distincte. La contrelecture indépendante juge les
certificats exacts sur la grille 1 mm, mais le reçu ne conserve pas les
sorties brutes de cette sonde pour un rejeu indépendant.

Pour q3, seuls les fragments d'atlas **complets** fournissent un compte
réutilisable ; un certificat profond incomplet n'est qu'un minorant.
Partager les graines d'une cellule exige un ticket possédé
`(X,compte,curseur Z)`, la recollecte de **toute** la coquille et un
relais EOF valide même en présence de contacts. Les 171 444 arêtes q3
seules du reçu v8 1 mm ne bénéficient pas du fragment q4. Des
[oracles u18](check_q3_shared_u18_20260922.py) couvrent les cas
ambigus ; la [note q3](Q3_STRUCTURE_ET_BORNES.md) et la
[contrelecture feuille](CONTRE_AUDIT_B_Q3_FEUILLE_WIP_20260923.md)
donnent aussi la descente rationnelle aux coupures dyadiques et une
palette privée de témoins. L'essai feuille non apparié augmentait
fortement les tests ponctuels q3 : mesurer rejets tardifs, fragments
`Leaf`/`Deep`, coquilles et octets avant de porter un collectif.

Pour q4, la [note de structure](Q4_STRUCTURE_ET_BORNES.md) et les
[niveaux orientés](CONTRE_AUDIT_B_Q4_NIVEAUX_ORIENTES_20260922.md)
bornent localement les centres peu profonds et proposent une sélection
de racines exactes. Ni construction/census global ni coût de sortie
sous-quadratique n'en découlent : les incidences site–cover dépassent déjà
`n²` sur la trame 1 mm, donc lire tous les covers déplacerait le coût.
Les contacts, le propriétaire et `centre∈conv(coquille)` restent
obligatoires. Une q4 peut survivre quand toutes ses faces q3 sont
rejetées : ne pas limiter ses graines aux q3 finalement émises
([fixture B](CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md)). La
[revue d'induction de l'atlas](Q4_INDUCTION_ATLAS_EVENEMENTS_20260923.md)
explicite, pour une arête propriétaire q4 déjà transmise avec cover complet,
pourquoi le centre positif échappe aux refus `Outside`/`Deep` et atteint un
groupe émis, même si les faces q3 sont rejetées. La
[preuve amont q3/q4](Q34_PROPRIETAIRE_PASSAGE_AMONT_20260923.md) établit
conditionnellement que les filtres du front, des rectangles, des paires et
du cœur ne perdent pas l'arête propriétaire d'un support positif admissible ;
elle ne borne pas le coût global. Le
[gate de flux entier à douze sites](q4_global_12sites_20260923/README.md)
compare **108 sorties complètes** à l'oracle rationnel sur deux permutations,
`s=8/10/12`, `Local28`/`Window30` et mono/W1/W4. Il ferme cette fixture
précise, pas la preuve des filtres WSPD ni la complétude globale d'une trame.
Un rejeu indépendant sans réécrire le reçu, sur les mêmes SHA de
bibliothèque et de gate produit, reproduit les **108/108** flux et
**3 580** contrôles ; le HEAD local a avancé à `28f0c284` sans modifier
ces dépendances.
La [porte q4 à profondeur non nulle](q4_depth_ladder_20260923/README.md)
étend la fixture aux seuils K4/K5/K10 : la q4 cible a 1/2/7 sites
intérieurs alors que chacune de ses quatre faces q3 atteint exactement
le seuil de rejet 3/4/9. Sur six nuages adversariaux de 13/14/19 sites,
**76 flux complets** mono et parallèles égalent un oracle rationnel,
avec clés, supports, profondeurs et coquilles. Cette porte teste le
cas « q4 admise sans face q3 admise » à plusieurs profondeurs ; elle ne
prouve ni l'induction générale ni la complétude des scènes LiDAR.
Le [cover commun par blocs](CONTRE_AUDIT_B_COVER_BATCH_20260923.md)
et la [saturation K−2 des seules arêtes q4](SEUIL_SATURATION_ATLAS_PAR_VOIE_20260923.md)
restent des pistes secondaires à mesurer avec le coût aval complet.

## FULL, sortie explicite et résidence

`684d8fc7` construit les ordres K en parallèle, publie les images
verticales après leurs lots et déplace les populations vers une banque
partagée. `84c74a5e` conserve le travail payé en cas d'échec et choisit
le plus petit K en échec entre les phases A/C ; deux mutants ciblés sont
tués. R5/R6/R7b exécutent ensuite cette voie sur G4, sans l'isoler de
tous les autres changements. Une phase de préparation/tri
peut garder deux buffers de requêtes coexistants, et les dix ordres
gardent simultanément leurs états : demander RSS et capacités **au même
instant**, par phase et K. Voir l'[audit des ordres
parallèles](PREFETCH_GEOMETRIE_FULL_PAR_K_20260923.md) et sa
[contrelecture B](CONTRE_AUDIT_B_FULL_PARALLELE_WIP_20260923.md).

La sortie explicite est déjà un verrou matériel. R5/K10 publie
**5,95–7,47 millions de nœuds**, autant moins dix références de parents
et **3,55–4,47 millions de contributions** par trame. À l'ABI courant,
les seuls tableaux nœuds/parents/contributions/successeurs/images
verticales représentent **770–968 Mio** retenus, hors banque de
populations, catalogue et capacités. Le catalogue compte
**4,38–5,51 millions de boules** et **0,982–1,235 Go décimaux** ;
le RSS K10 est **3,8–4,8 Gio**. Le [calcul de
résidence](CONTRE_AUDIT_B_RESIDENCE_CHAINE_20260922.md) établit déjà
`216n` octets publiés à K1 ; le cache direct
`48·nextpow2(16n)` appartient au résolveur séquentiel et n'est pas
configuré sur la voie statique R5. Une sortie exacte adressable par
fenêtres, CSR/arène ou externe, avec coût d'export et identité conservée,
est requise avant de promettre des dizaines de millions de points.
La [borne R5 détaillée](CONTRAT_COUTS_ET_PARALLELISATION.md) n'est ni une
borne asymptotique sur d'autres LiDAR ni un RSS de phase isolé.

Dans le meilleur cas R7b ON, 08/000100/K10, q3/q4 prend **5,226 s**,
FULL **3,199 s** et les autres postes de chaîne **1,154 s**. Rendre
q3/q4 et FULL gratuits sur ce chemin séquentiel mesuré laisserait donc
encore 1,154 s, sans borner un nouvel algorithme ni un chevauchement GPU.
`50690c12` isole le digest dans `times_ms.digest` ; il reste
**synchrone** dans l'appel public.
La destruction des temporaires déclarés dans le `try` est comprise
dans `chain_total` ; celle du résultat retourné intervient après l'appel.
Les présentations ont seulement 2–13 doublons pour 4,38–5,51 M clés
sur ces trames ; ce ratio ne se transfère pas à des passages LiDAR
superposés. Le catalogue arrive déjà strictement trié par `BallKey`
dans l'appel de chaîne. `50690c12` le certifie par balayage et évite
le tri redondant de `by_key` ; l'API générale trie toujours un catalogue
non ordonné et le tri distinct `by_level` reste nécessaire. Mesurer le
gain FULL et le pic de résidence simultanée. L'[analyse FULL](CONTRE_AUDIT_B_FULL_COUTS_ET_INTERFACES_20260922.md)
et la [piste de préfixe d'intrus](INTRUS_FULL_PREFIXE_EXACT_20260923.md)
documentent les autres postes ; le préfixe ne mérite un cache qu'après
mesure des répétitions par clé et worker.

`458fb0ed` porte désormais `anchor_meb_proposed` : la proposition Welzl
flottante est vérifiée par les formes et puissances entières, puis le
support de référence est repris sur le bord exact. Le gate différentiel
juge **28 956 ensembles** et tue le mutant sans canonisation ; le port
rapporte localement **154 → 70 Gcycles MEB** et **24,7 → 18,9 s** pour
la tour 08/000000/K10/W8 ; R7b mesure ensuite l'ablation de sa version
`8e8b83a3` sur G4. Une
[contre-épreuve FENV](check_meb_proposed_fenv_20260923.cpp) indépendante,
compilée `-O2 -frounding-math -fno-fast-math` contre le header publié
(SHA-256 `de54655393b09182…`), compare encore **42 544** cas sous
quatre arrondis et FTZ/DAZ activés ou non, sans divergence de
clé, niveau, support, coquille ou statut ; ce sidecar ne remplace pas
une porte FENV intégrée. Voir la [preuve du support
canonique](MEB_PROPOSITION_EXACTE_20260923.md). `78e94b04`
publie maintenant les compteurs `proposals/verified/canonical/fallbacks`
dans le JSON et un levier `tower_meb_proposal` ON/OFF. Le nouveau
préflight exige une proposition vérifiée quand ce levier est actif ;
R7b en donne désormais l'ablation G4, résumée plus haut, sur le paquet
`8e8b83a3`. L'ordre Welzl inverse
`power_order` dans le paquet `8e8b83a3`, alors que la récursion insère
dans l'ordre du tableau. `8fa03046` passe à une proposition
move-to-front avec les extrêmes en tête : la coordination rapporte
**43,1 → 15,8 Gcycles** locaux pour Welzl seul, sans reçu G4 sur ce
nouveau code. La [contre-épreuve FENV
actualisée](MEB_PROPOSITION_EXACTE_20260923.md) compare **42 544** cas
au header publié sous quatre arrondis et FTZ/DAZ, zéro divergence ;
aucune borne de coût générale ni garantie « espérée linéaire » ne suit
d'un ordre déterministe sur les facettes.

`02d55856` remplace les recherches binaires de clé FULL par une table
exacte à adressage ouvert, construite en parallèle puis lue après jonction.
Les collisions sont départagées par la clé entière ; aucune case occupée
n'est effacée. À 5,51 M boules, la capacité de 16 777 216 identifiants
`u32` ajoute **64 Mio** à `by_key`, toujours conservé. L'essai local
alterné 08/000000/K10/W8 passe de **18,5/17,7 s à 16,9/16,4 s** pour la
tour, avec digest égal ; la
[contrelecture indépendante](CONTRE_AUDIT_B_INDEX_CLES_FULL_20260923.md)
rejoue quatre portes ciblées, dont le mutant « clé absente » tué
causalement. Une seconde archive complète du pin `02d55856` inscrit
**128** tests `gate` et passe les portes FULL séquentielle et statique
même avec hachage forcé constant ; le mutant échoue. Le **127/127**
annoncé dans la coordination n'est pas le décompte de cette archive
complète. `ec6d1b74` vide désormais `key_slots` au début de
`Builder::finish()`, après la dernière recherche et les jonctions :
**64 Mio** de stockage logique sont libérés à 5,51 M boules avant banque
et encodage ; l'effet RSS réel reste à mesurer. R7b exécute le paquet
antérieur et ne mesure pas ce changement. Publier construction, sondes réussies et
absentes, longueurs de chaînes, RSS de pointe et ablations W1/W48 :
à plusieurs dizaines de millions de points, le coût total et la
résidence décident de la pertinence de cette table.

`75f27eee` remplace dans FULL la fusion série du tri des requêtes par
des seaux répartis et triés en parallèle, et parallélise concaténation
et détection des groupes. La [contrelecture du tri
FULL](CONTRE_AUDIT_B_SAMPLE_SORT_PUBLIE_20260923.md) confirme l'ordre
total, **400/400** cas en Release et la porte Clang ASan/UBSan, avec
mutant tué ; R7b exécute ce port sans ablation du tri. Le prélèvement à
positions fixes ne garantit pas un partage utile : un témoin W48
strict de 200 003 clés met **96,94 %** des éléments dans un seul seau.
Le nombre de workers créés ne borne donc pas le temps du plus gros
tri. `ec6d1b74` remplace les positions périodiques par un tirage
pseudo-aléatoire déterministe et ajoute ce témoin à la porte d'identité ;
il ne publie pas encore l'occupation des seaux ni un gain G4 apparié.
Le risque de déséquilibre en pire cas subsiste sans borne de taille
des seaux. Publier tailles non vides/maximales des seaux et CPU/mur par étape
sur les requêtes FULL LiDAR, W1/W8/W48, avec RSS réel : le second
tampon, `bucket_of` et les offsets coexistent au-delà du seul
`2×capacity×sizeof(Request)` annoncé pour les requêtes.
Une contre-épreuve isolée du paquet périodique `8e8b83a3` sur
08/000000 entier sans sol K10/s8 ne voit **aucun** déséquilibre massif :
les neuf tris de requêtes ont 32/32 seaux utiles à W8 et 192/192 à
W48, maxima **1,27–1,70×** la moyenne ; les objets et digests sont
égaux. Cette mesure locale n'est ni un reçu G4 ni une mesure des
séparateurs pseudo-aléatoires de `ec6d1b74`.

Le [préflight B du brouillon plat FULL](CONTRE_AUDIT_B_BROUILLON_PLAT_FULL_WIP_20260923.md)
sur le commit local historique `f93dc1659` trouve une frontière
publique à fermer : les offsets CSR du nouveau `FullCoverageFlatDraft`
sont lus avant toute validation de forme. Un `level` non vide avec
`batch_begin` trop court suffit à sortir du contrat de refus typé ;
offsets décroissants ou hors plage posent le même problème. Le producteur
statique paraît préserver l'ordre, mais aucune nouvelle porte directe
vectoriel/plat, forme invalide, RSS/allocation ou chrono G4 n'est jointe
au port. Les lots groupés allouent encore une action temporaire.
Le port est publié depuis **`092b1d86a`** ; la même surcharge publique
est encore lisible sans contrôle préalable des offsets à la date de cette
revue. Le [micro-test ASan/UBSan](flat_draft_invalid_probe.cpp) confirme
maintenant un **heap-buffer-overflow exécuté** dans `FlatDraftSource::actions`
sur une banque valide et un CSR public invalide. Cela n'est pas une erreur
géométrique démontrée sur la sortie interne valide ; la surcharge publique
doit refuser la forme avant tout parcours.
La [relecture du correctif CSR sur sources mutables](RELECTURE_CORRECTIF_CSR_PLAT_WIP_20260923.md)
constate que le constructeur valide désormais les trois tableaux d'offsets
**avant** ce premier accès. Le micro-test causal de B et la porte modifiée
passent sous GCC 13.3 ASan/UBSan ; le micro-test revient à
`kInvalidInput/coverage_flat_draft_shape`. C'est une correction positive
de la frontière publique, d'abord relue dans un worktree mutable puis
**publiée sur `main` par `3765080cf`**. Les tests de cette relecture ne
sont pas un rejeu du commit ni une qualification FULL/G4 dédiée. Le gate
compare seulement les **tailles** des
arènes vectorielle/plate et ne faute pas la surcharge plate : compléter
l'égalité champ par champ et les pannes d'allocation de cette voie.
Le [reçu plat local](CONTRE_AUDIT_B_RECU_BROUILLON_PLAT_LOCAL_20260923.md)
du commit développeur `5f36d5536` passe 11/11 SHA pour cinq couples
sur la seule trame sans sol 08/000000, W8/s8. À digest FULL, comptes
d'ordres et travail FULL égaux, la tour K5 gagne 1,7–4,3 % ; une
seule paire K10 exploitable passe de 14,705 à 11,327 s. Les résultats
sont `complete_relative`, sans catalogue clé par clé ni payload FULL
archivé. L'affirmation 152/152 portes n'est pas accompagnée du log,
les binaires n'ont que des préfixes SHA et aucune commande n'est
épinglée. Le défaut de forme CSR subsiste dans **ce paquet historique** ;
le correctif a été publié ensuite par `3765080cf`. Aucune ablation G4
propre au chemin plat n'est publiée.

La [réduction de la phase A en graphe
temporel](PHASE_A_GRAPHE_TEMPOREL_20260923.md) retrouve exactement
composantes, parents et IDs des lots par coupes de niveau, si les égalités
sont fermées ensemble. Elle ouvre une voie de composantes parallèles dans
un ordre K, sans coût ni gain prouvé. La sonde locale 08/000000/K10
compte seulement **1,72 %** des blocs dans les lots groupés : le seul
parallélisme *au sein d'un lot* toucherait donc peu de blocs sur cette
trame, sans que cette fraction mesure sa part des cycles. Une variante
à têtes physiques garde les ancres historiques, mais son ablation
appariée sur le même catalogue rend la même tour octet par octet sans
gain stable et avec **54–159 Mio** de RSS supplémentaire : ne pas la
porter sur ce seul régime.

Le [lemme du maximum d'ID](PHASE_A_MAX_ID_COMPOSANTE_20260923.md) donne
une reconstruction hors des barrières de niveau : dans une composante
au seuil ouvert, la racine canonique est le plus grand ID de création
qu'elle contient. Quatre fixtures et 3 000 historiques abstraits
reproduisent groupes, parents et IDs, mais ne testent pas le produit.
La construction quasi linéaire d'une hiérarchie de composantes pondérée,
avec requêtes aux seuils ouvert/fermé, reste le verrou avant tout port.

Le [rapport C sur les alternatives](AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md)
propose D5, une tour FULL maigre (jointure des selles, saut au centre,
phase A sans allocations, sortie compacte). Sa
[contrelecture B](CONTRE_AUDIT_B_D5_FULL_MAIGRE_20260923.md) juge la
réduction de MEB et les racines de facettes locales prometteuses, mais
**×6–13 sur FULL reste une projection**, pas une tour G4 appariée : le
sidecar n'émet pas le payload complet et omet du chrono certaines
préparations. R8/K10 garde déjà **1,05–1,24 s** dans la queue
validation/populations/images/banque/encodage seule. Avant un port,
corriger la règle 0 du saut, le tampon 13 racines face à la fixture
32 racines, et l'ordre de programme des contributions compactes ; puis
comparer l'expansion octet par octet et mesurer chaque sous-phase. Un
catalogue « scellé » peut éviter une validation redondante seulement
si la chaîne certifie **toutes** les clés émises et conserve ses preuves ;
il ne prouve pas les clés entièrement manquantes.
Le futur callback D5 a trois verrous concrets
([lecture B du code](CONTRE_AUDIT_B_D5_FULL_MAIGRE_20260923.md)) :
sa garde actuelle ne lie pas facette et **composante** cible, elle exige
les compteurs de l'ancien résolveur d'intrus, et sa présence désactive
le parallélisme entre ordres K. Le produit actuel ne branche **aucun**
callback externe et applique déjà la règle 0 : ce ne sont pas des bugs
de la tour publiée. Un shadow par facette comparant la racine au seuil
ouvert pré-lot, sans remplacer la cible, doit précéder le port.
Le [premier port borné D5](../receipts/saddle_index_negative_20260923/README.md)
est **clos négativement** et retiré du produit : sur 08/000200 sans sol
16k/K10/W8, la jointure exacte des selles évite 1,012 M MEB, mais construit
et trie 10,188 M entrées. À digest et ordres égaux, la phase 0 passe de
2 582 à 2 752 ms et la tour de 3 990 à 4 029 ms (CPU local partagé,
un cas). La [contrelecture B](CONTRE_AUDIT_B_INDEX_SELLES_NEGATIF_20260923.md)
précise que le juge de cibles hit par hit n'est pas capturé dans les deux
sorties chronométrées : celles-ci ne prouvent pas l'égalité du payload
complet. Le prochain essai D5 doit réduire le coût de l'index ou coupler
la jointure au **saut au centre avec règle 0**, et mesurer les recherches
d'intrus ainsi que la racine pré-lot par facette. Aucun gain de cette
tranche n'est transférable à une trame entière G4.
La [contrelecture des preuves D5](CONTRELEC_D5_NAISSANCE_ET_PORTE_E1_20260923.md)
ferme le cas de borne basse omis dans la preuve du lemme C et donne une
fixture à quatre sites où la porte E1 accepte une cible dans la mauvaise
composante. Comparer la racine pré-lot **par facette** au produit, ou
certifier le saut par le témoin central, avant de compacter la sortie.
La [contre-épreuve du k-NN de D5](d5_knn_aabb_counterexample_20260923/README.md)
rectifie la borne de coût du plan de saut : sur une entrée u18/K5
géométriquement non terminale avec **sept sites dans la boule fermée**,
les préfixes ajoutant 8/16/32/64 sites extérieurs font visiter
**29/45/77/141 nœuds sur autant**, car leurs boîtes AABB croisent la
zone du seuil final. La famille géométrique donne un pire cas Ω(n)
pour cette recherche, indépendamment des sites fermés ; aucune facette
effectivement émise par FULL n'est démontrée par la sonde. Remplacer
la projection `O(|D̄∩P|)` par le compte de nœuds admissibles, puis
mesurer `tree_nodes` et les feuilles sur les coupes/densités LiDAR.
La [contrelecture des chiffres et verdicts
C](CONTRE_AUDIT_B_ALTERNATIVES_C_20260923.md) précise que les 66–70 %
d'ancres longues proviennent des fenêtres arête/rectangle d'une **seule
sonde locale** 08/000200/K5/W1, non du CPU q3/q4 G4 ; les 5,9 % portent
sur les seules émissions q3+q4. Après correction modélisée de l'horloge,
le critère secondaire D3 est satisfait sur 5/7 coupes, pas 7/7 ; les
parts K10 de trame entière sont estimées. Les probabilités et verdicts
« K10 hors de portée » ou « émission par niveau nécessaire à 100 ms »
restent des jugements conditionnels, jamais des impossibilités prouvées.
Le contrat brut entier multi-séquence et le coût total sans sol demeurent.

## Portes de preuve encore ouvertes

Le lecteur des reçus a progressé : `e5688680` impose les identités
physiques des voies, du cache, des coquilles et du cœur ; `cc4664e5`
lie la marque et le calendrier archivés aux **valeurs exactes** déjà
vérifiées par l'hôte. `a5872918` répare les appels du selftest à cette
nouvelle interface. Sur contenu figé, les **21/21 selftests** passent en
Python normal puis sous `-O`, avec les mutations de date future et de
calendrier plausible refusées. Avant `78ce9fd4` et sa réception, ces portes
hors GCP ne certifiaient pas encore R6 ; la contrelecture des sorties
brutes R5 restait positive et R2 demeure refusé. Les [contre-fixtures
v5](CONTRE_AUDIT_B_G4_RECEPTION_V5_20260923.md), les [identités
v6](RECEPTION_V6_IDENTITES_MANQUANTES_20260923.md) et la
[relecture v8](CONTRE_AUDIT_B_RECEPTION_V8_GARDES_WIP_20260923.md)
gardent l'historique sans être des défauts du lecteur publié courant.

`78ce9fd4` ferme les deux derniers écarts protocolaires suivis : le
premier cas doit activer les cinq leviers que le préflight exercera,
et le lecteur contrôle aussi le résumé exact de chaque cas tué dans
une réception `partial`. Un fichier absent rend un refus typé. Sur
ce commit figé, **21/21 selftests normal et 21/21 sous `-O`** passent,
dont les mutations ON/OFF de plan et suppression/altération du résumé.
R6 a ensuite apporté cette preuve de réception pour le snapshot.

`50690c12` remplace la fusion série par un **sample-sort parallèle** :
tris des slots, splitters de clés, puis tris de plages possédées ; une
clé de frontière entière va dans la même plage par `lower_bound`.
Ce n'est **pas** le `parallel_sort(all)` à tampon unique d'un chantier
mutable précédent. Les slots et plages restent simultanément résidents
durant la collecte ; mesurer le maximum par plage, la crête RSS, le temps
et les échecs de ressources sur le chemin de chaîne. Le fast-path FULL
rejuge strictement toutes les clés. Les portes locales passent, sans
reçu G4 ni ablation FULL de ces changements. Le [contre-audit du
sample-sort publié](CONTRE_AUDIT_B_SAMPLE_SORT_PUBLIE_20260923.md)
sépare ce chemin du tri global WIP abandonné avant publication.
Le prélèvement de splitters prend jusqu'à `16×4W` clés **par slot non
vide**, sans pondérer par sa taille : une répartition très inégale des
présentations peut laisser une plage beaucoup plus grosse que la
moyenne malgré `presentation_ranges>1`. Un mutant compilé produit deux plages,
dont une vide et l'autre contenant tout, et passe encore la porte :
ce compteur n'est pas un plancher de parallélisme utile. Publier le
nombre de plages non vides, le maximum et
la distribution des tailles de plages sur LiDAR ; comparer, si ce
déséquilibre apparaît, un échantillon pondéré ou une partition en deux
passes suivie d'un seul tri par plage. L'ordre exact ne dépend pas de
la qualité des splitters, seul le coût en dépend.
Le [shadow de distribution](AUDIT_DISTRIBUTION_SAMPLE_SORT_20260923.md)
mesure sur 08/000000 sans sol K5/W8 **32/32 plages non vides** et un
maximum de **1,52 à 1,59 fois la moyenne** malgré des slots déséquilibrés
par un facteur 5,40 : le déséquilibre redouté n'est donc pas observé
sur cette trame. Il révèle en revanche le coût du double tri : avec les
**mêmes** séparateurs et une suite finale de 1 306 699 présentations
égale élément par élément, l'histogramme/scatter des slots bruts suivi
d'un seul tri par plage enlève **31 077 584 comparaisons complètes**, au
prix de **6 533 495 comparaisons de clés** de classification et d'un
tampon/scatter. Le choix d'un échantillon global pondéré reste non
mesuré ; les comparateurs ne sont pas un temps G4 ou un gain FULL.
Pour comparer un futur reçu à R6, ajouter `times_ms.digest` à
`chain_total` sur le périmètre mural ancien. Le nouveau `chain_cpu_s`
exclut également le condensé, mais aucun `digest_cpu_s` n'est publié :
une comparaison CPU·s R6/R7 brute serait trompeuse. `6200bb5a` borne
désormais `read + chain_total + digest` par le mur externe et tue la
mutation qui plaçait `read=3 600 000 ms` sous un mur externe de 60 s.
Il classe `std::length_error` et `std::system_error` comme manque de
ressources, chronomètre fusion et recensus même en cas d'échec et vide
les résumés d'ordres sur refus ; la porte de chaîne exerce une panne de
lancement après q3/q4. Ce correctif n'a pas de reçu G4.

Prochaines mesures : mêmes octets et masque figé, trames **entières** de
plusieurs séquences sans sol puis brutes, s8/10/12, K5 et K10, W1/W24/W48,
profil float32 et grille fine **séparés**. Les sept morceaux spatiaux
1 mm du [reçu v8 LiDAR](../../morsehgp3D_v8/receipts/lidar_ground_20260921/README.md)
ont maintenant des chronos v12 locaux sur les trois scènes sans sol de
la séquence 08. La décimation emboîtée 1/4–1/2–1 est mesurée sur les
sept secteurs de ces trois scènes sans sol à K5/K10 et sur les sept
secteurs physiques de la trame brute 08/000000 à K5/K10. Répéter sur
d'autres graines et séquences.
Ni les morceaux ni les décimations ne valident le contrat de trame entière.
Publier travail amont,
formes et atlas, candidats
résiduels, coquilles, catalogue, sorties FULL, CPU/mur et RSS par phase,
y compris les échecs et les replis exacts. R15 exécute déjà une **tour
entière hybride G4** (S2/S3/S4a sur GPU, q4 et FULL sur CPU), avec
transferts et retours CPU exacts, jugée
`complete_relative` sur trois trames sans sol de la seule séquence 08.
La session à buffers résidents, le profil brut float32 par défaut, les
autres séquences et le seuil d'une seconde restent à construire ou à
qualifier. Verdict public : **`not_claimed`**.
