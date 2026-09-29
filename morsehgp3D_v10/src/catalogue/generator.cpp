// Generateur du catalogue par boites de centres. Voir catalogue.hpp pour l'objet et les references.
//
// Repere de l'arbre : sites mis a l'echelle X = x << kT (kT = 6 bits sous-unitaires), boites entieres
// demi-ouvertes [lo, hi) de cotes quelconques (paves). Arbre binaire : chaque noeud ajuste sa boite a l'enveloppe de
// sa liste, puis la coupe au milieu de son plus long cote. En u18 : |X| < 2^24, tests de dominance (forme D-loc) et
// bissectrices en i64 ; test droite des centres (lemme Z, zonogone) en i128 ; centres et recensement en i128.
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <memory>
#include <mutex>
#include <numeric>

#include "catalogue/catalogue.hpp"
#include "sched/sort.hpp"

namespace mhgp10 {

namespace {

constexpr int kT = 6;
// Stagnation : sous l'echelle de la grille (plus long cote de la boite ajustee <= 2^kT, un millimetre en u18), une
// liste de plus de M sites qui ne decroit plus signale une degenerescence cospherique. Si le centre commun c de m
// sites cospheriques est dans la boite fermee, aucun d'eux n'en domine un autre (distances egales en c) : la liste
// garde les m sites a toute profondeur, alors qu'une configuration generique converge vers les K plus proches
// voisins (K < M). On arrete apres 9 niveaux binaires sans decroissance : volume divise par 2^9, soit chaque cote
// par 8, comme les 3 niveaux de l'octree precedent.
constexpr int kStagnationLimit = 9;
// Reservoir des dominateurs d'un noeud : au plus 3 K sites.
constexpr u32 kPool = 3 * kMaxCatalogueOrder;
// Filtre des noeuds en i64 : la plus grande quantite est la cle du reservoir, < 27 2^(2 (B + T)).
static_assert(2 * (kCoordinateBits + kT) + 5 <= 63, "filtre des noeuds : bornes i64");

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

struct alignas(64) Local {  // une par fil : pas de faux partage entre compteurs voisins
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
  a.filter_tests += b.filter_tests;
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

// Filtre d'un noeud : liste certifiee de la boite a partir de la liste parente (lemme D). Reservoir : les
// min(|parent|, 3K) sites les plus proches du centre de la boite, ordre (distance, site) ; Y : son plus petit
// prefixe de poids >= 3K (ou tout le reservoir) ; x est exclu si et seulement si ses dominateurs dans Y pesent >= K.
// Forme D-loc, repere local x' = X - lo, A = |x'|^2, boite de cotes h_i = hi_i - lo_i :
//   max_{C dans Qbar} (|Y - C|^2 - |X - C|^2) = A(y) - A(x) + sum_i max(0, 2h_i x'_i - 2h_i y'_i),
// donc y domine x si et seulement si A(x) - A(y) > sum_i max(0, 2h_i x'_i - 2h_i y'_i) : le meme entier que la forme
// par coins, donc la meme decision. Cle du reservoir : dd = |2X - (lo + hi)|^2 = sum_i (2x'_i - h_i)^2, distance au
// centre exact de la boite (x4). Bornes (u18, T = 6) : |x'_i| < 2^24, 2h_i <= 2^25, A < 3 2^48, |2h_i x'_i| < 2^49,
// membre droit < 3 2^49, dd < 27 2^48 : i64.
// Garde G omise (corollaire G inclus dans D) : S0, plus petit prefixe du reservoir de poids >= K, est inclus dans Y ;
// un x que G exclut est domine par tout S0, donc par un poids >= K de Y, et D l'exclut aussi ; reservoir de poids
// < K : ni G ni D n'excluent. Comptage sans branchement sur S0 puis sur Y \ S0 : le poids partiel ne fait que
// croitre, donc s'arreter apres S0 des qu'il atteint K ne change pas la decision.
// Les listes sont des sous-suites de (0, ..., n - 1) : le rang dans la liste departage comme l'indice de site.
// Hors ligne : inline dans process (recursif), le noyau deborde ses registres sur la pile.
[[gnu::noinline]] void filter_node(const Ctx& C, Local& L, const Box& Q, const std::vector<u32>& parent,
                                   std::vector<u32>& cand) {
  const u32 np = static_cast<u32>(parent.size());
  const i64 hx = Q.hi[0] - Q.lo[0], hy = Q.hi[1] - Q.lo[1], hz = Q.hi[2] - Q.lo[2];
  // reservoir par insertion, cle (dd, rang)
  const u32 cap = std::min<u32>(np, 3u * static_cast<u32>(C.K));
  i64 kd[kPool];
  u32 kr[kPool], nb = 0;
  for (u32 t = 0; t < np; ++t) {
    const P3& X = C.X[parent[t]];
    const i64 da = 2 * (X.x - Q.lo[0]) - hx, db = 2 * (X.y - Q.lo[1]) - hy, dc = 2 * (X.z - Q.lo[2]) - hz;
    const i64 dd = da * da + db * db + dc * dc;
    if (nb == cap && dd >= kd[nb - 1]) continue;
    u32 j = nb < cap ? nb++ : nb - 1;
    for (; j > 0 && kd[j - 1] > dd; --j) {
      kd[j] = kd[j - 1];
      kr[j] = kr[j - 1];
    }
    kd[j] = dd;
    kr[j] = t;
  }
  // Y en SoA (2h_i x'_i, A, poids), S0 = ses s0 premiers
  i64 yx[kPool], yy[kPool], yz[kPool], ya[kPool], yw[kPool];
  u32 ny = 0, s0 = 0;
  for (u64 acc = 0; ny < nb && acc < 3 * u64(C.K); ++ny) {
    const u32 s = parent[kr[ny]];
    const i64 a = C.X[s].x - Q.lo[0], b = C.X[s].y - Q.lo[1], c = C.X[s].z - Q.lo[2];
    yx[ny] = 2 * hx * a;
    yy[ny] = 2 * hy * b;
    yz[ny] = 2 * hz * c;
    ya[ny] = a * a + b * b + c * c;
    yw[ny] = C.w[s];
    acc += C.w[s];
    if (s0 == 0 && acc >= u64(C.K)) s0 = ny + 1;
  }
  if (s0 == 0) s0 = ny;
  // sortie sans branchement, dans l'ordre de la liste parente
  cand.resize(np);
  u32 m = 0;
  u64 tests = 0;
  for (u32 t = 0; t < np; ++t) {
    const u32 s = parent[t];
    const i64 a = C.X[s].x - Q.lo[0], b = C.X[s].y - Q.lo[1], c = C.X[s].z - Q.lo[2];
    const i64 px = 2 * hx * a, py = 2 * hy * b, pz = 2 * hz * c, pa = a * a + b * b + c * c;
    auto weight = [&](u32 r0, u32 r1) {  // poids des dominateurs de x parmi Y[r0, r1)
      i64 w = 0;
      for (u32 r = r0; r < r1; ++r) {
        const i64 d = std::max<i64>(px - yx[r], 0) + std::max<i64>(py - yy[r], 0) + std::max<i64>(pz - yz[r], 0);
        w += -i64(pa - ya[r] > d) & yw[r];  // masque : sans branchement
      }
      return w;
    };
    i64 dom = weight(0, s0);
    tests += s0;
    if (dom < C.K) {
      dom += weight(s0, ny);
      tests += ny - s0;
    }
    cand[m] = s;
    m += dom < C.K;
  }
  cand.resize(m);
  L.led.filter_tests += tests;
}

// Noeud : liste certifiee de Q, puis boite ajustee S = Q inter [env.lo, env.hi + 1), env enveloppe fermee de la liste
// (coordonnees entieres). S contient tout centre admis de Q : une boule admise a son support dans sa coquille, donc
// dans la liste (theoreme C), et son centre dans l'interieur relatif de conv(support), inclus dans l'enveloppe fermee.
// S vide est le lemme K (aucun centre possible). S est coupee en deux au milieu de son plus long cote : les deux
// moities pavent S, chaque centre admis reste dans une seule feuille. Une boite fille est incluse dans S, donc dans
// l'enveloppe de sa liste parente : la pre-ignorance par cette enveloppe (J2) ne se declencherait jamais.
void process(const Ctx& C, Local& L, const Box& Q, const std::vector<u32>& parent, int parent_stag,
             std::vector<Task>* spill) {
  ++L.led.nodes;
  std::vector<u32> cand;
  filter_node(C, L, Q, parent, cand);
  // Lemme K : liste vide, ou boite ajustee vide sur un axe (l'enveloppe manque Q) : aucun centre possible dans Q.
  if (cand.empty()) {
    ++L.led.skipped_bbox;
    return;
  }
  Box S = {{INT64_MAX, INT64_MAX, INT64_MAX}, {INT64_MIN, INT64_MIN, INT64_MIN}};  // enveloppe, puis boite ajustee
  for (u32 s : cand) {
    const P3& x = C.X[s];
    S.lo[0] = std::min(S.lo[0], x.x);
    S.lo[1] = std::min(S.lo[1], x.y);
    S.lo[2] = std::min(S.lo[2], x.z);
    S.hi[0] = std::max(S.hi[0], x.x);
    S.hi[1] = std::max(S.hi[1], x.y);
    S.hi[2] = std::max(S.hi[2], x.z);
  }
  int ax = 0;  // axe du plus long cote de S (le premier en cas d'egalite)
  for (int a = 0; a < 3; ++a) {
    S.lo[a] = std::max(S.lo[a], Q.lo[a]);
    S.hi[a] = std::min(S.hi[a] + 1, Q.hi[a]);
    if (S.lo[a] >= S.hi[a]) {
      ++L.led.skipped_bbox;
      return;
    }
    if (S.hi[a] - S.lo[a] > S.hi[ax] - S.lo[ax]) ax = a;
  }
  const i64 side = S.hi[ax] - S.lo[ax];
  const int stag = (side <= (i64(1) << kT) && cand.size() == parent.size()) ? parent_stag + 1 : 0;
  if (cand.size() > C.M && side > 1 && stag < kStagnationLimit) {
    const i64 mid = (S.lo[ax] + S.hi[ax]) / 2;
    auto shared = spill ? std::make_shared<const std::vector<u32>>(cand) : nullptr;
    for (int o = 0; o < 2; ++o) {
      Box ch = S;
      (o ? ch.lo[ax] : ch.hi[ax]) = mid;
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
  enumerate_leaf(C, L, cand, S);
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
  // M(K) : taille de feuille, table fixe calibree sur t_boxes de l'arbre binaire (trames 00 et 02, quart 01,
  // synthetique ; K = 3, 5, 10, 12) ; le catalogue n'en depend pas.
  if (C.M == 0) C.M = params.kmax <= 3 ? 12 : (params.kmax <= 6 ? 16 : (params.kmax <= 10 ? 24 : 28));
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
  using Clock = std::chrono::steady_clock;
  auto since = [](Clock::time_point t) { return std::chrono::duration<double>(Clock::now() - t).count(); };
  auto t0 = Clock::now();
  auto all = std::make_shared<std::vector<u32>>(n);
  for (u32 s = 0; s < n; ++s) (*all)[s] = s;
  std::vector<Task> frontier{Task{root, all, 0}};
  std::vector<Local> locals(pool.size());
  const size_t target = size_t(64) * pool.size();
  // Frontiere pilotee par la charge (GEN_v2 § 10.1) : le cout d'une tache suit le nombre de sites de sa boite, et la
  // densite LiDAR est tres concentree (une cellule de 2 m porte 6 % des sites de la trame 02). Apres la cible en
  // nombre, on ne developpe plus que les taches de plus de n / (64 P) sites ; les taches finales partent par charge
  // decroissante. L'arbre et la sortie (ordre canonique) ne dependent pas de cette coupe.
  const u64 cap = std::max<u64>(1, u64(n) / target);
  auto inside = [&](const Task& t) {
    u64 c = 0;
    for (u32 s : *t.parent) {
      const P3& x = C.X[s];
      c += x.x >= t.box.lo[0] && x.x < t.box.hi[0] && x.y >= t.box.lo[1] && x.y < t.box.hi[1] &&
           x.z >= t.box.lo[2] && x.z < t.box.hi[2];
    }
    return c;
  };
  std::vector<Task> kept;
  std::vector<u64> kept_load;
  while (!frontier.empty()) {
    if (frontier.size() >= target) {
      std::vector<u64> load(frontier.size());
      pool.parallel_for(frontier.size(), 64, [&](u64 b, u64 e, unsigned) {
        for (u64 i = b; i < e; ++i) load[i] = inside(frontier[i]);
      });
      std::vector<Task> heavy;
      for (size_t i = 0; i < frontier.size(); ++i) {
        if (load[i] > cap) {
          heavy.push_back(std::move(frontier[i]));
        } else {
          kept.push_back(std::move(frontier[i]));
          kept_load.push_back(load[i]);
        }
      }
      frontier.swap(heavy);
      if (frontier.empty()) break;
    }
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
  for (Task& t : frontier) {  // frontiere epuisee avant la cible (petits nuages)
    kept_load.push_back(inside(t));
    kept.push_back(std::move(t));
  }
  frontier.clear();
  std::vector<u32> order(kept.size());
  std::iota(order.begin(), order.end(), 0u);
  std::stable_sort(order.begin(), order.end(), [&](u32 a, u32 b) { return kept_load[a] > kept_load[b]; });
  cat.tasks = kept.size();
  cat.max_task_sites = kept_load.empty() ? 0 : kept_load[order[0]];
  cat.t_frontier = since(t0);
  t0 = Clock::now();
  pool.parallel_for(order.size(), 1, [&](u64 b, u64 e, unsigned wk) {
    for (u64 i = b; i < e; ++i) {
      const Task& t = kept[order[i]];
      process(C, locals[wk], t.box, *t.parent, t.parent_stag, nullptr);
    }
  });
  kept.clear();
  cat.t_boxes = since(t0);
  t0 = Clock::now();
  // Echec eventuel (feuille trop large) : refus transactionnel.
  for (const Local& L : locals)
    if (!L.fail.ok()) return L.fail;
  // Rassemblement et ordre canonique (niveau exact, S*), en parallele : collecte a des positions fixees par des
  // prefixes, tri parallele sur la cle (approximation du niveau, S*) qui est un ordre total hors boule emise deux
  // fois (refusee plus bas), donc resultat identique au tri sequentiel ; puis reparation exacte des bandes.
  struct Ref {
    double approx;
    std::array<u32, 4> sup;
    u32 local, rec;
  };
  std::vector<u64> first(locals.size() + 1, 0);
  for (u32 li = 0; li < locals.size(); ++li) {
    merge_ledger(cat.ledger, locals[li].led);
    first[li + 1] = first[li] + locals[li].recs.size();
  }
  UninitVector<Ref> refs(first.back());
  pool.parallel_for(locals.size(), 1, [&](u64 b, u64 e, unsigned) {
    for (u64 li = b; li < e; ++li)
      for (u32 r = 0; r < locals[li].recs.size(); ++r)
        refs[first[li] + r] = Ref{locals[li].recs[r].level.approx(), locals[li].recs[r].sup, u32(li), r};
  });
  auto rec = [&](const Ref& x) -> const Rec& { return locals[x.local].recs[x.rec]; };
  cat.t_collect = since(t0);
  auto t1 = Clock::now();
  sched::parallel_sort(pool, refs, [](const Ref& x, const Ref& y) {
    if (x.approx != y.approx) return x.approx < y.approx;
    return x.sup < y.sup;
  });
  cat.t_sort = since(t1);
  t1 = Clock::now();
  // reparation exacte des bandes flottantes (erreur relative de approx < 2^-50) : bandes reperees en serie,
  // triees en parallele (chacune ne touche que ses positions)
  std::vector<std::pair<u64, u64>> bands;
  for (size_t i = 0; i < refs.size();) {
    size_t j = i + 1;
    while (j < refs.size() && refs[j].approx - refs[j - 1].approx <= refs[j].approx * 0x1p-40) ++j;
    if (j - i > 1) {
      bands.push_back({i, j});
      cat.band_members += j - i;
    }
    i = j;
  }
  cat.bands = bands.size();
  pool.parallel_for(bands.size(), 16, [&](u64 b0, u64 e0, unsigned) {
    for (u64 q = b0; q < e0; ++q)
      std::sort(refs.begin() + bands[q].first, refs.begin() + bands[q].second, [&](const Ref& x, const Ref& y) {
        const int c = geom::compare(rec(x).level, rec(y).level);
        if (c != 0) return c < 0;
        return x.sup < y.sup;
      });
  });
  cat.t_bands = since(t1);
  cat.t_order = since(t0);
  t0 = Clock::now();
  const u32 nb = static_cast<u32>(refs.size());
  // comparaisons exactes des voisins en parallele (cmp[0] n'est jamais lu) ; le premier defaut, dans l'ordre, decide
  // du refus
  UninitVector<signed char> cmp(nb);
  t1 = Clock::now();
  pool.parallel_for(nb, 4096, [&](u64 b0, u64 e0, unsigned) {
    for (u64 b = std::max<u64>(b0, 1); b < e0; ++b)
      cmp[b] = static_cast<signed char>(geom::compare(rec(refs[b - 1]).level, rec(refs[b]).level));
  });
  cat.rank.resize(nb);
  cat.support.resize(nb);
  cat.qmin.resize(nb);
  cat.p.resize(nb);
  cat.u.resize(nb);
  cat.flags.resize(nb);
  cat.n_interior.resize(nb);
  cat.pop_off.resize(u64(nb) + 1);
  cat.pop_off[0] = 0;
  cat.t_compare = since(t1);
  t1 = Clock::now();
  // premier defaut dans l'ordre (refus deterministe), rangs et decalages par sommes prefixes paralleles
  const u64 chunks = std::min<u64>(u64(pool.size()) * 8, std::max<u64>(1, nb / 4096));
  auto chunk_lo = [&](u64 c) { return u64(nb) * c / chunks; };
  std::vector<u64> bad(chunks, u64(nb)), inc(chunks + 1, 0), pops(chunks + 1, 0);
  pool.parallel_for(chunks, 1, [&](u64 c0, u64 c1, unsigned) {
    for (u64 c = c0; c < c1; ++c)
      for (u64 b = chunk_lo(c); b < chunk_lo(c + 1); ++b) {
        if (b > 0 && bad[c] == u64(nb) && (cmp[b] > 0 || (cmp[b] == 0 && refs[b - 1].sup == refs[b].sup))) bad[c] = b;
        if (b > 0 && cmp[b] < 0) ++inc[c + 1];
        pops[c + 1] += rec(refs[b]).pop_len;
      }
  });
  for (u64 c = 0; c < chunks; ++c) {
    if (bad[c] < nb) return fail(cmp[bad[c]] > 0 ? Reason::rank_order : Reason::census_mismatch);  // boule emise 2 fois
    inc[c + 1] += inc[c];
    pops[c + 1] += pops[c];
  }
  cat.level.resize(nb ? inc[chunks] + 1 : 0);
  pool.parallel_for(chunks, 1, [&](u64 c0, u64 c1, unsigned) {
    for (u64 c = c0; c < c1; ++c) {
      u32 rank = u32(inc[c]);
      u64 off = pops[c];
      for (u64 b = chunk_lo(c); b < chunk_lo(c + 1); ++b) {
        if (b > 0 && cmp[b] < 0) ++rank;
        if (b == 0 || cmp[b] < 0) cat.level[rank] = rec(refs[b]).level;
        cat.rank[b] = rank;
        cat.pop_off[b] = off;
        off += rec(refs[b]).pop_len;
      }
    }
  });
  cat.pop_off[nb] = pops[chunks];
  cat.pop.resize(cat.pop_off[nb]);
  cat.t_ranks = since(t1);
  t1 = Clock::now();
  pool.parallel_for(nb, 4096, [&](u64 b0, u64 e0, unsigned) {
    for (u64 b = b0; b < e0; ++b) {
      const Rec& r = rec(refs[b]);
      cat.support[b] = r.sup;
      cat.qmin[b] = r.q;
      cat.p[b] = r.p;
      cat.u[b] = r.u;
      cat.flags[b] = r.flags;
      cat.n_interior[b] = r.n_i;
      const std::vector<u32>& src = locals[refs[b].local].pop;
      std::copy(src.begin() + r.pop_begin, src.begin() + r.pop_begin + r.pop_len, cat.pop.begin() + cat.pop_off[b]);
    }
  });
  cat.t_copy = since(t1);
  cat.t_assemble = since(t0);
  return cat;
}

}  // namespace mhgp10
