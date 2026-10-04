// Sonde des acces verifies de Result (hors produit) : value() et take() sur un refus terminent le processus
// (std::terminate), jamais un comportement indefini ni une valeur inventee. tests.cmake attend l'arret anormal.
//   misuse_probe value_ok            code 0 : l'acces a la valeur d'un succes (temoin : la sonde elle-meme marche)
//   misuse_probe value_on_refusal    arret anormal
//   misuse_probe const_value_on_refusal, take_on_refusal    de meme
// Usage invalide : code 99. Si l'acces ne terminait pas, la sonde finirait par le code 0, que sa porte refuse.
#include <cstdio>
#include <cstring>
#include <utility>

#include "core/core.hpp"

int main(int argc, char** argv) {
  using namespace mhgp11;
  if (argc != 2) return 99;
  Result<int> good = 5;
  Result<int> refused = fail(Reason::memory_budget);
  const Result<int>& view = refused;
  int read = 0;
  if (std::strcmp(argv[1], "value_ok") == 0) read = good.value();
  else if (std::strcmp(argv[1], "value_on_refusal") == 0) read = refused.value();
  else if (std::strcmp(argv[1], "const_value_on_refusal") == 0) read = view.value();
  else if (std::strcmp(argv[1], "take_on_refusal") == 0) read = std::move(refused).take();
  else return 99;
  std::printf("valeur lue %d\n", read);
  return 0;
}
