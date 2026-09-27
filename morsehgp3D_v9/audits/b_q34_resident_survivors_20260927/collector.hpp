#pragma once
// Output-only audit seam. No geometric predicate or candidate array lives here.
#include "../b_q34_cuda_waves_20260927/cuda_api.hpp"
#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
namespace mhgp9::audit::resident_survivors {
namespace cw=cuda_waves;
using cw::u64;using cw::u32;using cw::u8;
inline void require_output(bool value,const char* reason) {if(!value) throw std::runtime_error(reason);}
inline u64 plus(u64 a,u64 b) {u64 out=0;require_output(cw::add(a,b,out),"survivors.counter_overflow");return out;}
inline u64 times(u64 a,u64 b) {u64 out=0;require_output(cw::multiply(a,b,out),"survivors.byte_overflow");return out;}
template<class T> inline size_t count_size(u64 n) {
  require_output(n<=std::numeric_limits<size_t>::max()/sizeof(T),"survivors.allocation_size");return static_cast<size_t>(n);
}
struct Payload {u32 a,b;u8 mask;u8 reserved[3];};
static_assert(sizeof(Payload)==12 && alignof(Payload)==4 && offsetof(Payload,a)==0 && offsetof(Payload,b)==4 && offsetof(Payload,mask)==8);
static_assert(std::is_standard_layout_v<Payload> && std::is_trivially_copyable_v<Payload> && sizeof(cw::Edge)==24);
struct OutputMemory {
  u64 size{},capacity{},growths{},growth_copy_bytes{},append_copy_bytes{},edge_peak_bytes{},
    array_peak_bytes{},sort_scratch_bytes{},host_payload_bytes{},debug_key_bytes{},host_conversion_peak_bytes{};
};
struct OutputTimes {double append_ms{},split_ms{},sort_ms{},validate_ms{},download_ms{},release_ms{};};
struct OrderedRun {
  bool available=false;std::string error,device;std::vector<Payload> survivors;
  std::vector<u64> debug_keys;cw::Counters counters;cw::Timing timing;OutputMemory memory;OutputTimes output_times;
};
// Exact capacity decision shared by CPU/GPU. No global INT_MAX ceiling.
// If a positive growth occurs, old+new coexist until the copy has finished.
inline u64 next_capacity(u64 capacity,u64 needed,u64 quantum) {
  require_output(quantum>0,"survivors.zero_quantum");
  if(needed<=capacity) return capacity;
  const auto limit=static_cast<u64>(std::numeric_limits<size_t>::max()/sizeof(cw::Edge));
  require_output(needed<=limit,"survivors.allocation_size");
  const u64 doubled=capacity>limit/2?limit:2*capacity;
  return std::max(needed,std::max(std::min(quantum,limit),doubled));
}
// This collector proves append/ownership/size independently of CUDA. Its sort
// is a CPU reference, not a claim about the eventual device primitive.
class PortableCollector final {
 public:
  explicit PortableCollector(u64 quantum):quantum_(quantum) {require_output(quantum>0,"survivors.zero_quantum");}
  PortableCollector(const PortableCollector&)=delete;PortableCollector& operator=(const PortableCollector&)=delete;
  void append(const cw::Edge* input,u64 count,bool inject_allocation_failure=false) {
    require_output(!finished_ && !failed_,"survivors.collector_closed");
    try {
      require_output(count==0 || input,"survivors.null_input");const auto needed=plus(size_,count);
      const auto next=next_capacity(capacity_,needed,quantum_);
      for(u64 i=0;i<count;++i) require_output(input[i].mask!=0 && (input[i].mask&~6U)==0,"survivors.invalid_payload");
      if(next!=capacity_) {
        if(inject_allocation_failure) throw std::bad_alloc();
        auto replacement=std::unique_ptr<cw::Edge[]>(new cw::Edge[count_size<cw::Edge>(next)]);
        memory_.edge_peak_bytes=std::max(memory_.edge_peak_bytes,times(plus(capacity_,next),sizeof(cw::Edge)));
        if(size_) std::copy_n(data_.get(),count_size<cw::Edge>(size_),replacement.get());
        memory_.growth_copy_bytes=plus(memory_.growth_copy_bytes,times(size_,sizeof(cw::Edge)));
        data_=std::move(replacement);capacity_=next;++memory_.growths;
      }
      if(count) std::copy_n(input,count_size<cw::Edge>(count),data_.get()+size_);
      size_=needed;memory_.size=size_;memory_.capacity=capacity_;
      memory_.append_copy_bytes=plus(memory_.append_copy_bytes,times(count,sizeof(cw::Edge)));
    } catch(...) {failed_=true;throw;}
  }
  void finish(OrderedRun& output,bool debug) {
    require_output(!finished_ && !failed_,"survivors.collector_closed");
    try {
      u64 inversions=0;for(u64 i=1;i<size_;++i) inversions+=data_[i-1].ordinal>=data_[i].ordinal;
      if(size_) std::sort(data_.get(),data_.get()+size_,[](const auto& a,const auto& b){
#ifdef MHGP9_SURVIVORS_MUTANT_LOW32
        return static_cast<u32>(a.ordinal)<static_cast<u32>(b.ordinal);
#else
        return a.ordinal<b.ordinal;
#endif
      });
      for(u64 i=1;i<size_;++i) require_output(data_[i-1].ordinal<data_[i].ordinal,"survivors.duplicate_or_unsorted");
      std::vector<Payload> payload;std::vector<u64> keys;payload.resize(count_size<Payload>(size_));
      if(debug) keys.resize(count_size<u64>(size_));
      for(u64 i=0;i<size_;++i) {
        const auto& e=data_[i];payload[i]={e.a,e.b,e.mask,{0,0,0}};
#ifdef MHGP9_SURVIVORS_MUTANT_PAYLOAD
        if(size_>1) payload[i].a=data_[(i+1)%size_].a;
#endif
        if(debug) keys[i]=e.ordinal;
      }
      memory_.host_payload_bytes=times(payload.capacity(),sizeof(Payload));memory_.debug_key_bytes=times(keys.capacity(),sizeof(u64));
      memory_.array_peak_bytes=std::max(memory_.edge_peak_bytes,plus(times(capacity_,sizeof(cw::Edge)),
        plus(memory_.host_payload_bytes,memory_.debug_key_bytes)));
      const auto all_inversions=plus(output.counters.out_of_order,inversions);
      data_.reset();finished_=true;output.survivors=std::move(payload);output.debug_keys=std::move(keys);output.memory=memory_;
      output.counters.out_of_order=all_inversions;
    } catch(...) {failed_=true;throw;}
  }
  u64 size() const {return size_;}u64 capacity() const {return capacity_;}
  const OutputMemory& memory() const {return memory_;}
 private:
  std::unique_ptr<cw::Edge[]> data_;u64 quantum_,size_{},capacity_{};OutputMemory memory_;bool finished_{},failed_{};
};
}
