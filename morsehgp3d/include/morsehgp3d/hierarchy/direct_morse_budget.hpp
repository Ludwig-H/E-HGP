#pragma once

#include "morsehgp3d/contract/canonical_id.hpp"

#include <cstdint>
#include <functional>
#include <string_view>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t direct_morse_budget_schema_version = 1U;
inline constexpr std::string_view direct_morse_budget_backend =
    "reference_cpu";
inline constexpr std::string_view direct_morse_budget_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_morse_budget_mode = "budgeted";
inline constexpr std::string_view direct_morse_budget_deployment_status =
    "architecture_only";
inline constexpr std::string_view direct_morse_budget_public_status =
    "not_claimed";
inline constexpr std::string_view direct_morse_budget_proof_basis =
    "finite_effective_u64_byte_and_monotonic_nanosecond_limits_checked_"
    "additions_decomposed_transactional_scratch_reserve_deterministic_"
    "first_refusal_and_domain_separated_snapshot_digest_v1";

enum class ExactDirectMorseBudgetBoundary : std::uint8_t {
  before_batch = 1U,
  before_run = 2U,
  before_merge = 3U,
  before_serialization = 4U,
  checkpoint = 5U,
  final = 6U,
};

// This order is normative.  When several axes refuse the same operation, the
// first reason is device, host, scratch, output, then time.
enum class ExactDirectMorseBudgetRefusalReason : std::uint8_t {
  none = 0U,
  device = 1U,
  host = 2U,
  scratch = 3U,
  output = 4U,
  time = 5U,
};

enum class ExactDirectMorseBudgetDecision : std::uint8_t {
  not_evaluated = 0U,
  accepted = 1U,
  rejected_limit_exceeded = 2U,
  rejected_arithmetic_overflow = 3U,
  rejected_clock_regression = 4U,
};

// This is an already resolved, finite execution policy.  Detection-derived
// defaults and the nullable public-v2 BudgetPolicy are resolved before this
// layer.  Durable time accounting uses integer nanoseconds, never binary
// floating-point seconds.
struct ExactDirectMorseEffectiveBudgetPolicy {
  std::uint64_t device_limit_bytes{};
  std::uint64_t host_limit_bytes{};
  std::uint64_t scratch_limit_bytes{};
  std::uint64_t output_limit_bytes{};
  std::uint64_t time_limit_ns{};

  friend bool operator==(
      const ExactDirectMorseEffectiveBudgetPolicy&,
      const ExactDirectMorseEffectiveBudgetPolicy&) = default;
};

struct ExactDirectMorseByteBudgetDemand {
  std::uint64_t used_bytes{};
  std::uint64_t reserved_bytes{};

  friend bool operator==(
      const ExactDirectMorseByteBudgetDemand&,
      const ExactDirectMorseByteBudgetDemand&) = default;
};

// The four reservations must coexist with already used scratch.  In
// particular, a checkpoint reserve is never lent to a run or merge.
struct ExactDirectMorseScratchBudgetDemand {
  std::uint64_t used_bytes{};
  std::uint64_t temporary_reserved_bytes{};
  std::uint64_t worst_merge_reserved_bytes{};
  std::uint64_t checkpoint_reserved_bytes{};
  std::uint64_t safety_margin_reserved_bytes{};

  friend bool operator==(
      const ExactDirectMorseScratchBudgetDemand&,
      const ExactDirectMorseScratchBudgetDemand&) = default;
};

// operation_reserved_ns is the pessimistic duration of the next atomic unit;
// checkpoint_reserved_ns remains available for checkpoint and orderly stop.
struct ExactDirectMorseTimeBudgetDemand {
  std::uint64_t operation_reserved_ns{};
  std::uint64_t checkpoint_reserved_ns{};

  friend bool operator==(
      const ExactDirectMorseTimeBudgetDemand&,
      const ExactDirectMorseTimeBudgetDemand&) = default;
};

struct ExactDirectMorseBudgetDemand {
  ExactDirectMorseByteBudgetDemand device{};
  ExactDirectMorseByteBudgetDemand host{};
  ExactDirectMorseScratchBudgetDemand scratch{};
  ExactDirectMorseByteBudgetDemand output{};
  ExactDirectMorseTimeBudgetDemand time{};

  friend bool operator==(
      const ExactDirectMorseBudgetDemand&,
      const ExactDirectMorseBudgetDemand&) = default;
};

struct ExactDirectMorseByteBudgetAxisSnapshot {
  std::uint64_t limit_bytes{};
  std::uint64_t used_bytes{};
  // Saturates to UINT64_MAX only when summing decomposed scratch reserves
  // overflows.  addition_overflow then makes the axis fail closed.
  std::uint64_t reserved_bytes{};
  std::uint64_t remaining_bytes{};
  bool addition_overflow{false};
  bool within_budget{false};

  friend bool operator==(
      const ExactDirectMorseByteBudgetAxisSnapshot&,
      const ExactDirectMorseByteBudgetAxisSnapshot&) = default;
};

struct ExactDirectMorseTimeBudgetAxisSnapshot {
  std::uint64_t limit_ns{};
  // Accumulated monotonic duration.  Raw steady-clock epochs are never part of
  // a snapshot because they cannot be compared across process restarts.
  std::uint64_t elapsed_ns{};
  std::uint64_t operation_reserved_ns{};
  std::uint64_t checkpoint_reserved_ns{};
  // Saturates to UINT64_MAX when either reserve addition or
  // elapsed-plus-reserve addition overflows.
  std::uint64_t reserved_ns{};
  std::uint64_t remaining_ns{};
  bool clock_regressed{false};
  bool addition_overflow{false};
  bool within_budget{false};

  friend bool operator==(
      const ExactDirectMorseTimeBudgetAxisSnapshot&,
      const ExactDirectMorseTimeBudgetAxisSnapshot&) = default;
};

struct ExactDirectMorseBudgetSnapshot {
  static constexpr std::string_view backend = direct_morse_budget_backend;
  static constexpr std::string_view profile = direct_morse_budget_profile;
  static constexpr std::string_view mode = direct_morse_budget_mode;
  static constexpr std::string_view deployment_status =
      direct_morse_budget_deployment_status;
  static constexpr std::string_view public_status =
      direct_morse_budget_public_status;
  static constexpr std::string_view proof_basis =
      direct_morse_budget_proof_basis;

  std::uint32_t schema_version{direct_morse_budget_schema_version};
  ExactDirectMorseBudgetBoundary boundary{
      ExactDirectMorseBudgetBoundary::before_batch};
  std::uint64_t sequence_number{};
  ExactDirectMorseEffectiveBudgetPolicy effective_policy{};
  ExactDirectMorseBudgetDemand requested_demand{};
  ExactDirectMorseByteBudgetAxisSnapshot device{};
  ExactDirectMorseByteBudgetAxisSnapshot host{};
  ExactDirectMorseByteBudgetAxisSnapshot scratch{};
  ExactDirectMorseByteBudgetAxisSnapshot output{};
  ExactDirectMorseTimeBudgetAxisSnapshot time{};
  ExactDirectMorseBudgetRefusalReason first_refusal_reason{
      ExactDirectMorseBudgetRefusalReason::none};
  ExactDirectMorseBudgetDecision decision{
      ExactDirectMorseBudgetDecision::not_evaluated};
  contract::CanonicalId snapshot_digest{};

  bool accounting_evaluation_complete{false};
  bool deterministic_refusal_order_applied{false};
  bool scratch_reserve_components_accounted_together{false};
  bool arithmetic_overflow_detected{false};
  bool monotonic_clock_certified{false};
  // Public contract v2 BudgetSnapshot has no time_reserved_s field.  A
  // lossless public-v2 projection is therefore deliberately not claimed.
  bool public_v2_projection_claimed{false};
  bool public_v2_time_reserve_representable{false};
  bool architecture_only{true};
  bool hierarchy_or_scientific_state_mutated{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};

  [[nodiscard]] bool accepted() const noexcept {
    return decision == ExactDirectMorseBudgetDecision::accepted &&
           first_refusal_reason ==
               ExactDirectMorseBudgetRefusalReason::none;
  }

  friend bool operator==(
      const ExactDirectMorseBudgetSnapshot&,
      const ExactDirectMorseBudgetSnapshot&) = default;
};

using ExactDirectMorseBudgetNanosecondClock =
    std::function<std::uint64_t()>;

// One process epoch for deterministic boundary snapshots.  committed_elapsed
// carries elapsed duration from an earlier durable epoch.  A clock regression
// or elapsed-duration overflow is latched and every later snapshot fails
// closed on the time axis.
class ExactDirectMorseBudgetTracker {
 public:
  explicit ExactDirectMorseBudgetTracker(
      ExactDirectMorseEffectiveBudgetPolicy policy,
      std::uint64_t committed_elapsed_ns = 0U,
      ExactDirectMorseBudgetNanosecondClock clock = {});

  ExactDirectMorseBudgetTracker(
      const ExactDirectMorseBudgetTracker&) = delete;
  ExactDirectMorseBudgetTracker& operator=(
      const ExactDirectMorseBudgetTracker&) = delete;
  ExactDirectMorseBudgetTracker(
      ExactDirectMorseBudgetTracker&&) = delete;
  ExactDirectMorseBudgetTracker& operator=(
      ExactDirectMorseBudgetTracker&&) = delete;

  [[nodiscard]] ExactDirectMorseBudgetSnapshot snapshot(
      ExactDirectMorseBudgetBoundary boundary,
      std::uint64_t sequence_number,
      const ExactDirectMorseBudgetDemand& demand);

  [[nodiscard]] const ExactDirectMorseEffectiveBudgetPolicy& policy()
      const noexcept {
    return policy_;
  }

 private:
  ExactDirectMorseEffectiveBudgetPolicy policy_{};
  ExactDirectMorseBudgetNanosecondClock clock_;
  std::uint64_t last_clock_ns_{};
  std::uint64_t accumulated_elapsed_ns_{};
  bool clock_regression_latched_{false};
  bool elapsed_overflow_latched_{false};
};

// This checks the canonical byte projection only.  As with every wire
// checksum, it is not scientific provenance and cannot replace a fresh budget
// evaluation from trusted measurements and policy.
[[nodiscard]] bool verify_exact_direct_morse_budget_snapshot_digest(
    const ExactDirectMorseBudgetSnapshot& snapshot);

}  // namespace morsehgp3d::hierarchy
