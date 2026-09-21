// Auditeur B, 21 septembre 2026 — voies q3/q4 du front WSPD sur un scan LiDAR (sources c5308651).
// Question du constructeur (tranche31) : au front multivoie, la faiblesse dominante est-elle le choix de K témoins
// autour du pivot, ou l'absence de crédits h_a/h_b transmis par blocs ? Deux mesures exactes, aucun flottant :
//  1. PLAFOND par rectangle résiduel : pour chaque voie q ∈ {3,4} portée par un rectangle A×B émis par le front
//     (masque 6, mode MidpointSamples, options par défaut), nombre U_q de sites z hors A∪B vérifiant le prédicat de
//     boîte du front (h_min(A.box,B.box,{z}) > 0 et α_q·h_min² > Ξ_max, α3 = 3, α4 = 2), par descente préordre de
//     l'index avec bornes conjointes, plafonné au seuil h_q = Kmax + 2 − q. U_q ≥ h_q : un proposeur ponctuel parfait
//     rejetait la voie sur ce rectangle (la fenêtre de K rangs ne l'a pas fait) ; U_q < h_q : aucun proposeur de sites
//     testés contre les boîtes ne le peut, quelle que soit sa fenêtre.
//  2. JUGE D'ÉCHANTILLON exact par paire : M paires (a,b) tirées uniformément dans la masse résiduelle de la voie
//     (rectangle ∝ |A||B|, puis paire uniforme), et pour chacune le compte exact c_q des sites z ≠ a,b du citron
//     L_q(a,b) = {H(z) > 0 et α_q·H(z)² > Ξ(z)}, H = (z−a)·(b−z), Ξ = |(z−a)×(b−z)|², par balayage complet du nuage.
//     c_q ≥ h_q : la paire est rejetable exactement pour la voie q (elle ne porte aucun support q positif propriétaire de
//     profondeur < h_q, lemme du citron). Les témoins de chaque paire sont classés : universels de boîte du rectangle
//     (accessibles à un proposeur ponctuel), non universels hors A∪B, dans A, dans B (accessibles seulement à des
//     crédits par paire ou par bloc, h_a/h_b).
//  3. FENÊTRES L = K, 2K, 4K de rangs autour du pivot du front (même descente vers le milieu, même formule de fenêtre,
//     rangs de A/B sautés, arrêt à h_q crédits) : rectangles et masse que la voie perdrait avec une fenêtre élargie ;
//     L = K doit rejeter zéro rectangle émis (contrôle de cohérence avec le front).
//  4. DESCENTE SATURANTE par paire (le « second front » demandé avant covers et seeds) : pour chaque paire tirée, compte
//     exact c_q par descente préordre de l'index avec les boîtes singleton de a et b (nœud entier admis si 4H_min > 0 et
//     α_q·(4H_min)² > 16·Ξ_max, nœud écarté si 4H_max ≤ 0, feuilles jugées par le citron exact), arrêt à h_q ; la
//     décision doit coïncider avec le balayage complet (désaccords comptés, exigés nuls) ; visites de nœuds relevées.
//     Deux ordres sont mesurés : préordre de l'index (gauche d'abord) et « milieu d'abord » (pile explicite, enfant le
//     plus proche du milieu de ab visité en premier, égalité à gauche), même décision exigée.
//  5. COUVERTURE ET SEEDS par paire : sites de la couverture close du constructeur |2z−a−b|² ≤ 4|b−a|² et de la
//     couverture serrée |2z−a−b|² ≤ 3|b−a|² (rayon √3·D suffisant pour toute boule q3 propriétaire), seeds q3 aigus à
//     arête ab maximale (égalités incluses), et, pour les paires non rejetables de la voie q3, nombre de seeds dont la
//     circumboule a une profondeur < h_q (prédicat i128 Δ|z−a|² < (z−a)·N) ; pour les premières paires rejetables,
//     vérification exécutable du lemme du citron (aucun seed de profondeur < h_q, violations exigées nulles).
//  V2 (contrôle croisé spatial) : le balayage complet O(n) par paire est remplacé par deux descentes exactes de l'index,
//     (a) la descente saturante « milieu d'abord » avec les boîtes singleton de a et b pour la décision du citron
//     (identique au § 4 bis, validée contre le balayage complet sur 72 000 paires dans front_lanes_lidar_20260921), et
//     (b) une descente de couverture : un nœud est ouvert si le minimum sur sa boîte de |2z−a−b|² (somme par axe du
//     carré minimal de l'intervalle [2·low−a−b, 2·high−a−b], nul s'il contient 0) est ≤ 4|b−a|², ses feuilles sont
//     testées exactement. Le compte exact du citron n'est plus disponible pour les paires conservées (seule la décision
//     l'est) ; les classes de témoins sont mises à zéro (le compte saturé n'est pas un compte de témoins) et la
//     couverture n'est descendue que pour les paires conservées ou les paires rejetables soumises au lemme ; les deux
//     descentes de contrôle du § 4 (préordre) et du § 4 bis sont retirées (la descente (a) est celle du § 4 bis, dont la
//     décision a coïncidé avec le balayage complet sur 72 000 paires tirées et 15,7 M paires exhaustives des reçus
//     T32_8k et T34) ; le § 3 (fenêtres) et le § 1 (plafond) restent calculés par rectangle. Tout le reste (seeds,
//     propriété, census entier, lemme) est inchangé, et les sorties (supports, profondeurs) doivent coïncider avec la V1.
//  6. MODE EXHAUSTIF (samples_per_lane = 0) : toutes les paires résiduelles de chaque voie sont jugées dans l'ordre des
//     rectangles (aucun tirage) ; les seeds q3 sont comptés avec la règle de propriété du raccord 31 (égalité admise,
//     départage par la plus petite clé d'IDs), les boules émises des paires conservées sont écrites en clair
//     (triplets d'IDs triés, un par ligne) dans le fichier optionnel donné en septième argument, pour comparaison
//     exacte avec le flux q3 du moteur ; le lemme est vérifié sur les `lemma_budget` premières paires rejetables.
// Rien n'est modifié dans le moteur ; le front est appelé tel quel. Compter, jamais promouvoir.
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
// Distance de boîte au milieu du produit, en quarts d'unité : la règle de descente du front (égalité à gauche).
i64 midpoint_distance4(const std::array<i64, 3>& center4, const Box3& box) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto delta = std::max<i64>({0, 4 * static_cast<i64>(box.low[axis]) - center4[axis], center4[axis] - 4 * static_cast<i64>(box.high[axis])});
    result += delta * delta;
  }
  return result;
}
unsigned alpha(unsigned lane) { return lane == 1 ? 3U : 2U; }  // lane 1 = q3, lane 2 = q4
// Prédicat de boîte du front pour un site z (identique à Front::filter : h_minimum strict puis α·h² > Ξ_high).
bool box_witness(const Box3& a, const Box3& b, const Point3& z, unsigned lane) {
  const auto zb = singleton_box(z);
  const i64 h = spindle_detail::h_minimum(a, b, zb);
  if (h <= 0) return false;
  const i128 xi = spindle_detail::xi_bounds(a, b, zb).high;
  return static_cast<i128>(alpha(lane)) * spindle_detail::square(h) > xi;
}
// Citron exact de la paire (a,b) : H > 0 et α·H² > Ξ, en entiers (H ≤ 3·65535² < 2^34, Ξ < 2^68).
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
// Circumboule exacte du triangle (a,b,c) : u = b−a, v = c−a, w = u×v, Δ = |w|² (> 0 si non aligné),
// N = |u|²·(v×w) + |v|²·(w×u), centre o = a + N/(2Δ). z strictement intérieur ⟺ Δ·|z−a|² < (z−a)·N (tout < 2^103).
struct Circum { i128 delta{}; std::array<i128, 3> N{}; };
std::array<i64, 3> cross(const std::array<i64, 3>& p, const std::array<i64, 3>& q) {
  return {p[1] * q[2] - p[2] * q[1], p[2] * q[0] - p[0] * q[2], p[0] * q[1] - p[1] * q[0]};
}
Circum circum(const Point3& a, const Point3& b, const Point3& c) {
  std::array<i64, 3> u{}, v{};
  for (std::size_t axis = 0; axis < 3; ++axis) { u[axis] = static_cast<i64>(b[axis]) - a[axis]; v[axis] = static_cast<i64>(c[axis]) - a[axis]; }
  const auto w = cross(u, v);
  const auto vw = cross(v, w), wu = cross(w, u);
  i64 uu = 0, vv = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) { uu += u[axis] * u[axis]; vv += v[axis] * v[axis]; }
  Circum out;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    out.delta += static_cast<i128>(w[axis]) * w[axis];
    out.N[axis] = static_cast<i128>(uu) * vw[axis] + static_cast<i128>(vv) * wu[axis];
  }
  return out;
}
bool inside_circum(const Circum& C, const Point3& a, const Point3& z) {
  i128 lhs = 0, rhs = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 d = static_cast<i64>(z[axis]) - a[axis];
    lhs += static_cast<i128>(d) * d;
    rhs += static_cast<i128>(d) * C.N[axis];
  }
  return C.delta * lhs < rhs;
}
i64 sq_dist(const Point3& p, const Point3& q) {
  i64 out = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) { const i64 d = static_cast<i64>(p[axis]) - q[axis]; out += d * d; }
  return out;
}
struct Rect { std::uint32_t a, b; std::uint8_t mask; };
std::pair<std::size_t, std::size_t> edge_key(std::size_t a, std::size_t b) { return a < b ? std::pair{a, b} : std::pair{b, a}; }
// Minimum exact de |2z−a−b|² sur les z entiers ou réels d'une boîte (somme séparable).
i64 min_cover_distance(const Box3& box, const Point3& a, const Point3& b) {
  i64 out = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 lo = 2 * static_cast<i64>(box.low[axis]) - a[axis] - b[axis], hi = 2 * static_cast<i64>(box.high[axis]) - a[axis] - b[axis];
    const i64 m = (lo <= 0 && hi >= 0) ? 0 : std::min(lo * lo, hi * hi);
    out += m;
  }
  return out;
}
}  // namespace
int main(int argc, char** argv) {
  if (argc < 4) { std::fprintf(stderr, "usage: file.u16le kmax s [samples_per_lane=2000|0=exhaustif] [seed=1] [samples|pure] [supports_out] [lemma_budget=100]\n"); return 2; }
  const std::string file = argv[1];
  const unsigned kmax = static_cast<unsigned>(std::atoi(argv[2]));
  const unsigned s = static_cast<unsigned>(std::atoi(argv[3]));
  const std::size_t samples = argc > 4 ? std::strtoull(argv[4], nullptr, 10) : 2000;
  const unsigned seed = argc > 5 ? static_cast<unsigned>(std::atoi(argv[5])) : 1;
  const std::string fmode = argc > 6 ? argv[6] : "samples";
  const std::string supports_out = argc > 7 ? argv[7] : "";
  const u64 lemma_budget = argc > 8 ? std::strtoull(argv[8], nullptr, 10) : 100;  // paires rejetables q3 soumises au lemme
  if (kmax < 3 || kmax > 10 || s < 8) { std::fprintf(stderr, "kmax in 3..10 and s >= 8 required\n"); return 2; }
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
  auto res = run_wspd_front(*index, kmax, s, fmode == "pure" ? WspdFrontMode::Pure : WspdFrontMode::MidpointSamples,
                            [&](const WspdRectangle& r) { rects.push_back({static_cast<std::uint32_t>(r.a_node), static_cast<std::uint32_t>(r.b_node), r.lane_mask}); }, 6);
  const double front_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  // 1. Plafond par rectangle et par voie.
  std::array<std::vector<u64>, 3> rect_hist, mass_hist;
  for (unsigned lane = 1; lane < 3; ++lane) { rect_hist[lane].assign(thresholds[lane] + 1, 0); mass_hist[lane].assign(thresholds[lane] + 1, 0); }
  std::array<u64, 3> lane_rects{}, lane_mass{}, node_visits{}, point_tests{};
  std::vector<std::array<std::uint8_t, 3>> ceiling(rects.size());  // U_q plafonné, par rectangle
  const std::array<unsigned, 3> windows{kmax, 2 * kmax, 4 * kmax};
  std::array<std::array<u64, 3>, 3> win_rects{}, win_mass{};  // [lane][window]
  std::array<u64, 3> path_steps{}, window_tests{}, unsearchable{};
  const auto t1 = std::chrono::steady_clock::now();
  for (std::size_t r = 0; r < rects.size(); ++r) {
    const auto& A = nodes[rects[r].a]; const auto& B = nodes[rects[r].b];
    const u64 mass = static_cast<u64>(A.range.size()) * B.range.size();
    const Q2JointPreparedBounds prepared(A.box, B.box);
    for (unsigned lane = 1; lane < 3; ++lane) {
      if (!(rects[r].mask & (1U << lane))) continue;
      const unsigned h_q = thresholds[lane];
      unsigned count = 0;
      std::size_t cursor = 0;
      while (cursor < nodes.size() && count < h_q) {
        const auto& z = nodes[cursor]; ++node_visits[lane];
        const auto bnd = prepared.bounds(z.box);  // extrema exacts de 4H sur A.box × B.box × z.box
        if (bnd.maximum4 <= 0) { cursor = z.escape; continue; }
        if (z.left == Q2SpatialNode::absent) {
          ++point_tests[lane];
          const auto rank = z.range.first;
          if (!contains(A.range, rank) && !contains(B.range, rank) && box_witness(A.box, B.box, sites[order[rank]], lane)) ++count;
          cursor = z.escape; continue;
        }
        if (bnd.minimum4 > 0) {
          // Tout le nœud est universel pour q si α·(4H_min)² > 16·Ξ_high (Ξ_high pris sur la boîte du nœud).
          const i128 xi16 = static_cast<i128>(16) * spindle_detail::xi_bounds(A.box, B.box, z.box).high;
          if (static_cast<i128>(alpha(lane)) * (static_cast<i128>(bnd.minimum4) * bnd.minimum4) > xi16) {
            count = static_cast<unsigned>(std::min<u64>(h_q, static_cast<u64>(count) + z.range.size()));
            cursor = z.escape; continue;
          }
        }
        cursor = z.left;
      }
      ceiling[r][lane] = static_cast<std::uint8_t>(count);
      rect_hist[lane][count] += 1; mass_hist[lane][count] += mass;
      lane_rects[lane] += 1; lane_mass[lane] += mass;
    }
    // 3. Fenêtres autour du pivot du front (une descente par rectangle, partagée par les voies).
    if (n - A.range.size() - B.range.size() < thresholds[2]) { for (unsigned lane = 1; lane < 3; ++lane) if (rects[r].mask & (1U << lane)) ++unsearchable[lane]; continue; }
    std::array<i64, 3> center4{};
    for (std::size_t axis = 0; axis < 3; ++axis) center4[axis] = static_cast<i64>(A.box.low[axis]) + A.box.high[axis] + B.box.low[axis] + B.box.high[axis];
    std::size_t node = 0;
    while (nodes[node].left != Q2SpatialNode::absent) {
      ++path_steps[1];
      const auto left = nodes[node].left, right = nodes[node].right;
      node = midpoint_distance4(center4, nodes[left].box) <= midpoint_distance4(center4, nodes[right].box) ? left : right;
    }
    const auto pivot = nodes[node].range.first;
    for (unsigned lane = 1; lane < 3; ++lane) {
      if (!(rects[r].mask & (1U << lane))) continue;
      const unsigned h_q = thresholds[lane];
      for (std::size_t wdx = 0; wdx < windows.size(); ++wdx) {
        const auto L = std::min<std::size_t>(windows[wdx], n);
        const auto first = std::min(pivot > L / 2 ? pivot - L / 2 : 0, n - L);
        unsigned credits = 0;
        for (auto rank = first; rank < first + L && credits < h_q; ++rank) {
          if (contains(A.range, rank) || contains(B.range, rank)) continue;
          ++window_tests[lane];
          if (box_witness(A.box, B.box, sites[order[rank]], lane)) ++credits;
        }
        if (credits >= h_q) { win_rects[lane][wdx] += 1; win_mass[lane][wdx] += mass; }
      }
    }
  }
  const double ceil_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t1).count();
  // 2. Juge d'échantillon exact par paire, descente saturante, couverture, seeds et lemme du citron.
  u64 leaf_multi = 0;
  for (const auto& z : nodes) if (z.left == Q2SpatialNode::absent && z.range.size() != 1) ++leaf_multi;
  std::mt19937_64 rng(seed);
  struct LaneSample { u64 pairs{}, rejectable{}, rejectable_with_ceiling{}, rejectable_without_ceiling{}, kept{}, kept_with_ceiling{};
                      u64 w_universal{}, w_nonuniversal_outside{}, w_in_a{}, w_in_b{}, w_total{}, scan_sites{};
                      u64 d_visits{}, d_leaf_tests{}, d_whole_nodes{}, d_visits_rejectable{}, d_visits_kept{}, d_visits_max{}, d_disagreements{};
                      u64 n_visits{}, n_visits_rejectable{}, n_visits_kept{}, n_visits_max{}, n_disagreements{};
                      u64 cover_rejectable{}, cover_kept{}, cover_kept_max{}, tight_rejectable{}, tight_kept{};
                      u64 seeds_rejectable{}, seeds_kept{}, seeds_kept_max{}, emitted_kept{}, kept_with_emission{}, census_tests_kept{};
                      u64 lemma_pairs{}, lemma_seeds{}, lemma_tests{}, lemma_violations{}; };
  std::array<LaneSample, 3> ls{};
  std::FILE* supports = supports_out.empty() ? nullptr : std::fopen(supports_out.c_str(), "w");
  if (!supports_out.empty() && !supports) { std::fprintf(stderr, "cannot write %s\n", supports_out.c_str()); return 2; }
  const bool exhaustive = samples == 0;
  std::vector<std::size_t> cover_ranks, seed_ranks;
  const auto t2 = std::chrono::steady_clock::now();
  for (unsigned lane = 1; lane < 3; ++lane) {
    std::vector<u64> prefix; prefix.reserve(rects.size() + 1); prefix.push_back(0);
    std::vector<std::size_t> which; which.reserve(rects.size());
    for (std::size_t r = 0; r < rects.size(); ++r) {
      if (!(rects[r].mask & (1U << lane))) continue;
      const auto& A = nodes[rects[r].a]; const auto& B = nodes[rects[r].b];
      prefix.push_back(prefix.back() + static_cast<u64>(A.range.size()) * B.range.size()); which.push_back(r);
    }
    if (prefix.back() == 0) continue;
    std::uniform_int_distribution<u64> pick(0, prefix.back() - 1);
    const unsigned h_q = thresholds[lane];
    const u64 draws = exhaustive ? prefix.back() : samples;
    for (u64 m = 0; m < draws; ++m) {
      const u64 t = exhaustive ? m : pick(rng);
      const auto it = std::upper_bound(prefix.begin(), prefix.end(), t);
      const std::size_t slot = static_cast<std::size_t>(it - prefix.begin()) - 1;
      const auto r = which[slot];
      const auto& A = nodes[rects[r].a]; const auto& B = nodes[rects[r].b];
      const u64 offset = t - prefix[slot];
      const auto rank_a = A.range.first + static_cast<std::size_t>(offset / B.range.size());
      const auto rank_b = B.range.first + static_cast<std::size_t>(offset % B.range.size());
      const Point3 pa = sites[order[rank_a]], pb = sites[order[rank_b]];
      const i64 L2 = sq_dist(pa, pb);  // |b−a|² = 4D²
      u64 total = 0, universal = 0, in_a = 0, in_b = 0, cover = 0, tight = 0;
      cover_ranks.clear(); seed_ranks.clear();
      // (a) Décision du citron par descente saturante « milieu d'abord » (boîtes singleton).
      {
        const Box3 box_a0 = singleton_box(pa), box_b0 = singleton_box(pb);
        const Q2JointPreparedBounds pb0(box_a0, box_b0);
        std::array<i64, 3> mid4{};
        for (std::size_t axis = 0; axis < 3; ++axis) mid4[axis] = 2 * (static_cast<i64>(pa[axis]) + pb[axis]);
        unsigned c = 0;
        std::vector<std::size_t> st{0};
        while (!st.empty() && c < h_q) {
          const auto idx = st.back(); st.pop_back();
          const auto& z = nodes[idx];
          const auto bnd = pb0.bounds(z.box);
          if (bnd.maximum4 <= 0) continue;
          if (z.left == Q2SpatialNode::absent) {
            const auto rank = z.range.first;
            if (rank != rank_a && rank != rank_b && lemon(pa, pb, sites[order[rank]], lane)) ++c;
            continue;
          }
          if (bnd.minimum4 > 0) {
            const i128 xi16 = static_cast<i128>(16) * spindle_detail::xi_bounds(box_a0, box_b0, z.box).high;
            if (static_cast<i128>(alpha(lane)) * (static_cast<i128>(bnd.minimum4) * bnd.minimum4) > xi16) {
              c = static_cast<unsigned>(std::min<u64>(h_q, static_cast<u64>(c) + z.range.size())); continue;
            }
          }
          const bool lf = midpoint_distance4(mid4, nodes[z.left].box) <= midpoint_distance4(mid4, nodes[z.right].box);
          st.push_back(lf ? z.right : z.left); st.push_back(lf ? z.left : z.right);
        }
        total = c;  // saturé à h_q : décision exacte, compte exact seulement en dessous du seuil
      }
      const bool rejectable_now = total >= h_q;
      const bool need_cover = !rejectable_now || ls[lane].lemma_pairs < lemma_budget;
      // (b) Couverture fermée |2z−a−b|² ≤ 4|b−a|² par descente avec borne inférieure de boîte, feuilles exactes.
      if (need_cover) {
        std::vector<std::size_t> st{0};
        while (!st.empty()) {
          const auto idx = st.back(); st.pop_back();
          const auto& z = nodes[idx];
          if (min_cover_distance(z.box, pa, pb) > 4 * L2) continue;
          if (z.left == Q2SpatialNode::absent) {
            const auto rank = z.range.first;
            if (rank == rank_a || rank == rank_b) continue;
            const Point3 pz = sites[order[rank]];
            i64 d2 = 0, hz = 0;
            for (std::size_t axis = 0; axis < 3; ++axis) {
              const i64 e = 2 * static_cast<i64>(pz[axis]) - pa[axis] - pb[axis]; d2 += e * e;
              hz += (static_cast<i64>(pa[axis]) - pz[axis]) * (static_cast<i64>(pb[axis]) - pz[axis]);
            }
            if (d2 <= 4 * L2) {
              ++cover; cover_ranks.push_back(rank);
              if (d2 <= 3 * L2) ++tight;
              if (lane == 1 && hz > 0) {
                const i64 az = sq_dist(pz, pa), bz = sq_dist(pz, pb);
                const auto ida = order[rank_a], idb = order[rank_b], idz = order[rank];
                const auto owner = edge_key(ida, idb);
                const bool owned = !(az > L2 || (az == L2 && edge_key(ida, idz) < owner)) && !(bz > L2 || (bz == L2 && edge_key(idb, idz) < owner));
                if (owned) seed_ranks.push_back(rank);
              }
            }
            continue;
          }
          st.push_back(z.right); st.push_back(z.left);
        }
        std::sort(cover_ranks.begin(), cover_ranks.end()); std::sort(seed_ranks.begin(), seed_ranks.end());
      }
      auto& L = ls[lane];
      ++L.pairs; (void)universal; (void)in_a; (void)in_b;  // V2 : pas de balayage, pas de classes de témoins
      const bool rejectable = total >= h_q;
      const bool ceil_ok = ceiling[r][lane] >= h_q;
      if (rejectable) { ++L.rejectable; if (ceil_ok) ++L.rejectable_with_ceiling; else ++L.rejectable_without_ceiling; }
      else { ++L.kept; if (ceil_ok) ++L.kept_with_ceiling; }
      // V2 : les § 4 et 4 bis (descentes de contrôle) sont retirés ; la décision vient de (a).
      // 5. Couverture, seeds et profondeur des circumboules (voie q3).
      if (rejectable) { L.cover_rejectable += cover; L.tight_rejectable += tight; L.seeds_rejectable += seed_ranks.size(); }
      else { L.cover_kept += cover; L.tight_kept += tight; L.cover_kept_max = std::max(L.cover_kept_max, cover); L.seeds_kept += seed_ranks.size();
             L.seeds_kept_max = std::max<u64>(L.seeds_kept_max, seed_ranks.size()); }
      if (lane == 1 && (!rejectable || L.lemma_pairs < lemma_budget)) {
        u64 emitted = 0, tests = 0;
        for (const auto rank_c : seed_ranks) {
          const Point3 pc = sites[order[rank_c]];
          const Circum C = circum(pa, pb, pc);
          u64 depth = 0;
          for (const auto rank_z : cover_ranks) {
            if (rank_z == rank_c) continue;
            ++tests;
            if (inside_circum(C, pa, sites[order[rank_z]])) { if (++depth >= h_q) break; }
          }
          if (depth < h_q) {
            ++emitted;
            if (supports && !rejectable) {
              std::array<std::size_t, 3> ids{order[rank_a], order[rank_b], order[rank_c]};
              std::sort(ids.begin(), ids.end());
              std::fprintf(supports, "%zu %zu %zu %llu\n", ids[0], ids[1], ids[2], (unsigned long long)depth);
            }
          }
        }
        if (rejectable) { ++L.lemma_pairs; L.lemma_seeds += seed_ranks.size(); L.lemma_tests += tests; L.lemma_violations += emitted; }
        else { L.emitted_kept += emitted; L.census_tests_kept += tests; if (emitted) ++L.kept_with_emission; }
      }
    }
  }
  const double sample_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t2).count();
  const auto& w = res.work;
  std::printf("{\"schema\":\"audit_b_front_lanes_lidar_v5_spatial\",\"file\":\"%s\",\"n\":%zu,\"kmax\":%u,\"s\":%u,\"front_mode\":\"%s\",\"mask\":6,\"seed\":%u,\"samples_per_lane\":%zu",
              file.c_str(), n, kmax, s, fmode.c_str(), seed, samples);
  std::printf(",\"front\":{\"product_visits\":%llu,\"witness_searches\":%llu,\"emitted_rectangles\":%llu,\"fully_rejected_products\":%llu,\"lane_rectangles\":[%llu,%llu,%llu],\"rejected_pair_mass\":[%llu,%llu,%llu],\"residual_pair_mass\":[%llu,%llu,%llu],\"total_unordered_pairs\":%llu,\"active_lane_mask\":%u,\"ms\":%.1f}",
              (unsigned long long)w.product_visits, (unsigned long long)w.witness_searches, (unsigned long long)w.emitted_rectangles, (unsigned long long)w.fully_rejected_products,
              (unsigned long long)w.lane_rectangles[0], (unsigned long long)w.lane_rectangles[1], (unsigned long long)w.lane_rectangles[2],
              (unsigned long long)w.rejected_pair_mass[0], (unsigned long long)w.rejected_pair_mass[1], (unsigned long long)w.rejected_pair_mass[2],
              (unsigned long long)w.residual_pair_mass[0], (unsigned long long)w.residual_pair_mass[1], (unsigned long long)w.residual_pair_mass[2],
              (unsigned long long)res.total_unordered_pairs, static_cast<unsigned>(res.active_lane_mask), front_ms);
  std::printf(",\"ceiling\":{\"ms\":%.1f", ceil_ms);
  for (unsigned lane = 1; lane < 3; ++lane) {
    const unsigned h_q = thresholds[lane];
    std::printf(",\"q%u\":{\"threshold\":%u,\"rectangles\":%llu,\"mass\":%llu,\"rectangles_at_threshold\":%llu,\"mass_at_threshold\":%llu,\"node_visits\":%llu,\"point_tests\":%llu,\"rect_hist\":[",
                lane + 2, h_q, (unsigned long long)lane_rects[lane], (unsigned long long)lane_mass[lane], (unsigned long long)rect_hist[lane][h_q], (unsigned long long)mass_hist[lane][h_q],
                (unsigned long long)node_visits[lane], (unsigned long long)point_tests[lane]);
    for (unsigned c = 0; c <= h_q; ++c) std::printf("%s%llu", c ? "," : "", (unsigned long long)rect_hist[lane][c]);
    std::printf("],\"mass_hist\":[");
    for (unsigned c = 0; c <= h_q; ++c) std::printf("%s%llu", c ? "," : "", (unsigned long long)mass_hist[lane][c]);
    std::printf("],\"windows\":{\"path_steps\":%llu,\"window_tests\":%llu,\"unsearchable\":%llu", (unsigned long long)path_steps[1], (unsigned long long)window_tests[lane], (unsigned long long)unsearchable[lane]);
    for (std::size_t wdx = 0; wdx < windows.size(); ++wdx)
      std::printf(",\"L%uK\":{\"rectangles\":%llu,\"mass\":%llu}", static_cast<unsigned>(1U << wdx), (unsigned long long)win_rects[lane][wdx], (unsigned long long)win_mass[lane][wdx]);
    std::printf("}}");
  }
  std::printf("},\"sample\":{\"ms\":%.1f", sample_ms);
  for (unsigned lane = 1; lane < 3; ++lane) {
    const auto& L = ls[lane];
    std::printf(",\"q%u\":{\"pairs\":%llu,\"rejectable\":%llu,\"rejectable_with_ceiling\":%llu,\"rejectable_without_ceiling\":%llu,\"kept\":%llu,\"kept_with_ceiling\":%llu,\"witnesses_total\":%llu,\"witnesses_universal_box\":%llu,\"witnesses_nonuniversal_outside\":%llu,\"witnesses_in_a\":%llu,\"witnesses_in_b\":%llu,\"scanned_sites\":%llu",
                lane + 2, (unsigned long long)L.pairs, (unsigned long long)L.rejectable, (unsigned long long)L.rejectable_with_ceiling, (unsigned long long)L.rejectable_without_ceiling,
                (unsigned long long)L.kept, (unsigned long long)L.kept_with_ceiling, (unsigned long long)L.w_total, (unsigned long long)L.w_universal,
                (unsigned long long)L.w_nonuniversal_outside, (unsigned long long)L.w_in_a, (unsigned long long)L.w_in_b, (unsigned long long)L.scan_sites);
    std::printf(",\"descent\":{\"node_visits\":%llu,\"leaf_tests\":%llu,\"whole_nodes\":%llu,\"visits_rejectable\":%llu,\"visits_kept\":%llu,\"visits_max\":%llu,\"disagreements\":%llu}",
                (unsigned long long)L.d_visits, (unsigned long long)L.d_leaf_tests, (unsigned long long)L.d_whole_nodes, (unsigned long long)L.d_visits_rejectable,
                (unsigned long long)L.d_visits_kept, (unsigned long long)L.d_visits_max, (unsigned long long)L.d_disagreements);
    std::printf(",\"descent_near_first\":{\"node_visits\":%llu,\"visits_rejectable\":%llu,\"visits_kept\":%llu,\"visits_max\":%llu,\"disagreements\":%llu}",
                (unsigned long long)L.n_visits, (unsigned long long)L.n_visits_rejectable, (unsigned long long)L.n_visits_kept, (unsigned long long)L.n_visits_max, (unsigned long long)L.n_disagreements);
    std::printf(",\"cover\":{\"sites_rejectable\":%llu,\"sites_kept\":%llu,\"sites_kept_max\":%llu,\"tight_rejectable\":%llu,\"tight_kept\":%llu}",
                (unsigned long long)L.cover_rejectable, (unsigned long long)L.cover_kept, (unsigned long long)L.cover_kept_max, (unsigned long long)L.tight_rejectable, (unsigned long long)L.tight_kept);
    std::printf(",\"seeds\":{\"rejectable\":%llu,\"kept\":%llu,\"kept_max\":%llu,\"emitted_kept\":%llu,\"kept_with_emission\":%llu,\"census_tests_kept\":%llu}",
                (unsigned long long)L.seeds_rejectable, (unsigned long long)L.seeds_kept, (unsigned long long)L.seeds_kept_max, (unsigned long long)L.emitted_kept,
                (unsigned long long)L.kept_with_emission, (unsigned long long)L.census_tests_kept);
    std::printf(",\"lemma\":{\"pairs\":%llu,\"seeds\":%llu,\"tests\":%llu,\"violations\":%llu}}",
                (unsigned long long)L.lemma_pairs, (unsigned long long)L.lemma_seeds, (unsigned long long)L.lemma_tests, (unsigned long long)L.lemma_violations);
  }
  if (supports) std::fclose(supports);
  std::printf("},\"index\":{\"nodes\":%zu,\"leaves_with_several_ranks\":%llu},\"exhaustive\":%s,\"lemma_budget\":%llu}\n", nodes.size(), (unsigned long long)leaf_multi, exhaustive ? "true" : "false", (unsigned long long)lemma_budget);
  return 0;
}
