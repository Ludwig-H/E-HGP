// MorseHGP3D v3 — LE FALSIFICATEUR D'ANCRES q3/q4 PAR COEUR UNIVERSEL DE
// JUNG (NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811, porte
// d'implementation en cinq points) ET PROFONDEUR FERMEE TERMINALE — trois
// modes isoles : `core` (coeur seul), `depth` (profondeur seule, aucun prune
// de bloc), `combined` (coeur puis profondeur sur les survivantes).
//
// LE THEOREME (note auditeur) : pour une paire (a,b) qui est une arete de
// longueur MAXIMALE d'un support propre positif q3/q4, le centre de la
// sphere appartient au disque mediateur de Jung `||t||^2 <= D^2/12` (q3) ou
// `D^2/8` (q4). Un temoin w avec U = 2w-a-b, d = b-a, g = D^2 - ||U||^2,
// Q = ||d x U||^2 est strictement interieur a la sphere de TOUT centre du
// disque de Jung entier — donc SUFFISANT pour le sous-ensemble des centres
// realisables, jamais necessaire (audit etat courant) — quand :
//
//     q3 : g > 0  et  3 g^2 > 4 Q        q4 : g > 0  et  g^2 > 2 Q
//
// (prédicats entiers, i128, produits < ~2^78). Neuf temoins universels (q3)
// ou huit (q4), PointId distincts hors extremites, donnent p >= K+2-q donc
// p+q >= 12 : l'ancre est morte pour le seul quotient H0 normalise a K=10.
// COUVERTURE : tout support non inerte a son arete diametre canonique parmi
// les paires RESIDUELLES — un bloc supprime la contiendrait avec ses 9/8
// temoins et le support serait inerte (note auditeur, consequence de
// couverture). Le residuel est donc un sur-ensemble complet des ancres.
//
// LA MACHINE : la partition canonique du self-produit (L,L / LxR / R,R,
// division du plus gros cote) ; par bloc croise, le PRE-PRUNE de boule
// inscrite par intervalles (q3 : 3 max||U||^2 < min D^2 STRICT ; q4 :
// 15 max||U||^2 <= 4 min D^2, l'inegalite LARGE est licite car
// D/sqrt(15) < D sin(15 deg) — note auditeur) ; par noeud temoin contre une
// paire EXACTE, le test des HUIT COINS du prédicat polynomial (le spindle
// est une intersection de boules ouvertes, donc convexe : huit coins
// stricts => boite entiere stricte) ; aux terminaux, le prédicat exact par
// point. Chaque paire finit PRUNEE (ancre morte) ou RESIDUELLE (candidate
// ancre) : `pruned + residual = C(n,2)` par lane, sorts independants par
// lane, aucun residuel n'alimente une autre lane.
//
// MUTANT MORT-NE DOCUMENTE : « plage recouvrant une extremite » — une
// extremite w = a donne U = -d donc g = 0 : le prédicat strict la refuse
// deja, et un noeud contenant une extremite ne passe jamais le test des
// coins (le coin de l'extremite echoue). L'exclusion positionnelle est
// conservee par defense en profondeur ; le mutant ne peut pas mordre.
//
// L'ORACLE EXHAUSTIF (--oracle 1, n <= 32) : tous les triples/quadruples,
// miniboule exacte, support propre positif ssi n_support == q, census p par
// sphere_side, non-inerte ssi p < K+2-q ; l'ancre canonique (plus petite
// paire parmi les aretes de longueur maximale, apres traitement des ex
// aequo) doit etre RESIDUELLE. Chaque bloc prune conserve ses plages et ses
// temoins ; l'oracle REJOUE chaque certificat sur les coordonnees brutes.
//
// C'EST UN FALSIFICATEUR COUNT-ONLY : aucune ancre formee en aval, aucun
// census, aucune BallActivation, aucun reporter mediateur. Codes : 0 OK ;
// 1 violation ; 2 CLI ; 3 budget ; 4 mutant tue.
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "mhgp/miniball.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/exact_ray_sweep.hpp"

namespace {

using i64 = long long;
using i128 = __int128;

struct Node {
  i64 lo[3] = {0, 0, 0};
  i64 hi[3] = {0, 0, 0};
  int begin = 0, end = 0;
  int left = -1, right = -1;
};

struct Tree {
  std::vector<Node> nodes;
  std::vector<int> order;
  const std::vector<mhgp::P3>* points = nullptr;

  void build(const std::vector<mhgp::P3>& cloud, int leaf_size) {
    points = &cloud;
    order.resize(cloud.size());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = (int)i;
    nodes.clear();
    nodes.reserve(2 * (cloud.size() / (std::size_t)leaf_size + 2));
    build_range(0, (int)order.size(), leaf_size);
  }

  int build_range(int begin, int end, int leaf_size) {
    const int self = (int)nodes.size();
    nodes.push_back(Node{});
    Node node{};
    node.begin = begin;
    node.end = end;
    for (int d = 0; d < 3; ++d) {
      node.lo[d] = (i64)1 << 40;
      node.hi[d] = -((i64)1 << 40);
    }
    for (int t = begin; t < end; ++t) {
      const mhgp::P3& p = (*points)[(std::size_t)order[(std::size_t)t]];
      const i64 c[3] = {(i64)p.x, (i64)p.y, (i64)p.z};
      for (int d = 0; d < 3; ++d) {
        node.lo[d] = std::min(node.lo[d], c[d]);
        node.hi[d] = std::max(node.hi[d], c[d]);
      }
    }
    if (end - begin > leaf_size) {
      int axis = 0;
      for (int d = 1; d < 3; ++d)
        if (node.hi[d] - node.lo[d] > node.hi[axis] - node.lo[axis]) axis = d;
      const int mid = begin + (end - begin) / 2;
      std::nth_element(order.begin() + begin, order.begin() + mid, order.begin() + end,
                       [&](int a, int b) {
                         const mhgp::P3& u = (*points)[(std::size_t)a];
                         const mhgp::P3& w = (*points)[(std::size_t)b];
                         const i64 ua = axis == 0 ? (i64)u.x : (axis == 1 ? (i64)u.y : (i64)u.z);
                         const i64 wa = axis == 0 ? (i64)w.x : (axis == 1 ? (i64)w.y : (i64)w.z);
                         if (ua != wa) return ua < wa;
                         return a < b;
                       });
      node.left = build_range(begin, mid, leaf_size);
      node.right = build_range(mid, end, leaf_size);
    }
    nodes[(std::size_t)self] = node;
    return self;
  }
};

struct ProbeInjections {
  bool gt_to_ge = false;             // strict change en large : un contact compte
  bool threshold_minus_one = false;  // seuil 8/7 au lieu de 9/8
  bool witness_duplicated = false;   // chaque credit compte double
  bool last_block_omitted = false;   // la derniere paire residuelle est perdue
  bool duplicate_compensated = false;// fils duplique + fils omis, masses egales
  bool anchor_non_maximal = false;   // ORACLE : ancre = plus petite arete
  // MUTANT MORT-NE DOCUMENTE (le second, apres « plage recouvrant une
  // extremite ») : elargir le juge aux tuples NON PROPRES ne peut pas mordre.
  // Preuve : si l'ancre canonique (X,Y) — arete MAXIMALE du tuple — est
  // prunee, la miniboule du tuple contient X et Y dans sa boule fermee et son
  // rayon respecte Jung (r <= D/sqrt(3) pour un triple plan, r <= D
  // sqrt(3/8) pour un quadruple, D = |XY|), donc la projection equidistante
  // de son centre reste dans le disque de Jung de la lane. Pour un temoin
  // diametral strict w, f(o) = |oX|^2 - |ow|^2 est AFFINE en o de derivee
  // axiale 2(w-X).d^ > 0 : tout decalage h >= 0 du centre vers Y conserve
  // f > 0 (et h <= 0 par symetrie avec Y), donc les 9/8 temoins du coeur —
  // comme les delta temoins de la profondeur — restent strictement
  // interieurs a la miniboule : p >= seuil, le tuple est inerte et le juge
  // elargi ne peut jamais exiger sa residualite. Conserve en defense en
  // profondeur, sans porte tueuse possible.
  bool oracle_accept_nonpositive = false;  // ORACLE : support non propre accepte
  // MUTANTS DE LA PROFONDEUR FERMEE (tranche 2, REPONSE_AUDIT_ANCRES) :
  bool depth_open_boundary = false;  // l'antipode compte dans l'arc semi-ouvert
  bool depth_always_dropped = false; // la banque des projections nulles perdue
  // MUTANTS DES PORTES 1 ET 3 DE L'AUDIT ETAT COURANT :
  bool q4_degenerate_accept = false; // la garde min D^2 > 0 du pre-prune q4 otee
  bool q4_large_to_strict = false;   // l'egalite sure 15U^2=4D^2 refusee a tort
  bool depth_prune_disarmed = false; // la decision delta >= seuil debranchee
  bool depth_confounded_dropped = false; // les rayons confondus otes de l'arc
  bool diametral_open_to_closed = false; // le collecteur credite les contacts
  // MUTANTS DE LA RECEPTION L4/HERITAGE DU COEUR (miroir de la gate q2
  // d705bcde) — chacun meurt par le DIFFERENTIEL bi-mode (egalite de tous
  // les sorts et masses contre la baseline sans L4 ni heritage) :
  bool l4_sign = false;              // le sup substitue a l'infimum
  bool inherit_shift = false;        // handle herite decale d'une position
  bool inherit_double_count = false; // herite recompte dans un credit de noeud
  bool inherit_stale_sibling = false;// recolte transmise a un bloc etranger
  bool stop_at_inherited = false;    // (seuil-1) herites : la recherche sautee
  bool any() const {
    return gt_to_ge || threshold_minus_one || witness_duplicated || last_block_omitted ||
           duplicate_compensated || anchor_non_maximal || oracle_accept_nonpositive ||
           depth_open_boundary || depth_always_dropped || q4_degenerate_accept ||
           q4_large_to_strict || depth_prune_disarmed || depth_confounded_dropped ||
           diametral_open_to_closed || l4_sign || inherit_shift ||
           inherit_double_count || inherit_stale_sibling || stop_at_inherited;
  }
};

// LE PREDICAT POLYNOMIAL EXACT du coeur universel, par point et par paire
// exacte. `arity` est 3 ou 4. Bornes u16 : |g| < 2^38, g^2 < 2^76,
// Q < 2^75, 3g^2 < 2^78 — i128.
inline bool universal_witness(int arity, const mhgp::P3& w, const mhgp::P3& a,
                              const mhgp::P3& b, bool ge_mutant) {
  const i64 dx = (i64)b.x - a.x, dy = (i64)b.y - a.y, dz = (i64)b.z - a.z;
  const i64 ux = 2 * (i64)w.x - a.x - b.x;
  const i64 uy = 2 * (i64)w.y - a.y - b.y;
  const i64 uz = 2 * (i64)w.z - a.z - b.z;
  const i64 d2 = dx * dx + dy * dy + dz * dz;
  const i64 u2 = ux * ux + uy * uy + uz * uz;
  const i64 g = d2 - u2;
  if (ge_mutant ? g < 0 : g <= 0) return false;   // g > 0 AVANT la mise au carre
  const i64 cx = dy * uz - dz * uy;
  const i64 cy = dz * ux - dx * uz;
  const i64 cz = dx * uy - dy * ux;
  const i128 q = (i128)cx * cx + (i128)cy * cy + (i128)cz * cz;
  const i128 g2 = (i128)g * g;
  if (arity == 3) return ge_mutant ? 3 * g2 >= 4 * q : 3 * g2 > 4 * q;
  return ge_mutant ? g2 >= 2 * q : g2 > 2 * q;
}

// LE PRE-PRUNE DE BOULE INSCRITE par intervalles, sur le triple de boites :
// max ||U||^2 par axe (extremes de l'intervalle 2W - A - B) et min D^2 par
// axe (zero si les intervalles se recouvrent, sinon le carre de l'ecart).
// q3 : 3 max||U||^2 < min D^2 (STRICT) ; q4 : 15 max||U||^2 <= 4 min D^2
// (LARGE licite — note auditeur). Vrai => TOUS les points de W sont
// universels pour TOUTES les paires du bloc.
inline i64 max_u2_over_boxes(const Node& w, const Node& a, const Node& b) {
  i64 total = 0;
  for (int d = 0; d < 3; ++d) {
    const i64 low = 2 * w.lo[d] - a.hi[d] - b.hi[d];
    const i64 high = 2 * w.hi[d] - a.lo[d] - b.lo[d];
    total += std::max(low * low, high * high);
  }
  return total;
}

inline i64 min_d2_over_boxes(const Node& a, const Node& b) {
  i64 total = 0;
  for (int d = 0; d < 3; ++d) {
    const i64 low = a.lo[d] - b.hi[d];
    const i64 high = a.hi[d] - b.lo[d];
    if (low > 0) total += low * low;
    else if (high < 0) total += high * high;
    // sinon l'intervalle contient zero : contribution minimale nulle
  }
  return total;
}

// L'INFIMUM SEPARABLE EXACT L4 (x4) du triple de boites, REUTILISE de la lane
// q2 (audit etat courant : « reutiliser L4 comme rejet de noeuds temoins —
// g > 0 implique l'interieur diametral ») : par axe, pour chaque couple
// d'extremites (x,y), le minimum en w de 4(w-x)(w-y) = (2w-x-y)^2 - (x-y)^2
// est atteint a t = clip(x+y, [2wl, 2wh]) et vaut (t-2x)(t-2y) ; minimum sur
// les quatre couples puis somme des axes. `L4 >= 0` certifie qu'aucun point
// de la boite temoin n'est STRICTEMENT interieur a la boule diametrale d'une
// paire du bloc — or tout temoin universel de Jung verifie g > 0, qui est
// exactement cet interieur strict : le noeud est retire. Decision monotone
// sous raffinement ; jamais exclusif avec le pre-prune de boule inscrite
// (spindle vrai => interieurs stricts => L4 < 0). Bornes : |t-2x| < 3*2^17,
// produits < 2^38, somme < 2^40 (i64). C'est l'infimum continu sur — la
// parite grille peut rendre le minimum entier plus grand ; la borne est
// conservatrice, jamais annoncee exacte.
inline i64 pair_witness_inf4(const Node& witness, const Node& a, const Node& b) {
  i64 total = 0;
  for (int d = 0; d < 3; ++d) {
    i64 best = (i64)1 << 62;
    const i64 twice_low = 2 * witness.lo[d], twice_high = 2 * witness.hi[d];
    const i64 a_ext[2] = {a.lo[d], a.hi[d]};
    const i64 b_ext[2] = {b.lo[d], b.hi[d]};
    for (int ai = 0; ai < 2; ++ai)
      for (int bi = 0; bi < 2; ++bi) {
        i64 t = a_ext[ai] + b_ext[bi];
        if (t < twice_low) t = twice_low;
        if (t > twice_high) t = twice_high;
        const i64 value = (t - 2 * a_ext[ai]) * (t - 2 * b_ext[bi]);
        if (value < best) best = value;
      }
    total += best;
  }
  return total;
}

// Le SUP separable du meme triple (le mutant l4-sign le substitue a
// l'infimum : il retire des noeuds porteurs de temoins et le differentiel
// bi-mode le voit).
inline i64 pair_witness_sup4(const Node& witness, const Node& a, const Node& b) {
  i64 total = 0;
  for (int d = 0; d < 3; ++d) {
    i64 best = -((i64)1 << 62);
    const i64 w_ext[2] = {2 * witness.lo[d], 2 * witness.hi[d]};
    const i64 a_ext[2] = {a.lo[d], a.hi[d]};
    const i64 b_ext[2] = {b.lo[d], b.hi[d]};
    for (int wi = 0; wi < 2; ++wi)
      for (int ai = 0; ai < 2; ++ai)
        for (int bi = 0; bi < 2; ++bi) {
          const i64 value = (w_ext[wi] - 2 * a_ext[ai]) * (w_ext[wi] - 2 * b_ext[bi]);
          if (value > best) best = value;
        }
    total += best;
  }
  return total;
}

// LES TEMOINS HERITES (audit etat courant : « heriter sans perte les 9/8
// PointId deja universels ») : au plus neuf POSITIONS de feuille deja
// certifiees UNIVERSELLES pour le bloc parent — les paires d'un enfant sont
// un sous-ensemble de celles du parent et ses plages d'extremites des
// sous-plages, donc un temoin universel hors extremites du parent le reste
// pour l'enfant. Jamais un scalaire sans identifiants, jamais une frontiere
// plafonnee : la recherche ne cherche que les temoins manquants.
struct InheritedWitnesses {
  std::array<int, 9> positions{};
  int count = 0;
};

inline bool spindle_block_universal(int arity, const Node& w, const Node& a, const Node& b,
                                    const ProbeInjections& injections) {
  const i64 max_u2 = max_u2_over_boxes(w, a, b);
  const i64 min_d2 = min_d2_over_boxes(a, b);
  if (arity == 3)
    return injections.gt_to_ge ? 3 * max_u2 <= min_d2 : 3 * max_u2 < min_d2;
  // LA GARDE DEGENEREE (audit etat courant, motif 1) : l'egalite large q4
  // n'est licite QUE si min D^2 > 0 sur tout le bloc. Sur des PointId
  // colocalises, D^2 = U^2 = 0 rendrait 0 <= 0 vrai alors que le prédicat
  // exact refuse g = 0 — on descend jusqu'au prédicat ponctuel.
  if (min_d2 <= 0 && !injections.q4_degenerate_accept) return false;
  // L'egalite LARGE est licite (D/sqrt(15) < D sin(15 deg), note auditeur) ;
  // le mutant strict perd l'egalite rationnelle sure et la fixture
  // q4-rational-equality le tue.
  if (injections.q4_large_to_strict) return 15 * max_u2 < 4 * min_d2;
  return 15 * max_u2 <= 4 * min_d2;
}

// LE TEST DES HUIT COINS contre une paire EXACTE : le spindle est convexe
// (intersection de boules ouvertes) — huit coins stricts impliquent la
// boite entiere stricte, donc tous les points du noeud universels.
inline bool node_universal_for_pair(int arity, const Node& node, const mhgp::P3& a,
                                    const mhgp::P3& b, bool ge_mutant,
                                    i64* evals_out = nullptr) {
  for (int corner = 0; corner < 8; ++corner) {
    const mhgp::P3 w{(mhgp::i32)(corner & 1 ? node.hi[0] : node.lo[0]),
                     (mhgp::i32)(corner & 2 ? node.hi[1] : node.lo[1]),
                     (mhgp::i32)(corner & 4 ? node.hi[2] : node.lo[2])};
    if (evals_out != nullptr) ++*evals_out;
    if (!universal_witness(arity, w, a, b, ge_mutant)) return false;
  }
  return true;
}

enum class PairFate : std::uint8_t { kUnassigned = 0, kPruned = 1, kResidual = 2 };

// Les trois modes exiges par l'audit : le partage du parcours ne doit pas
// masquer le cout marginal d'un certificat.
enum class LaneMode : std::uint8_t { kCore = 0, kDepth = 1, kCombined = 2 };

struct LaneReceipt {
  i64 states = 0;
  i64 block_prunes = 0;         // blocs entiers prunes (coeur seulement)
  i64 l4_skipped_nodes = 0;     // noeuds temoins retires par L4 >= 0
  i64 l4_skipped_points = 0;    // points couverts par ces retraits (multiplicite)
  i64 inherited_credits = 0;    // temoins herites credites (multiplicite)
  i64 early_exits = 0;          // recherches arretees au seuil
  i64 full_plus_one = 0;        // seuil atteint avec (seuil-1) herites + du neuf
  i64 pruned_pairs = 0;
  i64 residual_pairs = 0;       // LA sortie : les candidates ancres
  i64 terminal_pairs = 0;       // paires resolues une a une
  i64 terminal_prunes = 0;      // paires prunees au terminal par le COEUR
  i64 node_visits = 0;
  i64 corner_tests = 0;         // NOEUDS soumis au test des coins (chacun
                                // execute 1 a 8 evaluations, voir corner_evals)
  i64 corner_evals = 0;         // evaluations de coins reellement executees
  i64 corner_accepts = 0;       // noeuds acceptes entiers par les huit coins
  i64 point_tests = 0;
  i64 spindle_block_hits = 0;
  i64 depth_max = 0;
  i64 stack_high_water = 0;
  // La profondeur fermee (multiplicites de travail, compteurs SEPARES du
  // coeur — l'audit refuse leur melange en mode combine) :
  i64 depth_pairs = 0;          // paires passees au sweep
  i64 depth_dead = 0;           // paires tuees par delta >= seuil
  i64 depth_witnesses = 0;      // temoins diametraux collectes
  i64 depth_always = 0;         // projections nulles (banque always)
  i64 depth_biggest_m = 0;      // plus grand m d'un sweep
  i64 depth_collect_visits = 0; // visites d'arbre de la COLLECTE diametrale
  i64 depth_collect_point_tests = 0;  // tests ponctuels de la collecte
  i64 depth_angle_groups = 0;   // groupes de rayons confondus (somme)
  i64 depth_sweep_steps = 0;    // avancees du deux-pointeurs (somme)
  i64 depth_sort_compares = 0;  // comparaisons du tri du noyau (somme — le
                                // vrai cout O(m log m), audit etat courant)
  i64 depth_scratch_capacity = 0;     // high-water des temoins collectes
  i64 depth_scratch_bytes = 0;  // high-water OCTETS : temoins + rayons +
                                // dir/count du noyau (l'audit refusait un
                                // compteur qui ignore les tampons locaux)
};

// Le certificat d'un prune, pour le rejeu de l'oracle : plages d'extremites
// et positions des temoins credites (>= seuil). `pair_a/pair_b < 0` : bloc ;
// sinon paire terminale (positions exactes). Un prune de PROFONDEUR porte le
// delta revendique : le juge le recalcule depuis les coordonnees brutes.
struct PruneCertificate {
  int a_begin = 0, a_end = 0, b_begin = 0, b_end = 0;
  int pair_a = -1, pair_b = -1;
  bool depth_kind = false;
  i64 claimed_delta = -1;
  std::vector<int> witness_positions;
};

// LE SWEEP EXACT DE PROFONDEUR FERMEE : la PRIMITIVE ANGULAIRE COMMUNE de
// `exact_ray_sweep.hpp` (tri de rayons + deux pointeurs, O(m log m) — la
// double boucle Theta(m^2) etait le NO-GO 50 k de l'audit etat courant).
// Preuve, bornes u16 et semantique des mutants : voir le header partage ;
// le juge quadratique independant reste plus bas et ne partage rien avec la
// primitive (README : « recue contre un juge quadratique independant »).
using DepthSweepStats = mhgp3v::RaySweepStats;

inline i64 exact_closed_depth(const std::vector<mhgp::P3>& pts, const mhgp::P3& a,
                              const mhgp::P3& b, const std::vector<int>& witness_ids,
                              i64* always_out, const ProbeInjections& injections,
                              DepthSweepStats* stats = nullptr) {
  mhgp3v::RaySweepInjections sweep_injections;
  sweep_injections.always_dropped = injections.depth_always_dropped;
  sweep_injections.open_boundary = injections.depth_open_boundary;
  sweep_injections.confounded_dropped = injections.depth_confounded_dropped;
  return mhgp3v::exact_closed_depth(pts, a, b, witness_ids, always_out,
                                    sweep_injections, stats);
}

struct LaneProbe {
  const Tree* tree = nullptr;
  int arity = 3;
  int threshold = 9;
  LaneMode mode = LaneMode::kCore;
  ProbeInjections injections;
  LaneReceipt receipt;
  i64 max_states = 0;
  bool budget_exceeded = false;
  // BASELINE DE RECEPTION du differentiel bi-mode : ni L4, ni heritage —
  // le comportement recu du coeur avant cette tranche, SANS les injections
  // du sujet (le juge n'herite jamais du mutant).
  bool baseline = false;
  std::vector<PairFate>* fate = nullptr;
  int fate_points = 0;
  bool fate_violated = false;
  std::vector<PruneCertificate>* certificates = nullptr;   // mode oracle
  std::vector<int> stack_;
  std::vector<int> harvest_;
  std::vector<int> diametral_scratch_;
  InheritedWitnesses last_harvest_;   // pour le mutant inherit-stale-sibling
  i64 last_residual_pair = -1;   // pour last-block-omitted

  int effective_threshold() const {
    return threshold - (injections.threshold_minus_one ? 1 : 0);
  }

  static bool ranges_disjoint(const Node& witness, const Node& a, const Node& b) {
    const bool overlap_a = witness.begin < a.end && a.begin < witness.end;
    const bool overlap_b = witness.begin < b.end && b.begin < witness.end;
    return !overlap_a && !overlap_b;
  }

  void assign_pair(int i, int j, PairFate value) {
    if (fate == nullptr) return;
    if (i > j) std::swap(i, j);
    const i64 n = fate_points;
    const i64 index = (i64)i * (2 * n - i - 1) / 2 + (j - i - 1);
    if ((*fate)[(std::size_t)index] != PairFate::kUnassigned) {
      fate_violated = true;
      return;
    }
    (*fate)[(std::size_t)index] = value;
  }

  // Compte des temoins UNIVERSELS pour tout un bloc (A,B) de boites : boule
  // inscrite par intervalles sur les noeuds, prédicat par point aux
  // feuilles. Sortie precoce au seuil ; recolte des positions pour le
  // certificat de rejeu ET pour l'heritage des enfants (au plus 9). Les trois
  // accelerateurs exacts de la gate q2, transposes au coeur : L4 d'abord
  // (aucun temoin universel sans g > 0, donc sans interieur diametral
  // strict), credits herites exclus du recomptage, pile reutilisee. La
  // baseline du differentiel n'en utilise aucun.
  i64 count_universal_block(const Node& a, const Node& b,
                            const InheritedWitnesses& inherited,
                            InheritedWitnesses* harvested) {
    harvest_.clear();
    const InheritedWitnesses empty{};
    const InheritedWitnesses& credited = baseline ? empty : inherited;
    const auto is_inherited = [&](int position) {
      for (int i = 0; i < credited.count; ++i)
        if (credited.positions[(std::size_t)i] == position) return true;
      return false;
    };
    const auto harvest = [&](int position) {
      if (harvested->count < 9)
        harvested->positions[(std::size_t)harvested->count++] = position;
    };
    harvested->count = 0;
    i64 total = credited.count;
    receipt.inherited_credits += credited.count;
    for (int i = 0; i < credited.count; ++i) {
      harvest(credited.positions[(std::size_t)i]);
      harvest_.push_back(credited.positions[(std::size_t)i]);
    }
    // MUTANT stop-at-inherited : (seuil-1) herites suffiraient — la
    // recherche du dernier temoin est sautee et le prune est perdu.
    if (injections.stop_at_inherited && credited.count >= effective_threshold() - 1)
      return total;
    if (total >= effective_threshold()) {
      ++receipt.early_exits;
      return total;
    }
    const int credit = injections.witness_duplicated ? 2 : 1;   // MUTANT : double
    stack_.clear();
    stack_.push_back(0);
    while (!stack_.empty()) {
      receipt.stack_high_water = std::max(receipt.stack_high_water, (i64)stack_.size());
      const int node_index = stack_.back();
      stack_.pop_back();
      const Node& node = tree->nodes[(std::size_t)node_index];
      ++receipt.node_visits;
      // L4 D'ABORD (jamais en baseline) : aucun interieur diametral strict
      // dans la boite pour aucune paire du bloc — aucun g > 0 possible, le
      // noeud est retire, plages d'extremites comprises. Le mutant l4-sign
      // prend le sup pour l'infimum et retire des noeuds porteurs. Le mutant
      // q4-degenerate-accept retablit la faille HISTORIQUE complete du
      // pre-prune q4 (ni garde min D^2 > 0, ni L4 — le chemin de 1dfe07b) :
      // sur des boites colocalisees, L4 = 0 rejetterait sinon les noeuds
      // avant que la fausse acceptation degeneree puisse mordre.
      if (!baseline && !injections.q4_degenerate_accept) {
        const i64 inf4 = injections.l4_sign ? pair_witness_sup4(node, a, b)
                                            : pair_witness_inf4(node, a, b);
        if (inf4 >= 0) {
          ++receipt.l4_skipped_nodes;
          receipt.l4_skipped_points += node.end - node.begin;
          continue;
        }
      }
      if (ranges_disjoint(node, a, b)) {
        if (spindle_block_universal(arity, node, a, b, injections)) {
          ++receipt.spindle_block_hits;
          i64 fresh = node.end - node.begin;
          // MUTANT inherit-double-count : l'herite present dans le noeud
          // serait recompte — le total gonfle et prune a tort.
          if (!baseline && !injections.inherit_double_count)
            for (int i = 0; i < credited.count; ++i)
              if (credited.positions[(std::size_t)i] >= node.begin &&
                  credited.positions[(std::size_t)i] < node.end)
                --fresh;
          total += credit * fresh;
          for (int t = node.begin; t < node.end; ++t)
            if (!is_inherited(t)) {
              harvest(t);
              harvest_.push_back(t);
            }
          if (total >= effective_threshold()) {
            ++receipt.early_exits;
            if (credited.count == effective_threshold() - 1) ++receipt.full_plus_one;
            return total;
          }
          continue;
        }
        if (node.left < 0) {
          for (int t = node.begin; t < node.end; ++t) {
            if (is_inherited(t)) continue;
            const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
            ++receipt.point_tests;
            // Par point contre les BOITES d'extremites : boite temoin
            // degeneree, meme borne d'intervalles — conservateur exact.
            Node point_box{};
            for (int d = 0; d < 3; ++d) {
              const i64 c = d == 0 ? (i64)w.x : (d == 1 ? (i64)w.y : (i64)w.z);
              point_box.lo[d] = c;
              point_box.hi[d] = c;
            }
            if (spindle_block_universal(arity, point_box, a, b, injections)) {
              total += credit;
              harvest(t);
              harvest_.push_back(t);
            }
          }
          if (total >= effective_threshold()) {
            ++receipt.early_exits;
            if (credited.count == effective_threshold() - 1) ++receipt.full_plus_one;
            return total;
          }
          continue;
        }
        stack_.push_back(node.right);
        stack_.push_back(node.left);
        continue;
      }
      if (node.left < 0) {
        for (int t = node.begin; t < node.end; ++t) {
          const bool in_a = t >= a.begin && t < a.end;
          const bool in_b = t >= b.begin && t < b.end;
          if (in_a || in_b) continue;
          if (is_inherited(t)) continue;
          const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
          ++receipt.point_tests;
          Node point_box{};
          for (int d = 0; d < 3; ++d) {
            const i64 c = d == 0 ? (i64)w.x : (d == 1 ? (i64)w.y : (i64)w.z);
            point_box.lo[d] = c;
            point_box.hi[d] = c;
          }
          if (spindle_block_universal(arity, point_box, a, b, injections)) {
            total += credit;
            harvest(t);
            if (certificates != nullptr && harvest_.size() < 32) harvest_.push_back(t);
          }
        }
        if (total >= effective_threshold()) {
          ++receipt.early_exits;
          if (credited.count == effective_threshold() - 1) ++receipt.full_plus_one;
          return total;
        }
        continue;
      }
      stack_.push_back(node.right);
      stack_.push_back(node.left);
    }
    return total;
  }

  // Compte des temoins universels pour une paire EXACTE (positions pos_a,
  // pos_b) : coins convexes sur les noeuds disjoints des deux positions,
  // prédicat exact par point ailleurs. Les temoins herites du bloc parent
  // restent universels pour la paire (sous-ensemble de ses paires) et hors
  // de ses extremites (sous-plages) ; L4 avec boites d'extremites degenerees
  // retire les noeuds sans interieur diametral strict.
  i64 count_universal_pair(int pos_a, int pos_b, const InheritedWitnesses& inherited) {
    harvest_.clear();
    const mhgp::P3& a = (*tree->points)[(std::size_t)tree->order[(std::size_t)pos_a]];
    const mhgp::P3& b = (*tree->points)[(std::size_t)tree->order[(std::size_t)pos_b]];
    const InheritedWitnesses empty{};
    const InheritedWitnesses& credited = baseline ? empty : inherited;
    const auto is_inherited = [&](int position) {
      for (int i = 0; i < credited.count; ++i)
        if (credited.positions[(std::size_t)i] == position) return true;
      return false;
    };
    Node box_a{}, box_b{};
    for (int d = 0; d < 3; ++d) {
      const i64 ca = d == 0 ? (i64)a.x : (d == 1 ? (i64)a.y : (i64)a.z);
      const i64 cb = d == 0 ? (i64)b.x : (d == 1 ? (i64)b.y : (i64)b.z);
      box_a.lo[d] = box_a.hi[d] = ca;
      box_b.lo[d] = box_b.hi[d] = cb;
    }
    i64 total = credited.count;
    receipt.inherited_credits += credited.count;
    for (int i = 0; i < credited.count; ++i)
      harvest_.push_back(credited.positions[(std::size_t)i]);
    if (injections.stop_at_inherited && credited.count >= effective_threshold() - 1)
      return total;
    if (total >= effective_threshold()) {
      ++receipt.early_exits;
      return total;
    }
    const int credit = injections.witness_duplicated ? 2 : 1;
    stack_.clear();
    stack_.push_back(0);
    while (!stack_.empty()) {
      receipt.stack_high_water = std::max(receipt.stack_high_water, (i64)stack_.size());
      const int node_index = stack_.back();
      stack_.pop_back();
      const Node& node = tree->nodes[(std::size_t)node_index];
      ++receipt.node_visits;
      if (!baseline) {
        const i64 inf4 = injections.l4_sign ? pair_witness_sup4(node, box_a, box_b)
                                            : pair_witness_inf4(node, box_a, box_b);
        if (inf4 >= 0) {
          ++receipt.l4_skipped_nodes;
          receipt.l4_skipped_points += node.end - node.begin;
          continue;
        }
      }
      const bool holds_endpoint = (pos_a >= node.begin && pos_a < node.end) ||
                                  (pos_b >= node.begin && pos_b < node.end);
      if (!holds_endpoint) {
        ++receipt.corner_tests;   // noeuds SOUMIS ; les evaluations reelles
                                  // (1 a 8, arret au premier echec) sont
                                  // comptees separement — audit etat courant.
        i64 evals = 0;
        const bool universal =
            node_universal_for_pair(arity, node, a, b, injections.gt_to_ge, &evals);
        receipt.corner_evals += evals;
        if (universal) {
          ++receipt.corner_accepts;
          i64 fresh = node.end - node.begin;
          if (!baseline && !injections.inherit_double_count)
            for (int i = 0; i < credited.count; ++i)
              if (credited.positions[(std::size_t)i] >= node.begin &&
                  credited.positions[(std::size_t)i] < node.end)
                --fresh;
          total += credit * fresh;
          for (int t = node.begin; t < node.end; ++t)
            if (!is_inherited(t)) harvest_.push_back(t);
          if (total >= effective_threshold()) {
            ++receipt.early_exits;
            if (credited.count == effective_threshold() - 1) ++receipt.full_plus_one;
            return total;
          }
          continue;
        }
      }
      if (node.left < 0) {
        for (int t = node.begin; t < node.end; ++t) {
          if (t == pos_a || t == pos_b) continue;
          if (is_inherited(t)) continue;
          const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
          ++receipt.point_tests;
          if (universal_witness(arity, w, a, b, injections.gt_to_ge)) {
            total += credit;
            if (certificates != nullptr && harvest_.size() < 32) harvest_.push_back(t);
          }
        }
        if (total >= effective_threshold()) {
          ++receipt.early_exits;
          if (credited.count == effective_threshold() - 1) ++receipt.full_plus_one;
          return total;
        }
        continue;
      }
      stack_.push_back(node.right);
      stack_.push_back(node.left);
    }
    return total;
  }

  void record_block_certificate(const Node& a, const Node& b) {
    if (certificates == nullptr) return;
    PruneCertificate certificate;
    certificate.a_begin = a.begin;
    certificate.a_end = a.end;
    certificate.b_begin = b.begin;
    certificate.b_end = b.end;
    certificate.witness_positions = harvest_;
    certificates->push_back(std::move(certificate));
  }

  void record_pair_certificate(int pos_a, int pos_b) {
    if (certificates == nullptr) return;
    PruneCertificate certificate;
    certificate.pair_a = pos_a;
    certificate.pair_b = pos_b;
    certificate.witness_positions = harvest_;
    certificates->push_back(std::move(certificate));
  }

  // COLLECTE des temoins stricts de la boule diametrale d'une paire EXACTE,
  // par l'arbre : sup < 0 sur un noeud disjoint des positions d'extremites
  // credite tout le noeud, l'infimum >= 0 le retire, les feuilles testent le
  // prédicat exact (2w-2a).(2w-2b), positions d'extremites exclues.
  void collect_diametral_witnesses(int pos_a, int pos_b, std::vector<int>* out) {
    out->clear();
    const mhgp::P3& a = (*tree->points)[(std::size_t)tree->order[(std::size_t)pos_a]];
    const mhgp::P3& b = (*tree->points)[(std::size_t)tree->order[(std::size_t)pos_b]];
    const i64 ax[3] = {(i64)a.x, (i64)a.y, (i64)a.z};
    const i64 bx[3] = {(i64)b.x, (i64)b.y, (i64)b.z};
    stack_.clear();
    stack_.push_back(0);
    while (!stack_.empty()) {
      const int node_index = stack_.back();
      stack_.pop_back();
      const Node& node = tree->nodes[(std::size_t)node_index];
      ++receipt.depth_collect_visits;   // compteur SEPARE du coeur (audit)
      // Par axe, 4*(w-a)(w-b) = V^2 - d^2 avec V = 2w-a-b : le maximum sur
      // [lo,hi] est aux extremes ; le minimum CONTINU vaut -d^2 quand
      // l'intervalle contient zero (V=0), sinon l'extreme le plus proche.
      // C'est l'infimum continu SUR, pas l'infimum grille : sur u16, la
      // parite de V peut rendre le minimum entier strictement plus grand
      // (audit, profondeur point 4) — la borne est conservatrice (elle ne
      // fabrique jamais un faux temoin), elle n'est pas annoncee exacte.
      i64 sup = 0, inf = 0;
      for (int d = 0; d < 3; ++d) {
        const i64 lo = 2 * node.lo[d] - ax[d] - bx[d];
        const i64 hi = 2 * node.hi[d] - ax[d] - bx[d];
        const i64 dd = bx[d] - ax[d];
        const i64 pa = lo * lo - dd * dd;
        const i64 pb = hi * hi - dd * dd;
        sup += std::max(pa, pb);
        inf += (lo <= 0 && 0 <= hi) ? -(dd * dd) : std::min(pa, pb);
      }
      // MUTANT diametral-open-to-closed : le collecteur crediterait les
      // CONTACTS (dot = 0, points de coquille diametrale) comme temoins —
      // delta surestime, la fixture diametral-contact le tue par faux prune.
      const bool closed_mutant = injections.diametral_open_to_closed;
      if (closed_mutant ? inf > 0 : inf >= 0) continue;   // aucun temoin strict
      const bool holds_endpoint = (pos_a >= node.begin && pos_a < node.end) ||
                                  (pos_b >= node.begin && pos_b < node.end);
      if (sup < 0 && !holds_endpoint) {
        for (int t = node.begin; t < node.end; ++t)
          out->push_back(tree->order[(std::size_t)t]);
        continue;
      }
      if (node.left < 0) {
        for (int t = node.begin; t < node.end; ++t) {
          if (t == pos_a || t == pos_b) continue;
          const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
          ++receipt.depth_collect_point_tests;
          i64 dot = 0;
          for (int d = 0; d < 3; ++d) {
            const i64 c = d == 0 ? (i64)w.x : (d == 1 ? (i64)w.y : (i64)w.z);
            dot += (2 * c - ax[d] - bx[d] - (bx[d] - ax[d])) *
                   (2 * c - ax[d] - bx[d] + (bx[d] - ax[d]));
          }
          if (closed_mutant ? dot <= 0 : dot < 0)
            out->push_back(tree->order[(std::size_t)t]);
        }
        continue;
      }
      stack_.push_back(node.right);
      stack_.push_back(node.left);
    }
  }

  // Le sweep de profondeur pour une paire : delta = always + m - max ouvert.
  i64 pair_depth(int pos_a, int pos_b) {
    collect_diametral_witnesses(pos_a, pos_b, &diametral_scratch_);
    const mhgp::P3& a = (*tree->points)[(std::size_t)tree->order[(std::size_t)pos_a]];
    const mhgp::P3& b = (*tree->points)[(std::size_t)tree->order[(std::size_t)pos_b]];
    i64 always = 0;
    DepthSweepStats stats;
    const i64 delta = exact_closed_depth(*tree->points, a, b, diametral_scratch_, &always,
                                         injections, &stats);
    ++receipt.depth_pairs;
    receipt.depth_witnesses += (i64)diametral_scratch_.size();
    receipt.depth_always += always;
    receipt.depth_biggest_m =
        std::max(receipt.depth_biggest_m, (i64)diametral_scratch_.size());
    receipt.depth_angle_groups += stats.groups;
    receipt.depth_sweep_steps += stats.steps;
    receipt.depth_sort_compares += stats.sort_compares;
    receipt.depth_scratch_capacity =
        std::max(receipt.depth_scratch_capacity, (i64)diametral_scratch_.capacity());
    receipt.depth_scratch_bytes = std::max(
        receipt.depth_scratch_bytes,
        stats.scratch_bytes + (i64)(diametral_scratch_.capacity() * sizeof(int)));
    return delta;
  }

  // Resolution TERMINALE d'une paire : prunee ou residuelle, sort unique.
  // core : coeur universel seul ; depth : profondeur seule ; combined :
  // coeur d'abord (sortie precoce), profondeur pour les survivantes.
  void resolve_pair(int pos_a, int pos_b, const InheritedWitnesses& inherited) {
    ++receipt.terminal_pairs;
    const int ia = tree->order[(std::size_t)pos_a];
    const int ib = tree->order[(std::size_t)pos_b];
    if (mode != LaneMode::kDepth) {
      const i64 found = count_universal_pair(pos_a, pos_b, inherited);
      if (found >= effective_threshold()) {
        ++receipt.pruned_pairs;
        ++receipt.terminal_prunes;
        assign_pair(ia, ib, PairFate::kPruned);
        record_pair_certificate(pos_a, pos_b);
        return;
      }
    }
    if (mode != LaneMode::kCore) {
      const i64 delta = pair_depth(pos_a, pos_b);
      // MUTANT depth-prune-disarmed : la decision delta >= seuil est
      // debranchee — le sweep tourne mais ne tue plus rien ; les fixtures
      // nine-axial/eight-axial en mode depth exigent le prune et le tuent.
      if (!injections.depth_prune_disarmed && delta >= effective_threshold()) {
        ++receipt.depth_dead;
        ++receipt.pruned_pairs;
        assign_pair(ia, ib, PairFate::kPruned);
        if (certificates != nullptr) {
          PruneCertificate certificate;
          certificate.pair_a = pos_a;
          certificate.pair_b = pos_b;
          certificate.depth_kind = true;
          certificate.claimed_delta = delta;
          certificates->push_back(std::move(certificate));
        }
        return;
      }
    }
    ++receipt.residual_pairs;
    last_residual_pair = (i64)pos_a * tree->order.size() + pos_b;
    assign_pair(ia, ib, PairFate::kResidual);
  }

  void process(int ia, int ib, i64 depth, const InheritedWitnesses& inherited) {
    if (budget_exceeded) return;
    ++receipt.states;
    receipt.depth_max = std::max(receipt.depth_max, depth);
    if (max_states > 0 && receipt.states > max_states) {
      budget_exceeded = true;
      return;
    }
    const Node& a = tree->nodes[(std::size_t)ia];
    const Node& b = tree->nodes[(std::size_t)ib];
    if (ia == ib) {
      if (a.left < 0) {
        for (int ta = a.begin; ta < a.end; ++ta)
          for (int tb = ta + 1; tb < a.end; ++tb) resolve_pair(ta, tb, {});
        return;
      }
      // Aucun heritage interne : le parent interne n'a pas de recherche.
      // MUTANT inherit-stale-sibling : la recolte du DERNIER bloc croise est
      // transmise a des blocs qui ne l'ont jamais validee.
      InheritedWitnesses internal_inherit{};
      if (injections.inherit_stale_sibling) internal_inherit = last_harvest_;
      process(a.left, a.left, depth + 1, internal_inherit);
      process(a.left, a.right, depth + 1, internal_inherit);
      process(a.right, a.right, depth + 1, internal_inherit);
      return;
    }
    // En mode profondeur SEULE, aucun prune de bloc : l'isolation du cout du
    // sweep exige que chaque paire atteigne son terminal.
    InheritedWitnesses harvested{};
    if (mode != LaneMode::kDepth) {
      const i64 found = count_universal_block(a, b, inherited, &harvested);
      last_harvest_ = harvested;
      // MUTANT inherit-shift : chaque handle transmis est decale d'une
      // position — le fils crediterait d'autres points que les certifies.
      if (injections.inherit_shift) {
        const int size = (int)tree->order.size();
        for (int i = 0; i < harvested.count; ++i)
          harvested.positions[(std::size_t)i] =
              (harvested.positions[(std::size_t)i] + 1) % size;
      }
      if (found >= effective_threshold()) {
        ++receipt.block_prunes;
        const i64 pairs = (i64)(a.end - a.begin) * (i64)(b.end - b.begin);
        receipt.pruned_pairs += pairs;
        record_block_certificate(a, b);
        if (fate != nullptr)
          for (int ta = a.begin; ta < a.end; ++ta)
            for (int tb = b.begin; tb < b.end; ++tb)
              assign_pair(tree->order[(std::size_t)ta], tree->order[(std::size_t)tb],
                          PairFate::kPruned);
        return;
      }
    }
    const bool a_leaf = a.left < 0, b_leaf = b.left < 0;
    if (a_leaf && b_leaf) {
      for (int ta = a.begin; ta < a.end; ++ta)
        for (int tb = b.begin; tb < b.end; ++tb) resolve_pair(ta, tb, harvested);
      return;
    }
    const bool split_a = !a_leaf && (b_leaf || (a.end - a.begin) >= (b.end - b.begin));
    const int split_index = split_a ? ia : ib;
    const int child_left = tree->nodes[(std::size_t)split_index].left;
    const int child_right = tree->nodes[(std::size_t)split_index].right;
    const Node& left_node = tree->nodes[(std::size_t)child_left];
    const Node& right_node = tree->nodes[(std::size_t)child_right];
    if (injections.duplicate_compensated &&
        left_node.end - left_node.begin == right_node.end - right_node.begin) {
      // MUTANT COMPENSE : les ledgers agreges ferment, seul le sort par
      // paire voit la duplication et l'omission.
      if (split_a) {
        process(child_left, ib, depth + 1, harvested);
        process(child_left, ib, depth + 1, harvested);
      } else {
        process(ia, child_left, depth + 1, harvested);
        process(ia, child_left, depth + 1, harvested);
      }
      return;
    }
    if (split_a) {
      process(child_left, ib, depth + 1, harvested);
      process(child_right, ib, depth + 1, harvested);
    } else {
      process(ia, child_left, depth + 1, harvested);
      process(ia, child_right, depth + 1, harvested);
    }
  }
};

// L'ORACLE EXHAUSTIF (n <= 32) : miniboule de chaque tuple, support propre
// positif ssi n_support == arite, census p exact, non-inerte ssi p < seuil,
// ancre canonique = plus petite paire parmi les aretes de longueur maximale.
struct OracleAnchor {
  int a = -1, b = -1;   // PointId, a < b
};

inline i64 dist2_points(const mhgp::P3& u, const mhgp::P3& v) {
  const i64 dx = (i64)u.x - v.x, dy = (i64)u.y - v.y, dz = (i64)u.z - v.z;
  return dx * dx + dy * dy + dz * dz;
}

inline OracleAnchor canonical_anchor(const std::vector<mhgp::P3>& pts, const int* ids,
                                     int arity, bool non_maximal_mutant) {
  OracleAnchor anchor;
  i64 best = non_maximal_mutant ? ((i64)1 << 62) : -1;
  for (int i = 0; i < arity; ++i)
    for (int j = i + 1; j < arity; ++j) {
      const i64 d2 = dist2_points(pts[(std::size_t)ids[i]], pts[(std::size_t)ids[j]]);
      int lo = ids[i], hi = ids[j];
      if (lo > hi) std::swap(lo, hi);
      const bool better =
          non_maximal_mutant
              ? (d2 < best || (d2 == best && (lo < anchor.a || (lo == anchor.a && hi < anchor.b))))
              : (d2 > best || (d2 == best && (lo < anchor.a || (lo == anchor.a && hi < anchor.b))));
      if (better) {
        best = d2;
        anchor.a = lo;
        anchor.b = hi;
      }
    }
  return anchor;
}

// LE JUGE DE PROFONDEUR INDEPENDANT (audit etat courant, profondeur point
// 2) : un calcul brut de delta qui ne partage NI la collecte par l'arbre, NI
// la base mediatrice, NI l'identite « min ferme = always + m - max ouvert »
// du sujet. Il collecte P par balayage direct O(n) du nuage, projette dans
// une AUTRE base entiere (e1' = (0,-dz,dy), ou (0,1,0) si d est axial en x),
// et MINIMISE DIRECTEMENT le compte ferme h(nu) par perturbation symbolique :
// h est constant par morceaux sur le cercle et ses sauts sont aux
// perpendiculaires des w_j ; chaque arc ouvert adjacent a un candidat
// nu ∈ {±w_j, ±rot90(w_j)} est visite par les versions epsilon ∈ {-1,0,+1}
// (rotation infinitesimale : le signe en w.nu = 0 devient celui de
// eps*cross(nu,w)). O(m^2) — juge borne n <= 32, jamais un chemin produit.
inline i64 reference_closed_depth(const std::vector<mhgp::P3>& pts, int id_a, int id_b) {
  const mhgp::P3& a = pts[(std::size_t)id_a];
  const mhgp::P3& b = pts[(std::size_t)id_b];
  const i64 dx = (i64)b.x - a.x, dy = (i64)b.y - a.y, dz = (i64)b.z - a.z;
  i64 e1x, e1y, e1z;
  if (dy != 0 || dz != 0) { e1x = 0; e1y = -dz; e1z = dy; }
  else { e1x = 0; e1y = 1; e1z = 0; }
  const i64 e2x = dy * e1z - dz * e1y;
  const i64 e2y = dz * e1x - dx * e1z;
  const i64 e2z = dx * e1y - dy * e1x;
  i64 always = 0;
  std::vector<std::pair<i64, i64>> planar;
  for (int x = 0; x < (int)pts.size(); ++x) {
    if (x == id_a || x == id_b) continue;
    const mhgp::P3& w = pts[(std::size_t)x];
    const i64 va[3] = {(i64)w.x - a.x, (i64)w.y - a.y, (i64)w.z - a.z};
    const i64 vb[3] = {(i64)w.x - b.x, (i64)w.y - b.y, (i64)w.z - b.z};
    if (va[0] * vb[0] + va[1] * vb[1] + va[2] * vb[2] >= 0) continue;   // P strict
    const i64 vx = 2 * (i64)w.x - a.x - b.x;
    const i64 vy = 2 * (i64)w.y - a.y - b.y;
    const i64 vz = 2 * (i64)w.z - a.z - b.z;
    const i64 cx = dy * vz - dz * vy;
    const i64 cy = dz * vx - dx * vz;
    const i64 cz = dx * vy - dy * vx;
    if (cx == 0 && cy == 0 && cz == 0) { ++always; continue; }
    planar.push_back({vx * e1x + vy * e1y + vz * e1z, vx * e2x + vy * e2y + vz * e2z});
  }
  if (planar.empty()) return always;
  i64 best = -1;
  const auto closed_count = [&](i64 nx, i64 ny, int eps) {
    i64 total = always;
    for (const auto& w : planar) {
      const i128 s = (i128)w.first * nx + (i128)w.second * ny;
      if (s > 0) { ++total; continue; }
      if (s != 0) continue;
      if (eps == 0) { ++total; continue; }
      const i128 cr = (i128)nx * w.second - (i128)ny * w.first;   // cross(nu, w)
      if ((eps > 0 && cr > 0) || (eps < 0 && cr < 0)) ++total;
    }
    return total;
  };
  for (const auto& w : planar)
    for (int sign = -1; sign <= 1; sign += 2)
      for (int rot = 0; rot < 2; ++rot) {
        const i64 nx = rot == 0 ? sign * w.first : sign * -w.second;
        const i64 ny = rot == 0 ? sign * w.second : sign * w.first;
        for (int eps = -1; eps <= 1; ++eps) {
          const i64 h = closed_count(nx, ny, eps);
          if (best < 0 || h < best) best = h;
        }
      }
  return best;
}

}  // namespace

int main(int argc, char** argv) {
  int n = 2400, coord = 0, leaf_size = 8, oracle = 0;
  i64 seed = 20260810, max_states = 200000000;
  i64 min_block_prunes = 0, min_corner_accepts = 0, min_terminal_prunes = 0;
  i64 min_depth_dead = 0, min_depth_pairs = 0, min_full_plus_one = 0;
  int lane_q3 = 1, lane_q4 = 1, differential = 0;
  mhgp3v::CloudFamily family = mhgp3v::CloudFamily::kTerrain;
  std::string fixture_name, mode = "core";
  ProbeInjections injections;
  auto integer = [](const char* text, i64* value) {
    const char* last = text + strlen(text);
    unsigned long long magnitude = 0;
    const auto r = std::from_chars(text, last, magnitude);
    if (text == last || r.ec != std::errc{} || r.ptr != last) return false;
    if (magnitude > 1000000000ULL) return false;
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
    if (!strcmp(argv[i], "--mode")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --mode\n"); return 2; }
      mode = argv[++i];
      continue;
    }
    if (!strcmp(argv[i], "--lane")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --lane\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "q3")) { lane_q3 = 1; lane_q4 = 0; }
      else if (!strcmp(argv[i], "q4")) { lane_q3 = 0; lane_q4 = 1; }
      else if (!strcmp(argv[i], "both")) { lane_q3 = 1; lane_q4 = 1; }
      else { std::printf("ECHEC : lane inconnue %s\n", argv[i]); return 2; }
      continue;
    }
    if (!strcmp(argv[i], "--inject")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --inject\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "gt-to-ge")) injections.gt_to_ge = true;
      else if (!strcmp(argv[i], "threshold-minus-one")) injections.threshold_minus_one = true;
      else if (!strcmp(argv[i], "witness-duplicated")) injections.witness_duplicated = true;
      else if (!strcmp(argv[i], "last-block-omitted")) injections.last_block_omitted = true;
      else if (!strcmp(argv[i], "duplicate-compensated"))
        injections.duplicate_compensated = true;
      else if (!strcmp(argv[i], "anchor-non-maximal")) injections.anchor_non_maximal = true;
      else if (!strcmp(argv[i], "oracle-accept-nonpositive"))
        injections.oracle_accept_nonpositive = true;
      else if (!strcmp(argv[i], "depth-open-boundary"))
        injections.depth_open_boundary = true;
      else if (!strcmp(argv[i], "depth-always-dropped"))
        injections.depth_always_dropped = true;
      else if (!strcmp(argv[i], "q4-degenerate-accept"))
        injections.q4_degenerate_accept = true;
      else if (!strcmp(argv[i], "q4-large-to-strict"))
        injections.q4_large_to_strict = true;
      else if (!strcmp(argv[i], "depth-prune-disarmed"))
        injections.depth_prune_disarmed = true;
      else if (!strcmp(argv[i], "depth-confounded-dropped"))
        injections.depth_confounded_dropped = true;
      else if (!strcmp(argv[i], "diametral-open-to-closed"))
        injections.diametral_open_to_closed = true;
      else if (!strcmp(argv[i], "l4-sign")) injections.l4_sign = true;
      else if (!strcmp(argv[i], "inherit-shift")) injections.inherit_shift = true;
      else if (!strcmp(argv[i], "inherit-double-count"))
        injections.inherit_double_count = true;
      else if (!strcmp(argv[i], "inherit-stale-sibling"))
        injections.inherit_stale_sibling = true;
      else if (!strcmp(argv[i], "stop-at-inherited")) injections.stop_at_inherited = true;
      else { std::printf("ECHEC : injection inconnue %s\n", argv[i]); return 2; }
      continue;
    }
    i64 value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    if (!has) { std::printf("ECHEC : argument %s sans valeur\n", argv[i]); return 2; }
    if (!strcmp(argv[i], "--points")) n = (int)value;
    else if (!strcmp(argv[i], "--coord")) coord = (int)value;
    else if (!strcmp(argv[i], "--leaf-size")) leaf_size = (int)value;
    else if (!strcmp(argv[i], "--seed")) seed = value;
    else if (!strcmp(argv[i], "--max-states")) max_states = value;
    else if (!strcmp(argv[i], "--oracle")) oracle = (int)value;
    // LES PLANCHERS ANTI VERT-PAR-VACUITE (audit, coeur point 5 : « des
    // planchers separes pour blocs, coins et terminaux ») : chaque lane doit
    // atteindre chaque plancher actif, sinon code 3.
    else if (!strcmp(argv[i], "--min-block-prunes")) min_block_prunes = value;
    else if (!strcmp(argv[i], "--min-corner-accepts")) min_corner_accepts = value;
    else if (!strcmp(argv[i], "--min-terminal-prunes")) min_terminal_prunes = value;
    else if (!strcmp(argv[i], "--min-depth-dead")) min_depth_dead = value;
    else if (!strcmp(argv[i], "--min-depth-pairs")) min_depth_pairs = value;
    else if (!strcmp(argv[i], "--differential")) differential = (int)value;
    else if (!strcmp(argv[i], "--min-full-plus-one")) min_full_plus_one = value;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    ++i;
  }
  LaneMode lane_mode;
  if (mode == "core") lane_mode = LaneMode::kCore;
  else if (mode == "depth") lane_mode = LaneMode::kDepth;
  else if (mode == "combined") lane_mode = LaneMode::kCombined;
  else {
    std::printf("ECHEC : mode inconnu %s (core, depth, combined)\n", mode.c_str());
    return 2;
  }
  if (n < 4 || n > 100000 || coord < 0 || coord > 65536 || leaf_size < 2 ||
      leaf_size > 256 || max_states < 1 || oracle < 0 || oracle > 1 ||
      differential < 0 || differential > 1) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }
  // LE DIFFERENTIEL BI-MODE (reception L4/heritage du coeur, miroir de la
  // gate q2) : sorts et masses compares a une baseline sans L4 ni heritage.
  // Il n'a de sens qu'en mode core ; un plancher full-plus-one sans
  // differentiel REFUSE (l'audit q2 a montre qu'un plancher silencieusement
  // ignore est un trou de gate).
  if (differential == 1 && mode != "core") {
    std::printf("ECHEC : --differential exige --mode core\n");
    return 2;
  }
  if (min_full_plus_one > 0 && differential != 1) {
    std::printf("ECHEC : --min-full-plus-one exige --differential 1 — un plancher"
                " ignore serait un trou de gate\n");
    return 2;
  }
  if (differential == 1 && n > 3000) {
    std::printf("ECHEC : le differentiel bi-mode exige n <= 3000 (deux tableaux de"
                " sorts)\n");
    return 2;
  }

  const bool is_fixture = !fixture_name.empty();
  std::vector<mhgp::P3> pts;
  // Les fixtures gravees : voir les assertions apres le run.
  if (is_fixture) {
    if (fixture_name == "thin-acute") {
      // Triangle acutangle MINCE sur le cercle de rayon 1025 : l'arete
      // courte AB longe la sphere, son spindle q3 depasse la boule — neuf
      // temoins hors de la boule la prunent, tandis que le support
      // {A,B,C} reste non inerte et ses aretes maximales AC=BC (ex aequo)
      // survivent. L'ancre canonique est (0,2).
      pts = {mhgp::P3{11023, 10064, 10000}, mhgp::P3{11023, 9936, 10000},
             mhgp::P3{8975, 10000, 10000}};
      for (int j = 0; j < 9; ++j)
        pts.push_back(mhgp::P3{(mhgp::i32)(11026 + j), 10000, 10000});
    } else if (fixture_name == "contact-q3") {
      // La fixture de CONTACT de la note auditeur : equilateral propre
      // a=(10,10,10), b=(13,13,10), z=(13,10,13), centre (12,11,11),
      // r^2=6 ; w=(11,12,9) est extra-shell et satisfait EXACTEMENT
      // 3g^2=4Q et 3||U||^2=D^2 pour la SEULE paire (a,b) — les deux autres
      // aretes du support ont g < 0 (correction d'audit) ; strict le refuse,
      // large le compterait (assertions directes du prédicat plus bas).
      pts = {mhgp::P3{10, 10, 10}, mhgp::P3{13, 13, 10}, mhgp::P3{13, 10, 13},
             mhgp::P3{11, 12, 9}};
    } else if (fixture_name == "contact-q4") {
      // Tetraedre regulier propre r^2=27, w=(15,11,11) avec g^2=2Q exact.
      pts = {mhgp::P3{13, 13, 13}, mhgp::P3{13, 7, 7}, mhgp::P3{7, 13, 7},
             mhgp::P3{7, 7, 13}, mhgp::P3{15, 11, 11}};
    } else if (fixture_name == "nine-axial" || fixture_name == "eight-axial" ||
               fixture_name == "seven-axial") {
      // Les fixtures AXIALES de la reponse d'audit : k points strictement
      // sur le segment ouvert (a,b). Chaque point axial strict est
      // UNIVERSEL (Q = 0, g > 0). nine : q3 ET q4 prunent (9 >= 9 > 8).
      // eight : q3 residuelle (8 < 9), q4 prunee (8 >= 8). seven : les
      // deux residuelles cote q3, q4 residuelle (7 < 8).
      const int axial = fixture_name == "nine-axial" ? 9
                        : fixture_name == "eight-axial" ? 8 : 7;
      pts = {mhgp::P3{30000, 30000, 30000}, mhgp::P3{30100, 30000, 30000}};
      for (int j = 0; j < axial; ++j)
        pts.push_back(mhgp::P3{(mhgp::i32)(30010 + 10 * j), 30000, 30000});
    } else if (fixture_name == "q4-tetra") {
      // Tetraedre regulier de centre (39995,39995,39995) et rayon carre 108
      // (correction d'audit : pas 3) : support q4 propre, p=0, non inerte,
      // six aretes ex aequo (288) — l'ancre canonique (0,1) doit etre
      // residuelle en q4.
      pts = {mhgp::P3{40001, 40001, 40001}, mhgp::P3{40001, 39989, 39989},
             mhgp::P3{39989, 40001, 39989}, mhgp::P3{39989, 39989, 40001}};
    } else if (fixture_name == "q4-colocated") {
      // LA FIXTURE DEGENEREE q4 (audit, coeur point 1) : vingt PointId
      // distincts a la MEME coordonnee — assez pour qu'un bloc (2,2) ait
      // seize temoins externes, au-dessus du seuil 8. Pour toute paire et
      // tout temoin, D^2 = U^2 = g = 0 : le prédicat ponctuel refuse tout,
      // et l'ancienne comparaison large 15*0 <= 4*0 fabriquait un
      // certificat de bloc faux. Nominal : AUCUN prune (190 residuelles par
      // lane, zero bloc) ; le mutant q4-degenerate-accept retablit la
      // faille et meurt au rejeu (temoins a g=0 non rejouables).
      for (int j = 0; j < 20; ++j) pts.push_back(mhgp::P3{12345, 23456, 34567});
    } else if (fixture_name == "q4-rational-equality") {
      // L'EGALITE SURE DU SOUS-TEST RATIONNEL q4 (note auditeur) :
      // a=(20,20,20), b=(30,24,22), w=(27,24,21) donnent D^2=120, U^2=32,
      // donc 15*U^2 = 4*D^2 = 480 EXACTEMENT — l'egalite large est licite
      // (D/sqrt(15) < D sin 15 deg) et le prédicat exact confirme g=88,
      // Q=704, g^2=7744 > 2Q=1408. Le mutant strict q4-large-to-strict
      // perdrait ce certificat de bloc et meurt ici.
      pts = {mhgp::P3{20, 20, 20}, mhgp::P3{30, 24, 22}, mhgp::P3{27, 24, 21}};
    } else if (fixture_name == "u16-extremes-anchor") {
      // LES EXTREMES u16 (audit, coeur point 5) : la paire diagonale
      // (0,0,0)-(65535,65535,65535) est la magnitude maximale du profil ;
      // neuf temoins diagonaux stricts (colineaires : Q=0, g>0) sont
      // universels pour q3 ET q4 — la paire (0,1) est prunee dans les deux
      // lanes, en mode core comme en mode depth (projections nulles :
      // always=9, delta=9). Aucun produit ne depasse 2^78 (documente au
      // prédicat) ; le sweep borne son dot par 2^107 < 2^127.
      pts = {mhgp::P3{0, 0, 0}, mhgp::P3{65535, 65535, 65535}};
      for (int j = 1; j <= 9; ++j) {
        const mhgp::i32 t = (mhgp::i32)(6000 * j + 2000);
        pts.push_back(mhgp::P3{t, t, t});
      }
    } else if (fixture_name == "u16-extremes-skew") {
      // LES EXTREMES u16 NON COLINEAIRES (audit etat courant, profondeur
      // durcissement 2 : « u16-extremes-anchor n'exerce que neuf projections
      // nulles ») : paire diagonale maximale (0,0,0)-(65535,65535,65535) et
      // NEUF PAIRES ANTIPODALES EXACTES de temoins stricts hors axe — chaque
      // reflet w' = (a+b) - w donne V' = -V exactement. Verification entiere
      // externe rejouee par le juge n<=32 : dix-huit rayons non nuls, zero
      // projection nulle, aucune paire confondue hors des reflets, max
      // ouvert = 9 (un rayon par paire antipodale, ancre comprise),
      // delta = 0 + 18 - 9 = 9 — prune q3 (9>=9) ET q4 (9>=8). Magnitudes
      // exercees : |w1| ~ 2^33, |w2| ~ 2^49.8, cross ~ 2^82.8 < 2^89,
      // dot ~ 2^99.6 < 2^107 — les bornes i128 documentees du noyau mordent
      // enfin sur du non-colineaire.
      pts = {mhgp::P3{0, 0, 0}, mhgp::P3{65535, 65535, 65535}};
      const int skew[18][3] = {
          {65535, 0, 32768},     {0, 65535, 32767},
          {0, 32768, 65535},     {65535, 32767, 0},
          {32768, 65535, 0},     {32767, 0, 65535},
          {65535, 32768, 0},     {0, 32767, 65535},
          {32768, 0, 65535},     {32767, 65535, 0},
          {0, 65535, 32768},     {65535, 0, 32767},
          {49151, 65535, 1},     {16384, 0, 65534},
          {1, 16384, 65535},     {65534, 49151, 0},
          {65535, 16384, 49152}, {0, 49151, 16383}};
      for (const auto& s : skew)
        pts.push_back(mhgp::P3{(mhgp::i32)s[0], (mhgp::i32)s[1], (mhgp::i32)s[2]});
      mode = "depth";
      lane_mode = LaneMode::kDepth;
    } else if (fixture_name == "diametral-contact") {
      // LES CONTACTS DU PREDICAT DIAMETRAL (audit, profondeur point 3) :
      // vingt points EXACTEMENT sur la sphere diametrale de (a,b)
      // (m=(1005,1000,1000), r^2=25 ; tranches x=1001,1005,1009). Le
      // collecteur strict n'en credite AUCUN : P est vide, delta=0, la
      // paire (0,1) reste residuelle en profondeur. Le mutant
      // diametral-open-to-closed les crediterait : les vingt directions se
      // repartissent en six paires antipodales de sorte que TOUT demi-plan
      // ouvert generique en contient exactement 10 — delta mutant
      // 20-10=10 >= 9, un FAUX prune q3 qui tue le mutant.
      pts = {mhgp::P3{1000, 1000, 1000}, mhgp::P3{1010, 1000, 1000}};
      const int shell[20][3] = {
          {1005, 1005, 1000}, {1005, 995, 1000}, {1005, 1000, 1005},
          {1005, 1000, 995},  {1005, 1003, 1004}, {1005, 1003, 996},
          {1005, 997, 1004},  {1005, 997, 996},  {1005, 1004, 1003},
          {1005, 1004, 997},  {1005, 996, 1003}, {1005, 996, 997},
          {1001, 1003, 1000}, {1001, 997, 1000}, {1001, 1000, 1003},
          {1001, 1000, 997},  {1009, 1003, 1000}, {1009, 997, 1000},
          {1009, 1000, 1003}, {1009, 1000, 997}};
      for (const auto& s : shell)
        pts.push_back(mhgp::P3{(mhgp::i32)s[0], (mhgp::i32)s[1], (mhgp::i32)s[2]});
      mode = "depth";
      lane_mode = LaneMode::kDepth;
    } else if (fixture_name == "confounded-arc") {
      // LE MUTANT DES RAYONS CONFONDUS (audit, profondeur point 3) : onze
      // temoins CONFONDUS sur le rayon +x du plan mediateur et neuf temoins
      // etales dans le demi-plan superieur. Le max ouvert nominal est 20
      // (arc ancre sur +x : les onze confondus + les neuf etales) donc
      // delta = 20-20 = 0 : la paire (0,1) reste residuelle. Le mutant
      // depth-confounded-dropped ote dix confondus de l'arc : max ouvert
      // 10, delta 10 >= 9 — un FAUX prune q3 qui le tue.
      pts = {mhgp::P3{2000, 2000, 2000}, mhgp::P3{2000, 2000, 2100}};
      for (int k = 1; k <= 11; ++k)
        pts.push_back(mhgp::P3{(mhgp::i32)(2000 + k), 2000, 2050});
      const int spread[9][2] = {{5, 1}, {4, 1}, {3, 1}, {2, 1}, {1, 1},
                                {1, 2}, {1, 3}, {1, 4}, {1, 5}};
      for (const auto& s : spread)
        pts.push_back(mhgp::P3{(mhgp::i32)(2000 + s[0]), (mhgp::i32)(2000 + s[1]), 2050});
      mode = "depth";
      lane_mode = LaneMode::kDepth;
    } else if (fixture_name == "depth-rays") {
      // LE SWEEP AUX CAS DE BORD (rayons confondus, antipodes, banque
      // always) : paire axiale en z, deux temoins confondus +x, deux
      // antipodes -x, un +y, un AXIAL (projection nulle). Derive a la main :
      // always = 1, m = 5, max ouvert = 3 (arc ancre en +x : les deux
      // confondus + le +y, antipodes exclus) — delta = 1 + 5 - 3 = 3,
      // invariant par permutation des temoins.
      pts = {mhgp::P3{500, 500, 400}, mhgp::P3{500, 500, 600},
             mhgp::P3{530, 500, 500}, mhgp::P3{540, 500, 500},
             mhgp::P3{460, 500, 500}, mhgp::P3{450, 500, 500},
             mhgp::P3{500, 530, 500}, mhgp::P3{500, 500, 520}};
      mode = "combined";
      lane_mode = LaneMode::kCombined;
    } else if (fixture_name == "scope-depth") {
      // Le nuage de portee de l'audit, cote PROFONDEUR : les dix temoins de
      // (a,b) se projettent TOUS sur le meme rayon du plan mediateur
      // (z = 100 constant, y < 100) — max ouvert = m = 10, delta = 0. La
      // paire SURVIT a la profondeur comme au coeur : ab reste l'ancre du
      // support q3 {a,b,z}, dans les deux formalismes.
      pts.push_back(mhgp::P3{50, 100, 100});
      pts.push_back(mhgp::P3{150, 100, 100});
      pts.push_back(mhgp::P3{100, 160, 100});
      const i64 scope_witnesses[10][2] = {{51, 95}, {149, 95}, {51, 94}, {149, 94},
                                          {51, 93}, {149, 93}, {52, 92}, {148, 92},
                                          {51, 92}, {149, 92}};
      for (const auto& w : scope_witnesses)
        pts.push_back(mhgp::P3{(mhgp::i32)w[0], (mhgp::i32)w[1], 100});
      mode = "combined";
      lane_mode = LaneMode::kCombined;
    } else {
      std::printf("ECHEC : fixture inconnue %s\n", fixture_name.c_str());
      return 2;
    }
    n = (int)pts.size();
    leaf_size = 2;
    oracle = 1;
    std::printf("provenance : --fixture %s (%d points graves, feuilles <= %d, oracle"
                " force, --mode %s)\n", fixture_name.c_str(), n, leaf_size, mode.c_str());
  } else {
    if (coord == 0) coord = mhgp3v::cloud_family_default_coord(family, n);
    pts = mhgp3v::make_family_cloud(family, n, coord, seed);
    if ((int)pts.size() < n) { std::printf("ECHEC : nuage non genere\n"); return 3; }
    // CONTRAT DE CARDINALITE (audit etat courant) : le generateur borne
    // chaque push par n ; le driver exige l'egalite avant l'arbre.
    if ((int)pts.size() != n) {
      std::printf("ECHEC : contrat de cardinalite viole — %zu points rendus pour %d"
                  " demandes\n", pts.size(), n);
      return 1;
    }
    std::printf("provenance : --points %d --coord %d --seed %lld --family %s"
                " --leaf-size %d --mode %s\n", n, coord, seed,
                mhgp3v::cloud_family_name(family), leaf_size, mode.c_str());
  }
  if (oracle == 1 && n > 32) {
    std::printf("ECHEC : l'oracle exhaustif exige n <= 32\n");
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

  Tree tree;
  tree.build(pts, leaf_size);
  const i128 all_pairs = (i128)n * (n - 1) / 2;

  struct LaneRun {
    int arity;
    int threshold;
    LaneReceipt receipt;
    std::vector<PairFate> fate;
    std::vector<PruneCertificate> certificates;
    double seconds = 0.0;
  };
  std::vector<LaneRun> runs;
  if (lane_q3) runs.push_back({3, 9, {}, {}, {}, 0.0});
  if (lane_q4) runs.push_back({4, 8, {}, {}, {}, 0.0});

  for (LaneRun& run : runs) {
    // LA BASELINE DU DIFFERENTIEL, D'ABORD ET SANS INJECTIONS : ses erreurs
    // sont les siennes (jamais un faux code 4 — l'audit q2 exige un juge qui
    // echoue independamment des injections du sujet) et son sort doit etre
    // TOTALEMENT assigne.
    LaneReceipt baseline_receipt{};
    std::vector<PairFate> baseline_fate;
    if (differential == 1) {
      baseline_fate.assign((std::size_t)((i64)n * (n - 1) / 2), PairFate::kUnassigned);
      LaneProbe reference;
      reference.tree = &tree;
      reference.arity = run.arity;
      reference.threshold = run.threshold;
      reference.mode = lane_mode;
      reference.baseline = true;
      reference.max_states = max_states;
      reference.fate = &baseline_fate;
      reference.fate_points = n;
      reference.process(0, 0, 0, {});
      if (reference.budget_exceeded) {
        std::printf("ECHEC : budget d'etats depasse dans la baseline\n");
        return 3;
      }
      if (reference.fate_violated) {
        std::printf("ECHEC de la baseline : sort double dans la reference\n");
        return 1;
      }
      for (std::size_t k = 0; k < baseline_fate.size(); ++k)
        if (baseline_fate[k] == PairFate::kUnassigned) {
          std::printf("ECHEC de la baseline : un sort de reference manque\n");
          return 1;
        }
      baseline_receipt = reference.receipt;
    }
    LaneProbe probe;
    probe.tree = &tree;
    probe.arity = run.arity;
    probe.threshold = run.threshold;
    probe.mode = lane_mode;
    probe.injections = injections;
    probe.max_states = max_states;
    if (oracle == 1 || differential == 1) {
      run.fate.assign((std::size_t)((i64)n * (n - 1) / 2), PairFate::kUnassigned);
      probe.fate = &run.fate;
      probe.fate_points = n;
      if (oracle == 1) probe.certificates = &run.certificates;
    }
    const auto t0 = std::chrono::steady_clock::now();
    probe.process(0, 0, 0, {});
    const auto t1 = std::chrono::steady_clock::now();
    run.seconds = std::chrono::duration<double>(t1 - t0).count();
    if (probe.budget_exceeded) {
      std::printf("ECHEC : budget d'etats depasse (%lld) — la sonde refuse, elle ne"
                  " tronque pas\n", max_states);
      return 3;
    }
    // MUTANT last-block-omitted : la derniere paire residuelle est perdue.
    if (injections.last_block_omitted && probe.receipt.residual_pairs > 0)
      --probe.receipt.residual_pairs;
    run.receipt = probe.receipt;
    if (probe.fate_violated)
      return fail("le ledger de fate", "une paire a recu deux sorts — multiplicite violee");
    const i128 covered = (i128)run.receipt.pruned_pairs + (i128)run.receipt.residual_pairs;
    if (covered != all_pairs)
      return fail("l'identite du ledger", "prunees + residuelles != C(n,2)");
    if (oracle == 1 || differential == 1)
      for (std::size_t k = 0; k < run.fate.size(); ++k)
        if (run.fate[k] == PairFate::kUnassigned)
          return fail("le ledger de fate", "une paire sans sort — partition violee");
    // LES PLANCHERS (audit, coeur point 5) : chaque lane doit atteindre
    // chaque plancher actif — une campagne vacue refuse a code 3, elle ne
    // verdit pas.
    const auto floor_violated = [&](const char* what, i64 got, i64 want) {
      std::printf("ECHEC : plancher viole — %s=%lld < %lld (lane q%d)\n", what, got,
                  want, run.arity);
      return 3;
    };
    if (min_block_prunes > 0 && run.receipt.block_prunes < min_block_prunes)
      return floor_violated("blocs-prunes", run.receipt.block_prunes, min_block_prunes);
    if (min_corner_accepts > 0 && run.receipt.corner_accepts < min_corner_accepts)
      return floor_violated("coins-acceptes", run.receipt.corner_accepts,
                            min_corner_accepts);
    if (min_terminal_prunes > 0 && run.receipt.terminal_prunes < min_terminal_prunes)
      return floor_violated("prunes-terminales", run.receipt.terminal_prunes,
                            min_terminal_prunes);
    if (min_depth_dead > 0 && run.receipt.depth_dead < min_depth_dead)
      return floor_violated("tuees-delta", run.receipt.depth_dead, min_depth_dead);
    if (min_depth_pairs > 0 && run.receipt.depth_pairs < min_depth_pairs)
      return floor_violated("paires-sweep", run.receipt.depth_pairs, min_depth_pairs);
    // LE DIFFERENTIEL BI-MODE : egalite de TOUS les sorts et de toutes les
    // masses de decision contre la baseline — une inclusion unilaterale
    // survivrait a une perte de prune, l'egalite non.
    if (differential == 1) {
      if (baseline_receipt.states != run.receipt.states ||
          baseline_receipt.block_prunes != run.receipt.block_prunes ||
          baseline_receipt.pruned_pairs != run.receipt.pruned_pairs ||
          baseline_receipt.residual_pairs != run.receipt.residual_pairs ||
          baseline_receipt.terminal_pairs != run.receipt.terminal_pairs)
        return fail("le differentiel bi-mode",
                    "les masses du mode optimise different de la baseline recue");
      for (std::size_t k = 0; k < run.fate.size(); ++k)
        if (run.fate[k] != baseline_fate[k])
          return fail("le differentiel bi-mode",
                      "un sort de paire differe entre baseline et optimise");
      std::printf("differentiel q%d : sorts et masses IDENTIQUES — baseline"
                  " visites=%lld tests=%lld ; optimise visites=%lld tests=%lld"
                  " (L4=%lld noeuds/%lld points, herites=%lld, sorties=%lld,"
                  " plein+1=%lld)\n", run.arity, baseline_receipt.node_visits,
                  baseline_receipt.point_tests, run.receipt.node_visits,
                  run.receipt.point_tests, run.receipt.l4_skipped_nodes,
                  run.receipt.l4_skipped_points, run.receipt.inherited_credits,
                  run.receipt.early_exits, run.receipt.full_plus_one);
      if (min_full_plus_one > 0 && run.receipt.full_plus_one < min_full_plus_one)
        return fail("le plancher full-plus-one",
                    "le cas (seuil-1)-herites-plus-un-nouveau n'apparait pas assez"
                    " pour armer son mutant");
    }
  }

  // L'ORACLE : rejeu des certificats puis comparaison exhaustive des ancres.
  if (oracle == 1) {
    for (LaneRun& run : runs) {
      // 1. REJEU : chaque temoin de chaque certificat, sur les coordonnees
      // brutes, contre CHAQUE paire couverte — prédicat nominal (jamais le
      // mutant : le juge n'herite pas des injections du sujet).
      for (const PruneCertificate& certificate : run.certificates) {
        if (certificate.depth_kind) {
          // REJEU D'UN PRUNE DE PROFONDEUR par le JUGE INDEPENDANT : delta
          // recalcule de zero depuis les coordonnees brutes — autre base,
          // autre collecte, minimisation directe du compte ferme, jamais la
          // fonction du sujet ni ses injections (audit, profondeur point 2).
          const i64 replay_delta = reference_closed_depth(
              pts, tree.order[(std::size_t)certificate.pair_a],
              tree.order[(std::size_t)certificate.pair_b]);
          if (replay_delta != certificate.claimed_delta || replay_delta < run.threshold)
            return fail("le rejeu des certificats de profondeur",
                        "le delta du juge independant ne confirme pas le prune");
          continue;
        }
        std::vector<std::pair<int, int>> pairs;
        if (certificate.pair_a >= 0) {
          pairs.push_back({certificate.pair_a, certificate.pair_b});
        } else {
          for (int ta = certificate.a_begin; ta < certificate.a_end; ++ta)
            for (int tb = certificate.b_begin; tb < certificate.b_end; ++tb)
              pairs.push_back({ta, tb});
        }
        // DEDUP DEFENSIVE (audit, coeur point 2) : les temoins sont convertis
        // en PointId STABLES puis dedupliques AVANT le seuil — un producteur
        // qui pousserait deux fois la meme position ne compte qu'une fois.
        std::vector<int> witness_ids;
        witness_ids.reserve(certificate.witness_positions.size());
        for (int position : certificate.witness_positions)
          witness_ids.push_back(tree.order[(std::size_t)position]);
        std::sort(witness_ids.begin(), witness_ids.end());
        witness_ids.erase(std::unique(witness_ids.begin(), witness_ids.end()),
                          witness_ids.end());
        i64 distinct = 0;
        for (int witness_id : witness_ids) {
          bool ok = true;
          for (const auto& pr : pairs) {
            const int id_a = tree.order[(std::size_t)pr.first];
            const int id_b = tree.order[(std::size_t)pr.second];
            if (witness_id == id_a || witness_id == id_b) { ok = false; break; }
            const mhgp::P3& a = pts[(std::size_t)id_a];
            const mhgp::P3& b = pts[(std::size_t)id_b];
            const mhgp::P3& w = pts[(std::size_t)witness_id];
            if (!universal_witness(run.arity, w, a, b, false)) { ok = false; break; }
          }
          if (ok) ++distinct;
        }
        if (distinct < run.threshold)
          return fail("le rejeu des certificats",
                      "un prune n'a pas ses temoins universels rejouables");
      }
      // 1 bis. LE JUGE DE SORT DE LA PROFONDEUR (audit, profondeur point 2) :
      // en mode depth, le sort machine de CHAQUE paire doit coincider avec le
      // seuil applique au delta du juge independant ; en mode combined, une
      // paire residuelle a forcement traverse la profondeur et son delta juge
      // doit rester sous le seuil, tandis qu'un prune du coeur est exempte.
      if (lane_mode != LaneMode::kCore) {
        std::vector<bool> core_pruned((std::size_t)((i64)n * (n - 1) / 2), false);
        const auto pair_index = [&](int id_a, int id_b) {
          if (id_a > id_b) std::swap(id_a, id_b);
          return (i64)id_a * (2 * (i64)n - id_a - 1) / 2 + (id_b - id_a - 1);
        };
        for (const PruneCertificate& certificate : run.certificates) {
          if (certificate.depth_kind) continue;
          if (certificate.pair_a >= 0) {
            core_pruned[(std::size_t)pair_index(
                tree.order[(std::size_t)certificate.pair_a],
                tree.order[(std::size_t)certificate.pair_b])] = true;
          } else {
            for (int ta = certificate.a_begin; ta < certificate.a_end; ++ta)
              for (int tb = certificate.b_begin; tb < certificate.b_end; ++tb)
                core_pruned[(std::size_t)pair_index(tree.order[(std::size_t)ta],
                                                    tree.order[(std::size_t)tb])] = true;
          }
        }
        for (int i = 0; i < n; ++i)
          for (int j = i + 1; j < n; ++j) {
            const i64 index = pair_index(i, j);
            if (lane_mode == LaneMode::kCombined && core_pruned[(std::size_t)index])
              continue;
            const i64 judge_delta = reference_closed_depth(pts, i, j);
            const bool judge_prunes = judge_delta >= run.threshold;
            const bool machine_pruned = run.fate[(std::size_t)index] == PairFate::kPruned;
            if (judge_prunes != machine_pruned)
              return fail("le juge de sort de la profondeur",
                          "un sort machine contredit le delta du juge independant");
          }
      }
      // 2. EXHAUSTIF : toute ancre canonique d'un support propre positif non
      // inerte doit etre residuelle.
      const int t_q = run.threshold;
      std::vector<int> ids(4, 0);
      const auto check_tuple = [&](const int* tuple) -> const char* {
        const mhgp::MiniballResult mb =
            mhgp::miniball_of(pts, tuple, run.arity);
        if (!mb.ok) return nullptr;
        // Support propre positif ssi la miniboule exige tout le tuple ; le
        // MUTANT oracle-accept-nonpositive accepte aussi les autres.
        if (!injections.oracle_accept_nonpositive && mb.n_support != run.arity)
          return nullptr;
        int p = 0;
        for (int x = 0; x < n; ++x)
          if (mhgp::sphere_side(mb.sph, pts[(std::size_t)x]) < 0) ++p;
        if (p >= t_q) return nullptr;   // inerte : l'omission est licite
        const OracleAnchor anchor =
            canonical_anchor(pts, tuple, run.arity, injections.anchor_non_maximal);
        const i64 index =
            (i64)anchor.a * (2 * (i64)n - anchor.a - 1) / 2 + (anchor.b - anchor.a - 1);
        if (run.fate[(std::size_t)index] != PairFate::kResidual)
          return "l'ancre canonique d'un support non inerte n'est pas residuelle";
        return nullptr;
      };
      const char* violation = nullptr;
      if (run.arity == 3) {
        for (int i = 0; i < n && violation == nullptr; ++i)
          for (int j = i + 1; j < n && violation == nullptr; ++j)
            for (int k = j + 1; k < n && violation == nullptr; ++k) {
              ids[0] = i; ids[1] = j; ids[2] = k;
              violation = check_tuple(ids.data());
            }
      } else {
        for (int i = 0; i < n && violation == nullptr; ++i)
          for (int j = i + 1; j < n && violation == nullptr; ++j)
            for (int k = j + 1; k < n && violation == nullptr; ++k)
              for (int l = k + 1; l < n && violation == nullptr; ++l) {
                ids[0] = i; ids[1] = j; ids[2] = k; ids[3] = l;
                violation = check_tuple(ids.data());
              }
      }
      if (violation != nullptr) return fail("l'oracle exhaustif des ancres", violation);
    }
  }

  // LES ASSERTIONS DE FIXTURE.
  if (is_fixture) {
    const auto fate_of = [&](const LaneRun& run, int i, int j) {
      if (i > j) std::swap(i, j);
      const i64 index = (i64)i * (2 * (i64)n - i - 1) / 2 + (j - i - 1);
      return run.fate[(std::size_t)index];
    };
    const auto lane_run = [&](int arity) -> const LaneRun* {
      for (const LaneRun& run : runs)
        if (run.arity == arity) return &run;
      return nullptr;
    };
    if (fixture_name == "thin-acute") {
      const LaneRun* q3 = lane_run(3);
      if (q3 != nullptr) {
        if (fate_of(*q3, 0, 1) != PairFate::kPruned)
          return fail("la fixture thin-acute", "l'arete courte AB n'est pas prunee en q3");
        if (fate_of(*q3, 0, 2) != PairFate::kResidual ||
            fate_of(*q3, 1, 2) != PairFate::kResidual)
          return fail("la fixture thin-acute",
                      "une arete maximale du support non inerte a ete prunee");
      }
    } else if (fixture_name == "contact-q3" || fixture_name == "contact-q4") {
      // ASSERTIONS DIRECTES du prédicat aux contacts exacts de la note :
      // strict refuse, large accepterait — c'est la fixture qui tue
      // gt-to-ge, independamment de la granularite des blocs.
      const bool is_q3 = fixture_name == "contact-q3";
      const int arity = is_q3 ? 3 : 4;
      const int w_id = is_q3 ? 3 : 4;
      const int support = is_q3 ? 3 : 4;
      int boundary_pairs = 0;
      for (int i = 0; i < support; ++i)
        for (int j = i + 1; j < support; ++j) {
          const bool strict =
              universal_witness(arity, pts[(std::size_t)w_id], pts[(std::size_t)i],
                                pts[(std::size_t)j], false);
          const bool large =
              universal_witness(arity, pts[(std::size_t)w_id], pts[(std::size_t)i],
                                pts[(std::size_t)j], true);
          if (strict)
            return fail("la fixture de contact",
                        "un point de coquille extreme a ete classe interieur strict");
          if (large) ++boundary_pairs;   // l'egalite exacte n'existe qu'a la
                                         // paire (a,b) — les autres ont g < 0
        }
      if (boundary_pairs < 1)
        return fail("la fixture de contact",
                    "aucune paire n'atteint la frontiere exacte du prédicat — la"
                    " fixture ne mord plus");
      if (injections.gt_to_ge)
        return fail("la fixture de contact",
                    "le mutant large a compte le contact comme interieur");
    } else if (fixture_name == "nine-axial") {
      const LaneRun* q3 = lane_run(3);
      const LaneRun* q4 = lane_run(4);
      if (q3 != nullptr && fate_of(*q3, 0, 1) != PairFate::kPruned)
        return fail("la fixture nine-axial", "9 temoins axiaux ne prunent pas q3");
      if (q4 != nullptr && fate_of(*q4, 0, 1) != PairFate::kPruned)
        return fail("la fixture nine-axial", "9 temoins axiaux ne prunent pas q4");
      // EN MODE depth, la fixture doit former un CERTIFICAT DE PROFONDEUR
      // (audit, profondeur point 1) : delta(0,1)=9 par la banque always,
      // toutes les paires balayees, aucun prune de bloc.
      if (lane_mode == LaneMode::kDepth) {
        for (const LaneRun& run : runs) {
          if (run.receipt.depth_dead < 1)
            return fail("la fixture nine-axial",
                        "aucune paire tuee par delta — la profondeur est desarmee");
          if (run.receipt.depth_pairs != (i64)all_pairs)
            return fail("la fixture nine-axial",
                        "le sweep n'a pas visite toutes les paires en mode depth");
          if (run.receipt.block_prunes != 0)
            return fail("la fixture nine-axial",
                        "un prune de bloc est interdit en mode depth");
        }
      }
    } else if (fixture_name == "eight-axial") {
      const LaneRun* q3 = lane_run(3);
      const LaneRun* q4 = lane_run(4);
      if (q3 != nullptr && fate_of(*q3, 0, 1) != PairFate::kResidual)
        return fail("la fixture eight-axial",
                    "8 temoins pruneraient q3 — le neuvieme est obligatoire");
      if (q4 != nullptr && fate_of(*q4, 0, 1) != PairFate::kPruned)
        return fail("la fixture eight-axial", "8 temoins ne prunent pas q4");
      if (lane_mode == LaneMode::kDepth) {
        for (const LaneRun& run : runs) {
          if (run.arity == 4 && run.receipt.depth_dead < 1)
            return fail("la fixture eight-axial",
                        "aucune paire q4 tuee par delta — la profondeur est desarmee");
          if (run.receipt.depth_pairs != (i64)all_pairs)
            return fail("la fixture eight-axial",
                        "le sweep n'a pas visite toutes les paires en mode depth");
          if (run.receipt.block_prunes != 0)
            return fail("la fixture eight-axial",
                        "un prune de bloc est interdit en mode depth");
        }
      }
    } else if (fixture_name == "seven-axial") {
      const LaneRun* q4 = lane_run(4);
      if (q4 != nullptr && fate_of(*q4, 0, 1) != PairFate::kResidual)
        return fail("la fixture seven-axial",
                    "7 temoins pruneraient q4 — le huitieme est obligatoire");
    } else if (fixture_name == "q4-tetra") {
      const LaneRun* q4 = lane_run(4);
      if (q4 != nullptr && fate_of(*q4, 0, 1) != PairFate::kResidual)
        return fail("la fixture q4-tetra",
                    "l'ancre canonique du tetraedre non inerte n'est pas residuelle");
    } else if (fixture_name == "depth-rays") {
      // ASSERTION DIRECTE du sweep : delta = 3 exactement (always = 1,
      // m = 5, max ouvert = 3), et permutation-invariance des temoins.
      std::vector<int> witness_ids = {2, 3, 4, 5, 6, 7};
      i64 always = 0;
      const i64 delta = exact_closed_depth(pts, pts[0], pts[1], witness_ids, &always,
                                           injections);
      if (delta != 3)
        return fail("la fixture depth-rays",
                    "delta != 3 — rayons confondus, antipodes ou banque always faux");
      if (always != (injections.depth_always_dropped ? 0 : 1))
        return fail("la fixture depth-rays", "la banque always ne compte pas l'axial");
      std::reverse(witness_ids.begin(), witness_ids.end());
      const i64 delta_reversed = exact_closed_depth(pts, pts[0], pts[1], witness_ids,
                                                    nullptr, injections);
      if (delta_reversed != delta)
        return fail("la fixture depth-rays", "delta depend de l'ordre des temoins");
    } else if (fixture_name == "scope-depth") {
      // Les dix temoins se projettent sur UN rayon : delta = 0, la paire
      // survit a la profondeur — et le sort machine de (a,b) en q3 est
      // RESIDUEL (aucun des deux certificats ne la tue).
      std::vector<int> witness_ids;
      for (int w = 3; w < n; ++w) witness_ids.push_back(w);
      const i64 delta = exact_closed_depth(pts, pts[0], pts[1], witness_ids, nullptr,
                                           injections);
      if (delta != 0)
        return fail("la fixture scope-depth",
                    "delta != 0 — dix temoins d'un seul cote devraient s'annuler");
      const LaneRun* q3 = lane_run(3);
      if (q3 != nullptr && fate_of(*q3, 0, 1) != PairFate::kResidual)
        return fail("la fixture scope-depth",
                    "ab a ete prunee alors que coeur et profondeur la gardent");
    } else if (fixture_name == "q4-colocated") {
      // AUCUN certificat ne peut exister sur des PointId colocalises :
      // zero bloc, zero prune, toutes les paires residuelles, par lane.
      for (const LaneRun& run : runs) {
        if (run.receipt.block_prunes != 0 || run.receipt.spindle_block_hits != 0)
          return fail("la fixture q4-colocated",
                      "un certificat de bloc a ete fabrique sur D^2=0");
        if (run.receipt.pruned_pairs != 0 ||
            run.receipt.residual_pairs != (i64)all_pairs)
          return fail("la fixture q4-colocated",
                      "une paire colocalisee a ete prunee malgre g=0");
      }
    } else if (fixture_name == "q4-rational-equality") {
      // L'honnetete de la fixture d'abord : l'egalite 15*U^2 = 4*D^2 doit
      // etre EXACTE sur les coordonnees gravees, avec D^2 > 0.
      const i64 d2 = dist2_points(pts[0], pts[1]);
      i64 u2 = 0;
      {
        const i64 ux = 2 * (i64)pts[2].x - pts[0].x - pts[1].x;
        const i64 uy = 2 * (i64)pts[2].y - pts[0].y - pts[1].y;
        const i64 uz = 2 * (i64)pts[2].z - pts[0].z - pts[1].z;
        u2 = ux * ux + uy * uy + uz * uz;
      }
      if (d2 <= 0 || 15 * u2 != 4 * d2)
        return fail("la fixture q4-rational-equality",
                    "l'egalite rationnelle exacte ne mord plus — fixture derivee");
      // Le prédicat exact accepte le temoin, et le pre-prune de bloc large
      // accepte l'egalite (garde D^2 > 0 satisfaite). Le mutant strict la
      // refuserait : c'est la porte du sous-test rationnel.
      if (!universal_witness(4, pts[2], pts[0], pts[1], false))
        return fail("la fixture q4-rational-equality",
                    "le prédicat exact refuse le temoin de l'egalite sure");
      Node box_a{}, box_b{}, box_w{};
      for (int d = 0; d < 3; ++d) {
        const i64 ca = d == 0 ? (i64)pts[0].x : (d == 1 ? (i64)pts[0].y : (i64)pts[0].z);
        const i64 cb = d == 0 ? (i64)pts[1].x : (d == 1 ? (i64)pts[1].y : (i64)pts[1].z);
        const i64 cw = d == 0 ? (i64)pts[2].x : (d == 1 ? (i64)pts[2].y : (i64)pts[2].z);
        box_a.lo[d] = box_a.hi[d] = ca;
        box_b.lo[d] = box_b.hi[d] = cb;
        box_w.lo[d] = box_w.hi[d] = cw;
      }
      if (!spindle_block_universal(4, box_w, box_a, box_b, injections))
        return fail("la fixture q4-rational-equality",
                    "l'egalite large sure du pre-prune q4 a ete refusee");
    } else if (fixture_name == "u16-extremes-anchor") {
      const LaneRun* q3 = lane_run(3);
      const LaneRun* q4 = lane_run(4);
      if (q3 != nullptr && fate_of(*q3, 0, 1) != PairFate::kPruned)
        return fail("la fixture u16-extremes-anchor",
                    "la paire diagonale extreme n'est pas prunee en q3");
      if (q4 != nullptr && fate_of(*q4, 0, 1) != PairFate::kPruned)
        return fail("la fixture u16-extremes-anchor",
                    "la paire diagonale extreme n'est pas prunee en q4");
      if (lane_mode == LaneMode::kDepth)
        for (const LaneRun& run : runs)
          if (run.receipt.depth_dead < 1)
            return fail("la fixture u16-extremes-anchor",
                        "aucune paire extreme tuee par delta en mode depth");
    } else if (fixture_name == "u16-extremes-skew") {
      // Honnetete d'abord : chaque temoin est STRICTEMENT diametral et les
      // dix-huit projections sont non nulles (aucune banque always).
      for (int w = 2; w < n; ++w) {
        const i64 va[3] = {(i64)pts[(std::size_t)w].x - pts[0].x,
                           (i64)pts[(std::size_t)w].y - pts[0].y,
                           (i64)pts[(std::size_t)w].z - pts[0].z};
        const i64 vb[3] = {(i64)pts[(std::size_t)w].x - pts[1].x,
                           (i64)pts[(std::size_t)w].y - pts[1].y,
                           (i64)pts[(std::size_t)w].z - pts[1].z};
        if (va[0] * vb[0] + va[1] * vb[1] + va[2] * vb[2] >= 0)
          return fail("la fixture u16-extremes-skew",
                      "un temoin grave n'est plus strictement diametral — fixture"
                      " derivee");
      }
      std::vector<int> witness_ids;
      for (int w = 2; w < n; ++w) witness_ids.push_back(w);
      i64 always = 0;
      const i64 delta = exact_closed_depth(pts, pts[0], pts[1], witness_ids, &always,
                                           injections);
      if (always != 0)
        return fail("la fixture u16-extremes-skew",
                    "une projection nulle est apparue — la fixture doit exercer le"
                    " chemin NON colineaire");
      if (delta != 9)
        return fail("la fixture u16-extremes-skew",
                    "delta != 9 — neuf paires antipodales exactes aux extremes u16"
                    " doivent rendre exactement 9");
      std::reverse(witness_ids.begin(), witness_ids.end());
      const i64 delta_reversed = exact_closed_depth(pts, pts[0], pts[1], witness_ids,
                                                    nullptr, injections);
      if (delta_reversed != delta)
        return fail("la fixture u16-extremes-skew", "delta depend de l'ordre des temoins");
      const LaneRun* q3 = lane_run(3);
      const LaneRun* q4 = lane_run(4);
      if (q3 != nullptr && fate_of(*q3, 0, 1) != PairFate::kPruned)
        return fail("la fixture u16-extremes-skew",
                    "la paire diagonale n'est pas tuee par delta=9 en q3");
      if (q4 != nullptr && fate_of(*q4, 0, 1) != PairFate::kPruned)
        return fail("la fixture u16-extremes-skew",
                    "la paire diagonale n'est pas tuee par delta=9 en q4");
      for (const LaneRun& run : runs)
        if (run.receipt.depth_dead < 1)
          return fail("la fixture u16-extremes-skew",
                      "aucune paire tuee par delta en mode depth");
    } else if (fixture_name == "diametral-contact") {
      // Honnetete : chaque point grave est EXACTEMENT sur la sphere
      // diametrale de (0,1) — dot nul, jamais temoin strict.
      for (int w = 2; w < n; ++w) {
        const i64 va[3] = {(i64)pts[(std::size_t)w].x - pts[0].x,
                           (i64)pts[(std::size_t)w].y - pts[0].y,
                           (i64)pts[(std::size_t)w].z - pts[0].z};
        const i64 vb[3] = {(i64)pts[(std::size_t)w].x - pts[1].x,
                           (i64)pts[(std::size_t)w].y - pts[1].y,
                           (i64)pts[(std::size_t)w].z - pts[1].z};
        if (va[0] * vb[0] + va[1] * vb[1] + va[2] * vb[2] != 0)
          return fail("la fixture diametral-contact",
                      "un point grave n'est plus un contact exact — fixture derivee");
      }
      const LaneRun* q3 = lane_run(3);
      if (q3 != nullptr && fate_of(*q3, 0, 1) != PairFate::kResidual)
        return fail("la fixture diametral-contact",
                    "vingt contacts de coquille ont fabrique un faux prune de"
                    " profondeur");
    } else if (fixture_name == "confounded-arc") {
      // ASSERTION DIRECTE du sweep : les onze rayons confondus appartiennent
      // a l'arc ancre sur +x, delta = 0 exactement.
      std::vector<int> witness_ids;
      for (int w = 2; w < n; ++w) witness_ids.push_back(w);
      const i64 delta = exact_closed_depth(pts, pts[0], pts[1], witness_ids, nullptr,
                                           injections);
      if (delta != 0)
        return fail("la fixture confounded-arc",
                    "delta != 0 — les rayons confondus sont sortis de l'arc");
      const LaneRun* q3 = lane_run(3);
      if (q3 != nullptr && fate_of(*q3, 0, 1) != PairFate::kResidual)
        return fail("la fixture confounded-arc",
                    "la paire est morte d'un faux prune de profondeur");
    }
  }

  if (injections.any()) {
    std::printf("MUTANT SURVIVANT : aucune porte n'a mordu\n");
    return 0;
  }

  const i64 tree_bytes = (i64)(tree.nodes.size() * sizeof(Node) + tree.order.size() * sizeof(int));
  std::printf("arbre      : %zu noeuds, feuilles <= %d, %lld octets\n", tree.nodes.size(),
              leaf_size, tree_bytes);
  for (const LaneRun& run : runs) {
    const double share =
        100.0 * (double)run.receipt.residual_pairs / (double)(i64)all_pairs;
    std::printf("lane q%d    : etats=%lld blocs-prunes=%lld — prunees=%lld residuelles=%lld"
                " (%.2f %% de C(n,2)=%lld) — terminales=%lld — ledger FERME\n",
                run.arity, run.receipt.states, run.receipt.block_prunes,
                run.receipt.pruned_pairs, run.receipt.residual_pairs, share,
                (i64)all_pairs, run.receipt.terminal_pairs);
    std::printf("  coeur    : visites=%lld noeuds-coins=%lld (evals=%lld"
                " acceptes=%lld) points=%lld inscrites=%lld prunes-terminales=%lld"
                " pile-max=%lld profondeur=%lld — %.3f s de phase locale (1 thread, pas"
                " un warm_e2e)\n", run.receipt.node_visits, run.receipt.corner_tests,
                run.receipt.corner_evals, run.receipt.corner_accepts,
                run.receipt.point_tests, run.receipt.spindle_block_hits,
                run.receipt.terminal_prunes, run.receipt.stack_high_water,
                run.receipt.depth_max, run.seconds);
    std::printf("  recherche: L4-retraits=%lld noeuds (%lld points, multiplicite),"
                " herites=%lld (multiplicite), sorties precoces=%lld, plein+1=%lld\n",
                run.receipt.l4_skipped_nodes, run.receipt.l4_skipped_points,
                run.receipt.inherited_credits, run.receipt.early_exits,
                run.receipt.full_plus_one);
    if (run.receipt.depth_pairs > 0) {
      std::printf("  sweep    : paires=%lld tuees-delta=%lld temoins=%lld (max m=%lld)"
                  " always=%lld groupes=%lld pas=%lld tri=%lld comparaisons\n",
                  run.receipt.depth_pairs,
                  run.receipt.depth_dead, run.receipt.depth_witnesses,
                  run.receipt.depth_biggest_m, run.receipt.depth_always,
                  run.receipt.depth_angle_groups, run.receipt.depth_sweep_steps,
                  run.receipt.depth_sort_compares);
      std::printf("  collecte : visites=%lld tests=%lld scratch-max=%lld elements,"
                  " %lld octets tampons compris (compteurs SEPARES du coeur — audit)\n",
                  run.receipt.depth_collect_visits,
                  run.receipt.depth_collect_point_tests,
                  run.receipt.depth_scratch_capacity, run.receipt.depth_scratch_bytes);
    }
  }
  if (oracle == 1)
    std::printf("oracle     : certificats rejoues et ancres exhaustives comparees —"
                " aucun desaccord\n");
  if (is_fixture)
    std::printf("OK : fixture %s recue\n", fixture_name.c_str());
  else
    std::printf("OK : falsificateur d'ancres count-only — les residuelles sont la"
                " sortie, aucune admission n'est prononcee\n");
  return 0;
}
