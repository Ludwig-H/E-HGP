#pragma once
// Plain C++17 API seen by NVCC; no Prepared, span, Boost, or native builder.
#include "wire.hpp"
#include <string>
#include <vector>
namespace mhgp9::audit::cuda_waves {
struct Counters {u64 queries{},q3{},q4{},rejected3{},rejected4{},visits{},waves{},out_of_order{};};
struct Timing {
  double upload_ms{},waves_ms{},download_ms{},order_ms{},release_ms{},total_ms{};
  u64 upload_bytes{},download_bytes{},device_bytes{},q_actual{};
};
struct DeviceRun {
  bool available=false;std::string error,device;
  std::vector<Edge> survivors;Counters counters;Timing timing;
};
// Input pointers remain borrowed and immutable until this synchronous call
// returns. Only Snapshot creates a production call; raw View is not public
// untrusted geometry. Internal device buffers are destroyed before return.
DeviceRun execute_cuda(View,size_t q);
}
