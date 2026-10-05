// Declarations internes du module points (tranche S9), partagees par ses fichiers : incidences fortes, qualification,
// comparaisons exactes de sommes de racines, constructeur de l'arbre de points. Aucun autre module ne les inclut.
#pragma once

#include <algorithm>
#include <array>
#include <initializer_list>
#include <optional>

#include "points/points.hpp"
#include "sched/sched.hpp"

namespace mhgp11::points {

// Constructeur de l'arbre de points (point_tree.cpp), ami de PointTree.
struct PointTreeBuilder;

}  // namespace mhgp11::points

namespace mhgp11::points_detail {

// Grain des boucles paralleles par site ou par noeud : positions fixes, resultat independant du nombre de fils.
inline constexpr u64 kGrain = 256;

// Incidence forte : (rang << 32) | noeud, le rang de la boule et son noeud de rattachement (coupe fermee).
constexpr u64 pack(u32 rank, u32 node) noexcept { return (u64{rank} << 32) | node; }
constexpr u32 rank_of(u64 packed) noexcept { return static_cast<u32>(packed >> 32); }
constexpr u32 node_of(u64 packed) noexcept { return static_cast<u32>(packed); }

// Incidences fortes par site (CSR a decalages u64), triees par (rang, noeud) dans chaque ligne : port du bloc
// d'incidences de MHGP11PH (bench/points_export.cpp, write_incidences), tire du rattachement W_K sans descente.
struct Incidences {
  Buffer<u64> offsets;  // n + 1
  Buffer<u64> packed;   // I
  u64 widest = 0;       // plus longue ligne
};
// Octets admis par build_incidences pour n sites et I incidences (decalages, valeurs, curseurs).
constexpr u64 incidence_bytes(u64 n, u64 total) noexcept { return 8 * (n + 1) + 8 * total + 8 * n; }
// K = 1 : la feuille de chaque site, au rang 0. K >= 2 : les populations I_b union U_b des boules fortes de W_K
// (p + q_min <= K), au rang de la boule et sur son noeud de rattachement. Refus : memory_budget, points_invariant
// (feuille absente ou en double, site sans incidence, rang ou noeud hors domaine).
[[nodiscard]] Outcome build_incidences(const OrderTree& tree, MemoryBudget& budget, sched::Pool* pool,
                                       Incidences& out) noexcept;

// Execute body sur [0, n) : sur le Pool s'il existe, sinon en un seul appel (fil 0).
[[nodiscard]] Outcome run(sched::Pool* pool, u64 n, void* context, sched::Pool::Body body) noexcept;
inline u32 workers(const sched::Pool* pool) noexcept { return pool == nullptr ? 1 : pool->size(); }

// Qualification (port de qualify, bench/points_hierarchy.py:215-246) : qual[v] = rang ou le noeud v couvre au moins
// m sites distincts pendant sa vie, kNone s'il ne les couvre jamais. m <= K : le rang du noeud. m = K + 1 : toute
// fusion a sa naissance ; une naissance au m-ieme site distinct de ses propres incidences (rang de premiere
// incidence par site, m-ieme plus petit). Refus : parameter_out_of_range (autre m), memory_budget.
[[nodiscard]] Outcome qualify(const OrderTree& tree, const Incidences& incidences, u32 m, MemoryBudget& budget,
                              sched::Pool* pool, Buffer<u32>& qual, u64& qualified) noexcept;

// Comptes des decisions de racines (par fil, sommes apres la jointure).
struct RootTally {
  u64 table = 0, exact = 0, on_demand = 0;
  void add(const RootTally& o) noexcept {
    table += o.table;
    exact += o.exact;
    on_demand += o.on_demand;
  }
};

// Comparaisons exactes de sommes de racines de niveaux du catalogue, par rangs : encadrement de la table (num), puis
// repli exact. Un rang absent de la table voit sa racine calculee a la demande (RootTable::root_of), sans ecriture.
class Roots {
 public:
  Roots(const num::RootTable& table, std::span<const num::Level> levels) noexcept : table_(table), levels_(levels) {}
  std::span<const num::Level> levels() const noexcept { return levels_; }
  // Signe de sqrt(l_a) + sqrt(l_b) - sqrt(l_c) - sqrt(l_d) ; repli num::sqrt_cmp2 (port de sqrt_cmp2,
  // bench/points_radius.py:48-50 : elevations au carre controlees, toujours decide).
  [[nodiscard]] Outcome two_vs_two(u32 a, u32 b, u32 c, u32 d, int& out, RootTally& tally) const noexcept;
  // Ordre de deux dates (t, M, Q) : six racines, repli RadicalSum (port de RValue.cmp et sign_of_radicals,
  // bench/points_radius.py:93-117 et 148-150) ; refus radical_sign_budget au-dela du budget.
  [[nodiscard]] Outcome compare_dates(const std::array<u32, 3>& a, const std::array<u32, 3>& b, num::RadicalSum& sum,
                                      int& out, RootTally& tally) const noexcept;

 private:
  Outcome root(u32 rank, u128& out, RootTally& tally) const noexcept;
  const num::RootTable& table_;
  std::span<const num::Level> levels_;
};

// Proposition binary64 du niveau d'un rang (lecture seulement : la recherche du rang plancher la certifie en entier).
double approximate(const num::Level& level) noexcept;

}  // namespace mhgp11::points_detail

namespace mhgp11::points {

// Arbre de points (point_tree.cpp, port de tower_point_tree, bench/points_flat.py:462-539) a partir d'une pendaison
// complete : au rang r, fusions de la foret de rang r, puis entrees de date exactement sqrt(l_r) (un plateau), puis
// entrees strictement entre r et r + 1, triees par date exacte et groupees par date egale (un plateau par date).
// Refus : memory_budget, radical_sign_budget, points_invariant.
struct PointTreeBuilder {
  struct Input {
    const OrderForest* forest = nullptr;
    std::span<const u32> t, m, q, owner, floor;
    std::span<const u8> strict;
    const points_detail::Roots* roots = nullptr;
  };
  [[nodiscard]] static Outcome build(const Input& in, num::RadicalSum& sum, MemoryBudget& budget, PointTree& out,
                                     points_detail::RootTally& tally) noexcept;
};

}  // namespace mhgp11::points

namespace mhgp11::points {

// Etat d'un appel de hang (hang.cpp, settle.cpp), ami de PointHierarchy. Brouillons par fil : departs et noeuds
// distincts d'une ligne d'incidences (2 x widest mots), comptes de racines et compteurs, sommes apres la jointure.
struct HangBuilder {
  const OrderTree& tree;
  MemoryBudget& budget;
  sched::Pool* pool;
  std::span<const ForestNode> nodes;
  std::span<const num::Level> levels;
  u32 n = 0, m = 0;
  points_detail::Incidences inc;
  Buffer<u32> qual, above;
  std::optional<AncestorIndex> ancestors;
  num::RootTable table;
  std::optional<points_detail::Roots> roots;
  PointHierarchy out;
  std::array<Buffer<u64>, sched::kMaxWorkers> scratch;  // par fil : departs puis noeuds distincts (2 x widest)
  std::array<points_detail::RootTally, sched::kMaxWorkers> tallies;
  std::array<PointsStats, sched::kMaxWorkers> counts;

  HangBuilder(const OrderTree& t, MemoryBudget& b, sched::Pool* p) noexcept
      : tree(t), budget(b), pool(p), nodes(t.forest().nodes()), levels(t.domain().catalogue().levels()) {}

  Outcome prepare() noexcept;
  Outcome fill_roots() noexcept;
  Outcome hang_sites() noexcept;
  Outcome referenced() noexcept;
  Outcome start(u64 packed, u32& node, u32& rank) const noexcept;
  Outcome site(u32 s, u32 worker) noexcept;
  Outcome rival(u32 s, u32 worker, u32 p1, u32& big_m, u32& q) noexcept;
  Outcome settle(u32 s, u32 worker, u32 p1, u32 big_m, u32 q) noexcept;
  Outcome floor_of(u32 s, u32 worker, u32 low, u32 high, u32& floor, int& sign) noexcept;
  Outcome above_floor(u32 s, u32 worker, u32 rank, int& sign) noexcept;
  // Appel entier (settle.cpp) : incidences, qualification, racines, pendaison, rangs references, arbre de points.
  [[nodiscard]] static Result<PointHierarchy> run(const OrderTree& tree, u32 m, MemoryBudget& budget,
                                                  sched::Pool* pool) noexcept;
  static Outcome site_body(void* context, u64 begin, u64 end, u32 worker) noexcept {
    HangBuilder& h = *static_cast<HangBuilder*>(context);
    for (u64 s = begin; s < end; ++s) MHGP11_TRY(h.site(static_cast<u32>(s), worker));
    return {};
  }
};

}  // namespace mhgp11::points
