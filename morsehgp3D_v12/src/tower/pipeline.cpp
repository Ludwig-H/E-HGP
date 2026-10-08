// Session recouverte de la tour (pipeline.hpp) : build_tower. Ouverture de l'etage G (open_stage : sorties allouees,
// cellules remplies), pre-passe de la foret (octets de T par ordre), admission UNIQUE de tout ce que la region allouera
// (G et foret y coexistent : une admission par etage ne tiendrait pas, MemoryBudget::admit suppose un seul pilote),
// espaces des fils, index des naissances de tous les ordres, region (pipeline_run.cpp), puis cloture : issues dans
// l'ordre de la voie sequentielle, compteurs de G fusionnes et controles par ordre, compteurs et durees de la foret
// fusionnes dans l'ordre des fils, grand livre, diagnostics.
#include <algorithm>

#include "tower/pipeline.hpp"
#include "tower/profile.hpp"

namespace mhgp12 {
namespace tower::detail {
namespace {

// Pre-passe : octets de l'etage T de chaque ordre et sa plus large cohorte de naissances, un ordre par tache.
struct KernelBytes {
  std::span<const ForestInput> inputs;
  std::span<u64> bytes, widest;
  static Outcome body(void* raw, u64 begin, u64 end, u32) noexcept {
    auto& s = *static_cast<KernelBytes*>(raw);
    for (u64 i = begin; i < end; ++i) {
      s.bytes[i] = kernel_bytes(s.inputs[i]);
      s.widest[i] = widest_cohort(s.inputs[i]);
    }
    return {};
  }
};

// Octets de la region : index des naissances de tous les ordres, espaces et compteurs des fils, foret bornee (T exact ;
// M, V et R avec au plus naissances - 1 evenements, donc au plus 2 naissances - 1 noeuds, au plus ne lignes et des
// branches au plus les representants relus), drapeaux des tranches, accumulateurs par (ordre, fil).
u64 region_bytes(const Pipeline& p, std::span<const u64> t_bytes, std::span<const u64> widest_of,
                 u32 sites) noexcept {
  const u64 per_order = sizeof(OrderCounters) + sizeof(SectionCycles) + sizeof(ForestWork) + sizeof(ForestPhysical);
  u64 bytes = tower_detail::workers_bytes(p.threads, sites) + u64{p.threads} * sizeof(WorkerTotals) +
              u64{p.threads} * p.orders * per_order;
  // numerotation par morceaux (levier N) : au plus un tampon de spheres vivant par fil, de la plus large cohorte
  u64 widest = 1;
  for (u32 i = 1; i < p.orders; ++i) widest = std::max(widest, widest_of[i]);
  if (p.orders >= 2) bytes += u64{p.threads} * widest * (sizeof(num::Sphere) + 4);
  for (u32 i = 0; i < p.orders; ++i) {
    const ForestInput& in = p.forest.inputs[i];
    const u64 nb = in.birth_key.size(), ne = nb == 0 ? 0 : nb - 1;
    bytes += t_bytes[i] + contraction_bytes(in, ne) + registry_bytes_bound(in, ne) + p.g_items[i];
    if (i >= 1) bytes += tower_detail::PopulationTable::bytes_for(nb, p.orders) + 4 * (nb + ne);
  }
  return bytes;
}

template <class T>
Outcome zeroed(Buffer<T>& buffer, u64 n, MemoryBudget& budget) noexcept {
  if (n == 0) return {};
  MHGP12_TRY(buffer.allocate(n, budget));
  for (u64 j = 0; j < n; ++j) buffer[j] = T{};
  return {};
}

// Espaces des fils, accumulateurs et drapeaux des tranches (memoire deja admise).
Outcome staff(Pipeline& p, const GlobalIndex& index, MemoryBudget& budget) noexcept {
  const u64 cases = u64{p.orders} * p.threads;
  MHGP12_TRY(tower_detail::staff_workers(index, p.threads, budget, p.team));
  MHGP12_TRY(zeroed(p.g_counters, cases, budget));
  if constexpr (tower_detail::kProfile) MHGP12_TRY(zeroed(p.g_profiles, cases, budget));
  MHGP12_TRY(zeroed(p.f_counters, cases, budget));
  MHGP12_TRY(zeroed(p.f_physical, cases, budget));
  MHGP12_TRY(zeroed(p.totals, p.threads, budget));
  for (u32 i = 0; i < p.orders; ++i) MHGP12_TRY(zeroed(p.g_flags[i], p.g_items[i], budget));
  return {};
}

// Index des naissances des ordres 2 .. K, un objet par ordre, construits ENSEMBLE (T2-d-B2) : chaque phase de la
// construction joue toutes les tables dans une invocation du Pool (PopulationTable::build_all) au lieu d'une suite de
// constructions ordre par ordre (environ dix invocations chacune). Memes tables, meme refus (premier ordre en echec).
// Les phases etant communes, la duree n'est plus separable par ordre : tables_ns est le mur de l'ensemble et
// table_ns[k] reste nul dans la Session (la voie sequentielle, resolve_tower, garde un index par ordre et ses chronos).
Outcome build_tables(Pipeline& p, MemoryBudget& budget, sched::Pool& pool, ResolutionDiagnostics& diag) noexcept {
  if (p.orders < 2) return {};
  const Stopwatch watch;
  std::array<tower_detail::PopulationTable*, kMaxOrder> tables{};
  std::array<tower_detail::PopulationSource, kMaxOrder> sources{};
  const u32 count = p.orders - 1;
  for (u32 i = 0; i < count; ++i) {
    const Order k = static_cast<Order>(i + 2);
    const ResolvedOrder& order = p.out.order(k);
    tables[i] = &p.tables[k - 1];
    sources[i] = tower_detail::PopulationSource{order.birth_keys(), order.birth_ranks(), k};
  }
  using tower_detail::PopulationSource;
  using tower_detail::PopulationTable;
  MHGP12_TRY(PopulationTable::build_all(std::span<PopulationTable* const>(tables.data(), count),
                                        std::span<const PopulationSource>(sources.data(), count), p.domain.catalogue,
                                        budget, pool));
  diag.tables_ns += watch.nanoseconds();
  for (u32 i = 0; i < count; ++i) diag.table_bytes += tables[i]->bytes();
  return {};
}

// Issue de G : refus des tranches, puis controles globaux des ordres dont toutes les tranches ont reussi (fusion :
// le plus petit ordre en echec, comme resolve_orders qui s'arrete au premier).
Outcome close_resolution(Pipeline& p) noexcept {
  Outcome out;
  for (u32 w = 0; w < p.threads; ++w) out = merge(out, p.totals[w].g_outcome);
  for (u32 i = 1; i < p.orders; ++i) {
    bool whole = true;
    for (u64 s = 0; s < p.g_items[i]; ++s) whole = whole && (p.g_flags[i][s] & kSliceDone) != 0;
    if (!whole) continue;
    auto& order = tower_detail::StageAccess::order(p.out, static_cast<Order>(i + 1));
    out = merge(out, tower_detail::close_order(order, p.g_counters.span().subspan(u64{i} * p.threads, p.threads)));
  }
  return out;
}

// Issue de la foret : premier rang de la voie sequentielle en echec ; sinon toute etape doit etre terminee.
Outcome close_forest(const Pipeline& p) noexcept {
  for (u32 rank = 0; rank < kRefusalRanks; ++rank) {
    Outcome out;
    for (u32 w = 0; w < p.threads; ++w) out = merge(out, p.totals[w].forest_outcome[rank]);
    if (!out.ok()) return out;
  }
  for (u32 i = 0; i < p.orders; ++i)
    for (u32 s = 0; s < kStepCount; ++s)
      if (!step_done(p, i, static_cast<Step>(s))) return fail(Reason::tower_invariant);
  return {};
}

void add_physical(ForestPhysical& t, const ForestPhysical& part) noexcept {
  t.births_ns += part.births_ns;
  t.leaves_ns += part.leaves_ns;
  t.kernel_ns += part.kernel_ns;
  t.history_ns += part.history_ns;
  t.contraction_ns += part.contraction_ns;
  t.vertical_ns += part.vertical_ns;
  t.registry_ns += part.registry_ns;
}

// Compteurs et durees des morceaux fusionnes dans l'ordre des fils ; etages du grand livre en temps-fils.
void close_ledger(Pipeline& p, TowerDiagnostics& diag) noexcept {
  BuildState& b = p.forest;
  for (u32 i = 0; i < p.orders; ++i) {
    for (u32 w = 0; w < p.threads; ++w) {
      add_work(b.counters[i], p.f_counters[u64{i} * p.threads + w]);
      add_physical(b.physical[i], p.f_physical[u64{i} * p.threads + w]);
    }
    const ForestPhysical& ph = b.physical[i];
    b.kernels_ns += ph.births_ns + ph.leaves_ns + ph.kernel_ns + ph.history_ns;
    b.contraction_ns += ph.contraction_ns;
    b.vertical_births_ns += ph.vertical_ns;
    b.registry_ns += ph.registry_ns;
  }
  fill_ledger(b, p.threads, diag.forest);
  for (u32 w = 0; w < p.threads; ++w) {
    const WorkerTotals& t = p.totals[w];
    diag.g_thread_ns += t.g_ns;
    diag.forest_thread_ns += t.forest_ns;
    diag.forest_after_g_ns += t.forest_after_g_ns;
    diag.kernel_jobs += t.kernel_jobs;
    diag.kernel_stops += t.kernel_stops;
    diag.hint_jobs += t.hint_jobs;
    diag.hinted_reps += t.hinted;
    diag.hint_jobs_during_g += t.hints_during_g;
  }
  if constexpr (tower_detail::kProfile)
    for (u32 i = 1; i < p.orders; ++i)
      for (u32 w = 0; w < p.threads; ++w)
        for (int s = 0; s < kProfileSections; ++s) {
          diag.resolution.profile[i + 1].cycles[s] += p.g_profiles[u64{i} * p.threads + w].cycles[s];
          diag.resolution.profile[i + 1].count[s] += p.g_profiles[u64{i} * p.threads + w].count[s];
        }
}

}  // namespace

Outcome open_session(SessionRun& r) {  // peut lever std::bad_alloc (pipeline.hpp)
  TowerDiagnostics& diag = r.diag;
  const Stopwatch watch;
  MHGP12_TRY(tower_detail::open_stage(r.index, r.catalogue, r.budget, r.pool, diag.resolution, r.opened));
  r.domain.emplace(tower_detail::Domain{r.index, r.catalogue, r.opened.points.span()});
  Resolution& out = r.opened.out;
  const u32 orders = out.orders();
  for (Order k = 1; k <= orders; ++k) r.inputs[k - 1] = forest_input(out.order(k));
  r.params.slice_events = kSessionSliceEvents;
  r.forest = std::make_unique<BuildState>(r.index.cloud(), r.balls,
                                          std::span<const ForestInput>(r.inputs.data(), orders), r.params, r.budget);
  r.forest->forests.kmax = static_cast<Order>(orders);
  r.pipeline = std::make_unique<Pipeline>(*r.domain, out, *r.forest, r.team, r.pool.size());
  Pipeline& p = *r.pipeline;
  p.orders = orders;
  for (u32 i = 0; i < orders; ++i)
    p.g_items[i] = (u64{out.order(static_cast<Order>(i + 1)).cells()} + tower_detail::kCellGrain - 1) /
                   tower_detail::kCellGrain;
  std::array<u64, kMaxOrder> t_bytes{}, widest{};
  KernelBytes sizes{r.forest->inputs, std::span<u64>(t_bytes.data(), orders), std::span<u64>(widest.data(), orders)};
  MHGP12_TRY(r.pool.parallel_for(orders, 1, &sizes, &KernelBytes::body));
  diag.admitted_bytes = region_bytes(p, std::span<const u64>(t_bytes.data(), orders),
                                     std::span<const u64>(widest.data(), orders), r.index.cloud().sites());
  diag.open_ns = watch.nanoseconds();
  return {};
}

Outcome admit_session(SessionRun& r) noexcept {
  const Stopwatch watch;
  MHGP12_TRY(r.budget.admit(r.diag.admitted_bytes));
  MHGP12_TRY(staff(*r.pipeline, r.index, r.budget));
  r.diag.resolution.workspace_ns = watch.nanoseconds();
  MHGP12_TRY(build_tables(*r.pipeline, r.budget, r.pool, r.diag.resolution));
  prepare_graph(*r.pipeline);
  r.diag.open_ns += watch.nanoseconds();
  return {};
}

Outcome play_session(SessionRun& r) noexcept {
  Pipeline& p = *r.pipeline;
  TowerDiagnostics& diag = r.diag;
  const Stopwatch watch;
  p.start = std::chrono::steady_clock::now();
  MHGP12_TRY(r.pool.parallel_for(p.threads, 1, &p, run_region));
  const u64 g_end = p.g_end_ns.load(std::memory_order_acquire);
  diag.g_end_ns = diag.open_ns + g_end;
  MHGP12_TRY(close_resolution(p));
  MHGP12_TRY(close_forest(p));
  close_ledger(p, diag);
  diag.resolution.resolve_ns = diag.g_thread_ns;
  diag.resolution.threads = p.threads;
  diag.resolution.profiled = tower_detail::kProfile;
  diag.resolution.workspace_bytes = u64{p.threads} * r.index.cloud().sites() * sizeof(SiteIdx);
  diag.resolution.peak_bytes = r.budget.peak();
  diag.threads = p.threads;
  for (u32 i = 0; i < p.orders; ++i) {
    diag.order_g_end_ns[i] = diag.open_ns + p.order_g_end[i].load(std::memory_order_relaxed);
    diag.order_kernel_end_ns[i] = diag.open_ns + p.order_kernel_end[i].load(std::memory_order_relaxed);
    diag.order_m_end_ns[i] = diag.open_ns + p.order_m_end[i].load(std::memory_order_relaxed);
    diag.order_v_end_ns[i] = i == 0 ? 0 : diag.open_ns + p.order_v_end[i].load(std::memory_order_relaxed);
    diag.order_r_end_ns[i] = diag.open_ns + p.order_r_end[i].load(std::memory_order_relaxed);
  }
  diag.end_ns = diag.open_ns + watch.nanoseconds();
  return {};
}

}  // namespace tower::detail

Result<Tower> build_tower(const GlobalIndex& index, const Catalogue& catalogue, MemoryBudget& budget, sched::Pool& pool,
                          TowerDiagnostics* diagnostics) noexcept {
  return guarded([&]() -> Result<Tower> {
    auto run = std::make_unique<tower::detail::SessionRun>(index, catalogue, budget, pool);
    MHGP12_TRY(tower::detail::open_session(*run));
    MHGP12_TRY(tower::detail::admit_session(*run));
    MHGP12_TRY(tower::detail::play_session(*run));
    if (diagnostics != nullptr) *diagnostics = run->diag;
    return Tower{std::move(run->opened.out), std::move(run->forest->forests)};
  });
}

}  // namespace mhgp12
