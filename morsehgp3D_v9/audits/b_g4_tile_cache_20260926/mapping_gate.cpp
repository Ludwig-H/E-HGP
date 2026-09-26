// Independent scheduling model plus the actual public checked-add helper.
// No CUDA execution: device mapping is compared to this model separately.
#include "../../src/gpu/filter_runner.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
using U = std::uint64_t;
__extension__ using Wide = unsigned __int128;
void need(bool value, const char* cause) {
  if (!value) throw std::runtime_error(cause);
}
struct Rectangle { U rows, columns; bool active; };
struct Location { std::size_t rectangle; U row, column; };
struct Totals { U additions=0, configurations=0, pairs=0, representatives=0, bypass=0; };

void additions(Totals& count) {
  constexpr U maximum = std::numeric_limits<U>::max();
  const std::array<U,9> values{0,1,15,16,32,maximum/2,maximum/2+1,maximum-1,maximum};
  for (const U first : values) for (const U second : values) {
    U total=first;
    const bool got=mhgp9::gpu::add_filter_mass(total,second);
    const Wide exact=static_cast<Wide>(first)+second;
    const bool expected=exact<=maximum;
    need(got==expected,"cause=tile_mapping.checked_add_status");
    need(total==(expected ? static_cast<U>(exact) : first),"cause=tile_mapping.checked_add_value");
    ++count.additions;
  }
  U state=0x95d7ba82f361ca19ULL;
  for (unsigned i=0;i<10000;++i) {
    state^=state<<13; state^=state>>7; state^=state<<17;
    const U first=state;
    state^=state<<13; state^=state>>7; state^=state<<17;
    const U second=state;
    U total=first;
    const Wide exact=static_cast<Wide>(first)+second;
    const bool expected=exact<=maximum;
    need(mhgp9::gpu::add_filter_mass(total,second)==expected,"cause=tile_mapping.random_add_status");
    need(total==(expected ? static_cast<U>(exact) : first),"cause=tile_mapping.random_add_value");
    ++count.additions;
  }
  const U side=std::numeric_limits<std::uint32_t>::max()-U{1};
  const U mass=side*side;
  U total=mass;
  need(mhgp9::gpu::add_filter_mass(total,maximum-mass) && total==maximum,
       "cause=tile_mapping.largest_factor_sum");
  need(!mhgp9::gpu::add_filter_mass(total,1) && total==maximum,
       "cause=tile_mapping.largest_factor_overflow");
  count.additions+=2;
}

std::size_t locate(const std::vector<U>& offsets,U position) {
  // Exact binary search used by the kernels, with the terminal prefix
  // excluded from rect_count. Repeated zero-mass offsets are intentional.
  std::size_t low=0,high=offsets.size()-1;
  while (high-low>1) {
    const std::size_t middle=low+(high-low)/2;
    if (offsets[middle]<=position) low=middle;
    else high=middle;
  }
  return low;
}

void mapping(const std::vector<Rectangle>& rectangles,Totals& count) {
  std::vector<U> pairs(1,0),tiles(1,0);
  std::vector<Location> pair_reference,tile_reference;
  for (std::size_t r=0;r<rectangles.size();++r) {
    const auto& x=rectangles[r];
    U pair_total=pairs.back(),tile_total=tiles.back();
    if (x.active) {
      need(mhgp9::gpu::add_filter_mass(pair_total,x.rows*x.columns),"cause=tile_mapping.fixture_mass");
      if (x.columns>=16)
        need(mhgp9::gpu::add_filter_mass(tile_total,x.rows*((x.columns-1)/32+1)),
             "cause=tile_mapping.fixture_tiles");
      for (U row=0;row<x.rows;++row) for (U column=0;column<x.columns;++column) {
        pair_reference.push_back({r,row,column});
        if (x.columns>=16 && column%32==0) tile_reference.push_back({r,row,column});
      }
    }
    pairs.push_back(pair_total); tiles.push_back(tile_total);
  }
  need(pairs.back()==pair_reference.size() && tiles.back()==tile_reference.size(),
       "cause=tile_mapping.reference_size");
  for (U p=0;p<pairs.back();++p) {
    const std::size_t r=locate(pairs,p);
    const auto& x=rectangles[r];
    const U local=p-pairs[r],row=local/x.columns,column=local%x.columns;
    const auto& expected=pair_reference[static_cast<std::size_t>(p)];
    need(expected.rectangle==r && expected.row==row && expected.column==column,
         "cause=tile_mapping.pair_identity");
    ++count.pairs;
    if (x.columns<16) { ++count.bypass; continue; }
    U t=tiles[r]+row*((x.columns-1)/32+1)+column/32;
#ifdef MHGP9_TILE_MAPPING_MUTANT_FLAT
    t=tiles[r]+local/32;  // Wrong: lets a tile cross the end of a row.
#endif
    need(t<tile_reference.size(),"cause=tile_mapping.tile_range");
    const auto& rep=tile_reference[static_cast<std::size_t>(t)];
    need(rep.rectangle==r && rep.row==row && rep.column==32*(column/32),
         "cause=tile_mapping.row_representative");
  }
  for (U t=0;t<tiles.back();++t) {
    const std::size_t r=locate(tiles,t);
    const auto& x=rectangles[r];
    const U tpr=(x.columns-1)/32+1,local=t-tiles[r];
    const auto& expected=tile_reference[static_cast<std::size_t>(t)];
    need(expected.rectangle==r && expected.row==local/tpr && expected.column==(local%tpr)*32,
         "cause=tile_mapping.representative_identity");
    ++count.representatives;
  }
  ++count.configurations;
}
}

int main() {
  try {
    Totals count;
    additions(count);
    for (const U rows : {1,2,3,31,32,33,129})
      for (const U columns : {1,15,16,17,31,32,33,63,64,65,127,128,129}) {
        // Leading, interior and trailing inactive or bypassed rectangles.
        mapping({{3,33,false},{rows,columns,true},{2,65,false},{2,15,true},
                 {rows,columns,true},{4,1,true},{1,32,false}},count);
        mapping({{rows,columns,false},{1,31,false}},count);
      }
    need(count.configurations==182 && count.pairs>300000 && count.representatives>10000,
         "cause=tile_mapping.nonvacuous");
    std::cout<<"{\"status\":\"pass\",\"checked_additions\":"<<count.additions
             <<",\"configurations\":"<<count.configurations<<",\"pairs\":"<<count.pairs
             <<",\"representatives\":"<<count.representatives<<",\"bypassed_pairs\":"<<count.bypass<<"}\n";
    return 0;
  } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
