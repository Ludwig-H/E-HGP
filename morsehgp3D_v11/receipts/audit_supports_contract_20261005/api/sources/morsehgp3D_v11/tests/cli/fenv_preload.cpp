// Bibliotheque de prechargement de la porte mhgp11_cli_contract (hors produit) : demasque l'exception flottante
// inexacte avant main (LD_PRELOAD), sans aucune operation flottante. L'executable mhgp11, inchange, doit alors refuser
// a la creation de sa Session : environment_selftest, invariant viole, code 3, aucun dossier. C'est le seul chemin de
// code 3 du CLI qu'une porte puisse provoquer sans crochet dans le produit (regle 6).
#include <cfenv>

namespace {

[[gnu::constructor]] void mhgp11_unmask_inexact() {
#if defined(__GLIBC__)
  ::feenableexcept(FE_INEXACT);
#endif
}

}  // namespace
