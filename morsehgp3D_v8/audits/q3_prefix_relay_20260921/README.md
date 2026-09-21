# Audit A — relais q3 sans reprise du compte à la racine

21 septembre 2026. Réponse à la demande de relais compact après la tranche 34.
Prototype indépendant CPU u16, `public_status=not_claimed` ; GCP non utilisé.

## Réponse de structure

**Le relais peut garder seulement le compte et le curseur du préfixe,
puis reprendre l'ordre « minimum de puissance d'abord » à l'intérieur
de chaque sous-arbre restant.** Il n'a pas besoin de stocker une frontière
Z ni une pile par graine.

Le contexte partagé fixe index immuable, arête, seuil K−1, bloc original
de graines et ordre Z. Tant que le partage est actif, un cadre X conserve
`(x_node, count, cursor)` : tous les sites précédant le curseur dans cet
ordre sont entièrement classifiés, avec le même compte strict pour chaque
graine valide de X. Les fils X héritent du même état figé.

La descente Z est une boucle dans ce cadre. Elle ne crée **pas** de tâche
pour chaque fils Z : un rejet saute par `escape`, une admission crédite
puis saute, une ambiguïté descend. Une feuille Z ambiguë impose de
diviser X avant de la consommer, ou de relayer X. La pile des tâches X
reste donc bornée par `1+hX≤49`, indépendamment du nombre de visites Z.
La boîte de centres n'est préparée que pour le cadre X actuellement
traité, et réutilisée pendant sa boucle Z ; aucune boîte par témoin.

Au seuil de petit bloc choisi pour le relais, parcourir sa plage de rangs
sans fabriquer de liste. Pour chaque graine aiguë propriétaire, préparer
sa boule une fois et repartir de la **même copie** du ticket parental.
Les graines invalides sont ignorées comme supports, mais restent dans Z.
Le seuil de relais limite le partage ; il ne tronque jamais le census.
Relayer avant de préparer une nouvelle enveloppe pour les petits blocs
évite d'imposer ces divisions exactes à des millions de petites tâches.

## Du curseur à la forêt restante

Pour le préordre global de l'index, si u est le premier nœud non consommé,
les racines `u, escape(u), escape(escape(u)), …` partitionnent exactement
le suffixe de rangs restant. Les sous-arbres sont disjoints ; aucun parent
du préfixe n'est à reclasser. Fin de tableau signifie suffixe vide.

Traiter ces racines une à une, dans cet ordre. Pour chacune, le census
exact existant peut reprendre son choix de fils par minimum de puissance,
avec une seule pile locale de 49 cadres, réutilisée pour toute la graine.
À la fin d'un sous-arbre, passer à l'escape de **sa racine**, pas à un
curseur laissé par l'ordre interne modifié. Ce réordonnancement est sûr
parce que l'état individuel ne sera plus hérité par d'autres graines.

Les deux voies sont bien différentes : un parcours partagé conservant
un préfixe global, puis un census individuel dont l'ordre interne peut
varier. Le census32 actuel ne peut pas recevoir directement un crédit :
il repart de zéro et de la racine. Le raccord demande une entrée interne
acceptant le suffixe ; recommencer cette fonction avec un crédit serait
un double compte.

La coquille reste une seconde collecte **globale** pour les seules boules
acceptées, y compris lorsque le curseur du compte est déjà à EOF. Les
contacts écartés par `min≥0` dans le préfixe doivent y réapparaître.
Un ticket déjà saturé rejette sans poursuivre ; EOF sous le seuil donne
la profondeur exacte sans nouvelle visite de compte.

Un ticket brut `(count,cursor)` n'est pas une preuve autonome : l'entrée
de production doit le garder privé et attaché au contexte qui le certifie.
Changer l'arête, l'index, le seuil, le bloc ou l'ordre avec le même compte
invalide son contrat. Une migration future partage le contexte possédé,
jamais un pointeur vers la pile du producteur.
`PreparedPower` emprunte la boule et ses coefficients : leur adresse reste
stable jusqu'à la fin du census et de la collecte.

## Deux pièges exercés

Avec a=(20,20,20), b=(40,20,20), z=(30,31,20),
x1=(30,32,20), x2=(30,37,20), prendre l'ordre Z `[a,b,z,x1,x2]`.
Le préfixe avant x1 donne le crédit commun1. Pour K3, x1 a profondeur1,
x2 profondeur2. Chaque graine doit reprendre le ticket `(1,avant_x1)` :
réutiliser le ticket muté par la première perd le témoin x1 de la seconde.
Les coquilles contiennent également a et b déjà rencontrés dans le
préfixe. Cette fixture ne dépend ni de points alignés entre captures ni
d'une approximation flottante.

Ne pas charger toutes les racines résiduelles puis les réordonner dans
la pile49 existante sans nouvelle preuve de capacité. Une grande racine
traitée avant de nombreuses petites racines déjà empilées peut cumuler
la frontière initiale et sa propre descente. La stratégie proposée garde
la forêt implicite et une seule racine active. Une autre politique peut
être sûre, mais doit compter séparément son stockage et ses préparations.

## Vérification indépendante et portée

La [sonde](probe.cpp) réutilise explicitement la primitive de puissance
et la référence census32 dans une copie figée de `q3_ball_census.cpp`,
avec les en-têtes et bibliothèques32 épinglés. Elle ne modifie pas35.
Chaque graine est comparée à des sphères calculées en coordonnées
cartésiennes rationnelles, puis à un census global indépendant.

Les préfixes de cette sonde sont calculés exhaustivement **pour vérifier
le contrat du relais**. Ce coût est publié séparément ; ce calcul n'est
pas un filtre commun proposé au moteur. Ni le partage des centres ni le
générateur global q3 ne sont implémentés ou chronométrés ici.

Deux captures fermées, [r1](receipts/r1/MANIFEST.json) et
[complément coquille30](receipts/shell30/MANIFEST.json), totalisent :

- 128 appels C++, 230 graines et **1 150 relais** ;
- 55 appels Release et 55 appels Clang ASan/UBSan, fuites explicitement
  activées, sur petites fixtures ; sorties et compteurs identiques ;
- 18 appels Release sur les anciennes entrées LiDAR des scans 0/100/200,
  à 8k/16k/32k et K5/10 : 52 graines, 260 relais ;
- comparaison à 1 329 920 puissances globales calculées depuis les sphères
  rationnelles indépendantes ;
- retour à l'entrée racine identique aux **26 compteurs** de la référence ;
  aucun retour à la racine de compte après les quatre autres coutures.

Le complément ne change ni source ni binaire : dans shell30, l'arête
propriétaire `(5,29)` et la graine 8 donnent une boule de profondeur zéro
avec **30 contacts**, conservés dans les cinq coutures et les deux builds.
La première sélection de r1 n'exerçait que quatre contacts au plus ; elle
reste publiée avec ce périmètre. Préfixe saturé, EOF accepté, crédit
positif accepté et plusieurs racines résiduelles sont tous exercés.

Le [modèle de forêt](forest_model.py), normal/−O identique, vérifie
360 partitions et 3 216 relais abstraits, plus quatre cas de la fixture
q3 réelle. Quatre mutations sont réfutées : reprise racine avec crédit,
effacement du crédit, ticket modifié réutilisé pour la graine suivante,
oubli des dernières racines. Sur son contrearbre de hauteur 8, la forêt
réordonnée cumule 15 cadres contre une borne initiale de 9 ; les racines
successives culminent à 8. Ce contrearbre est abstrait, pas une fixture
géométrique native prétendument mesurée.

Le [lecteur](read_closed.py) contrôle les clôtures, sources, entrées,
commandes et registres ; il recalcule les sphères et les sorties. Les
résultats normal/−O sont conservés dans [VALIDATION.json](VALIDATION.json).
L'inventaire [CLOSURE.json](CLOSURE.json) ferme les fichiers de cet audit.
Le premier lecteur exigeait un crédit positif accepté dans chaque capture,
y compris le complément dont les seules boules acceptées ont profondeur
zéro. Cet échec de périmètre est [conservé](history/VALIDATION_first.json)
avec la source du lecteur ; le contrôle est désormais exigé dans r1,
et la coquille 30 dans le complément. Aucun résultat C++ ni binaire modifié.

Cette preuve porte sur le **relais individuel d'un préfixe certifié**.
Elle ne qualifie pas encore le filtre commun de centres, le port 35 ou
un gain global. Les préfixes LiDAR historiques testent ici l'exactitude ;
ils ne remplacent pas le nouveau [protocole scène/moitiés/quarts](../../docs/PROTOCOLE_LIDAR_SPATIAL_20260921.md).
Le relais conserve une préparation exacte par graine et la collecte
globale des sorties acceptées. Mesurer ensuite partage, résidu, relais,
coquilles et mémoire ensemble, sur ce nouveau protocole.

## Relecture du port34

La [contrelecture statique de q4](REVIEW34.json) n'identifie pas de défaut concret dans
les rejets stricts, le domaine, le cache, les émissions ou les réductions
des registres. Sept fichiers ciblés concordent avec les hashes du reçu
constructeur `qualification_r2/smoke_ewedfs4y`, aux 216 sources closes.
Ce constat n'est pas une nouvelle exécution de ses 96 CTests.

Le constructeur a repris le test O(1) d'atlas sans feuille avant tout
résumé, et ses mesures globales confirment que LiveOnly porte l'essentiel
du gain de navigation. Nos anciens essais d'arêtes choisies restent des
preuves séparées. La priorité annoncée au census q3 est cohérente ;
ni une autre variante de cache q4 ni un nouveau dispatcher ne sont requis
pour établir ce relais compact.
