#pragma once
#include "snapshot.hpp"
namespace mhgp9::audit::cuda_waves {
struct Run {
  bool available=false;std::string error,device;
  gen::Q34FilterBatch output;Counters counters;Timing timing;
};
// CUDA version is isolated in runner.cu. A successful result is published
// only after complete decoding/filtering, ordered S, and device destruction.
Run run_cuda(const Snapshot&,size_t q);
} // namespace mhgp9::audit::cuda_waves
