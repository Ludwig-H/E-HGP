#include "../cuda/phase15_morton_yao48_device_tiled_pair_frontier_internal.hpp"

#include "morsehgp3d/exact/point.hpp"
#include "morsehgp3d/gpu/morton_lbvh_build.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include <cuda_runtime_api.h>

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <set>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#if defined(__linux__)
#include <sys/resource.h>
#endif

namespace {

using Clock = std::chrono::steady_clock;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::MortonLbvhBuildContext;
using morsehgp3d::gpu::MortonLbvhDeviceBuildResult;
using morsehgp3d::gpu::MortonLbvhDeviceTraversalLease;
using morsehgp3d::gpu::MortonYao48DeviceCandidateTileLeaseAudit;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierAdvance;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierAudit;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierConfig;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierContext;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierPruneSemantics;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierStatus;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierStopReason;
using morsehgp3d::gpu::MortonYao48DeviceTiledPairFrontierYieldReason;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledAdoptedTraversal;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledAnchorControl;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledAnchorCheckpoint;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledAnchorStatus;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledYieldReason;
using morsehgp3d::gpu::detail::Phase15MortonYao48DeviceTiledBatch;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledCandidateRecord;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledCachedPruneWitness;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledExecutionKind;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledFailureCode;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledNodeBounds;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledPairFrontierContextState;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledPruneRegionRecord;
using morsehgp3d::gpu::detail::Phase15MortonYao48DeviceTiledRequest;
using morsehgp3d::gpu::detail::
    Phase15MortonYao48DeviceTiledWitnessBankSlot;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhSnapshotNode;
using morsehgp3d::spatial::MortonLeafRecord;
using morsehgp3d::spatial::PointId;

inline constexpr std::string_view kSchema =
    "morsehgp3d.phase15.morton_yao48_device_tiled_pair_frontier_"
    "qualification.v1";
inline constexpr std::string_view kWarmComponentSchema =
    "morsehgp3d.phase15.morton_yao48_device_tiled_pair_frontier_"
    "warm_component_qualification.v1";
inline constexpr std::string_view kScalingSmokeSchema =
    "morsehgp3d.phase15.morton_yao48_device_tiled_pair_frontier_"
    "scaling_smoke.v1";
inline constexpr std::string_view kDirectScaleSchema =
    "morsehgp3d.phase15.morton_yao48_device_tiled_pair_frontier_"
    "direct_scale.v1";
inline constexpr std::string_view kStrictInteriorThresholdSchema =
    "morsehgp3d.phase15.morton_yao48_device_tiled_pair_frontier_"
    "strict_interior_threshold_qualification.v1";
inline constexpr std::string_view kBackend = "cuda_g4";
inline constexpr std::string_view kProfile = "hgp_reduced";
inline constexpr std::string_view kMode =
    "offline_device_tiled_pair_frontier_qualification";
inline constexpr std::string_view kWarmComponentMode =
    "offline_device_tiled_pair_frontier_warm_component_qualification";
inline constexpr std::string_view kStrictInteriorThresholdMode =
    "native_bounded_q3_gabriel_exact_diametral_pair_support_negative_only_"
    "qualification";
inline constexpr std::string_view kStrictInteriorThresholdScientificScope =
    "q3_gabriel_exact_diametral_pair_support_negative_only";
inline constexpr std::string_view kStrictInteriorThresholdFixture =
    "q3_shell_strict_discriminant";
inline constexpr std::string_view kDeploymentStatus = "component_only";
inline constexpr std::string_view kPublicStatus = "not_claimed";

#if defined(MORSEHGP3D_GIT_SHA)
inline constexpr std::string_view kGitSha = MORSEHGP3D_GIT_SHA;
#else
inline constexpr std::string_view kGitSha = "unavailable";
#endif

inline constexpr std::uint64_t kInvalid = UINT64_MAX;
inline constexpr std::uint64_t kCandidateFlagAmbiguousCone =
    UINT64_C(1) << 0U;
inline constexpr std::uint64_t kCandidateFlagCertifiedCone =
    UINT64_C(1) << 1U;
inline constexpr std::uint64_t kCandidateFlagBankInserted =
    UINT64_C(1) << 2U;
inline constexpr std::uint64_t kCandidateFlagBankReplaced =
    UINT64_C(1) << 3U;
inline constexpr std::uint64_t kCandidateKnownFlags =
    kCandidateFlagAmbiguousCone | kCandidateFlagCertifiedCone |
    kCandidateFlagBankInserted | kCandidateFlagBankReplaced;
inline constexpr std::uint64_t kPruneFlagClosedNonnegativeInterval =
    morsehgp3d::gpu::detail::
        phase15_morton_yao48_device_tiled_prune_flag_closed_nonnegative_interval;
inline constexpr std::uint64_t kPruneFlagStrictPositiveInterval =
    morsehgp3d::gpu::detail::
        phase15_morton_yao48_device_tiled_prune_flag_strict_positive_interval;
inline constexpr std::int64_t kCoordinateDenominator =
    INT64_C(1) << 20U;
inline constexpr std::int64_t kMaximumAbsoluteScaledCoordinate =
    INT64_C(64) * kCoordinateDenominator;
inline constexpr std::uint64_t kFnvOffsetBasis =
    UINT64_C(14695981039346656037);
inline constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);

static_assert(std::numeric_limits<long double>::digits >= 64);
static_assert(kMaximumAbsoluteScaledCoordinate < INT64_C(100000000));

struct Options {
  std::size_t point_count{257U};
  std::string family{"adversarial_mixed_dyadic"};
  std::string fixture;
  std::string scientific_scope;
  std::optional<std::size_t> maximum_closed_rank;
  MortonYao48DeviceTiledPairFrontierPruneSemantics prune_semantics{
      MortonYao48DeviceTiledPairFrontierPruneSemantics::closed_rank_window};
  bool all_ranks{false};
  bool scaling_smoke{false};
  bool direct_scale{false};
  bool point_count_explicit{false};
  bool family_explicit{false};
  bool fixture_explicit{false};
  bool scientific_scope_explicit{false};
  bool anchor_tile_capacity_explicit{false};
  std::size_t anchor_tile_capacity{
      morsehgp3d::gpu::
          morton_yao48_device_tiled_pair_frontier_maximum_anchor_tile_capacity};
  std::size_t sampled_prune_limit{4096U};
  std::size_t exact_prune_target_check_limit{1'000'000U};
  // Diagnostic-only deterministic sampling of the per-record host
  // revalidation: when present, only the first N candidate records and the
  // first N prune-region records of each anchor receive the full host
  // validation, the per-anchor partition validation and the witness-bank
  // validation are skipped, and the run reports
  // every_record_validated=false with coverage_partition_complete=false.
  // Absent means the historical full qualification audit.
  std::optional<std::size_t> record_validation_sample_limit;
  std::size_t warmup_run_count{0U};
  std::uint64_t seed{UINT64_C(0x15a048d1e7c93b25)};
};

struct ScaledPoint {
  std::array<std::int64_t, 3U> coordinate{};
};

struct CloudFeatures {
  std::size_t equality_shell_point_count{};
  std::size_t permutation_sign_point_count{};
  std::size_t jitter_grid_point_count{};
  std::size_t clustered_point_count{};
  std::size_t deterministic_fill_point_count{};
};

struct GeneratedCloud {
  std::vector<CertifiedPoint3> points;
  std::vector<ScaledPoint> scaled_points;
  CloudFeatures features{};
};

struct AnchorPartition {
  std::vector<std::uint64_t> candidate_positions;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> prune_intervals;
};

struct QualificationAnchorProgress {
  std::uint64_t candidate_count{};
  std::uint64_t prune_region_count{};
  std::uint64_t certified_pruned_pair_mass{};
  std::uint64_t node_visit_count{};
  std::uint64_t ambiguous_cone_candidate_count{};
  std::uint64_t unbanked_candidate_count{};
  bool complete{false};
};

struct RankMetrics {
  std::size_t maximum_closed_rank{};
  MortonYao48DeviceTiledPairFrontierPruneSemantics prune_semantics{
      MortonYao48DeviceTiledPairFrontierPruneSemantics::closed_rank_window};
  std::size_t required_witness_count{};
  std::size_t tile_count{};
  std::size_t chunk_count{};
  std::size_t resumed_chunk_count{};
  std::size_t fresh_tile_device_arena_allocation_count{};
  std::size_t fresh_tile_device_arena_reuse_count{};
  std::size_t candidate_yield_anchor_count{};
  std::size_t prune_yield_anchor_count{};
  std::size_t complete_anchor_count{};
  std::size_t censored_anchor_count{};
  std::size_t candidate_record_count{};
  std::size_t prune_region_count{};
  std::size_t bank_entry_count{};
  std::size_t ambiguous_candidate_count{};
  std::size_t unbanked_candidate_count{};
  std::size_t fully_recertified_prune_count{};
  std::size_t sampled_recertified_prune_count{};
  std::uint64_t candidate_pair_mass{};
  std::uint64_t certified_pruned_pair_mass{};
  std::uint64_t unresolved_pair_mass{};
  std::uint64_t unordered_pair_universe_count{};
  std::uint64_t physical_node_visit_count{};
  std::uint64_t prune_witness_target_check_count{};
  std::uint64_t bounded_exact_pair_count{};
  std::uint64_t bounded_admitted_pair_count{};
  std::uint64_t bounded_candidate_admitted_pair_count{};
  std::uint64_t bounded_candidate_rejected_pair_count{};
  std::uint64_t bounded_non_support_shell_equality_count{};
  std::uint64_t output_digest{kFnvOffsetBasis};
  std::uint64_t build_ns{};
  std::uint64_t lease_release_ns{};
  std::uint64_t adoption_ns{};
  std::uint64_t node_copy_ns{};
  std::uint64_t launcher_ns{};
  std::uint64_t qualification_output_copy_ns{};
  std::uint64_t cpu_recertification_ns{};
  std::uint64_t bounded_bruteforce_ns{};
  std::uint64_t qualification_device_to_host_bytes{};
  std::uint64_t traversal_device_capacity_bytes{};
  std::uint64_t retained_node_bound_view_capacity_bytes{};
  std::uint64_t node_bound_view_allocation_capacity_bytes{};
  std::uint64_t resolved_node_bound_count{};
  std::uint64_t node_bound_view_build_allocation_count{};
  std::uint64_t node_bound_view_build_kernel_launch_count{};
  std::uint64_t node_bound_view_build_synchronization_count{};
  std::uint64_t node_bound_view_validation_device_to_host_byte_count{};
  std::uint64_t peak_tile_output_device_capacity_bytes{};
  std::uint64_t device_total_bytes{};
  std::uint64_t minimum_device_free_bytes{};
  std::size_t validated_candidate_record_count{};
  std::size_t validated_prune_region_record_count{};
  std::size_t skipped_record_validation_count{};
  bool record_validation_sampled{false};
  bool every_record_validated{false};
  bool bounded_bruteforce_performed{false};
  bool every_prune_fully_recertified{false};
  bool coverage_partition_complete{false};
  bool q3_exact_diametral_pair_support_gabriel_negative_only{false};
  bool gamma2_silent_handoff_required{false};
  bool gamma2_prune_or_discard_authorized{false};
  bool component_contract_validated{false};
  bool node_bound_view_extent_authenticated{false};
  bool node_bound_view_validation_sentinel_authenticated{false};
  bool node_bound_view_bound_to_snapshot_identity{false};
  bool node_bound_view_built_once_per_adoption{false};
  bool node_bound_view_reused_without_tile_allocation{false};
  bool source_node_extremum_point_ids_retained{false};
  bool node_bound_view_build_included_in_context_creation{false};
  bool node_bound_view_build_included_in_traversal_adoption_ns{false};
};

struct ScalingSmokeMetrics {
  std::size_t maximum_closed_rank{};
  std::size_t advance_count{};
  std::size_t chunk_ready_advance_count{};
  std::size_t resumed_advance_count{};
  std::size_t candidate_yield_count{};
  std::size_t prune_yield_count{};
  std::size_t mixed_yield_count{};
  std::size_t candidate_tile_lease_count{};
  std::size_t candidate_tile_release_count{};
  std::size_t completed_anchor_count{};
  std::size_t certified_prune_region_count{};
  std::size_t launcher_call_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  std::size_t traversal_subdivision_count{};
  std::size_t maximum_traversal_subdivision_count_per_anchor{};
  std::size_t resume_control_device_to_host_count{};
  std::size_t resume_control_device_to_host_byte_count{};
  std::size_t anchor_control_device_to_host_count{};
  std::size_t anchor_control_device_to_host_byte_count{};
  std::size_t retained_node_bound_view_capacity_bytes{};
  std::size_t node_bound_view_allocation_capacity_bytes{};
  std::size_t resolved_node_bound_count{};
  std::size_t node_bound_view_build_allocation_count{};
  std::size_t node_bound_view_build_kernel_launch_count{};
  std::size_t node_bound_view_build_synchronization_count{};
  std::size_t node_bound_view_validation_device_to_host_byte_count{};
  std::uint64_t candidate_pair_mass{};
  std::uint64_t certified_pruned_pair_mass{};
  std::uint64_t unresolved_pair_mass{};
  std::uint64_t unordered_pair_universe_count{};
  std::uint64_t physical_node_visit_count{};
  std::uint64_t traversal_device_capacity_bytes{};
  std::uint64_t peak_candidate_device_capacity_bytes{};
  std::uint64_t peak_control_device_capacity_bytes{};
  std::uint64_t peak_prune_device_capacity_bytes{};
  std::uint64_t peak_tile_output_device_capacity_bytes{};
  std::uint64_t peak_witness_bank_device_capacity_bytes{};
  std::uint64_t device_total_bytes{};
  std::uint64_t initial_device_free_bytes{};
  std::uint64_t minimum_device_free_bytes{};
  std::uint64_t final_device_free_bytes{};
  std::size_t device_memory_observation_count{};
  std::uint64_t generation_ns{};
  std::uint64_t canonicalization_ns{};
  std::uint64_t build_ns{};
  std::uint64_t lease_release_ns{};
  std::uint64_t host_release_ns{};
  std::uint64_t context_creation_ns{};
  std::uint64_t context_release_ns{};
  std::uint64_t frontier_ns{};
  std::uint64_t total_ns{};
  MortonYao48DeviceTiledPairFrontierStopReason stop_reason{
      MortonYao48DeviceTiledPairFrontierStopReason::none};
  bool censored{false};
  bool coverage_complete{false};
  bool backpressure_validated{false};
  bool execution_success{false};
};

class QualificationMismatch final : public std::runtime_error {
 public:
  explicit QualificationMismatch(const std::string& message)
      : std::runtime_error(message) {}
};

[[nodiscard]] constexpr std::string_view prune_semantics_name(
    MortonYao48DeviceTiledPairFrontierPruneSemantics semantics) noexcept {
  switch (semantics) {
    case MortonYao48DeviceTiledPairFrontierPruneSemantics::
        closed_rank_window:
      return "closed_rank_window";
    case MortonYao48DeviceTiledPairFrontierPruneSemantics::
        strict_interior_threshold:
      return "strict_interior_threshold";
  }
  return "invalid";
}

[[nodiscard]] MortonYao48DeviceTiledPairFrontierPruneSemantics
parse_prune_semantics(std::string_view text) {
  if (text == "closed_rank_window") {
    return MortonYao48DeviceTiledPairFrontierPruneSemantics::
        closed_rank_window;
  }
  if (text == "strict_interior_threshold") {
    return MortonYao48DeviceTiledPairFrontierPruneSemantics::
        strict_interior_threshold;
  }
  throw std::invalid_argument(
      "--prune-semantics must be closed_rank_window or "
      "strict_interior_threshold");
}

[[noreturn]] void mismatch(const std::string& message) {
  throw QualificationMismatch(message);
}

void require(bool condition, const std::string& message) {
  if (!condition) {
    mismatch(message);
  }
}

[[nodiscard]] std::size_t parse_size(
    std::string_view text,
    const char* role) {
  std::size_t value = 0U;
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end) {
    throw std::invalid_argument(role);
  }
  return value;
}

[[nodiscard]] std::uint64_t parse_u64(
    std::string_view text,
    const char* role) {
  std::uint64_t value = 0U;
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto result = std::from_chars(begin, end, value);
  if (result.ec != std::errc{} || result.ptr != end) {
    throw std::invalid_argument(role);
  }
  return value;
}

[[nodiscard]] Options parse_options(int argc, char** argv) {
  Options options;
  for (int index = 1; index < argc; ++index) {
    const std::string_view argument{argv[index]};
    if (argument == "--point-count" && index + 1 < argc) {
      options.point_count =
          parse_size(argv[++index], "invalid --point-count");
      options.point_count_explicit = true;
    } else if (argument == "--family" && index + 1 < argc) {
      options.family = argv[++index];
      options.family_explicit = true;
    } else if (argument == "--fixture" && index + 1 < argc) {
      options.fixture = argv[++index];
      options.fixture_explicit = true;
    } else if (
        argument == "--prune-semantics" && index + 1 < argc) {
      options.prune_semantics = parse_prune_semantics(argv[++index]);
    } else if (
        argument == "--scientific-scope" && index + 1 < argc) {
      options.scientific_scope = argv[++index];
      options.scientific_scope_explicit = true;
    } else if (
        argument == "--maximum-closed-rank" && index + 1 < argc) {
      options.maximum_closed_rank =
          parse_size(argv[++index], "invalid --maximum-closed-rank");
    } else if (argument == "--all-ranks") {
      options.all_ranks = true;
    } else if (argument == "--scaling-smoke") {
      options.scaling_smoke = true;
    } else if (argument == "--direct-scale") {
      options.direct_scale = true;
    } else if (argument == "--anchor-tile-capacity" && index + 1 < argc) {
      options.anchor_tile_capacity =
          parse_size(argv[++index], "invalid --anchor-tile-capacity");
      options.anchor_tile_capacity_explicit = true;
    } else if (argument == "--sampled-prune-limit" && index + 1 < argc) {
      options.sampled_prune_limit =
          parse_size(argv[++index], "invalid --sampled-prune-limit");
    } else if (
        argument == "--exact-prune-target-check-limit" &&
        index + 1 < argc) {
      options.exact_prune_target_check_limit = parse_size(
          argv[++index], "invalid --exact-prune-target-check-limit");
    } else if (
        argument == "--record-validation-sample-limit" && index + 1 < argc) {
      options.record_validation_sample_limit = parse_size(
          argv[++index], "invalid --record-validation-sample-limit");
    } else if (argument == "--warmup-run-count" && index + 1 < argc) {
      options.warmup_run_count =
          parse_size(argv[++index], "invalid --warmup-run-count");
    } else if (argument == "--seed" && index + 1 < argc) {
      options.seed = parse_u64(argv[++index], "invalid --seed");
    } else {
      throw std::invalid_argument(
          "usage: gpu_morton_yao48_device_tiled_pair_frontier_qualification "
          "[--point-count N] [--family adversarial_mixed_dyadic|"
          "shell_permutation_dyadic|jitter_grid_dyadic|clusters_dyadic|"
          "affine_uniform_binary64] "
          "[--prune-semantics closed_rank_window|"
          "strict_interior_threshold] "
          "[--scientific-scope "
          "q3_gabriel_exact_diametral_pair_support_negative_only] "
          "[--fixture q3_shell_strict_discriminant] "
          "[--maximum-closed-rank R|--all-ranks] "
          "[--scaling-smoke|--direct-scale] "
          "[--anchor-tile-capacity N] [--sampled-prune-limit N] "
          "[--exact-prune-target-check-limit N] "
          "[--record-validation-sample-limit N] "
          "[--warmup-run-count 0|1] [--seed N]");
    }
  }
  if (options.scaling_smoke && options.direct_scale) {
    throw std::invalid_argument(
        "--scaling-smoke and --direct-scale are mutually exclusive");
  }
  const bool strict_interior_threshold =
      options.prune_semantics ==
      MortonYao48DeviceTiledPairFrontierPruneSemantics::
          strict_interior_threshold;
  if (strict_interior_threshold) {
    if (!options.fixture_explicit ||
        options.fixture != kStrictInteriorThresholdFixture) {
      throw std::invalid_argument(
          "strict_interior_threshold requires "
          "--fixture q3_shell_strict_discriminant");
    }
    if (!options.scientific_scope_explicit ||
        options.scientific_scope !=
            kStrictInteriorThresholdScientificScope) {
      throw std::invalid_argument(
          "strict_interior_threshold requires --scientific-scope "
          "q3_gabriel_exact_diametral_pair_support_negative_only");
    }
    if (options.scaling_smoke || options.direct_scale ||
        options.point_count_explicit || options.family_explicit ||
        options.maximum_closed_rank.has_value() || options.all_ranks ||
        options.anchor_tile_capacity_explicit) {
      throw std::invalid_argument(
          "the strict_interior_threshold q=3 fixture owns its cloud, rank "
          "matrix, and tile-capacity matrix");
    }
    options.point_count = 4U;
    options.family = std::string{kStrictInteriorThresholdFixture};
  } else if (
      options.fixture_explicit || options.scientific_scope_explicit) {
    throw std::invalid_argument(
        "--fixture and --scientific-scope are only valid with "
        "strict_interior_threshold");
  }
  if (options.scaling_smoke && !options.point_count_explicit) {
    options.point_count = 1'000'000U;
  }
  if (options.direct_scale && !options.point_count_explicit) {
    throw std::invalid_argument(
        "--direct-scale requires --point-count 10000000 or 30000000");
  }
  const bool large_scale = options.scaling_smoke || options.direct_scale;
  const std::size_t maximum_point_count =
      large_scale ? 30'000'000U : 50'000U;
  if (options.point_count < 2U ||
      options.point_count > maximum_point_count) {
    throw std::length_error(
        large_scale
            ? "scaling --point-count must lie in [2,30000000]"
            : "qualification --point-count must lie in [2,50000]");
  }
  if (options.direct_scale && options.point_count != 10'000'000U &&
      options.point_count != 30'000'000U) {
    throw std::invalid_argument(
        "--direct-scale only accepts --point-count 10000000 or 30000000");
  }
  if (options.maximum_closed_rank.has_value() && options.all_ranks) {
    throw std::invalid_argument(
        "--maximum-closed-rank and --all-ranks are mutually exclusive");
  }
  if (large_scale && options.all_ranks) {
    throw std::invalid_argument(
        "large-scale modes accept one --maximum-closed-rank only");
  }
  if (options.maximum_closed_rank.has_value() &&
      (*options.maximum_closed_rank < 2U ||
       *options.maximum_closed_rank >
           morsehgp3d::gpu::
               morton_yao48_device_tiled_pair_frontier_maximum_closed_rank)) {
    throw std::invalid_argument("--maximum-closed-rank must lie in [2,11]");
  }
  if (options.anchor_tile_capacity == 0U ||
      options.anchor_tile_capacity >
          morsehgp3d::gpu::
              morton_yao48_device_tiled_pair_frontier_maximum_anchor_tile_capacity) {
    throw std::invalid_argument("--anchor-tile-capacity must lie in [1,4096]");
  }
  if (options.sampled_prune_limit == 0U ||
      options.exact_prune_target_check_limit == 0U) {
    throw std::invalid_argument(
        "qualification sampling limits must be strictly positive");
  }
  if (options.warmup_run_count > 1U) {
    throw std::invalid_argument("--warmup-run-count must be zero or one");
  }
  if (options.warmup_run_count != 0U &&
      (large_scale || strict_interior_threshold || options.all_ranks ||
       !options.maximum_closed_rank.has_value() ||
       *options.maximum_closed_rank != 6U)) {
    throw std::invalid_argument(
        "--warmup-run-count 1 requires the explicit standard "
        "--maximum-closed-rank 6");
  }
  const bool known_family =
      options.family == "adversarial_mixed_dyadic" ||
      options.family == "shell_permutation_dyadic" ||
      options.family == "jitter_grid_dyadic" ||
      options.family == "clusters_dyadic";
  if (!large_scale && !strict_interior_threshold && !known_family) {
    throw std::invalid_argument("unknown --family");
  }
  if (large_scale) {
    if (options.family_explicit &&
        options.family != "affine_uniform_binary64") {
      throw std::invalid_argument(
          "large-scale modes only accept --family affine_uniform_binary64");
    }
    options.family = "affine_uniform_binary64";
  }
  return options;
}

[[nodiscard]] std::vector<std::size_t> requested_ranks(
    const Options& options) {
  if (options.maximum_closed_rank.has_value()) {
    return {*options.maximum_closed_rank};
  }
  if (options.all_ranks) {
    return {2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U};
  }
  if (options.point_count <= 257U) {
    return {2U, 3U, 11U};
  }
  return {11U};
}

[[nodiscard]] std::uint64_t splitmix64(std::uint64_t value) noexcept {
  value += UINT64_C(0x9e3779b97f4a7c15);
  value =
      (value ^ (value >> 30U)) * UINT64_C(0xbf58476d1ce4e5b9);
  value =
      (value ^ (value >> 27U)) * UINT64_C(0x94d049bb133111eb);
  return value ^ (value >> 31U);
}

[[nodiscard]] std::size_t scaling_permutation_multiplier(
    std::size_t point_count) {
  std::size_t multiplier = static_cast<std::size_t>(
      UINT64_C(2654435761) % static_cast<std::uint64_t>(point_count));
  multiplier = std::max(multiplier, std::size_t{1U});
  while (std::gcd(multiplier, point_count) != 1U) {
    ++multiplier;
  }
  return multiplier;
}

[[nodiscard]] std::vector<CertifiedPoint3> generate_scaling_points(
    const Options& options) {
  constexpr std::uint64_t mantissa_mask =
      (UINT64_C(1) << 52U) - UINT64_C(1);
  constexpr std::uint64_t one_exponent = UINT64_C(0x3ff0000000000000);
  const std::uint64_t point_count = static_cast<std::uint64_t>(
      options.point_count);
  const std::uint64_t x_step = std::max(
      UINT64_C(1), mantissa_mask / point_count);
  const std::size_t multiplier =
      scaling_permutation_multiplier(options.point_count);
  const std::size_t offset = static_cast<std::size_t>(
      options.seed % point_count);

  std::vector<CertifiedPoint3> points;
  points.reserve(options.point_count);
  for (std::size_t source_index = 0U;
       source_index < options.point_count;
       ++source_index) {
    const std::size_t logical_index = static_cast<std::size_t>(
        (static_cast<std::uint64_t>(source_index) *
             static_cast<std::uint64_t>(multiplier) +
         static_cast<std::uint64_t>(offset)) %
        point_count);
    const std::uint64_t identity =
        static_cast<std::uint64_t>(logical_index);
    const std::uint64_t x_mantissa = identity * x_step;
    const std::uint64_t y_mantissa =
        splitmix64(identity ^ options.seed) & mantissa_mask;
    const std::uint64_t z_mantissa =
        splitmix64(identity ^ std::rotl(options.seed, 23)) & mantissa_mask;
    points.push_back(CertifiedPoint3::from_binary64_bits(
        {one_exponent | x_mantissa,
         one_exponent | y_mantissa,
         one_exponent | z_mantissa}));
  }
  return points;
}

void append_scaled_point(
    GeneratedCloud& generated,
    std::set<std::array<std::int64_t, 3U>>& used,
    std::array<std::int64_t, 3U> coordinate,
    std::size_t& feature_counter,
    std::size_t point_count) {
  if (generated.points.size() >= point_count || !used.insert(coordinate).second) {
    return;
  }
  for (const std::int64_t value : coordinate) {
    if (value < -kMaximumAbsoluteScaledCoordinate ||
        value > kMaximumAbsoluteScaledCoordinate) {
      throw std::overflow_error(
          "a qualification coordinate escaped its exact int64 envelope");
    }
  }
  const auto decode = [](std::int64_t value) {
    return std::ldexp(static_cast<double>(value), -20);
  };
  const double x = decode(coordinate[0U]);
  const double y = decode(coordinate[1U]);
  const double z = decode(coordinate[2U]);
  if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) ||
      std::ldexp(x, 20) != static_cast<double>(coordinate[0U]) ||
      std::ldexp(y, 20) != static_cast<double>(coordinate[1U]) ||
      std::ldexp(z, 20) != static_cast<double>(coordinate[2U])) {
    throw std::logic_error(
        "the qualification generator lost an exact binary64 dyadic");
  }
  generated.points.push_back(CertifiedPoint3::from_binary64(x, y, z));
  generated.scaled_points.push_back(ScaledPoint{coordinate});
  ++feature_counter;
}

void append_shell_and_permutations(
    GeneratedCloud& generated,
    std::set<std::array<std::int64_t, 3U>>& used,
    std::size_t point_count) {
  constexpr std::int64_t unit = kCoordinateDenominator;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    for (const std::int64_t sign : {INT64_C(-1), INT64_C(1)}) {
      std::array<std::int64_t, 3U> coordinate{};
      coordinate[axis] = sign * INT64_C(5) * unit;
      append_scaled_point(
          generated,
          used,
          coordinate,
          generated.features.equality_shell_point_count,
          point_count);
    }
  }
  append_scaled_point(
      generated,
      used,
      {0, 0, 0},
      generated.features.equality_shell_point_count,
      point_count);
  constexpr std::array<std::array<std::uint8_t, 3U>, 6U> permutations{{
      {{0U, 1U, 2U}},
      {{0U, 2U, 1U}},
      {{1U, 0U, 2U}},
      {{1U, 2U, 0U}},
      {{2U, 0U, 1U}},
      {{2U, 1U, 0U}},
  }};
  for (const auto& permutation : permutations) {
    for (std::uint8_t sign_mask = 0U; sign_mask < 8U; ++sign_mask) {
      constexpr std::array<std::int64_t, 3U> shell_source{
          0, INT64_C(3) * unit, INT64_C(4) * unit};
      std::array<std::int64_t, 3U> shell{};
      for (std::size_t position = 0U; position < 3U; ++position) {
        const std::size_t axis = permutation[position];
        const std::int64_t sign =
            (sign_mask & static_cast<std::uint8_t>(1U << axis)) == 0U
                ? INT64_C(1)
                : INT64_C(-1);
        shell[axis] = sign * shell_source[position];
      }
      append_scaled_point(
          generated,
          used,
          shell,
          generated.features.equality_shell_point_count,
          point_count);

      constexpr std::array<std::int64_t, 3U> direction_source{
          INT64_C(7) * unit,
          INT64_C(3) * unit,
          INT64_C(1) * unit};
      std::array<std::int64_t, 3U> direction{};
      for (std::size_t position = 0U; position < 3U; ++position) {
        const std::size_t axis = permutation[position];
        const std::int64_t sign =
            (sign_mask & static_cast<std::uint8_t>(1U << axis)) == 0U
                ? INT64_C(1)
                : INT64_C(-1);
        direction[axis] = sign * direction_source[position];
      }
      append_scaled_point(
          generated,
          used,
          direction,
          generated.features.permutation_sign_point_count,
          point_count);
    }
  }
}

void append_jitter_grid(
    GeneratedCloud& generated,
    std::set<std::array<std::int64_t, 3U>>& used,
    std::size_t point_count,
    std::uint64_t seed) {
  constexpr std::int64_t unit = kCoordinateDenominator;
  std::size_t logical_index = 0U;
  for (std::int64_t ix = -2; ix <= 2; ++ix) {
    for (std::int64_t iy = -2; iy <= 2; ++iy) {
      for (std::int64_t iz = -2; iz <= 2; ++iz) {
        const std::uint64_t random = splitmix64(
            seed ^ static_cast<std::uint64_t>(logical_index));
        const std::int64_t jitter_x =
            static_cast<std::int64_t>(random & UINT64_C(0x1ff)) - 256;
        const std::int64_t jitter_y =
            static_cast<std::int64_t>((random >> 9U) & UINT64_C(0x1ff)) -
            256;
        const std::int64_t jitter_z =
            static_cast<std::int64_t>((random >> 18U) & UINT64_C(0x1ff)) -
            256;
        append_scaled_point(
            generated,
            used,
            {(INT64_C(12) * unit) + ix * (unit / 2) + jitter_x,
             (-INT64_C(9) * unit) + iy * (unit / 2) + jitter_y,
             (INT64_C(7) * unit) + iz * (unit / 2) + jitter_z},
            generated.features.jitter_grid_point_count,
            point_count);
        ++logical_index;
      }
    }
  }
}

void append_clusters(
    GeneratedCloud& generated,
    std::set<std::array<std::int64_t, 3U>>& used,
    std::size_t point_count,
    std::uint64_t seed) {
  constexpr std::int64_t unit = kCoordinateDenominator;
  constexpr std::array<std::array<std::int64_t, 3U>, 4U> centers{{
      {{-INT64_C(24) * unit, -INT64_C(18) * unit, INT64_C(20) * unit}},
      {{INT64_C(22) * unit, -INT64_C(21) * unit, -INT64_C(17) * unit}},
      {{-INT64_C(19) * unit, INT64_C(23) * unit, -INT64_C(22) * unit}},
      {{INT64_C(25) * unit, INT64_C(19) * unit, INT64_C(18) * unit}},
  }};
  for (std::size_t cluster = 0U; cluster < centers.size(); ++cluster) {
    for (std::size_t member = 0U; member < 32U; ++member) {
      const std::uint64_t random = splitmix64(
          seed ^ (static_cast<std::uint64_t>(cluster) << 32U) ^
          static_cast<std::uint64_t>(member));
      const auto perturb = [random](unsigned int shift) {
        return static_cast<std::int64_t>(
                   (random >> shift) & UINT64_C(0xffff)) -
               INT64_C(32768);
      };
      append_scaled_point(
          generated,
          used,
          {centers[cluster][0U] + perturb(0U),
           centers[cluster][1U] + perturb(16U),
           centers[cluster][2U] + perturb(32U)},
          generated.features.clustered_point_count,
          point_count);
    }
  }
}

void append_deterministic_fill(
    GeneratedCloud& generated,
    std::set<std::array<std::int64_t, 3U>>& used,
    std::size_t point_count,
    std::uint64_t seed) {
  constexpr std::int64_t unit = kCoordinateDenominator;
  std::uint64_t logical_index = 0U;
  while (generated.points.size() < point_count) {
    const std::uint64_t random_x = splitmix64(seed ^ logical_index);
    const std::uint64_t random_y =
        splitmix64(std::rotl(seed, 19) ^ logical_index);
    const std::uint64_t random_z =
        splitmix64(std::rotl(seed, 41) ^ logical_index);
    const std::int64_t x =
        INT64_C(36) * unit +
        static_cast<std::int64_t>(logical_index * UINT64_C(257)) +
        static_cast<std::int64_t>(random_x & UINT64_C(0xff));
    const std::int64_t y =
        static_cast<std::int64_t>(random_y %
                                  static_cast<std::uint64_t>(32 * unit)) -
        INT64_C(16) * unit;
    const std::int64_t z =
        static_cast<std::int64_t>(random_z %
                                  static_cast<std::uint64_t>(32 * unit)) -
        INT64_C(16) * unit;
    append_scaled_point(
        generated,
        used,
        {x, y, z},
        generated.features.deterministic_fill_point_count,
        point_count);
    ++logical_index;
  }
}

[[nodiscard]] GeneratedCloud generate_cloud(const Options& options) {
  GeneratedCloud generated;
  generated.points.reserve(options.point_count);
  generated.scaled_points.reserve(options.point_count);
  std::set<std::array<std::int64_t, 3U>> used;
  if (options.family == "adversarial_mixed_dyadic" ||
      options.family == "shell_permutation_dyadic") {
    append_shell_and_permutations(generated, used, options.point_count);
  }
  if (options.family == "adversarial_mixed_dyadic" ||
      options.family == "jitter_grid_dyadic") {
    append_jitter_grid(generated, used, options.point_count, options.seed);
  }
  if (options.family == "adversarial_mixed_dyadic" ||
      options.family == "clusters_dyadic") {
    append_clusters(
        generated,
        used,
        options.point_count,
        std::rotl(options.seed, 7));
  }
  append_deterministic_fill(
      generated, used, options.point_count, std::rotl(options.seed, 29));
  require(
      generated.points.size() == options.point_count &&
          generated.scaled_points.size() == options.point_count,
      "the deterministic qualification cloud has the wrong cardinality");
  if (options.family == "adversarial_mixed_dyadic" &&
      options.point_count >= 257U) {
    require(
        generated.features.equality_shell_point_count != 0U &&
            generated.features.permutation_sign_point_count != 0U &&
            generated.features.jitter_grid_point_count != 0U &&
            generated.features.clustered_point_count != 0U,
        "the mixed qualification cloud omitted an adversarial family");
  }
  return generated;
}

enum class StrictInteriorThresholdFixtureCloud : std::uint8_t {
  boundary_shell,
  strict_interior,
};

[[nodiscard]] constexpr std::string_view strict_fixture_cloud_name(
    StrictInteriorThresholdFixtureCloud cloud) noexcept {
  switch (cloud) {
    case StrictInteriorThresholdFixtureCloud::boundary_shell:
      return "boundary_shell";
    case StrictInteriorThresholdFixtureCloud::strict_interior:
      return "strict_interior";
  }
  return "invalid";
}

[[nodiscard]] GeneratedCloud generate_strict_interior_threshold_fixture(
    StrictInteriorThresholdFixtureCloud fixture_cloud) {
  constexpr std::int64_t unit = kCoordinateDenominator;
  GeneratedCloud generated;
  generated.points.reserve(4U);
  generated.scaled_points.reserve(4U);
  std::set<std::array<std::int64_t, 3U>> used;
  std::size_t fixture_point_count = 0U;
  const auto append = [&](std::array<std::int64_t, 3U> coordinate) {
    append_scaled_point(
        generated, used, coordinate, fixture_point_count, 4U);
  };

  // A and X are the minimum and maximum Morton leaves.  The two middle
  // points are either exactly on the diametral shell of AX or strictly
  // inside it.  All coordinates are exact binary64 dyadics.
  append({-INT64_C(5) * unit,
          -INT64_C(5) * unit,
          -INT64_C(5) * unit});
  if (fixture_cloud ==
      StrictInteriorThresholdFixtureCloud::boundary_shell) {
    append({INT64_C(5) * unit,
            INT64_C(5) * unit,
            -INT64_C(5) * unit});
    append({INT64_C(5) * unit,
            -INT64_C(5) * unit,
            INT64_C(5) * unit});
  } else {
    append({0, 0, 0});
    append({unit, unit, unit});
  }
  append({INT64_C(5) * unit,
          INT64_C(5) * unit,
          INT64_C(5) * unit});
  require(
      generated.points.size() == 4U &&
          generated.scaled_points.size() == 4U &&
          fixture_point_count == 4U,
      "the strict-interior discriminant fixture is incomplete");
  return generated;
}

template <typename Duration>
[[nodiscard]] std::uint64_t nanoseconds(Duration duration) {
  const auto value =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
  if (value < 0) {
    throw std::runtime_error(
        "the monotonic qualification clock moved backwards");
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    const char* message) {
  if (!std::in_range<std::uint64_t>(value)) {
    throw std::overflow_error(message);
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::uint64_t checked_u64_sum(
    std::uint64_t left,
    std::uint64_t right,
    const char* message) {
  if (right > std::numeric_limits<std::uint64_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_product(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    throw std::overflow_error(message);
  }
  return left * right;
}

[[nodiscard]] std::size_t checked_size_sum(
    std::size_t left,
    std::size_t right,
    const char* message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t traversal_subdivision_ceiling(
    std::size_t certified_node_count,
    std::size_t node_visit_capacity) {
  require(
      certified_node_count != 0U && node_visit_capacity != 0U,
      "a traversal subdivision ceiling requires positive extents");
  return certified_node_count / node_visit_capacity +
         (certified_node_count % node_visit_capacity == 0U ? 0U : 1U);
}

[[nodiscard]] std::uint64_t unordered_pair_count(std::size_t point_count) {
  const std::uint64_t count = checked_u64(
      point_count, "the qualification point count does not fit uint64");
  return count * (count - UINT64_C(1)) / UINT64_C(2);
}

[[nodiscard]] std::uint64_t triangular_offset(
    std::uint64_t anchor_position,
    std::uint64_t partner_position) {
  return anchor_position * (anchor_position - UINT64_C(1)) /
             UINT64_C(2) +
         partner_position;
}

[[nodiscard]] std::uint64_t hash_word(
    std::uint64_t digest,
    std::uint64_t word) noexcept {
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
  return digest;
}

[[nodiscard]] std::uint64_t scaled_cloud_digest(
    std::span<const ScaledPoint> points) noexcept {
  std::uint64_t digest = hash_word(
      kFnvOffsetBasis, static_cast<std::uint64_t>(points.size()));
  for (const ScaledPoint& point : points) {
    for (const std::int64_t coordinate : point.coordinate) {
      digest = hash_word(digest, std::bit_cast<std::uint64_t>(coordinate));
    }
  }
  return digest;
}

void hash_candidate(
    std::uint64_t& digest,
    const Phase15MortonYao48DeviceTiledCandidateRecord& record) noexcept {
  for (const std::uint64_t word : {
           record.support_u,
           record.support_v,
           record.anchor_morton_position,
           record.partner_morton_position,
           record.owner_cone_index,
           record.flags}) {
    digest = hash_word(digest, word);
  }
}

void hash_prune(
    std::uint64_t& digest,
    const Phase15MortonYao48DeviceTiledPruneRegionRecord& record) noexcept {
  for (const std::uint64_t word : {
           record.anchor_morton_position,
           record.node_index,
           record.certified_pair_mass,
           record.retained_witness_count,
           record.retained_witness_bank_mask,
           record.flags}) {
    digest = hash_word(digest, word);
  }
  for (const std::uint64_t witness : record.witness_point_ids) {
    digest = hash_word(digest, witness);
  }
}

void hash_bank_slot(
    std::uint64_t& digest,
    std::uint64_t anchor,
    std::uint64_t cone,
    const Phase15MortonYao48DeviceTiledWitnessBankSlot& slot) noexcept {
  digest = hash_word(digest, anchor);
  digest = hash_word(digest, cone);
  digest = hash_word(digest, slot.witness_point_id);
  digest = hash_word(digest, slot.witness_morton_position);
  digest = hash_word(digest, slot.squared_distance_lower_bits);
  digest = hash_word(digest, slot.squared_distance_upper_bits);
}

void hash_anchor_control(
    std::uint64_t& digest,
    const Phase15MortonYao48DeviceTiledAnchorControl& control) noexcept {
  for (const std::uint64_t word : {
           control.anchor_morton_position,
           control.candidate_count,
           control.prune_region_count,
           control.certified_pruned_pair_mass,
           control.node_visit_count,
           control.cumulative_candidate_count,
           control.cumulative_prune_region_count,
           control.cumulative_certified_pruned_pair_mass,
           control.cumulative_node_visit_count,
           control.status,
           control.yield_reason,
           control.stop_reason,
           control.failure_code,
           control.ambiguous_cone_candidate_count,
           control.unbanked_candidate_count,
           control.cumulative_ambiguous_cone_candidate_count,
           control.cumulative_unbanked_candidate_count,
           control.unresolved_pair_mass,
           control.tile_epoch,
           control.chunk_sequence,
           control.reserved_zero}) {
    digest = hash_word(digest, word);
  }
}

[[nodiscard]] int exact_phi_sign(
    const ScaledPoint& witness,
    const ScaledPoint& first,
    const ScaledPoint& second) {
  std::int64_t sum = 0;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const std::int64_t left =
        witness.coordinate[axis] - first.coordinate[axis];
    const std::int64_t right =
        witness.coordinate[axis] - second.coordinate[axis];
    const std::int64_t product = left * right;
    if ((product > 0 &&
         sum > std::numeric_limits<std::int64_t>::max() - product) ||
        (product < 0 &&
         sum < std::numeric_limits<std::int64_t>::min() - product)) {
      throw std::overflow_error(
          "the exact dyadic qualification phi accumulator overflowed");
    }
    sum += product;
  }
  return sum < 0 ? -1 : (sum == 0 ? 0 : 1);
}

[[nodiscard]] std::int64_t exact_squared_distance_numerator(
    const ScaledPoint& first,
    const ScaledPoint& second) {
  std::int64_t sum = 0;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const std::int64_t delta =
        first.coordinate[axis] - second.coordinate[axis];
    const std::int64_t square = delta * delta;
    if (sum > std::numeric_limits<std::int64_t>::max() - square) {
      throw std::overflow_error(
          "the exact dyadic qualification distance accumulator overflowed");
    }
    sum += square;
  }
  return sum;
}

[[nodiscard]] std::size_t exact_cone_index(
    const ScaledPoint& source,
    const ScaledPoint& target) {
  std::array<std::uint64_t, 3U> absolute_delta{};
  std::array<std::uint8_t, 3U> axes{{0U, 1U, 2U}};
  std::uint8_t negative_mask = 0U;
  bool nonzero = false;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const std::int64_t delta =
        target.coordinate[axis] - source.coordinate[axis];
    nonzero = nonzero || delta != 0;
    if (delta < 0) {
      negative_mask = static_cast<std::uint8_t>(
          negative_mask |
          static_cast<std::uint8_t>(std::uint8_t{1U} << axis));
    }
    absolute_delta[axis] = static_cast<std::uint64_t>(
        delta < 0 ? -delta : delta);
  }
  require(nonzero, "an exact cone qualification received duplicate points");
  std::sort(
      axes.begin(),
      axes.end(),
      [&absolute_delta](std::uint8_t left, std::uint8_t right) {
        const std::size_t left_axis = left;
        const std::size_t right_axis = right;
        if (absolute_delta[left_axis] != absolute_delta[right_axis]) {
          return absolute_delta[left_axis] > absolute_delta[right_axis];
        }
        return left < right;
      });
  constexpr std::array<std::array<std::uint8_t, 3U>, 6U> permutations{{
      {{0U, 1U, 2U}},
      {{0U, 2U, 1U}},
      {{1U, 0U, 2U}},
      {{1U, 2U, 0U}},
      {{2U, 0U, 1U}},
      {{2U, 1U, 0U}},
  }};
  const auto found = std::find(permutations.begin(), permutations.end(), axes);
  require(found != permutations.end(), "the exact cone order is invalid");
  const std::size_t order = static_cast<std::size_t>(
      std::distance(permutations.begin(), found));
  return static_cast<std::size_t>(negative_mask) * 6U + order;
}

[[nodiscard]] bool invalid_bank_slot(
    const Phase15MortonYao48DeviceTiledWitnessBankSlot& slot) noexcept {
  bool cached_fields_invalid = true;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    cached_fields_invalid =
        cached_fields_invalid &&
        slot.witness_coordinate_bits[axis] == kInvalid &&
        slot.direction_lower_bits[axis] == kInvalid &&
        slot.direction_upper_bits[axis] == kInvalid;
  }
  return cached_fields_invalid && slot.witness_point_id == kInvalid &&
         slot.witness_morton_position == kInvalid &&
         slot.squared_distance_lower_bits == kInvalid &&
         slot.squared_distance_upper_bits == kInvalid;
}

class CudaDeviceGuard final {
 public:
  explicit CudaDeviceGuard(int target_device) {
    check(cudaGetDevice(&original_device_), "cudaGetDevice");
    if (original_device_ != target_device) {
      check(cudaSetDevice(target_device), "cudaSetDevice qualification target");
      changed_ = true;
    }
  }

  ~CudaDeviceGuard() noexcept {
    if (changed_) {
      static_cast<void>(cudaSetDevice(original_device_));
    }
  }

  CudaDeviceGuard(const CudaDeviceGuard&) = delete;
  CudaDeviceGuard& operator=(const CudaDeviceGuard&) = delete;

  static void check(cudaError_t status, const char* operation) {
    if (status != cudaSuccess) {
      const char* description = cudaGetErrorString(status);
      throw std::runtime_error(
          std::string{operation} + " failed: " +
          (description == nullptr ? "unknown CUDA error" : description));
    }
  }

 private:
  int original_device_{-1};
  bool changed_{false};
};

template <typename Record>
[[nodiscard]] std::vector<Record> copy_device_records(
    const Record* device_records,
    std::size_t count,
    RankMetrics& metrics) {
  require(device_records != nullptr, "a qualification device view is null");
  std::vector<Record> host_records(count);
  const std::size_t bytes = checked_product(
      count, sizeof(Record), "a qualification D2H copy overflows size_t");
  const auto start = Clock::now();
  CudaDeviceGuard::check(
      cudaMemcpy(
          host_records.data(),
          device_records,
          bytes,
          cudaMemcpyDeviceToHost),
      "cudaMemcpy qualification output D2H");
  const auto end = Clock::now();
  metrics.qualification_output_copy_ns += nanoseconds(end - start);
  metrics.qualification_device_to_host_bytes += checked_u64(
      bytes, "the qualification D2H byte count does not fit uint64");
  return host_records;
}

void validate_device_nodes(
    std::span<const MortonLbvhSnapshotNode> nodes,
    std::size_t point_count) {
  require(
      nodes.size() == 2U * point_count - 1U,
      "the copied traversal-node extent is invalid");
  for (std::size_t index = 0U; index < nodes.size(); ++index) {
    const MortonLbvhSnapshotNode& node = nodes[index];
    require(
        node.leaf_begin < node.leaf_end && node.leaf_end <= point_count,
        "a copied traversal node has an invalid leaf interval");
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      require(
          node.lower_point_ids[axis] < point_count &&
              node.upper_point_ids[axis] < point_count,
          "a copied traversal node has an invalid AABB witness");
    }
    const std::uint64_t width = node.leaf_end - node.leaf_begin;
    if (width == UINT64_C(1)) {
      require(
          node.is_leaf(), "a copied unit-width traversal node is not a leaf");
    } else {
      require(
          node.left_child < index && node.right_child < index &&
              node.right_child + UINT64_C(1) == index,
          "a copied traversal internal node violates strict postorder");
      const MortonLbvhSnapshotNode& left =
          nodes[static_cast<std::size_t>(node.left_child)];
      const MortonLbvhSnapshotNode& right =
          nodes[static_cast<std::size_t>(node.right_child)];
      require(
          left.leaf_begin == node.leaf_begin &&
              left.leaf_end == right.leaf_begin &&
              right.leaf_end == node.leaf_end,
          "a copied traversal internal node has inconsistent child ranges");
    }
  }
  const MortonLbvhSnapshotNode& root = nodes.back();
  require(
      root.leaf_begin == 0U && root.leaf_end == point_count,
      "the copied traversal root does not cover the cloud");
}

void validate_batch_contract(
    const Phase15MortonYao48DeviceTiledBatch& batch,
    const Phase15MortonYao48DeviceTiledRequest& request,
    int cuda_device) {
  const std::size_t expected_candidates = checked_product(
      request.anchor_count,
      request.candidate_capacity_per_anchor,
      "the expected candidate capacity overflows size_t");
  const std::size_t expected_prunes = checked_product(
      request.anchor_count,
      request.prune_region_capacity_per_anchor,
      "the expected prune capacity overflows size_t");
  const std::size_t expected_banks = checked_product(
      checked_product(
          request.anchor_count,
          request.witness_bank_count_per_anchor,
          "the expected bank count overflows size_t"),
      request.witness_slot_count_per_bank,
      "the expected bank-slot capacity overflows size_t");
  const std::size_t expected_cached_prune_witnesses = checked_product(
      request.anchor_count,
      morsehgp3d::gpu::
          morton_yao48_device_tiled_pair_frontier_cached_prune_witnesses_per_anchor,
      "the expected cached prune witness capacity overflows size_t");
  std::size_t expected_arena_bytes = checked_product(
      expected_candidates,
      sizeof(Phase15MortonYao48DeviceTiledCandidateRecord),
      "the expected candidate arena bytes overflow size_t");
  expected_arena_bytes = checked_size_sum(
      expected_arena_bytes,
      checked_product(
          expected_prunes,
          sizeof(Phase15MortonYao48DeviceTiledPruneRegionRecord),
          "the expected prune arena bytes overflow size_t"),
      "the expected arena bytes overflow size_t");
  expected_arena_bytes = checked_size_sum(
      expected_arena_bytes,
      checked_product(
          expected_banks,
          sizeof(Phase15MortonYao48DeviceTiledWitnessBankSlot),
          "the expected witness arena bytes overflow size_t"),
      "the expected arena bytes overflow size_t");
  expected_arena_bytes = checked_size_sum(
      expected_arena_bytes,
      checked_product(
          expected_cached_prune_witnesses,
          sizeof(Phase15MortonYao48DeviceTiledCachedPruneWitness),
          "the expected cached prune witness arena bytes overflow size_t"),
      "the expected arena bytes overflow size_t");
  expected_arena_bytes = checked_size_sum(
      expected_arena_bytes,
      checked_product(
          request.anchor_count,
          sizeof(Phase15MortonYao48DeviceTiledAnchorControl),
          "the expected control arena bytes overflow size_t"),
      "the expected arena bytes overflow size_t");
  expected_arena_bytes = checked_size_sum(
      expected_arena_bytes,
      checked_product(
          request.anchor_count,
          sizeof(Phase15MortonYao48DeviceTiledAnchorCheckpoint),
          "the expected checkpoint arena bytes overflow size_t"),
      "the expected arena bytes overflow size_t");
  expected_arena_bytes = checked_size_sum(
      expected_arena_bytes,
      sizeof(std::uint64_t),
      "the expected pending-anchor arena bytes overflow size_t");
  const std::size_t maximum_subdivision_count =
      traversal_subdivision_ceiling(
          request.certified_node_count,
          request.node_visit_capacity_per_anchor);
  require(
      request.tile_epoch != 0U && request.chunk_sequence != 0U &&
          (request.resume_same_tile
               ? request.chunk_sequence > 1U
               : request.chunk_sequence == 1U),
      "the qualification request has an invalid tile/chunk sequence");
  require(
      batch.retained_output_owner != nullptr &&
          batch.device_candidate_records != nullptr &&
          batch.device_prune_regions != nullptr &&
          batch.device_witness_bank_slots != nullptr &&
          batch.device_cached_prune_witnesses != nullptr &&
          batch.device_anchor_controls != nullptr &&
          batch.host_anchor_controls.size() == request.anchor_count &&
          batch.physical_candidate_capacity == expected_candidates &&
          batch.physical_prune_region_capacity == expected_prunes &&
          batch.physical_witness_bank_slot_capacity == expected_banks &&
          batch.physical_cached_prune_witness_capacity ==
              expected_cached_prune_witnesses &&
          batch.physical_anchor_control_capacity == request.anchor_count &&
          batch.physical_anchor_checkpoint_capacity == request.anchor_count &&
          batch.physical_pending_anchor_count_capacity == 1U &&
          batch.physical_device_arena_capacity_bytes == expected_arena_bytes,
      "the CUDA tile batch has inconsistent physical extents");
  require(
      batch.anchor_control_device_to_host_count == request.anchor_count &&
          batch.anchor_control_device_to_host_byte_count ==
              request.anchor_count *
                  sizeof(Phase15MortonYao48DeviceTiledAnchorControl) &&
          batch.candidate_device_to_host_count == 0U &&
          batch.certified_prune_device_to_host_count == 0U &&
          batch.traversal_subdivision_count != 0U &&
          batch.traversal_subdivision_count <= maximum_subdivision_count &&
          batch.maximum_traversal_subdivision_count_per_anchor ==
              maximum_subdivision_count &&
          batch.kernel_launch_count == batch.traversal_subdivision_count &&
          batch.synchronization_count ==
              batch.traversal_subdivision_count + 1U &&
          batch.resume_control_device_to_host_count ==
              batch.traversal_subdivision_count &&
          batch.resume_control_device_to_host_byte_count ==
              batch.traversal_subdivision_count * sizeof(std::uint64_t),
      "the production launcher performed a forbidden output D2H copy");
  require(
      batch.source_snapshot_epoch == request.source_snapshot_epoch &&
          batch.output_buffer_epoch == request.output_buffer_epoch &&
          batch.tile_epoch == request.tile_epoch &&
          batch.chunk_sequence == request.chunk_sequence &&
          batch.resume_same_tile == request.resume_same_tile &&
          !batch.process_restart_resumable &&
          batch.cuda_device == cuda_device &&
          batch.execution_kind ==
              Phase15MortonYao48DeviceTiledExecutionKind::cuda &&
          batch.prune_semantics == request.prune_semantics &&
          batch.required_witness_count == request.required_witness_count &&
          batch.metadata_digest ==
              morsehgp3d::gpu::detail::
                  phase15_morton_yao48_device_tiled_metadata_digest(batch),
      "the CUDA tile batch identity or digest is invalid");
  require(
      request.resume_same_tile
          ? !batch.fresh_tile_device_arena_allocated &&
                !batch.fresh_tile_device_arena_reused
          : batch.fresh_tile_device_arena_allocated !=
                batch.fresh_tile_device_arena_reused,
      "the CUDA tile batch has invalid fresh-arena lifecycle metadata");
  require(
      batch.fixed_anchor_segments_allocated &&
          batch.output_owner_detached_for_tile_lifetime &&
          batch.interval_cone_classification_requested &&
          batch.ambiguous_cone_to_unbanked_candidate_requested &&
          batch.target_tested_before_bank_insert_requested &&
          batch.retained_witnesses_outside_pruned_subtree_requested &&
          batch
              .witness_direction_intervals_cached_per_bank_slot_requested &&
          batch.active_witness_slot_mask_authenticated_requested &&
          batch
              .inactive_witness_slots_skipped_in_physical_order_requested &&
          batch.last_prune_witness_direction_cache_retained_requested &&
          (request.prune_semantics ==
                   MortonYao48DeviceTiledPairFrontierPruneSemantics::
                       closed_rank_window
               ? batch
                         .nonnegative_diametral_witness_interval_lower_bound_requested &&
                     !batch
                          .strictly_positive_diametral_witness_interval_lower_bound_requested
               : !batch
                          .nonnegative_diametral_witness_interval_lower_bound_requested &&
                     batch
                         .strictly_positive_diametral_witness_interval_lower_bound_requested) &&
          batch.censored_anchor_outputs_invalidated &&
          batch.cuda_execution_contract_satisfied,
      "the CUDA tile batch omitted a required fail-open contract flag");
  require(
      !batch.exact_diametral_rank_evaluated &&
          !batch.scientific_pair_catalog_published &&
          !batch.dense_pair_fallback_performed &&
          !batch.global_pair_matrix_materialized &&
          !batch.higher_order_structure_materialized,
      "the CUDA tile batch made a forbidden product or scientific claim");
}

struct QualificationControlSummary {
  std::uint64_t candidate_count{};
  std::uint64_t prune_region_count{};
  std::uint64_t certified_pruned_pair_mass{};
  bool all_complete{true};
  bool any_chunk_ready{false};
  bool any_fatal{false};
  bool candidate_yield{false};
  bool prune_yield{false};
};

[[nodiscard]] QualificationControlSummary validate_qualification_controls(
    const Phase15MortonYao48DeviceTiledBatch& batch,
    const Phase15MortonYao48DeviceTiledRequest& request,
    std::vector<QualificationAnchorProgress>& progress,
    RankMetrics& metrics) {
  require(
      progress.size() == request.anchor_count,
      "the qualification continuation progress has a foreign extent");
  QualificationControlSummary summary;
  const std::uint64_t launched_visit_capacity = checked_u64(
      checked_product(
          batch.traversal_subdivision_count,
          request.node_visit_capacity_per_anchor,
          "the qualification launched visit capacity overflows size_t"),
      "the qualification launched visit capacity does not fit uint64");
  for (std::size_t slot = 0U; slot < request.anchor_count; ++slot) {
    const Phase15MortonYao48DeviceTiledAnchorControl& control =
        batch.host_anchor_controls[slot];
    QualificationAnchorProgress& prior = progress[slot];
    const std::uint64_t expected_anchor = checked_u64(
        request.anchor_begin + slot,
        "the qualification anchor position does not fit uint64");
    const std::uint64_t expected_candidate_count = checked_u64_sum(
        prior.candidate_count,
        control.candidate_count,
        "the qualification cumulative candidate count overflowed");
    const std::uint64_t expected_prune_region_count = checked_u64_sum(
        prior.prune_region_count,
        control.prune_region_count,
        "the qualification cumulative prune count overflowed");
    const std::uint64_t expected_pruned_mass = checked_u64_sum(
        prior.certified_pruned_pair_mass,
        control.certified_pruned_pair_mass,
        "the qualification cumulative prune mass overflowed");
    const std::uint64_t expected_node_visit_count = checked_u64_sum(
        prior.node_visit_count,
        control.node_visit_count,
        "the qualification cumulative visit count overflowed");
    const std::uint64_t expected_ambiguous_count = checked_u64_sum(
        prior.ambiguous_cone_candidate_count,
        control.ambiguous_cone_candidate_count,
        "the qualification cumulative ambiguity count overflowed");
    const std::uint64_t expected_unbanked_count = checked_u64_sum(
        prior.unbanked_candidate_count,
        control.unbanked_candidate_count,
        "the qualification cumulative unbanked count overflowed");
    require(
        control.anchor_morton_position == expected_anchor &&
            control.tile_epoch == request.tile_epoch &&
            control.chunk_sequence == request.chunk_sequence &&
            control.reserved_zero == 0U &&
            control.candidate_count <=
                request.candidate_capacity_per_anchor &&
            control.prune_region_count <=
                request.prune_region_capacity_per_anchor &&
            control.node_visit_count <= launched_visit_capacity &&
            control.cumulative_candidate_count ==
                expected_candidate_count &&
            control.cumulative_prune_region_count ==
                expected_prune_region_count &&
            control.cumulative_certified_pruned_pair_mass ==
                expected_pruned_mass &&
            control.cumulative_node_visit_count ==
                expected_node_visit_count &&
            control.cumulative_node_visit_count <=
                request.certified_node_count &&
            control.cumulative_ambiguous_cone_candidate_count ==
                expected_ambiguous_count &&
            control.cumulative_unbanked_candidate_count ==
                expected_unbanked_count &&
            control.ambiguous_cone_candidate_count <=
                control.unbanked_candidate_count &&
            control.unbanked_candidate_count <= control.candidate_count &&
            control.cumulative_ambiguous_cone_candidate_count <=
                control.cumulative_unbanked_candidate_count &&
            control.cumulative_unbanked_candidate_count <=
                control.cumulative_candidate_count &&
            control.yield_reason <= static_cast<std::uint64_t>(
                                        Phase15MortonYao48DeviceTiledYieldReason::
                                            prune_segment_full) &&
            control.stop_reason <= static_cast<std::uint64_t>(
                                       MortonYao48DeviceTiledPairFrontierStopReason::
                                           fatal_failure) &&
            control.failure_code <= static_cast<std::uint64_t>(
                                         Phase15MortonYao48DeviceTiledFailureCode::
                                             internal_invariant),
        "an anchor control violates its v4 delta/cumulative identity");
    require(
        (control.prune_region_count == 0U) ==
                (control.certified_pruned_pair_mass == 0U) &&
            control.prune_region_count <=
                control.certified_pruned_pair_mass &&
            control.prune_region_count <= control.node_visit_count &&
            control.candidate_count <=
                control.node_visit_count - control.prune_region_count &&
            control.cumulative_candidate_count <= expected_anchor &&
            control.cumulative_certified_pruned_pair_mass <=
                expected_anchor - control.cumulative_candidate_count &&
            control.unresolved_pair_mass ==
                expected_anchor - control.cumulative_candidate_count -
                    control.cumulative_certified_pruned_pair_mass,
        "an anchor control violates its v4 pair partition");

    const auto status =
        static_cast<Phase15MortonYao48DeviceTiledAnchorStatus>(
            control.status);
    const auto yield_reason =
        static_cast<Phase15MortonYao48DeviceTiledYieldReason>(
            control.yield_reason);
    bool complete = false;
    switch (status) {
      case Phase15MortonYao48DeviceTiledAnchorStatus::active:
        throw QualificationMismatch(
            "the CUDA launcher returned an internal active anchor state");
      case Phase15MortonYao48DeviceTiledAnchorStatus::chunk_ready:
        require(
            !prior.complete &&
                control.stop_reason == static_cast<std::uint64_t>(
                                           MortonYao48DeviceTiledPairFrontierStopReason::
                                               none) &&
                control.failure_code == static_cast<std::uint64_t>(
                                            Phase15MortonYao48DeviceTiledFailureCode::
                                                none) &&
                control.unresolved_pair_mass != 0U &&
                ((yield_reason ==
                      Phase15MortonYao48DeviceTiledYieldReason::
                          candidate_segment_full &&
                  control.candidate_count ==
                      request.candidate_capacity_per_anchor) ||
                 (yield_reason ==
                      Phase15MortonYao48DeviceTiledYieldReason::
                          prune_segment_full &&
                  control.prune_region_count ==
                      request.prune_region_capacity_per_anchor)),
            "a chunk-ready anchor did not exhaust its reported segment");
        summary.any_chunk_ready = true;
        if (yield_reason ==
            Phase15MortonYao48DeviceTiledYieldReason::
                candidate_segment_full) {
          summary.candidate_yield = true;
          ++metrics.candidate_yield_anchor_count;
        } else {
          summary.prune_yield = true;
          ++metrics.prune_yield_anchor_count;
        }
        break;
      case Phase15MortonYao48DeviceTiledAnchorStatus::complete:
        require(
            yield_reason ==
                    Phase15MortonYao48DeviceTiledYieldReason::none &&
                control.stop_reason == static_cast<std::uint64_t>(
                                           MortonYao48DeviceTiledPairFrontierStopReason::
                                               none) &&
                control.failure_code == static_cast<std::uint64_t>(
                                            Phase15MortonYao48DeviceTiledFailureCode::
                                                none) &&
                control.unresolved_pair_mass == 0U &&
                (!prior.complete ||
                 (control.candidate_count == 0U &&
                  control.prune_region_count == 0U &&
                  control.certified_pruned_pair_mass == 0U &&
                  control.node_visit_count == 0U &&
                  control.ambiguous_cone_candidate_count == 0U &&
                  control.unbanked_candidate_count == 0U)),
            "a complete anchor reports unresolved or duplicated work");
        complete = true;
        if (!prior.complete) {
          ++metrics.complete_anchor_count;
        }
        break;
      case Phase15MortonYao48DeviceTiledAnchorStatus::fatal:
        require(
            !prior.complete &&
                yield_reason ==
                    Phase15MortonYao48DeviceTiledYieldReason::none &&
                control.stop_reason != static_cast<std::uint64_t>(
                                           MortonYao48DeviceTiledPairFrontierStopReason::
                                               none) &&
                control.candidate_count == 0U &&
                control.prune_region_count == 0U &&
                control.certified_pruned_pair_mass == 0U &&
                control.ambiguous_cone_candidate_count == 0U &&
                control.unbanked_candidate_count == 0U,
            "a fatal anchor attempted to publish a partial output chunk");
        summary.any_fatal = true;
        ++metrics.censored_anchor_count;
        break;
      default:
        throw QualificationMismatch(
            "an anchor control has an unknown v4 status");
    }

    hash_anchor_control(metrics.output_digest, control);
    metrics.physical_node_visit_count = checked_u64_sum(
        metrics.physical_node_visit_count,
        control.node_visit_count,
        "the qualification physical visit count overflowed");
    metrics.ambiguous_candidate_count += static_cast<std::size_t>(
        control.ambiguous_cone_candidate_count);
    metrics.unbanked_candidate_count += static_cast<std::size_t>(
        control.unbanked_candidate_count);
    summary.candidate_count = checked_u64_sum(
        summary.candidate_count,
        control.candidate_count,
        "the qualification chunk candidate count overflowed");
    summary.prune_region_count = checked_u64_sum(
        summary.prune_region_count,
        control.prune_region_count,
        "the qualification chunk prune count overflowed");
    summary.certified_pruned_pair_mass = checked_u64_sum(
        summary.certified_pruned_pair_mass,
        control.certified_pruned_pair_mass,
        "the qualification chunk prune mass overflowed");
    prior = QualificationAnchorProgress{
        control.cumulative_candidate_count,
        control.cumulative_prune_region_count,
        control.cumulative_certified_pruned_pair_mass,
        control.cumulative_node_visit_count,
        control.cumulative_ambiguous_cone_candidate_count,
        control.cumulative_unbanked_candidate_count,
        complete};
    summary.all_complete = summary.all_complete && complete;
  }
  require(
      batch.capacity_yield_resumable == summary.any_chunk_ready &&
          !batch.process_restart_resumable &&
          !(summary.any_chunk_ready && summary.any_fatal) &&
          (summary.all_complete || summary.any_chunk_ready ||
           summary.any_fatal),
      "the CUDA batch has an inconsistent v4 aggregate state");
  return summary;
}

void validate_candidate_record(
    const Phase15MortonYao48DeviceTiledCandidateRecord& record,
    std::uint64_t anchor_position,
    std::span<const MortonLeafRecord> leaves,
    std::span<const ScaledPoint> scaled_points,
    RankMetrics& metrics) {
  require(
      record.anchor_morton_position == anchor_position &&
          record.partner_morton_position < anchor_position,
      "a candidate record violates Morton pair ownership");
  const PointId anchor_id =
      leaves[static_cast<std::size_t>(anchor_position)].point_id;
  const PointId partner_id =
      leaves[static_cast<std::size_t>(record.partner_morton_position)].point_id;
  require(
      record.support_u == std::min(anchor_id, partner_id) &&
          record.support_v == std::max(anchor_id, partner_id) &&
          record.support_u < record.support_v,
      "a candidate record has inconsistent canonical supports");
  require(
      (record.flags & ~kCandidateKnownFlags) == 0U,
      "a candidate record contains an unknown flag");
  const bool ambiguous =
      (record.flags & kCandidateFlagAmbiguousCone) != 0U;
  const bool certified =
      (record.flags & kCandidateFlagCertifiedCone) != 0U;
  const bool inserted =
      (record.flags & kCandidateFlagBankInserted) != 0U;
  const bool replaced =
      (record.flags & kCandidateFlagBankReplaced) != 0U;
  require(
      ambiguous != certified && (!replaced || inserted),
      "a candidate record has inconsistent cone/bank flags");
  if (ambiguous) {
    require(
        record.owner_cone_index == UINT64_C(48) && !inserted && !replaced,
        "an ambiguous candidate was assigned to a witness bank");
  } else {
    const std::size_t exact_cone = exact_cone_index(
        scaled_points[static_cast<std::size_t>(anchor_id)],
        scaled_points[static_cast<std::size_t>(partner_id)]);
    require(
        record.owner_cone_index == exact_cone && exact_cone < 48U,
        "a certified device cone disagrees with exact dyadic replay");
  }
  ++metrics.candidate_record_count;
  metrics.candidate_pair_mass += UINT64_C(1);
  hash_candidate(metrics.output_digest, record);
}

[[nodiscard]] std::vector<std::uint64_t> sampled_target_positions(
    std::uint64_t begin,
    std::uint64_t end,
    std::size_t maximum_count) {
  const std::uint64_t width = end - begin;
  const std::size_t count = std::min(
      maximum_count,
      static_cast<std::size_t>(width));
  std::vector<std::uint64_t> positions;
  positions.reserve(count);
  for (std::size_t index = 0U; index < count; ++index) {
    const std::uint64_t numerator =
        static_cast<std::uint64_t>(index) * width;
    const std::uint64_t position =
        begin + numerator / static_cast<std::uint64_t>(count);
    if (positions.empty() || positions.back() != position) {
      positions.push_back(position);
    }
  }
  if (positions.empty() || positions.back() != end - UINT64_C(1)) {
    positions.push_back(end - UINT64_C(1));
  }
  return positions;
}

void recertify_prune_targets(
    const Phase15MortonYao48DeviceTiledPruneRegionRecord& record,
    const MortonLbvhSnapshotNode& node,
    std::span<const MortonLeafRecord> leaves,
    std::span<const ScaledPoint> scaled_points,
    std::span<const std::uint64_t> target_positions,
    MortonYao48DeviceTiledPairFrontierPruneSemantics prune_semantics,
    RankMetrics& metrics) {
  const PointId anchor_id =
      leaves[static_cast<std::size_t>(record.anchor_morton_position)].point_id;
  for (const std::uint64_t target_position : target_positions) {
    require(
        target_position >= node.leaf_begin &&
            target_position < node.leaf_end,
        "a sampled prune target lies outside its node");
    const PointId target_id =
        leaves[static_cast<std::size_t>(target_position)].point_id;
    for (std::size_t witness_index = 0U;
         witness_index < record.retained_witness_count;
         ++witness_index) {
      const PointId witness_id = record.witness_point_ids[witness_index];
      const int sign = exact_phi_sign(
          scaled_points[static_cast<std::size_t>(witness_id)],
          scaled_points[static_cast<std::size_t>(anchor_id)],
          scaled_points[static_cast<std::size_t>(target_id)]);
      require(
          prune_semantics ==
                  MortonYao48DeviceTiledPairFrontierPruneSemantics::
                      closed_rank_window
              ? sign <= 0
              : sign < 0,
          prune_semantics ==
                  MortonYao48DeviceTiledPairFrontierPruneSemantics::
                      closed_rank_window
              ? "a device prune witness fails exact closed-diametral replay"
              : "a device prune witness is not strictly interior");
      ++metrics.prune_witness_target_check_count;
    }
  }
}

void validate_prune_record(
    const Phase15MortonYao48DeviceTiledPruneRegionRecord& record,
    std::uint64_t anchor_position,
    std::size_t maximum_closed_rank,
    MortonYao48DeviceTiledPairFrontierPruneSemantics prune_semantics,
    std::size_t required_witness_count,
    std::span<const MortonLbvhSnapshotNode> nodes,
    std::span<const MortonLeafRecord> leaves,
    std::span<const ScaledPoint> scaled_points,
    std::span<const std::uint64_t> position_by_point_id,
    const Options& options,
    RankMetrics& metrics) {
  require(
      record.anchor_morton_position == anchor_position &&
          record.node_index < nodes.size() &&
          record.flags ==
              (prune_semantics ==
                       MortonYao48DeviceTiledPairFrontierPruneSemantics::
                           closed_rank_window
                   ? kPruneFlagClosedNonnegativeInterval
                   : kPruneFlagStrictPositiveInterval),
      "a prune record has invalid ownership, node, or flags");
  const MortonLbvhSnapshotNode& node =
      nodes[static_cast<std::size_t>(record.node_index)];
  const std::uint64_t width = node.leaf_end - node.leaf_begin;
  require(
      node.leaf_end <= anchor_position &&
          record.certified_pair_mass == width &&
          record.retained_witness_count == required_witness_count &&
          required_witness_count ==
              morsehgp3d::gpu::
                  morton_yao48_device_tiled_pair_frontier_required_witness_count(
                      prune_semantics, maximum_closed_rank) &&
          record.retained_witness_count <= 10U &&
          (record.retained_witness_bank_mask >> 48U) == 0U &&
          record.retained_witness_bank_mask != 0U,
      "a prune record has invalid mass or witness cardinality");
  const PointId anchor_id =
      leaves[static_cast<std::size_t>(anchor_position)].point_id;
  std::array<PointId, 10U> witnesses{};
  std::uint64_t exact_bank_mask = 0U;
  for (std::size_t witness_index = 0U;
       witness_index < 10U;
       ++witness_index) {
    const PointId witness_id = record.witness_point_ids[witness_index];
    if (witness_index >= record.retained_witness_count) {
      require(
          witness_id == 0U,
          "an unused prune witness slot is not canonical zero");
      continue;
    }
    require(
        witness_id < scaled_points.size() && witness_id != anchor_id,
        "a prune record contains an invalid witness PointId");
    const std::uint64_t witness_position =
        position_by_point_id[static_cast<std::size_t>(witness_id)];
    require(
        witness_position >= node.leaf_end &&
            witness_position < anchor_position,
        "a prune witness is not outside the pruned subtree and before anchor");
    const std::size_t cone = exact_cone_index(
        scaled_points[static_cast<std::size_t>(anchor_id)],
        scaled_points[static_cast<std::size_t>(witness_id)]);
    exact_bank_mask |= UINT64_C(1) << cone;
    witnesses[witness_index] = witness_id;
  }
  const auto witness_end =
      witnesses.begin() +
      static_cast<std::ptrdiff_t>(record.retained_witness_count);
  std::sort(witnesses.begin(), witness_end);
  require(
      std::adjacent_find(witnesses.begin(), witness_end) == witness_end &&
          exact_bank_mask == record.retained_witness_bank_mask,
      "a prune record has duplicate witnesses or an invalid bank mask");

  const std::uint64_t full_check_work =
      width * static_cast<std::uint64_t>(record.retained_witness_count);
  const bool bounded_cloud = scaled_points.size() <= 257U;
  const bool affordable_full_check =
      metrics.prune_witness_target_check_count <=
          options.exact_prune_target_check_limit &&
      full_check_work <=
          options.exact_prune_target_check_limit -
              metrics.prune_witness_target_check_count;
  if (bounded_cloud || affordable_full_check) {
    std::vector<std::uint64_t> targets;
    targets.reserve(static_cast<std::size_t>(width));
    for (std::uint64_t position = node.leaf_begin;
         position < node.leaf_end;
         ++position) {
      targets.push_back(position);
    }
    recertify_prune_targets(
        record,
        node,
        leaves,
        scaled_points,
        targets,
        prune_semantics,
        metrics);
    ++metrics.fully_recertified_prune_count;
  } else if (
      metrics.sampled_recertified_prune_count <
      options.sampled_prune_limit) {
    const std::vector<std::uint64_t> targets =
        sampled_target_positions(node.leaf_begin, node.leaf_end, 8U);
    recertify_prune_targets(
        record,
        node,
        leaves,
        scaled_points,
        targets,
        prune_semantics,
        metrics);
    ++metrics.sampled_recertified_prune_count;
  }

  ++metrics.prune_region_count;
  metrics.certified_pruned_pair_mass += width;
  hash_prune(metrics.output_digest, record);
}

void validate_bank_slot(
    const Phase15MortonYao48DeviceTiledWitnessBankSlot& slot,
    std::uint64_t anchor_position,
    std::size_t cone,
    std::span<const MortonLeafRecord> leaves,
    std::span<const ScaledPoint> scaled_points,
    std::span<const std::uint64_t> position_by_point_id,
    std::span<const std::uint64_t> candidate_positions,
    RankMetrics& metrics) {
  require(
      slot.witness_point_id < scaled_points.size() &&
          slot.witness_morton_position < anchor_position &&
          position_by_point_id[static_cast<std::size_t>(slot.witness_point_id)] ==
              slot.witness_morton_position &&
          std::binary_search(
              candidate_positions.begin(),
              candidate_positions.end(),
              slot.witness_morton_position),
      "a retained bank slot is not an emitted earlier candidate");
  const PointId anchor_id =
      leaves[static_cast<std::size_t>(anchor_position)].point_id;
  require(
      exact_cone_index(
          scaled_points[static_cast<std::size_t>(anchor_id)],
          scaled_points[static_cast<std::size_t>(slot.witness_point_id)]) ==
          cone,
      "a retained bank slot is stored in the wrong exact cone");
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const std::int64_t anchor_coordinate =
        scaled_points[static_cast<std::size_t>(anchor_id)].coordinate[axis];
    const std::int64_t witness_coordinate =
        scaled_points[static_cast<std::size_t>(slot.witness_point_id)]
            .coordinate[axis];
    const double expected_witness_coordinate =
        std::ldexp(static_cast<double>(witness_coordinate), -20);
    require(
        slot.witness_coordinate_bits[axis] ==
            std::bit_cast<std::uint64_t>(expected_witness_coordinate),
        "a retained bank slot cached a foreign witness coordinate");
    require(
        slot.direction_lower_bits[axis] != kInvalid &&
            slot.direction_upper_bits[axis] != kInvalid,
        "a retained bank slot has no finite cached direction interval");
    const double direction_lower =
        std::bit_cast<double>(slot.direction_lower_bits[axis]);
    const double direction_upper =
        std::bit_cast<double>(slot.direction_upper_bits[axis]);
    const long double exact_direction = std::ldexp(
        static_cast<long double>(witness_coordinate - anchor_coordinate),
        -20);
    require(
        std::isfinite(direction_lower) &&
            std::isfinite(direction_upper) &&
            direction_lower <= direction_upper &&
            static_cast<long double>(direction_lower) <= exact_direction &&
            exact_direction <= static_cast<long double>(direction_upper) &&
            (exact_direction == 0.0L
                 ? direction_lower == 0.0 && direction_upper == 0.0
                 : (exact_direction > 0.0L
                        ? direction_lower > 0.0
                        : direction_upper < 0.0)),
        "a retained bank slot cached an invalid directional interval");
  }
  require(
      slot.squared_distance_lower_bits != kInvalid &&
          slot.squared_distance_upper_bits != kInvalid,
      "a retained bank slot has no finite distance interval");
  const double lower =
      std::bit_cast<double>(slot.squared_distance_lower_bits);
  const double upper =
      std::bit_cast<double>(slot.squared_distance_upper_bits);
  require(
      std::isfinite(lower) && std::isfinite(upper) && lower >= 0.0 &&
          lower <= upper,
      "a retained bank slot has an invalid distance interval");
  const std::int64_t exact_numerator = exact_squared_distance_numerator(
      scaled_points[static_cast<std::size_t>(anchor_id)],
      scaled_points[static_cast<std::size_t>(slot.witness_point_id)]);
  const long double exact_distance = std::ldexp(
      static_cast<long double>(exact_numerator), -40);
  require(
      static_cast<long double>(lower) <= exact_distance &&
          exact_distance <= static_cast<long double>(upper),
      "a retained bank interval excludes the exact dyadic distance");
  ++metrics.bank_entry_count;
  hash_bank_slot(
      metrics.output_digest,
      anchor_position,
      static_cast<std::uint64_t>(cone),
      slot);
}

void validate_anchor_partition(
    AnchorPartition& partition,
    std::uint64_t anchor_position) {
  std::sort(
      partition.candidate_positions.begin(),
      partition.candidate_positions.end());
  require(
      std::adjacent_find(
          partition.candidate_positions.begin(),
          partition.candidate_positions.end()) ==
          partition.candidate_positions.end(),
      "an anchor emitted a duplicate candidate partner");
  std::sort(partition.prune_intervals.begin(), partition.prune_intervals.end());
  std::uint64_t covered_mass = static_cast<std::uint64_t>(
      partition.candidate_positions.size());
  std::uint64_t prior_end = 0U;
  bool first = true;
  for (const auto& [begin, end] : partition.prune_intervals) {
    require(
        begin < end && end <= anchor_position &&
            (first || begin >= prior_end),
        "an anchor emitted overlapping or out-of-domain prune regions");
    const auto candidate = std::lower_bound(
        partition.candidate_positions.begin(),
        partition.candidate_positions.end(),
        begin);
    require(
        candidate == partition.candidate_positions.end() || *candidate >= end,
        "an anchor candidate overlaps a certified prune region");
    covered_mass += end - begin;
    prior_end = end;
    first = false;
  }
  require(
      covered_mass == anchor_position,
      "an anchor candidate/prune partition does not cover its strict prefix");
}

void bounded_bruteforce_replay(
    std::span<const MortonLeafRecord> leaves,
    std::span<const ScaledPoint> scaled_points,
    std::size_t maximum_closed_rank,
    MortonYao48DeviceTiledPairFrontierPruneSemantics prune_semantics,
    std::span<const std::uint8_t> pair_classification,
    bool require_non_support_shell_equality,
    RankMetrics& metrics) {
  const auto start = Clock::now();
  require(
      scaled_points.size() <= 257U &&
          pair_classification.size() ==
              unordered_pair_count(scaled_points.size()),
      "the bounded brute-force replay has an invalid extent");
  for (std::uint64_t anchor_position = 1U;
       anchor_position < leaves.size();
       ++anchor_position) {
    const PointId anchor_id =
        leaves[static_cast<std::size_t>(anchor_position)].point_id;
    for (std::uint64_t partner_position = 0U;
         partner_position < anchor_position;
         ++partner_position) {
      const PointId partner_id =
          leaves[static_cast<std::size_t>(partner_position)].point_id;
      std::size_t closed_rank = 0U;
      std::size_t strict_interior_count = 0U;
      for (PointId witness_id = 0U;
           witness_id < static_cast<PointId>(scaled_points.size());
           ++witness_id) {
        const int sign = exact_phi_sign(
            scaled_points[static_cast<std::size_t>(witness_id)],
            scaled_points[static_cast<std::size_t>(anchor_id)],
            scaled_points[static_cast<std::size_t>(partner_id)]);
        if (sign <= 0) {
          ++closed_rank;
        }
        if (sign < 0) {
          ++strict_interior_count;
        }
        if (sign == 0 && witness_id != anchor_id &&
            witness_id != partner_id) {
          ++metrics.bounded_non_support_shell_equality_count;
        }
      }
      require(
          closed_rank >= 2U,
          "an exact diametral closed rank omitted a support");
      const std::size_t pair_index = static_cast<std::size_t>(
          triangular_offset(anchor_position, partner_position));
      const std::uint8_t classification = pair_classification[pair_index];
      require(
          classification == 1U || classification == 2U,
          "the bounded device frontier left an unordered pair uncovered");
      ++metrics.bounded_exact_pair_count;
      const bool admitted =
          prune_semantics ==
                  MortonYao48DeviceTiledPairFrontierPruneSemantics::
                      closed_rank_window
              ? closed_rank <= maximum_closed_rank
              : strict_interior_count <
                    morsehgp3d::gpu::
                        morton_yao48_device_tiled_pair_frontier_strict_interior_threshold;
      if (admitted) {
        ++metrics.bounded_admitted_pair_count;
        require(
            classification == 1U,
            "the device frontier pruned a bounded exact admissible pair");
      }
      require(
          classification != 2U || !admitted,
          prune_semantics ==
                  MortonYao48DeviceTiledPairFrontierPruneSemantics::
                      closed_rank_window
              ? "the device frontier pruned a bounded pair inside the "
                "closed-rank window"
              : "the device frontier pruned a pair with fewer than two "
                "strictly interior witnesses");
      if (classification == 1U) {
        if (admitted) {
          ++metrics.bounded_candidate_admitted_pair_count;
        } else {
          ++metrics.bounded_candidate_rejected_pair_count;
        }
      }
    }
  }
  require(
      metrics.bounded_exact_pair_count == metrics.unordered_pair_universe_count,
      "the bounded brute-force replay did not visit every unordered pair");
  if (require_non_support_shell_equality) {
    require(
        metrics.bounded_non_support_shell_equality_count != 0U,
        "the bounded qualification did not exercise a non-support shell equality");
  }
  metrics.bounded_bruteforce_ns = nanoseconds(Clock::now() - start);
  metrics.bounded_bruteforce_performed = true;
}

[[nodiscard]] std::uint64_t peak_host_rss_bytes() {
#if defined(__linux__)
  rusage usage{};
  if (getrusage(RUSAGE_SELF, &usage) != 0 || usage.ru_maxrss < 0) {
    throw std::runtime_error("getrusage could not read peak host RSS");
  }
  const std::uint64_t kibibytes =
      static_cast<std::uint64_t>(usage.ru_maxrss);
  if (kibibytes >
      std::numeric_limits<std::uint64_t>::max() / UINT64_C(1024)) {
    throw std::overflow_error("the peak host RSS byte count overflows uint64");
  }
  return kibibytes * UINT64_C(1024);
#else
  return 0U;
#endif
}

void observe_scaling_device_memory(
    ScalingSmokeMetrics& metrics,
    std::string_view role) {
  std::size_t free_bytes = 0U;
  std::size_t total_bytes = 0U;
  const cudaError_t status = cudaMemGetInfo(&free_bytes, &total_bytes);
  if (status != cudaSuccess) {
    const char* const description = cudaGetErrorString(status);
    throw std::runtime_error(
        std::string{role} + ": " +
        (description == nullptr ? "unknown CUDA error" : description));
  }
  const std::uint64_t free_u64 = checked_u64(
      free_bytes, "the scaling free-device-memory count does not fit uint64");
  const std::uint64_t total_u64 = checked_u64(
      total_bytes,
      "the scaling total-device-memory count does not fit uint64");
  require(
      total_u64 != 0U && free_u64 <= total_u64,
      "the scaling CUDA memory observation is inconsistent");
  if (metrics.device_memory_observation_count == 0U) {
    metrics.device_total_bytes = total_u64;
    metrics.initial_device_free_bytes = free_u64;
    metrics.minimum_device_free_bytes = free_u64;
  } else {
    require(
        metrics.device_total_bytes == total_u64,
        "the scaling CUDA device capacity changed during one process");
    metrics.minimum_device_free_bytes =
        std::min(metrics.minimum_device_free_bytes, free_u64);
  }
  metrics.final_device_free_bytes = free_u64;
  ++metrics.device_memory_observation_count;
}

void validate_scaling_tile_lease(
    const MortonYao48DeviceCandidateTileLeaseAudit& audit,
    const Options& options,
    std::size_t expected_anchor_begin,
    std::size_t expected_anchor_end) {
  const std::size_t expected_node_count = 2U * options.point_count - 1U;
  const std::size_t expected_node_bound_view_capacity_bytes =
      checked_product(
          expected_node_count,
          sizeof(Phase15MortonYao48DeviceTiledNodeBounds),
          "the scaling-smoke node-bound view extent overflows size_t");
  require(
      audit.schema_version ==
              morsehgp3d::gpu::
                  morton_yao48_device_tiled_pair_frontier_schema_version &&
          audit.point_count == options.point_count &&
          audit.certified_node_count == 2U * options.point_count - 1U &&
          audit.retained_node_bound_view_capacity_bytes ==
              expected_node_bound_view_capacity_bytes &&
          audit.node_bound_view_allocation_capacity_bytes ==
              checked_size_sum(
                  expected_node_bound_view_capacity_bytes,
                  2U * sizeof(std::uint64_t),
                  "the scaling-smoke node-bound allocation extent overflows size_t") &&
          audit.resolved_node_bound_count == expected_node_count &&
          audit.node_bound_view_build_allocation_count == 1U &&
          audit.node_bound_view_build_kernel_launch_count == 1U &&
          audit.node_bound_view_build_synchronization_count == 1U &&
          audit.node_bound_view_validation_device_to_host_byte_count ==
              2U * sizeof(std::uint64_t) &&
          audit.maximum_closed_rank ==
              options.maximum_closed_rank.value_or(11U) &&
          audit.prune_semantics ==
              MortonYao48DeviceTiledPairFrontierPruneSemantics::
                  closed_rank_window &&
          audit.required_witness_count == audit.maximum_closed_rank - 1U &&
          audit.anchor_begin == expected_anchor_begin &&
          audit.anchor_end == expected_anchor_end &&
          audit.fixed_candidate_capacity_per_anchor ==
              morsehgp3d::gpu::
                  morton_yao48_device_tiled_pair_frontier_candidates_per_anchor &&
          audit.fixed_prune_region_capacity_per_anchor ==
              morsehgp3d::gpu::
                  morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor &&
          audit.fixed_witness_bank_count_per_anchor ==
              morsehgp3d::gpu::
                  morton_yao48_device_tiled_pair_frontier_witness_bank_count &&
          audit.tile_epoch != 0U && audit.chunk_sequence != 0U &&
          audit.resumes_same_tile == (audit.chunk_sequence > 1U) &&
          !audit.process_restart_resumable &&
          (audit.yield_reason ==
               MortonYao48DeviceTiledPairFrontierYieldReason::none) ==
              !audit.resumable_after_lease_release,
      "the scaling-smoke tile lease has inconsistent extents");
  require(
      audit.traversal_owner_retained &&
          audit.source_cloud_identity_retained &&
          audit.source_device_views_retained &&
          audit.source_device_extents_retained &&
          audit.source_views_bound_to_snapshot_identity &&
          audit.node_bound_view_extent_authenticated &&
          audit.node_bound_view_validation_sentinel_authenticated &&
          audit.node_bound_view_bound_to_snapshot_identity &&
          audit.node_bound_view_built_once_per_adoption &&
          audit.node_bound_view_reused_without_tile_allocation &&
          audit.source_node_extremum_point_ids_retained &&
          audit.node_bound_view_build_included_in_context_creation &&
          audit.output_owner_retained &&
          audit.output_buffers_detached_for_tile_lifetime &&
          audit.cuda_device_storage_retained &&
          !audit.host_fake_lifecycle_exercised,
      "the scaling-smoke tile lease lost a CUDA authority");
  require(
      !audit.candidate_device_to_host_performed &&
          !audit.certified_prune_device_to_host_performed &&
          audit.witness_direction_intervals_cached_per_bank_slot &&
          audit.active_witness_slot_mask_authenticated &&
          audit.inactive_witness_slots_skipped_in_physical_order &&
          audit.last_prune_witness_direction_cache_retained &&
          audit.nonnegative_diametral_witness_interval_lower_bound_required &&
          !audit
               .strictly_positive_diametral_witness_interval_lower_bound_required &&
          !audit.exact_diametral_rank_evaluated &&
          !audit.scientific_pair_catalog_published &&
          !audit.dense_pair_fallback_performed &&
          !audit.global_pair_matrix_materialized &&
          !audit.higher_order_structure_materialized &&
          !audit.public_status_claimed,
      "the scaling-smoke tile lease made a forbidden scientific claim");
}

void validate_scaling_advance(
    const MortonYao48DeviceTiledPairFrontierAdvance& advance,
    const Options& options,
    std::size_t expected_advance_sequence,
    std::size_t expected_anchor_begin) {
  const MortonYao48DeviceTiledPairFrontierAudit& audit = advance.audit;
  const std::size_t maximum_closed_rank =
      options.maximum_closed_rank.value_or(11U);
  const std::size_t maximum_subdivision_count =
      traversal_subdivision_ceiling(
          audit.certified_node_count,
          morsehgp3d::gpu::
              morton_yao48_device_tiled_pair_frontier_node_visits_per_anchor);
  const std::size_t expected_node_bound_view_capacity_bytes =
      checked_product(
          audit.certified_node_count,
          sizeof(Phase15MortonYao48DeviceTiledNodeBounds),
          "the scaling-smoke public node-bound view extent overflows size_t");
  require(
      audit.schema_version ==
              morsehgp3d::gpu::
                  morton_yao48_device_tiled_pair_frontier_schema_version &&
          audit.advance_sequence == expected_advance_sequence &&
          audit.point_count == options.point_count &&
          audit.certified_node_count == 2U * options.point_count - 1U &&
          audit.retained_node_bound_view_capacity_bytes ==
              expected_node_bound_view_capacity_bytes &&
          audit.node_bound_view_allocation_capacity_bytes ==
              checked_size_sum(
                  expected_node_bound_view_capacity_bytes,
                  2U * sizeof(std::uint64_t),
                  "the scaling-smoke public node-bound allocation extent overflows size_t") &&
          audit.resolved_node_bound_count == audit.certified_node_count &&
          audit.node_bound_view_build_allocation_count == 1U &&
          audit.node_bound_view_build_kernel_launch_count == 1U &&
          audit.node_bound_view_build_synchronization_count == 1U &&
          audit.node_bound_view_validation_device_to_host_byte_count ==
              2U * sizeof(std::uint64_t) &&
          audit.maximum_closed_rank == maximum_closed_rank &&
          audit.anchor_tile_capacity == options.anchor_tile_capacity &&
          audit.fixed_node_visit_capacity_per_anchor ==
              morsehgp3d::gpu::
                  morton_yao48_device_tiled_pair_frontier_node_visits_per_anchor &&
          audit.maximum_traversal_subdivision_count_per_anchor ==
              maximum_subdivision_count &&
          audit.fixed_candidate_capacity_per_anchor ==
              morsehgp3d::gpu::
                  morton_yao48_device_tiled_pair_frontier_candidates_per_anchor &&
          audit.fixed_prune_region_capacity_per_anchor ==
              morsehgp3d::gpu::
                  morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor &&
          audit.fixed_witness_bank_count_per_anchor ==
              morsehgp3d::gpu::
                  morton_yao48_device_tiled_pair_frontier_witness_bank_count &&
          audit.tile_epoch != 0U && audit.chunk_sequence != 0U &&
          audit.resumes_same_tile == (audit.chunk_sequence > 1U) &&
          !audit.process_restart_resumable &&
          advance.yield_reason == audit.yield_reason &&
          (audit.yield_reason ==
               MortonYao48DeviceTiledPairFrontierYieldReason::none) ==
              !audit.resumable_capacity_yield,
      "the scaling-smoke public audit has inconsistent fixed metadata");
  require(
      audit.transaction_traversal_subdivision_count != 0U &&
          audit.transaction_traversal_subdivision_count <=
              maximum_subdivision_count &&
          audit.cumulative_traversal_subdivision_count >=
              audit.transaction_traversal_subdivision_count &&
          audit.cuda_kernel_launch_count ==
              audit.transaction_traversal_subdivision_count &&
          audit.cuda_synchronization_count ==
              audit.transaction_traversal_subdivision_count + 1U &&
          audit.resume_control_device_to_host_count ==
              audit.transaction_traversal_subdivision_count &&
          audit.resume_control_device_to_host_byte_count ==
              audit.transaction_traversal_subdivision_count *
                  sizeof(std::uint64_t),
      "the scaling-smoke public audit has inconsistent resume receipts");
  require(
      audit.transaction_anchor_begin == expected_anchor_begin &&
          audit.transaction_anchor_end > audit.transaction_anchor_begin &&
          audit.transaction_anchor_end <= options.point_count &&
          audit.transaction_committed_anchor_count <=
              audit.transaction_anchor_end - audit.transaction_anchor_begin &&
          audit.next_anchor_position ==
              audit.transaction_anchor_begin +
                  audit.transaction_committed_anchor_count &&
          audit.completed_anchor_count + 1U == audit.next_anchor_position,
      "the scaling-smoke public audit has an invalid committed prefix");
  require(
      audit.cumulative_candidate_pair_mass +
                  audit.cumulative_certified_pruned_pair_mass +
              audit.unresolved_pair_mass ==
          audit.unordered_pair_universe_count &&
          audit.unordered_pair_universe_count ==
              unordered_pair_count(options.point_count),
      "the scaling-smoke public audit does not close its global mass identity");
  require(
      audit.source_traversal_lease_authenticated &&
          audit.node_bound_view_extent_authenticated &&
          audit.node_bound_view_validation_sentinel_authenticated &&
          audit.node_bound_view_bound_to_snapshot_identity &&
          audit.node_bound_view_built_once_per_adoption &&
          audit.node_bound_view_reused_without_tile_allocation &&
          audit.source_node_extremum_point_ids_retained &&
          audit.node_bound_view_build_included_in_context_creation &&
          audit.fixed_per_anchor_caps_enforced &&
          audit.atomic_completed_anchor_prefix_validated &&
          audit.candidate_pruned_unresolved_partition_validated &&
          audit.traversal_lease_owner_retained &&
          audit.source_cloud_identity_retained &&
          audit.candidate_tile_lease_backpressure_bounded_to_one &&
          audit.cuda_execution_performed &&
          !audit.host_fake_launcher_exercised,
      "the scaling-smoke public audit did not authenticate its CUDA path");
  require(
      !audit.candidate_device_to_host_performed &&
          !audit.certified_prune_device_to_host_performed &&
          audit.candidate_device_to_host_count == 0U &&
          audit.certified_prune_device_to_host_count == 0U &&
          !audit.exact_diametral_rank_evaluated &&
          !audit.scientific_pair_catalog_published &&
          !audit.scientific_decision_published &&
          !audit.dense_pair_fallback_performed &&
          !audit.global_pair_matrix_materialized &&
          !audit.ordinary_delaunay_materialized &&
          !audit.higher_order_delaunay_mosaic_materialized &&
          !audit.global_cell_or_coface_arena_materialized &&
          !audit.public_status_claimed,
      "the scaling-smoke public path performed a forbidden operation");
  require(
      audit.interval_cone_classification_required &&
          audit.ambiguous_cone_routed_to_unbanked_candidate &&
          audit.target_tested_before_witness_bank_insert &&
          audit.retained_witnesses_outside_pruned_subtree_required &&
          audit.witness_direction_intervals_cached_per_bank_slot &&
          audit.active_witness_slot_mask_authenticated &&
          audit.inactive_witness_slots_skipped_in_physical_order &&
          audit.last_prune_witness_direction_cache_retained &&
          audit.nonnegative_diametral_witness_interval_lower_bound_required &&
          !audit
               .strictly_positive_diametral_witness_interval_lower_bound_required &&
          audit.prune_semantics ==
              MortonYao48DeviceTiledPairFrontierPruneSemantics::
                  closed_rank_window &&
          audit.required_witness_count == audit.maximum_closed_rank - 1U,
      "the scaling-smoke public audit omitted a proof obligation");
  const bool has_tile = advance.candidate_tile.has_value();
  require(
      audit.candidate_tile_lease_published == has_tile &&
          audit.candidate_tile_lease_outstanding == has_tile &&
          has_tile ==
              (audit.transaction_candidate_pair_mass != 0U ||
               audit.transaction_certified_prune_region_count != 0U),
      "the scaling-smoke public audit violates single-tile backpressure");
  if (advance.status ==
      MortonYao48DeviceTiledPairFrontierStatus::frontier_complete) {
    require(
        advance.stop_reason ==
                MortonYao48DeviceTiledPairFrontierStopReason::none &&
            advance.yield_reason ==
                MortonYao48DeviceTiledPairFrontierYieldReason::none &&
            !audit.resumable_capacity_yield &&
            audit.pair_coverage_partition_complete &&
            !audit.terminally_censored &&
            audit.next_anchor_position == options.point_count &&
            audit.unresolved_pair_mass == 0U,
        "the scaling-smoke public frontier completion is inconsistent");
  } else if (
      advance.status ==
      MortonYao48DeviceTiledPairFrontierStatus::tile_complete) {
    require(
        advance.stop_reason ==
                MortonYao48DeviceTiledPairFrontierStopReason::none &&
            advance.yield_reason ==
                MortonYao48DeviceTiledPairFrontierYieldReason::none &&
            !audit.resumable_capacity_yield &&
            !audit.pair_coverage_partition_complete &&
            !audit.terminally_censored &&
            audit.transaction_committed_anchor_count ==
                audit.transaction_anchor_end -
                    audit.transaction_anchor_begin,
        "the scaling-smoke public tile completion is inconsistent");
  } else if (
      advance.status ==
      MortonYao48DeviceTiledPairFrontierStatus::chunk_ready) {
    require(
        advance.stop_reason ==
                MortonYao48DeviceTiledPairFrontierStopReason::none &&
            (advance.yield_reason ==
                 MortonYao48DeviceTiledPairFrontierYieldReason::
                     candidate_segment_full ||
             advance.yield_reason ==
                 MortonYao48DeviceTiledPairFrontierYieldReason::
                     prune_segment_full ||
             advance.yield_reason ==
                 MortonYao48DeviceTiledPairFrontierYieldReason::
                     mixed_segments_full) &&
            audit.resumable_capacity_yield &&
            audit.transaction_committed_anchor_count == 0U &&
            audit.next_anchor_position == audit.transaction_anchor_begin &&
            !audit.pair_coverage_partition_complete &&
            !audit.terminally_censored,
        "the scaling public chunk-ready continuation is inconsistent");
  } else {
    require(
        advance.status == MortonYao48DeviceTiledPairFrontierStatus::censored &&
            advance.stop_reason !=
                MortonYao48DeviceTiledPairFrontierStopReason::none &&
            advance.yield_reason ==
                MortonYao48DeviceTiledPairFrontierYieldReason::none &&
            !audit.resumable_capacity_yield &&
            audit.terminally_censored &&
            audit.censored_anchor_outputs_withheld &&
            !audit.pair_coverage_partition_complete,
        "the scaling-smoke public censure is not fail-stop");
  }
}

[[nodiscard]] ScalingSmokeMetrics run_scaling_smoke(
    const Options& options,
    std::vector<CertifiedPoint3> input_points,
    std::uint64_t generation_ns,
    Clock::time_point total_start) {
  ScalingSmokeMetrics metrics;
  metrics.maximum_closed_rank =
      options.maximum_closed_rank.value_or(11U);
  metrics.generation_ns = generation_ns;
  metrics.unordered_pair_universe_count =
      unordered_pair_count(options.point_count);
  observe_scaling_device_memory(
      metrics, "cudaMemGetInfo before large-scale qualification");

  const auto canonicalization_start = Clock::now();
  std::optional<CanonicalPointCloud> cloud;
  cloud.emplace(CanonicalPointCloud::rejecting_duplicates(input_points));
  metrics.canonicalization_ns =
      nanoseconds(Clock::now() - canonicalization_start);
  input_points.clear();
  input_points.shrink_to_fit();

  std::optional<MortonLbvhDeviceTraversalLease> traversal_lease;
  Clock::time_point host_release_start{};
  {
    MortonLbvhBuildContext builder{options.point_count};
    const auto build_start = Clock::now();
    MortonLbvhDeviceBuildResult build = builder.build(*cloud);
    metrics.build_ns = nanoseconds(Clock::now() - build_start);
    require(
        build.complete_certified_build() && build.cuda_qualified_build() &&
            build.certified_index().validated_for(*cloud) &&
            build.audit().point_count == options.point_count &&
            build.audit().required_node_count ==
                2U * options.point_count - 1U &&
            !build.audit().higher_order_delaunay_mosaic_materialized &&
            !build.audit().global_cell_or_coface_arena_materialized &&
            !build.audit().public_status_claimed,
        "the scaling-smoke LBVH build is not a certified CUDA build");
    const auto lease_start = Clock::now();
    traversal_lease.emplace(
        builder.release_device_traversal_lease(build));
    metrics.lease_release_ns = nanoseconds(Clock::now() - lease_start);
    const auto& lease_audit = traversal_lease->audit();
    require(
        traversal_lease->ready() && traversal_lease->cuda_resident() &&
            lease_audit.point_count == options.point_count &&
            lease_audit.certified_node_count ==
                2U * options.point_count - 1U &&
            lease_audit.canonical_coordinate_words_retained &&
            lease_audit.active_morton_point_ids_retained &&
            lease_audit.certified_device_nodes_retained &&
            lease_audit.cuda_device_storage_retained &&
            !lease_audit.host_fake_lifecycle_exercised &&
            !lease_audit.second_host_snapshot_retained &&
            !lease_audit.higher_order_delaunay_mosaic_materialized &&
            !lease_audit.global_cell_or_coface_arena_materialized &&
            !lease_audit.public_status_claimed,
        "the scaling-smoke traversal lease is not CUDA-resident");
    metrics.traversal_device_capacity_bytes = checked_u64(
        lease_audit.persistent_device_byte_capacity,
        "the scaling traversal capacity does not fit uint64");
    observe_scaling_device_memory(
        metrics, "cudaMemGetInfo after large-scale LBVH build");
    host_release_start = Clock::now();
  }
  cloud.reset();
  metrics.host_release_ns =
      nanoseconds(Clock::now() - host_release_start);

  const MortonYao48DeviceTiledPairFrontierConfig config{
      metrics.maximum_closed_rank,
      options.anchor_tile_capacity};
  const auto context_start = Clock::now();
  std::optional<MortonYao48DeviceTiledPairFrontierContext> context;
  context.emplace(std::move(*traversal_lease), config);
  traversal_lease.reset();
  metrics.context_creation_ns = nanoseconds(Clock::now() - context_start);
  require(
      context->ready() && !context->poisoned() &&
          context->config() == config,
      "the scaling-smoke public context is not ready");
  observe_scaling_device_memory(
      metrics, "cudaMemGetInfo after large-scale frontier allocation");

  std::size_t expected_anchor_begin = 1U;
  std::uint64_t active_tile_epoch = 0U;
  std::uint64_t expected_chunk_sequence = 1U;
  const auto frontier_start = Clock::now();
  while (true) {
    MortonYao48DeviceTiledPairFrontierAdvance advance = context->advance();
    ++metrics.advance_count;
    validate_scaling_advance(
        advance, options, metrics.advance_count, expected_anchor_begin);
    const MortonYao48DeviceTiledPairFrontierAudit audit = advance.audit;
    if (active_tile_epoch == 0U) {
      require(
          audit.chunk_sequence == 1U && !audit.resumes_same_tile,
          "a scaling tile did not start at chunk one");
      active_tile_epoch = audit.tile_epoch;
      expected_chunk_sequence = 1U;
    }
    require(
        audit.tile_epoch == active_tile_epoch &&
            audit.chunk_sequence == expected_chunk_sequence,
        "a scaling tile changed epoch or skipped a chunk sequence");
    if (audit.resumes_same_tile) {
      ++metrics.resumed_advance_count;
    }
    if (advance.status ==
        MortonYao48DeviceTiledPairFrontierStatus::chunk_ready) {
      ++metrics.chunk_ready_advance_count;
      switch (advance.yield_reason) {
        case MortonYao48DeviceTiledPairFrontierYieldReason::
            candidate_segment_full:
          ++metrics.candidate_yield_count;
          break;
        case MortonYao48DeviceTiledPairFrontierYieldReason::
            prune_segment_full:
          ++metrics.prune_yield_count;
          break;
        case MortonYao48DeviceTiledPairFrontierYieldReason::
            mixed_segments_full:
          ++metrics.mixed_yield_count;
          break;
        case MortonYao48DeviceTiledPairFrontierYieldReason::none:
          throw QualificationMismatch(
              "a chunk-ready scaling advance has no yield reason");
      }
    }
    metrics.completed_anchor_count = audit.completed_anchor_count;
    metrics.certified_prune_region_count +=
        audit.transaction_certified_prune_region_count;
    metrics.candidate_pair_mass = audit.cumulative_candidate_pair_mass;
    metrics.certified_pruned_pair_mass =
        audit.cumulative_certified_pruned_pair_mass;
    metrics.unresolved_pair_mass = audit.unresolved_pair_mass;
    metrics.physical_node_visit_count =
        audit.cumulative_physical_node_visit_count;
    metrics.launcher_call_count = audit.launcher_call_count;
    metrics.kernel_launch_count += audit.cuda_kernel_launch_count;
    metrics.synchronization_count += audit.cuda_synchronization_count;
    metrics.traversal_subdivision_count =
        audit.cumulative_traversal_subdivision_count;
    metrics.maximum_traversal_subdivision_count_per_anchor =
        audit.maximum_traversal_subdivision_count_per_anchor;
    metrics.resume_control_device_to_host_count +=
        audit.resume_control_device_to_host_count;
    metrics.resume_control_device_to_host_byte_count +=
        audit.resume_control_device_to_host_byte_count;
    metrics.anchor_control_device_to_host_count +=
        audit.anchor_control_device_to_host_count;
    metrics.anchor_control_device_to_host_byte_count +=
        audit.anchor_control_device_to_host_byte_count;
    metrics.retained_node_bound_view_capacity_bytes =
        audit.retained_node_bound_view_capacity_bytes;
    metrics.node_bound_view_allocation_capacity_bytes =
        audit.node_bound_view_allocation_capacity_bytes;
    metrics.resolved_node_bound_count = audit.resolved_node_bound_count;
    metrics.node_bound_view_build_allocation_count =
        audit.node_bound_view_build_allocation_count;
    metrics.node_bound_view_build_kernel_launch_count =
        audit.node_bound_view_build_kernel_launch_count;
    metrics.node_bound_view_build_synchronization_count =
        audit.node_bound_view_build_synchronization_count;
    metrics.node_bound_view_validation_device_to_host_byte_count =
        audit.node_bound_view_validation_device_to_host_byte_count;
    expected_anchor_begin = audit.next_anchor_position;

    const std::size_t requested_anchor_count =
        audit.transaction_anchor_end - audit.transaction_anchor_begin;
    const std::uint64_t candidate_bytes = checked_u64(
        checked_product(
            checked_product(
                requested_anchor_count,
                morsehgp3d::gpu::
                    morton_yao48_device_tiled_pair_frontier_candidates_per_anchor,
                "the scaling candidate record capacity overflows size_t"),
            sizeof(Phase15MortonYao48DeviceTiledCandidateRecord),
            "the scaling candidate byte capacity overflows size_t"),
        "the scaling candidate byte capacity does not fit uint64");
    const std::uint64_t prune_bytes = checked_u64(
        checked_product(
            checked_product(
                requested_anchor_count,
                morsehgp3d::gpu::
                    morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor,
                "the scaling prune record capacity overflows size_t"),
            sizeof(Phase15MortonYao48DeviceTiledPruneRegionRecord),
            "the scaling prune byte capacity overflows size_t"),
        "the scaling prune byte capacity does not fit uint64");
    const std::uint64_t witness_bytes = checked_u64(
        checked_product(
            checked_product(
                checked_product(
                    requested_anchor_count,
                    morsehgp3d::gpu::
                        morton_yao48_device_tiled_pair_frontier_witness_bank_count,
                    "the scaling witness bank capacity overflows size_t"),
                metrics.maximum_closed_rank - 1U,
                "the scaling witness slot capacity overflows size_t"),
            sizeof(Phase15MortonYao48DeviceTiledWitnessBankSlot),
            "the scaling witness byte capacity overflows size_t"),
        "the scaling witness byte capacity does not fit uint64");
    const std::uint64_t cached_prune_witness_bytes = checked_u64(
        checked_product(
            checked_product(
                requested_anchor_count,
                morsehgp3d::gpu::
                    morton_yao48_device_tiled_pair_frontier_cached_prune_witnesses_per_anchor,
                "the scaling cached prune witness capacity overflows "
                "size_t"),
            sizeof(Phase15MortonYao48DeviceTiledCachedPruneWitness),
            "the scaling cached prune witness byte capacity overflows "
            "size_t"),
        "the scaling cached prune witness byte capacity does not fit "
        "uint64");
    const std::uint64_t control_bytes = checked_u64(
        checked_product(
            requested_anchor_count,
            sizeof(Phase15MortonYao48DeviceTiledAnchorControl),
            "the scaling control byte capacity overflows size_t"),
        "the scaling control byte capacity does not fit uint64");
    const std::uint64_t checkpoint_bytes = checked_u64(
        checked_product(
            requested_anchor_count,
            sizeof(Phase15MortonYao48DeviceTiledAnchorCheckpoint),
            "the scaling checkpoint byte capacity overflows size_t"),
        "the scaling checkpoint byte capacity does not fit uint64");
    std::uint64_t tile_arena_bytes = checked_u64_sum(
        candidate_bytes,
        prune_bytes,
        "the scaling tile arena bytes overflow uint64");
    tile_arena_bytes = checked_u64_sum(
        tile_arena_bytes,
        witness_bytes,
        "the scaling tile arena bytes overflow uint64");
    tile_arena_bytes = checked_u64_sum(
        tile_arena_bytes,
        cached_prune_witness_bytes,
        "the scaling tile arena bytes overflow uint64");
    tile_arena_bytes = checked_u64_sum(
        tile_arena_bytes,
        control_bytes,
        "the scaling tile arena bytes overflow uint64");
    tile_arena_bytes = checked_u64_sum(
        tile_arena_bytes,
        checkpoint_bytes,
        "the scaling tile arena bytes overflow uint64");
    tile_arena_bytes = checked_u64_sum(
        tile_arena_bytes,
        sizeof(std::uint64_t),
        "the scaling tile arena bytes overflow uint64");
    metrics.peak_candidate_device_capacity_bytes = std::max(
        metrics.peak_candidate_device_capacity_bytes, candidate_bytes);
    metrics.peak_prune_device_capacity_bytes = std::max(
        metrics.peak_prune_device_capacity_bytes, prune_bytes);
    metrics.peak_witness_bank_device_capacity_bytes = std::max(
        metrics.peak_witness_bank_device_capacity_bytes, witness_bytes);
    metrics.peak_control_device_capacity_bytes = std::max(
        metrics.peak_control_device_capacity_bytes, control_bytes);
    metrics.peak_tile_output_device_capacity_bytes = std::max(
        metrics.peak_tile_output_device_capacity_bytes,
        tile_arena_bytes);
    observe_scaling_device_memory(
        metrics, "cudaMemGetInfo with a large-scale tile resident");

    if (advance.candidate_tile.has_value()) {
      require(
          advance.candidate_tile->ready() &&
              advance.candidate_tile->cuda_resident() &&
              !advance.candidate_tile->host_fake(),
          "the scaling-smoke public candidate tile is not CUDA-resident");
      const MortonYao48DeviceCandidateTileLeaseAudit lease_audit =
          advance.candidate_tile->audit();
      validate_scaling_tile_lease(
          lease_audit,
          options,
          audit.transaction_anchor_begin,
          audit.transaction_anchor_end);
      require(
          lease_audit.tile_epoch == audit.tile_epoch &&
              lease_audit.chunk_sequence == audit.chunk_sequence &&
              lease_audit.yield_reason == audit.yield_reason &&
              lease_audit.resumes_same_tile == audit.resumes_same_tile,
          "the scaling lease does not match its chunk audit");
      metrics.peak_candidate_device_capacity_bytes = std::max(
          metrics.peak_candidate_device_capacity_bytes,
          checked_u64(
              checked_product(
                  lease_audit.physical_candidate_capacity,
                  sizeof(Phase15MortonYao48DeviceTiledCandidateRecord),
                  "the scaling candidate capacity overflows size_t"),
              "the scaling candidate bytes do not fit uint64"));
      metrics.peak_prune_device_capacity_bytes = std::max(
          metrics.peak_prune_device_capacity_bytes,
          checked_u64(
              checked_product(
                  lease_audit.physical_prune_region_capacity,
                  sizeof(Phase15MortonYao48DeviceTiledPruneRegionRecord),
                  "the scaling prune capacity overflows size_t"),
              "the scaling prune bytes do not fit uint64"));
      ++metrics.candidate_tile_lease_count;
      advance.candidate_tile.reset();
      ++metrics.candidate_tile_release_count;
    }

    if (advance.status ==
        MortonYao48DeviceTiledPairFrontierStatus::frontier_complete) {
      metrics.coverage_complete = true;
      metrics.stop_reason = MortonYao48DeviceTiledPairFrontierStopReason::none;
      break;
    }
    if (advance.status ==
        MortonYao48DeviceTiledPairFrontierStatus::chunk_ready) {
      if (expected_chunk_sequence ==
          std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error(
            "the scaling chunk sequence overflowed uint64_t");
      }
      ++expected_chunk_sequence;
      continue;
    }
    if (advance.status ==
        MortonYao48DeviceTiledPairFrontierStatus::censored) {
      metrics.censored = true;
      metrics.stop_reason = advance.stop_reason;
      break;
    }
    active_tile_epoch = 0U;
    expected_chunk_sequence = 1U;
  }
  metrics.frontier_ns = nanoseconds(Clock::now() - frontier_start);
  const bool pair_mass_partition_validated =
      metrics.candidate_pair_mass + metrics.certified_pruned_pair_mass +
              metrics.unresolved_pair_mass ==
          metrics.unordered_pair_universe_count;
  const bool complete_coverage_validated =
      metrics.coverage_complete && !metrics.censored &&
      metrics.stop_reason ==
          MortonYao48DeviceTiledPairFrontierStopReason::none &&
      metrics.unresolved_pair_mass == 0U &&
      metrics.candidate_pair_mass + metrics.certified_pruned_pair_mass ==
          metrics.unordered_pair_universe_count;
  const bool censored_direct_scale_diagnostic_validated =
      options.direct_scale && !metrics.coverage_complete &&
      metrics.censored &&
      metrics.stop_reason !=
          MortonYao48DeviceTiledPairFrontierStopReason::none &&
      metrics.unresolved_pair_mass != 0U && pair_mass_partition_validated;
  require(
      metrics.candidate_tile_lease_count ==
              metrics.candidate_tile_release_count &&
          !context->poisoned() &&
          context->terminally_censored() == metrics.censored &&
          pair_mass_partition_validated &&
          (complete_coverage_validated ||
           censored_direct_scale_diagnostic_validated),
      "the large-scale profile did not close its terminal mass ledger or "
      "release exactly one tile before advancing");
  metrics.backpressure_validated = true;
  metrics.execution_success = true;
  const auto context_release_start = Clock::now();
  context.reset();
  metrics.context_release_ns =
      nanoseconds(Clock::now() - context_release_start);
  observe_scaling_device_memory(
      metrics, "cudaMemGetInfo after large-scale context release");
  metrics.total_ns = nanoseconds(Clock::now() - total_start);
  return metrics;
}

[[nodiscard]] std::vector<ScaledPoint> canonical_scaled_points(
    const CanonicalPointCloud& cloud,
    std::span<const ScaledPoint> source_scaled_points) {
  std::vector<ScaledPoint> canonical(cloud.size());
  for (PointId point_id = 0U;
       point_id < static_cast<PointId>(cloud.size());
       ++point_id) {
    const std::size_t source_index = cloud.source_index(point_id);
    require(
        source_index < source_scaled_points.size(),
        "a canonical point has an invalid qualification source index");
    canonical[static_cast<std::size_t>(point_id)] =
        source_scaled_points[source_index];
  }
  return canonical;
}

[[nodiscard]] RankMetrics qualify_rank(
    const Options& options,
    const CanonicalPointCloud& cloud,
    std::span<const ScaledPoint> scaled_points,
    std::size_t maximum_closed_rank,
    MortonYao48DeviceTiledPairFrontierPruneSemantics prune_semantics,
    std::vector<std::uint8_t>* point_id_pair_classification = nullptr) {
  RankMetrics metrics;
  metrics.maximum_closed_rank = maximum_closed_rank;
  metrics.prune_semantics = prune_semantics;
  const bool strict_interior_semantics =
      prune_semantics ==
      MortonYao48DeviceTiledPairFrontierPruneSemantics::
          strict_interior_threshold;
  metrics.q3_exact_diametral_pair_support_gabriel_negative_only =
      strict_interior_semantics;
  metrics.gamma2_silent_handoff_required = strict_interior_semantics;
  metrics.gamma2_prune_or_discard_authorized = false;
  metrics.required_witness_count =
      morsehgp3d::gpu::
          morton_yao48_device_tiled_pair_frontier_required_witness_count(
              prune_semantics, maximum_closed_rank);
  require(
      metrics.required_witness_count != 0U &&
          metrics.required_witness_count <= 10U,
      "the qualification prune semantics has no valid witness threshold");
  metrics.unordered_pair_universe_count = unordered_pair_count(cloud.size());
  metrics.output_digest = hash_word(
      metrics.output_digest, static_cast<std::uint64_t>(maximum_closed_rank));
  metrics.output_digest = hash_word(
      metrics.output_digest, static_cast<std::uint64_t>(prune_semantics));
  metrics.output_digest = hash_word(
      metrics.output_digest,
      static_cast<std::uint64_t>(metrics.required_witness_count));

  const std::size_t node_count = 2U * cloud.size() - 1U;
  MortonLbvhBuildContext builder{cloud.size()};
  const auto build_start = Clock::now();
  MortonLbvhDeviceBuildResult build = builder.build(cloud);
  metrics.build_ns = nanoseconds(Clock::now() - build_start);
  require(
      build.complete_certified_build() && build.cuda_qualified_build() &&
          build.certified_index().validated_for(cloud) &&
          build.audit().point_count == cloud.size() &&
          build.audit().required_node_count == node_count &&
          !build.audit().higher_order_delaunay_mosaic_materialized &&
          !build.audit().global_cell_or_coface_arena_materialized &&
          !build.audit().public_status_claimed,
      "the qualification LBVH build is not CUDA-qualified and certified");

  const auto lease_start = Clock::now();
  MortonLbvhDeviceTraversalLease lease =
      builder.release_device_traversal_lease(build);
  metrics.lease_release_ns = nanoseconds(Clock::now() - lease_start);
  const auto lease_audit = lease.audit();
  require(
      lease.ready() && lease.cuda_resident() &&
          lease_audit.point_count == cloud.size() &&
          lease_audit.certified_node_count == node_count &&
          lease_audit.canonical_coordinate_words_retained &&
          lease_audit.active_morton_point_ids_retained &&
          lease_audit.certified_device_nodes_retained &&
          lease_audit.cuda_device_storage_retained &&
          !lease_audit.host_fake_lifecycle_exercised &&
          !lease_audit.second_host_snapshot_retained &&
          !lease_audit.higher_order_delaunay_mosaic_materialized &&
          !lease_audit.global_cell_or_coface_arena_materialized &&
          !lease_audit.public_status_claimed,
      "the qualification traversal lease is not a certified CUDA authority");
  metrics.traversal_device_capacity_bytes = checked_u64(
      lease_audit.persistent_device_byte_capacity,
      "the traversal device byte capacity does not fit uint64");

  const std::span<const MortonLeafRecord> leaves =
      build.certified_index().leaves();
  require(
      leaves.size() == cloud.size(),
      "the certified LBVH leaf count does not match the cloud");
  std::vector<std::uint64_t> position_by_point_id(cloud.size(), kInvalid);
  for (std::size_t position = 0U; position < leaves.size(); ++position) {
    const PointId point_id = leaves[position].point_id;
    require(
        point_id < cloud.size() &&
            position_by_point_id[static_cast<std::size_t>(point_id)] == kInvalid,
        "the certified Morton leaf order is not a PointId permutation");
    position_by_point_id[static_cast<std::size_t>(point_id)] = position;
  }

  const auto adoption_start = Clock::now();
  Phase15MortonYao48DeviceTiledAdoptedTraversal adopted =
      morsehgp3d::gpu::detail::
          adopt_phase15_morton_yao48_device_tiled_traversal(
              std::move(lease));
  metrics.adoption_ns = nanoseconds(Clock::now() - adoption_start);
  require(
      !lease.ready() && adopted.retained_owner != nullptr &&
          adopted.source_cloud_identity != nullptr &&
          adopted.device_coordinate_bits != nullptr &&
          adopted.device_morton_point_ids != nullptr &&
          adopted.device_nodes != nullptr &&
          adopted.device_node_bounds != nullptr &&
          adopted.point_count == cloud.size() &&
          adopted.certified_node_count == node_count &&
          adopted.retained_node_bound_view_capacity_bytes ==
              checked_product(
                  node_count,
                  sizeof(Phase15MortonYao48DeviceTiledNodeBounds),
                  "the qualification node-bound view extent overflows size_t") &&
          adopted.node_bound_view_allocation_capacity_bytes ==
              checked_size_sum(
                  checked_product(
                      node_count,
                      sizeof(Phase15MortonYao48DeviceTiledNodeBounds),
                      "the qualification node-bound view extent overflows size_t"),
                  2U * sizeof(std::uint64_t),
                  "the qualification node-bound allocation extent overflows size_t") &&
          adopted.resolved_node_bound_count == node_count &&
          adopted.node_bound_view_build_allocation_count == 1U &&
          adopted.node_bound_view_build_kernel_launch_count == 1U &&
          adopted.node_bound_view_build_synchronization_count == 1U &&
          adopted.node_bound_view_validation_device_to_host_byte_count ==
              2U * sizeof(std::uint64_t) &&
          adopted.node_bound_view_extent_authenticated &&
          adopted.node_bound_view_validation_sentinel_authenticated &&
          adopted.node_bound_view_bound_to_snapshot_identity &&
          adopted.node_bound_view_built_once_per_adoption &&
          adopted.node_bound_view_reused_without_tile_allocation &&
          adopted.source_node_extremum_point_ids_retained &&
          adopted.node_bound_view_build_included_in_context_creation &&
          adopted.execution_kind ==
              Phase15MortonYao48DeviceTiledExecutionKind::cuda &&
          adopted.canonical_coordinate_words_retained &&
          adopted.active_morton_point_ids_retained &&
          adopted.certified_device_nodes_retained &&
          adopted.cuda_device_storage_retained &&
          !adopted.host_fake_lifecycle_exercised,
      "the Phase 15 qualification could not authenticate traversal adoption");
  metrics.retained_node_bound_view_capacity_bytes = checked_u64(
      adopted.retained_node_bound_view_capacity_bytes,
      "the qualification retained node-bound view extent does not fit uint64");
  metrics.node_bound_view_allocation_capacity_bytes = checked_u64(
      adopted.node_bound_view_allocation_capacity_bytes,
      "the qualification node-bound allocation extent does not fit uint64");
  metrics.resolved_node_bound_count = checked_u64(
      adopted.resolved_node_bound_count,
      "the qualification resolved node-bound count does not fit uint64");
  metrics.node_bound_view_build_allocation_count = checked_u64(
      adopted.node_bound_view_build_allocation_count,
      "the qualification node-bound allocation count does not fit uint64");
  metrics.node_bound_view_build_kernel_launch_count = checked_u64(
      adopted.node_bound_view_build_kernel_launch_count,
      "the qualification node-bound kernel count does not fit uint64");
  metrics.node_bound_view_build_synchronization_count = checked_u64(
      adopted.node_bound_view_build_synchronization_count,
      "the qualification node-bound synchronization count does not fit uint64");
  metrics.node_bound_view_validation_device_to_host_byte_count = checked_u64(
      adopted.node_bound_view_validation_device_to_host_byte_count,
      "the qualification node-bound validation copy does not fit uint64");
  metrics.node_bound_view_extent_authenticated =
      adopted.node_bound_view_extent_authenticated;
  metrics.node_bound_view_validation_sentinel_authenticated =
      adopted.node_bound_view_validation_sentinel_authenticated;
  metrics.node_bound_view_bound_to_snapshot_identity =
      adopted.node_bound_view_bound_to_snapshot_identity;
  metrics.node_bound_view_built_once_per_adoption =
      adopted.node_bound_view_built_once_per_adoption;
  metrics.node_bound_view_reused_without_tile_allocation =
      adopted.node_bound_view_reused_without_tile_allocation;
  metrics.source_node_extremum_point_ids_retained =
      adopted.source_node_extremum_point_ids_retained;
  metrics.node_bound_view_build_included_in_context_creation =
      adopted.node_bound_view_build_included_in_context_creation;
  metrics.node_bound_view_build_included_in_traversal_adoption_ns = true;

  CudaDeviceGuard device_guard{adopted.cuda_device};
  std::size_t free_bytes = 0U;
  std::size_t total_bytes = 0U;
  CudaDeviceGuard::check(
      cudaMemGetInfo(&free_bytes, &total_bytes),
      "cudaMemGetInfo before Phase 15 qualification");
  metrics.device_total_bytes = checked_u64(
      total_bytes, "the CUDA total byte count does not fit uint64");
  metrics.minimum_device_free_bytes = checked_u64(
      free_bytes, "the CUDA free byte count does not fit uint64");

  const auto node_copy_start = Clock::now();
  std::vector<MortonLbvhSnapshotNode> nodes(node_count);
  const std::size_t node_bytes = checked_product(
      node_count,
      sizeof(MortonLbvhSnapshotNode),
      "the qualification node-copy bytes overflow size_t");
  CudaDeviceGuard::check(
      cudaMemcpy(
          nodes.data(),
          adopted.device_nodes,
          node_bytes,
          cudaMemcpyDeviceToHost),
      "cudaMemcpy qualification traversal nodes D2H");
  metrics.node_copy_ns = nanoseconds(Clock::now() - node_copy_start);
  metrics.qualification_device_to_host_bytes += checked_u64(
      node_bytes, "the qualification node-copy count does not fit uint64");
  validate_device_nodes(nodes, cloud.size());

  std::vector<std::uint8_t> pair_classification;
  if (cloud.size() <= 257U) {
    pair_classification.resize(
        static_cast<std::size_t>(metrics.unordered_pair_universe_count), 0U);
  }

  Phase15MortonYao48DeviceTiledPairFrontierContextState launcher_state;
  for (std::size_t anchor_begin = 0U;
       anchor_begin < cloud.size();
       anchor_begin += options.anchor_tile_capacity) {
    const std::size_t anchor_count = std::min(
        options.anchor_tile_capacity, cloud.size() - anchor_begin);
    ++metrics.tile_count;
    std::vector<AnchorPartition> partitions(anchor_count);
    std::vector<QualificationAnchorProgress> progress(anchor_count);
    std::uint64_t tile_epoch = 0U;
    std::uint64_t chunk_sequence = 1U;
    bool tile_complete = false;
    while (!tile_complete) {
      Phase15MortonYao48DeviceTiledRequest request;
      request.source_snapshot_epoch = adopted.source_snapshot_epoch;
      request.output_buffer_epoch = launcher_state.advance_epoch();
      if (tile_epoch == 0U) {
        tile_epoch = request.output_buffer_epoch;
      }
      request.tile_epoch = tile_epoch;
      request.chunk_sequence = chunk_sequence;
      request.point_count = cloud.size();
      request.certified_node_count = node_count;
      request.anchor_begin = anchor_begin;
      request.anchor_count = anchor_count;
      request.maximum_closed_rank = maximum_closed_rank;
      request.prune_semantics = prune_semantics;
      request.required_witness_count = metrics.required_witness_count;
      request.node_visit_capacity_per_anchor =
          morsehgp3d::gpu::
              morton_yao48_device_tiled_pair_frontier_node_visits_per_anchor;
      request.candidate_capacity_per_anchor =
          morsehgp3d::gpu::
              morton_yao48_device_tiled_pair_frontier_candidates_per_anchor;
      request.prune_region_capacity_per_anchor =
          morsehgp3d::gpu::
              morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor;
      request.witness_bank_count_per_anchor =
          morsehgp3d::gpu::
              morton_yao48_device_tiled_pair_frontier_witness_bank_count;
      request.witness_slot_count_per_bank = request.required_witness_count;
      request.resume_same_tile = chunk_sequence > 1U;

      const auto launch_start = Clock::now();
      Phase15MortonYao48DeviceTiledBatch batch =
          morsehgp3d::gpu::detail::
              build_phase15_morton_yao48_device_tiled_pair_frontier_on_device(
                  launcher_state, adopted, request);
      metrics.launcher_ns += nanoseconds(Clock::now() - launch_start);
      ++metrics.chunk_count;
      if (request.resume_same_tile) {
        ++metrics.resumed_chunk_count;
      } else if (batch.fresh_tile_device_arena_reused) {
        ++metrics.fresh_tile_device_arena_reuse_count;
      } else {
        require(
            batch.fresh_tile_device_arena_allocated,
            "a fresh qualification tile neither allocated nor reused its "
            "device arena");
        ++metrics.fresh_tile_device_arena_allocation_count;
      }
      validate_batch_contract(batch, request, adopted.cuda_device);

      std::size_t tile_free_bytes = 0U;
      std::size_t tile_total_bytes = 0U;
      CudaDeviceGuard::check(
          cudaMemGetInfo(&tile_free_bytes, &tile_total_bytes),
          "cudaMemGetInfo during Phase 15 qualification");
      require(
          tile_total_bytes == total_bytes,
          "the CUDA total memory changed during qualification");
      metrics.minimum_device_free_bytes = std::min(
          metrics.minimum_device_free_bytes,
          checked_u64(
              tile_free_bytes,
              "the CUDA tile free byte count does not fit uint64"));
      const std::uint64_t tile_output_bytes = checked_u64(
          batch.physical_device_arena_capacity_bytes,
          "the complete tile arena bytes do not fit uint64");
      metrics.peak_tile_output_device_capacity_bytes = std::max(
          metrics.peak_tile_output_device_capacity_bytes, tile_output_bytes);

      const QualificationControlSummary control_summary =
          validate_qualification_controls(
              batch, request, progress, metrics);
      require(
          !control_summary.any_fatal,
          "the standard qualification encountered a terminal anchor failure");

      const auto recertification_start = Clock::now();
      const std::uint64_t copy_ns_before =
          metrics.qualification_output_copy_ns;
      const std::uint64_t candidate_mass_before = metrics.candidate_pair_mass;
      for (std::size_t slot = 0U; slot < anchor_count; ++slot) {
        const auto& control = batch.host_anchor_controls[slot];
        const std::size_t segment_begin = checked_product(
            slot,
            request.candidate_capacity_per_anchor,
            "a candidate segment offset overflows size_t");
        const std::size_t authorized_count =
            static_cast<std::size_t>(control.candidate_count);
        if (authorized_count == 0U) {
          continue;
        }
        const std::vector<Phase15MortonYao48DeviceTiledCandidateRecord>
            candidates = copy_device_records(
                batch.device_candidate_records + segment_begin,
                authorized_count,
                metrics);
        std::size_t validated_in_anchor = 0U;
        for (const auto& record : candidates) {
          const std::uint64_t anchor_position =
              static_cast<std::uint64_t>(anchor_begin + slot);
          // Deterministic first-N-per-anchor sampling of the host
          // revalidation; the skipped records are still copied, counted and
          // partitioned, and the run is fail-closed reported as sampled.
          if (!options.record_validation_sample_limit.has_value() ||
              validated_in_anchor <
                  *options.record_validation_sample_limit) {
            validate_candidate_record(
                record, anchor_position, leaves, scaled_points, metrics);
            ++metrics.validated_candidate_record_count;
            ++validated_in_anchor;
          } else {
            ++metrics.candidate_record_count;
            metrics.candidate_pair_mass += UINT64_C(1);
            hash_candidate(metrics.output_digest, record);
            ++metrics.skipped_record_validation_count;
          }
          partitions[slot].candidate_positions.push_back(
              record.partner_morton_position);
          if (!pair_classification.empty()) {
            const std::size_t pair_index = static_cast<std::size_t>(
                triangular_offset(
                    anchor_position, record.partner_morton_position));
            require(
                pair_classification[pair_index] == 0U,
                "the bounded qualification emitted a duplicate pair");
            pair_classification[pair_index] = 1U;
          }
        }
      }
      require(
          metrics.candidate_pair_mass - candidate_mass_before ==
              control_summary.candidate_count,
          "the qualification candidate prefix count disagrees with controls");
      const std::size_t prune_count_before = metrics.prune_region_count;
      const std::uint64_t pruned_mass_before =
          metrics.certified_pruned_pair_mass;
      for (std::size_t slot = 0U; slot < anchor_count; ++slot) {
        const auto& control = batch.host_anchor_controls[slot];
        const std::size_t segment_begin = checked_product(
            slot,
            request.prune_region_capacity_per_anchor,
            "a prune segment offset overflows size_t");
        const std::size_t authorized_count =
            static_cast<std::size_t>(control.prune_region_count);
        if (authorized_count == 0U) {
          continue;
        }
        const std::vector<Phase15MortonYao48DeviceTiledPruneRegionRecord>
            prunes = copy_device_records(
                batch.device_prune_regions + segment_begin,
                authorized_count,
                metrics);
        std::size_t validated_prunes_in_anchor = 0U;
        for (const auto& record : prunes) {
          const std::uint64_t anchor_position =
              static_cast<std::uint64_t>(anchor_begin + slot);
          if (!options.record_validation_sample_limit.has_value() ||
              validated_prunes_in_anchor <
                  *options.record_validation_sample_limit) {
            validate_prune_record(
                record,
                anchor_position,
                maximum_closed_rank,
                prune_semantics,
                metrics.required_witness_count,
                nodes,
                leaves,
                scaled_points,
                position_by_point_id,
                options,
                metrics);
            ++metrics.validated_prune_region_record_count;
            ++validated_prunes_in_anchor;
          } else {
            // Skipped host revalidation keeps the structural bound check,
            // the exact counters and the record digest so a sampled run
            // reproduces the full run's output digest bit for bit.
            require(
                static_cast<std::size_t>(record.node_index) < nodes.size(),
                "a sampled prune record references an out-of-range node");
            const MortonLbvhSnapshotNode& sampled_node =
                nodes[static_cast<std::size_t>(record.node_index)];
            ++metrics.prune_region_count;
            metrics.certified_pruned_pair_mass +=
                sampled_node.leaf_end - sampled_node.leaf_begin;
            hash_prune(metrics.output_digest, record);
            ++metrics.skipped_record_validation_count;
          }
          const MortonLbvhSnapshotNode& node =
              nodes[static_cast<std::size_t>(record.node_index)];
          partitions[slot].prune_intervals.emplace_back(
              node.leaf_begin, node.leaf_end);
          if (!pair_classification.empty()) {
            for (std::uint64_t partner_position = node.leaf_begin;
                 partner_position < node.leaf_end;
                 ++partner_position) {
              const std::size_t pair_index = static_cast<std::size_t>(
                  triangular_offset(anchor_position, partner_position));
              require(
                  pair_classification[pair_index] == 0U,
                  "bounded candidate/prune regions overlap");
              pair_classification[pair_index] = 2U;
            }
          }
        }
      }
      require(
          metrics.prune_region_count - prune_count_before ==
                  control_summary.prune_region_count &&
              metrics.certified_pruned_pair_mass - pruned_mass_before ==
                  control_summary.certified_pruned_pair_mass,
          "the qualification prune prefixes disagree with controls");
      if (control_summary.all_complete) {
        for (std::size_t slot = 0U; slot < anchor_count; ++slot) {
          require(
              partitions[slot].candidate_positions.size() ==
                      progress[slot].candidate_count &&
                  partitions[slot].prune_intervals.size() ==
                      progress[slot].prune_region_count,
              "a completed anchor cumulative count disagrees with its prefixes");
          if (!options.record_validation_sample_limit.has_value()) {
            validate_anchor_partition(
                partitions[slot],
                static_cast<std::uint64_t>(anchor_begin + slot));
          }
        }
        if (!options.record_validation_sample_limit.has_value()) {
          const std::vector<Phase15MortonYao48DeviceTiledWitnessBankSlot>
              banks = copy_device_records(
                  batch.device_witness_bank_slots,
                  batch.physical_witness_bank_slot_capacity,
                  metrics);
          const std::size_t slots_per_anchor = checked_product(
              request.witness_bank_count_per_anchor,
              request.witness_slot_count_per_bank,
              "the bank slots per anchor overflow size_t");
          for (std::size_t slot = 0U; slot < anchor_count; ++slot) {
            std::vector<PointId> retained_ids;
            retained_ids.reserve(slots_per_anchor);
            for (std::size_t cone = 0U;
                 cone < request.witness_bank_count_per_anchor;
                 ++cone) {
              bool empty_seen = false;
              for (std::size_t bank_slot = 0U;
                   bank_slot < request.witness_slot_count_per_bank;
                   ++bank_slot) {
                const std::size_t index =
                    slot * slots_per_anchor +
                    cone * request.witness_slot_count_per_bank + bank_slot;
                const auto& retained = banks[index];
                if (invalid_bank_slot(retained)) {
                  empty_seen = true;
                  continue;
                }
                require(
                    !empty_seen,
                    "a witness bank contains a nonempty slot after a sentinel");
                validate_bank_slot(
                    retained,
                    static_cast<std::uint64_t>(anchor_begin + slot),
                    cone,
                    leaves,
                    scaled_points,
                    position_by_point_id,
                    partitions[slot].candidate_positions,
                    metrics);
                retained_ids.push_back(retained.witness_point_id);
              }
            }
            std::sort(retained_ids.begin(), retained_ids.end());
            require(
                std::adjacent_find(retained_ids.begin(), retained_ids.end()) ==
                    retained_ids.end(),
                "an anchor retained the same witness in multiple bank slots");
          }
        }
      }
      const std::uint64_t tile_recertification_ns =
          nanoseconds(Clock::now() - recertification_start);
      const std::uint64_t tile_copy_ns =
          metrics.qualification_output_copy_ns - copy_ns_before;
      require(
          tile_recertification_ns >= tile_copy_ns,
          "qualification copy timing exceeds enclosing recertification timing");
      metrics.cpu_recertification_ns +=
          tile_recertification_ns - tile_copy_ns;
      batch.retained_output_owner.reset();
      tile_complete = control_summary.all_complete;
      if (!tile_complete) {
        require(
            control_summary.any_chunk_ready,
            "an incomplete qualification tile has no resumable chunk");
        if (chunk_sequence == std::numeric_limits<std::uint64_t>::max()) {
          throw std::overflow_error(
              "the qualification chunk sequence overflowed uint64_t");
        }
        ++chunk_sequence;
      }
    }
  }

  require(
      metrics.candidate_pair_mass <= metrics.unordered_pair_universe_count &&
          metrics.certified_pruned_pair_mass <=
              metrics.unordered_pair_universe_count -
                  metrics.candidate_pair_mass,
      "the qualified output mass exceeds the unordered-pair universe");
  metrics.unresolved_pair_mass =
      metrics.unordered_pair_universe_count - metrics.candidate_pair_mass -
      metrics.certified_pruned_pair_mass;
  require(
      metrics.candidate_pair_mass + metrics.certified_pruned_pair_mass +
              metrics.unresolved_pair_mass ==
              metrics.unordered_pair_universe_count &&
          metrics.censored_anchor_count == 0U &&
          metrics.complete_anchor_count == cloud.size() &&
          metrics.unresolved_pair_mass == 0U,
      "the global candidate/pruned/unresolved mass identity does not close");
  metrics.record_validation_sampled =
      options.record_validation_sample_limit.has_value();
  metrics.every_record_validated =
      !metrics.record_validation_sampled &&
      metrics.skipped_record_validation_count == 0U;
  // A sampled run keeps the exact mass identities and the output digest but
  // can never claim the full coverage-partition audit.
  metrics.coverage_partition_complete = !metrics.record_validation_sampled;
  if (cloud.size() <= 257U) {
    bounded_bruteforce_replay(
        leaves,
        scaled_points,
        maximum_closed_rank,
        prune_semantics,
        pair_classification,
        options.family == "adversarial_mixed_dyadic" ||
            options.family == "shell_permutation_dyadic" ||
            options.family == "q3_boundary_shell",
        metrics);
  }
  if (point_id_pair_classification != nullptr) {
    require(
        !pair_classification.empty(),
        "the requested PointId pair partition is unavailable");
    point_id_pair_classification->assign(pair_classification.size(), 0U);
    for (std::uint64_t anchor_position = 1U;
         anchor_position < leaves.size();
         ++anchor_position) {
      const PointId anchor_id =
          leaves[static_cast<std::size_t>(anchor_position)].point_id;
      for (std::uint64_t partner_position = 0U;
           partner_position < anchor_position;
           ++partner_position) {
        const PointId partner_id =
            leaves[static_cast<std::size_t>(partner_position)].point_id;
        const std::uint64_t point_id_anchor =
            std::max<std::uint64_t>(anchor_id, partner_id);
        const std::uint64_t point_id_partner =
            std::min<std::uint64_t>(anchor_id, partner_id);
        const std::size_t source_index = static_cast<std::size_t>(
            triangular_offset(anchor_position, partner_position));
        const std::size_t target_index = static_cast<std::size_t>(
            triangular_offset(point_id_anchor, point_id_partner));
        require(
            (*point_id_pair_classification)[target_index] == 0U,
            "the PointId pair partition contains a duplicate");
        (*point_id_pair_classification)[target_index] =
            pair_classification[source_index];
      }
    }
  }
  metrics.every_prune_fully_recertified =
      metrics.fully_recertified_prune_count == metrics.prune_region_count;
  require(
      metrics.fresh_tile_device_arena_allocation_count != 0U &&
          metrics.fresh_tile_device_arena_allocation_count +
                  metrics.fresh_tile_device_arena_reuse_count ==
              metrics.tile_count,
      "the qualification fresh-arena lifecycle does not close its tile "
      "count");
  metrics.component_contract_validated = true;
  return metrics;
}

void require_warmup_measurement_equivalence(
    const RankMetrics& warmup,
    const RankMetrics& measured) {
  require(
      warmup.maximum_closed_rank == measured.maximum_closed_rank &&
          warmup.prune_semantics == measured.prune_semantics &&
          warmup.required_witness_count == measured.required_witness_count &&
          warmup.tile_count == measured.tile_count &&
          warmup.chunk_count == measured.chunk_count &&
          warmup.resumed_chunk_count == measured.resumed_chunk_count &&
          warmup.fresh_tile_device_arena_allocation_count ==
              measured.fresh_tile_device_arena_allocation_count &&
          warmup.fresh_tile_device_arena_reuse_count ==
              measured.fresh_tile_device_arena_reuse_count &&
          warmup.candidate_yield_anchor_count ==
              measured.candidate_yield_anchor_count &&
          warmup.prune_yield_anchor_count ==
              measured.prune_yield_anchor_count &&
          warmup.complete_anchor_count == measured.complete_anchor_count &&
          warmup.censored_anchor_count == measured.censored_anchor_count &&
          warmup.candidate_record_count == measured.candidate_record_count &&
          warmup.prune_region_count == measured.prune_region_count &&
          warmup.bank_entry_count == measured.bank_entry_count &&
          warmup.ambiguous_candidate_count ==
              measured.ambiguous_candidate_count &&
          warmup.unbanked_candidate_count ==
              measured.unbanked_candidate_count &&
          warmup.fully_recertified_prune_count ==
              measured.fully_recertified_prune_count &&
          warmup.sampled_recertified_prune_count ==
              measured.sampled_recertified_prune_count &&
          warmup.candidate_pair_mass == measured.candidate_pair_mass &&
          warmup.certified_pruned_pair_mass ==
              measured.certified_pruned_pair_mass &&
          warmup.unresolved_pair_mass == measured.unresolved_pair_mass &&
          warmup.unordered_pair_universe_count ==
              measured.unordered_pair_universe_count &&
          warmup.physical_node_visit_count ==
              measured.physical_node_visit_count &&
          warmup.prune_witness_target_check_count ==
              measured.prune_witness_target_check_count &&
          warmup.bounded_exact_pair_count ==
              measured.bounded_exact_pair_count &&
          warmup.bounded_admitted_pair_count ==
              measured.bounded_admitted_pair_count &&
          warmup.bounded_candidate_admitted_pair_count ==
              measured.bounded_candidate_admitted_pair_count &&
          warmup.bounded_candidate_rejected_pair_count ==
              measured.bounded_candidate_rejected_pair_count &&
          warmup.bounded_non_support_shell_equality_count ==
              measured.bounded_non_support_shell_equality_count &&
          warmup.output_digest == measured.output_digest &&
          warmup.qualification_device_to_host_bytes ==
              measured.qualification_device_to_host_bytes &&
          warmup.traversal_device_capacity_bytes ==
              measured.traversal_device_capacity_bytes &&
          warmup.retained_node_bound_view_capacity_bytes ==
              measured.retained_node_bound_view_capacity_bytes &&
          warmup.node_bound_view_allocation_capacity_bytes ==
              measured.node_bound_view_allocation_capacity_bytes &&
          warmup.resolved_node_bound_count ==
              measured.resolved_node_bound_count &&
          warmup.node_bound_view_build_allocation_count ==
              measured.node_bound_view_build_allocation_count &&
          warmup.node_bound_view_build_kernel_launch_count ==
              measured.node_bound_view_build_kernel_launch_count &&
          warmup.node_bound_view_build_synchronization_count ==
              measured.node_bound_view_build_synchronization_count &&
          warmup.node_bound_view_validation_device_to_host_byte_count ==
              measured.node_bound_view_validation_device_to_host_byte_count &&
          warmup.peak_tile_output_device_capacity_bytes ==
              measured.peak_tile_output_device_capacity_bytes &&
          warmup.bounded_bruteforce_performed ==
              measured.bounded_bruteforce_performed &&
          warmup.every_prune_fully_recertified ==
              measured.every_prune_fully_recertified &&
          warmup.coverage_partition_complete ==
              measured.coverage_partition_complete &&
          warmup.q3_exact_diametral_pair_support_gabriel_negative_only ==
              measured.q3_exact_diametral_pair_support_gabriel_negative_only &&
          warmup.gamma2_silent_handoff_required ==
              measured.gamma2_silent_handoff_required &&
          warmup.gamma2_prune_or_discard_authorized ==
              measured.gamma2_prune_or_discard_authorized &&
          warmup.component_contract_validated ==
              measured.component_contract_validated &&
          warmup.node_bound_view_extent_authenticated ==
              measured.node_bound_view_extent_authenticated &&
          warmup.node_bound_view_validation_sentinel_authenticated ==
              measured.node_bound_view_validation_sentinel_authenticated &&
          warmup.node_bound_view_bound_to_snapshot_identity ==
              measured.node_bound_view_bound_to_snapshot_identity &&
          warmup.node_bound_view_built_once_per_adoption ==
              measured.node_bound_view_built_once_per_adoption &&
          warmup.node_bound_view_reused_without_tile_allocation ==
              measured.node_bound_view_reused_without_tile_allocation &&
          warmup.source_node_extremum_point_ids_retained ==
              measured.source_node_extremum_point_ids_retained &&
          warmup.node_bound_view_build_included_in_context_creation ==
              measured.node_bound_view_build_included_in_context_creation &&
          warmup.node_bound_view_build_included_in_traversal_adoption_ns ==
              measured.node_bound_view_build_included_in_traversal_adoption_ns,
      "the warmup and measured component passes changed their exact "
      "partition, digest, closure, or deterministic structural counters");
}

struct StrictInteriorThresholdCaseMetrics {
  StrictInteriorThresholdFixtureCloud fixture_cloud{
      StrictInteriorThresholdFixtureCloud::boundary_shell};
  std::size_t maximum_closed_rank_metadata{};
  std::size_t anchor_tile_capacity{};
  std::uint64_t cloud_digest{};
  bool target_ax_pruned{false};
  RankMetrics rank_metrics;
  std::vector<std::uint8_t> point_id_pair_classification;
};

struct StrictInteriorThresholdQualificationMetrics {
  std::vector<StrictInteriorThresholdCaseMetrics> strict_cases;
  StrictInteriorThresholdCaseMetrics closed_boundary_control;
  std::uint64_t total_ns{};
  bool boundary_shell_preserved{false};
  bool strict_interior_target_pruned{false};
  bool maximum_closed_rank_metadata_invariance_qualified{false};
  bool tile_capacity_invariance_qualified{false};
  bool exact_all_pair_oracle_replayed{false};
  bool q3_exact_diametral_pair_support_gabriel_negative_only{false};
  bool gamma2_silent_handoff_required{false};
  bool gamma2_prune_or_discard_authorized{false};
  bool closed_boundary_control_pruned{false};
  bool success{false};
};

[[nodiscard]] PointId find_fixture_point_id(
    std::span<const ScaledPoint> scaled_points,
    const std::array<std::int64_t, 3U>& coordinate) {
  std::optional<PointId> found;
  for (PointId point_id = 0U;
       point_id < static_cast<PointId>(scaled_points.size());
       ++point_id) {
    if (scaled_points[static_cast<std::size_t>(point_id)].coordinate ==
        coordinate) {
      require(
          !found.has_value(),
          "the strict-interior fixture coordinate is duplicated");
      found = point_id;
    }
  }
  require(
      found.has_value(),
      "the strict-interior fixture coordinate is absent");
  return *found;
}

[[nodiscard]] std::size_t strict_interior_pair_count(
    std::span<const ScaledPoint> scaled_points,
    PointId support_u,
    PointId support_v) {
  std::size_t count = 0U;
  for (PointId witness = 0U;
       witness < static_cast<PointId>(scaled_points.size());
       ++witness) {
    if (exact_phi_sign(
            scaled_points[static_cast<std::size_t>(witness)],
            scaled_points[static_cast<std::size_t>(support_u)],
            scaled_points[static_cast<std::size_t>(support_v)]) < 0) {
      ++count;
    }
  }
  return count;
}

[[nodiscard]] std::size_t closed_rank(
    std::span<const ScaledPoint> scaled_points,
    PointId support_u,
    PointId support_v) {
  std::size_t count = 0U;
  for (PointId witness = 0U;
       witness < static_cast<PointId>(scaled_points.size());
       ++witness) {
    if (exact_phi_sign(
            scaled_points[static_cast<std::size_t>(witness)],
            scaled_points[static_cast<std::size_t>(support_u)],
            scaled_points[static_cast<std::size_t>(support_v)]) <= 0) {
      ++count;
    }
  }
  return count;
}

[[nodiscard]] StrictInteriorThresholdCaseMetrics
run_strict_interior_threshold_case(
    const Options& options,
    StrictInteriorThresholdFixtureCloud fixture_cloud,
    std::size_t maximum_closed_rank_metadata,
    std::size_t anchor_tile_capacity,
    MortonYao48DeviceTiledPairFrontierPruneSemantics prune_semantics) {
  GeneratedCloud generated =
      generate_strict_interior_threshold_fixture(fixture_cloud);
  CanonicalPointCloud cloud =
      CanonicalPointCloud::rejecting_duplicates(generated.points);
  std::vector<ScaledPoint> scaled_points =
      canonical_scaled_points(cloud, generated.scaled_points);
  require(
      cloud.size() == 4U && scaled_points.size() == 4U,
      "the strict-interior fixture canonicalization changed cardinality");

  constexpr std::int64_t unit = kCoordinateDenominator;
  const PointId point_a = find_fixture_point_id(
      scaled_points,
      {-INT64_C(5) * unit,
       -INT64_C(5) * unit,
       -INT64_C(5) * unit});
  const PointId point_x = find_fixture_point_id(
      scaled_points,
      {INT64_C(5) * unit,
       INT64_C(5) * unit,
       INT64_C(5) * unit});
  const std::size_t ax_strict_count =
      strict_interior_pair_count(scaled_points, point_a, point_x);
  const std::size_t ax_closed_rank =
      closed_rank(scaled_points, point_a, point_x);
  require(
      fixture_cloud ==
              StrictInteriorThresholdFixtureCloud::boundary_shell
          ? ax_strict_count == 0U && ax_closed_rank == 4U
          : ax_strict_count == 2U && ax_closed_rank == 4U,
      "the A/X discriminant does not have its exact shell/interior geometry");

  std::size_t strict_prune_eligible_pair_count = 0U;
  std::size_t closed_rank3_prune_eligible_pair_count = 0U;
  for (PointId upper = 1U;
       upper < static_cast<PointId>(scaled_points.size());
       ++upper) {
    for (PointId lower = 0U; lower < upper; ++lower) {
      const bool strict_eligible =
          strict_interior_pair_count(scaled_points, upper, lower) >= 2U;
      const bool closed_eligible =
          closed_rank(scaled_points, upper, lower) > 3U;
      strict_prune_eligible_pair_count += strict_eligible ? 1U : 0U;
      closed_rank3_prune_eligible_pair_count +=
          closed_eligible ? 1U : 0U;
      if (strict_eligible || closed_eligible) {
        require(
            upper == std::max(point_a, point_x) &&
                lower == std::min(point_a, point_x),
            "the discriminant fixture has an unexpected prune-eligible pair");
      }
    }
  }
  require(
      strict_prune_eligible_pair_count ==
              (fixture_cloud ==
                       StrictInteriorThresholdFixtureCloud::boundary_shell
                   ? 0U
                   : 1U) &&
          closed_rank3_prune_eligible_pair_count == 1U,
      "the discriminant fixture does not isolate A/X");

  Options case_options = options;
  case_options.anchor_tile_capacity = anchor_tile_capacity;
  case_options.family =
      fixture_cloud ==
              StrictInteriorThresholdFixtureCloud::boundary_shell
          ? "q3_boundary_shell"
          : "q3_strict_interior";
  StrictInteriorThresholdCaseMetrics result;
  result.fixture_cloud = fixture_cloud;
  result.maximum_closed_rank_metadata = maximum_closed_rank_metadata;
  result.anchor_tile_capacity = anchor_tile_capacity;
  result.cloud_digest = scaled_cloud_digest(scaled_points);
  result.rank_metrics = qualify_rank(
      case_options,
      cloud,
      scaled_points,
      maximum_closed_rank_metadata,
      prune_semantics,
      &result.point_id_pair_classification);
  const std::size_t ax_pair_index = static_cast<std::size_t>(
      triangular_offset(
          std::max<std::uint64_t>(point_a, point_x),
          std::min<std::uint64_t>(point_a, point_x)));
  require(
      ax_pair_index < result.point_id_pair_classification.size(),
      "the A/X pair index lies outside the qualified partition");
  result.target_ax_pruned =
      result.point_id_pair_classification[ax_pair_index] == 2U;

  const bool strict_semantics =
      prune_semantics ==
      MortonYao48DeviceTiledPairFrontierPruneSemantics::
          strict_interior_threshold;
  const std::uint64_t expected_pruned_mass =
      strict_semantics
          ? (fixture_cloud ==
                     StrictInteriorThresholdFixtureCloud::boundary_shell
                 ? UINT64_C(0)
                 : UINT64_C(1))
          : UINT64_C(1);
  require(
      result.rank_metrics.bounded_bruteforce_performed &&
          result.rank_metrics.bounded_exact_pair_count == 6U &&
          result.rank_metrics.unordered_pair_universe_count == 6U &&
          result.rank_metrics.required_witness_count == 2U &&
          result.rank_metrics.prune_semantics == prune_semantics &&
          result.rank_metrics.certified_pruned_pair_mass ==
              expected_pruned_mass &&
          result.rank_metrics.candidate_pair_mass ==
              UINT64_C(6) - expected_pruned_mass &&
          result.rank_metrics.unresolved_pair_mass == 0U &&
          result.rank_metrics.bounded_admitted_pair_count ==
              UINT64_C(6) - expected_pruned_mass &&
          result.rank_metrics.bounded_candidate_admitted_pair_count ==
              UINT64_C(6) - expected_pruned_mass &&
          result.rank_metrics.bounded_candidate_rejected_pair_count == 0U &&
          result.rank_metrics.every_prune_fully_recertified &&
          result.rank_metrics.coverage_partition_complete &&
          result.rank_metrics
                  .q3_exact_diametral_pair_support_gabriel_negative_only ==
              strict_semantics &&
          result.rank_metrics.gamma2_silent_handoff_required ==
              strict_semantics &&
          !result.rank_metrics.gamma2_prune_or_discard_authorized &&
          result.rank_metrics.component_contract_validated &&
          result.target_ax_pruned == (expected_pruned_mass == 1U),
      "the native discriminant run disagrees with its exact all-pair oracle");
  return result;
}

[[nodiscard]] StrictInteriorThresholdQualificationMetrics
run_strict_interior_threshold_qualification(
    const Options& options,
    Clock::time_point total_start) {
  StrictInteriorThresholdQualificationMetrics result;
  constexpr std::array<StrictInteriorThresholdFixtureCloud, 2U> clouds{
      StrictInteriorThresholdFixtureCloud::boundary_shell,
      StrictInteriorThresholdFixtureCloud::strict_interior};
  constexpr std::array<std::size_t, 2U> rank_metadata{2U, 11U};
  constexpr std::array<std::size_t, 2U> tile_capacities{1U, 4U};
  result.strict_cases.reserve(
      clouds.size() * rank_metadata.size() * tile_capacities.size());
  for (const auto fixture_cloud : clouds) {
    for (const std::size_t maximum_closed_rank_metadata : rank_metadata) {
      for (const std::size_t anchor_tile_capacity : tile_capacities) {
        result.strict_cases.push_back(run_strict_interior_threshold_case(
            options,
            fixture_cloud,
            maximum_closed_rank_metadata,
            anchor_tile_capacity,
            MortonYao48DeviceTiledPairFrontierPruneSemantics::
                strict_interior_threshold));
      }
    }
  }
  result.closed_boundary_control = run_strict_interior_threshold_case(
      options,
      StrictInteriorThresholdFixtureCloud::boundary_shell,
      3U,
      4U,
      MortonYao48DeviceTiledPairFrontierPruneSemantics::
          closed_rank_window);

  for (const auto fixture_cloud : clouds) {
    const std::vector<std::uint8_t>* reference = nullptr;
    for (const auto& test_case : result.strict_cases) {
      if (test_case.fixture_cloud != fixture_cloud) {
        continue;
      }
      if (reference == nullptr) {
        reference = &test_case.point_id_pair_classification;
      } else {
        require(
            test_case.point_id_pair_classification == *reference,
            "strict-interior pair partitions differ by rank metadata or "
            "tile capacity");
      }
    }
    require(
        reference != nullptr,
        "the strict-interior matrix omitted a fixture cloud");
  }

  result.boundary_shell_preserved = std::all_of(
      result.strict_cases.begin(),
      result.strict_cases.end(),
      [](const StrictInteriorThresholdCaseMetrics& test_case) {
        return test_case.fixture_cloud !=
                   StrictInteriorThresholdFixtureCloud::boundary_shell ||
               (!test_case.target_ax_pruned &&
                test_case.rank_metrics.certified_pruned_pair_mass == 0U);
      });
  result.strict_interior_target_pruned = std::all_of(
      result.strict_cases.begin(),
      result.strict_cases.end(),
      [](const StrictInteriorThresholdCaseMetrics& test_case) {
        return test_case.fixture_cloud !=
                   StrictInteriorThresholdFixtureCloud::strict_interior ||
               (test_case.target_ax_pruned &&
                test_case.rank_metrics.certified_pruned_pair_mass == 1U);
      });
  result.maximum_closed_rank_metadata_invariance_qualified = true;
  result.tile_capacity_invariance_qualified = true;
  result.exact_all_pair_oracle_replayed = std::all_of(
      result.strict_cases.begin(),
      result.strict_cases.end(),
      [](const StrictInteriorThresholdCaseMetrics& test_case) {
        return test_case.rank_metrics.bounded_bruteforce_performed &&
               test_case.rank_metrics.bounded_exact_pair_count == 6U;
      });
  result.q3_exact_diametral_pair_support_gabriel_negative_only =
      std::all_of(
          result.strict_cases.begin(),
          result.strict_cases.end(),
          [](const StrictInteriorThresholdCaseMetrics& test_case) {
            return test_case.rank_metrics
                .q3_exact_diametral_pair_support_gabriel_negative_only;
          });
  result.gamma2_silent_handoff_required = std::all_of(
      result.strict_cases.begin(),
      result.strict_cases.end(),
      [](const StrictInteriorThresholdCaseMetrics& test_case) {
        return test_case.rank_metrics.gamma2_silent_handoff_required;
      });
  result.gamma2_prune_or_discard_authorized = std::any_of(
      result.strict_cases.begin(),
      result.strict_cases.end(),
      [](const StrictInteriorThresholdCaseMetrics& test_case) {
        return test_case.rank_metrics.gamma2_prune_or_discard_authorized;
      });
  result.closed_boundary_control_pruned =
      result.closed_boundary_control.target_ax_pruned &&
      result.closed_boundary_control.rank_metrics
              .certified_pruned_pair_mass == 1U;
  result.success =
      result.strict_cases.size() == 8U &&
      result.boundary_shell_preserved &&
      result.strict_interior_target_pruned &&
      result.maximum_closed_rank_metadata_invariance_qualified &&
      result.tile_capacity_invariance_qualified &&
      result.exact_all_pair_oracle_replayed &&
      result.q3_exact_diametral_pair_support_gabriel_negative_only &&
      result.gamma2_silent_handoff_required &&
      !result.gamma2_prune_or_discard_authorized &&
      result.closed_boundary_control_pruned;
  result.total_ns = nanoseconds(Clock::now() - total_start);
  return result;
}

[[nodiscard]] std::string json_escape(std::string_view text) {
  std::ostringstream output;
  for (const char raw_character : text) {
    const unsigned char character =
        static_cast<unsigned char>(raw_character);
    switch (character) {
      case '"':
        output << "\\\"";
        break;
      case '\\':
        output << "\\\\";
        break;
      case '\b':
        output << "\\b";
        break;
      case '\f':
        output << "\\f";
        break;
      case '\n':
        output << "\\n";
        break;
      case '\r':
        output << "\\r";
        break;
      case '\t':
        output << "\\t";
        break;
      default:
        if (character < 0x20U) {
          output << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                 << static_cast<unsigned int>(character) << std::dec;
        } else {
          output << static_cast<char>(character);
        }
    }
  }
  return output.str();
}

void print_rank_metrics(const RankMetrics& metrics) {
  std::cout
      << '{'
      << "\"bank_entry_count\":" << metrics.bank_entry_count << ','
      << "\"bounded_admitted_pair_count\":"
      << metrics.bounded_admitted_pair_count << ','
      << "\"bounded_bruteforce_ns\":" << metrics.bounded_bruteforce_ns
      << ','
      << "\"bounded_bruteforce_performed\":"
      << (metrics.bounded_bruteforce_performed ? "true" : "false") << ','
      << "\"bounded_candidate_admitted_pair_count\":"
      << metrics.bounded_candidate_admitted_pair_count << ','
      << "\"bounded_candidate_rejected_pair_count\":"
      << metrics.bounded_candidate_rejected_pair_count << ','
      << "\"bounded_exact_pair_count\":" << metrics.bounded_exact_pair_count
      << ','
      << "\"bounded_non_support_shell_equality_count\":"
      << metrics.bounded_non_support_shell_equality_count << ','
      << "\"build_ns\":" << metrics.build_ns << ','
      << "\"candidate_pair_mass\":" << metrics.candidate_pair_mass << ','
      << "\"candidate_record_count\":" << metrics.candidate_record_count
      << ','
      << "\"candidate_yield_anchor_count\":"
      << metrics.candidate_yield_anchor_count << ','
      << "\"censored_anchor_count\":" << metrics.censored_anchor_count
      << ','
      << "\"certified_pruned_pair_mass\":"
      << metrics.certified_pruned_pair_mass << ','
      << "\"complete_anchor_count\":" << metrics.complete_anchor_count
      << ','
      << "\"component_contract_validated\":"
      << (metrics.component_contract_validated ? "true" : "false") << ','
      << "\"chunk_count\":" << metrics.chunk_count << ','
      << "\"coverage_partition_complete\":"
      << (metrics.coverage_partition_complete ? "true" : "false") << ','
      << "\"cpu_recertification_ns\":" << metrics.cpu_recertification_ns
      << ','
      << "\"device_total_bytes\":" << metrics.device_total_bytes << ','
      << "\"every_prune_fully_recertified\":"
      << (metrics.every_prune_fully_recertified ? "true" : "false") << ','
      << "\"every_record_validated\":"
      << (metrics.every_record_validated ? "true" : "false") << ','
      << "\"record_validation_sampled\":"
      << (metrics.record_validation_sampled ? "true" : "false") << ','
      << "\"validated_candidate_record_count\":"
      << metrics.validated_candidate_record_count << ','
      << "\"validated_prune_region_record_count\":"
      << metrics.validated_prune_region_record_count << ','
      << "\"skipped_record_validation_count\":"
      << metrics.skipped_record_validation_count << ','
      << "\"gamma2_prune_or_discard_authorized\":"
      << (metrics.gamma2_prune_or_discard_authorized ? "true" : "false")
      << ','
      << "\"gamma2_silent_handoff_required\":"
      << (metrics.gamma2_silent_handoff_required ? "true" : "false") << ','
      << "\"fully_recertified_prune_count\":"
      << metrics.fully_recertified_prune_count << ','
      << "\"fresh_tile_device_arena_allocation_count\":"
      << metrics.fresh_tile_device_arena_allocation_count << ','
      << "\"fresh_tile_device_arena_reuse_count\":"
      << metrics.fresh_tile_device_arena_reuse_count << ','
      << "\"launcher_ns\":" << metrics.launcher_ns << ','
      << "\"lease_release_ns\":" << metrics.lease_release_ns << ','
      << "\"maximum_closed_rank\":" << metrics.maximum_closed_rank << ','
      << "\"prune_semantics\":\""
      << prune_semantics_name(metrics.prune_semantics) << "\","
      << "\"required_witness_count\":"
      << metrics.required_witness_count << ','
      << "\"minimum_device_free_bytes\":" << metrics.minimum_device_free_bytes
      << ','
      << "\"node_bound_view_allocation_capacity_bytes\":"
      << metrics.node_bound_view_allocation_capacity_bytes << ','
      << "\"node_bound_view_bound_to_snapshot_identity\":"
      << (metrics.node_bound_view_bound_to_snapshot_identity ? "true" : "false")
      << ','
      << "\"node_bound_view_build_allocation_count\":"
      << metrics.node_bound_view_build_allocation_count << ','
      << "\"node_bound_view_build_included_in_context_creation\":"
      << (metrics.node_bound_view_build_included_in_context_creation
              ? "true"
              : "false")
      << ','
      << "\"node_bound_view_build_included_in_traversal_adoption_ns\":"
      << (metrics.node_bound_view_build_included_in_traversal_adoption_ns
              ? "true"
              : "false")
      << ','
      << "\"node_bound_view_build_kernel_launch_count\":"
      << metrics.node_bound_view_build_kernel_launch_count << ','
      << "\"node_bound_view_build_synchronization_count\":"
      << metrics.node_bound_view_build_synchronization_count << ','
      << "\"node_bound_view_built_once_per_adoption\":"
      << (metrics.node_bound_view_built_once_per_adoption ? "true" : "false")
      << ','
      << "\"node_bound_view_extent_authenticated\":"
      << (metrics.node_bound_view_extent_authenticated ? "true" : "false")
      << ','
      << "\"node_bound_view_reused_without_tile_allocation\":"
      << (metrics.node_bound_view_reused_without_tile_allocation
              ? "true"
              : "false")
      << ','
      << "\"node_bound_view_validation_device_to_host_byte_count\":"
      << metrics.node_bound_view_validation_device_to_host_byte_count << ','
      << "\"node_bound_view_validation_sentinel_authenticated\":"
      << (metrics.node_bound_view_validation_sentinel_authenticated
              ? "true"
              : "false")
      << ','
      << "\"node_copy_ns\":" << metrics.node_copy_ns << ','
      << "\"output_digest_fnv1a\":" << metrics.output_digest << ','
      << "\"peak_tile_output_device_capacity_bytes\":"
      << metrics.peak_tile_output_device_capacity_bytes << ','
      << "\"physical_node_visit_count\":"
      << metrics.physical_node_visit_count << ','
      << "\"q3_exact_diametral_pair_support_gabriel_negative_only\":"
      << (metrics.q3_exact_diametral_pair_support_gabriel_negative_only
              ? "true"
              : "false")
      << ','
      << "\"prune_region_count\":" << metrics.prune_region_count << ','
      << "\"prune_yield_anchor_count\":"
      << metrics.prune_yield_anchor_count << ','
      << "\"prune_witness_target_check_count\":"
      << metrics.prune_witness_target_check_count << ','
      << "\"qualification_device_to_host_bytes\":"
      << metrics.qualification_device_to_host_bytes << ','
      << "\"qualification_output_copy_ns\":"
      << metrics.qualification_output_copy_ns << ','
      << "\"sampled_recertified_prune_count\":"
      << metrics.sampled_recertified_prune_count << ','
      << "\"resumed_chunk_count\":" << metrics.resumed_chunk_count << ','
      << "\"resolved_node_bound_count\":"
      << metrics.resolved_node_bound_count << ','
      << "\"retained_node_bound_view_capacity_bytes\":"
      << metrics.retained_node_bound_view_capacity_bytes << ','
      << "\"source_node_extremum_point_ids_retained\":"
      << (metrics.source_node_extremum_point_ids_retained ? "true" : "false")
      << ','
      << "\"tile_count\":" << metrics.tile_count << ','
      << "\"traversal_adoption_ns\":" << metrics.adoption_ns << ','
      << "\"traversal_device_capacity_bytes\":"
      << metrics.traversal_device_capacity_bytes << ','
      << "\"unbanked_candidate_count\":"
      << metrics.unbanked_candidate_count << ','
      << "\"unordered_pair_universe_count\":"
      << metrics.unordered_pair_universe_count << ','
      << "\"unresolved_pair_mass\":" << metrics.unresolved_pair_mass
      << '}';
}

void print_strict_interior_threshold_case(
    const StrictInteriorThresholdCaseMetrics& test_case) {
  const bool strict_semantics =
      test_case.rank_metrics.prune_semantics ==
      MortonYao48DeviceTiledPairFrontierPruneSemantics::
          strict_interior_threshold;
  const std::uint64_t expected_pruned_mass =
      strict_semantics &&
              test_case.fixture_cloud ==
                  StrictInteriorThresholdFixtureCloud::boundary_shell
          ? UINT64_C(0)
          : UINT64_C(1);
  std::cout
      << "{\"anchor_tile_capacity\":"
      << test_case.anchor_tile_capacity << ','
      << "\"cloud\":\""
      << strict_fixture_cloud_name(test_case.fixture_cloud) << "\","
      << "\"cloud_digest_fnv1a\":" << test_case.cloud_digest << ','
      << "\"expected_candidate_pair_mass\":"
      << UINT64_C(6) - expected_pruned_mass << ','
      << "\"expected_certified_pruned_pair_mass\":"
      << expected_pruned_mass << ','
      << "\"maximum_closed_rank_metadata\":"
      << test_case.maximum_closed_rank_metadata << ','
      << "\"rank_result\":";
  print_rank_metrics(test_case.rank_metrics);
  std::cout
      << ",\"target_ax_negative_covered\":"
      << (test_case.target_ax_pruned ? "true" : "false") << '}';
}

void print_strict_interior_threshold_json(
    const StrictInteriorThresholdQualificationMetrics& metrics) {
  std::cout
      << "{\"backend\":\"" << kBackend << "\","
      << "\"boundary_shell_preserved\":"
      << (metrics.boundary_shell_preserved ? "true" : "false") << ','
      << "\"closed_boundary_control\":";
  print_strict_interior_threshold_case(metrics.closed_boundary_control);
  std::cout
      << ",\"closed_boundary_control_negative_covered\":"
      << (metrics.closed_boundary_control_pruned ? "true" : "false")
      << ','
      << "\"component_only\":true,"
      << "\"deployment_status\":\"" << kDeploymentStatus << "\","
      << "\"exact_all_pair_all_witness_oracle\":"
      << (metrics.exact_all_pair_oracle_replayed ? "true" : "false")
      << ','
      << "\"fixture\":\"" << kStrictInteriorThresholdFixture << "\","
      << "\"git_sha\":\"" << json_escape(kGitSha) << "\","
      << "\"global_cell_or_coface_arena_materialized\":false,"
      << "\"global_pair_matrix_materialized\":false,"
      << "\"gamma2_catalog_published\":false,"
      << "\"gamma2_prune_or_discard\":"
      << (metrics.gamma2_prune_or_discard_authorized ? "true" : "false")
      << ','
      << "\"gamma2_silent_handoff_required\":"
      << (metrics.gamma2_silent_handoff_required ? "true" : "false")
      << ','
      << "\"hierarchy_reduction_performed\":false,"
      << "\"maximum_closed_rank_metadata_invariance_qualified\":"
      << (metrics.maximum_closed_rank_metadata_invariance_qualified
              ? "true"
              : "false")
      << ','
      << "\"mode\":\"" << kStrictInteriorThresholdMode << "\","
      << "\"morse_hgp_hierarchy_claimed\":false,"
      << "\"negative_region_interpretation\":"
         "\"no_q3_gabriel_triangle_with_exact_diametral_pair_minimal_"
         "support_only\","
      << "\"ordinary_delaunay_materialized\":false,"
      << "\"pair_frontier_negative_regions_are_gamma2_discard\":false,"
      << "\"point_count_per_cloud\":4,"
      << "\"profile\":\"" << kProfile << "\","
      << "\"public_status\":\"" << kPublicStatus << "\","
      << "\"q\":3,"
      << "\"q3_exact_diametral_pair_support_gabriel_negative_only\":"
      << (metrics.q3_exact_diametral_pair_support_gabriel_negative_only
              ? "true"
              : "false")
      << ','
      << "\"required_witness_count\":2,"
      << "\"run_count\":9,"
      << "\"schema\":\"" << kStrictInteriorThresholdSchema << "\","
      << "\"scientific_scope\":\""
      << kStrictInteriorThresholdScientificScope << "\","
      << "\"scientific_pair_catalog_published\":false,"
      << "\"strict_cases\":[";
  for (std::size_t index = 0U;
       index < metrics.strict_cases.size();
       ++index) {
    if (index != 0U) {
      std::cout << ',';
    }
    print_strict_interior_threshold_case(metrics.strict_cases[index]);
  }
  std::cout
      << "],\"strict_interior_target_negative_covered\":"
      << (metrics.strict_interior_target_pruned ? "true" : "false")
      << ','
      << "\"success\":" << (metrics.success ? "true" : "false") << ','
      << "\"tile_capacity_invariance_qualified\":"
      << (metrics.tile_capacity_invariance_qualified ? "true" : "false")
      << ','
      << "\"total_ns\":" << metrics.total_ns << "}\n";
}

[[nodiscard]] std::string_view stop_reason_name(
    MortonYao48DeviceTiledPairFrontierStopReason reason) noexcept {
  switch (reason) {
    case MortonYao48DeviceTiledPairFrontierStopReason::none:
      return "none";
    case MortonYao48DeviceTiledPairFrontierStopReason::node_visit_capacity:
      return "node_visit_capacity";
    case MortonYao48DeviceTiledPairFrontierStopReason::fatal_failure:
      return "fatal_failure";
  }
  return "invalid";
}

void print_scaling_smoke_json(
    const Options& options,
    const ScalingSmokeMetrics& metrics) {
  const bool pair_mass_partition_validated =
      metrics.candidate_pair_mass + metrics.certified_pruned_pair_mass +
              metrics.unresolved_pair_mass ==
          metrics.unordered_pair_universe_count;
  const bool complete_pair_mass_validated =
      metrics.coverage_complete && metrics.unresolved_pair_mass == 0U &&
      metrics.candidate_pair_mass + metrics.certified_pruned_pair_mass ==
          metrics.unordered_pair_universe_count;
  std::cout
      << "{\"advance_count\":" << metrics.advance_count << ','
      << "\"anchor_control_device_to_host_byte_count\":"
      << metrics.anchor_control_device_to_host_byte_count << ','
      << "\"anchor_control_device_to_host_count\":"
      << metrics.anchor_control_device_to_host_count << ','
      << "\"anchor_control_record_size_bytes\":"
      << sizeof(Phase15MortonYao48DeviceTiledAnchorControl) << ','
      << "\"anchor_tile_capacity\":" << options.anchor_tile_capacity << ','
      << "\"backend\":\"" << kBackend << "\","
      << "\"backpressure_validated\":"
      << (metrics.backpressure_validated ? "true" : "false") << ','
      << "\"build_ns\":" << metrics.build_ns << ','
      << "\"candidate_device_to_host_count\":0,"
      << "\"candidate_yield_count\":" << metrics.candidate_yield_count
      << ','
      << "\"candidate_record_size_bytes\":"
      << sizeof(Phase15MortonYao48DeviceTiledCandidateRecord) << ','
      << "\"candidate_pair_mass\":" << metrics.candidate_pair_mass << ','
      << "\"candidate_tile_lease_count\":"
      << metrics.candidate_tile_lease_count << ','
      << "\"candidate_tile_release_count\":"
      << metrics.candidate_tile_release_count << ','
      << "\"canonicalization_ns\":" << metrics.canonicalization_ns << ','
      << "\"chunk_ready_advance_count\":"
      << metrics.chunk_ready_advance_count << ','
      << "\"censored\":" << (metrics.censored ? "true" : "false") << ','
      << "\"certified_prune_region_count\":"
      << metrics.certified_prune_region_count << ','
      << "\"certified_pruned_pair_mass\":"
      << metrics.certified_pruned_pair_mass << ','
      << "\"completed_anchor_count\":" << metrics.completed_anchor_count
      << ','
      << "\"component_only\":true,"
      << "\"context_creation_ns\":" << metrics.context_creation_ns << ','
      << "\"node_bound_view_build_included_in_context_creation\":true,"
      << "\"context_release_ns\":" << metrics.context_release_ns << ','
      << "\"coverage_complete\":"
      << (metrics.coverage_complete ? "true" : "false") << ','
      << "\"coverage_success\":"
      << (metrics.coverage_complete ? "true" : "false") << ','
      << "\"dense_pair_fallback_performed\":false,"
      << "\"deployment_status\":\"profile_only\","
      << "\"device_memory_observation_count\":"
      << metrics.device_memory_observation_count << ','
      << "\"device_total_bytes\":" << metrics.device_total_bytes << ','
      << "\"direct_large_scale_guard_passed\":"
      << (options.direct_scale ? "true" : "false") << ','
      << "\"exact_diametral_rank_evaluated\":false,"
      << "\"exactness_claimed\":false,"
      << "\"execution_success\":"
      << (metrics.execution_success ? "true" : "false") << ','
      << "\"family\":\"affine_uniform_binary64\","
      << "\"final_device_free_bytes\":"
      << metrics.final_device_free_bytes << ','
      << "\"fixed_candidate_capacity_per_anchor\":"
      << morsehgp3d::gpu::
             morton_yao48_device_tiled_pair_frontier_candidates_per_anchor
      << ','
      << "\"fixed_node_visit_capacity_per_anchor\":"
      << morsehgp3d::gpu::
             morton_yao48_device_tiled_pair_frontier_node_visits_per_anchor
      << ','
      << "\"fixed_prune_region_capacity_per_anchor\":"
      << morsehgp3d::gpu::
             morton_yao48_device_tiled_pair_frontier_prune_regions_per_anchor
      << ','
      << "\"fixed_witness_bank_count_per_anchor\":"
      << morsehgp3d::gpu::
             morton_yao48_device_tiled_pair_frontier_witness_bank_count
      << ','
      << "\"frontier_ns\":" << metrics.frontier_ns << ','
      << "\"generation_ns\":" << metrics.generation_ns << ','
      << "\"git_sha\":\"" << json_escape(kGitSha) << "\","
      << "\"global_pair_matrix_materialized\":false,"
      << "\"higher_order_structure_materialized\":false,"
      << "\"host_release_ns\":" << metrics.host_release_ns << ','
      << "\"initial_device_free_bytes\":"
      << metrics.initial_device_free_bytes << ','
      << "\"kernel_launch_count\":" << metrics.kernel_launch_count << ','
      << "\"launcher_call_count\":" << metrics.launcher_call_count << ','
      << "\"lease_release_ns\":" << metrics.lease_release_ns << ','
      << "\"maximum_closed_rank\":" << metrics.maximum_closed_rank << ','
      << "\"prune_semantics\":\"closed_rank_window\","
      << "\"required_witness_count\":"
      << metrics.maximum_closed_rank - 1U << ','
      << "\"maximum_traversal_subdivision_count_per_anchor\":"
      << metrics.maximum_traversal_subdivision_count_per_anchor << ','
      << "\"minimum_device_free_bytes\":"
      << metrics.minimum_device_free_bytes << ','
      << "\"mixed_yield_count\":" << metrics.mixed_yield_count << ','
      << "\"node_bound_view_allocation_capacity_bytes\":"
      << metrics.node_bound_view_allocation_capacity_bytes << ','
      << "\"node_bound_view_build_allocation_count\":"
      << metrics.node_bound_view_build_allocation_count << ','
      << "\"node_bound_view_build_kernel_launch_count\":"
      << metrics.node_bound_view_build_kernel_launch_count << ','
      << "\"node_bound_view_build_synchronization_count\":"
      << metrics.node_bound_view_build_synchronization_count << ','
      << "\"node_bound_view_validation_device_to_host_byte_count\":"
      << metrics.node_bound_view_validation_device_to_host_byte_count << ','
      << "\"node_bound_view_validation_sentinel_authenticated\":true,"
      << "\"node_bound_view_extent_authenticated\":true,"
      << "\"node_bound_view_bound_to_snapshot_identity\":true,"
      << "\"node_bound_view_built_once_per_adoption\":true,"
      << "\"node_bound_view_reused_without_tile_allocation\":true,"
      << "\"mode\":\""
      << (options.direct_scale
              ? "device_resident_budgeted_morton_yao48_anchor_tiles_direct_scale"
              : "device_resident_budgeted_morton_yao48_anchor_tiles_scaling_smoke")
      << "\","
      << "\"ordinary_delaunay_materialized\":false,"
      << "\"ordinary_emst_computed\":false,"
      << "\"peak_candidate_device_capacity_bytes\":"
      << metrics.peak_candidate_device_capacity_bytes << ','
      << "\"peak_control_device_capacity_bytes\":"
      << metrics.peak_control_device_capacity_bytes << ','
      << "\"peak_host_rss_bytes\":" << peak_host_rss_bytes() << ','
      << "\"peak_prune_device_capacity_bytes\":"
      << metrics.peak_prune_device_capacity_bytes << ','
      << "\"peak_tile_output_device_capacity_bytes\":"
      << metrics.peak_tile_output_device_capacity_bytes << ','
      << "\"peak_witness_bank_device_capacity_bytes\":"
      << metrics.peak_witness_bank_device_capacity_bytes << ','
      << "\"pair_mass_partition_validated\":"
      << (pair_mass_partition_validated ? "true" : "false") << ','
      << "\"complete_pair_mass_validated\":"
      << (complete_pair_mass_validated ? "true" : "false") << ','
      << "\"point_count\":" << options.point_count << ','
      << "\"resolved_node_bound_count\":"
      << metrics.resolved_node_bound_count << ','
      << "\"retained_node_bound_view_capacity_bytes\":"
      << metrics.retained_node_bound_view_capacity_bytes << ','
      << "\"source_node_extremum_point_ids_retained\":true,"
      << "\"process_restart_resumable\":false,"
      << "\"product_claimed\":false,"
      << "\"profile\":\"" << kProfile << "\","
      << "\"profile_only\":true,"
      << "\"prune_region_record_size_bytes\":"
      << sizeof(Phase15MortonYao48DeviceTiledPruneRegionRecord) << ','
      << "\"prune_yield_count\":" << metrics.prune_yield_count << ','
      << "\"public_status\":\"" << kPublicStatus << "\","
      << "\"qualification_claimed\":false,"
      << "\"raw_candidate_or_prune_view_accessed\":false,"
      << "\"resume_control_device_to_host_byte_count\":"
      << metrics.resume_control_device_to_host_byte_count << ','
      << "\"resume_control_device_to_host_count\":"
      << metrics.resume_control_device_to_host_count << ','
      << "\"resumed_advance_count\":" << metrics.resumed_advance_count
      << ','
      << "\"run_kind\":\""
      << (options.direct_scale ? "direct_scale" : "scaling_smoke")
      << "\","
      << "\"scalability_claimed\":false,"
      << "\"schema\":\""
      << (options.direct_scale ? kDirectScaleSchema : kScalingSmokeSchema)
      << "\","
      << "\"scientific_claimed\":false,"
      << "\"scientific_pair_catalog_published\":false,"
      << "\"seed\":" << options.seed << ','
      << "\"stop_reason\":\"" << stop_reason_name(metrics.stop_reason)
      << "\","
      << "\"synchronization_count\":" << metrics.synchronization_count
      << ','
      << "\"total_ns\":" << metrics.total_ns << ','
      << "\"traversal_device_capacity_bytes\":"
      << metrics.traversal_device_capacity_bytes << ','
      << "\"traversal_subdivision_count\":"
      << metrics.traversal_subdivision_count << ','
      << "\"unordered_pair_universe_count\":"
      << metrics.unordered_pair_universe_count << ','
      << "\"unresolved_pair_mass\":" << metrics.unresolved_pair_mass << ','
      << "\"witness_bank_slot_size_bytes\":"
      << sizeof(Phase15MortonYao48DeviceTiledWitnessBankSlot) << ','
      << "\"warm_e2e_slo_claimed\":false}\n";
}

void print_scaling_failure_json(
    std::string_view message,
    bool direct_scale_requested) {
  std::cout
      << "{\"backend\":\"" << kBackend << "\","
      << "\"component_only\":true,"
      << "\"deployment_status\":\"profile_only\","
      << "\"error\":\"" << json_escape(message) << "\","
      << "\"exactness_claimed\":false,"
      << "\"execution_success\":false,"
      << "\"git_sha\":\"" << json_escape(kGitSha) << "\","
      << "\"mode\":\""
      << (direct_scale_requested
              ? "device_resident_budgeted_morton_yao48_anchor_tiles_direct_scale"
              : "device_resident_budgeted_morton_yao48_anchor_tiles_scaling_smoke")
      << "\","
      << "\"product_claimed\":false,"
      << "\"profile\":\"" << kProfile << "\","
      << "\"profile_only\":true,"
      << "\"public_status\":\"" << kPublicStatus << "\","
      << "\"qualification_claimed\":false,"
      << "\"run_kind\":\""
      << (direct_scale_requested ? "direct_scale" : "scaling_smoke")
      << "\","
      << "\"scalability_claimed\":false,"
      << "\"schema\":\""
      << (direct_scale_requested ? kDirectScaleSchema : kScalingSmokeSchema)
      << "\","
      << "\"scientific_claimed\":false}\n";
}

void print_failure_json(std::string_view message) {
  std::cout
      << "{\"backend\":\"" << kBackend << "\","
      << "\"deployment_status\":\"" << kDeploymentStatus << "\","
      << "\"error\":\"" << json_escape(message) << "\","
      << "\"git_sha\":\"" << json_escape(kGitSha) << "\","
      << "\"mode\":\"" << kMode << "\","
      << "\"profile\":\"" << kProfile << "\","
      << "\"public_status\":\"" << kPublicStatus << "\","
      << "\"schema\":\"" << kSchema << "\","
      << "\"success\":false}\n";
}

void print_strict_interior_threshold_failure_json(
    std::string_view message) {
  std::cout
      << "{\"backend\":\"" << kBackend << "\","
      << "\"component_only\":true,"
      << "\"deployment_status\":\"" << kDeploymentStatus << "\","
      << "\"error\":\"" << json_escape(message) << "\","
      << "\"gamma2_prune_or_discard\":false,"
      << "\"git_sha\":\"" << json_escape(kGitSha) << "\","
      << "\"hierarchy_reduction_performed\":false,"
      << "\"mode\":\"" << kStrictInteriorThresholdMode << "\","
      << "\"profile\":\"" << kProfile << "\","
      << "\"public_status\":\"" << kPublicStatus << "\","
      << "\"schema\":\"" << kStrictInteriorThresholdSchema << "\","
      << "\"scientific_scope\":\""
      << kStrictInteriorThresholdScientificScope << "\","
      << "\"success\":false}\n";
}

}  // namespace

int main(int argc, char** argv) {
  bool scaling_smoke_requested = false;
  bool direct_scale_requested = false;
  bool strict_interior_threshold_requested = false;
  for (int index = 1; index < argc; ++index) {
    scaling_smoke_requested =
        scaling_smoke_requested ||
        std::string_view{argv[index]} == "--scaling-smoke";
    direct_scale_requested =
        direct_scale_requested ||
        std::string_view{argv[index]} == "--direct-scale";
    strict_interior_threshold_requested =
        strict_interior_threshold_requested ||
        std::string_view{argv[index]} == "strict_interior_threshold";
  }
  try {
    const Options options = parse_options(argc, argv);
    const auto total_start = Clock::now();
    if (options.prune_semantics ==
        MortonYao48DeviceTiledPairFrontierPruneSemantics::
            strict_interior_threshold) {
      const StrictInteriorThresholdQualificationMetrics strict_metrics =
          run_strict_interior_threshold_qualification(
              options, total_start);
      print_strict_interior_threshold_json(strict_metrics);
      return strict_metrics.success ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    if (options.scaling_smoke || options.direct_scale) {
      const auto generation_start = Clock::now();
      std::vector<CertifiedPoint3> scaling_points =
          generate_scaling_points(options);
      const std::uint64_t generation_ns =
          nanoseconds(Clock::now() - generation_start);
      const ScalingSmokeMetrics scaling_metrics = run_scaling_smoke(
          options,
          std::move(scaling_points),
          generation_ns,
          total_start);
      print_scaling_smoke_json(options, scaling_metrics);
      return EXIT_SUCCESS;
    }
    const auto generation_start = Clock::now();
    GeneratedCloud generated = generate_cloud(options);
    const std::uint64_t generation_ns =
        nanoseconds(Clock::now() - generation_start);

    const auto canonicalization_start = Clock::now();
    CanonicalPointCloud cloud =
        CanonicalPointCloud::rejecting_duplicates(generated.points);
    std::vector<ScaledPoint> scaled_points =
        canonical_scaled_points(cloud, generated.scaled_points);
    const std::uint64_t cloud_digest = scaled_cloud_digest(scaled_points);
    const std::uint64_t canonicalization_ns =
        nanoseconds(Clock::now() - canonicalization_start);
    generated.points.clear();
    generated.points.shrink_to_fit();
    generated.scaled_points.clear();
    generated.scaled_points.shrink_to_fit();

    const std::vector<std::size_t> ranks = requested_ranks(options);
    std::vector<RankMetrics> results;
    results.reserve(ranks.size());
    const bool warm_component_profile = options.warmup_run_count == 1U;
    std::uint64_t warmup_total_ns = 0U;
    bool warmup_measurement_equivalence_validated = false;
    bool any_censure = false;
    bool all_coverage_complete = true;
    std::uint64_t total_ns = 0U;
    if (warm_component_profile) {
      require(
          ranks.size() == 1U,
          "the warm component profile requires exactly one rank");
      const auto warmup_start = Clock::now();
      const RankMetrics warmup = qualify_rank(
          options,
          cloud,
          scaled_points,
          ranks.front(),
          options.prune_semantics);
      warmup_total_ns = nanoseconds(Clock::now() - warmup_start);

      const auto measured_start = Clock::now();
      results.push_back(
          qualify_rank(
              options,
              cloud,
              scaled_points,
              ranks.front(),
              options.prune_semantics));
      total_ns = nanoseconds(Clock::now() - measured_start);
      require_warmup_measurement_equivalence(warmup, results.back());
      warmup_measurement_equivalence_validated = true;
      any_censure = any_censure || results.back().censored_anchor_count != 0U;
      all_coverage_complete =
          all_coverage_complete && results.back().coverage_partition_complete;
    } else {
      for (const std::size_t rank : ranks) {
        results.push_back(
            qualify_rank(
                options,
                cloud,
                scaled_points,
                rank,
                options.prune_semantics));
        any_censure =
            any_censure || results.back().censored_anchor_count != 0U;
        all_coverage_complete =
            all_coverage_complete &&
            results.back().coverage_partition_complete;
      }
      total_ns = nanoseconds(Clock::now() - total_start);
    }
    const bool coverage_success = !any_censure;
    const bool exit_success = coverage_success;
    const bool rank_triplet_exercised =
        std::find(ranks.begin(), ranks.end(), 2U) != ranks.end() &&
        std::find(ranks.begin(), ranks.end(), 3U) != ranks.end() &&
        std::find(ranks.begin(), ranks.end(), 11U) != ranks.end();
    const bool all_rank_thresholds_exercised =
        ranks == std::vector<std::size_t>{
                     2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U};

    std::cout
        << "{\"anchor_tile_capacity\":" << options.anchor_tile_capacity
        << ','
        << "\"backend\":\"" << kBackend << "\","
        << "\"canonicalization_ns\":" << canonicalization_ns << ','
        << "\"cloud_digest_fnv1a\":" << cloud_digest << ','
        << "\"component_only\":true,"
        << "\"coverage_partition_complete\":"
        << (all_coverage_complete ? "true" : "false") << ','
        << "\"delaunay_materialized\":false,"
        << "\"dense_pair_fallback_performed\":false,"
        << "\"deployment_status\":\"" << kDeploymentStatus << "\","
        << "\"equality_shell_point_count\":"
        << generated.features.equality_shell_point_count << ','
        << "\"exact_prune_target_check_limit\":"
        << options.exact_prune_target_check_limit << ','
        << "\"family\":\"" << json_escape(options.family) << "\","
        << "\"generation_ns\":" << generation_ns << ','
        << "\"git_sha\":\"" << json_escape(kGitSha) << "\","
        << "\"global_pair_matrix_materialized\":false,"
        << "\"guard_size_at_most_50000\":true,"
        << "\"guard_size_passed\":"
        << (coverage_success ? "true" : "false")
        << ','
        << "\"higher_order_structure_materialized\":false,"
        << "\"jitter_grid_point_count\":"
        << generated.features.jitter_grid_point_count << ','
        << "\"mode\":\""
        << (warm_component_profile ? kWarmComponentMode : kMode) << "\","
        << "\"ordinary_delaunay_materialized\":false,"
        << "\"ordinary_emst_computed\":false,"
        << "\"peak_host_rss_bytes\":" << peak_host_rss_bytes() << ','
        << "\"permutation_sign_point_count\":"
        << generated.features.permutation_sign_point_count << ','
        << "\"point_count\":" << options.point_count << ','
        << "\"profile\":\"" << kProfile << "\","
        << "\"profile_exit_success\":"
        << (exit_success ? "true" : "false") << ','
        << "\"public_status\":\"" << kPublicStatus << "\","
        << "\"rank_results\":[";
    for (std::size_t index = 0U; index < results.size(); ++index) {
      if (index != 0U) {
        std::cout << ',';
      }
      print_rank_metrics(results[index]);
    }
    std::cout
        << "],\"all_rank_thresholds_exercised\":"
        << (all_rank_thresholds_exercised ? "true" : "false") << ','
        << "\"required_rank_triplet_exercised\":"
        << (rank_triplet_exercised ? "true" : "false") << ','
        << "\"sampled_prune_limit\":" << options.sampled_prune_limit << ','
        << "\"schema\":\""
        << (warm_component_profile ? kWarmComponentSchema : kSchema)
        << "\","
        << "\"scientific_pair_catalog_published\":false,"
        << "\"seed\":" << options.seed << ','
        << "\"success\":" << (coverage_success ? "true" : "false") << ','
        << "\"tight_cluster_point_count\":"
        << generated.features.clustered_point_count << ','
        << "\"total_ns\":" << total_ns;
    if (warm_component_profile) {
      std::cout
          << ",\"cuda_process_reused_across_runs\":true,"
          << "\"hierarchy_reduction_performed\":false,"
          << "\"hierarchy_tree_count\":0,"
          << "\"independent_rank_context_per_run\":true,"
          << "\"rank_threshold_count\":1,"
          << "\"timed_scope\":"
             "\"complete_rank_component_pass_excluding_generation_"
             "canonicalization_and_warmup\","
          << "\"warm_e2e_measured\":false,"
          << "\"warm_e2e_slo_claimed\":false,"
          << "\"warmup_excluded_from_total_ns\":true,"
          << "\"warmup_measurement_equivalence_validated\":"
          << (warmup_measurement_equivalence_validated ? "true" : "false")
          << ','
          << "\"warmup_run_count\":" << options.warmup_run_count << ','
          << "\"warmup_total_ns\":" << warmup_total_ns;
    }
    std::cout << "}\n";
    return exit_success ? EXIT_SUCCESS : EXIT_FAILURE;
  } catch (const std::exception& error) {
    if (scaling_smoke_requested || direct_scale_requested) {
      print_scaling_failure_json(error.what(), direct_scale_requested);
    } else if (strict_interior_threshold_requested) {
      print_strict_interior_threshold_failure_json(error.what());
    } else {
      print_failure_json(error.what());
    }
    return EXIT_FAILURE;
  }
}
