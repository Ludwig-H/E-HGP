// Porte du module io sous penurie de memoire injectee. Les operateurs new sont remplaces dans ce seul executable
// (hors produit) : la k-ieme allocation sans exception echoue, et chaque allocation avec exception est comptee.
//   starvation : dans l'ordre du CLI (plan, lecture, creation, commit), read_u32le refuse memory_budget a chacune de
//                ses quatre allocations tour a tour sans rien laisser de reserve, et le dossier abandonne n'est pas
//                publie ; plan, create et commit n'appellent aucun operator new ; aucun operator new qui leve n'est
//                jamais appele (le noexcept du module est honnete).
// Les quatre formes scalaires sont remplacees ensemble (tests/core/alloc_fault.cpp).
#include <cstdlib>
#include <new>
#include <string>
#include <vector>

#include "io_support.hpp"
#include "test.hpp"

namespace {

long nothrow_countdown = -1;  // >= 0 : allocations sans exception encore admises avant un echec
unsigned long long nothrow_calls = 0, throwing_calls = 0;

}  // namespace

[[gnu::noinline]] void* operator new(std::size_t size) {
  ++throwing_calls;
  void* block = std::malloc(size == 0 ? 1 : size);
  if (block == nullptr) throw std::bad_alloc();
  return block;
}
[[gnu::noinline]] void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
  ++nothrow_calls;
  if (nothrow_countdown == 0) return nullptr;
  if (nothrow_countdown > 0) --nothrow_countdown;
  return std::malloc(size == 0 ? 1 : size);
}
[[gnu::noinline]] void operator delete(void* block) noexcept { std::free(block); }
[[gnu::noinline]] void operator delete(void* block, std::size_t) noexcept { std::free(block); }

using namespace mhgp12;
using namespace mhgp12::io_test;

MHGP12_TEST(starvation, 15) {
  Scratch s;
  REQUIRE(s.ok());
  std::vector<u8> points, ids;
  for (u32 i = 0; i < 5; ++i) {
    put32(points, i);
    put32(points, 2 * i);
    put32(points, 3 * i);
    put32(ids, 100 + i);
  }
  REQUIRE(write_file(s.path("points.u32le"), points));
  REQUIRE(write_file(s.path("ids.u32le"), ids));
  const std::string p = s.path("points.u32le"), q = s.path("ids.u32le"), d = s.path("D");
  const std::vector<const char*> inputs{p.c_str(), q.c_str()};

  MemoryBudget budget(MemoryBudget::kUnlimited);
  unsigned refusals = 0, clean = 0;
  bool published = false;
  unsigned long long throwing_inside = 0, nothrow_in_transaction = 0;
  for (long k = 0; k < 16 && !published; ++k) {
    const unsigned long long throwing_before = throwing_calls;
    unsigned long long nothrow_before = nothrow_calls;
    Result<io::OutputDirectory> planned = io::OutputDirectory::plan(d.c_str(), inputs);
    nothrow_in_transaction += nothrow_calls - nothrow_before;
    if (!planned.ok()) break;
    io::OutputDirectory out = std::move(planned).take();
    nothrow_countdown = k;
    Result<io::InputFiles> read = io::read_u32le(p.c_str(), q.c_str(), budget);
    nothrow_countdown = -1;
    if (!read.ok()) {
      ++refusals;
      clean += read.outcome().reason == Reason::memory_budget && budget.used() == 0 ? 1 : 0;
      throwing_inside += throwing_calls - throwing_before;
      continue;  // le dossier planifie est abandonne : rien n'est cree
    }
    nothrow_before = nothrow_calls;
    Result<io::FileWriter*> w = out.create("x.bin");
    const bool written =
        w.ok() && w.value()->u32s(read.value().x.span()).ok() && w.value()->u32s(read.value().y.span()).ok();
    published = written && out.commit("{}").ok();
    nothrow_in_transaction += nothrow_calls - nothrow_before;
    throwing_inside += throwing_calls - throwing_before;
    CHECK_EQ(read.value().x[4], 4u);
    CHECK_EQ(idx(read.value().ids[4]), 104u);
  }
  CHECK(published);
  // quatre tableaux, donc quatre refus avant le succes, chacun propre
  CHECK_EQ(refusals, 4u);
  CHECK_EQ(clean, refusals);
  CHECK_EQ(throwing_inside, 0u);
  CHECK_EQ(nothrow_in_transaction, 0u);
  CHECK(budget.released().ok());
  CHECK(entries(s.root()) == std::vector<std::string>({"D", "ids.u32le", "points.u32le"}));
  CHECK(entries(d) == std::vector<std::string>({"manifeste.json", "x.bin"}));
  CHECK_EQ(read_file(d + "/x.bin").size(), 40u);
  // temoin : le remplacement est bien celui que le produit appelle
  nothrow_countdown = 0;
  Buffer<u32> probe;
  CHECK_EQ(probe.allocate(4, budget).reason, Reason::memory_budget);
  nothrow_countdown = -1;
}

MHGP12_TEST_MAIN()
