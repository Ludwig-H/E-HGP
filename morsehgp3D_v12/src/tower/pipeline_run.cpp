// Region de la Session recouverte (pipeline.hpp) : graphe des etapes de la foret, reclamation et execution des
// travaux par les fils, terminaison. Un fil annonce son travail (in_flight) AVANT de reclamer, et tout ce qu'un travail
// rend reclamable (tranche publiee, etape liberee, noyau rendu) est ecrit AVANT son retrait. Le dernier retrait
// conserve son passage par zero : une annonce concurrente ne doit pas effacer ce droit de sortir si le dry-scan
// est vide. Ce retour est individuel, pas une quiescence globale : un nouveau participant peut encore travailler
// et reste responsable de ses publications. A defaut de travail, un fil patiente puis recommence.
#include <algorithm>
#include <thread>

#include "tower/pipeline.hpp"
#include "tower/profile.hpp"

namespace mhgp12::tower::detail {

StepDeps step_dependencies(Step s) noexcept {
  switch (s) {
    case kCheck: return {0, {}};
    case kNumber: return {1, {{kCheck, false}}};
    case kKernel: return {1, {{kNumber, false}}};
    case kHistory: return {1, {{kKernel, false}}};
    case kSlices: return {1, {{kKernel, false}}};
    case kClasses: return {1, {{kSlices, false}}};
    case kNodes0: return {1, {{kClasses, false}}};
    case kNodes: return {1, {{kNodes0, false}}};
    case kParents: return {1, {{kNodes, false}}};
    case kPlace: return {1, {{kParents, false}}};
    case kChildren: return {1, {{kPlace, false}}};
    case kFinish: return {1, {{kChildren, false}}};
    case kLower: return {2, {{kFinish, false}, {kFinish, true}}};
    case kBirths: return {1, {{kLower, false}}};
    case kMerges: return {2, {{kBirths, false}, {kHistory, true}}};
    case kRows: return {2, {{kFinish, false}, {kHistory, false}}};
    case kCollect: return {1, {{kRows, false}}};
    case kPlaceRows: return {1, {{kCollect, false}}};
    case kFill: return {1, {{kPlaceRows, false}}};
    case kStepCount: break;
  }
  return {0, {}};
}

u32 refusal_rank(Step s) noexcept {
  if (s == kCheck) return 0;
  if (s == kNumber) return 1;
  if (s == kKernel || s == kHistory) return 3;
  if (s >= kSlices && s <= kFinish) return 4;
  if (s == kLower || s == kBirths) return 5;
  if (s == kMerges) return 6;
  return 7;
}

bool step_done(const Pipeline& p, u32 i, Step s) noexcept {
  return (p.steps[i][s].flags.load(std::memory_order_acquire) & kStepDone) != 0;
}

void prepare_graph(Pipeline& p) noexcept {
  u64 start = 0;
  for (u32 i = p.orders; i-- > 0;) {
    p.g_start[i] = start;
    start += p.g_items[i];
    p.kernel_slice[i].store(0, std::memory_order_relaxed);
  }
  p.g_total = start;
  for (u32 i = 0; i < p.orders; ++i)
    for (u32 s = 0; s < kStepCount; ++s) {
      const StepDeps deps = step_dependencies(static_cast<Step>(s));
      StepState& st = p.steps[i][s];
      u32 pending = 0;
      for (u32 e = 0; e < deps.count; ++e) pending += !deps.edge[e].below || i > 0;
      const bool absent = i == 0 && (s == kLower || s == kBirths || s == kMerges);  // aucune verticale a k = 1
      st.pending.store(absent ? 0 : pending, std::memory_order_relaxed);
      st.items.store(1, std::memory_order_relaxed);
      st.next.store(0, std::memory_order_relaxed);
      st.finished.store(0, std::memory_order_relaxed);
      st.flags.store(absent ? kStepDone : 0, std::memory_order_relaxed);
    }
}

namespace {

using Clock = std::chrono::steady_clock;

enum class JobKind : u8 { none, g, kernel, step };
struct Job {
  JobKind kind = JobKind::none;
  u32 order = 0;
  Step step = kCheck;
  u64 item = 0;
};

u64 now_ns(const Pipeline& p) noexcept {
  return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - p.start).count());
}

u8 slice_flag(Pipeline& p, u32 i, u64 s) noexcept {
  return std::atomic_ref<u8>(p.g_flags[i][s]).load(std::memory_order_acquire);
}

ForestWork& work_of(Pipeline& p, u32 i, u32 w) noexcept { return p.f_counters[u64{i} * p.threads + w]; }
ForestPhysical& physical_of(Pipeline& p, u32 i, u32 w) noexcept { return p.f_physical[u64{i} * p.threads + w]; }

void record(Pipeline& p, u32 w, u32 rank, const Outcome& o) noexcept {
  p.totals[w].forest_outcome[rank] = merge(p.totals[w].forest_outcome[rank], o);
}

void complete_step(Pipeline& p, u32 i, Step s) noexcept;

// Un predecesseur de (j, t) est termine : l'etape devient prete au dernier ; sans morceau, elle est terminee aussitot.
void release(Pipeline& p, u32 j, Step t) noexcept {
  StepState& st = p.steps[j][t];
  if (st.pending.fetch_sub(1, std::memory_order_acq_rel) == 1 && st.items.load(std::memory_order_relaxed) == 0)
    complete_step(p, j, t);
}

void complete_step(Pipeline& p, u32 i, Step s) noexcept {
  p.steps[i][s].flags.store(kStepDone, std::memory_order_release);
  if (s == kFinish) p.order_m_end[i].store(now_ns(p), std::memory_order_relaxed);
  if (s == kMerges) p.order_v_end[i].store(now_ns(p), std::memory_order_relaxed);
  if (s == kFill) p.order_r_end[i].store(now_ns(p), std::memory_order_relaxed);
  for (u32 t = 0; t < kStepCount; ++t) {
    const StepDeps deps = step_dependencies(static_cast<Step>(t));
    for (u32 e = 0; e < deps.count; ++e) {
      if (deps.edge[e].step != s) continue;
      const u32 j = deps.edge[e].below ? i + 1 : i;
      if (j < p.orders) release(p, j, static_cast<Step>(t));
    }
  }
  p.epoch.fetch_add(1, std::memory_order_release);
}

bool step_claimable(const Pipeline& p, u32 i, u32 s) noexcept {
  const StepState& st = p.steps[i][s];
  if (st.pending.load(std::memory_order_acquire) != 0 || (st.flags.load(std::memory_order_acquire) & kStepDone))
    return false;
  return st.next.load(std::memory_order_relaxed) < st.items.load(std::memory_order_relaxed);
}

bool kernel_claimable(Pipeline& p, u32 i) noexcept {
  const StepState& st = p.steps[i][kKernel];
  if (st.pending.load(std::memory_order_acquire) != 0 || st.flags.load(std::memory_order_acquire) != 0) return false;
  const u64 s = p.kernel_slice[i].load(std::memory_order_acquire);
  return s == p.g_items[i] || (slice_flag(p, i, s) & kSliceDone) != 0;
}

// Reclamation (claim) ou simple constat (dry) d'un travail, par ordre de preference.
bool find_job(Pipeline& p, bool claim, Job& job) noexcept {
  if (!p.g_failed.load(std::memory_order_relaxed)) {
    for (u32 i = p.orders; i-- > 0;) {
      if (!kernel_claimable(p, i)) continue;
      u32 idle = 0;
      if (!claim) return true;
      if (p.steps[i][kKernel].flags.compare_exchange_strong(idle, kStepBusy, std::memory_order_acq_rel)) {
        job = Job{JobKind::kernel, i, kKernel, 0};
        return true;
      }
    }
    for (u32 i = p.orders; i-- > 0;)
      for (u32 s = 0; s < kStepCount; ++s) {
        if (s == kKernel || !step_claimable(p, i, s)) continue;
        if (!claim) return true;
        StepState& st = p.steps[i][s];
        u64 n = st.next.load(std::memory_order_relaxed);
        while (n < st.items.load(std::memory_order_relaxed))
          if (st.next.compare_exchange_weak(n, n + 1, std::memory_order_acq_rel)) {
            job = Job{JobKind::step, i, static_cast<Step>(s), n};
            return true;
          }
      }
  }
  if (p.g_next.load(std::memory_order_relaxed) >= p.g_total) return false;
  if (!claim) return true;
  const u64 j = p.g_next.fetch_add(1, std::memory_order_relaxed);
  if (j >= p.g_total) return false;
  u32 i = 0;
  while (j < p.g_start[i] || j >= p.g_start[i] + p.g_items[i]) ++i;
  job = Job{JobKind::g, i, kCheck, j - p.g_start[i]};
  return true;
}

struct LeafWindow {
  u64 begin = 0, end = 0;
};

// Feuilles d'une tranche ; ns et, pour la seule pre-passe de G, sa vraie fenetre murale.
Outcome slice_leaves(Pipeline& p, u32 i, u32 w, u64 begin, u64 end, u64& ns,
                     LeafWindow* window = nullptr) noexcept {
  const u64 t0 = now_ns(p);
  const Outcome o = resolve_leaves(p.forest.inputs[i], p.forest.forests.orders[i], p.forest.work[i].leaves.span(),
                                   begin, end, work_of(p, i, w));
  const u64 t1 = now_ns(p);
  const u64 spent = t1 - t0;
  if (window != nullptr) *window = LeafWindow{t0, t1};
  physical_of(p, i, w).leaves_ns += spent;
  ns += spent;
  return o;
}

// Temps-fils d'un travail de la foret [t0, t1) (fenetre murale, pas du temps CPU) : total du fil, part posterieure a
// la fin de G telle que publiee a l'instant t1 (une tache finie avant la publication compte avant).
void charge_forest(Pipeline& p, u32 w, u64 t0, u64 t1) noexcept {
  p.totals[w].forest_ns += t1 - t0;
  const u64 g_end = p.g_end_ns.load(std::memory_order_acquire);
  if (g_end != 0 && t1 > g_end) p.totals[w].forest_after_g_ns += t1 - std::max(t0, g_end);
}

void atomic_max(std::atomic<u64>& a, u64 v) noexcept {
  u64 seen = a.load(std::memory_order_relaxed);
  while (seen < v && !a.compare_exchange_weak(seen, v, std::memory_order_relaxed)) {
  }
}

// Fin du calcul d'une tranche de G (instant t1, avant toute pre-passe des feuilles) : maxima de l'ordre et de tous
// les ordres, puis comptages ; le fil qui acheve le dernier calcul (de l'ordre, de tous les ordres) publie le maximum.
// Chaque maximum est ecrit avant le comptage de son fil, et les comptages (acq_rel) ordonnent ces ecritures avant la
// lecture du dernier : le maximum publie est celui de toutes les tranches.
void note_g_end(Pipeline& p, u32 i, u64 t1) noexcept {
  atomic_max(p.order_g_max[i], t1);
  atomic_max(p.g_max, t1);
  if (p.g_done[i].fetch_add(1, std::memory_order_acq_rel) + 1 == p.g_items[i])
    p.order_g_end[i].store(p.order_g_max[i].load(std::memory_order_relaxed), std::memory_order_relaxed);
  if (p.g_computed.fetch_add(1, std::memory_order_acq_rel) + 1 == p.g_total)
    p.g_end_ns.store(std::max<u64>(1, p.g_max.load(std::memory_order_relaxed)), std::memory_order_release);
}

void run_g(Pipeline& p, u32 w, u32 i, u64 slice) noexcept {
  const Order k = static_cast<Order>(i + 1);
  ResolvedOrder& order = tower_detail::StageAccess::order(p.out, k);
  const u64 begin = slice * tower_detail::kCellGrain;
  const u64 end = std::min<u64>(order.cells(), begin + tower_detail::kCellGrain);
  const u64 t0 = now_ns(p);
  Outcome o;
  if (k == 1) {
    o = tower_detail::first_order_cells(p.domain.catalogue, order, begin, end);
  } else {
    const tower_detail::ResolveContext context{p.domain, p.out,
                                               tower_detail::OrderView{k, order.birth_keys(), &p.tables[i]}};
    const auto counters = p.g_counters.span().subspan(u64{i} * p.threads, p.threads);
    const auto profiles = tower_detail::kProfile ? p.g_profiles.span().subspan(u64{i} * p.threads, p.threads)
                                   : std::span<SectionCycles>{};
    o = tower_detail::resolve_cells(context, order, p.team, counters, profiles, p.tables[i], begin, end, w);
  }
  const u64 t1 = now_ns(p);
  p.totals[w].g_ns += t1 - t0;
  note_g_end(p, i, t1);
  u8 flag = kSliceDone;
  if (!o.ok()) {
    p.totals[w].g_outcome = merge(p.totals[w].g_outcome, o);
    p.g_failed.store(true, std::memory_order_relaxed);
    flag = kSliceFailed;
  } else if (!p.g_failed.load(std::memory_order_relaxed) && step_done(p, i, kNumber)) {
    u64 spent = 0;
    LeafWindow window;
    const Outcome leaves = slice_leaves(p, i, w, begin, end, spent, &window);
    if (leaves.ok()) flag |= kSliceLeaves;
    else record(p, w, 2, leaves);
    charge_forest(p, w, window.begin, window.end);
  }
  std::atomic_ref<u8>(p.g_flags[i][slice]).store(flag, std::memory_order_release);
  p.epoch.fetch_add(1, std::memory_order_release);
}

// Noyau de l'ordre i (drapeau kStepBusy tenu) : tranches terminees dans l'ordre des cellules, puis cloture.
// Rend la duree des feuilles calculees par le noyau (comptee a part).
u64 run_kernel_job(Pipeline& p, u32 w, u32 i) noexcept {
  StepState& st = p.steps[i][kKernel];
  const ForestInput& in = p.forest.inputs[i];
  OrderWork& work = p.forest.work[i];
  OrderForest& f = p.forest.forests.orders[i];
  ++p.totals[w].kernel_jobs;
  u64 leaves_ns = 0;
  for (;;) {
    const u64 s = p.kernel_slice[i].load(std::memory_order_relaxed);
    if (s == p.g_items[i]) {
      const Outcome closed = close_kernel(in, f, work, p.forest.counters[i]);
      work.leaves.reset();
      if (!closed.ok()) {
        record(p, w, 3, closed);
        st.flags.store(kStepFailed, std::memory_order_release);
        return leaves_ns;
      }
      p.order_kernel_end[i].store(now_ns(p), std::memory_order_relaxed);
      complete_step(p, i, kKernel);
      return leaves_ns;
    }
    const u8 flag = slice_flag(p, i, s);
    if (!(flag & kSliceDone)) break;
    const u64 begin = s * tower_detail::kCellGrain;
    const u64 end = std::min<u64>(in.cell_ball.size(), begin + tower_detail::kCellGrain);
    Outcome o = (flag & kSliceLeaves) ? Outcome{} : slice_leaves(p, i, w, begin, end, leaves_ns);
    const u32 rank = o.ok() ? 3 : 2;
    if (o.ok()) o = advance_kernel(in, work.leaves.span(), f, work, p.forest.counters[i], end);
    if (!o.ok()) {
      record(p, w, rank, o);
      st.flags.store(kStepFailed, std::memory_order_release);
      return leaves_ns;
    }
    p.kernel_slice[i].store(s + 1, std::memory_order_release);
  }
  ++p.totals[w].kernel_stops;
  st.flags.store(0, std::memory_order_release);  // rendu : un autre fil le reprendra quand la tranche sera publiee
  p.epoch.fetch_add(1, std::memory_order_release);
  return leaves_ns;
}

// Morceau [item * size, ...) borne par count.
u64 piece_end(u64 item, u64 size, u64 count) noexcept { return std::min(count, (item + 1) * size); }
u64 pieces(u64 count, u64 size) noexcept { return (count + size - 1) / size; }

void set_items(Pipeline& p, u32 i, Step s, u64 items) noexcept {
  p.steps[i][s].items.store(items, std::memory_order_relaxed);
}

// Corps d'un morceau d'une etape ; rang de refus de la voie sequentielle dans rank.
Outcome step_body(Pipeline& p, u32 w, u32 i, Step s, u64 item, u32& rank) noexcept {
  BuildState& b = p.forest;
  const ForestInput& in = b.inputs[i];
  OrderForest& f = b.forests.orders[i];
  OrderWork& work = b.work[i];
  rank = refusal_rank(s);
  switch (s) {
    case kCheck: return check_forest_input(in);
    case kNumber: {  // numerotation (number_order chronometre births_ns), puis ouverture du noyau (kernel_ns)
      MHGP12_TRY(number_order(b, i));
      rank = 3;
      const u64 t0 = now_ns(p);
      const Outcome opened = open_kernel(in, f, work, b.budget);
      physical_of(p, i, w).kernel_ns += now_ns(p) - t0;
      return opened;
    }
    case kHistory: return build_history(f, work, b.counters[i], b.budget);
    case kSlices:
      MHGP12_TRY(open_contraction(b, i));
      for (const Step phase : {kClasses, kNodes, kParents, kChildren}) set_items(p, i, phase, work.slices.size());
      return {};
    case kClasses: contract_classes(work, work.slices[item]); return {};
    case kNodes0: return allocate_nodes(b, i);
    case kNodes: contract_nodes(work, f, work.slices[item]); return {};
    case kParents: contract_parents(work, f, work.slices[item]); return {};
    case kPlace: return place_children(b, i);
    case kChildren: contract_children(work, f, work.slices[item]); return {};
    case kFinish: return finish_order(b, i);
    case kLower:
      MHGP12_TRY(open_verticals(b, i));
      set_items(p, i, kBirths, pieces(f.births, kVerticalItems));
      set_items(p, i, kMerges, pieces(f.nodes() - f.births, kVerticalItems));
      return {};
    case kBirths:
      return birth_images(in, b.inputs[i - 1], b.forests.orders[i - 1], f, item * kVerticalItems,
                          piece_end(item, kVerticalItems, f.births), work_of(p, i, w));
    case kMerges:
      return merge_images(b.forests.orders[i - 1], f, f.births + item * kVerticalItems,
                          f.births + piece_end(item, kVerticalItems, f.nodes() - f.births), work_of(p, i, w));
    case kRows: {
      release_events(b, i);
      u64 reads = 0;
      MHGP12_TRY(prepare_rows(b, i, reads));
      const u64 rows = f.retained_cell.size();
      set_items(p, i, kCollect, pieces(rows, kRowItems));
      set_items(p, i, kFill, pieces(rows, kRowItems));
      return {};
    }
    case kCollect:
      return collect_rows(b, i, item * kRowItems, piece_end(item, kRowItems, f.retained_cell.size()),
                          work_of(p, i, w));
    case kPlaceRows: return place_rows(b, i);
    case kFill: fill_rows(b, i, item * kRowItems, piece_end(item, kRowItems, f.retained_cell.size())); return {};
    case kKernel:
    case kStepCount: break;
  }
  return fail(Reason::tower_invariant);
}

// Duree d'un morceau, imputee a l'etage de son etape (diagnostic physique par ordre et par fil) ; kNumber impute
// lui-meme ses deux parties.
void charge_stage(Pipeline& p, u32 w, u32 i, Step s, u64 ns) noexcept {
  ForestPhysical& ph = physical_of(p, i, w);
  if (s == kNumber) return;
  if (s == kCheck) ph.births_ns += ns;
  else if (s == kHistory) ph.history_ns += ns;
  else if (s >= kSlices && s <= kFinish) ph.contraction_ns += ns;
  else if (s >= kLower && s <= kMerges) ph.vertical_ns += ns;
  else ph.registry_ns += ns;
}

void run_step(Pipeline& p, u32 w, const Job& job) noexcept {
  const u32 i = job.order;
  StepState& st = p.steps[i][job.step];
  const u64 t0 = now_ns(p);
  u32 rank = 0;
  const Outcome o = step_body(p, w, i, job.step, job.item, rank);
  if (!o.ok()) {
    record(p, w, rank, o);
    st.flags.fetch_or(kStepFailed, std::memory_order_acq_rel);
  }
  if (st.finished.fetch_add(1, std::memory_order_acq_rel) + 1 == st.items.load(std::memory_order_relaxed) &&
      !(st.flags.load(std::memory_order_acquire) & kStepFailed)) {
    if (job.step == kFill) close_rows(p.forest, i);
    complete_step(p, i, job.step);
  }
  const u64 t1 = now_ns(p);
  charge_stage(p, w, i, job.step, t1 - t0);
  charge_forest(p, w, t0, t1);
}

void run_job(Pipeline& p, u32 w, const Job& job) noexcept {
  if (job.kind == JobKind::g) {
    run_g(p, w, job.order, job.item);
  } else if (job.kind == JobKind::kernel) {
    const u64 t0 = now_ns(p);
    const u64 leaves = run_kernel_job(p, w, job.order);
    const u64 t1 = now_ns(p);
    physical_of(p, job.order, w).kernel_ns += t1 - t0 - leaves;
    charge_forest(p, w, t0, t1);
  } else {
    run_step(p, w, job);
  }
}

// Patience d'un fil sans travail : pauses, puis cessions du processeur, puis sommeils de 20 microsecondes (la queue de
// la region peut ne laisser qu'un fil au travail : les autres ne doivent ni le ralentir ni consommer les coeurs).
void relax(u32 round) noexcept {
#if defined(__x86_64__) || defined(__i386__)
  if (round < 64) {
    __builtin_ia32_pause();
    return;
  }
#endif
  if (round < 256) std::this_thread::yield();
  else std::this_thread::sleep_for(std::chrono::microseconds(20));
}

}  // namespace

Outcome run_region(void* raw, u64, u64, u32 w) noexcept {
  Pipeline& p = *static_cast<Pipeline*>(raw);
  u32 idle = 0;
  for (;;) {
    p.in_flight.fetch_add(1, std::memory_order_acq_rel);  // annonce AVANT de reclamer
    Job job;
    if (find_job(p, true, job)) {
      idle = 0;
      run_job(p, w, job);
      p.in_flight.fetch_sub(1, std::memory_order_acq_rel);  // APRES tout ce que le travail rend reclamable
      continue;
    }
    const bool last = p.in_flight.fetch_sub(1, std::memory_order_acq_rel) == 1;
    const u64 seen = p.epoch.load(std::memory_order_acquire);
    if (last && !find_job(p, false, job)) return {};
    if (find_job(p, false, job)) continue;  // du travail est apparu : reclamer
    // Rien a reclamer : attendre une nouvelle epoque (ou la fin de tout travail en cours) avant d'explorer a nouveau.
    while (p.epoch.load(std::memory_order_acquire) == seen && p.in_flight.load(std::memory_order_acquire) != 0)
      relax(idle++);
  }
}

}  // namespace mhgp12::tower::detail
