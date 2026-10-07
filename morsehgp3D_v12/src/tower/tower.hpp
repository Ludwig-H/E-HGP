// En-tete public du module tower (tranche T2 de la v12, docs/CONTRAT_TOUR.md) : premiere partie, l'etage G (la
// resolution), voie CPU de reference. Pour chaque ordre k = 1..K, les naissances, les cellules de fenetre non
// naissances (jonctions et cellules inertes, pont de LEM-T4) avec leurs representants (traces strictes), et la cible de
// chaque representant (CONTRAT_TOUR.md, paragraphes 4 et 7). Le noyau, la contraction, les verticales et le registre
// (etages T, M, V, R) consomment ces sorties.
//
// Cellules (port de cells.cpp et cells_classify.cpp de la v11 gelee, ac081a06f) : pour chaque boule b de Cat_K et
// chaque ordre k de sa fenetre [p + q_min - 1, min(p + m, K)], t = k - p. t = m : naissance. Coquille reguliere
// (m = q_min) : forme analytique, t = m - 1, m traces (U privee d'un site), jonction. Coquille etendue : les traces
// strictes sont les t-parties A de U SEPARABLES (le centre hors de l'enveloppe de A ; OBJ-T2), enumerees dans l'ordre
// lexicographique (celui de la v11) ; aucune : naissance ; un seul morceau (A ~ A' si A u A' est separable) : cellule
// inerte, sinon jonction. La separabilite se lit sur les supports de la sphere contenus dans U (Caratheodory) ;
// plafonds declares kMaxCellCombinations et kMaxShellWitnesses, refus cell_capacity au-dela (LEM-T7 reste une
// amelioration ulterieure).
//
// Resolution (CONTRAT_TOUR.md, paragraphe 4.1, politique de saut de la v11 : les k plus petits SiteIdx de I) : sonde de
// la table de populations (LEM-POP), proposition flottante (Welzl de la v10, ne decide rien) puis LEM-T1 (S dans F et
// F dans P_b, sur les identifiants), sinon certificat exact du support propose et canonisation par positions parmi les
// sites de F sur la sphere, sinon repli exact ; census GARDE de seuil k ; decroissance stricte controlee AVANT toute
// sortie par saut, depuis le rang de la cellule de la jonction ; pas inerte sous la fenetre ; arret sur la premiere
// cellule de fenetre (cible << cellule >>) ou une naissance (cible << naissance >>).
//
// Parallelisme : fonction pure du domaine immuable, repartie par tranches de cellules sur le Pool ; chaque
// representant a sa case, ecrite par un seul fil ; aucun atomique partage ; sorties et compteurs du travail identiques
// a tout nombre de fils. Capacite : comptage puis reservation, refus avant allocation, jamais un prefixe publie.
#pragma once

#include <array>
#include <span>
#include <utility>

#include "catalogue/catalogue.hpp"
#include "index/index.hpp"

namespace mhgp12 {

namespace sched {
class Pool;
}
namespace tower_detail {
struct StageAccess;
}

// ---- Cibles de 4 octets (CONTRAT_TOUR.md, paragraphe 7) -------------------------------------------------------------
// Bit 31 : genre (0 naissance, 1 cellule) ; 31 bits d'indice dans l'espace de son genre (naissance de l'ordre, ou
// cellule de fenetre de l'ordre). Au plus 2^31 - 1 naissances et 2^31 - 1 cellules par ordre : un indice vaut au plus
// 2^31 - 2, et 0xFFFFFFFF (kNoTarget, reserve) n'est jamais produit.
inline constexpr u32 kTargetCellBit = 0x80000000u;
inline constexpr u32 kTargetIndexMask = 0x7FFFFFFFu;
inline constexpr u32 kNoTarget = 0xFFFFFFFFu;
inline constexpr u64 kMaxOrderBirths = kTargetIndexMask;  // 2^31 - 1 (CST-0212)
inline constexpr u64 kMaxOrderCells = kTargetIndexMask;   // 2^31 - 1
inline constexpr u64 kMaxOrderRepresentatives = kNone;    // 2^32 - 1 : indices u32 sous la sentinelle kNone
// Plafonds declares des coquilles etendues (refus cell_capacity) : t-parties enumerees par cellule, et supports de la
// sphere dans U (paires de milieu, triangles strictement aigus coplanaires au centre, tetraedres le contenant).
inline constexpr u64 kMaxCellCombinations = 4096;
inline constexpr u32 kMaxShellWitnesses = 4096;

constexpr u32 birth_target(u32 birth) noexcept { return birth; }
constexpr u32 cell_target(u32 cell) noexcept { return kTargetCellBit | cell; }
constexpr bool target_is_cell(u32 target) noexcept { return (target & kTargetCellBit) != 0; }
constexpr u32 target_index(u32 target) noexcept { return target & kTargetIndexMask; }

// Genre d'une cellule de fenetre non naissance.
inline constexpr u8 kCellInert = 1;     // un seul morceau : cellule inerte (jonction pour le noyau, LEM-T4)
inline constexpr u8 kCellExtended = 2;  // coquille etendue (m > q_min)

// Histogramme des longueurs de chaine (plus petites boules calculees par representant) : 0 .. 14, puis >= 15.
inline constexpr int kChainBins = 16;

// Compteurs d'un ordre (CONTRAT_TOUR.md, paragraphe 8). OBJET : fonctions de la tour seule. TRAVAIL : fonctions de la
// politique declaree (saut aux k plus petits SiteIdx de I, ni G-L3 ni G-L6) et de l'ordre canonique ; identiques a
// tout nombre de fils, hors de toute empreinte de l'objet. Routes des plus petites boules : t1 (LEM-T1), cert_table et
// cert_census (certificat exact du support propose, puis table ou census), repli_table et repli_census (repli exact).
struct OrderCounters {
  u64 births = 0, cells = 0, inert_cells = 0, extended_cells = 0, representatives = 0;  // objet
  u64 probes = 0, first_probe_hits = 0, probe_hits_after_steps = 0;
  u64 route_t1 = 0, route_cert_table = 0, route_cert_census = 0, route_fallback_table = 0, route_fallback_census = 0;
  u64 fallback_no_proposal = 0, fallback_not_in_part = 0, fallback_certificate = 0;
  u64 census_saturated = 0, census_complete = 0, census_sites = 0, census_sites_max = 0, census_nodes = 0;
  u64 jumps_catalogue = 0, jumps_census = 0, inert_steps = 0;
  u64 cell_stops = 0, birth_stops = 0, controls = 0, max_chain = 0;
  std::array<u64, kChainBins> chain_histogram{};
  u64 smallest_balls() const noexcept {
    return route_t1 + route_cert_table + route_cert_census + route_fallback_table + route_fallback_census;
  }
  friend bool operator==(const OrderCounters&, const OrderCounters&) = default;
};

// Sorties de l'ordre k. Naissances : k = 1, les sites (cle = SiteIdx, rang 0) ; k >= 2, les cellules (b, k) de genre
// naissance (cle = BallIdx), cles croissantes. Cellules : les cellules de fenetre non naissances, BallIdx croissants
// (rangs croissants au sens large). Representants : traces strictes I u A, codees par le masque de A dans U (bit j =
// j-ieme site de U, SiteIdx croissants), dans l'ordre lexicographique des A ; cellule c : [offsets[c], offsets[c+1]).
class ResolvedOrder {
 public:
  ResolvedOrder() = default;
  ResolvedOrder(const ResolvedOrder&) = delete;
  ResolvedOrder& operator=(const ResolvedOrder&) = delete;
  ResolvedOrder(ResolvedOrder&&) noexcept = default;
  ResolvedOrder& operator=(ResolvedOrder&&) noexcept = default;

  Order order() const noexcept { return order_; }
  u32 births() const noexcept { return static_cast<u32>(birth_keys_.size()); }
  u32 cells() const noexcept { return static_cast<u32>(cell_balls_.size()); }
  u64 representatives() const noexcept { return targets_.size(); }
  std::span<const u32> birth_keys() const noexcept { return birth_keys_.span(); }
  std::span<const LevelRank> birth_ranks() const noexcept { return birth_ranks_.span(); }
  std::span<const BallIdx> cell_balls() const noexcept { return cell_balls_.span(); }
  std::span<const LevelRank> cell_ranks() const noexcept { return cell_ranks_.span(); }
  std::span<const u8> cell_flags() const noexcept { return cell_flags_.span(); }
  std::span<const u64> cell_offsets() const noexcept { return cell_offsets_.span(); }
  std::span<const u64> trace_masks() const noexcept { return trace_masks_.span(); }
  std::span<const u32> targets() const noexcept { return targets_.span(); }
  const OrderCounters& counters() const noexcept { return counters_; }

 private:
  friend struct tower_detail::StageAccess;
  Order order_ = 0;
  Buffer<u32> birth_keys_;
  Buffer<LevelRank> birth_ranks_;
  Buffer<BallIdx> cell_balls_;
  Buffer<LevelRank> cell_ranks_;
  Buffer<u8> cell_flags_;
  Buffer<u64> cell_offsets_;
  Buffer<u64> trace_masks_;
  Buffer<u32> targets_;
  OrderCounters counters_;
};

// Diagnostics PHYSIQUES (jamais dans une empreinte) : fils, octets, durees par etape et par ordre.
struct ResolutionDiagnostics {
  u64 threads = 0, workspace_bytes = 0, table_bytes = 0, peak_bytes = 0;
  u64 count_ns = 0, fill_ns = 0, tables_ns = 0, resolve_ns = 0;
  std::array<u64, 13> order_ns{};  // resolution de l'ordre k (indice k)
};

// Resultat de l'etage G : un ResolvedOrder par ordre 1..orders() (orders() = min(K, n)), et la table des cellules
// de fenetre par boule : window_target(b, k) rend la cible de la cellule (b, k) de l'ordre k (naissance ou cellule),
// kNoTarget hors de la fenetre de b. Proprietaire immuable ; deplacement seulement ; le budget survit au resultat.
class Resolution {
 public:
  Resolution() = default;
  Resolution(const Resolution&) = delete;
  Resolution& operator=(const Resolution&) = delete;
  Resolution(Resolution&&) noexcept = default;
  Resolution& operator=(Resolution&&) noexcept = default;

  Order kmax() const noexcept { return kmax_; }
  Order orders() const noexcept { return orders_count_; }
  // Exige 1 <= k <= orders().
  const ResolvedOrder& order(Order k) const noexcept { return orders_[k - 1]; }
  // Exige idx(b) < nombre de boules du catalogue.
  u32 window_target(BallIdx b, Order k) const noexcept;

 private:
  friend struct tower_detail::StageAccess;
  std::array<ResolvedOrder, 12> orders_;
  Buffer<u64> window_offsets_;  // boules + 1 decalages
  Buffer<u32> window_targets_;  // cibles des cellules (b, k), k = lo(b) .. hi(b)
  Buffer<u8> window_lo_;        // premier ordre de la fenetre de b
  Order kmax_ = 0, orders_count_ = 0;
};

// Capacite d'un ordre, avant toute allocation : au plus kMaxOrderBirths naissances, kMaxOrderCells cellules et
// kMaxOrderRepresentatives representants ; sinon tower_capacity.
[[nodiscard]] Outcome check_order_capacity(u64 births, u64 cells, u64 representatives) noexcept;

// Etage G. Le catalogue DOIT etre celui de index.cloud() (memes SiteIdx). Refus, dans l'ordre des etapes, sans rien
// publier : kmax_out_of_range, cell_capacity, tower_capacity, memory_budget, puis pendant la resolution
// catalogue_missing_ball ((H2) violee), census_mismatch (census et catalogue en desaccord), shell_capacity (coquille
// d'un census au-dela de 64 sites), tower_invariant ; Outcome::order porte l'ordre en cause. Le Pool et le budget sont
// empruntes pendant l'appel ; diagnostics, s'il est donne, n'est rempli qu'au succes.
[[nodiscard]] Result<Resolution> resolve_tower(const GlobalIndex& index, const Catalogue& catalogue,
                                               MemoryBudget& budget, sched::Pool& pool,
                                               ResolutionDiagnostics* diagnostics = nullptr) noexcept;

}  // namespace mhgp12
