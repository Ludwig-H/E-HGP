// Etat du constructeur FULL, interne au module tower ; capacites bornees et tous tableaux budgetes.
#pragma once
#include <atomic>
#include "tower/forest.hpp"

namespace mhgp11::sched {
class Pool;
}

namespace mhgp11::tower_detail {
class RegularVerticalSeeds;
class PopulationLookup;
class SeedLog;

template<class T, class Less>
void forest_sort(std::span<T> values, Less less) noexcept {
  const auto sift = [&](u64 root, u64 count) noexcept {
    while (root < count / 2) {
      u64 child = 2 * root + 1;
      if (child + 1 < count && less(values[child], values[child + 1])) ++child;
      if (!less(values[root], values[child])) break;
      std::swap(values[root], values[child]); root = child;
    }
  };
  for (u64 i = values.size() / 2; i != 0; --i) sift(i - 1, values.size());
  for (u64 end = values.size(); end > 1; --end) {
    std::swap(values[0], values[end - 1]); sift(0, end - 1);
  }
}

// Pipeline des ordres concurrents (forest_pipeline.cpp). Etat publie d'une foret pendant sa publication : noeuds
// [0,nodes) complets, enfants compris ; closed = dernier rang clos + 1 (0 : aucun), toute fusion future a un rang
// >= closed ; done : publication complete ; abandoned : refus ici ou en amont, les lecteurs rendent sans resultat.
// Ecrit par la seule tache de publication (release : nodes, puis done/abandoned, puis closed), lu par les balayages
// (acquire : closed, puis done, abandoned, nodes). closed change a chaque annonce et vaut kNone a la fin ou a
// l'abandon : les balayages attendent ses changements (std::atomic::wait), la publication les notifie.
struct ForestProgress {
  std::atomic<u32> nodes{0}, closed{0};
  std::atomic<bool> done{false}, abandoned{false};
  void finish(u32 count, bool complete) noexcept {
    nodes.store(count, std::memory_order_release);
    (complete ? done : abandoned).store(true, std::memory_order_release);
    closed.store(kNone, std::memory_order_release);
    closed.notify_all();
  }
};
// Blocs de cellules regulieres d'un ordre : 0 en attente, 1 graines ecrites, 2 un refus dans le bloc (la publication
// de l'ordre abandonne ; le refus est rendu par sa tache de resolution). Octets lus/ecrits par std::atomic_ref ;
// chaque bloc termine incremente epoch (release) et la notifie : la publication attend ses changements.
struct JobGate {
  std::span<u8> state;
  u32 width = 1;  // cellules par bloc
  std::atomic<u32>* epoch = nullptr;
};
// Decision pure du balayage suivi (porte deterministe) : prochaine entree du flux haut d'apres l'etat publie lu.
// Fusion publiee : ordre de sweep_all (naissance d'abord a rang egal). Sinon une naissance passe si la
// publication est finie ou si son rang <= closed (toute fusion future a un rang >= closed) ; fin quand tout est lu.
enum class FollowStep : u8 { birth, merge, wait, end };
inline FollowStep follow_step(bool birth_left, LevelRank birth_rank, bool merge_published, LevelRank merge_rank,
                              bool done, u32 closed) noexcept {
  if (merge_published)
    return birth_left && idx(birth_rank) <= idx(merge_rank) ? FollowStep::birth : FollowStep::merge;
  if (birth_left && (done || idx(birth_rank) <= closed)) return FollowStep::birth;
  if (!birth_left && done) return FollowStep::end;
  return FollowStep::wait;
}
// Les fusions basses de rang <= level sont toutes publiees : publication finie, ou dernier rang clos >= level.
inline bool follow_lower_ready(LevelRank level, bool done, u32 closed) noexcept { return done || idx(level) < closed; }
// Vue lue par un balayage suivi : closed d'abord (kNone lu implique done ou abandoned visibles, donc jamais d'attente
// sur un closed fige), puis done, abandoned, nodes ; block() attend un changement de closed puis relit tout.
struct ProgressView {
  const ForestProgress& state;
  u32 nodes = 0, closed = 0;
  bool done = false, abandoned = false;
  u64* wait_ns = nullptr;  // diagnostic : attente bloquee cumulee, mur, si non nul
  void refresh() noexcept {
    closed = state.closed.load(std::memory_order_acquire);
    done = state.done.load(std::memory_order_acquire);
    abandoned = state.abandoned.load(std::memory_order_acquire);
    nodes = state.nodes.load(std::memory_order_acquire);
  }
  void block() noexcept {
    if (wait_ns == nullptr) {
      state.closed.wait(closed, std::memory_order_acquire);
    } else {
      const Stopwatch clock;
      state.closed.wait(closed, std::memory_order_acquire);
      *wait_ns += clock.nanoseconds();
    }
    refresh();
  }
};
// Attente de l'ordre bas jusqu'a ce que le niveau soit clos ; faux si sa publication abandonne. Un abandon publie
// closed = kNone, qui rend le predicat de pret vrai : la garde doit donc aussi suivre la boucle, sinon un abandon
// survenu pendant l'attente laisse lire un ordre dont la graine reguliere n'est plus garantie publiee (audit P1 du
// 4 octobre 2026). Gabarit : la porte le joue sur une vue scriptee et sur un vrai ForestProgress.
template <class View>
bool await_lower(View& low, LevelRank level) noexcept {
  while (!follow_lower_ready(level, low.done, low.closed)) {
    if (low.abandoned) return false;
    low.block();
  }
  return !low.abandoned;
}

// Etat de chaine d'une naissance ; son parent DSU est tenu a part (ForestBuilder::parents) : find ne lit que ce
// tableau dense de u32, sans les champs de chaine. Octets par naissance : kForestStateBytes (etat, parent, touched).
// Naissance d'une cohorte en cours de tri : sphere exacte, cle et rang.
struct BirthRecord { num::Sphere sphere; u32 key; LevelRank rank; };
// Tranches de cohortes par ordre dans les naissances paralleles (phase 2) : les cohortes sont independantes.
inline constexpr u64 kCohortChunks = 32;

struct ForestState {
  u32 top, head, tail, next;
  bool touched;
};
inline constexpr u64 kForestStateBytes = sizeof(ForestState) + 2 * sizeof(u32);
// Series de naissances de meme rang dans une plage de boules (rang croissant avec l'indice) : plus longue serie,
// series de tete et de queue. Composition associative dans l'ordre des plages : meme resultat qu'un parcours unique.
struct BirthRuns {
  u64 largest = 0, head = 0, tail = 0;
  LevelRank head_rank{0}, tail_rank{0};
  bool any = false, uniform = false;  // uniform : toutes les naissances de la plage ont le meme rang
  void add(LevelRank rank) noexcept;
  void append(const BirthRuns& next) noexcept;
};
// Compteurs d'une plage de classification ; leur somme ne depend pas du decoupage.
struct ClassifyCounts {
  u64 classified = 0, births = 0, regular_jobs = 0;
  ClassificationLedger classification;
  BirthRuns runs;
};
[[nodiscard]] Outcome classify_range(const FullDomain&, u32 k, std::span<u8> kinds, u32 begin, u32 end,
                                     ClassifyCounts&) noexcept;
[[nodiscard]] Outcome add_classify_counts(ClassifyCounts& sum, const ClassifyCounts& part) noexcept;
// Blocs de boules de la classification concurrente d'un ordre : debuts des naissances et des cellules regulieres
// (sommes prefixes ; births[count] et jobs[count] sont les totaux). Base des naissances par blocs.
inline constexpr u64 kClassifyChunks = 256;
struct BirthBlocks {
  u64 width = 1, count = 0;
  std::array<u64, kClassifyChunks + 1> births{}, jobs{};
};
struct ForestBuilder {
  const FullDomain& domain;
  u32 k;
  MemoryBudget& budget;
  OrderTimings* timings;
  DescentMemo* memo;
  ForestParallel* parallel;
  bool dense_birth_lookup;
  RegularVerticalSeeds* vertical_seeds;
  const PopulationLookup* population = nullptr;  // facultatif ; aucune decision du DSU n'en depend
  OrderTimings* parallel_timings = nullptr;
  OrderForest result;
  Buffer<u8> kinds;  // 0 hors fenetre, 1 naissance, 2 traces strictes ; B octets.
  Buffer<u32> parents;  // DSU des naissances ; racine = plus petite naissance canonique de la composante
  Buffer<ForestState> states;
  Buffer<u32> touched;
  u32 touched_count = 0;
  u64 regular_jobs = 0;  // cellules regulieres de jonction (kinds=2, m=qmin) ; voie des ordres concurrents
  u64 birth_runs = 0;    // plus longue serie de naissances de meme rang si >1, sinon 0 (classification)
  CensusWorkspace* extended_scratch = nullptr;  // espace census des cellules etendues, voie concurrente
  // Pipeline : avancement publie pour les balayages, blocs de graines attendus ; abandoned : bloc en refus.
  ForestProgress* progress = nullptr;
  const JobGate* gate = nullptr;
  u64 confirmed = 0;  // blocs [0,confirmed) deja vus resolus
  u64* wait_ns = nullptr;  // diagnostic : attente bloquee cumulee des blocs, mur, si non nul
  // Profil echantillonne du publieur (pipeline chronometre, diagnostic du 7 octobre) : une cellule reguliere et une
  // cloture de plateau sur kPublishSample sont chronometrees ; comptes complets. Aucune decision n'en depend.
  bool sample_publish = false;
  u64 cells_seen = 0, closes_seen = 0, cell_sample_ns = 0, close_sample_ns = 0;
  u64 sample_clock_ns = 0;  // plus court intervalle vide mesure : cout d'une lecture d'horloge, a retrancher
  u32 unannounced = 0;
  bool abandoned = false;
  // Journal des graines (build_order, ou build_order_full sur l'ordre K de FULL : OrderLog ; seed_log.hpp), nul partout
  // ailleurs : cell et regular_cell y consignent chaque graine de naissance, jamais une racine ni un top. Aucune
  // decision du DSU n'en depend.
  SeedLog* seed_log = nullptr;

  ForestBuilder(const FullDomain& d, u32 order, MemoryBudget& b, OrderTimings* t = nullptr,
                DescentMemo* m = nullptr, ForestParallel* p = nullptr, bool dense = false,
                RegularVerticalSeeds* seeds = nullptr) noexcept
      : domain(d), k(order), budget(b), timings(t), memo(m), parallel(p), dense_birth_lookup(dense),
        vertical_seeds(seeds) {}
  Result<OrderForest> run() noexcept;
  Outcome classify() noexcept;
  Outcome adopt(const ClassifyCounts&) noexcept;
  u64 birth_bytes() const noexcept;  // octets que births() reservera ; admission prealable des taches
  Outcome births() noexcept;
  Outcome allocate_births() noexcept;  // allocations de births() et prepare_states(), sans remplissage
  // Etapes des naissances par blocs (parallel_births) ; chacune ecrit des cases disjointes de cet ordre.
  Outcome site_births() noexcept;                                               // ordre 1, une tache
  Outcome birth_block(const BirthBlocks&, u64 block, std::span<BallIdx> jobs) noexcept;
  // Ordre >= 2, tri des cohortes par tranches : bornes alignees sur les cohortes (lecture seule, par le pilote), puis
  // tranche [first, last) avec le tampon de l'ouvrier et le registre de la tranche, puis registres ajoutes.
  Outcome cohort_bounds(std::span<u64> bounds) const noexcept;
  Outcome birth_cohort_chunk(u64 first, u64 last, std::span<BirthRecord> scratch, ForestLedger& work) noexcept;
  Outcome add_cohort_ledgers(std::span<const ForestLedger> chunks) noexcept;
  Outcome birth_dense(const BirthBlocks&, u64 block) noexcept;
  void births_done() noexcept;
  Outcome prepare_states() noexcept;
  Outcome plateaus() noexcept;
  Outcome finish() noexcept;
  // Voie des ordres concurrents (forest_concurrent.cpp) : graines regulieres deja resolues par ordinal.
  Outcome collect_jobs(std::span<BallIdx> jobs) const noexcept;
  Outcome publish(std::span<const BallIdx> jobs, std::span<const NodeIdx> seeds) noexcept;
  bool await_job(u64 job) noexcept;      // pipeline : false si le bloc porte un refus
  void announce(LevelRank closed, bool last) noexcept;
  Outcome sampled_close(LevelRank level) noexcept;  // close, chronometree une fois sur kPublishSample si sample_publish
  Outcome cell(BallIdx) noexcept;
  Outcome regular_cell(BallIdx, std::span<const NodeIdx>) noexcept;
  // Prechargement des parents et etats DSU des graines du job donne : aucun effet sur une decision. Seulement dans
  // un bloc deja confirme (lecture acquise par await_job) ou hors pipeline, ou aucun resolveur n'ecrit en meme temps.
  void prefetch_seeds(std::span<const NodeIdx> seeds, u64 job) const noexcept;
  Outcome regular_work(const DescentLedger& work) noexcept { return add_descent(result.ledger_.descent, work); }
  Outcome regular_plateau() noexcept { return cell_add(result.ledger_.plateaus, 1); }
  Outcome close(LevelRank) noexcept;
  u32 find(u32) noexcept;
  Outcome touch(u32) noexcept;
  Outcome unite_roots(u32& root, u32 other) noexcept;
};

[[nodiscard]] Outcome add_cell_work(CellLedger&, const CellLedger&) noexcept;
// Journal des graines demande a FULL (livraison L2b, docs/SORTIES.md paragraphe 11) : pose sur le seul constructeur de
// l'ordre `order` (log nul : aucun journal, FULL inchange). Le journal n'est ecrit que par cell et regular_cell, que
// seul le fil qui applique les cellules de cet ordre appelle : le pilote dans la voie non concurrente
// (forest_vertical.cpp ; ForestParallel::flush applique les lots sur le pilote apres le join des descentes), la tache
// de publication unique de l'ordre dans la voie concurrente (PublishTasks, forest_concurrent.cpp, ou Pipeline::publish,
// forest_pipeline.cpp). Jamais lu pendant la construction : le balayage le lit apres le retour (join du Pool).
struct OrderLog {
  SeedLog* log = nullptr;
  Order order = 0;
};
inline SeedLog* order_log(const OrderLog& wanted, u32 k) noexcept { return k == wanted.order ? wanted.log : nullptr; }
// Corps de build_full sur un domaine EMPRUNTE (forest_vertical.cpp) : forets et verticales 1..kmax dans orders,
// contextes rendus au retour, diagnostics publies au succes seulement ; build_full et build_order_full le partagent.
// Precondition : 1 <= kmax <= n et ForestParallel::validate(params, pool) deja verifies par l'appelant.
[[nodiscard]] Outcome build_forests(const FullDomain&, Order kmax, MemoryBudget&, FullTimings*, const FullParams&,
                                    sched::Pool*, OrderLog,
                                    std::array<std::optional<OrderForest>, kMaxMebSites>& orders) noexcept;
// Verticales d'une foret (images basses) rendues a leur budget : l'arbre d'ordre K tire de FULL n'en garde aucune.
void drop_verticals(OrderForest&) noexcept;
// Voie des ordres concurrents (forest_concurrent.cpp), Pool obligatoire : forets de 1..kmax dans orders ; journal
// des graines sur l'ordre demande (OrderLog), ecrit par sa seule tache de publication.
[[nodiscard]] Outcome build_concurrent(const FullDomain&, Order kmax, MemoryBudget&, FullTimings*, ForestParallel&,
                                       sched::Pool&, RegularVerticalSeeds*, PopulationLookup*, bool dense,
                                       std::array<std::optional<OrderForest>, kMaxMebSites>& orders,
                                       OrderLog log = {}) noexcept;
// Images basses de chaque ordre (Pool, ordre apres ordre) puis K-1 balayages fermes concurrents.
[[nodiscard]] Outcome concurrent_verticals(const FullDomain&, std::span<OrderForest* const> forests, MemoryBudget&,
                                           ForestParallel&, sched::Pool&, std::span<OrderTimings> times,
                                           const RegularVerticalSeeds*, const PopulationLookup*) noexcept;
// Naissances par blocs (ordres concurrents, lookup dense ; forest_build.cpp) : memes noeuds, meme ordre canonique
// (cohortes de meme rang triees par centre), memes etats DSU, memes listes de cellules regulieres et meme table dense
// que births(), prepare_states() et collect_jobs(). Allocations faites par allocate_births ; jobs[i] dimensionne. Le
// tampon du tri des cohortes (un par ouvrier) et les registres de tranche sont admis et alloues ici, dans budget.
[[nodiscard]] Outcome parallel_births(std::span<ForestBuilder* const> builders, std::span<const BirthBlocks> blocks,
                                      std::span<const std::span<BallIdx>> jobs, MemoryBudget& budget,
                                      sched::Pool& pool) noexcept;
// Pipeline des ordres concurrents (forest_pipeline.cpp). pipeline_lanes : taches de resolution, 0 si la voie par
// etages s'impose (memo de lane, K<2, W<2K ou moins de 2K espaces census) ; alors aucun etat n'est touche.
class ClosedAncestorSweep;
[[nodiscard]] u32 pipeline_lanes(const ForestParallel&, const sched::Pool&, u32 kmax, bool memo) noexcept;
[[nodiscard]] Outcome pipeline_orders(const FullDomain&, MemoryBudget&, ForestParallel&, sched::Pool&,
                                      RegularVerticalSeeds*, const PopulationLookup*,
                                      std::span<ForestBuilder* const> builders,
                                      std::span<const std::span<const BallIdx>> jobs,
                                      std::span<const std::span<NodeIdx>> seeds, u32 lanes, FullTimings*) noexcept;
// Verticales du pipeline (forest_vertical.cpp) : images basses allouees, balayage suivi, travail ajoute apres join.
[[nodiscard]] Outcome allocate_verticals(const OrderForest& lower, OrderForest& upper, MemoryBudget&) noexcept;
[[nodiscard]] Outcome follow_verticals(const FullDomain&, const OrderForest& lower, OrderForest& upper, MemoryBudget&,
                                       ClosedAncestorSweep&, ForestParallel&, const RegularVerticalSeeds*,
                                       const PopulationLookup*, CensusWorkspace*, const ForestProgress& low,
                                       const ForestProgress& up, ForestLedger& work, u64* wait_ns = nullptr) noexcept;
[[nodiscard]] Outcome add_vertical_work(OrderForest& upper, const ForestLedger& work) noexcept;
[[nodiscard]] Outcome forest_verticals(const FullDomain&, const OrderForest&, OrderForest&, MemoryBudget&,
                                      DescentMemo* = nullptr, ForestParallel* = nullptr,
                                      OrderTimings* = nullptr, const RegularVerticalSeeds* = nullptr,
                                      const PopulationLookup* = nullptr) noexcept;

}  // namespace mhgp11::tower_detail
