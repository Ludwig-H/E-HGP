# Suite du 10 septembre, soirée : cache de résolutions, réserve du journal, front de témoins par lots

Second auditeur. Objet : le WIP **non commité** du constructeur observé à
20:35 UTC après `11cde758` (`snapshots/SHA256SUMS`) : `ResolverCache` dans
`full_ball_tower.hpp` (`910f45ba…`), réserve exacte des arènes dans
`full_coverage_certificate.hpp` (`7608e70e…`), libération des structures de
construction, `facet_resolver_cache_gate.cpp` (`4f17c0e5…`), portes de tour et
de travail à 28 nuages, `witness_front.hpp` (`07d990f2…`), `witness_batch.hpp`
(`66f31ead…`) et `witness_front_gate.cpp` (`c1bfc683…`). Rien n'est reçu : ces
octets n'ont ni pin ni reçu constructeur. GCP non utilisé ; aucun temps revendiqué.

## 1. Lecture du cache de résolutions

Mémo direct-mapped de 2^⌈log₂ 16n⌉ entrées de 48 octets, clé = facette triée
exacte (comparaison complète au hit, une collision n'évince que), remis à zéro
par ordre, semé par les naissances fermées (`seed_closed_anchor` : la population
S = I∪U est l'unique K-facette de sa boule à K = |S|, sa clé mène au nœud né),
consulté avant la MEB, stocké après un hit d'ancre sous la clé initiale, jamais
une autorité (allocation impossible ⇒ cache désactivé). Le jeton mémorisé est
normalisé par `root()` à la lecture : les fusions ultérieures sont suivies. La
garde `full_ball_representative_not_strict` est sautée sur hit, sans perte : une
facette résolue à un lot antérieur a une MEB strictement inférieure à tout lot
ultérieur. **Lecture favorable.** Remarque de résidence : 768 octets par point
(48 × 16n), soit 7,7 Gio à dix millions de points ; la désactivation sur
`bad_alloc` est correcte mais silencieuse, un budget déclaré serait préférable.

## 2. Portes du constructeur rejouées (`portes/`)

| Porte | O2 | ASan/UBSan (`detect_leaks=1`) |
| --- | --- | --- |
| `facet_resolver_cache_gate` | 0 ; 28 nuages, 556 hits, 352 semis, 2 752 slots libérés | **avorte** : `alloc-dealloc-mismatch (operator new vs free)` |
| idem, copie corrigée (`facet_resolver_cache_gate_nothrow.diff`) | 0, même sortie | 0, même sortie |
| `full_ball_tower_gate` (28 nuages, fixtures à lots groupés reprises) | 0 ; 170 320 contrôles, 112 ordres, 2 508 coupes, 45 948 contrôles verticaux | 0, mêmes octets |
| `full_ball_work_gate` | 0 ; peignes 256/512/1024 | — |
| `witness_front_gate` | 0 ; 1 728 cas, 429 268 contrôles, 88 560 lignes, 311 968 requêtes, 13 rejets | 0, mêmes octets |

Cause de l'avortement (`facet_resolver_cache_gate_san.asan_report_head.txt`) :
la porte remplace `operator new(size_t)` et les `operator delete` pour injecter
des pannes d'allocation, mais pas `operator new(size_t, nothrow_t)` ;
`std::stable_sort` de `validate_catalogue` obtient son tampon temporaire par
`new(nothrow)` (intercepté par ASan) et le rend par le `delete` remplacé, qui
appelle `free`. Défaut de la porte, pas du produit : ajouter les surcharges
`nothrow` de `new`/`delete` suffit (copie corrigée verte O2 et ASan/UBSan).
Verrou pour le reçu de ce delta : sa capture ASan doit porter cette correction.

## 3. Objet inchangé avec le cache (`empreintes/`, `corpus/`)

- Empreinte de payload de la sonde retenue identique avec et sans cache sur
  `uniform` 400 et 2 000, `scanline_overlap_multiecho` 2 000 (3 726 coquilles
  supplémentaires), `scanline_single_pass` 2 000, `terrain` 2 000 ; mêmes
  nombres de nœuds et de coquilles supplémentaires.
- Corpus aléatoire rejoué contre le header avec cache, même juge Gamma rationnel :
  207 + 300 + 2 000 nuages, 777 628 coupes, **0 divergence, 0 refus** ; le mutant
  `drop_extra_ball` reste détecté (148 refus, 128 coupes divergentes).
- Fixture à neuf points : `same_radius_steps=1`, tour complète.

## 4. Front de témoins par lots (`front_temoins/`)

Différentiel indépendant front scalaire `alive_rectangles_fused` contre
`alive_rectangles_batched` avec le backend hôte `CpuWitnessBatch` sur des
nuages générés, égalité **ordonnée** des rectangles, masques et cœurs, du
grand-livre des masses et du travail physique :

| Famille | n | s | lot | fils | lignes | requêtes | nœuds visités | identique |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| uniform | 2 000 | 8 | 4 096 | 1 | 161 514 | 1 103 050 | 94 180 201 | oui |
| uniform | 2 000 | 8 | 100 000 | 4 | 161 514 | 1 103 050 | 94 180 201 | oui |
| uniform | 2 000 | 10 | 4 096 | 2 | 168 908 | 1 243 829 | 104 400 351 | oui |
| uniform | 2 000 | 12 | 4 096 | 2 | 174 268 | 1 337 274 | 111 391 611 | oui |
| scanline_overlap_multiecho | 2 000 | 8 | 4 096 | 2 | 21 283 | 86 991 | 3 739 973 | oui |
| eight_clusters | 2 000 | 8 | 4 096 | 2 | 121 977 | 591 660 | 46 115 396 | oui |
| terrain | 2 000 | 8 | 4 096 | 2 | 47 261 | 278 516 | 14 358 805 | oui |
| uniform | 8 000 | 8 | 65 536 | 4 | 754 686 | 6 051 749 | 563 616 452 | oui |
| scanline_overlap_multiecho | 8 000 | 8 | 1 000 | 4 | 89 893 | 391 932 | 19 347 957 | oui |

Lecture : seules les requêtes de témoins sont déléguées ; géométrie, ordre des
vagues, scission et grand-livre restent sur l'hôte ; les décisions sont
rejouées dans l'ordre de la vague ; échec de forme ou d'association ⇒ aucune
sortie (13 rejets atomiques dans la porte). Aucun backend CUDA n'est jugé ici.

## 5. Ce qui reste

Les verrous du reçu principal (§ 5) sont inchangés ; le constructeur a repris les
fixtures à lots groupés (`grouped.fixture_not_singleton_only`), le rejeu des quatre
mutants sur le header commité et les contrôles nommés des blocs réels 50k restent
dus. Pour ce delta : porte du cache corrigée pour ASan, budget de résidence du
cache déclaré, et un reçu ancré aux octets réellement commités.
