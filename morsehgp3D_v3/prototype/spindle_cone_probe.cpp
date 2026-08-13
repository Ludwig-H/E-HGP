// MorseHGP3D v3 — FERMETURE D'ANCRES AVANT `PairId` PAR BANQUE DE CONES.
//
// Specification : audits/AUDIT_REPONSES_MUR_AMAS_CENSUS_SPINDLE_20260812.md
// sections 3 et 7 (reponses Q2 et Q6), et prototype/spindle_cone.hpp.
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=proposition_math_non_recue,
//         public_status=not_claimed.
//
// Ce binaire n'est PAS un benchmark et ne qualifie aucun SLO.
//
// ---------------------------------------------------------------------------
// CE QUE CE SUJET REPARE
//
// Le certificat de front historique cherche ses temoins dans des boules
// CONCENTRIQUES au milieu de l'ancre, de rayons D/2, D/sqrt(12), D/sqrt(15).
// Ces boules sont incluses dans les spindles universels, mais elles n'en sont
// qu'une petite partie : elles ne contiennent JAMAIS de point proche des
// extremites. Sur une famille en amas, l'ancre inter-amas a son milieu dans le
// vide, donc le certificat ne trouve rien et `candidate_pairs` reste quadratique.
//
// Le contre-exemple grave ici tranche : a=(10,10,10), b=(30,10,10),
// z=(11,10,10). Le temoin `z` est colle a `a`, donc a distance 9 du milieu,
// tres au-dela du rayon D/sqrt(15)=5,16 de la boule q4 — le front historique
// ne le voit pas. Pourtant g=76>0 et Q=0 : `z` est universel q3 ET q4. Les
// temoins existent, ils sont seulement ailleurs que la ou on les cherchait.
//
// ---------------------------------------------------------------------------
// L'ORDONNANCE
//
//   1. LBVH Morton exact resident, partage par la banque et les cibles.
//   2. Par endpoint `a` : une requete k-NN EXACTE et bornee construit la
//      banque `Z_a` des `M` plus proches voisins. Jamais un tri de la liste
//      complete — c'est precisement le cout que l'on veut eviter.
//   3. DFS sur les noeuds CIBLES. A `a` et `z` fixes, le domaine des cibles
//      est un cone convexe ouvert : la porte `ALL` par huit coins est une
//      EQUIVALENCE, donc un seul temoin est credite pour toute la masse
//      partenaire du noeud. Les credits sont HERITES par les enfants (un cone
//      qui contient la boite contient toute sous-boite) et ne sont jamais
//      recomptes.
//   4. Des que `smax-1`, `smax-2` et `smax-3` credits distincts sont acquis
//      sur les lanes q2, q3, q4, le noeud entier est ferme : aucun `PairId`
//      n'est forme. La banque n'est JAMAIS balayee par paire.
//   5. Une porte `NONE` fail-open retire du travail les temoins dont le cone
//      ne rencontre pas la boite, sans jamais descendre.
//
// Le certificat est fail-open dans les deux sens : il ne peut que RENONCER a
// fermer une paire, jamais fermer une paire vivante. Le juge ponctuel borne
// (`--verify`) le verifie sur toutes les paires ordonnees, avec une TROISIEME
// ecriture arithmetique (g,Q) qui ne partage aucune quantite intermediaire
// avec le chemin produit.
//
// CODES DE SORTIE : 1 desaccords du juge, 2 campagne refusee avant calcul,
// 3 plancher ou invariant viole, 4 mutant tue.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "prototype/anchor_envelope.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/morton_lbvh.hpp"
#include "prototype/spindle_cone.hpp"

namespace {

using mhgp::i128;
using mhgp::i64;
using mhgp::P3;
using mhgp3v::CloudFamily;
using mhgp3v::LbvhNode;
using mhgp3v::MortonLbvh;
using mhgp3v::cone::Box;
using mhgp3v::cone::ConeMutant;
using mhgp3v::cone::Witness;

namespace cone = mhgp3v::cone;

[[noreturn]] void refuse(const char* why) {
  std::fprintf(stderr, "REFUS : %s\n", why);
  std::exit(2);
}
[[noreturn]] void violate(const char* why) {
  std::fprintf(stderr, "REFUS : %s\n", why);
  std::exit(3);
}

// LES MASQUES DE CREDITS SONT MULTI-MOTS. La banque est le levier dominant du
// certificat : a `M=16` la parcimonie tombe sous 1 %, a `M=64` elle atteint
// 35 % sur `uniform n=500`. Une limite de 64 bits serait donc une limite
// MATHEMATIQUE deguisee en detail d'implementation.
constexpr int kMaskWords = 4;
constexpr int kMaxBank = 64 * kMaskWords;

// Masque de banque : `kMaxBank` bits, un par slot de temoin.
struct Mask {
  unsigned long long w[kMaskWords] = {0, 0, 0, 0};
};

inline bool mask_test(const Mask& m, int j) {
  return (m.w[j >> 6] & (1ULL << (j & 63))) != 0ULL;
}
inline void mask_set(Mask* m, int j) { m->w[j >> 6] |= 1ULL << (j & 63); }
inline int mask_popcount(const Mask& m) {
  int c = 0;
  for (int k = 0; k < kMaskWords; ++k) c += __builtin_popcountll(m.w[k]);
  return c;
}

// ---------------------------------------------------------------------------
// LEDGER. Aucun compteur n'est optionnel, aucun n'est estime. Les compteurs
// nomment leur certificat : ce sont des cones, pas des boules de milieu.
// ---------------------------------------------------------------------------
struct Ledger {
  long long knn_calls = 0;            // une requete par endpoint
  long long knn_node_visits = 0;      // noeuds ouverts par ces requetes
  long long bank_sum = 0;             // somme des tailles de banque
  long long bank_hwm = 0;             // plus grande banque construite
  long long target_node_visits = 0;   // noeuds cibles ouverts
  long long witness_node_tests = 0;   // appels au classifieur (temoin, noeud)
  long long corner_evals = 0;         // predicats ponctuels de coin evalues
  long long credits_new = 0;          // credits acquis a un noeud
  long long credits_inherited = 0;    // credits deja acquis chez un ancetre
  // ATTENTION AU VOCABULAIRE. Ces deux compteurs disent « ce TEMOIN ne peut
  // rien crediter dans ce sous-arbre ». Ils ne ferment AUCUNE masse de paires
  // et ne sont jamais un prune : la fermeture est `cone_node_prunes`.
  long long witness_none_q3 = 0;
  long long witness_none_q4 = 0;
  long long witness_dropped = 0;      // temoins retires du sous-arbre (NONE q2)
  long long cone_node_prunes = 0;     // noeuds fermes sur les trois lanes
  long long mass_closed_q2 = 0;       // masse de paires ORDONNEES par lane morte
  long long mass_closed_q3 = 0;
  long long mass_closed_q4 = 0;
  long long mass_alive_q2 = 0;        // masse restee vivante, meme lane
  long long mass_alive_q3 = 0;
  long long mass_alive_q4 = 0;
  long long mass_closed = 0;          // masse fermee sur les TROIS lanes
  long long mass_surviving = 0;       // masse restante (au moins une lane vive)
  long long candidate_pairs = 0;      // survivants b > a, banque de `a` seule
  long long unknown_to_residual = 0;  // blocs transferes au cap de budget
  long long residual_block_mass = 0;
  long long target_leaf_pair_tests = 0;  // tests par PairId : cape, jamais libre
  long long bank_restarts = 0;           // reconstructions de banque : doit rester 0
  long long pairid_before_terminal = 0;  // invariant : doit rester 0
  long long depth_hwm = 0;

  void merge(const Ledger& o) {
    knn_calls += o.knn_calls;
    knn_node_visits += o.knn_node_visits;
    bank_sum += o.bank_sum;
    bank_hwm = std::max(bank_hwm, o.bank_hwm);
    target_node_visits += o.target_node_visits;
    witness_node_tests += o.witness_node_tests;
    corner_evals += o.corner_evals;
    credits_new += o.credits_new;
    credits_inherited += o.credits_inherited;
    witness_none_q3 += o.witness_none_q3;
    witness_none_q4 += o.witness_none_q4;
    witness_dropped += o.witness_dropped;
    cone_node_prunes += o.cone_node_prunes;
    mass_closed_q2 += o.mass_closed_q2;
    mass_closed_q3 += o.mass_closed_q3;
    mass_closed_q4 += o.mass_closed_q4;
    mass_alive_q2 += o.mass_alive_q2;
    mass_alive_q3 += o.mass_alive_q3;
    mass_alive_q4 += o.mass_alive_q4;
    mass_closed += o.mass_closed;
    mass_surviving += o.mass_surviving;
    candidate_pairs += o.candidate_pairs;
    unknown_to_residual += o.unknown_to_residual;
    residual_block_mass += o.residual_block_mass;
    target_leaf_pair_tests += o.target_leaf_pair_tests;
    bank_restarts += o.bank_restarts;
    pairid_before_terminal += o.pairid_before_terminal;
    depth_hwm = std::max(depth_hwm, o.depth_hwm);
  }
};

// ---------------------------------------------------------------------------
// BANQUE : les M plus proches voisins EXACTS de `a`, par requete bornee sur le
// LBVH.
//
// L'ORDRE CANONIQUE EST (d2, CLE MORTON), PAS (d2, PointId). Ce n'est pas un
// detail de confort : une egalite de distance tranchee par l'identifiant rend
// le CONTENU de la banque dependant de la numerotation, donc l'ensemble des
// paires fermees aussi. La porte de permutation l'a effectivement refute —
// 60 paires divergentes sur `uniform n=400`, 19 sur `terrain`. La cle Morton
// est une bijection des coordonnees : deux points distincts ne peuvent pas la
// partager, et deux points colocalises sont geometriquement interchangeables.
// La banque devient ainsi une fonction du seul nuage.
// ---------------------------------------------------------------------------
struct BankEntry {
  i64 d2 = 0;
  unsigned long long key = 0;
  int id = 0;
  bool worse_than(const BankEntry& o) const {
    return d2 > o.d2 || (d2 == o.d2 && key > o.key);
  }
};

i64 aabb_min_dist2(const LbvhNode& nd, const P3& p) {
  const i64 c[3] = {(i64)p.x, (i64)p.y, (i64)p.z};
  i64 s = 0;
  for (int d = 0; d < 3; ++d) {
    i64 g = 0;
    if (c[d] < nd.lo[d]) g = nd.lo[d] - c[d];
    else if (c[d] > nd.hi[d]) g = c[d] - nd.hi[d];
    s += g * g;
  }
  return s;
}

// Tas MAX de taille bornee : la racine porte le pire voisin retenu.
void bank_push(std::vector<BankEntry>* heap, int cap, const BankEntry& e) {
  if ((int)heap->size() < cap) {
    heap->push_back(e);
    std::push_heap(heap->begin(), heap->end(),
                   [](const BankEntry& x, const BankEntry& y) { return y.worse_than(x); });
    return;
  }
  if (e.worse_than(heap->front())) return;
  std::pop_heap(heap->begin(), heap->end(),
                [](const BankEntry& x, const BankEntry& y) { return y.worse_than(x); });
  heap->back() = e;
  std::push_heap(heap->begin(), heap->end(),
                 [](const BankEntry& x, const BankEntry& y) { return y.worse_than(x); });
}

void build_bank(const MortonLbvh& tree, const std::vector<P3>& pts, int a, int cap,
                std::vector<BankEntry>* heap, std::vector<Witness>* bank, Ledger* led) {
  const P3& pa = pts[(std::size_t)a];
  heap->clear();
  int stack[96];
  int sp = 0;
  stack[sp++] = 0;
  ++led->knn_calls;
  while (sp > 0) {
    const int ni = stack[--sp];
    const LbvhNode& nd = tree.nodes[(std::size_t)ni];
    ++led->knn_node_visits;
    if ((int)heap->size() >= cap) {
      const i64 worst = heap->front().d2;
      const i64 near = aabb_min_dist2(nd, pa);
      if (near > worst) continue;
    }
    if (nd.left < 0) {
      for (int t = nd.begin; t < nd.end; ++t) {
        const int id = tree.order[(std::size_t)t];
        if (id == a) continue;
        const P3& z = pts[(std::size_t)id];
        const i64 dx = (i64)z.x - (i64)pa.x;
        const i64 dy = (i64)z.y - (i64)pa.y;
        const i64 dz = (i64)z.z - (i64)pa.z;
        BankEntry e{};
        e.d2 = dx * dx + dy * dy + dz * dz;
        e.key = tree.keys[(std::size_t)t];
        e.id = id;
        if (e.d2 == 0) continue;  // colocalise : shell, il releve du prefligh duplicats
        bank_push(heap, cap, e);
      }
      continue;
    }
    // Descente au plus proche d'abord : le tas se remplit vite, donc la
    // coupure `near > worst` devient efficace des les premiers noeuds.
    const i64 dl = aabb_min_dist2(tree.nodes[(std::size_t)nd.left], pa);
    const i64 dr = aabb_min_dist2(tree.nodes[(std::size_t)nd.right], pa);
    if (dl <= dr) {
      stack[sp++] = nd.right;
      stack[sp++] = nd.left;
    } else {
      stack[sp++] = nd.left;
      stack[sp++] = nd.right;
    }
    if (sp > 90) violate("pile k-NN saturee");
  }
  std::vector<BankEntry> sorted(heap->begin(), heap->end());
  std::sort(sorted.begin(), sorted.end(),
            [](const BankEntry& x, const BankEntry& y) { return y.worse_than(x); });
  bank->clear();
  for (const BankEntry& e : sorted)
    bank->push_back(cone::make_witness(pa, pts[(std::size_t)e.id], e.id));
  led->bank_sum += (long long)bank->size();
  led->bank_hwm = std::max(led->bank_hwm, (long long)bank->size());
}

// ---------------------------------------------------------------------------
// DFS DES NOEUDS CIBLES. Un frame porte les credits DEJA ACQUIS sur un
// ancetre : un cone qui contient la boite du parent contient celle de tous ses
// enfants, donc un credit ne se reteste ni ne se recompte jamais. Le masque
// `dead` retire definitivement du sous-arbre les temoins dont le cone ne
// rencontre meme pas le demi-espace q2.
// ---------------------------------------------------------------------------
struct Frame {
  int node = 0;
  int depth = 0;
  Mask m2, m3, m4;   // credits acquis, embotes m4 < m3 < m2
  Mask nq3, nq4;     // refutations NONE heritees
  Mask dead;         // temoins retires du sous-arbre
  unsigned lane_dead = 0;  // bits 2,3,4 : lane deja fermee
};

struct Options {
  long long n = 200;
  long long coord = -1;
  long long seed = 1;
  long long smax = 11;
  long long bank_cap = 48;
  long long leaf = 8;
  long long max_depth = 60;
  long long max_visits_per_endpoint = 1 << 20;
  long long leaf_pair_tests = 0;
  bool verify = false;
  bool symmetric = false;
  bool selftest = false;
  bool permute = false;
  CloudFamily family = CloudFamily::kUniform;
  ConeMutant mutant = ConeMutant::kNone;
  std::string fixture;
  long long min_prunes = 0;
  long long min_mass_closed = 0;
  long long min_bank = 0;
  long long min_none_q4 = 0;
  long long min_credits_inherited = 0;
  // LES DEUX PLANCHERS QUI EXIGENT QUE LE VERDICT AIT EU LIEU. Un CTest a
  // `PASS_REGULAR_EXPRESSION` IGNORE le code de sortie : la porte serait verte
  // meme si un plancher rendait 3. Ces portes sont donc des tests a code, et
  // c'est ici que se tient l'anti-vacuite que le regex tenait auparavant : sans
  // `--verify` (resp. `--permute`), le plancher correspondant REFUSE en code 2
  // avant tout calcul, et il ne peut donc plus passer sans juge.
  long long min_judge_accord = 0;
  long long min_permutation_accord = 0;
};

// Bitset des paires ORDONNEES fermees. Il ne sert qu'au juge et a la mesure
// symetrique ; le chemin produit n'en a pas besoin et ne l'alloue pas.
struct ClosedBits {
  std::vector<unsigned long long> w;
  long long n = 0;
  void reset(long long nn) {
    n = nn;
    w.assign((std::size_t)((nn * nn + 63) / 64), 0ULL);
  }
  bool empty() const { return w.empty(); }
  void set(long long a, long long b) {
    const long long k = a * n + b;
    w[(std::size_t)(k >> 6)] |= 1ULL << (k & 63);
  }
  bool test(long long a, long long b) const {
    const long long k = a * n + b;
    return (w[(std::size_t)(k >> 6)] & (1ULL << (k & 63))) != 0ULL;
  }
};

struct Run {
  Ledger led;
  ClosedBits closed;      // vide hors `--verify` / `--symmetric`
  long long candidate_pairs_symmetric = 0;
  double wall_s = 0.0;
};

// Masse de paires ORDONNEES d'un noeud pour l'endpoint `a` : tous ses points
// sauf `a` lui-meme. C'est la masse (a,b), b != a — pas un compte de PointId,
// et surtout pas le `C(n,2)` reconstruit que le contre-audit a refute.
inline long long node_ordered_mass(const LbvhNode& nd, int rank_a) {
  long long m = nd.end - nd.begin;
  if (rank_a >= nd.begin && rank_a < nd.end) --m;
  return m;
}

void account_alive(const LbvhNode& nd, int rank_a, unsigned lane_dead, Ledger* led) {
  const long long mass = node_ordered_mass(nd, rank_a);
  if (mass <= 0) return;
  if ((lane_dead & 4u) == 0) led->mass_alive_q2 += mass;
  if ((lane_dead & 8u) == 0) led->mass_alive_q3 += mass;
  if ((lane_dead & 16u) == 0) led->mass_alive_q4 += mass;
  led->mass_surviving += mass;
}

void run_endpoint(const MortonLbvh& tree, int a, int rank_a,
                  const std::vector<Witness>& bank, const Options& opt, Ledger* led,
                  ClosedBits* closed) {
  const int nb = (int)bank.size();
  if (nb == 0) return;
  const int need2 = mhgp3v::anchor::lane_death_threshold((int)opt.smax, 2);
  const int need3 = mhgp3v::anchor::lane_death_threshold((int)opt.smax, 3);
  const int need4 = mhgp3v::anchor::lane_death_threshold((int)opt.smax, 4);

  std::vector<Frame> stack;
  stack.reserve(128);
  stack.push_back(Frame{});
  long long visits = 0;

  while (!stack.empty()) {
    Frame fr = stack.back();
    stack.pop_back();
    const LbvhNode& nd = tree.nodes[(std::size_t)fr.node];
    ++led->target_node_visits;
    ++visits;
    led->depth_hwm = std::max(led->depth_hwm, (long long)fr.depth);

    const long long mass = node_ordered_mass(nd, rank_a);
    if (mass <= 0) continue;  // le noeud ne contient que `a`

    Box box{};
    for (int d = 0; d < 3; ++d) { box.lo[d] = nd.lo[d]; box.hi[d] = nd.hi[d]; }

    // Les credits herites sont comptes une fois par noeud visite : ce sont
    // exactement les tests que la hierarchie evite de repayer.
    int c2 = mask_popcount(fr.m2);
    int c3 = mask_popcount(fr.m3);
    int c4 = mask_popcount(fr.m4);
    led->credits_inherited += c2;

    for (int j = 0; j < nb; ++j) {
      if (c2 >= need2 && c3 >= need3 && c4 >= need4) break;
      if (mask_test(fr.dead, j)) continue;

      // `want` est la premiere lane que ce temoin n'a pas encore acquise. Une
      // lane deja creditee ne se recredite jamais : c'est le bit
      // `q3_already_credited` du contre-audit, tenu par slot de banque.
      // MUTANT kIgnoreInherited : le temoin repart de la lane la plus basse
      // meme s'il a deja ete credite chez un ancetre. Un enfant `ALL-W4` d'un
      // parent `ALL-W3` recredite alors q3 et q2 : la meme plage est comptee
      // deux fois. C'est exactement la faute que le bit « deja credite q3 »
      // du contre-audit interdit.
      const bool ignore_inherited = (opt.mutant == ConeMutant::kIgnoreInherited);
      int want;
      if (ignore_inherited) want = cone::kLaneQ2;
      else if (!mask_test(fr.m2, j)) want = cone::kLaneQ2;
      else if (!mask_test(fr.m3, j)) want = cone::kLaneQ3;
      else if (!mask_test(fr.m4, j)) want = cone::kLaneQ4;
      else continue;
      if (want >= cone::kLaneQ4 && mask_test(fr.nq4, j)) continue;
      if (want >= cone::kLaneQ3 && mask_test(fr.nq3, j)) continue;

      // `floor` est la plus petite lane >= want dont le seuil n'est pas encore
      // atteint : sous ce plancher, le test ne peut rien apporter au noeud.
      int floor = 0;
      if (want <= cone::kLaneQ2 && c2 < need2) floor = cone::kLaneQ2;
      else if (want <= cone::kLaneQ3 && c3 < need3) floor = cone::kLaneQ3;
      else if (c4 < need4) floor = cone::kLaneQ4;
      if (floor == 0) continue;

      ++led->witness_node_tests;
      const int lane =
          cone::all_lane_of_box(bank[(std::size_t)j], box, opt.mutant, floor, &led->corner_evals);
      if (lane >= floor) {
        // MUTANT kDuplicateSlot : le meme slot est credite une seconde fois,
        // ce qui fabrique un temoin qui n'existe pas.
        const bool dup = (opt.mutant == ConeMutant::kDuplicateSlot) || ignore_inherited;
        if (!mask_test(fr.m2, j) || dup) { mask_set(&fr.m2, j); ++c2; ++led->credits_new; }
        if (lane >= cone::kLaneQ3 && (!mask_test(fr.m3, j) || dup)) { mask_set(&fr.m3, j); ++c3; }
        if (lane >= cone::kLaneQ4 && (!mask_test(fr.m4, j) || dup)) { mask_set(&fr.m4, j); ++c4; }
        continue;
      }
      if (lane != cone::kLaneNone) continue;  // le cone rencontre la boite : rien a refuter
      const unsigned nm = cone::none_mask_of_box(bank[(std::size_t)j], box, opt.mutant);
      if ((nm & cone::kNoneQ3) != 0u) { mask_set(&fr.nq3, j); ++led->witness_none_q3; }
      if ((nm & cone::kNoneQ4) != 0u) { mask_set(&fr.nq4, j); ++led->witness_none_q4; }
      if ((nm & cone::kNoneQ2) != 0u) { mask_set(&fr.dead, j); ++led->witness_dropped; }
    }

    // Fermeture par lane, comptee UNE SEULE FOIS a l'endroit ou elle tombe.
    unsigned lane_dead = fr.lane_dead;
    if ((lane_dead & 4u) == 0u && c2 >= need2) { lane_dead |= 4u; led->mass_closed_q2 += mass; }
    if ((lane_dead & 8u) == 0u && c3 >= need3) { lane_dead |= 8u; led->mass_closed_q3 += mass; }
    if ((lane_dead & 16u) == 0u && c4 >= need4) { lane_dead |= 16u; led->mass_closed_q4 += mass; }

    if ((lane_dead & 28u) == 28u) {
      ++led->cone_node_prunes;
      led->mass_closed += mass;
      if (closed != nullptr) {
        for (int t = nd.begin; t < nd.end; ++t) {
          const int id = tree.order[(std::size_t)t];
          if (id != a) closed->set(a, id);
        }
      }
      continue;  // AUCUN `PairId` n'est forme : le noeud entier est mort
    }

    if (nd.left < 0) {
      // Feuille : la decision est terminale. Les tests par `PairId` sont
      // desarmes par defaut — c'est le cout que la route interdit.
      if (opt.leaf_pair_tests > 0) {
        for (int t = nd.begin; t < nd.end; ++t) {
          const int id = tree.order[(std::size_t)t];
          if (id == a) continue;
          led->target_leaf_pair_tests += (long long)(nb - mask_popcount(fr.dead));
        }
      }
      account_alive(nd, rank_a, lane_dead, led);
      for (int t = nd.begin; t < nd.end; ++t) {
        const int id = tree.order[(std::size_t)t];
        if (id > a) ++led->candidate_pairs;
      }
      continue;
    }

    if (fr.depth + 1 > opt.max_depth || visits > opt.max_visits_per_endpoint) {
      // CAP DE BUDGET : le bloc part au residuel AVEC son recu. Il n'est ni
      // supprime, ni developpe en toutes ses feuilles.
      ++led->unknown_to_residual;
      led->residual_block_mass += mass;
      account_alive(nd, rank_a, lane_dead, led);
      continue;
    }

    Frame child = fr;
    child.lane_dead = lane_dead;
    child.depth = fr.depth + 1;
    child.node = nd.left;
    stack.push_back(child);
    child.node = nd.right;
    stack.push_back(child);
  }
}

// ---------------------------------------------------------------------------
// JUGE PONCTUEL BORNE. Il enumere toutes les paires ordonnees et tous les
// temoins avec la representation HISTORIQUE (g,Q), qui ne partage aucune
// quantite intermediaire avec le chemin produit : ni H, ni E2, ni X2. Un
// defaut commun ne peut donc pas se compenser.
// ---------------------------------------------------------------------------
struct Judge {
  std::vector<unsigned char> truly_dead;  // paire ordonnee fermable au pointwise
  long long closable = 0;
};

Judge judge_pairs(const std::vector<P3>& pts, int smax) {
  const int n = (int)pts.size();
  Judge jj{};
  jj.truly_dead.assign((std::size_t)n * (std::size_t)n, 0);
  const int need2 = mhgp3v::anchor::lane_death_threshold(smax, 2);
  const int need3 = mhgp3v::anchor::lane_death_threshold(smax, 3);
  const int need4 = mhgp3v::anchor::lane_death_threshold(smax, 4);
  for (int a = 0; a < n; ++a) {
    for (int b = 0; b < n; ++b) {
      if (b == a) continue;
      int p2 = 0, p3 = 0, p4 = 0;
      for (int z = 0; z < n; ++z) {
        if (z == a || z == b) continue;
        const int lane = cone::lane_of_target_gq(
            (i64)pts[(std::size_t)a].x, (i64)pts[(std::size_t)a].y, (i64)pts[(std::size_t)a].z,
            (i64)pts[(std::size_t)z].x, (i64)pts[(std::size_t)z].y, (i64)pts[(std::size_t)z].z,
            (i64)pts[(std::size_t)b].x, (i64)pts[(std::size_t)b].y, (i64)pts[(std::size_t)b].z);
        if (lane >= cone::kLaneQ2) ++p2;
        if (lane >= cone::kLaneQ3) ++p3;
        if (lane >= cone::kLaneQ4) ++p4;
      }
      if (p2 >= need2 && p3 >= need3 && p4 >= need4) {
        jj.truly_dead[(std::size_t)a * (std::size_t)n + (std::size_t)b] = 1;
        ++jj.closable;
      }
    }
  }
  return jj;
}

// ---------------------------------------------------------------------------
// SELFTEST ARITHMETIQUE : les TROIS ecritures doivent coincider exactement.
// (H, E2 X2) est le chemin produit, (H, R) sert la porte NONE, (g, Q) est la
// representation historique du lemme du noeud spindle.
// ---------------------------------------------------------------------------
long long g_rng = 0;
i64 next_rand(i64 lo, i64 hi) {
  g_rng = g_rng * 6364136223846793005LL + 1442695040888963407LL;
  const unsigned long long v = (unsigned long long)g_rng >> 17;
  return lo + (i64)(v % (unsigned long long)(hi - lo + 1));
}

int selftest_representations(long long draws, long long span) {
  long long checked = 0;
  for (long long it = 0; it < draws; ++it) {
    i64 a[3], z[3], b[3];
    for (int d = 0; d < 3; ++d) {
      a[d] = next_rand(0, span);
      z[d] = next_rand(0, span);
      b[d] = next_rand(0, span);
    }
    if (a[0] == z[0] && a[1] == z[1] && a[2] == z[2]) continue;
    if (a[0] == b[0] && a[1] == b[1] && a[2] == b[2]) continue;
    const Witness w = cone::make_witness(a[0], a[1], a[2], z[0], z[1], z[2], 0);
    const int l1 = cone::lane_of_target(w, b[0], b[1], b[2]);
    const int l2 = cone::lane_of_target_hr(w, b[0], b[1], b[2]);
    const int l3 = cone::lane_of_target_gq(a[0], a[1], a[2], z[0], z[1], z[2], b[0], b[1], b[2]);
    if (l1 != l2 || l1 != l3) {
      std::fprintf(stderr,
                   "DESACCORD ARITHMETIQUE a=(%lld,%lld,%lld) z=(%lld,%lld,%lld) "
                   "b=(%lld,%lld,%lld) : E2X2=%d HR=%d gQ=%d\n",
                   (long long)a[0], (long long)a[1], (long long)a[2], (long long)z[0],
                   (long long)z[1], (long long)z[2], (long long)b[0], (long long)b[1],
                   (long long)b[2], l1, l2, l3);
      return -1;
    }
    ++checked;
  }
  return (int)std::min<long long>(checked, 1000000);
}

// INVARIANCE PAR ISOMETRIE DU RESEAU. Le predicat n'emploie que des produits
// scalaires et des NORMES de produit vectoriel : il est donc invariant par
// translation entiere, par permutation des axes et par reflexion. Une faute de
// signe ou d'indice dans le produit vectoriel casse au moins l'une des six
// permutations. C'est une porte, pas un confort.
bool selftest_isometries(long long draws) {
  static const int kPerm[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2},
                                  {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
  for (long long it = 0; it < draws; ++it) {
    i64 a[3], z[3], b[3];
    for (int d = 0; d < 3; ++d) {
      a[d] = next_rand(0, 200);
      z[d] = next_rand(0, 200);
      b[d] = next_rand(0, 200);
    }
    if (a[0] == z[0] && a[1] == z[1] && a[2] == z[2]) continue;
    const Witness w0 = cone::make_witness(a[0], a[1], a[2], z[0], z[1], z[2], 0);
    const int ref = cone::lane_of_target(w0, b[0], b[1], b[2]);
    const i64 shift[3] = {next_rand(0, 60000), next_rand(0, 60000), next_rand(0, 60000)};
    for (int pi = 0; pi < 6; ++pi) {
      i64 pa[3], pz[3], pb[3];
      for (int d = 0; d < 3; ++d) {
        pa[d] = a[kPerm[pi][d]] + shift[d];
        pz[d] = z[kPerm[pi][d]] + shift[d];
        pb[d] = b[kPerm[pi][d]] + shift[d];
      }
      const Witness w = cone::make_witness(pa[0], pa[1], pa[2], pz[0], pz[1], pz[2], 0);
      if (cone::lane_of_target(w, pb[0], pb[1], pb[2]) != ref ||
          cone::lane_of_target_hr(w, pb[0], pb[1], pb[2]) != ref ||
          cone::lane_of_target_gq(pa[0], pa[1], pa[2], pz[0], pz[1], pz[2], pb[0], pb[1],
                                  pb[2]) != ref) {
        std::fprintf(stderr, "ISOMETRIE REFUTEE : permutation %d, translation (%lld,%lld,%lld)\n",
                     pi, (long long)shift[0], (long long)shift[1], (long long)shift[2]);
        return false;
      }
    }
  }
  return true;
}

// Porte `ALL` par huit coins : sur une boite ENTIERE, l'inclusion equivaut a
// l'appartenance de tous ses points de reseau. Le brut ci-dessous enumere ces
// points : il ferme l'equivalence dans les deux sens, pas seulement la
// suffisance.
bool selftest_boxes(long long draws, long long* boxes_checked, long long* all_hits,
                    long long* none_hits) {
  for (long long it = 0; it < draws; ++it) {
    i64 a[3], z[3], lo[3], hi[3];
    for (int d = 0; d < 3; ++d) {
      a[d] = next_rand(0, 40);
      z[d] = next_rand(0, 40);
      lo[d] = next_rand(0, 40);
      hi[d] = lo[d] + next_rand(0, 4);
    }
    if (a[0] == z[0] && a[1] == z[1] && a[2] == z[2]) continue;
    const Witness w = cone::make_witness(a[0], a[1], a[2], z[0], z[1], z[2], 0);
    Box box{};
    for (int d = 0; d < 3; ++d) { box.lo[d] = lo[d]; box.hi[d] = hi[d]; }
    const int all_lane = cone::all_lane_of_box(w, box);
    const unsigned nm = cone::none_mask_of_box(w, box);
    int brute_min = cone::kLaneQ4;
    int brute_max = cone::kLaneNone;
    for (i64 x = lo[0]; x <= hi[0]; ++x)
      for (i64 y = lo[1]; y <= hi[1]; ++y)
        for (i64 t = lo[2]; t <= hi[2]; ++t) {
          const int lane = cone::lane_of_target(w, x, y, t);
          brute_min = std::min(brute_min, lane);
          brute_max = std::max(brute_max, lane);
        }
    if (all_lane != brute_min) {
      std::fprintf(stderr, "PORTE ALL FAUSSE : huit coins=%d, reseau=%d\n", all_lane, brute_min);
      return false;
    }
    if ((nm & cone::kNoneQ3) != 0u && brute_max >= cone::kLaneQ3) {
      std::fprintf(stderr, "PORTE NONE q3 FAUSSE : refutation contredite par le reseau\n");
      return false;
    }
    if ((nm & cone::kNoneQ4) != 0u && brute_max >= cone::kLaneQ4) {
      std::fprintf(stderr, "PORTE NONE q4 FAUSSE : refutation contredite par le reseau\n");
      return false;
    }
    if ((nm & cone::kNoneQ2) != 0u && brute_max >= cone::kLaneQ2) {
      std::fprintf(stderr, "PORTE NONE q2 FAUSSE : refutation contredite par le reseau\n");
      return false;
    }
    ++*boxes_checked;
    if (all_lane >= cone::kLaneQ3) ++*all_hits;
    if ((nm & cone::kNoneQ3) != 0u) ++*none_hits;
  }
  return true;
}

// ---------------------------------------------------------------------------
// FIXTURES GRAVEES AUX COORDONNEES EXACTES.
// ---------------------------------------------------------------------------
struct FixtureCase {
  const char* name;
  i64 a[3], z[3], b[3];
  int expected;  // lane attendue
  const char* why;
};

const FixtureCase kFixtures[] = {
    // Frontiere q4 du contre-audit : 3H^2 = E2 X2 exactement. L'egalite est
    // HORS du cone ouvert, donc la lane s'arrete a q3.
    {"q4-frontiere", {10, 10, 10}, {11, 10, 10}, {12, 11, 11}, cone::kLaneQ3,
     "3H^2 = E2 X2 : l'egalite reste hors du cone q4"},
    // Frontiere q3 : 4H^2 = E2 X2 exactement.
    {"q3-frontiere", {10, 10, 10}, {11, 9, 10}, {11, 8, 11}, cone::kLaneQ2,
     "4H^2 = E2 X2 : l'egalite reste hors du cone q3"},
    // LE CONTRE-EXEMPLE QUI REPARE LE MUR DES AMAS. Le temoin est colle a
    // l'endpoint, a distance 9 du milieu, tres au-dela du rayon D/sqrt(15)
    // du front historique ; il est pourtant universel q3 ET q4.
    {"amas-axial", {10, 10, 10}, {11, 10, 10}, {30, 10, 10}, cone::kLaneQ4,
     "temoin colle a l'endpoint, invisible a la boule de milieu, universel q4"},
    // Apex : b = z n'est jamais un temoin de lui-meme.
    {"apex-exclu", {10, 10, 10}, {11, 10, 10}, {11, 10, 10}, cone::kLaneNone,
     "H = 0 a l'apex : le temoin ne se compte jamais lui-meme"},
    // Cone oppose : la cible est du cote de `a`.
    {"cone-oppose", {10, 10, 10}, {11, 10, 10}, {9, 10, 10}, cone::kLaneNone,
     "H < 0 : la positivite exclut le cone oppose"},
    // Le noeud spindle positif de la section 3 du contre-audit, vu depuis un
    // de ses coins : le temoin y est universel q4 pour la paire (a,b).
    {"spindle-coin", {20, 20, 20}, {23, 19, 19}, {32, 20, 20}, cone::kLaneQ4,
     "coin de la boite [23,29]x[19,21]x[19,21] : universel q4"},
    // LARGEUR. E2*X2 = 1,0375e19 depasse 2^63 = 9,2233e18, tandis que
    // 4H^2 = 4,6114e18 ne le depasse pas. En i64 le produit s'enroule et
    // devient NEGATIF : la comparaison rend q4 la ou la verite est q2. La
    // promotion avant multiplication n'est donc pas une precaution, c'est la
    // difference entre un certificat et un tirage.
    {"largeur-i128", {65535, 0, 0}, {32768, 0, 0}, {0, 65535, 65535}, cone::kLaneQ2,
     "E2 X2 deborde i64, 4H^2 non : le mutant etroit rend q4 au lieu de q2"},
};

int run_fixtures(bool verbose) {
  int failures = 0;
  for (const FixtureCase& f : kFixtures) {
    const Witness w = cone::make_witness(f.a[0], f.a[1], f.a[2], f.z[0], f.z[1], f.z[2], 0);
    const int l1 = cone::lane_of_target(w, f.b[0], f.b[1], f.b[2]);
    const int l2 = cone::lane_of_target_hr(w, f.b[0], f.b[1], f.b[2]);
    const int l3 = cone::lane_of_target_gq(f.a[0], f.a[1], f.a[2], f.z[0], f.z[1], f.z[2],
                                           f.b[0], f.b[1], f.b[2]);
    const bool ok = (l1 == f.expected && l2 == f.expected && l3 == f.expected);
    if (!ok) ++failures;
    if (verbose)
      std::printf("fixture %-14s attendu=%d E2X2=%d HR=%d gQ=%d %s  (%s)\n", f.name, f.expected,
                  l1, l2, l3, ok ? "OK" : "FAUX", f.why);
  }
  // Le mutant centre-seul : le centre de la boite est strictement q4, deux de
  // ses coins ne sont meme pas q3. Tester le centre credite un temoin qui ne
  // couvre pas le noeud.
  const Witness w = cone::make_witness(10, 10, 10, 11, 10, 10, 0);
  Box box{};
  box.lo[0] = 12; box.hi[0] = 12;
  box.lo[1] = 8;  box.hi[1] = 12;
  box.lo[2] = 10; box.hi[2] = 10;
  const int all_lane = cone::all_lane_of_box(w, box);
  const int centre_lane = cone::all_lane_of_box(w, box, ConeMutant::kCenterOnly);
  const bool ok = (all_lane == cone::kLaneQ2 && centre_lane == cone::kLaneQ4);
  if (!ok) ++failures;
  if (verbose)
    std::printf("fixture %-14s huit_coins=%d centre_seul=%d %s  (le centre credite a tort)\n",
                "centre-seul", all_lane, centre_lane, ok ? "OK" : "FAUX");

  // BLOC ENTIEREMENT DANS LE CONE, LA OU LA BORNE Hmin/Rmax REPOND UNKNOWN.
  // Section 3 du contre-audit : a=(0,0,0), z=(10,10,0) et le bloc cible
  // [11,100]x[11,100]x{0}. Tous ses points verifient q4 strictement ; le
  // certificat par `Hmin`/`Rmax` y repond pourtant UNKNOWN, parce que ses deux
  // extrema viennent de coins differents (Hmin=20, Rmax=792100, 2Hmin^2=800).
  // La porte `iff` par huit coins tranche ou la borne decouplee renonce : elle
  // est STRICTEMENT plus serree, et c'est la raison de la preferer.
  const Witness wbloc = cone::make_witness(0, 0, 0, 10, 10, 0, 0);
  Box bloc{};
  bloc.lo[0] = 11;  bloc.hi[0] = 100;
  bloc.lo[1] = 11;  bloc.hi[1] = 100;
  bloc.lo[2] = 0;   bloc.hi[2] = 0;
  const int bloc_lane = cone::all_lane_of_box(wbloc, bloc);
  // Le certificat decouple, reproduit ici a l'identique de la section 3.
  i64 hmin = 0;
  i128 rmax = 0;
  for (int k = 0; k < 8; ++k) {
    i64 cc[3];
    cone::box_corner(bloc, k, cc);
    const i64 t[3] = {cc[0] - 10, cc[1] - 10, cc[2] - 0};
    const i64 e[3] = {10, 10, 0};
    const i64 h = t[0] * e[0] + t[1] * e[1] + t[2] * e[2];
    if (k == 0 || h < hmin) hmin = h;
    const i64 cr[3] = {t[1] * e[2] - t[2] * e[1], t[2] * e[0] - t[0] * e[2],
                       t[0] * e[1] - t[1] * e[0]};
    const i128 r = (i128)cr[0] * cr[0] + (i128)cr[1] * cr[1] + (i128)cr[2] * cr[2];
    if (r > rmax) rmax = r;
  }
  const bool decouple_tranche = (hmin > 0 && 2 * (i128)hmin * (i128)hmin > rmax);
  const bool bloc_ok = (bloc_lane == cone::kLaneQ4 && !decouple_tranche && hmin == 20 &&
                        rmax == 792100);
  if (!bloc_ok) ++failures;
  if (verbose)
    std::printf("fixture %-14s huit_coins=%d Hmin=%lld Rmax=%lld decouple_tranche=%s %s\n",
                "bloc-iff", bloc_lane, (long long)hmin, (long long)rmax,
                decouple_tranche ? "oui" : "non", bloc_ok ? "OK" : "FAUX");

  // MASSE EGALE, AABB EGALE, TEMOINS DIFFERENTS. Section 3 du contre-audit :
  // la seule masse d'un noeud ne certifie JAMAIS un budget. Les deux ensembles
  // ci-dessous ont dix points et la meme boite [1,19]x{10}x{10} ; le premier
  // porte neuf temoins universels, le second un seul.
  const i64 riches[10] = {1, 11, 12, 13, 14, 15, 16, 17, 18, 19};
  const i64 pauvres[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 19};
  int n_riches = 0, n_pauvres = 0;
  for (int k = 0; k < 10; ++k) {
    if (cone::lane_of_target_gq(10, 10, 10, riches[k], 10, 10, 30, 10, 10) >= cone::kLaneQ4)
      ++n_riches;
    if (cone::lane_of_target_gq(10, 10, 10, pauvres[k], 10, 10, 30, 10, 10) >= cone::kLaneQ4)
      ++n_pauvres;
  }
  const bool masse_ok = (n_riches == 9 && n_pauvres == 1);
  if (!masse_ok) ++failures;
  if (verbose)
    std::printf("fixture %-14s meme_masse=10 meme_aabb temoins_q4=%d contre %d %s\n",
                "masse-nue", n_riches, n_pauvres, masse_ok ? "OK" : "FAUX");

  // La fixture de largeur est aussi une porte de MUTANT deterministe : elle
  // exige que le calcul etroit reponde AUTREMENT. Sans cette exigence, une
  // implementation qui promeut correctement passerait le test sans jamais
  // prouver que la promotion sert a quelque chose.
  const Witness wide = cone::make_witness(65535, 0, 0, 32768, 0, 0, 0);
  const int wide_exact = cone::lane_of_target(wide, 0, 65535, 65535);
  const int wide_narrow = cone::lane_of_target(wide, 0, 65535, 65535, ConeMutant::kNarrowI64);
  const bool wide_ok = (wide_exact == cone::kLaneQ2 && wide_narrow == cone::kLaneQ4);
  if (!wide_ok) ++failures;
  if (verbose)
    std::printf("fixture %-14s exact=%d etroit=%d %s  (i64 enroule le produit E2 X2)\n",
                "largeur-mutant", wide_exact, wide_narrow, wide_ok ? "OK" : "FAUX");
  return failures;
}

// ---------------------------------------------------------------------------
bool parse_ll(const char* s, long long* out) {
  if (s == nullptr || *s == '\0') return false;
  char* end = nullptr;
  const long long v = std::strtoll(s, &end, 10);
  if (end == nullptr || *end != '\0') return false;
  *out = v;
  return true;
}

Run execute(const std::vector<P3>& pts, const Options& opt, bool want_closed) {
  Run run{};
  const int n = (int)pts.size();
  MortonLbvh tree;
  tree.build(pts, (int)opt.leaf);
  std::vector<int> rank_of((std::size_t)n, 0);
  for (int t = 0; t < n; ++t) rank_of[(std::size_t)tree.order[(std::size_t)t]] = t;
  if (want_closed) run.closed.reset(n);

  std::vector<BankEntry> heap;
  std::vector<Witness> bank;
  const auto t0 = std::chrono::steady_clock::now();
  for (int a = 0; a < n; ++a) {
    build_bank(tree, pts, a, (int)opt.bank_cap, &heap, &bank, &run.led);
    run_endpoint(tree, a, rank_of[(std::size_t)a], bank, opt, &run.led,
                 want_closed ? &run.closed : nullptr);
  }
  const auto t1 = std::chrono::steady_clock::now();
  run.wall_s = std::chrono::duration<double>(t1 - t0).count();
  // FERMETURE SYMETRIQUE. Le predicat de temoin universel est symetrique en
  // (a,b) ; les deux banques ne le sont pas. Une paire non ordonnee est donc
  // morte des que L'UN de ses deux endpoints la ferme. C'est une MESURE, pas
  // le chemin produit : elle demande le bitset des paires ordonnees.
  if (want_closed) {
    for (int a = 0; a < n; ++a)
      for (int b = a + 1; b < n; ++b)
        if (!run.closed.test(a, b) && !run.closed.test(b, a)) ++run.candidate_pairs_symmetric;
  }
  return run;
}

void print_ledger(const Options& opt, const std::vector<P3>& pts, const Run& run) {
  const Ledger& c = run.led;
  const long long n = (long long)pts.size();
  std::printf("SpindleConeReceipt-v1\n");
  std::printf(
      "cadre phase=exploration_v3_hors_registre backend=cpu_reference "
      "profile=quantized_u16_input_only mode=proposition_math_non_recue "
      "public_status=not_claimed\n");
  std::printf("cloud family=%s n=%lld coord=%lld seed=%lld smax=%lld leaf=%lld bank=%lld inject=%s\n",
              opt.fixture.empty() ? mhgp3v::cloud_family_name(opt.family) : opt.fixture.c_str(),
              n, opt.coord, opt.seed, opt.smax, opt.leaf, opt.bank_cap,
              cone::cone_mutant_name(opt.mutant));
  std::printf("banque knn_calls=%lld knn_node_visits=%lld taille_moyenne=%lld hwm=%lld restarts=%lld\n",
              c.knn_calls, c.knn_node_visits, c.knn_calls > 0 ? c.bank_sum / c.knn_calls : 0,
              c.bank_hwm, c.bank_restarts);
  std::printf(
      "cible node_visits=%lld witness_node_tests=%lld corner_evals=%lld credits_new=%lld "
      "credits_herites=%lld profondeur_hwm=%lld\n",
      c.target_node_visits, c.witness_node_tests, c.corner_evals, c.credits_new,
      c.credits_inherited, c.depth_hwm);
  std::printf("refutation_temoin none_q3=%lld none_q4=%lld retires_q2=%lld\n",
              c.witness_none_q3, c.witness_none_q4, c.witness_dropped);
  std::printf(
      "masse_ordonnee entree=%lld fermee=%lld survivante=%lld prunes=%lld "
      "candidate_pairs=%lld\n",
      n * (n - 1), c.mass_closed, c.mass_surviving, c.cone_node_prunes, c.candidate_pairs);
  if (!run.closed.empty())
    std::printf("symetrique paires_non_ordonnees=%lld candidate_pairs_symetrique=%lld\n",
                n * (n - 1) / 2, run.candidate_pairs_symmetric);
  std::printf("masse_par_lane q2=%lld/%lld q3=%lld/%lld q4=%lld/%lld\n", c.mass_closed_q2,
              c.mass_alive_q2, c.mass_closed_q3, c.mass_alive_q3, c.mass_closed_q4,
              c.mass_alive_q4);
  std::printf(
      "budget unknown_to_residual=%lld residual_block_mass=%lld target_leaf_pair_tests=%lld "
      "PairId_before_terminal=%lld\n",
      c.unknown_to_residual, c.residual_block_mass, c.target_leaf_pair_tests,
      c.pairid_before_terminal);
  std::printf("temps wall_s=%f\n", run.wall_s);
}

}  // namespace

int main(int argc, char** argv) {
  Options opt{};
  bool fixtures_only = false;

  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const std::size_t eq = arg.find('=');
    const std::string key = (eq == std::string::npos) ? arg : arg.substr(0, eq);
    const std::string val = (eq == std::string::npos) ? std::string() : arg.substr(eq + 1);
    long long parsed = 0;
    if (key == "--points") { if (!parse_ll(val.c_str(), &parsed)) refuse("--points invalide"); opt.n = parsed; }
    else if (key == "--coord") { if (!parse_ll(val.c_str(), &parsed)) refuse("--coord invalide"); opt.coord = parsed; }
    else if (key == "--seed") { if (!parse_ll(val.c_str(), &parsed)) refuse("--seed invalide"); opt.seed = parsed; }
    else if (key == "--smax") { if (!parse_ll(val.c_str(), &parsed)) refuse("--smax invalide"); opt.smax = parsed; }
    else if (key == "--bank") { if (!parse_ll(val.c_str(), &parsed)) refuse("--bank invalide"); opt.bank_cap = parsed; }
    else if (key == "--leaf") { if (!parse_ll(val.c_str(), &parsed)) refuse("--leaf invalide"); opt.leaf = parsed; }
    else if (key == "--max-depth") { if (!parse_ll(val.c_str(), &parsed)) refuse("--max-depth invalide"); opt.max_depth = parsed; }
    else if (key == "--max-visits") { if (!parse_ll(val.c_str(), &parsed)) refuse("--max-visits invalide"); opt.max_visits_per_endpoint = parsed; }
    else if (key == "--leaf-pair-tests") { if (!parse_ll(val.c_str(), &parsed)) refuse("--leaf-pair-tests invalide"); opt.leaf_pair_tests = parsed; }
    else if (key == "--min-prunes") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-prunes invalide"); opt.min_prunes = parsed; }
    else if (key == "--min-mass-closed") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-mass-closed invalide"); opt.min_mass_closed = parsed; }
    else if (key == "--min-bank") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-bank invalide"); opt.min_bank = parsed; }
    else if (key == "--min-none-q4") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-none-q4 invalide"); opt.min_none_q4 = parsed; }
    else if (key == "--min-credits-herites") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-credits-herites invalide"); opt.min_credits_inherited = parsed; }
    else if (key == "--min-accord-juge") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-accord-juge invalide"); opt.min_judge_accord = parsed; }
    else if (key == "--min-accord-permutation") { if (!parse_ll(val.c_str(), &parsed)) refuse("--min-accord-permutation invalide"); opt.min_permutation_accord = parsed; }
    else if (key == "--verify") { opt.verify = true; }
    else if (key == "--symmetric") { opt.symmetric = true; }
    else if (key == "--selftest") { opt.selftest = true; }
    else if (key == "--fixtures") { fixtures_only = true; }
    else if (key == "--permute") { opt.permute = true; }
    else if (key == "--family") {
      if (val == "uniform") opt.family = CloudFamily::kUniform;
      else if (val == "terrain") opt.family = CloudFamily::kTerrain;
      else if (val == "eight_clusters") opt.family = CloudFamily::kEightClusters;
      else if (val == "scanline_single_pass") opt.family = CloudFamily::kScanlineSinglePass;
      else if (val == "scanline_overlap_multiecho") opt.family = CloudFamily::kScanlineOverlapMultiecho;
      else refuse("--family inconnue");
    } else if (key == "--inject") {
      if (val == "cone-no-positivity") opt.mutant = ConeMutant::kNoPositivity;
      else if (val == "cone-accept-equality") opt.mutant = ConeMutant::kAcceptEquality;
      else if (val == "cone-swap-coeff") opt.mutant = ConeMutant::kSwapCoeff;
      else if (val == "cone-center-only") opt.mutant = ConeMutant::kCenterOnly;
      else if (val == "cone-seven-corners") opt.mutant = ConeMutant::kSevenCorners;
      else if (val == "cone-narrow-i64") opt.mutant = ConeMutant::kNarrowI64;
      else if (val == "cone-none-uses-rmax") opt.mutant = ConeMutant::kNoneUsesRmax;
      else if (val == "cone-duplicate-slot") opt.mutant = ConeMutant::kDuplicateSlot;
      else if (val == "cone-ignore-inherited") opt.mutant = ConeMutant::kIgnoreInherited;
      else refuse("--inject inconnu");
    } else {
      refuse("argument inconnu");
    }
  }

  if (fixtures_only) {
    const int bad = run_fixtures(true);
    if (bad != 0) {
      std::fprintf(stderr, "REFUS : %d fixture(s) gravee(s) refutee(s)\n", bad);
      return 3;
    }
    std::printf("fixtures accord=OUI refutees=0\n");
    return 0;
  }

  if (opt.selftest) {
    g_rng = opt.seed * 2654435761LL + 12345LL;
    const int bad = run_fixtures(false);
    if (bad != 0) {
      std::fprintf(stderr, "REFUS : %d fixture(s) gravee(s) refutee(s)\n", bad);
      return 3;
    }
    // Deux regimes : petites coordonnees (frontieres frequentes) et pleine
    // largeur u16 (ou le mutant i64 deborde).
    const int small = selftest_representations(200000, 12);
    if (small < 0) return 1;
    const int wide = selftest_representations(200000, 65535);
    if (wide < 0) return 1;
    if (!selftest_isometries(20000)) return 1;
    long long boxes = 0, all_hits = 0, none_hits = 0;
    if (!selftest_boxes(20000, &boxes, &all_hits, &none_hits)) return 1;
    if (all_hits < 100 || none_hits < 100) {
      std::fprintf(stderr,
                   "REFUS : porte vide — inclusions=%lld refutations=%lld sur %lld boites\n",
                   all_hits, none_hits, boxes);
      return 3;
    }
    std::printf("selftest accord=OUI triples_petits=%d triples_u16=%d isometries=120000 "
                "boites=%lld inclusions=%lld refutations=%lld fixtures_refutees=0\n",
                small, wide, boxes, all_hits, none_hits);
    return 0;
  }

  if (opt.n < 2 || opt.n > 65535) refuse("--points hors du profil dense u16 [2, 65535]");
  if (opt.smax < 4) refuse("--smax hors bornes");
  if (opt.bank_cap < 1 || opt.bank_cap > kMaxBank) refuse("--bank hors bornes [1, 64]");
  if (opt.leaf < 1 || opt.leaf > 256) refuse("--leaf hors bornes");
  if (opt.mutant != ConeMutant::kNone && !opt.verify)
    refuse("un mutant sans juge ne prouve rien : --inject exige --verify");
  // Un plancher de verdict sans le verdict serait exactement le vert par
  // vacuite. Le refus est ANTERIEUR au calcul, donc code 2.
  if (opt.min_judge_accord > 0 && !opt.verify)
    refuse("un plancher d'accord du juge exige --verify");
  if (opt.min_permutation_accord > 0 && !opt.permute)
    refuse("un plancher d'accord de permutation exige --permute");
  if (opt.verify && opt.n > 700)
    refuse("le juge ponctuel est borne : --verify exige --points <= 700");
  if (opt.coord < 0) opt.coord = mhgp3v::cloud_family_default_coord(opt.family, (int)opt.n);
  if (opt.coord < 2 || opt.coord > 65536) refuse("--coord hors bornes");

  const std::vector<P3> pts =
      mhgp3v::make_family_cloud(opt.family, (int)opt.n, (int)opt.coord, opt.seed);
  const int n = (int)pts.size();

  if ((opt.symmetric || opt.permute) && opt.n > 20000)
    refuse("le bitset des paires ordonnees alloue n^2 bits : --points <= 20000 exige");
  const Run run = execute(pts, opt, opt.verify || opt.symmetric || opt.permute);
  print_ledger(opt, pts, run);

  // ---- Invariants de structure, avant tout plancher --------------------
  const Ledger& c = run.led;
  if (c.pairid_before_terminal != 0)
    violate("un PairId a ete forme avant une decision terminale");
  if (c.bank_restarts != 0) violate("la banque a ete reconstruite : le credit n'est pas herite");
  if (opt.leaf_pair_tests == 0 && c.target_leaf_pair_tests != 0)
    violate("des tests par PairId ont eu lieu alors qu'ils sont desarmes");
  const long long ordered = (long long)n * ((long long)n - 1);
  if (c.mass_closed + c.mass_surviving != ordered)
    violate("identite de masse ordonnee violee");
  if (c.mass_closed_q2 + c.mass_alive_q2 != ordered) violate("identite de masse q2 violee");
  if (c.mass_closed_q3 + c.mass_alive_q3 != ordered) violate("identite de masse q3 violee");
  if (c.mass_closed_q4 + c.mass_alive_q4 != ordered) violate("identite de masse q4 violee");

  // ---- Juge ponctuel ----------------------------------------------------
  int disagreements = 0;
  long long judge_agreed = -1;        // -1 : le juge n'a pas tourne
  long long permutation_accord = -1;  // -1 : la permutation n'a pas tourne
  if (opt.verify) {
    const Judge jj = judge_pairs(pts, (int)opt.smax);
    long long wrong = 0;
    long long agreed = 0;
    for (int a = 0; a < n && wrong < 8; ++a) {
      for (int b = 0; b < n; ++b) {
        if (b == a) continue;
        const std::size_t k = (std::size_t)a * (std::size_t)n + (std::size_t)b;
        if (!run.closed.test(a, b)) continue;
        if (jj.truly_dead[k] == 0) {
          if (wrong < 8)
            std::fprintf(stderr,
                         "DESACCORD : la paire ordonnee (%d,%d) est fermee par le certificat "
                         "mais reste vivante au juge ponctuel\n",
                         a, b);
          ++wrong;
        } else {
          ++agreed;
        }
      }
    }
    const double taux = jj.closable > 0 ? 100.0 * (double)c.mass_closed / (double)jj.closable : 0.0;
    std::printf("juge fermees=%lld accord=%lld desaccords=%lld fermables_pointwise=%lld "
                "parcimonie=%.2f%% accord=%s\n",
                c.mass_closed, agreed, wrong, jj.closable, taux, wrong == 0 ? "OUI" : "NON");
    disagreements = (int)wrong;
    judge_agreed = agreed;
  }

  // ---- Equivariance par permutation -------------------------------------
  //
  // L'INVARIANT EST L'ENSEMBLE DES DECISIONS, PAS LE TRAVAIL. La banque est
  // ordonnee par (d2, PointId) : une renumerotation change donc l'ORDRE dans
  // lequel les temoins sont testes, donc les sorties anticipees, donc
  // `witness_node_tests`, `corner_evals` et les refutations NONE. Elle ne peut
  // pas changer un verdict de lane : quand un noeud ne ferme pas, TOUS les
  // temoins encore utiles ont ete testes, et un seuil atteint le reste quel
  // que soit l'ordre d'arrivee. La porte compare donc l'ensemble des paires
  // ORDONNEES fermees, pas les compteurs de travail — c'est le seul enonce
  // que la structure autorise, et il est plus fort.
  if (opt.permute) {
    std::vector<int> perm((std::size_t)n);
    for (int i = 0; i < n; ++i) perm[(std::size_t)i] = i;
    g_rng = opt.seed * 1099511628211LL + 7LL;
    for (int i = n - 1; i > 0; --i) {
      const int j = (int)next_rand(0, i);
      std::swap(perm[(std::size_t)i], perm[(std::size_t)j]);
    }
    std::vector<P3> permuted((std::size_t)n);
    for (int i = 0; i < n; ++i) permuted[(std::size_t)perm[(std::size_t)i]] = pts[(std::size_t)i];
    const Run other = execute(permuted, opt, true);
    long long differ = 0;
    long long same_closed = 0;  // paires ordonnees fermees DES DEUX COTES
    for (int a = 0; a < n; ++a)
      for (int b = 0; b < n; ++b) {
        if (b == a) continue;
        const bool here = run.closed.test(a, b);
        const bool there = other.closed.test(perm[(std::size_t)a], perm[(std::size_t)b]);
        if (here != there) ++differ;
        else if (here) ++same_closed;
      }
    permutation_accord = same_closed;
    const bool same_mass = other.led.mass_closed == c.mass_closed &&
                           other.led.cone_node_prunes == c.cone_node_prunes &&
                           other.led.mass_closed_q2 == c.mass_closed_q2 &&
                           other.led.mass_closed_q3 == c.mass_closed_q3 &&
                           other.led.mass_closed_q4 == c.mass_closed_q4 &&
                           other.led.target_node_visits == c.target_node_visits;
    std::printf("permutation paires_divergentes=%lld paires_concordantes=%lld "
                "masse_fermee=%lld/%lld prunes=%lld/%lld travail_identique=%s accord=%s\n",
                differ, same_closed, c.mass_closed, other.led.mass_closed, c.cone_node_prunes,
                other.led.cone_node_prunes, same_mass ? "OUI" : "NON",
                (differ == 0 && same_mass) ? "OUI" : "NON");
    if (differ != 0 || !same_mass) {
      if (opt.mutant == ConeMutant::kNone) {
        std::fprintf(stderr,
                     "DESACCORD : %lld paires fermees dependent de la numerotation des points\n",
                     differ);
        return 1;
      }
      ++disagreements;
    }
  }

  // ---- Mutant : tue par le juge, ou par un ledger different --------------
  if (opt.mutant != ConeMutant::kNone) {
    Options clean = opt;
    clean.mutant = ConeMutant::kNone;
    clean.permute = false;
    const Run ref = execute(pts, clean, false);
    const bool ledger_differs = ref.led.mass_closed != c.mass_closed ||
                                ref.led.cone_node_prunes != c.cone_node_prunes ||
                                ref.led.mass_closed_q3 != c.mass_closed_q3 ||
                                ref.led.mass_closed_q4 != c.mass_closed_q4 ||
                                ref.led.witness_node_tests != c.witness_node_tests ||
                                ref.led.witness_none_q3 != c.witness_none_q3 ||
                                ref.led.witness_none_q4 != c.witness_none_q4;
    const char* par = disagreements > 0 ? "juge" : (ledger_differs ? "ledger" : "non");
    std::printf("mutant %s tue_par=%s reference_fermee=%lld mutant_fermee=%lld\n",
                cone::cone_mutant_name(opt.mutant), par, ref.led.mass_closed, c.mass_closed);
    if (disagreements > 0 || ledger_differs) {
      std::fprintf(stderr, "MUTANT TUE (%s) : %s\n", par, cone::cone_mutant_name(opt.mutant));
      return 4;
    }
    violate("MUTANT SURVIVANT : la porte est vacueuse pour cette injection");
  }

  if (disagreements > 0) return 1;

  // ---- Planchers : ils interdisent le vert par vacuite -------------------
  if (c.cone_node_prunes < opt.min_prunes) {
    std::fprintf(stderr, "REFUS : plancher de prunes non atteint (%lld < %lld)\n",
                 c.cone_node_prunes, opt.min_prunes);
    return 3;
  }
  if (c.mass_closed < opt.min_mass_closed) {
    std::fprintf(stderr, "REFUS : plancher de masse fermee non atteint (%lld < %lld)\n",
                 c.mass_closed, opt.min_mass_closed);
    return 3;
  }
  if (c.knn_calls > 0 && c.bank_sum / c.knn_calls < opt.min_bank) {
    std::fprintf(stderr, "REFUS : plancher de banque non atteint (%lld < %lld)\n",
                 c.bank_sum / c.knn_calls, opt.min_bank);
    return 3;
  }
  if (c.witness_none_q4 < opt.min_none_q4) {
    std::fprintf(stderr, "REFUS : plancher de refutations q4 non atteint (%lld < %lld)\n",
                 c.witness_none_q4, opt.min_none_q4);
    return 3;
  }
  if (c.credits_inherited < opt.min_credits_inherited) {
    std::fprintf(stderr, "REFUS : plancher de credits herites non atteint (%lld < %lld)\n",
                 c.credits_inherited, opt.min_credits_inherited);
    return 3;
  }
  // Ces deux planchers ne portent pas sur le TRAVAIL mais sur le VERDICT : ils
  // exigent qu'un juge independant, ou une renumerotation complete, ait
  // effectivement confirme une masse de paires fermees. C'est ce qui remplace
  // le `PASS_REGULAR_EXPRESSION` retire, lequel ignorait le code de sortie et
  // aurait donc laisse passer un plancher viole.
  // La sentinelle `-1` dit « ce verdict n'a pas eu lieu ». Elle ne doit pas
  // tomber sous un plancher nul : un plancher non demande ne refuse rien. Le
  // cas « plancher demande, verdict absent » est deja refuse en code 2 plus
  // haut, donc `judge_agreed` est >= 0 des que le plancher est arme.
  if (opt.min_judge_accord > 0 && judge_agreed < opt.min_judge_accord) {
    std::fprintf(stderr, "REFUS : plancher d'accord du juge non atteint (%lld < %lld)\n",
                 judge_agreed, opt.min_judge_accord);
    return 3;
  }
  if (opt.min_permutation_accord > 0 && permutation_accord < opt.min_permutation_accord) {
    std::fprintf(stderr, "REFUS : plancher d'accord de permutation non atteint (%lld < %lld)\n",
                 permutation_accord, opt.min_permutation_accord);
    return 3;
  }
  return 0;
}
