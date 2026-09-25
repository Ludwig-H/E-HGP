// Auditeur C, 25 septembre 2026 — juge a supports independants sur trames entieres (audit, hors produit).
//
// Specification : audit B `b_full_raw_completeness_gap_20260924`. Les epingles CPU des trames brutes et les
// jumeaux moteur/lots ne comparent que les cles PRESENTEES : une omission commune leur echappe. Ce juge
// fabrique des supports depuis les seules coordonnees et IDs d'entree, jamais depuis les boules emises :
//   - 32 ancres, figees avant tout jugement : 16 d'une permutation a graine publiee, 8 de la queue de densite
//     (plus grande distance au 10e voisin, departage par ID), 8 des huit quantiles de distance au centre entier
//     de la boite englobante (minimum d'un hachage de l'ID par quantile) ;
//   - pour chaque ancre, ses 12 plus proches voisins et les partenaires de rangs 32, 64, 128, 256 dans l'ordre
//     (distance^2, ID) ; tous les supports de tailles 2, 3 et 4 contenant l'ancre (au plus 32 x 696).
// Pour chaque support, un oracle distinct du produit :
//   - resout le centre par Gram/Cramer en entiers multiprecision (Boost cpp_int) ; support POSITIF si et
//     seulement si ses poids barycentriques sont tous strictement positifs (centre dans l'interieur relatif) ;
//   - reduit la sphere a sa forme puissance primitive A|x|^2 + B.x + C (A > 0, pgcd 1) ;
//   - recense sur TOUS les sites de la trame avec son propre arbre k-d (filtre flottant conservateur, signe
//     entier exact aux feuilles, arret des qu'il y a plus de K-1 interieurs) ;
//   - calcule q_min par les sous-ensembles positifs de tailles 2 a 4 de la coquille (coquille <= 24 ; voir v2).
// Toute boule admissible (p + q_min <= K + 1) doit etre au catalogue de la chaine avec meme cle, meme rayon exact
// (niveau), meme p, meme q_min (arite) et les MEMES ensembles d'IDs d'interieur et de coquille (IDs = rangs
// d'entree ; bijection de l'index verifiee avant tout). Marqueurs : MISSING_KEY, LEVEL_DIFF, INTERIOR_DIFF,
// SHELL_IDS_DIFF, ARITY_DIFF, SHELL_OVER_12. Strates non vacantes exigees (sinon code 3) : q2 reguliere a
// p = K-1, q3 reguliere a p = K-2 (dont une longue, arete max >= 1600 unites), q4 positive a p <= K-3.
// Mutants (copies du catalogue) : --inject=drop-q2 | drop-q3 | drop-q4 (retrait d'une cle trouvee de la strate,
// MISSING_KEY), --inject=shell-sub (un ID de coquille substitue a taille egale, SHELL_IDS_DIFF), --inject=ext-trim
// (coquille etendue trouvee tronquee a son arite, SHELL_IDS_DIFF) et --inject=ext-arity (arite d'une boule etendue
// trouvee faussee, ARITY_DIFF).
// Revue adverse (v2) : q_min calcule aussi pour les coquilles de 13 a 24 sites, SHELL_OVER_12 seulement dans une
// fenetre admissible et compte par cle distincte (au-dela de 24 sites : SHELL_DOMAIN, non juge, code 3) ; chaque
// ecart imprime la cle, l'ancre, le support et les IDs des deux cotes ; condense du catalogue, empreinte FNV de
// l'entree (encodage de la sonde), fils et leviers publies, --expect-catalogue-digest=HEX (refus en code 2 sur
// ecart) ; boules etendues comptees et --expect-extended=N (compte exact, sinon code 3) ; mode fixture-cospheric.
// Contre-verification (v3) : coquille > 24 sites jugee par son admissibilite seule (budget K+1-p <= 3, sous-ensembles
// de taille 2 ou 3, jusqu'a 96 sites ; au-dela SHELL_DOMAIN par cle distincte) ; grille d'ancres completee jusqu'a 32
// (8 queue de densite et 8 quantiles distincts, sinon ANCHOR_GRID_SHORT) ; comptes par strate et temoins cote oracle
// imprimes AVANT l'appel de la chaine (EXPECTED, WITNESS), VACUOUS nomme la strate vacante ; q4 temoin reguliere
// seulement ; en mode file, --expect-catalogue-digest obligatoire (sauf --unpinned), valeur vide refusee, casse
// ignoree, --expect-input-fnv=HEX ; bornes des IDs du catalogue ; mutants auto-verifies (code 4 si et seulement si
// l'unique ecart est le marqueur attendu, sinon MUTANT_NOT_KILLED, code 1), plus --inject=level (LEVEL_DIFF) et
// --inject=interior-sub (INTERIOR_DIFF) ; ext-trim vise la plus grande coquille etendue, ext-arity une q3 etendue.
//
//   raw_support_judge file <cloud.u32le> <K> <workers> --expect-catalogue-digest=HEX|--unpinned [--expect-input-fnv=HEX]
//                     [--seed=S] [--inject=...] [--expect-extended=N]
//   raw_support_judge family <uniform|terrain|clusters> <n> <K> <workers> [options]
//   raw_support_judge fixture-cospheric <K> <workers> [options]   (coquilles etendues gravees, 417 sites)
//
// Recherche adverse bornee et independante, PAS un theoreme de completude, ni une preuve GPU/G4.
// Code 0 conforme ; 1 ecart (ou mutant non tue) ; 2 argument, entree, chaine ou condense ; 3 strate vacante, domaine,
// grille courte ou compte etendu ; 4 mutant tue par son seul marqueur attendu.
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <string>
#include <tuple>
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

std::uint64_t splitmix(std::uint64_t z) {
  z += 0x9e3779b97f4a7c15ull;
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
  return z ^ (z >> 31);
}

// Empreinte d'entree de la sonde : FNV-1a 64 de n puis x, y, z, chacun en u64 petit-boutiste.
std::uint64_t input_fnv(const std::vector<Point3>& pts) {
  std::uint64_t h = 14695981039346656037ull;
  const auto word = [&](std::uint64_t w) { for (int b = 0; b < 8; ++b) { h ^= (w >> (8 * b)) & 0xffu; h *= 1099511628211ull; } };
  word(pts.size());
  for (const auto& q : pts) { word(std::uint64_t(q.x)); word(std::uint64_t(q.y)); word(std::uint64_t(q.z)); }
  return h;
}

// Fixture cospherique des juges q2/q3 v9 (configurations decorrelees) : cube (q2, coquille 8, et six faces), triangle
// aigu et point hors plan (q3, coquille 4), cinq points de x^2+y^2+z^2 = 9 (q4, coquille 5) ; fond de 400 points.
// Avec la graine par defaut, les ancres n'atteignent que la boule du cube et quatre faces (x=-1, x=1, y=-1, z=1) ; les
// 8 boules etendues comptees sont ces cinq, la q3 et la q4 gravees, plus une q2 fortuite {12,13,15} (angle droit en 12
// dans le groupe de cinq). --expect-extended=8 epingle ce compte observe, il ne le derive pas.
std::vector<Point3> fixture_cospheric() {
  using C = mhgp9::gen::Coordinate;
  std::vector<Point3> pts;
  const auto add = [&](int ox, int oy, int oz, int x, int y, int z) {
    pts.push_back(Point3{static_cast<C>(ox + 100 * x), static_cast<C>(oy + 100 * y), static_cast<C>(oz + 100 * z)});
  };
  for (const int x : {-1, 1}) for (const int y : {-1, 1}) for (const int z : {-1, 1}) add(100000, 70000, 130000, x, y, z);
  const int tri[4][3] = {{5, 0, 0}, {-3, 4, 0}, {-3, -4, 0}, {0, 0, 5}};
  for (const auto& q : tri) add(150000, 125000, 60000, q[0], q[1], q[2]);
  const int five[5][3] = {{-3, 0, 0}, {-2, -2, -1}, {-2, -2, 1}, {-1, 2, -2}, {2, -1, 2}};
  for (const auto& q : five) add(205000, 185000, 115000, q[0], q[1], q[2]);
  std::uint64_t state = 0x9e3779b97f4a7c15ull;
  for (int i = 0; i < 400; ++i) {
    C c[3];
    for (auto& v : c) { state = state * 6364136223846793005ull + 1442695040888963407ull; v = static_cast<C>((state >> 33) % 30000); }
    pts.push_back(Point3{c[0], c[1], c[2]});
  }
  return pts;
}

using Key = std::array<i128, 5>;

cpp_int big(i128 v) {
  const bool neg = v < 0;
  const u128 u = neg ? (u128)(-(v + 1)) + 1 : (u128)v;
  cpp_int r = static_cast<std::uint64_t>(u >> 64);
  r <<= 64;
  r += static_cast<std::uint64_t>(u);
  return neg ? cpp_int(-r) : r;
}

// Index propre du juge : arbre k-d sur les coordonnees d'entree (indices = rangs d'entree = IDs).
struct Node { i64 lo[3], hi[3]; std::uint32_t begin, end, left, right; };
struct Tree {
  std::vector<std::array<i64, 3>> p;
  std::vector<std::uint32_t> idx;
  std::vector<Node> nodes;
  std::uint32_t build(std::uint32_t b, std::uint32_t e) {
    Node nd{};
    for (int k = 0; k < 3; ++k) { nd.lo[k] = INT64_MAX; nd.hi[k] = INT64_MIN; }
    for (std::uint32_t i = b; i < e; ++i)
      for (int k = 0; k < 3; ++k) { nd.lo[k] = std::min(nd.lo[k], p[idx[i]][k]); nd.hi[k] = std::max(nd.hi[k], p[idx[i]][k]); }
    nd.begin = b; nd.end = e; nd.left = nd.right = UINT32_MAX;
    const std::uint32_t id = static_cast<std::uint32_t>(nodes.size());
    nodes.push_back(nd);
    if (e - b > 8) {
      int ax = 0;
      for (int k = 1; k < 3; ++k) if (nd.hi[k] - nd.lo[k] > nd.hi[ax] - nd.lo[ax]) ax = k;
      const std::uint32_t mid = b + (e - b) / 2;
      std::nth_element(idx.begin() + b, idx.begin() + mid, idx.begin() + e, [&](std::uint32_t l, std::uint32_t r) {
        return p[l][ax] < p[r][ax] || (p[l][ax] == p[r][ax] && l < r);
      });
      const std::uint32_t l = build(b, mid), r = build(mid, e);
      nodes[id].left = l; nodes[id].right = r;
    }
    return id;
  }
  void init(const std::vector<Point3>& pts) {
    p.resize(pts.size());
    for (std::size_t i = 0; i < pts.size(); ++i) p[i] = {pts[i].x, pts[i].y, pts[i].z};
    idx.resize(pts.size()); std::iota(idx.begin(), idx.end(), 0u); nodes.clear();
    build(0, static_cast<std::uint32_t>(pts.size()));
  }
  i64 d2(std::uint32_t a, std::uint32_t b) const {
    i64 s = 0;
    for (int k = 0; k < 3; ++k) { const i64 d = p[a][k] - p[b][k]; s += d * d; }
    return s;
  }
  // Les k plus proches sites de q (hors q), ordre (distance^2, ID), exact.
  std::vector<std::uint32_t> knn(std::uint32_t q, std::size_t k) const {
    using E = std::pair<i64, std::uint32_t>;
    std::priority_queue<E> best;  // max-tas des k meilleurs
    std::vector<std::uint32_t> stack{0};
    while (!stack.empty()) {
      const Node& n = nodes[stack.back()]; stack.pop_back();
      i64 dmin = 0;
      for (int a = 0; a < 3; ++a) {
        const i64 c = p[q][a];
        const i64 d = c < n.lo[a] ? n.lo[a] - c : c > n.hi[a] ? c - n.hi[a] : 0;
        dmin += d * d;
      }
      if (best.size() == k && dmin > best.top().first) continue;
      if (n.left == UINT32_MAX) {
        for (std::uint32_t i = n.begin; i < n.end; ++i) {
          const std::uint32_t s = idx[i];
          if (s == q) continue;
          const E e{d2(q, s), s};
          if (best.size() < k) best.push(e);
          else if (e < best.top()) { best.pop(); best.push(e); }
        }
      } else { stack.push_back(n.left); stack.push_back(n.right); }
    }
    std::vector<E> v;
    while (!best.empty()) { v.push_back(best.top()); best.pop(); }
    std::sort(v.begin(), v.end());
    std::vector<std::uint32_t> out;
    for (const auto& e : v) out.push_back(e.second);
    return out;
  }
};

// Sphere d'un support : centre O = p0 + Pv / D (Gram/Cramer), forme primitive (A, B, C), poids positifs ?
struct Ball {
  bool nondegenerate = false, positive = false;
  cpp_int D, Pv[3];            // centre non reduit, rayon^2 = |Pv|^2 / D^2
  std::array<i64, 3> p0{};
  cpp_int A, B[3], C;          // forme puissance primitive, A > 0
};

cpp_int det3(const cpp_int m[3][3]) {
  return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
         m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

Ball solve(const Tree& t, const std::vector<std::uint32_t>& s) {
  Ball b;
  const std::size_t m = s.size() - 1;  // nombre d'aretes depuis p0
  b.p0 = t.p[s[0]];
  cpp_int u[3][3];
  for (std::size_t i = 0; i < m; ++i)
    for (int k = 0; k < 3; ++k) u[i][k] = cpp_int(t.p[s[i + 1]][k] - b.p0[k]);
  cpp_int G[3][3], h[3];
  for (std::size_t i = 0; i < 3; ++i) for (std::size_t j = 0; j < 3; ++j) G[i][j] = (i == j) ? 1 : 0;
  for (std::size_t i = 0; i < m; ++i) {
    h[i] = u[i][0] * u[i][0] + u[i][1] * u[i][1] + u[i][2] * u[i][2];
    for (std::size_t j = 0; j < m; ++j) G[i][j] = 2 * (u[i][0] * u[j][0] + u[i][1] * u[j][1] + u[i][2] * u[j][2]);
  }
  // Le bloc m x m est plonge dans une matrice 3 x 3 d'identite : meme determinant, meme solution.
  const cpp_int D = det3(G);
  if (D == 0) return b;
  b.nondegenerate = true;
  cpp_int dets[3], sum = 0;
  for (std::size_t i = 0; i < m; ++i) {
    cpp_int Gi[3][3];
    for (std::size_t r = 0; r < 3; ++r) for (std::size_t c = 0; c < 3; ++c) Gi[r][c] = G[r][c];
    for (std::size_t r = 0; r < m; ++r) Gi[r][i] = h[r];
    dets[i] = det3(Gi);
    sum += dets[i];
  }
  bool pos = D * (D - sum) > 0;
  for (std::size_t i = 0; i < m; ++i) pos = pos && D * dets[i] > 0;
  b.positive = pos;
  b.D = D;
  for (int k = 0; k < 3; ++k) { b.Pv[k] = 0; for (std::size_t i = 0; i < m; ++i) b.Pv[k] += dets[i] * u[i][k]; }
  // s(x) = D|x-p0|^2 - 2(x-p0).Pv = D|x|^2 - 2(D p0 + Pv).x + D|p0|^2 + 2 p0.Pv ; puissance(x) = s(x) / D.
  cpp_int f[5];
  f[0] = D;
  f[4] = 0;
  for (int k = 0; k < 3; ++k) {
    f[1 + k] = -2 * (D * b.p0[k] + b.Pv[k]);
    f[4] += D * b.p0[k] * b.p0[k] + 2 * b.p0[k] * b.Pv[k];
  }
  cpp_int g = abs(f[0]);
  for (int i = 1; i < 5; ++i) g = gcd(g, abs(f[i]));
  if (f[0] < 0) g = -g;
  b.A = f[0] / g; b.C = f[4] / g;
  for (int k = 0; k < 3; ++k) b.B[k] = f[1 + k] / g;
  return b;
}

// Puissance primitive au site (signe exact) : i128 dans les bornes u18 des cles, cpp_int sinon.
struct Form {
  bool small = false;
  i128 A = 0, B[3] = {0, 0, 0}, C = 0;
  cpp_int bA, bB[3], bC;
  double cx = 0, cy = 0, cz = 0, r2 = 0;
  int sign(const std::array<i64, 3>& x) const {
    if (small) {
      const i128 v = A * (i128(x[0]) * x[0] + i128(x[1]) * x[1] + i128(x[2]) * x[2]) + B[0] * x[0] + B[1] * x[1] + B[2] * x[2] + C;
      return v < 0 ? -1 : v > 0 ? 1 : 0;
    }
    const cpp_int v = bA * (cpp_int(x[0]) * x[0] + cpp_int(x[1]) * x[1] + cpp_int(x[2]) * x[2]) + bB[0] * x[0] +
                      bB[1] * x[1] + bB[2] * x[2] + bC;
    return v < 0 ? -1 : v > 0 ? 1 : 0;
  }
};

bool fits(const cpp_int& v, int bits) { return abs(v) < (cpp_int(1) << bits); }

Form make_form(const Ball& b) {
  Form f;
  f.bA = b.A; f.bC = b.C;
  for (int k = 0; k < 3; ++k) f.bB[k] = b.B[k];
  f.small = fits(b.A, 76) && fits(b.B[0], 96) && fits(b.B[1], 96) && fits(b.B[2], 96) && fits(b.C, 116);
  if (f.small) {
    const auto to128 = [](const cpp_int& v) {
      const bool neg = v < 0;
      cpp_int a = neg ? cpp_int(-v) : v;
      const u128 lo = static_cast<std::uint64_t>(a & cpp_int(0xffffffffffffffffull));
      const u128 hi = static_cast<std::uint64_t>(a >> 64);
      const i128 r = static_cast<i128>((hi << 64) | lo);
      return neg ? -r : r;
    };
    f.A = to128(b.A); f.C = to128(b.C);
    for (int k = 0; k < 3; ++k) f.B[k] = to128(b.B[k]);
  }
  const double A = b.A.convert_to<double>();
  f.cx = -b.B[0].convert_to<double>() / (2 * A);
  f.cy = -b.B[1].convert_to<double>() / (2 * A);
  f.cz = -b.B[2].convert_to<double>() / (2 * A);
  const double dx = double(b.p0[0]) - f.cx, dy = double(b.p0[1]) - f.cy, dz = double(b.p0[2]) - f.cz;
  f.r2 = dx * dx + dy * dy + dz * dz;
  return f;
}

// Recensement sur tout le nuage : faux si plus de cap interieurs (arret). Filtre flottant conservateur.
bool census(const Tree& t, const Form& f, std::size_t cap, std::vector<std::uint32_t>& in, std::vector<std::uint32_t>& sh) {
  in.clear(); sh.clear();
  const double margin = 1e-6 * (f.r2 + 1.0) + 4.0;
  std::vector<std::uint32_t> stack{0};
  const double c[3] = {f.cx, f.cy, f.cz};
  while (!stack.empty()) {
    const Node& n = t.nodes[stack.back()]; stack.pop_back();
    double dmin = 0;
    for (int k = 0; k < 3; ++k) {
      const double lo = double(n.lo[k]), hi = double(n.hi[k]);
      const double d = c[k] < lo ? lo - c[k] : c[k] > hi ? c[k] - hi : 0.0;
      dmin += d * d;
    }
    if (dmin > f.r2 + margin) continue;
    if (n.left == UINT32_MAX) {
      for (std::uint32_t i = n.begin; i < n.end; ++i) {
        const int sg = f.sign(t.p[t.idx[i]]);
        if (sg < 0) { in.push_back(t.idx[i]); if (in.size() > cap) return false; }
        else if (sg == 0) sh.push_back(t.idx[i]);
      }
    } else { stack.push_back(n.left); stack.push_back(n.right); }
  }
  std::sort(in.begin(), in.end()); std::sort(sh.begin(), sh.end());
  return true;
}

// Meme centre ? (p0 + Pv/D) == (q0 + Qv/E)  <=>  D E (p0 - q0) + E Pv - D Qv == 0.
bool same_centre(const Ball& a, const Ball& b) {
  for (int k = 0; k < 3; ++k)
    if (a.D * b.D * cpp_int(a.p0[k] - b.p0[k]) + b.D * a.Pv[k] - a.D * b.Pv[k] != 0) return false;
  return true;
}

// q_min : plus petite taille d'un sous-ensemble positif de la coquille dont le centre est celui de la boule.
unsigned qmin_of(const Tree& t, const Ball& ball, const std::vector<std::uint32_t>& shell, unsigned qmax = 4) {
  const std::size_t n = shell.size();
  for (unsigned q = 2; q <= qmax && q <= n; ++q) {
    std::vector<std::size_t> c(q);
    std::iota(c.begin(), c.end(), std::size_t{0});
    while (true) {
      std::vector<std::uint32_t> sub;
      for (const auto i : c) sub.push_back(shell[i]);
      const Ball b = solve(t, sub);
      if (b.nondegenerate && b.positive && same_centre(b, ball)) return q;
      int i = int(q) - 1;
      while (i >= 0 && c[i] == n - q + i) --i;
      if (i < 0) break;
      ++c[i];
      for (std::size_t j = i + 1; j < q; ++j) c[j] = c[j - 1] + 1;
    }
  }
  return 99;
}

struct Expected {
  std::vector<std::uint32_t> in, sh, support;
  unsigned q = 0;
  cpp_int D, Pv[3];
  i64 max_edge2 = 0;
};

std::string ids(const std::vector<std::uint32_t>& v) {
  std::string r;
  for (const auto x : v) r += (r.empty() ? "" : ",") + std::to_string(x);
  return "[" + r + "]";
}

std::string key_text(const std::array<i128, 5>& k) {
  std::string r;
  for (const auto v : k) r += (r.empty() ? "" : ",") + big(v).str();
  return "(" + r + ")";
}


// Garde d'index (bijection des PointIds puis positions), comme les juges q2/q3 v8.
int check_index(const std::string& label, const std::vector<Point3>& pts, const mhgp9::tower::CloudIndex& ix,
                std::vector<std::uint32_t>& id_of_u) {
  const std::size_t n = pts.size();
  if (ix.has_duplicate_positions() || ix.upos.size() != n) { std::printf("%s INDEX_SHAPE\n", label.c_str()); return 2; }
  std::vector<bool> seen(n, false);
  id_of_u.assign(n, 0);
  for (std::size_t u = 0; u < n; ++u) {
    const auto id = static_cast<std::uint64_t>(ix.point_id(static_cast<std::int32_t>(u)));
    if (id >= n || seen[id]) { std::printf("%s INDEX_ID\n", label.c_str()); return 2; }
    seen[id] = true;
    const auto& q = pts[id];
    if (ix.upos[u].x != q.x || ix.upos[u].y != q.y || ix.upos[u].z != q.z) {
      std::printf("%s INDEX_POSITION\n", label.c_str());
      return 2;
    }
    id_of_u[u] = static_cast<std::uint32_t>(id);
  }
  return 0;
}

struct Opts {
  std::uint64_t seed = 0x5eed2026c0ffee25ull;
  std::string inject;
  std::string expect_digest, expect_fnv;
  bool digest_required = false, unpinned = false;
  long long expect_extended = -1;
};

std::string lower(std::string v) {
  for (auto& ch : v) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  return v;
}

int run(const std::string& label, const std::vector<Point3>& pts, unsigned K, std::size_t workers, const Opts& op) {
  const std::uint64_t seed = op.seed;
  const std::string& inject = op.inject;
  if (K < 3 || K > 10 || pts.size() < 300) { std::fprintf(stderr, "K must be in 3..10, n >= 300\n"); return 2; }
  if (op.digest_required && op.expect_digest.empty() && !op.unpinned) {
    std::fprintf(stderr, "file mode needs --expect-catalogue-digest=HEX (or --unpinned)\n");
    return 2;
  }
  const std::size_t n = pts.size();
  char fnv[17];
  std::snprintf(fnv, sizeof fnv, "%016llx", (unsigned long long)input_fnv(pts));
  if (!op.expect_fnv.empty() && lower(op.expect_fnv) != fnv) {
    std::printf("%s INPUT_FNV_DIFF got=%s expected=%s\n", label.c_str(), fnv, op.expect_fnv.c_str());
    return 2;
  }
  // ---- 1. Ancres et supports, depuis la seule entree (avant tout jugement).
  Tree t; t.init(pts);
  std::vector<std::pair<std::uint32_t, const char*>> anchors;
  const auto taken = [&](std::uint32_t a) {
    for (const auto& x : anchors) if (x.first == a) return true;
    return false;
  };
  const auto add_anchor = [&](std::uint32_t a, const char* why) {
    if (taken(a)) return false;
    anchors.push_back({a, why});
    return true;
  };
  {
    std::vector<std::uint32_t> perm(n);
    std::iota(perm.begin(), perm.end(), 0u);
    std::uint64_t st = seed;
    for (std::size_t i = 0; i < 16; ++i) { st = splitmix(st); std::swap(perm[i], perm[i + st % (n - i)]); add_anchor(perm[i], "random"); }
    std::vector<std::pair<i64, std::uint32_t>> dens(n);
    for (std::uint32_t s = 0; s < n; ++s) { const auto nn = t.knn(s, 10); dens[s] = {-t.d2(s, nn.back()), s}; }
    std::sort(dens.begin(), dens.end());  // plus grande distance d'abord, puis plus petit ID
    for (std::size_t i = 0, got = 0; i < n && got < 8; ++i) got += add_anchor(dens[i].second, "density_tail");  // completee
    i64 lo[3] = {INT64_MAX, INT64_MAX, INT64_MAX}, hi[3] = {INT64_MIN, INT64_MIN, INT64_MIN};
    for (const auto& p : t.p) for (int k = 0; k < 3; ++k) { lo[k] = std::min(lo[k], p[k]); hi[k] = std::max(hi[k], p[k]); }
    const i64 cc[3] = {(lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, (lo[2] + hi[2]) / 2};
    std::vector<std::pair<i64, std::uint32_t>> rad(n);
    for (std::uint32_t s = 0; s < n; ++s) {
      i64 d = 0;
      for (int k = 0; k < 3; ++k) d += (t.p[s][k] - cc[k]) * (t.p[s][k] - cc[k]);
      rad[s] = {d, s};
    }
    std::sort(rad.begin(), rad.end());
    for (std::size_t qd = 0; qd < 8; ++qd) {
      std::uint32_t pick = UINT32_MAX;
      std::uint64_t best = UINT64_MAX;
      for (std::size_t i = n * qd / 8; i < n * (qd + 1) / 8; ++i) {
        if (taken(rad[i].second)) continue;  // site deja ancre : minimum pris parmi les autres
        const std::uint64_t h = splitmix(rad[i].second ^ seed);
        if (h < best) { best = h; pick = rad[i].second; }
      }
      if (pick != UINT32_MAX) add_anchor(pick, "radius_quantile");
    }
  }
  std::printf("%s ANCHORS seed=%016llx count=%zu", label.c_str(), (unsigned long long)seed, anchors.size());
  for (const auto& a : anchors) std::printf(" %u:%s", a.first, a.second);
  std::printf("\n");
  if (anchors.size() != 32) { std::printf("%s ANCHOR_GRID_SHORT count=%zu\n", label.c_str(), anchors.size()); return 3; }
  // ---- 2. Oracle : boules admissibles distinctes.
  std::map<Key, Expected> expected;
  std::uint64_t supports = 0, positive = 0, degenerate = 0, admissible = 0, too_deep = 0, big_forms = 0;
  std::map<Key, std::size_t> over12_keys;  // coquille > 12 dans une fenetre admissible, par cle distincte
  std::map<Key, std::size_t> domain_keys;  // coquille > 96 dont l'admissibilite n'est pas decidee, par cle distincte
  std::map<Key, unsigned> big_q;           // coquilles > 24 : q decide (ou borne) par cle, calcule une fois
  std::vector<std::uint32_t> in, sh;
  for (const auto& a : anchors) {
    const auto nn = t.knn(a.first, 256);
    std::vector<std::uint32_t> part(nn.begin(), nn.begin() + std::min<std::size_t>(12, nn.size()));
    for (const std::size_t r : {32, 64, 128, 256}) if (r <= nn.size()) part.push_back(nn[r - 1]);
    const std::size_t m = part.size();
    std::vector<std::vector<std::uint32_t>> sup;
    for (std::size_t i = 0; i < m; ++i) {
      sup.push_back({a.first, part[i]});
      for (std::size_t j = i + 1; j < m; ++j) {
        sup.push_back({a.first, part[i], part[j]});
        for (std::size_t k = j + 1; k < m; ++k) sup.push_back({a.first, part[i], part[j], part[k]});
      }
    }
    for (const auto& s : sup) {
      ++supports;
      const Ball b = solve(t, s);
      if (!b.nondegenerate) { ++degenerate; continue; }
      if (!b.positive) continue;
      ++positive;
      const Form f = make_form(b);
      if (!f.small) ++big_forms;
      if (!census(t, f, K - 1, in, sh)) { ++too_deep; continue; }
      for (const auto x : s)
        if (!std::binary_search(sh.begin(), sh.end(), x)) { std::printf("%s INTERNAL_SUPPORT_OFF_SHELL\n", label.c_str()); return 2; }
      const std::size_t p = in.size();
      if (!f.small) { std::printf("%s KEY_OUTSIDE_I128 anchor=%u support=%s\n", label.c_str(), a.first, ids(s).c_str()); return 2; }
      const Key key = {f.A, f.B[0], f.B[1], f.B[2], f.C};
      unsigned q = 0;
      if (sh.size() <= 24) {
        q = qmin_of(t, b, sh);
        if (q > 4) { std::printf("%s INTERNAL_QMIN\n", label.c_str()); return 2; }
      } else {
        // Coquille > 24 : seule l'admissibilite compte (la chaine refuse au-dela de 12 sites). Comme p <= K-1 et que le
        // support est lui-meme positif, q_min <= |support| ; il suffit de chercher un sous-ensemble positif de taille
        // au plus K+1-p (2 ou 3 quand le support ne suffit pas), en O(|coquille|^3) au pire.
        const auto it = big_q.find(key);
        if (it != big_q.end()) {
          q = it->second;
        } else {
          const std::size_t budget = K + 1 - p;
          if (budget >= s.size()) q = static_cast<unsigned>(s.size());  // borne superieure : admissible
          else if (sh.size() <= 96) q = qmin_of(t, b, sh, static_cast<unsigned>(budget));  // 99 si aucune
          else q = 0;  // non decide
          big_q.emplace(key, q);
        }
        if (q == 0) {
          if (domain_keys.emplace(key, sh.size()).second)
            std::printf("%s SHELL_DOMAIN key=%s p=%zu shell=%zu anchor=%u support=%s\n", label.c_str(), key_text(key).c_str(),
                        p, sh.size(), a.first, ids(s).c_str());
          continue;
        }
      }
      if (p + q > K + 1) continue;
      if (sh.size() > 12) {  // admissible et coquille > 12 : la chaine aurait du refuser (chain_shell_above_12)
        if (over12_keys.emplace(key, sh.size()).second)
          std::printf("%s SHELL_OVER_12 key=%s p=%zu q%s%u shell=%zu anchor=%u support=%s interior=%s shell_ids=%s\n",
                      label.c_str(), key_text(key).c_str(), p, sh.size() > 24 ? "<=" : "=", q, sh.size(), a.first,
                      ids(s).c_str(), ids(in).c_str(), ids(sh).c_str());
        continue;
      }
      ++admissible;
      auto it = expected.find(key);
      if (it == expected.end()) {
        Expected e;
        e.in = in; e.sh = sh; e.q = q; e.D = b.D; e.support = s;
        for (int k = 0; k < 3; ++k) e.Pv[k] = b.Pv[k];
        for (std::size_t i = 0; i < sh.size(); ++i)
          for (std::size_t j = i + 1; j < sh.size(); ++j) e.max_edge2 = std::max(e.max_edge2, t.d2(sh[i], sh[j]));
        expected.emplace(key, std::move(e));
      } else if (it->second.in != in || it->second.sh != sh || it->second.q != q) {
        std::printf("%s INTERNAL_INCONSISTENT_BALL\n", label.c_str());
        return 2;
      }
    }
  }
  // ---- 2 bis. Strates cote oracle, publiees avant tout appel de la chaine. Temoins : q2 reguliere a p = K-1, q3
  // reguliere a p = K-2 (longue : arete max >= 1600 unites), q4 reguliere a p <= K-3 ; les boules etendues a part.
  const auto stratum = [&](const Expected& e) -> int {
    const std::size_t p = e.in.size();
    if (e.sh.size() != e.q) return 0;
    if (e.q == 2 && p == K - 1) return 2;
    if (e.q == 3 && p == K - 2) return e.max_edge2 >= 1600LL * 1600LL ? 31 : 3;
    if (e.q == 4 && p + 3 <= K) return 4;
    return 0;
  };
  std::array<std::uint64_t, 32> exp_strata{}, exp_ext{};
  std::array<const Key*, 32> witness{};
  for (const auto& [key, e] : expected) {
    if (e.sh.size() > e.q) ++exp_ext[e.q];
    const int st = stratum(e);
    ++exp_strata[static_cast<std::size_t>(st)];
    if (st && !witness[static_cast<std::size_t>(st)]) witness[static_cast<std::size_t>(st)] = &key;
  }
  std::printf("%s EXPECTED keys=%zu q2crit=%llu q3crit=%llu q3crit_long=%llu q4pos=%llu extended_q2=%llu extended_q3=%llu "
              "extended_q4=%llu\n", label.c_str(), expected.size(), (unsigned long long)exp_strata[2],
              (unsigned long long)(exp_strata[3] + exp_strata[31]), (unsigned long long)exp_strata[31],
              (unsigned long long)exp_strata[4], (unsigned long long)exp_ext[2], (unsigned long long)exp_ext[3],
              (unsigned long long)exp_ext[4]);
  for (const auto& [st, name] : std::vector<std::pair<int, const char*>>{{2, "q2crit"}, {3, "q3crit"}, {31, "q3crit_long"}, {4, "q4pos"}}) {
    if (!witness[static_cast<std::size_t>(st)]) continue;
    const Expected& e = expected.at(*witness[static_cast<std::size_t>(st)]);
    std::printf("%s WITNESS %s key=%s p=%zu q=%u support=%s interior=%s shell=%s\n", label.c_str(), name,
                key_text(*witness[static_cast<std::size_t>(st)]).c_str(), e.in.size(), e.q, ids(e.support).c_str(),
                ids(e.in).c_str(), ids(e.sh).c_str());
  }
  // ---- 3. Catalogue de la chaine (moteur, leviers par defaut) et bijection de l'index.
  mhgp9::ChainOptions o;
  o.kmax = K; o.workers = workers; o.tower_static_threads = static_cast<int>(workers);
  o.keep_catalogue = true; o.run_tower = false; o.catalogue_digest = true;
  const auto r = mhgp9::run_tower_chain(pts, o);
  if (r.status != mhgp9::ChainStatus::kComplete) {
    std::printf("%s chain_status=%s reason=%s\n", label.c_str(), mhgp9::chain_status_name(r.status), r.reason.c_str());
    return 2;
  }
  char digest[17];
  std::snprintf(digest, sizeof digest, "%016llx", (unsigned long long)r.catalogue_digest);
  std::printf("%s CATALOGUE catalogue_digest=%s input_fnv=%s s=%u workers=%zu levers=default(engine)\n", label.c_str(),
              digest, fnv, o.separation_s, workers);
  if (!op.expect_digest.empty() && lower(op.expect_digest) != digest) {
    std::printf("%s CATALOGUE_DIGEST_DIFF got=%s expected=%s\n", label.c_str(), digest, op.expect_digest.c_str());
    return 2;
  }
  std::vector<mhgp9::tower::InputPoint> input(n);
  for (std::size_t i = 0; i < n; ++i)
    input[i] = mhgp9::tower::InputPoint{static_cast<mhgp9::tower::PointId>(i), P3{pts[i].x, pts[i].y, pts[i].z}};
  const auto ix = mhgp9::tower::build_cloud_index(input);
  std::vector<std::uint32_t> id_of_u;
  if (const int bad = check_index(label, pts, ix, id_of_u)) return bad;
  std::map<Key, std::size_t> by_key;
  std::vector<BallData> cat = r.catalogue_balls;
  for (std::size_t i = 0; i < cat.size(); ++i) {
    const auto& k = cat[i].key;
    if (!by_key.emplace(Key{k.a, k.b[0], k.b[1], k.b[2], k.c}, i).second) {
      std::printf("%s DUPLICATE_KEY key=%s\n", label.c_str(), key_text(Key{k.a, k.b[0], k.b[1], k.b[2], k.c}).c_str());
      return 1;
    }
  }
  // ---- 4. Mutants (copies du catalogue), chacun avec son marqueur attendu.
  const char* want_marker = nullptr;
  if (!inject.empty()) {
    const bool ext = inject == "ext-trim" || inject == "ext-arity";
    const int want = inject == "drop-q2" ? 2 : inject == "drop-q3" ? 3 : inject == "drop-q4" ? 4 : inject == "shell-sub" ? 3
                   : inject == "level" ? 3 : inject == "interior-sub" ? 1 : ext ? 0 : -1;
    if (want < 0) { std::fprintf(stderr, "unknown --inject\n"); return 2; }
    want_marker = inject.rfind("drop-", 0) == 0 ? "MISSING_KEY" : inject == "ext-arity" ? "ARITY_DIFF"
                : inject == "level" ? "LEVEL_DIFF" : inject == "interior-sub" ? "INTERIOR_DIFF" : "SHELL_IDS_DIFF";
    // Cible : premiere cle trouvee de la strate (ordre des cles) ; ext-trim : plus grande coquille etendue ; ext-arity :
    // premiere q3 etendue, a defaut premiere etendue ; interior-sub : premiere boule a interieur non vide.
    const Key* target = nullptr;
    for (const auto& [key, e] : expected) {
      if (by_key.find(key) == by_key.end()) continue;
      const int st = stratum(e);
      bool ok;
      if (inject == "ext-trim") ok = e.sh.size() > e.q && (!target || e.sh.size() > expected.at(*target).sh.size());
      else if (inject == "ext-arity") ok = e.sh.size() > e.q && (!target || (e.q == 3 && expected.at(*target).q != 3));
      else if (inject == "interior-sub") ok = !target && !e.in.empty();
      else ok = !target && (st == want || (want == 3 && st == 31));
      if (ok) target = &key;
    }
    bool done = false;
    if (target) {
      const auto it = by_key.find(*target);
      if (inject == "ext-trim") {
        cat[it->second].n_shell = cat[it->second].arity;
      } else if (inject == "ext-arity") {
        cat[it->second].arity = static_cast<mhgp9::tower::u8>(cat[it->second].arity == 4 ? 3 : cat[it->second].arity + 1);
      } else if (inject == "shell-sub") {
        auto& ball = cat[it->second];
        std::int32_t other = 0;
        const auto in_ball = [&](std::int32_t u) {
          return std::find(ball.interior().begin(), ball.interior().end(), u) != ball.interior().end() ||
                 std::find(ball.shell().begin(), ball.shell().end(), u) != ball.shell().end();
        };
        while (in_ball(other)) ++other;
        ball.shell_ids[0] = other;
      } else if (inject == "interior-sub") {
        auto& ball = cat[it->second];
        std::int32_t other = 0;
        const auto in_ball = [&](std::int32_t u) {
          return std::find(ball.interior().begin(), ball.interior().end(), u) != ball.interior().end() ||
                 std::find(ball.shell().begin(), ball.shell().end(), u) != ball.shell().end();
        };
        while (in_ball(other)) ++other;
        ball.interior_ids[0] = other;
      } else if (inject == "level") {
        cat[it->second].level.den += 1;
      } else {
        by_key.erase(it);
      }
      done = true;
      std::printf("%s INJECT %s key=%s expect=%s\n", label.c_str(), inject.c_str(), key_text(*target).c_str(), want_marker);
    }
    if (!done) { std::printf("%s INJECT_TARGET_ABSENT\n", label.c_str()); return 3; }
  }
  // ---- 5. Comparaison champ par champ.
  std::uint64_t found = 0, missing = 0, level_diff = 0, interior_diff = 0, shell_diff = 0, arity_diff = 0, extended = 0;
  std::array<std::uint64_t, 32> strata{}, strata_ext{};
  for (const auto& [key, e] : expected) {
    const auto it = by_key.find(key);
    const std::string where = "key=" + key_text(key) + " anchor=" + std::to_string(e.support[0]) + " support=" + ids(e.support) +
                              " interior=" + ids(e.in) + " shell=" + ids(e.sh) + " q=" + std::to_string(e.q);
    if (it == by_key.end()) {
      ++missing;
      std::printf("%s MISSING_KEY p=%zu q=%u shell=%zu %s\n", label.c_str(), e.in.size(), e.q, e.sh.size(), where.c_str());
      continue;
    }
    const BallData& ball = cat[it->second];
    bool in_range = ball.n_interior <= mhgp9::tower::kBallInteriorMax && ball.n_shell <= mhgp9::tower::kBallShellMax;
    for (const auto u : ball.interior()) in_range = in_range && u >= 0 && static_cast<std::size_t>(u) < n;
    for (const auto u : ball.shell()) in_range = in_range && u >= 0 && static_cast<std::size_t>(u) < n;
    if (!in_range) { std::printf("%s CATALOGUE_ID_RANGE %s\n", label.c_str(), where.c_str()); return 2; }
    cpp_int num = ball.level.num[2];
    num <<= 64; num += ball.level.num[1];
    num <<= 64; num += ball.level.num[0];
    const cpp_int p2 = e.Pv[0] * e.Pv[0] + e.Pv[1] * e.Pv[1] + e.Pv[2] * e.Pv[2];
    bool ok = true;
    if (num * e.D * e.D != p2 * big(ball.level.den)) {
      ++level_diff; ok = false;
      std::printf("%s LEVEL_DIFF catalogue=%s/%s oracle=%s/%s %s\n", label.c_str(), num.str().c_str(), big(ball.level.den).str().c_str(),
                  p2.str().c_str(), cpp_int(e.D * e.D).str().c_str(), where.c_str());
    }
    std::vector<std::uint32_t> ci, cs;
    for (const auto u : ball.interior()) ci.push_back(id_of_u[static_cast<std::size_t>(u)]);
    for (const auto u : ball.shell()) cs.push_back(id_of_u[static_cast<std::size_t>(u)]);
    std::sort(ci.begin(), ci.end()); std::sort(cs.begin(), cs.end());
    if (ci != e.in) { ++interior_diff; ok = false; std::printf("%s INTERIOR_DIFF p=%zu/%zu catalogue=%s %s\n", label.c_str(), ci.size(), e.in.size(), ids(ci).c_str(), where.c_str()); }
    if (cs != e.sh) { ++shell_diff; ok = false; std::printf("%s SHELL_IDS_DIFF shell=%zu/%zu catalogue=%s %s\n", label.c_str(), cs.size(), e.sh.size(), ids(cs).c_str(), where.c_str()); }
    if (ball.arity != e.q) { ++arity_diff; ok = false; std::printf("%s ARITY_DIFF %u/%u %s\n", label.c_str(), unsigned(ball.arity), e.q, where.c_str()); }
    if (!ok) continue;
    ++found;
    if (e.sh.size() > e.q) { ++extended; ++strata_ext[e.q]; }
    ++strata[static_cast<std::size_t>(stratum(e))];
  }
  const std::uint64_t q3crit = strata[3] + strata[31];
  std::printf("%s n=%zu kmax=%u anchors=%zu supports=%llu degenerate=%llu positive=%llu too_deep=%llu admissible=%llu "
              "expected_unique=%zu found=%llu missing=%llu level_diff=%llu interior_diff=%llu shell_diff=%llu "
              "arity_diff=%llu extended=%llu extended_q2=%llu extended_q3=%llu extended_q4=%llu shell_over_12=%zu "
              "shell_domain=%zu big_forms=%llu q2crit=%llu q3crit=%llu q3crit_long=%llu q4pos=%llu catalogue=%zu\n",
              label.c_str(), n, K, anchors.size(), (unsigned long long)supports, (unsigned long long)degenerate,
              (unsigned long long)positive, (unsigned long long)too_deep, (unsigned long long)admissible, expected.size(),
              (unsigned long long)found, (unsigned long long)missing, (unsigned long long)level_diff,
              (unsigned long long)interior_diff, (unsigned long long)shell_diff, (unsigned long long)arity_diff,
              (unsigned long long)extended, (unsigned long long)strata_ext[2], (unsigned long long)strata_ext[3],
              (unsigned long long)strata_ext[4], over12_keys.size(), domain_keys.size(), (unsigned long long)big_forms,
              (unsigned long long)strata[2], (unsigned long long)q3crit, (unsigned long long)strata[31],
              (unsigned long long)strata[4], cat.size());
  if (want_marker) {
    const std::uint64_t total = missing + level_diff + interior_diff + shell_diff + arity_diff + over12_keys.size();
    const std::string w = want_marker;
    const std::uint64_t hit = w == "MISSING_KEY" ? missing : w == "LEVEL_DIFF" ? level_diff : w == "INTERIOR_DIFF" ? interior_diff
                            : w == "ARITY_DIFF" ? arity_diff : shell_diff;
    if (total == 1 && hit == 1) { std::printf("%s MUTANT_KILLED inject=%s marker=%s\n", label.c_str(), inject.c_str(), want_marker); return 4; }
    std::printf("%s MUTANT_NOT_KILLED inject=%s expected=%s total_diffs=%llu\n", label.c_str(), inject.c_str(), want_marker,
                (unsigned long long)total);
    return 1;
  }
  if (missing || level_diff || interior_diff || shell_diff || arity_diff || !over12_keys.empty()) return 1;
  if (op.expect_extended >= 0 && extended != static_cast<std::uint64_t>(op.expect_extended)) {
    std::printf("%s EXTENDED_COUNT got=%llu expected=%lld\n", label.c_str(), (unsigned long long)extended, op.expect_extended);
    return 3;
  }
  if (!domain_keys.empty()) { std::printf("%s DOMAIN keys=%zu\n", label.c_str(), domain_keys.size()); return 3; }
  std::string vac;
  if (!exp_strata[2]) vac += ",q2crit";
  if (!exp_strata[3] && !exp_strata[31]) vac += ",q3crit";
  if (!exp_strata[31]) vac += ",q3crit_long";
  if (!exp_strata[4]) vac += ",q4pos";
  if (!vac.empty()) { std::printf("%s VACUOUS strata=%s\n", label.c_str(), vac.substr(1).c_str()); return 3; }
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    Opts op;
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
      const std::string a = argv[i];
      if (a.rfind("--seed=", 0) == 0) op.seed = std::stoull(a.substr(7), nullptr, 0);
      else if (a.rfind("--inject=", 0) == 0) op.inject = a.substr(9);
      else if (a.rfind("--expect-catalogue-digest=", 0) == 0) {
        op.expect_digest = a.substr(26);
        if (op.expect_digest.size() != 16) { std::fprintf(stderr, "--expect-catalogue-digest needs 16 hex digits\n"); return 2; }
      } else if (a.rfind("--expect-input-fnv=", 0) == 0) {
        op.expect_fnv = a.substr(19);
        if (op.expect_fnv.size() != 16) { std::fprintf(stderr, "--expect-input-fnv needs 16 hex digits\n"); return 2; }
      } else if (a == "--unpinned") op.unpinned = true;
      else if (a.rfind("--expect-extended=", 0) == 0) {
        op.expect_extended = std::stoll(a.substr(18));
        if (op.expect_extended < 0) { std::fprintf(stderr, "--expect-extended needs N >= 0\n"); return 2; }
      }
      else if (a.rfind("--", 0) == 0) { std::fprintf(stderr, "unknown option %s\n", a.c_str()); return 2; }
      else args.push_back(a);
    }
    if (args.size() == 4 && args[0] == "file") {
      op.digest_required = true;
      const auto slash = args[1].find_last_of('/');
      return run(args[1].substr(slash == std::string::npos ? 0 : slash + 1), read_u32le(args[1]),
                 static_cast<unsigned>(std::stoul(args[2])), std::stoul(args[3]), op);
    }
    if (args.size() == 5 && args[0] == "family") {
      const auto fx = mhgp9::gen::bench::make_front_fixture(std::stoul(args[2]), args[1], 3);
      return run(args[1] + "_" + args[2], fx.points, static_cast<unsigned>(std::stoul(args[3])), std::stoul(args[4]), op);
    }
    if (args.size() == 3 && args[0] == "fixture-cospheric")
      return run("fixture_cospheric", fixture_cospheric(), static_cast<unsigned>(std::stoul(args[1])), std::stoul(args[2]), op);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "error: %s\n", e.what());
    return 2;
  }
  std::fprintf(stderr, "usage: raw_support_judge file <cloud.u32le> <K> <workers> | family <name> <n> <K> <workers> "
                       "| fixture-cospheric <K> <workers> [--seed=S] [--inject=drop-q2|drop-q3|drop-q4|shell-sub|ext-trim|ext-arity|level|"
                       "interior-sub] [--expect-catalogue-digest=HEX|--unpinned] [--expect-input-fnv=HEX] [--expect-extended=N]\n");
  return 2;
}
