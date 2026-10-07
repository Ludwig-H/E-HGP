// Sonde de compilation : une largeur de coordonnees ne se construit que par CoordWidth::of, qui la controle.
// MHGP12_NEGATIVE=1 : construction directe depuis un entier, refusee (constructeur prive).
#include "cloud/cloud.hpp"

int main() {
#if defined(MHGP12_NEGATIVE) && MHGP12_NEGATIVE == 1
  const mhgp12::CoordWidth forged(99);
  return forged.bits();
#else
  const mhgp12::CoordWidth width;
  return width.bits() == mhgp12::kCoordBits ? 0 : 1;
#endif
}
