// Auditeur B (15 sept. 2026) — bilan net de la « surproposition » (docs/P0_SURPROPOSITION_TEMOINS_Q2.md) mesuré sur une
// copie d'audit du moteur 2741d614 dont Front::filter est étendu (fenêtre L = factor·K autour du même pivot, deux
// intervalles disjoints, crédits conservés, politique B0). Le patch vit dans le dossier d'audit (front_filter.patch,
// apply_patch.sh), jamais dans l'arbre du constructeur ; factor = 1 reproduit le moteur épinglé, ce que le lanceur
// mesure en reliant ce même probe, compilé avec -DMHGP8_AUDIT_UNPATCHED, à la bibliothèque non patchée de 2741d614.
// Paramètres lus par la bibliothèque patchée : MHGP8_AUDIT_L_FACTOR (1, 2, 4) et MHGP8_AUDIT_B0 (0 = tous produits,
// 16 = petits facteurs) ; toute autre valeur est refusée.
// Sortie : compteurs du front (recherches, propositions, tests H, crédits, rejets, rectangles émis, extension), du
// census (candidats, admis, visites Z, avances de curseur, scissions), du Pool et de la collecte, temps mur du
// pipeline complet (front + census + collecte), temps du front seul (mesuré APRÈS le pipeline, masque 1,
// consommateur vide, et dont les compteurs doivent égaler ceux du front du pipeline : front_match), et un condensé
// canonique du flux de supports (ids ordonnés, somme et xor de condensés FNV par support ; témoin d'identité de flux
// modulo collision, pas un oracle).
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "pipeline/wspd_q2_census.hpp"
#include "front_fixtures.hpp"
using namespace mhgp8;
namespace {
u64 fnv(u64 h, u64 v) { for (int i = 0; i < 8; ++i) { h ^= (v >> (8 * i)) & 0xffU; h *= 1099511628211ULL; } return h; }
}  // namespace
int main(int argc, char** argv) {
  if (argc < 5) { std::fprintf(stderr, "usage: n family kmax s [seed=3] [pool_min_factor=64]\n"); return 2; }
  const std::size_t n = std::strtoull(argv[1], nullptr, 10); const std::string fam = argv[2];
  const unsigned kmax = std::atoi(argv[3]); const unsigned s = std::atoi(argv[4]);
  const unsigned seed = argc > 5 ? std::atoi(argv[5]) : 3; const std::size_t pool = argc > 6 ? std::strtoull(argv[6], nullptr, 10) : 64;
  const char* lf = std::getenv("MHGP8_AUDIT_L_FACTOR"); const char* b0 = std::getenv("MHGP8_AUDIT_B0");
  auto fx = bench::make_front_fixture(n, fam, seed);
  auto cloud = prepare_cloud(fx.points); auto index = make_q2_cloud_index(cloud);
  u64 supports = 0, interior_total = 0, shell_total = 0, digest_sum = 1469598103934665603ULL, digest_xor = 0;
  auto consumer = [&](const Q2Support& sp) {
    ++supports; interior_total += sp.interior.size(); shell_total += sp.shell.size();
    u64 h = 1469598103934665603ULL;
    h = fnv(h, std::min(sp.a_id, sp.b_id)); h = fnv(h, std::max(sp.a_id, sp.b_id)); h = fnv(h, sp.key.diameter_squared);
    for (auto c : sp.key.center_twice) h = fnv(h, c);
    u64 si = 0; for (auto id : sp.interior) si += fnv(h, id); h = fnv(h, si);
    u64 ss = 0; for (auto id : sp.shell) ss += fnv(h, id); h = fnv(h, ss);
    digest_sum += h; digest_xor ^= h;
  };
  const auto t0 = std::chrono::steady_clock::now();
  auto res = run_wspd_q2_census(*index, kmax, s, WspdFrontMode::MidpointSamples, Q2CensusMode::SharedBlocks, consumer,
                                Q2SiblingMode::Saturating, Q2WitnessOrder::ComplementFirst, Q2AnchorMode::Individual, pool);
  const double total_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  // Front seul, masque 1, consommateur vide, après le pipeline (caches chauds) : coût du front avec la fenêtre choisie.
  const auto f0 = std::chrono::steady_clock::now();
  auto front_only = run_wspd_front(*index, kmax, s, WspdFrontMode::MidpointSamples, [](const WspdRectangle&) {}, 1);
  const double front_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - f0).count();
  const bool front_match = front_only.work == res.front.work && front_only.total_unordered_pairs == res.front.total_unordered_pairs;
  const auto& fw = res.front.work; const auto& cw = res.census.work; const auto& pw = res.pool_work;
#ifdef MHGP8_AUDIT_UNPATCHED
  const u64 ext_products = 0, ext_proposals = 0, ext_rejections = 0; const char* build = "unpatched";
#else
  const u64 ext_products = fw.extended_products, ext_proposals = fw.extended_proposals, ext_rejections = fw.extended_rejections; const char* build = "patched";
#endif
  std::printf("family=%s n=%zu kmax=%u s=%u seed=%u pool=%zu build=%s l_factor=%s b0=%s front_match=%d"
              " front_searches=%llu front_descent_steps=%llu front_box_distance_tests=%llu front_proposals=%llu front_in_factors=%llu"
              " front_h_tests=%llu front_lane_credits=%llu front_rejected_products=%llu front_emitted=%llu front_residual_mass=%llu"
              " extended_products=%llu extended_proposals=%llu extended_rejections=%llu"
              " census_input_rectangles=%llu anchor_queries=%llu candidates=%llu accepted=%llu rejected=%llu"
              " count_node_visits=%llu count_bound_tests=%llu count_point_tests=%llu cursor_advances=%llu witness_splits=%llu"
              " payload_node_visits=%llu pool_filtered_pairs=%llu pool_selected_rectangles=%llu pool_preparation_ms=%.1f pool_selected_total_ms=%.1f"
              " supports=%llu interior_total=%llu shell_total=%llu digest_sum=%016llx digest_xor=%016llx"
              " count_ms=%.1f payload_ms=%.1f total_ms=%.1f front_only_ms=%.1f\n",
    fam.c_str(), n, kmax, s, seed, pool, build, lf ? lf : "1", b0 ? b0 : "0", front_match ? 1 : 0,
    (unsigned long long)fw.witness_searches, (unsigned long long)fw.witness_descent_steps, (unsigned long long)fw.witness_box_distance_tests,
    (unsigned long long)fw.proposed_sites, (unsigned long long)fw.proposals_in_factors, (unsigned long long)fw.h_bound_tests,
    (unsigned long long)fw.witness_lane_credits, (unsigned long long)fw.fully_rejected_products, (unsigned long long)fw.emitted_rectangles,
    (unsigned long long)fw.residual_pair_mass[0], (unsigned long long)ext_products, (unsigned long long)ext_proposals, (unsigned long long)ext_rejections,
    (unsigned long long)res.input_rectangles, (unsigned long long)res.anchor_queries, (unsigned long long)res.census.candidate_pairs,
    (unsigned long long)res.census.accepted_pairs, (unsigned long long)res.census.rejected_pairs, (unsigned long long)cw.count_node_visits,
    (unsigned long long)cw.count_bound_tests, (unsigned long long)cw.count_point_tests, (unsigned long long)cw.cursor_advances,
    (unsigned long long)cw.witness_splits, (unsigned long long)cw.payload_node_visits, (unsigned long long)pw.filtered_pairs,
    (unsigned long long)pw.selected_rectangles, pw.preparation_ms, pw.selected_total_ms,
    (unsigned long long)supports, (unsigned long long)interior_total, (unsigned long long)shell_total,
    (unsigned long long)digest_sum, (unsigned long long)digest_xor, res.census.count_ms, res.census.payload_ms, total_ms, front_ms);
  return front_match ? 0 : 3;
}
