// MorseHGP3D v4 — PROBE DE FORET SUR FLUX REELS : les trois lanes WSPD
// alimentent le SpherePlateau d'echelle puis le fold a macro-lots.
//
// Chaine : WSPD (q2/q3/q4, generateurs seulement) -> sort/RLE par BallKey
// primitive inter-lanes -> UN census exact par cle (forme (A,B,C) uniforme,
// descente d'arbre separable par axe), collectant I_B ET U_B COMPLETS ->
// expansion des plateaux (§ 5.3bis, roles § 5.2) -> ForestEvents par K ->
// build_forest (macro-lots same_exact_level).
//
// JUGE (--judge, borne) : la MEME semantique depuis une enumeration brute
// aux predicats de production (toutes paires / triangles aigus / tetraedres
// centres) avec census brut point a point — il juge la COMPLETUDE WSPD, le
// census d'arbre et le RLE. Compare par K : nombre d'evenements, lots,
// multiensemble des nœuds, attachements nes au lot, partitions apres
// chaque lot.
// Codes : 0 conforme, 1 desaccord, 2 refus (dont resource_exhausted),
// 3 invariant/plancher, 4 mutant tue (--inject=rle-drop |
// census-nonstrict).
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/forest/forest.hpp"
#include "../src/forest/sphere_plateau.hpp"
#include "../src/pipeline/ball_stream.hpp"

namespace {

using namespace mhgp4;

struct Args {
  CloudFamily family = CloudFamily::kUniform;
  bool family_ok = true;
  int n = 120;
  int coord = 0;
  long long seed = 3;
  i64 s = 8;
  u64 smax = 11;
  size_t shell_cap = 12;
  bool judge = false;
  bool inj_rle_drop = false;
  bool inj_census_nonstrict = false;
  u64 min_balls = 0;
  u64 min_fusions = 0;
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
    else if (const char* v = val("--shell-cap=")) a.shell_cap = (size_t)std::atoll(v);
    else if (const char* v = val("--min-balls=")) a.min_balls = (u64)std::atoll(v);
    else if (const char* v = val("--min-fusions=")) a.min_fusions = (u64)std::atoll(v);
    else if (arg == "--judge") a.judge = true;
    else if (arg == "--inject=rle-drop") a.inj_rle_drop = true;
    else if (arg == "--inject=census-nonstrict") a.inj_census_nonstrict = true;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      a.family_ok = false;
    }
  }
  return a;
}

// Foret par K depuis une liste de boules censusees : expansion + fold.
// Retourne 0 ou un code de sortie. `per_k_events` compte les evenements.
struct BallData {
  Q3BallKey key;
  Q4Level level;
  std::vector<i32> interior, shell;
};

int forests_from_balls(const std::vector<BallData>& balls,
                       const std::vector<P3>& pos, u64 per_k_events[11],
                       ForestResult per_k_result[11]) {
  std::vector<std::vector<ForestEvent>> ev_k(11);
  std::vector<PlateauEvent> pevents;
  for (const BallData& b : balls) {
    const BallRat c = ball_center(b.key);
    pevents.clear();
    expand_plateau(c, pos, b.interior, b.shell, 11, &pevents);
    for (const PlateauEvent& pe : pevents) {
      const size_t K = pe.tpart.size() + pe.ipart.size() - 1;
      if (K < 1 || K > 10) continue;
      ForestEvent ev;
      ev.q = (u8)pe.tpart.size();
      ev.d = (u8)pe.ipart.size();
      ev.active_mask = pe.active_mask;
      for (size_t t = 0; t < pe.tpart.size(); ++t)
        ev.support[t] = (PointId)pe.tpart[t];
      for (size_t t = 0; t < pe.ipart.size(); ++t)
        ev.interior[t] = (PointId)pe.ipart[t];
      ev.level = b.level;
      ev_k[K].push_back(ev);
    }
  }
  for (int K = 1; K <= 10; ++K) {
    per_k_events[K] = ev_k[(size_t)K].size();
    per_k_result[K] = build_forest(ev_k[(size_t)K]);
    if (per_k_result[K].attach_violations != 0 ||
        per_k_result[K].birth_violations != 0) {
      std::fprintf(stderr, "INVARIANT : violations de roles (K=%d)\n", K);
      return 3;
    }
  }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  const Args a = parse(argc, argv);
  if (!a.family_ok || a.n < 3 || a.s < 1) {
    std::fprintf(stderr, "REFUS : arguments invalides\n");
    return 2;
  }
  if (a.smax > 11) {
    std::fprintf(stderr, "REFUS : profil K_max<=10 (smax<=11)\n");
    return 2;
  }
  const std::vector<P3> pts = make_family_cloud(
      a.family, a.n,
      a.coord > 0 ? a.coord : cloud_family_default_coord(a.family, a.n), a.seed);
  const u64 smax_eff = std::min<u64>(a.smax, pts.size());
  if (smax_eff < 5) {
    std::fprintf(stderr, "REFUS : s_max effectif trop petit\n");
    return 2;
  }
  const auto t0 = std::chrono::steady_clock::now();
  const CloudIndex ix = build_cloud_index(pts);
  if ((size_t)ix.unique_count() != pts.size()) {
    std::fprintf(stderr, "REFUS unsupported_degeneracy : positions dupliquees\n");
    return 2;
  }

  // 1. Generateurs WSPD -> RLE par BallKey (arite minimale d'abord).
  std::vector<BallCandidate> cands;
  BallStreamStats st;
  collect_candidate_balls(ix, a.s, smax_eff, &cands, &st);
  std::stable_sort(cands.begin(), cands.end(), ball_candidate_less);
  if (!a.inj_rle_drop)  // MUTANT : dedupe saute, boules re-censusees
    cands.erase(std::unique(cands.begin(), cands.end(),
                            [](const BallCandidate& x, const BallCandidate& y) {
                              return x.key == y.key;
                            }),
                cands.end());
  st.unique_balls = cands.size();
  const auto t1 = std::chrono::steady_clock::now();

  // 2. UN census exact par cle (I_B et U_B complets).
  std::vector<BallData> balls;
  balls.reserve(cands.size());
  for (const BallCandidate& bc : cands) {
    BallData b;
    b.key = bc.key;
    b.level = bc.level;
    bool overflow = false;
    if (!ball_census(ix, bc.key, 9, a.shell_cap, &b.interior, &b.shell,
                     &overflow)) {
      if (overflow) {
        std::fprintf(stderr,
                     "REFUS resource_exhausted : coquille > %zu (plafond "
                     "explicite, jamais de troncature)\n",
                     a.shell_cap);
        return 2;
      }
      ++st.balls_dead_depth;  // |I_B| > 9 : aucun K <= 10
      continue;
    }
    if (a.inj_census_nonstrict) {
      // MUTANT : la coquille comptee interieure (P <= 0).
      for (const i32 u : b.shell) b.interior.push_back(u);
      b.shell.clear();
    }
    st.census_interior += b.interior.size();
    st.census_shell += b.shell.size();
    balls.push_back(std::move(b));
  }
  const auto t2 = std::chrono::steady_clock::now();

  // 3. Expansion + fold par K.
  u64 sev[11] = {};
  ForestResult sres[11];
  {
    const int rc = forests_from_balls(balls, ix.upos, sev, sres);
    if (rc) return rc;
  }
  const auto t3 = std::chrono::steady_clock::now();

  // 4. JUGE : meme semantique depuis l'enumeration brute aux predicats de
  // production, census brut point a point (juge la completude WSPD, le
  // census d'arbre et le RLE).
  u64 disagreements = 0;
  if (a.judge) {
    const int m = ix.unique_count();
    std::vector<BallCandidate> jcands;
    for (i32 i = 0; i < m; ++i)
      for (i32 j = i + 1; j < m; ++j) {
        const P3 &pa = ix.upos[(size_t)i], &pb = ix.upos[(size_t)j];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        jcands.push_back(BallCandidate{q2_ball_key(pa, pb),
                                       promote_q3_level(q2_exact_level(D2)), 2});
      }
    for (i32 i = 0; i < m; ++i)
      for (i32 j = i + 1; j < m; ++j)
        for (i32 k = j + 1; k < m; ++k) {
          const i32 vs[3] = {i, j, k};
          int bu = 0, bv = 1;
          i64 bl2 = -1;
          for (int s0 = 0; s0 < 3; ++s0)
            for (int s1 = s0 + 1; s1 < 3; ++s1) {
              const i64 l2 = p3_norm2(
                  p3_sub(ix.upos[(size_t)vs[s1]], ix.upos[(size_t)vs[s0]]));
              if (l2 > bl2) bl2 = l2, bu = s0, bv = s1;
            }
          const i32 ia = vs[bu], ib = vs[bv];
          i32 ixx = -1;
          for (const i32 u : vs)
            if (u != ia && u != ib) ixx = u;
          const P3 &pa = ix.upos[(size_t)ia], &pb = ix.upos[(size_t)ib],
                   &px = ix.upos[(size_t)ixx];
          const P3 vv{2 * px.x - pa.x - pb.x, 2 * px.y - pa.y - pb.y,
                      2 * px.z - pa.z - pb.z};
          if (p3_norm2(vv) <= bl2) continue;
          const Q3Form f3 = q3_form(pa, pb, px);
          jcands.push_back(BallCandidate{
              q3_ball_key(f3), promote_q3_level(q3_exact_level(pa, pb, px)), 3});
        }
    const auto compact = [&]() {
      std::stable_sort(jcands.begin(), jcands.end(), ball_candidate_less);
      jcands.erase(std::unique(jcands.begin(), jcands.end(),
                               [](const BallCandidate& x, const BallCandidate& y) {
                                 return x.key == y.key;
                               }),
                   jcands.end());
    };
    for (i32 i = 0; i < m; ++i)
      for (i32 j = i + 1; j < m; ++j)
        for (i32 k = j + 1; k < m; ++k)
          for (i32 l = k + 1; l < m; ++l) {
            if (jcands.size() > 2000000) compact();
            const i32 vs[4] = {i, j, k, l};
            const Q4Form f4 = q4_form(ix.upos[(size_t)vs[0]], ix.upos[(size_t)vs[1]],
                                      ix.upos[(size_t)vs[2]], ix.upos[(size_t)vs[3]]);
            if (f4.det == 0) continue;
            if (!q4_center_strictly_inside(f4, ix.upos[(size_t)vs[0]],
                                           ix.upos[(size_t)vs[1]],
                                           ix.upos[(size_t)vs[2]],
                                           ix.upos[(size_t)vs[3]]))
              continue;
            jcands.push_back(BallCandidate{q3_ball_key_reduce(q4_ball_form(f4)),
                                           q4_level_raw(f4), 4});
          }
    std::stable_sort(jcands.begin(), jcands.end(), ball_candidate_less);
    jcands.erase(std::unique(jcands.begin(), jcands.end(),
                             [](const BallCandidate& x, const BallCandidate& y) {
                               return x.key == y.key;
                             }),
                 jcands.end());
    std::vector<BallData> jballs;
    for (const BallCandidate& bc : jcands) {
      BallData b;
      b.key = bc.key;
      b.level = bc.level;
      bool dead = false, over = false;
      for (i32 u = 0; u < m && !dead && !over; ++u) {
        const P3& p = ix.upos[(size_t)u];
        const i128 pw = bc.key.a * p3_norm2(p) + bc.key.b[0] * p.x +
                        bc.key.b[1] * p.y + bc.key.b[2] * p.z + bc.key.c;
        if (pw < 0) {
          b.interior.push_back(u);
          if (b.interior.size() > 9) dead = true;
        } else if (pw == 0) {
          b.shell.push_back(u);
          if (b.shell.size() > a.shell_cap) over = true;
        }
      }
      if (over) {
        std::fprintf(stderr, "REFUS resource_exhausted (juge)\n");
        return 2;
      }
      if (dead) continue;
      jballs.push_back(std::move(b));
    }
    u64 jev[11] = {};
    ForestResult jres[11];
    {
      const int rc = forests_from_balls(jballs, ix.upos, jev, jres);
      if (rc) return rc;
    }
    for (int K = 1; K <= 10; ++K) {
      const bool same_nodes = [&] {
        std::vector<std::pair<u64, u64>> sn, jn;
        for (const ForestNode& nd : sres[K].nodes) sn.push_back({nd.batch, nd.absorbed});
        for (const ForestNode& nd : jres[K].nodes) jn.push_back({nd.batch, nd.absorbed});
        std::sort(sn.begin(), sn.end());
        std::sort(jn.begin(), jn.end());
        return sn == jn;
      }();
      if (sev[K] != jev[K] || sres[K].batches != jres[K].batches ||
          !same_nodes || sres[K].new_attachments != jres[K].new_attachments ||
          sres[K].final_partition != jres[K].final_partition) {
        std::fprintf(stderr, "DESACCORD foret K=%d (flux WSPD contre brut)\n", K);
        ++disagreements;
      }
    }
  }
  const auto t4 = std::chrono::steady_clock::now();

  const auto ms = [](auto d) {
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(d).count() /
           1000.0;
  };
  u64 events_total = 0, fusions_total = 0, nodes_total = 0;
  for (int K = 1; K <= 10; ++K) {
    events_total += sev[K];
    fusions_total += sres[K].fusions;
    nodes_total += sres[K].nodes.size();
  }
  std::printf(
      "famille=%s n=%zu s=%lld smax=%llu seed=%lld candidats=%llu/%llu/%llu "
      "boules_uniques=%llu mortes_profondeur=%llu census_int=%llu "
      "census_shell=%llu evenements=%llu fusions=%llu noeuds=%llu "
      "desaccords=%llu t_flux_ms=%.1f t_census_ms=%.1f t_fold_ms=%.1f "
      "t_juge_ms=%.1f\n",
      cloud_family_name(a.family), pts.size(), (long long)a.s,
      (unsigned long long)smax_eff, a.seed, (unsigned long long)st.candidates[0],
      (unsigned long long)st.candidates[1], (unsigned long long)st.candidates[2],
      (unsigned long long)st.unique_balls,
      (unsigned long long)st.balls_dead_depth,
      (unsigned long long)st.census_interior, (unsigned long long)st.census_shell,
      (unsigned long long)events_total, (unsigned long long)fusions_total,
      (unsigned long long)nodes_total, (unsigned long long)disagreements,
      ms(t1 - t0), ms(t2 - t1), ms(t3 - t2), ms(t4 - t3));

  if (a.inj_rle_drop || a.inj_census_nonstrict) {
    if (a.judge && disagreements > 0) {
      std::printf("MUTANT TUE\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  if (a.judge && disagreements > 0) return 1;
  if (st.unique_balls < a.min_balls || fusions_total < a.min_fusions) {
    std::fprintf(stderr, "PLANCHER : boules=%llu fusions=%llu\n",
                 (unsigned long long)st.unique_balls,
                 (unsigned long long)fusions_total);
    return 3;
  }
  return 0;
}
