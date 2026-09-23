// Build without MHGP9_ENABLE_CUDA: the device run is unavailable.
#include "filter_runner.hpp"

namespace mhgp9::gpu {

FilterOutput run_filters(const FilterInput&) {
  FilterOutput out;
  out.error = "built without MHGP9_ENABLE_CUDA";
  return out;
}

}  // namespace mhgp9::gpu
