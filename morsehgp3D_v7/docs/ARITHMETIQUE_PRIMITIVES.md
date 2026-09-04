# Grand-livre arithmétique q2/q3/q4 et niveaux

Statut : preuve locale conditionnelle, `public_status=not_claimed`.
Lecture de la source réelle gelée, pas résultat expérimental. Les chemins
et lignes ci-dessous sont relatifs à `morsehgp3D_v7/` ; les hashes relus
sont dans le [reçu de lecture](../receipts/arithmetic_review_20260904/README.md).
Ce texte complète le § 4 de la [cartographie](QUALIFICATION_S1_PRIMITIVES.md)
sans promouvoir le théorème horizontal global. Il est porté explicitement
depuis l'overlay documentaire `build/v7_arithmetic_obligations/`. Les filtres
flottants, le front, les tailles de conteneurs et la concurrence ne sont
pas requalifiés ici.

## 1. Domaine et sens exact de la conclusion

Poser $M=65535<2^{16}$ et $R=2^{64}$. Chaque position a trois coordonnées
entières dans $[0,M]$. Une différence de positions a chaque composante
dans $[-M,M]$. Les types réels sont i64 pour ces coordonnées et leurs
petits produits, i128 pour les formes et u64/u128 pour les limbs.
L'exactitude suppose l'implémentation GNU des entiers 128 bits et les
opérations entières C++ conformes au binaire exécuté. Cette note ne prouve
pas la correction du compilateur ni du matériel.

Un argument « le résultat final tient » n'est pas suffisant : chaque
addition, multiplication, négation et conversion ci-dessous est bornée
avant exécution. Les casts de décomposition en mots non signés sont des
extractions modulo R voulues, pas des tests numériques approchés.

Les fonctions Q3Form/Q4Form supposent une forme issue des mêmes points
que ceux du prédicat appelé. Les niveaux exigent un dénominateur positif.
Les structures sont publiques et ne valident pas à elles seules ces
préconditions ; un Q3Form arbitraire ne devient pas valide parce que ses
champs ont les mêmes types.

## 2. Petites coordonnées et q2

| Source / expression réellement écrite | Intermédiaires et borne | Conclusion |
| --- | --- | --- |
| `core/types.hpp:43`, `p3_sub` | Chaque soustraction de positions est dans $[-M,M]$ | i64 exact |
| `types.hpp:44`, `p3_add` appliqué aux positions | Chaque somme dans $[0,2M]$ | i64 exact ; ne vaut pas pour deux P3 arbitraires |
| `types.hpp:46`, trois produits puis somme | Chaque produit de deltas a module au plus $M^2$, sommes partielles au plus $2M^2$ puis $3M^2<2^{34}$ | i64 exact |
| `types.hpp:48`, composante de croix | Deux produits de module au plus $M^2$, différence au plus $2M^2<2^{33}$ | i64 exact ; la norme carrée de cette croix n'a PAS la même précondition |
| `q2.hpp:19–21`, `-(i128)(a_i+b_i)` | La somme se fait d'abord en i64, au plus $2M$ ; cast exact puis négation | Pas de promotion trop tardive |
| `q2.hpp:22`, C | Chaque produit est promu avant multiplication ; $0\leq C\leq3M^2<2^{34}$ | i128 exact |
| `q2.hpp:26`, D2/4 | $0\leq D2\leq3M^2$ ; dénominateur 4 positif | Réduction sûre ; D2 négatif est hors domaine du niveau |
| `keys.hpp:94`, puissance q2 | Norme au plus $3M^2$, trois termes Bz de module au plus $2M^2$ chacun, C au plus $3M^2$ ; toute somme partielle a module au plus $12M^2<2^{36}$ | i128 exact |

Le résultat produit est bien la boule diamétrale :
$|2z-(a+b)|^2-|b-a|^2=4(|z|^2-(a+b)\cdot z+a\cdot b)$.
Le coefficient A vaut 1, donc aucun PGCD n'est nécessaire à la clé q2.
Les supports de deux points sont distincts dans le pipeline ; le niveau
zéro reste arithmétiquement représentable pour les objets qui l'autorisent.

## 3. q3 : chaque intermédiaire Gram et puissance

Écrire $d=b-a$, $u=x-a$, $D=d\cdot d$, $E=u\cdot u$, $F=d\cdot u$.
Alors $D,E\leq3M^2$ et $|F|\leq3M^2$. L'identité de Gram donne
$G=DE-F^2\geq0$ ; G est strictement positif lorsque le triangle n'est pas
colinéaire. Aucun flottant n'intervient dans ces expressions.

| Source / expression | Borne de chaque étage écrit |
| --- | --- |
| `q3.hpp:38–42`, deltas, D/E/F | Comme § 2 : produits et sommes i64 sous $3M^2$ |
| `q3.hpp:45`, `(i128)D*E - (i128)F*F` | Chaque produit non négatif au plus $9M^4<2^{68}$ ; différence entre deux tels nombres, donc module au plus $9M^4$, pas un débordement compensé |
| `q3.hpp:46–47`, D-F et E-F | Ces différences sont encore calculées en i64, module au plus $6M^2<2^{35}$ ; multiplication ensuite promue, module au plus $18M^4$ |
| `q3.hpp:48–50`, W | Chaque terme c1*d ou c2*u a module au plus $18M^5$ ; somme au plus $36M^5<2^{86}$ |
| `q3.hpp:56`, `G*norm2(v)` | v est un delta ; norme calculée en i64 au plus $3M^2$ ; produit au plus $27M^6$ |
| `q3.hpp:56`, somme Wv puis soustraction | Chaque Wv au plus $36M^6$ ; sommes successives au plus $72M^6$, $108M^6$ ; résultat au plus $135M^6<2^{104}$ |
| `q3.hpp:65`, `-(2*G*a_i+W_i)` | 2G au plus $18M^4$ ; produit par a au plus $18M^5$ ; somme au plus $54M^5<2^{86}$ ; négation sûre |
| `q3.hpp:66`, `wa += W_i*a_i` | Trois produits de module au plus $36M^6$ ; accumulateur au plus $108M^6$ |
| `q3.hpp:68`, norme de a puis C | Cast avant chaque carré ; somme au plus $3M^2$ ; produit par G au plus $27M^6$ ; somme avec wa au plus $135M^6<2^{104}$ |

Tous ces intermédiaires sont loin de la frontière signée $2^{127}$.
Le résultat de `q3_ball_form` est obtenu en développant
$G|z-a|^2-W\cdot(z-a)$, sans centre arrondi.

### Les commentaires 86/104 sont prouvables

Le tableau S1 remplace successivement les bornes par des puissances de
deux et obtient B<2^87, C<2^105. Ce n'est pas une contradiction avec les
commentaires historiques. En conservant les constantes, on obtient
$|B_i|\leq54M^5<64\cdot2^{80}=2^{86}$ et
$|C|\leq135M^6<256\cdot2^{96}=2^{104}$.
Ces bornes n'exigent pas que le centre appartienne au triangle : la
non-colinéarité ne sert ici qu'à A>0, pas à réduire les modules.
La porte AxisBounds à 87/105 couvre un domaine plus grand, et reste sûre.

Pour la puissance développée générique de la clé q3 (`keys.hpp:95`), le
premier terme vaut au plus $27M^6$, chacun des trois termes Bz au plus
$54M^6$, puis C au plus $135M^6$. Les sommes intermédiaires de gauche à
droite sont bornées par 27, 81, 135, 189 puis $324M^6<2^{105}$.
Une éventuelle division par le PGCD ne peut augmenter ces modules.

### Niveau q3 et garde de rang

`q3.hpp:79` calcule X par `(D+E)-2*F` en i64. D+E et 2F ont module au
plus $6M^2$ ; leur différence a une borne directe $12M^2<2^{36}$, donc
la cancellation ne masque aucun débordement. L'identité
$X=D+E-2F=|b-x|^2$ resserre ensuite le RÉSULTAT à $3M^2$.
`q3.hpp:81` multiplie d'abord DE en i128, puis X :
$0\leq DEX\leq27M^6<2^{101}$. Le dénominateur ligne 82 satisfait
$4G\leq36M^4<2^{70}$. Le commentaire général num<2^101 et produits
q3/q3<2^171 est donc valide ; les 102/172 du tableau sont plus lâches.

Le générateur appelle `is_acute_seed` avant la forme (`generate.hpp:795–797`).
La lentille impose que ab soit une arête maximale ; le test strict sur
$|2x-a-b|^2$ rejette alors tous les triangles non aigus, donc les colinéaires.
Dans ce test (`q3.hpp:103–104`), chaque composante de 2x-a-b est de module
au plus 2M ; les carrés et somme i64 sont sous $12M^2<2^{36}$.
La complétion vérifie en plus les trois produits scalaires stricts et G>0
(`silent_incidence.hpp:175–180`). Le rendu choisit une arête maximale
puis fait le même test strict (`render.hpp:135–148`).

### Helper ancien q3_detail et conversions de centre

`q3_detail::axis_min` (`q3.hpp:117–129`) n'est PAS AxisBounds optimisé.
Il divise W par 2G puis convertit le quotient en i64 avant clip. Pour
une vraie forme de triangle aigu, W/(2G) est le centre relatif ; le
centre appartient au triangle, donc chaque composante est dans [-M,M].
Le cast et `t1+1` sont alors sûrs. Même pour un triangle non colinéaire,
$G\geq1$ et $|W/(2G)|^2=DEX/(4G)\leq27M^6/4$, donc chaque composante
a module inférieur à $3M^3<2^{50}$ : le cast reste représentable.

Cette dernière garantie dépend de l'identité de vraie forme, pas des
seules bornes indépendantes G/W. Un Q3Form artificiel G=1, W=2^80 serait
hors de cette précondition. Un G=0 provoque une division par zéro si le
helper est appelé directement. La recherche des appels dans src/cli ne
trouve pas de consommateur actuel de `q3_ball_depth`, seulement sa
définition ; ne pas confondre ce helper public avec la route census active.
Les bornes décalées bz-a restent dans [-M,M], donc ses évaluations sont
elles aussi couvertes par les bornes Wv ci-dessus.

## 4. q4 : Cramer, signe et niveau sans centre flottant

`q4_form` accepte même les quadruplets coplanaires : former det=0 ne
déborde pas. Les appels aux niveaux/cofaces exigent ensuite det>0.
Les coordonnées et les différences ci-dessous sont celles des quatre
positions u16 réelles, avant tout résultat de Cramer.

| Source / expression réellement écrite | Borne par étapes |
| --- | --- |
| `q4.hpp:51–55`, e puis M et r | e_i dans [-M,M] ; `2*e_i` calculé en i64, module au plus 2M ; normes r au plus $3M^2$ |
| `q4.hpp:59`, cofacteur | Chaque produit de deux entrées, promu avant multiplication, au plus $4M^2$ ; différence au plus $8M^2<2^{35}$ |
| `q4.hpp:61–63`, cofactors signés | Négations de module au plus $8M^2$, loin de INT128_MIN |
| `q4.hpp:64`, det | Trois termes au plus $16M^3$ chacun ; sommes partielles au plus $32M^3$ puis $48M^3<2^{54}$ |
| `q4.hpp:65–67`, N' | Trois termes au plus $24M^4$ chacun ; sommes partielles au plus $48M^4$ puis $72M^4<2^{71}$ |
| `q4.hpp:68–70`, canonisation | det et tous N' sont négés ensemble, modules inchangés ; aucun INT128_MIN possible |
| `q4.hpp:77`, puissance | Premier terme au plus $144M^5$ ; somme des trois N'v au plus $216M^5$, fois deux au plus $432M^5$ ; soustraction au plus $576M^5<2^{90}$ |
| `q4.hpp:141`, B | det*a au plus $48M^4$, somme avec N' au plus $120M^4$, fois -2 au plus $240M^4<2^{72}$ |
| `q4.hpp:142–144`, C | na au plus $216M^5$ ; norme puis produit par det au plus $144M^5$ ; 2na au plus $432M^5$ ; somme au plus $576M^5<2^{90}$ |
| `q4.hpp:150`, den | det*det dans i128, au plus $2304M^6<2^{108}$ |
| `q4.hpp:151`, num | Trois carrés non négatifs de module au plus $(72M^4)^2$ ; somme au plus $15552M^8<2^{142}$, donc U192 suffit largement |

Les preuves plus grossières du tableau (det<2^54, N'<2^71, donc
num<2^144 et den<2^108) restent suffisantes. Les commentaires q4
57/72/146/114 sont encore plus larges, sans être faux.

La matrice M est la matrice des trois équations des distances égales :
$M(c-a)=r$. L'adjugée calculée aux lignes 61–67 donne
$c-a=N'/\mathrm{det}$ lorsque det est non nul. Changer ensemble le signe
du déterminant et des numérateurs conserve ce centre rationnel.
La puissance vérifie
$P_4(z)=\mathrm{det}(|z-c|^2-|c-a|^2)$ ; le niveau est donc
$|N'|^2/\mathrm{det}^2$, sans extraction de racine.

### Orientation de chaque face : i64 puis i128

Dans `q4.hpp:86`, chaque mineur de deux deltas a module au plus $2M^2$.
Les trois termes du volume ont module au plus $2M^3$ ; sommes partielles
au plus $4M^3$ puis $6M^3<2^{51}$. Tout reste en i64 sans promotion.
Le déterminant brut de la matrice 2e vaut exactement huit fois ce volume.

Les trois sommets de la face sont choisis parmi quatre par une boucle
bornée : `fp[0..2]` reçoit exactement trois pointeurs. Les r0/r1 sont des
deltas de module au plus M. À la ligne 98,
$|\mathrm{det}\,dp_i|\leq48M^4$ puis
$|rc_i|\leq120M^4<2^{71}$ (la borne 2^72 du tableau suffit aussi).
Dans `det3_i128` lignes 44–46, chaque mineur est formé après promotion,
module au plus $2M^2$ ; chaque terme avec rc au plus $240M^6$ ; les sommes
partielles au plus $480M^6$ puis $720M^6<2^{106}$, donc sous 2^107 aussi.

Comme $rc=\mathrm{det}(c-\mathrm{face}_0)$ et det>0, le signe du
déterminant de face est celui du centre réel. Les signes des sommets
opposés alternent (-V,+V,-V,+V), par permutation/translation des lignes.
Le rejet strict de zéro exclut le centre sur une face. Le générateur
rejette det=0 avant ce prédicat (`generate.hpp:1153–1166`), de même que
la complétion (`silent_incidence.hpp:192–195`). Le rendu ne demande pas
le bien-centrage, mais rejette det=0 avant le niveau (`render.hpp:154–156`) :
les bornes de Cramer ci-dessus ne supposent justement PAS le bien-centrage.

`q4_i64_prefilter` lignes 109–115 reçoit des distances carrées réelles :
chacune au plus $3M^2$ ; multiplication par deux et sommes de deux
distances au plus $6M^2<2^{35}$, donc i64 exact. Les comparaisons d'owner
ne font que comparer distances et PointId, sans arithmétique risquée.

## 5. Produits larges : preuve des colonnes écrites

Il faut distinguer les capacités génériques de ces primitives du domaine
beaucoup plus étroit des niveaux effectivement issus des lanes.

### 5.1 Produit u128 × u128 vers U192

Dans `wide.hpp:27–40`, écrire $x=x_0+Rx_1$ et $y=y_0+Ry_1$,
avec chaque chiffre dans $[0,R-1]$. Tout produit partiel est au plus
$(R-1)^2=R^2-2R+1$, donc tient dans u128. Les shifts de 64 opèrent sur
u128 (64<128). Les casts u64 extraient les chiffres bas voulus.

`mid` ligne 36 est au plus $(R-2)+(R-1)+(R-1)=3R-4$ ; son report vaut
au plus 2. L'accumulateur haut ligne 38 est au plus
$2+(R-2)+(R-2)+(R^2-2R+1)=R^2-1$, donc même cette expression tient en
u128 avant le cast final. En développant le produit par colonnes, les
trois mots rendus sont exactement les trois mots bas du produit.

La PRÉCONDITION supplémentaire $xy<R^3=2^{192}$ est indispensable pour
que ces trois mots soient tout le produit. Sans elle la fonction ne
refuse pas, elle tronque. Il n'y a pas d'UB unsigned, mais la valeur
mathématique demandée n'est plus représentée. Pour q3, les croisements
réduits restent sous 2^171 et satisfont cette précondition.

### 5.2 Produit U192 × u128 vers U320

Dans `wide.hpp:44–63`, les six produits partiels ont la même borne
$(R-1)^2$. Les accumulateurs sont formés en u128 dès le premier terme.
Les bornes successives des lignes 54, 56 et 58 sont respectivement
$3R-4$, $4R-4$ et $3R-2$ ; leurs reports sont au plus 2, 3 et 2.
Ils sont très inférieurs à $2^{128}$. Le dernier accumulateur est au
plus R par cette borne lâche, toujours sûr en u128.

L'identité des colonnes montre que ce dernier accumulateur est exactement
le quotient entier du produit par $R^4$. Or les types imposent
$n<R^3$ et $d<R^2$, donc $nd<R^5$ : ce dernier chiffre est finalement
strictement inférieur à R. Le cast u64 ne perd donc aucun chiffre utile.
Contrairement à U192, aucun sous-domaine additionnel n'est nécessaire
pour la capacité du résultat U320 : les tailles des entrées la garantissent.

### 5.3 Somme de trois carrés

Dans `wide.hpp:78–89`, chaque carré doit tenir en U192 et la SOMME aussi.
Ces préconditions sont vraies pour N' ci-dessus. Les sommes sont non
négatives ; une somme partielle ne dépasse donc jamais la somme finale.
L'accumulateur bas est au plus $2R-2$ ; le milieu au plus $2R-1$, reports
au plus 1. À la ligne 87, `sq.w[2] + carry` se fait d'abord en u64 : la
précondition sur la somme finale garantit que cette somme puis l'ajout
de `r.w[2]` restent chacun sous R. Aucun wrap intermédiaire n'est caché.
Hors précondition de somme, un wrap est possible et n'est pas signalé.

## 6. Comparaison de niveaux et limite du mutant haut

`compare_exact_level` (`level.hpp:51–55`) compare les deux produits croisés
en U320. Pour des numérateurs U192 et des dénominateurs i128 STRICTEMENT
positifs, les types suffisent : chaque produit est inférieur à 2^319,
donc la comparaison est exacte sur tout ce domaine numérique, pas
seulement sur les niveaux géométriques. Le cast du dénominateur en u128
est alors exact. Les comparateurs U192/U320 inspectent les mots du haut
vers le bas ; leurs petits indices int descendent jusqu'à -1 sans débordement.

`compare_rational` (`level.hpp:32–34`) exige en revanche, outre den>0 et
num>=0, que les produits croisés tiennent en U192. C'est vrai pour q2/q3,
mais pas pour deux Rational128 positifs arbitraires. `promote_level`
ligne 61 scinde exactement un numérateur i128 NON NÉGATIF en deux mots,
avec le troisième nul ; un numérateur négatif est hors de son contrat.
L'égalité `ExactLevel::operator==` ne compare que la représentation :
1/2 et 2/4 sont sémantiquement égaux sans être des couples égaux.

### Un mot U320 n'est jamais atteint par ces lanes

Avec les seules bornes grossières det<2^54 et N'<2^71, on obtient
numq4<2^144, denq4<2^108, donc croisements q4/q4<2^252. Les constantes
gardées ci-dessus resserrent encore à 2^250. Les mélanges q2/q3/q4 sont
plus petits. Ainsi le mot w[4], qui porte les bits 256..319, vaut toujours
zéro pour des niveaux produits directement par ces lanes u16.

Le mutant `level-trunc-hi` possède DEUX sites : `wide.hpp:39` met à zéro
w[2] de U192, et `wide.hpp:61` met à zéro w[4] de U320. Une divergence
observée dans un calcul de carré U192 ne qualifie pas le second site.
Il faut une porte autonome utilisant le domaine numérique complet de
U320/ExactLevel, avec un mot haut effectivement non nul, ou deux noms
d'injection distincts. Cette distinction de domaine ne doit jamais être
présentée comme un niveau géométrique u16 atteignant 320 bits.

De plus, la lecture actuelle ne trouve AUCUNE inscription de
`level-trunc-hi` dans CMake, ni de cible `mhgp7_level_cmp`. Les commentaires
qui citent cette porte ne sont pas une preuve d'exécution v7. Le test
`selftest.cpp:54–61` ne compare ses produits qu'à des propriétés de
commutativité/monotonie, et son U320 est inférieur à environ 2^193 :
il n'exerce pas w[4] et n'est pas un oracle indépendant du produit.

## 7. PGCD, réductions et divisions

### Valeur absolue et Euclide : tout le domaine des types

`uabs128` (`intmath.hpp:49`) traite même INT128_MIN : v+1 est représentable,
sa négation est au plus INT128_MAX, puis le cast unsigned et +1 donnent
exactement 2^127. Aucune négation directe de INT128_MIN n'est écrite.

`ugcd64` lignes 51–58 n'évalue `%` que pour y non nul. Chaque reste
est dans [0,y), l'invariant PGCD est conservé et y décroît strictement
jusqu'à zéro. Le même argument vaut dans la boucle 128 bits lignes 63–74.
Le passage en 64 bits n'arrive que pour $1\leq y<R$ ; y64 est alors exact,
et $x\bmod y64<y64<R$ justifie l'autre cast. Les retours x=0 ou y=0
traitent tout u128, y compris gcd(0,0)=0. Ces opérations ne multiplient
pas de quotients : pas de produit intermédiaire caché.

### Les casts du PGCD aux dénominateurs sont justifiés

Dans `ball_key_reduce` (`keys.hpp:101–110`), la précondition A>0 implique
que le PGCD intermédiaire reste positif et au plus A, donc au plus
INT128_MAX. Son cast u128→i128 est exact. Il divise réellement tous les
coefficients ; la division signée par un entier positif ne rencontre
jamais le couple INT128_MIN/-1. Le raccourci lorsque g=1 est correct,
car ajouter des coefficients à un PGCD égal à 1 ne peut le changer.
Cet argument de réduction vaut même pour des B/C i128 plus larges que
les lanes ; il ne garantit pas que leur puissance soit évaluable en i128.

Dans `rational_reduce` (`level.hpp:22–28`), den>0 implique de même
$1\leq g\leq\mathrm{den}\leq\mathrm{INT128MAX}$. Tous les casts et
divisions sont donc sûrs, même si num est négatif ou INT128_MIN.
La réduction conserve le signe du numérateur et un dénominateur positif.
Pour num=0, elle donne 0/1. Cela ne rend pas les comparateurs unsigned
compatibles avec les rationnels négatifs : leurs contrats restent distincts.

### Précondition manquante du commentaire floor_div128

Le commentaire `intmath.hpp:76` ne cite que den!=0. Pour l'ensemble i128,
il faut aussi exclure `(num,den)=(INT128_MIN,-1)` : le quotient positif
2^127 n'est pas représentable et la division signée est indéfinie.
Une fois ces deux cas exclus, l'éventuel `--q` ne sous-déborde pas : si
le quotient tronqué valait INT128_MIN, le seul cas admis serait une
division exacte, qui ne déclenche pas la correction.

Dans la route courante la recherche ne trouve qu'AxisBounds comme appel
direct à `floor_div128`. Son dénominateur 2A est positif et inférieur à
2^69, son numérateur a module inférieur à 2^87 : les deux exclusions et
le produit de reconstruction du reste sont assurés. Il ne s'agit donc
pas d'un UB produit atteint ; c'est une obligation d'API bas niveau à
documenter/tester sans exécuter volontairement la division interdite.

## 8. Ce qui est fermé, ce qui reste ouvert

Sous les préconditions nommées, les opérations entières q2/q3/q4/Cramer,
les colonnes des produits larges et les réductions ont maintenant un
argument couvrant chaque intermédiaire numérique écrit. Aucun débordement
signé ni cast hors représentation n'a été établi sur ces appels produit.
Les identités des formes, de Cramer et des produits croisés composent
leur sens mathématique avec cet argument de largeur.

Restent distincts et non résolus par ce texte : la preuve de tous les
intermédiaires affine/Jung/corde/cellules du front ; la totalité des appels
des helpers géométriques génériques ; les invariants d'index et de
cardinalité ; la compilation, FMA et l'environnement ; les contrats
complets des erreurs/publications et l'exactitude horizontale/verticale.
Les racines flottantes/corrigées du même fichier intmath ne sont pas
promues par l'audit du PGCD.

Les portes indépendantes ciblées manquantes sont proposées dans
le [plan des portes](PLAN_PORTES_ARITHMETIQUES.md). Elles doivent juger les juges et imposer des
planchers par propriété, sans transformer des accords aléatoires en
preuve universelle. Aucune de ces propositions n'a été exécutée ici.
