# Interface de l'étage G (résolution) du module `src/tower/` — v12

Première publication 7 octobre 2026, 19:16 UTC ; révision 20:01 UTC (forme livrée, ci-dessous ; les révisions sont
datées en bas). Agent « étage G ». Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference`,
`objet=full_pi0`, `quantification=quantized_u21_input_only`, `public_status=not_claimed`, GCP non utilisé. Base :
`main` = `6710723399876266d1efd7b45cf0d6adcd49a862`.

## 1. Fichiers de l'étage G (aucun préfixe `forest_`, `vertical_`, `registry_`, `export_`)

- `src/tower/tower.hpp` : en-tête public, section « étage G » (à fusionner avec la section T/M/V/R) ;
- `src/tower/module.cmake` : `mhgp12_module_sources(supports.cpp cells.cpp cells_stage.cpp populations.cpp
  resolve.cpp stage.cpp)` (à compléter par les sources de T/M/V/R) ;
- internes : `internal.hpp`, `stage.hpp`, `proposal.hpp`, `supports.cpp`, `cells.cpp`, `cells_stage.cpp`,
  `populations.cpp`, `resolve.cpp`, `stage.cpp`, `source_pins.json` ;
- portes : `tests/tower/tests.cmake` (à fusionner), `unit.cpp`, `unit_support.hpp`, `g_dump.py`, `g_oracle.py`,
  `g_diff_v11.py`, `g_determinism.py` ; `tests/mutants/tower.json` (7 mutants, à fusionner avec ceux de T/M/V) ; sonde
  `bench/tower_probe.cpp` et exportateur de test `bench/tower_export.hpp` ;
- raisons ajoutées en fin de `src/core/reasons.def` (module `tower`) : `catalogue_missing_ball`, `census_mismatch`,
  `tower_capacity` (`resource_exhausted`), `tower_invariant` (`invariant_violated`), `cell_capacity`
  (`unsupported_degeneracy`). **T/M/V/R réutilisent `tower_capacity` et `tower_invariant`.**
- table des modules : ligne `tower` (dépendances `core`, `num`, `sched`, `cloud`, `index`, `catalogue`) dans
  `docs/ARCHITECTURE.md` § 5 et `cmake/modules.cmake`.

## 2. Entrée

```cpp
// Le catalogue DOIT etre construit sur index.cloud() (memes SiteIdx). Pool et budget empruntes pendant l'appel.
[[nodiscard]] Result<Resolution> resolve_tower(const GlobalIndex& index, const Catalogue& catalogue,
                                               MemoryBudget& budget, sched::Pool& pool,
                                               ResolutionDiagnostics* diagnostics = nullptr) noexcept;
[[nodiscard]] Outcome check_order_capacity(u64 births, u64 cells, u64 representatives) noexcept;  // tower_capacity
```

Refus (jamais un préfixe publié) : `kmax_out_of_range`, `empty_input`, `cell_capacity` (énumération d'une coquille
étendue au-delà de `kMaxCellCombinations = 4096` t-parties ou `kMaxShellWitnesses = 4096` supports), `tower_capacity`
(avant allocation), `memory_budget`, puis pendant la résolution `catalogue_missing_ball`, `census_mismatch`,
`shell_capacity` (coquille d'un census de plus de 64 sites), `tower_invariant`. `Outcome::order` porte l'ordre `k`.

## 3. Sortie

```cpp
inline constexpr u32 kTargetCellBit = 0x80000000u, kTargetIndexMask = 0x7FFFFFFFu, kNoTarget = 0xFFFFFFFFu;
constexpr u32 birth_target(u32 birth); constexpr u32 cell_target(u32 cell);
constexpr bool target_is_cell(u32 t);  constexpr u32 target_index(u32 t);
inline constexpr u8 kCellInert = 1, kCellExtended = 2;   // drapeaux d'une cellule

class Resolution {
  Order kmax() const;  Order orders() const;          // orders() = min(K, n)
  const ResolvedOrder& order(Order k) const;          // 1 <= k <= orders()
  u32 window_target(BallIdx b, Order k) const;        // cible de la cellule (b, k) : naissance ou cellule de l'ordre
};                                                    // k ; kNoTarget hors de la fenetre de b (utile a LEM-T6 : la
                                                      // jonction de la meme boule a l'ordre k - 1)
class ResolvedOrder {
  Order order() const;  u32 births() const;  u32 cells() const;  u64 representatives() const;
  std::span<const u32>       birth_keys();   // k = 1 : SiteIdx (= indice de naissance) ; k >= 2 : BallIdx ; croissants
  std::span<const LevelRank> birth_ranks();  // rang dense du niveau (0 a k = 1)
  std::span<const BallIdx>   cell_balls();   // cellules de fenetre NON naissances (jonctions ET inertes), BallIdx croissants
  std::span<const LevelRank> cell_ranks();   // rang de la boule de la cellule (croissant au sens large)
  std::span<const u8>        cell_flags();   // kCellInert | kCellExtended
  std::span<const u64>       cell_offsets(); // cells() + 1 : representants de la cellule c = [off[c], off[c+1])
  std::span<const u64>       trace_masks();  // trace stricte I u A : masque de A dans U (bit j = j-ieme site de U,
                                             // SiteIdx croissants), ordre lexicographique des A (celui de la v11)
  std::span<const u32>       targets();      // cible de chaque representant
  const OrderCounters& counters();           // objet / travail (paragraphe 8)
};
```

**Lecture des cibles par le noyau T** (CONTRAT_TOUR.md § 4.1, `_forest_v12` de l'oracle) :

- **naissances** de l'ordre : indices `0 .. births()-1` de `birth_keys()` (ordre des clés, donc rangs croissants au
  sens large ; la numérotation canonique par (rang, centre exact) reste celle de M) ;
- **jonctions** : toutes les cellules `0 .. cells()-1` (inertes comprises, pont de `LEM-T4`), par rang croissant puis
  indice de cellule (l'ordre de la liste) ;
- `birth_target(i)` : élément = la naissance `i` ; `cell_target(c)` : élément = l'élément de la cellule `c` (rang
  strictement inférieur, donc déjà traitée), **relu à sa racine courante** ; tous les représentants d'une cellule
  traitée sont dans une même composante ;
- date d'usage : rang de la cible < rang de la jonction, garanti par la décroissance contrôlée de G (aucune garde
  `beta(F0) <= l(r_b - 1)`, `CST-0104`) ;
- `k = 1` : naissances = les `n` sites, toute cible est une naissance (le site).

Constat utile au noyau : sur ng00–02 à K5, chaque cible de la v12 a pour nœud v11 **exactement** la graine de la v11
(cible « cellule » `c'` : graine de la première trace de `c'`) : le différentiel T peut donc comparer nœud à nœud.

## 4. Compteurs (`OrderCounters`, CONTRAT_TOUR.md § 8)

Objet : `births`, `cells`, `inert_cells`, `extended_cells`, `representatives`. Travail (politique `v11_indices` :
saut aux `k` plus petits `SiteIdx` de `I`, sans `G-L3`) : `probes`, `first_probe_hits`, `probe_hits_after_steps`,
routes `route_t1`, `route_cert_table`, `route_cert_census`, `route_fallback_table`, `route_fallback_census`, raisons
de repli, `census_saturated`, `census_complete`, `census_sites` (somme), `census_sites_max`, `census_nodes`,
`jumps_catalogue`, `jumps_census`, `inert_steps`, `cell_stops`, `birth_stops`, `controls`, `max_chain`,
`chain_histogram[16]`. Contrôlés avant publication : `controls = plus petites boules + succès de sonde` et une chaîne
par représentant. Physiques : `ResolutionDiagnostics` (fils, octets, durées par étape et par ordre).

## Révisions

- 20:01 UTC : `window_target`, drapeaux, raisons, compteurs et constat du différentiel ajoutés ; liste des fichiers.
- 21:07 UTC : liste des portes complétée (`g_determinism.py`, 7 mutants) ; aucune signature changée.
