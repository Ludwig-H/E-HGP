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
//   `kUpperSatureIncr`  maintient un MAJORANT SATURE de facon incrementale au
//                       lieu de la masse exacte. Faute de surete, et c'est le
//                       § 3.3 de l'arbitrage : une fois `upper` ecrete a `h_q`,
//                       il ne peut plus redescendre juste. Vrai majorant `100`,
//                       stocke `10` ; la masse chute de `5`, vrai `95`, stocke
//                       `5 < h` — `CORE_CLEAR` est declare sur un bloc dont une
//                       paire atteint le seuil.
enum class PfMutant : std::uint8_t {
  kNone = 0,
  kSatureTronque,
  kClearLarge,
  kPrunedLarge,
  kPendingPruned,
  kCapAvantVerdict,
  kUpperSatureIncr,
  kCoutTuileLargeur   // cap sur `pair_mass * largeur` : le P0 de `08b7007`
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
  // LA MASSE EST EXACTE, LE MAJORANT EST DERIVE. Saturer `lower` est sûr — un
  // etat qui l'atteint est terminal, il n'en redescend jamais. Saturer `upper`
  // ne l'est PAS : apres une scission qui prouve beaucoup de `NONE`, il faut
  // pouvoir redescendre de `h_q` vers une valeur strictement inferieure, ce
  // qu'une valeur ecretee ne permet plus. C'est le § 3.3 de l'arbitrage, et le
  // mutant `upper-sature-incremental` montre que la faute est de SURETE.
  std::uint32_t frontier_candidate_mass_exact = 0;
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

  // `upper_open_sat = min(h_q, lower_open_sat + masse)`. Derive, jamais stocke.
  std::uint8_t upper_open_sat() const {
    const std::uint32_t h = reject_threshold;
    const std::uint64_t u = (std::uint64_t)lower_open_sat + frontier_candidate_mass_exact;
    return (std::uint8_t)(u > h ? h : u);
  }

  void sature(long long lower, long long masse, PfMutant mu = PfMutant::kNone);

  // Le majorant sature stocke que le mutant maintient a la place de la masse.
  // Il n'existe QUE pour rendre la faute du § 3.3 exprimable et executable.
  std::uint8_t upper_sat_mutant = 0;
  std::uint8_t upper_effectif(PfMutant mu) const {
    return (mu == PfMutant::kUpperSatureIncr) ? upper_sat_mutant : upper_open_sat();
  }
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
inline void CoreDepthLedger::sature(long long lower, long long masse, PfMutant mu) {
  const long long h = reject_threshold;
  const long long m = masse < 0 ? 0 : masse;
  if (mu == PfMutant::kSatureTronque) {
    lower_open_sat = (std::uint8_t)(lower < 0 ? 0 : lower);
    frontier_candidate_mass_exact = (std::uint32_t)(std::uint8_t)m;
    return;
  }
  lower_open_sat = (std::uint8_t)(lower > h ? h : (lower < 0 ? 0 : lower));
  frontier_candidate_mass_exact = (std::uint32_t)m;
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
//
// `witness_batch` autorise `SPLIT_WITNESS_BATCH(etat, spans[0:B])` : remplacer
// simultanement plusieurs parents mixtes deux a deux incomparables preserve
// l'antichaine, la partition de masse et l'invariant `L <= N <= U`, et ne
// depend d'aucun ordre entre les scissions (§ 5 de l'arbitrage). L'ABI doit
// l'autoriser des maintenant : sans lui, une famille a `Theta(n)` scissions
// imposerait `Theta(n)` synchronisations globales sur GPU.
struct PolitiqueV0 {
  std::uint64_t exact_tile_cap = 64;
  std::uint64_t frontier_cap = 64;   // `kCapRacines` d'un CTA
  std::uint32_t witness_batch = 1;   // `B` ; `0` signifie « tous les mixtes »
  bool witness_budget_available = true;
  bool cap_sur_masse_seule = false;  // reproduit la section 9 a la lettre
};

inline bool est_pruned(const CoreDepthLedger& L, PfMutant mu = PfMutant::kNone) {
  return (mu == PfMutant::kPrunedLarge) ? (L.lower_open_sat > L.reject_threshold)
                                        : (L.lower_open_sat >= L.reject_threshold);
}

inline bool est_core_clear(const CoreDepthLedger& L, PfMutant mu = PfMutant::kNone) {
  const std::uint8_t u = L.upper_effectif(mu);
  return (mu == PfMutant::kClearLarge) ? (u <= L.reject_threshold)
                                       : (u < L.reject_threshold);
}

// ---------------------------------------------------------------------------
// LE COUT D'UNE TUILE EXACTE SE COMPTE EN EVALUATIONS PONCTUELLES, PAS EN
// HANDLES. C'est le P0 de l'audit `08b7007`, et le contre-exemple est direct :
//
//   `pair_mass = 64`, UN span mixte de population `256`, `frontier_width = 1`,
//   `exact_tile_cap = 64`.
//
// Un cap sur `pair_mass * frontier_width` calcule `64` et accepte la tuile ; la
// boucle execute `64 * 256 = 16 384` lectures. Le cap est depasse d'un facteur
// egal a la POPULATION du span, soit jusqu'a `50 000` sur la cible. Ce n'est pas
// une objection asymptotique : la fonction de coût et la boucle qu'elle pretend
// borner ne parlaient pas de la meme chose.
//
// Les spans `ALL` n'ont pas a etre rescannes : l'exactificateur initialise
// `compte = lower` et ne parcourt que les candidats mixtes. Le coût ponctuel
// vaut alors EXACTEMENT `pair_mass * frontier_candidate_mass_exact`.
//
// DEUX RESSOURCES, DEUX CAPS. `frontier_cap` borne les handles residents ;
// `exact_tile_cap` borne les evaluations ponctuelles. Les confondre etait la
// faute.
inline std::uint64_t evaluations_tuile(const CoreDepthLedger& L, const PairFrame& F) {
  return F.pair_mass * (std::uint64_t)L.frontier_candidate_mass_exact;
}

// La comparaison passe par une DIVISION, jamais par le produit : une ABI de cap
// n'a aucune raison d'introduire un debordement la ou une division suffit.
inline bool tuile_tient(std::uint64_t pair_mass, std::uint32_t masse_candidate,
                        std::uint64_t cap) {
  if (masse_candidate == 0u) return true;   // rien de mixte : la tuile est libre
  return pair_mass <= cap / (std::uint64_t)masse_candidate;
}

inline bool cout_tuile_tient(const CoreDepthLedger& L, const PairFrame& F,
                             const PolitiqueV0& p, PfMutant mu = PfMutant::kNone) {
  if (p.cap_sur_masse_seule) return F.pair_mass <= p.exact_tile_cap;
  if (mu == PfMutant::kCoutTuileLargeur) {
    const std::uint64_t w = (L.frontier_width == 0u) ? 1u : (std::uint64_t)L.frontier_width;
    return F.pair_mass * w <= p.exact_tile_cap;  // la faute du P0, rejouee
  }
  return tuile_tient(F.pair_mass, L.frontier_candidate_mass_exact, p.exact_tile_cap);
}

inline Action choisir_action(const CoreDepthLedger& L, const PairFrame& F,
                             bool span_mixte_non_feuille, bool endpoint_scindable,
                             const PolitiqueV0& p, PfMutant mu = PfMutant::kNone) {
  const bool tient = cout_tuile_tient(L, F, p, mu);
  if (mu == PfMutant::kCapAvantVerdict && tient) return Action::kExactifyTile;
  if (est_pruned(L, mu)) return Action::kTerminal;      // PRUNED_BY_UNIVERSAL_DEPTH
  if (est_core_clear(L, mu)) return Action::kTerminal;  // CORE_CLEAR
  if (tient) return Action::kExactifyTile;
  if (p.witness_budget_available && span_mixte_non_feuille &&
      (std::uint64_t)L.frontier_width < p.frontier_cap)
    return Action::kSplitWitness;
  if (endpoint_scindable) return Action::kSplitOneEndpoint;
  return Action::kPending;
}

// ---------------------------------------------------------------------------
// LE LOT EFFECTIF. Un budget booleen ne suffit plus des que l'action est
// batchee : `budget = 1` avec `witness_batch = 8` produisait huit scissions et
// un budget negatif. Le batch est mathematiquement sûr — les parents d'une
// antichaine sont deux a deux incomparables —, mais le CONTRAT DE RESSOURCE ne
// l'etait pas. C'est le P1 de l'audit `08b7007`.
//
// `B = 0` signifie « tous les spans scindables », borne par le budget restant.
struct ActionRecord {
  Action kind = Action::kPending;
  std::uint32_t witness_count = 0;
};

inline std::uint32_t lot_effectif(std::uint32_t demande, long long budget_restant,
                                  std::uint32_t spans_scindables) {
  if (budget_restant <= 0 || spans_scindables == 0u) return 0u;
  std::uint32_t b = (demande == 0u) ? spans_scindables : demande;
  if (b > spans_scindables) b = spans_scindables;
  if ((long long)b > budget_restant) b = (std::uint32_t)budget_restant;
  return b;
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
  // La MASSE EXACTE, pas un majorant ecrete — meme raison qu'au ledger.
  std::uint32_t frontier_candidate_mass_exact = 0;
  // `policy_version` est enregistre pour la reproductibilite des PERFORMANCES.
  // Il n'entre ni dans la correction ni dans la semantique du record : une
  // continuation capee sous une politique doit se reprendre sous une autre et
  // rendre les memes identites. C'est le § 4.2 de l'arbitrage, et la porte
  // `mhgp3v_pairframe_reprise_politique_croisee` l'exige.
  std::uint32_t policy_version = 0;
  std::uint32_t schema_version = 1;
  std::uint32_t cloud_epoch = 0;
  std::uint64_t pair_mass = 0;
  std::vector<NodeHandle> decided_spans;   // spans ALL : ils portent le minorant
  std::vector<NodeHandle> mixed_spans;     // spans encore indecis
  std::vector<NodeHandle> relation_spans;  // spans endpoint, rejoues apres restriction
};

// ---------------------------------------------------------------------------
// LE CODEC FAIL-CLOSED. Section 4 de l'audit `08b7007`.
//
// L'aller-retour sur tampon VALIDE ne prouve rien contre un tampon tronque ou
// un champ de taille corrompu : le parseur precedent lisait `na` et `nm` sans
// preflight, donc un entier fabrique provoquait une lecture hors limites au lieu
// d'un refus type. `ValidContinuationRoundTrip-v0` n'est pas
// `FailClosedContinuationCodec-v1`, et il fallait le dire.
//
// Le decodeur ci-dessous PREFLIGHTE tout avant la moindre lecture utile ou
// allocation : magic, schema, taille d'en-tete, taille de charge utile, somme de
// controle, puis cardinalites contre les octets restants. Ensuite seulement il
// valide la SEMANTIQUE — domaine des handles, antichaine, doublons, disjonction
// des trois plages, masse recalculee, seuil, epoque — et refuse tout octet
// residuel.
enum class DecodeError : std::uint8_t {
  kOk = 0,
  kTropCourt,          // moins que l'en-tete
  kMagic,              // signature absente
  kSchema,             // version de schema inconnue de l'execution
  kEnTete,             // `header_bytes` incoherent
  kLongueur,           // `payload_bytes` ne colle pas au tampon
  kSomme,              // somme de controle
  kCardinalite,        // `na`/`nm`/`nr` ne tiennent pas dans la charge utile
  kHandleHorsDomaine,  // un `NodeHandle` sort de l'arbre actif
  kDoublon,            // un span apparait deux fois
  kAncetre,            // un span est ancetre d'un autre : ce n'est plus une antichaine
  kMasse,              // la masse stockee ne vaut pas la masse recalculee
  kSeuil,              // `lower_open_sat` a deja atteint `h_q` : l'etat serait terminal
  kEpoque,             // epoque, lane ou rectangle etrangers a l'execution
  kResidu              // octets finaux inattendus
};

inline const char* decode_error_nom(DecodeError e) {
  switch (e) {
    case DecodeError::kOk: return "OK";
    case DecodeError::kTropCourt: return "TROP_COURT";
    case DecodeError::kMagic: return "MAGIC";
    case DecodeError::kSchema: return "SCHEMA";
    case DecodeError::kEnTete: return "EN_TETE";
    case DecodeError::kLongueur: return "LONGUEUR";
    case DecodeError::kSomme: return "SOMME";
    case DecodeError::kCardinalite: return "CARDINALITE";
    case DecodeError::kHandleHorsDomaine: return "HANDLE_HORS_DOMAINE";
    case DecodeError::kDoublon: return "DOUBLON";
    case DecodeError::kAncetre: return "ANCETRE";
    case DecodeError::kMasse: return "MASSE";
    case DecodeError::kSeuil: return "SEUIL";
    case DecodeError::kEpoque: return "EPOQUE";
    case DecodeError::kResidu: return "RESIDU";
  }
  return "?";
}

constexpr std::uint32_t kContinuationMagic = 0x50464331u;  // "PFC1"
constexpr std::uint32_t kContinuationSchema = 1u;
constexpr std::uint32_t kContinuationHeaderBytes = 44u;

// CRC32 sans table : court, deterministe, et suffisant pour detecter une
// corruption de champ. Ce n'est pas une signature — l'audit demande une somme
// de controle, pas une authentification.
inline std::uint32_t crc32(const std::uint8_t* p, std::size_t n) {
  std::uint32_t c = 0xffffffffu;
  for (std::size_t i = 0; i < n; ++i) {
    c ^= p[i];
    for (int k = 0; k < 8; ++k) c = (c >> 1) ^ (0xedb88320u & (0u - (c & 1u)));
  }
  return ~c;
}

// L'appelant fournit le domaine : le codec ne connait ni l'arbre ni le nuage, et
// c'est voulu — il ne doit pas pouvoir croire sur parole ce qu'il decode.
struct ContexteDecodage {
  std::uint32_t nb_noeuds = 0;      // domaine des spans temoins
  // Les extremites `A`/`B` viennent de la partition des PAIRES, les spans de
  // l'arbre des TEMOINS : deux domaines distincts, et les confondre relacherait
  // la validation du plus petit des deux. `0` signifie « meme domaine ».
  std::uint32_t nb_endpoints = 0;
  std::uint32_t cloud_epoch = 0;
  std::uint8_t threshold = 0;
  const void* arbre = nullptr;
  std::uint32_t (*population)(const void*, NodeHandle) = nullptr;
  bool (*est_ancetre_ou_egal)(const void*, NodeHandle, NodeHandle) = nullptr;
};

inline void pf_ecrire32(std::vector<std::uint8_t>* b, std::uint32_t v) {
  for (int i = 0; i < 4; ++i) b->push_back((std::uint8_t)((v >> (8 * i)) & 0xffu));
}
inline std::uint32_t pf_lire32(const std::uint8_t* b, std::size_t o) {
  std::uint32_t v = 0;
  for (int i = 0; i < 4; ++i) v |= (std::uint32_t)b[o + (std::size_t)i] << (8 * i);
  return v;
}

inline std::vector<std::uint8_t> encode_continuation(const CoreContinuation& k) {
  std::vector<std::uint8_t> charge;
  pf_ecrire32(&charge, (std::uint32_t)k.rect_id);
  pf_ecrire32(&charge, (std::uint32_t)k.a_node);
  pf_ecrire32(&charge, (std::uint32_t)k.b_node);
  pf_ecrire32(&charge, (std::uint32_t)k.lane);
  pf_ecrire32(&charge, (std::uint32_t)k.lower_open_sat);
  pf_ecrire32(&charge, k.frontier_candidate_mass_exact);
  pf_ecrire32(&charge, k.policy_version);
  pf_ecrire32(&charge, (std::uint32_t)(k.pair_mass & 0xffffffffu));
  pf_ecrire32(&charge, (std::uint32_t)(k.pair_mass >> 32));
  pf_ecrire32(&charge, (std::uint32_t)k.decided_spans.size());
  pf_ecrire32(&charge, (std::uint32_t)k.mixed_spans.size());
  pf_ecrire32(&charge, (std::uint32_t)k.relation_spans.size());
  for (std::size_t i = 0; i < k.decided_spans.size(); ++i)
    pf_ecrire32(&charge, (std::uint32_t)k.decided_spans[i]);
  for (std::size_t i = 0; i < k.mixed_spans.size(); ++i)
    pf_ecrire32(&charge, (std::uint32_t)k.mixed_spans[i]);
  for (std::size_t i = 0; i < k.relation_spans.size(); ++i)
    pf_ecrire32(&charge, (std::uint32_t)k.relation_spans[i]);

  std::vector<std::uint8_t> b;
  pf_ecrire32(&b, kContinuationMagic);
  pf_ecrire32(&b, kContinuationSchema);
  pf_ecrire32(&b, kContinuationHeaderBytes);
  pf_ecrire32(&b, (std::uint32_t)charge.size());
  pf_ecrire32(&b, 0u);
  pf_ecrire32(&b, k.cloud_epoch);
  pf_ecrire32(&b, 0u);
  pf_ecrire32(&b, (std::uint32_t)k.threshold);
  pf_ecrire32(&b, 0u);
  pf_ecrire32(&b, 0u);
  pf_ecrire32(&b, charge.empty() ? 0u : crc32(&charge[0], charge.size()));
  b.insert(b.end(), charge.begin(), charge.end());
  return b;
}

inline DecodeError decode_continuation(const std::uint8_t* buf, std::size_t n,
                                       const ContexteDecodage& c,
                                       CoreContinuation* out) {
  // ---- PREFLIGHT. Rien n'est lu au-dela de ce qui a ete prouve present.
  if (buf == nullptr || n < kContinuationHeaderBytes) return DecodeError::kTropCourt;
  if (pf_lire32(buf, 0) != kContinuationMagic) return DecodeError::kMagic;
  if (pf_lire32(buf, 4) != kContinuationSchema) return DecodeError::kSchema;
  if (pf_lire32(buf, 8) != kContinuationHeaderBytes) return DecodeError::kEnTete;
  const std::uint32_t charge_octets = pf_lire32(buf, 12);
  if ((std::uint64_t)kContinuationHeaderBytes + charge_octets != (std::uint64_t)n)
    return DecodeError::kLongueur;
  if (charge_octets % 4u != 0u) return DecodeError::kLongueur;
  const std::uint8_t* charge = buf + kContinuationHeaderBytes;
  const std::uint32_t somme = pf_lire32(buf, 40);
  if ((charge_octets == 0u ? 0u : crc32(charge, charge_octets)) != somme)
    return DecodeError::kSomme;
  if (pf_lire32(buf, 20) != c.cloud_epoch) return DecodeError::kEpoque;
  if (pf_lire32(buf, 28) != (std::uint32_t)c.threshold) return DecodeError::kSeuil;
  const std::uint32_t mots = charge_octets / 4u;
  if (mots < 12u) return DecodeError::kCardinalite;
  const std::uint32_t na = pf_lire32(charge, 36), nm = pf_lire32(charge, 40),
                      nr = pf_lire32(charge, 44);
  // Les cardinalites sont confrontees aux octets RESTANTS avant toute
  // allocation : c'est le seul moment ou un entier fabrique est inoffensif.
  const std::uint64_t attendus = 12ull + (std::uint64_t)na + nm + nr;
  if (attendus != (std::uint64_t)mots) return DecodeError::kResidu;

  CoreContinuation k;
  k.rect_id = (int)pf_lire32(charge, 0);
  k.a_node = (int)pf_lire32(charge, 4);
  k.b_node = (int)pf_lire32(charge, 8);
  k.lane = (std::uint8_t)pf_lire32(charge, 12);
  k.lower_open_sat = (std::uint8_t)pf_lire32(charge, 16);
  k.frontier_candidate_mass_exact = pf_lire32(charge, 20);
  k.policy_version = pf_lire32(charge, 24);
  k.pair_mass = (std::uint64_t)pf_lire32(charge, 28) |
                ((std::uint64_t)pf_lire32(charge, 32) << 32);
  k.threshold = c.threshold;
  k.cloud_epoch = c.cloud_epoch;
  k.schema_version = kContinuationSchema;
  if (k.lane != 2u && k.lane != 3u && k.lane != 4u) return DecodeError::kEpoque;
  if (k.lower_open_sat >= c.threshold) return DecodeError::kSeuil;

  std::size_t o = 48;
  std::vector<NodeHandle>* cibles[3] = {&k.decided_spans, &k.mixed_spans,
                                        &k.relation_spans};
  const std::uint32_t tailles[3] = {na, nm, nr};
  for (int t = 0; t < 3; ++t)
    for (std::uint32_t i = 0; i < tailles[t]; ++i, o += 4) {
      const int h = (int)pf_lire32(charge, o);
      if (h < 0 || (std::uint32_t)h >= c.nb_noeuds) return DecodeError::kHandleHorsDomaine;
      cibles[t]->push_back(h);
    }
  const std::uint32_t dom_ep = (c.nb_endpoints == 0u) ? c.nb_noeuds : c.nb_endpoints;
  if (k.a_node < 0 || (std::uint32_t)k.a_node >= dom_ep ||
      k.b_node < 0 || (std::uint32_t)k.b_node > dom_ep)
    return DecodeError::kHandleHorsDomaine;

  // ---- SEMANTIQUE. Les trois plages forment ensemble une antichaine : aucun
  // doublon, aucune relation ancetre-descendant. Un parent et son descendant
  // compteraient deux fois la meme population.
  std::vector<NodeHandle> tous;
  tous.insert(tous.end(), k.decided_spans.begin(), k.decided_spans.end());
  tous.insert(tous.end(), k.mixed_spans.begin(), k.mixed_spans.end());
  tous.insert(tous.end(), k.relation_spans.begin(), k.relation_spans.end());
  for (std::size_t i = 0; i < tous.size(); ++i)
    for (std::size_t j = i + 1; j < tous.size(); ++j) {
      if (tous[i] == tous[j]) return DecodeError::kDoublon;
      if (c.est_ancetre_ou_egal != nullptr &&
          (c.est_ancetre_ou_egal(c.arbre, tous[i], tous[j]) ||
           c.est_ancetre_ou_egal(c.arbre, tous[j], tous[i])))
        return DecodeError::kAncetre;
    }
  if (c.population != nullptr) {
    std::uint64_t masse = 0;
    for (std::size_t i = 0; i < k.mixed_spans.size(); ++i)
      masse += c.population(c.arbre, k.mixed_spans[i]);
    if (masse != (std::uint64_t)k.frontier_candidate_mass_exact)
      return DecodeError::kMasse;
  }
  if (out != nullptr) *out = k;
  return DecodeError::kOk;
}

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
