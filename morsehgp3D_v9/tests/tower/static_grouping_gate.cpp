// MorseHGP3D v9 — porte differentielle du regroupement de la phase 0 de la tour.
//
// La phase 0 statique regroupe ses requetes (cle de facette, consommateur,
// ordinal) par classes de cle egale, prend la PREMIERE requete de chaque
// classe (ordinal minimal : son consommateur donne `before`) et ecrit une
// cible par requete. Deux voies : le temoin trie (tri des requetes de 56 o
// par cle et ordinal, puis debuts de groupes) et les classes de hachage
// exactes (table a adressage ouvert, cles comparees en entier ; les graines
// y sont indexees de meme, sans tri). Pour chaque
// ordre K >= 2, les deux voies doivent rendre OCTET POUR OCTET :
//   - le vecteur des cibles statiques (une par ordinal de requete) ;
//   - le vecteur des premieres requetes (ordinal minimal de la classe de
//     chaque requete), qui fixe la partition et le choix de la premiere ;
// puis la meme tour (condense), le meme statut et les memes compteurs (tous
// les champs de travail de FullBallStats, lanes comprises a W egal ; seules
// les capacites echantillonnees static_peak_* different par construction).
// W parcourt {1, 2, 3, 4, 8}, phase A recouvrante ou non.
//
//   mhgp9_tower_static_grouping_gate --selftest
//   mhgp9_tower_static_grouping_gate --file=<.u32le> --k=K --digest=<hex16>
//       [--catalogue-digest=<hex16>] [--threads=1,2,3,4,8] [--witness-threads=8]
//       [--min-requests=N]
//
// --selftest : nuages deterministes (grappes u18, grille entiere cospherique),
// catalogue produit par la chaine (4 fils), tours temoin et hachee a chaque W.
// Compile avec MHGP9_TOWER_GROUP_TEST_WEAK_HASH (cible de test), le hachage
// n'a que quatre valeurs : des cles distinctes partagent etiquette et case
// de depart, et la comparaison exacte doit les separer (planchers : refus
// d'etiquette > 0 dans les classes ET dans l'index des graines) ; seuls les
// petits nuages y sont joues (cout quadratique).
// --file : une trame (08/000000 au CMake), chaine a 8 fils, condenses epingles
// de la tour (et du catalogue), puis le temoin a --witness-threads et la voie
// hachee a chaque --threads, comparees au temoin.
//
// Planchers (code 3) : voie effectivement prise a chaque ordre (trace path =
// 2 pour la voie hachee, 1 pour le temoin), classes a plusieurs requetes,
// graines trouvees,
// plus d'un ouvrier a W >= 2, au moins 8 192 requetes a un ordre (--selftest)
// ou --min-requests (--file). Code 0 conforme, 1 desaccord (ligne `cause=`),
// 2 argument ou entree, 3 plancher.
#include <array>
#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include "../../src/chain/tower_chain.hpp"
#include "../../src/tower/forest/full_ball_tower.hpp"

namespace {
using mhgp9::tower::FullBallStats;
using mhgp9::tower::FullBallStaticTrace;
using mhgp9::tower::FullBallStatus;
using mhgp9::tower::FullBallTowerResult;

#if defined(MHGP9_TOWER_GROUP_TEST_WEAK_HASH)
constexpr bool kWeakHash = true;
#else
constexpr bool kWeakHash = false;
#endif

struct Mismatch { std::string cause; };  // code 1
struct Floor { std::string cause; };     // code 3

// First differing work field, or nullptr. `lanes`: compare the thread-shaped
// counters too (same W on both sides).
const char* differing_work(const FullBallStats& a, const FullBallStats& b, bool lanes) {
#define MHGP9_SAME(field) \
  if (!(a.field == b.field)) return #field;
  MHGP9_SAME(records) MHGP9_SAME(extra_records) MHGP9_SAME(anchor_blocks) MHGP9_SAME(regular_blocks)
  MHGP9_SAME(extra_blocks) MHGP9_SAME(representatives) MHGP9_SAME(anchor_hits) MHGP9_SAME(key_lookups)
  MHGP9_SAME(intruder_queries) MHGP9_SAME(intruder_nodes) MHGP9_SAME(intruder_power_tests)
  MHGP9_SAME(interior_ranges) MHGP9_SAME(same_radius_steps) MHGP9_SAME(descending_steps)
  MHGP9_SAME(max_chain_steps) MHGP9_SAME(births) MHGP9_SAME(merges) MHGP9_SAME(contributions)
  MHGP9_SAME(inert_blocks) MHGP9_SAME(declared_support_checks) MHGP9_SAME(singleton_lots)
  MHGP9_SAME(grouped_lots) MHGP9_SAME(lot_dsu_slots) MHGP9_SAME(lower_edges_indexed)
  MHGP9_SAME(lower_nodes_activated) MHGP9_SAME(lower_edges_activated) MHGP9_SAME(lower_queries)
  MHGP9_SAME(lower_find_steps) MHGP9_SAME(lower_path_writes) MHGP9_SAME(resolver_cache_queries)
  MHGP9_SAME(resolver_cache_hits) MHGP9_SAME(resolver_cache_stores) MHGP9_SAME(resolver_cache_evictions)
  MHGP9_SAME(resolver_cache_seed_stores) MHGP9_SAME(resolver_cache_slots) MHGP9_SAME(resolver_cache_reset_slots)
  MHGP9_SAME(resolver_cache_bytes) MHGP9_SAME(resolver_cache_released_slots)
  MHGP9_SAME(static_requests) MHGP9_SAME(static_unique) MHGP9_SAME(static_seeded)
  MHGP9_SAME(static_post_seed_queries) MHGP9_SAME(static_post_seed_hits) MHGP9_SAME(static_post_seed_terminals)
  MHGP9_SAME(static_batch_calls) MHGP9_SAME(presorted_catalogues) MHGP9_SAME(static_batch_work_known)
  if (lanes) {
    MHGP9_SAME(static_workers_created) MHGP9_SAME(static_lanes_used) MHGP9_SAME(parallel_orders)
    MHGP9_SAME(overlapped_orders)
  }
  for (const auto work : {&FullBallStats::validation_work, &FullBallStats::resolve_work}) {
    const auto& x = a.*work;
    const auto& y = b.*work;
    if (x.calls != y.calls || x.supports_by_size != y.supports_by_size || x.power_tests != y.power_tests ||
        x.materializations != y.materializations || x.pair_distances != y.pair_distances ||
        x.proposals != y.proposals || x.verified_proposals != y.verified_proposals ||
        x.boundary_canonicalizations != y.boundary_canonicalizations ||
        x.proposal_fallbacks != y.proposal_fallbacks)
      return work == &FullBallStats::validation_work ? "validation_work" : "resolve_work";
  }
#undef MHGP9_SAME
  return nullptr;
}

struct Run {
  FullBallTowerResult tower;
  FullBallStaticTrace trace;
  std::uint64_t digest = 0;
  int threads = 0;
  bool hashed = false, overlap = true;
};

Run build(const mhgp9::tower::CloudIndex& ix, const std::vector<mhgp9::tower::BallData>& balls, unsigned kmax,
          int threads, bool hashed, bool overlap) {
  Run r;
  r.threads = threads;
  r.hashed = hashed;
  r.overlap = overlap;
  r.tower = mhgp9::tower::build_full_ball_tower(ix, balls, kmax, threads, {}, true,
      // Pool off (review before R21): this gate judges hash against sort on
      // the per-call helpers, whose worker counts are the planned widths,
      // deterministic and equal on both paths; pool x hash is judged by
      // mhgp9_chain_tower_tail (eight combinations) and task_pool_gate.
      mhgp9::tower::FullBallTowerOptions{.overlap_static = overlap, .hash_grouping = hashed,
                                         .persistent_pool = false, .trace = &r.trace});
  r.digest = mhgp9::tower_digest(r.tower);
  r.tower.orders.clear();  // the digest is kept, the forests are not needed
  r.tower.orders.shrink_to_fit();
  return r;
}

std::string where(const char* label, const Run& r, unsigned k) {
  char text[160];
  std::snprintf(text, sizeof text, " cloud=%s W=%d overlap=%d K=%u", label, r.threads, r.overlap ? 1 : 0, k);
  return text;
}

struct Tally {
  std::uint64_t compared_requests = 0, max_requests = 0, deduplicated = 0, seeded = 0;
  std::uint64_t tag_rejects = 0, seed_tag_rejects = 0, probes = 0, runs = 0, multi_worker = 0;
};

// The hashed run `h` against the complete witness `w`: the firsts of every
// order whose classes `h` reached (recorded before any resolution failure),
// then the targets of every order, then status, path, digest and work.
void compare(const char* label, const Run& w, const Run& h, unsigned kmax, std::uint64_t digest, Tally& t) {
  if (w.tower.status != FullBallStatus::kCompleteRelative)
    throw Mismatch{std::string("cause=witness.status reason=") + w.tower.reason + where(label, w, kmax)};
  for (unsigned k = 2; k <= kmax; ++k) {
    if (w.trace.path[k] != 1 || w.trace.firsts[k].size() != w.tower.stats.static_requests[k] ||
        w.trace.targets[k].size() != w.tower.stats.static_requests[k])
      throw Floor{"cause=floor.witness_path" + where(label, w, k)};
    if (h.trace.path[k] && h.trace.firsts[k] != w.trace.firsts[k])
      throw Mismatch{"cause=grouping.firsts" + where(label, h, k)};
  }
  for (unsigned k = 2; k <= kmax; ++k)
    if (h.trace.targets[k] != w.trace.targets[k]) throw Mismatch{"cause=grouping.targets" + where(label, h, k)};
  if (h.tower.status != FullBallStatus::kCompleteRelative)
    throw Mismatch{std::string("cause=grouping.status reason=") + h.tower.reason + where(label, h, kmax)};
  for (unsigned k = 2; k <= kmax; ++k)
    if (h.trace.path[k] != 2) throw Floor{"cause=floor.path" + where(label, h, k)};
  if (w.digest != digest || h.digest != digest) throw Mismatch{"cause=grouping.digest" + where(label, h, kmax)};
  if (const char* field = differing_work(w.tower.stats, h.tower.stats, w.threads == h.threads && w.overlap == h.overlap))
    throw Mismatch{std::string("cause=grouping.work field=") + field + where(label, h, kmax)};
  for (unsigned k = 2; k <= kmax; ++k) {
    const auto& s = h.tower.stats;
    t.compared_requests += s.static_requests[k];
    t.max_requests = std::max<std::uint64_t>(t.max_requests, s.static_requests[k]);
    t.deduplicated += s.static_requests[k] - s.static_unique[k];
    t.seeded += s.static_seeded[k];
    t.tag_rejects += h.trace.tag_rejects[k];
    t.seed_tag_rejects += h.trace.seed_tag_rejects[k];
    t.probes += h.trace.probes[k];
  }
  ++t.runs;
  if (h.threads >= 2 && h.tower.stats.static_workers_created >= 2) ++t.multi_worker;
}

std::vector<mhgp9::tower::InputPoint> index_input(const std::vector<mhgp9::gen::Point3>& points) {
  std::vector<mhgp9::tower::InputPoint> input(points.size());
  for (std::size_t i = 0; i < points.size(); ++i)
    input[i] = mhgp9::tower::InputPoint{static_cast<mhgp9::tower::PointId>(i),
                                        mhgp9::tower::P3{points[i].x, points[i].y, points[i].z}};
  return input;
}

std::vector<mhgp9::gen::Point3> clusters(int n, std::uint64_t seed) {
  // Three u18 clusters (64-bit LCG), as the native preflight of the G4 worker.
  std::vector<mhgp9::gen::Point3> points;
  std::uint64_t state = seed;
  for (int i = 0; i < n; ++i) {
    const std::int32_t centre = (i % 3) * 40000 + 20000;
    std::int32_t c[3];
    for (int axis = 0; axis < 3; ++axis) {
      state = state * 6364136223846793005ull + 1442695040888963407ull;
      c[axis] = centre + static_cast<std::int32_t>((state >> 40) % 9000);
    }
    points.push_back({c[0], c[1], c[2]});
  }
  return points;
}

std::vector<mhgp9::gen::Point3> grid(int side, std::int32_t step) {
  std::vector<mhgp9::gen::Point3> points;
  for (int x = 0; x < side; ++x)
    for (int y = 0; y < side; ++y)
      for (int z = 0; z < side; ++z) points.push_back({1000 + x * step, 1000 + y * step, 1000 + z * step});
  return points;
}

// Chain catalogue of one cloud, then witness and hashed towers at each W.
void cloud(const char* label, const std::vector<mhgp9::gen::Point3>& points, unsigned kmax, Tally& t) {
  mhgp9::ChainOptions options;
  options.kmax = kmax;
  options.workers = 4;
  options.tower_static_threads = 4;
  options.tower_hash_grouping = false;  // the chain's tower is the witness: the reference digest
  options.keep_catalogue = true;
  auto chain = mhgp9::run_tower_chain(points, options);
  if (chain.status != mhgp9::ChainStatus::kComplete)
    throw Mismatch{std::string("cause=chain.status cloud=") + label + " reason=" + chain.reason};
  const auto balls = std::move(chain.catalogue_balls);
  const std::uint64_t digest = chain.tower_digest;
  const auto ix = mhgp9::tower::build_cloud_index(index_input(points));
  const auto reference = build(ix, balls, kmax, 1, true, true);
  for (const int threads : {1, 2, 3, 4, 8})
    for (const bool overlap : {true, false}) {
      if (!overlap && threads != 4) continue;  // run_orders_parallel once
      const auto witness = build(ix, balls, kmax, threads, false, overlap);
      const auto hashed = build(ix, balls, kmax, threads, true, overlap);
      compare(label, witness, hashed, kmax, digest, t);
      for (unsigned k = 2; k <= kmax; ++k)
        if (hashed.trace.targets[k] != reference.trace.targets[k] || hashed.trace.firsts[k] != reference.trace.firsts[k])
          throw Mismatch{"cause=grouping.threads" + where(label, hashed, k)};
    }
  std::printf("cloud=%s n=%zu kmax=%u balls=%zu digest=%016" PRIx64 "\n", label, points.size(), kmax, balls.size(),
              digest);
}

int selftest() {
  Tally t;
  if (!kWeakHash) cloud("clusters_1500", clusters(1500, 3), 5, t);
  cloud("clusters_240", clusters(240, 11), 6, t);
  cloud("grid_27", grid(3, 40), 5, t);
  std::printf("static_grouping_gate mode=%s runs=%" PRIu64 " requests=%" PRIu64 " max_requests=%" PRIu64
              " deduplicated=%" PRIu64 " seeded=%" PRIu64 " tag_rejects=%" PRIu64 " seed_tag_rejects=%" PRIu64
              " probes=%" PRIu64 " multi_worker=%" PRIu64 "\n",
              kWeakHash ? "weak_hash" : "hash", t.runs, t.compared_requests, t.max_requests, t.deduplicated, t.seeded,
              t.tag_rejects, t.seed_tag_rejects, t.probes, t.multi_worker);
  if (t.deduplicated == 0 || t.seeded == 0 || t.multi_worker < 4 * (kWeakHash ? 2u : 3u))
    throw Floor{"cause=floor.classes_seeds_or_workers"};
  if (!kWeakHash && t.max_requests < 8192) throw Floor{"cause=floor.parallel_requests"};
  if (kWeakHash && (t.tag_rejects == 0 || t.seed_tag_rejects == 0)) throw Floor{"cause=floor.no_collision_split"};
  return 0;
}

bool parse_hex(std::string_view text, std::uint64_t& value) {
  if (text.size() != 16) return false;
  value = 0;
  for (const char ch : text) {
    const int digit = ch >= '0' && ch <= '9' ? ch - '0' : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10 : -1;
    if (digit < 0) return false;
    value = value * 16 + static_cast<std::uint64_t>(digit);
  }
  return true;
}

bool parse_list(std::string_view text, std::vector<int>& values) {
  values.clear();
  while (!text.empty()) {
    const auto comma = text.find(',');
    const auto item = text.substr(0, comma);
    if (item.empty() || item.size() > 3) return false;
    int v = 0;
    for (const char ch : item) {
      if (ch < '0' || ch > '9') return false;
      v = v * 10 + (ch - '0');
    }
    if (v < 1 || v > 256) return false;
    values.push_back(v);
    if (comma == std::string_view::npos) break;
    text.remove_prefix(comma + 1);
  }
  return !values.empty();
}

std::vector<mhgp9::gen::Point3> read_frame(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::invalid_argument("cannot open input");
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (!path.ends_with(".u32le") || bytes.empty() || bytes.size() % 12 != 0)
    throw std::invalid_argument("input must be a non-empty .u32le of site records");
  std::vector<mhgp9::gen::Point3> points(bytes.size() / 12);
  for (std::size_t i = 0; i < points.size(); ++i) {
    std::uint32_t c[3];
    for (int a = 0; a < 3; ++a) {
      c[a] = 0;
      for (int b = 0; b < 4; ++b) c[a] |= static_cast<std::uint32_t>(bytes[(3 * i + a) * 4 + b]) << (8 * b);
      if (c[a] > 262143u) throw std::invalid_argument("coordinate outside [0, 2^18)");
    }
    points[i] = {static_cast<mhgp9::gen::Coordinate>(c[0]), static_cast<mhgp9::gen::Coordinate>(c[1]),
                 static_cast<mhgp9::gen::Coordinate>(c[2])};
  }
  return points;
}

int frame(const std::string& path, unsigned kmax, std::uint64_t digest, bool check_catalogue,
          std::uint64_t catalogue, const std::vector<int>& threads, const std::vector<int>& witness_threads,
          std::uint64_t min_requests) {
  const auto points = read_frame(path);
  mhgp9::ChainOptions options;
  options.kmax = kmax;
  options.workers = 8;
  options.tower_hash_grouping = false;  // the chain's tower is the witness: the pinned digest
  options.keep_catalogue = true;
  options.catalogue_digest = check_catalogue;
  auto chain = mhgp9::run_tower_chain(points, options);
  if (chain.status != mhgp9::ChainStatus::kComplete) throw Mismatch{"cause=chain.status reason=" + chain.reason};
  if (chain.tower_digest != digest || (check_catalogue && chain.catalogue_digest != catalogue)) {
    std::printf("chain tower_digest=%016" PRIx64 " catalogue_digest=%016" PRIx64 "\n", chain.tower_digest,
                chain.catalogue_digest);
    throw Mismatch{"cause=frame.pinned_digest"};
  }
  const auto balls = std::move(chain.catalogue_balls);
  chain = mhgp9::ChainResult{};  // release the chain's own tower
  const auto ix = mhgp9::tower::build_cloud_index(index_input(points));
  Tally t;
  for (const int wt : witness_threads) {
    const auto witness = build(ix, balls, kmax, wt, false, true);
    for (const int ht : threads) {
      const auto hashed = build(ix, balls, kmax, ht, true, true);
      compare("frame", witness, hashed, kmax, digest, t);
      std::printf("frame K=%u witness_W=%d hashed_W=%d equal static_ms witness=%.1f hashed=%.1f\n", kmax, wt, ht,
                  witness.tower.times.static_ms, hashed.tower.times.static_ms);
      std::fflush(stdout);
    }
  }
  std::printf("static_grouping_frame K=%u sites=%zu balls=%zu digest=%016" PRIx64 " runs=%" PRIu64
              " requests=%" PRIu64 " max_requests=%" PRIu64 " deduplicated=%" PRIu64 " seeded=%" PRIu64
              " tag_rejects=%" PRIu64 " seed_tag_rejects=%" PRIu64 " probes=%" PRIu64 " multi_worker=%" PRIu64 "\n",
              kmax, points.size(), balls.size(), digest, t.runs, t.compared_requests, t.max_requests, t.deduplicated,
              t.seeded, t.tag_rejects, t.seed_tag_rejects, t.probes, t.multi_worker);
  if (t.max_requests < min_requests || t.deduplicated == 0 || t.seeded == 0) throw Floor{"cause=floor.frame_requests"};
  return 0;
}
}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 2 && std::string_view(argv[1]) == "--selftest") return selftest();
    std::string path;
    unsigned kmax = 0;
    std::uint64_t digest = 0, catalogue = 0, min_requests = 1;
    bool has_digest = false, has_catalogue = false;
    std::vector<int> threads{1, 2, 3, 4, 8}, witness_threads{8};
    for (int i = 1; i < argc; ++i) {
      const std::string_view arg(argv[i]);
      if (arg.starts_with("--file=")) path = std::string(arg.substr(7));
      else if (arg == "--k=5") kmax = 5;
      else if (arg == "--k=10") kmax = 10;
      else if (arg.starts_with("--digest=")) has_digest = parse_hex(arg.substr(9), digest);
      else if (arg.starts_with("--catalogue-digest=")) {
        has_catalogue = parse_hex(arg.substr(19), catalogue);
        if (!has_catalogue) path.clear();
      }
      else if (arg.starts_with("--threads=")) { if (!parse_list(arg.substr(10), threads)) path.clear(); }
      else if (arg.starts_with("--witness-threads=")) { if (!parse_list(arg.substr(18), witness_threads)) path.clear(); }
      else if (arg.starts_with("--min-requests=")) {
        min_requests = 0;
        for (const char ch : arg.substr(15)) {
          if (ch < '0' || ch > '9' || min_requests > 1000000000000ull) { path.clear(); break; }
          min_requests = min_requests * 10 + static_cast<std::uint64_t>(ch - '0');
        }
      }
      else { path.clear(); break; }
    }
    if (path.empty() || !kmax || !has_digest) {
      std::fprintf(stderr, "usage: mhgp9_tower_static_grouping_gate --selftest | --file=<.u32le> --k=5|10 "
                           "--digest=<hex16> [--catalogue-digest=<hex16>] [--threads=L] [--witness-threads=L] "
                           "[--min-requests=N]\n");
      return 2;
    }
    return frame(path, kmax, digest, has_catalogue, catalogue, threads, witness_threads, min_requests);
  } catch (const Mismatch& m) {
    std::printf("%s\n", m.cause.c_str());
    return 1;
  } catch (const Floor& f) {
    std::printf("%s\n", f.cause.c_str());
    return 3;
  } catch (const std::invalid_argument& e) {
    std::fprintf(stderr, "input refusal: %s\n", e.what());
    return 2;
  }
}
