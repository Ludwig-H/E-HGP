#include "cuda_api.hpp"
namespace mhgp9::audit::cuda_waves {
DeviceRun execute_cuda(View,size_t) {DeviceRun r;r.error="CUDA not compiled";return r;}
}
