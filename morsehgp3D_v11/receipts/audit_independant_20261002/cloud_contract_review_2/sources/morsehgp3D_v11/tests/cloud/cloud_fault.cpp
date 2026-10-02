// Porte de cloud sous penurie de memoire injectee. Les operateurs new sont remplaces dans ce seul executable (hors
// produit) : la k-ieme allocation sans exception echoue, et chaque allocation avec exception est comptee.
//   starvation : prepare_cloud refuse memory_budget a chacune de ses allocations tour a tour, ne laisse rien de
//                reserve, ne leve jamais, et n'appelle jamais l'operateur new qui leve (son noexcept est honnete).
//
// Les quatre formes scalaires sont remplacees ensemble : sous un sanitizer, un bloc de malloc rendu par l'operator
// delete du runtime serait un desaccord d'allocateur (tests/core/alloc_fault.cpp).
#include <cstdlib>
#include <new>
#include <vector>

#include "cloud/cloud.hpp"
#include "test.hpp"

namespace {

// Etat de test, un seul fil.
long nothrow_countdown = -1;  // >= 0 : allocations sans exception encore admises avant un echec
unsigned long long nothrow_calls = 0, throwing_calls = 0;

}  // namespace

void* operator new(std::size_t size) {
  ++throwing_calls;
  void* block = std::malloc(size == 0 ? 1 : size);
  if (block == nullptr) throw std::bad_alloc();
  return block;
}
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
  ++nothrow_calls;
  if (nothrow_countdown == 0) return nullptr;
  if (nothrow_countdown > 0) --nothrow_countdown;
  return std::malloc(size == 0 ? 1 : size);
}
void operator delete(void* block) noexcept { std::free(block); }
void operator delete(void* block, std::size_t) noexcept { std::free(block); }

using namespace mhgp11;

MHGP11_TEST(starvation, 9) {
  // six points, trois sites (cles 0, 3 et 65) ; attendu ecrit a la main
  const std::vector<u32> x{5, 0, 5, 1, 0, 5}, y{0, 0, 0, 1, 0, 0}, z{0, 0, 0, 0, 0, 0};
  std::vector<PointId> ids;
  for (u32 id : {30u, 10u, 20u, 50u, 40u, 60u}) ids.push_back(make_id<PointId>(id));
  const std::vector<u32> want_x{0, 1, 5}, want_w{2, 1, 3}, want_val{10, 40, 50, 20, 30, 60};

  MemoryBudget budget(MemoryBudget::kUnlimited);
  unsigned refusals = 0, clean = 0;
  bool done = false;
  unsigned long long throwing_inside = 0, nothrow_inside = 0;
  for (long k = 0; k < 64 && !done; ++k) {
    const unsigned long long throwing_before = throwing_calls, nothrow_before = nothrow_calls;
    nothrow_countdown = k;
    const Result<Cloud> r = prepare_cloud(x, y, z, ids, CoordWidth(), budget);
    nothrow_countdown = -1;
    throwing_inside += throwing_calls - throwing_before;
    if (!r.ok()) {
      ++refusals;
      clean += r.outcome().reason == Reason::memory_budget && budget.used() == 0 ? 1 : 0;
      continue;
    }
    done = true;
    nothrow_inside = nothrow_calls - nothrow_before;
    const Cloud& c = r.value();
    bool same = c.sites() == 3 && c.weight == 6 && c.ids.well_formed();
    for (u32 s = 0; same && s < 3; ++s) same = c.x[s] == want_x[s] && c.w[s] == want_w[s];
    for (u64 i = 0; same && i < 6; ++i) same = idx(c.ids.val[i]) == want_val[i];
    CHECK(same);
  }
  CHECK(done);
  // trois tableaux de tri puis six tableaux du resultat : neuf allocations, donc neuf refus avant le succes
  CHECK_EQ(refusals, 9u);
  CHECK_EQ(clean, refusals);
  CHECK_EQ(nothrow_inside, 9u);
  CHECK_EQ(throwing_inside, 0u);
  CHECK(budget.released().ok());
  CHECK_EQ(status_of(Reason::memory_budget), Status::resource_exhausted);
  // temoin : le remplacement est bien celui que le produit appelle
  nothrow_countdown = 0;
  Buffer<u32> probe;
  CHECK_EQ(probe.allocate(4, budget).reason, Reason::memory_budget);
  nothrow_countdown = -1;
}

MHGP11_TEST_MAIN()
