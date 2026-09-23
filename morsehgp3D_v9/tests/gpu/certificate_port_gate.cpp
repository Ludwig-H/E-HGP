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
//     reference), trois familles, K3/K5/K10 ;
//   - capacite reduite : une arete est mise en attente (deferred, sans aucun
//     compteur) exactement quand son coeur ou son cover depasse la capacite ;
//     les autres gardent la meme decision ;
//   - planchers : cellules exterieures, profondes et echouees, voies q3/q4
//     prouvees et ouvertes, plages fusionnees, fermetures par le coeur et par
//     le cover, mises en attente, tous non nuls ;
//   - mutants du comparateur : un masque, un test uniforme, une plage
//     fusionnee ou un maximum de cover faux d'une unite sont detectes.
//
//   mhgp9_gpu_certificate_port_gate [--n=2000] [--k=3,5,10]
//
// Code 0 conforme, 1 desaccord ou mutant survivant (`cause=`), 2 argument,
// 3 plancher.
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "../../src/gen/lanes/edge_cover.hpp"
#include "../../src/gen/lanes/q34_dead_lanes.hpp"
#include "../../src/gen/pipeline/wspd_q34.hpp"
#include "../../src/gpu/certificate.hpp"
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
                  std::size_t b, std::uint8_t mask, unsigned kmax) {
  Reference r;
  auto& w = r.work;
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

int main(int argc, char** argv) {
  std::size_t n = 2000;
  std::vector<unsigned> ks{3, 5, 10};
  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (arg.starts_with("--n=")) {
      const auto digits = arg.substr(4);
      const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), n);
      if (digits.empty() || error != std::errc{} || end != digits.data() + digits.size()) return 2;
    } else if (arg.starts_with("--k=")) {
      ks = parse_list(arg.substr(4));
      if (ks.empty()) return 2;
    } else {
      return 2;
    }
  }
  if (n < 64 || n > 65536 || std::any_of(ks.begin(), ks.end(), [](unsigned k) { return k < 2 || k > 10; })) {
    std::fprintf(stderr, "usage: mhgp9_gpu_certificate_port_gate [--n=2000] [--k=3,5,10]\n");
    return 2;
  }
  unsigned long long edges = 0, deferred = 0, core_closed = 0, cover_closed = 0, open = 0, mutants = 0;
  gpu::CertificateWork total{};
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
      gen::Q34DeadLaneProver prover;
      Slab full(static_cast<gpu::u32>(n)), small(64);
      for (const auto& edge : survivors) {
        const auto a = order[edge.a_rank], b = order[edge.b_rank];
        const auto expected = product(index, prover, a, b, edge.mask, kmax);
        gpu::CertificateWork got{};
        const auto result = gpu::certify_edge(gpu::HostGroup{}, view, edge.a_rank, edge.b_rank, edge.mask, kmax,
                                              true, full.view, got);
        const std::string at = where + " edge " + std::to_string(a) + "-" + std::to_string(b);
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
              "merged_ranges=%llu q3_proved=%llu q4_proved=%llu mutants=%llu\n",
              n, edges, core_closed, cover_closed, open, deferred, total.dead_core.cells + total.dead.cells,
              total.dead_core.outside_cells + total.dead.outside_cells,
              total.dead_core.deep_cells + total.dead.deep_cells,
              total.dead_core.failed_cells + total.dead.failed_cells,
              total.dead_core.uniform_tests + total.dead.uniform_tests,
              total.dead_core.point_tests + total.dead.point_tests,
              total.core_cover.merged_ranges + total.cover.merged_ranges,
              total.dead_core.q3_proved + total.dead.q3_proved, total.dead_core.q4_proved + total.dead.q4_proved,
              mutants);
  const bool floors = edges > 0 && core_closed > 0 && cover_closed > 0 && open > 0 && deferred > 0 &&
                      total.dead_core.outside_cells > 0 && total.dead.outside_cells > 0 &&
                      total.dead_core.deep_cells > 0 && total.dead.deep_cells > 0 &&
                      total.dead_core.failed_cells > 0 && total.dead.failed_cells > 0 &&
                      total.dead_core.point_tests > 0 && total.dead.point_tests > 0 &&
                      total.core_cover.merged_ranges > 0 && total.cover.merged_ranges > 0 &&
                      total.dead.q3_proved > 0 && total.dead.q4_proved > 0 && total.dead.q3_open > 0 &&
                      total.dead.q4_open > 0 && mutants >= 12;
  if (!floors) {
    std::printf("cause=floor\n");
    return 3;
  }
  return 0;
}
