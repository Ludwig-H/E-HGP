// MorseHGP3D v4 — FLUX DE BOULES : les trois lanes WSPD comme generateurs,
// puis sort/RLE par BallKey et UN census par cle (l'ABI SpherePlateau de
// l'audit bloquant, version echelle).
//
// COMPLETUDE SOUS LES SEUILS h_q (derive_v4) : un plateau pertinent pour la
// foret (∃ σ = I_B ∪ T, |σ| <= K_max+1 = 11) dont le support minimal est
// d'arite q verifie |T| >= q donc |I_B| <= 11 - 1 - (q - 1) - ... plus
// simplement : |I_B| <= K_max + 1 - q ; ses temoins de fuseau
// W_q(a,b) ⊆ I_B sont donc au plus K_max + 1 - q - 1 < h_q = s_max - q + 1
// (s_max = K_max + 1). L'ancre du support minimal SURVIT toujours aux
// filtres h_coeur/h_a/h_b : les seuils du profil sont exactement calibres
// pour ne perdre aucun plateau pertinent. (Carathéodory garantit un support
// minimal d'arite 2, 3 ou 4 dans U_B — audit § 2.)
//
// CENSUS PAR CLE : la forme primitive (A, B, C) donne le predicat UNIFORME
// P(z) = A·|z|² + B·z + C (< 0 interieur strict, = 0 coquille), lanes
// confondues. Largeurs u16 : A < 2^68, |B_i| < 2^86, |C| < 2^104 ;
// A|z|² < 2^102, |B·z| < 2^105 → i128. Descente d'arbre separable par axe
// (parabole convexe, minimum de reseau aux entiers voisins du sommet).
//
// NIVEAU CANONIQUE PAR BOULE : au RLE, le representant de niveau retenu est
// celui du generateur d'ARITE MINIMALE (puis plus petite representation) —
// q2/q3 donnent des fractions canoniques, q4 un representant (|N'|², det²) ;
// sujet et juge appliquent la meme regle, les representants coincident.
#pragma once

#include <algorithm>
#include <atomic>
#include <cfenv>
#include <chrono>
#include <cmath>
#include <limits>
#include <thread>
#include <vector>

#include "../events/acute_seed.hpp"
#include "../events/edge_cover.hpp"
#include "../events/q2_event.hpp"
#include "../events/q4_event.hpp"
#include "../events/witness_count.hpp"
#include "../wspd/wavefront.hpp"

namespace mhgp4 {

struct BallCandidate {
  Q3BallKey key;
  Q4Level level;
  u8 arity;  // arite du generateur (2, 3, 4)
};

inline bool ball_candidate_less(const BallCandidate& x, const BallCandidate& y) {
  if (!(x.key == y.key)) return x.key < y.key;
  if (x.arity != y.arity) return x.arity < y.arity;
  return x.level < y.level;  // representation : depart deterministe
}

// Statistiques du flux (compteurs, jamais une autorite).
struct BallStreamStats {
  u64 rect_alive[3] = {0, 0, 0};
  u64 anchors[3] = {0, 0, 0};
  u64 candidates[3] = {0, 0, 0};
  u64 unique_balls = 0;
  u64 census_interior = 0;
  u64 census_shell = 0;
  u64 balls_dead_depth = 0;  // |I_B| >= h_qmin : aucun K <= K_max
  // Passe count-only (audit « prefiltre exact par boule ») : ses couts,
  // separes de ceux du census complet — un gain de census ne doit jamais
  // dissimuler un tri ou une passe qui mange le gain.
  u64 prefilter_nodes = 0;
  u64 prefilter_leaf_tests = 0;
  u64 prefilter_range_add_mass = 0;
  u64 full_census_keys = 0;
  // Filtre de profondeur A LA GENERATION (par lane) : candidats tues par
  // le minorant certifie sur le cover, AVANT toute emission.
  u64 gen_depth_killed[3] = {0, 0, 0};
  // Ancres q4 tuees par le compte W_4 exact (reponse auditeur « minorant
  // q4 et axial streaming » § 1) : W_4 est le plus grand cœur anchor-only
  // (owner + Jung) — n4 >= h_4 rend l'ancre inutile AVANT seed × completion.
  u64 anchors_killed_w4 = 0;
  // Selection axiale bornee (compteurs exiges par l'audit « axial borne ») :
  // seeds traites, sites balayes, seeds morts par permanents (p >= h_4),
  // groupes de mu emis.
  u64 axial_seeds = 0;
  u64 axial_sites = 0;
  u64 axial_seeds_dead_perm = 0;
  u64 axial_groups_emitted = 0;
  // Sweep a DEUX COTES — compteurs a UNITES SEPAREES (audit « sweep reçu
  // et kernel sans alloc » § 2 : ne jamais additionner des racines et des
  // groupes). Racines croisees : positives sous L / negatives sur U
  // (>= k temoins du cote OPPOSE => d >= h_4 exact) — les exclusions par
  // leur propre cote ne comptent pas, elles existaient dans le sweep
  // unilateral. Groupes : en fenetre [L,U], et tues par d_j >= h_4 AVANT
  // valid_completion/q4_form. Appels : volume reel de valid_completion.
  u64 axial_roots_pruned_cross = 0;
  u64 axial_groups_in_window = 0;
  u64 axial_groups_killed_depth = 0;
  u64 axial_completion_calls = 0;
  u64 axial_verify_mismatch = 0;
  // Cœur universel du seed (audit « axial arbre et cœur de seed » § 1) :
  // seeds tues par >= h_4 temoins seed-universels (ou J < 0 : aucun
  // tetraedre admissible), et sites du cover examines par le compte.
  u64 seeds_killed_seed_core = 0;
  u64 seed_core_sites = 0;
  // Etage flottant certifie du SIGNE de P (audit « filtre flottant »
  // § 1.1) : signes certifies par la borne 2^58, replis exacts, et
  // desaccords de la reception kFloatVerify (signe certifie recoupe par
  // l'exact — toujours 0).
  u64 float_cert_neg = 0;
  u64 float_cert_pos = 0;
  u64 float_fallback = 0;
  u64 float_mismatch = 0;
  // Etage d'INTERVALLES de Jung (audit § 1.2 / contre-audit 04c71a2
  // § 6) : sur un site certifie P < 0, si [2P²] et [J][B²] se separent,
  // temoin certifie (kill) ou non-temoin certifie (skip) SANS exact ;
  // sinon repli cmp_2p2_jb2. Les certifications sont recoupees sous
  // kFloatVerify (desaccords dans float_mismatch).
  u64 jung_cert_kill = 0;
  u64 jung_cert_skip = 0;
  u64 jung_fallback = 0;
  // Decomposition de t_gen exigee par l'audit § 3 (chemin axial) :
  // cœur de seed, materialisation A,B des sites, reduction a deux cotes
  // (seuils + groupes + prefixes + d_j), emission (valid_completion +
  // q4_form + verify). En millisecondes ; le cœur est aussi chronometre
  // sur le chemin baseline.
  double t_seed_core_ms = 0, t_axial_ab_ms = 0, t_reduce_ms = 0,
         t_emit_ms = 0;
  // Fusion des statistiques d'un ouvrier parallele : addition membre a
  // membre — les champs non touches par la generation valent zero chez
  // l'ouvrier, l'addition est donc toujours sure. Les chronos deviennent
  // du temps CPU CUMULE (somme des fils), plus du temps mural.
  void add_from(const BallStreamStats& o) {
    for (int i = 0; i < 3; ++i) {
      rect_alive[i] += o.rect_alive[i];
      anchors[i] += o.anchors[i];
      candidates[i] += o.candidates[i];
      gen_depth_killed[i] += o.gen_depth_killed[i];
    }
    unique_balls += o.unique_balls;
    census_interior += o.census_interior;
    census_shell += o.census_shell;
    balls_dead_depth += o.balls_dead_depth;
    prefilter_nodes += o.prefilter_nodes;
    prefilter_leaf_tests += o.prefilter_leaf_tests;
    prefilter_range_add_mass += o.prefilter_range_add_mass;
    full_census_keys += o.full_census_keys;
    anchors_killed_w4 += o.anchors_killed_w4;
    axial_seeds += o.axial_seeds;
    axial_sites += o.axial_sites;
    axial_seeds_dead_perm += o.axial_seeds_dead_perm;
    axial_groups_emitted += o.axial_groups_emitted;
    axial_roots_pruned_cross += o.axial_roots_pruned_cross;
    axial_groups_in_window += o.axial_groups_in_window;
    axial_groups_killed_depth += o.axial_groups_killed_depth;
    axial_completion_calls += o.axial_completion_calls;
    axial_verify_mismatch += o.axial_verify_mismatch;
    seeds_killed_seed_core += o.seeds_killed_seed_core;
    seed_core_sites += o.seed_core_sites;
    float_cert_neg += o.float_cert_neg;
    float_cert_pos += o.float_cert_pos;
    float_fallback += o.float_fallback;
    float_mismatch += o.float_mismatch;
    jung_cert_kill += o.jung_cert_kill;
    jung_cert_skip += o.jung_cert_skip;
    jung_fallback += o.jung_fallback;
    t_seed_core_ms += o.t_seed_core_ms;
    t_axial_ab_ms += o.t_axial_ab_ms;
    t_reduce_ms += o.t_reduce_ms;
    t_emit_ms += o.t_emit_ms;
  }
};

// Drapeaux du chemin axial (mutants + verification de reception).
enum : u32 {
  kAxialShortGroup = 1u,     // MUTANT : k-1 (prefixe trop court)
  kAxialDropTies = 2u,       // MUTANT : ties de frontiere supprimes
  kAxialFirstRep = 4u,       // MUTANT : premier representant valide
  kAxialIgnoreOpposite = 8u, // MUTANT : d_j sans le cote oppose
  kAxialDepthNonstrict = 16u,// MUTANT : le groupe compte sa propre coquille
  kAxialReverseNeg = 32u,    // MUTANT : suffixe negatif remplace par prefixe
  kAxialVerify = 64u,        // reception : d_j recoupe par le scan complet
  kAxialSeedCoreNonstrict = 128u,  // MUTANT : egalites comptees au cœur
  kFloatSmallThreshold = 256u,  // MUTANT : borne flottante trop petite
  kFloatVerify = 512u,          // reception : signes certifies recoupes
  kFloatIgnoreRounding = 1024u,  // MUTANT : garde d'arrondi ignoree
};

// Site axial d'un seed : A = puissance q3, B = normale·(z−a), mu = A/B.
struct AxialSite {
  i128 A;
  i64 B;
  i32 u;
};

// Comparaison exacte de mu = A/B entre sites du MEME cote (B1, B2 > 0
// exiges — l'appelant normalise) : signe de A1·B2 − A2·B1. Largeurs u16 :
// |A| < 2^107 (puissance q3), |B| < 2^54 (normale × ecart) — produits
// croises < 2^161 < 2^192, U192 suffit.
MHGP4_HD inline int cmp_mu_same_side(i128 A1, i64 B1, i128 A2, i64 B2) {
  const int s1 = A1 < 0 ? -1 : (A1 > 0 ? 1 : 0);
  const int s2 = A2 < 0 ? -1 : (A2 > 0 ? 1 : 0);
  if (s1 != s2) return s1 < s2 ? -1 : 1;
  if (s1 == 0) return 0;
  const U192 m1 = mul_level_192(detail_ev::uabs(A1), (u128)(u64)B2);
  const U192 m2 = mul_level_192(detail_ev::uabs(A2), (u128)(u64)B1);
  const int c = cmp_u192(m1, m2);
  return s1 > 0 ? c : -c;
}

// Comparaison exacte 2·P² <=> J·B² (cœur de seed). Sous u16 les produits
// atteignent ~212 bits (audit § 1) : U320, jamais i128. Preconditions :
// P <= 0, J >= 0 ; produits < 2^260 (contrat de mul_192_128_to_320).
MHGP4_HD inline int cmp_2p2_jb2(i128 P, i128 J, i64 B) {
  const u128 ap = (u128)(-P);
  const u64 pw[3] = {(u64)ap, (u64)(ap >> 64), 0};
  U320 lhs = mul_192_128_to_320(pw, ap);  // P² < 2^209
  u64 carry = 0;                          // ×2 : decalage d'un bit, sans
  for (int i = 0; i < 5; ++i) {           // debordement (2P² < 2^210)
    const u64 nc = lhs.w[i] >> 63;
    lhs.w[i] = (lhs.w[i] << 1) | carry;
    carry = nc;
  }
  const u128 ju = (u128)J;
  const u64 jw[3] = {(u64)ju, (u64)(ju >> 64), 0};
  const u128 b2 = (u128)((i128)B * B);
  return cmp_u320(lhs, mul_192_128_to_320(jw, b2));  // J·B² < 2^210
}

// ---- ETAGE FLOTTANT CERTIFIE DU SIGNE DE P — FORME AFFINE PAR ANCRE
// (audits « filtre flottant » § 1.1 puis « filtre certifie et niveaux
// q3 » § 1 + 3). L'etage vit DANS les lanes q3 et q4 (memes cinq fma,
// donnees partagees) ; conditions GRAVEES : round-to-nearest, jamais de
// fast-math, sequence FIGEE
//   t  = fma(N2, u2, fma(N1, u1, N0·u0)) ;
//   L^ = fma(G, q, −(t + t)) ;
// sur les SITES AFFINES de l'ancre u = 2z−a−b, q = |u|²−D2 (entiers
// < 2^36, donc EXACTS en binaire64 — l'erreur de conversion des sites
// disparait), et le seed N = W − G·d (|N| < 2^87), G < 2^68. Seules les
// conversions de G et N (ulp) et les cinq arrondis contribuent, toutes
// RELATIVES aux grandeurs : |L^ − L| <= ~8·2^-53·(G·|q| + 2·Σ|N_i u_i|),
// majore une fois PAR SEED par E = 2^-48·(G_d·qmax + 2·|N|_1·umax) avec
// qmax/umax les majorants de l'ancre (calcules par fill_affine_sites).
// Le facteur 2^-48 laisse ×4 de marge et absorbe les arrondis du calcul
// de E lui-meme — volontairement LACHE : des replis exacts, jamais une
// fausse decision. Decision : L^ < −E => L < 0 certifie ; L^ > +E =>
// L > 0 certifie ; sinon repli AFFINE EXACT i128 (|L| < 2^105), et
// P = L/4 est exact (identite L = 4·q3_power et divisibilite gravees par
// la porte --q3-affine-gate). Jung (2P² vs JB²) et cmp_mu N'utilisent
// PAS ce seuil : leurs bornes propres (§ 1.2/1.3) sont des chantiers
// distincts.
// Sequence FIGEE et borne par seed — PARTAGEES par les lanes q3/q4 et
// par la porte --q3-affine-gate (le temoin de forte annulation teste CES
// fonctions, jamais une copie).
inline double affine_l_hat(double gd, double nd0, double nd1, double nd2,
                           double u0, double u1, double u2, double q) {
  const double t = std::fma(nd2, u2, std::fma(nd1, u1, nd0 * u0));
  return std::fma(gd, q, -(t + t));
}
inline double affine_l_bound(double gd, double nd0, double nd1, double nd2,
                             double qmax, double umax) {
  return 0x1p-48 *
         std::fma(gd, qmax,
                  2.0 * (std::fabs(nd0) + std::fabs(nd1) + std::fabs(nd2)) *
                      umax);
}
// ---- ETAGE D'INTERVALLES DE JUNG (audit § 1.2, contre-audit 04c71a2
// § 6 : « jamais le seuil de P mis au carre — intervalle sortant sur
// [P], puis propagation dans 2P² vs J·B² »). PRECONDITIONS : L < 0
// certifie (lh < -e, donc P ∈ [(lh-e)/4, (lh+e)/4] avec borne sup
// < 0) et J >= 0 (J < 0 : le seed meurt avant). Toutes les erreurs
// sont RELATIVES (aucune annulation : lh±e garde le signe, produits de
// termes de meme signe ; l'addition IEEE a une erreur relative <= u
// meme proche de zero) : conversions de J et B (<= u chacune),
// (lh±e) (u), carre (u), produits (u) — < 8u par cote. Le facteur de
// garde 2^-40 = 2^13·u les absorbe avec ×1000 de marge :
//   inf(2[P]²) > sup([J][B]²)  =>  2P² > J·B² STRICT : temoin certifie
//   (compte sous la regle stricte ET sous le mutant nonstrict) ;
//   sup(2[P]²) < inf([J][B]²)  =>  2P² < J·B² : non-temoin certifie
//   (exclu sous les deux regles) ; sinon repli exact (les egalites
//   tombent TOUJOURS dans le repli — la semantique du mutant
//   seed-core-nonstrict vit dans l'exact, jamais dans l'intervalle).
// MUTANT jung-swap-bounds : le kill teste 2·Pl² (le mauvais bout de
// l'intervalle) — le temoin a cheval grave dans --q3-affine-gate le
// voit.
constexpr double kJungGuard = 0x1p-40;
inline int jung_interval_sign(double lh, double e, double jlo, double jhi,
                              i64 b, bool mutant_swap = false) {
  const double bd = (double)b;
  const double b2 = bd * bd;
  const double pu = (lh + e) * 0.25;  // borne sup de P (la plus pres de 0)
  const double pl = (lh - e) * 0.25;  // borne inf de P
  const double pk = mutant_swap ? pl : pu;  // MUTANT : mauvais bout
  const double lhs_min = 2.0 * (pk * pk) * (1.0 - kJungGuard);
  const double rhs_max = jhi * (b2 * (1.0 + kJungGuard));
  if (lhs_min > rhs_max) return 1;
  const double lhs_max = 2.0 * (pl * pl) * (1.0 + kJungGuard);
  const double rhs_min = jlo * (b2 * (1.0 - kJungGuard));
  if (lhs_max < rhs_min) return -1;
  return 0;
}
constexpr double kFloatMutantShrink = 0x1p-20;  // MUTANT : borne /2^20
// CONDITIONS DE COMPILATION/EXECUTION EXECUTABLES (contre-audit 04c71a2
// § 4) : la preuve de la borne suppose binaire64, round-to-nearest,
// aucune reassociation. Sous __FAST_MATH__ le filtre est desactive A LA
// COMPILATION ; a l'execution, un mode d'arrondi != FE_TONEAREST le
// desactive aussi (borne = +inf => repli exact integral ; la CORRECTION
// ne depend jamais du filtre, seul le debit change). Les fils crees
// demarrent dans l'environnement flottant par defaut (nearest) — la
// verification au fil principal ne peut que sur-desactiver.
#if defined(__FAST_MATH__)
constexpr bool kFloatFilterCompileEnabled = false;
#else
constexpr bool kFloatFilterCompileEnabled = true;
#endif
inline bool float_filter_runtime_enabled() {
  return kFloatFilterCompileEnabled && std::fegetround() == FE_TONEAREST;
}

// ---- Cœur universel du seed (audit « axial arbre et cœur de seed » § 1).
// Jung : tout tetraedre ACCEPTE par la production (six aretes <= D,
// centre strictement interieur => circumboule = miniboule) verifie
// R² <= 3D/8, donc sa racine axiale verifie 2·mu² <= J = D·(3G − 2EX)
// (avec R_mu² = DEX/(4G) + mu²/(4G) et |n|² = G). Un site z tel que
// P(z) < 0 et 2·P(z)² > J·B(z)² est STRICTEMENT interieur a TOUTE sphere
// admissible du seed : Phi_mu(z) = P − mu·B <= P + racine(J/2)·|B| < 0.
// L'egalite 2P² = J·B² n'est PAS comptee (le site peut etre SUR la
// sphere extremale — mutant seed-core-nonstrict, qui compte aussi les
// points du faisceau P = 0, B = 0). J < 0 : aucun tetraedre admissible
// (R3² > 3D/8 deja sur le cercle du seed) — le seed meurt sans temoin.
// Compte sur l'antichaine de l'ancre, descente ALL/NONE par boite
// (ALL : Pmax < 0 et 2·Pmax² > J·Babs² ; NONE : Pmin >= 0), arret a h.
// FAIL-OPEN : ne tue que sur des interieurs stricts certifies du nuage ;
// les temoins hors de l'antichaine sont simplement omis. EXACT pour la
// sortie post-RLE : une emission q4 supprimee appartient a un groupe qui
// meurt de toute facon (>= h_4 interieurs si le label q_min est 4 ; si
// une emission d'arite inferieure partage la cle, le groupe subsiste par
// elle, inchange) — meme argument que depth_dead et le filtre W_4.
// HISTORIQUE DES VARIANTES DU COMPTE (toutes tranchees PAR LA MESURE,
// sorties identiques a chaque fois) : (1) descente de l'antichaine avec
// tests ALL/NONE par boite (axis_min/axis_max i128 par nœud) — PERDANTE
// par ×12 contre le scan aplati du cover (t_core 27,0 -> 2,2 s sur
// eight_clusters n=1000 ; kills STRICTEMENT identiques sur les deux
// familles : les temoins vivaient tous dans le cover de l'ancre, le
// sur-univers des handles n'ajoutait rien) ; (2) budget
// d'atteignabilite : neutre (−0,06 % de nœuds) ; (3) ordre de visite
// par P au milieu de boite : negatif (+22 %). Le compte retenu est le
// scan du cover aplati avec sortie anticipee a h_4 — il vit dans la
// boucle de seed de collect_candidate_balls.

// ---- PRIMITIVE DU SWEEP A DEUX COTES, extraite et testable SEULE (audit
// « sweep reçu et kernel sans alloc » § 3) : aucune geometrie, AUCUNE
// allocation. Entree : sites (A, B, u), permanents p < h, seuil h,
// drapeaux ; sortie : <= 16 groupes tries par mu (d_j exact, verdict),
// gid par site (0xff hors fenetre), compteurs a unites separees.
// Borne des tableaux fixes : k = h − p <= 8 ; en fenetre, chaque cote a
// au plus k valeurs distinctes (au plus k−1 sites strictement avant le
// k-ieme, plus la valeur seuil), fusion des deux signes => <= 2k <= 16
// groupes HORS MUTANT. Un mutant de classification peut deborder : le
// site excedentaire est ecarte et `overflow` le signale (l'appelant le
// rend bruyant via le compteur de reception).
struct MuGroupFixed {
  AxialSite head;  // normalise (B > 0)
  u64 npos = 0, nneg = 0, dj = 0;
  bool alive = false;
};
struct AxialSweepResult {
  MuGroupFixed groups[16];
  u64 pos_before[16] = {};
  u64 neg_after[16] = {};
  int ngroups = 0;
  u64 roots_pruned_cross = 0;   // positives sous L, negatives sur U
  u64 groups_killed_depth = 0;  // en fenetre, d_j >= h
  bool overflow = false;
};
inline AxialSweepResult axial_two_sided_sweep(const AxialSite* sites,
                                              size_t n, u64 p, u64 h,
                                              u32 flags, u8* gid) {
  AxialSweepResult r;
  u64 kk = h - p;
  if ((flags & kAxialShortGroup) && kk > 1) --kk;  // MUTANT
  // Seuils bornes des deux cotes (cote negatif normalise (−A, −B) : mu
  // inchangee — −A/−B = A/B — mais B > 0 pour le comparateur ; la
  // selection y est DESCENDANTE). Plein : remplacer l'extremum si
  // STRICTEMENT meilleur (l'egalite au seuil ne change pas la k-ieme
  // valeur).
  AxialSite side_arr[2][10];
  size_t side_n[2] = {0, 0};
  const auto dcmp = [&](int side, const AxialSite& x, const AxialSite& y) {
    const int c = cmp_mu_same_side(x.A, x.B, y.A, y.B);
    return side == 0 ? c : -c;
  };
  const auto push_bounded = [&](const AxialSite& sx, int side) {
    AxialSite* arr = side_arr[side];
    size_t& m = side_n[side];
    if (m == (size_t)kk && dcmp(side, sx, arr[m - 1]) >= 0) return;
    size_t i = (m < (size_t)kk) ? m++ : (size_t)kk - 1;
    while (i > 0 && dcmp(side, sx, arr[i - 1]) < 0) {
      arr[i] = arr[i - 1];
      --i;
    }
    arr[i] = sx;
  };
  for (size_t i = 0; i < n; ++i) {
    const AxialSite& s0 = sites[i];
    if (s0.B > 0) push_bounded(s0, 0);
    else push_bounded(AxialSite{-s0.A, -s0.B, s0.u}, 1);
  }
  const bool has_U = side_n[0] == (size_t)kk;
  const bool has_L = side_n[1] == (size_t)kk;
  const AxialSite Uth = has_U ? side_arr[0][kk - 1] : AxialSite{};
  const AxialSite Lth = has_L ? side_arr[1][kk - 1] : AxialSite{};
  // Classification : fenetre [L, U], TIES DE FRONTIERE INCLUS (mutant
  // drop-ties : < / >). Racines croisees — positive sous L (>= k
  // negatifs strictement au-dessus) et negative sur U (>= k positifs
  // strictement en dessous) : d >= h exact, comptees ; les exclusions
  // par leur PROPRE cote ne le sont pas.
  u64 pos_lt_L = 0, neg_gt_U = 0;
  for (size_t i = 0; i < n; ++i) {
    gid[i] = 0xff;
    const AxialSite& s0 = sites[i];
    const bool pos = s0.B > 0;
    const AxialSite sz = pos ? s0 : AxialSite{-s0.A, -s0.B, s0.u};
    const int cL = has_L ? cmp_mu_same_side(sz.A, sz.B, Lth.A, Lth.B) : 1;
    const int cU = has_U ? cmp_mu_same_side(sz.A, sz.B, Uth.A, Uth.B) : -1;
    bool below = (flags & kAxialDropTies) ? cL <= 0 : cL < 0;  // MUTANT
    bool above = (flags & kAxialDropTies) ? cU >= 0 : cU > 0;  // MUTANT
    if (flags & kAxialIgnoreOpposite) {
      // MUTANT CAUSAL (audit § 3) : chaque racine ne lit que SON ordre —
      // le seuil issu du cote oppose ne l'ecarte plus.
      if (pos) below = false;
      else above = false;
    }
    if (below) {
      if (pos) {
        ++pos_lt_L;
        ++r.roots_pruned_cross;
      }
      continue;
    }
    if (above) {
      if (!pos) {
        ++neg_gt_U;
        ++r.roots_pruned_cross;
      }
      continue;
    }
    int g = -1;
    for (int j = 0; j < r.ngroups; ++j)
      if (cmp_mu_same_side(sz.A, sz.B, r.groups[j].head.A,
                           r.groups[j].head.B) == 0) {
        g = j;
        break;
      }
    if (g < 0) {
      if (r.ngroups == 16) {
        r.overflow = true;
        continue;
      }
      g = r.ngroups++;
      r.groups[g].head = sz;
    }
    if (pos) ++r.groups[g].npos;
    else ++r.groups[g].nneg;
    gid[i] = (u8)g;
  }
  // Tri des <= 16 groupes par mu croissante, remap des gid.
  int order[16];
  for (int j = 0; j < r.ngroups; ++j) order[j] = j;
  std::sort(order, order + r.ngroups, [&](int x, int y) {
    return cmp_mu_same_side(r.groups[x].head.A, r.groups[x].head.B,
                            r.groups[y].head.A, r.groups[y].head.B) < 0;
  });
  MuGroupFixed sorted[16];
  u8 inv[16];
  for (int j = 0; j < r.ngroups; ++j) {
    sorted[j] = r.groups[order[j]];
    inv[order[j]] = (u8)j;
  }
  for (int j = 0; j < r.ngroups; ++j) r.groups[j] = sorted[j];
  for (size_t i = 0; i < n; ++i)
    if (gid[i] != 0xff) gid[i] = inv[gid[i]];
  // Prefixes positifs / suffixes negatifs, puis d_j exact et verdict.
  u64 pref = pos_lt_L;
  for (int j = 0; j < r.ngroups; ++j) {
    r.pos_before[j] = pref;
    pref += r.groups[j].npos;
  }
  u64 suff = neg_gt_U;
  for (int j = r.ngroups; j-- > 0;) {
    r.neg_after[j] = suff;
    suff += r.groups[j].nneg;
  }
  for (int j = 0; j < r.ngroups; ++j) {
    MuGroupFixed& g = r.groups[j];
    u64 dj = p + r.pos_before[j];
    if (flags & kAxialIgnoreOpposite) {
      // MUTANT CAUSAL : un groupe purement positif ignore le suffixe
      // negatif, purement negatif le prefixe positif ; un groupe mixte
      // est bilateral par nature et garde les deux termes.
      if (g.npos == 0) dj = p + r.neg_after[j];
      else if (g.nneg != 0) dj += r.neg_after[j];
    } else {
      dj += (flags & kAxialReverseNeg)
                ? (suff - r.neg_after[j] - g.nneg)  // MUTANT : prefixe
                : r.neg_after[j];
    }
    if (flags & kAxialDepthNonstrict) dj += g.npos + g.nneg;  // MUTANT
    g.dj = dj;
    g.alive = dj < h;
    if (!g.alive) ++r.groups_killed_depth;
  }
  return r;
}

namespace detail_bs {

// Vague WSPD ternaire d'une lane : rectangles vivants (h_coeur < h).
struct AliveRect {
  WspdRect r;
  u64 core;
};

inline void wspd_alive(const CloudIndex& ix, i64 s, const u64 h_of[3], int lane_idx,
                       u8 mask, u64 h, std::vector<AliveRect>* out) {
  out->clear();
  if (ix.nodes.empty()) return;
  std::vector<WspdRect> wave, next;
  for (const RadixNode& n : ix.nodes) wave.push_back(WspdRect{n.left, n.right});
  while (!wave.empty()) {
    next.clear();
    for (const WspdRect& r : wave) {
      const FusedCounts fc = count_universal_witnesses_234(ix, r.a, r.b, h_of, mask, false);
      if (fc.c[lane_idx] >= h) continue;
      i64 ba[3], bb[3];
      const auto va = detail::node_view(ix, r.a, ba);
      const auto vb = detail::node_view(ix, r.b, bb);
      if (detail::separated(va, vb, s, 1)) {
        const FusedCounts ff = count_universal_witnesses_234(ix, r.a, r.b, h_of, mask, true);
        if (ff.c[lane_idx] < h) out->push_back(AliveRect{r, ff.c[lane_idx]});
        continue;
      }
      const i64 w2a = detail::box_w2(va);
      const i64 w2b = detail::box_w2(vb);
      const bool split_a = (r.a >= 0) && (r.b < 0 || w2a >= w2b);
      const NodeRef keep = split_a ? r.b : r.a;
      const RadixNode& n = ix.nodes[(size_t)(split_a ? r.a : r.b)];
      next.push_back(split_a ? WspdRect{n.left, keep} : WspdRect{keep, n.left});
      next.push_back(split_a ? WspdRect{n.right, keep} : WspdRect{keep, n.right});
    }
    wave.swap(next);
  }
}

// Histogrammes h_a/h_b a 8 coins d'un rectangle, pour une lane.
inline void corner_histograms(const CloudIndex& ix, Lane lane, const AliveRect& ar,
                              std::vector<u64>* ha, std::vector<u64>* hb) {
  const NodeRange ra = range_of(ix, ar.r.a);
  const NodeRange rb = range_of(ix, ar.r.b);
  const AxisBox boxA = box_of_node(ix, ar.r.a);
  const AxisBox boxB = box_of_node(ix, ar.r.b);
  const int na = ra.last - ra.first + 1;
  const int nb = rb.last - rb.first + 1;
  ha->assign((size_t)na, 0);
  hb->assign((size_t)nb, 0);
  for (int ia = 0; ia < na; ++ia)
    for (int iz = 0; iz < na; ++iz) {
      if (iz == ia) continue;
      if (universal_over_corners(lane, ix.upos[(size_t)(ra.first + ia)], boxB,
                                 ix.upos[(size_t)(ra.first + iz)]))
        ++(*ha)[(size_t)ia];
    }
  for (int ib = 0; ib < nb; ++ib)
    for (int iz = 0; iz < nb; ++iz) {
      if (iz == ib) continue;
      if (universal_over_corners(lane, ix.upos[(size_t)(rb.first + ib)], boxA,
                                 ix.upos[(size_t)(rb.first + iz)]))
        ++(*hb)[(size_t)ib];
    }
}

}  // namespace detail_bs

// Collecte les boules candidates des trois lanes (generateurs seulement —
// AUCUN census ici : il se fait une fois par cle unique, en aval).
// FILTRE DE PROFONDEUR A LA GENERATION (reponse a la question du minorant
// par boule) : le cover de l'ancre est un SOUS-ENSEMBLE du nuage, donc
// #{z ∈ cover : P_B(z) < 0} MINORE |I_B| — un minorant qui atteint h_q tue
// le candidat exactement comme la passe count-only l'aurait fait, mais
// AVANT l'emission (ni tri, ni descente d'arbre par boule ; le scan sort
// des l'atteinte du seuil, quelques dizaines de nanosecondes par candidat
// profond). La completude du cover n'est PAS requise : un sous-compte ne
// tue jamais a tort. MUTANT genfilter-nonstrict : P <= 0 compte la
// coquille (et les supports memes) — des boules a plateau meurent a tort,
// le juge voit les evenements manquants.
//
// SELECTION AXIALE BORNEE (lane q4, reponse auditeur « axial borne ») : le
// faisceau des spheres par le seed (a,b,x) est parametre par
// mu_z = A_z/B_z (A = puissance q3, B = normale·(z−a)) ; cote B > 0, tout
// predecesseur STRICT de mu_y est interieur strict a la sphere de y (cote
// B < 0 : successeur) ; les permanents (B = 0, A < 0) sont interieurs a
// TOUTE la famille. Une completion utile appartient donc au prefixe borne
// p + preds <= h_4 − 1 : trois balayages lineaires du cover (p ; seuils
// bornes k = h_4 − p <= 8 par cote, multiplicites comptees ; retenue TIES
// INCLUS — eliminer les egalites de frontiere selon l'ordre memoire serait
// faux), puis UNE emission par groupe exact de mu — un groupe de meme mu
// est une seule sphere, l'aval consomme des BallKeys. Le representant emis
// est le MINIMUM ball_candidate_less des membres VALIDES du groupe (les
// predicats de support ne sont jamais l'autorite sur un seul representant).
// Les deux chemins rendent le MEME objet post-RLE (porte appariee, egalite
// cles + arite + representation). RESULTAT NEGATIF HONNETE (reçu axial
// borne) : sur CPU, la baseline rejette la plupart des completions a la
// LENTILLE (~3 operations i64) tandis que le balayage axial paye A,B sur
// TOUS les sites de TOUS les seeds — mesure : t_gen +7 % a n=1600 malgre
// 18,8 M -> 0,8 M d'evaluations q4 ; les deux postes croissent au meme
// rythme, pas de croisement CPU. Le chemin axial reste OPT-IN pour le port
// GPU (travail borne regulier, sans sorties anticipees divergentes) — la
// production CPU est la baseline (axial_bounded = false par defaut).
// MUTANTS du chemin axial : axial-short-group (k−1 : perd la boule de
// frontiere de profondeur), axial-drop-ties (< au lieu de <= : perd le
// groupe ex aequo), axial-first-rep (premier membre valide au lieu du
// minimum canonique : la representation de niveau post-RLE change).
// PARALLELISME PAR RECTANGLE (directive utilisateur « paralléliser ») :
// les rectangles vivants d'une lane sont independants — chaque ouvrier a
// son brouillon (LaneScratch), son vecteur d'emissions et ses stats,
// fusionnes a la fin. L'EXACTITUDE ne depend pas de l'ordre : le
// MULTIENSEMBLE des emissions est identique quel que soit le decoupage,
// et le tri stable + RLE en aval canonise l'ordre — la porte
// --par-gate exige l'egalite au bit pres post-RLE entre 1 et N fils.
// num_threads = 1 (defaut) : chemin sequentiel historique, sans fil.
inline void collect_candidate_balls(const CloudIndex& ix, i64 s, u64 smax_eff,
                                    std::vector<BallCandidate>* out,
                                    BallStreamStats* st,
                                    bool mutant_genfilter_nonstrict = false,
                                    bool axial_bounded = false,
                                    u32 axial_flags = 0, int num_threads = 1,
                                    bool mutant_par_drop_shard = false) {
  out->clear();
  // Garde d'arrondi (contre-audit 04c71a2 § 4) : evaluee UNE FOIS au fil
  // appelant ; false => bornes +inf, tout passe par le repli exact.
  const bool float_on = (axial_flags & kFloatIgnoreRounding)
                            ? true  // MUTANT : garde ignoree
                            : float_filter_runtime_enabled();
  const u64 h_of[3] = {lane_h(Lane::kQ2, smax_eff), lane_h(Lane::kQ3, smax_eff),
                       lane_h(Lane::kQ4, smax_eff)};
  std::vector<detail_bs::AliveRect> alive;
  struct LaneScratch {
    std::vector<u64> ha, hb;
    std::vector<CoverPoint> cover;
    std::vector<AxialSite> axial;
    std::vector<u8> axial_gid;
    std::vector<NodeRef> handles;
    u64 cover_nodes = 0, visits = 0;
    // DONNEES DE SITE AFFINES par ancre (audit e27acfa § 1) :
    // u = 2z − a − b, q = |u|² − D2 — entiers < 2^36, donc EXACTS en
    // binaire64 (plus aucune erreur de conversion cote site). Calculees
    // UNE FOIS par ancre et partagees par TOUS ses seeds ; l'identite
    // L(z,x) = G_x·q_z − 2·u_z·N_x = 4·q3_power(f_x, z) est gravee par
    // la porte --q3-affine-gate. qmax/umax : majorants de l'ancre pour
    // la borne flottante par seed.
    // Un seul jeu de tableaux ENTIERS : la conversion i64 -> double d'un
    // entier < 2^36 est EXACTE et vaut une instruction — la faire au vol
    // dans la boucle divise par deux le trafic memoire du remplissage.
    std::vector<i64> su0, su1, su2, sq;
    double qmax_d = 1.0, umax_d = 1.0;
    void fill_affine_sites(const CloudIndex& ix, const P3& pa, const P3& pb,
                           i64 D2) {
      const size_t nc = cover.size();
      su0.resize(nc); su1.resize(nc); su2.resize(nc); sq.resize(nc);
      i64 qmax = 1, umax = 1;
      const i64 sx = pa.x + pb.x, sy = pa.y + pb.y, sz = pa.z + pb.z;
      for (size_t i = 0; i < nc; ++i) {
        const P3& pz = ix.upos[(size_t)cover[i].u];
        const i64 u0 = 2 * pz.x - sx, u1 = 2 * pz.y - sy, u2 = 2 * pz.z - sz;
        const i64 qz = u0 * u0 + u1 * u1 + u2 * u2 - D2;
        su0[i] = u0; su1[i] = u1; su2[i] = u2; sq[i] = qz;
        const i64 qa = qz < 0 ? -qz : qz;
        if (qa > qmax) qmax = qa;
        for (const i64 uu : {u0 < 0 ? -u0 : u0, u1 < 0 ? -u1 : u1,
                             u2 < 0 ? -u2 : u2})
          if (uu > umax) umax = uu;
      }
      qmax_d = (double)qmax;
      umax_d = (double)umax;
    }
  };
  // Chaque lane : sequentiel a 1 fil, sinon tirage dynamique des
  // rectangles (compteur atomique) — les gros rectangles ne bloquent pas
  // la fin de vague. MUTANT par-drop-shard : la fusion oublie le premier
  // ouvrier (la porte --par-gate doit le voir).
  const auto run_rects = [&](auto&& body) {
    const size_t nrect = alive.size();
    if (num_threads <= 1 || nrect <= 1) {
      LaneScratch sc;
      for (size_t i = 0; i < nrect; ++i) body(alive[i], sc, out, st);
      return;
    }
    const size_t T = std::min((size_t)num_threads, nrect);
    std::vector<std::vector<BallCandidate>> louts(T);
    std::vector<BallStreamStats> lst(T);
    std::atomic<size_t> next{0};
    std::vector<std::thread> pool;
    for (size_t t = 0; t < T; ++t)
      pool.emplace_back([&, t] {
        LaneScratch sc;
        for (;;) {
          const size_t i = next.fetch_add(1);
          if (i >= nrect) break;
          body(alive[i], sc, &louts[t], &lst[t]);
        }
      });
    for (auto& th : pool) th.join();
    for (size_t t = 0; t < T; ++t) {
      if (mutant_par_drop_shard && t == 0) continue;  // MUTANT
      out->insert(out->end(), louts[t].begin(), louts[t].end());
      st->add_from(lst[t]);
    }
  };
  // ---- q2.
  detail_bs::wspd_alive(ix, s, h_of, 0, 0b001, h_of[0], &alive);
  st->rect_alive[0] = alive.size();
  run_rects([&](const detail_bs::AliveRect& ar, LaneScratch& sc,
                std::vector<BallCandidate>* lout, BallStreamStats* lst) {
    auto& ha = sc.ha;
    auto& hb = sc.hb;
    auto* out = lout;
    auto* st = lst;
    detail_bs::corner_histograms(ix, Lane::kQ2, ar, &ha, &hb);
    const NodeRange ra = range_of(ix, ar.r.a);
    const NodeRange rb = range_of(ix, ar.r.b);
    const u64 need = h_of[0] - ar.core;
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        ++st->anchors[0];
        if (ha[(size_t)(ua - ra.first)] + hb[(size_t)(ub - rb.first)] >= need)
          continue;
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        out->push_back(BallCandidate{q2_ball_key(pa, pb),
                                     promote_q3_level(q2_exact_level(D2)), 2});
        ++st->candidates[0];
      }
  });
  // ---- q3.
  detail_bs::wspd_alive(ix, s, h_of, 1, 0b010, h_of[1], &alive);
  st->rect_alive[1] = alive.size();
  run_rects([&](const detail_bs::AliveRect& ar, LaneScratch& sc,
                std::vector<BallCandidate>* lout, BallStreamStats* lst) {
    auto& ha = sc.ha;
    auto& hb = sc.hb;
    auto& cover = sc.cover;
    auto& handles = sc.handles;
    auto& cover_nodes = sc.cover_nodes;
    auto& visits = sc.visits;
    auto* out = lout;
    auto* st = lst;
    detail_bs::corner_histograms(ix, Lane::kQ3, ar, &ha, &hb);
    const NodeRange ra = range_of(ix, ar.r.a);
    const NodeRange rb = range_of(ix, ar.r.b);
    const AxisBox boxA = box_of_node(ix, ar.r.a);
    const AxisBox boxB = box_of_node(ix, ar.r.b);
    rect_cover_handles(ix, boxA, boxB, 3, false, &handles, &cover_nodes);
    const u64 need = h_of[1] - ar.core;
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        ++st->anchors[1];
        if (ha[(size_t)(ua - ra.first)] + hb[(size_t)(ub - rb.first)] >= need)
          continue;
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        anchor_cover_from_handles(ix, handles, pa, pb, D2, 3, &cover, &visits);
        // Remplissage PARESSEUX des sites affines : au premier seed aigu
        // seulement — une ancre sans seed ne paie pas l'O(cover).
        bool affine_filled = false;
        for (const CoverPoint& cp : cover) {
          if (cp.u == ua || cp.u == ub) continue;
          const P3& px = ix.upos[(size_t)cp.u];
          if (!is_acute_seed(pa, pb, px, D2, ix.bucket_ids[ix.bucket_start[(size_t)ua]],
                             ix.bucket_ids[ix.bucket_start[(size_t)ub]],
                             ix.bucket_ids[ix.bucket_start[(size_t)cp.u]]))
            continue;
          if (!affine_filled) {
            sc.fill_affine_sites(ix, pa, pb, D2);
            affine_filled = true;
          }
          const Q3Form f3 = q3_form(pa, pb, px);
          // KERNEL AFFINE PAR ANCRE (audit e27acfa § 1) : N = W − G·d une
          // fois par seed, interaction = un produit scalaire sur les
          // donnees de site partagees ; etage flottant certifie sur sites
          // EXACTS (seules les conversions de G et N et cinq arrondis
          // contribuent — borne par seed E = 2^-48·(G·qmax + 2|N|₁·umax),
          // derivation au reçu « filtre flottant »), repli = affine exact
          // i128 (< 2^105). Semantique STRICTEMENT identique : L = 4P.
          const i128 N0 = f3.w[0] - f3.g * (i128)(pb.x - pa.x);
          const i128 N1 = f3.w[1] - f3.g * (i128)(pb.y - pa.y);
          const i128 N2 = f3.w[2] - f3.g * (i128)(pb.z - pa.z);
          const double Gd = (double)f3.g;
          const double Nd0 = (double)N0, Nd1 = (double)N1, Nd2 = (double)N2;
          double fbnd =
              float_on ? affine_l_bound(Gd, Nd0, Nd1, Nd2, sc.qmax_d,
                                        sc.umax_d)
                       : std::numeric_limits<double>::infinity();
          if (axial_flags & kFloatSmallThreshold)
            fbnd *= kFloatMutantShrink;  // MUTANT
          const auto exact_L = [&](size_t iz) {
            return f3.g * (i128)sc.sq[iz] -
                   2 * ((i128)sc.su0[iz] * N0 + (i128)sc.su1[iz] * N1 +
                        (i128)sc.su2[iz] * N2);
          };
          u64 depth = 0;
          bool deep = false;
          for (size_t iz = 0; iz < cover.size(); ++iz) {
            const double Lh = affine_l_hat(
                Gd, Nd0, Nd1, Nd2, (double)sc.su0[iz], (double)sc.su1[iz],
                (double)sc.su2[iz], (double)sc.sq[iz]);
            bool interior;
            if (Lh < -fbnd) {
              ++st->float_cert_neg;
              if ((axial_flags & kFloatVerify) && !(exact_L(iz) < 0))
                ++st->float_mismatch;
              interior = true;
            } else if (Lh > fbnd) {
              ++st->float_cert_pos;
              if ((axial_flags & kFloatVerify) && !(exact_L(iz) > 0))
                ++st->float_mismatch;
              interior = false;
            } else {
              ++st->float_fallback;
              const i128 L = exact_L(iz);
              interior = L < 0 || (mutant_genfilter_nonstrict && L <= 0);
            }
            if (interior && ++depth >= h_of[1]) {
              deep = true;
              break;
            }
          }
          if (deep) {
            ++st->gen_depth_killed[1];
            continue;
          }
          out->push_back(BallCandidate{q3_ball_key(f3),
                                       promote_q3_level(q3_exact_level(pa, pb, px)),
                                       3});
          ++st->candidates[1];
        }
      }
  });
  // ---- q4.
  detail_bs::wspd_alive(ix, s, h_of, 2, 0b100, h_of[2], &alive);
  st->rect_alive[2] = alive.size();
  run_rects([&](const detail_bs::AliveRect& ar, LaneScratch& sc,
                std::vector<BallCandidate>* lout, BallStreamStats* lst) {
    auto& ha = sc.ha;
    auto& hb = sc.hb;
    auto& cover = sc.cover;
    auto& axial = sc.axial;
    auto& axial_gid = sc.axial_gid;
    auto& handles = sc.handles;
    auto& cover_nodes = sc.cover_nodes;
    auto& visits = sc.visits;
    auto* out = lout;
    auto* st = lst;
    detail_bs::corner_histograms(ix, Lane::kQ4, ar, &ha, &hb);
    const NodeRange ra = range_of(ix, ar.r.a);
    const NodeRange rb = range_of(ix, ar.r.b);
    const AxisBox boxA = box_of_node(ix, ar.r.a);
    const AxisBox boxB = box_of_node(ix, ar.r.b);
    rect_cover_handles(ix, boxA, boxB, 3, false, &handles, &cover_nodes);
    const u64 need = h_of[2] - ar.core;
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        ++st->anchors[2];
        if (ha[(size_t)(ua - ra.first)] + hb[(size_t)(ub - rb.first)] >= need)
          continue;
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        anchor_cover_from_handles(ix, handles, pa, pb, D2, 3, &cover, &visits);
        // COMPTE W_4 EXACT PAR ANCRE : tout z ∈ W_4(a,b) est strictement
        // interieur a TOUTE boule q4 possedee par l'ancre (cœur universel
        // owner + Jung — il n'y a pas de region anchor-only plus grande).
        // n4 >= h_4 tue l'ANCRE entiere avant les boucles seed × completion ;
        // W_4 ⊆ W_2 ⊆ cover coef 3, et un sous-compte ne tue jamais a tort.
        {
          u64 n4 = 0;
          for (const CoverPoint& cz : cover) {
            if (cz.u == ua || cz.u == ub) continue;
            if (in_spindle(Lane::kQ4, pa, pb, ix.upos[(size_t)cz.u]) &&
                ++n4 >= h_of[2])
              break;
          }
          if (n4 >= h_of[2]) {
            ++st->anchors_killed_w4;
            continue;
          }
        }
        // Sites affines partages par tous les seeds de l'ancre —
        // remplissage PARESSEUX au premier cœur de seed effectif (une
        // ancre tuee par W_4 ou sans seed aigu ne paie rien).
        bool affine_filled = false;
        const auto pid = [&](i32 u) { return ix.bucket_ids[ix.bucket_start[(size_t)u]]; };
        for (const CoverPoint& cx : cover) {
          if (cx.u == ua || cx.u == ub) continue;
          const P3& px = ix.upos[(size_t)cx.u];
          if (!is_acute_seed(pa, pb, px, D2, pid(ua), pid(ub), pid(cx.u))) continue;
          const i64 l_ax = p3_norm2(p3_sub(px, pa));
          const i64 l_bx = p3_norm2(p3_sub(px, pb));
          // Cœur universel du seed (audit « axial arbre et cœur de seed »)
          // — COMMUN aux deux chemins : la porte appariee reste appariee,
          // et le juge des petits n protege la regle elle-meme (un temoin
          // est un interieur strict de toute sphere admissible, la sortie
          // post-RLE est inchangee — argument depth_dead/W_4 en tete de
          // seed_core_kills).
          const Q3Form f3s = q3_form(pa, pb, px);
          const P3 nrm = p3_cross(p3_sub(pb, pa), p3_sub(px, pa));
          {
            const auto c0 = std::chrono::steady_clock::now();
            const i128 Jb = (i128)D2 * (3 * f3s.g - 2 * (i128)l_ax * l_bx);
            const bool nonstrict =
                (axial_flags & kAxialSeedCoreNonstrict) != 0;
            // Compte des temoins sur le COVER APLATI de l'ancre (variante
            // gagnante — en-tete « historique des variantes ») : minorant
            // fail-open (un sous-compte ne tue jamais a tort), sortie
            // anticipee a h_4, ~une puissance q3 par site examine.
            bool dead = Jb < 0;
            if (!dead) {
              if (!affine_filled) {
                sc.fill_affine_sites(ix, pa, pb, D2);
                affine_filled = true;
              }
              // KERNEL AFFINE (audit e27acfa § 1), meme forme que la lane
              // q3 : N = W − G·d une fois par seed, sites (u,q) partages
              // de l'ancre, borne par seed E = 2^-48·(G·qmax + 2|N|₁·umax).
              // P > 0 certifie => jamais temoin, sans payer l'i128 ; sinon
              // repli affine exact, P = L/4 EXACT (divisibilite et identite
              // L = 4·q3_power gravees par --q3-affine-gate) — requis par
              // Jung (la borne d'intervalles de 2P² vs JB² est le chantier
              // suivant, audit § 1.2).
              const i128 N0 = f3s.w[0] - f3s.g * (i128)(pb.x - pa.x);
              const i128 N1 = f3s.w[1] - f3s.g * (i128)(pb.y - pa.y);
              const i128 N2 = f3s.w[2] - f3s.g * (i128)(pb.z - pa.z);
              const double Gd = (double)f3s.g;
              const double Nd0 = (double)N0, Nd1 = (double)N1,
                           Nd2 = (double)N2;
              double fbnd2 =
                  float_on ? affine_l_bound(Gd, Nd0, Nd1, Nd2, sc.qmax_d,
                                            sc.umax_d)
                           : std::numeric_limits<double>::infinity();
              if (axial_flags & kFloatSmallThreshold)
                fbnd2 *= kFloatMutantShrink;  // MUTANT
              // Intervalle de J par seed (J >= 0 ici : J < 0 a deja tue).
              const double Jd = (double)Jb;
              const double Jlo = Jd * (1.0 - kJungGuard);
              const double Jhi = Jd * (1.0 + kJungGuard);
              const auto exact_L = [&](size_t iz) {
                return f3s.g * (i128)sc.sq[iz] -
                       2 * ((i128)sc.su0[iz] * N0 + (i128)sc.su1[iz] * N1 +
                            (i128)sc.su2[iz] * N2);
              };
              u64 fcount = 0;
              for (size_t iz = 0; iz < cover.size(); ++iz) {
                const CoverPoint& cz = cover[iz];
                if (cz.u == ua || cz.u == ub || cz.u == cx.u) continue;
                ++st->seed_core_sites;
                const double Lh = affine_l_hat(
                    Gd, Nd0, Nd1, Nd2, (double)sc.su0[iz], (double)sc.su1[iz],
                    (double)sc.su2[iz], (double)sc.sq[iz]);
                if (Lh > fbnd2) {
                  ++st->float_cert_pos;
                  if ((axial_flags & kFloatVerify) && !(exact_L(iz) > 0))
                    ++st->float_mismatch;
                  continue;  // P > 0 certifie : jamais temoin
                }
                if (Lh < -fbnd2) {
                  // P < 0 certifie : etage d'INTERVALLES de Jung — un
                  // site separe ne paie NI l'affine i128 NI le U320.
                  ++st->float_cert_neg;
                  const P3& pz = ix.upos[(size_t)cz.u];
                  const i64 Bz = p3_dot(nrm, p3_sub(pz, f3s.a));
                  const int js = jung_interval_sign(Lh, fbnd2, Jlo, Jhi, Bz);
                  if (js != 0) {
                    if (axial_flags & kFloatVerify) {
                      const i128 Pz = exact_L(iz) / 4;
                      const int c = (Pz < 0) ? cmp_2p2_jb2(Pz, Jb, Bz) : -2;
                      if ((js > 0) != (c > 0)) ++st->float_mismatch;
                    }
                    if (js > 0) {
                      ++st->jung_cert_kill;
                      if (++fcount >= h_of[2]) break;
                    } else {
                      ++st->jung_cert_skip;
                    }
                    continue;
                  }
                  ++st->jung_fallback;
                  const i128 Pz = exact_L(iz) / 4;
                  if ((axial_flags & kFloatVerify) && !(Pz < 0))
                    ++st->float_mismatch;
                  const int c = cmp_2p2_jb2(Pz, Jb, Bz);
                  if ((nonstrict ? (c >= 0) : (c > 0)) && ++fcount >= h_of[2])
                    break;
                  continue;
                }
                // Bande d'incertitude du signe : chemin exact integral.
                ++st->float_fallback;
                const i128 Pz = exact_L(iz) / 4;
                if (nonstrict ? (Pz > 0) : (Pz >= 0)) continue;
                const P3& pz = ix.upos[(size_t)cz.u];
                const i64 Bz = p3_dot(nrm, p3_sub(pz, f3s.a));
                const int c = cmp_2p2_jb2(Pz, Jb, Bz);
                if ((nonstrict ? (c >= 0) : (c > 0)) && ++fcount >= h_of[2])
                  break;
              }
              dead = fcount >= h_of[2];
            }
            st->t_seed_core_ms += std::chrono::duration<double, std::milli>(
                                      std::chrono::steady_clock::now() - c0)
                                      .count();
            if (dead) {
              ++st->seeds_killed_seed_core;
              continue;
            }
          }
          // Predicats du chemin de base pour UNE completion : lentille,
          // owner 6 aretes, exact-once du seed, det, centre strict. Rend
          // true et remplit le candidat si la completion est valide.
          const auto valid_completion = [&](i32 uy, BallCandidate* cand,
                                            Q4Form* f4out) {
            if (uy == cx.u || uy == ua || uy == ub) return false;
            const P3& py = ix.upos[(size_t)uy];
            const i64 l_ay = p3_norm2(p3_sub(py, pa));
            const i64 l_by = p3_norm2(p3_sub(py, pb));
            if (l_ay > D2 || l_by > D2) return false;
            const i64 l_xy = p3_norm2(p3_sub(py, px));
            if (l_xy > D2) return false;
            if (!tetra_owned_by(D2, l_ax, l_ay, l_bx, l_by, l_xy, pid(ua),
                                pid(ub), pid(cx.u), pid(uy)))
              return false;
            // Exact-once du seed (le RLE dedupliquerait de toute facon ;
            // ceci borne le flux de candidats).
            const P3 vy{2 * py.x - pa.x - pb.x, 2 * py.y - pa.y - pb.y,
                        2 * py.z - pa.z - pb.z};
            if (p3_norm2(vy) > D2 && pid(uy) < pid(cx.u)) return false;
            const Q4Form f4 = q4_form(pa, pb, px, py);
            if (f4.det == 0) return false;
            if (!q4_center_strictly_inside(f4, pa, pb, px, py)) return false;
            *cand = BallCandidate{q3_ball_key_reduce(q4_ball_form(f4)),
                                  q4_level_raw(f4), 4};
            *f4out = f4;
            return true;
          };
          const auto depth_dead = [&](const Q4Form& f4) {
            u64 depth = 0;
            for (const CoverPoint& cz : cover) {
              const i128 pw = q4_power(f4, ix.upos[(size_t)cz.u]);
              if ((pw < 0 || (mutant_genfilter_nonstrict && pw <= 0)) &&
                  ++depth >= h_of[2])
                return true;
            }
            return false;
          };
          if (!axial_bounded) {
            // BASELINE APPARIEE : enumeration de toutes les completions —
            // meme objet post-RLE, seulement plus de candidats evalues.
            for (const CoverPoint& cy : cover) {
              BallCandidate cand;
              Q4Form f4;
              if (!valid_completion(cy.u, &cand, &f4)) continue;
              if (depth_dead(f4)) {
                ++st->gen_depth_killed[2];
                continue;
              }
              out->push_back(cand);
              ++st->candidates[2];
            }
            continue;
          }
          // --- SELECTION AXIALE BORNEE (en-tete de fonction). ---
          ++st->axial_seeds;
          const auto ta0 = std::chrono::steady_clock::now();
          // Balayage 1 : A,B caches (SoA seed-local), permanents p.
          axial.clear();
          u64 p_perm = 0;
          for (const CoverPoint& cz : cover) {
            if (cz.u == ua || cz.u == ub || cz.u == cx.u) continue;
            const P3& pz = ix.upos[(size_t)cz.u];
            const i128 A = q3_power(f3s, pz);
            const i64 B = p3_dot(nrm, p3_sub(pz, pa));
            if (B == 0) {
              if (A < 0) ++p_perm;  // interieur de TOUTE la famille
              continue;
            }
            axial.push_back(AxialSite{A, B, cz.u});
          }
          st->axial_sites += axial.size();
          if (p_perm >= h_of[2]) {
            ++st->axial_seeds_dead_perm;
            continue;
          }
          const auto ta1 = std::chrono::steady_clock::now();
          st->t_axial_ab_ms +=
              std::chrono::duration<double, std::milli>(ta1 - ta0).count();
          // PRIMITIVE du sweep a deux cotes (d_cover(mu) = p + P_<(mu) +
          // N_>(mu) lu dans les deux ordres) — kernel SANS ALLOCATION par
          // seed (audit « sweep reçu et kernel sans alloc » § 1) : <= 16
          // groupes en tableaux fixes, gid par site dans un tampon
          // reutilise entre les seeds.
          axial_gid.resize(axial.size());
          const AxialSweepResult sw =
              axial_two_sided_sweep(axial.data(), axial.size(), p_perm,
                                    h_of[2], axial_flags, axial_gid.data());
          st->axial_roots_pruned_cross += sw.roots_pruned_cross;
          st->axial_groups_in_window += (u64)sw.ngroups;
          st->axial_groups_killed_depth += sw.groups_killed_depth;
          // Le debordement des tableaux fixes est IMPOSSIBLE hors mutant
          // (borne 2k <= 16 prouvee en tete de primitive) : le rendre
          // bruyant sur le canal de reception.
          if (sw.overflow) ++st->axial_verify_mismatch;
          const auto ta2 = std::chrono::steady_clock::now();
          st->t_reduce_ms +=
              std::chrono::duration<double, std::milli>(ta2 - ta1).count();
          // Seconde passe sur les sites : valid_completion pour les SEULS
          // groupes vivants, meilleur representant canonique par groupe
          // (mutant first-rep : premier valide, les suivants ignores) —
          // et premier membre de CHAQUE groupe en fenetre retenu pour la
          // reception etendue.
          bool have[16] = {};
          bool has_vm[16] = {};
          BallCandidate best[16];
          i32 vmember[16];
          for (size_t i = 0; i < axial.size(); ++i) {
            const u8 g = axial_gid[i];
            if (g == 0xff) continue;
            if (!has_vm[g]) {
              vmember[g] = axial[i].u;
              has_vm[g] = true;
            }
            if (!sw.groups[g].alive) continue;
            if ((axial_flags & kAxialFirstRep) && have[g]) continue;  // MUTANT
            BallCandidate cand;
            Q4Form f4;
            ++st->axial_completion_calls;
            if (!valid_completion(axial[i].u, &cand, &f4)) continue;
            if (!have[g] || ball_candidate_less(cand, best[g])) {
              best[g] = cand;
              have[g] = true;
            }
          }
          if (axial_flags & kAxialVerify) {
            // Reception ETENDUE (audit § 3) : d_j recoupe par le scan
            // q4_power < 0 pour TOUS les groupes en fenetre, groupes
            // MORTS compris — B != 0 garantit un tetraedre non coplanaire
            // (det != 0, orientation canonisee par q4_form), meme si le
            // membre echoue ensuite a l'owner ou au centre.
            for (int j = 0; j < sw.ngroups; ++j) {
              if (!has_vm[j]) continue;
              const Q4Form f4 =
                  q4_form(pa, pb, px, ix.upos[(size_t)vmember[j]]);
              u64 scan = 0;
              for (const CoverPoint& cz : cover)
                if (q4_power(f4, ix.upos[(size_t)cz.u]) < 0) ++scan;
              if (scan != p_perm + sw.pos_before[j] + sw.neg_after[j])
                ++st->axial_verify_mismatch;
            }
          }
          for (int j = 0; j < sw.ngroups; ++j) {
            if (!sw.groups[j].alive || !have[j]) continue;
            out->push_back(best[j]);
            ++st->candidates[2];
            ++st->axial_groups_emitted;
          }
          st->t_emit_ms += std::chrono::duration<double, std::milli>(
                               std::chrono::steady_clock::now() - ta2)
                               .count();
        }
      }
  });
}

// Census EXACT d'une boule par sa forme primitive : interieurs stricts et
// coquille complete. Descente separable par axe (parabole convexe par axe,
// minimum de reseau aux entiers voisins du sommet, maxima aux bornes).
// Retourne false si |I_B| depasse `interior_cap` (boule sans K <= K_max) OU
// si |U_B| depasse `shell_cap` (a traiter en resource_exhausted par
// l'appelant — jamais une troncature silencieuse).
// PASSE COUNT-ONLY DE PROFONDEUR (audit « prefiltre exact par boule ») :
// decide `|I_B| >= h` AVANT toute materialisation de I_B/U_B.
//
// SEUIL EXACT PAR ARITE MINIMALE (audit § 1) : apres le RLE, q_min =
// arite du premier candidat du groupe (le tri met l'arite avant la
// representation) est la cardinalite minimale d'un support de la
// miniboule — tout T d'un evenement du plateau contient un support
// minimal, donc |T| >= q_min, et un evenement utile a K <= K_max exige
// |I_B| <= K_max + 1 - q_min. Regle de mort : |I_B| >= h_qmin =
// K_max + 2 - q_min (10/9/8 interieurs pour q_min = 2/3/4). La regle
// s'appuie sur la completude par lane (en-tete) : une boule au label
// q_min = 4 mais de support reel 2 serait NON pertinente pour q2 (sinon
// la lane q2 l'aurait emise) — la tuer plus tot est exact.
//
// Decisions de descente (les inegalites comptent) :
//   mn >= 0 : aucun point strictement interieur — ELAGUE, y compris les
//             regions de coquille pure (mn == 0) que le census, lui,
//             doit descendre ;
//   mx <  0 : tout le nœud est strictement interieur — range-add sature
//             en O(1), sans allocation. STRICT : a mx == 0 une coquille
//             serait comptee interieure (MUTANT range-add-max-le-zero,
//             tue : des boules a plateau meurent a tort) ;
//   sinon    scission ; a la feuille, test exact (mn == mx).
// Rend true des que count atteint h ; sinon false et *count_out = compte
// EXACT des interieurs stricts (recoupe par la passe census, invariant).
inline bool ball_depth_at_least(const CloudIndex& ix, const Q3BallKey& k,
                                u64 h, u64* count_out,
                                bool mutant_range_add_le = false,
                                BallStreamStats* st = nullptr) {
  *count_out = 0;
  if (ix.nodes.empty()) return h == 0;
  const auto axis_val = [&](int i, i64 t) { return k.a * ((i128)t * t) + k.b[i] * t; };
  const auto axis_min = [&](int i, i64 lo, i64 hi) {
    const i128 num = -k.b[i];
    const i128 den = 2 * k.a;
    i128 q = num / den;
    if (num % den != 0 && ((num < 0) != (den < 0))) --q;
    const i64 t1 = (i64)q;
    i128 best = 0;
    bool first = true;
    for (const i64 cand : {t1, t1 + 1, lo, hi}) {
      const i64 c = std::min(std::max(cand, lo), hi);
      const i128 v = axis_val(i, c);
      if (first || v < best) best = v, first = false;
    }
    return best;
  };
  const auto axis_max = [&](int i, i64 lo, i64 hi) {
    return std::max(axis_val(i, lo), axis_val(i, hi));
  };
  u64 count = 0;
  std::vector<NodeRef> stack{0};
  while (!stack.empty()) {
    const NodeRef z = stack.back();
    stack.pop_back();
    if (st) ++st->prefilter_nodes;
    const AxisBox bz = box_of_node(ix, z);
    i128 mn = k.c, mx = k.c;
    for (int i = 0; i < 3; ++i) {
      mn += axis_min(i, bz.lo[i], bz.hi[i]);
      mx += axis_max(i, bz.lo[i], bz.hi[i]);
    }
    if (mn >= 0) continue;  // rien de STRICTEMENT interieur (coquille incluse)
    if (mx < 0 || (mutant_range_add_le && mx <= 0)) {
      const u64 w = (z < 0) ? 1
                            : (u64)(ix.nodes[(size_t)z].last -
                                    ix.nodes[(size_t)z].first + 1);
      if (st) st->prefilter_range_add_mass += w;
      count += w;
      if (count >= h) return true;
      continue;
    }
    if (z < 0) {
      // Test exact au point (la boite serree d'une feuille rend mn == mx,
      // mais le test ne SUPPOSE pas cette etroitesse).
      if (st) ++st->prefilter_leaf_tests;
      const P3& p = ix.upos[(size_t)(-1 - z)];
      const i128 pw = k.a * p3_norm2(p) + k.b[0] * p.x + k.b[1] * p.y +
                      k.b[2] * p.z + k.c;
      if (pw < 0 && ++count >= h) return true;
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  *count_out = count;
  return false;
}

inline bool ball_census(const CloudIndex& ix, const Q3BallKey& k,
                        size_t interior_cap, size_t shell_cap,
                        std::vector<i32>* interior, std::vector<i32>* shell,
                        bool* shell_overflow) {
  interior->clear();
  shell->clear();
  *shell_overflow = false;
  if (ix.nodes.empty()) return true;
  const auto axis_val = [&](int i, i64 t) { return k.a * ((i128)t * t) + k.b[i] * t; };
  const auto axis_min = [&](int i, i64 lo, i64 hi) {
    const i128 num = -k.b[i];
    const i128 den = 2 * k.a;
    i128 q = num / den;
    if (num % den != 0 && ((num < 0) != (den < 0))) --q;
    const i64 t1 = (i64)q;
    i128 best = 0;
    bool first = true;
    for (const i64 cand : {t1, t1 + 1, lo, hi}) {
      const i64 c = std::min(std::max(cand, lo), hi);
      const i128 v = axis_val(i, c);
      if (first || v < best) best = v, first = false;
    }
    return best;
  };
  const auto axis_max = [&](int i, i64 lo, i64 hi) {
    return std::max(axis_val(i, lo), axis_val(i, hi));
  };
  std::vector<NodeRef> stack{0};
  while (!stack.empty()) {
    const NodeRef z = stack.back();
    stack.pop_back();
    const AxisBox bz = box_of_node(ix, z);
    i128 mn = k.c, mx = k.c;
    for (int i = 0; i < 3; ++i) {
      mn += axis_min(i, bz.lo[i], bz.hi[i]);
      mx += axis_max(i, bz.lo[i], bz.hi[i]);
    }
    if (mn > 0) continue;  // strict : mn == 0 descend (coquilles a voir)
    if (z < 0) {
      const i32 u = -1 - z;
      const P3& p = ix.upos[(size_t)u];
      const i128 pw = k.a * p3_norm2(p) + k.b[0] * p.x + k.b[1] * p.y +
                      k.b[2] * p.z + k.c;
      if (pw < 0) {
        interior->push_back(u);
        if (interior->size() > interior_cap) return false;
      } else if (pw == 0) {
        shell->push_back(u);
        if (shell->size() > shell_cap) {
          *shell_overflow = true;
          return false;
        }
      }
      continue;
    }
    stack.push_back(ix.nodes[(size_t)z].left);
    stack.push_back(ix.nodes[(size_t)z].right);
  }
  return true;
}

}  // namespace mhgp4
