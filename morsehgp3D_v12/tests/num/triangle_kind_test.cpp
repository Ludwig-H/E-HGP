// Triangles classes AVANT centre/niveau ; Sphere::through reste generique sur les droits et obtus.
#include <algorithm>
#include <array>
#include <stdexcept>
#include "num/num.hpp"
#include "test.hpp"
using namespace mhgp12;
using namespace mhgp12::num;
namespace {
using Triple = std::array<Point,3>;
Point point(i64 x,i64 y,i64 z) {
  const auto p=Point::make(x,y,z);
  if (!p.ok()) throw std::runtime_error("fixture triangle hors domaine");
  return p.value();
}
void judge(const Triple& p,TriangleKind expected) {
  CHECK_EQ(classify_triangle(p[0],p[1],p[2]),expected);
  CHECK_EQ(strictly_acute(p[0],p[1],p[2]),expected==TriangleKind::strict);
  const auto sphere=Sphere::through(p[0],p[1],p[2]); REQUIRE(sphere.ok());
  CHECK_EQ(sphere.value().has_value(),expected!=TriangleKind::degenerate);
}
}
MHGP12_TEST(kinds,240) {
  const std::array<Triple,5> cases{{
    {point(0,0,0),point(0,0,0),point(4,0,0)},
    {point(0,0,0),point(2,0,0),point(4,0,0)},
    {point(0,0,0),point(4,0,0),point(2,3,0)},
    {point(0,0,0),point(4,0,0),point(0,3,0)},
    {point(0,0,0),point(4,0,0),point(1,1,0)}}};
  const std::array<TriangleKind,5> kinds{TriangleKind::degenerate,TriangleKind::degenerate,
    TriangleKind::strict,TriangleKind::non_strict,TriangleKind::non_strict};
  for (u32 i=0;i<cases.size();++i) for (i64 scale:{i64{1},i64{kCoordMax/4}}) {
    std::array<u32,3> order{0,1,2};
    do {
      Triple p;
      for (u32 j=0;j<3;++j) {
        const auto a=cases[i][order[j]].coordinates();
        p[j]=point(scale*a[0],scale*a[1],scale*a[2]);
      }
      judge(p,kinds[i]);
    } while (std::next_permutation(order.begin(),order.end()));
  }
}
MHGP12_TEST(extremes,60) {
  const i64 m=kCoordMax;
  const std::array<Triple,5> cases{{
    {point(m,m,m),point(m-1,m,m),point(m,m-1,m)},
    {point(0,0,0),point(m,m,0),point(m,0,m)},
    {point(0,0,0),point(m,m,m),point(m-1,m-1,m-1)},
    {point(0,0,0),point(m,m-1,m),point(m-1,m-2,m-1)},
    {point(0,m,m),point(m,0,m),point(m,m,0)}}};
  const std::array<TriangleKind,5> kinds{TriangleKind::non_strict,TriangleKind::strict,
    TriangleKind::degenerate,TriangleKind::non_strict,TriangleKind::strict};
  for (u32 i=0;i<cases.size();++i) for (u32 rotation=0;rotation<3;++rotation) {
    const Triple p{cases[i][rotation],cases[i][(rotation+1)%3],cases[i][(rotation+2)%3]};
    judge(p,kinds[i]);
  }
}
MHGP12_TEST_MAIN()
