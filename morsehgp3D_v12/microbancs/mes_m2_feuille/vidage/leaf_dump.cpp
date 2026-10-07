// Vidage des feuilles du catalogue de la v11 (microbanc MES-M2 de la v12, hors produit).
//
// Lie a la bibliotheque v11 gelee (libmhgp11.a, moteur ac081a06f, profil u21) et a ses en-tetes internes, sans les
// modifier. Trois passes :
//   1. parcours sequentiel des boites (catalogue_detail::walk), file differee branchee sur Run::deferred : chaque
//      feuille admissible a la voie lot (graphe de paires, 1 <= m <= 32) est copiee (boite T0 demi-ouverte, SiteIdx) ;
//      la partition est celle de toutes les voies (meme filtre G1, meme ajustement, meme bissection) ;
//   2. sortie de reference par leaf.cpp (enumerate_leaf, voie graphe de paires, sans file ni feuille source unique) :
//      emissions dans l'ordre de leaf.cpp (S*, p, m, qmin, populations I puis U) et quinze compteurs logiques, lus
//      comme difference du grand livre de la feuille ;
//   3. temoin v11 : la feuille source unique leaf_device.hpp jouee sur l'hote (contrat R7) ; son statut est note et,
//      si elle est resolue, ses emissions et compteurs doivent egaler ceux de leaf.cpp (bit 2 du statut sinon).
// Les chronos des passes 2 et 3 sont indicatifs (machine locale, un fil).
//
// Usage : mhgp12_leaf_dump <xyz.u32le> <ids.u32le> <K> <taille_feuille> <sortie.bin> [cache=1]
// Sortie standard : une ligne JSON de synthese. Codes : 0 conforme, 2 refus ou arguments, 3 invariant viole.
#include <algorithm>
#include <chrono>
#include <iostream>
#include <new>
#include <string>
#include <vector>

#include "catalogue/internal.hpp"
#include "catalogue/leaf_device.hpp"
#include "mhgp12/leaf/dump_format.hpp"
#include "whole_input.hpp"

using namespace mhgp11;
using namespace mhgp11::catalogue_detail;
namespace dump = mhgp12::dump;

namespace {

u64 now_ns() {
  return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::steady_clock::now().time_since_epoch())
                              .count());
}

// File differee du parcours : copie chaque feuille admissible (la voie lot de la v11 la mettrait en file).
struct DumpQueue final : LeafQueue {
  std::vector<dump::Job> jobs;
  std::vector<u32> sites;
  Outcome push(std::span<const SiteIdx> leaf, const Box& box) noexcept override {
    dump::Job job;
    job.begin = sites.size();
    job.m = static_cast<u32>(leaf.size());
    for (int a = 0; a < 3; ++a) {
      job.lo[a] = box.lo[a];
      job.hi[a] = box.hi[a];
    }
    for (SiteIdx s : leaf) sites.push_back(idx(s));
    jobs.push_back(job);
    return {};
  }
};

std::array<u64, dump::kCounters> counters_of(const CatalogueLedger& l) {
  return {l.dominance_tests, l.prefixes, l.judged, l.census_tests, l.emitted, l.incidences, l.q4_candidates,
          l.q4_levels, l.region_pair_tests, l.region_pair_rejects, l.region_line_tests, l.region_line_rejects,
          l.region_line_evaluations, l.region_line_cache_hits, l.region_line_fallbacks};
}

std::array<u64, dump::kCounters> counters_of(const leaf_device::Counts& c) {
  return {c.dominance_tests, c.prefixes, c.judged, c.census_tests, c.emitted, c.incidences, c.q4_candidates,
          c.q4_levels, c.region_pair_tests, c.region_pair_rejects, c.region_line_tests, c.region_line_rejects,
          c.region_line_evaluations, c.region_line_cache_hits, c.region_line_fallbacks};
}

// Puits de capture de la feuille source unique v11 : enregistrements et populations en rangs locaux.
struct CaptureSink {
  const u32* sites = nullptr;
  u32 m = 0;
  std::vector<dump::Record> records;
  std::vector<u8> population;
  void emit(const leaf_device::Ball& ball, const u32*, const u32*, const u32* interior_local,
            const u32* shell_local) {
    dump::Record r{};
    for (int k = 0; k < 4; ++k) {
      r.support[k] = dump::kNoLocal;
      for (u32 i = 0; i < m; ++i)
        if (sites[i] == ball.support[k]) r.support[k] = static_cast<u8>(i);
    }
    r.p = static_cast<u8>(ball.p);
    r.m = static_cast<u8>(ball.m);
    r.qmin = static_cast<u8>(ball.qmin);
    r.pad = 0;
    records.push_back(r);
    for (u32 i = 0; i < ball.p; ++i) population.push_back(static_cast<u8>(interior_local[i]));
    for (u32 i = 0; i < ball.m; ++i) population.push_back(static_cast<u8>(shell_local[i]));
  }
};

int fail_with(const Outcome& o, const char* stage) {
  std::cout << "{\"phase\":\"exit\",\"stage\":\"" << stage << "\",\"reason\":\"" << reason_name(o.reason)
            << "\"}\n";
  return exit_code(o);
}

int run(int argc, char** argv) {
  if (argc < 6 || argc > 7) return 2;
  u64 kmax = 0, leaf_size = 0, cache = 1;
  if (!bench::parse(argv[3], kmax) || !bench::parse(argv[4], leaf_size) || kmax < 1 || kmax > 12 ||
      leaf_size < kmax + 3 || leaf_size > 256)
    return 2;
  if (argc == 7 && (!bench::parse(argv[6], cache) || cache > 1)) return 2;
  const std::string out_path = argv[5];

  MemoryBudget budget(u64{64} << 30);
  const u64 t_read = now_ns();
  auto input = bench::read_input(argv[1], argv[2], budget);
  if (!input.ok()) return fail_with(input.outcome(), "read_input");
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                             input.value().ids.span(), CoordWidth(), budget);
  if (!cloud.ok()) return fail_with(cloud.outcome(), "prepare_cloud");
  const Cloud& c = cloud.value();
  for (u32 w : c.w())
    if (w != 1) return fail_with(fail(Reason::multiplicity_unsupported), "weights");
  const u64 read_ns = now_ns() - t_read;

  CatalogueParams params;
  params.kmax = static_cast<int>(kmax);
  params.leaf_size = static_cast<u32>(leaf_size);
  params.max_leaf = 256;
  params.cache_center_lines = cache != 0;
  params.pair_graph = true;
  if (const Outcome o = check_catalogue_params(params); !o.ok()) return fail_with(o, "params");

  // 1. Parcours sequentiel, feuilles admissibles en file.
  const u64 t_walk = now_ns();
  Workspace walk_space;
  if (const Outcome o = walk_space.allocate(params.max_leaf, budget, params.cache_center_lines, params.pair_graph);
      !o.ok())
    return fail_with(o, "workspace");
  Collector walk_collector;
  DumpQueue queue;
  Run walker{c, params, budget, walk_space, walk_collector, {}, nullptr, &queue};
  if (const Outcome o = walk(walker); !o.ok()) return fail_with(o, "walk");
  const u64 walk_ns = now_ns() - t_walk;
  const u64 n_leaves = queue.jobs.size();
  if (walker.ledger.leaves < n_leaves) return fail_with(fail(Reason::catalogue_invariant), "walk_leaves");

  dump::LeafDump d;
  d.header.coord_bits = kCoordBits;
  d.header.kmax = kmax;
  d.header.leaf_size = leaf_size;
  d.header.max_leaf = params.max_leaf;
  d.header.flags = (params.cache_center_lines ? dump::kFlagCache : 0) | dump::kFlagPairGraph;
  d.header.n_sites = c.sites();
  d.header.n_leaves = n_leaves;
  d.header.n_leaf_sites = queue.sites.size();
  d.header.walk_leaves = walker.ledger.leaves;
  d.header.walk_inline_leaves = walker.ledger.leaves - n_leaves;
  d.x.assign(c.x().begin(), c.x().end());
  d.y.assign(c.y().begin(), c.y().end());
  d.z.assign(c.z().begin(), c.z().end());
  d.jobs = std::move(queue.jobs);
  d.sites = std::move(queue.sites);
  d.counts.assign(n_leaves * dump::kCounters, 0);
  d.status.assign(n_leaves, 0);
  d.record_begin.assign(n_leaves + 1, 0);
  d.population_begin.assign(n_leaves + 1, 0);

  // 2. Reference leaf.cpp, feuille par feuille. Capacites maximales d'une feuille de 32 sites : au plus une emission
  // par prefixe de 2 a 4 sites (sum C(32,q) = 41 416), au plus 32 incidences par emission.
  constexpr u64 kMaxRecords = 496 + 4960 + 35960;
  Workspace ref_space;
  if (const Outcome o = ref_space.allocate(dump::kMaxLeafSites, budget, params.cache_center_lines, params.pair_graph);
      !o.ok())
    return fail_with(o, "workspace_ref");
  Buffer<Emission> records;
  Buffer<SiteIdx> population;
  if (const Outcome o = records.allocate(kMaxRecords, budget); !o.ok()) return fail_with(o, "records");
  if (const Outcome o = population.allocate(kMaxRecords * dump::kMaxLeafSites, budget); !o.ok())
    return fail_with(o, "population");
  Collector collector;
  collector.filling = true;
  collector.records = records.span();
  collector.population = population.span();
  Run reference{c, params, budget, ref_space, collector, {}, nullptr, nullptr};
  std::array<u64, dump::kCounters> totals{};
  std::vector<SiteIdx> ids(dump::kMaxLeafSites);
  u64 leafcpp_ns = 0;
  for (u64 j = 0; j < n_leaves; ++j) {
    const dump::Job& job = d.jobs[j];
    const u32* local = d.sites.data() + job.begin;
    for (u32 i = 0; i < job.m; ++i) ids[i] = make_id<SiteIdx>(local[i]);
    Box box;
    for (int a = 0; a < 3; ++a) {
      box.lo[a] = job.lo[a];
      box.hi[a] = job.hi[a];
    }
    reference.ledger = CatalogueLedger{};
    collector.balls = collector.incidences = 0;
    const u64 t = now_ns();
    const Outcome o = enumerate_leaf(reference, std::span<const SiteIdx>(ids.data(), job.m), box);
    leafcpp_ns += now_ns() - t;
    if (!o.ok()) return fail_with(o, "enumerate_leaf");
    const auto counts = counters_of(reference.ledger);
    for (u32 f = 0; f < dump::kCounters; ++f) {
      if (counts[f] > 0xFFFFFFFFu) return fail_with(fail(Reason::catalogue_counter_overflow), "counts");
      d.counts[j * dump::kCounters + f] = static_cast<u32>(counts[f]);
      totals[f] += counts[f];
    }
    d.status[j] = dump::kStatusReference;
    if (collector.balls != counts[4]) return fail_with(fail(Reason::catalogue_invariant), "emitted");
    for (u64 b = 0; b < collector.balls; ++b) {
      const Emission& e = records[b];
      dump::Record r{};
      for (int k = 0; k < 4; ++k) {
        r.support[k] = dump::kNoLocal;
        if (k >= e.ball.qmin) continue;
        const u32* at = std::lower_bound(local, local + job.m, idx(e.ball.support[k]));
        if (at == local + job.m || *at != idx(e.ball.support[k]))
          return fail_with(fail(Reason::catalogue_invariant), "support_rank");
        r.support[k] = static_cast<u8>(at - local);
      }
      r.p = static_cast<u8>(e.ball.p);
      r.m = static_cast<u8>(e.ball.m);
      r.qmin = e.ball.qmin;
      r.pad = 0;
      d.records.push_back(r);
      const u64 n = u64(e.ball.p) + e.ball.m;
      for (u64 k = 0; k < n; ++k) {
        const u32 site = idx(population[e.population_begin + k]);
        const u32* at = std::lower_bound(local, local + job.m, site);
        if (at == local + job.m || *at != site) return fail_with(fail(Reason::catalogue_invariant), "population");
        d.population.push_back(static_cast<u8>(at - local));
      }
    }
    d.record_begin[j + 1] = d.records.size();
    d.population_begin[j + 1] = d.population.size();
  }
  d.header.n_records = d.records.size();
  d.header.n_population = d.population.size();

  // 3. Temoin v11 : feuille source unique sur l'hote. Passe de chrono (puits de comptage), puis passe de capture.
  u64 v11_unresolved = 0, v11_mismatch = 0, v11_count_ns = 0;
  for (u64 j = 0; j < n_leaves; ++j) {
    const dump::Job& job = d.jobs[j];
    leaf_device::Input in;
    in.x = d.x.data();
    in.y = d.y.data();
    in.z = d.z.data();
    in.sites = d.sites.data() + job.begin;
    in.m = job.m;
    for (int a = 0; a < 3; ++a) {
      in.lo[a] = job.lo[a];
      in.hi[a] = job.hi[a];
    }
    in.kmax = static_cast<int>(kmax);
    in.cache = params.cache_center_lines;
    leaf_device::Counts timing_counts;
    leaf_device::CountSink count_sink;
    const u64 t = now_ns();
    const u32 timed = leaf_device::run_leaf(in, timing_counts, count_sink);
    v11_count_ns += now_ns() - t;
    leaf_device::Counts counts;
    CaptureSink sink;
    sink.sites = in.sites;
    sink.m = in.m;
    const u32 status = leaf_device::run_leaf(in, counts, sink);
    if (status != timed) return fail_with(fail(Reason::catalogue_invariant), "v11_device_status");
    if (status != leaf_device::kOk) {
      d.status[j] |= dump::kStatusV11DeviceUnresolved;
      ++v11_unresolved;
      continue;
    }
    const auto device_counts = counters_of(counts);
    bool same = sink.records.size() == d.record_begin[j + 1] - d.record_begin[j] &&
                sink.population.size() == d.population_begin[j + 1] - d.population_begin[j];
    for (u32 f = 0; same && f < dump::kCounters; ++f) same = device_counts[f] == d.counts[j * dump::kCounters + f];
    for (u64 b = 0; same && b < sink.records.size(); ++b)
      same = std::memcmp(&sink.records[b], &d.records[d.record_begin[j] + b], sizeof(dump::Record)) == 0;
    for (u64 k = 0; same && k < sink.population.size(); ++k)
      same = sink.population[k] == d.population[d.population_begin[j] + k];
    if (!same) {
      d.status[j] |= dump::kStatusV11DeviceMismatch;
      ++v11_mismatch;
    }
  }

  const u64 t_write = now_ns();
  std::string error;
  if (!dump::write(out_path, d, error)) {
    std::cerr << error << '\n';
    return 2;
  }
  const u64 write_ns = now_ns() - t_write;

  u64 max_m = 0, sum_m = 0, emitting = 0;
  for (u64 j = 0; j < n_leaves; ++j) {
    max_m = std::max<u64>(max_m, d.jobs[j].m);
    sum_m += d.jobs[j].m;
    emitting += d.record_begin[j + 1] != d.record_begin[j];
  }
  std::cout << "{\"phase\":\"dump\",\"format\":\"MHGP12LF\",\"version\":" << dump::kVersion
            << ",\"coord_bits\":" << kCoordBits << ",\"kmax\":" << kmax << ",\"leaf_size\":" << leaf_size
            << ",\"cache_center_lines\":" << (params.cache_center_lines ? "true" : "false")
            << ",\"sites\":" << c.sites() << ",\"walk_nodes\":" << walker.ledger.nodes
            << ",\"walk_leaves\":" << walker.ledger.leaves << ",\"dumped_leaves\":" << n_leaves
            << ",\"inline_leaves\":" << d.header.walk_inline_leaves << ",\"inline_balls\":" << walk_collector.balls
            << ",\"filter_tests\":" << walker.ledger.filter_tests << ",\"max_depth\":" << walker.ledger.max_depth
            << ",\"max_leaf\":" << walker.ledger.max_leaf << ",\"leaf_sites\":" << sum_m << ",\"max_m\":" << max_m
            << ",\"emitting_leaves\":" << emitting << ",\"records\":" << d.header.n_records
            << ",\"population\":" << d.header.n_population << ",\"reference_counts\":{";
  for (u32 f = 0; f < dump::kCounters; ++f)
    std::cout << (f ? "," : "") << '"' << dump::kCounterNames[f] << "\":" << totals[f];
  std::cout << "},\"v11_device_unresolved\":" << v11_unresolved << ",\"v11_device_mismatch\":" << v11_mismatch
            << ",\"read_ns\":" << read_ns << ",\"walk_ns\":" << walk_ns << ",\"leafcpp_ns\":" << leafcpp_ns
            << ",\"v11_device_host_ns\":" << v11_count_ns << ",\"write_ns\":" << write_ns << "}\n";
  if (v11_mismatch != 0) return 3;
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    return run(argc, argv);
  } catch (const std::bad_alloc&) {
    std::cout << "{\"phase\":\"exit\",\"stage\":\"allocation\",\"reason\":\"memory_budget\"}\n";
    return 2;
  }
}
