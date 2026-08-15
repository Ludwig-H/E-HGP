# Ré-audit du préfiltre combiné et du cœur-boule

Date : 15 août 2026 UTC.

Sujet audité : `main`, commit
`66b4f0c414e8cfaceb366acde2734bf6531a265c`, soit le delta depuis
`09ab55439a41f41cccbd920eb13fd1ff11150ff6`. Les commits `6220ea3` puis
`66b4f0c`, poussés pendant l'audit, sont inclus dans le verdict.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict exécutif

| objet | verdict au pin audité | conséquence |
| --- | --- | --- |
| réparation du double crédit q2 | **reçue dans le domaine u16** | le masque par frame rend les crédits q2 disjoints ; la fixture, le mutant et l'oracle `PairId` exercent bien le défaut |
| histogramme pour `s_max<=32` | **réparé** | le tableau est maintenant dimensionné par `s_max` |
| `corner64_all_lane` | **reçu comme autorité `ALL` AABB au témoin ponctuel** | exact sur l'enveloppe continue `Box(A) x Box(B)`, seulement suffisant sur les ensembles discrets |
| maximalité globale de `h,h_a,h_b` | **toujours non reçue** | le chemin par défaut reste conservateur, `h_a/h_b` restent quadratiques et leurs extrema restent décorrélés |
| cœur midpoint de `spindle_core_ball` (`6220ea3`) | **reçu comme sous-certificat ouvert borné** | rayon rationnel sous-approché, sphère--boîte et parité descente/direct sont sûrs ; cette réception ne s'étend pas à l'apex ajouté ensuite |
| chemin `--ha=boule` de `66b4f0c` | **P0 sur `separation=1`, non reçu ailleurs** | le carré oublie le signe de `gamma_q` et certifie un faux témoin q3/q4 ; la route n'est heureusement pas celle par défaut |
| claims architecturaux du cœur-boule | **à corriger** | avec `sphere_of(box)` il est dominé par Corner64 ; ball-only ne conserve ni `h_a/h_b` ni toute la fermeture |
| profil CLI u16 | **P0 de garde** | `--coord` hors u16 est accepté, déborde sous UBSan et peut fermer à tort |
| ancien reçu du 15 août | **q2 toujours invalide** | réparer le code ne répare pas les lignes déjà produites au commit fautif ; elles doivent être régénérées |

La bonne route n'est donc pas d'abandonner la boule. C'est de l'utiliser comme
**fast path de comptage en bloc**, puis de conserver l'autorité exacte aux
coins sur tout le complément. Pour `h_a/h_b`, la région continue exacte face à
une boule partenaire se décide par la distance signée au complément d'un cône
robuste, pas par la petite boule centrée au milieu. Les théorèmes et
l'algorithme correspondant sont donnés ci-dessous.

Le push `66b4f0c` confirme empiriquement qu'une **seule** boule d'apex est un
mauvais remplacement sur les trois nuages mesurés. Il ne ferme pas la route
dual-tree proposée ici : elle n'a pas été implémentée, et l'arrêt après `h_q`
succès ne transforme pas une suite arbitraire d'échecs en coût
`O(|A|h_q)`.

## 1. Réparation q2 : le P0 ciblé est fermé

### 1.1 Preuve sur le code

La pile du cœur transporte désormais un `Frame{node,mask}`. Quand un nœud
interne `Z`, disjoint de `A` et `B`, satisfait le certificat q2 en bloc :

1. sa population est créditée une fois ;
2. le bit q2 est retiré du masque transmis à ses enfants ;
3. les bits q3/q4 continuent leur descente ;
4. une feuille ne crédite que les lanes encore actives.

Les nœuds crédités et les feuilles résiduelles forment donc une antichaîne de
sous-arbres disjoints pour chaque lane. Un même index Morton — donc un même
`PointId` dans cette vue bijective — ne peut plus être crédité deux fois. Le
cap à `h_q` ne change pas cette preuve.

### 1.2 Portes réellement exercées

Compilation directe, faute de CMake dans l'environnement d'audit :

```text
g++ -std=c++20 -O2 -Wall -Wextra -Werror \
  -Imorsehgp3D_v2/include -Imorsehgp3D_v3 \
  morsehgp3D_v3/prototype/combined_prefilter_probe.cpp
```

Résultats reproduits au `HEAD` :

| exécution | résultat utile | code |
| --- | --- | ---: |
| `--fixture=coeur5` | `bulk_credits=1`, q2 `21/21` vivantes, doubles/faux/couverture `0/0/0` | 0 |
| même fixture, `--inject=bulk-sans-masque --juge=7` | cinq IDs doubles, une ancre faussement morte | 1 |
| `uniform,n=160,s=6,--oracle=160` | q2 `4054` vivantes, `8666` fermées, faux/doubles/couverture `0/0/0` | 0 |
| `eight_clusters,n=160,s=8,--oracle=160` | q2 `3599/9121`, faux/doubles/couverture `0/0/0` | 0 |
| `terrain,n=160,s=6,--oracle=160` | q2 `2939/9781`, faux/doubles/couverture `0/0/0` | 0 |

Le premier chiffre `4054/8666` est exactement celui de l'ablation sans bulk
de l'audit précédent. Un balayage supplémentaire de 756 petites campagnes
u16, puis une parité bulk/sans-bulk sur 192 configurations, n'a produit aucun
désaccord.

L'oracle de décision est pertinent : pour une paire ponctuelle, les boîtes
dégénèrent en singletons, donc `H` et `Xi` sont exacts ; tous les
`k != i,j` sont énumérés et la symétrie des endpoints est respectée. Il teste
bien :

```text
fermee_par_le_prefiltre  =>  |X\{a,b} inter W_q(a,b)| >= h_q.
```

### 1.3 Durcissements encore requis

Ils ne rouvrent pas la preuve du masque dans son domaine, mais doivent être
corrigés avant de recevoir le probe comme objet borné.

- `--oracle=N` n'impose pas `n<=N`. `--points=201 --oracle=1` lance l'oracle
  et sort 0, alors que la limite annoncée est 200. Utiliser `N` comme vraie
  borne (`0<n<=N<=200`), ou remplacer l'option par un booléen et imposer
  séparément `n<=200`. `compare-corner512`, qui active aussi l'oracle, doit
  recevoir le même cap. `--oracle=-1` est également accepté : valider le signe
  et la largeur **avant** toute conversion vers le type interne.
- Les conversions CLI vers `int` ont lieu avant la validation :
  `--points=4294967298` se replie actuellement sur `n=2`. Parser dans un type
  assez large, exiger la consommation complète, borner, puis seulement caster.
- Les CTests n'exigent pas `oracle_paires>0`. Les trois cas courants exercent
  aujourd'hui respectivement `8666/9121/9781` paires, mais une régression
  fail-open pourrait rendre la porte verte par vacuité. Ajouter un plancher
  interne et une regex `[1-9][0-9]*`.
- `recouvrements` reste un champ mort. Le supprimer du verdict, ou l'alimenter
  à partir du vrai ledger d'identités.
- `core_ids` ne trace que q2. C'est suffisant tant que q3/q4 ne bulkent pas ;
  le futur cœur-boule doit journaliser les IDs **par lane** avant tout crédit
  q3/q4 en bloc.
- Conserver la voie bulk q2 est le bon choix, mais ajouter un mode permanent
  `--no-bulk` et une porte métamorphique comparant les trois comptes et les
  survivantes généralise la fixture ponctuelle.

### 1.4 P0 de domaine découvert pendant le ré-audit

Les deux probes annoncent `quantized_u16_input_only`, mais seule la population
est bornée. `--coord` et les coordonnées produites ne sont pas vérifiés.
Avec :

```text
--points=40 --coord=2147483647 --family=uniform --oracle=40
```

UBSan arrête le probe sur un overflow signé dans les primitives géométriques.
À `n=200`, la version non arrêtée peut finir avec des centaines de faux morts.
Le correctif est simple et bloquant :

1. refuser tout `--coord` explicite hors de l'intervalle documenté ;
2. après génération, vérifier chaque composante dans `[0,65535]` avant Morton
   et avant toute arithmétique ;
3. dans `core_ball_probe`, exiger aussi que `from_chars` consomme toute la
   chaîne (`--coord=12x` est actuellement lu comme `12`) et borner les
   paramètres des fixtures apex ;
4. graver une porte négative pleine largeur et une porte `65535` positive.

Dans `core_ball_probe`, les racines entières sont en outre recalculées par une
boucle partant de zéro. Une valeur CLI immense peut donc suspendre le probe
avant même de produire un diagnostic. Après la garde u16, employer
`isqrt_floor` logarithmique et cacher la sphère de chaque nœud : ce sont des
corrections de robustesse et de coût, sans changement du théorème.

## 2. `Corner64` : théorème et code reçus, portée limitée

Posons `e=z-a`, `t=b-z`, `H=e.t`, `E=|e|^2`, `T=|t|^2`. Les trois lanes sont :

```text
q2 : H>0
q3 : H>0 et 4H^2>ET
q4 : H>0 et 3H^2>ET.
```

À `t` fixé, les `e` admissibles forment un cône de Lorentz ouvert convexe ;
par symétrie, il en va de même en `t`. Par convexité successive :

```text
Box(A) x Box(B) est ALL au témoin z
ssi les 8 x 8 couples de sommets sont admissibles.
```

L'échange des endpoints envoie `(e,t)` sur `(-t,-e)` et conserve `H` et
`ET`. Les tests stricts, les coefficients `4/3` et les largeurs `i64/i128` de
`corner64_all_lane` sont corrects sous u16. Les trois campagnes appariées
reproduisent :

```text
gagne q2 = 0 ; gagne q3,q4 > 0 ; perd = 0 ; faux = 0 ;
corner64_desaccords = 0.
```

Cela reçoit l'autorité mathématique et les résiduels `counter-only`. Cela ne
reçoit pas le `+17/+18 %` comme mesure épinglée : aucun brut chronométrique ni
protocole de machine n'est committé, et un replay local donne un autre surcoût.

Trois actions gardent cette réception falsifiable :

- faire appeler à `corner64_all_lane` l'autorité arithmétique commune de
  `soc64_rect.hpp`, plutôt que recopier sa logique ;
- graver les quatre contre-exemples de l'audit précédent, le swap `A/B`, une
  égalité de shell et la pleine largeur u16 ;
- doubler les CTests nominaux à `PASS_REGULAR_EXPRESSION` par une exigence de
  code zéro : la regex positive peut sinon masquer un futur code non nul.

`Corner64` ne rend pas pour autant les trois `h` maximaux : il n'est pas la
route par défaut, il ne traite que `h_coeur`, et `h_a/h_b` utilisent toujours
les anciennes bornes ainsi que les auto-jointures explicites.

## 3. Théorème corrigé du cœur-boule

### 3.1 Boule midpoint : elle est ouverte

Soit `L=|ab|`, `m=(a+b)/2`, et soit `alpha_q` le demi-angle admissible entre
`e=z-a` et `t=b-z` :

```text
alpha_2 = 90 degres,
alpha_3 = 60 degres,
alpha_4 = arccos(1/sqrt(3)).
```

Le seuil de l'angle `a-z-b` vaut `pi-alpha_q`. La plus grande boule centrée en
`m` contenue dans le fuseau **ouvert** est :

```text
B°(m, kappa_q L),
kappa_2 = 1/2,
kappa_3 = 1/(2 sqrt(3)),
kappa_4 = sin(15 degres).
```

La distance du milieu à la frontière dans un plan méridien donne ces trois
rayons. La symétrie centrale et la convexité du fuseau montrent en outre que
le milieu est un centre d'inball optimal.

Le cercle ou la sphère tangente n'appartient pas au crédit. Par exemple :

```text
a=(0,0,0), b=(2,0,0), z=(1,1,0)
```

donne `|z-m|=kappa_2 L=1` mais `H=0`. En q3, l'exemple entier

```text
a=(100,100,100), b=(102,98,104), z=(102,100,102)
```

vérifie `|z-m|^2=L^2/12` et `4H^2=ET` : c'est encore le shell. Toute écriture
`distance <= R_q` doit donc devenir `distance < R_q`.

### 3.2 Deux boules d'endpoints : certificat simple

Supposons `A` et `B` contenus dans les boules `Ball(c_A,r_A)` et
`Ball(c_B,r_B)`. Posons `d=|c_B-c_A|`, `r=r_A+r_B` et
`m=(c_A+c_B)/2`. Pour tout `(a,b)` :

```text
|m_ab-m| <= r/2,
|ab| >= d-r.
```

Par conséquent, pour chaque lane :

```text
R_dec,q = kappa_q(d-r)-r/2,
B°(m,R_dec,q) subset intersection_{a in A,b in B} W_q(a,b),
```

dès que `R_dec,q>0`. C'est un certificat sûr, mais pas la région universelle
maximale.

### 3.3 Borne couplée, gratuite et plus grande

La formule précédente maximise séparément le déplacement du milieu et la
perte de longueur. On peut conserver leur corrélation. Écrivons
`a=c_A+u`, `b=c_B+v`, puis :

```text
p=(u+v)/2,  w=(v-u)/2.
```

Alors `|p|^2+|w|^2=(|u|^2+|v|^2)/2` et :

```text
kappa_q |b-a| - |p|
 >= kappa_q d - 2 kappa_q |w| - |p|
 >= kappa_q d
    - sqrt((4 kappa_q^2+1)(r_A^2+r_B^2)/2).
```

On obtient donc le second rayon sûr :

```text
R_coup,q = kappa_q d
           - sqrt((4 kappa_q^2+1)(r_A^2+r_B^2)/2).
```

Utiliser `max(0,R_dec,q,R_coup,q)`. La borne couplée est stricte et elle est
atteinte dans le cas équilibré, assez séparé, par une perturbation de longueur
opposée à l'axe et une perturbation de milieu orthogonale. Elle améliore donc
réellement le cœur sans changer l'ordre de coût.

Une dérivation utile pour l'implémentation évite de voir Cauchy comme une
boîte noire. Avec `x=|v-u|`, `C=2(r_A^2+r_B^2)` et
`S=r_A+r_B`, la pénalité à majorer est

```text
G(x)=kappa_q x + (1/2)sqrt(C-x^2),  0<=x<=S.
```

Son maximum est atteint en
`x*=2 kappa_q sqrt(C)/sqrt(1+4 kappa_q^2)` ; pour les trois lanes,
`x*<=S`. On retrouve exactement la formule `R_coup` ci-dessus. Dans le cas
équilibré `r_A=r_B=r`, et dès que `d>=x*` — en particulier dans le régime WSPD
séparé considéré —, cette borne est même la plus grande boule commune de centre
`m` :

```text
R*_q = kappa_q d-r sqrt(1+4 kappa_q^2).
```

Sous la convention WSPD `d>=(s+2)r`, les seuils continus équilibrés deviennent
`s>2sqrt(2)-2` en q2, `s>2` en q3 et
`s>sqrt(1+4 sin^2(15°))/sin(15°)-2 = 2,351...` en q4. Ils sont nettement
meilleurs que ceux de `R_dec`. Ce sont des seuils de la géométrie continue,
pas des `iff` pour le code à rayon entier ni pour chaque rectangle non
équilibré.

Elle ne rend toujours pas le cœur maximal : le rayon exact de la meilleure
boule centrée en `m` serait l'infimum de

```text
kappa_q |c_B-c_A+v-u| - |u+v|/2
```

sur les deux boules d'incertitude. Ce petit problème robuste peut être gardé
comme oracle de fixture ou résolu plus tard ; les deux rayons ci-dessus sont
déjà des minorants analytiques sûrs.

### 3.4 Réception ciblée de `spindle_core_ball.hpp` au `6220ea3`

Le nouveau fichier implémente `R_dec` dans les unités quadruplées : distance
des centres minorée, rayons majorés, puis rationnels sous-approchés de
`2 kappa_q`. Les inégalités exactes qui justifient les constantes sont :

```text
3 * 577350^2 = 999999067500 < 10^12,
(2*10^12-517638^2)^2 > 3*10^24.
```

La seconde, avec `2*10^12-517638^2>0`, équivaut à
`517638/10^6 < sqrt(2-sqrt(3))`. Le code emploie bien le plancher, puis
`far<R4^2` pour `ALL`. `ball_disjoint_box` emploie `near>R4^2` : c'est sûr mais
un peu plus faible que `>=`, l'égalité ne contenant aucun point de la boule
ouverte.

Replays directs au pin :

| porte | résultat |
| --- | --- |
| tangence | six verdicts exacts, trois crédits, zéro faux |
| dérive | `234481` crédits, zéro faux ; le mutant sans dérive produit `1209` faux |
| seuil | dix lignes conformes ; les deux mutants de constantes meurent |
| `uniform,n=400,s=8` | direct=descente sur les trois lanes, `2107709` vérifications, zéro faux |
| `eight_clusters,n=400,s=8` | direct=descente, `317871` vérifications, zéro faux |
| `terrain,n=400,s=8` | direct=descente, `783581` vérifications, zéro faux |

Les six mutants déclarés meurent avec le code attendu. Cela reçoit la
primitive comme **sous-certificat**, pas les claims de fermeture, de coût ou
son raccord au préfiltre combiné.

Les mêmes fixtures et petits nuages passent sous ASan/UBSan dans le domaine
annoncé ; aucun faux témoin du chemin nominal n'a été trouvé.

Une affirmation de l'en-tête et de la note est toutefois fausse : arrondir le
rayon vers le haut d'une unité n'est pas toujours absorbé par la stricte
inégalité. La norme d'un vecteur de grille n'est pas nécessairement entière.
Deux fixtures u16 exactes :

```text
q3 : a=(0,0,0), b=(14,0,0), z=(7,1,4)
     R4 vrai=28/sqrt(3)=16,165..., |4z-M|^2=272.
     floor=16 rejette ; ceil=17 accepte ; 4H^2=4096<4356.

q4 : a=(0,0,0), b=(8,0,0), z=(4,1,2)
     R4 vrai=16 sin(15°)=8,282..., |4z-M|^2=80.
     floor=8 rejette ; ceil=9 accepte ; 3H^2=363<441.
```

Le chemin nominal, qui arrondit vers le bas, reste sûr. Mais le commentaire
« non-mutant » doit être retiré et `ceil` doit devenir un mutant causal tué par
ces deux points. La fixture actuelle à une seule coordonnée perpendiculaire ne
peut pas voir ce cas à norme irrationnelle.

Enfin, le rayon couplé de 3.3, le vrai `ceil_sqrt` des rayons d'endpoints et les
gardes u16 ne sont pas encore implémentés. Le probe vérifie son propre
range-count de boule ; il ne vérifie pas encore la parité du futur montage
`ball bulk + Corner512/64 complément + h_a + h_b`.

Le plancher final du rayon perd en outre jusqu'à presque une unité quadruplée.
On peut supprimer cette perte **sans** passer au plafond. Avec
`D=2^30`, `A_q=floor((2 kappa_q)D)` et

```text
T_q=A_q L2-S2 D,
```

un point ou le coin extrême d'une boîte est crédité exactement pour le rayon
fixe sous-approché lorsque

```text
T_q>0 et dist2 D^2<T_q^2.
```

Tout tient en `i128` sous u16. Les constantes sont certifiées à la compilation
par `3A_3^2<D^2` et, en posant `X=2D^2-A_4^2`, par
`X>0 && X^2>3D^4`. Q30 laisse assez de marge pour que cette dernière preuve
elle-même tienne en `i128`. Cette route conserve la stricte inégalité et tue
proprement le faux relâchement `+1`.

## 4. La convention WSPD du code change le tableau

Le code ne teste pas `d >= s max(r_A,r_B)`. Il teste :

```text
d-r_A-r_B >= s max(r_A,r_B).
```

Avec `r=r_A+r_B<=2 max`, la formule simple donne donc :

```text
R_dec,q/r >= kappa_q s/2 - 1/2.
```

| lane | seuil de garantie uniforme | `R/r`, s=6 | s=8 | s=10 |
| --- | ---: | ---: | ---: | ---: |
| q2 | `s>2.000` | `1.000` | `1.500` | `2.000` |
| q3 | `s>3.464` | `0.366` | `0.655` | `0.943` |
| q4 | `s>3.864` | `0.276` | `0.535` | `0.794` |

Le tableau `4.000/5.464/5.864` de la note est donc décalé de deux unités de
séparation. Même le tableau corrigé n'est pas un « si et seulement si » pour
chaque rectangle : c'est une garantie uniforme au pire cas équilibré. Un
rectangle plus éloigné peut avoir un rayon positif à un `s` inférieur.

Le code entier ajoute une nuance : `R_dec>0` en réel ne garantit pas que son
plancher entier soit positif. Avec un rayon de nœud `rmax2`, la garantie porte
sur `floor(alpha_q s rmax2)>2rmax2`, où `alpha_q` sous-approche `2kappa_q`.
Les petits nœuds sont donc le pire cas discret ; à `rmax2=1`, il faut par
exemple `s>=3` en q2 et `s>=6` en q3/q4 pour ce chemin entier particulier.
Le test fixe Q30 de 3.4 enlève le plancher final, mais pas l'obligation de
raisonner sur une boule ouverte pouvant ne contenir aucun point de grille.

La forme close donne une explication qualitative de l'effet de `s`, mais elle
n'explique pas à elle seule un facteur précis du reçu : le nombre et la
population des rectangles changent simultanément, et le reçu q2 historique
reste invalide.

## 5. Boule contre AABB : le sens de domination dépend de l'enveloppe

### 5.1 Avec le code actuel, la boule est dominée

`sphere_of(box)` construit une sphère **circonscrite à l'AABB**. Ainsi :

```text
Box(A) subset Sphere(A),  Box(B) subset Sphere(B).
```

Tout site du cœur-boule est donc universel sur les deux AABB et doit être
accepté par Corner64. Le cœur-boule est un sous-certificat de Corner64, pas un
certificat incomparable.

Exemple ponctuel : `a=(0,0,0)`, `b=(10,0,0)`, `z=(1,0,0)`. Le site passe q4
par colinéarité stricte, donc Corner64 l'accepte, mais
`|z-m|=4>10 sin(15°)` : il est hors de la boule q4.

Conséquences algorithmiques :

- un nœud entièrement dans la boule peut être crédité en bloc avant Corner512 ;
- un nœud disjoint de la boule ne peut **pas** être élagué de la recherche
  Corner64 ; il est seulement disjoint de ce sous-certificat ;
- Corner64 « seulement sur la frontière de la boule » perd des témoins ;
- boule en fast path, puis Corner512/Corner64 sur tout le complément, rend
  exactement la même fermeture que Corner64 seul, potentiellement plus vite.

### 5.2 Comment obtenir de vrais gains complémentaires

Construire une boule englobante des **PointId du nœud** qui ne prétend pas
contenir son AABB — MEB exacte, sphère de Ritter certifiée, ou centre AABB avec
rayon égal au maximum dirigé sur les points. Cette sphère et l'AABB deviennent
alors réellement incomparables tout en contenant chacune les données. La
réunion des deux certificats peut augmenter `h_coeur`, à condition d'unir les
IDs par lane au lieu d'additionner des populations qui se recouvrent.

Un gain immédiat, même sans changer de centre :
`sphere_of(box)` emploie `floor(sqrt(x))+1`, y compris quand `x` est un carré.
Le vrai plafond est :

```text
r=floor_sqrt(x); if (r*r<x) ++r;
```

Il donne rayon zéro aux singletons et augmente gratuitement les rayons de cœur
tout en restant englobant.

## 6. La bonne solution de comptage

### 6.1 `h_coeur` : une seule descente, sans perdre Corner64

Les trois boules ont le même centre et des rayons emboîtés. Une frame porte un
masque de lanes, exactement comme la réparation q2 :

1. `Box(Z)` strictement dans la boule de lane q : créditer `pop(Z)`, inscrire
   ses IDs dans le ledger de q, retirer q du masque des enfants ;
2. sinon, tester `corner512_all_lane(A,B,Z)` ; s'il rend `ALL`, créditer le
   bloc de la même façon ;
3. un échec du test boule est `UNKNOWN`, jamais `NONE` ; appliquer séparément
   un vrai certificat `NONE` ou descendre ;
4. à une feuille non créditée, employer Corner64 ;
5. saturer à `h_q`, mais conserver l'unicité des IDs avant saturation dans les
   fixtures.

Avec `sphere_of(box)`, cette route conserve exactement le `h_coeur` AABB de
Corner64 et ne change que son coût. Avec une sphère des points, elle peut aussi
gagner des IDs discrets.

### 6.2 `h_a/h_b` : la boule seule change le filtre

À `a` fixé, pour `z != a`, et `B` contenu dans `Ball(c,r)`, la boule midpoint n'est qu'une
petite sous-région. Contre-exemple q2/q3/q4 :

```text
A={a=(0,0,0), z=(1,0,0)},
B={(18,0,0),(22,0,0)},  c=(20,0,0), r=2.
```

`z` est universel pour les deux partenaires, par colinéarité et `H>0`. La
boule q2 proposée est centrée en 10, de rayon 8 : elle ne contient pas `z` ;
les boules q3/q4 sont plus petites encore. Une requête ball-only diminue donc
`h_a(a)`. Elle correspond à un nouveau minorant, pas à la branche « valeurs
inchangées » de Q23.

La région continue exacte face à la boule partenaire a pourtant une formule
fermée. Posons :

```text
e=z-a, u=e/|e|, t0=c-z,
s=u.t0, rho=sqrt(|t0|^2-s^2).
```

La boule entière `Ball(c,r)` est dans le cône admissible de lane q si et
seulement si :

```text
s sin(alpha_q) - rho cos(alpha_q) > r.
```

C'est la distance signée de `t0` au complément du cône ouvert. Pour q2, elle
se réduit à `e.(c-z)>r|e|`. En posant
`J=e.t0`, `E=|e|^2`, `Q=E|t0|^2-J^2`, les trois écritures sans normalisation
sont :

```text
q2 : J > r sqrt(E),
q3 : sqrt(3) J - sqrt(Q) > 2r sqrt(E),
q4 : sqrt(2) J - sqrt(Q) > sqrt(3) r sqrt(E).
```

Les côtés droits et les racines sont comparés avec arrondis dirigés. Cette
autorité **cône robuste** est maximale pour
la relaxation par une boule partenaire ; la boule midpoint n'en est qu'un
fast path.

Deux implémentations utiles :

- par ligne : requête de boule en fast path, puis fallback exact
  `point a x Box(B) x Box(Z)` à 64 coins sur tous les nœuds/IDs non crédités ;
- factorisée : auto-jointure ordonnée et disjointe `U(anchor) x Z(witness)`.
  Si `corner512_all_lane(U,B,Z)>=q`, faire un range-add `|Z|` à chaque feuille
  ancre de `U`. Sinon scinder `U` ou `Z`. Traiter les deux orientations de
  chaque paire de nœuds disjoints et supprimer la diagonale.

La seconde conserve des `h_a(a)` ponctuels : un verdict de bloc n'impose pas
une valeur commune, il ajoute paresseusement le même crédit à chaque ancre,
puis les crédits sont propagés aux feuilles. Le pire cas reste quadratique,
mais tout bloc `ALL` évite ses couples. Même construction pour `h_b`.

Toute requête `h_a` doit rester restreinte au pool `A`, et toute requête `h_b`
au pool `B`. Compter globalement casserait la disjonction additive.

### 6.3 Réception de la boule d'apex de `66b4f0c`

Le nouveau chemin fixe `u=c_B-a`, `D=|u|`, `e=z-a` et écrit
`b-z=u+(delta-e)`. Comme `|delta-e|<=r_B+|e|<=r_B+2r_A=N`, il obtient le
**cône suffisant constant**

```text
angle(e,u) < gamma_q = theta'_q-arcsin(N/D).
```

Si `gamma_q>0`, toute boule ouverte de centre `a+l u/|u|` et de rayon
`l sin(gamma_q)` est incluse dans ce cône. Puisque la descente reste limitée au
pool `A`, `l>0` peut effectivement être choisi pour la couverture sans changer
la sûreté. Les formules entières de `apex_ball_of` sous-approchent ensuite le
rayon et les comparaisons strictes sont dans le bon sens.

Deux qualifications sont indispensables.

Premièrement, ce cône n'est **pas** « la région exacte de `h_a` ». Le passage
de `|e|` à `2r_A` est uniforme et conservateur. Même avec `B={b}`, q2 donne,
sur une direction faisant l'angle `phi` avec `ab`, la condition
`|e|<|ab| cos(phi)` : la frontière dépend de la distance à l'apex et n'est pas
un cône. L'autorité exacte pour la relaxation sphérique reste le test de
distance signée de 6.2.

Deuxièmement, le code perd le signe en mettant
`sin(theta'_q-arcsin(N/D))` au carré. `W=U-N^2>0` ne garantit que `N<D` ; il
faut aussi, **avant** le carré :

```text
q3 : 3W>N^2,
q4 : 2W>N^2.
```

Sans ces gardes, le chemin optionnel n'est pas fail-open dans tout son domaine
CLI. Contre-fixture u16, déjà séparée à `s=1` :

```text
Box(A)=[(692,840,1000),(1308,1160,1000)], rA2=695,
Box(B)=[(1820,755,1000),(2180,1245,1000)], rB2=609,
d2-rA2-rB2=696 >= max(rA2,rB2)=695,
a=(1000,1000,1000), z=(1308,847,1000), b=(1938,1245,1000).
```

Pour q4, `U=4 000 000`, `N=1 999`, `W=3 999`. Le code accepte car

```text
1 198 524 000 000 < 1 760 229 561 475,
```

alors que le fuseau exact refuse :

```text
H=133 146, E=118 273, T=555 304,
3H^2-ET=-12 493 898 044.
```

Un montage temporaire de six PointId réalisant les deux AABB, avec
`smax=4,separation=1`, confronte la décision entière à l'oracle du probe : q4
ferme deux ancres, dont une à tort (`oracle_faux_morts=1`, code 1). Le défaut
atteint donc bien `h_a` puis la décision, pas seulement une primitive isolée.
Pour l'ancre `(a,b)` ci-dessus, le vrai compte vaut zéro, le faux témoin `z`
donne le compte un et atteint exactement `h_4=1`.

Le correctif est donc une comparaison entière constante, plus une fixture
`--separation=1` qui doit tuer l'ancien code. Les portes actuelles à `s=6` ne
peuvent pas voir ce défaut.

Enfin, la conclusion de complexité de la note est invalide : l'auto-jointure
s'arrête après `h_q` **succès**, pas après `h_q` essais. Si les témoins sont
rares ou tardifs, elle examine encore `Theta(|A|^2)` couples. La campagne
`n=4000,s=6` établit seulement que la boule unique est moins bonne sur ces
trois nuages. Les unités de `travail_ha` sont d'ailleurs différentes — visites
de nœuds contre prédicats ponctuels — et aucun brut chronométrique n'est
versionné. Cela justifie de ne pas adopter cette boule, mais ne ferme ni le
théorème de pire cas ni l'auto-jointure dual-tree de 6.2, qui n'a pas été
implémentée.

La justification numérique `N/D environ 3/s` de la note est également trop
grossière pour faire preuve. La séparation donne
`D>=s max(r_A,r_B)+r_B`, donc, dans le cas équilibré,
`N/D<=3/(s+1)`, pas une égalité `3/s`. Employer directement les inégalités
entières de signe ci-dessus évite toute ambiguïté de convention ou d'ancre.

Une amélioration intermédiaire plus fidèle à l'idée de Claude consiste à
compter le cône suffisant lui-même : il est convexe, donc huit coins dans le
cône certifient `ALL` pour une AABB de témoins ; un échec reste `UNKNOWN` et
descend. Cela domine toute boule unique inscrite dans ce cône. Plusieurs
valeurs de `l` peuvent aussi être unies par ledger d'IDs, mais le test du cône
évite directement ce choix.

Une fois le garde de signe posé, les largeurs `i128`, la stricte de
`apex_contains_box` et l'arrondi supérieur de la racine soustraite sont dans le
bon sens sous u16. `apex_disjoint_box` peut employer `>=` puisque la boule est
ouverte ; son `>` actuel est seulement moins élaguant. En revanche, les deux
fixtures apex ne confrontent que `apex_ball_contains` à un partenaire
ponctuel. Ajouter une porte `direct == tree` par ancre et lane, avec planchers
non nuls de bulk et d'élagage, pour recevoir `apex_ball_of`,
`apex_contains_box` et `apex_disjoint_box` ensemble. Enfin, dix-huit recherches
sans contre-exemple ne prouvent pas que remplacer le plafond de la racine
soustraite par son plancher soit inatteignable : conserver le plafond sain et
ne publier aucun « non-mutant » sans lemme réseau.

Les couvertures des fixtures doivent enfin sortir **par lane** : dans la
fixture serrée, q4 ne couvre que `71,258 %`, alors que l'agrégat des trois lanes
vaut `81,1 %`. Une moyenne agrégée ne peut pas recevoir le lane le plus étroit.
Le reçu doit aussi imprimer `ha_mode=jointure|boule` : sans ce champ, deux
algorithmes différents peuvent produire des lignes extérieurement
indiscernables.

### 6.4 Où se trouvent les ensembles réellement maximaux

Les définitions maximales de l'audit précédent restent :

```text
C_q(A,B) = {z hors A union B : forall a in A,b in B, z in W_q(a,b)},
A_q(a;B) = {z in A\{a} : forall b in B, z in W_q(a,b)},
B_q(A;b) = {z in B\{b} : forall a in A, z in W_q(a,b)}.
```

La convexité séparée donne un théorème utile : remplacer `A` et `B` par leurs
enveloppes convexes ne change pas ces quantificateurs sur les ensembles
discrets. Il suffit donc, exactement, de tester les sommets des enveloppes
convexes ; une AABB est un polytope extérieur plus gros et reste
conservatrice. En dimension trois, des hulls ou k-DOP de nœud fournissent une
échelle de compromis entre les 64 coins constants et les ensembles maximaux.

## 7. Arithmétique sûre pour les boules ouvertes

Les `kappa_q` et les distances sont irrationnels. « Ce sont des distances » ne
constitue pas une preuve de largeur ni d'arrondi. Une réalisation simple et
fail-open utilise un sous-approché fixe de `kappa_q`.

Dans les unités actuelles, `c2=2c` et `r2=2r`. Posons :

```text
g2 = floor(|c2_B-c2_A|)-r2_A-r2_B,
K_q = floor(2^F kappa_q),
R4 = floor(2 K_q g2 / 2^F) - (r2_A+r2_B).
```

Alors `R4 <= 4 R_dec,q`. Le centre en quadruple coordonnées est
`M4=c2_A+c2_B`, et un point entier `z` n'est crédité que si :

```text
R4>0 et |4z-M4|^2 < R4^2.
```

Pour un nœud AABB de témoins :

- `ALL` si sa distance maximale carrée à `M4` est strictement `<R4^2` ;
- disjoint si sa distance minimale carrée est `>=R4^2` ;
- sinon `MIXED` et descente.

Les maxima/minima par axe sont exacts. Les produits sont formés dans une
largeur démontrée (`i128` est le choix simple). La borne couplée demande de la
même manière un sous-approché du terme positif et un sur-approché dirigé de la
racine soustraite. Aucun `double` non encadré ne doit décider un crédit.

## 8. Ce que « k-NN » peut exactement vouloir dire

Pour le seul certificat cœur-boule, après exclusion des vrais IDs de
`A union B` :

```text
d_(h_q)(m) < R_q  <=>  le cœur-boule contient au moins h_q IDs.
```

Cela implique que toutes les ancres du rectangle meurent à cette lane. Ce
n'est pas une équivalence avec « l'ancre meurt » : `h_a+h_b` peut compléter le
seuil, et des sites intérieurs vrais peuvent ne figurer dans aucun certificat.

Un range-count saturé à `h_q` est plus direct qu'un k-NN générique et permet
les crédits de sous-arbres. Son coût est celui des nœuds visités. « Coût de
surface » est une intuition moyenne, pas une borne : l'octree Morton peut
visiter `Theta(n)` nœuds sur une requête, et `|A|` requêtes peuvent rester
quadratiques. Publier `nodes_visited`, `bulk_population`, `mixed_leaves`, HWM
et percentiles par famille avant toute revendication asymptotique.

## 9. Réponses constructives à Q21--Q25

- **Q21.** Oui : Corner512 est nécessaire et suffisant pour l'enveloppe
  continue AABB, par convexité séparée. Sur les PointId, seul son succès `ALL`
  est exploitable ; son échec est `UNKNOWN`.
- **Q22.** Corner64 est la spécialisation correcte au témoin ponctuel. Les
  résiduels appariés sont reçus `counter-only`; le coût à l'échelle demande un
  reçu brut et une machine épinglée.
- **Q23.** La boule d'apex unique de `66b4f0c` est plus faible, plus lente sur
  les trois mesures fournies, et actuellement fausse à `s=1`. Cela suffit pour
  ne pas l'adopter, pas pour fermer la route dual-tree non testée. Après le
  garde de signe, utiliser le cône comme fast path puis fallback 8/64 coins,
  ou l'auto-jointure hiérarchique avec range-add décrite en 6.2.
- **Q24.** Garder le bulk q2, le masque par frame et ajouter la parité
  permanente bulk/sans-bulk.
- **Q25.** Écrire partout « arête maximale canonique du support ». En q2
  seulement, préciser qu'elle est aussi la paire antipodale/diamétrale de la
  miniboule.

## 10. Plan d'action proposé à Claude

### P0 — avant toute mesure

1. borner réellement le profil u16 et `--oracle` ;
2. corriger dans les textes l'implication définissant `W_q` ;
3. rendre toutes les boules ouvertes et tuer le mutant `<=` par les deux
   shells entiers ci-dessus ;
4. aligner la formule de séparation sur `d-r_A-r_B>=s max` ;
5. ne jamais élaguer Corner64 au seul motif « hors du cœur-boule ».
6. dans `apex_sin2`, imposer `3W>N^2` en q3 et `2W>N^2` en q4 avant
   tout carré ; graver la contre-fixture `separation=1` et un oracle qui ferme
   réellement à tort sans ce garde.

### P1 — compter plus, plus vite, sans changer le filtre

7. vrai `ceil_sqrt`, puis rayons `max(R_dec,R_coup)` à arrondis dirigés ; pour
   `R_dec`, préférer le test fixe Q30 sans plancher final ; remplacer les
   boucles de racine depuis zéro et cacher les sphères par nœud ;
8. cœur : ball-ALL bon marché, Corner512-ALL sur le complément, Corner64 aux
   feuilles, ledger d'IDs par lane ;
9. `h_a/h_b` : fast path cône, autorité robuste exacte ou 64 coins en fallback,
   requêtes restreintes au bon pool ;
10. mode auto-jointure hiérarchique avec range-add ponctuel et ablation contre
   les boucles quadratiques ;
11. si un gain de fermeture au-delà de Corner64 est recherché, construire des
    sphères englobant les points plutôt que les AABB.

### P2 — réception

12. oracle `closed_prefilter subset true_dead` non vide sur les trois lanes ;
13. parité bulk/fallback, swap endpoint, doublons de position, shell, largeur ;
    ajouter explicitement le mutant rayon `+1` et une fixture à norme de grille
    irrationnelle ; pour l'apex, exiger aussi `direct==tree`, bulk et élagage
    non nuls, `apex_requetes>0` et une couverture publiée par lane ;
14. comparer séparément `h` obtenu, nœuds visités, temps et HWM ;
15. seulement alors régénérer la campagne et le reçu q2.

## 11. Statut de l'ancien audit

L'audit `AUDIT_PREFILTRE_COMBINE_HMAX_Q2_Q3_Q4_20260815.md` reste l'autorité
historique du défaut et des ensembles maximaux. Son verdict logiciel q2 au pin
`4cd1f82` est **résolu** par `3bf1bf3`; ses conclusions sur le sens de
l'implication, la maximalité, le coût total et les extrema q3/q4 restent
actives. Le présent document le supersède pour le `HEAD=66b4f0c`.
