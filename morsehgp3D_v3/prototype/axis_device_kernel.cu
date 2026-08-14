// MorseHGP3D v3 — LE KERNEL `Q4SeedAxisTopR4`. Un thread par `Q4Seed3`.
//
// Ce fichier ne contient AUCUNE geometrie : il transporte le lot et appelle
// `evaluate_seed`, ecrite une seule fois dans `axis_device_job.hpp` et compilee
// pour les deux cibles. Meme discipline que le front d'onde et l'ancre.
#include <cuda_runtime.h>

#include <cstdio>
#include <cstring>

#include "prototype/axis_device_job.hpp"

namespace mhgp3v {
namespace axisdev {

namespace {

__global__ void axis_kernel(AxisJob job, SeedOut* out) {
  const int index = blockIdx.x * blockDim.x + threadIdx.x;
  if (index >= job.nseeds) return;
  evaluate_seed(job, index, &out[index]);
}

bool record(const char* what, cudaError_t status, char* error, int capacity) {
  if (status == cudaSuccess) return true;
  snprintf(error, (size_t)capacity, "%s : %s", what, cudaGetErrorString(status));
  return false;
}

}  // namespace

bool launch_axis_on_gpu(const AxisJob& job, SeedOut* out, double* milliseconds,
                        char* error, int error_capacity) {
  *milliseconds = 0.0;
  if (job.nseeds <= 0) return true;

  const int nsites = job.site_offset[job.nseeds];
  mhgp::i64* d_coords = nullptr;
  int* d_seed = nullptr;
  int* d_offset = nullptr;
  int* d_site = nullptr;
  SeedOut* d_out = nullptr;
  bool ok = true;
  auto libere = [&]() {
    if (d_coords) cudaFree(d_coords);
    if (d_seed) cudaFree(d_seed);
    if (d_offset) cudaFree(d_offset);
    if (d_site) cudaFree(d_site);
    if (d_out) cudaFree(d_out);
  };

  ok = ok && record("cudaMalloc coords",
                    cudaMalloc(&d_coords, sizeof(mhgp::i64) * 3 * (size_t)job.npoints),
                    error, error_capacity);
  ok = ok && record("cudaMalloc seed",
                    cudaMalloc(&d_seed, sizeof(int) * 3 * (size_t)job.nseeds),
                    error, error_capacity);
  ok = ok && record("cudaMalloc offset",
                    cudaMalloc(&d_offset, sizeof(int) * ((size_t)job.nseeds + 1)),
                    error, error_capacity);
  ok = ok && record("cudaMalloc site",
                    cudaMalloc(&d_site, sizeof(int) * (size_t)nsites),
                    error, error_capacity);
  ok = ok && record("cudaMalloc out",
                    cudaMalloc(&d_out, sizeof(SeedOut) * (size_t)job.nseeds),
                    error, error_capacity);
  ok = ok && record("memcpy coords",
                    cudaMemcpy(d_coords, job.coords,
                               sizeof(mhgp::i64) * 3 * (size_t)job.npoints,
                               cudaMemcpyHostToDevice), error, error_capacity);
  ok = ok && record("memcpy seed",
                    cudaMemcpy(d_seed, job.seed, sizeof(int) * 3 * (size_t)job.nseeds,
                               cudaMemcpyHostToDevice), error, error_capacity);
  ok = ok && record("memcpy offset",
                    cudaMemcpy(d_offset, job.site_offset,
                               sizeof(int) * ((size_t)job.nseeds + 1),
                               cudaMemcpyHostToDevice), error, error_capacity);
  ok = ok && record("memcpy site",
                    cudaMemcpy(d_site, job.site_id, sizeof(int) * (size_t)nsites,
                               cudaMemcpyHostToDevice), error, error_capacity);
  if (!ok) { libere(); return false; }

  AxisJob device_job = job;
  device_job.coords = d_coords;
  device_job.seed = d_seed;
  device_job.site_offset = d_offset;
  device_job.site_id = d_site;

  cudaEvent_t debut, fin;
  ok = ok && record("event create", cudaEventCreate(&debut), error, error_capacity);
  ok = ok && record("event create", cudaEventCreate(&fin), error, error_capacity);
  if (!ok) { libere(); return false; }

  const int block = 128;
  const int grid = (job.nseeds + block - 1) / block;
  // Un warmup non chronometre : le premier lancement paie la creation du
  // contexte, et le confondre avec le debit serait une mesure fausse.
  axis_kernel<<<grid, block>>>(device_job, d_out);
  ok = ok && record("warmup", cudaDeviceSynchronize(), error, error_capacity);
  cudaEventRecord(debut);
  axis_kernel<<<grid, block>>>(device_job, d_out);
  cudaEventRecord(fin);
  ok = ok && record("kernel", cudaDeviceSynchronize(), error, error_capacity);
  float ms = 0.0F;
  if (ok) cudaEventElapsedTime(&ms, debut, fin);
  *milliseconds = (double)ms;
  ok = ok && record("memcpy out",
                    cudaMemcpy(out, d_out, sizeof(SeedOut) * (size_t)job.nseeds,
                               cudaMemcpyDeviceToHost), error, error_capacity);
  cudaEventDestroy(debut);
  cudaEventDestroy(fin);
  libere();
  return ok;
}

}  // namespace axisdev
}  // namespace mhgp3v
