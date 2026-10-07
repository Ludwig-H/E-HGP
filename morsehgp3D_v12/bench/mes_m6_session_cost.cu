// MES-M6 : cout de la Session residente sur l'appareil (docs/PLAN.md, T0 ; docs/ARCHITECTURE.md, section 2).
//
// Mesure, dans un seul processus neuf : ouverture du contexte (premier appel CUDA), creation de flux, reservations
// epinglees et pool d'appareil, latence d'un noyau vide (lancement + synchronisation) selon le mode d'attente de
// l'hote, debit et latence des transferts (epingle et pageable, hote vers appareil et retour), lancement d'un graphe
// de dix noyaux contre dix lancements, et allocation asynchrone a chaud depuis le pool. Rien ne decide : banc publie,
// qui fixe le budget du regime residant (decision D1).
//
// Sortie : une ligne JSON par mesure sur stdout (cle "mes":"M6"), quantiles en microsecondes, interpoles lineairement
// entre rangs (la mediane d'un effectif pair est la moyenne des deux rangs centraux, CST-0209). Toute mesure repetee
// separe son premier usage (ligne "<nom>_first", preparation et chargement compris) des repetitions chaudes, seules
// comptees dans les quantiles (CST-0210).
// Codes : 0 conforme ; 2 usage faux ; 3 echec CUDA (message sur stderr).
// Usage : mes_m6_session_cost [--sync=spin|yield|blocking] [--reps=N]   (N de 10 a 100000, 2000 par defaut)
#include <cuda_runtime.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

struct CudaFailure {
  const char* what;
  cudaError_t error;
};

void check(cudaError_t error, const char* what) {
  if (error != cudaSuccess) throw CudaFailure{what, error};
}

double micros(Clock::duration d) { return std::chrono::duration<double, std::micro>(d).count(); }

struct Quantiles {
  double p05, p50, p95, max;
};

void emit(const char* name, const std::string& extra, const Quantiles& q, size_t n) {
  std::printf("{\"mes\":\"M6\",\"name\":\"%s\"%s,\"n\":%zu,\"p05_us\":%.3f,\"p50_us\":%.3f,\"p95_us\":%.3f,"
              "\"max_us\":%.3f}\n",
              name, extra.c_str(), n, q.p05, q.p50, q.p95, q.max);
}

// Quantile par interpolation lineaire entre les rangs floor(h) et floor(h)+1, h = q (n - 1) ; v non vide.
double interpolated(const std::vector<double>& sorted, double q) {
  const double h = q * static_cast<double>(sorted.size() - 1);
  const size_t low = static_cast<size_t>(h);
  if (low + 1 >= sorted.size()) return sorted.back();
  return sorted[low] + (h - static_cast<double>(low)) * (sorted[low + 1] - sorted[low]);
}

Quantiles quantiles(std::vector<double> v) {
  std::sort(v.begin(), v.end());
  return {interpolated(v, 0.05), interpolated(v, 0.50), interpolated(v, 0.95), v.back()};
}

// Auto-test de la regle des quantiles, joue avant tout appel CUDA : 1..10 donne une mediane de 5,5.
bool quantiles_selftest() {
  std::vector<double> v;
  for (int i = 10; i >= 1; --i) v.push_back(i);
  const Quantiles q = quantiles(v);
  std::vector<double> odd = {3.0, 1.0, 2.0};
  return q.p50 == 5.5 && q.max == 10.0 && quantiles(odd).p50 == 2.0;
}

// Premier usage puis repetitions chaudes : body() mesure une repetition en microsecondes.
template <class Body>
void first_then_warm(const char* name, const std::string& extra, int reps, Body body) {
  const double first = body();
  std::printf("{\"mes\":\"M6\",\"name\":\"%s_first\"%s,\"n\":1,\"us\":%.3f}\n", name, extra.c_str(), first);
  std::vector<double> v;
  v.reserve(static_cast<size_t>(reps));
  for (int r = 0; r < reps; ++r) v.push_back(body());
  emit(name, extra, quantiles(v), v.size());
}


void emit_once(const char* name, double us) {
  std::printf("{\"mes\":\"M6\",\"name\":\"%s\",\"n\":1,\"us\":%.3f}\n", name, us);
}

__global__ void empty_kernel() {}

__global__ void touch_kernel(unsigned* data, size_t words) {
  const size_t i = blockIdx.x * static_cast<size_t>(blockDim.x) + threadIdx.x;
  if (i < words) data[i] += 1u;
}

int run(unsigned sync_flag, const char* sync_name, int reps) {
  // 1. Ouverture du contexte : premier appel CUDA du processus, apres le choix du mode d'attente.
  const auto t0 = Clock::now();
  check(cudaSetDeviceFlags(sync_flag), "cudaSetDeviceFlags");
  check(cudaFree(nullptr), "context");
  emit_once("context_open", micros(Clock::now() - t0));

  cudaDeviceProp prop{};
  check(cudaGetDeviceProperties(&prop, 0), "cudaGetDeviceProperties");
  std::printf("{\"mes\":\"M6\",\"name\":\"device\",\"device\":\"%s\",\"sm\":%d,\"cc\":\"%d.%d\",\"sync\":\"%s\","
              "\"global_mib\":%zu}\n",
              prop.name, prop.multiProcessorCount, prop.major, prop.minor, sync_name,
              static_cast<size_t>(prop.totalGlobalMem >> 20));

  // 2. Flux et premier lancement (chargement paresseux du module).
  auto t = Clock::now();
  cudaStream_t stream{};
  cudaStream_t copy{};
  check(cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking), "stream");
  check(cudaStreamCreateWithFlags(&copy, cudaStreamNonBlocking), "stream copy");
  emit_once("two_streams", micros(Clock::now() - t));
  t = Clock::now();
  empty_kernel<<<1, 1, 0, stream>>>();
  check(cudaGetLastError(), "first launch");
  check(cudaStreamSynchronize(stream), "first sync");
  emit_once("first_launch", micros(Clock::now() - t));

  // 3. Reservations : epingle sur l'hote, pool d'appareil retenu (seuil de liberation maximal).
  constexpr size_t kBig = size_t{256} << 20;
  t = Clock::now();
  void* pinned = nullptr;
  check(cudaMallocHost(&pinned, kBig), "cudaMallocHost");
  emit_once("pinned_alloc_256mib", micros(Clock::now() - t));
  std::memset(pinned, 1, kBig);
  std::vector<unsigned char> pageable(kBig, 1);
  cudaMemPool_t pool{};
  check(cudaDeviceGetDefaultMemPool(&pool, 0), "pool");
  unsigned long long threshold = ~0ull;
  check(cudaMemPoolSetAttribute(pool, cudaMemPoolAttrReleaseThreshold, &threshold), "pool threshold");
  t = Clock::now();
  void* device = nullptr;
  check(cudaMallocAsync(&device, kBig, stream), "cudaMallocAsync cold");
  check(cudaStreamSynchronize(stream), "sync alloc");
  emit_once("pool_alloc_cold_256mib", micros(Clock::now() - t));
  check(cudaFreeAsync(device, stream), "cudaFreeAsync");
  check(cudaStreamSynchronize(stream), "sync free");
  first_then_warm("pool_alloc_free_256mib", "", std::min(reps, 1000), [&] {
    const auto s = Clock::now();
    check(cudaMallocAsync(&device, kBig, stream), "cudaMallocAsync warm");
    check(cudaFreeAsync(device, stream), "cudaFreeAsync warm");
    check(cudaStreamSynchronize(stream), "sync warm alloc");
    return micros(Clock::now() - s);
  });
  check(cudaMallocAsync(&device, kBig, stream), "cudaMallocAsync resident");
  check(cudaStreamSynchronize(stream), "sync resident");

  // 4. Latence d'un noyau vide, lancement et synchronisation.
  first_then_warm("empty_kernel_launch_sync", "", reps, [&] {
    const auto s = Clock::now();
    empty_kernel<<<1, 1, 0, stream>>>();
    check(cudaStreamSynchronize(stream), "sync empty");
    return micros(Clock::now() - s);
  });

  // 5. Dix noyaux : lancements successifs contre un graphe capture.
  constexpr int kChain = 10;
  first_then_warm("ten_launches_sync", "", reps, [&] {
    const auto s = Clock::now();
    for (int i = 0; i < kChain; ++i) empty_kernel<<<1, 1, 0, stream>>>();
    check(cudaStreamSynchronize(stream), "sync chain");
    return micros(Clock::now() - s);
  });
  {
    cudaGraph_t graph{};
    cudaGraphExec_t exec{};
    const auto prepare = Clock::now();
    check(cudaStreamBeginCapture(stream, cudaStreamCaptureModeThreadLocal), "capture begin");
    for (int i = 0; i < kChain; ++i) empty_kernel<<<1, 1, 0, stream>>>();
    check(cudaStreamEndCapture(stream, &graph), "capture end");
    check(cudaGraphInstantiate(&exec, graph, 0), "graph instantiate");
    emit_once("graph_of_ten_prepare", micros(Clock::now() - prepare));
    first_then_warm("graph_of_ten_sync", "", reps, [&] {
      const auto s = Clock::now();
      check(cudaGraphLaunch(exec, stream), "graph launch");
      check(cudaStreamSynchronize(stream), "sync graph");
      return micros(Clock::now() - s);
    });
    check(cudaGraphExecDestroy(exec), "graph exec destroy");
    check(cudaGraphDestroy(graph), "graph destroy");
  }

  // 6. Transferts : tailles d'une trame (60 000 sites x 12 octets = 720 Kio) a un lot de plusieurs millions de sites.
  const size_t sizes[] = {size_t{4} << 10, size_t{64} << 10, size_t{720} << 10, size_t{4} << 20, size_t{64} << 20,
                          kBig};
  for (const size_t bytes : sizes) {
    const int n = bytes >= (size_t{64} << 20) ? std::max(10, reps / 100) : std::max(10, reps / 10);
    for (int pinned_side = 1; pinned_side >= 0; --pinned_side) {
      void* host = pinned_side ? pinned : static_cast<void*>(pageable.data());
      for (int direction = 0; direction < 2; ++direction) {
        char extra[128];
        std::snprintf(extra, sizeof extra, ",\"bytes\":%zu,\"host\":\"%s\",\"dir\":\"%s\"", bytes,
                      pinned_side ? "pinned" : "pageable", direction == 0 ? "h2d" : "d2h");
        first_then_warm("copy", extra, n, [&] {
          const auto s = Clock::now();
          if (direction == 0)
            check(cudaMemcpyAsync(device, host, bytes, cudaMemcpyHostToDevice, copy), "h2d");
          else
            check(cudaMemcpyAsync(host, device, bytes, cudaMemcpyDeviceToHost, copy), "d2h");
          check(cudaStreamSynchronize(copy), "sync copy");
          return micros(Clock::now() - s);
        });
      }
    }
  }

  // 7. Un noyau qui touche 256 Mio : debit de la memoire de l'appareil, pour situer les budgets d'etage.
  {
    const size_t words = kBig / sizeof(unsigned);
    const unsigned threads = 256;
    const unsigned blocks = static_cast<unsigned>((words + threads - 1) / threads);
    char extra[64];
    std::snprintf(extra, sizeof extra, ",\"bytes_rw\":%zu", 2 * kBig);
    first_then_warm("touch_256mib", extra, std::max(10, reps / 100), [&] {
      const auto s = Clock::now();
      touch_kernel<<<blocks, threads, 0, stream>>>(static_cast<unsigned*>(device), words);
      check(cudaStreamSynchronize(stream), "sync touch");
      return micros(Clock::now() - s);
    });
  }

  check(cudaFreeAsync(device, stream), "free resident");
  check(cudaStreamSynchronize(stream), "final sync");
  check(cudaFreeHost(pinned), "cudaFreeHost");
  check(cudaStreamDestroy(copy), "stream destroy");
  check(cudaStreamDestroy(stream), "stream destroy");
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  unsigned flag = cudaDeviceScheduleSpin;
  const char* name = "spin";
  int reps = 2000;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--sync=spin") {
      flag = cudaDeviceScheduleSpin;
      name = "spin";
    } else if (arg == "--sync=yield") {
      flag = cudaDeviceScheduleYield;
      name = "yield";
    } else if (arg == "--sync=blocking") {
      flag = cudaDeviceScheduleBlockingSync;
      name = "blocking";
    } else if (arg.rfind("--reps=", 0) == 0) {
      char* end = nullptr;
      const long value = std::strtol(arg.c_str() + 7, &end, 10);
      if (*end != '\0' || value < 10 || value > 100000) {
        std::fprintf(stderr, "mes_m6 : --reps hors de [10, 100000]\n");
        return 2;
      }
      reps = static_cast<int>(value);
    } else {
      std::fprintf(stderr, "usage : %s [--sync=spin|yield|blocking] [--reps=N]\n", argv[0]);
      return 2;
    }
  }
  if (!quantiles_selftest()) {
    std::fprintf(stderr, "mes_m6 : auto-test des quantiles en echec\n");
    return 3;
  }
  try {
    return run(flag, name, reps);
  } catch (const CudaFailure& failure) {
    std::fprintf(stderr, "mes_m6 : echec CUDA (%s) : %s\n", failure.what, cudaGetErrorString(failure.error));
    return 3;
  }
}
