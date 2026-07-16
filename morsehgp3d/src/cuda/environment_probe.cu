#include <cstddef>
#include <cstdint>

extern "C" __global__ void morsehgp3d_environment_probe(
    std::uint32_t* output,
    std::size_t count) {
  const auto index = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
  if (index < count) {
    output[index] = static_cast<std::uint32_t>(index) ^ UINT32_C(0x4d4f5253);
  }
}
