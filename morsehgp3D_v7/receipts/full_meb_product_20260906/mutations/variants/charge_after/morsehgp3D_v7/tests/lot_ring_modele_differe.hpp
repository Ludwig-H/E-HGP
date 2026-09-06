// MorseHGP3D v6 — MODELE D'ORDONNANCEMENT DIFFERE C6 (auto-test du
// scheduler SEULEMENT). Ajout SEPARE demande par les auditeurs
// (REPONSE_AUDITEUR_CONCEPTION_C6_20260902 § 3) : tests/cuda_stub.hpp reste
// STRICTEMENT SEQUENTIEL et n'est pas modifie ; sa semantique reste l'oracle
// hote C2-C5.
//
// CE QUE CE MODELE EST : un executeur ou le TEST choisit l'ordre
// d'achevement. Chaque operation se lance (`lancer`) et entre dans une file
// EN VOL ; le test acheve celle de son choix (`achever(rang)`). Il force
// ainsi des fins inversees, le bouclage d'un emplacement, une queue et des
// scenarios d'erreur — sans aucun fil, sans aucun `sleep`, sans aucune
// horloge, donc sans aucun non-determinisme.
//
// CE QU'IL N'EST PAS : ni un device, ni une preuve d'absence de course. Il ne
// dit rien du materiel, rien de la visibilite memoire, rien du temps. Aucun
// verdict de performance n'en sort. La garde device C6a et la parite reelle
// en session G4 restent les seules preuves du device.
//
// TEMOINS INDEPENDANTS : l'occupation des emplacements est reconstruite ICI,
// depuis le journal des baux de l'anneau, sans consulter les etats internes —
// un emplacement repris avant que son bail soit rendu produit donc une
// DOUBLE OCCUPATION visible par le modele, meme si l'anneau (mute) ne
// bronche pas.
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../src/gpu/lot_ring.hpp"

namespace mhgp7 {
namespace test {

class ModeleDiffereC6 {
 public:
  enum class Op : u8 { kPack = 0, kH2d = 1, kKernels = 2, kD2h = 3, kRebuild = 4 };

  struct EnVol {
    Op op = Op::kPack;
    u32 lot = gpu::kNoLot;
    u64 rang_lancement = 0;
  };

  explicit ModeleDiffereC6(gpu::LotRing* ring) : ring_(ring) {
    for (int r = 0; r < gpu::kResourceCount; ++r)
      occ_[r].assign(ring_->depth((gpu::Resource)r), gpu::kNoLot);
  }

  static const char* op_name(Op op) {
    switch (op) {
      case Op::kPack: return "pack";
      case Op::kH2d: return "h2d";
      case Op::kKernels: return "kernels";
      case Op::kD2h: return "d2h";
      case Op::kRebuild: return "rebuild";
    }
    return "?";
  }

  // Lance le DEBUT d'une operation. `kPack` admet le prochain lot (son numero
  // est ensuite lisible par `dernier_lot()`).
  gpu::StepResult lancer(Op op, u32 lot = gpu::kNoLot) {
    gpu::StepResult r;
    u32 cible = lot;
    if (op == Op::kPack) {
      const gpu::AdmitResult a = ring_->admit();
      r.step = a.step;
      r.why = a.why;
      cible = a.lot;
      dernier_lot_ = a.lot;
    } else if (op == Op::kH2d) {
      r = ring_->h2d_begin(lot);
    } else if (op == Op::kKernels) {
      r = ring_->kernels_begin(lot);
    } else if (op == Op::kD2h) {
      r = ring_->d2h_begin(lot);
    } else {
      r = ring_->rebuild_begin(lot);
    }
    avancer_journal();
    if (r.ok()) {
      en_vol_.push_back(EnVol{op, cible, lancements_});
      ++lancements_;
    }
    observer();
    return r;
  }

  // Acheve l'operation de rang `rang` dans la file EN VOL — le TEST choisit,
  // jamais une horloge ni un ordonnanceur.
  gpu::StepResult achever(size_t rang, const gpu::D2hTamper& tamper = gpu::D2hTamper{}) {
    gpu::StepResult r;
    if (rang >= en_vol_.size()) {
      r.step = gpu::Step::kRefused;
      r.why = "achever : rang hors de la file en vol";
      return r;
    }
    const EnVol v = en_vol_[rang];
    switch (v.op) {
      case Op::kPack: r = ring_->pack_end(v.lot); break;
      case Op::kH2d: r = ring_->h2d_end(v.lot); break;
      case Op::kKernels: r = ring_->kernels_end(v.lot); break;
      case Op::kD2h: r = ring_->d2h_end(v.lot, tamper); break;
      case Op::kRebuild: r = ring_->rebuild_end(v.lot); break;
    }
    avancer_journal();
    if (r.ok()) {
      if (rang != 0) ++inversions_;  // achevement hors ordre de lancement
      en_vol_.erase(en_vol_.begin() + (std::ptrdiff_t)rang);
      ++achevements_;
    }
    observer();
    return r;
  }

  gpu::StepResult achever_fifo(const gpu::D2hTamper& tamper = gpu::D2hTamper{}) {
    return achever(0, tamper);
  }

  // La validation est une lecture hote ATOMIQUE : ni transfert, ni evenement,
  // donc rien a differer.
  gpu::StepResult valider(u32 lot) {
    const gpu::StepResult r = ring_->validate(lot);
    avancer_journal();
    observer();
    return r;
  }

  // Chaine d'un lot deja admis, en FIFO strict, jusqu'au LANCEMENT de sa
  // reconstruction (celle-ci reste EN VOL : le bail OUT court encore).
  gpu::StepResult lot_jusqu_au_rebuild(u32 lot, const gpu::D2hTamper& tamper = gpu::D2hTamper{}) {
    gpu::StepResult r;
    if (!(r = achever_rang_de(Op::kPack, lot)).ok()) return r;
    if (!(r = lancer(Op::kH2d, lot)).ok()) return r;
    if (!(r = achever_rang_de(Op::kH2d, lot)).ok()) return r;
    if (!(r = lancer(Op::kKernels, lot)).ok()) return r;
    if (!(r = achever_rang_de(Op::kKernels, lot)).ok()) return r;
    if (!(r = lancer(Op::kD2h, lot)).ok()) return r;
    if (!(r = achever_rang_de(Op::kD2h, lot, tamper)).ok()) return r;
    if (!(r = valider(lot)).ok()) return r;
    return lancer(Op::kRebuild, lot);
  }

  // Chaine complete d'un lot deja admis, reconstruction achevee comprise.
  gpu::StepResult lot_complet(u32 lot, const gpu::D2hTamper& tamper = gpu::D2hTamper{}) {
    const gpu::StepResult r = lot_jusqu_au_rebuild(lot, tamper);
    if (!r.ok()) return r;
    return achever_rang_de(Op::kRebuild, lot);
  }

  gpu::StepResult achever_rang_de(Op op, u32 lot, const gpu::D2hTamper& tamper = gpu::D2hTamper{}) {
    const size_t rang = rang_de(op, lot);
    if (rang == kAucunRang) {
      gpu::StepResult r;
      r.step = gpu::Step::kRefused;
      r.why = std::string("achever_rang_de : ") + op_name(op) + " du lot " + std::to_string(lot) +
              " n'est pas en vol";
      return r;
    }
    return achever(rang, tamper);
  }

  static constexpr size_t kAucunRang = (size_t)-1;

  size_t rang_de(Op op, u32 lot) const {
    for (size_t i = 0; i < en_vol_.size(); ++i)
      if (en_vol_[i].op == op && en_vol_[i].lot == lot) return i;
    return kAucunRang;
  }

  size_t en_vol() const { return en_vol_.size(); }
  const EnVol& vol(size_t rang) const { return en_vol_[rang]; }
  u32 dernier_lot() const { return dernier_lot_; }

  u64 doubles_occupations() const { return doubles_; }
  u64 liberations_orphelines() const { return orphelines_; }
  u64 inversions() const { return inversions_; }
  u64 lancements() const { return lancements_; }
  u64 achevements() const { return achevements_; }
  u32 pic_tickets() const { return pic_tickets_; }
  u64 observations_triple() const { return triples_; }
  u32 occupant(gpu::Resource r, u32 slot) const { return occ_[(int)r][slot]; }

 private:
  // Reconstruction TEMOIN de l'occupation, depuis le seul journal des baux.
  void avancer_journal() {
    const std::vector<gpu::LeaseEvent>& j = ring_->journal();
    while (lu_ < j.size()) {
      const gpu::LeaseEvent& e = j[lu_++];
      u32& o = occ_[(int)e.res][e.slot];
      if (e.acquire) {
        if (o != gpu::kNoLot) ++doubles_;  // DEUX lots sur le meme emplacement
        o = e.lot;
      } else {
        if (o == gpu::kNoLot) ++orphelines_;
        o = gpu::kNoLot;
      }
    }
  }

  // RECOUVREMENT A TROIS TICKETS : un lot dont la sortie est en
  // reconstruction, un lot sur le device, un lot en cours d'encodage.
  void observer() {
    const u32 vivants = ring_->live_tickets();
    if (vivants > pic_tickets_) pic_tickets_ = vivants;
    bool rebuild = false, device = false, pack = false;
    for (u32 k = 0; k < ring_->n_lots(); ++k) {
      const gpu::LotState s = ring_->ticket(k).state;
      if (s == gpu::LotState::kRebuilt) rebuild = true;
      if (s == gpu::LotState::kH2d || s == gpu::LotState::kKernels || s == gpu::LotState::kD2h)
        device = true;
      if (s == gpu::LotState::kPacking || s == gpu::LotState::kReady) pack = true;
    }
    if (rebuild && device && pack) ++triples_;
  }

  gpu::LotRing* ring_;
  std::vector<EnVol> en_vol_;
  std::vector<u32> occ_[gpu::kResourceCount];
  size_t lu_ = 0;
  u64 doubles_ = 0, orphelines_ = 0, inversions_ = 0, lancements_ = 0, achevements_ = 0, triples_ = 0;
  u32 pic_tickets_ = 0;
  u32 dernier_lot_ = gpu::kNoLot;
};

}  // namespace test
}  // namespace mhgp7

