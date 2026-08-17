# Audit ciblé après `5a08ab6` — conserver les naissances et croissances de composantes

Date : 17 août 2026.  
Pin audité : `5a08ab682b53c13fcea1aee4ccd0dad2fe928644`.  
Commit de plateau également reçu : `052fed427b75bab7356da796544977a7a905d3e8`.

## Verdict

Les deux corrections récentes sont bonnes et ferment effectivement les audits précédents :

- le quotient `SpherePlateau` correspond à la définition pure de Gabriel hors position générale ;
- la distinction `active` / `attachment` est maintenant correcte, y compris sur les plateaux ;
- les facettes nées dans le lot ne sont plus comptées comme des composantes pré-lot absorbées ;
- le juge recalcule désormais les rayons de naissance par une voie distincte.

Il reste cependant un verrou d'ABI avant de raccorder les flux WSPD réels : **`ForestResult` ne décrit actuellement que les fusions de composantes préexistantes. Il ne conserve ni les naissances de composantes, ni leurs croissances par ajout de facettes au niveau courant.**

Les partitions de test restent justes parce que les `snapshots` conservent tout l'état après chaque lot. Mais ces instantanés sont optionnels, quadratiques en sortie, et ne font pas partie du résultat de production. Le résultat public actuel ne suffit donc pas à reconstruire la hiérarchie `theta_K(r)` comme famille de polyèdres, ni le futur `F_K^render`.

---

## 1. Trois types de transition existent dans un macro-lot

Fixons un niveau exact `lambda` et une composante finale `C` touchée par le lot. Notons :

```text
P(C) = composantes distinctes présentes juste avant lambda qui aboutissent dans C,
N(C) = facettes qui naissent exactement à lambda et appartiennent à C.
```

Il y a trois cas structurels :

```text
|P(C)| = 0, N(C) non vide  -> naissance d'une composante ;
|P(C)| = 1, N(C) non vide  -> croissance d'une composante sans fusion ;
|P(C)| >= 2                -> fusion, éventuellement accompagnée de facettes nouvelles.
```

Le code courant n'émet un `ForestNode` que dans le troisième cas. `new_attachments` est un cardinal global : il ne dit ni **quelles** facettes sont nées, ni **dans quelle** composante elles sont entrées. Le couple

```text
(batch, absorbed)
```

ne représente donc pas les deux premiers cas et ne représente que partiellement le troisième.

Ce n'est pas un détail de visualisation. Un K-polyèdre est une composante **comme ensemble de K-facettes**. Ajouter une facette sans fusionner deux composantes modifie le polyèdre.

---

## 2. La fixture carrée contient déjà une naissance perdue

La nouvelle attente de la fixture cocyclique est mathématiquement correcte : pour `K=3`, au niveau `R²=100`, les quatre facettes triangulaires naissent simultanément et forment immédiatement une composante. Aucune composante n'existait juste avant, donc il n'y a effectivement **aucune fusion**.

Mais en conclure

```text
K=3 -> aucun nœud
```

n'est correct que pour une **vue merge-only**. La hiérarchie HGP contient bien un polyèdre non trivial qui naît à ce niveau :

```text
P(C) = vide,
N(C) = les quatre triangles du carré.
```

Le `ForestResult` courant est vide sur ce cas, hormis ses compteurs. Pris seul, il ne permet pas de savoir qu'une composante à quatre facettes est apparue à `R²=100`.

Une sortie appelée `merge_skeleton` pourrait honnêtement omettre cette naissance. Une sortie appelée « forêt HGP complète » ne le peut pas.

---

## 3. Fixture minimale de croissance unaire

Prendre quatre points entiers :

```text
a = ( 8,10,10),
b = (12,10,10),
z = (10,11,10),
w = (10,13,10).
```

Pour la forêt `K=2` :

```text
|a-z|² = |b-z|² = 5,
|z-w|² = 4,
|a-w|² = |b-w|² = 13.
```

Les triangles `azw` et `bzw` ont pour miniboules leurs côtés diamétraux `aw` et `bw`, donc naissent à

```text
R² = 13/4 < 4.
```

Ils sont de Gabriel dans cette fixture, et relient déjà les facettes `az` et `bz` via `zw` strictement avant le niveau 4.

Au niveau

```text
R² = |a-b|²/4 = 4,
```

le simplexe

```text
sigma = {a,b,z}
```

est l'événement q2 de profondeur un : `z` est strictement intérieur à la boule diamétrale de `ab`, tandis que `w` est extérieur. Ses facettes `az` et `bz` sont actives ; la facette `ab` naît au niveau.

Il ne se produit donc aucune fusion de composantes préexistantes :

```text
|P(C)| = 1,
N(C) = {ab}.
```

Le polyèdre existant grandit en absorbant la nouvelle facette `ab`. Le code courant n'émet aucun `ForestNode` et incrémente seulement `new_attachments`. L'identité de `ab` et la composante cible sont perdues.

Ajouter une porte permanente :

```text
q2_unary_component_growth
```

avec l'attente explicite :

```text
une transition de croissance,
un prédécesseur,
une facette née : {a,b},
niveau exact : 4.
```

---

## 4. Correction minimale recommandée

Les rôles nécessaires sont déjà calculés correctement dans `build_forest`. Il suffit de ne plus les réduire à deux compteurs.

Une ABI canonique possible est :

```cpp
struct ComponentDelta {
  Q4Level level;
  FacetKey output_component;          // identifiant canonique post-lot
  SmallVec<FacetKey> predecessors;    // racines canoniques pré-lot
  SmallVec<FacetKey> born_facets;     // facettes nées exactement dans le lot
};
```

Pour chaque racine post-lot touchée :

```text
predecessors = racines distinctes de `active || existed`,
born_facets  = rôles `attachment && !existed && !active`.
```

Émettre un `ComponentDelta` dès que

```text
predecessors.size() != 1 || !born_facets.empty().
```

On obtient alors sans ambiguïté :

```text
0 prédécesseur + facettes nées -> naissance ;
1 prédécesseur + facettes nées -> croissance ;
>=2 prédécesseurs              -> multifusion.
```

Le `ForestNode{batch, absorbed}` actuel devient une vue dérivée des seuls deltas à au moins deux prédécesseurs. Il peut rester pour les statistiques, mais ne doit plus être l'unique payload hiérarchique.

Deux détails sont indispensables :

1. le niveau exact doit être conservé dans le résultat, pas seulement son indice `batch` ;
2. l'identifiant de composante doit être déterministe, par exemple la plus petite `FacetKey` de la composante post-lot.

### Variante directement alignée sur le Théorème 5

Puisque l'objet du manuscrit est un K-MST, une autre ABI valable consiste à rendre :

```cpp
struct ForestArc {
  FacetKey u;
  FacetKey v;
  Q4Level level;
  uint64_t batch;
};

struct ForestOutput {
  std::vector<FacetKey> vertices;     // au moins F_K^render
  std::vector<ForestArc> mst_arcs;    // toutes les unions effectivement retenues
  std::vector<Q4Level> batch_levels;
};
```

Il faut alors enregistrer aussi les unions qui attachent une facette née dans le lot. La naissance du carré `K=3` est représentée par un arbre de trois arcs au niveau 100 ; la croissance unaire par l'arc qui rattache `ab` au composant existant.

Cette variante demande un tie-break canonique à l'intérieur d'un plateau si l'on veut un K-MST bit-à-bit reproductible. La représentation `ComponentDelta` est naturellement indépendante de l'ordre interne et convient mieux au dendrogramme.

---

## 5. Le juge peut fermer ce point sans nouvelle géométrie

Le juge possède déjà :

- les rôles de facettes par rayon de naissance indépendant ;
- les racines pré-lot ;
- les facettes nouvelles ;
- les composantes post-lot.

Il peut donc construire ses propres `ComponentDelta` sans réutiliser le code sujet et comparer :

```text
niveau exact,
composante canonique de sortie,
ensemble des prédécesseurs,
ensemble des facettes nées.
```

Les trois fixtures structurantes deviennent :

```text
q2_one_interior_attachment : fusion de 2 prédécesseurs + 1 facette née ;
square K=3                : naissance de 0 prédécesseur + 4 facettes nées ;
q2_unary_component_growth : croissance de 1 prédécesseur + 1 facette née.
```

Un mutant utile est :

```text
drop_nonmerge_deltas
```

Il reproduit exactement le `ForestResult` actuel : partitions finales justes, mais naissances et croissances absentes.

---

## Ordre utile

1. conserver les identités des facettes nées et leur composante cible ;
2. graver les fixtures `square K=3` comme naissance et `q2_unary_component_growth` comme croissance ;
3. seulement ensuite figer l'ABI de raccord WSPD vers la forêt ;
4. construire `F_K^conn`, `F_K^render` et les cartes verticales sur ce payload complet.

## Conclusion

Claude a correctement réparé la sémantique des **enfants** d'une fusion. Le dernier raccord est de ne pas jeter pour autant les facettes qui ne sont pas des enfants : elles sont la matière même dont les polyèdres naissent et grandissent.

La fermeture union-find et les partitions sont saines. Ce qui manque est local mais structurant : transformer le compteur `new_attachments` en transitions identifiées. Sans cela, la v4 rend un excellent squelette de fusions, pas encore la hiérarchie HGP complète annoncée par son contrat.
