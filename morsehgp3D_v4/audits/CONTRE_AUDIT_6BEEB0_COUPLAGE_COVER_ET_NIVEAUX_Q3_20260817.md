# Contre-audit ciblé après `6beeb0d` — cover couplé exact et ordre des niveaux q3

Date : 17 août 2026.  
Pin de code audité : `5964214c43dd58618e5d3c389d889d574f3ba7f6` inclus.  
Note contre-auditée : `AUDIT_CIBLE_5964214_Q3_EVENT_ET_PROCHAIN_VERROU_20260817.md`, commit `6beeb0d`.

Je confirme le verdict de cette note : les paquets de témoins, le passage site-major, le cover rectangulaire, la `BallKey` et la formule du niveau q3 sont mathématiquement corrects. Je ne répète pas les raccords d'événement, d'owner ou de capacité déjà prescrits.

Deux verrous utiles restent à expliciter.

---

## 1. Remplacer le cover décorrélé par un classifieur couplé exact

Le test actuel du cover rectangulaire utilise séparément :

```text
dist(2Box(Z), S_AB)^2
et
Dmax^2 = max_{a in A,b in B} |a-b|^2.
```

Il est sûr, mais il perd la corrélation entre la somme `a+b` et la différence `a-b` : le point de `A×B` réalisant la somme la plus favorable n'est généralement pas celui qui réalise `Dmax`.

La relation exacte à classifier est

```text
Psi(a,b,z) = 3|a-b|^2 - |2z-a-b|^2.
```

Un site appartient au cover de l'ancre exactement lorsque `Psi >= 0`.

### 1.1 Extrema exacts sur trois AABB

La fonction est séparable par coordonnées :

```text
Psi = sum_i psi_i,
psi_i(a,b,z) = 3(a-b)^2 - (2z-a-b)^2.
```

Les variables des trois axes étant indépendantes,

```text
Psi_min = sum_i psi_i,min,
Psi_max = sum_i psi_i,max.
```

Pour des intervalles entiers `A=[a0,a1]`, `B=[b0,b1]`, `Z=[z0,z1]` :

#### Maximum d'un axe

`psi` est convexe séparément en `a` et `b`, puis concave en `z`. Donc

```text
psi_max = max_{a in {a0,a1}, b in {b0,b1}}
          [3(a-b)^2 - dist(a+b, [2z0,2z1])^2].
```

Quatre candidats suffisent.

#### Minimum d'un axe

`psi` est concave en `z`, donc le minimum est atteint pour `z=z0` ou `z=z1`.
À `z` fixé, poser `x=a-z`, `y=b-z` :

```text
psi = 2(x^2+y^2-4xy).
```

Le minimum sur le rectangle `A×B` est atteint parmi :

```text
- les quatre coins ;
- pour a in {a0,a1}, b = clamp(2a-z, B) ;
- pour b in {b0,b1}, a = clamp(2b-z, A).
```

Ce sont exactement les minima des quatre arêtes ; l'unique point stationnaire intérieur est un point selle. Huit candidats par valeur de `z` suffisent.

Sous u16, chaque `psi_i` et la somme 3D tiennent largement dans `i64`.

### 1.2 Trichotomie de bloc

Pour une tâche `(A_node,B_node,Z_node)` :

```text
Psi_max < 0  => NONE : aucune incidence cover ;
Psi_min >= 0 => ALL  : toutes les incidences cover ;
sinon        => MIXED.
```

Cette trichotomie est exacte sur l'enveloppe continue des boîtes, donc fail-open sur les points du nuage. Elle subsume le test actuel : tout nœud rejeté par `dist>S/Dmax` est aussi `NONE`, mais l'inverse est souvent faux.

### 1.3 Architecture recommandée

Ne pas construire d'abord un tableau de toutes les ancres `(s,D^2)`. Utiliser directement les sous-arbres radix des deux facteurs et le witness-tree :

```text
Task = (A', B', Z').
```

Les valeurs `h_a/h_b` déjà calculées peuvent être agrégées par nœud :

```text
ha_min, ha_max sur A',
hb_min, hb_max sur B'.
```

Avec `need=h_3-h_coeur` :

```text
ha_min+hb_min >= need => toutes les ancres sont mortes ;
ha_max+hb_max <  need => toutes les ancres sont survivantes ;
sinon                 => bloc mixte, scission d'un facteur endpoint.
```

Sur un bloc d'ancres survivantes, appliquer ensuite `Psi` : `NONE` se jette, `ALL` devient un handle factorisé, `MIXED` se scinde. On partage ainsi aussi le **filtre exact du cover**, alors que le commit `40b309c` ne partage encore que la traversée haute puis rescane les handles pour chaque ancre.

Première porte, counter-only : comparer à `cover=root|rectangle` sur petits `n`, avec

```text
missing_cover_ids = 0,
extra_cover_ids   = 0,
relation_ALL_mass,
relation_MIXED_mass,
point_tests_avoided.
```

Si `MIXED` domine encore, seulement alors passer au LBVH des circumcentres. Cette expérience décide quel arbre est réellement utile, au lieu de les construire tous par réflexe.

---

## 2. Le prochain verrou de correction : comparer les niveaux sans overflow

La fraction q3 réduite est correcte :

```text
level = num/den = D E X / (4G),
num > 0, den > 0.
```

Mais la forêt exige maintenant un ordre exact des niveaux. Un produit croisé en `i128` est interdit.

Sous u16 :

```text
D,E,X < 2^34,
num = D E X < 2^101,
G < 2^68,
den = 4G < 2^70.
```

Comparer deux niveaux demande

```text
num1*den2 ? num2*den1,
```

et chaque produit peut atteindre moins de `2^171`. Un entier non signé de **192 bits** suffit avec une marge nette ; `i128` ne suffit pas.

### Primitive minimale

```cpp
struct U192 { u64 lo, mid, hi; };

U192 mul_level(u128 num, u128 den); // préconditions de largeur 101×70
int compare_level(Q3Level x, Q3Level y) {
  return cmp(mul_level(x.num,y.den),
             mul_level(y.num,x.den));
}
```

L'égalité de niveaux se teste directement par les couples réduits `(num,den)`. Ajouter des fixtures aux coordonnées u16 extrêmes et un mutant qui tronque le mot haut du produit.

Les événements de même niveau doivent être traités dans **un même macro-lot** : racines de composantes gelées avant le lot, puis toutes les multifusions du plateau, sans chronologie binaire artificielle.

---

## 3. Raccord immédiat vers la forêt

Une fois le `Q3Event` unique matérialisé comme le demande `6beeb0d` :

```text
d = |InteriorIds|,
K = d+2.
```

Pour le support `S={a,b,x}` et les intérieurs `I`, les trois facettes actives sont exactement

```text
I union (S sans {a}),
I union (S sans {b}),
I union (S sans {x}).
```

Relier ces trois facettes par un chemin de deux unions suffit pour la connectivité du K-graphe ; le rendu conserve séparément `F_K^render` selon le contrat déjà gravé.

---

## Ordre conseillé à Claude

1. Matérialiser et digérer l'enregistrement `Q3Event` complet, comme prescrit par `6beeb0d`.
2. Ajouter le comparateur `U192` et les macro-lots de niveaux égaux : c'est le verrou exact avant la forêt.
3. Prototyper le classifieur couplé `Psi` en counter-only sur les mêmes reçus.
4. Si le gain porte sur `visites_filtre`, développer la récursion `(A',B',Z')`; si les `q3_power` dominent déjà, passer au LBVH des centres.

Il n'y a pas de nouvelle objection géométrique à q3. Le travail utile consiste désormais à conserver l'événement comme un objet unique, à ordonner ses niveaux sans overflow, et à factoriser la dernière relation encore développée ancre par ancre.