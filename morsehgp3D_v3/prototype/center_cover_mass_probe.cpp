// MorseHGP3D v3 — LE FALSIFICATEUR DE MASSE P15-HOCUDA-P1a : CENTER-COVER q4
// PAR 64 PATCHS, MASS-ONLY (NOTE_SOLUTION_P1A_CENTER_COVER_MASSONLY_20260811,
// note AUDITEE — elle est l'unique autorite de ce fichier).
//
// PORTEE : la lane q4 au SEUIL HUIT, exclusivement. Le probe partitionne
// implicitement les paires, accumule `pruned_mass` et `microtile_mass`, et
// n'emet NI paire, NI ancre, NI support. q3 et son seuil neuf appartiennent au
// futur P1 complet, hors de cette porte.
//
// LE THEOREME (note §1--§2) : pour un bloc croise (A,B) de la partition
// triangulaire T(N) = T(L) sqcup (L x R) sqcup T(R) sur le LBVH Morton
// partage, si CHAQUE patch non certifie infaisable possede huit PointId
// distincts, hors des plages A et B, strictement interieurs pour TOUT centre
// du patch et TOUTE paire du bloc, alors tout support q4 propre positif dont
// l'arete canonique appartient a A x B a au moins huit interieurs stricts :
// p + q >= 12, le bloc est H0-inerte a K = 10, sa masse |A|*|B| est prunee.
//
// LE DOMAINE (note §2) : pour une paire maximale (a,b), le centre c = M + t
// verifie t.d = 0 et r^2 = D^2/4 + ||t||^2 ; Jung en dimension trois donne
// r^2 <= 3 D^2/8, donc ||t||^2 <= D^2/8. Avec X = maxdist^2(A,B) >= D^2 et
//
//     R_t = min{ r dans Z>=0 : 8 r^2 >= X },
//
// le pave T0 = prod_d [ floor((A_lo+B_lo)/2) - R_t, ceil((A_hi+B_hi)/2) + R_t ]
// contient tous les centres pertinents. La racine se calcule par
// isqrt(floor(X/8)) PUIS incrementation ssi 8r^2 < X : la troncature seule
// omet des centres (fixture jung-root : X = 6, isqrt(0) = 0 alors que R_t = 1).
//
// LES 64 PATCHS (note §2) : sur chaque axe entier [l,h], les bornes
// b_j = l + j(h-l)/4, j = 0..4, definissent quatre intervalles FERMES ; les
// produits donnent exactement 64 patchs indexes par
// patch_id = j_x | (j_y << 2) | (j_z << 4). Les coins ont des coordonnees
// C/4 : tout ce fichier travaille en COORDONNEES x4 (entiers C), et les
// distances carrees y sont donc a l'ECHELLE 16. Le facteur correct est seize,
// pas quatre : pour c = 1/4 et S = [0,1], une echelle quatre rend un minimum
// -1/16 et fabrique un faux certificat (mutant clip-scale-4).
//
// LE CERTIFICAT TEMOIN (note §4) : pour un coin C (echelle 4), un cote S dans
// {A,B} et un temoin z,
//
//     Q_d = clip(C_d, [4 S_lo,d, 4 S_hi,d]),
//     F(C,z,S) = somme_d ( (Q_d - C_d)^2 - (4 z_d - C_d)^2 ) > 0
//
// equivaut a ||c-z||^2 < dist^2(c,S). Comme h(c) = dist^2(c,S) - ||c-z||^2 est
// CONCAVE, son minimum sur le patch est atteint a l'un des huit coins : les
// huit coins suffisent. F = 0 ECHOUE (le prédicat est strict). Le cote engage
// suffit : au vrai centre c*, dist^2(c*,S) <= ||c*-a||^2 = r^2.
//
// LES INFAISABILITES (note §3), toutes STRICTES et cumulatives :
//     N_min(T,A) > N_max(T,B)  ou la symetrique      (aucun mediateur)
//     N_min(T,A) > 6X  ou  N_min(T,B) > 6X           (Jung q4 : 16 r^2 <= 6X)
//     N_mid(T)   > 2X                                (||t||^2 <= X/8)
// Zero, doute ou preuve absente laisse un PATCH SURVIVANT (fail-open).
//
// LA MACHINE (note §6) : MICROTUILE AVANT COVER. Une tache de masse <= mu est
// commise dans P_microtile SANS evaluer les 64 patchs ; un self-bloc non
// terminal est remplace par ses deux self-enfants et leur produit croise (le
// prune diagonal reste NON active : ni preuve ni fixture) ; un produit croise
// non terminal est prune ou partage. Le ledger transactionnel ferme en
// permanence P_prune + P_microtile + P_pending = C(n,2).
//
// LE JUGE (note §7, n <= 32) : il ne partage ni le decoupage, ni `clip`, ni
// les bornes du sujet. Il rejoue chaque preuve d'infaisabilite, chaque temoin
// contre CHAQUE POINT REEL du cote engage aux huit coins en i128, enumere les
// q4 propres positifs par un oracle determinantal INDEPENDANT (Cramer exact,
// barycentriques strictement positives) et exige que tout support de moins de
// huit interieurs stricts ait son arete canonique HORS de la masse prunee.
// Il developpe enfin blocs prunes et microtuiles et exige exactement un sort
// par paire : une omission compensee par un doublon meurt ici, la ou une
// egalite scalaire de masse la laisserait passer.
//
// Codes : 0 OK ; 1 violation ; 2 CLI ; 3 plancher/budget ; 4 mutant tue.
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "mhgp/miniball.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/morton_lbvh.hpp"

namespace {

using i64 = long long;
using i128 = __int128;
using Node = mhgp3v::LbvhNode;
using Lbvh = mhgp3v::MortonLbvh;

constexpr int kArity = 4;        // q4 exclusivement
constexpr int kThreshold = 8;    // K + 2 - q = 12 - 4
constexpr int kPatches = 64;

// ---------------------------------------------------------------------------
// ARITHMETIQUE ENTIERE EXACTE. Bornes u16 (note §4) : X <= 3*65535^2,
// R_t <= 40132, |Q_d - C_d| et |4 z_d - C_d| <= 422668, donc |F| < 2^39 : i64
// suffit au sujet. Chaque operande est elargi AVANT soustraction, carre ou
// produit — un resultat final borne ne rend pas un intermediaire int32
// acceptable.
// ---------------------------------------------------------------------------
inline i64 isqrt_i64(i64 v) {
  if (v <= 0) return 0;
  i64 r = (i64)std::sqrt((double)v);
  while (r > 0 && r * r > v) --r;
  while ((r + 1) * (r + 1) <= v) ++r;
  return r;
}

// R_t = min{ r >= 0 : 8 r^2 >= X }. La troncature seule est le mutant
// ceil-root-truncated.
inline i64 radius_t(i64 x, bool truncated_mutant) {
  i64 r = isqrt_i64(x / 8);
  if (truncated_mutant) return r;
  while (8 * r * r < x) ++r;
  return r;
}

// R_w = min{ r >= 0 : 8 r^2 >= 3X } : tout temoin strict d'un vrai centre
// verifie ||z-c||^2 < r^2 <= 3X/8 (note §5).
inline i64 radius_w(i64 x) {
  i64 r = isqrt_i64((3 * x) / 8);
  while (8 * r * r < 3 * x) ++r;
  return r;
}

// Une boite entiere generique. Les boites de POINTS sont a l'echelle 1 ; les
// patchs et les boites derivees sont a l'ECHELLE 4.
struct Box {
  i64 lo[3] = {0, 0, 0};
  i64 hi[3] = {0, 0, 0};
};

inline Box box_of_node(const Node& node) {
  Box b;
  for (int d = 0; d < 3; ++d) {
    b.lo[d] = node.lo[d];
    b.hi[d] = node.hi[d];
  }
  return b;
}

inline Box scale4(const Box& b) {
  Box out;
  for (int d = 0; d < 3; ++d) {
    out.lo[d] = 4 * b.lo[d];
    out.hi[d] = 4 * b.hi[d];
  }
  return out;
}

// dist^2 minimale entre deux boites, exacte et separable.
inline i64 box_dist2(const Box& u, const Box& v) {
  i64 total = 0;
  for (int d = 0; d < 3; ++d) {
    if (u.lo[d] > v.hi[d]) {
      const i64 gap = u.lo[d] - v.hi[d];
      total += gap * gap;
    } else if (v.lo[d] > u.hi[d]) {
      const i64 gap = v.lo[d] - u.hi[d];
      total += gap * gap;
    }
  }
  return total;
}

// dist^2 maximale entre deux boites, exacte (atteinte a des coins opposes).
inline i64 box_maxdist2(const Box& u, const Box& v) {
  i64 total = 0;
  for (int d = 0; d < 3; ++d) {
    const i64 left = u.lo[d] - v.hi[d];
    const i64 right = u.hi[d] - v.lo[d];
    const i64 a = left < 0 ? -left : left;
    const i64 b = right < 0 ? -right : right;
    const i64 far = a > b ? a : b;
    total += far * far;
  }
  return total;
}

// dist^2 d'un POINT (echelle 4) a une boite (echelle 4) : la forme `clip` du
// certificat temoin, ecrite une seule fois.
inline i64 point_box_dist2_scaled(const i64 c[3], const Box& s4) {
  i64 total = 0;
  for (int d = 0; d < 3; ++d) {
    i64 q = c[d];
    if (q < s4.lo[d]) q = s4.lo[d];
    if (q > s4.hi[d]) q = s4.hi[d];
    const i64 delta = q - c[d];
    total += delta * delta;
  }
  return total;
}

// dist^2 MAXIMALE d'un point (echelle 4) a une boite (echelle 4) : le
// majorant E_W(C) du certificat par sous-arbre.
inline i64 point_box_maxdist2_scaled(const i64 c[3], const Box& w4) {
  i64 total = 0;
  for (int d = 0; d < 3; ++d) {
    const i64 left = c[d] - w4.lo[d];
    const i64 right = w4.hi[d] - c[d];
    const i64 a = left < 0 ? -left : left;
    const i64 b = right < 0 ? -right : right;
    const i64 far = a > b ? a : b;
    total += far * far;
  }
  return total;
}

// ---------------------------------------------------------------------------
// LE DOMAINE T0 ET SES 64 PATCHS (note §2), en coordonnees x4.
// ---------------------------------------------------------------------------
struct BlockGeometry {
  i64 x = 0;             // maxdist^2(A,B)
  i64 radius_t = 0;
  i64 radius_w = 0;
  i64 t0_lo4[3] = {0, 0, 0};   // T0 a l'echelle 4
  i64 t0_hi4[3] = {0, 0, 0};
  i64 bound4[3][5] = {};       // les cinq bornes b_j de chaque axe, echelle 4
  Box mid4;                    // la boite des milieux, echelle 4
};

inline i64 floor_div2(i64 v) { return v >= 0 ? v / 2 : -((-v + 1) / 2); }
inline i64 ceil_div2(i64 v) { return v >= 0 ? (v + 1) / 2 : -((-v) / 2); }

inline BlockGeometry block_geometry(const Box& a, const Box& b, bool truncated_root,
                                    bool q3_radius_mutant) {
  BlockGeometry g;
  g.x = box_maxdist2(a, b);
  // MUTANT q4-radius-replaced-by-q3 : le disque q3 (D^2/12) sous-majore le
  // domaine q4 (D^2/8) et omet de vrais centres.
  g.radius_t = q3_radius_mutant ? radius_t(g.x * 8 / 12, truncated_root)
                                : radius_t(g.x, truncated_root);
  g.radius_w = radius_w(g.x);
  for (int d = 0; d < 3; ++d) {
    const i64 lo = floor_div2(a.lo[d] + b.lo[d]) - g.radius_t;
    const i64 hi = ceil_div2(a.hi[d] + b.hi[d]) + g.radius_t;
    g.t0_lo4[d] = 4 * lo;
    g.t0_hi4[d] = 4 * hi;
    // b_j = l + j (h-l)/4 ; a l'echelle 4 : 4l + j (h-l), entier exact.
    for (int j = 0; j <= 4; ++j) g.bound4[d][j] = 4 * lo + (i64)j * (hi - lo);
    // La boite des milieux : [(A_lo+B_lo)/2, (A_hi+B_hi)/2], echelle 4 =
    // [2(A_lo+B_lo), 2(A_hi+B_hi)] — exact, sans division.
    g.mid4.lo[d] = 2 * (a.lo[d] + b.lo[d]);
    g.mid4.hi[d] = 2 * (a.hi[d] + b.hi[d]);
  }
  return g;
}

inline Box patch_box(const BlockGeometry& g, int patch_id) {
  const int j[3] = {patch_id & 3, (patch_id >> 2) & 3, (patch_id >> 4) & 3};
  Box p;
  for (int d = 0; d < 3; ++d) {
    p.lo[d] = g.bound4[d][j[d]];
    p.hi[d] = g.bound4[d][j[d] + 1];
    if (p.hi[d] < p.lo[d]) std::swap(p.lo[d], p.hi[d]);   // pave degenere
  }
  return p;
}

// Les huit coins d'un patch, echelle 4.
inline void patch_corners(const Box& p, i64 corners[8][3]) {
  for (int k = 0; k < 8; ++k)
    for (int d = 0; d < 3; ++d)
      corners[k][d] = (k >> d) & 1 ? p.hi[d] : p.lo[d];
}

enum class PatchStatus : std::uint8_t {
  kUnknown = 0,
  kInfeasibleMediator = 1,   // N_min(T,A) > N_max(T,B) ou symetrique
  kInfeasibleJung = 2,       // N_min(T,S) > 6X
  kInfeasibleMidpoint = 3,   // N_mid(T) > 2X
  kCovered = 4,              // huit temoins certifies
  kSurviving = 5,            // ni infaisable, ni couvert
};

inline bool status_is_infeasible(PatchStatus s) {
  return s == PatchStatus::kInfeasibleMediator || s == PatchStatus::kInfeasibleJung ||
         s == PatchStatus::kInfeasibleMidpoint;
}

// LES TROIS PREUVES STRICTES D'INFAISABILITE (note §3). Une egalite, un doute
// ou une preuve absente rend kUnknown : le patch SURVIT (fail-open).
// `strict_to_large` remplace chaque `>` par `>=` : le mutant supprime des
// patchs qui portent de vrais centres (la fixture jung-equality le tue —
// elle realise N_mid = 2X exactement).
inline PatchStatus patch_infeasibility(const BlockGeometry& g, const Box& patch,
                                       const Box& a4, const Box& b4,
                                       bool strict_to_large) {
  const auto gt = [&](i64 u, i64 v) { return strict_to_large ? u >= v : u > v; };
  const i64 n_min_a = box_dist2(patch, a4);
  const i64 n_max_a = box_maxdist2(patch, a4);
  const i64 n_min_b = box_dist2(patch, b4);
  const i64 n_max_b = box_maxdist2(patch, b4);
  if (gt(n_min_a, n_max_b) || gt(n_min_b, n_max_a))
    return PatchStatus::kInfeasibleMediator;
  if (gt(n_min_a, 6 * g.x) || gt(n_min_b, 6 * g.x)) return PatchStatus::kInfeasibleJung;
  if (gt(box_dist2(patch, g.mid4), 2 * g.x)) return PatchStatus::kInfeasibleMidpoint;
  return PatchStatus::kUnknown;
}

// LE CERTIFICAT TEMOIN PONCTUEL, echelle 16 : F > 0 aux huit coins.
// `corner_skipped` n'evalue que sept coins ; `witness_strict_to_large` admet
// F = 0. Les deux fabriquent des credits faux.
inline bool witness_covers_patch(const i64 corners[8][3], const i64 z4[3],
                                 const Box& side4, bool corner_skipped,
                                 bool strict_to_large, bool clip_scale_4,
                                 i64* corner_evals) {
  const int limit = corner_skipped ? 7 : 8;
  for (int k = 0; k < limit; ++k) {
    if (corner_evals != nullptr) ++*corner_evals;
    const i64 d_side = point_box_dist2_scaled(corners[k], side4);
    i64 to_witness = 0;
    for (int d = 0; d < 3; ++d) {
      // MUTANT clip-scale-4 : la distance au temoin est mesuree a l'echelle
      // QUATRE (celle des marges affines) alors que `d_side` est a l'echelle
      // SEIZE. Le temoin parait quatre fois plus proche : credits et prunes
      // faux. L'exemple unidimensionnel de la note (c = 1/4, S = [0,1],
      // minimum -1/16) refute cette echelle.
      const i64 delta = clip_scale_4 ? (corners[k][d] / 2 - 2 * (z4[d] / 4))
                                     : (corners[k][d] - z4[d]);
      to_witness += delta * delta;
    }
    const bool ok = strict_to_large ? d_side >= to_witness : d_side > to_witness;
    if (!ok) return false;
  }
  return true;
}

// LE CERTIFICAT PAR SOUS-ARBRE (note §5) : si E_W(C) < D_S(C) aux huit coins,
// TOUS les points de W sont temoins pour ce patch et sa plage credite la
// banque. Les plages acceptees sont disjointes par patch (antichaine).
inline bool node_covers_patch(const i64 corners[8][3], const Box& w4, const Box& side4,
                              bool corner_skipped, bool strict_to_large,
                              bool clip_scale_4, i64* corner_evals) {
  const int limit = corner_skipped ? 7 : 8;
  for (int k = 0; k < limit; ++k) {
    if (corner_evals != nullptr) ++*corner_evals;
    const i64 d_side = point_box_dist2_scaled(corners[k], side4);
    i64 e_w = point_box_maxdist2_scaled(corners[k], w4);
    if (clip_scale_4) e_w /= 4;   // MUTANT clip-scale-4 (voir ci-dessus)
    const bool ok = strict_to_large ? d_side >= e_w : d_side > e_w;
    if (!ok) return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// LES INJECTIONS, LES REÇUS ET LE LEDGER.
// ---------------------------------------------------------------------------
struct Injections {
  bool clip_scale_4 = false;              // l'echelle quatre au lieu de seize
  bool ceil_root_truncated = false;       // isqrt sans incrementation
  bool patch_omitted = false;             // le patch 63 n'est jamais controle
  bool feasible_declared_infeasible = false;  // un patch survivant est efface
  bool strict_to_large = false;           // infaisabilite `>` devient `>=`
  bool corner_skipped = false;            // sept coins sur huit
  bool witness_duplicated = false;        // un PointId credite deux fois
  bool witness_strict_to_large = false;   // temoin admis a F = 0
  bool witness_subtrees_overlap = false;  // deux plages temoins se recouvrent
  bool endpoint_overlap_subtracted = false;  // masse retiree sans reconstruire
  bool stale_inherited_credit = false;    // bit de patch parent non recertifie
  bool q4_radius_replaced_by_q3 = false;  // domaine sous-majore
  bool microtile_truncated = false;       // masse microtuile tronquee
  bool terminal_compensated = false;      // une paire omise, une dupliquee
  bool cover_before_terminal = false;     // les 64 patchs avant la microtuile
  bool any() const {
    return clip_scale_4 || ceil_root_truncated || patch_omitted ||
           feasible_declared_infeasible || strict_to_large || corner_skipped ||
           witness_duplicated || witness_strict_to_large || witness_subtrees_overlap ||
           endpoint_overlap_subtracted || stale_inherited_credit ||
           q4_radius_replaced_by_q3 || microtile_truncated || terminal_compensated ||
           cover_before_terminal;
  }
};

// Le crédit d'un patch : le PointId, le cote engage et la plage du noeud
// accepte (pour le rejeu de la decomposition disjointe).
struct PatchCredit {
  int point_id = -1;
  int side = 0;              // 0 = A, 1 = B
  int node_begin = -1;       // plage acceptee (positions Morton)
  int node_end = -1;
};

struct PatchReceipt {
  PatchStatus status = PatchStatus::kUnknown;
  std::vector<PatchCredit> credits;
};

// Le reçu complet d'un bloc prune (mode oracle seulement).
struct BlockReceipt {
  int a_begin = 0, a_end = 0, b_begin = 0, b_end = 0;
  i64 x = 0, radius_t = 0, radius_w = 0;
  std::array<PatchReceipt, kPatches> patches;
};

// Le reçu d'une microtuile (mode oracle) : la bijection des sorts l'exige.
struct MicrotileReceipt {
  int a_begin = 0, a_end = 0, b_begin = 0, b_end = 0;
  bool is_self = false;
};

struct MassReceipt {
  i64 blocks_tried = 0;         // produits croises soumis au cover
  i64 block_prunes = 0;
  i128 pruned_mass = 0;
  i64 microtiles = 0;
  i128 microtile_mass = 0;
  i64 splits = 0;
  i64 self_splits = 0;
  i64 patch_slots = 0;          // 64 * blocks_tried (identite logique)
  i64 patch_infeasible[3] = {0, 0, 0};   // mediateur, Jung, milieux
  i64 patch_surviving = 0;
  i64 patch_covered = 0;
  i64 witness_node_visits = 0;  // visites (noeud temoin, masque)
  i64 witness_point_tests = 0;
  i64 corner_evals = 0;
  i64 accepted_nodes = 0;       // sous-arbres credites en bloc
  i64 ambiguous_leaves = 0;
  i64 prefilter_skips = 0;      // blocs sans huit tiers possibles
  i64 stack_high_water = 0;
  i64 max_block_mass = 0;
};

// ---------------------------------------------------------------------------
// LA MACHINE : partition triangulaire, MICROTUILE AVANT COVER, ledger
// transactionnel. Aucune allocation par bloc : arenes et scratch reutilises.
// ---------------------------------------------------------------------------
struct CoverMachine {
  const Lbvh* tree = nullptr;
  Injections injections;
  MassReceipt receipt;
  i64 microtile = 64;
  i64 max_states = 0;
  bool budget_exceeded = false;
  bool oracle_mode = false;
  std::vector<BlockReceipt>* block_receipts = nullptr;
  std::vector<MicrotileReceipt>* microtile_receipts = nullptr;

  // La tache : un self-bloc (a_node seul) ou un produit croise.
  struct Task {
    int a_node = 0;
    int b_node = -1;   // < 0 : self-bloc
  };
  std::vector<Task> stack_;
  // Scratch du cover, reutilise d'un bloc a l'autre.
  std::array<PatchStatus, kPatches> status_{};
  std::array<int, kPatches> counts_{};
  std::array<Box, kPatches> patch_boxes_{};
  i64 corners_[kPatches][8][3] = {};
  std::array<Box, kPatches> crop_{};
  std::vector<std::pair<int, unsigned long long>> witness_stack_;
  std::array<std::vector<PatchCredit>, kPatches> credits_;
  i128 pending_ = 0;

  static i128 self_mass(i64 m) { return (i128)m * (m - 1) / 2; }

  i64 node_size(int index) const {
    const Node& node = tree->nodes[(std::size_t)index];
    return node.end - node.begin;
  }

  i128 task_mass(const Task& task) const {
    if (task.b_node < 0) return self_mass(node_size(task.a_node));
    return (i128)node_size(task.a_node) * node_size(task.b_node);
  }

  // Le cover d'un produit croise : rend vrai si les 64 patchs sont soit
  // infaisables, soit couverts au seuil huit.
  bool cover_block(const Node& a, const Node& b) {
    ++receipt.blocks_tried;
    receipt.patch_slots += kPatches;
    const Box a_box = box_of_node(a), b_box = box_of_node(b);
    const Box a4 = scale4(a_box), b4 = scale4(b_box);
    const BlockGeometry geom = block_geometry(a_box, b_box, injections.ceil_root_truncated,
                                              injections.q4_radius_replaced_by_q3);
    // Le PREFILTRE (note §5) : sans huit tiers possibles hors de A et B,
    // aucun cover n'est atteignable. Il ne s'applique qu'aux produits croises
    // de plages DISJOINTES ; additions et soustractions sont verifiees en i64.
    const i64 outside =
        (i64)tree->order.size() - (i64)(a.end - a.begin) - (i64)(b.end - b.begin);
    if (outside < kThreshold) {
      ++receipt.prefilter_skips;
      return false;
    }
    int pending_patches = 0;
    for (int p = 0; p < kPatches; ++p) {
      // MUTANT stale-inherited-credit : le credit du bloc PARENT survit sans
      // recertification des coins de l'enfant (les 64 grilles different).
      if (!injections.stale_inherited_credit) counts_[(std::size_t)p] = 0;
      credits_[(std::size_t)p].clear();
      patch_boxes_[(std::size_t)p] = patch_box(geom, p);
      // MUTANT patch-omitted : le dernier patch n'est jamais controle et
      // compte comme couvert — un vrai centre y survivrait sans temoin.
      if (injections.patch_omitted && p == kPatches - 1) {
        status_[(std::size_t)p] = PatchStatus::kCovered;
        continue;
      }
      status_[(std::size_t)p] = patch_infeasibility(geom, patch_boxes_[(std::size_t)p],
                                                    a4, b4, injections.strict_to_large);
      // MUTANT feasible-declared-infeasible : un patch survivant est efface.
      if (injections.feasible_declared_infeasible &&
          status_[(std::size_t)p] == PatchStatus::kUnknown) {
        status_[(std::size_t)p] = PatchStatus::kInfeasibleJung;
      }
      if (status_is_infeasible(status_[(std::size_t)p])) {
        ++receipt.patch_infeasible[(int)status_[(std::size_t)p] - 1];
        continue;
      }
      patch_corners(patch_boxes_[(std::size_t)p], corners_[p]);
      // Le crop PAR PATCH (plus serre que T0 dilate) : T_i (+/-) R_w, en
      // coordonnees ENTIERES (echelle 1) pour tester l'AABB des noeuds.
      for (int d = 0; d < 3; ++d) {
        crop_[(std::size_t)p].lo[d] =
            floor_div4(patch_boxes_[(std::size_t)p].lo[d]) - geom.radius_w;
        crop_[(std::size_t)p].hi[d] =
            ceil_div4(patch_boxes_[(std::size_t)p].hi[d]) + geom.radius_w;
      }
      ++pending_patches;
    }
    if (pending_patches > 0) collect_witnesses(a, b, a4, b4);
    bool all_closed = true;
    for (int p = 0; p < kPatches; ++p) {
      if (status_is_infeasible(status_[(std::size_t)p]) ||
          status_[(std::size_t)p] == PatchStatus::kCovered)
        continue;
      if (counts_[(std::size_t)p] >= kThreshold) {
        status_[(std::size_t)p] = PatchStatus::kCovered;
        ++receipt.patch_covered;
      } else {
        status_[(std::size_t)p] = PatchStatus::kSurviving;
        ++receipt.patch_surviving;
        all_closed = false;
      }
    }
    if (all_closed && oracle_mode && block_receipts != nullptr) {
      BlockReceipt r;
      r.a_begin = a.begin; r.a_end = a.end;
      r.b_begin = b.begin; r.b_end = b.end;
      r.x = geom.x; r.radius_t = geom.radius_t; r.radius_w = geom.radius_w;
      for (int p = 0; p < kPatches; ++p) {
        r.patches[(std::size_t)p].status = status_[(std::size_t)p];
        r.patches[(std::size_t)p].credits = credits_[(std::size_t)p];
      }
      block_receipts->push_back(std::move(r));
    }
    return all_closed;
  }

  static i64 floor_div4(i64 v) { return v >= 0 ? v / 4 : -((-v + 3) / 4); }
  static i64 ceil_div4(i64 v) { return v >= 0 ? (v + 3) / 4 : -((-v) / 4); }

  static bool boxes_intersect(const Box& u, const Box& v) {
    for (int d = 0; d < 3; ++d)
      if (u.hi[d] < v.lo[d] || v.hi[d] < u.lo[d]) return false;
    return true;
  }

  // Crediter un patch avec des PointId CANONIQUES : les premiers de la plage
  // Morton residuelle, jusqu'a combler le seuil huit.
  void credit_patch(int p, int begin, int end, int side, int node_begin, int node_end) {
    int slot = counts_[(std::size_t)p];
    for (int t = begin; t < end && slot < kThreshold; ++t, ++slot) {
      if (oracle_mode) {
        PatchCredit credit;
        credit.point_id = tree->order[(std::size_t)t];
        credit.side = side;
        credit.node_begin = node_begin;
        credit.node_end = node_end;
        credits_[(std::size_t)p].push_back(credit);
        // MUTANT witness-duplicated : le meme PointId est credite deux fois.
        if (injections.witness_duplicated && slot + 1 < kThreshold) {
          credits_[(std::size_t)p].push_back(credit);
          ++slot;
        }
      } else if (injections.witness_duplicated && slot + 1 < kThreshold) {
        ++slot;
      }
    }
    counts_[(std::size_t)p] = slot;
  }

  // LA RANGE-QUERY COLLECTIVE (note §5) : des etats (noeud temoin, masque de
  // patchs actifs), un crop PAR PATCH, des sous-arbres acceptes en bloc
  // formant une antichaine par patch, et le test ponctuel aux feuilles. Un
  // noeud qui chevauche A ou B DESCEND jusqu'a des sous-arbres reellement
  // disjoints ; il n'est jamais credite par soustraction de masse.
  void collect_witnesses(const Node& a, const Node& b, const Box& a4, const Box& b4) {
    unsigned long long initial = 0;
    for (int p = 0; p < kPatches; ++p)
      if (status_[(std::size_t)p] == PatchStatus::kUnknown &&
          counts_[(std::size_t)p] < kThreshold)
        initial |= 1ULL << p;
    if (initial == 0) return;
    witness_stack_.clear();
    witness_stack_.push_back({0, initial});
    while (!witness_stack_.empty()) {
      receipt.stack_high_water =
          std::max(receipt.stack_high_water, (i64)witness_stack_.size());
      const auto [node_index, incoming] = witness_stack_.back();
      witness_stack_.pop_back();
      const Node& node = tree->nodes[(std::size_t)node_index];
      ++receipt.witness_node_visits;
      const Box node_box = box_of_node(node);
      const Box node4 = scale4(node_box);
      unsigned long long mask = incoming;
      // Filtrage par le crop PAR PATCH et par les patchs deja combles.
      unsigned long long active = 0;
      for (unsigned long long m = mask; m != 0; m &= m - 1) {
        const int p = __builtin_ctzll(m);
        if (counts_[(std::size_t)p] >= kThreshold) continue;
        if (!boxes_intersect(node_box, crop_[(std::size_t)p])) continue;
        active |= 1ULL << p;
      }
      if (active == 0) continue;
      const bool overlaps_endpoints =
          (node.begin < a.end && a.begin < node.end) ||
          (node.begin < b.end && b.begin < node.end);
      if (!overlaps_endpoints) {
        // Le noeud est DISJOINT des deux plages : ses points sont tous des
        // temoins licites. On tente de crediter le sous-arbre ENTIER.
        unsigned long long remaining = active;
        for (unsigned long long m = active; m != 0; m &= m - 1) {
          const int p = __builtin_ctzll(m);
          for (int side = 0; side < 2; ++side) {
            const Box& side4 = side == 0 ? a4 : b4;
            if (!node_covers_patch(corners_[p], node4, side4, injections.corner_skipped,
                                   injections.witness_strict_to_large,
                                   injections.clip_scale_4, &receipt.corner_evals))
              continue;
            ++receipt.accepted_nodes;
            credit_patch(p, node.begin, node.end, side, node.begin, node.end);
            // L'ANTICHAINE : le sous-arbre credité est consomme pour ce
            // patch et n'est plus descendu. MUTANT witness-subtrees-overlap :
            // le bit reste actif et les memes feuilles creditent a nouveau.
            if (!injections.witness_subtrees_overlap) remaining &= ~(1ULL << p);
            break;
          }
        }
        active = remaining;
        if (active == 0) continue;
      } else if (injections.endpoint_overlap_subtracted && node.left < 0) {
        // MUTANT endpoint-overlap-subtracted : la masse chevauchante est
        // retiree sans reconstruire la plage residuelle — des PointId
        // d'extremite peuvent etre credites.
        for (unsigned long long m = active; m != 0; m &= m - 1) {
          const int p = __builtin_ctzll(m);
          credit_patch(p, node.begin, node.end, 0, node.begin, node.end);
        }
        continue;
      }
      if (node.left < 0) {
        ++receipt.ambiguous_leaves;
        for (int t = node.begin; t < node.end; ++t) {
          const bool in_a = t >= a.begin && t < a.end;
          const bool in_b = t >= b.begin && t < b.end;
          if (in_a || in_b) continue;   // jamais un PointId d'extremite
          const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
          const i64 z4[3] = {4 * (i64)w.x, 4 * (i64)w.y, 4 * (i64)w.z};
          for (unsigned long long m = active; m != 0; m &= m - 1) {
            const int p = __builtin_ctzll(m);
            if (counts_[(std::size_t)p] >= kThreshold) continue;
            ++receipt.witness_point_tests;
            for (int side = 0; side < 2; ++side) {
              const Box& side4 = side == 0 ? a4 : b4;
              if (!witness_covers_patch(corners_[p], z4, side4, injections.corner_skipped,
                                        injections.witness_strict_to_large,
                                        injections.clip_scale_4, &receipt.corner_evals))
                continue;
              credit_patch(p, t, t + 1, side, t, t + 1);
              break;
            }
          }
        }
        continue;
      }
      witness_stack_.push_back({node.right, active});
      witness_stack_.push_back({node.left, active});
    }
  }

  // Le cote a partager : un seul interne le designe ; sinon le plus grand
  // couple (span, cardinalite), tie-break par le plus petit NodeId.
  bool split_left_side(int ia, int ib) const {
    const Node& a = tree->nodes[(std::size_t)ia];
    const Node& b = tree->nodes[(std::size_t)ib];
    const bool a_internal = a.left >= 0, b_internal = b.left >= 0;
    if (a_internal && !b_internal) return true;
    if (!a_internal && b_internal) return false;
    const auto span_of = [](const Node& node) {
      i64 best = 0;
      for (int d = 0; d < 3; ++d) best = std::max(best, node.hi[d] - node.lo[d]);
      return best;
    };
    const i64 sa = span_of(a), sb = span_of(b);
    if (sa != sb) return sa > sb;
    const i64 ca = a.end - a.begin, cb = b.end - b.begin;
    if (ca != cb) return ca > cb;
    return ia <= ib;
  }

  // LE RUN : partition triangulaire, MICROTUILE AVANT COVER, ledger
  // transactionnel P_prune + P_microtile + P_pending = C(n,2).
  bool run() {
    const i64 n = (i64)tree->order.size();
    pending_ = self_mass(n);
    stack_.clear();
    stack_.push_back({0, -1});
    i64 states = 0;
    while (!stack_.empty()) {
      receipt.stack_high_water = std::max(receipt.stack_high_water, (i64)stack_.size());
      const Task task = stack_.back();
      stack_.pop_back();
      if (max_states > 0 && ++states > max_states) {
        budget_exceeded = true;
        return false;
      }
      const i128 mass = task_mass(task);
      if (mass == 0) continue;
      receipt.max_block_mass = std::max(receipt.max_block_mass, (i64)mass);
      const Node& a = tree->nodes[(std::size_t)task.a_node];
      // LA POLITIQUE : microtuile AVANT cover. Le mutant cover-before-terminal
      // evalue les 64 patchs meme sur une tache terminale — chaque microtuile
      // paie alors exactement le travail que le terminal devait eviter.
      if (injections.cover_before_terminal && task.b_node >= 0 && mass <= microtile)
        (void)cover_block(a, tree->nodes[(std::size_t)task.b_node]);
      if (mass <= microtile) {
        ++receipt.microtiles;
        i128 committed = mass;
        // MUTANT microtile-truncated : une unite de masse disparait.
        if (injections.microtile_truncated && committed > 0) --committed;
        receipt.microtile_mass += committed;
        pending_ -= mass;
        if (oracle_mode && microtile_receipts != nullptr) {
          MicrotileReceipt r;
          r.is_self = task.b_node < 0;
          r.a_begin = a.begin; r.a_end = a.end;
          if (task.b_node >= 0) {
            const Node& b = tree->nodes[(std::size_t)task.b_node];
            r.b_begin = b.begin; r.b_end = b.end;
          }
          microtile_receipts->push_back(r);
        }
        continue;
      }
      if (task.b_node < 0) {
        // SELF-BLOC : jamais prune (ni preuve ni fixture du prune diagonal) ;
        // remplace par ses deux self-enfants et leur produit croise.
        if (a.left < 0) {
          std::printf("ECHEC : une feuille self depasse le seuil microtuile —"
                      " le preflight mu >= leaf_size^2 est viole\n");
          budget_exceeded = true;
          return false;
        }
        ++receipt.self_splits;
        const i128 left = self_mass(node_size(a.left));
        const i128 right = self_mass(node_size(a.right));
        const i128 cross = (i128)node_size(a.left) * node_size(a.right);
        if (left + cross + right != mass) {
          std::printf("ECHEC : l'identite de split self est violee\n");
          budget_exceeded = true;
          return false;
        }
        stack_.push_back({a.right, -1});
        stack_.push_back({a.left, a.right});
        stack_.push_back({a.left, -1});
        continue;
      }
      const Node& b = tree->nodes[(std::size_t)task.b_node];
      if (cover_block(a, b)) {
        ++receipt.block_prunes;
        receipt.pruned_mass += mass;
        pending_ -= mass;
        continue;
      }
      ++receipt.splits;
      const bool split_a = split_left_side(task.a_node, task.b_node);
      const Node& target = split_a ? a : b;
      if (target.left < 0) {
        std::printf("ECHEC : aucun cote partageable pour une masse superieure au"
                    " seuil microtuile\n");
        budget_exceeded = true;
        return false;
      }
      const i128 child_left = split_a
          ? (i128)node_size(target.left) * node_size(task.b_node)
          : (i128)node_size(task.a_node) * node_size(target.left);
      const i128 child_right = split_a
          ? (i128)node_size(target.right) * node_size(task.b_node)
          : (i128)node_size(task.a_node) * node_size(target.right);
      if (child_left + child_right != mass) {
        std::printf("ECHEC : l'identite de split croise est violee\n");
        budget_exceeded = true;
        return false;
      }
      if (split_a) {
        stack_.push_back({target.right, task.b_node});
        stack_.push_back({target.left, task.b_node});
      } else {
        stack_.push_back({task.a_node, target.right});
        stack_.push_back({task.a_node, target.left});
      }
    }
    return true;
  }

  i128 pending() const { return pending_; }
};

// ---------------------------------------------------------------------------
// L'ORACLE DETERMINANTAL INDEPENDANT (note §7). Il ne partage aucune primitive
// avec le sujet : sphere circonscrite par Cramer exact, positivite par quatre
// orient3d, profondeur stricte par la FORME AFFINE de la puissance.
//
// Bornes u16, toutes verifiees : A ~ 2^18 donc det ~ 2^54 ; b ~ 2^34 donc les
// numerateurs ~ 2^70 ; orient3d melange 2^17 x 2^17 x 2^70 ~ 2^104 ; la forme
// affine de puissance melange 2^54 x 2^34 et 2^70 x 2^17, soit ~ 2^88. Tout
// tient dans i128 avec marge — jamais la forme ||den*x - cn||^2, qui
// atteindrait 2^140.
// ---------------------------------------------------------------------------
namespace judge {

struct Sphere {
  i128 cn[3] = {0, 0, 0};   // centre = cn / den
  i128 den = 0;
};

inline i128 det3(const i128 m[3][3]) {
  return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
         m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
         m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

inline i128 coord_of(const mhgp::P3& p, int d) {
  return d == 0 ? (i128)p.x : (d == 1 ? (i128)p.y : (i128)p.z);
}

// La sphere circonscrite de quatre points, par Cramer exact. Rend faux sur un
// quadruplet coplanaire (determinant nul).
inline bool circumsphere(const mhgp::P3* q, Sphere* out) {
  i128 a[3][3], b[3];
  for (int i = 0; i < 3; ++i) {
    i128 norm = 0, norm0 = 0;
    for (int d = 0; d < 3; ++d) {
      a[i][d] = 2 * (coord_of(q[i + 1], d) - coord_of(q[0], d));
      norm += coord_of(q[i + 1], d) * coord_of(q[i + 1], d);
      norm0 += coord_of(q[0], d) * coord_of(q[0], d);
    }
    b[i] = norm - norm0;
  }
  const i128 den = det3(a);
  if (den == 0) return false;
  for (int c = 0; c < 3; ++c) {
    i128 m[3][3];
    for (int i = 0; i < 3; ++i)
      for (int d = 0; d < 3; ++d) m[i][d] = d == c ? b[i] : a[i][d];
    out->cn[c] = det3(m);
  }
  out->den = den;
  return true;
}

// Le signe de la puissance de `x` : negatif si STRICTEMENT interieur.
// Forme affine : den*(||x||^2 - ||ref||^2) - 2*cn.(x - ref), fois le signe de
// den. `ref` est un point de la sphere (un sommet du support).
inline int power_sign(const Sphere& s, const mhgp::P3& x, const mhgp::P3& ref) {
  i128 value = 0;
  for (int d = 0; d < 3; ++d) {
    const i128 xd = coord_of(x, d), rd = coord_of(ref, d);
    value += s.den * (xd * xd - rd * rd) - 2 * s.cn[d] * (xd - rd);
  }
  if (s.den < 0) value = -value;
  if (value < 0) return -1;
  if (value == 0) return 0;
  return 1;
}

// orient3d exact d'un point RATIONNEL cn/den contre le plan (u,v,w).
inline int orient_rational(const mhgp::P3& u, const mhgp::P3& v, const mhgp::P3& w,
                           const Sphere& s) {
  i128 m[3][3];
  for (int d = 0; d < 3; ++d) {
    m[0][d] = coord_of(v, d) - coord_of(u, d);
    m[1][d] = coord_of(w, d) - coord_of(u, d);
    m[2][d] = s.cn[d] - s.den * coord_of(u, d);
  }
  i128 value = det3(m);
  if (s.den < 0) value = -value;
  return value < 0 ? -1 : (value == 0 ? 0 : 1);
}

inline int orient_point(const mhgp::P3& u, const mhgp::P3& v, const mhgp::P3& w,
                        const mhgp::P3& z) {
  i128 m[3][3];
  for (int d = 0; d < 3; ++d) {
    m[0][d] = coord_of(v, d) - coord_of(u, d);
    m[1][d] = coord_of(w, d) - coord_of(u, d);
    m[2][d] = coord_of(z, d) - coord_of(u, d);
  }
  const i128 value = det3(m);
  return value < 0 ? -1 : (value == 0 ? 0 : 1);
}

// SUPPORT PROPRE POSITIF : le centre circonscrit appartient a l'INTERIEUR du
// tetraedre, donc sa sphere est la miniboule des quatre sommets et Jung
// s'applique. Test : pour chaque face, le centre est strictement du meme cote
// que le sommet oppose. Un quadruplet cosphérique non positif est refuse — la
// borne de Jung ne doit jamais lui etre etendue (note §2).
inline bool positive_support(const mhgp::P3* q, const Sphere& s) {
  static const int faces[4][4] = {{1, 2, 3, 0}, {0, 2, 3, 1}, {0, 1, 3, 2},
                                  {0, 1, 2, 3}};
  for (const auto& f : faces) {
    const int side_apex = orient_point(q[f[0]], q[f[1]], q[f[2]], q[f[3]]);
    if (side_apex == 0) return false;   // degenere
    const int side_center = orient_rational(q[f[0]], q[f[1]], q[f[2]], s);
    if (side_center != side_apex) return false;   // hors ou sur la frontiere
  }
  return true;
}

inline i64 dist2_points(const mhgp::P3& u, const mhgp::P3& v) {
  const i64 dx = (i64)u.x - v.x, dy = (i64)u.y - v.y, dz = (i64)u.z - v.z;
  return dx * dx + dy * dy + dz * dz;
}

// L'ARETE CANONIQUE (note §1) : distance carree maximale d'abord, puis la plus
// petite paire ordonnee de PointId parmi les ex aequo.
inline std::pair<int, int> canonical_edge(const std::vector<mhgp::P3>& pts,
                                          const int* ids) {
  i64 best = -1;
  std::pair<int, int> anchor{-1, -1};
  for (int i = 0; i < kArity; ++i)
    for (int j = i + 1; j < kArity; ++j) {
      const i64 d2 = dist2_points(pts[(std::size_t)ids[i]], pts[(std::size_t)ids[j]]);
      int lo = ids[i], hi = ids[j];
      if (lo > hi) std::swap(lo, hi);
      if (d2 > best || (d2 == best && std::pair<int, int>{lo, hi} < anchor)) {
        best = d2;
        anchor = {lo, hi};
      }
    }
  return anchor;
}

}  // namespace judge

}  // namespace

int main(int argc, char** argv) {
  int n = 2400, coord = 0, leaf_size = 8, oracle = 0, permute = 0;
  i64 seed = 20260811, microtile = 64, max_states = 200000000;
  i64 min_block_prunes = 0, min_pruned_mass = 0, min_microtiles = 0, min_splits = 0;
  i64 min_patch_surviving = 0, min_accepted_nodes = 0, min_witness_point_tests = 0;
  mhgp3v::CloudFamily family = mhgp3v::CloudFamily::kTerrain;
  std::string fixture_name;
  Injections injections;
  auto integer = [](const char* text, i64* value) {
    const char* last = text + strlen(text);
    unsigned long long magnitude = 0;
    const auto r = std::from_chars(text, last, magnitude);
    if (text == last || r.ec != std::errc{} || r.ptr != last) return false;
    if (magnitude > 1000000000000ULL) return false;
    *value = (i64)magnitude;
    return true;
  };
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "--family")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --family\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "uniform")) family = mhgp3v::CloudFamily::kUniform;
      else if (!strcmp(argv[i], "terrain")) family = mhgp3v::CloudFamily::kTerrain;
      else if (!strcmp(argv[i], "scanline_single_pass"))
        family = mhgp3v::CloudFamily::kScanlineSinglePass;
      else if (!strcmp(argv[i], "scanline_overlap_multiecho"))
        family = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
      else { std::printf("ECHEC : famille inconnue %s\n", argv[i]); return 2; }
      continue;
    }
    if (!strcmp(argv[i], "--fixture")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --fixture\n"); return 2; }
      fixture_name = argv[++i];
      continue;
    }
    if (!strcmp(argv[i], "--inject")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --inject\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "clip-scale-4")) injections.clip_scale_4 = true;
      else if (!strcmp(argv[i], "ceil-root-truncated")) injections.ceil_root_truncated = true;
      else if (!strcmp(argv[i], "patch-omitted")) injections.patch_omitted = true;
      else if (!strcmp(argv[i], "feasible-declared-infeasible"))
        injections.feasible_declared_infeasible = true;
      else if (!strcmp(argv[i], "strict-to-large")) injections.strict_to_large = true;
      else if (!strcmp(argv[i], "corner-skipped")) injections.corner_skipped = true;
      else if (!strcmp(argv[i], "witness-duplicated")) injections.witness_duplicated = true;
      else if (!strcmp(argv[i], "witness-strict-to-large"))
        injections.witness_strict_to_large = true;
      else if (!strcmp(argv[i], "witness-subtrees-overlap"))
        injections.witness_subtrees_overlap = true;
      else if (!strcmp(argv[i], "endpoint-overlap-subtracted"))
        injections.endpoint_overlap_subtracted = true;
      else if (!strcmp(argv[i], "stale-inherited-credit"))
        injections.stale_inherited_credit = true;
      else if (!strcmp(argv[i], "q4-radius-replaced-by-q3"))
        injections.q4_radius_replaced_by_q3 = true;
      else if (!strcmp(argv[i], "microtile-truncated")) injections.microtile_truncated = true;
      else if (!strcmp(argv[i], "terminal-compensated"))
        injections.terminal_compensated = true;
      else if (!strcmp(argv[i], "cover-before-terminal"))
        injections.cover_before_terminal = true;
      else { std::printf("ECHEC : injection inconnue %s\n", argv[i]); return 2; }
      continue;
    }
    i64 value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    if (!has) { std::printf("ECHEC : argument %s sans valeur\n", argv[i]); return 2; }
    // BORNAGE AVANT CAST : un grand entier ne se replie jamais dans un int.
    if (value > 2147483647LL &&
        (!strcmp(argv[i], "--points") || !strcmp(argv[i], "--coord") ||
         !strcmp(argv[i], "--leaf-size") || !strcmp(argv[i], "--oracle") ||
         !strcmp(argv[i], "--permute"))) {
      std::printf("ECHEC : campagne absurde\n");
      return 2;
    }
    if (!strcmp(argv[i], "--points")) n = (int)value;
    else if (!strcmp(argv[i], "--coord")) coord = (int)value;
    else if (!strcmp(argv[i], "--leaf-size")) leaf_size = (int)value;
    else if (!strcmp(argv[i], "--seed")) seed = value;
    else if (!strcmp(argv[i], "--microtile")) microtile = value;
    else if (!strcmp(argv[i], "--max-states")) max_states = value;
    else if (!strcmp(argv[i], "--oracle")) oracle = (int)value;
    else if (!strcmp(argv[i], "--permute")) permute = (int)value;
    else if (!strcmp(argv[i], "--min-block-prunes")) min_block_prunes = value;
    else if (!strcmp(argv[i], "--min-pruned-mass")) min_pruned_mass = value;
    else if (!strcmp(argv[i], "--min-microtiles")) min_microtiles = value;
    else if (!strcmp(argv[i], "--min-splits")) min_splits = value;
    else if (!strcmp(argv[i], "--min-patch-surviving")) min_patch_surviving = value;
    else if (!strcmp(argv[i], "--min-accepted-nodes")) min_accepted_nodes = value;
    else if (!strcmp(argv[i], "--min-witness-point-tests")) min_witness_point_tests = value;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    ++i;
  }
  if (n < 4 || n > 100000 || coord < 0 || coord > 65536 || leaf_size < 2 ||
      leaf_size > 256 || microtile < 1 || max_states < 1 || oracle < 0 || oracle > 1 ||
      permute < 0 || permute > 1) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }

  const auto pt = [](int x, int y, int z) {
    mhgp::P3 p{};
    p.x = (mhgp::i32)x; p.y = (mhgp::i32)y; p.z = (mhgp::i32)z;
    return p;
  };
  const bool is_fixture = !fixture_name.empty();
  std::vector<mhgp::P3> pts;
  if (is_fixture) {
    if (fixture_name == "jung-root") {
      // LA RACINE ENTIERE (note §2) : q4 propre positif de centre (3/4,1,1/2)
      // et de diametre carre six. Une racine tronquee isqrt(6/8) = 0 omet le
      // centre ; la valeur correcte est R_t = 1. Les DEUX aretes de carre six
      // exercent aussi le tie-break lexicographique (l'ancre est (0,3)).
      pts = {pt(0,0,0), pt(0,0,1), pt(0,2,0), pt(2,1,1)};
    } else if (fixture_name == "jung-equality") {
      // L'EGALITE DE JUNG (note §2) : tetraedre regulier D^2 = 8, r^2 = 3,
      // ||t||^2 = 1 = X/8. Le test des milieux realise N_mid = 2X = 16
      // EXACTEMENT : un test large supprimerait un vrai centre.
      pts = {pt(11,11,11), pt(11,9,9), pt(9,11,9), pt(9,9,11)};
    } else if (fixture_name == "prunable-block") {
      // UN BLOC REELLEMENT PRUNABLE : deux groupes d'extremites distants de
      // mille, et un amas dense de temoins autour du milieu. Tout temoin a
      // moins de 178 unites du milieu est universel aux huit coins de TOUT
      // patch (marge derivee hors bande) : les patchs faisables se couvrent.
      for (int k = 0; k < 3; ++k) pts.push_back(pt(1000 + k, 1000, 1000 + 2 * k));
      for (int k = 0; k < 3; ++k) pts.push_back(pt(2000 + k, 1000, 1000 + 2 * k));
      for (int k = 0; k < 14; ++k)
        pts.push_back(pt(1490 + 3 * k, 1000 + (k % 5) - 2, 1000 + (k % 3) - 1));
    } else if (fixture_name == "microtile-first") {
      // LA POLITIQUE MICROTUILE AVANT COVER : huit points epars, seuil
      // microtuile par defaut — AUCUN bloc croise n'atteint le cover, donc
      // aucun des 64 patchs n'est evalue. Le mutant cover-before-terminal
      // paie ce travail et se trahit par blocks_tried > 0.
      pts = {pt(100,100,100), pt(60000,200,300), pt(300,60000,400), pt(400,500,60000),
             pt(50000,50000,100), pt(200,40000,40000), pt(30000,100,30000),
             pt(100,30000,30000)};
    } else if (fixture_name == "u16-extremes") {
      // LES EXTREMES u16 : les huit coins de la grille et un amas central —
      // X atteint 3*65535^2, R_t et R_w sont maximaux, aucun intermediaire ne
      // deborde (bornes documentees en tete).
      for (int k = 0; k < 8; ++k)
        pts.push_back(pt(k & 1 ? 65535 : 0, k & 2 ? 65535 : 0, k & 4 ? 65535 : 0));
      for (int k = 0; k < 12; ++k)
        pts.push_back(pt(32760 + k, 32768 - (k % 4), 32770 + (k % 3)));
    } else {
      std::printf("ECHEC : fixture inconnue %s\n", fixture_name.c_str());
      return 2;
    }
    n = (int)pts.size();
    leaf_size = 2;
    microtile = 4;              // mu >= leaf_size^2 = 4
    // La fixture de POLITIQUE releve son seuil pour que TOUTE tache soit
    // terminale : c'est ce qui rend `blocks_tried == 0` observable.
    if (fixture_name == "microtile-first") microtile = 16;
    oracle = 1;
    std::printf("provenance : --fixture %s (%d points graves, feuilles <= %d,"
                " microtuile %lld, oracle force)\n", fixture_name.c_str(), n, leaf_size,
                microtile);
  } else {
    if (coord == 0) coord = mhgp3v::cloud_family_default_coord(family, n);
    pts = mhgp3v::make_family_cloud(family, n, coord, seed);
    if ((int)pts.size() < n) { std::printf("ECHEC : nuage non genere\n"); return 3; }
    if ((int)pts.size() != n) {
      std::printf("ECHEC : contrat de cardinalite viole — %zu points rendus pour %d"
                  " demandes\n", pts.size(), n);
      return 1;
    }
    unsigned long long digest = 1469598103934665603ULL;
    for (const mhgp::P3& q : pts)
      for (int c = 0; c < 3; ++c) {
        const unsigned long long v =
            (unsigned long long)(c == 0 ? q.x : (c == 1 ? q.y : q.z));
        for (int byte = 0; byte < 4; ++byte) {
          digest ^= (v >> (8 * byte)) & 0xFFULL;
          digest *= 1099511628211ULL;
        }
      }
    std::printf("provenance : --points %d --coord %d --seed %lld --family %s"
                " --leaf-size %d --microtile %lld digest=%016llx\n", n, coord, seed,
                mhgp3v::cloud_family_name(family), leaf_size, microtile, digest);
  }
  // LE PREFLIGHT DU SEUIL (note §6) : mu >= leaf_size^2, afin que toute tache
  // insecable soit terminale.
  if (microtile < (i64)leaf_size * leaf_size) {
    std::printf("ECHEC : preflight viole — microtuile %lld < leaf_size^2 %lld\n",
                microtile, (i64)leaf_size * leaf_size);
    return 2;
  }
  if (oracle == 1 && n > 32) {
    std::printf("ECHEC : le juge exhaustif exige n <= 32\n");
    return 2;
  }
  const auto fail = [&](const char* what, const char* detail) {
    if (injections.any()) {
      std::printf("mutant tue par %s : %s\n", what, detail);
      return 4;
    }
    std::printf("ECHEC %s : %s\n", what, detail);
    return 1;
  };

  Lbvh tree;
  tree.build(pts, leaf_size);
  const i128 all_pairs = (i128)n * (n - 1) / 2;

  std::vector<BlockReceipt> block_receipts;
  std::vector<MicrotileReceipt> microtile_receipts;
  CoverMachine machine;
  machine.tree = &tree;
  machine.injections = injections;
  machine.microtile = microtile;
  machine.max_states = max_states;
  if (oracle == 1) {
    machine.oracle_mode = true;
    machine.block_receipts = &block_receipts;
    machine.microtile_receipts = &microtile_receipts;
  }
  const auto t0 = std::chrono::steady_clock::now();
  const bool ok = machine.run();
  const auto t1 = std::chrono::steady_clock::now();
  const double seconds = std::chrono::duration<double>(t1 - t0).count();
  if (!ok) {
    if (machine.budget_exceeded) {
      std::printf("ECHEC : budget d'etats depasse (%lld) — le probe refuse, il ne"
                  " tronque pas\n", max_states);
      return 3;
    }
    return 1;
  }
  const MassReceipt& r = machine.receipt;
  // MUTANT terminal-compensated : une paire omise et une autre dupliquee, a
  // masse totale INCHANGEE. Seule la bijection du juge le voit.
  if (injections.terminal_compensated && !microtile_receipts.empty()) {
    MicrotileReceipt& first = microtile_receipts.front();
    if (first.is_self && first.a_end - first.a_begin >= 2) {
      ++first.a_begin;
      ++first.a_end;
    } else if (!first.is_self) {
      ++first.b_begin;
      ++first.b_end;
    }
  }
  if (machine.pending() != 0)
    return fail("le ledger transactionnel", "P_pending n'est pas nul a la fermeture");
  if (r.pruned_mass + r.microtile_mass != all_pairs)
    return fail("l'identite du ledger", "P_prune + P_microtile != C(n,2)");

  // -------------------------------------------------------------------------
  // LE JUGE INDEPENDANT (note §7) : rejeu des reçus, bijection des sorts,
  // oracle determinantal exhaustif. Il reconstruit tout depuis les POINTS
  // REELS, sans reutiliser une seule structure du sujet.
  // -------------------------------------------------------------------------
  if (oracle == 1) {
    const auto box_of_range = [&](int begin, int end) {
      Box b;
      for (int d = 0; d < 3; ++d) { b.lo[d] = (i64)1 << 40; b.hi[d] = -((i64)1 << 40); }
      for (int t = begin; t < end; ++t) {
        const mhgp::P3& p = pts[(std::size_t)tree.order[(std::size_t)t]];
        const i64 c[3] = {(i64)p.x, (i64)p.y, (i64)p.z};
        for (int d = 0; d < 3; ++d) {
          b.lo[d] = std::min(b.lo[d], c[d]);
          b.hi[d] = std::max(b.hi[d], c[d]);
        }
      }
      return b;
    };
    // 1. LE REJEU DES REÇUS DE BLOCS.
    for (const BlockReceipt& block : block_receipts) {
      const Box a_box = box_of_range(block.a_begin, block.a_end);
      const Box b_box = box_of_range(block.b_begin, block.b_end);
      // Reconstruction INDEPENDANTE de X, R_t, R_w et des bornes de patchs.
      i128 jx = 0;
      for (int d = 0; d < 3; ++d) {
        const i128 left = (i128)a_box.lo[d] - b_box.hi[d];
        const i128 right = (i128)a_box.hi[d] - b_box.lo[d];
        const i128 la = left < 0 ? -left : left;
        const i128 rb = right < 0 ? -right : right;
        const i128 far = la > rb ? la : rb;
        jx += far * far;
      }
      if (jx != block.x)
        return fail("le rejeu des reçus", "le juge recalcule un autre X pour un bloc");
      i128 jrt = 0;
      while (8 * jrt * jrt < jx) ++jrt;
      if (jrt != block.radius_t)
        return fail("le rejeu des reçus",
                    "le juge recalcule un autre R_t — racine tronquee ou rayon q3");
      i128 jbound[3][5];
      for (int d = 0; d < 3; ++d) {
        const i128 sum_lo = (i128)a_box.lo[d] + b_box.lo[d];
        const i128 sum_hi = (i128)a_box.hi[d] + b_box.hi[d];
        const i128 lo = (sum_lo >= 0 ? sum_lo / 2 : -((-sum_lo + 1) / 2)) - jrt;
        const i128 hi = (sum_hi >= 0 ? (sum_hi + 1) / 2 : -((-sum_hi) / 2)) + jrt;
        for (int j = 0; j <= 4; ++j) jbound[d][j] = 4 * lo + (i128)j * (hi - lo);
      }
      for (int p = 0; p < kPatches; ++p) {
        const PatchReceipt& patch = block.patches[(std::size_t)p];
        if (patch.status == PatchStatus::kSurviving ||
            patch.status == PatchStatus::kUnknown)
          return fail("le rejeu des reçus",
                      "un bloc prune porte un patch survivant ou sans statut");
        const int j[3] = {p & 3, (p >> 2) & 3, (p >> 4) & 3};
        i128 plo[3], phi[3];
        for (int d = 0; d < 3; ++d) {
          plo[d] = jbound[d][j[d]];
          phi[d] = jbound[d][j[d] + 1];
          if (phi[d] < plo[d]) std::swap(plo[d], phi[d]);
        }
        if (status_is_infeasible(patch.status)) {
          // Rejeu de la preuve STRICTE, en i128, depuis les points reels.
          const auto dist2 = [&](const Box& s) {
            i128 total = 0;
            for (int d = 0; d < 3; ++d) {
              const i128 slo = 4 * (i128)s.lo[d], shi = 4 * (i128)s.hi[d];
              if (plo[d] > shi) { const i128 g = plo[d] - shi; total += g * g; }
              else if (slo > phi[d]) { const i128 g = slo - phi[d]; total += g * g; }
            }
            return total;
          };
          const auto maxdist2 = [&](const Box& s) {
            i128 total = 0;
            for (int d = 0; d < 3; ++d) {
              const i128 slo = 4 * (i128)s.lo[d], shi = 4 * (i128)s.hi[d];
              const i128 left = plo[d] - shi, right = phi[d] - slo;
              const i128 la = left < 0 ? -left : left;
              const i128 rb = right < 0 ? -right : right;
              const i128 far = la > rb ? la : rb;
              total += far * far;
            }
            return total;
          };
          i128 mid = 0;
          for (int d = 0; d < 3; ++d) {
            const i128 mlo = 2 * ((i128)a_box.lo[d] + b_box.lo[d]);
            const i128 mhi = 2 * ((i128)a_box.hi[d] + b_box.hi[d]);
            if (plo[d] > mhi) { const i128 g = plo[d] - mhi; mid += g * g; }
            else if (mlo > phi[d]) { const i128 g = mlo - phi[d]; mid += g * g; }
          }
          const bool proved = dist2(a_box) > maxdist2(b_box) ||
                              dist2(b_box) > maxdist2(a_box) ||
                              dist2(a_box) > 6 * jx || dist2(b_box) > 6 * jx ||
                              mid > 2 * jx;
          if (!proved)
            return fail("le rejeu des reçus",
                        "un patch declare infaisable n'a aucune preuve stricte");
          continue;
        }
        // PATCH COUVERT : huit PointId DISTINCTS, hors des plages A et B,
        // rejoues contre CHAQUE POINT REEL du cote engage aux huit coins.
        std::vector<int> ids;
        for (const PatchCredit& credit : patch.credits) ids.push_back(credit.point_id);
        std::vector<int> unique_ids = ids;
        std::sort(unique_ids.begin(), unique_ids.end());
        unique_ids.erase(std::unique(unique_ids.begin(), unique_ids.end()),
                         unique_ids.end());
        if ((int)unique_ids.size() < kThreshold)
          return fail("le rejeu des reçus",
                      "un patch couvert n'a pas huit PointId DISTINCTS");
        for (const PatchCredit& credit : patch.credits) {
          for (int t = block.a_begin; t < block.a_end; ++t)
            if (tree.order[(std::size_t)t] == credit.point_id)
              return fail("le rejeu des reçus", "un temoin appartient a la plage A");
          for (int t = block.b_begin; t < block.b_end; ++t)
            if (tree.order[(std::size_t)t] == credit.point_id)
              return fail("le rejeu des reçus", "un temoin appartient a la plage B");
          const mhgp::P3& z = pts[(std::size_t)credit.point_id];
          const int side_begin = credit.side == 0 ? block.a_begin : block.b_begin;
          const int side_end = credit.side == 0 ? block.a_end : block.b_end;
          for (int k = 0; k < 8; ++k) {
            i128 corner[3];
            for (int d = 0; d < 3; ++d) corner[d] = (k >> d) & 1 ? phi[d] : plo[d];
            i128 to_z = 0;
            for (int d = 0; d < 3; ++d) {
              const i128 delta = corner[d] - 4 * judge::coord_of(z, d);
              to_z += delta * delta;
            }
            // CHAQUE point reel du cote engage — pas les coins de sa boite :
            // la fonction est convexe en s et son minimum peut etre interieur.
            for (int t = side_begin; t < side_end; ++t) {
              const mhgp::P3& s = pts[(std::size_t)tree.order[(std::size_t)t]];
              i128 to_s = 0;
              for (int d = 0; d < 3; ++d) {
                const i128 delta = corner[d] - 4 * judge::coord_of(s, d);
                to_s += delta * delta;
              }
              if (!(to_z < to_s))
                return fail("le rejeu des reçus",
                            "un temoin credite n'est pas strictement interieur pour un"
                            " coin et un point reel du cote engage");
            }
          }
        }
      }
    }
    // 2. LA BIJECTION DES SORTS : chaque paire non ordonnee reçoit EXACTEMENT
    // un sort. Une omission compensee par un doublon meurt ici.
    std::vector<std::uint8_t> fate((std::size_t)all_pairs, 0);
    const auto pair_index = [&](int i, int j) {
      if (i > j) std::swap(i, j);
      return (i64)i * (2 * (i64)n - i - 1) / 2 + (j - i - 1);
    };
    bool doubled = false, out_of_range = false;
    const auto assign = [&](int id_a, int id_b, std::uint8_t value) {
      const i64 index = pair_index(id_a, id_b);
      if (index < 0 || index >= (i64)all_pairs) { out_of_range = true; return; }
      if (fate[(std::size_t)index] != 0) { doubled = true; return; }
      fate[(std::size_t)index] = value;
    };
    for (const BlockReceipt& block : block_receipts)
      for (int ta = block.a_begin; ta < block.a_end; ++ta)
        for (int tb = block.b_begin; tb < block.b_end; ++tb)
          assign(tree.order[(std::size_t)ta], tree.order[(std::size_t)tb], 1);
    for (const MicrotileReceipt& tile : microtile_receipts) {
      if (tile.is_self) {
        for (int ta = tile.a_begin; ta < tile.a_end; ++ta)
          for (int tb = ta + 1; tb < tile.a_end; ++tb)
            assign(tree.order[(std::size_t)ta], tree.order[(std::size_t)tb], 2);
      } else {
        for (int ta = tile.a_begin; ta < tile.a_end; ++ta)
          for (int tb = tile.b_begin; tb < tile.b_end; ++tb)
            assign(tree.order[(std::size_t)ta], tree.order[(std::size_t)tb], 2);
      }
    }
    if (out_of_range)
      return fail("la bijection des sorts", "un reçu designe une paire hors domaine");
    if (doubled)
      return fail("la bijection des sorts", "une paire a recu deux sorts");
    for (std::size_t k = 0; k < fate.size(); ++k)
      if (fate[k] == 0)
        return fail("la bijection des sorts",
                    "une paire n'a aucun sort — une omission compensee par un doublon"
                    " est invisible au ledger scalaire");
    // 3. L'ORACLE DETERMINANTAL EXHAUSTIF : tout support q4 propre positif de
    // moins de huit interieurs stricts doit avoir son arete canonique HORS de
    // la masse prunee.
    i64 positive_supports = 0, non_inert = 0;
    int ids[4];
    for (ids[0] = 0; ids[0] < n; ++ids[0])
      for (ids[1] = ids[0] + 1; ids[1] < n; ++ids[1])
        for (ids[2] = ids[1] + 1; ids[2] < n; ++ids[2])
          for (ids[3] = ids[2] + 1; ids[3] < n; ++ids[3]) {
            const mhgp::P3 quad[4] = {pts[(std::size_t)ids[0]], pts[(std::size_t)ids[1]],
                                      pts[(std::size_t)ids[2]], pts[(std::size_t)ids[3]]};
            judge::Sphere sphere;
            if (!judge::circumsphere(quad, &sphere)) continue;
            if (!judge::positive_support(quad, sphere)) continue;
            ++positive_supports;
            i64 strict_inside = 0;
            for (int x = 0; x < n; ++x)
              if (judge::power_sign(sphere, pts[(std::size_t)x], quad[0]) < 0)
                ++strict_inside;
            if (strict_inside >= kThreshold) continue;   // inerte : omission licite
            ++non_inert;
            const auto anchor = judge::canonical_edge(pts, ids);
            if (fate[(std::size_t)pair_index(anchor.first, anchor.second)] == 1)
              return fail("l'oracle determinantal",
                          "l'arete canonique d'un support q4 propre positif non inerte"
                          " appartient a la masse prunee");
          }
    std::printf("oracle     : %lld supports q4 propres positifs dont %lld non inertes ;"
                " reçus rejoues, bijection des sorts fermee — aucun desaccord\n",
                positive_supports, non_inert);
  }

  // L'EQUIVARIANCE PAR PERMUTATION (note §8) : une permutation ne doit
  // preserver que l'EXACTITUDE et les identites de masse — un filtre
  // suffisant et incomplet n'est pas tenu de rendre la meme partition
  // prune/microtuile.
  if (permute == 1) {
    std::vector<mhgp::P3> reversed(pts.rbegin(), pts.rend());
    Lbvh tree2;
    tree2.build(reversed, leaf_size);
    CoverMachine machine2;
    machine2.tree = &tree2;
    machine2.injections = injections;
    machine2.microtile = microtile;
    machine2.max_states = max_states;
    if (!machine2.run()) {
      std::printf("ECHEC : budget depasse dans la permutation\n");
      return 3;
    }
    if (machine2.pending() != 0 ||
        machine2.receipt.pruned_mass + machine2.receipt.microtile_mass != all_pairs)
      return fail("l'equivariance par permutation",
                  "le ledger ne ferme plus apres renommage des PointId");
    std::printf("permutation: ledger FERME apres renversement — prunes=%lld"
                " masse=%lld (contre %lld/%lld)\n", machine2.receipt.block_prunes,
                (i64)machine2.receipt.pruned_mass, r.block_prunes, (i64)r.pruned_mass);
  }

  // LES PLANCHERS ANTI VERT-PAR-VACUITE (note §8).
  const auto floor_violated = [&](const char* what, i64 got, i64 want) {
    std::printf("ECHEC : plancher viole — %s=%lld < %lld\n", what, got, want);
    return 3;
  };
  if (min_block_prunes > 0 && r.block_prunes < min_block_prunes)
    return floor_violated("blocs-prunes", r.block_prunes, min_block_prunes);
  if (min_pruned_mass > 0 && (i64)r.pruned_mass < min_pruned_mass)
    return floor_violated("masse-prunee", (i64)r.pruned_mass, min_pruned_mass);
  if (min_microtiles > 0 && r.microtiles < min_microtiles)
    return floor_violated("microtuiles", r.microtiles, min_microtiles);
  if (min_splits > 0 && r.splits < min_splits)
    return floor_violated("splits", r.splits, min_splits);
  if (min_patch_surviving > 0 && r.patch_surviving < min_patch_surviving)
    return floor_violated("patchs-survivants", r.patch_surviving, min_patch_surviving);
  if (min_accepted_nodes > 0 && r.accepted_nodes < min_accepted_nodes)
    return floor_violated("noeuds-acceptes", r.accepted_nodes, min_accepted_nodes);
  if (min_witness_point_tests > 0 && r.witness_point_tests < min_witness_point_tests)
    return floor_violated("tests-ponctuels", r.witness_point_tests,
                          min_witness_point_tests);

  // LES ASSERTIONS DE FIXTURE.
  if (is_fixture) {
    if (fixture_name == "jung-root") {
      // ASSERTIONS DIRECTES de la racine entiere : la valeur correcte et la
      // troncature fautive sont toutes deux exhibees sur les octets graves.
      const Box a_box{{0, 0, 0}, {0, 0, 0}};
      const Box b_box{{2, 1, 1}, {2, 1, 1}};
      const i64 x = box_maxdist2(a_box, b_box);
      if (x != 6)
        return fail("la fixture jung-root", "le diametre carre grave n'est plus six");
      if (radius_t(x, false) != 1 || radius_t(x, true) != 0)
        return fail("la fixture jung-root",
                    "R_t correct != 1 ou la troncature fautive != 0 — la fixture ne"
                    " separe plus les deux racines");
      // Le centre exact (3/4, 1, 1/2) appartient a T0 avec R_t = 1 et PAS avec
      // la racine tronquee : l'omission est donc observable.
      const mhgp::P3 quad[4] = {pts[0], pts[1], pts[2], pts[3]};
      judge::Sphere sphere;
      if (!judge::circumsphere(quad, &sphere) || !judge::positive_support(quad, sphere))
        return fail("la fixture jung-root",
                    "le quadruplet grave n'est plus un support propre positif");
      for (int truncated = 0; truncated < 2; ++truncated) {
        const BlockGeometry geom = block_geometry(a_box, b_box, truncated == 1, false);
        bool inside = true;
        for (int d = 0; d < 3; ++d) {
          // centre_d appartient a [lo, hi] : compare en numerateurs sur den.
          const i128 num = sphere.cn[d], den = sphere.den;
          const i128 lo4 = geom.t0_lo4[d], hi4 = geom.t0_hi4[d];
          const i128 scaled = den > 0 ? 4 * num : -4 * num;
          const i128 abs_den = den > 0 ? den : -den;
          if (scaled < lo4 * abs_den || scaled > hi4 * abs_den) inside = false;
        }
        if (inside == (truncated == 1))
          return fail("la fixture jung-root",
                      "l'appartenance du centre a T0 ne separe plus la racine correcte"
                      " de la racine tronquee");
      }
      const auto anchor = judge::canonical_edge(pts, (const int[]){0, 1, 2, 3});
      if (anchor.first != 0 || anchor.second != 3)
        return fail("la fixture jung-root",
                    "l'ancre canonique n'est plus (0,3) — le tie-break lexicographique"
                    " des deux aretes de carre six a derive");
    } else if (fixture_name == "jung-equality") {
      // L'EGALITE EXACTE : pour l'arete (0,1), le vrai centre realise
      // N_mid = 2X. Le test STRICT le conserve, un test large le supprime.
      const Box a_box{{11, 11, 11}, {11, 11, 11}};
      const Box b_box{{11, 9, 9}, {11, 9, 9}};
      const i64 x = box_maxdist2(a_box, b_box);
      if (x != 8) return fail("la fixture jung-equality", "D^2 n'est plus huit");
      const mhgp::P3 quad[4] = {pts[0], pts[1], pts[2], pts[3]};
      judge::Sphere sphere;
      if (!judge::circumsphere(quad, &sphere) || !judge::positive_support(quad, sphere))
        return fail("la fixture jung-equality",
                    "le tetraedre regulier n'est plus un support propre positif");
      // Le centre (10,10,10) est a distance carree 1 du milieu (11,10,10) :
      // 16*1 = 16 = 2X exactement.
      const i128 mid_dist16 = 16;
      if (mid_dist16 != 2 * (i128)x)
        return fail("la fixture jung-equality",
                    "N_mid n'egale plus 2X — la fixture ne mord plus sur le test"
                    " strict des milieux");
      // L'ASSERTION DECISIVE, directe sur le prédicat (le bloc {p0} x {p1} a
      // une masse d'une paire et n'atteint jamais le cover) : le patch
      // DEGENERE reduit au vrai centre (10,10,10) realise l'egalite EXACTE
      // des TROIS preuves a la fois —
      //     N_min(T,A) = N_max(T,B) = 6X = 48   et   N_mid(T) = 2X = 16 —
      // parce que ce centre est a distance carree X/8 = 1 du milieu et a
      // distance carree r^2 = 3 = 3X/8 des deux extremites. Le prédicat
      // STRICT le conserve ; le mutant strict-to-large le declare infaisable
      // et tue un vrai centre. La note appelle exactement ce cas.
      {
        const BlockGeometry geom = block_geometry(a_box, b_box, false, false);
        const Box a4 = scale4(a_box), b4 = scale4(b_box);
        Box center_patch;
        for (int d = 0; d < 3; ++d) center_patch.lo[d] = center_patch.hi[d] = 40;
        if (box_dist2(center_patch, geom.mid4) != 2 * x ||
            box_dist2(center_patch, a4) != 6 * x ||
            box_maxdist2(center_patch, b4) != 6 * x)
          return fail("la fixture jung-equality",
                      "les trois egalites exactes du centre ont derive — la fixture"
                      " ne mord plus sur les preuves strictes");
        if (status_is_infeasible(patch_infeasibility(geom, center_patch, a4, b4,
                                                     injections.strict_to_large)))
          return fail("la fixture jung-equality",
                      "le patch reduit au vrai centre a ete declare infaisable —"
                      " l'egalite doit CONSERVER le patch");
      }
    } else if (fixture_name == "prunable-block") {
      if (r.block_prunes < 1)
        return fail("la fixture prunable-block",
                    "aucun bloc prune — le center-cover ne mord plus");
      if (r.pruned_mass <= 0)
        return fail("la fixture prunable-block", "la masse prunee est nulle");
    } else if (fixture_name == "microtile-first") {
      // LA POLITIQUE : aucune tache n'atteint le cover, donc AUCUN des 64
      // patchs n'est evalue. Le mutant cover-before-terminal se trahit ici.
      if (r.blocks_tried != 0 || r.patch_slots != 0)
        return fail("la fixture microtile-first",
                    "les 64 patchs ont ete evalues avant le seuil terminal");
      if (r.microtile_mass != all_pairs)
        return fail("la fixture microtile-first",
                    "toute la masse devrait etre terminale");
    } else if (fixture_name == "u16-extremes") {
      if (r.pruned_mass + r.microtile_mass != all_pairs)
        return fail("la fixture u16-extremes", "le ledger ne ferme pas aux extremes");
    }
  }

  if (injections.any()) {
    std::printf("MUTANT SURVIVANT : aucune porte n'a mordu\n");
    return 0;
  }

  const double terminal_share =
      all_pairs > 0 ? 100.0 * (double)(i64)r.microtile_mass / (double)(i64)all_pairs : 0.0;
  std::printf("arbre      : %zu noeuds, feuilles <= %d, microtuile %lld\n",
              tree.nodes.size(), leaf_size, microtile);
  std::printf("partition  : blocs tentes=%lld prunes=%lld splits=%lld (self=%lld)"
              " microtuiles=%lld masse-max-bloc=%lld\n", r.blocks_tried, r.block_prunes,
              r.splits, r.self_splits, r.microtiles, r.max_block_mass);
  std::printf("patchs     : slots=%lld infaisables mediateur=%lld Jung=%lld"
              " milieux=%lld — couverts=%lld survivants=%lld\n", r.patch_slots,
              r.patch_infeasible[0], r.patch_infeasible[1], r.patch_infeasible[2],
              r.patch_covered, r.patch_surviving);
  std::printf("temoins    : visites=%lld noeuds-acceptes=%lld feuilles-ambigues=%lld"
              " tests-ponctuels=%lld coins=%lld prefiltre=%lld pile=%lld\n",
              r.witness_node_visits, r.accepted_nodes, r.ambiguous_leaves,
              r.witness_point_tests, r.corner_evals, r.prefilter_skips,
              r.stack_high_water);
  std::printf("ledger     : prunee=%lld + terminale=%lld = %lld = C(n,2) — FERME"
              " (part terminale %.2f %%, %.3f s de phase locale, 1 thread)\n",
              (i64)r.pruned_mass, (i64)r.microtile_mass, (i64)all_pairs, terminal_share,
              seconds);
  if (is_fixture)
    std::printf("OK : fixture %s recue\n", fixture_name.c_str());
  else
    std::printf("OK : falsificateur de masse P1a q4 mass-only — le ledger est la"
                " sortie, aucune ancre n'est emise et aucune admission prononcee\n");
  return 0;
}
