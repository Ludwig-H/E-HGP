// Propriete de presentation q4 : le predicat sur un autre tetraedre reste independant.
#include <algorithm>
#include <array>
#include <stdexcept>
#include <type_traits>
#include "num/num.hpp"
#include "num/q4_weights.hpp"
#include "profile_values.hpp"
#include "test.hpp"
using namespace mhgp12;
using namespace mhgp12::num;
namespace {
using Tuple=std::array<Point,4>;
Point point(i64 x,i64 y,i64 z) {
  const auto p=Point::make(x,y,z);
  if (!p.ok()) throw std::runtime_error("fixture q4 hors domaine");
  return p.value();
}
Tuple regular(i64 s,i64 shift=0) {
  return {point(shift,shift,shift),point(s+shift,s+shift,shift),
          point(s+shift,shift,s+shift),point(shift,s+shift,s+shift)};
}
Q4Candidate make(const Tuple& p) {
  const auto c=Q4Candidate::through(p[0],p[1],p[2],p[3]);
  if (!c.ok() || !c.value()) throw std::runtime_error("fixture q4 degeneree");
  return *c.value();
}
void judge(const Tuple& p,bool wanted) {
  const auto candidate=make(p);
  CHECK_EQ(candidate.q4_presentation_strictly_inside(),wanted);
  const auto actual=strictly_inside(candidate,p[0],p[1],p[2],p[3]); REQUIRE(actual.ok());
  CHECK_EQ(actual.value(),wanted);
  const auto full=candidate.materialize(); REQUIRE(full.ok());
  const auto eager=Sphere::through(p[0],p[1],p[2],p[3]); REQUIRE(eager.ok() && eager.value());
  CHECK_EQ(full.value().q4_presentation_strictly_inside(),wanted);
  CHECK_EQ(eager.value()->q4_presentation_strictly_inside(),wanted);
  CHECK(full.value().anchor()==candidate.anchor()); CHECK(full.value().numerator()==candidate.numerator());
  CHECK_EQ(full.value().denominator(),candidate.denominator());
  CHECK(eager.value()->anchor()==candidate.anchor()); CHECK(eager.value()->numerator()==candidate.numerator());
  CHECK_EQ(eager.value()->denominator(),candidate.denominator());
  CHECK_EQ(compare(to_wide(full.value().level().numerator()),to_wide(eager.value()->level().numerator())),0);
  CHECK_EQ(compare(to_wide(full.value().level().denominator()),to_wide(eager.value()->level().denominator())),0);
  const auto full_inside=strictly_inside(full.value(),p[0],p[1],p[2],p[3]); REQUIRE(full_inside.ok());
  CHECK_EQ(full_inside.value(),wanted);
  CHECK_EQ(candidate.presentation_arity(),4u); CHECK_EQ(full.value().presentation_arity(),4u);
}
}
MHGP12_TEST(presentation,3600) {
  const std::array<Tuple,4> bases{regular(2),
    Tuple{point(0,0,0),point(4,0,0),point(2,3,0),point(2,0,2)},
    Tuple{point(0,0,0),point(4,0,0),point(0,4,0),point(0,0,4)},
    Tuple{point(10,5,5),point(9,8,5),point(5,2,1),point(1,5,8)}};
  const std::array<bool,4> positives{true,false,false,true};
  for (u32 i=0;i<bases.size();++i) for (i64 scale:{i64{1},i64{1}<<(kCoordBits-4)}) {
    std::array<u32,4> order{0,1,2,3};
    do {
      Tuple p;
      for (u32 j=0;j<4;++j) {
        const auto a=bases[i][order[j]].coordinates();
        p[j]=point(scale*a[0],scale*a[1],scale*a[2]);
      }
      judge(p,positives[i]);
    } while (std::next_permutation(order.begin(),order.end()));
  }
}
MHGP12_TEST(boundaries,50) {
  const auto native=mhgp12::num::detail::q4_weights_i128;
  CHECK_EQ(sizeof(Sphere),profile_test::kSphereBytes);
  CHECK_EQ(sizeof(Q4Candidate),profile_test::kQ4CandidateBytes);
  judge(regular(kCoordMax),true);
  CHECK(!native(regular(kCoordMax)));
  judge(regular(4,kCoordMax-4),true); CHECK(native(regular(4,kCoordMax-4)));
  for (const auto p:{Tuple{point(0,0,0),point(4,0,0),point(4,4,0),point(0,4,0)},
                     Tuple{point(0,0,0),point(4,0,0),point(4,4,0),point(0,0,0)}}) {
    auto c=Q4Candidate::through(p[0],p[1],p[2],p[3]); REQUIRE(c.ok()); CHECK(!c.value());
    auto s=Sphere::through(p[0],p[1],p[2],p[3]); REQUIRE(s.ok()); CHECK(!s.value());
  }
  // Frontiere du certificat natif aux profils 21 et 24 (la branche du profil 18 de la v11 est retiree avec lui).
  for (i64 delta:{i64{-1},i64{0},i64{1}}) {
    const i64 length=(i64{1}<<20)+delta;
    auto p=regular(length); CHECK_EQ(native(p),delta<=0); judge(p,true);
    const i64 offset=kCoordMax-length;
    p=regular(length,offset); CHECK_EQ(native(p),delta<=0); judge(p,true);
  }
}
MHGP12_TEST(foreign_and_owners,40) {
  const std::array<Point,5> shell{point(5,5,0),point(2,1,5),point(10,5,5),point(2,9,5),point(5,9,8)};
  auto c=make({shell[0],shell[1],shell[2],shell[4]}); REQUIRE(c.q4_presentation_strictly_inside());
  for (const auto tuple:{std::array<u32,4>{0,1,2,3},std::array<u32,4>{0,1,3,4},std::array<u32,4>{1,2,3,4}}) {
    const auto other=make({shell[tuple[0]],shell[tuple[1]],shell[tuple[2]],shell[tuple[3]]});
    CHECK(!other.q4_presentation_strictly_inside());
    auto inside=strictly_inside(c,shell[tuple[0]],shell[tuple[1]],shell[tuple[2]],shell[tuple[3]]);
    REQUIRE(inside.ok()); CHECK(!inside.value());
    const auto a=c.materialize(),b=other.materialize(); REQUIRE(a.ok() && b.ok());
    CHECK_EQ(compare_centers(a.value(),b.value()),0); CHECK_EQ(compare(a.value().level(),b.value().level()),0);
    inside=strictly_inside(a.value(),shell[tuple[0]],shell[tuple[1]],shell[tuple[2]],shell[tuple[3]]);
    REQUIRE(inside.ok()); CHECK(!inside.value());
  }
  const auto saved=c; c=make({point(0,0,0),point(4,0,0),point(0,4,0),point(0,0,4)});
  CHECK(saved.q4_presentation_strictly_inside()); CHECK(!c.q4_presentation_strictly_inside());
  auto copied=saved.materialize(); REQUIRE(copied.ok()); CHECK(copied.value().q4_presentation_strictly_inside());
  auto moved=std::move(copied.value()); CHECK(moved.q4_presentation_strictly_inside());
  c=saved; CHECK(c.q4_presentation_strictly_inside());
  const auto pair=Sphere::through(point(0,1,1),point(2,1,1)); REQUIRE(pair.ok() && pair.value());
  const auto tri=Sphere::through(point(0,0,1),point(2,0,1),point(0,2,1)); REQUIRE(tri.ok() && tri.value());
  const auto hull=regular(2);
  for (const auto& sphere:{Sphere::point(point(1,1,1)),*pair.value(),*tri.value()}) {
    CHECK(!sphere.q4_presentation_strictly_inside());
    const auto inside=strictly_inside(sphere,hull[0],hull[1],hull[2],hull[3]);
    REQUIRE(inside.ok()); CHECK(inside.value());
  }
  static_assert(!std::is_default_constructible_v<Q4Candidate> && !std::is_default_constructible_v<Sphere>);
}
MHGP12_TEST_MAIN()
