#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>

namespace {
using U32=std::uint32_t;
using U64=std::uint64_t;
__extension__ using U128=unsigned __int128;
constexpr U64 max32=std::numeric_limits<U32>::max();
constexpr U64 max64=std::numeric_limits<U64>::max();
struct Span32 { U32 first,count; };
struct Span64 { U64 first; U32 count; };
// Pure offsets have no NodeIdx sentinel interpretation. Last address UINT_MAX
// may be represented; this is separate from the product's reserved-ID rules.
std::optional<Span32> checked_span32(U64 first,U64 count) {
  if(first>max32 || count>max32) return std::nullopt;
  if(count && count-1>max32-first) return std::nullopt;
  return Span32{U32(first),U32(count)};
}
std::optional<Span64> checked_span64(U64 first,U64 count,U64 storage_extent) {
  if(count>max32 || first>storage_extent || count>storage_extent-first) return std::nullopt;
  return Span64{first,U32(count)};
}
std::optional<U64> checked_index(Span64 span,U64 relative,U64 storage_extent) {
  if(span.first>storage_extent || U64(span.count)>storage_extent-span.first || relative>=span.count)
    return std::nullopt;
  return span.first+relative;
}
std::optional<U64> append_end(U64 first,U64 count) {
  if(count>max64-first) return std::nullopt;
  return first+count;
}
U64 checks=0;
void require(bool ok,const char* reason) { ++checks; if(!ok) throw std::runtime_error(reason); }
}
int main() {
  try {
    std::array<U64,9> starts={0,1,max32-1,max32,max32+1,max32+185032613,max64-20000,max64-1,max64};
    std::array<U64,9> lengths={0,1,2,58,74,92,20000,max32,max32+1};
    for(U64 first:starts) for(U64 count:lengths) {
      auto narrow=checked_span32(first,count);
      const bool expect32=first<=max32 && count<=max32 && (!count || U128(first)+count-1<=max32);
      require(bool(narrow)==expect32,"32-bit append domain mismatch");
      auto end=append_end(first,count);
      require(bool(end)==(U128(first)+count<=max64),"64-bit append overflow mismatch");
      if(end) {
        auto wide=checked_span64(first,count,*end);
        require(bool(wide)==(count<=max32),"checked wide span mismatch");
        if(wide) {
          require(!checked_index(*wide,count,*end),"one-past relative index admitted");
          if(count) {
            auto last=checked_index(*wide,count-1,*end);
            require(last && *last==*end-1,"last wide address mismatch");
            require(!checked_index(*wide,count-1,*end-1),"span accepted truncated storage");
          }
        }
      }
    }
    // Ball-major flattening, matching the real loop rather than grouping by K.
    constexpr U64 balls=20000000, reps_per_ball=58+74+92;
    std::array<U64,3> perK={balls*58,balls*74,balls*92};
    const U64 total=balls*reps_per_ball, cells=balls*10;
    for(U64 count:perK) require(count<max32,"model must pass per-K sr guard");
    require(cells<max32 && balls*3<=cells,"model must pass atlas/ExtCell cardinal guards");
    require(8192ULL*20000<max32,"source's per-chunk upper bound must fit u32");
    const U64 first=(balls-1)*reps_per_ball+58+74, count=92, relative=91;
    const U32 old_first=static_cast<U32>(first), old_index=old_first+static_cast<U32>(relative);
    require(U64(old_index)!=first+relative,"legacy global offset alias absent");
    require(!checked_span32(first,count),"narrowing must be rejected before insertion");
    auto wide=checked_span64(first,count,total);
    require(wide && checked_index(*wide,relative,total)==std::optional<U64>{total-1},
            "checked u64 span did not address last representative");
    // Addition also fails before the starting offset itself overflows.
    const U64 cross_first=max32-9,cross_count=20,cross_extent=cross_first+cross_count;
    const U32 cross_old=U32(cross_first)+U32(cross_count-1);
    require(cross_old==9 && !checked_span32(cross_first,cross_count),"relative-add wrap witness absent");
    auto cross=checked_span64(cross_first,cross_count,cross_extent);
    require(cross && checked_index(*cross,cross_count-1,cross_extent)==std::optional<U64>{max32+10},
            "u64 addition repair failed");
    std::cout<<"{\"status\":\"PASS\",\"checks\":"<<checks
             <<",\"abstract_model\":{\"balls\":"<<balls<<",\"atlas_cells\":"<<cells
             <<",\"ext_cells\":"<<cells<<",\"ext_join_cells\":"<<balls*3
             <<",\"perK_representatives\":["<<perK[0]<<','<<perK[1]<<','<<perK[2]
             <<"],\"global_representatives\":"<<total<<",\"last_span_first\":"<<first
             <<",\"old_first\":"<<old_first<<",\"old_last_index\":"<<old_index
             <<",\"exact_last_index\":"<<total-1<<"},\"add_wrap_old\":"<<cross_old
             <<",\"actual_geometry_realized\":false,\"product_tower_executed\":false,\"GCP\":false}\n";
  } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
