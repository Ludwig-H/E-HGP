// Sonde des portes du socle (hors produit) : rend un code choisi, ecrit une ligne, ou meurt par signal. Sert a juger
// cmake/run_expect.cmake et cmake/expect_refusal.cmake eux-memes (tests/support/tests.cmake).
//   exit_probe [say <texte>]... [kill] [code <n>...]
//     say <texte>  ecrit <texte> et une fin de ligne sur la sortie standard
//     kill         se termine par SIGKILL (aucun sanitizer ne l'intercepte)
//     code <n>...  code de sortie = le DERNIER entier donne (les options de refus d'une porte s'ajoutent a la fin)
// Usage invalide : code 99.
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int main(int argc, char** argv) {
  int i = 1;
  while (i + 1 < argc && std::strcmp(argv[i], "say") == 0) {
    std::printf("%s\n", argv[i + 1]);
    i += 2;
  }
  std::fflush(stdout);
  if (i < argc && std::strcmp(argv[i], "kill") == 0) {
    std::raise(SIGKILL);
    return 99;
  }
  if (i + 1 < argc && std::strcmp(argv[i], "code") == 0) {
    char* end = nullptr;
    const long value = std::strtol(argv[argc - 1], &end, 10);
    if (end == argv[argc - 1] || *end != '\0' || value < 0 || value > 255) return 99;
    return static_cast<int>(value);
  }
  return 99;
}
