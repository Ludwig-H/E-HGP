#include "runner.hpp"
namespace mhgp9::audit::cuda_waves {
Run run_cuda(const Snapshot& snapshot,size_t q) {
  const auto start=std::chrono::steady_clock::now();Run out;
  auto device=execute_cuda(snapshot.view(),q);
  out.available=device.available;out.device=device.device;out.error=device.error;out.timing=device.timing;
  if (out.error.empty() && out.available) {
    try {
      const auto t=std::chrono::steady_clock::now();
      for(size_t i=1;i<device.survivors.size();++i)
        if(device.survivors[i-1].ordinal>=device.survivors[i].ordinal) ++device.counters.out_of_order;
      out.output=finish(snapshot,device.survivors,device.counters);out.counters=device.counters;
      out.timing.order_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t).count();
    } catch(const std::exception& e) {out.error=e.what();out.output={};out.counters={};}
  }
  const auto release=std::chrono::steady_clock::now();std::vector<Edge>().swap(device.survivors);
  out.timing.release_ms+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-release).count();
  out.timing.total_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
  return out;
}
}
