// Build without MHGP9_ENABLE_CUDA: the device run is unavailable.
#include "filter_runner.hpp"

namespace mhgp9::gpu {

FilterOutput run_filters(const FilterInput& input) {
  FilterOutput out;
  out.error = validate_filter_input(input);
  if (out.error.empty()) out.error = "built without MHGP9_ENABLE_CUDA";
  return out;
}

BatchOutput run_filter_batch(const FilterInput& input) {
  BatchOutput out;
  out.error = validate_filter_input(input);
  out.error_kind = BatchError::input_guard;
  if (out.error.empty()) {
    out.error = "built without MHGP9_ENABLE_CUDA";
    out.error_kind = BatchError::no_device;
  }
  return out;
}

std::string warm_up() { return "built without MHGP9_ENABLE_CUDA"; }

}  // namespace mhgp9::gpu
