// Portes de panne de la Session et du calcul (hors produit ; mhgp11_api_session_fault_*). Les operateurs new et
// pthread_create sont remplaces dans ce seul executable (comme tests/sched/fault.cpp et tests/tower/forest_fault.cpp) :
//   session    : la creation refusee d'un fil du Pool rend session_overhead ; chaque allocation de Session::make
//                refusee tour a tour rend session_overhead (Pool), puis memory_budget (compte du budget), dans cet
//                ordre et jamais l'inverse ; chaque fil cree est joint ;
//   starvation : chaque allocation de compute refusee tour a tour, a W1 (ordre des allocations deterministe), rend
//                memory_budget sans reservation laissee dans le budget de la Session ; la tour calculee ensuite est
//                identique a celle d'avant les pannes, gardee vivante jusque-la ;
//   publication : chaque allocation de publish (le manifeste) refusee tour a tour rend memory_budget, etat none, ni D
//                ni D.pending ; la publication suivante, sans panne, est complete.
#include <dlfcn.h>
#include <pthread.h>

#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <cstdio>
#include <new>
#include <string>
#include <vector>

#include "api_support.hpp"
#include "test.hpp"

namespace {

std::atomic<long long> allocation_left{-1}, thread_left{-1};
std::atomic<unsigned long long> allocations{0}, created{0}, joined{0};

bool refuse_allocation() noexcept {
  allocations.fetch_add(1);
  return allocation_left.load() >= 0 && allocation_left.fetch_sub(1) == 0;
}

template <class Function>
Function next_symbol(const char* name) noexcept {
  void* address = dlsym(RTLD_NEXT, name);
  Function function = nullptr;
  static_assert(sizeof(function) == sizeof(address));
  std::memcpy(&function, &address, sizeof(function));
  return function;
}

}  // namespace

[[gnu::noinline]] void* operator new(std::size_t size) {
  if (refuse_allocation()) throw std::bad_alloc();
  if (void* block = std::malloc(size == 0 ? 1 : size)) return block;
  throw std::bad_alloc();
}
[[gnu::noinline]] void* operator new[](std::size_t size) { return ::operator new(size); }
[[gnu::noinline]] void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
  if (refuse_allocation()) return nullptr;
  return std::malloc(size == 0 ? 1 : size);
}
[[gnu::noinline]] void* operator new[](std::size_t size, const std::nothrow_t& tag) noexcept {
  return ::operator new(size, tag);
}
[[gnu::noinline]] void operator delete(void* block) noexcept { std::free(block); }
[[gnu::noinline]] void operator delete(void* block, std::size_t) noexcept { std::free(block); }
[[gnu::noinline]] void operator delete(void* block, const std::nothrow_t&) noexcept { std::free(block); }
[[gnu::noinline]] void operator delete[](void* block) noexcept { std::free(block); }
[[gnu::noinline]] void operator delete[](void* block, std::size_t) noexcept { std::free(block); }
[[gnu::noinline]] void operator delete[](void* block, const std::nothrow_t&) noexcept { std::free(block); }

extern "C" int pthread_create(pthread_t* thread, const pthread_attr_t* attr, void* (*start)(void*),
                              void* argument) noexcept {
  using Function = int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
  static const Function real = next_symbol<Function>("pthread_create");
  if (thread_left.load() >= 0 && thread_left.fetch_sub(1) == 0) return EAGAIN;
  if (real == nullptr) return ENOSYS;
  const int result = real(thread, attr, start, argument);
  if (result == 0) created.fetch_add(1);
  return result;
}

extern "C" int pthread_join(pthread_t thread, void** result) {
  using Function = int (*)(pthread_t, void**);
  static const Function real = next_symbol<Function>("pthread_join");
  if (real == nullptr) return ENOSYS;
  const int outcome = real(thread, result);
  if (outcome == 0) joined.fetch_add(1);
  return outcome;
}

using namespace mhgp11;
using namespace mhgp11::api_test;

MHGP11_TEST(session, 19) {
  {
    auto warm = api::Session::make({MemoryBudget::kUnlimited, 3});  // bibliotheques initialisees hors zone armee
    REQUIRE(warm.ok());
  }
  for (long long position = 0; position < 2; ++position) {
    const unsigned long long made = created.load(), closed = joined.load();
    thread_left.store(position);
    auto refused = api::Session::make({MemoryBudget::kUnlimited, 3});
    thread_left.store(-1);
    CHECK(!refused.ok() && refused.outcome().reason == Reason::session_overhead);
    CHECK_EQ(created.load() - made, joined.load() - closed);
  }
  const unsigned long long before = allocations.load();
  auto measured = api::Session::make({MemoryBudget::kUnlimited, 2});
  const unsigned long long count = allocations.load() - before;
  REQUIRE(measured.ok());
  CHECK(count >= 3);
  bool budget_seen = false;
  unsigned overhead = 0;
  for (unsigned long long position = 0; position < count; ++position) {
    allocation_left.store(static_cast<long long>(position));
    auto refused = api::Session::make({MemoryBudget::kUnlimited, 2});
    allocation_left.store(-1);
    REQUIRE(!refused.ok());
    const Reason reason = refused.outcome().reason;
    CHECK(reason == Reason::session_overhead || reason == Reason::memory_budget);
    CHECK(!(budget_seen && reason == Reason::session_overhead));  // jamais le Pool apres le budget
    budget_seen = budget_seen || reason == Reason::memory_budget;
    overhead += reason == Reason::session_overhead ? 1u : 0u;
  }
  CHECK(budget_seen);
  CHECK(overhead >= 2);
  CHECK_EQ(created.load(), joined.load() + 1);  // seul le fil de `measured` vit encore
}

MHGP11_TEST(starvation, 60) {
  auto made = api::Session::make({MemoryBudget::kUnlimited, 1});
  REQUIRE(made.ok());
  api::Session& session = made.value();
  const Points points = points_of({{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}, {5, 1, 0}});
  // Tour d'avant les pannes, gardee vivante jusqu'a la comparaison finale ; ses octets restent reserves.
  const unsigned long long before = allocations.load();
  auto kept = api::compute(session, points.view(), api::FullRequest{3});
  const unsigned long long count = allocations.load() - before;
  REQUIRE(kept.ok());
  const u64 held = session.budget().used();
  CHECK(count >= 20);
  REQUIRE(count < 100000);
  unsigned long long refusals = 0;
  for (unsigned long long position = 0; position < count; ++position) {
    Outcome outcome;
    {
      allocation_left.store(static_cast<long long>(position));
      auto attempt = api::compute(session, points.view(), api::FullRequest{3});
      allocation_left.store(-1);
      outcome = attempt.ok() ? Outcome{} : attempt.outcome();
    }
    CHECK_EQ(session.budget().used(), held);  // refus ou produit rendu : rien de plus ne reste reserve
    if (outcome.ok()) continue;
    ++refusals;
    CHECK_EQ(outcome.reason, Reason::memory_budget);
  }
  std::printf("starvation allocations=%llu refus=%llu\n", count, refusals);
  CHECK_EQ(refusals, count);  // aucune allocation du calcul n'est facultative : chacune refusee rend un refus
  auto after = api::compute(session, points.view(), api::FullRequest{3});
  REQUIRE(after.ok());
  CHECK(same_tower(after.value().full(), kept.value().full()));  // tour d'apres les pannes = tour d'avant
}

MHGP11_TEST(publication, 14) {
  auto made = api::Session::make({MemoryBudget::kUnlimited, 1});
  REQUIRE(made.ok());
  api::Session& session = made.value();
  Scratch s;
  REQUIRE(s.ok());
  const Points points = points_of({{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}, {5, 1, 0}});
  auto product = api::compute(session, points.view(), api::FullRequest{3});
  REQUIRE(product.ok());
  unsigned long long count = 0;
  {
    auto planned = io::OutputDirectory::plan(s.path("M").c_str(), {});
    REQUIRE(planned.ok());
    const unsigned long long before = allocations.load();
    const api::Publication published = api::publish(session, product.value(), planned.value(), provenance_of(points));
    count = allocations.load() - before;
    REQUIRE(published.ok());
  }
  CHECK(count >= 1);
  REQUIRE(count < 1000);
  for (unsigned long long position = 0; position < count; ++position) {
    const std::string d = s.path("P" + std::to_string(position));
    {
      auto planned = io::OutputDirectory::plan(d.c_str(), {});
      REQUIRE(planned.ok());
      allocation_left.store(static_cast<long long>(position));
      const api::Publication refused = api::publish(session, product.value(), planned.value(), provenance_of(points));
      allocation_left.store(-1);
      CHECK_EQ(refused.outcome.reason, Reason::memory_budget);
      CHECK(refused.state == api::PublicationState::none && refused.manifest_sha256 == io::Digest{});
    }
    CHECK(!exists(d) && !exists(d + ".pending"));
  }
  std::printf("publication allocations=%llu\n", count);
  {
    auto planned = io::OutputDirectory::plan(s.path("Z").c_str(), {});
    REQUIRE(planned.ok());
    CHECK(api::publish(session, product.value(), planned.value(), provenance_of(points)).ok());
  }
  CHECK(entries(s.path("Z")) == std::vector<std::string>({"full.mhgp11ful1", "manifeste.json"}));
}

MHGP11_TEST_MAIN()
