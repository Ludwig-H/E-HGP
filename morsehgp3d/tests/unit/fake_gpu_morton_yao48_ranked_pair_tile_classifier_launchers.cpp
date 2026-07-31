#include "fake_gpu_morton_yao48_ranked_pair_tile_classifier_launchers.hpp"

#include "phase15_morton_yao48_ranked_pair_tile_classifier_internal.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::test_support {
namespace {

std::mutex fake_mutex;
FakeMortonYao48RankedPairTileClassifierConfiguration fake_configuration;
std::size_t fake_next_launch{};
std::atomic<std::size_t> fake_launch_count{0U};
std::atomic<std::size_t> fake_live_lease_count{0U};

}  // namespace

void configure_fake_gpu_morton_yao48_ranked_pair_tile_classifier(
    FakeMortonYao48RankedPairTileClassifierConfiguration configuration) {
  std::lock_guard<std::mutex> lock{fake_mutex};
  fake_configuration = std::move(configuration);
  fake_next_launch = 0U;
}

void reset_fake_gpu_morton_yao48_ranked_pair_tile_classifier() noexcept {
  std::lock_guard<std::mutex> lock{fake_mutex};
  fake_configuration = {};
  fake_next_launch = 0U;
  fake_launch_count.store(0U, std::memory_order_relaxed);
  fake_live_lease_count.store(0U, std::memory_order_relaxed);
}

std::size_t
fake_gpu_morton_yao48_ranked_pair_tile_classifier_launch_count() noexcept {
  return fake_launch_count.load(std::memory_order_relaxed);
}

std::size_t
fake_gpu_morton_yao48_ranked_pair_tile_classifier_live_lease_count()
    noexcept {
  return fake_live_lease_count.load(std::memory_order_relaxed);
}

}  // namespace morsehgp3d::gpu::test_support

namespace morsehgp3d::gpu::detail {
namespace {

using Corruption = test_support::
    FakeMortonYao48RankedPairTileClassifierCorruption;
using FakeLaunch =
    test_support::FakeMortonYao48RankedPairTileClassifierLaunch;

struct FakeResidentRankedPairCatalog final {
  std::vector<std::uint64_t> support_u;
  std::vector<std::uint64_t> support_v;
  std::vector<std::uint8_t> closed_rank;
  std::vector<std::uint64_t> rank_offsets;
  std::vector<std::uint64_t> strict_offsets;
  std::vector<std::uint64_t> strict_point_ids;
  std::vector<std::uint64_t> shell_offsets;
  std::vector<std::uint64_t> shell_point_ids;
};

[[nodiscard]] FakeLaunch next_launch() {
  std::lock_guard<std::mutex> lock{test_support::fake_mutex};
  if (test_support::fake_next_launch >=
      test_support::fake_configuration.launches.size()) {
    throw std::runtime_error(
        "the fake resident ranked-pair classifier transcript is exhausted");
  }
  return test_support::fake_configuration
      .launches[test_support::fake_next_launch++];
}

[[nodiscard]] std::uint64_t checked_sum(
    std::uint64_t left,
    std::uint64_t right,
    const char* message) {
  if (right > std::numeric_limits<std::uint64_t>::max() - left) {
    throw std::length_error(message);
  }
  return left + right;
}

[[nodiscard]] std::size_t checked_size(
    std::uint64_t value,
    const char* message) {
  if (value > static_cast<std::uint64_t>(
                  std::numeric_limits<std::size_t>::max())) {
    throw std::length_error(message);
  }
  return static_cast<std::size_t>(value);
}

[[nodiscard]] Phase15MortonYao48RankedPairTileClassifierDeviceOutput
make_output(
    Phase15MortonYao48RankedPairTileClassifierContextState& context,
    const Phase15MortonYao48RankedPairTileClassifierRequest& request,
    std::uint64_t record_count,
    std::uint64_t strict_count,
    std::uint64_t shell_count) {
  auto storage = std::make_shared<FakeResidentRankedPairCatalog>();
  const std::size_t records = checked_size(
      record_count, "the fake catalog record count does not fit size_t");
  const std::size_t strict = checked_size(
      strict_count, "the fake strict payload count does not fit size_t");
  const std::size_t shell = checked_size(
      shell_count, "the fake shell payload count does not fit size_t");
  const std::size_t total_payload = checked_size(
      checked_sum(
          strict_count,
          shell_count,
          "the fake total payload count overflowed"),
      "the fake total payload count does not fit size_t");
  const std::size_t maximum_payload_per_record =
      request.point_count < 2U
          ? 0U
          : std::min(
                request.fixed_config.maximum_closed_rank - 2U,
                request.point_count - 2U);
  const std::size_t maximum_payload =
      maximum_payload_per_record != 0U &&
              records > std::numeric_limits<std::size_t>::max() /
                            maximum_payload_per_record
          ? std::numeric_limits<std::size_t>::max()
          : records * maximum_payload_per_record;
  if (total_payload > maximum_payload) {
    throw std::invalid_argument(
        "the fake catalog payload mass cannot match its closed-rank window");
  }

  struct RecordShape final {
    std::size_t strict_count{};
    std::size_t shell_count{};
  };
  std::vector<RecordShape> shapes(records);
  const auto distribute = [&](std::size_t count, bool is_strict) {
    std::size_t remaining = count;
    for (auto& shape : shapes) {
      const std::size_t occupied =
          shape.strict_count + shape.shell_count;
      const std::size_t assigned = std::min(
          remaining, maximum_payload_per_record - occupied);
      if (is_strict) {
        shape.strict_count += assigned;
      } else {
        shape.shell_count += assigned;
      }
      remaining -= assigned;
    }
    if (remaining != 0U) {
      throw std::logic_error(
          "the fake catalog payload distributor lost capacity");
    }
  };
  distribute(strict, true);
  distribute(shell, false);
  std::stable_sort(
      shapes.begin(),
      shapes.end(),
      [](const RecordShape& left, const RecordShape& right) {
        return left.strict_count + left.shell_count <
               right.strict_count + right.shell_count;
      });

  storage->support_u.reserve(records);
  storage->support_v.reserve(records);
  storage->closed_rank.reserve(records);
  storage->strict_offsets.reserve(records + 1U);
  storage->strict_offsets.push_back(0U);
  storage->strict_point_ids.reserve(strict);
  storage->shell_offsets.reserve(records + 1U);
  storage->shell_offsets.push_back(0U);
  storage->shell_point_ids.reserve(shell);

  std::size_t support_u = 0U;
  std::size_t support_v = 1U;
  for (const auto& shape : shapes) {
    if (support_u >= request.point_count ||
        support_v >= request.point_count) {
      throw std::invalid_argument(
          "the fake catalog contains more records than support pairs");
    }
    storage->support_u.push_back(static_cast<std::uint64_t>(support_u));
    storage->support_v.push_back(static_cast<std::uint64_t>(support_v));
    const std::size_t payload_count =
        shape.strict_count + shape.shell_count;
    storage->closed_rank.push_back(
        static_cast<std::uint8_t>(payload_count + 2U));

    std::size_t selected = 0U;
    for (std::size_t point_id = 0U;
         point_id < request.point_count && selected < payload_count;
         ++point_id) {
      if (point_id == support_u || point_id == support_v) {
        continue;
      }
      if (selected < shape.strict_count) {
        storage->strict_point_ids.push_back(
            static_cast<std::uint64_t>(point_id));
      } else {
        storage->shell_point_ids.push_back(
            static_cast<std::uint64_t>(point_id));
      }
      ++selected;
    }
    if (selected != payload_count) {
      throw std::logic_error(
          "the fake catalog could not assign distinct non-support points");
    }
    storage->strict_offsets.push_back(
        static_cast<std::uint64_t>(storage->strict_point_ids.size()));
    storage->shell_offsets.push_back(
        static_cast<std::uint64_t>(storage->shell_point_ids.size()));

    ++support_v;
    if (support_v == request.point_count) {
      ++support_u;
      support_v = support_u + 1U;
    }
  }
  if (storage->strict_point_ids.size() != strict ||
      storage->shell_point_ids.size() != shell) {
    throw std::logic_error(
        "the fake catalog payload totals do not match their transcript");
  }

  storage->rank_offsets.resize(
      request.fixed_config.maximum_closed_rank + 2U);
  std::size_t rank_cursor = 0U;
  for (std::size_t rank = 0U;
       rank <= request.fixed_config.maximum_closed_rank + 1U;
       ++rank) {
    while (rank_cursor < records &&
           storage->closed_rank[rank_cursor] < rank) {
      ++rank_cursor;
    }
    storage->rank_offsets[rank] =
        static_cast<std::uint64_t>(rank_cursor);
  }
  context.replace_launcher_state(storage);

  Phase15MortonYao48RankedPairTileClassifierDeviceOutput output;
  output.owner = storage;
  output.support_u = storage->support_u.data();
  output.support_v = storage->support_v.data();
  output.closed_rank = storage->closed_rank.data();
  output.rank_offsets = storage->rank_offsets.data();
  output.strict_offsets = storage->strict_offsets.data();
  output.strict_point_ids = storage->strict_point_ids.data();
  output.shell_offsets = storage->shell_offsets.data();
  output.shell_point_ids = storage->shell_point_ids.data();
  output.catalog_record_capacity =
      request.fixed_config.catalog_record_capacity;
  output.payload_point_id_capacity =
      request.fixed_config.payload_point_id_capacity;
  output.exact_fallback_capacity =
      request.fixed_config.exact_fallback_capacity;
  output.catalog_record_count = records;
  output.strict_point_id_count = strict;
  output.shell_point_id_count = shell;
  output.rank_offset_count =
      request.fixed_config.maximum_closed_rank + 2U;
  output.record_offset_count = records + 1U;
  return output;
}

void apply_corruption(
    Phase15MortonYao48RankedPairTileClassifierReceipt& receipt,
    Corruption corruption) {
  switch (corruption) {
    case Corruption::none:
    case Corruption::corrupt_receipt_digest:
    case Corruption::simulated_launcher_failure:
      return;
    case Corruption::stale_source_epoch:
      --receipt.source_snapshot_epoch;
      return;
    case Corruption::stale_candidate_epoch:
      --receipt.candidate_buffer_epoch;
      return;
    case Corruption::stale_catalog_epoch:
      --receipt.catalog_buffer_epoch;
      return;
    case Corruption::stale_tile_epoch:
      --receipt.tile_epoch;
      return;
    case Corruption::stale_chunk_sequence:
      --receipt.chunk_sequence;
      return;
    case Corruption::cumulative_record_rollback:
      receipt.cumulative_accepted_record_count =
          receipt.cumulative_accepted_record_count == 0U
              ? 1U
              : receipt.cumulative_accepted_record_count - 1U;
      return;
    case Corruption::wrong_candidate_mass:
      ++receipt.transaction_candidate_count;
      return;
    case Corruption::candidate_device_to_host:
      receipt.candidate_device_to_host_count = 1U;
      receipt.candidate_device_to_host_performed = true;
      return;
    case Corruption::certified_prune_device_to_host:
      receipt.certified_prune_device_to_host_count = 1U;
      receipt.certified_prune_device_to_host_performed = true;
      return;
    case Corruption::callback_per_pair:
      receipt.legacy_pair_callback_count = 1U;
      receipt.callback_per_pair_performed = true;
      return;
    case Corruption::dense_pair_scan:
      receipt.dense_pair_scan_performed = true;
      return;
    case Corruption::missing_source_authentication:
      receipt.source_identity_authenticated = false;
      return;
    case Corruption::missing_output_owner:
      receipt.output.owner.reset();
      return;
    case Corruption::malformed_output_layout:
      ++receipt.output.record_offset_count;
      return;
    case Corruption::invalid_closed_rank:
      if (receipt.output.catalog_record_count != 0U) {
        const_cast<std::uint8_t*>(receipt.output.closed_rank)[0U] = 1U;
      }
      return;
    case Corruption::unsorted_record_order:
      if (receipt.output.catalog_record_count > 1U) {
        std::swap(
            const_cast<std::uint64_t*>(receipt.output.support_v)[0U],
            const_cast<std::uint64_t*>(receipt.output.support_v)[1U]);
      }
      return;
    case Corruption::invalid_rank_offsets:
      if (receipt.output.rank_offset_count != 0U) {
        const_cast<std::uint64_t*>(receipt.output.rank_offsets)[0U] = 1U;
      }
      return;
    case Corruption::invalid_payload_offsets:
      if (receipt.output.record_offset_count != 0U) {
        ++const_cast<std::uint64_t*>(receipt.output.strict_offsets)
              [receipt.output.catalog_record_count];
      }
      return;
    case Corruption::invalid_payload_point_id:
      if (receipt.output.strict_point_id_count != 0U) {
        const_cast<std::uint64_t*>(receipt.output.strict_point_ids)[0U] =
            receipt.point_count;
      } else if (receipt.output.shell_point_id_count != 0U) {
        const_cast<std::uint64_t*>(receipt.output.shell_point_ids)[0U] =
            receipt.point_count;
      }
      return;
  }
}

}  // namespace

Phase15MortonYao48RankedPairTileClassifierReceipt
classify_phase15_morton_yao48_ranked_pair_tile_on_device(
    Phase15MortonYao48RankedPairTileClassifierContextState& context,
    const Phase15MortonYao48DeviceCandidateTilePrivateViews& candidate_views,
    const Phase15MortonYao48RankedPairTileClassifierRequest& request) {
  test_support::fake_launch_count.fetch_add(1U, std::memory_order_relaxed);
  const FakeLaunch launch = next_launch();
  if (launch.corruption == Corruption::simulated_launcher_failure) {
    throw std::runtime_error(
        "simulated resident ranked-pair classifier launcher failure");
  }
  if (!context.config_matches(request.fixed_config) ||
      !candidate_views.ready || !candidate_views.host_fake ||
      candidate_views.retained_authority_identity == nullptr ||
      candidate_views.source_owner_identity == nullptr ||
      candidate_views.source_cloud_identity == nullptr ||
      candidate_views.device_coordinate_bits != nullptr ||
      candidate_views.device_morton_point_ids != nullptr ||
      candidate_views.device_nodes != nullptr ||
      candidate_views.device_candidate_records != nullptr ||
      candidate_views.device_prune_regions != nullptr ||
      candidate_views.device_anchor_controls != nullptr ||
      candidate_views.cuda_device != -1 ||
      request.source_snapshot_epoch !=
          candidate_views.source_snapshot_epoch ||
      request.candidate_buffer_epoch !=
          candidate_views.candidate_buffer_epoch ||
      request.point_count != candidate_views.point_count ||
      request.certified_node_count != candidate_views.certified_node_count ||
      request.anchor_begin != candidate_views.anchor_begin ||
      request.anchor_end != candidate_views.anchor_end ||
      request.fixed_config.maximum_closed_rank <
          morton_yao48_ranked_pair_tile_classifier_minimum_closed_rank ||
      request.fixed_config.maximum_closed_rank >
          morton_yao48_ranked_pair_tile_classifier_maximum_closed_rank ||
      candidate_views.prune_semantics !=
          MortonYao48DeviceTiledPairFrontierPruneSemantics::
              closed_rank_window ||
      candidate_views.required_witness_count !=
          request.fixed_config.maximum_closed_rank - 1U ||
      request.catalog_buffer_epoch == 0U || request.tile_epoch == 0U ||
      request.chunk_sequence == 0U ||
      launch.exact_fallback_count > request.candidate_count ||
      checked_sum(
          checked_sum(
              launch.accepted_record_count,
              launch.above_window_count,
              "the fake classified candidate partition overflowed"),
          launch.exact_fallback_count,
          "the fake fallback candidate partition overflowed") !=
          request.candidate_count) {
    throw std::invalid_argument(
        "the fake resident ranked-pair classifier received malformed "
        "authority, request, or transcript");
  }

  test_support::fake_live_lease_count.store(1U, std::memory_order_relaxed);
  struct LiveLeaseReset final {
    ~LiveLeaseReset() {
      test_support::fake_live_lease_count.store(
          0U, std::memory_order_relaxed);
    }
  } live_lease_reset;

  Phase15MortonYao48RankedPairTileClassifierReceipt receipt;
  receipt.fixed_config = request.fixed_config;
  receipt.source_snapshot_epoch = request.source_snapshot_epoch;
  receipt.candidate_buffer_epoch = request.candidate_buffer_epoch;
  receipt.catalog_buffer_epoch = request.catalog_buffer_epoch;
  receipt.tile_epoch = request.tile_epoch;
  receipt.chunk_sequence = request.chunk_sequence;
  receipt.point_count = request.point_count;
  receipt.certified_node_count = request.certified_node_count;
  receipt.anchor_begin = request.anchor_begin;
  receipt.anchor_end = request.anchor_end;
  receipt.transaction_candidate_count = request.candidate_count;
  receipt.transaction_certified_pruned_pair_mass =
      request.transaction_certified_pruned_pair_mass;
  receipt.cumulative_certified_pruned_pair_mass =
      request.cumulative_certified_pruned_pair_mass;
  receipt.unordered_pair_universe_count =
      request.unordered_pair_universe_count;
  receipt.frontier_unresolved_pair_mass =
      request.frontier_unresolved_pair_mass;
  receipt.launcher_call_count = 1U;
  receipt.kernel_launch_count = 1U;
  receipt.synchronization_count = 1U;
  receipt.cuda_device = -1;
  receipt.execution_kind =
      Phase15MortonYao48RankedPairTileClassifierExecutionKind::host_fake;
  receipt.same_tile_continuation = request.same_tile_continuation;
  receipt.tile_closed_after_chunk = request.tile_closed_after_chunk;
  receipt.frontier_closed_after_chunk = request.frontier_closed_after_chunk;
  receipt.source_identity_authenticated = true;
  receipt.source_epoch_authenticated = true;
  receipt.candidate_epoch_authenticated = true;
  receipt.tile_chunk_sequence_authenticated = true;
  receipt.candidate_tile_lease_alive_during_launch = true;
  receipt.fixed_linear_capacities_validated = true;
  receipt.exact_multiorder_classification_implemented = true;
  receipt.one_multiorder_launch_used = true;
  receipt.count_scan_payload_layout_validated = true;
  receipt.output_records_sorted_unique = true;
  receipt.output_rollback_validated = true;

  receipt.required_catalog_record_count = checked_sum(
      request.prior_accepted_record_count,
      launch.accepted_record_count,
      "the fake required record count overflowed");
  receipt.required_payload_point_id_count = checked_sum(
      checked_sum(
          request.prior_strict_payload_point_id_count,
          request.prior_shell_payload_point_id_count,
          "the fake prior payload count overflowed"),
      checked_sum(
          launch.strict_payload_point_id_count,
          launch.shell_payload_point_id_count,
          "the fake transaction payload count overflowed"),
      "the fake required payload count overflowed");
  receipt.required_exact_fallback_count = checked_sum(
      request.prior_exact_fallback_count,
      launch.exact_fallback_count,
      "the fake required fallback count overflowed");

  if (launch.capacity_stop_reason ==
          MortonYao48RankedPairTileClassifierStopReason::none &&
      launch.exact_fallback_count == 0U) {
    receipt.status = static_cast<std::uint64_t>(
        request.frontier_closed_after_chunk
            ? MortonYao48RankedPairTileClassifierStatus::frontier_closed
            : MortonYao48RankedPairTileClassifierStatus::chunk_committed);
    receipt.stop_reason = static_cast<std::uint64_t>(
        MortonYao48RankedPairTileClassifierStopReason::none);
    receipt.failure_code = static_cast<std::uint64_t>(
        Phase15MortonYao48RankedPairTileClassifierFailureCode::none);
    receipt.transaction_accepted_record_count =
        launch.accepted_record_count;
    receipt.transaction_above_window_count = launch.above_window_count;
    receipt.transaction_strict_payload_point_id_count =
        launch.strict_payload_point_id_count;
    receipt.transaction_shell_payload_point_id_count =
        launch.shell_payload_point_id_count;
    receipt.transaction_exact_fallback_count = launch.exact_fallback_count;
    receipt.cumulative_candidate_count = checked_sum(
        request.prior_candidate_count,
        request.candidate_count,
        "the fake cumulative candidate count overflowed");
    receipt.cumulative_accepted_record_count =
        receipt.required_catalog_record_count;
    receipt.cumulative_above_window_count = checked_sum(
        request.prior_above_window_count,
        launch.above_window_count,
        "the fake cumulative above-window count overflowed");
    receipt.cumulative_unresolved_count = request.prior_unresolved_count;
    receipt.cumulative_strict_payload_point_id_count = checked_sum(
        request.prior_strict_payload_point_id_count,
        launch.strict_payload_point_id_count,
        "the fake cumulative strict payload count overflowed");
    receipt.cumulative_shell_payload_point_id_count = checked_sum(
        request.prior_shell_payload_point_id_count,
        launch.shell_payload_point_id_count,
        "the fake cumulative shell payload count overflowed");
    receipt.cumulative_exact_fallback_count =
        receipt.required_exact_fallback_count;
    receipt.output = make_output(
        context,
        request,
        receipt.cumulative_accepted_record_count,
        receipt.cumulative_strict_payload_point_id_count,
        receipt.cumulative_shell_payload_point_id_count);
  } else {
    const bool exact_predicate_failure =
        launch.capacity_stop_reason ==
            MortonYao48RankedPairTileClassifierStopReason::none &&
        launch.exact_fallback_count != 0U;
    receipt.status = static_cast<std::uint64_t>(
        exact_predicate_failure
            ? MortonYao48RankedPairTileClassifierStatus::
                  exact_predicate_failure
            : MortonYao48RankedPairTileClassifierStatus::capacity_exhausted);
    receipt.stop_reason = static_cast<std::uint64_t>(
        exact_predicate_failure
            ? MortonYao48RankedPairTileClassifierStopReason::
                  exact_predicate_failure
            : launch.capacity_stop_reason);
    receipt.failure_code = static_cast<std::uint64_t>(
        exact_predicate_failure
            ? Phase15MortonYao48RankedPairTileClassifierFailureCode::
                  exact_predicate_failure
            : Phase15MortonYao48RankedPairTileClassifierFailureCode::
                  capacity_exhausted);
    receipt.transaction_exact_fallback_count =
        launch.exact_fallback_count;
    if (exact_predicate_failure) {
      receipt.required_catalog_record_count =
          request.prior_accepted_record_count;
      receipt.required_payload_point_id_count = checked_sum(
          request.prior_strict_payload_point_id_count,
          request.prior_shell_payload_point_id_count,
          "the fake prior payload count overflowed");
    }
    receipt.transaction_unresolved_count = request.candidate_count;
    receipt.cumulative_candidate_count = checked_sum(
        request.prior_candidate_count,
        request.candidate_count,
        "the fake censored candidate count overflowed");
    receipt.cumulative_accepted_record_count =
        request.prior_accepted_record_count;
    receipt.cumulative_above_window_count =
        request.prior_above_window_count;
    receipt.cumulative_unresolved_count = checked_sum(
        request.prior_unresolved_count,
        request.candidate_count,
        "the fake censored unresolved count overflowed");
    receipt.cumulative_strict_payload_point_id_count =
        request.prior_strict_payload_point_id_count;
    receipt.cumulative_shell_payload_point_id_count =
        request.prior_shell_payload_point_id_count;
    receipt.cumulative_exact_fallback_count =
        receipt.required_exact_fallback_count;
    receipt.output_invalidated = true;
    context.replace_launcher_state({});
  }

  apply_corruption(receipt, launch.corruption);
  receipt.receipt_digest =
      phase15_morton_yao48_ranked_pair_tile_classifier_receipt_digest(
          receipt);
  if (launch.corruption == Corruption::corrupt_receipt_digest) {
    receipt.receipt_digest ^= UINT64_C(1);
  }
  return receipt;
}

}  // namespace morsehgp3d::gpu::detail
