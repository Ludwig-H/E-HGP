#include "morsehgp3d/gpu/pair_support_phi.hpp"
#include "morsehgp3d/hierarchy/pair_support_stream.hpp"
#include "morsehgp3d/spatial/lbvh.hpp"
#include "morsehgp3d/spatial/point_cloud.hpp"

#include "pair_support_smoke_clouds.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#if !defined(MORSEHGP3D_GIT_SHA)
#error "The Phase 14Q component smoke requires a canonical Git SHA"
#endif

namespace {

using Clock = std::chrono::steady_clock;
using morsehgp3d::exact::CertifiedPoint3;
using morsehgp3d::gpu::PairSupportPhiContext;
using morsehgp3d::gpu::PairSupportRankPruneAudit;
using morsehgp3d::gpu::PairSupportRankPruneBatchResult;
using morsehgp3d::gpu::PairSupportRankPruneBudget;
using morsehgp3d::gpu::PairSupportRankPruneCapacity;
using morsehgp3d::gpu::PairSupportRankTraversalBackend;
using morsehgp3d::hierarchy::ExactPairSupportAuthorityContext;
using morsehgp3d::hierarchy::ExactPairSupportCheckpoint;
using morsehgp3d::hierarchy::ExactPairSupportRankKeepCertificate;
using morsehgp3d::hierarchy::ExactPairSupportRankPruneBatchProposal;
using morsehgp3d::hierarchy::ExactPairSupportRankPruneBatchRequest;
using morsehgp3d::hierarchy::ExactPairSupportRankPruneProposal;
using morsehgp3d::hierarchy::ExactPairSupportRankPruneProposedChunk;
using morsehgp3d::hierarchy::
    ExactPairSupportRankPruneProposedChunkVerification;
using morsehgp3d::hierarchy::ExactPairSupportStopReason;
using morsehgp3d::hierarchy::ExactPairSupportStreamAudit;
using morsehgp3d::hierarchy::ExactPairSupportStreamBudget;
using morsehgp3d::hierarchy::ExactPairSupportStreamChunk;
using morsehgp3d::hierarchy::ExactPairSupportStreamStatus;
using morsehgp3d::hierarchy::ExactPairSupportWitnessNodeEntry;
using morsehgp3d::spatial::CanonicalPointCloud;
using morsehgp3d::spatial::MortonLbvhIndex;
namespace smoke_clouds = morsehgp3d::tools::pair_support_smoke;

constexpr std::size_t kMaximumRunnerPointCount = 50'000U;
constexpr std::size_t kMaximumRunnerWorkUnitBudget = 1'000'000U;
constexpr std::size_t kMaximumRunnerProductCapacity = 65'536U;
constexpr std::size_t kMaximumRunnerWorkItemCapacity = 1'048'576U;
constexpr std::size_t kMaximumRunnerReceiptCapacity = 1'048'576U;
constexpr std::size_t kMaximumRunnerEpochBudget = 256U;
constexpr std::size_t kMaximumRunnerResidentReplayCount = 32U;
constexpr std::size_t kMaximumFrontierEntryCount = 65'536U;
constexpr std::size_t kMaximumAuxiliaryFrontierEntryCount = 8'192U;
constexpr std::size_t kMaximumEmittedRecordCount = 2'048U;
constexpr std::size_t kMaximumEmittedPointIdReferenceCount = 32'768U;
constexpr std::size_t kMaximumClosedBallQueryCount = 2'048U;
constexpr std::size_t kLegacyP1QueryCapacity = 1U;
constexpr std::size_t kRequestedMaximumOrder = 10U;
constexpr std::size_t kResidentLbvhNodeByteCount =
    10U * sizeof(std::uint64_t);
constexpr std::uint64_t kFnvOffsetBasis =
    UINT64_C(14695981039346656037);
constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);
static_assert(kResidentLbvhNodeByteCount == 80U);

struct Options {
  std::string family;
  std::size_t point_count{};
  std::size_t work_unit_budget{};
  PairSupportRankPruneCapacity capacity{};
  PairSupportRankPruneBudget epoch_budget{};
  std::size_t resident_replay_count{};
};

struct RankPruneRunAudit {
  PairSupportRankTraversalBackend traversal_backend{
      PairSupportRankTraversalBackend::two_frontier};
  std::size_t callback_count{};
  std::size_t input_product_count{};
  std::size_t required_strict_interior_point_count{};
  std::size_t traversal_epoch_count{};
  std::size_t count_kernel_launch_count{};
  std::size_t exclusive_scan_count{};
  std::size_t emit_kernel_launch_count{};
  std::size_t stackless_kernel_launch_count{};
  std::size_t host_synchronization_count{};
  std::size_t visited_work_item_count{};
  std::size_t visit_budget_count{};
  std::size_t peak_frontier_count{};
  std::size_t output_terminal_count{};
  std::size_t cpu_exact_terminal_recertification_count{};
  std::size_t strict_interior_terminal_count{};
  std::size_t anchor_noninterior_terminal_count{};
  std::size_t unresolved_external_leaf_terminal_count{};
  std::size_t prune_product_count{};
  std::size_t keep_certificate_product_count{};
  // Source-compatible receipt/proposal aliases remain independently audited.
  std::size_t output_receipt_count{};
  std::size_t cpu_exact_phi_recertification_count{};
  std::size_t proposed_product_count{};
  std::size_t fallback_product_count{};
  std::size_t snapshot_h2d_byte_count{};
  std::size_t escape_snapshot_h2d_byte_count{};
  std::size_t active_product_h2d_byte_count{};
  std::size_t initial_frontier_h2d_byte_count{};
  std::size_t traversal_metadata_d2h_byte_count{};
  std::size_t physical_terminal_d2h_byte_count{};
  std::size_t active_terminal_d2h_byte_count{};
  std::size_t physical_receipt_d2h_byte_count{};
  std::size_t active_receipt_d2h_byte_count{};
  std::size_t device_frontier_double_buffer_byte_capacity{};
  std::size_t device_escape_snapshot_byte_capacity{};
  std::size_t device_terminal_byte_capacity{};
  std::size_t device_receipt_byte_capacity{};
  std::size_t device_scan_workspace_byte_capacity{};
  std::size_t device_fixed_workspace_byte_capacity{};
  std::uint64_t first_buffer_epoch{};
  std::uint64_t last_buffer_epoch{};
  std::uint64_t terminal_digest_fold_fnv1a{kFnvOffsetBasis};
  std::uint64_t receipt_digest_fold_fnv1a{kFnvOffsetBasis};
  bool work_item_capacity_exhausted{false};
  bool receipt_capacity_exhausted{false};
  bool visit_budget_exhausted{false};
  bool epoch_budget_exhausted{false};
  bool stackless_single_product_traversal_used{false};
  bool stackless_product_batch_traversal_used{false};
  bool every_device_frontier_exhausted{true};
  bool every_snapshot_validated{true};
  bool every_product_record_validated{true};
  bool every_stable_terminal_transcript_validated{true};
  bool every_terminal_antichain_validated{true};
  bool every_keep_coverage_recertified{true};
  bool every_anchor_ball_culling_enabled{true};
  // Source-compatible validation aliases remain independently audited.
  bool every_stable_transcript_validated{true};
  bool every_antichain_validated{true};
  bool every_cpu_recertification_complete{true};
};

struct AssistedMeasurement {
  ExactPairSupportRankPruneProposedChunk proposed;
  RankPruneRunAudit gpu_audit;
  std::uint64_t elapsed_nanoseconds{};
};

[[noreturn]] void fail(std::string_view message) {
  throw std::runtime_error{std::string{message}};
}

void require(bool condition, std::string_view message) {
  if (!condition) {
    fail(message);
  }
}

[[nodiscard]] std::size_t parse_size(
    std::string_view text,
    std::string_view role) {
  std::size_t value = 0U;
  const char* const begin = text.data();
  const char* const end = text.data() + text.size();
  const auto [position, error] = std::from_chars(begin, end, value);
  if (text.empty() || error != std::errc{} || position != end) {
    fail(std::string{role} + " must be a canonical natural number");
  }
  return value;
}

[[nodiscard]] Options parse_options(int count, char** values) {
  Options options;
  bool family_seen = false;
  bool point_count_seen = false;
  bool work_budget_seen = false;
  bool product_capacity_seen = false;
  bool work_item_capacity_seen = false;
  bool receipt_capacity_seen = false;
  bool epoch_budget_seen = false;
  bool resident_replays_seen = false;
  for (int index = 1; index < count; ++index) {
    const std::string_view option{values[index]};
    if (option == "--help" || option == "-h") {
      std::cout
          << "Usage: "
             "morsehgp3d_gpu_pair_support_rank_search_component_smoke "
             "--family uniform_latin|eight_clusters --point-count N "
             "--work-unit-budget W --product-capacity P "
             "--work-item-capacity F --receipt-capacity R "
             "--epoch-budget E --resident-replays Q\n";
      std::exit(EXIT_SUCCESS);
    }
    if (index + 1 >= count) {
      fail(std::string{"missing value after "} + std::string{option});
    }
    const std::string_view value{values[++index]};
    if (option == "--family") {
      require(!family_seen, "--family may be supplied only once");
      options.family = std::string{value};
      family_seen = true;
    } else if (option == "--point-count") {
      require(
          !point_count_seen,
          "--point-count may be supplied only once");
      options.point_count = parse_size(value, "--point-count");
      point_count_seen = true;
    } else if (option == "--work-unit-budget") {
      require(
          !work_budget_seen,
          "--work-unit-budget may be supplied only once");
      options.work_unit_budget =
          parse_size(value, "--work-unit-budget");
      work_budget_seen = true;
    } else if (option == "--product-capacity") {
      require(
          !product_capacity_seen,
          "--product-capacity may be supplied only once");
      options.capacity.maximum_product_count =
          parse_size(value, "--product-capacity");
      product_capacity_seen = true;
    } else if (option == "--work-item-capacity") {
      require(
          !work_item_capacity_seen,
          "--work-item-capacity may be supplied only once");
      options.capacity.maximum_work_item_count =
          parse_size(value, "--work-item-capacity");
      work_item_capacity_seen = true;
    } else if (option == "--receipt-capacity") {
      require(
          !receipt_capacity_seen,
          "--receipt-capacity may be supplied only once");
      options.capacity.maximum_receipt_count =
          parse_size(value, "--receipt-capacity");
      receipt_capacity_seen = true;
    } else if (option == "--epoch-budget") {
      require(
          !epoch_budget_seen,
          "--epoch-budget may be supplied only once");
      options.epoch_budget.maximum_epoch_count =
          parse_size(value, "--epoch-budget");
      epoch_budget_seen = true;
    } else if (option == "--resident-replays") {
      require(
          !resident_replays_seen,
          "--resident-replays may be supplied only once");
      options.resident_replay_count =
          parse_size(value, "--resident-replays");
      resident_replays_seen = true;
    } else {
      fail(std::string{"unknown option: "} + std::string{option});
    }
  }
  require(
      family_seen && point_count_seen && work_budget_seen &&
          product_capacity_seen && work_item_capacity_seen &&
          receipt_capacity_seen && epoch_budget_seen &&
          resident_replays_seen,
      "all Phase 14Q component-smoke options are mandatory");
  require(
      options.family == "uniform_latin" ||
          options.family == "eight_clusters",
      "--family must be uniform_latin or eight_clusters");
  require(
      options.point_count >= 2U &&
          options.point_count <= kMaximumRunnerPointCount,
      "--point-count is outside the bounded range 2..50000");
  require(
      options.work_unit_budget > 0U &&
          options.work_unit_budget <= kMaximumRunnerWorkUnitBudget,
      "--work-unit-budget is outside the bounded range 1..1000000");
  require(
      options.capacity.maximum_product_count > 0U &&
          options.capacity.maximum_product_count <=
              kMaximumRunnerProductCapacity,
      "--product-capacity is outside the bounded range 1..65536");
  require(
      options.capacity.maximum_work_item_count > 0U &&
          options.capacity.maximum_work_item_count <=
              kMaximumRunnerWorkItemCapacity,
      "--work-item-capacity is outside the bounded range 1..1048576");
  require(
      options.capacity.maximum_receipt_count > 0U &&
          options.capacity.maximum_receipt_count <=
              kMaximumRunnerReceiptCapacity,
      "--receipt-capacity is outside the bounded range 1..1048576");
  require(
      options.capacity.maximum_work_item_count >=
          options.capacity.maximum_product_count,
      "--work-item-capacity must cover the initial product frontier");
  require(
      options.epoch_budget.maximum_epoch_count > 0U &&
          options.epoch_budget.maximum_epoch_count <=
              kMaximumRunnerEpochBudget,
      "--epoch-budget is outside the bounded range 1..256");
  require(
      options.resident_replay_count > 0U &&
          options.resident_replay_count <=
              kMaximumRunnerResidentReplayCount,
      "--resident-replays is outside the bounded range 1..32");
  return options;
}

template <typename Duration>
[[nodiscard]] std::uint64_t nanoseconds(Duration duration) {
  const auto value =
      std::chrono::duration_cast<std::chrono::nanoseconds>(duration)
          .count();
  if (value < 0) {
    fail("the monotonic component-smoke clock moved backwards");
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    fail(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_multiply(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (left != 0U &&
      right > std::numeric_limits<std::size_t>::max() / left) {
    fail(message);
  }
  return left * right;
}

void hash_word(std::uint64_t& digest, std::uint64_t word) noexcept {
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
}

void accumulate_rank_prune_audit(
    RankPruneRunAudit& aggregate,
    const PairSupportRankPruneAudit& audit,
    const Options& options) {
  require(
      audit.capacity == options.capacity &&
          audit.budget == options.epoch_budget,
      "a P2 callback changed its trusted capacity or epoch budget");
  require(
      audit.input_product_count > 0U &&
          audit.required_strict_interior_point_count > 0U,
      "a P2 callback did not exercise a positive product batch");
  require(
      audit.prune_product_count <= audit.input_product_count &&
          audit.keep_certificate_product_count <=
              audit.input_product_count - audit.prune_product_count &&
          audit.fallback_product_count ==
              audit.input_product_count -
                  audit.prune_product_count -
                  audit.keep_certificate_product_count,
      "the P4 prune/keep/fallback product accounting does not close");
  require(
      audit.cpu_exact_terminal_recertification_count ==
              audit.gpu_output_terminal_count &&
          checked_add(
              checked_add(
                  audit.strict_interior_terminal_count,
                  audit.anchor_noninterior_terminal_count,
                  "the P4 terminal classification count overflows"),
              audit.unresolved_external_leaf_terminal_count,
              "the P4 terminal classification count overflows") ==
              audit.gpu_output_terminal_count,
      "the P4 exact terminal classification accounting does not close");
  require(
      audit.gpu_output_receipt_count ==
              audit.gpu_output_terminal_count &&
          audit.cpu_exact_phi_recertification_count ==
              audit.cpu_exact_terminal_recertification_count &&
          audit.proposed_product_count == audit.prune_product_count &&
          audit.physical_receipt_d2h_byte_count ==
              audit.physical_terminal_d2h_byte_count &&
          audit.active_receipt_d2h_byte_count ==
              audit.active_terminal_d2h_byte_count &&
          audit.device_receipt_byte_capacity ==
              audit.device_terminal_byte_capacity &&
          audit.receipt_digest_fnv1a == audit.terminal_digest_fnv1a,
      "the P4 source-compatible receipt aliases diverge from terminals");
  if (audit.stackless_single_product_traversal_used ||
      audit.stackless_product_batch_traversal_used) {
    require(
        audit.stackless_single_product_traversal_used !=
                audit.stackless_product_batch_traversal_used &&
            audit.traversal_backend ==
                (audit.stackless_single_product_traversal_used
                     ? PairSupportRankTraversalBackend::
                           stackless_single_product
                     : PairSupportRankTraversalBackend::
                           stackless_product_batch) &&
            (audit.stackless_single_product_traversal_used
                 ? audit.capacity.maximum_product_count == 1U
                 : audit.capacity.maximum_product_count > 1U) &&
            audit.input_product_count > 0U &&
            audit.input_product_count <=
                audit.capacity.maximum_product_count &&
            audit.gpu_traversal_epoch_count == 1U &&
            audit.gpu_count_kernel_launch_count == 0U &&
            audit.gpu_exclusive_scan_count == 0U &&
            audit.gpu_emit_kernel_launch_count == 1U &&
            audit.gpu_stackless_kernel_launch_count == 1U &&
            audit.gpu_host_synchronization_count ==
                1U + static_cast<std::size_t>(
                         audit.gpu_output_terminal_count != 0U) &&
            audit.gpu_visited_work_item_count <=
                audit.gpu_visit_budget_count &&
            audit.initial_frontier_h2d_byte_count == 0U &&
            audit.device_frontier_double_buffer_byte_capacity == 0U &&
            audit.device_scan_workspace_byte_capacity == 0U,
        "the stackless launch/visit accounting does not close");
  } else {
    require(
        audit.traversal_backend ==
                PairSupportRankTraversalBackend::two_frontier &&
            audit.gpu_count_kernel_launch_count ==
                audit.gpu_traversal_epoch_count &&
            audit.gpu_exclusive_scan_count ==
                2U * audit.gpu_traversal_epoch_count &&
            audit.gpu_emit_kernel_launch_count <=
                audit.gpu_traversal_epoch_count &&
            audit.gpu_stackless_kernel_launch_count == 0U &&
            audit.gpu_host_synchronization_count ==
                audit.gpu_traversal_epoch_count + 1U &&
            audit.gpu_visit_budget_count == 0U &&
            audit.escape_snapshot_h2d_byte_count == 0U &&
            audit.device_escape_snapshot_byte_capacity == 0U &&
            !audit.visit_budget_exhausted,
        "the P2 two-frontier count/scan/emit accounting does not close");
  }
  require(
      audit.immutable_lbvh_snapshot_validated &&
          audit.product_records_validated &&
          audit.stable_terminal_transcript_validated &&
          audit.disjoint_terminal_antichains_validated &&
          audit.keep_coverage_recertification_complete &&
          audit.anchor_ball_culling_enabled &&
          audit.stable_receipt_transcript_validated &&
          audit.disjoint_receipt_antichains_validated &&
          audit.cpu_exact_recertification_complete &&
          !audit.global_support_product_prune_published &&
          !audit.public_status_published,
      "the P4 terminal audit does not preserve exact proposal-only semantics");
  require(
      !audit.work_item_capacity_exhausted &&
          !audit.receipt_capacity_exhausted &&
          !audit.visit_budget_exhausted &&
          !audit.epoch_budget_exhausted &&
          audit.device_frontier_exhausted,
      "the P4 component smoke exhausted a capacity or epoch budget "
      "before terminal coverage");
  if (aggregate.callback_count == 0U) {
    aggregate.traversal_backend = audit.traversal_backend;
    aggregate.stackless_single_product_traversal_used =
        audit.stackless_single_product_traversal_used;
    aggregate.stackless_product_batch_traversal_used =
        audit.stackless_product_batch_traversal_used;
    aggregate.required_strict_interior_point_count =
        audit.required_strict_interior_point_count;
    aggregate.first_buffer_epoch = audit.buffer_epoch;
  } else {
    require(
        audit.required_strict_interior_point_count ==
                aggregate.required_strict_interior_point_count &&
            audit.traversal_backend == aggregate.traversal_backend &&
            audit.stackless_single_product_traversal_used ==
                aggregate.stackless_single_product_traversal_used &&
            audit.stackless_product_batch_traversal_used ==
                aggregate.stackless_product_batch_traversal_used,
        "a P2 callback changed its rank threshold or traversal backend "
        "within one run");
    require(
        audit.buffer_epoch > aggregate.last_buffer_epoch,
        "the P2 callback buffer epoch did not advance");
  }
  aggregate.last_buffer_epoch = audit.buffer_epoch;
  aggregate.callback_count = checked_add(
      aggregate.callback_count,
      1U,
      "the P2 callback count overflows size_t");
#define MORSEHGP3D_ACCUMULATE_AUDIT(target, source) \
  aggregate.target = checked_add(                    \
      aggregate.target,                              \
      audit.source,                                  \
      "a P2 aggregate counter overflows size_t")
  MORSEHGP3D_ACCUMULATE_AUDIT(
      input_product_count, input_product_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      traversal_epoch_count, gpu_traversal_epoch_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      count_kernel_launch_count, gpu_count_kernel_launch_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      exclusive_scan_count, gpu_exclusive_scan_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      emit_kernel_launch_count, gpu_emit_kernel_launch_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      stackless_kernel_launch_count,
      gpu_stackless_kernel_launch_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      host_synchronization_count,
      gpu_host_synchronization_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      visited_work_item_count, gpu_visited_work_item_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      visit_budget_count, gpu_visit_budget_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      output_terminal_count, gpu_output_terminal_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      cpu_exact_terminal_recertification_count,
      cpu_exact_terminal_recertification_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      strict_interior_terminal_count,
      strict_interior_terminal_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      anchor_noninterior_terminal_count,
      anchor_noninterior_terminal_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      unresolved_external_leaf_terminal_count,
      unresolved_external_leaf_terminal_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      prune_product_count, prune_product_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      keep_certificate_product_count,
      keep_certificate_product_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      output_receipt_count, gpu_output_receipt_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      cpu_exact_phi_recertification_count,
      cpu_exact_phi_recertification_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      proposed_product_count, proposed_product_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      fallback_product_count, fallback_product_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      snapshot_h2d_byte_count, snapshot_h2d_byte_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      escape_snapshot_h2d_byte_count,
      escape_snapshot_h2d_byte_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      active_product_h2d_byte_count,
      active_product_h2d_byte_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      initial_frontier_h2d_byte_count,
      initial_frontier_h2d_byte_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      traversal_metadata_d2h_byte_count,
      traversal_metadata_d2h_byte_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      physical_terminal_d2h_byte_count,
      physical_terminal_d2h_byte_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      active_terminal_d2h_byte_count,
      active_terminal_d2h_byte_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      physical_receipt_d2h_byte_count,
      physical_receipt_d2h_byte_count);
  MORSEHGP3D_ACCUMULATE_AUDIT(
      active_receipt_d2h_byte_count,
      active_receipt_d2h_byte_count);
#undef MORSEHGP3D_ACCUMULATE_AUDIT
  aggregate.peak_frontier_count = std::max(
      aggregate.peak_frontier_count,
      audit.gpu_peak_frontier_count);
  aggregate.device_frontier_double_buffer_byte_capacity = std::max(
      aggregate.device_frontier_double_buffer_byte_capacity,
      audit.device_frontier_double_buffer_byte_capacity);
  aggregate.device_escape_snapshot_byte_capacity = std::max(
      aggregate.device_escape_snapshot_byte_capacity,
      audit.device_escape_snapshot_byte_capacity);
  aggregate.device_terminal_byte_capacity = std::max(
      aggregate.device_terminal_byte_capacity,
      audit.device_terminal_byte_capacity);
  aggregate.device_receipt_byte_capacity = std::max(
      aggregate.device_receipt_byte_capacity,
      audit.device_receipt_byte_capacity);
  aggregate.device_scan_workspace_byte_capacity = std::max(
      aggregate.device_scan_workspace_byte_capacity,
      audit.device_scan_workspace_byte_capacity);
  aggregate.device_fixed_workspace_byte_capacity = std::max(
      aggregate.device_fixed_workspace_byte_capacity,
      audit.device_fixed_workspace_byte_capacity);
  aggregate.work_item_capacity_exhausted =
      aggregate.work_item_capacity_exhausted ||
      audit.work_item_capacity_exhausted;
  aggregate.receipt_capacity_exhausted =
      aggregate.receipt_capacity_exhausted ||
      audit.receipt_capacity_exhausted;
  aggregate.visit_budget_exhausted =
      aggregate.visit_budget_exhausted ||
      audit.visit_budget_exhausted;
  aggregate.epoch_budget_exhausted =
      aggregate.epoch_budget_exhausted ||
      audit.epoch_budget_exhausted;
  aggregate.every_device_frontier_exhausted =
      aggregate.every_device_frontier_exhausted &&
      audit.device_frontier_exhausted;
  aggregate.every_snapshot_validated =
      aggregate.every_snapshot_validated &&
      audit.immutable_lbvh_snapshot_validated;
  aggregate.every_product_record_validated =
      aggregate.every_product_record_validated &&
      audit.product_records_validated;
  aggregate.every_stable_terminal_transcript_validated =
      aggregate.every_stable_terminal_transcript_validated &&
      audit.stable_terminal_transcript_validated;
  aggregate.every_terminal_antichain_validated =
      aggregate.every_terminal_antichain_validated &&
      audit.disjoint_terminal_antichains_validated;
  aggregate.every_keep_coverage_recertified =
      aggregate.every_keep_coverage_recertified &&
      audit.keep_coverage_recertification_complete;
  aggregate.every_anchor_ball_culling_enabled =
      aggregate.every_anchor_ball_culling_enabled &&
      audit.anchor_ball_culling_enabled;
  aggregate.every_stable_transcript_validated =
      aggregate.every_stable_transcript_validated &&
      audit.stable_receipt_transcript_validated;
  aggregate.every_antichain_validated =
      aggregate.every_antichain_validated &&
      audit.disjoint_receipt_antichains_validated;
  aggregate.every_cpu_recertification_complete =
      aggregate.every_cpu_recertification_complete &&
      audit.cpu_exact_recertification_complete;
  hash_word(
      aggregate.terminal_digest_fold_fnv1a,
      audit.terminal_digest_fnv1a);
  hash_word(
      aggregate.receipt_digest_fold_fnv1a,
      audit.receipt_digest_fnv1a);
}

[[nodiscard]] bool stable_gpu_audit_equal(
    const RankPruneRunAudit& left,
    const RankPruneRunAudit& right) {
  return left.callback_count == right.callback_count &&
         left.traversal_backend == right.traversal_backend &&
         left.stackless_single_product_traversal_used ==
             right.stackless_single_product_traversal_used &&
         left.stackless_product_batch_traversal_used ==
             right.stackless_product_batch_traversal_used &&
         left.input_product_count == right.input_product_count &&
         left.required_strict_interior_point_count ==
             right.required_strict_interior_point_count &&
         left.traversal_epoch_count == right.traversal_epoch_count &&
         left.count_kernel_launch_count ==
             right.count_kernel_launch_count &&
         left.exclusive_scan_count == right.exclusive_scan_count &&
         left.emit_kernel_launch_count ==
             right.emit_kernel_launch_count &&
         left.stackless_kernel_launch_count ==
             right.stackless_kernel_launch_count &&
         left.host_synchronization_count ==
             right.host_synchronization_count &&
         left.visited_work_item_count ==
             right.visited_work_item_count &&
         left.visit_budget_count == right.visit_budget_count &&
         left.peak_frontier_count == right.peak_frontier_count &&
         left.output_terminal_count == right.output_terminal_count &&
         left.cpu_exact_terminal_recertification_count ==
             right.cpu_exact_terminal_recertification_count &&
         left.strict_interior_terminal_count ==
             right.strict_interior_terminal_count &&
         left.anchor_noninterior_terminal_count ==
             right.anchor_noninterior_terminal_count &&
         left.unresolved_external_leaf_terminal_count ==
             right.unresolved_external_leaf_terminal_count &&
         left.prune_product_count == right.prune_product_count &&
         left.keep_certificate_product_count ==
             right.keep_certificate_product_count &&
         left.output_receipt_count == right.output_receipt_count &&
         left.cpu_exact_phi_recertification_count ==
             right.cpu_exact_phi_recertification_count &&
         left.proposed_product_count == right.proposed_product_count &&
         left.fallback_product_count == right.fallback_product_count &&
         left.active_product_h2d_byte_count ==
             right.active_product_h2d_byte_count &&
         left.initial_frontier_h2d_byte_count ==
             right.initial_frontier_h2d_byte_count &&
         left.traversal_metadata_d2h_byte_count ==
             right.traversal_metadata_d2h_byte_count &&
         left.physical_terminal_d2h_byte_count ==
             right.physical_terminal_d2h_byte_count &&
         left.active_terminal_d2h_byte_count ==
             right.active_terminal_d2h_byte_count &&
         left.physical_receipt_d2h_byte_count ==
             right.physical_receipt_d2h_byte_count &&
         left.active_receipt_d2h_byte_count ==
             right.active_receipt_d2h_byte_count &&
         left.device_frontier_double_buffer_byte_capacity ==
             right.device_frontier_double_buffer_byte_capacity &&
         left.device_escape_snapshot_byte_capacity ==
             right.device_escape_snapshot_byte_capacity &&
         left.device_terminal_byte_capacity ==
             right.device_terminal_byte_capacity &&
         left.device_receipt_byte_capacity ==
             right.device_receipt_byte_capacity &&
         left.device_scan_workspace_byte_capacity ==
             right.device_scan_workspace_byte_capacity &&
         left.device_fixed_workspace_byte_capacity ==
             right.device_fixed_workspace_byte_capacity &&
         left.terminal_digest_fold_fnv1a ==
             right.terminal_digest_fold_fnv1a &&
         left.receipt_digest_fold_fnv1a ==
             right.receipt_digest_fold_fnv1a &&
         left.work_item_capacity_exhausted ==
             right.work_item_capacity_exhausted &&
         left.receipt_capacity_exhausted ==
             right.receipt_capacity_exhausted &&
         left.visit_budget_exhausted ==
             right.visit_budget_exhausted &&
         left.epoch_budget_exhausted ==
             right.epoch_budget_exhausted &&
         left.every_device_frontier_exhausted ==
             right.every_device_frontier_exhausted &&
         left.every_snapshot_validated ==
             right.every_snapshot_validated &&
         left.every_product_record_validated ==
             right.every_product_record_validated &&
         left.every_stable_terminal_transcript_validated ==
             right.every_stable_terminal_transcript_validated &&
         left.every_terminal_antichain_validated ==
             right.every_terminal_antichain_validated &&
         left.every_keep_coverage_recertified ==
             right.every_keep_coverage_recertified &&
         left.every_anchor_ball_culling_enabled ==
             right.every_anchor_ball_culling_enabled &&
         left.every_stable_transcript_validated ==
             right.every_stable_transcript_validated &&
         left.every_antichain_validated ==
             right.every_antichain_validated &&
         left.every_cpu_recertification_complete ==
             right.every_cpu_recertification_complete;
}

[[nodiscard]] ExactPairSupportStreamBudget make_stream_budget(
    const Options& options) {
  return ExactPairSupportStreamBudget{
      options.work_unit_budget,
      kMaximumFrontierEntryCount,
      kMaximumAuxiliaryFrontierEntryCount,
      kMaximumEmittedRecordCount,
      kMaximumEmittedPointIdReferenceCount,
      kMaximumClosedBallQueryCount,
      checked_multiply(
          kMaximumClosedBallQueryCount,
          options.point_count,
          "the component-smoke point-classification cap overflows")};
}

[[nodiscard]] bool canonical_terminal_less(
    const ExactPairSupportWitnessNodeEntry& left,
    const ExactPairSupportWitnessNodeEntry& right) noexcept {
  return left.leaf_begin < right.leaf_begin ||
         (left.leaf_begin == right.leaf_begin &&
          (left.leaf_end < right.leaf_end ||
           (left.leaf_end == right.leaf_end &&
            left.node_index < right.node_index)));
}

void require_exact_keep_coverage(
    const morsehgp3d::gpu::PairSupportRankKeepProductCertificate&
        certificate,
    std::size_t point_count) {
  require(
      certificate.product.self_product == 0U,
      "a GPU P4 keep certificate targets a self product");
  std::vector<std::pair<std::uint64_t, std::uint64_t>> coverage;
  coverage.reserve(certificate.canonical_terminals.size() + 2U);
  coverage.emplace_back(
      certificate.product.first_leaf_begin,
      certificate.product.first_leaf_end);
  coverage.emplace_back(
      certificate.product.second_leaf_begin,
      certificate.product.second_leaf_end);
  for (const auto& terminal : certificate.canonical_terminals) {
    coverage.emplace_back(
        terminal.terminal.leaf_begin,
        terminal.terminal.leaf_end);
  }
  std::sort(coverage.begin(), coverage.end());
  std::uint64_t cursor = 0U;
  for (const auto& [leaf_begin, leaf_end] : coverage) {
    require(
        leaf_begin == cursor &&
            leaf_begin < leaf_end &&
            leaf_end <= static_cast<std::uint64_t>(point_count),
        "a GPU P4 keep certificate does not form an exact root cut");
    cursor = leaf_end;
  }
  require(
      cursor == static_cast<std::uint64_t>(point_count),
      "a GPU P4 keep certificate does not cover the LBVH root");
}

[[nodiscard]] ExactPairSupportRankPruneBatchProposal invoke_gpu_callback(
    PairSupportPhiContext& context,
    const Options& options,
    RankPruneRunAudit& aggregate,
    const ExactPairSupportRankPruneBatchRequest& request) {
  require(
      !request.canonical_products.empty(),
      "the CPU stream requested an empty P2 batch");
  PairSupportRankPruneBatchResult gpu_result =
      context.propose_rank_prunes(
          request.canonical_products,
          request.required_strict_interior_point_count,
          options.epoch_budget);
  accumulate_rank_prune_audit(aggregate, gpu_result.audit, options);

  ExactPairSupportRankPruneBatchProposal result;
  result.proposals.reserve(gpu_result.proposals.size());
  result.keep_certificates.reserve(
      gpu_result.keep_certificates.size());
  std::vector<std::uint8_t> product_disposition(
      request.canonical_products.size(), 0U);
  std::size_t previous_product_index = 0U;
  bool first = true;
  for (auto& proposal : gpu_result.proposals) {
    require(
        proposal.product_index < request.canonical_products.size() &&
            proposal.product ==
                request.canonical_products[proposal.product_index] &&
            !proposal.strict_witness_receipts.empty() &&
            proposal.strict_witness_receipts.size() <=
                request.required_strict_interior_point_count &&
            proposal.strict_interior_point_count >=
                request.required_strict_interior_point_count,
        "the GPU P2 proposal contradicts its synchronous CPU request");
    require(
        product_disposition[proposal.product_index] == 0U,
        "the GPU P4 classified one product more than once");
    product_disposition[proposal.product_index] = 1U;
    require(
        first || previous_product_index < proposal.product_index,
        "the GPU P2 proposals are not in canonical product order");
    first = false;
    previous_product_index = proposal.product_index;
    std::size_t receipt_point_count = 0U;
    for (std::size_t receipt_index = 0U;
         receipt_index < proposal.strict_witness_receipts.size();
         ++receipt_index) {
      const auto& receipt =
          proposal.strict_witness_receipts[receipt_index];
      require(
          receipt.leaf_begin < receipt.leaf_end,
          "a GPU P2 receipt has an empty Morton interval");
      if (receipt_index != 0U) {
        const auto& previous =
            proposal.strict_witness_receipts[receipt_index - 1U];
        require(
            canonical_terminal_less(previous, receipt) &&
                previous.leaf_end <= receipt.leaf_begin,
            "GPU P2 receipts are not in canonical interval order");
      }
      receipt_point_count = checked_add(
          receipt_point_count,
          static_cast<std::size_t>(
              receipt.leaf_end - receipt.leaf_begin),
          "the GPU P2 receipt cardinality overflows size_t");
      if (receipt_index + 1U <
          proposal.strict_witness_receipts.size()) {
        require(
            receipt_point_count <
                request.required_strict_interior_point_count,
            "a GPU P2 proposal retained receipts after its minimal prefix");
      }
    }
    require(
        receipt_point_count == proposal.strict_interior_point_count,
        "the GPU P2 receipt cardinality does not match its proposal");
    result.proposals.push_back(ExactPairSupportRankPruneProposal{
        proposal.product,
        std::move(proposal.strict_witness_receipts)});
  }

  previous_product_index = 0U;
  first = true;
  for (const auto& certificate : gpu_result.keep_certificates) {
    require(
        certificate.product_index <
                request.canonical_products.size() &&
            certificate.product ==
                request.canonical_products[
                    certificate.product_index] &&
            !certificate.canonical_terminals.empty() &&
            certificate.strict_interior_point_count <
                request.required_strict_interior_point_count,
        "the GPU P4 keep certificate contradicts its synchronous "
        "CPU request");
    require(
        product_disposition[certificate.product_index] == 0U,
        "the GPU P4 prune and keep subsets are not disjoint");
    product_disposition[certificate.product_index] = 2U;
    require(
        first ||
            previous_product_index < certificate.product_index,
        "the GPU P4 keep certificates are not in canonical "
        "product order");
    first = false;
    previous_product_index = certificate.product_index;

    std::vector<ExactPairSupportWitnessNodeEntry>
        canonical_terminals;
    canonical_terminals.reserve(
        certificate.canonical_terminals.size());
    std::size_t diagnostic_strict_point_count = 0U;
    for (std::size_t terminal_index = 0U;
         terminal_index < certificate.canonical_terminals.size();
         ++terminal_index) {
      const auto& gpu_terminal =
          certificate.canonical_terminals[terminal_index];
      const ExactPairSupportWitnessNodeEntry& terminal =
          gpu_terminal.terminal;
      require(
          terminal.leaf_begin < terminal.leaf_end &&
              terminal.leaf_end <=
                  static_cast<std::uint64_t>(
                      options.point_count),
          "a GPU P4 keep terminal has an invalid Morton interval");
      if (terminal_index != 0U) {
        const auto& previous =
            certificate.canonical_terminals[
                terminal_index - 1U]
                .terminal;
        require(
            canonical_terminal_less(previous, terminal) &&
                previous.leaf_end <= terminal.leaf_begin,
            "GPU P4 keep terminals are not a canonical "
            "disjoint antichain");
      }
      if (gpu_terminal.kind ==
          morsehgp3d::gpu::
              PairSupportRankTerminalKind::strict_interior) {
        diagnostic_strict_point_count = checked_add(
            diagnostic_strict_point_count,
            static_cast<std::size_t>(
                terminal.leaf_end - terminal.leaf_begin),
            "the GPU P4 strict-terminal cardinality overflows "
            "size_t");
      }
      // Deliberately discard the device-side kind.  The exact hierarchy
      // reclassifies this authenticated terminal before accepting a keep.
      canonical_terminals.push_back(terminal);
    }
    require(
        diagnostic_strict_point_count ==
            certificate.strict_interior_point_count,
        "the GPU P4 keep strict-terminal cardinality does not close");
    require_exact_keep_coverage(certificate, options.point_count);
    result.keep_certificates.push_back(
        ExactPairSupportRankKeepCertificate{
            certificate.product, std::move(canonical_terminals)});
  }

  const std::size_t fallback_product_count =
      static_cast<std::size_t>(std::count(
          product_disposition.begin(),
          product_disposition.end(),
          static_cast<std::uint8_t>(0U)));
  require(
      result.proposals.size() ==
              gpu_result.audit.prune_product_count &&
          result.keep_certificates.size() ==
              gpu_result.audit.keep_certificate_product_count &&
          fallback_product_count ==
              gpu_result.audit.fallback_product_count,
      "the GPU P4 returned subsets disagree with its exact product "
      "accounting");
  return result;
}

[[nodiscard]] AssistedMeasurement run_assisted(
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportStreamBudget& stream_budget,
    const ExactPairSupportCheckpoint& source_checkpoint,
    PairSupportPhiContext& gpu_context,
    const Options& options) {
  RankPruneRunAudit aggregate;
  const auto callback =
      [&gpu_context, &options, &aggregate](
          const ExactPairSupportRankPruneBatchRequest& request) {
        return invoke_gpu_callback(
            gpu_context, options, aggregate, request);
      };
  const Clock::time_point start = Clock::now();
  ExactPairSupportRankPruneProposedChunk proposed =
      morsehgp3d::hierarchy::
          build_exact_pair_support_stream_chunk_with_rank_prune_proposals(
              authority,
              stream_budget,
              source_checkpoint,
              options.capacity.maximum_product_count,
              callback);
  const Clock::time_point end = Clock::now();
  require(
      aggregate.callback_count > 0U,
      "the bounded component smoke did not exercise the GPU P4 callback");
  std::size_t consumed_keep_terminal_count = 0U;
  for (const auto& certificate :
       proposed.consumed_keep_certificates) {
    consumed_keep_terminal_count = checked_add(
        consumed_keep_terminal_count,
        certificate.canonical_terminals.size(),
        "the consumed P4 keep-terminal count overflows size_t");
  }
  require(
      aggregate.prune_product_count > 0U &&
          proposed.audit.consumed_proposal_count > 0U &&
          proposed.audit.proposal_pruned_pair_count > 0U,
      "the bounded component smoke did not consume an effective GPU "
      "rank-prune proposal");
  require(
      aggregate.keep_certificate_product_count > 0U &&
          proposed.audit.consumed_keep_certificate_count > 0U &&
          !proposed.consumed_keep_certificates.empty(),
      "the bounded component smoke did not consume an effective GPU "
      "conservative-keep certificate");
  require(
      proposed.audit.consumed_proposal_count ==
              proposed.consumed_proposals.size() &&
          proposed.audit.consumed_keep_certificate_count ==
              proposed.consumed_keep_certificates.size() &&
          proposed.audit.exact_receipt_recertification_count ==
              proposed.audit.consumed_receipt_count &&
          proposed.audit.exact_keep_terminal_recertification_count ==
              consumed_keep_terminal_count &&
          proposed.audit.keep_terminal_classification_count ==
              consumed_keep_terminal_count &&
          checked_add(
              checked_add(
                  proposed.audit.keep_strict_interior_terminal_count,
                  proposed.audit.keep_anchor_noninterior_terminal_count,
                  "the P4 consumed terminal-class count overflows"),
              proposed.audit.keep_external_leaf_terminal_count,
              "the P4 consumed terminal-class count overflows") ==
              consumed_keep_terminal_count,
      "the exact CPU prune/keep replay accounting does not close");
  require(
      proposed.audit.transcript_ephemeral &&
          proposed.audit.no_forbidden_global_structure_materialized &&
          !proposed.audit.durable_wire_v1_claimed,
      "the bounded component smoke escaped its ephemeral terminal-proposal "
      "contract");
  require(
      !aggregate.work_item_capacity_exhausted &&
          !aggregate.receipt_capacity_exhausted &&
          !aggregate.epoch_budget_exhausted &&
          aggregate.every_device_frontier_exhausted &&
          aggregate.every_keep_coverage_recertified &&
          aggregate.every_anchor_ball_culling_enabled,
      "the bounded component smoke exhausted a P4 capacity or lost "
      "terminal coverage");
  return AssistedMeasurement{
      std::move(proposed),
      std::move(aggregate),
      nanoseconds(end - start)};
}

[[nodiscard]] std::string_view status_text(
    ExactPairSupportStreamStatus status) {
  switch (status) {
    case ExactPairSupportStreamStatus::complete:
      return "complete";
    case ExactPairSupportStreamStatus::budget_exhausted:
      return "budget_exhausted";
  }
  fail("an invalid pair-support stream status escaped");
}

[[nodiscard]] std::string_view stop_reason_text(
    ExactPairSupportStopReason reason) {
  switch (reason) {
    case ExactPairSupportStopReason::none:
      return "none";
    case ExactPairSupportStopReason::work_unit_limit:
      return "work_unit_limit";
    case ExactPairSupportStopReason::frontier_entry_limit:
      return "frontier_entry_limit";
    case ExactPairSupportStopReason::auxiliary_frontier_entry_limit:
      return "auxiliary_frontier_entry_limit";
    case ExactPairSupportStopReason::emitted_record_limit:
      return "emitted_record_limit";
    case ExactPairSupportStopReason::emitted_point_id_reference_limit:
      return "emitted_point_id_reference_limit";
    case ExactPairSupportStopReason::global_closed_ball_query_limit:
      return "global_closed_ball_query_limit";
    case ExactPairSupportStopReason::closed_ball_node_visit_limit:
      return "closed_ball_node_visit_limit";
  }
  fail("an invalid pair-support stop reason escaped");
}

void write_bool(std::ostream& output, bool value) {
  output << (value ? "true" : "false");
}

[[nodiscard]] std::string_view traversal_backend_text(
    PairSupportRankTraversalBackend backend) {
  switch (backend) {
    case PairSupportRankTraversalBackend::two_frontier:
      return "two_frontier";
    case PairSupportRankTraversalBackend::stackless_single_product:
      return "stackless_single_product";
    case PairSupportRankTraversalBackend::stackless_product_batch:
      return "stackless_product_batch";
  }
  fail("an invalid pair-rank traversal backend escaped");
}

void write_stream_audit(
    std::ostream& output,
    const ExactPairSupportStreamAudit& audit) {
  output
      << "{\"accepted_event_count\":" << audit.accepted_event_count
      << ",\"exact_phi_aabb_bound_count\":"
      << audit.exact_phi_aabb_bound_count
      << ",\"global_closed_ball_query_count\":"
      << audit.global_closed_ball_query_count
      << ",\"leaf_pair_classification_count\":"
      << audit.leaf_pair_classification_count
      << ",\"maximum_frontier_entry_count\":"
      << audit.maximum_frontier_entry_count
      << ",\"maximum_witness_frontier_entry_count\":"
      << audit.maximum_witness_frontier_entry_count
      << ",\"pair_partition_accounting_certified\":";
  write_bool(output, audit.pair_partition_accounting_certified);
  output
      << ",\"rank_prune_search_count\":"
      << audit.rank_prune_search_count
      << ",\"rank_pruned_pair_count\":"
      << audit.rank_pruned_pair_count
      << ",\"rank_pruned_product_count\":"
      << audit.rank_pruned_product_count
      << ",\"remaining_frontier_pair_count\":"
      << audit.remaining_frontier_pair_count
      << ",\"resolved_pair_count\":" << audit.resolved_pair_count
      << ",\"support_product_expansion_count\":"
      << audit.support_product_expansion_count
      << ",\"support_product_visit_count\":"
      << audit.support_product_visit_count
      << ",\"total_pair_count\":" << audit.total_pair_count
      << ",\"witness_node_visit_count\":"
      << audit.witness_node_visit_count
      << ",\"work_unit_count\":" << audit.work_unit_count << '}';
}

void write_gpu_audit(
    std::ostream& output,
    const RankPruneRunAudit& audit) {
  output
      << "{\"active_product_h2d_byte_count\":"
      << audit.active_product_h2d_byte_count
      << ",\"active_terminal_d2h_byte_count\":"
      << audit.active_terminal_d2h_byte_count
      << ",\"active_receipt_d2h_byte_count\":"
      << audit.active_receipt_d2h_byte_count
      << ",\"anchor_noninterior_terminal_count\":"
      << audit.anchor_noninterior_terminal_count
      << ",\"callback_count\":" << audit.callback_count
      << ",\"count_kernel_launch_count\":"
      << audit.count_kernel_launch_count
      << ",\"cpu_exact_terminal_recertification_count\":"
      << audit.cpu_exact_terminal_recertification_count
      << ",\"cpu_exact_phi_recertification_count\":"
      << audit.cpu_exact_phi_recertification_count
      << ",\"device_frontier_double_buffer_byte_capacity\":"
      << audit.device_frontier_double_buffer_byte_capacity
      << ",\"device_escape_snapshot_byte_capacity\":"
      << audit.device_escape_snapshot_byte_capacity
      << ",\"device_terminal_byte_capacity\":"
      << audit.device_terminal_byte_capacity
      << ",\"device_receipt_byte_capacity\":"
      << audit.device_receipt_byte_capacity
      << ",\"device_scan_workspace_byte_capacity\":"
      << audit.device_scan_workspace_byte_capacity
      << ",\"device_fixed_workspace_byte_capacity\":"
      << audit.device_fixed_workspace_byte_capacity
      << ",\"emit_kernel_launch_count\":"
      << audit.emit_kernel_launch_count
      << ",\"escape_snapshot_h2d_byte_count\":"
      << audit.escape_snapshot_h2d_byte_count
      << ",\"epoch_budget_exhausted\":";
  write_bool(output, audit.epoch_budget_exhausted);
  output
      << ",\"every_antichain_validated\":";
  write_bool(output, audit.every_antichain_validated);
  output << ",\"every_anchor_ball_culling_enabled\":";
  write_bool(output, audit.every_anchor_ball_culling_enabled);
  output << ",\"every_cpu_recertification_complete\":";
  write_bool(output, audit.every_cpu_recertification_complete);
  output << ",\"every_device_frontier_exhausted\":";
  write_bool(output, audit.every_device_frontier_exhausted);
  output << ",\"every_keep_coverage_recertified\":";
  write_bool(output, audit.every_keep_coverage_recertified);
  output << ",\"every_product_record_validated\":";
  write_bool(output, audit.every_product_record_validated);
  output << ",\"every_snapshot_validated\":";
  write_bool(output, audit.every_snapshot_validated);
  output << ",\"every_stable_terminal_transcript_validated\":";
  write_bool(
      output, audit.every_stable_terminal_transcript_validated);
  output << ",\"every_stable_transcript_validated\":";
  write_bool(output, audit.every_stable_transcript_validated);
  output << ",\"every_terminal_antichain_validated\":";
  write_bool(output, audit.every_terminal_antichain_validated);
  output
      << ",\"exclusive_scan_count\":" << audit.exclusive_scan_count
      << ",\"fallback_product_count\":"
      << audit.fallback_product_count
      << ",\"first_buffer_epoch\":" << audit.first_buffer_epoch
      << ",\"resident_traversal_host_synchronization_count\":"
      << audit.host_synchronization_count
      << ",\"initial_frontier_h2d_byte_count\":"
      << audit.initial_frontier_h2d_byte_count
      << ",\"input_product_count\":" << audit.input_product_count
      << ",\"keep_certificate_product_count\":"
      << audit.keep_certificate_product_count
      << ",\"last_buffer_epoch\":" << audit.last_buffer_epoch
      << ",\"output_terminal_count\":"
      << audit.output_terminal_count
      << ",\"output_receipt_count\":" << audit.output_receipt_count
      << ",\"peak_frontier_count\":" << audit.peak_frontier_count
      << ",\"physical_terminal_d2h_byte_count\":"
      << audit.physical_terminal_d2h_byte_count
      << ",\"physical_receipt_d2h_byte_count\":"
      << audit.physical_receipt_d2h_byte_count
      << ",\"prune_product_count\":"
      << audit.prune_product_count
      << ",\"proposed_product_count\":"
      << audit.proposed_product_count
      << ",\"receipt_capacity_exhausted\":";
  write_bool(output, audit.receipt_capacity_exhausted);
  output
      << ",\"receipt_digest_fold_fnv1a\":"
      << audit.receipt_digest_fold_fnv1a
      << ",\"required_strict_interior_point_count\":"
      << audit.required_strict_interior_point_count
      << ",\"snapshot_h2d_byte_count\":"
      << audit.snapshot_h2d_byte_count
      << ",\"stackless_kernel_launch_count\":"
      << audit.stackless_kernel_launch_count
      << ",\"stackless_single_product_traversal_used\":";
  write_bool(
      output, audit.stackless_single_product_traversal_used);
  output
      << ",\"stackless_product_batch_traversal_used\":";
  write_bool(
      output, audit.stackless_product_batch_traversal_used);
  output
      << ",\"strict_interior_terminal_count\":"
      << audit.strict_interior_terminal_count
      << ",\"terminal_capacity_exhausted\":";
  write_bool(output, audit.receipt_capacity_exhausted);
  output
      << ",\"terminal_digest_fold_fnv1a\":"
      << audit.terminal_digest_fold_fnv1a
      << ",\"traversal_epoch_count\":"
      << audit.traversal_epoch_count
      << ",\"traversal_metadata_d2h_byte_count\":"
      << audit.traversal_metadata_d2h_byte_count
      << ",\"traversal_backend\":\""
      << traversal_backend_text(audit.traversal_backend) << '"'
      << ",\"unresolved_external_leaf_terminal_count\":"
      << audit.unresolved_external_leaf_terminal_count
      << ",\"visited_work_item_count\":"
      << audit.visited_work_item_count
      << ",\"visit_budget_count\":" << audit.visit_budget_count
      << ",\"visit_budget_exhausted\":";
  write_bool(output, audit.visit_budget_exhausted);
  output
      << ",\"work_item_capacity_exhausted\":";
  write_bool(output, audit.work_item_capacity_exhausted);
  output << '}';
}

void write_proposal_audit(
    std::ostream& output,
    const morsehgp3d::hierarchy::
        ExactPairSupportRankPruneProposalAudit& audit) {
  output
      << "{\"batch_request_count\":" << audit.batch_request_count
      << ",\"cache_replacement_count\":"
      << audit.cache_replacement_count
      << ",\"consumed_keep_certificate_count\":"
      << audit.consumed_keep_certificate_count
      << ",\"consumed_proposal_count\":"
      << audit.consumed_proposal_count
      << ",\"consumed_receipt_count\":"
      << audit.consumed_receipt_count
      << ",\"consumed_witness_point_count\":"
      << audit.consumed_witness_point_count
      << ",\"cpu_fallback_product_count\":"
      << audit.cpu_fallback_product_count
      << ",\"durable_wire_v1_claimed\":";
  write_bool(output, audit.durable_wire_v1_claimed);
  output
      << ",\"exact_keep_terminal_recertification_count\":"
      << audit.exact_keep_terminal_recertification_count
      << ",\"exact_receipt_recertification_count\":"
      << audit.exact_receipt_recertification_count
      << ",\"keep_anchor_noninterior_point_count\":"
      << audit.keep_anchor_noninterior_point_count
      << ",\"keep_anchor_noninterior_terminal_count\":"
      << audit.keep_anchor_noninterior_terminal_count
      << ",\"keep_external_leaf_terminal_count\":"
      << audit.keep_external_leaf_terminal_count
      << ",\"keep_strict_interior_point_count\":"
      << audit.keep_strict_interior_point_count
      << ",\"keep_strict_interior_terminal_count\":"
      << audit.keep_strict_interior_terminal_count
      << ",\"keep_terminal_classification_count\":"
      << audit.keep_terminal_classification_count
      << ",\"maximum_products_per_batch\":"
      << audit.maximum_products_per_batch
      << ",\"maximum_requested_batch_product_count\":"
      << audit.maximum_requested_batch_product_count
      << ",\"no_forbidden_global_structure_materialized\":";
  write_bool(
      output, audit.no_forbidden_global_structure_materialized);
  output
      << ",\"proposal_pruned_pair_count\":"
      << audit.proposal_pruned_pair_count
      << ",\"requested_product_count\":"
      << audit.requested_product_count
      << ",\"transcript_ephemeral\":";
  write_bool(output, audit.transcript_ephemeral);
  output << '}';
}

void write_chunk_summary(
    std::ostream& output,
    const ExactPairSupportStreamChunk& chunk) {
  output
      << "{\"candidate_prepared\":";
  write_bool(output, chunk.candidate_prepared);
  output
      << ",\"checkpoint_digest\":\""
      << chunk.next_checkpoint.checkpoint_digest.to_lower_hex()
      << "\",\"event_count\":" << chunk.events.size()
      << ",\"extra_shell_diagnostic_count\":"
      << chunk.relevant_extra_shell_diagnostics.size()
      << ",\"hierarchy_reduction_performed\":";
  write_bool(output, chunk.hierarchy_reduction_performed);
  output
      << ",\"no_forbidden_global_structure_materialized\":";
  write_bool(
      output, chunk.no_forbidden_global_structure_materialized);
  output
      << ",\"output_chain_digest\":\""
      << chunk.output_chain_digest.to_lower_hex()
      << "\",\"status\":\"" << status_text(chunk.status)
      << "\",\"stop_reason\":\""
      << stop_reason_text(chunk.stop_reason)
      << "\",\"stream_audit\":";
  write_stream_audit(output, chunk.cumulative_audit_after);
  output << '}';
}

void write_verification(
    std::ostream& output,
    const ExactPairSupportRankPruneProposedChunkVerification&
        verification) {
  const auto& candidate = verification.candidate_chunk_verification;
  output
      << "{\"candidate_chunk_verification\":{"
      << "\"chunk_transition_verified\":";
  write_bool(output, candidate.chunk_transition_verified);
  output << ",\"fresh_replay_certified\":";
  write_bool(output, candidate.fresh_replay_certified);
  output << ",\"next_checkpoint_integrity_verified\":";
  write_bool(output, candidate.next_checkpoint_integrity_verified);
  output << ",\"prepared_transition_chain_matches\":";
  write_bool(output, candidate.prepared_transition_chain_matches);
  output << ",\"records_individually_exact\":";
  write_bool(output, candidate.records_individually_exact);
  output << ",\"requested_budget_certified\":";
  write_bool(output, candidate.requested_budget_certified);
  output << ",\"source_checkpoint_integrity_verified\":";
  write_bool(output, candidate.source_checkpoint_integrity_verified);
  output
      << "},\"durable_wire_v1_claimed\":";
  write_bool(output, verification.durable_wire_v1_claimed);
  output << ",\"every_consumed_proposal_replayed_once\":";
  write_bool(
      output, verification.every_consumed_proposal_replayed_once);
  output
      << ",\"every_consumed_keep_certificate_replayed_once\":";
  write_bool(
      output,
      verification.every_consumed_keep_certificate_replayed_once);
  output << ",\"keep_candidate_transition_consistent\":";
  write_bool(
      output, verification.keep_candidate_transition_consistent);
  output << ",\"no_duplicate_keep_certificate_retained\":";
  write_bool(
      output, verification.no_duplicate_keep_certificate_retained);
  output << ",\"no_unconsumed_keep_certificate_retained\":";
  write_bool(
      output,
      verification.no_unconsumed_keep_certificate_retained);
  output << ",\"no_unconsumed_proposal_retained\":";
  write_bool(
      output, verification.no_unconsumed_proposal_retained);
  output << ",\"proposed_transition_verified\":";
  write_bool(output, verification.proposed_transition_verified);
  output << '}';
}

}  // namespace

int main(int argument_count, char** argument_values) {
  try {
    const Options options =
        parse_options(argument_count, argument_values);
    const auto generator =
        options.family == "uniform_latin"
            ? smoke_clouds::uniform_latin_points
            : smoke_clouds::eight_clusters_points;

    const Clock::time_point generation_start = Clock::now();
    const std::vector<CertifiedPoint3> input =
        generator(options.point_count);
    const Clock::time_point generation_end = Clock::now();

    const Clock::time_point canonicalization_start = Clock::now();
    const CanonicalPointCloud cloud =
        CanonicalPointCloud::rejecting_duplicates(
            std::span<const CertifiedPoint3>{input});
    const Clock::time_point canonicalization_end = Clock::now();

    const Clock::time_point lbvh_start = Clock::now();
    const MortonLbvhIndex index = MortonLbvhIndex::build(cloud);
    const Clock::time_point lbvh_end = Clock::now();

    const Clock::time_point authority_start = Clock::now();
    const ExactPairSupportAuthorityContext authority{
        index, cloud, kRequestedMaximumOrder};
    const ExactPairSupportCheckpoint source_checkpoint =
        morsehgp3d::hierarchy::
            make_initial_exact_pair_support_checkpoint(authority);
    const Clock::time_point authority_end = Clock::now();
    const ExactPairSupportStreamBudget stream_budget =
        make_stream_budget(options);

    const Clock::time_point context_start = Clock::now();
    PairSupportPhiContext gpu_context{
        index,
        cloud,
        kLegacyP1QueryCapacity,
        options.capacity};
    const Clock::time_point context_end = Clock::now();

    const Clock::time_point baseline_start = Clock::now();
    const ExactPairSupportStreamChunk baseline =
        morsehgp3d::hierarchy::build_exact_pair_support_stream_chunk(
            authority, stream_budget, source_checkpoint);
    const Clock::time_point baseline_end = Clock::now();

    AssistedMeasurement first_assisted = run_assisted(
        authority,
        stream_budget,
        source_checkpoint,
        gpu_context,
        options);
    require(
        first_assisted.gpu_audit.snapshot_h2d_byte_count ==
            checked_multiply(
                gpu_context.node_count(),
                kResidentLbvhNodeByteCount,
                "the resident LBVH snapshot byte count overflows"),
        "the first P4 pass did not upload exactly one immutable LBVH "
        "snapshot");
    const bool stackless_expected =
        gpu_context.node_count() <=
            static_cast<std::size_t>(
                std::numeric_limits<std::uint32_t>::max());
    const bool stackless_single_expected =
        stackless_expected &&
        options.capacity.maximum_product_count == 1U;
    const bool stackless_batch_expected =
        stackless_expected &&
        options.capacity.maximum_product_count > 1U;
    const std::size_t expected_escape_snapshot_bytes =
        stackless_expected
            ? checked_multiply(
                  gpu_context.node_count(),
                  sizeof(std::uint32_t),
                  "the resident stackless escape snapshot byte count "
                  "overflows")
            : 0U;
    require(
        first_assisted.gpu_audit.escape_snapshot_h2d_byte_count ==
                expected_escape_snapshot_bytes &&
            first_assisted.gpu_audit
                    .device_escape_snapshot_byte_capacity ==
                expected_escape_snapshot_bytes &&
            first_assisted.gpu_audit
                    .stackless_single_product_traversal_used ==
                stackless_single_expected &&
            first_assisted.gpu_audit
                    .stackless_product_batch_traversal_used ==
                stackless_batch_expected,
        "the first stackless pass did not separate the 4N escape snapshot from "
        "the shared 80N LBVH snapshot");
    std::vector<AssistedMeasurement> resident_replays;
    resident_replays.reserve(options.resident_replay_count);
    for (std::size_t replay_index = 0U;
         replay_index < options.resident_replay_count;
         ++replay_index) {
      AssistedMeasurement replay = run_assisted(
          authority,
          stream_budget,
          source_checkpoint,
          gpu_context,
          options);
      require(
          replay.proposed == first_assisted.proposed,
          "a resident P4 replay changed its proposed CPU transition");
      require(
          stable_gpu_audit_equal(
              replay.gpu_audit, first_assisted.gpu_audit),
          "a resident P4 replay changed its stable GPU audit");
      require(
          replay.gpu_audit.snapshot_h2d_byte_count == 0U,
          "a resident P4 replay repeated the immutable LBVH snapshot upload");
      require(
          replay.gpu_audit.escape_snapshot_h2d_byte_count == 0U,
          "a resident P5a replay repeated the immutable escape snapshot "
          "upload");
      resident_replays.push_back(std::move(replay));
    }

    const Clock::time_point verification_start = Clock::now();
    const ExactPairSupportRankPruneProposedChunkVerification verification =
        morsehgp3d::hierarchy::
            verify_exact_pair_support_stream_proposed_chunk(
                authority,
                stream_budget,
                source_checkpoint,
                options.capacity.maximum_product_count,
                first_assisted.proposed);
    const Clock::time_point verification_end = Clock::now();
    const auto& candidate_verification =
        verification.candidate_chunk_verification;
    require(
        verification.proposed_transition_verified &&
            verification.every_consumed_proposal_replayed_once &&
            verification.no_unconsumed_proposal_retained &&
            verification
                .every_consumed_keep_certificate_replayed_once &&
            verification
                .no_unconsumed_keep_certificate_retained &&
            verification.no_duplicate_keep_certificate_retained &&
            verification.keep_candidate_transition_consistent &&
            candidate_verification.source_checkpoint_integrity_verified &&
            candidate_verification.next_checkpoint_integrity_verified &&
            candidate_verification.requested_budget_certified &&
            candidate_verification.records_individually_exact &&
            candidate_verification.prepared_transition_chain_matches &&
            candidate_verification.chunk_transition_verified &&
            candidate_verification.fresh_replay_certified &&
            !verification.durable_wire_v1_claimed,
        "the fresh ephemeral CPU verification rejected the P4 "
        "prune/keep transition");

    const bool historical_chunk_equal =
        first_assisted.proposed.candidate_chunk == baseline;
    const bool historical_checkpoint_digest_equal =
        first_assisted.proposed.candidate_chunk.next_checkpoint
            .checkpoint_digest ==
        baseline.next_checkpoint.checkpoint_digest;

    std::cout
        << "{\"artifact_role\":"
           "\"pair_rank_prune_component_smoke\","
        << "\"backend\":\"cuda_g4_plus_reference_cpu\","
        << "\"benchmark_output_contract_v1_satisfied\":false,"
        << "\"anchor_ball_culling_enabled\":true,"
        << "\"component_only\":true,"
        << "\"component_success\":true,"
        << "\"durable_checkpoint_wire_exercised\":false,"
        << "\"durable_wire_v1_claimed\":false,"
        << "\"family\":\"" << options.family << "\","
        << "\"full_cloud_hierarchy_executed\":false,"
        << "\"full_hierarchy_executed\":false,"
        << "\"full_pipeline_executed\":false,"
        << "\"git_sha\":\"" << MORSEHGP3D_GIT_SHA << "\","
        << "\"gpu_terminal_kinds_authoritative\":false,"
        << "\"hierarchy_reduction_performed\":false,"
        << "\"higher_support_arities_3_4_executed\":false,"
        << "\"legacy_p1_query_capacity\":"
        << kLegacyP1QueryCapacity << ','
        << "\"massive_product_path_claimed\":false,"
        << "\"mode\":"
           "\"prune_or_keep_terminal_proposal_then_cpu_exact_recertified\","
        << "\"morse_batches_executed\":false,"
        << "\"phase\":\"14Q\","
        << "\"point_count\":" << options.point_count << ','
        << "\"profile\":\"hgp_reduced\","
        << "\"public_status\":null,"
        << "\"qualification_claimed\":false,"
        << "\"requested_maximum_order\":"
        << kRequestedMaximumOrder << ','
        << "\"resident_replay_count\":"
        << options.resident_replay_count << ','
        << "\"result_materialized\":false,"
        << "\"rank_prune_and_keep_exercised\":true,"
        << "\"scalability_claimed\":false,"
        << "\"schema\":"
           "\"morsehgp3d.phase14q.pair_rank_search_component_smoke.v3\","
        << "\"scientific_public_result_claimed\":false,"
        << "\"slo_claimed\":false,"
        << "\"support_sizes_exercised\":[2],"
        << "\"terminal_coverage_recertified\":true,"
        << "\"ten_million_product_path_claimed\":false,"
        << "\"ten_orders_materialized\":false,"
        << "\"terminal_facade_built\":false,"
        << "\"timing_class\":\"component_only\","
        << "\"warm_e2e_measured\":false,"
        << "\"warm_e2e_protocol_executed\":false,"
        << "\"warm_e2e_slo_claimed\":false,"
        << "\"budgets\":{\"epoch_count\":"
        << options.epoch_budget.maximum_epoch_count
        << ",\"maximum_auxiliary_frontier_entry_count\":"
        << stream_budget.maximum_auxiliary_frontier_entry_count
        << ",\"maximum_closed_ball_query_count\":"
        << stream_budget.maximum_global_closed_ball_query_count
        << ",\"maximum_emitted_point_id_reference_count\":"
        << stream_budget.maximum_emitted_point_id_reference_count
        << ",\"maximum_emitted_record_count\":"
        << stream_budget.maximum_emitted_record_count
        << ",\"maximum_frontier_entry_count\":"
        << stream_budget.maximum_frontier_entry_count
        << ",\"maximum_closed_ball_node_visit_count\":"
        << stream_budget.maximum_closed_ball_node_visit_count
        << ",\"maximum_work_unit_count\":"
        << stream_budget.maximum_work_unit_count
        << ",\"product_capacity\":"
        << options.capacity.maximum_product_count
        << ",\"receipt_capacity\":"
        << options.capacity.maximum_receipt_count
        << ",\"terminal_capacity\":"
        << options.capacity.maximum_receipt_count
        << ",\"work_item_capacity\":"
        << options.capacity.maximum_work_item_count << "},"
        << "\"timings_ns\":{\"assisted_first\":"
        << first_assisted.elapsed_nanoseconds
        << ",\"authority_and_initial_checkpoint\":"
        << nanoseconds(authority_end - authority_start)
        << ",\"canonicalization\":"
        << nanoseconds(
               canonicalization_end - canonicalization_start)
        << ",\"fresh_ephemeral_verification\":"
        << nanoseconds(verification_end - verification_start)
        << ",\"generation\":"
        << nanoseconds(generation_end - generation_start)
        << ",\"gpu_context_construction\":"
        << nanoseconds(context_end - context_start)
        << ",\"historical_reference_cpu_chunk\":"
        << nanoseconds(baseline_end - baseline_start)
        << ",\"lbvh\":"
        << nanoseconds(lbvh_end - lbvh_start)
        << ",\"resident_replays\":[";
    for (std::size_t index_value = 0U;
         index_value < resident_replays.size();
         ++index_value) {
      if (index_value != 0U) {
        std::cout << ',';
      }
      std::cout
          << resident_replays[index_value].elapsed_nanoseconds;
    }
    std::cout << "]},\"historical_reference_cpu\":";
    write_chunk_summary(std::cout, baseline);
    std::cout << ",\"assisted_first\":{\"candidate_chunk\":";
    write_chunk_summary(
        std::cout, first_assisted.proposed.candidate_chunk);
    std::cout << ",\"gpu_audit\":";
    write_gpu_audit(std::cout, first_assisted.gpu_audit);
    std::cout << ",\"proposal_audit\":";
    write_proposal_audit(std::cout, first_assisted.proposed.audit);
    std::cout
        << "},\"historical_comparison\":{"
           "\"candidate_chunk_equal\":";
    write_bool(std::cout, historical_chunk_equal);
    std::cout << ",\"checkpoint_digest_equal\":";
    write_bool(std::cout, historical_checkpoint_digest_equal);
    std::cout
        << ",\"equality_required_for_success\":false},"
        << "\"resident_replays\":[";
    for (std::size_t index_value = 0U;
         index_value < resident_replays.size();
         ++index_value) {
      if (index_value != 0U) {
        std::cout << ',';
      }
      std::cout << "{\"gpu_audit\":";
      write_gpu_audit(
          std::cout, resident_replays[index_value].gpu_audit);
      std::cout << ",\"proposed_transition_equal\":true}";
    }
    std::cout << "],\"fresh_ephemeral_verification\":";
    write_verification(std::cout, verification);
    std::cout
        << ",\"forbidden_global_structure_materialized\":false,"
        << "\"global_support_product_prune_published_by_gpu\":false,"
        << "\"public_status_published_by_gpu\":false}\n";
    return EXIT_SUCCESS;
  } catch (const std::exception& error) {
    std::cerr
        << "Phase 14Q pair rank-search component smoke failed: "
        << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
