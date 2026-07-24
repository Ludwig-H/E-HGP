#include "morsehgp3d/hierarchy/direct_morse_budget.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::string_view snapshot_digest_domain =
    "MorseHGP3D/phase15/direct-morse-budget-snapshot/v1";

struct CheckedSum {
  std::uint64_t value{};
  bool overflow{false};
};

[[nodiscard]] CheckedSum checked_add(
    std::uint64_t left,
    std::uint64_t right) noexcept {
  if (right > std::numeric_limits<std::uint64_t>::max() - left) {
    return CheckedSum{
        std::numeric_limits<std::uint64_t>::max(), true};
  }
  return CheckedSum{left + right, false};
}

[[nodiscard]] bool valid_boundary(
    ExactDirectMorseBudgetBoundary boundary) noexcept {
  switch (boundary) {
    case ExactDirectMorseBudgetBoundary::before_batch:
    case ExactDirectMorseBudgetBoundary::before_run:
    case ExactDirectMorseBudgetBoundary::before_merge:
    case ExactDirectMorseBudgetBoundary::before_serialization:
    case ExactDirectMorseBudgetBoundary::checkpoint:
    case ExactDirectMorseBudgetBoundary::final:
      return true;
  }
  return false;
}

void append_u8(
    contract::CanonicalSha256Builder& builder,
    std::uint8_t value) {
  const std::array<std::uint8_t, 1U> bytes{value};
  builder.update(bytes);
}

void append_bool(
    contract::CanonicalSha256Builder& builder,
    bool value) {
  append_u8(builder, value ? std::uint8_t{1U} : std::uint8_t{0U});
}

void append_u32(
    contract::CanonicalSha256Builder& builder,
    std::uint32_t value) {
  std::array<std::uint8_t, 4U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    const std::size_t shift = (bytes.size() - 1U - index) * 8U;
    bytes[index] = static_cast<std::uint8_t>(value >> shift);
  }
  builder.update(bytes);
}

void append_u64(
    contract::CanonicalSha256Builder& builder,
    std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    const std::size_t shift = (bytes.size() - 1U - index) * 8U;
    bytes[index] = static_cast<std::uint8_t>(value >> shift);
  }
  builder.update(bytes);
}

void append_text(
    contract::CanonicalSha256Builder& builder,
    std::string_view text) {
  static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));
  append_u64(builder, static_cast<std::uint64_t>(text.size()));
  builder.update(text);
}

void append_policy(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseEffectiveBudgetPolicy& policy) {
  append_u64(builder, policy.device_limit_bytes);
  append_u64(builder, policy.host_limit_bytes);
  append_u64(builder, policy.scratch_limit_bytes);
  append_u64(builder, policy.output_limit_bytes);
  append_u64(builder, policy.time_limit_ns);
}

void append_byte_demand(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseByteBudgetDemand& demand) {
  append_u64(builder, demand.used_bytes);
  append_u64(builder, demand.reserved_bytes);
}

void append_scratch_demand(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseScratchBudgetDemand& demand) {
  append_u64(builder, demand.used_bytes);
  append_u64(builder, demand.temporary_reserved_bytes);
  append_u64(builder, demand.worst_merge_reserved_bytes);
  append_u64(builder, demand.checkpoint_reserved_bytes);
  append_u64(builder, demand.safety_margin_reserved_bytes);
}

void append_time_demand(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseTimeBudgetDemand& demand) {
  append_u64(builder, demand.operation_reserved_ns);
  append_u64(builder, demand.checkpoint_reserved_ns);
}

void append_demand(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseBudgetDemand& demand) {
  append_byte_demand(builder, demand.device);
  append_byte_demand(builder, demand.host);
  append_scratch_demand(builder, demand.scratch);
  append_byte_demand(builder, demand.output);
  append_time_demand(builder, demand.time);
}

void append_byte_axis(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseByteBudgetAxisSnapshot& axis) {
  append_u64(builder, axis.limit_bytes);
  append_u64(builder, axis.used_bytes);
  append_u64(builder, axis.reserved_bytes);
  append_u64(builder, axis.remaining_bytes);
  append_bool(builder, axis.addition_overflow);
  append_bool(builder, axis.within_budget);
}

void append_time_axis(
    contract::CanonicalSha256Builder& builder,
    const ExactDirectMorseTimeBudgetAxisSnapshot& axis) {
  append_u64(builder, axis.limit_ns);
  append_u64(builder, axis.elapsed_ns);
  append_u64(builder, axis.operation_reserved_ns);
  append_u64(builder, axis.checkpoint_reserved_ns);
  append_u64(builder, axis.reserved_ns);
  append_u64(builder, axis.remaining_ns);
  append_bool(builder, axis.clock_regressed);
  append_bool(builder, axis.addition_overflow);
  append_bool(builder, axis.within_budget);
}

[[nodiscard]] contract::CanonicalId canonical_snapshot_digest(
    const ExactDirectMorseBudgetSnapshot& snapshot) {
  contract::CanonicalSha256Builder builder;
  append_text(builder, snapshot_digest_domain);
  append_text(builder, ExactDirectMorseBudgetSnapshot::backend);
  append_text(builder, ExactDirectMorseBudgetSnapshot::profile);
  append_text(builder, ExactDirectMorseBudgetSnapshot::mode);
  append_text(
      builder, ExactDirectMorseBudgetSnapshot::deployment_status);
  append_text(builder, ExactDirectMorseBudgetSnapshot::public_status);
  append_text(builder, ExactDirectMorseBudgetSnapshot::proof_basis);
  append_u32(builder, snapshot.schema_version);
  append_u8(
      builder, static_cast<std::uint8_t>(snapshot.boundary));
  append_u64(builder, snapshot.sequence_number);
  append_policy(builder, snapshot.effective_policy);
  append_demand(builder, snapshot.requested_demand);
  append_byte_axis(builder, snapshot.device);
  append_byte_axis(builder, snapshot.host);
  append_byte_axis(builder, snapshot.scratch);
  append_byte_axis(builder, snapshot.output);
  append_time_axis(builder, snapshot.time);
  append_u8(
      builder,
      static_cast<std::uint8_t>(snapshot.first_refusal_reason));
  append_u8(
      builder, static_cast<std::uint8_t>(snapshot.decision));
  append_bool(builder, snapshot.accounting_evaluation_complete);
  append_bool(builder, snapshot.deterministic_refusal_order_applied);
  append_bool(
      builder, snapshot.scratch_reserve_components_accounted_together);
  append_bool(builder, snapshot.arithmetic_overflow_detected);
  append_bool(builder, snapshot.monotonic_clock_certified);
  append_bool(builder, snapshot.public_v2_projection_claimed);
  append_bool(
      builder, snapshot.public_v2_time_reserve_representable);
  append_bool(builder, snapshot.architecture_only);
  append_bool(builder, snapshot.hierarchy_or_scientific_state_mutated);
  append_bool(
      builder, snapshot.forbidden_global_structure_materialized);
  append_bool(builder, snapshot.public_status_claimed);
  return builder.finalize();
}

[[nodiscard]] ExactDirectMorseByteBudgetAxisSnapshot evaluate_byte_axis(
    std::uint64_t limit,
    std::uint64_t used,
    std::uint64_t reserved,
    bool prior_overflow = false) noexcept {
  ExactDirectMorseByteBudgetAxisSnapshot axis;
  axis.limit_bytes = limit;
  axis.used_bytes = used;
  axis.reserved_bytes = reserved;
  const CheckedSum total = checked_add(used, reserved);
  axis.addition_overflow = prior_overflow || total.overflow;
  axis.within_budget =
      !axis.addition_overflow && total.value <= limit;
  axis.remaining_bytes =
      axis.within_budget ? limit - total.value : 0U;
  return axis;
}

[[nodiscard]] ExactDirectMorseByteBudgetAxisSnapshot
evaluate_scratch_axis(
    std::uint64_t limit,
    const ExactDirectMorseScratchBudgetDemand& demand) noexcept {
  CheckedSum reserve{
      demand.temporary_reserved_bytes, false};
  const auto add_reserve =
      [&reserve](std::uint64_t value) noexcept {
        if (reserve.overflow) {
          return;
        }
        reserve = checked_add(reserve.value, value);
      };
  add_reserve(demand.worst_merge_reserved_bytes);
  add_reserve(demand.checkpoint_reserved_bytes);
  add_reserve(demand.safety_margin_reserved_bytes);
  return evaluate_byte_axis(
      limit,
      demand.used_bytes,
      reserve.value,
      reserve.overflow);
}

[[nodiscard]] ExactDirectMorseTimeBudgetAxisSnapshot evaluate_time_axis(
    std::uint64_t limit,
    std::uint64_t elapsed,
    const ExactDirectMorseTimeBudgetDemand& demand,
    bool clock_regressed,
    bool elapsed_overflow) noexcept {
  ExactDirectMorseTimeBudgetAxisSnapshot axis;
  axis.limit_ns = limit;
  axis.elapsed_ns = elapsed;
  axis.operation_reserved_ns = demand.operation_reserved_ns;
  axis.checkpoint_reserved_ns = demand.checkpoint_reserved_ns;
  const CheckedSum reserve = checked_add(
      demand.operation_reserved_ns,
      demand.checkpoint_reserved_ns);
  axis.reserved_ns = reserve.value;
  const CheckedSum total = checked_add(elapsed, reserve.value);
  axis.clock_regressed = clock_regressed;
  axis.addition_overflow =
      elapsed_overflow || reserve.overflow || total.overflow;
  axis.within_budget =
      !axis.clock_regressed && !axis.addition_overflow &&
      total.value <= limit;
  axis.remaining_ns =
      axis.within_budget ? limit - total.value : 0U;
  return axis;
}

[[nodiscard]] std::uint64_t steady_clock_nanoseconds() noexcept {
  static_assert(std::chrono::steady_clock::is_steady);
  const auto duration =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch());
  const auto count = duration.count();
  if (count <= 0) {
    return 0U;
  }
  return static_cast<std::uint64_t>(count);
}

}  // namespace

ExactDirectMorseBudgetTracker::ExactDirectMorseBudgetTracker(
    ExactDirectMorseEffectiveBudgetPolicy policy,
    std::uint64_t committed_elapsed_ns,
    ExactDirectMorseBudgetNanosecondClock clock)
    : policy_(policy),
      clock_(std::move(clock)),
      accumulated_elapsed_ns_(committed_elapsed_ns) {
  if (!clock_) {
    clock_ = steady_clock_nanoseconds;
  }
  last_clock_ns_ = clock_();
}

ExactDirectMorseBudgetSnapshot
ExactDirectMorseBudgetTracker::snapshot(
    ExactDirectMorseBudgetBoundary boundary,
    std::uint64_t sequence_number,
    const ExactDirectMorseBudgetDemand& demand) {
  if (!valid_boundary(boundary)) {
    throw std::invalid_argument(
        "a Phase-15 budget snapshot requires a canonical boundary");
  }

  const std::uint64_t observed_clock_ns = clock_();
  if (!clock_regression_latched_ && !elapsed_overflow_latched_) {
    if (observed_clock_ns < last_clock_ns_) {
      clock_regression_latched_ = true;
    } else {
      const std::uint64_t delta =
          observed_clock_ns - last_clock_ns_;
      const CheckedSum elapsed =
          checked_add(accumulated_elapsed_ns_, delta);
      last_clock_ns_ = observed_clock_ns;
      if (elapsed.overflow) {
        accumulated_elapsed_ns_ =
            std::numeric_limits<std::uint64_t>::max();
        elapsed_overflow_latched_ = true;
      } else {
        accumulated_elapsed_ns_ = elapsed.value;
      }
    }
  }

  ExactDirectMorseBudgetSnapshot result;
  result.boundary = boundary;
  result.sequence_number = sequence_number;
  result.effective_policy = policy_;
  result.requested_demand = demand;
  result.device = evaluate_byte_axis(
      policy_.device_limit_bytes,
      demand.device.used_bytes,
      demand.device.reserved_bytes);
  result.host = evaluate_byte_axis(
      policy_.host_limit_bytes,
      demand.host.used_bytes,
      demand.host.reserved_bytes);
  result.scratch =
      evaluate_scratch_axis(policy_.scratch_limit_bytes, demand.scratch);
  result.output = evaluate_byte_axis(
      policy_.output_limit_bytes,
      demand.output.used_bytes,
      demand.output.reserved_bytes);
  result.time = evaluate_time_axis(
      policy_.time_limit_ns,
      accumulated_elapsed_ns_,
      demand.time,
      clock_regression_latched_,
      elapsed_overflow_latched_);

  result.accounting_evaluation_complete = true;
  result.deterministic_refusal_order_applied = true;
  result.scratch_reserve_components_accounted_together = true;
  result.arithmetic_overflow_detected =
      result.device.addition_overflow ||
      result.host.addition_overflow ||
      result.scratch.addition_overflow ||
      result.output.addition_overflow ||
      result.time.addition_overflow;
  result.monotonic_clock_certified =
      !result.time.clock_regressed;
  result.public_v2_projection_claimed = false;
  result.public_v2_time_reserve_representable = false;
  result.architecture_only = true;
  result.hierarchy_or_scientific_state_mutated = false;
  result.forbidden_global_structure_materialized = false;
  result.public_status_claimed = false;

  const auto reject_byte_axis =
      [&result](
          ExactDirectMorseBudgetRefusalReason reason,
          const ExactDirectMorseByteBudgetAxisSnapshot& axis) {
        result.first_refusal_reason = reason;
        result.decision =
            axis.addition_overflow
                ? ExactDirectMorseBudgetDecision::
                      rejected_arithmetic_overflow
                : ExactDirectMorseBudgetDecision::
                      rejected_limit_exceeded;
      };

  if (!result.device.within_budget) {
    reject_byte_axis(
        ExactDirectMorseBudgetRefusalReason::device, result.device);
  } else if (!result.host.within_budget) {
    reject_byte_axis(
        ExactDirectMorseBudgetRefusalReason::host, result.host);
  } else if (!result.scratch.within_budget) {
    reject_byte_axis(
        ExactDirectMorseBudgetRefusalReason::scratch, result.scratch);
  } else if (!result.output.within_budget) {
    reject_byte_axis(
        ExactDirectMorseBudgetRefusalReason::output, result.output);
  } else if (!result.time.within_budget) {
    result.first_refusal_reason =
        ExactDirectMorseBudgetRefusalReason::time;
    if (result.time.addition_overflow) {
      result.decision =
          ExactDirectMorseBudgetDecision::
              rejected_arithmetic_overflow;
    } else if (result.time.clock_regressed) {
      result.decision =
          ExactDirectMorseBudgetDecision::
              rejected_clock_regression;
    } else {
      result.decision =
          ExactDirectMorseBudgetDecision::
              rejected_limit_exceeded;
    }
  } else {
    result.first_refusal_reason =
        ExactDirectMorseBudgetRefusalReason::none;
    result.decision = ExactDirectMorseBudgetDecision::accepted;
  }

  result.snapshot_digest = canonical_snapshot_digest(result);
  return result;
}

bool verify_exact_direct_morse_budget_snapshot_digest(
    const ExactDirectMorseBudgetSnapshot& snapshot) {
  return snapshot.snapshot_digest == canonical_snapshot_digest(snapshot);
}

}  // namespace morsehgp3d::hierarchy
