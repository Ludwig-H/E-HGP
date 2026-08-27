// MorseHGP3D v5 — porte du tri stable parallele (src/parallel/sort.hpp).
//
//   (a) egalite STRICTE avec std::stable_sort sur des entrees aleatoires
//       deterministes (mt19937 grave) de tailles 0, 1, 2, 1000, 2^16, 2^20,
//       cles a 8 valeurs distinctes + identifiant secondaire NON compare (toute
//       instabilite se voit), en u32 (permutation d'indices, comparateur
//       capturant un tableau de cles) et en FacetKey (44 octets, cle = k,
//       identite dans p[0]) ; fils 1, 2, 3, 8, 13 ; variantes a 2^16 : cles
//       toutes egales, entree deja triee, entree triee a rebours ;
//   (b) plancher : au moins un cas a cree >= 2 ouvriers ; invariants : jamais
//       plus d'ouvriers que de fils demandes, 1 ouvrier sous le seuil ;
//   (c) mutant `parallel-sort-unstable` (la fusion prend la droite d'abord en
//       cas d'egalite) tue : code 4 ;
//   (d) mesure INFORMATIVE (aucun claim, aucun seuil) : sequentiel contre
//       parallele sur 2^20 u32 et 2^20 FacetKey, paires ABBA intra-processus,
//       mediane des rapports par paire (jamais rapport de medianes).
// Codes : 0 conforme, 2 refus (argument ou mutant inconnu), 3 invariant ou
// plancher viole (ou mutant survivant), 4 mutant tue.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

#include "../src/lanes/keys.hpp"
#include "../src/parallel/sort.hpp"

using namespace mhgp5;

static_assert(sizeof(FacetKey) == 44, "FacetKey doit peser 44 octets");

namespace {

constexpr int kThreadsList[] = {1, 2, 3, 8, 13};
constexpr size_t kSizes[] = {0, 1, 2, 1000, (size_t)1 << 16, (size_t)1 << 20};
constexpr u32 kDistinctKeys = 8;

enum class Shape { kRandom, kAllEqual, kSorted, kReversed };

const char* shape_name(Shape s) {
  switch (s) {
    case Shape::kRandom: return "aleatoire";
    case Shape::kAllEqual: return "toutes_egales";
    case Shape::kSorted: return "triee";
    case Shape::kReversed: return "rebours";
  }
  return "?";
}

// Cles a 8 valeurs, dans l'ordre demande par la forme.
std::vector<u32> make_keys(size_t n, Shape shape, std::mt19937& rng) {
  std::vector<u32> keys(n);
  for (size_t i = 0; i < n; ++i) {
    switch (shape) {
      case Shape::kRandom: keys[i] = (u32)(rng() % kDistinctKeys); break;
      case Shape::kAllEqual: keys[i] = 3; break;
      case Shape::kSorted: keys[i] = (u32)((i * kDistinctKeys) / std::max<size_t>(1, n)); break;
      case Shape::kReversed: keys[i] = (u32)(kDistinctKeys - 1 - (i * kDistinctKeys) / std::max<size_t>(1, n)); break;
    }
  }
  return keys;
}

struct Outcome {
  bool equal = true;
  size_t workers = 0;
};

// Cas u32 : on trie une permutation d'indices par un comparateur qui capture le
// tableau des cles ; l'indice lui-meme est l'identite secondaire non comparee.
Outcome run_u32(size_t n, Shape shape, int threads, u32 seed, std::vector<u32>* ref_cache) {
  std::mt19937 rng(seed);
  const std::vector<u32> keys = make_keys(n, shape, rng);
  std::vector<u32> perm(n);
  for (size_t i = 0; i < n; ++i) perm[i] = (u32)i;
  if (shape == Shape::kRandom) std::shuffle(perm.begin(), perm.end(), rng);
  const auto less = [&keys](u32 x, u32 y) { return keys[x] < keys[y]; };
  if (ref_cache->size() != n || n == 0) {
    *ref_cache = perm;
    std::stable_sort(ref_cache->begin(), ref_cache->end(), less);
  }
  Outcome o;
  o.workers = parallel_stable_sort_vector(&perm, less, threads);
  o.equal = (perm == *ref_cache);
  return o;
}

// Cas FacetKey (44 octets) : cle = k dans {1..8}, identite = p[0] (jamais
// comparee), le reste de p est du bruit non compare.
Outcome run_facet(size_t n, Shape shape, int threads, u32 seed, std::vector<FacetKey>* ref_cache) {
  std::mt19937 rng(seed);
  const std::vector<u32> keys = make_keys(n, shape, rng);
  std::vector<FacetKey> v(n);
  for (size_t i = 0; i < n; ++i) {
    v[i].k = (u8)(1 + keys[i]);
    v[i].p[0] = (PointId)i;
    for (int j = 1; j < kFacetMaxK; ++j) v[i].p[j] = (PointId)rng();
  }
  const auto less = [](const FacetKey& x, const FacetKey& y) { return x.k < y.k; };
  if (ref_cache->size() != n || n == 0) {
    *ref_cache = v;
    std::stable_sort(ref_cache->begin(), ref_cache->end(), less);
  }
  Outcome o;
  o.workers = parallel_stable_sort_vector(&v, less, threads);
  o.equal = (v == *ref_cache);
  return o;
}

double median(std::vector<double> v) {
  std::sort(v.begin(), v.end());
  const size_t m = v.size() / 2;
  return (v.size() % 2 == 1) ? v[m] : 0.5 * (v[m - 1] + v[m]);
}

template <typename Fn>
double time_ms(Fn&& fn) {
  const auto t0 = std::chrono::steady_clock::now();
  fn();
  const auto t1 = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::milli>(t1 - t0).count();
}

// Banc apparie contrebalance : chaque paire = A B B A (A sequentiel, B
// parallele) ; on retient la mediane des rapports A/B par paire.
template <typename V, typename Less>
void bench(const char* label, const V& input, Less less, int threads, int pairs) {
  V work = input;
  // Echauffement (non compte).
  work = input;
  std::stable_sort(work.begin(), work.end(), less);
  work = input;
  parallel_stable_sort_vector(&work, less, threads);
  std::vector<double> ratios;
  double sum_seq = 0, sum_par = 0;
  for (int p = 0; p < pairs; ++p) {
    double seq = 0, par = 0;
    work = input;
    seq += time_ms([&] { std::stable_sort(work.begin(), work.end(), less); });
    work = input;
    par += time_ms([&] { parallel_stable_sort_vector(&work, less, threads); });
    work = input;
    par += time_ms([&] { parallel_stable_sort_vector(&work, less, threads); });
    work = input;
    seq += time_ms([&] { std::stable_sort(work.begin(), work.end(), less); });
    seq *= 0.5;
    par *= 0.5;
    sum_seq += seq;
    sum_par += par;
    ratios.push_back(seq / par);
    std::printf("  banc %s paire %d : sequentiel %.1f ms, parallele(%d fils) %.1f ms, rapport %.2f\n", label, p + 1, seq,
                threads, par, seq / par);
  }
  std::printf("banc %s (INFORMATIF, aucun claim) : %d paires ABBA, moyenne seq %.1f ms / par %.1f ms, "
              "mediane des rapports apparies %.2f\n",
              label, pairs, sum_seq / pairs, sum_par / pairs, median(ratios));
}

}  // namespace

int main(int argc, char** argv) {
  std::string inject;
  int bench_pairs = 3, bench_threads = 8;
  u32 seed = 3;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else if (arg.rfind("--bench-pairs=", 0) == 0) bench_pairs = std::atoi(arg.c_str() + 14);
    else if (arg.rfind("--bench-threads=", 0) == 0) bench_threads = std::atoi(arg.c_str() + 16);
    else if (arg.rfind("--seed=", 0) == 0) seed = (u32)std::atoi(arg.c_str() + 7);
    else return 2;
  }
  if (bench_pairs < 0 || bench_threads < 1) return 2;
  if (!inject.empty() && !mutants_enable(inject)) return 2;

  size_t cases = 0, mismatches = 0, max_workers = 0;
  bool invariant_ok = true;
  const auto record = [&](const char* type, size_t n, Shape shape, int threads, const Outcome& o) {
    ++cases;
    if (!o.equal) ++mismatches;
    max_workers = std::max(max_workers, o.workers);
    bool inv = o.workers >= 1 && o.workers <= (size_t)threads;
    if ((n < kParallelSortMinElems || threads <= 1) && o.workers != 1) inv = false;
    if (!inv) invariant_ok = false;
    std::printf("cas %-8s n=%-8zu forme=%-13s fils=%-2d ouvriers=%-2zu %s%s\n", type, n, shape_name(shape), threads,
                o.workers, o.equal ? "identique" : "DIFFERENT", inv ? "" : " INVARIANT-VIOLE");
  };

  for (const size_t n : kSizes) {
    std::vector<u32> ref_u32;
    std::vector<FacetKey> ref_facet;
    for (const int threads : kThreadsList) {
      record("u32", n, Shape::kRandom, threads, run_u32(n, Shape::kRandom, threads, seed, &ref_u32));
      record("facet44", n, Shape::kRandom, threads, run_facet(n, Shape::kRandom, threads, seed, &ref_facet));
    }
  }
  for (const Shape shape : {Shape::kAllEqual, Shape::kSorted, Shape::kReversed}) {
    const size_t n = (size_t)1 << 16;
    std::vector<u32> ref_u32;
    std::vector<FacetKey> ref_facet;
    for (const int threads : kThreadsList) {
      record("u32", n, shape, threads, run_u32(n, shape, threads, seed, &ref_u32));
      record("facet44", n, shape, threads, run_facet(n, shape, threads, seed, &ref_facet));
    }
  }
  std::printf("parallel_sort_gate : %zu cas, %zu divergences, ouvriers max %zu\n", cases, mismatches, max_workers);

  if (!invariant_ok) {
    std::fprintf(stderr, "INVARIANT : nombre d'ouvriers hors contrat\n");
    return 3;
  }
  if (!inject.empty()) {
    if (mismatches > 0) {
      std::fprintf(stderr, "MUTANT TUE : %s (%zu divergences)\n", inject.c_str(), mismatches);
      return 4;
    }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s survivant\n", inject.c_str());
    return 3;
  }
  if (mismatches > 0) {
    std::fprintf(stderr, "INVARIANT : la sortie differe de std::stable_sort\n");
    return 3;
  }
  if (max_workers < 2) {
    std::fprintf(stderr, "PORTE INEFFICACE : aucun parallelisme exerce\n");
    return 3;
  }

  if (bench_pairs > 0) {
    const size_t n = (size_t)1 << 20;
    std::mt19937 rng(seed);
    const std::vector<u32> keys = make_keys(n, Shape::kRandom, rng);
    std::vector<u32> perm(n);
    for (size_t i = 0; i < n; ++i) perm[i] = (u32)i;
    std::shuffle(perm.begin(), perm.end(), rng);
    bench("u32(2^20, indices, cles capturees)", perm, [&keys](u32 x, u32 y) { return keys[x] < keys[y]; },
          bench_threads, bench_pairs);
    std::vector<FacetKey> v(n);
    for (size_t i = 0; i < n; ++i) {
      v[i].k = (u8)(1 + keys[i]);
      v[i].p[0] = (PointId)i;
      for (int j = 1; j < kFacetMaxK; ++j) v[i].p[j] = (PointId)rng();
    }
    bench("facet44(2^20)", v, [](const FacetKey& x, const FacetKey& y) { return x.k < y.k; }, bench_threads,
          bench_pairs);
  }
  std::printf("parallel_sort_gate OK\n");
  return 0;
}
