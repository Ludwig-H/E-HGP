#include "morsehgp3d/hierarchy/direct_sparse_carrier_origin_capability.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <optional>
#include <utility>
#include <vector>

namespace morsehgp3d::hierarchy {
namespace {

[[nodiscard]] bool typed_token_less(
    const ExactFrozenIncidenceToken& left,
    const ExactFrozenIncidenceToken& right) noexcept {
  if (left.kind != right.kind) {
    return static_cast<std::uint8_t>(left.kind) <
           static_cast<std::uint8_t>(right.kind);
  }
  return left.token_id < right.token_id;
}

[[nodiscard]] bool strict_points(
    std::span<const spatial::PointId> points) noexcept {
  for (std::size_t index = 1U; index < points.size(); ++index) {
    if (points[index - 1U] >= points[index]) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool valid_slice(
    std::size_t offset,
    std::size_t count,
    std::size_t size) noexcept {
  return offset <= size && count <= size - offset;
}

struct TerminalReference {
  ExactDirectSparseCarrierOriginSeedBinding binding{};
  ExactDirectSparseStableFacetHandle root_handle{};
};

struct RootAuthority {
  ExactDirectSparseStableFacetHandle root_handle{};
  std::optional<ExactFrozenIncidencePriorRootId> prior_root_id;
  ExactDirectSparseRootCoverageId coverage_id{};
  std::size_t materialized_point_offset{};
  std::size_t materialized_point_count{};
  ExactFrozenIncidenceToken token{};
  std::size_t typed_coverage_record_index{};
  ExactDirectSparseCarrierOriginKind kind{
      ExactDirectSparseCarrierOriginKind::not_certified};
};

}  // namespace

struct ExactDirectSparseCarrierOriginCapabilityResult::Impl {
  std::uint32_t schema_version{
      direct_sparse_carrier_origin_capability_schema_version};
  ExactDirectSparseCarrierOriginCapabilityBudget requested_budget{};
  ExactDirectSparseStableFacetForestStamp forest_stamp{};
  ExactDirectSparseRootLedgerStamp ledger_stamp{};
  ExactDirectSparseCarrierOriginCapabilityCounters counters{};
  std::vector<ExactDirectSparseCarrierOriginRecord> origins;
  std::vector<ExactDirectNormalizedH0StableFacetResolution>
      carrier_resolutions;
  std::vector<ExactDirectNormalizedH0SparseForestCarrierAttachment>
      carrier_attachments;
  std::vector<ExactDirectFrozenUnifiedPriorRootCoverage>
      prior_root_coverages;
  std::vector<spatial::PointId> prior_root_coverage_points;
  std::vector<ExactDirectFrozenUnifiedLatentCarrierCoverage>
      latent_carrier_coverages;
  std::vector<spatial::PointId> latent_carrier_coverage_points;
  bool every_bound_closure_terminal_root_derived{false};
  bool forest_and_ledger_stamps_aligned{false};
  bool every_distinct_root_probed_once{false};
  bool every_probe_observed_for_ledger_stamp{false};
  bool requested_coverages_proof_bound{false};
  bool rooted_tokens_are_prior_root_ids{false};
  bool latent_tokens_are_terminal_root_handles{false};
  bool carrier_outputs_canonical{false};
  bool carrier_only_scope{true};
  bool equal_facet_resolution_claimed{false};
  bool no_external_root_or_coverage_span_consumed{false};
  bool forest_or_ledger_logical_state_mutated{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  ExactDirectSparseCarrierOriginCapabilityDisposition disposition{
      ExactDirectSparseCarrierOriginCapabilityDisposition::not_certified};
  ExactDirectSparseCarrierOriginCapabilityDecision decision{
      ExactDirectSparseCarrierOriginCapabilityDecision::not_certified};
  bool minted{false};
};

ExactDirectSparseCarrierOriginCapabilityResult::
    ExactDirectSparseCarrierOriginCapabilityResult() noexcept = default;
ExactDirectSparseCarrierOriginCapabilityResult::~
    ExactDirectSparseCarrierOriginCapabilityResult() = default;
ExactDirectSparseCarrierOriginCapabilityResult::
    ExactDirectSparseCarrierOriginCapabilityResult(
        ExactDirectSparseCarrierOriginCapabilityResult&&) noexcept = default;
ExactDirectSparseCarrierOriginCapabilityResult&
ExactDirectSparseCarrierOriginCapabilityResult::operator=(
    ExactDirectSparseCarrierOriginCapabilityResult&&) noexcept = default;

ExactDirectSparseCarrierOriginCapabilityResult::
    ExactDirectSparseCarrierOriginCapabilityResult(
        std::unique_ptr<const Impl> impl) noexcept
    : impl_(std::move(impl)) {}

std::uint32_t
ExactDirectSparseCarrierOriginCapabilityResult::schema_version()
    const noexcept {
  return impl_ == nullptr ? 0U : impl_->schema_version;
}

const ExactDirectSparseCarrierOriginCapabilityBudget&
ExactDirectSparseCarrierOriginCapabilityResult::requested_budget()
    const & noexcept {
  static const ExactDirectSparseCarrierOriginCapabilityBudget empty{};
  return impl_ == nullptr ? empty : impl_->requested_budget;
}

const ExactDirectSparseStableFacetForestStamp&
ExactDirectSparseCarrierOriginCapabilityResult::forest_stamp()
    const & noexcept {
  static const ExactDirectSparseStableFacetForestStamp empty{};
  return impl_ == nullptr ? empty : impl_->forest_stamp;
}

const ExactDirectSparseRootLedgerStamp&
ExactDirectSparseCarrierOriginCapabilityResult::ledger_stamp()
    const & noexcept {
  static const ExactDirectSparseRootLedgerStamp empty{};
  return impl_ == nullptr ? empty : impl_->ledger_stamp;
}

const ExactDirectSparseCarrierOriginCapabilityCounters&
ExactDirectSparseCarrierOriginCapabilityResult::counters() const & noexcept {
  static const ExactDirectSparseCarrierOriginCapabilityCounters empty{};
  return impl_ == nullptr ? empty : impl_->counters;
}

std::span<const ExactDirectSparseCarrierOriginRecord>
ExactDirectSparseCarrierOriginCapabilityResult::origins() const & noexcept {
  return impl_ == nullptr
             ? std::span<const ExactDirectSparseCarrierOriginRecord>{}
             : std::span<const ExactDirectSparseCarrierOriginRecord>{
                   impl_->origins};
}

std::span<const ExactDirectNormalizedH0StableFacetResolution>
ExactDirectSparseCarrierOriginCapabilityResult::
    carrier_stable_facet_resolutions() const & noexcept {
  return impl_ == nullptr
             ? std::span<
                   const ExactDirectNormalizedH0StableFacetResolution>{}
             : std::span<
                   const ExactDirectNormalizedH0StableFacetResolution>{
                   impl_->carrier_resolutions};
}

std::span<const ExactDirectNormalizedH0SparseForestCarrierAttachment>
ExactDirectSparseCarrierOriginCapabilityResult::carrier_attachments()
    const & noexcept {
  return impl_ == nullptr
             ? std::span<const
                   ExactDirectNormalizedH0SparseForestCarrierAttachment>{}
             : std::span<const
                   ExactDirectNormalizedH0SparseForestCarrierAttachment>{
                   impl_->carrier_attachments};
}

std::span<const ExactDirectFrozenUnifiedPriorRootCoverage>
ExactDirectSparseCarrierOriginCapabilityResult::prior_root_coverages()
    const & noexcept {
  return impl_ == nullptr
             ? std::span<
                   const ExactDirectFrozenUnifiedPriorRootCoverage>{}
             : std::span<
                   const ExactDirectFrozenUnifiedPriorRootCoverage>{
                   impl_->prior_root_coverages};
}

std::span<const spatial::PointId>
ExactDirectSparseCarrierOriginCapabilityResult::
    prior_root_coverage_point_references() const & noexcept {
  return impl_ == nullptr
             ? std::span<const spatial::PointId>{}
             : std::span<const spatial::PointId>{
                   impl_->prior_root_coverage_points};
}

std::span<const ExactDirectFrozenUnifiedLatentCarrierCoverage>
ExactDirectSparseCarrierOriginCapabilityResult::latent_carrier_coverages()
    const & noexcept {
  return impl_ == nullptr
             ? std::span<
                   const ExactDirectFrozenUnifiedLatentCarrierCoverage>{}
             : std::span<
                   const ExactDirectFrozenUnifiedLatentCarrierCoverage>{
                   impl_->latent_carrier_coverages};
}

std::span<const spatial::PointId>
ExactDirectSparseCarrierOriginCapabilityResult::
    latent_carrier_coverage_point_references() const & noexcept {
  return impl_ == nullptr
             ? std::span<const spatial::PointId>{}
             : std::span<const spatial::PointId>{
                   impl_->latent_carrier_coverage_points};
}

ExactDirectSparseCarrierOriginCapabilityDisposition
ExactDirectSparseCarrierOriginCapabilityResult::disposition()
    const noexcept {
  return impl_ == nullptr
             ? ExactDirectSparseCarrierOriginCapabilityDisposition::
                   not_certified
             : impl_->disposition;
}

ExactDirectSparseCarrierOriginCapabilityDecision
ExactDirectSparseCarrierOriginCapabilityResult::decision() const noexcept {
  return impl_ == nullptr
             ? ExactDirectSparseCarrierOriginCapabilityDecision::not_certified
             : impl_->decision;
}

bool ExactDirectSparseCarrierOriginCapabilityResult::
    no_external_root_or_coverage_span_consumed() const noexcept {
  return impl_ != nullptr &&
         impl_->no_external_root_or_coverage_span_consumed;
}

bool ExactDirectSparseCarrierOriginCapabilityResult::source_exactness_claimed()
    const noexcept {
  return impl_ != nullptr && impl_->source_exactness_claimed;
}

bool ExactDirectSparseCarrierOriginCapabilityResult::public_status_claimed()
    const noexcept {
  return impl_ != nullptr && impl_->public_status_claimed;
}

bool ExactDirectSparseCarrierOriginCapabilityResult::
    certified_exact_carrier_origin_capability() const noexcept {
  if (impl_ == nullptr || !impl_->minted ||
      impl_->schema_version !=
          direct_sparse_carrier_origin_capability_schema_version ||
      impl_->disposition !=
          ExactDirectSparseCarrierOriginCapabilityDisposition::complete ||
      (impl_->decision !=
           ExactDirectSparseCarrierOriginCapabilityDecision::
               complete_empty_carrier_set &&
       impl_->decision !=
           ExactDirectSparseCarrierOriginCapabilityDecision::
               complete_exact_rooted_latent_carrier_origins_and_coverages) ||
      !impl_->every_bound_closure_terminal_root_derived ||
      !impl_->forest_and_ledger_stamps_aligned ||
      !impl_->every_distinct_root_probed_once ||
      !impl_->every_probe_observed_for_ledger_stamp ||
      !impl_->requested_coverages_proof_bound ||
      !impl_->rooted_tokens_are_prior_root_ids ||
      !impl_->latent_tokens_are_terminal_root_handles ||
      !impl_->carrier_outputs_canonical || !impl_->carrier_only_scope ||
      impl_->equal_facet_resolution_claimed ||
      !impl_->no_external_root_or_coverage_span_consumed ||
      impl_->forest_or_ledger_logical_state_mutated ||
      impl_->source_exactness_claimed || impl_->public_status_claimed ||
      impl_->ledger_stamp.aligned_forest_stamp != impl_->forest_stamp ||
      impl_->origins.size() != impl_->counters.carrier_count ||
      impl_->carrier_resolutions.size() != impl_->origins.size() ||
      impl_->carrier_attachments.size() !=
          impl_->counters.distinct_terminal_root_count ||
      impl_->counters.distinct_terminal_root_count >
          impl_->counters.carrier_count ||
      impl_->prior_root_coverages.size() !=
          impl_->counters.rooted_terminal_root_count ||
      impl_->latent_carrier_coverages.size() !=
          impl_->counters.latent_terminal_root_count ||
      impl_->counters.active_root_probe_count !=
          impl_->counters.distinct_terminal_root_count ||
      impl_->counters.shared_terminal_reference_count !=
          impl_->counters.carrier_count -
              impl_->counters.distinct_terminal_root_count ||
      impl_->counters.rooted_terminal_root_count +
              impl_->counters.latent_terminal_root_count !=
          impl_->counters.distinct_terminal_root_count ||
      impl_->prior_root_coverage_points.size() +
              impl_->latent_carrier_coverage_points.size() !=
          impl_->counters.output_point_reference_count ||
      impl_->counters.carrier_count >
          impl_->requested_budget.maximum_carrier_count ||
      impl_->counters.distinct_terminal_root_count >
          impl_->requested_budget.maximum_distinct_terminal_root_count ||
      impl_->counters.output_point_reference_count >
          impl_->requested_budget.maximum_output_point_reference_count) {
    return false;
  }

  for (std::size_t index = 0U; index < impl_->origins.size(); ++index) {
    const auto& origin = impl_->origins[index];
    const auto& resolution = impl_->carrier_resolutions[index];
    if (origin.stable_source_facet_token_index !=
            resolution.stable_source_facet_token_index ||
        origin.token != resolution.token ||
        origin.prior_root_id != resolution.prior_root_id ||
        (index != 0U &&
         (impl_->origins[index - 1U].closure_seed_index >=
              origin.closure_seed_index ||
          impl_->origins[index - 1U].stable_source_facet_token_index >=
              origin.stable_source_facet_token_index)) ||
        (origin.kind == ExactDirectSparseCarrierOriginKind::rooted) !=
            origin.prior_root_id.has_value() ||
        origin.coverage_id == 0U) {
      return false;
    }
  }
  for (std::size_t index = 1U; index < impl_->carrier_attachments.size();
       ++index) {
    if (!typed_token_less(
            impl_->carrier_attachments[index - 1U].token,
            impl_->carrier_attachments[index].token)) {
      return false;
    }
  }

  std::size_t cursor = 0U;
  for (std::size_t index = 0U; index < impl_->prior_root_coverages.size();
       ++index) {
    const auto& coverage = impl_->prior_root_coverages[index];
    if (coverage.point_reference_offset != cursor ||
        !valid_slice(
            coverage.point_reference_offset,
            coverage.point_reference_count,
            impl_->prior_root_coverage_points.size()) ||
        (index != 0U &&
         impl_->prior_root_coverages[index - 1U].prior_root_id >=
             coverage.prior_root_id) ||
        !strict_points(std::span<const spatial::PointId>{
            impl_->prior_root_coverage_points}
                           .subspan(
                               coverage.point_reference_offset,
                               coverage.point_reference_count))) {
      return false;
    }
    cursor += coverage.point_reference_count;
  }
  if (cursor != impl_->prior_root_coverage_points.size()) {
    return false;
  }
  cursor = 0U;
  for (std::size_t index = 0U;
       index < impl_->latent_carrier_coverages.size();
       ++index) {
    const auto& coverage = impl_->latent_carrier_coverages[index];
    if (coverage.point_reference_offset != cursor ||
        !valid_slice(
            coverage.point_reference_offset,
            coverage.point_reference_count,
            impl_->latent_carrier_coverage_points.size()) ||
        (index != 0U &&
         impl_->latent_carrier_coverages[index - 1U]
                 .latent_carrier_token_id >=
             coverage.latent_carrier_token_id) ||
        !strict_points(std::span<const spatial::PointId>{
            impl_->latent_carrier_coverage_points}
                           .subspan(
                               coverage.point_reference_offset,
                               coverage.point_reference_count))) {
      return false;
    }
    cursor += coverage.point_reference_count;
  }
  return cursor == impl_->latent_carrier_coverage_points.size();
}

bool ExactDirectSparseCarrierOriginCapabilityResult::
    certified_budget_exhaustion() const noexcept {
  if (impl_ == nullptr || !impl_->minted ||
      impl_->disposition !=
          ExactDirectSparseCarrierOriginCapabilityDisposition::
              budget_exhausted ||
      !impl_->origins.empty() || !impl_->carrier_resolutions.empty() ||
      !impl_->carrier_attachments.empty() ||
      !impl_->prior_root_coverages.empty() ||
      !impl_->prior_root_coverage_points.empty() ||
      !impl_->latent_carrier_coverages.empty() ||
      !impl_->latent_carrier_coverage_points.empty()) {
    return false;
  }
  switch (impl_->decision) {
    case ExactDirectSparseCarrierOriginCapabilityDecision::
        no_carrier_budget_exhausted:
    case ExactDirectSparseCarrierOriginCapabilityDecision::
        no_distinct_terminal_root_budget_exhausted:
    case ExactDirectSparseCarrierOriginCapabilityDecision::
        no_active_root_probe_budget_exhausted:
    case ExactDirectSparseCarrierOriginCapabilityDecision::
        no_coverage_budget_exhausted:
    case ExactDirectSparseCarrierOriginCapabilityDecision::
        no_output_point_budget_exhausted:
      return true;
    default:
      return false;
  }
}

bool ExactDirectSparseCarrierOriginCapabilityResult::certified_rejection()
    const noexcept {
  if (impl_ == nullptr || !impl_->minted ||
      impl_->disposition !=
          ExactDirectSparseCarrierOriginCapabilityDisposition::rejected ||
      !impl_->origins.empty() || !impl_->carrier_resolutions.empty() ||
      !impl_->carrier_attachments.empty() ||
      !impl_->prior_root_coverages.empty() ||
      !impl_->prior_root_coverage_points.empty() ||
      !impl_->latent_carrier_coverages.empty() ||
      !impl_->latent_carrier_coverage_points.empty()) {
    return false;
  }
  switch (impl_->decision) {
    case ExactDirectSparseCarrierOriginCapabilityDecision::
        no_forest_rejected:
    case ExactDirectSparseCarrierOriginCapabilityDecision::
        no_closure_rejected:
    case ExactDirectSparseCarrierOriginCapabilityDecision::
        no_ledger_rejected:
    case ExactDirectSparseCarrierOriginCapabilityDecision::
        no_forest_ledger_stamp_alignment_rejected:
    case ExactDirectSparseCarrierOriginCapabilityDecision::
        no_input_shape_rejected:
    case ExactDirectSparseCarrierOriginCapabilityDecision::
        no_active_root_unobserved_rejected:
    case ExactDirectSparseCarrierOriginCapabilityDecision::
        no_coverage_rejected:
    case ExactDirectSparseCarrierOriginCapabilityDecision::
        no_token_namespace_rejected:
    case ExactDirectSparseCarrierOriginCapabilityDecision::
        no_allocation_failed:
      return true;
    default:
      return false;
  }
}

bool ExactDirectSparseCarrierOriginCapabilityResult::
    certified_fail_closed_contradiction() const noexcept {
  if (impl_ == nullptr || !impl_->minted ||
      impl_->disposition !=
          ExactDirectSparseCarrierOriginCapabilityDisposition::contradiction ||
      !impl_->origins.empty() || !impl_->carrier_resolutions.empty() ||
      !impl_->carrier_attachments.empty() ||
      !impl_->prior_root_coverages.empty() ||
      !impl_->prior_root_coverage_points.empty() ||
      !impl_->latent_carrier_coverages.empty() ||
      !impl_->latent_carrier_coverage_points.empty()) {
    return false;
  }
  return impl_->decision ==
             ExactDirectSparseCarrierOriginCapabilityDecision::
                 contradiction_active_root_probe ||
         impl_->decision ==
             ExactDirectSparseCarrierOriginCapabilityDecision::
                 contradiction_coverage ||
         impl_->decision ==
             ExactDirectSparseCarrierOriginCapabilityDecision::
                 contradiction_stamp_changed;
}

bool ExactDirectSparseCarrierOriginCapabilityResult::certified_outcome()
    const noexcept {
  return certified_exact_carrier_origin_capability() ||
         certified_budget_exhaustion() || certified_rejection() ||
         certified_fail_closed_contradiction();
}

bool ExactDirectSparseCarrierOriginCapabilityResult::certified_for(
    const ExactDirectSparseStableFacetForestStamp& expected_forest,
    const ExactDirectSparseRootLedgerStamp& expected_ledger) const noexcept {
  return impl_ != nullptr && impl_->forest_stamp == expected_forest &&
         impl_->ledger_stamp == expected_ledger && certified_outcome();
}

ExactDirectSparseCarrierOriginCapabilityResult
derive_exact_direct_sparse_carrier_origin_capability(
    const ExactDirectSparseStableFacetDescentClosureResult& closure,
    std::span<const ExactDirectSparseCarrierOriginSeedBinding> seed_bindings,
    const ExactDirectSparseStableFacetForest& forest,
    const ExactDirectSparseRootLedger& ledger,
    const ExactDirectSparseCarrierOriginCapabilityBudget& budget) {
  auto state = std::make_unique<
      ExactDirectSparseCarrierOriginCapabilityResult::Impl>();
  state->requested_budget = budget;
  state->forest_stamp = forest.current_stamp();
  state->ledger_stamp = ledger.current_stamp();

  const auto finish = [&state](
                          ExactDirectSparseCarrierOriginCapabilityDisposition
                              disposition,
                          ExactDirectSparseCarrierOriginCapabilityDecision
                              decision) {
    if (disposition !=
        ExactDirectSparseCarrierOriginCapabilityDisposition::complete) {
      state->origins.clear();
      state->carrier_resolutions.clear();
      state->carrier_attachments.clear();
      state->prior_root_coverages.clear();
      state->prior_root_coverage_points.clear();
      state->latent_carrier_coverages.clear();
      state->latent_carrier_coverage_points.clear();
    }
    state->disposition = disposition;
    state->decision = decision;
    state->minted = true;
    return ExactDirectSparseCarrierOriginCapabilityResult{
        std::move(state)};
  };

  if (!forest.certified_structure_only_forest()) {
    return finish(
        ExactDirectSparseCarrierOriginCapabilityDisposition::rejected,
        ExactDirectSparseCarrierOriginCapabilityDecision::no_forest_rejected);
  }
  if (!closure.certified_complete_relative_positive_closure() ||
      !closure.certified_for(state->forest_stamp)) {
    return finish(
        ExactDirectSparseCarrierOriginCapabilityDisposition::rejected,
        ExactDirectSparseCarrierOriginCapabilityDecision::
            no_closure_rejected);
  }
  if (!ledger.certified_structure_only_ledger()) {
    return finish(
        ExactDirectSparseCarrierOriginCapabilityDisposition::rejected,
        ExactDirectSparseCarrierOriginCapabilityDecision::no_ledger_rejected);
  }
  if (state->ledger_stamp.aligned_forest_stamp != state->forest_stamp) {
    return finish(
        ExactDirectSparseCarrierOriginCapabilityDisposition::rejected,
        ExactDirectSparseCarrierOriginCapabilityDecision::
            no_forest_ledger_stamp_alignment_rejected);
  }
  state->forest_and_ledger_stamps_aligned = true;

  if (seed_bindings.size() > budget.maximum_carrier_count) {
    state->counters.carrier_count = budget.maximum_carrier_count;
    return finish(
        ExactDirectSparseCarrierOriginCapabilityDisposition::budget_exhausted,
        ExactDirectSparseCarrierOriginCapabilityDecision::
            no_carrier_budget_exhausted);
  }
  state->counters.carrier_count = seed_bindings.size();
  const auto projections = closure.seed_projections();
  const auto nodes = closure.nodes();

  try {
    std::vector<TerminalReference> terminal_references;
    terminal_references.reserve(seed_bindings.size());
    std::vector<ExactDirectSparseStableFacetHandle> terminal_roots;
    terminal_roots.reserve(seed_bindings.size());
    for (std::size_t index = 0U; index < seed_bindings.size(); ++index) {
      const auto& binding = seed_bindings[index];
      if (binding.closure_seed_index >= projections.size() ||
          (index != 0U &&
           (seed_bindings[index - 1U].closure_seed_index >=
                binding.closure_seed_index ||
            seed_bindings[index - 1U].stable_source_facet_token_index >=
                binding.stable_source_facet_token_index))) {
        return finish(
            ExactDirectSparseCarrierOriginCapabilityDisposition::rejected,
            ExactDirectSparseCarrierOriginCapabilityDecision::
                no_input_shape_rejected);
      }
      const auto& projection = projections[binding.closure_seed_index];
      if (projection.seed_index != binding.closure_seed_index ||
          binding.stable_source_facet_token_index >=
              state->forest_stamp.stable_facet_token_count ||
          projection.closure_disposition !=
              ExactDirectSparseStableFacetDescentClosureDisposition::
                  relative_positive ||
          projection.terminal_node_index >= nodes.size()) {
        return finish(
            ExactDirectSparseCarrierOriginCapabilityDisposition::rejected,
            ExactDirectSparseCarrierOriginCapabilityDecision::
                no_input_shape_rejected);
      }
      const auto& terminal = nodes[projection.terminal_node_index];
      if (terminal.node_index != projection.terminal_node_index ||
          terminal.kind !=
              ExactDirectSparseStableFacetDescentNodeKind::
                  stable_positive_terminal ||
          !terminal.terminal_pointer_certified ||
          terminal.terminal_node_index != terminal.node_index ||
          !terminal.stable_positive_attachment.has_value() ||
          terminal.stable_positive_attachment->component_size == 0U) {
        return finish(
            ExactDirectSparseCarrierOriginCapabilityDisposition::rejected,
            ExactDirectSparseCarrierOriginCapabilityDecision::
                no_input_shape_rejected);
      }
      const auto root =
          terminal.stable_positive_attachment->root_handle;
      terminal_references.push_back({binding, root});
      terminal_roots.push_back(root);
    }
    state->every_bound_closure_terminal_root_derived = true;

    std::sort(terminal_roots.begin(), terminal_roots.end());
    terminal_roots.erase(
        std::unique(terminal_roots.begin(), terminal_roots.end()),
        terminal_roots.end());
    if (terminal_roots.size() >
        budget.maximum_distinct_terminal_root_count) {
      state->counters.distinct_terminal_root_count =
          budget.maximum_distinct_terminal_root_count;
      return finish(
          ExactDirectSparseCarrierOriginCapabilityDisposition::
              budget_exhausted,
          ExactDirectSparseCarrierOriginCapabilityDecision::
              no_distinct_terminal_root_budget_exhausted);
    }
    state->counters.distinct_terminal_root_count = terminal_roots.size();
    state->counters.shared_terminal_reference_count =
        seed_bindings.size() - terminal_roots.size();

    std::vector<ExactDirectSparseRootLedgerActiveRootProbeResult> proofs;
    proofs.reserve(terminal_roots.size());
    for (const auto root : terminal_roots) {
      auto proof = ledger.probe_active_root(root, budget.active_root_probe);
      ++state->counters.active_root_probe_count;
      if (forest.current_stamp() != state->forest_stamp ||
          ledger.current_stamp() != state->ledger_stamp) {
        return finish(
            ExactDirectSparseCarrierOriginCapabilityDisposition::
                contradiction,
            ExactDirectSparseCarrierOriginCapabilityDecision::
                contradiction_stamp_changed);
      }
      if (proof.certified_budget_exhaustion() &&
          proof.certified_for(state->ledger_stamp)) {
        return finish(
            ExactDirectSparseCarrierOriginCapabilityDisposition::
                budget_exhausted,
            ExactDirectSparseCarrierOriginCapabilityDecision::
                no_active_root_probe_budget_exhausted);
      }
      if (proof.certified_unobserved() &&
          proof.certified_for(state->ledger_stamp)) {
        return finish(
            ExactDirectSparseCarrierOriginCapabilityDisposition::rejected,
            ExactDirectSparseCarrierOriginCapabilityDecision::
                no_active_root_unobserved_rejected);
      }
      if (proof.certified_fail_closed_contradiction() &&
          proof.certified_for(state->ledger_stamp)) {
        return finish(
            ExactDirectSparseCarrierOriginCapabilityDisposition::
                contradiction,
            ExactDirectSparseCarrierOriginCapabilityDecision::
                contradiction_active_root_probe);
      }
      if (!proof.certified_observed() ||
          !proof.certified_for(state->ledger_stamp) ||
          proof.requested_forest_root_handle != root ||
          proof.entry.forest_root_handle != root) {
        return finish(
            ExactDirectSparseCarrierOriginCapabilityDisposition::rejected,
            ExactDirectSparseCarrierOriginCapabilityDecision::
                no_active_root_unobserved_rejected);
      }
      proofs.push_back(std::move(proof));
    }
    state->every_distinct_root_probed_once = true;
    state->every_probe_observed_for_ledger_stamp = true;

    const auto materialized =
        ledger.materialize_active_coverages_from_active_root_proofs(
            proofs, budget.coverage);
    if (forest.current_stamp() != state->forest_stamp ||
        ledger.current_stamp() != state->ledger_stamp) {
      return finish(
          ExactDirectSparseCarrierOriginCapabilityDisposition::contradiction,
          ExactDirectSparseCarrierOriginCapabilityDecision::
              contradiction_stamp_changed);
    }
    if (materialized.certified_budget_exhaustion() &&
        materialized.certified_for(state->ledger_stamp)) {
      return finish(
          ExactDirectSparseCarrierOriginCapabilityDisposition::
              budget_exhausted,
          ExactDirectSparseCarrierOriginCapabilityDecision::
              no_coverage_budget_exhausted);
    }
    if (materialized.certified_fail_closed_contradiction() &&
        materialized.certified_for(state->ledger_stamp)) {
      return finish(
          ExactDirectSparseCarrierOriginCapabilityDisposition::contradiction,
          ExactDirectSparseCarrierOriginCapabilityDecision::
              contradiction_coverage);
    }
    if (!materialized.certified_requested_only_materialization() ||
        !materialized.certified_for(state->ledger_stamp)) {
      return finish(
          ExactDirectSparseCarrierOriginCapabilityDisposition::rejected,
          ExactDirectSparseCarrierOriginCapabilityDecision::
              no_coverage_rejected);
    }
    state->requested_coverages_proof_bound = true;

    const auto coverage_records = materialized.records();
    const auto coverage_points = materialized.point_ids();
    if (coverage_records.size() != terminal_roots.size()) {
      return finish(
          ExactDirectSparseCarrierOriginCapabilityDisposition::contradiction,
          ExactDirectSparseCarrierOriginCapabilityDecision::
              contradiction_coverage);
    }
    if (coverage_points.size() >
        budget.maximum_output_point_reference_count) {
      state->counters.output_point_reference_count =
          budget.maximum_output_point_reference_count;
      return finish(
          ExactDirectSparseCarrierOriginCapabilityDisposition::
              budget_exhausted,
          ExactDirectSparseCarrierOriginCapabilityDecision::
              no_output_point_budget_exhausted);
    }

    std::vector<RootAuthority> authorities;
    authorities.reserve(terminal_roots.size());
    for (std::size_t index = 0U; index < terminal_roots.size(); ++index) {
      const auto& proof = proofs[index];
      const auto& coverage = coverage_records[index];
      if (coverage.forest_root_handle != terminal_roots[index] ||
          coverage.forest_root_handle != proof.entry.forest_root_handle ||
          coverage.prior_root_id != proof.entry.prior_root_id ||
          coverage.coverage_id != proof.entry.coverage_id ||
          coverage.coverage_id == 0U ||
          !valid_slice(
              coverage.point_offset,
              coverage.point_count,
              coverage_points.size())) {
        return finish(
            ExactDirectSparseCarrierOriginCapabilityDisposition::
                contradiction,
            ExactDirectSparseCarrierOriginCapabilityDecision::
                contradiction_coverage);
      }
      RootAuthority authority;
      authority.root_handle = terminal_roots[index];
      authority.prior_root_id = proof.entry.prior_root_id;
      authority.coverage_id = coverage.coverage_id;
      authority.materialized_point_offset = coverage.point_offset;
      authority.materialized_point_count = coverage.point_count;
      if (proof.certified_observed_rooted()) {
        if (!authority.prior_root_id.has_value()) {
          return finish(
              ExactDirectSparseCarrierOriginCapabilityDisposition::
                  contradiction,
              ExactDirectSparseCarrierOriginCapabilityDecision::
                  contradiction_active_root_probe);
        }
        authority.kind = ExactDirectSparseCarrierOriginKind::rooted;
        authority.token = {
            ExactFrozenIncidenceTokenKind::rooted_carrier,
            *authority.prior_root_id};
      } else if (proof.certified_observed_latent()) {
        if (authority.prior_root_id.has_value() ||
            !std::in_range<ExactFrozenIncidenceTokenId>(
                authority.root_handle)) {
          return finish(
              ExactDirectSparseCarrierOriginCapabilityDisposition::rejected,
              ExactDirectSparseCarrierOriginCapabilityDecision::
                  no_token_namespace_rejected);
        }
        authority.kind = ExactDirectSparseCarrierOriginKind::latent;
        authority.token = {
            ExactFrozenIncidenceTokenKind::latent_carrier,
            static_cast<ExactFrozenIncidenceTokenId>(
                authority.root_handle)};
      } else {
        return finish(
            ExactDirectSparseCarrierOriginCapabilityDisposition::
                contradiction,
            ExactDirectSparseCarrierOriginCapabilityDecision::
                contradiction_active_root_probe);
      }
      authorities.push_back(authority);
    }

    std::vector<std::size_t> rooted_indices;
    std::vector<std::size_t> latent_indices;
    rooted_indices.reserve(authorities.size());
    latent_indices.reserve(authorities.size());
    for (std::size_t index = 0U; index < authorities.size(); ++index) {
      if (authorities[index].kind ==
          ExactDirectSparseCarrierOriginKind::rooted) {
        rooted_indices.push_back(index);
      } else {
        latent_indices.push_back(index);
      }
    }
    std::sort(
        rooted_indices.begin(),
        rooted_indices.end(),
        [&authorities](std::size_t left, std::size_t right) {
          return *authorities[left].prior_root_id <
                 *authorities[right].prior_root_id;
        });
    std::sort(
        latent_indices.begin(),
        latent_indices.end(),
        [&authorities](std::size_t left, std::size_t right) {
          return authorities[left].token.token_id <
                 authorities[right].token.token_id;
        });

    state->prior_root_coverages.reserve(rooted_indices.size());
    state->latent_carrier_coverages.reserve(latent_indices.size());
    state->carrier_attachments.reserve(authorities.size());
    state->prior_root_coverage_points.reserve(coverage_points.size());
    state->latent_carrier_coverage_points.reserve(coverage_points.size());
    for (std::size_t ordered = 0U; ordered < rooted_indices.size();
         ++ordered) {
      auto& authority = authorities[rooted_indices[ordered]];
      authority.typed_coverage_record_index = ordered;
      const auto points = coverage_points.subspan(
          authority.materialized_point_offset,
          authority.materialized_point_count);
      const std::size_t offset = state->prior_root_coverage_points.size();
      state->prior_root_coverage_points.insert(
          state->prior_root_coverage_points.end(),
          points.begin(),
          points.end());
      state->prior_root_coverages.push_back(
          {*authority.prior_root_id, offset, points.size()});
      state->carrier_attachments.push_back(
          {authority.token,
           authority.root_handle,
           authority.prior_root_id});
    }
    for (std::size_t ordered = 0U; ordered < latent_indices.size();
         ++ordered) {
      auto& authority = authorities[latent_indices[ordered]];
      authority.typed_coverage_record_index = ordered;
      const auto points = coverage_points.subspan(
          authority.materialized_point_offset,
          authority.materialized_point_count);
      const std::size_t offset =
          state->latent_carrier_coverage_points.size();
      state->latent_carrier_coverage_points.insert(
          state->latent_carrier_coverage_points.end(),
          points.begin(),
          points.end());
      state->latent_carrier_coverages.push_back(
          {authority.token.token_id, offset, points.size()});
      state->carrier_attachments.push_back(
          {authority.token, authority.root_handle, std::nullopt});
    }

    state->origins.reserve(terminal_references.size());
    state->carrier_resolutions.reserve(terminal_references.size());
    for (const auto& terminal : terminal_references) {
      const auto found = std::lower_bound(
          authorities.begin(),
          authorities.end(),
          terminal.root_handle,
          [](const RootAuthority& authority,
             ExactDirectSparseStableFacetHandle root) {
            return authority.root_handle < root;
          });
      if (found == authorities.end() ||
          found->root_handle != terminal.root_handle) {
        return finish(
            ExactDirectSparseCarrierOriginCapabilityDisposition::
                contradiction,
            ExactDirectSparseCarrierOriginCapabilityDecision::
                contradiction_coverage);
      }
      state->origins.push_back(
          {terminal.binding.closure_seed_index,
           terminal.binding.stable_source_facet_token_index,
           found->root_handle,
           found->token,
           found->prior_root_id,
           found->coverage_id,
           found->typed_coverage_record_index,
           found->kind});
      state->carrier_resolutions.push_back(
          {terminal.binding.stable_source_facet_token_index,
           found->token,
           found->prior_root_id});
    }

    state->counters.rooted_terminal_root_count = rooted_indices.size();
    state->counters.latent_terminal_root_count = latent_indices.size();
    state->counters.output_point_reference_count = coverage_points.size();
    state->rooted_tokens_are_prior_root_ids = true;
    state->latent_tokens_are_terminal_root_handles = true;
    state->carrier_outputs_canonical = true;
    state->no_external_root_or_coverage_span_consumed = true;

    if (forest.current_stamp() != state->forest_stamp ||
        ledger.current_stamp() != state->ledger_stamp) {
      return finish(
          ExactDirectSparseCarrierOriginCapabilityDisposition::contradiction,
          ExactDirectSparseCarrierOriginCapabilityDecision::
              contradiction_stamp_changed);
    }
    return finish(
        ExactDirectSparseCarrierOriginCapabilityDisposition::complete,
        seed_bindings.empty()
            ? ExactDirectSparseCarrierOriginCapabilityDecision::
                  complete_empty_carrier_set
            : ExactDirectSparseCarrierOriginCapabilityDecision::
                  complete_exact_rooted_latent_carrier_origins_and_coverages);
  } catch (const std::bad_alloc&) {
    return finish(
        ExactDirectSparseCarrierOriginCapabilityDisposition::rejected,
        ExactDirectSparseCarrierOriginCapabilityDecision::
            no_allocation_failed);
  }
}

}  // namespace morsehgp3d::hierarchy
