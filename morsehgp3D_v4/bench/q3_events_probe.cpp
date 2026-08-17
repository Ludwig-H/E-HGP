// MorseHGP3D v4 — PROBE D'EVENEMENTS q3 : ancres survivantes -> supports.
//
// Chaine complete de la lane q3, counter-only :
//   WSPD ternaire fusionnee -> rectangles vivants q3 -> ancres survivantes
//   (h_coeur + h_a + h_b < h_3, par histogramme) -> porteurs x dans
//   lentille(ab) ∖ boule diametrale (requete d'arbre), acuite STRICTE
//   V² > D², owner EdgeKey -> profondeur de circum-boule exacte (Gram) ;
//   depth <= h_3 - 1 -> EVENEMENT q3 ; shell supplementaire -> refus
//   transactionnel compte a part (audit du 17 aout, Q5.2).
//
// JUGE (--judge, oracle borne petit n) : enumeration brute de TOUS les
// triangles {i<j<k} — owner par longueur/EdgeKey, acuite, profondeur — et
// comparaison PAR IDENTITES (SupportKey) : manquants, en trop, en accord.
// Codes : 0 conforme, 1 desaccord d'identites, 2 refus, 3 invariant.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/events/q3_instruction.hpp"
#include "../src/events/witness_count.hpp"
#include "../src/wspd/wavefront.hpp"

namespace {

using namespace mhgp4;

struct Args {
  CloudFamily family = CloudFamily::kUniform;
  bool family_ok = true;
  int n = 2000;
  int coord = 0;
  long long seed = 3;
  i64 s = 8;
  u64 smax = 11;
  bool judge = false;
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
    if (const char* v = val("--family=")) a.family_ok = parse_family(v, &a.family);
    else if (const char* v = val("--n=")) a.n = std::atoi(v);
    else if (const char* v = val("--coord=")) a.coord = std::atoi(v);
    else if (const char* v = val("--seed=")) a.seed = std::atoll(v);
    else if (const char* v = val("--s=")) a.s = std::atoll(v);
    else if (const char* v = val("--smax=")) a.smax = (u64)std::atoll(v);
    else if (const char* v = val("--min-events=")) a.min_events = (u64)std::atoll(v);
    else if (arg == "--judge") a.judge = true;
    else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      a.family_ok = false;
    }
  }
  return a;
}

// SupportKey q3 : triplet trie d'index de positions uniques (sites
// distincts, donc PointId ↔ position unique apres refus des doublons).
struct Key3 {
  i32 u[3];
  bool operator<(const Key3& o) const {
    for (int i = 0; i < 3; ++i)
      if (u[i] != o.u[i]) return u[i] < o.u[i];
    return false;
  }
  bool operator==(const Key3& o) const {
    return u[0] == o.u[0] && u[1] == o.u[1] && u[2] == o.u[2];
  }
};

Key3 make_key(i32 x, i32 y, i32 z) {
  Key3 k{{x, y, z}};
  std::sort(k.u, k.u + 3);
  return k;
}

// Requete de lentille : positions uniques x avec |x-a|² <= D² ET |x-b|² <= D².
void lens_query(const CloudIndex& ix, const P3& pa, const P3& pb, i64 D2,
                std::vector<i32>* out) {
  out->clear();
  if (ix.nodes.empty()) return;
  const auto box_min_dist2 = [](const AxisBox& b, const P3& p) {
    i128 s = 0;
    const i64 c[3] = {p.x, p.y, p.z};
    for (int i = 0; i < 3; ++i) {
      i64 d = 0;
      if (c[i] < b.lo[i]) d = b.lo[i] - c[i];
      else if (c[i] > b.hi[i]) d = c[i] - b.hi[i];
      s += (i128)d * d;
    }
    return s;
  };
  std::vector<NodeRef> stack{0};
  while (!stack.empty()) {
    const NodeRef z = stack.back();
    stack.pop_back();
    const AxisBox bz = box_of_node(ix, z);
    if (box_min_dist2(bz, pa) > D2 || box_min_dist2(bz, pb) > D2) continue;
    if (z < 0) {
      const i32 u = -1 - z;
      const P3& p = ix.upos[(size_t)u];
      if (p3_norm2(p3_sub(p, pa)) <= D2 && p3_norm2(p3_sub(p, pb)) <= D2)
        out->push_back(u);
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
}

}  // namespace

int main(int argc, char** argv) {
  using namespace mhgp4;
  const Args a = parse(argc, argv);
  if (!a.family_ok || a.n < 4 || a.s < 1) {
    std::fprintf(stderr, "REFUS : arguments invalides\n");
    return 2;
  }
  const int coord = a.coord > 0 ? a.coord : cloud_family_default_coord(a.family, a.n);
  const std::vector<P3> pts = make_family_cloud(a.family, a.n, coord, a.seed);
  const u64 smax_eff = std::min<u64>(a.smax, pts.size());
  if (smax_eff < 5) {
    std::fprintf(stderr, "REFUS : s_max effectif trop petit\n");
    return 2;
  }
  const u64 h3 = lane_h(Lane::kQ3, smax_eff);
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax_eff), h3, lane_h(Lane::kQ4, smax_eff)};
  const auto t0 = std::chrono::steady_clock::now();
  const CloudIndex ix = build_cloud_index(pts);
  if ((size_t)ix.unique_count() != pts.size()) {
    std::fprintf(stderr, "REFUS unsupported_degeneracy : positions dupliquees\n");
    return 2;
  }

  // 1. WSPD ternaire, lane q3 seulement : blocs morts / rectangles vivants.
  std::vector<WspdRect> alive;
  if (!ix.nodes.empty()) {
    std::vector<WspdRect> wave, next;
    for (const RadixNode& n : ix.nodes) wave.push_back(WspdRect{n.left, n.right});
    while (!wave.empty()) {
      next.clear();
      for (const WspdRect& r : wave) {
        const FusedCounts fc =
            count_universal_witnesses_234(ix, r.a, r.b, h_of, 0b010, false);
        if (fc.c[1] >= h3) continue;  // bloc mort pour q3
        i64 ba[3], bb[3];
        const auto va = detail::node_view(ix, r.a, ba);
        const auto vb = detail::node_view(ix, r.b, bb);
        if (detail::separated(va, vb, a.s, 1)) {
          const FusedCounts ff =
              count_universal_witnesses_234(ix, r.a, r.b, h_of, 0b010, true);
          if (ff.c[1] < h3) alive.push_back(r);
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

  // 2. Instruction : chaque ancre du rectangle vivant, porteurs, evenements.
  std::vector<Key3> events;
  u64 anchors_seen = 0, carriers_seen = 0, shell_refused = 0;
  std::vector<i32> lens;
  for (const WspdRect& r : alive) {
    const NodeRange ra = range_of(ix, r.a);
    const NodeRange rb = range_of(ix, r.b);
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        ++anchors_seen;
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        lens_query(ix, pa, pb, D2, &lens);
        for (const i32 ux : lens) {
          if (ux == ua || ux == ub) continue;
          const P3& px = ix.upos[(size_t)ux];
          // Acuite stricte en x : V² > D² avec V = 2x-a-b.
          const P3 v{2 * px.x - pa.x - pb.x, 2 * px.y - pa.y - pb.y,
                     2 * px.z - pa.z - pb.z};
          if (p3_norm2(v) <= D2) continue;
          const i64 l_ax = p3_norm2(p3_sub(px, pa));
          const i64 l_bx = p3_norm2(p3_sub(px, pb));
          if (!anchor_owns_q3(D2, l_ax, l_bx, (PointId)ua, (PointId)ub,
                              (PointId)ux))
            continue;
          ++carriers_seen;
          const Q3Form f = q3_form(pa, pb, px);
          u64 shell = 0;
          const u64 depth = q3_ball_depth(ix, f, ua, ub, ux, h3, &shell);
          if (depth >= h3) continue;
          if (shell > 0) {
            ++shell_refused;  // refus transactionnel (position generale)
            continue;
          }
          events.push_back(make_key(ua, ub, ux));
        }
      }
  }
  std::sort(events.begin(), events.end());
  events.erase(std::unique(events.begin(), events.end()), events.end());
  const auto t2 = std::chrono::steady_clock::now();

  // 3. Juge par identites (oracle borne).
  u64 missing = 0, extra = 0;
  if (a.judge) {
    std::vector<Key3> truth;
    const int m = ix.unique_count();
    for (i32 i = 0; i < m; ++i)
      for (i32 j = i + 1; j < m; ++j)
        for (i32 k = j + 1; k < m; ++k) {
          const P3 &p1 = ix.upos[(size_t)i], &p2 = ix.upos[(size_t)j],
                   &p3v = ix.upos[(size_t)k];
          const i64 l12 = p3_norm2(p3_sub(p2, p1));
          const i64 l13 = p3_norm2(p3_sub(p3v, p1));
          const i64 l23 = p3_norm2(p3_sub(p3v, p2));
          // Owner brut : arete de longueur max, tie par EdgeKey minimale.
          struct E { i64 l; PointId x, y; i32 apex; };
          E es[3] = {{l12, (PointId)i, (PointId)j, k},
                     {l13, (PointId)i, (PointId)k, j},
                     {l23, (PointId)j, (PointId)k, i}};
          int best = 0;
          for (int e = 1; e < 3; ++e) {
            if (es[e].l > es[best].l ||
                (es[e].l == es[best].l &&
                 edge_key_less(edge_key(es[e].x, es[e].y),
                               edge_key(es[best].x, es[best].y))))
              best = e;
          }
          const P3& oa = ix.upos[(size_t)es[best].x];
          const P3& ob = ix.upos[(size_t)es[best].y];
          const P3& ox = ix.upos[(size_t)es[best].apex];
          const P3 vv{2 * ox.x - oa.x - ob.x, 2 * ox.y - oa.y - ob.y,
                      2 * ox.z - oa.z - ob.z};
          if (p3_norm2(vv) <= es[best].l) continue;  // pas strictement aigu
          const Q3Form f = q3_form(oa, ob, ox);
          u64 shell = 0;
          const u64 depth = q3_ball_depth(ix, f, (i32)es[best].x, (i32)es[best].y,
                                          es[best].apex, h3, &shell);
          if (depth >= h3 || shell > 0) continue;
          truth.push_back(make_key(i, j, k));
        }
    std::sort(truth.begin(), truth.end());
    std::vector<Key3> diff;
    std::set_difference(truth.begin(), truth.end(), events.begin(), events.end(),
                        std::back_inserter(diff));
    missing = diff.size();
    diff.clear();
    std::set_difference(events.begin(), events.end(), truth.begin(), truth.end(),
                        std::back_inserter(diff));
    extra = diff.size();
  }
  const auto t3 = std::chrono::steady_clock::now();

  const auto ms = [](auto d) {
    return (double)std::chrono::duration_cast<std::chrono::microseconds>(d).count() /
           1000.0;
  };
  std::printf(
      "famille=%s n=%d coord=%d s=%lld smax=%llu seed=%lld rect_vivants_q3=%zu "
      "ancres_vues=%llu porteurs=%llu evenements=%zu ev_par_point=%.1f "
      "shell_refus=%llu juge_manquants=%llu juge_en_trop=%llu "
      "t_wspd_ms=%.1f t_instruction_ms=%.1f t_juge_ms=%.1f\n",
      cloud_family_name(a.family), a.n, coord, (long long)a.s,
      (unsigned long long)smax_eff, a.seed, alive.size(),
      (unsigned long long)anchors_seen, (unsigned long long)carriers_seen,
      events.size(), (double)events.size() / (double)pts.size(),
      (unsigned long long)shell_refused, (unsigned long long)missing,
      (unsigned long long)extra, ms(t1 - t0), ms(t2 - t1), ms(t3 - t2));

  if (a.judge && (missing > 0 || extra > 0)) return 1;
  if (events.size() < a.min_events) {
    std::fprintf(stderr, "PLANCHER : %zu evenements < %llu\n", events.size(),
                 (unsigned long long)a.min_events);
    return 3;
  }
  return 0;
}
