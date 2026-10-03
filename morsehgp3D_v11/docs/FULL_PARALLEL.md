# Descentes régulières par lots possédés

Cette voie est optionnelle et attend sa qualification native propre. Elle ne
revendique aucun gain de temps. Les captures antérieures, notamment celles du
FULL séquentiel et des mémos, conservent leurs sources et leur portée.

`build_full(..., FullParams, sched::Pool*)` garde le chemin historique lorsque
`regular_batch_capacity=0`. Dans ce cas `descent_lanes=1` et
`lane_memo_capacity=0` sont requis. En mode actif, le Pool est obligatoire,
`1 <= regular_batch_capacity <= 4096`, `1 <= descent_lanes <= 256` et la capacité
de chaque mémo vaut zéro ou une puissance de deux. Le nombre de lanes peut
dépasser la taille du lot. Q=4096 borne le tampon de cellules, **jamais le nombre
de traces examinées**. Aucune limite de recherche ou troncation n'est ajoutée.

## Cellules et dates

Une cellule stricte régulière a `m=qmin`, `t=qmin-1` et `qmin` faces, avec
`qmin` dans 2..4. Le centre est dans l'intérieur relatif du support minimal ;
chacune de ces faces est donc une trace stricte. Le travailleur reconstitue
`I union (U sans un sommet)` dans l'ordre croissant des SiteIdx. Omettre les
sommets de U en ordre décroissant reproduit l'ordre lexicographique exact des
faces de `build_cell`. Aucun test d'aiguïté nouveau ne restreint les faces.

Chaque descente lit seulement le domaine immuable et renvoie une naissance.
Son niveau initial exact doit rester strictement inférieur au niveau de la
cellule ; le niveau terminal seul ne remplace pas cette garde. Les travailleurs
ne lisent ni ne modifient le DSU. Ils peuvent donc anticiper les descentes de
plusieurs niveaux dans un même lot.

Après le join, le pilote applique les cellules dans l'ordre du catalogue et les
faces dans leur ordre historique. Il retrouve la racine DSU de chaque naissance
à cet instant, touche les anciennes composantes et effectue les unions. Une
cellule reste indivisible ; la première racine est locale à cette cellule.
Le pilote clôt un plateau uniquement avant d'appliquer un rang supérieur, ou à
la fin de l'ordre. Une fin de lot ne clôt pas le plateau. Les cellules étendues
provoquent un flush puis suivent intégralement l'ancien `cell()` ; le rang actif
est conservé. Les multifusions et les verticales fermées restent inchangées.

## Propriété, lanes et mémoire

Les lots possèdent un BallIdx et quatre NodeIdx par cellule. Chaque lane possède
son mémo, lié au même domaine et conservé pendant tous les ordres du FULL. Le
mémo historique reste distinct, pour les cellules étendues et les verticales.
La lane d'une cellule est son ordinal régulier global modulo L. Dans un lot,
une tâche traite tous ses ordinaux espacés de L ; elle seule utilise sa table.
Le nombre de workers ne modifie donc ni la suite des requêtes d'une lane, ni
l'ordre des publications dans son mémo. Le taux de succès reste à mesurer.

Les Buffers budgetés propres à ce contexte sont exactement
`Q*sizeof(Job) + L*sizeof(Lane) + L*C*DescentMemo::slot_bytes()` ; la factory
contrôle les sommes et produits avant allocation. Les tableaux d'options de
mémos sont bornés à 256 sur la pile. Aucun tableau de toutes les traces n'est
créé. Le contexte et ses mémos sont détruits avant le déplacement final du
domaine dans le FullTower.

Avant chaque dispatch, le pilote vérifie la marge `4*n*min(W,L,J)` où J est le
nombre de cellules du lot. Une descente active détient au plus un LocatedPart ;
ses tableaux census I/U sont disjoints et contiennent ensemble au plus n sites.
MEB et la recherche de support n'ajoutent pas de Buffer, et les cellules
étendues sont hors de ce dispatch. Les allocations réelles restent facturées
au MemoryBudget atomique partagé ; cette admission est une borne, pas une
allocation fictive de 4n par lane. L'appel suppose un pilote unique et aucune
allocation extérieure concurrente sur ce budget pendant le dispatch. Le pic
réel peut dépendre du scheduling.

Toute erreur du Pool est rendue après acquittement de tous ses travailleurs.
Le constructeur ne publie ni domaine déplacé, ni résultat, ni diagnostics
partiels. Un Pool occupé est refusé, sans repli séquentiel implicite. Les refus
d'allocation rendent les réservations de l'appel ; les résultats antérieurs
et leurs vues restent vivants.

## Compteurs et chronométrage

ForestLedger conserve ses compteurs structurels : cellules, combinaisons,
traces, plateaux, unions, anciennes composantes et verticales. Les compteurs de
descente décrivent le travail réellement payé, mémo compris. Ils peuvent
différer du mono historique qui utilise une autre table ; pour Q/L/C fixés ils
doivent rester égaux entre W1/W4/W48. Le cumul des lanes se fait après join.

FullTimings expose Q, L, C et les octets des mémos de lanes, tous nuls lorsque
la voie est inactive. OrderTimings ajoute `regular_batches`, `regular_cells`,
`regular_traces`, `extended_cells`, `max_regular_batch`,
`regular_dispatch_ns`, `regular_task_sum_ns`, `regular_task_max_ns`,
`regular_publish_ns` et `extended_ns`. Les dix champs sont nuls dans l'ancienne
voie. Dispatch, publication et traitement étendu sont des sous-intervalles
disjoints de `plateaus_ns` ; ils ne lui sont pas ajoutés. La dernière clôture
est incluse dans plateaus mais peut rester hors de ces sous-intervalles.

Les durées des tâches sont des sommes/maxima d'intervalles, avec
`task_sum <= min(W,L,Q)*dispatch_wall` et `task_max <= dispatch_wall` après
cumul des lots. Sans pointeur de diagnostics, aucune horloge n'est créée.
La publication des diagnostics se fait seulement au succès complet du FULL.

## Portes préparées

Le différentiel natif couvre W1/W4/W48, Q1/Q2/Q4096, mémos désactivés/activés,
échelles u18/u21/u24 par la matrice, égalité des forêts et des verticales,
égalité du travail entre workers, cas réguliers et coquilles étendues. Le
témoin `(0,2,4)` exige la fusion ternaire à la même date malgré Q1, puis les
images verticales fermées. Les portes de refus vérifient les paramètres, le
domaine étranger, le Pool réentrant et les diagnostics conservés.

L'injection atomique parcourt toutes les positions d'allocation observées en
W1 puis W4 ; elle mesure séparément les fautes survenues hors pilote, sans
présumer ce dernier nombre avant exécution. La sonde parallèle Q2/L4/W4 passe
le même juge Python Definition indépendant que la sonde historique, avec et
sans mémos. Les cinq mutations prévues visent la dernière face, la clôture par
lot, l'omission du travail d'une lane, une racine de naissance figée et
l'omission d'une cellule étendue. Aucun résultat natif n'est encore acquis.
