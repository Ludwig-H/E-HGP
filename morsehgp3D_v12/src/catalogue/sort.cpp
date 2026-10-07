// Tri exact d'une permutation des boules : blocs de kRun tries par tas, puis fusions de paires de blocs par tuiles
// independantes (co-rangs) ; chaque tuile decoupe le MEME merge stable global, donc la permutation ne depend pas du
// nombre de fils. Port explicite de src/catalogue/sort_indices.cpp de la v11 (ac081a06f), comparateur v12.
#include "catalogue/sort.hpp"

#include <cmath>

#include "sched/sched.hpp"

namespace mhgp12::catalogue_detail {
namespace {

constexpr u64 kRun = 2048;
constexpr u64 kTile = 4096;
static_assert(kTile == 2 * kRun, "chaque tuile appartient a une seule paire de runs");
constexpr double kOrdered = 1.0 - 0x1p-40;  // F4 : c <= (1-u)^(Ex+Ey+1), Ex = Ey = 6

template <int Words>
double magnitude_key(const num::Wide<Words>& value) noexcept {
  const int length = value.bit_length();
  if (length <= 64) return static_cast<double>(value.words[0]);
  const int shift = length - 64, word = shift / 64, bit = shift % 64;
  u64 top = value.words[word] >> bit;
  if (bit != 0) top |= value.words[word + 1] << (64 - bit);  // bit != 0 implique word + 1 < Words
  return std::ldexp(static_cast<double>(top), shift);
}

struct Less {
  const Cloud* cloud;
  std::span<const BallRecord> records;
  std::span<const num::Level> levels;
  const double* keys;
  bool operator()(u32 a, u32 b) const noexcept {
    const int key_order = level_key_order(keys[a], keys[b]);
    if (key_order != 0) return key_order < 0;
    const int order = num::compare(levels[a], levels[b]);
    if (order != 0) return order < 0;
    const int support = compare_support_positions(*cloud, records[a].support, records[b].support);
    return support != 0 ? support < 0 : a < b;  // egalite : doublon, refuse par le parcours des rangs
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

// i + j = rank ; A[i-1] <= B[j] et B[j-1] < A[i], les termes absents etant omis.
Result<u64> co_rank(std::span<const u32> a, std::span<const u32> b, u64 rank, const Less& less) noexcept {
  u64 low = rank > b.size() ? rank - b.size() : 0;
  u64 high = rank < a.size() ? rank : a.size();
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
  Less less;
  u64 size;
  std::span<const u32> input;
  std::span<u32> output;
  u64 width = kRun;

  Outcome merge_tile(u64 tile) noexcept {
    const u64 start = tile * kTile;
    const u64 first = (start / (2 * width)) * (2 * width);
    const u64 middle = first + width < size ? first + width : size;
    const u64 last = first + 2 * width < size ? first + 2 * width : size;
    const u64 stop = start + kTile < last ? start + kTile : last;
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

  static Outcome initial(void* context, u64 begin, u64 end, u32) noexcept {
    auto& sort = *static_cast<Sort*>(context);
    for (u64 run = begin; run < end; ++run) {
      const u64 first = run * kRun;
      const u64 last = first + kRun < sort.size ? first + kRun : sort.size;
      for (u64 i = first; i < last; ++i) sort.output[i] = static_cast<u32>(i);
      heap_sort(sort.output.subspan(first, last - first), sort.less);
    }
    return {};
  }

  static Outcome merging(void* context, u64 begin, u64 end, u32) noexcept {
    auto& sort = *static_cast<Sort*>(context);
    for (u64 tile = begin; tile < end; ++tile) MHGP12_TRY(sort.merge_tile(tile));
    return {};
  }
};

}  // namespace

double level_key(const num::Level& level) noexcept {
  return magnitude_key(num::to_wide(level.numerator())) / magnitude_key(num::to_wide(level.denominator()));
}

int level_key_order(double a, double b) noexcept {
  if (a == 0 || b == 0) return (a > b) - (a < b);
  if (a < kOrdered * b) return -1;
  if (b < kOrdered * a) return 1;
  return 0;
}

namespace {
using Position = std::array<u32, 3>;
// Positions d'un support (2 a 4 sites, kNone au-dela), triees par ordre lexicographique (insertion).
u32 sorted_positions(const Cloud& cloud, const u32 (&s)[4], std::array<Position, 4>& out) noexcept {
  u32 n = 0;
  for (u32 k = 0; k < 4; ++k) {
    if (s[k] == kNone) continue;
    const Position p{cloud.x()[s[k]], cloud.y()[s[k]], cloud.z()[s[k]]};
    u32 at = n++;
    while (at > 0 && p < out[at - 1]) {
      out[at] = out[at - 1];
      --at;
    }
    out[at] = p;
  }
  return n;
}
}  // namespace

int compare_support_positions(const Cloud& cloud, const u32 (&a)[4], const u32 (&b)[4]) noexcept {
  std::array<Position, 4> pa{}, pb{};
  const u32 na = sorted_positions(cloud, a, pa), nb = sorted_positions(cloud, b, pb);
  for (u32 k = 0; k < na && k < nb; ++k)
    if (pa[k] != pb[k]) return pa[k] < pb[k] ? -1 : 1;
  return (na > nb) - (na < nb);
}

Result<Buffer<u32>> sort_balls(const Cloud& cloud, std::span<const BallRecord> records,
                               std::span<const num::Level> levels, std::span<const double> keys,
                               MemoryBudget& budget, sched::Pool& pool) noexcept {
  const u64 n = records.size();
  if (n >= kNone) return fail(Reason::index_overflow_u32);
  Buffer<u32> current, alternate;
  if (n == 0) return current;
  MHGP12_TRY(current.allocate(n, budget));
  MHGP12_TRY(alternate.allocate(n, budget));
  Sort sort{Less{&cloud, records, levels, keys.data()}, n, {}, current.span()};
  MHGP12_TRY(pool.parallel_for((n + kRun - 1) / kRun, 1, &sort, &Sort::initial));
  for (u64 width = kRun; width < n; width *= 2) {
    sort.input = current.span();
    sort.output = alternate.span();
    sort.width = width;
    MHGP12_TRY(pool.parallel_for((n + kTile - 1) / kTile, 1, &sort, &Sort::merging));
    current.swap(alternate);
  }
  return current;
}

}  // namespace mhgp12::catalogue_detail
