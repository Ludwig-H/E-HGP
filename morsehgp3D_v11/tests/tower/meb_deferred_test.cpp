// Differentiel de controle : ancienne voie eager, positivite q4 GENERIQUE, compteurs et champs bruts.
// La geometrie independante est jugee ailleurs par Gram/Fraction ; ce temoin vise le changement de route.
#include "test_support.hpp"
#include "test.hpp"
using namespace tower_test;
namespace {
struct Eager {
  std::optional<num::Sphere> sphere;
  std::array<SiteIdx,4> support{};
  u8 arity=0;
  MebLedger ledger;
  u64 q3_degenerate=0,q3_non_strict=0,q4_degenerate=0,q4_non_strict=0,q4_outside=0;
};
bool advance(std::array<u32,4>& tuple,u8 q,u32 n) {
  for (u8 j=q;j!=0;) {
    --j;
    if (tuple[j]<n-q+j) {
      ++tuple[j];
      for (u8 k=static_cast<u8>(j+1);k<q;++k) tuple[k]=tuple[k-1]+1;
      return true;
    }
  }
  return false;
}
Result<std::optional<num::Sphere>> through(const std::array<num::Point,12>& p,
                                         const std::array<u32,4>& t,u8 q) {
  if (q==1) return std::optional<num::Sphere>{num::Sphere::point(p[t[0]])};
  if (q==2) return num::Sphere::through(p[t[0]],p[t[1]]);
  if (q==3) return num::Sphere::through(p[t[0]],p[t[1]],p[t[2]]);
  return num::Sphere::through(p[t[0]],p[t[1]],p[t[2]],p[t[3]]);
}
Eager eager(const Cloud& cloud,const std::vector<SiteIdx>& ids) {
  Eager out; std::array<num::Point,12> p{}; const u32 n=static_cast<u32>(ids.size());
  for (u32 i=0;i<n;++i) {
    const auto s=idx(ids[i]); p[i]=num::Point::make(cloud.x()[s],cloud.y()[s],cloud.z()[s]).value();
  }
  auto consider=[&](const std::array<u32,4>& t,u8 q) {
    ++out.ledger.presentations;
    const auto made=through(p,t,q);
    if (!made.ok()) throw std::runtime_error("eager sphere refused");
    if (!made.value()) {
      if (q==3) ++out.q3_degenerate;
      if (q==4) ++out.q4_degenerate;
      return;
    }
    ++out.ledger.nondegenerate;
    bool positive=q<3 || num::strictly_acute(p[t[0]],p[t[1]],p[t[2]]);
    if (q==4) {
      const auto inside=num::strictly_inside(*made.value(),p[t[0]],p[t[1]],p[t[2]],p[t[3]]);
      if (!inside.ok()) throw std::runtime_error("eager positivity refused");
      positive=inside.value();
    }
    if (!positive) {
      if (q==3) ++out.q3_non_strict;
      if (q==4) ++out.q4_non_strict;
      return;
    }
    ++out.ledger.positive;
    for (u32 i=0;i<n;++i) {
      ++out.ledger.point_tests; const auto side=num::side(*made.value(),p[i]);
      if (!side.ok()) throw std::runtime_error("eager side refused");
      if (side.value()>0) { if (q==4) ++out.q4_outside; return; }
    }
    ++out.ledger.containing; out.sphere=*made.value(); out.arity=q;
    for (u8 j=0;j<q;++j) out.support[j]=ids[t[j]];
  };
  std::array<u32,4> tuple{};
  if (n>1) {
    i64 longest=-1;
    for (u32 a=0;a+1<n;++a) for (u32 b=a+1;b<n;++b) {
      ++out.ledger.diameter_pairs; i64 d=0;
      for (u32 j=0;j<3;++j) { const i64 v=i64{p[a].coordinates()[j]}-p[b].coordinates()[j]; d+=v*v; }
      if (d>longest) { longest=d; tuple[0]=a; tuple[1]=b; }
    }
  }
  consider(tuple,n==1?u8{1}:u8{2});
  for (u8 q=3;q<=4 && q<=n && !out.sphere;++q) {
    tuple={0,1,2,3};
    do { consider(tuple,q); } while (!out.sphere && advance(tuple,q,n));
  }
  return out;
}
std::vector<std::vector<Xyz>> fixtures() {
  return {{{3,4,5}},{{0,0,0},{4,0,0},{2,3,0}},
    {{1,2,0},{0,5,0},{8,1,0},{8,9,0}},
    {{10,5,5},{9,8,5},{5,2,1},{1,5,8}},
    {{0,0,0},{4,4,0},{4,0,4},{0,4,4}},
    {{5,5,0},{2,1,5},{10,5,5},{2,9,5},{5,9,8}},
    {{0,0,0},{1,0,0},{2,0,0},{4,4,0},{4,0,4},{0,4,4}},
    {{0,0,0},{4,4,0},{4,0,4},{0,4,4},{7,1,2},{3,7,1},{1,2,7}},
    {{0,0,5},{2,7,4},{0,7,6},{8,4,0},{8,1,6},{1,2,8}}};
}
void compare_result(const BoundedMeb& actual,const Eager& expected) {
  CHECK(actual.ledger()==expected.ledger); CHECK_EQ(actual.support().size(),expected.arity);
  CHECK(equal(actual.support(),std::span<const SiteIdx>(expected.support.data(),expected.arity)));
  CHECK(actual.sphere().anchor()==expected.sphere->anchor());
  CHECK(actual.sphere().numerator()==expected.sphere->numerator());
  CHECK_EQ(actual.sphere().denominator(),expected.sphere->denominator());
  CHECK_EQ(num::compare(num::to_wide(actual.sphere().level().numerator()),
                        num::to_wide(expected.sphere->level().numerator())),0);
  CHECK_EQ(num::compare(num::to_wide(actual.sphere().level().denominator()),
                        num::to_wide(expected.sphere->level().denominator())),0);
  CHECK_EQ(actual.sphere().presentation_arity(),expected.arity);
}
}
MHGP11_TEST(eager_parity,438) {
  u64 q3_degenerate=0,q3_non_strict=0,q4_degenerate=0,q4_non_strict=0,q4_outside=0;
  MemoryBudget budget(MemoryBudget::kUnlimited);
  for (const auto& base:fixtures()) for (u32 scale:{1u,kCoordMax/16}) {
    auto points=base;
    for (auto& p:points) for (auto& v:p) v*=scale;
    auto input=points; input.push_back({16*scale,16*scale,16*scale}); input.push_back({0,16*scale,0});
    auto cloud=Input(input).prepare(budget); REQUIRE(cloud.ok());
    std::vector<SiteIdx> part; for (const auto& p:points) part.push_back(site(cloud.value(),p));
    std::sort(part.begin(),part.end(),[](SiteIdx a,SiteIdx b){return idx(a)<idx(b);});
    const auto expected=eager(cloud.value(),part); REQUIRE(expected.sphere.has_value());
    const u64 held=budget.used();
    auto actual=bounded_meb(cloud.value(),part); REQUIRE(actual.ok()); compare_result(actual.value(),expected);
    CHECK_EQ(budget.used(),held);
    std::reverse(part.begin(),part.end()); auto reversed=bounded_meb(cloud.value(),part); REQUIRE(reversed.ok());
    compare_result(reversed.value(),expected); CHECK_EQ(budget.used(),held);
    q3_degenerate+=expected.q3_degenerate; q3_non_strict+=expected.q3_non_strict;
    q4_degenerate+=expected.q4_degenerate; q4_non_strict+=expected.q4_non_strict; q4_outside+=expected.q4_outside;
  }
  CHECK(q3_degenerate>0); CHECK(q3_non_strict>0); CHECK(q4_degenerate>0);
  CHECK(q4_non_strict>0); CHECK(q4_outside>0); CHECK(budget.released().ok());
}
MHGP11_TEST_MAIN()
