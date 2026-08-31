// MorseHGP3D v6 — porte permanente `linked_arcs_u16` : le contrat
// SORTIE-SENSIBLE (barriere d'Edelsbrunner–Pach transposee en entiers u16).
//
// Fixture gravee (morsehgp3D_v5/audits/STRATEGIE_SOUS_QUADRATIQUE_Q3_Q4_20260830) :
// deux arcs enlaces A_i = (x_i, u_i, 30000), B_i = (60000 - x_i, 30000, u_i)
// sur les litteraux x[17], u[17] ci-dessous ; pour n dans {2, 4, 8, 16}
// (pas 8/4/2/1 dans la ligne de 17 indices), N = 2n + 2 points, et les comptes
// de cles distinctes valent EXACTEMENT q3 = 2n(n+1) dans {12, 40, 144, 544} et
// q4 = n² dans {4, 16, 64, 256} — des litteraux du test, jamais une formule lue.
//
// (a) ORACLE LOCAL INDEPENDANT : enumeration exhaustive des triples/quadruples
//     en OBig384 (oracle/obig.hpp, seul import de verdict — aucune primitive de
//     src/lanes ni src/pipeline) : rang, acuite stricte (les trois angles),
//     centre strictement interieur (quadruples), aucune puissance <= 0 hors
//     support, reduction par pgcd, comptage des cles distinctes.
// (b) ROUTE PRODUIT : generate_candidates (s = 8, smax = 11) — chaque cle
//     attendue EXACTEMENT UNE FOIS pre-RLE ; apres sort + deduplicate +
//     prefilter : survie a profondeur zero ; apres census : interieur vide et
//     coquille = support.
// (c) EQUIVARIANCE : permutation physique (PointId conserves) => meme
//     multiensemble pre-RLE ; reetiquetage (id -> N-1-id) => meme ensemble de
//     cles (les cles ne dependent que des positions).
// (d) PLANCHERS : comptes litteraux et identites entieres exactes
//     q3 = 2n(n+1), q4 = n·n, n·(q3+q4) = (3n+2)·n².
//
// Marges gravees a n = 16 : plus petite marge d'acuite 58928 (forme
// |uv|² + |uw|² - |vw|²) ; plus petites puissances brutes positives de
// non-support 9505372644204968192 (q3, > INT64_MAX) et 2588950695868800 (q4).
//
// Options : --n=<2|4|8|16> (defaut : joue les quatre) ; --oracle-i64 (mutant
// d'oracle : les memes calculs avec des accumulateurs 64 bits tronques — la
// puissance q3 exterieure > INT64_MAX a n = 16 doit changer le verdict).
// Codes : 0 conforme ; 1 desaccord ; 2 refus ; 3 plancher viole, invariant ou
// mutant non tue ; 4 mutant tue.
#include <algorithm>
#include <array>
#include <cstdio>
#include <map>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "../oracle/obig.hpp"
#include "../src/pipeline/expand.hpp"
#include "../src/pipeline/generate.hpp"

using namespace mhgp6;

namespace {

using mhgp6_oracle::OBig384;

// ---- Fixture : litteraux de la strategie (lignes 184-186), jamais cos(). ----
constexpr long long kFixX[17] = {59952, 59963, 59973, 59981, 59988, 59993, 59997, 59999, 60000,
                                 59999, 59997, 59993, 59988, 59981, 59973, 59963, 59952};
constexpr long long kFixU[17] = {27601, 27900, 28200, 28500, 28800, 29100, 29400, 29700, 30000,
                                 30300, 30600, 30900, 31200, 31500, 31800, 32100, 32399};

constexpr int kNs[4] = {2, 4, 8, 16};
constexpr u64 kQ3Lit[4] = {12, 40, 144, 544};   // litteraux, PAS une formule
constexpr u64 kQ4Lit[4] = {4, 16, 64, 256};     // litteraux, PAS une formule

struct RawPoint {
  long long c[3];
};

// A_0..A_n puis B_0..B_n (l'ordre litteral EST l'ordre des PointId nominaux).
std::vector<RawPoint> fixture_points(int n) {
  const int step = 16 / n;
  std::vector<RawPoint> pts;
  for (int i = 0; i <= 16; i += step) pts.push_back(RawPoint{{kFixX[i], kFixU[i], 30000}});
  for (int i = 0; i <= 16; i += step) pts.push_back(RawPoint{{60000 - kFixX[i], 30000, kFixU[i]}});
  return pts;
}

// ---- Aides OBig locales de l'oracle (magnitudes ; aucune division ailleurs).
bool ob_even(const OBig384& a) { return (a.w[0] & 1u) == 0; }
void ob_shr1(OBig384* a) {
  for (int i = 0; i + 1 < OBig384::kLimbs; ++i) a->w[i] = (a->w[i] >> 1) | (a->w[i + 1] << 31);
  a->w[OBig384::kLimbs - 1] >>= 1;
}
bool ob_bit(const OBig384& a, int b) { return ((a.w[b / 32] >> (b % 32)) & 1u) != 0; }
void ob_set_bit(OBig384* a, int b) { a->w[b / 32] |= (std::uint32_t)1u << (b % 32); }

// pgcd binaire des magnitudes ; gcd(x, 0) = x.
OBig384 ob_gcd_mag(OBig384 a, OBig384 b) {
  a.neg = false;
  b.neg = false;
  if (a.is_zero()) return b;
  if (b.is_zero()) return a;
  int shift = 0;
  while (ob_even(a) && ob_even(b)) {
    ob_shr1(&a);
    ob_shr1(&b);
    ++shift;
  }
  while (ob_even(a)) ob_shr1(&a);
  for (;;) {
    while (ob_even(b)) ob_shr1(&b);
    if (OBig384::cmp_mag(a, b) > 0) std::swap(a, b);
    b = OBig384::sub_mag(b, a);
    if (b.is_zero()) break;
  }
  while (shift-- > 0) a = OBig384::add_mag(a, a);
  return a;
}

// Division exacte par une magnitude qui divise (division longue binaire) ;
// le signe du dividende est conserve.
OBig384 ob_div_exact(const OBig384& a, const OBig384& g) {
  OBig384 q, r;
  OBig384 gm = g;
  gm.neg = false;
  const OBig384 one = OBig384::from_i64(1);
  for (int b = a.bit_length() - 1; b >= 0; --b) {
    r = OBig384::add_mag(r, r);
    if (ob_bit(a, b)) r = OBig384::add_mag(r, one);
    if (OBig384::cmp_mag(r, gm) >= 0) {
      r = OBig384::sub_mag(r, gm);
      ob_set_bit(&q, b);
    }
  }
  q.neg = a.neg;
  q.canon();
  return q;
}

void key_reduce_ring(std::array<OBig384, 5>* k) {
  const OBig384 one = OBig384::from_i64(1);
  OBig384 g = (*k)[0];
  g.neg = false;
  for (int i = 1; i < 5 && OBig384::cmp_mag(g, one) != 0; ++i) g = ob_gcd_mag(g, (*k)[(size_t)i]);
  if (OBig384::cmp_mag(g, one) <= 0) return;
  for (auto& t : *k) t = ob_div_exact(t, g);
}

// Decimal si la valeur tient en i128, hexadecimal sinon (impression seulement).
std::string ob_str(const OBig384& v) {
  mhgp6_oracle::oi128 x = 0;
  if (!v.to_i128(&x)) return v.hex();
  if (x == 0) return "0";
  const bool neg = x < 0;
  mhgp6_oracle::ou128 m = neg ? (mhgp6_oracle::ou128)(-(x + 1)) + 1 : (mhgp6_oracle::ou128)x;
  std::string s;
  while (m != 0) {
    s.insert(s.begin(), (char)('0' + (int)(m % 10)));
    m /= 10;
  }
  return neg ? "-" + s : s;
}

// ---- Anneau du mutant d'oracle : accumulateurs 64 bits, troncature mod 2^64
// (arithmetique non signee, VOLONTAIREMENT etroite — le contrat i64 tronque).
struct Wrap64 {
  unsigned long long v = 0;
  static Wrap64 from_i64(long long x) { return Wrap64{(unsigned long long)x}; }
  int sign() const {
    const long long s = (long long)v;
    return s == 0 ? 0 : (s < 0 ? -1 : 1);
  }
  friend Wrap64 operator+(const Wrap64& a, const Wrap64& b) { return Wrap64{a.v + b.v}; }
  friend Wrap64 operator-(const Wrap64& a, const Wrap64& b) { return Wrap64{a.v - b.v}; }
  friend Wrap64 operator*(const Wrap64& a, const Wrap64& b) { return Wrap64{a.v * b.v}; }
  Wrap64 operator-() const { return Wrap64{0ull - v}; }
  friend int cmp(const Wrap64& a, const Wrap64& b) {
    const long long x = (long long)a.v, y = (long long)b.v;
    return x < y ? -1 : (x > y ? 1 : 0);
  }
};

void key_reduce_ring(std::array<Wrap64, 5>* k) {
  unsigned long long g = 0;
  for (const Wrap64& t : *k) {
    const unsigned long long m = t.sign() < 0 ? 0ull - t.v : t.v;
    g = std::gcd(g, m);
  }
  if (g <= 1) return;
  for (Wrap64& t : *k) {
    const bool neg = t.sign() < 0;
    const unsigned long long q = (neg ? 0ull - t.v : t.v) / g;
    t.v = neg ? 0ull - q : q;
  }
}

// ---- Oracle local independant (template sur l'anneau ; les deux anneaux
// executent EXACTEMENT le meme chemin de decision). Les formes sont
// reimplementees ici : q3 (Gram, base au premier sommet — le polynome
// (A, B, C) est independant du sommet de base, meme coefficient dominant G et
// meme sphere) ; q4 (Cramer relatif, canonisation det > 0).
template <typename T>
struct OracleOut {
  std::vector<unsigned char> tri, quad;  // decision par support, ordre i<j<k(<l)
  std::vector<std::array<T, 5>> keys3, keys4;  // cles reduites, triees, distinctes
  bool has_acute = false, has_p3 = false, has_p4 = false;
  T min_acute{}, min_pow3{}, min_pow4{};  // marges brutes sur les supports acceptes
};

template <typename T>
void oracle_enumerate(const std::vector<RawPoint>& pts, OracleOut<T>* out) {
  const int N = (int)pts.size();
  std::vector<std::array<T, 3>> P((size_t)N);
  std::vector<T> n2((size_t)N);
  const T zero = T::from_i64(0);
  const T two = T::from_i64(2);
  for (int i = 0; i < N; ++i) {
    for (int c = 0; c < 3; ++c) P[(size_t)i][(size_t)c] = T::from_i64(pts[(size_t)i].c[c]);
    const auto& p = P[(size_t)i];
    n2[(size_t)i] = p[0] * p[0] + p[1] * p[1] + p[2] * p[2];
  }
  const auto vsub = [](const std::array<T, 3>& a, const std::array<T, 3>& b) {
    return std::array<T, 3>{a[0] - b[0], a[1] - b[1], a[2] - b[2]};
  };
  const auto vdot = [](const std::array<T, 3>& a, const std::array<T, 3>& b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  };
  const auto det3 = [](const std::array<T, 3>& r0, const std::array<T, 3>& r1, const std::array<T, 3>& r2) {
    return r2[0] * (r0[1] * r1[2] - r0[2] * r1[1]) - r2[1] * (r0[0] * r1[2] - r0[2] * r1[0]) +
           r2[2] * (r0[0] * r1[1] - r0[1] * r1[0]);
  };
  const auto note_min = [](bool* has, T* cur, const T& v) {
    if (!*has || cmp(v, *cur) < 0) {
      *cur = v;
      *has = true;
    }
  };
  const auto power_of = [&](const std::array<T, 5>& k, int z) {
    const auto& p = P[(size_t)z];
    return k[0] * n2[(size_t)z] + k[1] * p[0] + k[2] * p[1] + k[3] * p[2] + k[4];
  };

  // ---- Triples : rang, acuite stricte, sphere vide (ni interieur ni coquille
  // hors support), reduction pgcd.
  for (int i = 0; i < N; ++i)
    for (int j = i + 1; j < N; ++j)
      for (int k = j + 1; k < N; ++k) {
        unsigned char acc = 0;
        [&]() {
          const auto d = vsub(P[(size_t)j], P[(size_t)i]);
          const auto u = vsub(P[(size_t)k], P[(size_t)i]);
          const std::array<T, 3> cr{d[1] * u[2] - d[2] * u[1], d[2] * u[0] - d[0] * u[2],
                                    d[0] * u[1] - d[1] * u[0]};
          if (cr[0].sign() == 0 && cr[1].sign() == 0 && cr[2].sign() == 0) return;  // colineaires
          const auto e = vsub(P[(size_t)k], P[(size_t)j]);
          const T lij = vdot(d, d), lik = vdot(u, u), ljk = vdot(e, e);
          // Acuite stricte aux trois sommets : forme |uv|² + |uw|² - |vw|².
          const T mi = lij + lik - ljk, mj = lij + ljk - lik, mk = lik + ljk - lij;
          if (mi.sign() <= 0 || mj.sign() <= 0 || mk.sign() <= 0) return;
          // Forme de la circumboule (Gram) : G = DE - F², W = E(D-F)d + D(E-F)u,
          // (A, B, C) = (G, -(2Ga + W), G|a|² + W·a).
          const T D = lij, E = lik, F = vdot(d, u);
          const T G = D * E - F * F;
          const T c1 = E * (D - F), c2 = D * (E - F);
          std::array<T, 5> key;
          key[0] = G;
          T wa = zero;
          for (int t = 0; t < 3; ++t) {
            const T Wt = c1 * d[(size_t)t] + c2 * u[(size_t)t];
            key[(size_t)(1 + t)] = -(two * G * P[(size_t)i][(size_t)t] + Wt);
            wa = wa + Wt * P[(size_t)i][(size_t)t];
          }
          key[4] = G * n2[(size_t)i] + wa;
          T ball_min{};
          bool has_bm = false;
          for (int z = 0; z < N; ++z) {
            if (z == i || z == j || z == k) continue;
            const T pz = power_of(key, z);
            if (pz.sign() <= 0) return;  // puissance < 0 interdite, = 0 interdite hors support
            note_min(&has_bm, &ball_min, pz);
          }
          acc = 1;
          note_min(&out->has_acute, &out->min_acute, mi);
          note_min(&out->has_acute, &out->min_acute, mj);
          note_min(&out->has_acute, &out->min_acute, mk);
          if (has_bm) note_min(&out->has_p3, &out->min_pow3, ball_min);
          key_reduce_ring(&key);
          out->keys3.push_back(key);
        }();
        out->tri.push_back(acc);
      }

  // ---- Quadruples : non coplanaire, centre strictement interieur
  // (determinants de cotes), sphere vide, reduction pgcd.
  for (int i = 0; i < N; ++i)
    for (int j = i + 1; j < N; ++j)
      for (int k = j + 1; k < N; ++k)
        for (int l = k + 1; l < N; ++l) {
          unsigned char acc = 0;
          [&]() {
            const auto e1 = vsub(P[(size_t)j], P[(size_t)i]);
            const auto e2 = vsub(P[(size_t)k], P[(size_t)i]);
            const auto e3 = vsub(P[(size_t)l], P[(size_t)i]);
            const std::array<T, 3> m0{two * e1[0], two * e1[1], two * e1[2]};
            const std::array<T, 3> m1{two * e2[0], two * e2[1], two * e2[2]};
            const std::array<T, 3> m2{two * e3[0], two * e3[1], two * e3[2]};
            const T r0 = vdot(e1, e1), r1 = vdot(e2, e2), r2 = vdot(e3, e3);
            const auto cof = [](const std::array<T, 3>& x, const std::array<T, 3>& y, int j0, int j1) {
              return x[(size_t)j0] * y[(size_t)j1] - x[(size_t)j1] * y[(size_t)j0];
            };
            const T c00 = cof(m1, m2, 1, 2), c01 = -cof(m1, m2, 0, 2), c02 = cof(m1, m2, 0, 1);
            const T c10 = -cof(m0, m2, 1, 2), c11 = cof(m0, m2, 0, 2), c12 = -cof(m0, m2, 0, 1);
            const T c20 = cof(m0, m1, 1, 2), c21 = -cof(m0, m1, 0, 2), c22 = cof(m0, m1, 0, 1);
            T det = m0[0] * c00 + m0[1] * c01 + m0[2] * c02;
            std::array<T, 3> np{c00 * r0 + c10 * r1 + c20 * r2, c01 * r0 + c11 * r1 + c21 * r2,
                                c02 * r0 + c12 * r1 + c22 * r2};
            if (det.sign() == 0) return;  // coplanaires
            if (det.sign() < 0) {
              det = -det;
              for (int t = 0; t < 3; ++t) np[(size_t)t] = -np[(size_t)t];
            }
            // Centre strictement interieur : pour chaque face, le centre et le
            // sommet oppose strictement du meme cote ; un zero est un refus.
            const T vol = det3(e1, e2, e3);
            const std::array<T, 3>* v4[4] = {&P[(size_t)i], &P[(size_t)j], &P[(size_t)k], &P[(size_t)l]};
            for (int s = 0; s < 4; ++s) {
              const std::array<T, 3>* fp[3];
              int t = 0;
              for (int w = 0; w < 4; ++w)
                if (w != s) fp[t++] = v4[w];
              const auto f1 = vsub(*fp[1], *fp[0]);
              const auto f2 = vsub(*fp[2], *fp[0]);
              const auto dp = vsub(*fp[0], P[(size_t)i]);
              const std::array<T, 3> rc{np[0] - det * dp[0], np[1] - det * dp[1], np[2] - det * dp[2]};
              const T side = det3(f1, f2, rc);
              if (side.sign() == 0) return;
              // Convention de signe des faces (rangs pairs/impairs) : sur la
              // face s paire, le sommet oppose est du cote vol < 0.
              const bool even_face = (s % 2) == 0;
              const bool want_pos = even_face ? (vol.sign() < 0) : (vol.sign() > 0);
              if ((side.sign() > 0) != want_pos) return;
            }
            // Forme (A, B, C) = (det, -2(det a + N'), det|a|² + 2 N'·a).
            std::array<T, 5> key;
            key[0] = det;
            T na = zero;
            for (int t = 0; t < 3; ++t) {
              key[(size_t)(1 + t)] = -(two * (det * P[(size_t)i][(size_t)t] + np[(size_t)t]));
              na = na + np[(size_t)t] * P[(size_t)i][(size_t)t];
            }
            key[4] = det * n2[(size_t)i] + two * na;
            T ball_min{};
            bool has_bm = false;
            for (int z = 0; z < N; ++z) {
              if (z == i || z == j || z == k || z == l) continue;
              const T pz = power_of(key, z);
              if (pz.sign() <= 0) return;
              note_min(&has_bm, &ball_min, pz);
            }
            acc = 1;
            if (has_bm) note_min(&out->has_p4, &out->min_pow4, ball_min);
            key_reduce_ring(&key);
            out->keys4.push_back(key);
          }();
          out->quad.push_back(acc);
        }

  // Cles DISTINCTES (tri lexicographique par cmp, dedoublonnage).
  const auto lex_less = [](const std::array<T, 5>& x, const std::array<T, 5>& y) {
    for (int t = 0; t < 5; ++t) {
      const int c = cmp(x[(size_t)t], y[(size_t)t]);
      if (c != 0) return c < 0;
    }
    return false;
  };
  const auto lex_eq = [](const std::array<T, 5>& x, const std::array<T, 5>& y) {
    for (int t = 0; t < 5; ++t)
      if (cmp(x[(size_t)t], y[(size_t)t]) != 0) return false;
    return true;
  };
  std::sort(out->keys3.begin(), out->keys3.end(), lex_less);
  out->keys3.erase(std::unique(out->keys3.begin(), out->keys3.end(), lex_eq), out->keys3.end());
  std::sort(out->keys4.begin(), out->keys4.end(), lex_less);
  out->keys4.erase(std::unique(out->keys4.begin(), out->keys4.end(), lex_eq), out->keys4.end());
}

// ---- Route produit et equivariance. --------------------------------------
struct ExpectedKey {
  BallKey key;
  u8 arity;
};

std::vector<InputPoint> fixture_input(const std::vector<RawPoint>& pts) {
  std::vector<InputPoint> in(pts.size());
  for (size_t i = 0; i < pts.size(); ++i) {
    in[i].id = (PointId)i;  // PointId = ordre litteral A_0..A_n, B_0..B_n
    in[i].position = P3{pts[i].c[0], pts[i].c[1], pts[i].c[2]};
  }
  return in;
}

bool run_generate(const std::vector<InputPoint>& in, std::vector<BallCandidate>* cands, GenerateStats* gs) {
  const CloudIndex ix = build_cloud_index(in);
  if (!ix.valid || ix.has_duplicate_positions()) return false;
  GenerateOptions go;
  go.s = 8;
  go.smax = 11;
  go.threads = 2;
  generate_candidates(ix, go, cands, gs);
  return true;
}

// splitmix64 : permutation physique deterministe, independante de la
// bibliotheque standard (jamais std::shuffle dans une porte permanente).
u64 splitmix64(u64* s) {
  *s += 0x9e3779b97f4a7c15ull;
  u64 z = *s;
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
  z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
  return z ^ (z >> 31);
}

bool candidate_eq(const BallCandidate& x, const BallCandidate& y) {
  return x.key == y.key && x.arity == y.arity && x.level == y.level;
}

std::vector<BallKey> key_set_of(const std::vector<BallCandidate>& cands) {
  std::vector<BallKey> keys;
  keys.reserve(cands.size());
  for (const BallCandidate& c : cands) keys.push_back(c.key);
  std::sort(keys.begin(), keys.end());
  keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
  return keys;
}

u64 check_product(int n, const std::vector<RawPoint>& pts, const std::vector<ExpectedKey>& expected) {
  u64 dis = 0;
  const std::vector<InputPoint> in = fixture_input(pts);
  std::vector<BallCandidate> cands;
  GenerateStats gs;
  if (!run_generate(in, &cands, &gs)) {
    ++dis;
    std::fprintf(stderr, "n=%d : entree produit invalide (index)\n", n);
    return dis;
  }

  // (b1) Multiensemble PRE-RLE : chaque cle attendue EXACTEMENT UNE FOIS.
  std::map<BallKey, std::pair<u64, u8>> occ;  // occurrences, arite du dernier
  for (const BallCandidate& c : cands) {
    auto& e = occ[c.key];
    ++e.first;
    e.second = c.arity;
  }
  u64 found3 = 0, found4 = 0;
  for (const ExpectedKey& ek : expected) {
    const auto it = occ.find(ek.key);
    if (it == occ.end()) {
      ++dis;
      std::fprintf(stderr, "n=%d : cle attendue (arite %d) ABSENTE du pre-RLE\n", n, (int)ek.arity);
      continue;
    }
    if (it->second.first != 1 || it->second.second != ek.arity) {
      ++dis;
      std::fprintf(stderr, "n=%d : cle attendue (arite %d) : %llu occurrence(s) pre-RLE, arite vue %d\n", n,
                   (int)ek.arity, (unsigned long long)it->second.first, (int)it->second.second);
      continue;
    }
    if (ek.arity == 3)
      ++found3;
    else
      ++found4;
  }

  // (b2) sort + deduplicate + prefilter : survie a profondeur ZERO.
  const CloudIndex ix = build_cloud_index(in);
  std::vector<BallCandidate> rle = cands;
  sort_candidates(&rle, 1);
  deduplicate_candidates(&rle);
  std::vector<Survivor> surv;
  ExpandStats es;
  prefilter_balls(ix, rle, 11, 1, &surv, &es);
  std::map<BallKey, std::pair<u64, u8>> smap;  // profondeur, arite
  for (const Survivor& s : surv) smap[rle[s.idx].key] = {s.depth, rle[s.idx].arity};
  for (const ExpectedKey& ek : expected) {
    const auto it = smap.find(ek.key);
    if (it == smap.end()) {
      ++dis;
      std::fprintf(stderr, "n=%d : cle attendue (arite %d) morte au prefiltre\n", n, (int)ek.arity);
    } else if (it->second.first != 0 || it->second.second != ek.arity) {
      ++dis;
      std::fprintf(stderr, "n=%d : cle attendue (arite %d) : profondeur %llu != 0 ou arite %d\n", n, (int)ek.arity,
                   (unsigned long long)it->second.first, (int)it->second.second);
    }
  }

  // (b3) census : interieur vide, coquille = support.
  std::vector<BallData> balls;
  const PipelineStatus ps = census_balls(ix, rle, surv, 11, kBallShellMax, 1, &balls, &es);
  if (ps != PipelineStatus::kCompleteRegular) {
    ++dis;
    std::fprintf(stderr, "n=%d : census_balls hors complete_regular\n", n);
  } else {
    std::map<BallKey, const BallData*> bmap;
    for (const BallData& b : balls) bmap[b.key] = &b;
    for (const ExpectedKey& ek : expected) {
      const auto it = bmap.find(ek.key);
      if (it == bmap.end()) {
        ++dis;
        std::fprintf(stderr, "n=%d : cle attendue (arite %d) absente du census\n", n, (int)ek.arity);
      } else if (it->second->n_interior != 0 || it->second->n_shell != ek.arity) {
        ++dis;
        std::fprintf(stderr, "n=%d : cle attendue (arite %d) : interieur %d != 0 ou coquille %d != arite\n", n,
                     (int)ek.arity, (int)it->second->n_interior, (int)it->second->n_shell);
      }
    }
  }

  // (c1) Permutation physique deterministe, PointId conserves : MEME
  // multiensemble pre-RLE (cle, arite, niveau).
  std::vector<InputPoint> perm = in;
  u64 seed = 0x51ba9e1cull ^ (u64)n;
  for (size_t i = perm.size() - 1; i > 0; --i) std::swap(perm[i], perm[splitmix64(&seed) % (i + 1)]);
  std::vector<BallCandidate> cperm;
  GenerateStats gsp;
  if (!run_generate(perm, &cperm, &gsp)) {
    ++dis;
    std::fprintf(stderr, "n=%d : entree permutee invalide\n", n);
  } else {
    std::vector<BallCandidate> a = cands, b = cperm;
    sort_candidates(&a, 1);
    sort_candidates(&b, 1);
    if (a.size() != b.size()) {
      ++dis;
      std::fprintf(stderr, "n=%d : equivariance (permutation) : %zu vs %zu candidats\n", n, a.size(), b.size());
    } else {
      for (size_t i = 0; i < a.size(); ++i)
        if (!candidate_eq(a[i], b[i])) {
          ++dis;
          std::fprintf(stderr, "n=%d : equivariance (permutation) : candidat %zu divergent\n", n, i);
          break;
        }
    }
  }

  // (c2) Reetiquetage id -> N-1-id : MEME ensemble de cles (les cles ne
  // dependent que des positions ; le tie-break d'owner peut changer).
  std::vector<InputPoint> rel = in;
  for (InputPoint& p : rel) p.id = (PointId)(rel.size() - 1) - p.id;
  std::vector<BallCandidate> crel;
  GenerateStats gsr;
  if (!run_generate(rel, &crel, &gsr)) {
    ++dis;
    std::fprintf(stderr, "n=%d : entree reetiquetee invalide\n", n);
  } else {
    const std::vector<BallKey> k0 = key_set_of(cands), k1 = key_set_of(crel);
    if (k0 != k1) {
      ++dis;
      std::fprintf(stderr, "n=%d : equivariance (reetiquetage) : %zu vs %zu cles distinctes\n", n, k0.size(),
                   k1.size());
    }
  }

  std::printf("n=%d produit : pre-RLE q2=%llu q3=%llu q4=%llu ; attendues trouvees exact-once q3=%llu q4=%llu\n", n,
              (unsigned long long)gs.candidates[0], (unsigned long long)gs.candidates[1],
              (unsigned long long)gs.candidates[2], (unsigned long long)found3, (unsigned long long)found4);
  return dis;
}

// ---- Cadre d'une taille n : oracle, marges, produit, planchers. -----------
void run_for_n(int n, u64* dis, u64* floors) {
  const int idx = n == 2 ? 0 : n == 4 ? 1 : n == 8 ? 2 : 3;
  const u64 q3l = kQ3Lit[idx], q4l = kQ4Lit[idx], un = (u64)n;

  // (d) Planchers : les litteraux satisfont les identites entieres EXACTES
  // q3 = 2n(n+1), q4 = n·n, n·(q3+q4) = (3n+2)·n² (ratio 3 + 2/n en entiers).
  if (q3l != 2 * un * (un + 1) || q4l != un * un || un * (q3l + q4l) != (3 * un + 2) * un * un) {
    ++*floors;
    std::fprintf(stderr, "n=%d : identites entieres des litteraux violees\n", n);
    return;
  }

  const std::vector<RawPoint> pts = fixture_points(n);
  if (pts.size() != (size_t)(2 * n + 2)) {
    ++*floors;
    std::fprintf(stderr, "n=%d : cardinal N != 2n+2\n", n);
    return;
  }

  // (a) Oracle independant en OBig384 (drapeau collant efface avant campagne).
  mhgp6_oracle::clear_overflow();
  OracleOut<OBig384> ob;
  oracle_enumerate(pts, &ob);
  if (mhgp6_oracle::overflow_seen()) {
    ++*floors;
    std::fprintf(stderr, "n=%d : debordement OBig384 dans l'oracle — echec ferme\n", n);
    return;
  }
  std::printf("n=%d oracle  : q3=%zu q4=%zu (attendus %llu / %llu)\n", n, ob.keys3.size(), ob.keys4.size(),
              (unsigned long long)q3l, (unsigned long long)q4l);
  if (ob.keys3.size() != (size_t)q3l || ob.keys4.size() != (size_t)q4l) {
    ++*dis;
    std::fprintf(stderr, "n=%d : desaccord oracle/litteraux\n", n);
  }

  // Marges gravees a n = 16 (au moins i128/OBig, jamais une conversion etroite).
  if (n == 16) {
    std::printf("n=16 marges : acuite=%s pow3=%s pow4=%s\n", ob_str(ob.min_acute).c_str(),
                ob_str(ob.min_pow3).c_str(), ob_str(ob.min_pow4).c_str());
    const OBig384 want_acute = OBig384::from_i64(58928);
    const OBig384 want_p3 = OBig384::from_i128((mhgp6_oracle::oi128)9505372644204968192ull);
    const OBig384 want_p4 = OBig384::from_i64(2588950695868800ll);
    const OBig384 i64max = OBig384::from_i64(9223372036854775807ll);
    if (!ob.has_acute || ob.min_acute != want_acute) {
      ++*dis;
      std::fprintf(stderr, "n=16 : marge d'acuite != 58928\n");
    }
    if (!ob.has_p3 || ob.min_pow3 != want_p3) {
      ++*dis;
      std::fprintf(stderr, "n=16 : puissance q3 exterieure minimale != 9505372644204968192\n");
    }
    if (!ob.has_p4 || ob.min_pow4 != want_p4) {
      ++*dis;
      std::fprintf(stderr, "n=16 : puissance q4 exterieure minimale != 2588950695868800\n");
    }
    if (!ob.has_p3 || !(ob.min_pow3 > i64max)) {
      ++*floors;
      std::fprintf(stderr, "n=16 : plancher viole — la puissance q3 exterieure ne depasse pas INT64_MAX\n");
    }
  }

  // Conversion de representation OBig -> BallKey (jamais une decision) pour
  // confronter la route produit.
  std::vector<ExpectedKey> expected;
  expected.reserve(ob.keys3.size() + ob.keys4.size());
  bool conv_ok = true;
  const auto push_expected = [&](const std::array<OBig384, 5>& k, u8 arity) {
    mhgp6_oracle::oi128 v[5];
    for (int t = 0; t < 5; ++t)
      if (!k[(size_t)t].to_i128(&v[t])) {
        conv_ok = false;
        return;
      }
    BallKey bk;
    bk.a = v[0];
    bk.b[0] = v[1];
    bk.b[1] = v[2];
    bk.b[2] = v[3];
    bk.c = v[4];
    expected.push_back(ExpectedKey{bk, arity});
  };
  for (const auto& k : ob.keys3) push_expected(k, 3);
  for (const auto& k : ob.keys4) push_expected(k, 4);
  if (!conv_ok || expected.size() != ob.keys3.size() + ob.keys4.size()) {
    ++*floors;
    std::fprintf(stderr, "n=%d : cle d'oracle hors i128 (profil u16 viole)\n", n);
    return;
  }

  // (b) + (c) Route produit et equivariance.
  *dis += check_product(n, pts, expected);
}

// ---- Mutant d'oracle --oracle-i64 : verdict DIFFERENT exige a n = 16. -----
int run_oracle_i64_mutant() {
  const std::vector<RawPoint> pts = fixture_points(16);
  mhgp6_oracle::clear_overflow();
  OracleOut<OBig384> ob;
  oracle_enumerate(pts, &ob);
  if (mhgp6_oracle::overflow_seen() || ob.keys3.size() != 544 || ob.keys4.size() != 256) {
    std::fprintf(stderr, "oracle-i64 : reference OBig invalide (q3=%zu q4=%zu) — kill non certifiable\n",
                 ob.keys3.size(), ob.keys4.size());
    return 3;
  }
  OracleOut<Wrap64> wr;
  oracle_enumerate(pts, &wr);
  std::printf("oracle-i64 n=16 : OBig q3=%zu q4=%zu ; i64 tronque q3=%zu q4=%zu\n", ob.keys3.size(), ob.keys4.size(),
              wr.keys3.size(), wr.keys4.size());
  bool differ = ob.keys3.size() != wr.keys3.size() || ob.keys4.size() != wr.keys4.size();
  for (size_t i = 0; i < ob.tri.size() && !differ; ++i) differ = ob.tri[i] != wr.tri[i];
  for (size_t i = 0; i < ob.quad.size() && !differ; ++i) differ = ob.quad[i] != wr.quad[i];
  if (differ) {
    std::printf("oracle-i64 : verdict different — mutant TUE\n");
    return 4;
  }
  std::fprintf(stderr, "oracle-i64 : verdict identique — mutant NON tue\n");
  return 3;
}

}  // namespace

int main(int argc, char** argv) {
  std::vector<int> ns;
  bool oracle_i64 = false;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--oracle-i64") {
      oracle_i64 = true;
    } else if (arg.rfind("--n=", 0) == 0) {
      const std::string v = arg.substr(4);
      if (v == "2")
        ns.push_back(2);
      else if (v == "4")
        ns.push_back(4);
      else if (v == "8")
        ns.push_back(8);
      else if (v == "16")
        ns.push_back(16);
      else {
        std::fprintf(stderr, "refus : --n=%s (2, 4, 8 ou 16)\n", v.c_str());
        return 2;
      }
    } else {
      std::fprintf(stderr, "argument inconnu : %s\n", arg.c_str());
      return 2;
    }
  }
  if (oracle_i64) {
    if (!ns.empty() && !(ns.size() == 1 && ns[0] == 16)) {
      std::fprintf(stderr, "refus : --oracle-i64 se joue a n=16 (la puissance > INT64_MAX y vit)\n");
      return 2;
    }
    return run_oracle_i64_mutant();
  }
  if (ns.empty()) ns.assign(kNs, kNs + 4);
  u64 dis = 0, floors = 0;
  for (const int n : ns) run_for_n(n, &dis, &floors);
  if (floors != 0) return 3;
  if (dis != 0) return 1;
  std::printf("linked_arcs_u16 : conforme\n");
  return 0;
}
