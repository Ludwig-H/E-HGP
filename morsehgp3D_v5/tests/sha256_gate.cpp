// MorseHGP3D v5 — porte SHA-256 : vecteurs FIPS 180-4 et egalite bit a bit du
// chemin accelere (SHA-NI, s'il est disponible) avec le chemin portable sur
// des tampons aleatoires de toutes tailles et decoupages. Codes : 0, 3.
#include <cstdio>
#include <random>
#include <string>
#include <vector>

#include "../src/core/sha256.hpp"

using namespace mhgp5;

int main() {
  int bad = 0;
  const auto hex_of = [](const std::string& msg, bool portable) {
    Sha256 h(portable);
    h.update(msg.data(), msg.size());
    return h.hex();
  };
  const struct { const char* msg; const char* want; } fips[] = {
      {"", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
      {"abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
      {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"},
  };
  for (const auto& f : fips)
    for (const bool portable : {true, false})
      if (hex_of(f.msg, portable) != f.want) { std::fprintf(stderr, "FIPS : %s (portable=%d)\n", f.msg, (int)portable); ++bad; }
  {  // un million de 'a'
    std::string a(1000000, 'a');
    for (const bool portable : {true, false})
      if (hex_of(a, portable) != "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0") { std::fprintf(stderr, "FIPS 1M a\n"); ++bad; }
  }
  // Egalite des deux chemins sur des tampons aleatoires et des decoupages arbitraires.
  std::mt19937_64 rng(42);
  u64 cases = 0;
  for (const size_t n : {1u, 55u, 56u, 63u, 64u, 65u, 127u, 1000u, 4096u, 65535u, 1u << 20}) {
    std::vector<uint8_t> buf(n);
    for (uint8_t& b : buf) b = (uint8_t)rng();
    Sha256 hp(true), ha(false);
    size_t i = 0;
    while (i < n) {
      const size_t k = std::min(n - i, (size_t)(1 + rng() % 200));
      hp.update(buf.data() + i, k);
      ha.update(buf.data() + i, k);
      i += k;
    }
    Sha256 whole(false);
    whole.update(buf.data(), n);
    const std::string a = hp.hex(), b = ha.hex(), c = whole.hex();
    if (a != b || a != c) { std::fprintf(stderr, "divergence portable/accelere a n=%zu\n", n); ++bad; }
    ++cases;
  }
  std::printf("sha256_gate accelere=%d fips=4 cas_aleatoires=%llu\n", (int)Sha256::accelerated(), (unsigned long long)cases);
  if (bad) return 3;
  std::printf("sha256_gate OK\n");
  return 0;
}
