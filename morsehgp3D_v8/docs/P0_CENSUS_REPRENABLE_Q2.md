# Census q2 reprenable : conserver le travail déjà fait

Cadre : `phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=implementation_v8_p0`,
`public_status=not_claimed`. Tranche15, composant CPU distinct du pipeline
multi-CPU existant. Aucun raccord FULL ni GPU dans cette étape.

## Pourquoi cet objet

La redistribution du front ne permet pas d'aider un worker déjà entré dans
un gros census. Avant de répartir ce travail intérieur, il faut pouvoir
l'interrompre proprement : conserver ce qu'il a trouvé et reprendre au même
endroit, sur le même index, éventuellement dans un autre thread.

`Q2CensusContinuation` réalise cette première opération pour une ancre et
un nœud B de l'index global. Elle n'est **pas encore un répartiteur** : sa pile
entière peut migrer, mais ses frères B ne s'exécutent pas simultanément.
Les chemins Pool, Pairwise et conjoint ne sont pas raccordés. Le pipeline
existant et ses défauts restent inchangés ; il ne crée pas cet objet par ancre.

## Ce qui est conservé

Le propriétaire partagé garde l'index et le nuage immuables. L'ancre est
exprimée par son rang spatial global, puis résolue en ID original. B est
un nœud de ce même index, validé disjoint de l'ancre. Aucun scan de facteur,
copie de coordonnées ou arbre local n'est nécessaire à la création.

Chaque tâche conserve le nœud B courant, le compte exact déjà acquis,
le curseur Z non consommé, la phase de visite et le frère éventuel. Le
contexte B original reste commun à tous les descendants. La pile, les
constantes préparées et les buffers de collecte appartiennent à l'objet.
Chaque frère en attente garde **son propre** triplet compte/curseur/phase
figé lors de la division, et non l'état courant de l'enfant exécuté avant lui.

Quatre étapes sont distinguées : entrée de tâche, décision sur un témoin,
admission d'une plage, collecte et émission d'un support. Une entrée déjà
payée n'est jamais rejouée : ni nouvelle préparation de bornes, ni nouveau
test du frère, ni nouveau compteur de tâche. Lors d'une division de B,
les enfants héritent ensemble du compte, du curseur et de la phase.

Les crédits d'amont ne sont pas préchargés. La racine commence à zéro ;
la collecte repart de la racine de l'index pour obtenir tous les intérieurs
et toute la coquille, comme le contrat q2 précédent.

## Une tranche n'est pas un plafond d'exploration

`advance(quantum, consumer)` effectue au plus `quantum` transitions, puis
rend la main. Appeler de nouveau poursuit le reliquat jusqu'à `Done`.
Une valeur nulle est invalide ; aucune valeur positive ne tronque le résultat.

Une collecte et son callback restent atomiques. Leur coût peut atteindre
O(n + taille de coquille) : un quantum ne borne donc ni le temps mural d'un
appel ni les octets de payload. La coquille n'est pas limitée par K.

Une admission comptabilise sa masse une seule fois avant les émissions.
Un état suspendu peut donc annoncer plus de paires acceptées que de supports
déjà émis. Seul l'état terminé porte les égalités finales. Le code prévoit
une émission paire par paire pour les plages admises, mais la gate actuelle
n'exerce **aucune plage multiple partiellement émise** dans ce raccord B/Z.
Les pauses après admission sont réelles et concernent des singletons.

## Durées de vie et erreurs

Les callbacks utilisateurs sont empruntés pendant l'appel et jamais conservés
entre deux reprises. Leurs spans de payload expirent avec le callback.
L'objet non copiable/non déplaçable se transmet par son propriétaire unique,
après synchronisation avec la fin de l'appel précédent.

`advance`, `snapshot`, `pending` et `memory` partagent une acquisition
exclusive sans attente ; chevauchement et réentrance sont rejetés. La vue
`index()` est immuable et ne lit pas l'état mutable de parcours. L'objet
doit survivre à tous les appels, y compris aux tentatives rejetées.

Un argument d'appel invalide n'empoisonne pas un état prêt. Une exception
qui s'échappe d'une avance acquise, notamment dans la collecte ou le callback,
rend l'état `Failed`, définitivement non reprenable. Les sorties antérieures
ne sont pas annulées. Un rejet de réentrance capturé à l'intérieur du callback
ne fait pas échouer l'avance extérieure. Après `Done`, un appel valide est
sans effet et ne réémet rien.

## Travail et mémoire : ce que l'on peut affirmer

À terminaison, les 26 compteurs census, les six compteurs du frère et les
quatre de l'ordre doivent être identiques au parcours récursif de référence.
Ils restent séparés des onze compteurs de reprise.

Noter T le nombre de tâches B, V les visites géométriques Z, O la somme des
quatre compteurs structurels d'ordre, A le nombre de plages admises et S le
nombre de supports. Le nombre de transitions est exactement

`M = T + V + O + A + S`, avec `A ≤ S`.

Sans appel supplémentaire après la fin, le nombre d'avances vaut
`ceil(M / quantum)` et celui des pauses est inférieur de un. Aucun nouveau
terme quadratique n'est introduit par la suspension de **ce parcours**.
Cela ne borne pas T, V, le nombre total d'ancres ou les sorties globales :
la complexité sous-quadratique générale reste ouverte.

Seul B se divise ; l'arbre u16 a profondeur au plus 48. La réservation initiale
est de 49 cadres, sans troncature ni quota de recherche. Dans les builds
actuels, un cadre fait 128 octets : **6 272 octets de pile réservée par objet**,
avant buffers de payload et métadonnées. Il ne faut surtout pas instancier
simultanément cette représentation CPU pour des millions d'ancres.

`memory()` compte les capacités des vecteurs, pas le RSS. Nuage et index
partagés, métadonnées, pile native et stockage de l'appelant sont exclus.
Une future file de petits descripteurs avec état actif privé par worker
reste à concevoir ; cette classe n'est pas une représentation GPU qualifiée.

## Mesures et autorité

Les preuves propres sont dans
[les reçus de reprise](../receipts/q2_census_resume_20260914/README.md).
Les préflights, puis les captures fermées, sont distingués. Les essais
indépendants de l'auditeur B ne sont pas hérités comme qualification produit.

La nouvelle sonde choisit, parmi le front réel q2, un rectangle au plus grand
B, puis au plus grand petit facteur, puis aux plus petits IDs de nœuds.
Elle teste l'ancre médiane de son petit facteur. Le front entier est bien
parcouru pour la sélection, mais **une seule ancre** est ensuite comptée.
Sa mesure n'est ni le census du front complet ni le contrat de tour 50k.

Les temps `total_ms` de la continuation somment les intervalles actifs,
sans l'attente entre appels. La sonde sépare génération, préparation,
sélection, création, temps actifs et intervalle englobant les reprises.
La référence inclut construction/destruction de son moteur ; l'intervalle
de reprise exclut création/destruction de la continuation. La référence
passe en premier, donc les caches ne sont pas symétriques. Ces colonnes
ne constituent pas un duel permettant d'annoncer un gain d'algorithme.
Le temps de sonde précède ses destructions et la dernière écriture JSON.

## Suite prioritaire

Extraire des frères B non visités dans des continuations indépendantes,
avec propriété unique et comptabilité additive des masses et du travail
déjà payé ; une simple migration de toute la pile ne parallélise pas l'ancre.
Ensuite seulement raccorder le dispatcher. Le Pool doit rester préparé une
fois par rectangle, dans un plan parental possédé et partagé, pas par job.
Mesurer l'occupation utile, le coût des découpages et la mémoire simultanée.

La réflexion q3/q4 suit les mêmes invariants mais change les objets
géométriques : [stratégie q3/q4](Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md).
Le contrat reste la tour complète K=1..10 à 50k sous une seconde sur G4,
avec repli K=1..5, puis 100 ms et plusieurs dizaines de millions de points.
