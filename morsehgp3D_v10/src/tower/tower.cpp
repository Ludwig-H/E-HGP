#include "tower/tower.hpp"

#include <algorithm>
#include <atomic>
#include <memory>
#include <unordered_map>

#include "catalogue/support.hpp"

namespace mhgp10 {

namespace {

using geom::P3;

constexpr u32 kMaxFacet = kMaxCatalogueOrder;
constexpr u64 kMaxSubsets = 20000;  // quotient local d'une coquille etendue : au-dela, refus explicite

struct Facet {
  u32 n = 0;
  std::array<u32, kMaxFacet> s{};
  void sort() {  // insertion (n <= 12) : evite le faux positif -Warray-bounds de std::sort sur un prefixe
    for (u32 i = 1; i < n; ++i) {
      const u32 v = s[i];
      u32 j = i;
      while (j > 0 && s[j - 1] > v) {
        s[j] = s[j - 1];
        --j;
      }
      s[j] = v;
    }
  }
};

struct FacetHash {
  size_t operator()(const std::array<u32, kMaxFacet>& a) const {
    u64 h = 1469598103934665603ull;
    for (u32 v : a) h = (h ^ v) * 1099511628211ull;
    return static_cast<size_t>(h);
  }
};

std::array<u32, kMaxFacet> facet_key(const Facet& f) {
  std::array<u32, kMaxFacet> k;
  k.fill(kNone);
  for (u32 i = 0; i < f.n; ++i) k[i] = f.s[i];
  return k;
}

struct SupHash {
  size_t operator()(const std::array<u32, 4>& a) const {
    u64 h = 1469598103934665603ull;
    for (u32 v : a) h = (h ^ v) * 1099511628211ull;
    return static_cast<size_t>(h);
  }
};

struct Sphere {
  bool empty = true, zero = false;
  u32 anchor = kNone;
  geom::Center c{};
  geom::Level level{};
};

struct Geo {
  const Cloud& cloud;
  const SiteTree& tree;
  const Catalogue& cat;
  std::vector<P3> P;
  std::unordered_map<std::array<u32, 4>, u32, SupHash> lookup;
  const P3& operator()(u32 s) const { return P[s]; }
};

// ----------------------------------------------------------------- MEB exacte (Welzl)

Sphere through(const Geo& g, const u32* R, int nr) {
  Sphere S;
  if (nr == 0) return S;
  S.empty = false;
  S.anchor = R[0];
  if (nr == 1) {
    S.zero = true;
    S.c = geom::Center{{0, 0, 0}, 1};
    return S;
  }
  if (nr == 2) {
    geom::center2(g.P[R[0]], g.P[R[1]], S.c);
    S.level = geom::level2(g.P[R[0]], g.P[R[1]]);
    return S;
  }
  auto tri = [&](u32 a, u32 b, u32 d) {
    if (!geom::center3(g.P[a], g.P[b], g.P[d], S.c)) return false;
    S.anchor = a;
    S.level = geom::level3(g.P[a], g.P[b], g.P[d]);
    return true;
  };
  if (nr == 3) {
    if (!tri(R[0], R[1], R[2])) {  // alignes : ne peut pas arriver en arithmetique exacte (garde)
      geom::center2(g.P[R[0]], g.P[R[2]], S.c);
      S.level = geom::level2(g.P[R[0]], g.P[R[2]]);
    }
    return S;
  }
  if (geom::center4(g.P[R[0]], g.P[R[1]], g.P[R[2]], g.P[R[3]], S.c)) {
    S.level = geom::level4(S.c);
    return S;
  }
  // quatre points cocycliques : la sphere minimale passant par eux est celle du cercle
  for (int a = 0; a < 4; ++a)
    for (int b = a + 1; b < 4; ++b)
      for (int d = b + 1; d < 4; ++d)
        if (tri(R[a], R[b], R[d])) return S;
  return S;
}

bool contains(const Geo& g, const Sphere& S, u32 z) {
  if (S.empty) return false;
  if (S.zero) return z == S.anchor;
  return geom::side_key(S.c, g.P[S.anchor], g.P[z]) <= 0;
}

Sphere welzl(const Geo& g, const u32* F, int n, u32* R, int nr) {
  if (n == 0 || nr == 4) return through(g, R, nr);
  Sphere S = welzl(g, F, n - 1, R, nr);
  if (contains(g, S, F[n - 1])) return S;
  R[nr] = F[n - 1];
  return welzl(g, F, n - 1, R, nr + 1);
}

Sphere meb(const Geo& g, const Facet& F) {
  u32 R[4];
  return welzl(g, F.s.data(), static_cast<int>(F.n), R, 0);
}

// ----------------------------------------------------------------- structure locale

enum class Local { birth, inert, join, refused };

// Structure de la sphere (I, U, centre) a l'ordre k (t = k - |I|, 1 <= t <= |U|). regular : U est le
// support (affinement independant, centre dans l'interieur relatif). reps : un representant I u A par
// morceau (tous les morceaux si join, le premier sinon).
Local local_structure(const Geo& g, std::span<const u32> I, std::span<const u32> U, u32 anchor, const geom::Center& c,
                      u32 t, bool regular, std::vector<Facet>& reps) {
  reps.clear();
  const u32 m = static_cast<u32>(U.size());
  auto make = [&](const std::vector<u32>& A) {
    Facet f;
    for (u32 s : I) f.s[f.n++] = s;
    for (u32 s : A) f.s[f.n++] = s;
    f.sort();
    return f;
  };
  if (regular) {
    if (t == m) return Local::birth;
    if (t + 1 == m) {
      for (u32 j = 0; j < m; ++j) {
        std::vector<u32> A;
        for (u32 i = 0; i < m; ++i)
          if (i != j) A.push_back(U[i]);
        reps.push_back(make(A));
      }
      return Local::join;
    }
    reps.push_back(make(std::vector<u32>(U.begin(), U.begin() + t)));
    return Local::inert;
  }
  // coquille etendue : sous-ensembles de taille t (ordre lexicographique), separables par Gordan
  if (m > 24) return Local::refused;
  std::vector<u32> masks;
  std::vector<u32> idx(t);
  for (u32 i = 0; i < t; ++i) idx[i] = i;
  std::vector<u32> A(t);
  const P3& a = g.P[anchor];
  for (;;) {
    for (u32 i = 0; i < t; ++i) A[i] = U[idx[i]];
    if (!center_in_closed_hull(g, std::span<const u32>(A), a, c)) {
      u32 mask = 0;
      for (u32 i = 0; i < t; ++i) mask |= 1u << idx[i];
      masks.push_back(mask);
      if (masks.size() > kMaxSubsets) return Local::refused;
    }
    int pos = static_cast<int>(t) - 1;
    while (pos >= 0 && idx[pos] == m - t + static_cast<u32>(pos)) --pos;
    if (pos < 0) break;
    ++idx[pos];
    for (u32 i = pos + 1; i < t; ++i) idx[i] = idx[i - 1] + 1;
  }
  if (masks.empty()) return Local::birth;
  // morceaux : A ~ A' ssi A u A' separable
  const u32 ns = static_cast<u32>(masks.size());
  std::vector<u32> dsu(ns);
  for (u32 i = 0; i < ns; ++i) dsu[i] = i;
  auto find = [&](u32 x) {
    while (dsu[x] != x) x = dsu[x] = dsu[dsu[x]];
    return x;
  };
  std::vector<u32> B;
  for (u32 i = 0; i < ns; ++i)
    for (u32 j = i + 1; j < ns; ++j) {
      if (find(i) == find(j)) continue;
      const u32 un = masks[i] | masks[j];
      B.clear();
      for (u32 b = 0; b < m; ++b)
        if (un >> b & 1) B.push_back(U[b]);
      if (!center_in_closed_hull(g, std::span<const u32>(B), a, c)) dsu[std::max(find(i), find(j))] = std::min(find(i), find(j));
    }
  std::vector<u32> firsts;
  for (u32 i = 0; i < ns; ++i)
    if (find(i) == i) firsts.push_back(i);
  for (u32 i : firsts) {
    std::vector<u32> S;
    for (u32 b = 0; b < m; ++b)
      if (masks[i] >> b & 1) S.push_back(U[b]);
    reps.push_back(make(S));
    if (firsts.size() == 1) break;
  }
  return firsts.size() >= 2 ? Local::join : Local::inert;
}

// ----------------------------------------------------------------- un ordre

struct OrderCtx {
  const Geo& g;
  int k;
  std::vector<u32> birth_node;              // par boule : noeud de naissance a cet ordre (kNone sinon)
  std::vector<std::atomic<u32>> cell_min;   // par boule : naissance de la cellule (b, k) (memo)
  std::unordered_map<std::array<u32, kMaxFacet>, u32, FacetHash> seeds;  // sommet de naissance -> noeud
  std::atomic<u32> error{0};                // Reason + 1
  std::atomic<u64> steps{0}, memo_hits{0};
  explicit OrderCtx(const Geo& gg, int kk) : g(gg), k(kk), birth_node(gg.cat.balls(), kNone), cell_min(gg.cat.balls()) {
    for (auto& a : cell_min) a.store(kNone, std::memory_order_relaxed);
  }
  void set_error(Reason r) {
    u32 expected = 0;
    error.compare_exchange_strong(expected, static_cast<u32>(r) + 1);
  }
};

struct Scratch {
  std::vector<std::pair<i128, u32>> near;
  std::vector<u32> I, U;
  std::vector<Facet> reps;
  std::vector<u32> pend;
};

// Descente d'une k-partie vers un noeud de naissance.
u32 resolve(OrderCtx& o, Facet F, Scratch& sc) {
  const Geo& g = o.g;
  const u32 k = static_cast<u32>(o.k);
  sc.pend.clear();
  geom::Level prev{};
  bool has_prev = false;
  u32 node = kNone;
  for (;;) {
    o.steps.fetch_add(1, std::memory_order_relaxed);
    if (k == 1) {
      node = F.s[0];
      break;
    }
    if (const auto hit = o.seeds.find(facet_key(F)); hit != o.seeds.end()) {  // semis : sommet de naissance
      node = hit->second;
      break;
    }
    const Sphere S = meb(g, F);
    if (has_prev && geom::compare(S.level, prev) >= 0) {
      o.set_error(Reason::descent_no_terminal);
      return kNone;
    }
    prev = S.level;
    has_prev = true;
    const P3& a = g.P[S.anchor];
    g.tree.nearest(a, S.c, k + 1, sc.near);
    if (sc.near.size() >= k && sc.near[k - 1].first < 0) {  // au moins k sites strictement interieurs
      F.n = k;
      for (u32 i = 0; i < k; ++i) F.s[i] = sc.near[i].second;
      F.sort();
      continue;
    }
    g.tree.closed_ball(a, S.c, sc.I, sc.U);
    std::array<u32, 4> sup;
    u8 q = 0;
    if (!canonical_support(g, std::span<const u32>(sc.U), a, S.c, sup, q)) {
      o.set_error(Reason::descent_no_terminal);
      return kNone;
    }
    const u32 p = static_cast<u32>(sc.I.size());
    const u32 m = static_cast<u32>(sc.U.size());
    const auto it = g.lookup.find(sup);
    const u32 b = it == g.lookup.end() ? kNone : it->second;
    if (b != kNone && k + 1 >= p + q && k <= p + m) {
      if (o.birth_node[b] != kNone) {
        node = o.birth_node[b];
        break;
      }
      const u32 memo = o.cell_min[b].load(std::memory_order_relaxed);
      if (memo != kNone) {
        o.memo_hits.fetch_add(1, std::memory_order_relaxed);
        node = memo;
        break;
      }
      sc.pend.push_back(b);
    }
    const Local L = local_structure(g, sc.I, sc.U, S.anchor, S.c, k - p, m == q, sc.reps);
    if (L == Local::refused) {
      o.set_error(Reason::shell_quotient_budget);
      return kNone;
    }
    if (L == Local::birth) {  // une naissance absente de la table : catalogue incomplet
      o.set_error(Reason::census_mismatch);
      return kNone;
    }
    F = sc.reps[0];
  }
  for (u32 b : sc.pend) o.cell_min[b].store(node, std::memory_order_relaxed);
  return node;
}

struct Join {
  u32 rank;
  u32 ball;
  std::vector<Facet> reps;
  std::vector<u32> nodes;
};

u32 rank_at_most(const Catalogue& cat, u64 e) {
  // plus grand rang r (decale : 0 = niveau nul) tel que niveau(r) <= e
  if (e == 0) return 0;
  geom::Level L;
  L.num = arith::I192::from_u128(e);
  L.den = arith::I128w::from_u128(1);
  u32 lo = 0, hi = static_cast<u32>(cat.level.size());  // nombre de niveaux <= e
  while (lo < hi) {
    const u32 mid = (lo + hi) / 2;
    if (geom::compare(cat.level[mid], L) <= 0) lo = mid + 1;
    else hi = mid;
  }
  return lo;  // rang decale du dernier niveau <= e
}

Outcome build_order(const Geo& g, int k, const TowerParams& params, sched::Pool& pool, OrderForest& out,
                    std::unique_ptr<OrderCtx>& keep) {
  const Catalogue& cat = g.cat;
  const Cloud& cloud = g.cloud;
  keep = std::make_unique<OrderCtx>(g, k);
  OrderCtx& o = *keep;
  out.k = k;
  // naissances et jonctions
  struct Birth {
    u32 rank, ref;
  };
  std::vector<Birth> births;
  std::vector<Join> joins;
  if (k == 1)
    for (u32 s = 0; s < cloud.sites(); ++s) births.push_back({0, s});
  {
    std::vector<Facet> reps;
    for (u32 b = 0; b < cat.balls(); ++b) {
      const u32 p = cat.p[b];
      const auto I = cat.interior(b);
      const auto U = cat.shell(b);
      const u32 m = static_cast<u32>(U.size());
      if (!(p < u32(k) && u32(k) <= p + m)) continue;
      if (u32(k) + 1 < p + cat.qmin[b]) continue;  // sous la fenetre : inerte
      const geom::Center c = ball_center(cloud, cat, b);
      const Local L = local_structure(g, I, U, cat.support[b][0], c, u32(k) - p, !(cat.flags[b] & kExtendedShell), reps);
      if (L == Local::refused) return fail(Reason::shell_quotient_budget, u8(k));
      if (L == Local::birth) births.push_back({cat.rank[b] + 1, b});
      else if (L == Local::join) joins.push_back({cat.rank[b] + 1, b, reps, {}});
    }
  }
  // numerotation des naissances (ordre rang, puis boule / site)
  std::sort(births.begin(), births.end(), [](const Birth& x, const Birth& y) {
    return x.rank != y.rank ? x.rank < y.rank : x.ref < y.ref;
  });
  const u32 nb = static_cast<u32>(births.size());
  if (k > 1) {
    o.seeds.reserve(nb * 2);
    for (u32 i = 0; i < nb; ++i) {
      const u32 b = births[i].ref;
      o.birth_node[b] = i;
      if (!(cat.flags[b] & kExtendedShell) && cat.pop_off[b + 1] - cat.pop_off[b] == u64(k)) {
        Facet f;  // sommet de naissance regulier : la boule fermee I u U, exactement k sites
        for (u32 s2 : cat.interior(b)) f.s[f.n++] = s2;
        for (u32 s2 : cat.shell(b)) f.s[f.n++] = s2;
        f.sort();
        o.seeds.emplace(facet_key(f), i);
      }
    }
  }
  // resolution des representants (parallele, pure a memo pres)
  std::vector<Scratch> scratch(pool.size());
  pool.parallel_for(joins.size(), 64, [&](u64 b, u64 e, unsigned wk) {
    for (u64 j = b; j < e; ++j) {
      Join& J = joins[j];
      J.nodes.clear();
      for (const Facet& f : J.reps) J.nodes.push_back(resolve(o, f, scratch[wk]));
    }
  });
  if (o.error.load()) return fail(static_cast<Reason>(o.error.load() - 1), u8(k));
  out.joins = joins.size();
  out.births = nb;
  // Kruskal par plateaux
  out.rank.clear();
  out.birth.clear();
  for (const Birth& bt : births) {
    out.rank.push_back(bt.rank);
    out.birth.push_back(bt.ref);
  }
  out.parent.assign(nb, kNone);
  out.child_off.assign(nb + 1, 0);
  out.child_val.clear();
  std::vector<u32> dsu(nb), top(nb);
  for (u32 i = 0; i < nb; ++i) dsu[i] = top[i] = i;
  auto find = [&](u32 x) {
    while (dsu[x] != x) x = dsu[x] = dsu[dsu[x]];
    return x;
  };
  std::sort(joins.begin(), joins.end(), [](const Join& x, const Join& y) {
    return x.rank != y.rank ? x.rank < y.rank : x.ball < y.ball;
  });
  std::vector<std::pair<u32, u32>> members;
  for (size_t i = 0; i < joins.size();) {
    size_t j = i;
    while (j < joins.size() && joins[j].rank == joins[i].rank) ++j;
    const u32 rk = joins[i].rank;
    // racines pre-lot de chaque representant, puis unions du lot
    std::vector<std::vector<u32>> pre(j - i);
    for (size_t t = i; t < j; ++t)
      for (u32 nd : joins[t].nodes) pre[t - i].push_back(find(nd));
    for (auto& roots : pre)
      for (size_t r = 1; r < roots.size(); ++r) {
        const u32 x = find(roots[0]), y = find(roots[r]);
        if (x != y) dsu[std::max(x, y)] = std::min(x, y);
      }
    members.clear();
    for (auto& roots : pre)
      for (u32 r : roots) members.push_back({find(r), r});
    std::sort(members.begin(), members.end());
    members.erase(std::unique(members.begin(), members.end()), members.end());
    for (size_t a = 0; a < members.size();) {
      size_t b = a;
      while (b < members.size() && members[b].first == members[a].first) ++b;
      if (b - a >= 2) {
        const u32 node = static_cast<u32>(out.rank.size());
        out.rank.push_back(rk);
        out.birth.push_back(kNone);
        out.parent.push_back(kNone);
        std::vector<u32> kids;
        for (size_t t = a; t < b; ++t) kids.push_back(top[members[t].second]);
        std::sort(kids.begin(), kids.end());
        for (u32 kid : kids) {
          out.child_val.push_back(kid);
          out.parent[kid] = node;
        }
        out.child_off.push_back(static_cast<u32>(out.child_val.size()));
        top[members[a].first] = node;
        ++out.merges;
      }
      a = b;
    }
    i = j;
  }
  u32 roots = 0;
  for (u32 v = 0; v < out.rank.size(); ++v) roots += out.parent[v] == kNone;
  if (roots != 1) return fail(Reason::root_count, u8(k));
  out.descent_steps = o.steps.load();
  out.memo_hits = o.memo_hits.load();
  // attaches C n X
  if (params.points) {
    const u32 n = cloud.sites();
    out.point_node.assign(n, kNone);
    out.point_level.assign(n, 0);
    pool.parallel_for(n, 64, [&](u64 b, u64 e, unsigned wk) {
      Scratch& sc = scratch[wk];
      for (u64 x = b; x < e; ++x) {
        const P3& px = g.P[x];
        g.tree.nearest(px, geom::Center{{0, 0, 0}, 1}, u32(k), sc.near);
        Facet F;
        F.n = u32(k);
        for (u32 i = 0; i < u32(k); ++i) F.s[i] = sc.near[i].second;
        F.sort();
        const u64 lev = static_cast<u64>(sc.near[k - 1].first);
        u32 v = resolve(o, F, sc);
        if (v == kNone) return;
        const u32 r = rank_at_most(cat, lev);
        while (out.parent[v] != kNone && out.rank[out.parent[v]] <= r) v = out.parent[v];
        out.point_node[x] = v;
        out.point_level[x] = lev;
      }
    });
    if (o.error.load()) return fail(static_cast<Reason>(o.error.load() - 1), u8(k));
    out.descent_steps = o.steps.load();
    out.memo_hits = o.memo_hits.load();
  }
  return Outcome{};
}

// Cartes verticales de l'ordre k (up) vers l'ordre k - 1 (down). Naissance de la boule b au niveau a : une
// (k-1)-partie de sa boule fermee est realisee au centre, donc sa descente a l'ordre k - 1 donne une naissance
// de la composante de L_{k-1}(a) qui contient la composante nee ; l'image est l'ancetre vivant au niveau a.
// Fusion : image = ancetre au niveau de la fusion de l'image d'un enfant ; tous les enfants doivent s'accorder
// (naturalite), sinon invariant_violated.
Outcome build_verticals(const Geo& g, OrderForest& up, const OrderForest& down, OrderCtx& octx, sched::Pool& pool) {
  const Catalogue& cat = g.cat;
  const u32 nn = static_cast<u32>(up.rank.size());
  up.lower.assign(nn, kNone);
  auto ancestor = [&](u32 v, u32 r) {
    while (down.parent[v] != kNone && down.rank[down.parent[v]] <= r) v = down.parent[v];
    return v;
  };
  const u32 km1 = static_cast<u32>(octx.k);
  std::vector<Scratch> scratch(pool.size());
  std::atomic<u32> error{0};
  // naissances (independantes)
  pool.parallel_for(nn, 64, [&](u64 b0, u64 e0, unsigned wk) {
    for (u64 v = b0; v < e0; ++v) {
      const u32 ball = up.birth[v];
      if (ball == kNone) continue;
      Facet F;
      if (up.k == 2) {  // naissance d'ordre 2 : une (k-1)-partie est un site de la boule fermee
        F.n = 1;
        F.s[0] = cat.support[ball][0];
      } else {
        for (u32 s2 : cat.interior(ball))
          if (F.n < km1) F.s[F.n++] = s2;
        for (u32 s2 : cat.shell(ball))
          if (F.n < km1) F.s[F.n++] = s2;
        F.sort();
      }
      const u32 m = resolve(octx, F, scratch[wk]);
      if (m == kNone) {
        error.store(1);
        return;
      }
      up.lower[v] = ancestor(m, up.rank[v]);
    }
  });
  if (error.load() || octx.error.load()) return fail(Reason::descent_no_terminal, u8(up.k));
  // fusions : dans l'ordre de creation (enfants avant parents)
  for (u32 v = 0; v < nn; ++v) {
    if (up.birth[v] != kNone) continue;
    u32 image = kNone;
    for (u32 j = up.child_off[v]; j < up.child_off[v + 1]; ++j) {
      const u32 c = ancestor(up.lower[up.child_val[j]], up.rank[v]);
      if (image == kNone) image = c;
      else if (image != c) return fail(Reason::vertical_naturality, u8(up.k));
    }
    up.lower[v] = image;
  }
  return Outcome{};
}

}  // namespace

Result<Tower> build_tower(const Cloud& cloud, const SiteTree& tree, const Catalogue& cat, const TowerParams& params,
                          sched::Pool& pool) {
  if (params.kmax < 1 || params.kmax > cat.kmax) return fail(Reason::kmax_out_of_range);
  for (u32 s = 0; s < cloud.sites(); ++s)
    if (cloud.w[s] != 1) return fail(Reason::shell_quotient_budget);  // multiplicites : semantique a venir
  Geo g{cloud, tree, cat, {}, {}};
  g.P.resize(cloud.sites());
  for (u32 s = 0; s < cloud.sites(); ++s) g.P[s] = P3{i64(cloud.x[s]), i64(cloud.y[s]), i64(cloud.z[s])};
  g.lookup.reserve(cat.balls() * 2);
  for (u32 b = 0; b < cat.balls(); ++b) g.lookup.emplace(cat.support[b], b);
  Tower t;
  t.kmax = std::min<int>(params.kmax, static_cast<int>(cloud.sites()));
  t.orders.resize(t.kmax);
  std::vector<std::unique_ptr<OrderCtx>> ctx(t.kmax + 1);
  Outcome worst;
  for (int k = 1; k <= t.kmax; ++k) {
    if (params.only_order > 0 && k != params.only_order) continue;
    const Outcome o = build_order(g, k, params, pool, t.orders[k - 1], ctx[k]);
    if (!o.ok() && (worst.ok() || o.precedes(worst))) worst = o;
  }
  if (!worst.ok()) return worst;
  if (params.verticals && params.only_order == 0)
    for (int k = 2; k <= t.kmax; ++k) {
      const Outcome o = build_verticals(g, t.orders[k - 1], t.orders[k - 2], *ctx[k - 1], pool);
      if (!o.ok()) return o;
    }
  return t;
}

PointDendrogram point_dendrogram(const Catalogue& cat, const OrderForest& f, const Cloud& cloud) {
  // table de niveaux fusionnee : niveaux des noeuds (rangs du catalogue) et niveaux d'entree (entiers)
  struct Key {
    bool from_cat;
    u32 rank;
    u64 e;
  };
  auto level_of = [&](const Key& k) -> geom::Level {
    if (k.from_cat) {
      if (k.rank == 0) return geom::Level{arith::I192{}, arith::I128w::from_u128(1)};
      return cat.level[k.rank - 1];
    }
    geom::Level L;
    L.num = arith::I192::from_u128(k.e);
    L.den = arith::I128w::from_u128(1);
    return L;
  };
  std::vector<Key> keys;
  for (u32 r : f.rank) keys.push_back({true, r, 0});
  for (u64 e : f.point_level) keys.push_back({false, 0, e});
  std::vector<geom::Level> lv;
  lv.reserve(keys.size());
  for (const Key& k : keys) lv.push_back(level_of(k));
  std::vector<u32> order(keys.size());
  for (u32 i = 0; i < order.size(); ++i) order[i] = i;
  std::sort(order.begin(), order.end(), [&](u32 a, u32 b) {
    const double x = lv[a].approx(), y = lv[b].approx();
    if (x != y && std::abs(x - y) > 1e-9 * std::max(x, y)) return x < y;
    return geom::compare(lv[a], lv[b]) < 0;
  });
  std::vector<u32> merged(keys.size());
  PointDendrogram d;
  for (u32 i = 0; i < order.size(); ++i) {
    if (i == 0 || geom::compare(lv[order[i - 1]], lv[order[i]]) != 0) d.level.push_back(lv[order[i]].approx());
    merged[order[i]] = static_cast<u32>(d.level.size() - 1);
  }
  const u32 nn = static_cast<u32>(f.rank.size());
  d.node_rank.resize(nn);
  for (u32 v = 0; v < nn; ++v) d.node_rank[v] = merged[v];
  d.parent = f.parent;
  d.child_off.assign(nn + 1, 0);
  // CSR des enfants depuis les parents (les enfants d'une fusion sont crees avant elle)
  std::vector<u32> cnt(nn, 0);
  for (u32 v = 0; v < nn; ++v)
    if (f.parent[v] != kNone) ++cnt[f.parent[v]];
  for (u32 v = 0; v < nn; ++v) d.child_off[v + 1] = d.child_off[v] + cnt[v];
  d.child_val.resize(d.child_off[nn]);
  std::vector<u32> fill(d.child_off.begin(), d.child_off.end() - 1);
  for (u32 v = 0; v < nn; ++v)
    if (f.parent[v] != kNone) d.child_val[fill[f.parent[v]]++] = v;
  const u32 n = cloud.sites();
  d.point_node = f.point_node;
  d.point_rank.resize(n);
  d.point_weight.assign(n, 1);
  for (u32 x = 0; x < n; ++x) d.point_rank[x] = merged[nn + x];
  return d;
}

}  // namespace mhgp10
