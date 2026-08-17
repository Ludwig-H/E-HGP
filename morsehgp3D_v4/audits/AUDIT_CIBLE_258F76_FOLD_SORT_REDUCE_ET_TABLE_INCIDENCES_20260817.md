# Audit ciblé après `258f76` — unifier forêt et rendu par une table d’incidences `sort/reduce`

Date : 17 août 2026.  
Pins de code reçus : `c1bfaf8a8a00af93b30a9d671f546f37b94fcf0f`, `06795db2b099d146409aca0991060cfb992260e5`.  
Pin de mesure : `258f76bdf35b0929d41331c613f26862361cfcf0`.

## Verdict

Les filtres récents sont mathématiquement reçus :

- le scan strict du cover par boule est un minorant de profondeur sûr ;
- le compte exact de `W_4(a,b)` tue correctement une ancre entière avant `seed × complétion` ;
- les sorties jugées restent inchangées ;
- les pentes `400/800/1600` montrent que le nombre de boules publiées et d’événements est désormais raisonnable.

La mesure désigne maintenant un verrou d’implémentation indépendant de la géométrie :

```text
t_fold = 1,76 s, 4,52 s, 12,81 s
```

et la pente la plus forte du tableau vient du fold fondé sur plusieurs `std::map`.

Je conseille de ne pas remplacer ces maps une par une. La forêt, les `ComponentDelta` et `F_K^render` consomment tous la **même relation incidence événement–facette**. Une table commune, internée par tri puis traitée par réductions segmentées, ferme les trois objets à la fois et correspond directement aux primitives GPU visées.

---

## 1. L’identité qui rend le fold statique

Fixons un ordre `K` et les événements déjà triés par niveau exact. On leur attribue d’abord les `BatchId` par :

```text
same_exact_level(e_i.level,e_j.level).
```

Pour chaque événement et chacune de ses `K+1` facettes, émettre une occurrence :

```cpp
struct FacetOccurrence {
  FacetKey key;
  EventId event;
  BatchId batch;
  uint8_t role;   // ACTIVE ou ATTACHMENT
  uint8_t slot;   // position dans l’événement
};
```

Après tri/unique par `FacetKey`, chaque facette reçoit un `FacetId` dense.

Définir ensuite :

```text
first_touch[f] = min batch des occurrences de f.
```

Alors, dans le fold courant :

```text
f.existed_before_batch(b)  <=>  first_touch[f] < b.
```

**Preuve.** Le code insère toutes les facettes touchées par un lot à la fin de la phase de rôles de ce lot. Une facette existe donc dans `id_of` avant `b` exactement si elle a été touchée dans un lot strictement antérieur. CQFD.

Cette identité supprime le seul état qui semblait imposer une map dynamique pour les rôles.

Le critère pré-lot devient exactement :

```text
prebatch(f,b) = active_in_batch(f,b) || first_touch[f] < b.
```

Et les deux invariants actuels deviennent des réductions statiques :

```text
attach_in_batch && first_touch < batch  -> attach_violation,
attach_in_batch && active_in_batch      -> birth_violation.
```

Une facette née dans le lot est :

```text
born(f,b) = attach_in_batch && first_touch[f] == b && !active_in_batch.
```

C’est exactement la sémantique actuellement reçue, sans dictionnaire ordonné.

---

## 2. Phase A — interner une seule fois les facettes

Pour chaque `K` séparément :

1. trier les événements par `compare_exact_level` et graver les `BatchId` ;
2. émettre les `M_K=(K+1)|E_K|` occurrences ;
3. trier les occurrences lexicographiquement par `FacetKey` ;
4. marquer les ruptures de clé, faire une somme préfixe et attribuer les `FacetId` ;
5. diffuser le `FacetId` dans les `K+1` slots de chaque événement ;
6. calculer `first_touch[f]` par minimum segmenté.

À `K` fixé, une `FacetKey` a exactement `K` mots u32 : le tri peut être un radix multi-passes sur SoA. La référence CPU peut commencer par `std::sort` ; l’ABI est déjà celle de CUB/Thrust.

Il faut traiter les ordres `K` l’un après l’autre. La mémoire est alors :

```text
O((K+1)|E_K| + |F_K|),
```

jamais la somme simultanée des dix ordres.

---

## 3. Phase B — agréger les rôles par `(batch,facet_id)`

Réordonner une vue compacte des occurrences par :

```text
(batch, facet_id).
```

Une réduction OR par segment fournit :

```cpp
struct BatchFacet {
  BatchId batch;
  FacetId facet;
  bool active;
  bool attach;
};
```

Il n’y a plus de `std::map<FacetKey,Role>` par macro-lot.

Les événements conservent parallèlement leurs tableaux plats :

```text
event_facets[event][0..K].
```

Toutes les opérations géométriques sont terminées ; le fold ne manipule plus que des entiers denses et des bornes de segments.

---

## 4. Phase C — union-find par lot, réductions pour les deltas

La dépendance entre niveaux impose naturellement l’ordre séquentiel des macro-lots. Rien n’impose en revanche des maps à l’intérieur d’un lot.

Pour un lot `b` :

### 4.1 Geler les parents pré-lot

Pour chaque `BatchFacet` du lot vérifiant :

```text
active || first_touch[facet] < b,
```

calculer avant toute union :

```text
pre_root = find(facet),
pre_canon = canon[pre_root].
```

Trier/unique les `pre_root` si plusieurs facettes touchent déjà la même composante.

### 4.2 Fermer les événements du lot

Pour chaque événement, unir ses `K+1` `FacetId` en étoile. Maintenir :

```text
canon[new_root] = min(canon[root_a],canon[root_b]),
```

comme aujourd’hui. L’ordre interne des unions ne change ni la partition ni le minimum canonique.

### 4.3 Construire les `ComponentDelta`

Après les unions :

- pour chaque racine pré-lot gelée, émettre `(post_root,pre_canon)` ;
- pour chaque facette `born`, émettre `(post_root,facet_id)`.

Deux tris/réductions segmentées par `post_root` donnent :

```text
parents(post_root) = pre_canons distincts,
born(post_root)    = facettes nées triées.
```

Fusionner les deux flux segmentés et émettre le delta lorsque :

```text
parents.size()!=1 || !born.empty().
```

Le `ForestNode` reste la vue dérivée `parents.size()>=2`.

Cette procédure reproduit champ par champ :

```text
output,
parents,
born,
new_attachments,
attach_violations,
birth_violations,
nodes.
```

### 4.4 Partition finale

À la fin :

```text
root[f] = find(f),
component_key[f] = canon[root[f]].
```

Un tri par `FacetId` suffit à sérialiser la partition. La map publique peut rester une vue de test ; elle ne doit plus être la structure de calcul.

---

## 5. Le même tableau ferme `F_K^render`

`build_render` reconstruit aujourd’hui une seconde structure :

```text
map<FacetKey,map<BatchId,multiplicité>>.
```

Or chaque `FacetOccurrence` est précisément une incidence d’un K-simplexe sur une facette.

Sur la table déjà internée :

1. trier les occurrences par `(facet_id,batch)` ;
2. compter la longueur de chaque segment ;
3. publier directement :

   ```text
   FacetIncidences{facet_id, [(batch,multiplicity),...]}
   ```

4. `F_K^render` est l’ensemble des `FacetId` uniques ;
5. `facet_birth_level` est calculé **une seule fois par facette unique**, jamais par occurrence.

Ainsi une seule matérialisation sert :

```text
connectivité,
ComponentDelta,
rendu,
multiplicités,
table des naissances de facettes.
```

Réécrire séparément `build_forest` puis `build_render` ferait payer deux tris et deux interning différents, vieille habitude humaine consistant à dupliquer une donnée parce que deux fonctions lui donnent des noms distincts.

---

## 6. Réception mathématique de l’équivalence

La version sort/reduce doit être reçue contre la version map, pas simplement contre les partitions finales.

Comparer pour chaque `K` :

```text
batch_levels par égalité sémantique,
batch_of_event,
multiensemble des ForestNode,
ComponentDelta complets (output,parents,born,niveau),
new_attachments,
violations,
final_partition par blocs,
F_K^render,
multiplicités par (facette,lot),
naissance exacte de chaque facette.
```

Portes indispensables :

1. carré cosphérique `K=1/2/3` ;
2. `q2_one_interior_attachment` ;
3. croissance unaire ;
4. tie q2/q4 à représentations de niveau différentes ;
5. ordre aléatoire des événements **à l’intérieur** de chaque macro-lot ;
6. relabeling des `PointId` et permutation physique ;
7. nuages jugés `n<=120` sur `uniform` et `eight_clusters`.

Mutants utiles :

```text
first-touch-le-batch        // compte les attachements nés au lot comme préexistants
no-unique-pre-roots         // gonfle parents/absorbed
reduce-active-by-last       // remplace OR par le dernier rôle rencontré
drop-born-segment           // perd les croissances sans fusion
render-unique-per-batch     // écrase la multiplicité à 1
```

Le premier doit mourir sur `q2_one_interior_attachment`, le troisième sur un plateau où une facette est rencontrée plusieurs fois dans le même lot, le quatrième sur la croissance unaire, le cinquième sur le carré `K=2`.

---

## 7. Mesures à publier

Séparer au minimum :

```text
occurrences,
facettes_uniques,
t_intern_sort,
t_role_sort_reduce,
t_union_batches,
t_delta_sort_reduce,
t_render_reduce,
peak_bytes,
max_occurrences_in_batch,
max_unique_facets_in_batch.
```

Conserver pendant une phase la référence map derrière :

```text
--fold=map | sort
```

La référence map peut ensuite rester limitée aux portes petites, comme l’oracle de miniboule.

---

## Ordre utile

1. Conserver les filtres q4 actuels et les nouvelles pentes comme baseline.
2. Introduire `FacetOccurrenceTable` et recevoir l’interning par tri.
3. Brancher le fold sort/reduce en double calcul contre la référence map.
4. Brancher le rendu sur la même table.
5. Ensuite seulement supprimer les maps du chemin d’échelle.
6. Poursuivre en parallèle la sélection axiale bornée, qui attaque l’autre poste dominant.

Les deux chantiers sont orthogonaux : l’axial réduit les formes q4 évaluées ; la table d’incidences réduit le coût par événement réellement conservé. Les pentes montrent qu’il faudra les deux pour approcher l’échelle visée.

## Conclusion

Le verrou mathématique de la forêt est désormais fermé ; le verrou actuel est de ne pas recalculer par arbres rouges-noirs une relation d’incidence déjà entièrement connue.

L’identité

```text
existed_before_batch <=> first_touch < batch
```

permet une réécriture exacte, déterministe, sans hash et directement transposable sur GPU. C’est le prochain gain structurel du fold, pas une micro-optimisation de conteneur.
