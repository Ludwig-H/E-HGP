#include "morsehgp3d/gpu/spatial_bounds.hpp"

#include "phase4_spatial_bounds_internal.hpp"
#include "rational_binary64_enclosure.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/rational.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

using exact::ExactRational;
using exact::ExactRational3;

constexpr std::uint64_t kFnvOffsetBasis = UINT64_C(14695981039346656037);
constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);

using detail::DirectedEnclosure;
using detail::enclose_nonnegative_rational;
using detail::enclose_rational;
using detail::kExponentMask;
using detail::kFractionMask;
using detail::kPositiveInfinityBits;
using detail::kSignBit;

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

[[nodiscard]] exact::ExactLevel validated_cutoff(
    const exact::ExactLevel& squared_cutoff) {
  exact::ExactLevel canonical{
      squared_cutoff.numerator(), squared_cutoff.denominator()};
  if (canonical != squared_cutoff) {
    throw std::invalid_argument("an exact squared cutoff must be canonical");
  }
  return canonical;
}

void validate_box(const spatial::ExactDyadicAabb3& box) {
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const std::uint64_t lower = box.lower_binary64_bits[axis];
    const std::uint64_t upper = box.upper_binary64_bits[axis];
    if (exact::canonicalize_binary64_bits(lower) != lower ||
        exact::canonicalize_binary64_bits(upper) != upper) {
      throw std::invalid_argument(
          "a spatial AABB must use canonical finite binary64 bounds");
    }
    if (exact::binary64_total_order_key(lower) >
        exact::binary64_total_order_key(upper)) {
      throw std::invalid_argument(
          "a spatial AABB lower bound exceeds its upper bound");
    }
  }
}

[[nodiscard]] exact::ExactLevel exact_minimum_squared_distance(
    const spatial::ExactDyadicAabb3& box,
    const ExactRational3& query) {
  ExactRational squared_distance;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const ExactRational lower = ExactRational::from_binary64_bits(
        box.lower_binary64_bits[axis]);
    const ExactRational upper = ExactRational::from_binary64_bits(
        box.upper_binary64_bits[axis]);
    const ExactRational coordinate = query.coordinate(axis);
    ExactRational delta;
    if (coordinate < lower) {
      delta = lower - coordinate;
    } else if (coordinate > upper) {
      delta = coordinate - upper;
    }
    squared_distance = squared_distance + delta * delta;
  }
  return exact::ExactLevel{std::move(squared_distance)};
}

void hash_word(std::uint64_t& digest, std::uint64_t word) noexcept {
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
}

[[nodiscard]] bool is_nonnegative_interval_word(std::uint64_t bits) noexcept {
  if ((bits & kSignBit) != 0U) {
    return false;
  }
  if ((bits & kExponentMask) != kExponentMask) {
    return true;
  }
  return (bits & kFractionMask) == 0U;
}

void set_enclosure_audit(
    SpatialBoundsAudit& audit,
    const std::array<DirectedEnclosure, 3>& query_enclosures,
    const DirectedEnclosure& cutoff_enclosure) {
  for (std::size_t axis = 0U; axis < query_enclosures.size(); ++axis) {
    audit.query_lower_bits[axis] = query_enclosures[axis].lower_bits;
    audit.query_upper_bits[axis] = query_enclosures[axis].upper_bits;
    audit.query_enclosure[axis] = query_enclosures[axis].status;
  }
  audit.cutoff_lower_bits = cutoff_enclosure.lower_bits;
  audit.cutoff_upper_bits = cutoff_enclosure.upper_bits;
  audit.cutoff_enclosure = cutoff_enclosure.status;
}

[[nodiscard]] bool enclosure_supported(
    const std::array<DirectedEnclosure, 3>& query_enclosures,
    const DirectedEnclosure& cutoff_enclosure) noexcept {
  if (cutoff_enclosure.status == DirectedEnclosureStatus::unsupported_range) {
    return false;
  }
  for (const DirectedEnclosure& enclosure : query_enclosures) {
    if (enclosure.status == DirectedEnclosureStatus::unsupported_range) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] SpatialBoundsResult unsupported_result(
    std::size_t box_count,
    const std::array<DirectedEnclosure, 3>& query_enclosures,
    const DirectedEnclosure& cutoff_enclosure) {
  SpatialBoundsResult result;
  result.decisions.assign(box_count, SpatialBoundsDecision::unknown);
  result.audit.gpu_input_box_count = box_count;
  result.audit.unsupported_range_fallback_count = box_count;
  result.audit.cpu_exact_recertification_complete = true;
  result.audit.all_boxes_classified = true;
  set_enclosure_audit(
      result.audit, query_enclosures, cutoff_enclosure);
  return result;
}

[[nodiscard]] SpatialBoundsResult validate_and_recertify(
    const detail::SpatialBoundsProposalBatch& batch,
    std::span<const spatial::ExactDyadicAabb3> boxes,
    const ExactRational3& query,
    const exact::ExactLevel& squared_cutoff,
    const std::array<DirectedEnclosure, 3>& query_enclosures,
    const DirectedEnclosure& cutoff_enclosure) {
  SpatialBoundsResult result;
  result.decisions.assign(boxes.size(), SpatialBoundsDecision::unknown);
  SpatialBoundsAudit& audit = result.audit;
  audit.gpu_input_box_count = boxes.size();
  audit.gpu_output_record_count = batch.records.size();
  audit.gpu_launch_count = 1U;
  audit.buffer_epoch = batch.buffer_epoch;
  set_enclosure_audit(audit, query_enclosures, cutoff_enclosure);

  if (batch.records.size() != boxes.size()) {
    throw std::runtime_error(
        "the GPU spatial-bounds proposal did not return one record per AABB");
  }
  std::vector<unsigned char> seen(boxes.size(), 0U);
  std::vector<detail::SpatialBoundsProposalRecord> records_by_index(
      boxes.size());
  for (const detail::SpatialBoundsProposalRecord& record : batch.records) {
    if (!std::in_range<std::size_t>(record.box_index) ||
        static_cast<std::size_t>(record.box_index) >= boxes.size()) {
      throw std::runtime_error(
          "the GPU spatial-bounds proposal returned an AABB index outside its batch");
    }
    const std::size_t index = static_cast<std::size_t>(record.box_index);
    if (seen[index] != 0U) {
      throw std::runtime_error(
          "the GPU spatial-bounds proposal repeated an AABB index");
    }
    seen[index] = 1U;
    ++audit.gpu_unique_box_index_count;

    if (!is_nonnegative_interval_word(
            record.lower_squared_distance_bits) ||
        !is_nonnegative_interval_word(
            record.upper_squared_distance_bits) ||
        record.lower_squared_distance_bits == kPositiveInfinityBits ||
        (record.upper_squared_distance_bits != kPositiveInfinityBits &&
         record.lower_squared_distance_bits >
             record.upper_squared_distance_bits)) {
      throw std::runtime_error(
          "the GPU spatial-bounds proposal returned an invalid distance interval");
    }

    switch (record.decision_code) {
      case detail::spatial_bounds_prune_code:
        if (record.upper_squared_distance_bits == kPositiveInfinityBits ||
            record.lower_squared_distance_bits <=
                cutoff_enclosure.upper_bits) {
          throw std::runtime_error(
              "the GPU spatial-bounds prune proposal lacks a strict FP64 margin");
        }
        result.decisions[index] = SpatialBoundsDecision::prune;
        ++audit.gpu_prune_proposal_count;
        break;
      case detail::spatial_bounds_visit_code:
        if (record.upper_squared_distance_bits == kPositiveInfinityBits ||
            record.upper_squared_distance_bits >= cutoff_enclosure.lower_bits) {
          throw std::runtime_error(
              "the GPU spatial-bounds visit proposal lacks a strict FP64 margin");
        }
        result.decisions[index] = SpatialBoundsDecision::visit;
        ++audit.gpu_visit_proposal_count;
        break;
      case detail::spatial_bounds_unknown_code:
        ++audit.gpu_unknown_proposal_count;
        break;
      default:
        throw std::runtime_error(
            "the GPU spatial-bounds proposal returned an invalid decision code");
    }
    records_by_index[index] = record;
  }
  if (audit.gpu_unique_box_index_count != boxes.size() ||
      audit.gpu_prune_proposal_count + audit.gpu_visit_proposal_count +
              audit.gpu_unknown_proposal_count !=
          boxes.size()) {
    throw std::runtime_error(
        "the GPU spatial-bounds proposal did not close its exhaustive permutation");
  }
  audit.proposal_permutation_complete = true;

  std::uint64_t digest = kFnvOffsetBasis;
  for (std::size_t index = 0U; index < boxes.size(); ++index) {
    const detail::SpatialBoundsProposalRecord& record = records_by_index[index];
    hash_word(digest, static_cast<std::uint64_t>(index));
    hash_word(digest, record.lower_squared_distance_bits);
    hash_word(digest, record.upper_squared_distance_bits);
    hash_word(digest, record.decision_code);

    if (result.decisions[index] != SpatialBoundsDecision::prune) {
      continue;
    }
    const exact::ExactLevel lower_bound =
        exact_minimum_squared_distance(boxes[index], query);
    ++audit.cpu_exact_prune_recertification_count;
    if (lower_bound <= squared_cutoff) {
      throw std::runtime_error(
          "the GPU spatial-bounds proposal attempted a false strict prune");
    }
    const exact::ExactLevel margin{
        lower_bound.rational() - squared_cutoff.rational()};
    if (!audit.minimum_certified_strict_margin.has_value() ||
        margin < *audit.minimum_certified_strict_margin) {
      audit.minimum_certified_strict_margin = margin;
    }
    ++audit.certified_prune_count;
  }
  if (audit.cpu_exact_prune_recertification_count !=
          audit.gpu_prune_proposal_count ||
      audit.certified_prune_count != audit.gpu_prune_proposal_count ||
      (audit.certified_prune_count == 0U) !=
          !audit.minimum_certified_strict_margin.has_value()) {
    throw std::logic_error(
        "the exact spatial-bounds prune recertification did not close");
  }
  audit.proposal_digest_fnv1a = digest;
  audit.cpu_exact_recertification_complete = true;
  audit.all_boxes_classified = true;
  return result;
}

}  // namespace

SpatialBoundsContext::SpatialBoundsContext(
    std::span<const spatial::ExactDyadicAabb3> boxes)
    : state_(std::make_shared<detail::SpatialBoundsContextState>()),
      boxes_(boxes.begin(), boxes.end()) {
  if (boxes_.empty()) {
    throw std::invalid_argument(
        "a GPU spatial-bounds context requires at least one AABB");
  }
  packed_boxes_.reserve(boxes_.size());
  for (const spatial::ExactDyadicAabb3& box : boxes_) {
    validate_box(box);
    packed_boxes_.push_back(detail::SpatialBoundsInputRecord{
        {box.lower_binary64_bits[0],
         box.lower_binary64_bits[1],
         box.lower_binary64_bits[2]},
        {box.upper_binary64_bits[0],
         box.upper_binary64_bits[1],
         box.upper_binary64_bits[2]}});
  }
}

SpatialBoundsContext::~SpatialBoundsContext() noexcept = default;
SpatialBoundsContext::SpatialBoundsContext(SpatialBoundsContext&&) noexcept =
    default;
SpatialBoundsContext& SpatialBoundsContext::operator=(
    SpatialBoundsContext&&) noexcept = default;

SpatialBoundsResult SpatialBoundsContext::classify_strict_prune(
    const ExactRational3& query,
    const exact::ExactLevel& squared_cutoff) {
  if (state_ == nullptr || boxes_.empty() ||
      packed_boxes_.size() != boxes_.size()) {
    throw std::invalid_argument(
        "a moved-from GPU spatial-bounds context is not queryable");
  }
  const ExactRational3 canonical_query = validated_query(query);
  const exact::ExactLevel canonical_cutoff = validated_cutoff(squared_cutoff);

  std::array<DirectedEnclosure, 3> query_enclosures{};
  std::array<std::uint64_t, 3> query_lower_bits{};
  std::array<std::uint64_t, 3> query_upper_bits{};
  for (std::size_t axis = 0U; axis < query_enclosures.size(); ++axis) {
    query_enclosures[axis] =
        enclose_rational(canonical_query.coordinate(axis));
    query_lower_bits[axis] = query_enclosures[axis].lower_bits;
    query_upper_bits[axis] = query_enclosures[axis].upper_bits;
  }
  const DirectedEnclosure cutoff_enclosure =
      enclose_nonnegative_rational(canonical_cutoff.rational());
  if (!enclosure_supported(query_enclosures, cutoff_enclosure)) {
    return unsupported_result(
        boxes_.size(), query_enclosures, cutoff_enclosure);
  }

  return state_->with_gpu_section([&] {
    const detail::SpatialBoundsProposalBatch batch =
        detail::propose_strict_aabb_prunes_on_gpu(
            *state_,
            packed_boxes_,
            query_lower_bits,
            query_upper_bits,
            cutoff_enclosure.lower_bits,
            cutoff_enclosure.upper_bits);
    return validate_and_recertify(
        batch,
        boxes_,
        canonical_query,
        canonical_cutoff,
        query_enclosures,
        cutoff_enclosure);
  });
}

}  // namespace morsehgp3d::gpu
