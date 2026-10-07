// Certificat global d'orientation : normale triangulaire commune, candidats fermes et reference toujours Wide.
#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

#include "num/num.hpp"
#include "num/orientation_certificate.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::num;
namespace {
Point point(i64 x,i64 y,i64 z) {
  const auto p=Point::make(x,y,z);
  if (!p.ok()) throw std::runtime_error("orientation fixture hors domaine");
  return p.value();
}
Q4Candidate candidate(const std::array<Point,4>& p) {
  const auto made=Q4Candidate::through(p[0],p[1],p[2],p[3]);
  if (!made.ok() || !made.value()) throw std::runtime_error("orientation fixture degeneree");
  return *made.value();
}
Sphere triangle(i64 scale) {
  const auto made=Sphere::through(point(0,0,0),point(scale,scale,0),point(scale,0,scale));
  if (!made.ok() || !made.value()) throw std::runtime_error("triangle degenere");
  return *made.value();
}
std::array<Point,4> regular(i64 scale) {
  return {point(0,0,0),point(scale,scale,0),point(scale,0,scale),point(0,scale,scale)};
}
template<class Ball>
Wide<4> reference(Point a,Point b,Point c,const Ball& ball) {
  std::array<i64,3> u{},v{},offset{};
  for (u32 j=0;j<3;++j) {
    u[j]=i64{b.coordinates()[j]}-a.coordinates()[j];
    v[j]=i64{c.coordinates()[j]}-a.coordinates()[j];
    offset[j]=i64{ball.anchor().coordinates()[j]}-a.coordinates()[j];
  }
  const std::array<i64,3> normal{u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};
  Wide<4> total;
  for (u32 j=0;j<3;++j) {
    const i128 coordinate=ball.numerator()[j]+ball.denominator()*offset[j];
    const auto term=multiply(to_wide(coordinate),to_wide(i128{normal[j]}));
    Wide<4> next;
    if (!add(total,term,next)) throw std::runtime_error("reference orientation trop large");
    total=next;
  }
  return total;
}
template<class Ball>
void judge(const Ball& ball,Point a,Point b,Point c) {
  const auto forward=orientation(a,b,c,ball), reverse=orientation(a,c,b,ball), flat=orientation(a,a,c,ball);
  REQUIRE(forward.ok() && reverse.ok() && flat.ok());
  const int expected=reference(a,b,c,ball).sign();
  CHECK_EQ(forward.value(),expected); CHECK_EQ(reverse.value(),-expected); CHECK_EQ(flat.value(),0);
}
}

MHGP12_TEST(orientation_limits,97) {
  constexpr i128 d=i128{1}<<(124-3*kCoordBits), n=i128{1}<<(124-2*kCoordBits);
  const auto cert=mhgp12::num::detail::global_orientation_i128;
  const i128 maximum=static_cast<i128>((u128{1}<<127)-1), minimum=-maximum-1;
  CHECK(!cert(0,{})); CHECK(!cert(-1,{})); CHECK(cert(1,{}));
  CHECK(cert(d-1,{})); CHECK(!cert(d,{})); CHECK(!cert(d+1,{}));
  for (u32 axis=0;axis<3;++axis) {
    std::array<i128,3> ns{};
    ns[axis]=n-1; CHECK(cert(1,ns)); ns[axis]=n; CHECK(!cert(1,ns)); ns[axis]=n+1; CHECK(!cert(1,ns));
    ns[axis]=-n+1; CHECK(cert(1,ns)); ns[axis]=-n; CHECK(!cert(1,ns)); ns[axis]=-n-1; CHECK(!cert(1,ns));
    ns[axis]=maximum; CHECK(!cert(1,ns)); ns[axis]=minimum; CHECK(!cert(1,ns));
  }
  // Les extrema multiaffines sont aux coins du carre. Un seul ancrage pour les deux differences.
  bool positive=false,negative=false;
  for (u32 a=0;a<4;++a) for (u32 b=0;b<4;++b) for (u32 c=0;c<4;++c) {
    const i64 ax=a&1u,ay=a>>1,bx=b&1u,by=b>>1,cx=c&1u,cy=c>>1;
    const i64 normal=(bx-ax)*(cy-ay)-(by-ay)*(cx-ax);
    CHECK(normal>=-1 && normal<=1); positive=positive||normal==1; negative=negative||normal==-1;
  }
  CHECK(positive); CHECK(negative);
  const i64 m=kCoordMax;
  CHECK(m*(-m)-m*m < -m*m);  // Deux Vec arbitraires peuvent donner 2m^2 : preuve inapplicable.
}

MHGP12_TEST(orientation_public,1050) {
  CHECK_EQ(sizeof(Sphere),kCoordBits==21?std::size_t{144}:std::size_t{160});
  CHECK_EQ(sizeof(Q4Candidate),std::size_t{80});
  const auto zero=point(0,0,0), y=point(0,kCoordMax,0), z=point(0,0,kCoordMax);
  for (const i64 scale : {i64{4},i64{kCoordMax}}) {
    const auto points=regular(scale); std::array<u32,4> order{0,1,2,3};
    do {
      const std::array<Point,4> p{points[order[0]],points[order[1]],points[order[2]],points[order[3]]};
      const auto ball=candidate(p);
      REQUIRE(ball.orientation_i128_certified()==(scale==4));
      const auto full=ball.materialize(); REQUIRE(full.ok());
      const auto eager=Sphere::through(p[0],p[1],p[2],p[3]); REQUIRE(eager.ok() && eager.value());
      CHECK(full.value().orientation_i128_certified()==ball.orientation_i128_certified());
      CHECK(eager.value()->orientation_i128_certified()==ball.orientation_i128_certified());
      CHECK(full.value().numerator()==ball.numerator()); CHECK_EQ(full.value().denominator(),ball.denominator());
      CHECK(full.value().anchor()==ball.anchor());
      CHECK_EQ(compare(to_wide(full.value().level().numerator()),to_wide(eager.value()->level().numerator())),0);
      CHECK_EQ(compare(to_wide(full.value().level().denominator()),to_wide(eager.value()->level().denominator())),0);
      judge(ball,zero,z,y); judge(full.value(),zero,z,y); judge(*eager.value(),zero,z,y);
      const auto inside=strictly_inside(ball,p[0],p[1],p[2],p[3]); REQUIRE(inside.ok()); CHECK(inside.value());
    } while (std::next_permutation(order.begin(),order.end()));
  }
}

MHGP12_TEST(orientation_owners,34) {
  const auto zero=point(0,0,0), top=point(kCoordMax,kCoordMax,kCoordMax);
  auto sphere=triangle(4); REQUIRE(sphere.orientation_i128_certified());
  sphere=triangle(kCoordMax); REQUIRE(!sphere.orientation_i128_certified());
  CHECK(!sphere.q3_power_i128_certified());  // Les deux certificats sont distincts.
  judge(sphere,zero,point(0,0,kCoordMax),point(0,kCoordMax,0));
  sphere=triangle(4); REQUIRE(sphere.orientation_i128_certified());
  CHECK(Sphere::point(top).orientation_i128_certified());
  const auto pair=Sphere::through(zero,top); REQUIRE(pair.ok() && pair.value());
  CHECK(pair.value()->orientation_i128_certified()); judge(*pair.value(),zero,point(1,0,0),point(0,1,0));
  auto p=regular(4); auto ball=candidate(p); REQUIRE(ball.orientation_i128_certified());
  const auto copied=ball; p[0]=top; CHECK(copied.anchor()==zero);
  ball=candidate(regular(kCoordMax)); REQUIRE(!ball.orientation_i128_certified());
  const auto large=ball.materialize(); REQUIRE(large.ok());
  CHECK(large.value().orientation_i128_certified()==ball.orientation_i128_certified());
  const auto xy=point(kCoordMax,kCoordMax,0), z=point(0,0,kCoordMax), y=point(0,kCoordMax,0);
  const auto contact=orientation(zero,xy,z,ball); REQUIRE(contact.ok()); CHECK_EQ(contact.value(),0);
  const auto negative=orientation(zero,z,y,ball); REQUIRE(negative.ok()); CHECK_EQ(negative.value(),-1);
  const auto positive=orientation(zero,y,z,large.value()); REQUIRE(positive.ok()); CHECK_EQ(positive.value(),1);
  CHECK(reference(zero,z,y,ball).bit_length()==(kCoordBits==21?127:145));
  if constexpr (kCoordBits==24) {
    const auto exact=reference(zero,z,y,ball);
    const u128 magnitude=u128{exact.words[0]}|(u128{exact.words[1]}<<64);
    const u128 wrapped=u128{0}-magnitude;
    CHECK(exact.sign()==-1); CHECK((wrapped>>(127))==0 && wrapped!=0);  // Signe faux si troncature 128 bits.
  }
  ball=copied; REQUIRE(ball.orientation_i128_certified());
  const auto restored=ball.materialize(); REQUIRE(restored.ok()); CHECK(restored.value().orientation_i128_certified());
  judge(restored.value(),zero,z,y);
  {  // Aux profils 21 et 24 (garde de la v11 pour son profil 18, abandonne).
    constexpr int exponent=(124-3*kCoordBits-1)/3;
    const i64 s=i64{1}<<exponent;
    for (const i64 offset : {i64{-1},i64{0},i64{1}}) {
      const auto boundary=candidate({zero,point(s,0,0),point(0,s,0),point(0,0,s+offset)});
      REQUIRE(boundary.orientation_i128_certified()==(offset<0));
      const auto full=boundary.materialize(); REQUIRE(full.ok());
      CHECK(full.value().orientation_i128_certified()==boundary.orientation_i128_certified());
      judge(boundary,zero,z,y);
    }
  }
}
