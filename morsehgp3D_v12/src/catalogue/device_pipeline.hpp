// Voie appareil du catalogue (tranche T1-b) : crochet du parcours (lots de feuilles, rejeu exact des non resolues,
// admission, ecriture dans l'arene) et voie complete, ecrits une fois pour l'executeur CUDA et l'executeur Pool. Voir
// device_driver.hpp pour la sequence et la regle de la premiere faute. Sorties anticipees (tranche T2-d,
// finish_outputs.hpp) : au dernier lot, des que ses comptes sont admis, les Buffer hote de sortie (boules, decalages,
// populations, table) sont reserves a leur taille exacte et leurs pages touchees sur le Pool pendant que l'appareil
// ecrit le lot (Copy, Replay) ; les niveaux suivent dans la fin d'etage.
// Catalogue en flux (tranche T1-d) : l'arene est residente (ci-dessus) tant que le budget de l'appareil la porte ; si sa
// croissance est refusee, l'arene deja ecrite est rapatriee en un lot de l'hote, et chaque lot suivant ecrit son arene
// a la base 0 de l'appareil puis la rapatrie (Chunk, decalages de population locaux) : l'appareil est reemploye. A la
// fin, voie complete si l'arene est residente et que la fin d'etage tient dans la place libre de l'appareil ; sinon
// l'etat du parcours (front, lots de feuilles) est rendu, puis voie complete si elle tient alors, sinon voie par
// tranches de cles (finish_slices.hpp) sur l'arene de l'hote. Meme catalogue a l'octet ; une trame de 60 000 sites
// reste residente, en un lot et une tranche, et garde son etat d'un appel a l'autre.
#pragma once

#include "catalogue/device_driver.hpp"
#include "catalogue/finish_slices.hpp"

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
  fin::FinishOutput& out;     // sorties hote anticipees (voie complete)
  std::vector<Chunk>& arena;  // arene de l'hote (mode en flux)
  DeviceTotals totals{};
  u64 pending_leaves = 0, pending_sites = 0, balls = 0, incidences = 0;
  u64 held = 0, held_incidences = 0, streamed = 0;  // arene sur l'appareil ; lots rapatries
  bool finishing = false, prepared = false, streaming = false;

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
  // Arene de l'appareil [0, held) rapatriee en un lot de l'hote (decalages de population locaux au lot).
  Outcome take_arena() noexcept {
    if (held == 0) return {};
    Chunk chunk;
    MHGP12_TRY(chunk.records.allocate(held, budget));
    MHGP12_TRY(chunk.population.allocate(held_incidences, budget));
    MHGP12_TRY(b.download(chunk.records.data(), st.records, held, 0));
    if (held_incidences != 0) MHGP12_TRY(b.download(chunk.population.data(), st.population, held_incidences, 0));
    try {
      arena.push_back(std::move(chunk));
    } catch (const std::bad_alloc&) {
      return fail(Reason::memory_budget);
    }
    held = held_incidences = 0;
    ++streamed;
    return {};
  }
  // Passage au mode en flux : arene residente rapatriee (s'il en reste), tableaux de l'arene de l'appareil rendus.
  Outcome start_streaming() noexcept {
    MHGP12_TRY(take_arena());
    MHGP12_TRY(b.release(st.records));
    MHGP12_TRY(b.release(st.population));
    streaming = true;
    return {};
  }
  // Admission d'un lot de `more` boules et `more_incidences` incidences : arene residente agrandie (contenu garde) ;
  // refusee par le budget, passage au mode en flux ; en flux, arene du seul lot.
  Outcome admit(u64 more, u64 more_incidences) noexcept {
    if (!streaming) {
      Outcome grown = b.ensure_keep(st.records, held + more, held);
      if (grown.ok()) grown = b.ensure_keep(st.population, held_incidences + more_incidences, held_incidences);
      if (grown.ok() || grown.reason != Reason::memory_budget) return grown;
      MHGP12_TRY(start_streaming());
    }
    MHGP12_TRY(b.ensure(st.records, more));
    return b.ensure(st.population, more_incidences);
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
    // admission : comptes exacts, rejeu compris, avant l'ecriture (arene residente, ou celle du seul lot en flux)
    MHGP12_TRY(admit(c.balls + stage.balls(), grown_incidences - incidences));
    // boules rejouees par l'hote : places disjointes de celles de Copy et Replay, copiees avant leur lancement
    MHGP12_TRY(upload_replayed(stage, held + c.balls, held_incidences + c.incidences));
    LeafArrays<B>& a = st.leaves;
    const FillView fill{a.cls.data(),     a.cases.data(),        a.balls.data(),           a.incidences.data(),
                        a.ball_at.data(), a.population_at.data(), st.records.data() + held, st.population.data(),
                        held_incidences,  st.fault.data()};
    MHGP12_TRY(b.launch(CopyKernel{v, fill}, n));
    MHGP12_TRY(b.launch(ReplayKernel{v, fill}, n));
    if (last && fin::kAnticipateOutputs && !streaming) MHGP12_TRY(prepare(grown, grown_incidences));  // pendant Copy
    MHGP12_TRY(b.sync());  // diagnostic : l'ecriture du lot est comptee dans l'emission
    MHGP12_TRY(accumulate(c, stage));
    held += c.balls + stage.balls();
    held_incidences += grown_incidences - incidences;
    if (streaming) MHGP12_TRY(take_arena());  // arene du lot rapatriee, appareil reemploye
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


template <class B, class... A>
Outcome release_all(B& b, A&... arrays) noexcept {
  Outcome out{};
  ((out = out.ok() ? b.release(arrays) : out), ...);
  return out;
}

// Etat du parcours rendu (front, lots de feuilles) : la fin d'etage n'en a plus besoin. Seulement quand la fin d'etage
// complete ne tient pas dans la place libre (l'appel suivant le refait croitre).
template <class B>
Outcome release_traversal(B& b, DeviceState<B>& st) noexcept {
  Front<B>& f = st.front;
  LeafArrays<B>& l = st.leaves;
  MHGP12_TRY(release_all(b, f.parents[0], f.parents[1], f.task_begin[0], f.task_begin[1], f.list[0], f.list[1],
                         f.chunk_top, f.keep, f.reservoir, f.task_out, f.child_out, f.child_scan, f.tile_sum,
                         f.tile_offset, f.totals, f.leaves, f.leaf_sites));
  MHGP12_TRY(release_all(b, l.cls, l.balls, l.incidences, l.ball_at, l.population_at, l.counts, l.unresolved,
                         l.unresolved_at, l.unresolved_sites, l.unresolved_site_at, l.status, l.cases, l.tiles,
                         l.total, l.held_leaves, l.held_sites));
  return release_all(b, l.order.keys[0], l.order.keys[1], l.order.vals[0], l.order.vals[1], l.order.tile_hist,
                     l.order.bin_total, l.order.tile_and, l.order.tile_or, l.order.result, l.scan.tile_sum,
                     l.scan.total);
}

template <class B, class K>
Outcome release_radix(B& b, fin::RadixArrays<B, K>& r) noexcept {
  return release_all(b, r.keys[0], r.keys[1], r.vals[0], r.vals[1], r.tile_hist, r.bin_total, r.tile_and, r.tile_or,
                     r.result);
}

// Tableaux de la fin d'etage et de l'arene d'une tranche rendus : avant la voie par tranches (ceux d'un appel
// precedent), et apres elle (dimensionnes sur la place libre de cet appel, ils serreraient l'appel suivant).
template <class B>
Outcome release_finish(B& b, DeviceState<B>& st) noexcept {
  fin::FinishArrays<B>& a = st.finish;
  for (auto* r : {&a.sites, &a.positions, &a.table}) MHGP12_TRY(release_radix(b, *r));
  MHGP12_TRY(release_radix(b, a.keys));
  MHGP12_TRY(release_all(b, a.lexrank, a.verdict, a.fault, a.pick, a.levels, a.level_of_rank, a.pick_levels, a.fkey,
                         a.flag, a.before, a.offset, a.table_offset, a.pkey, a.pick_pkey, a.balls, a.values,
                         a.scan.tile_sum, a.scan.total));
  return release_all(b, st.slices.records, st.slices.population);
}

// Fin d'etage de la voie appareil : complete si l'arene est residente et que ses tableaux tiennent dans la place
// libre de l'appareil (au besoin une fois l'etat du parcours rendu) ; sinon arene rapatriee (si elle etait residente),
// tableaux de l'arene rendus et fin d'etage par tranches de cles, entre deux rendus des tableaux de la fin d'etage.
template <class B>
Outcome device_finish(B& b, DeviceState<B>& st, const Cloud& cloud, LeafHook<B>& hook, fin::FinishOutput& out,
                      MemoryBudget& budget, sched::Pool& pool, fin::FinishStats& fs) noexcept {
  const u64 full = fin::full_finish_bytes(hook.balls, hook.incidences, cloud.sites());
  if (hook.streaming || full > b.room()) MHGP12_TRY(release_traversal(b, st));
  if (!hook.streaming && full <= b.room()) {
    if (!hook.prepared && fin::kAnticipateOutputs)  // aucun lot final (feuilles toutes remises plus tot)
      MHGP12_TRY(hook.prepare(hook.balls, hook.incidences));
    const fin::FinishInput fi{st.x.data(), st.y.data(), st.z.data(), cloud.sites(), st.records.data(),
                              st.population.data(), hook.balls, hook.incidences};
    return fin::finish_stage(b, st.finish, fi, out, true, budget, pool, fs);
  }
  MHGP12_TRY(hook.start_streaming());
  MHGP12_TRY(release_finish(b, st));
  Buffer<u64> ball_at, incidence_at;
  fin::ArenaView view;
  MHGP12_TRY(fin::arena_bases(hook.arena, ball_at, incidence_at, view, budget));
  if (view.balls != hook.balls || view.incidences != hook.incidences) return fail(Reason::catalogue_invariant);
  const fin::SliceInput in{st.x.data(), st.y.data(), st.z.data(), cloud.sites(), view, &hook.arena};
  const Outcome done = fin::finish_sliced(b, st.finish, st.slices, in, out, 0, budget, pool, fs);
  const Outcome released = release_finish(b, st);
  return done.ok() ? released : done;
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
  std::vector<Chunk> arena;
  LeafHook<B> hook{b, st, cloud, params, budget, pool, out, arena};
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
  fin::FinishStats fs;
  MHGP12_TRY(device_finish(b, st, cloud, hook, out, budget, pool, fs));
  Stopwatch adopt_watch;
  auto result = Assembly::adopt(out, static_cast<Order>(params.kmax), ledger);
  if (!result.ok()) return result.outcome();
  if (result.value().population().size() != ledger.incidences) return fail(Reason::catalogue_invariant);
  device_diagnostics(front, hook.totals, fs, traversal_wall, traversal_moved, meter_since(b.meter, start), diag);
  diag.publish_ns += adopt_watch.nanoseconds();
  diag.finish_slices = fs.slices;
  diag.arena_streamed = hook.streamed;
  diag.arena_bytes = hook.balls * sizeof(BallRecord) + hook.incidences * sizeof(SiteIdx);
  diag.peak_bytes = budget.peak();
  return result;
}

}  // namespace mhgp12::catalogue_detail::dev
