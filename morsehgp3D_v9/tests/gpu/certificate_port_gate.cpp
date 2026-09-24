// MorseHGP3D v9 — porte du port portable du certificat de voie morte (S3,
// 23 septembre 2026).
//
// Juge : gpu/certificate.hpp compile pour l'hote (groupe de 32 voies emule)
// et le prouveur produit (Q34EdgeCover::make_diametral puis make,
// Q34DeadLaneProver load/prove, dans l'ordre d'Engine::filtered_edge) prennent
// la meme decision sur chaque arete survivante du filtre temoin, avec les
// memes compteurs, champ par champ (cover du coeur et du cover, prouveur du
// coeur et du cover, constructions, sites, fermetures, maximum).
//   - aretes : les survivants reels du chemin par lots (filtre CPU de
//     reference), trois familles, K2 (q3 seul), K3, K5, K10 ; avec le coeur
//     diametral, et sans lui (une arete sur quatre, cover seul) ;
//   - capacite reduite : une arete est mise en attente (deferred, sans aucun
//     compteur) exactement quand son coeur ou son cover depasse la capacite ;
//     les autres gardent la meme decision ;
//   - planchers : cellules exterieures, profondes et echouees, voies q3/q4
//     prouvees et ouvertes, plages fusionnees, fermetures par le coeur et par
//     le cover, mises en attente, tous non nuls ;
//   - mutants du comparateur : un masque, un test uniforme, une plage
//     fusionnee ou un maximum de cover faux d'une unite sont detectes ;
//   - garde d'entree du GPU (validate_certificate_input, seule garantie de
//     terminaison et de bornes du noyau) : l'entree reelle est acceptee,
//     chaque champ forge un a un est refuse (liens d'echappement cycliques ou
//     incoherents, rangs, masques, capacite) ;
//   - parcours par blocs (24 septembre 2026, build_cover_chunked, celui de
//     l'appareil et du jumeau) : sur chaque arete, pour la boule du coeur ET
//     celle du cover, a pleine capacite et a capacite 64, meme reponse, memes
//     plages (octet pour octet), memes sites et meme travail de parcours que
//     build_cover (un noeud par pas, le temoin) ; certify_edge des deux
//     parcours rend le meme statut, le meme masque et le meme travail.
//     Planchers : blocs, blocs a plusieurs noeuds visites, et chaque sortie
//     d'un bloc (fils gauche et echappement dans le bloc, fils gauche au-dela
//     de la derniere voie, echappement au-dela du bloc, fin de l'index),
//     parcours mis en attente ; mutants du comparateur (travail, plage).
//     Trois mutants compiles du produit (MHGP9_CERTIFICATE_MUTANT_*) sont
//     tues par cette porte (code 1, `cause=`).
//
//   mhgp9_gpu_certificate_port_gate [--n=2000] [--k=2,3,5,10]
//   mhgp9_gpu_certificate_port_gate --file=nuage.u32le --k=5 [--workers=4]
//     [--min-edges=N] [--device]
//     (toutes les survivantes d'un nuage fichier : les deux parcours, les
//     deux chargements et le prouveur produit, arete par arete ; publie le
//     travail en passes de groupe, avant (visites, une passe par plage) et
//     apres (blocs, fenetres de 32 sites) ; code 3 sous le plancher d'aretes
//     ou d'une classe de sortie. Avec --device (session G4), l'appel de
//     l'appareil (gpu::run_certificate_batch) compare au jumeau, octet pour
//     octet : statuts, masques et travail somme ; sans appareil, refus
//     explicite (code 2, ligne `certificate_device_compare absent`).)
//
// Code 0 conforme, 1 desaccord ou mutant survivant (`cause=`), 2 argument,
// 3 plancher.
#include <algorithm>
#include <atomic>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <fstream>
#include <iterator>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "../../src/gen/lanes/edge_cover.hpp"
#include "../../src/gen/lanes/q34_dead_lanes.hpp"
#include "../../src/gen/pipeline/wspd_q34.hpp"
#include "../../src/gpu/certificate.hpp"
#include "../../src/gpu/filter_runner.hpp"
#include "../../src/gpu/flat_index.hpp"
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

gpu::CoverWork cover_of(const gen::Q34EdgeCoverWork& w) {
  return gpu::CoverWork{w.node_visits,    w.bound_tests,    w.point_tests,     w.admitted_nodes, w.rejected_nodes,
                        w.split_nodes,    w.admitted_sites, w.rejected_sites, w.retained_ranges, w.merged_ranges};
}

gpu::DeadWork dead_of(const gen::Q34DeadLaneWork& w) {
  return gpu::DeadWork{w.loads,        w.form_sites,  w.cells,     w.outside_cells, w.deep_cells, w.failed_cells,
                       w.uniform_tests, w.point_tests, w.q3_proved, w.q3_open,       w.q4_proved,  w.q4_open};
}

bool same_cover(const gpu::CoverWork& a, const gpu::CoverWork& b) {
  return a.node_visits == b.node_visits && a.bound_tests == b.bound_tests && a.point_tests == b.point_tests &&
         a.admitted_nodes == b.admitted_nodes && a.rejected_nodes == b.rejected_nodes &&
         a.split_nodes == b.split_nodes && a.admitted_sites == b.admitted_sites &&
         a.rejected_sites == b.rejected_sites && a.retained_ranges == b.retained_ranges &&
         a.merged_ranges == b.merged_ranges;
}

bool same_dead(const gpu::DeadWork& a, const gpu::DeadWork& b) {
  return a.loads == b.loads && a.form_sites == b.form_sites && a.cells == b.cells &&
         a.outside_cells == b.outside_cells && a.deep_cells == b.deep_cells && a.failed_cells == b.failed_cells &&
         a.uniform_tests == b.uniform_tests && a.point_tests == b.point_tests && a.q3_proved == b.q3_proved &&
         a.q3_open == b.q3_open && a.q4_proved == b.q4_proved && a.q4_open == b.q4_open;
}

bool same_edge_cover(const gpu::EdgeCoverWork& a, const gpu::EdgeCoverWork& b) {
  return a.node_visits == b.node_visits && a.bound_tests == b.bound_tests && a.point_tests == b.point_tests &&
         a.admitted_nodes == b.admitted_nodes && a.rejected_nodes == b.rejected_nodes &&
         a.split_nodes == b.split_nodes && a.admitted_sites == b.admitted_sites &&
         a.rejected_sites == b.rejected_sites && a.retained_ranges == b.retained_ranges &&
         a.merged_ranges == b.merged_ranges;
}

bool same_work(const gpu::CertificateWork& a, const gpu::CertificateWork& b) {
  return a.core_builds == b.core_builds && a.core_sites == b.core_sites &&
         a.core_closed_edges == b.core_closed_edges && same_cover(a.core_cover, b.core_cover) &&
         same_dead(a.dead_core, b.dead_core) && a.cover_builds == b.cover_builds && a.cover_sites == b.cover_sites &&
         a.max_cover_sites == b.max_cover_sites && same_cover(a.cover, b.cover) && same_dead(a.dead, b.dead);
}

// The product certificate of one edge, in the order of Engine::filtered_edge.
struct Reference {
  std::uint8_t mask{};
  gpu::CertificateWork work{};
  std::size_t core_sites{}, cover_sites{};  // cover_sites 0 when the core closed the edge
};

Reference product(const gen::Q2CensusIndexPtr& index, gen::Q34DeadLaneProver& prover, std::size_t a,
                  std::size_t b, std::uint8_t mask, unsigned kmax, bool with_core = true) {
  Reference r;
  auto& w = r.work;
  if (!with_core) {
    const auto cover = gen::Q34EdgeCover::make(index, {a, b});
    gen::Q34DeadLaneWork dead{};
    prover.load(*cover, dead);
    r.mask = static_cast<std::uint8_t>(mask & ~prover.prove(kmax, mask, dead));
    w.cover_builds = 1;
    w.cover_sites = cover->site_count();
    w.max_cover_sites = cover->site_count();
    w.cover = cover_of(cover->work());
    w.dead = dead_of(dead);
    r.cover_sites = cover->site_count();
    return r;
  }
  const auto core = gen::Q34EdgeCover::make_diametral(index, {a, b});
  gen::Q34DeadLaneWork dead_core{};
  prover.load(*core, dead_core);
  mask = static_cast<std::uint8_t>(mask & ~prover.prove(kmax, mask, dead_core));
  w.core_builds = 1;
  w.core_sites = core->site_count();
  w.core_cover = cover_of(core->work());
  w.dead_core = dead_of(dead_core);
  r.core_sites = core->site_count();
  if (mask == 0) {
    w.core_closed_edges = 1;
    return r;
  }
  const auto cover = gen::Q34EdgeCover::make(index, {a, b});
  gen::Q34DeadLaneWork dead{};
  prover.load(*cover, dead);
  mask = static_cast<std::uint8_t>(mask & ~prover.prove(kmax, mask, dead));
  w.cover_builds = 1;
  w.cover_sites = cover->site_count();
  w.max_cover_sites = cover->site_count();
  w.cover = cover_of(cover->work());
  w.dead = dead_of(dead);
  r.cover_sites = cover->site_count();
  r.mask = mask;
  return r;
}

struct Slab {
  std::vector<gpu::u32> ranges, frontiers;
  std::vector<gpu::i64> fc, fx, fy;
  gpu::CertificateSlab view{};
  explicit Slab(gpu::u32 capacity)
      : ranges(2 * std::size_t{capacity}), frontiers(gpu::prover_levels * std::size_t{capacity}), fc(capacity),
        fx(capacity), fy(capacity) {
    view = gpu::CertificateSlab{ranges.data(), fc.data(), fx.data(), fy.data(), frontiers.data(), capacity};
  }
};

// Chunked walk statistics (floors, not a judge): chunks, visits, and how
// each chunk ends or goes on.
struct WalkStats {
  unsigned long long walks = 0, deferred = 0, chunks = 0, visits = 0, multi = 0, forms = 0, load_passes = 0;
  unsigned long long replay_steps = 0;  // replay iterations: a run of split nodes, or one other node
  unsigned long long in_splits = 0, in_escapes = 0, exit_splits = 0, exit_escapes = 0, exit_ends = 0;
  void add(const WalkStats& o) {
    walks += o.walks;
    deferred += o.deferred;
    chunks += o.chunks;
    visits += o.visits;
    multi += o.multi;
    forms += o.forms;
    load_passes += o.load_passes;
    replay_steps += o.replay_steps;
    in_splits += o.in_splits;
    in_escapes += o.in_escapes;
    exit_splits += o.exit_splits;
    exit_escapes += o.exit_escapes;
    exit_ends += o.exit_ends;
  }
  [[nodiscard]] bool floors() const {
    return walks > 0 && deferred > 0 && chunks > 0 && visits > chunks && multi > 0 && forms > 0 && in_splits > 0 &&
           in_escapes > 0 && exit_splits > 0 && exit_escapes > 0 && exit_ends > 0;
  }
};

// The host group of the twin, counting the chunks of build_cover_chunked and
// classifying their ends by an independent chase of the ballot planes, and
// counting the group passes of the form loads (for_each: 32 sites a pass).
struct ChunkCountingGroup : gpu::HostGroup {
  WalkStats* stats;
  template <class F>
  void for_each(gpu::u32 count, F f) const {
    stats->load_passes += (count + 31) / 32;
    gpu::HostGroup::for_each(count, f);
  }
  template <unsigned N, class Code>
  void ballot_bits(gpu::u32 base, gpu::u32 count, Code code, gpu::u32 (&planes)[N]) const {
    gpu::HostGroup::ballot_bits(base, count, code, planes);
    ++stats->chunks;
    gpu::u32 lane = 0, visited = 1;
    bool run = false;  // inside a run of split nodes
    for (;; ++visited) {
      if (((planes[0] >> lane) & 1U) != 0) {
        if (!run) ++stats->replay_steps;
        run = true;
        if (lane == 31) {
          ++stats->exit_splits;
          break;
        }
        ++stats->in_splits;
        ++lane;
        continue;
      }
      run = false;
      ++stats->replay_steps;
      gpu::u32 offset = 0;
      for (unsigned k = 0; k < 5; ++k) offset |= ((planes[2 + k] >> lane) & 1U) << k;
      if (offset == gpu::chunk_exit) {
        ++stats->exit_escapes;
        break;
      }
      if (base + offset + 1 >= count) {
        ++stats->exit_ends;
        break;
      }
      ++stats->in_escapes;
      lane = offset + 1;
    }
    if (visited > 1) ++stats->multi;
  }
};

bool same_prover(const gpu::Prover& a, const gpu::Prover& b) {
  for (int i = 0; i < 3; ++i)
    if (a.a_basis[i] != b.a_basis[i] || a.b_basis[i] != b.b_basis[i]) return false;
  return a.diameter_squared == b.diameter_squared && a.disk_bound == b.disk_bound && a.n == b.n;
}

// The chunked walk against build_cover (one node per step) on one ball of
// edge ab: same answer, same work (also when the slab overflows), and when
// decided the same range count, sites and ranges, byte for byte; then the
// chunked form load against load_forms (one pass per range) on those
// ranges: same prover, same work, and the same three form arrays, byte for
// byte. Empty when equal.
std::string compare_walks(const gpu::CertificateIndex& view, const std::int32_t* pa, const std::int32_t* pb,
                          bool diametral, const Slab& chunked, const Slab& sequential, WalkStats& stats,
                          unsigned long long* mutants) {
  const auto ball = gpu::edge_ball(pa, pb, diametral);
  gpu::u32 rc1 = 0, s1 = 0, rc2 = 0, s2 = 0;
  gpu::EdgeCoverWork w1{}, w2{};
  const bool ok1 = gpu::build_cover_chunked(ChunkCountingGroup{{}, &stats}, view, ball, chunked.view, rc1, s1, w1);
  const bool ok2 = gpu::build_cover(gpu::HostGroup{}, view, ball, sequential.view, rc2, s2, w2);
  ++stats.walks;
  if (ok1 != ok2) return "walk.deferral";
  if (!same_edge_cover(w1, w2)) return "walk.work";
  if (!ok1) {
    ++stats.deferred;
    return {};
  }
  stats.visits += w1.node_visits;
  if (rc1 != rc2 || s1 != s2) return "walk.shape";
  if (!std::equal(chunked.ranges.begin(), chunked.ranges.begin() + 2 * std::size_t{rc1}, sequential.ranges.begin()))
    return "walk.ranges";
  if (s1 < 2) return "walk.endpoints";
  gpu::EdgeDeadWork d1{}, d2{};
  const auto p1 = gpu::load_forms_chunked(gpu::HostGroup{}, view, pa, pb, chunked.view, rc1, s1, d1);
  const auto p2 = gpu::load_forms(gpu::HostGroup{}, view, pa, pb, sequential.view, rc2, s2, d2);
  if (!same_prover(p1, p2) || d1.loads != d2.loads || d1.form_sites != d2.form_sites) return "load.prover";
  const auto same_forms = [&](const std::vector<gpu::i64>& x, const std::vector<gpu::i64>& y) {
    return std::equal(x.begin(), x.begin() + s1, y.begin());
  };
  if (!same_forms(chunked.fc, sequential.fc) || !same_forms(chunked.fx, sequential.fx) ||
      !same_forms(chunked.fy, sequential.fy))
    return "load.forms";
  stats.forms += s1;
  if (mutants != nullptr) {
    // Comparator mutant: one form off by one.
    auto f = chunked.fy;
    ++f[s1 - 1];
    if (same_forms(f, sequential.fy)) return "mutant.load_form_survived";
    ++*mutants;
    // Comparator mutants: one counter and one range bound off by one.
    auto w = w1;
    ++w.merged_ranges;
    if (same_edge_cover(w, w2)) return "mutant.walk_work_survived";
    ++*mutants;
    if (rc1 != 0) {
      auto r = chunked.ranges;
      ++r[2 * std::size_t{rc1} - 1];
      if (std::equal(r.begin(), r.begin() + 2 * std::size_t{rc1}, sequential.ranges.begin()))
        return "mutant.walk_range_survived";
      ++*mutants;
    }
  }
  return {};
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

}  // namespace

template <class T>
bool parse_number(std::string_view digits, T& value) {
  const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), value);
  return !digits.empty() && error == std::errc{} && end == digits.data() + digits.size();
}

// Every survivor of a file cloud (real batch path, CPU filter): the chunked
// walk against build_cover on both balls, certify_edge with both walks, and
// the product prover, edge by edge; the call's own walks are counted (visits
// = warp steps of the sequential walk, chunks = decision passes of the
// chunked walk). Deterministic totals whatever the worker count.
int file_mode(const std::string& path, unsigned kmax, std::size_t workers, unsigned long long min_edges,
              bool device) {
  std::ifstream in(path, std::ios::binary);
  std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
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
  std::vector<gen::Q34SurvivingEdge> survivors;
  const gen::Q34BatchFilter capture = [&](const gen::Q2CensusIndex& ix, unsigned k,
                                          std::span<const gen::WspdRectangle> rectangles) {
    auto batch = gen::run_q34_filter_batch_cpu(ix, k, rectangles, workers);
    survivors = batch.survivors;
    return batch;
  };
  static_cast<void>(gen::run_wspd_q34_batched(index, kmax, 8, q34_options(), workers,
                                              [](std::size_t, const gen::Q34SeedCandidate&) {}, 16, capture,
                                              nullptr));
  const auto nodes = gpu::flatten_nodes(*index);
  const auto escapes = gpu::flatten_escapes(*index);
  std::vector<std::int32_t> rank_points;
  for (const auto& p : index->spatial_points()) {
    rank_points.push_back(p.x);
    rank_points.push_back(p.y);
    rank_points.push_back(p.z);
  }
  const gpu::CertificateIndex view{nodes.data(), escapes.data(), static_cast<gpu::u32>(nodes.size()),
                                   rank_points.data()};
  const auto order = index->spatial_order();
  const gpu::u32 capacity = gpu::default_certificate_capacity;
  struct Part {
    WalkStats call, walks, witness_call;
    gpu::CertificateWork work{};
    unsigned long long decided = 0, deferred = 0, core_closed = 0;
    std::size_t failed_at = ~std::size_t{0};
    std::string cause;
    std::exception_ptr error;
  };
  std::vector<Part> parts(workers);
  std::vector<gpu::u8> twin_status(survivors.size(), 0xff), twin_mask(survivors.size(), 0);
  std::atomic<std::size_t> next{0};
  constexpr std::size_t grain = 4096;
  const auto run = [&](Part& part) {
    gen::Q34DeadLaneProver prover;
    Slab full(capacity), witness(capacity);
    for (;;) {
      const std::size_t block = next.fetch_add(1);
      if (block * grain >= survivors.size()) return;
      for (std::size_t j = block * grain; j < std::min(survivors.size(), (block + 1) * grain); ++j) {
        const auto& edge = survivors[j];
        const auto fault = [&](const std::string& cause) {
          if (j < part.failed_at) {
            part.failed_at = j;
            part.cause = cause + " K" + std::to_string(kmax) + " survivor " + std::to_string(j);
          }
        };
        const std::int32_t* pa = rank_points.data() + 3 * std::size_t{edge.a_rank};
        const std::int32_t* pb = rank_points.data() + 3 * std::size_t{edge.b_rank};
        std::string cause;
        for (const bool diametral : {true, false})
          if (cause.empty()) cause = compare_walks(view, pa, pb, diametral, full, witness, part.walks, nullptr);
        if (!cause.empty()) {
          fault(cause);
          return;
        }
        gpu::CertificateWork got{}, seq{};
        const auto result = gpu::certify_edge(ChunkCountingGroup{{}, &part.call}, view, edge.a_rank, edge.b_rank,
                                              edge.mask, kmax, true, full.view, got);
        const auto one = gpu::certify_edge<gpu::CertificateWalk::sequential>(
            ChunkCountingGroup{{}, &part.witness_call}, view, edge.a_rank, edge.b_rank, edge.mask, kmax, true,
            witness.view, seq);
        if (one.status != result.status || one.mask != result.mask || !same_work(seq, got)) {
          fault("walk.certificate");
          return;
        }
        twin_status[j] = static_cast<gpu::u8>(result.status);
        twin_mask[j] = result.status == gpu::CertificateStatus::decided ? result.mask : edge.mask;
        const auto expected = product(index, prover, order[edge.a_rank], order[edge.b_rank], edge.mask, kmax);
        if (result.status == gpu::CertificateStatus::deferred) {
          if (expected.core_sites <= capacity && expected.cover_sites <= capacity) {
            fault("port.deferral");
            return;
          }
          ++part.deferred;
          continue;
        }
        if (result.status != gpu::CertificateStatus::decided) {
          fault("port.status");
          return;
        }
        if (result.mask != expected.mask) {
          fault("port.mask");
          return;
        }
        if (!same_work(got, expected.work)) {
          fault("port.work");
          return;
        }
        ++part.decided;
        part.core_closed += got.core_closed_edges;
        gpu::add_certificate(part.work, got);
      }
    }
  };
  {
    std::vector<std::thread> threads;
    for (std::size_t t = 0; t < workers; ++t)
      threads.emplace_back([&, t] {
        try {
          run(parts[t]);
        } catch (...) {
          parts[t].error = std::current_exception();
        }
      });
    for (auto& t : threads) t.join();
    for (const auto& part : parts)
      if (part.error) std::rethrow_exception(part.error);
  }
  Part total;
  for (const auto& part : parts) {
    total.call.add(part.call);
    total.walks.add(part.walks);
    total.witness_call.add(part.witness_call);
    gpu::add_certificate(total.work, part.work);
    total.decided += part.decided;
    total.deferred += part.deferred;
    total.core_closed += part.core_closed;
    if (part.failed_at < total.failed_at) {
      total.failed_at = part.failed_at;
      total.cause = part.cause;
    }
  }
  if (!total.cause.empty()) return fail(total.cause);
  if (device) {
    // The device call on every survivor against the twin, byte for byte:
    // per-edge statuses and masks (the input mask when deferred) and the
    // work summed over the decided edges.
    std::vector<gpu::u32> ea, eb;
    std::vector<gpu::u8> em;
    for (const auto& e : survivors) {
      ea.push_back(e.a_rank);
      eb.push_back(e.b_rank);
      em.push_back(e.mask);
    }
    gpu::CertificateInput in;
    in.index.nodes = nodes.data();
    in.index.node_count = nodes.size();
    in.index.rank_points = rank_points.data();
    in.index.rank_count = rank_points.size() / 3;
    in.index.kmax = kmax;
    in.escapes = escapes.data();
    in.edge_a = ea.data();
    in.edge_b = eb.data();
    in.edge_mask = em.data();
    in.edge_count = ea.size();
    in.dead_core = true;
    const auto out = gpu::run_certificate_batch(in);
    if (out.error_kind == gpu::BatchError::no_device) {
      std::printf("certificate_device_compare absent error=%s\n", out.error.c_str());
      return 2;
    }
    if (!out.error.empty()) return fail("device.error " + out.error);
    if (out.status.size() != survivors.size() || out.masks.size() != survivors.size())
      return fail("device.shape");
    for (std::size_t j = 0; j < survivors.size(); ++j)
      if (out.status[j] != twin_status[j] || out.masks[j] != twin_mask[j])
        return fail("device.edge survivor " + std::to_string(j));
    if (!same_work(out.work, total.work)) return fail("device.work");
    std::printf("certificate_device_compare K=%u device=%s edges=%zu deferred=%llu identical=1 warps=%u "
                "kernel_ms=%.3f upload_ms=%.3f download_ms=%.3f total_ms=%.3f\n",
                kmax, out.device.c_str(), survivors.size(), static_cast<unsigned long long>(out.deferred),
                out.warps, out.kernel_ms, out.upload_ms, out.download_ms, out.total_ms);
  }
  const unsigned long long visits = total.work.core_cover.node_visits + total.work.cover.node_visits;
  std::printf("certificate_port_file K=%u survivors=%zu decided=%llu deferred=%llu core_closed=%llu "
              "call_visits=%llu call_chunks=%llu multi_chunks=%llu in_splits=%llu in_escapes=%llu "
              "exit_splits=%llu exit_escapes=%llu exit_ends=%llu replay_steps=%llu load_passes_before=%llu "
              "load_passes_after=%llu "
              "compared_walks=%llu compared_visits=%llu compared_forms=%llu "
              "core_sites=%llu cover_sites=%llu cells=%llu uniform_tests=%llu\n",
              kmax, survivors.size(), total.decided, total.deferred, total.core_closed, visits, total.call.chunks,
              total.call.multi, total.call.in_splits, total.call.in_escapes, total.call.exit_splits,
              total.call.exit_escapes, total.call.exit_ends, total.call.replay_steps, total.witness_call.load_passes,
              total.call.load_passes,
              total.walks.walks, total.walks.visits, total.walks.forms,
              total.work.core_sites, total.work.cover_sites, total.work.dead_core.cells + total.work.dead.cells,
              total.work.dead_core.uniform_tests + total.work.dead.uniform_tests);
  const auto& c = total.call;
  const bool floors = total.decided >= min_edges && total.decided > 0 && c.chunks > 0 && visits > c.chunks &&
                      c.multi > 0 && c.in_splits > 0 && c.in_escapes > 0 && c.exit_splits > 0 &&
                      c.exit_escapes > 0 && c.exit_ends > 0 && total.walks.walks == 2 * survivors.size() &&
                      c.load_passes > 0 && c.load_passes <= total.witness_call.load_passes;
  if (!floors) {
    std::printf("cause=floor\n");
    return 3;
  }
  return 0;
}

int main(int argc, char** argv) {
  std::size_t n = 2000, workers = 4;
  unsigned long long min_edges = 1;
  std::string file;
  bool k_given = false, file_options = false, device = false;
  std::vector<unsigned> ks{2, 3, 5, 10};
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (arg.starts_with("--n=")) {
      if (!parse_number(arg.substr(4), n)) return 2;
    } else if (arg.starts_with("--k=")) {
      ks = parse_list(arg.substr(4));
      k_given = true;
      if (ks.empty()) return 2;
    } else if (arg.starts_with("--file=")) {
      file = std::string(arg.substr(7));
      if (file.empty()) return 2;
    } else if (arg.starts_with("--workers=")) {
      if (!parse_number(arg.substr(10), workers) || workers == 0 || workers > 256) return 2;
      file_options = true;
    } else if (arg.starts_with("--min-edges=")) {
      if (!parse_number(arg.substr(12), min_edges)) return 2;
      file_options = true;
    } else if (arg == "--device") {
      device = true;
      file_options = true;
    } else {
      return 2;
    }
  }
  if (n < 64 || n > 65536 || std::any_of(ks.begin(), ks.end(), [](unsigned k) { return k < 2 || k > 10; })) {
    std::fprintf(stderr, "usage: mhgp9_gpu_certificate_port_gate [--n=2000] [--k=2,3,5,10]\n"
                         "       mhgp9_gpu_certificate_port_gate --file=nuage.u32le --k=K [--workers=4] "
                         "[--min-edges=N]\n");
    return 2;
  }
  if (!file.empty()) {
    if (!k_given || ks.size() != 1) return 2;
    return file_mode(file, ks[0], workers, min_edges, device);
  }
  if (file_options) return 2;  // --workers, --min-edges and --device only with --file
  unsigned long long edges = 0, deferred = 0, core_closed = 0, cover_closed = 0, open = 0, mutants = 0,
                     coverless = 0, guards = 0;
  gpu::CertificateWork total{};
  WalkStats walk;
  for (const std::string_view family : {"uniform", "terrain", "clusters"}) {
    const auto fixture = gen::bench::make_front_fixture(n, family, 3);
    const auto cloud = gen::prepare_cloud(fixture.points);
    const auto index = gen::make_q2_cloud_index(cloud);
    const auto nodes = gpu::flatten_nodes(*index);
    const auto escapes = gpu::flatten_escapes(*index);
    std::vector<std::int32_t> rank_points;
    for (const auto& p : index->spatial_points()) {
      rank_points.push_back(p.x);
      rank_points.push_back(p.y);
      rank_points.push_back(p.z);
    }
    const gpu::CertificateIndex view{nodes.data(), escapes.data(), static_cast<gpu::u32>(nodes.size()),
                                     rank_points.data()};
    const auto order = index->spatial_order();
    bool guarded = false;
    for (const unsigned kmax : ks) {
      const std::string where = std::string(family) + "/K" + std::to_string(kmax);
      // The real survivors of the witness filter (CPU reference batch).
      std::vector<gen::Q34SurvivingEdge> survivors;
      const gen::Q34BatchFilter capture = [&](const gen::Q2CensusIndex& ix, unsigned k,
                                              std::span<const gen::WspdRectangle> rectangles) {
        auto batch = gen::run_q34_filter_batch_cpu(ix, k, rectangles, 4);
        survivors = batch.survivors;
        return batch;
      };
      static_cast<void>(gen::run_wspd_q34_batched(index, kmax, 8, q34_options(), 4,
                                                  [](std::size_t, const gen::Q34SeedCandidate&) {}, 16, capture,
                                                  nullptr));
      // Input guard of the device call: the real input is accepted, each
      // forged field is refused (never a device call here).
      if (!guarded && survivors.size() >= 2) {
        guarded = true;
        std::vector<gpu::u32> ea, eb;
        std::vector<gpu::u8> em;
        for (std::size_t i = 0; i < 2; ++i) {
          ea.push_back(survivors[i].a_rank);
          eb.push_back(survivors[i].b_rank);
          em.push_back(survivors[i].mask);
        }
        auto forged_nodes = nodes;
        auto forged_escapes = escapes;
        const auto input = [&](const std::vector<gpu::FlatNode>& ns, const std::vector<gpu::u32>& es) {
          gpu::CertificateInput in;
          in.index.nodes = ns.data();
          in.index.node_count = ns.size();
          in.index.rank_points = rank_points.data();
          in.index.rank_count = rank_points.size() / 3;
          in.index.kmax = kmax;
          in.escapes = es.data();
          in.edge_a = ea.data();
          in.edge_b = eb.data();
          in.edge_mask = em.data();
          in.edge_count = ea.size();
          in.dead_core = true;
          return in;
        };
        if (!gpu::validate_certificate_input(input(nodes, escapes)).empty()) return fail("guard.real_refused " + where);
        std::size_t internal = 0, leaf = 0;
        while (internal < nodes.size() && nodes[internal].left == gpu::absent32) ++internal;
        while (leaf < nodes.size() && nodes[leaf].left != gpu::absent32) ++leaf;
        if (internal >= nodes.size() || leaf >= nodes.size()) return fail("guard.tree_shape " + where);
        const auto refused = [&](const gpu::CertificateInput& in) {
          return !gpu::validate_certificate_input(in).empty();
        };
        std::vector<std::pair<const char*, bool>> cases;
        {
          auto in = input(nodes, escapes);
          in.escapes = nullptr;
          cases.emplace_back("null escapes", refused(in));
        }
        const auto with_escape = [&](std::size_t at, gpu::u32 value) {
          forged_escapes = escapes;
          forged_escapes[at] = value;
          return refused(input(nodes, forged_escapes));
        };
        cases.emplace_back("root escape", with_escape(0, static_cast<gpu::u32>(nodes.size() - 1)));
        cases.emplace_back("cyclic escape", with_escape(internal, static_cast<gpu::u32>(internal)));
        cases.emplace_back("backward escape", with_escape(leaf, 0));
        cases.emplace_back("leaf escape skips", with_escape(leaf, std::min<gpu::u32>(
                                                                    static_cast<gpu::u32>(leaf + 2),
                                                                    static_cast<gpu::u32>(nodes.size()))));
        cases.emplace_back("left child escape", with_escape(nodes[internal].left, nodes[internal].right + 1));
        cases.emplace_back("right child escape", with_escape(nodes[internal].right, escapes[internal] - 1));
        {
          forged_nodes = nodes;
          std::swap(forged_nodes[internal].left, forged_nodes[internal].right);
          cases.emplace_back("left child not next", refused(input(forged_nodes, escapes)));
        }
        const auto with_edge = [&](int field, gpu::u32 value) {
          auto a2 = ea, b2 = eb;
          auto m2 = em;
          if (field == 0) a2[1] = value;
          else if (field == 1) b2[1] = value;
          else m2[1] = static_cast<gpu::u8>(value);
          auto in = input(nodes, escapes);
          in.edge_a = a2.data();
          in.edge_b = b2.data();
          in.edge_mask = m2.data();
          return refused(in);
        };
        cases.emplace_back("rank outside", with_edge(0, static_cast<gpu::u32>(rank_points.size() / 3)));
        cases.emplace_back("same endpoints", with_edge(1, ea[1]));
        cases.emplace_back("mask zero", with_edge(2, 0));
        cases.emplace_back("mask bit 0", with_edge(2, 3));
        {
          auto in = input(nodes, escapes);
          in.capacity = 1;
          cases.emplace_back("capacity one", refused(in));
        }
        {
          // A multi-rank leaf (auditor B) in an otherwise valid preorder tree
          // of three points: only the singleton-leaf rule can refuse it; the
          // same tree with that leaf split in two is accepted.
          const std::vector<std::int32_t> tiny{0, 0, 0, 10, 0, 0, 20, 0, 0};
          const auto box = [](std::int32_t lo, std::int32_t hi) { return gpu::FlatBox{{lo, 0, 0}, {hi, 0, 0}}; };
          const std::vector<gpu::FlatNode> merged{{box(0, 20), 1, 2, 0, 3}, {box(0, 0), gpu::absent32, gpu::absent32, 0, 1},
                                                  {box(10, 20), gpu::absent32, gpu::absent32, 1, 3}};
          const std::vector<gpu::u32> merged_escapes{3, 2, 3};
          const std::vector<gpu::FlatNode> split{{box(0, 20), 1, 2, 0, 3}, {box(0, 0), gpu::absent32, gpu::absent32, 0, 1},
                                                 {box(10, 20), 3, 4, 1, 3}, {box(10, 10), gpu::absent32, gpu::absent32, 1, 2},
                                                 {box(20, 20), gpu::absent32, gpu::absent32, 2, 3}};
          const std::vector<gpu::u32> split_escapes{5, 2, 5, 4, 5};
          const std::vector<gpu::u32> ta{0}, tb{1};
          const std::vector<gpu::u8> tm{2};
          const auto tiny_input = [&](const std::vector<gpu::FlatNode>& ns, const std::vector<gpu::u32>& es) {
            gpu::CertificateInput in;
            in.index.nodes = ns.data();
            in.index.node_count = ns.size();
            in.index.rank_points = tiny.data();
            in.index.rank_count = 3;
            in.index.kmax = 3;
            in.escapes = es.data();
            in.edge_a = ta.data();
            in.edge_b = tb.data();
            in.edge_mask = tm.data();
            in.edge_count = 1;
            return in;
          };
          if (!gpu::validate_certificate_input(tiny_input(split, split_escapes)).empty())
            return fail("guard.tiny_split_refused " + where);
          cases.emplace_back("multi-rank leaf", gpu::validate_certificate_input(tiny_input(merged, merged_escapes)) ==
                                                    "leaf with more than one rank");
        }
        {
          // Lanes outside K (auditor B): q4 at K2, anything at K1.
          auto in = input(nodes, escapes);
          auto m2 = em;
          m2[1] = 4;
          in.edge_mask = m2.data();
          in.index.kmax = 2;
          cases.emplace_back("q4 lane at K2", refused(in));
          m2[1] = 2;
          in.index.kmax = 1;
          cases.emplace_back("q3 lane at K1", refused(in));
        }
        for (const auto& [label, ok] : cases) {
          if (!ok) return fail(std::string("guard.accepted ") + label + " " + where);
          ++guards;
        }
      }
      gen::Q34DeadLaneProver prover;
      Slab full(static_cast<gpu::u32>(n)), small(64), witness(static_cast<gpu::u32>(n)), witness_small(64);
      for (const auto& edge : survivors) {
        const auto a = order[edge.a_rank], b = order[edge.b_rank];
        const std::string at = where + " edge " + std::to_string(a) + "-" + std::to_string(b);
        // The chunked walk against build_cover, on both balls of the edge,
        // at full and at reduced capacity.
        {
          const std::int32_t* pa = rank_points.data() + 3 * std::size_t{edge.a_rank};
          const std::int32_t* pb = rank_points.data() + 3 * std::size_t{edge.b_rank};
          for (const bool diametral : {true, false}) {
            auto cause = compare_walks(view, pa, pb, diametral, full, witness, walk,
                                       edges % 997 == 1 && diametral ? &mutants : nullptr);
            if (cause.empty()) cause = compare_walks(view, pa, pb, diametral, small, witness_small, walk, nullptr);
            if (!cause.empty()) return fail(cause + " " + at);
          }
        }
        const auto expected = product(index, prover, a, b, edge.mask, kmax);
        gpu::CertificateWork got{};
        const auto result = gpu::certify_edge(gpu::HostGroup{}, view, edge.a_rank, edge.b_rank, edge.mask, kmax,
                                              true, full.view, got);
        {
          gpu::CertificateWork seq{};
          const auto one = gpu::certify_edge<gpu::CertificateWalk::sequential>(
              gpu::HostGroup{}, view, edge.a_rank, edge.b_rank, edge.mask, kmax, true, witness.view, seq);
          if (one.status != result.status || one.mask != result.mask || !same_work(seq, got))
            return fail("walk.certificate " + at);
        }
        if (result.status != gpu::CertificateStatus::decided) return fail("port.status " + at);
        if (result.mask != expected.mask) return fail("port.mask " + at);
        if (!same_work(got, expected.work)) return fail("port.work " + at);
        ++edges;
        gpu::add_certificate(total, got);
        if (expected.work.core_closed_edges != 0) ++core_closed;
        else if (expected.mask == 0) ++cover_closed;
        else ++open;
        // Reduced capacity: deferred exactly when a built ball exceeds it.
        gpu::CertificateWork reduced{};
        const auto limited = gpu::certify_edge(gpu::HostGroup{}, view, edge.a_rank, edge.b_rank, edge.mask, kmax,
                                               true, small.view, reduced);
        const bool overflow = expected.core_sites > 64 || expected.cover_sites > 64;
        if (overflow) {
          gpu::CertificateWork zero{};
          if (limited.status != gpu::CertificateStatus::deferred || !same_work(reduced, zero))
            return fail("port.deferral " + at);
          ++deferred;
        } else if (limited.status != gpu::CertificateStatus::decided || limited.mask != expected.mask ||
                   !same_work(reduced, expected.work)) {
          return fail("port.reduced " + at);
        }
        // Without the diametral core (cover only), one edge in four.
        if (edges % 4 == 0) {
          const auto bare = product(index, prover, a, b, edge.mask, kmax, false);
          gpu::CertificateWork plain{};
          const auto alone = gpu::certify_edge(gpu::HostGroup{}, view, edge.a_rank, edge.b_rank, edge.mask, kmax,
                                               false, full.view, plain);
          if (alone.status != gpu::CertificateStatus::decided || alone.mask != bare.mask ||
              !same_work(plain, bare.work))
            return fail("port.coverless " + at);
          ++coverless;
        }
        // Comparator mutants on the first edges of each case.
        if (edges % 997 == 1) {
          auto m = expected;
          m.mask = static_cast<std::uint8_t>(m.mask ^ 2U);
          if (result.mask == m.mask) return fail("mutant.mask_survived " + at);
          m = expected;
          ++m.work.dead_core.uniform_tests;
          if (same_work(got, m.work)) return fail("mutant.uniform_tests_survived " + at);
          m = expected;
          ++m.work.core_cover.merged_ranges;
          if (same_work(got, m.work)) return fail("mutant.merged_ranges_survived " + at);
          m = expected;
          ++m.work.max_cover_sites;
          if (same_work(got, m.work)) return fail("mutant.max_cover_sites_survived " + at);
          mutants += 4;
        }
      }
    }
  }
  std::printf("gpu_certificate_port_gate n=%zu edges=%llu core_closed=%llu cover_closed=%llu open=%llu "
              "deferred=%llu cells=%llu outside=%llu deep=%llu failed=%llu uniform_tests=%llu point_tests=%llu "
              "merged_ranges=%llu q3_proved=%llu q4_proved=%llu mutants=%llu coverless=%llu guards=%llu "
              "walks=%llu walk_deferred=%llu visits=%llu chunks=%llu multi_chunks=%llu forms=%llu in_splits=%llu "
              "in_escapes=%llu exit_splits=%llu exit_escapes=%llu exit_ends=%llu\n",
              n, edges, core_closed, cover_closed, open, deferred, total.dead_core.cells + total.dead.cells,
              total.dead_core.outside_cells + total.dead.outside_cells,
              total.dead_core.deep_cells + total.dead.deep_cells,
              total.dead_core.failed_cells + total.dead.failed_cells,
              total.dead_core.uniform_tests + total.dead.uniform_tests,
              total.dead_core.point_tests + total.dead.point_tests,
              total.core_cover.merged_ranges + total.cover.merged_ranges,
              total.dead_core.q3_proved + total.dead.q3_proved, total.dead_core.q4_proved + total.dead.q4_proved,
              mutants, coverless, guards, walk.walks, walk.deferred, walk.visits, walk.chunks, walk.multi, walk.forms,
              walk.in_splits, walk.in_escapes, walk.exit_splits, walk.exit_escapes, walk.exit_ends);
  const bool floors = edges > 0 && core_closed > 0 && cover_closed > 0 && open > 0 && deferred > 0 &&
                      total.dead_core.outside_cells > 0 && total.dead.outside_cells > 0 &&
                      total.dead_core.deep_cells > 0 && total.dead.deep_cells > 0 &&
                      total.dead_core.failed_cells > 0 && total.dead.failed_cells > 0 &&
                      total.dead_core.point_tests > 0 && total.dead.point_tests > 0 &&
                      total.core_cover.merged_ranges > 0 && total.cover.merged_ranges > 0 &&
                      total.dead.q3_proved > 0 && total.dead.q4_proved > 0 && total.dead.q3_open > 0 &&
                      total.dead.q4_open > 0 && mutants >= 12 && coverless >= 1000 && guards >= 3 * 16 &&
                      walk.floors();
  if (!floors) {
    std::printf("cause=floor\n");
    return 3;
  }
  return 0;
}
