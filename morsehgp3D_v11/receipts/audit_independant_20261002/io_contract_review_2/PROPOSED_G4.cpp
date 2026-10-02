// Cas propose pour G4, non compile ni execute par l'auditeur.
#include "io/sha256.hpp"
#include <string_view>

int main() {
  mhgp11::io::Sha256 h;
  if (!h.update("a").ok()) return 1;
  const auto before = h.digest();
  if (!h.update(std::string_view{}).ok()) return 1;
  if (h.bytes() != 1 || h.digest() != before) return 1;
  if (!h.update("bc").ok()) return 1;
  const auto hex = mhgp11::io::sha256_hex(h.digest());
  return std::string_view(hex.data(), hex.size()) ==
                 "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
             ? 0 : 1;
}
