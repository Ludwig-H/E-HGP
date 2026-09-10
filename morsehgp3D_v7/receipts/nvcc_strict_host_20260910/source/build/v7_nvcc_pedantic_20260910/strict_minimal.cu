#include <cstdio>
#include <cuda_runtime.h>
__global__ void mhgp7_minimal_kernel(int* output) { output[threadIdx.x] = 7; }
int main() {
  int* output = nullptr;
  if (cudaMalloc(&output, 4 * sizeof(int)) != cudaSuccess) return 2;
  mhgp7_minimal_kernel<<<1, 4>>>(output);
  const auto result = cudaDeviceSynchronize();
  (void)cudaFree(output);
  std::puts("compiled device fixture");
  return result == cudaSuccess ? 0 : 1;
}
