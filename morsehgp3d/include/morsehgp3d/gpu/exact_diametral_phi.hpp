#pragma once

#include "morsehgp3d/exact/predicate.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace morsehgp3d::gpu {

namespace detail {
class ExactDiametralPhiContextState;
}  // namespace detail

inline constexpr std::string_view exact_diametral_phi_backend =
    "cuda_g4_exact_fixed_limb_plus_cpu_multiprecision_fallback";
inline constexpr std::string_view exact_diametral_phi_profile = "hgp_reduced";
inline constexpr std::string_view exact_diametral_phi_mode =
    "exact_binary64_point_predicate_batch";
inline constexpr std::string_view exact_diametral_phi_deployment_status =
    "architecture_only";
inline constexpr std::string_view exact_diametral_phi_public_status =
    "not_claimed";

// The scientific predicate is
//
//   Phi(x,u,v) = sum_i (x_i-u_i)(x_i-v_i).
//
// All coordinates are finite binary64 words and are interpreted as exact
// dyadic rationals. Signed zero spellings denote the same exact coordinate.
struct ExactDiametralPhiInput {
  std::uint64_t replay_id{};
  std::array<std::uint64_t, 3U> witness_bits{};
  std::array<std::uint64_t, 3U> first_support_bits{};
  std::array<std::uint64_t, 3U> second_support_bits{};

  friend bool operator==(
      const ExactDiametralPhiInput&,
      const ExactDiametralPhiInput&) = default;
};
static_assert(std::is_standard_layout_v<ExactDiametralPhiInput>);
static_assert(std::is_trivially_copyable_v<ExactDiametralPhiInput>);

enum class ExactDiametralPhiCertificationStage : std::uint8_t {
  gpu_fp64_interval = 0U,
  gpu_fixed_limb_dyadic = 1U,
  cpu_multiprecision = 2U,
};

struct ExactDiametralPhiDecision {
  std::uint64_t replay_id{};
  exact::PredicateSign sign{exact::PredicateSign::zero};
  ExactDiametralPhiCertificationStage certification_stage{
      ExactDiametralPhiCertificationStage::cpu_multiprecision};

  friend bool operator==(
      const ExactDiametralPhiDecision&,
      const ExactDiametralPhiDecision&) = default;
};

struct ExactDiametralPhiBatchOptions {
  // Qualification mode independently recomputes every GPU-decided sign with
  // the arbitrary-precision CPU oracle and fails closed on a contradiction.
  bool audit_gpu_signs{false};
};

struct ExactDiametralPhiBatchAudit {
  std::size_t input_count{};
  std::size_t maximum_query_count{};
  std::size_t gpu_fp64_interval_count{};
  std::size_t gpu_fixed_limb_dyadic_count{};
  std::size_t gpu_exact_fallback_count{};
  std::size_t cpu_multiprecision_count{};
  std::size_t exact_zero_count{};
  std::size_t gpu_known_audited_count{};
  std::size_t disagreement_count{};
  std::size_t launcher_call_count{};
  std::size_t cuda_kernel_launch_count{};
  std::size_t cuda_synchronization_count{};
  int cuda_device{-1};
  bool host_fake_launcher_exercised{false};
  bool cuda_execution_performed{false};
  bool all_results_exact{false};
  bool interval_zero_certification_forbidden{true};
  bool epsilon_used{false};
  bool scientific_catalog_decision_published{false};
  bool public_status_claimed{false};

  friend bool operator==(
      const ExactDiametralPhiBatchAudit&,
      const ExactDiametralPhiBatchAudit&) = default;
};

struct ExactDiametralPhiBatchResult {
  std::vector<ExactDiametralPhiDecision> decisions;
  ExactDiametralPhiBatchAudit audit;
};

// One context owns fixed-capacity device storage and one CUDA stream. Calls are
// synchronous and serialized per context; separate contexts may run in
// parallel. Every returned sign is exact. Device uncertainty is never exposed
// as zero: it is resolved by one batched CPU multiprecision fallback.
class ExactDiametralPhiContext final {
 public:
  explicit ExactDiametralPhiContext(std::size_t maximum_query_count);
  ~ExactDiametralPhiContext() noexcept;

  ExactDiametralPhiContext(ExactDiametralPhiContext&&) noexcept;
  ExactDiametralPhiContext& operator=(ExactDiametralPhiContext&&) noexcept;
  ExactDiametralPhiContext(const ExactDiametralPhiContext&) = delete;
  ExactDiametralPhiContext& operator=(const ExactDiametralPhiContext&) = delete;

  [[nodiscard]] ExactDiametralPhiBatchResult decide(
      std::span<const ExactDiametralPhiInput> inputs,
      ExactDiametralPhiBatchOptions options = {});

  [[nodiscard]] std::size_t maximum_query_count() const noexcept {
    return maximum_query_count_;
  }

 private:
  std::shared_ptr<detail::ExactDiametralPhiContextState> state_;
  std::size_t maximum_query_count_{};
};

}  // namespace morsehgp3d::gpu
