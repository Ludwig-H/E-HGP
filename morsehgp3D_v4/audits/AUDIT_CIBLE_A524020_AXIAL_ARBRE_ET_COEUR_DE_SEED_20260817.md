# Audit ciblé après `a524020` — le sweep à deux côtés ne supprime pas le balayage `A,B`

Date : 17 août 2026.  
Pin audité : `a524020eb27fcc755c4afed32ed9ed30b563b2ce`.

## Verdict

Le reçu est très utile : `eight_clusters` révèle un régime dense réellement distinct, où l’axial borné devient rentable et où le coût est presque entièrement dans la génération q4.

Je reçois donc la remontée en priorité du chemin axial. Il faut cependant corriger un point de diagnostic avant de coder :

> le sweep exact à deux côtés supprime les scans `depth_dead` par groupe et les doublons bilatéraux, mais il ne supprime pas le premier balayage qui calcule `A_z,B_z` pour chaque site du cover et pour chacun des 4,4 millions de seeds.

Le reçu dit précisément que ce balayage est désormais le poste restant. Le sweep à deux côtés est nécessaire, mais il ne peut pas, à lui seul, résoudre `eight_clusters`. Il faut déplacer la sélection axiale sur l’arbre et tuer des seeds avant de matérialiser tous leurs `AxialSite`.

---

## 1. Un cœur universel exact propre au seed

Fixons une ancre owner `(a,b)` et un seed aigu `(a,b,x)`. Notons

```text
D = |b-a|^2,
E = |x-a|^2,
F = (b-a)·(x-a),
X = |b-x|^2 = D+E-2F,
G = DE-F^2,
n = (b-a)×(x-a).
```

La forme q3 est

```text
P(z) = G |z-a|^2 - W·(z-a),
B(z) = n·(z-a).
```

Les sphères du faisceau passant par `(a,b,x)` s’écrivent

```text
Phi_mu(z) = P(z) - mu B(z).
```

Leur centre et leur rayon carré sont

```text
c_mu = a + (W + mu n)/(2G),
R_mu^2 = R_3^2 + mu^2/(4G),
R_3^2 = DEX/(4G).
```

Toute complétion q4 acceptée par la production est un tétraèdre bien centré dont l’arête owner est un diamètre de longueur carrée `D`. Sa circumboule est sa miniboule ; Jung en dimension trois donne

```text
R_mu^2 <= 3D/8.
```

Par conséquent

```text
2 mu^2 <= J,
J = D (3G - 2EX).
```

Donc un site `z` est strictement intérieur à **toute** sphère q4 admissible de ce seed dès que

```text
P(z) < 0
et
2 P(z)^2 > J B(z)^2.
```

En effet, pour tout `mu` admissible,

```text
Phi_mu(z) <= P(z) + sqrt(J/2) |B(z)| < 0.
```

C’est un vrai fuseau seed-local, tridimensionnel, beaucoup plus large que les seuls permanents coplanaires `B=0, P<0`.

### Usage

Avant de construire le tableau `AxialSite` :

```text
compter les témoins seed-universels jusqu’à h_4 ;
si le compte atteint h_4, tuer le seed entier.
```

Le compte peut partir de l’antichaîne de cover déjà construite pour l’ancre. Omettre les témoins hors du cover reste fail-open.

### Test ALL sur une boîte

Pour un nœud `Z`, calculer exactement :

```text
Pmax(Z) = max P(z),
Babs(Z) = max |B(z)|.
```

`Pmax` utilise les `axis_max` q3 déjà présents ; `Babs` est l’extrême d’une forme linéaire sur l’AABB.

Alors

```text
Pmax(Z) < 0
et
2 Pmax(Z)^2 > J Babs(Z)^2
```

implique que tout le nœud est témoin universel : créditer son poids en O(1). Si `Pmin(Z) >= 0`, aucun point du nœud ne convient. Sinon descendre, avec arrêt à `h_4`.

Les produits atteignent environ 212 bits sous u16 : réutiliser `U320`, pas `i128`.

Cette primitive attaque directement le nombre de seeds qui paient le balayage axial complet.

---

## 2. Pour les seeds survivants : top-k axial par branchement sur l’arbre

Le chemin courant calcule encore `(A_z,B_z)` pour tout `z` du cover. Or on ne demande que :

```text
les h_4 plus petites racines du côté B>0,
les h_4 plus grandes racines du côté B<0,
avec toutes les égalités de frontière.
```

Comme `h_4 <= 8`, il faut faire une requête d’ordre borné sur l’arbre, non aplatir le cover.

Pour un nœud, les bornes exactes disponibles sont

```text
Alo <= P(z) <= Ahi,
Blo <= B(z) <= Bhi.
```

Si `0 < Blo <= Bhi`, des bornes rationnelles sûres sur `mu=P/B` sont :

```text
lower = Alo / (Alo < 0 ? Blo : Bhi),
upper = Ahi / (Ahi <= 0 ? Bhi : Blo).
```

La preuve est une simple monotonie du quotient selon le signe du numérateur. Les comparaisons restent exactes avec le comparateur U192 existant.

Pour `Bhi < 0`, normaliser

```text
A' = -A,
B' = -B > 0
```

puis appliquer les mêmes bornes ; la recherche se fait dans l’ordre décroissant de `mu`.

Si l’intervalle de `B` rencontre zéro, descendre. À une feuille `B=0`, traiter le permanent éventuel.

### Parcours

- côté positif : visiter d’abord les enfants de plus petite borne inférieure ; dès que `h_4` éléments sont connus, élaguer un nœud dont `lower` est strictement au-delà du seuil courant ;
- côté négatif : symétriquement avec la borne supérieure ;
- ne jamais élaguer à égalité, afin de conserver tout le groupe frontière ;
- un DFS ordonnant les deux enfants suffit pour le prototype CPU : éviter une `priority_queue` allouée 4,4 millions de fois.

Le pire cas reste le scan complet, donc la voie est fail-open. Dans le régime dense, le nombre de feuilles évaluées peut en revanche tomber de `|cover|` à un petit multiple de la profondeur de l’arbre et de `h_4`.

Une fois les deux listes extrémales obtenues, le sweep à deux côtés déjà proposé calcule exactement

```text
d_cover(mu) = p + P_<(mu) + N_>(mu)
```

sur au plus seize groupes. Il peut alors supprimer `depth_dead` du chemin axial.

---

## 3. Ce qu’il faut mesurer avant toute conclusion

Séparer dans `t_gen` :

```text
t_seed_core,
t_axial_tree_bounds,
t_axial_leaf_AB,
t_two_sided_reduce,
t_valid_completion,
t_q4_form.
```

Publier aussi :

```text
seeds_total,
seeds_killed_seed_core,
axial_tree_nodes,
axial_leaf_AB,
flat_axial_sites_equivalent,
groups_bilateral,
groups_depth_killed.
```

La porte importante sur `eight_clusters,n=1000` est :

```text
mêmes BallKeys post-RLE,
mêmes SpherePlateau,
mêmes événements/deltas/rendu,
mais axial_leaf_AB << flat_axial_sites_equivalent.
```

Si les bornes de quotient restent trop lâches et que presque toutes les feuilles sont visitées, le prochain chantier sera alors clairement le traitement groupé des seeds d’une ancre. Il ne faudra pas attribuer cet échec au sweep à deux côtés, qui agit plus tard dans la chaîne.

---

## 4. Portes minimales

1. Fixture du contre-audit précédent `R^2=1513/49` : mort par le seul côté opposé.
2. Fixture de frontière du cœur seed-local : égalité

   ```text
   2P^2 = J B^2
   ```

   non comptée ; mutant `seed-core-nonstrict` tué.
3. Comparaison flat axial contre tree axial sur les deux familles jugées et la sphère cosphérique.
4. Mutant `ratio-bound-wrong-sign` : utilise `Bhi` au lieu de `Blo` pour un numérateur négatif et doit perdre une racine extrême.
5. Mutant `tree-prune-boundary-ties` : remplace l’élagage strict par large et perd le groupe ex æquo.
6. Compte exact `d_cover` comparé au scan `q4_power<0`, pas seulement le verdict mort/vivant.

---

## Ordre recommandé

1. Implémenter le sweep à deux côtés pour fermer la redondance et supprimer `depth_dead`.
2. Ajouter immédiatement le cœur seed-local de Jung, car il peut tuer avant tout tableau axial.
3. Ajouter la sélection top-k sur l’arbre pour réduire le nombre réel de calculs `A,B`.
4. Garder un choix adaptatif : baseline sur petits covers, axial-tree sur covers denses.
5. Reprendre ensuite `eight_clusters,n=1000`, puis seulement le run `n=8000` à risque.

Le reçu a correctement trouvé le régime où l’axial devient utile. Il faut maintenant optimiser le poste effectivement mesuré. Le sweep bidirectionnel ferme le calcul des groupes ; le cœur seed-local et la requête top-k ferment le balayage qui les précède.