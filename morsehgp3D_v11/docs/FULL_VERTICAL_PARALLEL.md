# Descentes verticales parallèles optionnelles

Cette tranche prépare les graines basses des naissances avant le balayage
fermé. Elle ne parallélise ni le DSU de ce balayage ni les contrôles des
enfants. Aucun gain temporel ni qualification native n'est acquis par le port.

## API et propriété

`FullParams::parallel_verticals=false` conserve le chemin antérieur. La valeur
`true` exige un `Pool` et `regular_batch_capacity=Q>0` déjà valides. Les bornes
existantes restent `Q<=4096`, `1<=descent_lanes=L<=256` et une capacité de mémo
nulle ou puissance de deux. Q borne une fenêtre de travail, jamais le nombre
de naissances traitées. Une configuration invalide est refusée avec
`parameter_out_of_range`, avant allocation et transfert du domaine.

Le contexte `ForestParallel` garde ses L mémos privés pendant tous les ordres
de FULL. La nouvelle voie réutilise ces mémos et son `Buffer<Lane>` ; elle
n'alloue ni second jeu de tables ni tableau de résultats supplémentaire.
L'ordinal est celui de la naissance dans son ordre canonique, **remis à zéro
pour chaque ordre k**. La lane est `ordinal % L`. Chaque lane parcourt ses
ordinaux croissants, quels que soient Q et W. Les appels réguliers et verticaux
ne se chevauchent pas. Leur attribution respective est explicite : l'ancien
curseur des cellules régulières n'est pas avancé par les naissances verticales.

Le mémo historique de FULL continue de servir le chemin étendu. Quand l'option
est active, il ne reçoit plus les descentes verticales. La partition des mémos
peut donc changer leur travail payé par rapport au chemin antérieur. Les
résultats géométriques et les compteurs de structure doivent rester identiques.
À configuration L/capacités identique, le travail payé est indépendant de W
et de Q. Aucun taux de répétition ou de succès des mémos n'est présumé.

## Graine, date et coupe fermée

Pour une naissance haute de niveau λ et d'ordre k, la partie choisie demeure
exactement les k−1 premiers `SiteIdx` de la réunion triée I∪U de sa boule.
La descente vérifie `initial_level<=λ` ; substituer le niveau terminal serait
incorrect. Le lookup `lower.birth_node` est une recherche binaire const dans
une table immuable, sans cache mutable. Il donne une **naissance basse**, pas
une composante déjà élevée à λ. Son rang doit également être au plus celui
de la naissance haute.

Les workers écrivent ces graines dans les cases de naissances du tableau
`upper.lower_` déjà requis par les verticales. Ces cases sont disjointes entre
jobs et distinctes des indices de fusion. Tous les workers sont joints avant
le balayage. Les cas de refus peuvent laisser ce brouillon partiellement
rempli ; le constructeur FULL le détruit sans le publier.

Le pilote reprend alors le mélange historique des deux flux triés de
naissances et de fusions. Il active **toutes** les fusions basses de niveau
au plus λ avant une requête. Pour une naissance il remplace sa propre graine
par l'image fermée, après l'avoir lue. Pour une fusion il relève et compare
les images de **tous** les enfants, déjà traités à des niveaux strictement
inférieurs. Aucune autre case de naissance non consommée n'est écrasée.

Le témoin `(0,2,4)` distingue la graine basse et son image fermée ; le carré
à k=4 impose d'accepter l'égalité `initial_level=λ`. Les multifusions de même
date et les vérifications de tous les enfants restent celles du balayage.

## Mémoire, échecs et travail

Les réservations persistantes sont inchangées à Q/L/capacités fixés : le
tableau des verticales a la capacité retenue des nœuds, le DSU de balayage
trois tableaux u32 de taille égale au nombre de nœuds bas. La fenêtre n'ajoute
aucune réserve Buffer. Chaque lane active détient au plus un census de n
`SiteIdx`, donc 4n octets. Avant chaque dispatch, `admit(4n*C)` utilise
`C=min(W,L,nombre de naissances de la fenêtre)`.

Cette admission est un précontrôle de budget, **pas une réservation** ni une
garantie de succès de l'allocateur. Une allocation de census peut refuser
après préparation du DSU, du tableau vertical et des premiers ordres. Le Pool
joint toutes les lanes avant de rendre l'échec. Domaine, ancien résultat et
diagnostics du demandeur restent intacts ; les buffers du nouvel appel sont
libérés. Un Pool déjà occupé refuse sans attente bloquante.

Chaque lane remet son accumulateur de travail à zéro à chaque fenêtre.
Après join, le pilote additionne seulement ce delta au `DescentLedger`, puis
ajoute le nombre de naissances à `vertical_descents`. Il ne rejoue pas le
travail régulier déjà agrégé. Les compteurs du balayage et `vertical_checks`
restent attribués aux véritables appels du pilote.

`FullTimings::parallel_verticals` indique l'option effective. Les sept champs
suivants d'`OrderTimings` sont nuls hors option et à k=1 :

- `vertical_batches`, `vertical_resolutions`, `max_vertical_batch` ;
- `vertical_dispatch_ns`, `vertical_task_sum_ns`, `vertical_task_max_ns` ;
- `vertical_sweep_ns`.

Les intervalles dispatch et sweep sont disjoints et inclus dans `verticals_ns`.
Les tâches sont incluses dans leurs dispatchs ; leur somme est au plus
`min(W,L,Q)*vertical_dispatch_ns`, leur maximum au plus ce même temps mur.
La somme n'est pas une durée à soustraire du mur. Avec `timings=nullptr`,
aucune horloge supplémentaire n'est lue. Les diagnostics sont publiés avec
le résultat FULL complet, jamais à la fin d'une seule fenêtre.

## Portes préparées

Le modèle indépendant utilise Gamma_k/Fraction et des marches de parents,
sans reprendre la descente ni le DSU du produit. Il couvre 324 calendriers,
3 618 contrôles, 20 corruptions, une date initiale égale et 29 strictes. Les
exécutions Python normale et `-O` passent. Le juge natif prévu conserve la
Definition exhaustive : deux lots, mémo éteint/actif, 240 ordres, 1 734 nœuds
et 1 346 images verticales, sur chacun des profils 18/21/24.

Les portes C++ préparées comparent W1/W4/W48 et Q1/Q2/Q4096, l'égalité des
dates, 63 naissances sur 48 lanes, les refus de configuration, le Pool occupé
pendant la phase verticale elle-même, et l'injection de chaque allocation.
L'injection sans mémo couvre les allocations tardives des census. Le nombre
de pannes observées hors pilote est imprimé, sans exiger un ordonnancement
particulier. Quatre mutants nouveaux ciblent la graine non élevée, l'égalité
de date refusée, le travail de descente omis et une naissance non comptée.
Le mutant historique de remontée garde sa porte et sa causalité après
l'extraction du helper commun. Aucun build natif local n'a été effectué.
