// MorseHGP3D v6 — ANNEAU DE LOTS : CONTRAT DES BAUX (durees de vie) et
// ORDONNANCEMENT DIFFERE (C6, jalons 1 et 3 de la sequence de livraison de
// audits/REPONSE_AUDITEUR_CONCEPTION_C6_20260902.md).
//
// HOTE PUR : aucun CUDA, aucun tampon epingle, aucune horloge, aucune mesure
// de performance, aucune bascule du chemin de production. Rien ici n'est
// appele par src/pipeline/run.hpp ni par cli/mhgp6_cuda.cu a ce jour ; ce
// fichier est le CONTRAT que C6a devra respecter, pas C6a.
//
// ----------------------------------------------------------------- PREUVE
// CE QUE CE FICHIER PROUVE (avec tests/lot_ring_gate.cpp et le modele
// differe de tests/lot_ring_modele_differe.hpp) : la DISCIPLINE
// D'ORDONNANCEMENT. Qu'aucune ressource hote n'est reprise avant que son
// bail soit rendu ; que la retraite des lots suit l'ordre de `base_global`
// et JAMAIS l'ordre d'achevement ; que l'epoque et la base globale d'un
// emplacement reutilise sont celles de son NOUVEL occupant ; qu'un lot est
// entierement valide avant toute reconstruction de ce lot ; qu'aucun prefixe
// n'est visible avant l'echange terminal.
//
// CE QU'IL NE PROUVE PAS : rien du materiel, rien du parallelisme reel. Il
// n'y a ici ni fil, ni flux CUDA, ni evenement, ni horloge : les achevements
// sont ordonnes PAR LE TEST. Aucune course reelle (data race, visibilite
// memoire, reordonnancement du compilateur ou du materiel) n'est exercee ;
// aucun temps, aucun debit, aucun gain n'est mesure ; la conformite du
// device reste jugee par mhgp6_census_device*, et la parite reelle par le
// pilote en session G4. C'est un AUTO-TEST DE L'ORDONNANCEUR, au sens exact
// ou les auditeurs l'ont qualifie (§ 3 « Stub differe ») ; tests/cuda_stub.hpp
// reste sequentiel et n'est pas touche.
//
// Le contrat est ecrit pour rester VRAI une fois branche sur de vrais
// transferts asynchrones : chaque `*_end` devient alors l'attente de
// l'evenement CUDA du flux du lot, chaque `Step::kBlocked` une attente de
// contre-pression, et aucune regle ci-dessous ne suppose la synchronicite ni
// un ordre d'achevement particulier.
//
// -------------------------------------------------------- CHAINE D'ETATS
// Chaque ticket de lot suit l'ordre logique
//   FREE -> PACKING -> READY -> H2D -> KERNELS -> D2H -> READY_HOST
//        -> VALIDATED -> REBUILT -> FREE
// mais TROIS ressources aux durees de vie SEPAREES le portent, chacune avec
// sa propre machine a etats et son propre compteur d'epoques :
//
//   IN     (tampon hote d'entree)   FREE -> PACKING -> READY -> H2D -> FREE
//          bail rendu apres h2d_done ; l'encodage du lot k+1 peut alors
//          commencer alors meme que le device travaille sur k.
//   DEVICE (jeu de tampons device)  FREE -> H2D -> KERNELS -> D2H -> FREE
//          bail rendu apres d2h_done. En C6a il n'y a QU'UN jeu device et
//          QU'UN flux : `device_slots = 1`.
//   OUT    (tampon hote de sortie)  FREE -> H2D (remplissage des sentinelles,
//          quand OUT en est la source) -> KERNELS -> D2H -> READY_HOST
//          -> VALIDATED -> REBUILT -> FREE
//          bail rendu SEULEMENT apres rebuild_done.
//
// C'est la separation demandee par les auditeurs : deux emplacements a bail
// UNIQUE ne peuvent pas porter simultanement pack(k+1), device(k) et
// rebuild(k-1) ; deux paires IN/OUT a baux SEPARES le peuvent, parce que
// pack(k+1) ecrit IN[p] pendant que rebuild(k-1) lit OUT[p]. Seul le
// transfert suivant VERS OUT[p] attend rebuild_done — et si le
// preremplissage hote des sentinelles emploie ce meme OUT comme source, son
// remplissage ET son H2D l'attendent aussi (`out_is_sentinel_source`, vrai en
// C6a). AUCUNE fin de calcul ne vaut implicitement fin de lecture hote :
// `d2h_end` rend le bail DEVICE, jamais le bail OUT.
//
// ANNEAU STRICT : l'emplacement d'un lot est impose, `slot = lot % depth`.
// Il n'est jamais choisi opportunistement — la place d'un lot dans l'anneau
// est fonction de son index seul, donc reproductible, et un emplacement
// occupe donne un `Step::kBlocked` (contre-pression), jamais un vol.
//
// ------------------------------------------------- GRANULARITE VERSIONNEE
// `lot_ring_granularite_v1` (levee d'ambiguite demandee par les auditeurs) :
// un lot est ENTIEREMENT valide (tous ses enregistrements, depuis l'index
// local 0) avant TOUTE reconstruction de ce lot ; les lots deja reconstruits
// restent dans des temporaires INVISIBLES ; la corruption d'un lot tardif
// jette l'ensemble — zero enregistrement, zero compteur semantique visible.
// L'echange terminal (`publish`) est le SEUL instant de visibilite.
//
// --------------------------------------- REGLE GLOBALE DE CHOIX D'ERREUR
// `lot_ring_erreur_v1` — DETERMINISTE et INDEPENDANTE DE L'ORDRE D'ARRIVEE.
// La PREMIERE erreur (dans le temps) ferme l'admission et declenche le
// drainage ; elle ne decide PAS du verdict. L'erreur RETENUE est le minimum
// lexicographique de la cle
//     (index_global, rang_etage, code, message)
// sur TOUTES les erreurs enregistrees pendant l'operation, ou :
//   - `index_global` = base_global(lot) + index_local de l'enregistrement
//     fautif ; une panne qui ne porte pas sur un enregistrement (erreur
//     device, refus de transfert) prend base_global(lot) — c'est la
//     « reduction deterministe au plus petit index global » de docs/GPU.md ;
//   - `rang_etage` = rang de l'etage dans la chaine ci-dessus (PACKING = 1
//     ... REBUILT = 8) ;
//   - `code` = code numerique, `message` = chaine, departages en dernier.
// Consequence exigee et testee : rejouer la meme scene avec des ordres
// d'achevement differents retient la MEME erreur.
//
// LIBERATION PAR RAII : chaque pas qui rend un bail passe par un jeton de
// portee (`ScopedRelease`), de sorte qu'un refus tardif dans le meme pas ne
// laisse pas le bail pendu ; et la destruction de l'anneau sans `publish`
// reussi ABANDONNE l'operation — tous les baux rendus, tous les temporaires
// detruits, rien de visible chez l'appelant.
//
// MUTANTS (points d'injection ci-dessous, tues code 4 par
// tests/lot_ring_gate.cpp) : `c6-reuse-before-lease`, `c6-merge-by-completion`,
// `c6-wrong-epoch`, `c6-publish-prefix`, `c6-rebuild-before-validate`.
#pragma once

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "../core/mutants.hpp"
#include "../core/types.hpp"

namespace mhgp6 {
namespace gpu {

// Chaine d'etats LITTERALE du contrat (l'ordre des valeurs EST le rang
// d'etage de la regle de choix d'erreur).
enum class LotState : u8 {
  kFree = 0,
  kPacking = 1,
  kReady = 2,
  kH2d = 3,
  kKernels = 4,
  kD2h = 5,
  kReadyHost = 6,
  kValidated = 7,
  kRebuilt = 8,
};

inline const char* lot_state_name(LotState s) {
  switch (s) {
    case LotState::kFree: return "FREE";
    case LotState::kPacking: return "PACKING";
    case LotState::kReady: return "READY";
    case LotState::kH2d: return "H2D";
    case LotState::kKernels: return "KERNELS";
    case LotState::kD2h: return "D2H";
    case LotState::kReadyHost: return "READY_HOST";
    case LotState::kValidated: return "VALIDATED";
    case LotState::kRebuilt: return "REBUILT";
  }
  return "?";
}

// Les TROIS ressources a durees de vie separees.
enum class Resource : u8 { kIn = 0, kDevice = 1, kOut = 2 };
inline constexpr int kResourceCount = 3;

inline const char* resource_name(Resource r) {
  switch (r) {
    case Resource::kIn: return "IN";
    case Resource::kDevice: return "DEVICE";
    case Resource::kOut: return "OUT";
  }
  return "?";
}

inline constexpr u32 kNoLot = 0xffffffffu;
inline constexpr u32 kNoSlot = 0xffffffffu;
// Sentinelles hote : une ecriture omise est DETECTEE, jamais consommee
// (meme doctrine que la frontiere D2H f3704e99).
inline constexpr u64 kRecordSentinel = ~0ull;
inline constexpr u8 kShellSentinel = 0xff;

// BAIL : le minimum contractuel `{epoch, base_global, nb}` — plus le lot
// proprietaire et l'etat de la machine PROPRE a cette ressource.
struct Lease {
  u64 epoch = 0;         // rang de REUTILISATION de l'emplacement (1 = premiere prise)
  u64 base_global = 0;   // index global de la premiere boule logee ici
  u32 nb = 0;            // boules logees
  u32 lot = kNoLot;      // ticket proprietaire (kNoLot si rendu)
  LotState state = LotState::kFree;
};

// Enregistrement transporte. `value` est l'index global de la boule : le
// multiensemble publie DOIT etre exactement 0..nb_total-1 dans cet ordre, ce
// qui rend toute rupture de bail, de base ou d'ordre de fusion VISIBLE dans
// l'objet et pas seulement dans un compteur. `n_shell` est une longueur
// DECLAREE par le device : jamais une borne de lecture avant validation.
struct LotRecord {
  u64 value = kRecordSentinel;
  u8 n_shell = kShellSentinel;
};

// Journal des prises et rendus de bail : le modele differe y reconstruit une
// occupation TEMOIN independante de l'anneau (il ne consulte pas les etats).
struct LeaseEvent {
  Resource res = Resource::kIn;
  u32 slot = kNoSlot;
  u32 lot = kNoLot;
  u64 epoch = 0;
  u64 base_global = 0;
  u32 nb = 0;
  bool acquire = true;
};

struct LotError {
  bool present = false;
  u64 global_index = 0;
  u8 stage_rank = 0;
  u16 code = 0;
  std::string message;
};

// Ordre lexicographique de `lot_ring_erreur_v1`.
inline bool lot_error_less(const LotError& a, const LotError& b) {
  if (a.global_index != b.global_index) return a.global_index < b.global_index;
  if (a.stage_rank != b.stage_rank) return a.stage_rank < b.stage_rank;
  if (a.code != b.code) return a.code < b.code;
  return a.message < b.message;
}

struct RingCounters {
  u64 lots_admitted = 0, lots_retired = 0, tails = 0;
  u64 rotations[kResourceCount] = {0, 0, 0};
  u64 blocked[kResourceCount] = {0, 0, 0};
  u64 leases_live = 0, leases_taken = 0, leases_released = 0;
  u64 errors_recorded = 0;
  // Lectures qu'un contrat respecte rend IMPOSSIBLES (temoins des mutants).
  u64 sentinel_reads = 0;        // reconstruction d'une sortie deja reprise
  u64 out_of_domain_reads = 0;   // longueur non validee employee comme borne
  u64 merges_out_of_order = 0;   // fusion hors ordre de base_global
};

struct LotTicket {
  u32 index = kNoLot;
  u64 base_global = 0;
  u32 nb = 0;
  u32 slot[kResourceCount] = {kNoSlot, kNoSlot, kNoSlot};
  u64 epoch[kResourceCount] = {0, 0, 0};
  u64 lease_base[kResourceCount] = {0, 0, 0};  // base DECLAREE par le bail pris
  LotState state = LotState::kFree;
  bool kernels_done = false;
  bool validated = false;
  bool rebuilt = false;
  bool retired = false;
};

// HOOK DE TEST (jamais un chemin produit) : simule une ecriture device
// corrompue a la descente, pour graver la fixture de corruption tardive.
struct D2hTamper {
  bool on = false;
  u32 local = 0;
  bool bad_value = false;
  bool bad_shell = false;
};

enum class Step : u8 { kOk = 0, kBlocked = 1, kRefused = 2 };

struct StepResult {
  Step step = Step::kOk;
  std::string why;
  bool ok() const { return step == Step::kOk; }
  bool blocked() const { return step == Step::kBlocked; }
  bool refused() const { return step == Step::kRefused; }
};

struct AdmitResult {
  Step step = Step::kOk;
  u32 lot = kNoLot;
  std::string why;
  bool ok() const { return step == Step::kOk; }
  bool blocked() const { return step == Step::kBlocked; }
  bool refused() const { return step == Step::kRefused; }
};

class LotRing {
 public:
  struct Config {
    u64 nb_total = 0;
    u32 lot = 0;
    u32 in_slots = 2;
    u32 out_slots = 2;
    u32 device_slots = 1;
    // C6a : les sentinelles sont PREREMPLIES PAR L'HOTE dans le tampon OUT,
    // qui devient donc source du H2D — son remplissage et son H2D attendent
    // le bail OUT. A false, le bail OUT n'est pris qu'au D2H (variante
    // `k_fill_sentinels`, C6b).
    bool out_is_sentinel_source = true;
    u8 shell_cap = 12;
  };

  explicit LotRing(const Config& cfg) : cfg_(cfg) {
    // PREVALIDATION DE TOUS LES PRODUITS DE TAILLES AVANT ALLOCATION.
    if (cfg_.lot == 0) ctor_error_ = "invalid_input : lot nul";
    else if (cfg_.nb_total == 0) ctor_error_ = "invalid_input : nb_total nul";
    else if (cfg_.nb_total > (u64)1 << 32) ctor_error_ = "invalid_input : nb_total au-dela du domaine du modele";
    else if (cfg_.lot > (1u << 24)) ctor_error_ = "invalid_input : lot au-dela du domaine du modele";
    else if (cfg_.in_slots < 1 || cfg_.in_slots > 8) ctor_error_ = "invalid_input : in_slots hors [1, 8]";
    else if (cfg_.out_slots < 1 || cfg_.out_slots > 8) ctor_error_ = "invalid_input : out_slots hors [1, 8]";
    else if (cfg_.device_slots < 1 || cfg_.device_slots > 8) ctor_error_ = "invalid_input : device_slots hors [1, 8]";
    else if (cfg_.shell_cap > 64) ctor_error_ = "invalid_input : shell_cap hors domaine";
    if (!ctor_error_.empty()) return;
    const u64 slots_total = (u64)cfg_.in_slots + cfg_.out_slots + cfg_.device_slots;
    if (slots_total * (u64)cfg_.lot > ((u64)1 << 28)) {
      ctor_error_ = "resource_exhausted : produit emplacements x lot au-dela du budget du modele";
      return;
    }
    const u64 nlots = (cfg_.nb_total + cfg_.lot - 1) / cfg_.lot;
    if (nlots > ((u64)1 << 24)) {
      ctor_error_ = "resource_exhausted : nombre de lots au-dela du budget du modele";
      return;
    }
    n_lots_ = (u32)nlots;
    const u32 depths[kResourceCount] = {cfg_.in_slots, cfg_.device_slots, cfg_.out_slots};
    for (int r = 0; r < kResourceCount; ++r) {
      slots_[r].resize(depths[r]);
      for (Slot& s : slots_[r]) s.buf.assign(cfg_.lot, LotRecord{});
    }
    tickets_.resize(n_lots_);
    temp_.resize(n_lots_);
    shell_.assign(n_lots_, 0);
    for (u32 k = 0; k < n_lots_; ++k) {
      tickets_[k].index = k;
      tickets_[k].base_global = (u64)k * cfg_.lot;
      tickets_[k].nb = (u32)std::min<u64>(cfg_.lot, cfg_.nb_total - (u64)k * cfg_.lot);
    }
  }

  ~LotRing() {
    // RAII de l'OPERATION ENTIERE : sans echange terminal reussi, tout est
    // abandonne — baux rendus, temporaires et fusion invisible detruits.
    if (!published_) abort_all("destruction sans echange terminal");
  }

  LotRing(const LotRing&) = delete;
  LotRing& operator=(const LotRing&) = delete;

  const std::string& config_error() const { return ctor_error_; }
  u32 n_lots() const { return n_lots_; }
  const LotTicket& ticket(u32 lot) const { return tickets_[lot]; }
  const Lease& lease(Resource r, u32 slot) const { return slots_[(int)r][slot].lease; }
  u32 depth(Resource r) const { return (u32)slots_[(int)r].size(); }
  const RingCounters& counters() const { return c_; }
  const std::vector<LeaseEvent>& journal() const { return journal_; }
  const LotError& error() const { return err_; }
  const std::vector<LotError>& all_errors() const { return all_errors_; }
  bool admission_open() const { return admission_open_; }
  const std::string& admission_closed_why() const { return admission_closed_why_; }
  u32 next_retire() const { return next_retire_; }
  // Taille de la fusion INVISIBLE (temoin de porte ; ce n'est PAS une
  // publication : rien n'a ete rendu a l'appelant).
  size_t pending_size() const { return pending_.size(); }
  // Nombre de lots dont le ticket n'est pas retire et pas libre : les
  // tickets logiques coexistants (la profondeur reelle de l'anneau).
  u32 live_tickets() const {
    u32 n = 0;
    for (const LotTicket& t : tickets_)
      if (t.state != LotState::kFree) ++n;
    return n;
  }

  // ------------------------------------------------------------- ADMISSION
  // Prend le bail IN du prochain lot et ouvre son encodage (PACKING). Le bail
  // OUT n'est PAS pris ici : c'est exactement ce qui autorise pack(k+1)
  // pendant rebuild(k-1).
  AdmitResult admit() {
    AdmitResult a;
    if (!ctor_error_.empty()) return {Step::kRefused, kNoLot, ctor_error_};
    if (!admission_open_)
      return {Step::kRefused, kNoLot, "admission fermee : drainage apres la premiere erreur"};
    if (next_admit_ >= n_lots_) return {Step::kRefused, kNoLot, "tous les lots sont deja admis"};
    const u32 lot = next_admit_;
    const StepResult s = acquire(Resource::kIn, lot);
    if (!s.ok()) return {s.step, kNoLot, s.why};
    tickets_[lot].state = LotState::kPacking;
    slots_[(int)Resource::kIn][tickets_[lot].slot[(int)Resource::kIn]].lease.state = LotState::kPacking;
    ++next_admit_;
    ++c_.lots_admitted;
    if (tickets_[lot].nb < cfg_.lot) ++c_.tails;
    a.lot = lot;
    return a;
  }

  // ------------------------------------------------------------- ENCODAGE
  // L'encodeur ecrit dans le tampon IN a partir de la base DECLAREE PAR LE
  // BAIL (comme l'encodeur reel ecrit a l'offset i*112 du tampon epingle
  // qu'on lui a remis) : une base de bail fausse produit donc des octets
  // faux, pas seulement un temoin faux.
  StepResult pack_end(u32 lot) {
    StepResult r;
    if (!(r = expect(lot, LotState::kPacking, "pack_end")).ok()) return r;
    Slot& in = slot_of(Resource::kIn, lot);
    for (u32 i = 0; i < tickets_[lot].nb; ++i) in.buf[i] = LotRecord{in.lease.base_global + i, kShellSentinel};
    in.lease.state = LotState::kReady;
    tickets_[lot].state = LotState::kReady;
    return r;
  }

  // ---------------------------------------------------------------- MONTEE
  StepResult h2d_begin(u32 lot) {
    StepResult r;
    if (!(r = expect(lot, LotState::kReady, "h2d_begin")).ok()) return r;
    if (!admission_open_)
      return {Step::kRefused, "drainage : aucun nouveau transfert n'est lance apres la premiere erreur"};
    // Le bail OUT d'abord quand il est source des sentinelles : c'est LUI qui
    // porte l'attente explicite de rebuild_done.
    bool took_out = false;
    if (cfg_.out_is_sentinel_source) {
      const StepResult o = acquire(Resource::kOut, lot);
      if (!o.ok()) return o;
      took_out = true;
    }
    ScopedRelease undo_out(took_out ? this : nullptr, Resource::kOut,
                           took_out ? tickets_[lot].slot[(int)Resource::kOut] : 0);
    const StepResult d = acquire(Resource::kDevice, lot);
    if (!d.ok()) return d;  // le bail OUT est rendu par le jeton
    undo_out.disarm();
    if (took_out) {
      // REMPLISSAGE HOTE DES SENTINELLES : il ecrit dans OUT, donc il attend
      // le bail OUT — la source du H2D descendant n'est jamais une allocation
      // indeterminee.
      Slot& out = slot_of(Resource::kOut, lot);
      for (u32 i = 0; i < cfg_.lot; ++i) out.buf[i] = LotRecord{};
      out.lease.state = LotState::kH2d;
    }
    slot_of(Resource::kIn, lot).lease.state = LotState::kH2d;
    slot_of(Resource::kDevice, lot).lease.state = LotState::kH2d;
    tickets_[lot].state = LotState::kH2d;
    return r;
  }

  StepResult h2d_end(u32 lot) {
    StepResult r;
    if (!(r = expect(lot, LotState::kH2d, "h2d_end")).ok()) return r;
    // Le transfert lit le tampon IN MAINTENANT — comme un vrai H2D
    // asynchrone lit le tampon epingle pendant que l'hote pourrait deja le
    // reecrire s'il ne respectait pas le bail. Aucune verification de
    // propriete ici : c'est le bail, pris a l'acquisition, qui protege.
    const Slot& in = slot_of(Resource::kIn, lot);
    Slot& dev = slot_of(Resource::kDevice, lot);
    for (u32 i = 0; i < tickets_[lot].nb; ++i) dev.buf[i] = in.buf[i];
    // BAIL IN RENDU APRES h2d_done (et par RAII).
    ScopedRelease rel(this, Resource::kIn, tickets_[lot].slot[(int)Resource::kIn]);
    (void)rel;
    tickets_[lot].slot[(int)Resource::kIn] = kNoSlot;
    // L'emplacement device reste en H2D jusqu'au lancement des kernels : le
    // bail device couvre H2D, kernels et D2H sans discontinuite.
    return r;
  }

  StepResult kernels_begin(u32 lot) {
    StepResult r;
    if (!(r = expect(lot, LotState::kH2d, "kernels_begin")).ok()) return r;
    if (tickets_[lot].slot[(int)Resource::kIn] != kNoSlot)
      return {Step::kRefused, "invariant : kernels lances avant la fin du transfert montant"};
    slot_of(Resource::kDevice, lot).lease.state = LotState::kKernels;
    if (cfg_.out_is_sentinel_source) slot_of(Resource::kOut, lot).lease.state = LotState::kKernels;
    tickets_[lot].state = LotState::kKernels;
    return r;
  }

  StepResult kernels_end(u32 lot) {
    StepResult r;
    if (!(r = expect(lot, LotState::kKernels, "kernels_end")).ok()) return r;
    // Les kernels transforment le tampon device EN PLACE, en lisant ce qui a
    // ete transfere : un montant corrompu se propage jusqu'au validateur.
    Slot& dev = slot_of(Resource::kDevice, lot);
    for (u32 i = 0; i < tickets_[lot].nb; ++i) dev.buf[i].n_shell = shell_of(dev.buf[i].value);
    tickets_[lot].kernels_done = true;
    return r;
  }

  // -------------------------------------------------------------- DESCENTE
  StepResult d2h_begin(u32 lot) {
    StepResult r;
    if (!(r = expect(lot, LotState::kKernels, "d2h_begin")).ok()) return r;
    if (!tickets_[lot].kernels_done)
      return {Step::kRefused, "invariant : descente lancee avant la fin des kernels du lot"};
    if (!cfg_.out_is_sentinel_source) {
      // ATTENTE EXPLICITE DU BAIL OUT : un transfert descendant vers une
      // sortie REUTILISEE attend que la reconstruction precedente ait rendu
      // sa duree de vie. Aucune fin de calcul ne vaut fin de lecture hote.
      const StepResult o = acquire(Resource::kOut, lot);
      if (!o.ok()) return o;
    }
    slot_of(Resource::kOut, lot).lease.state = LotState::kD2h;
    slot_of(Resource::kDevice, lot).lease.state = LotState::kD2h;
    tickets_[lot].state = LotState::kD2h;
    return r;
  }

  StepResult d2h_end(u32 lot, const D2hTamper& tamper = D2hTamper{}) {
    StepResult r;
    if (!(r = expect(lot, LotState::kD2h, "d2h_end")).ok()) return r;
    const Slot& dev = slot_of(Resource::kDevice, lot);
    Slot& out = slot_of(Resource::kOut, lot);
    const u32 nb = tickets_[lot].nb;
    for (u32 i = 0; i < nb; ++i) out.buf[i] = dev.buf[i];
    if (tamper.on && tamper.local < nb) {  // hook de test : ecriture device corrompue
      if (tamper.bad_value) out.buf[tamper.local].value ^= 1ull;
      if (tamper.bad_shell) out.buf[tamper.local].n_shell = (u8)(cfg_.shell_cap + 200);
    }
    // BAIL DEVICE RENDU APRES d2h_done — le bail OUT, lui, court encore.
    ScopedRelease rel(this, Resource::kDevice, tickets_[lot].slot[(int)Resource::kDevice]);
    (void)rel;
    tickets_[lot].slot[(int)Resource::kDevice] = kNoSlot;
    out.lease.state = LotState::kReadyHost;
    tickets_[lot].state = LotState::kReadyHost;
    return r;
  }

  // ------------------------------------------------------------ VALIDATION
  // Le lot ENTIER est parcouru depuis l'index local 0 : la premiere faute
  // rencontree est donc la plus petite en index global du lot
  // (`lot_ring_granularite_v1`).
  StepResult validate(u32 lot) {
    StepResult r;
    if (!(r = expect(lot, LotState::kReadyHost, "validate")).ok()) return r;
    const Slot& out = slot_of(Resource::kOut, lot);
    const LotTicket& t = tickets_[lot];
    for (u32 i = 0; i < t.nb; ++i) {
      const LotRecord& rec = out.buf[i];
      const u64 gi = t.base_global + i;
      if (rec.value == kRecordSentinel)
        return fail_record(lot, i, LotState::kValidated, 1, "sortie non ecrite (sentinelle lue au validateur)");
      if (rec.value != gi)
        return fail_record(lot, i, LotState::kValidated, 2,
                           "valeur inattendue : base globale ou bail rompu");
      if (rec.n_shell > cfg_.shell_cap)
        return fail_record(lot, i, LotState::kValidated, 3, "n_shell hors domaine");
    }
    slots_[(int)Resource::kOut][t.slot[(int)Resource::kOut]].lease.state = LotState::kValidated;
    tickets_[lot].validated = true;
    tickets_[lot].state = LotState::kValidated;
    return r;
  }

  // -------------------------------------------------------- RECONSTRUCTION
  StepResult rebuild_begin(u32 lot) {
    StepResult r;
    LotState required = LotState::kValidated;
    if (MHGP6_MUTANT("c6-rebuild-before-validate")) {
      // DEFAUT REINTRODUIT : la reconstruction accepte un lot pas encore
      // entierement valide — une longueur corrompue redevient une borne de
      // lecture (docs/GPU.md, frontiere D2H).
      required = LotState::kReadyHost;
    }
    if (tickets_[lot].state != required && tickets_[lot].state != LotState::kValidated)
      return {Step::kRefused, std::string("rebuild_begin : lot ") + std::to_string(lot) +
                                  " en etat " + lot_state_name(tickets_[lot].state) +
                                  " (le lot doit etre ENTIEREMENT valide)"};
    if (!admission_open_ && !tickets_[lot].validated)
      return {Step::kRefused, "drainage : aucune reconstruction n'est ouverte apres la premiere erreur"};
    slots_[(int)Resource::kOut][tickets_[lot].slot[(int)Resource::kOut]].lease.state = LotState::kRebuilt;
    tickets_[lot].state = LotState::kRebuilt;
    return r;
  }

  StepResult rebuild_end(u32 lot) {
    StepResult r;
    if (!(r = expect(lot, LotState::kRebuilt, "rebuild_end")).ok()) return r;
    const Slot& out = slot_of(Resource::kOut, lot);
    const u32 nb = tickets_[lot].nb;
    std::vector<u64> tmp;
    tmp.reserve(nb);
    u64 shell = 0;
    for (u32 i = 0; i < nb; ++i) {
      const LotRecord& rec = out.buf[i];
      if (rec.value == kRecordSentinel) ++c_.sentinel_reads;
      tmp.push_back(rec.value);
      // `n_shell` employe comme LONGUEUR : apres validation il est dans le
      // domaine par construction ; toute lecture hors domaine ici signale que
      // la reconstruction a precede la validation.
      if (rec.n_shell > cfg_.shell_cap) ++c_.out_of_domain_reads;
      else shell += rec.n_shell;
    }
    // TEMPORAIRE INVISIBLE : le lot reconstruit n'est encore vu de personne.
    temp_[lot] = std::move(tmp);
    shell_[lot] = shell;
    // BAIL OUT RENDU SEULEMENT MAINTENANT (rebuild_done), par RAII.
    {
      ScopedRelease rel(this, Resource::kOut, tickets_[lot].slot[(int)Resource::kOut]);
      (void)rel;
    }
    tickets_[lot].slot[(int)Resource::kOut] = kNoSlot;
    tickets_[lot].rebuilt = true;
    merge_ready(lot);
    return r;
  }

  // ------------------------------------------------------------- ERREURS
  // Panne qui ne porte pas sur un enregistrement (erreur device, refus de
  // transfert) : elle prend l'index global de la PREMIERE boule du lot.
  StepResult fail_device(u32 lot, u16 code, const std::string& why) {
    if (lot >= n_lots_) return {Step::kRefused, "fail_device : lot hors domaine"};
    record_error(tickets_[lot].base_global, tickets_[lot].state, code, why);
    return {Step::kRefused, why};
  }

  void close_admission(const char* why) {
    admission_open_ = false;
    admission_closed_why_ = why;
  }

  // ----------------------------------------------------- ECHANGE TERMINAL
  // Le SEUL instant de visibilite. Refus si une erreur a ete retenue, si un
  // lot n'est pas retire, ou si un bail court encore.
  bool publish(std::vector<u64>* values, u64* shell_sum, std::string* why) {
    const bool refuse = err_.present || next_retire_ != n_lots_ || c_.leases_live != 0 ||
                        !ctor_error_.empty();
    if (refuse && MHGP6_MUTANT("c6-publish-prefix")) {
      // DEFAUT REINTRODUIT : l'echange terminal a lieu malgre le refus — un
      // PREFIXE (les lots deja fusionnes) devient visible chez l'appelant.
      values->swap(pending_);
      *shell_sum = pending_shell_;
      if (why) *why = "prefixe publie";
      published_ = true;
      return true;
    }
    if (refuse) {
      if (why) {
        if (!ctor_error_.empty()) *why = ctor_error_;
        else if (err_.present) *why = err_.message;
        else if (next_retire_ != n_lots_) *why = "operation incomplete : lots non retires";
        else *why = "invariant : un bail court encore a l'echange terminal";
      }
      return false;  // rien n'est touche chez l'appelant
    }
    values->swap(pending_);
    *shell_sum = pending_shell_;
    published_ = true;
    if (why) why->clear();
    return true;
  }

  // Abandon explicite : baux rendus, temporaires et fusion invisible jetes.
  void abort_all(const char* why) {
    close_admission(why);
    for (int r = 0; r < kResourceCount; ++r)
      for (u32 s = 0; s < (u32)slots_[r].size(); ++s)
        if (slots_[r][s].lease.state != LotState::kFree) release((Resource)r, s);
    for (LotTicket& t : tickets_) {
      for (int r = 0; r < kResourceCount; ++r) t.slot[r] = kNoSlot;
      t.state = LotState::kFree;
      t.kernels_done = false;
    }
    temp_.assign(n_lots_, std::vector<u64>{});
    shell_.assign(n_lots_, 0);
    pending_.clear();
    pending_shell_ = 0;
  }

 private:
  struct Slot {
    Lease lease;
    u64 acquisitions = 0;
    std::vector<LotRecord> buf;
  };

  // LIBERATION PAR RAII : le jeton rend l'emplacement a la sortie de portee,
  // que le pas s'acheve normalement ou par un refus tardif.
  class ScopedRelease {
   public:
    ScopedRelease(LotRing* r, Resource res, u32 slot) : r_(r), res_(res), slot_(slot) {}
    ~ScopedRelease() {
      if (r_ != nullptr) r_->release(res_, slot_);
    }
    ScopedRelease(const ScopedRelease&) = delete;
    ScopedRelease& operator=(const ScopedRelease&) = delete;
    void disarm() { r_ = nullptr; }

   private:
    LotRing* r_;
    Resource res_;
    u32 slot_;
  };

  static u8 shell_of(u64 v) { return (u8)(v % 7); }

  Slot& slot_of(Resource r, u32 lot) { return slots_[(int)r][tickets_[lot].slot[(int)r]]; }
  const Slot& slot_of(Resource r, u32 lot) const { return slots_[(int)r][tickets_[lot].slot[(int)r]]; }

  StepResult expect(u32 lot, LotState st, const char* who) {
    if (!ctor_error_.empty()) return {Step::kRefused, ctor_error_};
    if (lot >= n_lots_) return {Step::kRefused, std::string(who) + " : lot hors domaine"};
    if (tickets_[lot].state != st)
      return {Step::kRefused, std::string(who) + " : lot " + std::to_string(lot) + " en etat " +
                                  lot_state_name(tickets_[lot].state) + ", attendu " + lot_state_name(st)};
    return StepResult{};
  }

  // ANNEAU STRICT : l'emplacement est impose par l'index du lot.
  StepResult acquire(Resource res, u32 lot) {
    const u32 s = lot % (u32)slots_[(int)res].size();
    Slot& sl = slots_[(int)res][s];
    bool free_slot = sl.lease.state == LotState::kFree;
    if (!free_slot && MHGP6_MUTANT("c6-reuse-before-lease")) {
      // DEFAUT REINTRODUIT : l'emplacement est repris alors que son bail
      // court encore (H2D en vol cote IN, reconstruction en vol cote OUT).
      free_slot = true;
    }
    if (!free_slot) {
      ++c_.blocked[(int)res];
      return {Step::kBlocked, std::string("bail non rendu : ") + resource_name(res) + "[" +
                                  std::to_string(s) + "] tenu par le lot " +
                                  std::to_string(sl.lease.lot) + " en etat " +
                                  lot_state_name(sl.lease.state)};
    }
    const u64 previous_base = sl.lease.base_global;
    ++sl.acquisitions;
    u64 epoch = sl.acquisitions;
    u64 base = tickets_[lot].base_global;
    if (sl.acquisitions >= 2 && MHGP6_MUTANT("c6-wrong-epoch")) {
      // DEFAUT REINTRODUIT : au bouclage, le bail garde l'epoque et la base
      // globale de l'occupant precedent — l'encodage repart d'une base
      // fausse (cousin de `gpu-lot-base-reset`, cote hote).
      epoch = sl.acquisitions - 1;
      base = previous_base;
    }
    sl.lease.epoch = epoch;
    sl.lease.base_global = base;
    sl.lease.nb = tickets_[lot].nb;
    sl.lease.lot = lot;
    sl.lease.state = LotState::kPacking;  // etat d'entree ; chaque pas le precise
    tickets_[lot].slot[(int)res] = s;
    tickets_[lot].epoch[(int)res] = epoch;
    tickets_[lot].lease_base[(int)res] = base;
    if (sl.acquisitions >= 2) ++c_.rotations[(int)res];
    ++c_.leases_live;
    ++c_.leases_taken;
    journal_.push_back(LeaseEvent{res, s, lot, epoch, base, tickets_[lot].nb, true});
    return StepResult{};
  }

  void release(Resource res, u32 slot) {
    Slot& sl = slots_[(int)res][slot];
    journal_.push_back(LeaseEvent{res, slot, sl.lease.lot, sl.lease.epoch, sl.lease.base_global,
                                  sl.lease.nb, false});
    sl.lease.state = LotState::kFree;
    sl.lease.lot = kNoLot;  // epoch et base_global RESTENT : temoin du sortant
    if (c_.leases_live > 0) --c_.leases_live;
    ++c_.leases_released;
  }

  // RETRAITE STRICTEMENT ORDONNEE PAR `base_global`, jamais par ordre de fin.
  void merge_ready(u32 just_finished) {
    if (MHGP6_MUTANT("c6-merge-by-completion")) {
      // DEFAUT REINTRODUIT : le lot qui vient de finir est fusionne TOUT DE
      // SUITE, dans l'ordre d'achevement.
      if (just_finished != next_retire_) ++c_.merges_out_of_order;
      append_lot(just_finished);
      tickets_[just_finished].retired = true;
      tickets_[just_finished].state = LotState::kFree;
      ++c_.lots_retired;
      while (next_retire_ < n_lots_ && tickets_[next_retire_].retired) ++next_retire_;
      return;
    }
    while (next_retire_ < n_lots_ && tickets_[next_retire_].rebuilt && !tickets_[next_retire_].retired) {
      append_lot(next_retire_);
      tickets_[next_retire_].retired = true;
      tickets_[next_retire_].state = LotState::kFree;
      ++c_.lots_retired;
      ++next_retire_;
    }
  }

  void append_lot(u32 lot) {
    pending_.insert(pending_.end(), temp_[lot].begin(), temp_[lot].end());
    pending_shell_ += shell_[lot];
    temp_[lot].clear();
    temp_[lot].shrink_to_fit();
  }

  StepResult fail_record(u32 lot, u32 local, LotState stage, u16 code, const char* msg) {
    const u64 gi = tickets_[lot].base_global + local;
    record_error(gi, stage, code, std::string(msg) + " (lot " + std::to_string(lot) + ", index global " +
                                      std::to_string(gi) + ")");
    return {Step::kRefused, err_.message};
  }

  void record_error(u64 global_index, LotState stage, u16 code, const std::string& message) {
    LotError e;
    e.present = true;
    e.global_index = global_index;
    e.stage_rank = (u8)stage;
    e.code = code;
    e.message = message;
    all_errors_.push_back(e);
    ++c_.errors_recorded;
    // La PREMIERE erreur ferme l'admission et declenche le drainage...
    if (admission_open_) close_admission("premiere erreur");
    // ... mais le verdict est le minimum de la regle globale, pas l'arrivee.
    if (!err_.present || lot_error_less(e, err_)) err_ = e;
  }

  Config cfg_;
  std::string ctor_error_;
  std::string admission_closed_why_;
  std::vector<Slot> slots_[kResourceCount];
  std::vector<LotTicket> tickets_;
  std::vector<std::vector<u64>> temp_;
  std::vector<u64> shell_;
  std::vector<u64> pending_;
  std::vector<LeaseEvent> journal_;
  std::vector<LotError> all_errors_;
  LotError err_;
  RingCounters c_;
  u64 pending_shell_ = 0;
  u32 n_lots_ = 0;
  u32 next_admit_ = 0;
  u32 next_retire_ = 0;
  bool admission_open_ = true;
  bool published_ = false;
};

}  // namespace gpu
}  // namespace mhgp6
