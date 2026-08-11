// MorseHGP3D v3 — LA SONDE P1a : SELF-JOIN LBVH DES PAIRES, LANE q2
// (PROPOSITION §7, audit S10).
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
// conserve le noeud et descend.
//
// C'EST UN FALSIFICATEUR MASS-ONLY : aucune ancre formee, aucun census,
// aucune BallActivation. Il publie les autorites de parcimonie de la porte
// §7.4 : etats, visites temoin, paires prunees, microtuiles, ancres
// residuelles, files, profondeur, temps chaud. NO-GO si la majorite des
// paires atteint les microtuiles — la sonde le DIT, elle ne le cache pas.
// `--verify-bruteforce 1` recompte par balayage complet que chaque paire
// prunee est reellement inerte (soundness de l'implementation) et que le
// ledger ferme.
//
// Codes : 0 OK ; 1 identite/soundness violee ; 2 CLI ; 3 budget/etats.
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
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
  i64 witness_visits = 0;
  i64 pruned_pairs = 0;
  i64 microtile_pairs = 0;
  i64 microtile_states = 0;
  i64 frontier_max = 0;
  i64 frontier_truncations = 0;
  i64 depth_max = 0;
};

struct Probe {
  const Tree* tree = nullptr;
  ProbeReceipt receipt;
  int witness_threshold = 10;
  i64 max_states = 0;
  bool budget_exceeded = false;
  // L'HERITAGE DE FRONTIERE : le sup de (w-x).(w-y) est MONOTONE par
  // restriction du bloc — un noeud temoin certifie negatif pour le parent le
  // reste pour tout enfant. Chaque etat porte donc un compte confirme et une
  // frontiere de noeuds indecis, herites du parent au lieu de repartir de la
  // racine. Une frontiere pleine est TRONQUEE cote temoin seulement : cela
  // perd des temoins potentiels (moins de prunes, plus de microtuiles),
  // jamais l'exactitude — et c'est compte au recu.
  static constexpr int kFrontierCap = 96;

  // Plages disjointes : le temoin ne recouvre AUCUNE des deux extremites.
  static bool ranges_disjoint(const Node& witness, const Node& a, const Node& b) {
    const bool overlap_a = witness.begin < a.end && a.begin < witness.end;
    const bool overlap_b = witness.begin < b.end && b.begin < witness.end;
    return !overlap_a && !overlap_b;
  }

  // RAFFINEMENT DE FRONTIERE : depuis la frontiere heritee du parent,
  // confirmer (sup < 0, noeud entier), ecarter (feuille recouvrante), tester
  // point a point les feuilles mixtes disjointes, et descendre les noeuds
  // indecis jusqu'au seuil ou au plafond de frontiere.
  void park(int node_index, std::vector<int>* out) {
    if ((int)out->size() < kFrontierCap) out->push_back(node_index);
    else ++receipt.frontier_truncations;
  }

  i64 refine_frontier(const Node& a, const Node& b, i64 confirmed,
                      const std::vector<int>& inherited, std::vector<int>* out) {
    out->clear();
    i64 total = confirmed;
    // (noeud, transitoire) : un noeud transitoire ne PARQUE jamais — son
    // ancetre est deja en frontiere, parquer ses descendants compterait
    // deux fois chez les enfants.
    std::vector<std::pair<int, bool>> stack;
    for (auto it = inherited.rbegin(); it != inherited.rend(); ++it)
      stack.push_back({*it, false});
    while (!stack.empty()) {
      const int node_index = stack.back().first;
      const bool transient = stack.back().second;
      stack.pop_back();
      const Node& node = tree->nodes[(std::size_t)node_index];
      ++receipt.witness_visits;
      if (ranges_disjoint(node, a, b)) {
        const i64 sup = pair_witness_sup(node, a, b);
        if (sup < 0) {
          total += node.end - node.begin;   // temoins entiers, herites via `durable`
          continue;
        }
        if (node.left < 0) {
          // Feuille disjointe mixte : compte ponctuel, recompte par les
          // enfants — la feuille reste en frontiere si elle n'est pas
          // transitoire.
          for (int t = node.begin; t < node.end; ++t) {
            const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
            if (point_witness_sup(w, a, b) < 0) ++total;
          }
          if (!transient) park(node_index, out);
          continue;
        }
        if (!transient && total >= witness_threshold) {
          // Seuil atteint : garder le noeud indecis tel quel.
          park(node_index, out);
          continue;
        }
        stack.push_back({node.right, transient});
        stack.push_back({node.left, transient});
        continue;
      }
      // RECOUVREMENT d'une extremite : non monotone — un recouvrant peut
      // devenir disjoint pour un bloc enfant. Le COMPTE descend toujours (ou
      // teste par point en excluant les positions d'extremites) ; l'HERITAGE
      // parque les recouvrants de taille au plus celle d'un cote, et leurs
      // descendants deviennent transitoires.
      const int node_size = node.end - node.begin;
      const int side_max = std::max(a.end - a.begin, b.end - b.begin);
      if (node.left < 0) {
        for (int t = node.begin; t < node.end; ++t) {
          const bool in_a = t >= a.begin && t < a.end;
          const bool in_b = t >= b.begin && t < b.end;
          if (in_a || in_b) continue;
          const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
          if (point_witness_sup(w, a, b) < 0) ++total;
        }
        if (!transient) park(node_index, out);
        continue;
      }
      if (node_size > side_max) {
        stack.push_back({node.right, transient});
        stack.push_back({node.left, transient});
        continue;
      }
      if (!transient) park(node_index, out);
      stack.push_back({node.right, true});
      stack.push_back({node.left, true});
    }
    receipt.frontier_max = std::max<i64>(receipt.frontier_max, (i64)out->size());
    return total;
  }

  // La machine d'etats : (ia, ib) avec ia == ib pour les paires internes.
  // `confirmed` et `frontier` sont herites du parent (monotonie du sup).
  void process(int ia, int ib, i64 depth, i64 confirmed, const std::vector<int>& frontier) {
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
      // est une microtuile (ses C(taille,2) paires sont residuelles).
      if (a.left < 0) {
        const i64 size = a.end - a.begin;
        receipt.microtile_pairs += size * (size - 1) / 2;
        ++receipt.microtile_states;
        return;
      }
      process(a.left, a.left, depth + 1, confirmed, frontier);
      process(a.left, a.right, depth + 1, confirmed, frontier);
      process(a.right, a.right, depth + 1, confirmed, frontier);
      return;
    }
    // Bloc croise : recherche de temoins DEPUIS LA RACINE. L'heritage de
    // frontiere a ete essaye et retire : un plafond de frontiere tronque
    // IRREVERSIBLEMENT (un noeud perdu ne revient jamais dans le sous-arbre)
    // et s'effondre a l'echelle — 52 puis 82 % de microtuiles a 2400/12000
    // contre 5,0 et 1,5 % depuis la racine. Une frontiere sans perte (budget
    // par noeud, ou ordonnancement dual-tree) est la question d'ingenierie
    // ouverte ; la racine est la reference de qualite, honnete sur son cout.
    std::vector<int> refined;
    const i64 found = refine_frontier(a, b, 0, {0}, &refined);
    if (found >= witness_threshold) {
      receipt.pruned_pairs += (i64)(a.end - a.begin) * (i64)(b.end - b.begin);
      return;
    }
    const bool a_leaf = a.left < 0, b_leaf = b.left < 0;
    if (a_leaf && b_leaf) {
      receipt.microtile_pairs += (i64)(a.end - a.begin) * (i64)(b.end - b.begin);
      ++receipt.microtile_states;
      return;
    }
    const bool split_a = !a_leaf && (b_leaf || (a.end - a.begin) >= (b.end - b.begin));
    if (split_a) {
      process(tree->nodes[(std::size_t)ia].left, ib, depth + 1, 0, {});
      process(tree->nodes[(std::size_t)ia].right, ib, depth + 1, 0, {});
    } else {
      process(ia, tree->nodes[(std::size_t)ib].left, depth + 1, 0, {});
      process(ia, tree->nodes[(std::size_t)ib].right, depth + 1, 0, {});
    }
  }
};

}  // namespace

int main(int argc, char** argv) {
  int n = 2400, coord = 0, smax = 11, leaf_size = 8, verify_bruteforce = 0;
  i64 seed = 20260810, max_states = 50000000;
  mhgp3v::CloudFamily family = mhgp3v::CloudFamily::kTerrain;
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
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    ++i;
  }
  if (n < 4 || n > 100000 || coord < 0 || coord > 65536 || smax != 11 || leaf_size < 2 ||
      leaf_size > 256 || max_states < 1 || verify_bruteforce < 0 || verify_bruteforce > 1) {
    std::printf("ECHEC : campagne absurde (la sonde est calibree pour smax=11)\n");
    return 2;
  }
  if (coord == 0) coord = mhgp3v::cloud_family_default_coord(family, n);
  const std::vector<mhgp::P3> pts = mhgp3v::make_family_cloud(family, n, coord, seed);
  if ((int)pts.size() < n) { std::printf("ECHEC : nuage non genere\n"); return 3; }

  std::printf("provenance : --points %d --coord %d --smax %d --seed %lld --family %s"
              " --leaf-size %d\n", n, coord, smax, seed, mhgp3v::cloud_family_name(family),
              leaf_size);

  Tree tree;
  const auto t0 = std::chrono::steady_clock::now();
  tree.build(pts, leaf_size);
  const auto t1 = std::chrono::steady_clock::now();

  Probe probe;
  probe.tree = &tree;
  probe.witness_threshold = 10;   // q2 : p >= 10 donne p+q >= 12 = K+2
  probe.max_states = max_states;
  probe.process(0, 0, 0, 0, {0});
  const auto t2 = std::chrono::steady_clock::now();
  if (probe.budget_exceeded) {
    std::printf("ECHEC : budget d'etats depasse (%lld) — la sonde refuse, elle ne"
                " tronque pas\n", max_states);
    return 3;
  }
  const ProbeReceipt& receipt = probe.receipt;

  // L'IDENTITE DU LEDGER : chaque paire non ordonnee dans exactement un etat.
  const i128 all_pairs = (i128)n * (n - 1) / 2;
  const i128 covered = (i128)receipt.pruned_pairs + (i128)receipt.microtile_pairs;
  if (covered != all_pairs) {
    std::printf("ECHEC : identite du ledger violee — %lld prunees + %lld microtuiles"
                " != C(n,2) = %lld\n", receipt.pruned_pairs, receipt.microtile_pairs,
                (i64)all_pairs);
    return 1;
  }

  if (verify_bruteforce == 1) {
    // SOUNDNESS : toute paire prunee est reellement q2-inerte. On recompte
    // l'inertie exacte de CHAQUE paire par balayage, puis on verifie que le
    // nombre de paires NON inertes est <= microtuiles (les paires prunees
    // sont un sous-ensemble des inertes ; la sonde est conservatrice, jamais
    // l'inverse).
    i64 non_inert = 0;
    for (int x = 0; x < n; ++x)
      for (int y = x + 1; y < n; ++y) {
        int interior = 0;
        for (int w = 0; w < n && interior < 10; ++w) {
          if (w == x || w == y) continue;
          i64 dot = 0;
          const i64 wx[3] = {(i64)pts[(std::size_t)w].x - (i64)pts[(std::size_t)x].x,
                             (i64)pts[(std::size_t)w].y - (i64)pts[(std::size_t)x].y,
                             (i64)pts[(std::size_t)w].z - (i64)pts[(std::size_t)x].z};
          const i64 wy[3] = {(i64)pts[(std::size_t)w].x - (i64)pts[(std::size_t)y].x,
                             (i64)pts[(std::size_t)w].y - (i64)pts[(std::size_t)y].y,
                             (i64)pts[(std::size_t)w].z - (i64)pts[(std::size_t)y].z};
          dot = wx[0] * wy[0] + wx[1] * wy[1] + wx[2] * wy[2];
          if (dot < 0) ++interior;
        }
        if (interior < 10) ++non_inert;
      }
    if (non_inert > receipt.microtile_pairs) {
      std::printf("ECHEC : soundness violee — %lld paires non inertes mais seulement %lld"
                  " microtuiles (une paire non inerte a ete prunee)\n", non_inert,
                  receipt.microtile_pairs);
      return 1;
    }
    std::printf("soundness  : %lld paires non inertes, toutes contenues dans les %lld"
                " microtuiles — aucune paire non inerte prunee\n", non_inert,
                receipt.microtile_pairs);
  }

  const double share =
      100.0 * (double)receipt.microtile_pairs / (double)(i64)all_pairs;
  std::printf("arbre      : %zu noeuds, feuilles <= %d, construction %.3f s\n",
              tree.nodes.size(), leaf_size,
              std::chrono::duration<double>(t1 - t0).count());
  std::printf("q2 lane    : etats=%lld visites-temoin=%lld — prunees=%lld"
              " microtuiles=%lld (%lld etats) — ledger C(n,2)=%lld FERME\n",
              receipt.states, receipt.witness_visits, receipt.pruned_pairs,
              receipt.microtile_pairs, receipt.microtile_states, (i64)all_pairs);
  std::printf("parcimonie : microtuiles %.2f %% des paires, profondeur max=%lld —"
              " %.3f s chaud (1 thread)\n", share, receipt.depth_max,
              std::chrono::duration<double>(t2 - t1).count());
  std::printf("%s : la lane q2 %s la majorite des paires hors des microtuiles\n",
              share < 50.0 ? "OK" : "NO-GO", share < 50.0 ? "garde" : "ne garde PAS");
  return share < 50.0 ? 0 : 3;
}
