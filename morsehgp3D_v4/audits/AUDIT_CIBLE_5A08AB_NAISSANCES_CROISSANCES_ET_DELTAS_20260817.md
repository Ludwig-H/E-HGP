# Audit ciblé après `5a08ab` — une forêt de partitions ne suffit pas encore à restituer la hiérarchie polyédrique

Date : 17 août 2026.  
Pin audité : `5a08ab682b53c13fcea1aee4ccd0dad2fe928644`.  
Pins principaux inclus : `052fed4` (`SpherePlateau`) et `5a08ab` (rôles actif/attachement).

## Verdict

Les deux corrections de forêt sont reçues positivement.

- Le quotient sphérique est mathématiquement correct : pour une boule `B`, les simplexes de Gabriel sont exactement `sigma = I_B union T`, avec `T subset U_B`, `|T| = K+1-|I_B|` et `c_B in conv(T)` fermé.
- `center_in_conv` réalise correctement Carathéodory en dimension trois par paire diamétrale, triangle fermé ou tétraèdre fermé.
- Le rôle d'une facette de plateau est correctement décidé par son rayon de naissance : retirer `v in T` est actif exactement lorsque `c_B notin conv(T sans {v})`; retirer un intérieur est toujours un attachement.
- Le macro-lot corrigé compte maintenant seulement les composantes présentes juste avant le niveau. La partition et les arités de fusion du sous-flux borné sont cohérentes.

Il reste cependant un verrou distinct avant le raccord WSPD et le rendu :

> `ForestResult` décrit encore seulement les partitions et un squelette de fusions. Il ne conserve pas les **naissances de composantes**, les **croissances sans fusion**, ni les **facettes nées pendant une fusion**.

Ce n'est pas une demande de confort d'API. Ces données sont précisément la trajectoire polyédrique d'une branche HGP et alimentent `Delta F`, `F_K^render`, les persistences et les cartes verticales.

---

## 1. Ce que le code conserve, et ce qu'il perd

Le code courant publie essentiellement

```cpp
ForestNode { batch, absorbed };
new_attachments;  // compteur global
```

avec un `ForestNode` seulement si au moins deux racines pré-lot sont absorbées.

Pour une composante finale `C` d'un macro-lot de niveau `lambda`, notons :

```text
P_C = composantes distinctes présentes juste avant lambda et envoyées dans C,
N_C = facettes de C dont le rayon de naissance vaut exactement lambda.
```

Le lot possède quatre comportements :

```text
|P_C| = 0, N_C non vide  : naissance d'une composante,
|P_C| = 1, N_C non vide  : croissance d'une branche sans fusion,
|P_C| >= 2               : fusion, éventuellement avec nouvelles facettes,
|P_C| = 1, N_C vide      : aucune modification utile à sérialiser.
```

Le noyau actuel ne restitue correctement que le nombre `|P_C|` dans le troisième cas. Il perd :

1. le premier cas, car aucun nœud n'est créé ;
2. le deuxième cas, car aucun nœud n'est créé et `new_attachments` n'est qu'un total global ;
3. dans le troisième cas, l'identité des enfants, de la composante finale et des facettes nées pendant la fusion ;
4. le niveau exact de naissance des facettes actives matérialisées pour la première fois.

Deux exécutions peuvent donc avoir les mêmes partitions, les mêmes couples `(batch,absorbed)` et le même nombre global d'attachements, tout en ayant des trajectoires de surfaces différentes.

Le dossier de représentation HGP appelle justement cette donnée

```text
Delta F_t = F_{t+1} sans F_t.
```

Elle ne peut pas être reconstruite depuis un compteur.

---

## 2. La fixture carrée montre déjà une naissance manquante

La fixture cocyclique existante est maintenant correctement interprétée au niveau `K=3` : les quatre facettes-triangles naissent ensemble au niveau carré `100`, et aucune composante pré-lot n'est absorbée.

Le test exige actuellement :

```text
K=3 : aucun ForestNode.
```

C'est correct si `ForestNode` signifie exclusivement « fusion ». Ce n'est pas encore une représentation de la hiérarchie : la composante non triviale à quatre facettes **naît** au niveau `100` et doit produire un état de branche

```text
kind = birth,
children = empty,
born_facets = les quatre triangles,
level^2 = 100.
```

Sans cet état, la composante existe dans l'instantané union-find mais n'existe dans aucun objet sérialisable de la forêt. Une partition finale peut vivre sans acte de naissance ; une hiérarchie, moins commodément.

---

## 3. La fixture q2 à un intérieur perd le delta de la fusion

Pour

```text
a = (0,0,0),
b = (4,0,0),
z = (2,1,0),
```

le lot carré `4` fusionne les deux facettes actives `{a,z}` et `{b,z}`, tandis que `{a,b}` naît dans le lot.

La correction actuelle donne justement

```text
absorbed = 2,
new_attachments = 1.
```

Le vrai état est plus précis :

```text
kind = fusion,
children = composantes de {a,z} et {b,z},
born_facets = {{a,b}}.
```

L'identité de `{a,b}` et son rattachement à cette fusion sont indispensables pour reconstruire la surface du parent. Un compteur global ne permet pas de savoir quelle composante a reçu quelle facette.

---

## 4. Fixture entière d'une croissance sans aucune fusion

Le deuxième cas n'est pas encore gravé. Prendre les quatre points u16 :

```text
a = (10,20,20),
b = (30,20,20),
z = (20,25,20),
w = (20,31,20).
```

### 4.1 L'événement tardif

La boule diamétrale de `ab` a

```text
centre = (20,20,20),
R^2 = 100.
```

On a

```text
|z-centre|^2 = 25 < 100,
|w-centre|^2 = 121 > 100.
```

Donc

```text
sigma = {a,b,z}
```

est un simplexe de Gabriel q2 de profondeur un, pour la forêt `K=2`, au niveau carré `100`.

Ses facettes actives sont `{a,z}` et `{b,z}` ; `{a,b}` est l'attachement né au lot.

### 4.2 Les deux facettes actives sont déjà dans la même composante

Pour le triangle `{a,z,w}`, la plus longue arête est `aw`, de longueur carrée

```text
|aw|^2 = 221.
```

Le test diamétral entier donne

```text
|2z-a-w|^2 = 101 < 221,
|2b-a-w|^2 = 1021 > 221.
```

Ainsi `{a,z,w}` est un événement q2 de la forêt `K=2` au niveau

```text
221/4 < 100,
```

et relie `{a,z}` à `{z,w}`. Symétriquement, `{b,z,w}` relie `{b,z}` à `{z,w}` au même niveau antérieur.

Juste avant `100`, `{a,z}` et `{b,z}` appartiennent donc déjà à **une seule composante**.

Au niveau `100`, l'événement `{a,b,z}` ne fusionne rien : il ajoute seulement la facette `{a,b}` à cette composante.

La sortie correcte est :

```text
kind = growth,
children = [composante existante],
born_facets = {{a,b}},
aucun nœud de fusion.
```

Une porte `q2_growth_without_merge` doit tuer un mutant qui supprime les mises à jour à un seul enfant. Les partitions et les nombres de nœuds resteraient identiques ; seul le delta polyédrique discrimine la faute.

---

## 5. ABI minimale conseillée

Conserver `ForestNode` comme vue dérivée des seules fusions est possible, mais la source de vérité doit devenir une mise à jour de composante :

```cpp
enum class UpdateKind : u8 {
  kBirth,
  kGrowth,
  kFusion,
};

struct ComponentUpdate {
  u64 batch;
  Q4Level level;
  ComponentStateId post;
  UpdateKind kind;
  SmallVector<ComponentStateId> children;  // 0, 1 ou >= 2
  SmallVector<FacetKey> born_facets;        // rho(f) = level
  SmallVector<EventRef> generators;         // optionnel mais utile au rendu
};
```

Après les unions d'un lot, pour chaque racine finale touchée :

1. collecter les racines pré-lot distinctes `P_C` ;
2. collecter les `FacetKey` nouvelles de rôle attachement `N_C` ;
3. produire :

```text
Birth  si |P_C| = 0,
Growth si |P_C| = 1 et N_C non vide,
Fusion si |P_C| >= 2,
rien   si |P_C| = 1 et N_C vide.
```

Dans une fusion avec `N_C` non vide, le même record porte simultanément les enfants et le delta de surface. `ForestNode{absorbed}` devient alors une projection triviale des records `Fusion`.

Il faut attribuer des identités persistantes aux états de composantes, et non exposer les indices mouvants de l'union-find.

---

## 6. Les facettes actives nouvellement rencontrées ont aussi un vrai niveau de naissance

Le code traite correctement une facette active jamais vue comme une composante singleton pré-lot. Mais il ne conserve pas son niveau de naissance exact, seulement le fait qu'il est strictement plus petit que le lot courant.

Pour les persistences et les feuilles de la hiérarchie, il faut une table

```cpp
FacetBirth {
  FacetKey facet;
  Q4Level exact_level;
};
```

Attention : une facette active n'est pas nécessairement elle-même un simplexe de Gabriel. On ne peut donc pas supposer qu'elle apparaîtra automatiquement dans le flux événementiel de l'ordre inférieur.

La voie exacte et bornée est simple sous `K<=10` : à la première rencontre d'une `FacetKey`, rechercher sa miniboule parmi ses supports de cardinal 2, 3 ou 4, soit au plus

```text
C(10,2) + C(10,3) + C(10,4) = 375
```

candidats, puis mémoïser le niveau. Le juge indépendant possède déjà cette recherche. La production doit employer ses formes q2/q3/q4 et le comparateur exact, pas appeler le juge OBig.

Si le produit public choisit de supprimer les composantes triviales antérieures, cette politique doit être explicite ; le niveau reste néanmoins nécessaire pour la persistance de la branche lors de sa première fusion.

---

## 7. Portes qui ferment réellement le contrat

Trois fixtures suffisent :

```text
q2_one_interior_attachment:
  Fusion, 2 enfants, born_facets = {{a,b}};

square_cospherical_K3_birth:
  Birth, 0 enfant, 4 facettes nées au niveau 100;

q2_growth_without_merge:
  Growth, 1 enfant, born_facets = {{a,b}} au niveau 100.
```

Mutants :

```text
drop_birth_updates,
drop_growth_updates,
drop_born_facets_on_fusion.
```

Comparer l'identité des enfants et des facettes, pas seulement leurs cardinalités.

---

## 8. Ordre de travail

1. Étendre le noyau de forêt vers `ComponentUpdate` et graver les trois cas.
2. Ajouter le registre exact de naissance des facettes ou déclarer précisément la politique de suppression des singletons.
3. Raccorder ensuite les flux WSPD et le `SpherePlateau` d'échelle.
4. Construire `F_K^render` et les `Delta F` directement depuis les mises à jour.
5. Ajouter ensuite les cartes verticales.

Le raccord WSPD avant cette ABI déplacerait simplement la perte d'information dans une bibliothèque plus difficile à corriger.

## Conclusion

`052fed4` et `5a08ab` réparent bien les composantes et la chronologie des **fusions**. Le verrou restant est de conserver la chronologie de l'objet polyédrique lui-même.

Une hiérarchie HGP n'est pas seulement la liste des moments où plusieurs composantes se rencontrent. Une composante peut naître avec plusieurs facettes, croître sans fusion, ou fusionner tout en acquérant de nouvelles facettes. Le noyau possède déjà toute cette information dans ses rôles de lot ; il faut maintenant cesser de la réduire à deux compteurs avant de brancher le reste du pipeline.
