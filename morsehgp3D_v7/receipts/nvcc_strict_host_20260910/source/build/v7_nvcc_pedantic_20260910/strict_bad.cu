#include <cuda_runtime.h>
__global__ void mhgp7_bad_kernel(int* output) { output[threadIdx.x] = 7; }
int mhgp7_bad_host(int n) {
  int variable_length[n];
  variable_length[0] = n;
  return variable_length[0];
}
