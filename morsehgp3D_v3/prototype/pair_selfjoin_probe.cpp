// MorseHGP3D v3 — LA SONDE SELF-JOIN q2 DES PAIRES (PROPOSITION §6.2,
// AUDIT_Q2_SELFJOIN_8A39C53, portes 1-2 de l'etat courant). L'ancien nom de
// travail « P1a » designait le center-cover retire ; la nomenclature est
// « self-join q2 ».
//
// LA MACHINE DE BLOCS : pour un noeud binaire N de fils L,R, les paires
// internes se decomposent en trois ensembles disjoints — internes a L,
// produit croise L x R, internes a R. Un produit croise est divise sur UN
// SEUL cote (le plus gros, regle deterministe). Chaque paire non ordonnee
// appartient ainsi a un unique etat ; le ledger DOIT fermer :
//
//     P_prune + P_microtile = C(n,2)     (par lane, exactement)
//
// LA LANE q2 : un temoin w est STRICTEMENT interieur a la boule diametrale
// de (x,y) exactement quand (w-x).(w-y) < 0. Sur un bloc (A,B) et un noeud
// temoin W, le SUP de (w-x).(w-y) sur le triple de boites est SEPARABLE par
// axe et ATTEINT sur les 8 combinaisons d'extremes — 24 produits entiers,
// aucun intervalle approche. Un sup strictement negatif compte TOUT W comme
// interieur pour TOUTES les paires du bloc. Dix temoins distincts, pris dans
// des plages de feuilles DISJOINTES de celles des deux extremites (un point
// de A ou B peut etre une extremite de paire), certifient p >= 10 donc
// p+q >= 12 = K+2 : le bloc entier est H0-inerte a la lane q2 (theoreme
// 4.2). Zero signifie shell et ne compte pas ; un sup traversant zero
// conserve le noeud et descend. La recherche part de la racine et s'arrete
// des le seuil atteint ; la mesure des auditeurs a montre que la sortie
// precoce seule ne retire que 0,23-0,39 % des visites (le dixieme temoin
// est acquis TARD), d'ou les trois accelerateurs EXACTS de l'audit epingle :
//
//   1. L'INFIMUM SEPARABLE L4 : par axe, t = clip(x+y, [2wl, 2wh]) et
//      min*4 = (t-2x)(t-2y) sur les quatre couples d'extremites ; somme des
//      axes. `L4 >= 0` certifie qu'un noeud ne contient AUCUN temoin strict
//      et le retire immediatement — decision monotone sous raffinement. Les
//      contacts restent au census ferme.
//   2. L'HERITAGE DE TEMOINS PAR IDENTIFIANTS : au plus neuf positions deja
//      certifiees strictes pour le bloc parent (elles restent strictes et
//      hors extremites sous restriction a un enfant — les plages d'un
//      enfant sont des sous-plages). Jamais un scalaire sans IDs, jamais une
//      frontiere tronquee : la recherche ne cherche que les temoins
//      MANQUANTS et exclut les herites du recomptage.
//   3. LA PILE REUTILISEE : capacite reservee a la construction, plus aucune
//      allocation en regime etabli (la premiere croissance peut reallouer).
//
// COMPTEURS : `L4-retraits (points)` et `herites` sont des MULTIPLICITES de
// travail par recherche (un meme point compte a chaque bloc qui le retire ou
// l'herite), jamais des cardinaux de PointId uniques.
//
// Les DECISIONS sont inchangees : un noeud L4 >= 0 ne porte aucun temoin,
// et les herites appartiennent a l'ensemble decouvrable du fils — les
// masses (etats, prunees, microtuiles) restent identiques au triplet pres,
// la porte terrain 400 le verifie. L'ancien heritage de frontiere PLAFONNEE
// reste retire (troncature irreversible : 52 puis 82 % de microtuiles a
// 2400/12000) ; le code mort confirmed/frontier/cap96 est supprime (constat
// de l'audit delta).
//
// C'EST UN FALSIFICATEUR MASS-ONLY : aucune ancre formee, aucun census,
// aucune BallActivation. Il publie les autorites de parcimonie de la porte
// §7.4 : etats, visites temoin, tests ponctuels, paires prunees,
// microtuiles, profondeur, pile, octets, temps chaud. NO-GO si la majorite
// des paires atteint les microtuiles — la sonde le DIT, elle ne le cache
// pas.
//
// LE DIFFERENTIEL NON COMPENSABLE (porte 4) : `--verify-bruteforce 1` tient
// un LEDGER DE FATE par paire — chaque paire non ordonnee recoit exactement
// UN sort (prunee ou microtuile, multiplicite un ; l'identite agregee
// C(n,2) est compensable, une omission et un doublon de meme masse
// s'annulent) — puis certifie par balayage exact que TOUTE paire non inerte
// (moins de dix temoins stricts) est en microtuile. Aucun compte agrege ne
// remplace cette inclusion.
//
// PORTEE q2 SEULEMENT : dix temoins dans la boule diametrale d'une paire ne
// prouvent RIEN sur les supports q3/q4 dont la sphere est decalee dans le
// plan mediateur — la fixture `q2-vs-q3-scope` grave la paire prunee q2 qui
// RESTE l'ancre du support propre q3 {a,b,z}. Les paires prunees q2 ne
// sortent JAMAIS d'une source d'ancres superieures.
//
// Codes : 0 OK ; 1 identite/soundness/plancher viole ; 2 CLI ; 3
// budget/NO-GO ; 4 mutant tue (--inject).
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/cloud_families.hpp"

namespace {

using i64 = long long;
using i128 = __int128;

struct Node {
  i64 lo[3] = {0, 0, 0};
  i64 hi[3] = {0, 0, 0};
  int begin = 0, end = 0;       // plage de feuilles [begin, end) dans `order`
  int left = -1, right = -1;    // left < 0 : feuille
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

// LE SUP EXACT de (w-x).(w-y) sur le triple de boites (W, A, B) : separable
// par axe, atteint sur les extremes — 8 combinaisons par axe, entiers i64
// (bornes 2^17, produits < 2^36, somme < 2^38).
inline i64 pair_witness_sup(const Node& witness, const Node& a, const Node& b) {
  i64 total = 0;
  for (int d = 0; d < 3; ++d) {
    i64 best = -((i64)1 << 62);
    const i64 w_ext[2] = {witness.lo[d], witness.hi[d]};
    const i64 a_ext[2] = {a.lo[d], a.hi[d]};
    const i64 b_ext[2] = {b.lo[d], b.hi[d]};
    for (int wi = 0; wi < 2; ++wi)
      for (int ai = 0; ai < 2; ++ai)
        for (int bi = 0; bi < 2; ++bi) {
          const i64 value = (w_ext[wi] - a_ext[ai]) * (w_ext[wi] - b_ext[bi]);
          if (value > best) best = value;
        }
    total += best;
  }
  return total;
}

// L'INFIMUM SEPARABLE EXACT (x4) du meme triple de boites (audit epingle
// AUDIT_Q2_SELFJOIN) : par axe, pour chaque couple d'extremites (x,y), le
// minimum en w de 4(w-x)(w-y) = (2w-x-y)^2 - (x-y)^2 est atteint au point
// t = clip(x+y, [2wl, 2wh]) et vaut (t-2x)(t-2y) ; minimum sur les quatre
// couples puis somme des axes. `L4 >= 0` certifie que la boite temoin ne
// contient AUCUN temoin strict — le noeud est retire, plages comprises, et
// la decision est monotone sous raffinement. Bornes : |t-2x| < 3*2^17,
// produits < 2^38, somme < 2^40 (i64).
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

// Le meme sup pour un TEMOIN PONCTUEL w contre le bloc (A,B).
inline i64 point_witness_sup(const mhgp::P3& w, const Node& a, const Node& b) {
  i64 total = 0;
  const i64 c[3] = {(i64)w.x, (i64)w.y, (i64)w.z};
  for (int d = 0; d < 3; ++d) {
    i64 best = -((i64)1 << 62);
    const i64 a_ext[2] = {a.lo[d], a.hi[d]};
    const i64 b_ext[2] = {b.lo[d], b.hi[d]};
    for (int ai = 0; ai < 2; ++ai)
      for (int bi = 0; bi < 2; ++bi) {
        const i64 value = (c[d] - a_ext[ai]) * (c[d] - b_ext[bi]);
        if (value > best) best = value;
      }
    total += best;
  }
  return total;
}

struct ProbeReceipt {
  i64 states = 0;
  i64 pruned_states = 0;
  i64 witness_visits = 0;        // noeuds visites par la recherche de temoins
  i64 witness_point_tests = 0;   // tests ponctuels dans les feuilles (porte 5)
  i64 pruned_pairs = 0;
  i64 microtile_pairs = 0;
  i64 microtile_states = 0;
  i64 depth_max = 0;
  i64 witness_stack_high_water = 0;   // pile de la recherche (elements)
  i64 l4_skipped_nodes = 0;      // noeuds retires par l'infimum L4 >= 0
  i64 l4_skipped_points = 0;     // points couverts par ces retraits
  i64 inherited_credits = 0;     // temoins herites credites sans recherche
  i64 early_exits = 0;           // recherches arretees au seuil
};

// LES INJECTIONS (porte 4) : chacune doit mourir par le ledger de fate ou
// l'identite, jamais par un compte agrege compensable.
struct ProbeInjections {
  bool skip_half_block = false;     // partition croisee : le fils droit est perdu
  bool drop_rr = false;             // partition interne : (R,R) est perdu
  bool threshold_nine = false;      // dixieme temoin : seuil 9 au lieu de 10
  bool count_shell = false;         // contact : dot == 0 compte comme interieur
  bool drop_last_microtile = false; // dernier bloc : la derniere microtuile est perdue
  // DUPLICATION COMPENSEE : quand les deux fils d'un split croise ont la meme
  // masse, l'un est traite DEUX fois et l'autre omis — l'identite agregee
  // C(n,2) FERME quand meme ; seul le sort par paire (multiplicite un,
  // partition) peut mordre. C'est le mutant qui prouve que le ledger de fate
  // est necessaire.
  bool duplicate_compensated = false;
  // OVERSHOOT DU GENERATEUR (contre-exemple de cardinalite de l'audit etat
  // courant) : l'ancienne garde `size < n` testee avant un pixel laissait un
  // ou deux echos multi-echo depasser n — ledger, fate et oracle etaient
  // alors dimensionnes avec le n demande. Le driver exige pts.size() == n.
  bool generator_overshoot = false;
  bool any() const {
    return skip_half_block || drop_rr || threshold_nine || count_shell ||
           drop_last_microtile || duplicate_compensated || generator_overshoot;
  }
};

// LE LEDGER DE FATE : un sort par paire non ordonnee, multiplicite un.
enum class PairFate : std::uint8_t { kUnassigned = 0, kPruned = 1, kMicrotile = 2 };

// LES TEMOINS HERITES : au plus neuf POSITIONS de feuille deja certifiees
// strictes pour le bloc parent — jamais un scalaire sans identifiants. Les
// plages d'un enfant sont des sous-plages du parent, donc un temoin hors des
// extremites du parent reste hors de celles de l'enfant, et strict pour
// toutes les paires de l'enfant (sous-ensemble des paires du parent).
struct InheritedWitnesses {
  std::array<int, 9> positions{};
  int count = 0;
};

struct Probe {
  const Tree* tree = nullptr;
  ProbeReceipt receipt;
  ProbeInjections injections;
  int witness_threshold = 10;
  i64 max_states = 0;
  bool budget_exceeded = false;
  // Fate : actif seulement a petit n (`--verify-bruteforce`) ; nullptr sinon.
  std::vector<PairFate>* fate = nullptr;
  int fate_points = 0;
  bool fate_violated = false;
  i64 last_microtile_a = -1, last_microtile_b = -1;   // pour drop-last-microtile
  // Pile REUTILISEE, capacite reservee : plus d'allocation en regime etabli.
  std::vector<int> stack_ = [] { std::vector<int> s; s.reserve(256); return s; }();

  // Plages disjointes : le temoin ne recouvre AUCUNE des deux extremites.
  static bool ranges_disjoint(const Node& witness, const Node& a, const Node& b) {
    const bool overlap_a = witness.begin < a.end && a.begin < witness.end;
    const bool overlap_b = witness.begin < b.end && b.begin < witness.end;
    return !overlap_a && !overlap_b;
  }

  // LA RECHERCHE DE TEMOINS a sortie precoce, infimum L4 et heritage par
  // identifiants : crediter les herites (exclus du recomptage), retirer tout
  // noeud `L4 >= 0` (aucun temoin strict possible), compter les noeuds
  // entierement negatifs et les points negatifs des feuilles (hors positions
  // d'extremites), s'arreter au seuil, et RECOLTER jusqu'a neuf positions
  // strictes pour les enfants. Aucune frontiere plafonnee, aucun cap : le
  // compte rendu est exact ou majore par le seuil, jamais tronque en
  // silence.
  i64 count_witnesses(const Node& a, const Node& b, const InheritedWitnesses& inherited,
                      InheritedWitnesses* harvested) {
    const auto is_inherited = [&](int position) {
      for (int i = 0; i < inherited.count; ++i)
        if (inherited.positions[(std::size_t)i] == position) return true;
      return false;
    };
    const auto harvest = [&](int position) {
      if (harvested->count < 9)
        harvested->positions[(std::size_t)harvested->count++] = position;
    };
    harvested->count = 0;
    i64 total = inherited.count;
    receipt.inherited_credits += inherited.count;
    for (int i = 0; i < inherited.count; ++i) harvest(inherited.positions[(std::size_t)i]);
    stack_.clear();
    stack_.push_back(0);
    while (!stack_.empty()) {
      receipt.witness_stack_high_water =
          std::max(receipt.witness_stack_high_water, (i64)stack_.size());
      const int node_index = stack_.back();
      stack_.pop_back();
      const Node& node = tree->nodes[(std::size_t)node_index];
      ++receipt.witness_visits;
      // L4 D'ABORD : aucun temoin strict dans la boite — retire, plages
      // d'extremites comprises. Le MUTANT count-shell confond coquille et
      // interieur PARTOUT : il ne retire que `L4 > 0`, sinon le contact
      // qu'il doit compter a tort serait retire avant de mordre.
      const i64 inf4 = pair_witness_inf4(node, a, b);
      if (injections.count_shell ? inf4 > 0 : inf4 >= 0) {
        ++receipt.l4_skipped_nodes;
        receipt.l4_skipped_points += node.end - node.begin;
        continue;
      }
      if (ranges_disjoint(node, a, b)) {
        const i64 sup = pair_witness_sup(node, a, b);
        // MUTANT count-shell : un sup nul (contact possible) compterait le
        // noeud entier — dot == 0 est SHELL, jamais interieur.
        if (sup < 0 || (injections.count_shell && sup == 0)) {
          i64 fresh = node.end - node.begin;
          for (int i = 0; i < inherited.count; ++i)
            if (inherited.positions[(std::size_t)i] >= node.begin &&
                inherited.positions[(std::size_t)i] < node.end)
              --fresh;   // deja credite par l'heritage
          total += fresh;
          for (int t = node.begin; t < node.end && harvested->count < 9; ++t)
            if (!is_inherited(t)) harvest(t);
          if (total >= witness_threshold) {
            ++receipt.early_exits;
            return total;
          }
          continue;
        }
        if (node.left < 0) {
          for (int t = node.begin; t < node.end; ++t) {
            if (is_inherited(t)) continue;
            const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
            ++receipt.witness_point_tests;
            const i64 sup_point = point_witness_sup(w, a, b);
            if (sup_point < 0 || (injections.count_shell && sup_point == 0)) {
              ++total;
              harvest(t);
            }
          }
          if (total >= witness_threshold) {
            ++receipt.early_exits;
            return total;
          }
          continue;
        }
        stack_.push_back(node.right);
        stack_.push_back(node.left);
        continue;
      }
      // RECOUVREMENT d'une extremite : descendre, ou tester par point en
      // excluant les positions d'extremites dans les feuilles.
      if (node.left < 0) {
        for (int t = node.begin; t < node.end; ++t) {
          const bool in_a = t >= a.begin && t < a.end;
          const bool in_b = t >= b.begin && t < b.end;
          if (in_a || in_b) continue;
          if (is_inherited(t)) continue;
          const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
          ++receipt.witness_point_tests;
          const i64 sup_point = point_witness_sup(w, a, b);
          if (sup_point < 0 || (injections.count_shell && sup_point == 0)) {
            ++total;
            harvest(t);
          }
        }
        if (total >= witness_threshold) {
          ++receipt.early_exits;
          return total;
        }
        continue;
      }
      stack_.push_back(node.right);
      stack_.push_back(node.left);
    }
    return total;
  }

  // Le sort d'une paire par identifiants ORIGINAUX (pas les positions de
  // feuille) : multiplicite un, sinon violation — l'idempotence d'un DSU
  // cacherait un doublon, le ledger non.
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

  void assign_cross_block(const Node& a, const Node& b, PairFate value) {
    if (fate == nullptr) return;
    for (int ta = a.begin; ta < a.end; ++ta)
      for (int tb = b.begin; tb < b.end; ++tb)
        assign_pair(tree->order[(std::size_t)ta], tree->order[(std::size_t)tb], value);
  }

  void assign_internal_block(const Node& a, PairFate value) {
    if (fate == nullptr) return;
    for (int ta = a.begin; ta < a.end; ++ta)
      for (int tb = ta + 1; tb < a.end; ++tb)
        assign_pair(tree->order[(std::size_t)ta], tree->order[(std::size_t)tb], value);
  }

  // La machine d'etats : (ia, ib) avec ia == ib pour les paires internes.
  // Les temoins recoltes d'un bloc croise sont herites par ses deux enfants.
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
      // Paires internes : decomposition L,L / L,R / R,R — une feuille interne
      // est une microtuile (ses C(taille,2) paires sont residuelles). Aucun
      // heritage : le parent interne n'a pas de recherche de temoins.
      if (a.left < 0) {
        const i64 size = a.end - a.begin;
        receipt.microtile_pairs += size * (size - 1) / 2;
        ++receipt.microtile_states;
        last_microtile_a = ia;
        last_microtile_b = ib;
        assign_internal_block(a, PairFate::kMicrotile);
        return;
      }
      process(a.left, a.left, depth + 1, {});
      process(a.left, a.right, depth + 1, {});
      if (!injections.drop_rr)   // MUTANT : la partition interne perd (R,R)
        process(a.right, a.right, depth + 1, {});
      return;
    }
    InheritedWitnesses harvested;
    const i64 found = count_witnesses(a, b, inherited, &harvested);
    if (found >= witness_threshold - (injections.threshold_nine ? 1 : 0)) {
      // MUTANT threshold-nine : neuf temoins pruneraient — p+q >= 11 < K+2.
      receipt.pruned_pairs += (i64)(a.end - a.begin) * (i64)(b.end - b.begin);
      ++receipt.pruned_states;
      assign_cross_block(a, b, PairFate::kPruned);
      return;
    }
    const bool a_leaf = a.left < 0, b_leaf = b.left < 0;
    if (a_leaf && b_leaf) {
      receipt.microtile_pairs += (i64)(a.end - a.begin) * (i64)(b.end - b.begin);
      ++receipt.microtile_states;
      last_microtile_a = ia;
      last_microtile_b = ib;
      assign_cross_block(a, b, PairFate::kMicrotile);
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
      // MUTANT COMPENSE : deux fois le fils gauche, zero fois le droit —
      // memes masses, l'agrege ferme, seul le sort par paire voit.
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
      if (!injections.skip_half_block)   // MUTANT : la partition croisee perd un fils
        process(child_right, ib, depth + 1, harvested);
    } else {
      process(ia, child_left, depth + 1, harvested);
      if (!injections.skip_half_block)
        process(ia, child_right, depth + 1, harvested);
    }
  }
};

// Le compte EXACT de temoins strictement interieurs d'une paire, par
// balayage complet — l'autorite du differentiel, independante de l'arbre.
inline int exact_interior_count(const std::vector<mhgp::P3>& pts, int x, int y, int cap) {
  int interior = 0;
  const int n = (int)pts.size();
  for (int w = 0; w < n && interior < cap; ++w) {
    if (w == x || w == y) continue;
    const i64 wx[3] = {(i64)pts[(std::size_t)w].x - (i64)pts[(std::size_t)x].x,
                       (i64)pts[(std::size_t)w].y - (i64)pts[(std::size_t)x].y,
                       (i64)pts[(std::size_t)w].z - (i64)pts[(std::size_t)x].z};
    const i64 wy[3] = {(i64)pts[(std::size_t)w].x - (i64)pts[(std::size_t)y].x,
                       (i64)pts[(std::size_t)w].y - (i64)pts[(std::size_t)y].y,
                       (i64)pts[(std::size_t)w].z - (i64)pts[(std::size_t)y].z};
    const i64 dot = wx[0] * wy[0] + wx[1] * wy[1] + wx[2] * wy[2];
    if (dot < 0) ++interior;
  }
  return interior;
}

// LES FIXTURES GRAVEES (porte 4). Chacune retourne son nuage et pose ses
// assertions specifiques apres le run via `check`.
struct Fixture {
  const char* name = nullptr;
  std::vector<mhgp::P3> cloud;
  // Paire visee (identifiants originaux) et sort exige.
  int pair_x = -1, pair_y = -1;
  bool require_fate = false;   // exiger expected_fate (la granularite de bloc
                               // peut legitimement laisser une paire inerte en
                               // microtuile : residuel conservateur, pas faux)
  PairFate expected_fate = PairFate::kUnassigned;
  int expected_interior = -1;   // compte exact exige (-1 : non impose)
};

Fixture make_fixture(const std::string& name) {
  Fixture fixture;
  if (name == "contact") {
    // CONTACT dot == 0 : dix observations EXACTEMENT sur la sphere
    // diametrale de (x,y) — coquille, jamais interieur. Les extremites sont
    // ISOLEES (x en feuille singleton par position minimale, y duplique pour
    // une boite de feuille exacte) et les temoins sont DUPLIQUES sur un meme
    // point de grille : les noeuds temoins ont des boites de largeur nulle,
    // le sup de bloc vaut EXACTEMENT zero — le mutant count-shell compte dix
    // et prune la paire non inerte ; le nominal la garde en microtuile.
    fixture.name = "contact";
    fixture.cloud.push_back(mhgp::P3{1000, 2000, 2000});   // x, minimal en x
    fixture.cloud.push_back(mhgp::P3{3000, 2000, 2000});   // y
    fixture.cloud.push_back(mhgp::P3{3000, 2000, 2000});   // y duplique
    for (int j = 0; j < 10; ++j)
      fixture.cloud.push_back(mhgp::P3{2000, 3000, 2000});   // (w-x).(w-y) == 0
    fixture.pair_x = 0;
    fixture.pair_y = 1;
    fixture.require_fate = true;
    fixture.expected_fate = PairFate::kMicrotile;
    fixture.expected_interior = 0;
    return fixture;
  }
  if (name == "tenth-witness") {
    // DIXIEME TEMOIN : exactement NEUF temoins stricts — p+q = 11 < K+2, la
    // paire n'est pas inerte et doit rester en microtuile ; le mutant
    // threshold-nine la prunerait. Les extremites sont dupliquees pour que
    // leurs boites de feuilles soient exactes : le compte de bloc atteint
    // EXACTEMENT neuf, jamais dix.
    fixture.name = "tenth-witness";
    fixture.cloud.push_back(mhgp::P3{10000, 10000, 10000});   // x
    fixture.cloud.push_back(mhgp::P3{10000, 10000, 10000});   // x duplique
    fixture.cloud.push_back(mhgp::P3{10400, 10000, 10000});   // y
    fixture.cloud.push_back(mhgp::P3{10400, 10000, 10000});   // y duplique
    for (int j = 0; j < 9; ++j)
      fixture.cloud.push_back(mhgp::P3{(mhgp::i32)(10050 + 40 * j), 10000, 10000});
    fixture.pair_x = 0;
    fixture.pair_y = 2;
    fixture.require_fate = true;
    fixture.expected_fate = PairFate::kMicrotile;
    fixture.expected_interior = 9;
    return fixture;
  }
  if (name == "duplicate") {
    // COORDONNEES DUPLIQUEES : deux observations sur le meme point de
    // grille. La partition et le ledger doivent fermer exactement — une
    // paire de doublons a une boule diametrale de rayon nul, jamais prunee.
    fixture.name = "duplicate";
    fixture.cloud = {mhgp::P3{100, 100, 100}, mhgp::P3{100, 100, 100},
                     mhgp::P3{200, 100, 100}, mhgp::P3{300, 200, 100},
                     mhgp::P3{150, 250, 300}, mhgp::P3{400, 400, 400}};
    fixture.pair_x = 0;
    fixture.pair_y = 1;
    fixture.require_fate = true;
    fixture.expected_fate = PairFate::kMicrotile;
    fixture.expected_interior = 0;
    return fixture;
  }
  if (name == "q2-vs-q3-scope") {
    // LA FIXTURE DE PORTEE de l'audit delta, coordonnees GRAVEES par
    // l'auditeur : ab satisfait le certificat de prune q2 (dix temoins
    // strictement dans sa boule diametrale — compte exact impose ci-dessous),
    // mais ab RESTE l'ancre du support propre q3 {a,b,z} dont le cercle
    // circonscrit (centre (100, 655/6, 100)) exclut STRICTEMENT les dix
    // temoins. Les paires prunees q2 ne sortent JAMAIS d'une source d'ancres
    // superieures ; les assertions q3 sont posees dans `main`. Le SORT de la
    // paire n'est pas impose : la granularite des blocs peut la laisser en
    // microtuile (residuel conservateur) — l'inertie exacte, elle, est
    // imposee.
    fixture.name = "q2-vs-q3-scope";
    fixture.cloud.push_back(mhgp::P3{50, 100, 100});    // a
    fixture.cloud.push_back(mhgp::P3{150, 100, 100});   // b
    fixture.cloud.push_back(mhgp::P3{100, 160, 100});   // z
    const i64 witnesses[10][2] = {{51, 95}, {149, 95}, {51, 94}, {149, 94}, {51, 93},
                                  {149, 93}, {52, 92}, {148, 92}, {51, 92}, {149, 92}};
    for (const auto& w : witnesses)
      fixture.cloud.push_back(mhgp::P3{(mhgp::i32)w[0], (mhgp::i32)w[1], 100});
    fixture.pair_x = 0;
    fixture.pair_y = 1;
    fixture.require_fate = false;
    fixture.expected_interior = 10;
    return fixture;
  }
  return fixture;   // name == nullptr : fixture inconnue
}

}  // namespace

int main(int argc, char** argv) {
  int n = 2400, coord = 0, smax = 11, leaf_size = 8, verify_bruteforce = 0;
  i64 seed = 20260810, max_states = 50000000;
  i64 min_pruned_pairs = 0, min_states = 0;
  mhgp3v::CloudFamily family = mhgp3v::CloudFamily::kTerrain;
  std::string fixture_name;
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
    if (!strcmp(argv[i], "--inject")) {
      if (i + 1 >= argc) { std::printf("ECHEC : valeur manquante pour --inject\n"); return 2; }
      ++i;
      if (!strcmp(argv[i], "skip-half-block")) injections.skip_half_block = true;
      else if (!strcmp(argv[i], "drop-rr")) injections.drop_rr = true;
      else if (!strcmp(argv[i], "threshold-nine")) injections.threshold_nine = true;
      else if (!strcmp(argv[i], "count-shell")) injections.count_shell = true;
      else if (!strcmp(argv[i], "drop-last-microtile")) injections.drop_last_microtile = true;
      else if (!strcmp(argv[i], "duplicate-compensated"))
        injections.duplicate_compensated = true;
      else if (!strcmp(argv[i], "generator-overshoot"))
        injections.generator_overshoot = true;
      else { std::printf("ECHEC : injection inconnue %s\n", argv[i]); return 2; }
      continue;
    }
    i64 value = 0;
    const bool has = (i + 1 < argc) && integer(argv[i + 1], &value);
    if (!has) { std::printf("ECHEC : argument %s sans valeur\n", argv[i]); return 2; }
    if (!strcmp(argv[i], "--points")) n = (int)value;
    else if (!strcmp(argv[i], "--coord")) coord = (int)value;
    else if (!strcmp(argv[i], "--smax")) smax = (int)value;
    else if (!strcmp(argv[i], "--leaf-size")) leaf_size = (int)value;
    else if (!strcmp(argv[i], "--seed")) seed = value;
    else if (!strcmp(argv[i], "--max-states")) max_states = value;
    else if (!strcmp(argv[i], "--verify-bruteforce")) verify_bruteforce = (int)value;
    else if (!strcmp(argv[i], "--min-pruned-pairs")) min_pruned_pairs = value;
    else if (!strcmp(argv[i], "--min-states")) min_states = value;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    ++i;
  }
  if (n < 4 || n > 100000 || coord < 0 || coord > 65536 || smax != 11 || leaf_size < 2 ||
      leaf_size > 256 || max_states < 1 || verify_bruteforce < 0 || verify_bruteforce > 1) {
    std::printf("ECHEC : campagne absurde (la sonde est calibree pour smax=11)\n");
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

  const bool is_fixture = !fixture_name.empty();
  Fixture fixture;
  std::vector<mhgp::P3> pts;
  if (is_fixture) {
    fixture = make_fixture(fixture_name);
    if (fixture.name == nullptr) {
      std::printf("ECHEC : fixture inconnue %s\n", fixture_name.c_str());
      return 2;
    }
    pts = fixture.cloud;
    n = (int)pts.size();
    leaf_size = 2;
    verify_bruteforce = 1;   // le ledger de fate est la raison d'etre des fixtures
    std::printf("provenance : --fixture %s (%d points graves, feuilles <= %d)\n",
                fixture.name, n, leaf_size);
  } else {
    if (coord == 0) coord = mhgp3v::cloud_family_default_coord(family, n);
    pts = mhgp3v::make_family_cloud(family, n, coord, seed, injections.generator_overshoot);
    if ((int)pts.size() < n) { std::printf("ECHEC : nuage non genere\n"); return 3; }
    // CONTRAT DE CARDINALITE (audit etat courant) : chaque push du generateur
    // est borne par n ; le driver EXIGE l'egalite avant de construire l'arbre
    // — un nuage plus grand que n desynchroniserait ledger, fate (acces hors
    // bornes du tableau de sorts) et oracle. Le mutant generator-overshoot
    // retablit l'ancienne garde `size < n` avant-pixel et meurt ici.
    if ((int)pts.size() != n) {
      const int code = fail("le contrat de cardinalite du generateur",
                            "le nuage rendu depasse le n demande — l'ancienne garde"
                            " avant-pixel laissait passer des echos");
      return code;
    }
    std::printf("provenance : --points %d --coord %d --smax %d --seed %lld"
                " --family %s --leaf-size %d\n", n, coord, smax, seed,
                mhgp3v::cloud_family_name(family), leaf_size);
  }
  if (verify_bruteforce == 1 && n > 3000) {
    std::printf("ECHEC : le ledger de fate exige n <= 3000 (n^2/2 sorts, n^3 balayage)\n");
    return 2;
  }
  Tree tree;
  const auto t0 = std::chrono::steady_clock::now();
  tree.build(pts, leaf_size);
  const auto t1 = std::chrono::steady_clock::now();

  std::vector<PairFate> fate;
  Probe probe;
  probe.tree = &tree;
  probe.injections = injections;
  probe.witness_threshold = 10;   // q2 : p >= 10 donne p+q >= 12 = K+2
  probe.max_states = max_states;
  if (verify_bruteforce == 1) {
    fate.assign((std::size_t)((i64)n * (n - 1) / 2), PairFate::kUnassigned);
    probe.fate = &fate;
    probe.fate_points = n;
  }
  probe.process(0, 0, 0, {});
  const auto t2 = std::chrono::steady_clock::now();
  if (probe.budget_exceeded) {
    std::printf("ECHEC : budget d'etats depasse (%lld) — la sonde refuse, elle ne"
                " tronque pas\n", max_states);
    return 3;
  }
  // MUTANT drop-last-microtile : le dernier bloc microtuile perd ses paires
  // apres coup — l'identite du ledger doit le voir.
  if (injections.drop_last_microtile && probe.last_microtile_a >= 0) {
    const Node& a = tree.nodes[(std::size_t)probe.last_microtile_a];
    const Node& b = tree.nodes[(std::size_t)probe.last_microtile_b];
    const i64 lost = probe.last_microtile_a == probe.last_microtile_b
                         ? (i64)(a.end - a.begin) * (a.end - a.begin - 1) / 2
                         : (i64)(a.end - a.begin) * (i64)(b.end - b.begin);
    probe.receipt.microtile_pairs -= lost;
  }
  const ProbeReceipt& receipt = probe.receipt;

  // L'IDENTITE DU LEDGER : chaque paire non ordonnee dans exactement un etat.
  const i128 all_pairs = (i128)n * (n - 1) / 2;
  const i128 covered = (i128)receipt.pruned_pairs + (i128)receipt.microtile_pairs;
  if (covered != all_pairs)
    return fail("l'identite du ledger",
                "prunees + microtuiles != C(n,2) — partition du self-produit violee");

  if (verify_bruteforce == 1) {
    // MULTIPLICITE UN : detectee au marquage (un sort deja assigne rejoue).
    if (probe.fate_violated)
      return fail("le ledger de fate", "une paire a recu deux sorts — multiplicite violee");
    // PARTITION : aucun sort manquant.
    for (std::size_t k = 0; k < fate.size(); ++k)
      if (fate[k] == PairFate::kUnassigned)
        return fail("le ledger de fate", "une paire n'a recu aucun sort — partition violee");
    // INCLUSION NON COMPENSABLE : toute paire non inerte est en microtuile.
    // Le balayage exact est l'autorite ; aucun compte agrege ne compense.
    i64 non_inert = 0;
    for (int x = 0; x < n; ++x)
      for (int y = x + 1; y < n; ++y) {
        const int interior = exact_interior_count(pts, x, y, 10);
        const i64 index = (i64)x * (2 * (i64)n - x - 1) / 2 + (y - x - 1);
        if (interior < 10) {
          ++non_inert;
          if (fate[(std::size_t)index] == PairFate::kPruned)
            return fail("la soundness par paire",
                        "une paire NON inerte a ete prunee — le sort par paire le voit,"
                        " un compte agrege l'aurait compense");
        }
      }
    std::printf("fate       : %lld paires non inertes, toutes en microtuile — partition,"
                " multiplicite un et inclusion certifiees paire par paire\n", non_inert);
  }

  // LES ASSERTIONS DE FIXTURE.
  if (is_fixture) {
    const int x = fixture.pair_x, y = fixture.pair_y;
    const int interior = exact_interior_count(pts, x, y, 1 << 20);
    if (fixture.expected_interior >= 0 && interior != fixture.expected_interior)
      return fail("la fixture", "compte interieur exact different du compte grave");
    const i64 index = (i64)x * (2 * (i64)n - x - 1) / 2 + (y - x - 1);
    if (fixture.require_fate && fate[(std::size_t)index] != fixture.expected_fate)
      return fail("la fixture", "le sort de la paire visee n'est pas le sort grave");
    if (!strcmp(fixture.name, "q2-vs-q3-scope")) {
      // LE SUPPORT q3 SURVIT A LA PRUNE q2 : centre exact (100, 655/6, 100),
      // r^2 = 93025/36. Chaque temoin est STRICTEMENT hors du cercle : en
      // multipliant par 36, |6w - (600,655,600)|^2 > 93025. L'ancre ab est
      // le plus long cote (D^2 = 10000 >= |az|^2 = |bz|^2 = 6100).
      const i64 c6[3] = {600, 655, 600};
      const i64 r2_36 = 93025;
      for (int w = 3; w < n; ++w) {
        const i64 dx = 6 * (i64)pts[(std::size_t)w].x - c6[0];
        const i64 dy = 6 * (i64)pts[(std::size_t)w].y - c6[1];
        const i64 dz = 6 * (i64)pts[(std::size_t)w].z - c6[2];
        if (dx * dx + dy * dy + dz * dz <= r2_36)
          return fail("la fixture de portee",
                      "un temoin q2 est dans le cercle q3 — la fixture ne discrimine plus");
      }
      const auto dist2 = [&](int u, int v) {
        const i64 dx = (i64)pts[(std::size_t)u].x - (i64)pts[(std::size_t)v].x;
        const i64 dy = (i64)pts[(std::size_t)u].y - (i64)pts[(std::size_t)v].y;
        const i64 dz = (i64)pts[(std::size_t)u].z - (i64)pts[(std::size_t)v].z;
        return dx * dx + dy * dy + dz * dz;
      };
      if (!(dist2(0, 1) >= dist2(0, 2) && dist2(0, 1) >= dist2(1, 2)))
        return fail("la fixture de portee", "ab n'est plus le plus long cote du support q3");
      // Le libelle dit le FAIT mathematique, jamais un sort de parcours : la
      // granularite conservatrice des blocs peut laisser ab en microtuile
      // (l'audit a montre prunes=0 sur cette fixture) — ab n'en est pas moins
      // H0-inerte q2 par ses dix temoins exacts.
      std::printf("portee     : ab a exactement dix temoins q2 (H0-inerte q2) ET reste"
                  " l'ancre du support propre q3 {a,b,z} — les paires H0-inertes q2 ne"
                  " sortent JAMAIS d'une source d'ancres superieures\n");
    }
  }

  // LES PLANCHERS (anti vert-par-vacuite) : une sonde qui ne prune rien ou
  // ne visite rien ne passe pas une porte qui les exige.
  if (min_pruned_pairs > 0 && receipt.pruned_pairs < min_pruned_pairs)
    return fail("le plancher de prune", "moins de paires prunees que le plancher exige");
  if (min_states > 0 && receipt.states < min_states)
    return fail("le plancher d'etats", "moins d'etats que le plancher exige");

  if (injections.any()) {
    std::printf("MUTANT SURVIVANT : aucune porte n'a mordu\n");
    return 0;
  }

  const double share =
      100.0 * (double)receipt.microtile_pairs / (double)(i64)all_pairs;
  const i64 tree_bytes = (i64)(tree.nodes.size() * sizeof(Node) + tree.order.size() * sizeof(int));
  std::printf("arbre      : %zu noeuds, feuilles <= %d, construction %.3f s, %lld octets\n",
              tree.nodes.size(), leaf_size,
              std::chrono::duration<double>(t1 - t0).count(), tree_bytes);
  std::printf("q2 lane    : etats=%lld (prunes=%lld) visites-temoin=%lld tests-ponctuels=%lld"
              " — prunees=%lld microtuiles=%lld (%lld etats) — ledger C(n,2)=%lld FERME\n",
              receipt.states, receipt.pruned_states, receipt.witness_visits,
              receipt.witness_point_tests, receipt.pruned_pairs, receipt.microtile_pairs,
              receipt.microtile_states, (i64)all_pairs);
  std::printf("recherche  : L4-retraits=%lld noeuds (%lld points), herites=%lld,"
              " sorties precoces=%lld\n", receipt.l4_skipped_nodes,
              receipt.l4_skipped_points, receipt.inherited_credits, receipt.early_exits);
  std::printf("parcimonie : microtuiles %.2f %% des paires, profondeur max=%lld, pile"
              " temoin max=%lld — %.3f s de phase locale (1 thread, pas un warm_e2e)\n",
              share, receipt.depth_max, receipt.witness_stack_high_water,
              std::chrono::duration<double>(t2 - t1).count());
  if (is_fixture) {
    std::printf("OK : fixture %s recue — ledger de fate, partition, inclusion\n",
                fixture.name);
    return 0;
  }
  std::printf("%s : la lane q2 %s la majorite des paires hors des microtuiles\n",
              share < 50.0 ? "OK" : "NO-GO", share < 50.0 ? "garde" : "ne garde PAS");
  return share < 50.0 ? 0 : 3;
}
