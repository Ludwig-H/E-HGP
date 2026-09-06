// MorseHGP3D v6 — ROUTE C6a (jalon 4 de la sequence de livraison de
// audits/REPONSE_AUDITEUR_CONCEPTION_C6_20260902.md), SOUS STUB.
//
// La route C5 (`src/gpu/pilot.hpp`, `stub_prefilter_census_route`) reste
// INTACTE et disponible : elle materialise GLOBALEMENT, pour TOUT le nuage,
// les 112 octets d'entree par boule, leur decodage en mots, et les 100
// octets de sortie par boule. C6a supprime cette residence globale sans
// toucher a l'objet : les candidats sont consommes PAR LOTS, chaque lot est
// encode par l'encodeur pur a offsets fixes (`pack_candidate_range`) dans un
// tampon d'ENTREE de l'anneau, execute par le backend (le stub aujourd'hui),
// puis valide et reconstruit lot par lot depuis un tampon de SORTIE.
//
// ------------------------------------------------------ CONTRAT DES BAUX
// L'ordonnancement est celui de `src/gpu/lot_ring.hpp`, employe ICI comme
// ORDONNANCEUR de la route : etats, baux, epoques, contre-pression, regle
// d'erreur `lot_ring_erreur_v1` et autorisation d'echange terminal. Les
// tampons REELS appartiennent a la route et sont indexes par l'EMPLACEMENT
// que l'anneau attribue au lot (`ticket(lot).slot[res]`) :
//   IN     rendu apres la montee   -> l'encodage du lot k+1 peut commencer
//                                     alors que le device travaille sur k ;
//   DEVICE rendu apres la descente -> un seul jeu, un seul lot device ;
//   OUT    rendu SEULEMENT apres la reconstruction -> une montee vers une
//                                     sortie reutilisee attend son bail.
// Le preremplissage des sentinelles est HOTE et ecrit dans le tampon OUT
// (`out_is_sentinel_source = true`) : son remplissage ET sa montee attendent
// donc le bail OUT — aucun noyau de remplissage en C6a.
//
// `lot_ring_granularite_v1` : un lot est ENTIEREMENT valide (depuis l'index
// local 0) avant TOUTE reconstruction de ce lot ; les lots deja reconstruits
// restent dans des temporaires INVISIBLES ; l'echange terminal est le SEUL
// instant de publication, et il est AUTORISE PAR L'ANNEAU (aucune erreur
// retenue, tous les lots retires, aucun bail vivant).
//
// ------------------------------------------------- FORME DU PREMIER JALON
// Litteralement celle qu'imposent les auditeurs : sentinelles remplies par
// l'hote, UN seul flux et UN seul jeu de memoire de calcul, reconstruction
// SEQUENTIELLE a `reserve` conservateur (jamais un `resize` parallele : la
// variante n'est pas ouverte avant mesure). Le recouvrement VISE est
// hote/device ; sous le stub il n'y a ni fil ni flux, donc AUCUN
// recouvrement reel n'est exerce et AUCUN temps n'est mesure ici — pas de
// chronometre, pas de seau, pas de claim de gain (la grammaire de
// chronometrie C6 est versionnee separement).
//
// L'ordonnanceur avance quatre etages par tour, dans l'ordre INVERSE des
// dependances (reconstruction, descente, montee+noyaux, encodage), ce qui
// fait coexister TROIS tickets logiques : k-1 attend sa reconstruction en
// tenant OUT, k tient DEVICE et OUT, k+1 tient IN. C'est la separation des
// baux qui l'autorise ; un anneau a emplacement unique rend au contraire la
// contre-pression visible (`blocked_*`).
//
// ----------------------------------------------------------------- LIMITES
// Sous stub, rien du materiel n'est prouve : pas de flux CUDA, pas
// d'evenement, pas de course, pas de memoire epinglee, pas de temps. Ce que
// cette route prouve est que l'OBJET est identique a celui des routes CPU et
// C5 lorsque la couture passe par l'anneau et l'encodeur pur.
//
// MUTANTS (points d'injection greppables, tues code 4 par
// tests/route_c6_gate.cpp) : `gpu-lot-base-reset` ci-dessous (la base du
// census redevient 0 a chaque lot — le chainage `cand_idx = base + gid` est
// rompu des le second lot) ; `c6-wrong-epoch`, `c6-reuse-before-lease` et
// `c6-publish-prefix` mordent depuis `src/gpu/lot_ring.hpp`.
#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "../pipeline/candidates.hpp"
#include "../pipeline/expand.hpp"
#include "census_kernels.cuh"
#include "lot_ring.hpp"
#include "pack.hpp"
#include "wire.hpp"

#if defined(__CUDACC__) || defined(MHGP7_FAKE_DEVICE)

namespace mhgp7 {
namespace gpu {

inline constexpr const char* kRouteC6Version = "route_c6a_v1";

struct RouteC6Options {
  // 0 = un seul lot (= nb_total). Sinon le lot effectif est min(lot, nb_total).
  size_t lot = 0;
  u32 in_slots = 2, out_slots = 2, device_slots = 1;
  u32 mut = 0;         // drapeaux de mutants KERNELS (memes bits que la route C5)
  bool witness = true; // temoin d'ordonnancement de l'anneau (8 o par boule, cf. compteurs)
  // HOOK DE TEST (jamais un chemin produit) : corrompt UN enregistrement DEJA
  // DESCENDU, pour graver la fixture « un refus au milieu ne publie rien ».
  bool tamper_on = false;
  u32 tamper_lot = 0, tamper_local = 0;
};

// COMPTEURS DETERMINISTES seulement — aucun temps, aucun debit, aucun gain.
struct RouteC6Counters {
  u64 nb_total = 0, lot_effectif = 0, lots = 0, lots_admis = 0, lots_retires = 0, tails = 0, tours = 0;
  u64 rotations_in = 0, rotations_device = 0, rotations_out = 0;
  u64 blocked_in = 0, blocked_device = 0, blocked_out = 0;
  u64 max_live_tickets = 0;
  // Residence DECLAREE (octets logiques ; ni padding, ni pic systeme).
  u64 octets_in_slots = 0, octets_device = 0, octets_out_slots = 0, octets_temoin = 0;
  u64 boules_encodees = 0, boules_validees = 0, boules_reconstruites = 0;
};

namespace route_c6_detail {

// SORTIES PAR BOULE en SoA — la decomposition EXACTE du transport reel
// (kWireOutBytesPerBall = 100 o : count 8 + pstatut 1 + ids 21x4 + cstatut 1
// + n_int 1 + n_shell 1 + cand_idx 4). Un tampon par emplacement.
struct OutSlab {
  std::vector<u64> count;
  std::vector<u8> pstatus, cstatus, nint, nsh;
  std::vector<i32> ids;
  std::vector<u32> cand;

  void allocate(size_t lot) {
    count.assign(lot, 0);
    pstatus.assign(lot, 0);
    cstatus.assign(lot, 0);
    nint.assign(lot, 0);
    nsh.assign(lot, 0);
    ids.assign(lot * (size_t)kOutIdsPerBall, 0);
    cand.assign(lot, 0);
  }

  // SENTINELLES REMPLIES PAR L'HOTE (f3704e99) : une ecriture device omise
  // laisse la sentinelle et le validateur refuse AVANT toute reconstruction.
  void fill_sentinels(size_t nb) {
    for (size_t i = 0; i < nb; ++i) {
      count[i] = ~0ull;
      pstatus[i] = kSentinelStatus;
      cstatus[i] = kSentinelStatus;
      nint[i] = 0xff;
      nsh[i] = 0xff;
      cand[i] = 0xffffffffu;
    }
    for (size_t i = 0; i < nb * (size_t)kOutIdsPerBall; ++i) ids[i] = kSentinelId;
  }

  void copy_from(const OutSlab& src, size_t nb) {
    for (size_t i = 0; i < nb; ++i) {
      count[i] = src.count[i];
      pstatus[i] = src.pstatus[i];
      cstatus[i] = src.cstatus[i];
      nint[i] = src.nint[i];
      nsh[i] = src.nsh[i];
      cand[i] = src.cand[i];
    }
    for (size_t i = 0; i < nb * (size_t)kOutIdsPerBall; ++i) ids[i] = src.ids[i];
  }
};

// `reserve` CONSERVATEUR de la reconstruction sequentielle (les auditeurs
// interdisent d'ouvrir la variante `resize` parallele avant mesure) : la
// capacite grandit geometriquement, jamais a `nb_total` d'avance — reserver
// tout le nuage rendrait a la residence globale ce que les lots lui otent.
template <class T>
inline void reserve_growth(std::vector<T>* v, size_t need) {
  if (v->capacity() - v->size() >= need) return;
  size_t want = v->size() + need;
  if (want < v->capacity() * 2) want = v->capacity() * 2;
  v->reserve(want);
}

}  // namespace route_c6_detail

// ROUTE C6a : meme signature utile que la route C5 (appelable comme
// `RunOptions::prefilter_census_override`), plus les options de lotissement
// et les compteurs. Rend "" ou le motif d'un REFUS TRANSACTIONNEL — jamais un
// prefixe publie.
inline std::string route_c6_prefilter_census(const CloudIndex& ix, const std::vector<BallCandidate>& cands,
                                             u64 smax, size_t shell_cap, std::vector<Survivor>* surv,
                                             std::vector<BallData>* balls, ExpandStats* st,
                                             const RouteC6Options& opt, RouteC6Counters* cnt) {
  using route_c6_detail::OutSlab;
  using route_c6_detail::reserve_growth;
  RouteC6Counters sink;
  RouteC6Counters& c = cnt != nullptr ? *cnt : sink;

  const GpuCloudIndexWire w = build_index_wire(ix);
  if (!w.error.empty()) return w.error;

  const u64 nb_total = (u64)cands.size();
  c.nb_total = nb_total;
  if (nb_total == 0) {
    // bc5812dc : le cas VIDE publie des sorties vides — jamais les anciennes
    // valeurs du caller conservees en silence.
    surv->clear();
    balls->clear();
    st->dead_depth = 0;
    st->survivors = 0;
    return "";
  }
  // MEME BORNE que `append_ball_in` (contrat gpu_wire_v1).
  if (nb_total > kWireMaxBalls) return "invalid_input : plus de 2^32 - 1 boules";

  // INDEX RESIDENT : inchange par rapport a C5 (il ne depend pas des lots).
  const GpuIndexHostView v = decode_index_wire(w);
  const i32* nl = v.node_left.data();
  const i32* nr = v.node_right.data();
  const i32* nf = v.node_first.data();
  const i32* nlast = v.node_last.data();
  const u16* nbox = v.node_box.data();
  const u16* up = v.upos.data();
  const u32* ws = v.wsum.data();

  const size_t lot = (size_t)std::min<u64>(opt.lot == 0 ? nb_total : (u64)opt.lot, nb_total);
  // PREVALIDATION DE TOUS LES PRODUITS DE TAILLES AVANT ALLOCATION.
  const WireSizePlan plan = wire_plan_bytes(lot);
  if (plan.status != PackStatus::kOk) return pack_status_name(plan.status);
  c.lot_effectif = (u64)lot;

  LotRing::Config rc;
  rc.nb_total = nb_total;
  rc.lot = (u32)lot;
  rc.in_slots = opt.in_slots;
  rc.out_slots = opt.out_slots;
  rc.device_slots = opt.device_slots;
  rc.out_is_sentinel_source = true;  // C6a : sentinelles HOTE, aucun noyau de remplissage
  rc.shell_cap = (u8)std::min<size_t>(shell_cap, 64);
  rc.shadow_payload = opt.witness;
  LotRing ring(rc);
  if (!ring.config_error().empty()) return ring.config_error();
  const u32 n_lots = ring.n_lots();
  c.lots = n_lots;

  // TOUTES LES RESSOURCES SONT ACQUISES AVANT LA PREMIERE ADMISSION (aucun
  // agrandissement en cours d'operation, aucune allocation par lot).
  std::vector<std::vector<u8>> in_slabs((size_t)opt.in_slots);
  for (std::vector<u8>& b : in_slabs) b.assign(plan.in_bytes, 0);
  std::vector<OutSlab> out_slabs((size_t)opt.out_slots);
  for (OutSlab& o : out_slabs) o.allocate(lot);
  std::vector<std::vector<u64>> dev_words((size_t)opt.device_slots);
  for (std::vector<u64>& d : dev_words) d.assign(lot * kBallWords, 0);
  std::vector<OutSlab> dev_out((size_t)opt.device_slots);
  for (OutSlab& o : dev_out) o.allocate(lot);
  c.octets_in_slots = (u64)opt.in_slots * (u64)plan.in_bytes;
  c.octets_out_slots = (u64)opt.out_slots * (u64)plan.out_bytes;
  c.octets_device = (u64)opt.device_slots * ((u64)lot * (u64)kBallWords * 8 + (u64)plan.out_bytes);
  c.octets_temoin =
      opt.witness ? nb_total * 8 + (u64)(opt.in_slots + opt.out_slots + opt.device_slots) * (u64)lot *
                                       (u64)sizeof(LotRecord)
                  : 0;

  // TEMPORAIRES INVISIBLES : rien n'est rendu au caller avant l'echange final.
  std::vector<Survivor> lsurv;
  std::vector<BallData> lballs;
  u64 dead = 0, l_int = 0, l_sh = 0;
  const u64 total_mass = ix.wsum.back();
  std::string err;

  u32 next_pack = 0, next_ker = 0, next_down = 0, next_reb = 0;
  const u64 max_tours = (u64)n_lots * 4 + 16;
  while (err.empty() && next_reb < n_lots) {
    if (++c.tours > max_tours) {
      err = "invariant : ordonnanceur C6 sans progres (borne de tours franchie)";
      break;
    }
    bool did = false;

    // ---- ETAGE A : RECONSTRUCTION du plus ancien lot descendu. Elle vient
    // EN PREMIER dans le tour : c'est elle qui rend le bail OUT que la montee
    // du lot suivant reclamera, et c'est ce decalage qui fait coexister trois
    // tickets.
    if (next_reb < next_down) {
      const u32 lot_id = next_reb;
      const LotTicket& t = ring.ticket(lot_id);
      const u32 q = t.slot[(int)Resource::kOut];
      const u64 base = t.base_global;
      const u32 nb = t.nb;
      const OutSlab& o = out_slabs[q];
      // PASSE 1 — VALIDATION DU LOT ENTIER avant toute reconstruction de ce
      // lot (`lot_ring_granularite_v1`), depuis l'index local 0 : la premiere
      // faute rencontree est donc la plus petite en index global du lot.
      const char* why = nullptr;
      u32 bad = 0;
      for (u32 i = 0; i < nb; ++i) {
        const size_t g = (size_t)base + i;
        const i32* row = &o.ids[(size_t)i * (size_t)kOutIdsPerBall];
        const u64 h = smax + 1 - (u64)cands[g].arity;
        // VALIDATEUR D2H CENTRALISE : `cand_idx` attendu = base + gid — c'est
        // ICI que le chainage inter-lots est juge.
        if (const char* z = validate_ball_out(o.pstatus[i], o.cstatus[i], o.nint[i], o.nsh[i], o.cand[i],
                                              (u32)g, row, w.n_upos, o.count[i], h, total_mass)) {
          why = z;
          bad = i;
          break;
        }
        if (o.pstatus[i] == kBallStackOverflow || o.cstatus[i] == kBallStackOverflow) {
          why = "invariant : pile DFS au-dela du profil (49) sur la route device";
          bad = i;
          break;
        }
        if (o.pstatus[i] == kBallAtLeastH) continue;  // boule morte : rien d'autre a juger
        if (o.cstatus[i] == kBallShellOverflow) {
          why = "coquille au-dela du plafond (jamais de troncature)";
          bad = i;
          break;
        }
        if (o.cstatus[i] == kBallInteriorOverflow || (u64)o.nint[i] != o.count[i]) {
          why = "invariant : census contredit la passe count-only (route device)";
          bad = i;
          break;
        }
      }
      if (why != nullptr) {
        ring.fail_at(lot_id, bad, 10, why);  // index global = base + local
        err = why;
        break;
      }
      c.boules_validees += nb;
      StepResult s = ring.validate(lot_id);
      if (!s.ok()) {
        err = s.why;
        ring.fail_device(lot_id, 11, err);
        break;
      }
      s = ring.rebuild_begin(lot_id);
      if (!s.ok()) {
        err = s.why;
        ring.fail_device(lot_id, 12, err);
        break;
      }
      // PASSE 2 — RECONSTRUCTION SEQUENTIELLE dans les temporaires.
      reserve_growth(&lsurv, nb);
      reserve_growth(&lballs, nb);
      for (u32 i = 0; i < nb; ++i) {
        if (o.pstatus[i] == kBallAtLeastH) {
          ++dead;
          continue;
        }
        const size_t g = (size_t)base + i;
        const i32* row = &o.ids[(size_t)i * (size_t)kOutIdsPerBall];
        lsurv.push_back(Survivor{(u32)g, o.count[i]});
        const BallCandidate& bc = cands[g];
        BallData bd;
        bd.key = bc.key;
        bd.level = bc.level;
        bd.arity = bc.arity;
        bd.n_interior = o.nint[i];
        bd.n_shell = o.nsh[i];
        for (u8 j = 0; j < bd.n_interior; ++j) bd.interior_ids[j] = row[j];
        for (u8 j = 0; j < bd.n_shell; ++j) bd.shell_ids[j] = row[9 + j];
        lballs.push_back(bd);
        l_int += bd.n_interior;
        l_sh += bd.n_shell;
      }
      c.boules_reconstruites += nb;
      s = ring.rebuild_end(lot_id);  // rend le bail OUT (et lui seul)
      if (!s.ok()) {
        err = s.why;
        ring.fail_device(lot_id, 12, err);
        break;
      }
      ++next_reb;
      did = true;
    }

    // ---- ETAGE B : DESCENTE du lot calcule (rend le bail DEVICE, jamais OUT).
    if (err.empty() && next_down < next_ker) {
      const u32 lot_id = next_down;
      StepResult s = ring.d2h_begin(lot_id);
      if (s.refused()) {
        err = s.why;
        ring.fail_device(lot_id, 13, err);
        break;
      }
      if (s.ok()) {
        const LotTicket& t = ring.ticket(lot_id);
        const u32 q = t.slot[(int)Resource::kOut];
        const u32 dslot = t.slot[(int)Resource::kDevice];
        const u32 nb = t.nb;
        out_slabs[q].copy_from(dev_out[dslot], nb);
        // HOOK DE TEST : ecriture device corrompue a la descente (fixture de
        // corruption tardive — jamais un chemin produit).
        if (opt.tamper_on && lot_id == opt.tamper_lot && opt.tamper_local < nb)
          out_slabs[q].cand[opt.tamper_local] ^= 1u;
        s = ring.d2h_end(lot_id);
        if (!s.ok()) {
          err = s.why;
          ring.fail_device(lot_id, 13, err);
          break;
        }
        ++next_down;
        did = true;
      }
    }

    // ---- ETAGE C : MONTEE puis NOYAUX (un seul flux, un seul jeu device).
    if (err.empty() && next_ker < next_pack) {
      const u32 lot_id = next_ker;
      StepResult s = ring.h2d_begin(lot_id);
      if (s.refused()) {
        err = s.why;
        ring.fail_device(lot_id, 14, err);
        break;
      }
      if (s.ok()) {
        const LotTicket& t = ring.ticket(lot_id);
        const u32 p = t.slot[(int)Resource::kIn];
        const u32 q = t.slot[(int)Resource::kOut];
        const u32 dslot = t.slot[(int)Resource::kDevice];
        const u32 nb = t.nb;
        const u64 base = t.base_global;
        // SENTINELLES REMPLIES PAR L'HOTE dans le tampon OUT — donc sous le
        // bail OUT, qui vient d'etre pris par `h2d_begin`.
        out_slabs[q].fill_sentinels(nb);
        // MONTEE : les 112 octets par boule du tampon IN deviennent les mots
        // u64 que le device relit (decodage petit-boutiste EXPLICITE — jamais
        // un reinterpret_cast des octets cote hote).
        for (size_t j = 0; j < (size_t)nb * kBallWords; ++j)
          dev_words[dslot][j] = wire_detail::read_u64(in_slabs[p], j);
        dev_out[dslot].copy_from(out_slabs[q], nb);
        s = ring.h2d_end(lot_id);  // rend le bail IN : pack(k+1) est libre
        if (!s.ok()) {
          err = s.why;
          ring.fail_device(lot_id, 14, err);
          break;
        }
        s = ring.kernels_begin(lot_id);
        if (!s.ok()) {
          err = s.why;
          ring.fail_device(lot_id, 15, err);
          break;
        }
        MHGP7_LAUNCH(k_prefilter, (nb + 255) / 256, 256, nl, nr, nf, nlast, nbox, up, ws, w.root,
                     dev_words[dslot].data(), nb, dev_out[dslot].count.data(),
                     dev_out[dslot].pstatus.data(), opt.mut);
        // MUTANT gpu-lot-base-reset : base toujours 0 — le chainage
        // `cand_idx = base + gid` est rompu des le SECOND lot et le
        // validateur du lot refuse (scene a lots > 1 exigee).
        const u32 kbase = MHGP7_MUTANT("gpu-lot-base-reset") ? 0u : (u32)base;
        MHGP7_LAUNCH(k_census, (nb + 255) / 256, 256, nl, nr, nbox, up, w.root, dev_words[dslot].data(),
                     nb, kbase, (u32)shell_cap, dev_out[dslot].ids.data(), dev_out[dslot].cstatus.data(),
                     dev_out[dslot].nint.data(), dev_out[dslot].nsh.data(), dev_out[dslot].cand.data(),
                     opt.mut);
        s = ring.kernels_end(lot_id);
        if (!s.ok()) {
          err = s.why;
          ring.fail_device(lot_id, 15, err);
          break;
        }
        ++next_ker;
        did = true;
      }
    }

    // ---- ETAGE D : ENCODAGE du prochain lot dans un tampon IN libre.
    if (err.empty() && next_pack < n_lots) {
      const AdmitResult a = ring.admit();
      if (a.refused()) {
        err = a.why;
        break;
      }
      if (a.ok()) {
        if (a.lot != next_pack) {
          err = "invariant : admission hors ordre dans l'anneau C6";
          break;
        }
        const u32 lot_id = a.lot;
        const u32 p = ring.ticket(lot_id).slot[(int)Resource::kIn];
        // L'ENCODEUR ECRIT DEPUIS LA BASE DECLAREE PAR LE BAIL (doctrine de
        // lot_ring.hpp) : une base de bail fausse produit donc des OCTETS
        // faux, pas seulement un temoin faux.
        const Lease& lease = ring.lease(Resource::kIn, p);
        const u64 base = lease.base_global;
        const u32 nb = lease.nb;
        if (base > nb_total || (u64)nb > nb_total - base) {
          err = "invariant : plage de bail hors du domaine des candidats";
          break;
        }
        const CandidateSpan span{cands.data() + base, nb};
        const PackStatus ps = pack_candidate_range(in_slabs[p].data(), in_slabs[p].size(), 0, span, smax);
        if (ps != PackStatus::kOk) {
          err = pack_status_name(ps);
          ring.fail_device(lot_id, 20, err);
          break;
        }
        const StepResult s = ring.pack_end(lot_id);
        if (!s.ok()) {
          err = s.why;
          ring.fail_device(lot_id, 20, err);
          break;
        }
        c.boules_encodees += nb;
        ++next_pack;
        did = true;
      }
    }

    const u64 live = (u64)ring.live_tickets();
    if (live > c.max_live_tickets) c.max_live_tickets = live;
    if (!did && err.empty()) {
      err = "invariant : ordonnanceur C6 bloque (aucun etage n'a progresse)";
      break;
    }
  }

  // ------------------------------------------------------ ECHANGE TERMINAL
  // Le SEUL instant de visibilite, et c'est L'ANNEAU qui l'autorise : aucune
  // erreur retenue, tous les lots retires dans l'ordre de `base_global`,
  // aucun bail vivant. Un refus laisse `surv`, `balls` et les statistiques
  // EXACTEMENT dans l'etat ou l'appelant les avait.
  std::vector<u64> temoin;
  u64 temoin_shell = 0;
  std::string why;
  const bool autorise = ring.publish(&temoin, &temoin_shell, &why);
  if (autorise) {
    surv->swap(lsurv);
    balls->swap(lballs);
    st->census_interior += l_int;
    st->census_shell += l_sh;
    st->dead_depth = dead;
    st->survivors = surv->size();
  }
  const RingCounters& rk = ring.counters();
  c.lots_admis = rk.lots_admitted;
  c.lots_retires = rk.lots_retired;
  c.tails = rk.tails;
  c.rotations_in = rk.rotations[(int)Resource::kIn];
  c.rotations_device = rk.rotations[(int)Resource::kDevice];
  c.rotations_out = rk.rotations[(int)Resource::kOut];
  c.blocked_in = rk.blocked[(int)Resource::kIn];
  c.blocked_device = rk.blocked[(int)Resource::kDevice];
  c.blocked_out = rk.blocked[(int)Resource::kOut];

  if (!err.empty()) return err;
  if (!autorise) return why.empty() ? "invariant : echange terminal C6 refuse sans motif" : why;
  // TEMOIN D'ORDONNANCEMENT : le multiensemble fusionne par l'anneau doit
  // etre exactement 0..nb_total-1 DANS CET ORDRE — une rupture de bail, de
  // base globale ou d'ordre de retraite y est visible.
  if (opt.witness) {
    if ((u64)temoin.size() != nb_total) return "invariant : temoin d'anneau de taille inattendue";
    for (u64 i = 0; i < nb_total; ++i)
      if (temoin[i] != i) return "invariant : temoin d'anneau hors de l'ordre global";
  }
  (void)temoin_shell;
  return "";
}

}  // namespace gpu
}  // namespace mhgp7

#endif  // __CUDACC__ || MHGP7_FAKE_DEVICE

