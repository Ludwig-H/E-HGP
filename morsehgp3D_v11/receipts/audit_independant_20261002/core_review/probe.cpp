#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <new>
#include <string>

#include "core/core.hpp"

namespace {
bool count_new = false;
bool fail_new = false;
unsigned allocations = 0;
}

// Test executable only: observe/inject ordinary C++ allocation, product unchanged.
void* operator new(std::size_t n) {
  if (count_new) ++allocations;
  if (fail_new) throw std::bad_alloc();
  if (void* p = std::malloc(n ? n : 1)) return p;
  throw std::bad_alloc();
}
void* operator new(std::size_t n, const std::nothrow_t&) noexcept {
  try { return ::operator new(n); } catch (...) { return nullptr; }
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

using namespace mhgp11;

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  const bool mutable_name = std::strstr(argv[1], "mutable") != nullptr;
  const bool reset_ledger = std::strstr(argv[1], "reset") != nullptr;
  const bool fault = std::strstr(argv[1], "fault") != nullptr;
  if (std::strcmp(argv[1], "budget") == 0) {
    MemoryBudget a(16), b(16);
    Buffer<u64> x, y;
    if (!x.allocate(1,a).ok() || !y.allocate(1,b).ok()) return 3;
    x[0]=11;y[0]=22;
    x=std::move(y);
    if (a.used()!=0 || b.used()!=8 || x[0]!=22 || !y.empty()) return 4;
    const auto refused=x.allocate(3,b);
    if (refused.reason!=Reason::memory_budget || !x.empty() || b.used()!=0) return 5;
    Buffer<u64> survivor;
    { MemoryBudget short_lived(8);if(!survivor.allocate(1,short_lived).ok()) return 6;survivor[0]=77; }
    if(survivor[0]!=77) return 7;
    survivor.reset();
    std::printf("{\"status\":\"PASS\",\"cross_budget_move\":true,\"destructive_refusal_documented\":true,\"survives_budget_owner\":true}\n");
    return 0;
  }
  if (fault) std::set_terminate([] {
    std::fputs("stage_timer_destructor_called_terminate_after_injected_bad_alloc\n", stderr);
    std::_Exit(42);  // Observe termination deterministically instead of abort/core dump.
  });
  Ledger ledger;
  std::string name="abc";
  {
    StageTimer timer(ledger, mutable_name ? std::string_view(name) : std::string_view("abc"));
    ledger.time("abc", ~u64{0});
    if (mutable_name) name[0]='d';  // View remains valid, same allocation, owner alive.
    if (reset_ledger) ledger=Ledger{};  // Same Ledger object alive, valid public assignment.
    count_new=true;
    fail_new=fault;
  }
  count_new=false;fail_new=false;
  const auto text=ledger.to_json();
  const bool abc=text.find("\"abc\":")!=std::string::npos;
  const bool dbc=text.find("\"dbc\":")!=std::string::npos;
  std::printf("{\"mode\":\"%s\",\"destructor_allocations\":%u,\"abc_present\":%s,\"dbc_present\":%s,\"stage_and_ledger_alive\":true}\n",
    argv[1],allocations,abc?"true":"false",dbc?"true":"false");
}
