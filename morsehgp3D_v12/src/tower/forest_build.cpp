// Orchestration etage par etage des etages T, M et V (forest.hpp) sur le Pool (build_forests ; la Session recouverte
// rejoue les memes etapes par ordre, pipeline.cpp) : controle des entrees, admission memoire par etage, puis trois
// regions paralleles. T : une tache par ordre (numerotation des naissances, noyau, historique : le proprietaire de
// l'ordre, sequentiel). M : quatre phases sur les tranches de tous les ordres, sommes prefixes entre les phases par le
// pilote. V : naissances de tous les ordres k >= 2 par morceaux, puis fusions. Toutes les sorties sont a des positions
// fixees par l'entree ou par les sommes prefixes : rien ne depend de l'entrelacement des fils.
#include <algorithm>
#include <memory>

#include "sched/sched.hpp"
#include "tower/forest_internal.hpp"

namespace mhgp12::tower {

Outcome check_forest_input(const ForestInput& in) noexcept {
  const u64 nb = in.birth_key.size(), nc = in.cell_ball.size();
  if (!operand_domain(nb) || !operand_domain(nc)) return fail(Reason::tower_capacity);
  if (in.k < 1 || in.k > kMaxOrder || nb == 0 || in.birth_rank.size() != nb || in.cell_rank.size() != nc ||
      in.rep_offsets.size() != nc + 1 || in.rep_offsets[0] != 0 || in.rep_offsets[nc] != in.targets.size())
    return fail(Reason::tower_invariant);
  for (u64 i = 1; i < nb; ++i)
    if (in.birth_key[i] <= in.birth_key[i - 1] || in.birth_rank[i] < in.birth_rank[i - 1])
      return fail(Reason::tower_invariant);
  for (u64 t = 0; t < nc; ++t) {
    if (in.rep_offsets[t + 1] <= in.rep_offsets[t]) return fail(Reason::tower_invariant);
    if (t > 0 && (in.cell_ball[t] <= in.cell_ball[t - 1] || in.cell_rank[t] < in.cell_rank[t - 1]))
      return fail(Reason::tower_invariant);
  }
  return {};
}

namespace detail {

// Octets de l'etage T d'un ordre : naissances (cle, noeud, ordre), spheres de la plus large cohorte, feuilles des
// representants, noyau, historique.
u64 kernel_bytes(const ForestInput& in) noexcept {
  const u64 nb = in.birth_key.size(), nc = in.cell_ball.size(), nr = in.targets.size();
  u64 widest = 1;
  for (u64 lo = 0; lo < nb;) {
    u64 hi = lo + 1;
    while (hi < nb && in.birth_rank[hi] == in.birth_rank[lo]) ++hi;
    widest = std::max(widest, hi - lo);
    lo = hi;
  }
  const u64 spheres = in.k >= 2 ? widest * (sizeof(num::Sphere) + 4) : 0;
  // naissances (cle, noeud, ordre) ; spheres ; feuilles ; union-find, elements des cellules, evenements et leurs
  // cellules, sommets, attaches ; historique (profondeur, decalages, liste, curseurs)
  return 12 * nb + spheres + 4 * nr + 16 * nb + 4 * nc + 24 * nb + 4 * nc + 8 * nb + nb + 8 * (nb + 1) + 4 * nb +
         8 * nb;
}

// Octets de l'etage M d'un ordre, ne evenements : nn = nb + classes <= nb + ne noeuds.
u64 contraction_bytes(const ForestInput& in, u64 ne) noexcept {
  const u64 nb = in.birth_key.size(), nn = nb + ne, nc = in.cell_ball.size();
  // travail par evenement, noeuds (rang, parent, plus petite naissance), CSR des enfants, comptes, sommets des
  // cellules, tranches (au plus une par evenement, plus une)
  return 24 * ne + 12 * nn + 8 * (nn + 1) + 4 * nn + 4 * nn + 4 * nc + sizeof(Slice) * (ne + 1);
}

// Numerotation canonique des naissances de l'ordre i (M, avant T) et tampon des feuilles.
Outcome number_order(BuildState& s, u32 i) noexcept {
  const ForestInput& in = s.inputs[i];
  OrderForest& f = s.forests.orders[i];
  OrderWork& w = s.work[i];
  const u32 nb = static_cast<u32>(in.birth_key.size());
  f.k = in.k;
  f.births = nb;
  Stopwatch watch;
  MHGP12_TRY(f.birth_key.allocate(nb, s.budget));
  MHGP12_TRY(f.birth_node.allocate(nb, s.budget));
  MHGP12_TRY(w.birth_order.allocate(nb, s.budget));
  MHGP12_TRY(number_births(s.cloud, s.balls, in, w.birth_order.span(), f.birth_node.span(), s.counters[i], s.budget));
  for (u32 v = 0; v < nb; ++v) f.birth_key[v] = in.birth_key[w.birth_order[v]];
  MHGP12_TRY(w.leaves.allocate(in.targets.size(), s.budget));
  s.physical[i].births_ns = watch.nanoseconds();
  return {};
}

void add_work(ForestWork& t, const ForestWork& p) noexcept {
  t.cohorts += p.cohorts;
  t.max_cohort = std::max(t.max_cohort, p.max_cohort);
  t.birth_targets += p.birth_targets;
  t.cell_targets += p.cell_targets;
  t.events += p.events;
  t.attaches += p.attaches;
  t.max_attach_depth = std::max(t.max_attach_depth, p.max_attach_depth);
  t.retained_cells += p.retained_cells;
  t.classes += p.classes;
  t.t6_from_cell += p.t6_from_cell;
  t.t6_climbs += p.t6_climbs;
  t.t6_from_birth += p.t6_from_birth;
  t.t5_queries += p.t5_queries;
  t.t5_hops += p.t5_hops;
  t.t5_max_hops = std::max(t.t5_max_hops, p.t5_max_hops);
  t.t5_probes += p.t5_probes;
  t.branch_reads += p.branch_reads;
  t.branches += p.branches;
}

namespace {

inline constexpr u64 kLeafChunk = 4096;  // cellules par morceau de la pre-passe

// Noyau et historique de l'ordre i (un proprietaire par ordre, sequentiel).
Outcome kernel_task(BuildState& s, u32 i) noexcept {
  OrderForest& f = s.forests.orders[i];
  OrderWork& w = s.work[i];
  Stopwatch kernel_watch;
  MHGP12_TRY(run_kernel(s.inputs[i], w.leaves.span(), f, w, s.counters[i], s.budget));
  s.physical[i].kernel_ns = kernel_watch.nanoseconds();
  w.leaves.reset();
  Stopwatch history_watch;
  MHGP12_TRY(build_history(f, w, s.counters[i], s.budget));
  s.physical[i].history_ns = history_watch.nanoseconds();
  return {};
}

Outcome numbering_body(void* context, u64 begin, u64 end, u32) noexcept {
  auto& s = *static_cast<BuildState*>(context);
  Outcome out;
  for (u64 i = begin; i < end; ++i) out = merge(out, number_order(s, static_cast<u32>(i)));
  return out;
}

Outcome kernel_body(void* context, u64 begin, u64 end, u32) noexcept {
  auto& s = *static_cast<BuildState*>(context);
  Outcome out;
  for (u64 i = begin; i < end; ++i) out = merge(out, kernel_task(s, static_cast<u32>(i)));
  return out;
}

// Morceau de la pre-passe : cellules [begin, end) de l'ordre, compteurs et duree propres.
struct LeafTask {
  u32 order = 0;
  u64 begin = 0, end = 0, ns = 0;
};
struct LeafRun {
  BuildState& state;
  std::span<LeafTask> tasks;
  std::span<ForestWork> counters;
};

Outcome leaves_body(void* context, u64 begin, u64 end, u32) noexcept {
  auto& run = *static_cast<LeafRun*>(context);
  Outcome out;
  for (u64 j = begin; j < end; ++j) {
    LeafTask& t = run.tasks[j];
    Stopwatch watch;
    BuildState& s = run.state;
    out = merge(out, resolve_leaves(s.inputs[t.order], s.forests.orders[t.order], s.work[t.order].leaves.span(),
                                    t.begin, t.end, run.counters[j]));
    t.ns = watch.nanoseconds();
  }
  return out;
}

// Pre-passe de tous les ordres, en morceaux de cellules ; compteurs sommes dans l'ordre des morceaux.
Outcome run_leaves(BuildState& s, sched::Pool& pool) noexcept {
  u64 count = 0;
  for (const ForestInput& in : s.inputs) count += (in.cell_ball.size() + kLeafChunk - 1) / kLeafChunk;
  if (count == 0) return {};
  Buffer<LeafTask> tasks;
  Buffer<ForestWork> counters;
  MHGP12_TRY(admit_stage(s, kStageT, count * (sizeof(LeafTask) + sizeof(ForestWork))));
  MHGP12_TRY(tasks.allocate(count, s.budget));
  MHGP12_TRY(counters.allocate(count, s.budget));
  u64 j = 0;
  for (u64 i = 0; i < s.inputs.size(); ++i) {
    const u64 nc = s.inputs[i].cell_ball.size();
    for (u64 b = 0; b < nc; b += kLeafChunk) tasks[j++] = LeafTask{static_cast<u32>(i), b, std::min(nc, b + kLeafChunk), 0};
  }
  for (u64 t = 0; t < count; ++t) counters[t] = ForestWork{};
  LeafRun run{s, tasks.span(), counters.span()};
  MHGP12_TRY(pool.parallel_for(count, 1, &run, leaves_body));
  for (u64 t = 0; t < count; ++t) {
    ForestWork& c = s.counters[tasks[t].order];
    c.birth_targets += counters[t].birth_targets;
    c.cell_targets += counters[t].cell_targets;
    s.physical[tasks[t].order].leaves_ns += tasks[t].ns;
  }
  return {};
}

}  // namespace

Outcome run_kernels(BuildState& s, sched::Pool& pool) noexcept {
  u64 bytes = 0;
  for (const ForestInput& in : s.inputs) {
    const u64 more = kernel_bytes(in);
    if (more > UINT64_MAX - bytes) return fail(Reason::memory_budget);
    bytes += more;
  }
  MHGP12_TRY(admit_stage(s, kStageT, bytes));
  Stopwatch watch;
  MHGP12_TRY(pool.parallel_for(s.inputs.size(), 1, &s, numbering_body));
  MHGP12_TRY(run_leaves(s, pool));
  MHGP12_TRY(pool.parallel_for(s.inputs.size(), 1, &s, kernel_body));
  s.kernels_ns = watch.nanoseconds();
  return {};
}

}  // namespace detail

Result<TowerForests> build_forests(const Cloud& cloud, const BallSource& balls, std::span<const ForestInput> inputs,
                                   const ForestParams& params, MemoryBudget& budget, sched::Pool& pool,
                                   ForestLedger* ledger) noexcept {
  return guarded([&]() -> Result<TowerForests> {
    if (inputs.empty() || inputs.size() > kMaxOrder || params.slice_events == 0) return fail(Reason::tower_invariant);
    for (u64 i = 0; i < inputs.size(); ++i) {
      MHGP12_TRY(check_forest_input(inputs[i]));
      if (inputs[i].k != i + 1) return fail(Reason::tower_invariant);
    }
    auto state = std::make_unique<detail::BuildState>(cloud, balls, inputs, params, budget);
    detail::BuildState& s = *state;
    s.forests.kmax = static_cast<Order>(inputs.size());
    MHGP12_TRY(detail::run_kernels(s, pool));
    MHGP12_TRY(detail::run_contraction(s, pool));
    MHGP12_TRY(detail::run_verticals(s, pool));
    MHGP12_TRY(detail::run_registry(s, pool));
    if (ledger != nullptr) detail::fill_ledger(s, pool.size(), *ledger);
    return std::move(s.forests);
  });
}

}  // namespace mhgp12::tower
