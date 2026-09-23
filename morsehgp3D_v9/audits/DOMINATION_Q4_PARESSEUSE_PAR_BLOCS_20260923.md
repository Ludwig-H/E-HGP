# q4 : éliminer des blocs de formes avant les niveaux

23 septembre 2026. Proposition mathématique exacte pour le moteur v9,
**non portée et non chronométrée**. Elle complète le certificat de rayon de
[l'audit q4](Q4_STRUCTURE_ET_BORNES.md), sans le remplacer par une promesse
globale sous-quadratique. L'entrée est le nuage de **sites distincts** du
même propriétaire immuable ; un masque sans sol définit son propre nuage.

## Certificat de domination indépendant de l'arête

Soit `C` une cellule convexe **fermée** de centres, de sommets exacts `V(C)`.
Soit `G` un ensemble de sites réels distincts (de préférence `T≤Kmax`
feuilles choisies dans l'index) et `Z` un nœud de l'index, avec
`G∩Z=∅` et boîte fermée `B_Z`.
Pour chaque sommet `v` de `C`, calculer

`M_G(v)=max_{g∈G}|g−v|²` et `m_Z(v)=min_{z∈B_Z}|z−v|²`.

Le minimum sur `B_Z` est la somme des minima axiaux exacts ; le maximum
sur les gardes réels se prépare **une fois par cellule** en
`O(|G|·|V(C)|)`, puis chaque nœud `Z` coûte `O(|V(C)|)`. Dans un nœud
de population suffisante, prélever exactement `T` IDs distincts de sa
plage spatiale évite d'énumérer toute sa population. Pour un nœud de gardes
compact, on peut le remplacer par le majorant plus lâche
`\widehat M_G(v)=max_{g∈B_G}|g−v|²`, lui aussi séparable par axe. Les
**huit** sommets d'une boîte de centres ou les **quatre** coins de son
parallélogramme local suffisent. Si

`M_G(v)<m_Z(v)` pour **tout** `v∈V(C)`,                                  `(⋆)`

alors, pour tous sites réels `g∈G`, `z∈Z` et tout `c∈C`, on a
`|g−c|²<|z−c|²`. En effet, à `g,z` fixés, la différence
`|z−c|²−|g−c|²=|z|²−|g|²−2(z−g)·c` est affine en `c` ; elle est
strictement positive aux sommets par `(⋆)`, donc sur leur enveloppe
convexe. La disjonction des **rangs de sites** de `G` et `Z` évite tout
double compte ; la stricte inégalité protège les égalités de coquille.
Remplacer `M_G` par `\widehat M_G` dans `(⋆)` conserve cette implication.

Pour préserver le **flux q4 actuel** jusqu'à `Kmax`, prendre
`|G|≥T₄=Kmax−2`. Une présentation q4 émise vérifie
`p=|I|≤Kmax−3` ; si `z∈Z` était intérieur **ou contact** de sa boule,
les `T₄` sites de `G` seraient strictement plus proches du centre que
`z`, donc strictement intérieurs de la boule. On aurait `p≥T₄`, une
contradiction. Ainsi **aucun site de Z n'est support, intérieur ou contact
d'une présentation q4 admise et centrée dans C**. Le critère ne dépend
ni de l'arête propriétaire ni du rayon : le même `(C,G,Z)` peut servir
un bloc d'arêtes `E` sans fabriquer `|E|·|Z|` formes `L_z`.

Ce seuil est **spécifique à la voie q4** : les codes q4 v8/v9 arrêtent
les feuilles à `inside≥K−2` ([v9](../src/gen/lanes/q4_local.cpp)).
Une BallKey de grande coquille peut avoir `q_min=3` ou `2` même lorsqu'une
présentation tétraédrique existe. Si le certificat doit préserver **à lui
seul toutes les BallKey admissibles**, sans invoquer la complétude séparée
des voies q2/q3 ni le contrat limité du flux q4, prendre la garde
conservatrice `|G|≥Kmax` : tout support minimal a `q_min≥2`, donc
`p≤Kmax−1`, et `Kmax` intérieurs imposés le rejettent. Pour la voie q3
isolée, `Kmax−1` gardes suffisent. Aucun crédit de profondeur issu de
`G` ne se transmet à une autre cellule ou un autre masque sans preuve.

À gardes **réels** identiques, `(⋆)` est au moins aussi fort que le test
indépendant `gap²(C,B_Z)>U_C`, où
`U_C=max_{g∈G,c∈C}|g−c|²` : ce dernier donne à chaque sommet
`m_Z(v)≥gap²>U_C≥M_G(v)`. L'implication inverse échoue. Prendre
`C=[0,10]×[0,1]²`, les huit sites de
`G={7,8}×{0,1}²` et `Z={(17,0,0)}`. L'ancien `U_C=66` et le gap
minimal vaut `49` : aucun rejet. Pourtant aux sommets `x=0`,
`M_G≤66<289≤m_Z`, et aux sommets `x=10`, `M_G≤11<49≤m_Z` ; `(⋆)`
rejette `Z`, même avec le majorant par boîte des gardes.

**Le test avec boîte `B_G` et l'ancien test sont incomparables.**
Le [contre-audit B](CONTRE_AUDIT_B_DOMINATION_Q4_20260923.md) donne
`C={(0,0,0)}`, `G={(1,0,0),(0,1,0),(0,0,1)}` et
`Z={(1,1,0)}` : l'ancien test passe (`1<2`), alors que le coin fictif
`(1,1,1)∈B_G` donne `\widehat M_G=3>m_Z=2`.
Le test `(⋆)` sur les trois sites réels passe aussi. La voie conseillée
à `K≤10` essaie donc d'abord un petit tableau de sites gardes réels ;
le majorant par nœud reste une option si l'on veut compter sa population
sans lire ses sites. L'[oracle entier autonome](check_q4_block_dominance_20260923.py)
vérifie les deux sens de comparaison, 1 200 cellules/boîtes aléatoires
et le cas d'un contact dominé par trois gardes à `K=5`, en modes Python
normal et `-O`. Il ne teste pas le générateur ni le coût LiDAR.

Plusieurs nœuds gardes peuvent être sommés **seulement s'ils forment une
antichaîne disjointe** : compter les populations des `G_i` qui satisfont
chacune la variante de `(⋆)` avec leur propre boîte contre le même `Z`,
et s'arrêter à `T`. Une admission à
égalité, un ancêtre avec son descendant ou deux retours fusionnés comptés
deux fois invalideraient le certificat. Si aucun nœud garde n'est assez
compact, utiliser des feuilles distinctes ; si le seuil n'est pas prouvé,
conserver `Z` et le parcours exact.

## Raccord paresseux et coût à mesurer

Une tâche `E×C` porte un ensemble **réel d'arêtes survivantes**, une
cellule de centres et des handles de nœuds de l'index. Elle doit couvrir
tous les centres q4 positifs des arêtes d'`E`. Une enveloppe certifiée
de départ existe : pour une q4 positive dont `ab` est bien la plus longue
arête propriétaire, en notant
`D=|a−b|²` et `m=(a+b)/2`, la variance barycentrique donne
`R²=Σ_i λ_i|v_i−c|²=½Σ_{i,j}λ_iλ_j|v_i−v_j|²≤
D(1−Σ_iλ_i²)/2≤3D/8` pour les quatre poids positifs. Donc
`|c−m|²=R²−D/4≤D/8`. Pour des boîtes d'extrémités
`A,B`, borner `D` au-dessus par les extrema axiaux de `A−B`, puis
élargir la boîte des milieux `(A+B)/2` d'un rayon entier/dyadique `r`
tel que `8r²≥Dmax`. Cette boîte `C_E` est un **surensemble** sûr,
éventuellement large ; diviser `E` ou `C_E` de façon couvrante affine le
test. Les cellules fermées servent à certifier, une règle de possession
demi-ouverte et les BallKeys à ne pas émettre deux fois.

Pour le profil entier u18 actuel, tout centre q4 positif appartient au
tétraèdre de ses supports : intersecter `C_E` avec `[0,M]³`,
`M=262143`, reste sûr. Aux sommets entiers de cette boîte, les distances
carrées et leurs différences tiennent dans i64 (`3M²<2^38`). Pour un
coin local dyadique `α,β` de `Q4LocalCell`, `Q=2^20`, utiliser plutôt
`v_i=N_i/(2Q)` avec
`N_i=Q(a_i+b_i)+α A_i+β B_i`, où `A,B` sont les deux bases locales
entières, et comparer les carrés des distances
**multipliées par `(2Q)²`**, sans division. Les bases locales et
`|α|,|β|≤2Q` donnent `|2Qg_i−N_i|≤6QM`, donc la somme de trois carrés
est `<108Q²M²<2^83` : i128 signé suffit après promotion préalable.
Ce budget ne se transfère pas au profil float32 ; celui-ci requiert son
propre filtre certifié et un repli exact.

Pour chaque cellule demandée, chercher des nœuds gardes dans l'index
**une fois**, puis visiter les nœuds témoins `Z`. Si la domination
s'applique, publier le certificat `(cellule, gardes, Z, seuil)` et ne pas
développer les formes de Z pour les arêtes de `E`. Les sites non éliminés
restent actifs dans un constructeur de niveaux peu profonds ou dans le
balayage par graines ; toute boule proposée repasse les tests exacts
de positivité, propriété, profondeur, coquille et clé. À un centre q4
accepté, les blocs éliminés sont strictement extérieurs, donc les
contacts et intérieurs résiduels sont complets. Pour une proposition
rejetée ou un quotient FULL, ne jamais supposer qu'un fragment incomplet
est une liste globale d'IDs ; recollecter sur l'index si nécessaire.
Un raccord simple essaie d'abord `T` **sites réels distincts** proches du
milieu de `C` comme gardes (leur proximité n'est qu'une heuristique), puis
teste `(⋆)` sur les nœuds `Z` ; si `Z` contient un garde, il faut le
scinder. Un certificat acquis sur `C` reste acquis sans recalcul sur tout
enfant `C'⊂C`, ce qui permet de mémoriser les décisions par cellule
spatiale globale et de les partager entre arêtes et workers. Les cellules
où les gardes changent ou les boîtes sont trop larges se raffinent ;
l'absence de certificat laisse la voie exacte intacte. La clé de cache
inclut l'identité de l'index/masque, les `T` IDs de gardes et les bornes
exactes de `C`, jamais seulement un ID local d'arête. Le petit tableau
possédé de gardes et les maxima `M_G(v)` pré-calculés évitent tout
décompte d'antichaîne dans la voie recommandée.
**Ne pas appliquer cette suppression au cover ou à l'atlas partagé q3.**
À `K=5`, le centre `c=(0,0,0)` est strictement dans le triangle q3 aigu
`(5,0,0),(-3,4,0),(-3,-4,0)` de rayon 5. Les trois gardes
`(0,0,0),(1,0,0),(0,1,0)` dominent `Z={(2,0,0)}` à `C={c}` ; pourtant
les quatre sont intérieurs à la boule. q3 doit rejeter `p=4`, mais perdrait
ce certificat et verrait `p=3` si l'on supprimait `Z` avec le seuil q4
`T₄=K−2`. Garder le cover/certificat q3 intact ou effectuer son census
exact ; même un seuil q3 adapté exige une cellule couvrant **ses** centres,
ce que l'enveloppe `C_E` ci-dessus ne garantit que pour q4.

Le nombre de cellules `E×C` et leur réutilisation sont une **obligation
de mesure**. Un `C_E` grossier peut ne rejeter aucun Z ; une division
excessive peut recopier chaque arête dans beaucoup de cellules. Prévoir
un repli exact par arête quand le coût estimé des tests gardes/produits
dépasse celui du chemin existant ; la limite est un choix d'algorithme,
jamais un quota qui supprime des candidats. Compter séparément les visites
de recherche des gardes, tests `(C,G,Z)`, décisions communes, replis,
incidences `E×C`, **formes réellement matérialisées**
`H=Σ_(e,C) h_(e,C)`, événements, census et sorties FULL. La borne
conditionnelle utile est un petit `H` et un nombre de tâches/certificats
commun inférieur à `n²` ; ni l'un ni l'autre n'est prouvé sur LiDAR.
Les tâches par cellules et nœuds se batchent sur CPU/GPU, avec comparaisons
entières exactes ou repli certifié et buffers bornés. Aucun chrono G4
n'est disponible pour cette proposition.

## Lentille de complétion : bonne spécialisation, pas un remplacement général

Les deux complétions d'un tétraèdre q4 de propriétaire `ab` vérifient
`|x−a|²,|x−b|²≤D`. Le
[`Q4PositiveDomain`](../../morsehgp3D_v8/src/lanes/q4_positive_domain.cpp)
calcule déjà la **population exacte** de cette lentille, mais son
`projection_points` est fixé à neuf projections (huit coins de l'AABB
des complétions plus l'origine) lorsqu'elle est non vide : il ne mesure
pas le nombre de complétions. Dans le
[reçu entier 1 mm](../../morsehgp3D_v8/receipts/u18_resume_20260922/ground_1mm_first/only_probe_01_s00_k5_w8.json),
`admitted_sites=610 570 836` sur `1 872 168` arêtes q4, soit
`326,13` complétions logiques en moyenne. La somme des paires non
ordonnées dans chaque lentille est donc, par Cauchy,
`Σ_e binom(t_e,2)≥((Σ_e t_e)²/E−Σ_e t_e)/2>99,25` milliards.
Une génération `t_e²` pour **toutes** les arêtes est exclue par cette
borne de travail sur cette trame ; les petites lentilles restent une
voie hybride exacte. Les témoins hors lentille peuvent être intérieurs
ou sur la coquille : la lentille ne remplace jamais le census global.

Le [`Q4SeedCellEngine`](../../morsehgp3D_v8/src/lanes/q4_local.cpp)
cherche déjà les graines aiguës/propriétaires (`9 550 974` sur ce reçu),
mais `LiveOnly` et `Joined` consultent leurs produits graine×cellule
**après** la construction de l'atlas Z. Le certificat `(C,G,Z)` vise
précisément cette préparation antérieure. Une capture *shadow* sur arêtes
LiDAR stratifiées, puis sur plusieurs trames entières sans sol/avec sol,
doit comparer coût total et sorties exactes q3/q4/FULL, notamment
l'atlas partagé avec q3 sur les `1 545 198` arêtes à deux voies.
