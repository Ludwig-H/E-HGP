# Audit ciblé après `a524020` — axial adaptatif maintenant, plan des centres si le coût par seed persiste

Date : 17 août 2026.  
Pin audité : `a524020eb27fcc755c4afed32ed9ed30b563b2ce`.  
Code de référence concerné : `63d364a46de1bbd5f02a98e259492021bf0c538b` (axial borné), `2b7bb3299e1a75f0fe9dd3bc5fdfff96e186fb57` (fold `sort/reduce`).

## Verdict

Le nouveau reçu est très utile et son diagnostic est cohérent :

- sur `uniform,n=1600`, l’axial borné coûte environ `+7 %` en génération ;
- sur `eight_clusters,n=1000`, il réduit les évaluations q4 d’environ `420×` et `t_gen` de `−29 %` ;
- à `eight_clusters,n=8000,K_max=10`, la baseline dépasse 90 minutes alors que l’objet final et l’aval ne sont pas le verrou ;
- le sweep axial à deux côtés du contre-audit `c829872` devient donc une priorité de code justifiée par une mesure, et non plus seulement une belle factorisation.

Je ne trouve aucune incohérence dans les compteurs publiés. En revanche, ces deux régimes opposés montrent qu’un choix global `--axial-on/off` ne peut pas devenir le chemin CPU définitif. La solution utile est un **dispatch exact par seed**, fondé sur la géométrie réellement rencontrée, puis le sweep à deux côtés sur les seeds denses.

Une seconde limite apparaît déjà : même le sweep à deux côtés calcule encore `(A_z,B_z)` pour tous les sites de tous les seeds. Avec `4,4 M` seeds à `n=1000`, cela peut rester le poste dominant. Je donne donc aussi la factorisation mathématique suivante, à n’ouvrir que si les mesures après sweep le confirment : une construction locale dans le plan des centres, par ancre, qui évite de rescanner le cover pour chaque seed.

---

## 1. Ne pas choisir l’algorithme par famille

`eight_clusters` n’augmente pas artificiellement sa densité interne : son domaine est dilaté avec `n` pour garder la même densité locale que `uniform`. Son caractère difficile vient de la géométrie des amas et des covers, pas d’un changement de régime de densité.

Le moteur ne doit donc jamais contenir :

```text
if family == eight_clusters: axial
```

La famille est un générateur de test, pas une propriété de l’objet. Le signal pertinent est local : taille du cover et nombre de complétions qui passent la lentille pour un seed donné.

---

## 2. Dispatch adaptatif exact par seed

Fixons une ancre `(a,b)` de longueur carrée `D2`, son `cover`, et un seed aigu `x`.

Le chemin baseline commence par les trois tests bon marché :

```text
|a-y|² <= D2,
|b-y|² <= D2,
|x-y|² <= D2.
```

Sur `uniform`, ils rejettent presque toutes les complétions en quelques opérations. Sur `eight_clusters`, beaucoup de points les passent et la suite coûte cher. C’est exactement l’information nécessaire au dispatch.

### 2.1 Préfiltre partagé par ancre

Construire une seule fois :

```cpp
anchor_lens = {
    y in cover : |a-y|² <= D2 && |b-y|² <= D2
};
```

Cette liste est indépendante du seed. Elle remplace deux calculs répétés dans chaque appel de `valid_completion`.

Pour chaque seed `x`, balayer `anchor_lens` et construire :

```cpp
seed_lens = { y in anchor_lens : |x-y|² <= D2 };
```

Le coût de ce balayage est la première partie exacte du chemin baseline ; il n’est pas du travail jeté.

### 2.2 Règle de choix

Notons :

```text
m = |cover|,
l = |seed_lens|.
```

Le coût résiduel baseline est croissant avec `l`, tandis que l’axial paie essentiellement le calcul `(A,B)` sur les `m` sites, puis au plus `2h_4` groupes.

Choisir :

```text
baseline si l <= alpha*m + beta,
axial deux-côtés sinon.
```

`alpha,beta` sont des paramètres de performance, jamais de correction. Ils doivent être calibrés par mesures appariées, par exemple sur une petite grille de valeurs, puis gravés avec le matériel et le compilateur du backend CPU. Une première implantation peut exposer :

```text
--axial-switch-alpha-num,
--axial-switch-alpha-den,
--axial-switch-beta.
```

Il est également possible d’arrêter le scan de `seed_lens` dès que le seuil est dépassé : la branche axiale n’a pas besoin de la liste complète. Le surcoût avant bascule est alors borné.

### 2.3 Exactitude

Les deux branches produisent le même multiensemble de `BallCandidate` après RLE :

- la baseline a déjà ses portes ;
- l’axial borné est apparié à la baseline ;
- le sweep deux-côtés doit être apparié de la même manière.

Le dispatch ne modifie donc que le chemin de calcul. Toute fonction déterministe de `(m,l,D2,h_4)` est admissible.

### 2.4 Porte permanente

Ajouter :

```text
--q4-path=baseline|axial|adaptive
```

et comparer, sur les portes existantes :

```text
BallKey + arité + représentation post-RLE,
I_B/U_B,
ForestEvent,
ComponentDelta,
RenderResult,
partition finale.
```

Fixtures spécifiques :

1. `l` exactement au seuil ;
2. `l` juste au-dessus ;
3. groupe axial d’égalité frontière ;
4. permutation physique et relabeling des `PointId` ;
5. `uniform,n=1600` : pas de régression importante ;
6. `eight_clusters,n=1000` : la majorité du travail dense bascule vers l’axial.

Compteurs :

```text
q4_seeds_baseline,
q4_seeds_axial,
anchor_lens_sites,
seed_lens_tests,
seed_lens_pass,
t_dispatch,
t_baseline,
t_axial_AB,
t_axial_groups.
```

---

## 3. Le sweep à deux côtés est maintenant la bonne première correction

Pour un seed `(a,b,x)`, avec

```text
A_z = P_3(z),
B_z = n·(z-a),
mu_z = A_z/B_z,
```

la profondeur exacte sur le cover de la sphère de paramètre `mu` est :

```text
d_cover(mu)
  = p
  + #{B_z > 0 et mu_z < mu}
  + #{B_z < 0 et mu_z > mu},
```

où `p = #{B_z=0,A_z<0}`.

Ainsi le sweep de `c829872` doit :

1. fusionner les groupes de même `mu` des deux côtés ;
2. calculer préfixes positifs et suffixes négatifs ;
3. tuer les groupes avec `d_cover >= h_4` avant `valid_completion` ;
4. retirer le scan `depth_dead(q4_power)` du chemin axial après une phase de recoupement.

Le nouveau reçu fournit précisément le régime où cette factorisation doit être mesurée : `eight_clusters,n=1000`, puis `2000`, avant de relancer `n=8000`.

### Réserve importante

Cette correction supprime les scans de profondeur et les complétions tuées par le côté opposé, mais elle conserve encore :

```text
un calcul (A_z,B_z) pour chaque couple (seed,z du cover).
```

Avec `4 416 744` seeds à `n=1000`, ce terme peut rester dominant même après disparition de `depth_dead`. Il faut donc publier séparément :

```text
seeds,
sum_cover_over_seeds,
AB_pairs,
groupes avant/après deux-côtés,
valid_completion_calls.
```

Si `AB_pairs` porte encore la pente, continuer à optimiser le même scan par quelques instructions SIMD ne suffira probablement pas pour `n=8000`.

---

## 4. Factorisation suivante : le plan des centres par ancre

Cette factorisation évite conceptuellement le balayage complet du cover pour chaque seed.

Fixons une ancre `(a,b)`. Posons :

```text
d   = b-a,
u_z = 2z-a-b,
q_z = |u_z|²-|d|²,
T   = 2c-a-b,
```

où `c` est le centre d’une sphère passant par `a,b`. Comme `c` appartient au plan médiateur :

```text
T·d = 0.
```

Le quadruple de la puissance de `z` relativement à cette sphère vaut exactement :

```text
Phi_z(T) = q_z - 2 u_z·T.
```

Donc :

```text
z intérieur  <=> Phi_z(T) < 0,
z coquille   <=> Phi_z(T) = 0.
```

Dans le plan bidimensionnel `d^perp`, chaque site `z` définit une droite orientée :

```text
ell_z : Phi_z(T)=0.
```

Une sphère q4 passant par `a,b,x,y` correspond exactement au point :

```text
T = ell_x intersection ell_y.
```

Sa profondeur sur un cover `C` est :

```text
d_C(T) = #{z in C : Phi_z(T)<0}.
```

La génération q4 pour une ancre devient donc :

> énumérer les intersections de droites de profondeur `< h_4`, puis appliquer les tests de lentille, owner et positivité.

Le disque de Jung borne en outre le domaine utile :

```text
|T|² <= D²/2.
```

### 4.1 Version exacte branch-and-bound, sans arrangement global

L’interdit d’architecture porte sur un arrangement global résident. Ici, la structure est locale à une ancre, temporaire et bornée en profondeur.

Construire un petit arbre 2D de cellules dans le plan des centres :

1. pour chaque cellule `Q` et chaque forme affine `Phi_z`, calculer `min_Q Phi_z` et `max_Q Phi_z` aux quatre coins, exactement ;
2. si `max_Q Phi_z < 0`, le site est intérieur sur toute la cellule : crédit collectif ;
3. si `min_Q Phi_z > 0`, il est extérieur sur toute la cellule ;
4. sinon sa droite traverse la cellule et reste dans la liste de conflit ;
5. tuer la cellule dès que le crédit intérieur atteint `h_4` ;
6. subdiviser seulement les cellules vivantes ;
7. lorsque la liste de droites traversantes est petite, énumérer leurs intersections, calculer `d_C(T)` exactement et former les groupes de sphères.

C’est le dual exact des descentes de témoins déjà utilisées dans l’espace des points :

```text
crédit ALL,
élagage fail-open,
cutoff ponctuel,
count -> scan -> fill.
```

La structure est naturellement GPU-friendly par vagues de cellules. Elle vise le régime `eight_clusters`, où de grandes régions du plan des centres sont profondes et peuvent être tuées collectivement.

### 4.2 Porte mathématique minimale

Avant toute optimisation :

```text
q4_power(a,b,x,y,z) a le même signe que Phi_z(T_xy)
```

sur un oracle rationnel borné.

Puis comparer :

```text
intersections vivantes du plan des centres
contre
BallKeys q4 survivantes de la baseline
```

après RLE, sur petits nuages et sur la fixture à contribution du côté opposé de `c829872`.

Mutants utiles :

```text
cell-credit-nonstrict,
drop-crossing-line,
jung-disk-too-small,
intersection-depth-minus-one.
```

Cette voie n’est pas à coder avant le sweep deux-côtés. Elle est le plan B rigoureux si `sum_cover_over_seeds` reste le poste dominant.

---

## 5. Ajouter un préflight de travail, pas seulement de sortie

Les audits sur la taille de sortie restent valides et orthogonaux. `eight_clusters,n=1000` montre cependant l’autre extrême : l’objet final est raisonnable, mais le travail de génération est énorme.

Le préflight doit donc publier deux familles de budgets :

```text
output_preflight:
  événements, facettes, incidences, octets ;

work_preflight:
  ancres vivantes,
  seeds,
  sum_cover_over_seeds,
  complétions de lentille,
  estimation baseline/axial.
```

Un `max_work_records` ou une continuation par tuiles est nécessaire pour qu’un cas comme `eight_clusters` ne découvre son coût qu’après plusieurs heures.

---

## Ordre recommandé à Claude

1. Implémenter le sweep axial à deux côtés déjà spécifié et le recevoir sur `eight_clusters,n=1000/2000`.
2. Ajouter le dispatch adaptatif `baseline|axial` par seed, jamais par famille.
3. Publier `sum_cover_over_seeds` et les temps décomposés.
4. Si le calcul `(A,B)` reste dominant, ouvrir la factorisation locale dans le plan des centres.
5. En parallèle, trancher le produit exact et le préflight de sortie pour 30M ; le problème de sortie et le problème de génération dense sont deux verrous différents.

## Conclusion

Le nouveau reçu ne réfute pas l’axial : il montre exactement où il devient utile. Il réfute seulement l’idée qu’un même chemin CPU soit optimal partout.

La réponse immédiate est un hybride exact piloté par la densité de complétions du seed. La réponse structurelle, si les millions de seeds restent le mur, est de cesser de traiter chaque seed comme un nouveau problème : pour une ancre fixée, toutes les sphères q4 sont déjà les sommets peu profonds d’un même arrangement local de droites dans le plan des centres.
