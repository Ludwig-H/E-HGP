// MorseHGP3D v4 — PROBE DE DESCENTE TERNAIRE q2 : la WSPD qui tue.
//
// Réponse au constat final de la v3 (« le WSPD est correct et AVEUGLE A LA
// SORTIE ») : le certificat de mort q2 est évalué PENDANT la récursion.
// Chaque couple actif de nœuds devient :
//
//   MORT      h_2 témoins universels comptés dans le cœur (boule-cœur à tout
//             niveau ; boule + descente Hmin au niveau terminal) — le couple
//             ne descend plus, sa masse est comptée morte ;
//   TERMINAL  séparé et non fermé — une ancre-rectangle à instruire ;
//   SCINDÉ    sinon (facteur de plus grand diamètre).
//
// INVARIANTS JUGES :
//   L1  ledger exact : masse_morte + masse_vivante = C(n,2) − Σ C(μ_u,2) ;
//   L2  fail-open : le juge ponctuel (équivalence exacte
//       `r_{h_2}(m) <= ‖ab‖/2`) échantillonne chaque bloc mort — une paire
//       vraiment vivante dans un bloc mort est un désaccord (code 1) ;
//   L3  plancher : au moins `--min-dead-pct` pourcent de la masse fermée
//       (contre le vert-par-vacuité du certificat).
//
// À petit n (`--true-alive`), le vrai vivant est calculé exhaustivement par
// le juge (oracle borné) et le probe publie la fermeture et le mou. Codes :
// 0 conforme, 1 désaccord du juge, 2 refus, 3 invariant/plancher,
// 4 mutant tué (--inject=radius-ceil).
#include <chrono>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/events/q2_witness_count.hpp"
#include "../src/wspd/wavefront.hpp"

namespace {

using namespace mhgp4;

struct Args {
  CloudFamily family = CloudFamily::kUniform;
  bool family_ok = true;
  int n = 8000;
  int coord = 0;
  long long seed = 3;
  i64 s = 8;
  u64 smax = 11;  // h_2 = smax - 1 (K_max = smax - 1)
  int judge_pairs_per_block = 8;
  bool true_alive = false;
  double min_dead_pct = 0.0;
  bool inject_radius_ceil = false;
  bool kill_descent = true;  // --kill=terminal : mesure appariée sans les
                             // tentatives internes (discipline v3 : tout gain
                             // se mesure contre une exécution désarmée)
};

bool parse_family(const char* name, CloudFamily* out) {
  const CloudFamily all[] = {CloudFamily::kUniform,
                             CloudFamily::kTerrain,
                             CloudFamily::kScanlineSinglePass,
                             CloudFamily::kScanlineOverlapMultiecho,
                             CloudFamily::kEightClusters,
                             CloudFamily::kTwoLines,
                             CloudFamily::kCollinearSeven};
  for (const CloudFamily f : all)
    if (std::strcmp(name, cloud_family_name(f)) == 0) {
      *out = f;
      return true;
    }
  return false;
}

Args parse(int argc, char** argv) {
  Args a;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto val = [&](const char* prefix) -> const char* {
      const size_t l = std::strlen(prefix);
      return arg.compare(0, l, prefix) == 0 ? arg.c_str() + l : nullptr;
    };
    if (const char* v = val("--family=")) a.family_ok = parse_family(v, &a.family);
    else if (const char* v = val("--n=")) a.n = std::atoi(v);
    else if (const char* v = val("--coord=")) a.coord = std::atoi(v);
    else if (const char* v = val("--seed=")) a.seed = std::atoll(v);
    else if (const char* v = val("--s=")) a.s = std::atoll(v);
    else if (const char* v = val("--smax=")) a.smax = (u64)std::atoll(v);
    else if (const char* v = val("--judge-pairs=")) a.judge_pairs_per_block = std::atoi(v);
    else if (const char* v = val("--min-dead-pct=")) a.min_dead_pct = std::atof(v);
    else if (arg == "--true-alive") a.true_alive = true;
    else if (arg == "--kill=terminal") a.kill_descent = false;
    else if (arg == "--kill=descente") a.kill_descent = true;
    else if (arg == "--inject=radius-ceil") a.inject_radius_ceil = true;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      a.family_ok = false;
    }
  }
  return a;
}

struct DeadBlock {
  NodeRef a, b;
};

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  const Args a = parse(argc, argv);
  if (!a.family_ok || a.n < 4 || a.s < 1 || a.smax < 3) {
    std::fprintf(stderr, "REFUS : arguments invalides\n");
    return 2;
  }
  const u64 h2 = a.smax - 1;
  const int coord = a.coord > 0 ? a.coord : cloud_family_default_coord(a.family, a.n);
  const std::vector<P3> pts = make_family_cloud(a.family, a.n, coord, a.seed);
  const bool counter_family =
      a.family == CloudFamily::kTwoLines || a.family == CloudFamily::kCollinearSeven;
  if (!counter_family && (int)pts.size() != a.n) {
    std::fprintf(stderr, "REFUS : famille %s a rendu %zu points pour n=%d\n",
                 cloud_family_name(a.family), pts.size(), a.n);
    return 2;
  }

  const auto t0 = std::chrono::steady_clock::now();
  const CloudIndex ix = build_cloud_index(pts);
  const auto t1 = std::chrono::steady_clock::now();

  Q2CountOpts cheap;   // tentative de mort a tout niveau : boule seulement
  cheap.use_hmin = false;
  cheap.mutant_ceil_distance = a.inject_radius_ceil;
  Q2CountOpts full;    // au niveau terminal : boule puis descente Hmin
  full.mutant_ceil_distance = a.inject_radius_ceil;

  u128 dead_mass = 0, alive_mass = 0;
  u64 dead_blocks_inner = 0, dead_blocks_terminal = 0, alive_rects = 0;
  u64 kill_attempts = 0;
  std::vector<DeadBlock> dead_blocks;

  if (!ix.nodes.empty()) {
    std::vector<WspdRect> wave, next;
    for (const RadixNode& n : ix.nodes) wave.push_back(WspdRect{n.left, n.right});
    while (!wave.empty()) {
      next.clear();
      for (const WspdRect& r : wave) {
        const u64 wa = detail::node_weight(ix, r.a);
        const u64 wb = detail::node_weight(ix, r.b);
        // 1. Tentative de mort bon marche (boule-cœur), a tout niveau.
        if (a.kill_descent) {
        ++kill_attempts;
        if (count_universal_witnesses_q2(ix, r.a, r.b, h2, cheap) >= h2) {
          dead_mass += (u128)wa * wb;
          ++dead_blocks_inner;
          dead_blocks.push_back(DeadBlock{r.a, r.b});
          continue;
        }
        }
        // 2. Terminal si separe ; l'autorite Hmin y retente la mort.
        i64 buf_a[3], buf_b[3];
        const auto va = detail::node_view(ix, r.a, buf_a);
        const auto vb = detail::node_view(ix, r.b, buf_b);
        if (detail::separated(va, vb, a.s, 1)) {
          if (count_universal_witnesses_q2(ix, r.a, r.b, h2, full) >= h2) {
            dead_mass += (u128)wa * wb;
            ++dead_blocks_terminal;
            dead_blocks.push_back(DeadBlock{r.a, r.b});
          } else {
            alive_mass += (u128)wa * wb;
            ++alive_rects;
          }
          continue;
        }
        // 3. Scission du facteur de plus grand diametre.
        const i64 w2a = detail::box_w2(va);
        const i64 w2b = detail::box_w2(vb);
        const bool split_a = (r.a >= 0) && (r.b < 0 || w2a >= w2b);
        const NodeRef keep = split_a ? r.b : r.a;
        const RadixNode& n = ix.nodes[(size_t)(split_a ? r.a : r.b)];
        next.push_back(split_a ? WspdRect{n.left, keep} : WspdRect{keep, n.left});
        next.push_back(split_a ? WspdRect{n.right, keep} : WspdRect{keep, n.right});
      }
      wave.swap(next);
    }
  }
  const auto t2 = std::chrono::steady_clock::now();

  // L1 — ledger exact.
  const u128 expected = expected_pair_mass(ix);
  const bool mass_ok = dead_mass + alive_mass == expected;

  // L2 — juge fail-open : echantillon de paires de chaque bloc mort.
  u64 judged = 0, false_deaths = 0;
  std::mt19937 rng(97u);
  for (const DeadBlock& db : dead_blocks) {
    const NodeRange ra = range_of(ix, db.a);
    const NodeRange rb = range_of(ix, db.b);
    std::vector<std::pair<i32, i32>> sample = {{ra.first, rb.first},
                                               {ra.first, rb.last},
                                               {ra.last, rb.first},
                                               {ra.last, rb.last}};
    std::uniform_int_distribution<i32> pa(ra.first, ra.last);
    std::uniform_int_distribution<i32> pb(rb.first, rb.last);
    for (int k = 4; k < a.judge_pairs_per_block; ++k)
      sample.push_back({pa(rng), pb(rng)});
    for (const auto& [uaI, ubI] : sample) {
      ++judged;
      if (true_interior_count_q2(ix, uaI, ubI, h2) < h2) ++false_deaths;
    }
  }

  // Vrai vivant exhaustif (oracle borne, petits n seulement).
  u128 true_alive_mass = 0;
  if (a.true_alive) {
    const int m = ix.unique_count();
    for (i32 ua = 0; ua < m; ++ua)
      for (i32 ub = ua + 1; ub < m; ++ub)
        if (true_interior_count_q2(ix, ua, ub, h2) < h2)
          true_alive_mass +=
              (u128)ix.range_weight(ua, ua) * ix.range_weight(ub, ub);
  }
  const auto t3 = std::chrono::steady_clock::now();

  const double total_d = (double)(u64)expected;
  const double dead_pct = total_d > 0 ? 100.0 * (double)(u64)dead_mass / total_d : 0.0;
  const auto ms = [](auto d) {
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(d).count() /
           1000.0;
  };
  std::printf(
      "famille=%s n=%d coord=%d s=%lld smax=%llu seed=%lld uniques=%d "
      "masse_morte_pct=%.2f blocs_morts_internes=%llu blocs_morts_terminaux=%llu "
      "rect_vivants=%llu masse=%s juge_paires=%llu fausses_morts=%llu",
      cloud_family_name(a.family), a.n, coord, (long long)a.s,
      (unsigned long long)a.smax, a.seed, ix.unique_count(), dead_pct,
      (unsigned long long)dead_blocks_inner, (unsigned long long)dead_blocks_terminal,
      (unsigned long long)alive_rects, mass_ok ? "EXACTE" : "ECART",
      (unsigned long long)judged, (unsigned long long)false_deaths);
  if (a.true_alive) {
    const double t_alive = (double)(u64)true_alive_mass;
    const double p_alive = (double)(u64)(expected - dead_mass);
    std::printf(" vrai_vivant=%.0f vivant_probe=%.0f mou=%.3f fermeture_pct=%.2f",
                t_alive, p_alive, t_alive > 0 ? p_alive / t_alive : 0.0,
                total_d - t_alive > 0
                    ? 100.0 * (double)(u64)dead_mass / (total_d - t_alive)
                    : 0.0);
  }
  std::printf(" t_index_ms=%.1f t_descente_ms=%.1f t_juge_ms=%.1f\n", ms(t1 - t0),
              ms(t2 - t1), ms(t3 - t2));

  if (false_deaths > 0) return a.inject_radius_ceil ? 4 : 1;
  if (a.inject_radius_ceil) {
    std::fprintf(stderr, "PORTE INEFFICACE : radius-ceil non discrimine ici\n");
    return 3;
  }
  if (!mass_ok) return 3;
  if (dead_pct < a.min_dead_pct) {
    std::fprintf(stderr, "PLANCHER : %.2f %% fermes < %.2f %% exiges\n", dead_pct,
                 a.min_dead_pct);
    return 3;
  }
  return 0;
}
