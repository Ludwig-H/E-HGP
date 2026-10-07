// Porte de core sous MHGP12_POISON : tout octet alloue par Buffer vaut 0xA5 tant qu'il n'est pas ecrit. La porte
// n'est enregistree que dans une construction MHGP12_POISON (tests.cmake) : sans empoisonnement, lire ces octets
// serait lire une memoire non initialisee.
#include "core/core.hpp"
#include "test.hpp"

using namespace mhgp12;

MHGP12_TEST(poison, 11) {
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

  // sous cache de blocs (CST-0007) : le bloc repris du cache est de nouveau empoisonne sur toute sa taille
  MemoryBudget cached(MemoryBudget::kUnlimited, u64{1} << 23);
  Buffer<u8> large;
  REQUIRE(large.allocate(300 * 1024, cached).ok());
  const u8* first = large.data();
  for (u8& b : large) b = 0x11;
  large.reset();
  REQUIRE(large.allocate(300 * 1024, cached).ok());
  CHECK(large.data() == first);  // repris du cache, pas de l'allocateur
  poisoned = 0;
  for (const u8 b : large) poisoned += b == 0xA5 ? 1 : 0;
  CHECK_EQ(poisoned, 300u * 1024u);
}

MHGP12_TEST_MAIN()
