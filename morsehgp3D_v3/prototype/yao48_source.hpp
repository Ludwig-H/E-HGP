// MorseHGP3D v3 — LE CŒUR DE LA SOURCE q2 YAO48/LBVH, EN-TETE PARTAGE
// (NOTE_SOLUTION_SOURCE_Q2_YAO48_LBVH_U16_20260811). Consommateurs : le
// falsificateur pair_yao48_source.cpp (juge, fixtures, mutants) et le
// harnais diagnostique warm_e2e nomme. La preuve, les contrats et les
// politiques de travail sont documentes en tete du falsificateur ; ce header
// ne porte que la machine.
#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/morton_lbvh.hpp"

namespace mhgp3v {
namespace yao48 {


using i64 = long long;
using i128 = __int128;

// LA DISPOSITION PARTAGEE (PROPOSITION, jalon 2) : Morton 48 bits et LBVH
// radix viennent de l'en-tete commun ; aucune lane ne garde sa propre copie.
using Node = mhgp3v::LbvhNode;
using Lbvh = mhgp3v::MortonLbvh;

// ---------------------------------------------------------------------------
// CHAMBRES YAO48 : signes (d >= 0 -> +) puis permutation par magnitudes
// DECROISSANTES, ex aequo par indice d'axe croissant — une regle TOTALE,
// identique pour les banques et les cibles (seule la coherence est exigee
// par le theoreme ; la regle est un choix de travail documente).
// ---------------------------------------------------------------------------
inline int chamber_of(i64 dx, i64 dy, i64 dz, i64 canon[3], bool perm_swapped) {
  const i64 d[3] = {dx, dy, dz};
  int octant = 0;
  i64 mag[3];
  for (int k = 0; k < 3; ++k) {
    if (d[k] < 0) octant |= 1 << k;
    mag[k] = d[k] < 0 ? -d[k] : d[k];
  }
  int perm[3] = {0, 1, 2};
  // MUTANT chamber-perm-swapped : la CIBLE est canonisee par magnitudes
  // CROISSANTES alors que les banques restent decroissantes — le desync
  // banque/coupe classe les cibles dans des chambres dont la banque decrit un
  // autre cone ; la fixture region-prune perd son prune et meurt.
  const auto less_rank = [&](int a, int b) {
    if (mag[a] != mag[b]) return perm_swapped ? mag[a] < mag[b] : mag[a] > mag[b];
    return a < b;
  };
  if (less_rank(perm[1], perm[0])) std::swap(perm[0], perm[1]);
  if (less_rank(perm[2], perm[1])) std::swap(perm[1], perm[2]);
  if (less_rank(perm[1], perm[0])) std::swap(perm[0], perm[1]);
  canon[0] = mag[perm[0]];
  canon[1] = mag[perm[1]];
  canon[2] = mag[perm[2]];
  // Encodage 0..5 de la permutation.
  int code = 0;
  if (perm[0] == 0) code = perm[1] == 1 ? 0 : 1;
  else if (perm[0] == 1) code = perm[1] == 0 ? 2 : 3;
  else code = perm[1] == 0 ? 4 : 5;
  return octant * 6 + code;
}

struct SourceInjections {
  bool strict_to_large = false;      // l'egalite directionnelle prune a tort
  bool d_understated = false;        // D = max des K-1 premiers temoins
  bool ownership_doubled = false;    // les feuilles emettent aussi le suffixe
  bool last_region_omitted = false;  // le dernier reçu de region est perdu
  bool census_skips_inf_zero = false;// les contacts inf4 = 0 sautes du census
  bool threshold_minus_one = false;  // tombstone a 9 stricts au lieu de 10
  bool chamber_perm_swapped = false; // desync banque/cible (voir chamber_of)
  bool radial_forgets_chamber = false;// l'enveloppe radiale omet une chambre
  bool any() const {
    return strict_to_large || d_understated || ownership_doubled ||
           last_region_omitted || census_skips_inf_zero || threshold_minus_one ||
           chamber_perm_swapped || radial_forgets_chamber;
  }
};

struct SourceReceipt {
  i64 anchors = 0;
  i64 bank_pops = 0;             // points extraits par le remplissage best-first
  i64 bank_node_visits = 0;      // noeuds pousses dans le tas du remplissage
  i64 bank_cone_visits = 0;      // noeuds visites par la phase B dirigee
  i64 full_chambers = 0;         // banques pleines (multiplicite par ancre)
  i64 underfull_chambers = 0;    // banques sous-pleines APRES remplissage
  i64 prune_node_visits = 0;     // noeuds visites par le parcours de coupe
  i64 region_prunes = 0;         // noeuds entiers remplaces par un reçu
  i64 region_pruned_mass = 0;    // paires couvertes par ces reçus
  i64 radial_prunes = 0;         // noeuds a cheval prunes par l'enveloppe 3D_c
  i64 radial_pruned_mass = 0;    // paires couvertes (comptees DANS region_pruned_mass)
  i64 antichain_nodes = 0;       // noeuds acceptes en bloc par les banques
  i64 point_tombstones = 0;      // cibles tombstonees au point par la coupe
  i64 survivors = 0;             // paires passees au classifieur
  i64 classify_node_visits = 0;  // noeuds visites par les parcours DE LOT
  i64 classify_box_tests = 0;    // evaluations de bornes par survivante
  i64 classify_point_tests = 0;
  i64 classify_list_tests = 0;   // tests par balayage de la liste de voisins
  i64 classify_list_pairs = 0;   // survivantes resolues par la liste
  i64 classifier_tombstones = 0; // dix stricts atteints au classifieur
  i64 census_records = 0;        // records fermes publies
  i64 census_closed_total = 0;   // somme des rangs fermes publies
  i64 census_strict_total = 0;
  i64 census_contact_total = 0;
  i64 stack_high_water = 0;
  i64 heap_high_water = 0;
};

enum class PairFate : std::uint8_t {
  kUnassigned = 0,
  kYaoRegion = 1,        // tombstone par reçu de region
  kYaoPoint = 2,         // tombstone par coupe au point
  kClassifierTomb = 3,   // tombstone par dix stricts au classifieur
  kCensus = 4,           // record ferme publie
};

inline bool fate_is_tombstone(PairFate fate) {
  return fate == PairFate::kYaoRegion || fate == PairFate::kYaoPoint ||
         fate == PairFate::kClassifierTomb;
}

// LA BANQUE FACTORISEE (exigence d'audit) : un reçu de region reference une
// banque `(ancre, chambre, version)` au lieu de recopier ses dix PointId.
struct BankTableEntry {
  int anchor_pos = -1;
  int chamber = -1;
  std::array<int, 10> ids{};
};

// Reçu de prune Yao48 pour le rejeu : la plage possedee de cibles (ou une
// cible exacte) et l'index de banque factorisee.
struct YaoReceipt {
  int anchor_pos = -1;
  int target_begin = -1, target_end = -1;   // plage de positions (region)
  int target_pos = -1;                      // ou cible exacte (point)
  int bank_index = -1;                      // entree de la table des banques
};

// Reçu de l'ENVELOPPE RADIALE multi-chambre (audit de reemploi §4) : la boite
// a cheval, et les banques (pleines) de TOUTES ses chambres compatibles. Le
// rejeu retrouve la banque de chaque cible par sa chambre nominale.
struct RadialReceipt {
  int anchor_pos = -1;
  int target_begin = -1, target_end = -1;
  std::vector<int> bank_indices;
};

struct CensusRecord {
  int pos_low = -1, pos_high = -1;   // positions ; ids via order[]
  i64 closed = 0, strict = 0, contacts = 0;
  std::vector<int> closed_ids;       // mode oracle : liste fermee triee
};

inline constexpr int kOrderK = 10;   // contrat H0 : K = 10, seuil de tombstone

struct YaoSource {
  const Lbvh* tree = nullptr;
  SourceInjections injections;
  SourceReceipt receipt;
  i64 bank_pop_budget = 4096;    // budget de TRAVAIL par ancre (fail-open)
  i64 max_work = 0;              // budget global : depasse => refus atomique
  bool budget_exceeded = false;
  bool oracle_mode = false;
  bool baseline = false;         // differentiel : classification seule
  bool antichain_banks = false;  // banques par antichaine de masse (audit §3)
  std::vector<PairFate>* fate = nullptr;
  int fate_points = 0;
  bool fate_violated = false;
  std::vector<YaoReceipt>* yao_receipts = nullptr;
  std::vector<RadialReceipt>* radial_receipts = nullptr;
  std::vector<CensusRecord>* census_records = nullptr;

  // Les banques de l'ancre courante : 48 chambres x (10 ids, D).
  struct Bank {
    std::array<int, 10> ids{};
    std::array<i64, 10> dist2{};
    int count = 0;
    i64 d_max = 0;
    int table_index = -1;   // entree factorisee (mode oracle)
  };
  std::array<Bank, 48> banks_;
  std::vector<int> stack_;
  i64 chamber_visits = 100000;   // garde-fou fail-open de la phase dirigee
  std::vector<BankTableEntry>* bank_table = nullptr;
  // Le lot de survivantes de l'ancre courante (classification par lots a
  // frontiere partagee — relancer la racine par paire est proscrit).
  struct SurvivorState {
    int target_pos = -1;
    i64 strict = 0, closed = 0, contacts = 0;
    bool done = false;
    bool resolved_by_list = false;
    std::vector<int> closed_ids;
  };
  std::vector<SurvivorState> batch_;
  std::vector<int> arena_;
  // LA LISTE DE VOISINS PARTAGEE DE L'ANCRE : sous-produit du remplissage
  // best-first exact — les points sortent DEJA tries par distance carree.
  // Tout temoin ferme d'une paire (p,q) verifie |w-p| <= |pq| : une
  // survivante avec |pq|^2 strictement sous la borne de completude se
  // classifie par un balayage de prefixe, sans AUCUN parcours d'arbre.
  // Les points colocalises a l'ancre (d^2 = 0, exclus du tas) sont des
  // contacts fermes de TOUTE paire de l'ancre : liste separee.
  std::vector<std::pair<i64, int>> near_list_;   // (d^2, position), trie
  std::vector<int> zero_list_;                   // positions colocalisees
  i64 near_complete_d2_ = -1;   // la liste est complete pour d^2 < borne
  // Fermeture PAR ANCRE du ledger (l'egalite globale seule peut masquer une
  // omission compensee par un doublon).
  bool per_anchor_violated = false;
  bool region_overlap_violated = false;
  std::vector<std::pair<int, int>> anchor_regions_;
  std::vector<int> radial_scratch_;

  int threshold() const { return kOrderK - (injections.threshold_minus_one ? 1 : 0); }

  void assign_pair(int id_a, int id_b, PairFate value) {
    if (fate == nullptr) return;
    if (id_a > id_b) std::swap(id_a, id_b);
    const i64 index = (i64)id_a * (2 * (i64)fate_points - id_a - 1) / 2 + (id_b - id_a - 1);
    if ((*fate)[(std::size_t)index] != PairFate::kUnassigned) {
      fate_violated = true;
      return;
    }
    (*fate)[(std::size_t)index] = value;
  }

  void complete_bank(Bank* bank, int chamber, int anchor_pos) {
    i64 d_max = 0;
    const int upto = injections.d_understated ? kOrderK - 1 : kOrderK;
    for (int i = 0; i < upto; ++i) d_max = std::max(d_max, bank->dist2[(std::size_t)i]);
    bank->d_max = d_max;
    if (bank_table != nullptr) {
      BankTableEntry entry;
      entry.anchor_pos = anchor_pos;
      entry.chamber = chamber;
      entry.ids = bank->ids;
      bank->table_index = (int)bank_table->size();
      bank_table->push_back(entry);
    }
  }

  static i64 dist2_max_to_box(const Node& node, const mhgp::P3& p) {
    i64 total = 0;
    const i64 c[3] = {(i64)p.x, (i64)p.y, (i64)p.z};
    for (int d = 0; d < 3; ++d) {
      const i64 low = c[d] - node.lo[d];
      const i64 high = node.hi[d] - c[d];
      const i64 far = low > high ? low : high;
      total += far * far;
    }
    return total;
  }

  // LES BANQUES PAR ANTICHAINE DE MASSE (audit de reemploi, §3) : le theoreme
  // de Yao n'exige pas les dix plus proches — N'IMPORTE QUELS dix temoins de
  // la chambre a distance carree <= D conviennent. Une seule passe best-first
  // accepte des NOEUDS entiers certifies dans une chambre (jamais descendus :
  // antichaine par construction), credite leur masse et majore
  // D_c = max des maxdist^2 de boites. Les noeuds d'une chambre deja pleine
  // sont sautes sans descente ; seuls les noeuds a cheval descendent. Les
  // chambres de l'octant +++ ne reçoivent aucun credit de NOEUD (une cible
  // dominante n'est jamais possedee et un noeud y contiendrait potentiellement
  // un colocalise de l'ancre, interdit par la variante stricte) ; leurs
  // points passent par la voie ponctuelle qui exclut dist^2 = 0.
  void fill_banks_antichain(int anchor_pos) {
    for (Bank& bank : banks_) {
      bank.count = 0;
      bank.d_max = 0;
      bank.table_index = -1;
    }
    const mhgp::P3& p = (*tree->points)[(std::size_t)tree->order[(std::size_t)anchor_pos]];
    using HeapItem = std::pair<i64, int>;
    std::priority_queue<HeapItem, std::vector<HeapItem>, std::greater<HeapItem>> heap;
    heap.push({0, 0});
    int full = 0;
    const auto credit = [&](Bank& bank, int chamber, int first_pos, int point_count,
                            i64 boxmax_d2) {
      const bool was_full = bank.count >= kOrderK;
      if (was_full) return;
      // Les identifiants canoniques du reçu : les dix premiers rencontres
      // dans l'ordre deterministe du parcours.
      if (bank_table != nullptr) {
        int slot = bank.count;
        for (int t = first_pos; t < first_pos + point_count && slot < kOrderK; ++t, ++slot)
          bank.ids[(std::size_t)slot] = tree->order[(std::size_t)t];
      }
      const bool completes = bank.count + point_count >= kOrderK;
      // MUTANT d-understated (mode antichaine) : la contribution du credit
      // qui COMPLETE la banque est omise du majorant — D_c trop petit, faux
      // prunes possibles, le juge tue.
      if (!(injections.d_understated && completes))
        bank.d_max = std::max(bank.d_max, boxmax_d2);
      bank.count += point_count;
      if (completes) {
        if (bank_table != nullptr) {
          BankTableEntry entry;
          entry.anchor_pos = anchor_pos;
          entry.chamber = chamber;
          entry.ids = bank.ids;
          bank.table_index = (int)bank_table->size();
          bank_table->push_back(entry);
        }
        ++full;
      }
    };
    while (!heap.empty() && full < 48) {
      receipt.heap_high_water = std::max(receipt.heap_high_water, (i64)heap.size());
      const HeapItem top = heap.top();
      heap.pop();
      const Node& node = tree->nodes[(std::size_t)top.second];
      ++receipt.bank_node_visits;
      const bool holds_anchor = anchor_pos >= node.begin && anchor_pos < node.end;
      int chamber;
      i64 mins[3];
      if (!holds_anchor && box_chamber(node, p, &chamber, mins) && chamber >= 6) {
        Bank& bank = banks_[(std::size_t)chamber];
        if (bank.count >= kOrderK) continue;   // chambre pleine : saut entier
        ++receipt.antichain_nodes;
        credit(bank, chamber, node.begin, node.end - node.begin,
               dist2_max_to_box(node, p));
        continue;   // jamais descendu : antichaine par construction
      }
      if (node.left < 0) {
        for (int t = node.begin; t < node.end; ++t) {
          if (t == anchor_pos) continue;
          const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
          const i64 dx = (i64)w.x - p.x, dy = (i64)w.y - p.y, dz = (i64)w.z - p.z;
          const i64 d2 = dx * dx + dy * dy + dz * dz;
          if (d2 == 0) continue;
          ++receipt.bank_pops;
          i64 canon[3];
          const int point_chamber = chamber_of(dx, dy, dz, canon, false);
          Bank& bank = banks_[(std::size_t)point_chamber];
          if (bank.count >= kOrderK) continue;
          credit(bank, point_chamber, t, 1, d2);
        }
        continue;
      }
      heap.push({dist2_to_box(tree->nodes[(std::size_t)node.left], p), node.left});
      heap.push({dist2_to_box(tree->nodes[(std::size_t)node.right], p), node.right});
    }
    receipt.full_chambers += full;
    receipt.underfull_chambers += 48 - full;
  }

  // Une boite PEUT-elle contenir une direction de la chambre ? Rejet exact
  // par signe d'axe ou par ordre de magnitudes impossible ; le doute rend
  // vrai. Partage par la phase B des banques et l'enveloppe radiale.
  static bool box_may_touch_chamber(const Node& node, const mhgp::P3& p, int chamber) {
    static constexpr int kPermsTouch[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2},
                                              {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    const i64 pc[3] = {(i64)p.x, (i64)p.y, (i64)p.z};
    const int octant = chamber / 6;
    const int* perm = kPermsTouch[chamber % 6];
    i64 mag_lo[3], mag_hi[3];
    for (int k = 0; k < 3; ++k) {
      if (((octant >> k) & 1) == 0) {   // d >= 0
        if (node.hi[k] < pc[k]) return false;
        mag_lo[k] = std::max<i64>(0, node.lo[k] - pc[k]);
        mag_hi[k] = node.hi[k] - pc[k];
      } else {                          // d < 0 strict
        if (node.lo[k] >= pc[k]) return false;
        mag_lo[k] = std::max<i64>(1, pc[k] - node.hi[k]);
        mag_hi[k] = pc[k] - node.lo[k];
      }
    }
    if (mag_hi[perm[0]] < mag_lo[perm[1]]) return false;
    if (mag_hi[perm[1]] < mag_lo[perm[2]]) return false;
    return true;
  }

  static i64 dist2_to_box(const Node& node, const mhgp::P3& p) {
    i64 total = 0;
    const i64 c[3] = {(i64)p.x, (i64)p.y, (i64)p.z};
    for (int d = 0; d < 3; ++d) {
      if (c[d] < node.lo[d]) total += (node.lo[d] - c[d]) * (node.lo[d] - c[d]);
      else if (c[d] > node.hi[d]) total += (c[d] - node.hi[d]) * (c[d] - node.hi[d]);
    }
    return total;
  }

  // REMPLISSAGE DES BANQUES : best-first EXACT AU POINT — le tas porte des
  // noeuds (borne inferieure de boite) ET des points (distance carree
  // exacte) ; les points sont admis en ordre de distance non decroissante,
  // donc les dix premiers d'une chambre SONT ses dix plus proches et D est
  // minimal. C'est neanmoins une politique de TRAVAIL : une banque
  // sous-pleine ou un budget court n'autorisent aucune coupe (fail-open) et
  // ne peuvent creer aucun faux prune. dist^2 = 0 est exclu (mort-ne
  // documente en tete). Encodage du tas : idx >= 0 noeud, idx < 0 point de
  // position -idx-1.
  void fill_banks(int anchor_pos) {
    for (Bank& bank : banks_) {
      bank.count = 0;
      bank.d_max = 0;
    }
    near_list_.clear();
    zero_list_.clear();
    near_complete_d2_ = -1;
    const mhgp::P3& p = (*tree->points)[(std::size_t)tree->order[(std::size_t)anchor_pos]];
    using HeapItem = std::pair<i64, int>;
    std::priority_queue<HeapItem, std::vector<HeapItem>, std::greater<HeapItem>> heap;
    heap.push({0, 0});
    i64 pops = 0;
    int full = 0;
    // PHASE A (politique de travail fail-open) : best-first global budgete —
    // remplit vite les chambres denses. Les chambres lentes (cones presque
    // tangents a une nappe) sont remplies par la PHASE B dirigee ci-dessous;
    // les mesures a 12 500 ont montre qu'une patience globale les declarait
    // sous-pleines a tort et faisait exploser les survivantes (6,16M contre
    // 4,54M), tandis que le best-first sans borne coutait n pops par ancre.
    while (!heap.empty() && full < 48 && pops < bank_pop_budget) {
      receipt.heap_high_water = std::max(receipt.heap_high_water, (i64)heap.size());
      const HeapItem top = heap.top();
      heap.pop();
      if (top.second < 0) {
        // Un POINT en ordre exact de distance : router vers sa chambre.
        const int t = -top.second - 1;
        ++pops;
        ++receipt.bank_pops;
        near_list_.push_back({top.first, t});   // sous-produit trie gratuit
        const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
        const i64 dx = (i64)w.x - p.x, dy = (i64)w.y - p.y, dz = (i64)w.z - p.z;
        i64 canon[3];
        // Les banques emploient TOUJOURS la canonisation nominale : le
        // mutant perm-swapped ne desynchronise que la cible.
        const int chamber = chamber_of(dx, dy, dz, canon, false);
        Bank& bank = banks_[(std::size_t)chamber];
        if (bank.count >= kOrderK) continue;
        bank.ids[(std::size_t)bank.count] = tree->order[(std::size_t)t];
        bank.dist2[(std::size_t)bank.count] = top.first;
        ++bank.count;
        if (bank.count == kOrderK) {
          complete_bank(&bank, chamber, anchor_pos);
          ++full;
        }
        continue;
      }
      const Node& node = tree->nodes[(std::size_t)top.second];
      ++receipt.bank_node_visits;
      if (node.left >= 0) {
        heap.push({dist2_to_box(tree->nodes[(std::size_t)node.left], p), node.left});
        heap.push({dist2_to_box(tree->nodes[(std::size_t)node.right], p), node.right});
        continue;
      }
      for (int t = node.begin; t < node.end; ++t) {
        if (t == anchor_pos) continue;
        const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
        const i64 dx = (i64)w.x - p.x, dy = (i64)w.y - p.y, dz = (i64)w.z - p.z;
        const i64 d2 = dx * dx + dy * dy + dz * dz;
        if (d2 == 0) {
          zero_list_.push_back(t);   // contact ferme de TOUTE paire de p
          continue;                  // strictement > 0 exige pour la banque
        }
        heap.push({d2, -t - 1});
      }
    }
    // LA BORNE DE COMPLETUDE de la liste de voisins : tout point de distance
    // carree STRICTEMENT inferieure au minimum du tas restant a ete extrait.
    // ATTENTION : la completude des colocalises (zero_list_) exige que le
    // parcours ait VISITE toutes les feuilles contenant la coordonnee de p ;
    // le best-first les visite en premier (borne 0), donc des que le sommet
    // du tas est strictement positif, zero_list_ est complete.
    near_complete_d2_ = heap.empty() ? ((i64)1 << 62) : heap.top().first;
    // PHASE B : remplissage DIRIGE par cone des chambres encore sous-pleines.
    // La mesure a 12 500 (terrain) : ~21 chambres seulement sont remplissables
    // par ancre — les cones hors nappe sont vides — et le best-first global
    // sans borne coute n pops par ancre. La descente dirigee rejette un
    // sous-arbre par un certificat exact : signe incompatible sur un axe, ou
    // ordre de magnitudes impossible (max|d_i| < min|d_j| la ou la chambre
    // exige |d_i| >= |d_j|). Les cones vides s'epuisent sur la frontiere de
    // la nappe ; un budget de visites par cone reste une politique fail-open.
    if (full < 48) {
      // UNE SEULE passe best-first pour TOUTES les chambres sous-pleines
      // (le champ proche n'est paye qu'une fois) ; terminaison EXACTE : tas
      // epuise = toutes les chambres restantes prouvees sous-pleines. Le
      // budget `chamber_visits` reste un garde-fou fail-open, compte.
      const auto any_underfull_compatible = [&](const Node& node) {
        for (int chamber = 0; chamber < 48; ++chamber)
          if (banks_[(std::size_t)chamber].count < kOrderK &&
              box_may_touch_chamber(node, p, chamber))
            return true;
        return false;
      };
      std::priority_queue<HeapItem, std::vector<HeapItem>, std::greater<HeapItem>>
          cone_heap;
      cone_heap.push({0, 0});
      i64 visits = 0;
      while (!cone_heap.empty() && full < 48 && visits < chamber_visits) {
        const HeapItem top = cone_heap.top();
        cone_heap.pop();
        if (top.second < 0) {
          const int t = -top.second - 1;
          ++receipt.bank_pops;
          const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
          const i64 dx = (i64)w.x - p.x, dy = (i64)w.y - p.y, dz = (i64)w.z - p.z;
          i64 canon[3];
          const int chamber = chamber_of(dx, dy, dz, canon, false);
          Bank& bank = banks_[(std::size_t)chamber];
          if (bank.count >= kOrderK) continue;
          bool already = false;
          for (int i = 0; i < bank.count; ++i)
            if (bank.ids[(std::size_t)i] == tree->order[(std::size_t)t]) already = true;
          if (already) continue;   // la phase A a pu le classer ici
          bank.ids[(std::size_t)bank.count] = tree->order[(std::size_t)t];
          bank.dist2[(std::size_t)bank.count] = top.first;
          ++bank.count;
          if (bank.count == kOrderK) {
            complete_bank(&bank, chamber, anchor_pos);
            ++full;
          }
          continue;
        }
        const Node& node = tree->nodes[(std::size_t)top.second];
        ++visits;
        ++receipt.bank_cone_visits;
        if (!any_underfull_compatible(node)) continue;
        if (node.left >= 0) {
          cone_heap.push({dist2_to_box(tree->nodes[(std::size_t)node.left], p), node.left});
          cone_heap.push(
              {dist2_to_box(tree->nodes[(std::size_t)node.right], p), node.right});
          continue;
        }
        for (int t = node.begin; t < node.end; ++t) {
          if (t == anchor_pos) continue;
          const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
          const i64 dx = (i64)w.x - p.x, dy = (i64)w.y - p.y, dz = (i64)w.z - p.z;
          const i64 d2 = dx * dx + dy * dy + dz * dz;
          if (d2 == 0) continue;
          i64 canon[3];
          if (banks_[(std::size_t)chamber_of(dx, dy, dz, canon, false)].count >= kOrderK)
            continue;   // sa chambre est deja pleine : inutile de le porter
          cone_heap.push({d2, -t - 1});
        }
      }
    }
    receipt.full_chambers += full;
    receipt.underfull_chambers += 48 - full;
  }

  // LA COUPE STRICTE sur des minima canoniques (point : ses coordonnees).
  bool directional_cut(const Bank& bank, i64 x, i64 y, i64 z) const {
    if (bank.count < kOrderK) return false;   // fail-open
    const i64 d_max = bank.d_max;
    if (injections.strict_to_large)
      return x * x >= d_max && (x + y) * (x + y) >= 2 * d_max &&
             (x + y + z) * (x + y + z) >= 3 * d_max;
    return x * x > d_max && (x + y) * (x + y) > 2 * d_max &&
           (x + y + z) * (x + y + z) > 3 * d_max;
  }

  // Certification de chambre d'une BOITE relative a p : octant fixe (boite
  // entierement d'un cote strict, ou entierement >= p — coherent avec la
  // regle du point car alors tous les d partagent le signe canonique), et
  // permutation fixee par separation des plages de magnitudes (l'egalite de
  // frontiere n'est certifiee que si le tie-break d'axe rend le meme ordre).
  // Une boite indecise descend : c'est une politique de travail fail-open.
  bool box_chamber(const Node& node, const mhgp::P3& p, int* chamber_out,
                   i64 mins_out[3]) const {
    i64 mag_lo[3], mag_hi[3];
    int octant = 0;
    for (int k = 0; k < 3; ++k) {
      const i64 c = k == 0 ? (i64)p.x : (k == 1 ? (i64)p.y : (i64)p.z);
      if (node.lo[k] >= c) {
        mag_lo[k] = node.lo[k] - c;
        mag_hi[k] = node.hi[k] - c;
      } else if (node.hi[k] < c) {
        octant |= 1 << k;
        mag_lo[k] = c - node.hi[k];
        mag_hi[k] = c - node.lo[k];
      } else {
        return false;   // signes mixtes
      }
    }
    int perm[3] = {0, 1, 2};
    const auto certified_before = [&](int a, int b) {
      if (mag_lo[a] > mag_hi[b]) return true;
      if (mag_lo[a] == mag_hi[b] && a < b) return true;   // tie-break d'axe
      return false;
    };
    // Tri de certification a trois elements.
    for (int pass = 0; pass < 2; ++pass)
      for (int i = 0; i < 2; ++i)
        if (!certified_before(perm[i], perm[i + 1])) std::swap(perm[i], perm[i + 1]);
    if (!certified_before(perm[0], perm[1]) || !certified_before(perm[1], perm[2]))
      return false;   // ordre non certifiable sur toute la boite
    int code = 0;
    if (perm[0] == 0) code = perm[1] == 1 ? 0 : 1;
    else if (perm[0] == 1) code = perm[1] == 0 ? 2 : 3;
    else code = perm[1] == 0 ? 4 : 5;
    *chamber_out = octant * 6 + code;
    mins_out[0] = mag_lo[perm[0]];
    mins_out[1] = mag_lo[perm[1]];
    mins_out[2] = mag_lo[perm[2]];
    return true;
  }

  void record_yao_region(int anchor_pos, const Node& node, const Bank& bank) {
    if (yao_receipts == nullptr) return;
    YaoReceipt r;
    r.anchor_pos = anchor_pos;
    r.target_begin = node.begin;
    r.target_end = node.end;
    r.bank_index = bank.table_index;
    yao_receipts->push_back(r);
  }

  void record_yao_point(int anchor_pos, int target_pos, const Bank& bank) {
    if (yao_receipts == nullptr) return;
    YaoReceipt r;
    r.anchor_pos = anchor_pos;
    r.target_pos = target_pos;
    r.bank_index = bank.table_index;
    yao_receipts->push_back(r);
  }

  // LE PARCOURS DE COUPE : prefixe possede [0, anchor_pos), reçus de masse
  // sur les noeuds entierement possedes et certifies ; les survivantes sont
  // COLLECTEES puis classifiees PAR LOT a frontiere partagee (relancer la
  // racine par paire n'est pas une architecture admise — PROPOSITION).
  void prune_traverse(int anchor_pos) {
    const int anchor_id = tree->order[(std::size_t)anchor_pos];
    const mhgp::P3& p = (*tree->points)[(std::size_t)anchor_id];
    batch_.clear();
    anchor_regions_.clear();
    stack_.clear();
    stack_.push_back(0);
    while (!stack_.empty()) {
      receipt.stack_high_water = std::max(receipt.stack_high_water, (i64)stack_.size());
      const int node_index = stack_.back();
      stack_.pop_back();
      const Node& node = tree->nodes[(std::size_t)node_index];
      ++receipt.prune_node_visits;
      if (node.begin >= anchor_pos && !injections.ownership_doubled) continue;
      const bool fully_owned = node.end <= anchor_pos;
      if (fully_owned && !baseline) {
        int chamber;
        i64 mins[3];
        const bool single_chamber = box_chamber(node, p, &chamber, mins);
        if (!single_chamber && !injections.chamber_perm_swapped) {
          // L'ENVELOPPE RADIALE MULTI-CHAMBRE (audit de reemploi, §4) : une
          // boite A CHEVAL sur plusieurs chambres est prunee entiere si
          // toutes les banques de ses chambres compatibles sont pleines et
          // si dist^2(p, boite) > 3*max D_c — l'echec d'une coupe stricte
          // impliquerait dist^2 <= 3*D_c. Toute egalite descend ; une banque
          // sous-pleine parmi les chambres possibles interdit l'enveloppe.
          i64 dmax = -1;
          bool all_full = true;
          radial_scratch_.clear();
          for (int c = 0; c < 48; ++c) {
            if (!box_may_touch_chamber(node, p, c)) continue;
            const Bank& bank = banks_[(std::size_t)c];
            if (bank.count < kOrderK) {
              // MUTANT radial-forgets-chamber : une chambre compatible
              // sous-pleine serait ignoree au lieu d'interdire l'enveloppe.
              if (injections.radial_forgets_chamber) continue;
              all_full = false;
              break;
            }
            if (bank.d_max > dmax) dmax = bank.d_max;
            if (radial_receipts != nullptr) radial_scratch_.push_back(bank.table_index);
          }
          if (all_full && dmax >= 0 && dist2_to_box(node, p) > 3 * dmax) {
            ++receipt.radial_prunes;
            receipt.radial_pruned_mass += node.end - node.begin;
            receipt.region_pruned_mass += node.end - node.begin;
            anchor_regions_.push_back({node.begin, node.end});
            if (radial_receipts != nullptr) {
              RadialReceipt r;
              r.anchor_pos = anchor_pos;
              r.target_begin = node.begin;
              r.target_end = node.end;
              r.bank_indices = radial_scratch_;
              radial_receipts->push_back(std::move(r));
            }
            if (fate != nullptr)
              for (int t = node.begin; t < node.end; ++t)
                assign_pair(anchor_id, tree->order[(std::size_t)t], PairFate::kYaoRegion);
            continue;
          }
        }
        if (single_chamber) {
          // La cible d'un mutant perm-swapped n'est desynchronisee qu'au
          // POINT : la certification de boite reste nominale, le desync se
          // voit au niveau feuille (fixture region-prune, plancher arme).
          const Bank& bank = banks_[(std::size_t)chamber];
          if (!injections.chamber_perm_swapped &&
              directional_cut(bank, mins[0], mins[1], mins[2])) {
            ++receipt.region_prunes;
            receipt.region_pruned_mass += node.end - node.begin;
            anchor_regions_.push_back({node.begin, node.end});
            // MUTANT last-region-omitted : le reçu ET sa masse du DERNIER
            // noeud prune sont perdus — le ledger global ne ferme plus.
            if (injections.last_region_omitted) last_region_mass_ = node.end - node.begin;
            record_yao_region(anchor_pos, node, bank);
            if (fate != nullptr)
              for (int t = node.begin; t < node.end; ++t)
                assign_pair(anchor_id, tree->order[(std::size_t)t], PairFate::kYaoRegion);
            continue;
          }
        }
      }
      if (node.left < 0) {
        const int end = injections.ownership_doubled ? node.end
                                                     : std::min(node.end, anchor_pos);
        for (int t = node.begin; t < end; ++t) {
          if (t == anchor_pos) continue;
          const mhgp::P3& q = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
          const i64 dx = (i64)q.x - p.x, dy = (i64)q.y - p.y, dz = (i64)q.z - p.z;
          i64 canon[3];
          const int chamber =
              chamber_of(dx, dy, dz, canon, injections.chamber_perm_swapped);
          const Bank& bank = banks_[(std::size_t)chamber];
          if (!baseline && directional_cut(bank, canon[0], canon[1], canon[2])) {
            ++receipt.point_tombstones;
            record_yao_point(anchor_pos, t, bank);
            assign_pair(anchor_id, tree->order[(std::size_t)t], PairFate::kYaoPoint);
            continue;
          }
          ++receipt.survivors;
          SurvivorState state;
          state.target_pos = t;
          batch_.push_back(std::move(state));
        }
        continue;
      }
      stack_.push_back(node.right);
      stack_.push_back(node.left);
    }
    // Les intervalles de regions d'une ancre sont DISJOINTS (exigence de
    // reçu) : verification en mode oracle.
    if (oracle_mode && anchor_regions_.size() > 1) {
      std::sort(anchor_regions_.begin(), anchor_regions_.end());
      for (std::size_t k = 1; k < anchor_regions_.size(); ++k)
        if (anchor_regions_[k].first < anchor_regions_[k - 1].second)
          region_overlap_violated = true;
    }
  }

  i64 last_region_mass_ = 0;   // pour le mutant last-region-omitted

  // Bornes separables exactes de 4*Phi_{u,v} sur une boite : par axe le
  // minimum est au clip du sommet de la parabole, le maximum a une extremite.
  static void bounds4(const Node& node, const mhgp::P3& u, const mhgp::P3& v,
                      i64* inf4, i64* sup4) {
    i64 lo_total = 0, hi_total = 0;
    for (int d = 0; d < 3; ++d) {
      const i64 a = d == 0 ? (i64)u.x : (d == 1 ? (i64)u.y : (i64)u.z);
      const i64 b = d == 0 ? (i64)v.x : (d == 1 ? (i64)v.y : (i64)v.z);
      const i64 wl = node.lo[d], wh = node.hi[d];
      i64 t = a + b;
      if (t < 2 * wl) t = 2 * wl;
      if (t > 2 * wh) t = 2 * wh;
      lo_total += (t - 2 * a) * (t - 2 * b);
      const i64 pa = (2 * wl - a - b), pb = (2 * wh - a - b);
      const i64 dd = a - b;
      hi_total += std::max(pa * pa, pb * pb) - dd * dd;
    }
    *inf4 = lo_total;
    *sup4 = hi_total;
  }

  void mark_tombstone(SurvivorState* state, int anchor_id) {
    state->done = true;
    ++receipt.classifier_tombstones;
    assign_pair(tree->order[(std::size_t)state->target_pos], anchor_id,
                PairFate::kClassifierTomb);
  }

  // LE CLASSIFIEUR TERMINAL PAR LOT : toutes les survivantes d'une ancre
  // partagent UN parcours (frontiere partagee — chaque noeud n'est visite
  // qu'une fois, chaque survivante encore indecise y evalue ses bornes).
  // inf4 > 0 la retire du sous-arbre ; inf4 = 0 n'a aucun interieur strict
  // mais peut porter des CONTACTS que le census ferme doit compter (mutant
  // grave) ; sup4 < 0 credite le noeud entier (les extremites, a Phi = 0,
  // ne peuvent pas y etre) ; l'arret a dix stricts marque la tombstone.
  void classify_batch(int anchor_pos) {
    if (batch_.empty()) return;
    const int anchor_id = tree->order[(std::size_t)anchor_pos];
    const mhgp::P3& v = (*tree->points)[(std::size_t)anchor_id];
    const int limit = threshold();
    // LA VOIE LISTE : une survivante avec |pq|^2 strictement sous la borne de
    // completude se classifie par le prefixe trie de la liste de voisins —
    // tout temoin ferme verifie |w-p| <= |pq| — sans aucun parcours d'arbre.
    // Exactitude identique a la voie arbre (Phi exact par point) : couverte
    // par l'invariance des politiques. Les colocalises de l'ancre sont des
    // contacts fermes de toute paire ; le mutant census-skips-inf-zero les
    // saute ici aussi (meme faute : les contacts perdus).
    if (near_complete_d2_ > 0) {
      for (SurvivorState& state : batch_) {
        const mhgp::P3& u =
            (*tree->points)[(std::size_t)tree->order[(std::size_t)state.target_pos]];
        const i64 dx = (i64)u.x - v.x, dy = (i64)u.y - v.y, dz = (i64)u.z - v.z;
        const i64 pq2 = dx * dx + dy * dy + dz * dz;
        if (pq2 >= near_complete_d2_) continue;   // voie arbre
        // Prefixe court seulement (politique de travail) : au-dela, la boule
        // |pq| est plus volumineuse que ce que l'arbre elague par inf4 — la
        // voie arbre est meilleure. Le seuil ne change aucun sort.
        const std::size_t prefix =
            (std::size_t)(std::upper_bound(
                              near_list_.begin(), near_list_.end(),
                              std::pair<i64, int>{pq2, (int)((1u << 31) - 1)}) -
                          near_list_.begin());
        if (prefix > 128) continue;   // voie arbre
        state.resolved_by_list = true;
        ++receipt.classify_list_pairs;
        state.closed = 2;   // les extremites, fermees par definition
        if (oracle_mode) {
          state.closed_ids.push_back(anchor_id);
          state.closed_ids.push_back(tree->order[(std::size_t)state.target_pos]);
        }
        if (!injections.census_skips_inf_zero)
          for (int t : zero_list_) {
            if (t == state.target_pos) continue;
            ++state.closed;
            ++state.contacts;
            if (oracle_mode) state.closed_ids.push_back(tree->order[(std::size_t)t]);
          }
        for (const std::pair<i64, int>& item : near_list_) {
          if (item.first > pq2) break;
          const int t = item.second;
          if (t == state.target_pos) continue;
          ++receipt.classify_list_tests;
          const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
          i64 phi = 0;
          for (int d = 0; d < 3; ++d) {
            const i64 wc = d == 0 ? (i64)w.x : (d == 1 ? (i64)w.y : (i64)w.z);
            const i64 a = d == 0 ? (i64)u.x : (d == 1 ? (i64)u.y : (i64)u.z);
            const i64 b = d == 0 ? (i64)v.x : (d == 1 ? (i64)v.y : (i64)v.z);
            phi += (wc - a) * (wc - b);
          }
          if (phi < 0) {
            ++state.strict;
            ++state.closed;
            if (oracle_mode) state.closed_ids.push_back(tree->order[(std::size_t)t]);
            if (state.strict >= limit) {
              mark_tombstone(&state, anchor_id);
              break;
            }
          } else if (phi == 0 && !injections.census_skips_inf_zero) {
            ++state.closed;
            ++state.contacts;
            if (oracle_mode) state.closed_ids.push_back(tree->order[(std::size_t)t]);
          }
        }
      }
    }
    arena_.clear();
    for (int i = 0; i < (int)batch_.size(); ++i)
      if (!batch_[(std::size_t)i].done && !batch_[(std::size_t)i].resolved_by_list)
        arena_.push_back(i);
    if (arena_.empty()) {
      publish_batch(anchor_pos, anchor_id);
      return;
    }
    struct Frame {
      int node;
      int begin, end;   // plage active dans l'arene (append-only, partagee
                        // entre les deux enfants — jamais recopiee)
    };
    std::vector<Frame> frames;
    frames.push_back({0, 0, (int)arena_.size()});
    while (!frames.empty()) {
      const Frame frame = frames.back();
      frames.pop_back();
      receipt.stack_high_water =
          std::max(receipt.stack_high_water, (i64)frames.size() + 1);
      const Node& node = tree->nodes[(std::size_t)frame.node];
      ++receipt.classify_node_visits;
      const int next_begin = (int)arena_.size();
      for (int k = frame.begin; k < frame.end; ++k) {
        const int state_index = arena_[(std::size_t)k];
        SurvivorState& state = batch_[(std::size_t)state_index];
        if (state.done) continue;
        const mhgp::P3& u =
            (*tree->points)[(std::size_t)tree->order[(std::size_t)state.target_pos]];
        i64 inf4, sup4;
        bounds4(node, u, v, &inf4, &sup4);
        ++receipt.classify_box_tests;
        if (inf4 > 0) continue;
        if (inf4 == 0 && injections.census_skips_inf_zero) continue;
        if (sup4 < 0 && node.left >= 0) {
          state.strict += node.end - node.begin;
          state.closed += node.end - node.begin;
          if (oracle_mode)
            for (int t = node.begin; t < node.end; ++t)
              state.closed_ids.push_back(tree->order[(std::size_t)t]);
          if (state.strict >= limit) mark_tombstone(&state, anchor_id);
          continue;
        }
        arena_.push_back(state_index);
      }
      const int next_end = (int)arena_.size();
      if (next_begin == next_end) continue;
      if (node.left < 0) {
        for (int t = node.begin; t < node.end; ++t) {
          const mhgp::P3& w = (*tree->points)[(std::size_t)tree->order[(std::size_t)t]];
          const i64 wc[3] = {(i64)w.x, (i64)w.y, (i64)w.z};
          for (int k = next_begin; k < next_end; ++k) {
            SurvivorState& state = batch_[(std::size_t)arena_[(std::size_t)k]];
            if (state.done) continue;
            const mhgp::P3& u =
                (*tree->points)[(std::size_t)tree->order[(std::size_t)state.target_pos]];
            ++receipt.classify_point_tests;
            i64 phi = 0;
            for (int d = 0; d < 3; ++d) {
              const i64 a = d == 0 ? (i64)u.x : (d == 1 ? (i64)u.y : (i64)u.z);
              const i64 b = d == 0 ? (i64)v.x : (d == 1 ? (i64)v.y : (i64)v.z);
              phi += (wc[d] - a) * (wc[d] - b);
            }
            if (phi < 0) {
              ++state.strict;
              ++state.closed;
              if (oracle_mode) state.closed_ids.push_back(tree->order[(std::size_t)t]);
              if (state.strict >= limit) mark_tombstone(&state, anchor_id);
            } else if (phi == 0) {
              ++state.closed;
              // Les extremites sont fermees par definition ; les CONTACTS
              // publies sont l'extra-shell seul.
              if (t != state.target_pos && t != anchor_pos) ++state.contacts;
              if (oracle_mode) state.closed_ids.push_back(tree->order[(std::size_t)t]);
            }
          }
        }
        continue;
      }
      frames.push_back({node.right, next_begin, next_end});
      frames.push_back({node.left, next_begin, next_end});
    }
    publish_batch(anchor_pos, anchor_id);
  }

  void publish_batch(int anchor_pos, int anchor_id) {
    for (SurvivorState& state : batch_) {
      if (state.done) continue;
      ++receipt.census_records;
      receipt.census_closed_total += state.closed;
      receipt.census_strict_total += state.strict;
      receipt.census_contact_total += state.contacts;
      assign_pair(tree->order[(std::size_t)state.target_pos], anchor_id,
                  PairFate::kCensus);
      if (census_records != nullptr) {
        CensusRecord record;
        record.pos_low = state.target_pos;
        record.pos_high = anchor_pos;
        record.closed = state.closed;
        record.strict = state.strict;
        record.contacts = state.contacts;
        std::sort(state.closed_ids.begin(), state.closed_ids.end());
        record.closed_ids = std::move(state.closed_ids);
        census_records->push_back(std::move(record));
      }
    }
    batch_.clear();
  }

  // Le travail TOTAL comptabilise — le plafond est controle avant et apres
  // chaque unite (l'ancre) et couvre banques, tas, parcours, bornes, tests
  // et piles (exigence d'audit : jamais un OK apres depassement).
  i64 work_done() const {
    return receipt.bank_pops + receipt.bank_node_visits + receipt.bank_cone_visits +
           receipt.prune_node_visits + receipt.classify_node_visits +
           receipt.classify_box_tests + receipt.classify_point_tests +
           receipt.stack_high_water + receipt.heap_high_water;
  }

  // UNE ANCRE COMPLETE : banques, coupe, classification par lot, fermeture
  // par ancre. Rend faux sur budget depasse (refus atomique).
  bool run_anchor(int j) {
    ++receipt.anchors;
    if (max_work > 0 && work_done() > max_work) {
      budget_exceeded = true;
      return false;
    }
    const i64 mass_before = receipt.region_pruned_mass + receipt.point_tombstones +
                            receipt.survivors;
    near_complete_d2_ = -1;   // la liste de voisins appartient a UNE ancre
    if (!baseline) {
      if (antichain_banks) fill_banks_antichain(j);
      else fill_banks(j);
    }
    prune_traverse(j);
    classify_batch(j);
    // FERMETURE PAR ANCRE : la masse traitee de l'ancre j est exactement
    // pos(j) — l'egalite globale seule pourrait masquer une omission
    // compensee par un doublon (exigence d'audit).
    const i64 mass_after = receipt.region_pruned_mass + receipt.point_tombstones +
                           receipt.survivors;
    if (mass_after - mass_before != j) per_anchor_violated = true;
    if (max_work > 0 && work_done() > max_work) {
      budget_exceeded = true;
      return false;
    }
    return true;
  }

  // LE RUN COMPLET mono-thread (probe, oracle, differentiels).
  bool run() {
    const int n = (int)tree->order.size();
    for (int j = 0; j < n; ++j)
      if (!run_anchor(j)) return false;
    if (injections.last_region_omitted && receipt.region_prunes > 0) {
      --receipt.region_prunes;
      receipt.region_pruned_mass -= last_region_mass_;
    }
    return true;
  }
};

// ---------------------------------------------------------------------------
// LE JUGE INDEPENDANT (oracle, n <= 256) : arithmetique distincte
// 4*Phi = ||2x-u-v||^2 - ||u-v||^2 en i128, scan quadratique complet.
// ---------------------------------------------------------------------------

// LA FUSION DES REÇUS DE SHARDS : sommes partout, maxima pour les
// high-waters. EXHAUSTIVE champ par champ (l'audit a refuse une fusion qui
// omettait radial, antichaine et voie liste) ; la porte d'invariance
// 1/2/N threads la scelle.
inline void merge_receipts(SourceReceipt* into, const SourceReceipt& from) {
  into->anchors += from.anchors;
  into->bank_pops += from.bank_pops;
  into->bank_node_visits += from.bank_node_visits;
  into->bank_cone_visits += from.bank_cone_visits;
  into->full_chambers += from.full_chambers;
  into->underfull_chambers += from.underfull_chambers;
  into->prune_node_visits += from.prune_node_visits;
  into->region_prunes += from.region_prunes;
  into->region_pruned_mass += from.region_pruned_mass;
  into->radial_prunes += from.radial_prunes;
  into->radial_pruned_mass += from.radial_pruned_mass;
  into->antichain_nodes += from.antichain_nodes;
  into->point_tombstones += from.point_tombstones;
  into->survivors += from.survivors;
  into->classify_node_visits += from.classify_node_visits;
  into->classify_box_tests += from.classify_box_tests;
  into->classify_point_tests += from.classify_point_tests;
  into->classify_list_tests += from.classify_list_tests;
  into->classify_list_pairs += from.classify_list_pairs;
  into->classifier_tombstones += from.classifier_tombstones;
  into->census_records += from.census_records;
  into->census_closed_total += from.census_closed_total;
  into->census_strict_total += from.census_strict_total;
  into->census_contact_total += from.census_contact_total;
  into->stack_high_water = std::max(into->stack_high_water, from.stack_high_water);
  into->heap_high_water = std::max(into->heap_high_water, from.heap_high_water);
}

// L'EGALITE CHAMP PAR CHAMP de deux reçus (porte d'invariance par shards).
inline bool receipts_equal(const SourceReceipt& a, const SourceReceipt& b) {
  return a.anchors == b.anchors && a.bank_pops == b.bank_pops &&
         a.bank_node_visits == b.bank_node_visits &&
         a.bank_cone_visits == b.bank_cone_visits &&
         a.full_chambers == b.full_chambers &&
         a.underfull_chambers == b.underfull_chambers &&
         a.prune_node_visits == b.prune_node_visits &&
         a.region_prunes == b.region_prunes &&
         a.region_pruned_mass == b.region_pruned_mass &&
         a.radial_prunes == b.radial_prunes &&
         a.radial_pruned_mass == b.radial_pruned_mass &&
         a.antichain_nodes == b.antichain_nodes &&
         a.point_tombstones == b.point_tombstones && a.survivors == b.survivors &&
         a.classify_node_visits == b.classify_node_visits &&
         a.classify_box_tests == b.classify_box_tests &&
         a.classify_point_tests == b.classify_point_tests &&
         a.classify_list_tests == b.classify_list_tests &&
         a.classify_list_pairs == b.classify_list_pairs &&
         a.classifier_tombstones == b.classifier_tombstones &&
         a.census_records == b.census_records &&
         a.census_closed_total == b.census_closed_total &&
         a.census_strict_total == b.census_strict_total &&
         a.census_contact_total == b.census_contact_total &&
         a.stack_high_water == b.stack_high_water &&
         a.heap_high_water == b.heap_high_water;
}

// L'EXECUTION PAR SHARDS D'ANCRES (diagnostic warm_e2e) : chaque thread
// possede sa machine complete (banques, piles, arenes) et consomme des
// tranches d'ancres par compteur atomique. Les DECISIONS par ancre sont
// independantes : la partition des threads ne change ni un sort ni une
// masse, seule la somme des compteurs se recompose. Ni sorts par paire ni
// reçus : le mode oracle reste mono-thread.
struct ShardedOutcome {
  SourceReceipt receipt;
  bool per_anchor_violated = false;
};

inline ShardedOutcome run_sharded(const MortonLbvh& tree,
                                  const SourceInjections& injections, i64 bank_pops,
                                  i64 chamber_visits, int threads) {
  ShardedOutcome outcome;
  if (threads < 1) threads = 1;
  const int n = (int)tree.order.size();
  std::atomic<int> next_chunk{0};
  constexpr int kChunk = 64;
  std::vector<SourceReceipt> receipts((std::size_t)threads);
  std::vector<char> violated((std::size_t)threads, 0);
  std::vector<std::thread> pool;
  pool.reserve((std::size_t)threads);
  for (int w = 0; w < threads; ++w) {
    pool.emplace_back([&, w]() {
      YaoSource source;
      source.tree = &tree;
      source.injections = injections;
      source.bank_pop_budget = bank_pops;
      source.chamber_visits = chamber_visits;
      source.max_work = 0;   // le chemin quasi-produit n'a pas de budget
      for (;;) {
        const int begin = next_chunk.fetch_add(kChunk);
        if (begin >= n) break;
        const int end = std::min(n, begin + kChunk);
        for (int j = begin; j < end; ++j) source.run_anchor(j);
      }
      receipts[(std::size_t)w] = source.receipt;
      violated[(std::size_t)w] = source.per_anchor_violated ? 1 : 0;
    });
  }
  for (std::thread& t : pool) t.join();
  for (int w = 0; w < threads; ++w) {
    merge_receipts(&outcome.receipt, receipts[(std::size_t)w]);
    if (violated[(std::size_t)w]) outcome.per_anchor_violated = true;
  }
  return outcome;
}

}  // namespace yao48
}  // namespace mhgp3v
