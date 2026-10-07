// Injection hors produit : allocations et creation de chaque thread. Toutes les formes new/delete utilisees
// sont appariees ; chaque fil cree est joint avant le retour de la factory ou la destruction du Pool.
#include <dlfcn.h>
#include <pthread.h>

#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>

#include "sched/sched.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::sched;

namespace {

std::atomic<long long> allocation_left{-1}, thread_left{-1};
std::atomic<bool> counting{false};
std::atomic<u64> allocations{0}, injections{0}, created{0}, joined{0}, thread_calls{0};

bool refuse_allocation() noexcept {
  if (counting.load()) allocations.fetch_add(1);
  if (allocation_left.load() >= 0 && allocation_left.fetch_sub(1) == 0) {
    injections.fetch_add(1);
    return true;
  }
  return false;
}

template <class Function>
Function next_symbol(const char* name) noexcept {
  void* address = dlsym(RTLD_NEXT, name);
  Function function = nullptr;
  static_assert(sizeof(function) == sizeof(address));
  std::memcpy(&function, &address, sizeof(function));
  return function;
}

Outcome count(void* pointer, u64 begin, u64 end, u32) {
  static_cast<std::atomic<u64>*>(pointer)->fetch_add(end - begin);
  return {};
}

}  // namespace

[[gnu::noinline]] void* operator new(std::size_t size) {
  if (refuse_allocation()) throw std::bad_alloc();
  if (void* block = std::malloc(size == 0 ? 1 : size)) return block;
  throw std::bad_alloc();
}
[[gnu::noinline]] void* operator new[](std::size_t size) { return ::operator new(size); }
[[gnu::noinline]] void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
  try {
    return ::operator new(size);
  } catch (const std::bad_alloc&) {
    return nullptr;
  }
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

extern "C" int pthread_create(pthread_t* thread, const pthread_attr_t* attr,
                              void* (*start)(void*), void* argument) noexcept {
  using Function = int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
  static const Function real = next_symbol<Function>("pthread_create");
  thread_calls.fetch_add(1);
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

MHGP12_TEST(thread_creation, 23) {
  // Temoin sans panne : l'interposition doit voir les vrais appels, y compris sous instrumentation.
  const u64 initial_created = created.load(), initial_joined = joined.load();
  {
    auto pool = make_pool({4});
    REQUIRE(pool.ok());
    std::atomic<u64> visits{0};
    CHECK(pool.value()->parallel_for(31, 3, &visits, count).ok());
    CHECK_EQ(visits.load(), 31u);
  }
  CHECK_EQ(created.load() - initial_created, 3u);
  CHECK_EQ(joined.load() - initial_joined, 3u);
  for (long long position = 0; position < 3; ++position) {
    const u64 calls_before = thread_calls.load(), made = created.load(), closed = joined.load();
    thread_left.store(position);
    auto refused = make_pool({4});
    thread_left.store(-1);
    CHECK(!refused.ok());
    CHECK_EQ(refused.outcome().reason, Reason::session_overhead);
    CHECK_EQ(refused.outcome().status(), Status::resource_exhausted);
    CHECK_EQ(thread_calls.load() - calls_before, static_cast<u64>(position) + 1);
    CHECK_EQ(created.load() - made, static_cast<u64>(position));
    CHECK_EQ(joined.load() - closed, static_cast<u64>(position));
  }
}

MHGP12_TEST(allocation, 30) {
  // Initialiser les bibliotheques avant de mesurer la factory ; aucune allocation de test dans la zone armee.
  { auto warm = make_pool({2}); REQUIRE(warm.ok()); }
  allocations.store(0);
  counting.store(true);
  auto measured = make_pool({4});
  counting.store(false);
  const u64 number = allocations.load();
  REQUIRE(measured.ok());
  measured.value().reset();
  CHECK(number >= 5);  // Pool, tableau, un etat pour chacun des trois threads
  REQUIRE(number < 64);
  for (u64 position = 0; position < number; ++position) {
    const u64 made = created.load(), closed = joined.load(), previous = injections.load();
    allocation_left.store(static_cast<long long>(position));
    auto refused = make_pool({4});
    allocation_left.store(-1);
    CHECK(!refused.ok());
    CHECK_EQ(refused.outcome().reason, Reason::session_overhead);
    CHECK_EQ(refused.outcome().status(), Status::resource_exhausted);
    CHECK_EQ(injections.load() - previous, 1u);
    CHECK_EQ(created.load() - made, joined.load() - closed);
  }
  // La borne haute est admise par le domaine, mais l'allocation est refusee avant creation de 255 threads.
  allocation_left.store(0);
  auto maximum = make_pool({kMaxWorkers});
  allocation_left.store(-1);
  CHECK(!maximum.ok());
  CHECK_EQ(maximum.outcome().reason, Reason::session_overhead);
  auto reusable = make_pool({2});
  REQUIRE(reusable.ok());
  std::atomic<u64> visits{0};
  CHECK(reusable.value()->parallel_for(23, 7, &visits, count).ok());
  CHECK_EQ(visits.load(), 23u);
  reusable.value().reset();
  CHECK_EQ(created.load(), joined.load());
}

MHGP12_TEST(allocation_free, 12) {
  for (u32 workers : {1u, 4u}) {
    auto pool = make_pool({workers});
    REQUIRE(pool.ok());
    std::atomic<u64> visits{0};
    CHECK(pool.value()->parallel_for(13, 1, &visits, count).ok());
    visits.store(0);
    allocations.store(0);
    const u64 previous = injections.load();
    counting.store(true);
    allocation_left.store(0);
    const auto result = pool.value()->parallel_for(1003, 7, &visits, count);
    allocation_left.store(-1);
    counting.store(false);
    CHECK(result.ok());
    CHECK_EQ(visits.load(), 1003u);
    CHECK_EQ(allocations.load(), 0u);
    CHECK_EQ(injections.load(), previous);
  }
}

MHGP12_TEST_MAIN()
