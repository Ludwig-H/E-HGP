#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include "cloud/grid32_primitives.hpp"
using namespace mhgp10;
using grid32::PointGrid32;
u64 checks=0,prefix_cases=0,translation_cases=0,reflection_cases=0;
void check(bool yes,const char* message) { ++checks; if(!yes) throw std::runtime_error(message); }
std::string decimal(u128 n) { std::string s; do{s.push_back(char('0'+n%10));n/=10;}while(n); return {s.rbegin(),s.rend()}; }
PointGrid32 translated(PointGrid32 p,const std::array<i64,3>& t) {
 const u32 xyz[3]={p.x,p.y,p.z};u32 q[3]{};
 for(unsigned k=0;k<3;++k) { const i128 value=i128(xyz[k])+i128(t[k]);
   if(value<0 || value>i128(std::numeric_limits<u32>::max())) throw std::runtime_error("invalid test translation");
   q[k]=u32(value); }
 return {q[0],q[1],q[2]};
}
int main() { try {
 constexpr u32 U=std::numeric_limits<u32>::max();
 constexpr PointGrid32 zero{0,0,0},cube{U,U,U};
 static_assert(grid32::squared_distance(zero,cube)==u128{3}*U*U);
 static_assert(grid32::decode_morton96(grid32::kMaxMortonKey)==cube);
 std::vector<PointGrid32> points={{0,0,0},{U,U,U},{U,0,U},{0,U,0},{1,2,3},{0x80000000u,0x40000000u,0x20000000u}};
 // Cross-axis carries near every bit boundary, rather than one-hot points.
 for(unsigned bit=0;bit<32;++bit) {
   const u32 p=u32{1}<<bit;
   points.push_back({p-1,p,~p});points.push_back({~p,p-1,p});points.push_back({p,~p,p-1});
 }
 for(PointGrid32 p:points) {
  const u128 full=grid32::morton96(p);
  for(unsigned k=0;k<=32;++k) {
    const u32 mask=k==32?U:(k==0?0u:(u32{1}<<k)-1);
    const PointGrid32 lower{p.x&mask,p.y&mask,p.z&mask};
    const PointGrid32 upper=k==32?PointGrid32{}:PointGrid32{p.x>>k,p.y>>k,p.z>>k};
    const u128 recomposed=(grid32::morton96(upper)<<(3*k))|grid32::morton96(lower);
    check(recomposed==full,"Morton prefix composition");
    const auto decoded=grid32::decode_morton96(recomposed);
    check(decoded && *decoded==p,"composed decode");++prefix_cases;
  }
 }
 // Negative and mixed signed translations, always checked before conversion.
 const std::array<std::array<i64,3>,5> shifts={{{{-i64(U),-i64(U),-i64(U)}},{{-2147483648LL,2147483647LL,-2147483648LL}},{{-17,31,-63}},{{0,0,0}},{{17,-31,63}}}};
 for(const auto& t:shifts) {
  u32 low[3]{},high[3]{};
  for(unsigned k=0;k<3;++k) { low[k]=u32(t[k]<0?-t[k]:0); high[k]=u32(t[k]>0?i64(U)-t[k]:U); }
  std::array<PointGrid32,8> corners{};
  for(unsigned i=0;i<8;++i) corners[i]={(i&1)?high[0]:low[0],(i&2)?high[1]:low[1],(i&4)?high[2]:low[2]};
  for(PointGrid32 a:corners) for(PointGrid32 b:corners) {
    const auto ta=translated(a,t),tb=translated(b,t);
    check(grid32::squared_distance(a,b)==grid32::squared_distance(ta,tb),"signed common translation");++translation_cases;
  }
 }
 for(PointGrid32 a:points) {
  const PointGrid32 b{a.z,a.x,a.y};
  const PointGrid32 ra{U-a.x,U-a.y,U-a.z},rb{U-b.x,U-b.y,U-b.z};
  check(grid32::squared_distance(a,b)==grid32::squared_distance(ra,rb),"cube reflection");++reflection_cases;
 }
 const PointGrid32 a{1,0,0},b{0,1,0};
 const auto a2=translated(a,{1,0,0}),b2=translated(b,{1,0,0});
 check(grid32::morton96(a)<grid32::morton96(b),"original Morton order");
 check(grid32::morton96(a2)>grid32::morton96(b2),"translated order reversal");
 check(grid32::squared_distance(a,b)==grid32::squared_distance(a2,b2),"order witness geometry");
 std::cout<<"{\"status\":\"ok\",\"checks\":"<<checks<<",\"prefix_cases\":"<<prefix_cases
  <<",\"signed_translation_cases\":"<<translation_cases<<",\"reflection_cases\":"<<reflection_cases
  <<",\"max_distance\":\""<<decimal(grid32::squared_distance(zero,cube))<<"\""
  <<",\"sizeof_point_observed\":"<<sizeof(PointGrid32)<<",\"alignof_point_observed\":"<<alignof(PointGrid32)
  <<",\"sizeof_u128_observed\":"<<sizeof(u128)<<",\"alignof_u128_observed\":"<<alignof(u128)
  <<",\"morton_order_translation_witness\":{\"before_keys\":[1,2],\"after_keys\":[8,3],\"squared_distance_before_after\":2}"
  <<",\"no_product_translation_or_wire_api_claimed\":true}\n";
 return 0;
 } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;} }
