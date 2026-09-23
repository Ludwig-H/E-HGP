// Auditeur C, 23 septembre 2026 — juge d'echantillon q3 independant du generateur (audit, hors produit).
//
// Cible : les boules q3 admissibles (p <= Kmax-2 interieurs stricts), dont la famille p = Kmax-2 est la zone
// que ni Euler ni la tour FULL ne jugent (README du dossier). Pour un site a tire, toute boule circonscrite a
// un triangle STRICTEMENT aigu (a,b,c) (centre dans l'interieur relatif, donc support positif, q_min <= 3)
// ayant p <= Kmax-2 interieurs stricts est admise : le catalogue DOIT contenir une boule de meme sphere
// (niveau exact egal et coquille contenant a,b,c), de meme coquille, de memes interieurs et d'arite coherente.
// Sens inverse (EXTRA) : toute boule reguliere a 3 sites d'arite 3 et p <= Kmax-2 dont la coquille contient un
// site tire doit etre retrouvee par l'enumeration (sinon elle est non critique ou mal recensee).
//
// Elagage exact (lemme de la demi-boule diametrale, preuve dans le README) : pour toute sphere B passant par a
// et b, de centre O, et m le milieu de ab, tout site strictement interieur a la boule diametrale D_ab tel que
// (y-m).(O-m) >= 0 est strictement interieur a B. Une arete ab d'une boule admissible a donc une profondeur de
// Tukey (demi-plans fermes, plan orthogonal a ab) <= Kmax-2 parmi les projections des interieurs stricts de
// D_ab. On garde S_a = { b : profondeur <= Kmax-2 } (profondeur d'un sous-ensemble <= vraie profondeur :
// l'elagage reste sur) et on enumere b, c dans S_a (l'arete ac verifie la meme condition). --compare rejuge
// sans elagage et exige le meme ensemble ; la fixture d'egalite (profondeur = p = Kmax-2) tue le mutant
// --inject=overprune (seuil Kmax-3).
//
// Arithmetique : u18 ; u = b-a, v = c-a, w = u x v ; O = a + P/D, P = |u|^2 (v x w) + |v|^2 (w x u),
// D = 2|w|^2 ; s(x) = D|x-a|^2 - 2(x-a).P < 0 <=> interieur strict, = 0 <=> sphere (|s| < 2^116, i128).
// Niveau : R^2 = |P|^2 / D^2, compare au niveau du catalogue en entiers multiprecision (Boost cpp_int).
// Arbre k-d propre : boites exactes pour les boules diametrales, filtre flottant conservateur pour les
// boules circonscrites, tests exacts aux feuilles. Aucune structure du generateur n'est utilisee ; sont lus
// le catalogue de la chaine et l'index des positions uniques (verifie contre les points d'entree).
//
//   q3_sample_judge family <uniform|terrain|clusters> <n> <Kmax> <sites> <workers> [options]
//   q3_sample_judge file <cut.u32le> <Kmax> <sites> <workers> [options]
//   q3_sample_judge fixture-eq [options]         (fixture d'egalite gravee, Kmax = 5, tous les sites)
// options : --compare | --no-prune ; --seed=<u64> ; --sites=<i,j,...> ; --long-sites=<N> ; --min-top=<n> ;
//           --min-long=<n> ; --inject=overprune | --inject=level | --inject=shell-dup | --inject=key |
//           --inject=drop-long (partenaires >= 1600 unites retires du parcours elague : --compare doit le voir)
//
// Code 0 conforme ; 1 manquante, EXTRA, desaccord d'elagage ou recoupement faux ; 2 argument/chaine ;
// 3 vacuite (plancher --min-top de cles q3 regulieres p = Kmax-2, arite 3, ou --min-long d'incidences longues
// non atteint ; mutant cible non tue).
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <numeric>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "src/chain/tower_chain.hpp"
#include "src/tower/tree/cloud_index.hpp"
#include "tests/gen/front_fixtures.hpp"

using mhgp9::gen::Point3;
using mhgp9::tower::BallData;
using mhgp9::tower::P3;
using boost::multiprecision::cpp_int;
__extension__ typedef __int128 i128;
__extension__ typedef unsigned __int128 u128;
using i64 = std::int64_t;

namespace {

constexpr std::size_t kSample = 64;  // premier essai de profondeur sur un sous-echantillon regulier

struct Options {
  int mode = 0;  // 0 elagage, 1 sans elagage, 2 comparaison
  std::uint64_t seed = 0x9e3779b97f4a7c15ull;
  std::uint64_t min_top = 1;
  bool overprune = false;
  bool corrupt_level = false;  // mutant : tous les niveaux du catalogue faux (den + 1)
  bool shell_dup = false;      // mutant : dernier site de coquille remplace par le premier (doublon)
  bool corrupt_key = false;    // mutant : cle seule faussee (c + 1), niveau et coquille intacts
  std::size_t long_sites = 0;  // --long-sites=N : N sites les plus isoles (distance au Kmax-ieme voisin)
  bool drop_long = false;      // mutant : partenaires b a >= 1600 unites retires du parcours elague
  std::uint64_t min_long = 0;  // --min-long=N : plancher d'incidences >= 1600 unites (sinon code 3)
  std::vector<std::size_t> sites;  // --sites= : sites imposes (indices de l'index), a la place du tirage
};

std::vector<Point3> read_u32le(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) throw std::invalid_argument("cannot open input");
  const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (bytes.size() % 12) throw std::invalid_argument("bad length");
  std::vector<Point3> pts(bytes.size() / 12);
  for (std::size_t i = 0; i < pts.size(); ++i) {
    std::uint32_t c[3];
    for (int a = 0; a < 3; ++a) {
      std::uint32_t v = 0;
      for (int b = 0; b < 4; ++b) v |= std::uint32_t(bytes[(3 * i + a) * 4 + b]) << (8 * b);
      if (v > 262143u) throw std::invalid_argument("coordinate outside 18 bits");
      c[a] = v;
    }
    pts[i] = Point3{(mhgp9::gen::Coordinate)c[0], (mhgp9::gen::Coordinate)c[1], (mhgp9::gen::Coordinate)c[2]};
  }
  return pts;
}

// Fixture d'egalite (contrelecture adverse de C) : a, b, trois sites sur le segment ab (projection nulle,
// comptes dans tout demi-plan), c tel que (a,b,c) soit aigu, un site oppose dans D_ab hors de B ; profondeur
// de ab = 3 = p = Kmax-2 a K5. Complement : 27 sites de grille eloignes, pour un nuage non trivial.
std::vector<Point3> fixture_eq() {
  std::vector<Point3> p = {{1000, 1000, 1000}, {1040, 1000, 1000}, {1010, 1000, 1000}, {1020, 1000, 1000},
                           {1030, 1000, 1000}, {1020, 1030, 1000}, {1020, 985, 1000}};
  for (int i = 0; i < 27; ++i)
    p.push_back({1100 + 20 * (i % 3), 1100 + 20 * ((i / 3) % 3), 1000 + 20 * (i / 9)});
  return p;
}

std::uint64_t fnv_points(const std::vector<Point3>& pts) {
  std::uint64_t h = 1469598103934665603ull;
  for (const auto& p : pts)
    for (const i64 v : {i64(p.x), i64(p.y), i64(p.z)})
      for (int b = 0; b < 8; ++b) { h ^= (std::uint64_t(v) >> (8 * b)) & 0xff; h *= 1099511628211ull; }
  return h;
}

// Tirage des sites : permutation de Fisher-Yates par splitmix64, graine publiee.
std::vector<std::size_t> sample_sites(std::size_t n, std::size_t take, std::uint64_t seed) {
  std::vector<std::size_t> v(n);
  std::iota(v.begin(), v.end(), std::size_t{0});
  std::uint64_t s = seed;
  const auto next = [&]() {
    std::uint64_t z = (s += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
  };
  for (std::size_t i = 0; i + 1 < n && i < take; ++i) std::swap(v[i], v[i + next() % (n - i)]);
  v.resize(std::min(take, n));
  return v;
}

struct V { i64 x, y, z; };
inline V sub(const P3& a, const P3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline i64 dot(const V& a, const V& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline V cross(const V& a, const V& b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }

// ---- Arbre k-d independant.
struct Node { i64 lo[3], hi[3]; std::uint32_t begin, end, left, right; };
struct Tree {
  const std::vector<P3>* pos = nullptr;
  std::vector<std::uint32_t> idx;
  std::vector<Node> nodes;
  std::uint32_t build(std::uint32_t b, std::uint32_t e) {
    Node nd{};
    for (int k = 0; k < 3; ++k) { nd.lo[k] = INT64_MAX; nd.hi[k] = INT64_MIN; }
    for (std::uint32_t i = b; i < e; ++i) {
      const P3& p = (*pos)[idx[i]];
      const i64 c[3] = {p.x, p.y, p.z};
      for (int k = 0; k < 3; ++k) { nd.lo[k] = std::min(nd.lo[k], c[k]); nd.hi[k] = std::max(nd.hi[k], c[k]); }
    }
    nd.begin = b; nd.end = e; nd.left = nd.right = UINT32_MAX;
    const std::uint32_t id = static_cast<std::uint32_t>(nodes.size());
    nodes.push_back(nd);
    if (e - b > 8) {
      int ax = 0;
      for (int k = 1; k < 3; ++k) if (nd.hi[k] - nd.lo[k] > nd.hi[ax] - nd.lo[ax]) ax = k;
      const std::uint32_t mid = b + (e - b) / 2;
      std::nth_element(idx.begin() + b, idx.begin() + mid, idx.begin() + e, [&](std::uint32_t l, std::uint32_t r) {
        const P3& pl = (*pos)[l]; const P3& pr = (*pos)[r];
        const i64 cl = ax == 0 ? pl.x : ax == 1 ? pl.y : pl.z, cr = ax == 0 ? pr.x : ax == 1 ? pr.y : pr.z;
        return cl < cr || (cl == cr && l < r);
      });
      const std::uint32_t l = build(b, mid), r = build(mid, e);
      nodes[id].left = l; nodes[id].right = r;
    }
    return id;
  }
  void init(const std::vector<P3>& p) {
    pos = &p; idx.resize(p.size()); std::iota(idx.begin(), idx.end(), 0u); nodes.clear();
    build(0, static_cast<std::uint32_t>(p.size()));
  }
};

// Interieurs stricts de la boule diametrale de (a,b), boites exactes : x interieur <=> |2x-(a+b)|^2 < |a-b|^2.
void diametral_interior(const Tree& t, const P3& a, const P3& b, std::vector<std::uint32_t>& out) {
  const i64 c[3] = {a.x + b.x, a.y + b.y, a.z + b.z};
  const V ab = sub(b, a);
  const i64 r2 = dot(ab, ab);
  std::vector<std::uint32_t> stack{0};
  while (!stack.empty()) {
    const Node& n = t.nodes[stack.back()]; stack.pop_back();
    i64 dmin = 0;
    for (int k = 0; k < 3; ++k) {
      const i64 lo = 2 * n.lo[k], hi = 2 * n.hi[k];
      const i64 d = c[k] < lo ? lo - c[k] : c[k] > hi ? c[k] - hi : 0;
      dmin += d * d;
    }
    if (dmin >= r2) continue;
    if (n.left == UINT32_MAX) {
      for (std::uint32_t i = n.begin; i < n.end; ++i) {
        const P3& x = (*t.pos)[t.idx[i]];
        if (dot(sub(x, a), sub(x, b)) < 0) out.push_back(t.idx[i]);
      }
    } else { stack.push_back(n.left); stack.push_back(n.right); }
  }
}

// Profondeur de Tukey (demi-plans fermes par l'origine, orthogonaux a t) des vecteurs q projetes sur le plan
// orthogonal a t. sigma = det(x,y,t) est invariant par projection ; comparaisons angulaires exactes.
unsigned tukey_depth(const std::vector<V>& q, const V& t) {
  unsigned zero = 0;
  std::vector<V> v;
  v.reserve(q.size());
  for (const auto& x : q) {
    const V cx = cross(x, t);
    if (cx.x == 0 && cx.y == 0 && cx.z == 0) ++zero; else v.push_back(x);
  }
  if (v.empty()) return zero;
  const i64 ax = std::llabs(t.x), ay = std::llabs(t.y), az = std::llabs(t.z);
  const V e = ax <= ay && ax <= az ? V{1, 0, 0} : ay <= az ? V{0, 1, 0} : V{0, 0, 1};
  const V r = cross(t, e);
  const auto half = [&](const V& x) {
    const i64 s1 = dot(cross(r, x), t);
    if (s1 > 0) return 0;
    if (s1 == 0 && dot(r, x) > 0) return 0;
    return 1;
  };
  const auto sigma = [&](const V& x, const V& y) {
    const i64 s = dot(cross(x, y), t);
    return s > 0 ? 1 : s < 0 ? -1 : 0;
  };
  std::vector<std::pair<int, std::size_t>> order(v.size());
  for (std::size_t i = 0; i < v.size(); ++i) order[i] = {half(v[i]), i};
  std::sort(order.begin(), order.end(), [&](const auto& l, const auto& r2) {
    if (l.first != r2.first) return l.first < r2.first;
    return sigma(v[l.second], v[r2.second]) > 0;
  });
  const std::size_t m = v.size();
  const auto in_half_turn = [&](const V& from, const V& x) {
    const int s = sigma(from, x);
    if (s > 0) return true;
    if (s < 0) return false;
    const V ft = cross(from, t), xt = cross(x, t);  // projections paralleles : meme sens inclus, oppose exclu
    return i128(ft.x) * xt.x + i128(ft.y) * xt.y + i128(ft.z) * xt.z > 0;
  };
  std::size_t best = 0, j = 0;
  for (std::size_t i = 0; i < m; ++i) {
    if (j < i) j = i;
    while (j < i + m && in_half_turn(v[order[i].second], v[order[j % m].second])) ++j;
    best = std::max(best, j - i);
  }
  return zero + static_cast<unsigned>(m - best);
}

struct Sphere {
  P3 a; i128 D; i128 P[3];
  double cx, cy, cz, r2;
  i128 s(const P3& x) const {
    const i128 dx = x.x - a.x, dy = x.y - a.y, dz = x.z - a.z;
    return D * (dx * dx + dy * dy + dz * dz) - 2 * (dx * P[0] + dy * P[1] + dz * P[2]);
  }
};

bool census(const Tree& t, const Sphere& sp, unsigned cap, std::vector<std::uint32_t>& inside,
            std::vector<std::uint32_t>& shell) {
  inside.clear(); shell.clear();
  const double margin = 1e-6 * (sp.r2 + 1.0) + 4.0;
  std::vector<std::uint32_t> stack{0};
  while (!stack.empty()) {
    const Node& n = t.nodes[stack.back()]; stack.pop_back();
    double dmin = 0, dmax = 0;
    const double c[3] = {sp.cx, sp.cy, sp.cz};
    for (int k = 0; k < 3; ++k) {
      const double lo = double(n.lo[k]), hi = double(n.hi[k]);
      const double d = c[k] < lo ? lo - c[k] : c[k] > hi ? c[k] - hi : 0.0;
      dmin += d * d;
      const double f = std::max(std::fabs(c[k] - lo), std::fabs(c[k] - hi));
      dmax += f * f;
    }
    if (dmin > sp.r2 + margin) continue;
    if (dmax < sp.r2 - margin) {
      for (std::uint32_t i = n.begin; i < n.end; ++i) inside.push_back(t.idx[i]);
      if (inside.size() > cap) return false;
      continue;
    }
    if (n.left == UINT32_MAX) {
      for (std::uint32_t i = n.begin; i < n.end; ++i) {
        const i128 v = sp.s((*t.pos)[t.idx[i]]);
        if (v < 0) { inside.push_back(t.idx[i]); if (inside.size() > cap) return false; }
        else if (v == 0) shell.push_back(t.idx[i]);
      }
    } else { stack.push_back(n.left); stack.push_back(n.right); }
  }
  return true;
}

cpp_int big(i128 v) {
  const bool neg = v < 0;
  const u128 u = neg ? (u128)(-(v + 1)) + 1 : (u128)v;
  cpp_int r = static_cast<std::uint64_t>(u >> 64);
  r <<= 64;
  r += static_cast<std::uint64_t>(u);
  return neg ? cpp_int(-r) : r;
}

// Niveau du catalogue (U192 lo, mid, hi / den) egal a |P|^2 / D^2 ? Avec a, b, c sur la coquille, un rayon
// egal au rayon circonscrit fixe la sphere : c'est la boule circonscrite (centre dans le plan de abc).
bool same_level(const BallData& ball, const Sphere& sp) {
  cpp_int num = ball.level.num[2];
  num <<= 64; num += ball.level.num[1];
  num <<= 64; num += ball.level.num[0];
  const cpp_int den = big(ball.level.den);
  const cpp_int D = big(sp.D);
  cpp_int p2 = 0;
  for (int k = 0; k < 3; ++k) { const cpp_int pk = big(sp.P[k]); p2 += pk * pk; }
  return num * D * D == p2 * den;
}

// Cle canonique independante (forme puissance a|z|^2 + b.z + c, a > 0, reduite par le pgcd) comparee a
// ball.key, champ par champ (contrelecture B v4 : la cle consommee par FULL, pas seulement le niveau).
bool same_key(const BallData& ball, const cpp_int (&f)[5]) {
  cpp_int g = abs(f[0]);
  for (int i = 1; i < 5; ++i) g = gcd(g, abs(f[i]));
  if (g == 0) return false;
  const cpp_int k[5] = {big(ball.key.a), big(ball.key.b[0]), big(ball.key.b[1]), big(ball.key.b[2]), big(ball.key.c)};
  for (int i = 0; i < 5; ++i) if (f[i] / g != k[i]) return false;
  return true;
}

// Forme puissance de la sphere circonscrite : s(x) = D|x|^2 + (-2(D a + P)).x + (D|a|^2 + 2 a.P).
void sphere_form(const Sphere& sp, cpp_int (&f)[5]) {
  const cpp_int D = big(sp.D);
  const cpp_int a[3] = {cpp_int(sp.a.x), cpp_int(sp.a.y), cpp_int(sp.a.z)};
  const cpp_int P[3] = {big(sp.P[0]), big(sp.P[1]), big(sp.P[2])};
  f[0] = D;
  cpp_int c = 0;
  for (int k = 0; k < 3; ++k) { f[1 + k] = -2 * (D * a[k] + P[k]); c += D * a[k] * a[k] + 2 * a[k] * P[k]; }
  f[4] = c;
}

struct Tri {
  std::uint32_t a, b, c;
  bool operator<(const Tri& o) const { return std::tie(a, b, c) < std::tie(o.a, o.b, o.c); }
  bool operator==(const Tri& o) const { return a == o.a && b == o.b && c == o.c; }
};

struct Totals {
  std::uint64_t partners = 0, kept = 0, triangles = 0, acute = 0, incidences = 0, found = 0, missing = 0;
  std::uint64_t extended = 0, over_shell = 0, extra = 0, cross_fail = 0, by_p[10] = {}, by_len[3] = {};
  std::uint64_t by_len_top[3] = {};
};

int run(const std::string& label, const std::vector<Point3>& points, unsigned kmax, std::size_t sites,
        std::size_t workers, const Options& opt) {
  if (kmax < 3 || kmax > 10) { std::fprintf(stderr, "Kmax must be in 3..10\n"); return 2; }
  mhgp9::ChainOptions o;
  o.kmax = kmax; o.workers = workers; o.tower_static_threads = static_cast<int>(workers);
  o.keep_catalogue = true; o.run_tower = false;
  const auto r = mhgp9::run_tower_chain(points, o);
  if (r.status != mhgp9::ChainStatus::kComplete) {
    std::printf("%s kmax=%u chain_status=%s reason=%s\n", label.c_str(), kmax, mhgp9::chain_status_name(r.status),
                r.reason.c_str());
    return 2;
  }
  std::vector<mhgp9::tower::InputPoint> input(points.size());
  for (std::size_t i = 0; i < points.size(); ++i)
    input[i] = mhgp9::tower::InputPoint{static_cast<mhgp9::tower::PointId>(i), P3{points[i].x, points[i].y, points[i].z}};
  const auto ix = mhgp9::tower::build_cloud_index(input);
  const std::vector<P3>& pos = ix.upos;
  const std::size_t n = pos.size();
  // Coherence de l'index lu avec les points d'entree (sites distincts : une position par site).
  if (n != points.size()) { std::printf("%s index_size_mismatch\n", label.c_str()); return 2; }
  for (std::size_t u = 0; u < n; ++u) {
    const auto& q = points[static_cast<std::size_t>(ix.point_id(static_cast<std::int32_t>(u)))];
    if (pos[u].x != q.x || pos[u].y != q.y || pos[u].z != q.z) {
      std::printf("%s index_position_mismatch\n", label.c_str());
      return 2;
    }
  }
  std::vector<BallData> cat = r.catalogue_balls;
  if (opt.corrupt_level) for (auto& ball : cat) ball.level.den += 1;
  if (opt.shell_dup)
    for (auto& ball : cat)
      if (ball.n_shell >= 3) ball.shell_ids[ball.n_shell - 1] = ball.shell_ids[0];
  if (opt.corrupt_key) for (auto& ball : cat) ball.key.c += 1;
  std::vector<std::vector<std::uint32_t>> by_site(n);
  for (std::size_t i = 0; i < cat.size(); ++i)
    for (const auto s : cat[i].shell()) by_site[static_cast<std::size_t>(s)].push_back(static_cast<std::uint32_t>(i));
  Tree tree; tree.init(pos);
  const unsigned pmax = kmax - 2;
  const unsigned prune_threshold = opt.overprune ? pmax - 1 : pmax;  // mutant : elague des profondeur = pmax

  const auto judge_site = [&](std::size_t a, bool prune, std::int64_t exclude, Totals& t, std::set<Tri>* expected,
                              std::unordered_set<std::uint32_t>& keys, std::vector<std::string>& lines,
                              bool& target_missed) {
    std::vector<std::uint32_t> keep, inner, inside, shell;
    std::vector<V> q;
    for (std::size_t b = 0; b < n; ++b) {
      if (b == a) continue;
      ++t.partners;
      if (prune && opt.drop_long) {  // mutant : le parcours elague perd les ancres longues
        const V e = sub(pos[b], pos[a]);
        if (dot(e, e) >= 1600ll * 1600) continue;
      }
      if (prune) {
        inner.clear();
        diametral_interior(tree, pos[a], pos[b], inner);
        if (inner.size() > prune_threshold) {
          const V axis = sub(pos[b], pos[a]);
          const auto qv = [&](std::uint32_t y) {
            return V{2 * pos[y].x - pos[a].x - pos[b].x, 2 * pos[y].y - pos[a].y - pos[b].y,
                     2 * pos[y].z - pos[a].z - pos[b].z};
          };
          bool pruned = false;
          if (inner.size() > 2 * kSample) {
            q.clear();
            const std::size_t step = inner.size() / kSample;
            for (std::size_t k = 0; k < inner.size(); k += step) q.push_back(qv(inner[k]));
            pruned = tukey_depth(q, axis) > prune_threshold;
          }
          if (!pruned) {
            q.clear();
            for (const auto y : inner) q.push_back(qv(y));
            pruned = tukey_depth(q, axis) > prune_threshold;
          }
          if (pruned) continue;
        }
      }
      keep.push_back(static_cast<std::uint32_t>(b));
    }
    t.kept += keep.size();
    std::unordered_set<std::uint32_t> matched_here;
    for (std::size_t i = 0; i < keep.size(); ++i) {
      for (std::size_t j = i + 1; j < keep.size(); ++j) {
        const std::uint32_t b = keep[i], c = keep[j];
        ++t.triangles;
        const V u = sub(pos[b], pos[a]), v = sub(pos[c], pos[a]);
        if (dot(u, v) <= 0 || dot(sub(pos[a], pos[b]), sub(pos[c], pos[b])) <= 0 ||
            dot(sub(pos[a], pos[c]), sub(pos[b], pos[c])) <= 0) continue;
        ++t.acute;
        const V w = cross(u, v);
        const i128 uu = dot(u, u), vv = dot(v, v);
        const V vw = cross(v, w), wu = cross(w, u);
        Sphere sp;
        sp.a = pos[a];
        sp.D = 2 * (i128(w.x) * w.x + i128(w.y) * w.y + i128(w.z) * w.z);
        sp.P[0] = uu * vw.x + vv * wu.x; sp.P[1] = uu * vw.y + vv * wu.y; sp.P[2] = uu * vw.z + vv * wu.z;
        const double Dd = double(sp.D);
        const double ox = double(sp.P[0]) / Dd, oy = double(sp.P[1]) / Dd, oz = double(sp.P[2]) / Dd;
        sp.cx = double(pos[a].x) + ox; sp.cy = double(pos[a].y) + oy; sp.cz = double(pos[a].z) + oz;
        sp.r2 = ox * ox + oy * oy + oz * oz;
        if (!census(tree, sp, pmax, inside, shell)) continue;
        const unsigned p = static_cast<unsigned>(inside.size());
        ++t.incidences; ++t.by_p[p];
        const i64 l2 = std::max({dot(u, u), dot(v, v), dot(sub(pos[c], pos[b]), sub(pos[c], pos[b]))});
        const int len = l2 < 500ll * 500 ? 0 : l2 < 1600ll * 1600 ? 1 : 2;  // unites de grille
        ++t.by_len[len];
        if (p == pmax) ++t.by_len_top[len];
        if (expected) {
          std::array<std::uint32_t, 3> k{static_cast<std::uint32_t>(a), b, c};
          std::sort(k.begin(), k.end());
          expected->insert(Tri{k[0], k[1], k[2]});
        }
        char buf[200];
        if (shell.size() > 12) {  // la chaine refuse toute coquille > 12 : sur une chaine complete, omission
          ++t.over_shell; ++t.missing;
          std::snprintf(buf, sizeof buf, "MISSING_SHELL_OVER_12 a=%zu b=%u c=%u p=%u shell=%zu", a, b, c, p,
                        shell.size());
          lines.push_back(buf);
          continue;
        }
        if (shell.size() > 3) ++t.extended;
        // Paire antipodale dans la coquille (2O = x + y) : q_min = 2, sinon 3.
        bool antipodal = false;
        for (std::size_t x = 0; x < shell.size() && !antipodal; ++x)
          for (std::size_t y = x + 1; y < shell.size() && !antipodal; ++y) {
            const P3& px = pos[shell[x]]; const P3& py = pos[shell[y]];
            antipodal = sp.D * (px.x + py.x - 2 * pos[a].x) == 2 * sp.P[0] &&
                        sp.D * (px.y + py.y - 2 * pos[a].y) == 2 * sp.P[1] &&
                        sp.D * (px.z + py.z - 2 * pos[a].z) == 2 * sp.P[2];
          }
        std::vector<std::int32_t> shell_sorted(shell.begin(), shell.end());
        std::sort(shell_sorted.begin(), shell_sorted.end());
        std::int64_t hit = -1;
        bool cross_ok = false;
        for (const auto bi : by_site[a]) {
          if (static_cast<std::int64_t>(bi) == exclude) continue;
          const auto& ball = cat[bi];
          if (ball.n_shell != shell.size()) continue;
          // Ensemble exact des sites de coquille (listes triees egales : ni doublon, ni site substitue).
          std::vector<std::int32_t> theirs(ball.shell().begin(), ball.shell().end());
          std::sort(theirs.begin(), theirs.end());
          if (theirs != shell_sorted || !same_level(ball, sp)) continue;
          cpp_int form[5];
          sphere_form(sp, form);
          if (!same_key(ball, form)) continue;
          hit = bi;
          // Recoupement : memes interieurs (ids distincts, strictement interieurs), arite coherente.
          std::vector<std::int32_t> ids(ball.interior().begin(), ball.interior().end());
          std::sort(ids.begin(), ids.end());
          std::vector<std::int32_t> mine(inside.begin(), inside.end());
          std::sort(mine.begin(), mine.end());
          cross_ok = ball.n_interior == p && ids == mine &&
                     std::adjacent_find(ids.begin(), ids.end()) == ids.end() &&
                     ball.arity == (antipodal ? 2 : 3);
          break;
        }
        if (hit >= 0 && cross_ok) {
          ++t.found;
          keys.insert(static_cast<std::uint32_t>(hit));
          matched_here.insert(static_cast<std::uint32_t>(hit));
        } else if (hit >= 0) {
          ++t.cross_fail;
          std::snprintf(buf, sizeof buf, "CROSS_CHECK_FAILED a=%zu b=%u c=%u p=%u shell=%zu", a, b, c, p, shell.size());
          lines.push_back(buf);
        } else {
          ++t.missing;
          if (exclude >= 0) {
            const auto sh = cat[static_cast<std::size_t>(exclude)].shell();
            target_missed = target_missed ||
                            (std::find(sh.begin(), sh.end(), static_cast<std::int32_t>(b)) != sh.end() &&
                             std::find(sh.begin(), sh.end(), static_cast<std::int32_t>(c)) != sh.end());
          }
          std::snprintf(buf, sizeof buf, "MISSING a=%zu b=%u c=%u p=%u shell=%zu", a, b, c, p, shell.size());
          lines.push_back(buf);
        }
      }
    }
    // Sens inverse : boules regulieres a 3 sites, d'arite 3, p <= pmax, passant par a, non retrouvees.
    if (exclude < 0) {
      for (const auto bi : by_site[a]) {
        const auto& ball = cat[bi];
        if (ball.n_shell != 3 || ball.arity != 3 || ball.n_interior > pmax) continue;
        if (matched_here.count(bi)) continue;
        ++t.extra;
        char buf[160];
        std::snprintf(buf, sizeof buf, "EXTRA a=%zu ball=%u p=%u", a, bi, unsigned(ball.n_interior));
        lines.push_back(buf);
      }
    }
  };

  std::vector<std::size_t> sampled = sample_sites(n, sites, opt.seed);
  if (opt.long_sites > 0) {
    // Sites isoles choisis depuis les seules coordonnees (sans le juge) : plus grande distance au Kmax-ieme
    // plus proche voisin, force brute exacte en entiers ; les ancres longues vivent dans ces zones creuses.
    std::vector<std::pair<i64, std::size_t>> iso(n);
    std::vector<i64> d2(n);
    for (std::size_t u = 0; u < n; ++u) {
      for (std::size_t x = 0; x < n; ++x) { const V e = sub(pos[x], pos[u]); d2[x] = dot(e, e); }
      std::nth_element(d2.begin(), d2.begin() + std::min<std::size_t>(kmax, n - 1), d2.end());
      iso[u] = {d2[std::min<std::size_t>(kmax, n - 1)], u};
    }
    std::sort(iso.begin(), iso.end(), [](const auto& l, const auto& r2) { return l.first > r2.first || (l.first == r2.first && l.second < r2.second); });
    sampled.clear();
    for (std::size_t i = 0; i < std::min(opt.long_sites, n); ++i) sampled.push_back(iso[i].second);
  }
  if (!opt.sites.empty()) {
    for (const auto a : opt.sites) if (a >= n) { std::fprintf(stderr, "site %zu out of range\n", a); return 2; }
    sampled = opt.sites;
  }
  Totals t;
  std::unordered_set<std::uint32_t> keys;
  std::vector<std::string> lines;
  int disagreements = 0;
  std::uint64_t kept_pruned = 0;
  for (const auto a : sampled) {
    bool dummy = false;
    const std::uint64_t long_before = t.by_len[2];
    if (opt.mode == 2) {
      Totals t1;
      std::set<Tri> e1, e2;
      std::unordered_set<std::uint32_t> k1;
      std::vector<std::string> l1;
      judge_site(a, true, -1, t1, &e1, k1, l1, dummy);
      judge_site(a, false, -1, t, &e2, keys, lines, dummy);
      kept_pruned += t1.kept;
      if (!(e1 == e2)) {
        ++disagreements;
        std::printf("%s PRUNE_DISAGREES a=%zu pruned=%zu full=%zu\n", label.c_str(), a, e1.size(), e2.size());
      }
    } else {
      judge_site(a, opt.mode == 0, -1, t, nullptr, keys, lines, dummy);
    }
    if (t.by_len[2] > long_before)  // sites porteurs d'incidences longues (>= 1600 unites) : cibles de --compare
      std::printf("%s LONG_SITE a=%zu incidences=%llu\n", label.c_str(), a,
                  (unsigned long long)(t.by_len[2] - long_before));
  }
  if (opt.mode == 2) t.kept = kept_pruned;
  for (const auto& l : lines) std::printf("%s %s\n", label.c_str(), l.c_str());
  std::uint64_t top_keys = 0, top_population = 0;
  const auto top = [&](std::uint32_t k) { return cat[k].n_interior == pmax && cat[k].arity == 3 && cat[k].n_shell == 3; };
  std::uint64_t q2_keys = 0;
  for (const auto k : keys) { if (top(k)) ++top_keys; if (cat[k].arity == 2) ++q2_keys; }
  for (const auto& ball : cat) if (ball.n_shell == 3 && ball.arity == 3 && ball.n_interior == pmax) ++top_population;
  // Mutant cible : une cle trouvee de rang p = pmax, retiree de la table, rejugee depuis un site tire de sa
  // coquille ; un triangle manquant doit porter deux autres sites de sa coquille.
  std::uint32_t target = UINT32_MAX;
  for (const auto k : keys) if (top(k) && (target == UINT32_MAX || k < target)) target = k;
  bool mutant_killed = false;
  if (target != UINT32_MAX) {
    for (const auto a : sampled) {
      const auto sh = cat[target].shell();
      if (std::find(sh.begin(), sh.end(), static_cast<std::int32_t>(a)) == sh.end()) continue;
      Totals tm; std::unordered_set<std::uint32_t> km; std::vector<std::string> lm;
      judge_site(a, opt.mode != 1, target, tm, nullptr, km, lm, mutant_killed);
      if (mutant_killed) break;
    }
  }
  std::printf("%s n=%zu kmax=%u balls=%zu input_fnv=%016llx seed=%016llx mode=%s%s sampled_sites=%zu partners=%llu "
              "kept=%llu triangles=%llu acute=%llu incidences=%llu found=%llu missing=%llu cross_fail=%llu extra=%llu "
              "unique_keys=%zu q2_keys=%llu top_keys=%llu top_population=%llu extended=%llu shell_over_12=%llu mutant_killed=%d "
              "len_lt500=%llu len_500_1600=%llu len_ge1600=%llu top_lt500=%llu top_500_1600=%llu top_ge1600=%llu "
              "prune_disagreements=%d",
              label.c_str(), n, kmax, cat.size(), (unsigned long long)fnv_points(points), (unsigned long long)opt.seed,
              opt.mode == 0 ? "prune" : opt.mode == 1 ? "no-prune" : "compare",
              opt.overprune ? "+overprune" : opt.corrupt_level ? "+level" : opt.shell_dup ? "+shell-dup" : opt.corrupt_key ? "+key"
              : opt.drop_long ? "+drop-long" : "",
              sampled.size(), (unsigned long long)t.partners, (unsigned long long)t.kept,
              (unsigned long long)t.triangles, (unsigned long long)t.acute, (unsigned long long)t.incidences,
              (unsigned long long)t.found, (unsigned long long)t.missing, (unsigned long long)t.cross_fail,
              (unsigned long long)t.extra, keys.size(), (unsigned long long)q2_keys, (unsigned long long)top_keys,
              (unsigned long long)top_population,
              (unsigned long long)t.extended, (unsigned long long)t.over_shell, mutant_killed ? 1 : 0,
              (unsigned long long)t.by_len[0], (unsigned long long)t.by_len[1], (unsigned long long)t.by_len[2],
              (unsigned long long)t.by_len_top[0], (unsigned long long)t.by_len_top[1],
              (unsigned long long)t.by_len_top[2], disagreements);
  for (unsigned p = 0; p <= pmax; ++p) std::printf(" p%u=%llu", p, (unsigned long long)t.by_p[p]);
  std::printf("\n");
  if (t.missing || t.cross_fail || t.extra || disagreements) return 1;
  if (!mutant_killed || top_keys < opt.min_top || t.by_len[2] < opt.min_long) return 3;
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    Options opt;
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
      const std::string a = argv[i];
      if (a == "--compare") opt.mode = 2;
      else if (a == "--no-prune") opt.mode = 1;
      else if (a == "--inject=overprune") opt.overprune = true;
      else if (a == "--inject=level") opt.corrupt_level = true;
      else if (a == "--inject=shell-dup") opt.shell_dup = true;
      else if (a == "--inject=key") opt.corrupt_key = true;
      else if (a == "--inject=drop-long") opt.drop_long = true;
      else if (a.rfind("--min-long=", 0) == 0) opt.min_long = std::stoull(a.substr(11));
      else if (a.rfind("--long-sites=", 0) == 0) opt.long_sites = std::stoul(a.substr(13));
      else if (a.rfind("--sites=", 0) == 0) {
        std::string list = a.substr(8);
        std::size_t pos = 0;
        while (pos <= list.size()) {
          const std::size_t comma = list.find(',', pos);
          opt.sites.push_back(std::stoul(list.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos)));
          if (comma == std::string::npos) break;
          pos = comma + 1;
        }
      }
      else if (a.rfind("--seed=", 0) == 0) opt.seed = std::stoull(a.substr(7), nullptr, 0);
      else if (a.rfind("--min-top=", 0) == 0) opt.min_top = std::stoull(a.substr(10));
      else if (a.rfind("--", 0) == 0) { std::fprintf(stderr, "unknown option %s\n", a.c_str()); return 2; }
      else args.push_back(a);
    }
    if (args.size() == 1 && args[0] == "fixture-eq") return run("fixture_eq", fixture_eq(), 5, 1000, 1, opt);
    if (args.size() == 6 && args[0] == "family") {
      const auto fx = mhgp9::gen::bench::make_front_fixture(std::stoul(args[2]), args[1], 3);
      return run(args[1] + "_" + args[2], fx.points, std::stoul(args[3]), std::stoul(args[4]), std::stoul(args[5]), opt);
    }
    if (args.size() == 5 && args[0] == "file") {
      const auto slash = args[1].find_last_of('/');
      return run(args[1].substr(slash == std::string::npos ? 0 : slash + 1), read_u32le(args[1]), std::stoul(args[2]),
                 std::stoul(args[3]), std::stoul(args[4]), opt);
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "error: %s\n", e.what());
    return 2;
  }
  std::fprintf(stderr, "usage: q3_sample_judge family <name> <n> <Kmax> <sites> <workers> [options] | "
                       "file <cut.u32le> <Kmax> <sites> <workers> [options] | fixture-eq [options]\n");
  return 2;
}
