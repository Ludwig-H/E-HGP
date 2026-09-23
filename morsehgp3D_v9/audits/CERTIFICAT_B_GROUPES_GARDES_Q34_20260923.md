# Certificat B — groupes de gardes, au-delà des paires

23 septembre 2026. Généralisation **mathématique exacte** du
[certificat de paires](CERTIFICAT_B_PAIRES_GARDES_RECTANGLE_20260923.md),
non portée dans le produit. Une palette où **aucune paire** n'est
universelle peut néanmoins fermer K5 par quatre **triples disjoints**.
Le [shadow LiDAR des paires](rect_pair_shadow_b_20260923/README.md)
ne borne donc pas la sélectivité de cette autre famille ; inversement,
aucun gain de durée des groupes n'a été mesuré.

## Lemme pour un groupe fixé

Soient l'arête propriétaire `ab`, `D=|b−a|²` et un groupe `G` de
`r≥2` sites préparés distincts. Pour chaque `g∈G`, poser
`w_g=2g−a−b`, puis

`H_G=rD−Σ_g|w_g|²`, `W_G=Σ_g w_g`,
`C_G=(b−a)×W_G`, `X_G=|C_G|²`.

| Voie | Certificat strict du groupe |
| --- | --- |
| q3 | `H_G>0` et `3H_G²>4X_G` |
| q4 | `H_G>0` et `H_G²>2X_G` |

Un groupe certifié fournit **au moins un** site strictement intérieur
à toute boule possédée par `ab` dans la voie. Si `K−1` groupes q3
ou `K−2` groupes q4 sont **disjoints en IDs**, la voie est fermée.
Les groupes peuvent différer entre voies ; leurs crédits ne s'ajoutent
jamais entre q3 et q4. Exclure `A∪B` n'est pas nécessaire à la preuve,
mais reste un choix conservateur pour un premier port.

Preuve : écrire un centre `c=(a+b)/2+t`. Il est équidistant de
`a,b`, donc `t·(b−a)=0`. La marge
`P_g(c)=|a−c|²−|g−c|²` est strictement positive exactement lorsque
`g` est intérieur. Un développement donne

`Σ_{g∈G} P_g(c)=H_G/4+W_G·t`.

La propriété du propriétaire borne `|t|²≤D/12` pour q3 et
`≤D/8` pour q4. Sur ces disques extérieurs, les minima de la
somme valent `H_G/4−|C_G|/√12` et `H_G/4−|C_G|/√8`.
Les tests du tableau sont leurs formes entières strictes.
Somme positive implique qu'au moins un `P_g>0` ; groupes disjoints
impliquent sites intérieurs distincts. Échec du test signifie
**absence de preuve**, jamais survie prouvée.

Pour certifier un produit `A×B` entier, le groupe fixé est testé aux
**64 couples croisés de coins** des deux boîtes fermées. En effet,

`H_G/4=−r(a·b)+(Σ_g g)·(a+b)−Σ_g|g|²`,

`C_G=2(b−a)×Σ_g g+2r(a×b)`.

Le couple `(H_G,C_G)` est affine séparément en `a` et `b`.
Les régions `H>(2/√3)|C|` et `H>√2|C|` sont des cônes
convexes stricts : les coins caractérisent la preuve pour les
**boîtes continues** et suffisent pour les facteurs discrets.
Un coin virtuel qui échoue ne réfute aucun site réel.

Sur u18, avec `M=2^18−1` et `r≤32`, les majorants
`|H_G|≤15rM²` et `X_G≤48r²M⁴` placent largement les tests
sous la capacité d'un entier signé 128 bits. Il faut promouvoir
**avant** les multiplications ; ne pas transférer ces bornes sans
preuve au profil float32 brut.

## Fixture K5 : quatre triples, aucune paire

Prendre `a=(3,10,10)`, `b=(17,10,10)` et quatre groupes de
trois sites. Les positions transverses communes sont, dans l'ordre,
`(y,z)=(16,10),(7,15),(7,5)`. Les décalages `x−10` sont :

| Groupe | Décalages des trois sites | `H_G` | `X_G` |
| --- | --- | ---: | ---: |
| 0 | `(0,0,0)` | 172 | 0 |
| 1 | `(1,2,−3)` | 116 | 0 |
| 2 | `(2,3,−5)` | 20 | 0 |
| 3 | `(3,1,−4)` | 68 | 0 |

Les 12 sites sont distincts, u18, et les quatre triples passent
strictement q3/q4. Ils donnent quatre crédits q3 et au moins trois
q4 : K5 est fermée. Mais **aucune des 66 paires** de gardes ne
passe même q3 : pour chaque paire `H≤120`, `X≥26 656`, donc
`3H²≤43 200<106 624≤4X`. q4 est plus stricte.
Le [contrôle entier autonome](check_group_guards.py) énumère ces
66 paires. Cette fixture prouve une différence de pouvoir, pas
sa fréquence dans LiDAR.

Une seconde fixture exerce les **64 coins réellement distincts** :
`A={(900,1000,1000),(901,1001,1001)}`,
`B={(1100,1000,1000),(1101,1001,1001)}`. Pour
`δ∈{−2,−1,1,2}`, prendre un triple
`((1000+10δ,1080,1000),(1000+10δ,960,1070),
(1000+10δ,960,930))`. Les minima sur tous les coins et triples
sont `H=36 136`, `3H²−4X=3 906 026 400` et
`H²−2X=1 300 107 952`, strictement positifs.
Aucune paire des 12 gardes ne passe uniformément q3 ;
même en ajoutant les quatre sites des facteurs, aucune des
120 paires ne le fait. C'est une fixture de **preuve sur boîtes**,
pas la promesse que ces nœuds surviennent dans le front WSPD réel.

## Coût et prochaine porte

Une palette de 32 sites donne **4 960 triples** ; tous les
soumettre à 64 coins coûterait jusqu'à **317 440 tests par
rectangle**. La sélection de triples disjoints est un problème
d'hypergraphe, pas un simple matching biparti. Une porte exacte
peut rester bornée comme *filtre incomplet* : proposer quelques
triples à somme vectorielle `C_G` petite et marge `H_G` grande,
tester les coins, arrêter à `K−1`/`K−2` groupes disjoints,
puis reprendre le moteur intact si la preuve échoue. Préparer
les contributions `H_g,C_g` par coin peut amortir l'arithmétique,
mais pas le coût de recherche. Une preuve positive devrait
conserver les IDs des groupes pour un lecteur indépendant.

Première ablation : sur les **299 rectangles positifs S2** du
shadow K5/s8, conserver la même palette et comparer paires,
triples bornés, subdivisions réelles de A/B et paires ponctuelles
par arête. Ce tri positif est un **oracle de diagnostic**,
indisponible avant S2 : il faut ensuite payer la sélection sur
**tous** les rectangles tentés. Publier F/covers réellement
évités, replis, CPU/mur/RSS, K5/K10, s8/10/12 et 8k/16k/32k.
Un nombre de propositions borné par rectangle ne prouve pas
la chaîne sous-quadratique tant que le nombre de rectangles
et le travail résiduel ne sont pas bornés.
