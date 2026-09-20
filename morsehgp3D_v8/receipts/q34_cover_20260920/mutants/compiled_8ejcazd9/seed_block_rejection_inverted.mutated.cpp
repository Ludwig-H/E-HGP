#include "lanes/q34_cover.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace mhgp8 {
namespace {

i64 distance_squared(Point3 a, Point3 b) noexcept {
  i64 value = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    const i64 delta = static_cast<i64>(a[axis]) - b[axis];
    value += delta * delta;
  }
  return value;
}

bool acute(Point3 a, Point3 b, Point3 x) noexcept {
  const auto ab = distance_squared(a, b), ax = distance_squared(a, x);
  const auto bx = distance_squared(b, x);
  return ab + ax > bx && ab + bx > ax && ax + bx > ab;
}

bool owned(std::span<const Point3> points, std::span<const std::size_t> ids,
           u64& tests) {
  const auto key = [](std::size_t a, std::size_t b) {
    return std::pair{std::min(a, b), std::max(a, b)};
  };
  const auto proposed = key(ids[0], ids[1]);
  const auto length = distance_squared(points[ids[0]], points[ids[1]]);
  for (std::size_t i = 0; i < ids.size(); ++i) {
    for (std::size_t j = i + 1; j < ids.size(); ++j) {
      if (i == 0 && j == 1) continue;
      counter_add(tests);
      const auto other = distance_squared(points[ids[i]], points[ids[j]]);
      if (other > length || (other == length && key(ids[i], ids[j]) < proposed))
        return false;
    }
  }
  return true;
}

u64 bytes(std::size_t capacity) {
  if (capacity > std::numeric_limits<u64>::max() / sizeof(std::size_t))
    throw std::overflow_error("mhgp8 covered candidate buffer exceeds u64");
  return static_cast<u64>(capacity) * sizeof(std::size_t);
}

void merge(Q34CoverSeedWork& dst, const Q34CoverSeedWork& src) {
#define MHGP8_ADD(field) counter_add(dst.field, src.field)
  MHGP8_ADD(site_reads);
  MHGP8_ADD(q3_shell_sort_comparisons);
  MHGP8_ADD(q4_shell_sort_comparisons);
  MHGP8_ADD(seed.seed_owner_tests);
  MHGP8_ADD(seed.seed_owner_rejections);
  MHGP8_ADD(seed.q3_point_tests);
  MHGP8_ADD(seed.q3_shell_ids);
  MHGP8_ADD(seed.q3_depth_rejections);
  MHGP8_ADD(seed.q3_emitted);
  MHGP8_ADD(seed.q4_depth_rejected_groups);
  MHGP8_ADD(seed.q4_depth_skipped_ids);
  MHGP8_ADD(seed.q4_presentations);
  MHGP8_ADD(seed.q4_owner_tests);
  MHGP8_ADD(seed.q4_owner_rejections);
  MHGP8_ADD(seed.q4_positive_tests);
  MHGP8_ADD(seed.q4_positive_rejections);
  MHGP8_ADD(seed.q4_seed_tests);
  MHGP8_ADD(seed.q4_seed_rejections);
  MHGP8_ADD(seed.q4_groups_without_support);
  MHGP8_ADD(seed.q4_unexamined_after_emit);
  MHGP8_ADD(seed.q4_emitted);
  MHGP8_ADD(seed.family.sites);
  MHGP8_ADD(seed.family.entries);
  MHGP8_ADD(seed.family.exits);
  MHGP8_ADD(seed.family.constant_inside);
  MHGP8_ADD(seed.family.constant_on);
  MHGP8_ADD(seed.family.constant_outside);
  MHGP8_ADD(seed.family.sort_comparisons);
  MHGP8_ADD(seed.family.group_comparisons);
  MHGP8_ADD(seed.family.groups);
  MHGP8_ADD(seed.family.callbacks);
  MHGP8_ADD(seed.family.event_count);
#undef MHGP8_ADD
  dst.peak_buffer_bytes = std::max(dst.peak_buffer_bytes, src.peak_buffer_bytes);
  dst.seed.q3_shell_capacity_bytes = std::max(
      dst.seed.q3_shell_capacity_bytes, src.seed.q3_shell_capacity_bytes);
  dst.seed.family.retained_capacity_bytes = std::max(
      dst.seed.family.retained_capacity_bytes, src.seed.family.retained_capacity_bytes);
  dst.seed.family.max_group = std::max(dst.seed.family.max_group, src.seed.family.max_group);
}

}  // namespace

Q34CoverSeedWork run_q34_cover_seed_candidates(Q34EdgeCoverPtr cover,
    std::size_t x_id, std::size_t kmax, const Q34SeedConsumer& consumer) {
  if (!cover || !consumer || kmax == 0)
    throw std::invalid_argument("mhgp8 covered seed requires cover, callback and positive Kmax");
  const auto points = cover->index()->cloud().points();
  if (x_id >= points.size()) throw std::out_of_range("mhgp8 covered seed ID outside cloud");
  const auto edge = cover->edge_ids();
  const std::array<std::size_t, 3> ids{edge[0], edge[1], x_id};
  const auto a = points[edge[0]], b = points[edge[1]], x = points[x_id];
  const auto face = ExactBall::make_q3({a, b, x});
  const auto family = Q4FamilySeed::make(a, b, x);
  if (!face || !family) throw std::invalid_argument("mhgp8 covered seed must be strictly acute");
  Q34CoverSeedWork result{};
  auto& work = result.seed;
  if (!owned(points, ids, work.seed_owner_tests)) {
    counter_add(work.seed_owner_rejections);
    return result;
  }
  if (kmax < 2) return result;

  std::vector<std::size_t> q3_shell, events, constant_shell;
  if (kmax >= 3) {
    events.reserve(cover->site_count());
    constant_shell.reserve(cover->site_count());
  }
  std::size_t q3_depth = 0, inside = 0;
  const auto order = cover->index()->spatial_order();
  for (const auto range : cover->ranges()) {
    for (auto rank = range.first; rank < range.last; ++rank) {
      const auto id = order[rank];
      counter_add(result.site_reads);
      counter_add(work.q3_point_tests);
      const auto power = family->power(points[id]);
      if (power < 0) ++q3_depth;
      else if (power == 0) {
        q3_shell.push_back(id);
        counter_add(work.q3_shell_ids);
      }
      if (kmax < 3) continue;
      counter_add(work.family.sites);
      const auto side = family->side(points[id]);
      if (side != 0) {
        events.push_back(id);
        counter_add(work.family.event_count);
        if (side > 0) counter_add(work.family.entries);
        else {
          counter_add(work.family.exits);
          ++inside;
        }
      } else if (power < 0) {
        counter_add(work.family.constant_inside);
        ++inside;
      } else if (power == 0) {
        counter_add(work.family.constant_on);
        constant_shell.push_back(id);
      } else counter_add(work.family.constant_outside);
    }
  }
  work.q3_shell_capacity_bytes = bytes(q3_shell.capacity());
  work.family.retained_capacity_bytes = bytes(events.capacity());
  counter_add(work.family.retained_capacity_bytes, bytes(constant_shell.capacity()));
  result.peak_buffer_bytes = work.q3_shell_capacity_bytes;
  counter_add(result.peak_buffer_bytes, work.family.retained_capacity_bytes);
  if (q3_depth >= kmax - 1) counter_add(work.q3_depth_rejections);
  else {
    std::sort(q3_shell.begin(), q3_shell.end(), [&](auto left, auto right) {
      counter_add(result.q3_shell_sort_comparisons);
      return left < right;
    });
    std::array<std::size_t, 4> support{edge[0], edge[1], x_id,
                                     std::numeric_limits<std::size_t>::max()};
    std::sort(support.begin(), support.begin() + 3);
    consumer(Q34SeedCandidate{3, support, *face, q3_depth, q3_shell, {}});
    counter_add(work.q3_emitted);
  }
  // q3 power-zero sites are NOT necessarily constant q4 shell sites: points
  // off the seed plane may lie on the q3 sphere. Keep the buffers distinct.
  std::vector<std::size_t>().swap(q3_shell);
  if (kmax < 3) return result;
  std::sort(constant_shell.begin(), constant_shell.end(), [&](auto left, auto right) {
    counter_add(result.q4_shell_sort_comparisons);
    return left < right;
  });
  std::sort(events.begin(), events.end(), [&](auto left, auto right) {
    counter_add(work.family.sort_comparisons);
    const auto comparison = family->compare_roots(points[left], points[right]);
    return comparison < 0 || (comparison == 0 && left < right);
  });

  std::size_t first = 0;
  while (first < events.size()) {
    std::size_t last = first + 1;
    while (last < events.size()) {
      counter_add(work.family.group_comparisons);
      if (family->compare_roots(points[events[first]], points[events[last]]) != 0) break;
      ++last;
    }
    std::size_t entries = 0, exits = 0;
    for (auto pos = first; pos < last; ++pos) {
      if (family->side(points[events[pos]]) > 0) ++entries;
      else ++exits;
    }
    if (exits > inside) throw std::logic_error("mhgp8 covered sweep exit underflow");
    inside -= exits;
    counter_add(work.family.groups);
    counter_add(work.family.callbacks);  // Internal positive-candidate cascade only.
    work.family.max_group = std::max(work.family.max_group, static_cast<u64>(last - first));
    if (inside >= kmax - 2) {
      counter_add(work.q4_depth_rejected_groups);
      counter_add(work.q4_depth_skipped_ids, static_cast<u64>(last - first));
    } else {
      bool emitted = false;
      for (auto pos = first; pos < last; ++pos) {
        const auto y_id = events[pos];
        counter_add(work.q4_presentations);
        std::array<std::size_t, 4> support{edge[0], edge[1], x_id, y_id};
        if (!owned(points, support, work.q4_owner_tests)) {
          counter_add(work.q4_owner_rejections);
          continue;
        }
        counter_add(work.q4_positive_tests);
        const auto ball = ExactBall::make_q4({a, b, x, points[y_id]});
        if (!ball) {
          counter_add(work.q4_positive_rejections);
          continue;
        }
        counter_add(work.q4_seed_tests);
        if (y_id < x_id && acute(a, b, points[y_id])) {
          counter_add(work.q4_seed_rejections);
          continue;
        }
        // Only here is 'inside' a certified GLOBAL depth: positive ball,
        // longest owner edge => entire closed ball lies in the edge cover.
        std::sort(support.begin(), support.end());
        consumer(Q34SeedCandidate{4, support, *ball, inside,
            std::span<const std::size_t>(events).subspan(first, last - first), constant_shell});
        counter_add(work.q4_emitted);
        counter_add(work.q4_unexamined_after_emit, static_cast<u64>(last - pos - 1));
        emitted = true;
        break;
      }
      if (!emitted) counter_add(work.q4_groups_without_support);
    }
    if (entries > cover->site_count() - inside)
      throw std::logic_error("mhgp8 covered sweep entry overflow");
    inside += entries;
    first = last;
  }
  if (inside != work.family.constant_inside + work.family.entries)
    throw std::logic_error("mhgp8 covered sweep final population mismatch");
  return result;
}

Q34EdgeWork run_q34_edge_candidates(Q34EdgeCoverPtr cover, std::size_t kmax,
                                   const Q34SeedConsumer& consumer) {
  if (!cover || !consumer || kmax == 0)
    throw std::invalid_argument("mhgp8 edge producer requires cover, callback and positive Kmax");
  Q34EdgeWork work{};
  if (kmax < 2) return work;
  const auto points = cover->index()->cloud().points();
  const auto nodes = cover->index()->spatial_nodes();
  const auto order = cover->index()->spatial_order();
  const auto edge = cover->edge_ids();
  const auto a = points[edge[0]], b = points[edge[1]];
  const auto diameter = distance_squared(a, b);
  std::size_t cursor = 0;
  while (cursor < nodes.size()) {
    const auto& node = nodes[cursor];
    counter_add(work.node_visits);
    if (node.range.size() == 1) {
      counter_add(work.point_tests);
      const auto id = order[node.range.first];
      if (acute(a, b, points[id])) {
        counter_add(work.acute_seeds);
        const std::array<std::size_t, 3> ids{edge[0], edge[1], id};
        if (owned(points, ids, work.owner_tests)) {
          counter_add(work.seeds);
          merge(work.covered, run_q34_cover_seed_candidates(cover, id, kmax, consumer));
        } else counter_add(work.owner_rejections);
      }
      cursor = node.escape;
      continue;
    }
    counter_add(work.bound_tests);
    i64 min_a = 0, min_b = 0, max_sum = 0;
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const i64 low = node.box.low[axis], high = node.box.high[axis];
      const i64 al = low - a[axis], ah = high - a[axis];
      const i64 bl = low - b[axis], bh = high - b[axis];
      const i64 near_a = al > 0 ? al : (ah < 0 ? ah : 0);
      const i64 near_b = bl > 0 ? bl : (bh < 0 ? bh : 0);
      min_a += near_a * near_a;
      min_b += near_b * near_b;
      // A convex sum of two squares reaches its interval maximum at an end.
      max_sum += std::max(al * al + bl * bl, ah * ah + bh * bh);
    }
    // Each quantity <=6*65535^2<2^35. Necessary conditions: both distances
    // <=D and their squared sum>D². Equal-length owner ties remain leaf tests.
    if (min_a > diameter || min_b > diameter || max_sum >= diameter) {
      counter_add(work.rejected_nodes);
      counter_add(work.rejected_sites, static_cast<u64>(node.range.size()));
      cursor = node.escape;
    } else {
      counter_add(work.split_nodes);
      cursor = node.left;
    }
  }
  return work;
}

}  // namespace mhgp8
