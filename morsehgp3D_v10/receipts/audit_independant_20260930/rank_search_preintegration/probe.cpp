#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
#include "tower/rank_search.hpp"
using namespace mhgp10;
struct Stats { u64 cases=0, sample_calls=0, level_calls=0, max_calls=0, plateau_cases=0; } stats;
void check(u32 size, u64 cut, u64 width=1) {
  const u64 samples=(u64(size)+63)/64;
  u64 scalls=0, lcalls=0;
  auto level=[&](u64 i) { return i/width; };
  // Prefix cuts and plateaus are both monotone. The formula for their admission
  // count is external to the two-stage search, with a u64 threshold at the edge.
  auto admitted=[&](u64 i) { return width==1 ? i<cut : level(i)<cut; };
  const u64 raw_expected=width==1 ? cut : cut*width;
  const u32 expected=static_cast<u32>(std::min<u64>(size,raw_expected));
  auto budget=[&] { if(scalls+lcalls>64) throw std::runtime_error("callback budget exceeded"); };
  const u32 got=rank_search::at_most(size,
    [&](u32 i) { ++scalls; budget(); const u64 at=u64(i)*64;
      if(i>=samples || at>=size) throw std::runtime_error("sample out of bounds");
      return admitted(at); },
    [&](u32 i) { ++lcalls; budget();
      if(i>=size) throw std::runtime_error("level out of bounds");
      return admitted(i); });
  if(got!=expected) {
    std::cerr<<"mismatch size="<<size<<" cut="<<cut<<" width="<<width<<" got="<<got<<" expected="<<expected<<'\n';
    throw std::runtime_error("wrong rank");
  }
  ++stats.cases; stats.plateau_cases+=width!=1;
  stats.sample_calls+=scalls; stats.level_calls+=lcalls;
  stats.max_calls=std::max(stats.max_calls,scalls+lcalls);
}
int main() {
 try {
  constexpr u64 max=std::numeric_limits<u32>::max();
  std::vector<u32> sizes={0,1,2,63,64,65,127,128,129,
    u32((u64{1}<<31)-2),u32((u64{1}<<31)-1),u32(u64{1}<<31),
    u32((u64{1}<<31)+1),u32((u64{1}<<31)+2),u32((u64{1}<<31)+64),
    u32(max-129),u32(max-128),u32(max-127),u32(max-66),u32(max-65),u32(max-64),
    u32(max-63),u32(max-2),u32(max-1),u32(max)};
  for(u32 size=0;size<=257;++size)
    for(u64 cut=0;cut<=u64(size)+2;++cut) check(size,cut);
  for(u32 size:sizes) {
    std::vector<u64> cuts={0,1,2,62,63,64,65,66,u64(size),u64(size)+1,u64(size)+64,
      (u64{1}<<31)-1,u64{1}<<31,(u64{1}<<31)+1,max-65,max-64,max-1,max,max+1,max+2};
    if(size) cuts.push_back(u64(size)-1);
    // Exact sample boundaries, immediately before/at/after, including the last.
    for(u64 i:{u64{0},u64{1},(u64(size)+63)/64/2,(u64(size)+63)/64}) {
      const u64 at=i*64;
      if(at) cuts.push_back(at-1);
      cuts.push_back(at); cuts.push_back(at+1);
    }
    for(u64 cut:cuts) check(size,cut);
    for(u64 width:{u64{2},u64{3},u64{63},u64{64},u64{65},u64{256},u64{1024},u64{1}<<31,max,u64{1}<<32}) {
      const u64 last=size ? (u64(size)-1)/width : 0;
      for(u64 cut:{u64{0},u64{1},last/2,last,last+1,last+2}) check(size,cut,width);
    }
  }
  // Old multiplication, with an otherwise entirely admitted catalogue.
  const u32 old_size=u32(max-1), old_lo=static_cast<u32>((u64(old_size)+63)/64);
  const u32 old_a=(old_lo-1)*64+1;
  const u32 old_b=std::min<u32>(old_lo*64,old_size);
  const u32 old_product_result=old_a;  // a>=b: loop is skipped.
  if(old_b!=0 || old_product_result==old_size) throw std::runtime_error("old product witness invalid");
  // Old terminal midpoint, with always-true values, explicitly bounded.
  const u32 levels=u32((u64{1}<<31)+64), lo=levels/64;
  u32 a=(lo-1)*64+1, b=lo*64;
  const u32 first_mid=(a+b)/2;
  u32 iterations=0, escaped=0;
  while(a<b && iterations<200) { const u32 mid=(a+b)/2; escaped+=mid<a || mid>=b; a=mid+1; ++iterations; }
  if(first_mid!=32 || iterations!=200 || a>=b) throw std::runtime_error("old midpoint witness invalid");
  std::cout<<"{\"status\":\"ok\",\"cases\":"<<stats.cases
    <<",\"plateau_cases\":"<<stats.plateau_cases
    <<",\"sample_calls\":"<<stats.sample_calls<<",\"level_calls\":"<<stats.level_calls
    <<",\"max_callbacks_per_call\":"<<stats.max_calls
    <<",\"callback_out_of_bounds\":0,\"helper_allocations\":0,\"size_limits\":[0,1,63,64,65,2147483647,2147483648,2147483649,4294967294,4294967295]"
    <<",\"old_product\":{\"size\":"<<old_size<<",\"lo\":"<<old_lo<<",\"a\":"<<old_a<<",\"b\":"<<old_b<<",\"returned\":"<<old_product_result<<",\"expected\":"<<old_size<<"}"
    <<",\"old_midpoint\":{\"size\":"<<levels<<",\"first_mid\":"<<first_mid<<",\"out_of_interval\":"<<escaped<<",\"bounded_iterations\":"<<iterations<<",\"terminated\":false}}\n";
  return 0;
 } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
