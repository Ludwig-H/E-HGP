# Premier front réel : des nœuds partagés, pas des plans par rectangle

14 septembre 2026. `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`. Le front est un générateur de
produits résiduels pour q2/q3/q4, pas encore le catalogue ou la tour FULL.

La [qualification locale](../receipts/wspd_front_20260914/README.md)
ferme 40 CTests Release/Clang ASan/UBSan et 72 mesures sur quatre
familles, 8k/16k/32k, Kmax10, s8/10/12. Résultat négatif important :
moins de rectangles ne compense pas le proposeur coûteux. Les amas
gardent un résidu presque quadratique ; la chaîne n'est pas qualifiée
sous-quadratique. Les builds `v8_front_20260914` et
`v8_front_sanitize_20260914` sont désormais épinglés.

## Pourquoi changer d'objet

La tranche précédente a supprimé la copie du nuage par rectangle, mais
Pool et Axis conservaient des tableaux et parcours proportionnels aux
facteurs. Sur A_i×B, ces parcours peuvent payer R fois B et devenir
quadratiques. Le nouveau front ne construit **aucun de ces plans** par
produit : il manipule deux numéros de nœuds et un masque de voies.

L'index global v8 possède déjà les points, une permutation vers leurs IDs
originaux et les boîtes de ses nœuds. Ces vues sont maintenant accessibles
en lecture seule. Une plage dans cet arbre contient des **rangs spatiaux**,
pas des IDs originaux ; la permutation appartient à cet index précis.
Les boîtes sont prises directement dans les nœuds, sans requête de boîte
de plage originale et sans scan de leurs coordonnées. L'arbre de plages
de `PreparedCloud` reste présent pour compatibilité, payé dans la préparation
globale ; le front ne l'interroge pas.

Ce premier pilote emploie l'arbre v8 à bisection au milieu de la plus grande
étendue. Il ne reprend ni l'arbre Morton ni les mesures v4 de l'auditeur.
Il ne revendique pas non plus la borne classique d'une autre construction
de WSPD. La convention choisie est explicitement `box_gap_diameter_v1` :
`gap(boîtes)² ≥ s² × max(diagonale_A², diagonale_B²)`. Les mesures s8/10/12
font maintenant varier les produits réellement générés, mais restent
incomparables aux s v4 sans conversion de convention.

## Couvrir toutes les paires sans les développer

Le parcours part de la racine avec elle-même. Un produit diagonal U×U
devient les trois morceaux non ordonnés L×L, L×R et R×R. Une diagonale
singleton ne contient aucune paire de points distincts. Chaque paire
apparaît donc une fois au plus bas ancêtre commun de ses deux feuilles.

Pour un produit disjoint A×B, trois issues sont possibles :

1. Des témoins certifiés éliminent une ou plusieurs voies q.
2. Si une voie reste et les boîtes sont séparées, émission d'un descripteur.
3. Sinon, subdivision du facteur de plus grande diagonale, gauche puis
   droite ; les deux produits enfants sont disjoints et couvrent le parent.

Les masques de voies sont transmis aux enfants. Un rejet sur un parent
reste valable sur ses sous-produits ; la voie éliminée n'est pas réactivée.
Le parcours emploie une pile de petits descripteurs, sans stocker le front
entier ni la liste de tous les rectangles. Sa réservation initiale est
fondée sur la profondeur u16, pas sur une limite d'exploration ou de sortie.
Le callback est synchrone et peut construire un autre flux ; ses exceptions
ne révoquent pas les émissions déjà effectuées.

Pour chaque voie active, la masse des produits rejetés plus celle des
produits émis vaut n(n−1)/2. Ce ledger est une vérification nécessaire,
pas une preuve de couverture unique : le juge borné vérifie chaque paire
individuellement, y compris son orientation et les réindexages.

## Proposer vite, décider strictement

`Pure` donne la référence sans élimination. `MidpointSamples` propose des
témoins autour du milieu des centres des boîtes. Ce milieu est représenté
par quatre fois ses coordonnées, en entiers. Une descente unique choisit
le fils dont la boîte est la plus proche de ce milieu, sans revenir sur
l'autre fils. C'est un proposeur, **pas une recherche des K plus proches
voisins exacte**.

À la feuille atteinte, une fenêtre d'au plus Kmax rangs distincts est
examinée. Les rangs appartenant à A ou B sont ignorés ; la fenêtre n'est
pas prolongée pour les remplacer. Aucun scan caché ne cherche à tout prix
K témoins admissibles. Depuis la vingtième tranche, une option explicite
(`WspdFrontProposals`, voie q2 seule, défaut inchangé) peut faire suivre
cette fenêtre historique de deux intervalles disjoints complétant une
fenêtre de 2·Kmax ou 4·Kmax rangs autour du même pivot, sans nouvelle
descente : lire la [surproposition de témoins](P0_SURPROPOSITION_TEMOINS_Q2.md). Depuis la
vingt-et-unième tranche, une seconde option de la même voie transmet aux
deux enfants d'un produit non rejeté les rangs de ses témoins certifiés,
comptés une fois et jamais retestés : lire les
[témoins hérités](P0_TEMOINS_HERITES_Q2.md). Un site de A ou de B ne pourrait de toute façon
pas être un témoin strict universel : en le choisissant comme extrémité,
le produit scalaire H vaut zéro.

Pour chaque proposition restante, calculer le minimum entier de H sur
A×B. S'il est strictement positif, il donne un témoin q2 pour tout le
produit. Pour q3/q4, calculer aussi une borne supérieure de Xi, partagée
par les deux voies, puis vérifier respectivement `3Hmin² > Xi_max` et
`2Hmin² > Xi_max`. Les bornes sont celles de la v8, requalifiées sur le
nouveau producteur ; aucune approximation flottante ne rejette une paire.

Les seuils sont Kmax, Kmax−1, Kmax−2 pour les voies actives. Chaque voie
est éliminée indépendamment : q4 peut disparaître alors que q2 reste.
L'échec d'une proposition laisse le produit en vie. La séparation n'est
pas requise pour ces certificats positifs, d'où leur utilisation **avant**
de finir le raffinement WSPD.

Les comptes partiels ne sont pas transmis dans cette première version.
Les enfants repartent de zéro avec leurs propres propositions : pas de
double crédit, mais un risque de perdre des crédits partiels utiles.
Transmettre ces certificats exigera des IDs distincts ou une représentation
de populations disjointes. Un bloc de témoins testé pendant la descente
est une autre piste ; des blocs emboîtés ne s'additionnent pas et un bloc
ne s'ajoute pas aux échantillons qu'il contient.

## Travail supprimé, travail encore non borné globalement

Avec T produits visités et R produits émis, le coût du front échantillonné
est O(T(D+Kmax)+R), où D≤48 pour cet index u16 ; avec une fenêtre élargie
de w·Kmax rangs (w = 2 ou 4), il devient O(T(D+w·Kmax)+R), et T lui-même
baisse quand des produits sont rejetés plus haut. Les compteurs publient les
descentes, distances de boîtes, propositions, tests H/Xi et rejets par voie.
Aucun terme de préparation par produit en |A|, |B| ou n n'est caché dans
ce proposeur. Le coût de construction de l'index global et la validation
du nuage restent payés une fois.

**Cela ne prouve pas que toute la chaîne est sous-quadratique.** Le nombre
T peut encore être quadratique ; le résidu et l'aval peuvent l'être aussi.
Une petite pile ne borne ni les visites ni le travail de la sortie.
Les compteurs de tailles des facteurs, masses par classe et résidus par
voie servent à distinguer ces termes sur 8k/16k/32k. Le callback de sonde
calcule un checksum sans développer les produits ; ce n'est pas un census.

Contre-fixture : A_i=(1000,i,0), B_j=(60000,j,0), avec m sites par rangée.
Pour z=A_k, l'inégalité q3 stricte impliquerait `3(j−k)² > 59000²`, après
annulation du facteur non nul ; le cas B est symétrique. Si
`3(m−1)² < 59000²`, aucun site n'est donc W3, et a fortiori W4, pour une
paire transversale. Cela vaut à n32k. **Même un proposeur ponctuel parfait
laisserait les m² paires q3/q4** : changer la fenêtre ou s ne règle pas ce
régime. Ce constat porte sur les candidates, pas sur la taille intrinsèque
des supports utiles. Les groupes collectifs proposés par les auditeurs et
un aval canonique restent nécessaires.

## Parallélisation et suite

Les tâches ne possèdent ni coordonnées ni tableaux de crédits. Leurs
données géométriques sont partagées en lecture et leurs masques décrivent
des obligations indépendantes. C'est le grain à distribuer ensuite dans
une file de tâches, avec comptages/préfixes pour les émissions. Le présent
parcours reste mono-thread : aucun gain multi-CPU/GPU n'est revendiqué.

Raccorder le résidu q2 directement à l'index global, sans recréer Axis ou
une factory de rectangle par descripteur. Concrètement, extraire le noyau
compte/collecte de sa dépendance obligatoire à `AxisQ2Plan`, puis employer
le nœud B du front comme racine de requête déjà préparée. Pour chaque
ancre A, commencer le census au compte zéro et au curseur Z racine : les
témoins du front ne deviennent pas des crédits ajoutés au census.
L'arbre B et l'index Z peuvent partager les mêmes nœuds immuables, mais
leurs rôles, rangs et curseurs restent distincts. Préférer une entrée
intégrée qui appelle le front, pas l'adoption libre de handles externes.

Fixture de raccord à porter : abscisses {0,5,10,11}, K=2, s=8 ; le produit
{0}×{10,11} reçoit le crédit commun du site 5, puis B se subdivise.
(0,10) est admissible avec intérieur {5}, (0,11) est rejeté avec {5,10}.
Exiger un compteur de subdivision après crédit non nul. Sur les huit
sommets du cube, conserver les quatre supports diamétraux et leur
coquille de huit sites, malgré leur clé de boule commune. Comparer les
quatre combinaisons Pure/Samples × Pairwise/SharedBlocks à l'oracle,
en incluant clés, intérieurs et coquilles, pas seulement leur nombre.

Puis traiter les certificats
collectifs q3/q4, les boules canoniques et les parents FULL. Le coût complet
du census global reste à mesurer avant clôture de P0. Les contrats de tour
50k sur G4 et plusieurs dizaines de millions restent ouverts. GCP non utilisé.
