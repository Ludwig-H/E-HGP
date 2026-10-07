// Banc CUDA MES-M5 (hors produit) : parcours des boites du catalogue en largeur sur l'appareil.
//
// Le meme texte que l'identite hote (include/mhgp12/traversal/bfs.hpp et driver.hpp) ; ici l'executeur est CUDA : un
// noyau de warps generique par corps de noyau (4 warps par bloc, memoire partagee par warp), un flux, tampons gardes
// d'une passe a l'autre (regime residant : aucune reservation apres l'echauffement, compte publie), totaux du niveau
// lus dans une memoire epinglee (un aller-retour par niveau).
//
// Une passe chronometree (evenements CUDA sur le flux, plus horloge de l'hote) :
//   upload    copie du nuage (x, y, z depuis une memoire epinglee) ;
//   levels    tous les niveaux (noyaux, lancements, allers-retours des totaux, enveloppe de la racine sur l'hote) ;
//   download  rapatriement de la table des feuilles et de leurs sites (memoire epinglee) ;
//   total     upload + levels + download : la mesure de la regle d'adoption (« transferts compris ») ;
//   resident  upload + levels : feuilles laissees sur l'appareil, comme dans la Session ;
//   wall      horloge de l'hote autour de la passe.
// Une passe de profil (hors chrono) jalonne chaque niveau : temps des noyaux seuls (somme sur les niveaux) et
// allers-retours. Puis une passe de verification : feuilles rapatriees et comparees au vidage (compare.hpp : statut,
// grand livre, ensemble des feuilles), et une passe par mutant demande (tue s'il differe).
//
// Usage : mhgp12_traversal_bench --dump F.bin [--dump G.bin ...] [--reps 15] [--warmup 3] [--mutants none|all|a,b]
//                                [--json sortie.json] [--no-profile] [--nonce JETON]
// Le JSON repete le jeton --nonce du pilote (preuve fraiche de la session, juge du script) ; il est ecrit dans un
// temporaire puis renomme (jamais un fichier partiel sous le nom attendu).
// Codes : 0 identite partout, 1 ecart d'identite (sans mutant), 2 refus (arguments, vidage invalide, erreur CUDA,
//         ecriture du JSON impossible). Les reservations pendant les passes chronometrees sont publiees
//         (timed_allocations) et refusent le banc dans le juge du script.
#include <cuda_runtime.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "mhgp12/traversal/bfs.hpp"
#include "mhgp12/traversal/compare.hpp"
#include "mhgp12/traversal/driver.hpp"
#include "mhgp12/traversal/format.hpp"

namespace {

using namespace mhgp12::traversal;
namespace fmt = mhgp12::traversal::format;
using fmt::u32;
using fmt::u64;

#define MHGP12_CUDA(call)                                                                                 \
  do {                                                                                                    \
    const cudaError_t mhgp12_e_ = (call);                                                                 \
    if (mhgp12_e_ != cudaSuccess) {                                                                       \
      std::fprintf(stderr, "CUDA %s : %s (%s:%d)\n", #call, cudaGetErrorString(mhgp12_e_), __FILE__, __LINE__); \
      std::exit(2);                                                                                       \
    }                                                                                                     \
  } while (0)

constexpr int kWarpsPerBlock = 4;

// Noyau generique : un warp par indice, memoire partagee propre a chaque warp.
template <class K>
__global__ void __launch_bounds__(32 * kWarpsPerBlock) warp_kernel(K k, u64 n) {
  __shared__ typename K::Shared shared[kWarpsPerBlock];
  const u32 warp = threadIdx.x >> 5;
  const u64 w = u64(blockIdx.x) * kWarpsPerBlock + warp;
  if (w >= n) return;  // uniforme sur le warp
  k(w, shared[warp]);
}

struct DeviceBackend {
  cudaStream_t stream{};
  bfs::LevelTotals* pinned_totals = nullptr;
  bool profile = false;
  std::vector<cudaEvent_t> events;  // passe de profil : 4 jalons par niveau
  std::vector<std::pair<int, u32>> marks;
  size_t used_events = 0;

  template <class T>
  struct Array {
    T* ptr = nullptr;
    u64 cap = 0;
    T* data() { return ptr; }
  };

  void open() {
    MHGP12_CUDA(cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking));
    MHGP12_CUDA(cudaMallocHost(reinterpret_cast<void**>(&pinned_totals), sizeof(bfs::LevelTotals)));
  }
  void close() {
    for (auto e : events) cudaEventDestroy(e);
    if (pinned_totals != nullptr) cudaFreeHost(pinned_totals);
    if (stream != nullptr) cudaStreamDestroy(stream);
  }

  template <class T>
  u64 ensure(Array<T>& a, u64 n) {
    if (a.cap >= n) return 0;
    const u64 cap = std::max<u64>(n, a.cap + a.cap / 2 + 1024);
    if (a.ptr != nullptr) {
      MHGP12_CUDA(cudaStreamSynchronize(stream));
      MHGP12_CUDA(cudaFree(a.ptr));
    }
    MHGP12_CUDA(cudaMalloc(reinterpret_cast<void**>(&a.ptr), cap * sizeof(T)));
    a.cap = cap;
    return 1;
  }
  template <class T>
  u64 ensure_keep(Array<T>& a, u64 n, u64 keep) {
    if (a.cap >= n) return 0;
    const u64 cap = std::max<u64>(n, a.cap + a.cap / 2 + 1024);
    T* fresh = nullptr;
    MHGP12_CUDA(cudaMalloc(reinterpret_cast<void**>(&fresh), cap * sizeof(T)));
    if (a.ptr != nullptr) {
      if (keep != 0) MHGP12_CUDA(cudaMemcpyAsync(fresh, a.ptr, keep * sizeof(T), cudaMemcpyDeviceToDevice, stream));
      MHGP12_CUDA(cudaStreamSynchronize(stream));
      MHGP12_CUDA(cudaFree(a.ptr));
    }
    a.ptr = fresh;
    a.cap = cap;
    return 1;
  }
  template <class T>
  void release(Array<T>& a) {
    if (a.ptr != nullptr) cudaFree(a.ptr);
    a.ptr = nullptr;
    a.cap = 0;
  }
  template <class T>
  u64 upload(Array<T>& a, const T* src, u64 n) {
    const u64 r = ensure(a, n);
    if (n != 0) MHGP12_CUDA(cudaMemcpyAsync(a.ptr, src, n * sizeof(T), cudaMemcpyHostToDevice, stream));
    return r;
  }
  template <class K>
  void launch(const K& k, u64 warps) {
    if (warps == 0) return;
    const u64 blocks = (warps + kWarpsPerBlock - 1) / kWarpsPerBlock;
    warp_kernel<K><<<static_cast<unsigned>(blocks), 32 * kWarpsPerBlock, 0, stream>>>(k, warps);
    MHGP12_CUDA(cudaGetLastError());
  }
  bfs::LevelTotals read_totals(Array<bfs::LevelTotals>& a) {
    MHGP12_CUDA(cudaMemcpyAsync(pinned_totals, a.ptr, sizeof(bfs::LevelTotals), cudaMemcpyDeviceToHost, stream));
    MHGP12_CUDA(cudaStreamSynchronize(stream));
    return *pinned_totals;
  }
  void mark(int point, u32 depth) {
    if (!profile) return;
    if (used_events == events.size()) {
      cudaEvent_t e{};
      MHGP12_CUDA(cudaEventCreate(&e));
      events.push_back(e);
    }
    MHGP12_CUDA(cudaEventRecord(events[used_events++], stream));
    marks.emplace_back(point, depth);
  }
};

template <class T>
struct Pinned {
  T* ptr = nullptr;
  u64 count = 0;
  void allocate(u64 n) {
    count = n;
    MHGP12_CUDA(cudaMallocHost(reinterpret_cast<void**>(&ptr), std::max<u64>(n, 1) * sizeof(T)));
  }
  ~Pinned() {
    if (ptr != nullptr) cudaFreeHost(ptr);
  }
};

struct PassTimes {
  double upload = 0, levels = 0, download = 0, total = 0, resident = 0, wall = 0;
};

struct Profile {
  double kernels_ms = 0, readback_ms = 0;
  std::vector<std::array<double, 3>> levels;  // noyaux avant lecture, aller-retour, noyaux apres
};

double median(std::vector<double> v) {
  if (v.empty()) return 0;
  std::sort(v.begin(), v.end());
  const size_t n = v.size();
  return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

void json_string(std::ostream& o, const std::string& s) {
  o << '"';
  for (char c : s) {
    if (c == '"' || c == '\\') o << '\\' << c;
    else if (static_cast<unsigned char>(c) < 0x20) o << ' ';
    else o << c;
  }
  o << '"';
}

void ledger_json(std::ostream& o, const Ledger& l) {
  o << "{\"nodes\":" << l.nodes << ",\"leaves\":" << l.leaves << ",\"filter_tests\":" << l.filter_tests
    << ",\"max_depth\":" << l.max_depth << ",\"max_leaf\":" << l.max_leaf << "}";
}

struct Case {
  const fmt::Dump& d;
  Pinned<u32> x, y, z;
  Pinned<bfs::Leaf> leaves;
  Pinned<u32> sites;
  bfs::Params params;
  explicit Case(const fmt::Dump& dump) : d(dump) {
    const u64 n = d.header.n_sites;
    x.allocate(n);
    y.allocate(n);
    z.allocate(n);
    std::memcpy(x.ptr, d.x.data(), n * 4);
    std::memcpy(y.ptr, d.y.data(), n * 4);
    std::memcpy(z.ptr, d.z.data(), n * 4);
    // Capacite de rapatriement : celle de la reference, plus une marge (un ecart se voit aux comptes).
    leaves.allocate(d.header.n_leaves + d.header.n_leaves / 2 + 1024);
    sites.allocate(d.header.n_leaf_sites + d.header.n_leaf_sites / 2 + 4096);
    params.kmax = static_cast<u32>(d.header.kmax);
    params.leaf_size = static_cast<u32>(d.header.leaf_size);
    params.max_leaf = static_cast<u32>(d.header.max_leaf);
    params.coord_bits = static_cast<u32>(d.header.coord_bits);
  }
};

template <int M>
struct Runner {
  DeviceBackend b;
  Driver<DeviceBackend, M> drv;
  Case& c;
  cudaEvent_t e0{}, e1{}, e2{}, e3{};
  explicit Runner(Case& cs) : drv(b, cs.params), c(cs) {
    b.open();
    drv.keep_stats = true;
    MHGP12_CUDA(cudaEventCreate(&e0));
    MHGP12_CUDA(cudaEventCreate(&e1));
    MHGP12_CUDA(cudaEventCreate(&e2));
    MHGP12_CUDA(cudaEventCreate(&e3));
  }
  ~Runner() {
    cudaStreamSynchronize(b.stream);
    b.release(drv.x);
    b.release(drv.y);
    b.release(drv.z);
    for (int i = 0; i < 2; ++i) {
      b.release(drv.parents[i]);
      b.release(drv.task_begin[i]);
      b.release(drv.list[i]);
    }
    b.release(drv.chunk_top);
    b.release(drv.keep);
    b.release(drv.reservoir);
    b.release(drv.task_out);
    b.release(drv.child_out);
    b.release(drv.child_scan);
    b.release(drv.tile_sum);
    b.release(drv.tile_offset);
    b.release(drv.totals);
    b.release(drv.leaves);
    b.release(drv.leaf_sites);
    cudaEventDestroy(e0);
    cudaEventDestroy(e1);
    cudaEventDestroy(e2);
    cudaEventDestroy(e3);
    b.close();
  }
  // Une passe complete ; rapatrie les feuilles dans la memoire epinglee du cas.
  RunResult pass(PassTimes& t) {
    NoHook hook;
    const auto h0 = std::chrono::steady_clock::now();
    MHGP12_CUDA(cudaEventRecord(e0, b.stream));
    u64 allocations = 0;
    drv.upload_cloud(c.x.ptr, c.y.ptr, c.z.ptr, c.d.header.n_sites, allocations);
    MHGP12_CUDA(cudaEventRecord(e1, b.stream));
    RunResult r = drv.run(c.x.ptr, c.y.ptr, c.z.ptr, hook);
    r.allocations += allocations;
    MHGP12_CUDA(cudaEventRecord(e2, b.stream));
    const u64 nl = std::min<u64>(r.n_leaves, c.leaves.count), ns = std::min<u64>(r.n_leaf_sites, c.sites.count);
    if (nl != 0)
      MHGP12_CUDA(cudaMemcpyAsync(c.leaves.ptr, drv.leaves.ptr, nl * sizeof(bfs::Leaf), cudaMemcpyDeviceToHost, b.stream));
    if (ns != 0)
      MHGP12_CUDA(cudaMemcpyAsync(c.sites.ptr, drv.leaf_sites.ptr, ns * sizeof(u32), cudaMemcpyDeviceToHost, b.stream));
    MHGP12_CUDA(cudaEventRecord(e3, b.stream));
    MHGP12_CUDA(cudaEventSynchronize(e3));
    const auto h1 = std::chrono::steady_clock::now();
    float a = 0, l = 0, d = 0;
    MHGP12_CUDA(cudaEventElapsedTime(&a, e0, e1));
    MHGP12_CUDA(cudaEventElapsedTime(&l, e1, e2));
    MHGP12_CUDA(cudaEventElapsedTime(&d, e2, e3));
    t.upload = a;
    t.levels = l;
    t.download = d;
    t.total = double(a) + l + d;
    t.resident = double(a) + l;
    t.wall = std::chrono::duration<double, std::milli>(h1 - h0).count();
    return r;
  }
  Profile profile_pass() {
    b.profile = true;
    b.used_events = 0;
    b.marks.clear();
    PassTimes t;
    pass(t);
    b.profile = false;
    Profile p;
    // Jalons par niveau : 0 debut, 1 avant lecture, 2 apres lecture, 3 fin.
    for (size_t i = 0; i + 3 < b.marks.size(); i += 4) {
      float k1 = 0, rb = 0, k2 = 0;
      MHGP12_CUDA(cudaEventElapsedTime(&k1, b.events[i], b.events[i + 1]));
      MHGP12_CUDA(cudaEventElapsedTime(&rb, b.events[i + 1], b.events[i + 2]));
      MHGP12_CUDA(cudaEventElapsedTime(&k2, b.events[i + 2], b.events[i + 3]));
      p.levels.push_back({k1, rb, k2});
      p.kernels_ms += double(k1) + k2;
      p.readback_ms += rb;
    }
    return p;
  }
  Comparison verify(const RunResult& r) {
    LeafSet mine;
    mine.leaves = reinterpret_cast<const fmt::Leaf*>(c.leaves.ptr);
    mine.n = std::min<u64>(r.n_leaves, c.leaves.count);
    mine.sites = c.sites.ptr;
    mine.n_sites = std::min<u64>(r.n_leaf_sites, c.sites.count);
    Comparison cmp = compare(c.d, r.status, r.ledger, mine);
    if (mine.n != r.n_leaves || mine.n_sites != r.n_leaf_sites) {
      cmp.leaves_equal = false;
      if (cmp.first.empty()) cmp.first = "rapatriement tronque : plus de feuilles que la reference et sa marge";
    }
    return cmp;
  }
};

template <class K>
void kernel_info(std::ostream& o, const char* name, bool& first) {
  cudaFuncAttributes a{};
  MHGP12_CUDA(cudaFuncGetAttributes(&a, warp_kernel<K>));
  int blocks = 0;
  MHGP12_CUDA(cudaOccupancyMaxActiveBlocksPerMultiprocessor(&blocks, warp_kernel<K>, 32 * kWarpsPerBlock, 0));
  o << (first ? "" : ",") << '"' << name << "\":{\"registers\":" << a.numRegs << ",\"local_bytes\":" << a.localSizeBytes
    << ",\"shared_bytes\":" << a.sharedSizeBytes << ",\"blocks_per_sm\":" << blocks << "}";
  first = false;
}

struct MutantResult {
  int mutant = 0;
  bool killed = false;
  Comparison cmp;
};

template <int M>
MutantResult run_mutant(Case& c) {
  Runner<M> r(c);
  PassTimes t;
  const RunResult res = r.pass(t);
  MutantResult m;
  m.mutant = M;
  m.cmp = r.verify(res);
  m.killed = !m.cmp.identity();
  return m;
}

MutantResult dispatch_mutant(int m, Case& c) {
  switch (m) {
    case bfs::kLostWitness: return run_mutant<bfs::kLostWitness>(c);
    case bfs::kFrameChildBox: return run_mutant<bfs::kFrameChildBox>(c);
    case bfs::kUnstableCompaction: return run_mutant<bfs::kUnstableCompaction>(c);
    case bfs::kBisectOffByOne: return run_mutant<bfs::kBisectOffByOne>(c);
    case bfs::kTieReversed: return run_mutant<bfs::kTieReversed>(c);
    default: return run_mutant<bfs::kAxisLastMax>(c);
  }
}

}  // namespace

int main(int argc, char** argv) {
  std::vector<std::string> paths;
  std::vector<int> mutants;
  int reps = 15, warmup = 3;
  bool profile = true;
  std::string json_path, nonce;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--dump" && i + 1 < argc) paths.push_back(argv[++i]);
    else if (a == "--reps" && i + 1 < argc) reps = std::atoi(argv[++i]);
    else if (a == "--warmup" && i + 1 < argc) warmup = std::atoi(argv[++i]);
    else if (a == "--json" && i + 1 < argc) json_path = argv[++i];
    else if (a == "--nonce" && i + 1 < argc) {
      nonce = argv[++i];  // jeton de la session : caracteres controles, recopie tel quel dans le JSON
      if (nonce.empty() || nonce.size() > 64) return 2;
      for (char c : nonce)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-' || c == '_' ||
              c == '.'))
          return 2;
    }
    else if (a == "--no-profile") profile = false;
    else if (a == "--mutants" && i + 1 < argc) {
      const std::string list = argv[++i];
      mutants.clear();
      if (list == "all") {
        for (int m = 1; m < bfs::kMutantCount; ++m) mutants.push_back(m);
      } else if (list != "none") {
        std::stringstream ss(list);
        std::string name;
        while (std::getline(ss, name, ',')) {
          int found = -1;
          for (int m = 1; m < bfs::kMutantCount; ++m)
            if (name == bfs::mutant_name(m)) found = m;
          if (found < 0) return 2;
          mutants.push_back(found);
        }
      }
    } else {
      return 2;
    }
  }
  if (paths.empty() || reps < 1 || warmup < 0) return 2;
  int device = 0;
  MHGP12_CUDA(cudaGetDevice(&device));
  cudaDeviceProp prop{};
  MHGP12_CUDA(cudaGetDeviceProperties(&prop, device));
  const auto t0 = std::chrono::steady_clock::now();
  MHGP12_CUDA(cudaFree(nullptr));  // contexte
  const double context_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  std::ostringstream out;
  out << "{\"bench\":\"mhgp12_traversal_bench\",\"nonce\":";
  json_string(out, nonce);
  out << ",\"device\":";
  json_string(out, prop.name);
  out << ",\"sm\":" << prop.multiProcessorCount << ",\"cc\":\"" << prop.major << '.' << prop.minor
      << "\",\"context_ms\":" << context_ms << ",\"warps_per_block\":" << kWarpsPerBlock << ",\"chunk\":" << bfs::kChunk
      << ",\"reps\":" << reps << ",\"warmup\":" << warmup << ",\"kernels\":{";
  bool first = true;
  kernel_info<bfs::SelectKernel<bfs::kNone>>(out, "select", first);
  kernel_info<bfs::MergeKernel<bfs::kNone>>(out, "merge", first);
  kernel_info<bfs::FilterKernel<bfs::kNone>>(out, "filter", first);
  kernel_info<bfs::CloseKernel<bfs::kNone>>(out, "close", first);
  kernel_info<bfs::ScanAKernel>(out, "scan_a", first);
  kernel_info<bfs::ScanBKernel>(out, "scan_b", first);
  kernel_info<bfs::ScanCKernel>(out, "scan_c", first);
  kernel_info<bfs::ScatterKernel<bfs::kNone>>(out, "scatter", first);
  kernel_info<bfs::EmitKernel<bfs::kNone>>(out, "emit", first);
  out << "},\"cases\":[";
  bool all_ok = true;
  for (size_t p = 0; p < paths.size(); ++p) {
    fmt::Dump d;
    std::string error;
    if (!fmt::read(paths[p], d, error)) {
      std::cerr << error << '\n';
      return 2;
    }
    if (d.header.kmax > bfs::kMaxOrder) {
      std::cerr << "refus : K hors du parcours : " << paths[p] << '\n';
      return 2;
    }
    Case c(d);
    std::vector<PassTimes> times;
    RunResult last;
    Profile prof;
    Comparison cmp;
    u64 timed_allocations = 0, warm_allocations = 0;
    {
      Runner<bfs::kNone> r(c);
      for (int w = 0; w < warmup; ++w) {
        PassTimes t;
        warm_allocations += r.pass(t).allocations;
      }
      for (int k = 0; k < reps; ++k) {
        PassTimes t;
        last = r.pass(t);
        timed_allocations += last.allocations;
        times.push_back(t);
      }
      cmp = r.verify(last);  // feuilles de la derniere passe chronometree
      if (profile) prof = r.profile_pass();
      // Seconde verification apres le profil : passes repetees identiques.
      PassTimes t;
      const RunResult again = r.pass(t);
      const Comparison cmp2 = r.verify(again);
      if (!cmp2.identity() || cmp2.digest != cmp.digest) {
        cmp.leaves_equal = false;
        if (cmp.first.empty()) cmp.first = "passes repetees differentes";
      }
    }
    std::vector<MutantResult> mres;
    for (int m : mutants) mres.push_back(dispatch_mutant(m, c));
    const bool identity = cmp.identity();  // les reservations pendant le chrono sont jugees a part (refus du banc)
    all_ok = all_ok && identity;
    std::vector<double> total, resident, levels, upload, download, wall;
    for (const auto& t : times) {
      total.push_back(t.total);
      resident.push_back(t.resident);
      levels.push_back(t.levels);
      upload.push_back(t.upload);
      download.push_back(t.download);
      wall.push_back(t.wall);
    }
    out << (p ? "," : "") << "{\"dump\":";
    json_string(out, paths[p]);
    const fmt::Header& h = d.header;
    out << ",\"producer\":" << h.producer << ",\"coord_bits\":" << h.coord_bits << ",\"kmax\":" << h.kmax
        << ",\"leaf_size\":" << h.leaf_size << ",\"sites\":" << h.n_sites << ",\"identity\":"
        << (identity ? "true" : "false") << ",\"status\":" << last.status << ",\"ledger\":";
    ledger_json(out, last.ledger);
    out << ",\"leaves\":" << cmp.leaves << ",\"reference_leaves\":" << cmp.reference_leaves << ",\"digest\":\""
        << std::hex << cmp.digest << "\",\"reference_digest\":\"" << cmp.reference_digest << std::dec
        << "\",\"missing\":" << cmp.missing << ",\"extra\":" << cmp.extra << ",\"list_mismatch\":" << cmp.list_mismatch
        << ",\"meta_mismatch\":" << cmp.meta_mismatch << ",\"first\":";
    json_string(out, cmp.first);
    out << ",\"levels\":" << last.levels << ",\"warm_allocations\":" << warm_allocations
        << ",\"timed_allocations\":" << timed_allocations << ",\"median_ms\":{\"total\":" << median(total)
        << ",\"resident\":" << median(resident) << ",\"levels\":" << median(levels) << ",\"upload\":" << median(upload)
        << ",\"download\":" << median(download) << ",\"wall\":" << median(wall) << "},\"total_ms\":[";
    for (size_t i = 0; i < total.size(); ++i) out << (i ? "," : "") << total[i];
    out << "],\"resident_ms\":[";
    for (size_t i = 0; i < resident.size(); ++i) out << (i ? "," : "") << resident[i];
    out << "],\"wall_ms\":[";
    for (size_t i = 0; i < wall.size(); ++i) out << (i ? "," : "") << wall[i];
    out << "],\"profile\":{\"kernels_ms\":" << prof.kernels_ms << ",\"readback_ms\":" << prof.readback_ms
        << ",\"levels\":[";
    for (size_t i = 0; i < prof.levels.size(); ++i)
      out << (i ? "," : "") << "[" << prof.levels[i][0] << "," << prof.levels[i][1] << "," << prof.levels[i][2] << "]";
    out << "]},\"mutants\":{";
    for (size_t i = 0; i < mres.size(); ++i) {
      out << (i ? "," : "") << '"' << bfs::mutant_name(mres[i].mutant) << "\":{\"killed\":"
          << (mres[i].killed ? "true" : "false") << ",\"first\":";
      json_string(out, mres[i].cmp.first);
      out << "}";
    }
    out << "},\"level_stats\":[";
    for (size_t k = 0; k < last.stats.size(); ++k) {
      const auto& s = last.stats[k];
      out << (k ? "," : "") << "[" << s.depth << "," << s.parents << "," << s.children << "," << s.tasks << ","
          << s.candidates << "," << s.tests << "," << s.splits << "," << s.leaves << "]";
    }
    out << "]}";
    std::fprintf(stderr, "%s : total %.3f ms, resident %.3f ms, noyaux %.3f ms (%s)\n", paths[p].c_str(),
                 median(total), median(resident), prof.kernels_ms, identity ? "identite" : "ECART");
  }
  out << "],\"identity\":" << (all_ok ? "true" : "false") << "}\n";
  if (!json_path.empty()) {  // temporaire puis renommage : jamais un resultat partiel sous le nom attendu
    const std::string tmp = json_path + ".tmp";
    std::ofstream file(tmp, std::ios::binary | std::ios::trunc);
    file << out.str();
    file.flush();
    const bool written = file.good();
    file.close();
    if (!written || file.fail() || std::rename(tmp.c_str(), json_path.c_str()) != 0) {
      std::remove(tmp.c_str());
      std::fprintf(stderr, "ecriture impossible : %s\n", json_path.c_str());
      return 2;
    }
  } else {
    std::cout << out.str();
  }
  return all_ok ? 0 : 1;
}
