# Remplacer le scan résiduel par un balayage local exact

Audit indépendant après2920b8b5. Proposition mathématique et prototype
d'audit ; aucune qualification produit, FULL ou G4 n'en découle.
Contexte : les formes communes de l'arête sont L_z(ξ,η)=c_z+a_zξ+b_zη,
égales à quatre fois la puissance de z. Une seed définit L_x=0.
La [carte précédente](../q4_center_blocks_20260920/MATH.md) servait seulement
à rejeter des familles ; son compte minorant ne fournissait pas un census.

## Une partition exacte de la population couverte

Pour chaque cellule fermée C conservée, partitionner les IDs du cover en :
I_C si max_C L_z<0 ; E_C si min_C L_z>0 ; A_C dans tous les autres cas.
Ces ensembles sont disjoints et couvrent tous les IDs. Poser c_C=|I_C|.
Pour tout centre t de C, on dispose alors des deux identités exactes :

$$p(t)=c_C+\sum_{z\in A_C}[L_z(t)<0],\qquad S(t)=\{z\in A_C:L_z(t)=0\}.$$

Le compte concerne d'abord le cover. Sur les centres utiles de la boule
positive propriétaire, ce cover contient tous les intérieurs et toute la
coquille ; le transfert global conserve ses conditions géométriques.
Les candidats restent soumis aux tests exacts de propriété, rang et positivité.

**min=0 doit rester dans A_C.** Le retrait min≥0 était correct pour un
certificat de profondeur seul, mais perdrait des coquilles et des événements.
Il faut aussi garder les formes identiquement nulles sur une droite de seed.
Les sites a,b sont identiquement nuls dans tout le plan : on peut les porter
une fois comme coquille commune, sans recopier leurs IDs dans toutes les feuilles.
Sous l'unicité des sites, ce sont les seules formes globalement nulles.

Lors d'un split, l'enfant hérite c_C et A_C. Il ajoute à son compte les IDs
nouvellement strictement intérieurs partout, retire ceux strictement extérieurs
partout et conserve les autres. Il ne relit ni ne recompte I_C/E_C. Si un budget
interrompt la classification, tous les IDs encore non traités doivent rester
actifs ; la formule reste exacte avec un surensemble A_C. Le budget ne permet
pas de les oublier. Cette invariance suppose de vrais IDs disjoints, pas des
crédits de groupes qui ne seraient que des minorants.

Une feuille DEEP peut rester un pur certificat de rejet. En revanche, le minimum
des bornes des enfants, ou un parent comprimé par union de régions certifiées,
n'est **pas** un compte exact : une forme ξ−1 donne profondeur1 à ξ=0 et0 à ξ=2.
Le minimum0 est sûr pour rejeter, mais ne remplace pas la profondeur du premier
point. Distinguer le type « certificat uniforme » du type « partition exacte ».
Le seuil q3 reste distinct ; une cellule DEEP pour q4 ne dispense pas du census q3.

Le scalaire c_C ne permet pas de restituer les IDs intérieurs. Pour ce payload,
retenir les listes ou blocs intérieurs de l'ascendance, ou faire un census final
unique après regroupement des boules. Ne pas reconstruire ces IDs par supposition.

## Balayer seulement les formes actives

Supposons b_x≠0 et utilisons ξ comme paramètre de la droite de seed.
Pour une forme z, poser p_z=c_z b_x−b_z c_x et s_z=a_z b_x−b_z a_x.
Sa restriction est (p_z+s_zξ)/b_x. Si s_z≠0, sa racine est −p_z/s_z ;
le signe de s_z/b_x distingue sortie et entrée. Si s_z=0, la restriction
est constante ; p_z=0 donne une coquille constante, jamais un intérieur.
Si b_x=0, échanger ξ et η dans **toutes** les formes, puis appliquer ces règles.

On peut initialiser à −∞ avec c_C, les constantes négatives et les sorties,
puis balayer **toutes** les racines des actives, même celles hors de C.
Ceci calcule l'extension algébrique c_C+Σ actifs ; elle égale la profondeur
globale seulement là où le contrat de la cellule et du cover l'autorise.
Les racines hors C mettent encore à jour le compte, mais ne sont pas publiées.
Si l'on ne conserve que les racines situées dans C, il faut initialiser au
début de l'intervalle local, en intégrant exactement les racines extérieures.
Une initialisation à −∞ suivie d'un oubli des événements hors C est incorrecte.

Les racines égales forment un seul groupe. Retirer toutes ses sorties,
lire sa profondeur stricte et sa coquille, puis ajouter ses entrées.
Ne pas saturer un compte qui subira ensuite des retraits : trois sorties aux
racines1/2/3 ont profondeur2 à la première racine ; saturer d'abord à2 donne1.
Le seuil seul ne permet pas de choisir un unique représentant de coquille.
Une présentation invalide n'élimine pas les autres candidats de son groupe.

## Comparer les racines sans produits de degré huit

Pour F_x=(c_x,a_x,b_x), F_z et F_w, noter Δ le déterminant3×3 de ces lignes.
L'élimination des deux premières colonnes par la troisième donne
$b_x\Delta=p_zs_w-s_zp_w$.
Pour s_z et s_w non nuls :

$$\mathop{sign}(r_z-r_w)=-\mathop{sign}(b_x)\mathop{sign}(\Delta)\mathop{sign}(s_z)\mathop{sign}(s_w).$$

Δ=0 conserve l'égalité géométrique ; un ID peut départager le rangement,
mais ne doit pas séparer le groupe. Avec M=65535, les bornes conservatrices
|c|≤15M², |a|,|b|≤8M² donnent |Δ|≤5760M⁶<2¹⁰⁹. Promouvoir avant chaque
produit et garder la somme des six termes en i128. Les produits naïfs p_zs_w
sont de degré huit et peuvent dépasser i128, même lorsque leur différence
réduite se calcule exactement. Le modèle exerce explicitement ce cas.

Les coordonnées d'une intersection non parallèle utilisent :
nξ=b_xc_z−b_zc_x, nη=a_zc_x−a_xc_z, den=a_xb_z−a_zb_x.
Normaliser den>0 ; le centre du plan est (nξ/den,nη/den).
Une comparaison à une frontière α/Q emploie Q·nξ contre α·den, et de même
pour η. À profondeur≤10 et |α|,|β|≤2Q, ces produits sont sous2⁸⁴ ; ils
n'exigent pas les produits rationnels de deux racines générales.

## Frontières : calcul fermé, émission possédée

Les bornes de classification portent sur les cellules fermées. Elles donnent
donc le bon compte et toute la coquille même à un coin. Mais plusieurs cellules
fermées peuvent recevoir la même racine. Un localisateur exact du centre désigne
une seule feuille, par exemple enfant droit/haut à égalité et bords extérieurs
conservés. Seule cette feuille émet. Les autres continuent leurs mises à jour
de balayage. Cette propriété locale ne remplace pas le représentant canonique
entre seeds ni le regroupement des boules identiques entre voies.

Contre-fixture réellement positive : a=(10,10,10), b=(12,12,10),
x=(10,12,8), y=(12,10,8). C'est un tétraèdre régulier, propriétaire ab
avec les IDs0/1, centre(11,11,9), rayon²3 et poids1/4. Dans la base entière,
L_x=16−16ξ+16η et L_y=16+16ξ+16η. Sur C=[0,1/4]×[−1,−3/4],
min L_y=0 et max L_y=8. La vraie racine(0,−1), coin inférieur gauche
possédé par C, est perdue si min≥0 retire y. Sa coquille complète comporte
les quatre sommets, sa profondeur stricte est zéro.

## Ce qui peut être comprimé, et ce qui reste coûteux

Des formes dont les triplets entiers sont proportionnels ont exactement la
même droite de zéros. Une clé primitive par PGCD peut partager cette droite,
avec deux populations d'IDs selon le signe du facteur. Hors de la droite,
l'une ou l'autre population est intérieure ; sur la droite, toutes sont sur
la coquille. La multiplication des comptes remplace alors des événements
répétés, mais ne supprime pas les IDs à rendre ni les essais de présentation.
Des droites seulement proches ne peuvent pas être fusionnées. Des droites
différentes concourant au même centre demandent encore un groupe de racines,
pas une confusion de leurs formes. Aucun bénéfice de cette compression n'est
présumé pour les nuages LiDAR ou l'adversaire dense.

Stocker les actives de toutes les feuilles introduit un nouveau coût.
Après traitement séparé des deux formes globalement nulles, une droite peut
rencontrer O(2^d) feuilles d'une grille de profondeur d : jusqu'à O(m·2^d)
incidences, en plus des descripteurs. Répliquer a,b aurait même coût O(4^d).
Le travail local dépend de la somme des tailles actives sur les incidences
seed×cellule, puis des tris et des sorties. Beaucoup de droites presque
concurrentes peuvent maintenir un terme S·m dans une seule cellule.
Le budget doit porter sur cette mémoire et ce travail aussi ; son repli
conserve les données ou revient au chemin exact, sans tronquer les familles.

[local_gate.py](local_gate.py) compare le modèle aux évaluations rationnelles
directes, aux racines et frontières, avec coquilles constantes, groupes mixtes,
initialisation hors cellule, hérité exact et mutants. C'est un contrôle fini
d'audit, distinct du prototype compilé et de toute qualification produit.
GCP non utilisé.
