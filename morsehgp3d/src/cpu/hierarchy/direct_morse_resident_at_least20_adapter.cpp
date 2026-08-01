#include "morsehgp3d/hierarchy/direct_morse_resident_at_least20_adapter.hpp"

#include <algorithm>
#include <array>
#include <exception>
#include <limits>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

static_assert(sizeof(std::size_t) <= sizeof(std::uint64_t));

struct AdapterSessionSeal final {};

void hash_u64(
    contract::CanonicalSha256Builder& builder,
    std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  for (std::size_t index = 0U; index < bytes.size(); ++index) {
    bytes[index] = static_cast<std::uint8_t>(
        value >> ((bytes.size() - 1U - index) * 8U));
  }
  builder.update(bytes);
}

void hash_id(
    contract::CanonicalSha256Builder& builder,
    const contract::CanonicalId& value) {
  builder.update(value.bytes());
}

[[nodiscard]] bool id_is_zero(
    const contract::CanonicalId& value) noexcept {
  return value == contract::CanonicalId{};
}

[[nodiscard]] contract::CanonicalId resident_scientific_digest(
    const ExactDirectSparseUnifiedLevelPlanResult& plan,
    ExactDirectMorseUnifiedResidentSourceKind source_kind) {
  contract::CanonicalSha256Builder builder;
  builder.update(
      "MorseHGP3D/direct-morse-resident-at-least20/science/v1/");
  hash_u64(builder, direct_morse_resident_at_least20_adapter_schema_version);
  hash_u64(
      builder,
      static_cast<std::uint64_t>(static_cast<std::underlying_type_t<
          ExactDirectMorseUnifiedResidentSourceKind>>(source_kind)));
  // These four immutable-plan digests are scientific source identity.  In
  // particular, the higher semantic digest already commits its stream schema,
  // traversal/proof/backend/profile/mode and requested/effective K; source_kind
  // distinguishes the two verified resident-plan constructions.  The
  // process-local resident authority and locator instance are deliberately
  // excluded and are bound separately by the private ticket capabilities.
  hash_id(builder, plan.source_pair_canonical_cloud_digest);
  hash_id(builder, plan.source_higher_canonical_cloud_digest);
  hash_id(builder, plan.source_pair_semantic_digest);
  hash_id(builder, plan.source_higher_semantic_digest);
  return builder.finalize();
}

[[nodiscard]] bool slice_fits(
    std::size_t offset,
    std::size_t count,
    std::size_t arena_size) noexcept {
  return offset <= arena_size && count <= arena_size - offset;
}

[[nodiscard]] bool checked_add(
    std::size_t left,
    std::size_t right,
    std::size_t& output) noexcept {
  if (left > std::numeric_limits<std::size_t>::max() - right) {
    return false;
  }
  output = left + right;
  return true;
}

[[nodiscard]] std::optional<ExactDirectAtLeast20SourceAction> view_action(
    ExactFrozenIncidenceHgpAction action) noexcept {
  switch (action) {
    case ExactFrozenIncidenceHgpAction::reduced_birth:
      return ExactDirectAtLeast20SourceAction::reduced_birth;
    case ExactFrozenIncidenceHgpAction::continuation:
      return ExactDirectAtLeast20SourceAction::continuation;
    case ExactFrozenIncidenceHgpAction::multifusion:
      return ExactDirectAtLeast20SourceAction::multifusion;
  }
  return std::nullopt;
}

[[nodiscard]] bool action_matches(
    ExactFrozenIncidenceHgpAction resident_action,
    ExactDirectAtLeast20SourceAction expected_action) noexcept {
  const auto mapped = view_action(resident_action);
  return mapped.has_value() && mapped.value() == expected_action;
}

[[nodiscard]] bool fresh_resident_genesis(
    const ExactDirectMorseUnifiedResidentSession& resident) noexcept {
  if (!resident.certified_resident_session() || resident.batch_cursor() != 0U ||
      resident.epoch() != 0U || !resident.group_records().empty() ||
      !resident.root_coverages().empty() ||
      resident.source_plan_initial_verification_count() != 1U ||
      resident.frozen_batch_source_replay_count() != 0U ||
      resident.frozen_batch_reconstruction_count() != 0U ||
      !resident.locator().certified_positive_locator()) {
    return false;
  }
  const auto stamp = resident.locator().snapshot_stamp();
  if (stamp.external_authority_id == 0U ||
      stamp.committed_batch_count != 0U || stamp.inserted_key_count != 0U ||
      stamp.component_union_count != 0U || stamp.binding_count != 0U) {
    return false;
  }
  return std::all_of(
      resident.component_states().begin(),
      resident.component_states().end(),
      [](const ExactDirectMorseUnifiedResidentComponentState& component) {
        return !component.active && !component.root_id.has_value() &&
            component.latent_point_coverage.empty();
      });
}

[[nodiscard]] bool source_plan_identity_is_certified(
    const ExactDirectMorseUnifiedResidentSession& resident) noexcept {
  const auto& plan = resident.plan();
  return plan.certified_bounded_plan() &&
      !id_is_zero(plan.source_pair_canonical_cloud_digest) &&
      !id_is_zero(plan.source_higher_canonical_cloud_digest) &&
      !id_is_zero(plan.source_pair_semantic_digest) &&
      !id_is_zero(plan.source_higher_semantic_digest);
}

}  // namespace

ExactDirectMorseResidentAtLeast20ProjectionSeam::Result
ExactDirectMorseResidentAtLeast20ProjectionSeam::project_verified_bundle(
    const ExactDirectMorseUnifiedResidentAuthorityBundle& bundle,
    std::uint64_t source_session_authority_id,
    const contract::CanonicalId& source_scientific_digest,
    const contract::CanonicalId& previous_source_commit_digest,
    std::size_t group_record_count_before,
    ExactDirectAtLeast20RootId next_root_id_before,
    const ExactDirectAtLeast20StreamViewBudget& view_budget) noexcept {
  Result output;
  try {
    const auto& frozen = bundle.frozen_batch;
    const auto& actions = frozen.action_plan;
    if (!bundle.certified_strict_pre_batch_bundle() ||
        source_session_authority_id == 0U ||
        bundle.identity.session_authority_id != source_session_authority_id ||
        id_is_zero(source_scientific_digest) ||
        bundle.source_batch_index != bundle.identity.batch_cursor ||
        frozen.source_batch_index != bundle.source_batch_index ||
        frozen.squared_level != bundle.squared_level ||
        frozen.order != bundle.order ||
        actions.groups.size() != frozen.quotient.groups.size() ||
        actions.groups.size() != frozen.coverage_deltas.size() ||
        actions.groups.size() > view_budget.maximum_batch_group_count ||
        bundle.identity.batch_cursor ==
            std::numeric_limits<std::size_t>::max() ||
        bundle.identity.epoch ==
            std::numeric_limits<std::size_t>::max()) {
      return output;
    }

    std::size_t total_prior_roots = 0U;
    std::size_t total_delta_points = 0U;
    for (std::size_t group_index = 0U;
         group_index < actions.groups.size(); ++group_index) {
      const auto& action = actions.groups[group_index];
      const auto& delta = frozen.coverage_deltas[group_index];
      if (!checked_add(
              total_prior_roots,
              action.prior_root_count,
              total_prior_roots) ||
          !checked_add(
              total_delta_points,
              delta.point_reference_count,
              total_delta_points)) {
        return output;
      }
    }
    if (total_prior_roots >
            view_budget.maximum_batch_prior_root_reference_count ||
        total_delta_points >
            view_budget.maximum_batch_coverage_delta_point_reference_count) {
      output.decision = Decision::view_budget_rejected;
      return output;
    }

    Projection projection;
    projection.source_batch_index = bundle.source_batch_index;
    projection.source_epoch = bundle.identity.epoch;
    projection.order = bundle.order;
    projection.squared_level = bundle.squared_level;
    projection.group_record_count_before = group_record_count_before;
    projection.next_root_id_before = next_root_id_before;
    projection.next_root_id_after = next_root_id_before;
    projection.expected_groups.reserve(actions.groups.size());

    for (std::size_t group_index = 0U;
         group_index < actions.groups.size(); ++group_index) {
      const auto& action = actions.groups[group_index];
      const auto& delta = frozen.coverage_deltas[group_index];
      const auto mapped_action = view_action(action.action);
      if (action.group_index != group_index ||
          delta.coverage_delta_record_index != group_index ||
          delta.owner_group_index != group_index ||
          action.q_r != action.prior_root_count ||
          delta.q_r != action.q_r || delta.action != action.action ||
          !mapped_action.has_value() ||
          !slice_fits(
              action.prior_root_offset,
              action.prior_root_count,
              actions.prior_root_ids.size()) ||
          !slice_fits(
              delta.point_reference_offset,
              delta.point_reference_count,
              frozen.coverage_delta_points.size())) {
        return output;
      }

      ExactDirectAtLeast20CommittedGroupInput projected;
      projected.source_group_index = group_index;
      projected.source_action = mapped_action.value();
      projected.q_r = action.q_r;
      const auto prior_begin = actions.prior_root_ids.begin() +
          static_cast<std::ptrdiff_t>(action.prior_root_offset);
      projected.prior_root_ids.assign(
          prior_begin,
          prior_begin +
              static_cast<std::ptrdiff_t>(action.prior_root_count));
      projected.coverage_delta_point_ids.reserve(
          delta.point_reference_count);
      for (std::size_t local = 0U;
           local < delta.point_reference_count; ++local) {
        const auto& point_reference = frozen.coverage_delta_points
            [delta.point_reference_offset + local];
        if (point_reference.owner_group_index != group_index) {
          return output;
        }
        projected.coverage_delta_point_ids.push_back(
            point_reference.point_id);
      }

      if (action.q_r == 1U) {
        projected.resultant_root_id = projected.prior_root_ids.front();
      } else {
        if (projection.next_root_id_after == 0U) {
          output.decision = Decision::root_id_mirror_rejected;
          return output;
        }
        projected.resultant_root_id = projection.next_root_id_after;
        projection.next_root_id_after =
            projection.next_root_id_after ==
                    std::numeric_limits<ExactDirectAtLeast20RootId>::max()
                ? 0U
                : projection.next_root_id_after + 1U;
      }
      projection.expected_groups.push_back(std::move(projected));
    }

    std::size_t expected_group_record_count_after = 0U;
    if (!checked_add(
            projection.group_record_count_before,
            projection.expected_groups.size(),
            expected_group_record_count_after)) {
      return output;
    }
    static_cast<void>(expected_group_record_count_after);

    auto& view_input = projection.view_input;
    view_input.source_session_authority_id = source_session_authority_id;
    view_input.source_batch_cursor_before = bundle.identity.batch_cursor;
    view_input.source_batch_cursor_after = bundle.identity.batch_cursor + 1U;
    view_input.source_epoch_before = bundle.identity.epoch;
    view_input.source_epoch_after = bundle.identity.epoch + 1U;
    view_input.order = bundle.order;
    view_input.squared_level = bundle.squared_level;
    view_input.source_scientific_digest = source_scientific_digest;
    view_input.previous_source_commit_digest =
        previous_source_commit_digest;
    view_input.groups = projection.expected_groups;
    view_input.complete_equal_level_batch_committed = true;
    view_input.source_actions_qr_root_successors_and_coverage_delta_certified =
        true;
    view_input.source_pruned_by_min_cluster_size = false;
    view_input.source_public_status_claimed = false;

    output.projection.emplace(std::move(projection));
    output.decision = Decision::complete;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision = Decision::allocation_failed;
    return output;
  } catch (const std::length_error&) {
    output.decision = Decision::view_budget_rejected;
    return output;
  } catch (const std::exception&) {
    return output;
  }
}

bool ExactDirectMorseResidentAtLeast20ProjectionSeam::
    committed_suffix_matches(
        const Projection& projection,
        std::span<const ExactDirectMorseUnifiedResidentGroupRecord>
            actual_records) noexcept {
  std::size_t expected_record_count = 0U;
  if (!checked_add(
          projection.group_record_count_before,
          projection.expected_groups.size(),
          expected_record_count) ||
      actual_records.size() != expected_record_count) {
    return false;
  }
  for (std::size_t local = 0U;
       local < projection.expected_groups.size(); ++local) {
    const auto& expected = projection.expected_groups[local];
    const auto& actual =
        actual_records[projection.group_record_count_before + local];
    if (actual.group_record_index !=
            projection.group_record_count_before + local ||
        actual.batch_index != projection.source_batch_index ||
        actual.owner_group_index != expected.source_group_index ||
        actual.squared_level != projection.squared_level ||
        actual.order != projection.order || actual.q_r != expected.q_r ||
        !action_matches(actual.action, expected.source_action) ||
        actual.resultant_root_id != expected.resultant_root_id ||
        actual.child_root_ids != expected.prior_root_ids ||
        actual.coverage_delta_points !=
            expected.coverage_delta_point_ids ||
        actual.empty_coverage_delta !=
            expected.coverage_delta_point_ids.empty()) {
      return false;
    }
  }
  return true;
}

struct ExactDirectMorseResidentAtLeast20AdapterPreparedBatch::Impl {
  std::shared_ptr<const AdapterSessionSeal> seal;
  ExactDirectMorseUnifiedResidentPreparedBatch resident_ticket;
  ExactDirectAtLeast20StreamViewPreparedBatch view_ticket;
  ExactDirectMorseResidentAtLeast20ProjectionSeam::Projection projection;
  contract::CanonicalId expected_view_source_commit_digest{};
  contract::CanonicalId expected_view_digest{};
  bool canonical_projection_certified{false};
  bool consumed{false};
};

struct ExactDirectMorseResidentAtLeast20Adapter::Impl {
  ExactDirectMorseUnifiedResidentSession resident;
  ExactDirectAtLeast20StreamViewSession view;
  ExactDirectAtLeast20StreamViewBudget view_budget{};
  std::shared_ptr<const AdapterSessionSeal> seal;
  contract::CanonicalId scientific_digest{};
  ExactDirectAtLeast20RootId next_root_id{1U};
  bool initialized{false};
};

ExactDirectMorseResidentAtLeast20AdapterPreparedBatch::
    ExactDirectMorseResidentAtLeast20AdapterPreparedBatch() noexcept = default;
ExactDirectMorseResidentAtLeast20AdapterPreparedBatch::
    ~ExactDirectMorseResidentAtLeast20AdapterPreparedBatch() = default;
ExactDirectMorseResidentAtLeast20AdapterPreparedBatch::
    ExactDirectMorseResidentAtLeast20AdapterPreparedBatch(
        ExactDirectMorseResidentAtLeast20AdapterPreparedBatch&&) noexcept =
        default;
ExactDirectMorseResidentAtLeast20AdapterPreparedBatch&
ExactDirectMorseResidentAtLeast20AdapterPreparedBatch::operator=(
    ExactDirectMorseResidentAtLeast20AdapterPreparedBatch&&) noexcept = default;
ExactDirectMorseResidentAtLeast20AdapterPreparedBatch::
    ExactDirectMorseResidentAtLeast20AdapterPreparedBatch(
        std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectMorseResidentAtLeast20AdapterPreparedBatch::valid()
    const noexcept {
  return impl_ != nullptr && impl_->seal != nullptr && !impl_->consumed &&
      impl_->canonical_projection_certified &&
      impl_->resident_ticket.valid() && impl_->view_ticket.valid();
}

bool ExactDirectMorseResidentAtLeast20AdapterPreparedBatch::consumed()
    const noexcept {
  return impl_ == nullptr || impl_->consumed;
}

std::size_t
ExactDirectMorseResidentAtLeast20AdapterPreparedBatch::source_batch_index()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->projection.source_batch_index;
}

ExactDirectAtLeast20RootId
ExactDirectMorseResidentAtLeast20AdapterPreparedBatch::next_root_id_before()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->projection.next_root_id_before;
}

ExactDirectAtLeast20RootId
ExactDirectMorseResidentAtLeast20AdapterPreparedBatch::next_root_id_after()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->projection.next_root_id_after;
}

bool ExactDirectMorseResidentAtLeast20AdapterPreparationResult::
    certified_prepared_batch() const noexcept {
  return ticket.has_value() && ticket->valid() &&
      resident_decision ==
          ExactDirectMorseUnifiedResidentPreparationDecision::
              complete_certified_prepared_batch &&
      view_decision == ExactDirectAtLeast20StreamConsumeDecision::not_consumed &&
      no_scientific_state_mutated &&
      genuine_resident_ticket_capability_held &&
      canonical_group_projection_certified &&
      root_id_allocation_mirrored_from_genesis &&
      view_allocations_and_digests_completed && !public_status_claimed &&
      decision ==
          ExactDirectMorseResidentAtLeast20AdapterPreparationDecision::
              complete_certified_atomic_prepared_batch;
}

bool ExactDirectMorseResidentAtLeast20AdapterCommitResult::
    certified_atomic_resident_and_view_commit() const noexcept {
  return resident_commit.certified_committed_batch() &&
      view_commit.certified_atomic_downstream_batch_fold() &&
      ticket_consumed && expected_records_match_new_resident_suffix &&
      view_commit_used_resident_capability && root_id_mirror_committed &&
      source_and_view_chains_advanced_together &&
      !no_scientific_state_mutated_on_failure && !public_status_claimed &&
      decision == ExactDirectMorseResidentAtLeast20AdapterCommitDecision::
          complete_certified_atomic_resident_and_view_commit;
}

ExactDirectMorseResidentAtLeast20Adapter::
    ExactDirectMorseResidentAtLeast20Adapter() noexcept = default;
ExactDirectMorseResidentAtLeast20Adapter::
    ~ExactDirectMorseResidentAtLeast20Adapter() = default;
ExactDirectMorseResidentAtLeast20Adapter::
    ExactDirectMorseResidentAtLeast20Adapter(
        ExactDirectMorseResidentAtLeast20Adapter&&) noexcept = default;
ExactDirectMorseResidentAtLeast20Adapter&
ExactDirectMorseResidentAtLeast20Adapter::operator=(
    ExactDirectMorseResidentAtLeast20Adapter&&) noexcept = default;
ExactDirectMorseResidentAtLeast20Adapter::
    ExactDirectMorseResidentAtLeast20Adapter(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

bool ExactDirectMorseResidentAtLeast20Adapter::certified_adapter_session()
    const noexcept {
  if (impl_ == nullptr || !impl_->initialized || impl_->seal == nullptr ||
      id_is_zero(impl_->scientific_digest) ||
      !impl_->resident.certified_resident_session() ||
      !impl_->view.initialized()) {
    return false;
  }
  const auto stamp = impl_->resident.locator().snapshot_stamp();
  return stamp.external_authority_id ==
             impl_->view.source_session_authority_id() &&
      stamp.committed_batch_count == impl_->resident.batch_cursor() &&
      impl_->resident.batch_cursor() ==
          impl_->view.next_source_batch_cursor() &&
      impl_->resident.epoch() == impl_->view.source_epoch() &&
      impl_->view.source_scientific_digest() == impl_->scientific_digest;
}

bool ExactDirectMorseResidentAtLeast20Adapter::complete() const noexcept {
  return certified_adapter_session() && impl_->resident.complete();
}

std::size_t ExactDirectMorseResidentAtLeast20Adapter::batch_cursor()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->resident.batch_cursor();
}

std::size_t ExactDirectMorseResidentAtLeast20Adapter::epoch() const noexcept {
  return impl_ == nullptr ? 0U : impl_->resident.epoch();
}

ExactDirectAtLeast20RootId
ExactDirectMorseResidentAtLeast20Adapter::next_root_id() const noexcept {
  return impl_ == nullptr ? 0U : impl_->next_root_id;
}

const contract::CanonicalId&
ExactDirectMorseResidentAtLeast20Adapter::source_scientific_digest()
    const noexcept {
  static const contract::CanonicalId empty{};
  return impl_ == nullptr ? empty : impl_->scientific_digest;
}

const ExactDirectMorseUnifiedResidentSession&
ExactDirectMorseResidentAtLeast20Adapter::resident() const noexcept {
  static const ExactDirectMorseUnifiedResidentSession empty;
  return impl_ == nullptr ? empty : impl_->resident;
}

const ExactDirectAtLeast20StreamViewSession&
ExactDirectMorseResidentAtLeast20Adapter::view() const noexcept {
  static const ExactDirectAtLeast20StreamViewSession empty;
  return impl_ == nullptr ? empty : impl_->view;
}

ExactDirectMorseResidentAtLeast20AdapterPreparationResult
ExactDirectMorseResidentAtLeast20Adapter::prepare_next() noexcept {
  ExactDirectMorseResidentAtLeast20AdapterPreparationResult output;
  output.no_scientific_state_mutated = true;
  const auto reject = [&](const auto decision) {
    output.ticket.reset();
    output.decision = decision;
    return std::move(output);
  };
  if (!certified_adapter_session()) {
    return reject(
        ExactDirectMorseResidentAtLeast20AdapterPreparationDecision::
            no_adapter_not_initialized);
  }

  try {
    auto resident_preparation = impl_->resident.prepare_next();
    output.resident_decision = resident_preparation.decision;
    if (!resident_preparation.certified_prepared_batch() ||
        !resident_preparation.ticket.has_value()) {
      return reject(
          ExactDirectMorseResidentAtLeast20AdapterPreparationDecision::
              no_resident_preparation_rejected);
    }
    output.genuine_resident_ticket_capability_held = true;

    const auto& bundle =
        resident_preparation.ticket->authority_bundle();
    const auto& plan = impl_->resident.plan();
    const auto locator_stamp = impl_->resident.locator().snapshot_stamp();
    if (!bundle.certified_strict_pre_batch_bundle() ||
        bundle.identity.session_authority_id !=
            impl_->view.source_session_authority_id() ||
        bundle.identity.epoch != impl_->resident.epoch() ||
        bundle.identity.batch_cursor != impl_->resident.batch_cursor() ||
        bundle.identity.locator_stamp != locator_stamp ||
        bundle.identity.source_pair_canonical_cloud_digest !=
            plan.source_pair_canonical_cloud_digest ||
        bundle.identity.source_higher_canonical_cloud_digest !=
            plan.source_higher_canonical_cloud_digest ||
        bundle.identity.source_pair_semantic_digest !=
            plan.source_pair_semantic_digest ||
        bundle.identity.source_higher_semantic_digest !=
            plan.source_higher_semantic_digest) {
      return reject(
          ExactDirectMorseResidentAtLeast20AdapterPreparationDecision::
              no_resident_authority_bundle_rejected);
    }

    auto projection_result =
        ExactDirectMorseResidentAtLeast20ProjectionSeam::
            project_verified_bundle(
                bundle,
                impl_->view.source_session_authority_id(),
                impl_->scientific_digest,
                impl_->view.source_commit_digest(),
                impl_->resident.group_records().size(),
                impl_->next_root_id,
                impl_->view_budget);
    if (projection_result.decision !=
            ExactDirectMorseResidentAtLeast20ProjectionSeam::Decision::
                complete ||
        !projection_result.projection.has_value()) {
      switch (projection_result.decision) {
        case ExactDirectMorseResidentAtLeast20ProjectionSeam::Decision::
            root_id_mirror_rejected:
          return reject(
              ExactDirectMorseResidentAtLeast20AdapterPreparationDecision::
                  no_root_id_mirror_rejected);
        case ExactDirectMorseResidentAtLeast20ProjectionSeam::Decision::
            view_budget_rejected:
          return reject(
              ExactDirectMorseResidentAtLeast20AdapterPreparationDecision::
                  no_view_preparation_rejected);
        case ExactDirectMorseResidentAtLeast20ProjectionSeam::Decision::
            allocation_failed:
          return reject(
              ExactDirectMorseResidentAtLeast20AdapterPreparationDecision::
                  no_allocation_failed);
        case ExactDirectMorseResidentAtLeast20ProjectionSeam::Decision::
            rejected:
        case ExactDirectMorseResidentAtLeast20ProjectionSeam::Decision::
            complete:
          return reject(
              ExactDirectMorseResidentAtLeast20AdapterPreparationDecision::
                  no_resident_authority_bundle_rejected);
      }
    }
    auto projection = std::move(projection_result.projection.value());
    auto prepared = std::make_unique<
        ExactDirectMorseResidentAtLeast20AdapterPreparedBatch::Impl>();
    prepared->seal = impl_->seal;
    prepared->expected_view_source_commit_digest =
        impl_->view.source_commit_digest();
    prepared->expected_view_digest = impl_->view.view_digest();
    auto view_preparation =
        impl_->view.prepare_resident_batch(
            std::move(projection.view_input));
    output.view_decision = view_preparation.result.decision;
    if (!view_preparation.certified_prepared_batch() ||
        !view_preparation.ticket.has_value() ||
        !view_preparation.resident_commit_capability_bound) {
      return reject(
          ExactDirectMorseResidentAtLeast20AdapterPreparationDecision::
              no_view_preparation_rejected);
    }

    prepared->resident_ticket =
        std::move(resident_preparation.ticket.value());
    prepared->view_ticket = std::move(view_preparation.ticket.value());
    prepared->projection = std::move(projection);
    prepared->canonical_projection_certified = true;
    output.ticket.emplace(
        ExactDirectMorseResidentAtLeast20AdapterPreparedBatch{
            std::move(prepared)});
    output.canonical_group_projection_certified = true;
    output.root_id_allocation_mirrored_from_genesis = true;
    output.view_allocations_and_digests_completed = true;
    output.public_status_claimed = false;
    output.decision =
        ExactDirectMorseResidentAtLeast20AdapterPreparationDecision::
            complete_certified_atomic_prepared_batch;
    return output;
  } catch (const std::bad_alloc&) {
    return reject(
        ExactDirectMorseResidentAtLeast20AdapterPreparationDecision::
            no_allocation_failed);
  } catch (const std::length_error&) {
    return reject(
        ExactDirectMorseResidentAtLeast20AdapterPreparationDecision::
            no_view_preparation_rejected);
  } catch (const std::exception&) {
    return reject(
        ExactDirectMorseResidentAtLeast20AdapterPreparationDecision::
            no_resident_authority_bundle_rejected);
  }
}

ExactDirectMorseResidentAtLeast20AdapterCommitResult
ExactDirectMorseResidentAtLeast20Adapter::commit(
    ExactDirectMorseResidentAtLeast20AdapterPreparedBatch&& ticket) noexcept {
  ExactDirectMorseResidentAtLeast20AdapterCommitResult output;
  if (!certified_adapter_session()) {
    output.no_scientific_state_mutated_on_failure = true;
    output.decision =
        ExactDirectMorseResidentAtLeast20AdapterCommitDecision::
            no_adapter_not_initialized;
    return output;
  }
  auto prepared = std::move(ticket.impl_);
  if (prepared == nullptr || prepared->consumed ||
      !prepared->canonical_projection_certified ||
      !prepared->resident_ticket.valid() || !prepared->view_ticket.valid()) {
    output.no_scientific_state_mutated_on_failure = true;
    output.decision =
        ExactDirectMorseResidentAtLeast20AdapterCommitDecision::
            no_ticket_not_valid;
    return output;
  }
  prepared->consumed = true;
  output.ticket_consumed = true;
  if (prepared->seal.get() != impl_->seal.get()) {
    output.no_scientific_state_mutated_on_failure = true;
    output.decision =
        ExactDirectMorseResidentAtLeast20AdapterCommitDecision::
            no_foreign_ticket_rejected;
    return output;
  }

  output.resident_commit =
      impl_->resident.commit(std::move(prepared->resident_ticket));
  if (!output.resident_commit.certified_committed_batch()) {
    output.no_scientific_state_mutated_on_failure =
        output.resident_commit.no_scientific_state_mutated_on_failure;
    output.decision =
        ExactDirectMorseResidentAtLeast20AdapterCommitDecision::
            no_resident_commit_rejected;
    return output;
  }

  // The resident transaction is now irreversible.  Every remaining check is
  // allocation-free; an impossible mismatch must stop the process rather than
  // return a false cross-component atomicity claim.
  const auto& actual_records = impl_->resident.group_records();
  const auto& projection = prepared->projection;
  if (projection.next_root_id_before != impl_->next_root_id ||
      impl_->resident.batch_cursor() !=
          projection.source_batch_index + 1U ||
      impl_->resident.epoch() != projection.source_epoch + 1U ||
      !ExactDirectMorseResidentAtLeast20ProjectionSeam::
          committed_suffix_matches(projection, actual_records)) {
    std::terminate();
  }
  output.expected_records_match_new_resident_suffix = true;

  output.view_commit = impl_->view.commit_prepared_batch(
      std::move(prepared->view_ticket), true);
  if (!output.view_commit.certified_atomic_downstream_batch_fold() ||
      !output.view_commit.upstream_source_commit_capability_verified ||
      output.view_commit.view_digest_before !=
          prepared->expected_view_digest ||
      output.view_commit.source_batch_cursor_after !=
          impl_->resident.batch_cursor() ||
      output.view_commit.source_epoch_after != impl_->resident.epoch() ||
      impl_->view.source_commit_digest() !=
          output.view_commit.source_commit_digest ||
      impl_->view.view_digest() != output.view_commit.view_digest_after ||
      impl_->view.source_commit_digest() ==
          prepared->expected_view_source_commit_digest) {
    std::terminate();
  }

  impl_->next_root_id = projection.next_root_id_after;
  output.view_commit_used_resident_capability = true;
  output.root_id_mirror_committed = true;
  output.source_and_view_chains_advanced_together =
      impl_->resident.batch_cursor() ==
          impl_->view.next_source_batch_cursor() &&
      impl_->resident.epoch() == impl_->view.source_epoch();
  output.no_scientific_state_mutated_on_failure = false;
  output.public_status_claimed = false;
  output.decision =
      ExactDirectMorseResidentAtLeast20AdapterCommitDecision::
          complete_certified_atomic_resident_and_view_commit;
  return output;
}

bool ExactDirectMorseResidentAtLeast20AdapterInitializationResult::
    certified_initialized_adapter() const noexcept {
  return adapter.has_value() && adapter->certified_adapter_session() &&
      !id_is_zero(source_scientific_digest) && resident_genesis_verified &&
      source_identity_derived_from_verified_immutable_plan &&
      root_id_allocation_mirrored_from_genesis &&
      view_initialized_at_resident_genesis &&
      no_global_facet_coface_or_gamma_catalog_materialized &&
      !public_status_claimed &&
      decision ==
          ExactDirectMorseResidentAtLeast20AdapterInitializationDecision::
              complete_certified_atomic_adapter_session;
}

ExactDirectMorseResidentAtLeast20AdapterInitializationResult
initialize_exact_direct_morse_resident_at_least20_adapter(
    ExactDirectMorseUnifiedResidentSession&& resident,
    const ExactDirectAtLeast20StreamViewBudget& view_budget) noexcept {
  ExactDirectMorseResidentAtLeast20AdapterInitializationResult output;
  if (!resident.certified_resident_session()) {
    output.decision =
        ExactDirectMorseResidentAtLeast20AdapterInitializationDecision::
            no_resident_session_rejected;
    return output;
  }
  if (!fresh_resident_genesis(resident)) {
    output.decision =
        ExactDirectMorseResidentAtLeast20AdapterInitializationDecision::
            no_non_genesis_resident_session_rejected;
    return output;
  }
  if (!source_plan_identity_is_certified(resident)) {
    output.decision =
        ExactDirectMorseResidentAtLeast20AdapterInitializationDecision::
            no_resident_source_identity_rejected;
    return output;
  }

  try {
    output.source_scientific_digest = resident_scientific_digest(
        resident.plan(), resident.source_kind());
    const std::uint64_t source_authority_id =
        resident.locator().snapshot_stamp().external_authority_id;
    auto view_initialization =
        initialize_exact_direct_at_least20_stream_view_session(
            source_authority_id,
            0U,
            0U,
            output.source_scientific_digest,
            contract::CanonicalId{},
            view_budget);
    if (!view_initialization.certified_initialized_session() ||
        !view_initialization.session.has_value()) {
      output.decision =
          ExactDirectMorseResidentAtLeast20AdapterInitializationDecision::
              no_view_initialization_rejected;
      return output;
    }

    auto seal = std::make_shared<const AdapterSessionSeal>();
    auto impl =
        std::make_unique<ExactDirectMorseResidentAtLeast20Adapter::Impl>();
    impl->view_budget = view_budget;
    impl->seal = std::move(seal);
    impl->scientific_digest = output.source_scientific_digest;
    impl->next_root_id = 1U;
    impl->resident = std::move(resident);
    impl->view = std::move(view_initialization.session.value());
    impl->initialized = true;
    ExactDirectMorseResidentAtLeast20Adapter adapter{std::move(impl)};
    if (!adapter.certified_adapter_session()) {
      output.decision =
          ExactDirectMorseResidentAtLeast20AdapterInitializationDecision::
              no_view_initialization_rejected;
      return output;
    }
    output.adapter.emplace(std::move(adapter));
    output.resident_genesis_verified = true;
    output.source_identity_derived_from_verified_immutable_plan = true;
    output.root_id_allocation_mirrored_from_genesis = true;
    output.view_initialized_at_resident_genesis = true;
    output.no_global_facet_coface_or_gamma_catalog_materialized = true;
    output.public_status_claimed = false;
    output.decision =
        ExactDirectMorseResidentAtLeast20AdapterInitializationDecision::
            complete_certified_atomic_adapter_session;
    return output;
  } catch (const std::bad_alloc&) {
    output.decision =
        ExactDirectMorseResidentAtLeast20AdapterInitializationDecision::
            no_allocation_failed;
    return output;
  } catch (const std::exception&) {
    output.decision =
        ExactDirectMorseResidentAtLeast20AdapterInitializationDecision::
            no_resident_source_identity_rejected;
    return output;
  }
}

}  // namespace morsehgp3d::hierarchy
