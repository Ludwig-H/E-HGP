// Certificat global q3 : seuils stricts internes et vraies Sphere fermees, contre une reference toujours Wide.
#include <algorithm>
#include "checked_power_support.hpp"
#include "num/power_certificate.hpp"
#include "test.hpp"
using namespace mhgp12;
using namespace mhgp12::num;
namespace {
Point point(i64 x, i64 y, i64 z) {
  auto p = Point::make(x,y,z);
  if (!p.ok()) throw std::runtime_error("certificate fixture hors domaine");
  return p.value();
}
Sphere sphere(Point a, Point b, Point c) {
  auto value = Sphere::through(a,b,c);
  if (!value.ok() || !value.value()) throw std::runtime_error("certificate fixture degeneree");
  return *value.value();
}
void judge(const Sphere& s, Point p, Point lo, Point hi) {
  const auto box = Box::make(lo,hi); REQUIRE(box.ok());
  const auto value = power(s,p), lower_power = power(s,lo);
  const auto side_value = side(s,p); const auto bounds = power_bounds(s,box.value());
  REQUIRE(value.ok() && lower_power.ok() && side_value.ok() && bounds.ok());
  CHECK(num_test::equals_wide(value.value(),num_test::wide_power(s,p)));
  CHECK_EQ(side_value.value(),num_test::wide_power(s,p).sign());
  const auto terms = checked_test::box_terms(s,box.value());
  CHECK(num_test::equals_wide(bounds.value().lower,checked_test::wide_sum(s,terms[0])));
  CHECK(num_test::equals_wide(bounds.value().upper,checked_test::wide_sum(s,terms[1])));
}
}

MHGP12_TEST(certificate_limits, 54) {
  constexpr i128 d = i128{1} << (123-2*kCoordBits), n = i128{1} << (124-kCoordBits);
  const i128 maximum = static_cast<i128>((u128{1} << 127)-1), minimum = -maximum-1;
  const auto cert = mhgp12::num::detail::q3_global_power_i128;
  CHECK(!cert(0,{})); CHECK(!cert(-1,{})); CHECK(cert(1,{}));
  CHECK(cert(d-1,{})); CHECK(!cert(d,{})); CHECK(!cert(d+1,{}));
  for (u32 axis = 0; axis < 3; ++axis) {
    std::array<i128,3> ns{};
    ns[axis]=n-1; CHECK(cert(1,ns)); ns[axis]=n; CHECK(!cert(1,ns)); ns[axis]=n+1; CHECK(!cert(1,ns));
    ns[axis]=-n+1; CHECK(cert(1,ns)); ns[axis]=-n; CHECK(!cert(1,ns)); ns[axis]=-n-1; CHECK(!cert(1,ns));
    ns[axis]=maximum; CHECK(!cert(1,ns)); ns[axis]=minimum; CHECK(!cert(1,ns));
  }
  const i64 norm = 3*i64{kCoordMax}*kCoordMax, factor = 2*i64{kCoordMax};
  for (u32 signs = 0; signs < 8; ++signs) {
    std::array<i128,3> ns{};
    for (u32 j = 0; j < 3; ++j) ns[j] = (signs & (u32{1}<<j)) != 0 ? n-1 : -n+1;
    CHECK(cert(d-1,ns));
    const auto value = mhgp12::num::detail::checked_power_sum(d-1,ns,norm,{factor,factor,factor});
    REQUIRE(value.has_value());
    auto reference = multiply(to_wide(d-1),to_wide(i128{norm}));
    for (const auto coordinate : ns) {
      Wide<4> next;
      if (!add(reference,multiply(to_wide(coordinate),to_wide(i128{factor})),next))
        throw std::runtime_error("certificate reference overflow");
      reference = next;
    }
    CHECK(num_test::equals_wide(*value,reference));
  }
}

MHGP12_TEST(certificate_public, 275) {
  CHECK_EQ(sizeof(Sphere), kCoordBits==21 ? std::size_t{144} : std::size_t{160});
  CHECK_EQ(sizeof(Q4Candidate),std::size_t{80});
  const auto zero = point(0,0,0), top = point(kCoordMax,kCoordMax,kCoordMax);
  bool negative = false;
  for (const i64 scale : {i64{4},i64{kCoordMax}}) {
    const std::array<Point,3> pts{zero,point(scale,scale,0),point(scale,0,scale)};
    std::array<u32,3> order{0,1,2};
    do {
      const auto s = sphere(pts[order[0]],pts[order[1]],pts[order[2]]);
      const bool expected = scale==4;
      REQUIRE(s.q3_power_i128_certified()==expected);  // Avant tout calcul susceptible de depasser i128.
      CHECK_EQ(s.presentation_arity(),u8{3});
      const auto copied = s; CHECK(copied.q3_power_i128_certified()==expected);
      auto assigned = Sphere::point(zero); assigned = copied;
      CHECK(assigned.q3_power_i128_certified()==expected); CHECK(assigned.numerator()==s.numerator());
      for (const auto n : s.numerator()) negative = negative || n<0;
      for (const auto p : {zero,pts[1],top}) judge(assigned,p,zero,top);
    } while (std::next_permutation(order.begin(),order.end()));
  }
  CHECK(negative);
}

MHGP12_TEST(certificate_owners, 29) {
  const i64 m=kCoordMax;
  const auto zero=point(0,0,0), top=point(m,m,m);
  const auto small=sphere(zero,point(4,0,0),point(0,4,0));
  const auto large=sphere(zero,point(m,m,0),point(m,0,m));
  REQUIRE(small.q3_power_i128_certified());
  auto copied=small; CHECK(copied.q3_power_i128_certified());
  CHECK(copied.numerator()==small.numerator()); CHECK_EQ(copied.denominator(),small.denominator());
  CHECK(compare(to_wide(copied.level().numerator()),to_wide(small.level().numerator()))==0);
  CHECK(compare(to_wide(copied.level().denominator()),to_wide(small.level().denominator()))==0);
  copied=large; REQUIRE(!copied.q3_power_i128_certified());
  CHECK_EQ(copied.denominator(),large.denominator());
  judge(copied,point(m,m,0),point(m,m,0),point(m,m,0));  // H=0, produit debordant aux grands profils.
  copied=small; REQUIRE(copied.q3_power_i128_certified()); judge(copied,top,zero,top);
  CHECK(!Sphere::point(zero).q3_power_i128_certified());
  const auto pair=Sphere::through(zero,top); REQUIRE(pair.ok() && pair.value());
  CHECK(!pair.value()->q3_power_i128_certified());
  const auto candidate=Q4Candidate::through(zero,point(4,0,0),point(0,4,0),point(0,0,4));
  REQUIRE(candidate.ok() && candidate.value());
  const auto four=candidate.value()->materialize(); REQUIRE(four.ok()); CHECK(!four.value().q3_power_i128_certified());
  const auto result=power(four.value(),top); REQUIRE(result.ok());
  CHECK(num_test::equals_wide(result.value(),num_test::wide_power(*candidate.value(),top)));
  {  // Aux profils 21 et 24 (garde de la v11 pour son profil 18, abandonne).
    const auto near=point(1,0,0);
    const auto attempt=checked_test::attempt(large,checked_test::query_terms(large,near));
    REQUIRE(attempt.has_value()); CHECK(*attempt!=0);
    judge(large,near,near,near);  // Non certifie globalement, mais vraie branche checked non nulle.
    constexpr int cross_bits=(123-2*kCoordBits-1)/2;
    const i64 a=i64{1}<<(cross_bits/2), b=i64{1}<<(cross_bits-cross_bits/2);
    for (const i64 offset : {i64{-1},i64{0},i64{1}}) {
      const auto s=sphere(zero,point(a,0,0),point(0,b+offset,0));
      REQUIRE(s.q3_power_i128_certified()==(offset<0));
      judge(s,top,zero,top);
    }
  }
  if constexpr (kCoordBits==24) {
    const i64 local=(i64{1}<<19)-1;
    const auto s=sphere(zero,point(local,local,0),point(local,0,local));
    REQUIRE(!s.q3_power_i128_certified());
    CHECK(num_test::wide_power(s,top).bit_length()>127);
    judge(s,top,zero,top);
  }
}
