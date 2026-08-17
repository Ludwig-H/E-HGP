# Audit bloquant après `e7e4d5` — le flux réel remplace encore les `PointId` par les indices denses

Date : 17 août 2026.  
Pin de code audité : `e7e4d5e58dbc8d0c1b57137b1eba2a9706029328`.  
HEAD documentaire au moment de la note : `ae9383b45afecdd4ba7058eadeb06f6bc2512f82`.

## Verdict

Le raccord

```text
WSPD -> BallKey RLE inter-lanes -> census I_B/U_B -> SpherePlateau -> forêt
```

est la bonne architecture. Le théorème de complétude sous les seuils `h_q`, le census uniforme par forme primitive et le quotient par boule sont cohérents.

Le chemin réel possède toutefois un défaut bloquant d'identité : les `ForestEvent` sont construits avec les **indices de positions uniques dans l'ordre spatial interne**, castés en `PointId`.

Le juge borné réutilise le même raccord et ne peut donc pas voir la faute.

---

## 1. Ligne fautive

Dans `forests_from_balls`, `PlateauEvent::tpart` et `ipart` contiennent les indices `i32` renvoyés par le census dans `ix.upos` :

```cpp
ev.support[t] = (PointId)pe.tpart[t];
ev.interior[t] = (PointId)pe.ipart[t];
```

Or ces entiers ne sont pas des identités externes. Ce sont les indices denses des positions uniques, ordonnées spatialement par le `CloudIndex`.

Les probes q3/q4 précédents faisaient justement le raccord correct :

```cpp
pid_of[u] = ix.bucket_ids[ix.bucket_start[u]];
```

puis construisaient `SupportKey`, `EdgeKey` et `InteriorIds` avec `pid_of[u]`.

Le nouveau fold perd ce raccord au dernier moment.

---

## 2. Conséquence mathématique

Une simple permutation des records d'entrée, à positions et vrais IDs inchangés, peut modifier :

```text
FacetKey,
ordre canonique des facettes,
partition finale sérialisée,
identité des enfants du dendrogramme,
cartes verticales futures,
Delta F et rendu par facette.
```

La géométrie et les `BallKey` restent identiques, mais la forêt change de noms parce qu'elle reçoit l'ordre interne comme identité.

Cela viole le contrat fondamental :

```text
PointId != SiteIndex != rang Morton.
```

Le défaut devient encore plus visible avec des IDs clairsemés ou supérieurs au bit 31.

---

## 3. Pourquoi le juge 0/0 ne le détecte pas

Le juge de `forest_probe` :

1. travaille lui aussi sur `ix.upos` ;
2. stocke ses census en indices denses ;
3. appelle le même `forests_from_balls` ;
4. compare donc deux forêts dans le même mauvais espace d'identités.

Il juge correctement :

- la complétude géométrique WSPD ;
- le RLE de boules ;
- le census d'arbre ;
- les partitions abstraites dans l'espace dense.

Il ne juge pas la conservation des `PointId` externes. Deux chemins partageant le même renommage illégitime peuvent encore afficher `0/0`, prouesse statistique que les tests logiciels accomplissent avec une régularité admirable.

---

## 4. Correction minimale

Conserver les indices denses pour la géométrie est correct. La conversion doit se faire exactement à la frontière événement -> forêt :

```cpp
int forests_from_balls(
    ...,
    const std::vector<PointId>& pid_of,
    ...)
```

puis

```cpp
ev.support[t]  = pid_of[(size_t)pe.tpart[t]];
ev.interior[t] = pid_of[(size_t)pe.ipart[t]];
```

Construire une fois :

```cpp
std::vector<PointId> pid_of(ix.unique_count());
for (size_t u = 0; u < pid_of.size(); ++u)
  pid_of[u] = ix.bucket_ids[ix.bucket_start[u]];
```

sous le profil actuel de positions distinctes.

Je recommande aussi de rendre l'erreur de type plus difficile :

```cpp
struct SiteIndex { i32 value; };
using PointId = u32;
```

et de nommer explicitement les champs bornés :

```cpp
PlateauEvent {
  vector<SiteIndex> shell_sites;
  vector<SiteIndex> interior_sites;
};
```

Un cast silencieux `SiteIndex -> PointId` ne doit pas compiler.

---

## 5. L'API d'entrée doit enfin porter les vrais IDs

Le générateur de benchmark peut continuer à créer des IDs artificiels. Le chemin de bibliothèque doit recevoir :

```cpp
struct InputPoint {
  PointId id;
  P3 position;
};
```

avec :

- unicité des `PointId` ;
- positions u16 valides ;
- tri spatial qui déplace les records sans réécrire `id` ;
- `bucket_ids` qui conserve ces identités.

Sans cette API, la porte de permutation ne peut tester qu'un renommage fabriqué par le probe lui-même.

---

## 6. Portes permanentes

### 6.1 Permutation à IDs conservés

Créer un petit nuage contenant des événements q2/q3/q4 et des plateaux, avec des IDs explicites non consécutifs, par exemple :

```text
3, 17, 9001, 2^31+5, 2^32-2, ...
```

Exécuter deux ordres physiques différents des mêmes records. Exiger, après tri canonique :

```text
mêmes BallKeys,
mêmes ForestEvents en vrais PointId,
mêmes FacetKeys,
mêmes partitions finales,
mêmes mises à jour de composantes.
```

### 6.2 Mutant

Ajouter :

```text
dense-index-as-pointid
```

qui rétablit les deux casts actuels. La permutation doit le tuer.

### 6.3 Juge

Le juge reçoit lui aussi les `InputPoint{id,position}` et construit ses simplexes/facettes avec `id`, jamais avec le rang de boucle. L'ordre de boucle reste un index d'accès seulement.

---

## 7. Ordre utile

1. Corriger cette frontière d'identité et graver la porte métamorphique.
2. Étendre ensuite la forêt conformément à `AUDIT_CIBLE_5A08AB_NAISSANCES_CROISSANCES_ET_DELTAS` : naissances, croissances et facettes nées par composante.
3. Ouvrir seulement après le rendu § 9.1 et les cartes verticales.
4. Revenir ensuite au préfiltre de profondeur, puisque les 7,6 millions de boules à `n=400` rendent la question de coût assez peu subtile.

## Conclusion

Le raccord WSPD vers `SpherePlateau` est géométriquement convaincant. Mais la première forêt dite réelle n'est pas encore une forêt sur les vrais points : elle est une forêt sur les rangs spatiaux internes.

La correction est locale et doit précéder toute sérialisation. Une identité stable perdue à la frontière du fold se propage ensuite partout avec une efficacité que les optimisations GPU elles-mêmes auraient du mal à égaler.
