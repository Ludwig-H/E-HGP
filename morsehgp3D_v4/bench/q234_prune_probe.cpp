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
  bool with_ha = false;
  bool true_alive = false;
  int ha_cap = 512;
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
    else if (arg == "--ha") a.with_ha = true;
    else if (arg == "--true-alive") a.true_alive = true;
    else if (const char* v = val("--ha-cap=")) a.ha_cap = std::atoi(v);
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

struct AliveRect {
  NodeRef a, b;
  u8 mask;
  u64 core[3];  // compte cœur (ecrete a h_q) par lane encore vivante
};

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  const Args a = parse(argc, argv);
  if (!a.family_ok || a.n < 4 || a.s < 1 || a.smax < 5) {
    std::fprintf(stderr, "REFUS : arguments invalides\n");
    return 2;
  }
  const int coord = a.coord > 0 ? a.coord : cloud_family_default_coord(a.family, a.n);
  const std::vector<P3> pts = make_family_cloud(a.family, a.n, coord, a.seed);
  const bool counter_family =
      a.family == CloudFamily::kTwoLines || a.family == CloudFamily::kCollinearSeven;
  if (!counter_family && (int)pts.size() != a.n) {
    std::fprintf(stderr, "REFUS : famille %s a rendu %zu points pour n=%d\n",
                 cloud_family_name(a.family), pts.size(), a.n);
    return 2;
  }

  // Seuils effectifs (audit 17 aout) : s_max = min(K_eff+1, n), K_eff = min(K_max, n).
  const u64 smax_eff = std::min<u64>(a.smax, pts.size());
  if (smax_eff < 5) {
    std::fprintf(stderr, "REFUS : s_max effectif %llu < 5\n",
                 (unsigned long long)smax_eff);
    return 2;
  }
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax_eff), lane_h(Lane::kQ3, smax_eff),
                       lane_h(Lane::kQ4, smax_eff)};
  const auto t0 = std::chrono::steady_clock::now();
  const CloudIndex ix = build_cloud_index(pts);
  // Contrat de l'audit du 17 aout : le profil exact travaille sur des sites
  // distincts ; les positions dupliquees sont refusees tant qu'un HGP
  // pondere n'est pas prouve.
  if ((size_t)ix.unique_count() != pts.size()) {
    std::fprintf(stderr, "REFUS unsupported_degeneracy : positions dupliquees\n");
    return 2;
  }
  const auto t1 = std::chrono::steady_clock::now();

  u128 dead_mass[3] = {0, 0, 0}, alive_mass[3] = {0, 0, 0};
  u64 dead_inner[3] = {0, 0, 0}, dead_terminal[3] = {0, 0, 0};
  u64 alive_rects = 0, alive_rects_lane[3] = {0, 0, 0};
  u64 nodes_visited = 0, corner_evals = 0;
  std::vector<DeadBlock> dead_blocks;
  std::vector<AliveRect> alive_list;

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
        // 1. Mort bon marche : UNE descente fusionnee (Hmin q2 + boules
        // q3/q4, sans autorite de feuille), trois compteurs.
        {
          const FusedCounts fc = count_universal_witnesses_234(
              ix, r.a, r.b, h_of, mask, /*with_corners=*/false,
              a.inject_radius_ceil);
          nodes_visited += fc.nodes_visited;
          for (int li = 0; li < 3; ++li) {
            if (!(mask & (1u << li)) || fc.c[li] < h_of[li]) continue;
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
          AliveRect ar{r.a, r.b, 0, {0, 0, 0}};
          const FusedCounts fc = count_universal_witnesses_234(
              ix, r.a, r.b, h_of, mask, /*with_corners=*/true,
              a.inject_radius_ceil);
          nodes_visited += fc.nodes_visited;
          corner_evals += fc.corner_evals;
          for (int li = 0; li < 3; ++li) {
            if (!(mask & (1u << li))) continue;
            if (fc.c[li] >= h_of[li]) {
              dead_mass[li] += mass;
              ++dead_terminal[li];
              dead_blocks.push_back(DeadBlock{r.a, r.b, li});
            } else {
              alive_mass[li] += mass;
              ++alive_rects_lane[li];
              ar.mask |= (u8)(1u << li);
              ar.core[li] = fc.c[li];
            }
          }
          if (ar.mask) {
            ++alive_rects;
            alive_list.push_back(ar);
          }
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

  // Phase h_a/h_b (--ha) : sur chaque rectangle vivant, comptes par
  // extremite via l'autorite 8 coins, decision par HISTOGRAMME de h_b ecrete
  // a need = h_q - h_coeur — jamais une paire materialisee (theoreme de
  // disjonction : cœur hors A∪B, h_a dans A, h_b dans B). Un rectangle plus
  // large que --ha-cap est passe (toutes ancres comptees vivantes,
  // conservateur, compte publie).
  u128 anchors_alive[3] = {0, 0, 0};
  u128 anchors_total[3] = {0, 0, 0};
  u64 ha_evals = 0, ha_skipped = 0, judge_anchors = 0, false_kills = 0;
  if (a.with_ha) {
    for (const AliveRect& ar : alive_list) {
      const NodeRange ra = range_of(ix, ar.a);
      const NodeRange rb = range_of(ix, ar.b);
      const AxisBox boxA = box_of_node(ix, ar.a);
      const AxisBox boxB = box_of_node(ix, ar.b);
      const int na = ra.last - ra.first + 1;
      const int nb = rb.last - rb.first + 1;
      const u128 rect_mass =
          (u128)ix.range_weight(ra.first, ra.last) * ix.range_weight(rb.first, rb.last);
      for (int li = 0; li < 3; ++li) {
        if (!(ar.mask & (1u << li))) continue;
        anchors_total[li] += rect_mass;
        if (na > a.ha_cap || nb > a.ha_cap) {
          ++ha_skipped;
          anchors_alive[li] += rect_mass;
          continue;
        }
        const Lane q = kLanes[li];
        const u64 need = h_of[li] - ar.core[li];  // >= 1 sur un rect vivant
        std::vector<u64> ha((size_t)na, 0), hb((size_t)nb, 0);
        for (int ia = 0; ia < na; ++ia)
          for (int iz = 0; iz < na; ++iz) {
            if (iz == ia) continue;
            ++ha_evals;
            if (universal_over_corners(q, ix.upos[(size_t)(ra.first + ia)], boxB,
                                       ix.upos[(size_t)(ra.first + iz)]))
              ha[(size_t)ia] += ix.range_weight(ra.first + iz, ra.first + iz);
          }
        for (int ib = 0; ib < nb; ++ib)
          for (int iz = 0; iz < nb; ++iz) {
            if (iz == ib) continue;
            ++ha_evals;
            // W_q est symetrique en (a,b) : H et Xi sont invariants par
            // echange des extremites.
            if (universal_over_corners(q, ix.upos[(size_t)(rb.first + ib)], boxA,
                                       ix.upos[(size_t)(rb.first + iz)]))
              hb[(size_t)ib] += ix.range_weight(rb.first + iz, rb.first + iz);
          }
        // Histogramme de h_b ecrete, pondere ; cum[t] = poids des b : h_b < t.
        std::vector<u64> hist(need + 1, 0);
        for (int ib = 0; ib < nb; ++ib)
          hist[(size_t)std::min<u64>(hb[(size_t)ib], need)] +=
              ix.range_weight(rb.first + ib, rb.first + ib);
        std::vector<u64> cum(need + 2, 0);
        for (u64 t = 1; t <= need + 1; ++t) cum[t] = cum[t - 1] + hist[t - 1];
        for (int ia = 0; ia < na; ++ia) {
          const u64 wa2 = ix.range_weight(ra.first + ia, ra.first + ia);
          const u64 hav = std::min<u64>(ha[(size_t)ia], need);
          anchors_alive[li] += (u128)wa2 * cum[need - hav];
        }
        // Juge fail-open : l'ancre (argmax h_a, argmax h_b), si tuee par la
        // somme, doit avoir un vrai compte >= h_q.
        int best_a = 0, best_b = 0;
        for (int ia = 1; ia < na; ++ia)
          if (ha[(size_t)ia] > ha[(size_t)best_a]) best_a = ia;
        for (int ib = 1; ib < nb; ++ib)
          if (hb[(size_t)ib] > hb[(size_t)best_b]) best_b = ib;
        if (ar.core[li] + ha[(size_t)best_a] + hb[(size_t)best_b] >= h_of[li] &&
            judge_anchors < 5000) {
          ++judge_anchors;
          if (true_spindle_count(q, ix, ra.first + best_a, rb.first + best_b,
                                 h_of[li]) < h_of[li])
            ++false_kills;
        }
      }
    }
  }
  const auto t2b = std::chrono::steady_clock::now();

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
  // Oracle borne (--true-alive) : vrai vivant exhaustif par lane, confronte
  // aux constantes de Poisson de l'audit (E[V_h]/n -> 2*pi*h/(3*v_q), soit
  // ~40 / 123,8 / 139,1 paires vivantes par point en regime homogene).
  if (a.true_alive) {
    u128 tv[3] = {0, 0, 0};
    const int m = ix.unique_count();
    for (i32 ua = 0; ua < m; ++ua)
      for (i32 ub = ua + 1; ub < m; ++ub)
        for (int li = 0; li < 3; ++li)
          if (true_spindle_count(kLanes[li], ix, ua, ub, h_of[li]) < h_of[li])
            tv[li] += 1;
    std::printf(
        "  vrai_vivant: q2=%llu (%.1f/pt, poisson 40,0) q3=%llu (%.1f/pt, "
        "poisson 123,8) q4=%llu (%.1f/pt, poisson 139,1)\n",
        (unsigned long long)(u64)tv[0], (double)(u64)tv[0] / (double)m,
        (unsigned long long)(u64)tv[1], (double)(u64)tv[1] / (double)m,
        (unsigned long long)(u64)tv[2], (double)(u64)tv[2] / (double)m);
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
      "morts_int=%llu/%llu/%llu morts_term=%llu/%llu/%llu rect_vivants=%llu rect_vivants_lane=%llu/%llu/%llu noeuds_temoins=%llu coins=%llu "
      "masse=%s juge=%llu fausses_morts=%llu t_index_ms=%.1f t_descente_ms=%.1f "
      "t_juge_ms=%.1f\n",
      cloud_family_name(a.family), a.n, coord, (long long)a.s,
      (unsigned long long)a.smax, a.seed, pct(dead_mass[0]), pct(dead_mass[1]),
      pct(dead_mass[2]), (unsigned long long)dead_inner[0],
      (unsigned long long)dead_inner[1], (unsigned long long)dead_inner[2],
      (unsigned long long)dead_terminal[0], (unsigned long long)dead_terminal[1],
      (unsigned long long)dead_terminal[2], (unsigned long long)alive_rects,
      (unsigned long long)alive_rects_lane[0], (unsigned long long)alive_rects_lane[1],
      (unsigned long long)alive_rects_lane[2], (unsigned long long)nodes_visited,
      (unsigned long long)corner_evals, mass_ok ? "EXACTE" : "ECART", (unsigned long long)judged,
      (unsigned long long)false_deaths, ms(t1 - t0), ms(t2 - t1), ms(t3 - t2b));
  if (a.with_ha) {
    const auto ppct = [&](int li) {
      return anchors_total[li] > 0
                 ? 100.0 * (double)(u64)anchors_alive[li] / (double)(u64)anchors_total[li]
                 : 0.0;
    };
    std::printf(
        "  ha: ancres_vivantes_q2=%.0f (%.1f%%) q3=%.0f (%.1f%%) q4=%.0f (%.1f%%) "
        "par_point_q4=%.1f evals=%llu rect_passes=%llu juge_ancres=%llu "
        "fausses_tueries=%llu t_ha_ms=%.1f\n",
        (double)(u64)anchors_alive[0], ppct(0), (double)(u64)anchors_alive[1], ppct(1),
        (double)(u64)anchors_alive[2], ppct(2),
        (double)(u64)anchors_alive[2] / (double)pts.size(),
        (unsigned long long)ha_evals, (unsigned long long)ha_skipped,
        (unsigned long long)judge_anchors, (unsigned long long)false_kills,
        ms(t2b - t2));
    if (false_kills > 0) return 1;
  }

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
