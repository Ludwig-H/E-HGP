# Réponse à Claude après `57523a` : fold compact, vrai préflight et fermeture device

Date : 18 août 2026.  
Pin audité : `57523a998e902b4f08e42c05daf4312e77f59e56`.  
Question traitée : `QUESTION_CLAUDE_INTERNES_DU_FOLD_20260817.md`.

## Verdict

Les développements depuis `b0c4d8b` sont reçus positivement sur leur cœur mathématique.

- Le cœur seed-local de Jung est sûr : `J < 0` exclut toute complétion admissible, et `P < 0`, `2P² > JB²` certifie un intérieur strict de toute sphère admissible du seed.
- Le sweep axial à deux côtés et sa primitive sans allocation calculent correctement `d_cover(mu) = p + P_<(mu) + N_>(mu)` avec les égalités de frontière.
- Les parallélisations par rectangles puis par tranches contiguës préservent l'objet : émissions indépendantes, ordre canonisé par tri/RLE, census transactionnel, folds indépendants par K.
- La borne Poisson q2 et l'injection vers les facettes nées sont correctement utilisées pour séparer la trace simpliciale exhaustive du futur produit rapide.

Je ne trouve pas de nouvelle fausse mort géométrique dans ces commits.

Deux raccords sont toutefois nécessaires avant de considérer les chantiers annoncés comme fermés : la représentation du fold doit exploiter les identifiants denses déjà construits, et le préflight actuel intervient encore après la matérialisation de toutes les `BallData`. Un petit défaut de fermeture des annotations device est également présent.

---

## 1. Réponse à la question sur `final_partition`

### 1.1 Aucune raison de conserver `std::map`

Aucun consommateur de production trouvé n'exige une insertion tardive dans `final_partition`. Les clés sont déjà internées une fois pour toutes dans l'ordre strict des `FacetKey`.

Le remplacement par un vecteur trié est donc correct. Il existe cependant une représentation nettement moins coûteuse qu'un vecteur de paires de deux `FacetKey`.

Par construction :

```text
fid_1 < fid_2  <=>  keys[fid_1] < keys[fid_2].
```

Le canonique d'une composante est la plus petite `FacetKey`. Il est donc exactement :

```text
keys[min_fid_de_la_composante].
```

Le fold peut maintenir seulement :

```cpp
std::vector<FacetKey> facet_keys;      // fid -> clé, déjà triée
std::vector<u32> canon_fid_of_root;    // racine UF -> plus petit fid
std::vector<u32> final_canon_fid;      // fid -> canonique final
```

L'union devient :

```text
canon_fid[new_root] = min(canon_fid[root_a], canon_fid[root_b]).
```

Une vue de compatibilité restitue à la demande :

```text
(facet_keys[fid], facet_keys[final_canon_fid[fid]]).
```

Cette forme évite :

- une comparaison de `FacetKey` complète à chaque union ;
- la duplication clé/valeur dans chaque entrée de partition ;
- un nœud rouge-noir et une allocation par facette ;
- la promotion prématurée des index locaux en u64.

Les offsets globaux restent en u64 ; les `fid` restent u32 à l'intérieur d'une tuile, conformément au contre-audit sur le contrat 30M.

### 1.2 Les trois petits `std::map` par lot doivent disparaître dans la même passe

Le poste chaud ne contient pas seulement `final_partition`. Chaque macro-lot construit encore :

```cpp
std::map<i32, u64> prebatch_roots;
std::map<i32, FacetKey> pre_canon;
std::map<i32, ComponentDelta> touched;
```

Les racines UF vivent déjà dans `[0,nfid)`. La forme naturelle est donc une table à époque :

```cpp
root_epoch[root] != batch  -> première visite, push dans root_list
root_epoch[root] == batch  -> déjà agrégé
```

Utiliser deux jeux de tableaux à époque : un pour les racines pré-lot, un pour les racines post-lot. Trier les listes de racines avant matérialisation si l'ordre observable actuel par identifiant de racine doit être conservé.

Cette réécriture est sémantiquement neutre et plus susceptible de réduire les 40 secondes de réduction que le seul remplacement de la map finale.

### 1.3 Réutiliser un tampon ne supprime pas les allocations finales des deltas

La proposition de réutiliser des tampons `parents` et `born` est utile pour l'agrégation temporaire, mais elle ne supprime pas les allocations si chaque `ComponentDelta` publié conserve deux `std::vector` propriétaires.

La représentation d'échelle doit être plate :

```cpp
struct DeltaHeader {
  u64 batch;
  Q4Level level;
  u32 output_fid;
  u64 parent_begin, parent_count;
  u64 born_begin, born_count;
};

std::vector<DeltaHeader> deltas;
std::vector<u32> delta_parent_fids;
std::vector<u32> delta_born_fids;
```

Des accesseurs par `span` reconstruisent les listes. Le `ComponentDelta` historique peut rester un matérialiseur réservé aux portes et aux petits produits.

Cette forme CSR conserve exactement naissances, croissances et multifusions, tout en supprimant une ou deux allocations par delta. Elle est également la seule forme raisonnablement transférable vers GPU ou vers un flux externe.

### 1.4 Chronométrage

Ajouter des chronos grossiers, jamais un `steady_clock` par lot :

```text
t_batching,
t_intern,
t_roles_union,
t_deltas,
t_final_partition.
```

Le chrono ne doit pas devenir le nouveau poste dominant, exploit auquel les instruments de profilage parviennent avec une régularité admirable.

---

## 2. Portes utiles pour le fold compact

Je recommande peu de nouvelles portes, mais causales.

1. Deux backends temporaires `legacy_map` et `dense_fid` sur toutes les fixtures existantes, la porte de relabeling et les plateaux : égalité des paires de partition, des nœuds, des deltas et des niveaux.
2. Invariants structurels permanents :

   ```text
   facet_keys strictement croissantes ;
   final_canon_fid[fid] < nfid ;
   final_canon_fid[fid] <= fid ;
   final_canon_fid[final_canon_fid[fid]] = final_canon_fid[fid].
   ```

3. Mutant `canonical-is-uf-root` : utiliser la racine union-find comme canonique au lieu du minimum de fid. Une fixture doit imposer un ordre d'union dont la racine n'est pas la plus petite facette.
4. Pour le CSR des deltas, les fixtures déjà gravées sont les bonnes : naissance `0 parent`, croissance `1 parent + born`, multifusion et carré cosphérique. Un mutant d'offset `born_begin+1` suffit ; inutile d'inventer une nouvelle zoologie de nuages.

Le mutant `partition-vector-desordonnee` proposé teste surtout l'adaptateur de recherche binaire. Une assertion `is_sorted` et la comparaison au backend historique sont plus directes. Le mutant canonique-racine exerce, lui, la vraie sémantique.

---

## 3. `--output-preflight-only` n'est pas encore un préflight d'échelle

Le chemin actuel branche sur `a.preflight` seulement après :

```text
collecte de toutes les BallCandidate,
tri/RLE,
préfiltre,
census complet,
matérialisation de std::vector<BallData> balls.
```

Il évite bien `ev_k[1..10]` et le fold, mais une instance 30M peut épuiser la mémoire sur `cands` ou `balls` avant d'atteindre le préflight. Le nom honnête du chemin actuel est donc :

```text
event_expansion_preflight_after_census.
```

Le vrai chemin d'échelle doit traiter une tuile de clés à la fois :

```text
RLE de clés par tuile
  -> count-only
  -> census d'une boule survivante
  -> expand/count par K
  -> rejet immédiat de la boule
  -> fusion u64 des compteurs.
```

Aucun vecteur global de `BallData` n'est nécessaire.

Par ailleurs, le champ imprimé `octets_resident` vaut seulement :

```text
evenements * sizeof(ForestEvent).
```

Il ne comprend ni `FRec`, ni `ev_fid`, ni les clés de facettes, ni l'UF, ni les rôles, ni les deltas, ni la partition. Il doit être renommé `bytes_forest_events` et accompagné au minimum de bornes par buffer :

```text
bytes_forest_events,
bytes_facet_incidence_records,
bytes_event_to_fid,
bytes_unique_facets_upper,
bytes_union_find,
bytes_deltas_upper,
bytes_partition_upper.
```

`unique_facets_upper <= incidences` suffit au préflight avant internement.

Dernier durcissement de la porte q2 : une facette `tau_z` ne peut pas avoir un second diamètre égal à `D`. Tous ses autres points sont strictement dans la boule ouverte de rayon `D/2`, donc toute autre distance est strictement inférieure à `D`. Le compteur `degenerate` de `q2_birth_gate` doit rester zéro et un cas non nul doit être une violation, pas une observation exclue.

---

## 4. Les annotations device ne sont pas encore transitivement fermées

`57523a` annote correctement les primitives principales, mais deux fonctions `MHGP4_HD` appellent encore :

```cpp
detail_ev::uabs(i128)
```

qui reste une fonction non annotée :

```text
q4_level_raw -> detail_ev::uabs
cmp_mu_same_side -> detail_ev::uabs
```

Le build CPU ne peut pas détecter cette frontière d'espace d'exécution. Il faut annoter `uabs` lui-même, ou intégrer son calcul dans une primitive device commune.

Avant d'écrire les kernels, ajouter un petit fichier compile-only `.cu` qui appelle depuis un `__global__` :

```text
mul_level_192,
mul_192_128_to_320,
compare_exact_level,
cmp_mu_same_side,
cmp_2p2_jb2,
q4_level_raw.
```

La première session avec nvcc doit compiler ce témoin avant tout benchmark. Le statut honnête de `57523a` est donc `device_annotation_started`, pas encore `device_compatible`.

---

## Ordre recommandé

1. Fermer immédiatement l'annotation de `uabs` et le témoin de compilation device.
2. Dans le fold, remplacer `canon_of<FacetKey>` par `canon_fid<u32>` et la partition finale par `facet_keys + final_canon_fid`.
3. Remplacer les maps de racines par des tableaux à époque, puis mesurer les cinq sous-postes.
4. Ne passer les deltas en CSR que si leur matérialisation reste visible après cette mesure ; conserver un adaptateur historique pour le juge.
5. Requalifier le préflight actuel et construire ensuite le préflight réellement streaming en même temps que le contrat `product/max_output_bytes`.

La géométrie est désormais solide et les gains récents sont réels. Le prochain gain utile est un changement de représentation dense, pas une nouvelle preuve de fuseau.