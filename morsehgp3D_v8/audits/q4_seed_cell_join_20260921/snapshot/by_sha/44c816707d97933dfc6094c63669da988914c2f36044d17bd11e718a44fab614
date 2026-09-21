#pragma once

#include "core/types.hpp"
#include "pipeline/prepared_cloud.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <span>

namespace mhgp8 {

// Exact, oriented one-parameter family through a STRICTLY acute triangle.
// If d=b-a, u=x-a, n=d cross u and G=|n|^2, its strict interiors satisfy
// power(z) - mu*side(z) < 0. This is a family primitive, not a positive q4
// support certificate, a bounded chord, a canonical ball or an HGP tower.
// The private immutable coefficients and copied coordinates have no borrowed
// aliases. No v7 implementation or qualification is inherited here.
class Q4FamilySeed final {
 public:
  // Right, obtuse, collinear and repeated-point triangles return nullopt.
  [[nodiscard]] static std::optional<Q4FamilySeed> make(Point3 a, Point3 b, Point3 x);

  [[nodiscard]] i128 power(Point3 z) const noexcept;
  [[nodiscard]] i64 side(Point3 z) const noexcept;
  // Compare the rational roots P(z1)/B(z1) and P(z2)/B(z2). Both B must be
  // nonzero, otherwise invalid_argument. Returns exactly -1, 0 or +1.
  // No P1*B2 product is formed: six prepared minors remove the common G.
  [[nodiscard]] int compare_roots(Point3 z1, Point3 z2) const;
  [[nodiscard]] i128 gram() const noexcept { return gram_; }
  [[nodiscard]] const std::array<Point3, 3>& points() const noexcept { return points_; }

 private:
  Q4FamilySeed(std::array<Point3, 3> points, std::array<i64, 3> normal,
               i128 gram, std::array<i128, 3> linear,
               std::array<i128, 6> minors) noexcept;
  const std::array<Point3, 3> points_;
  const std::array<i64, 3> normal_;
  const i128 gram_;
  const std::array<i128, 3> linear_;
  // Minors of rows (d,|d|^2), (u,|u|^2), in columns 01,02,03,12,13,23.
  const std::array<i128, 6> minors_;
};

struct Q4FamilyGroup {
  std::size_t representative_id{};  // Smallest original ID of this root.
  std::size_t depth{};              // Unsaturated STRICT interior population.
  std::span<const std::size_t> root_ids;        // Sorted original IDs, B != 0.
  std::span<const std::size_t> constant_shell;  // Sorted original IDs, B=P=0.
};

struct Q4FamilyWork {
  u64 sites{}, entries{}, exits{};
  u64 constant_inside{}, constant_on{}, constant_outside{};
  u64 sort_comparisons{}, group_comparisons{};
  u64 groups{}, max_group{}, callbacks{}, event_count{};
  // Capacities of the two owned ID vectors only, measured before their
  // destruction. Excludes cloud, seed, stack, metadata and callback storage.
  u64 retained_capacity_bytes{};
};

using Q4FamilyConsumer = std::function<void(const Q4FamilyGroup&)>;

// Scan ALL original cloud sites for one supplied acute seed, sort exact roots
// (then IDs), group equality and sweep: remove this group's exits, report its
// depth, then add its entries. No K saturation, chord/lens filtering, ownership
// rule, positive tetrahedron test or per-group interior-ID list is implied.
// The shell of a family member is exactly root_ids union constant_shell;
// constant_shell is shared by views, never copied once per group.
// With no noncoplanar site there is no event and therefore no callback. This
// entry does not emit the seed's mu=0/q3 ball unless it is an actual event.
//
// Null cloud/callback, repeated seed IDs and a nonacute seed are rejected with
// invalid_argument; out-of-range IDs with out_of_range. All input validation
// precedes emissions. This synchronous call owns the cloud for its duration.
// Callbacks must copy any retained views before returning: both spans expire
// at callback exit. A callback/allocation/counter exception propagates; earlier
// emissions are not undone and there is no successful partial work report.
// Per-family work is O(n + e log(1+e)), retained IDs O(n), where e<=n. Neither
// the number of seeds nor total pipeline work is bounded by this statement.
[[nodiscard]] Q4FamilyWork run_q4_family(
    CloudPtr cloud, std::array<std::size_t, 3> seed_ids,
    const Q4FamilyConsumer& consumer);

}  // namespace mhgp8
