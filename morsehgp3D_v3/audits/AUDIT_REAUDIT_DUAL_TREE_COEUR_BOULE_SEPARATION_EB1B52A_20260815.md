# Ré-audit de la suite donnée par Claude : dual-tree, cœur-boule et séparation

Date : 15 août 2026 UTC.

Pins audités : base `559cc649fb019eeecfea9495125cca7b50b44ad1`, puis
delta courant `eb1b52a62b9742262ca306f83b3a76ed993f7851`.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

Ce texte répond aux quatre notes de Claude postérieures au ré-audit du
préfiltre. Il supersède leurs conclusions de performance quand elles sont
contredites ci-dessous ; il ne supprime pas leurs preuves et réfutations
historiques.

> [!CAUTION]
> **Verdict court.** Je ne trouve pas de nouvelle fausse fermeture dans les
> chemins nominaux testés. La garde de signe de la boule d'apex est réparée,
> la borne couplée du cœur est mathématiquement sûre, et l'auto-jointure
> dual-tree conserve bien les comptes de l'autorité ponctuelle à huit coins.
>
> Deux conclusions quantitatives ne sont en revanche pas reçues.
>
> 1. Le gain dual-tree annoncé `2,2--3,0x` compare une baseline qui recalcule
>    trois fois un prédicat déjà multi-lane à un dual-tree qui ne le calcule
>    qu'une fois. Face à une baseline ponctuelle elle aussi fusionnée, le
>    dual-tree coûte `+0,22 %` à `+30,0 %` d'évaluations à `n=4000`.
> 2. À `terrain,n=32000`, `99,052 %` de la baisse de résiduel attribuée à
>    `s=8` vient de la diminution de la masse automatiquement laissée vivante
>    par `cap-cellule=512`. Le champ `cellule_max=482` invoqué pour exclure ce
>    biais est calculé **après** le rejet des rectangles hors cap.
>
> Le dual-tree est donc reçu comme transformation sémantique, pas comme
> optimisation. L'arbitrage `s=6/s=8` doit être refait à cap neutralisé. Le
> mode `--vrai-vivant` ajouté au `eb1b52a` est exact seulement conditionnellement
> à `masse_non_decide=0` et à la sûreté déjà prouvée du préfiltre ; sa porte
> actuelle ne peut pas prouver cette sûreté. Enfin, le retrait de
> l'échantillonneur repose sur une confusion entre erreur absolue et relative :
> ses neuf écarts sont tous à moins de `1,52 sigma` binomial.

## 1. Socle mathématique inchangé

Soit une base minimale positive `S` de taille `q`, sa miniboule `B`, et
`I_B=P inter B°`. Tout simplexe de Gabriel ayant cette miniboule doit contenir
`I_B` : sinon un site de `I_B` extérieur au simplexe serait précisément un
intrus de l'intérieur ouvert. Donc

`|sigma| >= q + |I_B|`.

Ainsi `h_q=s_max-q+1` sites intérieurs stricts **distincts** suffisent à tuer
toute ancre d'arité `q`, y compris hors position générale. À `s_max=11`, les
seuils restent `10/9/8`. Le shell n'est jamais crédité. Les ensembles
`C_q`, `A_q(a;B)` et `B_q(A;b)` du préfiltre sont disjoints par `PointId`, et
leur somme reste un minorant licite de `|I_B|`.

Ce pont avec le manuscrit est reçu depuis le premier audit. Les changements
présents portent sur la manière de calculer ces trois minorants et sur leur
coût, pas sur le seuil de mort.

## 2. Garde de signe et domaine u16 : reçus

Pour la boule d'apex, poser `U=D^2`, `W=U-N^2`. Le carré employé dans
`apex_sin2` n'est licite qu'après avoir vérifié le signe de `gamma_q`. Les
conditions exactes sont désormais imposées avant le carré :

```text
q2 : W > 0,
q3 : 3W > N^2,
q4 : 2W > N^2.
```

Elles équivalent à `gamma_q>0`; leur stricte est correcte. La fixture u16 à
`separation=1` et le mutant `apex-sans-garde` rendent la réparation causale.
Les gardes `coord<=65535`, `n<=oracle`, parsing intégral et validation avant
cast ferment également les entrées hors contrat trouvées au ré-audit.

Verdict : `cf6ee5e` est reçu sur ce périmètre.

## 3. Auto-jointure dual-tree

### 3.1 Théorème de conservation : reçu

Fixons un rectangle CK `A x B`. Pour `a,z` fixés, le partenaire admissible est
un cône convexe ; ses huit coins décident donc exactement l'universalité sur
`Box(B)`. Plus généralement, le prédicat est séparément convexe en `a`, `b` et
`z`. Tester les coins distincts de `Box(U) x Box(B) x Box(Z)` décide exactement
le statut `ALL` de cette enveloppe continue.

La récursion saine partitionne ensuite les couples ordonnés :

- `(U,U)` devient `(Ul,Ul)`, `(Ul,Ur)`, `(Ur,Ul)`, `(Ur,Ur)` ;
- un produit hors diagonale est scindé d'un seul côté ;
- seule la paire `(feuille, même feuille)` est retirée ;
- un bloc `ALL` ajoute `|Z|` à chaque ancre de `U` par range-add ;
- la lane créditée est retirée du masque avant toute descente.

Chaque couple `(a,z)`, `a!=z`, contribue donc exactement une fois par lane.
Le cutoff remplace seulement un sous-arbre par le même prédicat ponctuel. Enfin
`min(sum,h_q)` est appliqué après la somme ; le résultat est exactement le
compte ponctuel **après saturation**, seule valeur consommée par l'histogramme.

Le mutant sans masque réintroduit bien le défaut historique : `212` puis
`1525` fausses morts en régime tendu. La conservation du chemin sain est donc
reçue, relativement à l'autorité AABB à huit coins.

### 3.2 Le gain `2,2--3,0x` est un artefact de baseline

La baseline `--ha=corner8` boucle extérieurement sur `q=2,3,4`, alors que
`corner8_lane(a,B,z)` rend déjà la meilleure lane en une seule passe. Le même
couple est donc évalué jusqu'à trois fois. Le dual-tree fusionne les lanes par
construction : le rapport publié mélange deux optimisations.

J'ai instrumenté hors dépôt une baseline équitable : un seul appel
`corner8_lane(a,B,z)` alimente trois compteurs actifs, puis la boucle s'arrête
quand les trois seuils sont saturés. Elle produit exactement les mêmes
`h_a/h_b` et les mêmes survivantes. À `n=4000`, `s=6`, graine `3`, cutoff
dual `256` :

| famille | ponctuel multi-lane | dual-tree | surcoût dual |
| --- | ---: | ---: | ---: |
| `terrain` | `60 505 070` | `78 671 661` | `+30,0 %` |
| `uniform` | `55 995 548` | `56 120 733` | `+0,22 %` |
| `eight_clusters` | `60 405 438` | `64 949 890` | `+7,52 %` |

À `n=1000`, le dual est égal sur `uniform` et encore plus cher sur les deux
autres familles. Le cutoff `64` ou `256` n'est pas un théorème de rentabilité :
coins dupliqués, sorties anticipées et fréquence des blocs `ALL` modifient le
rapport réel entre test de bloc et tests ponctuels.

Verdict : retirer « adopté » et `2,2--3,0x`. La vraie optimisation reçue est la
**fusion des trois lanes**. Le dual-tree peut rester un diagnostic ou une
future primitive GPU, mais doit battre cette baseline avant promotion.

### 3.3 La porte d'égalité ne reçoit que le côté A

`--verifie-jointure` confronte `dA` à la référence, jamais `dB`. La symétrie du
lambda rend une faute peu probable, mais ne teste ni les offsets de `B`, ni la
seconde invocation, ni le mutant `oublie-b`. Publier séparément
`verifies_A/ecarts_A` et `verifies_B/ecarts_B`, puis tuer un mutant limité à
`dB`.

De même, `oracle_ids_doubles` journalise le cœur, pas les couples
`(ancre,témoin,lane)` de `h_a/h_b`. Ce compteur ne reçoit donc pas le masque du
dual-tree ; c'est l'oracle de fausses morts qui tue aujourd'hui le mutant.

### 3.4 Plus grands `h` : exact AABB n'est pas exact discret

Le dual-tree conserve le plus grand compte permis par la relaxation
**continue AABB**. Ce n'est pas le plus grand ensemble universel sur les points
occupés. Contre-exemple q2 u16 :

```text
a  = ( 999,1001,1000)       z  = (1000,1000,1000)
b1 = (1130,1110,1000)       b2 = (1090,1070,1000).
```

Avec `e=z-a=(1,-1,0)`, `e.(b1-z)=e.(b2-z)=20>0` : `z` est témoin pour les deux
vrais partenaires. Mais `Box(B)` contient le coin fictif
`b*=(1090,1110,1000)`, pour lequel `e.(b*-z)=-20`; le test aux coins refuse.
Le rectangle est pourtant séparé dès `s=3` : dans les unités du probe,
`284-2-57=225 >= 3*57`.

La route mathématique exacte est simple. À `a,z` fixés, l'ensemble des
partenaires acceptés est convexe, donc

`B subset admissible <=> conv(B) subset admissible`

et il suffit de tester les sommets de `conv(B)`. Pour le cœur, la convexité
séparée donne de même les couples de sommets de `conv(A) x conv(B)`. Cela donne
les vrais ensembles maximaux sous les dépendances autorisées, sans coins
fantômes. Le pire cas peut rester quadratique en nombre de sommets ; la voie
pratique est une échelle fail-open :

1. AABB/coins en fast path ;
2. k-DOP, OBB ou enveloppe convexe préstockée au nœud ;
3. descente hiérarchique sur `UNKNOWN`, avec budget ;
4. budget épuisé : ne pas créditer.

Une union de certificats se fait par `PointId`, jamais par addition de comptes
susceptibles de se chevaucher.

## 4. Borne couplée et cœur-boule

### 4.1 La borne couplée est sûre

Écrire `a=c_A+u`, `b=c_B+v`, `p=(u+v)/2`, `w=(v-u)/2`. Alors

`|p|^2+|w|^2=(|u|^2+|v|^2)/2`

et Cauchy majore la perte commune par

`sqrt((4 kappa_q^2+1)(r_A^2+r_B^2)/2)`.

Donc

`R_coup=kappa_q d-sqrt((4 kappa_q^2+1)(r_A^2+r_B^2)/2)`

est sûr ; `max(R_dec,R_coup)` l'est aussi. Les facteurs entiers `4`, `8/3` et
`6-2sqrt(3)`, les directions d'arrondi et les largeurs `i128` du chemin nominal
sont corrects sous u16. Je reçois le théorème et son implémentation.

La saturation causale n'est toutefois gravée qu'en q2. Les branches
irrationnelles q3/q4 demandent chacune une fixture de frontière et un mutant
de coefficient propre. Le commentaire disant que le mutant
`coup-plancher-racine` déborde nécessairement d'une unité n'est pas démontré
par la fixture actuelle : conserver le plafond par preuve, mais ne pas appeler
ce mutant causal sans faux témoin explicite.

### 4.2 La composition exécutée n'est pas celle revendiquée

Avec `sphere_of(Box)`, la sphère circonscrit l'AABB. Le cœur-boule est donc un
sous-certificat de Corner64. L'identité des comptes suit **si et seulement si**
le complément est effectivement traité par Corner64.

Or `--coeur=boule` n'active pas `--coeur=corner64`. Les CTests nouveaux
composent la boule avec `Hmin/Ximax`, et le reçu imprime même
`coeur_mode=bornes`. Sur `uniform,n=160`, boule OFF/ON :

```text
hcore q3 : 34527 -> 34528
hcore q4 : 28062 -> 28064
```

Les survivantes restent égales sur ce cas, mais l'égalité des **comptes** est
déjà fausse. La domination par Corner64 ne prouve rien sur la composition avec
une borne plus faible.

Deux options propres :

- définir `--coeur=boule` comme `boule+corner64`, puis gater l'égalité ;
- garder `boule+bornes`, mais la nommer ainsi et mesurer son gain propre.

Le reçu doit distinguer `bornes`, `corner64`, `boule+bornes` et
`boule+corner64`, ainsi que `rayon=dec/max` et la sphère d'endpoints. Une mesure
ne peut pas être reçue si deux algorithmes impriment la même ligne.

### 4.3 Le ledger q3/q4 est incomplet

Les bulk-crédits de la boule poussent leurs IDs dans `core_ids[q]`. Aux
feuilles, en revanche, les deux branches ne poussent que q2 ; q3 et q4
incrémentent leur compte sans journaliser l'identité. Ainsi
`oracle_ids_doubles=0` ne peut pas voir le montage exact
`bulk q3/q4 + redescente + feuille`.

Le masque sain est convaincant par preuve, et un sweep indépendant de
27 configurations (`3` familles, `s=1/4/8`, `smax=4/11/32`, `n=200`) donne les
mêmes lignes avec la boule OFF/ON devant Corner64. Mais ce n'est pas une porte
versionnée. Journaliser chaque crédit feuille dans les trois lanes, ajouter un
mutant `boule-sans-masque` distinct de l'ancien bulk q2, et exiger une fausse
mort q3 ou q4 sur une fixture tendue.

### 4.4 Deux pertes gratuites et un commentaire faux

- `sphere_of` dans le probe combiné et `h_sphere_sep` dans le probe cœur font
  encore `floor_sqrt(x)+1`. Sur un carré parfait, le vrai plafond est le floor ;
  sur un singleton, le rayon doit être `0`, pas `1`. Employer
  `r=floor_sqrt(x); if (r*r<x) ++r`. C'est sûr et augmente gratuitement les
  cœurs.
- Pour une boule ouverte, une boîte tangente est déjà disjointe : le test exact
  est `near2>=R2`. Le `>` actuel reste sûr mais descend inutilement.
- `spindle_core_ball.hpp` affirme encore que `R4+1` est absorbé par la stricte.
  C'est faux car les distances de grille sont des racines irrationnelles. En
  q3, `a=(0,0,0)`, `b=(7,0,0)`, `z=(3,2,0)` : le rayon quadruplé vrai vaut
  `14/sqrt(3)=8,083...`; le floor `8` refuse, `+1=9` accepte, tandis que le
  fuseau exact donne `4H^2=256<260=ET`. Restaurer le mutant `+1` et sa fixture.

### 4.5 La boule extérieure est réellement redondante

Ici la conclusion de Claude peut devenir un théorème. Si `Z` est disjointe de
la boule extérieure, alors `H(a,b,z)<=0` pour tout `a,b,z` des trois boîtes.
En particulier, pour chaque couple de coins d'endpoints,
`max_{z in Z} H<=0`. Le minimum de ces maxima, qui est exactement le majorant
séparable calculé par `h_any_upper`, est donc `<=0`. Placée après
`h_any_upper`, la boule extérieure ne peut jamais couper un nœud supplémentaire.

Conserver éventuellement le compteur comme réfutation exécutable, mais écrire
cette implication plutôt qu'une extrapolation de trois nuages.

### 4.6 Portes encore vacues ou incomplètes

Les CTests combinés `--ha=boule` contraignent l'oracle global, mais aucun
compteur propre à l'apex. Un chemin devenu no-op passerait. Publier par lane
`apex_requetes/non_vides/bulk/feuilles/elagages`, intégrer la fixture de signe
au probe combiné et comparer `direct==tree` par ancre.

Dans `core_ball_probe`, `--points=40 --coord=1` produit un seul point, zéro
rectangle et sort pourtant code zéro : la taille réellement générée n'est pas
comparée à `n`. C'est une porte entièrement vacue. Refuser tout nuage tronqué.
Enfin `coord` est une étendue exclusive : `coord=65535` n'exerce jamais la
coordonnée `65535`, tandis que le contrat annonce u16 inclusif. Soit autoriser
l'étendue `65536`, soit graver une fixture explicite contenant `0` et `65535`.
Les options flottantes encore lues par `atof` doivent être parsées jusqu'en fin
de chaîne, et les intervalles apex doivent imposer `lo<=hi`.

Les ratios de sphères des points (`0,969/0,960`) ne sont pas émis par le probe
avec leur agrégation ni accompagnés d'un brut. Une moyenne de rayons ne borne
pas les rectangles responsables des fermetures et ne clôt pas, à elle seule,
la piste des enveloppes centrées autrement.

## 5. `s=6` contre `s=8` : le cap explique le facteur `6,4`

Le probe traite tout rectangle dont une extrémité dépasse `cap=512` comme
indécidé et ajoute **toute** sa masse aux survivantes. À `terrain,n=32000` :

| `s` | résiduel q4 total | masse hors cap | résiduel jugé | fermeture sur masse jugée |
| ---: | ---: | ---: | ---: | ---: |
| `6` | `58 684 461` | `55 957 870` | `2 726 591` | `99,402 %` |
| `8` | `9 117 531` | `6 860 970` | `2 256 561` | `99,553 %` |

Le gain total vaut `49 566 930` ancres ; la seule baisse de masse hors cap vaut
`49 096 900`, soit **`99,052 %` du gain**. La conclusion « le cap n'est pas en
cause car `cellule_max=482` » est inversée : `cellule_max` est mis à jour après
le `continue` des rectangles hors cap, donc il est par construction inférieur
au cap. Le même défaut existait déjà au pin de l'ancien reçu ; `96,70 %` de son
écart s6/s8 venait de cette masse.

À `n=8000`, aucun rectangle n'est hors cap dans les deux régimes. Le gain de
`17,503 %` sur le résiduel y est donc bien une mesure du filtre courant. Sur la
seule masse jugée à `n=32000`, le gain vaut `17,238 %` : l'effet intrinsèque
observé est presque invariant. La mesure agrégée dit surtout que `s=8` raffine
assez la WSPD pour contourner le cap ; elle ne prouve pas qu'une séparation
`s=6` est géométriquement insuffisante.

Le seuil `2,4 us/ancre` reste un calcul opérationnel valable entre **ces deux
exécutables bornés**, mais ne sélectionne pas `s=8` contre l'alternative
`s=6 + traitement des gros rectangles`. Il suppose aussi des ensembles de
survivants comparables. Or le minorant additif n'est pas manifestement monotone
par raffinement : un témoin qui quitte `A` peut cesser d'être compté dans
`h_a(a;B)` et n'entrer dans le cœur que s'il devient universel pour tous les
autres `a'` du nouveau nœud. Il faut comparer les vrais `PairId`, pas seulement
les cardinaux.

### Protocole cap-aware demandé à Claude

1. publier `max_cellule_brut`, un histogramme de `max(|A|,|B|)` et
   `masse_non_decide` avant tout `continue` ;
2. neutraliser le cap en scindant récursivement tout endpoint trop gros, ce qui
   conserve exactement la partition de paires, ou balayer plusieurs caps ;
3. comparer `s=6/8/10` à stratégie de cap identique ;
4. matérialiser à petit/moyen `n` les ensembles de `PairId` survivants :
   intersection et deux différences ;
5. mesurer `uniform`, `eight_clusters` et plusieurs graines à `n=32000` avant
   de retirer les lignes `s=6` de la campagne.

## 6. Addendum `eb1b52a` : le « vrai vivant »

### 6.1 Le bon objet mathématique

Définir, pour chaque lane,

`V_q={(a,b): ||a-b||>0, |P inter W_q(a,b)|<h_q}`.

Il s'agit des ancres qui survivent au **critère idéal de témoins `W_q`**. Cet
ensemble est indépendant de la WSPD et de sa séparation. Il contient encore
des ancres sans aucune complétion positive ; ce n'est donc pas l'ensemble des
ancres réellement porteuses de supports. Le nom `W-vivantes` serait plus exact
que `vraiment vivantes`.

Si `S_q` est le résiduel du préfiltre et si sa sûreté est déjà établie, alors
`V_q subset S_q`. Tester exactement chaque paire de `S_q` suffit donc à compter
`V_q`. Sous ces deux préconditions, l'idée de Claude est juste et le mou
`|S_q|/|V_q|` mesure ce qu'un resserrement du **même critère de témoins** peut
encore gagner.

Cela ne borne pas tout le générateur : positivité, owner ou absence de
complétion peuvent encore retirer des ancres de `V_q`.

### 6.2 Le cap rend le compte faux

Le code quitte le rectangle hors cap avant le bloc `vrai_vivant`. Ces paires
sont ajoutées à `S_q`, mais aucune n'est testée pour `V_q`. Le reçu est donc
exact seulement si `masse_non_decide=0`, condition qui n'est ni imposée ni
imprimée comme statut du compte. Si cette masse est non nulle, le compte publié
est seulement une borne inférieure de `|V_q|` et le mou publié une borne
supérieure sans garantie d'utilité.

Contre-rejeu sur les mêmes 60 points `uniform`, `seed=3`, `s=8`, `smax=11` :

| cap | masse non décidée | `q4_vivantes` publié |
| ---: | ---: | ---: |
| `512` | `0` | `1594` |
| `1` | `492` sur `1770` | `1201` |

Les deux exécutions sortent code zéro et se disent exactes. C'est précisément
le biais qui invalidait la mesure s6/s8 à `n=32000`.

Correctif minimal : refuser `--vrai-vivant` dès que
`masse_non_decide!=0`. Correctif complet : tester aussi toutes les paires des
rectangles hors cap, puisqu'elles appartiennent par définition au résiduel, ou
les scinder jusqu'au domaine décidé.

### 6.3 L'invariant de sûreté est circulaire

Le code n'incrémente `vrai_vivantes` qu'après avoir établi que la paire est
dans `S_q`. Il en résulte structurellement

`vrai_vivantes <= nombre de paires parcourues dans S_q <= survivantes`.

La porte `survivantes >= vraiment vivantes` ne peut donc pas découvrir une
paire de `V_q` que le préfiltre aurait fermée à tort : cette paire n'est jamais
examinée. C'est une conséquence de la boucle, pas un oracle de sûreté.

La fixture historique le démontre directement : avec
`--fixture=coeur5 --inject=bulk-sans-masque --juge=7 --vrai-vivant`, le mutant
ferme une vraie paire q2, mais le nouveau mode imprime tout de même
`q2_vivantes=20`, `q2_mou=1.000`; il a simplement omis la vingt-et-unième
paire. Seul l'ancien oracle indépendant signale la fausse mort.

Le mode reste une mesure valide **conditionnée** à la preuve fail-open. Pour
recevoir indépendamment la sûreté, conserver à petit `n` l'oracle qui calcule
`V_q` sur toutes les `C(n,2)` paires, puis teste `V_q subset S_q`. Les CTests
nouveaux ne contraignent en outre que `q2_vivantes>0`, sans valeur attendue ni
non-vacuité q3/q4 ; exiger les trois lanes, `masse_non_decide=0`, une égalité
s6/s8 sur le même nuage et un mutant qui force une paire de `V_q` hors de
`S_q`. Le reçu étant imprimé avant les gardes finales, ne pas laisser une seule
`PASS_REGULAR_EXPRESSION` porter le verdict : doubler chaque golden par le code
de sortie attendu.

### 6.4 Le coût reste cubique au pire cas

Le coût est bien output-sensitive : `O(n^2+n sum_q |S_q|)` dans le code à
trois scans, ou `O(n^2+n |union_q S_q|)` après fusion. Aucune borne
`o(n^2)` sur le résiduel n'est prouvée, donc le majorant garanti reste
`O(n^3)`. Dès qu'une future entrée autorise des identités à positions
coïncidentes, `D=0` donne même un pire cas explicite : toutes les paires de ces
identités survivent et chaque scan atteint les `n-2` sites. La seule borne
`n<=40000` autorise environ `9,6 10^13` appels dans la version à trois lanes et
n'est pas un budget.

Deux économies immédiates reprennent exactement la leçon du dual-tree :

1. `corner8_lane(a,Box(b),z)` évalue huit fois le même coin quand `b` est
   ponctuel ; employer un prédicat point--point multi-lane unique ;
2. la boucle extérieure recalcule actuellement le scan pour q2, q3 et q4 ; un
   seul scan de `z` doit alimenter trois compteurs et éteindre chaque lane à son
   seuil.

La garde `n<=40000` est de plus placée après génération, octree et
matérialisation WSPD ; le test nommé « refus avant calcul » ne reçoit pas ce
claim. La route robuste est en deux passes : calculer d'abord `S_q` et son
masque par `PairId`, publier `|S|`, refuser si `n*|S|` dépasse un budget
explicite, puis effectuer un scan ponctuel multi-lane. Elle doit aussi retirer
les paires de positions identiques `D=0`, invalides comme ancres même si leurs
`PointId` restent présents avec tous les autres partenaires.

### 6.5 Interprétation du mou

Pour `mu=|S|/|V|`, la fraction maximale du résiduel que ce seul critère peut
encore fermer vaut `1-1/mu`, non `mu-1`. Ainsi `mu=1,537`, `1,288` et `1,747`
correspondent respectivement à `34,9 %`, `22,4 %` et `42,8 %` du résiduel
retirable au mieux par un calcul plus exact de `h_coeur+h_a+h_b`. Si `V=0`,
le ratio n'est pas `0.000` : il vaut `+inf` lorsque `S>0`, et doit être publié
`NA` pour `S=V=0`.

### 6.6 L'échantillonneur n'a pas la variance anormale annoncée

Soit `T=C(n,2)`, `V` le nombre `W-vivant`, `p=V/T`, et `X` le nombre vivant
parmi `K` paires uniformes indépendantes. Alors

`V_hat=T X/K`

est sans biais et, sous le modèle binomial,

```text
écart-type de X/K             = sqrt(p(1-p)/K),
écart-type relatif de V_hat/V = sqrt((1-p)/(Kp)).
```

Claude a comparé les erreurs **relatives à `V`** à la première quantité,
exprimée en points de proportion totale. Pour `uniform,n=600,q4`,
`T=179700`, `V=45913`, donc `p=0,2555`. Les écarts-types relatifs corrects sont
`2,414 %`, `1,207 %`, `0,604 %` pour `K=5000/20000/80000`, et non
`0,62/0,31/0,15 %`. Les neuf écarts publiés sont tous entre `-1,50` et
`+1,52` écart-type : ils sont parfaitement compatibles avec le binomial.

Le premier xorshift réduit par modulo pouvait justifier une amélioration du
générateur, mais ces chiffres ne prouvent aucun excès de variance. La voie
échantillonnée peut rester utile quand le scan exact dépasse le budget :
publier `X,K,T`, un intervalle de Wilson ou de Clopper--Pearson pour `p`, puis
le transformer par `T`. Ne jamais comparer une erreur relative à un écart-type
absolu.

## 7. Plan d'action adressé à Claude

### P0 de réception scientifique

1. Rétracter les claims `dual-tree adopté`, `2,2--3,0x` et le facteur `6,4`
   attribué aux certificats.
2. Faire de la baseline ponctuelle multi-lane la référence de travail.
3. Régénérer l'arbitrage de séparation après neutralisation explicite du cap.
4. Qualifier `--vrai-vivant` de mesure conditionnelle de `V_q`, refuser toute
   masse hors cap et retirer l'invariant circulaire comme preuve de sûreté.
5. Passer ce compte en deux passes, avec budget `n*|S|`, paires `D>0` et scan
   point--point multi-lane unique.
6. Corriger la normalisation de variance de l'échantillonneur et publier un
   intervalle binomial ; les neuf écarts actuels ne justifient pas son retrait.

### P1 de sûreté et de falsifiabilité

7. Compléter le ledger q2/q3/q4 aux feuilles et tuer un mutant de masque propre
   au bulk cœur-boule q3/q4.
8. Vérifier séparément `h_a` et `h_b` dans la porte dual-tree.
9. Versionner une égalité boule OFF/ON devant le **même** fallback Corner64.
10. Rendre les reçus injectifs sur les modes réellement exécutés.
11. Ajouter les frontières couplées q3/q4 et le mutant causal `rayon+1`.
12. Refuser tout nuage généré dont le cardinal diffère de `--points`, et exercer
   réellement les deux coordonnées u16 extrêmes.

### P1 de maximalité des `h`

13. Corriger le vrai plafond des sphères AABB et la tangence de la boule ouverte.
14. Employer Corner64 au cœur et le test huit coins multi-lane pour `h_a/h_b`
    comme baseline AABB exacte.
15. Expérimenter le fallback par enveloppes convexes ou hiérarchie de polytopes
    pour récupérer les témoins rejetés seulement par des coins fantômes.

### P2 de performance

16. Recalibrer ou retirer le dual-tree contre la baseline fusionnée, avec temps
    de paroi et unités homogènes.
17. Sortir par lane les nombres de blocs `ALL`, branches cutoff et feuilles ;
    un cutoff se choisit sur ces coûts observés, pas sur `512/8` seul.

## 8. Contrôles exécutés

- compilation directe C++20, `-O2 -Wall -Wextra -Werror`, des deux probes ;
- fixtures garde de signe, seuil/saturation couplés et mutants associés ;
- oracles combinés sur trois familles, y compris régimes `smax=32,s=1` ;
- sweep métamorphique cœur-boule OFF/ON sur 27 configurations devant Corner64 ;
- comparaison ponctuel multi-lane/dual-tree à `n=1000` et `n=4000` ;
- rejeu `terrain,n=8000,s=6/8`, sans masse hors cap ;
- décomposition exacte des reçus `terrain,n=32000,s=6/8` en masse jugée et
  masse hors cap ;
- contre-rejeu `vrai-vivant` cap `512/1` et mutant historique de fausse mort ;
- invariance du compte `W-vivant` entre `s=1` et `s=8` lorsque la masse hors cap
  est nulle ;
- reproduction exhaustive `uniform,n=600` et renormalisation des neuf tirages
  binomiaux publiés ;
- UBSan ciblé au bord u16.

Aucun fichier source ou CMake n'a été modifié par l'auditeur. Les corrections
ci-dessus sont volontairement laissées à Claude ; les seules écritures de
l'audit portent sur les README, `PROPOSITION.md` et `audits/`.
