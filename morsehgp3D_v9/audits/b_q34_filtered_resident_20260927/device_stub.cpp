#include "device.hpp"
#include <stdexcept>
namespace mhgp9::audit::resident {
Opened open_cuda(IndexInput) {throw std::runtime_error("resident.CUDA_not_compiled");}
}
