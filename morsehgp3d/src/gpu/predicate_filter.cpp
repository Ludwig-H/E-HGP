#include "morsehgp3d/gpu/predicate_filter.hpp"

#include "morsehgp3d/exact/binary64.hpp"
#include "morsehgp3d/exact/label.hpp"
#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/exact/predicates.hpp"
#include "phase2b_distance_filter_internal.hpp"
#include "phase2b_orientation_filter_internal.hpp"
#include "phase2b_power_bisector_filter_internal.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <future>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu {
namespace {

using exact::CertifiedPoint3;
using exact::CertificationStage;
using exact::ExactLabelMoments;
using exact::ExactRational;
using exact::ExactRational3;
using exact::PredicateDecision;
using exact::PredicateFilterPolicy;
using exact::PredicateSign;

static_assert(
    maximum_power_bisector_cardinality ==
    exact::maximum_power_label_cardinality);

struct IndexedDecision {
  std::size_t index{0};
  PredicateDecision decision{
      PredicateSign::zero, CertificationStage::cpu_multiprecision};
};

class PostGpuPublicationGuard final {
 public:
  explicit PostGpuPublicationGuard(
      detail::PredicateFilterContextState& context) noexcept
      : context_(context) {}

  PostGpuPublicationGuard(const PostGpuPublicationGuard&) = delete;
  PostGpuPublicationGuard& operator=(const PostGpuPublicationGuard&) = delete;

  ~PostGpuPublicationGuard() {
    if (armed_) {
      context_.mark_poisoned();
    }
  }

  void arm() noexcept { armed_ = true; }
  void release() noexcept { armed_ = false; }

 private:
  detail::PredicateFilterContextState& context_;
  bool armed_{false};
};

void require_finite_words(
    const std::array<std::uint64_t, 3>& words,
    const char* label) {
  for (const std::uint64_t word : words) {
    if (!exact::is_finite_binary64_bits(word)) {
      throw std::invalid_argument(
          std::string{"Phase 2B "} + label +
          " coordinates must contain finite binary64 words");
    }
  }
}

void validate_inputs(std::span<const SquaredDistanceFilterInput> inputs) {
  std::unordered_set<std::uint64_t> replay_ids;
  replay_ids.reserve(inputs.size());
  for (const SquaredDistanceFilterInput& input : inputs) {
    if (!replay_ids.insert(input.replay_id).second) {
      throw std::invalid_argument(
          "Phase 2B replay identifiers must be unique within a batch");
    }
    require_finite_words(input.witness_bits, "witness");
    require_finite_words(input.left_bits, "left");
    require_finite_words(input.right_bits, "right");
  }
}

void validate_inputs(std::span<const Orientation3DFilterInput> inputs) {
  std::unordered_set<std::uint64_t> replay_ids;
  replay_ids.reserve(inputs.size());
  for (const Orientation3DFilterInput& input : inputs) {
    if (!replay_ids.insert(input.replay_id).second) {
      throw std::invalid_argument(
          "Phase 2B replay identifiers must be unique within a batch");
    }
    require_finite_words(input.a_bits, "orientation point a");
    require_finite_words(input.b_bits, "orientation point b");
    require_finite_words(input.c_bits, "orientation point c");
    require_finite_words(input.d_bits, "orientation point d");
  }
}

[[nodiscard]] bool same_canonical_point(
    const std::array<std::uint64_t, 3>& left,
    const std::array<std::uint64_t, 3>& right) {
  for (std::size_t axis = 0U; axis < left.size(); ++axis) {
    if (exact::canonicalize_binary64_bits(left[axis]) !=
        exact::canonicalize_binary64_bits(right[axis])) {
      return false;
    }
  }
  return true;
}

void require_canonical_power_label(
    const std::array<PowerBisectorLabelPoint,
                     maximum_power_bisector_cardinality>& points,
    std::size_t cardinality,
    const char* label) {
  bool has_previous = false;
  std::uint32_t previous = 0U;
  for (std::size_t index = 0U; index < cardinality; ++index) {
    const PowerBisectorLabelPoint& point = points[index];
    if (has_previous && point.point_id <= previous) {
      throw std::invalid_argument(
          std::string{"Phase 2B power-bisector "} + label +
          " point identifiers must be sorted and unique");
    }
    require_finite_words(point.coordinate_bits, label);
    previous = point.point_id;
    has_previous = true;
  }
  for (std::size_t index = cardinality; index < points.size(); ++index) {
    const PowerBisectorLabelPoint& point = points[index];
    if (point.point_id != 0U ||
        point.coordinate_bits != std::array<std::uint64_t, 3>{}) {
      throw std::invalid_argument(
          std::string{"Phase 2B power-bisector "} + label +
          " trailing label storage must be zero initialized");
    }
  }
}

void validate_inputs(std::span<const PowerBisectorFilterInput> inputs) {
  std::unordered_set<std::uint64_t> replay_ids;
  replay_ids.reserve(inputs.size());
  for (const PowerBisectorFilterInput& input : inputs) {
    if (!replay_ids.insert(input.replay_id).second) {
      throw std::invalid_argument(
          "Phase 2B replay identifiers must be unique within a batch");
    }
    const std::size_t cardinality =
        static_cast<std::size_t>(input.cardinality);
    if (cardinality == 0U ||
        cardinality > maximum_power_bisector_cardinality) {
      throw std::invalid_argument(
          "Phase 2B power-bisector cardinality must be between one and ten");
    }
    require_finite_words(
        input.witness_numerator_bits, "power-bisector witness numerator");
    std::array<ExactRational, 3> numerators{};
    for (std::size_t axis = 0U; axis < numerators.size(); ++axis) {
      numerators[axis] = ExactRational::from_binary64_bits(
          input.witness_numerator_bits[axis]);
      if (numerators[axis].denominator() != 1) {
        throw std::invalid_argument(
            "Phase 2B power-bisector witness numerators must be exact binary64 integers");
      }
    }
    if (!exact::is_finite_binary64_bits(input.witness_denominator_bits)) {
      throw std::invalid_argument(
          "Phase 2B power-bisector denominator must be finite and strictly positive");
    }
    const ExactRational denominator = ExactRational::from_binary64_bits(
        input.witness_denominator_bits);
    if (denominator.sign() <= 0 || denominator.denominator() != 1) {
      throw std::invalid_argument(
          "Phase 2B power-bisector denominator must be a strictly positive binary64 integer");
    }
    exact::BigInt common_divisor = denominator.numerator();
    for (const ExactRational& numerator : numerators) {
      common_divisor = exact::greatest_common_divisor(
          std::move(common_divisor), numerator.numerator());
    }
    if (common_divisor != 1) {
      throw std::invalid_argument(
          "Phase 2B power-bisector homogeneous witness must be reduced canonically");
    }
    require_canonical_power_label(input.r_points, cardinality, "R label");
    require_canonical_power_label(input.q_points, cardinality, "Q label");
    for (std::size_t r_index = 0U; r_index < cardinality; ++r_index) {
      for (std::size_t q_index = 0U; q_index < cardinality; ++q_index) {
        if (input.r_points[r_index].point_id ==
                input.q_points[q_index].point_id &&
            !same_canonical_point(
                input.r_points[r_index].coordinate_bits,
                input.q_points[q_index].coordinate_bits)) {
          throw std::invalid_argument(
              "Phase 2B power-bisector reused a point identifier with different coordinates");
        }
      }
    }
  }
}

[[nodiscard]] CertifiedPoint3 point_from_words(
    const std::array<std::uint64_t, 3>& words) {
  return CertifiedPoint3::from_binary64_bits(words);
}

[[nodiscard]] ExactRational3 power_witness(
    const PowerBisectorFilterInput& input) {
  const ExactRational denominator = ExactRational::from_binary64_bits(
      input.witness_denominator_bits);
  std::array<ExactRational, 3> coordinates{};
  for (std::size_t axis = 0U; axis < coordinates.size(); ++axis) {
    coordinates[axis] =
        ExactRational::from_binary64_bits(input.witness_numerator_bits[axis]) /
        denominator;
  }
  return ExactRational3{coordinates};
}

[[nodiscard]] ExactLabelMoments power_label(
    const std::array<PowerBisectorLabelPoint,
                     maximum_power_bisector_cardinality>& points,
    std::size_t cardinality) {
  std::vector<CertifiedPoint3> point_table;
  std::vector<std::uint32_t> point_ids;
  point_table.reserve(cardinality);
  point_ids.reserve(cardinality);
  for (std::size_t index = 0U; index < cardinality; ++index) {
    point_table.push_back(point_from_words(points[index].coordinate_bits));
    point_ids.push_back(static_cast<std::uint32_t>(index));
  }
  return ExactLabelMoments::from_canonical_ids(point_ids, point_table);
}

[[nodiscard]] PredicateDecision cpu_decision(
    const SquaredDistanceFilterInput& input,
    PredicateFilterPolicy policy) {
  return exact::decide_squared_distance_order(
      point_from_words(input.witness_bits),
      point_from_words(input.left_bits),
      point_from_words(input.right_bits),
      nullptr,
      policy);
}

[[nodiscard]] PredicateDecision cpu_decision(
    const PowerBisectorFilterInput& input) {
  const std::size_t cardinality =
      static_cast<std::size_t>(input.cardinality);
  return exact::decide_power_bisector_side(
      power_witness(input),
      power_label(input.r_points, cardinality),
      power_label(input.q_points, cardinality));
}

[[nodiscard]] PredicateDecision cpu_decision(
    const Orientation3DFilterInput& input,
    PredicateFilterPolicy policy) {
  return exact::decide_orientation_3d(
      point_from_words(input.a_bits),
      point_from_words(input.b_bits),
      point_from_words(input.c_bits),
      point_from_words(input.d_bits),
      nullptr,
      policy);
}

[[nodiscard]] PredicateSign predicate_sign_from_gpu(FilterSign sign) {
  switch (sign) {
    case FilterSign::negative:
      return PredicateSign::negative;
    case FilterSign::positive:
      return PredicateSign::positive;
    case FilterSign::unknown:
      break;
  }
  throw std::logic_error("a GPU unknown cannot be promoted to a predicate sign");
}

template <typename Counters>
void record_cpu_stage(
    const PredicateDecision& decision,
    Counters& counters) {
  switch (decision.certification_stage()) {
    case CertificationStage::fp64_filtered:
      ++counters.cpu_fp64_filtered_certified;
      break;
    case CertificationStage::expansion:
      ++counters.cpu_expansion_certified;
      break;
    case CertificationStage::cpu_multiprecision:
      ++counters.cpu_multiprecision_certified;
      break;
  }
  if (decision.sign() == PredicateSign::zero) {
    ++counters.exact_zeros;
  }
}

[[nodiscard]] std::vector<IndexedDecision> resolve_unknowns(
    const std::vector<SquaredDistanceFilterInput>& inputs,
    const std::vector<std::size_t>& unknown_indices) {
  std::vector<IndexedDecision> decisions;
  decisions.reserve(unknown_indices.size());
  for (const std::size_t index : unknown_indices) {
    decisions.push_back(IndexedDecision{
        index,
        cpu_decision(inputs[index], PredicateFilterPolicy::allow_adaptive)});
  }
  return decisions;
}

[[nodiscard]] std::vector<IndexedDecision> resolve_unknowns(
    const std::vector<PowerBisectorFilterInput>& inputs,
    const std::vector<std::size_t>& unknown_indices) {
  std::vector<IndexedDecision> decisions;
  decisions.reserve(unknown_indices.size());
  for (const std::size_t index : unknown_indices) {
    decisions.push_back(IndexedDecision{index, cpu_decision(inputs[index])});
  }
  return decisions;
}

[[nodiscard]] std::vector<IndexedDecision> resolve_unknowns(
    const std::vector<Orientation3DFilterInput>& inputs,
    const std::vector<std::size_t>& unknown_indices) {
  std::vector<IndexedDecision> decisions;
  decisions.reserve(unknown_indices.size());
  for (const std::size_t index : unknown_indices) {
    decisions.push_back(IndexedDecision{
        index,
        cpu_decision(inputs[index], PredicateFilterPolicy::allow_adaptive)});
  }
  return decisions;
}

[[nodiscard]] SquaredDistanceBatchResult decide_batch(
    detail::PredicateFilterContextState& context,
    std::vector<SquaredDistanceFilterInput> inputs,
    SquaredDistanceBatchOptions options) {
  validate_inputs(inputs);
  PostGpuPublicationGuard publication_guard{context};
  const std::vector<FilterSign> gpu_outputs =
      detail::filter_squared_distance_signs_on_gpu(context, inputs);
  if (!inputs.empty()) {
    publication_guard.arm();
  }
  if (gpu_outputs.size() != inputs.size()) {
    throw std::runtime_error("the Phase 2B GPU output cardinality changed");
  }

  SquaredDistanceBatchResult result;
  result.decisions.resize(inputs.size());
  result.counters.gpu_inputs = static_cast<std::uint64_t>(inputs.size());
  std::vector<std::size_t> unknown_indices;
  unknown_indices.reserve(inputs.size());

  for (std::size_t index = 0U; index < inputs.size(); ++index) {
    const FilterSign output = gpu_outputs[index];
    switch (output) {
      case FilterSign::negative:
      case FilterSign::positive:
        ++result.counters.gpu_fp64_certified;
        result.decisions[index] = SquaredDistanceDecision{
            inputs[index].replay_id,
            output,
            predicate_sign_from_gpu(output),
            CertificationStage::fp64_filtered};
        break;
      case FilterSign::unknown:
        ++result.counters.gpu_unknown_forwarded;
        unknown_indices.push_back(index);
        break;
      default:
        throw std::runtime_error("the Phase 2B GPU returned an invalid tri-state");
    }
  }

  std::future<std::vector<IndexedDecision>> fallback_future;
  if (!unknown_indices.empty()) {
    ++result.counters.async_fallback_batches;
    fallback_future = std::async(
        std::launch::async,
        [&inputs, indices = unknown_indices] {
          return resolve_unknowns(inputs, indices);
        });
  }

  if (options.audit_gpu_signs) {
    for (std::size_t index = 0U; index < inputs.size(); ++index) {
      if (gpu_outputs[index] == FilterSign::unknown) {
        continue;
      }
      const PredicateDecision oracle = cpu_decision(
          inputs[index], PredicateFilterPolicy::multiprecision_only);
      if (oracle.sign() != result.decisions[index].sign) {
        throw std::runtime_error(
            "the Phase 2B GPU filter contradicted the CPU multiprecision oracle");
      }
      ++result.counters.gpu_known_audited;
    }
  }

  if (!unknown_indices.empty()) {
    for (const IndexedDecision& resolved : fallback_future.get()) {
      const std::size_t index = resolved.index;
      result.decisions[index] = SquaredDistanceDecision{
          inputs[index].replay_id,
          FilterSign::unknown,
          resolved.decision.sign(),
          resolved.decision.certification_stage()};
      record_cpu_stage(resolved.decision, result.counters);
    }
  }
  result.counters.remaining_unknown = 0U;
  publication_guard.release();
  return result;
}

[[nodiscard]] Orientation3DBatchResult decide_batch(
    detail::PredicateFilterContextState& context,
    std::vector<Orientation3DFilterInput> inputs,
    Orientation3DBatchOptions options) {
  validate_inputs(
      std::span<const Orientation3DFilterInput>{inputs.data(), inputs.size()});
  PostGpuPublicationGuard publication_guard{context};
  const std::vector<FilterSign> gpu_outputs =
      detail::filter_orientation_3d_signs_on_gpu(context, inputs);
  if (!inputs.empty()) {
    publication_guard.arm();
  }
  if (gpu_outputs.size() != inputs.size()) {
    throw std::runtime_error(
        "the Phase 2B orientation GPU output cardinality changed");
  }

  Orientation3DBatchResult result;
  result.decisions.resize(inputs.size());
  result.counters.gpu_inputs = static_cast<std::uint64_t>(inputs.size());
  std::vector<std::size_t> unknown_indices;
  unknown_indices.reserve(inputs.size());

  for (std::size_t index = 0U; index < inputs.size(); ++index) {
    const FilterSign output = gpu_outputs[index];
    switch (output) {
      case FilterSign::negative:
      case FilterSign::positive:
        ++result.counters.gpu_fp64_certified;
        result.decisions[index] = Orientation3DDecision{
            inputs[index].replay_id,
            output,
            predicate_sign_from_gpu(output),
            CertificationStage::fp64_filtered};
        break;
      case FilterSign::unknown:
        ++result.counters.gpu_unknown_forwarded;
        unknown_indices.push_back(index);
        break;
      default:
        throw std::runtime_error(
            "the Phase 2B orientation GPU returned an invalid tri-state");
    }
  }

  std::future<std::vector<IndexedDecision>> fallback_future;
  if (!unknown_indices.empty()) {
    ++result.counters.async_fallback_batches;
    fallback_future = std::async(
        std::launch::async,
        [&inputs, indices = unknown_indices] {
          return resolve_unknowns(inputs, indices);
        });
  }

  if (options.audit_gpu_signs) {
    for (std::size_t index = 0U; index < inputs.size(); ++index) {
      if (gpu_outputs[index] == FilterSign::unknown) {
        continue;
      }
      const PredicateDecision oracle = cpu_decision(
          inputs[index], PredicateFilterPolicy::multiprecision_only);
      if (oracle.sign() != result.decisions[index].sign) {
        throw std::runtime_error(
            "the Phase 2B orientation GPU filter contradicted the CPU "
            "multiprecision oracle");
      }
      ++result.counters.gpu_known_audited;
    }
  }

  if (!unknown_indices.empty()) {
    for (const IndexedDecision& resolved : fallback_future.get()) {
      const std::size_t index = resolved.index;
      result.decisions[index] = Orientation3DDecision{
          inputs[index].replay_id,
          FilterSign::unknown,
          resolved.decision.sign(),
          resolved.decision.certification_stage()};
      record_cpu_stage(resolved.decision, result.counters);
    }
  }
  result.counters.remaining_unknown = 0U;
  publication_guard.release();
  return result;
}

[[nodiscard]] PowerBisectorBatchResult decide_batch(
    detail::PredicateFilterContextState& context,
    std::vector<PowerBisectorFilterInput> inputs,
    PowerBisectorBatchOptions options) {
  validate_inputs(
      std::span<const PowerBisectorFilterInput>{inputs.data(), inputs.size()});
  PostGpuPublicationGuard publication_guard{context};
  const std::vector<FilterSign> gpu_outputs =
      detail::filter_power_bisector_signs_on_gpu(context, inputs);
  if (!inputs.empty()) {
    publication_guard.arm();
  }
  if (gpu_outputs.size() != inputs.size()) {
    throw std::runtime_error(
        "the Phase 2B power-bisector GPU output cardinality changed");
  }

  PowerBisectorBatchResult result;
  result.decisions.resize(inputs.size());
  result.counters.gpu_inputs = static_cast<std::uint64_t>(inputs.size());
  std::vector<std::size_t> unknown_indices;
  unknown_indices.reserve(inputs.size());

  for (std::size_t index = 0U; index < inputs.size(); ++index) {
    const FilterSign output = gpu_outputs[index];
    switch (output) {
      case FilterSign::negative:
      case FilterSign::positive:
        ++result.counters.gpu_fp64_certified;
        result.decisions[index] = PowerBisectorDecision{
            inputs[index].replay_id,
            output,
            predicate_sign_from_gpu(output),
            CertificationStage::fp64_filtered};
        break;
      case FilterSign::unknown:
        ++result.counters.gpu_unknown_forwarded;
        unknown_indices.push_back(index);
        break;
      default:
        throw std::runtime_error(
            "the Phase 2B power-bisector GPU returned an invalid tri-state");
    }
  }

  std::future<std::vector<IndexedDecision>> fallback_future;
  if (!unknown_indices.empty()) {
    ++result.counters.async_fallback_batches;
    fallback_future = std::async(
        std::launch::async,
        [&inputs, indices = unknown_indices] {
          return resolve_unknowns(inputs, indices);
        });
  }

  if (options.audit_gpu_signs) {
    for (std::size_t index = 0U; index < inputs.size(); ++index) {
      if (gpu_outputs[index] == FilterSign::unknown) {
        continue;
      }
      const PredicateDecision oracle = cpu_decision(inputs[index]);
      if (oracle.sign() != result.decisions[index].sign) {
        throw std::runtime_error(
            "the Phase 2B power-bisector GPU filter contradicted the CPU "
            "multiprecision oracle");
      }
      ++result.counters.gpu_known_audited;
    }
  }

  if (!unknown_indices.empty()) {
    for (const IndexedDecision& resolved : fallback_future.get()) {
      const std::size_t index = resolved.index;
      result.decisions[index] = PowerBisectorDecision{
          inputs[index].replay_id,
          FilterSign::unknown,
          resolved.decision.sign(),
          resolved.decision.certification_stage()};
      record_cpu_stage(resolved.decision, result.counters);
    }
  }
  result.counters.remaining_unknown = 0U;
  publication_guard.release();
  return result;
}

}  // namespace

PredicateFilterContext::PredicateFilterContext()
    : state_(std::make_shared<detail::PredicateFilterContextState>()) {}

PredicateFilterContext::~PredicateFilterContext() noexcept = default;

PredicateFilterContext::PredicateFilterContext(
    PredicateFilterContext&&) noexcept = default;

PredicateFilterContext& PredicateFilterContext::operator=(
    PredicateFilterContext&&) noexcept = default;

std::future<SquaredDistanceBatchResult> decide_squared_distance_batch_async(
    PredicateFilterContext& context,
    std::vector<SquaredDistanceFilterInput> inputs,
    SquaredDistanceBatchOptions options) {
  if (!context.state_) {
    throw std::logic_error(
        "cannot schedule work on a moved-from predicate filter context");
  }
  std::shared_ptr<detail::PredicateFilterContextState> state = context.state_;
  return std::async(
      std::launch::async,
      [state = std::move(state), owned_inputs = std::move(inputs), options]()
          mutable {
        return decide_batch(*state, std::move(owned_inputs), options);
      });
}

std::future<Orientation3DBatchResult> decide_orientation_3d_batch_async(
    PredicateFilterContext& context,
    std::vector<Orientation3DFilterInput> inputs,
    Orientation3DBatchOptions options) {
  if (!context.state_) {
    throw std::logic_error(
        "cannot schedule work on a moved-from predicate filter context");
  }
  std::shared_ptr<detail::PredicateFilterContextState> state = context.state_;
  return std::async(
      std::launch::async,
      [state = std::move(state), owned_inputs = std::move(inputs), options]()
          mutable {
        return decide_batch(*state, std::move(owned_inputs), options);
      });
}

std::future<PowerBisectorBatchResult> decide_power_bisector_batch_async(
    PredicateFilterContext& context,
    std::vector<PowerBisectorFilterInput> inputs,
    PowerBisectorBatchOptions options) {
  if (!context.state_) {
    throw std::logic_error(
        "cannot schedule work on a moved-from predicate filter context");
  }
  std::shared_ptr<detail::PredicateFilterContextState> state = context.state_;
  return std::async(
      std::launch::async,
      [state = std::move(state), owned_inputs = std::move(inputs), options]()
          mutable {
        return decide_batch(*state, std::move(owned_inputs), options);
      });
}

std::future<SquaredDistanceBatchResult> decide_squared_distance_batch_async(
    std::vector<SquaredDistanceFilterInput> inputs,
    SquaredDistanceBatchOptions options) {
  PredicateFilterContext context;
  return decide_squared_distance_batch_async(
      context, std::move(inputs), options);
}

std::future<Orientation3DBatchResult> decide_orientation_3d_batch_async(
    std::vector<Orientation3DFilterInput> inputs,
    Orientation3DBatchOptions options) {
  PredicateFilterContext context;
  return decide_orientation_3d_batch_async(
      context, std::move(inputs), options);
}

std::future<PowerBisectorBatchResult> decide_power_bisector_batch_async(
    std::vector<PowerBisectorFilterInput> inputs,
    PowerBisectorBatchOptions options) {
  PredicateFilterContext context;
  return decide_power_bisector_batch_async(
      context, std::move(inputs), options);
}

}  // namespace morsehgp3d::gpu
