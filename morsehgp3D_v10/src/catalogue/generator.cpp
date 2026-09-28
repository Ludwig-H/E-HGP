// Generateur du catalogue par boites de centres. Voir catalogue.hpp pour l'objet et les references.
//
// Repere de l'arbre : sites mis a l'echelle X = x << kT (kT = 6 bits sous-unitaires), boites entieres
// demi-ouvertes [lo, hi). En u18 : |X| < 2^24, tests de gardes, dominance et bissectrices en i64 ;
// test droite des centres (lemme Z, zonogone) en i128 ; centres et recensement en i128 (geometry.hpp).
#include <algorithm>
#include <cmath>
#include <memory>
#include <mutex>

#include "catalogue/catalogue.hpp"

namespace mhgp10 {

namespace {

constexpr int kT = 6;
constexpr int kStagnationLimit = 3;

using geom::P3;

struct Box {
  i64 lo[3], hi[3];
};

struct Rec {
  std::array<u32, 4> sup;
  u8 q, flags;
  u32 p, u, n_i;
  u64 pop_begin;
  u32 pop_len;
  geom::Level level;
};

struct Local {
  std::vector<Rec> recs;
  std::vector<u32> pop;
  CatalogueLedger led;
  Outcome fail;
  // tampons reutilises
  std::vector<std::array<u32, 4>> memo;
  std::vector<u32> shell, interior;
  std::vector<unsigned char> meets;
};

struct Ctx {
  const Cloud& cloud;
  std::vector<P3> P;       // coordonnees entieres
  std::vector<P3> X;       // coordonnees mises a l'echelle
  std::vector<i64> X2;     // |X|^2
  std::vector<u32> w;
  int K;
  u32 M, max_leaf;
};

inline void merge_ledger(CatalogueLedger& a, const CatalogueLedger& b) {
  a.nodes += b.nodes;
  a.leaves += b.leaves;
  a.skipped_bbox += b.skipped_bbox;
  a.sum_m += b.sum_m;
  a.max_m = std::max(a.max_m, b.max_m);
  a.guard_tests += b.guard_tests;
  a.dominance_tests += b.dominance_tests;
  a.pair_tests += b.pair_tests;
  a.triple_tests += b.triple_tests;
  a.line_hits += b.line_hits;
  a.quad_tests += b.quad_tests;
  a.judged += b.judged;
  a.emitted += b.emitted;
  a.extended += b.extended;
  a.weighted += b.weighted;
  a.max_shell = std::max(a.max_shell, b.max_shell);
  a.stalled_leaves += b.stalled_leaves;
}

// Centre rationnel (anchor + N/D) dans la boite demi-ouverte : lo D <= 2^T (a D + N) < hi D.
inline bool center_in_box(const P3& a, const geom::Center& c, const Box& Q) {
  const i64 av[3] = {a.x, a.y, a.z};
  for (int i = 0; i < 3; ++i) {
    const i128 v = (i128(av[i]) * c.D + c.N[i]) * (i128(1) << kT);
    if (v < i128(Q.lo[i]) * c.D || v >= i128(Q.hi[i]) * c.D) return false;
  }
  return true;
}

// Plan bissecteur de (i, j) rencontre la boite fermee.
inline bool bisector_meets(const Ctx& C, u32 i, u32 j, const Box& Q) {
  const P3& a = C.X[i];
  const P3& b = C.X[j];
  const i64 d[3] = {a.x - b.x, a.y - b.y, a.z - b.z};
  const i64 base = C.X2[i] - C.X2[j];
  i64 smax = 0, smin = 0;
  for (int k = 0; k < 3; ++k) {
    smax += (d[k] > 0 ? Q.hi[k] : Q.lo[k]) * d[k];
    smin += (d[k] > 0 ? Q.lo[k] : Q.hi[k]) * d[k];
  }
  return base - 2 * smax <= 0 && 0 <= base - 2 * smin;
}

// Lemme Z : la droite des centres equidistants de (a, b, d) (non alignes) rencontre la boite fermee.
inline bool center_line_meets(const Ctx& C, u32 ia, u32 ib, u32 id, const Box& Q) {
  const P3& A = C.X[ia];
  const P3& B = C.X[ib];
  const P3& Dp = C.X[id];
  const i64 u[3] = {A.x - B.x, A.y - B.y, A.z - B.z};
  const i64 v[3] = {A.x - Dp.x, A.y - Dp.y, A.z - Dp.z};
  // 2 F(centre de boite) = 2(|A|^2 - |B|^2) - 2 (lo + hi) . (A - B), idem pour D.
  i128 f0 = 2 * i128(C.X2[ia] - C.X2[ib]), f1 = 2 * i128(C.X2[ia] - C.X2[id]);
  i128 g[3][2];
  for (int k = 0; k < 3; ++k) {
    const i128 s = Q.lo[k] + Q.hi[k];
    f0 -= 2 * s * u[k];
    f1 -= 2 * s * v[k];
    const i128 h = Q.hi[k] - Q.lo[k];
    g[k][0] = -2 * h * u[k];
    g[k][1] = -2 * h * v[k];
  }
  for (int k = 0; k < 3; ++k) {
    if (g[k][0] == 0 && g[k][1] == 0) continue;
    const i128 n0 = -g[k][1], n1 = g[k][0];
    i128 lhs = n0 * f0 + n1 * f1;
    if (lhs < 0) lhs = -lhs;
    i128 rhs = 0;
    for (int j = 0; j < 3; ++j) {
      i128 t = n0 * g[j][0] + n1 * g[j][1];
      rhs += t < 0 ? -t : t;
    }
    if (lhs > rhs) return false;
  }
  return true;
}

// q_min et support canonique d'une coquille etendue (sites tries), centre (anchor + ctr).
void canonical_support(const Ctx& C, const std::vector<u32>& sh, const P3& anchor, const geom::Center& ctr,
                       const std::array<u32, 4>& gen, u8 qgen, std::array<u32, 4>& sup, u8& q) {
  const u32 m = static_cast<u32>(sh.size());
  sup = {kNone, kNone, kNone, kNone};
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j)
      if (geom::is_midpoint(C.P[sh[i]], C.P[sh[j]], anchor, ctr)) {
        sup = {sh[i], sh[j], kNone, kNone};
        q = 2;
        return;
      }
  if (qgen >= 3)
    for (u32 i = 0; i < m; ++i)
      for (u32 j = i + 1; j < m; ++j)
        for (u32 k = j + 1; k < m; ++k) {
          const P3 &a = C.P[sh[i]], &b = C.P[sh[j]], &c = C.P[sh[k]];
          const P3 cr = geom::cross(geom::sub(b, a), geom::sub(c, a));
          if (cr.x == 0 && cr.y == 0 && cr.z == 0) continue;
          if (!geom::acute(a, b, c)) continue;
          if (!geom::center_in_plane(a, b, c, anchor, ctr)) continue;
          sup = {sh[i], sh[j], sh[k], kNone};
          q = 3;
          return;
        }
  if (qgen >= 4)
    for (u32 i = 0; i < m; ++i)
      for (u32 j = i + 1; j < m; ++j)
        for (u32 k = j + 1; k < m; ++k)
          for (u32 l = k + 1; l < m; ++l) {
            const P3* t[4] = {&C.P[sh[i]], &C.P[sh[j]], &C.P[sh[k]], &C.P[sh[l]]};
            if (geom::orient(*t[0], *t[1], *t[2], *t[3]) == 0) continue;
            if (!geom::strictly_inside_tetra(t, anchor, ctr)) continue;
            sup = {sh[i], sh[j], sh[k], sh[l]};
            q = 4;
            return;
          }
  sup = gen;  // la presentation generatrice est un support (ne devrait pas arriver)
  q = qgen;
}

// Juge une sphere candidate : recensement exact sur la liste de la feuille, admission, emission.
void judge(const Ctx& C, Local& L, const std::vector<u32>& cand, u32 anchor_site, const geom::Center& ctr,
           const std::array<u32, 4>& gen, u8 qgen) {
  ++L.led.judged;
  const P3& a = C.P[anchor_site];
  u32 p = 0;
  L.shell.clear();
  L.interior.clear();
  for (u32 z : cand) {
    const int s = geom::side(ctr, a, C.P[z]);
    if (s < 0) {
      p += C.w[z];
      if (p >= static_cast<u32>(C.K)) return;  // toute admission exige p <= K - 1
      L.interior.push_back(z);
    } else if (s == 0) {
      L.shell.push_back(z);
    }
  }
  std::array<u32, 4> sup;
  u8 q = qgen;
  u8 flags = 0;
  if (L.shell.size() == qgen) {
    sup = gen;
  } else {
    flags |= kExtendedShell;
    canonical_support(C, L.shell, a, ctr, gen, qgen, sup, q);
    for (const auto& m : L.memo)
      if (m == sup) return;
    L.memo.push_back(sup);
  }
  u32 u = 0;
  for (u32 z : L.shell) {
    u += C.w[z];
    if (C.w[z] > 1) flags |= kWeightedShell;
  }
  const bool admit = (flags & kWeightedShell) ? p + 1 <= static_cast<u32>(C.K) : p + q <= static_cast<u32>(C.K) + 1;
  if (!admit) return;
  Rec r;
  r.sup = sup;
  r.q = q;
  r.flags = flags;
  r.p = p;
  r.u = u;
  r.n_i = static_cast<u32>(L.interior.size());
  r.pop_begin = L.pop.size();
  r.pop_len = static_cast<u32>(L.interior.size() + L.shell.size());
  L.pop.insert(L.pop.end(), L.interior.begin(), L.interior.end());
  L.pop.insert(L.pop.end(), L.shell.begin(), L.shell.end());
  if (qgen == 2) r.level = geom::level2(C.P[gen[0]], C.P[gen[1]]);
  else if (qgen == 3) r.level = geom::level3(C.P[gen[0]], C.P[gen[1]], C.P[gen[2]]);
  else r.level = geom::level4(ctr);
  L.recs.push_back(r);
  ++L.led.emitted;
  if (flags & kExtendedShell) ++L.led.extended;
  if (flags & kWeightedShell) ++L.led.weighted;
  L.led.max_shell = std::max<u64>(L.led.max_shell, L.shell.size());
}

void enumerate_leaf(const Ctx& C, Local& L, const std::vector<u32>& c, const Box& Q) {
  const u32 m = static_cast<u32>(c.size());
  L.memo.clear();
  L.meets.assign(size_t(m) * m, 0);
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j) {
      const unsigned char b = bisector_meets(C, c[i], c[j], Q) ? 1 : 0;
      L.meets[size_t(i) * m + j] = L.meets[size_t(j) * m + i] = b;
    }
  auto M = [&](u32 i, u32 j) { return L.meets[size_t(i) * m + j] != 0; };
  geom::Center ctr;
  // q2
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j) {
      if (!M(i, j)) continue;
      ++L.led.pair_tests;
      geom::center2(C.P[c[i]], C.P[c[j]], ctr);
      if (!center_in_box(C.P[c[i]], ctr, Q)) continue;
      judge(C, L, c, c[i], ctr, {c[i], c[j], kNone, kNone}, 2);
    }
  // q3 et q4
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j) {
      if (!M(i, j)) continue;
      for (u32 k = j + 1; k < m; ++k) {
        if (!M(i, k) || !M(j, k)) continue;
        const P3 &a = C.P[c[i]], &b = C.P[c[j]], &d = C.P[c[k]];
        const P3 cr = geom::cross(geom::sub(b, a), geom::sub(d, a));
        if (cr.x == 0 && cr.y == 0 && cr.z == 0) continue;
        ++L.led.triple_tests;
        if (!center_line_meets(C, c[i], c[j], c[k], Q)) continue;
        ++L.led.line_hits;
        if (geom::acute(a, b, d)) {
          geom::center3(a, b, d, ctr);
          if (center_in_box(a, ctr, Q)) judge(C, L, c, c[i], ctr, {c[i], c[j], c[k], kNone}, 3);
        }
        for (u32 l = k + 1; l < m; ++l) {
          if (!M(i, l) || !M(j, l) || !M(k, l)) continue;
          ++L.led.quad_tests;
          const P3& e = C.P[c[l]];
          if (!geom::center4(a, b, d, e, ctr)) continue;
          if (!center_in_box(a, ctr, Q)) continue;
          const P3* t[4] = {&a, &b, &d, &e};
          if (!geom::strictly_inside_tetra(t, a, ctr)) continue;
          judge(C, L, c, c[i], ctr, {c[i], c[j], c[k], c[l]}, 4);
        }
      }
    }
}

struct Task {
  Box box;
  std::shared_ptr<const std::vector<u32>> parent;
  int parent_stag;
};

// Filtre d'un noeud : liste certifiee de la boite a partir de la liste parente.
void filter_node(const Ctx& C, Local& L, const Box& Q, const std::vector<u32>& parent, std::vector<u32>& cand) {
  ++L.led.nodes;
  const i64 cq[3] = {Q.lo[0] + Q.hi[0], Q.lo[1] + Q.hi[1], Q.lo[2] + Q.hi[2]};
  const u32 np = static_cast<u32>(parent.size());
  const u32 pool_n = std::min<u32>(np, 3u * static_cast<u32>(C.K));
  std::vector<std::pair<i64, u32>> dd;
  dd.reserve(np);
  for (u32 s : parent) {
    const i64 dx = 2 * C.X[s].x - cq[0], dy = 2 * C.X[s].y - cq[1], dz = 2 * C.X[s].z - cq[2];
    dd.push_back({dx * dx + dy * dy + dz * dz, s});
  }
  if (pool_n < np) std::nth_element(dd.begin(), dd.begin() + pool_n, dd.end());
  std::sort(dd.begin(), dd.begin() + pool_n);
  // S0 : plus petit prefixe de poids >= K ; Y : prefixe de poids >= 3K (ou tout le reservoir).
  u32 s0 = 0;
  u64 acc = 0;
  while (s0 < pool_n && acc < u64(C.K)) acc += C.w[dd[s0++].second];
  if (acc < u64(C.K)) s0 = np;  // poids total < K : toutes les gardes (x se garde lui-meme)
  u32 ny = 0;
  acc = 0;
  while (ny < pool_n && acc < 3 * u64(C.K)) acc += C.w[dd[ny++].second];
  if (s0 == np) {
    std::sort(dd.begin(), dd.end());
  }
  cand.clear();
  for (u32 x : parent) {
    const P3& Xx = C.X[x];
    const i64 xx = C.X2[x];
    // gardes (lemme G)
    bool pass = false;
    for (u32 t = 0; t < s0 && !pass; ++t) {
      ++L.led.guard_tests;
      const u32 y = dd[t].second;
      const P3& Y = C.X[y];
      const i64 dx = Xx.x - Y.x, dy = Xx.y - Y.y, dz = Xx.z - Y.z;
      const i64 cx = dx > 0 ? Q.hi[0] : Q.lo[0], cy = dy > 0 ? Q.hi[1] : Q.lo[1], cz = dz > 0 ? Q.hi[2] : Q.lo[2];
      pass = xx - C.X2[y] - 2 * (cx * dx + cy * dy + cz * dz) <= 0;
    }
    if (!pass) continue;
    // dominateurs (lemme D)
    u64 dom = 0;
    bool dominated = false;
    for (u32 t = 0; t < ny; ++t) {
      ++L.led.dominance_tests;
      const u32 y = dd[t].second;
      const P3& Y = C.X[y];
      const i64 dx = Y.x - Xx.x, dy = Y.y - Xx.y, dz = Y.z - Xx.z;
      const i64 cx = dx > 0 ? Q.lo[0] : Q.hi[0], cy = dy > 0 ? Q.lo[1] : Q.hi[1], cz = dz > 0 ? Q.lo[2] : Q.hi[2];
      if (C.X2[y] - xx - 2 * (cx * dx + cy * dy + cz * dz) < 0) {
        dom += C.w[y];
        if (dom >= u64(C.K)) {
          dominated = true;
          break;
        }
      }
    }
    if (!dominated) cand.push_back(x);
  }
}

void process(const Ctx& C, Local& L, const Box& Q, const std::vector<u32>& parent, int parent_stag,
             std::vector<Task>* spill) {
  std::vector<u32> cand;
  filter_node(C, L, Q, parent, cand);
  // Lemme K (enveloppe, 3 directions) : aucun centre possible dans la boite.
  if (cand.empty()) {
    ++L.led.skipped_bbox;
    return;
  }
  for (int ax = 0; ax < 3; ++ax) {
    i64 lo = INT64_MAX, hi = INT64_MIN;
    for (u32 s : cand) {
      const i64 v = ax == 0 ? C.X[s].x : (ax == 1 ? C.X[s].y : C.X[s].z);
      lo = std::min(lo, v);
      hi = std::max(hi, v);
    }
    if (hi < Q.lo[ax] || lo >= Q.hi[ax]) {
      ++L.led.skipped_bbox;
      return;
    }
  }
  // Stagnation (liste qui ne decroit plus) : comptee seulement sous l'echelle de la grille (cote <= 2^kT,
  // un millimetre en u18), ou elle signale une sphere cospherique ; au-dessus, la decoupe continue toujours.
  const i64 side = Q.hi[0] - Q.lo[0];
  const int stag = (side <= (i64(1) << kT) && cand.size() == parent.size()) ? parent_stag + 1 : 0;
  if (cand.size() > C.M && side > 1 && stag < kStagnationLimit) {
    const i64 mid[3] = {(Q.lo[0] + Q.hi[0]) / 2, (Q.lo[1] + Q.hi[1]) / 2, (Q.lo[2] + Q.hi[2]) / 2};
    auto shared = spill ? std::make_shared<const std::vector<u32>>(cand) : nullptr;
    for (int o = 0; o < 8; ++o) {
      Box ch;
      for (int ax = 0; ax < 3; ++ax) {
        const bool up = (o >> ax) & 1;
        ch.lo[ax] = up ? mid[ax] : Q.lo[ax];
        ch.hi[ax] = up ? Q.hi[ax] : mid[ax];
      }
      if (spill) spill->push_back(Task{ch, shared, stag});
      else process(C, L, ch, cand, stag, nullptr);
    }
    return;
  }
  ++L.led.leaves;
  if (stag >= kStagnationLimit) ++L.led.stalled_leaves;
  L.led.sum_m += cand.size();
  L.led.max_m = std::max<u64>(L.led.max_m, cand.size());
  if (cand.size() > C.max_leaf) {
    const Outcome f = fail(Reason::wide_leaf);
    if (L.fail.ok()) L.fail = f;
    return;
  }
  enumerate_leaf(C, L, cand, Q);
}

}  // namespace

geom::Center ball_center(const Cloud& cloud, const Catalogue& cat, u32 b) {
  const auto& s = cat.support[b];
  auto pt = [&](u32 i) { return P3{i64(cloud.x[i]), i64(cloud.y[i]), i64(cloud.z[i])}; };
  geom::Center c{};
  if (cat.qmin[b] == 2) geom::center2(pt(s[0]), pt(s[1]), c);
  else if (cat.qmin[b] == 3) geom::center3(pt(s[0]), pt(s[1]), pt(s[2]), c);
  else geom::center4(pt(s[0]), pt(s[1]), pt(s[2]), pt(s[3]), c);
  return c;
}

Result<Catalogue> build_catalogue(const Cloud& cloud, const CatalogueParams& params, sched::Pool& pool) {
  if (params.kmax < 1 || params.kmax > kMaxCatalogueOrder) return fail(Reason::kmax_out_of_range);
  if (cloud.bits > kCoordinateBits) return fail(Reason::parameter_out_of_range);
  const u32 n = cloud.sites();
  Ctx C{cloud, {}, {}, {}, {}, params.kmax, params.leaf_size, params.max_leaf};
  if (C.M == 0) C.M = params.kmax <= 3 ? 12 : (params.kmax <= 6 ? 16 : (params.kmax <= 10 ? 24 : 32));
  C.P.resize(n);
  C.X.resize(n);
  C.X2.resize(n);
  C.w.resize(n);
  for (u32 s = 0; s < n; ++s) {
    C.P[s] = P3{i64(cloud.x[s]), i64(cloud.y[s]), i64(cloud.z[s])};
    C.X[s] = P3{C.P[s].x << kT, C.P[s].y << kT, C.P[s].z << kT};
    C.X2[s] = geom::dot(C.X[s], C.X[s]);
    C.w[s] = cloud.w[s];
  }
  Catalogue cat;
  cat.kmax = params.kmax;
  if (n < 2) {
    cat.pop_off.push_back(0);
    return cat;
  }
  // Phase sequentielle en largeur jusqu'a une frontiere assez large, puis taches paralleles.
  // Racine : cube dyadique minimal contenant les sites (tout centre critique est dans conv(X)).
  Box root;
  i64 lo[3] = {INT64_MAX, INT64_MAX, INT64_MAX}, ext = 0;
  for (u32 s = 0; s < n; ++s) {
    lo[0] = std::min(lo[0], C.X[s].x);
    lo[1] = std::min(lo[1], C.X[s].y);
    lo[2] = std::min(lo[2], C.X[s].z);
  }
  for (u32 s = 0; s < n; ++s)
    ext = std::max({ext, C.X[s].x - lo[0], C.X[s].y - lo[1], C.X[s].z - lo[2]});
  i64 side = 1;
  while (side <= ext) side <<= 1;
  for (int ax = 0; ax < 3; ++ax) {
    root.lo[ax] = lo[ax];
    root.hi[ax] = lo[ax] + side;
  }
  auto all = std::make_shared<std::vector<u32>>(n);
  for (u32 s = 0; s < n; ++s) (*all)[s] = s;
  std::vector<Task> frontier{Task{root, all, 0}};
  std::vector<Local> locals(pool.size());
  const size_t target = size_t(64) * pool.size();
  while (!frontier.empty() && frontier.size() < target) {
    std::vector<std::vector<Task>> spills(frontier.size());
    pool.parallel_for(frontier.size(), 1, [&](u64 b, u64 e, unsigned wk) {
      for (u64 i = b; i < e; ++i)
        process(C, locals[wk], frontier[i].box, *frontier[i].parent, frontier[i].parent_stag, &spills[i]);
    });
    std::vector<Task> next;
    for (auto& sp : spills)
      for (auto& t : sp) next.push_back(std::move(t));
    frontier.swap(next);
  }
  pool.parallel_for(frontier.size(), 1, [&](u64 b, u64 e, unsigned wk) {
    for (u64 i = b; i < e; ++i) process(C, locals[wk], frontier[i].box, *frontier[i].parent, frontier[i].parent_stag, nullptr);
  });
  frontier.clear();
  // Echec eventuel (feuille trop large) : refus transactionnel.
  for (const Local& L : locals)
    if (!L.fail.ok()) return L.fail;
  // Rassemblement et ordre canonique (niveau exact, S*).
  struct Ref {
    u32 local, rec;
    double approx;
  };
  std::vector<Ref> refs;
  for (u32 li = 0; li < locals.size(); ++li) {
    merge_ledger(cat.ledger, locals[li].led);
    for (u32 r = 0; r < locals[li].recs.size(); ++r) refs.push_back({li, r, locals[li].recs[r].level.approx()});
  }
  auto rec = [&](const Ref& x) -> const Rec& { return locals[x.local].recs[x.rec]; };
  std::sort(refs.begin(), refs.end(), [&](const Ref& x, const Ref& y) {
    if (x.approx != y.approx) return x.approx < y.approx;
    return rec(x).sup < rec(y).sup;
  });
  // reparation exacte des bandes flottantes (erreur relative de approx < 2^-50)
  for (size_t i = 0; i < refs.size();) {
    size_t j = i + 1;
    while (j < refs.size() && refs[j].approx - refs[j - 1].approx <= refs[j].approx * 0x1p-40) ++j;
    if (j - i > 1)
      std::sort(refs.begin() + i, refs.begin() + j, [&](const Ref& x, const Ref& y) {
        const int c = geom::compare(rec(x).level, rec(y).level);
        if (c != 0) return c < 0;
        return rec(x).sup < rec(y).sup;
      });
    i = j;
  }
  const u32 nb = static_cast<u32>(refs.size());
  cat.rank.resize(nb);
  cat.support.resize(nb);
  cat.qmin.resize(nb);
  cat.p.resize(nb);
  cat.u.resize(nb);
  cat.flags.resize(nb);
  cat.n_interior.resize(nb);
  cat.pop_off.resize(u64(nb) + 1);
  cat.pop_off[0] = 0;
  u64 total = 0;
  for (const Ref& x : refs) total += rec(x).pop_len;
  cat.pop.resize(total);
  u32 rank = 0;
  for (u32 b = 0; b < nb; ++b) {
    const Rec& r = rec(refs[b]);
    if (b > 0) {
      const Rec& prev = rec(refs[b - 1]);
      const int c = geom::compare(prev.level, r.level);
      if (c > 0) return fail(Reason::rank_order);
      if (c == 0 && prev.sup == r.sup) return fail(Reason::census_mismatch);  // boule emise deux fois
      if (c < 0) ++rank;
    }
    if (b == 0 || rank == cat.level.size()) cat.level.push_back(r.level);
    cat.rank[b] = rank;
    cat.support[b] = r.sup;
    cat.qmin[b] = r.q;
    cat.p[b] = r.p;
    cat.u[b] = r.u;
    cat.flags[b] = r.flags;
    cat.n_interior[b] = r.n_i;
    const std::vector<u32>& src = locals[refs[b].local].pop;
    std::copy(src.begin() + r.pop_begin, src.begin() + r.pop_begin + r.pop_len, cat.pop.begin() + cat.pop_off[b]);
    cat.pop_off[b + 1] = cat.pop_off[b] + r.pop_len;
  }
  return cat;
}

}  // namespace mhgp10
