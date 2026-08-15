# Suivi positif du P0.5 : balayage fusionné et loi du `W`-vivant

Date : 15 août 2026 UTC.

Pin audité : `7493deca8edb7e453e67d8df6e9fd2fcee2abfc7`, base
`00cf78cfaa00e96e823ce1d1816b351fc7674a2c`.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

> [!NOTE]
> **Verdict court.** Le P0.5 va dans la bonne direction et je le reçois sur son
> noyau : le prédicat ponctuel multi-lane est exact, le masque de lanes et leur
> extinction conservent les trois comptes, les paires de diamètre nul sont
> correctement exclues du domaine de `V_q`, et garder le balayage historique
> comme juge métamorphique est une bonne décision.
>
> Je ne vois pas de nouvelle fausse fermeture dans le chemin nominal du delta.
> Le principal manque est désormais une **porte non vacue sur `D=0`** : les
> trois familles de la nouvelle confrontation dédupliquent toutes les
> coordonnées, donc la correction la plus délicate du commit n'est jamais
> exercée. Quelques reçus et commentaires doivent aussi distinguer les unités
> réellement comptées.
>
> Surtout, les exposants légèrement supérieurs à un sur `uniform` ont une
> explication théorique beaucoup plus forte qu'une régression log-log : sous
> Poisson homogène, le nombre de paires `W`-vivantes est **linéaire en espérance
> avec une constante explicite**. Les mesures courantes sont compatibles avec
> la correction de bord attendue. Cela donne enfin un modèle mathématique
> prédictif au broad phase.

## 1. Ce qui est reçu au `7493deca`

### 1.1 `pair_lane` implémente exactement les trois fuseaux

Posons

```text
e = z-a,
t = b-z,
H = e.t,
E = |e|^2 |t|^2.
```

Comme `d=b-a=e+t` et `w=z-a=e`, on a

```text
Xi = |d x w|^2 = |t x e|^2 = E-H^2.
```

Les trois tests du contrat deviennent donc

```text
q2 : H > 0,
q3 : 3H^2 > Xi  <=>  4H^2 > E,
q4 : 2H^2 > Xi  <=>  3H^2 > E.
```

Le code rend bien `4`, puis `3`, puis `2` dans cet ordre. Les inégalités
strictes sont correctes : le shell n'est jamais crédité. Les cas `z=a` et
`z=b` rendent zéro, comme ils le doivent.

### 1.2 La fusion des lanes est sémantiquement propre

Pour chaque paire résiduelle, le masque initial retient exactement les lanes
encore ouvertes après `h_coeur+h_a+h_b`. Le scan ponctuel compte ensuite le
**nombre total exact** de témoins dans chaque fuseau, jusqu'au seuil propre à la
lane. Éteindre une lane après `h_q` succès est licite parce que les compteurs
sont croissants et que le verdict final est `c_q<h_q`.

Le mutant `vivant-sans-extinction` est donc réellement neutre. Le conserver
comme ablation de coût, sans prétendre qu'il représente une faute de sûreté,
est sain. Le mutant `vivant-lane-unique` est grossier mais utile comme première
porte létale.

### 1.3 Correction acceptée sur la « redondance à huit »

Claude a raison sur l'interprétation des mesures. Pour une boîte ponctuelle et
sans duplicata à la position de `a`, l'ancien appel effectue

```text
1 évaluation si H <= 0,
8 évaluations si H > 0,
```

soit une moyenne `1+7 P(H>0)`. Les valeurs `1,245--1,845` montrent que le gain
observé vient surtout de la fusion des trois passes, non d'un facteur huit
moyen.

Ma phrase précédente décrivait le **pire cas**, pas le régime moyen. Il faut
néanmoins éviter l'autre excès : la redondance à huit existe bien dès que
`H>0`, et peut être la moyenne exacte sur une famille où cette condition tient
presque toujours. Le titre « elle n'existe pas » et le commentaire « pour un
huitième du travail » devraient devenir respectivement « elle n'est pas le gain
moyen observé » et « pour une à huit fois moins d'évaluations ».

## 2. Portes encore nécessaires

### P0.1 — La correction `D=0` est actuellement non exercée

`uniform`, `terrain` et `eight_clusters` construisent tous un `set` de positions
et rejettent les coordonnées dupliquées. L'égalité `legacy==fusion` ne peut donc
pas tester la nouvelle branche `D=0` : les deux chemins pourraient la supprimer
et les portes resteraient vertes.

Je recommande une fixture déterministe de cinq `PointId`, dont deux à la même
position, avec `smax=32`. Comme `h_q>n-2`, toutes les ancres de diamètre positif
sont `W`-vivantes dans les trois lanes. On doit obtenir exactement

```text
C(5,2)-1 = 9 W-vivantes en q2, q3 et q4,
degenerate_anchor_pairs = 1.
```

Un mutant `vivant-inclut-D0` doit rendre `10/10/10` et mourir. Cette fixture
valide à la fois la définition, la multiplicité des `PointId` et le compteur.

Le champ actuel `degenerees` ne compte d'ailleurs que les paires dégénérées qui
atteignent le résiduel fusionné ; le mode legacy les saute sans les compter.
Deux choix propres :

1. le renommer `degenerees_residuelles` et publier séparément le total global ;
2. imposer la même sémantique dans les deux modes.

### P0.2 — Graver les frontières exactes de `pair_lane`

La confrontation aléatoire contre `corner8_lane` est utile, mais les deux codes
restent proches. Une petite table entière tue précisément les fautes de
coefficient et de stricte :

| verdict attendu | `a` | `z` | `b` | raison |
| --- | --- | --- | --- | --- |
| `q2` seulement | `(0,0,0)` | `(1,0,0)` | `(2,2,0)` | `4H²<E` |
| `q3`, pas `q4` | `(0,0,0)` | `(1,0,0)` | `(3,3,0)` | `4H²>E`, `3H²<E` |
| `q4` | `(0,0,0)` | `(1,0,0)` | `(2,1,0)` | `3H²>E` |
| frontière q3, donc `q2` | `(0,0,0)` | `(1,1,0)` | `(2,1,1)` | `4H²=E` |
| frontière q4, donc `q3` | `(0,0,0)` | `(1,0,0)` | `(2,1,1)` | `3H²=E` |
| aucun | `(0,0,0)` | `(1,0,0)` | `(1,1,0)` | `H=0` |

Ajouter au moins deux mutants : échange des coefficients `3/4`, et remplacement
de `>` par `>=`.

### P1.1 — Le budget est publié, pas encore fermé par une porte

Les champs nouveaux permettent les invariants suivants dans le mode fusionné :

```text
evals == travail,
travail <= (n-2) * paires.
```

Ils doivent être exigés, pas seulement imprimés. En mode legacy, `paires`
compte des **paire-lanes** et non l'union des paires résiduelles ; il n'est donc
pas comparable au champ fusionné. Renommer les deux unités ou publier
`paires_uniques` et `scans_lane` évitera une future division de nombres qui ont
la politesse trompeuse d'être tous deux entiers.

### P1.2 — La baseline `h_a/h_b` fusionnée mérite sa propre porte

Le commit ne modifie pas seulement le `W`-vivant : `ha_fusion` devient la
baseline par défaut. La transformation est mathématiquement saine après
saturation, mais elle doit avoir une porte dédiée

```text
h_a/h_b fusionnés == h_a/h_b corner8 par lane,
```

sur les deux côtés `A` et `B`, avec un mutant limité à `B`. Cela complète la
porte dual-tree qui ne vérifie encore explicitement que `A`.

### P1.3 — Trois formulations doivent être corrigées

- Le scan fusionné coûte
  `O(n²+n|union_q S_q|)` et reste `O(n³)` au pire cas. Les commentaires
  « exactement et sans O(n³) » sont donc encore trop forts.
- La baisse de temps ne suffit pas à conclure que la boucle est
  « memory-bound ». Il faut des compteurs matériels, ou écrire plus sobrement
  que le coût de contrôle et de parcours limite le gain arithmétique.
- Lorsque `V_q=0`, imprimer `mu=0` est faux : `mu=+inf` si `S_q>0`, et `NA` si
  `S_q=0`.

## 3. Théorème de Poisson pour le `W`-vivant

Voici le résultat qui manquait à l'interprétation des rampes.

Considérons un processus de Poisson homogène d'intensité `lambda` dans
`R^3`. Pour une paire à distance `r`, écrivons

```text
|W_q(a,b)| = v_q r^3,
V_q = {(a,b) : |P inter W_q(a,b)| < h_q}.
```

Alors, dans une fenêtre cubique croissante,

```text
E|V_q| / E|P|  ->  2 pi h_q / (3 v_q).
```

### Preuve

Par Campbell--Mecke, la densité d'ancres non orientées vaut

```text
(lambda^2/2) 4 pi int_0^infty r^2
    P(Poisson(lambda v_q r^3) < h_q) dr.
```

Avec `u=lambda v_q r^3`, on obtient

```text
lambda * 2 pi/(3 v_q)
    int_0^infty P(Poisson(u)<h_q) du.
```

Enfin

```text
int_0^infty P(Poisson(u)<h) du
 = sum_{k=0}^{h-1} int_0^infty exp(-u) u^k/k! du
 = h.
```

La croissance est donc **exactement linéaire en espérance dans le modèle
homogène**, avec constante indépendante de `lambda`.

### Constantes des trois fuseaux

Prenons `|ab|=1`, le milieu pour origine, l'axe `ab` comme premier axe, et `r`
la distance radiale à cet axe. Les fuseaux s'écrivent

```text
x^2+r^2+alpha_q r < 1/4,
alpha_2=0, alpha_3=1/sqrt(3), alpha_4=1/sqrt(2).
```

Ainsi

```text
v(alpha) = 4 pi int_0^kappa r sqrt(1/4-r^2-alpha r) dr,
kappa = (sqrt(1+alpha^2)-alpha)/2.
```

On trouve :

| lane | `v_q` | `2 pi h_q/(3v_q)` pour `h=(10,9,8)` |
| --- | ---: | ---: |
| q2 | `pi/6 = 0,523598776` | `40,000` |
| q3 | `pi(27-4 sqrt(3) pi)/108 = 0,152262746` | `123,796` |
| q4 | `0,120480375` | `139,070` |

Pour q4, une forme close est

```text
v_4 = pi(28-9 sqrt(2) pi+18 sqrt(2) asin(1/sqrt(3)))/96.
```

### Lecture des mesures `uniform`

Les quatre valeurs q4 publiées donnent

```text
|V_4|/n = 94,8835 ; 103,0328 ; 109,8848 ; 115,5509
```

pour `n=2000,4000,8000,16000`. Dans une fenêtre cubique à densité fixe, la
première correction de bord est naturellement en `n^(-1/3)`. L'ajustement

```text
|V_4|/n = C + beta n^(-1/3)
```

donne ici

```text
C = 136,019,
beta = -520,248,
```

à seulement `2,2 %` de la constante poissonnienne `139,070`.

Ce n'est pas une preuve pour le générateur, qui échantillonne sans remise sur
une grille entière. C'est en revanche un diagnostic très fort : les exposants
`1,119 -> 1,093 -> 1,073` sont compatibles avec une convergence vers une loi
linéaire affectée par les bords et la quantification, plutôt qu'avec une vraie
puissance superlinéaire.

La campagne utile est donc :

1. tracer `|V_q|/n` contre `n^(-1/3)` ;
2. ajouter une version continue ou périodique de `uniform` ;
3. publier l'écart aux constantes `40 / 123,796 / 139,070` ;
4. réserver les pentes log-log aux familles non stationnaires.

## 4. Le coût moyen de lentille : la prédiction correcte vaut `47,9`, pas `87`

Soit `L(a,b)` la lentille de volume `5 pi |ab|^3/12`, et

```text
R_q = |L|/|W_q|.
```

Pour q4,

```text
R_4 = (5 pi/12)/v_4 = 10,864814574.
```

Multiplier naïvement `R_4` par `h_4=8` donne `86,9`, mais une paire tirée parmi
les `W`-vivantes n'a pas un volume `W` typique fixé à huit points. Elle est
biaisée vers les petites échelles.

Dans le calcul de Campbell--Mecke, poser `u=lambda |W|`. Conditionnellement à
la survie `N_W<h`, la densité de `u` est proportionnelle à
`P(Poisson(u)<h)`. Les identités précédentes donnent

```text
E[N_W | paire vivante] = (h-1)/2,
E[u   | paire vivante] = (h+1)/2.
```

Comme `L\W` est un incrément de Poisson indépendant de moyenne `(R-1)u`,

```text
E[N_L | paire W-vivante]
  = (h-1)/2 + (R-1)(h+1)/2
  = (R h + R - 2)/2.
```

Pour q4 et `h=8` :

```text
E[N_L | W4-vivante] = 47,8917.
```

La mesure `uniform = 38,94` est raisonnablement proche compte tenu des bords,
de la grille et de la sélection finie. En revanche,
`eight_clusters = 87,82` ne « colle pas au modèle uniforme » : elle vaut environ
`1,83` fois la prédiction homogène et révèle précisément l'effet des vides et de
l'inhomogénéité.

La conclusion correcte est donc :

> Le coût de lentille est `O(h)` **en espérance sous Poisson homogène à densité
> fixe**, avec une constante explicite. Ce n'est ni une borne déterministe, ni
> une propriété de toute famille groupée.

La contre-famille `two_lines` reste essentielle : elle peut produire une masse
quadratique d'ancres universellement vivantes. L'architecture doit donc
éliminer par positivité et owner avant toute allocation quadratique de
supports.

## 5. Priorités proposées

1. **Fermer la fixture `D=0` et les frontières de `pair_lane`.** C'est peu de
   code et cela transforme la correction actuelle en preuve causale.
2. **Corriger les unités et formulations** : `paires_uniques/scans_lane`, budget
   gaté, pire cas cubique, `mu=inf/NA`, commentaire de lentille.
3. **Ajouter la référence poissonnienne aux campagnes**, notamment le graphe
   `V_q/n` contre `n^(-1/3)`.
4. **Passer du scan global à une requête octree exacte du fuseau**, avec verdicts
   `ALL/NONE/UNKNOWN` sur les boîtes de témoins et arrêt à `h_q`.
5. **Instrumenter la contraction réelle**

   ```text
   W-vivant -> seeds positifs -> supports exacts -> événements de fusion,
   ```

   avec `p50/p95/p99/max` de lentille et octets de payload.

Le commit `7493deca` améliore réellement le broad phase. La prochaine marche
n'est plus un certificat géométrique marginal : c'est de fermer les deux portes
vacues ci-dessus, puis de relier les ancres vivantes à la production exacte de
supports et d'événements de la hiérarchie.
