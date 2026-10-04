// Porte de core sous MHGP11_POISON : tout octet alloue par Buffer vaut 0xA5 tant qu'il n'est pas ecrit. La porte
// n'est enregistree que dans une construction MHGP11_POISON (tests.cmake) : sans empoisonnement, lire ces octets
// serait lire une memoire non initialisee.
#include "core/core.hpp"
#include "test.hpp"

using namespace mhgp11;

MHGP11_TEST(poison, 7) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  Buffer<u8> bytes;
  REQUIRE(bytes.allocate(4096, budget).ok());
  u64 poisoned = 0;
  for (const u8 b : bytes) poisoned += b == 0xA5 ? 1 : 0;
  CHECK_EQ(poisoned, 4096u);

  Buffer<u64> words;
  REQUIRE(words.allocate(512, budget).ok());
  poisoned = 0;
  for (const u64 w : words) poisoned += w == 0xA5A5A5A5A5A5A5A5u ? 1 : 0;
  CHECK_EQ(poisoned, 512u);

  // un bloc rendu puis repris est de nouveau empoisonne, meme si l'allocateur rend la meme adresse
  for (u8& b : bytes) b = 0x11;
  bytes.reset();
  REQUIRE(bytes.allocate(4096, budget).ok());
  poisoned = 0;
  for (const u8 b : bytes) poisoned += b == 0xA5 ? 1 : 0;
  CHECK_EQ(poisoned, 4096u);

  CHECK_EQ(budget.used(), 4096u + 4096u);
}

MHGP11_TEST_MAIN()
