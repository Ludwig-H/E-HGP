// Portes du Pool synchrone : ordinaux, barrieres, proprietes et refus apres jointure.
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

#include "sched/sched.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::sched;

static_assert(!std::is_default_constructible_v<Pool> && !std::is_copy_constructible_v<Pool>);
static_assert(!std::is_move_constructible_v<Pool> && !std::is_move_assignable_v<Pool>);

namespace {

Outcome empty(void*, u64, u64, u32) { return {}; }

template <class Predicate>
bool await(Predicate predicate) {
  const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (!predicate()) {
    if (std::chrono::steady_clock::now() >= limit) return false;
    std::this_thread::yield();
  }
  return true;
}

struct Coverage {
  u64 n = 0, grain = 1;
  u32 workers = 1;
  std::array<std::atomic<u32>, 1003> hits{};
  std::atomic<u64> chunks{0};
  std::atomic<bool> valid{true};
};

Outcome cover(void* pointer, u64 begin, u64 end, u32 worker) {
  auto& c = *static_cast<Coverage*>(pointer);
  // Reference u128 : multiplication et addition larges, distinctes du CAS/intervalle saturant du produit.
  const u128 expected_end = std::min<u128>(u128(begin) + c.grain, c.n);
  if (begin >= end || end != expected_end || begin % c.grain != 0 || end > c.n || worker >= c.workers) {
    c.valid.store(false);
    return fail(Reason::task_exception);
  }
  for (u64 i = begin; i < end; ++i) c.hits[i].fetch_add(1);
  c.chunks.fetch_add(1);
  return {};
}

struct Ranges {
  std::atomic<u32> count{0};
  std::array<std::pair<u64, u64>, 8> ranges{};
};

Outcome ranges(void* pointer, u64 begin, u64 end, u32) {
  auto& r = *static_cast<Ranges*>(pointer);
  const u32 i = r.count.fetch_add(1);
  if (i >= r.ranges.size()) return fail(Reason::task_exception);
  r.ranges[i] = {begin, end};
  return {};
}

struct Nested {
  Pool* pool = nullptr;
  std::atomic<u32> checked{0}, bad{0}, inner{0};
};

Outcome forbidden(void* pointer, u64, u64, u32) {
  static_cast<Nested*>(pointer)->inner.fetch_add(1);
  return {};
}

Outcome nested(void* pointer, u64, u64, u32) {
  auto& n = *static_cast<Nested*>(pointer);
  for (u64 count : {u64{0}, u64{1}}) {
    const auto result = n.pool->parallel_for(count, 1, pointer, forbidden);
    n.bad.fetch_add(result.reason == Reason::pool_busy ? 0u : 1u);
    n.checked.fetch_add(1);
  }
  return {};
}

struct Blocking {
  std::atomic<bool> entered{false}, release{false}, timed_out{false};
  std::atomic<u32> finished{0};
};

Outcome blocking(void* pointer, u64, u64, u32) {
  auto& b = *static_cast<Blocking*>(pointer);
  b.entered.store(true);
  if (!await([&] { return b.release.load(); })) b.timed_out.store(true);
  b.finished.fetch_add(1);
  return {};
}

struct Reductions {
  std::atomic<u32> completed{0};
  u32 mode = 0;
};

Outcome reductions(void* pointer, u64 begin, u64, u32) {
  auto& r = *static_cast<Reductions*>(pointer);
  r.completed.fetch_add(1);
  if (r.mode == 1) throw std::bad_alloc();
  if (r.mode == 2) throw 17;
  if (begin % 7 == 0) return fail(Reason::session_overhead, 3);
  if (begin % 5 == 0) return fail(Reason::memory_budget, 2);
  if (begin % 3 == 0) return fail(Reason::empty_input, 2);
  return {};
}

struct Rendezvous {
  u32 workers = 1;
  std::atomic<u32> entered{0}, finished{0};
  std::array<std::atomic<u32>, 8> hits{};
  std::atomic<bool> timed_out{false};
};

Outcome rendezvous(void* pointer, u64, u64, u32 worker) {
  auto& r = *static_cast<Rendezvous*>(pointer);
  if (worker >= r.workers || worker >= r.hits.size()) return fail(Reason::task_exception);
  r.hits[worker].fetch_add(1);
  r.entered.fetch_add(1);
  if (!await([&] { return r.entered.load() == r.workers; })) r.timed_out.store(true);
  r.finished.fetch_add(1);
  return {};
}

}  // namespace

MHGP12_TEST(domain, 21) {
  CHECK_EQ(kMaxWorkers, 256u);
  for (u32 workers : {0u, 257u, std::numeric_limits<u32>::max()}) {
    auto refused = make_pool({workers});
    CHECK(!refused.ok());
    CHECK_EQ(refused.outcome().reason, Reason::parameter_out_of_range);
  }
  auto default_pool = make_pool();
  REQUIRE(default_pool.ok());
  CHECK_EQ(default_pool.value()->size(), 1u);
  CHECK_EQ(default_pool.value()->parallel_for(0, 0, nullptr, empty).reason, Reason::parameter_out_of_range);
  CHECK_EQ(default_pool.value()->parallel_for(1, 0, nullptr, empty).reason, Reason::parameter_out_of_range);
  CHECK_EQ(default_pool.value()->parallel_for(0, 1, nullptr, nullptr).reason, Reason::parameter_out_of_range);
  CHECK_EQ(default_pool.value()->parallel_for(1, 1, nullptr, nullptr).reason, Reason::parameter_out_of_range);
  CHECK(default_pool.value()->parallel_for(0, 1, nullptr, empty).ok());
  CHECK(default_pool.value()->parallel_for(7, 3, nullptr, empty).ok());
  for (u32 workers : {2u, 4u, 8u}) {
    auto pool = make_pool({workers});
    REQUIRE(pool.ok());
    CHECK_EQ(pool.value()->size(), workers);
  }
}

MHGP12_TEST(coverage, 68) {
  for (u32 workers : {1u, 2u, 4u, 8u}) {
    auto pool = make_pool({workers});
    REQUIRE(pool.ok());
    for (u64 grain : {u64{1}, u64{7}, u64{256}, u64{4096}}) {
      Coverage c;
      c.n = 1003;
      c.grain = grain;
      c.workers = workers;
      CHECK(pool.value()->parallel_for(c.n, grain, &c, cover).ok());
      CHECK(c.valid.load());
      CHECK_EQ(c.chunks.load(), (c.n + grain - 1) / grain);
      bool once = true;
      for (const auto& hit : c.hits) once = once && hit.load() == 1;
      CHECK(once);
    }
  }
}

MHGP12_TEST(limits, 98) {
  const u64 high = std::numeric_limits<u64>::max();
  for (u32 workers : {1u, 4u}) {
    auto pool = make_pool({workers});
    REQUIRE(pool.ok());
    for (u64 n : {u64{0}, u64{1}, high - 1, high})
      for (u64 grain : {high / 3, high / 2, high - 1, high}) {
        Ranges actual;
        CHECK(pool.value()->parallel_for(n, grain, &actual, ranges).ok());
        std::vector<std::pair<u64, u64>> expected;
        for (u128 begin = 0; begin < n; begin += grain)
          expected.emplace_back(static_cast<u64>(begin), static_cast<u64>(std::min<u128>(n, begin + grain)));
        const u32 count = actual.count.load();
        REQUIRE(count <= actual.ranges.size());
        std::sort(actual.ranges.begin(), actual.ranges.begin() + count);
        CHECK(std::equal(expected.begin(), expected.end(), actual.ranges.begin(), actual.ranges.begin() + count));
      }
  }
}

MHGP12_TEST(short_jobs, 12) {
  for (u32 workers : {1u, 2u, 8u}) {
    auto pool = make_pool({workers});
    REQUIRE(pool.ok());
    bool valid = true, once = true, complete = true;
    for (u32 ordinal = 0; ordinal < 1000; ++ordinal) {
      Coverage c;
      c.n = ordinal % 2 == 0 ? 1 : 67;
      c.workers = workers;
      complete = pool.value()->parallel_for(c.n, 1, &c, cover).ok() && complete;
      valid = valid && c.valid.load();
      for (u64 i = 0; i < c.hits.size(); ++i) once = once && c.hits[i].load() == (i < c.n ? 1u : 0u);
    }
    CHECK(valid);
    CHECK(once);
    CHECK(complete);
  }
}

MHGP12_TEST(reentrant, 12) {
  for (u32 workers : {1u, 4u}) {
    auto pool = make_pool({workers});
    REQUIRE(pool.ok());
    Nested n{pool.value().get()};
    CHECK(pool.value()->parallel_for(16, 1, &n, nested).ok());
    CHECK_EQ(n.checked.load(), 32u);
    CHECK_EQ(n.bad.load(), 0u);
    CHECK_EQ(n.inner.load(), 0u);
    CHECK(pool.value()->parallel_for(1, 1, nullptr, empty).ok());
  }
}

MHGP12_TEST(concurrent, 20) {
  for (u32 workers : {1u, 4u}) {
    auto pool = make_pool({workers});
    REQUIRE(pool.ok());
    Blocking b;
    Outcome first;
    std::thread caller([&] { first = pool.value()->parallel_for(1, 1, &b, blocking); });
    const bool entered = await([&] { return b.entered.load(); });
    CHECK(entered);
    const auto second = pool.value()->parallel_for(1, 1, nullptr, empty);
    const auto zero = pool.value()->parallel_for(0, 1, nullptr, empty);
    b.release.store(true);
    caller.join();
    CHECK_EQ(second.reason, Reason::pool_busy);
    CHECK_EQ(zero.reason, Reason::pool_busy);
    CHECK(first.ok());
    CHECK(!b.timed_out.load());
    CHECK_EQ(b.finished.load(), 1u);
    CHECK(pool.value()->parallel_for(5, 1, nullptr, empty).ok());
    CHECK_EQ(second.status(), Status::invalid_input);
    CHECK_EQ(pool.value()->size(), workers);
  }
}

MHGP12_TEST(outcomes, 16) {
  for (u32 workers : {1u, 2u, 4u, 8u}) {
    auto pool = make_pool({workers});
    REQUIRE(pool.ok());
    Reductions r;
    CHECK(pool.value()->parallel_for(113, 1, &r, reductions) == fail(Reason::empty_input, 2));
    CHECK_EQ(r.completed.load(), 113u);
    CHECK(pool.value()->parallel_for(1, 1, nullptr, empty).ok());
  }
}

MHGP12_TEST(exceptions, 26) {
  for (u32 workers : {1u, 4u}) {
    auto pool = make_pool({workers});
    REQUIRE(pool.ok());
    for (u32 mode : {1u, 2u}) {
      Reductions r;
      r.mode = mode;
      const auto result = pool.value()->parallel_for(67, 1, &r, reductions);
      CHECK_EQ(result.reason, mode == 1 ? Reason::memory_budget : Reason::task_exception);
      CHECK_EQ(result.status(), mode == 1 ? Status::resource_exhausted : Status::invariant_violated);
      CHECK_EQ(r.completed.load(), 67u);
      CHECK(pool.value()->parallel_for(1, 1, nullptr, empty).ok());
    }
    Rendezvous r;
    r.workers = workers;
    CHECK(pool.value()->parallel_for(workers, 1, &r, rendezvous).ok());
    CHECK(!r.timed_out.load());
    CHECK_EQ(r.finished.load(), workers);
    bool once = true;
    for (u32 worker = 0; worker < workers; ++worker) once = once && r.hits[worker].load() == 1;
    CHECK(once);
  }
}

MHGP12_TEST_MAIN()
