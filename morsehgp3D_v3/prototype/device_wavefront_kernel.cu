// MorseHGP3D v3 — LE KERNEL. Un thread par sommet, le même corps que l'hôte.
//
// Ce fichier ne contient AUCUNE géométrie : il ne fait que transporter le lot et
// appeler `evaluate_vertex`, qui est écrit une seule fois dans
// `device_wavefront_job.hpp` et compilé pour les deux cibles. C'est la condition
// pour qu'une comparaison bit à bit ait un sens — sans quoi on comparerait deux
// implémentations et non deux exécutions.
//
// Discipline empruntée à `morsehgp3d` : l'hôte appelle un symbole qu'il ne définit
// pas, et c'est cette unité qui le définit. Un binaire compilé sans CUDA n'a donc
// simplement pas ce symbole, et son chemin hôte reste qualifiable seul.
#include <cuda_runtime.h>

#include <cstdio>
#include <cstring>

#include "prototype/device_wavefront_job.hpp"

namespace mhgp3v {
namespace device {

namespace {

__global__ void wavefront_kernel(WavefrontJob job, VertexVerdict* out) {
  const int index = blockIdx.x * blockDim.x + threadIdx.x;
  if (index >= job.vertex_count) return;
  evaluate_vertex(job, index, &out[index]);
}

bool record(const char* what, cudaError_t status, char* error, int capacity) {
  if (status == cudaSuccess) return true;
  snprintf(error, (size_t)capacity, "%s : %s", what, cudaGetErrorString(status));
  return false;
}

}  // namespace

bool launch_wavefront_on_gpu(const WavefrontJob& job, VertexVerdict* out, double* milliseconds,
                             char* error, int error_capacity) {
  *milliseconds = 0.0;
  if (job.vertex_count <= 0) return true;

  mhgp::P3* device_points = nullptr;
  mhgp::i32* device_root = nullptr;
  BoundedVertex* device_vertices = nullptr;
  VertexVerdict* device_out = nullptr;
  cudaEvent_t begin = nullptr, end = nullptr;
  bool ok = true;

  const size_t point_bytes = sizeof(mhgp::P3) * (size_t)job.point_count;
  const size_t root_bytes = sizeof(mhgp::i32) * (size_t)job.root_size;
  const size_t vertex_bytes = sizeof(BoundedVertex) * (size_t)job.vertex_count;
  const size_t out_bytes = sizeof(VertexVerdict) * (size_t)job.vertex_count;

  ok = ok && record("cudaMalloc points", cudaMalloc(&device_points, point_bytes), error,
                    error_capacity);
  ok = ok && record("cudaMalloc root", cudaMalloc(&device_root, root_bytes), error,
                    error_capacity);
  ok = ok && record("cudaMalloc vertices", cudaMalloc(&device_vertices, vertex_bytes), error,
                    error_capacity);
  ok = ok && record("cudaMalloc out", cudaMalloc(&device_out, out_bytes), error, error_capacity);
  ok = ok && record("memcpy points",
                    cudaMemcpy(device_points, job.points, point_bytes, cudaMemcpyHostToDevice),
                    error, error_capacity);
  ok = ok && record("memcpy root",
                    cudaMemcpy(device_root, job.root_base, root_bytes, cudaMemcpyHostToDevice),
                    error, error_capacity);
  ok = ok && record("memcpy vertices",
                    cudaMemcpy(device_vertices, job.vertices, vertex_bytes,
                               cudaMemcpyHostToDevice),
                    error, error_capacity);

  if (ok) {
    WavefrontJob device_job = job;
    device_job.points = device_points;
    device_job.root_base = device_root;
    device_job.vertices = device_vertices;

    ok = ok && record("eventCreate begin", cudaEventCreate(&begin), error, error_capacity);
    ok = ok && record("eventCreate end", cudaEventCreate(&end), error, error_capacity);
    if (ok) {
      const int block = 128;
      const int grid = (job.vertex_count + block - 1) / block;
      cudaEventRecord(begin);
      wavefront_kernel<<<grid, block>>>(device_job, device_out);
      cudaEventRecord(end);
      ok = record("kernel", cudaGetLastError(), error, error_capacity);
      ok = ok && record("synchronize", cudaEventSynchronize(end), error, error_capacity);
      if (ok) {
        float elapsed = 0.0f;
        cudaEventElapsedTime(&elapsed, begin, end);
        *milliseconds = (double)elapsed;
      }
    }
  }

  ok = ok && record("memcpy out", cudaMemcpy(out, device_out, out_bytes, cudaMemcpyDeviceToHost),
                    error, error_capacity);

  if (begin != nullptr) cudaEventDestroy(begin);
  if (end != nullptr) cudaEventDestroy(end);
  if (device_points != nullptr) cudaFree(device_points);
  if (device_root != nullptr) cudaFree(device_root);
  if (device_vertices != nullptr) cudaFree(device_vertices);
  if (device_out != nullptr) cudaFree(device_out);
  return ok;
}

}  // namespace device
}  // namespace mhgp3v
