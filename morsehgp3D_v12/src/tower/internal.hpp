// Declarations internes du module tower (etage G) : parties, supports et certificats exacts, cellules de fenetre,
// table de populations (LEM-POP), resolveur. Aucun autre module ne l'inclut.
#pragma once

#include <algorithm>
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
  static const Buffer<u64>& window_offsets(const Resolution& r) noexcept { return r.window_offsets_; }
  static const Buffer<u32>& window_targets(const Resolution& r) noexcept { return r.window_targets_; }
  static void set_orders(Resolution& r, Order kmax, Order orders) noexcept {
    r.kmax_ = kmax;
    r.orders_count_ = orders;
  }
};

// Cible de la cellule (b, k) lue sans le tableau des premiers ordres : la fenetre de b commence a p + q_min - 1 et finit
// a min(p + m, ordres) (meme valeur que Resolution::window_target) ; kNoTarget hors de la fenetre.
inline u32 window_target_of(const Resolution& r, u32 b, const CatalogueBall& data, Order k) noexcept {
  const u32 lo = data.p + data.qmin - 1, hi = std::min<u32>(data.p + data.m, r.orders());
  if (k < lo || k > hi) return kNoTarget;
  return StageAccess::window_targets(r)[StageAccess::window_offsets(r)[b] + (k - lo)];
}

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

// ---- radix.cpp : tri par base ----------------------------------------------------------------------------------------
// Entree de tri de l'etage G : cle de 64 bits, indice et donnee annexe (16 octets).
struct KeyedEntry {
  u64 key;
  u32 index;
  u32 aux;
};
// Tri par base STABLE des entrees selon key >> low_bit (low_bit <= 64 ; 64 : rien a trier), chiffres de 8 bits, sur
// le Pool ; places fixees par les donnees seules (memes octets a tout nombre de fils). data et scratch : meme taille ;
// rend celui des deux qui porte le resultat. places : tampon de travail de l'appelant (grandit si besoin, gardee d'un
// appel a l'autre), au plus radix_bytes(data.size()).
[[nodiscard]] Result<std::span<KeyedEntry>> radix_sort(std::span<KeyedEntry> data, std::span<KeyedEntry> scratch,
                                                       u32 low_bit, Buffer<u64>& places, MemoryBudget& budget,
                                                       sched::Pool& pool) noexcept;
u64 radix_bytes(u64 entries) noexcept;

// ---- populations.cpp : index des naissances de l'ordre k (LEM-POP) ---------------------------------------------------
// Empreinte additive d'un site (melange splitmix64 du SiteIdx) ; celle d'une partie est la somme de celles de ses sites
// modulo 2^64, masquee par le masque de la table (les portes provoquent des collisions par un masque faible).
// Adressage seulement : toute reponse exige l'egalite exacte des SiteIdx.
inline u64 site_key(u32 site) noexcept {
  u64 z = u64{site} + 0x9E3779B97F4A7C15ull;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  return z ^ (z >> 31);
}

// Naissance trouvee : indice de naissance de l'ordre et rang de sa boule (controle de decroissance).
struct PopulationHit {
  u32 birth;
  LevelRank rank;
};

// Naissances (b, k) de population exacte k (p + m = k) dans l'ordre canonique (empreinte, population) ; repertoire des
// seaux (bits de tete de l'empreinte) ; une fiche contigue par place : empreinte, naissance, rang, population
// (populations.cpp). Un meme objet sert a tous les ordres : ses tampons grandissent et sont gardes.
class PopulationTable {
 public:
  // Construction parallele et deterministe pour l'ordre k ; refus tower_invariant (population de taille differente de
  // k, ordre canonique viole, population repetee), memory_budget. key_mask : ~0 dans le produit.
  [[nodiscard]] Outcome build(const Catalogue& catalogue, std::span<const u32> birth_keys,
                              std::span<const LevelRank> birth_ranks, Order k, MemoryBudget& budget, sched::Pool& pool,
                              u64 key_mask = ~u64{0}) noexcept;
  // Empreinte de la partie F (k SiteIdx croissants).
  u64 key_of(const Part& f) const noexcept;
  u64 key_mask() const noexcept { return key_mask_; }
  u64 bucket_of(u64 key) const noexcept { return shift_ >= 64 ? 0 : key >> shift_; }
  u32 shift() const noexcept { return shift_; }
  // Naissance de population exactement F (d'empreinte key), ou rien : dichotomie dans le seau.
  std::optional<PopulationHit> find(const Part& f, u64 key) const noexcept;
  std::optional<PopulationHit> find(const Part& f) const noexcept { return find(f, key_of(f)); }
  // Premiere place d'empreinte key, ou kNone (premieres sondes en masse, G-L5).
  u32 candidate(u64 key) const noexcept;
  // Verification exacte d'un candidat pour F : sa fiche, puis (collision d'empreintes) dichotomie dans le seau.
  std::optional<PopulationHit> verify(u32 candidate, const Part& f) const noexcept;
  void prefetch(u32 candidate) const noexcept;
  // File de sondes (G-L7) : case du repertoire du seau de key, puis premiere fiche de ce seau (lecture de la case).
  void prefetch_directory(u64 key) const noexcept;
  void prefetch_bucket(u64 key) const noexcept;
  u64 entries() const noexcept { return entries_; }
  // Octets tenus (tampons gardes, construction comprise).
  u64 bytes() const noexcept {
    return records_.size() * 4 + directory_.size() * 4 + (made_.size() + scratch_.size()) * sizeof(KeyedEntry) +
           dense_rows_.size() * 4 + (block_start_.size() + places_.size()) * 8;
  }
  // Octets de la table et de sa construction pour E entrees (admission, borne superieure).
  static u64 bytes_for(u64 entries, Order k) noexcept;

 private:
  static constexpr u32 kHeaderWords = 4;  // fiche : empreinte (deux mots), naissance, rang, puis k SiteIdx
  Outcome lay_out(std::span<KeyedEntry> sorted, std::span<const LevelRank> birth_ranks, sched::Pool& pool) noexcept;
  const u32* record(u64 pos) const noexcept { return records_.data() + pos * width_; }
  u64 key_at(u64 pos) const noexcept { return u64{record(pos)[0]} | (u64{record(pos)[1]} << 32); }
  int compare_at(u64 pos, u64 key, const Part& f) const noexcept;
  u64 lower_bound(u64 lo, u64 hi, u64 key, const Part* f) const noexcept;
  PopulationHit hit_at(u64 pos) const noexcept { return {record(pos)[2], make_id<LevelRank>(record(pos)[3])}; }
  Buffer<u32> records_;    // fiches de width_ mots (capacite)
  Buffer<u32> directory_;  // 2^d + 1 premieres places des seaux (capacite)
  Buffer<KeyedEntry> made_, scratch_;
  Buffer<u32> dense_rows_;
  Buffer<u64> block_start_, places_;
  u64 entries_ = 0, directory_size_ = 0, key_mask_ = ~u64{0};
  u32 shift_ = 64, width_ = 0;
  Order k_ = 0;
};

// ---- first_probes.cpp : premieres sondes en masse par jointure triee (G-L5) -------------------------------------------
// Tampons de la jointure, gardes d'un ordre a l'autre (croissance seulement).
struct JoinBuffers {
  Buffer<KeyedEntry> entries, scratch;
  Buffer<u64> places;
  Buffer<u32> candidates;
  u64 bytes() const noexcept {
    return (entries.size() + scratch.size()) * sizeof(KeyedEntry) + places.size() * 8 + candidates.size() * 4;
  }
};
// candidates[r] (les R premieres cases de buffers.candidates) = premiere place de l'index d'empreinte egale a celle de
// la trace du representant r de l'ordre k, kNone sinon : empreintes des traces (une passe sur les cellules), tri par
// base des traces par seau, puis recherche par dichotomie dans le seau (bornee sous collisions). Aucune decision : la
// resolution verifie chaque candidat sur les SiteIdx (verify).
[[nodiscard]] Outcome first_probe_candidates(const Domain& d, const ResolvedOrder& order,
                                             const PopulationTable& table, JoinBuffers& buffers,
                                             MemoryBudget& budget, sched::Pool& pool) noexcept;
u64 first_probe_bytes(u64 representatives) noexcept;

// ---- resolve.cpp : resolution d'un representant ------------------------------------------------------------------------
struct OrderView {
  Order k = 0;
  std::span<const u32> birth_keys;
  const PopulationTable* table = nullptr;
};
// Premiere sonde deja jouee par les sondes en masse (G-L5) : done, et la naissance verifiee s'il y en a une.
struct FirstProbe {
  bool done = false;
  std::optional<PopulationHit> hit;
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
// Meme resolution, sections imputees au chronometre du fil (profile.hpp ; vide hors MHGP12_TOWER_PROFILE) ; si
// first.done, la premiere sonde n'est pas rejouee : son resultat est first.hit (memes compteurs et memes controles).
class SectionClock;
[[nodiscard]] Result<u32> resolve_part(const ResolveContext& c, Part f, LevelRank junction_rank,
                                       CensusWorkspace& workspace, OrderCounters& counters, SectionClock& clock,
                                       const FirstProbe& first) noexcept;
// Fusion d'un compteur de fil dans le total (sommes et maxima).
void add_counters(OrderCounters& total, const OrderCounters& part) noexcept;

}  // namespace mhgp12::tower_detail
