#pragma once
#include <cstddef>
#include <type_traits>
#include "wire.cuh"

namespace mhgp7::gpu_intruder_private {

enum class Status : u32 { ok=0, invalid_key=1, invalid_request=2, invalid_view=3, invariant=4 };
inline constexpr u32 kSelectedCapacity=10;
inline constexpr u32 kStackCapacity=49;  // 48 distinct Morton bits + current leaf.
struct IndexView {
  const i32* left=nullptr;
  const i32* right=nullptr;
  const i32* first=nullptr;
  const i32* last=nullptr;
  const u16* box6=nullptr;
  const u16* positions3=nullptr;
  u64 snapshot=0;
  u32 nodes=0, positions=0;
  i32 root=-1;
};
struct Request {
  u64 key_words[10]{};
  u64 snapshot=0;
  i32 selected[kSelectedCapacity]{};
  u32 selected_count=0;
};
struct Result {
  Status status=Status::invalid_request;
  i32 intruder=-1;
  u64 queries=0, nodes=0, interior_ranges=0, power_tests=0;
  u32 integer_min[3]{};
  u32 stack_peak=0, axis_divisions=0;
};
static_assert(std::is_trivially_copyable_v<IndexView> && std::is_standard_layout_v<IndexView>);
static_assert(std::is_trivially_copyable_v<Request> && std::is_standard_layout_v<Request>);
static_assert(std::is_trivially_copyable_v<Result> && std::is_standard_layout_v<Result>);

MHGP7_HD inline BallKey decode_key(const u64* words) {
  using gpu_ball_key_private::wire_i128;
  return {wire_i128(words), {wire_i128(words+2), wire_i128(words+4), wire_i128(words+6)}, wire_i128(words+8)};
}
MHGP7_HD inline bool key_admitted(const BallKey& key) {
  if (key.a<=0 || (u128)key.a >= ((u128)1<<68) || uabs128(key.c)>=((u128)1<<105)) return false;
  for (int i=0; i<3; ++i) if (uabs128(key.b[i])>=((u128)1<<87)) return false;
  return true;
}
MHGP7_HD inline i128 axis_value(const BallKey& key, int axis, u32 coordinate) {
  const i128 t=coordinate;
  return key.a*t*t+key.b[axis]*t;
}
MHGP7_HD inline bool make_minimizers(const BallKey& key, u32* output, u32* divisions) {
  if (!key_admitted(key)) return false;
  const i128 denominator=2*key.a;
  for (int i=0; i<3; ++i) {
    auto division=gpu_ball_key_private::divide128(-key.b[i], denominator);
    ++*divisions;
    if (division.status!=gpu_ball_key_private::Status::ok) return false;
    i128 q=division.quotient, r=division.remainder;
    if (r<0) { --q; r+=denominator; }
    i128 m=q+(r>key.a ? 1 : 0);
#if defined(MHGP7_INTRUDER_MUTANT) && MHGP7_INTRUDER_MUTANT == 1
    m=q;  // Lose the nearest integer when the upper neighbour is better.
#endif
#if defined(MHGP7_INTRUDER_MUTANT) && MHGP7_INTRUDER_MUTANT == 5
    m=(i64)m;  // Narrow an out-of-domain centre before clipping.
#endif
    output[i]=m<0 ? 0 : (m>65535 ? 65535 : (u32)m);
  }
  return true;
}
MHGP7_HD inline void box_bounds(const BallKey& key, const u32* integer_min,
    const u16* lo, const u16* hi, i128* low, i128* high) {
  *low=key.c; *high=key.c;
  for (int i=0; i<3; ++i) {
    const u32 m=integer_min[i]<lo[i] ? lo[i] : (integer_min[i]>hi[i] ? hi[i] : integer_min[i]);
    *low+=axis_value(key,i,m);
    const i128 a=axis_value(key,i,lo[i]), b=axis_value(key,i,hi[i]);
    *high+=a>b ? a : b;
  }
}
MHGP7_HD inline bool member(const Request& request, i32 index) {
#if defined(MHGP7_INTRUDER_MUTANT) && MHGP7_INTRUDER_MUTANT == 4
  (void)request; (void)index; return false;
#else
  u32 lo=0, hi=request.selected_count;
  while (lo<hi) {
    const u32 mid=lo+(hi-lo)/2;
    if (request.selected[mid]<index) lo=mid+1; else hi=mid;
  }
  return lo<request.selected_count && request.selected[lo]==index;
#endif
}
MHGP7_HD inline bool valid_ref(const IndexView& view, i32 node) {
  return node<0 ? (-1-(i64)node < view.positions) : ((u32)node < view.nodes);
}
// Borrowed view must originate in a validated owner. Cheap query-local checks
// are not a replacement for pointer lifetime, allocation sizes, or topology validation.
MHGP7_HD inline Result find_intruder(const IndexView& view, const Request& request) {
  Result result;
  if (!view.snapshot || !view.positions || view.positions>0x7fffffffu ||
      view.nodes!=view.positions-1 || !view.positions3 ||
      (view.nodes && (!view.left || !view.right || !view.first || !view.last || !view.box6)) ||
      (view.nodes ? view.root!=0 : view.root!=-1)) {
    result.status=Status::invalid_view; return result;
  }
  if (request.snapshot!=view.snapshot || request.selected_count>kSelectedCapacity) return result;
  for (u32 i=0; i<request.selected_count; ++i)
    if (request.selected[i]<0 || (u32)request.selected[i]>=view.positions ||
        (i && request.selected[i-1]>=request.selected[i])) return result;
  const BallKey key=decode_key(request.key_words);
  if (!make_minimizers(key,result.integer_min,&result.axis_divisions)) {
    result.status=Status::invalid_key; return result;
  }
  result.status=Status::ok; result.queries=1;
  i32 stack[kStackCapacity];
  u32 size=1; stack[0]=view.root; result.stack_peak=1;
  while (size) {
    const i32 node=stack[--size];
    ++result.nodes;
    if (!valid_ref(view,node)) { result.status=Status::invariant; return result; }
    const bool leaf=node<0;
    const u32 index=leaf ? (u32)(-1-(i64)node) : (u32)node;
    const u16* lo=leaf ? view.positions3+3ull*index : view.box6+6ull*index;
    const u16* hi=leaf ? lo : lo+3;
    for (int axis=0; axis<3; ++axis)
      if (lo[axis]>hi[axis]) { result.status=Status::invariant; return result; }
    i128 low,high;
    box_bounds(key,result.integer_min,lo,hi,&low,&high);
#if defined(MHGP7_INTRUDER_MUTANT) && MHGP7_INTRUDER_MUTANT == 2
    if (low>0) continue;
    if (high<=0) {
#else
    if (low>=0) continue;
    if (high<0) {
#endif
      ++result.interior_ranges;
      const i32 first=leaf ? (i32)index : view.first[index];
      const i32 last=leaf ? (i32)index : view.last[index];
      if (first<0 || last<first || (u32)last>=view.positions) { result.status=Status::invariant; return result; }
      for (i32 u=first; u<=last; ++u) if (!member(request,u)) { result.intruder=u; return result; }
    } else if (leaf) {
      if (!member(request,(i32)index)) {
        ++result.power_tests;
        i128 power=key.c;
        for (int axis=0; axis<3; ++axis) power+=axis_value(key,axis,lo[axis]);
        if (power<0) { result.intruder=(i32)index; return result; }
      }
    } else {
#if defined(MHGP7_INTRUDER_MUTANT) && MHGP7_INTRUDER_MUTANT == 6
      constexpr u32 capacity=8;
#else
      constexpr u32 capacity=kStackCapacity;
#endif
      if (size+2>capacity) { result.status=Status::invariant; return result; }
#if defined(MHGP7_INTRUDER_MUTANT) && MHGP7_INTRUDER_MUTANT == 3
      stack[size++]=view.left[index]; stack[size++]=view.right[index];
#else
      stack[size++]=view.right[index]; stack[size++]=view.left[index];
#endif
      if (size>result.stack_peak) result.stack_peak=size;
    }
  }
  return result;
}

}  // namespace mhgp7::gpu_intruder_private
