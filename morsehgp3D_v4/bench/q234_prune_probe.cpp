// MorseHGP3D v4 — PROBE TROIS LANES : une seule vague, trois fuseaux.
//
// Chaque couple actif porte le MASQUE de ses lanes encore vivantes
// (q2/q3/q4). A chaque niveau, chaque lane vivante tente sa mort par la
// boule-cœur de son arite (h_q = smax-q+1 temoins universels) ; au niveau
// terminal (separe), l'autorite pleine (Hmin pour q2, 64 coins pour q3/q4)
// retente. Une lane tuee ne se rejoue jamais plus bas : les fuseaux etant
// emboîtes W_4 ⊂ W_3 ⊂ W_2, le masque est OBLIGATOIRE — sans lui, double
// credit et fausses morts (mutant v3 `dual-sans-masque`).
//
// INVARIANTS JUGES (par lane) :
//   L1  ledger exact : morte_q + vivante_q = C(n,2) − Σ C(μ_u,2) ;
//   L2  fail-open : juge ponctuel exact (in_spindle) sur echantillon de
//       chaque bloc mort — une paire vivante dans un bloc mort = code 1 ;
//   L3  plancher --min-dead-pct par lane (vert-par-vacuite).
// Codes : 0 conforme, 1 desaccord du juge, 2 refus, 3 invariant, 4 mutant.
#include <chrono>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/events/witness_count.hpp"
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
  u64 smax = 11;
  int judge_pairs_per_block = 4;
  double min_dead_pct = 0.0;
  bool inject_radius_ceil = false;
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
    else if (arg == "--inject=radius-ceil") a.inject_radius_ceil = true;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      a.family_ok = false;
    }
  }
  return a;
}

constexpr Lane kLanes[3] = {Lane::kQ2, Lane::kQ3, Lane::kQ4};

struct MaskedRect {
  NodeRef a, b;
  u8 mask;  // bit i (0..2) = lane q(2+i) vivante
};

struct DeadBlock {
  NodeRef a, b;
  int lane_ix;
};

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  const Args a = parse(argc, argv);
  if (!a.family_ok || a.n < 4 || a.s < 1 || a.smax < 5) {
    std::fprintf(stderr, "REFUS : arguments invalides\n");
    return 2;
  }
  const u64 h_of[3] = {lane_h(Lane::kQ2, a.smax), lane_h(Lane::kQ3, a.smax),
                       lane_h(Lane::kQ4, a.smax)};
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

  WitnessCountOpts cheap;
  cheap.use_descent = false;
  cheap.mutant_ceil_distance = a.inject_radius_ceil;
  WitnessCountOpts full;
  full.mutant_ceil_distance = a.inject_radius_ceil;

  u128 dead_mass[3] = {0, 0, 0}, alive_mass[3] = {0, 0, 0};
  u64 dead_inner[3] = {0, 0, 0}, dead_terminal[3] = {0, 0, 0};
  u64 alive_rects = 0;
  std::vector<DeadBlock> dead_blocks;

  if (!ix.nodes.empty()) {
    std::vector<MaskedRect> wave, next;
    for (const RadixNode& n : ix.nodes)
      wave.push_back(MaskedRect{n.left, n.right, 0b111});
    while (!wave.empty()) {
      next.clear();
      for (const MaskedRect& r : wave) {
        const u64 wa = detail::node_weight(ix, r.a);
        const u64 wb = detail::node_weight(ix, r.b);
        const u128 mass = (u128)wa * wb;
        u8 mask = r.mask;
        // 1. Mort bon marche par lane vivante (boule-cœur de l'arite).
        for (int li = 0; li < 3 && mask; ++li) {
          if (!(mask & (1u << li))) continue;
          if (count_universal_witnesses(kLanes[li], ix, r.a, r.b, h_of[li], cheap) >=
              h_of[li]) {
            mask &= (u8)~(1u << li);
            dead_mass[li] += mass;
            ++dead_inner[li];
            dead_blocks.push_back(DeadBlock{r.a, r.b, li});
          }
        }
        if (!mask) continue;  // les trois lanes sont mortes : plus de descente
        // 2. Terminal si separe ; autorite pleine par lane restante.
        i64 buf_a[3], buf_b[3];
        const auto va = detail::node_view(ix, r.a, buf_a);
        const auto vb = detail::node_view(ix, r.b, buf_b);
        if (detail::separated(va, vb, a.s, 1)) {
          bool any_alive = false;
          for (int li = 0; li < 3; ++li) {
            if (!(mask & (1u << li))) continue;
            if (count_universal_witnesses(kLanes[li], ix, r.a, r.b, h_of[li], full) >=
                h_of[li]) {
              dead_mass[li] += mass;
              ++dead_terminal[li];
              dead_blocks.push_back(DeadBlock{r.a, r.b, li});
            } else {
              alive_mass[li] += mass;
              any_alive = true;
            }
          }
          if (any_alive) ++alive_rects;
          continue;
        }
        // 3. Scission du facteur de plus grand diametre, masque herite.
        const i64 w2a = detail::box_w2(va);
        const i64 w2b = detail::box_w2(vb);
        const bool split_a = (r.a >= 0) && (r.b < 0 || w2a >= w2b);
        const NodeRef keep = split_a ? r.b : r.a;
        const RadixNode& n = ix.nodes[(size_t)(split_a ? r.a : r.b)];
        next.push_back(split_a ? MaskedRect{n.left, keep, mask}
                               : MaskedRect{keep, n.left, mask});
        next.push_back(split_a ? MaskedRect{n.right, keep, mask}
                               : MaskedRect{keep, n.right, mask});
      }
      wave.swap(next);
    }
  }
  const auto t2 = std::chrono::steady_clock::now();

  const u128 expected = expected_pair_mass(ix);
  bool mass_ok = true;
  for (int li = 0; li < 3; ++li)
    mass_ok = mass_ok && (dead_mass[li] + alive_mass[li] == expected);

  u64 judged = 0, false_deaths = 0;
  std::mt19937 rng(97u);
  for (const DeadBlock& db : dead_blocks) {
    const NodeRange ra = range_of(ix, db.a);
    const NodeRange rb = range_of(ix, db.b);
    std::vector<std::pair<i32, i32>> sample = {{ra.first, rb.first},
                                               {ra.last, rb.last}};
    std::uniform_int_distribution<i32> pa(ra.first, ra.last);
    std::uniform_int_distribution<i32> pb(rb.first, rb.last);
    for (int k = 2; k < a.judge_pairs_per_block; ++k)
      sample.push_back({pa(rng), pb(rng)});
    for (const auto& [uaI, ubI] : sample) {
      ++judged;
      if (true_spindle_count(kLanes[db.lane_ix], ix, uaI, ubI, h_of[db.lane_ix]) <
          h_of[db.lane_ix])
        ++false_deaths;
    }
  }
  const auto t3 = std::chrono::steady_clock::now();

  const double total_d = (double)(u64)expected;
  const auto pct = [&](u128 m) {
    return total_d > 0 ? 100.0 * (double)(u64)m / total_d : 0.0;
  };
  const auto ms = [](auto d) {
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(d).count() /
           1000.0;
  };
  std::printf(
      "famille=%s n=%d coord=%d s=%lld smax=%llu seed=%lld "
      "morte_q2_pct=%.2f morte_q3_pct=%.2f morte_q4_pct=%.2f "
      "morts_int=%llu/%llu/%llu morts_term=%llu/%llu/%llu rect_vivants=%llu "
      "masse=%s juge=%llu fausses_morts=%llu t_index_ms=%.1f t_descente_ms=%.1f "
      "t_juge_ms=%.1f\n",
      cloud_family_name(a.family), a.n, coord, (long long)a.s,
      (unsigned long long)a.smax, a.seed, pct(dead_mass[0]), pct(dead_mass[1]),
      pct(dead_mass[2]), (unsigned long long)dead_inner[0],
      (unsigned long long)dead_inner[1], (unsigned long long)dead_inner[2],
      (unsigned long long)dead_terminal[0], (unsigned long long)dead_terminal[1],
      (unsigned long long)dead_terminal[2], (unsigned long long)alive_rects,
      mass_ok ? "EXACTE" : "ECART", (unsigned long long)judged,
      (unsigned long long)false_deaths, ms(t1 - t0), ms(t2 - t1), ms(t3 - t2));

  if (false_deaths > 0) return a.inject_radius_ceil ? 4 : 1;
  if (a.inject_radius_ceil) {
    std::fprintf(stderr, "PORTE INEFFICACE : radius-ceil non discrimine ici\n");
    return 3;
  }
  if (!mass_ok) return 3;
  for (int li = 0; li < 3; ++li)
    if (pct(dead_mass[li]) < a.min_dead_pct) {
      std::fprintf(stderr, "PLANCHER : lane q%d %.2f %% < %.2f %%\n", 2 + li,
                   pct(dead_mass[li]), a.min_dead_pct);
      return 3;
    }
  return 0;
}
