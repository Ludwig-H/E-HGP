#pragma once

#include "morsehgp3d/hierarchy/direct_morse_forest_journal.hpp"

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
    direct_morse_forest_source_authority_schema_version = 1U;
inline constexpr std::string_view
    direct_morse_forest_source_authority_backend = "reference_cpu";
inline constexpr std::string_view
    direct_morse_forest_source_authority_profile = "hgp_reduced";
inline constexpr std::string_view
    direct_morse_forest_source_authority_mode =
        "bounded_unified_batch_authority_view";
inline constexpr std::string_view
    direct_morse_forest_source_authority_public_status = "not_claimed";

// Immutable O(1) authority summary.  Construction freshly replays the
// resident cloud/facade/event/seed contract once.  A massive provider may
// later obtain the same manifest from a separately recertified archive; the
// reducer-facing seam needs no resident source arena once this summary and a
// batch provider are available.
struct ExactDirectMorseForestSourceManifest {
  std::uint32_t schema_version{
      direct_morse_forest_source_authority_schema_version};
  std::uint32_t event_journal_schema_version{};
  std::uint32_t seed_journal_schema_version{};
  ExactDirectSaddleArmSeedBudget trusted_seed_budget{};
  std::size_t point_count{};
  std::size_t requested_maximum_order{};
  std::size_t effective_maximum_order{};
  std::size_t direct_event_count{};
  std::size_t logical_event_projection_count{};
  std::size_t logical_role_record_count{};
  std::size_t batch_count{};
  std::size_t direct_birth_count{};
  std::size_t family_count{};
  std::size_t arm_seed_count{};
  std::size_t logical_event_journal_storage_entry_count{};
  std::size_t logical_seed_journal_storage_entry_count{};
  contract::CanonicalId source_pair_canonical_cloud_digest{};
  contract::CanonicalId source_higher_canonical_cloud_digest{};
  contract::CanonicalId source_pair_semantic_digest{};
  contract::CanonicalId source_higher_semantic_digest{};
  contract::CanonicalId source_identity_digest{};
  contract::CanonicalId initial_batch_chain_digest{};
  contract::CanonicalId final_batch_chain_digest{};
  contract::CanonicalId manifest_digest{};
  bool source_event_journal_freshly_verified{false};
  bool source_seed_journal_freshly_verified{false};
  bool singleton_prefix_implicit{false};
  bool batch_partition_dense_and_complete{false};
  bool forbidden_global_structure_materialized{false};
  bool public_status_claimed{false};

  [[nodiscard]] bool certified() const noexcept;

  friend bool operator==(
      const ExactDirectMorseForestSourceManifest&,
      const ExactDirectMorseForestSourceManifest&) = default;
};

// Non-owning indexed lookup.  The returned pointer is valid only for the
// synchronous source-window callback that supplied this view.
class ExactDirectMorseForestProjectionLookupView {
 public:
  using Function = const ExactDirectMorseEventProjection* (*)(
      const void*, std::size_t) noexcept;

  constexpr ExactDirectMorseForestProjectionLookupView() noexcept = default;
  constexpr ExactDirectMorseForestProjectionLookupView(
      const void* state, Function function) noexcept
      : state_(state), function_(function) {}

  [[nodiscard]] const ExactDirectMorseEventProjection* find(
      std::size_t logical_projection_index) const noexcept {
    return function_ == nullptr
               ? nullptr
               : function_(state_, logical_projection_index);
  }
  [[nodiscard]] explicit operator bool() const noexcept {
    return state_ != nullptr && function_ != nullptr;
  }

 private:
  const void* state_{};
  Function function_{};
};

class ExactDirectMorseForestEventLookupView {
 public:
  using Function = const ExactDirectSupportEvent* (*)(
      const void*, std::size_t) noexcept;

  constexpr ExactDirectMorseForestEventLookupView() noexcept = default;
  constexpr ExactDirectMorseForestEventLookupView(
      const void* state, Function function) noexcept
      : state_(state), function_(function) {}

  [[nodiscard]] const ExactDirectSupportEvent* find(
      std::size_t source_event_index) const noexcept {
    return function_ == nullptr
               ? nullptr
               : function_(state_, source_event_index);
  }
  [[nodiscard]] explicit operator bool() const noexcept {
    return state_ != nullptr && function_ != nullptr;
  }

 private:
  const void* state_{};
  Function function_{};
};

// One borrowed batch.  All record indices remain global.  The singleton
// batch has implicit_singleton_role_count=point_count and an empty physical
// role span.  No span or lookup may be retained after the consumer returns.
struct ExactDirectMorseForestSourceBatchWindow {
  std::uint32_t schema_version{
      direct_morse_forest_source_authority_schema_version};
  contract::CanonicalId manifest_digest{};
  contract::CanonicalId source_identity_digest{};
  contract::CanonicalId source_chain_digest{};
  contract::CanonicalId batch_identity_digest{};
  contract::CanonicalId successor_chain_digest{};
  std::size_t source_batch_index{};
  std::size_t logical_role_begin_index{};
  std::size_t implicit_singleton_role_count{};
  std::size_t family_begin_index{};
  std::size_t arm_seed_begin_index{};
  const ExactDirectMorseH0Batch* batch{};
  std::span<const ExactDirectMorseH0RoleRecord> roles;
  std::span<const ExactDirectSaddleArmFamilyRecord> families;
  std::span<const ExactDirectSaddleArmSeedRecord> arm_seeds;
  ExactDirectMorseForestProjectionLookupView projections;
  ExactDirectMorseForestEventLookupView events;
  // This is an authority assertion, not a cache bit: the provider must set it
  // only after freshly reconstructing or recertifying this exact window from
  // the immutable source authorities.  The reducer requires it for every
  // committed batch before reporting the legacy freshly-replayed facts.
  bool scientific_source_freshly_recertified{false};
  bool resident_records_borrowed_without_copy{false};
  bool synchronous_lifetime_only{true};

  [[nodiscard]] bool certified_relative_to(
      const ExactDirectMorseForestSourceManifest& manifest) const;
};

using ExactDirectMorseForestSourceBatchConsumerFunction = bool (*)(
    void*, const ExactDirectMorseForestSourceBatchWindow&);

class ExactDirectMorseForestSourceBatchConsumerView {
 public:
  ExactDirectMorseForestSourceBatchConsumerView() = default;

  template <
      class Consumer,
      std::enable_if_t<
          !std::is_same_v<
              std::remove_cvref_t<Consumer>,
              ExactDirectMorseForestSourceBatchConsumerView> &&
              std::is_invocable_r_v<
                  bool,
                  Consumer&,
                  const ExactDirectMorseForestSourceBatchWindow&>,
          int> = 0>
  ExactDirectMorseForestSourceBatchConsumerView(Consumer& consumer) noexcept
      : state_(static_cast<void*>(std::addressof(consumer))),
        function_([](
                      void* state,
                      const ExactDirectMorseForestSourceBatchWindow& window) {
          return std::invoke(*static_cast<Consumer*>(state), window);
        }) {}

  [[nodiscard]] bool operator()(
      const ExactDirectMorseForestSourceBatchWindow& window) const {
    return function_ != nullptr && function_(state_, window);
  }
  [[nodiscard]] explicit operator bool() const noexcept {
    return state_ != nullptr && function_ != nullptr;
  }

 private:
  void* state_{};
  ExactDirectMorseForestSourceBatchConsumerFunction function_{};
};

enum class ExactDirectMorseForestSourceBatchVisitDecision : std::uint8_t {
  no_provider,
  no_batch_out_of_range,
  no_window_inconsistent,
  no_consumer_rejected,
  complete_synchronous_visit,
};

using ExactDirectMorseForestSourceBatchProviderFunction =
    ExactDirectMorseForestSourceBatchVisitDecision (*)(
        void*, std::size_t, ExactDirectMorseForestSourceBatchConsumerView);

// Generic non-owning provider view.  A massive implementation may load one
// batch into private scratch, invoke consumer, and release that scratch before
// returning.  The API deliberately cannot return a window or ownership.
class ExactDirectMorseForestSourceBatchProviderView {
 public:
  ExactDirectMorseForestSourceBatchProviderView() = default;

  template <
      class Provider,
      std::enable_if_t<
          !std::is_same_v<
              std::remove_cvref_t<Provider>,
              ExactDirectMorseForestSourceBatchProviderView> &&
              std::is_invocable_r_v<
                  ExactDirectMorseForestSourceBatchVisitDecision,
                  Provider&,
                  std::size_t,
                  ExactDirectMorseForestSourceBatchConsumerView>,
          int> = 0>
  ExactDirectMorseForestSourceBatchProviderView(Provider& provider) noexcept
      : state_(static_cast<void*>(std::addressof(provider))),
        function_([](
                      void* state,
                      std::size_t batch_index,
                      ExactDirectMorseForestSourceBatchConsumerView consumer) {
          return std::invoke(
              *static_cast<Provider*>(state), batch_index, consumer);
        }) {}

  [[nodiscard]] ExactDirectMorseForestSourceBatchVisitDecision operator()(
      std::size_t batch_index,
      ExactDirectMorseForestSourceBatchConsumerView consumer) const {
    return function_ == nullptr
               ? ExactDirectMorseForestSourceBatchVisitDecision::no_provider
               : function_(state_, batch_index, consumer);
  }
  [[nodiscard]] explicit operator bool() const noexcept {
    return state_ != nullptr && function_ != nullptr;
  }

 private:
  void* state_{};
  ExactDirectMorseForestSourceBatchProviderFunction function_{};
};

[[nodiscard]] ExactDirectMorseForestSourceManifest
build_exact_direct_morse_forest_source_manifest(
    const spatial::CanonicalPointCloud& cloud,
    const ExactDirectSupportTerminalFacade& source_facade,
    const ExactDirectMorseEventJournalResult& source_event_journal,
    const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
    const ExactDirectSaddleArmSeedJournalResult& source_seed_journal);

// Compatibility adapter for current resident authorities.  It owns only the
// scalar manifest and a chain-prefix digest per source batch; every scientific
// record remains borrowed in place without copying.
class ExactDirectMorseForestResidentSourceAdapter {
 public:
  ExactDirectMorseForestResidentSourceAdapter(
      const spatial::CanonicalPointCloud& cloud,
      const ExactDirectSupportTerminalFacade& source_facade,
      const ExactDirectMorseEventJournalResult& source_event_journal,
      const ExactDirectSaddleArmSeedBudget& trusted_seed_budget,
      const ExactDirectSaddleArmSeedJournalResult& source_seed_journal);

  [[nodiscard]] const ExactDirectMorseForestSourceManifest& manifest()
      const noexcept {
    return manifest_;
  }

  [[nodiscard]] ExactDirectMorseForestSourceBatchVisitDecision visit_batch(
      std::size_t batch_index,
      ExactDirectMorseForestSourceBatchConsumerView consumer) const;

  [[nodiscard]] ExactDirectMorseForestSourceBatchVisitDecision operator()(
      std::size_t batch_index,
      ExactDirectMorseForestSourceBatchConsumerView consumer) const {
    return visit_batch(batch_index, consumer);
  }

 private:
  const ExactDirectSupportTerminalFacade* source_facade_{};
  const ExactDirectMorseEventJournalResult* source_event_journal_{};
  const ExactDirectSaddleArmSeedJournalResult* source_seed_journal_{};
  ExactDirectMorseForestSourceManifest manifest_{};
  std::vector<contract::CanonicalId> chain_prefixes_;
};

}  // namespace morsehgp3d::hierarchy
