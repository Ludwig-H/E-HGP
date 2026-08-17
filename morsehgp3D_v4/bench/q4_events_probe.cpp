// MorseHGP3D v4 — PROBE D'EVENEMENTS q4 : la lane q4 devient productrice.
//
// Chaine complete (MATHEMATIQUES.md § 4.5, audits bc5b05d/489c617) :
//   WSPD ternaire lane q4 -> rectangles vivants (h_coeur,4 < h_4) -> ancres
//   survivantes (h_coeur,4 + h_a,4 + h_b,4 < h_4) -> seeds aigus canoniques
//   (acute_seed.hpp, le predicat PARTAGE avec q3) -> completions y dans la
//   lentille (owner 6 aretes, exact-once par carrier minimal du tetraedre
//   forme) -> arite 4 stricte (centre strictement interieur) -> census de
//   circum-boule (cover coefficient 4, paquet base_4 en prefixe) ->
//   Q4Event{support, owner, ball, level, depth, interior}.
// LA SOURCE EST LA LANE q4, JAMAIS q3 : les survies sont incomparables ET
// le flux des evenements q3 est aveugle (fixture RENFORCEE 22+1 points de
// l'audit apres f6b29e1 : les quatre faces du tetraedre sont q3-profondes,
// les deux ancres maximales ab/xy sont q3-mortes/q4-vivantes, l'evenement
// q4 a profondeur 1 — son interieur z n'est visible que du coefficient 4).
//
// JUGE (--judge, oracle borne petit n) : enumeration brute de TOUS les
// 4-sous-ensembles — owner 6 aretes sur les vrais PointId, arite stricte,
// census — et comparaison des RECORDS COMPLETS en multiensemble.
// Codes : 0 conforme, 1 desaccord, 2 refus, 3 invariant, 4 mutant tue.
// Mutants : --inject=seeds-from-q3-live (ancres q3-vivantes seulement),
// --inject=seeds-from-q3-events (faces peu profondes seulement — le flux
// q3), --inject=cover-coef3 (census borne a 3D²), --inject=no-canonical
// (exact-once saute) — les trois premiers perdent ou alterent l'evenement
// grave de la fixture, le dernier duplique des supports.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/events/acute_seed.hpp"
#include "../src/events/edge_cover.hpp"
#include "../src/events/q4_event.hpp"
#include "../src/events/witness_count.hpp"
#include "../src/wspd/wavefront.hpp"

namespace {

using namespace mhgp4;

struct Args {
  CloudFamily family = CloudFamily::kUniform;
  bool family_ok = true;
  bool fixture = false;
  int n = 120;
  int coord = 0;
  long long seed = 3;
  i64 s = 8;
  u64 smax = 11;
  bool judge = false;
  bool exact_mode = false;
  bool inj_seeds_q3 = false;
  bool inj_seeds_q3_events = false;
  bool inj_cover3 = false;
  bool inj_no_canonical = false;
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
    else if (arg == "--inject=seeds-from-q3-live") a.inj_seeds_q3 = true;
    else if (arg == "--inject=seeds-from-q3-events") a.inj_seeds_q3_events = true;
    else if (arg == "--inject=cover-coef3") a.inj_cover3 = true;
    else if (arg == "--inject=no-canonical") a.inj_no_canonical = true;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      a.family_ok = false;
    }
  }
  return a;
}

// La fixture RENFORCEE de l'audit apres f6b29e1 : 13 points de bc5b05d +
// les neuf w_j = (196+j, 105, 300) (ids 13..21) qui rendent les faces
// axy/bxy q3-profondes (le flux des evenements q3 devient aveugle au
// tetraedre) et l'ancre xy q3-morte/q4-vivante, + z = (200,109,300)
// (id 22) : interieur de la circum-boule q4 (|z-c|² = 14641 < 14900) MAIS
// hors du cover coefficient 3 (|2z-(a+b)|² = 145924 > 3D² = 120000,
// <= 4D² = 160000) — il tue le mutant cover-coef3 ; H < 0 vis-a-vis de
// ab : les survies de l'audit sont inchangees.
std::vector<P3> fixture23() {
  std::vector<P3> pts = {
      {100, 300, 300}, {300, 300, 300}, {200, 160, 400}, {200, 160, 200},
      {200, 355, 300}, {200, 354, 310}, {200, 353, 315}, {200, 352, 320},
      {200, 351, 323}, {200, 350, 325}, {200, 356, 305}, {200, 355, 312},
      {200, 354, 317}};
  for (i64 j = 0; j < 9; ++j) pts.push_back(P3{196 + j, 105, 300});
  pts.push_back(P3{200, 109, 300});  // id 22
  return pts;
}

Q4Event make_event4(const SupportKey4& sk, const EdgeKey& ek,
                    const Q3BallKey& bk, const Q4Level& lv,
                    const i32* interior_u, u64 n_interior,
                    const std::vector<PointId>& pid_of) {
  Q4Event e;
  e.support = sk;
  e.owner = ek;
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
  if (!a.family_ok || (!a.fixture && a.n < 5) || a.s < 1) {
    std::fprintf(stderr, "REFUS : arguments invalides\n");
    return 2;
  }
  if (a.smax > 11) {
    std::fprintf(stderr, "REFUS : profil K_max<=10 (smax<=11) — smax=%llu\n",
                 (unsigned long long)a.smax);
    return 2;
  }
  const std::vector<P3> pts =
      a.fixture ? fixture23()
                : make_family_cloud(a.family, a.n,
                                    a.coord > 0 ? a.coord
                                                : cloud_family_default_coord(a.family, a.n),
                                    a.seed);
  const u64 smax_eff = std::min<u64>(a.smax, pts.size());
  if (smax_eff < 5) {
    std::fprintf(stderr, "REFUS : s_max effectif trop petit\n");
    return 2;
  }
  // MUTANT seeds-from-q3-live : le branchement interdit par l'audit
  // bc5b05d, injecte tel quel — une ancre ne seme que si elle est q3-VIVANTE
  // (compte EXACT n3 < h_3). La chaine reste la lane q4 partout ailleurs ;
  // sur la fixture, l'ancre (a,b) a n3 = 9 : le tetraedre est perdu.
  const Lane lane = Lane::kQ4;
  const u8 lane_mask = 0b100;
  const int lane_idx = 2;
  const u64 hq = lane_h(lane, smax_eff);
  const u64 h3_exact = lane_h(Lane::kQ3, smax_eff);
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax_eff), lane_h(Lane::kQ3, smax_eff),
                       lane_h(Lane::kQ4, smax_eff)};
  const i64 coef = a.inj_cover3 ? 3 : 4;
  const auto t0 = std::chrono::steady_clock::now();
  const CloudIndex ix = build_cloud_index(pts);
  if ((size_t)ix.unique_count() != pts.size()) {
    std::fprintf(stderr, "REFUS unsupported_degeneracy : positions dupliquees\n");
    return 2;
  }

  // 1. WSPD ternaire, lane q4 (ou q3 sous mutant) : rectangles vivants.
  struct AliveRect { WspdRect r; u64 core; };
  std::vector<AliveRect> alive;
  if (!ix.nodes.empty()) {
    std::vector<WspdRect> wave, next;
    for (const RadixNode& n : ix.nodes) wave.push_back(WspdRect{n.left, n.right});
    while (!wave.empty()) {
      next.clear();
      for (const WspdRect& r : wave) {
        const FusedCounts fc =
            count_universal_witnesses_234(ix, r.a, r.b, h_of, lane_mask, false);
        if (fc.c[lane_idx] >= hq) continue;
        i64 ba[3], bb[3];
        const auto va = detail::node_view(ix, r.a, ba);
        const auto vb = detail::node_view(ix, r.b, bb);
        if (detail::separated(va, vb, a.s, 1)) {
          const FusedCounts ff =
              count_universal_witnesses_234(ix, r.a, r.b, h_of, lane_mask, true);
          if (ff.c[lane_idx] < hq) alive.push_back(AliveRect{r, ff.c[lane_idx]});
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

  // 2. Instruction q4.
  std::vector<Q4Event> records;
  u64 anchors_seen = 0, anchors_killed_ha = 0, seeds_seen = 0,
      completions_tried = 0, coplanar_skips = 0, center_outside = 0,
      shell_refused = 0, power_tests = 0, rect_cover_nodes = 0,
      anchor_point_visits = 0;
  std::vector<PointId> pid_of((size_t)ix.unique_count());
  for (size_t u = 0; u < pid_of.size(); ++u)
    pid_of[u] = ix.bucket_ids[ix.bucket_start[u]];
  const auto pid = [&](i32 u) { return pid_of[(size_t)u]; };
  std::vector<CoverPoint> cover;
  std::vector<u64> ha, hb;
  for (const AliveRect& ar : alive) {
    const NodeRange ra = range_of(ix, ar.r.a);
    const NodeRange rb = range_of(ix, ar.r.b);
    const AxisBox boxA = box_of_node(ix, ar.r.a);
    const AxisBox boxB = box_of_node(ix, ar.r.b);
    const int na = ra.last - ra.first + 1;
    const int nb = rb.last - rb.first + 1;
    const u64 need = hq - ar.core;
    ha.assign((size_t)na, 0);
    hb.assign((size_t)nb, 0);
    for (int ia = 0; ia < na; ++ia)
      for (int iz = 0; iz < na; ++iz) {
        if (iz == ia) continue;
        if (universal_over_corners(lane, ix.upos[(size_t)(ra.first + ia)], boxB,
                                   ix.upos[(size_t)(ra.first + iz)]))
          ++ha[(size_t)ia];
      }
    for (int ib = 0; ib < nb; ++ib)
      for (int iz = 0; iz < nb; ++iz) {
        if (iz == ib) continue;
        if (universal_over_corners(lane, ix.upos[(size_t)(rb.first + ib)], boxA,
                                   ix.upos[(size_t)(rb.first + iz)]))
          ++hb[(size_t)ib];
      }
    std::vector<NodeRef> handles;
    rect_cover_handles(ix, boxA, boxB, coef, false, &handles, &rect_cover_nodes);
    i32 core_ids[8];
    const u64 core_n = collect_universal_ids(lane, ix, ar.r.a, ar.r.b, 8, core_ids);
    if (core_n != ar.core) {
      std::fprintf(stderr, "INVARIANT : core_ids=%llu != h_coeur=%llu\n",
                   (unsigned long long)core_n, (unsigned long long)ar.core);
      return 3;
    }
    std::vector<std::array<i32, 8>> ha_ids((size_t)na), hb_ids((size_t)nb);
    std::vector<u8> ha_idn((size_t)na, 0), hb_idn((size_t)nb, 0);
    for (int ia = 0; ia < na; ++ia)
      for (int iz = 0; iz < na && ha_idn[(size_t)ia] < 8; ++iz) {
        if (iz == ia) continue;
        if (universal_over_corners(lane, ix.upos[(size_t)(ra.first + ia)], boxB,
                                   ix.upos[(size_t)(ra.first + iz)]))
          ha_ids[(size_t)ia][ha_idn[(size_t)ia]++] = ra.first + iz;
      }
    for (int ib = 0; ib < nb; ++ib)
      for (int iz = 0; iz < nb && hb_idn[(size_t)ib] < 8; ++iz) {
        if (iz == ib) continue;
        if (universal_over_corners(lane, ix.upos[(size_t)(rb.first + ib)], boxA,
                                   ix.upos[(size_t)(rb.first + iz)]))
          hb_ids[(size_t)ib][hb_idn[(size_t)ib]++] = rb.first + iz;
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
        if (a.inj_seeds_q3 &&
            true_spindle_count(Lane::kQ3, ix, ua, ub, h3_exact) >= h3_exact)
          continue;  // MUTANT : l'ancre q3-morte ne seme plus
        anchor_cover_from_handles(ix, handles, pa, pb, D2, coef, &cover,
                                  &anchor_point_visits);
        i32 packet_ids[26];
        u64 base = 0;
        {
          const u8 nha = ha_idn[(size_t)(ua - ra.first)];
          const u8 nhb = hb_idn[(size_t)(ub - rb.first)];
          if (nha != ha[(size_t)(ua - ra.first)] ||
              nhb != hb[(size_t)(ub - rb.first)]) {
            std::fprintf(stderr, "INVARIANT : collecteur ha/hb != histogramme\n");
            return 3;
          }
          for (u64 t = 0; t < core_n; ++t) packet_ids[base++] = core_ids[t];
          for (u8 t = 0; t < nha; ++t)
            packet_ids[base++] = ha_ids[(size_t)(ua - ra.first)][t];
          for (u8 t = 0; t < nhb; ++t)
            packet_ids[base++] = hb_ids[(size_t)(ub - rb.first)][t];
          if (base != core_n + nha + nhb || base >= hq) {
            std::fprintf(stderr, "INVARIANT : taille de paquet %llu\n",
                         (unsigned long long)base);
            return 3;
          }
          for (u64 t = 0; t < base; ++t)
            for (u64 t2 = t + 1; t2 < base; ++t2)
              if (packet_ids[t] == packet_ids[t2]) {
                std::fprintf(stderr, "INVARIANT : doublon dans le paquet\n");
                return 3;
              }
        }
        const auto in_packet = [&](i32 u) {
          for (u64 t = 0; t < base; ++t)
            if (packet_ids[t] == u) return true;
          return false;
        };
        // Seeds aigus canoniques de l'ancre (predicat partage avec q3).
        std::vector<i32> seeds;
        for (const CoverPoint& cp : cover) {
          if (cp.u == ua || cp.u == ub) continue;
          if (!is_acute_seed(pa, pb, ix.upos[(size_t)cp.u], D2, pid(ua), pid(ub),
                             pid(cp.u)))
            continue;
          if (a.inj_seeds_q3_events) {
            // MUTANT : seul le FLUX des evenements q3 seme — une face
            // q3-profonde ne produit plus de seed. Sur la fixture, les
            // quatre faces du tetraedre sont profondes : il est perdu.
            const Q3Form f3 = q3_form(pa, pb, ix.upos[(size_t)cp.u]);
            u64 sh = 0;
            const u64 dq3 =
                q3_ball_depth(ix, f3, ua, ub, cp.u, h3_exact, &sh);
            if (dq3 >= h3_exact) continue;
          }
          seeds.push_back(cp.u);
        }
        seeds_seen += seeds.size();
        // Completions : quatrieme sommet y dans la lentille ENTIERE.
        for (const i32 ux : seeds) {
          const P3& px = ix.upos[(size_t)ux];
          const i64 l_ax = p3_norm2(p3_sub(px, pa));
          const i64 l_bx = p3_norm2(p3_sub(px, pb));
          for (const CoverPoint& cy : cover) {
            const i32 uy = cy.u;
            if (uy == ux || uy == ua || uy == ub) continue;
            const P3& py = ix.upos[(size_t)uy];
            const i64 l_ay = p3_norm2(p3_sub(py, pa));
            const i64 l_by = p3_norm2(p3_sub(py, pb));
            if (l_ay > D2 || l_by > D2) continue;  // lentille
            const i64 l_xy = p3_norm2(p3_sub(py, px));
            if (l_xy > D2) continue;
            ++completions_tried;
            if (!tetra_owned_by(D2, l_ax, l_ay, l_bx, l_by, l_xy, pid(ua),
                                pid(ub), pid(ux), pid(uy)))
              continue;
            // Exact-once : carrier canonique = plus petit PointId parmi les
            // faces incidentes a (a,b) strictement aigues du tetraedre.
            if (!a.inj_no_canonical) {
              const P3 vy{2 * py.x - pa.x - pb.x, 2 * py.y - pa.y - pb.y,
                          2 * py.z - pa.z - pb.z};
              const bool acute_y = p3_norm2(vy) > D2;
              if (acute_y && pid(uy) < pid(ux)) continue;
            }
            const Q4Form f4 = q4_form(pa, pb, px, py);
            if (f4.det == 0) {
              ++coplanar_skips;
              continue;
            }
            if (!q4_center_strictly_inside(f4, pa, pb, px, py)) {
              ++center_outside;
              continue;
            }
            // Census : paquet en prefixe, cover coefficient 4.
            u64 depth = base;
            bool shell = false;
            i32 interior_u[16];
            u8 ni = 0;
            for (const CoverPoint& wz : cover) {
              if (depth >= hq) break;
              const i32 uz = wz.u;
              if (uz == ua || uz == ub || uz == ux || uz == uy || in_packet(uz))
                continue;
              ++power_tests;
              const i128 pw = q4_power(f4, ix.upos[(size_t)uz]);
              if (pw < 0) {
                if (ni < 8) interior_u[ni++] = uz;
                ++depth;
              } else if (pw == 0) {
                shell = true;
              }
            }
            if (depth >= hq) continue;
            if (shell) {
              if (a.exact_mode) {
                std::fprintf(stderr,
                             "unsupported_degeneracy : extra-shell sur une "
                             "boule q4 survivante\n");
                return 2;
              }
              ++shell_refused;
              continue;
            }
            i32 all_interior[16];
            u64 n_interior = 0;
            for (u64 t = 0; t < base; ++t) all_interior[n_interior++] = packet_ids[t];
            for (u8 t = 0; t < ni; ++t) all_interior[n_interior++] = interior_u[t];
            if (n_interior != depth) {
              std::fprintf(stderr,
                           "INVARIANT : interieurs=%llu != profondeur=%llu\n",
                           (unsigned long long)n_interior,
                           (unsigned long long)depth);
              return 3;
            }
            records.push_back(make_event4(
                support_key4(pid(ua), pid(ub), pid(ux), pid(uy)),
                edge_key(pid(ua), pid(ub)),
                q3_ball_key_reduce(q4_ball_form(f4)), q4_level_raw(f4),
                all_interior, n_interior, pid_of));
          }
        }
      }
  }
  std::stable_sort(records.begin(), records.end());
  u64 duplicate_supports = 0;
  for (size_t t = 1; t < records.size(); ++t)
    if (records[t].support == records[t - 1].support) ++duplicate_supports;
  std::vector<Q3BallKey> ball_keys;
  ball_keys.reserve(records.size());
  for (const Q4Event& e : records) ball_keys.push_back(e.ball);
  std::sort(ball_keys.begin(), ball_keys.end());
  ball_keys.erase(std::unique(ball_keys.begin(), ball_keys.end()), ball_keys.end());
  const auto t2 = std::chrono::steady_clock::now();

  // 3. Juge brut : tous les 4-sous-ensembles, records complets.
  u64 missing = 0, extra = 0;
  if (a.judge) {
    std::vector<Q4Event> truth;
    const int m = ix.unique_count();
    const u64 h4 = lane_h(Lane::kQ4, smax_eff);
    for (i32 i = 0; i < m; ++i)
      for (i32 j = i + 1; j < m; ++j)
        for (i32 k = j + 1; k < m; ++k)
          for (i32 l = k + 1; l < m; ++l) {
            const i32 vs[4] = {i, j, k, l};
            // Owner : arete maximale des six, ex aequo par EdgeKey minimale.
            i32 bu = -1, bv = -1;
            i64 bl2 = -1;
            for (int s0 = 0; s0 < 4; ++s0)
              for (int s1 = s0 + 1; s1 < 4; ++s1) {
                const i64 l2 = p3_norm2(p3_sub(ix.upos[(size_t)vs[s1]],
                                               ix.upos[(size_t)vs[s0]]));
                if (l2 > bl2 ||
                    (l2 == bl2 &&
                     edge_key_less(edge_key(pid(vs[s0]), pid(vs[s1])),
                                   edge_key(pid(bu), pid(bv))))) {
                  bl2 = l2;
                  bu = vs[s0];
                  bv = vs[s1];
                }
              }
            i32 ox = -1, oy = -1;
            for (const i32 u : vs)
              if (u != bu && u != bv) (ox < 0 ? ox : oy) = u;
            const P3 &pa = ix.upos[(size_t)bu], &pb = ix.upos[(size_t)bv];
            const P3 &px = ix.upos[(size_t)ox], &py = ix.upos[(size_t)oy];
            const Q4Form f4 = q4_form(pa, pb, px, py);
            if (f4.det == 0) continue;
            if (!q4_center_strictly_inside(f4, pa, pb, px, py)) continue;
            u64 depth = 0, shell = 0;
            i32 interior_u[8];
            u8 ni = 0;
            for (i32 u = 0; u < m && depth < h4; ++u) {
              if (u == i || u == j || u == k || u == l) continue;
              const i128 pw = q4_power(f4, ix.upos[(size_t)u]);
              if (pw < 0) {
                if (ni < 8) interior_u[ni++] = u;
                ++depth;
              } else if (pw == 0) {
                ++shell;
              }
            }
            if (depth >= h4 || shell > 0) continue;
            if ((u64)ni != depth) {
              std::fprintf(stderr, "INVARIANT (juge) : collecteur\n");
              return 3;
            }
            truth.push_back(make_event4(
                support_key4(pid(i), pid(j), pid(k), pid(l)),
                edge_key(pid(bu), pid(bv)),
                q3_ball_key_reduce(q4_ball_form(f4)), q4_level_raw(f4),
                interior_u, depth, pid_of));
          }
    std::stable_sort(truth.begin(), truth.end());
    std::vector<Q4Event> diff;
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
      "source=%s n=%zu s=%lld smax=%llu seed=%lld lane=%s coef=%lld "
      "rect_vivants=%zu ancres_vues=%llu ancres_tuees_ha=%llu seeds=%llu "
      "completions=%llu coplanaires=%llu centre_hors=%llu evenements=%zu "
      "doublons=%llu ballkeys_uniques=%zu tests_puissance=%llu "
      "noeuds_cover=%llu visites_filtre=%llu shell_refus=%llu "
      "juge_manquants=%llu juge_en_trop=%llu t_wspd_ms=%.1f "
      "t_instruction_ms=%.1f t_juge_ms=%.1f\n",
      a.fixture ? "fixture23" : cloud_family_name(a.family), pts.size(),
      (long long)a.s, (unsigned long long)smax_eff, a.seed,
      a.inj_seeds_q3 ? "q3(MUTANT)" : "q4", (long long)coef, alive.size(),
      (unsigned long long)anchors_seen, (unsigned long long)anchors_killed_ha,
      (unsigned long long)seeds_seen, (unsigned long long)completions_tried,
      (unsigned long long)coplanar_skips, (unsigned long long)center_outside,
      records.size(), (unsigned long long)duplicate_supports, ball_keys.size(),
      (unsigned long long)power_tests, (unsigned long long)rect_cover_nodes,
      (unsigned long long)anchor_point_visits,
      (unsigned long long)shell_refused, (unsigned long long)missing,
      (unsigned long long)extra, ms(t1 - t0), ms(t2 - t1), ms(t3 - t2));

  // Fixture gravee : l'evenement {0,1,2,3} avec owner (0,1), profondeur 1,
  // interieur {22} (le point z, visible du seul cover coefficient 4).
  if (a.fixture) {
    const SupportKey4 want = support_key4(0, 1, 2, 3);
    const Q4Event* found = nullptr;
    for (const Q4Event& e : records)
      if (e.support == want) found = &e;
    const bool ok = found && found->owner.lo == 0 && found->owner.hi == 1 &&
                    found->depth == 1 && found->interior[0] == 22;
    if (a.inj_seeds_q3 || a.inj_seeds_q3_events || a.inj_cover3) {
      if (!ok) {
        std::printf("MUTANT TUE : le tetraedre grave est perdu ou altere\n");
        return 4;
      }
      std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
      return 3;
    }
    if (!ok) {
      std::fprintf(stderr, "FIXTURE : evenement {0,1,2,3} absent ou altere\n");
      return 3;
    }
  }
  if (a.inj_no_canonical) {
    if (duplicate_supports > 0) {
      std::printf("MUTANT TUE : exact-once perdu, supports dupliques\n");
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant non discrimine\n");
    return 3;
  }
  if (a.inj_seeds_q3 || a.inj_seeds_q3_events || a.inj_cover3) {
    // Ces mutants ne se jugent que sur la fixture gravee.
    std::fprintf(stderr, "PORTE INEFFICACE : mutant sans fixture\n");
    return 3;
  }
  if (ball_keys.size() != records.size()) {
    std::fprintf(stderr, "INVARIANT : %zu BallKeys pour %zu evenements\n",
                 ball_keys.size(), records.size());
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
