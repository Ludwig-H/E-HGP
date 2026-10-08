// MES-G-APP (microbanc hors produit) : les deux postes archetypes de l'etage G ecrits UNE fois, en source unique
// __host__ __device__ (meme discipline que la feuille du catalogue, src/catalogue/simt.hpp) :
//   - census GARDE A TEMOINS d'une boule certifiee (port de src/index/census_workspace.cpp, BorrowedPass::walk, et de
//     src/num/guard.cpp, GuardedSphere : pave, point entier le plus proche, coin lointain, puissance D|v|^2 - 2 N.v en
//     i128) ; un fil par requete, parcours prefixe SANS PILE par les pointeurs de sortie de l'index (escape) ;
//   - premiere sonde de la table de populations (LEM-POP, src/tower/populations.cpp : empreinte additive splitmix64,
//     repertoire par bits de tete, dichotomie sur (empreinte, population)) d'une trace stricte I u A
//     (src/tower/passes.cpp, build_trace) ; un fil par cellule.
// Ni allocation, ni recursion, ni flottant, ni exception : memes decisions que le produit, compteurs du travail compris
// (noeuds, sites testes, blocs, temoins, garde, voies), a l'octet. Voies prises en charge : native et certifiee (meme
// formule i128 que GuardedSphere::power_sign) ; une requete controlee ou large est rendue NON RESOLUE (repli exact de
// l'hote, CONTRAT_CATALOGUE R7). Les coquilles de plus de 64 sites sont signalees (le produit les refuse :
// shell_capacity).
//
// Mutant causal (copie compilee a part, jamais le chemin mesure) : MESG_MUTANT_COTE_NUL compte un site SUR la sphere
// comme interieur ; la porte d'identite doit le tuer (code 1).
#pragma once

#include <cstdint>

#if defined(__CUDACC__)
#define MESG_HD __host__ __device__ inline
#else
#define MESG_HD inline
#endif

namespace mesg {

using u8 = std::uint8_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i64 = std::int64_t;
__extension__ typedef __int128 i128;

inline constexpr u32 kAucun = 0xFFFFFFFFu;
inline constexpr u32 kMaxPartie = 12;    // K <= 12 (tower_detail::kMaxPart)
inline constexpr u32 kMaxCoquille = 64;  // coquille d'un census complet (tower_detail::kMaxShell)
inline constexpr u32 kMaxTemoins = 4;    // CensusWorkspace::kMaxWitnesses
inline constexpr u32 kSitesParRequete = kMaxPartie + kMaxCoquille;

// Noeud de l'index (index_detail::Node) : boite fermee, plage [debut, fin) de SiteIdx, premier noeud apres le
// sous-arbre en ordre prefixe.
struct Noeud {
  u32 lo[3];
  u32 hi[3];
  u32 debut, fin;
  u64 sortie;
};

enum Voie : u32 { kNative = 0, kCertifiee = 1, kControlee = 2, kLarge = 3 };

// Requete de census : boule certifiee (centre ancre + N/D, D > 0, coefficients natifs i128), repere de son support
// (coin minimal, etendue s), seuil k, temoins (support canonique, sites SUR la sphere), voie de GuardedSphere.
struct Requete {
  i128 n[3];
  i128 d;
  i64 ancre[3];
  u32 coin[3];
  u32 etendue;
  u32 seuil;
  u32 voie;
  u32 temoins_n;
  u32 temoins[kMaxTemoins];
};

// Garde preparee une fois par requete (constructeur de GuardedSphere) : pave ouvert ]coin - M, coin + 2M[, entier le
// plus proche du centre en local (ex aequo : le plus petit), seuil du coin lointain ceil(2 (c - o)).
struct Garde {
  i64 lo[3], hi[3], proche[3], seuil2[3];
  bool norme_courte;  // s + 2 <= 30 : |v|^2 en i64
};

// Compteurs d'une requete : ceux du registre du census (CensusLedger) et de la garde (GuardLedger), voies sommees.
struct Compteurs {
  u32 noeuds, tests, dedans, dehors, temoins, disjointes, partielles, garde_dehors, voies;
};

// Drapeaux d'un resultat.
inline constexpr u32 kDrapeauSature = 1u;
inline constexpr u32 kDrapeauNonResolu = 2u;    // voie controlee ou large : repli de l'hote
inline constexpr u32 kDrapeauCoquilleLarge = 4u;  // plus de 64 sites sur la sphere : le produit refuse (shell_capacity)
inline constexpr u32 kDrapeauInvariant = 8u;    // minorant > majorant (arithmetic_invariant du produit)

struct Resultat {
  u32 p, m, drapeaux, pad;
  Compteurs c;
};

// Dispositions partagees par l'hote (g++) et l'appareil (nvcc) : tableaux copies tels quels.
static_assert(sizeof(Noeud) == 40 && sizeof(Requete) == 144 && sizeof(Resultat) == 52,
              "mesg : disposition des tableaux partages");

MESG_HD int signe(i128 v) { return (v > 0) - (v < 0); }

// Rang du bit de poids faible (x != 0).
MESG_HD u32 ctz64(u64 x) {
#if defined(__CUDA_ARCH__)
  return static_cast<u32>(__ffsll(static_cast<long long>(x)) - 1);
#else
  return static_cast<u32>(__builtin_ctzll(x));
#endif
}

// GuardedSphere::GuardedSphere, coefficients natifs (detail::floor_division, detail::saturate).
MESG_HD void preparer(const Requete& r, Garde& g) {
  const i64 m = i64{1} << r.etendue;
  for (int j = 0; j < 3; ++j) {
    g.lo[j] = i64{r.coin[j]} - m;
    g.hi[j] = i64{r.coin[j]} + 2 * m;
    i128 q = r.n[j] / r.d;
    i128 reste = r.n[j] - q * r.d;
    if (reste < 0) {
      --q;
      reste += r.d;
    }
    const i64 qs = q < -m ? -m : q > m ? m : static_cast<i64>(q);
    const bool zero = reste == 0, demi = 2 * reste <= r.d;
    g.proche[j] = qs + (demi ? 0 : 1);
    g.seuil2[j] = 2 * qs + (zero ? 0 : demi ? 1 : 2);
  }
  g.norme_courte = r.etendue + 2 <= 30;
}

// GuardedSphere::power_sign, voies native et certifiee : D |v|^2 + sum N_j (-2 v_j), budget 6s+11 <= 107 bits.
MESG_HD int signe_puissance(const Requete& r, const Garde& g, const i64 (&v)[3]) {
  const i128 norme = g.norme_courte ? i128{v[0] * v[0] + v[1] * v[1] + v[2] * v[2]}
                                    : i128{v[0]} * v[0] + i128{v[1]} * v[1] + i128{v[2]} * v[2];
  i128 total = r.d * norme;
  for (int j = 0; j < 3; ++j) total += r.n[j] * (-2 * i128{v[j]});
  return signe(total);
}

// GuardedSphere::side : hors du pave, exterieur sans arithmetique ; sinon puissance a l'ecart a l'ancre.
MESG_HD int cote(const Requete& r, const Garde& g, u32 x, u32 y, u32 z, Compteurs& c) {
  const i64 p[3] = {i64{x}, i64{y}, i64{z}};
  for (int j = 0; j < 3; ++j)
    if (p[j] <= g.lo[j] || p[j] >= g.hi[j]) {
      ++c.garde_dehors;
      return 1;
    }
  const i64 v[3] = {p[0] - r.ancre[0], p[1] - r.ancre[1], p[2] - r.ancre[2]};
  ++c.voies;
  return signe_puissance(r, g, v);
}

// GuardedSphere::bound_signs : boite disjointe du pave, exterieure ; minorant au point entier le plus proche ramene dans
// la boite ; boite non contenue dans le pave : majorant positif sans arithmetique ; sinon coin lointain.
MESG_HD bool bornes(const Requete& r, const Garde& g, const Noeud& nd, int& bas, int& haut, Compteurs& c) {
  bool contenue = true;
  for (int j = 0; j < 3; ++j) {
    if (i64{nd.hi[j]} <= g.lo[j] || i64{nd.lo[j]} >= g.hi[j]) {
      ++c.disjointes;
      bas = haut = 1;
      return true;
    }
    contenue = contenue && i64{nd.lo[j]} > g.lo[j] && i64{nd.hi[j]} < g.hi[j];
  }
  i64 pres[3], loin[3];
  for (int j = 0; j < 3; ++j) {
    const i64 centre = r.ancre[j] + g.proche[j], lo = nd.lo[j], hi = nd.hi[j];
    pres[j] = (centre < lo ? lo : centre > hi ? hi : centre) - r.ancre[j];
    loin[j] = (lo + hi - 2 * r.ancre[j] >= g.seuil2[j] ? hi : lo) - r.ancre[j];
  }
  ++c.voies;
  const int b = signe_puissance(r, g, pres);
  if (b > 0) {
    bas = haut = 1;
    return true;
  }
  if (!contenue) {
    ++c.partielles;
    bas = b;
    haut = 1;
    return true;
  }
  ++c.voies;
  const int h = signe_puissance(r, g, loin);
  bas = b;
  haut = h;
  return b <= h;
}

// Census garde a temoins, une passe (CensusWorkspace::query avec temoins) : I croissant dans interieur[0..p), U dans
// l'ordre de decouverte (croissant) dans coquille[0..min(m, 64)). Rend p, m, drapeaux et compteurs. Compteurs et
// drapeaux en variables locales, ecrits une fois a la fin (out peut etre en memoire globale de l'appareil).
MESG_HD void census(const Requete& r, const Noeud* noeuds, u64 n_noeuds, const u32* x, const u32* y, const u32* z,
                    u32 taille_feuille, u32* interieur, u32* coquille, Resultat& out) {
  out.p = out.m = out.drapeaux = out.pad = 0;
  out.c = Compteurs{0, 0, 0, 0, 0, 0, 0, 0, 0};
  if (r.voie != kNative && r.voie != kCertifiee) {
    out.drapeaux = kDrapeauNonResolu;
    return;
  }
  Garde g;
  preparer(r, g);
  Compteurs c{0, 0, 0, 0, 0, 0, 0, 0, 0};
  u32 p = 0, m = 0, drapeaux = 0;
  const u32 seuil = r.seuil;
  for (u64 curseur = 0; curseur < n_noeuds && p < seuil;) {
    const Noeud& nd = noeuds[curseur];
    ++c.noeuds;
    bool temoin = false;
    for (u32 t = 0; t < r.temoins_n; ++t) temoin = temoin || (r.temoins[t] >= nd.debut && r.temoins[t] < nd.fin);
    const bool feuille = nd.fin - nd.debut <= taille_feuille;
    int bas = 0, haut = 0;
    if (temoin) {
      ++c.temoins;  // minorant <= 0 et majorant >= 0 au temoin : raffine sans bornes evaluees
    } else {
      if (!bornes(r, g, nd, bas, haut, c)) {
        drapeaux |= kDrapeauInvariant;
        break;
      }
      if (bas > 0) {
        ++c.dehors;
        curseur = nd.sortie;
        continue;
      }
      if (haut < 0) {
        ++c.dedans;
        const u32 n = nd.fin - nd.debut < seuil - p ? nd.fin - nd.debut : seuil - p;
        for (u32 i = 0; i < n; ++i) interieur[p + i] = nd.debut + i;
        p += n;
        curseur = nd.sortie;
        continue;
      }
    }
    if (!feuille) {
      ++curseur;
      continue;
    }
    for (u32 i = nd.debut; i < nd.fin && p < seuil; ++i) {
      ++c.tests;
      const int s = cote(r, g, x[i], y[i], z[i], c);
#if defined(MESG_MUTANT_COTE_NUL)
      if (s <= 0) {
#else
      if (s < 0) {
#endif
        interieur[p++] = i;
      } else if (s == 0) {
        if (m < kMaxCoquille) coquille[m] = i;
        else drapeaux |= kDrapeauCoquilleLarge;
        ++m;
      }
    }
    curseur = nd.sortie;
  }
  if (p == seuil) {
    drapeaux |= kDrapeauSature;
    drapeaux &= ~kDrapeauCoquilleLarge;  // census sature : la coquille n'est pas rendue
    m = 0;
  }
  out.p = p;
  out.m = m;
  out.drapeaux = drapeaux;
  out.c = c;
}

// ---------------------------------------------------------------------------------------------- premieres sondes
// Empreinte additive d'un site (tower_detail::site_key, melange splitmix64).
MESG_HD u64 cle_site(u32 site) {
  u64 z = u64{site} + 0x9E3779B97F4A7C15ull;
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  return z ^ (z >> 31);
}

// Table des naissances d'un ordre k (LEM-POP) : fiches de 3 + k mots (empreinte basse, empreinte haute, naissance,
// population triee) dans l'ordre canonique (empreinte, population), repertoire de 2^d + 1 places par d bits de tete.
struct Table {
  const u32* fiches;
  const u32* repertoire;
  u64 entrees;
  u32 decalage;  // 64 : un seul seau
  u32 k;
};

MESG_HD u64 cle_fiche(const Table& t, u64 pos) {
  const u32* f = t.fiches + pos * (3 + t.k);
  return u64{f[0]} | (u64{f[1]} << 32);
}

// Signe de (empreinte, population) de la fiche pos moins (cle, F).
MESG_HD int comparer(const Table& t, u64 pos, u64 cle, const u32* partie) {
  const u64 a = cle_fiche(t, pos);
  if (a != cle) return a < cle ? -1 : 1;
  const u32* f = t.fiches + pos * (3 + t.k) + 3;
  for (u32 i = 0; i < t.k; ++i)
    if (f[i] != partie[i]) return f[i] < partie[i] ? -1 : 1;
  return 0;
}

// Naissance de population exactement F, ou kAucun (PopulationTable::find).
MESG_HD u32 chercher(const Table& t, const u32* partie, u64 cle) {
  if (t.entrees == 0) return kAucun;
  const u64 seau = t.decalage >= 64 ? 0 : cle >> t.decalage;
  u64 lo = t.repertoire[seau];
  const u64 hi = t.repertoire[seau + 1];
  u64 h = hi;
  while (lo < h) {
    const u64 milieu = lo + (h - lo) / 2;
    if (comparer(t, milieu, cle, partie) < 0) lo = milieu + 1;
    else h = milieu;
  }
  if (lo < hi && comparer(t, lo, cle, partie) == 0) return t.fiches[lo * (3 + t.k) + 2];
  return kAucun;
}

// Trace stricte I u A du representant de masque `masque` (bit j : j-ieme site de U), SiteIdx croissants
// (passes.cpp, build_trace) ; rend son cardinal, 0 si elle depasse kMaxPartie.
MESG_HD u32 trace(const u32* interieur, u32 p, const u32* coquille, u64 masque, u32* partie) {
  u32 i = 0, n = 0;
  for (u64 reste = masque; i < p || reste != 0;) {
    const u32 j = reste != 0 ? ctz64(reste) : 0;
    const bool depuis_i = reste == 0 || (i < p && interieur[i] < coquille[j]);
    if (n == kMaxPartie) return 0;
    partie[n++] = depuis_i ? interieur[i++] : coquille[j];
    if (!depuis_i) reste &= reste - 1;
  }
  return n;
}

// Premieres sondes des representants d'une cellule de l'ordre k : sonde[r] = naissance ou kAucun.
MESG_HD void sonder_cellule(const Table& t, const u64* pop_debut, const u32* pop_sites, const u32* boule_p, u32 boule,
                            const u64* rep_debut, const u64* masques, u32 cellule, u32* sonde) {
  const u64 a = pop_debut[boule], b = pop_debut[boule + 1];
  const u32 p = boule_p[boule];
  const u32* interieur = pop_sites + a;
  const u32* coquille = pop_sites + a + p;
  (void)b;
  for (u64 r = rep_debut[cellule]; r < rep_debut[cellule + 1]; ++r) {
    u32 partie[kMaxPartie];
    const u32 n = trace(interieur, p, coquille, masques[r], partie);
    if (n != t.k) {
      sonde[r] = kAucun - 1;  // trace invalide : ecart garanti avec le produit
      continue;
    }
    u64 cle = 0;
    for (u32 i = 0; i < n; ++i) cle += cle_site(partie[i]);
    sonde[r] = chercher(t, partie, cle);
  }
}

// ---------------------------------------------------------------------------------------------- proposition
// Proposition flottante de plus petite boule (src/tower/proposal.hpp, DWelzl de la v10 porte par MES-M3) : copie
// TEXTUELLE de la classe du produit (empreinte relevee par le pilote), seules les fonctions membres sont annotees
// __host__ __device__. Rien n'y decide ; mais la route de chaque plus petite boule (LEM-T1, certificat, repli), donc
// les compteurs du travail, en depend : l'appareil doit rendre les memes bits que l'hote (binaire64 IEEE, ni
// contraction -fmad=false, ni -use_fast_math ; division IEEE par defaut).
#if defined(__CUDACC__)
#define MESG_HD_MEMBRE __host__ __device__
#else
#define MESG_HD_MEMBRE
#endif

struct DBallHD {
  double c[3] = {0, 0, 0};
  double r2 = -1;
  int R[4] = {0, 0, 0, 0};
  int nr = 0;
};

class DWelzlHD {
 public:
  static constexpr int kCapacity = 12;
  double p[kCapacity][3];
  bool ok = true;

  MESG_HD_MEMBRE DBallHD run(int n) {
    {
      const DBallHD B = run_support(n);
      if (ok) return B;
      ok = true;
    }
    // repli : Welzl a deplacement en tete, du plus loin au plus proche du barycentre
    double g[3] = {0, 0, 0};
    for (int i = 0; i < n; ++i)
      for (int a = 0; a < 3; ++a) g[a] += p[i][a];
    for (int a = 0; a < 3; ++a) g[a] /= n;
    double key[kCapacity];
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

 private:
  MESG_HD_MEMBRE DBallHD through(const int* R, int nr) {
    DBallHD B;
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
    if (nr == 3) return through3(B, a, p[R[1]], p[R[2]]);
    return through4(B, a, p[R[1]], p[R[2]], p[R[3]]);
  }
  MESG_HD_MEMBRE DBallHD through3(DBallHD B, const double* a, const double* b, const double* d) {
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
  MESG_HD_MEMBRE DBallHD through4(DBallHD B, const double* a, const double* b, const double* d, const double* e) {
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
  MESG_HD_MEMBRE bool contains(const DBallHD& B, int i) const {
    if (B.r2 < 0) return false;
    double d2 = 0;
    for (int a = 0; a < 3; ++a) {
      const double t = p[i][a] - B.c[a];
      d2 += t * t;
    }
    return d2 <= B.r2 * (1 + 1e-10) + 1e-9;
  }
  // Welzl a deplacement en tete (Gaertner) : L est la liste des points, S le support courant.
  int L[kCapacity];
  int S[4];
  int ns = 0;
  DBallHD ball;
  MESG_HD_MEMBRE void mtf(int end) {
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
  // Bornes explicites (n <= 4, nr < 4 avant ecriture) : meme resultat, sans le faux positif -Warray-bounds de GCC
  // une fois la recursion expansee dans run_support.
  MESG_HD_MEMBRE DBallHD small(const int* T, int n, int* R, int nr) {
    if (!ok) return DBallHD{};
    if (n <= 0 || n > 4 || nr < 0 || nr >= 4) return through(R, nr < 0 ? 0 : nr > 4 ? 4 : nr);
    DBallHD B = small(T, n - 1, R, nr);
    if (!ok || contains(B, T[n - 1])) return B;
    R[nr] = T[n - 1];
    return small(T, n - 1, R, nr + 1);
  }
  MESG_HD_MEMBRE int farthest(int n, const double* q) const {
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
  }
  // Depart sur une paire eloignee (le plus loin du barycentre, puis le plus loin de lui), puis, tant qu'un point sort
  // de la boule, plus petite boule du support et du pire point, ce point au bord (lemme de Welzl). Cout seulement.
  MESG_HD_MEMBRE DBallHD run_support(int n) {
    double gc[3] = {0, 0, 0};
    for (int i = 0; i < n; ++i)
      for (int a = 0; a < 3; ++a) gc[a] += p[i][a];
    for (int a = 0; a < 3; ++a) gc[a] /= n;
    const int ia = farthest(n, gc);
    const int ib = farthest(n, p[ia]);
    if (ia == ib) {
      ok = false;
      return DBallHD{};
    }
    int S2[4] = {ia, ib, 0, 0};
    DBallHD B = through(S2, 2);
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
      const int nt = B.nr < 0 ? 0 : B.nr > 4 ? 4 : B.nr;
      for (int i = 0; i < nt; ++i) T[i] = B.R[i];
      int R[4] = {v, 0, 0, 0};
      B = small(T, nt, R, 1);
    }
    ok = false;
    return DBallHD{};
  }
};

// Proposition d'une partie (resolve.cpp, propose) : repere local par le premier site.
struct Proposition {
  double c[3];
  double r2;
  u32 ok, nr;
  u32 r[4];
};

static_assert(sizeof(Proposition) == 56, "mesg : disposition des propositions");

MESG_HD void proposer(const u32* partie, u32 k, const u32* x, const u32* y, const u32* z, Proposition& out) {
  DWelzlHD w;
  const u32 o = partie[0];
  for (u32 i = 0; i < k; ++i) {
    w.p[i][0] = static_cast<double>(x[partie[i]]) - static_cast<double>(x[o]);
    w.p[i][1] = static_cast<double>(y[partie[i]]) - static_cast<double>(y[o]);
    w.p[i][2] = static_cast<double>(z[partie[i]]) - static_cast<double>(z[o]);
  }
  const DBallHD b = w.run(static_cast<int>(k));
  for (int a = 0; a < 3; ++a) out.c[a] = b.c[a];
  out.r2 = b.r2;
  out.ok = w.ok ? 1u : 0u;
  out.nr = static_cast<u32>(b.nr);
  for (int i = 0; i < 4; ++i) out.r[i] = static_cast<u32>(b.R[i]);
}

}  // namespace mesg
