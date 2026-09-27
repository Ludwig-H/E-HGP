// Isolated audit runner. DeviceBuffer/check pattern explicitly adapted from
// src/gpu/filter_runner.cu at 70168cc3b; no old P/E allocation path is called.
#include "device.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <cuda_runtime.h>
#include <cub/device/device_scan.cuh>
#include <chrono>

namespace mhgp9::audit::resident {
using cw::Rectangle;using cw::Segment;using cw::Factor;using cw::Class;using cw::Band;
using cw::Edge;using cw::View;using cw::Counters;using cw::DeviceRun;using cw::add;
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
  DeviceRun consume(View,size_t) override;
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

DeviceRun ResidentCuda::consume(View host,size_t requested_q) {
  DeviceRun result;const auto total_start=Clock::now();
  try {
    require(!closed_ && !failed_ && requested_q>0,"resident.closed_failed_or_zero_capacity");
    result.available=true;result.device=name_;
    host.nodes_count=input_.nodes_count;host.points_count=input_.points_count;host.k=input_.k;
    View device=host;
    // All counts remain u64 globally. Only this wave's scan uses CUB int.
    const auto q=static_cast<size_t>(std::min<u64>(host.effective,std::min<u64>(requested_q,0x7fffffffU)));
    result.timing.q_actual=q;
    DeviceBuffer<Rectangle> rectangles;DeviceBuffer<Segment> segments;DeviceBuffer<Factor> factors;
    DeviceBuffer<Class> classes;DeviceBuffer<Band> bands;DeviceBuffer<u32> ranks;
    DeviceBuffer<u8> credits;
    DeviceBuffer<Edge> slots,compact;DeviceBuffer<u32> flags,positions;
    DeviceBuffer<unsigned long long> totals;DeviceBuffer<unsigned char> scratch;
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
    Counters counts;std::vector<Edge> survivors;
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
    slots.release();compact.release();flags.release();positions.release();totals.release();scratch.release();
    result.timing.release_ms=elapsed(release_start);
    result.counters=counts;result.survivors=std::move(survivors);
  } catch(const std::exception& e) {
    failed_=true;result.error=e.what();result.survivors.clear();result.counters={};
  }
  result.timing.total_ms=elapsed(total_start);return result;
}
} // namespace mhgp9::audit::resident
