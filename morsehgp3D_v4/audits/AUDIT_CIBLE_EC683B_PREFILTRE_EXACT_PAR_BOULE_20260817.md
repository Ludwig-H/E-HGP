# Audit ciblé après `ec683b6` — préfiltrer exactement la profondeur avant le census complet

Date : 17 août 2026.  
Pin audité : `ec683b679fae58e67c92bd1dc10d4f54831fec40`.

## Verdict

Les commits `7c23b54` et `ec683b6` ferment correctement les audits précédents :

- la frontière `GeometryIndex -> PointId` est au bon endroit et la porte de relabeling est substantielle ;
- `ComponentDelta` conserve naissances, croissances et multifusions ;
- `build_render` garde toutes les facettes de tous les événements et leurs multiplicités d'incidence ;
- le niveau de naissance d'une facette est bien sa miniboule exacte, jamais sa première incidence.

Je n'ai pas trouvé de contre-exemple à `facet_birth_level`. L'argument « minimum des candidates contenantes » est valide : la vraie miniboule possède un support minimal de cardinal deux, trois ou quatre ; ce support apparaît respectivement comme paire diamétrale, triangle strictement aigu ou quadruplet non coplanaire dans l'énumération. Les candidates supplémentaires qui contiennent la facette ne peuvent pas avoir un rayon inférieur à la miniboule par définition.

Le verrou suivant est donc exactement celui révélé par le reçu : à `n=400`, environ 98 % des `7,6 M` boules uniques sont rejetées pour profondeur **après** avoir payé le census complet et la collecte de coquille.

La correction utile n'est pas un nouvel index géométrique. C'est une première passe par `BallKey`, strictement moins coûteuse, qui décide seulement

```text
|I_B| >= h ?
```

avant toute matérialisation de `I_B` ou `U_B`.

---

## 1. Seuil exact par arité minimale de la boule

Après le RLE inter-lanes, soit `q_min` la plus petite arité d'un générateur de la `BallKey`. C'est la cardinalité minimale d'un support de sa miniboule :

```text
q_min in {2,3,4}.
```

Pour un événement du plateau

```text
sigma = I_B union T,
c_B in conv(T),
```

on a nécessairement

```text
|T| >= q_min.
```

Un événement utile à un ordre `K <= K_max` doit vérifier

```text
|I_B| + |T| = K+1 <= K_max+1.
```

Donc

```text
|I_B| <= K_max+1-q_min = h_qmin-1,
h_qmin = K_max+2-q_min.
```

La règle de mort exacte est ainsi :

```text
|I_B| >= h_qmin  =>  aucun événement pour K <= K_max.
```

Au profil `K_max=10` :

```text
q_min=2 : mort à 10 intérieurs,
q_min=3 : mort à  9 intérieurs,
q_min=4 : mort à  8 intérieurs.
```

Le cap uniforme actuel à neuf intérieurs est donc inutilement faible pour les boules de support minimal q3/q4. Le premier candidat du groupe RLE porte déjà `q_min`, puisque `ball_candidate_less` trie l'arité avant la représentation ; il faut rendre ce champ explicite sous le nom `min_support_arity` et le conserver après déduplication.

---

## 2. Première passe exacte : compter sans collecter

Pour la forme primitive

```text
P_B(z) = A*|z|^2 + B*z + C,
```

le code possède déjà les extrema entiers exacts sur la boîte serrée d'un nœud :

```text
mn = min P_B sur l'AABB entière,
mx = max P_B sur l'AABB entière.
```

La passe de profondeur seulement utilise les décisions :

```text
mn >= 0  =>  aucun point du nœud n'est strictement intérieur : NONE,
mx <  0  =>  tous les points du nœud sont strictement intérieurs : range-add,
sinon    =>  scission ; à la feuille, test exact.
```

Les inégalités sont importantes :

- `mn >= 0` peut élaguer les coquilles, puisqu'elles ne comptent pas dans la profondeur ;
- `mx < 0` doit rester strict, sinon une coquille serait comptée intérieure.

Pseudo-code :

```cpp
bool depth_at_least(const CloudIndex& ix,
                    const Q3BallKey& key,
                    uint64_t h,
                    uint64_t* exact_or_saturated_count) {
  count = 0;
  stack = {root};
  while (!stack.empty()) {
    node = pop(stack);
    [mn,mx] = exact_integer_extrema(key, tight_box(node));
    if (mn >= 0) continue;
    if (mx < 0) {
      count = min(h, count + weight(node));
      if (count == h) return true;
      continue;
    }
    if (leaf(node)) {
      if (P_B(point(node)) < 0 && ++count == h) return true;
    } else {
      push(children(node));
    }
  }
  *exact_or_saturated_count = count;
  return false;
}
```

Cette passe :

- ne collecte aucun ID ;
- ne descend pas les régions de coquille pure ;
- range-add les blocs entièrement intérieurs ;
- s'arrête dès `h_qmin` ;
- rend le compte exact pour toute boule survivante.

Le census complet actuel devient alors la seconde passe, exécutée seulement si

```text
depth < h_qmin.
```

Il collecte les `depth` IDs intérieurs et toute la coquille, puis vérifie que la taille collectée vaut le compte de la première passe.

---

## 3. Ordre de pipeline conseillé

```text
candidats q2/q3/q4
 -> tri par BallKey, arité, représentation
 -> scan RLE : BallKey + min_support_arity
 -> passe count-only saturée à h_qmin
      -> morte : jeter immédiatement
      -> survivante : mémoriser depth exact
 -> census complet I_B/U_B seulement sur les survivantes
 -> expansion SpherePlateau
 -> forêt + rendu.
```

La seconde passe ne concerne alors, d'après le reçu actuel, qu'environ 2 % des clés. Même si la passe count-only visite encore l'arbre, ses décisions `mn>=0` et `mx<0` sont beaucoup plus fortes que celles du census de coquille, qui doit descendre tout nœud dont `mn=0`.

Mesures à publier séparément :

```text
prefilter_keys,
prefilter_dead,
prefilter_nodes,
prefilter_range_add_mass,
prefilter_leaf_tests,
full_census_keys,
full_census_nodes,
full_shell_ids.
```

Le temps du RLE, du count-only et du census complet doit rester séparé ; sinon une baisse spectaculaire du nombre de census pourra encore dissimuler un tri qui mange le gain, petite spécialité déjà observée avec la sélection axiale CPU.

---

## 4. Portes et mutants vraiment discriminants

### 4.1 Bord de profondeur

Réutiliser les fixtures de profondeur `h_q-1` déjà gravées :

```text
q2 : profondeur 9,
q3 : profondeur 8,
q4 : profondeur 7.
```

Elles doivent survivre. Un mutant

```text
threshold-minus-one
```

qui tue à `h_qmin-1` doit perdre l'événement.

### 4.2 Coquille non intérieure

La fixture carrée et les grandes cosphères doivent survivre au count-only selon leur profondeur réelle. Un mutant

```text
range-add-max-le-zero
```

qui remplace `mx<0` par `mx<=0` compte les coquilles comme intérieures et doit mourir.

### 4.3 Mort exacte

Ajouter une boule de chaque arité minimale avec exactement `h_qmin` points strictement intérieurs et aucun doute de coquille. La passe doit la tuer sans appeler le census complet.

### 4.4 Équivalence bout en bout

Sur les deux juges `n=120` :

```text
événements,
ComponentDelta,
RenderResult,
partitions finales
```

doivent être identiques avec et sans préfiltre. Le mutant `skip-full-census-after-survival` doit rappeler que le count-only ne connaît pas `U_B` et ne remplace jamais la seconde passe.

---

## 5. Un garde d'entrée à fermer avant l'échelle

La nouvelle API `InputPoint{id,position}` transporte enfin les vrais IDs, mais `build_cloud_index` ne préflight pas encore :

```text
unicité des PointId,
0 <= x,y,z <= 65535.
```

Ce garde est nécessaire avant les campagnes d'échelle :

- deux positions distinctes ayant le même ID rendent les `FacetKey` non injectives ;
- `morton_spread3` masque silencieusement à seize bits. Une coordonnée négative ou `>=65536` peut donc être placée dans une cellule qui ne la contient pas, ce qui invalide les élagages.

Ces cas doivent rendre `invalid_input` avant la construction de l'arbre. Deux petites portes `duplicate-pointid` et `coordinate-out-of-u16` suffisent ; inutile de les laisser devenir un rapport scientifique en trois actes.

---

## Conclusion

Le rendu symbolique de `ec683b6` est mathématiquement sain, y compris la miniboule exacte des facettes et les multiplicités des plateaux. Il n'y a aucune raison de rouvrir cette partie.

Le prochain gain doit être obtenu en exploitant une asymétrie très favorable : décider qu'une boule est trop profonde demande seulement un compteur intérieur strict, tandis que la coquille complète n'est nécessaire que pour les rares survivantes. Les extrema entiers et le range-add existent déjà ; il suffit enfin de les utiliser avant de collectionner plusieurs centaines de milliers de points sur des sphères destinées à la poubelle.
