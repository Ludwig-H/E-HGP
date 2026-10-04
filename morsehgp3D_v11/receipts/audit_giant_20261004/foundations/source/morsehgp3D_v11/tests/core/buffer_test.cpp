// Portes de core : budget memoire honnete (seul, puis sous concurrence), Buffer et ses refus, Csr bien forme.
#include <array>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "core/core.hpp"
#include "test.hpp"

using namespace mhgp11;

namespace {

// CSR de decalages et de taille de valeurs donnes ; les valeurs sont 100, 101, ...
Result<Csr<u32>> make_csr(const std::vector<u64>& off, u64 values, MemoryBudget& budget) {
  Csr<u32> c;
  MHGP11_TRY(c.off.allocate(off.size(), budget));
  MHGP11_TRY(c.val.allocate(values, budget));
  for (u64 i = 0; i < off.size(); ++i) c.off[i] = off[i];
  for (u64 i = 0; i < values; ++i) c.val[i] = static_cast<u32>(100 + i);
  return c;
}

bool well_formed(const std::vector<u64>& off, u64 values, MemoryBudget& budget) {
  const Result<Csr<u32>> c = make_csr(off, values, budget);
  return c.ok() && c.value().well_formed();
}

}  // namespace

MHGP11_TEST(budget, 57) {
  MemoryBudget budget(1000);
  CHECK_EQ(budget.limit(), 1000u);
  CHECK_EQ(budget.used(), 0u);
  CHECK_EQ(budget.peak(), 0u);

  Buffer<u64> a;
  CHECK(a.allocate(100, budget).ok());  // 800 octets
  CHECK_EQ(budget.used(), 800u);
  Buffer<u64> b;
  const Outcome refused = b.allocate(100, budget);  // 800 + 800 > 1000
  CHECK_EQ(refused.reason, Reason::memory_budget);
  CHECK_EQ(refused.status(), Status::resource_exhausted);
  CHECK(b.empty() && b.data() == nullptr);
  CHECK_EQ(budget.used(), 800u);  // un refus ne reserve rien
  CHECK_EQ(budget.peak(), 800u);

  // admission d'un etage par formule : juge ce qui tiendrait encore, sans rien reserver
  CHECK(budget.admit(200).ok());   // 800 + 200 = 1000 : la limite exacte est admise
  CHECK_EQ(budget.admit(201).reason, Reason::memory_budget);
  CHECK_EQ(budget.admit(201).status(), Status::resource_exhausted);
  CHECK(budget.admit(0).ok());
  CHECK_EQ(budget.admit(~u64{0}).reason, Reason::memory_budget);  // aucune somme ne deborde
  CHECK_EQ(budget.used(), 800u);
  CHECK_EQ(budget.peak(), 800u);

  CHECK(b.allocate(25, budget).ok());  // 800 + 200 = 1000 : la limite exacte est admise
  CHECK_EQ(budget.used(), 1000u);
  Buffer<u8> one;
  CHECK_EQ(one.allocate(1, budget).reason, Reason::memory_budget);  // un octet de trop
  CHECK_EQ(budget.admit(1).reason, Reason::memory_budget);          // budget plein : plus rien n'est admis
  CHECK(budget.admit(0).ok());
  CHECK_EQ(budget.used(), 1000u);
  CHECK_EQ(budget.peak(), 1000u);

  a.reset();
  CHECK_EQ(budget.used(), 200u);  // liberation en octets, pas en elements
  CHECK(one.allocate(1, budget).ok());
  CHECK_EQ(budget.used(), 201u);
  Buffer<u64> two;
  CHECK(two.allocate(2, budget).ok());
  b = std::move(two);  // l'affectation par deplacement rend les 200 octets de b et garde les 16 de two
  CHECK_EQ(budget.used(), 17u);
  CHECK_EQ(b.size(), 2u);
  b.reset();
  one.reset();
  CHECK_EQ(budget.used(), 0u);
  CHECK_EQ(budget.peak(), 1000u);  // le pic ne redescend pas

  // une demande plus grande que la limite est refusee meme sur un budget vide
  Buffer<u64> big;
  CHECK_EQ(big.allocate(126, budget).reason, Reason::memory_budget);
  CHECK_EQ(budget.used(), 0u);
  // reallocation : le contenu precedent est libere avant la nouvelle reservation
  CHECK(big.allocate(100, budget).ok());
  CHECK(big.allocate(110, budget).ok());
  CHECK_EQ(budget.used(), 880u);
  CHECK_EQ(big.allocate(200, budget).reason, Reason::memory_budget);
  CHECK(big.empty());  // apres un refus le tampon est vide
  CHECK_EQ(budget.used(), 0u);

  // pic par etage : restart_peak rend l'ancien pic et repart de ce qui est en usage
  CHECK(big.allocate(50, budget).ok());  // 400 octets en usage
  CHECK_EQ(budget.restart_peak(), 1000u);
  CHECK_EQ(budget.peak(), 400u);
  Buffer<u8> stage;
  CHECK(stage.allocate(100, budget).ok());
  stage.reset();
  CHECK_EQ(budget.peak(), 500u);  // pic de l'etage, pas celui d'avant
  CHECK_EQ(budget.restart_peak(), 500u);
  CHECK_EQ(budget.peak(), 400u);

  // fin de vie : un budget qui garde des octets reserves est une violation d'invariant
  CHECK_EQ(budget.released().reason, Reason::budget_not_released);
  CHECK_EQ(budget.released().status(), Status::invariant_violated);
  CHECK_EQ(exit_code(budget.released()), 3);
  big.reset();
  CHECK(budget.released().ok());

  // budget sans limite
  MemoryBudget unlimited(MemoryBudget::kUnlimited);
  CHECK_EQ(unlimited.limit(), ~u64{0});
  Buffer<u32> c;
  CHECK(c.allocate(1000, unlimited).ok());
  CHECK_EQ(unlimited.used(), 4000u);

  // un resultat qui survit a sa Session viole le contrat de duree de vie : released() le dit avant la destruction,
  // et le compte partage fait que la suite reste definie (le tampon rend ses octets a un compte encore vivant)
  Buffer<u32> survivor;
  {
    auto session = std::make_unique<MemoryBudget>(4096);
    CHECK(survivor.allocate(16, *session).ok());
    CHECK_EQ(session->released().reason, Reason::budget_not_released);
  }
  survivor[15] = 7;
  CHECK_EQ(survivor[15], 7u);
  survivor.reset();
}

// Budget sous concurrence. Trois phases, chacune deterministe quel que soit l'entrelacement des fils :
//   A. kThreads fils reservent chacun kBlock octets, la limite valant kThreads * kBlock : tous reussissent ;
//   B. le budget est plein : chaque fil qui demande un octet de plus est refuse ;
//   C. apres liberation, chaque fil alloue et rend kBlock octets kRounds fois : la somme vivante ne depasse jamais la
//      limite, donc aucune demande n'est refusee.
// A la fin : rien en usage, pic exactement egal a la limite. Les fils rangent leurs resultats dans leur case.
// La phase A est le contrat de admit : une fois la somme de l'etage admise par son pilote, aucune allocation de
// l'etage n'est refusee, quel que soit l'entrelacement.
MHGP11_TEST(budget_threads, 14) {
  constexpr unsigned kThreads = 8;
  constexpr u64 kBlock = 4096;
  constexpr unsigned kRounds = 5000;
  MemoryBudget budget(kThreads * kBlock);
  CHECK(budget.admit(kThreads * kBlock).ok());
  CHECK_EQ(budget.admit(kThreads * kBlock + 1).reason, Reason::memory_budget);
  std::array<Buffer<u8>, kThreads> held;
  std::array<unsigned, kThreads> ok{}, refused{}, round_ok{};

  std::vector<std::thread> threads;
  for (unsigned t = 0; t < kThreads; ++t)
    threads.emplace_back([&, t] { ok[t] = held[t].allocate(kBlock, budget).ok() ? 1 : 0; });
  for (std::thread& th : threads) th.join();
  threads.clear();
  unsigned sum_ok = 0;
  for (unsigned v : ok) sum_ok += v;
  CHECK_EQ(sum_ok, kThreads);
  CHECK_EQ(budget.used(), kThreads * kBlock);
  CHECK_EQ(budget.peak(), kThreads * kBlock);

  for (unsigned t = 0; t < kThreads; ++t)
    threads.emplace_back([&, t] {
      Buffer<u8> extra;
      refused[t] = extra.allocate(1, budget).reason == Reason::memory_budget ? 1 : 0;
    });
  for (std::thread& th : threads) th.join();
  threads.clear();
  unsigned sum_refused = 0;
  for (unsigned v : refused) sum_refused += v;
  CHECK_EQ(sum_refused, kThreads);
  CHECK_EQ(budget.used(), kThreads * kBlock);

  for (Buffer<u8>& h : held) h.reset();
  CHECK_EQ(budget.used(), 0u);
  for (unsigned t = 0; t < kThreads; ++t)
    threads.emplace_back([&, t] {
      unsigned good = 0;
      for (unsigned r = 0; r < kRounds; ++r) {
        Buffer<u8> block;
        if (block.allocate(kBlock, budget).ok()) {
          block[0] = static_cast<u8>(t);
          block[kBlock - 1] = static_cast<u8>(r);
          ++good;
        }
      }
      round_ok[t] = good;
    });
  for (std::thread& th : threads) th.join();
  u64 sum_rounds = 0;
  unsigned full_threads = 0;
  for (unsigned v : round_ok) {
    sum_rounds += v;
    full_threads += v == kRounds ? 1 : 0;
  }
  CHECK_EQ(sum_rounds, u64{kThreads} * kRounds);
  CHECK_EQ(full_threads, kThreads);
  CHECK_EQ(budget.used(), 0u);
  CHECK_EQ(budget.peak(), kThreads * kBlock);  // jamais au-dela de la limite, et la limite a ete atteinte
  CHECK(budget.peak() <= budget.limit());
  CHECK(budget.released().ok());
}

MHGP11_TEST(buffer, 1035) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  Buffer<u32> empty;
  CHECK(empty.empty());
  CHECK_EQ(empty.size(), 0u);
  CHECK(empty.data() == nullptr);
  CHECK(empty.begin() == empty.end());
  CHECK_EQ(empty.span().size(), 0u);
  CHECK(empty.allocate(0, budget).ok());  // zero element : aucun bloc, aucune reservation
  CHECK(empty.empty() && empty.data() == nullptr);
  CHECK_EQ(budget.used(), 0u);

  Buffer<u32> a;
  REQUIRE(a.allocate(1000, budget).ok());
  CHECK_EQ(a.size(), 1000u);
  CHECK(!a.empty());
  CHECK_EQ(budget.used(), 4000u);
  for (u64 i = 0; i < a.size(); ++i) a[i] = static_cast<u32>(3 * i + 1);
  const Buffer<u32>& view = a;
  for (u64 i = 0; i < view.size(); ++i) CHECK_EQ(view[i], 3 * i + 1);  // 1000 controles
  CHECK_EQ(static_cast<u64>(a.end() - a.begin()), 1000u);
  CHECK_EQ(a.span().size(), 1000u);
  CHECK(view.span().data() == a.data());
  u64 sum = 0;
  for (const u32 v : view) sum += v;
  CHECK_EQ(sum, 3u * (999u * 1000u / 2u) + 1000u);

  // deplacement : la source est vide, la reservation suit le contenu
  Buffer<u32> b(std::move(a));
  CHECK(a.empty() && a.data() == nullptr);  // etat apres deplacement, garanti par Buffer
  CHECK_EQ(b.size(), 1000u);
  CHECK_EQ(b[999], 2998u);
  CHECK_EQ(budget.used(), 4000u);
  Buffer<u32> c;
  c.swap(b);
  CHECK(b.empty());
  CHECK_EQ(c.size(), 1000u);
  c.reset();
  c.reset();  // idempotent
  CHECK(c.empty());
  CHECK_EQ(budget.used(), 0u);

  // garde de taille : n * sizeof(T) doit tenir dans u64. 2^61 elements de 8 octets font 2^64 octets, soit 0 modulo
  // 2^64 : sans la garde, l'allocation de zero octet reussirait et le tampon annoncerait 2^61 elements.
  CHECK_EQ(Buffer<u64>::kMaxCount, (~u64{0}) / 8);
  CHECK_EQ(Buffer<u8>::kMaxCount, ~u64{0});
  Buffer<u64> huge;
  const Outcome wrapped = huge.allocate(u64{1} << 61, budget);
  CHECK_EQ(wrapped.reason, Reason::memory_budget);
  CHECK(huge.empty() && huge.data() == nullptr);
  CHECK_EQ(huge.allocate(Buffer<u64>::kMaxCount + 1, budget).reason, Reason::memory_budget);
  CHECK_EQ(huge.allocate((u64{1} << 61) + 5, budget).reason, Reason::memory_budget);
  CHECK(huge.empty());
  CHECK_EQ(budget.used(), 0u);
  // sous un budget fini, une taille enorme mais representable est refusee par le budget, avant toute allocation
  MemoryBudget small(1 << 20);
  CHECK_EQ(huge.allocate(Buffer<u64>::kMaxCount, small).reason, Reason::memory_budget);
  CHECK_EQ(small.used(), 0u);
  CHECK_EQ(small.peak(), 0u);
}

MHGP11_TEST(csr, 25) {
  MemoryBudget budget(1 << 20);
  const Csr<u32> none{};
  CHECK(none.well_formed());  // aucune ligne, aucune valeur
  CHECK_EQ(none.rows(), 0u);

  Result<Csr<u32>> made = make_csr({0, 1, 1, 4}, 4, budget);
  REQUIRE(made.ok());
  const Csr<u32>& c = made.value();
  CHECK(c.well_formed());
  CHECK_EQ(c.rows(), 3u);
  CHECK_EQ(c.row(0).size(), 1u);
  CHECK_EQ(c.row(0)[0], 100u);
  CHECK_EQ(c.row(1).size(), 0u);  // ligne vide admise
  CHECK_EQ(c.row(2).size(), 3u);
  CHECK_EQ(c.row(2)[0], 101u);
  CHECK_EQ(c.row(2)[2], 103u);

  CHECK(well_formed({0}, 0, budget));           // zero ligne, decalage initial seul
  CHECK(well_formed({0, 0, 0}, 0, budget));     // deux lignes vides
  CHECK(well_formed({0, 2}, 2, budget));
  CHECK(!well_formed({}, 3, budget));           // des valeurs sans decalages
  CHECK(!well_formed({0}, 1, budget));          // dernier decalage different de la taille des valeurs
  CHECK(!well_formed({1, 2}, 2, budget));       // premier decalage non nul
  CHECK(!well_formed({0, 3, 2, 4}, 4, budget)); // decalages decroissants
  CHECK(!well_formed({0, 2, 5}, 4, budget));    // dernier decalage au-dela des valeurs
  CHECK(!well_formed({0, 2, 3}, 4, budget));    // dernier decalage en deca des valeurs
  CHECK(!well_formed({0, 4, 3}, 3, budget));    // decroissant sur la derniere ligne, dernier decalage juste
  CHECK(!well_formed({0, 1}, 0, budget));       // decalage final non nul sans valeurs
  CHECK_EQ(budget.used(), 32u + 16u);           // seul `made` est vivant : 4 decalages u64 et 4 valeurs u32
  // un decalage au-dela de 2^32 n'est pas tronque : il est compare tel quel a la taille des valeurs
  CHECK(!well_formed({0, u64{1} << 32}, 0, budget));
  CHECK(!well_formed({0, (u64{1} << 32) + 2}, 2, budget));
}
