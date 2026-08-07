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

// Counts the points PROVED strictly inside the ball, stopping as soon as the
// cap is exceeded.  A point whose squared distance is within the margin of the
// radius is skipped: under-counting is safe, since the caller only ever uses
// "the count exceeds s_max" as a reason to reject.
[[nodiscard]] bool population_exceeds(
    const std::vector<Vec3>& points,
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
  for (const Vec3& point : points) {
    const Vec3 offset = point - centre;
    if (dot(offset, offset) < inner_squared) {
      ++count;
      if (count > cap) {
        return true;
      }
    }
  }
  return false;
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

}  // namespace

bool local_germination_certificate_admissible(
    const LocalGerminationCertificate& certificate, std::string& reason) {
  reason.clear();
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
  if (certificate.mass_partition_identity_available) {
    reason = "mass_partition_identity_is_not_available_under_this_basis";
    return false;
  }
  if (!certificate.completeness_basis_declared) {
    reason = "completeness_basis_declared";
    return false;
  }
  return true;
}

bool local_germination_production_identity_holds(
    const LocalGerminationCertificate& certificate,
    const LocalGerminationProductionAudit& audit,
    std::string& reason) {
  reason.clear();
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
  if (previous.mass_partition_identity_available !=
          successor.mass_partition_identity_available ||
      previous.completeness_basis_declared !=
          successor.completeness_basis_declared) {
    reason = "declared_guarantees_changed";
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
  return true;
}

LocalGerminationCertificate generate_local_germination_candidates(
    const spatial::MortonLbvhIndex& index,
    const spatial::CanonicalPointCloud& cloud,
    std::size_t support_size,
    const LocalGerminationConfig& config,
    const LocalGerminationSink& sink) {
  static_cast<void>(index);
  if (support_size != 3U && support_size != 4U) {
    throw std::invalid_argument("a germinated support has size three or four");
  }
  if (config.maximum_relevant_closed_rank < support_size) {
    LocalGerminationCertificate empty;
    empty.proof_basis = std::string{local_germination_proof_basis};
    empty.support_size = support_size;
    empty.maximum_relevant_closed_rank = config.maximum_relevant_closed_rank;
    const auto theorem = theorem_jung_squared(support_size);
    empty.jung_squared_numerator = theorem.first;
    empty.jung_squared_denominator = theorem.second;
    empty.seed_disc_ring_count = config.seed_disc_ring_count;
    empty.segment_position_count = config.segment_position_count;
    empty.certified_margin_exponent = config.certified_margin_exponent;
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
  certificate.proof_basis = std::string{local_germination_proof_basis};
  certificate.support_size = support_size;
  certificate.maximum_relevant_closed_rank =
      config.maximum_relevant_closed_rank;
  certificate.jung_squared_numerator = jung_numerator;
  certificate.jung_squared_denominator = jung_denominator;
  certificate.seed_disc_ring_count = config.seed_disc_ring_count;
  certificate.segment_position_count = config.segment_position_count;
  certificate.certified_margin_exponent = config.certified_margin_exponent;
  LocalGerminationCounters& counters = certificate.counters;
  std::array<PointId, 4> support{};
  std::vector<PointId> retained;

  for (PointId first = 0U; first + 1U < static_cast<PointId>(point_count);
       ++first) {
    for (PointId second = first + 1U;
         second < static_cast<PointId>(point_count);
         ++second) {
      ++counters.pairs_examined;
      const Vec3& p = points[first];
      const Vec3& q = points[second];
      const Vec3 edge = q - p;
      const double diameter = norm(edge);
      if (!(diameter > 0.0)) {
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
          if (!population_exceeds(points, centre, seed_test_radius, cap,
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
            if (!population_exceeds(points, centre, segment_test_radius, cap,
                                    margin)) {
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
  std::string reason;
  seal.completeness_certificate_verified =
      local_germination_certificate_admissible(trusted_, reason);
  if (!seal.completeness_certificate_verified) {
    return seal;
  }
  seal.sealed = true;
  seal.certificate = trusted_;
  seal.committed_batch_count = committed_batch_count_;
  seal.event_count = events_.size();
  seal.diagnostic_count = diagnostics_.size();
  seal.mass_partition_identity_available =
      verification_basis_guarantees(seal.basis)
          .mass_partition_identity_available;
  sealed_ = true;
  return seal;
}

}  // namespace morsehgp3d::hierarchy
