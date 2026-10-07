// Sonde du SHA-256 du module io pour le differentiel contre hashlib (sha256_oracle.py). Hors produit.
// Entree standard : une ligne par cas, "<morceau> <message en hexadecimal>" (message vide : "<morceau> -").
// Sortie : une ligne par cas, l'empreinte en hexadecimal, le message etant absorbe par morceaux de <morceau> octets
// (0 : en une fois). Code 0, ou 2 sur une ligne mal formee.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "io/io.hpp"

namespace {

int nibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

}  // namespace

int main() {
  std::string line;
  while (std::getline(std::cin, line)) {
    const std::size_t space = line.find(' ');
    if (space == std::string::npos) return 2;
    const std::size_t chunk = std::stoul(line.substr(0, space));
    const std::string hex = line.substr(space + 1);
    std::vector<mhgp12::u8> message;
    if (hex != "-") {
      if (hex.size() % 2 != 0) return 2;
      for (std::size_t i = 0; i < hex.size(); i += 2) {
        const int hi = nibble(hex[i]), lo = nibble(hex[i + 1]);
        if (hi < 0 || lo < 0) return 2;
        message.push_back(static_cast<mhgp12::u8>(16 * hi + lo));
      }
    }
    mhgp12::io::Sha256 sha;
    const std::size_t step = chunk == 0 ? message.size() + 1 : chunk;
    for (std::size_t at = 0; at < message.size(); at += step)
      sha.update(std::span<const mhgp12::u8>(message.data() + at, std::min(step, message.size() - at)));
    const auto text = mhgp12::io::to_hex(sha.finish());
    std::printf("%.*s\n", static_cast<int>(text.size()), text.data());
  }
  return std::fflush(stdout) == 0 ? 0 : 2;
}
