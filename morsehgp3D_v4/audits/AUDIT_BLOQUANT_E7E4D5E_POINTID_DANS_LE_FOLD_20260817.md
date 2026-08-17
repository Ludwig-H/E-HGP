# Audit bloquant après `e7e4d5e` — le fold réel remplace encore les `PointId` par les rangs géométriques

Date : 17 août 2026.  
Pin de pipeline audité : `e7e4d5e58dbc8d0c1b57137b1eba2a9706029328`.  
Pins de forêt inclus : `052fed427b75bab7356da796544977a7a905d3e8`, `5a08ab682b53c13fcea1aee4ccd0dad2fe928644`.  
Audits parallèles pris en compte : `ae9383b45afecdd4ba7058eadeb06f6bc2512f82` et `f4abad00da46385b652bac84590c948403061084` sur les naissances/croissances.

## Verdict

Le raccord général est bon et je le reçois mathématiquement :

```text
WSPD q2/q3/q4 -> BallKey -> RLE inter-lanes -> un census I_B/U_B par boule
-> SpherePlateau -> événements par K -> macro-lots.
```

Le lemme de complétude sous les seuils `h_q` est correct : si un plateau contient un simplexe d'au plus `K_max+1` points et si sa miniboule a un support minimal d'arité `q`, alors `|I_B| <= K_max+1-q = h_q-1`. Les témoins universels du fuseau de l'ancre sont intérieurs à cette boule ; l'ancre du support minimal ne peut donc pas être tuée. Le census uniforme par la forme primitive `(A,B,C)` est également la bonne factorisation.

Il subsiste cependant une faute d'identité dans le chemin désormais présenté comme réel. Dans `forests_from_balls` :

```cpp
ev.support[t]  = (PointId)pe.tpart[t];
ev.interior[t] = (PointId)pe.ipart[t];
```

Or `pe.tpart` et `pe.ipart` contiennent des **indices de positions uniques dans `ix.upos`**, donc des rangs du layout Morton. Ce ne sont pas les `PointId` externes.

Le nouveau pipeline calcule ainsi la bonne géométrie, mais publie la forêt sur les mauvaises identités.

---

## 1. Pourquoi le problème est bloquant

Le contrat v4 distingue explicitement :

```text
PointId stable != index dense != rang Morton.
```

Les probes q2/q3/q4 précédents respectaient ce contrat avec :

```cpp
pid_of[u] = ix.bucket_ids[ix.bucket_start[u]];
```

Le raccord de forêt perd cette conversion. En conséquence :

- les `FacetKey` contiennent des rangs Morton ;
- les partitions finales sont indexées par le layout interne ;
- les futurs enfants de dendrogramme, `F_K^render`, votes et cartes verticales ne désignent plus les points d'entrée ;
- une permutation physique de l'entrée ou une autre construction équivalente de l'index peut changer toutes les clés publiques ;
- des IDs valides au-dessus du bit 31 disparaissent silencieusement au profit de petits entiers denses.

Ce n'est pas un défaut d'affichage. Les clés de facettes sont l'identité combinatoire de la forêt.

---

## 2. Pourquoi le juge `forest_probe` reste vert

Le juge borné travaille lui aussi sur `ix.upos` et construit son plateau avec les mêmes indices de positions uniques. Il finit donc par caster les mêmes rangs en `PointId`.

Il juge correctement :

- la complétude WSPD des **boules** ;
- le RLE ;
- le census géométrique ;
- les partitions dans le domaine des rangs Morton.

Il ne juge pas le raccord vers les identités externes. Deux chemins qui renomment pareillement les sommets peuvent être en accord parfait tout en violant l'ABI publique, ce talent collectif demeurant malheureusement très accessible aux logiciels.

---

## 3. Correction minimale

Conserver deux domaines de types explicites :

```cpp
using GeometryIndex = i32;  // index dans upos / arbre / census
using PointId       = u32;  // identité publique stable
```

Ajouter un accès unique :

```cpp
PointId CloudIndex::point_id(GeometryIndex u) const {
    return bucket_ids[bucket_start[(size_t)u]];
}
```

Le census et `expand_plateau` restent naturellement en `GeometryIndex`. La conversion n'a lieu qu'à la frontière combinatoire :

```cpp
for (size_t t = 0; t < pe.tpart.size(); ++t)
    ev.support[t] = ix.point_id(pe.tpart[t]);
for (size_t t = 0; t < pe.ipart.size(); ++t)
    ev.interior[t] = ix.point_id(pe.ipart[t]);
```

L'ordre du tableau `support` peut rester celui de `T` pour conserver l'alignement avec `active_mask`; `facet_minus` trie déjà la `FacetKey` produite. Il ne faut donc pas retrier `support` indépendamment du masque.

Le refus actuel des positions dupliquées rend la conversion univoque. Une future sémantique des doublons devra revoir le plateau étiqueté ; elle ne justifie pas de publier aujourd'hui le rang du bucket comme identité.

### API d'entrée

La bibliothèque réelle doit recevoir :

```cpp
struct InputPoint {
    PointId id;
    P3 position;
};
```

Le probe `vector<P3>` peut fabriquer `id=index_entree` comme commodité de test, mais le noyau ne doit pas en déduire que l'ordre physique est l'identité.

---

## 4. Porte permanente

Construire un nuage régulier petit avec des IDs externes volontairement non monotones, par exemple :

```text
positions : géométrie fixe
IDs       : {4000000000, 17, 3000000001, 2, 99991, ...}
```

Exécuter deux entrées qui ne diffèrent que par une permutation physique des enregistrements `{id,position}`.

Exiger, après tri canonique :

```text
mêmes BallKeys,
mêmes événements de plateau exprimés en PointId,
mêmes FacetKeys,
mêmes partitions et mises à jour de composantes,
aucune clé publique hors de l'ensemble des IDs fournis.
```

Le mutant :

```text
forest-use-geometry-index-as-pointid
```

doit reproduire le cast courant et mourir.

Ajouter une fixture avec égalités d'arêtes dont l'owner dépend de `EdgeKey` externe. Elle vérifie conjointement que :

1. le générateur choisit avec les vrais IDs ;
2. le plateau et la forêt conservent ensuite ces mêmes IDs.

Le juge doit construire sa vérité depuis les `InputPoint.id`, par une table indépendante `geometry_index -> external_id`, et non appeler la conversion du sujet.

---

## 5. Raccord avec les audits de sortie

Les audits `ae9383b` et `f4abad0` demandent justement des identités persistantes de composantes, des enfants et des facettes nées. Cette extension doit être faite **après** la correction présente : un `ComponentDelta` très complet mais rempli de rangs Morton resterait une excellente description du layout mémoire, objet pour lequel le manuscrit manifeste assez peu d'intérêt.

Ordre utile :

1. séparer `GeometryIndex` et `PointId` dans le raccord réel ;
2. graver la permutation d'IDs externes ;
3. implémenter les naissances/croissances et les identités parent-enfant ;
4. seulement ensuite figer `F_K^render`, les multiplicités d'incidence et les cartes verticales.

## Conclusion

La géométrie du nouveau flux réel est prometteuse et les mesures désignent clairement les prochains postes de coût. La faute trouvée est locale, mais doit être corrigée immédiatement : la frontière entre le census géométrique et la forêt combinatoire est précisément l'endroit où un index interne doit redevenir un `PointId`.

Après cette conversion et sa porte de permutation, le raccord WSPD pourra être reçu comme un vrai chemin public plutôt que comme une forêt exacte sur un renommage accidentel du nuage.
