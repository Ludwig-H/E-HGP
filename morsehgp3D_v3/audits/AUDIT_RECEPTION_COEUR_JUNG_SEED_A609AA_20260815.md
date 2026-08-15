# Réception mathématique du cœur de Jung par seed

Date : 15 août 2026 UTC.

Document audité :
[`AUDIT_SUIVI_PORTEUR_AIGU_GATEWAY_JUNG_207B542_20260815.md`](AUDIT_SUIVI_PORTEUR_AIGU_GATEWAY_JUNG_207B542_20260815.md),
commit `a609aa63b63dadf913875fc05fba88a91f5562e5`.

Contexte fonctionnel : `q4seed_axis_topr4.hpp` au même pin.

Cadre : `phase=exploration_v3_hors_registre`, `backend=math_reference`,
`profile=quantized_u16_input_only`, `mode=seed_permanent_core_review`,
`public_status=not_claimed`. GCP non utilisé.

> [!IMPORTANT]
> **Verdict court.** Le théorème du cœur permanent est correct et constitue un
> excellent fast path avant `Q4SeedAxisTopR4` : pour un seed aigu dont `ab` est
> l'arête owner, la boule centrée au circumcentre plan et de rayon `|ab|/4` est
> contenue dans toute boule q4 admissible par Jung. Huit vrais `PointId`
> distincts dans ce cœur tuent donc le seed sans calculer une racine.
>
> Trois corrections sont toutefois nécessaires avant implémentation :
>
> 1. l'inégalité large `<=` au rayon `|ab|/4` est **sûre**, donc ce n'est pas un
>    mutant létal ;
> 2. les passes `SeedCoreQuarter`, `SeedJungPermanent16` et le compteur permanent
>    déjà présent dans le noyau axial ne doivent jamais additionner deux fois le
>    même ID ;
> 3. le raccord exige un ledger de spans/IDs, pas un simple entier `p`, sinon le
>    replay du census et l'exact-once restent circulaires.
>
> Ces réserves ne diminuent pas l'intérêt du résultat. Elles évitent seulement
> de réintroduire, sous un autre costume, le double crédit parent-enfant déjà
> rencontré trois fois dans le dossier.

## 1. Paramétrisation reçue

Pour le seed

```text
T=(a,b,x),
d=b-a,
u=x-a,
D=d dot d,
E=u dot u,
F=d dot u,
G=DE-F^2=||d cross u||^2,
n=d cross u,
W=E(D-F)d+D(E-F)u,
```

les centres des sphères passant par `T` sont

```text
c(tau)=a+(W+tau n)/(2G).
```

Le circumcentre dans le plan du seed vaut

```text
c0=a+W/(2G).
```

Comme `W dot n=0` et `||n||^2=G`, en posant

```text
s=|tau|/(2 sqrt(G)),
```

on obtient exactement

```text
||c(tau)-c0||=s,
R(tau)^2=R0^2+s^2.
```

Si `ab` est l'arête maximale du futur q4 positif, Jung impose

```text
R(tau)<=RJ=sqrt(3D/8).
```

Le déplacement du centre est donc borné par

```text
0<=s<=T=sqrt(RJ^2-R0^2).
```

Ce domaine symétrique est plus large que le domaine réellement positif ou
compatible avec un apex donné. L'employer pour un cœur commun est donc sûr et
conservateur.

## 2. Intersection commune exacte

Soit `e_n` le vecteur normal unitaire au plan du seed et écrivons, pour
`p=z-c0`,

```text
p_n=p dot e_n.
```

La puissance de `z` dans la sphère de centre `c0+t e_n` est

```text
||p-t e_n||^2-(R0^2+t^2)
  = ||p||^2-2t p_n-R0^2.
```

Le maximum pour `t dans [-T,T]` vaut donc

```text
||p||^2+2T|p_n|-R0^2.
```

Ainsi l'intersection ouverte de **toutes** les boules de Jung incidentes au
seed est exactement :

```text
K_J(T)
 = {z : ||z-c0||^2+2T |(z-c0) dot e_n| < R0^2}.
```

Ce n'est pas seulement une boule. C'est le domaine convexe décrit, dans le
noyau existant, par les deux puissances d'extrémité :

```text
P_z(-tau_max)<0,
P_z(+tau_max)<0.
```

Comme `P_z(tau)` est affine en `tau`, les deux bouts sont nécessaires et
suffisants. La proposition `SeedJungPermanent16` est donc reçue mathématiquement
comme autorité `ALL_INTERIOR` sur un nœud AABB, sous arithmétique exacte.

## 3. La boule centrée maximale et le cœur rationnel

Le plus grand rayon d'une boule centrée en `c0` incluse dans `K_J` vaut

```text
rho=min_{0<=s<=T} [sqrt(R0^2+s^2)-s]
   =RJ-T
   =RJ-sqrt(RJ^2-R0^2).
```

La fonction entre crochets est strictement décroissante, d'où le minimum au
bout de Jung.

Pour un triangle strictement aigu dont `ab` est une arête maximale, l'angle `C`
opposé à `ab` vérifie

```text
60 deg <= C < 90 deg,
R0=|ab|/(2 sin C).
```

On en déduit :

```text
|ab|/2 < R0 <= |ab|/sqrt(3),

sin(15 deg)|ab| < rho <= |ab|/sqrt(6).
```

En particulier :

```text
closed_ball(c0, |ab|/4) subset K_J(T).
```

Le mot **fermée** est volontaire. Il fournit la correction de frontière de la
section suivante.

## 4. Correction : `<=` au quart est sûr

Avec

```text
v(z)=2G(z-a)-W,
```

on a

```text
z-c0=v(z)/(2G).
```

La boule de rayon `|ab|/4` s'écrit :

```text
4 ||v(z)||^2 <= D G^2.
```

Le document audité propose l'inégalité stricte et cite l'inégalité large parmi
les mutants. Ce mutant est **neutre** sous les préconditions annoncées :

```text
rho > sin(15 deg)|ab| > |ab|/4.
```

Même un point situé exactement à distance `|ab|/4` de `c0` reste donc
strictement intérieur à toute sphère admissible. L'inégalité `<=` peut être
adoptée sans faux crédit et gagne les points de frontière quantifiés.

Une fixture aléatoire ne pourra légitimement tuer ce changement. Le déclarer
létal fabriquerait une porte rouge contre une optimisation correcte, autre
tradition logicielle que le dépôt a déjà suffisamment documentée.

### Mutant de rayon réellement faux

Pour obtenir un mutant atteignable, employer par exemple le rayon

```text
17|ab|/64.
```

Il est plus grand que `sin(15 deg)|ab|`. Une famille de triangles isocèles
presque rectangles le réfute. Exemple relatif :

```text
a=(0,0,0),
b=(64,0,0),
x=(32,33,0).
```

Le seed est strictement aigu et `ab` est maximale, mais son `rho/|ab|` est
proche de `sin(15 deg)` et strictement inférieur à `17/64`. Un point choisi sur
la normale entre ces deux rayons est dans le mutant et hors d'une sphère au bout
de Jung.

### Raffinement rationnel optionnel

Le rayon

```text
33|ab|/128
```

reste strictement inférieur à `sin(15 deg)|ab|` et augmente le volume du cœur
d'environ dix pour cent par rapport au quart. Son test entier est :

```text
4096 ||v(z)||^2 <= 1089 D G^2.
```

Il mérite une ablation après réception du quart, pas avant. Le quart conserve
l'avantage d'une constante et d'un contrat particulièrement simples.

## 5. Largeurs

Sous u16 :

```text
D,E,|F| < 2^34,
G < 2^68,
|W_i| < 2^86,
|2G(z_i-a_i)-W_i| < 2^87.
```

Ainsi :

```text
4||v||^2 < 2^177,
D G^2 < 2^170.
```

Les deux membres tiennent largement dans `BigInt<4>` signé/non signé selon la
primitive employée. Ils ne tiennent pas dans `i128`. La porte doit construire
les carrés après promotion, jamais former un produit signé étroit puis le
convertir.

La primitive peut calculer le maximum sur une AABB sans huit coins : chaque
composante de `v(z)` dépend seulement de `z_i`, et son carré atteint son maximum
à une extrémité. Six évaluations de composante et trois `max` suffisent.

## 6. Contrat de ledger indispensable

### 6.1 Les deux certificateurs se recouvrent

`SeedCoreQuarter` est inclus dans `SeedJungPermanent16`. Les crédits ne
s'additionnent donc jamais naïvement :

```text
p != p_quarter + p_jung_full
```

si la seconde passe revisite les spans déjà crédités.

Ordonnance correcte :

```text
pass 1 : SeedCoreQuarter
  -> antichaîne disjointe de NodeSpan/PointId crédités ;

pass 2 : SeedJungPermanent16
  -> même witness tree, avec les spans de pass 1 masqués ;

p_core = cardinalité de l'union, saturée à r4.
```

Ou bien exécuter directement le certificateur fort et garder le quart comme
ablation de coût. Une combinaison où le quart crédite puis le fort redescend
sans masque reproduit exactement le P0 de double crédit du cœur q2.

### 6.2 Raccord au noyau axial

`Q4SeedAxisTopR4` compte déjà les permanents `B=0,A<0` et les racines situées
hors de `J_f`. Lui transmettre seulement un entier initial `p_core`, tout en lui
redonnant les mêmes sites, les recompterait.

L'interface doit porter au moins :

```text
initial_permanent_count,
initial_permanent_spans ou digest/ledger d'IDs,
active witness ranges non créditées.
```

Puis :

```text
p_total = |IDs_core union IDs_axis|,
k = r4-p_total.
```

Les ensembles sont disjoints par construction du parcours, pas par espoir. Le
replay du census doit retrouver les mêmes IDs et exiger :

```text
planned = filled = consumed,
pending = 0,
aucun ID seed,
aucun ID double.
```

Les points strictement permanents ne peuvent être ni roots ni shell ; le masquage
ne retire donc aucun apex shallow. Les positions dupliquées comptent avec leur
multiplicité de vrais `PointId`, tandis que les trois IDs du seed restent
exclus.

## 7. Fates

Le raccord doit conserver des fates exclusifs :

```text
DEAD_CORE_QUARTER        p>=r4 dans le fast path ;
DEAD_JUNG_PERMANENT      p>=r4 après le certificateur fort ;
OPEN_AXIS                p<r4, k=r4-p ;
PENDING_RESOURCE         capacité/continuation incomplète ;
UNSUPPORTED_DEGENERACY   précondition de seed ou d'identité non reçue ;
NUMERIC_FAILURE          largeur ou invariant arithmétique violé.
```

`PENDING_RESOURCE` ne devient jamais `DEAD`. Une allocation refusée après sept
crédits n'est pas une preuve de huitième intérieur, même si l'ordinateur paraît
très convaincu du contraire.

## 8. Gates recommandées

### Autorité ponctuelle

Pour un seed borné et chaque témoin :

1. calculer `P_z(-tau_max)` et `P_z(+tau_max)` exactement ;
2. vérifier que `SeedCoreQuarter` implique les deux signes strictement négatifs ;
3. confronter `SeedJungPermanent16` aux deux signes ;
4. vérifier la parité sous échange `a/b`, permutations d'axes et translations.

### Autorité de bloc

Sur petites AABB entières :

- énumérer tous les vrais points de la boîte ;
- `ALL` implique que chacun est permanent ;
- un échec rend seulement `MIXED` ;
- comparer le compte direct, l'antichaîne de spans et le replay d'IDs.

### Fixtures

1. seed proposé dans `a609aa` :

```text
a=(1000,1000,1000),
b=(1100,1000,1000),
x=(1050,1060,1000),
```

avec huit IDs près du centre exact

```text
c0=(1050, 1000+55/6, 1000).
```

Exiger `DEAD_CORE_QUARTER`, zéro root comparée et un ledger non vide.

2. un seed avec `p=7`, puis un seul root entrant : vérifier `k=1` et non `k=8` ;

3. le même nuage avec une position intérieure dupliquée : les deux `PointId`
comptent, mais aucune ancre `D=0` n'est créée ;

4. un point sur la frontière exacte du quart : il est légitimement crédité par
`<=` ;

5. le triangle presque rectangle ci-dessus : le mutant `17/64` doit être
réfuté.

### Mutants causaux

```text
core-rayon-17-sur-64,
core-oublie-un-axe,
core-carre-avant-promotion,
core-compte-seed,
core-parent-et-enfant,
core-deux-passes-sans-masque,
core-p-initial-sans-skip,
core-pending-devient-dead.
```

Ne pas employer `core-inegalite-large` au rayon `1/4` : il est sûr.

## 9. Ordre d'implémentation

1. corriger d'abord les noms `S4/V4` et l'owner faible signalés au
   `9a4b219` ;
2. écrire un microprobe autonome `SeedCoreQuarter`, avec autorité directe ;
3. brancher le ledger disjoint au noyau axial sur petit `n` ;
4. ajouter `SeedJungPermanent16` et mesurer son gain marginal après le quart ;
5. seulement ensuite l'intégrer au gateway factorisé et aux campagnes de
   pente.

Le théorème est assez fort pour mériter cette discipline. Le jeter directement
dans le grand probe garantirait surtout qu'un gain réel devienne impossible à
attribuer, ce qui est une manière très humaine de remercier une bonne idée.

## 10. Statut

Le cœur de Jung est **reçu mathématiquement**, pas logiciellement. La boule
quart, le domaine commun exact et la réduction `k=r4-p` sont corrects. La
réception logicielle attend encore l'arithmétique large, l'antichaîne d'IDs,
les fates, les mutants causaux et la parité avec le replay exact.