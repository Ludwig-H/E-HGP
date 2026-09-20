# Partager une carte de minorants dans le plan des centres

20 septembre 2026, audit indépendant, lecture de 8d0a0f0f. Modèle mathématique
et prototype d'audit, pas un port produit ni une qualification FULL/G4.
Les constructions ci-dessous ne supposent aucun alignement des points.

## Domaine et formes communes

Fixer l'arête ab, v=b−a, D=|v|², w_z=2z−a−b et t=2c−a−b. Un centre
équidistant de a,b satisfait t·v=0, et sa puissance vérifie
$4\Pi_c(z)=L_z(t)=|w_z|^2-D-2w_z\cdot t$.
Tous les seeds de l'arête partagent ces formes affines. Leurs centres q4
positifs propriétaires satisfont |t|²≤D/2 par la borne de rayon de Jung.
En effet, les quatre poids barycentriques positifs λ ont somme1 et donnent
$R^2=\sum_{i<j}\lambda_i\lambda_j|u_i-u_j|^2\leq D(1-\sum_i\lambda_i^2)/2\leq3D/8$.
L'identité $R^2=(D+|t|^2)/4$ entraîne alors le disque annoncé.
Les formes des sites a,b sont identiquement nulles : jamais des crédits.
Un futur sommet a lui aussi une puissance nulle à sa propre racine.

Choisir k avec h=|v_k| maximal, σ=sign(v_k), et les deux autres axes i,j.
Prendre A=h e_i−σv_i e_k et B=h e_j−σv_j e_k, puis t=Aξ+Bη.
La base est entière, perpendiculaire à v, sans normalisation irrationnelle.
Son Gram a valeurs propres h²,D et déterminant h²D ; D≤3h². Le disque est

$$2[(h^2+v_i^2)\xi^2+2v_iv_j\xi\eta+(h^2+v_j^2)\eta^2]\leq D.$$

Le carré [−2,2]² contient donc le disque. Pour ξ=α/q, η=β/q avec q dyadique,
$qL_z=q(|w_z|^2-D)-2(w_z\cdot A)\alpha-2(w_z\cdot B)\beta$.
Les extrema d'une forme sur une cellule sont exactement aux quatre coins.
Une forme dont le maximum est **strictement** négatif crédite son site ;
un minimum ≥0 ne crédite rien. Additionner les profondeurs aux seuls coins
ne donne pas un minorant intérieur : deux demi-plans opposés peuvent avoir
profondeur1 aux coins et profondeur0 sur leur droite commune.

## Enlever aussi des centres géométriquement impossibles

Soit Z l'ensemble des sites autres que a,b qui satisfont les deux conditions
fermées de lentille |z−a|²≤D et |z−b|²≤D. Il contient les deux complétions
de tout q4 positif propriétaire. **Ne pas limiter Z aux seeds aigus.**
Former une fois l'AABB de Z, projeter ses huit coins dans v⊥, ajouter 0,
et prendre leur enveloppe convexe P (au plus neuf sommets).

En effet, si λ sont les poids positifs du centre d'un tétraèdre abxy,
$t=\sum_u\lambda_u\pi_\perp(w_u)$ ; les projections de w_a et w_b sont
nulles. Donc t appartient à conv(0,π⊥w_x,π⊥w_y), puis à P. Une rotation
peut agrandir la surcouverture par l'AABB, mais ne détruit pas cette inclusion.
Les égalités de lentille restent admises ; aucun départage de seed ne fournit
une autorisation supplémentaire de supprimer une complétion de ce domaine.

La lentille borne les **complétions**, pas la population témoin. Le prototype
garde tous les sites du cover fermé |w|²≤4D pour les minorants ; le census
résiduel reste global. Exemple : le tétraèdre régulier
(0,0,0),(20,20,0),(20,0,20),(0,20,20) a D=800, c=(10,10,10), R²=300.
Le site (19,19,19) est strictement intérieur (distance²243), mais hors
lentille de l'arête issue de (0,0,0), à distance²1083 de cette extrémité.
Son w²=2092 le conserve dans le cover3200. Ne pas le retirer du census.

Fixture qui impose la distinction : a=(10,20,20), b=(30,20,20),
x=(20,29,27), y=(20,11,22). Les carrés de longueur sont ab400,
ax=bx230, ay=by185, xy349. La face abx est aiguë, aby obtuse. Pourtant
le tétraèdre est positif, avec poids (3839/8748,3839/8748,515/4374,10/2187)
et centre (20,1135/54,125/6). Son t=(0,55/27,5/3) sort du segment
conv(0,π⊥w_x), mais appartient au domaine obtenu avec x **et** y.

La construction de P utilise des coordonnées entières : pour un coin w,
prendre (D w_i−(w·v)v_i, D w_j−(w·v)v_j), de dénominateur commun Dh
dans la base A,B. Pour chaque côté entre les projections de deux coins
raw w_p,w_q, stocker ensuite la forme 3D n=v×(w_q−w_p), H=n·w_p,
orientée en n·t≤H. L'origine a pour représentant raw w=0. Cette conversion
évite de multiplier les gros déterminants du hull à chaque cellule.
Une cellule ne sort de P que si **tous** ses points violent strictement
une même inégalité : min_cell(n·t−H)>0. Une égalité reste conservée.
Un hull dégénéré nécessite un traitement explicite ; en rester au disque
est conservateur. Ne pas interpréter une liste vide de facettes comme une
preuve de domaine vide sans le contrôle de rang correspondant.

Pour sortir du disque, la boîte 3D englobant les quatre coins de la cellule
donne un minorant de |t|². Si ce minorant dépasse D/2, la cellule est vide
de centres utiles. Ce test est conservateur ; l'AABB peut rencontrer le
disque alors que le parallélogramme réel ne le rencontre pas.

## Une continuation de certificat, pas un arrangement de droites

Pour un petit pool d'IDs distincts, une tâche possède sa cellule, un compte
de témoins déjà universels sur celle-ci, et le masque des IDs encore
incertains. Un ID crédité est retiré du masque. Un ID uniformément extérieur
est également retiré. Les enfants héritent le compte et le masque ; ils
ne recomptent pas les témoins du parent. Les données d'arête et le pool
restent partagés immuables. La subdivision des centres ne nécessite aucun
tri ni construction de toutes les intersections entre droites.

Lorsque tous les enfants sont certifiés au seuil ou hors domaine, leur parent
peut être marqué certifié : les témoins peuvent différer entre enfants.
Leurs **comptes ne s'additionnent pas**. Une borne parentale obtenue de cette
union est le minimum des bornes des enfants non vides. Cette compression
stabilise les requêtes suivantes sur le même domaine.

Un prolongement vers des blocs Z du même index est possible : max et min de
L sur Z×cellule se calculent aux quatre coins du centre. À centre fixé,
le maximum sur la boîte des w utilise les endpoints de chaque axe ; le
minimum utilise w=clamp(t,boîte). Les nœuds Z crédités doivent porter des
ensembles d'IDs disjoints. Si un bloc Z incertain provoque un split du centre,
chaque enfant reprend avec son compte et **le même bloc non consommé**.
Un crédit de groupe ne s'ajoute jamais ensuite à celui de ses propres enfants.

Le schéma ne doit pas chercher à certifier indéfiniment tout le disque.
Il contient t=0, souvent peu profond et impossible comme centre q4 strictement
positif ; même le domaine P le conserve. Tangences, droites presque confondues
et vrais centres peu profonds laissent des cellules UNKNOWN. Budget de
certification ou profondeur atteints ⇒ UNKNOWN, puis traitement exact de la
famille résiduelle. Aucun budget ne supprime un candidat ou une sortie.
Si la profondeur stricte est au moins le seuil sur **tout** un domaine compact,
un ensemble fini de témoins fournit une marge locale positive et une couverture
finie ; une subdivision assez fine finit alors par certifier ce domaine.
Cette existence ne fournit ni profondeur pratique ni borne de coût générale.

## Revenir aux seeds et aux candidats

Une seed x restreint le plan à L_x=0. Une requête suit seulement les cellules
dont la fermeture peut rencontrer sa droite, ou sa corde plus courte quand
celle-ci est disponible. Un nœud certifié permet de couper cette descente.
Une feuille UNKNOWN rencontrée suffit à imposer le repli exact de la famille ;
inutile d'énumérer tous ses autres morceaux pour une simple décision de rejet.
Les contacts en un point doivent rester présents. Une convention de propriété
des cellules peut éviter les doubles visites de frontières ; elle ne change
jamais les tests stricts des témoins.

La carte ne fournit ni événements q4, ni coquilles, ni intérieurs à publier.
Ces objets proviennent encore du traitement exact des familles restantes.
Le compte q3, au centre de face et au seuil K−1, reste distinct du seuil q4
K−2. Un crédit de la carte ne précharge pas silencieusement le census.

Coût à publier : préparation du domaine et du pool, cellules et tests de
formes, masques transmis, visites de carte par seed, UNKNOWN, scans/tri/sorties
résiduels. Ni le nombre d'intersections d'une droite avec une carte adaptative
ni le nombre de cellules nécessaires n'est uniformément logarithmique.
Éviter l'arrangement explicite n'établit pas une borne sous-quadratique.

## Largeurs et contrôle indépendant

Avec M=65535 et q≤2¹⁰, |α|,|β|≤2q. Les coefficients de L sont O(M²),
et les tests de norme 3D O(M²q²). Une borne grossière de128M²q² reste
sous2⁵⁹. Pour le hull, les coordonnées projetées ont module≤12M³ ; les
orientations sont majorées par1152M⁶<2¹⁰⁷. Les normales de facettes ont
module≤4M², |n·A| et |n·B|≤8M³, |H|≤24M³. Leur test dyadique est sous
64M³q<2⁶⁴. Ces marges justifient i128 pour les opérations décrites au
plafond10, avec promotion **avant** multiplication ; pas une profondeur
illimitée, ni une qualification automatique d'autres formules.

[domain_gate.py](domain_gate.py) est autonome, à gardes explicites sous normal
et −O. Il confronte les formes et domaines aux centres calculés par Gram
rationnel, avec tétraèdres positifs, rotations par matrices entières non
axiales, extrêmes u16, complétion obtuse, contact au disque et plateau strict.
Ses mutants sont des fautes de modèle, pas des moteurs produit compilés.
GCP non utilisé.
