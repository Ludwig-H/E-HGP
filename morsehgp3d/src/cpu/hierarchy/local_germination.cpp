#include "morsehgp3d/hierarchy/local_germination.hpp"

#include "morsehgp3d/exact/point.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {

namespace {

using spatial::PointId;

struct Vec3 {
  double x{};
  double y{};
  double z{};
};

[[nodiscard]] Vec3 operator-(const Vec3& left, const Vec3& right) noexcept {
  return Vec3{left.x - right.x, left.y - right.y, left.z - right.z};
}

[[nodiscard]] Vec3 operator+(const Vec3& left, const Vec3& right) noexcept {
  return Vec3{left.x + right.x, left.y + right.y, left.z + right.z};
}

[[nodiscard]] Vec3 operator*(const Vec3& value, double scale) noexcept {
  return Vec3{value.x * scale, value.y * scale, value.z * scale};
}

[[nodiscard]] double dot(const Vec3& left, const Vec3& right) noexcept {
  return left.x * right.x + left.y * right.y + left.z * right.z;
}

[[nodiscard]] Vec3 cross(const Vec3& left, const Vec3& right) noexcept {
  return Vec3{
      left.y * right.z - left.z * right.y,
      left.z * right.x - left.x * right.z,
      left.x * right.y - left.y * right.x};
}

[[nodiscard]] double norm(const Vec3& value) noexcept {
  return std::sqrt(dot(value, value));
}

[[nodiscard]] Vec3 point_of(
    const spatial::CanonicalPointCloud& cloud, PointId id) {
  const exact::CertifiedPoint3& point = cloud.point(id);
  return Vec3{
      point.binary64_coordinate(0U),
      point.binary64_coordinate(1U),
      point.binary64_coordinate(2U)};
}

// Jung's constant squared, as an exact rational: 1/3 for a planar triangle,
// 3/8 for a tetrahedron.  Kept squared because every comparison is on squares,
// and kept rational because that is what makes the certificate checkable.
[[nodiscard]] std::pair<std::uint64_t, std::uint64_t> theorem_jung_squared(
    std::size_t support_size) {
  return support_size == 3U ? std::pair<std::uint64_t, std::uint64_t>{1U, 3U}
                            : std::pair<std::uint64_t, std::uint64_t>{3U, 8U};
}

// A uniform grid over the binary64 points, local to this reference and built
// once per run.
//
// It is an ACCELERATION, not a decision: it changes which points are looked at,
// never which are counted, because a ball is compared against the cells it
// actually meets and every point of those cells is then tested exactly as
// before.  The product path does not use it -- a device implementation queries
// the LBVH -- but it lets this reference reach the sizes where the certified
// restriction begins to bite, which is where its selectivity can be seen at all.
class UniformGrid {
 public:
  UniformGrid(const std::vector<Vec3>& points, double cell)
      : cell_(cell) {
    origin_ = points.front();
    Vec3 upper = points.front();
    for (const Vec3& point : points) {
      origin_.x = std::min(origin_.x, point.x);
      origin_.y = std::min(origin_.y, point.y);
      origin_.z = std::min(origin_.z, point.z);
      upper.x = std::max(upper.x, point.x);
      upper.y = std::max(upper.y, point.y);
      upper.z = std::max(upper.z, point.z);
    }
    const auto span = [&](double low, double high) {
      const double extent = high - low;
      const double count = std::floor(extent / cell_) + 1.0;
      return static_cast<std::size_t>(std::max(1.0, count));
    };
    dims_[0] = span(origin_.x, upper.x);
    dims_[1] = span(origin_.y, upper.y);
    dims_[2] = span(origin_.z, upper.z);
    const std::size_t cells = dims_[0] * dims_[1] * dims_[2];
    std::vector<std::size_t> counts(cells + 1U, 0U);
    std::vector<std::size_t> cell_of;
    cell_of.reserve(points.size());
    for (const Vec3& point : points) {
      const std::size_t index = linear(cell_index(point));
      cell_of.push_back(index);
      ++counts[index + 1U];
    }
    for (std::size_t index = 0U; index < cells; ++index) {
      counts[index + 1U] += counts[index];
    }
    starts_ = counts;
    items_.assign(points.size(), 0U);
    std::vector<std::size_t> cursor = counts;
    for (std::size_t point = 0U; point < points.size(); ++point) {
      items_[cursor[cell_of[point]]++] = point;
    }
  }

  // Calls `visit(point_index)` for every point of every cell the ball meets.
  template <typename Visitor>
  void for_each_candidate(
      const Vec3& centre, double radius, Visitor&& visit) const {
    std::array<std::size_t, 3> low{};
    std::array<std::size_t, 3> high{};
    const double coordinates[3] = {centre.x, centre.y, centre.z};
    const double origins[3] = {origin_.x, origin_.y, origin_.z};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const double relative_low = (coordinates[axis] - radius - origins[axis]) / cell_;
      const double relative_high = (coordinates[axis] + radius - origins[axis]) / cell_;
      const double clamped_low = std::max(0.0, std::floor(relative_low));
      const double clamped_high = std::min(
          static_cast<double>(dims_[axis] - 1U), std::floor(relative_high));
      if (clamped_high < clamped_low) {
        return;
      }
      low[axis] = static_cast<std::size_t>(clamped_low);
      high[axis] = static_cast<std::size_t>(clamped_high);
    }
    for (std::size_t x = low[0]; x <= high[0]; ++x) {
      for (std::size_t y = low[1]; y <= high[1]; ++y) {
        for (std::size_t z = low[2]; z <= high[2]; ++z) {
          const std::size_t index = linear({x, y, z});
          for (std::size_t slot = starts_[index]; slot < starts_[index + 1U];
               ++slot) {
            visit(items_[slot]);
          }
        }
      }
    }
  }

 private:
  [[nodiscard]] std::array<std::size_t, 3> cell_index(
      const Vec3& point) const noexcept {
    const double coordinates[3] = {point.x, point.y, point.z};
    const double origins[3] = {origin_.x, origin_.y, origin_.z};
    std::array<std::size_t, 3> cell{};
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      const double relative = std::floor((coordinates[axis] - origins[axis]) / cell_);
      const double clamped =
          std::min(std::max(0.0, relative), static_cast<double>(dims_[axis] - 1U));
      cell[axis] = static_cast<std::size_t>(clamped);
    }
    return cell;
  }

  [[nodiscard]] std::size_t linear(
      const std::array<std::size_t, 3>& cell) const noexcept {
    return (cell[0] * dims_[1] + cell[1]) * dims_[2] + cell[2];
  }

  Vec3 origin_{};
  double cell_{};
  std::array<std::size_t, 3> dims_{};
  std::vector<std::size_t> starts_;
  std::vector<std::size_t> items_;
};

// Counts the points PROVED strictly inside the ball, stopping as soon as the
// cap is exceeded.  A point whose squared distance is within the margin of the
// radius is skipped: under-counting is safe, since the caller only ever uses
// "the count exceeds s_max" as a reason to reject.
[[nodiscard]] bool population_exceeds(
    const std::vector<Vec3>& points,
    const UniformGrid& grid,
    const Vec3& centre,
    double radius,
    std::size_t cap,
    double margin) {
  if (radius <= 0.0) {
    return false;
  }
  const double inner = radius - margin;
  if (inner <= 0.0) {
    return false;
  }
  const double inner_squared = inner * inner;
  std::size_t count = 0U;
  bool exceeded = false;
  grid.for_each_candidate(centre, inner, [&](std::size_t point) {
    if (exceeded) {
      return;
    }
    const Vec3 offset = points[point] - centre;
    if (dot(offset, offset) < inner_squared) {
      ++count;
      if (count > cap) {
        exceeded = true;
      }
    }
  });
  return exceeded;
}

// An orthonormal pair spanning the plane perpendicular to `axis`.
void perpendicular_basis(const Vec3& axis, Vec3& first, Vec3& second) {
  const Vec3 seed = std::fabs(axis.x) < 0.9 ? Vec3{1.0, 0.0, 0.0}
                                            : Vec3{0.0, 1.0, 0.0};
  first = cross(axis, seed);
  const double first_norm = norm(first);
  first = first * (1.0 / first_norm);
  second = cross(axis, first);
  second = second * (1.0 / norm(second));
}

// Hexagonal covering of the unit disc: the centre, then `rings` rings whose
// spacing gives a covering radius of 1 / (rings + 1/2) once scaled.
struct DiscCover {
  std::vector<std::pair<double, double>> offsets;  // in units of the radius
  double covering_radius{};                        // in units of the radius
};

[[nodiscard]] DiscCover build_disc_cover(std::size_t rings) {
  DiscCover cover;
  cover.offsets.emplace_back(0.0, 0.0);
  if (rings == 0U) {
    cover.covering_radius = 1.0;
    return cover;
  }
  const double step = 1.0 / static_cast<double>(rings);
  for (std::size_t ring = 1U; ring <= rings; ++ring) {
    const double radius = step * static_cast<double>(ring);
    const std::size_t count = 6U * ring;
    for (std::size_t index = 0U; index < count; ++index) {
      const double angle = 2.0 * std::numbers::pi *
          static_cast<double>(index) / static_cast<double>(count);
      cover.offsets.emplace_back(radius * std::cos(angle),
                                 radius * std::sin(angle));
    }
  }
  // A ring spacing of `step` and an angular spacing of 2 pi / (6 ring) leave
  // every point of the disc within `step` of a sample, which is the bound used
  // below; it is deliberately conservative.
  cover.covering_radius = step;
  return cover;
}


// The direction sets whose covering radius is sealed with a proof: the face,
// vertex and edge normals of the cube.
[[nodiscard]] std::vector<Vec3> cube_directions(std::size_t count) {
  std::vector<Vec3> directions;
  const auto push = [&](double x, double y, double z) {
    const double length = std::sqrt(x * x + y * y + z * z);
    directions.push_back(Vec3{x / length, y / length, z / length});
  };
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    for (const double sign : {1.0, -1.0}) {
      push(axis == 0U ? sign : 0.0, axis == 1U ? sign : 0.0,
           axis == 2U ? sign : 0.0);
    }
  }
  if (count >= 14U) {
    for (const double x : {1.0, -1.0}) {
      for (const double y : {1.0, -1.0}) {
        for (const double z : {1.0, -1.0}) {
          push(x, y, z);
        }
      }
    }
  }
  if (count >= 26U) {
    for (const double a : {1.0, -1.0}) {
      for (const double b : {1.0, -1.0}) {
        push(0.0, a, b);
        push(a, 0.0, b);
        push(a, b, 0.0);
      }
    }
  }
  return directions;
}

struct AxisAlignedBox {
  Vec3 lower{};
  Vec3 upper{};
};

// Squared distance from a point to the box, zero inside.
[[nodiscard]] double squared_distance_to_box(
    const AxisAlignedBox& box, const Vec3& point) noexcept {
  const auto axis_gap = [](double value, double low, double high) {
    if (value < low) {
      return low - value;
    }
    if (value > high) {
      return value - high;
    }
    return 0.0;
  };
  const double dx = axis_gap(point.x, box.lower.x, box.upper.x);
  const double dy = axis_gap(point.y, box.lower.y, box.upper.y);
  const double dz = axis_gap(point.z, box.lower.z, box.upper.z);
  return dx * dx + dy * dy + dz * dz;
}

// Binary64 proposal for the geometrically certified upper bound on R(p).
//
// A radius is ADMISSIBLE when some direction still permits a ball through p of
// that radius holding at most s_max points.  For a direction covering a cap of
// half-angle theta, the cap's image lies in the ball of radius rho sin(theta)
// around p + rho cos(theta) u, so a cap whose image misses the convex region is
// dead; and the offset ball of radius rho (1 - sin theta) around the same centre
// is inside every tangent ball of the cap, so an over-populated offset ball
// kills the whole cap.
//
// Admissibility is a DOWN-SET in rho -- tangent balls in a fixed direction are
// nested increasing, and the region is convex and contains p -- so a bisection
// converges, and returning the upper end of the bracket gives an upper bound.
[[nodiscard]] double certified_tangent_radius_bound(
    const std::vector<Vec3>& points,
    const UniformGrid& grid,
    const std::vector<Vec3>& directions,
    const AxisAlignedBox& box,
    const Vec3& p,
    std::size_t cap,
    double covering_radius_radians,
    double ceiling,
    std::size_t& population_queries) {
  const double sine = std::sin(covering_radius_radians);
  const double cosine = std::cos(covering_radius_radians);
  const double factor = 1.0 - sine;
  if (factor <= 0.0) {
    return ceiling;
  }
  const auto admissible = [&](double radius) {
    for (const Vec3& direction : directions) {
      const Vec3 centre = p + direction * (radius * cosine);
      const double reach = radius * sine;
      if (squared_distance_to_box(box, centre) > reach * reach) {
        continue;
      }
      ++population_queries;
      if (!population_exceeds(points, grid, centre, radius * factor, cap, 0.0)) {
        return true;
      }
    }
    return false;
  };
  if (admissible(ceiling)) {
    return ceiling;
  }
  double low = 0.0;
  double high = ceiling;
  for (int step = 0; step < 40; ++step) {
    const double middle = 0.5 * (low + high);
    if (admissible(middle)) {
      low = middle;
    } else {
      high = middle;
    }
  }
  return high;
}

}  // namespace

std::size_t sealed_tangent_covering_radius_millidegrees(
    std::size_t direction_count) noexcept {
  // Exact covering radii of the cube's direction sets, each rounded UP so the
  // sealed value is never below the truth.  Their derivation is recorded in
  // docs/math/OPTIMISATIONS_JUNG_SUPPORTS_3_4.md section 8.5: the sets are
  // invariant under the full octahedral group, so it suffices to minimise the
  // largest cosine on the fundamental domain x >= y >= z >= 0, where it is the
  // maximum of three linear forms, and the minimum is the interior
  // equioscillation point -- the three domain boundaries all give strictly
  // larger values.
  switch (direction_count) {
    case 6U:
      // arccos(1/sqrt(3)) = 54.735610 degrees.
      return 54736U;
    case 14U:
      // arccos(1/sqrt(5 - 2 sqrt(3))) = 36.206023 degrees.
      return 36207U;
    case 26U:
      // arccos(1/sqrt(9 - 2 sqrt(2) - 2 sqrt(6))) = 27.569276 degrees.
      return 27570U;
    default:
      return 0U;
  }
}

bool local_germination_certificate_admissible(
    const LocalGerminationCertificate& certificate, std::string& reason) {
  reason.clear();
  // This verifier certifies a generator invocation that returned.  Lifecycle
  // placeholders (whether non-applicable or merely not executed yet) are
  // honest stream state, but they are not completeness certificates.
  if (!certificate.applicable) {
    reason = "certificate_not_applicable";
    return false;
  }
  if (!certificate.executed) {
    reason = "certificate_not_executed";
    return false;
  }
  if (certificate.proof_basis != local_germination_proof_basis) {
    reason = "proof_basis";
    return false;
  }
  if (certificate.support_size != 3U && certificate.support_size != 4U) {
    reason = "support_size";
    return false;
  }
  if (certificate.maximum_relevant_closed_rank < certificate.support_size) {
    reason = "maximum_relevant_closed_rank";
    return false;
  }
  if (certificate.jung_squared_denominator == 0U) {
    reason = "jung_squared_denominator";
    return false;
  }
  // The declared constant must be AT LEAST the theorem's.  A larger one only
  // enlarges the locus and shrinks the test balls, so it weakens rejection and
  // stays complete; a smaller one would move the locus and could lose an
  // accepted support.  Compared by cross-multiplication, exactly.
  const auto theorem = theorem_jung_squared(certificate.support_size);
  const __uint128_t left =
      static_cast<__uint128_t>(certificate.jung_squared_numerator) *
      static_cast<__uint128_t>(theorem.second);
  const __uint128_t right = static_cast<__uint128_t>(theorem.first) *
      static_cast<__uint128_t>(certificate.jung_squared_denominator);
  if (left < right) {
    reason = "jung_squared_below_the_theorem";
    return false;
  }
  // The disc of centres must be real: gamma^2 must exceed one quarter.
  if (static_cast<__uint128_t>(4U) *
          static_cast<__uint128_t>(certificate.jung_squared_numerator) <=
      static_cast<__uint128_t>(certificate.jung_squared_denominator)) {
    reason = "jung_squared_leaves_no_disc";
    return false;
  }
  if (certificate.segment_position_count == 0U) {
    reason = "segment_position_count";
    return false;
  }
  if (certificate.certified_margin_exponent > 0) {
    reason = "certified_margin_exponent";
    return false;
  }
  // A declared tangent bound must name a direction set whose covering radius is
  // proved, and must not understate it: a covering radius below the truth
  // leaves a direction untested and can lose a support.
  if (certificate.tangent_direction_count != 0U) {
    const std::size_t sealed = sealed_tangent_covering_radius_millidegrees(
        certificate.tangent_direction_count);
    if (sealed == 0U) {
      if (certificate.tangent_covering_radius_proved) {
        reason = "tangent_covering_radius_claimed_proved_without_a_seal";
        return false;
      }
    } else if (certificate.tangent_covering_radius_millidegrees < sealed) {
      reason = "tangent_covering_radius_below_the_sealed_proof";
      return false;
    }
  } else if (certificate.tangent_covering_radius_proved) {
    reason = "tangent_covering_radius_proved_without_a_direction_set";
    return false;
  }

  // The three regimes must be internally consistent.  The first two can close
  // the seed-source argument, but neither certifies the floating rejection path.
  using Restriction = LocalGerminationCertificate::SeedRestriction;
  switch (certificate.seed_restriction) {
    case Restriction::exhaustive_over_pairs:
      if (!certificate.seed_loop_exhaustive_over_pairs ||
          certificate.seed_neighbourhood_cutoff_multiple != 0U) {
        reason = "an_exhaustive_seed_loop_declared_a_restriction";
        return false;
      }
      break;
    case Restriction::certified_tangent_bound:
      if (certificate.seed_loop_exhaustive_over_pairs) {
        reason = "a_restricted_seed_loop_claimed_to_be_exhaustive";
        return false;
      }
      // The certified bound is only certified when its covering radius is.
      if (certificate.tangent_direction_count == 0U ||
          !certificate.tangent_covering_radius_proved) {
        reason = "a_certified_tangent_bound_without_a_proved_covering_radius";
        return false;
      }
      break;
    case Restriction::declared_heuristic_cutoff:
      if (certificate.seed_loop_exhaustive_over_pairs) {
        reason = "a_restricted_seed_loop_claimed_to_be_exhaustive";
        return false;
      }
      if (certificate.seed_neighbourhood_cutoff_multiple == 0U) {
        reason = "a_heuristic_cutoff_without_a_multiple";
        return false;
      }
      break;
  }
  if (certificate.mass_partition_identity_available) {
    reason = "mass_partition_identity_is_not_available_under_this_basis";
    return false;
  }
  if (!certificate.completeness_basis_declared) {
    reason = "completeness_basis_declared";
    return false;
  }
  // This proof basis seals the geometric constants, not the binary64
  // implementation of every rejection.  Until outward intervals and exact
  // fallbacks exist, a producer may honestly report false but may not forge a
  // certified floating path.
  if (certificate.floating_rejections_certified) {
    reason = "floating_rejections_claimed_certified_without_interval_proof";
    return false;
  }
  if (certificate.censored_by_operational_deadline ==
      (certificate.unexamined_seed_pair_count == 0U)) {
    reason = "operational_deadline_accounting";
    return false;
  }
  return true;
}

bool local_germination_production_identity_holds(
    const LocalGerminationCertificate& certificate,
    const LocalGerminationProductionAudit& audit,
    std::string& reason) {
  reason.clear();
  if (!certificate.applicable) {
    reason = "certificate_not_applicable";
    return false;
  }
  if (!certificate.executed) {
    reason = "certificate_not_executed";
    return false;
  }
  const LocalGerminationCounters& counters = certificate.counters;

  // The arity of a run is exclusive: a run for triples emits no quadruple and
  // conversely.  A mixed emission means the sink saw a support the run did not
  // generate.
  if (certificate.support_size == 3U && counters.quadruple_candidates != 0U) {
    reason = "a_triple_run_produced_quadruples";
    return false;
  }
  if (certificate.support_size == 4U && counters.triple_candidates != 0U) {
    reason = "a_quadruple_run_produced_triples";
    return false;
  }

  // The cross-check that carries the identity: the consumer's own tally must
  // equal the producer's counters, arity by arity.
  if (audit.observed_triple_emissions != counters.triple_candidates) {
    reason = "observed_triple_emissions";
    return false;
  }
  if (audit.observed_quadruple_emissions != counters.quadruple_candidates) {
    reason = "observed_quadruple_emissions";
    return false;
  }

  // Acceptance is decided downstream, by the exact terminal classification, and
  // it can only ever retain a subset of what was produced.  An acceptance count
  // above the production count means a support was accepted that the generator
  // never emitted, which is precisely the failure completeness must exclude.
  if (audit.accepted_supports > counters.candidates()) {
    reason = "accepted_supports_exceed_production";
    return false;
  }

  // Internal monotonicity of the accounting: nothing retained without being
  // examined, and no stage inventing work.
  if (counters.pairs_retained > counters.pairs_examined) {
    reason = "pairs_retained_exceed_pairs_examined";
    return false;
  }
  if (counters.third_vertices_retained + counters.third_vertices_free_rejected >
      counters.third_vertices_examined) {
    reason = "third_vertex_accounting_exceeds_its_examination";
    return false;
  }
  // A retained pair is a necessary condition for examining a third vertex, so a
  // run that examined third vertices without retaining a pair is incoherent.
  if (counters.third_vertices_examined != 0U && counters.pairs_retained == 0U) {
    reason = "third_vertices_without_a_retained_pair";
    return false;
  }
  // Likewise no candidate can exist without a retained third vertex.
  if (counters.candidates() != 0U && counters.third_vertices_retained == 0U) {
    reason = "candidates_without_a_retained_third_vertex";
    return false;
  }
  return true;
}

bool local_germination_resume_induction_holds(
    const LocalGerminationCertificate& previous,
    const LocalGerminationCertificate& successor,
    std::string& reason) {
  reason.clear();
  // A resumed run is the same run.  Any declared constant that moved makes the
  // successor a different producer, whose records may not be appended.
  if (previous.applicable != successor.applicable) {
    reason = "applicability_changed";
    return false;
  }
  if (previous.executed != successor.executed) {
    reason = "execution_state_changed";
    return false;
  }
  if (!previous.applicable) {
    reason = "resume_certificate_not_applicable";
    return false;
  }
  if (!previous.executed) {
    reason = "resume_certificate_not_executed";
    return false;
  }
  if (previous.proof_basis != successor.proof_basis) {
    reason = "proof_basis_changed";
    return false;
  }
  if (previous.support_size != successor.support_size) {
    reason = "support_size_changed";
    return false;
  }
  if (previous.maximum_relevant_closed_rank !=
      successor.maximum_relevant_closed_rank) {
    reason = "maximum_relevant_closed_rank_changed";
    return false;
  }
  if (previous.jung_squared_numerator != successor.jung_squared_numerator ||
      previous.jung_squared_denominator !=
          successor.jung_squared_denominator) {
    reason = "jung_squared_changed";
    return false;
  }
  if (previous.seed_disc_ring_count != successor.seed_disc_ring_count) {
    reason = "seed_disc_ring_count_changed";
    return false;
  }
  if (previous.segment_position_count != successor.segment_position_count) {
    reason = "segment_position_count_changed";
    return false;
  }
  if (previous.certified_margin_exponent !=
      successor.certified_margin_exponent) {
    reason = "certified_margin_exponent_changed";
    return false;
  }
  if (previous.seed_neighbourhood_cutoff_multiple !=
          successor.seed_neighbourhood_cutoff_multiple ||
      previous.seed_loop_exhaustive_over_pairs !=
          successor.seed_loop_exhaustive_over_pairs) {
    reason = "seed_loop_restriction_changed";
    return false;
  }
  if (previous.seed_restriction != successor.seed_restriction) {
    reason = "seed_restriction_kind_changed";
    return false;
  }
  if (previous.tangent_direction_count != successor.tangent_direction_count ||
      previous.tangent_covering_radius_millidegrees !=
          successor.tangent_covering_radius_millidegrees ||
      previous.tangent_covering_radius_proved !=
          successor.tangent_covering_radius_proved) {
    reason = "tangent_direction_declaration_changed";
    return false;
  }
  if (previous.mass_partition_identity_available !=
          successor.mass_partition_identity_available ||
      previous.completeness_basis_declared !=
          successor.completeness_basis_declared) {
    reason = "declared_guarantees_changed";
    return false;
  }

  if (previous.censored_by_operational_deadline ==
      (previous.unexamined_seed_pair_count == 0U)) {
    reason = "previous_operational_deadline_accounting";
    return false;
  }
  if (successor.censored_by_operational_deadline ==
      (successor.unexamined_seed_pair_count == 0U)) {
    reason = "successor_operational_deadline_accounting";
    return false;
  }

  // This scalar conservation is necessary but not sufficient for coverage.
  // No cursor or prefix digest currently identifies WHICH pairs were examined,
  // so sealing stays disabled below even when this arithmetic holds.  Wide
  // addition prevents a forged size_t wraparound from satisfying the equality.
  const __uint128_t previous_seed_pair_total =
      static_cast<__uint128_t>(previous.counters.pairs_examined) +
      static_cast<__uint128_t>(previous.unexamined_seed_pair_count);
  const __uint128_t successor_seed_pair_total =
      static_cast<__uint128_t>(successor.counters.pairs_examined) +
      static_cast<__uint128_t>(successor.unexamined_seed_pair_count);
  if (previous_seed_pair_total != successor_seed_pair_total) {
    reason = "seed_pair_total_changed";
    return false;
  }
  if (successor.unexamined_seed_pair_count >
      previous.unexamined_seed_pair_count) {
    reason = "unexamined_seed_pair_count_increased";
    return false;
  }

  // The conserved quantity: a resumed run extends the accounting, it never
  // revises it.  Every counter is non-decreasing.
  const LocalGerminationCounters& before = previous.counters;
  const LocalGerminationCounters& after = successor.counters;
  const auto monotone = [&](std::size_t left, std::size_t right,
                            const char* field) {
    if (left > right) {
      reason = field;
      return false;
    }
    return true;
  };
  if (!monotone(before.pairs_examined, after.pairs_examined,
                "pairs_examined_went_backwards") ||
      !monotone(before.pairs_retained, after.pairs_retained,
                "pairs_retained_went_backwards") ||
      !monotone(before.third_vertices_examined, after.third_vertices_examined,
                "third_vertices_examined_went_backwards") ||
      !monotone(before.third_vertices_free_rejected,
                after.third_vertices_free_rejected,
                "third_vertices_free_rejected_went_backwards") ||
      !monotone(before.third_vertices_retained, after.third_vertices_retained,
                "third_vertices_retained_went_backwards") ||
      !monotone(before.triple_candidates, after.triple_candidates,
                "triple_candidates_went_backwards") ||
      !monotone(before.quadruple_candidates, after.quadruple_candidates,
                "quadruple_candidates_went_backwards") ||
      !monotone(before.population_queries, after.population_queries,
                "population_queries_went_backwards")) {
    return false;
  }
  if (!previous.censored_by_operational_deadline &&
      (before.pairs_examined != after.pairs_examined ||
       before.pairs_retained != after.pairs_retained ||
       before.third_vertices_examined != after.third_vertices_examined ||
       before.third_vertices_free_rejected !=
           after.third_vertices_free_rejected ||
       before.third_vertices_retained != after.third_vertices_retained ||
       before.triple_candidates != after.triple_candidates ||
       before.quadruple_candidates != after.quadruple_candidates ||
       before.population_queries != after.population_queries)) {
    reason = "a_complete_seed_loop_was_extended";
    return false;
  }
  return true;
}

LocalGerminationCertificate generate_local_germination_candidates(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t support_size,
    const LocalGerminationConfig& config,
    const LocalGerminationSink& sink,
    const LocalGerminationGuard& guard) {
  static_cast<void>(index);
  if (guard.armed && guard.seed_pairs_per_deadline_test == 0U) {
    throw std::invalid_argument(
        "an armed germination guard needs a positive test interval");
  }
  if (support_size != 3U && support_size != 4U) {
    throw std::invalid_argument("a germinated support has size three or four");
  }
  if (config.maximum_relevant_closed_rank < support_size) {
    LocalGerminationCertificate empty;
    empty.support_size = support_size;
    empty.maximum_relevant_closed_rank = config.maximum_relevant_closed_rank;
    // No generator invocation took place and no proof basis was relied upon.
    // The arity identity is still carried so callers can distinguish this
    // canonical non-applicable slot from a default-constructed object.
    empty.applicable = false;
    empty.executed = false;
    return empty;
  }
  if (config.segment_position_count == 0U) {
    throw std::invalid_argument(
        "the segment covering needs at least one position");
  }

  const std::size_t point_count = cloud.size();
  std::vector<Vec3> points;
  points.reserve(point_count);
  for (PointId id = 0U; id < static_cast<PointId>(point_count); ++id) {
    points.push_back(point_of(cloud, id));
  }

  std::uint64_t jung_numerator = config.jung_squared_numerator;
  std::uint64_t jung_denominator = config.jung_squared_denominator;
  if (jung_denominator == 0U) {
    const auto theorem = theorem_jung_squared(support_size);
    jung_numerator = theorem.first;
    jung_denominator = theorem.second;
  }
  // Cell size aimed at a handful of points per cell; the grid is an
  // acceleration and its tuning changes no verdict.
  Vec3 span = Vec3{0.0, 0.0, 0.0};
  {
    Vec3 low = points.front();
    Vec3 high = points.front();
    for (const Vec3& point : points) {
      low.x = std::min(low.x, point.x);
      low.y = std::min(low.y, point.y);
      low.z = std::min(low.z, point.z);
      high.x = std::max(high.x, point.x);
      high.y = std::max(high.y, point.y);
      high.z = std::max(high.z, point.z);
    }
    span = high - low;
  }
  const double longest =
      std::max(span.x, std::max(span.y, std::max(span.z, 1.0e-300)));
  const double cell = longest /
      std::max(1.0, std::cbrt(static_cast<double>(point_count) / 4.0));
  const UniformGrid grid{points, cell};

  const double gamma_squared = static_cast<double>(jung_numerator) /
      static_cast<double>(jung_denominator);
  if (!(gamma_squared > 0.25)) {
    throw std::invalid_argument(
        "the declared squared Jung constant leaves no disc of centres");
  }
  if (config.certified_margin_exponent > 0) {
    throw std::invalid_argument(
        "the certified margin must not exceed the diameter");
  }
  const double certified_margin_scale =
      std::ldexp(1.0, config.certified_margin_exponent);
  const double gamma = std::sqrt(gamma_squared);
  const double disc_radius_coefficient = std::sqrt(gamma_squared - 0.25);
  const DiscCover cover = build_disc_cover(config.seed_disc_ring_count);
  const std::size_t cap = config.maximum_relevant_closed_rank;

  LocalGerminationCertificate certificate;
  certificate.applicable = true;
  certificate.executed = true;
  certificate.proof_basis = std::string{local_germination_proof_basis};
  certificate.support_size = support_size;
  certificate.maximum_relevant_closed_rank =
      config.maximum_relevant_closed_rank;
  certificate.jung_squared_numerator = jung_numerator;
  certificate.jung_squared_denominator = jung_denominator;
  certificate.seed_disc_ring_count = config.seed_disc_ring_count;
  certificate.segment_position_count = config.segment_position_count;
  certificate.certified_margin_exponent = config.certified_margin_exponent;
  certificate.seed_neighbourhood_cutoff_multiple =
      config.seed_neighbourhood_cutoff_multiple;
  certificate.tangent_direction_count = config.tangent_direction_count;
  certificate.tangent_covering_radius_millidegrees =
      sealed_tangent_covering_radius_millidegrees(
          config.tangent_direction_count);
  certificate.tangent_covering_radius_proved =
      config.tangent_direction_count != 0U &&
      certificate.tangent_covering_radius_millidegrees != 0U;
  if (config.tangent_direction_count != 0U) {
    certificate.seed_restriction =
        LocalGerminationCertificate::SeedRestriction::certified_tangent_bound;
    certificate.seed_loop_exhaustive_over_pairs = false;
  } else if (config.seed_neighbourhood_cutoff_multiple != 0U) {
    certificate.seed_restriction = LocalGerminationCertificate::
        SeedRestriction::declared_heuristic_cutoff;
    certificate.seed_loop_exhaustive_over_pairs = false;
  } else {
    certificate.seed_restriction =
        LocalGerminationCertificate::SeedRestriction::exhaustive_over_pairs;
    certificate.seed_loop_exhaustive_over_pairs = true;
  }
  LocalGerminationCounters& counters = certificate.counters;

  // The certified seed restriction: pairs must satisfy D <= 2 R(p) at BOTH
  // endpoints, which is the graph of the corollary.  Zero directions leaves the
  // loop exhaustive.
  std::vector<double> tangent_bound;
  if (config.tangent_direction_count != 0U) {
    const std::size_t sealed = sealed_tangent_covering_radius_millidegrees(
        config.tangent_direction_count);
    if (sealed == 0U) {
      throw std::invalid_argument(
          "the declared direction count has no sealed covering radius");
    }
    const double theta = static_cast<double>(sealed) / 1000.0 *
        std::numbers::pi / 180.0;
    const std::vector<Vec3> directions =
        cube_directions(config.tangent_direction_count);
    AxisAlignedBox box{points.front(), points.front()};
    for (const Vec3& point : points) {
      box.lower.x = std::min(box.lower.x, point.x);
      box.lower.y = std::min(box.lower.y, point.y);
      box.lower.z = std::min(box.lower.z, point.z);
      box.upper.x = std::max(box.upper.x, point.x);
      box.upper.y = std::max(box.upper.y, point.y);
      box.upper.z = std::max(box.upper.z, point.z);
    }
    const Vec3 extent = box.upper - box.lower;
    const double ceiling = norm(extent);
    tangent_bound.reserve(points.size());
    for (const Vec3& point : points) {
      tangent_bound.push_back(2.0 * certified_tangent_radius_bound(
                                        points, grid, directions, box, point, cap,
                                        theta, ceiling,
                                        counters.population_queries));
    }
  }

  std::array<PointId, 4> support{};
  std::vector<PointId> retained;

  // Total seed pairs, so a censured run can say how far it got rather than
  // merely that it stopped.
  const std::size_t seed_pair_total = point_count < 2U
      ? 0U
      : point_count * (point_count - 1U) / 2U;
  std::size_t pairs_since_deadline_test = 0U;
  bool censored = false;

  for (PointId first = 0U;
       first + 1U < static_cast<PointId>(point_count) && !censored;
       ++first) {
    for (PointId second = first + 1U;
         second < static_cast<PointId>(point_count);
         ++second) {
      // Tested between pairs, so the deadline is honoured to within one pair's
      // work.  Reading the clock on every pair of a large cloud would itself be
      // the cost under measurement.
      if (guard.armed) {
        ++pairs_since_deadline_test;
        if (pairs_since_deadline_test >= guard.seed_pairs_per_deadline_test) {
          pairs_since_deadline_test = 0U;
          if (std::chrono::steady_clock::now() >= guard.deadline) {
            censored = true;
            break;
          }
        }
      }
      ++counters.pairs_examined;
      const Vec3& p = points[first];
      const Vec3& q = points[second];
      const Vec3 edge = q - p;
      const double diameter = norm(edge);
      if (!(diameter > 0.0)) {
        continue;
      }
      // The certified restriction, applied at both endpoints.
      if (!tangent_bound.empty() &&
          (diameter > tangent_bound[first] || diameter > tangent_bound[second])) {
        continue;
      }
      const double margin = certified_margin_scale * diameter;
      const Vec3 midpoint = p + edge * 0.5;

      // J7 -- the seed.  The circumcentre lies in the disc of radius
      // sqrt(gamma^2 - 1/4) D around the midpoint, in the bisector plane, and
      // the circumradius is at least D/2, so a ball of radius D/2 minus the
      // covering radius around any sample of a covering is inside it.
      const Vec3 axis = edge * (1.0 / diameter);
      Vec3 basis_first{};
      Vec3 basis_second{};
      perpendicular_basis(axis, basis_first, basis_second);
      const double disc_radius = disc_radius_coefficient * diameter;
      const double seed_test_radius =
          0.5 * diameter - cover.covering_radius * disc_radius;
      bool seed_rejected = seed_test_radius > 0.0;
      if (seed_rejected) {
        for (const auto& offset : cover.offsets) {
          const Vec3 centre = midpoint +
              basis_first * (offset.first * disc_radius) +
              basis_second * (offset.second * disc_radius);
          ++counters.population_queries;
          if (!population_exceeds(points, grid, centre, seed_test_radius, cap,
                                  margin)) {
            seed_rejected = false;
            break;
          }
        }
      }
      if (seed_rejected) {
        continue;
      }
      ++counters.pairs_retained;

      // The remaining vertices lie in the lens: the diameter pair bounds every
      // distance in the support.
      retained.clear();
      for (PointId candidate = 0U;
           candidate < static_cast<PointId>(point_count);
           ++candidate) {
        if (candidate == first || candidate == second) {
          continue;
        }
        const Vec3& z = points[candidate];
        const double to_first = norm(z - p);
        const double to_second = norm(z - q);
        if (to_first > diameter || to_second > diameter) {
          continue;
        }
        ++counters.third_vertices_examined;

        // J4' -- the free test.  Any point equidistant from the three vertices
        // sits at distance sqrt(rho^2 - r_triangle^2) from the triangle's
        // circumcentre, so r >= r_triangle; a circumradius above gamma D is
        // impossible and needs no query at all.
        const Vec3 u = q - p;
        const Vec3 v = z - p;
        const Vec3 normal = cross(u, v);
        const double normal_norm = norm(normal);
        if (!(normal_norm > 0.0)) {
          // Affinely dependent: the classifier will resolve it, keep it.
          retained.push_back(candidate);
          ++counters.third_vertices_retained;
          continue;
        }
        const double uu = dot(u, u);
        const double vv = dot(v, v);
        const Vec3 to_centre =
            cross(u * vv - v * uu, normal) * (1.0 / (2.0 * normal_norm * normal_norm));
        const Vec3 circumcentre = p + to_centre;
        const double triangle_radius = norm(to_centre);
        if (triangle_radius > gamma * diameter) {
          ++counters.third_vertices_free_rejected;
          continue;
        }

        // J8 -- the segment.  The circumcentre lies on the line through the
        // triangle's circumcentre along its normal, within
        // sqrt(gamma^2 D^2 - r_triangle^2), and the circumradius is at least
        // r_triangle.
        const double half_length = std::sqrt(std::max(
            gamma_squared * diameter * diameter -
                triangle_radius * triangle_radius,
            0.0));
        const std::size_t positions = config.segment_position_count;
        const double covering =
            positions <= 1U
                ? half_length
                : half_length / static_cast<double>(positions - 1U);
        const double segment_test_radius = triangle_radius - covering;
        bool rejected = segment_test_radius > 0.0;
        if (rejected) {
          const Vec3 unit_normal = normal * (1.0 / normal_norm);
          for (std::size_t position = 0U; position < positions; ++position) {
            const double parameter = positions <= 1U
                ? 0.0
                : -half_length +
                    2.0 * half_length * static_cast<double>(position) /
                        static_cast<double>(positions - 1U);
            const Vec3 centre = circumcentre + unit_normal * parameter;
            ++counters.population_queries;
            if (!population_exceeds(points, grid, centre, segment_test_radius,
                                    cap, margin)) {
              rejected = false;
              break;
            }
          }
        }
        if (rejected) {
          continue;
        }
        retained.push_back(candidate);
        ++counters.third_vertices_retained;
      }

      if (support_size == 3U) {
        support[3] = 0U;
        for (const PointId third : retained) {
          std::array<PointId, 3> ids{first, second, third};
          std::sort(ids.begin(), ids.end());
          support[0] = ids[0];
          support[1] = ids[1];
          support[2] = ids[2];
          ++counters.triple_candidates;
          sink(support, 3U);
        }
        continue;
      }

      for (std::size_t left = 0U; left < retained.size(); ++left) {
        for (std::size_t right = left + 1U; right < retained.size(); ++right) {
          const Vec3& z = points[retained[left]];
          const Vec3& w = points[retained[right]];
          if (norm(w - z) > diameter) {
            continue;
          }
          std::array<PointId, 4> ids{
              first, second, retained[left], retained[right]};
          std::sort(ids.begin(), ids.end());
          support = ids;
          ++counters.quadruple_candidates;
          sink(support, 4U);
        }
      }
    }
  }
  certificate.censored_by_operational_deadline = censored;
  certificate.unexamined_seed_pair_count =
      seed_pair_total - std::min(seed_pair_total, counters.pairs_examined);
  return certificate;
}

ExactLocalGerminationChain::ExactLocalGerminationChain(
    LocalGerminationCertificate genesis)
    : trusted_(std::move(genesis)) {
  std::string reason;
  genesis_admissible_ =
      local_germination_certificate_admissible(trusted_, reason);
}

bool ExactLocalGerminationChain::commit(
    const LocalGerminationCertificate& successor,
    const LocalGerminationProductionAudit& audit,
    std::vector<ExactHigherSupportEvent> events,
    std::vector<ExactHigherSupportExtraShellDiagnostic> diagnostics,
    std::string& reason) {
  reason.clear();
  if (sealed_) {
    reason = "the_chain_is_sealed";
    return false;
  }
  if (!genesis_admissible_) {
    reason = "the_genesis_certificate_was_not_admissible";
    return false;
  }
  // Everything is verified before anything is mutated: the three contracts, in
  // the order in which they can fail cheapest first.
  if (!local_germination_certificate_admissible(successor, reason)) {
    return false;
  }
  if (!local_germination_production_identity_holds(successor, audit, reason)) {
    return false;
  }
  if (!local_germination_resume_induction_holds(trusted_, successor, reason)) {
    return false;
  }
  events_.insert(
      events_.end(),
      std::make_move_iterator(events.begin()),
      std::make_move_iterator(events.end()));
  diagnostics_.insert(
      diagnostics_.end(),
      std::make_move_iterator(diagnostics.begin()),
      std::make_move_iterator(diagnostics.end()));
  trusted_ = successor;
  ++committed_batch_count_;
  return true;
}

ExactLocalGerminationChain::Seal ExactLocalGerminationChain::seal() {
  Seal seal;
  if (!genesis_admissible_) {
    return seal;
  }
  seal.certificate = trusted_;
  seal.committed_batch_count = committed_batch_count_;
  seal.event_count = events_.size();
  seal.diagnostic_count = diagnostics_.size();
  seal.mass_partition_identity_available =
      verification_basis_guarantees(seal.basis)
          .mass_partition_identity_available;
  // Three independent obligations remain unavailable in this API:
  //   * a cursor or digest proving that resumes cover a contiguous suffix;
  //   * an identity between accepted audit deltas and appended event payloads;
  //   * certified outward intervals / exact fallbacks for every floating
  //     rejection in the producer.
  // A structurally admissible transcript is still useful diagnostically, but
  // none of these facts may be inferred from its monotone counters.  The seal
  // therefore stays explicitly false until the API can compute all three.
  seal.contiguous_seed_pair_prefix_verified = false;
  seal.accepted_payload_identity_verified = false;
  seal.floating_rejections_verified = false;
  seal.completeness_certificate_verified = false;
  return seal;
}

}  // namespace morsehgp3d::hierarchy
