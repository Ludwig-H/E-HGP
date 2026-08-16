// `PairFrame` — le tronc commun immuable du générateur sparse q2/q3/q4.
//
// Cadre : phase=exploration_v3_hors_registre, backend=cpu_reference,
//         profile=quantized_u16_input_only, mode=diagnostic_counter_only,
//         public_status=not_claimed.
//
// Specification : audits/AUDIT_POSITIF_DESCENTE_CIBLEE_PAIRFRAME_288032_20260816.md
// sections 8 et 9, et audits/AUDIT_CONSOLIDE_Q2_Q3_Q4_PAIR_MAJOR_APRES_79E73B6_20260816.md
// section 2.1.
//
// ---------------------------------------------------------------------------
// POURQUOI TROIS ETATS ET NON UN
//
// Le dossier impose l'autonomie des trois producteurs. Sont partageables : le
// `PointStore`, le LBVH, la partition neutre des paires, les AABB et les
// primitives geometriques pures. Ne le sont PAS : les verdicts, les caps, les
// continuations, les preuves de completude et les fates.
//
// Un `PairState` multi-lane dont q3 et q4 modifieraient la meme frontiere
// violerait cela, et surtout : une FERMETURE q4 ne doit jamais annuler du
// travail q3. Les fuseaux sont emboites — `W_4 < W_3 < W_2` — donc une paire
// morte en q4 peut parfaitement vivre en q3. Un kernel peut evaluer les trois
// predicats en une passe ; il doit ecrire dans trois etats separes.
//
// ---------------------------------------------------------------------------
// CE QUE LE LEDGER PORTE, ET CE QU'IL NE PORTE PLUS
//
// `upper_closed` a ete SUPERSEDE pour q3/q4 par l'audit `288032` section 5.2, et
// j'avais pose la question dans le bon sens : le shell est une notion **q2**,
// parce que c'est la seule lane ou le fuseau EST la miniboule du support. Pour
// q3/q4, `W_q` n'est qu'un cœur universel de prune ; son bord n'est pas la
// sphere finale du triangle ou du tetraedre.
//
// Le contrat est donc :
//
//   `CoreDepthLedger` q3/q4  : `lower_open`, `upper_open`
//   `BallCensusLedger` d'un support FIXE : interieur strict `I_B`, shell `U_B`
//
// Les compteurs sont SATURES a `h_q` : `min(h_q, lower_open)` suffit a decider,
// et cela evite de transporter des entiers dependant de `n` dans un etat GPU.
// La telemetrie brute vit a cote, jamais dans l'etat.
#ifndef MHGP3V_PROTOTYPE_PAIR_FRAME_HPP
#define MHGP3V_PROTOTYPE_PAIR_FRAME_HPP

#include <cstdint>
#include <vector>

namespace mhgp3v {
namespace pairframe {

using NodeHandle = int;   // `>= 0` nœud interne, `< 0` feuille `-1 - PointId trie`
using RectId = int;

// ---------------------------------------------------------------------------
// LE TRONC COMMUN. Il appartient a la partition neutre des paires et ne contient
// AUCUN fate de lane — c'est ce qui rend les trois lanes reellement autonomes.
struct PairFrame {
  RectId rect_id = -1;
  NodeHandle a_node = -1;
  NodeHandle b_node = -1;
  std::uint64_t pair_mass = 0;  // `|A| |B|`, la masse logique jamais expansee
};

// ---------------------------------------------------------------------------
// LES FATES DE CŒUR. Le vocabulaire est celui que l'audit impose, et il evite
// deux confusions que j'avais faites.
//
// `kCoreClear` n'est PAS « vivante exactement » : il prouve seulement qu'aucune
// paire du bloc n'est tuee par le cœur UNIVERSEL. Pour q3/q4 le census de la
// vraie circumboule reste obligatoire.
//
// `kPendingResource` n'est JAMAIS `kPruned` : un depassement de cap rend une
// continuation, pas un verdict.
enum class CoreFate : std::uint8_t {
  kMixedCore = 0,        // indecis : la frontiere doit encore etre raffinee
  kPrunedUniversal,      // `lower >= h_q` : toutes les paires du bloc sont mortes
  kCoreClear,            // `upper < h_q` : aucune n'est tuee par le cœur universel
  kExactify,             // masse assez petite : on developpe par tuiles
  kPendingResource       // cap atteint ; continuation serialisable, jamais un verdict
};

inline const char* core_fate_nom(CoreFate f) {
  switch (f) {
    case CoreFate::kMixedCore: return "MIXED_CORE";
    case CoreFate::kPrunedUniversal: return "PRUNED_BY_UNIVERSAL_DEPTH";
    case CoreFate::kCoreClear: return "CORE_CLEAR";
    case CoreFate::kExactify: return "EXACTIFY";
    case CoreFate::kPendingResource: return "PENDING_RESOURCE";
  }
  return "?";
}

// ---------------------------------------------------------------------------
// LES MUTANTS DE L'ABI. Chacun est une faute que j'ai reellement pu ecrire ;
// une porte doit le tuer, sinon la porte ne mesure rien.
//
//   `kSatureTronque`   tronque a huit bits au lieu de saturer a `h_q`.
//   `kClearLarge`      `upper <= h` au lieu de `<` : declare le bloc sain alors
//                      qu'une paire peut atteindre exactement le seuil. FAUTE
//                      DE SURETE — un support survit a tort.
//   `kPrunedLarge`     `lower > h` au lieu de `>=` : rate la mort a l'egalite.
//                      Conservateur en surete, mais il ne TERMINE plus : a
//                      pleine finesse `lower = upper = compte = h` n'est ni
//                      mort ni sain, et le bloc reste indecidable.
//   `kPendingPruned`   rend `PRUNED` sur depassement de cap. Le peche capital :
//                      une continuation devient un verdict.
//   `kCapAvantVerdict` teste le cap avant les deux verdicts. CONSERVATEUR : la
//                      tuile exacte redecide juste, donc le juge ne peut pas le
//                      voir. Il se tue au PLAFOND DE TRAVAIL, et il faut le
//                      dire ainsi plutot que de faire semblant.
enum class PfMutant : std::uint8_t {
  kNone = 0,
  kSatureTronque,
  kClearLarge,
  kPrunedLarge,
  kPendingPruned,
  kCapAvantVerdict
};

// ---------------------------------------------------------------------------
// LE LEDGER DE CŒUR. Sature a `reject_threshold`, donc borne independamment de
// `n` — c'est la propriete qui le rend transportable en registre GPU.
//
// `frontier` porte les spans encore indecis ; `relation_frontier` porte les
// spans ENDPOINT, ceux qui recouvrent `A` ou `B`. La distinction est le P0.1 de
// l'audit `79e73b6` : un endpoint n'est jamais credite au minorant, mais il est
// CONSERVE et rejoue apres toute restriction de `A` ou de `B`, parce qu'un `z`
// de `A` redevient un temoin ordinaire des que `A` ne le contient plus.
struct CoreDepthLedger {
  std::uint8_t reject_threshold = 0;   // `h_q = s_max - q + 1`
  std::uint8_t lower_open_sat = 0;     // `min(h_q, lower_open)`
  std::uint8_t upper_open_sat = 0;     // `min(h_q, upper_open)`
  // LA LARGEUR EST UN SCALAIRE, PAS UN `size()`. Un CTA ne materialise pas sa
  // frontiere pour savoir combien de spans il porte, et une politique qui
  // devrait appeler `size()` a chaque vague paierait `O(F)` la ou elle annonce
  // `O(1)`. Le vecteur reste pour la reference CPU et la continuation ; c'est
  // `frontier_width` que l'ordonnanceur lit.
  std::uint32_t frontier_width = 0;
  std::vector<NodeHandle> frontier;
  std::vector<NodeHandle> relation_frontier;
  std::uint64_t continuation_mass = 0;  // masse laissee indecise par un cap
  CoreFate fate = CoreFate::kMixedCore;

  void sature(long long lower, long long upper, PfMutant mu = PfMutant::kNone);
};

// La SATURATION EST UN THEOREME, pas une commodite. Pour tout `h >= 1` et tous
// `lower <= upper`, la decision — `lower >= h`, `upper < h`, sinon mixte — ne
// depend de `(lower, upper)` QUE par `(min(h,lower), min(h,upper))`. C'est ce
// qui autorise a ne transporter que deux octets au lieu de deux entiers
// dependant de `n`, et c'est verifie par la porte `mhgp3v_pairframe_saturation`
// sur des couples bruts allant jusqu'a `10^9`.
//
// Le mutant `sature-tronque` ecrit le compte brut tronque a huit bits au lieu
// de le saturer. Il survit a tout `n` petit et meurt des que `lower` franchit
// `256` : un bloc archi-mort redevient vivant. C'est exactement la faute que la
// saturation existe pour interdire.
inline void CoreDepthLedger::sature(long long lower, long long upper, PfMutant mu) {
  const long long h = reject_threshold;
  if (mu == PfMutant::kSatureTronque) {
    lower_open_sat = (std::uint8_t)(lower < 0 ? 0 : lower);
    upper_open_sat = (std::uint8_t)(upper < 0 ? 0 : upper);
    return;
  }
  lower_open_sat = (std::uint8_t)(lower > h ? h : (lower < 0 ? 0 : lower));
  upper_open_sat = (std::uint8_t)(upper > h ? h : (upper < 0 ? 0 : upper));
}

// ---------------------------------------------------------------------------
// LES ACTIONS DU SCHEDULER v0, dans l'ordre exact de la section 9 de l'audit.
//
// Une seule action est choisie par vague. On ne classifie PAS les enfants
// hypothetiques des trois candidats de scission pour en jeter deux : c'est la
// reponse a ma question Q3, et elle economise deux tiers du travail de decision.
enum class Action : std::uint8_t {
  kTerminal = 0,       // le fate est deja decide
  kExactifyTile,       // `pair_mass <= exact_tile_cap`
  kSplitWitness,       // un span mixte non-feuille existe et le budget le permet
  kSplitOneEndpoint,   // sinon, scinder `A` ou `B` — UNE seule fois par paire
  kPending             // ni l'un ni l'autre : continuation
};

inline const char* action_nom(Action a) {
  switch (a) {
    case Action::kTerminal: return "TERMINAL";
    case Action::kExactifyTile: return "EXACTIFY_TILE";
    case Action::kSplitWitness: return "SPLIT_WITNESS";
    case Action::kSplitOneEndpoint: return "SPLIT_ONE_ENDPOINT";
    case Action::kPending: return "PENDING";
  }
  return "?";
}

// La politique v0, litteralement celle de la section 9. Elle ne decide RIEN de
// geometrique : elle ordonne des verdicts deja calcules. C'est ce qui permet de
// graver l'invariance du resultat sous plusieurs politiques avant d'optimiser
// un score — l'heuristique du plus gros span n'entre ni dans la semantique, ni
// dans les cles persistantes.
// ---------------------------------------------------------------------------
// LE CAP PORTE SUR LA TUILE, PAS SUR LA MASSE — ET C'EST UNE CORRECTION.
//
// La section 9 pose `exact_tile_cap` sur `pair_mass`. Avec ce cap-la,
// `kPending` est INATTEIGNABLE, et cela se prouve en une ligne : `kPending`
// exige `pair_mass > cap`, aucun span mixte scindable, ET `endpoint_scindable`
// faux ; or un endpoint non scindable signifie `A` et `B` tous deux feuilles,
// donc `pair_mass = |A| |B| = 1 <= cap` pour tout `cap >= 1`. Contradiction.
//
// J'ai trouve cela en posant un PLANCHER de couverture sur la branche, pas en
// relisant le code : la porte `--politiques` a refuse de se valider elle-meme
// avec `pending_total=0`. Le plancher a fait exactement son travail.
//
// La ressource qui deborde vraiment sur GPU n'est pas la masse de la paire,
// c'est la LARGEUR DE FRONTIERE — le `kCapRacines` d'un CTA — et le cout d'une
// tuile exacte est `pair_mass * |frontiere|`. D'ou les deux caps :
//
//   `exact_tile_cap`  borne `pair_mass * |frontiere|`
//   `frontier_cap`    borne `|frontiere|`, donc interdit la scission de temoin
//
// Un bloc a paire singleton dont la frontiere est saturee et dont la tuile
// coûte plus que le cap ne peut alors ni se fermer, ni se scinder, ni
// s'exactifier : il rend une CONTINUATION, l'hote re-dispatche avec plus de
// place, et le resultat final est inchange. C'est le contrat du cap.
struct PolitiqueV0 {
  std::uint64_t exact_tile_cap = 64;
  std::uint64_t frontier_cap = 64;   // `kCapRacines` d'un CTA
  bool witness_budget_available = true;
  bool cap_sur_masse_seule = false;  // reproduit la section 9 a la lettre
};

inline bool est_pruned(const CoreDepthLedger& L, PfMutant mu = PfMutant::kNone) {
  return (mu == PfMutant::kPrunedLarge) ? (L.lower_open_sat > L.reject_threshold)
                                        : (L.lower_open_sat >= L.reject_threshold);
}

inline bool est_core_clear(const CoreDepthLedger& L, PfMutant mu = PfMutant::kNone) {
  return (mu == PfMutant::kClearLarge) ? (L.upper_open_sat <= L.reject_threshold)
                                       : (L.upper_open_sat < L.reject_threshold);
}

// Le coût d'une tuile exacte : autant d'evaluations que de paires fois la
// largeur de frontiere. C'est ce produit que le cap borne.
inline std::uint64_t cout_tuile(const CoreDepthLedger& L, const PairFrame& F,
                                const PolitiqueV0& p) {
  if (p.cap_sur_masse_seule) return F.pair_mass;
  const std::uint64_t w = (L.frontier_width == 0u) ? 1u : (std::uint64_t)L.frontier_width;
  return F.pair_mass * w;
}

inline Action choisir_action(const CoreDepthLedger& L, const PairFrame& F,
                             bool span_mixte_non_feuille, bool endpoint_scindable,
                             const PolitiqueV0& p, PfMutant mu = PfMutant::kNone) {
  const std::uint64_t cout = cout_tuile(L, F, p);
  if (mu == PfMutant::kCapAvantVerdict && cout <= p.exact_tile_cap)
    return Action::kExactifyTile;
  if (est_pruned(L, mu)) return Action::kTerminal;      // PRUNED_BY_UNIVERSAL_DEPTH
  if (est_core_clear(L, mu)) return Action::kTerminal;  // CORE_CLEAR
  if (cout <= p.exact_tile_cap) return Action::kExactifyTile;
  if (p.witness_budget_available && span_mixte_non_feuille &&
      (std::uint64_t)L.frontier_width < p.frontier_cap)
    return Action::kSplitWitness;
  if (endpoint_scindable) return Action::kSplitOneEndpoint;
  return Action::kPending;
}

// Le fate qui accompagne une action terminale.
inline CoreFate fate_terminal(const CoreDepthLedger& L, PfMutant mu = PfMutant::kNone) {
  if (est_pruned(L, mu)) return CoreFate::kPrunedUniversal;
  if (est_core_clear(L, mu)) return CoreFate::kCoreClear;
  return CoreFate::kMixedCore;
}

// L'ACTION DETERMINE LE FATE, ET C'EST ICI QUE LE PECHE EST EXPRIMABLE. Une
// action `kPending` rend `kPendingResource` : rien n'est decide, la masse part
// en continuation. Le mutant `pending-pruned` y rend `kPrunedUniversal`, ce qui
// transforme un manque de budget en preuve de mort.
inline CoreFate fate_apres_action(Action a, const CoreDepthLedger& L,
                                  PfMutant mu = PfMutant::kNone) {
  switch (a) {
    case Action::kTerminal: return fate_terminal(L, mu);
    case Action::kExactifyTile: return CoreFate::kExactify;
    case Action::kSplitWitness:
    case Action::kSplitOneEndpoint: return CoreFate::kMixedCore;
    case Action::kPending:
      return (mu == PfMutant::kPendingPruned) ? CoreFate::kPrunedUniversal
                                              : CoreFate::kPendingResource;
  }
  return CoreFate::kMixedCore;
}

// ---------------------------------------------------------------------------
// LA CONTINUATION TYPEE. Section 6.3 du contre-audit `a6171d` : un compteur
// n'est pas une continuation. Ce qui suit doit pouvoir etre ecrit, relu, et
// reprendre le calcul exactement la ou il s'est arrete.
//
// Elle conserve l'ANTICHAINE COMPLETE, pas seulement les spans mixtes : les
// spans deja decides portent le minorant, et les perdre reviendrait a
// recommencer. Elle conserve aussi la provenance (`PairFrame`) et la version de
// politique, parce qu'une reprise sous une autre politique doit rester
// comparable — le resultat ne depend pas de la politique, seul le travail en
// depend, et c'est precisement ce qu'une porte doit pouvoir verifier.
struct CoreContinuation {
  RectId rect_id = -1;
  NodeHandle a_node = -1, b_node = -1;
  std::uint8_t lane = 0;            // 2, 3 ou 4
  std::uint8_t threshold = 0;       // `h_q`
  std::uint8_t lower_open_sat = 0;
  std::uint8_t upper_open_sat = 0;
  std::uint32_t policy_version = 0;
  std::uint32_t cloud_epoch = 0;
  std::uint64_t pair_mass = 0;
  std::vector<NodeHandle> decided_spans;   // spans ALL : ils portent le minorant
  std::vector<NodeHandle> mixed_spans;     // spans encore indecis
  std::vector<NodeHandle> relation_spans;  // spans endpoint, rejoues apres restriction
};

// ---------------------------------------------------------------------------
// LES TROIS ETATS DE LANE. Ils ne partagent QUE le `PairFrame`.
struct Lane2State {
  CoreDepthLedger core;  // boule diametrale ; son bord EST le shell de la BallKey q2
};

struct Lane3State {
  CoreDepthLedger core;
  // La relation carrier de q3 est ENUMERATIVE : chaque porteur definit un
  // triangle distinct, donc aucun court-circuit existentiel n'est complet. Un
  // `ALL_CARRIER` y est un BLOC LOGIQUE de supports, pas une preuve d'existence.
  NodeHandle carrier_enumeration_root = -1;
};

struct Lane4State {
  CoreDepthLedger core;
  // ---- QUATRE CHOSES SEPAREES, ET LA SEPARATION EST LE POINT.
  //
  // Le premier porteur ACTIVE l'arete, mais il ne dispense jamais d'enumerer les
  // autres : chaque porteur peut encore produire un seed q4 distinct. Le
  // certificat d'existence peut donc etre annule, la racine d'enumeration
  // JAMAIS.
  int uniform_carrier_pid = -1;          // certificat d'existence, un VRAI PointId
  NodeHandle carrier_existence_frontier = -1;
  NodeHandle carrier_enumeration_root = -1;  // domaine complet, jamais perdu
  std::uint64_t jung_permanent_count = 0;    // interieurs permanents du cœur de Jung
};

}  // namespace pairframe
}  // namespace mhgp3v

#endif  // MHGP3V_PROTOTYPE_PAIR_FRAME_HPP
