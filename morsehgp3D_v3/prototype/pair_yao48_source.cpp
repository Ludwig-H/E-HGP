// MorseHGP3D v3 — LA SOURCE q2 CANDIDATE PRODUIT : MORTON/LBVH RESIDENT,
// COUPE YAO48 STRICTE FAIL-OPEN, CLASSIFICATION TERMINALE ET CENSUS FERME
// (NOTE_SOLUTION_SOURCE_Q2_YAO48_LBVH_U16_20260811, architecture de
// CATALOGUE_PAIRES_DIAMETRALES_EXACT.md respecialisee u16 : toute
// l'arithmetique decisive tient en i64, |Phi| < 3*2^34).
//
// LE THEOREME DE COUPE DIRECTIONNELLE (variante STRICTE, doc. catalogue §2) :
// pour une ancre p, une chambre canonique (8 octants x 6 permutations par
// magnitudes decroissantes), K temoins de PointId distincts de la chambre a
// distance carree <= D et STRICTEMENT > 0, et une cible q de coordonnees
// canoniques (x,y,z) dans la MEME chambre,
//
//     x^2 > D   et   (x+y)^2 > 2D   et   (x+y+z)^2 > 3D
//
// certifient Phi_{p,q}(w) < 0 pour les K temoins : K interieurs STRICTS
// distincts, q hors banque (x^2 > D >= dist^2 des temoins), p exclu
// (dist^2 > 0). A K = 10, la paire est tombstonee (p >= 10 donc p+q >= 12,
// bloc H0-inerte a K=10) SANS visite. Toute egalite descend ; l'echec du
// certificat ne classe RIEN (non-converse grave). Sur un noeud LBVH
// entierement contenu dans la chambre, les minima canoniques par axe donnent
// le meme certificat pour toutes ses feuilles : reçu de masse, zero paire
// materialisee.
//
// MUTANT MORT-NE DOCUMENTE (temoin colocalise avec l'ancre) : un temoin a
// dist^2 = 0 ne peut entrer que dans la chambre du vecteur nul (octant +++,
// permutation identite). Or une cible de cette chambre verifie d >= 0 par
// axe, donc domine l'ancre composante par composante, donc a une cle Morton
// superieure : elle n'est JAMAIS possedee par l'ancre — sauf colocalisee,
// auquel cas x = 0 et x^2 > D >= 0 ne passe jamais. La banque exclut
// dist^2 = 0 par defense en profondeur ; aucun mutant ne peut le rendre
// observable.
//
// LA CLASSIFICATION TERMINALE : chaque paire survivante (u,v) est classifiee
// par un parcours LBVH avec l'infimum/supremum separables exacts de
// 4*Phi sur les boites (les formules de clip recues de la lane self-join).
// inf4 > 0 ecarte le noeud ; inf4 = 0 n'a AUCUN interieur strict mais peut
// porter des CONTACTS : le census ferme doit y descendre (mutant grave) ;
// sup4 < 0 credite le noeud entier en stricts ; l'arret anticipe a dix
// stricts emet la tombstone, sinon le record publie le census ferme complet
// (rang, stricts, contacts, extremites comprises — Phi(u) = Phi(v) = 0).
//
// LE LEDGER (par ancre puis global) :
//     region_pruned_mass + point_tombstones + survivantes = positions possedees,
//     somme des possessions = C(n,2),
//     survivantes = tombstones_classifieur + records_census.
// Aucun tableau global de paires : le mode mesure compte, le mode oracle
// (n <= 256) tient les sorts pour le juge.
//
// LE JUGE INDEPENDANT (oracle) : scan quadratique complet, arithmetique
// VOLONTAIREMENT distincte (4*Phi = ||2x-u-v||^2 - ||u-v||^2 en i128, jamais
// la forme produit du sujet), sans Morton, sans LBVH, sans chambre. Il rejoue
// chaque reçu de prune temoin par temoin et compare TOUS les sorts, stricts,
// rangs et census.
//
// Codes : 0 OK ; 1 violation ; 2 CLI ; 3 plancher/budget ; 4 mutant tue.
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <queue>
#include <string>
#include <vector>

#include "mhgp/mhgp.hpp"
#include "prototype/cloud_families.hpp"
#include "prototype/morton_lbvh.hpp"

namespace {

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
  bool any() const {
    return strict_to_large || d_understated || ownership_doubled ||
           last_region_omitted || census_skips_inf_zero || threshold_minus_one ||
           chamber_perm_swapped;
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
  i64 point_tombstones = 0;      // cibles tombstonees au point par la coupe
  i64 survivors = 0;             // paires passees au classifieur
  i64 classify_node_visits = 0;  // noeuds visites par les parcours DE LOT
  i64 classify_box_tests = 0;    // evaluations de bornes par survivante
  i64 classify_point_tests = 0;
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

struct CensusRecord {
  int pos_low = -1, pos_high = -1;   // positions ; ids via order[]
  i64 closed = 0, strict = 0, contacts = 0;
  std::vector<int> closed_ids;       // mode oracle : liste fermee triee
};

constexpr int kOrderK = 10;   // contrat H0 : K = 10, seuil de tombstone

struct YaoSource {
  const Lbvh* tree = nullptr;
  SourceInjections injections;
  SourceReceipt receipt;
  i64 bank_pop_budget = 4096;    // budget de TRAVAIL par ancre (fail-open)
  i64 max_work = 0;              // budget global : depasse => refus atomique
  bool budget_exceeded = false;
  bool oracle_mode = false;
  bool baseline = false;         // differentiel : classification seule
  std::vector<PairFate>* fate = nullptr;
  int fate_points = 0;
  bool fate_violated = false;
  std::vector<YaoReceipt>* yao_receipts = nullptr;
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
    std::vector<int> closed_ids;
  };
  std::vector<SurvivorState> batch_;
  std::vector<int> arena_;
  // Fermeture PAR ANCRE du ledger (l'egalite globale seule peut masquer une
  // omission compensee par un doublon).
  bool per_anchor_violated = false;
  bool region_overlap_violated = false;
  std::vector<std::pair<int, int>> anchor_regions_;

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
        if (d2 == 0) continue;   // strictement > 0 exige (voir en-tete)
        heap.push({d2, -t - 1});
      }
    }
    // PHASE B : remplissage DIRIGE par cone des chambres encore sous-pleines.
    // La mesure a 12 500 (terrain) : ~21 chambres seulement sont remplissables
    // par ancre — les cones hors nappe sont vides — et le best-first global
    // sans borne coute n pops par ancre. La descente dirigee rejette un
    // sous-arbre par un certificat exact : signe incompatible sur un axe, ou
    // ordre de magnitudes impossible (max|d_i| < min|d_j| la ou la chambre
    // exige |d_i| >= |d_j|). Les cones vides s'epuisent sur la frontiere de
    // la nappe ; un budget de visites par cone reste une politique fail-open.
    static constexpr int kPerms[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2},
                                         {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    if (full < 48) {
      // UNE SEULE passe best-first pour TOUTES les chambres sous-pleines
      // (le champ proche n'est paye qu'une fois) ; terminaison EXACTE : tas
      // epuise = toutes les chambres restantes prouvees sous-pleines. Le
      // budget `chamber_visits` reste un garde-fou fail-open, compte.
      const i64 pc[3] = {(i64)p.x, (i64)p.y, (i64)p.z};
      // Compatibilite d'une boite avec le cone d'une chambre : intervalles
      // de magnitudes restreints au bon cote de p ; rejet exact si un axe
      // n'a aucun point du bon cote ou si l'ordre des magnitudes exige
      // l'impossible. Le doute descend.
      const auto box_compatible = [&](const Node& node, int chamber) {
        const int octant = chamber / 6;
        const int* perm = kPerms[chamber % 6];
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
      };
      const auto any_underfull_compatible = [&](const Node& node) {
        for (int chamber = 0; chamber < 48; ++chamber)
          if (banks_[(std::size_t)chamber].count < kOrderK &&
              box_compatible(node, chamber))
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
        if (box_chamber(node, p, &chamber, mins)) {
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
    arena_.clear();
    for (int i = 0; i < (int)batch_.size(); ++i) arena_.push_back(i);
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

  // LE RUN COMPLET. Rend faux sur budget global depasse (refus atomique).
  bool run() {
    const int n = (int)tree->order.size();
    for (int j = 0; j < n; ++j) {
      ++receipt.anchors;
      if (max_work > 0 && work_done() > max_work) {
        budget_exceeded = true;
        return false;
      }
      const i64 mass_before = receipt.region_pruned_mass + receipt.point_tombstones +
                              receipt.survivors;
      if (!baseline) fill_banks(j);
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
    }
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
inline int judge_phi_sign(const mhgp::P3& x, const mhgp::P3& u, const mhgp::P3& v) {
  i128 norm = 0, diam = 0;
  const i64 xs[3] = {(i64)x.x, (i64)x.y, (i64)x.z};
  const i64 us[3] = {(i64)u.x, (i64)u.y, (i64)u.z};
  const i64 vs[3] = {(i64)v.x, (i64)v.y, (i64)v.z};
  for (int d = 0; d < 3; ++d) {
    const i64 e = 2 * xs[d] - us[d] - vs[d];
    norm += (i128)e * e;
    const i64 dd = us[d] - vs[d];
    diam += (i128)dd * dd;
  }
  if (norm < diam) return -1;
  if (norm == diam) return 0;
  return 1;
}

mhgp::P3 pt(int x, int y, int z) {
  mhgp::P3 p{};
  p.x = (mhgp::i32)x;
  p.y = (mhgp::i32)y;
  p.z = (mhgp::i32)z;
  return p;
}

}  // namespace

int main(int argc, char** argv) {
  int n = 2400, coord = 0, leaf_size = 8, oracle = 0, differential = 0, permute = 0;
  int policy_differential = 0;
  i64 seed = 20260810, bank_pops = 4096, chamber_visits = 100000, max_work = 4000000000LL;
  i64 min_region_prunes = 0, min_point_tombstones = 0, min_census_records = 0;
  i64 min_underfull = 0, min_survivors = 0, min_classifier_tombs = 0;
  mhgp3v::CloudFamily family = mhgp3v::CloudFamily::kTerrain;
  std::string fixture_name;
  SourceInjections injections;
  auto integer = [](const char* text, i64* value) {
    const char* last = text + strlen(text);
    unsigned long long magnitude = 0;
    const auto r = std::from_chars(text, last, magnitude);
    if (text == last || r.ec != std::errc{} || r.ptr != last) return false;
    if (magnitude > 8000000000ULL) return false;
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
      if (!strcmp(argv[i], "strict-to-large")) injections.strict_to_large = true;
      else if (!strcmp(argv[i], "d-understated")) injections.d_understated = true;
      else if (!strcmp(argv[i], "ownership-doubled")) injections.ownership_doubled = true;
      else if (!strcmp(argv[i], "last-region-omitted"))
        injections.last_region_omitted = true;
      else if (!strcmp(argv[i], "census-skips-inf-zero"))
        injections.census_skips_inf_zero = true;
      else if (!strcmp(argv[i], "threshold-minus-one"))
        injections.threshold_minus_one = true;
      else if (!strcmp(argv[i], "chamber-perm-swapped"))
        injections.chamber_perm_swapped = true;
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
    else if (!strcmp(argv[i], "--bank-pops")) bank_pops = value;
    else if (!strcmp(argv[i], "--chamber-visits")) chamber_visits = value;
    else if (!strcmp(argv[i], "--max-work")) max_work = value;
    else if (!strcmp(argv[i], "--oracle")) oracle = (int)value;
    else if (!strcmp(argv[i], "--differential")) differential = (int)value;
    else if (!strcmp(argv[i], "--policy-differential")) policy_differential = (int)value;
    else if (!strcmp(argv[i], "--permute")) permute = (int)value;
    else if (!strcmp(argv[i], "--min-region-prunes")) min_region_prunes = value;
    else if (!strcmp(argv[i], "--min-point-tombstones")) min_point_tombstones = value;
    else if (!strcmp(argv[i], "--min-census-records")) min_census_records = value;
    else if (!strcmp(argv[i], "--min-underfull")) min_underfull = value;
    else if (!strcmp(argv[i], "--min-survivors")) min_survivors = value;
    else if (!strcmp(argv[i], "--min-classifier-tombstones")) min_classifier_tombs = value;
    else { std::printf("ECHEC : argument inconnu %s\n", argv[i]); return 2; }
    ++i;
  }
  if (n < 4 || n > 100000 || coord < 0 || coord > 65536 || leaf_size < 2 ||
      leaf_size > 256 || bank_pops < 48 || chamber_visits < 8 || max_work < 1 ||
      oracle < 0 || oracle > 1 || policy_differential < 0 || policy_differential > 1 ||
      differential < 0 || differential > 1 || permute < 0 || permute > 1) {
    std::printf("ECHEC : campagne absurde\n");
    return 2;
  }

  const bool is_fixture = !fixture_name.empty();
  std::vector<mhgp::P3> pts;
  if (is_fixture) {
    if (fixture_name == "directional-equality") {
      // L'EGALITE DIRECTIONNELLE EXACTE (verifiee en entiers hors bande) :
      // ancre p = (60000,60000,60000), Morton max par dominance composante
      // par composante. Banque de la chambre (---, x>=y>=z) : neuf temoins
      // proches stricts + le temoin (5,5,5) a dist^2 = 75 = D, PARALLELE a
      // (1,1,1). Cible q = p - (9,4,2) : x^2 = 81 > 75 et (x+y)^2 = 169 >
      // 150 passent, (x+y+z)^2 = 225 = 3D EXACTEMENT — la coupe stricte
      // descend. Verite : 9 stricts + le contact (5,5,5) (Phi = 75-75 = 0),
      // donc AUCUNE tombstone n'est licite : le mutant strict-to-large prune
      // et le juge le tue.
      pts = {pt(60000, 60000, 60000), pt(59991, 59996, 59998), pt(59995, 59995, 59995),
             pt(59996, 59997, 59999), pt(59996, 59998, 59999), pt(59995, 59997, 59998),
             pt(59995, 59996, 59999), pt(59994, 59997, 59998), pt(59994, 59996, 59999),
             pt(59996, 59997, 59998), pt(59995, 59998, 59999), pt(59994, 59998, 59999)};
    } else if (fixture_name == "region-prune") {
      // LE REÇU DE REGION (verifie en entiers hors bande) : dix temoins
      // proches (D = 1526) et un cluster lointain de huit cibles dans la
      // meme chambre, dont la boite passe la coupe stricte sur ses minima
      // canoniques — huit paires tombstonees par UN reçu de masse. Le mutant
      // chamber-perm-swapped desynchronise la cible et perd le prune.
      pts = {pt(60000, 60000, 60000)};
      const int wit[10][3] = {{30, 2, 1}, {31, 2, 1}, {32, 2, 1}, {33, 2, 1}, {34, 2, 1},
                              {35, 2, 1}, {36, 2, 1}, {37, 2, 1}, {38, 2, 1}, {39, 2, 1}};
      for (const auto& w : wit) pts.push_back(pt(60000 - w[0], 60000 - w[1], 60000 - w[2]));
      for (int k = 0; k < 8; ++k)
        pts.push_back(pt(60000 - (3000 + 7 * k), 60000 - (40 + 3 * k), 60000 - (20 + 2 * k)));
    } else if (fixture_name == "underfull") {
      // LE FAIL-OPEN EXERCE : six points epars — toutes les banques restent
      // sous-pleines, aucune coupe, toutes les paires au classifieur.
      pts = {pt(100, 100, 100), pt(60000, 200, 300), pt(300, 60000, 400),
             pt(400, 500, 60000), pt(50000, 50000, 100), pt(200, 40000, 40000)};
    } else if (fixture_name == "nonconverse") {
      // LE NON-CONVERSE GRAVE (doc. catalogue §2) : l'echec de la coupe ne
      // classe rien. Trois points, w EXACTEMENT sur la sphere diametrale de
      // (p,q) : census ferme 3, stricts 0, contact 1.
      pts = {pt(200, 200, 200), pt(198, 200, 200), pt(199, 199, 200)};
    } else if (fixture_name == "u16-extremes") {
      // LES EXTREMES u16 : diagonale complete + dix-huit temoins stricts en
      // neuf paires antipodales (la fixture skew de la lane profondeur) —
      // la paire diagonale est tombstonee par le CLASSIFIEUR (18 stricts).
      pts = {pt(0, 0, 0), pt(65535, 65535, 65535)};
      const int skew[18][3] = {
          {65535, 0, 32768},     {0, 65535, 32767},     {0, 32768, 65535},
          {65535, 32767, 0},     {32768, 65535, 0},     {32767, 0, 65535},
          {65535, 32768, 0},     {0, 32767, 65535},     {32768, 0, 65535},
          {32767, 65535, 0},     {0, 65535, 32768},     {65535, 0, 32767},
          {49151, 65535, 1},     {16384, 0, 65534},     {1, 16384, 65535},
          {65534, 49151, 0},     {65535, 16384, 49152}, {0, 49151, 16383}};
      for (const auto& s : skew) pts.push_back(pt(s[0], s[1], s[2]));
    } else if (fixture_name == "colocated") {
      // Vingt PointId a la meme coordonnee : aucune coupe possible (toutes
      // les distances nulles), toutes les paires en census de rang 20 avec
      // zero strict — les contacts sont TOUS les autres points.
      for (int j = 0; j < 20; ++j) pts.push_back(pt(12345, 23456, 34567));
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
    if ((int)pts.size() != n) {
      std::printf("ECHEC : contrat de cardinalite viole — %zu points rendus pour %d"
                  " demandes\n", pts.size(), n);
      return 1;
    }
    std::printf("provenance : --points %d --coord %d --seed %lld --family %s"
                " --leaf-size %d --bank-pops %lld\n", n, coord, seed,
                mhgp3v::cloud_family_name(family), leaf_size, bank_pops);
  }
  if (oracle == 1 && n > 256) {
    std::printf("ECHEC : l'oracle exhaustif exige n <= 256\n");
    return 2;
  }
  if (differential == 1 && n > 3000) {
    std::printf("ECHEC : le differentiel bi-mode exige n <= 3000\n");
    return 2;
  }
  if (policy_differential == 1 && n > 3000) {
    std::printf("ECHEC : le differentiel de politiques exige n <= 3000\n");
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
  const i64 all_pairs = (i64)n * (n - 1) / 2;

  std::vector<PairFate> fates;
  std::vector<YaoReceipt> receipts;
  std::vector<BankTableEntry> bank_table;
  std::vector<CensusRecord> census;
  YaoSource source;
  source.tree = &tree;
  source.injections = injections;
  source.bank_pop_budget = bank_pops;
  source.chamber_visits = chamber_visits;
  source.max_work = max_work;
  if (oracle == 1 || differential == 1 || policy_differential == 1) {
    fates.assign((std::size_t)all_pairs, PairFate::kUnassigned);
    source.fate = &fates;
    source.fate_points = n;
    if (oracle == 1) {
      source.oracle_mode = true;
      source.yao_receipts = &receipts;
      source.bank_table = &bank_table;
      source.census_records = &census;
    }
  }
  const auto t0 = std::chrono::steady_clock::now();
  if (!source.run()) {
    std::printf("ECHEC : budget de travail global depasse — la source refuse, elle ne"
                " tronque pas\n");
    return 3;
  }
  const auto t1 = std::chrono::steady_clock::now();
  const double seconds = std::chrono::duration<double>(t1 - t0).count();
  const SourceReceipt& r = source.receipt;
  if (source.fate_violated)
    return fail("le ledger de sorts", "une paire a recu deux sorts — multiplicite violee");
  if (source.per_anchor_violated)
    return fail("le ledger par ancre",
                "la masse traitee d'une ancre differe de pos(j) — une omission"
                " compensee par un doublon devient visible ici");
  if (source.region_overlap_violated)
    return fail("les reçus de region", "deux intervalles d'une meme ancre se recouvrent");

  // LE LEDGER GLOBAL : reçus de region + tombstones ponctuelles +
  // survivantes = C(n,2), et survivantes = tombstones classifieur + census.
  if (r.region_pruned_mass + r.point_tombstones + r.survivors != all_pairs)
    return fail("l'identite du ledger",
                "masse prunee + tombstones + survivantes != C(n,2)");
  if (r.classifier_tombstones + r.census_records != r.survivors)
    return fail("l'identite du ledger", "survivantes != tombstones + census");
  if (oracle == 1 || differential == 1)
    for (std::size_t k = 0; k < fates.size(); ++k)
      if (fates[k] == PairFate::kUnassigned)
        return fail("le ledger de sorts", "une paire sans sort — partition violee");

  // LE DIFFERENTIEL BI-MODE : la baseline classifie TOUTES les paires sans
  // aucune coupe ; les sorts agreges tombstone/census doivent coincider.
  if (differential == 1) {
    std::vector<PairFate> base_fates((std::size_t)all_pairs, PairFate::kUnassigned);
    YaoSource base;
    base.tree = &tree;
    base.baseline = true;
    base.max_work = max_work;
    base.fate = &base_fates;
    base.fate_points = n;
    if (!base.run()) {
      std::printf("ECHEC : budget depasse dans la baseline\n");
      return 3;
    }
    if (base.fate_violated) {
      std::printf("ECHEC de la baseline : sort double\n");
      return 1;
    }
    for (std::size_t k = 0; k < fates.size(); ++k) {
      const bool subject_tomb = fate_is_tombstone(fates[k]);
      const bool base_tomb = fate_is_tombstone(base_fates[k]);
      if (subject_tomb != base_tomb)
        return fail("le differentiel bi-mode",
                    "un sort tombstone/census differe de la baseline sans coupe");
    }
    std::printf("differentiel : sorts IDENTIQUES — baseline visites=%lld tests=%lld ;"
                " sujet visites=%lld tests=%lld (regions=%lld masse=%lld points=%lld)\n",
                base.receipt.classify_node_visits, base.receipt.classify_point_tests,
                r.classify_node_visits, r.classify_point_tests, r.region_prunes,
                r.region_pruned_mass, r.point_tombstones);
  }

  // L'ORACLE : rejeu des reçus puis differentiel exhaustif du juge.
  if (oracle == 1) {
    // 1. Chaque reçu Yao : dix PointId distincts, hors extremites, et
    // Phi < 0 rejoue par l'arithmetique DISTINCTE du juge pour chaque paire.
    for (const YaoReceipt& receipt_item : receipts) {
      const int anchor_id = tree.order[(std::size_t)receipt_item.anchor_pos];
      std::vector<int> targets;
      if (receipt_item.target_pos >= 0) targets.push_back(receipt_item.target_pos);
      else
        for (int t = receipt_item.target_begin; t < receipt_item.target_end; ++t)
          targets.push_back(t);
      // La banque FACTORISEE du reçu : (ancre, chambre, version) — dix
      // identifiants references une seule fois, jamais recopies par noeud.
      if (receipt_item.bank_index < 0 ||
          receipt_item.bank_index >= (int)bank_table.size())
        return fail("le rejeu des reçus Yao48", "un reçu ne reference aucune banque");
      const BankTableEntry& bank_entry = bank_table[(std::size_t)receipt_item.bank_index];
      if (bank_entry.anchor_pos != receipt_item.anchor_pos)
        return fail("le rejeu des reçus Yao48",
                    "un reçu reference la banque d'une autre ancre");
      std::array<int, 10> ids = bank_entry.ids;
      std::sort(ids.begin(), ids.end());
      for (int i = 1; i < 10; ++i)
        if (ids[(std::size_t)i] == ids[(std::size_t)(i - 1)])
          return fail("le rejeu des reçus Yao48", "temoin duplique dans un reçu");
      for (int target_pos : targets) {
        const int target_id = tree.order[(std::size_t)target_pos];
        for (int witness_id : ids) {
          if (witness_id == anchor_id || witness_id == target_id)
            return fail("le rejeu des reçus Yao48", "temoin egal a une extremite");
          if (judge_phi_sign(pts[(std::size_t)witness_id], pts[(std::size_t)anchor_id],
                             pts[(std::size_t)target_id]) >= 0)
            return fail("le rejeu des reçus Yao48",
                        "un temoin de reçu n'est pas strictement interieur");
        }
      }
    }
    // 2. Le juge exhaustif : sort et census de CHAQUE paire.
    std::size_t census_cursor = 0;
    std::vector<CensusRecord> census_sorted = census;
    std::sort(census_sorted.begin(), census_sorted.end(),
              [&](const CensusRecord& a, const CensusRecord& b) {
                int a_lo = tree.order[(std::size_t)a.pos_low];
                int a_hi = tree.order[(std::size_t)a.pos_high];
                if (a_lo > a_hi) std::swap(a_lo, a_hi);
                int b_lo = tree.order[(std::size_t)b.pos_low];
                int b_hi = tree.order[(std::size_t)b.pos_high];
                if (b_lo > b_hi) std::swap(b_lo, b_hi);
                if (a_lo != b_lo) return a_lo < b_lo;
                return a_hi < b_hi;
              });
    for (int i = 0; i < n; ++i)
      for (int j = i + 1; j < n; ++j) {
        i64 strict = 0, closed = 0;
        std::vector<int> judge_closed;
        for (int x = 0; x < n; ++x) {
          const int sign =
              judge_phi_sign(pts[(std::size_t)x], pts[(std::size_t)i], pts[(std::size_t)j]);
          if (sign < 0) { ++strict; ++closed; judge_closed.push_back(x); }
          else if (sign == 0) { ++closed; judge_closed.push_back(x); }
        }
        const i64 index = (i64)i * (2 * (i64)n - i - 1) / 2 + (j - i - 1);
        const PairFate fate_value = fates[(std::size_t)index];
        // Le juge n'herite JAMAIS des injections : seuil NOMINAL — c'est lui
        // qui tue threshold-minus-one.
        const bool judge_tomb = strict >= kOrderK;
        if (judge_tomb != fate_is_tombstone(fate_value))
          return fail("le juge exhaustif", "un sort machine contredit le juge");
        if (fate_value == PairFate::kCensus) {
          // Le record census suivant dans l'ordre canonique doit etre
          // exactement cette paire, avec la meme liste fermee.
          if (census_cursor >= census_sorted.size())
            return fail("le juge exhaustif", "un record census manque");
          const CensusRecord& record = census_sorted[census_cursor++];
          int lo = tree.order[(std::size_t)record.pos_low];
          int hi = tree.order[(std::size_t)record.pos_high];
          if (lo > hi) std::swap(lo, hi);
          if (lo != i || hi != j)
            return fail("le juge exhaustif", "l'ordre canonique des census diverge");
          if (record.strict != strict || record.closed != (i64)judge_closed.size() ||
              record.closed_ids != judge_closed)
            return fail("le juge exhaustif",
                        "un census ferme differe de la liste du juge");
        }
      }
    if (census_cursor != census_sorted.size())
      return fail("le juge exhaustif", "des records census surnumeraires existent");
    // 3. L'EQUIVARIANCE PAR PERMUTATIONS (l'audit exige PLUSIEURS ordres) :
    // le renversement ET un melange LCG grave rendent les memes sorts apres
    // renommage.
    if (permute == 1) {
      std::vector<std::vector<int>> sigmas;
      {
        std::vector<int> reversed_sigma((std::size_t)n);
        for (int i = 0; i < n; ++i) reversed_sigma[(std::size_t)i] = n - 1 - i;
        sigmas.push_back(std::move(reversed_sigma));
        std::vector<int> shuffled((std::size_t)n);
        for (int i = 0; i < n; ++i) shuffled[(std::size_t)i] = i;
        unsigned long long lcg = 0x9E3779B97F4A7C15ULL ^ (unsigned long long)seed;
        for (int i = n - 1; i > 0; --i) {
          lcg = lcg * 6364136223846793005ULL + 1442695040888963407ULL;
          const int j = (int)(lcg % (unsigned long long)(i + 1));
          std::swap(shuffled[(std::size_t)i], shuffled[(std::size_t)j]);
        }
        sigmas.push_back(std::move(shuffled));
      }
      for (const std::vector<int>& sigma : sigmas) {
        std::vector<mhgp::P3> permuted((std::size_t)n);
        for (int i = 0; i < n; ++i)
          permuted[(std::size_t)i] = pts[(std::size_t)sigma[(std::size_t)i]];
        Lbvh tree2;
        tree2.build(permuted, leaf_size);
        std::vector<PairFate> fates2((std::size_t)all_pairs, PairFate::kUnassigned);
        YaoSource source2;
        source2.tree = &tree2;
        source2.injections = injections;
        source2.bank_pop_budget = bank_pops;
        source2.chamber_visits = chamber_visits;
        source2.max_work = max_work;
        source2.fate = &fates2;
        source2.fate_points = n;
        if (!source2.run()) {
          std::printf("ECHEC : budget depasse dans la permutation\n");
          return 3;
        }
        for (int i = 0; i < n; ++i)
          for (int j = i + 1; j < n; ++j) {
            const i64 index2 = (i64)i * (2 * (i64)n - i - 1) / 2 + (j - i - 1);
            int oi = sigma[(std::size_t)i], oj = sigma[(std::size_t)j];
            if (oi > oj) std::swap(oi, oj);
            const i64 oindex = (i64)oi * (2 * (i64)n - oi - 1) / 2 + (oj - oi - 1);
            if (fate_is_tombstone(fates2[(std::size_t)index2]) !=
                fate_is_tombstone(fates[(std::size_t)oindex]))
              return fail("l'equivariance par permutation",
                          "un sort depend de la numerotation des PointId");
          }
      }
    }
  }

  // L'INVARIANCE DES POLITIQUES DE TRAVAIL (exigence d'audit) : les budgets
  // minimal et ample rendent les MEMES sorts et les memes agregats de census
  // — la coupe est une acceleration, jamais une autorite. Les sorts sont
  // objectifs (dix stricts existent ou non) et le classifieur exact rattrape
  // toute coupe manquee : toute divergence est un defaut.
  if (policy_differential == 1) {
    std::vector<PairFate> fates_min((std::size_t)all_pairs, PairFate::kUnassigned);
    YaoSource source_min;
    source_min.tree = &tree;
    source_min.injections = injections;
    source_min.bank_pop_budget = 48;
    source_min.chamber_visits = 8;
    source_min.max_work = max_work;
    source_min.fate = &fates_min;
    source_min.fate_points = n;
    if (!source_min.run()) {
      std::printf("ECHEC : budget depasse dans la politique minimale\n");
      return 3;
    }
    if (source_min.receipt.census_records != r.census_records ||
        source_min.receipt.census_closed_total != r.census_closed_total ||
        source_min.receipt.census_strict_total != r.census_strict_total ||
        source_min.receipt.census_contact_total != r.census_contact_total)
      return fail("l'invariance des politiques",
                  "les agregats de census dependent du budget des banques");
    for (std::size_t k = 0; k < fates_min.size(); ++k)
      if (fate_is_tombstone(fates_min[k]) != fate_is_tombstone(fates[k]))
        return fail("l'invariance des politiques",
                    "un sort depend du budget des banques");
    std::printf("politiques : sorts et census IDENTIQUES entre budgets minimal"
                " (48/8) et ample (%lld/%lld)\n", bank_pops, chamber_visits);
  }

  // LES PLANCHERS ANTI VERT-PAR-VACUITE.
  const auto floor_violated = [&](const char* what, i64 got, i64 want) {
    std::printf("ECHEC : plancher viole — %s=%lld < %lld\n", what, got, want);
    return 3;
  };
  if (min_region_prunes > 0 && r.region_prunes < min_region_prunes)
    return floor_violated("reçus-de-region", r.region_prunes, min_region_prunes);
  if (min_point_tombstones > 0 && r.point_tombstones < min_point_tombstones)
    return floor_violated("tombstones-ponctuelles", r.point_tombstones,
                          min_point_tombstones);
  if (min_census_records > 0 && r.census_records < min_census_records)
    return floor_violated("records-census", r.census_records, min_census_records);
  if (min_underfull > 0 && r.underfull_chambers < min_underfull)
    return floor_violated("chambres-sous-pleines", r.underfull_chambers, min_underfull);
  if (min_survivors > 0 && r.survivors < min_survivors)
    return floor_violated("survivantes", r.survivors, min_survivors);
  if (min_classifier_tombs > 0 && r.classifier_tombstones < min_classifier_tombs)
    return floor_violated("tombstones-classifieur", r.classifier_tombstones,
                          min_classifier_tombs);

  // LES ASSERTIONS DE FIXTURE.
  if (is_fixture) {
    const auto fate_of = [&](int i, int j) {
      if (i > j) std::swap(i, j);
      const i64 index = (i64)i * (2 * (i64)n - i - 1) / 2 + (j - i - 1);
      return fates[(std::size_t)index];
    };
    if (fixture_name == "directional-equality") {
      // Honnetete : l'egalite (x+y+z)^2 = 3D doit etre exacte et le temoin
      // (5,5,5) un contact exact de la paire (0,1).
      if (judge_phi_sign(pts[2], pts[0], pts[1]) != 0)
        return fail("la fixture directional-equality",
                    "le temoin parallele n'est plus un contact exact — fixture derivee");
      if (fate_of(0, 1) != PairFate::kCensus)
        return fail("la fixture directional-equality",
                    "la paire d'egalite exacte a ete tombstonee — l'egalite doit"
                    " descendre");
      if (r.point_tombstones != 0 || r.region_prunes != 0)
        return fail("la fixture directional-equality",
                    "une coupe a mordu alors que la seule paire coupable est en"
                    " egalite exacte");
    } else if (fixture_name == "region-prune") {
      if (r.region_prunes < 1)
        return fail("la fixture region-prune",
                    "aucun reçu de region — la coupe de boite ne mord plus");
      i64 cluster_tombs = 0;
      for (int k = 11; k < 19; ++k)
        if (fate_is_tombstone(fate_of(0, k))) ++cluster_tombs;
      if (cluster_tombs != 8)
        return fail("la fixture region-prune",
                    "les huit paires du cluster ne sont pas toutes tombstonees");
    } else if (fixture_name == "underfull") {
      if (r.underfull_chambers == 0 || r.region_prunes != 0 || r.point_tombstones != 0)
        return fail("la fixture underfull",
                    "le fail-open des banques sous-pleines n'est pas exerce");
      if (r.census_records != all_pairs)
        return fail("la fixture underfull",
                    "toutes les paires devraient finir en census");
    } else if (fixture_name == "nonconverse") {
      if (fate_of(0, 1) != PairFate::kCensus)
        return fail("la fixture nonconverse", "la paire (p,q) n'est pas un record");
      bool found = false;
      for (const CensusRecord& record : census) {
        int lo = tree.order[(std::size_t)record.pos_low];
        int hi = tree.order[(std::size_t)record.pos_high];
        if (lo > hi) std::swap(lo, hi);
        if (lo == 0 && hi == 1) {
          found = true;
          if (record.closed != 3 || record.strict != 0 || record.contacts != 1)
            return fail("la fixture nonconverse",
                        "le census de (p,q) doit etre ferme=3, stricts=0, contact=1");
        }
      }
      if (!found)
        return fail("la fixture nonconverse", "le record de (p,q) est introuvable");
    } else if (fixture_name == "u16-extremes") {
      if (!fate_is_tombstone(fate_of(0, 1)))
        return fail("la fixture u16-extremes",
                    "la paire diagonale n'est pas tombstonee malgre 18 stricts");
    } else if (fixture_name == "colocated") {
      if (r.region_prunes != 0 || r.point_tombstones != 0 ||
          r.classifier_tombstones != 0)
        return fail("la fixture colocated",
                    "une tombstone a ete fabriquee sur des points colocalises");
      if (r.census_records != all_pairs)
        return fail("la fixture colocated", "chaque paire colocalisee doit etre un"
                                            " record de rang n");
    }
  }

  if (injections.any()) {
    std::printf("MUTANT SURVIVANT : aucune porte n'a mordu\n");
    return 0;
  }

  const i64 tree_bytes =
      (i64)(tree.nodes.size() * sizeof(Node) + tree.order.size() * (sizeof(int) + 8));
  std::printf("arbre      : %zu noeuds, feuilles <= %d, %lld octets (cles Morton"
              " comprises)\n", tree.nodes.size(), leaf_size, tree_bytes);
  std::printf("banques    : pops=%lld visites=%lld cone-visites=%lld pleines=%lld"
              " sous-pleines=%lld (budget %lld par ancre, tas max=%lld)\n", r.bank_pops,
              r.bank_node_visits, r.bank_cone_visits, r.full_chambers,
              r.underfull_chambers, bank_pops, r.heap_high_water);
  std::printf("coupe      : visites=%lld reçus-region=%lld (masse %lld)"
              " tombstones-point=%lld\n", r.prune_node_visits, r.region_prunes,
              r.region_pruned_mass, r.point_tombstones);
  std::printf("classifieur: survivantes=%lld visites-lot=%lld boites=%lld tests=%lld"
              " tombstones=%lld census=%lld\n", r.survivors, r.classify_node_visits,
              r.classify_box_tests, r.classify_point_tests, r.classifier_tombstones,
              r.census_records);
  std::printf("census     : fermes=%lld stricts=%lld contacts=%lld — pile max=%lld\n",
              r.census_closed_total, r.census_strict_total, r.census_contact_total,
              r.stack_high_water);
  std::printf("ledger     : %lld + %lld + %lld = %lld = C(n,2) — FERME (%.3f s de"
              " phase locale, 1 thread, pas un warm_e2e)\n", r.region_pruned_mass,
              r.point_tombstones, r.survivors, all_pairs, seconds);
  if (oracle == 1)
    std::printf("oracle     : reçus rejoues et juge exhaustif compare — aucun"
                " desaccord\n");
  if (is_fixture)
    std::printf("OK : fixture %s recue\n", fixture_name.c_str());
  else
    std::printf("OK : source q2 Yao48/LBVH count-only — le ledger est la sortie,"
                " aucune admission n'est prononcee\n");
  return 0;
}
