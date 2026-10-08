# Interface des étages T, M, V, R et de l'export FUL1 (`src/tower/`, fichiers `forest_*`, `vertical_*`, `export_*`)

7 octobre 2026. Agent T/M/V/R. Espace `mhgp12::tower` (aucun nom en commun avec l'espace `mhgp12` de l'étage G).

```cpp
// Entree d'un ordre k (sortie de G, sans copie) : naissances par cle croissante, cellules non naissances par BallIdx
// croissants (jonctions et inertes), CSR des representants, cibles 4 octets (bit 31 = cellule).
struct ForestInput { Order k; std::span<const u32> birth_key; std::span<const LevelRank> birth_rank;
                     std::span<const BallIdx> cell_ball; std::span<const LevelRank> cell_rank;
                     std::span<const u64> rep_offsets; std::span<const u32> targets; };
template <class Resolved> ForestInput forest_input(const Resolved& order) noexcept;   // ResolvedOrder de G
BallSource catalogue_balls(const Catalogue& catalogue) noexcept;                      // S* lus en place (T1)

Result<TowerForests> build_forests(const Cloud&, const BallSource&, std::span<const ForestInput>,
                                   const ForestParams&, MemoryBudget&, sched::Pool&, ForestLedger* = nullptr) noexcept;
Result<u32> component_at(const OrderForest&, u32 leaf, u32 rank, u64* hops = nullptr, u64* probes = nullptr) noexcept;
Outcome validate_forests(const TowerForests&, MemoryBudget&) noexcept;              // portes, hors chronometre

struct FullSource { const Cloud* cloud; std::span<const num::Level> levels; BallSource balls;
                    const TowerForests* forests; };
Outcome export_full(const FullSource&, io::FileWriter&) noexcept;                    // FUL1, hors chronometre
Result<io::Digest> full_digest(const FullSource&, u64* bytes = nullptr) noexcept;
```

Chaîne du produit (outil `tests/tower/tower_chain.cpp`) :

```cpp
auto index = build_index(std::move(cloud), IndexParams{}, budget);
auto catalogue = build_catalogue(index.value().cloud(), params, budget, pool);
auto resolution = resolve_tower(index.value(), catalogue.value(), budget, pool);            // etage G
std::vector<tower::ForestInput> in;
for (Order k = 1; k <= resolution.value().orders(); ++k) in.push_back(tower::forest_input(resolution.value().order(k)));
auto forests = tower::build_forests(index.value().cloud(), tower::catalogue_balls(catalogue.value()), in, {},
                                    budget, pool, &ledger);
tower::FullSource src{&index.value().cloud(), catalogue.value().levels(), tower::catalogue_balls(catalogue.value()),
                      &forests.value()};
```

Registre `OrderForest` (étage R), numérotation canonique : `birth_key` (nœud de naissance → clé), `birth_node`
(indice d'entrée → nœud), `rank`, `parent` (`kNone` à la racine), `minleaf`, `children` (CSR triée), `lower`
(verticales, k ≥ 2), `cell_node` (sommet laissé par chaque cellule), `event_cell` (cellule de chaque événement),
hyperarêtes retenues par Kruskal (`retained_cell`, `retained_ball`, `retained_rank`) et leurs branches ouvertes
(`branches` : CSR, nœuds vivants à la coupe ouverte du rang de la cellule), historique d'attache (`attach_parent`,
`attach_rank`, `survivor_events`, `event_rank`, `event_node`).

Mémoire : chaque étage admet ses tampons avant de les allouer (`MemoryBudget::admit`, par le pilote) ; T, M, V
une fois (plus leurs tâches), R deux fois (travail et lignes, puis, après comptage, la sortie : `4·A` octets de
branches et `8·(R_k + 1)` de décalages par ordre) ; `validate_forests` admet aussi ses tampons. Porte
`mhgp12_tower_forest_admission` : pic mesuré de chaque étage ≤ octets admis.

Fusion faite sur `main` `9c5809919` (étage G intégré), patch final rebasé sur `c903774b1` (juge G durci et ses
portes conservés ; `tower_query_domain` = 31, après les raisons de l'appareil du catalogue) : codage des cibles de G (espace `mhgp12`) seul ; raisons `tower_capacity` et `tower_invariant`
de G réutilisées, `tower_query_domain` ajoutée ; dépendances du module : celles de G plus `io` (export) ;
`tower.hpp` : la section de G puis `#include "tower/export_full.hpp"` et `#include "tower/forest.hpp"` ;
`module.cmake`, `tests/tower/tests.cmake`, `tests/mutants/tower.json` : la part de G puis la mienne (mutants : 7 de
G, 9 des miens, plancher 16).
