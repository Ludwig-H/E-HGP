// Diametres ex aequo, extremes et replis : attendus analytiques independants du selecteur produit.
#include "test_support.hpp"
#include "test.hpp"
using namespace tower_test;
namespace {
struct Fixture { std::vector<Xyz> points; unsigned arity; i64 radius_num, radius_den; u64 candidates; };
bool contains(const Cloud& cloud, const BoundedMeb& meb) {
  for (SiteIdx s : all(cloud)) {
    const auto p = num::Point::make(cloud.x()[idx(s)], cloud.y()[idx(s)], cloud.z()[idx(s)]);
    if (!p.ok()) return false;
    const auto side = num::side(meb.sphere(), p.value());
    if (!side.ok() || side.value() > 0) return false;
  }
  return true;
}
std::vector<Fixture> canonical() {
  const u32 m = kCoordMax; const i64 square = i64{m} * m;
  std::vector<Fixture> out{{{{3,4,5}}, 1, 0, 1, 1},
    {{{0,0,0},{m,m,m}}, 2, 3 * square, 4, 1},
    {{{0,0,0},{4,0,0},{0,4,0},{4,4,0}}, 2, 8, 1, 1}};
  std::vector<Xyz> cube, line;
  for (u32 x : {0u,m}) for (u32 y : {0u,m}) for (u32 z : {0u,m}) cube.push_back({x,y,z});
  for (u32 i = 0; i < 12; ++i) line.push_back({m * i / 11,0,0});
  out.push_back({cube, 2, 3 * square, 4, 1}); out.push_back({line, 2, square, 4, 1});
  return out;
}
}  // namespace

MHGP11_TEST(canonical_pairs, 66) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  for (const auto& f : canonical()) {
    auto cloud = Input(f.points).prepare(budget); REQUIRE(cloud.ok());
    auto part = all(cloud.value()); const u64 held = budget.used(), n = part.size();
    auto meb = bounded_meb(cloud.value(), part); REQUIRE(meb.ok());
    std::vector<SiteIdx> expected{SiteIdx{0}};
    if (n > 1) expected.push_back(SiteIdx{static_cast<u32>(n - 1)});
    CHECK_EQ(meb.value().support().size(), f.arity); CHECK(equal(meb.value().support(), expected));
    CHECK_EQ(meb.value().ledger().presentations, 1u);
    CHECK_EQ(meb.value().ledger().diameter_pairs, n * (n - 1) / 2);
    CHECK_EQ(meb.value().ledger().point_tests, n);
    CHECK(support_strict(cloud.value(), meb.value())); CHECK(level_is(meb.value().sphere(), f.radius_num, f.radius_den));
    std::reverse(part.begin(), part.end()); auto reversed = bounded_meb(cloud.value(), part); REQUIRE(reversed.ok());
    CHECK(reversed.value().ledger() == meb.value().ledger()); CHECK(equal(reversed.value().support(), expected));
    CHECK_EQ(budget.used(), held);
  }
  CHECK(budget.released().ok());
}

MHGP11_TEST(fallback, 45) {
  const u32 m = kCoordMax;
  const std::vector<Fixture> fixtures{
    {{{0,0,0},{4,0,0},{2,3,0}}, 3, 169, 36, 2},
    {{{1,2,0},{0,5,0},{8,1,0},{8,9,0}}, 3, 25, 1, 4},
    {{{10,5,5},{9,8,5},{5,2,1},{1,5,8}}, 4, 25, 1, 6},
    {{{0,0,0},{m,m,0},{m,0,m},{0,m,m}}, 4, 3 * i64{m} * m, 4, 6}};
  MemoryBudget budget(MemoryBudget::kUnlimited);
  for (const auto& f : fixtures) {
    auto cloud = Input(f.points).prepare(budget); REQUIRE(cloud.ok());
    auto part = all(cloud.value()); const u64 held = budget.used(), n = part.size();
    auto meb = bounded_meb(cloud.value(), part); REQUIRE(meb.ok());
    CHECK_EQ(meb.value().support().size(), f.arity); CHECK_EQ(meb.value().ledger().presentations, f.candidates);
    CHECK_EQ(meb.value().ledger().diameter_pairs, n * (n - 1) / 2);
    CHECK(support_strict(cloud.value(), meb.value())); CHECK(level_is(meb.value().sphere(), f.radius_num, f.radius_den));
    CHECK(contains(cloud.value(), meb.value())); CHECK_EQ(budget.used(), held);
    std::reverse(part.begin(), part.end()); auto reversed = bounded_meb(cloud.value(), part); REQUIRE(reversed.ok());
    CHECK(equal(reversed.value().support(), meb.value().support()));
  }
  CHECK(budget.released().ok());
}
MHGP11_TEST_MAIN()
