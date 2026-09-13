# P0 — préparation partagée et filtre axial q2

13 septembre 2026. `implementation_v8_p0`, CPU mono, entrée u16,
`public_status=not_claimed`. Cette tranche prolonge les
[crédits locaux](P0_CREDITS_LOCAUX.md) ; elle ne produit pas la tour FULL.

Cette note décrit la deuxième tranche, publiée à `8e406f9b`. Son mode
axial `Independent` reste la référence par défaut. La
[troisième tranche](P0_ADDITION_ET_INTERSECTION.md) implémente l'addition
et l'intersection proposées ci-dessous, avec ses propres tests et reçus ;
ne pas attribuer ses résultats aux captures historiques de cette note.

## Deux problèmes différents, deux objets

Le premier problème était de refaire le même tri pour q2, q3 et q4.
`CreditBatch` conserve un seul propriétaire de rectangle et prépare une
seule fois les projections et cellules Tubes de chaque facteur. Chaque
voie applique ensuite son propre test et son seuil. Les trois plans
obtenus sont comparés physiquement aux trois appels séparés : crédits,
IDs, ordres et sous-produits compris. Pool et DualBlocks restent inchangés.
Les compteurs séparent préparation commune et requêtes propres aux voies.
Ce partage concerne **trois voies géométriques, pas trois hiérarchies K**.

Le second problème était le résidu : sur deux nappes, un témoin peut
rejeter beaucoup de paires sans être valable pour toute la nappe opposée.
`AxisQ2Plan` restreint B séparément pour chaque ancre a, grâce à des
colonnes exactement alignées dans A. Un index de boîtes transforme
ensuite ces restrictions en plages disjointes d'IDs de B. Il ne développe
pas les paires pour les compter ou pour construire le plan.

Les deux sondes comparent chaque variante à sa référence sur le **même
propriétaire**, alternativement référence en premier et variante en premier.
La génération, la copie/validation, chaque bras et l'inspection sont
chronométrés séparément. `*_total_ms` additionne le socle commun et un
seul bras ; ce n'est ni la durée des deux bras, ni celle de la tour HGP.
Les empreintes contrôlent la reproductibilité, pas l'exactitude géométrique.

## Pourquoi le filtre q2 ne perd aucune paire utile

Soit h le seuil q2 moins les crédits déjà certifiés du cœur. Le
propriétaire certifie que les IDs de ce cœur sont hors d'A et de B.
Si h=0, le rectangle est déjà éliminé et le plan est vide. Supposons h≥1.
Pour une ancre a, groupons les points z d'A ayant les mêmes coordonnées
qu'a sur deux axes. Sur l'axe restant i, le test d'intérieur de la boule
de diamètre ab se réduit à $H=(z_i-a_i)(b_i-z_i)>0$.

Si b dépasse strictement le h-ième successeur d'a dans cette colonne,
les h successeurs distincts sont tous des témoins. Même raisonnement
avec les prédécesseurs. Le rang sert à **trouver des témoins existants**,
pas à remplacer le test sur une colonne approximative. À l'égalité, le
h-ième témoin a H=0 : cet axe ne suffit plus à rejeter ; un autre axe
peut encore suffire. Les trois axes donnent jusqu'à six
demi-espaces de rejet ; chacun suffit à lui seul. Cette première version
n'additionne pas les crédits des axes. C'est un choix conservateur,
pas une nécessité : les colonnes coordonnées exactes ne se rencontrent
qu'en a, exclu des témoins, donc leurs populations sont disjointes.
L'auditeur a identifié cette amélioration après l'écriture du filtre.
Cette disjonction ne se transfère pas aux tubes épais ou aux crédits
de méthodes différentes sans connaître les IDs.

Le complément de ces demi-espaces est une boîte fermée par ancre.
L'index B rejette un nœud disjoint, émet sa plage s'il est entièrement
contenu, et descend sinon. Les enfants partitionnent les IDs : aucune
paire n'est perdue par l'index ni émise deux fois. Les juges bornés
confrontent chaque rejet au prédicat entier multiprécision indépendant.
Le filtre peut conserver des paires inutiles : c'est un préfiltre sûr,
**pas le census exact des supports q2**.

## Travail payé et limites

Trois tris préparent les colonnes en O(|A| log |A|), avec O(|A|) mémoire
hors sortie. L'arbre de B est construit une fois lorsque nécessaire.
La coupure au milieu d'une étendue u16 donne au plus 48 subdivisions
sur un chemin : c'est une borne de représentation, pas un plafond qui
interrompt les recherches. Son travail est compté par visites de points.
Les requêtes paient les nœuds visités J et les descripteurs émis D.
Le coût intermédiaire n'est pas caché derrière le seul nombre M de paires.
La mémoire temporaire totale est O(|A|+|B|+D) ; le travail de construction
de l'index est O(48|B|). L'expansion ultérieure paiera réellement O(M).

Sur deux grilles planes **complètes et correspondantes**, h≥1, de m points
par facteur, le résidu satisfait $M\leq m(2h+1)^2$. Pour des dimensions
w et t supérieures à h, la formule exacte est
$M=[w(2h+1)-h(h+1)][t(2h+1)-h(h+1)]$.
À h fixé, cette famille n'a donc plus un résidu quadratique.
La recette `sheet_full` v2 construit ces grilles sans rangée tronquée.

La recette historique `sheet` v1 n'est pas la même entrée : sa dernière
rangée peut être incomplète. Une contre-fixture conserve 540 candidates
pour une ancre alors que $(2h+1)^2=441$. Ne pas lui transférer la borne
par ancre d'une grille complète. Les deux familles sont testées séparément.
Hors alignements exacts, ce filtre peut garder tout A×B ; ni une petite
durée de préparation, ni la borne de profondeur de l'index ne prouvent
une complexité globale sous-quadratique. Aucun choix universel de méthode
n'est installé dans le produit.

L'auditeur a aussi construit une rotation entière qui conserve toutes
les distances mais supprime les colonnes cartésiennes exploitables :
à n32k, le filtre conserve alors les 256 millions de paires, avec
seulement 16 000 descripteurs. La construction reste compacte, le coût
aval ne l'est plus. Voir sa [contre-épreuve de rotation](../../audits/morsehgp3D_v8_complementaire/P0_AXES_ET_ROTATIONS.md).
Les candidates intermédiaires n'ont pas à être invariantes par rotation,
mais ce cas interdit de généraliser le gain favorable aux nuages usuels.

## Propriété et exceptions

Les plans conservent le propriétaire immuable. L'auditeur a reproduit un
défaut lorsque l'affectation d'un `CreditPlan` échouait entre changement
de propriétaire et remplacement des vecteurs. L'affectation construit
désormais une copie complète puis échange sans exception. Même garantie
pour `CreditBatch`. Une injection d'échecs d'allocation contrôle chaque
position jusqu'au succès, l'auto-affectation et les objets déplacés.
`AxisQ2Plan` n'autorise pas l'affectation et vérifie ses accès après déplacement.

## Suite de la refonte

L'auditeur propose des queues et fenêtres utilisant A **et** B, avec des
certificats géométriques : elles peuvent laisser moins de paires que le
filtre axial actuel, qui n'utilise que les colonnes d'A. Voir sa
[note indépendante sur les nappes](../../audits/morsehgp3D_v8_complementaire/P0_NAPPES_2D.md).
Ce résultat de prototype n'est pas un chronométrage du présent moteur.
Comparer ce proposeur puis le consommateur exact q2, coût total inclus.
Les groupes collectifs q3/q4 restent une autre piste, sans transfert
automatique de la preuve axiale q2.

La validation du nuage reste locale au propriétaire du rectangle : son
partage sur toute la WSPD n'est pas encore implémenté. La WSPD réelle
s8/10/12, les census q3/q4, les rattachements, les parents, l'export FULL,
la mémoire de pointe et la résidence CPU/GPU restent à qualifier.
Conserver les 8k/16k/32k ; ne pas présenter ces composants comme le
contrat 50k sous une seconde ou comme une qualification multi-millions.

Deux améliorations précises sont maintenant proposées par
[l'auditeur](../audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#8-raffinement-axial-en-cours--additionner-les-colonnes-exactes) :
additionner les colonnes exactes dans une requête de compte borné sur
l'index B, sans développer les paires, puis éviter l'initialisation à
zéro de tableaux immédiatement remplacés dans les voies actives. La
première ramènerait par compte fermé le résidu `sheet_full` n32k/h10 de
6 483 670 à 3 928 390 ; ce n'est pas encore un résultat de notre sonde.
Ces propositions restent séparées des sources et mesures gelées ici.
