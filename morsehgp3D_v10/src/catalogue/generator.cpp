// Generateur du catalogue par boites de centres. Voir catalogue.hpp pour l'objet et les references.
//
// Repere de l'arbre : sites mis a l'echelle X = x << kT (kT = 6 bits sous-unitaires), boites entieres
// demi-ouvertes [lo, hi). En u18 : |X| < 2^24, tests de gardes, dominance et bissectrices en i64 ;
// test droite des centres (lemme Z, zonogone) en i128 ; centres et recensement en i128 (geometry.hpp).
#include <algorithm>
#include <bit>
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
  // feuille : copies locales (coordonnees, repere T, poids), masques de dominance, paires et triplets vivants
  std::vector<P3> lp, lx;
  std::vector<i64> lx2;
  std::vector<u32> wl;
  std::vector<u64> dom, live2, live3;
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
  a.leaf_dominance_tests += b.leaf_dominance_tests;
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

// Milieu de deux sites (repere T) dans la boite demi-ouverte : forme q2 de center_in_box (D = 2,
// 2^T (a D + N) = X_a + X_b), exacte en i64.
inline bool midpoint_in_box(const P3& A, const P3& B, const Box& Q) {
  return 2 * Q.lo[0] <= A.x + B.x && A.x + B.x < 2 * Q.hi[0] && 2 * Q.lo[1] <= A.y + B.y && A.y + B.y < 2 * Q.hi[1] &&
         2 * Q.lo[2] <= A.z + B.z && A.z + B.z < 2 * Q.hi[2];
}

// Lemme Z : la droite des centres equidistants de trois sites A, B, D (repere T) rencontre la boite fermee.
// Avec u = A - B, v = A - D et F(C) = (f_AB(C), f_AD(C)), 2 F(Qbar) est le zonogone P + sum_k [-1, 1] g_k, ou
// P = 2 F(centre de boite) = (2 (|A|^2 - |B|^2) - 2 (lo + hi) . u, idem avec D et v), g_k = -2 h_k (u_k, v_k)
// et h_k = hi_k - lo_k > 0. Il contient 0 si et seulement si |n_k . P| <= sum_j |n_k . g_j| pour chaque g_k non
// nul, n_k = g_k^perp = 2 h_k (v_k, -u_k) (les cotes d'un zonogone sont les g_k). Or n_k . g_j = 4 h_k h_j c_kj
// avec c_kj = u_k v_j - v_k u_j (composantes de u x v au signe pres) ; la division par 2 h_k > 0 donne la forme
// entiere exacte |v_k P_0 - u_k P_1| <= 2 sum_{j != k} h_j |c_kj|, meme decision que la forme developpee.
// Bornes (u18, T = 6) : |X| < 2^24, |u|, |v| < 2^24, lo + hi < 2^26, h <= 2^24 : |P| < 2^54 et |c| < 2^49 en
// i64 ; |v_k P_0 - u_k P_1| < 2^79 et 2 sum h_j |c_kj| < 2^76 en i128.
// Rend -1 si A, B, D sont alignes (u x v = 0), 0 si la droite manque la boite, 1 si elle la rencontre.
inline int center_line_meets(const P3& A, i64 a2, const P3& B, i64 b2, const P3& Dp, i64 d2, const Box& Q) {
  const i64 u[3] = {A.x - B.x, A.y - B.y, A.z - B.z};
  const i64 v[3] = {A.x - Dp.x, A.y - Dp.y, A.z - Dp.z};
  const i64 c01 = u[0] * v[1] - v[0] * u[1], c02 = u[0] * v[2] - v[0] * u[2], c12 = u[1] * v[2] - v[1] * u[2];
  if (c01 == 0 && c02 == 0 && c12 == 0) return -1;
  i64 p0 = 2 * (a2 - b2), p1 = 2 * (a2 - d2), h[3];
  for (int k = 0; k < 3; ++k) {
    const i64 s = Q.lo[k] + Q.hi[k];
    p0 -= 2 * s * u[k];
    p1 -= 2 * s * v[k];
    h[k] = Q.hi[k] - Q.lo[k];
  }
  const i64 a01 = c01 < 0 ? -c01 : c01, a02 = c02 < 0 ? -c02 : c02, a12 = c12 < 0 ? -c12 : c12;
  const i128 r[3] = {2 * (i128(h[1]) * a01 + i128(h[2]) * a02), 2 * (i128(h[0]) * a01 + i128(h[2]) * a12),
                     2 * (i128(h[0]) * a02 + i128(h[1]) * a12)};
  for (int k = 0; k < 3; ++k) {
    if (u[k] == 0 && v[k] == 0) continue;
    i128 l = i128(v[k]) * p0 - i128(u[k]) * p1;
    if (l < 0) l = -l;
    if (l > r[k]) return 0;
  }
  return 1;
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

// Niveau exact d'une boule emise, dans la representation (num, den) de la presentation qui l'emettait en
// premier dans l'ordre d'enumeration de la feuille v1 (toutes les paires, puis chaque triplet (i, j, k) suivi
// de ses quadruplets (i, j, k, l)). Le niveau exact ne depend pas de la presentation ; sa representation,
// que la tour publie (cat.level), en depend, et la feuille v2 l'enumere dans un autre ordre : on la fixe ici.
//   coquille reguliere : S* est la seule presentation ;
//   coquille etendue, q_min = 2 : S* (paire antipodale lexicographiquement minimale ; paires d'abord) ;
//   q_min = 4 : S* (seules presentations : tetraedres a centre strictement interieur, S* le premier) ;
//   q_min = 3 : le premier tetraedre de coquille a centre strictement interieur dont le prefixe (i, j, k)
//   precede S* (un tel prefixe n'est jamais S*, dont le plan contient le centre), sinon S*.
geom::Level emitted_level(const Ctx& C, const std::vector<u32>& sh, const std::array<u32, 4>& sup, u8 q,
                          const std::array<u32, 4>& gen, const geom::Center& ctr) {
  if (q == 2) return geom::level2(C.P[sup[0]], C.P[sup[1]]);
  if (q == 4) {
    if (gen == sup) return geom::level4(ctr);
    geom::Center c4;
    geom::center4(C.P[sup[0]], C.P[sup[1]], C.P[sup[2]], C.P[sup[3]], c4);
    return geom::level4(c4);
  }
  const u32 m = static_cast<u32>(sh.size());
  const std::array<u32, 3> s3 = {sup[0], sup[1], sup[2]};
  for (u32 i = 0; i < m; ++i)
    for (u32 j = i + 1; j < m; ++j)
      for (u32 k = j + 1; k < m; ++k) {
        // coquille triee : les triplets defilent dans l'ordre lexicographique ; S* est atteint avant la fin
        if (!(std::array<u32, 3>{sh[i], sh[j], sh[k]} < s3)) return geom::level3(C.P[s3[0]], C.P[s3[1]], C.P[s3[2]]);
        for (u32 l = k + 1; l < m; ++l) {
          const P3 &a = C.P[sh[i]], &b = C.P[sh[j]], &d = C.P[sh[k]], &e = C.P[sh[l]];
          geom::Center c4;
          if (!geom::center4(a, b, d, e, c4)) continue;
          const P3* t[4] = {&a, &b, &d, &e};
          if (geom::strictly_inside_tetra(t, a, c4)) return geom::level4(c4);
        }
      }
  return geom::level3(C.P[sup[0]], C.P[sup[1]], C.P[sup[2]]);
}

// Juge une sphere candidate : recensement exact sur la liste de la feuille (copie locale L.lp, L.wl),
// admission, emission. theta : seuil d'admission de la presentation (feuille v2). Le recensement s'arrete des
// que le poids interieur le depasse : S* d'une boule admise ne s'arrete jamais (lemme S), une presentation
// non canonique peut s'arreter puisque S* est enumere dans la meme feuille. theta = K - 1 : sortie p >= K.
void judge(const Ctx& C, Local& L, const std::vector<u32>& cand, u32 anchor_local, const geom::Center& ctr,
           const std::array<u32, 4>& gen, u8 qgen, i64 theta) {
  ++L.led.judged;
  const P3 a = L.lp[anchor_local];
  const u32 m = static_cast<u32>(cand.size());
  u32 p = 0;
  L.shell.clear();
  L.interior.clear();
  for (u32 t = 0; t < m; ++t) {
    const int s = geom::side(ctr, a, L.lp[t]);
    if (s < 0) {
      p += L.wl[t];
      if (static_cast<i64>(p) > theta) return;  // toute admission de cette presentation exige p <= theta
      L.interior.push_back(cand[t]);
    } else if (s == 0) {
      L.shell.push_back(cand[t]);
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
    for (const auto& mm : L.memo)
      if (mm == sup) return;
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
  r.level = emitted_level(C, L.shell, sup, q, gen, ctr);
  L.recs.push_back(r);
  ++L.led.emitted;
  if (flags & kExtendedShell) ++L.led.extended;
  if (flags & kWeightedShell) ++L.led.weighted;
  L.led.max_shell = std::max<u64>(L.led.max_shell, L.shell.size());
}

// Bits d'indice global > b dans le mot t (t >= b / 64).
inline u64 bits_above(u32 b, u32 t) { return t > (b >> 6) ? ~u64(0) : ~((u64(2) << (b & 63)) - 1); }

// Population d'un mot (forme SWAR : le profil sans popcnt materiel appelait __popcountdi2).
inline u64 popcount64(u64 x) {
  x -= (x >> 1) & 0x5555555555555555ull;
  x = (x & 0x3333333333333333ull) + ((x >> 2) & 0x3333333333333333ull);
  x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0Full;
  return (x * 0x0101010101010101ull) >> 56;
}

// Feuille « v2 » (GEN_v1 § 2.6 et § 5.3) : on n'enumere que les presentations dont chaque partie peut
// appartenir au support canonique S* d'une boule admise de centre dans la feuille.
//
//   Dom[i] = { j : max_{C dans Qbar} (|X_j - C|^2 - |X_i - C|^2) < 0 }   (forme du lemme D, i64, repere T) ;
//   theta_q = K + 1 - q si toutes les positions de la liste pesent 1 (feuille serree), K - 1 sinon ;
//   w(T) = poids de l'union des Dom[s], s dans T.
// Une seule evaluation affine par paire non ordonnee (i, j) : g(C) = |X_j - C|^2 - |X_i - C|^2 a son maximum
// et son minimum sur Qbar aux coins choisis par le signe de X_j - X_i ; max g < 0 met j dans Dom[i], min g > 0
// met i dans Dom[j], et sinon min g <= 0 <= max g : c'est exactement le test de la bissectrice (qui coupe Qbar
// si et seulement si ni i ni j ne domine l'autre).
// Lemme M : si T est inclus dans la coquille d'une boule B de centre c dans Q (donc 2^T c dans Qbar), tout j
// de Dom[s], s dans T, est strictement plus proche de c que s, donc interieur a B : w(T) <= p(B). Si B est
// admise, sa coquille est dans la liste (theoreme C) et p(B) <= theta_{q_min} : coquille non ponderee,
// p + q_min <= K + 1 ; ponderee (feuille non serree), p <= K - 1. theta_q decroit avec q.
// Lemme S : S* d'une boule admise passe donc chacun des filtres suivants, et ses parties aussi :
//   paire (i, j)      : bissectrice qui coupe Qbar, w <= theta_2 -> juge q2 si le milieu est dans Q ;
//                       vivante (P2) si de plus w <= theta_3 (toute paire d'un S* de cardinal 3 ou 4) ;
//   triplet (i, j, k) : trois paires vivantes, w <= theta_3, non alignes, droite des centres qui coupe Qbar
//                       (lemme Z) -> juge q3 si aigu et centre dans Q ; vivant (H) si w <= theta_4 ;
//   quadruplet        : quatre triplets vivants, w <= theta_4 -> juge q4 si centre dans Q, strictement
//                       interieur au tetraedre ;
//   recensement       : arret seulement si p > theta de la presentation (judge).
// Une presentation non canonique peut etre ecartee sans perte : S* est enumere dans la meme feuille, et le
// memo des coquilles etendues rend l'emission unique quel que soit l'ordre d'enumeration. Aucun flottant.
// Masques sur NW mots de 64 bits ; NW = 0 : nombre de mots lu a l'execution (feuilles de plus de 256 sites).
template <int NW>
void enumerate_leaf_masks(const Ctx& C, Local& L, const std::vector<u32>& c, const Box& Q, u32 nw_rt) {
  const u32 m = static_cast<u32>(c.size());
  const u32 nw = NW > 0 ? static_cast<u32>(NW) : nw_rt;
  L.memo.clear();
  L.lp.resize(m);
  L.lx.resize(m);
  L.lx2.resize(m);
  L.wl.resize(m);
  bool tight = true;
  for (u32 i = 0; i < m; ++i) {
    L.lp[i] = C.P[c[i]];
    L.lx[i] = C.X[c[i]];
    L.lx2[i] = C.X2[c[i]];
    L.wl[i] = C.w[c[i]];
    tight = tight && L.wl[i] == 1;
  }
  const P3* const lp = L.lp.data();
  const P3* const lx = L.lx.data();
  const i64* const lx2 = L.lx2.data();
  const u32* const wl = L.wl.data();
  const i64 K = C.K;
  const i64 th2 = K - 1, th3 = tight ? K - 2 : K - 1, th4 = tight ? K - 3 : K - 1;
  L.dom.assign(size_t(m) * nw, 0);
  L.live2.assign(size_t(m) * nw, 0);
  // triplets vivants : ligne (a, b), a < b, au rang a m - a (a + 1) / 2 + b - a - 1 (triangle superieur)
  L.live3.assign(size_t(m) * (m - 1) / 2 * nw, 0);
  u64* const Dm = L.dom.data();
  u64* const P2 = L.live2.data();
  u64* const H = L.live3.data();
  auto hrow = [&](u32 a, u32 b) { return H + (size_t(a) * m - size_t(a) * (a + 1) / 2 + (b - a - 1)) * nw; };
  auto has = [&](const u64* row, u32 b) { return (row[b >> 6] >> (b & 63)) & 1; };
  auto setbit = [&](u64* row, u32 b) { row[b >> 6] |= u64(1) << (b & 63); };
  auto wword = [&](u64 x, u32 t) -> u64 {
    if (tight) return popcount64(x);
    u64 s = 0;
    for (; x; x &= x - 1) s += wl[t * 64 + static_cast<u32>(std::countr_zero(x))];
    return s;
  };
  // masques de dominance (et bissectrices)
  for (u32 i = 0; i < m; ++i) {
    const P3 Xi = lx[i];
    const i64 xx = lx2[i];
    for (u32 j = i + 1; j < m; ++j) {
      const P3& Y = lx[j];
      const i64 dx = Y.x - Xi.x, dy = Y.y - Xi.y, dz = Y.z - Xi.z;
      const i64 base = lx2[j] - xx;
      // C . (X_j - X_i) sur Qbar : minimum au coin (lo si d > 0, hi sinon), maximum au coin oppose
      const i64 cmin =
          (dx > 0 ? Q.lo[0] : Q.hi[0]) * dx + (dy > 0 ? Q.lo[1] : Q.hi[1]) * dy + (dz > 0 ? Q.lo[2] : Q.hi[2]) * dz;
      const i64 cmax =
          (dx > 0 ? Q.hi[0] : Q.lo[0]) * dx + (dy > 0 ? Q.hi[1] : Q.lo[1]) * dy + (dz > 0 ? Q.hi[2] : Q.lo[2]) * dz;
      if (base - 2 * cmin < 0) setbit(Dm + size_t(i) * nw, j);
      else if (base - 2 * cmax > 0) setbit(Dm + size_t(j) * nw, i);
    }
  }
  L.led.leaf_dominance_tests += u64(m) * (m - 1) / 2;
  geom::Center ctr;
  // paires : q2 et paires vivantes
  for (u32 i = 0; i < m; ++i) {
    const u64* Di = Dm + size_t(i) * nw;
    for (u32 j = i + 1; j < m; ++j) {
      const u64* Dj = Dm + size_t(j) * nw;
      if (has(Di, j) || has(Dj, i)) continue;  // bissectrice disjointe de Qbar
      u64 d = 0;
      for (u32 t = 0; t < nw; ++t) d += wword(Di[t] | Dj[t], t);
      if (static_cast<i64>(d) > th2) continue;
      if (static_cast<i64>(d) <= th3) {
        setbit(P2 + size_t(i) * nw, j);
        setbit(P2 + size_t(j) * nw, i);
      }
      ++L.led.pair_tests;
      if (!midpoint_in_box(lx[i], lx[j], Q)) continue;
      geom::center2(lp[i], lp[j], ctr);
      judge(C, L, c, i, ctr, {c[i], c[j], kNone, kNone}, 2, th2);
    }
  }
  if (th3 < 0) return;  // aucune paire vivante
  // triplets : q3 et triplets vivants
  for (u32 i = 0; i < m; ++i) {
    const u64* Di = Dm + size_t(i) * nw;
    const u64* Pi = P2 + size_t(i) * nw;
    for (u32 tj = i >> 6; tj < nw; ++tj)
      for (u64 js = Pi[tj] & bits_above(i, tj); js; js &= js - 1) {
        const u32 j = tj * 64 + static_cast<u32>(std::countr_zero(js));
        const u64* Dj = Dm + size_t(j) * nw;
        const u64* Pj = P2 + size_t(j) * nw;
        for (u32 tk = j >> 6; tk < nw; ++tk)
          for (u64 ks = Pi[tk] & Pj[tk] & bits_above(j, tk); ks; ks &= ks - 1) {
            const u32 k = tk * 64 + static_cast<u32>(std::countr_zero(ks));
            const u64* Dk = Dm + size_t(k) * nw;
            u64 d = 0;
            for (u32 t = 0; t < nw; ++t) d += wword(Di[t] | Dj[t] | Dk[t], t);
            if (static_cast<i64>(d) > th3) continue;
            const int line = center_line_meets(lx[i], lx2[i], lx[j], lx2[j], lx[k], lx2[k], Q);
            if (line < 0) continue;  // alignes
            ++L.led.triple_tests;
            if (line == 0) continue;
            ++L.led.line_hits;
            if (static_cast<i64>(d) <= th4) {
              setbit(hrow(i, j), k);
              setbit(hrow(i, k), j);
              setbit(hrow(j, k), i);
            }
            const P3 &a = lp[i], &b = lp[j], &e = lp[k];
            if (geom::acute(a, b, e)) {
              geom::center3(a, b, e, ctr);
              if (center_in_box(a, ctr, Q)) judge(C, L, c, i, ctr, {c[i], c[j], c[k], kNone}, 3, th3);
            }
          }
      }
  }
  if (th4 < 0) return;  // aucun triplet vivant
  // quadruplets : quatre triplets vivants (H[i, j] n'est non vide que si (i, j) est vivante)
  for (u32 i = 0; i < m; ++i) {
    const u64* Di = Dm + size_t(i) * nw;
    const u64* Pi = P2 + size_t(i) * nw;
    for (u32 tj = i >> 6; tj < nw; ++tj)
      for (u64 js = Pi[tj] & bits_above(i, tj); js; js &= js - 1) {
        const u32 j = tj * 64 + static_cast<u32>(std::countr_zero(js));
        const u64* Hij = hrow(i, j);
        const u64* Dj = Dm + size_t(j) * nw;
        for (u32 tk = j >> 6; tk < nw; ++tk)
          for (u64 ks = Hij[tk] & bits_above(j, tk); ks; ks &= ks - 1) {
            const u32 k = tk * 64 + static_cast<u32>(std::countr_zero(ks));
            const u64* Hik = hrow(i, k);
            const u64* Hjk = hrow(j, k);
            const u64* Dk = Dm + size_t(k) * nw;
            for (u32 tl = k >> 6; tl < nw; ++tl)
              for (u64 ls = Hij[tl] & Hik[tl] & Hjk[tl] & bits_above(k, tl); ls; ls &= ls - 1) {
                const u32 l = tl * 64 + static_cast<u32>(std::countr_zero(ls));
                const u64* Dl = Dm + size_t(l) * nw;
                u64 d = 0;
                for (u32 t = 0; t < nw; ++t) d += wword(Di[t] | Dj[t] | Dk[t] | Dl[t], t);
                if (static_cast<i64>(d) > th4) continue;
                ++L.led.quad_tests;
                const P3 &a = lp[i], &b = lp[j], &e = lp[k], &f = lp[l];
                if (!geom::center4(a, b, e, f, ctr)) continue;
                if (!center_in_box(a, ctr, Q)) continue;
                const P3* t4[4] = {&a, &b, &e, &f};
                if (!geom::strictly_inside_tetra(t4, a, ctr)) continue;
                judge(C, L, c, i, ctr, {c[i], c[j], c[k], c[l]}, 4, th4);
              }
          }
      }
  }
}

void enumerate_leaf(const Ctx& C, Local& L, const std::vector<u32>& c, const Box& Q) {
  const u32 m = static_cast<u32>(c.size());
  if (m <= 64) enumerate_leaf_masks<1>(C, L, c, Q, 1);
  else if (m <= 128) enumerate_leaf_masks<2>(C, L, c, Q, 2);
  else if (m <= 256) enumerate_leaf_masks<4>(C, L, c, Q, 4);
  else enumerate_leaf_masks<0>(C, L, c, Q, (m + 63) / 64);
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
