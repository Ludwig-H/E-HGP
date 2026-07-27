#pragma once

#include "morsehgp3d/hierarchy/direct_morse_forest_source_authority.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace morsehgp3d::hierarchy {

inline constexpr std::uint32_t
    direct_morse_forest_source_batch_wire_version = 1U;
inline constexpr std::string_view direct_morse_forest_source_wire_backend =
    "reference_cpu";
inline constexpr std::string_view direct_morse_forest_source_wire_profile =
    "hgp_reduced";
inline constexpr std::string_view direct_morse_forest_source_wire_mode =
    "canonical_bounded_source_batch_wire";
inline constexpr std::string_view
    direct_morse_forest_source_wire_public_status = "not_claimed";
inline constexpr std::size_t
    direct_morse_forest_source_batch_wire_header_byte_count = 64U;

// Finite per-batch limits.  They bound the canonical comparison before any
// wire-derived scientific record can be exposed.  The wire is never decoded
// into an authority: a provider compares it byte-for-byte with a freshly
// recertified source window and lends only that fresh window to the reducer.
struct ExactDirectMorseForestSourceWireLimits {
  std::size_t maximum_encoded_byte_count{64U * 1024U * 1024U};
  std::size_t maximum_role_record_count{1'000'000U};
  std::size_t maximum_family_record_count{1'000'000U};
  std::size_t maximum_arm_seed_record_count{4'000'000U};
  std::size_t maximum_projection_record_count{1'000'000U};
  std::size_t maximum_event_record_count{1'000'000U};
  std::size_t maximum_point_id_reference_count{16'000'000U};
  std::size_t maximum_exact_text_byte_count{2048U};
  std::size_t maximum_total_exact_text_byte_count{64U * 1024U * 1024U};

  [[nodiscard]] bool finite() const noexcept;

  friend bool operator==(
      const ExactDirectMorseForestSourceWireLimits&,
      const ExactDirectMorseForestSourceWireLimits&) = default;
};

enum class ExactDirectMorseForestSourceWireDecision : std::uint8_t {
  accepted,
  invalid_limits,
  manifest_not_certified,
  source_window_not_certified,
  source_window_inconsistent,
  numeric_overflow,
  encoded_byte_limit_exceeded,
  role_record_limit_exceeded,
  family_record_limit_exceeded,
  arm_seed_record_limit_exceeded,
  projection_record_limit_exceeded,
  event_record_limit_exceeded,
  point_id_reference_limit_exceeded,
  exact_text_limit_exceeded,
  total_exact_text_limit_exceeded,
  wire_size_mismatch,
  wire_content_mismatch,
};

struct ExactDirectMorseForestSourceBatchWireReceipt {
  std::size_t encoded_byte_count{};
  contract::CanonicalId payload_digest{};

  friend bool operator==(
      const ExactDirectMorseForestSourceBatchWireReceipt&,
      const ExactDirectMorseForestSourceBatchWireReceipt&) = default;
};

struct ExactDirectMorseForestEncodedSourceBatchWire {
  std::vector<std::uint8_t> bytes;
  ExactDirectMorseForestSourceBatchWireReceipt receipt{};
};

struct ExactDirectMorseForestSourceWireVerificationResult {
  ExactDirectMorseForestSourceWireDecision decision{
      ExactDirectMorseForestSourceWireDecision::source_window_inconsistent};
  std::optional<ExactDirectMorseForestSourceBatchWireReceipt> receipt;

  [[nodiscard]] bool accepted() const noexcept {
    return decision == ExactDirectMorseForestSourceWireDecision::accepted &&
           receipt.has_value();
  }
};

// No native layout, pointer, span or callback is copied.  Exact values use
// their canonical rational text under individual and cumulative byte caps.
// Invalid authorities throw std::invalid_argument, cap failures throw
// std::length_error and arithmetic overflow throws std::overflow_error.
[[nodiscard]] ExactDirectMorseForestEncodedSourceBatchWire
encode_exact_direct_morse_forest_source_batch_wire(
    const ExactDirectMorseForestSourceManifest& manifest,
    const ExactDirectMorseForestSourceBatchWindow& window,
    const ExactDirectMorseForestSourceWireLimits& limits);

// Compares an untrusted wire image to the canonical encoding of a fresh
// window without allocating an expected wire image.  Only accepted equality
// can authorize the caller to lend the already-fresh window.
[[nodiscard]] ExactDirectMorseForestSourceWireVerificationResult
verify_exact_direct_morse_forest_source_batch_wire(
    const ExactDirectMorseForestSourceManifest& manifest,
    const ExactDirectMorseForestSourceBatchWindow& fresh_window,
    std::span<const std::uint8_t> observed_wire,
    const ExactDirectMorseForestSourceWireLimits& limits) noexcept;

using ExactDirectMorseForestSourceBatchWireConsumerFunction = bool (*)(
    void*, std::span<const std::uint8_t>);

class ExactDirectMorseForestSourceBatchWireConsumerView {
 public:
  ExactDirectMorseForestSourceBatchWireConsumerView() = default;

  template <
      class Consumer,
      std::enable_if_t<
          !std::is_same_v<
              std::remove_cvref_t<Consumer>,
              ExactDirectMorseForestSourceBatchWireConsumerView> &&
              !std::is_pointer_v<std::remove_cvref_t<Consumer>> &&
              !std::is_function_v<std::remove_reference_t<Consumer>> &&
              !std::is_const_v<std::remove_reference_t<Consumer>> &&
              std::is_invocable_r_v<
                  bool, Consumer&, std::span<const std::uint8_t>>,
          int> = 0>
  ExactDirectMorseForestSourceBatchWireConsumerView(
      Consumer& consumer) noexcept
      : state_(static_cast<void*>(std::addressof(consumer))),
        function_([](void* state, std::span<const std::uint8_t> bytes) -> bool {
          return static_cast<bool>(
              std::invoke(*static_cast<Consumer*>(state), bytes));
        }) {}

  [[nodiscard]] bool operator()(
      std::span<const std::uint8_t> bytes) const {
    return function_ != nullptr && function_(state_, bytes);
  }
  [[nodiscard]] explicit operator bool() const noexcept {
    return state_ != nullptr && function_ != nullptr;
  }

 private:
  void* state_{};
  ExactDirectMorseForestSourceBatchWireConsumerFunction function_{};
};

enum class ExactDirectMorseForestSourceBatchWireVisitDecision
    : std::uint8_t {
  no_provider,
  no_batch_out_of_range,
  no_wire_unavailable,
  no_consumer_rejected,
  complete_synchronous_visit,
};

using ExactDirectMorseForestSourceBatchWireProviderFunction =
    ExactDirectMorseForestSourceBatchWireVisitDecision (*)(
        void*,
        std::size_t,
        ExactDirectMorseForestSourceBatchWireConsumerView);

// A loader owns its I/O buffer and lends at most one complete image for the
// duration of the callback.  Filesystem framing and crash recovery belong to
// the next durable increment, not to this process-local seam.
class ExactDirectMorseForestSourceBatchWireProviderView {
 public:
  ExactDirectMorseForestSourceBatchWireProviderView() = default;

  template <
      class Provider,
      std::enable_if_t<
          !std::is_same_v<
              std::remove_cvref_t<Provider>,
              ExactDirectMorseForestSourceBatchWireProviderView> &&
              !std::is_pointer_v<std::remove_cvref_t<Provider>> &&
              !std::is_function_v<std::remove_reference_t<Provider>> &&
              !std::is_const_v<std::remove_reference_t<Provider>> &&
              std::is_invocable_r_v<
                  ExactDirectMorseForestSourceBatchWireVisitDecision,
                  Provider&,
                  std::size_t,
                  ExactDirectMorseForestSourceBatchWireConsumerView>,
          int> = 0>
  ExactDirectMorseForestSourceBatchWireProviderView(
      Provider& provider) noexcept
      : state_(static_cast<void*>(std::addressof(provider))),
        function_([](
                      void* state,
                      std::size_t batch_index,
                      ExactDirectMorseForestSourceBatchWireConsumerView
                          consumer)
                      -> ExactDirectMorseForestSourceBatchWireVisitDecision {
          return static_cast<
              ExactDirectMorseForestSourceBatchWireVisitDecision>(
              std::invoke(
                  *static_cast<Provider*>(state), batch_index, consumer));
        }) {}

  [[nodiscard]] ExactDirectMorseForestSourceBatchWireVisitDecision
  operator()(
      std::size_t batch_index,
      ExactDirectMorseForestSourceBatchWireConsumerView consumer) const {
    return function_ == nullptr
               ? ExactDirectMorseForestSourceBatchWireVisitDecision::
                     no_provider
               : function_(state_, batch_index, consumer);
  }
  [[nodiscard]] explicit operator bool() const noexcept {
    return state_ != nullptr && function_ != nullptr;
  }

 private:
  void* state_{};
  ExactDirectMorseForestSourceBatchWireProviderFunction function_{};
};

// Concrete reducer-facing composition.  It owns only the scalar manifest,
// limits and two non-owning provider views.  For each call it borrows one
// untrusted wire, asks the source provider for the same freshly recertified
// window, compares canonically, invokes the consumer with the fresh window,
// then releases both callbacks before returning.  Reentrant visits fail
// closed.  Both provider contexts must outlive this object.
class ExactDirectMorseForestCanonicalWireSourceProvider {
 public:
  ExactDirectMorseForestCanonicalWireSourceProvider(
      ExactDirectMorseForestSourceManifest manifest,
      ExactDirectMorseForestSourceBatchProviderView fresh_source_provider,
      ExactDirectMorseForestSourceBatchWireProviderView wire_provider,
      ExactDirectMorseForestSourceWireLimits limits);

  [[nodiscard]] const ExactDirectMorseForestSourceManifest& manifest()
      const noexcept {
    return manifest_;
  }
  [[nodiscard]] ExactDirectMorseForestSourceWireDecision
  last_wire_decision() const noexcept {
    return last_wire_decision_;
  }
  [[nodiscard]] std::size_t verified_visit_count() const noexcept {
    return verified_visit_count_;
  }

  [[nodiscard]] ExactDirectMorseForestSourceBatchVisitDecision visit_batch(
      std::size_t batch_index,
      ExactDirectMorseForestSourceBatchConsumerView consumer);

  [[nodiscard]] ExactDirectMorseForestSourceBatchVisitDecision operator()(
      std::size_t batch_index,
      ExactDirectMorseForestSourceBatchConsumerView consumer) {
    return visit_batch(batch_index, consumer);
  }

 private:
  ExactDirectMorseForestSourceManifest manifest_{};
  ExactDirectMorseForestSourceBatchProviderView fresh_source_provider_{};
  ExactDirectMorseForestSourceBatchWireProviderView wire_provider_{};
  ExactDirectMorseForestSourceWireLimits limits_{};
  ExactDirectMorseForestSourceWireDecision last_wire_decision_{
      ExactDirectMorseForestSourceWireDecision::source_window_inconsistent};
  std::size_t verified_visit_count_{};
  bool visit_in_progress_{false};
};

}  // namespace morsehgp3d::hierarchy
