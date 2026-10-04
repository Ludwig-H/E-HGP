// Etats internes comptes, limites et contrats du catalogue sequentiel ; aucun etat global modifiable.
#pragma once

#include <algorithm>
#include <atomic>
#include <limits>

#include "catalogue/catalogue.hpp"

namespace mhgp11::catalogue_detail {

inline constexpr u32 kMaxLeaf = 1024;
inline constexpr u32 kMaxOrder = 12;
inline constexpr u32 kMaxWords = (kMaxLeaf + 63) / 64;
inline constexpr u32 kMaxDepth = 3 * kCoordBits;

// Transactionnel, egalement jugeable aux limites sans fabriquer de tableau geant.
inline Outcome checked_add(u64& value, u64 amount) noexcept {
  if (amount > std::numeric_limits<u64>::max() - value) return fail(Reason::catalogue_counter_overflow);
  value += amount;
  return {};
}

// Les tailles se gardent AVANT multiplication ; les octets simultanes s'additionnent sans debordement.
template <class T>
Outcome add_bytes(u64& bytes, u64 count) noexcept {
  if (count > Buffer<T>::kMaxCount) return fail(Reason::memory_budget);
  const u64 term = count * sizeof(T);
  if (term > std::numeric_limits<u64>::max() - bytes) return fail(Reason::memory_budget);
  bytes += term;
  return {};
}

struct Box {
  std::array<i64, 3> lo{}, hi{};  // T0 : 0<=lo<hi<=2^B, intervalles demi-ouverts
};

struct Emission {
  CatalogueBall ball;
  num::Level level;
  u64 population_begin = 0;
};

struct Workspace {
  Buffer<num::Point> points;
  Buffer<u64> dominance;
  Buffer<SiteIdx> interior, shell;
  Buffer<u8> center_lines;
  Buffer<u64> pair_rows;
  Outcome allocate(u32 capacity, MemoryBudget& budget, bool cache_center_lines = false,
                   bool pair_graph = false) noexcept;
};
Outcome workspace_memory_bound(u32 capacity, u32 workers, bool cache_center_lines, u64& bytes,
                               bool pair_graph = false) noexcept;

// Mode comptage : spans vides, filling=false. Mode remplissage : capacites EXACTES de la premiere passe.
// Les sommes sont controlees avant toute ecriture. Pas de publication depuis accept().
class SinglePassOutput;
struct Collector {
  bool filling = false;
  std::span<Emission> records;
  std::span<SiteIdx> population;
  u64 balls = 0, incidences = 0;
  SinglePassOutput* stream = nullptr;
  MemoryBudget* stream_budget = nullptr;
  Outcome accept(const CatalogueBall& ball, const num::Level& level, std::span<const SiteIdx> interior,
                 std::span<const SiteIdx> shell, const CatalogueParams& params) noexcept;
};

// Quota partage du preambule et des suffixes d'UNE passe. Une visite est admise avant tout travail de noeud.
// Avec limit==0, pas d'atomique sur le chemin illimite. Un refus ne publie jamais de ledger partiel.
class NodeQuota {
 public:
  explicit NodeQuota(u64 limit) noexcept : limit_(limit) {}
  u64 limit() const noexcept { return limit_; }
  Outcome claim() noexcept;

 private:
  const u64 limit_;
  std::atomic<u64> claimed_{0};
};

struct Run {
  const Cloud& cloud;
  const CatalogueParams& params;
  MemoryBudget& budget;
  Workspace& workspace;
  Collector& collector;
  CatalogueLedger ledger;
  NodeQuota* quota = nullptr;  // optionnel pour conserver le chemin sequentiel et ses agregats
};

// Noeud deja filtre/ajuste ; sa visite appartient au preambule. Le suffixe commence au choix feuille/coupe.
// storage.size() est la capacite parent payee par filter ; seuls les count premiers SiteIdx sont initialises.
struct ReadyNode {
  Buffer<SiteIdx> storage;
  u32 count = 0, depth = 0;
  Box box;
  std::span<const SiteIdx> sites() const noexcept { return storage.span().first(count); }
};

Outcome make_root(Run& run, Buffer<SiteIdx>& root, Box& box) noexcept;
Outcome prepare_node(Run& run, std::span<const SiteIdx> parent, const Box& box, u32 depth,
                     ReadyNode& ready) noexcept;
bool split_ready(const ReadyNode& ready, const CatalogueParams& params, Box& left, Box& right) noexcept;
Outcome run_ready(Run& run, const ReadyNode& ready) noexcept;

// G1 : retire seulement les sites possedant K dominateurs STRICTS distincts sur la fermeture de box.
Outcome walk(Run& run) noexcept;
// G3/G4 : enumeration des seuls supports de la feuille ; aucun parcours global de quadruplets.
Outcome enumerate_leaf(Run& run, std::span<const SiteIdx> sites, const Box& box) noexcept;
// Coquille complete triee ; donne le plus petit support minimal strictement positif ou un refus d'invariant.
Result<std::array<SiteIdx, 4>> canonical_support(const Cloud& cloud, std::span<const SiteIdx> shell,
                                               const num::Sphere& sphere, u8& qmin) noexcept;
Result<std::array<SiteIdx, 4>> canonical_support(const Cloud& cloud, std::span<const SiteIdx> shell,
                                               const num::Q3Candidate& sphere, u8& qmin) noexcept;
Result<std::array<SiteIdx, 4>> canonical_support(const Cloud& cloud, std::span<const SiteIdx> shell,
                                               const num::Q4Candidate& sphere, u8& qmin) noexcept;
Result<num::Point> point(const Cloud& cloud, SiteIdx site) noexcept;
// 0<=a,lo,hi<=2^B et budgets de Sphere. |N+D(a-lo)| <48*2^(5B), soit 5B+6<=126 en B24.
bool center_in_box(const num::Sphere& sphere, const Box& box) noexcept;
bool center_in_box(const num::Q3Candidate& sphere, const Box& box) noexcept;
bool center_in_box(const num::Q4Candidate& sphere, const Box& box) noexcept;

struct Assembly {
  static Result<Catalogue> build(const Cloud& cloud, const CatalogueParams& params, MemoryBudget& budget) noexcept;
  static Result<Catalogue> finish(Buffer<Emission>& records, Buffer<SiteIdx>& population,
                                  const CatalogueParams& params, const CatalogueLedger& ledger,
                                  MemoryBudget& budget, CatalogueTimings* timings = nullptr, sched::Pool* pool = nullptr,
                                  const CatalogueExecution* execution = nullptr) noexcept;
};

}  // namespace mhgp11::catalogue_detail
