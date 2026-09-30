// Porte de regression G1 (audit independant du 29 septembre 2026, GEOMETRIE_CATALOGUE.md et
// CONTRE_AUDIT_GEOMETRIE.md) : les requetes a centre rationnel de SiteTree (nearest, closed_ball) sont exactes pour
// tout centre de forme q2/q3/q4, meme loin du nuage.
//
// Defaut corrige : la marge fixe du filtre flottant (0,02) etait appliquee a tout centre, alors que l'erreur des
// distances carrees approchees croit comme 2^-53 |c|^2. Un centre circonscrit lointain perdait des sites de coquille
// et faussait le departage de nearest. Correction : garde rationnelle exacte du domaine du filtre (sites, ancre et
// centre dans le cube u18), repli exact au-dehors.
//
// A. Fixture gravee (sonde de l'auditeur) : sites (0,0,0), (225077,1,0), (225068,1,0), (152369,7,1), cospheriques ;
//    centre4 = ancre + N / D avec D = 18, N = (4051305, -455918672115, 2783084223159), soit
//    (450145/2, -50657630235/2, 309231580351/2), hors du cube. Egalite des quatre distances jugee par une autre formule
//    (|2z - 2c|^2 en entiers). Attendu : coquille des quatre sites, interieur vide, nearest(count) = les count premiers
//    sites par indice, cles nulles. Avant correction : coquille {0, 2, 3}, nearest(2) = {0, 2} et
//    nearest(3) = {0, 2, 3}.
// B. Balayage : triangles tres obtus, tetraedres presque plats, variantes de la fixture et simplexes quelconques,
//    plonges dans des nuages u18 aleatoires. Chaque requete est jugee contre une force brute d'arithmetique autre :
//    D s(z) = |D (z - a) - N|^2 - |N|^2 en entiers larges ; les cles rendues par nearest sont comprises. Planchers :
//    requetes hors du cube, et requetes « adverses » ou la marge fixe classait mal un site de coquille.
// C. Domaine du filtre (API filtered, absente avant la correction) : un centre est servi par le filtre ssi il est dans
//    le cube u18, jugement independant en entiers larges ; bords du cube graves ; centres de type MEB (milieux,
//    triangles aigus, tetraedres contenant leur centre, sites) toujours filtres ; nuage de 21 bits toujours en repli,
//    lui aussi exact. Compiler avec -DMHGP10_GATE_OLD_API retire la partie C pour rejouer A et B contre une
//    bibliotheque d'avant la correction.
//
// Codes : 0 conforme, 1 desaccord d'un juge, 3 plancher de couverture non atteint.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "arith/geometry.hpp"
#include "arith/wide.hpp"
#include "cloud/cloud.hpp"
#include "cloud/site_tree.hpp"

using namespace mhgp10;

namespace {

using arith::I128w;
using W4 = arith::Wide<4>;
constexpr i64 kL = kCoordinateLimit;

u64 failures = 0;
void expect(bool ok, const char* what, u64 detail = 0) {
  if (!ok) {
    if (failures < 20) std::printf("ECHEC %s (%llu)\n", what, static_cast<unsigned long long>(detail));
    ++failures;
  }
}

W4 wide(i128 v) {
  W4 r;
  arith::resize(I128w::from_i128(v), r);
  return r;
}
W4 wmul(i128 a, i128 b) { return arith::mul(I128w::from_i128(a), I128w::from_i128(b)); }
W4 wadd(const W4& a, const W4& b) {
  W4 r;
  if (!arith::add(a, b, r)) ++failures;  // impossible dans les bornes du juge (< 2^210)
  return r;
}
W4 wsub(const W4& a, const W4& b) { return wadd(a, b.negated()); }

geom::P3 site(const Cloud& c, u32 s) { return {i64(c.x[s]), i64(c.y[s]), i64(c.z[s])}; }

// Juge : D s(z) = |D (z - a) - N|^2 - |N|^2, entiers larges (formule et arithmetique autres que geom::side_key).
W4 judge_key(const geom::Center& c, const geom::P3& a, const geom::P3& z) {
  const i64 d[3] = {z.x - a.x, z.y - a.y, z.z - a.z};
  W4 v2, n2;
  for (int i = 0; i < 3; ++i) {
    const W4 vi = wsub(wmul(c.D, d[i]), wide(c.N[i]));  // |.| < 2^102
    I128w vn;
    if (!arith::resize(vi, vn)) ++failures;
    v2 = wadd(v2, arith::mul(vn, vn));
    n2 = wadd(n2, wmul(c.N[i], c.N[i]));
  }
  return wsub(v2, n2);
}

// Centre dans le cube ferme [0, L]^3 : 0 <= D a_i + N_i <= L D (entiers larges).
bool judge_in_cube(const geom::Center& c, const geom::P3& a) {
  const i64 av[3] = {a.x, a.y, a.z};
  for (int i = 0; i < 3; ++i) {
    const W4 t = wadd(wmul(c.D, av[i]), wide(c.N[i]));
    if (t.sign() < 0 || arith::cmp(t, wmul(c.D, kL)) > 0) return false;
  }
  return true;
}

// Ancienne decision flottante pour un site de coquille : distance approchee hors de [r2a - 0,02, r2a + 0,02]
// (memes operations que SiteTree : centre a + fl(N) / fl(D), somme des carres).
bool fixed_margin_misses(const geom::Center& c, const geom::P3& a, const geom::P3& z) {
  const double D = static_cast<double>(c.D);
  const double q[3] = {double(a.x) + static_cast<double>(c.N[0]) / D, double(a.y) + static_cast<double>(c.N[1]) / D,
                       double(a.z) + static_cast<double>(c.N[2]) / D};
  auto d2 = [&](const geom::P3& p) {
    const double dx = double(p.x) - q[0], dy = double(p.y) - q[1], dz = double(p.z) - q[2];
    return dx * dx + dy * dy + dz * dz;
  };
  const double r2a = d2(a), d = d2(z);
  return !(d >= r2a - 0.02 && d <= r2a + 0.02);
}

struct Stats {
  u64 queries = 0, far = 0, adverse = 0, nearest_checks = 0, filtered_in = 0, filtered_out = 0;
  u64 kind_far[5] = {}, kind_adverse[5] = {};
  int kind = 4;  // type du simplexe de la requete courante (balayage B)
};

// Une requete jugee : closed_ball et nearest(count) contre la force brute, puis le domaine du filtre.
void check_query(const SiteTree& tree, const geom::P3& a, const geom::Center& ctr, Stats& st) {
  const Cloud& c = tree.cloud();
  const u32 n = c.sites();
  std::vector<std::pair<W4, u32>> all;
  all.reserve(n);
  std::vector<u32> wi, wu;
  bool adverse = false;
  for (u32 s = 0; s < n; ++s) {
    const geom::P3 z = site(c, s);
    const W4 k = judge_key(ctr, a, z);
    all.push_back({k, s});
    if (k.sign() < 0) wi.push_back(s);
    if (k.sign() == 0) {
      wu.push_back(s);
      adverse |= fixed_margin_misses(ctr, a, z);
    }
  }
  std::vector<u32> gi, gu;
  tree.closed_ball(a, ctr, gi, gu);
  expect(gi == wi, "closed_ball interieur", st.queries);
  expect(gu == wu, "closed_ball coquille", st.queries);
  std::sort(all.begin(), all.end(), [](const auto& x, const auto& y) {
    const int k = arith::cmp(x.first, y.first);
    return k != 0 ? k < 0 : x.second < y.second;
  });
  std::vector<std::pair<i128, u32>> got;
  for (u32 count : {1u, 2u, 3u, 4u, 7u, 12u}) {
    tree.nearest(a, ctr, count, got);
    const size_t m = std::min<size_t>(count, n);
    bool same = got.size() == m;
    for (size_t i = 0; same && i < m; ++i)
      same = got[i].second == all[i].second && arith::cmp(wmul(ctr.D, got[i].first), all[i].first) == 0;
    expect(same, "nearest", st.queries * 100 + count);
    ++st.nearest_checks;
  }
  const bool far = !judge_in_cube(ctr, a);
  ++st.queries;
  st.far += far;
  st.adverse += adverse;
  st.kind_far[st.kind] += far;
  st.kind_adverse[st.kind] += adverse;
#ifndef MHGP10_GATE_OLD_API
  const bool f = tree.filtered(a, ctr);
  bool sites_u18 = true;
  for (u32 s = 0; s < n; ++s) sites_u18 &= c.x[s] <= kL && c.y[s] <= kL && c.z[s] <= kL;
  expect(f == (sites_u18 && !far), "filtered == centre dans le cube (nuage u18)", st.queries);
  st.filtered_in += f;
  st.filtered_out += !f;
#endif
}

Cloud make_cloud(const std::vector<geom::P3>& pts, int bits) {
  std::vector<u32> x, y, z, id;
  for (size_t i = 0; i < pts.size(); ++i) {
    x.push_back(u32(pts[i].x));
    y.push_back(u32(pts[i].y));
    z.push_back(u32(pts[i].z));
    id.push_back(u32(i));
  }
  auto r = prepare_cloud(x, y, z, id, bits);
  if (!r.ok()) {
    std::printf("ECHEC prepare_cloud\n");
    std::exit(3);
  }
  return r.take();
}

// ---------------------------------------------------------------- A : fixture gravee
void part_a() {
  const std::vector<geom::P3> pts = {{0, 0, 0}, {225077, 1, 0}, {225068, 1, 0}, {152369, 7, 1}};
  const Cloud cloud = make_cloud(pts, 18);
  const SiteTree tree(cloud);
  const geom::P3 a = pts[0];
  geom::Center ctr{};
  expect(geom::center4(pts[0], pts[1], pts[2], pts[3], ctr), "fixture : tetraedre non plat");
  const i128 wantN[3] = {4051305, -455918672115, i128{2783084223159}};
  expect(ctr.D == 18 && ctr.N[0] == wantN[0] && ctr.N[1] == wantN[1] && ctr.N[2] == wantN[2], "fixture : N, D graves");
  // 2c entier : (450145, -50657630235, 309231580351) ; quatre distances egales, formule |2z - 2c|^2.
  const i128 c2[3] = {450145, -50657630235, i128{309231580351}};
  const i64 av[3] = {a.x, a.y, a.z};
  for (int i = 0; i < 3; ++i) expect(c2[i] * 18 == 2 * (i128(av[i]) * 18 + wantN[i]), "fixture : 2c = 2 (a + N / D)");
  i128 d0 = -1;
  for (u32 s = 0; s < cloud.sites(); ++s) {
    const geom::P3 z = site(cloud, s);
    const i128 v[3] = {2 * i128(z.x) - c2[0], 2 * i128(z.y) - c2[1], 2 * i128(z.z) - c2[2]};
    const i128 d = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (d0 < 0) d0 = d;
    expect(d == d0, "fixture : quatre sites cospheriques", s);
  }
  std::vector<u32> gi, gu;
  tree.closed_ball(a, ctr, gi, gu);
  const bool ball_ok = gi.empty() && gu == std::vector<u32>{0, 1, 2, 3};
  expect(ball_ok, "fixture : coquille des quatre sites, interieur vide");
  std::printf("fixture_g1 interieur=%zu coquille=%zu", gi.size(), gu.size());
  for (u32 s : gu) std::printf(" %u", s);
  std::printf("\n");
  std::vector<std::pair<i128, u32>> got;
  for (u32 count = 1; count <= 4; ++count) {
    tree.nearest(a, ctr, count, got);
    bool ok = got.size() == count;
    for (u32 i = 0; ok && i < count; ++i) ok = got[i].first == 0 && got[i].second == i;
    expect(ok, "fixture : nearest exact (cles nulles, departage par indice)", count);
    std::printf("fixture_g1 nearest(%u) =", count);
    for (const auto& [k, s] : got) std::printf(" (%s,%u)", arith::to_string(I128w::from_i128(k)).c_str(), s);
    std::printf("\n");
  }
#ifndef MHGP10_GATE_OLD_API
  expect(!tree.filtered(a, ctr), "fixture : centre hors du domaine du filtre");
#endif
}

// ---------------------------------------------------------------- B : balayage des centres lointains
geom::P3 rnd_point(std::mt19937_64& g, i64 lo, i64 hi) {
  auto r = [&] { return lo + i64(g() % u64(hi - lo + 1)); };
  return {r(), r(), r()};
}
geom::P3 add(const geom::P3& p, const geom::P3& e) { return {p.x + e.x, p.y + e.y, p.z + e.z}; }
bool in_cube(const geom::P3& p) { return p.x >= 0 && p.y >= 0 && p.z >= 0 && p.x <= kL && p.y <= kL && p.z <= kL; }

// Simplexes presque degeneres a centre circonscrit lointain ; arite 3 ou 4, points dans le cube.
std::vector<geom::P3> far_simplex(std::mt19937_64& g, int kind) {
  const i64 m = i64{1} << 16;
  const geom::P3 p0 = rnd_point(g, m, kL - m);
  auto coord = [&](i64 r) { return i64(g() % u64(2 * r + 1)) - r; };
  auto vec = [&](i64 r) { return geom::P3{coord(r), coord(r), coord(r)}; };
  if (kind == 0) {  // triangle tres obtus : p2 pres du segment [p0, p1]
    const geom::P3 e = vec(i64{1} << 15);
    const double t = 0.2 + 0.6 * double(g() % 1000) / 1000.0;
    const geom::P3 o = vec(2);
    const geom::P3 p2{p0.x + i64(std::llround(t * double(e.x))) + o.x, p0.y + i64(std::llround(t * double(e.y))) + o.y,
                      p0.z + i64(std::llround(t * double(e.z))) + o.z};
    return {p0, add(p0, e), p2};
  }
  if (kind == 1 || kind == 2) {  // tetraedre presque plat : p3 pres du plan (p0, p1, p2), dans le triangle
    const geom::P3 e1 = vec(i64{1} << 15), e2 = vec(i64{1} << 15);
    const double al = 0.1 + 0.3 * double(g() % 1000) / 1000.0, be = 0.1 + 0.3 * double(g() % 1000) / 1000.0;
    const geom::P3 base{p0.x + i64(std::llround(al * double(e1.x) + be * double(e2.x))),
                        p0.y + i64(std::llround(al * double(e1.y) + be * double(e2.y))),
                        p0.z + i64(std::llround(al * double(e1.z) + be * double(e2.z)))};
    const geom::P3 p1 = add(p0, e1), p2 = add(p0, e2);
    if (kind == 1) return {p0, p1, p2, add(base, vec(1))};
    // kind 2 : le voisin entier de plus petit |det| non nul (centre tres lointain)
    geom::P3 best = base;
    i128 bd = -1;
    for (i64 dx = -1; dx <= 1; ++dx)
      for (i64 dy = -1; dy <= 1; ++dy)
        for (i64 dz = -1; dz <= 1; ++dz) {
          const geom::P3 q = add(base, {dx, dy, dz});
          const geom::P3 u = geom::sub(p1, p0), v = geom::sub(p2, p0), w = geom::sub(q, p0);
          const geom::P3 n = geom::cross(u, v);
          const i128 det = i128(n.x) * w.x + i128(n.y) * w.y + i128(n.z) * w.z;
          const i128 ad = det < 0 ? -det : det;
          if (ad != 0 && (bd < 0 || ad < bd)) {
            bd = ad;
            best = q;
          }
        }
    return {p0, p1, p2, best};
  }
  // kind 3 : variante de la fixture G1 (trois points presque alignes le long d'un axe, quatrieme presque dans le plan)
  const i64 dx1 = i64{150000} + i64(g() % 50000), dx2 = dx1 - 1 - i64(g() % 20), dx3 = i64(g() % 150000);
  const geom::P3 q0 = rnd_point(g, 0, 20000);
  return {q0, add(q0, {dx1, 1, 0}), add(q0, {dx2, 1, 0}), add(q0, {dx3, 1 + i64(g() % 9), 1})};
}

void part_b(Stats& st) {
  std::mt19937_64 g(20260929);
  for (int t = 0; t < 64; ++t) {
    std::vector<geom::P3> pts;
    std::vector<std::pair<int, std::vector<geom::P3>>> simplexes;
    for (int k = 0; k < 16; ++k) {
      std::vector<geom::P3> s = far_simplex(g, k % 4);
      bool ok = true;
      for (const auto& p : s) ok &= in_cube(p);
      if (!ok) continue;
      for (const auto& p : s) pts.push_back(p);
      simplexes.push_back({k % 4, s});
    }
    const u32 nbg = 40 + u32(g() % 260);
    const i64 span = (t % 3 == 0) ? 64 : kL;  // nuage dense en un coin, ou etale
    for (u32 i = 0; i < nbg; ++i) pts.push_back(rnd_point(g, 0, span));
    const Cloud cloud = make_cloud(pts, 18);
    const SiteTree tree(cloud);
    for (const auto& [kind, s] : simplexes) {
      geom::Center ctr;
      const bool ok = s.size() == 3 ? geom::center3(s[0], s[1], s[2], ctr) : geom::center4(s[0], s[1], s[2], s[3], ctr);
      if (!ok) continue;
      st.kind = kind;
      check_query(tree, s[0], ctr, st);
    }
    st.kind = 4;
    // simplexes quelconques du nuage (centres dans ou hors du cube)
    for (int q = 0; q < 24; ++q) {
      const int arity = 2 + int(g() % 3);
      geom::P3 p[4];
      for (int i = 0; i < 4; ++i) p[i] = site(cloud, u32(g() % cloud.sites()));
      geom::Center ctr;
      bool ok = true;
      if (arity == 2) geom::center2(p[0], p[1], ctr);
      else if (arity == 3) ok = geom::center3(p[0], p[1], p[2], ctr);
      else ok = geom::center4(p[0], p[1], p[2], p[3], ctr);
      if (ok) check_query(tree, p[0], ctr, st);
    }
  }
}

#ifndef MHGP10_GATE_OLD_API
// ---------------------------------------------------------------- C : domaine du filtre
struct DomainStats {
  u64 edges = 0, sites = 0, mids = 0, acute = 0, inside4 = 0, wide = 0;
};

void part_c(Stats& st, DomainStats& ds) {
  // Bords du cube graves : centre de (p0, p1, p2), relatif a p0, et appartenance attendue au domaine.
  struct Edge {
    geom::P3 p[3];
    bool in;
  };
  const Edge edges[] = {
      {{{0, 0, 0}, {0, 4, 0}, {1, 2, 0}}, false},                // centre (-3/2, 2, 0)
      {{{0, 0, 0}, {0, 2, 0}, {1, 1, 0}}, true},                 // centre (0, 1, 0), face x = 0
      {{{kL, 0, 0}, {kL, 2, 0}, {kL - 1, 1, 0}}, true},          // centre (L, 1, 0), face x = L
      {{{kL, 0, 0}, {kL, 4, 0}, {kL - 1, 2, 0}}, false},         // centre (L + 3/2, 2, 0)
      {{{6, kL, 7}, {8, kL, 7}, {7, kL - 1, 7}}, true},          // centre (7, L, 7), face y = L
      {{{5, kL, 7}, {13, kL, 7}, {9, kL - 1, 7}}, false},        // centre (9, L + 15/2, 7)
      {{{3, 3, 0}, {3, 11, 0}, {4, 7, 1}}, false},               // centre sous le plan z = 0
  };
  std::mt19937_64 g(7);
  for (const Edge& e : edges) {
    std::vector<geom::P3> pts(e.p, e.p + 3);
    for (int i = 0; i < 30; ++i) pts.push_back(rnd_point(g, 0, kL));
    const Cloud cloud = make_cloud(pts, 18);
    const SiteTree tree(cloud);
    geom::Center ctr;
    expect(geom::center3(e.p[0], e.p[1], e.p[2], ctr), "bord : triangle");
    expect(tree.filtered(e.p[0], ctr) == e.in, "bord du cube : domaine grave", ds.edges);
    expect(judge_in_cube(ctr, e.p[0]) == e.in, "bord du cube : juge", ds.edges);
    check_query(tree, e.p[0], ctr, st);
    ++ds.edges;
  }
  // Centres de type MEB (dans conv du support, donc dans le cube) : toujours servis par le filtre.
  for (int t = 0; t < 24; ++t) {
    const u32 n = 60 + u32(g() % 400);
    const i64 span = (t % 2 == 0) ? 40 : kL;
    std::vector<geom::P3> pts;
    for (u32 i = 0; i < n; ++i) pts.push_back(rnd_point(g, 0, span));
    const Cloud cloud = make_cloud(pts, 18);
    const SiteTree tree(cloud);
    for (int q = 0; q < 160; ++q) {
      geom::P3 p[4];
      for (int i = 0; i < 4; ++i) p[i] = site(cloud, u32(g() % cloud.sites()));
      geom::Center ctr;
      u64* counter = nullptr;
      switch (q % 4) {
        case 0:
          ctr = geom::Center{{0, 0, 0}, 1};
          counter = &ds.sites;
          break;
        case 1:
          geom::center2(p[0], p[1], ctr);
          counter = &ds.mids;
          break;
        case 2:
          if (!geom::acute(p[0], p[1], p[2]) || !geom::center3(p[0], p[1], p[2], ctr)) continue;
          counter = &ds.acute;
          break;
        default: {  // jusqu'a huit tirages pour un tetraedre contenant strictement son centre
          const geom::P3* tet[4] = {&p[0], &p[1], &p[2], &p[3]};
          bool found = false;
          for (int tries = 0; tries < 8 && !found; ++tries) {
            if (tries > 0)
              for (int i = 0; i < 4; ++i) p[i] = site(cloud, u32(g() % cloud.sites()));
            found = geom::center4(p[0], p[1], p[2], p[3], ctr) && geom::strictly_inside_tetra(tet, p[0], ctr);
          }
          if (!found) continue;
          counter = &ds.inside4;
        }
      }
      expect(tree.filtered(p[0], ctr), "centre de type MEB servi par le filtre", st.queries);
      check_query(tree, p[0], ctr, st);
      ++*counter;
    }
  }
  // Nuage de 21 bits (hors du domaine du filtre) : repli exact pour des centres representables (sites, milieux).
  for (int t = 0; t < 6; ++t) {
    std::vector<geom::P3> pts;
    const i64 lim21 = (i64{1} << 21) - 1;
    for (int i = 0; i < 200; ++i) pts.push_back(rnd_point(g, 0, lim21));
    pts.push_back({lim21, lim21, lim21});
    const Cloud cloud = make_cloud(pts, 21);
    const SiteTree tree(cloud);
    for (int q = 0; q < 40; ++q) {
      const geom::P3 a = site(cloud, u32(g() % cloud.sites())), b = site(cloud, u32(g() % cloud.sites()));
      geom::Center ctr{{0, 0, 0}, 1};
      if (q % 2) geom::center2(a, b, ctr);
      expect(!tree.filtered(a, ctr), "nuage de 21 bits hors du domaine du filtre", st.queries);
      check_query(tree, a, ctr, st);
      ++ds.wide;
    }
  }
}
#endif

}  // namespace

int main() {
  part_a();
  Stats st;
  part_b(st);
  std::printf("balayage requetes=%llu hors_cube=%llu adverses=%llu nearest=%llu\n",
              static_cast<unsigned long long>(st.queries), static_cast<unsigned long long>(st.far),
              static_cast<unsigned long long>(st.adverse), static_cast<unsigned long long>(st.nearest_checks));
  static const char* kKinds[5] = {"obtus", "plat", "plat_min", "variante_g1", "quelconque"};
  for (int k = 0; k < 5; ++k)
    std::printf("balayage type=%s hors_cube=%llu adverses=%llu\n", kKinds[k],
                static_cast<unsigned long long>(st.kind_far[k]), static_cast<unsigned long long>(st.kind_adverse[k]));
  bool floors = st.far >= 400 && st.adverse >= 300 && st.queries >= 1000;
#ifndef MHGP10_GATE_OLD_API
  DomainStats ds;
  part_c(st, ds);
  std::printf("domaine bords=%llu sites=%llu milieux=%llu aigus=%llu tetra_internes=%llu nuage21=%llu filtre=%llu "
              "repli=%llu\n",
              static_cast<unsigned long long>(ds.edges), static_cast<unsigned long long>(ds.sites),
              static_cast<unsigned long long>(ds.mids), static_cast<unsigned long long>(ds.acute),
              static_cast<unsigned long long>(ds.inside4), static_cast<unsigned long long>(ds.wide),
              static_cast<unsigned long long>(st.filtered_in), static_cast<unsigned long long>(st.filtered_out));
  floors = floors && ds.edges == 7 && ds.sites >= 500 && ds.mids >= 500 && ds.acute >= 100 && ds.inside4 >= 100 &&
           ds.wide >= 200 && st.filtered_in >= 2000 && st.filtered_out >= 600;
#endif
  if (failures) {
    std::printf("site_tree_far_center_echecs %llu\n", static_cast<unsigned long long>(failures));
    return 1;
  }
  if (!floors) {
    std::printf("site_tree_far_center_plancher_non_atteint\n");
    return 3;
  }
  std::printf("site_tree_far_center_ok\n");
  return 0;
}
