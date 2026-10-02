# Piste exacte de bornes pour les sites entiers de l'index

Source du port lu : `e8520481d`. Huit [copies Git](SOURCE_BEFORE.json), avant
la dérivation autonome ; la première lecture des sources avait déjà eu lieu.
La documentation LIVE évolue pendant la qualification, les sources numériques
et census correspondent au pin. Aucun produit modifié, natif/build/GCP lancé.

Pour une Sphere valide, centre c=a+N/D et D>0, `F(x)=D||x−a||²−2N·(x−a)`.
Une boîte fermée de points entiers Q a des extrémités entières validées.
Chaque axe de F est une parabole convexe ; ses axes sont indépendants.

1. Préparer une fois l'entier t_j le plus proche de c_j (égalité départagée
   vers le plus petit), puis x_j=clamp(t_j,lo_j,hi_j). Alors **F(x) est le
   minimum exact sur Q∩Z³**, donc un minorant pour tous les sites du nœud.
2. Choisir y_j=hi_j si `2(Da_j+N_j)<=D(lo_j+hi_j)`, sinon lo_j. Alors F(y)
   est le **maximum exact sur Q continu**, donc aussi sur ses sites entiers.
3. Rejeter seulement F(x)>0 ; accepter comme intérieur strict seulement
   F(y)<0. Les égalités restent des contacts à raffiner. Ces bornes sont au
   moins aussi serrées que LB/UB du port ; elles ne bornent pas le nombre de
   visites ou de requêtes FULL.

**Contrat distinct indispensable.** Le minorant discret ne peut pas remplacer
silencieusement [num::power_bounds](sources/morsehgp3D_v11/src/num/geometry.hpp),
qui garantit une boîte continue. Pour le q2 entre (0,0,0) et (1,0,0), sur la
boîte segment [0,1]×{0}×{0}, F vaut0 aux sites et −1/2 au centre. Le minorant
lattice0 ne minore donc pas tout le segment. Réserver cette option au census
de sites entiers de l'index ; aucun transfert à float32, aux boîtes de centres
du générateur ou à un autre domaine sans preuve. Le majorant exact par coin
garde en revanche son contrat continu.

La préparation conserve trois entiers signés larges (payload3×i128), sans
fabriquer un Point hors domaine : le centre peut être extérieur au profil.
Calculer C_j=Da_j+N_j en i128, quotient/reste exacts de C_j/D, arrondi entier
avec signe et ex æquo déclarés ; le clamp précède Point::make. C++ tronque
la division signée vers zéro : ne pas traiter ce quotient comme un floor.
Le modulo et `2abs(reste)` suffisent ; ni N² ni numérateur de degré dix.

Budgets : q3 D<24M⁴, |N_j|<24M⁵ donnent |C_j|<48M⁵,
|2C_j|<96M⁵<2^127 pour B≤24 ; D(lo+hi)<48M⁵, reste<D.
Q4 : D<6M³, |N_j|<9M⁴ donnent |C_j|<15M⁴. Les autres arités sont
plus petites. Quotient/arrondi restent sous i128 ; comparer les deux valeurs
sans former une soustraction non bornée. Après clamp, l'évaluation de chacun
des deux Points reprend exactement les budgets de puissance qualifiés :
q3 <216M⁶ (Wide B21/24), q4 <72M⁵ (natif B18/21/24), tous produits et
sommes partielles compris. Aucun nouveau type de Level ou division Wide.

[check.py](check.py) construit les sphères par Gram/Gauss/Fraction, compare
la puissance à la distance au centre et les extrema à un second oracle de
minimisation. Par profil :1 656 boîtes,7 000 points entiers énumérés ; contacts,
centres hors domaine, hautes coordonnées et q3 Wide contrôlés. Sur ces seules
boîtes synthétiques, extérieur certifié464→547, intérieur strict181→449.
Les contre-exemples au contrat continu sont explicites. Sorties normal/−O
identiques dans [RUN.json](RUN.json) ; ce sont des contrôles mathématiques,
sans box réelle du parcours G4 ni chrono de cette option.

Deux puissances par boîte, préparation/divisions et comparaisons sont payées.
Mesurer le coût total et les visites sur de vraies descentes avant de porter
cette option ; le nouvel index a déjà son banc propre, et la MEB/FULL reste
prioritaire. Aucun gain natif ou contrat massif acquis. Fermeture dans
SOURCE_AFTER, LEDGER et SHA256SUMS, hors seul manifeste racine lui-même.
