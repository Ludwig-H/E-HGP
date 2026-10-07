// Arene d'emissions des formes v12 (MES-M2, hors produit) : 16 octets par boule, 1 par incidence, places prises par
// atomique dans l'ordre d'arrivee des warps ; chaque enregistrement porte sa feuille. Disposition commune a l'appareil
// et a l'hote ; la verification (hote seulement) regroupe par feuille et compare au vidage de reference.
#pragma once

#include <cstdint>

namespace mhgp12::leaf {

struct ArenaRecord {
  std::uint32_t leaf;
  std::uint8_t support[4];  // S* en rangs locaux, 0xFF au-dela de qmin
  std::uint8_t p, m, qmin, q;
  std::uint32_t population_at;  // premiere incidence (rangs locaux, I puis U)
};
static_assert(sizeof(ArenaRecord) == 16, "arene : 16 octets par boule");

}  // namespace mhgp12::leaf
