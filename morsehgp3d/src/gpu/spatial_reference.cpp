#include "morsehgp3d/gpu/spatial_reference.hpp"

#include "phase4_spatial_reference_internal.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/rational.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

using exact::BigInt;
using exact::ExactRational;
using exact::ExactRational3;

constexpr std::uint64_t kPositiveMaximumFiniteBits =
    UINT64_C(0x7fefffffffffffff);
constexpr std::uint64_t kSignBit = UINT64_C(0x8000000000000000);
constexpr std::uint64_t kExponentMask = UINT64_C(0x7ff0000000000000);
constexpr std::uint64_t kFractionMask = UINT64_C(0x000fffffffffffff);
constexpr std::uint64_t kPositiveInfinityBits =
    UINT64_C(0x7ff0000000000000);
constexpr std::uint64_t kFnvOffsetBasis = UINT64_C(14695981039346656037);
constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);

struct ProjectedCoordinate {
  std::uint64_t bits{0U};
  QueryCoordinateProjection status{QueryCoordinateProjection::exact};
};

[[nodiscard]] ExactRational positive_binary64_rational(std::uint64_t bits) {
  if (bits > kPositiveMaximumFiniteBits) {
    throw std::logic_error("a projection search produced a non-finite binary64 word");
  }
  return ExactRational::from_binary64_bits(bits);
}

[[nodiscard]] std::uint64_t project_nonnegative_rational(
    const ExactRational& value) {
  if (value.sign() < 0) {
    throw std::logic_error("a nonnegative projection received a negative rational");
  }
  const ExactRational maximum =
      positive_binary64_rational(kPositiveMaximumFiniteBits);
  if (value > maximum) {
    throw std::range_error(
        "an exact spatial query coordinate exceeds finite binary64 proposal range");
  }

  std::uint64_t lower_bits = 0U;
  std::uint64_t upper_bits = kPositiveMaximumFiniteBits;
  while (lower_bits < upper_bits) {
    const std::uint64_t midpoint_bits =
        lower_bits + (upper_bits - lower_bits + 1U) / 2U;
    if (positive_binary64_rational(midpoint_bits) <= value) {
      lower_bits = midpoint_bits;
    } else {
      upper_bits = midpoint_bits - 1U;
    }
  }

  const ExactRational lower = positive_binary64_rational(lower_bits);
  if (lower == value || lower_bits == kPositiveMaximumFiniteBits) {
    return lower_bits;
  }
  const std::uint64_t adjacent_bits = lower_bits + 1U;
  const ExactRational adjacent = positive_binary64_rational(adjacent_bits);
  const ExactRational halfway =
      (lower + adjacent) / ExactRational{BigInt{2}};
  if (value < halfway) {
    return lower_bits;
  }
  if (value > halfway) {
    return adjacent_bits;
  }
  // Adjacent positive binary64 encodings alternate significand parity in the
  // low bit, including across subnormal and binade boundaries.
  return (lower_bits & 1U) == 0U ? lower_bits : adjacent_bits;
}

[[nodiscard]] ProjectedCoordinate project_coordinate(
    const ExactRational& value) {
  const bool negative = value.sign() < 0;
  const ExactRational magnitude = negative ? -value : value;
  const ExactRational maximum =
      positive_binary64_rational(kPositiveMaximumFiniteBits);
  if (magnitude > maximum) {
    return ProjectedCoordinate{
        kPositiveMaximumFiniteBits | (negative ? kSignBit : 0U),
        QueryCoordinateProjection::overflow_clamped};
  }
  const std::uint64_t magnitude_bits =
      project_nonnegative_rational(magnitude);
  const std::uint64_t projected_bits =
      negative ? magnitude_bits | kSignBit : magnitude_bits;
  const ExactRational projected =
      ExactRational::from_binary64_bits(projected_bits);
  QueryCoordinateProjection status = QueryCoordinateProjection::rounded;
  if (projected == value) {
    status = QueryCoordinateProjection::exact;
  } else if (magnitude_bits == 0U) {
    status = QueryCoordinateProjection::underflow;
  }
  return ProjectedCoordinate{projected_bits, status};
}

[[nodiscard]] ExactRational3 validated_query(const ExactRational3& query) {
  ExactRational3 canonical{
      query.numerator(0U),
      query.numerator(1U),
      query.numerator(2U),
      query.denominator()};
  if (canonical != query) {
    throw std::invalid_argument("an exact spatial query must be canonical");
  }
  return canonical;
}

[[nodiscard]] exact::ExactLevel validated_radius(
    const exact::ExactLevel& squared_radius) {
  exact::ExactLevel canonical{
      squared_radius.numerator(), squared_radius.denominator()};
  if (canonical != squared_radius) {
    throw std::invalid_argument("an exact squared radius must be canonical");
  }
  return canonical;
}

[[nodiscard]] SpatialReferenceAudit validate_proposal_batch(
    const detail::SpatialProposalBatch& batch,
    std::size_t point_count,
    const std::array<ProjectedCoordinate, 3>& projected_query) {
  SpatialReferenceAudit audit;
  audit.gpu_input_point_count = point_count;
  audit.gpu_output_record_count = batch.records.size();
  audit.gpu_launch_count = 1U;
  audit.buffer_epoch = batch.buffer_epoch;
  for (std::size_t axis = 0U; axis < projected_query.size(); ++axis) {
    audit.projected_query_bits[axis] = projected_query[axis].bits;
    audit.query_projection[axis] = projected_query[axis].status;
  }

  if (batch.records.size() != point_count) {
    throw std::runtime_error(
        "the GPU spatial proposal did not return one record per canonical point");
  }
  std::vector<unsigned char> seen(point_count, 0U);
  std::vector<std::uint64_t> distance_bits_by_id(point_count, 0U);
  for (const detail::SpatialProposalRecord& record : batch.records) {
    if (!std::in_range<std::size_t>(record.point_id) ||
        static_cast<std::size_t>(record.point_id) >= point_count) {
      throw std::runtime_error(
          "the GPU spatial proposal returned a PointId outside its namespace");
    }
    const std::size_t index = static_cast<std::size_t>(record.point_id);
    if (seen[index] != 0U) {
      throw std::runtime_error(
          "the GPU spatial proposal repeated a canonical PointId");
    }
    seen[index] = 1U;
    distance_bits_by_id[index] = record.squared_distance_bits;
    ++audit.gpu_unique_point_id_count;

    const std::uint64_t bits = record.squared_distance_bits;
    const bool non_finite = (bits & kExponentMask) == kExponentMask;
    const bool nan = non_finite && (bits & kFractionMask) != 0U;
    if (nan) {
      ++audit.gpu_nan_distance_proposal_count;
      continue;
    }
    if (bits == kPositiveInfinityBits) {
      ++audit.gpu_infinite_distance_proposal_count;
      continue;
    }
    if (non_finite || (bits & kSignBit) != 0U) {
      throw std::runtime_error(
          "the GPU spatial proposal returned an invalid squared distance");
    }
    ++audit.gpu_finite_distance_proposal_count;
  }
  if (audit.gpu_nan_distance_proposal_count != 0U) {
    throw std::runtime_error(
        "the GPU spatial proposal returned a NaN squared distance");
  }
  if (audit.gpu_unique_point_id_count != point_count ||
      audit.gpu_finite_distance_proposal_count +
              audit.gpu_infinite_distance_proposal_count !=
          point_count) {
    throw std::runtime_error(
        "the GPU spatial proposal did not close its exhaustive permutation");
  }
  audit.all_points_enumerated = true;
  std::uint64_t digest = kFnvOffsetBasis;
  for (std::size_t index = 0U; index < point_count; ++index) {
    const std::array<std::uint64_t, 2> words{
        static_cast<std::uint64_t>(index), distance_bits_by_id[index]};
    for (const std::uint64_t word : words) {
      for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
        digest ^= (word >> shift) & UINT64_C(0xff);
        digest *= kFnvPrime;
      }
    }
  }
  audit.proposal_digest_fnv1a = digest;
  return audit;
}

}  // namespace

SpatialReferenceContext::SpatialReferenceContext(
    const spatial::CanonicalPointCloud& cloud)
    : state_(std::make_shared<detail::SpatialReferenceContextState>()),
      cloud_identity_(cloud.identity_),
      point_count_(cloud.size()) {
  if (cloud_identity_ == nullptr || point_count_ == 0U) {
    throw std::invalid_argument(
        "a GPU spatial reference requires a nonempty canonical point cloud");
  }
  if (point_count_ > std::numeric_limits<std::size_t>::max() / 3U) {
    throw std::length_error("the GPU spatial coordinate packing size overflowed");
  }
  coordinate_bits_.resize(point_count_ * 3U);
  for (std::size_t index = 0U; index < point_count_; ++index) {
    const spatial::PointId id = static_cast<spatial::PointId>(index);
    const auto bits = cloud.point(id).canonical_input_bits();
    for (std::size_t axis = 0U; axis < bits.size(); ++axis) {
      coordinate_bits_[axis * point_count_ + index] = bits[axis];
    }
  }
}

SpatialReferenceContext::~SpatialReferenceContext() noexcept = default;
SpatialReferenceContext::SpatialReferenceContext(
    SpatialReferenceContext&&) noexcept = default;
SpatialReferenceContext& SpatialReferenceContext::operator=(
    SpatialReferenceContext&&) noexcept = default;

void SpatialReferenceContext::require_matching_cloud(
    const spatial::CanonicalPointCloud& cloud) const {
  if (state_ == nullptr || cloud_identity_ == nullptr || point_count_ == 0U) {
    throw std::invalid_argument(
        "a moved-from GPU spatial reference context is not queryable");
  }
  if (cloud.identity_ != cloud_identity_ || cloud.size() != point_count_) {
    throw std::invalid_argument(
        "the GPU spatial reference context belongs to a different PointId namespace");
  }
}

SpatialReferenceAudit SpatialReferenceContext::run_proposal(
    const ExactRational3& query) {
  std::array<ProjectedCoordinate, 3> projected{};
  std::array<std::uint64_t, 3> query_bits{};
  for (std::size_t axis = 0U; axis < projected.size(); ++axis) {
    projected[axis] = project_coordinate(query.coordinate(axis));
    query_bits[axis] = projected[axis].bits;
  }

  // The launch, copy-back, permutation validation, and possible poisoning form
  // one serialized transaction.  A concurrent caller may therefore observe
  // either the validated batch or the poisoned state, never an unvalidated
  // batch that escaped while another caller was closing the same context.
  return state_->with_gpu_section([&] {
    const detail::SpatialProposalBatch batch =
        detail::propose_squared_distances_on_gpu(
            *state_, coordinate_bits_, point_count_, query_bits);
    return validate_proposal_batch(batch, point_count_, projected);
  });
}

SpatialReferenceTopKResult SpatialReferenceContext::top_k(
    const spatial::CanonicalPointCloud& cloud,
    const ExactRational3& query,
    std::size_t requested_rank,
    const spatial::ExclusionSet& exclusions) {
  require_matching_cloud(cloud);
  if (!exclusions.validated_for(cloud)) {
    throw std::invalid_argument(
        "the exclusion set belongs to a different canonical point namespace");
  }
  const std::size_t eligible_point_count =
      point_count_ - exclusions.ids().size();
  if (requested_rank == 0U || requested_rank > eligible_point_count) {
    throw std::out_of_range(
        "the requested rank is outside the eligible point set");
  }
  const ExactRational3 canonical_query = validated_query(query);
  SpatialReferenceAudit audit = run_proposal(canonical_query);
  spatial::TopKPartition partition = spatial::brute_force_top_k(
      cloud, canonical_query, requested_rank, exclusions);
  audit.cpu_exact_distance_evaluation_count =
      partition.distance_evaluation_count();
  if (audit.cpu_exact_distance_evaluation_count != eligible_point_count) {
    throw std::logic_error(
        "the GPU reference did not recertify every eligible distance on CPU");
  }
  audit.cpu_exact_recertification_complete = true;
  return SpatialReferenceTopKResult{
      std::move(partition), std::move(audit)};
}

SpatialReferenceTopKResult SpatialReferenceContext::nearest(
    const spatial::CanonicalPointCloud& cloud,
    const ExactRational3& query,
    const spatial::ExclusionSet& exclusions) {
  return top_k(cloud, query, 1U, exclusions);
}

SpatialReferenceClosedBallResult SpatialReferenceContext::closed_ball(
    const spatial::CanonicalPointCloud& cloud,
    const ExactRational3& query,
    const exact::ExactLevel& squared_radius) {
  require_matching_cloud(cloud);
  const ExactRational3 canonical_query = validated_query(query);
  const exact::ExactLevel canonical_radius = validated_radius(squared_radius);
  SpatialReferenceAudit audit = run_proposal(canonical_query);
  spatial::ClosedBallPartition partition = spatial::brute_force_closed_ball(
      cloud, canonical_query, canonical_radius);
  audit.cpu_exact_distance_evaluation_count =
      partition.distance_evaluation_count();
  if (audit.cpu_exact_distance_evaluation_count != point_count_) {
    throw std::logic_error(
        "the GPU reference did not recertify every point distance on CPU");
  }
  audit.cpu_exact_recertification_complete = true;
  return SpatialReferenceClosedBallResult{
      std::move(partition), std::move(audit)};
}

}  // namespace morsehgp3d::gpu
