# Contre-audit B — préflight R21/v26, budget 100 ms explicite

24 septembre 2026. Bases examinées : v26 publiée `09885f163` ; plan brut
local WIP `61cfba666`. Aucun reçu G4 R21 n'était publié lors de cette
lecture. Le reçu R20 demeure le dernier résultat G4 : 18/18 tours relatifs,
mais 1,010–1,260 s à K5 sans sol sur trois trames de la seule séquence 08.
Le contrat de 100 ms porte sur la **tour FULL explicite**, nœuds, parents
et liens déjà matérialisés avant l'arrêt du chronomètre ; ni digest seul ni
sortie compacte à développer ensuite ne suffisent.

## Défaut causal du contrôle de mur externe

`bench/tower_probe.cpp` lit la trame, ouvre facultativement la session GPU,
exécute la chaîne, puis calcule les condensés. La sonde publie
`device_session.context_ms` et `reserve_ms` hors de `chain_total`, ce qui
est correct pour la latence chaude conditionnelle. Mais
`gcp-migration/tower_worker_v9.py:1087–1096`,
`validate_external_wall`, ne confronte au mur GNU que
`read + chain_total + digest + catalogue_digest`. Elle omet les deux
intervalles de session, pourtant séquentiels dans ce même processus.

Reproduction indépendante sur la version publiée : une valeur avec
`read=0,25 ms`, `chain_total=0,5 ms`, `digest=0,125 ms`,
`catalogue_digest=0`, `context_ms=1 000 000 ms` et
`reserve_ms=1 000 000 ms` est **acceptée** avec un mur externe de
`0,001875 s`. C'est un défaut d'attribution temporelle, pas une preuve
d'erreur géométrique ou de fraude des reçus R20, où ce levier n'existait
pas. Avant R21, additionner `context_ms + reserve_ms` à la borne du mur
externe, mais **pas** à `chain_total`, et ajouter une mutation causale qui
gonfle seulement ces champs tout en conservant les autres mesures et le
statut complet. La tolérance d'arrondi publiée peut rester inchangée.

## Portée réelle du plan R21 WIP

Le plan `61cfba666` passe de 18 à 30 cas en ajoutant, après les cas sans
sol, trois trames brutes avec sol (08/000000, 000100, 000200), chacune à
K5/K10, bras GPU et jumeau moteur. Les tailles, empreintes et FNV des
trois charges locales ont été recalculés. Ces cas sont utiles, mais restent
à `s=8`, séquence 08 et grille entière 1 mm ; ils ne couvrent pas `s=10/12`,
plusieurs séquences, ni le profil float32 secondaire. Les cas bruts n'ont
pas encore de condensés CPU indépendants épinglés : la comparaison interne
des jumeaux établit une identité relative, pas une référence absolue.
Un mutant qui remplace ensemble les condensés de tour, catalogue et
présentations du cas brut 08/000000/K5 reste accepté par le lecteur
`complete_relative` : la clé correspondante manque bien à
`PINNED_DIGESTS`. Le jumeau détecterait la corruption d'un seul bras,
mais pas une erreur commune. En positif, les nuages sans sol des trois
trames sont des sous-ensembles exacts des entrées brutes correspondantes,
sans site hors brut après la grille commune.
Avec un budget utile de 1 500 s et 600 s au plus par cas, un reçu partiel
est possible ; `validate_received` accepte `partial` dès qu'une seule tour
du plan est achevée, même si les douze cas bruts sont tous reportés.
Publier précisément les cas achevés et les reports/refus ; ne pas annoncer
« brut testé » sur le seul statut global.

Le même commit WIP a encore des scénarios d'autotest codés pour les 18
anciens cas (`tower_selftest_v9.py`, p. ex. `test_budget_exhaustion_skips_following_cases`,
`test_gpu_preflight_alone_is_not_gpu_executed`,
`test_protocol_defect_stops_the_campaign`) alors que le plan en a 30.
Une exécution sur le worktree actif a donné 28 tests dont six erreurs de
scénarios ; comme ce worktree bougeait pendant l'exécution, **ce n'est pas
une qualification figée ni un échec du moteur**. La discordance des
assertions est néanmoins visible dans le source `61cfba666`. Un rejeu
ciblé ultérieur de
`Protocol.test_nominal_session_completed` sur ce WIP échoue en 24,79 s
sur `GPU labels from complete LiDAR towers` (attendu à 18 cas, plan à
30) : la porte nominale est donc réellement rouge, indépendamment de
la première suite interrompue. Corriger les
attendus et rejouer normal puis `-O` sur un SHA gelé avant le prochain G4.

Le bras dit « chaud » de R21 ouvre encore un **nouveau processus pour
chaque cas** puis préchauffe avant `chain_total`. Il mesure ainsi une
latence de chaîne conditionnée par la session préouverte, non le débit
d'un unique processus multi-trames persistant. C'est un diagnostic
valable s'il est nommé comme tel ; il faut un test distinct en boucle
multi-trames dans un même processus pour qualifier le flux 10 Hz.
De plus, `warm_up_lanes` peut retourner un succès avec zéro warp prévu
pour une ardoise, sans l'avoir réservée ; le champ `opened=true` prouve
donc une ouverture CUDA, pas à lui seul la résidence de toutes les
ardoises. Contrôler les tailles réellement réservées et le temps
`gpu_prepare_wait_ms` avant d'attribuer un gain à leur préallocation.

## Chemin critique FULL vers 100 ms

Sur R20/08/000000/K5, FULL vaut 421,5 ms : validation 70,5, phase
statique 198,5, lots 56,5, populations 19,6, images 26,9, banque 17,2
et encodage 32,2 ms. Supprimer gratuitement la seule phase statique
laisserait encore environ 223 ms de FULL ; supprimer le seul tri ne peut
donc pas clore 100 ms. La chaîne hors FULL vaut encore 687 ms sur cette
trame. Le code construit bien `FullBallTowerResult.orders` (forêts,
parents, contributions, nœuds inférieurs) dans `chain_total`, mais la
résolution de certaines facettes et les copies/encodages imposent une
refonte conjointe, pas seulement un meilleur digest.

Le chemin critique actuel cache aussi q2 : la chaîne lance q2 après le
front q3/q4 et le rejoint après les autres travaux q3/q4 ; les 75–115 ms
de q2 K5 mesurés dans R20 sont donc recouverts aujourd'hui. Si les voies
q3/q4 deviennent beaucoup plus courtes, cette limite réapparaîtra.
À 08/000000/K5, les trois noyaux GPU filtre/certificats/voies prennent
environ 64 + 90 + 57 = 211 ms dans leurs passes sérielles actuelles,
sans front, recensement ni FULL. Une mise en flux des rectangles pourrait
chevaucher ces étapes, mais devra préserver la propriété exacte des
rectangles, les masques monotones et les ordinaux de sortie. La seule
superposition des passes ne suffira pas à 100 ms sans réduction du
travail géométrique et refonte de FULL.
Le signal de croissance renforce cette priorité sans prouver une loi
asymptotique : sur 08/000200 sans sol, la sonde 16k→32k/K5 de l'audit
aval S2 multiplie les paires étendues par 4,81 et les incidences
`core_sites` par 8,27, tandis que les supports q3+q4 émis ne sont
multipliés que par 1,83. Le seul nombre de boules de sortie serait donc
un mauvais substitut au travail de génération sur ces régimes finis.
À titre de critère de rejet **conditionnel**, si les 40 449 413
incidences cœur à 16k restaient inchangées, il faudrait ramener les
334 343 514 incidences à 32k sous 161 797 652 pour passer sous le
facteur quatre sur ce doublement : plus de 172,5 millions, soit 51,6 %,
à éviter **avant** de payer les certificats. Un nouvel algorithme qui
change aussi le coût 16k doit évidemment être jugé par ses deux mesures
appariées, non par ce seuil absolu. Décomposer le gonflement en nombre
de cœurs construits (×3,10) et taille moyenne (×2,66), puis tracer les
arêtes communes/nouvelles au doublement ; la seule quantité de sorties
ne discrimine pas ces mécanismes.

Pistes à éprouver dans cet ordre, **sans crédit de gain acquis** :

1. Diminuer exactement les paires/charges du cœur q3/q4 avant émission,
   avec certificats de blocs mesurés en shadow sur rectangles lourds et
   coût total `sélection + preuve + repli + aval` ; distinguer les voies
   q3 et q4, ne créditer le cœur que si les deux sont closes.
   R20/08/000000/K5 compte environ 23,7 M paires pour 1,13 M rectangles
   encore ouverts, soit seulement ~21 paires par rectangle en moyenne :
   **64 tests de coins sur chacun seraient probablement plus coûteux que
   l'expansion**. Un premier minorant exact O(1) par boîte et un seuil de
   masse doivent précéder les coins ; qualifier leur sélectivité réelle.
2. Résoudre les cibles de facettes en lots sur tout K en conservant rangs,
   identités et plateaux canoniques ; mesurer les visites MEB et les
   sorties, pas seulement le temps de tri.
3. Représenter la tour temporelle pour partager la phase statique entre K,
   puis attribuer les premières rencontres par minimum d'ordinal et
   préfixes stables ; tester d'abord contre les vrais catalogues R20.
4. Écrire directement la sortie explicite dans des plages préfixées,
   tout en gardant cette écriture et toute conversion dans `chain_total`.

Attention à `Builder::run()` : le chemin parallèle entre ordres K est
désactivé lorsqu'un `batch_resolver` externe est branché. Un simple
callback GPU peut donc détériorer FULL ; sa parallélisation doit être
vérifiée de bout en bout, avec comparaison exacte des sorties.

### Concurrence imbriquée à mesurer, pas gain acquis

Le reçu R20/08/000000/K5 fixe `workers=48`,
`tower_static_threads=48` et `tower_overlap_static=true`. Dans
`run_orders_overlapped`, cinq fils exécutent les ordres K ; chacun peut
lancer `parallel_items(chunks, 48)` depuis `order_prepare_lean`, tandis
que la phase statique emploie elle aussi jusqu'à 48 fils. Avec
`planned_workers=min(threads,items)`, la **borne architecturale** est
5×48 + 48 + 5 = 293 fils créés, plus le fil pilote, et non un pic
observé. Les tailles
de programmes et le calendrier peuvent la réduire. Aucun reçu R20 ne
mesure le nombre simultané ni l'ablation de cet emboîtement. Avant de
conclure que davantage de CPU aide FULL, instrumenter le pic de fils et
comparer, sur même catalogue et même G4, les couples de largeur
phase-statique/ordres (8, 16, 24, 48), avec trois répétitions, même
empreinte de tour et coûts mur/CPU par phase. Un ordonnanceur commun
borné à 48 fils est une piste si la contention se confirme.

Une lecture ciblée des durées de vie v22–v26 n'a montré ni course ni
utilisation après libération causale : la préparation GPU possède son
index, publie sous mutex/condition et joint son fil ; q2 et les voies
q3/q4 joignent leurs ouvriers avant destruction ; les états FULL sont
privés par K et les runners joints avant lecture des résultats. Ce n'est
pas une preuve exhaustive d'absence de course. Aucun résultat TSan
nouveau n'est revendiqué pour ces tranches ; une porte TSan rejouable
reste utile après correction des gates.

Le chemin d'échec mérite aussi une porte spécifique : dans les chemins
parallèles (avec ou sans overlap), les préparations statiques précèdent
tous les lots ; toute panne statique est donc lancée avant la lecture
des pannes de lots, même si le plus petit K en panne est un lot. Avec
`static K4 + lots K2`, la boucle séquentielle K croissant renvoie
`lots K2`, alors que les deux chemins parallèles renvoient `static K4`. Le gate
`order_failure_priority_gate.cpp` couvre déjà cette paire à quatre fils,
mais ne la compare pas à `static_threads=1` ; son commentaire promet
pourtant la même priorité. Ajouter ce contrôle différentiel. C'est une
discordance de refus, **pas** un faux succès géométrique connu et pas une
explication des 1,1 s R20.

## Portes d'exactitude et autres essais encore dus

Le [catalogue scellé proposé par C](PROPOSITION_C_CATALOGUE_SCELLE_20260924.md)
peut supprimer la passe 1 de FULL **seulement après** avoir transféré
dans la chaîne le test exact de positivité des supports réguliers. La
chaîne recalcule aujourd'hui q3 sans acuité stricte et q4 avec `det>0`
sans tous les poids strictement positifs ; le recensement et Euler ne
remplacent pas ce test. La proposition de C pose explicitement cette
condition. Sur R20/08/000000/K5, la passe 1 ne vaut que 17,9 ms des
70,5 ms de validation et 421,5 ms de FULL : levier exact et intéressant,
mais insuffisant isolément pour 100 ms.

Une garde de domaine paraît aussi manquer au bord des enregistrements
GPU : `gen::check_lanes_batch` vérifie arité, ordre, extrémités et champs
comptables, mais pas visiblement `support[k] < spatial_order.size()` pour
chaque support. `chain::key_and_level` indexe ensuite `points` avec ces
supports avant toute garde de ce type. Les enregistrements normaux sont
construits par le moteur ; le cas signalé est celui d'un résultat de
batch corrompu ou d'un nouveau backend. Il faut un mutant ciblé avec
ASan/UBSan et un refus typé avant d'utiliser ces enregistrements pour
sceller le catalogue. Cette lecture statique n'est pas une corruption
observée sur R20.

Les essais restants, à distinguer des reçus acquis :

| Porte | Dernière preuve pertinente | À produire |
| --- | --- | --- |
| Croissance 8k/16k/32k | six campagnes locales v12, **60 sondes** sans sol/s8, avec plusieurs pentes `core_sites` ≥ 2 ; secteurs bruts v12 sur une trame | refaire sur chemin récent, K5/K10, sans sol puis brut ; compiler travail total et sortie, pas seulement temps mur |
| Séparation WSPD | R8 historique CPU G4 compare s8/10/12 à K5 sur trois trames 08 ; R20/R21 GPU courant s8 seulement | mêmes entrées et sortie exacte à s8/10/12, K5/K10, coût complet G4 et pente locale |
| FULL K5 exact | T2 public recalcule la chaîne à `kmax=10,s=8` sur fixtures ; R20 est `complete_relative` | porte T2 directe `kmax=5`, variations s/W, comparaison structurelle des nœuds, parents, liens et coquilles |
| Régimes LiDAR | R20 sans sol, trois trames de 08 ; R21 brut planifié mais non reçu | plusieurs séquences, trames entières brutes et sans sol, segmentation et coût total séparés |
| Session et capacité | R21 processus par cas ; aucun 10M+ qualifié | flux persistant multi-trames, RSS/VRAM/pagination, adressage et sortie explicite à dizaines de millions |

La grille 1 mm est le profil temporel prioritaire décidé par l'utilisateur ;
le float32 natif reste un objectif secondaire distinct, non couvert par
ces qualifications u18. Aucun sous-ensemble 8k/16k/32k ne remplace une
trame entière pour le contrat de temps.

GCP non utilisé dans ce contre-audit ; aucun nouveau contrat acquis.
