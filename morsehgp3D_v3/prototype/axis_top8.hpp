// MorseHGP3D v3 — `AxisTop8` : LES COMPLETIONS q4 D'UNE FACE AIGUE, SANS PRODUIT.
//
// Specification : theoreme `FaceAxisExtremalCompletion-8` de
// audits/AUDIT_DEBLOCAGE_SOURCE_SHALLOW_SANS_DELAUNAY_20260814.md.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=diagnostic_exact_borne,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// L'IDEE
//
// A FACE FIXE, le centre d'une sphere passant par les trois sommets n'a plus
// qu'UN degre de liberte : il parcourt la droite normale a la face issue du
// circumcentre. La profondeur d'un site le long de cette droite est alors une
// fonction AFFINE, donc chaque site a au plus UNE racine. Le probleme q4 se
// reduit a un probleme de niveau en dimension UN, ou « peu profond » veut dire
// « extremal ». Il n'y a plus de produit `carrier x apex` a former.
//
// ---------------------------------------------------------------------------
// LA PARAMETRISATION, ET POURQUOI `T2` EST EXACTEMENT JUNG
//
// Face `f=(a,b,x)`, arete owner `ab`. Avec `d=b-a`, `u=x-a` :
//
//   D=d.d   E=u.u   F=d.u   G=D E-F^2=|d x u|^2   n=d x u
//   W=E(D-F) d + D(E-F) u
//   c(tau)=a+(W+tau n)/(2 G)
//
// `W` est dans le plan de la face et `n` lui est orthogonal, donc `W.n=0` et
//
//   R(tau)^2=|c(tau)-a|^2=(|W|^2+tau^2 G)/(4 G^2).
//
// L'identite ALGEBRIQUE `|W|^2=D G (G+(E-F)^2)` — verifiee terme a terme — donne
//
//   R(tau)^2 <= 3 D/8   <=>   2 tau^2 <= T2,   T2=D (G-2 (E-F)^2).
//
// Autrement dit `J_f={tau : 2 tau^2 <= T2}` n'est pas une heuristique : c'est
// EXACTEMENT la borne de Jung en dimension trois pour un support de diametre
// `|ab|`. Si `T2<=0`, aucune completion q4 strictement positive n'existe. A
// `T2=0` le centre est dans le plan de la face et le poids de l'apex est nul :
// accepter ce cas serait une faute de frontiere, pas une tolerance.
//
// ---------------------------------------------------------------------------
// LA PUISSANCE AFFINE
//
// Pour un site `z`, avec `s=z-a` :
//
//   P_z(tau)=A_z-tau B_z,   A_z=G |s|^2-W.s,   B_z=n.s
//   z est STRICTEMENT interieur a la sphere de parametre `tau`  <=>  P_z(tau)<0
//
//   B_z>0 : interieur exactement pour tau > alpha_z=A_z/B_z
//   B_z<0 : interieur exactement pour tau < alpha_z
//   B_z=0 : interieur partout si A_z<0, jamais si A_z>0, SHELL si A_z=0
//
// Aucune racine n'est jamais formee en flottant : toutes les comparaisons sont
// des signes de polynomes entiers.
//
// ---------------------------------------------------------------------------
// LE THEOREME
//
// Masquer les trois IDs de la face. Soit `p` le nombre de sites strictement
// interieurs sur TOUT `J_f` (les permanents : `B=0,A<0`, les entrants
// strictement avant le bout gauche, les sortants strictement apres le bout
// droit). Si `p>=8`, la face est morte pour q4. Sinon, avec `k=8-p`, garder
// parmi les racines non permanentes rencontrant `J_f` les `k` plus PETITES de
// signe `B>0` et les `k` plus GRANDES de signe `B<0`.
//
// Alors tout apex dont la circumsphere est reguliere et porte au plus sept
// interieurs est dans l'un de ces deux ensembles. Il y a donc au plus
// `2k=16-2p` apex candidats, et seize au pire.
//
// PREUVE. Si `B_y>0` et si `alpha_y` n'est pas parmi les `k` plus petites, au
// moins `k` racines entrantes lui sont strictement anterieures ; leurs sites
// sont donc interieurs en `tau_y`, et avec les `p` permanents la profondeur
// vaut au moins `p+k=8`. Contradiction. Le cas `B_y<0` est symetrique. Une
// egalite est un shell et ne credite jamais l'interieur. La preuve n'emploie ni
// hypothese probabiliste, ni voisinage metrique, ni Delaunay.
//
// ---------------------------------------------------------------------------
// LARGEURS PROUVEES, PROFIL u16
//
// Avec `U=65535` : `D,E,|F| <= 3U^2 < 2^34`, `G <= 9U^4 < 2^68`,
// `|n_i| <= 2U^2 < 2^33`, `|W_i| <= 2 * 3U^2 * 6U^2 * U < 2^86`,
// `|A| <= G|s|^2 + |W.s| < 2^104`, `|B| <= 3 * 2^33 * U < 2^51`.
//   comparaison de racines  : `A_1 B_2` vs `A_2 B_1`      < 2^155
//   appartenance a `J_f`    : `2 A^2` vs `T2 B^2`          < 2^209
// `i128` ne suffit donc PAS : les deux comparaisons passent par `BigInt<4>`,
// soit 256 bits signes. C'est la raison d'etre de `mhgp::mul128` ici.
#pragma once

#include <cstdint>

#include "mhgp/mhgp.hpp"

namespace mhgp3v {
namespace axis8 {

using mhgp::i128;
using mhgp::i64;
using B4 = mhgp::BigInt<4>;

// ---------------------------------------------------------------------------
// MUTANTS. Chacun doit etre TUE par une fixture nommee.
// ---------------------------------------------------------------------------
enum class A8Mutant {
  kNone,
  kCap15,             // plafonne la selection a quinze racines
  kKFixe7,            // `k=7` au lieu de `k=8-p`
  kSigneBInverse,     // garde les plus petites racines pour `B<0`
  kBoutsOuverts,      // traite `J_f` comme OUVERT : perd l'apex qui sature Jung
  kPermanenceLarge,   // `<=` au lieu de `<` sur la permanence : gonfle `p`
  kOublieBZero,       // ignore les permanents `B=0, A<0`
  kAbsAvantTri,       // trie sur `|B|`, donc perd l'orientation
};

inline const char* a8_mutant_name(A8Mutant m) {
  switch (m) {
    case A8Mutant::kNone: return "none";
    case A8Mutant::kCap15: return "a8-cap15";
    case A8Mutant::kKFixe7: return "a8-k-fixe-7";
    case A8Mutant::kSigneBInverse: return "a8-signe-b-inverse";
    case A8Mutant::kBoutsOuverts: return "a8-bouts-ouverts";
    case A8Mutant::kPermanenceLarge: return "a8-permanence-large";
    case A8Mutant::kOublieBZero: return "a8-oublie-b-zero";
    case A8Mutant::kAbsAvantTri: return "a8-abs-avant-tri";
  }
  return "?";
}

// ---------------------------------------------------------------------------
// L'AXE D'UNE FACE.
// ---------------------------------------------------------------------------
struct FaceAxis {
  i64 a[3] = {0, 0, 0};
  i128 D = 0, E = 0, F = 0, G = 0;
  i128 n[3] = {0, 0, 0};
  i128 W[3] = {0, 0, 0};
  i128 T2 = 0;
  bool reguliere = false;      // `G>0` : les trois points ne sont pas alignes
  bool jung_ouverte = false;   // `T2>0` : le segment n'est pas reduit a un point
};

inline FaceAxis face_axis(const i64 a[3], const i64 b[3], const i64 x[3]) {
  FaceAxis f;
  i128 d[3], u[3];
  for (int i = 0; i < 3; ++i) {
    f.a[i] = a[i];
    d[i] = (i128)b[i] - a[i];
    u[i] = (i128)x[i] - a[i];
  }
  f.D = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
  f.E = u[0] * u[0] + u[1] * u[1] + u[2] * u[2];
  f.F = d[0] * u[0] + d[1] * u[1] + d[2] * u[2];
  f.G = f.D * f.E - f.F * f.F;
  f.n[0] = d[1] * u[2] - d[2] * u[1];
  f.n[1] = d[2] * u[0] - d[0] * u[2];
  f.n[2] = d[0] * u[1] - d[1] * u[0];
  const i128 P = f.D - f.F, Q = f.E - f.F;
  for (int i = 0; i < 3; ++i) f.W[i] = f.E * P * d[i] + f.D * Q * u[i];
  f.T2 = f.D * (f.G - 2 * Q * Q);
  f.reguliere = (f.G > 0);
  f.jung_ouverte = f.reguliere && (f.T2 > 0);
  return f;
}

// Le triangle est-il STRICTEMENT aigu ? Les trois produits scalaires de coin.
inline bool face_aigue(const i64 a[3], const i64 b[3], const i64 x[3]) {
  auto dot = [](const i64 o[3], const i64 p[3], const i64 q[3]) -> i128 {
    i128 s = 0;
    for (int i = 0; i < 3; ++i) s += (i128)(p[i] - o[i]) * (q[i] - o[i]);
    return s;
  };
  return dot(a, b, x) > 0 && dot(b, a, x) > 0 && dot(x, a, b) > 0;
}

// ---------------------------------------------------------------------------
// LA PUISSANCE AFFINE D'UN SITE.
// ---------------------------------------------------------------------------
struct SitePower {
  i128 A = 0;
  i128 B = 0;
};

inline SitePower site_power(const FaceAxis& f, const i64 z[3]) {
  SitePower p;
  i128 s[3];
  for (int i = 0; i < 3; ++i) s[i] = (i128)z[i] - f.a[i];
  const i128 sn = s[0] * s[0] + s[1] * s[1] + s[2] * s[2];
  const i128 ws = f.W[0] * s[0] + f.W[1] * s[1] + f.W[2] * s[2];
  p.A = f.G * sn - ws;
  p.B = f.n[0] * s[0] + f.n[1] * s[1] + f.n[2] * s[2];
  return p;
}

// ---------------------------------------------------------------------------
// COMPARAISONS EXACTES SUR 256 BITS.
// ---------------------------------------------------------------------------

// signe de `2 A^2 - T2 Y^2`.
inline int sgn_2AA_moins_T2YY(i128 A, i128 Y, i128 T2) {
  const B4 aa = mhgp::mul128(A, A);
  const B4 aa2 = mhgp::big_add(aa, aa);
  const B4 yy = mhgp::mul128(T2, Y * Y);
  return mhgp::big_cmp(aa2, yy);
}

// signe de `A - Y tau_max` avec `tau_max^2 = T2/2`, `T2>0`.
inline int sgn_A_moins_Ytau(i128 A, i128 Y, i128 T2) {
  if (A == 0 && Y == 0) return 0;
  if (A >= 0 && Y <= 0) return 1;
  if (A <= 0 && Y >= 0) return -1;
  const int c = sgn_2AA_moins_T2YY(A, Y, T2);
  return (A > 0) ? c : -c;   // `A<0` et `Y<0` : le signe s'inverse
}

// signe de `alpha_z - eps tau_max`, `eps` valant `+1` ou `-1`. Suppose `B!=0`.
inline int cmp_racine_bout(const SitePower& p, i128 T2, int eps) {
  const int s = sgn_A_moins_Ytau(p.A, p.B * eps, T2);
  return (p.B > 0) ? s : -s;
}

// signe de `alpha_1 - alpha_2`. Suppose `B_1!=0` et `B_2!=0`.
inline int cmp_racines(const SitePower& p, const SitePower& q) {
  const B4 l = mhgp::mul128(p.A, q.B);
  const B4 r = mhgp::mul128(q.A, p.B);
  int s = mhgp::big_cmp(l, r);
  if (p.B < 0) s = -s;
  if (q.B < 0) s = -s;
  return s;
}

// ---------------------------------------------------------------------------
// CLASSIFICATION D'UN SITE SUR `J_f`.
// ---------------------------------------------------------------------------
enum class SiteClass {
  kPermanent,        // interieur sur tout `J_f`
  kJamais,           // interieur nulle part sur `J_f`
  kEntrant,          // `B>0` : interieur pour `tau > alpha`
  kSortant,          // `B<0` : interieur pour `tau < alpha`
  kShellPersistant,  // `B=0` et `A=0` : cospherique sur tout l'axe
};

inline SiteClass classify(const SitePower& p, i128 T2, A8Mutant mut = A8Mutant::kNone) {
  if (p.B == 0) {
    if (p.A == 0) return SiteClass::kShellPersistant;
    if (p.A < 0) {
      if (mut == A8Mutant::kOublieBZero) return SiteClass::kJamais;
      return SiteClass::kPermanent;
    }
    return SiteClass::kJamais;
  }
  const int gauche = cmp_racine_bout(p, T2, -1);   // signe de alpha + tau_max
  const int droite = cmp_racine_bout(p, T2, +1);   // signe de alpha - tau_max
  // `J_f` EST FERME. Une racine tombant exactement sur un bout reste une racine,
  // donc un apex candidat : c'est le cas du tetraedre REGULIER, qui realise la
  // borne de Jung a l'egalite. La permanence, elle, exige le bout STRICT — a
  // `alpha = -tau_max` le site est sur la sphere du bout gauche, pas dedans.
  const bool ouvert = (mut == A8Mutant::kBoutsOuverts);
  const bool perm_large = (mut == A8Mutant::kPermanenceLarge);
  if (p.B > 0) {
    if (perm_large ? (gauche <= 0) : (gauche < 0)) return SiteClass::kPermanent;
    if (ouvert ? (droite >= 0) : (droite > 0)) return SiteClass::kJamais;
    return SiteClass::kEntrant;
  }
  if (perm_large ? (droite >= 0) : (droite > 0)) return SiteClass::kPermanent;
  if (ouvert ? (gauche <= 0) : (gauche < 0)) return SiteClass::kJamais;
  return SiteClass::kSortant;
}

// ---------------------------------------------------------------------------
// SELECTION EXTREMALE. Capacite fixe, DEBORDEMENT EXPLICITE : jamais de
// troncature silencieuse. Les groupes d'egalite sont conserves entiers, donc
// une concurrence peut depasser `2k` ; c'est alors un refus, pas un choix.
// ---------------------------------------------------------------------------
inline constexpr int kCapSelection = 64;

struct Selection {
  bool face_morte = false;      // `T2<=0`, face degeneree, ou `p>=8`
  bool debordement = false;     // groupes d'egalite au-dela de la capacite
  bool shell_persistant = false;
  int p = 0;                    // permanents
  int k = 0;                    // `8-p`
  int n_entrants = 0, n_sortants = 0;
  int entrants[kCapSelection] = {0};
  int sortants[kCapSelection] = {0};
};

// `sites` : indices ; `pw(i)` rend la `SitePower` du site `i`. Les trois IDs de
// la face doivent DEJA etre masques par l'appelant.
template <class PowFn>
inline Selection select_top8(const FaceAxis& f, const int* sites, int n_sites,
                             PowFn pw, A8Mutant mut = A8Mutant::kNone) {
  Selection sel;
  const bool t2_ok = (f.T2 > 0);
  if (!f.reguliere || !t2_ok) { sel.face_morte = true; return sel; }

  // Passe 1 : les permanents. Ils decident `k` et ne sont jamais des apex.
  int perm = 0;
  for (int i = 0; i < n_sites; ++i) {
    const SitePower p = pw(sites[i]);
    const SiteClass c = classify(p, f.T2, mut);
    if (c == SiteClass::kPermanent) ++perm;
    else if (c == SiteClass::kShellPersistant) sel.shell_persistant = true;
  }
  sel.p = perm;
  if (perm >= 8) { sel.face_morte = true; return sel; }
  sel.k = (mut == A8Mutant::kKFixe7) ? 7 : (8 - perm);
  // Le cap du mutant est GLOBAL : c'est le nombre total de racines retenues
  // qu'il plafonne, pas celui d'un cote.
  const int cap_total = (mut == A8Mutant::kCap15) ? 15 : (2 * kCapSelection);

  // Passe 2 : les `k` plus petites racines entrantes et les `k` plus grandes
  // sortantes, groupes d'egalite complets.
  for (int pass = 0; pass < 2; ++pass) {
    const bool entrant = (pass == 0);
    int* out = entrant ? sel.entrants : sel.sortants;
    int& cnt = entrant ? sel.n_entrants : sel.n_sortants;
    // `garde_petites` : vrai pour les entrants. Le mutant l'inverse pour `B<0`.
    bool garde_petites = entrant;
    if (!entrant && mut == A8Mutant::kSigneBInverse) garde_petites = true;
    for (int i = 0; i < n_sites; ++i) {
      const int id = sites[i];
      const SitePower p = pw(id);
      const SiteClass c = classify(p, f.T2, mut);
      if (entrant ? (c != SiteClass::kEntrant) : (c != SiteClass::kSortant)) continue;
      // Combien de racines deja retenues sont STRICTEMENT meilleures ?
      int strictement_mieux = 0;
      for (int j = 0; j < n_sites; ++j) {
        if (j == i) continue;
        const SitePower q = pw(sites[j]);
        const SiteClass cq = classify(q, f.T2, mut);
        if (entrant ? (cq != SiteClass::kEntrant) : (cq != SiteClass::kSortant)) continue;
        int cr = cmp_racines(q, p);
        if (mut == A8Mutant::kAbsAvantTri && p.B < 0) cr = -cr;
        if (garde_petites ? (cr < 0) : (cr > 0)) ++strictement_mieux;
      }
      if (strictement_mieux >= sel.k) continue;   // hors des `k` extremes
      if (sel.n_entrants + sel.n_sortants >= cap_total) { sel.debordement = true; break; }
      if (cnt >= kCapSelection) { sel.debordement = true; break; }
      out[cnt++] = id;
    }
  }
  return sel;
}

}  // namespace axis8
}  // namespace mhgp3v
