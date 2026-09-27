// Explicit narrow output-only port of b_q34_filtered_resident_20260927 at af369c44.
// Isolated audit runner. DeviceBuffer/check pattern explicitly adapted from
// src/gpu/filter_runner.cu at 70168cc3b; no old P/E allocation path is called.
#include "device.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <cuda_runtime.h>
#include <cub/device/device_scan.cuh>
#include <cub/device/device_radix_sort.cuh>
#include <chrono>

namespace mhgp9::audit::resident_survivors {
using cw::Rectangle;using cw::Segment;using cw::Factor;using cw::Class;using cw::Band;
using cw::Edge;using cw::View;using cw::Counters;using cw::DeviceRun;using cw::add;
namespace {
using Clock=std::chrono::steady_clock;
void require(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
u64 checked_sum(u64 a,u64 b) {u64 out=0;require(add(a,b,out),"cuda_waves.counter_overflow");return out;}
double elapsed(Clock::time_point t) {return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
void check(cudaError_t e,const char* why) {if (e!=cudaSuccess) throw std::runtime_error(std::string(why)+": "+cudaGetErrorString(e));}
#define CW_CUDA(x) check((x),#x)
struct Ledger {u64 live{},peak{};explicit Ledger(u64 initial):live(initial),peak(initial) {}};
template<class T> struct DeviceBuffer {
  T* p=nullptr;size_t count=0;Ledger* ledger=nullptr;
  explicit DeviceBuffer(Ledger* l=nullptr):ledger(l) {}DeviceBuffer(const DeviceBuffer&)=delete;DeviceBuffer& operator=(const DeviceBuffer&)=delete;
  ~DeviceBuffer() {if (p) {cudaFree(p);if(ledger) ledger->live-=bytes();}}
  void allocate(size_t n) {
    require(!p && count==0 && n<=std::numeric_limits<size_t>::max()/sizeof(T),"cuda_waves.device_size");
    const auto next=ledger?plus(ledger->live,times(n,sizeof(T))):0;
    if(n) CW_CUDA(cudaMalloc(reinterpret_cast<void**>(&p),n*sizeof(T)));count=n;
    if(ledger) {ledger->live=next;ledger->peak=std::max(ledger->peak,next);}
  }
  void copy(const T* src,size_t n) {allocate(n);if(n) CW_CUDA(cudaMemcpy(p,src,n*sizeof(T),cudaMemcpyHostToDevice));}
  void release() {if(p) {CW_CUDA(cudaFree(p));p=nullptr;}if(ledger) ledger->live-=bytes();count=0;}
  void swap(DeviceBuffer& other) {require(ledger==other.ledger,"survivors.buffer_ledger");std::swap(p,other.p);std::swap(count,other.count);}
  u64 bytes() const {return count*sizeof(T);}
};
// Totals are per wave: Q<=INT_MAX and node_count<2^32 bound visits below
// 2^63. Host combines waves with checked u64 additions, never atomic wrap.
enum Count {Queries,Q3,Q4,Rejected3,Rejected4,Visits,Failure,CountSize};
__global__ void filter_kernel(View v,u64 base,u32 count,Edge* slots,u32* flags,unsigned long long* totals) {
  const u64 stride=u64(blockDim.x)*gridDim.x;
  unsigned long long local[6]{};
  for (u64 i=u64(blockIdx.x)*blockDim.x+threadIdx.x;i<count;i+=stride) {
    Edge e{};
    if (!decode(v,base+i,e)) {atomicExch(totals+Failure,1ULL);flags[i]=0;continue;}
    const auto input=e.mask;u64 visits=0;e.mask=point_filter(v,e,visits);
    if (e.mask==gpu::stack_failure) {atomicExch(totals+Failure,1ULL);flags[i]=0;continue;}
    slots[i]=e;flags[i]=e.mask!=0;
    ++local[Queries];local[Q3]+=(input&2U)!=0;local[Q4]+=(input&4U)!=0;
    local[Rejected3]+=(input&2U)!=0 && (e.mask&2U)==0;local[Rejected4]+=(input&4U)!=0 && (e.mask&4U)==0;
    local[Visits]+=visits;
  }
  for (unsigned i=0;i<6;++i) if(local[i]) atomicAdd(totals+i,local[i]);
}
__global__ void scatter_kernel(const Edge* slots,const u32* flags,const u32* positions,u32 count,Edge* compact) {
  for (u64 i=u64(blockIdx.x)*blockDim.x+threadIdx.x;i<count;i+=u64(blockDim.x)*gridDim.x)
    if(flags[i]) compact[positions[i]]=slots[i];
}
void accumulate(Counters& c,const unsigned long long* n) {
  c.queries=checked_sum(c.queries,n[Queries]);c.q3=checked_sum(c.q3,n[Q3]);c.q4=checked_sum(c.q4,n[Q4]);
  c.rejected3=checked_sum(c.rejected3,n[Rejected3]);c.rejected4=checked_sum(c.rejected4,n[Rejected4]);c.visits=checked_sum(c.visits,n[Visits]);++c.waves;
}
} // namespace

namespace {
__global__ void split_survivors(const Edge* edges,u64 count,u64* keys,Payload* values,unsigned long long* control) {
  unsigned long long inversions=0;
  for(u64 i=u64(blockIdx.x)*blockDim.x+threadIdx.x;i<count;i+=u64(blockDim.x)*gridDim.x) {
    const auto e=edges[i];keys[i]=e.ordinal;values[i]={e.a,e.b,e.mask,{0,0,0}};
    if(i) inversions+=edges[i-1].ordinal>=e.ordinal;
    if(e.mask==0 || (e.mask&~6U)!=0) atomicExch(control+2,1ULL);
  }
  if(inversions) atomicAdd(control,inversions);
}
__global__ void validate_order(const u64* keys,u64 count,unsigned long long* control) {
  for(u64 i=u64(blockIdx.x)*blockDim.x+threadIdx.x;i<count;i+=u64(blockDim.x)*gridDim.x)
    if(i && keys[i-1]>=keys[i]) atomicExch(control+1,1ULL);
}
void append_survivors(DeviceBuffer<Edge>& stored,u64& size,const Edge* compact,u64 live,u64 quantum,
                      OutputMemory& memory,OutputTimes& timing) {
  const auto start=Clock::now();const auto needed=plus(size,live);const auto next=next_capacity(stored.count,needed,quantum);
  if(next!=stored.count) {
    DeviceBuffer<Edge> replacement(stored.ledger);replacement.allocate(count_size<Edge>(next));
    memory.edge_peak_bytes=std::max(memory.edge_peak_bytes,times(plus(stored.count,next),sizeof(Edge)));
    if(size) CW_CUDA(cudaMemcpy(replacement.p,stored.p,count_size<Edge>(size)*sizeof(Edge),cudaMemcpyDeviceToDevice));
    memory.growth_copy_bytes=plus(memory.growth_copy_bytes,times(size,sizeof(Edge)));++memory.growths;
    stored.swap(replacement);replacement.release();
  }
  if(live) CW_CUDA(cudaMemcpy(stored.p+size,compact,count_size<Edge>(live)*sizeof(Edge),cudaMemcpyDeviceToDevice));
  size=needed;memory.size=size;memory.capacity=stored.count;
  memory.append_copy_bytes=plus(memory.append_copy_bytes,times(live,sizeof(Edge)));timing.append_ms+=elapsed(start);
}
void finish_survivors(DeviceBuffer<Edge>& stored,u64 size,bool debug,cudaDeviceProp prop,OrderedRun& result) {
  static_assert(sizeof(cub::detail::choose_offset_t<unsigned long long>)==8,"64-bit CUB offset required");
  if(!size) {stored.release();return;}
  auto* ledger=stored.ledger;
  DeviceBuffer<u64> key_a(ledger),key_b(ledger);DeviceBuffer<Payload> value_a(ledger),value_b(ledger);
  DeviceBuffer<unsigned long long> control(ledger);DeviceBuffer<unsigned char> scratch(ledger);
  const auto split=Clock::now();key_a.allocate(count_size<u64>(size));key_b.allocate(count_size<u64>(size));
  value_a.allocate(count_size<Payload>(size));value_b.allocate(count_size<Payload>(size));control.allocate(3);
  CW_CUDA(cudaMemset(control.p,0,3*sizeof(unsigned long long)));
  const unsigned blocks=static_cast<unsigned>(std::min<u64>((size+127)/128,u64(prop.multiProcessorCount)*64));
  split_survivors<<<blocks,128>>>(stored.p,size,key_a.p,value_a.p,control.p);CW_CUDA(cudaGetLastError());
  CW_CUDA(cudaDeviceSynchronize());stored.release();result.output_times.split_ms=elapsed(split);
  cub::DoubleBuffer<u64> keys(key_a.p,key_b.p);cub::DoubleBuffer<Payload> values(value_a.p,value_b.p);
  const auto sort=Clock::now();size_t bytes=0;
  // Explicit NumItemsT=64 bits, verified against CUDA12.9/CUB2.8.2 headers.
  // No conversion of global S to int; only the unchanged per-wave scan is int.
  CW_CUDA((cub::DeviceRadixSort::SortPairs<u64,Payload,unsigned long long>(nullptr,bytes,keys,values,size,0,64)));
  scratch.allocate(std::max<size_t>(bytes,1));result.memory.sort_scratch_bytes=scratch.bytes();
  CW_CUDA((cub::DeviceRadixSort::SortPairs<u64,Payload,unsigned long long>(scratch.p,bytes,keys,values,size,0,64)));
  CW_CUDA(cudaDeviceSynchronize());result.output_times.sort_ms=elapsed(sort);
  const auto validation=Clock::now();validate_order<<<blocks,128>>>(keys.Current(),size,control.p);
  CW_CUDA(cudaGetLastError());unsigned long long counters[3]{};
  CW_CUDA(cudaMemcpy(counters,control.p,sizeof(counters),cudaMemcpyDeviceToHost));
  require(counters[1]==0,"survivors.duplicate_or_unsorted");require(counters[2]==0,"survivors.invalid_payload");
  result.counters.out_of_order=counters[0];result.output_times.validate_ms=elapsed(validation);
  const auto download=Clock::now();result.survivors.resize(count_size<Payload>(size));
  CW_CUDA(cudaMemcpy(result.survivors.data(),values.Current(),count_size<Payload>(size)*sizeof(Payload),cudaMemcpyDeviceToHost));
  if(debug) {result.debug_keys.resize(count_size<u64>(size));
    CW_CUDA(cudaMemcpy(result.debug_keys.data(),keys.Current(),count_size<u64>(size)*sizeof(u64),cudaMemcpyDeviceToHost));}
  result.memory.host_payload_bytes=times(result.survivors.capacity(),sizeof(Payload));
  result.memory.debug_key_bytes=times(result.debug_keys.capacity(),sizeof(u64));
  result.timing.download_bytes=plus(result.timing.download_bytes,plus(sizeof(counters),plus(times(size,sizeof(Payload)),debug?times(size,sizeof(u64)):0)));
  result.output_times.download_ms=elapsed(download);result.timing.download_ms+=result.output_times.download_ms;
  result.timing.order_ms=result.output_times.split_ms+result.output_times.sort_ms+result.output_times.validate_ms;
  const auto release=Clock::now();key_a.release();key_b.release();value_a.release();value_b.release();control.release();scratch.release();
  result.output_times.release_ms=elapsed(release);result.timing.release_ms+=result.output_times.release_ms;
}
} // namespace

namespace {
__global__ void rectangles_kernel(const gpu::FlatNode* nodes,const RectQuery* queries,u32 count,unsigned k,u8* masks,unsigned long long* totals) {
  unsigned long long local=0;
  for(u64 i=u64(blockIdx.x)*blockDim.x+threadIdx.x;i<count;i+=u64(blockDim.x)*gridDim.x) {
    const auto q=queries[i];u64 visits=0;
    const auto mask=gpu::filter_boxes(nodes,nodes[q.a].box,nodes[q.b].box,k,q.mask,visits);
    local+=visits;
    if(mask==gpu::stack_failure) {atomicExch(totals+1,1ULL);masks[i]=0;} else masks[i]=mask;
  }
  if(local) atomicAdd(totals,local);
}
class ResidentCuda final:public Device {
 public:
  ResidentCuda(IndexInput input,cudaDeviceProp prop):input_(input),prop_(prop),name_(prop.name) {
    require(input.points_count<=std::numeric_limits<size_t>::max()/3,"resident.points_size");
    nodes_.copy(input.nodes,static_cast<size_t>(input.nodes_count));
    points_.copy(input.points,static_cast<size_t>(3*input.points_count));CW_CUDA(cudaDeviceSynchronize());
  }
  RectPass rectangles(const RectQuery* queries,u32 count) override {
    require(!closed_ && !failed_ && count>0 && count<=0x7fffffffU,"resident.rectangle_count");
    RectPass out;DeviceBuffer<RectQuery> query;DeviceBuffer<u8> masks;DeviceBuffer<unsigned long long> totals;
    const auto upload=Clock::now();query.copy(queries,count);masks.allocate(count);totals.allocate(2);
    out.upload_bytes=query.bytes();out.download_bytes=checked_sum(count,2*sizeof(unsigned long long));
    out.device_bytes=checked_sum(resident_bytes(),checked_sum(query.bytes(),checked_sum(masks.bytes(),totals.bytes())));
    CW_CUDA(cudaMemset(totals.p,0,2*sizeof(unsigned long long)));CW_CUDA(cudaDeviceSynchronize());out.upload_ms=elapsed(upload);
    const unsigned blocks=static_cast<unsigned>(std::min<u64>((u64(count)+127)/128,u64(prop_.multiProcessorCount)*64));
    const auto kernel=Clock::now();
    rectangles_kernel<<<blocks,128>>>(nodes_.p,query.p,count,input_.k,masks.p,totals.p);
    CW_CUDA(cudaGetLastError());CW_CUDA(cudaDeviceSynchronize());out.kernel_ms=elapsed(kernel);
    const auto download=Clock::now();unsigned long long counters[2]{};
    CW_CUDA(cudaMemcpy(counters,totals.p,sizeof(counters),cudaMemcpyDeviceToHost));
    require(counters[1]==0,"resident.rectangle_stack_failure");out.visits=counters[0];out.masks.resize(count);
    CW_CUDA(cudaMemcpy(out.masks.data(),masks.p,count,cudaMemcpyDeviceToHost));out.download_ms=elapsed(download);
    const auto release=Clock::now();query.release();masks.release();totals.release();out.release_ms=elapsed(release);return out;
  }
  OrderedRun consume(View,size_t,bool) override;
  double close() override {
    if(closed_) return 0;
    const auto start=Clock::now();nodes_.release();points_.release();closed_=true;return elapsed(start);
  }
  u64 resident_bytes() const {return checked_sum(nodes_.bytes(),points_.bytes());}
 private:
  IndexInput input_;cudaDeviceProp prop_;std::string name_;
  DeviceBuffer<gpu::FlatNode> nodes_;DeviceBuffer<std::int32_t> points_;bool closed_{},failed_{};
};
}
Opened open_cuda(IndexInput input) {
  Opened out;const auto init=Clock::now();int devices=0;CW_CUDA(cudaGetDeviceCount(&devices));
  require(devices>0,"resident.no_device");CW_CUDA(cudaSetDevice(0));
  cudaDeviceProp prop{};CW_CUDA(cudaGetDeviceProperties(&prop,0));require(prop.multiProcessorCount>0,"resident.no_multiprocessors");
  CW_CUDA(cudaFree(nullptr));out.init_ms=elapsed(init);
  const auto upload=Clock::now();auto context=std::make_unique<ResidentCuda>(input,prop);
  out.resident_bytes=context->resident_bytes();out.upload_ms=elapsed(upload);
  out.name=prop.name;out.cuda=true;out.device=std::move(context);return out;
}

OrderedRun ResidentCuda::consume(View host,size_t requested_q,bool debug_keys) {
  OrderedRun result;const auto total_start=Clock::now();
  try {
    require(!closed_ && !failed_ && requested_q>0,"resident.closed_failed_or_zero_capacity");
    result.available=true;result.device=name_;
    host.nodes_count=input_.nodes_count;host.points_count=input_.points_count;host.k=input_.k;
    View device=host;
    // All counts remain u64 globally. Only this wave's scan uses CUB int.
    const auto q=static_cast<size_t>(std::min<u64>(host.effective,std::min<u64>(requested_q,0x7fffffffU)));
    result.timing.q_actual=q;
    Ledger ledger(resident_bytes());DeviceBuffer<Rectangle> rectangles(&ledger);DeviceBuffer<Segment> segments(&ledger);DeviceBuffer<Factor> factors(&ledger);
    DeviceBuffer<Class> classes(&ledger);DeviceBuffer<Band> bands(&ledger);DeviceBuffer<u32> ranks(&ledger);
    DeviceBuffer<u8> credits(&ledger);
    DeviceBuffer<Edge> slots(&ledger),compact(&ledger);DeviceBuffer<u32> flags(&ledger),positions(&ledger);
    DeviceBuffer<unsigned long long> totals(&ledger);DeviceBuffer<unsigned char> scratch(&ledger);
    const auto upload_start=Clock::now();
#define COPY(field,count,buffer) buffer.copy(host.field,static_cast<size_t>(host.count));device.field=buffer.p;result.timing.upload_bytes=checked_sum(result.timing.upload_bytes,buffer.bytes())
    COPY(rectangles,rectangle_count,rectangles);COPY(segments,segment_count,segments);
    COPY(factors,factor_count,factors);COPY(classes,class_count,classes);COPY(bands,band_count,bands);
    COPY(ranks,rank_count,ranks);COPY(credits,credit_count,credits);
#undef COPY
    device.nodes=nodes_.p;device.points=points_.p;
    slots.allocate(q);compact.allocate(q);flags.allocate(q);positions.allocate(q);totals.allocate(CountSize);
    size_t scratch_bytes=0;
    if(q) {CW_CUDA(cub::DeviceScan::ExclusiveSum(nullptr,scratch_bytes,flags.p,positions.p,static_cast<int>(q)));scratch.allocate(scratch_bytes);}
    CW_CUDA(cudaDeviceSynchronize());result.timing.upload_ms=elapsed(upload_start);
    result.timing.device_bytes=checked_sum(result.timing.upload_bytes,checked_sum(slots.bytes(),checked_sum(compact.bytes(),
      checked_sum(flags.bytes(),checked_sum(positions.bytes(),checked_sum(totals.bytes(),scratch.bytes()))))));
    result.timing.device_bytes=checked_sum(result.timing.device_bytes,checked_sum(nodes_.bytes(),points_.bytes()));
    Counters counts;DeviceBuffer<Edge> survivors(&ledger);u64 survivor_size=0;
    for(u64 base=0;base<host.effective;) {
      const auto count=static_cast<u32>(std::min<u64>(q,host.effective-base));
      const auto wave_start=Clock::now();CW_CUDA(cudaMemset(totals.p,0,CountSize*sizeof(unsigned long long)));
      constexpr unsigned threads=128;
      const auto blocks=static_cast<unsigned>(std::min<u64>((u64(count)+threads-1)/threads,static_cast<u64>(prop_.multiProcessorCount)*64));
      filter_kernel<<<blocks,threads>>>(device,base,count,slots.p,flags.p,totals.p);CW_CUDA(cudaGetLastError());
      CW_CUDA(cub::DeviceScan::ExclusiveSum(scratch.p,scratch_bytes,flags.p,positions.p,static_cast<int>(count)));
      unsigned long long wave_counts[CountSize]{};u32 tail[2]{};
      CW_CUDA(cudaMemcpy(wave_counts,totals.p,sizeof(wave_counts),cudaMemcpyDeviceToHost));
      CW_CUDA(cudaMemcpy(tail,positions.p+count-1,sizeof(u32),cudaMemcpyDeviceToHost));
      CW_CUDA(cudaMemcpy(tail+1,flags.p+count-1,sizeof(u32),cudaMemcpyDeviceToHost));
      require(wave_counts[Failure]==0,"cuda_waves.device_decode_or_stack_failure");
      const u64 live=u64(tail[0])+tail[1];require(live<=count,"cuda_waves.compaction_count");
      scatter_kernel<<<blocks,threads>>>(slots.p,flags.p,positions.p,count,compact.p);CW_CUDA(cudaGetLastError());
      CW_CUDA(cudaDeviceSynchronize());result.timing.waves_ms+=elapsed(wave_start);
      append_survivors(survivors,survivor_size,compact.p,live,std::max<size_t>(q,1),result.memory,result.output_times);
      result.timing.download_bytes=checked_sum(result.timing.download_bytes,sizeof(wave_counts)+sizeof(tail));
      accumulate(counts,wave_counts);base+=count;
    }
    const auto release_start=Clock::now();
    // Explicit release is inside total_ms; RAII still handles every failure.
    rectangles.release();segments.release();factors.release();classes.release();bands.release();ranks.release();credits.release();
    slots.release();compact.release();flags.release();positions.release();totals.release();scratch.release();
    result.timing.release_ms=elapsed(release_start);
    result.counters=counts;finish_survivors(survivors,survivor_size,debug_keys,prop_,result);
    result.memory.array_peak_bytes=ledger.peak;result.timing.device_bytes=ledger.peak;
    require(ledger.live==resident_bytes(),"survivors.unreleased_device_buffer");
  } catch(const std::exception& e) {
    failed_=true;result.error=e.what();result.survivors.clear();result.debug_keys.clear();result.counters={};
  }
  result.timing.total_ms=elapsed(total_start);return result;
}
} // namespace mhgp9::audit::resident_survivors
