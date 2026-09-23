// Build without MHGP9_ENABLE_CUDA: the device run is unavailable.
#include "filter_runner.hpp"

namespace mhgp9::gpu {

FilterOutput run_filters(const FilterInput& input) {
  FilterOutput out;
  out.error = validate_filter_input(input);
  if (out.error.empty()) out.error = "built without MHGP9_ENABLE_CUDA";
  return out;
}

}  // namespace mhgp9::gpu
