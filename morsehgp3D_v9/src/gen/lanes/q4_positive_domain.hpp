#pragma once

#include "lanes/edge_cover.hpp"

#include <cstddef>
#include <memory>
#include <optional>

namespace mhgp9::gen {

struct Q4PositiveDomainWork {
  u64 node_visits{};
  // One pair of min/max squared-distance bounds for EACH endpoint, per
  // non-singleton. This counter therefore represents four scalar bounds.
  u64 bound_tests{};
  // Two endpoint/box containment queries only for a geometrically admitted
  // non-singleton. A possible endpoint forces refinement, not subtraction.
  u64 endpoint_box_tests{};
  u64 endpoint_leaf_tests{};  // One endpoint-ID membership query per leaf.
  u64 point_tests{};          // Both closed-ball predicates, non-endpoint leaf.
  u64 admitted_nodes{};
  u64 rejected_nodes{};
  u64 split_nodes{};
  u64 excluded_endpoints{};
  u64 admitted_sites{};
  u64 rejected_sites{};
  u64 box_merges{};  // admitted_nodes-1 if nonempty, otherwise zero.
  bool operator==(const Q4PositiveDomainWork&) const = default;
};

class Q4PositiveDomain;
using Q4PositiveDomainPtr = std::shared_ptr<const Q4PositiveDomain>;

// Preparation of the positive-q4 center-domain over ONE existing edge cover.
// Let D=|b-a|^2 and Z={z != a,b : |z-a|^2 <= D and |z-b|^2 <= D}.
// completion_box() is the EXACT coordinate AABB of Z, including obtuse
// completions and all lens boundary ties. It is empty iff Z is empty.
// It is NOT restricted to acute/canonical seeds and NOT a witness population:
// witnesses outside this lens may be strictly inside a candidate q4 ball.
//
// For positive q4 owned by this edge, projecting the eight AABB corners into
// (b-a)^perp and adding zero gives a safe center-domain overcoverage. That
// projection/hull is the consumer's separate operation. A degenerate hull
// must not be interpreted as an empty domain. Fewer than two completions
// rules out q4, but says nothing about q3.
//
// Immutable ownership is shared through cover()->index(); no cloud, ID list,
// range list or index is copied. Certified node boxes are admitted/rejected
// as blocks, refining only ambiguous geometry or possible endpoint nodes.
// O(visited nodes), at most O(n) for ONE edge, and O(1) fixed object storage;
// no traversal stack or systematic scalar scan of n sites is allocated.
// This is not a global subquadratic bound over all edges.
class Q4PositiveDomain final {
 public:
  // Null cover: invalid_argument, before any traversal. Other invalid edge
  // IDs/coordinates are already excluded by the immutable cover's factory.
  // Allocation/counter failures propagate; no partial object is published.
  [[nodiscard]] static Q4PositiveDomainPtr make(Q34EdgeCoverPtr cover);

  Q4PositiveDomain(const Q4PositiveDomain&) = delete;
  Q4PositiveDomain& operator=(const Q4PositiveDomain&) = delete;
  Q4PositiveDomain(Q4PositiveDomain&&) = delete;
  Q4PositiveDomain& operator=(Q4PositiveDomain&&) = delete;

  [[nodiscard]] const Q34EdgeCoverPtr& cover() const noexcept { return cover_; }
  [[nodiscard]] const std::optional<Box3>& completion_box() const noexcept { return box_; }
  [[nodiscard]] std::size_t completion_count() const noexcept { return population_; }
  [[nodiscard]] const Q4PositiveDomainWork& work() const noexcept { return work_; }

 private:
  explicit Q4PositiveDomain(Q34EdgeCoverPtr cover);
  void build();
  void admit(const Box3& box, std::size_t population);
  void reject(std::size_t population);

  Q34EdgeCoverPtr cover_;
  std::optional<Box3> box_;
  std::size_t population_{};
  Point3 a_, b_;
  i64 diameter_squared_{};
  Q4PositiveDomainWork work_{};
};

}  // namespace mhgp9::gen
