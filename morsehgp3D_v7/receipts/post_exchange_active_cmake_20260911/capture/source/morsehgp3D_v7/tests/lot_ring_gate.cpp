// MorseHGP3D v6 — PORTE DE L'ANNEAU DE LOTS (src/gpu/lot_ring.hpp, C6
// jalons 1 et 3). Elle juge la DISCIPLINE D'ORDONNANCEMENT sous un modele
// d'achevements DIFFERES pilote par le test
// (tests/lot_ring_modele_differe.hpp) : aucun fil, aucun `sleep`, aucune
// horloge, aucun CUDA, aucune mesure de performance.
//
// CE QUI EST EXIGE :
//   (1) CHAINE D'ETATS complete FREE -> PACKING -> READY -> H2D -> KERNELS
//       -> D2H -> READY_HOST -> VALIDATED -> REBUILT -> FREE, avec des
//       machines et des epoques DISTINCTES pour IN et pour OUT ;
//   (2) DUREES DE VIE SEPAREES : IN rendu apres h2d_done, jeu device apres
//       d2h_done, OUT SEULEMENT apres rebuild_done — et recouvrement
//       effectif pack(k+1) / device(k) / rebuild(k-1) (trois tickets
//       logiques coexistants, plancher `--min-triples`) ;
//   (3) ATTENTE EXPLICITE : un transfert vers une sortie REUTILISEE est
//       BLOQUE tant que la reconstruction precedente n'a pas rendu son
//       bail — dans les deux variantes (sentinelles hote source du H2D, et
//       bail OUT pris au D2H) ;
//   (4) RETRAITE STRICTEMENT ORDONNEE par `base_global`, jamais par ordre
//       de fin : sous des achevements inverses, l'objet publie est
//       IDENTIQUE (scene d'equivalence des ordres) ;
//   (5) GRANULARITE `lot_ring_granularite_v1` : lot ENTIEREMENT valide avant
//       toute reconstruction de ce lot, lots anterieurs dans des temporaires
//       INVISIBLES, corruption d'un lot tardif = tout est jete (zero
//       enregistrement, zero compteur visible) ;
//   (6) REGLE D'ERREUR `lot_ring_erreur_v1` : la premiere erreur ferme
//       l'admission et draine, mais le VERDICT est le minimum lexicographique
//       de (index_global, rang_etage, code, message) — donc INDEPENDANT de
//       l'ordre d'arrivee, ce que la scene prouve en rejouant les deux ordres ;
//   (7) RAII : un pas qui echoue apres avoir pris un bail le rend ; l'abandon
//       rend tous les baux et jette la fusion invisible ;
//   (8) PLANCHERS de couverture (`--min-*`) : rotations d'anneau, queues,
//       scenarios d'erreur, achevements inverses, lots, blocages, triples.
//
// CE QUE CETTE PORTE NE PROUVE PAS : ni le device, ni l'absence de course
// reelle, ni un temps, ni un gain. Elle est l'AUTO-TEST DU SCHEDULER
// qualifie comme tel par les auditeurs (§ 3) ; tests/cuda_stub.hpp reste
// sequentiel et intact.
//
// Codes : 0 conforme ; 1 desaccord ; 2 refus d'argument ; 3 plancher ;
// 4 mutant tue. SELECTIVITE : chaque injection saute DIRECTEMENT a sa
// scene-signature, et chaque scene-signature est aussi exercee SANS
// injection dans le run nominal (aucune scene vide, aucune clause terminale
// « tue par n'importe quoi »).
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../src/core/parse.hpp"
#include "lot_ring_modele_differe.hpp"

using namespace mhgp7;
using gpu::LotRing;
using gpu::Resource;
using gpu::Step;
using test::ModeleDiffereC6;
using Op = ModeleDiffereC6::Op;

namespace {

int failures = 0;

struct Couverture {
  u64 lots = 0, rotations = 0, tails = 0, inversions = 0, erreurs = 0, blocages = 0, triples = 0;
  u64 scenes = 0;
};
Couverture cov;

void expect(bool ok, const char* what) {
  if (!ok) {
    std::printf("ECHEC : %s\n", what);
    ++failures;
  } else {
    std::printf("ok : %s\n", what);
  }
}

void moissonner(const LotRing& r, const ModeleDiffereC6& m) {
  const gpu::RingCounters& c = r.counters();
  cov.lots += c.lots_admitted;
  for (int i = 0; i < gpu::kResourceCount; ++i) {
    cov.rotations += c.rotations[i];
    cov.blocages += c.blocked[i];
  }
  cov.tails += c.tails;
  cov.erreurs += c.errors_recorded > 0 ? 1 : 0;
  cov.inversions += m.inversions();
  cov.triples += m.observations_triple();
  ++cov.scenes;
}

std::vector<u64> attendu(u64 nb_total) {
  std::vector<u64> v;
  v.reserve(nb_total);
  for (u64 i = 0; i < nb_total; ++i) v.push_back(i);
  return v;
}

u64 attendu_shell(u64 nb_total) {
  u64 s = 0;
  for (u64 i = 0; i < nb_total; ++i) s += i % 7;
  return s;
}

LotRing::Config conf(u64 nb_total, u32 lot, u32 in_slots, u32 out_slots, u32 dev_slots,
                     bool sentinel_source = true) {
  LotRing::Config c;
  c.nb_total = nb_total;
  c.lot = lot;
  c.in_slots = in_slots;
  c.out_slots = out_slots;
  c.device_slots = dev_slots;
  c.out_is_sentinel_source = sentinel_source;
  return c;
}

// ---------------------------------------------------------------- SCENE 1
// Chaine d'etats complete, epoques et bases par ressource, retraite ordonnee,
// echange terminal. Trois lots (dont une queue) sur un anneau de profondeur 2.
void scene_chaine_nominale() {
  LotRing r(conf(7, 3, 2, 2, 1));
  ModeleDiffereC6 m(&r);
  bool ok = r.config_error().empty() && r.n_lots() == 3;
  for (u32 k = 0; k < 3; ++k) {
    ok = ok && m.lancer(Op::kPack).ok();
    ok = ok && m.dernier_lot() == k;
    ok = ok && r.ticket(k).state == gpu::LotState::kPacking;
    ok = ok && m.achever_rang_de(Op::kPack, k).ok() && r.ticket(k).state == gpu::LotState::kReady;
    ok = ok && m.lancer(Op::kH2d, k).ok() && r.ticket(k).state == gpu::LotState::kH2d;
    // Le bail IN court encore PENDANT le transfert montant.
    ok = ok && r.lease(Resource::kIn, k % 2).state == gpu::LotState::kH2d;
    ok = ok && m.achever_rang_de(Op::kH2d, k).ok();
    // ... et il est rendu APRES h2d_done, pas avant.
    ok = ok && r.lease(Resource::kIn, k % 2).state == gpu::LotState::kFree;
    ok = ok && m.lancer(Op::kKernels, k).ok() && r.ticket(k).state == gpu::LotState::kKernels;
    ok = ok && m.achever_rang_de(Op::kKernels, k).ok();
    ok = ok && m.lancer(Op::kD2h, k).ok() && r.ticket(k).state == gpu::LotState::kD2h;
    ok = ok && m.achever_rang_de(Op::kD2h, k).ok() && r.ticket(k).state == gpu::LotState::kReadyHost;
    // Le jeu device est rendu apres d2h_done ; la SORTIE, elle, court encore.
    ok = ok && r.lease(Resource::kDevice, 0).state == gpu::LotState::kFree;
    ok = ok && r.lease(Resource::kOut, k % 2).state == gpu::LotState::kReadyHost;
    ok = ok && m.valider(k).ok() && r.ticket(k).state == gpu::LotState::kValidated;
    ok = ok && m.lancer(Op::kRebuild, k).ok() && r.ticket(k).state == gpu::LotState::kRebuilt;
    ok = ok && r.lease(Resource::kOut, k % 2).state == gpu::LotState::kRebuilt;
    ok = ok && m.achever_rang_de(Op::kRebuild, k).ok();
    ok = ok && r.lease(Resource::kOut, k % 2).state == gpu::LotState::kFree;
    ok = ok && r.ticket(k).state == gpu::LotState::kFree && r.next_retire() == k + 1;
  }
  expect(ok, "chaine FREE->PACKING->READY->H2D->KERNELS->D2H->READY_HOST->VALIDATED->REBUILT->FREE");
  // Epoques et bases : machines DISTINCTES pour IN et OUT, bouclage au lot 2.
  bool ep = r.ticket(0).epoch[(int)Resource::kIn] == 1 && r.ticket(1).epoch[(int)Resource::kIn] == 1 &&
            r.ticket(2).epoch[(int)Resource::kIn] == 2 && r.ticket(2).epoch[(int)Resource::kOut] == 2 &&
            r.ticket(2).epoch[(int)Resource::kDevice] == 3;
  ep = ep && r.ticket(2).lease_base[(int)Resource::kIn] == 6 &&
       r.ticket(2).lease_base[(int)Resource::kOut] == 6;
  ep = ep && r.counters().rotations[(int)Resource::kIn] == 1 &&
       r.counters().rotations[(int)Resource::kOut] == 1 &&
       r.counters().rotations[(int)Resource::kDevice] == 2;
  expect(ep, "epoques et bases globales par ressource : compteurs distincts, bouclage au lot 2");
  expect(r.counters().tails == 1 && r.ticket(2).nb == 1, "queue : dernier lot incomplet (nb=1)");
  std::vector<u64> out;
  u64 shell = 0;
  std::string why;
  const bool pub = r.publish(&out, &shell, &why);
  expect(pub && out == attendu(7) && shell == attendu_shell(7), "echange terminal : objet complet et ordonne");
  expect(r.counters().leases_live == 0 && r.counters().sentinel_reads == 0 &&
             r.counters().out_of_domain_reads == 0 && r.counters().merges_out_of_order == 0,
         "aucun bail pendu, aucune lecture de sentinelle, aucune fusion hors ordre");
  moissonner(r, m);
}

// ---------------------------------------------------------------- SCENE 2
// RECOUVREMENT : pack(k+1) pendant device(k) pendant rebuild(k-1). Trois
// tickets logiques coexistent sur deux paires IN/OUT a baux separes.
void scene_recouvrement() {
  LotRing r(conf(12, 2, 2, 2, 1));
  ModeleDiffereC6 m(&r);
  bool ok = r.n_lots() == 6;
  // Lot 0 jusqu'a la reconstruction EN COURS (le bail OUT0 court encore).
  ok = ok && m.lancer(Op::kPack).ok() && m.lot_jusqu_au_rebuild(0).ok();
  // Lot 1 sur le device (il tient le jeu device et OUT1).
  ok = ok && m.lancer(Op::kPack).ok() && m.achever_rang_de(Op::kPack, 1).ok();
  ok = ok && m.lancer(Op::kH2d, 1).ok() && m.achever_rang_de(Op::kH2d, 1).ok();
  ok = ok && m.lancer(Op::kKernels, 1).ok();
  // Lot 2 en cours d'encodage : il prend IN0, rendu par h2d_done(0) — donc
  // pack(k+1) tourne PENDANT device(k) et rebuild(k-1).
  ok = ok && m.lancer(Op::kPack).ok() && m.achever_rang_de(Op::kPack, 2).ok();
  const bool triple = r.ticket(0).state == gpu::LotState::kRebuilt &&
                      r.ticket(1).state == gpu::LotState::kKernels &&
                      r.ticket(2).state == gpu::LotState::kReady && r.live_tickets() == 3;
  expect(ok && triple, "recouvrement : rebuild(k-1), device(k) et pack(k+1) coexistent (3 tickets)");
  // Le jeu device est rendu (d2h_done du lot 1) : le SEUL bail manquant au
  // lot 2 est desormais celui de la sortie OUT0.
  bool suite = m.achever_rang_de(Op::kKernels, 1).ok() && m.lancer(Op::kD2h, 1).ok() &&
               m.achever_rang_de(Op::kD2h, 1).ok();
  suite = suite && r.lease(Resource::kDevice, 0).state == gpu::LotState::kFree;
  const gpu::StepResult bloque = m.lancer(Op::kH2d, 2);
  expect(suite && bloque.blocked(),
         "transfert vers une sortie reutilisee : BLOQUE tant que rebuild_done manque");
  expect(m.achever_rang_de(Op::kRebuild, 0).ok() && r.next_retire() == 1,
         "rebuild_done du lot 0 : bail OUT rendu, lot retire");
  expect(m.lancer(Op::kH2d, 2).ok(), "la montee du lot 2 passe des que le bail OUT est rendu");
  // Deroulement du reste.
  bool fin = m.achever_rang_de(Op::kH2d, 2).ok();
  fin = fin && m.valider(1).ok() && m.lancer(Op::kRebuild, 1).ok() &&
        m.achever_rang_de(Op::kRebuild, 1).ok();
  fin = fin && m.lancer(Op::kKernels, 2).ok() && m.achever_rang_de(Op::kKernels, 2).ok();
  fin = fin && m.lancer(Op::kD2h, 2).ok() && m.achever_rang_de(Op::kD2h, 2).ok();
  fin = fin && m.valider(2).ok() && m.lancer(Op::kRebuild, 2).ok() &&
        m.achever_rang_de(Op::kRebuild, 2).ok();
  for (u32 k = 3; k < 6; ++k) fin = fin && m.lancer(Op::kPack).ok() && m.lot_complet(k).ok();
  std::vector<u64> out;
  u64 shell = 0;
  std::string why;
  const bool pub = r.publish(&out, &shell, &why);
  expect(fin && pub && out == attendu(12) && shell == attendu_shell(12),
         "recouvrement : objet publie complet et dans l'ordre global");
  expect(m.pic_tickets() >= 3 && m.observations_triple() > 0,
         "pic de tickets logiques >= 3 (la profondeur deux porte sur IN et OUT, pas sur un slot unique)");
  moissonner(r, m);
}

// ---------------------------------------------------------------- SCENE 3
// BOUCLAGE de l'anneau sur cinq lots, dans les DEUX variantes de source des
// sentinelles : a `sentinel_source=false`, c'est le D2H lui-meme qui attend
// le bail OUT.
void scene_bouclage(bool sentinel_source) {
  LotRing r(conf(10, 2, 2, 2, 1, sentinel_source));
  ModeleDiffereC6 m(&r);
  bool ok = r.n_lots() == 5;
  u64 bloques = 0;
  // La reconstruction du lot k-2 n'est achevee qu'a l'instant ou le lot k
  // reclame le MEME emplacement de sortie : chaque bouclage passe donc par
  // une attente explicite du bail, jamais par un vol.
  for (u32 k = 0; k < 5; ++k) {
    ok = ok && m.lancer(Op::kPack).ok() && m.achever_rang_de(Op::kPack, k).ok();
    if (sentinel_source && k >= 2) {
      // Source des sentinelles = OUT : c'est la MONTEE qui attend.
      if (m.lancer(Op::kH2d, k).blocked()) ++bloques;
      ok = ok && m.achever_rang_de(Op::kRebuild, k - 2).ok();
    }
    ok = ok && m.lancer(Op::kH2d, k).ok() && m.achever_rang_de(Op::kH2d, k).ok();
    ok = ok && m.lancer(Op::kKernels, k).ok() && m.achever_rang_de(Op::kKernels, k).ok();
    if (!sentinel_source && k >= 2) {
      // Bail OUT pris au D2H : c'est la DESCENTE qui attend.
      if (m.lancer(Op::kD2h, k).blocked()) ++bloques;
      ok = ok && m.achever_rang_de(Op::kRebuild, k - 2).ok();
    }
    ok = ok && m.lancer(Op::kD2h, k).ok() && m.achever_rang_de(Op::kD2h, k).ok();
    ok = ok && m.valider(k).ok();
    ok = ok && m.lancer(Op::kRebuild, k).ok();  // reconstruction laissee EN VOL
  }
  ok = ok && m.achever_rang_de(Op::kRebuild, 3).ok() && m.achever_rang_de(Op::kRebuild, 4).ok();
  std::vector<u64> out;
  u64 shell = 0;
  std::string why;
  const bool pub = r.publish(&out, &shell, &why);
  const gpu::RingCounters& c = r.counters();
  expect(ok && pub && out == attendu(10) && shell == attendu_shell(10),
         sentinel_source ? "bouclage (sentinelles hote) : objet complet"
                         : "bouclage (bail OUT pris au D2H) : objet complet");
  expect(c.rotations[(int)Resource::kIn] >= 3 && c.rotations[(int)Resource::kOut] >= 3,
         "bouclage : au moins trois reutilisations d'emplacement par ressource");
  expect(bloques >= 3, "bouclage : la reutilisation d'une sortie a ete ATTENDUE, pas volee");
  expect(c.sentinel_reads == 0 && c.out_of_domain_reads == 0 && c.merges_out_of_order == 0,
         "bouclage : aucune lecture de sentinelle ni fusion hors ordre");
  moissonner(r, m);
}

// ---------------------------------------------------------------- SCENE 4
// FINS INVERSEES : deux jeux device (palier ulterieur), achevements forces
// dans l'ordre inverse a chaque etage. La retraite reste ordonnee.
void scene_fins_inversees() {
  LotRing r(conf(8, 2, 2, 2, 2));
  ModeleDiffereC6 m(&r);
  bool ok = true;
  for (u32 base = 0; base < 4; base += 2) {
    const u32 a = base, b = base + 1;
    ok = ok && m.lancer(Op::kPack).ok() && m.lancer(Op::kPack).ok();
    ok = ok && m.achever_rang_de(Op::kPack, b).ok() && m.achever_rang_de(Op::kPack, a).ok();
    ok = ok && m.lancer(Op::kH2d, a).ok() && m.lancer(Op::kH2d, b).ok();
    ok = ok && m.achever_rang_de(Op::kH2d, b).ok() && m.achever_rang_de(Op::kH2d, a).ok();
    ok = ok && m.lancer(Op::kKernels, a).ok() && m.lancer(Op::kKernels, b).ok();
    ok = ok && m.achever_rang_de(Op::kKernels, b).ok() && m.achever_rang_de(Op::kKernels, a).ok();
    ok = ok && m.lancer(Op::kD2h, a).ok() && m.lancer(Op::kD2h, b).ok();
    ok = ok && m.achever_rang_de(Op::kD2h, b).ok() && m.achever_rang_de(Op::kD2h, a).ok();
    ok = ok && m.valider(b).ok() && m.valider(a).ok();
    ok = ok && m.lancer(Op::kRebuild, a).ok() && m.lancer(Op::kRebuild, b).ok();
    // Le lot b termine sa reconstruction AVANT le lot a : la fusion doit
    // attendre a, sans quoi l'ordre global serait celui des achevements.
    ok = ok && m.achever_rang_de(Op::kRebuild, b).ok();
    const bool retenu = r.next_retire() == base && r.pending_size() == base * 2;
    ok = ok && retenu;
    ok = ok && m.achever_rang_de(Op::kRebuild, a).ok();
  }
  std::vector<u64> out;
  u64 shell = 0;
  std::string why;
  const bool pub = r.publish(&out, &shell, &why);
  expect(ok && pub && out == attendu(8) && shell == attendu_shell(8),
         "fins inversees a chaque etage : objet publie dans l'ordre de base_global");
  expect(m.inversions() >= 8, "fins inversees : achevements hors FIFO effectivement forces");
  expect(r.counters().merges_out_of_order == 0, "fins inversees : aucune fusion en ordre d'achevement");
  moissonner(r, m);
}

// ---------------------------------------------------------------- SCENE 5
// QUEUES : dernier lot incomplet, lot unique plus petit que la taille de lot,
// et division exacte (aucune queue) — les trois formes.
void scene_queues() {
  struct Cas {
    u64 nb;
    u32 lot;
    u64 tails;
  };
  const Cas cas[3] = {{5, 2, 1}, {3, 8, 1}, {6, 3, 0}};
  for (const Cas& t : cas) {
    LotRing r(conf(t.nb, t.lot, 2, 2, 1));
    ModeleDiffereC6 m(&r);
    bool ok = true;
    for (u32 k = 0; k < r.n_lots(); ++k) {
      ok = ok && m.lancer(Op::kPack).ok() && m.lot_complet(k).ok();
    }
    std::vector<u64> out;
    u64 shell = 0;
    std::string why;
    const bool pub = r.publish(&out, &shell, &why);
    char buf[160];
    std::snprintf(buf, sizeof buf, "queue : nb_total=%llu lot=%u -> %llu queue(s), objet complet",
                  (unsigned long long)t.nb, t.lot, (unsigned long long)t.tails);
    expect(ok && pub && out == attendu(t.nb) && shell == attendu_shell(t.nb) &&
               r.counters().tails == t.tails,
           buf);
    moissonner(r, m);
  }
}

// ---------------------------------------------------------------- SCENE 6
// ERREUR PRECOCE : panne device sur le premier lot, travail deja lance
// draine, admission fermee, rien de visible.
void scene_erreur_precoce() {
  LotRing r(conf(8, 2, 2, 2, 1));
  ModeleDiffereC6 m(&r);
  bool ok = m.lancer(Op::kPack).ok() && m.achever_rang_de(Op::kPack, 0).ok();
  ok = ok && m.lancer(Op::kH2d, 0).ok() && m.achever_rang_de(Op::kH2d, 0).ok();
  ok = ok && m.lancer(Op::kKernels, 0).ok();
  // Lot 1 deja admis et encode : c'est le travail a DRAINER.
  ok = ok && m.lancer(Op::kPack).ok() && m.achever_rang_de(Op::kPack, 1).ok();
  r.fail_device(0, 7, "cudaErrorLaunchFailure (simule)");
  expect(ok && !r.admission_open(), "erreur precoce : l'admission est fermee des la premiere erreur");
  expect(r.admit().refused(), "erreur precoce : aucune nouvelle admission");
  expect(m.lancer(Op::kH2d, 1).refused(), "erreur precoce : aucun nouveau transfert lance (drainage)");
  // DRAINAGE du travail deja lance : les fins sont acceptees.
  expect(m.achever_rang_de(Op::kKernels, 0).ok(), "erreur precoce : le travail deja lance se draine");
  std::vector<u64> out;
  u64 shell = 123;
  std::string why;
  const bool pub = r.publish(&out, &shell, &why);
  expect(!pub && out.empty() && shell == 123 && !why.empty(),
         "erreur precoce : echange terminal REFUSE, sortie de l'appelant intacte");
  expect(r.error().present && r.error().global_index == 0 && r.error().code == 7,
         "erreur precoce : erreur retenue au plus petit index global (0)");
  r.abort_all("fin de scene");
  expect(r.counters().leases_live == 0 && r.pending_size() == 0,
         "abandon RAII : tous les baux rendus, fusion invisible jetee");
  moissonner(r, m);
}

// ---------------------------------------------------------------- SCENE 7
// ERREUR TARDIVE (fixture demandee par les auditeurs) : deux lots deja
// reconstruits et fusionnes dans les temporaires INVISIBLES, corruption du
// dernier lot -> zero enregistrement visible.
int scene_erreur_tardive(bool injecte) {
  LotRing r(conf(6, 2, 2, 2, 1));
  ModeleDiffereC6 m(&r);
  bool ok = true;
  for (u32 k = 0; k < 2; ++k) ok = ok && m.lancer(Op::kPack).ok() && m.lot_complet(k).ok();
  const bool prefixe_interne = r.pending_size() == 4;
  gpu::D2hTamper t;
  t.on = true;
  t.local = 1;
  t.bad_value = true;
  ok = ok && m.lancer(Op::kPack).ok() && m.achever_rang_de(Op::kPack, 2).ok();
  ok = ok && m.lancer(Op::kH2d, 2).ok() && m.achever_rang_de(Op::kH2d, 2).ok();
  ok = ok && m.lancer(Op::kKernels, 2).ok() && m.achever_rang_de(Op::kKernels, 2).ok();
  ok = ok && m.lancer(Op::kD2h, 2).ok() && m.achever_rang_de(Op::kD2h, 2, t).ok();
  const bool refus = m.valider(2).refused();
  std::vector<u64> out;
  u64 shell = 0;
  std::string why;
  const bool pub = r.publish(&out, &shell, &why);
  if (injecte) {
    if (pub || !out.empty()) {
      std::printf("mutant c6-publish-prefix TUE : prefixe publie (%zu enregistrements, publish=%s)\n",
                  out.size(), pub ? "vrai" : "faux");
      return 4;
    }
    std::printf("MUTANT NON TUE : c6-publish-prefix n'a rien publie\n");
    return 1;
  }
  expect(ok && prefixe_interne && refus, "erreur tardive : deux lots dans des temporaires INVISIBLES, refus au lot 2");
  expect(!pub && out.empty() && shell == 0,
         "erreur tardive : zero enregistrement, zero compteur visible (rien avant l'echange terminal)");
  expect(r.error().present && r.error().global_index == 5,
         "erreur tardive : index global exact de l'enregistrement fautif");
  r.abort_all("fin de scene");
  moissonner(r, m);
  return 0;
}

// ---------------------------------------------------------------- SCENE 8
// REGLE GLOBALE DETERMINISTE : deux fautes, l'ARRIVEE inversee par rapport a
// l'index global. Le verdict est le meme dans les deux sens.
u64 scene_regle_erreur(bool tardif_dabord) {
  LotRing r(conf(6, 2, 2, 2, 1));
  ModeleDiffereC6 m(&r);
  bool ok = m.lancer(Op::kPack).ok() && m.lot_complet(0).ok();
  gpu::D2hTamper t;
  t.on = true;
  t.local = 0;
  t.bad_value = true;
  for (u32 k = 1; k < 3; ++k) {
    ok = ok && m.lancer(Op::kPack).ok() && m.achever_rang_de(Op::kPack, k).ok();
    ok = ok && m.lancer(Op::kH2d, k).ok() && m.achever_rang_de(Op::kH2d, k).ok();
    ok = ok && m.lancer(Op::kKernels, k).ok() && m.achever_rang_de(Op::kKernels, k).ok();
    ok = ok && m.lancer(Op::kD2h, k).ok() && m.achever_rang_de(Op::kD2h, k, t).ok();
  }
  if (tardif_dabord) {
    ok = ok && m.valider(2).refused() && m.valider(1).refused();
  } else {
    ok = ok && m.valider(1).refused() && m.valider(2).refused();
  }
  expect(ok && r.all_errors().size() == 2, "regle d'erreur : les deux fautes sont enregistrees");
  const u64 retenu = r.error().global_index;
  r.abort_all("fin de scene");
  moissonner(r, m);
  return retenu;
}

// ---------------------------------------------------------------- SCENE 9
// EQUIVALENCE DES ORDRES D'ACHEVEMENT : le meme travail, trois ordres, un
// seul objet publie.
void scene_equivalence_ordres() {
  std::vector<u64> ref;
  u64 ref_shell = 0;
  bool ok = true;
  for (int variante = 0; variante < 3; ++variante) {
    LotRing r(conf(8, 2, 2, 2, 2));
    ModeleDiffereC6 m(&r);
    bool v = true;
    for (u32 base = 0; base < 4; base += 2) {
      const u32 a = base, b = base + 1;
      v = v && m.lancer(Op::kPack).ok() && m.lancer(Op::kPack).ok();
      // variante 0 : FIFO ; 1 : inverse ; 2 : alterne.
      if (variante == 0) v = v && m.achever_rang_de(Op::kPack, a).ok() && m.achever_rang_de(Op::kPack, b).ok();
      else v = v && m.achever_rang_de(Op::kPack, b).ok() && m.achever_rang_de(Op::kPack, a).ok();
      v = v && m.lancer(Op::kH2d, a).ok() && m.lancer(Op::kH2d, b).ok();
      if (variante == 1) v = v && m.achever_rang_de(Op::kH2d, b).ok() && m.achever_rang_de(Op::kH2d, a).ok();
      else v = v && m.achever_rang_de(Op::kH2d, a).ok() && m.achever_rang_de(Op::kH2d, b).ok();
      v = v && m.lancer(Op::kKernels, a).ok() && m.lancer(Op::kKernels, b).ok();
      if (variante == 2) v = v && m.achever_rang_de(Op::kKernels, b).ok() && m.achever_rang_de(Op::kKernels, a).ok();
      else v = v && m.achever_rang_de(Op::kKernels, a).ok() && m.achever_rang_de(Op::kKernels, b).ok();
      v = v && m.lancer(Op::kD2h, a).ok() && m.lancer(Op::kD2h, b).ok();
      if (variante != 0) v = v && m.achever_rang_de(Op::kD2h, b).ok() && m.achever_rang_de(Op::kD2h, a).ok();
      else v = v && m.achever_rang_de(Op::kD2h, a).ok() && m.achever_rang_de(Op::kD2h, b).ok();
      v = v && m.valider(a).ok() && m.valider(b).ok();
      v = v && m.lancer(Op::kRebuild, a).ok() && m.lancer(Op::kRebuild, b).ok();
      if (variante == 0) v = v && m.achever_rang_de(Op::kRebuild, a).ok() && m.achever_rang_de(Op::kRebuild, b).ok();
      else v = v && m.achever_rang_de(Op::kRebuild, b).ok() && m.achever_rang_de(Op::kRebuild, a).ok();
    }
    std::vector<u64> out;
    u64 shell = 0;
    std::string why;
    v = v && r.publish(&out, &shell, &why);
    if (variante == 0) {
      ref = out;
      ref_shell = shell;
    } else {
      v = v && out == ref && shell == ref_shell;
    }
    ok = ok && v && out == attendu(8);
    moissonner(r, m);
  }
  expect(ok, "equivalence : trois ordres d'achevement, un seul objet publie");
}

// --------------------------------------------------------------- SCENE 10
// RAII : un pas qui prend un bail puis echoue le REND. La montee du lot 1
// obtient OUT1 puis se heurte au jeu device occupe : OUT1 doit revenir libre.
void scene_raii_bail_rendu() {
  LotRing r(conf(4, 2, 2, 2, 1));
  ModeleDiffereC6 m(&r);
  bool ok = m.lancer(Op::kPack).ok() && m.achever_rang_de(Op::kPack, 0).ok();
  ok = ok && m.lancer(Op::kH2d, 0).ok();  // le lot 0 tient le jeu device
  ok = ok && m.lancer(Op::kPack).ok() && m.achever_rang_de(Op::kPack, 1).ok();
  const u64 avant = r.counters().leases_live;
  const gpu::StepResult bloque = m.lancer(Op::kH2d, 1);
  expect(ok && bloque.blocked(), "RAII : la montee du lot 1 est bloquee par le jeu device");
  expect(r.lease(Resource::kOut, 1).state == gpu::LotState::kFree && r.counters().leases_live == avant,
         "RAII : le bail OUT pris avant l'echec est RENDU (aucun bail pendu)");
  r.abort_all("fin de scene");
  expect(r.counters().leases_live == 0, "RAII : l'abandon rend tous les baux");
  moissonner(r, m);
}

// --------------------------------------------------------------- SCENE 11
// DOMAINE : les configurations hors domaine sont REFUSEES avant toute
// allocation, et l'anneau refuse alors toute operation.
void scene_domaine() {
  const LotRing::Config mauvais[5] = {conf(0, 4, 2, 2, 1), conf(8, 0, 2, 2, 1), conf(8, 4, 0, 2, 1),
                                      conf(8, 4, 2, 9, 1), conf(8, 4, 2, 2, 0)};
  bool ok = true;
  for (const LotRing::Config& c : mauvais) {
    LotRing r(c);
    ok = ok && !r.config_error().empty() && r.admit().refused();
  }
  expect(ok, "domaine : nb_total, lot et profondeurs hors domaine sont refuses (jamais clampes)");
}

// --------------------------------------------------------------- SCENE 12
// TRANSITIONS ILLEGALES : la chaine d'etats n'est pas decorative — chaque
// raccourci est REFUSE, et le refus ne consomme ni ne publie rien.
void scene_transitions_illegales() {
  LotRing r(conf(4, 2, 2, 2, 1));
  ModeleDiffereC6 m(&r);
  bool ok = m.lancer(Op::kPack).ok();
  ok = ok && r.h2d_begin(0).refused();          // montee avant la fin de l'encodage
  ok = ok && r.validate(0).refused();           // validation avant toute descente
  ok = ok && m.achever_rang_de(Op::kPack, 0).ok();
  ok = ok && r.pack_end(0).refused();           // second pack_end
  ok = ok && r.kernels_begin(0).refused();      // kernels avant la montee
  ok = ok && m.lancer(Op::kH2d, 0).ok();
  ok = ok && r.kernels_begin(0).refused();      // kernels avant h2d_done
  ok = ok && m.achever_rang_de(Op::kH2d, 0).ok();
  ok = ok && m.lancer(Op::kKernels, 0).ok();
  ok = ok && r.d2h_begin(0).refused();          // descente avant la fin des kernels
  ok = ok && m.achever_rang_de(Op::kKernels, 0).ok();
  ok = ok && m.lancer(Op::kD2h, 0).ok() && m.achever_rang_de(Op::kD2h, 0).ok();
  ok = ok && r.rebuild_begin(0).refused();      // reconstruction avant validation
  ok = ok && m.valider(0).ok() && m.lancer(Op::kRebuild, 0).ok();
  std::vector<u64> out;
  u64 shell = 0;
  std::string why;
  ok = ok && !r.publish(&out, &shell, &why) && out.empty();  // publication avant la fin
  expect(ok, "transitions illegales : chaque raccourci de la chaine est refuse, rien n'est publie");
  r.abort_all("fin de scene");
  moissonner(r, m);
}

// ------------------------------------------------- SCENES-SIGNATURE (1, 3, 5)
// Chacune est exercee SANS injection (contrat affirme) et AVEC (signature du
// defaut) — jamais une clause terminale « tue par n'importe quoi ».

int scene_reuse_before_lease(bool injecte) {
  LotRing r(conf(6, 2, 2, 2, 1));
  ModeleDiffereC6 m(&r);
  bool ok = m.lancer(Op::kPack).ok() && m.lot_jusqu_au_rebuild(0).ok();
  ok = ok && m.lancer(Op::kPack).ok() && m.lot_complet(1).ok();
  ok = ok && m.lancer(Op::kPack).ok() && m.achever_rang_de(Op::kPack, 2).ok();
  // Le lot 2 vise OUT0, tenu par la reconstruction EN COURS du lot 0.
  const gpu::StepResult s = m.lancer(Op::kH2d, 2);
  if (injecte) {
    if (!s.ok()) {
      std::printf("MUTANT NON TUE : la reutilisation a ete bloquee (%s)\n", s.why.c_str());
      return 1;
    }
    const bool fini = m.achever_rang_de(Op::kRebuild, 0).ok();
    const bool tue = m.doubles_occupations() >= 1 && r.counters().sentinel_reads >= 1;
    std::printf("mutant c6-reuse-before-lease %s : doubles_occupations=%llu sentinelles_lues=%llu\n",
                tue ? "TUE" : "NON TUE", (unsigned long long)m.doubles_occupations(),
                (unsigned long long)r.counters().sentinel_reads);
    r.abort_all("fin de scene mutante");
    return (fini && tue) ? 4 : 1;
  }
  expect(ok && s.blocked(), "signature 1 : la sortie n'est pas reprise avant que son bail soit rendu");
  expect(m.doubles_occupations() == 0 && m.liberations_orphelines() == 0,
         "signature 1 : temoin d'occupation independant sans double occupation");
  expect(m.achever_rang_de(Op::kRebuild, 0).ok() && m.lancer(Op::kH2d, 2).ok() &&
             r.counters().sentinel_reads == 0,
         "signature 1 : la montee passe apres rebuild_done, sans lecture de sentinelle");
  r.abort_all("fin de scene");
  moissonner(r, m);
  return 0;
}

int scene_merge_by_completion(bool injecte) {
  LotRing r(conf(4, 2, 2, 2, 1));
  ModeleDiffereC6 m(&r);
  bool ok = m.lancer(Op::kPack).ok() && m.lot_jusqu_au_rebuild(0).ok();
  ok = ok && m.lancer(Op::kPack).ok() && m.lot_jusqu_au_rebuild(1).ok();
  // Le lot 1 acheve sa reconstruction AVANT le lot 0.
  ok = ok && m.achever_rang_de(Op::kRebuild, 1).ok();
  const size_t apres_lot1 = r.pending_size();
  ok = ok && m.achever_rang_de(Op::kRebuild, 0).ok();
  std::vector<u64> out;
  u64 shell = 0;
  std::string why;
  const bool pub = r.publish(&out, &shell, &why);
  if (injecte) {
    const bool tue = !pub || out != attendu(4) || r.counters().merges_out_of_order > 0;
    std::printf("mutant c6-merge-by-completion %s : fusion=[", tue ? "TUE" : "NON TUE");
    for (size_t i = 0; i < out.size(); ++i) std::printf("%s%llu", i ? "," : "", (unsigned long long)out[i]);
    std::printf("] hors_ordre=%llu\n", (unsigned long long)r.counters().merges_out_of_order);
    return tue ? 4 : 1;
  }
  expect(ok && apres_lot1 == 0, "signature 2 : un lot fini hors tour reste dans son temporaire invisible");
  expect(pub && out == attendu(4) && r.counters().merges_out_of_order == 0,
         "signature 2 : la retraite suit base_global, jamais l'ordre d'achevement");
  moissonner(r, m);
  return 0;
}

int scene_wrong_epoch(bool injecte) {
  LotRing r(conf(6, 2, 2, 2, 1));
  ModeleDiffereC6 m(&r);
  bool ok = true;
  for (u32 k = 0; k < 2; ++k) ok = ok && m.lancer(Op::kPack).ok() && m.lot_complet(k).ok();
  // Lot 2 : bouclage sur IN0 et OUT0 (epoque 2, base globale 4).
  ok = ok && m.lancer(Op::kPack).ok() && m.achever_rang_de(Op::kPack, 2).ok();
  const u64 ep = r.ticket(2).epoch[(int)Resource::kIn];
  const u64 base = r.ticket(2).lease_base[(int)Resource::kIn];
  ok = ok && m.lancer(Op::kH2d, 2).ok() && m.achever_rang_de(Op::kH2d, 2).ok();
  ok = ok && m.lancer(Op::kKernels, 2).ok() && m.achever_rang_de(Op::kKernels, 2).ok();
  ok = ok && m.lancer(Op::kD2h, 2).ok() && m.achever_rang_de(Op::kD2h, 2).ok();
  const gpu::StepResult v = m.valider(2);
  if (injecte) {
    const bool temoin = ep != 2 || base != 4;
    const bool refus = v.refused() && r.error().present && r.error().global_index == 4 &&
                       r.error().code == 2;
    std::printf("mutant c6-wrong-epoch %s : epoque=%llu base_bail=%llu refus=%s\n",
                (temoin && refus) ? "TUE" : "NON TUE", (unsigned long long)ep,
                (unsigned long long)base, v.refused() ? "oui" : "non");
    r.abort_all("fin de scene mutante");
    return (temoin && refus) ? 4 : 1;
  }
  expect(ok && ep == 2 && base == 4,
         "signature 3 : au bouclage, l'epoque et la base globale sont celles du NOUVEL occupant");
  expect(v.ok() && !r.error().present, "signature 3 : le lot boucle est valide sans faute");
  r.abort_all("fin de scene");
  moissonner(r, m);
  return 0;
}

int scene_rebuild_before_validate(bool injecte) {
  LotRing r(conf(2, 2, 2, 2, 1));
  ModeleDiffereC6 m(&r);
  gpu::D2hTamper t;
  t.on = true;
  t.local = 0;
  t.bad_shell = true;
  bool ok = m.lancer(Op::kPack).ok() && m.achever_rang_de(Op::kPack, 0).ok();
  ok = ok && m.lancer(Op::kH2d, 0).ok() && m.achever_rang_de(Op::kH2d, 0).ok();
  ok = ok && m.lancer(Op::kKernels, 0).ok() && m.achever_rang_de(Op::kKernels, 0).ok();
  ok = ok && m.lancer(Op::kD2h, 0).ok() && m.achever_rang_de(Op::kD2h, 0, t).ok();
  const gpu::StepResult rb = m.lancer(Op::kRebuild, 0);  // AVANT toute validation
  if (injecte) {
    if (!rb.ok()) {
      std::printf("MUTANT NON TUE : la reconstruction anticipee a ete refusee (%s)\n", rb.why.c_str());
      return 1;
    }
    const bool fini = m.achever_rang_de(Op::kRebuild, 0).ok();
    const bool tue = r.counters().out_of_domain_reads >= 1;
    std::printf("mutant c6-rebuild-before-validate %s : lectures_hors_domaine=%llu\n",
                tue ? "TUE" : "NON TUE", (unsigned long long)r.counters().out_of_domain_reads);
    r.abort_all("fin de scene mutante");
    return (fini && tue) ? 4 : 1;
  }
  expect(ok && rb.refused(), "signature 4 : aucune reconstruction avant que le lot ENTIER soit valide");
  const gpu::StepResult v = m.valider(0);
  expect(v.refused() && r.error().code == 3 && r.counters().out_of_domain_reads == 0,
         "signature 4 : la longueur hors domaine est refusee au validateur, jamais lue comme borne");
  r.abort_all("fin de scene");
  moissonner(r, m);
  return 0;
}

// Les cinq noms `c6-*` sont au registre partage (src/core/mutants.hpp) depuis
// le cablage de cette porte : le repli local qui les activait en attendant a
// ete retire. Un nom hors registre reste un refus (code 2).
bool activer_mutant(const std::string& nom) { return mutants_enable(nom); }

}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  // PLANCHERS graves a ~50 % des valeurs MESUREES sur ce depot le 2 septembre
  // (scenes=20 lots=63 rotations=87 queues=3 inversions=76 erreurs=5
  // blocages=9 triples=4) : une couverture vide rend 3, jamais 0.
  i64 min_rotations = 40, min_tails = 2, min_erreurs = 2, min_inversions = 36, min_lots = 30,
      min_blocages = 4, min_triples = 2;
  bool ok = true;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    const auto val = [&](const char* p) -> const char* {
      const size_t l = std::strlen(p);
      return a.compare(0, l, p) == 0 ? a.c_str() + l : nullptr;
    };
    i64 v = 0;
    if (const char* s = val("--inject=")) inject = s;
    else if (const char* s = val("--min-rotations=")) { ok = parse_i64_exact(s, &v) && v >= 0 && ok; min_rotations = v; }
    else if (const char* s = val("--min-tails=")) { ok = parse_i64_exact(s, &v) && v >= 0 && ok; min_tails = v; }
    else if (const char* s = val("--min-erreurs=")) { ok = parse_i64_exact(s, &v) && v >= 0 && ok; min_erreurs = v; }
    else if (const char* s = val("--min-inversions=")) { ok = parse_i64_exact(s, &v) && v >= 0 && ok; min_inversions = v; }
    else if (const char* s = val("--min-lots=")) { ok = parse_i64_exact(s, &v) && v >= 0 && ok; min_lots = v; }
    else if (const char* s = val("--min-blocages=")) { ok = parse_i64_exact(s, &v) && v >= 0 && ok; min_blocages = v; }
    else if (const char* s = val("--min-triples=")) { ok = parse_i64_exact(s, &v) && v >= 0 && ok; min_triples = v; }
    else {
      std::fprintf(stderr, "REFUS : argument inconnu %s\n", a.c_str());
      return 2;
    }
  }
  if (!ok) {
    std::fprintf(stderr, "REFUS : parametre mal forme ou hors domaine\n");
    return 2;
  }
  if (!inject.empty() && !activer_mutant(inject)) {
    std::fprintf(stderr, "REFUS : mutant inconnu %s\n", inject.c_str());
    return 2;
  }

  // SELECTIVITE : chaque injection saute directement a sa scene-signature.
  if (inject == "c6-reuse-before-lease") return scene_reuse_before_lease(true);
  if (inject == "c6-merge-by-completion") return scene_merge_by_completion(true);
  if (inject == "c6-wrong-epoch") return scene_wrong_epoch(true);
  if (inject == "c6-publish-prefix") return scene_erreur_tardive(true);
  if (inject == "c6-rebuild-before-validate") return scene_rebuild_before_validate(true);
  if (!inject.empty()) {
    std::fprintf(stderr, "REFUS : mutant sans scene-signature dans cette porte\n");
    return 2;
  }

  scene_chaine_nominale();
  scene_recouvrement();
  scene_bouclage(true);
  scene_bouclage(false);
  scene_fins_inversees();
  scene_queues();
  scene_erreur_precoce();
  (void)scene_erreur_tardive(false);
  // REGLE GLOBALE : les deux ordres d'ARRIVEE retiennent la MEME erreur.
  const u64 a = scene_regle_erreur(true);
  const u64 b = scene_regle_erreur(false);
  expect(a == 2 && b == 2,
         "regle d'erreur deterministe : le verdict est le plus petit index global, pas la premiere arrivee");
  scene_equivalence_ordres();
  scene_raii_bail_rendu();
  scene_domaine();
  scene_transitions_illegales();
  (void)scene_reuse_before_lease(false);
  (void)scene_merge_by_completion(false);
  (void)scene_wrong_epoch(false);
  (void)scene_rebuild_before_validate(false);

  std::printf(
      "lot_ring_gate scenes=%llu lots=%llu rotations=%llu queues=%llu inversions=%llu erreurs=%llu "
      "blocages=%llu triples=%llu echecs=%d\n",
      (unsigned long long)cov.scenes, (unsigned long long)cov.lots, (unsigned long long)cov.rotations,
      (unsigned long long)cov.tails, (unsigned long long)cov.inversions, (unsigned long long)cov.erreurs,
      (unsigned long long)cov.blocages, (unsigned long long)cov.triples, failures);
  if (failures) return 1;
  if (cov.lots < (u64)min_lots || cov.rotations < (u64)min_rotations || cov.tails < (u64)min_tails ||
      cov.inversions < (u64)min_inversions || cov.erreurs < (u64)min_erreurs ||
      cov.blocages < (u64)min_blocages || cov.triples < (u64)min_triples) {
    std::printf("PLANCHER : couverture insuffisante (le vert par vacuite est refuse)\n");
    return 3;
  }
  return 0;
}

