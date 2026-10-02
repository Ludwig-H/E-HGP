// Sonde de compilation : une largeur de coordonnees ne se construit que par CoordWidth::of, qui la controle.
// MHGP11_NEGATIVE=1 : construction directe depuis un entier, refusee (constructeur prive).
#include "cloud/cloud.hpp"

int main() {
#if defined(MHGP11_NEGATIVE) && MHGP11_NEGATIVE == 1
  const mhgp11::CoordWidth forged(99);
  return forged.bits();
#else
  const mhgp11::CoordWidth width;
  return width.bits() == mhgp11::kCoordBits ? 0 : 1;
#endif
}
