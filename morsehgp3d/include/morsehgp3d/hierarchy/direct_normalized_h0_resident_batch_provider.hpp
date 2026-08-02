#pragma once

#include "morsehgp3d/hierarchy/direct_normalized_h0_incidence_reduction_authority.hpp"
#include "morsehgp3d/hierarchy/direct_sparse_unified_level_plan.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_normalized_h0_resident_batch_provider_schema_version = 1U;
inline constexpr std::string_view
    direct_normalized_h0_resident_batch_provider_backend = "reference_cpu";
inline constexpr std::string_view
    direct_normalized_h0_resident_batch_provider_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_normalized_h0_resident_batch_provider_mode =
        "authenticated_structure_only_windowed_normalized_source_provider_v1";
inline constexpr std::string_view
    direct_normalized_h0_resident_batch_provider_public_status =
        "not_claimed";
inline constexpr std::string_view
    direct_normalized_h0_resident_batch_provider_deployment_status =
        "architecture_only_reference_cpu_window_provider_not_cuda_not_"
        "massive_qualified";

// The provider budget is per borrowed window, except for the batch scan cap
// used by the one authentication replay.  It never authorizes a complete
// source arena.  Stable facet ids name the already-certified source namespace;
// the resident may retain only ids and keys that have appeared in a committed
// prefix.
struct ExactDirectNormalizedH0ResidentBatchProviderBudget {
  std::size_t maximum_batch_scan_count{};
  std::size_t maximum_window_facet_count{};
  std::size_t maximum_window_facet_point_count{};
  std::size_t maximum_window_direct_reference_count{};
  std::size_t maximum_window_residual_reference_count{};
  std::size_t maximum_window_coface_facet_reference_count{};
  std::size_t maximum_window_logical_entry_count{};

  friend bool operator==(
      const ExactDirectNormalizedH0ResidentBatchProviderBudget&,
      const ExactDirectNormalizedH0ResidentBatchProviderBudget&) = default;
};

// O(1) scientific identity and terminal chain.  The scalar
// stable_facet_token_count is a namespace/capacity fact, not a resident facet
// catalog.  No key, coface or incidence is owned by this object.
struct ExactDirectNormalizedH0ResidentSourceManifest {
  static constexpr std::string_view backend =
      direct_normalized_h0_resident_batch_provider_backend;
  static constexpr std::string_view profile =
      direct_normalized_h0_resident_batch_provider_profile;
  static constexpr std::string_view mode =
      direct_normalized_h0_resident_batch_provider_mode;
  static constexpr std::string_view public_status =
      direct_normalized_h0_resident_batch_provider_public_status;
  static constexpr std::string_view deployment_status =
      direct_normalized_h0_resident_batch_provider_deployment_status;

  std::uint32_t schema_version{
      direct_normalized_h0_resident_batch_provider_schema_version};
  std::size_t point_count{};
  std::size_t requested_maximum_order{};
  std::size_t effective_maximum_order{};
  std::size_t batch_count{};
  std::size_t normalized_coface_count{};
  std::size_t stable_facet_token_count{};
  std::size_t direct_reference_count{};
  std::size_t residual_reference_count{};
  std::size_t coface_facet_reference_count{};
  contract::CanonicalId source_pair_canonical_cloud_digest{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  contract::CanonicalId source_pair_semantic_digest{};
  contract::CanonicalId source_higher_semantic_digest{};
  contract::CanonicalId source_terminal_digest{};
  contract::CanonicalId source_gateway_scientific_identity_digest{};
  contract::CanonicalId horizontal_incidence_authority_digest{};
  contract::CanonicalId source_identity_digest{};
  contract::CanonicalId initial_batch_chain_digest{};
  contract::CanonicalId final_batch_chain_digest{};
  contract::CanonicalId manifest_digest{};
  bool horizontal_incidence_reduction_certified{false};
  bool incidence_complete_reduction_proved{false};
  bool stable_facet_ids_are_source_global_and_immutable{false};
  bool batches_dense_complete_and_strictly_level_ordered{false};
  bool exact_rational_levels_retained{false};
  bool no_complete_source_plan_owned{false};
  bool no_global_facet_coface_incidence_or_gamma_catalog_owned{false};
  bool no_star_cell_or_higher_order_delaunay_owned{false};
  bool source_exactness_claimed{false};
  bool vertical_maps_complete{false};
  bool public_status_claimed{false};

  [[nodiscard]] bool certified() const noexcept;

  friend bool operator==(
      const ExactDirectNormalizedH0ResidentSourceManifest&,
      const ExactDirectNormalizedH0ResidentSourceManifest&) = default;
};

// A window-local index is used only while rebuilding one frozen batch.  The
// stable source index survives in the transcript and is also the resident
// component-handle namespace.  Records are sorted by stable source index.
struct ExactDirectNormalizedH0ResidentWindowFacetToken {
  std::size_t window_facet_token_index{};
  std::size_t stable_source_facet_token_index{};
  ExactDirectSparseFacetKey facet_key{};

  friend bool operator==(
      const ExactDirectNormalizedH0ResidentWindowFacetToken&,
      const ExactDirectNormalizedH0ResidentWindowFacetToken&) = default;
};

// One synchronously borrowed normalized batch.  The standard direct,
// residual and coface-reference records use window-local facet indices, while
// their source record/coface indices remain global.  No span may be retained
// after the provider callback returns; a resident ticket instead owns a
// bounded copy of this one window and its local-to-stable translation.
struct ExactDirectNormalizedH0ResidentBatchWindow {
  std::uint32_t schema_version{
      direct_normalized_h0_resident_batch_provider_schema_version};
  contract::CanonicalId manifest_digest{};
  contract::CanonicalId source_identity_digest{};
  contract::CanonicalId source_chain_digest{};
  contract::CanonicalId batch_identity_digest{};
  contract::CanonicalId successor_chain_digest{};
  std::size_t source_batch_index{};
  std::size_t source_future_snapshot_index{};
  exact::ExactLevel squared_level{};
  std::size_t order{};
  std::span<const ExactDirectNormalizedH0ResidentWindowFacetToken>
      facet_tokens;
  std::span<const ExactDirectSparseUnifiedLevelPlanDirectReference>
      direct_references;
  std::span<const ExactDirectSparseUnifiedLevelPlanResidualReference>
      residual_references;
  std::span<const ExactDirectSparseUnifiedLevelPlanCofaceFacetReference>
      coface_facet_references;
  bool scientific_source_freshly_recertified{false};
  bool resident_records_borrowed_without_copy{false};
  bool synchronous_lifetime_only{true};
  bool no_future_facet_or_batch_payload_retained{false};
  bool no_complete_source_plan_materialized{false};
  bool no_global_facet_coface_incidence_or_gamma_catalog_materialized{false};
  bool public_status_claimed{false};

  [[nodiscard]] bool certified_relative_to(
      const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
      const ExactDirectNormalizedH0ResidentBatchProviderBudget& budget) const;
};

using ExactDirectNormalizedH0ResidentBatchConsumerFunction = bool (*)(
    void*, const ExactDirectNormalizedH0ResidentBatchWindow&);

class ExactDirectNormalizedH0ResidentBatchConsumerView {
 public:
  ExactDirectNormalizedH0ResidentBatchConsumerView() = default;

  template <
      class Consumer,
      std::enable_if_t<
          !std::is_same_v<
              std::remove_cvref_t<Consumer>,
              ExactDirectNormalizedH0ResidentBatchConsumerView> &&
              !std::is_pointer_v<std::remove_cvref_t<Consumer>> &&
              !std::is_function_v<std::remove_reference_t<Consumer>> &&
              !std::is_const_v<std::remove_reference_t<Consumer>> &&
              std::is_invocable_r_v<
                  bool,
                  Consumer&,
                  const ExactDirectNormalizedH0ResidentBatchWindow&>,
          int> = 0>
  ExactDirectNormalizedH0ResidentBatchConsumerView(
      Consumer& consumer) noexcept
      : state_(static_cast<void*>(std::addressof(consumer))),
        function_([](
                      void* state,
                      const ExactDirectNormalizedH0ResidentBatchWindow&
                          window) -> bool {
          return static_cast<bool>(
              std::invoke(*static_cast<Consumer*>(state), window));
        }) {}

  [[nodiscard]] bool operator()(
      const ExactDirectNormalizedH0ResidentBatchWindow& window) const {
    return function_ != nullptr && function_(state_, window);
  }
  [[nodiscard]] explicit operator bool() const noexcept {
    return state_ != nullptr && function_ != nullptr;
  }

 private:
  void* state_{};
  ExactDirectNormalizedH0ResidentBatchConsumerFunction function_{};
};

enum class ExactDirectNormalizedH0ResidentBatchVisitDecision
    : std::uint8_t {
  no_provider,
  no_batch_out_of_range,
  no_window_inconsistent,
  no_consumer_rejected,
  complete_synchronous_visit,
};

using ExactDirectNormalizedH0ResidentBatchProviderFunction =
    ExactDirectNormalizedH0ResidentBatchVisitDecision (*)(
        void*,
        std::size_t,
        ExactDirectNormalizedH0ResidentBatchConsumerView);

// Non-owning type-erased provider.  A product provider may read exactly one
// archive/device-to-host window into private scratch, call the consumer, then
// release it before returning.  Calls on one provider are externally
// serialized and the provider must outlive every resident session using it.
class ExactDirectNormalizedH0ResidentBatchProviderView {
 public:
  ExactDirectNormalizedH0ResidentBatchProviderView() = default;

  template <
      class Provider,
      std::enable_if_t<
          !std::is_same_v<
              std::remove_cvref_t<Provider>,
              ExactDirectNormalizedH0ResidentBatchProviderView> &&
              !std::is_pointer_v<std::remove_cvref_t<Provider>> &&
              !std::is_function_v<std::remove_reference_t<Provider>> &&
              !std::is_const_v<std::remove_reference_t<Provider>> &&
              std::is_invocable_r_v<
                  ExactDirectNormalizedH0ResidentBatchVisitDecision,
                  Provider&,
                  std::size_t,
                  ExactDirectNormalizedH0ResidentBatchConsumerView>,
          int> = 0>
  ExactDirectNormalizedH0ResidentBatchProviderView(Provider& provider) noexcept
      : state_(static_cast<void*>(std::addressof(provider))),
        function_([](
                      void* state,
                      std::size_t batch_index,
                      ExactDirectNormalizedH0ResidentBatchConsumerView
                          consumer)
                      -> ExactDirectNormalizedH0ResidentBatchVisitDecision {
          return std::invoke(
              *static_cast<Provider*>(state), batch_index, consumer);
        }) {}

  [[nodiscard]] ExactDirectNormalizedH0ResidentBatchVisitDecision operator()(
      std::size_t batch_index,
      ExactDirectNormalizedH0ResidentBatchConsumerView consumer) const {
    return function_ == nullptr
        ? ExactDirectNormalizedH0ResidentBatchVisitDecision::no_provider
        : function_(state_, batch_index, consumer);
  }
  [[nodiscard]] explicit operator bool() const noexcept {
    return state_ != nullptr && function_ != nullptr;
  }

  [[nodiscard]] const void* identity() const noexcept { return state_; }

 private:
  void* state_{};
  ExactDirectNormalizedH0ResidentBatchProviderFunction function_{};
};

enum class ExactDirectNormalizedH0ResidentProviderVerificationDecision
    : std::uint8_t {
  not_certified,
  no_manifest_rejected,
  no_provider,
  no_batch_budget_exhausted,
  no_window_rejected,
  no_chain_or_order_rejected,
  no_source_totals_rejected,
  complete_authenticated_stream_replay,
};

struct ExactDirectNormalizedH0ResidentProviderVerification {
  // Process-local binding only.  It proves that this replay used the exact
  // non-owning provider object named by ProviderView; it is not serialized and
  // does not promote the source or the public hierarchy to exact status.
  const void* provider_identity{};
  contract::CanonicalId manifest_digest{};
  std::size_t batch_visit_count{};
  std::size_t consumer_callback_count{};
  std::size_t direct_reference_count{};
  std::size_t residual_reference_count{};
  std::size_t coface_facet_reference_count{};
  contract::CanonicalId final_batch_chain_digest{};
  bool provider_identity_bound{false};
  bool manifest_certified{false};
  bool every_window_freshly_recertified{false};
  bool batches_dense_and_strictly_level_ordered{false};
  bool final_chain_matches_manifest{false};
  bool exact_source_totals_match_manifest{false};
  bool no_window_payload_retained{false};
  bool multiple_consumer_callbacks_observed{false};
  bool source_exactness_claimed{false};
  bool public_status_claimed{false};
  bool result_certified{false};
  ExactDirectNormalizedH0ResidentProviderVerificationDecision decision{
      ExactDirectNormalizedH0ResidentProviderVerificationDecision::
          not_certified};

  friend bool operator==(
      const ExactDirectNormalizedH0ResidentProviderVerification&,
      const ExactDirectNormalizedH0ResidentProviderVerification&) = default;
};

// Ticket-owned one-batch compatibility image.  It is never a certified
// global UnifiedLevelPlan: it contains one batch, window-local indices and a
// bijective local-to-stable translation only.
struct ExactDirectNormalizedH0ResidentOwnedBatchWindow {
  ExactDirectSparseUnifiedLevelPlanResult local_plan{};
  std::vector<std::size_t> local_to_stable_facet_token_indices;
  contract::CanonicalId manifest_digest{};
  contract::CanonicalId source_chain_digest{};
  contract::CanonicalId batch_identity_digest{};
  contract::CanonicalId successor_chain_digest{};
  std::size_t source_batch_index{};
  bool exact_window_copied{false};
  bool no_future_payload_owned{false};

  [[nodiscard]] bool certified_relative_to(
      const ExactDirectNormalizedH0ResidentSourceManifest& manifest) const
      noexcept;
};

[[nodiscard]] ExactDirectNormalizedH0ResidentProviderVerification
verify_exact_direct_normalized_h0_resident_batch_provider(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    ExactDirectNormalizedH0ResidentBatchProviderView provider,
    const ExactDirectNormalizedH0ResidentBatchProviderBudget& budget);

[[nodiscard]] ExactDirectNormalizedH0ResidentOwnedBatchWindow
copy_exact_direct_normalized_h0_resident_batch_window(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    const ExactDirectNormalizedH0ResidentBatchWindow& window,
    const ExactDirectNormalizedH0ResidentBatchProviderBudget& budget);

// Compatibility-only builders for bounded fixtures and migration checks.
// They consume an already materialized v7 compatibility plan but never make a
// second whole-plan copy.  Product/massive providers must obtain the same
// manifest and windows from their recertified stream/archive instead.
[[nodiscard]] ExactDirectNormalizedH0ResidentSourceManifest
build_exact_direct_normalized_h0_resident_source_manifest_from_compatibility_plan(
    const ExactDirectSparseUnifiedLevelPlanResult& compatibility_plan,
    const ExactDirectNormalizedH0IncidenceReductionAuthority&
        incidence_authority);

struct ExactDirectNormalizedH0ResidentCompatibilityWindowScratch {
  std::vector<ExactDirectNormalizedH0ResidentWindowFacetToken> facet_tokens;
  std::vector<ExactDirectSparseUnifiedLevelPlanDirectReference>
      direct_references;
  std::vector<ExactDirectSparseUnifiedLevelPlanResidualReference>
      residual_references;
  std::vector<ExactDirectSparseUnifiedLevelPlanCofaceFacetReference>
      coface_facet_references;
};

[[nodiscard]] ExactDirectNormalizedH0ResidentBatchWindow
borrow_exact_direct_normalized_h0_resident_compatibility_window(
    const ExactDirectNormalizedH0ResidentSourceManifest& manifest,
    const ExactDirectSparseUnifiedLevelPlanResult& compatibility_plan,
    std::size_t batch_index,
    const contract::CanonicalId& source_chain_digest,
    ExactDirectNormalizedH0ResidentCompatibilityWindowScratch& scratch);

}  // namespace morsehgp3d::hierarchy
