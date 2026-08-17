# Audit ciblé après `48e4467` — sweep reçu, mais le kernel CPU alloue encore par seed

Date : 17 août 2026.  
Pin audité : `48e44675413c1760827dec6123e54a33775ba562`.  
Contre-audits pris en compte : `b8c4a4d` (cœur seed-local et sélection sur arbre), `55178b3` (dispatch adaptatif et plan des centres).

## Verdict

Le sweep axial à deux côtés est **reçu mathématiquement**.

Pour un seed fixé, l'implémentation calcule correctement

```text
d_cover(mu)
  = p
  + #{B_z > 0 et mu_z < mu}
  + #{B_z < 0 et mu_z > mu},
```

avec :

- permanents stricts `B=0, A<0` ;
- `U` égal à la `k`-ième racine positive croissante et `L` à la `k`-ième racine négative décroissante, multiplicité comprise ;
- égalités de frontière conservées ;
- groupes de même `mu` fusionnés entre les deux signes ;
- préfixes positifs et suffixes négatifs stricts ;
- minimum canonique pris sur tous les compléteurs valides du groupe.

J'ai en outre confronté abstraitement la logique `[L,U] + préfixe/suffixe` à une énumération directe sur des milliers de multisets rationnels signés aléatoires : aucun désaccord de groupe vivant n'apparaît. La fixture `1513/49` est géométriquement correcte et la nouvelle mesure `eight_clusters,n=1000` confirme un gain réel.

Je ne demande donc aucune correction de la géométrie de production.

Deux raccords d'implémentation et de réception sont toutefois utiles avant d'ouvrir une structure plus complexe.

---

## 1. Supprimer les allocations dynamiques par seed avant de conclure sur le coût de `(A,B)`

Le code sait que le nombre de groupes distincts dans la fenêtre est borné par

```text
2 (h_4-p) <= 16.
```

Il construit pourtant, pour chacun des `4 416 744` seeds du cas dur :

```cpp
std::vector<MuGroup> groups;
std::vector<u64> pos_before;
std::vector<u64> neg_after;
```

et chaque `MuGroup` possède encore :

```cpp
std::vector<i32> members;
```

Il n'y a aucune petite-vector optimisation dans `std::vector`. Le chemin effectue donc potentiellement plusieurs allocations de tas par seed, puis une allocation supplémentaire par groupe non vide. À plusieurs millions de seeds, ce coût peut représenter une part substantielle des `87,5 s` attribuées globalement au balayage axial.

Il faut fermer cette constante avant de décider que seul le calcul arithmétique de `(A,B)` subsiste.

### Réécriture exacte proposée

Utiliser des tableaux fixes :

```cpp
struct MuGroupFixed {
    AxialSite head;
    u64 npos;
    u64 nneg;
    u64 depth;
    bool alive;
};

std::array<MuGroupFixed, 16> groups;
std::array<u64, 16> pos_before;
std::array<u64, 16> neg_after;
u8 ngroups = 0;
```

Conserver dans un tableau parallèle réutilisé l'identifiant de groupe de chaque `AxialSite` :

```cpp
std::vector<u8> axial_gid;  // capacité conservée entre seeds, 0xff hors fenêtre
```

La chaîne devient :

1. calculer les seuils `L,U` comme aujourd'hui ;
2. scanner `axial`, classer les racines hors fenêtre et attribuer un `gid<16` aux autres ;
3. trier les seize groupes au plus et remapper les `gid` ;
4. calculer `d_j` dans les tableaux fixes ;
5. scanner une seconde fois `axial` et appeler `valid_completion` seulement si `groups[gid].alive` ;
6. maintenir directement le meilleur `BallCandidate` de chaque groupe.

Le nombre de sites revisités est le même ordre que dans les vecteurs de membres actuels. On retire seulement les allocations, l'indirection et les destructions de petits conteneurs. Cette forme est également bien plus proche du futur kernel GPU.

### Mesure à faire avant l'arbre axial

Sur `uniform,n=1600` et `eight_clusters,n=1000`, publier séparément :

```text
t_AB,
t_group_classification,
t_valid_completion,
seeds,
AB_pairs,
groups_distincts,
valid_completion_calls.
```

Comparer l'ancien chemin axial à la variante sans allocation avec sorties identiques. Si `t_AB` domine encore après cela, le cœur seed-local et la sélection top-k sur l'arbre de `b8c4a4d` deviennent la prochaine étape justifiée. Sinon une partie du mur était simplement l'allocateur, cette divinité discrète à laquelle les boucles internes sacrifient volontiers des secondes entières.

---

## 2. Les compteurs mélangent actuellement des racines et des groupes

`axial_groups_killed_two_sided` est incrémenté :

- une fois **par racine/site** positif sous `L` ou négatif au-dessus de `U` ;
- une fois **par groupe** en fenêtre tel que `d_j >= h_4`.

Le reçu l'interprète prudemment comme `877 737 092 racines`, mais le champ porte le mot `groups` et additionne deux unités différentes. Ce nombre ne peut pas servir à calibrer un dispatch adaptatif ni à estimer les appels évités.

Séparer :

```cpp
u64 axial_roots_pruned_cross_window;
u64 axial_groups_pruned_two_sided_depth;
u64 axial_groups_in_window;
u64 axial_valid_completion_calls;
```

Les deux premières quantités ont des significations différentes : la première mesure le volume du balayage déjà payé ; la seconde mesure le vrai nombre de formations q4 évitées après groupement.

---

## 3. La fixture `1513/49` n'isole pas causalement deux mutants annoncés

La production reste correcte, mais les commentaires de la porte et du reçu sont trop forts pour :

```text
axial-ignore-opposite-side
axial-reverse-negative
```

Dans le code actuel, les deux mutants ne modifient que le calcul de `d_j` **à l'intérieur** de `[L,U]`. La classification par `L` et `U` continue à utiliser les deux côtés sans mutation. Or, dans la fixture `1513/49`, le compléteur positif est précisément rejeté **avant la table des groupes**, parce que sa racine est sous `L` fourni par les trois racines négatives.

Cette fixture ne peut donc pas, à elle seule, faire « survivre la sphère » sous ces deux flags comme l'affirment les commentaires. Les mutants meurent vraisemblablement sur les nuages généraux de la porte, ce qui reste utile, mais la causalité annoncée n'est pas celle du code.

### Porte propre

Extraire une primitive testable indépendante de la génération géométrique :

```cpp
AxialSweepResult axial_two_sided_sweep(
    span<const AxialSite> sites,
    u64 permanents,
    u64 h,
    u32 flags);
```

Puis graver un multiset synthétique minimal :

```text
une racine positive mu=0,
trois racines négatives mu=1,2,3,
p=0, h=3.
```

Le chemin normal rejette `mu=0` par le côté opposé. Un vrai mutant `ignore-opposite` doit la conserver ; un vrai mutant `reverse-negative` doit inverser le verdict. La fixture géométrique `1513/49` reste ensuite la porte d'intégration entre ce sweep et les `BallKey`.

Pendant la réception, `kAxialVerify` gagnerait aussi à recouper **tous les groupes en fenêtre avant leur mort**, pas seulement les groupes finalement émis. Pour un membre quelconque `y` du groupe, `B_y != 0` garantit que `(a,b,x,y)` est non coplanaire ; sa sphère permet de comparer `d_j` au scan `q4_power<0`, même si ce membre échoue ensuite à l'owner ou à la positivité.

---

## Ordre utile

1. Conserver `48e4467` comme référence exacte du sweep bidirectionnel.
2. Remplacer les petits vecteurs par les tableaux fixes `<=16` et séparer les compteurs d'unités.
3. Extraire la primitive de sweep et rendre les mutants causalement isolés.
4. Mesurer de nouveau `uniform,n=1600` et `eight_clusters,n=1000`.
5. Ensuite seulement choisir entre :
   - dispatch adaptatif baseline/axial ;
   - cœur seed-local + top-k sur l'arbre ;
   - plan local des centres si `AB_pairs` reste réellement dominant.

Le sweep à deux côtés est une bonne avancée et peut rester en production opt-in. La prochaine économie la moins risquée n'est pas encore une nouvelle géométrie : c'est d'exploiter jusqu'au bout sa borne mathématique de seize groupes, jusque dans la représentation mémoire du kernel.
