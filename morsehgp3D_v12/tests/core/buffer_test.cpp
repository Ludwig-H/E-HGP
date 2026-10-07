// Portes de core : budget memoire honnete (seul, puis sous concurrence), Buffer et ses refus, Csr bien forme.
#include <array>
#include <atomic>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "core/core.hpp"
#include "test.hpp"

using namespace mhgp12;

namespace {

// CSR de decalages et de taille de valeurs donnes ; les valeurs sont 100, 101, ...
Result<Csr<u32>> make_csr(const std::vector<u64>& off, u64 values, MemoryBudget& budget) {
  Csr<u32> c;
  MHGP12_TRY(c.off.allocate(off.size(), budget));
  MHGP12_TRY(c.val.allocate(values, budget));
  for (u64 i = 0; i < off.size(); ++i) c.off[i] = off[i];
  for (u64 i = 0; i < values; ++i) c.val[i] = static_cast<u32>(100 + i);
  return c;
}

bool well_formed(const std::vector<u64>& off, u64 values, MemoryBudget& budget) {
  const Result<Csr<u32>> c = make_csr(off, values, budget);
  return c.ok() && c.value().well_formed();
}

}  // namespace

MHGP12_TEST(budget, 57) {
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
MHGP12_TEST(budget_threads, 14) {
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

// Reservation sans bloc (memoire du GPU, R7) : comptee avec les Buffer du meme budget, refusee comme eux, rendue
// par reset ou a la destruction.
MHGP12_TEST(reservation, 21) {
  MemoryBudget budget(1000);
  Buffer<u64> host;
  REQUIRE(host.allocate(50, budget).ok());  // 400 octets d'hote
  {
    BudgetReservation device;
    CHECK_EQ(device.bytes(), 0u);
    CHECK(device.reserve(0, budget).ok());  // zero octet : rien de reserve
    CHECK_EQ(budget.used(), 400u);
    CHECK(device.reserve(600, budget).ok());  // coexistence : 400 + 600 = 1000, la limite exacte est admise
    CHECK_EQ(device.bytes(), 600u);
    CHECK_EQ(budget.used(), 1000u);
    CHECK_EQ(budget.peak(), 1000u);
    BudgetReservation more;
    const Outcome refused = more.reserve(1, budget);
    CHECK_EQ(refused.reason, Reason::memory_budget);
    CHECK_EQ(more.bytes(), 0u);
    CHECK_EQ(budget.used(), 1000u);  // un refus ne reserve rien
    Buffer<u8> blocked;
    CHECK_EQ(blocked.allocate(1, budget).reason, Reason::memory_budget);  // la reservation borne aussi les Buffer
    CHECK(device.reserve(100, budget).ok());  // nouvelle reservation : l'ancienne est rendue d'abord
    CHECK_EQ(budget.used(), 500u);
    CHECK_EQ(more.reserve(~u64{0}, budget).reason, Reason::memory_budget);  // aucune somme ne deborde
    device.reset();
    device.reset();  // idempotent
    CHECK_EQ(budget.used(), 400u);
    CHECK(device.reserve(500, budget).ok());
    CHECK_EQ(budget.used(), 900u);
  }
  CHECK_EQ(budget.used(), 400u);  // rendue a la destruction
  host.reset();
  CHECK(budget.released().ok());
  CHECK_EQ(budget.peak(), 1000u);
}

MHGP12_TEST(buffer, 1035) {
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

MHGP12_TEST(csr, 25) {
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

// Cache de blocs (MemoryBudget(limit, cache_bytes), 7 octobre 2026) : meme bloc repris dans la meme classe de taille,
// bloc neuf dans une autre, deux blocs vivants jamais confondus, blocs inactifs sous la capacite et au plus 32 par
// classe, petites tailles hors cache, compte du budget identique avec et sans cache, fils concurrents.
MHGP12_TEST(block_cache, 40) {
  constexpr u64 kKib = 1024;
  {
    MemoryBudget plain(MemoryBudget::kUnlimited);
    Buffer<u8> b;
    REQUIRE(b.allocate(300 * kKib, plain).ok());
    b.reset();
    CHECK_EQ(plain.cache_stats().capacity, 0u);
    CHECK_EQ(plain.cache_stats().misses, 0u);
  }
  MemoryBudget budget(MemoryBudget::kUnlimited, 8 * kKib * kKib);
  Buffer<u8> a;
  REQUIRE(a.allocate(300 * kKib, budget).ok());
  const u8* first = a.data();
  for (u64 i = 0; i < a.size(); i += 4096) a[i] = 7;
  a.reset();
  CHECK_EQ(budget.used(), 0u);
  CHECK_EQ(budget.cache_stats().misses, 1u);
  CHECK(budget.cache_stats().idle >= 300 * kKib);
  REQUIRE(a.allocate(300 * kKib, budget).ok());  // meme taille : meme bloc
  CHECK(a.data() == first);
  CHECK_EQ(budget.cache_stats().hits, 1u);
  CHECK_EQ(budget.cache_stats().idle, 0u);
  CHECK_EQ(budget.used(), 315392u);  // taille physique : classe de 300 Kio, 77 pages de 4 Kio (CST-0007)
  CHECK_EQ(budget.cache_stats().held, 315392u);
  a.reset();
  REQUIRE(a.allocate(300 * kKib - 1000, budget).ok());  // meme classe : meme bloc
  CHECK(a.data() == first);
  Buffer<u8> b;
  REQUIRE(b.allocate(300 * kKib - 1000, budget).ok());  // meme classe, cache vide : bloc neuf, distinct du vivant
  CHECK(b.data() != a.data());
  a.reset(); b.reset();
  Buffer<u8> c;
  REQUIRE(c.allocate(330 * kKib, budget).ok());  // classe superieure : ni l'un ni l'autre
  CHECK(c.data() != first);
  c.reset();
  {
    Buffer<u8> small;  // sous 256 Kio : hors cache
    const auto before = budget.cache_stats();
    REQUIRE(small.allocate(100 * kKib, budget).ok());
    small.reset();
    CHECK_EQ(budget.cache_stats().hits + budget.cache_stats().misses, before.hits + before.misses);
  }
  CHECK(budget.cache_stats().idle <= budget.cache_stats().capacity);
  // Capacite : 1 Mio de blocs inactifs au plus ; les autres sont rendus au systeme.
  MemoryBudget tight(MemoryBudget::kUnlimited, kKib * kKib);
  {
    std::array<Buffer<u8>, 4> held;
    for (auto& h : held) REQUIRE(h.allocate(400 * kKib, tight).ok());
  }
  CHECK(tight.cache_stats().idle <= kKib * kKib);
  CHECK(tight.cache_stats().rejected >= 2);
  // Au plus 32 blocs inactifs par classe.
  MemoryBudget roomy(MemoryBudget::kUnlimited, u64{1} << 34);
  {
    std::vector<Buffer<u8>> held(33);
    for (auto& h : held) REQUIRE(h.allocate(260 * kKib, roomy).ok());
  }
  CHECK_EQ(roomy.cache_stats().rejected, 1u);
  // Sous cache, le compte porte la taille physique des blocs (CST-0007) : au moins le compte sans cache, au plus ses
  // 9/8, et held = vivants + inactifs a chaque pas.
  MemoryBudget with(MemoryBudget::kUnlimited, u64{1} << 30), without(MemoryBudget::kUnlimited);
  bool bounded = true;
  {
    std::vector<Buffer<u8>> x(6), y(6);
    for (int step = 0; step < 24; ++step) {
      const u64 i = static_cast<u64>(step % 6), size = (128 + 97 * static_cast<u64>(step)) * kKib;
      if (step % 4 == 3) { x[i].reset(); y[i].reset(); continue; }
      bounded = bounded && x[i].allocate(size, with).ok() && y[i].allocate(size, without).ok();
      bounded = bounded && with.used() >= without.used() && with.used() <= without.used() + without.used() / 8;
      bounded = bounded && with.cache_stats().held == with.used() + with.cache_stats().idle;
    }
  }
  CHECK(bounded);
  CHECK(with.used() == 0 && without.used() == 0 && with.peak() >= without.peak());
  CHECK_EQ(with.cache_stats().held, with.cache_stats().idle);
  // Fils concurrents : chaque bloc vivant porte la marque de son fil du premier au dernier octet.
  MemoryBudget shared(MemoryBudget::kUnlimited, u64{1} << 28);
  std::atomic<u64> bad{0}, cacheable{0};
  {
    std::vector<std::thread> threads;
    for (u32 t = 0; t < 8; ++t)
      threads.emplace_back([&shared, &bad, &cacheable, t] {
        u64 state = 0x9E3779B97F4A7C15ull * (t + 1);
        for (int it = 0; it < 150; ++it) {
          state = state * 6364136223846793005ull + 1442695040888963407ull;
          const u64 size = 200 * kKib + (state >> 33) % (1800 * kKib);
          Buffer<u8> mine;
          if (!mine.allocate(size, shared).ok()) { ++bad; continue; }
          cacheable += size >= 256 * kKib;
          mine[0] = static_cast<u8>(t); mine[size - 1] = static_cast<u8>(t);
          std::this_thread::yield();
          if (mine[0] != t || mine[size - 1] != t) ++bad;
        }
      });
    for (auto& th : threads) th.join();
  }
  const auto stats = shared.cache_stats();
  CHECK_EQ(bad.load(), 0u);
  CHECK_EQ(stats.hits + stats.misses, cacheable.load());
  CHECK(stats.hits > 0 && stats.idle <= stats.capacity);
  CHECK_EQ(shared.used(), 0u);
  CHECK_EQ(stats.held, stats.idle);
}

// Cache compte comme une reserve sous une limite finie (CST-0007, CST-0019) : blocs vivants a leur taille physique,
// blocs inactifs sous la meme limite, restitution des inactifs avant un refus, repli a la taille exacte.
MHGP12_TEST(block_cache_limit, 40) {
  constexpr u64 kKib = 1024, kClass256 = 262144, kClass257 = 286720, kClass300 = 315392;  // capacites de classe
  {
    // Temoin de l'auditeur : limite 256 Kio + 1, bloc de 256 Kio + 1. La classe (280 Kio) ne tient pas : bloc exact,
    // jamais garde ; rien d'inactif ne reste hors de la limite.
    MemoryBudget budget(kClass256 + 1, kKib * kKib);
    Buffer<u8> b;
    REQUIRE(b.allocate(kClass256 + 1, budget).ok());
    CHECK_EQ(budget.used(), kClass256 + 1);
    CHECK_EQ(budget.cache_stats().exact, 1u);
    CHECK_EQ(budget.peak(), kClass256 + 1);
    b.reset();
    CHECK_EQ(budget.used(), 0u);
    CHECK_EQ(budget.cache_stats().idle, 0u);
    CHECK_EQ(budget.cache_stats().held, 0u);
    // admission sous cache : marge de 1/8 (la classe arrondit de moins de 10,7 %)
    CHECK_EQ(budget.admit(kClass256 + 1).reason, Reason::memory_budget);
    CHECK(budget.admit(229000).ok());
    CHECK(budget.peak() <= budget.limit());
  }
  {
    // Les blocs inactifs comptent sous la limite et sont rendus au systeme pour faire place.
    MemoryBudget budget(600000, kKib * kKib);
    Buffer<u8> a, b;
    REQUIRE(a.allocate(kClass256 + 1, budget).ok());
    CHECK_EQ(budget.used(), kClass257);
    a.reset();
    CHECK_EQ(budget.used(), 0u);
    CHECK_EQ(budget.cache_stats().idle, kClass257);
    CHECK_EQ(budget.cache_stats().held, kClass257);
    CHECK(budget.admit(500000).ok());  // l'inactif ne bloque pas l'admission : il sera rendu
    REQUIRE(b.allocate(300 * kKib, budget).ok());  // 280 Kio inactifs + 308 Kio > 600 000 : l'inactif est rendu
    CHECK_EQ(budget.cache_stats().evicted, 1u);
    CHECK_EQ(budget.cache_stats().idle, 0u);
    CHECK_EQ(budget.used(), kClass300);
    CHECK_EQ(budget.cache_stats().held, kClass300);
    // Classe de 280 Kio : 308 + 280 Kio > 600 000 meme cache vide ; la taille exacte (263 680) tient.
    Buffer<u8> c;
    REQUIRE(c.allocate(263680, budget).ok());
    CHECK_EQ(budget.cache_stats().exact, 1u);
    CHECK_EQ(budget.used(), kClass300 + 263680);
    CHECK_EQ(budget.peak(), kClass300 + 263680);
    Buffer<u8> d;
    CHECK_EQ(d.allocate(30000, budget).reason, Reason::memory_budget);  // 315 392 + 263 680 + 30 000 > 600 000
    CHECK(budget.cache_stats().held <= budget.limit() && budget.peak() <= budget.limit());
    c.reset();  // bloc exact : rendu au systeme, jamais garde
    CHECK_EQ(budget.cache_stats().idle, 0u);
    CHECK_EQ(budget.cache_stats().held, kClass300);
    b.reset();  // bloc de classe : garde
    CHECK_EQ(budget.cache_stats().idle, kClass300);
    // Une reservation sans bloc (tableaux de l'appareil) rend aussi les inactifs.
    BudgetReservation r;
    REQUIRE(r.reserve(400000, budget).ok());
    CHECK_EQ(budget.cache_stats().evicted, 2u);
    CHECK_EQ(budget.used(), 400000u);
    CHECK_EQ(budget.cache_stats().held, 400000u);
    r.reset();
    CHECK_EQ(budget.cache_stats().held, 0u);
    CHECK(budget.released().ok());
  }
  {
    // Fils concurrents sous une limite finie et un cache plus grand qu'elle : aucune reservation au-dela de la limite,
    // chaque bloc vivant garde la marque de son fil, comptes revenus a l'equilibre.
    MemoryBudget budget(12 * kKib * kKib, 64 * kKib * kKib);
    std::atomic<u64> bad{0}, refused{0}, over{0};
    {
      std::vector<std::thread> threads;
      for (u32 t = 0; t < 8; ++t)
        threads.emplace_back([&budget, &bad, &refused, &over, t] {
          u64 state = 0xD1B54A32D192ED03ull * (t + 1);
          for (int it = 0; it < 200; ++it) {
            state = state * 6364136223846793005ull + 1442695040888963407ull;
            const u64 size = 200 * kKib + (state >> 33) % (1800 * kKib);
            Buffer<u8> mine;
            if (!mine.allocate(size, budget).ok()) { ++refused; continue; }
            if (budget.cache_stats().held > budget.limit()) ++over;
            mine[0] = static_cast<u8>(t); mine[size - 1] = static_cast<u8>(t);
            std::this_thread::yield();
            if (mine[0] != t || mine[size - 1] != t) ++bad;
          }
        });
      for (auto& th : threads) th.join();
    }
    const auto stats = budget.cache_stats();
    CHECK_EQ(bad.load(), 0u);
    CHECK_EQ(over.load(), 0u);
    CHECK(budget.peak() <= budget.limit());
    CHECK_EQ(budget.used(), 0u);
    CHECK_EQ(stats.held, stats.idle);
    CHECK(stats.held <= budget.limit());
    CHECK(stats.hits > 0);
    CHECK(refused.load() < 8 * 200);
  }
}
