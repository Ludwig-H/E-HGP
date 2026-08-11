// MorseHGP3D v3 — LE FALSIFICATEUR D'ANCRES q3/q4 PAR COEUR UNIVERSEL DE
// JUNG (NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811, porte
// d'implementation en cinq points ; REPONSE_AUDIT_ANCRES : la profondeur
// fermee est le filtre TERMINAL de la tranche suivante — ici `--mode core`
// seulement, les autres modes refusent explicitement).
//
// LE THEOREME (note auditeur) : pour une paire (a,b) qui est une arete de
// longueur MAXIMALE d'un support propre positif q3/q4, le centre de la
// sphere appartient au disque mediateur de Jung `||t||^2 <= D^2/12` (q3) ou
// `D^2/8` (q4). Un temoin w avec U = 2w-a-b, d = b-a, g = D^2 - ||U||^2,
// Q = ||d x U||^2 est STRICTEMENT interieur a TOUTES les spheres admissibles
// exactement quand :
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
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "mhgp/miniball.hpp"
#include "prototype/cloud_families.hpp"

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
  bool oracle_accept_nonpositive = false;  // ORACLE : support non propre accepte
  bool any() const {
    return gt_to_ge || threshold_minus_one || witness_duplicated || last_block_omitted ||
           duplicate_compensated || anchor_non_maximal || oracle_accept_nonpositive;
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

inline bool spindle_block_universal(int arity, const Node& w, const Node& a, const Node& b,
                                    bool ge_mutant) {
  const i64 max_u2 = max_u2_over_boxes(w, a, b);
  const i64 min_d2 = min_d2_over_boxes(a, b);
  if (arity == 3) return ge_mutant ? 3 * max_u2 <= min_d2 : 3 * max_u2 < min_d2;
  return 15 * max_u2 <= 4 * min_d2;   // large licite, mutant sans objet ici
}

// LE TEST DES HUIT COINS contre une paire EXACTE : le spindle est convexe
// (intersection de boules ouvertes) — huit coins stricts impliquent la
// boite entiere stricte, donc tous les points du noeud universels.
inline bool node_universal_for_pair(int arity, const Node& node, const mhgp::P3& a,
                                    const mhgp::P3& b, bool ge_mutant) {
  for (int corner = 0; corner < 8; ++corner) {
    const mhgp::P3 w{(mhgp::i32)(corner & 1 ? node.hi[0] : node.lo[0]),
                     (mhgp::i32)(corner & 2 ? node.hi[1] : node.lo[1]),
                     (mhgp::i32)(corner & 4 ? node.hi[2] : node.lo[2])};
    if (!universal_witness(arity, w, a, b, ge_mutant)) return false;
  }
  return true;
}

enum class PairFate : std::uint8_t { kUnassigned = 0, kPruned = 1, kResidual = 2 };

struct LaneReceipt {
  i64 states = 0;
  i64 block_prunes = 0;         // blocs entiers prunes
  i64 pruned_pairs = 0;
  i64 residual_pairs = 0;       // LA sortie : les candidates ancres
  i64 terminal_pairs = 0;       // paires resolues une a une
  i64 node_visits = 0;
  i64 corner_tests = 0;
  i64 point_tests = 0;
  i64 spindle_block_hits = 0;
  i64 depth_max = 0;
  i64 stack_high_water = 0;
};

// Le certificat d'un prune, pour le rejeu de l'oracle : plages d'extremites
// et positions des temoins credites (>= seuil). `pair_a/pair_b < 0` : bloc ;
// sinon paire terminale (positions exactes).
struct PruneCertificate {
  int a_begin = 0, a_end = 0, b_begin = 0, b_end = 0;
  int pair_a = -1, pair_b = -1;
  std::vector<int> witness_positions;
};

struct LaneProbe {
  const Tree* tree = nullptr;
  int arity = 3;
  int threshold = 9;
  ProbeInjections injections;
  LaneReceipt receipt;
  i64 max_states = 0;
  bool budget_exceeded = false;
  std::vector<PairFate>* fate = nullptr;
  int fate_points = 0;
  bool fate_violated = false;
  std::vector<PruneCertificate>* certificates = nullptr;   // mode oracle
  std::vector<int> stack_;
  std::vector<int> harvest_;
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
  // certificat de rejeu.
  i64 count_universal_block(const Node& a, const Node& b) {
    harvest_.clear();
    i64 total = 0;
    const int credit = injections.witness_duplicated ? 2 : 1;   // MUTANT : double
    stack_.clear();
    stack_.push_back(0);
    while (!stack_.empty()) {
      receipt.stack_high_water = std::max(receipt.stack_high_water, (i64)stack_.size());
      const int node_index = stack_.back();
      stack_.pop_back();
      const Node& node = tree->nodes[(std::size_t)node_index];
      ++receipt.node_visits;
      if (ranges_disjoint(node, a, b)) {
        if (spindle_block_universal(arity, node, a, b, injections.gt_to_ge)) {
          ++receipt.spindle_block_hits;
          total += credit * (i64)(node.end - node.begin);
          for (int t = node.begin; t < node.end; ++t) harvest_.push_back(t);
          if (total >= effective_threshold()) return total;
          continue;
        }
        if (node.left < 0) {
          for (int t = node.begin; t < node.end; ++t) {
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
            if (spindle_block_universal(arity, point_box, a, b, injections.gt_to_ge)) {
              total += credit;
              harvest_.push_back(t);
            }
          }
          if (total >= effective_threshold()) return total;
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
          const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
          ++receipt.point_tests;
          Node point_box{};
          for (int d = 0; d < 3; ++d) {
            const i64 c = d == 0 ? (i64)w.x : (d == 1 ? (i64)w.y : (i64)w.z);
            point_box.lo[d] = c;
            point_box.hi[d] = c;
          }
          if (spindle_block_universal(arity, point_box, a, b, injections.gt_to_ge)) {
            total += credit;
            if (certificates != nullptr && harvest_.size() < 32) harvest_.push_back(t);
          }
        }
        if (total >= effective_threshold()) return total;
        continue;
      }
      stack_.push_back(node.right);
      stack_.push_back(node.left);
    }
    return total;
  }

  // Compte des temoins universels pour une paire EXACTE (positions pos_a,
  // pos_b) : coins convexes sur les noeuds disjoints des deux positions,
  // prédicat exact par point ailleurs.
  i64 count_universal_pair(int pos_a, int pos_b) {
    harvest_.clear();
    const mhgp::P3& a = (*tree->points)[(std::size_t)tree->order[(std::size_t)pos_a]];
    const mhgp::P3& b = (*tree->points)[(std::size_t)tree->order[(std::size_t)pos_b]];
    i64 total = 0;
    const int credit = injections.witness_duplicated ? 2 : 1;
    stack_.clear();
    stack_.push_back(0);
    while (!stack_.empty()) {
      receipt.stack_high_water = std::max(receipt.stack_high_water, (i64)stack_.size());
      const int node_index = stack_.back();
      stack_.pop_back();
      const Node& node = tree->nodes[(std::size_t)node_index];
      ++receipt.node_visits;
      const bool holds_endpoint = (pos_a >= node.begin && pos_a < node.end) ||
                                  (pos_b >= node.begin && pos_b < node.end);
      if (!holds_endpoint) {
        ++receipt.corner_tests;
        if (node_universal_for_pair(arity, node, a, b, injections.gt_to_ge)) {
          total += credit * (i64)(node.end - node.begin);
          for (int t = node.begin; t < node.end; ++t) harvest_.push_back(t);
          if (total >= effective_threshold()) return total;
          continue;
        }
      }
      if (node.left < 0) {
        for (int t = node.begin; t < node.end; ++t) {
          if (t == pos_a || t == pos_b) continue;
          const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
          ++receipt.point_tests;
          if (universal_witness(arity, w, a, b, injections.gt_to_ge)) {
            total += credit;
            if (certificates != nullptr && harvest_.size() < 32) harvest_.push_back(t);
          }
        }
        if (total >= effective_threshold()) return total;
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

  // Resolution TERMINALE d'une paire : prunee ou residuelle, sort unique.
  void resolve_pair(int pos_a, int pos_b) {
    ++receipt.terminal_pairs;
    const i64 found = count_universal_pair(pos_a, pos_b);
    const int ia = tree->order[(std::size_t)pos_a];
    const int ib = tree->order[(std::size_t)pos_b];
    if (found >= effective_threshold()) {
      ++receipt.pruned_pairs;
      assign_pair(ia, ib, PairFate::kPruned);
      record_pair_certificate(pos_a, pos_b);
      return;
    }
    ++receipt.residual_pairs;
    last_residual_pair = (i64)pos_a * tree->order.size() + pos_b;
    assign_pair(ia, ib, PairFate::kResidual);
  }

  void process(int ia, int ib, i64 depth) {
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
          for (int tb = ta + 1; tb < a.end; ++tb) resolve_pair(ta, tb);
        return;
      }
      process(a.left, a.left, depth + 1);
      process(a.left, a.right, depth + 1);
      process(a.right, a.right, depth + 1);
      return;
    }
    const i64 found = count_universal_block(a, b);
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
    const bool a_leaf = a.left < 0, b_leaf = b.left < 0;
    if (a_leaf && b_leaf) {
      for (int ta = a.begin; ta < a.end; ++ta)
        for (int tb = b.begin; tb < b.end; ++tb) resolve_pair(ta, tb);
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
        process(child_left, ib, depth + 1);
        process(child_left, ib, depth + 1);
      } else {
        process(ia, child_left, depth + 1);
        process(ia, child_left, depth + 1);
      }
      return;
    }
    if (split_a) {
      process(child_left, ib, depth + 1);
      process(child_right, ib, depth + 1);
    } else {
      process(ia, child_left, depth + 1);
      process(ia, child_right, depth + 1);
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

}  // namespace

int main(int argc, char** argv) {
  int n = 2400, coord = 0, leaf_size = 8, oracle = 0;
  i64 seed = 20260810, max_states = 200000000;
  int lane_q3 = 1, lane_q4 = 1;
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
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    ++i;
  }
  if (mode != "core") {
    std::printf("ECHEC : --mode %s non implemente dans cette tranche — la profondeur"
                " fermee est le filtre terminal de la tranche suivante (REPONSE_AUDIT"
                "_ANCRES) ; seul `core` est disponible\n", mode.c_str());
    return 2;
  }
  if (n < 4 || n > 100000 || coord < 0 || coord > 65536 || leaf_size < 2 ||
      leaf_size > 256 || max_states < 1 || oracle < 0 || oracle > 1) {
    std::printf("ECHEC : campagne absurde\n");
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
      // 3g^2=4Q et 3||U||^2=D^2 pour chaque paire — strict le refuse,
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
      // Tetraedre regulier r^2=3 (reponse pont) : support q4 propre, p=0,
      // non inerte, six aretes ex aequo (288) — l'ancre canonique (0,1)
      // doit etre residuelle en q4.
      pts = {mhgp::P3{40001, 40001, 40001}, mhgp::P3{40001, 39989, 39989},
             mhgp::P3{39989, 40001, 39989}, mhgp::P3{39989, 39989, 40001}};
    } else {
      std::printf("ECHEC : fixture inconnue %s\n", fixture_name.c_str());
      return 2;
    }
    n = (int)pts.size();
    leaf_size = 2;
    oracle = 1;
    std::printf("provenance : --fixture %s (%d points graves, feuilles <= %d, oracle"
                " force)\n", fixture_name.c_str(), n, leaf_size);
  } else {
    if (coord == 0) coord = mhgp3v::cloud_family_default_coord(family, n);
    pts = mhgp3v::make_family_cloud(family, n, coord, seed);
    if ((int)pts.size() < n) { std::printf("ECHEC : nuage non genere\n"); return 3; }
    // Le generateur peut rendre plus de points que demandes (echo de
    // recouvrement) : le ledger porte sur le nuage REEL.
    const int requested = n;
    n = (int)pts.size();
    std::printf("provenance : --points %d (rendus %d) --coord %d --seed %lld --family %s"
                " --leaf-size %d --mode core\n", requested, n, coord, seed,
                mhgp3v::cloud_family_name(family), leaf_size);
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
    LaneProbe probe;
    probe.tree = &tree;
    probe.arity = run.arity;
    probe.threshold = run.threshold;
    probe.injections = injections;
    probe.max_states = max_states;
    if (oracle == 1) {
      run.fate.assign((std::size_t)((i64)n * (n - 1) / 2), PairFate::kUnassigned);
      probe.fate = &run.fate;
      probe.fate_points = n;
      probe.certificates = &run.certificates;
    }
    const auto t0 = std::chrono::steady_clock::now();
    probe.process(0, 0, 0);
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
    if (oracle == 1)
      for (std::size_t k = 0; k < run.fate.size(); ++k)
        if (run.fate[k] == PairFate::kUnassigned)
          return fail("le ledger de fate", "une paire sans sort — partition violee");
  }

  // L'ORACLE : rejeu des certificats puis comparaison exhaustive des ancres.
  if (oracle == 1) {
    for (LaneRun& run : runs) {
      // 1. REJEU : chaque temoin de chaque certificat, sur les coordonnees
      // brutes, contre CHAQUE paire couverte — prédicat nominal (jamais le
      // mutant : le juge n'herite pas des injections du sujet).
      for (const PruneCertificate& certificate : run.certificates) {
        std::vector<std::pair<int, int>> pairs;
        if (certificate.pair_a >= 0) {
          pairs.push_back({certificate.pair_a, certificate.pair_b});
        } else {
          for (int ta = certificate.a_begin; ta < certificate.a_end; ++ta)
            for (int tb = certificate.b_begin; tb < certificate.b_end; ++tb)
              pairs.push_back({ta, tb});
        }
        i64 distinct = 0;
        for (int position : certificate.witness_positions) {
          bool ok = true;
          for (const auto& pr : pairs) {
            if (position == pr.first || position == pr.second) { ok = false; break; }
            const mhgp::P3& a = pts[(std::size_t)tree.order[(std::size_t)pr.first]];
            const mhgp::P3& b = pts[(std::size_t)tree.order[(std::size_t)pr.second]];
            const mhgp::P3& w = pts[(std::size_t)tree.order[(std::size_t)position]];
            if (!universal_witness(run.arity, w, a, b, false)) { ok = false; break; }
          }
          if (ok) ++distinct;
        }
        if (distinct < run.threshold)
          return fail("le rejeu des certificats",
                      "un prune n'a pas ses temoins universels rejouables");
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
    } else if (fixture_name == "eight-axial") {
      const LaneRun* q3 = lane_run(3);
      const LaneRun* q4 = lane_run(4);
      if (q3 != nullptr && fate_of(*q3, 0, 1) != PairFate::kResidual)
        return fail("la fixture eight-axial",
                    "8 temoins pruneraient q3 — le neuvieme est obligatoire");
      if (q4 != nullptr && fate_of(*q4, 0, 1) != PairFate::kPruned)
        return fail("la fixture eight-axial", "8 temoins ne prunent pas q4");
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
    std::printf("  travail  : visites=%lld coins=%lld points=%lld inscrites=%lld"
                " pile-max=%lld profondeur=%lld — %.3f s de phase locale (1 thread, pas"
                " un warm_e2e)\n", run.receipt.node_visits, run.receipt.corner_tests,
                run.receipt.point_tests, run.receipt.spindle_block_hits,
                run.receipt.stack_high_water, run.receipt.depth_max, run.seconds);
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
