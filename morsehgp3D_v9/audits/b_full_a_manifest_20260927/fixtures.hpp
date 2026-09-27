#pragma once
// Rational fixture catalogue only, never a candidate producer. Integer/key/
// census conversions explicitly port tests/tower/full_ball_tower_gate.cpp
// (SHA c915775562c298a1a926b56ecbe7bbd05555618294c721a70e91020a71e359a7).
// Enumerate positive supports of size <=4 directly instead of all subset MEBs;
// the same complete set of nonzero MEB balls results, including the 12-site
// D5 shell fixture. The native MEB/resolver/lean decisions are not copied here.
#include "manifest.hpp"
#include "../../oracle/tower/local_plateau_oracle.hpp"
#include <bit>
namespace mhgp9::audit::a_manifest::fixtures {
namespace oracle=mhgp9::tower::local_plateau_oracle;
using oracle::Int;using oracle::Rat;
inline Int integer(i128 n) {
  const bool negative=n<0;const u128 magnitude=negative?static_cast<u128>(-(n+1))+1:static_cast<u128>(n);
  Int value=static_cast<u64>(magnitude>>64);value<<=64;value+=static_cast<u64>(magnitude);return negative?-value:value;
}
inline i128 narrow(const Int& value) {
  const bool negative=value<0;const Int magnitude=negative?-value:value;need(magnitude<(Int(1)<<127),"fixture.int128");
  const Int low=magnitude&((Int(1)<<64)-1),high=magnitude>>64;
  const u128 packed=(static_cast<u128>(high.convert_to<u64>())<<64)|low.convert_to<u64>();return negative?-static_cast<i128>(packed):static_cast<i128>(packed);
}
inline Int gcd(Int a,Int b) {if(a<0) a=-a;if(b<0) b=-b;while(b!=0) {const Int r=a%b;a=b;b=r;}return a;}
inline BallKey key(const oracle::Ball& ball) {
  std::array<Rat,5> coefficients{Rat(1),-Rat(2)*ball.center[0],-Rat(2)*ball.center[1],-Rat(2)*ball.center[2],-ball.radius2};
  for(const auto& x:ball.center) coefficients[4]+=x*x;
  Int denominator=1;for(const auto& c:coefficients) denominator=denominator/gcd(denominator,c.denominator())*c.denominator();
  std::array<Int,5> integral;Int divisor=0;
  for(size_t j=0;j<5;++j) {integral[j]=coefficients[j].numerator()*(denominator/coefficients[j].denominator());divisor=gcd(divisor,integral[j]);}
  for(auto& c:integral) c/=divisor;
  return {narrow(integral[0]),{narrow(integral[1]),narrow(integral[2]),narrow(integral[3])},narrow(integral[4])};
}
inline ExactLevel level(const Rat& value) {
  need(value>=Rat(0) && value.numerator()<(Int(1)<<192),"fixture.level");ExactLevel out{};Int remaining=value.numerator(),mask=(Int(1)<<64)-1;
  for(size_t j=0;j<3;++j) {out.num[j]=(remaining&mask).convert_to<u64>();remaining>>=64;}out.den=narrow(value.denominator());return out;
}
inline Rat distance2(const P3& p,const oracle::Ball& ball) {
  const std::array<i64,3> v{p.x,p.y,p.z};Rat out(0);
  for(size_t j=0;j<3;++j) {const Rat d=Rat(v[j])-ball.center[j];out+=d*d;}return out;
}
struct Fixture {const char* name;std::vector<P3> points;unsigned k;};
inline std::vector<Fixture> all() {
  auto out=std::vector<Fixture>{
    {"pair",{{0,0,0},{2,0,0}},2},
    {"triangle",{{0,0,0},{2,2,0},{2,0,2}},3},
    {"tetra",{{0,0,0},{2,2,0},{2,0,2},{0,2,2}},4},
    {"u18_tetra",{{0,0,0},{262142,262142,0},{262142,0,262142},{0,262142,262142}},4},
    {"square",{{0,0,0},{2,0,0},{2,2,0},{0,2,0}},4},
    {"ABCZ",{{1,8,0},{5,10,0},{9,8,0},{5,0,0}},4},
    {"ABCZ_double",{{1,8,0},{5,10,0},{9,8,0},{5,0,0},{201,8,0},{205,10,0},{209,8,0},{205,0,0}},3},
    {"inert",{{2,2,2},{2,0,0},{0,2,0},{0,0,2},{0,0,0}},5},
    {"portal_equal",{{0,2,0},{2,4,0},{4,2,0},{2,0,0},{2,2,0},{2,1,0}},5},
    // Same radius 25: noncontributing positive-triangle block and a distant
    // contributing diameter-pair block at K2. Different catalogue keys.
    {"mixed_equal_radius",{{205,10,0},{197,14,0},{197,6,0},{45,10,0},{55,10,0}},3}
  };
  // D5 counterexample, verbatim coordinates from verifications_adverses.md:
  // a 12-site shell can merge 32 roots at K6, not thirteen.
  Fixture ico{"ico12",{},10};
  for(P3 v:std::vector<P3>{{0,1,7},{5,5,0},{3,4,5},{7,0,1},{4,-3,5},{-5,0,5}})
    for(int sign:{-1,1}) ico.points.push_back({1000+100*sign*v.x,1000+100*sign*v.y,1000+100*sign*v.z});
  out.push_back(std::move(ico));return out;
}
inline std::vector<InputPoint> input(const Fixture& f,bool variant) {
  // Original first eight IDs from full_ball_tower_gate::input; four more
  // distinct IDs extend its non-identity fixture to the D5 shell.
  const std::array<PointId,12> ids{std::numeric_limits<PointId>::max(),17,0,902,2147483648u,3,65536,42,7,1001,19,999};
  need(f.points.size()<=ids.size(),"fixture.ids");std::vector<InputPoint> out;
  for(size_t j=0;j<f.points.size();++j) out.push_back({variant?ids[j]:static_cast<PointId>(j),f.points[j]});
  if(variant) std::reverse(out.begin(),out.end());
  return out;
}
inline std::vector<BallData> catalogue(const Fixture& f,const CloudIndex& ix) {
  need(f.points.size()<32,"fixture.mask_width");const u32 end=u32{1}<<f.points.size();
  struct Row {oracle::Ball ball;unsigned q;};std::map<BallKey,Row> candidates;
  for(u32 mask=1;mask<end;++mask) {const unsigned q=std::popcount(mask);if(q<2 || q>4) continue;
    const auto ball=oracle::detail::support_ball(f.points,mask);if(!ball) continue;
    const auto [it,fresh]=candidates.emplace(key(*ball),Row{*ball,q});if(!fresh) it->second.q=std::min(q,it->second.q);}
  std::vector<BallData> out;
  for(const auto& [ball_key,candidate]:candidates) {
    const auto& ball=candidate.ball;BallData row{};row.key=ball_key;row.level=level(ball.radius2);row.arity=static_cast<u8>(candidate.q);
    for(const auto& point:f.points) {const auto d=distance2(point,ball);if(d>ball.radius2) continue;
      const auto found=std::find(ix.upos.begin(),ix.upos.end(),point);need(found!=ix.upos.end(),"fixture.index");const auto id=static_cast<i32>(found-ix.upos.begin());
      if(d==ball.radius2) {need(row.n_shell<kBallShellMax,"fixture.shell");row.shell_ids[row.n_shell++]=id;}
      else {need(row.n_interior<kBallInteriorMax,"fixture.interior");row.interior_ids[row.n_interior++]=id;}}
    if(row.n_interior+row.arity>std::min<size_t>(f.k+1,f.points.size())) continue;
    std::sort(row.interior_ids,row.interior_ids+row.n_interior);std::sort(row.shell_ids,row.shell_ids+row.n_shell);out.push_back(row);
  }
  return out;
}
inline void distinct_representations(std::vector<BallData>& balls) {
  for(size_t j=0;j<balls.size();++j) {
    const u64 factor=j+2;auto& level=balls[j].level;u128 carry=0;
    for(unsigned word=0;word<3;++word) {const u128 x=u128(level.num[word])*factor+carry;level.num[word]=static_cast<u64>(x);carry=x>>64;}
    need(carry==0 && level.den<((i128{1}<<126)/factor),"fixture.level_scale");level.den*=factor;
  }
  std::reverse(balls.begin(),balls.end()); // BallId is not the key-order rank.
}
}
