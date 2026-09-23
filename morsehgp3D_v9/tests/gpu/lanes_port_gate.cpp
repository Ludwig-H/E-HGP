// MorseHGP3D v9 — porte du port portable de la voie q3 sans atlas (S4a,
// 23 septembre 2026).
//
// Juge : gpu/lanes.hpp compile pour l'hote (groupe de 32 voies emule) rend,
// sur chaque arete certifiee dont la voie q3 reste ouverte, exactement les
// boules q3 de la voie du moteur (gen::engine_q3_records : Engine avec la
// seule voie q3, recensement GlobalBoxes de l'index global, un autre
// algorithme) : multiensemble (arite, support, cle, profondeur, taille et
// empreinte de coquille), et memes graines et emissions sommees.
//   - aretes : les survivants certifies reels du chemin par lots (filtre et
//     certificats CPU de reference), trois familles, K2 (q3 seul), K3, K5, K10,
//     et une fixture gravee cospherique (points entiers de deux spheres de
//     rayon 35, coquilles de plus de trois sites) ;
//   - lot : l'executeur hote (gpu/lanes_host.hpp) rend les memes enregistrements
//     et le meme registre que les appels arete par arete, quel que soit le
//     nombre de fils, et passe check_lanes_batch et judge_lanes_filter ;
//   - capacite reduite : une arete est mise en attente (sans compteur)
//     exactement quand son cover depasse l'ardoise, ou ses boules l'ardoise
//     d'enregistrements, ou sa reservation l'arene (ordre des aretes) ; les
//     autres gardent les memes enregistrements ;
//   - planchers : aretes q3 seules, rejets en profondeur, boules emises,
//     coquilles de plus de trois sites, mises en attente, tous non nuls ;
//   - mutants de la sortie (juge et controle de frontiere) : un
//     enregistrement retire, duplique, deplace, une profondeur, une cle, une
//     empreinte ou une coquille faussee, une voie decidee non demandee, un
//     registre faux d'une unite sont refuses ;
//   - garde d'entree (validate_lanes_input) : l'entree reelle est acceptee,
//     chaque champ forge un a un est refuse.
//
//   mhgp9_gpu_lanes_port_gate [--n=2000] [--k=2,3,5,10]
//   mhgp9_gpu_lanes_port_gate --file=nuage.u32le --k=5 [--workers=8]
//     (statistiques de travail par arete sur un nuage fichier, sans porte)
//
// Code 0 conforme, 1 desaccord ou mutant survivant (`cause=`), 2 argument,
// 3 plancher.
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "../../src/gen/pipeline/wspd_q34.hpp"
#include "../../src/gpu/flat_index.hpp"
#include "../../src/gpu/lanes_host.hpp"
#include "../gen/front_fixtures.hpp"

namespace {

using namespace mhgp9;

int fail(const std::string& cause) {
  std::printf("cause=%s\n", cause.c_str());
  return 1;
}

gen::WspdQ34Options q34_options() {
  gen::WspdQ34Options o;
  o.front_mode = gen::WspdFrontMode::MidpointSamples;
  o.requested_lane_mask = 6;
  o.q4_backend = gen::WspdQ4Backend::Local28;
  o.local = gen::Q4LocalOptions{};
  o.local.saturate_deep = true;
  o.local.retain_q3_fragments = true;
  o.witness_mode = gen::WspdQ34WitnessMode::RectanglePair;
  o.q3_census_mode = gen::WspdQ3CensusMode::GlobalBoxes;
  o.witness_bounds_mode = gen::Q34WitnessBoundsMode::Affine;
  o.q4_seed_cells = gen::Q4SeedCellOptions{gen::Q4SeedCellMode::LiveOnly, 64};
  o.q3_atlas_consultation = true;
  o.q3_leaf_census = true;
  o.dead_lanes = true;
  o.dead_core = true;
  o.jobs_by_mass = true;
  return o;
}

// The index as the device sees it.
struct Flat {
  std::vector<gpu::FlatNode> nodes;
  std::vector<gpu::u32> escapes, rank_ids;
  std::vector<std::int32_t> rank_points;
  explicit Flat(const gen::Q2CensusIndex& index)
      : nodes(gpu::flatten_nodes(index)), escapes(gpu::flatten_escapes(index)) {
    for (const auto& p : index.spatial_points()) {
      rank_points.push_back(p.x);
      rank_points.push_back(p.y);
      rank_points.push_back(p.z);
    }
    for (const auto id : index.spatial_order()) rank_ids.push_back(static_cast<gpu::u32>(id));
  }
  gpu::LanesIndex view() const {
    return gpu::LanesIndex{gpu::CertificateIndex{nodes.data(), escapes.data(), static_cast<gpu::u32>(nodes.size()),
                                                 rank_points.data()},
                           rank_ids.data()};
  }
  gpu::LanesInput input(unsigned kmax, const std::vector<gpu::u32>& a, const std::vector<gpu::u32>& b) const {
    gpu::LanesInput in;
    in.index.nodes = nodes.data();
    in.index.node_count = nodes.size();
    in.index.rank_points = rank_points.data();
    in.index.rank_count = rank_points.size() / 3;
    in.index.kmax = kmax;
    in.escapes = escapes.data();
    in.rank_ids = rank_ids.data();
    in.edge_a = a.data();
    in.edge_b = b.data();
    in.edge_count = a.size();
    return in;
  }
};

struct Slab {
  std::vector<gpu::u32> ranges, ranks, seeds, scratch;
  std::vector<std::int32_t> points;
  std::vector<gpu::LaneRecord> records;
  gpu::LanesSlab view{};
  Slab(gpu::u32 capacity, gpu::u32 record_capacity)
      : ranges(2 * std::size_t{capacity}), ranks(capacity), seeds(capacity), scratch(capacity),
        points(3 * std::size_t{capacity}), records(record_capacity) {
    view = gpu::LanesSlab{ranges.data(),  points.data(),  ranks.data(), seeds.data(),
                          scratch.data(), records.data(), capacity,     record_capacity};
  }
};

gen::Q34LaneRecord record_of(const gpu::LaneRecord& from, std::uint32_t edge) {
  gen::Q34LaneRecord to;
  for (int c = 0; c < 5; ++c) to.key[c] = from.key[c];
  for (int c = 0; c < 4; ++c) to.support[c] = from.support[c];
  to.edge = edge;
  to.depth = from.depth;
  to.shell = from.shell;
  to.arity = static_cast<std::uint8_t>(from.arity);
  to.shell_sum = from.shell_sum;
  to.shell_xor = from.shell_xor;
  return to;
}

bool record_less(const gen::Q34LaneRecord& a, const gen::Q34LaneRecord& b) {
  if (a.key != b.key) return a.key < b.key;
  if (a.support != b.support) return a.support < b.support;
  if (a.depth != b.depth) return a.depth < b.depth;
  if (a.shell != b.shell) return a.shell < b.shell;
  if (a.shell_sum != b.shell_sum) return a.shell_sum < b.shell_sum;
  return a.shell_xor < b.shell_xor;
}

bool same_records(std::vector<gen::Q34LaneRecord> a, std::vector<gen::Q34LaneRecord> b) {
  for (auto& r : a) r.edge = 0;
  for (auto& r : b) r.edge = 0;
  std::sort(a.begin(), a.end(), record_less);
  std::sort(b.begin(), b.end(), record_less);
  return a == b;
}

gen::Q34LanesWork work_of(const gpu::Q3Work& w) {
  gen::Q34LanesWork t;
  t.edges = w.edges;
  t.cover_sites = w.cover_sites;
  t.max_cover_sites = w.max_cover_sites;
  t.cover = gen::Q34EdgeCoverWork{w.cover.node_visits,    w.cover.bound_tests,    w.cover.point_tests,
                                  w.cover.admitted_nodes, w.cover.rejected_nodes, w.cover.split_nodes,
                                  w.cover.admitted_sites, w.cover.rejected_sites, w.cover.retained_ranges,
                                  w.cover.merged_ranges};
  t.seed_tests = w.seed_tests;
  t.acute_sites = w.acute_sites;
  t.owner_rejections = w.owner_rejections;
  t.seeds = w.seeds;
  t.census_point_tests = w.census_point_tests;
  t.census_inside_sites = w.census_inside_sites;
  t.census_shell_sites = w.census_shell_sites;
  t.census_outside_sites = w.census_outside_sites;
  t.depth_rejections = w.depth_rejections;
  t.emitted = w.emitted;
  t.shell_ids = w.shell_ids;
  return t;
}

// The host batch output mapped to survivor ordinals (as the chain does).
gen::Q34LanesBatch batch_of(const gpu::LanesOutput& out, const std::vector<std::size_t>& where, std::size_t n) {
  gen::Q34LanesBatch batch;
  batch.backend = out.device;
  batch.decided.assign(n, 0);
  batch.record_begin.assign(n, 0);
  batch.record_count.assign(n, 0);
  for (std::size_t i = 0; i < where.size(); ++i)
    if (out.status[i] == static_cast<gpu::u8>(gpu::CertificateStatus::decided)) {
      batch.decided[where[i]] = 2;
      batch.record_begin[where[i]] = out.record_begin[i];
      batch.record_count[where[i]] = out.record_count[i];
    }
  for (const auto& r : out.records) batch.records.push_back(record_of(r, static_cast<std::uint32_t>(where[r.edge])));
  batch.work = work_of(out.work);
  return batch;
}

std::vector<unsigned> parse_list(std::string_view text) {
  std::vector<unsigned> out;
  while (!text.empty()) {
    const auto comma = text.find(',');
    const auto item = text.substr(0, comma);
    unsigned value = 0;
    const auto [end, error] = std::from_chars(item.data(), item.data() + item.size(), value);
    if (item.empty() || error != std::errc{} || end != item.data() + item.size()) return {};
    out.push_back(value);
    if (comma == std::string_view::npos) break;
    text.remove_prefix(comma + 1);
  }
  return out;
}

// Survivors and S3 certificates of the real batch path (CPU references).
struct Certified {
  std::vector<gen::Q34SurvivingEdge> survivors;
  gen::Q34CertificateBatch certificates;
};

Certified certified_survivors(const gen::Q2CensusIndexPtr& index, unsigned kmax, std::size_t workers) {
  Certified out;
  const gen::Q34BatchFilter filter = [&](const gen::Q2CensusIndex& ix, unsigned k,
                                         std::span<const gen::WspdRectangle> rectangles) {
    return gen::run_q34_filter_batch_cpu(ix, k, rectangles, workers);
  };
  const gen::Q34CertificateFilter certificates = [&](const gen::Q2CensusIndexPtr& ix, unsigned k, bool core,
                                                     std::span<const gen::Q34SurvivingEdge> edges) {
    auto c = gen::run_q34_certificate_batch_cpu(ix, k, core, edges, workers);
    out.survivors.assign(edges.begin(), edges.end());
    out.certificates = c;
    return c;
  };
  static_cast<void>(gen::run_wspd_q34_batched(index, kmax, 8, q34_options(), workers,
                                              [](std::size_t, const gen::Q34SeedCandidate&) {}, 16, filter, nullptr,
                                              &certificates));
  return out;
}

// Per-edge work statistics of the q3 lanes on one cloud (no gate).
int file_stats(const std::string& path, unsigned kmax, std::size_t workers) {
  std::ifstream in(path, std::ios::binary);
  std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (!in.eof() && !in.good()) return 2;
  if (bytes.empty() || bytes.size() % 12 != 0) return 2;
  std::vector<gen::Point3> points(bytes.size() / 12);
  for (std::size_t i = 0; i < points.size(); ++i) {
    std::uint32_t c[3];
    std::memcpy(c, bytes.data() + 12 * i, 12);
    points[i] = gen::Point3{static_cast<std::int32_t>(c[0]), static_cast<std::int32_t>(c[1]),
                            static_cast<std::int32_t>(c[2])};
  }
  const auto index = gen::make_q2_cloud_index(gen::prepare_cloud(points));
  const auto certified = certified_survivors(index, kmax, workers);
  const Flat flat(*index);
  const auto view = flat.view();
  Slab slab(gpu::default_lanes_capacity, gpu::default_record_capacity);
  struct Row {
    unsigned long long tests, sites, seeds, emitted, steps;
  };
  std::vector<Row> rows;
  for (std::size_t j = 0; j < certified.survivors.size(); ++j) {
    if (certified.certificates.deferred[j] != 0 || (certified.certificates.masks[j] & 2U) == 0) continue;
    gpu::u32 count = 0;
    gpu::EdgeQ3Work local{};
    const auto status = gpu::q3_lane(gpu::HostGroup{}, view, certified.survivors[j].a_rank,
                                     certified.survivors[j].b_rank, kmax, slab.view, count, local);
    if (status != gpu::CertificateStatus::decided) return fail("stats.undecided");
    // Warp steps of the site-parallel censuses: at most ceil(sites/32) per seed.
    rows.push_back({local.census_point_tests, local.cover_sites, local.seeds, local.emitted,
                    (local.census_point_tests + 31) / 32});
  }
  const auto report = [&](const char* name, auto field) {
    std::vector<unsigned long long> v;
    unsigned long long sum = 0;
    for (const auto& r : rows) {
      v.push_back(field(r));
      sum += field(r);
    }
    std::sort(v.begin(), v.end());
    const auto at = [&](double q) { return v.empty() ? 0ULL : v[static_cast<std::size_t>(q * (v.size() - 1))]; };
    std::printf("%s sum=%llu p50=%llu p90=%llu p99=%llu p999=%llu max=%llu\n", name, sum, at(0.5), at(0.9),
                at(0.99), at(0.999), v.empty() ? 0ULL : v.back());
  };
  std::printf("lanes_stats file=%s K=%u edges=%zu\n", path.c_str(), kmax, rows.size());
  report("census_point_tests", [](const Row& r) { return r.tests; });
  report("cover_sites", [](const Row& r) { return r.sites; });
  report("seeds", [](const Row& r) { return r.seeds; });
  report("emitted", [](const Row& r) { return r.emitted; });
  report("warp_steps_lower_bound", [](const Row& r) { return r.steps; });
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  std::size_t n = 2000, workers = 4;
  std::vector<unsigned> ks{2, 3, 5, 10};
  std::string file;
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (arg.starts_with("--n=")) {
      const auto digits = arg.substr(4);
      const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), n);
      if (digits.empty() || error != std::errc{} || end != digits.data() + digits.size()) return 2;
    } else if (arg.starts_with("--workers=")) {
      const auto digits = arg.substr(10);
      const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), workers);
      if (digits.empty() || error != std::errc{} || end != digits.data() + digits.size() || workers == 0) return 2;
    } else if (arg.starts_with("--k=")) {
      ks = parse_list(arg.substr(4));
      if (ks.empty()) return 2;
    } else if (arg.starts_with("--file=")) {
      file = std::string(arg.substr(7));
    } else {
      return 2;
    }
  }
  if (n < 64 || n > 65536 || std::any_of(ks.begin(), ks.end(), [](unsigned k) { return k < 2 || k > 10; })) {
    std::fprintf(stderr, "usage: mhgp9_gpu_lanes_port_gate [--n=2000] [--k=2,3,5,10]\n");
    return 2;
  }
  if (!file.empty()) {
    if (ks.size() != 1) return 2;
    return file_stats(file, ks[0], workers);
  }
  unsigned long long edges = 0, q3_only = 0, rejections = 0, emitted = 0, wide_shells = 0, deferred = 0,
                     mutants = 0, guards = 0, judged = 0;
  const auto options = q34_options();
  // Engraved cospherical fixture: the integer points of the spheres of
  // radius 5 (scaled by 7) and 7 (scaled by 5), both of radius 35, around two
  // centres 100 apart: every triangle on a great circle has a q3 ball whose
  // shell is the whole circle's points.
  std::vector<gen::Point3> spheres;
  for (const std::int32_t cx : {1000, 1100})
    for (int x = -7; x <= 7; ++x)
      for (int y = -7; y <= 7; ++y)
        for (int z = -7; z <= 7; ++z) {
          const int r2 = x * x + y * y + z * z;
          if (r2 == 25) spheres.push_back({cx + 7 * x, 1000 + 7 * y, 1000 + 7 * z});
          if (r2 == 49 && (x * y != 0 || y * z != 0 || x * z != 0))  // axis points are already there
            spheres.push_back({cx + 5 * x, 1000 + 5 * y, 1000 + 5 * z});
        }
  for (const std::string_view family : {"uniform", "terrain", "clusters", "spheres"}) {
    const auto points = family == "spheres" ? spheres : gen::bench::make_front_fixture(n, family, 3).points;
    const auto index = gen::make_q2_cloud_index(gen::prepare_cloud(points));
    const Flat flat(*index);
    const auto view = flat.view();
    const auto order = index->spatial_order();
    bool guarded = false;
    for (const unsigned kmax : ks) {
      const std::string where_name = std::string(family) + "/K" + std::to_string(kmax);
      const auto certified = certified_survivors(index, kmax, workers);
      const auto& survivors = certified.survivors;
      std::vector<std::uint8_t> asked(survivors.size(), 0);
      std::vector<std::size_t> where;
      std::vector<gpu::u32> ea, eb;
      for (std::size_t j = 0; j < survivors.size(); ++j)
        if (certified.certificates.deferred[j] == 0 && (certified.certificates.masks[j] & 2U) != 0) {
          asked[j] = 2;
          where.push_back(j);
          ea.push_back(survivors[j].a_rank);
          eb.push_back(survivors[j].b_rank);
          if ((certified.certificates.masks[j] & 4U) == 0) ++q3_only;
        }
      // 1. Edge by edge: the host compilation against the engine's q3 lane.
      Slab slab(gpu::default_lanes_capacity, gpu::default_record_capacity);
      gpu::Q3Work direct{};
      std::vector<std::vector<gen::Q34LaneRecord>> per_edge(where.size());
      std::vector<gpu::u32> cover_sites(where.size()), counts(where.size());
      for (std::size_t i = 0; i < where.size(); ++i) {
        gpu::u32 count = 0;
        gpu::EdgeQ3Work local{};
        const auto status = gpu::q3_lane(gpu::HostGroup{}, view, ea[i], eb[i], kmax, slab.view, count, local);
        if (status != gpu::CertificateStatus::decided) return fail("edge.undecided " + where_name);
        gpu::add_q3_edge(direct, local);
        cover_sites[i] = local.cover_sites;
        counts[i] = count;
        for (gpu::u32 r = 0; r < count; ++r) per_edge[i].push_back(record_of(slab.records[r], 0));
        gen::WspdQ3Work q3{};
        const auto reference = gen::engine_q3_records(index, kmax, options, order[ea[i]], order[eb[i]], &q3);
        if (!same_records(per_edge[i], reference)) return fail("edge.records " + where_name);
        if (q3.seeds != local.seeds || q3.emitted != local.emitted)
          return fail("edge.seeds_or_emitted " + where_name);
        for (const auto& r : per_edge[i]) wide_shells += r.shell > 3 ? 1U : 0U;
      }
      edges += where.size();
      rejections += direct.depth_rejections;
      emitted += direct.emitted;
      // 2. The batch runner: same records and ledger, 1 and 4 threads, and
      // the trust boundary plus the judge accept it.
      const auto in = flat.input(kmax, ea, eb);
      for (const std::size_t threads : {std::size_t{1}, workers}) {
        const auto out = gpu::run_lanes_batch_host(in, threads);
        if (!out.error.empty() || out.deferred != 0 || out.faults != 0) return fail("batch.status " + where_name);
        const auto batch = batch_of(out, where, survivors.size());
        if (batch.work != work_of(direct)) return fail("batch.work " + where_name);
        for (std::size_t i = 0; i < where.size(); ++i) {
          const auto j = where[i];
          std::vector<gen::Q34LaneRecord> slice(batch.records.begin() + batch.record_begin[j],
                                                batch.records.begin() + batch.record_begin[j] + batch.record_count[j]);
          if (!same_records(slice, per_edge[i])) return fail("batch.records " + where_name);
        }
        try {
          gen::check_lanes_batch(batch, *index, kmax, survivors, asked);
        } catch (const std::exception& e) {
          return fail(std::string("batch.check ") + where_name + " " + e.what());
        }
      }
      const auto reference_batch = batch_of(gpu::run_lanes_batch_host(in, workers), where, survivors.size());
      gen::Q34LanesJudgeWork judge_work;
      const auto judge = gen::judge_lanes_filter(
          [&](const gen::Q2CensusIndexPtr&, unsigned, std::span<const gen::Q34SurvivingEdge>,
              std::span<const std::uint8_t>) { return reference_batch; },
          options, workers, &judge_work);
      try {
        static_cast<void>(judge(index, kmax, survivors, asked));
      } catch (const std::exception& e) {
        return fail(std::string("judge.refused_real ") + where_name + " " + e.what());
      }
      judged += judge_work.judged;
      // 3. Mutants of the output: each must be refused by the boundary or the judge.
      if (!reference_batch.records.empty()) {
        std::size_t target = 0;
        for (std::size_t j = 0; j < survivors.size(); ++j)
          if (reference_batch.record_count[j] != 0) {
            target = j;
            break;
          }
        const auto first = reference_batch.record_begin[target];
        std::vector<std::pair<const char*, std::function<void(gen::Q34LanesBatch&)>>> cases{
            {"drop", [&](gen::Q34LanesBatch& b) {
               b.records.erase(b.records.begin() + first);
               --b.record_count[target];
               for (auto& begin : b.record_begin)
                 if (begin > first) --begin;
               --b.work.emitted;
               --b.work.seeds;
               b.work.shell_ids -= reference_batch.records[first].shell;
               b.work.census_shell_sites -= reference_batch.records[first].shell;
               b.work.census_outside_sites += reference_batch.records[first].shell;
             }},
            {"duplicate", [&](gen::Q34LanesBatch& b) {
               b.records.push_back(b.records[first]);
               ++b.record_count[target];
             }},
            {"depth", [&](gen::Q34LanesBatch& b) { b.records[first].depth ^= 1U; }},
            {"key", [&](gen::Q34LanesBatch& b) { b.records[first].key[4] += 1; }},
            {"fingerprint", [&](gen::Q34LanesBatch& b) { b.records[first].shell_xor ^= 1U; }},
            {"shell", [&](gen::Q34LanesBatch& b) {
               ++b.records[first].shell;
               ++b.work.shell_ids;
             }},
            {"edge_field", [&](gen::Q34LanesBatch& b) { b.records[first].edge ^= 1U; }},
            {"widened", [&](gen::Q34LanesBatch& b) {
               for (std::size_t j = 0; j < survivors.size(); ++j)
                 if (asked[j] == 0) {
                   b.decided[j] = 2;
                   return;
                 }
               b.decided[target] = 6;
             }},
            {"seeds", [&](gen::Q34LanesBatch& b) { ++b.work.seeds; }},
            {"census", [&](gen::Q34LanesBatch& b) { ++b.work.census_point_tests; }},
        };
        for (const auto& [name, mutate] : cases) {
          auto mutated = reference_batch;
          mutate(mutated);
          const auto mutant = gen::judge_lanes_filter(
              [&](const gen::Q2CensusIndexPtr&, unsigned, std::span<const gen::Q34SurvivingEdge>,
                  std::span<const std::uint8_t>) { return mutated; },
              options, workers, nullptr);
          bool refused = false;
          try {
            static_cast<void>(mutant(index, kmax, survivors, asked));
          } catch (const std::logic_error&) {
            refused = true;
          }
          if (!refused) return fail(std::string("mutant.survived ") + name + " " + where_name);
          ++mutants;
        }
      }
      // 4. Reduced capacities: exactly the edges beyond them are deferred.
      if (!where.empty()) {
        std::vector<gpu::u32> sorted = cover_sites;
        std::sort(sorted.begin(), sorted.end());
        const gpu::u32 capacity = std::max<gpu::u32>(2, sorted[sorted.size() / 2]);
        auto small = in;
        small.capacity = capacity;
        small.record_capacity = 1;
        std::size_t total = 0;
        for (const auto c : counts) total += c;
        small.arena_capacity = total / 2;
        const auto out = gpu::run_lanes_batch_host(small, workers);
        if (!out.error.empty() || out.faults != 0) return fail("small.status " + where_name);
        unsigned long long reserved = 0;
        for (std::size_t i = 0; i < where.size(); ++i) {
          bool expect_deferred = cover_sites[i] > capacity || counts[i] > 1;
          if (!expect_deferred) {
            reserved += counts[i];
            expect_deferred = reserved > small.arena_capacity;
          }
          const bool is_deferred = out.status[i] == static_cast<gpu::u8>(gpu::CertificateStatus::deferred);
          if (expect_deferred != is_deferred) return fail("small.deferral " + where_name);
          if (is_deferred) {
            ++deferred;
            continue;
          }
          std::vector<gen::Q34LaneRecord> slice;
          for (gpu::u32 r = 0; r < out.record_count[i]; ++r)
            slice.push_back(record_of(out.records[out.record_begin[i] + r], 0));
          if (!same_records(slice, per_edge[i])) return fail("small.records " + where_name);
        }
      }
      // 5. Input guard (never a device call here).
      if (!guarded && ea.size() >= 2) {
        guarded = true;
        if (!gpu::validate_lanes_input(in).empty()) return fail("guard.real_refused " + where_name);
        std::vector<std::pair<const char*, gpu::LanesInput>> forged;
        {
          auto f = in;
          f.rank_ids = nullptr;
          forged.emplace_back("null rank ids", f);
        }
        {
          auto f = in;
          f.index.kmax = 1;
          forged.emplace_back("K1", f);
        }
        {
          auto f = in;
          f.capacity = 1;
          forged.emplace_back("capacity one", f);
        }
        {
          auto f = in;
          f.escapes = nullptr;
          forged.emplace_back("null escapes", f);
        }
        {
          auto f = in;
          f.arena_capacity = std::size_t{1} << 33;
          forged.emplace_back("arena beyond u32", f);
        }
        auto same = eb;
        same[1] = ea[1];
        auto outside = ea;
        outside[1] = static_cast<gpu::u32>(flat.rank_ids.size());
        {
          auto f = in;
          f.edge_b = same.data();
          forged.emplace_back("same endpoints", f);
        }
        {
          auto f = in;
          f.edge_a = outside.data();
          forged.emplace_back("rank outside", f);
        }
        for (const auto& [name, f] : forged) {
          if (gpu::validate_lanes_input(f).empty()) return fail(std::string("guard.accepted ") + name);
          ++guards;
        }
      }
    }
  }
  std::printf("lanes_port_gate n=%zu edges=%llu q3_only=%llu depth_rejections=%llu emitted=%llu wide_shells=%llu "
              "deferred=%llu judged=%llu mutants=%llu guards=%llu\n",
              n, edges, q3_only, rejections, emitted, wide_shells, deferred, judged, mutants, guards);
  if (edges == 0 || q3_only == 0 || rejections == 0 || emitted == 0 || wide_shells == 0 || deferred == 0 ||
      judged == 0 || guards == 0 || mutants == 0)
    return 3;
  return 0;
}
