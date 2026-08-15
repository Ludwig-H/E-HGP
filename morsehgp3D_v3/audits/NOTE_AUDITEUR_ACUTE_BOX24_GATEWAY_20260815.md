# Note de l'auditeur — `AcuteBox24`, gateway aigu exact sur AABB

Date : 15 août 2026 UTC.

Pins de contexte :

- code audité : `5ce2634cc6e1e5fa9dedc3b9736ce799802d40a5` ;
- proposition consolidée lue au `0d50d653a367d9ce636e816365c7824f170225ad`.

Cadre : `phase=exploration_v3_hors_registre`, `backend=math_reference`,
`profile=quantized_u16_input_only`, `mode=acute_carrier_block_gateway`,
`public_status=not_claimed`.

> [!IMPORTANT]
> `two_lines` montre qu'aucun resserrement universel du `W`-vivant ne peut
> suffire : q3/q4 gardent `Theta(n^2)` ancres alors qu'il n'existe aucune face
> aiguë. La positivité doit donc intervenir **avant** l'expansion par `PairId`.
>
> Il existe pour cela un classifieur de blocs plus simple que `CORNER512`.
> Pour trois AABB `A,B,C`, le maximum du produit scalaire d'acuité
>
> ```text
> Phi(a,b,x) = (a-x) dot (b-x)
> ```
>
> est exact avec seulement `3*8=24` évaluations scalaires 1D. Son minimum exact
> demande `3*4=12` évaluations supplémentaires, toutes entières après
> multiplication par quatre. En le combinant à deux bornes corrélées de
> maximalité de l'arête `ab`, on obtient un gateway `NONE/ALL/MIXED` sûr qui
> annihile la contre-famille à deux droites avant toute face ou sweep.

## 1. Condition exacte du carrier

Pour

```text
D = ||a-b||^2,
E = ||a-x||^2,
X = ||b-x||^2,
```

la face `abx` est strictement aiguë et possède `ab` comme arête maximale si et
seulement si

```text
Phi = (a-x) dot (b-x) > 0,
D-E >= 0,
D-X >= 0,
```

puis le tie de longueur est tranché par l'`EdgeKey` canonique.

En effet,

```text
2 Phi = E+X-D.
```

Si `D>=E,X`, l'angle opposé à `ab` est le plus grand ; il suffit donc que cet
angle soit strictement aigu, c'est-à-dire `E+X>D`.

Pour un support q4 positif et son owner maximal `ab`, le lemme déjà reçu assure
qu'au moins une des faces incidentes `abx/aby` satisfait cette condition. Le
gateway ne dépend donc pas d'une sortie Lane3 : il reconstruit ses propres
carriers géométriques.

## 2. Extrema exacts de `Phi` sur trois AABB

Écrivons, axe par axe,

```text
phi_i(a,b,x) = (a-x)(b-x).
```

Les boîtes étant cartésiennes,

```text
max Phi = sum_i max phi_i,
min Phi = sum_i min phi_i.
```

Les choix optimaux des trois coordonnées se combinent librement en un vrai
triplet de coins/points des AABB.

### 2.1 Maximum : huit coins par axe

À `x` fixé, `phi_i` est bilinéaire en `(a,b)` : son maximum sur
`A_i x B_i` est atteint aux quatre couples d'extrémités. Pour `a,b` fixés,

```text
phi_i(x) = x^2-(a+b)x+ab
```

est convexe ; son maximum sur `C_i` est atteint à une extrémité.

Ainsi

```text
max phi_i
 = max_{a in {Alo,Ahi}, b in {Blo,Bhi}, x in {Clo,Chi}}
     (a-x)(b-x).
```

Il faut huit produits par axe, donc vingt-quatre en dimension trois. Le verdict

```text
Phi_max <= 0  =>  NONE_ACUTE
```

est **exact sur l'enveloppe continue** des trois boîtes, donc a fortiori sûr
sur leurs points stockés. L'égalité est rejetée : l'acuité est stricte.

### 2.2 Minimum : quatre projections par axe

À `a,b` fixés, la même parabole atteint son minimum en
`x=(a+b)/2`, projeté sur l'intervalle `C_i`. Le minimum global en `(a,b)` est
atteint pour des extrémités de `A_i` et `B_i`, puisque pour tout `x` fixé la
fonction est bilinéaire.

Tout se calcule sans fraction. Pour chaque couple d'extrémités `(a,b)`, poser

```text
x2 = clip(a+b, 2*Clo, 2*Chi).
```

Alors

```text
4 phi_i = (2a-x2)(2b-x2).
```

D'où

```text
4 Phi_min
 = sum_i min_{a endpoint, b endpoint}
     (2a-x2)(2b-x2).
```

Douze produits suffisent en dimension trois. Le verdict

```text
Phi_min > 0  =>  ALL_ACUTE
```

est lui aussi exact sur les AABB continues.

### 2.3 Largeurs

Sous u16, chaque différence simple tient sur 17 bits signés ; chaque produit
mis à l'échelle par quatre reste sous 34 bits, et la somme tridimensionnelle
sous 36 bits. Un `i64` signé suffit largement. Aucun `i128` n'est nécessaire à
ce classifieur.

## 3. Maximalité de l'arête avec dépendances conservées

Deux différences doivent être non négatives :

```text
Delta_E = D-E = (b-x) dot (b+x-2a),
Delta_X = D-X = (x-a) dot (2b-a-x).
```

Ces factorisations sont préférables à `D_interval-E_interval` : elles conservent
la compensation des grandes coordonnées communes, notamment la hauteur `H` de
`two_lines`.

Pour chaque axe, construire les intervalles

```text
U_E = B-C,
V_E = B+C-2A,
U_X = C-A,
V_X = 2B-A-C,
```

puis l'enclosure du produit par les quatre produits d'extrémités. La somme des
trois intervalles donne des bornes sûres

```text
[Delta_E_lo, Delta_E_hi],
[Delta_X_lo, Delta_X_hi].
```

Elles ne sont pas revendiquées exactes à cause des dépendances résiduelles, mais
elles sont fail-open et peu coûteuses.

Le classifieur de bloc devient :

```text
NONE
  si Phi_hi <= 0
  ou Delta_E_hi < 0
  ou Delta_X_hi < 0;

ALL_STRICT_OWNER
  si Phi_lo > 0
  et Delta_E_lo > 0
  et Delta_X_lo > 0;

MIXED
  sinon.
```

Pour la première version, exiger les deux `Delta_lo>0` dans `ALL` évite toute
question de tie. Une égalité possible descend jusqu'aux feuilles, où l'owner
exact emploie longueur puis `EdgeKey`.

Des bornes de distances AABB ordinaires peuvent être placées en prétest :

```text
Dmax < Emin  => NONE_OWNER,
Dmax < Xmin  => NONE_OWNER.
```

Elles sont moins corrélées, mais presque gratuites.

## 4. Pourquoi `two_lines` meurt exactement

Pour une paire croisée

```text
a=A_i=(i,0,0),
b=B_j=(0,j,H),
```

un carrier potentiel appartient nécessairement à l'une des deux droites.

### Carrier `x=A_k`

Si `k<=i`,

```text
Phi = (A_i-A_k) dot (B_j-A_k)
    = -k(i-k) <= 0.
```

La face n'est pas strictement aiguë.

Si `k>i`,

```text
Delta_X = ||A_i-B_j||^2-||A_k-B_j||^2
        = i^2-k^2 < 0.
```

L'arête `ab` n'est pas maximale.

### Carrier `x=B_l`

Symétriquement, si `l<=j`,

```text
Phi = -l(j-l) <= 0,
```

et si `l>j`,

```text
Delta_E = j^2-l^2 < 0.
```

Il n'existe donc aucun carrier aigu pour aucune paire croisée, sans développer
les `m^2` ancres. Sur des blocs d'indices ordonnés, `Phi_hi<=0` ou une borne de
`Delta` devient uniforme ; seuls les blocs coupant la frontière `k=i` ou
`l=j` doivent être scindés. Les cas d'égalité correspondent au même endpoint et
sont exclus par `PointId`/position avant émission.

La porte contractuelle devient :

```text
two_lines :
  q4 W-vivant = Theta(n^2),
  AcuteCarrierBlock emitted = 0,
  faces = 0,
  sweep events = 0,
  aucune allocation indexée par PairId.
```

Le nombre de tâches physiques doit être mesuré sur au moins quatre tailles ; la
preuve ci-dessus garantit la sortie nulle, pas à elle seule une pente de
scheduling.

## 5. Ordonnance candidate

Pour chaque `PairBlock4 = A x B` encore ouvert :

1. interroger l'octree des troisièmes sommets dans la fenêtre de lentille ;
2. appliquer les prétests de distances ;
3. appliquer `AcuteBox24` et les deux bornes `Delta` ;
4. `NONE` : supprimer le triple-bloc ;
5. `ALL_STRICT_OWNER` : émettre un `AcuteCarrierBlock`, sans `PairId` ;
6. `MIXED` : scinder le facteur qui porte la plus grande incertitude normalisée
   parmi `A/B/C` ;
7. à la feuille : exclure `x=a,b`, appliquer owner exact, rang et choix du
   carrier primaire ;
8. seulement alors lancer `Q4SeedAxisTopR4` et la sweep 1D.

Il est important que la fenêtre carrier soit un index spatial, non une boucle
sur tous les sites pour chaque paire. Sinon le gateway arrive après le coût
qu'il devait supprimer, cette tradition logicielle consistant à installer la
sortie de secours derrière l'incendie.

Le même classifieur peut alimenter Lane3 : `ALL_ACUTE` est un bloc de scheduling,
mais chaque triangle doit encore produire sa propre `BallKey` et son census. Il
ne faut pas réutiliser les sorties Lane3 dans Lane4.

## 6. Portes et mutants

### Autorité exhaustive à petit domaine

Pour des AABB entières de span borné :

- énumérer tous les triplets `(a,b,x)` ;
- vérifier que `NONE` ne contient aucun carrier exact ;
- vérifier que `ALL_STRICT_OWNER` ne contient que des carriers exacts ;
- comparer `Phi_lo/Phi_hi` aux extrema continus calculés par rationnels ;
- exercer permutations d'axes, échange `A/B`, translations et homothéties
  entières positives.

### Fixtures de frontière

- `Phi=0` : angle droit, doit rester `NONE_ACUTE` ;
- `Delta_E=0` ou `Delta_X=0` : tie de longueur, doit rester `MIXED` tant que
  l'`EdgeKey` n'est pas singleton ;
- carrier aigu géométrique dont la boule q3 est profonde : il doit rester dans
  la source q4 ;
- coordonnées larges u16 presque orthogonales ;
- AABB avec optimum de `Phi_min` au milieu, afin de tuer un calcul limité aux
  coins.

### Mutants recommandés

```text
acute-max-oublie-un-coin,
acute-min-coins-seuls,
acute-angle-large,
acute-oublie-DeltaE,
acute-oublie-DeltaX,
acute-tie-accepte-en-bloc.
```

Le mutant `acute-min-coins-seuls` est important : une fonction convexe peut
avoir son minimum strictement à l'intérieur de `C`, même si son maximum est aux
coins.

## 7. Statut

Cette note fournit un classifieur mathématique et une ordonnance candidate. Elle
ne reçoit encore ni prototype, ni pente, ni mémoire, ni SLO. Son intérêt est
plus étroit et plus décisif : elle place une porte de positivité **avant** la
masse quadratique révélée par `two_lines`, avec une arithmétique u16 très bon
marché et une autorité exhaustive simple à écrire.
