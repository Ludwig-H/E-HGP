#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <new>
#include <string>
#include <type_traits>
#include <unistd.h>

#include "core/ledger.hpp"
#include "core/status.hpp"

namespace {
std::size_t allocations = 0;
bool refuse_allocations = false;
[[noreturn]] void terminate_handler() noexcept {
  static constexpr char message[] = "unexpected_terminate\n";
  const auto ignored = ::write(STDERR_FILENO, message, sizeof(message) - 1);
  (void)ignored;
  std::_Exit(42);
}
void* allocate(std::size_t n) {
  if (refuse_allocations) throw std::bad_alloc{};
  if (void* p = std::malloc(n == 0 ? 1 : n)) {
    ++allocations;
    return p;
  }
  throw std::bad_alloc{};
}
}  // namespace

void* operator new(std::size_t n) { return allocate(n); }
void* operator new[](std::size_t n) { return allocate(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

int main() {
  using namespace mhgp11;
  static_assert(std::is_trivially_destructible_v<Stopwatch>);
  static_assert(std::is_nothrow_destructible_v<Stopwatch>);
  static_assert(std::chrono::steady_clock::is_steady);
  std::set_terminate(terminate_handler);

  Stopwatch readings;
  u64 last = readings.nanoseconds();
  bool monotone = true;
  for (unsigned i = 0; i < 10000; ++i) {
    const u64 next = readings.nanoseconds();
    monotone = monotone && next >= last;
    last = next;
  }

  Ledger ledger;
  std::string name = "abc";
  ledger.time(name, 7);
  std::size_t before = 0;
  {
    const Stopwatch watch;
    name[0] = 'd';  // backing storage and name stay alive
    (void)watch.nanoseconds();
    before = allocations;
    refuse_allocations = true;
  }  // no ledger or name dependency, even while allocations are refused
  refuse_allocations = false;
  const std::size_t mutable_destructor_allocations = allocations - before;
  const bool mutation_did_not_publish = ledger.nanoseconds("abc") == 7 && ledger.nanoseconds("dbc") == 0;

  ledger.time("abc", 1);
  {
    const Stopwatch watch;
    ledger = Ledger{};  // same living object, no surviving stage entry
    (void)watch.nanoseconds();
    before = allocations;
    refuse_allocations = true;
  }
  refuse_allocations = false;
  const std::size_t reset_destructor_allocations = allocations - before;
  const bool reset_did_not_publish = ledger.to_json() == "{\"counters\":{},\"nanoseconds\":{}}";

  ledger.time("existing", 7);
  const std::string unchanged = ledger.to_json();
  Stopwatch publication;
  const u64 measured = publication.nanoseconds();
  refuse_allocations = true;
  const Outcome refusal = guarded([&]() -> Outcome {
    ledger.time("new_stage", measured);
    return {};
  });
  refuse_allocations = false;
  const bool transaction_preserved = ledger.to_json() == unchanged;
  const bool memory_refusal = refusal.reason == Reason::memory_budget && refusal.status() == Status::resource_exhausted;

  refuse_allocations = true;
  const Outcome existing = guarded([&]() -> Outcome {
    ledger.time("existing", 1);
    return {};
  });
  refuse_allocations = false;
  const bool existing_no_allocation = existing.ok() && ledger.nanoseconds("existing") == 8;
  const Outcome retry = guarded([&]() -> Outcome {
    ledger.time("new_stage", measured);
    return {};
  });
  const bool retry_published = retry.ok() && ledger.nanoseconds("new_stage") == measured &&
                               ledger.to_json().find("\"new_stage\":") != std::string::npos;

  const bool ok = monotone && mutable_destructor_allocations == 0 && reset_destructor_allocations == 0 &&
                  mutation_did_not_publish && reset_did_not_publish && transaction_preserved && memory_refusal &&
                  existing_no_allocation && retry_published;
  std::printf("{\"status\":\"%s\",\"monotone_reads\":10001,\"monotone\":%s,"
              "\"trivially_destructible\":true,\"mutable_destructor_allocations\":%zu,"
              "\"reset_destructor_allocations\":%zu,\"mutation_did_not_publish\":%s,"
              "\"reset_did_not_publish\":%s,\"explicit_guarded_reason\":\"%.*s\","
              "\"transaction_preserved\":%s,\"existing_no_allocation\":%s,\"retry_published\":%s}\n",
              ok ? "PASS" : "FAIL", monotone ? "true" : "false", mutable_destructor_allocations,
              reset_destructor_allocations, mutation_did_not_publish ? "true" : "false",
              reset_did_not_publish ? "true" : "false", static_cast<int>(reason_name(refusal.reason).size()),
              reason_name(refusal.reason).data(), transaction_preserved ? "true" : "false",
              existing_no_allocation ? "true" : "false", retry_published ? "true" : "false");
  return ok ? 0 : 1;
}
