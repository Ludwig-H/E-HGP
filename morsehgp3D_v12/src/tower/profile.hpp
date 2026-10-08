// Profil de l'etage G par composante (CONTRAT_TOUR.md, paragraphes 4.3 et 11 : MES-M7 sur le produit). Actif
// seulement si la construction definit MHGP12_TOWER_PROFILE (par exemple -DCMAKE_CXX_FLAGS=-DMHGP12_TOWER_PROFILE),
// hors de toute construction par defaut : alors chaque lecture du compteur est encadree par lfence (les chargements
// anterieurs sont termines avant la lecture, la latence d'une sonde reste dans sa section ; passe plus lente que la
// construction normale, comme la replique de MES-M7). Sinon chaque section est vide : ni instruction, ni sortie, ni
// compteur change. Les cycles sont PHYSIQUES : jamais dans une empreinte ni dans un compteur de l'objet ou du travail.
#pragma once

#include "tower/tower.hpp"

#if defined(MHGP12_TOWER_PROFILE)
#include <x86intrin.h>
#endif

namespace mhgp12::tower_detail {

#if defined(MHGP12_TOWER_PROFILE)
inline constexpr bool kProfile = true;
inline u64 profile_tick() noexcept {
  _mm_lfence();
  const u64 t = __rdtsc();
  _mm_lfence();
  return t;
}
#else
inline constexpr bool kProfile = false;
inline u64 profile_tick() noexcept { return 0; }
#endif

// Chronometre de sections d'un fil : lap(s) impute a la section s les cycles ecoules depuis le lap precedent (ou la
// construction) ; sink nul : rien n'est compte.
class SectionClock {
 public:
  explicit SectionClock(SectionCycles* sink) noexcept : sink_(sink), last_(profile_tick()) {}
  void lap(ProfileSection section) noexcept {
    if constexpr (kProfile) {
      const u64 now = profile_tick();
      if (sink_ != nullptr) {
        sink_->cycles[section] += now - last_;
        ++sink_->count[section];
      }
      last_ = now;
    }
  }
  // Cycles ecoules depuis start (lecture du compteur), imputes a section sans toucher au dernier lap.
  void charge(ProfileSection section, u64 start) noexcept {
    if constexpr (kProfile) {
      if (sink_ != nullptr) {
        sink_->cycles[section] += profile_tick() - start;
        ++sink_->count[section];
      }
    }
  }

 private:
  SectionCycles* sink_;
  u64 last_;
};

}  // namespace mhgp12::tower_detail
