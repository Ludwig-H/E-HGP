#include "morsehgp3d/gpu/morton_yao48_radial_subtree_filter.hpp"

#include "../cuda/phase15_morton_yao48_radial_subtree_filter_internal.hpp"
#include "morsehgp3d/hierarchy/yao48_cone.hpp"
#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace morsehgp3d::gpu {
namespace {

using hierarchy::classify_exact_yao48_cone;

inline constexpr std::uint64_t kAll48BanksMask =
    (UINT64_C(1) << 48U) - UINT64_C(1);

struct PreparedExactQuery {
  detail::Phase15MortonYao48RadialDeviceInput device{};
  std::uint64_t source_snapshot_epoch{};
  spatial::PointId anchor_point_id{};
  std::uint64_t leaf_begin{};
  std::uint64_t leaf_end{};
  std::array<spatial::PointId, 3U> lower_point_ids{};
  std::array<spatial::PointId, 3U> upper_point_ids{};
};

struct ExactReplay {
  bool accepted{};
  exact::ExactLevel aabb_minimum_squared_distance{};
  exact::ExactLevel three_times_maximum_bank_radius{};
  std::size_t witness_distance_evaluation_count{};
  std::size_t cone_recertification_count{};
};

[[nodiscard]] bool finite_nonnegative_word(std::uint64_t bits) noexcept {
  constexpr std::uint64_t exponent_mask = UINT64_C(0x7ff0000000000000);
  constexpr std::uint64_t sign_mask = UINT64_C(0x8000000000000000);
  return (bits & exponent_mask) != exponent_mask &&
         (bits & sign_mask) == 0U;
}

[[nodiscard]] std::size_t checked_node_count(std::size_t point_count) {
  if (point_count == 0U ||
      point_count > std::numeric_limits<std::size_t>::max() / 2U + 1U) {
    throw std::length_error(
        "the radial-subtree node universe overflows size_t");
  }
  return 2U * point_count - 1U;
}

[[nodiscard]] exact::ExactLevel squared_distance(
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId first,
    spatial::PointId second) {
  exact::ExactRational value;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational delta =
        cloud.point(first).coordinate(axis) -
        cloud.point(second).coordinate(axis);
    value = value + delta * delta;
  }
  return exact::ExactLevel{std::move(value)};
}

[[nodiscard]] exact::ExactLevel aabb_minimum_squared_distance(
    const spatial::CanonicalPointCloud& cloud,
    spatial::PointId anchor,
    const std::array<spatial::PointId, 3U>& lower_ids,
    const std::array<spatial::PointId, 3U>& upper_ids) {
  exact::ExactRational value;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    const exact::ExactRational coordinate =
        cloud.point(anchor).coordinate(axis);
    const exact::ExactRational lower =
        cloud.point(lower_ids[axis]).coordinate(axis);
    const exact::ExactRational upper =
        cloud.point(upper_ids[axis]).coordinate(axis);
    if (lower > upper) {
      throw std::invalid_argument(
          "a radial-subtree AABB lower witness exceeds its upper witness");
    }
    exact::ExactRational delta;
    if (coordinate < lower) {
      delta = lower - coordinate;
    } else if (coordinate > upper) {
      delta = coordinate - upper;
    }
    value = value + delta * delta;
  }
  return exact::ExactLevel{std::move(value)};
}

[[nodiscard]] std::uint64_t binary64_bits(double value) noexcept {
  return std::bit_cast<std::uint64_t>(value);
}

[[nodiscard]] PreparedExactQuery prepare_query(
    const spatial::CanonicalPointCloud& cloud,
    std::uint64_t source_snapshot_epoch,
    spatial::PointId anchor_point_id,
    std::size_t leaf_begin,
    std::size_t leaf_end,
    const std::array<spatial::PointId, 3U>& lower_point_ids,
    const std::array<spatial::PointId, 3U>& upper_point_ids,
    std::span<const std::size_t> leaf_position_by_point_id,
    const MortonYao48RadialSubtreeQuery& query) {
  const std::size_t point_count = cloud.size();
  if (query.replay_id == 0U || source_snapshot_epoch == 0U ||
      query.maximum_closed_rank < 2U ||
      query.maximum_closed_rank >
          morton_yao48_radial_subtree_filter_maximum_witness_count + 1U) {
    throw std::invalid_argument(
        "a radial-subtree query has invalid authority, interval or rank");
  }
  if (leaf_begin >= leaf_end ||
      leaf_end > static_cast<std::size_t>(query.anchor_morton_position) ||
      leaf_position_by_point_id.size() != point_count) {
    throw std::invalid_argument(
        "a radial-subtree node is not wholly inside the owner's strict "
        "Morton prefix");
  }

  PreparedExactQuery prepared;
  prepared.device.replay_id = query.replay_id;
  prepared.source_snapshot_epoch = source_snapshot_epoch;
  prepared.anchor_point_id = anchor_point_id;
  prepared.leaf_begin = static_cast<std::uint64_t>(leaf_begin);
  prepared.leaf_end = static_cast<std::uint64_t>(leaf_end);
  prepared.lower_point_ids = lower_point_ids;
  prepared.upper_point_ids = upper_point_ids;
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    prepared.device.anchor_bits[axis] = binary64_bits(
        cloud.point(anchor_point_id).binary64_coordinate(axis));
    prepared.device.lower_bits[axis] = binary64_bits(
        cloud.point(lower_point_ids[axis]).binary64_coordinate(axis));
    prepared.device.upper_bits[axis] = binary64_bits(
        cloud.point(upper_point_ids[axis]).binary64_coordinate(axis));
  }

  const std::size_t required_witness_count =
      query.maximum_closed_rank - 1U;
  bool every_device_bound_valid = true;
  for (std::size_t cone = 0U;
       cone < morton_yao48_radial_subtree_filter_cone_count;
       ++cone) {
    if (query.bank_witness_counts[cone] != required_witness_count) {
      throw std::invalid_argument(
          "a radial-subtree query requires every Yao48 bank to be full");
    }
    for (std::size_t slot = 0U; slot < required_witness_count; ++slot) {
      const spatial::PointId witness =
          query.bank_witness_point_ids[cone][slot];
      bool duplicates_previous_slot = false;
      for (std::size_t previous_slot = 0U; previous_slot < slot;
           ++previous_slot) {
        duplicates_previous_slot =
            duplicates_previous_slot ||
            query.bank_witness_point_ids[cone][previous_slot] == witness;
      }
      if (witness >= static_cast<spatial::PointId>(point_count) ||
          witness == anchor_point_id || duplicates_previous_slot ||
          (leaf_position_by_point_id[static_cast<std::size_t>(witness)] >=
               leaf_begin &&
           leaf_position_by_point_id[static_cast<std::size_t>(witness)] <
               leaf_end)) {
        throw std::invalid_argument(
            "a radial-subtree bank does not contain K distinct witnesses "
            "outside the proposed subtree");
      }
    }
    const std::uint64_t upper_bits =
        query.bank_squared_radius_upper_bits[cone];
    prepared.device.bank_radius_upper_bits[cone] = upper_bits;
    every_device_bound_valid =
        every_device_bound_valid && finite_nonnegative_word(upper_bits) &&
        upper_bits != 0U;
  }
  prepared.device.full_bank_mask = kAll48BanksMask;
  prepared.device.device_eligible =
      every_device_bound_valid ? std::uint8_t{1U} : std::uint8_t{0U};
  return prepared;
}

[[nodiscard]] ExactReplay replay_exact_prune(
    const spatial::CanonicalPointCloud& cloud,
    const MortonYao48RadialSubtreeQuery& query,
    const PreparedExactQuery& prepared) {
  ExactReplay replay;
  const std::size_t required_witness_count =
      query.maximum_closed_rank - 1U;
  exact::ExactLevel maximum_bank_radius;
  bool maximum_initialized = false;
  for (std::size_t cone = 0U;
       cone < morton_yao48_radial_subtree_filter_cone_count;
       ++cone) {
    exact::ExactLevel bank_radius;
    for (std::size_t slot = 0U; slot < required_witness_count; ++slot) {
      const spatial::PointId witness =
          query.bank_witness_point_ids[cone][slot];
      const std::size_t exact_cone = classify_exact_yao48_cone(
          cloud.point(prepared.anchor_point_id), cloud.point(witness))
                                         .cone_index;
      ++replay.cone_recertification_count;
      if (exact_cone != cone) {
        return replay;
      }
      const exact::ExactLevel distance =
          squared_distance(cloud, prepared.anchor_point_id, witness);
      ++replay.witness_distance_evaluation_count;
      if (slot == 0U || bank_radius < distance) {
        bank_radius = distance;
      }
    }
    if (!finite_nonnegative_word(
            query.bank_squared_radius_upper_bits[cone])) {
      return replay;
    }
    const exact::ExactLevel claimed_upper{exact::ExactRational::
        from_binary64_bits(query.bank_squared_radius_upper_bits[cone])};
    if (claimed_upper < bank_radius) {
      return replay;
    }
    if (!maximum_initialized || maximum_bank_radius < bank_radius) {
      maximum_bank_radius = bank_radius;
      maximum_initialized = true;
    }
  }
  if (!maximum_initialized) {
    return replay;
  }
  replay.aabb_minimum_squared_distance = aabb_minimum_squared_distance(
      cloud,
      prepared.anchor_point_id,
      prepared.lower_point_ids,
      prepared.upper_point_ids);
  replay.three_times_maximum_bank_radius = exact::ExactLevel{
      exact::BigInt{3} * maximum_bank_radius.numerator(),
      maximum_bank_radius.denominator()};
  replay.accepted = replay.aabb_minimum_squared_distance >=
      replay.three_times_maximum_bank_radius;
  return replay;
}

}  // namespace

MortonYao48RadialSubtreeFilterContext::
    MortonYao48RadialSubtreeFilterContext(std::size_t maximum_query_count)
    : state_(std::make_shared<
             detail::Phase15MortonYao48RadialSubtreeFilterState>()),
      maximum_query_count_(maximum_query_count) {
  if (maximum_query_count_ == 0U) {
    throw std::invalid_argument(
        "the radial-subtree filter requires a nonzero fixed query capacity");
  }
}

MortonYao48RadialSubtreeFilterContext::
    ~MortonYao48RadialSubtreeFilterContext() noexcept = default;
MortonYao48RadialSubtreeFilterContext::
    MortonYao48RadialSubtreeFilterContext(
        MortonYao48RadialSubtreeFilterContext&&) noexcept = default;
MortonYao48RadialSubtreeFilterContext&
MortonYao48RadialSubtreeFilterContext::operator=(
    MortonYao48RadialSubtreeFilterContext&&) noexcept = default;

MortonYao48RadialSubtreeFilterResult
MortonYao48RadialSubtreeFilterContext::decide(
    const spatial::CanonicalPointCloud& cloud,
    const MortonLbvhDeviceBuildResult& certified_build,
    std::span<const MortonYao48RadialSubtreeQuery> queries) {
  if (!state_) {
    throw std::logic_error(
        "a moved-from radial-subtree filter cannot decide");
  }
  if (queries.empty()) {
    throw std::invalid_argument(
        "the radial-subtree filter does not accept an empty batch");
  }
  if (queries.size() > maximum_query_count_) {
    throw std::length_error(
        "the radial-subtree batch exceeds its fixed query capacity");
  }
  if (!certified_build.complete_certified_build()) {
    throw std::invalid_argument(
        "the radial-subtree filter requires a complete certified Morton "
        "LBVH build");
  }
  const spatial::MortonLbvhIndex& index =
      certified_build.certified_index();
  const MortonLbvhDeviceBuildAudit& build_audit = certified_build.audit();
  if (!index.ready() || !index.validated_for(cloud) ||
      index.point_count_ != cloud.size() ||
      index.nodes_.size() != checked_node_count(cloud.size()) ||
      index.leaves_.size() != cloud.size() ||
      build_audit.snapshot_buffer_epoch == 0U ||
      build_audit.point_count != cloud.size() ||
      build_audit.required_node_count != index.nodes_.size() ||
      !build_audit.snapshot_import_certified ||
      !build_audit.strict_find_split_postorder_requested ||
      !build_audit.minimum_point_id_aabb_witness_rule_requested) {
    throw std::invalid_argument(
        "the radial-subtree filter received an unauthenticated Morton "
        "snapshot authority");
  }

  return state_->with_gpu_section([&] {
    std::vector<PreparedExactQuery> prepared;
    std::vector<detail::Phase15MortonYao48RadialDeviceInput> device_inputs;
    prepared.reserve(queries.size());
    device_inputs.reserve(queries.size());
    std::set<std::uint64_t> replay_ids;
    for (const MortonYao48RadialSubtreeQuery& query : queries) {
      if (!replay_ids.insert(query.replay_id).second) {
        throw std::invalid_argument(
            "radial-subtree replay identifiers must be unique per batch");
      }
      if (query.anchor_morton_position >= index.leaves_.size() ||
          query.node_index >= index.nodes_.size()) {
        throw std::invalid_argument(
            "a radial-subtree query names an out-of-range Morton anchor or "
            "node");
      }
      const std::size_t anchor_position =
          static_cast<std::size_t>(query.anchor_morton_position);
      const auto& node =
          index.nodes_[static_cast<std::size_t>(query.node_index)];
      prepared.push_back(prepare_query(
          cloud,
          build_audit.snapshot_buffer_epoch,
          index.leaves_[anchor_position].point_id,
          node.leaf_begin,
          node.leaf_end,
          node.lower_point_ids,
          node.upper_point_ids,
          index.leaf_position_by_point_id_,
          query));
      device_inputs.push_back(prepared.back().device);
    }

    const detail::Phase15MortonYao48RadialDeviceBatch batch =
        detail::propose_phase15_morton_yao48_radial_subtrees_on_gpu(
            *state_, device_inputs, maximum_query_count_);
    if (batch.records.size() != queries.size() ||
        batch.maximum_query_count != maximum_query_count_ ||
        batch.launcher_call_count != 1U ||
        !batch.directed_round_down_aabb_implemented ||
        !batch.directed_round_up_three_radius_implemented ||
        !batch.equality_proposes_prune) {
      throw std::runtime_error(
          "the radial-subtree launcher violated its immutable contract");
    }
    const bool cuda = batch.execution_kind ==
        detail::Phase15MortonYao48RadialExecutionKind::cuda;
    if ((cuda &&
         (batch.cuda_device < 0 || batch.kernel_launch_count != 1U ||
          batch.synchronization_count != 1U)) ||
        (!cuda &&
         (batch.cuda_device != -1 || batch.kernel_launch_count != 0U ||
          batch.synchronization_count != 0U))) {
      throw std::runtime_error(
          "the radial-subtree launcher misreported its execution kind");
    }

    MortonYao48RadialSubtreeFilterResult result;
    result.decisions.reserve(queries.size());
    result.audit.input_count = queries.size();
    result.audit.maximum_query_count = maximum_query_count_;
    result.audit.launcher_call_count = batch.launcher_call_count;
    result.audit.cuda_kernel_launch_count = batch.kernel_launch_count;
    result.audit.cuda_synchronization_count = batch.synchronization_count;
    result.audit.cuda_device = batch.cuda_device;
    result.audit.host_fake_launcher_exercised = !cuda;
    result.audit.cuda_execution_performed = cuda;
    result.audit.directed_round_down_aabb_recipe_requested = true;
    result.audit.directed_round_up_three_radius_recipe_requested = true;
    result.audit.equality_proposes_prune = true;
    result.audit.all_48_banks_required_full = true;
    result.audit.exact_cpu_replay_required_for_every_prune = true;
    result.audit.fail_open_on_nonfinite_overflow_or_ambiguity = true;
    result.audit.subtree_pruning_implemented = true;
    result.audit.leaf_traversal_performed = false;
    result.audit.pair_record_emitted = false;
    result.audit.product_path_enabled = false;

    for (std::size_t index = 0U; index < queries.size(); ++index) {
      const auto& record = batch.records[index];
      if (record.replay_id != queries[index].replay_id ||
          (record.proposal !=
               detail::Phase15MortonYao48RadialProposal::descend &&
           record.proposal !=
               detail::Phase15MortonYao48RadialProposal::prune)) {
        throw std::runtime_error(
            "the radial-subtree launcher returned a foreign record");
      }
      const PreparedExactQuery& exact_query = prepared[index];
      if (exact_query.device.device_eligible != 0U) {
        ++result.audit.device_eligible_input_count;
      } else {
        ++result.audit.device_fail_open_input_count;
      }
      MortonYao48RadialSubtreeDecisionRecord decision;
      decision.query = queries[index];
      decision.cuda_proposed_prune = record.proposal ==
          detail::Phase15MortonYao48RadialProposal::prune;
      if (decision.cuda_proposed_prune) {
        ++result.audit.cuda_prune_proposal_count;
        ++result.audit.exact_replay_count;
        const ExactReplay replay = replay_exact_prune(
            cloud, queries[index], exact_query);
        result.audit.exact_witness_distance_evaluation_count +=
            replay.witness_distance_evaluation_count;
        result.audit.exact_cone_recertification_count +=
            replay.cone_recertification_count;
        const bool exact_accepts =
            exact_query.device.device_eligible != 0U &&
            record.directed_bounds_valid != 0U && replay.accepted;
        if (exact_accepts) {
          decision.decision =
              MortonYao48RadialSubtreeDecision::certified_prune;
          decision.exact_replay_accepted = true;
          decision.exact_receipt = std::make_unique<
              MortonYao48RadialSubtreeExactReceipt>(
              MortonYao48RadialSubtreeExactReceipt{
                  queries[index].replay_id,
                  exact_query.source_snapshot_epoch,
                  queries[index].anchor_morton_position,
                  exact_query.anchor_point_id,
                  queries[index].node_index,
                  exact_query.leaf_begin,
                  exact_query.leaf_end,
                  exact_query.leaf_end - exact_query.leaf_begin,
                  queries[index].maximum_closed_rank,
                  replay.aabb_minimum_squared_distance,
                  replay.three_times_maximum_bank_radius});
          ++result.audit.exact_accepted_prune_count;
        } else {
          ++result.audit.hostile_or_false_proposal_rejected_count;
        }
      }
      result.decisions.push_back(std::move(decision));
    }
    return result;
  });
}

}  // namespace morsehgp3d::gpu
