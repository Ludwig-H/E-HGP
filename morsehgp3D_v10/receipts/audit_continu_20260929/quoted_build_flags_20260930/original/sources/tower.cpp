#include "tower/tower.hpp"

#include <algorithm>
#include <atomic>
#include <cfenv>
#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>

#include "catalogue/support.hpp"
#include "tower/rank_search.hpp"

// Filtres flottants de la tour (marge kApproxMargin, orientation semi-statique ; doctrine v4 : filtre certifie a repli
// exact). Leurs preuves supposent IEEE-754 binaire64 strict et l'arrondi au plus proche, sans reassociation, sans
// division remplacee par un produit par l'inverse, sans precision etendue ni contraction FMA. Meme refus a la
// compilation que site_tree.cpp (memes macros, memes limites de clang) ; CMakeLists.txt refuse -Ofast et -ffast-math
// dans chaque CMAKE_CXX_FLAGS* et impose -ffp-contract=off a toutes les unites ; hors FE_TONEAREST, dans le fil qui
// execute la descente, les filtres sont coupes (resolve). Portes : mhgp10_regression_tower_fast_math,
// mhgp10_regression_tower_rounding.
#if defined(__FAST_MATH__) || defined(__ASSOCIATIVE_MATH__) || defined(__RECIPROCAL_MATH__)
#error "mhgp10_tower_fast_math_interdit : les filtres flottants de la tour exigent IEEE-754 strict (pas de -ffast-math)"
#endif
#if defined(__FLT_EVAL_METHOD__) && __FLT_EVAL_METHOD__ != 0
#error "mhgp10_tower_precision_etendue_interdite : les filtres flottants de la tour exigent le binaire64 strict"
#endif

namespace mhgp10 {

namespace {

using geom::P3;
using Clock = std::chrono::steady_clock;
double seconds_since(Clock::time_point t0) { return std::chrono::duration<double>(Clock::now() - t0).count(); }

constexpr u32 kMaxFacet = kMaxCatalogueOrder;
constexpr u64 kMaxSubsets = 20000;  // quotient local d'une coquille etendue : au-dela, refus explicite
constexpr u32 kMaxShellEnum = 24;   // coquille etendue de plus de 24 sites : refus explicite
constexpr u32 kOrders = kMaxCatalogueOrder + 1;
// Marge des filtres de distance flottants : deux fois une borne large (< 1e-3 en u18) de l'erreur absolue d'une
// distance carree calculee en double au centre approche a + N / D ; la meme que celle de SiteTree (preuve dans
// site_tree.cpp, arrondi au plus proche, sans contraction). Hors FE_TONEAREST, les filtres sont coupes (resolve).
constexpr double kApproxMargin = 0.02;

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

// F = A u B (deux suites triees disjointes), trie.
inline void merge_into(Facet& F, const u32* A, u32 na, const u32* B, u32 nb) {
  u32 i = 0, j = 0;
  F.n = na + nb;
  for (u32 x = 0; x < F.n; ++x) F.s[x] = (j >= nb || (i < na && A[i] < B[j])) ? A[i++] : B[j++];
}

// ----------------------------------------------------------------- index plats

inline u64 mix64(u64 h) {
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccdull;
  h ^= h >> 33;
  h *= 0xc4ceb9fe1a85ec53ull;
  h ^= h >> 33;
  return h;
}

// Hachage d'une suite de sites (sert a l'adressage seulement : toute egalite est verifiee sur les cles).
inline u64 hash_sites(const u32* s, u32 n) {
  static constexpr u64 kMul[12] = {0x9E3779B97F4A7C15ull, 0xC2B2AE3D27D4EB4Full, 0x165667B19E3779F9ull,
                                   0xD6E8FEB86659FD93ull, 0xFF51AFD7ED558CCDull, 0xC4CEB9FE1A85EC53ull,
                                   0x94D049BB133111EBull, 0xBF58476D1CE4E5B9ull, 0x2545F4914F6CDD1Dull,
                                   0x9FB21C651E98DF25ull, 0x4CF5AD432745937Full, 0xDAA66D2C7DDF743Full};
  u64 h = n;
  for (u32 i = 0; i < n && i < 12; ++i) h += (u64(s[i]) + 1) * kMul[i];
  return mix64(h);
}

// Adressage ouvert, sondage lineaire ; une entree = (etiquette 32 bits << 32) | (valeur + 1), 0 = vide. La cle
// n'est pas stockee : l'appelant la verifie sur la valeur. Insertion concurrente par CAS ; si la cle est deja
// presente, la plus petite valeur est gardee. La table ne depend donc pas de l'ordre d'insertion (une cle occupe
// une seule entree, la premiere libre de sa sequence de sondage au moment ou elle y arrive, et y porte le minimum).
class FlatIndex {
 public:
  // Alloue (non initialise) ; zero() doit couvrir [0, capacity()) avant toute insertion.
  [[nodiscard]] bool init(u64 n) {
    u64 cap = 16;
    while (cap < n + n / 2 + 1) cap <<= 1;
    cap_ = cap;
    return slots_.allocate(cap);
  }
  void zero(u64 b, u64 e) { std::memset(static_cast<void*>(slots_.data() + b), 0, (e - b) * sizeof(u64)); }
  u64 capacity() const { return cap_; }
  // Prechargement : premiere entree de la sequence de sondage de h ; valeur candidate si l'etiquette y concorde.
  const u64* first_slot(u64 h) const { return cap_ ? slots_.data() + (h & (cap_ - 1)) : nullptr; }
  static u32 candidate(u64 cur, u64 h) { return cur != 0 && (cur >> 32) == (h >> 32) ? static_cast<u32>(cur) - 1 : kNone; }
  template <class SameKey>
  void insert(u64 h, u32 value, SameKey same_key) {
    const u64 tag = h >> 32;
    const u64 entry = (tag << 32) | (u64(value) + 1);
    u64 pos = h & (cap_ - 1);
    for (;;) {
      std::atomic_ref<u64> slot(slots_[pos]);
      u64 cur = slot.load(std::memory_order_acquire);
      if (cur == 0) {
        if (slot.compare_exchange_strong(cur, entry, std::memory_order_acq_rel)) return;
        // cur recharge : l'entree vient d'etre prise, on la reexamine
      }
      if ((cur >> 32) == tag && same_key(static_cast<u32>(cur) - 1)) {
        while (static_cast<u32>(cur) - 1 > value)
          if (slot.compare_exchange_weak(cur, entry, std::memory_order_acq_rel)) return;
        return;
      }
      pos = (pos + 1) & (cap_ - 1);
    }
  }
  template <class Matches>
  u32 find(u64 h, Matches matches) const {
    if (cap_ == 0) return kNone;
    const u64 tag = h >> 32;
    u64 pos = h & (cap_ - 1);
    for (;;) {
      const u64 cur = slots_[pos];  // lecture apres la construction (barriere de fin du parallel_for)
      if (cur == 0) return kNone;
      if ((cur >> 32) == tag && matches(static_cast<u32>(cur) - 1)) return static_cast<u32>(cur) - 1;
      pos = (pos + 1) & (cap_ - 1);
    }
  }

 private:
  u64 cap_ = 0;
  Buffer<u64> slots_;
};

struct Sphere {
  bool empty = true, zero = false;
  u32 anchor = kNone;
  geom::Center c{};
  geom::Level level{};
  // MEB certifiee : support R (sites) et centre dans l'interieur relatif de conv(R) (strict) ; nr = 0 si inconnu
  u8 nr = 0;
  bool strict = false;
  u32 R[4] = {kNone, kNone, kNone, kNone};
  // Niveau exact calcule a la demande (level_of) ; centre approche cq = a + N / D et r2a = |a - cq|^2 en double.
  // Le centre est dans conv(F) (MEB), donc dans la boite des sites : l'erreur absolue de toute distance carree
  // approchee au centre approche, r2a compris, est < 1e-3 en u18 (borne de SiteTree).
  bool has_level = false;
  double cq[3] = {0, 0, 0};
  double r2a = 0;
};

inline double approx_d2(const Sphere& S, const P3& z) {
  const double dx = double(z.x) - S.cq[0], dy = double(z.y) - S.cq[1], dz = double(z.z) - S.cq[2];
  return dx * dx + dy * dy + dz * dz;
}

void set_approx(Sphere& S, const P3& a) {
  if (S.zero) {
    S.cq[0] = double(a.x);
    S.cq[1] = double(a.y);
    S.cq[2] = double(a.z);
    S.r2a = 0;
    return;
  }
  const double D = static_cast<double>(S.c.D);
  S.cq[0] = double(a.x) + static_cast<double>(S.c.N[0]) / D;
  S.cq[1] = double(a.y) + static_cast<double>(S.c.N[1]) / D;
  S.cq[2] = double(a.z) + static_cast<double>(S.c.N[2]) / D;
  S.r2a = approx_d2(S, a);
}

struct Geo {
  const Cloud& cloud;
  const SiteTree& tree;
  const Catalogue& cat;
  std::vector<P3> P;
  FlatIndex lookup;  // support canonique -> boule (la plus petite si doublon)
  const P3& operator()(u32 s) const { return P[s]; }
  // pre : adresse a precharger pour la boule candidate (donnees de la boule lues ensuite), avant la verification
  // de la cle, pour que les deux defauts de cache se recouvrent.
  template <class T = char>
  u32 find_ball(const std::array<u32, 4>& sup, const T* pre = nullptr) const {
    return lookup.find(hash_sites(sup.data(), 4), [&](u32 b) {
      if (pre) __builtin_prefetch(pre + b);
      return cat.support[b] == sup;
    });
  }
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

// Proposition flottante (Welzl en double) : seul le support propose en sort. Aucune decision n'en depend : la
// sphere exacte passant par ce support est certifiee ou rejetee par verify_meb.
struct DBall {
  double c[3] = {0, 0, 0};
  double r2 = -1;
  int R[4] = {0, 0, 0, 0};
  int nr = 0;
};

struct DWelzl {
  double p[kMaxFacet][3];
  bool ok = true;

  DBall through(const int* R, int nr) {
    DBall B;
    B.nr = nr;
    for (int i = 0; i < nr; ++i) B.R[i] = R[i];
    if (nr == 0) return B;
    const double* a = p[R[0]];
    if (nr == 1) {
      for (int i = 0; i < 3; ++i) B.c[i] = a[i];
      B.r2 = 0;
      return B;
    }
    if (nr == 2) {
      const double* b = p[R[1]];
      double r2 = 0;
      for (int i = 0; i < 3; ++i) {
        B.c[i] = 0.5 * (a[i] + b[i]);
        const double d = b[i] - a[i];
        r2 += d * d;
      }
      B.r2 = 0.25 * r2;
      return B;
    }
    if (nr == 3) {
      const double *b = p[R[1]], *d = p[R[2]];
      const double u[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
      const double v[3] = {d[0] - a[0], d[1] - a[1], d[2] - a[2]};
      const double w[3] = {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
      const double uu = u[0] * u[0] + u[1] * u[1] + u[2] * u[2], vv = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
      const double ww = w[0] * w[0] + w[1] * w[1] + w[2] * w[2];
      if (!(ww > 1e-9 * uu * vv)) {
        ok = false;
        return B;
      }
      const double t[3] = {uu * v[0] - vv * u[0], uu * v[1] - vv * u[1], uu * v[2] - vv * u[2]};
      const double n[3] = {t[1] * w[2] - t[2] * w[1], t[2] * w[0] - t[0] * w[2], t[0] * w[1] - t[1] * w[0]};
      const double D = 2 * ww;
      double r2 = 0;
      for (int i = 0; i < 3; ++i) {
        const double o = n[i] / D;
        B.c[i] = a[i] + o;
        r2 += o * o;
      }
      B.r2 = r2;
      return B;
    }
    const double *b = p[R[1]], *d = p[R[2]], *e = p[R[3]];
    const double u[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    const double v[3] = {d[0] - a[0], d[1] - a[1], d[2] - a[2]};
    const double s[3] = {e[0] - a[0], e[1] - a[1], e[2] - a[2]};
    const double vs[3] = {v[1] * s[2] - v[2] * s[1], v[2] * s[0] - v[0] * s[2], v[0] * s[1] - v[1] * s[0]};
    const double su[3] = {s[1] * u[2] - s[2] * u[1], s[2] * u[0] - s[0] * u[2], s[0] * u[1] - s[1] * u[0]};
    const double uv[3] = {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
    const double det = u[0] * vs[0] + u[1] * vs[1] + u[2] * vs[2];
    const double uu = u[0] * u[0] + u[1] * u[1] + u[2] * u[2], vv = v[0] * v[0] + v[1] * v[1] + v[2] * v[2],
                 ss = s[0] * s[0] + s[1] * s[1] + s[2] * s[2];
    if (!(det * det > 1e-9 * uu * vv * ss)) {
      ok = false;
      return B;
    }
    const double D = 2 * det;
    double r2 = 0;
    for (int i = 0; i < 3; ++i) {
      const double o = (uu * vs[i] + vv * su[i] + ss * uv[i]) / D;
      B.c[i] = a[i] + o;
      r2 += o * o;
    }
    B.r2 = r2;
    return B;
  }
  bool contains(const DBall& B, int i) const {
    if (B.r2 < 0) return false;
    double d2 = 0;
    for (int a = 0; a < 3; ++a) {
      const double t = p[i][a] - B.c[a];
      d2 += t * t;
    }
    return d2 <= B.r2 * (1 + 1e-10) + 1e-9;
  }
  // Welzl a deplacement en tete (Gaertner) : L est la liste des points, S le support courant.
  int L[kMaxFacet];
  int S[4];
  int ns = 0;
  DBall ball;
  void mtf(int end) {
    ball = through(S, ns);
    if (!ok || ns == 4) return;
    for (int i = 0; i < end; ++i) {
      if (contains(ball, L[i])) continue;
      S[ns++] = L[i];
      mtf(i);
      --ns;
      if (!ok) return;
      const int v = L[i];
      for (int j = i; j > 0; --j) L[j] = L[j - 1];
      L[0] = v;
    }
  }
  // Welzl recursif sur T[0..n) avec R au bord (petits ensembles : |T| <= 4).
  DBall small(const int* T, int n, int* R, int nr) {
    if (!ok) return DBall{};
    if (n == 0 || nr == 4) return through(R, nr);
    DBall B = small(T, n - 1, R, nr);
    if (!ok || contains(B, T[n - 1])) return B;
    R[nr] = T[n - 1];
    return small(T, n - 1, R, nr + 1);
  }
  // Proposition par mise a jour du support : depart sur une paire eloignee (le plus loin du barycentre, puis le
  // plus loin de lui), puis, tant qu'un point sort de la boule, MEB(support u {pire point}) avec ce point au bord
  // (lemme de Welzl : un point hors de mb(P) est au bord de mb(P u {p})). Le rayon croit strictement a chaque tour.
  // Heuristique de cout seulement : un echec (degenerescence flottante, tours epuises) passe a Welzl complet.
  DBall run_support(int n) {
    double gc[3] = {0, 0, 0};
    for (int i = 0; i < n; ++i)
      for (int a = 0; a < 3; ++a) gc[a] += p[i][a];
    for (int a = 0; a < 3; ++a) gc[a] /= n;
    auto far_from = [&](const double* q) {
      int best = 0;
      double bd = -1;
      for (int i = 0; i < n; ++i) {
        double d = 0;
        for (int a = 0; a < 3; ++a) d += (p[i][a] - q[a]) * (p[i][a] - q[a]);
        if (d > bd) {
          bd = d;
          best = i;
        }
      }
      return best;
    };
    const int ia = far_from(gc);
    const int ib = far_from(p[ia]);
    if (ia == ib) {
      ok = false;
      return DBall{};
    }
    int S[4] = {ia, ib, 0, 0};
    DBall B = through(S, 2);
    for (int iter = 0; iter < 8 && ok; ++iter) {
      int v = -1;
      double worst = 0;
      for (int i = 0; i < n; ++i) {
        if (contains(B, i)) continue;
        double d = 0;
        for (int a = 0; a < 3; ++a) d += (p[i][a] - B.c[a]) * (p[i][a] - B.c[a]);
        if (d - B.r2 > worst) {
          worst = d - B.r2;
          v = i;
        }
      }
      if (v < 0) return B;
      int T[4];
      const int nt = B.nr;
      for (int i = 0; i < nt; ++i) T[i] = B.R[i];
      int R[4] = {v, 0, 0, 0};
      B = small(T, nt, R, 1);
    }
    ok = false;
    return DBall{};
  }
  DBall run(int n) {
    {
      const DBall B = run_support(n);
      if (ok) return B;
      ok = true;
    }
    // repli : Welzl a deplacement en tete, du plus loin au plus proche du barycentre
    double g[3] = {0, 0, 0};
    for (int i = 0; i < n; ++i)
      for (int a = 0; a < 3; ++a) g[a] += p[i][a];
    for (int a = 0; a < 3; ++a) g[a] /= n;
    double key[kMaxFacet];
    for (int i = 0; i < n; ++i) {
      double d = 0;
      for (int a = 0; a < 3; ++a) d += (p[i][a] - g[a]) * (p[i][a] - g[a]);
      int j = i;
      while (j > 0 && key[j - 1] < d) {
        key[j] = key[j - 1];
        L[j] = L[j - 1];
        --j;
      }
      key[j] = d;
      L[j] = i;
    }
    ns = 0;
    mtf(n);
    return ball;
  }
};

// Signe de w . (c - p) pour le centre c = anchor + N / D et le plan (p, q, r), w = (q - p) x (r - p) : meme predicat
// que geom::orient_center_wide (D > 0). Filtre semi-statique : w exact (< 2^39, exact en double), cc = N + D (anchor
// - p) exact en i128 puis arrondi (erreur relative <= u = 2^-53). Chaque produit arrondi vaut w_i cc_i (1 + t),
// |t| <= gamma_2 ; la somme de trois termes ajoute gamma_2 : |s - v| <= gamma_4 / (1 - gamma_2) * S <= 5u S, ou S est
// la somme des |produits arrondis|. La borne calculee fl(S) * 2^-49 = 16u fl(S) > 5u S : si |s| la depasse, le signe
// est celui de v ; sinon repli exact (arithmetique large). filter faux (hors FE_TONEAREST) : repli exact direct.
// decided compte les signes tranches par le filtre.
int orient_center_filtered(const P3& p, const P3& q, const P3& r, const P3& anchor, const geom::Center& c, bool filter,
                           u64& decided) {
  if (filter) {
    const P3 a = geom::sub(q, p), b = geom::sub(r, p);
    const i128 w[3] = {i128(a.y) * b.z - i128(a.z) * b.y, i128(a.z) * b.x - i128(a.x) * b.z,
                       i128(a.x) * b.y - i128(a.y) * b.x};
    const i128 cc[3] = {c.N[0] + c.D * (anchor.x - p.x), c.N[1] + c.D * (anchor.y - p.y),
                        c.N[2] + c.D * (anchor.z - p.z)};
    double sum = 0, mag = 0;
    for (int i = 0; i < 3; ++i) {
      const double t = static_cast<double>(w[i]) * static_cast<double>(cc[i]);
      sum += t;
      mag += std::fabs(t);
    }
    const double bound = mag * 0x1p-49;
    if (sum > bound || sum < -bound) {
      ++decided;
      return sum > bound ? 1 : -1;
    }
  }
  return geom::orient_center_wide(p, q, r, anchor, c);
}

// Certificat exact de MEB : la sphere passant par le support propose contient F (cles exactes <= 0) et son centre
// est dans l'enveloppe convexe fermee du support (triangle non obtus, tetraedre ferme). Par le lemme de la MEB
// (pour toute boule B(c', r') contenant R, r'^2 >= r^2 + |c - c'|^2 quand c est barycentre convexe de R sur la
// sphere), c'est l'unique MEB de F : la meme sphere que Welzl exact, representation mise a part.
// filter : filtres flottants permis (FE_TONEAREST dans le fil de la descente) ; sinon toutes les decisions sont
// exactes.
bool verify_meb(const Geo& g, const Facet& F, const DBall& B, Sphere& S, bool filter, ResolveCounters& cnt) {
  const int nr = B.nr;
  if (nr < 2) return false;
  const u32 r0 = F.s[B.R[0]];
  const P3& a = g.P[r0];
  S.empty = false;
  S.zero = false;
  S.anchor = r0;
  S.nr = static_cast<u8>(nr);
  for (int i = 0; i < nr; ++i) S.R[i] = F.s[B.R[i]];
  if (nr == 2) {
    geom::center2(a, g.P[F.s[B.R[1]]], S.c);
    S.strict = true;  // milieu du segment : interieur relatif
  } else if (nr == 3) {
    const P3 &b = g.P[F.s[B.R[1]]], &d = g.P[F.s[B.R[2]]];
    if (!geom::center3(a, b, d, S.c)) return false;
    const i64 da = geom::dot(geom::sub(b, a), geom::sub(d, a)), db = geom::dot(geom::sub(a, b), geom::sub(d, b)),
              dd = geom::dot(geom::sub(a, d), geom::sub(b, d));
    if (da < 0 || db < 0 || dd < 0) return false;
    S.strict = da > 0 && db > 0 && dd > 0;  // triangle aigu : centre strictement interieur
  } else {
    const P3* t[4] = {&a, &g.P[F.s[B.R[1]]], &g.P[F.s[B.R[2]]], &g.P[F.s[B.R[3]]]};
    if (!geom::center4(*t[0], *t[1], *t[2], *t[3], S.c)) return false;
    S.strict = true;
    for (int f = 0; f < 4; ++f) {
      const int so = geom::orient(*t[(f + 1) % 4], *t[(f + 2) % 4], *t[(f + 3) % 4], *t[f]);
      const int sc =
          orient_center_filtered(*t[(f + 1) % 4], *t[(f + 2) % 4], *t[(f + 3) % 4], a, S.c, filter, cnt.fp_orient);
      if (sc != 0 && sc != so) return false;
      if (sc == 0) S.strict = false;
    }
  }
  // F dans la boule fermee. Le support y est par construction (sphere passant par R). Pour les autres sites, filtre
  // flottant a marge : la distance carree approchee au centre approche a une erreur absolue < kApproxMargin / 2
  // (meme borne que l'elagage de SiteTree) ; d < r2a - kApproxMargin prouve l'interieur strict, sinon cle exacte.
  // Filtre coupe : cle exacte pour chaque site.
  set_approx(S, a);
  const double inner = S.r2a - kApproxMargin;
  u64 decided = 0;
  bool inside = true;
  for (u32 i = 0; i < F.n && inside; ++i) {
    bool support = false;
    for (int j = 0; j < nr; ++j) support |= static_cast<int>(i) == B.R[j];
    if (support) continue;
    const P3& z = g.P[F.s[i]];
    if (filter && approx_d2(S, z) < inner) {
      ++decided;
      continue;
    }
    inside = geom::side_key(S.c, a, z) <= 0;
  }
  cnt.fp_meb += decided;
  if (!inside) return false;
  S.has_level = false;  // niveau exact a la demande (level_of)
  return true;
}

// Niveau exact d'une sphere : calcule depuis son support certifie si besoin (memes formules que through()).
const geom::Level& level_of(const Geo& g, Sphere& S) {
  if (!S.has_level) {
    const P3& a = g.P[S.R[0]];
    if (S.nr == 2) S.level = geom::level2(a, g.P[S.R[1]]);
    else if (S.nr == 3) S.level = geom::level3(a, g.P[S.R[1]], g.P[S.R[2]]);
    else S.level = geom::level4(S.c);
    S.has_level = true;
  }
  return S.level;
}

// La proposition flottante (DWelzl) reste active hors FE_TONEAREST : elle ne decide rien, verify_meb certifie ou
// rejette en exact ; seule la representation (ancre, support) peut changer, pas la sphere ni les sorties.
Sphere meb(const Geo& g, const Facet& F, bool filter, u64& fallbacks, ResolveCounters& cnt) {
  if (F.n >= 2) {
    DWelzl w;
    for (u32 i = 0; i < F.n; ++i) {
      const P3& p = g.P[F.s[i]];
      w.p[i][0] = double(p.x);
      w.p[i][1] = double(p.y);
      w.p[i][2] = double(p.z);
    }
    const DBall B = w.run(static_cast<int>(F.n));
    Sphere S;
    if (w.ok && verify_meb(g, F, B, S, filter, cnt)) return S;
  }
  ++fallbacks;
  u32 R[4];
  Sphere S = welzl(g, F.s.data(), static_cast<int>(F.n), R, 0);
  S.has_level = true;  // through() calcule le niveau exact
  set_approx(S, g.P[S.anchor]);
  return S;
}

// ----------------------------------------------------------------- structure locale

enum class Local : u8 { inert = 0, birth = 1, join = 2, refused = 3 };

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
  if (m > kMaxShellEnum) return Local::refused;
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

u64 binomial(u32 m, u32 t) {
  if (t > m) return 0;
  u64 r = 1;
  for (u32 i = 1; i <= t; ++i) r = r * (m - t + i) / i;  // exact : C(m-t+i, i) entier a chaque pas
  return r;
}

// Premier representant de la structure locale, seul utile a la descente : c'est celui du morceau qui contient la
// premiere t-partie separable (ordre lexicographique), puisque les racines des morceaux sont leurs plus petits
// indices. Memes issues que local_structure (birth, refused), sans calculer les morceaux ; le decompte complet
// n'est fait que si le refus « plus de kMaxSubsets parties separables » est possible (C(m, t) > kMaxSubsets).
Local first_rep(const Geo& g, std::span<const u32> I, std::span<const u32> U, u32 anchor, const geom::Center& c,
                u32 t, bool regular, Facet& rep) {
  const u32 m = static_cast<u32>(U.size());
  if (regular) {  // I et U tries : fusion
    if (t == m) return Local::birth;
    if (t + 1 == m) merge_into(rep, I.data(), static_cast<u32>(I.size()), U.data() + 1, m - 1);
    else merge_into(rep, I.data(), static_cast<u32>(I.size()), U.data(), t);
    return Local::inert;
  }
  rep.n = 0;
  for (u32 s : I) rep.s[rep.n++] = s;
  if (m > kMaxShellEnum) return Local::refused;
  const bool may_refuse = binomial(m, t) > kMaxSubsets;
  u32 idx[kMaxShellEnum];
  u32 A[kMaxShellEnum];
  for (u32 i = 0; i < t; ++i) idx[i] = i;
  const P3& a = g.P[anchor];
  u64 count = 0;
  u32 first[kMaxShellEnum];
  for (;;) {
    for (u32 i = 0; i < t; ++i) A[i] = U[idx[i]];
    if (!center_in_closed_hull(g, std::span<const u32>(A, t), a, c)) {
      if (count == 0)
        for (u32 i = 0; i < t; ++i) first[i] = A[i];
      ++count;
      if (!may_refuse) break;
      if (count > kMaxSubsets) return Local::refused;
    }
    int pos = static_cast<int>(t) - 1;
    while (pos >= 0 && idx[pos] == m - t + static_cast<u32>(pos)) --pos;
    if (pos < 0) break;
    ++idx[pos];
    for (u32 i = pos + 1; i < t; ++i) idx[i] = idx[i - 1] + 1;
  }
  if (count == 0) return Local::birth;
  for (u32 i = 0; i < t; ++i) rep.s[rep.n++] = first[i];
  rep.sort();
  return Local::inert;
}

// ----------------------------------------------------------------- atlas des cellules (boule, K)

// Cellule (b, K) pour K dans la fenetre [p + q_min - 1, min(p + m, K derniere)] de la boule, restreinte aux ordres
// construits. val : noeud de naissance pour une cellule de naissance, memo de descente sinon (kNone = inconnu).
// Donnees d'une boule groupees (une ligne de cache par consultation) : fenetre construite, premiere cellule, et
// (p, q_min, m) du catalogue pour la garde de la descente (satures ; au-dela, comparaison au catalogue).
struct BallInfo {
  u32 base = kNone;  // premiere cellule (kNone : aucune)
  u16 m = 0;         // |U| en positions (sature a 0xFFFF)
  u8 lo = 1, hi = 0; // fenetre [lo, hi] restreinte aux ordres construits
  u8 p = 0;          // |I| en positions (sature a 0xFF)
  u8 q = 0;          // q_min
  u8 ext = 0;        // coquille etendue
  u8 pad = 0;
};

struct Atlas {
  u32 kfirst = 1, klast = 0;
  std::vector<BallInfo> info;
  Buffer<u8> kind;  // par cellule (lu seulement pour les coquilles etendues ; les autres sont analytiques)
  Buffer<u32> val;  // acces concurrents par std::atomic_ref
  u64 cells = 0;
};

// Structure locale d'une cellule a coquille etendue (rare) : calculee une fois, completement.
struct ExtCell {
  u32 ball = 0;
  u8 k = 0;
  Local kind = Local::inert;
  u32 rep_first = 0, nreps = 0;
};

// Pointeurs de saut de Myers (1983, forme « skew-binary ») : un pointeur et une profondeur par noeud. Les rangs
// croissent (au sens large) vers la racine, donc « rang <= r » est vrai sur un prefixe du chemin vers la racine ;
// l'ancetre le plus haut de ce prefixe est atteint en O(log profondeur) sauts. Le resultat est l'unique noeud que
// rend la remontee parent par parent.
struct Jumps {
  std::vector<u32> jump, depth;
  void build(const OrderForest& f) {
    const u32 nn = static_cast<u32>(f.rank.size());
    jump.resize(nn);
    depth.resize(nn);
    for (u32 v = nn; v-- > 0;) {  // parents apres leurs enfants : ordre decroissant
      const u32 p = f.parent[v];
      if (p == kNone) {
        jump[v] = v;
        depth[v] = 0;
        continue;
      }
      depth[v] = depth[p] + 1;
      const u32 jp = jump[p];
      jump[v] = (depth[p] - depth[jp] == depth[jp] - depth[jump[jp]]) ? jump[jp] : p;
    }
  }
};

// ----------------------------------------------------------------- un ordre

struct OrderRun {
  int k = 0;
  u32 site_births = 0;          // K = 1 : les sites sont les premieres naissances (rang 0)
  Buffer<u32> birth_ball;  // naissances du catalogue, ordre (rang, boule) = ordre des boules
  Buffer<u32> join_ball;   // jonctions, ordre (rang, boule) = ordre des boules
  Buffer<u32> join_ext;    // indice ExtCell (coquille etendue) ou kNone (representants analytiques)
  Buffer<u32> join_off;    // CSR des representants
  Buffer<u32> rep_node;    // naissance atteinte par la descente de chaque representant

  FlatIndex seeds;              // semis H_K : population triee d'une naissance reguliere de k sites -> naissance
  Buffer<u32> spop;        // population triee de chaque naissance semee (k sites, indice = naissance)
  Jumps jumps;                  // pointeurs de saut de la foret construite
  std::atomic<u32> error{0};    // Reason + 1 (resolution des jonctions, attaches)
  std::atomic<u32> verror{0};   // Reason + 1 (verticale K -> K - 1)
  Outcome outcome;
  u32 nbirths() const { return site_births + static_cast<u32>(birth_ball.size()); }
};

struct Scratch {
  std::vector<std::pair<i128, u32>> near;
  std::vector<std::pair<i128, u32>> knn;  // attaches : k plus proches d'un site
  std::vector<std::pair<double, u32>> nearD;
  std::vector<u32> I, U;
  std::vector<u32> pend;
  std::vector<u32> buf;
  std::vector<std::pair<u32, u32>> pairs;
  u64 meb_fallbacks = 0;
  u64 walk[kOrders] = {};
  ResolveCounters c[3][kOrders];  // 0 jonctions, 1 attaches, 2 verticales
};

bool same_population(const Catalogue& cat, u32 b, const Facet& F) {
  const auto I = cat.interior(b);
  const auto U = cat.shell(b);
  if (I.size() + U.size() != F.n) return false;
  size_t i = 0, j = 0;
  for (u32 x = 0; x < F.n; ++x) {
    const u32 v = (j >= U.size() || (i < I.size() && I[i] < U[j])) ? I[i++] : U[j++];
    if (v != F.s[x]) return false;
  }
  return true;
}

void population(const Catalogue& cat, u32 b, Facet& F) {  // I et U tries (garde de l'etage L) : fusion
  const auto I = cat.interior(b);
  const auto U = cat.shell(b);
  merge_into(F, I.data(), static_cast<u32>(I.size()), U.data(), static_cast<u32>(U.size()));
}

struct Ctx {
  const Geo& g;
  Atlas& atlas;
  std::vector<std::unique_ptr<OrderRun>>& runs;  // runs[k]
};

// Descente d'une k-partie vers un noeud de naissance.
u32 resolve(const Ctx& X, OrderRun& o, std::atomic<u32>& error, Facet F, Scratch& sc, ResolveCounters& cnt,
            u64 h0 = 0, bool have_h0 = false) {
  const Geo& g = X.g;
  const Catalogue& cat = g.cat;
  const u32 k = static_cast<u32>(o.k);
  auto set_error = [&](Reason r) {
    u32 expected = 0;
    error.compare_exchange_strong(expected, static_cast<u32>(r) + 1);
  };
  sc.pend.clear();
  ++cnt.resolves;
  Sphere prev;
  bool has_prev = false;
  u32 node = kNone;
  for (;;) {
    ++cnt.steps;
    if (k == 1) {
      node = F.s[0];
      break;
    }
    {
      const u64 h = have_h0 ? h0 : hash_sites(F.s.data(), F.n);
      have_h0 = false;
      const u32 seed = o.seeds.find(h, [&](u32 i) {
        const u32* q = o.spop.data() + u64(i) * k;
        for (u32 x = 0; x < k; ++x)
          if (q[x] != F.s[x]) return false;
        return true;
      });
      if (seed != kNone) {  // semis : sommet de naissance
        ++cnt.seed_hits;
        node = seed;
        break;
      }
    }
    // Filtres flottants de ce pas (certification de MEB, garde I3, saut K-NN) : leurs preuves supposent l'arrondi au
    // plus proche. Mode lu a chaque pas qui calcule une MEB (seuls ces pas filtrent), dans le fil qui execute la
    // descente ; hors FE_TONEAREST, chaque decision passe par la voie exacte, memes sorties (doctrine v4). Les requetes
    // de l'arbre des sites lisent leur propre mode (SiteTree::filtered).
    const bool filters = std::fegetround() == FE_TONEAREST;
    if (!filters) ++cnt.fp_cut;
    Sphere S = meb(g, F, filters, sc.meb_fallbacks, cnt);
    ++cnt.meb;
    // I3 : le niveau decroit strictement a chaque pas. Filtre : |r2a - r^2| < 1e-3 pour chaque sphere, donc
    // r2a < r2a_prec - kApproxMargin prouve la decroissance ; sinon (ou filtre coupe) comparaison exacte des niveaux.
    if (has_prev) {
      if (filters && S.r2a < prev.r2a - kApproxMargin) {
        ++cnt.fp_level;
      } else {
        ++cnt.level_exact;
        if (geom::compare(level_of(g, S), level_of(g, prev)) >= 0) {
          set_error(Reason::descent_no_terminal);
          return kNone;
        }
      }
    }
    prev = S;
    has_prev = true;
    const P3& a = g.P[S.anchor];
    // Recensement de la sphere (I, U). Si la MEB certifiee a son centre dans l'interieur relatif de son support R
    // et qu'une boule du catalogue a R pour support canonique, c'est cette sphere (R et l'arite determinent la
    // sphere) : son recensement exact (theoreme C du catalogue) est I u U, sans requete. Sinon, boule fermee par
    // l'arbre. Juge d'echantillon deterministe : pour une boule sur 32 (selon son indice), les deux recensements
    // sont compares.
    // Recensement de la sphere : (p, q, m) d'abord, les sites de I et U seulement s'ils servent (saut, representant,
    // juge). Si la MEB certifiee a son centre dans l'interieur relatif de son support R et qu'une boule du catalogue a
    // R pour support canonique, c'est cette sphere (R et l'arite determinent la sphere) : son recensement exact
    // (theoreme C du catalogue) est celui de la boule, lu dans BallInfo, sans requete. Sinon, boule fermee par
    // l'arbre. Juge d'echantillon deterministe : pour une boule sur 32 (selon son indice), les deux recensements
    // sont compares.
    std::span<const u32> I, U;
    u32 b = kNone;
    u32 p = 0, m = 0;
    u8 q = 0;
    bool from_cat = false;
    const BallInfo* bi = nullptr;
    if (S.strict) {
      std::array<u32, 4> r = {kNone, kNone, kNone, kNone};
      for (u32 i = 0; i < S.nr; ++i) {  // insertion (evite le faux positif -Warray-bounds de std::sort sur un prefixe)
        u32 j = i;
        while (j > 0 && r[j - 1] > S.R[i]) {
          r[j] = r[j - 1];
          --j;
        }
        r[j] = S.R[i];
      }
      b = g.find_ball(r, X.atlas.info.data());
      ++cnt.lookups;
      if (b != kNone) {
        from_cat = true;
        ++cnt.census_cat;
        bi = &X.atlas.info[b];
        p = bi->p == 0xFF ? cat.n_interior[b] : bi->p;
        m = bi->m == 0xFFFF ? static_cast<u32>(cat.shell(b).size()) : bi->m;
        q = bi->q;
        if ((mix64(b) & 31) == 0) {
          const auto Ic = cat.interior(b);
          const auto Uc = cat.shell(b);
          g.tree.closed_ball(a, S.c, sc.I, sc.U);
          ++cnt.closed_balls;
          if (Ic.size() != p || Uc.size() != m || !std::equal(Ic.begin(), Ic.end(), sc.I.begin(), sc.I.end()) ||
              !std::equal(Uc.begin(), Uc.end(), sc.U.begin(), sc.U.end())) {
            set_error(Reason::census_mismatch);
            return kNone;
          }
        }
      }
    }
    if (!from_cat) {
      g.tree.closed_ball(a, S.c, sc.I, sc.U);
      ++cnt.closed_balls;
      I = sc.I;
      U = sc.U;
      p = static_cast<u32>(I.size());
      m = static_cast<u32>(U.size());
    }
    // Saut K-NN ssi au moins k sites strictement interieurs (cle < 0) ; ses k plus proches (cle exacte, indice)
    // sont alors des sites interieurs.
    if (p >= k) {
      ++cnt.knn_jumps;
      if (from_cat) I = cat.interior(b);
      F.n = k;
      if (p == k) {  // exactement k interieurs : ce sont les k plus proches (I trie par indice)
        for (u32 i = 0; i < k; ++i) F.s[i] = I[i];
        continue;
      }
      // Selection en double : si l'ecart entre la k-ieme et la (k+1)-ieme distance approchee depasse kApproxMargin
      // (erreurs < 1e-3), les k plus proches approches sont les k plus proches exacts ; sinon (ou filtre coupe)
      // selection exacte (cle, indice).
      bool selected = false;
      if (filters) {
        sc.nearD.clear();
        for (u32 z : I) sc.nearD.push_back({approx_d2(S, g.P[z]), z});
        std::nth_element(sc.nearD.begin(), sc.nearD.begin() + k, sc.nearD.end());
        double dk = sc.nearD[0].first;
        for (u32 i = 1; i < k; ++i) dk = std::max(dk, sc.nearD[i].first);
        if (sc.nearD[k].first - dk > kApproxMargin) {
          ++cnt.fp_jump;
          for (u32 i = 0; i < k; ++i) F.s[i] = sc.nearD[i].second;
          selected = true;
        }
      }
      if (!selected) {
        ++cnt.jump_exact;
        sc.near.clear();
        for (u32 z : I) sc.near.push_back({geom::side_key(S.c, a, g.P[z]), z});
        std::partial_sort(sc.near.begin(), sc.near.begin() + k, sc.near.end());
        for (u32 i = 0; i < k; ++i) F.s[i] = sc.near[i].second;
      }
      F.sort();
      continue;
    }
    if (!from_cat) {
      std::array<u32, 4> sup = {kNone, kNone, kNone, kNone};
      if (S.strict && S.nr == m) {
        // la coquille est exactement le support certifie de la MEB, centre dans son interieur relatif : aucune
        // paire antipodale ni face contenant le centre (ils le placeraient au bord), donc q_min = |U| et S* = U
        // trie, ce que rend canonical_support
        q = static_cast<u8>(m);
        for (u32 i = 0; i < m; ++i) sup[i] = U[i];
      } else if (!canonical_support(g, U, a, S.c, sup, q)) {
        set_error(Reason::descent_no_terminal);
        return kNone;
      }
      b = g.find_ball(sup, X.atlas.info.data());
      ++cnt.lookups;
      if (b != kNone) bi = &X.atlas.info[b];
    }
    if (b != kNone && k + 1 >= p + q && k <= p + m) {
      // la boule du catalogue de ce support est cette sphere : memes interieur, q_min et coquille (garde)
      const bool same = (bi->p == 0xFF ? cat.n_interior[b] == p : bi->p == p) && bi->q == q &&
                        (bi->m == 0xFFFF ? cat.shell(b).size() == m : bi->m == m);
      if (!same || k < bi->lo || k > bi->hi) {
        set_error(Reason::census_mismatch);
        return kNone;
      }
      const u32 c = bi->base + (k - bi->lo);
      const bool birth_cell = bi->ext ? X.atlas.kind[c] == static_cast<u8>(Local::birth) : k == p + m;
      if (birth_cell) {
        ++cnt.birth_hits;
        node = std::atomic_ref<u32>(X.atlas.val[c]).load(std::memory_order_relaxed);
        break;
      }
      const u32 memo = std::atomic_ref<u32>(X.atlas.val[c]).load(std::memory_order_relaxed);
      if (memo != kNone) {
        ++cnt.memo_hits;
        node = memo;
        break;
      }
      sc.pend.push_back(c);
    }
    if (from_cat) {
      I = cat.interior(b);
      U = cat.shell(b);
    }
    Facet rep;
    const Local L = first_rep(g, I, U, S.anchor, S.c, k - p, m == q, rep);
    ++cnt.local_calls;
    if (L == Local::refused) {
      set_error(Reason::shell_quotient_budget);
      return kNone;
    }
    if (L == Local::birth) {  // une naissance absente de la table : catalogue incomplet
      set_error(Reason::census_mismatch);
      return kNone;
    }
    F = rep;
  }
  for (u32 c : sc.pend) std::atomic_ref<u32>(X.atlas.val[c]).store(node, std::memory_order_relaxed);
  return node;
}

// niveau <= e (entier) : num <= e * den, exact (num, den >= 0 ; e * den < 2^45 * 2^124 tient sur trois mots).
inline bool level_at_most(const geom::Level& L, u64 e) {
  arith::I192 ed;
  if (!arith::resize(arith::mul(arith::Wide<1>::from_u128(e), L.den), ed)) return true;  // e * den >= 2^192 > num
  return arith::cmp(L.num, ed) <= 0;
}

// Plus grand rang r (decale : 0 = niveau nul) tel que niveau(r) <= e : nombre de niveaux <= e. Dichotomie a deux
// niveaux, exacte : d'abord sur un niveau sur 64 (copie compacte, reste en cache), puis dans le bloc de 64.
struct RankIndex {
  static constexpr u32 kStep = rank_search::kStep;
  std::vector<geom::Level> sample;  // sample[i] = level[i * kStep]
  void build(const Catalogue& cat) {
    sample.clear();
    for (u64 i = 0; i < cat.level.size(); i += kStep) sample.push_back(cat.level[i]);
  }
  u32 at_most(const Catalogue& cat, u64 e) const {
    if (e == 0) return 0;
    return rank_search::at_most(static_cast<u32>(cat.level.size()),
                               [&](u32 i) { return level_at_most(sample[i], e); },
                               [&](u32 i) { return level_at_most(cat.level[i], e); });
  }
};

// Representant r de la jonction j de l'ordre o.
void join_rep(const Catalogue& cat, const OrderRun& o, const std::vector<Facet>& ext_reps,
              const std::vector<ExtCell>& ext, u32 j, u32 r, Facet& F) {
  const u32 e = o.join_ext[j];
  if (e != kNone) {
    F = ext_reps[ext[e].rep_first + r];
    return;
  }
  // coquille reguliere : I u U \ {U[r]} (I et U tries par le catalogue : fusion)
  const u32 b = o.join_ball[j];
  const auto I = cat.interior(b);
  const auto U = cat.shell(b);
  u32 A[kMaxFacet];
  u32 na = 0;
  for (u32 i = 0; i < U.size(); ++i)
    if (i != r) A[na++] = U[i];
  merge_into(F, I.data(), static_cast<u32>(I.size()), A, na);
}

// Kruskal par plateaux d'un ordre : les lots sont les jonctions de meme rang ; chaque representant est rattache a sa
// racine pre-lot ; les racines d'un meme groupe apres unions forment une multifusion. Racine d'une composante =
// plus petit indice de naissance (union par minimum) : numerotation independante de l'ordre des unions.
Outcome kruskal(const Catalogue& cat, OrderRun& o, OrderForest& out, Scratch& sc) {
  const u32 k = static_cast<u32>(o.k);
  const u32 nb = o.nbirths();
  const u32 nj = static_cast<u32>(o.join_ball.size());
  out.rank.clear();
  out.birth.clear();
  out.rank.reserve(nb + nj);
  out.birth.reserve(nb + nj);
  for (u32 i = 0; i < o.site_births; ++i) {
    out.rank.push_back(0);
    out.birth.push_back(i);
  }
  for (u32 b : o.birth_ball) {
    out.rank.push_back(cat.rank[b] + 1);
    out.birth.push_back(b);
  }
  out.parent.assign(nb, kNone);
  out.parent.reserve(nb + nj);
  out.child_off.assign(nb + 1, 0);
  out.child_off.reserve(nb + nj + 1);
  out.child_val.clear();
  std::vector<u32> dsu(nb), top(nb);
  for (u32 i = 0; i < nb; ++i) dsu[i] = top[i] = i;
  auto find = [&](u32 x) {
    while (dsu[x] != x) x = dsu[x] = dsu[dsu[x]];
    return x;
  };
  std::vector<u32>& pre = sc.buf;
  auto& members = sc.pairs;
  std::vector<u32> kids;
  for (u32 i = 0; i < nj;) {
    const u32 rk = cat.rank[o.join_ball[i]] + 1;
    u32 j = i;
    while (j < nj && cat.rank[o.join_ball[j]] + 1 == rk) ++j;
    // racines pre-lot de chaque representant, puis unions du lot
    pre.clear();
    for (u32 r = o.join_off[i]; r < o.join_off[j]; ++r) pre.push_back(find(o.rep_node[r]));
    for (u32 t = i; t < j; ++t) {
      const u32 r0 = o.join_off[t] - o.join_off[i];
      for (u32 r = r0 + 1; r < o.join_off[t + 1] - o.join_off[i]; ++r) {
        const u32 x = find(pre[r0]), y = find(pre[r]);
        if (x != y) dsu[std::max(x, y)] = std::min(x, y);
      }
    }
    members.clear();
    for (u32 r : pre) members.push_back({find(r), r});
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
        kids.clear();
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
  return Outcome{};
}

// Ancetre vivant au rang r (coupe fermee) d'un noeud de la foret f.
inline u32 ancestor(const OrderForest& f, const Jumps& J, u32 v, u32 r, u64& steps) {
  while (f.parent[v] != kNone && f.rank[f.parent[v]] <= r) {
    const u32 j = J.jump[v];
    v = f.rank[j] <= r ? j : f.parent[v];
    ++steps;
  }
  return v;
}

// Parcours a plat des items (ordre, i) de plusieurs ordres : order_of(chunk) par prefixe.
struct Flat {
  std::vector<u32> ks;
  std::vector<u64> off;  // off[i] = debut des items de ks[i]
  u64 total() const { return off.empty() ? 0 : off.back(); }
  void add(u32 k, u64 n) {
    if (off.empty()) off.push_back(0);
    ks.push_back(k);
    off.push_back(off.back() + n);
  }
  // appelle f(k, i_local) pour chaque item de [b, e)
  template <class Fn>
  void each(u64 b, u64 e, Fn f) const {
    size_t idx = std::upper_bound(off.begin(), off.end(), b) - off.begin() - 1;
    for (u64 x = b; x < e; ++x) {
      while (x >= off[idx + 1]) ++idx;
      if (!f(ks[idx], x - off[idx])) return;
    }
  }
};

}  // namespace

Result<Tower> build_tower(const Cloud& cloud, const SiteTree& tree, const Catalogue& cat, const TowerParams& params,
                          sched::Pool& pool) {
  if (params.kmax < 1 || params.kmax > cat.kmax) return fail(Reason::kmax_out_of_range);
  // Les filtres flottants (marge kApproxMargin, SiteTree) sont prouves pour des coordonnees u18 ; le catalogue refuse
  // deja un nuage plus large, la tour le refuse aussi (garde, meme raison).
  if (cloud.bits > kCoordinateBits) return fail(Reason::parameter_out_of_range);
  for (u32 s = 0; s < cloud.sites(); ++s)
    if (cloud.w[s] != 1) return fail(Reason::multiplicity_unsupported);  // multiplicites : semantique a venir
  Tower t;
  TowerStats& ts = t.stats;
  auto tstage = Clock::now();
  const u32 n = cloud.sites();
  const u32 nballs = cat.balls();
  Geo g{cloud, tree, cat, {}, {}};
  g.P.resize(n);
  for (u32 s = 0; s < n; ++s) g.P[s] = P3{i64(cloud.x[s]), i64(cloud.y[s]), i64(cloud.z[s])};
  if (!g.lookup.init(nballs)) return fail(Reason::memory_budget);
  pool.parallel_for((g.lookup.capacity() + 65535) / 65536, 1, [&](u64 b0, u64 e0, unsigned) {
    for (u64 blk = b0; blk < e0; ++blk) g.lookup.zero(blk * 65536, std::min(g.lookup.capacity(), (blk + 1) * 65536));
  });
  pool.parallel_for(nballs, 4096, [&](u64 b0, u64 e0, unsigned) {
    for (u64 b = b0; b < e0; ++b) {
      const auto& sup = cat.support[b];
      g.lookup.insert(hash_sites(sup.data(), 4), static_cast<u32>(b), [&](u32 o) { return cat.support[o] == sup; });
    }
  });
  t.kmax = std::min<int>(params.kmax, static_cast<int>(n));
  t.orders.resize(t.kmax);
  const u32 keff = static_cast<u32>(t.kmax);
  // ordres construits : [kfirst, klast]
  Atlas atlas;
  if (params.only_order > 0) {
    atlas.kfirst = static_cast<u32>(params.only_order);
    atlas.klast = static_cast<u32>(params.only_order) <= keff ? static_cast<u32>(params.only_order) : 0;
  } else {
    atlas.kfirst = 1;
    atlas.klast = keff;
  }
  std::vector<std::unique_ptr<OrderRun>> runs(kOrders);
  for (u32 k = atlas.kfirst; k <= atlas.klast; ++k) {
    runs[k] = std::make_unique<OrderRun>();
    runs[k]->k = static_cast<int>(k);
    runs[k]->site_births = k == 1 ? n : 0;
  }
  ts.t_prepare = seconds_since(tstage);
  tstage = Clock::now();

  // ---- etage L : atlas, naissances, jonctions (une passe parallele par morceaux fixes de boules)
  constexpr u64 kChunk = 1u << 13;
  const u64 nchunks = (u64(nballs) + kChunk - 1) / kChunk;
  // compteurs par morceau et par ordre : cellules, naissances, jonctions, representants (hors coquilles etendues)
  std::vector<u64> ch_cells(nchunks + 1, 0);
  std::vector<std::array<u32, kOrders>> ch_births(nchunks + 1), ch_joins(nchunks + 1), ch_reps(nchunks + 1),
      ch_cellk(nchunks);
  std::vector<std::vector<u32>> ch_ext(nchunks);  // boules a coquille etendue de chaque morceau
  atlas.info.resize(nballs);
  std::atomic<u32> rank_ok{1}, pop_sorted{1};
  pool.parallel_for(nballs, kChunk, [&](u64 b0, u64 e0, unsigned) {
    const u64 ch = b0 / kChunk;
    auto& nbirth = ch_births[ch];
    auto& njoin = ch_joins[ch];
    auto& nrep = ch_reps[ch];
    auto& ncellk = ch_cellk[ch];
    nbirth.fill(0);
    njoin.fill(0);
    nrep.fill(0);
    ncellk.fill(0);
    u64 cells = 0;
    for (u64 b = b0; b < e0; ++b) {
      if (b > 0 && cat.rank[b] < cat.rank[b - 1]) rank_ok.store(0, std::memory_order_relaxed);
      {  // I et U strictement croissants (les representants sont construits par fusion)
        const auto I = cat.interior(static_cast<u32>(b));
        const auto U = cat.shell(static_cast<u32>(b));
        for (size_t i = 1; i < I.size(); ++i)
          if (I[i - 1] >= I[i]) pop_sorted.store(0, std::memory_order_relaxed);
        for (size_t i = 1; i < U.size(); ++i)
          if (U[i - 1] >= U[i]) pop_sorted.store(0, std::memory_order_relaxed);
      }
      const u32 p = cat.p[b];
      const u32 m = static_cast<u32>(cat.shell(static_cast<u32>(b)).size());
      const u32 lo = std::max(p + cat.qmin[b] - 1, atlas.kfirst);
      const u32 hi = std::min(p + m, atlas.klast);
      const bool any = lo <= hi;
      BallInfo& bi = atlas.info[b];
      bi.lo = static_cast<u8>(any ? lo : 1);
      bi.hi = static_cast<u8>(any ? hi : 0);
      bi.base = kNone;
      bi.m = static_cast<u16>(std::min<u32>(m, 0xFFFF));
      bi.p = static_cast<u8>(std::min<u32>(cat.n_interior[b], 0xFF));
      bi.q = cat.qmin[b];
      bi.ext = (cat.flags[b] & kExtendedShell) ? 1 : 0;
      if (!any) continue;
      cells += hi - lo + 1;
      for (u32 k = lo; k <= hi; ++k) ++ncellk[k];
      if (cat.flags[b] & kExtendedShell) {
        ch_ext[ch].push_back(static_cast<u32>(b));
        continue;
      }
      // coquille reguliere : jonction a t = m - 1, naissance a t = m
      if (p + m - 1 >= lo && p + m - 1 <= hi) {
        ++njoin[p + m - 1];
        nrep[p + m - 1] += m;
      }
      if (p + m >= lo && p + m <= hi) ++nbirth[p + m];
    }
    ch_cells[ch] = cells;
  });
  if (!rank_ok.load()) return fail(Reason::rank_order);
  if (!pop_sorted.load()) return fail(Reason::csr_bounds);
  // structures locales des coquilles etendues (completes, une fois par cellule)
  std::vector<u32> ext_balls;
  std::vector<u64> ch_ext_off(nchunks + 1, 0);
  for (u64 ch = 0; ch < nchunks; ++ch) {
    ch_ext_off[ch] = ext_balls.size();
    ext_balls.insert(ext_balls.end(), ch_ext[ch].begin(), ch_ext[ch].end());
  }
  ch_ext_off[nchunks] = ext_balls.size();
  std::vector<std::vector<std::pair<ExtCell, std::vector<Facet>>>> ext_local(ext_balls.size());
  pool.parallel_for(ext_balls.size(), 1, [&](u64 b0, u64 e0, unsigned) {
    std::vector<Facet> reps;
    for (u64 x = b0; x < e0; ++x) {
      const u32 b = ext_balls[x];
      const geom::Center c = ball_center(cloud, cat, b);
      for (u32 k = atlas.info[b].lo; k <= atlas.info[b].hi; ++k) {
        const Local L = local_structure(g, cat.interior(b), cat.shell(b), cat.support[b][0], c, k - cat.p[b], false, reps);
        ExtCell e;
        e.ball = b;
        e.k = static_cast<u8>(k);
        e.kind = L;
        ext_local[x].push_back({e, L == Local::join ? reps : std::vector<Facet>{}});
      }
    }
  });
  std::vector<ExtCell> ext;
  std::vector<Facet> ext_reps;
  std::vector<u32> ext_first(ext_balls.size() + 1, 0);  // premiere ExtCell de chaque boule etendue
  for (size_t x = 0; x < ext_balls.size(); ++x) {
    ext_first[x] = static_cast<u32>(ext.size());
    for (auto& [e, reps] : ext_local[x]) {
      e.rep_first = static_cast<u32>(ext_reps.size());
      e.nreps = static_cast<u32>(reps.size());
      ext_reps.insert(ext_reps.end(), reps.begin(), reps.end());
      ext.push_back(e);
    }
  }
  ext_first[ext_balls.size()] = static_cast<u32>(ext.size());
  ext_local.clear();
  // refus d'une structure locale : l'ordre echoue (premier etage)
  for (const ExtCell& e : ext)
    if (e.kind == Local::refused && runs[e.k]->outcome.ok()) runs[e.k]->outcome = fail(Reason::shell_quotient_budget, e.k);
  // contributions des coquilles etendues aux compteurs de leur morceau
  for (u64 ch = 0; ch < nchunks; ++ch)
    for (u64 x = ch_ext_off[ch]; x < ch_ext_off[ch + 1]; ++x)
      for (u32 i = ext_first[x]; i < ext_first[x + 1]; ++i) {
        const ExtCell& e = ext[i];
        if (e.kind == Local::birth) ++ch_births[ch][e.k];
        else if (e.kind == Local::join) {
          ++ch_joins[ch][e.k];
          ch_reps[ch][e.k] += e.nreps;
        }
      }
  // prefixes par morceau (sommes en u64 ; tout index publie en u32, sinon refus index_overflow_u32)
  {
    u64 cells = 0;
    std::array<u64, kOrders> sb{}, sj{}, sr{};
    for (u64 ch = 0; ch <= nchunks; ++ch) {
      const u64 c = ch < nchunks ? ch_cells[ch] : 0;
      const auto nbirth = ch < nchunks ? ch_births[ch] : std::array<u32, kOrders>{};
      const auto njoin = ch < nchunks ? ch_joins[ch] : std::array<u32, kOrders>{};
      const auto nrep = ch < nchunks ? ch_reps[ch] : std::array<u32, kOrders>{};
      if (cells >= kNone) return fail(Reason::index_overflow_u32);
      ch_cells[ch] = cells;
      for (u32 k = 0; k < kOrders; ++k) {
        if (sb[k] >= kNone || sj[k] >= kNone || sr[k] >= kNone) return fail(Reason::index_overflow_u32);
        ch_births[ch][k] = static_cast<u32>(sb[k]);
        ch_joins[ch][k] = static_cast<u32>(sj[k]);
        ch_reps[ch][k] = static_cast<u32>(sr[k]);
      }
      cells += c;
      for (u32 k = 0; k < kOrders; ++k) {
        sb[k] += nbirth[k];
        sj[k] += njoin[k];
        sr[k] += nrep[k];
      }
    }
    if (cells >= kNone) return fail(Reason::index_overflow_u32);
    atlas.cells = cells;
  }
  if (!atlas.kind.allocate(atlas.cells) || !atlas.val.allocate(atlas.cells)) return fail(Reason::memory_budget);
  // par boule reguliere : noeud du representant I u U \ {U[m-1]} de sa jonction (verticale de sa naissance)
  Buffer<u32> ball_vnode;
  if (!ball_vnode.allocate(nballs)) return fail(Reason::memory_budget);
  for (u32 k = atlas.kfirst; k <= atlas.klast; ++k) {
    OrderRun& o = *runs[k];
    const u64 nb = ch_births[nchunks][k], nj = ch_joins[nchunks][k], nr = ch_reps[nchunks][k];
    if (!o.birth_ball.allocate(nb) || !o.join_ball.allocate(nj) || !o.join_ext.allocate(nj) ||
        !o.join_off.allocate(nj + 1) || !o.rep_node.allocate(nr))
      return fail(Reason::memory_budget);
    o.join_off[nj] = static_cast<u32>(nr);
  }
  pool.parallel_for(nballs, kChunk, [&](u64 b0, u64 e0, unsigned) {
    const u64 ch = b0 / kChunk;
    u64 cell = ch_cells[ch];
    auto nb = ch_births[ch];
    auto nj = ch_joins[ch];
    auto nr = ch_reps[ch];
    u64 x = ch_ext_off[ch];
    for (u64 b = b0; b < e0; ++b) {
      ball_vnode[b] = kNone;
      const u32 lo = atlas.info[b].lo, hi = atlas.info[b].hi;
      if (lo > hi) continue;
      atlas.info[b].base = static_cast<u32>(cell);
      const bool extended = cat.flags[b] & kExtendedShell;
      const u32 p = cat.p[b];
      const u32 m = static_cast<u32>(cat.shell(static_cast<u32>(b)).size());
      for (u32 k = lo; k <= hi; ++k, ++cell) {
        OrderRun& o = *runs[k];
        Local kind;
        u32 e = kNone;
        if (extended) {
          e = ext_first[x] + (k - lo);
          kind = ext[e].kind;
        } else {
          kind = (k == p + m) ? Local::birth : (k + 1 == p + m ? Local::join : Local::inert);
        }
        atlas.kind[cell] = static_cast<u8>(kind);
        u32 v = kNone;
        if (kind == Local::birth) {
          v = o.site_births + nb[k];
          o.birth_ball[nb[k]++] = static_cast<u32>(b);
        } else if (kind == Local::join) {
          const u32 j = nj[k]++;
          o.join_ball[j] = static_cast<u32>(b);
          o.join_ext[j] = e;
          o.join_off[j] = nr[k];
          nr[k] += extended ? ext[e].nreps : m;
        }
        atlas.val[cell] = v;
      }
      if (extended) ++x;
    }
  });
  for (u32 k = atlas.kfirst; k <= atlas.klast; ++k) {
    OrderForest& out = t.orders[k - 1];
    out.stats.local_cells = 0;
    for (u64 ch = 0; ch < nchunks; ++ch) out.stats.local_cells += ch_cellk[ch][k];
    out.joins = runs[k]->join_ball.size();
    out.births = runs[k]->nbirths();
  }
  ts.t_local = seconds_since(tstage);
  tstage = Clock::now();

  // ---- semis H_K : naissances regulieres dont la population a exactement k sites
  Flat births_flat;
  {
    Flat zero_flat;
    for (u32 k = std::max(2u, atlas.kfirst); k <= atlas.klast; ++k) {
      if (!runs[k]->seeds.init(runs[k]->birth_ball.size()) || !runs[k]->spop.allocate(runs[k]->birth_ball.size() * u64(k)))
        return fail(Reason::memory_budget);
      births_flat.add(k, runs[k]->birth_ball.size());
      zero_flat.add(k, (runs[k]->seeds.capacity() + 65535) / 65536);
    }
    pool.parallel_for(zero_flat.total(), 1, [&](u64 b0, u64 e0, unsigned) {
      zero_flat.each(b0, e0, [&](u32 k, u64 blk) {
        FlatIndex& t2 = runs[k]->seeds;
        t2.zero(blk * 65536, std::min(t2.capacity(), (blk + 1) * 65536));
        return true;
      });
    });
  }
  pool.parallel_for(births_flat.total(), 1024, [&](u64 b0, u64 e0, unsigned) {
    Facet F;
    births_flat.each(b0, e0, [&](u32 k, u64 i) {
      OrderRun& o = *runs[k];
      const u32 b = o.birth_ball[i];
      if (!(cat.flags[b] & kExtendedShell) && cat.pop_off[b + 1] - cat.pop_off[b] == u64(k)) {
        population(cat, b, F);
        std::copy(F.s.begin(), F.s.begin() + k, o.spop.data() + i * k);
        o.seeds.insert(hash_sites(F.s.data(), F.n), o.site_births + static_cast<u32>(i),
                       [&](u32 other) { return same_population(cat, o.birth_ball[other - o.site_births], F); });
      }
      return true;
    });
  });
  ts.t_seeds = seconds_since(tstage);
  tstage = Clock::now();

  Ctx X{g, atlas, runs};
  std::vector<Scratch> scratch(pool.size());

  // ---- etage G : resolution des representants de toutes les jonctions de tous les ordres
  {
    Flat joins_flat;
    for (u32 k = atlas.kfirst; k <= atlas.klast; ++k)
      if (runs[k]->outcome.ok()) joins_flat.add(k, runs[k]->join_ball.size());
    pool.parallel_for(joins_flat.total(), 32, [&](u64 b0, u64 e0, unsigned wk) {
      Scratch& sc = scratch[wk];
      // Lots de representants : facettes et hachages d'abord, emplacements des semis precharges, puis les
      // populations candidates, puis les descentes (les defauts de cache des lots se recouvrent). L'ordre de
      // traitement ne change aucun resultat : chaque descente est une fonction de (F, k).
      constexpr u32 kBatch = 32;
      Facet Fb[kBatch];
      u64 Hb[kBatch];
      u32 Rb[kBatch], Kb[kBatch];
      u32 nbat = 0;
      auto flush = [&]() {
        for (u32 i = 0; i < nbat; ++i) {
          const OrderRun& o = *runs[Kb[i]];
          if (const u64* sl = o.seeds.first_slot(Hb[i])) {
            const u32 v = FlatIndex::candidate(*sl, Hb[i]);
            if (v != kNone) __builtin_prefetch(o.spop.data() + u64(v) * Kb[i]);
          }
        }
        for (u32 i = 0; i < nbat; ++i) {
          OrderRun& o = *runs[Kb[i]];
          o.rep_node[Rb[i]] = resolve(X, o, o.error, Fb[i], sc, sc.c[0][Kb[i]], Hb[i], true);
        }
        nbat = 0;
      };
      joins_flat.each(b0, e0, [&](u32 k, u64 j) {
        OrderRun& o = *runs[k];
        for (u32 r = o.join_off[j]; r < o.join_off[j + 1]; ++r) {
          Facet& F = Fb[nbat];
          join_rep(cat, o, ext_reps, ext, static_cast<u32>(j), r - o.join_off[j], F);
          Hb[nbat] = hash_sites(F.s.data(), F.n);
          if (const u64* sl = o.seeds.first_slot(Hb[nbat])) __builtin_prefetch(sl);
          Rb[nbat] = r;
          Kb[nbat] = k;
          if (++nbat == kBatch) flush();
        }
        return true;
      });
      flush();
      joins_flat.each(b0, e0, [&](u32 k, u64 j) {
        OrderRun& o = *runs[k];
        if (o.join_ext[j] == kNone) ball_vnode[o.join_ball[j]] = o.rep_node[o.join_off[j + 1] - 1];
        return true;
      });
    });
    for (u32 k = atlas.kfirst; k <= atlas.klast; ++k) {
      OrderRun& o = *runs[k];
      if (o.outcome.ok() && o.error.load()) o.outcome = fail(static_cast<Reason>(o.error.load() - 1), u8(k));
    }
  }
  ts.t_resolve = seconds_since(tstage);
  tstage = Clock::now();

  // ---- Kruskal par plateaux : un ordre par tache, le plus grand d'abord
  {
    std::vector<u32> ks;
    for (u32 k = atlas.klast; k >= atlas.kfirst && k >= 1; --k)
      if (runs[k]->outcome.ok()) ks.push_back(k);
    pool.parallel_for(ks.size(), 1, [&](u64 b0, u64 e0, unsigned wk) {
      for (u64 i = b0; i < e0; ++i) {
        const u32 k = ks[i];
        const auto t0 = Clock::now();
        OrderForest& out = t.orders[k - 1];
        out.k = static_cast<int>(k);
        const Outcome oc = kruskal(cat, *runs[k], out, scratch[wk]);
        if (!oc.ok()) runs[k]->outcome = oc;
        else runs[k]->jumps.build(out);
        runs[k]->rep_node.reset();
        out.stats.t_kruskal = seconds_since(t0);
      }
    });
  }
  ts.t_kruskal = seconds_since(tstage);
  tstage = Clock::now();

  // ---- attaches C n X
  if (params.points) {
    RankIndex ranks;
    ranks.build(cat);
    std::vector<u32> pk;  // ordres a attacher
    for (u32 k = atlas.kfirst; k <= atlas.klast; ++k)
      if (runs[k]->outcome.ok()) {
        t.orders[k - 1].point_node.assign(n, kNone);
        t.orders[k - 1].point_level.assign(n, 0);
        pk.push_back(k);
      }
    // Entree cover (k >= 2) : la premiere boule du catalogue, dans l'ordre canonique donc par niveau croissant,
    // dont la boule fermee contient x et au moins k sites, a pour rayon alpha_k(x) ; toutes les k-parties d'une
    // boule fermee contiennent son centre dans leur region temoin, donc une seule resolution par boule donne la
    // composante qui couvre chacun de ses points au niveau de la boule.
    const bool cover = params.entry == PointEntry::cover;
    const u32 extra = cover ? u32(std::max(0, params.cover_extra)) : 0;
    if (cover && int(pk.empty() ? 0 : pk.back() + extra) > cat.kmax) return fail(Reason::kmax_out_of_range);
    std::vector<u32> pk_core;
    for (u32 k : pk)
      if (!cover || (k == 1 && extra == 0)) pk_core.push_back(k);
    if (cover) {
      for (u32 k : pk) {
        if (k == 1 && extra == 0) continue;
        OrderRun& o = *runs[k];
        OrderForest& out = t.orders[k - 1];
        out.point_cat_rank.assign(n, 0);
        // boule couvrante : poids >= k + extra (poids > positions : multiplicites, refusees en amont) ; son centre
        // est dans L_k a son niveau, et toute k-partie de la boule fermee le contient dans sa region temoin
        const u32 kc = k + extra;
        auto covering = [&](u64 b) { return cat.p[b] + cat.u[b] >= kc && cat.pop_off[b + 1] - cat.pop_off[b] >= kc; };
        // composante de L_k(niveau de b) qui contient le centre de b : une k-partie quelconque de la boule fermee
        auto cover_node = [&](u32 b, Scratch& sc, Facet& F) -> u32 {
          const auto I = cat.interior(b);
          const auto U = cat.shell(b);
          size_t i = 0, j = 0;
          F.n = k;
          for (u32 m = 0; m < k; ++m) F.s[m] = (j >= U.size() || (i < I.size() && I[i] < U[j])) ? I[i++] : U[j++];
          const u32 v = resolve(X, o, o.error, F, sc, sc.c[1][k]);
          return v == kNone ? kNone : ancestor(out, o.jumps, v, cat.rank[b] + 1, sc.walk[k]);  // kNone : erreur
        };                                                                                      // deja enregistree
        // premiere boule couvrante de chaque site : plus petit indice (ordre canonique, donc par niveau croissant)
        std::vector<u32> first(n, kNone);
        pool.parallel_for(cat.balls(), 4096, [&](u64 b0, u64 e0, unsigned) {
          for (u64 b = b0; b < e0; ++b) {
            if (!covering(b)) continue;
            for (u64 q = cat.pop_off[b]; q < cat.pop_off[b + 1]; ++q) {
              std::atomic_ref<u32> f(first[cat.pop[q]]);
              u32 cur = f.load(std::memory_order_relaxed);
              while (b < cur && !f.compare_exchange_weak(cur, u32(b), std::memory_order_relaxed)) {}
            }
          }
        });
        if (std::find(first.begin(), first.end(), kNone) != first.end()) {
          if (!o.error.load()) o.error.store(u32(Reason::census_mismatch) + 1);
          continue;
        }
        if (params.ball_nodes) {  // relation de couverture complete (vote) : une resolution par boule couvrante
          out.ball_node.assign(cat.balls(), kNone);
          pool.parallel_for(cat.balls(), 256, [&](u64 b0, u64 e0, unsigned wk) {
            Facet F;
            for (u64 b = b0; b < e0; ++b)
              if (covering(b)) out.ball_node[b] = cover_node(u32(b), scratch[wk], F);
          });
          for (u32 x = 0; x < n; ++x) out.point_node[x] = out.ball_node[first[x]];
        } else {  // seules les premieres boules couvrantes : au plus n resolutions
          std::vector<u32> need(first);
          std::sort(need.begin(), need.end());
          need.erase(std::unique(need.begin(), need.end()), need.end());
          std::vector<u32> node(need.size(), kNone);
          pool.parallel_for(need.size(), 64, [&](u64 i0, u64 e0, unsigned wk) {
            Facet F;
            for (u64 i = i0; i < e0; ++i) node[i] = cover_node(need[i], scratch[wk], F);
          });
          for (u32 x = 0; x < n; ++x)
            out.point_node[x] = node[std::lower_bound(need.begin(), need.end(), first[x]) - need.begin()];
        }
        for (u32 x = 0; x < n; ++x) out.point_cat_rank[x] = cat.rank[first[x]] + 1;
      }
    }
    // Entree core : une requete par site ; les k plus proches (cle exacte = distance carree, puis indice) sont le
    // prefixe des kq plus proches pour tout k <= kq ; D_k(x) est la cle du k-ieme.
    pk.swap(pk_core);
    const u32 kq = pk.empty() ? 0 : pk.back();
    if (kq > 0)
      pool.parallel_for(n, 64, [&](u64 b0, u64 e0, unsigned wk) {
        Scratch& sc = scratch[wk];
        Facet F;
        for (u64 x = b0; x < e0; ++x) {
          const P3& px = g.P[x];
          g.tree.nearest(px, geom::Center{{0, 0, 0}, 1}, kq, sc.knn);
          ++sc.c[1][kq].knn_queries;
          for (u32 k : pk) {
            OrderRun& o = *runs[k];
            OrderForest& out = t.orders[k - 1];
            F.n = k;
            for (u32 i = 0; i < k; ++i) F.s[i] = sc.knn[i].second;
            F.sort();
            const u64 lev = static_cast<u64>(sc.knn[k - 1].first);
            u32 v = resolve(X, o, o.error, F, sc, sc.c[1][k]);
            if (v == kNone) continue;  // erreur deja enregistree pour l'ordre
            const u32 r = ranks.at_most(cat, lev);
            v = ancestor(out, o.jumps, v, r, sc.walk[k]);
            out.point_node[x] = v;
            out.point_level[x] = lev;
          }
        }
      });
    for (u32 k = atlas.kfirst; k <= atlas.klast; ++k) {
      OrderRun& o = *runs[k];
      if (o.outcome.ok() && o.error.load()) o.outcome = fail(static_cast<Reason>(o.error.load() - 1), u8(k));
    }
  }
  ts.t_points = seconds_since(tstage);
  tstage = Clock::now();

  // compteurs (jonctions, attaches)
  for (u32 k = atlas.kfirst; k <= atlas.klast; ++k) {
    OrderForest& out = t.orders[k - 1];
    for (Scratch& sc : scratch) {
      out.stats.join.add(sc.c[0][k]);
      out.stats.point.add(sc.c[1][k]);
      out.stats.walk_steps += sc.walk[k];
      sc.walk[k] = 0;
    }
    out.descent_steps = out.stats.join.steps + out.stats.point.steps;
    out.memo_hits = out.stats.join.memo_hits + out.stats.point.memo_hits;
  }
  // issue : le plus petit ordre en echec
  {
    Outcome worst;
    for (u32 k = atlas.kfirst; k <= atlas.klast; ++k) {
      const Outcome& o = runs[k]->outcome;
      if (!o.ok() && (worst.ok() || o.precedes(worst))) worst = o;
    }
    if (!worst.ok()) return worst;
  }

  // ---- cartes verticales K -> K - 1
  // Naissance de la boule b au niveau a : une (k-1)-partie de sa boule fermee est realisee au centre, donc sa
  // descente a l'ordre k - 1 donne une naissance de la composante de L_{k-1}(a) qui contient la composante nee ;
  // l'image est l'ancetre vivant au niveau a. Fusion : image = ancetre au niveau de la fusion de l'image d'un
  // enfant ; tous les enfants doivent s'accorder (naturalite), sinon invariant_violated.
  if (params.verticals && params.only_order == 0 && keff >= 2) {
    Flat vb;
    for (u32 k = 2; k <= keff; ++k) {
      OrderForest& up = t.orders[k - 1];
      up.lower.assign(up.rank.size(), kNone);
      vb.add(k, runs[k]->nbirths());  // les naissances sont les premiers noeuds
    }
    pool.parallel_for(vb.total(), 64, [&](u64 b0, u64 e0, unsigned wk) {
      Scratch& sc = scratch[wk];
      Facet F;
      vb.each(b0, e0, [&](u32 k, u64 v) {
        OrderForest& up = t.orders[k - 1];
        const OrderForest& down = t.orders[k - 2];
        OrderRun& od = *runs[k - 1];
        const u32 ball = up.birth[v];
        if (!(cat.flags[ball] & kExtendedShell)) {
          // Naissance reguliere (p + m = k) : la (k-1)-partie « k-1 premiers sites de I puis U » est I u U \ {U[m-1]},
          // dernier representant de la jonction de la meme boule a l'ordre k - 1 (fenetre [k - 1, k]), deja resolu :
          // la descente est une fonction de (F, k - 1) seule.
          const BallInfo& bi = atlas.info[ball];
          if (bi.lo <= k - 1 && k - 1 <= bi.hi && ball_vnode[ball] != kNone) {
            ++sc.c[2][k].reused;
            up.lower[v] = ancestor(down, od.jumps, ball_vnode[ball], up.rank[v], sc.walk[k]);
            return true;
          }
        }
        F.n = 0;
        if (k == 2) {  // naissance d'ordre 2 : une (k-1)-partie est un site de la boule fermee
          F.n = 1;
          F.s[0] = cat.support[ball][0];
        } else {
          for (u32 s2 : cat.interior(ball))
            if (F.n < k - 1) F.s[F.n++] = s2;
          for (u32 s2 : cat.shell(ball))
            if (F.n < k - 1) F.s[F.n++] = s2;
          F.sort();
        }
        const u32 m = resolve(X, od, runs[k]->verror, F, sc, sc.c[2][k]);
        if (m == kNone) return false;
        up.lower[v] = ancestor(down, od.jumps, m, up.rank[v], sc.walk[k]);
        return true;
      });
    });
    std::vector<u32> ks;
    for (u32 k = keff; k >= 2; --k)
      if (!runs[k]->verror.load()) ks.push_back(k);
    pool.parallel_for(ks.size(), 1, [&](u64 b0, u64 e0, unsigned wk) {
      for (u64 i = b0; i < e0; ++i) {
        const u32 k = ks[i];
        const auto t0 = Clock::now();
        OrderForest& up = t.orders[k - 1];
        const OrderForest& down = t.orders[k - 2];
        const Jumps& dj = runs[k - 1]->jumps;
        u64& steps = scratch[wk].walk[k];
        const u32 nn = static_cast<u32>(up.rank.size());
        // fusions : dans l'ordre de creation (enfants avant parents)
        for (u32 v = 0; v < nn; ++v) {
          if (up.birth[v] != kNone) continue;
          u32 image = kNone;
          for (u32 j = up.child_off[v]; j < up.child_off[v + 1]; ++j) {
            const u32 c = ancestor(down, dj, up.lower[up.child_val[j]], up.rank[v], steps);
            if (image == kNone) image = c;
            else if (image != c) {
              runs[k]->outcome = fail(Reason::vertical_naturality, u8(k));
              break;
            }
          }
          if (!runs[k]->outcome.ok()) break;
          up.lower[v] = image;
        }
        up.stats.t_vertical = seconds_since(t0);
      }
    });
    for (u32 k = 2; k <= keff; ++k) {
      OrderForest& up = t.orders[k - 1];
      for (const Scratch& sc : scratch) {
        up.stats.vertical.add(sc.c[2][k]);
        up.stats.walk_steps += sc.walk[k];
      }
    }
    for (u32 k = 2; k <= keff; ++k) {
      if (runs[k]->verror.load()) return fail(Reason::descent_no_terminal, u8(k));
      if (!runs[k]->outcome.ok()) return runs[k]->outcome;
    }
  }
  ts.t_vertical = seconds_since(tstage);
  for (const Scratch& sc : scratch) ts.meb_fallbacks += sc.meb_fallbacks;
  return t;
}

PointDendrogram point_dendrogram(const Catalogue& cat, const OrderForest& f, const Cloud& cloud) {
  // Table de niveaux fusionnee des noeuds et des points, dans l'ordre exact des niveaux.
  //  - Cles du catalogue (noeuds ; points en entree cover) : rang r, de niveau 0 si r = 0 et cat.level[r - 1] sinon.
  //    Les niveaux du catalogue croissent strictement avec le rang : le tri par denombrement des rangs est exact.
  //  - Cles entieres (points en entree core) : D_K(x), triees comme entiers.
  //  Les deux suites sont fusionnees par comparaison exacte, avec un raccourci double quand les approximations
  //  s'ecartent de plus de 1e-9 en relatif, tres au-dessus de l'erreur d'arrondi de approx(). Les cles exactement
  //  egales recoivent le meme rang : leur ordre relatif ne change pas la sortie.
  const u32 nn = static_cast<u32>(f.rank.size());
  const u32 n = cloud.sites();
  const bool cover = !f.point_cat_rank.empty();
  auto cat_level = [&](u32 r) -> geom::Level {
    if (r == 0) return geom::Level{arith::I192{}, arith::I128w::from_u128(1)};
    return cat.level[r - 1];
  };
  auto int_level = [](u64 e) {
    geom::Level L;
    L.num = arith::I192::from_u128(e);
    L.den = arith::I128w::from_u128(1);
    return L;
  };
  auto near = [](double x, double y) { return x == y || std::abs(x - y) <= 1e-9 * std::max(x, y); };
  // A : cles du catalogue (0..nn-1 : noeuds ; nn.. : points en entree cover), par rang croissant
  const u32 na = nn + (cover ? n : 0);
  auto rank_of = [&](u32 i) { return i < nn ? f.rank[i] : f.point_cat_rank[i - nn]; };
  u32 rmax = 0;
  for (u32 i = 0; i < na; ++i) rmax = std::max(rmax, rank_of(i));
  std::vector<u32> start(u64(rmax) + 2, 0), A(na);
  for (u32 i = 0; i < na; ++i) ++start[u64(rank_of(i)) + 1];
  for (u64 r = 0; r <= rmax; ++r) start[r + 1] += start[r];
  for (u32 i = 0; i < na; ++i) A[start[rank_of(i)]++] = i;
  // B : points en entree core, par D_K croissant
  std::vector<u32> B;
  if (!cover) {
    B.resize(n);
    for (u32 x = 0; x < n; ++x) B[x] = x;
    std::sort(B.begin(), B.end(), [&](u32 a, u32 b) { return f.point_level[a] < f.point_level[b]; });
  }
  u32 cached = kNone;
  double cached_x = 0;
  auto x_rank = [&](u32 r) {  // approximation d'un niveau du catalogue, une fois par rang (A est trie par rang)
    if (r != cached) {
      cached = r;
      cached_x = cat_level(r).approx();
    }
    return cached_x;
  };
  std::vector<u32> merged(u64(nn) + n);
  PointDendrogram d;
  u64 ia = 0, ib = 0;
  bool first = true, prev_cat = false;
  u32 prev_r = 0;
  u64 prev_e = 0;
  double prev_x = 0;
  while (ia < A.size() || ib < B.size()) {
    bool take_a = ib == B.size();
    if (!take_a && ia < A.size()) {
      const u32 r = rank_of(A[ia]);
      const u64 e = f.point_level[B[ib]];
      const double xa = x_rank(r), xb = int_level(e).approx();
      take_a = !near(xa, xb) ? xa < xb : geom::compare(cat_level(r), int_level(e)) <= 0;
    }
    u32 slot, r = 0;
    u64 e = 0;
    double x;
    if (take_a) {
      slot = A[ia++];
      r = rank_of(slot);
      x = x_rank(r);
    } else {
      const u32 p = B[ib++];
      slot = nn + p;
      e = f.point_level[p];
      x = int_level(e).approx();
    }
    // niveaux publies en double, strictement croissants : deux niveaux exacts distincts dont les doubles coincident
    // (ou s'inversent d'un ulp) partagent un rang ; l'ordre exact des noeuds et des points est preserve
    // (regression tests/regression/test_level_collision.py)
    bool distinct = true;
    if (!first) {
      if (take_a && prev_cat) distinct = r != prev_r;
      else if (!take_a && !prev_cat) distinct = e != prev_e;
      else
        distinct = !near(prev_x, x) || geom::compare(prev_cat ? cat_level(prev_r) : int_level(prev_e),
                                                      take_a ? cat_level(r) : int_level(e)) != 0;
    }
    if (first || (distinct && x > d.level.back())) d.level.push_back(x);
    merged[slot] = static_cast<u32>(d.level.size() - 1);
    first = false;
    prev_cat = take_a;
    prev_r = r;
    prev_e = e;
    prev_x = x;
  }
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
  d.point_node = f.point_node;
  d.point_rank.resize(n);
  d.point_weight.assign(n, 1);
  for (u32 x = 0; x < n; ++x) d.point_rank[x] = merged[nn + x];
  return d;
}

}  // namespace mhgp10
