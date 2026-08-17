# Audit q4 ciblé : lemme du préfixe et représentation du niveau

Date : 17 août 2026.  
Pin de code audité : `8d52000147d846f282c4e676e30cf0c6e444b7bb`.  
Contre-audit pris en compte : `1cecc23695665d4a538b7cfc22b530358c35941c`.

## Verdict

La baseline q4 est mathématiquement bien orientée. Je reçois statiquement :

- `AcuteSeed` formé en amont des census q3/q4 ;
- le Cramer relatif de `Q4Form` et la puissance affine `q4_power` ;
- le test d'arité 4 par position stricte du centre dans le tétraèdre ;
- l'owner sur les six arêtes ;
- le cover de coefficient 4 ;
- l'exact-once par carrier aigu canonique.

Je ne vois pas d'autre verrou utile dans ce commit. Les deux questions posées par Claude ont des réponses nettes : le lemme du préfixe ternaire est vrai et possède une preuve courte ; le niveau q4 non réduit est suffisant pour la forêt, à condition de distinguer strictement égalité de représentation et égalité mathématique.

## 1. Preuve v4 du lemme du préfixe ternaire

**Lemme.** Soit `T={a,b,x,y}` un tétraèdre non dégénéré dont la circum-sphère a son centre `c` strictement à l'intérieur de `T`. Si `ab` est une arête de longueur maximale de `T`, alors au moins une des faces `abx` ou `aby` est strictement aiguë.

**Preuve.** Translatisons le centre en l'origine et posons

```text
u=a-c,  v=b-c,  p=x-c,  q=y-c,
```

avec

```text
|u|=|v|=|p|=|q|=R.
```

Comme `c` est strictement intérieur, il existe des poids strictement positifs

```text
alpha,beta,gamma,delta > 0,
alpha+beta+gamma+delta=1,
alpha u + beta v + gamma p + delta q = 0.
```

Supposons les deux faces `abx` et `aby` non aiguës. Puisque `ab` est maximale dans chacune, l'angle non aigu est celui opposé à `ab`. On a donc

```text
(u-p)·(v-p) <= 0,
(u-q)·(v-q) <= 0.
```

Posons

```text
s = u+v,
tau = R^2 + u·v = |s|^2/2 >= 0.
```

Les deux inégalités donnent

```text
p·s >= tau,
q·s >= tau,
```

et, trivialement,

```text
u·s = v·s = tau.
```

En prenant le produit scalaire de la relation barycentrique avec `s`,

```text
0 = alpha u·s + beta v·s + gamma p·s + delta q·s
  >= tau(alpha+beta+gamma+delta)
  = tau.
```

Ainsi `tau=0`, donc `s=0` et `v=-u`. Le centre `c` est alors le milieu de l'arête `ab`, donc appartient au bord du tétraèdre, contradiction avec son appartenance stricte à l'intérieur.

L'une des deux faces a donc son angle opposé à `ab` strictement aigu. Comme `ab` est une arête maximale de cette face, ses deux autres angles sont plus petits ; la face entière est strictement aiguë. CQFD.

### Conséquence d'implémentation

La source q4 par `AcuteSeed` est complète : tout événement q4 positif possédé par `ab` possède au moins un carrier aigu incident à `ab`. Si les deux carriers sont aigus, choisir le plus petit `PointId` donne un exact-once déterministe. Aucun héritage du statut d'événement q3 n'est requis.

Cette preuve doit remplacer le statut `theoreme_v3` du lemme dans `MATHEMATIQUES.md`.

## 2. Q12 : le représentant non réduit est suffisant

Pour q4,

```text
R^2 = N/D,
N = |N'|^2 < 2^146,
D = det^2 < 2^114.
```

Il n'est pas nécessaire de calculer `gcd(N,D)` pour construire la forêt. Le représentant non réduit `(U192 N, u128 D)` est exact et stable pour le support calculé. L'ordre et les plateaux se décident par produits croisés :

```text
N1/D1 <=> N2/D2
ssi
N1*D2 <=> N2*D1.
```

Chaque produit q4/q4 est inférieur à `2^260`; `U320` suffit. Le même comparateur peut recevoir q3 et q2 après promotion de leurs numérateurs et dénominateurs.

### Contrat indispensable

`Q4Level::operator==`, qui compare actuellement les champs, n'est qu'une **égalité de représentation**. Deux boules distinctes peuvent avoir le même rayon avec des couples non réduits différents. Pour les macro-lots de la forêt, l'unique égalité sémantique autorisée est

```text
same_level(x,y) := compare_level_320(x,y) == 0.
```

Je recommande de rendre cette distinction visible dans l'API :

```cpp
bool same_level_representation(...);  // champs identiques
int  compare_exact_level(...);        // produit croisé U320
bool same_exact_level(...) { return compare_exact_level(...) == 0; }
```

Le macro-lot gèle les racines avant le plateau, puis applique ensemble toutes les multifusions de niveau sémantiquement égal. Il ne doit jamais grouper par `operator==` ni par hash du couple brut.

### Canonisation publique

L'option (b) est donc reçue pour le calcul interne et la forêt. Une fraction pgcd-réduite n'est nécessaire que si le format d'export exige explicitement une représentation rationnelle canonique indépendante du producteur. Dans ce cas, la réduction 192/128 peut être différée à la sérialisation des seuls événements survivants ; elle ne doit pas revenir sur le chemin des candidats.

## 3. Fixture d'indépendance q4

Le contre-audit `1cecc23` est correct : la fixture à quatorze points ferme l'indépendance vis-à-vis des **ancres q3 vivantes**, mais deux autres faces restent des événements q3. Cela ne met pas en cause le code actuel, qui produit bien ses seeds avant tout census q3. En revanche, la porte ne justifie pas encore la phrase « invisible depuis le flux q3 ».

La version renforcée à vingt-deux points proposée dans ce contre-audit doit remplacer ou compléter la porte actuelle, avec deux mutants distincts :

```text
q4-seeds-from-q3-live,
q4-seeds-from-q3-events.
```

## Ordre utile de la suite

1. verser la preuve ci-dessus dans le dossier mathématique ;
2. renforcer la fixture conformément à `1cecc23` ;
3. construire l'oracle q4 indépendant contre la baseline énumérée actuelle ;
4. ajouter `U320` et l'égalité sémantique des niveaux ;
5. recevoir ensuite la sélection axiale contre cette baseline.

Il n'y a pas lieu de rouvrir q3 ni d'ajouter une nouvelle optimisation avant ces étapes. La baseline q4 est saine ; le prochain travail utile est de lui donner une vérité indépendante, puis de remplacer l'énumération par l'accélérateur axial sans changer les records.
