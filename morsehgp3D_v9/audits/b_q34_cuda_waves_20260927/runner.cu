// Isolated audit runner. DeviceBuffer/check pattern explicitly adapted from
// src/gpu/filter_runner.cu at 70168cc3b; no old P/E allocation path is called.
#include "cuda_api.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <cuda_runtime.h>
#include <cub/device/device_scan.cuh>
#include <chrono>

namespace mhgp9::audit::cuda_waves {
namespace {
using Clock=std::chrono::steady_clock;
void require(bool ok,const char* why) {if(!ok) throw std::runtime_error(why);}
u64 checked_sum(u64 a,u64 b) {u64 out=0;require(add(a,b,out),"cuda_waves.counter_overflow");return out;}
double elapsed(Clock::time_point t) {return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
void check(cudaError_t e,const char* why) {if (e!=cudaSuccess) throw std::runtime_error(std::string(why)+": "+cudaGetErrorString(e));}
#define CW_CUDA(x) check((x),#x)
template<class T> struct DeviceBuffer {
  T* p=nullptr;size_t count=0;
  DeviceBuffer()=default;DeviceBuffer(const DeviceBuffer&)=delete;DeviceBuffer& operator=(const DeviceBuffer&)=delete;
  ~DeviceBuffer() {if (p) cudaFree(p);}
  void allocate(size_t n) {require(n<=std::numeric_limits<size_t>::max()/sizeof(T),"cuda_waves.device_size");count=n;if(n) CW_CUDA(cudaMalloc(reinterpret_cast<void**>(&p),n*sizeof(T)));}
  void copy(const T* src,size_t n) {allocate(n);if(n) CW_CUDA(cudaMemcpy(p,src,n*sizeof(T),cudaMemcpyHostToDevice));}
  void release() {if(p) {CW_CUDA(cudaFree(p));p=nullptr;}count=0;}
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

DeviceRun execute_cuda(View host,size_t requested_q) {
  DeviceRun result;const auto total_start=Clock::now();
  try {
    require(requested_q>0,"cuda_waves.zero_capacity");
    int devices=0;CW_CUDA(cudaGetDeviceCount(&devices));require(devices>0,"cuda_waves.no_device");
    result.available=true;CW_CUDA(cudaSetDevice(0));cudaDeviceProp prop{};CW_CUDA(cudaGetDeviceProperties(&prop,0));result.device=prop.name;
    View device=host;
    // All counts remain u64 globally. Only this wave's scan uses CUB int.
    const auto q=static_cast<size_t>(std::min<u64>(host.effective,std::min<u64>(requested_q,0x7fffffffU)));
    result.timing.q_actual=q;
    DeviceBuffer<Rectangle> rectangles;DeviceBuffer<Segment> segments;DeviceBuffer<Factor> factors;
    DeviceBuffer<Class> classes;DeviceBuffer<Band> bands;DeviceBuffer<u32> ranks;
    DeviceBuffer<u8> credits;DeviceBuffer<gpu::FlatNode> nodes;DeviceBuffer<std::int32_t> points;
    DeviceBuffer<Edge> slots,compact;DeviceBuffer<u32> flags,positions;
    DeviceBuffer<unsigned long long> totals;DeviceBuffer<unsigned char> scratch;
    const auto upload_start=Clock::now();
#define COPY(field,count,buffer) buffer.copy(host.field,static_cast<size_t>(host.count));device.field=buffer.p;result.timing.upload_bytes=checked_sum(result.timing.upload_bytes,buffer.bytes())
    COPY(rectangles,rectangle_count,rectangles);COPY(segments,segment_count,segments);
    COPY(factors,factor_count,factors);COPY(classes,class_count,classes);COPY(bands,band_count,bands);
    COPY(ranks,rank_count,ranks);COPY(credits,credit_count,credits);COPY(nodes,nodes_count,nodes);
#undef COPY
    require(host.points_count<=std::numeric_limits<size_t>::max()/3,"cuda_waves.points_size");
    points.copy(host.points,static_cast<size_t>(3*host.points_count));device.points=points.p;
    result.timing.upload_bytes=checked_sum(result.timing.upload_bytes,points.bytes());
    slots.allocate(q);compact.allocate(q);flags.allocate(q);positions.allocate(q);totals.allocate(CountSize);
    size_t scratch_bytes=0;
    if(q) {CW_CUDA(cub::DeviceScan::ExclusiveSum(nullptr,scratch_bytes,flags.p,positions.p,static_cast<int>(q)));scratch.allocate(scratch_bytes);}
    CW_CUDA(cudaDeviceSynchronize());result.timing.upload_ms=elapsed(upload_start);
    result.timing.device_bytes=checked_sum(result.timing.upload_bytes,checked_sum(slots.bytes(),checked_sum(compact.bytes(),
      checked_sum(flags.bytes(),checked_sum(positions.bytes(),checked_sum(totals.bytes(),scratch.bytes()))))));
    Counters counts;std::vector<Edge> survivors;
    for(u64 base=0;base<host.effective;) {
      const auto count=static_cast<u32>(std::min<u64>(q,host.effective-base));
      const auto wave_start=Clock::now();CW_CUDA(cudaMemset(totals.p,0,CountSize*sizeof(unsigned long long)));
      constexpr unsigned threads=128;
      const auto blocks=static_cast<unsigned>(std::min<u64>((u64(count)+threads-1)/threads,static_cast<u64>(prop.multiProcessorCount)*64));
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
      const auto download_start=Clock::now();
      const auto old=survivors.size();require(live<=survivors.max_size()-old,"cuda_waves.output_size");
      survivors.resize(old+static_cast<size_t>(live));
      if(live) CW_CUDA(cudaMemcpy(survivors.data()+old,compact.p,static_cast<size_t>(live)*sizeof(Edge),cudaMemcpyDeviceToHost));
      result.timing.download_bytes=checked_sum(result.timing.download_bytes,checked_sum(live*sizeof(Edge),sizeof(wave_counts)+sizeof(tail)));
      result.timing.download_ms+=elapsed(download_start);accumulate(counts,wave_counts);base+=count;
    }
    const auto release_start=Clock::now();
    // Explicit release is inside total_ms; RAII still handles every failure.
    rectangles.release();segments.release();factors.release();classes.release();bands.release();ranks.release();credits.release();
    nodes.release();points.release();slots.release();compact.release();flags.release();positions.release();totals.release();scratch.release();
    result.timing.release_ms=elapsed(release_start);
    result.counters=counts;result.survivors=std::move(survivors);
  } catch(const std::exception& e) {
    result.error=e.what();result.survivors.clear();result.counters={};
  }
  result.timing.total_ms=elapsed(total_start);return result;
}
} // namespace mhgp9::audit::cuda_waves
