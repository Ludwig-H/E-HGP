// MorseHGP3D v4 — PROBE D'EVENEMENTS q2 : la troisieme lane productrice.
//
// Chaine : WSPD ternaire lane q2 -> rectangles vivants (h_coeur,2 < h_2)
// -> ancres survivantes (h_coeur + h_a + h_b < h_2, histogrammes 8 coins)
// -> census diametral EXACT par requete de cover a coefficient 1
// (W_2(a,b) est la boule diametrale ouverte : interieur < D², coquille
// == D², a et b sur la sphere exclus comme support) -> Q2Event{support,
// ball (A=1, primitive), niveau D²/4, depth, interieurs tries}.
// Exact-once : la partition CK des paires — chaque paire vit dans
// exactement un rectangle ; l'invariant de doublons le grave.
//
// JUGE (--judge, oracle borne) : TOUTES les paires {i<j}, census brut sur
// tous les points, records complets en multiensemble.
// Codes : 0 conforme, 1 desaccord, 2 refus, 3 invariant, 4 mutant tue.
// Mutants : --inject=sign-le (la coquille comptee interieure : publie un
// evenement la ou la verite refuse — tue par la fixture a coquille),
// --inject=drop-anchor-filter... (reserve). Fixture gravee (--fixture) :
// a=(0,0,0), b=(4,0,0), coquille z=(2,2,0) (|2z-s|² = 16 = D²), interieur
// w=(2,1,0) — l'arete ab est REFUSEE (coquille) en regime regulier.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/events/edge_cover.hpp"
#include "../src/events/q2_event.hpp"
#include "../src/events/witness_count.hpp"
#include "../src/wspd/wavefront.hpp"

namespace {

using namespace mhgp4;

struct Args {
  CloudFamily family = CloudFamily::kUniform;
  bool family_ok = true;
  bool fixture = false;
  int n = 400;
  int coord = 0;
  long long seed = 3;
  i64 s = 8;
  u64 smax = 11;
  bool judge = false;
  bool exact_mode = false;
  bool inj_sign_le = false;
  u64 min_events = 0;
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
    if (arg == "--fixture") a.fixture = true;
    else if (const char* v = val("--family=")) a.family_ok = parse_family(v, &a.family);
    else if (const char* v = val("--n=")) a.n = std::atoi(v);
    else if (const char* v = val("--coord=")) a.coord = std::atoi(v);
    else if (const char* v = val("--seed=")) a.seed = std::atoll(v);
    else if (const char* v = val("--s=")) a.s = std::atoll(v);
    else if (const char* v = val("--smax=")) a.smax = (u64)std::atoll(v);
    else if (const char* v = val("--min-events=")) a.min_events = (u64)std::atoll(v);
    else if (arg == "--judge") a.judge = true;
    else if (arg == "--exact") a.exact_mode = true;
    else if (arg == "--inject=sign-le") a.inj_sign_le = true;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      a.family_ok = false;
    }
  }
  return a;
}

// Fixture a coquille : l'arete (0,1) a un interieur strict w et une
// COQUILLE exacte z (|2z-(a+b)|² = D²) — refusee en regime regulier ; le
// mutant sign-le la publie a tort. Deux points lointains isolent le reste.
std::vector<P3> fixture_shell() {
  return {{0, 0, 0}, {4, 0, 0}, {2, 2, 0}, {2, 1, 0}, {40, 40, 40}, {60, 20, 50}};
}

Q2Event make_event2(const EdgeKey& ek, const Q3BallKey& bk, const Q3Level& lv,
                    const i32* interior_u, u64 n_interior,
                    const std::vector<PointId>& pid_of) {
  Q2Event e;
  e.support = ek;
  e.ball = bk;
  e.level = lv;
  n_interior = std::min<u64>(n_interior, e.interior.size());
  e.depth = (u8)n_interior;
  for (u64 t = 0; t < n_interior; ++t)
    e.interior[t] = pid_of[(size_t)interior_u[t]];
  for (u64 t = 1; t < n_interior; ++t) {
    const PointId v = e.interior[t];
    u64 w = t;
    for (; w > 0 && e.interior[w - 1] > v; --w) e.interior[w] = e.interior[w - 1];
    e.interior[w] = v;
  }
  return e;
}

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  const Args a = parse(argc, argv);
  if (!a.family_ok || (!a.fixture && a.n < 3) || a.s < 1) {
    std::fprintf(stderr, "REFUS : arguments invalides\n");
    return 2;
  }
  if (a.smax > 11) {
    std::fprintf(stderr, "REFUS : profil K_max<=10 (smax<=11) — smax=%llu\n",
                 (unsigned long long)a.smax);
    return 2;
  }
  const std::vector<P3> pts =
      a.fixture ? fixture_shell()
                : make_family_cloud(a.family, a.n,
                                    a.coord > 0 ? a.coord
                                                : cloud_family_default_coord(a.family, a.n),
                                    a.seed);
  const u64 smax_eff = std::min<u64>(a.smax, pts.size());
  if (smax_eff < 3) {
    std::fprintf(stderr, "REFUS : s_max effectif trop petit\n");
    return 2;
  }
  const u64 h2 = lane_h(Lane::kQ2, smax_eff);
  const u64 h_of[3] = {h2, lane_h(Lane::kQ3, smax_eff), lane_h(Lane::kQ4, smax_eff)};
  const auto t0 = std::chrono::steady_clock::now();
  const CloudIndex ix = build_cloud_index(pts);
  if ((size_t)ix.unique_count() != pts.size()) {
    std::fprintf(stderr, "REFUS unsupported_degeneracy : positions dupliquees\n");
    return 2;
  }

  // 1. WSPD ternaire, lane q2.
  struct AliveRect { WspdRect r; u64 core; };
  std::vector<AliveRect> alive;
  if (!ix.nodes.empty()) {
    std::vector<WspdRect> wave, next;
    for (const RadixNode& n : ix.nodes) wave.push_back(WspdRect{n.left, n.right});
    while (!wave.empty()) {
      next.clear();
      for (const WspdRect& r : wave) {
        const FusedCounts fc =
            count_universal_witnesses_234(ix, r.a, r.b, h_of, 0b001, false);
        if (fc.c[0] >= h2) continue;
        i64 ba[3], bb[3];
        const auto va = detail::node_view(ix, r.a, ba);
        const auto vb = detail::node_view(ix, r.b, bb);
        if (detail::separated(va, vb, a.s, 1)) {
          const FusedCounts ff =
              count_universal_witnesses_234(ix, r.a, r.b, h_of, 0b001, true);
          if (ff.c[0] < h2) alive.push_back(AliveRect{r, ff.c[0]});
          continue;
        }
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
  const auto t1 = std::chrono::steady_clock::now();

  // 2. Instruction q2 : histogrammes h_a/h_b puis census diametral exact.
  std::vector<Q2Event> records;
  u64 anchors_seen = 0, anchors_killed_ha = 0, shell_refused = 0,
      census_points = 0;
  std::vector<PointId> pid_of((size_t)ix.unique_count());
  for (size_t u = 0; u < pid_of.size(); ++u)
    pid_of[u] = ix.bucket_ids[ix.bucket_start[u]];
  const auto pid = [&](i32 u) { return pid_of[(size_t)u]; };
  std::vector<CoverPoint> ball;
  std::vector<u64> ha, hb;
  for (const AliveRect& ar : alive) {
    const NodeRange ra = range_of(ix, ar.r.a);
    const NodeRange rb = range_of(ix, ar.r.b);
    const AxisBox boxA = box_of_node(ix, ar.r.a);
    const AxisBox boxB = box_of_node(ix, ar.r.b);
    const int na = ra.last - ra.first + 1;
    const int nb = rb.last - rb.first + 1;
    const u64 need = h2 - ar.core;
    ha.assign((size_t)na, 0);
    hb.assign((size_t)nb, 0);
    for (int ia = 0; ia < na; ++ia)
      for (int iz = 0; iz < na; ++iz) {
        if (iz == ia) continue;
        if (universal_over_corners(Lane::kQ2, ix.upos[(size_t)(ra.first + ia)],
                                   boxB, ix.upos[(size_t)(ra.first + iz)]))
          ++ha[(size_t)ia];
      }
    for (int ib = 0; ib < nb; ++ib)
      for (int iz = 0; iz < nb; ++iz) {
        if (iz == ib) continue;
        if (universal_over_corners(Lane::kQ2, ix.upos[(size_t)(rb.first + ib)],
                                   boxA, ix.upos[(size_t)(rb.first + iz)]))
          ++hb[(size_t)ib];
      }
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        ++anchors_seen;
        if (ha[(size_t)(ua - ra.first)] + hb[(size_t)(ub - rb.first)] >= need) {
          ++anchors_killed_ha;
          continue;
        }
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        // Census diametral : cover coefficient 1 = boule fermee, interieur
        // strict <, coquille == (support a,b exclu — il est sur la sphere).
        cover_query(ix, pa, pb, D2, 1, &ball);
        census_points += ball.size();
        u64 depth = 0, shell = 0;
        i32 interior_u[16];
        u8 ni = 0;
        for (const CoverPoint& cp : ball) {
          if (cp.u == ua || cp.u == ub) continue;
          // MUTANT sign-le : la coquille est comptee interieure.
          if (cp.dist2q < D2 || (a.inj_sign_le && cp.dist2q == D2)) {
            if (ni < 16) interior_u[ni++] = cp.u;
            ++depth;
          } else if (cp.dist2q == D2) {
            ++shell;
          }
        }
        if (depth >= h2) continue;
        if (shell > 0) {
          if (a.exact_mode) {
            std::fprintf(stderr,
                         "unsupported_degeneracy : coquille sur une boule q2 "
                         "survivante\n");
            return 2;
          }
          ++shell_refused;
          continue;
        }
        if ((u64)ni != depth) {
          std::fprintf(stderr, "INVARIANT : interieurs=%u != profondeur=%llu\n",
                       (unsigned)ni, (unsigned long long)depth);
          return 3;
        }
        records.push_back(make_event2(edge_key(pid(ua), pid(ub)),
                                      q2_ball_key(pa, pb), q2_exact_level(D2),
                                      interior_u, depth, pid_of));
      }
  }
  std::stable_sort(records.begin(), records.end());
  u64 duplicate_supports = 0;
  for (size_t t = 1; t < records.size(); ++t)
    if (records[t].support.lo == records[t - 1].support.lo &&
        records[t].support.hi == records[t - 1].support.hi)
      ++duplicate_supports;
  const auto t2 = std::chrono::steady_clock::now();

  // 3. Juge brut : toutes les paires, records complets.
  u64 missing = 0, extra = 0;
  if (a.judge) {
    std::vector<Q2Event> truth;
    const int m = ix.unique_count();
    for (i32 i = 0; i < m; ++i)
      for (i32 j = i + 1; j < m; ++j) {
        const P3 &pa = ix.upos[(size_t)i], &pb = ix.upos[(size_t)j];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        const i64 s2[3] = {pa.x + pb.x, pa.y + pb.y, pa.z + pb.z};
        u64 depth = 0, shell = 0;
        i32 interior_u[16];
        u8 ni = 0;
        for (i32 u = 0; u < m && depth < h2; ++u) {
          if (u == i || u == j) continue;
          const P3& pz = ix.upos[(size_t)u];
          const i64 t0v = 2 * pz.x - s2[0];
          const i64 t1v = 2 * pz.y - s2[1];
          const i64 t2v = 2 * pz.z - s2[2];
          const i64 d2 = t0v * t0v + t1v * t1v + t2v * t2v;
          if (d2 < D2) {
            if (ni < 16) interior_u[ni++] = u;
            ++depth;
          } else if (d2 == D2) {
            ++shell;
          }
        }
        if (depth >= h2 || shell > 0) continue;
        if ((u64)ni != depth) {
          std::fprintf(stderr, "INVARIANT (juge) : collecteur\n");
          return 3;
        }
        truth.push_back(make_event2(edge_key(pid(i), pid(j)), q2_ball_key(pa, pb),
                                    q2_exact_level(D2), interior_u, depth,
                                    pid_of));
      }
    std::stable_sort(truth.begin(), truth.end());
    std::vector<Q2Event> diff;
    std::set_difference(truth.begin(), truth.end(), records.begin(),
                        records.end(), std::back_inserter(diff));
    missing = diff.size();
    diff.clear();
    std::set_difference(records.begin(), records.end(), truth.begin(),
                        truth.end(), std::back_inserter(diff));
    extra = diff.size();
  }
  const auto t3 = std::chrono::steady_clock::now();

  const auto ms = [](auto d) {
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(d).count() /
           1000.0;
  };
  std::printf(
      "source=%s n=%zu s=%lld smax=%llu seed=%lld rect_vivants=%zu "
      "ancres_vues=%llu ancres_tuees_ha=%llu evenements=%zu doublons=%llu "
      "points_census=%llu shell_refus=%llu juge_manquants=%llu "
      "juge_en_trop=%llu t_wspd_ms=%.1f t_instruction_ms=%.1f t_juge_ms=%.1f\n",
      a.fixture ? "fixture_shell" : cloud_family_name(a.family), pts.size(),
      (long long)a.s, (unsigned long long)smax_eff, a.seed, alive.size(),
      (unsigned long long)anchors_seen, (unsigned long long)anchors_killed_ha,
      records.size(), (unsigned long long)duplicate_supports,
      (unsigned long long)census_points, (unsigned long long)shell_refused,
      (unsigned long long)missing, (unsigned long long)extra, ms(t1 - t0),
      ms(t2 - t1), ms(t3 - t2));

  // Fixture gravee : l'arete (0,1) est REFUSEE pour coquille en regime
  // regulier ; le mutant sign-le la publie a tort (le juge le voit aussi).
  if (a.fixture) {
    bool published_01 = false;
    for (const Q2Event& e : records)
      if (e.support.lo == 0 && e.support.hi == 1) published_01 = true;
    if (a.inj_sign_le) {
      if (published_01) {
        std::printf("MUTANT TUE : la coquille publiee a tort\n");
        return 4;
      }
      std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
      return 3;
    }
    if (published_01 || shell_refused == 0) {
      std::fprintf(stderr, "FIXTURE : refus de coquille attendu sur (0,1)\n");
      return 3;
    }
  } else if (a.inj_sign_le) {
    std::fprintf(stderr, "PORTE INEFFICACE : mutant sans fixture\n");
    return 3;
  }
  if (duplicate_supports > 0) {
    std::fprintf(stderr, "EXACT-ONCE VIOLE : %llu supports dupliques\n",
                 (unsigned long long)duplicate_supports);
    return 3;
  }
  if (a.judge && (missing > 0 || extra > 0)) return 1;
  if (records.size() < a.min_events) {
    std::fprintf(stderr, "PLANCHER : %zu evenements < %llu\n", records.size(),
                 (unsigned long long)a.min_events);
    return 3;
  }
  return 0;
}
