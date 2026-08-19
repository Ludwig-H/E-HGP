# Réponse ciblée après `1d6fd06` — fold plat, portée du préflight et première porte CUDA

Date : 18 août 2026.
Pins de code audités :

- parallélisation et cœur de seed : `9549659`, `5281a20`, `2383874` ;
- fold `sort/reduce` et profil : `2b7bb32`, `b21cd6b` ;
- préflight et plafond : `699c894`, `1d6fd06` ;
- compatibilité device : `57523a9` ;
- reçu d’échelle local : `0f170f1`.

Question traitée : `QUESTION_CLAUDE_INTERNES_DU_FOLD_20260817.md`.

## Verdict

Les progrès récents sont reçus positivement.

- La parallélisation par rectangles puis par tranches aval préserve l’objet après RLE et respecte la séparation des états par ouvrier.
- Le cœur de seed aplati est mathématiquement sûr et son choix contre la descente d’arbre est correctement décidé par la mesure, non par préférence architecturale.
- Le fold `sort/reduce` conserve la sémantique reçue des macro-lots, naissances, croissances, multifusions et multiplicités.
- Le compte Poisson q2 et l’injection des facettes nées sont correctement transformés en portes bornées.
- `max_output_bytes` refuse bien avant la construction de `ev_k`.
- La campagne locale v2 montre que la cellule historiquement bloquée `eight_clusters,n=8000,smax=11` termine désormais, sans changement de cardinalités observables.

Je ne vois aucune nouvelle faute géométrique. Les trois remarques suivantes sont des raccords d’architecture concrets, pas une invitation à rouvrir les preuves déjà fermées.

---

## 1. Réponse à la question du fold : aucune raison de conserver `std::map` pour `final_partition`

`final_partition` est construite **après le dernier macro-lot**, lorsque :

- l’ensemble des facettes est définitif ;
- les `FacetKey` sont déjà internées dans l’ordre strictement croissant ;
- aucune insertion tardive n’existe ;
- les consommateurs demandent seulement une itération ordonnée, une égalité de contenu ou une consultation par clé.

Mathématiquement, la partition finale est le graphe fini de la fonction

```text
FacetKey -> canonique de sa composante.
```

La représentation canonique naturelle est donc un vecteur trié, pas un arbre de recherche alloué nœud par nœud.

### ABI recommandée

```cpp
struct PartitionEntry {
    FacetKey facet;
    FacetKey component;
};

struct FlatPartition {
    std::vector<PartitionEntry> entries; // strictement croissantes en facet

    const FacetKey* find(const FacetKey& f) const;
};
```

`find` est un `lower_bound`. L’itération possède exactement le même ordre observable que la `std::map` actuelle. Le remplissage est linéaire :

```cpp
entries.reserve(nfid);
for (u64 fid = 0; fid < nfid; ++fid)
    entries.push_back({keys[fid], canon_of[uf.find(fid)]});
```

Il ne faut surtout pas reconstruire ensuite une `std::map` « pour l’ABI » dans le chemin d’échelle : cela réintroduirait tout le coût au dernier moment. Les petits juges peuvent disposer d’un adaptateur vers une map si un ancien test l’exige encore.

### Portes suffisantes

Pour chaque fixture déjà reçue, comparer l’ancienne map et le vecteur comme suites ordonnées de paires. Ajouter les invariants :

```text
size == facets,
clés strictement croissantes,
aucune clé dupliquée,
lookup de chaque clé == valeur de référence,
lookup de clés absentes == absent.
```

Les fixtures réellement utiles sont déjà présentes :

- naissance cosphérique ;
- croissance unaire ;
- multifusion avec attachement né ;
- égalité q2/q4 de niveau à représentations différentes ;
- permutation physique et relabeling des `PointId`.

Le mutant proposé `partition-vector-desordonnee` est utile, mais je recommande d’ajouter un mutant plus sémantique :

```text
partition-stale-root
```

qui écrit `canon_of[fid]` au lieu de `canon_of[find(fid)]`. Une chaîne de fusions sur plusieurs lots doit le tuer. Il vérifie la vraie frontière union-find/partition, tandis qu’un simple échange de deux cases teste surtout que `lower_bound` aime les données triées, révélation moins bouleversante.

### Chronométrage permanent

Séparer au minimum :

```text
t_batch_reduce,
t_delta_materialize,
t_final_partition.
```

Le profil actuel regroupe encore environ quarante secondes sous « réduction + partition ». Le remplacement de `final_partition` doit être livré seul d’abord ; il donnera enfin la part attribuable aux deltas.

---

## 2. Les tampons réutilisés ne supprimeront pas à eux seuls les allocations finales des `ComponentDelta`

La proposition de Claude est saine comme première étape, mais il faut distinguer :

```text
scratch de construction réutilisé
```

et

```text
propriété finale de deux std::vector par delta.
```

Même si `parents` et `born` sont assemblés dans des tampons réutilisés, chaque `ComponentDelta` matérialisé finit encore par posséder ses propres allocations. Si le nouveau chrono montre que ce poste reste dominant, la bonne représentation d’échelle est plate :

```cpp
struct DeltaHeader {
    u64 batch;
    Q4Level level;
    FacetKey output;
    u64 parent_begin;
    u32 parent_count;
    u64 born_begin;
    u32 born_count;
};

struct FlatDeltas {
    std::vector<DeltaHeader> headers;
    std::vector<FacetKey> parents;
    std::vector<FacetKey> born;
};
```

Un `ComponentDeltaView` restitue l’ABI sémantique aux consommateurs. Cette forme est :

- allocation-libre par delta ;
- sérialisable et segmentable ;
- compatible avec un flux externe ;
- directement transposable en offsets GPU.

Je déconseille un `SmallVector<N>` comme contrat définitif : le nombre de parents d’une multifusion et le nombre de facettes nées ne possèdent pas de petite borne universelle. Il peut servir de transition CPU, pas d’ABI d’échelle.

### Agrégation sans `std::map<i32, ComponentDelta>`

Si ce sous-poste est confirmé, émettre dans le lot deux tableaux plats :

```text
ParentRec{post_root, pre_canon},
BornRec{post_root, facet}.
```

Puis trier/réduire par `post_root`, ou employer un tableau à époque indexé par racine. Les segments obtenus sont ajoutés directement dans `FlatDeltas`.

### Réception

Comparer champ par champ les vues plates à l’ancien `ComponentDelta` sur :

```text
0 parent + born       (naissance),
1 parent + born       (croissance),
>=2 parents           (multifusion),
parents sans born,
plateau avec plusieurs facettes nées,
deux composantes distinctes touchées dans le même lot.
```

Mutants utiles :

```text
delta-parent-offset-plus-one,
delta-born-count-minus-one,
delta-coalesce-post-roots.
```

Les mutants sémantiques existants `drop-nonmerge`, `attach-prebatch` et les portes de plateau doivent rester ; ils jugent l’objet, tandis que les nouveaux jugent le layout.

---

## 3. `--output-preflight-only` est exact, mais c’est un préflight **tardif des événements**

Le mode actuel possède une valeur réelle : il compte exactement les `ForestEvent` et les incidences sans construire `ev_k` ni lancer le fold. `max_output_bytes` protège donc correctement l’allocation de ces vecteurs.

Il ne faut cependant pas lui attribuer un contrat plus large. Avant d’entrer dans le bloc de préflight, le programme a déjà matérialisé :

```text
tous les BallCandidate après génération,
le RLE complet,
toutes les survivantes,
tous les BallData,
I_B et U_B sous forme de std::vector par boule.
```

Ainsi, un cas 30M peut manquer de mémoire **avant que le préflight ne rende son verdict**. Le plafond actuel protège :

```text
allocation de ForestEvent
```

mais pas :

```text
candidats,
BallData,
incidences FRec,
union-find,
partition finale,
deltas,
rendu.
```

Deux contrats propres sont possibles :

1. renommer/documenter l’état courant comme `event_preflight_after_census` et `max_event_bytes` ;
2. conserver les noms publics, mais ajouter explicitement :

   ```text
   max_work_bytes,
   max_product_bytes,
   preflight_stage=after_census.
   ```

Pour rendre le préflight protecteur à 30M, la route exacte est celle déjà suggérée par les audits de sortie :

```text
génération par partitions
  -> runs triés de BallCandidate
  -> merge/RLE externe
  -> census d’une clé ou d’un petit lot
  -> expansion et comptage
  -> rejet immédiat du BallData
  -> aucun tableau global de boules ni d’événements.
```

Le mode actuel doit rester l’oracle petit/moyen régime de ce futur chemin streaming.

### Arithmétique transactionnelle

Les compteurs et projections doivent utiliser des additions et multiplications vérifiées ou saturantes :

```text
checked_add_u64,
checked_mul_u64.
```

Actuellement `tot_ev * sizeof(ForestEvent)` peut théoriquement reboucler avant la comparaison au plafond. Une porte artificielle avec un compteur de base proche de `UINT64_MAX` suffit à tuer un mutant `preflight-wrap`. Il n’est pas nécessaire de fabriquer quatre milliards d’événements pour tester un entier, même si l’industrie a parfois tenté des stratégies comparables.

Enfin, `max_output_bytes` ne compte aujourd’hui que les octets de `ForestEvent`. Tant que le produit demandé est une forêt résidente, il faut soit :

- le renommer `max_event_bytes` ;
- soit ajouter une borne conservatrice séparée pour les records d’incidence, le DSU, la partition et les deltas.

Un plafond qui laisse entrer les événements puis meurt sur `FRec` n’est pas faux, mais ce n’est pas un plafond de sortie global.

---

## 4. La chaîne `MHGP4_HD` est incomplète avant même le premier kernel

L’annotation des primitives en limbes est la bonne direction. Une dépendance transitive reste cependant host-only :

```cpp
namespace detail_ev {
inline u128 uabs(i128 v); // pas MHGP4_HD
}
```

alors que deux fonctions désormais `MHGP4_HD` l’appellent directement :

```cpp
q4_level_raw(...),
cmp_mu_same_side(...).
```

Le premier vrai appel depuis un kernel ne recevra donc pas une chaîne entièrement device-callable. La correction locale est simplement :

```cpp
MHGP4_HD inline u128 uabs(i128 v) { ... }
```

puis audit transitif de chaque fonction annotée.

Les 109 portes CPU ne peuvent pas exercer cette propriété : hors nvcc, `MHGP4_HD` est vide. Avant d’écrire les noyaux complets, ajouter un minuscule `device_arithmetic_smoke.cu` qui :

1. appelle depuis un kernel chacune des primitives annotées ;
2. utilise des témoins traversant les mots hauts de U192/U320 ;
3. recopie les résultats ;
4. exige l’égalité bit à bit avec le calcul host.

Cette porte doit compiler et s’exécuter avant que le cœur de seed ou le sweep ne soient portés. Elle isolera les problèmes de compilation device, de largeur et de transitivité sans les enfouir dans un kernel de plusieurs centaines de lignes.

---

## Ordre recommandé à Claude

1. Annoter `detail_ev::uabs` et graver le smoke device lors de la première session CUDA.
2. Remplacer **seulement** `final_partition` par `FlatPartition`, ajouter les trois chronos et mesurer `n=8000`.
3. Si `t_delta_materialize` reste important, passer les deltas en CSR plat avec vues d’ABI.
4. Qualifier le plafond actuel comme plafond d’événements tardif et ajouter les opérations arithmétiques vérifiées.
5. Après décision du champ `product`, construire le préflight streaming et les budgets de travail/produit correspondants.

La v4 est maintenant assez avancée pour que les prochains gains viennent moins d’une nouvelle identité géométrique que du choix exact de ce qui reste résident. C’est une bonne nouvelle : les preuves tiennent, et l’optimisation peut enfin cesser de négocier avec elles.
