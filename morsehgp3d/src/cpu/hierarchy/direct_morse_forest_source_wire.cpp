#include "morsehgp3d/hierarchy/direct_morse_forest_source_wire.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::array<std::uint8_t, 8U> wire_magic{
    'M', '3', 'D', '1', '5', 'K', 'B', '1'};
constexpr std::uint32_t wire_kind = 1U;

class WireFailure final : public std::exception {
 public:
  explicit WireFailure(
      ExactDirectMorseForestSourceWireDecision decision) noexcept
      : decision_(decision) {}

  [[nodiscard]] ExactDirectMorseForestSourceWireDecision decision()
      const noexcept {
    return decision_;
  }
  [[nodiscard]] const char* what() const noexcept override {
    return "direct Morse forest source wire failure";
  }

 private:
  ExactDirectMorseForestSourceWireDecision decision_;
};

[[noreturn]] void fail(
    ExactDirectMorseForestSourceWireDecision decision) {
  throw WireFailure{decision};
}

[[nodiscard]] std::size_t checked_add(
    std::size_t left, std::size_t right) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    fail(ExactDirectMorseForestSourceWireDecision::numeric_overflow);
  }
  return left + right;
}

[[nodiscard]] std::uint64_t checked_u64(std::size_t value) {
  if constexpr (
      std::numeric_limits<std::size_t>::max() >
      std::numeric_limits<std::uint64_t>::max()) {
    if (value > std::numeric_limits<std::uint64_t>::max()) {
      fail(ExactDirectMorseForestSourceWireDecision::numeric_overflow);
    }
  }
  return static_cast<std::uint64_t>(value);
}

void store_u32(
    std::span<std::uint8_t> bytes,
    std::size_t offset,
    std::uint32_t value) noexcept {
  for (std::size_t index = 0U; index < 4U; ++index) {
    bytes[offset + index] = static_cast<std::uint8_t>(
        value >> ((3U - index) * 8U));
  }
}

void store_u64(
    std::span<std::uint8_t> bytes,
    std::size_t offset,
    std::uint64_t value) noexcept {
  for (std::size_t index = 0U; index < 8U; ++index) {
    bytes[offset + index] = static_cast<std::uint8_t>(
        value >> ((7U - index) * 8U));
  }
}

[[nodiscard]] std::uint32_t load_u32(
    std::span<const std::uint8_t> bytes,
    std::size_t offset) noexcept {
  std::uint32_t value = 0U;
  for (std::size_t index = 0U; index < 4U; ++index) {
    value = static_cast<std::uint32_t>(
        (value << 8U) | bytes[offset + index]);
  }
  return value;
}

[[nodiscard]] std::uint64_t load_u64(
    std::span<const std::uint8_t> bytes,
    std::size_t offset) noexcept {
  std::uint64_t value = 0U;
  for (std::size_t index = 0U; index < 8U; ++index) {
    value = (value << 8U) | bytes[offset + index];
  }
  return value;
}

[[nodiscard]] ExactDirectMorseForestSourceWireDecision
preflight_observed_wire(
    std::span<const std::uint8_t> observed_wire,
    const ExactDirectMorseForestSourceWireLimits& limits) noexcept {
  if (observed_wire.size() > limits.maximum_encoded_byte_count) {
    return ExactDirectMorseForestSourceWireDecision::
        encoded_byte_limit_exceeded;
  }
  if (observed_wire.size() <
      direct_morse_forest_source_batch_wire_header_byte_count) {
    return ExactDirectMorseForestSourceWireDecision::wire_size_mismatch;
  }
  if (!std::equal(
          wire_magic.begin(), wire_magic.end(), observed_wire.begin()) ||
      load_u32(observed_wire, 8U) !=
          direct_morse_forest_source_batch_wire_version ||
      load_u32(observed_wire, 12U) != wire_kind ||
      load_u32(observed_wire, 16U) != 0U ||
      load_u32(observed_wire, 20U) != 0U) {
    return ExactDirectMorseForestSourceWireDecision::wire_content_mismatch;
  }
  const std::uint64_t declared_payload_byte_count =
      load_u64(observed_wire, 24U);
  if constexpr (
      std::numeric_limits<std::size_t>::max() <
      std::numeric_limits<std::uint64_t>::max()) {
    if (declared_payload_byte_count >
        std::numeric_limits<std::size_t>::max()) {
      return ExactDirectMorseForestSourceWireDecision::numeric_overflow;
    }
  }
  const std::size_t payload_byte_count =
      static_cast<std::size_t>(declared_payload_byte_count);
  if (payload_byte_count >
      std::numeric_limits<std::size_t>::max() -
          direct_morse_forest_source_batch_wire_header_byte_count) {
    return ExactDirectMorseForestSourceWireDecision::numeric_overflow;
  }
  const std::size_t declared_wire_byte_count =
      direct_morse_forest_source_batch_wire_header_byte_count +
      payload_byte_count;
  return declared_wire_byte_count == observed_wire.size()
             ? ExactDirectMorseForestSourceWireDecision::accepted
             : ExactDirectMorseForestSourceWireDecision::wire_size_mismatch;
}

[[nodiscard]] std::array<
    std::uint8_t,
    direct_morse_forest_source_batch_wire_header_byte_count>
make_header(
    std::size_t payload_byte_count,
    const contract::CanonicalId& payload_digest) {
  std::array<
      std::uint8_t,
      direct_morse_forest_source_batch_wire_header_byte_count>
      header{};
  std::copy(wire_magic.begin(), wire_magic.end(), header.begin());
  store_u32(header, 8U, direct_morse_forest_source_batch_wire_version);
  store_u32(header, 12U, wire_kind);
  store_u32(header, 16U, 0U);
  store_u32(header, 20U, 0U);
  store_u64(header, 24U, checked_u64(payload_byte_count));
  std::copy(
      payload_digest.bytes().begin(),
      payload_digest.bytes().end(),
      header.begin() + 32);
  return header;
}

class MeasuringWriter {
 public:
  explicit MeasuringWriter(std::size_t maximum_byte_count) noexcept
      : maximum_byte_count_(maximum_byte_count) {}

  void append(std::span<const std::uint8_t> bytes) {
    if (byte_count_ > maximum_byte_count_ ||
        bytes.size() > maximum_byte_count_ - byte_count_) {
      fail(ExactDirectMorseForestSourceWireDecision::
               encoded_byte_limit_exceeded);
    }
    byte_count_ = checked_add(byte_count_, bytes.size());
    digest_.update(bytes);
  }

  [[nodiscard]] std::size_t byte_count() const noexcept {
    return byte_count_;
  }
  [[nodiscard]] contract::CanonicalId finalize() {
    return digest_.finalize();
  }

 private:
  std::size_t maximum_byte_count_{};
  std::size_t byte_count_{};
  contract::CanonicalSha256Builder digest_;
};

class BufferWriter {
 public:
  explicit BufferWriter(std::span<std::uint8_t> output) noexcept
      : output_(output) {}

  void append(std::span<const std::uint8_t> bytes) {
    if (bytes.size() > output_.size() - offset_) {
      fail(ExactDirectMorseForestSourceWireDecision::numeric_overflow);
    }
    digest_.update(bytes);
    std::copy(
        bytes.begin(),
        bytes.end(),
        output_.begin() + static_cast<std::ptrdiff_t>(offset_));
    offset_ += bytes.size();
  }

  [[nodiscard]] std::size_t offset() const noexcept { return offset_; }
  [[nodiscard]] contract::CanonicalId finalize() {
    return digest_.finalize();
  }

 private:
  std::span<std::uint8_t> output_;
  std::size_t offset_{};
  contract::CanonicalSha256Builder digest_;
};

class ComparisonWriter {
 public:
  explicit ComparisonWriter(std::span<const std::uint8_t> observed) noexcept
      : observed_(observed) {}

  void append(std::span<const std::uint8_t> bytes) {
    digest_.update(bytes);
    if (bytes.size() > observed_.size() - offset_) {
      equal_ = false;
      offset_ = observed_.size();
      return;
    }
    equal_ = equal_ && std::equal(
        bytes.begin(),
        bytes.end(),
        observed_.begin() + static_cast<std::ptrdiff_t>(offset_));
    offset_ += bytes.size();
  }

  [[nodiscard]] bool equal_and_complete() const noexcept {
    return equal_ && offset_ == observed_.size();
  }
  [[nodiscard]] contract::CanonicalId finalize() {
    return digest_.finalize();
  }

 private:
  std::span<const std::uint8_t> observed_;
  std::size_t offset_{};
  bool equal_{true};
  contract::CanonicalSha256Builder digest_;
};

template <class Writer>
void write_u8(Writer& writer, std::uint8_t value) {
  const std::array<std::uint8_t, 1U> bytes{value};
  writer.append(bytes);
}

template <class Writer>
void write_u32(Writer& writer, std::uint32_t value) {
  std::array<std::uint8_t, 4U> bytes{};
  store_u32(bytes, 0U, value);
  writer.append(bytes);
}

template <class Writer>
void write_u64(Writer& writer, std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  store_u64(bytes, 0U, value);
  writer.append(bytes);
}

template <class Writer>
void write_size(Writer& writer, std::size_t value) {
  write_u64(writer, checked_u64(value));
}

template <class Writer>
void write_id(
    Writer& writer, const contract::CanonicalId& identifier) {
  writer.append(identifier.bytes());
}

struct EncodingAudit {
  const ExactDirectMorseForestSourceWireLimits* limits{};
  std::size_t point_id_reference_count{};
  std::size_t total_exact_text_byte_count{};

  void add_point_id_references(std::size_t count) {
    point_id_reference_count =
        checked_add(point_id_reference_count, count);
    if (point_id_reference_count >
        limits->maximum_point_id_reference_count) {
      fail(ExactDirectMorseForestSourceWireDecision::
               point_id_reference_limit_exceeded);
    }
  }

  void add_exact_text(std::size_t count) {
    if (count > limits->maximum_exact_text_byte_count) {
      fail(ExactDirectMorseForestSourceWireDecision::
               exact_text_limit_exceeded);
    }
    total_exact_text_byte_count =
        checked_add(total_exact_text_byte_count, count);
    if (total_exact_text_byte_count >
        limits->maximum_total_exact_text_byte_count) {
      fail(ExactDirectMorseForestSourceWireDecision::
               total_exact_text_limit_exceeded);
    }
  }
};

[[nodiscard]] bool add_within_limit(
    std::size_t value,
    std::size_t limit,
    std::size_t& total) noexcept {
  if (total > limit || value > limit - total) {
    return false;
  }
  total += value;
  return true;
}

[[nodiscard]] bool add_decimal_digit_upper_bound(
    const exact::BigInt& value,
    std::size_t limit,
    std::size_t& total) {
  if (value == 0) {
    return add_within_limit(1U, limit, total);
  }
  const auto& backend = value.backend();
  using Limb = std::remove_cv_t<
      std::remove_pointer_t<decltype(backend.limbs())>>;
  constexpr std::size_t limb_bit_count =
      std::numeric_limits<Limb>::digits;
  const std::size_t limb_count = backend.size();
  if (limb_count == 0U || limb_count - 1U >
      std::numeric_limits<std::size_t>::max() / limb_bit_count) {
    return false;
  }
  // cpp_int is sign-magnitude.  Counting complete limbs is conservative and
  // avoids copying a hostile magnitude merely to inspect its highest bit.
  const std::size_t complete_limb_bit_count =
      (limb_count - 1U) * limb_bit_count;
  const std::size_t leading_bit_count = static_cast<std::size_t>(
      std::bit_width(backend.limbs()[limb_count - 1U]));
  if (leading_bit_count == 0U ||
      complete_limb_bit_count >
          std::numeric_limits<std::size_t>::max() - leading_bit_count) {
    return false;
  }
  const std::size_t bit_count =
      complete_limb_bit_count + leading_bit_count;

  // 30103 / 100000 is a strict upper approximation of log10(2).
  constexpr std::size_t numerator = 30103U;
  constexpr std::size_t denominator = 100000U;
  const std::size_t quotient = bit_count / denominator;
  const std::size_t remainder = bit_count % denominator;
  if (quotient > std::numeric_limits<std::size_t>::max() / numerator) {
    return false;
  }
  const std::size_t whole = quotient * numerator;
  const std::size_t remainder_product = remainder * numerator;
  const std::size_t fractional =
      remainder_product / denominator +
      (remainder_product % denominator == 0U ? 0U : 1U);
  if (whole > std::numeric_limits<std::size_t>::max() - fractional) {
    return false;
  }
  return add_within_limit(whole + fractional, limit, total);
}

[[nodiscard]] bool canonical_rational_allocation_is_bounded(
    const exact::ExactRational& value,
    std::size_t exact_text_byte_limit) {
  const std::size_t tolerated_limit =
      exact_text_byte_limit > std::numeric_limits<std::size_t>::max() - 2U
          ? exact_text_byte_limit
          : exact_text_byte_limit + 2U;
  std::size_t upper_byte_count = 0U;
  if (value.numerator() < 0 &&
      !add_within_limit(1U, tolerated_limit, upper_byte_count)) {
    return false;
  }
  return add_decimal_digit_upper_bound(
             value.numerator(), tolerated_limit, upper_byte_count) &&
         add_within_limit(1U, tolerated_limit, upper_byte_count) &&
         add_decimal_digit_upper_bound(
             value.denominator(), tolerated_limit, upper_byte_count);
}

struct PreflightAudit {
  const ExactDirectMorseForestSourceWireLimits* limits{};
  std::size_t point_id_reference_count{};
  std::size_t total_exact_text_upper_bound{};

  void add_point_id_references(std::size_t count) {
    point_id_reference_count = checked_add(point_id_reference_count, count);
    if (point_id_reference_count >
        limits->maximum_point_id_reference_count) {
      fail(ExactDirectMorseForestSourceWireDecision::
               point_id_reference_limit_exceeded);
    }
  }

  void add_exact(const exact::ExactRational& value) {
    if (!canonical_rational_allocation_is_bounded(
            value, limits->maximum_exact_text_byte_count)) {
      fail(ExactDirectMorseForestSourceWireDecision::
               exact_text_limit_exceeded);
    }
    const std::string text = value.canonical_key();
    if (text.size() > limits->maximum_exact_text_byte_count) {
      fail(ExactDirectMorseForestSourceWireDecision::
               exact_text_limit_exceeded);
    }
    if (total_exact_text_upper_bound >
            limits->maximum_total_exact_text_byte_count ||
        text.size() > limits->maximum_total_exact_text_byte_count -
                          total_exact_text_upper_bound) {
      fail(ExactDirectMorseForestSourceWireDecision::
               total_exact_text_limit_exceeded);
    }
    total_exact_text_upper_bound += text.size();
  }

  void check_exact_allocation(const exact::ExactRational& value) const {
    if (!canonical_rational_allocation_is_bounded(
            value, limits->maximum_exact_text_byte_count)) {
      fail(ExactDirectMorseForestSourceWireDecision::
               exact_text_limit_exceeded);
    }
    if (value.canonical_key().size() >
        limits->maximum_exact_text_byte_count) {
      fail(ExactDirectMorseForestSourceWireDecision::
               exact_text_limit_exceeded);
    }
  }
};

void preflight_window_bounds(
    const ExactDirectMorseForestSourceBatchWindow& window,
    const ExactDirectMorseForestSourceWireLimits& limits) {
  if (window.batch == nullptr || !window.projections || !window.events) {
    fail(ExactDirectMorseForestSourceWireDecision::
             source_window_not_certified);
  }
  PreflightAudit audit{&limits};
  audit.add_exact(window.batch->squared_level.rational());
  for (const auto& role : window.roles) {
    const auto* projection =
        window.projections.find(role.event_projection_index);
    if (projection == nullptr) {
      fail(ExactDirectMorseForestSourceWireDecision::
               source_window_inconsistent);
    }
    const auto* event = window.events.find(projection->source_index);
    if (event == nullptr) {
      fail(ExactDirectMorseForestSourceWireDecision::
               source_window_inconsistent);
    }
    if (projection->support_size == 0U || projection->support_size > 4U ||
        event->support_size != projection->support_size) {
      fail(ExactDirectMorseForestSourceWireDecision::
               source_window_inconsistent);
    }
    audit.add_point_id_references(projection->support_size);
    audit.add_point_id_references(event->support_size);
    audit.add_point_id_references(event->interior_ids.size());
    audit.add_exact(projection->squared_level.rational());
    for (std::size_t axis = 0U; axis < 3U; ++axis) {
      audit.add_exact(event->center.coordinate(axis));
    }
    audit.add_exact(event->squared_level.rational());
  }
  for (const auto& family : window.families) {
    audit.add_exact(family.critical_squared_level.rational());
    if (family.journal_role_record_index <
            window.logical_role_begin_index ||
        family.journal_role_record_index -
                window.logical_role_begin_index >=
            window.roles.size()) {
      fail(ExactDirectMorseForestSourceWireDecision::
               source_window_inconsistent);
    }
    const auto& role = window.roles[
        family.journal_role_record_index -
        window.logical_role_begin_index];
    const auto* projection =
        window.projections.find(role.event_projection_index);
    if (role.role != ExactDirectMorseH0Role::saddle ||
        role.event_projection_index !=
            family.journal_event_projection_index ||
        projection == nullptr ||
        projection->source_index != family.source_event_index) {
      fail(ExactDirectMorseForestSourceWireDecision::
               source_window_inconsistent);
    }
    const auto* event = window.events.find(projection->source_index);
    if (event == nullptr) {
      fail(ExactDirectMorseForestSourceWireDecision::
               source_window_inconsistent);
    }
    // certified_relative_to hashes this event again for a family.  It is not
    // serialized twice, so protect the allocation without charging wire text.
    audit.check_exact_allocation(event->squared_level.rational());
  }
  audit.add_point_id_references(window.arm_seeds.size());
}

template <class Writer>
void write_exact_text(
    Writer& writer, std::string_view text, EncodingAudit& audit) {
  audit.add_exact_text(text.size());
  write_size(writer, text.size());
  writer.append(std::span<const std::uint8_t>{
      reinterpret_cast<const std::uint8_t*>(text.data()), text.size()});
}

template <class Writer>
void write_rational(
    Writer& writer,
    const exact::ExactRational& value,
    EncodingAudit& audit) {
  const std::size_t remaining_total =
      audit.limits->maximum_total_exact_text_byte_count -
      audit.total_exact_text_byte_count;
  const std::size_t effective_limit = std::min(
      audit.limits->maximum_exact_text_byte_count, remaining_total);
  if (!canonical_rational_allocation_is_bounded(value, effective_limit)) {
    fail(remaining_total < audit.limits->maximum_exact_text_byte_count
             ? ExactDirectMorseForestSourceWireDecision::
                   total_exact_text_limit_exceeded
             : ExactDirectMorseForestSourceWireDecision::
                   exact_text_limit_exceeded);
  }
  const std::string text = value.canonical_key();
  write_exact_text(writer, text, audit);
}

template <class Writer>
void write_level(
    Writer& writer,
    const exact::ExactLevel& level,
    EncodingAudit& audit) {
  write_rational(writer, level.rational(), audit);
}

template <class Writer>
void write_optional_size(
    Writer& writer, const std::optional<std::size_t>& value) {
  write_u8(writer, value.has_value() ? std::uint8_t{1U} : std::uint8_t{0U});
  if (value.has_value()) {
    write_size(writer, *value);
  }
}

[[nodiscard]] bool valid_projection_source(
    ExactDirectMorseEventSource source) noexcept {
  return source == ExactDirectMorseEventSource::canonical_singleton ||
         source ==
             ExactDirectMorseEventSource::direct_support_terminal_event;
}

[[nodiscard]] bool valid_role(ExactDirectMorseH0Role role) noexcept {
  return role == ExactDirectMorseH0Role::birth ||
         role == ExactDirectMorseH0Role::saddle;
}

template <class Writer>
void write_projection_and_event(
    Writer& writer,
    const ExactDirectMorseEventProjection& projection,
    const ExactDirectSupportEvent& event,
    EncodingAudit& audit) {
  if (!valid_projection_source(projection.source) ||
      projection.source !=
          ExactDirectMorseEventSource::direct_support_terminal_event ||
      projection.source_index != event.event_index ||
      projection.support_size == 0U || projection.support_size > 4U ||
      event.support_size != projection.support_size ||
      !std::equal(
          projection.support_ids.begin(),
          projection.support_ids.begin() +
              static_cast<std::ptrdiff_t>(projection.support_size),
          event.support_ids.begin()) ||
      event.squared_level != projection.squared_level ||
      event.closed_rank != projection.closed_rank ||
      event.birth_order != projection.birth_order ||
      event.saddle_order != projection.saddle_order) {
    fail(ExactDirectMorseForestSourceWireDecision::
             source_window_inconsistent);
  }

  write_size(writer, projection.event_projection_index);
  write_u8(writer, static_cast<std::uint8_t>(projection.source));
  write_size(writer, projection.source_index);
  write_u8(writer, projection.support_size);
  audit.add_point_id_references(projection.support_size);
  for (std::size_t index = 0U;
       index < static_cast<std::size_t>(projection.support_size);
       ++index) {
    write_u64(
        writer,
        static_cast<std::uint64_t>(projection.support_ids[index]));
  }
  write_level(writer, projection.squared_level, audit);
  write_size(writer, projection.closed_rank);
  write_optional_size(writer, projection.birth_order);
  write_optional_size(writer, projection.saddle_order);

  write_size(writer, event.event_index);
  write_u8(writer, event.support_size);
  audit.add_point_id_references(event.support_size);
  for (std::size_t index = 0U;
       index < static_cast<std::size_t>(event.support_size);
       ++index) {
    write_u64(
        writer,
        static_cast<std::uint64_t>(event.support_ids[index]));
  }
  for (std::size_t axis = 0U; axis < 3U; ++axis) {
    write_rational(writer, event.center.coordinate(axis), audit);
  }
  write_level(writer, event.squared_level, audit);
  write_size(writer, event.interior_ids.size());
  audit.add_point_id_references(event.interior_ids.size());
  for (const spatial::PointId point_id : event.interior_ids) {
    write_u64(writer, static_cast<std::uint64_t>(point_id));
  }
  write_size(writer, event.closed_rank);
  write_size(writer, event.exterior_count);
  write_optional_size(writer, event.birth_order);
  write_optional_size(writer, event.saddle_order);
}

template <class Writer>
void emit_payload(
    Writer& writer,
    const ExactDirectMorseForestSourceManifest& manifest,
    const ExactDirectMorseForestSourceBatchWindow& window,
    const ExactDirectMorseForestSourceWireLimits& limits) {
  if (!limits.finite()) {
    fail(ExactDirectMorseForestSourceWireDecision::invalid_limits);
  }
  if (!manifest.certified()) {
    fail(ExactDirectMorseForestSourceWireDecision::
             manifest_not_certified);
  }
  if (window.roles.size() > limits.maximum_role_record_count) {
    fail(ExactDirectMorseForestSourceWireDecision::
             role_record_limit_exceeded);
  }
  if (window.families.size() > limits.maximum_family_record_count) {
    fail(ExactDirectMorseForestSourceWireDecision::
             family_record_limit_exceeded);
  }
  if (window.arm_seeds.size() > limits.maximum_arm_seed_record_count) {
    fail(ExactDirectMorseForestSourceWireDecision::
             arm_seed_record_limit_exceeded);
  }
  if (window.roles.size() > limits.maximum_projection_record_count) {
    fail(ExactDirectMorseForestSourceWireDecision::
             projection_record_limit_exceeded);
  }
  if (window.roles.size() > limits.maximum_event_record_count) {
    fail(ExactDirectMorseForestSourceWireDecision::
             event_record_limit_exceeded);
  }
  preflight_window_bounds(window, limits);
  bool window_certified = false;
  try {
    window_certified = window.certified_relative_to(manifest);
  } catch (...) {
    fail(ExactDirectMorseForestSourceWireDecision::
             source_window_not_certified);
  }
  if (!window_certified) {
    fail(ExactDirectMorseForestSourceWireDecision::
             source_window_not_certified);
  }

  EncodingAudit audit{&limits};
  write_u32(writer, window.schema_version);
  write_id(writer, window.manifest_digest);
  write_id(writer, window.source_identity_digest);
  write_id(writer, window.source_chain_digest);
  write_id(writer, window.batch_identity_digest);
  write_id(writer, window.successor_chain_digest);
  write_size(writer, window.source_batch_index);
  write_size(writer, window.logical_role_begin_index);
  write_size(writer, window.implicit_singleton_role_count);
  write_size(writer, window.family_begin_index);
  write_size(writer, window.arm_seed_begin_index);

  const ExactDirectMorseH0Batch& batch = *window.batch;
  write_size(writer, batch.batch_index);
  write_size(writer, batch.order);
  write_level(writer, batch.squared_level, audit);
  write_size(writer, batch.role_record_offset);
  write_size(writer, batch.role_record_count);
  write_size(writer, batch.birth_role_count);
  write_size(writer, batch.saddle_role_count);

  write_size(writer, window.roles.size());
  for (std::size_t local = 0U; local < window.roles.size(); ++local) {
    const auto& role = window.roles[local];
    if (!valid_role(role.role) ||
        role.role_record_index !=
            checked_add(window.logical_role_begin_index, local) ||
        role.batch_index != window.source_batch_index ||
        (local != 0U &&
         window.roles[local - 1U].event_projection_index >=
             role.event_projection_index)) {
      fail(ExactDirectMorseForestSourceWireDecision::
               source_window_inconsistent);
    }
    write_size(writer, role.role_record_index);
    write_size(writer, role.batch_index);
    write_size(writer, role.event_projection_index);
    write_u8(writer, static_cast<std::uint8_t>(role.role));
  }

  // One direct role occurs at most once in a fixed (order, level) batch, so
  // the canonical role order is also a unique projection/event table order.
  write_size(writer, window.roles.size());
  for (const auto& role : window.roles) {
    const auto* projection =
        window.projections.find(role.event_projection_index);
    if (projection == nullptr ||
        projection->event_projection_index !=
            role.event_projection_index) {
      fail(ExactDirectMorseForestSourceWireDecision::
               source_window_inconsistent);
    }
    const auto* event = window.events.find(projection->source_index);
    if (event == nullptr) {
      fail(ExactDirectMorseForestSourceWireDecision::
               source_window_inconsistent);
    }
    write_projection_and_event(writer, *projection, *event, audit);
  }

  write_size(writer, window.families.size());
  std::size_t expected_arm_offset = window.arm_seed_begin_index;
  for (std::size_t local = 0U; local < window.families.size(); ++local) {
    const auto& family = window.families[local];
    if (family.family_index !=
            checked_add(window.family_begin_index, local) ||
        family.journal_batch_index != window.source_batch_index ||
        family.order != batch.order ||
        family.critical_squared_level != batch.squared_level ||
        family.arm_seed_offset != expected_arm_offset ||
        family.journal_role_record_index < window.logical_role_begin_index ||
        family.journal_role_record_index - window.logical_role_begin_index >=
            window.roles.size()) {
      fail(ExactDirectMorseForestSourceWireDecision::
               source_window_inconsistent);
    }
    const std::size_t role_local =
        family.journal_role_record_index - window.logical_role_begin_index;
    const auto& role = window.roles[role_local];
    const auto* projection =
        window.projections.find(role.event_projection_index);
    if (role.role != ExactDirectMorseH0Role::saddle ||
        role.event_projection_index !=
            family.journal_event_projection_index ||
        projection == nullptr ||
        projection->source_index != family.source_event_index ||
        family.arm_seed_count >
            window.arm_seeds.size() -
                (expected_arm_offset - window.arm_seed_begin_index)) {
      fail(ExactDirectMorseForestSourceWireDecision::
               source_window_inconsistent);
    }

    write_size(writer, family.family_index);
    write_size(writer, family.source_event_index);
    write_size(writer, family.journal_event_projection_index);
    write_size(writer, family.journal_role_record_index);
    write_size(writer, family.journal_batch_index);
    write_size(writer, family.order);
    write_level(writer, family.critical_squared_level, audit);
    write_size(writer, family.arm_seed_offset);
    write_size(writer, family.arm_seed_count);
    write_id(writer, family.source_event_arm_identity_digest);

    for (std::size_t seed_local = 0U;
         seed_local < family.arm_seed_count;
         ++seed_local) {
      const std::size_t global_seed =
          checked_add(family.arm_seed_offset, seed_local);
      const auto& seed = window.arm_seeds[
          global_seed - window.arm_seed_begin_index];
      if (seed.arm_seed_index != global_seed ||
          seed.family_index != family.family_index) {
        fail(ExactDirectMorseForestSourceWireDecision::
                 source_window_inconsistent);
      }
    }
    expected_arm_offset = checked_add(
        expected_arm_offset, family.arm_seed_count);
  }
  if (expected_arm_offset - window.arm_seed_begin_index !=
      window.arm_seeds.size()) {
    fail(ExactDirectMorseForestSourceWireDecision::
             source_window_inconsistent);
  }

  write_size(writer, window.arm_seeds.size());
  audit.add_point_id_references(window.arm_seeds.size());
  for (std::size_t local = 0U; local < window.arm_seeds.size(); ++local) {
    const auto& seed = window.arm_seeds[local];
    if (seed.arm_seed_index !=
        checked_add(window.arm_seed_begin_index, local)) {
      fail(ExactDirectMorseForestSourceWireDecision::
               source_window_inconsistent);
    }
    write_size(writer, seed.arm_seed_index);
    write_size(writer, seed.family_index);
    write_u64(
        writer,
        static_cast<std::uint64_t>(seed.removed_support_point_id));
  }
}

struct PayloadMeasurement {
  std::size_t byte_count{};
  contract::CanonicalId digest{};
};

[[nodiscard]] PayloadMeasurement measure_payload(
    const ExactDirectMorseForestSourceManifest& manifest,
    const ExactDirectMorseForestSourceBatchWindow& window,
    const ExactDirectMorseForestSourceWireLimits& limits) {
  if (!limits.finite()) {
    fail(ExactDirectMorseForestSourceWireDecision::invalid_limits);
  }
  MeasuringWriter writer{
      limits.maximum_encoded_byte_count -
      direct_morse_forest_source_batch_wire_header_byte_count};
  emit_payload(writer, manifest, window, limits);
  const std::size_t payload_byte_count = writer.byte_count();
  const std::size_t encoded_byte_count = checked_add(
      direct_morse_forest_source_batch_wire_header_byte_count,
      payload_byte_count);
  if (encoded_byte_count > limits.maximum_encoded_byte_count) {
    fail(ExactDirectMorseForestSourceWireDecision::
             encoded_byte_limit_exceeded);
  }
  return {payload_byte_count, writer.finalize()};
}

[[noreturn]] void rethrow_encode_failure(
    ExactDirectMorseForestSourceWireDecision decision) {
  switch (decision) {
    case ExactDirectMorseForestSourceWireDecision::invalid_limits:
    case ExactDirectMorseForestSourceWireDecision::manifest_not_certified:
    case ExactDirectMorseForestSourceWireDecision::
        source_window_not_certified:
    case ExactDirectMorseForestSourceWireDecision::
        source_window_inconsistent:
      throw std::invalid_argument(
          "the direct Morse forest source wire authority is invalid");
    case ExactDirectMorseForestSourceWireDecision::numeric_overflow:
      throw std::overflow_error(
          "the direct Morse forest source wire size overflows");
    case ExactDirectMorseForestSourceWireDecision::
        encoded_byte_limit_exceeded:
    case ExactDirectMorseForestSourceWireDecision::
        role_record_limit_exceeded:
    case ExactDirectMorseForestSourceWireDecision::
        family_record_limit_exceeded:
    case ExactDirectMorseForestSourceWireDecision::
        arm_seed_record_limit_exceeded:
    case ExactDirectMorseForestSourceWireDecision::
        projection_record_limit_exceeded:
    case ExactDirectMorseForestSourceWireDecision::
        event_record_limit_exceeded:
    case ExactDirectMorseForestSourceWireDecision::
        point_id_reference_limit_exceeded:
    case ExactDirectMorseForestSourceWireDecision::
        exact_text_limit_exceeded:
    case ExactDirectMorseForestSourceWireDecision::
        total_exact_text_limit_exceeded:
      throw std::length_error(
          "the direct Morse forest source wire exceeds a finite limit");
    case ExactDirectMorseForestSourceWireDecision::accepted:
    case ExactDirectMorseForestSourceWireDecision::wire_size_mismatch:
    case ExactDirectMorseForestSourceWireDecision::wire_content_mismatch:
      throw std::logic_error(
          "an internal direct Morse forest source wire decision is invalid");
  }
  throw std::logic_error(
      "an unknown direct Morse forest source wire decision was produced");
}

}  // namespace

bool ExactDirectMorseForestSourceWireLimits::finite() const noexcept {
  const std::size_t unbounded = std::numeric_limits<std::size_t>::max();
  return maximum_encoded_byte_count >=
             direct_morse_forest_source_batch_wire_header_byte_count &&
         maximum_encoded_byte_count != unbounded &&
         maximum_role_record_count != unbounded &&
         maximum_family_record_count != unbounded &&
         maximum_arm_seed_record_count != unbounded &&
         maximum_projection_record_count != unbounded &&
         maximum_event_record_count != unbounded &&
         maximum_point_id_reference_count != unbounded &&
         maximum_exact_text_byte_count != unbounded &&
         maximum_total_exact_text_byte_count != unbounded;
}

ExactDirectMorseForestEncodedSourceBatchWire
encode_exact_direct_morse_forest_source_batch_wire(
    const ExactDirectMorseForestSourceManifest& manifest,
    const ExactDirectMorseForestSourceBatchWindow& window,
    const ExactDirectMorseForestSourceWireLimits& limits) {
  try {
    const PayloadMeasurement measurement =
        measure_payload(manifest, window, limits);
    const std::size_t encoded_byte_count = checked_add(
        direct_morse_forest_source_batch_wire_header_byte_count,
        measurement.byte_count);
    ExactDirectMorseForestEncodedSourceBatchWire result;
    result.bytes.resize(encoded_byte_count);
    const auto header =
        make_header(measurement.byte_count, measurement.digest);
    std::copy(header.begin(), header.end(), result.bytes.begin());
    BufferWriter writer{std::span<std::uint8_t>{result.bytes}.subspan(
        direct_morse_forest_source_batch_wire_header_byte_count)};
    emit_payload(writer, manifest, window, limits);
    if (writer.offset() != measurement.byte_count ||
        writer.finalize() != measurement.digest) {
      throw std::logic_error(
          "the direct Morse forest source wire changed between passes");
    }
    result.receipt = {encoded_byte_count, measurement.digest};
    return result;
  } catch (const WireFailure& failure) {
    rethrow_encode_failure(failure.decision());
  }
}

ExactDirectMorseForestSourceWireVerificationResult
verify_exact_direct_morse_forest_source_batch_wire(
    const ExactDirectMorseForestSourceManifest& manifest,
    const ExactDirectMorseForestSourceBatchWindow& fresh_window,
    std::span<const std::uint8_t> observed_wire,
    const ExactDirectMorseForestSourceWireLimits& limits) noexcept {
  ExactDirectMorseForestSourceWireVerificationResult result;
  try {
    if (!limits.finite()) {
      result.decision =
          ExactDirectMorseForestSourceWireDecision::invalid_limits;
      return result;
    }
    const auto preflight = preflight_observed_wire(observed_wire, limits);
    if (preflight != ExactDirectMorseForestSourceWireDecision::accepted) {
      result.decision = preflight;
      return result;
    }
    const PayloadMeasurement measurement =
        measure_payload(manifest, fresh_window, limits);
    const std::size_t expected_size = checked_add(
        direct_morse_forest_source_batch_wire_header_byte_count,
        measurement.byte_count);
    if (observed_wire.size() != expected_size) {
      result.decision =
          ExactDirectMorseForestSourceWireDecision::wire_size_mismatch;
      return result;
    }
    const auto expected_header =
        make_header(measurement.byte_count, measurement.digest);
    if (!std::equal(
            expected_header.begin(),
            expected_header.end(),
            observed_wire.begin())) {
      result.decision =
          ExactDirectMorseForestSourceWireDecision::wire_content_mismatch;
      return result;
    }
    ComparisonWriter writer{observed_wire.subspan(
        direct_morse_forest_source_batch_wire_header_byte_count)};
    emit_payload(writer, manifest, fresh_window, limits);
    if (!writer.equal_and_complete() ||
        writer.finalize() != measurement.digest) {
      result.decision =
          ExactDirectMorseForestSourceWireDecision::wire_content_mismatch;
      return result;
    }
    result.decision = ExactDirectMorseForestSourceWireDecision::accepted;
    result.receipt = ExactDirectMorseForestSourceBatchWireReceipt{
        expected_size, measurement.digest};
    return result;
  } catch (const WireFailure& failure) {
    result.decision = failure.decision();
    return result;
  } catch (...) {
    result.decision = ExactDirectMorseForestSourceWireDecision::
        source_window_inconsistent;
    return result;
  }
}

ExactDirectMorseForestCanonicalWireSourceProvider::
    ExactDirectMorseForestCanonicalWireSourceProvider(
        ExactDirectMorseForestSourceManifest manifest,
        ExactDirectMorseForestSourceBatchProviderView fresh_source_provider,
        ExactDirectMorseForestSourceBatchWireProviderView wire_provider,
        ExactDirectMorseForestSourceWireLimits limits)
    : manifest_(std::move(manifest)),
      fresh_source_provider_(std::move(fresh_source_provider)),
      wire_provider_(std::move(wire_provider)),
      limits_(limits) {
  if (!manifest_.certified() || !fresh_source_provider_ || !wire_provider_ ||
      !limits_.finite()) {
    throw std::invalid_argument(
        "a canonical source wire provider requires certified finite authorities");
  }
}

ExactDirectMorseForestSourceBatchVisitDecision
ExactDirectMorseForestCanonicalWireSourceProvider::visit_batch(
    std::size_t batch_index,
    ExactDirectMorseForestSourceBatchConsumerView consumer) {
  if (visit_in_progress_) {
    return ExactDirectMorseForestSourceBatchVisitDecision::
        no_window_inconsistent;
  }
  ExactDirectMorseForestSourceWireDecision current_wire_decision =
      ExactDirectMorseForestSourceWireDecision::source_window_inconsistent;
  struct DecisionPublish {
    ExactDirectMorseForestSourceWireDecision* destination{};
    const ExactDirectMorseForestSourceWireDecision* source{};
    ~DecisionPublish() { *destination = *source; }
  } publish{&last_wire_decision_, &current_wire_decision};
  if (batch_index >= manifest_.batch_count) {
    return ExactDirectMorseForestSourceBatchVisitDecision::
        no_batch_out_of_range;
  }
  if (!consumer) {
    return ExactDirectMorseForestSourceBatchVisitDecision::
        no_window_inconsistent;
  }
  visit_in_progress_ = true;
  struct VisitRelease {
    bool* flag{};
    ~VisitRelease() { *flag = false; }
  } release{&visit_in_progress_};

  bool wire_callback_called = false;
  bool fresh_callback_called = false;
  bool canonical_equality = false;
  bool downstream_rejected = false;
  ExactDirectMorseForestSourceBatchVisitDecision fresh_decision =
      ExactDirectMorseForestSourceBatchVisitDecision::no_provider;

  auto inspect_wire = [&](std::span<const std::uint8_t> observed_wire) {
    if (wire_callback_called) {
      current_wire_decision = ExactDirectMorseForestSourceWireDecision::
          source_window_inconsistent;
      return false;
    }
    wire_callback_called = true;
    const auto preflight = preflight_observed_wire(observed_wire, limits_);
    if (preflight !=
        ExactDirectMorseForestSourceWireDecision::accepted) {
      current_wire_decision = preflight;
      return false;
    }

    auto inspect_fresh = [&](const ExactDirectMorseForestSourceBatchWindow&
                                 fresh_window) {
      if (fresh_callback_called) {
        current_wire_decision = ExactDirectMorseForestSourceWireDecision::
            source_window_inconsistent;
        return false;
      }
      fresh_callback_called = true;
      const auto verification =
          verify_exact_direct_morse_forest_source_batch_wire(
              manifest_, fresh_window, observed_wire, limits_);
      current_wire_decision = verification.decision;
      if (!verification.accepted()) {
        return false;
      }
      canonical_equality = true;
      const bool accepted = consumer(fresh_window);
      downstream_rejected = !accepted;
      return accepted;
    };
    fresh_decision = fresh_source_provider_(
        batch_index,
        ExactDirectMorseForestSourceBatchConsumerView{inspect_fresh});
    return fresh_decision ==
               ExactDirectMorseForestSourceBatchVisitDecision::
                   complete_synchronous_visit &&
           canonical_equality && !downstream_rejected;
  };

  const auto wire_decision = wire_provider_(
      batch_index,
      ExactDirectMorseForestSourceBatchWireConsumerView{inspect_wire});
  if (downstream_rejected) {
    return ExactDirectMorseForestSourceBatchVisitDecision::
        no_consumer_rejected;
  }
  if (wire_decision ==
      ExactDirectMorseForestSourceBatchWireVisitDecision::
          no_batch_out_of_range) {
    return ExactDirectMorseForestSourceBatchVisitDecision::
        no_batch_out_of_range;
  }
  if (wire_decision !=
          ExactDirectMorseForestSourceBatchWireVisitDecision::
              complete_synchronous_visit ||
      fresh_decision !=
          ExactDirectMorseForestSourceBatchVisitDecision::
              complete_synchronous_visit ||
      !wire_callback_called || !fresh_callback_called ||
      !canonical_equality ||
      current_wire_decision !=
          ExactDirectMorseForestSourceWireDecision::accepted) {
    return ExactDirectMorseForestSourceBatchVisitDecision::
        no_window_inconsistent;
  }
  if (verified_visit_count_ != std::numeric_limits<std::size_t>::max()) {
    ++verified_visit_count_;
  }
  return ExactDirectMorseForestSourceBatchVisitDecision::
      complete_synchronous_visit;
}

}  // namespace morsehgp3d::hierarchy
