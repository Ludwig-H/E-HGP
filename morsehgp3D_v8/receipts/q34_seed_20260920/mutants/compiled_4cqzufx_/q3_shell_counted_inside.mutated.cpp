#include "lanes/q34_seed.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mhgp8 {
namespace {

i64 distance_squared(Point3 a, Point3 b) noexcept {
  const i64 x = static_cast<i64>(a.x) - b.x;
  const i64 y = static_cast<i64>(a.y) - b.y;
  const i64 z = static_cast<i64>(a.z) - b.z;
  return x * x + y * y + z * z;  // <=3*65535^2 <2^34.
}

auto edge_key(std::size_t a, std::size_t b) noexcept {
  return std::pair{std::min(a, b), std::max(a, b)};
}

bool owned(std::span<const Point3> points, std::span<const std::size_t> ids,
           u64& tests) {
  const auto proposed = edge_key(ids[0], ids[1]);
  const auto length = distance_squared(points[ids[0]], points[ids[1]]);
  for (std::size_t i = 0; i < ids.size(); ++i) {
    for (std::size_t j = i + 1; j < ids.size(); ++j) {
      if (i == 0 && j == 1) continue;
      counter_add(tests);
      const auto other = distance_squared(points[ids[i]], points[ids[j]]);
      if (other > length || (other == length && edge_key(ids[i], ids[j]) < proposed))
        return false;
    }
  }
  return true;
}

bool acute(Point3 a, Point3 b, Point3 x) noexcept {
  const i64 ab = distance_squared(a, b);
  const i64 ax = distance_squared(a, x);
  const i64 bx = distance_squared(b, x);
  // Strict triangle squared-length inequalities are equivalent to the three
  // positive vertex scalar products. Sums <2^35 fit i64, without floating point.
  return ab + ax > bx && ab + bx > ax && ax + bx > ab;
}

}  // namespace

Q34SeedWork run_q34_seed_candidates(CloudPtr cloud,
    std::array<std::size_t, 3> seed_ids, std::size_t kmax,
    const Q34SeedConsumer& consumer) {
  if (!cloud || !consumer || kmax == 0)
    throw std::invalid_argument("mhgp8 q34 seed requires cloud, callback and positive Kmax");
  const auto points = cloud->points();
  for (const auto id : seed_ids) {
    if (id >= points.size()) throw std::out_of_range("mhgp8 q34 seed ID outside cloud");
  }
  const auto a = points[seed_ids[0]], b = points[seed_ids[1]], x = points[seed_ids[2]];
  const auto face = ExactBall::make_q3({a, b, x});
  if (!face) throw std::invalid_argument("mhgp8 q34 seed must be strictly acute");
  Q34SeedWork work{};
  if (!owned(points, seed_ids, work.seed_owner_tests)) {
    counter_add(work.seed_owner_rejections);
    return work;
  }
  if (kmax >= 2) {
    // One scan of this seed's q3 ball, not one census per q4 completion.
    // Saturation is safe here only because this scan never removes credits.
    std::size_t depth = 0;
    std::vector<std::size_t> shell;
    for (std::size_t id = 0; id < points.size(); ++id) {
      counter_add(work.q3_point_tests);
      const auto power = face->power(points[id]);
      if (power <= 0) {  // MUTANT: shell credited as strict interior
        ++depth;
        if (depth == kmax - 1) break;
      } else if (power == 0) {
        shell.push_back(id);
        counter_add(work.q3_shell_ids);
      }
    }
    if (shell.capacity() > std::numeric_limits<u64>::max() / sizeof(std::size_t))
      throw std::overflow_error("mhgp8 q3 shell storage exceeds u64");
    work.q3_shell_capacity_bytes = static_cast<u64>(shell.capacity()) * sizeof(std::size_t);
    if (depth == kmax - 1) {
      counter_add(work.q3_depth_rejections);
    } else {
      std::array<std::size_t, 4> support{seed_ids[0], seed_ids[1], seed_ids[2],
                                       std::numeric_limits<std::size_t>::max()};
      std::sort(support.begin(), support.begin() + 3);
      consumer(Q34SeedCandidate{3, support, *face, depth, shell, {}});
      counter_add(work.q3_emitted);
    }
  }  // Release the q3-only buffer before allocating the q4 event segment.
  if (kmax < 3) return work;

  work.family = run_q4_family(cloud, seed_ids, [&](const Q4FamilyGroup& group) {
    if (group.depth >= kmax - 2) {
      counter_add(work.q4_depth_rejected_groups);
      counter_add(work.q4_depth_skipped_ids, static_cast<u64>(group.root_ids.size()));
      return;
    }
    for (std::size_t position = 0; position < group.root_ids.size(); ++position) {
      const auto y_id = group.root_ids[position];
      const auto y = points[y_id];
      counter_add(work.q4_presentations);
      std::array<std::size_t, 4> support{seed_ids[0], seed_ids[1], seed_ids[2], y_id};
      if (!owned(points, support, work.q4_owner_tests)) {
        counter_add(work.q4_owner_rejections);
        continue;
      }
      counter_add(work.q4_positive_tests);
      const auto ball = ExactBall::make_q4({a, b, x, y});
      if (!ball) {
        counter_add(work.q4_positive_rejections);
        continue;
      }
      counter_add(work.q4_seed_tests);
      if (y_id < seed_ids[2] && acute(a, b, y)) {
        counter_add(work.q4_seed_rejections);
        continue;
      }
      std::sort(support.begin(), support.end());
      consumer(Q34SeedCandidate{4, support, *ball, group.depth,
                               group.root_ids, group.constant_shell});
      counter_add(work.q4_emitted);
      counter_add(work.q4_unexamined_after_emit,
                  static_cast<u64>(group.root_ids.size() - position - 1));
      return;
    }
    counter_add(work.q4_groups_without_support);
  });
  return work;
}

}  // namespace mhgp8
