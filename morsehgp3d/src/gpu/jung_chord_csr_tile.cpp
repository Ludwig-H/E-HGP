#include "morsehgp3d/gpu/jung_chord_csr_tile.hpp"

#include "phase15_jung_chord_csr_tile_internal.hpp"

#include <algorithm>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::gpu {
namespace {

struct JungChordDetachedOwner final {
  std::shared_ptr<void> source_owner;
  std::shared_ptr<void> output_owner;
};

[[nodiscard]] bool valid_config(const JungChordCsrTileConfig& config) {
  return (config.support_size == 3U || config.support_size == 4U) &&
      config.maximum_relevant_closed_rank >= config.support_size &&
      config.maximum_relevant_closed_rank <=
          jung_chord_csr_tile_maximum_closed_rank &&
      config.anchor_capacity != 0U &&
      config.anchor_capacity <= jung_chord_csr_tile_maximum_anchor_capacity &&
      config.chord_point_id_capacity != 0U;
}

[[nodiscard]] std::size_t sorted_quantile(
    const std::vector<std::size_t>& values,
    std::size_t numerator,
    std::size_t denominator) {
  if (values.empty()) {
    return 0U;
  }
  const std::size_t rank =
      (numerator * values.size() + denominator - 1U) / denominator;
  return values[rank == 0U ? 0U : rank - 1U];
}

void saturating_add(
    std::uint64_t value,
    std::uint64_t& accumulator,
    bool* saturated = nullptr) {
  if (value > std::numeric_limits<std::uint64_t>::max() - accumulator) {
    accumulator = std::numeric_limits<std::uint64_t>::max();
    if (saturated != nullptr) {
      *saturated = true;
    }
    return;
  }
  accumulator += value;
}

[[nodiscard]] std::uint64_t choose_two_saturated(
    std::uint64_t value,
    bool& saturated) {
  if (value < 2U) {
    return 0U;
  }
  const std::uint64_t left = (value & 1U) == 0U ? value / 2U : value;
  const std::uint64_t right = (value & 1U) == 0U ? value - 1U
                                                 : (value - 1U) / 2U;
  if (right != 0U && left > std::numeric_limits<std::uint64_t>::max() / right) {
    saturated = true;
    return std::numeric_limits<std::uint64_t>::max();
  }
  return left * right;
}

[[nodiscard]] bool canonical_anchor_source(const std::string& source) {
  return source == "explicit_proposal_only" ||
      source == "morton_window_proposal_only";
}

}  // namespace

JungChordCsrTileLease::JungChordCsrTileLease(
    JungChordCsrTileAudit audit,
    std::shared_ptr<void> retained_owner,
    std::shared_ptr<const void> source_cloud_identity,
    std::shared_ptr<const void> source_index_identity,
    const JungChordAnchor* device_anchors,
    const std::uint64_t* device_chord_offsets,
    const spatial::PointId* device_chord_point_ids,
    const std::uint8_t* device_chord_flags,
    const std::uint64_t* device_constant_interior_counts,
    const std::uint8_t* device_anchor_flags,
    const std::uint64_t* source_device_coordinate_bits,
    const std::uint64_t* source_device_morton_point_ids,
    const void* source_device_nodes,
    int cuda_device,
    bool host_fake)
    : audit_(std::move(audit)),
      retained_owner_(std::move(retained_owner)),
      source_cloud_identity_(std::move(source_cloud_identity)),
      source_index_identity_(std::move(source_index_identity)),
      device_anchors_(device_anchors),
      device_chord_offsets_(device_chord_offsets),
      device_chord_point_ids_(device_chord_point_ids),
      device_chord_flags_(device_chord_flags),
      device_constant_interior_counts_(device_constant_interior_counts),
      device_anchor_flags_(device_anchor_flags),
      source_device_coordinate_bits_(source_device_coordinate_bits),
      source_device_morton_point_ids_(source_device_morton_point_ids),
      source_device_nodes_(source_device_nodes),
      cuda_device_(cuda_device),
      host_fake_(host_fake) {}

bool JungChordCsrTileLease::ready() const noexcept {
  if (!retained_owner_ || !source_cloud_identity_ || !source_index_identity_ ||
      audit_.schema_version != jung_chord_csr_tile_schema_version ||
      audit_.source_snapshot_epoch == 0U || audit_.tile_epoch == 0U ||
      audit_.point_count == 0U || audit_.anchor_count == 0U ||
      audit_.required_chord_point_id_count !=
          audit_.committed_chord_point_id_count ||
      !audit_.source_identity_authenticated ||
      !audit_.source_epoch_authenticated || !audit_.anchors_sorted_unique ||
      !audit_.count_scan_emit_used || !audit_.stackless_lbvh_traversal_used ||
      !audit_.directed_binary64_intervals_used ||
      !audit_.ambiguity_emitted_fail_open ||
      !audit_.certified_outward_jung_range_bound_used ||
      !audit_.depth_cutoff_certified || !audit_.output_publication_atomic ||
      audit_.output_withheld || audit_.global_pair_matrix_materialized ||
      audit_.support_tuple_universe_materialized ||
      audit_.ordinary_delaunay_materialized ||
      audit_.higher_order_delaunay_mosaic_materialized ||
      audit_.global_cell_or_coface_arena_materialized ||
      audit_.scientific_decision_published || audit_.public_status_claimed ||
      host_fake_ != audit_.host_fake_launcher_exercised) {
    return false;
  }
  if (host_fake_) {
    return cuda_device_ == -1 && device_anchors_ == nullptr &&
        device_chord_offsets_ == nullptr &&
        device_chord_point_ids_ == nullptr &&
        device_chord_flags_ == nullptr &&
        device_constant_interior_counts_ == nullptr &&
        device_anchor_flags_ == nullptr &&
        source_device_coordinate_bits_ == nullptr &&
        source_device_morton_point_ids_ == nullptr &&
        source_device_nodes_ == nullptr && !audit_.cuda_execution_performed &&
        !audit_.cuda_device_storage_retained;
  }
  return cuda_device_ >= 0 && device_anchors_ != nullptr &&
      device_chord_offsets_ != nullptr &&
      (audit_.committed_chord_point_id_count == 0U ||
       (device_chord_point_ids_ != nullptr && device_chord_flags_ != nullptr)) &&
      device_constant_interior_counts_ != nullptr &&
      device_anchor_flags_ != nullptr &&
      source_device_coordinate_bits_ != nullptr &&
      source_device_morton_point_ids_ != nullptr &&
      source_device_nodes_ != nullptr && audit_.cuda_execution_performed &&
      audit_.cuda_device_storage_retained &&
      audit_.source_device_coordinates_retained &&
      audit_.source_device_morton_point_ids_retained &&
      audit_.source_device_nodes_retained;
}

bool JungChordCsrTileLease::cuda_resident() const noexcept {
  return ready() && !host_fake_;
}

bool JungChordCsrTileLease::host_fake() const noexcept {
  return ready() && host_fake_;
}

JungChordCsrTileContext::JungChordCsrTileContext(
    MortonLbvhDeviceTraversalLease&& traversal_lease,
    JungChordCsrTileConfig config)
    : config_(config) {
  if (!valid_config(config_) || !traversal_lease.ready()) {
    throw std::invalid_argument(
        "a Jung-chord CSR context requires a valid config and ready LBVH "
        "traversal lease");
  }
  source_audit_ = traversal_lease.audit_;
  source_cloud_identity_ = traversal_lease.source_cloud_identity_;
  source_index_identity_ = traversal_lease.source_index_identity_;
  device_coordinate_bits_ = traversal_lease.device_coordinate_bits_;
  device_morton_point_ids_ = traversal_lease.device_morton_point_ids_;
  device_nodes_ = traversal_lease.device_nodes_;
  cuda_device_ = traversal_lease.cuda_device_;
  host_fake_ = source_audit_.host_fake_lifecycle_exercised;
  auto owner = std::make_shared<MortonLbvhDeviceTraversalLease>(
      std::move(traversal_lease));
  source_owner_ = std::move(owner);
  state_ = std::make_shared<detail::Phase15JungChordCsrTileContextState>();
  if (!ready()) {
    throw std::invalid_argument(
        "the Jung-chord CSR context rejected a mixed or unauthenticated "
        "LBVH lease shape");
  }
}

JungChordCsrTileContext::~JungChordCsrTileContext() noexcept = default;
JungChordCsrTileContext::JungChordCsrTileContext(
    JungChordCsrTileContext&&) noexcept = default;
JungChordCsrTileContext& JungChordCsrTileContext::operator=(
    JungChordCsrTileContext&&) noexcept = default;

bool JungChordCsrTileContext::ready() const noexcept {
  if (!state_ || !source_owner_ || !source_cloud_identity_ ||
      !source_index_identity_ || !valid_config(config_) ||
      source_audit_.source_snapshot_epoch == 0U ||
      source_audit_.point_count == 0U ||
      source_audit_.certified_node_count !=
          2U * source_audit_.point_count - 1U ||
      !source_audit_.source_snapshot_import_certified ||
      !source_audit_.source_index_identity_retained ||
      source_audit_.higher_order_delaunay_mosaic_materialized ||
      source_audit_.global_cell_or_coface_arena_materialized ||
      source_audit_.public_status_claimed) {
    return false;
  }
  if (host_fake_) {
    return !source_audit_.cuda_device_storage_retained &&
        device_coordinate_bits_ == nullptr &&
        device_morton_point_ids_ == nullptr && device_nodes_ == nullptr &&
        cuda_device_ == -1;
  }
  return source_audit_.cuda_device_storage_retained &&
      device_coordinate_bits_ != nullptr &&
      device_morton_point_ids_ != nullptr && device_nodes_ != nullptr &&
      cuda_device_ >= 0;
}

bool JungChordCsrTileContext::poisoned() const noexcept {
  return !state_ || state_->poisoned.load(std::memory_order_acquire);
}

JungChordCsrTileAdvance JungChordCsrTileContext::classify(
    JungChordAnchorTile tile) {
  if (!ready()) {
    throw std::logic_error("a moved-from Jung-chord context cannot classify");
  }
  std::lock_guard lock{state_->mutex};
  if (state_->poisoned.load(std::memory_order_acquire)) {
    throw std::logic_error("a poisoned Jung-chord context cannot classify");
  }
  if (!outstanding_tile_lifetime_.expired()) {
    throw std::logic_error(
        "release the preceding Jung-chord CSR lease before classifying the "
        "next explicit anchor tile");
  }

  JungChordCsrTileAdvance advance;
  auto& audit = advance.audit;
  audit.source_snapshot_epoch = source_audit_.source_snapshot_epoch;
  audit.tile_epoch = next_tile_epoch_;
  audit.point_count = source_audit_.point_count;
  audit.certified_node_count = source_audit_.certified_node_count;
  audit.support_size = config_.support_size;
  audit.maximum_relevant_closed_rank =
      config_.maximum_relevant_closed_rank;
  audit.maximum_constant_interior_count =
      config_.maximum_relevant_closed_rank - config_.support_size;
  audit.anchor_capacity = config_.anchor_capacity;
  audit.chord_point_id_capacity = config_.chord_point_id_capacity;
  audit.anchor_count = tile.anchors.size();
  audit.anchor_source = tile.anchor_source;
  audit.anchor_source_window = tile.source_window;
  audit.source_identity_authenticated = true;
  audit.source_epoch_authenticated = true;
  audit.output_publication_atomic = true;
  audit.quadratic_chord_pair_baseline_executed = false;

  const bool sorted = std::is_sorted(tile.anchors.begin(), tile.anchors.end());
  const bool unique = std::adjacent_find(
      tile.anchors.begin(), tile.anchors.end()) == tile.anchors.end();
  audit.anchors_sorted_unique = sorted && unique;
  const bool anchor_ids_valid = std::all_of(
      tile.anchors.begin(), tile.anchors.end(), [&](const auto& anchor) {
        return anchor.support_u < anchor.support_v &&
            anchor.support_v < source_audit_.point_count;
      });
  if (tile.anchors.empty() || tile.anchors.size() > config_.anchor_capacity ||
      !canonical_anchor_source(tile.anchor_source) || !sorted || !unique ||
      !anchor_ids_valid) {
    advance.status = tile.anchors.size() > config_.anchor_capacity
        ? JungChordCsrTileStatus::capacity_exhausted
        : JungChordCsrTileStatus::malformed_input;
    advance.stop_reason = tile.anchors.size() > config_.anchor_capacity
        ? JungChordCsrTileStopReason::anchor_capacity
        : JungChordCsrTileStopReason::malformed_input;
    ++next_tile_epoch_;
    return advance;
  }

  detail::Phase15JungChordCsrTileReceipt receipt;
  try {
    receipt = detail::launch_phase15_jung_chord_csr_tile(
        detail::Phase15JungChordCsrTileRequest{
            config_,
            tile.anchors,
            source_audit_.point_count,
            source_audit_.certified_node_count,
            source_audit_.source_snapshot_epoch,
            next_tile_epoch_,
            device_coordinate_bits_,
            device_morton_point_ids_,
            device_nodes_,
            cuda_device_,
            host_fake_});
  } catch (...) {
    state_->poisoned.store(true, std::memory_order_release);
    throw;
  }

  audit.required_chord_point_id_count = receipt.required_chord_count;
  audit.committed_chord_point_id_count = receipt.committed_chord_count;
  audit.physical_device_arena_capacity_bytes =
      receipt.physical_device_arena_bytes;
  audit.required_device_arena_capacity_bytes =
      receipt.required_device_arena_bytes;
  audit.retained_output_device_byte_count =
      receipt.retained_output_device_bytes;
  audit.logical_output_device_byte_count =
      receipt.logical_output_device_bytes;
  audit.configured_capacity_upper_bound_byte_count =
      receipt.configured_capacity_upper_bound_bytes;
  audit.cub_scan_temporary_capacity_bytes = receipt.cub_scan_temporary_bytes;
  audit.retained_source_coordinate_word_capacity =
      source_audit_.retained_coordinate_word_capacity;
  audit.retained_source_morton_point_id_capacity =
      source_audit_.retained_morton_point_id_capacity;
  audit.retained_source_node_capacity = source_audit_.retained_node_capacity;
  audit.anchor_host_to_device_byte_count = receipt.anchor_h2d_bytes;
  audit.control_device_to_host_byte_count = receipt.control_d2h_bytes;
  audit.chord_device_to_host_byte_count = receipt.chord_d2h_bytes;
  audit.cuda_kernel_launch_count = receipt.kernel_launches;
  audit.cuda_library_submission_count = receipt.library_submissions;
  audit.cuda_synchronization_count = receipt.synchronizations;
  audit.anchor_upload_nanoseconds = receipt.anchor_upload_ns;
  audit.count_kernel_nanoseconds = receipt.count_kernel_ns;
  audit.scan_nanoseconds = receipt.scan_ns;
  audit.capacity_preflight_nanoseconds = receipt.capacity_preflight_ns;
  audit.emit_kernel_nanoseconds = receipt.emit_kernel_ns;
  audit.total_device_nanoseconds = receipt.total_device_ns;
  audit.host_fake_launcher_exercised = receipt.host_fake;
  audit.cuda_execution_performed = receipt.cuda_executed;
  audit.cuda_device_storage_retained = receipt.output_valid && !receipt.host_fake;
  audit.source_device_coordinates_retained =
      receipt.output_valid && !receipt.host_fake;
  audit.source_device_morton_point_ids_retained =
      receipt.output_valid && !receipt.host_fake;
  audit.source_device_nodes_retained = receipt.output_valid && !receipt.host_fake;
  audit.count_scan_emit_used =
      receipt.status == JungChordCsrTileStatus::tile_complete;
  audit.per_anchor_diagnostics_collected =
      config_.collect_per_anchor_diagnostics;
  audit.stackless_lbvh_traversal_used = true;
  audit.directed_binary64_intervals_used = true;
  audit.ambiguity_emitted_fail_open = true;
  audit.diameter_lens_support_eligibility_flagged = true;
  audit.certified_outward_jung_range_bound_used = true;
  audit.depth_cutoff_certified = true;
  audit.chord_device_to_host_performed = receipt.chord_d2h_bytes != 0U;

  std::vector<std::size_t> chord_counts;
  std::vector<std::size_t> range_counts;
  std::vector<std::size_t> eligible_counts;
  chord_counts.reserve(receipt.anchor_controls.size());
  range_counts.reserve(receipt.anchor_controls.size());
  eligible_counts.reserve(receipt.anchor_controls.size());
  for (const auto& control : receipt.anchor_controls) {
    const auto chord_count = static_cast<std::size_t>(control.chord_count);
    const auto range_count =
        static_cast<std::size_t>(control.range_candidate_count);
    chord_counts.push_back(chord_count);
    range_counts.push_back(range_count);
    eligible_counts.push_back(
        static_cast<std::size_t>(control.support_eligible_chords));
    audit.depth_rejected_anchor_count +=
        (control.flags & detail::phase15_jung_chord_anchor_depth_rejected) != 0U;
    audit.degenerate_anchor_count +=
        (control.flags & detail::phase15_jung_chord_anchor_degenerate) != 0U;
    saturating_add(control.constant_interior_lower_bound,
                   audit.constant_interior_lower_bound_sum);
    saturating_add(control.count_node_visits, audit.count_pass_node_visit_count);
    saturating_add(control.emit_node_visits, audit.emit_pass_node_visit_count);
    saturating_add(control.count_leaf_tests, audit.count_pass_leaf_test_count);
    saturating_add(control.emit_leaf_tests, audit.emit_pass_leaf_test_count);
    saturating_add(control.bulk_constant_interior_points,
                   audit.bulk_constant_interior_point_count);
    saturating_add(control.bulk_constant_exterior_points,
                   audit.bulk_constant_exterior_point_count);
    saturating_add(control.range_pruned_points,
                   audit.certified_outward_jung_range_pruned_point_count);
    saturating_add(control.range_candidate_count,
                   audit.range_candidate_point_count_sum);
    saturating_add(control.certified_active_chords,
                   audit.certified_active_chord_count);
    saturating_add(control.ambiguous_active_chords,
                   audit.ambiguous_fail_open_chord_count);
    saturating_add(control.support_eligible_chords,
                   audit.support_eligible_chord_count);
    saturating_add(control.support_eligible_ambiguous_chords,
                   audit.support_eligible_ambiguous_chord_count);
    bool choose_saturated = false;
    const std::uint64_t baseline =
        choose_two_saturated(control.chord_count, choose_saturated);
    audit.quadratic_chord_pair_baseline_saturated |= choose_saturated;
    saturating_add(
        baseline,
        audit.quadratic_chord_pair_baseline_count,
        &audit.quadratic_chord_pair_baseline_saturated);
  }
  std::sort(chord_counts.begin(), chord_counts.end());
  std::sort(range_counts.begin(), range_counts.end());
  std::sort(eligible_counts.begin(), eligible_counts.end());
  audit.chord_count_p50 = sorted_quantile(chord_counts, 50U, 100U);
  audit.chord_count_p95 = sorted_quantile(chord_counts, 95U, 100U);
  audit.chord_count_p99 = sorted_quantile(chord_counts, 99U, 100U);
  audit.chord_count_maximum =
      chord_counts.empty() ? 0U : *std::max_element(
          chord_counts.begin(), chord_counts.end());
  audit.range_candidate_count_p50 = sorted_quantile(range_counts, 50U, 100U);
  audit.range_candidate_count_p95 = sorted_quantile(range_counts, 95U, 100U);
  audit.range_candidate_count_p99 = sorted_quantile(range_counts, 99U, 100U);
  audit.range_candidate_count_maximum =
      range_counts.empty() ? 0U : *std::max_element(
          range_counts.begin(), range_counts.end());
  audit.census_censored_by_depth_cutoff =
      audit.depth_rejected_anchor_count != 0U;
  audit.support_eligible_count_p50 =
      sorted_quantile(eligible_counts, 50U, 100U);
  audit.support_eligible_count_p95 =
      sorted_quantile(eligible_counts, 95U, 100U);
  audit.support_eligible_count_p99 =
      sorted_quantile(eligible_counts, 99U, 100U);
  audit.support_eligible_count_maximum = eligible_counts.empty()
      ? 0U
      : eligible_counts.back();

  advance.status = receipt.status;
  advance.stop_reason = receipt.stop_reason;
  if (receipt.status == JungChordCsrTileStatus::execution_failure ||
      receipt.stop_reason == JungChordCsrTileStopReason::count_emit_mismatch ||
      receipt.stop_reason == JungChordCsrTileStopReason::traversal_failure) {
    state_->poisoned.store(true, std::memory_order_release);
  }
  const bool publish = receipt.status == JungChordCsrTileStatus::tile_complete &&
      receipt.stop_reason == JungChordCsrTileStopReason::none &&
      receipt.output_valid && receipt.output_owner &&
      (!config_.collect_per_anchor_diagnostics ||
       receipt.anchor_controls.size() == tile.anchors.size());
  audit.output_withheld = !publish;
  if (publish) {
    auto detached = std::make_shared<JungChordDetachedOwner>();
    detached->source_owner = source_owner_;
    detached->output_owner = std::move(receipt.output_owner);
    audit.ordinary_delaunay_materialized = false;
    audit.higher_order_delaunay_mosaic_materialized = false;
    audit.global_cell_or_coface_arena_materialized = false;
    audit.scientific_decision_published = false;
    audit.public_status_claimed = false;
    JungChordCsrTileLease published(
        audit,
        detached,
        source_cloud_identity_,
        source_index_identity_,
        receipt.device_anchors,
        receipt.device_offsets,
        receipt.device_chord_point_ids,
        receipt.device_chord_flags,
        receipt.device_constant_interior_counts,
        receipt.device_anchor_flags,
        device_coordinate_bits_,
        device_morton_point_ids_,
        device_nodes_,
        receipt.host_fake ? -1 : cuda_device_,
        receipt.host_fake);
    if (!published.ready()) {
      state_->poisoned.store(true, std::memory_order_release);
      throw std::runtime_error(
          "the Jung-chord launcher returned an invalid atomic publication");
    }
    advance.tile.emplace(std::move(published));
    outstanding_tile_lifetime_ = detached;
  }
  ++next_tile_epoch_;
  return advance;
}

}  // namespace morsehgp3d::gpu
