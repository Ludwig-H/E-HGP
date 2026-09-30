// Porte de regression G1 et contrat de SiteTree (audit independant du 29 septembre 2026, GEOMETRIE_CATALOGUE.md et
// CONTRE_AUDIT_GEOMETRIE.md ; contre-audit continu CONTRE_AUDIT_SITETREE_CORRIGE_20260929.md ; second tour de
// correction du 30 septembre). Les requetes a centre rationnel de SiteTree (nearest, closed_ball) sont exactes pour
// tout centre de forme q2/q3/q4, meme loin du nuage, sous les quatre modes d'arrondi ; filtered() decide le chemin
// selon son contrat complet (six conditions, site_tree.hpp).
//
// Defauts corriges. Tour 1 : la marge fixe du filtre flottant (0,02) etait appliquee a tout centre, alors que l'erreur
// des distances carrees approchees croit comme 2^-53 |c|^2 ; un centre circonscrit lointain perdait des sites de
// coquille et faussait le departage de nearest. Garde rationnelle exacte du domaine du filtre (sites, ancre et centre
// dans le cube u18), repli exact au-dehors. Tour 2 : le filtre servait aussi hors FE_TONEAREST, hypothese de sa
// preuve ; il y est coupe (doctrine v4) et la voie exacte prend le relais.
//
// Chaque requete est jugee une fois par une force brute d'arithmetique autre (D s(z) = |D (z - a) - N|^2 - |N|^2 en
// entiers larges), puis rejouee sous FE_TONEAREST, FE_UPWARD, FE_DOWNWARD et FE_TOWARDZERO sur un arbre construit en
// FE_TONEAREST. Dans chaque mode : closed_ball et nearest(count) egaux au juge (cles comprises), filtered() egal au
// domaine du contrat juge en entiers larges (donc vrai seulement en FE_TONEAREST), mode d'arrondi inchange apres les
// requetes.
//
// A. Fixture gravee (sonde de l'auditeur) : sites (0,0,0), (225077,1,0), (225068,1,0), (152369,7,1), cospheriques ;
//    centre4 = ancre + N / D avec D = 18, N = (4051305, -455918672115, 2783084223159), soit
//    (450145/2, -50657630235/2, 309231580351/2), hors du cube. Egalite des quatre distances jugee par une autre formule
//    (|2z - 2c|^2 en entiers). Attendu dans chaque mode : coquille des quatre sites, interieur vide,
//    nearest(count) = les count premiers sites par indice, cles nulles. Avant le tour 1 : coquille {0, 2, 3},
//    nearest(2) = {0, 2} et nearest(3) = {0, 2, 3}.
// B. Balayage : triangles tres obtus, tetraedres presque plats, variantes de la fixture et simplexes quelconques,
//    plonges dans des nuages u18 aleatoires. Compte des requetes « adverses » : la marge fixe sans garde classait mal
//    un site de coquille (mesure dans chaque mode, plancher en FE_TONEAREST seulement).
// C. Domaine du filtre : bords du cube graves ; centres de type MEB (servis en FE_TONEAREST) ; nuage de 21 bits
//    (repli) ; nuage de 21 bits dont l'ancre et le centre sont dans le cube u18 (repli par le seul drapeau des sites) ;
//    ancres fabriquees hors du cube, centre dans le cube (repli par la seule ancre) ; table gravee des bornes du
//    contrat (D, N, ancre, extremes i128 et i64 : filtered seul, sans requete, ces valeurs sortant de la
//    precondition de representation).
// D. Arrondi lu a chaque requete dans le fil qui l'execute : arbre construit en FE_UPWARD puis interroge ; fils en
//    mode different du fil principal.
//
// Planchers, fixes avant la premiere execution de cette version : geometriques (requetes du balayage >= 1000, centres
// hors du cube >= 400, bords = 7, sites et milieux >= 500, aigus et tetraedres >= 100, nuage 21 bits >= 200, nuage
// 21 bits a ancre et centre dans le cube >= 200, ancres fabriquees >= 100, bornes gravees = 30, arrondi D1 = 40 et
// D2 = 20) ; FE_TONEAREST : adverses du balayage >= 300, filtrees >= 2000, repli >= 600 ; chaque mode dirige : replis
// d'arrondi (requetes du domaine du contrat servies par le repli exact) >= 2000, plancher distinct des planchers
// geometriques.
//
// Codes : 0 conforme, 1 desaccord d'un juge, 2 mode d'arrondi indisponible (refus avant calcul), 3 plancher non
// atteint. Compiler avec -DMHGP10_GATE_OLD_API retire tout appel a filtered() (parties A et B, sorties seules) pour
// rejouer contre une bibliotheque d'avant le tour 1.
#include <algorithm>
#include <cfenv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <random>
#include <string>
#include <thread>
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
constexpr i128 kMaxD = i128{1} << 82, kMaxN = i128{1} << 100;  // bornes (3) et (5) du contrat de filtered()

// Modes d'arrondi ; l'indice 0 (FE_TONEAREST) est celui de la preuve du filtre.
constexpr int kModes = 4;
const int kMode[kModes] = {FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO};
const char* const kModeName[kModes] = {"nearest", "upward", "downward", "towardzero"};

void set_mode(int m) {
  if (std::fesetround(kMode[m]) != 0 || std::fegetround() != kMode[m]) {
    std::printf("ECHEC mode d'arrondi %s indisponible\n", kModeName[m]);
    std::exit(2);
  }
}

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
bool in_cube(const geom::P3& p) { return p.x >= 0 && p.y >= 0 && p.z >= 0 && p.x <= kL && p.y <= kL && p.z <= kL; }

// Tous les sites dans le cube u18, lu sur les coordonnees du nuage (pas sur l'arbre).
bool cloud_u18(const Cloud& c) {
  for (u32 s = 0; s < c.sites(); ++s)
    if (c.x[s] > kL || c.y[s] > kL || c.z[s] > kL) return false;
  return true;
}

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

// Centre dans le cube ferme [0, L]^3 : 0 <= D a_i + N_i <= L D (entiers larges, D > 0).
bool judge_in_cube(const geom::Center& c, const geom::P3& a) {
  const i64 av[3] = {a.x, a.y, a.z};
  for (int i = 0; i < 3; ++i) {
    const W4 t = wadd(wmul(c.D, av[i]), wide(c.N[i]));
    if (t.sign() < 0 || arith::cmp(t, wmul(c.D, kL)) > 0) return false;
  }
  return true;
}

// Domaine du filtre selon le contrat de filtered(), conditions (2) a (6) ; la condition (1), le mode d'arrondi, est
// ajoutee par l'appelant.
bool judge_domain(bool sites_u18, const geom::P3& a, const geom::Center& c) {
  if (!sites_u18 || c.D <= 0 || c.D > kMaxD || !in_cube(a)) return false;
  for (int i = 0; i < 3; ++i)
    if (c.N[i] < -kMaxN || c.N[i] > kMaxN) return false;
  return judge_in_cube(c, a);
}

// Ancienne decision flottante pour un site de coquille : distance approchee hors de [r2a - 0,02, r2a + 0,02]
// (memes operations que SiteTree : centre a + fl(N) / fl(D), somme des carres), dans le mode d'arrondi courant.
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

// Verite d'une requete : interieur, coquille et ordre (D s(z), site) de tous les sites.
struct Judged {
  std::vector<u32> interior, shell;
  std::vector<std::pair<W4, u32>> order;
};
Judged judge(const Cloud& c, const geom::P3& a, const geom::Center& ctr) {
  Judged j;
  j.order.reserve(c.sites());
  for (u32 s = 0; s < c.sites(); ++s) {
    const W4 k = judge_key(ctr, a, site(c, s));
    j.order.push_back({k, s});
    if (k.sign() < 0) j.interior.push_back(s);
    if (k.sign() == 0) j.shell.push_back(s);
  }
  std::sort(j.order.begin(), j.order.end(), [](const auto& x, const auto& y) {
    const int k = arith::cmp(x.first, y.first);
    return k != 0 ? k < 0 : x.second < y.second;
  });
  return j;
}

// Reponses de l'arbre a une requete, dans le mode d'arrondi du fil appelant.
constexpr int kCounts = 6;
constexpr u32 kCount[kCounts] = {1, 2, 3, 4, 7, 12};
struct Answer {
  std::vector<u32> interior, shell;
  std::vector<std::pair<i128, u32>> nearest[kCounts];
  bool filtered = false;
};
Answer ask(const SiteTree& tree, const geom::P3& a, const geom::Center& ctr) {
  Answer r;
  tree.closed_ball(a, ctr, r.interior, r.shell);
  for (int j = 0; j < kCounts; ++j) tree.nearest(a, ctr, kCount[j], r.nearest[j]);
#ifndef MHGP10_GATE_OLD_API
  r.filtered = tree.filtered(a, ctr);
#endif
  return r;
}

// Sorties egales au juge ; rend le nombre de controles nearest.
u64 compare(const Answer& r, const Judged& j, const geom::Center& ctr, u64 tag) {
  expect(r.interior == j.interior, "closed_ball interieur", tag);
  expect(r.shell == j.shell, "closed_ball coquille", tag);
  for (int k = 0; k < kCounts; ++k) {
    const size_t m = std::min<size_t>(kCount[k], j.order.size());
    bool same = r.nearest[k].size() == m;
    for (size_t i = 0; same && i < m; ++i)
      same = r.nearest[k][i].second == j.order[i].second &&
             arith::cmp(wmul(ctr.D, r.nearest[k][i].first), j.order[i].first) == 0;
    expect(same, "nearest", tag * 100 + kCount[k]);
  }
  return kCounts;
}

struct ModeStats {
  u64 filtered_in = 0, filtered_out = 0, rounding_fallbacks = 0, adverse = 0;
};

struct Stats {
  u64 queries = 0, far = 0, nearest_checks = 0;
  u64 kind_far[5] = {}, kind_adverse[5] = {};  // adverses en FE_TONEAREST
  int kind = 4;                                // type du simplexe de la requete courante (balayage B)
  ModeStats mode[kModes];
};

// Une requete jugee une fois, rejouee sous les quatre modes d'arrondi (arbre deja construit).
void check_query(const SiteTree& tree, const geom::P3& a, const geom::Center& ctr, Stats& st) {
  const Cloud& c = tree.cloud();
  const Judged j = judge(c, a, ctr);
  const bool far = !judge_in_cube(ctr, a);
#ifndef MHGP10_GATE_OLD_API
  const bool domain = judge_domain(cloud_u18(c), a, ctr);
#endif
  for (int m = 0; m < kModes; ++m) {
    set_mode(m);
    const Answer r = ask(tree, a, ctr);
    bool adverse = false;
    for (u32 s : j.shell) adverse |= fixed_margin_misses(ctr, a, site(c, s));
    const bool retained = std::fegetround() == kMode[m];
    set_mode(0);
    const u64 tag = st.queries * 10 + u64(m);
    expect(retained, "mode d'arrondi conserve par les requetes", tag);
    st.nearest_checks += compare(r, j, ctr, tag);
    ModeStats& ms = st.mode[m];
    ms.adverse += adverse;
    if (m == 0) st.kind_adverse[st.kind] += adverse;
#ifndef MHGP10_GATE_OLD_API
    expect(r.filtered == (m == 0 && domain), "filtered == domaine du contrat, mode d'arrondi compris", tag);
    ms.filtered_in += r.filtered;
    ms.filtered_out += !r.filtered;
    ms.rounding_fallbacks += domain && !r.filtered;
#endif
  }
  ++st.queries;
  st.far += far;
  st.kind_far[st.kind] += far;
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
  for (int m = 0; m < kModes; ++m) {
    set_mode(m);
    Answer r;
    tree.closed_ball(a, ctr, r.interior, r.shell);
    for (u32 count = 1; count <= 4; ++count) tree.nearest(a, ctr, count, r.nearest[count - 1]);
#ifndef MHGP10_GATE_OLD_API
    r.filtered = tree.filtered(a, ctr);
#endif
    const bool retained = std::fegetround() == kMode[m];
    set_mode(0);
    expect(retained, "fixture : mode d'arrondi conserve", u64(m));
    expect(r.interior.empty() && r.shell == std::vector<u32>{0, 1, 2, 3},
           "fixture : coquille des quatre sites, interieur vide", u64(m));
    std::printf("fixture_g1 mode=%s interieur=%zu coquille=%zu", kModeName[m], r.interior.size(), r.shell.size());
    for (u32 s : r.shell) std::printf(" %u", s);
    std::printf("\n");
    for (u32 count = 1; count <= 4; ++count) {
      const auto& got = r.nearest[count - 1];
      bool ok = got.size() == count;
      for (u32 i = 0; ok && i < count; ++i) ok = got[i].first == 0 && got[i].second == i;
      expect(ok, "fixture : nearest exact (cles nulles, departage par indice)", count * 10 + u32(m));
      std::printf("fixture_g1 mode=%s nearest(%u) =", kModeName[m], count);
      for (const auto& [k, s] : got) std::printf(" (%s,%u)", arith::to_string(I128w::from_i128(k)).c_str(), s);
      std::printf("\n");
    }
#ifndef MHGP10_GATE_OLD_API
    expect(!r.filtered, "fixture : centre hors du domaine du filtre", u64(m));
#endif
  }
}

// ---------------------------------------------------------------- B : balayage des centres lointains
geom::P3 rnd_point(std::mt19937_64& g, i64 lo, i64 hi) {
  auto r = [&] { return lo + i64(g() % u64(hi - lo + 1)); };
  return {r(), r(), r()};
}
geom::P3 add(const geom::P3& p, const geom::P3& e) { return {p.x + e.x, p.y + e.y, p.z + e.z}; }

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
  u64 edges = 0, sites = 0, mids = 0, acute = 0, inside4 = 0, wide = 0, wide_in_cube = 0, forged = 0, bounds = 0;
};

// Bornes gravees du contrat de filtered() : ancre, N, D et decision attendue en FE_TONEAREST sur un nuage u18.
struct Bound {
  geom::P3 a;
  i128 N[3];
  i128 D;
  bool in;
  const char* what;
};

void part_c(Stats& st, DomainStats& ds) {
  // C1. Bords du cube graves : centre de (p0, p1, p2), relatif a p0, et appartenance attendue au domaine.
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
  // C2. Centres de type MEB (dans conv du support, donc dans le cube) : servis par le filtre en FE_TONEAREST.
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
  // C3. Nuage de 21 bits (hors du domaine du filtre) : repli exact pour des centres representables (sites, milieux).
  const i64 lim21 = (i64{1} << 21) - 1;
  for (int t = 0; t < 6; ++t) {
    std::vector<geom::P3> pts;
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
  // C4. Nuage de 21 bits dont l'ancre et le centre sont dans le cube u18 (site du cube, milieu de deux sites du cube) :
  //     seul le drapeau des sites exclut le filtre. Tue le mutant qui ignore ce drapeau.
  for (int t = 0; t < 4; ++t) {
    std::vector<geom::P3> pts;
    for (int i = 0; i < 150; ++i) pts.push_back(rnd_point(g, 0, kL));
    for (int i = 0; i < 50; ++i) pts.push_back(rnd_point(g, 0, lim21));
    pts.push_back({lim21, lim21, lim21});
    const Cloud cloud = make_cloud(pts, 21);
    const SiteTree tree(cloud);
    std::vector<geom::P3> inside;
    for (u32 s = 0; s < cloud.sites(); ++s)
      if (in_cube(site(cloud, s))) inside.push_back(site(cloud, s));
    for (int q = 0; q < 60; ++q) {
      const geom::P3 a = inside[g() % inside.size()], b = inside[g() % inside.size()];
      geom::Center ctr{{0, 0, 0}, 1};
      if (q % 2) geom::center2(a, b, ctr);
      const bool both_in = in_cube(a) && judge_in_cube(ctr, a);
      expect(both_in, "nuage de 21 bits : ancre et centre dans le cube u18", st.queries);
      expect(!cloud_u18(cloud), "nuage de 21 bits : un site hors du cube u18", st.queries);
      expect(!tree.filtered(a, ctr), "nuage de 21 bits, ancre et centre dans le cube : hors du domaine", st.queries);
      check_query(tree, a, ctr, st);
      ds.wide_in_cube += both_in;
    }
  }
  // C5. Ancres fabriquees hors du cube (une unite hors d'une face, ou 2^20 au-dela), centre dans le cube (site ou
  //     milieu de deux sites) : seule l'ancre exclut le filtre. Tue le mutant qui ne controle plus l'ancre. Cles
  //     representables : |z - a| < 2^21, D <= 2, |N_i| < 2^22.
  {
    std::vector<geom::P3> pts;
    for (int i = 0; i < 200; ++i) pts.push_back(rnd_point(g, 0, kL));
    pts.push_back({0, 0, 0});
    pts.push_back({kL, kL, kL});
    const Cloud cloud = make_cloud(pts, 18);
    const SiteTree tree(cloud);
    const i64 away[4] = {-1, kL + 1, -(i64{1} << 20), kL + (i64{1} << 20)};
    for (int q = 0; q < 128; ++q) {
      const geom::P3 b = site(cloud, u32(g() % cloud.sites())), b2 = site(cloud, u32(g() % cloud.sites()));
      geom::P3 a = rnd_point(g, 0, kL);
      i64* coord[3] = {&a.x, &a.y, &a.z};
      *coord[q % 3] = away[(q / 3) % 4];
      geom::Center ctr;
      if (q % 2 == 0) {  // centre = site b
        ctr = geom::Center{{b.x - a.x, b.y - a.y, b.z - a.z}, 1};
      } else {  // centre = milieu de b et b2
        ctr = geom::Center{{b.x + b2.x - 2 * a.x, b.y + b2.y - 2 * a.y, b.z + b2.z - 2 * a.z}, 2};
      }
      expect(!in_cube(a) && judge_in_cube(ctr, a), "ancre fabriquee hors du cube, centre dans le cube", u64(q));
      expect(!tree.filtered(a, ctr), "ancre hors du cube : hors du domaine du filtre", u64(q));
      check_query(tree, a, ctr, st);
      ++ds.forged;
    }
  }
  // C6. Table gravee des bornes du contrat de filtered() (conditions (3) a (6)), filtered seul : aucune requete, les
  //     valeurs extremes sortant de la precondition de representation. Attendu : la colonne `in` en FE_TONEAREST,
  //     faux dans les modes diriges ; le juge du contrat (entiers larges) doit donner la meme colonne.
  {
    std::vector<geom::P3> pts;
    for (int i = 0; i < 100; ++i) pts.push_back(rnd_point(g, 0, kL));
    pts.push_back({0, 0, 0});
    pts.push_back({kL, kL, kL});
    const Cloud cloud = make_cloud(pts, 18);
    const SiteTree tree(cloud);
    const i128 D82 = i128{1} << 82, N100 = i128{1} << 100, LD = i128(kL) * D82;
    const i128 imax = static_cast<i128>((u128{1} << 127) - 1), imin = -imax - 1;
    const i64 lmin = std::numeric_limits<i64>::min(), lmax = std::numeric_limits<i64>::max();
    const Bound bounds[] = {
        {{0, 0, 0}, {0, 0, 0}, 1, true, "centre sur l'ancre, D = 1"},
        {{0, 0, 0}, {0, 0, 0}, 0, false, "D = 0"},
        {{0, 0, 0}, {0, 0, 0}, -1, false, "D = -1"},
        {{0, 0, 0}, {0, 0, 0}, D82, true, "D = 2^82"},
        {{0, 0, 0}, {0, 0, 0}, D82 + 1, false, "D = 2^82 + 1"},
        {{0, 0, 0}, {0, 0, 0}, imax, false, "D = i128 max"},
        {{0, 0, 0}, {0, 0, 0}, imin, false, "D = i128 min"},
        {{0, 0, 0}, {LD, 0, 0}, D82, true, "centre (L, 0, 0), N_x = L 2^82"},
        {{0, 0, 0}, {LD + 1, 0, 0}, D82, false, "centre juste au-dela de x = L"},
        {{kL, kL, kL}, {-LD, -LD, -LD}, D82, true, "centre (0, 0, 0) depuis le coin (L, L, L)"},
        {{kL, kL, kL}, {-LD - 1, 0, 0}, D82, false, "centre juste sous x = 0"},
        {{0, 0, 0}, {1, 1, 1}, D82, true, "centre a 2^-82 du coin"},
        {{0, 0, 0}, {-1, 0, 0}, D82, false, "centre a x = -2^-82"},
        {{0, 0, 0}, {N100, 0, 0}, D82, false, "N_x = 2^100 : centre a x = 2^18"},
        {{0, 0, 0}, {N100 + 1, 0, 0}, D82, false, "N_x = 2^100 + 1"},
        {{0, 0, 0}, {-N100, 0, 0}, D82, false, "N_x = -2^100"},
        {{0, 0, 0}, {-N100 - 1, 0, 0}, D82, false, "N_x = -2^100 - 1"},
        {{0, 0, 0}, {imax, 0, 0}, 1, false, "N_x = i128 max"},
        {{kL, kL, kL}, {imin, 0, 0}, D82, false, "N_x = i128 min"},
        {{kL, 0, 0}, {imax, imax, imax}, D82, false, "N = i128 max sur les trois axes"},
        {{-1, 5, 5}, {6, 0, 0}, 1, false, "ancre x = -1, centre (5, 5, 5)"},
        {{kL + 1, 5, 5}, {-6, 0, 0}, 1, false, "ancre x = L + 1, centre (L - 5, 5, 5)"},
        {{5, -1, 5}, {0, 6, 0}, 1, false, "ancre y = -1, centre (5, 5, 5)"},
        {{5, 5, kL + 1}, {0, 0, -6}, 1, false, "ancre z = L + 1, centre (5, 5, L - 5)"},
        {{0, 5, 5}, {5, 0, 0}, 1, true, "ancre sur la face x = 0"},
        {{kL, 5, 5}, {-5, 0, 0}, 1, true, "ancre sur la face x = L"},
        {{lmin, 0, 0}, {0, 0, 0}, 1, false, "ancre x = i64 min"},
        {{lmax, 0, 0}, {0, 0, 0}, 1, false, "ancre x = i64 max"},
        {{lmin, 0, 0}, {i128{5} - lmin, 0, 0}, 1, false, "ancre x = i64 min, centre (5, 0, 0)"},
        {{lmax, 0, 0}, {2 * (i128{5} - lmax), 0, 0}, 2, false, "ancre x = i64 max, centre (5, 0, 0), D = 2"},
    };
    for (const Bound& b : bounds) {
      const geom::Center ctr{{b.N[0], b.N[1], b.N[2]}, b.D};
      expect(judge_domain(true, b.a, ctr) == b.in, b.what, ds.bounds);
      bool got[kModes];
      for (int m = 0; m < kModes; ++m) {
        set_mode(m);
        got[m] = tree.filtered(b.a, ctr);
        const bool retained = std::fegetround() == kMode[m];
        set_mode(0);
        expect(retained, "borne gravee : mode d'arrondi conserve", ds.bounds * 10 + u64(m));
      }
      for (int m = 0; m < kModes; ++m) expect(got[m] == (m == 0 && b.in), b.what, ds.bounds * 10 + u64(m));
      ++ds.bounds;
    }
  }
}

// ---------------------------------------------------------------- D : arrondi lu par requete, dans le fil appelant
struct RoundingStats {
  u64 built_upward = 0, threads = 0;
};

void part_d(Stats& st, RoundingStats& rs) {
  std::mt19937_64 g(11);
  std::vector<geom::P3> pts;
  for (int i = 0; i < 300; ++i) pts.push_back(rnd_point(g, 0, kL));
  // D1. Nuage et arbre construits en FE_UPWARD, puis requetes du domaine (site, milieu) sous les quatre modes : le mode
  //     lu est celui de la requete, pas celui de la construction.
  set_mode(1);
  const Cloud cloud = make_cloud(pts, 18);
  const SiteTree tree(cloud);
  set_mode(0);
  for (int q = 0; q < 40; ++q) {
    const geom::P3 a = site(cloud, u32(g() % cloud.sites())), b = site(cloud, u32(g() % cloud.sites()));
    geom::Center ctr{{0, 0, 0}, 1};
    if (q % 2) geom::center2(a, b, ctr);
    expect(tree.filtered(a, ctr), "arbre construit en FE_UPWARD : requete du domaine filtree en FE_TONEAREST", u64(q));
    check_query(tree, a, ctr, st);
    ++rs.built_upward;
  }
  // D2. Fils : le mode qui decide est celui du fil qui execute la requete. (a) fil principal en FE_TONEAREST, fil
  //     ouvrier en FE_DOWNWARD : filtre coupe dans l'ouvrier seulement ; (b) fil principal en FE_UPWARD, ouvrier
  //     explicitement en FE_TONEAREST : filtre servi dans l'ouvrier seulement. Sorties de l'ouvrier egales au juge.
  for (int q = 0; q < 20; ++q) {
    const geom::P3 a = site(cloud, u32(g() % cloud.sites())), b = site(cloud, u32(g() % cloud.sites()));
    geom::Center ctr{{0, 0, 0}, 1};
    if (q % 2) geom::center2(a, b, ctr);
    const Judged j = judge(cloud, a, ctr);
    const bool main_nearest = q < 10;
    const int worker_mode = main_nearest ? FE_DOWNWARD : FE_TONEAREST;
    Answer r;
    bool worker_set = false, worker_retained = false;
    set_mode(main_nearest ? 0 : 1);
    std::thread worker([&] {
      worker_set = std::fesetround(worker_mode) == 0 && std::fegetround() == worker_mode;
      r = ask(tree, a, ctr);
      worker_retained = std::fegetround() == worker_mode;
    });
    worker.join();
    const bool main_filtered = tree.filtered(a, ctr);
    set_mode(0);
    if (!worker_set) {
      std::printf("ECHEC mode d'arrondi indisponible dans un fil ouvrier\n");
      std::exit(2);
    }
    expect(worker_retained, "fil ouvrier : mode d'arrondi conserve", u64(q));
    expect(r.filtered == !main_nearest, "fil ouvrier : filtered suit le mode de l'ouvrier", u64(q));
    expect(main_filtered == main_nearest, "fil principal : filtered suit le mode du fil principal", u64(q));
    st.nearest_checks += compare(r, j, ctr, 900000 + u64(q));
    ++rs.threads;
  }
}
#endif

}  // namespace

int main() {
  set_mode(0);
  part_a();
  Stats st;
  part_b(st);
  std::printf("balayage requetes=%llu hors_cube=%llu adverses=%llu nearest=%llu\n",
              static_cast<unsigned long long>(st.queries), static_cast<unsigned long long>(st.far),
              static_cast<unsigned long long>(st.mode[0].adverse),
              static_cast<unsigned long long>(st.nearest_checks));
  static const char* kKinds[5] = {"obtus", "plat", "plat_min", "variante_g1", "quelconque"};
  for (int k = 0; k < 5; ++k)
    std::printf("balayage type=%s hors_cube=%llu adverses=%llu\n", kKinds[k],
                static_cast<unsigned long long>(st.kind_far[k]), static_cast<unsigned long long>(st.kind_adverse[k]));
  u64 b_adverse[kModes];
  for (int m = 0; m < kModes; ++m) b_adverse[m] = st.mode[m].adverse;
  std::printf("balayage adverses_par_mode");
  for (int m = 0; m < kModes; ++m)
    std::printf(" %s=%llu", kModeName[m], static_cast<unsigned long long>(b_adverse[m]));
  std::printf("\n");
  // Planchers geometriques du balayage ; plancher adverse en FE_TONEAREST seulement (mesure dans chaque mode).
  bool floors = st.far >= 400 && st.queries >= 1000 && b_adverse[0] >= 300;
#ifndef MHGP10_GATE_OLD_API
  DomainStats ds;
  part_c(st, ds);
  std::printf("domaine bords=%llu sites=%llu milieux=%llu aigus=%llu tetra_internes=%llu nuage21=%llu "
              "nuage21_ancre_centre_u18=%llu ancres_fabriquees=%llu bornes_gravees=%llu\n",
              static_cast<unsigned long long>(ds.edges), static_cast<unsigned long long>(ds.sites),
              static_cast<unsigned long long>(ds.mids), static_cast<unsigned long long>(ds.acute),
              static_cast<unsigned long long>(ds.inside4), static_cast<unsigned long long>(ds.wide),
              static_cast<unsigned long long>(ds.wide_in_cube), static_cast<unsigned long long>(ds.forged),
              static_cast<unsigned long long>(ds.bounds));
  RoundingStats rs;
  part_d(st, rs);
  std::printf("arrondi arbre_construit_upward=%llu fils=%llu\n", static_cast<unsigned long long>(rs.built_upward),
              static_cast<unsigned long long>(rs.threads));
  floors = floors && ds.edges == 7 && ds.sites >= 500 && ds.mids >= 500 && ds.acute >= 100 && ds.inside4 >= 100 &&
           ds.wide >= 200 && ds.wide_in_cube >= 200 && ds.forged >= 100 && ds.bounds == 30 &&
           rs.built_upward == 40 && rs.threads == 20;
  // FE_TONEAREST : filtre et repli tous deux exerces ; modes diriges : replis d'arrondi (requetes du domaine du
  // contrat servies par le repli exact), plancher distinct des planchers geometriques.
  floors = floors && st.mode[0].filtered_in >= 2000 && st.mode[0].filtered_out >= 600;
  for (int m = 1; m < kModes; ++m) floors = floors && st.mode[m].rounding_fallbacks >= 2000;
#endif
  for (int m = 0; m < kModes; ++m)
    std::printf("mode=%s requetes=%llu filtre=%llu repli=%llu replis_arrondi=%llu adverses=%llu\n", kModeName[m],
                static_cast<unsigned long long>(st.queries), static_cast<unsigned long long>(st.mode[m].filtered_in),
                static_cast<unsigned long long>(st.mode[m].filtered_out),
                static_cast<unsigned long long>(st.mode[m].rounding_fallbacks),
                static_cast<unsigned long long>(st.mode[m].adverse));
  expect(std::fegetround() == FE_TONEAREST, "mode d'arrondi restaure en fin de porte");
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
