// Voie appareil du catalogue (tranche T1-b) : crochet du parcours (lots de feuilles, rejeu exact des non resolues,
// admission, ecriture dans l'arene) et voie complete, ecrits une fois pour l'executeur CUDA et l'executeur Pool. Voir
// device_driver.hpp pour la sequence et la regle de la premiere faute. Sorties anticipees (tranche T2-d,
// finish_outputs.hpp) : au dernier lot, des que ses comptes sont admis, les Buffer hote de sortie (boules, decalages,
// populations, table) sont reserves a leur taille exacte et leurs pages touchees sur le Pool pendant que l'appareil
// ecrit le lot (Copy, Replay) ; les niveaux suivent dans la fin d'etage.
#pragma once

#include "catalogue/device_driver.hpp"

namespace mhgp12::catalogue_detail::dev {

// Refus propres a la voie appareil : appareil indisponible (construction sans CUDA, aucun appareil, contexte
// impossible a ouvrir) ou erreur du pilote CUDA pendant le calcul.
inline Outcome device_refusal(bool opened) noexcept {
  return fail(opened ? Reason::device_fault : Reason::device_unavailable);
}

template <class B>
struct LeafHook {
  B& b;
  DeviceState<B>& st;
  const Cloud& cloud;
  const CatalogueParams& params;
  MemoryBudget& budget;
  sched::Pool& pool;
  fin::FinishOutput& out;  // sorties hote anticipees (voie complete)
  DeviceTotals totals{};
  u64 pending_leaves = 0, pending_sites = 0, balls = 0, incidences = 0;
  bool finishing = false, prepared = false;

  u64 leaf_base() const noexcept { return pending_leaves; }
  u64 leaf_site_base() const noexcept { return pending_sites; }
  Outcome after_level(const bfs::Level&, const bfs::LevelTotals& t, u32) noexcept {
    pending_leaves += t.f[3];
    pending_sites += t.f[4];
    return pending_leaves >= kBatchLeaves ? flush() : Outcome{};
  }
  // Refus du parcours : les feuilles des niveaux deja emis passent d'abord (la voie CPU les a consommees).
  Outcome refuse(Outcome refusal) noexcept {
    MHGP12_TRY(flush());
    return refusal;
  }
  Outcome finish() noexcept {
    finishing = true;
    return flush();
  }
  Outcome flush() noexcept {
    for (u64 first = 0; first < pending_leaves; first += kBatchLeaves) {
      const u64 n = pending_leaves - first < kBatchLeaves ? pending_leaves - first : kBatchLeaves;
      MHGP12_TRY(batch(first, n, finishing && first + n == pending_leaves));
    }
    pending_leaves = pending_sites = 0;
    return {};
  }
  // Sorties anticipees a leur taille exacte (comptes admis de tous les lots), une seule fois.
  Outcome prepare(u64 total_balls, u64 total_incidences) noexcept {
    prepared = true;
    return fin::prepare_outputs(out, total_balls, total_incidences, cloud.sites(), budget, pool, b.meter);
  }

  // Feuilles non resolues du lot : compactees, rapatriees, rejouees par LeafStage ; rend leur premiere faute.
  Result<Failure> replay(const LeafView& v, const BatchCounts& c, LeafStage& stage) noexcept {
    LeafArrays<B>& a = st.leaves;
    MHGP12_TRY(b.ensure(a.held_leaves, c.unresolved));
    MHGP12_TRY(b.ensure(a.held_sites, c.unresolved_sites));
    MHGP12_TRY(b.launch(CompactKernel{v, a.cls.data(), a.unresolved_at.data(), a.unresolved_site_at.data(),
                                      a.held_leaves.data(), a.held_sites.data()},
                        v.n));
    Buffer<bfs::Leaf> leaves;
    Buffer<u32> sites;
    MHGP12_TRY(leaves.allocate(c.unresolved, budget));
    MHGP12_TRY(sites.allocate(c.unresolved_sites, budget));
    MHGP12_TRY(b.download(leaves.data(), a.held_leaves, c.unresolved, 0));
    MHGP12_TRY(b.download(sites.data(), a.held_sites, c.unresolved_sites, 0));
    totals.replayed_leaves += c.unresolved;
    return replay_unresolved(stage, leaves.span(), sites.span());
  }

  // Boules rejouees par l'hote copiees dans l'arene a partir de (at, population_at).
  Outcome upload_replayed(LeafStage& stage, u64 at, u64 population_at) noexcept {
    for (const Chunk& chunk : stage.chunks()) {
      Buffer<BallRecord> moved;
      MHGP12_TRY(moved.allocate(chunk.records.size(), budget));
      for (u64 i = 0; i < chunk.records.size(); ++i) {
        moved[i] = chunk.records[i];
        moved[i].population += population_at;
        moved[i].chunk = 0;
      }
      MHGP12_TRY(b.upload(st.records, moved.data(), moved.size(), at));
      MHGP12_TRY(b.upload(st.population, chunk.population.data(), chunk.population.size(), population_at));
      at += chunk.records.size();
      population_at += chunk.population.size();
    }
    return {};
  }

  Outcome accumulate(const BatchCounts& c, LeafStage& stage) noexcept {
    if (c.stats.overflow != 0) return fail(Reason::catalogue_counter_overflow);
    LeafCounts device{};
    for (u32 f = 0; f < kLeafCounters; ++f) device.c[f] = c.stats.counts[f];
    if (!add_leaf_counts(totals.counts, device) || !add_leaf_counts(totals.counts, stage.totals().counts))
      return fail(Reason::catalogue_counter_overflow);
    for (u32 f = 0; f < 5; ++f) totals.tiers[f] += c.stats.tiers[f];
    totals.max_span = c.stats.max_span > totals.max_span ? c.stats.max_span : totals.max_span;
    totals.replayed_balls += stage.balls();
    totals.replayed_wide += c.stats.tiers[4];  // causes de non-resolution (ClassifyKernel) : m > 32
    totals.replayed_span += c.stats.tiers[3];  // etendue > 16
    totals.rewritten_device += c.stats.rewritten;
    totals.rewritten_host += stage.totals().rewritten;
    ++totals.batches;
    return {};
  }

  Outcome batch(u64 first, u64 n, bool last) noexcept {
    const NetWatch<B> watch(b);
    const LeafView v{st.x.data(), st.y.data(), st.z.data(), st.front.leaves.data() + first,
                     st.front.leaf_sites.data(), n, params.kmax};
    const auto counted = count_batch(b, st.leaves, v);
    if (!counted.ok()) return counted.outcome();
    totals.leaves_ns += watch.nanoseconds();
    const NetWatch<B> emission_watch(b);
    const BatchCounts& c = counted.value();
    Failure device{};
    if (c.stats.failure != kNoFailure)
      device = Failure{c.stats.failure / 4,
                       fail(c.stats.failure % 4 == 0 ? Reason::shell_capacity : Reason::catalogue_invariant)};
    LeafStage stage(cloud, params, budget, pool);
    Failure host{};
    if (c.unresolved > 0) {
      const auto replayed = replay(v, c, stage);
      if (!replayed.ok()) return replayed.outcome();
      host = replayed.value();
    }
    const Failure earliest_failure = earliest(device, host);
    MHGP12_TRY(earliest_failure.outcome);
    u64 host_incidences = 0;
    for (const Chunk& chunk : stage.chunks()) host_incidences += chunk.population.size();
    const u64 grown = balls + c.balls + stage.balls();
    u64 grown_incidences = 0;
    if (grown >= kNone) return fail(Reason::index_overflow_u32);
    if (__builtin_add_overflow(incidences, c.incidences, &grown_incidences) ||
        __builtin_add_overflow(grown_incidences, host_incidences, &grown_incidences))
      return fail(Reason::memory_budget);
    MHGP12_TRY(b.ensure_keep(st.records, grown, balls));  // admission : comptes exacts, rejeu compris, avant l'ecriture
    MHGP12_TRY(b.ensure_keep(st.population, grown_incidences, incidences));
    // boules rejouees par l'hote : places disjointes de celles de Copy et Replay, copiees avant leur lancement
    MHGP12_TRY(upload_replayed(stage, balls + c.balls, incidences + c.incidences));
    LeafArrays<B>& a = st.leaves;
    const FillView fill{a.cls.data(),     a.cases.data(),        a.balls.data(),            a.incidences.data(),
                        a.ball_at.data(), a.population_at.data(), st.records.data() + balls, st.population.data(),
                        incidences,       st.fault.data()};
    MHGP12_TRY(b.launch(CopyKernel{v, fill}, n));
    MHGP12_TRY(b.launch(ReplayKernel{v, fill}, n));
    if (last && fin::kAnticipateOutputs) MHGP12_TRY(prepare(grown, grown_incidences));  // pendant Copy et Replay
    MHGP12_TRY(b.sync());  // diagnostic : l'ecriture du lot est comptee dans l'emission
    MHGP12_TRY(accumulate(c, stage));
    balls = grown;
    incidences = grown_incidences;
    totals.emission_ns += emission_watch.nanoseconds();
    return {};
  }
};

// Diagnostics de la voie appareil dans CatalogueDiagnostics, durees disjointes (CST-0235), chacune nette des
// transferts, de la preparation des sorties et de la publication : parcours (traversal_ns), feuilles (count_ns :
// classement, comptage J3, decalages), emission (fill_ns : rejeu des non resolues, admission, ecriture), fin d'etage
// (levels_ns, sort_ns, assemble_ns, table_ns) ; transferts du raccord complet (transfer_ns : TOUTE copie hote <->
// appareil de l'appel, nuage, totaux de niveau, feuilles non resolues et boules rejouees, repli des chaines, sorties en
// flux) ; preparation des sorties (outputs_ns : reservation anticipee et premier toucher, tranche T2-d) ; publication
// (publish_ns : niveaux materialises au fil du flux, puis adoption, dont l'appelant ajoute la duree).
inline void device_diagnostics(const TraversalDiagnostics& front, const DeviceTotals& t, const fin::FinishStats& f,
                               u64 traversal_wall, u64 traversal_moved, const TransferMeter& moved,
                               CatalogueDiagnostics& d) noexcept {
  d.levels = front.levels;
  d.tasks = front.tasks;
  d.candidates = front.candidates;
  d.leaves_narrow = t.tiers[0];
  d.leaves_medium = t.tiers[1];
  d.leaves_wide = t.tiers[2];
  d.leaves_exact = t.tiers[3];
  d.leaves_virtual_warp = t.tiers[4];
  d.max_leaf_span = t.max_span;
  const u64 inner = traversal_moved + t.leaves_ns + t.emission_ns;
  d.traversal_ns = traversal_wall > inner ? traversal_wall - inner : 0;  // parcours seul, net
  d.count_ns = t.leaves_ns;                                              // feuilles
  d.fill_ns = t.emission_ns;                                             // emission
  d.levels_ns = f.keys_ns;
  d.sort_ns = f.sort_ns;
  d.assemble_ns = f.check_ns + f.emit_ns + f.take_ns;
  d.table_ns = f.table_ns;
  d.transfer_ns = moved.ns;
  d.transfer_h2d_bytes = moved.h2d_bytes;
  d.transfer_d2h_bytes = moved.d2h_bytes;
  d.transfer_ops = moved.ops;
  d.outputs_ns = moved.outputs_ns;
  d.outputs_bytes = moved.outputs_bytes;
  d.stream_chunks = moved.chunks;
  d.publish_ns = moved.publish_ns;
  d.chains_repaired = f.chains_repaired;
  d.chain_elements = f.chain_elements;
  d.batches = t.batches;
  d.replayed_leaves = t.replayed_leaves;
  d.replayed_balls = t.replayed_balls;
  d.replayed_wide = t.replayed_wide;
  d.replayed_span = t.replayed_span;
  d.rewritten_device = t.rewritten_device;
  d.rewritten_host = t.rewritten_host;
  d.leaves_rewritten = t.rewritten_device + t.rewritten_host;  // meme sens que la voie CPU
}


// Voie appareil complete sur un etat resident ; refus dans l'ordre de build_catalogue (parametres, nuage vide,
// multiplicites, ressources et calcul).
template <class B>
Result<Catalogue> device_catalogue(B& b, DeviceState<B>& st, const Cloud& cloud, const CatalogueParams& params,
                                   MemoryBudget& budget, sched::Pool& pool, CatalogueDiagnostics& diag) noexcept {
  MHGP12_TRY(check_catalogue_params(params));
  if (cloud.sites() == 0) return fail(Reason::empty_input);
  for (const u32 weight : cloud.w())
    if (weight != 1) return fail(Reason::multiplicity_unsupported);  // decision D8 : refus par defaut
  const u64 n = cloud.sites();
  const TransferMeter start = b.meter;  // compteur cumule de l'executeur (resident) : deltas de cet appel
  Stopwatch watch;
  for (auto* array : {&st.x, &st.y, &st.z}) MHGP12_TRY(b.ensure(*array, n));
  MHGP12_TRY(b.upload(st.x, cloud.x().data(), n, 0));
  MHGP12_TRY(b.upload(st.y, cloud.y().data(), n, 0));
  MHGP12_TRY(b.upload(st.z, cloud.z().data(), n, 0));
  MHGP12_TRY(b.ensure(st.fault, 1));
  MHGP12_TRY(b.put(st.fault, 0u, 0));
  fin::FinishOutput out;
  LeafHook<B> hook{b, st, cloud, params, budget, pool, out};
  const bfs::Params tp{static_cast<u32>(params.kmax), params.leaf_size, params.max_leaf, static_cast<u32>(kCoordBits)};
  const TraversalInput in{st.x.data(), st.y.data(), st.z.data(), n, traversal_root(cloud.x(), cloud.y(), cloud.z())};
  TraversalLedger walked;
  TraversalDiagnostics front;
  MHGP12_TRY(traverse_with(b, st.front, in, tp, hook, walked, front));
  const u64 traversal_wall = watch.nanoseconds(), traversal_moved = b.meter.off_stage() - start.off_stage();
  const auto fault = b.read(st.fault, 0);
  if (!fault.ok()) return fault.outcome();
  if (fault.value() != 0) return fail(Reason::catalogue_invariant);
  const CatalogueLedger ledger = make_ledger(walked, hook.totals.counts);
  if (ledger.emitted != hook.balls) return fail(Reason::catalogue_invariant);
  if (!hook.prepared && fin::kAnticipateOutputs)  // aucun lot final (feuilles toutes remises plus tot)
    MHGP12_TRY(hook.prepare(hook.balls, hook.incidences));
  const fin::FinishInput fi{st.x.data(), st.y.data(), st.z.data(), cloud.sites(), st.records.data(),
                            st.population.data(), hook.balls, hook.incidences};
  fin::FinishStats fs;
  MHGP12_TRY(fin::finish_stage(b, st.finish, fi, out, true, budget, pool, fs));
  Stopwatch adopt_watch;
  auto result = Assembly::adopt(out, static_cast<Order>(params.kmax), ledger);
  if (!result.ok()) return result.outcome();
  if (result.value().population().size() != ledger.incidences) return fail(Reason::catalogue_invariant);
  device_diagnostics(front, hook.totals, fs, traversal_wall, traversal_moved, meter_since(b.meter, start), diag);
  diag.publish_ns += adopt_watch.nanoseconds();
  diag.arena_bytes = hook.balls * sizeof(BallRecord) + hook.incidences * sizeof(SiteIdx);
  diag.peak_bytes = budget.peak();
  return result;
}

}  // namespace mhgp12::catalogue_detail::dev
