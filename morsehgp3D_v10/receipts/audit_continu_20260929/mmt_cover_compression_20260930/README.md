# MMt : compression exacte de la couverture et lignée médiane

Capture du 30 septembre 2026, 19:10:56–19:10:57 UTC. Prototype Fraction
autonome sur arbres rationnels abstraits ; aucune géométrie native,
mesure LiDAR, exécution GPU ou utilisation GCP. Aucun moteur partagé
modifié. Les données de couverture doivent être complètes : le test ne
prouve pas que les seeds d'un catalogue particulier sont complets.

## Entrée et compression

L'arbre original donne naissances rationnelles et parents. Un seed (v,c)
signifie que v couvre le point à partir de c, avec v vivant à c :
birth(v)≤c<birth(parent(v)), ou c≥birth(v) à la racine. La couverture
persiste et passe aux ancêtres lors des fusions. On conserve le minimum
c pour chaque propriétaire distinct.

Préparer une fois les intervalles DFS et les sauts d'ancêtres/LCA de
l'arbre original. Pour S propriétaires distincts, les trier par DFS,
ajouter les LCA de voisins consécutifs, puis la racine ; relier ces nœuds
par une pile d'intervalles DFS. La fermeture LCA contient au plus 2S−1
nœuds ; avec la racine éventuellement ajoutée, au plus 2S. Elle demande
au plus S−1 requêtes LCA. Aucun chemin original n'est parcouru pour
chaque seed par le compresseur.

Pour un nœud virtuel v : si un enfant virtuel est couvert, la couverture
commence à birth(v) ; sinon v est un seed terminal et commence à son c.
Un seed tardif à un ancêtre déjà couvert par un enfant est redondant.
L'atome comprimé de v est [c_v,min(B,birth(parent_virtuel(v)))] ; il est
omis si sa longueur est nulle. À la racine la fin est B.

Pourquoi c'est exact : entre deux nœuds virtuels, aucune autre activation
initiale ni réunion de deux lignées couvertes n'est possible, sinon un
seed ou son LCA serait présent. Les intervalles des continuations
originales se juxtaposent et leurs longueurs se télescopent. À toute coupe
s, la masse acquise par cet intervalle appartient à anc_s(v) dans
l'arbre ORIGINAL. L'identité tient donc par composante originale, pas
seulement pour le total W. Elle reste valable après B, quand les masses
finies continuent à se réunir. Les plateaux exacts donnent des durées
nulles ; on groupe leurs événements et prend la coupe fermée à égalité.

Le prototype utilise des sauts binaires : préparation globale
O(M log M), mémoire O(M log M), LCA O(log M). Après cette préparation,
la compression par point est O(I+S log S+S log M), où I compte les
seeds d'entrée avant déduplication. Une structure LCA en temps constant
pourrait modifier ce dernier terme ; elle n'est pas implémentée ici.
Aucune borne globale sur I, S ou leur somme sur les points n'est acquise.

## Réduction médiane vérifiée

Choisir m comme médiane pondérée DFS des origines d'atomes, avec les
poids FINAUX w=e−c. Une composante dont la masse courante dépasse W/2
possède plus de W/2 de poids finaux dans son sous-arbre, intervalle DFS :
elle contient donc m. Avant la naissance de m, aucune majorité stricte
n'est possible. Après première majorité, le maximum est la masse de sa
lignée. Avant majorité, cette égalité au maximum global n'est pas affirmée.

Pour un atome (v,c,e), poser h=LCA(v,m) et a=max(c,birth(h)). Sa contribution
à la lignée est zéro avant a, puis min(s,e)−c. À a : saut min(a,e)−c ;
si a<e, pente +1 puis −1 à e. Sinon toute la masse arrive par le saut.
Cela donne au plus deux positions d'événement par atome. Sur une vraie
couverture d'un point, la pente entre événements est 0 ou 1 ; une pente
plus grande révèle un chevauchement. L'arbre original reste nécessaire
pour publier l'ancêtre réellement vivant à la date finale.

## Contrôles et mutations

108 cas : huit manuels et cent générés déterministement, jusqu'à 31
nœuds. Ils couvrent activation tardive, seed interne sans descendant,
seed interne redondant, duplicate propriétaire, activation de racine,
plateaux exacts et chaîne longue. L'oracle indépendant développe les
chemins ; le compresseur ne le fait pas. 15 360 comparaisons contrôlent
les masses à chaque événement et entre événements, en coupes ouvertes
et fermées, W, le quotient des plateaux, la masse médiane et la majorité.

Quatre mutations donnent code 3 par mismatch causal : activation à la
naissance au lieu de c ; fin au bord B au lieu du parent virtuel ; join
médian trop tôt ; coupe ouverte substituée à fermée. Chacune est tuée
en normal et -O. Les deux sorties de référence sont identiques, code 0.
Le lecteur vérifie les empreintes et rejoue une copie immutable des
bytes épinglés, y compris les mutations ; il n'importe aucune source
partagée. Les flottants n'interviennent pas dans les comparaisons.

Limites : pas de date MMt complète ni de comparaison de sommes de
racines dans ce lot, pas de raccord aux incidences du moteur, pas de
borne sur leurs nombres ou sur la taille des entiers rationnels, pas de
parallélisation qualifiée, pas de contrat 100 ms. Cette compression est
un candidat de port exact, pas un résultat de performance.
