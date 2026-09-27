#pragma once
// Audit-only wire decoder, explicitly ported from waves.hpp at 70168cc3b.
// The witness predicate is INCLUDED unchanged, not reimplemented.
#include "gpu/witness_filter.hpp"

namespace mhgp9::audit::cuda_waves {
using u64=std::uint64_t;using u32=std::uint32_t;using u8=std::uint8_t;
inline constexpr u64 absent=~u64{0};
struct Rectangle {u64 raw_base,arena;u32 a_first,b_first,a_size,b_size;u8 mask;};
struct Segment {u64 rectangle,band,end;};
struct Factor {u64 original_first,first,class_first;u32 size;std::uint16_t classes;};
struct Class {u32 first,last;u8 credit;};
struct Band {u32 a_class,b_first,b_last;};
struct Edge {u64 ordinal;u32 a,b;u8 mask;};
struct View {
  const Rectangle* rectangles{};const Segment* segments{};
  const Factor* factors{};const Class* classes{};const Band* bands{};
  const u32* ranks{};const u8* credits{};
  const gpu::FlatNode* nodes{};const std::int32_t* points{};
  u64 rectangle_count{},segment_count{},factor_count{},class_count{},band_count{},rank_count{},credit_count{};
  u64 nodes_count{},points_count{},effective{};unsigned k{};
};
MHGP9_HD inline bool add(u64 a,u64 b,u64& out) {if (b>absent-a) return false;out=a+b;return true;}
MHGP9_HD inline bool multiply(u64 a,u64 b,u64& out) {if (a!=0 && b>absent/a) return false;out=a*b;return true;}
MHGP9_HD inline bool range(u64 first,u64 size,u64 count) {return first<=count && size<=count-first;}

// Returns false on an invalid view/index/overflow, never a surviving mask.
// Borrow only from a private certified Snapshot, or its exact device copy.
MHGP9_HD inline bool decode(const View& v,u64 position,Edge& out) {
  if (position>=v.effective || v.segment_count==0 || v.k<1 || v.k>10) return false;
  u64 low=0,high=v.segment_count;
  while (low<high) {const u64 mid=low+(high-low)/2;if (v.segments[mid].end<=position) low=mid+1;else high=mid;}
  if (low==v.segment_count) return false;
  const auto& segment=v.segments[low];const u64 begin=low==0?0:v.segments[low-1].end;
  if (begin>position || segment.rectangle>=v.rectangle_count) return false;
  const auto& r=v.rectangles[segment.rectangle];const u64 offset=position-begin;
  if (r.mask==0 || (r.mask&~6U)!=0 || !range(r.a_first,r.a_size,v.points_count) ||
      !range(r.b_first,r.b_size,v.points_count)) return false;
  u64 a=0,b=0;unsigned mask=r.mask;
  if (segment.band==absent) {
    u64 mass=0;if (r.b_size==0 || !multiply(r.a_size,r.b_size,mass) || offset>=mass) return false;
    a=u64(r.a_first)+offset/r.b_size;b=u64(r.b_first)+offset%r.b_size;
  } else {
    if (r.arena==absent || r.arena>=v.factor_count/2 || segment.band>=v.band_count) return false;
    const auto& af=v.factors[2*r.arena];const auto& bf=v.factors[2*r.arena+1];
    const auto& band=v.bands[segment.band];
    if (band.a_class>=af.classes || !range(af.class_first,af.classes,v.class_count) ||
        !range(af.first,af.size,v.rank_count) || !range(bf.first,bf.size,v.rank_count) ||
        !range(bf.first,bf.size,v.credit_count) || band.b_first>=band.b_last || band.b_last>bf.size) return false;
    const auto& ac=v.classes[af.class_first+band.a_class];
    if (ac.first>=ac.last || ac.last>af.size) return false;
    const u64 width=band.b_last-band.b_first;u64 mass=0;
    if (!multiply(ac.last-ac.first,width,mass) || offset>=mass) return false;
    const u64 ag=ac.first+offset/width,bg=band.b_first+offset%width;
    if (!add(af.original_first,v.ranks[af.first+ag],a) || !add(bf.original_first,v.ranks[bf.first+bg],b)) return false;
    const unsigned bc=v.credits[bf.first+bg];mask=0;
    if ((r.mask&2U)!=0 && v.k>=2 && unsigned(ac.credit>>4U)+(bc>>4U)<v.k-1) mask|=2;
    if ((r.mask&4U)!=0 && v.k>=3 && unsigned(ac.credit&15U)+(bc&15U)<v.k-2) mask|=4;
#ifdef MHGP9_AUDIT_CUDA_WAVES_MUTANT_MASK6
    mask=6;
#endif
  }
  if (a<r.a_first || a-r.a_first>=r.a_size || b<r.b_first || b-r.b_first>=r.b_size ||
      a>=v.points_count || b>=v.points_count || a==b || mask==0) return false;
  u64 ordinal=0,base=0;
  if (!multiply(a-r.a_first,r.b_size,base) || !add(base,b-r.b_first,base) || !add(r.raw_base,base,ordinal)) return false;
#ifdef MHGP9_AUDIT_CUDA_WAVES_MUTANT_ORDINAL
  ordinal=position;
#endif
  out={ordinal,static_cast<u32>(a),static_cast<u32>(b),static_cast<u8>(mask)};return true;
}
MHGP9_HD inline u8 point_filter(const View& v,const Edge& e,u64& visits) {
  gpu::FlatBox a{},b{};
  for (unsigned d=0;d!=3;++d) {
    a.low[d]=a.high[d]=v.points[3*u64(e.a)+d];b.low[d]=b.high[d]=v.points[3*u64(e.b)+d];
  }
  return gpu::filter<true>(v.nodes,a,b,v.k,e.mask,visits);
}
} // namespace mhgp9::audit::cuda_waves
