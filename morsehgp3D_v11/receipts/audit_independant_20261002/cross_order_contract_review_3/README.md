# Laminarité par ordre et contrat d'une hiérarchie commune

2026-10-02 12:24:48 UTC. Sources documentaires et primitives de `6a22a9118`
copiées avant contrôle. Modèle Gamma indépendant collinéaire, Fraction ;
aucun import ni appel au produit/oracle v11, aucun build, CTest produit ou GCP.
L'exploration initiale à quatre points n'a trouvé aucun témoin (code1),
reproduite et conservée ; elle ne prétend pas prouver une propriété générale.
Le témoin à sept points ci-dessous est jugé séparément, normal/−O.

## Témoin fort : deux familles de descendants statiques se croisent

X={0,10,11,26,27,45,46}, sites distincts sur un axe, IDs 0..6, K1/K2.
Toutes les coordonnées sont admissibles u18. À K1, tous les sites entrent
core à β=0. La branche S1={0,10,11} naît à β=25 (gap10), puis fusionne
avec {26,27} à β=225/4 (gap15).

À K2, tous sauf 0 entrent core à β=1 ; 0 entre à β=100. Les deux
triples (10,11,26) et (11,26,27), span16, joignent les branches à β=64.
Leur parent rejoint la branche {45,46} à β=361/4 (span19), AVANT
l'activation core de 0. Le groupe de descendants attachés au nœud β64
est donc S2={10,11,26,27}, même une fois toutes les attaches terminées.
La population géométrique de sa branche couvre déjà 0 ; ce n'est pas
l'appartenance core. Ne pas confondre ces deux objets.

S1∩S2={10,11}, S1\S2={0}, S2\S1={26,27} : croisement strict.
Chaque ordre reste laminaire ; leur réunion ne l'est pas. Aucune hiérarchie
laminaire commune ne peut conserver simultanément ces deux groupes exacts.
Les verticales à coupe fixée sont cohérentes : le croisement concerne des
branches à des niveaux différents et ne contredit aucune inclusion L2⊆L1.

## Modèle et contrelecture

Pour les points collinéaires, β(F)=(max F−min F)²/4. Chaque k-partie naît
à β(F), chaque (k+1)-partie relie ses faces au même seuil ; union complète
par plateau. La convexité des intersections de boules et la MEB sur l'axe
identifient exactement les composantes de la région continue en 3D.
Le modèle mémorise les états de coupe fermée, construit naissances/fusions,
attache chaque site au nœud vivant à Dk(x), puis prend les unions des
descendants. Il ne s'agit pas seulement de groupes momentanément actifs.

Les événements/dates du témoin ont aussi été relus manuellement par un
second agent. Les sorties complètes et le croisement sont conservés ;
la future porte native sur G4 doit confirmer les mêmes groupes/parents/dates.

## Implication utile pour le développeur

P4 de MATHEMATIQUES prouve la laminarité à ORDRE FIXÉ. L'indiquer explicitement,
sans transformer la livraison des baselines core/cover en promesse d'une
hiérarchie commune à tous les ordres. Le choix global doit déclarer son
critère, puis mesurer groupes présents, pertes de projection et compatibilité.
Une règle peut légitimement perdre S1 ou S2 ; la perte doit rester observable.
Le témoin n'établit aucune supériorité statistique de l'une de ces options.

## Lecture des entiers et niveaux sur la même source

`narrow<Bits>` borne la magnitude avant conversion native ; aux seuils63/127,
les conversions signées restent représentables. Zéro négatif normalisé.
Add/sub/resize travaillent dans un temporaire, même avec alias entrée/sortie ;
le refus conserve l'ancienne sortie. Chaque colonne du produit Wide est bornée
par (2^64−1)²+2(2^64−1)=2^128−1 ; la retenue ne peut déborder u128.
Aucun défaut trouvé par cette lecture.

Level ferme numérateur>=0, dénominateur>0 et budgets, puis compare des produits
à largeur complète sans réduction ni flottant. Budgets de comparaison B18/21/24 :
272/314/356 bits, contre capacités des produits5/6/7 mots, soit320/384/448bits.
Cette lecture favorable ne qualifie ni une sérialisation canonique ni u32 ;
la qualification native est celle des reçus G4 du développeur, relus séparément.
