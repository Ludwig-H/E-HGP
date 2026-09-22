#include "lanes/q4_family.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <vector>

namespace mhgp8 {
namespace {

using Vector3 = std::array<i64, 3>;
using Lifted = std::array<i64, 4>;

Vector3 difference(Point3 z, Point3 a) noexcept {
  return {static_cast<i64>(z.x) - a.x, static_cast<i64>(z.y) - a.y,
          static_cast<i64>(z.z) - a.z};
}

i64 dot(const Vector3& a, const Vector3& b) noexcept {
  // Differences are at most M=262143 in magnitude: |dot| <= 3*M^2<2^38.
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

Lifted lifted(Point3 z, Point3 a) noexcept {
  const auto v = difference(z, a);
  return {v[0], v[1], v[2], dot(v, v)};
}

i128 minor(const Lifted& a, const Lifted& b, std::size_t i, std::size_t j) noexcept {
  return static_cast<i128>(a[i]) * b[j] - static_cast<i128>(a[j]) * b[i];
}

std::array<i128, 6> minors(const Lifted& a, const Lifted& b) noexcept {
  return {minor(a, b, 0, 1), minor(a, b, 0, 2), minor(a, b, 0, 3),
          minor(a, b, 1, 2), minor(a, b, 1, 3), minor(a, b, 2, 3)};
}

int sign(i128 value) noexcept { return (value > 0) - (value < 0); }

u64 capacity_bytes(std::size_t capacity) {
  if (capacity > std::numeric_limits<u64>::max() / sizeof(std::size_t))
    throw std::overflow_error("mhgp8 q4 family vector bytes exceed u64");
  return static_cast<u64>(capacity) * sizeof(std::size_t);
}

}  // namespace

Q4FamilySeed::Q4FamilySeed(std::array<Point3, 3> points, std::array<i64, 3> normal,
    i128 gram, std::array<i128, 3> linear, std::array<i128, 6> prepared_minors) noexcept
    : points_(points), normal_(normal), gram_(gram), linear_(linear), minors_(prepared_minors) {}

std::optional<Q4FamilySeed> Q4FamilySeed::make(Point3 a, Point3 b, Point3 x) {
  require_valid_point(a);
  require_valid_point(b);
  require_valid_point(x);
  const auto d = difference(b, a);
  const auto u = difference(x, a);
  const i64 dd = dot(d, d);
  const i64 uu = dot(u, u);
  const i64 du = dot(d, u);
  // The three vertex scalar products are F, D-F and E-F.
  if (du <= 0 || dd - du <= 0 || uu - du <= 0) return std::nullopt;
  const i128 gram = static_cast<i128>(dd) * uu - static_cast<i128>(du) * du;
  if (gram <= 0) return std::nullopt;
  const Vector3 normal{d[1] * u[2] - d[2] * u[1],
                       d[2] * u[0] - d[0] * u[2],
                       d[0] * u[1] - d[1] * u[0]};
  // W=E(D-F)d+D(E-F)u. Promote BEFORE each product, not after evaluation.
  const i128 along_d = static_cast<i128>(uu) * (dd - du);
  const i128 along_u = static_cast<i128>(dd) * (uu - du);
  std::array<i128, 3> linear{};
  for (std::size_t axis = 0; axis < 3; ++axis)
    linear[axis] = along_d * d[axis] + along_u * u[axis];
  const Lifted dl{d[0], d[1], d[2], dd};
  const Lifted ul{u[0], u[1], u[2], uu};
  return Q4FamilySeed({a, b, x}, normal, gram, linear, minors(dl, ul));
}

i128 Q4FamilySeed::power(Point3 z) const noexcept {
  const auto v = difference(z, points_[0]);
  // G <= 12*M^4, |W_i| <= 36*M^5 suffice even before using acuteness.
  // Thus |G*|v|^2-W.v| <= 144*M^6 < 2^116 < 2^117; intermediates fit i128.
  i128 result = gram_ * dot(v, v);
  for (std::size_t axis = 0; axis < 3; ++axis) result -= linear_[axis] * v[axis];
  return result;
}

i64 Q4FamilySeed::side(Point3 z) const noexcept {
  const auto v = difference(z, points_[0]);
  // Each normal component <=2*M^2: |B| <=6*M^3 <2^57, including partial sums.
  return normal_[0] * v[0] + normal_[1] * v[1] + normal_[2] * v[2];
}

int Q4FamilySeed::compare_roots(Point3 z1, Point3 z2) const {
  const auto b1 = side(z1);
  const auto b2 = side(z2);
  if (b1 == 0 || b2 == 0)
    throw std::invalid_argument("mhgp8 q4 root comparison requires two noncoplanar sites");
  const auto m = minors(lifted(z1, points_[0]), lifted(z2, points_[0]));
  // Laplace expansion in the first two rows of [d,D; u,E; v1,V1; v2,V2].
  // The 24 determinant monomials have total absolute value <=72*M^5<2^97,
  // so both the six products and every partial sum fit i128. In contrast,
  // computing P1*B2-P2*B1 directly would not be justified in i128.
  const i128 delta = minors_[0] * m[5] - minors_[1] * m[4] + minors_[2] * m[3]
                   + minors_[3] * m[2] - minors_[4] * m[1] + minors_[5] * m[0];
  // G*delta=P2*B1-P1*B2 and G>0; both denominator signs are essential.
  return -sign(delta) * sign(b1) * sign(b2);
}

Q4FamilyWork run_q4_family(CloudPtr cloud, std::array<std::size_t, 3> seed_ids,
                          const Q4FamilyConsumer& consumer) {
  if (!cloud || !consumer)
    throw std::invalid_argument("mhgp8 q4 family requires a cloud and valid callback");
  const auto points = cloud->points();
  for (const auto id : seed_ids) {
    if (id >= points.size()) throw std::out_of_range("mhgp8 q4 seed ID is outside the cloud");
  }
  if (seed_ids[0] == seed_ids[1] || seed_ids[0] == seed_ids[2] || seed_ids[1] == seed_ids[2])
    throw std::invalid_argument("mhgp8 q4 seed requires three distinct IDs");
  const auto seed = Q4FamilySeed::make(points[seed_ids[0]], points[seed_ids[1]], points[seed_ids[2]]);
  if (!seed) throw std::invalid_argument("mhgp8 q4 family requires a strictly acute seed");

  Q4FamilyWork work;
  std::vector<std::size_t> events;
  std::vector<std::size_t> constant_shell;
  // Two per-family ID buffers, no coordinate copies and no per-root payload.
  // Reserve before scanning/emitting; both capacities are reported, even when
  // one class is empty. This first scalar primitive does not tile the segment.
  events.reserve(points.size());
  constant_shell.reserve(points.size());
  std::size_t inside = 0;
  for (std::size_t id = 0; id < points.size(); ++id) {
    counter_add(work.sites);
    const auto b = seed->side(points[id]);
    if (b != 0) {
      events.push_back(id);
      counter_add(work.event_count);
      if (b > 0) counter_add(work.entries);
      else {
        counter_add(work.exits);
        ++inside;  // Interior at mu=-infinity.
      }
    } else {
      const auto p = seed->power(points[id]);
      if (p < 0) {
        counter_add(work.constant_inside);
        ++inside;
      } else if (p == 0) {
        counter_add(work.constant_on);
        constant_shell.push_back(id);
      } else {
        counter_add(work.constant_outside);
      }
    }
  }
  work.retained_capacity_bytes = capacity_bytes(events.capacity());
  counter_add(work.retained_capacity_bytes, capacity_bytes(constant_shell.capacity()));
  std::sort(events.begin(), events.end(), [&](std::size_t left, std::size_t right) {
    counter_add(work.sort_comparisons);
    const int comparison = seed->compare_roots(points[left], points[right]);
    return comparison < 0 || (comparison == 0 && left < right);
  });

  std::size_t first = 0;
  while (first < events.size()) {
    std::size_t last = first + 1;
    while (last < events.size()) {
      counter_add(work.group_comparisons);
      if (seed->compare_roots(points[events[first]], points[events[last]]) != 0) break;
      ++last;
    }
    std::size_t entries = 0;
    std::size_t exits = 0;
    for (auto position = first; position < last; ++position) {
      if (seed->side(points[events[position]]) > 0) ++entries;
      else ++exits;
    }
    if (exits > inside)
      throw std::logic_error("mhgp8 q4 family exit count exceeds the active interior");
    inside -= exits;
    counter_add(work.groups);
    work.max_group = std::max(work.max_group, static_cast<u64>(last - first));
    consumer(Q4FamilyGroup{events[first], inside,
        std::span<const std::size_t>(events).subspan(first, last - first), constant_shell});
    counter_add(work.callbacks);
    if (entries > points.size() - inside)
      throw std::logic_error("mhgp8 q4 family entry count exceeds the cloud population");
    inside += entries;
    first = last;
  }
  if (inside != work.constant_inside + work.entries)
    throw std::logic_error("mhgp8 q4 family final population does not match its entries");
  return work;
}

}  // namespace mhgp8
