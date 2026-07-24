#include "morsehgp3d/hierarchy/atomic_linear_run_store.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <dirent.h>
#include <fcntl.h>
#include <limits>
#include <new>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

namespace morsehgp3d::hierarchy {
namespace {

constexpr std::string_view lock_file_name = ".atomic-linear-run.lock";
constexpr std::string_view head_file_name = "HEAD";
constexpr std::string_view head_temporary_file_name = ".HEAD.tmp";
constexpr std::string_view final_prefix = "linear-run-";
constexpr std::string_view final_suffix = ".run";
constexpr std::string_view temporary_prefix = ".linear-run-";
constexpr std::string_view temporary_suffix = ".tmp";
constexpr std::size_t sequence_digit_count = 20U;

constexpr std::array<std::uint8_t, 16U> transition_magic{
    'M', 'H', 'G', 'P', '3', 'D', '-', 'L',
    'I', 'N', 'E', 'A', 'R', '-', 'V', '1'};
constexpr std::array<std::uint8_t, 16U> head_magic{
    'M', 'H', 'G', 'P', '3', 'D', '-', 'L',
    'I', 'N', '-', 'H', 'E', 'A', 'D', '1'};
constexpr std::uint8_t transition_wire_kind = 1U;
constexpr std::uint8_t head_wire_kind = 2U;
constexpr std::size_t envelope_header_byte_count = 32U;
constexpr std::size_t transition_metadata_byte_count = 200U;
constexpr std::size_t checksum_byte_count =
    contract::CanonicalId::byte_count;
constexpr std::size_t transition_payload_offset =
    envelope_header_byte_count + transition_metadata_byte_count;
constexpr std::size_t head_body_byte_count = 128U;
constexpr std::string_view run_contract_domain =
    "MorseHGP3D/phase15/atomic-linear/run-contract/v1/sha256/";
constexpr std::string_view output_chain_domain =
    "MorseHGP3D/phase15/atomic-linear/output-chain/v1/sha256/";
constexpr std::string_view transition_checksum_domain =
    "MorseHGP3D/phase15/atomic-linear/transition-wire/v1/sha256/";
constexpr std::string_view head_checksum_domain =
    "MorseHGP3D/phase15/atomic-linear/head-wire/v1/sha256/";

static_assert(
    envelope_header_byte_count + transition_metadata_byte_count +
            checksum_byte_count ==
        atomic_linear_run_transition_fixed_wire_byte_count);
static_assert(
    envelope_header_byte_count + head_body_byte_count +
            checksum_byte_count ==
        atomic_linear_run_head_wire_byte_count);

class UniqueFileDescriptor {
 public:
  UniqueFileDescriptor() noexcept = default;
  explicit UniqueFileDescriptor(int descriptor) noexcept
      : descriptor_(descriptor) {}
  ~UniqueFileDescriptor() {
    if (descriptor_ >= 0) {
      static_cast<void>(::close(descriptor_));
    }
  }

  UniqueFileDescriptor(const UniqueFileDescriptor&) = delete;
  UniqueFileDescriptor& operator=(const UniqueFileDescriptor&) = delete;
  UniqueFileDescriptor(UniqueFileDescriptor&& other) noexcept
      : descriptor_(std::exchange(other.descriptor_, -1)) {}
  UniqueFileDescriptor& operator=(UniqueFileDescriptor&& other) noexcept {
    if (this != &other) {
      if (descriptor_ >= 0) {
        static_cast<void>(::close(descriptor_));
      }
      descriptor_ = std::exchange(other.descriptor_, -1);
    }
    return *this;
  }

  [[nodiscard]] int get() const noexcept { return descriptor_; }
  [[nodiscard]] bool valid() const noexcept { return descriptor_ >= 0; }

 private:
  int descriptor_{-1};
};

[[noreturn]] void throw_system_error(
    int error_number,
    std::string_view message) {
  throw std::system_error(
      error_number, std::generic_category(), std::string{message});
}

[[noreturn]] void throw_last_system_error(std::string_view message) {
  throw_system_error(errno, message);
}

[[nodiscard]] std::size_t checked_add(
    std::size_t left,
    std::size_t right,
    std::string_view message) {
  if (right > std::numeric_limits<std::size_t>::max() - left) {
    throw std::overflow_error(std::string{message});
  }
  return left + right;
}

[[nodiscard]] std::uint64_t checked_increment(
    std::uint64_t value,
    std::string_view message) {
  if (value == std::numeric_limits<std::uint64_t>::max()) {
    throw std::overflow_error(std::string{message});
  }
  return value + 1U;
}

[[nodiscard]] std::uint64_t checked_u64(
    std::size_t value,
    std::string_view message) {
  if constexpr (
      std::numeric_limits<std::size_t>::max() >
      std::numeric_limits<std::uint64_t>::max()) {
    if (value > std::numeric_limits<std::uint64_t>::max()) {
      throw std::overflow_error(std::string{message});
    }
  }
  return static_cast<std::uint64_t>(value);
}

[[nodiscard]] std::size_t checked_size(
    std::uint64_t value,
    std::string_view message) {
  if constexpr (
      std::numeric_limits<std::uint64_t>::max() >
      std::numeric_limits<std::size_t>::max()) {
    if (value > std::numeric_limits<std::size_t>::max()) {
      throw std::overflow_error(std::string{message});
    }
  }
  return static_cast<std::size_t>(value);
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

void store_identifier(
    std::span<std::uint8_t> bytes,
    std::size_t offset,
    const contract::CanonicalId& identifier) {
  std::copy(
      identifier.bytes().begin(),
      identifier.bytes().end(),
      bytes.begin() + static_cast<std::ptrdiff_t>(offset));
}

[[nodiscard]] contract::CanonicalId load_identifier(
    std::span<const std::uint8_t> bytes,
    std::size_t offset) {
  std::array<std::uint8_t, contract::CanonicalId::byte_count> value{};
  std::copy_n(
      bytes.begin() + static_cast<std::ptrdiff_t>(offset),
      value.size(),
      value.begin());
  return contract::CanonicalId{value};
}

void hash_u32(
    contract::CanonicalSha256Builder& builder,
    std::uint32_t value) {
  std::array<std::uint8_t, 4U> bytes{};
  store_u32(bytes, 0U, value);
  builder.update(bytes);
}

void hash_u64(
    contract::CanonicalSha256Builder& builder,
    std::uint64_t value) {
  std::array<std::uint8_t, 8U> bytes{};
  store_u64(bytes, 0U, value);
  builder.update(bytes);
}

[[nodiscard]] contract::CanonicalId compute_contract_digest(
    const AtomicLinearRunContract& contract_value,
    const AtomicLinearRunStoreLimits& limits) {
  contract::CanonicalSha256Builder builder;
  builder.update(run_contract_domain);
  hash_u32(builder, atomic_linear_run_store_schema_version);
  hash_u32(builder, atomic_linear_run_transition_wire_version);
  hash_u32(builder, atomic_linear_run_head_wire_version);
  builder.update(contract_value.application_contract_digest.bytes());
  builder.update(contract_value.initial_checkpoint_digest.bytes());
  builder.update(contract_value.initial_output_chain_digest.bytes());
  hash_u64(builder, contract_value.initial_chunk_index);
  hash_u64(builder, contract_value.initial_batch_index);
  hash_u64(
      builder,
      checked_u64(
          limits.maximum_committed_transition_count,
          "the atomic linear transition cap does not fit uint64"));
  hash_u64(
      builder,
      checked_u64(
          limits.maximum_payload_byte_count,
          "the atomic linear payload cap does not fit uint64"));
  hash_u64(
      builder,
      checked_u64(
          limits.maximum_encoded_transition_byte_count,
          "the atomic linear encoded cap does not fit uint64"));
  hash_u64(
      builder,
      checked_u64(
          limits.maximum_total_encoded_transition_byte_count,
          "the atomic linear total byte cap does not fit uint64"));
  hash_u64(builder, limits.maximum_batch_span);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId compute_output_chain_digest(
    const contract::CanonicalId& source_output_chain_digest,
    const AtomicLinearRunTransition& transition) {
  contract::CanonicalSha256Builder builder;
  builder.update(output_chain_domain);
  builder.update(transition.run_contract_digest.bytes());
  builder.update(source_output_chain_digest.bytes());
  hash_u64(builder, transition.sequence);
  hash_u64(builder, transition.chunk_index);
  hash_u64(builder, transition.batch_begin_index);
  hash_u64(builder, transition.batch_end_index);
  builder.update(transition.source_checkpoint_digest.bytes());
  builder.update(transition.successor_checkpoint_digest.bytes());
  builder.update(transition.budget_snapshot_digest.bytes());
  hash_u64(
      builder,
      checked_u64(
          transition.payload.size(),
          "an atomic linear payload does not fit uint64"));
  builder.update(transition.payload);
  return builder.finalize();
}

[[nodiscard]] contract::CanonicalId checksum(
    std::string_view domain,
    std::span<const std::uint8_t> bytes) {
  contract::CanonicalSha256Builder builder;
  builder.update(domain);
  builder.update(bytes);
  return builder.finalize();
}

[[nodiscard]] std::vector<std::uint8_t> encode_transition(
    AtomicLinearRunTransition& transition) {
  const std::size_t wire_size = checked_add(
      atomic_linear_run_transition_fixed_wire_byte_count,
      transition.payload.size(),
      "an atomic linear transition wire size overflows");
  std::vector<std::uint8_t> bytes(wire_size, 0U);
  std::copy(
      transition_magic.begin(), transition_magic.end(), bytes.begin());
  store_u32(
      bytes, transition_magic.size(),
      atomic_linear_run_transition_wire_version);
  bytes[transition_magic.size() + 4U] = transition_wire_kind;
  store_u64(
      bytes,
      transition_magic.size() + 8U,
      checked_u64(
          transition_metadata_byte_count + transition.payload.size(),
          "an atomic linear transition body does not fit uint64"));

  std::size_t offset = envelope_header_byte_count;
  store_identifier(bytes, offset, transition.run_contract_digest);
  offset += contract::CanonicalId::byte_count;
  store_u64(bytes, offset, transition.sequence);
  offset += 8U;
  store_u64(bytes, offset, transition.chunk_index);
  offset += 8U;
  store_u64(bytes, offset, transition.batch_begin_index);
  offset += 8U;
  store_u64(bytes, offset, transition.batch_end_index);
  offset += 8U;
  store_identifier(bytes, offset, transition.source_checkpoint_digest);
  offset += contract::CanonicalId::byte_count;
  store_identifier(bytes, offset, transition.successor_checkpoint_digest);
  offset += contract::CanonicalId::byte_count;
  store_identifier(bytes, offset, transition.output_chain_digest);
  offset += contract::CanonicalId::byte_count;
  store_identifier(bytes, offset, transition.budget_snapshot_digest);
  offset += contract::CanonicalId::byte_count;
  store_u64(
      bytes,
      offset,
      checked_u64(
          transition.payload.size(),
          "an atomic linear payload does not fit uint64"));
  offset += 8U;
  if (offset != transition_payload_offset) {
    throw std::logic_error(
        "the atomic linear transition layout is inconsistent");
  }
  std::copy(
      transition.payload.begin(),
      transition.payload.end(),
      bytes.begin() + static_cast<std::ptrdiff_t>(offset));
  const std::size_t checksum_offset = bytes.size() - checksum_byte_count;
  transition.wire_sha256 = checksum(
      transition_checksum_domain,
      std::span<const std::uint8_t>{bytes.data(), checksum_offset});
  store_identifier(bytes, checksum_offset, transition.wire_sha256);
  return bytes;
}

[[nodiscard]] AtomicLinearRunTransition decode_transition(
    std::span<const std::uint8_t> bytes,
    const AtomicLinearRunStoreLimits& limits) {
  if (bytes.size() <
          atomic_linear_run_transition_fixed_wire_byte_count ||
      bytes.size() > limits.maximum_encoded_transition_byte_count ||
      !std::equal(
          transition_magic.begin(), transition_magic.end(), bytes.begin()) ||
      load_u32(bytes, transition_magic.size()) !=
          atomic_linear_run_transition_wire_version ||
      bytes[transition_magic.size() + 4U] != transition_wire_kind ||
      bytes[transition_magic.size() + 5U] != 0U ||
      bytes[transition_magic.size() + 6U] != 0U ||
      bytes[transition_magic.size() + 7U] != 0U) {
    throw std::runtime_error(
        "an atomic linear transition has an invalid envelope");
  }
  const std::size_t body_size = checked_size(
      load_u64(bytes, transition_magic.size() + 8U),
      "an atomic linear transition body is not addressable");
  if (body_size != bytes.size() - envelope_header_byte_count -
                       checksum_byte_count ||
      body_size < transition_metadata_byte_count) {
    throw std::runtime_error(
        "an atomic linear transition has an invalid body length");
  }
  const std::size_t payload_size =
      body_size - transition_metadata_byte_count;
  if (payload_size > limits.maximum_payload_byte_count ||
      load_u64(bytes, transition_payload_offset - 8U) !=
          checked_u64(
              payload_size,
              "an atomic linear payload does not fit uint64")) {
    throw std::runtime_error(
        "an atomic linear transition has an invalid payload length");
  }
  const std::size_t checksum_offset = bytes.size() - checksum_byte_count;
  const contract::CanonicalId expected = checksum(
      transition_checksum_domain, bytes.first(checksum_offset));
  const contract::CanonicalId observed =
      load_identifier(bytes, checksum_offset);
  if (expected != observed) {
    throw std::runtime_error(
        "an atomic linear transition SHA-256 does not match");
  }

  AtomicLinearRunTransition transition;
  std::size_t offset = envelope_header_byte_count;
  transition.run_contract_digest = load_identifier(bytes, offset);
  offset += contract::CanonicalId::byte_count;
  transition.sequence = load_u64(bytes, offset);
  offset += 8U;
  transition.chunk_index = load_u64(bytes, offset);
  offset += 8U;
  transition.batch_begin_index = load_u64(bytes, offset);
  offset += 8U;
  transition.batch_end_index = load_u64(bytes, offset);
  offset += 8U;
  transition.source_checkpoint_digest = load_identifier(bytes, offset);
  offset += contract::CanonicalId::byte_count;
  transition.successor_checkpoint_digest = load_identifier(bytes, offset);
  offset += contract::CanonicalId::byte_count;
  transition.output_chain_digest = load_identifier(bytes, offset);
  offset += contract::CanonicalId::byte_count;
  transition.budget_snapshot_digest = load_identifier(bytes, offset);
  offset += contract::CanonicalId::byte_count;
  offset += 8U;
  transition.payload.assign(
      bytes.begin() + static_cast<std::ptrdiff_t>(offset),
      bytes.begin() +
          static_cast<std::ptrdiff_t>(offset + payload_size));
  transition.wire_sha256 = observed;
  return transition;
}

struct HeadRecord {
  contract::CanonicalId run_contract_digest{};
  std::uint64_t committed_transition_count{};
  std::uint64_t total_encoded_transition_byte_count{};
  std::uint64_t next_chunk_index{};
  std::uint64_t next_batch_index{};
  contract::CanonicalId checkpoint_digest{};
  contract::CanonicalId output_chain_digest{};

  friend bool operator==(const HeadRecord&, const HeadRecord&) = default;
};

[[nodiscard]] std::array<
    std::uint8_t,
    atomic_linear_run_head_wire_byte_count>
encode_head(const HeadRecord& head) {
  std::array<std::uint8_t, atomic_linear_run_head_wire_byte_count> bytes{};
  std::copy(head_magic.begin(), head_magic.end(), bytes.begin());
  store_u32(
      bytes, head_magic.size(), atomic_linear_run_head_wire_version);
  bytes[head_magic.size() + 4U] = head_wire_kind;
  store_u64(
      bytes,
      head_magic.size() + 8U,
      static_cast<std::uint64_t>(head_body_byte_count));
  std::size_t offset = envelope_header_byte_count;
  store_identifier(bytes, offset, head.run_contract_digest);
  offset += contract::CanonicalId::byte_count;
  store_u64(bytes, offset, head.committed_transition_count);
  offset += 8U;
  store_u64(bytes, offset, head.total_encoded_transition_byte_count);
  offset += 8U;
  store_u64(bytes, offset, head.next_chunk_index);
  offset += 8U;
  store_u64(bytes, offset, head.next_batch_index);
  offset += 8U;
  store_identifier(bytes, offset, head.checkpoint_digest);
  offset += contract::CanonicalId::byte_count;
  store_identifier(bytes, offset, head.output_chain_digest);
  const std::size_t checksum_offset =
      bytes.size() - checksum_byte_count;
  store_identifier(
      bytes,
      checksum_offset,
      checksum(
          head_checksum_domain,
          std::span<const std::uint8_t>{
              bytes.data(), checksum_offset}));
  return bytes;
}

[[nodiscard]] HeadRecord decode_head(
    std::span<const std::uint8_t> bytes) {
  if (bytes.size() != atomic_linear_run_head_wire_byte_count ||
      !std::equal(head_magic.begin(), head_magic.end(), bytes.begin()) ||
      load_u32(bytes, head_magic.size()) !=
          atomic_linear_run_head_wire_version ||
      bytes[head_magic.size() + 4U] != head_wire_kind ||
      bytes[head_magic.size() + 5U] != 0U ||
      bytes[head_magic.size() + 6U] != 0U ||
      bytes[head_magic.size() + 7U] != 0U ||
      load_u64(bytes, head_magic.size() + 8U) !=
          head_body_byte_count) {
    throw std::runtime_error(
        "an atomic linear HEAD has an invalid envelope");
  }
  const std::size_t checksum_offset =
      bytes.size() - checksum_byte_count;
  if (checksum(
          head_checksum_domain, bytes.first(checksum_offset)) !=
      load_identifier(bytes, checksum_offset)) {
    throw std::runtime_error(
        "an atomic linear HEAD SHA-256 does not match");
  }
  HeadRecord head;
  std::size_t offset = envelope_header_byte_count;
  head.run_contract_digest = load_identifier(bytes, offset);
  offset += contract::CanonicalId::byte_count;
  head.committed_transition_count = load_u64(bytes, offset);
  offset += 8U;
  head.total_encoded_transition_byte_count = load_u64(bytes, offset);
  offset += 8U;
  head.next_chunk_index = load_u64(bytes, offset);
  offset += 8U;
  head.next_batch_index = load_u64(bytes, offset);
  offset += 8U;
  head.checkpoint_digest = load_identifier(bytes, offset);
  offset += contract::CanonicalId::byte_count;
  head.output_chain_digest = load_identifier(bytes, offset);
  return head;
}

[[nodiscard]] std::string sequence_file_name(
    std::uint64_t sequence,
    bool temporary) {
  std::array<char, sequence_digit_count> digits{};
  std::array<char, sequence_digit_count> raw{};
  const auto converted = std::to_chars(
      raw.data(), raw.data() + raw.size(), sequence);
  if (converted.ec != std::errc{}) {
    throw std::overflow_error(
        "an atomic linear sequence cannot be formatted");
  }
  const std::size_t count =
      static_cast<std::size_t>(converted.ptr - raw.data());
  digits.fill('0');
  std::copy_n(
      raw.data(), count, digits.data() + (digits.size() - count));
  const std::string_view prefix =
      temporary ? temporary_prefix : final_prefix;
  const std::string_view suffix =
      temporary ? temporary_suffix : final_suffix;
  std::string result;
  result.reserve(prefix.size() + digits.size() + suffix.size());
  result.append(prefix);
  result.append(digits.data(), digits.size());
  result.append(suffix);
  return result;
}

[[nodiscard]] std::optional<std::uint64_t> parse_sequence_file_name(
    std::string_view name,
    bool temporary) {
  const std::string_view prefix =
      temporary ? temporary_prefix : final_prefix;
  const std::string_view suffix =
      temporary ? temporary_suffix : final_suffix;
  if (!name.starts_with(prefix) || !name.ends_with(suffix) ||
      name.size() !=
          prefix.size() + sequence_digit_count + suffix.size()) {
    return std::nullopt;
  }
  const std::string_view digits = name.substr(
      prefix.size(), sequence_digit_count);
  if (!std::all_of(
          digits.begin(),
          digits.end(),
          [](char value) { return value >= '0' && value <= '9'; })) {
    return std::nullopt;
  }
  std::uint64_t sequence = 0U;
  const auto converted = std::from_chars(
      digits.data(), digits.data() + digits.size(), sequence);
  if (converted.ec != std::errc{} ||
      converted.ptr != digits.data() + digits.size()) {
    return std::nullopt;
  }
  return sequence;
}

[[nodiscard]] struct stat descriptor_metadata(
    int descriptor,
    std::string_view role) {
  struct stat metadata {};
  if (::fstat(descriptor, &metadata) != 0) {
    throw_last_system_error(
        std::string{"cannot inspect "} + std::string{role});
  }
  return metadata;
}

[[nodiscard]] struct stat named_metadata(
    int directory_fd,
    const std::string& name,
    std::string_view role) {
  struct stat metadata {};
  if (::fstatat(
          directory_fd,
          name.c_str(),
          &metadata,
          AT_SYMLINK_NOFOLLOW) != 0) {
    throw_last_system_error(
        std::string{"cannot inspect "} + std::string{role});
  }
  return metadata;
}

void require_regular_file(
    const struct stat& metadata,
    nlink_t expected_link_count,
    std::string_view role) {
  if (!S_ISREG(metadata.st_mode) ||
      metadata.st_nlink != expected_link_count ||
      metadata.st_size < 0) {
    throw std::runtime_error(
        std::string{role} + " is not the expected regular file");
  }
}

[[nodiscard]] bool same_inode(
    const struct stat& left,
    const struct stat& right) noexcept {
  return left.st_dev == right.st_dev && left.st_ino == right.st_ino;
}

[[nodiscard]] bool same_file_snapshot(
    const struct stat& left,
    const struct stat& right) noexcept {
  return same_inode(left, right) && left.st_size == right.st_size &&
         left.st_nlink == right.st_nlink &&
         left.st_mtim.tv_sec == right.st_mtim.tv_sec &&
         left.st_mtim.tv_nsec == right.st_mtim.tv_nsec &&
         left.st_ctim.tv_sec == right.st_ctim.tv_sec &&
         left.st_ctim.tv_nsec == right.st_ctim.tv_nsec;
}

void write_all(
    int descriptor,
    std::span<const std::uint8_t> bytes) {
  std::size_t offset = 0U;
  while (offset < bytes.size()) {
    const ssize_t count = ::pwrite(
        descriptor,
        bytes.data() + offset,
        bytes.size() - offset,
        static_cast<off_t>(offset));
    if (count < 0 && errno == EINTR) {
      continue;
    }
    if (count <= 0) {
      throw_last_system_error("cannot write an atomic linear file");
    }
    offset += static_cast<std::size_t>(count);
  }
}

[[nodiscard]] std::vector<std::uint8_t> read_descriptor_bytes(
    int descriptor,
    std::size_t maximum_byte_count,
    std::string_view role) {
  const struct stat before = descriptor_metadata(descriptor, role);
  if (!S_ISREG(before.st_mode) || before.st_size < 0 ||
      static_cast<std::uintmax_t>(before.st_size) >
          maximum_byte_count) {
    throw std::runtime_error(
        std::string{role} + " exceeds its bounded regular-file contract");
  }
  const std::size_t byte_count =
      static_cast<std::size_t>(before.st_size);
  std::vector<std::uint8_t> bytes(byte_count);
  std::size_t offset = 0U;
  while (offset < bytes.size()) {
    const ssize_t count = ::pread(
        descriptor,
        bytes.data() + offset,
        bytes.size() - offset,
        static_cast<off_t>(offset));
    if (count < 0 && errno == EINTR) {
      continue;
    }
    if (count <= 0) {
      throw_last_system_error(
          std::string{"cannot read "} + std::string{role});
    }
    offset += static_cast<std::size_t>(count);
  }
  const struct stat after = descriptor_metadata(descriptor, role);
  if (!same_file_snapshot(before, after)) {
    throw std::runtime_error(
        std::string{role} + " changed during bounded read");
  }
  return bytes;
}

void verify_descriptor_bytes(
    int descriptor,
    std::span<const std::uint8_t> expected,
    std::string_view role) {
  const auto observed =
      read_descriptor_bytes(descriptor, expected.size(), role);
  if (!std::equal(
          observed.begin(), observed.end(),
          expected.begin(), expected.end())) {
    throw std::runtime_error(
        std::string{role} + " failed exact reread");
  }
}

[[nodiscard]] UniqueFileDescriptor create_temporary_file(
    int directory_fd,
    const std::string& name,
    std::string_view role) {
  UniqueFileDescriptor descriptor{
      ::openat(
          directory_fd,
          name.c_str(),
          O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW |
              O_NONBLOCK,
          S_IRUSR | S_IWUSR)};
  if (!descriptor.valid()) {
    throw_last_system_error(
        std::string{"cannot create "} + std::string{role});
  }
  require_regular_file(
      descriptor_metadata(descriptor.get(), role), 1, role);
  return descriptor;
}

struct ReadFile {
  UniqueFileDescriptor descriptor;
  struct stat metadata {};
  std::vector<std::uint8_t> bytes;
};

[[nodiscard]] ReadFile read_named_file(
    int directory_fd,
    const std::string& name,
    std::size_t maximum_byte_count,
    nlink_t expected_link_count,
    std::string_view role) {
  UniqueFileDescriptor descriptor{
      ::openat(
          directory_fd,
          name.c_str(),
          O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK)};
  if (!descriptor.valid()) {
    throw_last_system_error(
        std::string{"cannot open "} + std::string{role});
  }
  const struct stat metadata = descriptor_metadata(
      descriptor.get(), role);
  require_regular_file(metadata, expected_link_count, role);
  auto bytes = read_descriptor_bytes(
      descriptor.get(), maximum_byte_count, role);
  return {std::move(descriptor), metadata, std::move(bytes)};
}

[[nodiscard]] bool unlink_if_present(
    int directory_fd,
    const std::string& name,
    int& error_number) noexcept {
  if (::unlinkat(directory_fd, name.c_str(), 0) == 0 || errno == ENOENT) {
    return true;
  }
  error_number = errno;
  return false;
}

struct DirectoryInventory {
  bool head_present{false};
  bool head_temporary_present{false};
  std::vector<std::uint64_t> final_sequences;
  std::vector<std::uint64_t> temporary_sequences;
};

[[nodiscard]] DirectoryInventory inventory_directory(
    int directory_fd,
    std::size_t maximum_sequence_entry_count) {
  const int stream_descriptor = ::openat(
      directory_fd,
      ".",
      O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
  if (stream_descriptor < 0) {
    throw_last_system_error(
        "cannot open an independent atomic linear directory inventory");
  }
  DIR* stream = ::fdopendir(stream_descriptor);
  if (stream == nullptr) {
    const int error_number = errno;
    static_cast<void>(::close(stream_descriptor));
    throw_system_error(
        error_number, "cannot inventory an atomic linear directory");
  }
  DirectoryInventory inventory;
  errno = 0;
  while (dirent* entry = ::readdir(stream)) {
    const std::string_view name{entry->d_name};
    if (name == "." || name == ".." || name == lock_file_name) {
      continue;
    }
    if (name == head_file_name) {
      inventory.head_present = true;
      continue;
    }
    if (name == head_temporary_file_name) {
      inventory.head_temporary_present = true;
      continue;
    }
    if (const auto sequence = parse_sequence_file_name(name, false)) {
      if (inventory.final_sequences.size() >=
          maximum_sequence_entry_count -
              inventory.temporary_sequences.size()) {
        const int ignored = ::closedir(stream);
        static_cast<void>(ignored);
        throw std::runtime_error(
            "an atomic linear directory exceeds its sequence-entry cap");
      }
      inventory.final_sequences.push_back(*sequence);
      continue;
    }
    if (const auto sequence = parse_sequence_file_name(name, true)) {
      if (inventory.temporary_sequences.size() >=
          maximum_sequence_entry_count -
              inventory.final_sequences.size()) {
        const int ignored = ::closedir(stream);
        static_cast<void>(ignored);
        throw std::runtime_error(
            "an atomic linear directory exceeds its sequence-entry cap");
      }
      inventory.temporary_sequences.push_back(*sequence);
      continue;
    }
    const int ignored = ::closedir(stream);
    static_cast<void>(ignored);
    throw std::runtime_error(
        "an atomic linear directory contains an unknown entry");
  }
  const int read_error = errno;
  if (::closedir(stream) != 0 && read_error == 0) {
    throw_last_system_error(
        "cannot close an atomic linear directory inventory");
  }
  if (read_error != 0) {
    throw_system_error(
        read_error, "cannot read an atomic linear directory inventory");
  }
  std::sort(
      inventory.final_sequences.begin(),
      inventory.final_sequences.end());
  std::sort(
      inventory.temporary_sequences.begin(),
      inventory.temporary_sequences.end());
  return inventory;
}

void notify(
    const AtomicLinearRunPublishOptions& options,
    AtomicLinearRunPublishStage stage) noexcept {
  if (options.observer != nullptr) {
    options.observer(stage, options.observer_state);
  }
}

void validate_limits(const AtomicLinearRunStoreLimits& limits) {
  const std::size_t maximum = std::numeric_limits<std::size_t>::max();
  if (limits.maximum_committed_transition_count == 0U ||
      limits.maximum_payload_byte_count == 0U ||
      limits.maximum_encoded_transition_byte_count <
          atomic_linear_run_transition_fixed_wire_byte_count ||
      limits.maximum_total_encoded_transition_byte_count <
          atomic_linear_run_transition_fixed_wire_byte_count ||
      limits.maximum_batch_span == 0U ||
      limits.maximum_committed_transition_count == maximum ||
      limits.maximum_payload_byte_count == maximum ||
      limits.maximum_encoded_transition_byte_count == maximum ||
      limits.maximum_total_encoded_transition_byte_count == maximum ||
      checked_add(
          atomic_linear_run_transition_fixed_wire_byte_count,
          limits.maximum_payload_byte_count,
          "the atomic linear payload cap overflows") >
          limits.maximum_encoded_transition_byte_count ||
      static_cast<std::uintmax_t>(
          limits.maximum_encoded_transition_byte_count) >
          static_cast<std::uintmax_t>(
              std::numeric_limits<off_t>::max())) {
    throw std::invalid_argument(
        "atomic linear run limits must be positive, finite and addressable");
  }
  static_cast<void>(checked_u64(
      limits.maximum_committed_transition_count,
      "the atomic linear transition cap does not fit uint64"));
  static_cast<void>(checked_u64(
      limits.maximum_total_encoded_transition_byte_count,
      "the atomic linear total byte cap does not fit uint64"));
}

void validate_callback(
    const AtomicLinearRunRecertifier& recertifier) {
  if (!recertifier) {
    throw std::invalid_argument(
        "an atomic linear run requires a recertifier");
  }
}

[[nodiscard]] HeadRecord head_from_state(
    const contract::CanonicalId& run_contract_digest,
    std::size_t committed_count,
    std::size_t total_bytes,
    const AtomicLinearRunTrustedState& state) {
  return {
      run_contract_digest,
      checked_u64(
          committed_count,
          "the atomic linear committed count does not fit uint64"),
      checked_u64(
          total_bytes,
          "the atomic linear total byte count does not fit uint64"),
      state.next_chunk_index,
      state.next_batch_index,
      state.checkpoint_digest,
      state.output_chain_digest};
}

}  // namespace

contract::CanonicalId compute_atomic_linear_run_contract_digest(
    const AtomicLinearRunContract& contract_value,
    const AtomicLinearRunStoreLimits& limits) {
  validate_limits(limits);
  return compute_contract_digest(contract_value, limits);
}

struct AtomicLinearRunStore::Impl {
  Impl(
      OpenMode mode,
      const std::filesystem::path& dedicated_directory,
      AtomicLinearRunContract supplied_contract,
      AtomicLinearRunStoreLimits supplied_limits,
      AtomicLinearRunRecertifier supplied_recertifier,
      std::optional<AtomicLinearRunExternalAnchor> supplied_anchor)
      : contract_value(std::move(supplied_contract)),
        limits(std::move(supplied_limits)),
        recertifier(std::move(supplied_recertifier)),
        expected_anchor(std::move(supplied_anchor)) {
    validate_limits(limits);
    validate_callback(recertifier);
    run_digest = compute_contract_digest(contract_value, limits);
    reset_trusted_state();
    durable_status.external_anchor_supplied = expected_anchor.has_value();

    directory = UniqueFileDescriptor{
        ::open(
            dedicated_directory.c_str(),
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW)};
    if (!directory.valid()) {
      throw_last_system_error(
          "cannot open the dedicated atomic linear run directory");
    }
    lock = UniqueFileDescriptor{
        ::openat(
            directory.get(),
            std::string{lock_file_name}.c_str(),
            O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK,
            S_IRUSR | S_IWUSR)};
    if (!lock.valid()) {
      throw_last_system_error(
          "cannot open the atomic linear run writer lock");
    }
    require_regular_file(
        descriptor_metadata(
            lock.get(), "the atomic linear run writer lock"),
        1,
        "the atomic linear run writer lock");
    if (::flock(lock.get(), LOCK_EX | LOCK_NB) != 0) {
      throw_last_system_error(
          "cannot acquire the atomic linear run writer lock");
    }
    const struct stat named_lock = named_metadata(
        directory.get(),
        std::string{lock_file_name},
        "the named atomic linear run writer lock");
    if (!same_inode(
            descriptor_metadata(
                lock.get(), "the held atomic linear run writer lock"),
            named_lock)) {
      throw std::runtime_error(
          "the atomic linear run writer lock name changed");
    }
    durable_status.writer_lock_acquired = true;

    if (mode == OpenMode::create_new) {
      if (expected_anchor.has_value()) {
        throw std::invalid_argument(
            "create_new does not accept an external anchor");
      }
      initialize_new();
    } else {
      recover_existing();
    }
  }

  void reset_trusted_state() noexcept {
    trusted = {
        0U,
        contract_value.initial_chunk_index,
        contract_value.initial_batch_index,
        contract_value.initial_checkpoint_digest,
        contract_value.initial_output_chain_digest};
  }

  [[nodiscard]] AtomicLinearRunExternalAnchor current_anchor()
      const noexcept {
    return {
        trusted.next_sequence,
        trusted.next_chunk_index,
        trusted.next_batch_index,
        trusted.checkpoint_digest,
        trusted.output_chain_digest};
  }

  void refresh_status() noexcept {
    durable_status.committed_transition_count = committed_count;
    durable_status.total_encoded_transition_byte_count = total_bytes;
    durable_status.current_anchor = current_anchor();
    durable_status.failed_closed_reopen_required = failed_closed;
    durable_status.process_local_ticket_serialized = false;
    durable_status.retained_transition_history_count = 0U;
    durable_status.global_gamma_cell_count = 0U;
    durable_status.higher_order_delaunay_cell_count = 0U;
  }

  [[nodiscard]] HeadRecord read_head(nlink_t links = 1) const {
    const auto read = read_named_file(
        directory.get(),
        std::string{head_file_name},
        atomic_linear_run_head_wire_byte_count,
        links,
        "the atomic linear run HEAD");
    return decode_head(read.bytes);
  }

  void publish_initial_head(const HeadRecord& head) {
    const auto bytes = encode_head(head);
    bool temporary_present = false;
    bool head_present = false;
    try {
      UniqueFileDescriptor temporary = create_temporary_file(
          directory.get(),
          std::string{head_temporary_file_name},
          "the initial atomic linear HEAD temporary");
      temporary_present = true;
      write_all(temporary.get(), bytes);
      if (::fdatasync(temporary.get()) != 0) {
        throw_last_system_error(
            "cannot synchronize the initial atomic linear HEAD");
      }
      verify_descriptor_bytes(
          temporary.get(),
          bytes,
          "the initial atomic linear HEAD temporary");
      if (::linkat(
              directory.get(),
              std::string{head_temporary_file_name}.c_str(),
              directory.get(),
              std::string{head_file_name}.c_str(),
              0) != 0) {
        throw_last_system_error(
            "cannot publish the initial atomic linear HEAD");
      }
      head_present = true;
      const struct stat temporary_metadata = named_metadata(
          directory.get(),
          std::string{head_temporary_file_name},
          "the linked initial atomic linear HEAD temporary");
      const struct stat head_metadata = named_metadata(
          directory.get(),
          std::string{head_file_name},
          "the linked initial atomic linear HEAD");
      require_regular_file(
          temporary_metadata,
          2,
          "the linked initial atomic linear HEAD temporary");
      require_regular_file(
          head_metadata, 2, "the linked initial atomic linear HEAD");
      if (!same_inode(temporary_metadata, head_metadata)) {
        throw std::runtime_error(
            "the initial atomic linear HEAD hard link changed inode");
      }
      if (::unlinkat(
              directory.get(),
              std::string{head_temporary_file_name}.c_str(),
              0) != 0) {
        throw_last_system_error(
            "cannot remove the initial atomic linear HEAD temporary");
      }
      temporary_present = false;
      if (::fsync(directory.get()) != 0) {
        throw_last_system_error(
            "cannot synchronize the initial atomic linear HEAD directory");
      }
    } catch (...) {
      int ignored = 0;
      if (temporary_present) {
        static_cast<void>(unlink_if_present(
            directory.get(),
            std::string{head_temporary_file_name},
            ignored));
      }
      if (head_present) {
        static_cast<void>(unlink_if_present(
            directory.get(), std::string{head_file_name}, ignored));
      }
      static_cast<void>(::fsync(directory.get()));
      throw;
    }
  }

  void initialize_new() {
    DirectoryInventory inventory =
        inventory_directory(directory.get(), 1U);
    const HeadRecord initial =
        head_from_state(run_digest, 0U, 0U, trusted);
    if (inventory.head_present ||
        !inventory.final_sequences.empty() ||
        !inventory.temporary_sequences.empty()) {
      throw std::runtime_error(
          "create_new requires an empty atomic linear namespace");
    }
    if (inventory.head_temporary_present) {
      static_cast<void>(read_named_file(
          directory.get(),
          std::string{head_temporary_file_name},
          atomic_linear_run_head_wire_byte_count,
          1,
          "an interrupted initial atomic linear HEAD temporary"));
      if (::unlinkat(
              directory.get(),
              std::string{head_temporary_file_name}.c_str(),
              0) != 0 ||
          ::fsync(directory.get()) != 0) {
        throw std::runtime_error(
            "an interrupted initial atomic linear HEAD is invalid");
      }
      ++durable_status.removed_uncommitted_temporary_file_count;
    }
    publish_initial_head(initial);
    authoritative_head = initial;
    durable_status.authoritative_head_certified = true;
    durable_status.linear_prefix_replayed = true;
    refresh_status();
  }

  void validate_head_contract_and_caps(const HeadRecord& head) const {
    if (head.run_contract_digest != run_digest) {
      throw std::runtime_error(
          "an atomic linear HEAD belongs to another run contract");
    }
    const std::size_t count = checked_size(
        head.committed_transition_count,
        "an atomic linear HEAD count is not addressable");
    const std::size_t bytes = checked_size(
        head.total_encoded_transition_byte_count,
        "an atomic linear HEAD byte count is not addressable");
    if (count > limits.maximum_committed_transition_count ||
        bytes > limits.maximum_total_encoded_transition_byte_count ||
        ((count == 0U) != (bytes == 0U)) ||
        (count != 0U &&
         count >
             bytes /
                 atomic_linear_run_transition_fixed_wire_byte_count) ||
        (count != 0U &&
         (bytes / count >
              limits.maximum_encoded_transition_byte_count ||
          (bytes / count ==
               limits.maximum_encoded_transition_byte_count &&
           bytes % count != 0U)))) {
      throw std::runtime_error(
          "an atomic linear HEAD exceeds the trusted limits");
    }
  }

  [[nodiscard]] bool transition_shape_matches(
      const AtomicLinearRunTransition& transition,
      const AtomicLinearRunTrustedState& source_state) const {
    if (transition.schema_version !=
            atomic_linear_run_store_schema_version ||
        transition.run_contract_digest != run_digest ||
        transition.sequence != source_state.next_sequence ||
        transition.chunk_index != source_state.next_chunk_index ||
        transition.batch_begin_index != source_state.next_batch_index ||
        transition.batch_end_index <= transition.batch_begin_index ||
        transition.batch_end_index - transition.batch_begin_index >
            limits.maximum_batch_span ||
        transition.source_checkpoint_digest !=
            source_state.checkpoint_digest ||
        transition.payload.size() > limits.maximum_payload_byte_count ||
        checked_add(
            atomic_linear_run_transition_fixed_wire_byte_count,
            transition.payload.size(),
            "an atomic linear transition size overflows") >
            limits.maximum_encoded_transition_byte_count) {
      return false;
    }
    return transition.output_chain_digest ==
           compute_output_chain_digest(
               source_state.output_chain_digest, transition);
  }

  [[nodiscard]] AtomicLinearRunTrustedState successor_state(
      const AtomicLinearRunTransition& transition) const {
    return {
        checked_increment(
            transition.sequence,
            "the atomic linear transition sequence is exhausted"),
        checked_increment(
            transition.chunk_index,
            "the atomic linear chunk index is exhausted"),
        transition.batch_end_index,
        transition.successor_checkpoint_digest,
        transition.output_chain_digest};
  }

  [[nodiscard]] AtomicLinearRunRecertification recertify(
      const AtomicLinearRunTransition& transition,
      AtomicLinearRunRecertificationPhase phase) {
    AtomicLinearRunRecertification result;
    try {
      result = recertifier(transition, phase);
    } catch (...) {
      return {};
    }
    if (phase == AtomicLinearRunRecertificationPhase::publication) {
      ++durable_status.publication_recertification_count;
    } else if (
        phase == AtomicLinearRunRecertificationPhase::recovery) {
      ++durable_status.recovery_recertification_count;
    } else {
      ++durable_status.uncommitted_cleanup_recertification_count;
    }
    return result;
  }

  void verify_expected_anchor_at_current_prefix() {
    if (!expected_anchor.has_value() ||
        durable_status.external_anchor_verified) {
      return;
    }
    if (expected_anchor->committed_transition_count ==
        trusted.next_sequence) {
      if (*expected_anchor != current_anchor()) {
        throw std::runtime_error(
            "the external atomic linear anchor disagrees with replay");
      }
      durable_status.external_anchor_verified = true;
    }
  }

  [[nodiscard]] AtomicLinearRunTransition read_and_recertify(
      std::uint64_t sequence,
      nlink_t expected_links,
      const AtomicLinearRunTrustedState& source_state,
      std::size_t maximum_encoded_byte_count,
      AtomicLinearRunRecertificationPhase phase,
      std::size_t& encoded_byte_count) {
    const ReadFile read = read_named_file(
        directory.get(),
        sequence_file_name(sequence, false),
        maximum_encoded_byte_count,
        expected_links,
        "an atomic linear transition");
    AtomicLinearRunTransition transition =
        decode_transition(read.bytes, limits);
    if (!transition_shape_matches(transition, source_state)) {
      throw std::runtime_error(
          "an atomic linear transition breaks the trusted chain");
    }
    const AtomicLinearRunRecertification replay =
        recertify(transition, phase);
    if (!replay.accepted()) {
      throw std::runtime_error(
          "an atomic linear transition failed recovery recertification");
    }
    encoded_byte_count = read.bytes.size();
    durable_status.maximum_observed_payload_byte_count = std::max(
        durable_status.maximum_observed_payload_byte_count,
        transition.payload.size());
    durable_status.maximum_observed_encoded_transition_byte_count =
        std::max(
            durable_status.maximum_observed_encoded_transition_byte_count,
            read.bytes.size());
    return transition;
  }

  void normalize_initial_head_link_window(
      DirectoryInventory& inventory) {
    if (!inventory.head_temporary_present ||
        !inventory.head_present ||
        authoritative_head.committed_transition_count != 0U ||
        !inventory.final_sequences.empty() ||
        !inventory.temporary_sequences.empty()) {
      return;
    }
    const struct stat head = named_metadata(
        directory.get(),
        std::string{head_file_name},
        "the initial atomic linear HEAD");
    const struct stat temporary = named_metadata(
        directory.get(),
        std::string{head_temporary_file_name},
        "the initial atomic linear HEAD temporary");
    require_regular_file(head, 2, "the initial atomic linear HEAD");
    require_regular_file(
        temporary, 2, "the initial atomic linear HEAD temporary");
    if (!same_inode(head, temporary) ||
        ::unlinkat(
            directory.get(),
            std::string{head_temporary_file_name}.c_str(),
            0) != 0 ||
        ::fsync(directory.get()) != 0) {
      throw std::runtime_error(
          "the initial atomic linear HEAD link window is invalid");
    }
    ++durable_status.removed_uncommitted_temporary_file_count;
    inventory.head_temporary_present = false;
  }

  void recover_existing() {
    DirectoryInventory inventory = inventory_directory(
        directory.get(),
        checked_add(
            limits.maximum_committed_transition_count,
            1U,
            "the atomic linear inventory cap overflows"));
    if (!inventory.head_present) {
      throw std::runtime_error(
          "open_existing requires an authoritative atomic linear HEAD");
    }

    nlink_t head_links = 1;
    if (inventory.head_temporary_present &&
        inventory.final_sequences.empty() &&
        inventory.temporary_sequences.empty()) {
      head_links = 2;
    }
    authoritative_head = read_head(head_links);
    validate_head_contract_and_caps(authoritative_head);
    normalize_initial_head_link_window(inventory);

    const std::size_t head_count = checked_size(
        authoritative_head.committed_transition_count,
        "the atomic linear committed count is not addressable");
    const std::size_t head_total_bytes = checked_size(
        authoritative_head.total_encoded_transition_byte_count,
        "the atomic linear HEAD bytes are not addressable");
    if (expected_anchor.has_value() &&
        expected_anchor->committed_transition_count >
            authoritative_head.committed_transition_count) {
      throw std::runtime_error(
          "the atomic linear directory is older than its external anchor");
    }
    if (inventory.temporary_sequences.size() > 1U ||
        inventory.final_sequences.size() < head_count ||
        inventory.final_sequences.size() > head_count + 1U) {
      throw std::runtime_error(
          "the atomic linear directory disagrees with HEAD");
    }
    for (std::size_t index = 0U;
         index < inventory.final_sequences.size();
         ++index) {
      if (inventory.final_sequences[index] !=
          static_cast<std::uint64_t>(index)) {
        throw std::runtime_error(
            "atomic linear transition files are not contiguous");
      }
    }
    if (!inventory.temporary_sequences.empty() &&
        inventory.temporary_sequences.front() !=
            authoritative_head.committed_transition_count) {
      throw std::runtime_error(
          "an atomic linear temporary has the wrong sequence");
    }

    reset_trusted_state();
    committed_count = 0U;
    total_bytes = 0U;
    verify_expected_anchor_at_current_prefix();
    for (std::size_t index = 0U; index < head_count; ++index) {
      std::size_t encoded_byte_count = 0U;
      const std::size_t remaining_head_bytes =
          head_total_bytes - total_bytes;
      AtomicLinearRunTransition transition = read_and_recertify(
          static_cast<std::uint64_t>(index),
          1,
          trusted,
          std::min(
              limits.maximum_encoded_transition_byte_count,
              remaining_head_bytes),
          AtomicLinearRunRecertificationPhase::recovery,
          encoded_byte_count);
      total_bytes = checked_add(
          total_bytes,
          encoded_byte_count,
          "the recovered atomic linear byte count overflows");
      if (total_bytes >
          limits.maximum_total_encoded_transition_byte_count) {
        throw std::runtime_error(
            "the recovered atomic linear prefix exceeds its byte cap");
      }
      trusted = successor_state(transition);
      ++committed_count;
      ++durable_status.recovered_transition_count;
      verify_expected_anchor_at_current_prefix();
    }
    if (total_bytes != head_total_bytes ||
        trusted.next_sequence !=
            authoritative_head.committed_transition_count ||
        trusted.next_chunk_index != authoritative_head.next_chunk_index ||
        trusted.next_batch_index != authoritative_head.next_batch_index ||
        trusted.checkpoint_digest !=
            authoritative_head.checkpoint_digest ||
        trusted.output_chain_digest !=
            authoritative_head.output_chain_digest) {
      throw std::runtime_error(
          "the replayed atomic linear prefix disagrees with HEAD");
    }
    if (expected_anchor.has_value() &&
        !durable_status.external_anchor_verified) {
      throw std::runtime_error(
          "the external atomic linear anchor was not crossed by replay");
    }

    const bool orphan_present =
        inventory.final_sequences.size() == head_count + 1U;
    const bool any_uncommitted_entry =
        orphan_present || !inventory.temporary_sequences.empty() ||
        inventory.head_temporary_present;
    if (any_uncommitted_entry &&
        (committed_count >=
             limits.maximum_committed_transition_count ||
         total_bytes >=
             limits.maximum_total_encoded_transition_byte_count)) {
      throw std::runtime_error(
          "a capped atomic linear prefix cannot have an uncommitted suffix");
    }
    bool directory_mutated = false;
    if (orphan_present) {
      const bool temporary_present =
          !inventory.temporary_sequences.empty();
      std::size_t orphan_bytes = 0U;
      AtomicLinearRunTransition orphan = read_and_recertify(
          authoritative_head.committed_transition_count,
          temporary_present ? 2 : 1,
          trusted,
          std::min(
              limits.maximum_encoded_transition_byte_count,
              limits.maximum_total_encoded_transition_byte_count -
                  total_bytes),
          AtomicLinearRunRecertificationPhase::
              recovery_uncommitted_cleanup,
          orphan_bytes);
      const AtomicLinearRunTrustedState orphan_successor =
          successor_state(orphan);
      const HeadRecord expected_next_head = head_from_state(
          run_digest,
          checked_add(
              committed_count,
              1U,
              "an atomic linear orphan count overflows"),
          checked_add(
              total_bytes,
              orphan_bytes,
              "an atomic linear orphan byte count overflows"),
          orphan_successor);
      if (expected_next_head.committed_transition_count >
              checked_u64(
                  limits.maximum_committed_transition_count,
                  "the atomic linear transition cap does not fit uint64") ||
          expected_next_head.total_encoded_transition_byte_count >
              checked_u64(
                  limits.maximum_total_encoded_transition_byte_count,
                  "the atomic linear total byte cap does not fit uint64")) {
        throw std::runtime_error(
            "an atomic linear orphan exceeds its run limits");
      }
      if (inventory.head_temporary_present) {
        // Before rename, HEAD.tmp is never authoritative.  Its write may
        // have torn at any byte despite a full apparent size, so only its
        // bounded regular-file shape is trusted during suffix cleanup.
        static_cast<void>(read_named_file(
            directory.get(),
            std::string{head_temporary_file_name},
            atomic_linear_run_head_wire_byte_count,
            1,
            "an uncommitted atomic linear HEAD temporary"));
      }
      if (temporary_present) {
        const struct stat final_metadata = named_metadata(
            directory.get(),
            sequence_file_name(
                authoritative_head.committed_transition_count, false),
            "an atomic linear orphan final");
        const struct stat temporary_metadata = named_metadata(
            directory.get(),
            sequence_file_name(
                authoritative_head.committed_transition_count, true),
            "an atomic linear orphan temporary");
        if (!same_inode(final_metadata, temporary_metadata)) {
          throw std::runtime_error(
              "an atomic linear orphan links two different inodes");
        }
      }
      int error_number = 0;
      if (inventory.head_temporary_present &&
          !unlink_if_present(
              directory.get(),
              std::string{head_temporary_file_name},
              error_number)) {
        throw_system_error(
            error_number,
            "cannot remove an uncommitted atomic linear HEAD");
      }
      if (temporary_present &&
          !unlink_if_present(
              directory.get(),
              sequence_file_name(
                  authoritative_head.committed_transition_count, true),
              error_number)) {
        throw_system_error(
            error_number,
            "cannot remove an atomic linear transition temporary");
      }
      if (!unlink_if_present(
              directory.get(),
              sequence_file_name(
                  authoritative_head.committed_transition_count, false),
              error_number)) {
        throw_system_error(
            error_number,
            "cannot remove an atomic linear transition orphan");
      }
      durable_status.removed_uncommitted_temporary_file_count +=
          static_cast<std::size_t>(temporary_present) +
          static_cast<std::size_t>(
              inventory.head_temporary_present);
      ++durable_status.removed_uncommitted_final_file_count;
      directory_mutated = true;
    } else {
      if (inventory.head_temporary_present) {
        throw std::runtime_error(
            "an atomic linear HEAD temporary has no transition orphan");
      }
      if (!inventory.temporary_sequences.empty()) {
        const std::string temporary_name = sequence_file_name(
            authoritative_head.committed_transition_count, true);
        const struct stat temporary_metadata = named_metadata(
            directory.get(),
            temporary_name,
            "an uncommitted atomic linear transition temporary");
        require_regular_file(
            temporary_metadata,
            1,
            "an uncommitted atomic linear transition temporary");
        if (static_cast<std::uintmax_t>(temporary_metadata.st_size) >
                limits.maximum_encoded_transition_byte_count ||
            static_cast<std::uintmax_t>(temporary_metadata.st_size) >
                limits.maximum_total_encoded_transition_byte_count -
                    total_bytes) {
          throw std::runtime_error(
              "an uncommitted atomic linear temporary exceeds its cap");
        }
        if (::unlinkat(
                directory.get(), temporary_name.c_str(), 0) != 0) {
          throw_last_system_error(
              "cannot remove an uncommitted atomic linear temporary");
        }
        ++durable_status.removed_uncommitted_temporary_file_count;
        directory_mutated = true;
      }
    }
    if (directory_mutated && ::fsync(directory.get()) != 0) {
      throw_last_system_error(
          "cannot synchronize atomic linear recovery cleanup");
    }
    durable_status.authoritative_head_certified = true;
    durable_status.linear_prefix_replayed = true;
    refresh_status();
  }

  [[nodiscard]] bool cleanup_pre_head_publication(
      const std::string& temporary_name,
      const std::string& final_name,
      bool temporary_present,
      bool final_present,
      bool head_temporary_present,
      int& error_number) noexcept {
    bool okay = true;
    if (head_temporary_present) {
      okay = unlink_if_present(
                 directory.get(),
                 std::string{head_temporary_file_name},
                 error_number) &&
             okay;
    }
    if (temporary_present) {
      okay = unlink_if_present(
                 directory.get(), temporary_name, error_number) &&
             okay;
    }
    if (final_present) {
      okay = unlink_if_present(
                 directory.get(), final_name, error_number) &&
             okay;
    }
    if (::fsync(directory.get()) != 0) {
      error_number = errno;
      okay = false;
    }
    return okay;
  }

  [[nodiscard]] bool authoritative_head_still_certified()
      const noexcept {
    try {
      return read_head() == authoritative_head;
    } catch (...) {
      return false;
    }
  }

  [[nodiscard]] AtomicLinearRunPublishResult publish(
      AtomicLinearRunChunkProposal proposal,
      AtomicLinearRunPublishOptions options) {
    AtomicLinearRunPublishResult result;
    result.current_anchor = current_anchor();
    result.committed_transition_count = committed_count;
    result.total_encoded_transition_byte_count = total_bytes;
    if (failed_closed) {
      result.decision = AtomicLinearRunPublishDecision::
          indeterminate_io_failure_reopen_required;
      return result;
    }
    if (committed_count >=
            limits.maximum_committed_transition_count ||
        proposal.payload.size() > limits.maximum_payload_byte_count ||
        total_bytes >
            limits.maximum_total_encoded_transition_byte_count) {
      result.decision =
          AtomicLinearRunPublishDecision::store_limit_rejected;
      return result;
    }
    if (proposal.chunk_index != trusted.next_chunk_index ||
        proposal.batch_begin_index != trusted.next_batch_index ||
        proposal.batch_end_index <= proposal.batch_begin_index ||
        proposal.batch_end_index - proposal.batch_begin_index >
            limits.maximum_batch_span ||
        trusted.next_sequence ==
            std::numeric_limits<std::uint64_t>::max() ||
        trusted.next_chunk_index ==
            std::numeric_limits<std::uint64_t>::max()) {
      result.decision =
          AtomicLinearRunPublishDecision::transition_shape_rejected;
      return result;
    }

    AtomicLinearRunTransition transition;
    transition.run_contract_digest = run_digest;
    transition.sequence = trusted.next_sequence;
    transition.chunk_index = proposal.chunk_index;
    transition.batch_begin_index = proposal.batch_begin_index;
    transition.batch_end_index = proposal.batch_end_index;
    transition.source_checkpoint_digest = trusted.checkpoint_digest;
    transition.successor_checkpoint_digest =
        proposal.successor_checkpoint_digest;
    transition.budget_snapshot_digest = proposal.budget_snapshot_digest;
    transition.payload = std::move(proposal.payload);
    transition.output_chain_digest = compute_output_chain_digest(
        trusted.output_chain_digest, transition);

    std::vector<std::uint8_t> encoded;
    try {
      encoded = encode_transition(transition);
    } catch (const std::bad_alloc&) {
      result.decision =
          AtomicLinearRunPublishDecision::store_limit_rejected;
      return result;
    } catch (const std::exception&) {
      result.decision =
          AtomicLinearRunPublishDecision::transition_shape_rejected;
      return result;
    }
    if (encoded.size() >
            limits.maximum_encoded_transition_byte_count ||
        encoded.size() >
            limits.maximum_total_encoded_transition_byte_count -
                total_bytes) {
      result.decision =
          AtomicLinearRunPublishDecision::store_limit_rejected;
      return result;
    }
    result.encoded_transition_byte_count = encoded.size();
    result.recertification = recertify(
        transition, AtomicLinearRunRecertificationPhase::publication);
    if (!result.recertification.accepted()) {
      result.decision =
          AtomicLinearRunPublishDecision::recertification_rejected;
      return result;
    }

    const AtomicLinearRunTrustedState next_state =
        successor_state(transition);
    const std::size_t next_count = checked_add(
        committed_count,
        1U,
        "the atomic linear committed count overflows");
    const std::size_t next_total = checked_add(
        total_bytes,
        encoded.size(),
        "the atomic linear total byte count overflows");
    const HeadRecord next_head = head_from_state(
        run_digest, next_count, next_total, next_state);
    const auto encoded_head = encode_head(next_head);
    const std::string temporary_name =
        sequence_file_name(transition.sequence, true);
    const std::string final_name =
        sequence_file_name(transition.sequence, false);

    UniqueFileDescriptor transition_temporary;
    UniqueFileDescriptor head_temporary;
    bool transition_temporary_present = false;
    bool transition_final_present = false;
    bool head_temporary_present = false;
    bool head_replaced = false;
    try {
      transition_temporary = create_temporary_file(
          directory.get(),
          temporary_name,
          "an atomic linear transition temporary");
      transition_temporary_present = true;
      write_all(transition_temporary.get(), encoded);
      notify(
          options,
          AtomicLinearRunPublishStage::
              transition_temporary_file_written);
      if (::fdatasync(transition_temporary.get()) != 0) {
        throw_last_system_error(
            "cannot synchronize an atomic linear transition temporary");
      }
      verify_descriptor_bytes(
          transition_temporary.get(),
          encoded,
          "the synchronized atomic linear transition temporary");
      if (decode_transition(encoded, limits) != transition) {
        throw std::runtime_error(
            "an atomic linear transition reread changed its record");
      }
      notify(
          options,
          AtomicLinearRunPublishStage::
              transition_temporary_file_synchronized_and_reread);
      verify_descriptor_bytes(
          transition_temporary.get(),
          encoded,
          "the observer-visible atomic linear transition temporary");

      if (::linkat(
              directory.get(),
              temporary_name.c_str(),
              directory.get(),
              final_name.c_str(),
              0) != 0) {
        throw_last_system_error(
            "cannot create an immutable atomic linear transition link");
      }
      transition_final_present = true;
      notify(
          options,
          AtomicLinearRunPublishStage::
              transition_immutable_link_created);
      const auto verify_linked_transition =
          [&](nlink_t expected_links) {
            const struct stat descriptor = descriptor_metadata(
                transition_temporary.get(),
                "the atomic linear transition descriptor");
            const struct stat named = named_metadata(
                directory.get(),
                final_name,
                "the atomic linear transition final");
            require_regular_file(
                descriptor,
                expected_links,
                "the atomic linear transition descriptor");
            require_regular_file(
                named,
                expected_links,
                "the atomic linear transition final");
            if (!same_inode(descriptor, named)) {
              throw std::runtime_error(
                  "the atomic linear transition final changed inode");
            }
            verify_descriptor_bytes(
                transition_temporary.get(),
                encoded,
                "the linked atomic linear transition");
          };
      verify_linked_transition(2);
      if (::unlinkat(
              directory.get(), temporary_name.c_str(), 0) != 0) {
        throw_last_system_error(
            "cannot remove an atomic linear transition temporary link");
      }
      transition_temporary_present = false;
      notify(
          options,
          AtomicLinearRunPublishStage::
              transition_temporary_link_removed);
      verify_linked_transition(1);
      if (::fsync(directory.get()) != 0) {
        throw_last_system_error(
            "cannot synchronize an atomic linear transition link");
      }
      notify(
          options,
          AtomicLinearRunPublishStage::
              transition_directory_synchronized);
      verify_linked_transition(1);

      head_temporary = create_temporary_file(
          directory.get(),
          std::string{head_temporary_file_name},
          "an atomic linear HEAD temporary");
      head_temporary_present = true;
      write_all(head_temporary.get(), encoded_head);
      notify(
          options,
          AtomicLinearRunPublishStage::
              head_temporary_file_written);
      if (::fdatasync(head_temporary.get()) != 0) {
        throw_last_system_error(
            "cannot synchronize an atomic linear HEAD temporary");
      }
      verify_descriptor_bytes(
          head_temporary.get(),
          encoded_head,
          "the synchronized atomic linear HEAD temporary");
      if (decode_head(encoded_head) != next_head) {
        throw std::runtime_error(
            "an atomic linear HEAD reread changed its record");
      }
      notify(
          options,
          AtomicLinearRunPublishStage::
              head_temporary_file_synchronized_and_reread);
      verify_descriptor_bytes(
          head_temporary.get(),
          encoded_head,
          "the observer-visible atomic linear HEAD temporary");
      verify_linked_transition(1);
      if (read_head() != authoritative_head) {
        throw std::runtime_error(
            "the authoritative atomic linear HEAD changed before rename");
      }
      const struct stat verified_head_descriptor = descriptor_metadata(
          head_temporary.get(),
          "the verified atomic linear HEAD descriptor");
      if (::renameat(
              directory.get(),
              std::string{head_temporary_file_name}.c_str(),
              directory.get(),
              std::string{head_file_name}.c_str()) != 0) {
        throw_last_system_error(
            "cannot replace the authoritative atomic linear HEAD");
      }
      head_temporary_present = false;
      head_replaced = true;

      const auto verify_publication = [&]() {
        const struct stat published_head = named_metadata(
            directory.get(),
            std::string{head_file_name},
            "the published atomic linear HEAD");
        require_regular_file(
            published_head, 1, "the published atomic linear HEAD");
        if (!same_inode(
                verified_head_descriptor, published_head)) {
          throw std::runtime_error(
              "the published atomic linear HEAD changed inode");
        }
        verify_descriptor_bytes(
            head_temporary.get(),
            encoded_head,
            "the published atomic linear HEAD");
        verify_linked_transition(1);
      };
      verify_publication();
      notify(options, AtomicLinearRunPublishStage::head_replaced);
      verify_publication();
      if (::fsync(directory.get()) != 0) {
        throw_last_system_error(
            "cannot synchronize the authoritative atomic linear HEAD");
      }
      notify(
          options,
          AtomicLinearRunPublishStage::
              head_directory_synchronized);
      verify_publication();
    } catch (const std::system_error& error) {
      result.system_error_number = error.code().value();
      transition_temporary = UniqueFileDescriptor{};
      head_temporary = UniqueFileDescriptor{};
      if (!head_replaced &&
          result.system_error_number != EEXIST &&
          cleanup_pre_head_publication(
              temporary_name,
              final_name,
              transition_temporary_present,
              transition_final_present,
              head_temporary_present,
              result.system_error_number) &&
          authoritative_head_still_certified()) {
        result.decision =
            AtomicLinearRunPublishDecision::retryable_io_failure;
        return result;
      }
      failed_closed = true;
      durable_status.authoritative_head_certified = false;
      refresh_status();
      result.decision = AtomicLinearRunPublishDecision::
          indeterminate_io_failure_reopen_required;
      return result;
    } catch (...) {
      transition_temporary = UniqueFileDescriptor{};
      head_temporary = UniqueFileDescriptor{};
      failed_closed = true;
      durable_status.authoritative_head_certified = false;
      refresh_status();
      result.decision = AtomicLinearRunPublishDecision::
          indeterminate_io_failure_reopen_required;
      return result;
    }

    authoritative_head = next_head;
    trusted = next_state;
    committed_count = next_count;
    total_bytes = next_total;
    durable_status.maximum_observed_payload_byte_count = std::max(
        durable_status.maximum_observed_payload_byte_count,
        transition.payload.size());
    durable_status.maximum_observed_encoded_transition_byte_count =
        std::max(
            durable_status.maximum_observed_encoded_transition_byte_count,
            encoded.size());
    refresh_status();
    result.decision =
        AtomicLinearRunPublishDecision::durably_published;
    result.current_anchor = current_anchor();
    result.committed_transition_count = committed_count;
    result.total_encoded_transition_byte_count = total_bytes;
    result.trusted_state_advanced = true;
    return result;
  }

  AtomicLinearRunContract contract_value;
  AtomicLinearRunStoreLimits limits;
  AtomicLinearRunRecertifier recertifier;
  std::optional<AtomicLinearRunExternalAnchor> expected_anchor;
  contract::CanonicalId run_digest{};
  AtomicLinearRunTrustedState trusted{};
  HeadRecord authoritative_head{};
  UniqueFileDescriptor directory;
  UniqueFileDescriptor lock;
  AtomicLinearRunStoreStatus durable_status{};
  std::size_t committed_count{};
  std::size_t total_bytes{};
  bool failed_closed{false};
};

AtomicLinearRunStore::AtomicLinearRunStore(
    OpenMode mode,
    const std::filesystem::path& dedicated_directory,
    AtomicLinearRunContract contract_value,
    AtomicLinearRunStoreLimits limits,
    AtomicLinearRunRecertifier recertifier,
    std::optional<AtomicLinearRunExternalAnchor> expected_anchor)
    : impl_(std::make_unique<Impl>(
          mode,
          dedicated_directory,
          std::move(contract_value),
          std::move(limits),
          std::move(recertifier),
          std::move(expected_anchor))) {}

AtomicLinearRunStore AtomicLinearRunStore::create_new(
    const std::filesystem::path& dedicated_directory,
    AtomicLinearRunContract contract_value,
    AtomicLinearRunStoreLimits limits,
    AtomicLinearRunRecertifier recertifier) {
  return AtomicLinearRunStore{
      OpenMode::create_new,
      dedicated_directory,
      std::move(contract_value),
      std::move(limits),
      std::move(recertifier),
      std::nullopt};
}

AtomicLinearRunStore AtomicLinearRunStore::open_existing(
    const std::filesystem::path& dedicated_directory,
    AtomicLinearRunContract contract_value,
    AtomicLinearRunStoreLimits limits,
    AtomicLinearRunRecertifier recertifier,
    std::optional<AtomicLinearRunExternalAnchor> expected_anchor) {
  return AtomicLinearRunStore{
      OpenMode::open_existing,
      dedicated_directory,
      std::move(contract_value),
      std::move(limits),
      std::move(recertifier),
      std::move(expected_anchor)};
}

AtomicLinearRunStore::~AtomicLinearRunStore() = default;

AtomicLinearRunPublishResult AtomicLinearRunStore::publish_next(
    AtomicLinearRunChunkProposal proposal,
    AtomicLinearRunPublishOptions options) {
  return impl_->publish(std::move(proposal), options);
}

const AtomicLinearRunTrustedState& AtomicLinearRunStore::trusted_state()
    const noexcept {
  return impl_->trusted;
}

const contract::CanonicalId& AtomicLinearRunStore::run_contract_digest()
    const noexcept {
  return impl_->run_digest;
}

const AtomicLinearRunStoreStatus& AtomicLinearRunStore::status()
    const noexcept {
  return impl_->durable_status;
}

}  // namespace morsehgp3d::hierarchy
