# Contrelecture du parallélisme FULL

Sources e02a6c235, 4 octobre 2026. Lecture des quinze interfaces et corps
épinglés dans SOURCES.json ; modèles Python bornés, aucun build natif ni GCP.
La capsule complète précédente reste l'autorité pour ses captures natives.

La voie en succès ne modifie pas le modèle mathématique. Classification et
résolution écrivent des plages disjointes. Les résolveurs ne lisent pas le
DSU : ils lisent les naissances/lookup immuables ; publication des unions par
un seul acteur pour chaque ordre. Le balayage suivi utilise sa propre union
par taille et active toutes les fusions basses de rang inférieur ou égal à
la requête. Les enfants d'une fusion ne changent plus après publication.
Les champs parent modifiés plus tard sont distincts des champs rang/enfants
consultés. Ne pas appeler nodes()/ancestor_closed() sur une forêt en cours
pour remplacer cette lecture : ces méthodes lisent count_/parents mutables.

Le Pool attend l'acquittement de tous ses workers avant de rendre le Job
emprunté et refuse les appels simultanés/réentrants. Il continue à exécuter
les chunks après un refus Outcome. Dans le pipeline, chaque bloc résolu
publie son état même si une descente refuse ; publication et balayage
n'attendent que des tâches d'indice inférieur. Le nombre de tâches est au
plus W, et le graphe d'attentes est acyclique. Cela ne garantit pas la durée
d'une tâche ni une bonne mise en balance.

Les workspaces des lots sont indexés par slot lorsque le nombre de tâches
est inférieur à W, sinon par worker ; les deux cas restent dans le budget
min(W,L,Q). Les ordres concurrents réservent min(W,L), indépendamment de Q.
Le pipeline indexe les workspaces par tâche, pas par worker, et son admission
garantit assez de cases. Les mémos des lanes cycliques sont injectifs parmi
les slots actifs. Ces arguments algébriques sont exercés sur des paramètres
bornés par check.py ; ce n'est ni une nouvelle porte TSan ni une preuve de
toutes les propriétés du modèle mémoire C++.

Le défaut d'abandon publié dans l'audit géant est confirmé. Après réveil,
closed=kNone rend low_ready vrai même si abandoned=true ; il faut contrôler
l'abandon après la boucle. Aucune nouvelle race native ni faux succès FULL
n'est revendiqué. Un préfixe réellement clos en succès conserve ses
garanties : l'anomalie concerne la sentinelle d'abandon.

Vitesse : W48/K5 réserve 39 résolveurs, cinq publications et quatre suivis ;
K10 en réserve 29, dix et neuf. Le recouvrement évite des barrières mais
immobilise parfois des workers en attente. La voie par étages précharge les
données des prochains jobs ; la voie pipeline ne reprend pas ce lookahead.
Ce sont des pistes A/B, pas des gains chronométrés ni une explication
quantifiée de l'écart v10. Les compteurs sum/max de tâches ne mesurent pas le
temps CPU ; les phases du pipeline sont des queues disjointes, pas la somme
des coûts intrinsèques des résolveurs, publications et suivis recouverts.

Exécution : python3 check.py et python3 -O check.py. Les snapshots sont
immuables après clôture. Les résultats et hashes sont ajoutés à la clôture.
