# Retirer du tri les racines extérieures à la cellule

Proposition mathématique indépendante, postérieure au gel du premier prototype
de balayage local. Le C++ capturé n'applique pas ce clipping ; aucun gain produit
n'est revendiqué ici.

Soit J=L_x⁻¹(0)∩C, segment fermé éventuellement réduit à un coin, et choisir
t₀∈J. Pour une forme active non constante sur la droite de seed, conserver son
événement si sa racine appartient à C, **bornes comprises**. Sinon sa restriction
ne s'annule nulle part sur J : son signe y est constant, égal à son signe en t₀.
Elle devient donc une contribution constante [L_z(t₀)<0], sans événement à trier.
Les formes déjà constantes sur la droite gardent leur traitement antérieur,
notamment les coquilles identiquement nulles.

Si R désigne les événements retenus et O les formes non constantes retirées,
la partition exacte de la feuille donne, pour tout t∈J :

$$p(t)=c_C+\sum_{z\in O}[L_z(t_0)<0]
+\sum_{z\text{ constante}}[L_z(t)<0]
+\sum_{z\in R}[L_z(t)<0].$$

La coquille contient les formes constantes nulles et les événements retenus
s'annulant en t. Initialiser le balayage à −∞ avec les deux contributions
constantes et les sorties de R, puis retirer les sorties de chaque groupe,
lire profondeur stricte et coquille, puis ajouter les entrées. Cela calcule
l'extension algébrique de cette formule ; seuls ses centres dans J sont utilisés.
Aucune initialisation à « bord moins epsilon » n'est nécessaire. Une tangence
au coin reste un événement, même lorsque J est réduit à ce point.

Pour construire t₀ sans racine carrée, écrire L_x=c+aξ+bη et les coordonnées
des côtés α/Q, β/Q. Si b≠0, l'intersection avec ξ=α/Q possède les coordonnées
homogènes (nξ,nη,d)=(αb,−Qc−aα,Qb). Si a≠0, l'intersection avec η=β/Q donne
(−Qc−bβ,βa,Qa). Normaliser d>0 et conserver un point dans la cellule fermée,
par les comparaisons Qnξ aux bornes αd et Qnη aux bornes βd. Une droite non
nulle rencontrant le rectangle fournit au moins une de ces intersections,
y compris lorsqu'elle coïncide avec un côté.

Avec |c|≤15M², |a|,|b|≤8M², M=65535 et |α|,|β|≤2Q, on a |n|≤31M²Q,
d≤8M²Q. Le signe d'une forme z en t₀ est celui de c_zd+a_znξ+b_znη :
la somme des valeurs absolues des termes est au plus616M⁴Q, donc sous2⁸⁴
pour Q≤1024. Promouvoir chaque produit en i128 avant calcul. Cette évaluation
n'introduit pas le produit croisé de deux racines générales de degré huit.

Le nombre W d'IDs actifs parcourus reste inchangé : chaque forme est encore
classée et chaque racine non constante localisée. Le gain recherché concerne
les événements retenus, comparaisons de tri et groupes. La localisation avant
tri ajoute du travail par événement ; compter les calculs de racines aussi,
sans présumer leur diminution. Le temps total reste à mesurer.
Compter séparément constructions/évaluations de t₀, localisations, événements
extérieurs convertis en constantes, événements retenus, tris et groupes.
Le carré local dans W et les grosses coquilles concurrentes restent ouverts.

[clip_gate.py](clip_gate.py) compare exactement balayage complet et clipping
sur des segments rationnels fermés : groupes mixtes aux bornes, segment réduit
à un point, coquilles constantes, événements extérieurs, coefficients rationnels
et constructions entières de t₀. Il réfute le retrait des événements extérieurs
sans leur contribution constante. Ce modèle ne qualifie pas un port C++.
