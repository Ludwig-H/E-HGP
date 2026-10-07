// Executeur CUDA de la voie appareil du catalogue (tranche T1-b) et contexte resident CatalogueDevice. Port explicite
// de l'executeur du microbanc MES-M5 (microbancs/mes_m5_parcours/cuda/traversal_bench.cu : noyau de warps generique,
// quatre warps par bloc, memoire partagee par warp, un flux, totaux lus en memoire epinglee) ; les noyaux de feuille
// (J3, borne de registres de MES-M2) sont lances par device_leaf.cu. Les noyaux et les pilotes sont ceux de
// l'executeur Pool (source unique) ; ce fichier ne fait que les lancer, copier et compter.
//
// Memoire : chaque tableau de l'appareil et la memoire epinglee de transit sont reserves dans le budget du contexte
// (BudgetReservation) AVANT cudaMalloc ou cudaMallocHost ; un refus de l'un ou de l'autre rend memory_budget, sans rien
// publier. Les tableaux sont gardes d'un appel a l'autre (regime resident, decision D1) et rendus a la destruction du
// contexte. Erreurs du pilote : device_fault, et le contexte refuse ensuite tout appel (une erreur collante de CUDA
// rend le contexte inutilisable). Attente en mode yield (MES-M6). Transferts (CST-0235, raccord complet) : chaque copie
// hote <-> appareil attend d'abord les noyaux en file (hors chrono), puis est chronometree et comptee (TransferMeter).
#include <cuda_runtime.h>

#include <type_traits>

#include "catalogue/device_pipeline.hpp"
#include "catalogue/exec_host.hpp"

namespace mhgp12 {
namespace catalogue_detail::dev {
namespace {

inline constexpr u32 kWarpsPerBlock = 4;
inline constexpr u32 kThreads = 32 * kWarpsPerBlock;
inline constexpr u64 kStagingMin = u64{1} << 20;           // memoire epinglee de transit, par tranches
inline constexpr u64 kStagingMax = u64{64} << 20;

// Noyau de warps generique : un warp par indice, memoire partagee propre a chaque warp.
template <class K>
__global__ void __launch_bounds__(kThreads) warp_kernel(K k, u64 n) {
  __shared__ typename K::Shared shared[kWarpsPerBlock];
  const u32 warp = threadIdx.x >> 5;
  const u64 w = u64(blockIdx.x) * kWarpsPerBlock + warp;
  if (w >= n) return;  // uniforme sur le warp
  k(w, shared[warp]);
}

}  // namespace

struct CudaExecutor {
  MemoryBudget& budget;
  sched::Pool* pool = nullptr;  // copies hote paralleles, donne a chaque appel
  cudaStream_t stream = nullptr;
  bool broken = false;
  u8* staging = nullptr;
  u64 staging_bytes = 0;
  BudgetReservation staging_reservation;
  u64 allocations = 0, device_bytes = 0;
  TransferMeter meter{};

  template <class T>
  struct Array {
    T* ptr = nullptr;
    u64 cap = 0;
    BudgetReservation reservation;
    Array() = default;
    Array(const Array&) = delete;
    Array& operator=(const Array&) = delete;
    ~Array() {
      if (ptr != nullptr) cudaFree(ptr);
    }
    T* data() const noexcept { return ptr; }
  };

  explicit CudaExecutor(MemoryBudget& b) noexcept : budget(b) {}
  CudaExecutor(const CudaExecutor&) = delete;
  CudaExecutor& operator=(const CudaExecutor&) = delete;
  ~CudaExecutor() {
    if (stream != nullptr) cudaStreamSynchronize(stream);
    if (staging != nullptr) cudaFreeHost(staging);
    if (stream != nullptr) cudaStreamDestroy(stream);
  }

  // Erreur du pilote : le contexte est perdu (device_fault) ; un defaut de memoire de l'appareil est un refus de
  // ressources, sans perte du contexte.
  Outcome check(cudaError_t e) noexcept {
    if (e == cudaSuccess) return {};
    if (e == cudaErrorMemoryAllocation) {
      (void)cudaGetLastError();
      return fail(Reason::memory_budget);
    }
    broken = true;
    return device_refusal(true);
  }
  Outcome sync() noexcept { return check(cudaStreamSynchronize(stream)); }

  // Tableau d'au moins n elements ; les `keep` premiers sont gardes. Reservation dans le budget avant cudaMalloc.
  template <class T>
  Outcome grow(Array<T>& a, u64 n, u64 keep) noexcept {
    if (a.cap >= n) return {};
    const u64 cap = n > a.cap + a.cap / 2 + 1024 ? n : a.cap + a.cap / 2 + 1024;
    if (cap > ~u64{0} / sizeof(T)) return fail(Reason::memory_budget);
    BudgetReservation reservation;
    MHGP12_TRY(reservation.reserve(cap * sizeof(T), budget));
    T* fresh = nullptr;
    MHGP12_TRY(check(cudaMalloc(reinterpret_cast<void**>(&fresh), cap * sizeof(T))));
    Outcome moved{};
    if (keep != 0) moved = check(cudaMemcpyAsync(fresh, a.ptr, keep * sizeof(T), cudaMemcpyDeviceToDevice, stream));
    if (moved.ok()) moved = sync();  // l'ancien tableau peut servir a des noyaux en file
    if (!moved.ok()) {
      cudaFree(fresh);
      return moved;
    }
    if (a.ptr != nullptr) cudaFree(a.ptr);
    device_bytes += (cap - a.cap) * sizeof(T);
    a.ptr = fresh;
    a.cap = cap;
    a.reservation.swap(reservation);  // l'ancienne reservation est rendue en sortie
    ++allocations;
    return {};
  }
  template <class T>
  Outcome ensure(Array<T>& a, u64 n) noexcept {
    return grow(a, n, 0);
  }
  template <class T>
  Outcome ensure_keep(Array<T>& a, u64 n, u64 keep) noexcept {
    return grow(a, n, keep);
  }

  // Memoire epinglee de transit d'au moins `bytes` octets (au plus kStagingMax), reservee dans le budget.
  Outcome stage(u64 bytes) noexcept {
    bytes = bytes < kStagingMin ? kStagingMin : bytes < kStagingMax ? bytes : kStagingMax;
    if (staging_bytes >= bytes) return {};
    MHGP12_TRY(sync());
    if (staging != nullptr) cudaFreeHost(staging);
    staging = nullptr;
    staging_bytes = 0;
    staging_reservation.reset();
    MHGP12_TRY(staging_reservation.reserve(bytes, budget));
    const Outcome made = check(cudaMallocHost(reinterpret_cast<void**>(&staging), bytes));
    if (!made.ok()) {
      staging = nullptr;
      staging_reservation.reset();
      return made;
    }
    staging_bytes = bytes;
    ++allocations;
    return {};
  }

  // Copie hote parallele (transit epingle <-> memoire de l'appel).
  Outcome host_copy(u8* to, const u8* from, u64 bytes) noexcept {
    PoolExecutor host{*pool, budget};
    return host.copy(to, from, bytes);
  }

  template <class T>
  Outcome upload(Array<T>& a, const T* from, u64 n, u64 at) noexcept {
    const u64 total = n * sizeof(T);
    MHGP12_TRY(stage(total));
    MHGP12_TRY(sync());  // noyaux en file : hors du chrono des transferts
    const Stopwatch watch;
    for (u64 done = 0; done < total; done += staging_bytes) {
      const u64 part = total - done < staging_bytes ? total - done : staging_bytes;
      MHGP12_TRY(host_copy(staging, reinterpret_cast<const u8*>(from) + done, part));
      MHGP12_TRY(check(cudaMemcpyAsync(reinterpret_cast<u8*>(a.ptr + at) + done, staging, part,
                                       cudaMemcpyHostToDevice, stream)));
      MHGP12_TRY(sync());
    }
    meter.note(watch, total, 0);
    return {};
  }
  template <class T>
  Outcome download(T* to, Array<T>& a, u64 n, u64 at) noexcept {
    const u64 total = n * sizeof(T);
    MHGP12_TRY(stage(total));
    MHGP12_TRY(sync());  // noyaux en file : hors du chrono des transferts
    const Stopwatch watch;
    for (u64 done = 0; done < total; done += staging_bytes) {
      const u64 part = total - done < staging_bytes ? total - done : staging_bytes;
      MHGP12_TRY(check(cudaMemcpyAsync(staging, reinterpret_cast<const u8*>(a.ptr + at) + done, part,
                                       cudaMemcpyDeviceToHost, stream)));
      MHGP12_TRY(sync());
      MHGP12_TRY(host_copy(reinterpret_cast<u8*>(to) + done, staging, part));
    }
    meter.note(watch, 0, total);
    return {};
  }
  template <class T>
  Outcome put(Array<T>& a, const T& value, u64 at) noexcept {
    return upload(a, &value, 1, at);
  }
  template <class T>
  Result<T> read(Array<T>& a, u64 at) noexcept {
    T value{};
    MHGP12_TRY(download(&value, a, 1, at));
    return value;
  }
  Result<bfs::LevelTotals> read_totals(Array<bfs::LevelTotals>& a) noexcept { return read(a, 0); }
  template <class T>
  Outcome take(Array<T>& a, Buffer<T>& out, u64 n) noexcept {
    Buffer<T> made;
    MHGP12_TRY(made.allocate(n, budget));
    if (n != 0) MHGP12_TRY(download(made.data(), a, n, 0));
    out.swap(made);
    return {};
  }
  template <class T, class U>
  Outcome take_cast(Array<U>& a, Buffer<T>& out, u64 n) noexcept {
    static_assert(sizeof(T) == sizeof(U) && std::is_trivially_copyable_v<T>, "take_cast : types de meme taille");
    Buffer<T> made;
    MHGP12_TRY(made.allocate(n, budget));
    if (n != 0) MHGP12_TRY(download(reinterpret_cast<U*>(static_cast<void*>(made.data())), a, n, 0));
    out.swap(made);
    return {};
  }

  template <class K>
  Outcome launch(const K& kernel, u64 warps) noexcept {
    if (broken) return device_refusal(true);
    if (warps == 0) return {};
    const u64 blocks = (warps + kWarpsPerBlock - 1) / kWarpsPerBlock;
    if (blocks > 0x7FFFFFFFull) return fail(Reason::index_overflow_u32);
    if constexpr (std::is_same_v<K, CountKernel>) {
      return check(static_cast<cudaError_t>(launch_count_kernel(kernel, warps, stream)));
    } else if constexpr (std::is_same_v<K, ReplayKernel>) {
      return check(static_cast<cudaError_t>(launch_replay_kernel(kernel, warps, stream)));
    } else {
      warp_kernel<K><<<static_cast<unsigned>(blocks), kThreads, 0, stream>>>(kernel, warps);
      return check(cudaGetLastError());
    }
  }
};

}  // namespace catalogue_detail::dev

struct CatalogueDevice::Impl {
  explicit Impl(MemoryBudget& b) noexcept : exec(b) {}
  catalogue_detail::dev::CudaExecutor exec;
  catalogue_detail::dev::DeviceState<catalogue_detail::dev::CudaExecutor> state;  // detruit avant l'executeur
};

CatalogueDevice::CatalogueDevice(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
CatalogueDevice::CatalogueDevice(CatalogueDevice&& other) noexcept = default;
CatalogueDevice::~CatalogueDevice() = default;

Result<CatalogueDevice> CatalogueDevice::open(MemoryBudget& budget) noexcept {
  using catalogue_detail::dev::device_refusal;
  int count = 0;
  if (cudaGetDeviceCount(&count) != cudaSuccess || count < 1) {
    (void)cudaGetLastError();
    return device_refusal(false);
  }
  if (cudaSetDevice(0) != cudaSuccess) return device_refusal(false);
  const cudaError_t flags = cudaSetDeviceFlags(cudaDeviceScheduleYield);  // deja actif : sans effet
  if (flags != cudaSuccess && flags != cudaErrorSetOnActiveProcess) return device_refusal(false);
  (void)cudaGetLastError();
  if (cudaFree(nullptr) != cudaSuccess) return device_refusal(false);  // contexte
  return guarded([&]() -> Result<CatalogueDevice> {
    auto impl = std::make_unique<Impl>(budget);
    if (cudaStreamCreateWithFlags(&impl->exec.stream, cudaStreamNonBlocking) != cudaSuccess) {
      impl->exec.stream = nullptr;
      return device_refusal(false);
    }
    return CatalogueDevice(std::move(impl));
  });
}

Result<Catalogue> build_catalogue_device(const Cloud& cloud, const CatalogueParams& params, CatalogueDevice& device,
                                         sched::Pool& pool, CatalogueDiagnostics* diagnostics) noexcept {
  MHGP12_TRY(check_catalogue_params(params));
  auto& impl = device.impl();
  if (impl.exec.broken) return catalogue_detail::dev::device_refusal(true);
  impl.exec.pool = &pool;
  impl.exec.allocations = 0;
  CatalogueDiagnostics diag;
  auto result = guarded([&]() {
    return catalogue_detail::dev::device_catalogue(impl.exec, impl.state, cloud, params, impl.exec.budget, pool, diag);
  });
  if (!result.ok()) return result;
  diag.device_bytes = impl.exec.device_bytes;
  diag.pinned_bytes = impl.exec.staging_bytes;
  diag.allocations = impl.exec.allocations;
  if (diagnostics != nullptr) *diagnostics = diag;
  return result;
}

}  // namespace mhgp12
