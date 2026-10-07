// Sonde d'admission uniquement : aucun element de payload n'est lu.
#include "format.hpp"
#include <iostream>
int main(int argc, char** argv) {
  if (argc != 2) return 2;
  try {
    mhgp12::dump::Reader reader(argv[1]);
    const auto section = reader.get<mhgp12::dump::u32>("POPVAL");
    std::cout << "admitted " << section.second << '\n';
  } catch (const std::runtime_error&) {
    std::cout << "refused\n";
  }
  return 0;
}
