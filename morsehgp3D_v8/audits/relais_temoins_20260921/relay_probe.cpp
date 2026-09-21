// Auditeur B, 21 septembre 2026 — relais rectangle → paires pour la recherche de témoins (question A/B du journal 34 :
// « une suggestion de relais qui limite les reprises de racine sans grossir l'état des millions de petits blocs »).
// Sources du front épinglées (front.cpp inchangé depuis c5308651) ; rien du moteur q3/q4 n'est appelé.
//  Pour chaque rectangle résiduel A×B émis par le front (masque 6) et chaque voie q ∈ {3,4} (α3 = 3, α4 = 2, seuil
//  h_q = K + 2 − q) :
//   1. Recherche SATURANTE de témoins universels de boîte (celle de la tranche 32, enfant le plus proche du milieu
//      d'abord) : si h_q crédits sont atteints, le rectangle est rejeté pour la voie et aucune paire n'est visitée.
//   2. Sinon, recherche EXHAUSTIVE (sans saturation) sur le même index : les nœuds entiers universels donnent un crédit
//      commun U (tous leurs sites sont dans le citron de TOUTES les paires du rectangle), les nœuds à 4H_max ≤ 0 sont
//      exclus (aucun site n'est dans le citron d'aucune paire), les feuilles restantes non universelles forment la
//      liste C des CANDIDATS du rectangle. C'est l'état de relais : transitoire, un rectangle à la fois, jamais
//      stocké par paire ni par bloc.
//   3. Chaque paire (a,b) du rectangle ne teste que C : compte du citron = U + #{c ∈ C, c ≠ a,b : citron(a,b,c)}.
//      Identité exacte (les sites hors U ∪ C sont hors du citron de toute paire), vérifiée sur M paires par voie tirées
//      dans la masse résiduelle par balayage complet du nuage (désaccords exigés nuls) ; pour ces mêmes paires, la
//      descente saturante « milieu d'abord » depuis la racine (mesure de front_lanes_lidar) est rejouée pour comparer
//      son nombre de visites au nombre de candidats testés par le relais.
//  Rien n'est modifié dans le moteur. Compter, jamais promouvoir.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <random>
#include <string>
#include <vector>
#include "wspd/front.hpp"
#include "pipeline/q2_census.hpp"
#include "pipeline/q2_joint_bounds.hpp"
#include "pipeline/prepared_cloud.hpp"
#include "spindle/predicates.hpp"
using namespace mhgp8;
namespace {
bool contains(Range range, std::size_t rank) { return rank >= range.first && rank < range.last; }
i64 midpoint_distance4(const std::array<i64, 3>& center4, const Box3& box) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto delta = std::max<i64>({0, 4 * static_cast<i64>(box.low[axis]) - center4[axis], center4[axis] - 4 * static_cast<i64>(box.high[axis])});
    result += delta * delta;
  }
  return result;
}
unsigned alpha(unsigned lane) { return lane == 1 ? 3U : 2U; }
bool box_witness(const Box3& a, const Box3& b, const Point3& z, unsigned lane) {
  const auto zb = singleton_box(z);
  const i64 h = spindle_detail::h_minimum(a, b, zb);
  if (h <= 0) return false;
  const i128 xi = spindle_detail::xi_bounds(a, b, zb).high;
  return static_cast<i128>(alpha(lane)) * spindle_detail::square(h) > xi;
}
bool lemon(const Point3& a, const Point3& b, const Point3& z, unsigned lane) {
  i64 h = 0;
  std::array<i64, 3> u{}, w{};
  for (std::size_t axis = 0; axis < 3; ++axis) {
    u[axis] = static_cast<i64>(z[axis]) - a[axis];
    w[axis] = static_cast<i64>(b[axis]) - z[axis];
    h += u[axis] * w[axis];
  }
  if (h <= 0) return false;
  i128 xi = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const std::size_t j = (axis + 1) % 3, k = (axis + 2) % 3;
    const i64 c = u[j] * w[k] - u[k] * w[j];
    xi += static_cast<i128>(c) * c;
  }
  return static_cast<i128>(alpha(lane)) * (static_cast<i128>(h) * h) > xi;
}
bool node_universal(const Q2JointPreparedBounds& prepared, const Box3& a, const Box3& b, const Q2SpatialNode& z, unsigned lane, i64 minimum4) {
  (void)prepared;
  const i128 xi16 = static_cast<i128>(16) * spindle_detail::xi_bounds(a, b, z.box).high;
  return static_cast<i128>(alpha(lane)) * (static_cast<i128>(minimum4) * minimum4) > xi16;
}
struct Rect { std::uint32_t a, b; std::uint8_t mask; };
struct LaneStats {
  u64 rectangles{}, mass{}, rect_rejected{}, rect_rejected_mass{}, sat_visits{};
  u64 exhaustive_rects{}, exhaustive_mass{}, exh_visits{}, exh_leaf_tests{}, exh_admitted_nodes{};
  u64 candidates_sum{}, candidates_max{}, universal_sum{};
  std::array<u64, 20> candidates_hist{};  // [floor(log2(1+|C|))]
  u64 pairs{}, pair_tests{}, pairs_rejected_relay{}, pairs_kept_relay{};
  u64 sample_pairs{}, sample_disagreements{}, sample_relay_tests{}, sample_relay_tests_saturating{}, sample_root_visits{}, sample_root_visits_exhaustive{}, sample_in_rejected_rect{};
};
}  // namespace
int main(int argc, char** argv) {
  if (argc < 4) { std::fprintf(stderr, "usage: file.u16le kmax s [samples_per_lane=2000] [seed=1]\n"); return 2; }
  const std::string file = argv[1];
  const unsigned kmax = static_cast<unsigned>(std::atoi(argv[2]));
  const unsigned s = static_cast<unsigned>(std::atoi(argv[3]));
  const std::size_t samples = argc > 4 ? std::strtoull(argv[4], nullptr, 10) : 2000;
  const unsigned seed = argc > 5 ? static_cast<unsigned>(std::atoi(argv[5])) : 1;
  if (kmax < 4 || kmax > 10 || s < 8) { std::fprintf(stderr, "kmax in 4..10 and s >= 8 required\n"); return 2; }
  std::vector<Point3> points;
  {
    std::ifstream in(file, std::ios::binary);
    if (!in) { std::fprintf(stderr, "cannot open %s\n", file.c_str()); return 2; }
    std::vector<unsigned char> raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (raw.size() % 6 != 0) { std::fprintf(stderr, "u16le triples expected\n"); return 2; }
    for (std::size_t i = 0; i < raw.size(); i += 6)
      points.push_back({static_cast<std::uint16_t>(raw[i] | (raw[i + 1] << 8)), static_cast<std::uint16_t>(raw[i + 2] | (raw[i + 3] << 8)),
                        static_cast<std::uint16_t>(raw[i + 4] | (raw[i + 5] << 8))});
  }
  const std::size_t n = points.size();
  auto cloud = prepare_cloud(points);
  auto index = make_q2_cloud_index(cloud);
  const auto nodes = index->spatial_nodes(); const auto order = index->spatial_order(); const auto sites = index->cloud().points();
  const std::array<unsigned, 3> thresholds{kmax, kmax - 1, kmax - 2};
  std::vector<Rect> rects;
  const auto t0 = std::chrono::steady_clock::now();
  auto res = run_wspd_front(*index, kmax, s, WspdFrontMode::MidpointSamples,
                            [&](const WspdRectangle& r) { rects.push_back({static_cast<std::uint32_t>(r.a_node), static_cast<std::uint32_t>(r.b_node), r.lane_mask}); }, 6);
  const double front_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  std::array<LaneStats, 3> ls{};
  // Échantillon : M paires par voie, tirées dans la masse résiduelle ; on retient (rectangle, offset) et on juge au passage.
  std::mt19937_64 rng(seed);
  std::array<std::vector<std::pair<std::size_t, u64>>, 3> sample;  // (indice de rectangle, offset de paire), trié par rectangle
  for (unsigned lane = 1; lane < 3; ++lane) {
    std::vector<u64> prefix{0}; std::vector<std::size_t> which;
    for (std::size_t r = 0; r < rects.size(); ++r) {
      if (!(rects[r].mask & (1U << lane))) continue;
      prefix.push_back(prefix.back() + static_cast<u64>(nodes[rects[r].a].range.size()) * nodes[rects[r].b].range.size()); which.push_back(r);
    }
    if (prefix.back() == 0) continue;
    std::uniform_int_distribution<u64> pick(0, prefix.back() - 1);
    for (std::size_t m = 0; m < samples; ++m) {
      const u64 t = pick(rng);
      const std::size_t slot = static_cast<std::size_t>(std::upper_bound(prefix.begin(), prefix.end(), t) - prefix.begin()) - 1;
      sample[lane].push_back({which[slot], t - prefix[slot]});
    }
    std::sort(sample[lane].begin(), sample[lane].end());
  }
  std::array<std::size_t, 3> sample_cursor{};
  std::vector<std::size_t> stack, candidates;
  const auto t1 = std::chrono::steady_clock::now();
  for (std::size_t r = 0; r < rects.size(); ++r) {
    const auto& A = nodes[rects[r].a]; const auto& B = nodes[rects[r].b];
    const u64 mass = static_cast<u64>(A.range.size()) * B.range.size();
    const Q2JointPreparedBounds prepared(A.box, B.box);
    std::array<i64, 3> center4{};
    for (std::size_t axis = 0; axis < 3; ++axis) center4[axis] = static_cast<i64>(A.box.low[axis]) + A.box.high[axis] + B.box.low[axis] + B.box.high[axis];
    for (unsigned lane = 1; lane < 3; ++lane) {
      if (!(rects[r].mask & (1U << lane))) continue;
      auto& L = ls[lane];
      const unsigned h_q = thresholds[lane];
      ++L.rectangles; L.mass += mass;
      // 1. Recherche saturante (tranche 32) : milieu d'abord.
      unsigned credits = 0;
      stack.assign(1, 0);
      while (!stack.empty() && credits < h_q) {
        const auto idx = stack.back(); stack.pop_back();
        const auto& z = nodes[idx]; ++L.sat_visits;
        const auto bnd = prepared.bounds(z.box);
        if (bnd.maximum4 <= 0) continue;
        if (z.left == Q2SpatialNode::absent) {
          const auto rank = z.range.first;
          if (!contains(A.range, rank) && !contains(B.range, rank) && box_witness(A.box, B.box, sites[order[rank]], lane)) ++credits;
          continue;
        }
        if (bnd.minimum4 > 0 && node_universal(prepared, A.box, B.box, z, lane, bnd.minimum4)) {
          credits = static_cast<unsigned>(std::min<u64>(h_q, static_cast<u64>(credits) + z.range.size())); continue;
        }
        const bool left_first = midpoint_distance4(center4, nodes[z.left].box) <= midpoint_distance4(center4, nodes[z.right].box);
        stack.push_back(left_first ? z.right : z.left); stack.push_back(left_first ? z.left : z.right);
      }
      const bool rect_rejected = credits >= h_q;
      if (rect_rejected) { ++L.rect_rejected; L.rect_rejected_mass += mass; }
      u64 universal = 0;
      candidates.clear();
      if (!rect_rejected) {
        // 2. Recherche exhaustive : crédit commun U et candidats C.
        ++L.exhaustive_rects; L.exhaustive_mass += mass;
        stack.assign(1, 0);
        while (!stack.empty()) {
          const auto idx = stack.back(); stack.pop_back();
          const auto& z = nodes[idx]; ++L.exh_visits;
          const auto bnd = prepared.bounds(z.box);
          if (bnd.maximum4 <= 0) continue;
          if (z.left == Q2SpatialNode::absent) {
            ++L.exh_leaf_tests;
            const auto rank = z.range.first;
            if (!contains(A.range, rank) && !contains(B.range, rank) && box_witness(A.box, B.box, sites[order[rank]], lane)) ++universal;
            else candidates.push_back(rank);
            continue;
          }
          if (bnd.minimum4 > 0 && node_universal(prepared, A.box, B.box, z, lane, bnd.minimum4)) { ++L.exh_admitted_nodes; universal += z.range.size(); continue; }
          stack.push_back(z.left); stack.push_back(z.right);
        }
        L.universal_sum += universal; L.candidates_sum += candidates.size(); L.candidates_max = std::max<u64>(L.candidates_max, candidates.size());
        unsigned bucket = 0; for (u64 c = candidates.size() + 1; c > 1; c >>= 1) ++bucket;
        ++L.candidates_hist[std::min<unsigned>(bucket, 19)];
        // 3. Toutes les paires du rectangle ne testent que C.
        for (auto rank_a = A.range.first; rank_a < A.range.last; ++rank_a) {
          const Point3 pa = sites[order[rank_a]];
          for (auto rank_b = B.range.first; rank_b < B.range.last; ++rank_b) {
            const Point3 pb = sites[order[rank_b]];
            ++L.pairs;
            u64 count = universal;
            for (const auto rank_c : candidates) {
              if (rank_c == rank_a || rank_c == rank_b) continue;
              ++L.pair_tests;
              if (lemon(pa, pb, sites[order[rank_c]], lane) && ++count >= h_q) break;
            }
            if (count >= h_q) ++L.pairs_rejected_relay; else ++L.pairs_kept_relay;
          }
        }
      }
      // 4. Échantillon : identité contre balayage complet, coût du relais contre la descente depuis la racine.
      auto& cursor = sample_cursor[lane];
      while (cursor < sample[lane].size() && sample[lane][cursor].first == r) {
        const u64 offset = sample[lane][cursor].second; ++cursor;
        const auto rank_a = A.range.first + static_cast<std::size_t>(offset / B.range.size());
        const auto rank_b = B.range.first + static_cast<std::size_t>(offset % B.range.size());
        const Point3 pa = sites[order[rank_a]], pb = sites[order[rank_b]];
        ++L.sample_pairs;
        u64 total = 0;
        for (std::size_t rank = 0; rank < n; ++rank)
          if (rank != rank_a && rank != rank_b && lemon(pa, pb, sites[order[rank]], lane)) ++total;
        if (rect_rejected) {
          ++L.sample_in_rejected_rect;
          if (total < h_q) ++L.sample_disagreements;  // un rectangle rejeté ne peut contenir une paire non rejetable
        } else {
          u64 count = universal;
          for (const auto rank_c : candidates) {
            if (rank_c == rank_a || rank_c == rank_b) continue;
            ++L.sample_relay_tests;
            if (lemon(pa, pb, sites[order[rank_c]], lane)) ++count;
          }
          if (count != total) ++L.sample_disagreements;  // identité exacte sans saturation
          u64 sat = universal;
          for (const auto rank_c : candidates) {
            if (sat >= h_q) break;
            if (rank_c == rank_a || rank_c == rank_b) continue;
            ++L.sample_relay_tests_saturating;
            if (lemon(pa, pb, sites[order[rank_c]], lane)) ++sat;
          }
        }
        // Descente saturante depuis la racine, boîtes singleton, milieu d'abord (front_lanes_lidar § 4 bis).
        const Box3 box_a = singleton_box(pa), box_b = singleton_box(pb);
        const Q2JointPreparedBounds pair_bounds(box_a, box_b);
        std::array<i64, 3> mid4{};
        for (std::size_t axis = 0; axis < 3; ++axis) mid4[axis] = 2 * (static_cast<i64>(pa[axis]) + pb[axis]);
        unsigned ncount = 0;
        std::vector<std::size_t> st{0};
        while (!st.empty() && ncount < h_q) {
          const auto idx = st.back(); st.pop_back();
          const auto& z = nodes[idx]; ++L.sample_root_visits;
          if (!rect_rejected) ++L.sample_root_visits_exhaustive;
          const auto bnd = pair_bounds.bounds(z.box);
          if (bnd.maximum4 <= 0) continue;
          if (z.left == Q2SpatialNode::absent) {
            const auto rank = z.range.first;
            if (rank != rank_a && rank != rank_b && lemon(pa, pb, sites[order[rank]], lane)) ++ncount;
            continue;
          }
          if (bnd.minimum4 > 0) {
            const i128 xi16 = static_cast<i128>(16) * spindle_detail::xi_bounds(box_a, box_b, z.box).high;
            if (static_cast<i128>(alpha(lane)) * (static_cast<i128>(bnd.minimum4) * bnd.minimum4) > xi16) {
              ncount = static_cast<unsigned>(std::min<u64>(h_q, static_cast<u64>(ncount) + z.range.size())); continue;
            }
          }
          const bool left_first = midpoint_distance4(mid4, nodes[z.left].box) <= midpoint_distance4(mid4, nodes[z.right].box);
          st.push_back(left_first ? z.right : z.left); st.push_back(left_first ? z.left : z.right);
        }
      }
    }
  }
  const double relay_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t1).count();
  const auto& w = res.work;
  std::printf("{\"schema\":\"audit_b_relay_probe_v2\",\"file\":\"%s\",\"n\":%zu,\"kmax\":%u,\"s\":%u,\"mask\":6,\"front_mode\":\"samples\",\"seed\":%u,\"samples_per_lane\":%zu",
              file.c_str(), n, kmax, s, seed, samples);
  std::printf(",\"front\":{\"emitted_rectangles\":%llu,\"lane_rectangles\":[%llu,%llu,%llu],\"residual_pair_mass\":[%llu,%llu,%llu],\"total_unordered_pairs\":%llu,\"ms\":%.1f},\"relay_ms\":%.1f",
              (unsigned long long)w.emitted_rectangles, (unsigned long long)w.lane_rectangles[0], (unsigned long long)w.lane_rectangles[1], (unsigned long long)w.lane_rectangles[2],
              (unsigned long long)w.residual_pair_mass[0], (unsigned long long)w.residual_pair_mass[1], (unsigned long long)w.residual_pair_mass[2],
              (unsigned long long)res.total_unordered_pairs, front_ms, relay_ms);
  for (unsigned lane = 1; lane < 3; ++lane) {
    const auto& L = ls[lane];
    std::printf(",\"q%u\":{\"threshold\":%u,\"rectangles\":%llu,\"mass\":%llu,\"rect_rejected\":%llu,\"rect_rejected_mass\":%llu,\"saturating_visits\":%llu,"
                "\"exhaustive_rects\":%llu,\"exhaustive_mass\":%llu,\"exhaustive_visits\":%llu,\"exhaustive_leaf_tests\":%llu,\"exhaustive_admitted_nodes\":%llu,"
                "\"candidates_sum\":%llu,\"candidates_max\":%llu,\"universal_sum\":%llu,\"candidates_hist\":[",
                lane + 2, thresholds[lane], (unsigned long long)L.rectangles, (unsigned long long)L.mass, (unsigned long long)L.rect_rejected, (unsigned long long)L.rect_rejected_mass,
                (unsigned long long)L.sat_visits, (unsigned long long)L.exhaustive_rects, (unsigned long long)L.exhaustive_mass, (unsigned long long)L.exh_visits,
                (unsigned long long)L.exh_leaf_tests, (unsigned long long)L.exh_admitted_nodes, (unsigned long long)L.candidates_sum, (unsigned long long)L.candidates_max,
                (unsigned long long)L.universal_sum);
    for (std::size_t i = 0; i < L.candidates_hist.size(); ++i) std::printf("%s%llu", i ? "," : "", (unsigned long long)L.candidates_hist[i]);
    std::printf("],\"pairs\":%llu,\"pair_tests\":%llu,\"pairs_rejected_relay\":%llu,\"pairs_kept_relay\":%llu,"
                "\"sample\":{\"pairs\":%llu,\"disagreements\":%llu,\"relay_tests\":%llu,\"relay_tests_saturating\":%llu,\"root_visits\":%llu,\"root_visits_exhaustive\":%llu,\"in_rejected_rect\":%llu}}",
                (unsigned long long)L.pairs, (unsigned long long)L.pair_tests, (unsigned long long)L.pairs_rejected_relay, (unsigned long long)L.pairs_kept_relay,
                (unsigned long long)L.sample_pairs, (unsigned long long)L.sample_disagreements, (unsigned long long)L.sample_relay_tests, (unsigned long long)L.sample_relay_tests_saturating,
                (unsigned long long)L.sample_root_visits, (unsigned long long)L.sample_root_visits_exhaustive, (unsigned long long)L.sample_in_rejected_rect);
  }
  std::printf(",\"index_nodes\":%zu}\n", nodes.size());
  return 0;
}
