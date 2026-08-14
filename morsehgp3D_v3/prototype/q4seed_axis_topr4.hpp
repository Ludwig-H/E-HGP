// MorseHGP3D v3 — `Q4SeedAxisTopR4` : NOYAU INTERNE DE `Lane4`.
//
// Specification : PROPOSITION.md section 7.2,
// `Q4SeedAxisExtremalCompletion-r4`.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=diagnostic_exact_borne,
//         public_status=not_claimed.
//
// ---------------------------------------------------------------------------
// CE QUE CE FICHIER N'EST PAS
//
// Son entree est un `Q4Seed3` : un prefixe ternaire CREE ET POSSEDE par
// `Lane4`. Ce n'est ni un support q3, ni un evenement q3, ni un record lu
// depuis `Lane3`. Qu'il ait trois IDs et soit geometriquement un triangle ne
// lui donne aucune parente avec la lane q3 : q2, q3 et q4 sont TROIS
// ENUMERATIONS INDEPENDANTES, et le rang n'est pas hereditaire. Un `Q4Seed3`
// dont la miniboule propre porte neuf interieurs — donc mort pour q3 — reste
// un prefixe generateur parfaitement valide ici.
//
// ---------------------------------------------------------------------------
// L'IDEE
//
// A `Q4Seed3` fixe, le centre d'une sphere passant par les trois sites n'a plus
// qu'UN degre de liberte : il parcourt la droite normale au prefixe issue de
// son circumcentre. La puissance d'un site le long de cette droite est alors
// AFFINE, donc chaque site a au plus UNE racine, et « peu profond » devient
// « extremal ». Le produit prefixe x apex disparait.
//
// ---------------------------------------------------------------------------
// LA PARAMETRISATION, ET POURQUOI `T2` EST EXACTEMENT JUNG
//
// `Q4Seed3 = (a,b,x)`, arete owner `ab`. Avec `d=b-a`, `u=x-a` :
//
//   D=d.d   E=u.u   F=d.u   G=D E-F^2=|d x u|^2   n=d x u
//   W=E(D-F) d + D(E-F) u
//   c(tau)=a+(W+tau n)/(2 G)
//
// `W` est dans le plan du prefixe et `n` lui est orthogonal, donc `W.n=0` et
//
//   R(tau)^2=(|W|^2+tau^2 G)/(4 G^2).
//
// L'identite ALGEBRIQUE `|W|^2=D G (G+(E-F)^2)` — verifiee terme a terme — donne
//
//   R(tau)^2 <= 3 D/8   <=>   2 tau^2 <= T2,   T2=D (G-2 (E-F)^2).
//
// `J_f={tau : 2 tau^2 <= T2}` n'est donc pas une heuristique : c'est EXACTEMENT
// la borne de Jung en dimension trois pour un support de diametre `|ab|`.
//
// COROLLAIRE, ET IL EST UTILE. Un prefixe owner STRICTEMENT AIGU a toujours
// `T2>0`. En effet `T2>0` equivaut a `sin^2 C > 2/3` ou `C` est l'angle oppose a
// l'arete owner ; celle-ci etant maximale, `C` est le plus grand angle, donc
// `C>=60` degres, et l'acuite donne `C<90`, d'ou `sin^2 C>=3/4>2/3`. La branche
// `T2<=0` est donc INATTEIGNABLE sous acuite owner — elle reste implementee et
// testee sur un prefixe obtus, jamais supprimee.
//
// `J_f` EST FERME. Le tetraedre REGULIER sature Jung a l'egalite : la racine de
// son apex tombe exactement sur le bout. Traiter le bout comme ouvert perd cet
// apex ; c'est le mutant `a8-bouts-ouverts`.
//
// ---------------------------------------------------------------------------
// LA PUISSANCE AFFINE
//
// Pour un site `z`, avec `s=z-a` :
//
//   P_z(tau)=A_z-tau B_z,   A_z=G |s|^2-W.s,   B_z=n.s
//   z est STRICTEMENT interieur a la sphere de parametre `tau`  <=>  P_z(tau)<0
//
//   B_z>0 : interieur exactement pour tau > rho_z=A_z/B_z    (ENTRANT)
//   B_z<0 : interieur exactement pour tau < rho_z            (SORTANT)
//   B_z=0 : interieur partout si A_z<0, jamais si A_z>0, SHELL PERSISTANT si A_z=0
//
// Aucune racine n'est jamais formee en flottant : toutes les comparaisons sont
// des signes de polynomes entiers.
//
// ---------------------------------------------------------------------------
// LE THEOREME, PARAMETRE PAR `r4`
//
// `r4=smax-3` est le premier nombre d'interieurs qui REJETTE un support q4 ; a
// `smax=11`, `r4=8`. Masquer les trois IDs du `Q4Seed3` et noter `p` le nombre
// de sites strictement interieurs sur TOUT `J_f` — les `B=0,A<0`, les entrants
// strictement avant le bout gauche, les sortants strictement apres le bout
// droit. Si `p>=r4`, aucune completion pertinente n'existe. Sinon `k=r4-p` et
//
//   First_k = les `k` plus petites racines parmi `B>0`
//   Last_k  = les `k` plus grandes racines parmi `B<0`
//
// Tout apex regulier dont la circumsphere porte moins de `r4` interieurs est
// dans `First_k union Last_k`, donc
//
//   candidate_root_groups <= 2 (r4 - p) <= 2 (smax - 3).
//
// PREUVE. Un apex de signe `B>0` absent de `First_k` a deja `k` entrants
// strictement anterieurs ; avec les `p` permanents sa profondeur atteint `r4`.
// Le cas `B<0` est symetrique. Une egalite est un shell et ne credite jamais
// l'interieur. Ni probabilite, ni voisinage metrique, ni Delaunay.
//
// LE MEME NOYAU DECIDE LA MORT PAR GAPS, ET EXACTEMENT. La profondeur minoree
// par les seuls extremes retenus vaut la vraie profondeur partout ou celle-ci
// est inferieure a `r4` : un contributeur non retenu impliquerait deja `k`
// extremes interieurs. Verifier `min >= r4` sur `J_f` est donc une DECISION,
// pas une heuristique, et son recu tient en `p + 2(r4-p) = 2 r4 - p` IDs.
//
// ---------------------------------------------------------------------------
// LARGEURS PROUVEES, PROFIL u16
//
// Avec `U=65535` : `D,E,|F| <= 3U^2 < 2^34`, `G <= 9U^4 < 2^68`,
// `|n_i| <= 2U^2 < 2^33`, `|W_i| < 2^86`, `|A| < 2^104`, `|B| < 2^51`.
//   comparaison de racines  : `A_1 B_2` vs `A_2 B_1`      < 2^155
//   appartenance a `J_f`    : `2 A^2` vs `T2 B^2`          < 2^209
// `i128` ne suffit donc PAS : les deux comparaisons passent par `BigInt<4>`.
//
// ---------------------------------------------------------------------------
// HOTE ET DEVICE PARTAGENT CE FICHIER, ET C'EST LA CONDITION DE LA PARITE
//
// Toutes les fonctions numeriques portent `MHGP_HD`. Le kernel CUDA
// n'implemente AUCUNE geometrie : il appelle `select_axis_topr4`, exactement la
// fonction que l'hote compile. Sans cela, une comparaison bit a bit
// comparerait deux implementations et non deux executions.
#pragma once

#include <cstdint>

#include "mhgp/mhgp.hpp"

namespace mhgp3v {
namespace q4axis {

using mhgp::i128;
using mhgp::i64;
using B4 = mhgp::BigInt<4>;

// ---------------------------------------------------------------------------
// MUTANTS. Chacun doit etre TUE par une porte nommee.
// ---------------------------------------------------------------------------
enum class Mutant {
  kNone,
  kCapMoinsUn,        // plafonne la selection a `2 r4 - 1` racines
  kKFixeR4MoinsUn,    // `k=r4-1` au lieu de `k=r4-p`
  kSigneBInverse,     // garde les plus petites racines pour `B<0`
  kBoutsOuverts,      // traite `J_f` comme OUVERT : perd l'apex qui sature Jung
  kPermanenceLarge,   // `<=` au lieu de `<` sur la permanence : gonfle `p`
  kOublieBZero,       // ignore les permanents `B=0, A<0`
  kAbsAvantTri,       // trie sur `|B|`, donc perd l'orientation
  kShellCompteInterieur,  // credite le shell persistant comme interieur
  kGapLarge,          // `>` au lieu de `>=` sur la mort par gaps
};

inline const char* mutant_name(Mutant m) {
  switch (m) {
    case Mutant::kNone: return "none";
    case Mutant::kCapMoinsUn: return "a8-cap-moins-un";
    case Mutant::kKFixeR4MoinsUn: return "a8-k-fixe-r4-moins-un";
    case Mutant::kSigneBInverse: return "a8-signe-b-inverse";
    case Mutant::kBoutsOuverts: return "a8-bouts-ouverts";
    case Mutant::kPermanenceLarge: return "a8-permanence-large";
    case Mutant::kOublieBZero: return "a8-oublie-b-zero";
    case Mutant::kAbsAvantTri: return "a8-abs-avant-tri";
    case Mutant::kShellCompteInterieur: return "a8-shell-compte-interieur";
    case Mutant::kGapLarge: return "a8-gap-large";
  }
  return "?";
}

// ---------------------------------------------------------------------------
// L'AXE D'UN `Q4Seed3`.
// ---------------------------------------------------------------------------
struct SeedAxis {
  i64 a[3] = {0, 0, 0};
  i128 D = 0, E = 0, F = 0, G = 0;
  i128 n[3] = {0, 0, 0};
  i128 W[3] = {0, 0, 0};
  i128 T2 = 0;
  bool reguliere = false;      // `G>0` : les trois sites ne sont pas alignes
  bool jung_ouverte = false;   // `T2>0` : le segment n'est pas reduit a un point
};

MHGP_HD inline SeedAxis seed_axis(const i64 a[3], const i64 b[3], const i64 x[3]) {
  SeedAxis f;
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

// Le prefixe est-il STRICTEMENT aigu ? Les trois produits scalaires de coin.
MHGP_HD inline bool seed_aigu(const i64 a[3], const i64 b[3], const i64 x[3]) {
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

MHGP_HD inline SitePower site_power(const SeedAxis& f, const i64 z[3]) {
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
MHGP_HD inline int sgn_2AA_moins_T2YY(i128 A, i128 Y, i128 T2) {
  const B4 aa = mhgp::mul128(A, A);
  const B4 aa2 = mhgp::big_add(aa, aa);
  const B4 yy = mhgp::mul128(T2, Y * Y);
  return mhgp::big_cmp(aa2, yy);
}

// signe de `A - Y tau_max` avec `tau_max^2 = T2/2`, `T2>0`.
MHGP_HD inline int sgn_A_moins_Ytau(i128 A, i128 Y, i128 T2) {
  if (A == 0 && Y == 0) return 0;
  if (A >= 0 && Y <= 0) return 1;
  if (A <= 0 && Y >= 0) return -1;
  const int c = sgn_2AA_moins_T2YY(A, Y, T2);
  return (A > 0) ? c : -c;   // `A<0` et `Y<0` : le signe s'inverse
}

// signe de `rho_z - eps tau_max`, `eps` valant `+1` ou `-1`. Suppose `B!=0`.
MHGP_HD inline int cmp_racine_bout(const SitePower& p, i128 T2, int eps) {
  const int s = sgn_A_moins_Ytau(p.A, p.B * eps, T2);
  return (p.B > 0) ? s : -s;
}

// signe de `rho_1 - rho_2`. Suppose `B_1!=0` et `B_2!=0`.
MHGP_HD inline int cmp_racines(const SitePower& p, const SitePower& q) {
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
  kJamais,           // interieur nulle part sur `J_f`, et racine hors de `J_f`
  kEntrant,          // `B>0`, racine dans `J_f`
  kSortant,          // `B<0`, racine dans `J_f`
  kShellPersistant,  // `B=0` et `A=0` : cospherique sur tout l'axe
};

MHGP_HD inline SiteClass classify(const SitePower& p, i128 T2, Mutant mut = Mutant::kNone) {
  if (p.B == 0) {
    if (p.A == 0) return SiteClass::kShellPersistant;
    if (p.A < 0) {
      if (mut == Mutant::kOublieBZero) return SiteClass::kJamais;
      return SiteClass::kPermanent;
    }
    return SiteClass::kJamais;
  }
  const int gauche = cmp_racine_bout(p, T2, -1);   // signe de rho + tau_max
  const int droite = cmp_racine_bout(p, T2, +1);   // signe de rho - tau_max
  // La permanence exige le bout STRICT : a `rho = -tau_max` le site est SUR la
  // sphere du bout gauche, pas dedans. La candidature, elle, admet le bout
  // ferme : c'est le tetraedre regulier.
  const bool ouvert = (mut == Mutant::kBoutsOuverts);
  const bool perm_large = (mut == Mutant::kPermanenceLarge);
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
// SELECTION EXTREMALE, AVEC LES VRAIS `PointId`.
//
// Capacites fixes et DEBORDEMENT EXPLICITE : jamais de troncature silencieuse.
// Les groupes d'egalite sont conserves entiers, donc une concurrence peut
// depasser `2k` ; c'est alors un refus, pas un choix.
// ---------------------------------------------------------------------------
inline constexpr int kCapRacines = 64;
inline constexpr int kCapPermanents = 32;
inline constexpr int kCapShell = 32;

enum class SeedVerdict {
  kOuvert,           // au moins un apex candidat subsiste
  kMortDegeneree,    // `G<=0` : les trois sites sont alignes
  kMortT2,           // `T2<=0` : aucune completion strictement positive
  kMortPermanents,   // `p>=r4`
  kMortGap,          // profondeur minoree `>= r4` partout sur `J_f`
  kDebordement,      // concurrence au-dela de la capacite : fail-closed
};

inline const char* verdict_name(SeedVerdict v) {
  switch (v) {
    case SeedVerdict::kOuvert: return "OUVERT";
    case SeedVerdict::kMortDegeneree: return "MORT_DEGENEREE";
    case SeedVerdict::kMortT2: return "MORT_T2";
    case SeedVerdict::kMortPermanents: return "MORT_PERMANENTS";
    case SeedVerdict::kMortGap: return "MORT_GAP";
    case SeedVerdict::kDebordement: return "DEBORDEMENT";
  }
  return "?";
}

struct Selection {
  SeedVerdict verdict = SeedVerdict::kOuvert;
  int r4 = 8;
  int p = 0, k = 0;
  int n_perm = 0, n_shell = 0, n_entrants = 0, n_sortants = 0;
  int permanents[kCapPermanents] = {0};
  int shell[kCapShell] = {0};
  int entrants[kCapRacines] = {0};
  int sortants[kCapRacines] = {0};
  int profondeur_min_minoree = 0;   // recu de la mort par gaps
};

// `sites` : vrais `PointId`, les trois du `Q4Seed3` DEJA masques par l'appelant.
// `pw(i)` rend la `SitePower` du site `i`.
template <class PowFn>
MHGP_HD inline Selection select_axis_topr4(const SeedAxis& f, const int* sites, int n_sites,
                                   PowFn pw, int r4 = 8, Mutant mut = Mutant::kNone) {
  Selection sel;
  sel.r4 = r4;
  if (!f.reguliere) { sel.verdict = SeedVerdict::kMortDegeneree; return sel; }
  if (f.T2 <= 0) { sel.verdict = SeedVerdict::kMortT2; return sel; }

  // Passe 1 : les permanents et le shell persistant, AVEC leurs identites.
  for (int i = 0; i < n_sites; ++i) {
    const int id = sites[i];
    const SiteClass c = classify(pw(id), f.T2, mut);
    if (c == SiteClass::kPermanent) {
      if (sel.n_perm < kCapPermanents) sel.permanents[sel.n_perm] = id;
      ++sel.n_perm;
    } else if (c == SiteClass::kShellPersistant) {
      if (sel.n_shell >= kCapShell) { sel.verdict = SeedVerdict::kDebordement; return sel; }
      sel.shell[sel.n_shell++] = id;
    }
  }
  sel.p = sel.n_perm;
  if (sel.p >= r4) { sel.verdict = SeedVerdict::kMortPermanents; return sel; }
  if (sel.n_perm > kCapPermanents) { sel.verdict = SeedVerdict::kDebordement; return sel; }
  sel.k = (mut == Mutant::kKFixeR4MoinsUn) ? (r4 - 1) : (r4 - sel.p);
  const int cap_total = (mut == Mutant::kCapMoinsUn) ? (2 * r4 - 1) : (2 * kCapRacines);

  // Passe 2 : `First_k` et `Last_k`, groupes d'egalite complets.
  //
  // LA SEMANTIQUE EST INCHANGEE, LE COUT NE L'EST PAS. La regle reste « garder
  // `z` si strictement moins de `k` sites lui sont strictement meilleurs », ce
  // qui equivaut a « garder tout site dont la racine est au plus la `k`-ieme
  // plus petite, multiplicites comprises » — donc les groupes d'egalite entiers.
  // Une premiere passe retient les `k` meilleures racines dans un tableau trie
  // borne, une seconde collecte tout ce qui atteint ce seuil : `O(m k)` au lieu
  // de `O(m^2)`. A `smax=6`, `k <= 3` : le gain est d'emblee de deux ordres.
  for (int pass = 0; pass < 2; ++pass) {
    const bool entrant = (pass == 0);
    int* out = entrant ? sel.entrants : sel.sortants;
    int& cnt = entrant ? sel.n_entrants : sel.n_sortants;
    bool garde_petites = entrant;
    if (!entrant && mut == Mutant::kSigneBInverse) garde_petites = true;
    const SiteClass vise = entrant ? SiteClass::kEntrant : SiteClass::kSortant;
    auto mieux = [&](const SitePower& a, const SitePower& b) {
      int cr = cmp_racines(a, b);
      if (mut == Mutant::kAbsAvantTri && b.B < 0) cr = -cr;
      return garde_petites ? (cr < 0) : (cr > 0);
    };
    // Passe 2a : le seuil, c'est-a-dire la `k`-ieme meilleure racine.
    SitePower seuil[kCapRacines];
    int ns = 0;
    bool seuil_atteint = false;
    for (int i = 0; i < n_sites; ++i) {
      const SitePower p = pw(sites[i]);
      if (classify(p, f.T2, mut) != vise) continue;
      int pos = ns;
      while (pos > 0 && mieux(p, seuil[pos - 1])) --pos;
      if (pos >= sel.k) continue;
      const int fin = (ns < sel.k) ? ns : sel.k - 1;
      for (int t = fin; t > pos; --t) seuil[t] = seuil[t - 1];
      seuil[pos] = p;
      if (ns < sel.k) ++ns;
      if (ns >= sel.k) seuil_atteint = true;
    }
    // Passe 2b : tout site atteignant le seuil, groupes d'egalite compris.
    for (int i = 0; i < n_sites; ++i) {
      const int id = sites[i];
      const SitePower p = pw(sites[i]);
      if (classify(p, f.T2, mut) != vise) continue;
      if (seuil_atteint && mieux(seuil[ns - 1], p)) continue;
      if (sel.n_entrants + sel.n_sortants >= cap_total || cnt >= kCapRacines) {
        sel.verdict = SeedVerdict::kDebordement;
        return sel;
      }
      out[cnt++] = id;
    }
  }

  // Passe 3 : LA MORT PAR GAPS, DECIDEE, PAS DEVINEE.
  //
  // Le predicat tronque est EXACT :
  //   depth(tau) >= r4  <=>  p + min(k, N_in(rho<tau)) + min(k, N_out(rho>tau)) >= r4
  // ou `N_in`/`N_out` comptent sur les seuls extremes retenus. En effet un
  // contributeur non retenu implique deja `k` extremes du meme signe strictement
  // du bon cote, donc la troncature a `k` ne perd rien. Une egalite est un
  // shell et ne credite jamais l'interieur.
  //
  // Il suffit d'evaluer sur l'ordre fusionne des racines retenues : le bout
  // gauche, chaque racine ELLE-MEME, et l'intervalle qui la suit.
  {
    const int nr = sel.n_entrants + sel.n_sortants;
    auto racine = [&](int u) {
      return pw((u < sel.n_entrants) ? sel.entrants[u] : sel.sortants[u - sel.n_entrants]);
    };
    auto borne = [&](int v) { return v < sel.k ? v : sel.k; };
    // `strict` : on evalue AU parametre d'une racine (le groupe egal est shell).
    // `apres` : on evalue juste apres (le groupe egal est deja entre).
    auto profondeur = [&](const SitePower* tau, bool apres) {
      int nin = 0, nout = 0;
      for (int t = 0; t < sel.n_entrants; ++t) {
        const int c = tau ? cmp_racines(pw(sel.entrants[t]), *tau)
                          : cmp_racine_bout(pw(sel.entrants[t]), f.T2, -1);
        if (tau ? (apres ? c <= 0 : c < 0) : (c < 0)) ++nin;
      }
      for (int t = 0; t < sel.n_sortants; ++t) {
        const int c = tau ? cmp_racines(pw(sel.sortants[t]), *tau)
                          : cmp_racine_bout(pw(sel.sortants[t]), f.T2, -1);
        if (c > 0) ++nout;
      }
      return sel.p + borne(nin) + borne(nout);
    };
    int minimum = profondeur(nullptr, false);        // bout gauche ferme
    for (int u = 0; u < nr; ++u) {
      const SitePower r = racine(u);
      const int a1 = profondeur(&r, false);
      const int a2 = profondeur(&r, true);
      if (a1 < minimum) minimum = a1;
      if (a2 < minimum) minimum = a2;
    }
    sel.profondeur_min_minoree = minimum;
    const bool morte = (mut == Mutant::kGapLarge) ? (minimum > r4) : (minimum >= r4);
    if (morte) sel.verdict = SeedVerdict::kMortGap;
  }
  return sel;
}

// ---------------------------------------------------------------------------
// LE REPLAY DU CENSUS. Il rend les VRAIES identites `I_B` et `U_B`, pas des
// cardinaux. Le groupe d'egalite de la racine est RANGE-REPORTE : en production
// c'est une requete bornee sur l'octree, ici un balayage compte comme tel.
// ---------------------------------------------------------------------------
// LES PRECONDITIONS SONT DANS L'API, PAS DANS UN COMMENTAIRE. La preuve du
// replay ne porte QUE sur un apex retenu de profondeur strictement inferieure a
// `r4`, sur une selection non debordee. Appele hors de ce domaine, le replay
// peut legitimement omettre des non-extremes : il rend alors `kHorsDomaine` et
// ne pretend a aucun census exact.
// LE PROFIL DECIDE CE QU'UNE EGALITE VAUT. Sous `RelevantGP`, toute egalite
// hors support et tout shell persistant rendent `unsupported_degeneracy` : un
// compteur n'est pas un fate. Hors de ce profil, une politique de plateau
// DECLAREE publie la liste complete, et la capacite prouvee doit suffire.
enum class Profil { kRelevantGp, kPlateau };

enum class CensusFate {
  kExact,                    // `I_B` et `U_B` sont complets et consommables
  kPendingCap,               // capacite depassee : le compte requis est publie
  kUnsupportedDegeneracy,    // egalite hors support ou shell persistant
  kHorsDomaine,              // precondition violee : rien n'est publie
};

inline const char* census_fate_name(CensusFate f) {
  switch (f) {
    case CensusFate::kExact: return "EXACT";
    case CensusFate::kPendingCap: return "PENDING_CAP";
    case CensusFate::kUnsupportedDegeneracy: return "UNSUPPORTED_DEGENERACY";
    case CensusFate::kHorsDomaine: return "HORS_DOMAINE";
  }
  return "?";
}

// CAPACITE PROUVEE. `I_B` : au plus `kCapPermanents` permanents plus les deux
// cotes retenus. `U_B` : les trois IDs du seed, le shell persistant, et le
// groupe d'egalite de la racine — lequel est INCLUS dans les groupes retenus
// des deux cotes, jamais au-dela.
inline constexpr int kCapInterieur = kCapPermanents + 2 * kCapRacines;
inline constexpr int kCapShellTotal = 3 + kCapShell + 2 * kCapRacines;

struct Census {
  CensusFate fate = CensusFate::kExact;
  int n_interieur = 0, n_shell = 0;
  int requis_interieur = 0, requis_shell = 0;   // comptes VOULUS, meme si tronques
  int interieur[kCapInterieur] = {0};
  int shell[kCapShellTotal] = {0};
  bool degenere = false;     // groupe d'egalite non trivial ou shell persistant
  long long range_reports = 0;
};

// `sel` doit etre `kOuvert` ou `kMortGap` — jamais `kDebordement` — et `apex`
// doit etre l'un des extremes retenus. Le shell est reconstruit DEPUIS LES
// GROUPES RETENUS : pour un apex deja prouve shallow, tout site de racine egale
// appartient necessairement a ces groupes complets, donc aucun second balayage
// global n'est requis. C'est la reparation preferable de l'auditeur.
template <class PowFn>
MHGP_HD inline Census census_replay(const Selection& sel, int apex, const int seed3[3],
                            PowFn pw, Mutant mut = Mutant::kNone,
                            Profil profil = Profil::kRelevantGp) {
  Census c;
  // LE DOMAINE DE LA PREUVE, IMPOSE ET NON COMMENTE. Le replay n'est exact que
  // pour un apex SHALLOW d'un seed OUVERT. `MORT_GAP` est aussi hors domaine :
  // le seed est ferme, aucun apex n'y est pertinent, et publier un census y
  // serait publier un objet inexistant.
  if (sel.verdict != SeedVerdict::kOuvert) {
    c.fate = CensusFate::kHorsDomaine;
    return c;
  }
  {
    bool retenu = false;
    for (int t = 0; t < sel.n_entrants && !retenu; ++t) retenu = (sel.entrants[t] == apex);
    for (int t = 0; t < sel.n_sortants && !retenu; ++t) retenu = (sel.sortants[t] == apex);
    if (!retenu) { c.fate = CensusFate::kHorsDomaine; return c; }
  }
  const SitePower pa = pw(apex);
  auto pousse_int = [&](int id) {
    ++c.requis_interieur;
    if (c.n_interieur < kCapInterieur) c.interieur[c.n_interieur++] = id;
    else c.fate = CensusFate::kPendingCap;
  };
  auto pousse_shell = [&](int id) {
    ++c.requis_shell;
    if (c.n_shell < kCapShellTotal) c.shell[c.n_shell++] = id;
    else c.fate = CensusFate::kPendingCap;
  };
  // `I_B` = permanents, entrants retenus strictement anterieurs, sortants
  // retenus strictement posterieurs.
  if (sel.n_perm > kCapPermanents) c.fate = CensusFate::kPendingCap;
  for (int t = 0; t < sel.n_perm && t < kCapPermanents; ++t) pousse_int(sel.permanents[t]);
  for (int t = 0; t < sel.n_entrants; ++t) {
    const int z = sel.entrants[t];
    if (z != apex && cmp_racines(pw(z), pa) < 0) pousse_int(z);
  }
  for (int t = 0; t < sel.n_sortants; ++t) {
    const int z = sel.sortants[t];
    if (z != apex && cmp_racines(pw(z), pa) > 0) pousse_int(z);
  }
  // UN APEX PROFOND EST HORS DOMAINE, ET LE REPLAY LE DECOUVRE LUI-MEME. La
  // preuve « tout interieur est dans les extremes retenus » suppose la
  // profondeur strictement inferieure a `r4` ; au-dela, des non-extremes
  // peuvent manquer et la liste serait fausse sans le dire.
  if (c.requis_interieur >= sel.r4) {
    Census hd;
    hd.fate = CensusFate::kHorsDomaine;
    hd.requis_interieur = c.requis_interieur;
    return hd;
  }
  // `U_B` = seed, shell persistant, groupe d'egalite de la racine.
  for (int t = 0; t < 3; ++t) pousse_shell(seed3[t]);
  for (int t = 0; t < sel.n_shell; ++t) {
    pousse_shell(sel.shell[t]);
    c.degenere = true;
    if (mut == Mutant::kShellCompteInterieur) pousse_int(sel.shell[t]);
  }
  for (int pass = 0; pass < 2; ++pass) {
    const int cnt = pass ? sel.n_sortants : sel.n_entrants;
    const int* tab = pass ? sel.sortants : sel.entrants;
    for (int t = 0; t < cnt; ++t) {
      ++c.range_reports;
      if (cmp_racines(pw(tab[t]), pa) != 0) continue;
      pousse_shell(tab[t]);
      if (tab[t] != apex) c.degenere = true;
    }
  }
  if (c.degenere && profil == Profil::kRelevantGp &&
      c.fate == CensusFate::kExact)
    c.fate = CensusFate::kUnsupportedDegeneracy;
  return c;
}

}  // namespace q4axis
}  // namespace mhgp3v
