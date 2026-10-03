// Permutation exacte, blocs fixes puis fusion par co-rangs ; aucune copie de gros Emission dans le tri.
// Inspiration critique : R2/777406b82 tris indirects et positions fixes. Pas de port du PSRS ni de ses vecteurs.
// Cles F3/F4 (ARCHITECTURE.md paragraphe 4) depuis le 3 octobre 2026 : une cle double par niveau, comparee avec
// marge prouvee ; sinon la comparaison exacte historique decide. La permutation est donc inchangee.
#include "catalogue/sort_indices.hpp"
#include "catalogue/internal.hpp"
#include "catalogue/sort_level_key.hpp"
#include "sched/sched.hpp"

namespace mhgp11::catalogue_detail {
namespace {

constexpr u64 kRun = 2048;
constexpr u64 kTile = 4096;
static_assert(kTile == 2 * kRun, "chaque tuile appartient a une seule paire de runs");

struct Less {
  std::span<const Emission> records;
  u64* comparisons;
  const double* keys;

  bool operator()(u32 a, u32 b) const noexcept {
    if (comparisons != nullptr) ++*comparisons;
    // Decision approchee seulement hors de la bande prouvee ; egalites et voisins proches restent exacts.
    const int key_order = level_key_order(keys[a], keys[b]);
    if (key_order != 0) return key_order < 0;
    const int order = num::compare(records[a].level, records[b].level);
    if (order != 0) return order < 0;
    const auto& left = records[a].ball.support;
    const auto& right = records[b].ball.support;
    return left != right ? left < right : a < b;
  }
};

void sift(std::span<u32> indices, u64 root, u64 count, const Less& less) noexcept {
  while (root < count / 2) {
    u64 child = 2 * root + 1;
    if (child + 1 < count && less(indices[child], indices[child + 1])) ++child;
    if (!less(indices[root], indices[child])) return;
    std::swap(indices[root], indices[child]);
    root = child;
  }
}

void heap_sort(std::span<u32> indices, const Less& less) noexcept {
  const u64 count = indices.size();
  if (count < 2) return;
  for (u64 i = count / 2; i > 0; --i) sift(indices, i - 1, count, less);
  for (u64 end = count; end > 1; --end) {
    std::swap(indices[0], indices[end - 1]);
    sift(indices, 0, end - 1, less);
  }
}

// i+j=rank ; A[i-1]<=B[j] et B[j-1]<A[i], les termes absents etant omis.
// Chaque frontiere decoupe le MEME merge stable global : aucune perte, repetition ou communication entre tuiles.
Result<u64> co_rank(std::span<const u32> a, std::span<const u32> b, u64 rank, const Less& less) noexcept {
  u64 low = rank > b.size() ? rank - b.size() : 0;
  u64 high = std::min<u64>(rank, a.size());
  while (low <= high) {
    const u64 i = low + (high - low) / 2;
    const u64 j = rank - i;
    if (i > 0 && j < b.size() && less(b[j], a[i - 1])) {
      high = i - 1;
    } else if (j > 0 && i < a.size() && !less(b[j - 1], a[i])) {
      low = i + 1;
    } else {
      return i;
    }
  }
  return fail(Reason::catalogue_invariant);
}

struct Sort {
  std::span<const Emission> records;
  std::span<const u32> input;
  std::span<u32> output;
  std::span<double> keys;
  std::array<u64, sched::kMaxWorkers> comparisons{};  // une case par worker, lue uniquement apres join
  u64 width = kRun;
  bool counting = false;

  static Outcome prepare(void* context, u64 begin, u64 end, u32) noexcept {
    auto& sort = *static_cast<Sort*>(context);
    for (u64 run = begin; run < end; ++run)
      for (u64 i = run * kRun; i < std::min<u64>((run + 1) * kRun, sort.records.size()); ++i)
        sort.keys[i] = level_key(sort.records[i].level);
    return {};
  }

  Outcome merge_tile(u64 tile, const Less& less) noexcept {
    const u64 start = tile * kTile;
    const u64 first = (start / (2 * width)) * (2 * width);
    const u64 middle = std::min<u64>(first + width, records.size());
    const u64 last = std::min<u64>(first + 2 * width, records.size());
    const u64 stop = std::min(start + kTile, last);
    const auto a = input.subspan(first, middle - first);
    const auto b = input.subspan(middle, last - middle);
    const auto begin_a = co_rank(a, b, start - first, less);
    if (!begin_a.ok()) return begin_a.outcome();
    const auto end_a = co_rank(a, b, stop - first, less);
    if (!end_a.ok()) return end_a.outcome();
    u64 i = begin_a.value(), j = start - first - i, at = start;
    const u64 end_i = end_a.value(), end_j = stop - first - end_i;
    while (i < end_i && j < end_j) output[at++] = less(b[j], a[i]) ? b[j++] : a[i++];
    while (i < end_i) output[at++] = a[i++];
    while (j < end_j) output[at++] = b[j++];
    return at == stop ? Outcome{} : fail(Reason::catalogue_invariant);
  }

  static Outcome initial(void* context, u64 begin, u64 end, u32 worker) noexcept {
    auto& sort = *static_cast<Sort*>(context);
    if (worker >= sched::kMaxWorkers) return fail(Reason::catalogue_invariant);
    u64 comparisons = 0;  // compteur sur pile : pas de faux partage a chaque comparaison
    const Less less{sort.records, sort.counting ? &comparisons : nullptr, sort.keys.data()};
    for (u64 run = begin; run < end; ++run) {
      const u64 first = run * kRun;
      const u64 last = std::min<u64>(first + kRun, sort.records.size());
      for (u64 i = first; i < last; ++i) sort.output[i] = static_cast<u32>(i);
      heap_sort(sort.output.subspan(first, last - first), less);
    }
    sort.comparisons[worker] += comparisons;
    return {};
  }

  static Outcome merging(void* context, u64 begin, u64 end, u32 worker) noexcept {
    auto& sort = *static_cast<Sort*>(context);
    if (worker >= sched::kMaxWorkers) return fail(Reason::catalogue_invariant);
    u64 comparisons = 0;
    const Less less{sort.records, sort.counting ? &comparisons : nullptr, sort.keys.data()};
    for (u64 tile = begin; tile < end; ++tile) MHGP11_TRY(sort.merge_tile(tile, less));
    sort.comparisons[worker] += comparisons;
    return {};
  }
};

Outcome execute(sched::Pool* pool, u64 tasks, Sort& sort, sched::Pool::Body body) noexcept {
  return pool == nullptr ? body(&sort, 0, tasks, 0) : pool->parallel_for(tasks, 1, &sort, body);
}

}  // namespace

Result<Buffer<u32>> sort_indices(std::span<const Emission> records, MemoryBudget& budget,
                                 sched::Pool* pool, u64* comparisons) noexcept {
  if (records.size() >= kNone) return fail(Reason::index_overflow_u32);
  Buffer<u32> current, alternate;
  if (records.empty()) {
    if (comparisons != nullptr) *comparisons = 0;
    return current;
  }
  u64 bytes = 0;
  MHGP11_TRY(add_bytes<u32>(bytes, 2 * u64(records.size())));
  MHGP11_TRY(add_bytes<double>(bytes, records.size()));
  MHGP11_TRY(budget.admit(bytes));
  MHGP11_TRY(current.allocate(records.size(), budget));
  MHGP11_TRY(alternate.allocate(records.size(), budget));
  Buffer<double> keys;  // rendu avec alternate, avant l'assemblage
  MHGP11_TRY(keys.allocate(records.size(), budget));
  Sort sort{records, {}, current.span(), keys.span()};
  sort.counting = comparisons != nullptr;
  MHGP11_TRY(execute(pool, (records.size() + kRun - 1) / kRun, sort, Sort::prepare));
  MHGP11_TRY(execute(pool, (records.size() + kRun - 1) / kRun, sort, Sort::initial));
  for (u64 width = kRun; width < records.size(); width *= 2) {
    sort.input = current.span();
    sort.output = alternate.span();
    sort.width = width;
    MHGP11_TRY(execute(pool, (records.size() + kTile - 1) / kTile, sort, Sort::merging));
    current.swap(alternate);
  }
  // Borne des comparaisons, N<2^32, L=ceil(log2 N). Tas initiaux <=4N*min(L,11).
  // L-11 etages si N>2048 : chacun <=N comparaisons de merge, plus <=132*ceil(N/4096)
  // pour les deux recherches par tuile (33 iterations, deux tests). Ce total est <2N<=4N.
  // Au total <=4NL<2^39 : chaque compteur de worker et leur somme tiennent sans saturation dans u64.
  u64 total = 0;
  for (u64 value : sort.comparisons) total += value;
  if (comparisons != nullptr) *comparisons = total;
  return current;  // alternate est rendu ; current reste reserve jusqu'a la fin de l'assemblage.
}

}  // namespace mhgp11::catalogue_detail
