// Declarations internes du module tower (etage G) : parties, supports et certificats exacts, cellules de fenetre,
// table de populations (LEM-POP), resolveur. Aucun autre module ne l'inclut.
#pragma once

#include <array>
#include <optional>
#include <span>

#include "sched/sched.hpp"
#include "tower/tower.hpp"

namespace mhgp12::tower_detail {

inline constexpr u32 kMaxPart = 12;   // K <= 12 : une partie de descente a au plus 12 sites
inline constexpr u32 kMaxShell = 64;  // coquille d'une boule du catalogue ou d'un census complet : masques de 64 bits

// Partie de descente : k SiteIdx strictement croissants.
struct Part {
  std::array<u32, kMaxPart> id{};
  u32 k = 0;
};

// Domaine immuable de la resolution : index (son Cloud), catalogue, points exacts des sites.
struct Domain {
  const GlobalIndex& index;
  const Catalogue& catalogue;
  std::span<const num::Point> points;  // un Point par SiteIdx
};

// Acces prive aux tableaux des resultats publics (stage.cpp seulement).
struct StageAccess {
  static ResolvedOrder& order(Resolution& r, Order k) noexcept { return r.orders_[k - 1]; }
  static Order& order_id(ResolvedOrder& o) noexcept { return o.order_; }
  static Buffer<u32>& birth_keys(ResolvedOrder& o) noexcept { return o.birth_keys_; }
  static Buffer<LevelRank>& birth_ranks(ResolvedOrder& o) noexcept { return o.birth_ranks_; }
  static Buffer<BallIdx>& cell_balls(ResolvedOrder& o) noexcept { return o.cell_balls_; }
  static Buffer<LevelRank>& cell_ranks(ResolvedOrder& o) noexcept { return o.cell_ranks_; }
  static Buffer<u8>& cell_flags(ResolvedOrder& o) noexcept { return o.cell_flags_; }
  static Buffer<u64>& cell_offsets(ResolvedOrder& o) noexcept { return o.cell_offsets_; }
  static Buffer<u64>& trace_masks(ResolvedOrder& o) noexcept { return o.trace_masks_; }
  static Buffer<u32>& targets(ResolvedOrder& o) noexcept { return o.targets_; }
  static OrderCounters& counters(ResolvedOrder& o) noexcept { return o.counters_; }
  static Buffer<u64>& window_offsets(Resolution& r) noexcept { return r.window_offsets_; }
  static Buffer<u32>& window_targets(Resolution& r) noexcept { return r.window_targets_; }
  static Buffer<u8>& window_lo(Resolution& r) noexcept { return r.window_lo_; }
  static void set_orders(Resolution& r, Order kmax, Order orders) noexcept {
    r.kmax_ = kmax;
    r.orders_count_ = orders;
  }
};

// ---- supports.cpp : supports et certificats exacts -------------------------------------------------------------------
// Support canonique de la v12 (CST-0113) parmi des sites TOUS sur la sphere : cardinal minimal, puis plus petite liste
// triee des positions (ordre lexicographique des coordonnees) ; paires de milieu, triangles strictement aigus
// coplanaires au centre, tetraedres contenant strictement le centre. Rend l'arite (2 a 4) et le support en SiteIdx
// croissants ; 0 si aucun support. Au plus kMaxShell sites, sinon shell_capacity.
[[nodiscard]] Result<u8> canonical_support(const Domain& d, const num::Sphere& sphere, std::span<const u32> sites,
                                           std::array<u32, 4>& support) noexcept;
// Supports de la sphere contenus dans la coquille (masques, bit j = shell[j]) : X dans U est NON separable ssi X
// contient l'un d'eux (Caratheodory, minimalite). Rend leur nombre ; au-dela de out.size(), cell_capacity.
[[nodiscard]] Result<u32> shell_witnesses(const Domain& d, const num::Sphere& sphere, std::span<const SiteIdx> shell,
                                          std::span<u64> out) noexcept;
// Boule certifiee d'une boule du catalogue, depuis son S*.
[[nodiscard]] Result<num::CertifiedBall> catalogue_sphere(const Domain& d, u32 ball) noexcept;

struct Certified {
  num::CertifiedBall ball;
  std::array<u32, 4> support;  // support canonique parmi les sites de F sur la sphere, SiteIdx croissants
  u8 arity;
};
// Certificat exact d'un support S (sites de F, SiteIdx croissants) pour la partie F : centre dans l'enveloppe de S
// (signes barycentriques, NUM-CERTIFIEE) et tous les sites de F du cote ferme ; puis canonisation parmi F sur la sphere.
// Rien si S est degenere, si son centre n'est pas strictement dans son enveloppe, ou si un site de F sort.
[[nodiscard]] Result<std::optional<Certified>> certify_part(const Domain& d, const Part& f,
                                                            std::span<const u32> support) noexcept;
// Repli exact (bounded_meb de la v11 : diametre, puis triangles, puis tetraedres) : un support strict de la plus petite
// boule de F, SiteIdx croissants ; rend son arite.
[[nodiscard]] Result<u8> exact_support(const Domain& d, const Part& f, std::array<u32, 4>& support) noexcept;

// ---- cells.cpp : cellules de fenetre --------------------------------------------------------------------------------
// Memoire de travail d'un fil pour les coquilles etendues (comptee dans le budget par l'etage).
struct CellScratch {
  std::span<u64> witnesses;  // kMaxShellWitnesses
  std::span<u64> masks;      // kMaxCellCombinations : traces strictes d'une cellule, puis copie triee
  std::span<u64> sorted;     // kMaxCellCombinations
  std::span<u32> parent;     // kMaxCellCombinations
};

// Forme de la cellule (b, k) : naissance, ou cellule de fenetre (drapeaux kCellInert, kCellExtended) a reps traces.
struct CellShape {
  bool birth = false;
  u8 flags = 0;
  u64 reps = 0;
};
// Fenetre [lo, hi] de la boule b bornee par orders (lo > hi : aucune cellule).
struct Window {
  u32 lo = 0, hi = 0;
};
Window ball_window(const CatalogueBall& ball, Order orders) noexcept;
// Visite des cellules de la boule b : visit(k, shape, masks) pour k = lo .. hi, masks (taille shape.reps) n'etant lu
// que si fill. La coquille etendue est enumeree une fois par ordre (plafonds declares, refus cell_capacity).
template <class Visit>
Outcome visit_cells(const Domain& d, u32 ball, Order orders, CellScratch& scratch, Visit&& visit) noexcept;

// Classe une coquille etendue a l'ordre t (1 <= t < m) : traces strictes dans scratch.masks, forme rendue.
[[nodiscard]] Result<CellShape> classify_extended(u32 m, u32 t, std::span<const u64> witnesses,
                                                  CellScratch& scratch) noexcept;

template <class Visit>
Outcome visit_cells(const Domain& d, u32 ball, Order orders, CellScratch& scratch, Visit&& visit) noexcept {
  const auto& data = d.catalogue.balls_data()[ball];
  const Window w = ball_window(data, orders);
  if (w.lo > w.hi) return {};
  const u32 m = data.m;
  if (m > kMaxShell || m < data.qmin || data.qmin < 2) return fail(Reason::tower_invariant);
  if (m == data.qmin) {  // coquille reguliere : t = m - 1 (jonction a m traces) ou t = m (naissance)
    for (u32 k = w.lo; k <= w.hi; ++k) {
      const u32 t = k - data.p;
      if (t == m) {
        MHGP12_TRY(visit(k, CellShape{true, 0, 0}, std::span<const u64>{}));
        continue;
      }
      if (t + 1 != m) return fail(Reason::tower_invariant);
      const u64 full = m == 64 ? ~u64{0} : (u64{1} << m) - 1;
      for (u32 r = 0; r < m; ++r) scratch.masks[r] = full & ~(u64{1} << (m - 1 - r));  // ordre lexicographique des A
      MHGP12_TRY(visit(k, CellShape{false, 0, m}, std::span<const u64>(scratch.masks.data(), m)));
    }
    return {};
  }
  auto sphere = catalogue_sphere(d, ball);
  if (!sphere.ok()) return sphere.outcome();
  auto count = shell_witnesses(d, sphere.value().sphere(), d.catalogue.shell(make_id<BallIdx>(ball)),
                               scratch.witnesses);
  if (!count.ok()) return count.outcome();
  const std::span<const u64> witnesses(scratch.witnesses.data(), count.value());
  for (u32 k = w.lo; k <= w.hi; ++k) {
    const u32 t = k - data.p;
    if (t == m) {
      MHGP12_TRY(visit(k, CellShape{true, kCellExtended, 0}, std::span<const u64>{}));
      continue;
    }
    auto shape = classify_extended(m, t, witnesses, scratch);
    if (!shape.ok()) return shape.outcome();
    MHGP12_TRY(visit(k, shape.value(), std::span<const u64>(scratch.masks.data(), shape.value().reps)));
  }
  return {};
}

// ---- populations.cpp : table de populations (LEM-POP) ----------------------------------------------------------------
// Naissances (b, k) de population exacte k (p + m = k) : population triee -> indice de naissance. Cases a etiquette
// (etiquette 32 bits, naissance + 1), egalite verifiee sur les SiteIdx contre la CSR du catalogue.
class PopulationTable {
 public:
  [[nodiscard]] Outcome build(const Catalogue& catalogue, std::span<const u32> birth_keys, Order k,
                              MemoryBudget& budget) noexcept;
  // Indice de naissance de la boule de population exactement F, ou rien.
  std::optional<u32> find(const Catalogue& catalogue, std::span<const u32> birth_keys, const Part& f) const noexcept;
  u64 bytes() const noexcept { return slots_.size() * sizeof(u64); }
  static u64 capacity_for(u64 entries) noexcept;

 private:
  Buffer<u64> slots_;
};
// Nombre d'entrees de la table de l'ordre k : naissances de population exacte k.
u64 population_entries(const Catalogue& catalogue, std::span<const u32> birth_keys, Order k) noexcept;

// ---- resolve.cpp : resolution d'un representant ------------------------------------------------------------------------
struct OrderView {
  Order k = 0;
  std::span<const u32> birth_keys;
  const PopulationTable* table = nullptr;
};
struct ResolveContext {
  const Domain& domain;
  const Resolution& windows;  // window_target(b, k) deja rempli
  OrderView order;
};
// LEM-T1 (CONTRAT_TOUR.md, paragraphe 4.1 ; CST-0101, WIT-T1-CARRE) : si le support propose S (SiteIdx croissants)
// est dans F, si S = S*(b) pour une boule b du catalogue et si F est dans P_b, alors la plus petite boule de F est b,
// sans arithmetique ; les deux inclusions sont testees sur les identifiants. Sinon rien.
std::optional<u32> lem_t1(const Domain& d, const Part& f, std::span<const u32> support) noexcept;
// Cible de la partie f, trace stricte d'une cellule de rang junction_rank de l'ordre k >= 2 (date initiale controlee).
[[nodiscard]] Result<u32> resolve_part(const ResolveContext& c, Part f, LevelRank junction_rank,
                                       CensusWorkspace& workspace, OrderCounters& counters) noexcept;
// Fusion d'un compteur de fil dans le total (sommes et maxima).
void add_counters(OrderCounters& total, const OrderCounters& part) noexcept;

}  // namespace mhgp12::tower_detail
