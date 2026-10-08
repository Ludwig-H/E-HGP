# Translation avec permutation Morton : témoin pour TMVR

Protocole proposé, **pas qualification native**. Aucun défaut nouveau n'est allégué. Produit relu à `eb86468bf` ;
TMVR repo5, base `c903774b1`, patch `6f0643acca9d6c27…`. [capture.json](capture.json) épingle les corps utiles.
Le test complète le carré de `forest_unit.cpp:237,287` et le déterminisme par fils/tranches (`:486`).

## Témoin effectivement vérifié

[fixture.json](fixture.json) donne K=4, les PointId conservés, la translation Δ=(2,0,0) et les forêts attendues.

| Point | XYZ | PointId | Morton avant | Morton après |
| --- | --- | ---: | ---: | ---: |
| A | (0,0,0) | 41 | 0 | 8 |
| B | (2,0,0) | 7 | 8 | 64 |
| C | (0,2,0) | 99 | 16 | 24 |
| D | (2,2,0) | 13 | 24 | 80 |

Les sites passent de `[A,B,C,D]` à `[A,C,B,D]` : **φ=(0,2,1,3)**, dans u21. `cloud.cpp:64` forme Morton sur les
coordonnées absolues ; `fill_cloud` les conserve. Appeler directement `prepare_cloud` : une préparation soustrayant
le minimum annulerait ce témoin. Construire φ par jointure des `Cloud::points(SiteIdx)` sur les PointId, puis
exiger φ≠identité.

La translation conserve distances, MEB, rayons, séparabilité, inclusions et ordre lexicographique des positions ;
elle ajoute Δ aux centres. Le catalogue contient quatre boules de côté de rayon carré 1 et le cercle de rayon
carré 2, dont le support canonique est AD.

[check.py](check.py) utilise `Fraction` : MEB des 15 parties non vides, supports de taille 1 à 3 dans le plan z=0.
Pour chaque k et niveau 0,1,2, les sommets du nerf sont les k-parties dont la MEB tient au rayon ; une arête relie
deux parties lorsque la MEB de leur union y tient. Ce graphe donne les composantes de l'union des intersections
convexes de k boules. Les **12 coupes**, **30 requêtes**, **7 verticales**, **5 boules/S*** et **16 traces strictes**
sont vérifiées après transport, contre les forêts de la fixture. Lectures normal et `-O` identiques.

Ces seize traces théoriques ont **treize classes** sous la fermeture transitive de `MEB(A∪B)<rayon` : huit
singletons séparés sur les quatre côtés à k1, une classe de quatre singletons sur le cercle à k1, quatre classes
isolées de paires adjacentes sur le cercle à k2. L'oracle vérifie les deux quotients et leurs représentants minimaux
par SiteIdx ; sur ce témoin, A reste le minimum de la seule classe non triviale. Le G épinglé stocke toutes les
traces strictes, donc seize représentants physiques : `cells.cpp:109–110` garde `shape.reps=s`, même lorsque la
cellule est inerte, et `cells_stage.cpp:87–88` copie tous les masques. Une future représentation compactée pourrait
n'en stocker que treize ; la cardinalité théorique ne doit pas être imposée à son interface.

## Limite imposée par la politique G

`resolve.cpp:300+` choisit les plus petits **SiteIdx** dans certains sauts. Après translation, TARG brut, masques,
travail et historiques peuvent changer sans modifier FULL. Pour ce carré seulement, l'oracle vérifie que chaque
trace stricte a une MEB de population exactement k : k1 rend le site, k2 trouve la naissance d'un côté. Aucun saut
ni pas inerte ne dépend donc de la politique. L'égalité des cibles **transportées** est licite ici, pas en général.

## Protocole de la future porte

Ajouter un groupe `translation` après `catalogue` dans `tests/tower/forest_unit.cpp`, puis sa déclaration CTest.
Reprendre la chaîne de `admission:558+` : `prepare_cloud → build_index → build_catalogue(K4) → resolve_tower →
forest_input → build_forests`. Conserver les deux propriétaires. Commencer W1 ; aucun appel natif n'est joué ici.
Clé de boule : `(centre−Δ, rayon²)` rationnels exacts. Clé de trace : ensemble des PointId.

| Domaine | Comparaison |
| --- | --- |
| Cloud | PointId/poids/sites égaux ; coordonnées après−Δ égales via φ ; ordre Morton différent. |
| Catalogue | Bijection des cinq boules ; rang, p,m,q_min égaux ; S*, intérieur et coquille transportés puis comparés comme ensembles ; valeurs exactes des niveaux 0,1,2. |
| G | Naissances/cellules jointes par PointId à k1, clé de boule ensuite ; mêmes rangs et formes. Reconstruire `I∪A` depuis shell+mask. Dans le G épinglé, joindre les seize traces stockées par leurs PointId et comparer les naissances cibles jointes. Si la représentation change, comparer les classes du quotient et la composante atteinte : les représentants choisis peuvent différer. |
| M,V | `births`, `root`, `rank`, `parent`, `minleaf`, `children.off/val`, `lower` égaux en numérotation canonique. Transporter `birth_key` ; comparer `birth_node` après jointure des naissances d'entrée. |
| R public | `cell_node` joint par cellule ; `retained_cell/ball/rank` transportés ; branches CSR en nœuds canoniques. Les IDs d'événements/attaches et tableaux de travail ne sont pas des identités intrinsèques. |
| Requêtes | Chaque naissance canonique, rangs0..2 : même refus `tower_query_domain` avant naissance, sinon même `component_at`. Coupe ouverte au rang r = coupe fermée r−1. Ne pas comparer les nombres de sauts/sondes. |

Pour des témoins ultérieurs avec sauts G, comparer la composante atteinte à la coupe ouverte de la cellule source :
cible naissance → `birth_node[index]`, cible cellule → `cell_node[index]` ; prendre `minleaf` de ce nœud, puis
`component_at(forest, minleaf, rang_source−1)`. L'API attend une naissance, pas un nœud arbitraire. Ce quotient
admet correctement des TARG distincts dans une même composante. Ni hash brut, ni rationnels non réduits ne le remplacent.

Attentes exactes : k1/k2 ont quatre naissances puis une multifusion de quatre enfants aux rangs1/2 ; k3/k4 une
naissance au rang2. Toutes les verticales k2 valent4 ; k3→k2 vaut4 ; k4→k3 vaut0. Le tableau brut `birth_node` k1
passe de `[0,2,1,3]` à `[0,1,2,3]` : l'exiger identique serait une erreur du test. R retient à k1 AC,AB,CD, branches
`[0,1]`, `[0,2]`, `[1,3]` ; à k2 le cercle, branches `[0,1,2,3]`. Ces attentes R découlent du Kruskal dans l'ordre
canonique des cellules ; leur exécution native reste à vérifier.

```sh
python check.py --repo /workspaces/E-HGP --tmv "$TMV"
python -O check.py --repo /workspaces/E-HGP --tmv "$TMV"
```

`TMV` désigne le scratch `v12_tour_TMV` épinglé. Hashes avant/après, aucune compilation, aucune qualification du
produit ou clôture de constat. La configuration `_GLIBCXX_DEBUG` garde le
[point distinct du comparateur](../../audit_reponses_20261007/comparateur_naissances/README.md).
