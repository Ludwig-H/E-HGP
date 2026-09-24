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
//     chaque champ forge un a un est refuse ;
//   - arene par defaut depassee sous l'ardoise de sites (fixture exacte et
//     compacte de l'auditeur : 45 grappes de 110 sites, 108 boules q3 par
//     arete a K5, 4 860 enregistrements pour une arene de 4 816) : la seule
//     derniere arete mise en attente, les autres egales au moteur ;
//   - S4b, voie q4 (gpu/q4_lanes.hpp) : sur chaque arete certifiee dont la
//     voie q4 reste ouverte, les tetraedres de la voie q4 du moteur
//     (gen::engine_q4_records, Local28 a atlas) ; lot mixte q3 + q4 accepte
//     par check_lanes_batch et judge_lanes_filter ; fixture de l'auditeur B
//     (une graine, deux groupes, deux tetraedres a K3) ; tampon d'evenements
//     reduit : mises en attente, les autres aretes egales ; mutants q4 ;
//   - panne d'allocation de l'executeur hote (ardoise d'enregistrements
//     demesuree, auditeur A) : exception rendue a l'appelant apres jointure de
//     tous les fils, jamais une terminaison, a un et a plusieurs fils ; un lot
//     vide rend un resultat vide.
//
//   mhgp9_gpu_lanes_port_gate [--n=2000] [--k=2,3,5,10]
//   mhgp9_gpu_lanes_port_gate --file=nuage.u32le --k=5 [--workers=8] [--compare
//     [--min-q3-records=N] [--min-q4-seeds=N] [--min-q4-records=N] [--all-asked]]
//     (statistiques de travail par arete sur un nuage fichier ; avec
//     --compare, les voies q3 et q4 de chaque arete certifiee comparees a
//     celles du moteur : code 1 au premier ecart. Les exclusions sont
//     publiees : survivantes, certificats reportes, aretes sans voie, voies
//     reportees par l'ardoise (repli moteur dans la chaine). Au moins un
//     plancher est exige (code 2 sinon) ; code 3 sous un plancher, ou avec
//     --all-asked si une arete demandee n'est pas comparee ; code 1 si les
//     identites du registre q4 du lecteur sont violees ; code 2 pour un
//     nuage refuse par prepare_cloud.)
//
// Code 0 conforme, 1 desaccord ou mutant survivant (`cause=`), 2 argument,
// 3 plancher.
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <functional>
#include <iterator>
#include <new>
#include <stdexcept>
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

struct Q4Buffers {
  std::vector<gpu::u32> positions, bits, list;
  gpu::Q4Slab view{};
  explicit Q4Buffers(gpu::u32 capacity) : positions(capacity), bits(capacity), list(capacity) {
    view = gpu::Q4Slab{positions.data(), bits.data(), list.data(), capacity};
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

gen::Q34Lanes4Work work4_of(const gpu::Q4Work& w) {
  return gen::Q34Lanes4Work{w.edges, w.seeds, w.certified, w.certified_chunk1, w.survivors, w.pass_chunks,
      w.pass_site_tests, w.buffered_events, w.max_buffered, w.live_buckets, w.filter_steps, w.bucket_events,
      w.candidates, w.foreign_candidates, w.groups, w.compare_steps, w.depth_rejected_groups, w.positivity_tests,
      w.groups_without_valid, w.emitted, w.emitting_seeds, w.multi_emission_seeds, w.max_emissions_per_seed,
      w.shell_ids, w.max_group, w.constant_shell_sites, w.list_steps, w.group_steps};
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
  t.q3_edges = w.q3_edges;
  t.census_seeds = w.census_seeds;
  return t;
}

// The host batch output mapped to survivor ordinals (as the chain does).
gen::Q34LanesBatch batch_of(const gpu::LanesOutput& out, const std::vector<std::size_t>& where, std::size_t n,
                            const std::vector<gpu::u8>* lanes = nullptr) {
  gen::Q34LanesBatch batch;
  batch.backend = out.device;
  batch.decided.assign(n, 0);
  batch.record_begin.assign(n, 0);
  batch.record_count.assign(n, 0);
  batch.work4 = work4_of(out.work4);
  for (std::size_t i = 0; i < where.size(); ++i)
    if (out.status[i] == static_cast<gpu::u8>(gpu::CertificateStatus::decided)) {
      batch.decided[where[i]] = lanes == nullptr ? 2 : (*lanes)[i];
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
// Every certified edge's asked lanes (q3 and, from K3, q4) against the
// engine's own lanes, edge by edge, on a file cloud (reports).
struct CompareFloors {
  unsigned long long q3_records = 0, q4_seeds = 0, q4_records = 0;
  bool all_asked = false;
};

int file_compare(const gen::Q2CensusIndexPtr& index, const Certified& certified, unsigned kmax,
                 const CompareFloors& floors) {
  const Flat flat(*index);
  const auto view = flat.view();
  const auto order = index->spatial_order();
  const auto options = q34_options();
  Slab slab(gpu::default_lanes_capacity, gpu::default_record_capacity);
  Q4Buffers q4b(gpu::default_event_capacity);
  gpu::Q3Work w3{};
  gpu::Q4Work w4{};
  unsigned long long edges = 0, q3_edges = 0, q4_edges = 0, records3 = 0, records4 = 0;
  unsigned long long certificate_deferred = 0, no_lanes = 0, q3_asked = 0, q4_asked = 0, lanes_deferred = 0;
  for (std::size_t j = 0; j < certified.survivors.size(); ++j) {
    // Excluded edges are counted: in the chain a deferred certificate or
    // lanes call falls back to the engine itself (both lanes).
    if (certified.certificates.deferred[j] != 0) {
      ++certificate_deferred;
      continue;
    }
    const auto lanes = static_cast<gpu::u8>(certified.certificates.masks[j] & (kmax >= 3 ? 6U : 2U));
    if (lanes == 0) {
      ++no_lanes;
      continue;
    }
    q3_asked += (lanes & 2U) != 0;
    q4_asked += (lanes & 4U) != 0;
    gpu::u32 count = 0;
    gpu::EdgeQ3Work l3{};
    gpu::Q4Work l4{};
    const auto& e = certified.survivors[j];
    const auto status =
        gpu::edge_lanes(gpu::HostGroup{}, view, e.a_rank, e.b_rank, lanes, kmax, slab.view, q4b.view, count, l3, l4);
    if (status == gpu::CertificateStatus::deferred) {
      ++lanes_deferred;
      continue;
    }
    if (status != gpu::CertificateStatus::decided) return fail("compare.fault");
    // A record or q4 work of a lane that was not asked is a port fault.
    std::vector<gen::Q34LaneRecord> mine3, mine4;
    for (gpu::u32 r = 0; r < count; ++r) {
      const auto arity = slab.records[r].arity;
      if ((arity == 3 && (lanes & 2U) == 0) || (arity == 4 && (lanes & 4U) == 0) || (arity != 3 && arity != 4))
        return fail("compare.unasked_arity edge " + std::to_string(j));
      (arity == 3 ? mine3 : mine4).push_back(record_of(slab.records[r], 0));
    }
    if ((lanes & 4U) == 0 && l4.seeds != 0) return fail("compare.unasked_q4 edge " + std::to_string(j));
    if ((lanes & 2U) != 0) {
      if (!same_records(mine3, gen::engine_q3_records(index, kmax, options, order[e.a_rank], order[e.b_rank])))
        return fail("compare.q3 edge " + std::to_string(j));
      ++q3_edges;
    }
    if ((lanes & 4U) != 0) {
      if (!same_records(mine4, gen::engine_q4_records(index, kmax, options, order[e.a_rank], order[e.b_rank])))
        return fail("compare.q4 edge " + std::to_string(j));
      ++q4_edges;
    }
    ++edges;
    records3 += mine3.size();
    records4 += mine4.size();
    gpu::add_q3_edge(w3, l3);
    gpu::add_q4(w4, l4);
  }
  const bool all = certificate_deferred == 0 && lanes_deferred == 0;
  const bool floors_ok = records3 >= floors.q3_records && w4.seeds >= floors.q4_seeds &&
                         records4 >= floors.q4_records && (!floors.all_asked || all);
  std::printf("lanes_compare K=%u survivors=%zu certificate_deferred=%llu no_lanes=%llu q3_asked=%llu "
              "q4_asked=%llu lanes_deferred=%llu edges=%llu q3_edges=%llu q4_edges=%llu q3_records=%llu "
              "q4_seeds=%llu q4_records=%llu all_asked_compared=%d floors_ok=%d equal=1\n",
              kmax, certified.survivors.size(), certificate_deferred, no_lanes, q3_asked, q4_asked,
              lanes_deferred, edges, q3_edges, q4_edges, records3, w4.seeds, records4, all ? 1 : 0,
              floors_ok ? 1 : 0);
  std::printf("q4_ledger seeds=%llu certified=%llu certified_chunk1=%llu survivors=%llu pass_chunks=%llu "
              "buffered_events=%llu max_buffered=%llu live_buckets=%llu groups=%llu compare_steps=%llu "
              "depth_rejected_groups=%llu groups_without_valid=%llu emitted=%llu multi_emission_seeds=%llu "
              "max_emissions_per_seed=%llu max_group=%llu foreign_candidates=%llu filter_steps=%llu "
              "bucket_events=%llu list_steps=%llu group_steps=%llu\n",
              w4.seeds, w4.certified, w4.certified_chunk1, w4.survivors, w4.pass_chunks, w4.buffered_events,
              w4.max_buffered, w4.live_buckets, w4.groups, w4.compare_steps, w4.depth_rejected_groups,
              w4.groups_without_valid, w4.emitted, w4.multi_emission_seeds, w4.max_emissions_per_seed, w4.max_group,
              w4.foreign_candidates, w4.filter_steps, w4.bucket_events, w4.list_steps, w4.group_steps);
  // The reader's identities of the q4 ledger (tower_worker_v9 validate_lanes)
  // on the exact per-edge work.
  if (w4.seeds != w4.certified + w4.survivors || w4.certified_chunk1 > w4.certified ||
      w4.groups != w4.depth_rejected_groups + w4.groups_without_valid + w4.emitted || w4.emitted != records4 ||
      w4.list_steps < w4.filter_steps || 32 * w4.list_steps < w4.bucket_events ||
      w4.group_steps < 2 * w4.compare_steps || w4.compare_steps < w4.groups)
    return fail("compare.q4_ledger");
  return floors_ok ? 0 : 3;
}

int file_stats(const std::string& path, unsigned kmax, std::size_t workers, bool compare,
               const CompareFloors& floors) {
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
  gen::Q2CensusIndexPtr index;
  try {
    index = gen::make_q2_cloud_index(gen::prepare_cloud(points));
  } catch (const std::invalid_argument& e) {
    std::fprintf(stderr, "refused cloud: %s\n", e.what());
    return 2;
  }
  const auto certified = certified_survivors(index, kmax, workers);
  if (compare) return file_compare(index, certified, kmax, floors);
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
  bool compare = false;
  CompareFloors floors;
  const auto floor_of = [](std::string_view text, unsigned long long& out) {
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), out);
    return error == std::errc{} && end == text.data() + text.size();
  };
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
      if (file.empty()) return 2;
    } else if (arg == "--compare") {
      compare = true;
    } else if (arg.starts_with("--min-q3-records=")) {
      if (!floor_of(arg.substr(17), floors.q3_records)) return 2;
    } else if (arg.starts_with("--min-q4-seeds=")) {
      if (!floor_of(arg.substr(15), floors.q4_seeds)) return 2;
    } else if (arg.starts_with("--min-q4-records=")) {
      if (!floor_of(arg.substr(17), floors.q4_records)) return 2;
    } else if (arg == "--all-asked") {
      floors.all_asked = true;
    } else {
      return 2;
    }
  }
  if (n < 64 || n > 65536 || std::any_of(ks.begin(), ks.end(), [](unsigned k) { return k < 2 || k > 10; })) {
    std::fprintf(stderr, "usage: mhgp9_gpu_lanes_port_gate [--n=2000] [--k=2,3,5,10]\n");
    return 2;
  }
  // Floors only with --compare, --compare only on a file and never without
  // a floor (a comparison of nothing would be equal).
  const bool any_floor = floors.q3_records != 0 || floors.q4_seeds != 0 || floors.q4_records != 0;
  if (!compare && (any_floor || floors.all_asked)) return 2;
  if (compare && (file.empty() || !any_floor)) return 2;
  if (!file.empty()) {
    if (ks.size() != 1) return 2;
    try {
      return file_stats(file, ks[0], workers, compare, floors);
    } catch (const std::exception& e) {
      return fail(std::string("file.exception ") + e.what());
    }
  }
  unsigned long long edges = 0, q3_only = 0, rejections = 0, emitted = 0, wide_shells = 0, deferred = 0,
                     mutants = 0, guards = 0, judged = 0, faults = 0, q4_judged = 0, q4_deferred = 0;
  gpu::Q4Work q4_total{};
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
      // 6. S4b: the q4 lanes edge by edge against the engine's q4 lane, then
      // one mixed q3 + q4 batch through the boundary and the judge, a reduced
      // event buffer, and q4 output mutants.
      if (kmax >= 3) {
        Q4Buffers q4b(gpu::default_event_capacity);
        std::vector<std::size_t> where4;
        std::vector<gpu::u32> fa, fb;
        std::vector<gpu::u8> lanes;
        std::vector<std::uint8_t> asked6(survivors.size(), 0);
        for (std::size_t j = 0; j < survivors.size(); ++j) {
          if (certified.certificates.deferred[j] != 0) continue;
          const auto mask = static_cast<gpu::u8>(certified.certificates.masks[j] & 6U);
          if (mask == 0) continue;
          where4.push_back(j);
          fa.push_back(survivors[j].a_rank);
          fb.push_back(survivors[j].b_rank);
          lanes.push_back(mask);
          asked6[j] = mask;
          if ((mask & 4U) == 0) continue;
          gpu::u32 count = 0;
          gpu::EdgeQ3Work l3{};
          gpu::Q4Work w4{};
          const auto status = gpu::edge_lanes(gpu::HostGroup{}, view, survivors[j].a_rank, survivors[j].b_rank, 4,
                                              kmax, slab.view, q4b.view, count, l3, w4);
          if (status != gpu::CertificateStatus::decided) return fail("q4.undecided " + where_name);
          std::vector<gen::Q34LaneRecord> mine;
          for (gpu::u32 r = 0; r < count; ++r) mine.push_back(record_of(slab.records[r], 0));
          const auto reference = gen::engine_q4_records(index, kmax, options, order[survivors[j].a_rank],
                                                        order[survivors[j].b_rank]);
          if (!same_records(mine, reference)) return fail("q4.records " + where_name);
          gpu::add_q4(q4_total, w4);
        }
        auto in6 = flat.input(kmax, fa, fb);
        in6.edge_lanes = lanes.data();
        const auto out6 = gpu::run_lanes_batch_host(in6, workers);
        if (!out6.error.empty() || out6.deferred != 0 || out6.faults != 0)
          return fail("q4.batch " + where_name + " deferred=" + std::to_string(out6.deferred) + " faults=" +
                      std::to_string(out6.faults) + " records=" + std::to_string(out6.records.size()) + " edges=" +
                      std::to_string(where4.size()) + " " + out6.error);
        const auto batch6 = batch_of(out6, where4, survivors.size(), &lanes);
        gen::Q34LanesJudgeWork judge6;
        const auto judged6 = gen::judge_lanes_filter(
            [&](const gen::Q2CensusIndexPtr&, unsigned, std::span<const gen::Q34SurvivingEdge>,
                std::span<const std::uint8_t>) { return batch6; },
            options, workers, &judge6);
        try {
          static_cast<void>(judged6(index, kmax, survivors, asked6));
        } catch (const std::exception& e) {
          return fail(std::string("q4.judge_refused_real ") + where_name + " " + e.what());
        }
        q4_judged += judge6.judged;
        // Reduced event buffer: deferrals, the other edges unchanged.
        auto small6 = in6;
        small6.event_capacity = 2;
        const auto outs = gpu::run_lanes_batch_host(small6, workers);
        if (!outs.error.empty() || outs.faults != 0) return fail("q4.small " + where_name);
        q4_deferred += outs.deferred;
        // Output mutants on a q4 record: the judge or the boundary refuses.
        std::size_t target = batch6.records.size();
        for (std::size_t r = 0; r < batch6.records.size(); ++r)
          if (batch6.records[r].arity == 4) {
            target = r;
            break;
          }
        if (target < batch6.records.size()) {
          const std::vector<std::pair<const char*, std::function<void(gen::Q34LanesBatch&)>>> q4_cases{
              {"q4_depth", [&](gen::Q34LanesBatch& b) { b.records[target].depth ^= 1U; }},
              {"q4_key", [&](gen::Q34LanesBatch& b) { b.records[target].key[4] += 1; }},
              {"q4_support", [&](gen::Q34LanesBatch& b) { b.records[target].support[1] ^= 1U; }},
              {"q4_fingerprint", [&](gen::Q34LanesBatch& b) { b.records[target].shell_sum += 1; }},
              {"q4_arity", [&](gen::Q34LanesBatch& b) { b.records[target].arity = 3; }},
              {"q4_groups", [&](gen::Q34LanesBatch& b) { ++b.work4.groups; }},
              {"q4_seeds", [&](gen::Q34LanesBatch& b) { ++b.work4.certified; }},
          };
          for (const auto& [name, mutate] : q4_cases) {
            auto mutated = batch6;
            mutate(mutated);
            const auto mutant = gen::judge_lanes_filter(
                [&](const gen::Q2CensusIndexPtr&, unsigned, std::span<const gen::Q34SurvivingEdge>,
                    std::span<const std::uint8_t>) { return mutated; },
                options, workers, nullptr);
            bool refused = false;
            try {
              static_cast<void>(mutant(index, kmax, survivors, asked6));
            } catch (const std::logic_error&) {
              refused = true;
            }
            if (!refused) return fail(std::string("q4.mutant_survived ") + name + " " + where_name);
            ++mutants;
          }
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
        // An allocation failure in every worker's slab (records of 512 GiB):
        // the call throws after joining its threads, with several blocks.
        if (ea.size() > 128) {
          for (const std::size_t threads : {std::size_t{1}, workers}) {
            auto huge = in;
            huge.record_capacity = 0xffffffffU;
            bool thrown = false;
            try {
              static_cast<void>(gpu::run_lanes_batch_host(huge, threads));
            } catch (const std::bad_alloc&) {
              thrown = true;
            } catch (const std::length_error&) {
              thrown = true;
            }
            if (!thrown) return fail("fault.allocation_not_reported " + where_name);
            ++faults;
          }
          auto empty = in;
          empty.edge_count = 0;
          const auto none = gpu::run_lanes_batch_host(empty, workers);
          if (!none.error.empty() || !none.status.empty() || !none.records.empty() || none.work.edges != 0)
            return fail("empty.not_empty " + where_name);
        }
      }
    }
  }
  // Auditor B's multi-group fixture: one seed, two root groups, two
  // tetrahedra at K3 (edge of IDs 0 and 1).
  unsigned long long b_fixture = 0;
  {
    const std::vector<gen::Point3> points{{0, 0, 2}, {4, 0, 2}, {2, 3, 2}, {2, 2, 4}, {2, 2, 0}};
    const auto index = gen::make_q2_cloud_index(gen::prepare_cloud(points));
    const Flat flat(*index);
    const auto order = index->spatial_order();
    gpu::u32 ra = 0, rb = 0;
    for (std::size_t r = 0; r < order.size(); ++r) {
      if (order[r] == 0) ra = static_cast<gpu::u32>(r);
      if (order[r] == 1) rb = static_cast<gpu::u32>(r);
    }
    Slab slab(gpu::default_lanes_capacity, gpu::default_record_capacity);
    Q4Buffers q4b(gpu::default_event_capacity);
    gpu::u32 count = 0;
    gpu::EdgeQ3Work l3{};
    gpu::Q4Work w4{};
    if (gpu::edge_lanes(gpu::HostGroup{}, flat.view(), ra, rb, 4, 3, slab.view, q4b.view, count, l3, w4) !=
        gpu::CertificateStatus::decided)
      return fail("b_fixture.status");
    std::vector<gen::Q34LaneRecord> mine;
    for (gpu::u32 r = 0; r < count; ++r) mine.push_back(record_of(slab.records[r], 0));
    if (count != 2 || w4.multi_emission_seeds != 1 || w4.groups < 2 ||
        !same_records(mine, gen::engine_q4_records(index, 3, options, 0, 1)))
      return fail("b_fixture.two_tetrahedra");
    gpu::add_q4(q4_total, w4);
    b_fixture = count;
  }
  // A root exactly on an INTERIOR bound of the grid (L6): a = (0,2,2),
  // b = (3,2,2), x = (1,0,2), y = (1,2,0) give mubar = 12, the grid
  // -12,-9,...,12 and the root of y at mu = 6 = g_6, the high end of bucket
  // 5 and the low end of bucket 6. Found by an exact search; the candidate
  // is foreign in bucket 5 and decided in bucket 6: one tetrahedron at K3,
  // not two (examined twice) and not zero (examined nowhere).
  unsigned long long bound_fixture = 0;
  {
    const std::vector<gen::Point3> points{{0, 2, 2}, {3, 2, 2}, {1, 0, 2}, {1, 2, 0}};
    const auto index = gen::make_q2_cloud_index(gen::prepare_cloud(points));
    const Flat flat(*index);
    const auto order = index->spatial_order();
    gpu::u32 ra = 0, rb = 0;
    for (std::size_t r = 0; r < order.size(); ++r) {
      if (order[r] == 0) ra = static_cast<gpu::u32>(r);
      if (order[r] == 1) rb = static_cast<gpu::u32>(r);
    }
    Slab slab(gpu::default_lanes_capacity, gpu::default_record_capacity);
    Q4Buffers q4b(gpu::default_event_capacity);
    gpu::u32 count = 0;
    gpu::EdgeQ3Work l3{};
    gpu::Q4Work w4{};
    if (gpu::edge_lanes(gpu::HostGroup{}, flat.view(), ra, rb, 4, 3, slab.view, q4b.view, count, l3, w4) !=
        gpu::CertificateStatus::decided)
      return fail("bound_fixture.status");
    std::vector<gen::Q34LaneRecord> mine;
    for (gpu::u32 r = 0; r < count; ++r) mine.push_back(record_of(slab.records[r], 0));
    if (count != 1 || w4.foreign_candidates == 0 ||
        !same_records(mine, gen::engine_q4_records(index, 3, options, 0, 1)))
      return fail("bound_fixture.one_tetrahedron");
    gpu::add_q4(q4_total, w4);
    bound_fixture = count;
  }
  // A surviving seed without any buffered event (review S4b): on the edge of
  // IDs 0 and 3 at K3, one seed survives with live buckets and no list
  // step, so live_buckets > list_steps (the reader's first bound was false);
  // list_steps >= filter_steps and 32 list_steps >= bucket_events hold.
  unsigned long long zero_buffer_fixture = 0;
  {
    const std::vector<gen::Point3> points{{30, 24, 8}, {14, 6, 6}, {27, 35, 29}, {4, 17, 26}, {13, 38, 29}};
    const auto index = gen::make_q2_cloud_index(gen::prepare_cloud(points));
    const Flat flat(*index);
    const auto order = index->spatial_order();
    gpu::u32 ra = 0, rb = 0;
    for (std::size_t r = 0; r < order.size(); ++r) {
      if (order[r] == 0) ra = static_cast<gpu::u32>(r);
      if (order[r] == 3) rb = static_cast<gpu::u32>(r);
    }
    Slab slab(gpu::default_lanes_capacity, gpu::default_record_capacity);
    Q4Buffers q4b(gpu::default_event_capacity);
    gpu::u32 count = 0;
    gpu::EdgeQ3Work l3{};
    gpu::Q4Work w4{};
    if (gpu::edge_lanes(gpu::HostGroup{}, flat.view(), ra, rb, 4, 3, slab.view, q4b.view, count, l3, w4) !=
        gpu::CertificateStatus::decided)
      return fail("zero_buffer.status");
    std::vector<gen::Q34LaneRecord> mine;
    for (gpu::u32 r = 0; r < count; ++r) mine.push_back(record_of(slab.records[r], 0));
    if (count == 0 || w4.live_buckets <= w4.list_steps || w4.list_steps < w4.filter_steps ||
        32 * w4.list_steps < w4.bucket_events || !same_records(mine, gen::engine_q4_records(index, 3, options, 0, 3)))
      return fail("zero_buffer.survivor");
    gpu::add_q4(q4_total, w4);
    zero_buffer_fixture = count;
  }
  // The auditor's compact arena fixture: 45 clusters, each a = (-1000,0,0),
  // b = (1000,0,0) and the 108 integer points (0,u,v) with u^2+v^2 = 1105^2.
  // Every triangle abx is acute with ab longest and every other point of the
  // circle is outside its ball: 108 q3 balls per edge at K5, 4 860 records
  // for a default arena of 16E + 4096 = 4 816: exactly the last edge (host
  // order) is deferred, the others equal to the engine.
  unsigned long long arena_deferred = 0;
  {
    constexpr int clusters = 45, radius = 1105, half = 1000;
    std::vector<std::array<int, 2>> circle;
    for (int u = -radius; u <= radius; ++u)
      for (int v = -radius; v <= radius; ++v)
        if (u * u + v * v == radius * radius) circle.push_back({u, v});
    if (circle.size() != 108) return fail("arena.circle");
    std::vector<gen::Point3> points;
    for (int c = 0; c < clusters; ++c) {
      const int cx = 1000 + 5000 * c;
      points.push_back({cx - half, radius, radius});
      points.push_back({cx + half, radius, radius});
      for (const auto& [u, v] : circle) points.push_back({cx, radius + u, radius + v});
    }
    const auto index = gen::make_q2_cloud_index(gen::prepare_cloud(points));
    const Flat flat(*index);
    const auto order = index->spatial_order();
    std::vector<gpu::u32> rank_of(order.size());
    for (std::size_t r = 0; r < order.size(); ++r) rank_of[order[r]] = static_cast<gpu::u32>(r);
    std::vector<gpu::u32> ea, eb;
    const std::size_t stride = 2 + circle.size();
    for (int c = 0; c < clusters; ++c) {
      ea.push_back(rank_of[stride * c]);
      eb.push_back(rank_of[stride * c + 1]);
    }
    const auto out = gpu::run_lanes_batch_host(flat.input(5, ea, eb), workers);
    if (!out.error.empty() || out.faults != 0 || out.deferred != 1 ||
        out.status.back() != static_cast<gpu::u8>(gpu::CertificateStatus::deferred) ||
        out.records.size() != 108 * static_cast<std::size_t>(clusters - 1))
      return fail("arena.deferral");
    for (int c = 0; c < clusters; ++c) {
      if (out.status[c] == static_cast<gpu::u8>(gpu::CertificateStatus::deferred)) {
        ++arena_deferred;
        continue;
      }
      std::vector<gen::Q34LaneRecord> slice;
      for (gpu::u32 r = 0; r < out.record_count[c]; ++r)
        slice.push_back(record_of(out.records[out.record_begin[c] + r], 0));
      if (slice.size() != 108) return fail("arena.balls");
      if (c % 8 == 0 &&
          !same_records(slice, gen::engine_q3_records(index, 5, options, order[ea[c]], order[eb[c]])))
        return fail("arena.records");
    }
  }
  std::printf("lanes_port_gate n=%zu edges=%llu q3_only=%llu depth_rejections=%llu emitted=%llu wide_shells=%llu "
              "deferred=%llu judged=%llu mutants=%llu guards=%llu faults=%llu arena_deferred=%llu q4_edges=%llu "
              "q4_seeds=%llu q4_survivors=%llu q4_groups=%llu q4_emitted=%llu q4_multi=%llu q4_without_valid=%llu "
              "q4_max_group=%llu q4_judged=%llu q4_deferred=%llu b_fixture=%llu bound_fixture=%llu "
              "zero_buffer_fixture=%llu\n",
              n, edges, q3_only, rejections, emitted, wide_shells, deferred, judged, mutants, guards, faults,
              arena_deferred, q4_total.edges, q4_total.seeds, q4_total.survivors, q4_total.groups, q4_total.emitted,
              q4_total.multi_emission_seeds, q4_total.groups_without_valid, q4_total.max_group, q4_judged,
              q4_deferred, b_fixture, bound_fixture, zero_buffer_fixture);
  if (edges == 0 || q3_only == 0 || rejections == 0 || emitted == 0 || wide_shells == 0 || deferred == 0 ||
      judged == 0 || guards == 0 || mutants == 0 || faults == 0 || arena_deferred != 1 || q4_total.edges == 0 ||
      q4_total.certified_chunk1 == 0 || q4_total.survivors == 0 || q4_total.emitted == 0 ||
      q4_total.multi_emission_seeds == 0 || q4_total.groups_without_valid == 0 ||
      q4_total.depth_rejected_groups == 0 || q4_total.max_group < 3 || q4_judged == 0 || q4_deferred == 0 ||
      b_fixture != 2 || bound_fixture != 1 || zero_buffer_fixture == 0)
    return 3;
  return 0;
}
