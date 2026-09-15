// Auditeur B (15 sept. 2026) — plafond de tout proposeur de témoins du front q2 (sources 2741d614).
// Pour chaque rectangle émis par le front (produit A×B non rejeté par la fenêtre historique de K propositions),
// quatre mesures exactes sur le même prédicat que le front (H_min(A.box, B.box, {z}) > 0, témoin « universel de boîte ») :
//  1. plafond : nombre U de sites z universels, par descente préordre de l'index avec bornes conjointes, plafonné à K.
//     U ≥ K : rejetable par un proposeur parfait (et par aucun ancêtre si U < K, les boîtes des ancêtres étant plus grandes) ;
//  2. certificat de bloc : sur le chemin de descente du front vers le milieu du produit (mêmes distances, mêmes égalités),
//     taille du plus haut nœud dont la borne conjointe est strictement positive (0 si aucun) ; certificat si taille ≥ K ;
//  3. fenêtres L = K, 2K, 4K autour du pivot du front (même formule de fenêtre, mêmes sauts des rangs de A/B) : crédits
//     stricts ; rejet si ≥ K crédits. L = K doit rejeter zéro rectangle émis (contrôle de cohérence avec le front) ;
//  4. masse de paires |A|·|B| portée par chaque classe (masse résiduelle q2 du front = candidats du census avant filtrage Pool).
// Le front est lancé avec le masque 1 (voie q2 seule), comme par le census intégré.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "wspd/front.hpp"
#include "pipeline/q2_census.hpp"
#include "pipeline/q2_joint_bounds.hpp"
#include "front_fixtures.hpp"
using namespace mhgp8;
namespace {
i64 midpoint_distance4(const std::array<i64, 3>& center4, const Box3& box) {
  i64 result = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const auto delta = std::max<i64>({0, 4 * static_cast<i64>(box.low[axis]) - center4[axis],
                                       center4[axis] - 4 * static_cast<i64>(box.high[axis])});
    result += delta * delta;
  }
  return result;
}
bool contains(Range range, std::size_t rank) { return rank >= range.first && rank < range.last; }
unsigned block_bin(u64 size) { return size == 0 ? 0 : size == 1 ? 1 : size < 5 ? 2 : size < 10 ? 3 : size < 16 ? 4 : size < 32 ? 5 : 6; }
}  // namespace
int main(int argc, char** argv) {
  if (argc < 5) { std::fprintf(stderr, "usage: n family kmax s [seed] [pure|samples]\n"); return 2; }
  const std::size_t n = std::strtoull(argv[1], nullptr, 10); const std::string fam = argv[2];
  const unsigned kmax = std::atoi(argv[3]); const unsigned s = std::atoi(argv[4]); const unsigned seed = argc > 5 ? std::atoi(argv[5]) : 3;
  const std::string fmode = argc > 6 ? argv[6] : "samples";
  auto fx = bench::make_front_fixture(n, fam, seed);
  auto cloud = prepare_cloud(fx.points); auto index = make_q2_cloud_index(cloud);
  const auto nodes = index->spatial_nodes(); const auto order = index->spatial_order(); const auto points = index->cloud().points();
  std::vector<u64> rect_hist(kmax + 1, 0), mass_hist(kmax + 1, 0);
  std::array<u64, 7> block_hist{};
  const std::array<unsigned, 3> windows{kmax, 2 * kmax, 4 * kmax};
  std::array<u64, 3> win_rects{}, win_mass{};
  u64 rects = 0, node_visits = 0, mass_total = 0, block_rects = 0, block_mass = 0, path_steps = 0, window_tests = 0;
  const auto t0 = std::chrono::steady_clock::now();
  auto consumer = [&](const WspdRectangle& r) {
    if (!(r.lane_mask & 1U)) return;
    ++rects;
    const auto& A = nodes[r.a_node]; const auto& B = nodes[r.b_node];
    const u64 mass = (u64)A.range.size() * B.range.size(); mass_total += mass;
    const Q2JointPreparedBounds prepared(A.box, B.box);
    // 1. plafond : parcours préordre avec échappement (nœud 0 = racine, escape = prochain nœud hors sous-arbre).
    unsigned count = 0;
    std::size_t cursor = 0;
    while (cursor < nodes.size() && count < kmax) {
      const auto& z = nodes[cursor]; ++node_visits;
      const auto b = prepared.bounds(z.box);
      if (b.minimum4 > 0) { count = (unsigned)std::min<u64>(kmax, (u64)count + z.range.size()); cursor = z.escape; }
      else if (b.maximum4 <= 0) { cursor = z.escape; }
      else if (z.left == Q2SpatialNode::absent) { cursor = z.escape; }  // feuille singleton non universelle (minimum4 <= 0), rien à descendre
      else { cursor = z.left; }
    }
    rect_hist[count] += 1; mass_hist[count] += mass;
    // 2. chemin du front vers le milieu (parcouru par le front seulement si assez de sites extérieurs : même condition ici),
    //    certificat de bloc = plus haut nœud du chemin à borne conjointe > 0.
    const bool searchable = points.size() - A.range.size() - B.range.size() >= kmax;
    if (!searchable) { block_hist[0] += 1; return; }
    std::array<i64, 3> center4{};
    for (std::size_t axis = 0; axis < 3; ++axis) center4[axis] = (i64)A.box.low[axis] + A.box.high[axis] + B.box.low[axis] + B.box.high[axis];
    std::size_t node = 0; u64 block = 0;
    while (true) {
      if (block == 0 && prepared.bounds(nodes[node].box).minimum4 > 0) block = nodes[node].range.size();
      if (nodes[node].left == Q2SpatialNode::absent) break;
      ++path_steps;
      const auto left = nodes[node].left, right = nodes[node].right;
      node = midpoint_distance4(center4, nodes[left].box) <= midpoint_distance4(center4, nodes[right].box) ? left : right;
    }
    block_hist[block_bin(block)] += 1;
    if (block >= kmax) { ++block_rects; block_mass += mass; }
    // 3. fenêtres L autour du pivot (formule du front), sauts des rangs de A/B, arrêt à K crédits.
    const auto pivot = nodes[node].range.first;
    for (std::size_t w = 0; w < windows.size(); ++w) {
      const auto L = std::min<std::size_t>(windows[w], order.size());
      const auto first = std::min(pivot > L / 2 ? pivot - L / 2 : 0, order.size() - L);
      unsigned credits = 0;
      for (auto rank = first; rank < first + L && credits < kmax; ++rank) {
        if (contains(A.range, rank) || contains(B.range, rank)) continue;
        ++window_tests;
        if (prepared.bounds(singleton_box(points[order[rank]])).minimum4 > 0) ++credits;
      }
      if (credits >= kmax) { win_rects[w] += 1; win_mass[w] += mass; }
    }
  };
  // Masque 1 = voie q2 seule, comme le census intégré : front_emitted et front_residual_mass deviennent comparables à rects/mass.
  auto res = run_wspd_front(*index, kmax, s, fmode == "pure" ? WspdFrontMode::Pure : WspdFrontMode::MidpointSamples, consumer, 1);
  const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  const u64 rect_ge = rect_hist[kmax], mass_ge = mass_hist[kmax];
  auto pct = [](u64 a, u64 b) { return b ? (double)a / (double)b : 0.0; };
  std::printf("family=%s n=%zu kmax=%u s=%u front=%s rects=%llu mass=%llu front_emitted=%llu front_lane_q2_rectangles=%llu front_residual_mass=%llu front_rejected_products=%llu"
              " ceil_rects=%llu ceil_mass=%llu ceil_rect_share=%.4f ceil_mass_share=%.4f"
              " block_rects=%llu block_mass=%llu block_rect_share=%.4f block_mass_share=%.4f"
              " wink_rects=%llu win2k_rects=%llu win2k_mass=%llu win2k_rect_share=%.4f win2k_mass_share=%.4f"
              " win4k_rects=%llu win4k_mass=%llu win4k_rect_share=%.4f win4k_mass_share=%.4f"
              " node_visits=%llu path_steps=%llu window_tests=%llu ms=%.1f\n",
    fam.c_str(), n, kmax, s, fmode.c_str(), (unsigned long long)rects, (unsigned long long)mass_total,
    (unsigned long long)res.work.emitted_rectangles, (unsigned long long)res.work.lane_rectangles[0],
    (unsigned long long)res.work.residual_pair_mass[0], (unsigned long long)res.work.fully_rejected_products,
    (unsigned long long)rect_ge, (unsigned long long)mass_ge, pct(rect_ge, rects), pct(mass_ge, mass_total),
    (unsigned long long)block_rects, (unsigned long long)block_mass, pct(block_rects, rects), pct(block_mass, mass_total),
    (unsigned long long)win_rects[0], (unsigned long long)win_rects[1], (unsigned long long)win_mass[1], pct(win_rects[1], rects), pct(win_mass[1], mass_total),
    (unsigned long long)win_rects[2], (unsigned long long)win_mass[2], pct(win_rects[2], rects), pct(win_mass[2], mass_total),
    (unsigned long long)node_visits, (unsigned long long)path_steps, (unsigned long long)window_tests, ms);
  std::printf("  rect_hist:"); for (unsigned c = 0; c <= kmax; ++c) std::printf(" %u:%llu", c, (unsigned long long)rect_hist[c]); std::printf("\n");
  std::printf("  mass_hist:"); for (unsigned c = 0; c <= kmax; ++c) std::printf(" %u:%llu", c, (unsigned long long)mass_hist[c]); std::printf("\n");
  static const char* bins[7] = {"0", "1", "2-4", "5-9", "10-15", "16-31", "32+"};
  std::printf("  block_hist:"); for (unsigned c = 0; c < 7; ++c) std::printf(" %s:%llu", bins[c], (unsigned long long)block_hist[c]); std::printf("\n");
  return 0;
}
