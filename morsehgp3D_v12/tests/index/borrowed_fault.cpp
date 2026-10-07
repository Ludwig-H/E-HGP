// Construction refusee position par position ; requetes et garde de rappel sans aucune allocation propre.
#include <cstdlib>
#include <new>
#include "test_support.hpp"
#include "test.hpp"
namespace {
long countdown=-1;
unsigned long long calls=0;
bool deny() noexcept {
  ++calls;
  if (countdown==0) return true;
  if (countdown>0) --countdown;
  return false;
}
}
[[gnu::noinline]] void* operator new(std::size_t size) {
  if (deny()) throw std::bad_alloc();
  void* p=std::malloc(size==0 ? 1:size);
  if (p==nullptr) throw std::bad_alloc();
  return p;
}
[[gnu::noinline]] void* operator new(std::size_t size,const std::nothrow_t&) noexcept {
  return deny() ? nullptr:std::malloc(size==0 ? 1:size);
}
[[gnu::noinline]] void operator delete(void* p) noexcept { std::free(p); }
[[gnu::noinline]] void operator delete(void* p,std::size_t) noexcept { std::free(p); }
using namespace mhgp12;
using namespace index_test;
namespace {
struct Observed {
  u64 calls=0; bool good=true;
  static Outcome callback(void* raw,const BorrowedCensus& result) {
    auto& o=*static_cast<Observed*>(raw); ++o.calls;
    o.good=o.good && result.ledger().passes==1;
    return {};
  }
};
}
MHGP12_TEST(starvation, 20) {
  MemoryBudget owner(MemoryBudget::kUnlimited),work(MemoryBudget::kUnlimited);
  auto cloud=octa().prepare(owner); REQUIRE(cloud.ok());
  auto index=build_index(std::move(cloud.value()),{4},owner); REQUIRE(index.ok());
  const auto base=owner.used();
  for (long position=0;position<2;++position) {
    countdown=position;
    auto denied=CensusWorkspace::make(index.value(),work);
    countdown=-1;
    CHECK(!denied.ok()); CHECK_EQ(denied.outcome().reason,Reason::memory_budget);
    CHECK(work.released().ok()); CHECK_EQ(owner.used(),base);
  }
  const auto before=calls;
  auto workspace=CensusWorkspace::make(index.value(),work); REQUIRE(workspace.ok());
  CHECK_EQ(calls-before,2u); CHECK_EQ(work.used(),44u);
  const auto sphere=ball(),outside=num::Sphere::point(point(100,100,100));
  Observed observed;
  const auto query_start=calls; countdown=0;
  bool success=true;
  for (u32 repeat=0;repeat<20;++repeat) for (u32 k:{1u,4u,kNone}) {
    success=workspace.value()->query(index.value(),sphere,k,&observed,Observed::callback).ok() && success;
    success=workspace.value()->query(index.value(),outside,k,&observed,Observed::callback).ok() && success;
  }
  countdown=-1;
  CHECK(success); CHECK(observed.good); CHECK_EQ(observed.calls,120u); CHECK_EQ(calls,query_start);
  CHECK_EQ(work.used(),44u); workspace.value().reset(); CHECK(work.released().ok()); CHECK_EQ(owner.used(),base);
}
MHGP12_TEST_MAIN()
