#include "morsehgp3d/hierarchy/pair_support_stream_durable.hpp"

#include "morsehgp3d/contract/canonical_id.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <dirent.h>
#include <fcntl.h>
#include <limits>
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

constexpr std::string_view lock_file_name = ".pair-support.lock";
constexpr std::string_view head_file_name = "HEAD";
constexpr std::string_view head_temporary_file_name = ".HEAD.tmp";
constexpr std::string_view final_prefix = "pair-support-";
constexpr std::string_view final_suffix = ".p9c";
constexpr std::string_view temporary_prefix = ".pair-support-";
constexpr std::string_view temporary_suffix = ".tmp";
constexpr std::size_t sequence_digit_count = 20U;

constexpr std::array<std::uint8_t, 16U> head_magic{
    'M', 'H', 'G', 'P', '3', 'D', '-', 'P',
    'A', 'I', 'R', '-', 'H', 'E', 'A', 'D'};
constexpr std::uint32_t head_wire_version = 1U;
constexpr std::uint8_t head_wire_kind = 2U;
constexpr std::uint8_t head_wire_flags = 0U;
constexpr std::size_t head_payload_byte_count = 80U;
constexpr std::size_t head_header_byte_count =
    head_magic.size() + 4U + 1U + 1U + 8U;
constexpr std::size_t head_without_checksum_byte_count =
    head_header_byte_count + head_payload_byte_count;
constexpr std::size_t head_checksum_byte_count =
    contract::CanonicalId::byte_count;
constexpr std::size_t head_wire_byte_count =
    head_without_checksum_byte_count + head_checksum_byte_count;
static_assert(head_wire_byte_count == 142U);
constexpr std::string_view head_checksum_domain =
    "MorseHGP3D/phase9/pair-support/head-wire/v1/sha256/";
constexpr std::string_view run_contract_domain =
    "MorseHGP3D/phase9/pair-support/run-contract/v1/sha256/";

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

[[nodiscard]] bool is_unbounded(std::size_t value) noexcept {
  return value == std::numeric_limits<std::size_t>::max();
}

[[nodiscard]] bool config_has_unbounded_limit(
    const ExactPairSupportDurableConfig& config) noexcept {
  const ExactPairSupportStreamBudget& budget = config.fixed_chunk_budget;
  const ExactPairSupportStreamCodecLimits& codec = config.codec_limits;
  return is_unbounded(config.maximum_committed_transition_count) ||
         is_unbounded(config.maximum_total_encoded_byte_count) ||
         is_unbounded(budget.maximum_work_unit_count) ||
         is_unbounded(budget.maximum_frontier_entry_count) ||
         is_unbounded(budget.maximum_auxiliary_frontier_entry_count) ||
         is_unbounded(budget.maximum_emitted_record_count) ||
         is_unbounded(
             budget.maximum_emitted_point_id_reference_count) ||
         is_unbounded(budget.maximum_global_closed_ball_query_count) ||
         is_unbounded(budget.maximum_point_classification_count) ||
         is_unbounded(codec.maximum_encoded_byte_count) ||
         is_unbounded(codec.maximum_frontier_entry_count) ||
         is_unbounded(codec.maximum_auxiliary_entry_count) ||
         is_unbounded(codec.maximum_record_count) ||
         is_unbounded(codec.maximum_point_id_reference_count) ||
         is_unbounded(codec.maximum_exact_text_byte_count) ||
         is_unbounded(codec.maximum_total_exact_text_byte_count);
}

void validate_config(const ExactPairSupportDurableConfig& config) {
  if (config.maximum_committed_transition_count == 0U ||
      config.maximum_total_encoded_byte_count == 0U ||
      config.fixed_chunk_budget.maximum_work_unit_count == 0U ||
      config_has_unbounded_limit(config) ||
      static_cast<std::uintmax_t>(
          config.codec_limits.maximum_encoded_byte_count) >
          static_cast<std::uintmax_t>(
              std::numeric_limits<off_t>::max())) {
    throw std::invalid_argument(
        "durable pair-support caps must be positive, finite and addressable");
  }
  static_cast<void>(checked_u64(
      config.maximum_committed_transition_count,
      "the durable transition cap does not fit uint64"));
  static_cast<void>(checked_u64(
      config.maximum_total_encoded_byte_count,
      "the durable byte cap does not fit uint64"));
}

void store_u32(
    std::span<std::uint8_t> bytes,
    std::size_t offset,
    std::uint32_t value) noexcept {
  for (std::size_t index = 0U; index < 4U; ++index) {
    const std::size_t shift = (3U - index) * 8U;
    bytes[offset + index] =
        static_cast<std::uint8_t>(value >> shift);
  }
}

void store_u64(
    std::span<std::uint8_t> bytes,
    std::size_t offset,
    std::uint64_t value) noexcept {
  for (std::size_t index = 0U; index < 8U; ++index) {
    const std::size_t shift = (7U - index) * 8U;
    bytes[offset + index] =
        static_cast<std::uint8_t>(value >> shift);
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
  std::array<std::uint8_t, contract::CanonicalId::byte_count> identifier{};
  std::copy_n(
      bytes.begin() + static_cast<std::ptrdiff_t>(offset),
      identifier.size(),
      identifier.begin());
  return contract::CanonicalId{identifier};
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

[[nodiscard]] contract::CanonicalId compute_run_contract_digest(
    const ExactPairSupportAuthorityContext& authority,
    const ExactPairSupportStreamBudget& budget,
    const ExactPairSupportCheckpoint& initial_checkpoint) {
  contract::CanonicalSha256Builder builder;
  builder.update(run_contract_domain);
  hash_u32(builder, pair_support_checkpoint_schema_version);
  hash_u32(builder, pair_support_traversal_version);
  hash_u32(builder, pair_support_stream_chunk_codec_version);
  hash_u32(builder, pair_support_durable_schema_version);
  builder.update(initial_checkpoint.checkpoint_digest.bytes());
  builder.update(authority.manifest().semantic_digest.bytes());
  hash_u64(builder, checked_u64(
      budget.maximum_work_unit_count,
      "the durable work budget does not fit uint64"));
  hash_u64(builder, checked_u64(
      budget.maximum_frontier_entry_count,
      "the durable frontier budget does not fit uint64"));
  hash_u64(builder, checked_u64(
      budget.maximum_auxiliary_frontier_entry_count,
      "the durable auxiliary budget does not fit uint64"));
  hash_u64(builder, checked_u64(
      budget.maximum_emitted_record_count,
      "the durable record budget does not fit uint64"));
  hash_u64(builder, checked_u64(
      budget.maximum_emitted_point_id_reference_count,
      "the durable point-id budget does not fit uint64"));
  hash_u64(builder, checked_u64(
      budget.maximum_global_closed_ball_query_count,
      "the durable closed-ball budget does not fit uint64"));
  hash_u64(builder, checked_u64(
      budget.maximum_point_classification_count,
      "the durable classification budget does not fit uint64"));
  return builder.finalize();
}

struct HeadRecord {
  contract::CanonicalId run_contract_digest{};
  std::uint64_t committed_transition_count{};
  std::uint64_t total_encoded_byte_count{};
  contract::CanonicalId committed_checkpoint_digest{};

  friend bool operator==(const HeadRecord&, const HeadRecord&) = default;
};

[[nodiscard]] contract::CanonicalId compute_head_checksum(
    std::span<const std::uint8_t> bytes_without_checksum) {
  contract::CanonicalSha256Builder builder;
  builder.update(head_checksum_domain);
  builder.update(bytes_without_checksum);
  return builder.finalize();
}

[[nodiscard]] std::array<std::uint8_t, head_wire_byte_count>
encode_head(const HeadRecord& head) {
  std::array<std::uint8_t, head_wire_byte_count> bytes{};
  std::copy(head_magic.begin(), head_magic.end(), bytes.begin());
  store_u32(bytes, head_magic.size(), head_wire_version);
  bytes[head_magic.size() + 4U] = head_wire_kind;
  bytes[head_magic.size() + 5U] = head_wire_flags;
  store_u64(
      bytes,
      head_magic.size() + 6U,
      static_cast<std::uint64_t>(head_payload_byte_count));
  std::size_t offset = head_header_byte_count;
  store_identifier(bytes, offset, head.run_contract_digest);
  offset += contract::CanonicalId::byte_count;
  store_u64(bytes, offset, head.committed_transition_count);
  offset += 8U;
  store_u64(bytes, offset, head.total_encoded_byte_count);
  offset += 8U;
  store_identifier(bytes, offset, head.committed_checkpoint_digest);
  const contract::CanonicalId checksum = compute_head_checksum(
      std::span<const std::uint8_t>{
          bytes.data(), head_without_checksum_byte_count});
  store_identifier(bytes, head_without_checksum_byte_count, checksum);
  return bytes;
}

[[nodiscard]] HeadRecord decode_head(
    std::span<const std::uint8_t> bytes) {
  if (bytes.size() != head_wire_byte_count) {
    throw std::runtime_error(
        "a durable pair-support HEAD has an invalid fixed size");
  }
  if (!std::equal(head_magic.begin(), head_magic.end(), bytes.begin())) {
    throw std::runtime_error(
        "a durable pair-support HEAD has invalid magic");
  }
  if (load_u32(bytes, head_magic.size()) != head_wire_version ||
      bytes[head_magic.size() + 4U] != head_wire_kind ||
      bytes[head_magic.size() + 5U] != head_wire_flags ||
      load_u64(bytes, head_magic.size() + 6U) !=
          head_payload_byte_count) {
    throw std::runtime_error(
        "a durable pair-support HEAD has an unsupported envelope");
  }
  const contract::CanonicalId expected_checksum = compute_head_checksum(
      bytes.first(head_without_checksum_byte_count));
  const contract::CanonicalId observed_checksum = load_identifier(
      bytes, head_without_checksum_byte_count);
  if (expected_checksum != observed_checksum) {
    throw std::runtime_error(
        "a durable pair-support HEAD checksum does not match");
  }
  HeadRecord head;
  std::size_t offset = head_header_byte_count;
  head.run_contract_digest = load_identifier(bytes, offset);
  offset += contract::CanonicalId::byte_count;
  head.committed_transition_count = load_u64(bytes, offset);
  offset += 8U;
  head.total_encoded_byte_count = load_u64(bytes, offset);
  offset += 8U;
  head.committed_checkpoint_digest = load_identifier(bytes, offset);
  return head;
}

[[nodiscard]] std::string sequence_file_name(
    std::uint64_t sequence,
    bool temporary) {
  std::array<char, sequence_digit_count> digits{};
  std::array<char, sequence_digit_count> unpadded{};
  const auto conversion = std::to_chars(
      unpadded.data(), unpadded.data() + unpadded.size(), sequence);
  if (conversion.ec != std::errc{}) {
    throw std::overflow_error(
        "a durable pair-support sequence cannot be formatted");
  }
  const std::size_t written = static_cast<std::size_t>(
      conversion.ptr - unpadded.data());
  if (written > digits.size()) {
    throw std::overflow_error(
        "a durable pair-support sequence exceeds twenty digits");
  }
  const std::size_t padding = digits.size() - written;
  digits.fill('0');
  std::copy_n(unpadded.data(), written, digits.data() + padding);

  const std::string_view prefix =
      temporary ? temporary_prefix : final_prefix;
  const std::string_view suffix =
      temporary ? temporary_suffix : final_suffix;
  std::string name;
  name.reserve(prefix.size() + digits.size() + suffix.size());
  name.append(prefix);
  name.append(digits.data(), digits.size());
  name.append(suffix);
  return name;
}

[[nodiscard]] std::optional<std::uint64_t> parse_sequence_file_name(
    std::string_view name,
    bool temporary) {
  const std::string_view prefix =
      temporary ? temporary_prefix : final_prefix;
  const std::string_view suffix =
      temporary ? temporary_suffix : final_suffix;
  if (!name.starts_with(prefix) || !name.ends_with(suffix) ||
      name.size() != prefix.size() + sequence_digit_count + suffix.size()) {
    return std::nullopt;
  }
  const std::string_view digits =
      name.substr(prefix.size(), sequence_digit_count);
  std::uint64_t sequence = 0U;
  const auto parsed = std::from_chars(
      digits.data(), digits.data() + digits.size(), sequence);
  if (parsed.ec != std::errc{} ||
      parsed.ptr != digits.data() + digits.size()) {
    return std::nullopt;
  }
  return sequence;
}

[[nodiscard]] bool has_reserved_prefix(std::string_view name) noexcept {
  return name.starts_with(final_prefix) ||
         name.starts_with(temporary_prefix) ||
         name.starts_with("HEAD") || name.starts_with(".HEAD") ||
         name == lock_file_name;
}

struct DirectoryInventory {
  std::size_t final_file_count{};
  std::optional<std::uint64_t> maximum_final_sequence;
  std::optional<std::uint64_t> temporary_sequence;
  bool head_present{false};
  bool head_temporary_present{false};
};

[[nodiscard]] DirectoryInventory inventory_directory(
    int directory_fd,
    std::size_t maximum_final_file_count) {
  const int duplicate = ::fcntl(directory_fd, F_DUPFD_CLOEXEC, 0);
  if (duplicate < 0) {
    throw_last_system_error(
        "cannot duplicate the durable pair-support directory descriptor");
  }
  DIR* raw_directory = ::fdopendir(duplicate);
  if (raw_directory == nullptr) {
    const int error_number = errno;
    static_cast<void>(::close(duplicate));
    throw_system_error(
        error_number, "cannot enumerate the durable pair-support directory");
  }

  DirectoryInventory inventory;
  try {
    errno = 0;
    while (dirent* entry = ::readdir(raw_directory)) {
      const std::string_view name{entry->d_name};
      if (name == "." || name == ".." || name == lock_file_name) {
        errno = 0;
        continue;
      }
      if (name == head_file_name) {
        inventory.head_present = true;
        errno = 0;
        continue;
      }
      if (name == head_temporary_file_name) {
        inventory.head_temporary_present = true;
        errno = 0;
        continue;
      }
      if (const auto sequence = parse_sequence_file_name(name, false)) {
        inventory.final_file_count = checked_add(
            inventory.final_file_count,
            1U,
            "the durable pair-support file count overflows size_t");
        if (inventory.final_file_count > maximum_final_file_count) {
          throw std::runtime_error(
              "the durable pair-support transition inventory exceeds its cap");
        }
        if (!inventory.maximum_final_sequence.has_value() ||
            *sequence > *inventory.maximum_final_sequence) {
          inventory.maximum_final_sequence = *sequence;
        }
        errno = 0;
        continue;
      }
      if (const auto sequence = parse_sequence_file_name(name, true)) {
        if (inventory.temporary_sequence.has_value()) {
          throw std::runtime_error(
              "the durable pair-support directory contains multiple transition temporaries");
        }
        inventory.temporary_sequence = *sequence;
        errno = 0;
        continue;
      }
      if (has_reserved_prefix(name)) {
        throw std::runtime_error(
            "the durable pair-support directory contains a malformed reserved name");
      }
      throw std::runtime_error(
          "the durable pair-support directory is not dedicated to this run");
    }
    if (errno != 0) {
      throw_last_system_error(
          "cannot finish enumerating the durable pair-support directory");
    }
  } catch (...) {
    static_cast<void>(::closedir(raw_directory));
    throw;
  }
  if (::closedir(raw_directory) != 0) {
    throw_last_system_error(
        "cannot close the durable pair-support directory enumeration");
  }
  return inventory;
}

void require_regular_file_with_links(
    const struct stat& metadata,
    nlink_t expected_link_count,
    std::string_view role) {
  if (!S_ISREG(metadata.st_mode) ||
      metadata.st_nlink != expected_link_count) {
    throw std::runtime_error(
        std::string{role} + " has invalid type or link count");
  }
}

[[nodiscard]] struct stat descriptor_metadata(
    int descriptor,
    std::string_view role) {
  struct stat metadata {};
  if (::fstat(descriptor, &metadata) != 0) {
    throw_last_system_error(std::string{"cannot inspect "} + std::string{role});
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
    throw_last_system_error(std::string{"cannot inspect "} + std::string{role});
  }
  return metadata;
}

[[nodiscard]] bool same_inode(
    const struct stat& left,
    const struct stat& right) noexcept {
  return left.st_dev == right.st_dev && left.st_ino == right.st_ino;
}

void require_lock_identity(int directory_fd, int lock_fd) {
  const struct stat descriptor = descriptor_metadata(
      lock_fd, "the durable pair-support writer lock");
  require_regular_file_with_links(
      descriptor, 1, "the durable pair-support writer lock");
  const struct stat named = named_metadata(
      directory_fd,
      std::string{lock_file_name},
      "the durable pair-support writer lock name");
  require_regular_file_with_links(
      named, 1, "the durable pair-support writer lock name");
  if (!same_inode(descriptor, named)) {
    throw std::runtime_error(
        "the durable pair-support writer lock path changed during acquisition");
  }
}

struct ReadFile {
  std::vector<std::uint8_t> bytes;
  struct stat metadata {};
};

[[nodiscard]] ReadFile read_bounded_regular_file(
    int directory_fd,
    const std::string& name,
    std::size_t maximum_file_byte_count,
    nlink_t expected_link_count,
    std::string_view role) {
  UniqueFileDescriptor descriptor{
      ::openat(
          directory_fd,
          name.c_str(),
          O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK)};
  if (!descriptor.valid()) {
    throw_last_system_error(std::string{"cannot open "} + std::string{role});
  }
  ReadFile read;
  read.metadata = descriptor_metadata(descriptor.get(), role);
  require_regular_file_with_links(
      read.metadata, expected_link_count, role);
  if (read.metadata.st_size < 0) {
    throw std::runtime_error(std::string{role} + " has negative size");
  }
  const std::uintmax_t unsigned_size =
      static_cast<std::uintmax_t>(read.metadata.st_size);
  if (unsigned_size > maximum_file_byte_count ||
      unsigned_size > std::numeric_limits<std::size_t>::max()) {
    throw std::runtime_error(
        std::string{role} + " exceeds its byte cap");
  }
  const std::size_t size = static_cast<std::size_t>(unsigned_size);
  read.bytes.resize(size);
  std::size_t offset = 0U;
  while (offset < read.bytes.size()) {
    const ssize_t count = ::pread(
        descriptor.get(),
        read.bytes.data() + offset,
        read.bytes.size() - offset,
        static_cast<off_t>(offset));
    if (count < 0) {
      if (errno == EINTR) {
        continue;
      }
      throw_last_system_error(std::string{"cannot read "} + std::string{role});
    }
    if (count == 0) {
      throw std::runtime_error(
          std::string{role} + " was truncated during reading");
    }
    offset = checked_add(
        offset,
        static_cast<std::size_t>(count),
        "a durable pair-support read offset overflows size_t");
  }
  const struct stat after = descriptor_metadata(descriptor.get(), role);
  if (!same_inode(read.metadata, after) ||
      after.st_size != read.metadata.st_size ||
      after.st_nlink != read.metadata.st_nlink ||
      !S_ISREG(after.st_mode)) {
    throw std::runtime_error(
        std::string{role} + " changed during reading");
  }
  return read;
}

[[nodiscard]] HeadRecord read_head_file(
    int directory_fd,
    nlink_t expected_link_count = 1) {
  const ReadFile read = read_bounded_regular_file(
      directory_fd,
      std::string{head_file_name},
      head_wire_byte_count,
      expected_link_count,
      "the authoritative durable pair-support HEAD");
  return decode_head(read.bytes);
}

void write_all(int descriptor, std::span<const std::uint8_t> bytes) {
  std::size_t offset = 0U;
  while (offset < bytes.size()) {
    const ssize_t count = ::write(
        descriptor,
        bytes.data() + offset,
        bytes.size() - offset);
    if (count < 0) {
      if (errno == EINTR) {
        continue;
      }
      throw_last_system_error(
          "cannot write a durable pair-support temporary file");
    }
    if (count == 0) {
      throw std::runtime_error(
          "a durable pair-support temporary write made no progress");
    }
    offset = checked_add(
        offset,
        static_cast<std::size_t>(count),
        "a durable pair-support write offset overflows size_t");
  }
}

void verify_written_bytes(
    int descriptor,
    std::span<const std::uint8_t> expected,
    std::string_view role) {
  const struct stat before = descriptor_metadata(descriptor, role);
  require_regular_file_with_links(before, 1, role);
  if (before.st_size < 0 ||
      static_cast<std::uintmax_t>(before.st_size) != expected.size()) {
    throw std::runtime_error(
        std::string{role} + " has an unexpected size after writing");
  }
  std::array<std::uint8_t, 64U * 1024U> buffer{};
  std::size_t offset = 0U;
  while (offset < expected.size()) {
    const std::size_t requested =
        std::min(buffer.size(), expected.size() - offset);
    const ssize_t count = ::pread(
        descriptor,
        buffer.data(),
        requested,
        static_cast<off_t>(offset));
    if (count < 0) {
      if (errno == EINTR) {
        continue;
      }
      throw_last_system_error(
          "cannot reread a durable pair-support temporary file");
    }
    if (count == 0) {
      throw std::runtime_error(
          "a durable pair-support temporary file failed readback");
    }
    const std::size_t read_count = static_cast<std::size_t>(count);
    if (!std::equal(
            buffer.begin(),
            buffer.begin() + static_cast<std::ptrdiff_t>(read_count),
            expected.begin() + static_cast<std::ptrdiff_t>(offset))) {
      throw std::runtime_error(
          "a durable pair-support temporary file changed during readback");
    }
    offset = checked_add(
        offset,
        read_count,
        "a durable pair-support readback offset overflows size_t");
  }
  const struct stat after = descriptor_metadata(descriptor, role);
  if (!same_inode(before, after) || after.st_size != before.st_size ||
      after.st_nlink != 1 || !S_ISREG(after.st_mode)) {
    throw std::runtime_error(
        std::string{role} + " changed during readback");
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
          O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK,
          S_IRUSR | S_IWUSR)};
  if (!descriptor.valid()) {
    throw_last_system_error(std::string{"cannot create "} + std::string{role});
  }
  const struct stat metadata = descriptor_metadata(descriptor.get(), role);
  require_regular_file_with_links(metadata, 1, role);
  return descriptor;
}

void notify(
    const ExactPairSupportDurablePublishOptions& options,
    ExactPairSupportDurablePublishStage stage) noexcept {
  if (options.observer != nullptr) {
    options.observer(stage, options.observer_state);
  }
}

[[nodiscard]] bool transition_makes_progress(
    const ExactPairSupportStreamChunk& chunk) noexcept {
  return chunk.next_checkpoint.complete() ||
         chunk.cumulative_audit_after.work_unit_count >
             chunk.cumulative_audit_before.work_unit_count;
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

}  // namespace

struct ExactPairSupportDurableSink::Impl {
  Impl(
      OpenMode mode,
      const std::filesystem::path& dedicated_directory,
      const ExactPairSupportAuthorityContext& authority,
      ExactPairSupportDurableConfig supplied_config,
      std::optional<ExactPairSupportDurableExternalPrefixAnchor>
          supplied_expected_prefix_anchor)
      : config(std::move(supplied_config)),
        verifier(authority),
        expected_prefix_anchor(
            std::move(supplied_expected_prefix_anchor)) {
    validate_config(config);
    run_contract_digest = compute_run_contract_digest(
        authority,
        config.fixed_chunk_budget,
        verifier.trusted_checkpoint());
    durable_status.external_prefix_anchor_supplied =
        expected_prefix_anchor.has_value();

    directory = UniqueFileDescriptor{
        ::open(
            dedicated_directory.c_str(),
            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW)};
    if (!directory.valid()) {
      throw_last_system_error(
          "cannot open the dedicated durable pair-support directory");
    }

    lock = UniqueFileDescriptor{
        ::openat(
            directory.get(),
            std::string{lock_file_name}.c_str(),
            O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK,
            S_IRUSR | S_IWUSR)};
    if (!lock.valid()) {
      throw_last_system_error(
          "cannot open the durable pair-support writer lock");
    }
    const struct stat lock_metadata = descriptor_metadata(
        lock.get(), "the durable pair-support writer lock");
    require_regular_file_with_links(
        lock_metadata, 1, "the durable pair-support writer lock");
    if (::flock(lock.get(), LOCK_EX | LOCK_NB) != 0) {
      throw_last_system_error(
          "cannot acquire the durable pair-support writer lock");
    }
    require_lock_identity(directory.get(), lock.get());
    durable_status.writer_lock_acquired = true;

    if (mode == OpenMode::create_new) {
      if (expected_prefix_anchor.has_value()) {
        throw std::invalid_argument(
            "create_new does not accept an external prefix anchor");
      }
      initialize_new();
    } else {
      recover_existing();
    }
  }

  [[nodiscard]] ExactPairSupportDurableExternalPrefixAnchor
  current_anchor() const noexcept {
    return ExactPairSupportDurableExternalPrefixAnchor{
        authoritative_head.committed_transition_count,
        authoritative_head.committed_checkpoint_digest};
  }

  void refresh_status() noexcept {
    const ExactPairSupportIncrementalVerifierStatus& verifier_status =
        verifier.status();
    durable_status.committed_transition_count = committed_transition_count;
    durable_status.total_encoded_byte_count = total_encoded_byte_count;
    durable_status.current_prefix_anchor = current_anchor();
    durable_status.anchored_prefix_certified =
        verifier_status.anchored_prefix_certified;
    durable_status.anchored_run_certified =
        verifier_status.anchored_run_certified;
    durable_status.terminal_checkpoint_reached =
        verifier_status.terminal_checkpoint_reached;
    durable_status.failed_closed =
        io_failed_closed || verifier_status.failed_closed;
    durable_status.retained_chunk_history_count = 0U;
    durable_status.persistent_top_m_cell_count = 0U;
    durable_status.global_gamma_coface_count = 0U;
    durable_status.global_gamma_incidence_count = 0U;
    durable_status.materialized_pair_arena_count = 0U;
  }

  void initialize_new() {
    const ExactPairSupportCheckpoint& initial =
        verifier.trusted_checkpoint();
    const HeadRecord head{
        run_contract_digest,
        0U,
        0U,
        initial.checkpoint_digest};
    const auto encoded = encode_head(head);
    const DirectoryInventory inventory = inventory_directory(
        directory.get(), 0U);
    const bool lone_initial_temporary =
        !inventory.head_present && inventory.head_temporary_present &&
        inventory.final_file_count == 0U &&
        !inventory.temporary_sequence.has_value();
    if (inventory.head_present || inventory.final_file_count != 0U ||
        inventory.temporary_sequence.has_value() ||
        (inventory.head_temporary_present && !lone_initial_temporary)) {
      throw std::runtime_error(
          "create_new requires an empty durable pair-support namespace");
    }
    if (lone_initial_temporary) {
      validate_head_temporary(head);
      if (::unlinkat(
              directory.get(),
              std::string{head_temporary_file_name}.c_str(),
              0) != 0) {
        throw_last_system_error(
            "cannot remove an interrupted initial durable pair-support HEAD temporary");
      }
      durable_status.removed_uncommitted_temporary_file_count = 1U;
      if (::fsync(directory.get()) != 0) {
        throw_last_system_error(
            "cannot synchronize interrupted initial HEAD cleanup");
      }
    }

    bool temporary_present = false;
    bool head_link_present = false;
    try {
      UniqueFileDescriptor temporary = create_temporary_file(
          directory.get(),
          std::string{head_temporary_file_name},
          "the initial durable pair-support HEAD temporary");
      temporary_present = true;
      write_all(temporary.get(), encoded);
      if (::fdatasync(temporary.get()) != 0) {
        throw_last_system_error(
            "cannot synchronize the initial durable pair-support HEAD");
      }
      verify_written_bytes(
          temporary.get(),
          encoded,
          "the initial durable pair-support HEAD temporary");
      if (::linkat(
              directory.get(),
              std::string{head_temporary_file_name}.c_str(),
              directory.get(),
              std::string{head_file_name}.c_str(),
              0) != 0) {
        throw_last_system_error(
            "cannot publish a no-replace initial durable pair-support HEAD");
      }
      head_link_present = true;
      const struct stat linked_temporary = named_metadata(
          directory.get(),
          std::string{head_temporary_file_name},
          "the linked initial durable pair-support HEAD temporary");
      const struct stat linked_head = named_metadata(
          directory.get(),
          std::string{head_file_name},
          "the linked initial durable pair-support HEAD");
      require_regular_file_with_links(
          linked_temporary,
          2,
          "the linked initial durable pair-support HEAD temporary");
      require_regular_file_with_links(
          linked_head,
          2,
          "the linked initial durable pair-support HEAD");
      if (!same_inode(linked_temporary, linked_head)) {
        throw std::runtime_error(
            "the no-replace initial HEAD publication did not preserve one inode");
      }
      if (::unlinkat(
              directory.get(),
              std::string{head_temporary_file_name}.c_str(),
              0) != 0) {
        throw_last_system_error(
            "cannot remove the initial durable pair-support HEAD temporary link");
      }
      temporary_present = false;
      if (::fsync(directory.get()) != 0) {
        throw_last_system_error(
            "cannot synchronize the initial durable pair-support HEAD");
      }
    } catch (...) {
      int ignored_error = 0;
      if (temporary_present) {
        static_cast<void>(unlink_if_present(
            directory.get(),
            std::string{head_temporary_file_name},
            ignored_error));
      }
      if (head_link_present) {
        static_cast<void>(unlink_if_present(
            directory.get(),
            std::string{head_file_name},
            ignored_error));
      }
      static_cast<void>(::fsync(directory.get()));
      throw;
    }

    authoritative_head = head;
    durable_status.authoritative_head_certified = true;
    refresh_status();
  }

  void validate_head_contract_and_caps(const HeadRecord& head) const {
    if (head.run_contract_digest != run_contract_digest) {
      throw std::runtime_error(
          "the durable pair-support HEAD belongs to another run contract");
    }
    const std::size_t count = checked_size(
        head.committed_transition_count,
        "the durable pair-support HEAD count does not fit size_t");
    const std::size_t byte_count = checked_size(
        head.total_encoded_byte_count,
        "the durable pair-support HEAD byte count does not fit size_t");
    if (count > config.maximum_committed_transition_count ||
        byte_count > config.maximum_total_encoded_byte_count) {
      throw std::runtime_error(
          "the durable pair-support HEAD exceeds the external caps");
    }
    if ((count == 0U) != (byte_count == 0U)) {
      throw std::runtime_error(
          "the durable pair-support HEAD has inconsistent empty-prefix counters");
    }
  }

  void validate_inventory_against_head(
      const DirectoryInventory& inventory,
      std::size_t committed_count,
      bool initial_head_link_window) const {
    if (inventory.maximum_final_sequence.has_value()) {
      if (*inventory.maximum_final_sequence ==
          std::numeric_limits<std::uint64_t>::max()) {
        throw std::runtime_error(
            "the durable pair-support transition sequence is exhausted");
      }
      const std::uint64_t contiguous_count =
          *inventory.maximum_final_sequence + 1U;
      if (contiguous_count > std::numeric_limits<std::size_t>::max() ||
          static_cast<std::size_t>(contiguous_count) !=
              inventory.final_file_count) {
        throw std::runtime_error(
            "the durable pair-support transition files are not contiguous from zero");
      }
    } else if (inventory.final_file_count != 0U) {
      throw std::logic_error(
          "a durable pair-support directory inventory is inconsistent");
    }

    const std::size_t maximum_allowed_count = checked_add(
        committed_count,
        1U,
        "the durable pair-support HEAD count cannot admit an orphan");
    if (inventory.final_file_count < committed_count ||
        inventory.final_file_count > maximum_allowed_count) {
      throw std::runtime_error(
          "the durable pair-support final files disagree with authoritative HEAD");
    }
    const std::uint64_t expected_temporary_sequence = checked_u64(
        committed_count,
        "the next durable pair-support sequence does not fit uint64");
    if (inventory.temporary_sequence.has_value() &&
        *inventory.temporary_sequence != expected_temporary_sequence) {
      throw std::runtime_error(
          "a durable pair-support temporary does not match authoritative HEAD");
    }
    const bool orphan_present =
        inventory.final_file_count == maximum_allowed_count;
    if (inventory.head_temporary_present && !orphan_present &&
        !initial_head_link_window) {
      throw std::runtime_error(
          "a durable pair-support HEAD temporary has no transition orphan");
    }
    if (inventory.head_temporary_present &&
        inventory.temporary_sequence.has_value()) {
      throw std::runtime_error(
          "durable pair-support HEAD and transition temporaries cannot coexist");
    }
  }

  void verify_external_anchor_at_current_prefix() {
    if (!expected_prefix_anchor.has_value() ||
        durable_status.external_prefix_anchor_verified) {
      return;
    }
    const std::uint64_t current_count = checked_u64(
        committed_transition_count,
        "the recovered durable count does not fit uint64");
    if (expected_prefix_anchor->committed_transition_count == current_count) {
      if (expected_prefix_anchor->checkpoint_digest !=
          verifier.trusted_checkpoint().checkpoint_digest) {
        throw std::runtime_error(
            "the external durable prefix anchor checkpoint does not match replay");
      }
      durable_status.external_prefix_anchor_verified = true;
    }
  }

  [[nodiscard]] ReadFile read_transition(
      std::uint64_t sequence,
      std::size_t current_total,
      std::size_t maximum_total,
      nlink_t expected_link_count,
      std::string_view role) const {
    if (current_total > maximum_total) {
      throw std::runtime_error(
          "the durable pair-support byte prefix already exceeds its cap");
    }
    const std::size_t remaining_total = maximum_total - current_total;
    const std::size_t maximum_file_byte_count = std::min(
        config.codec_limits.maximum_encoded_byte_count,
        remaining_total);
    ReadFile read = read_bounded_regular_file(
        directory.get(),
        sequence_file_name(sequence, false),
        maximum_file_byte_count,
        expected_link_count,
        role);
    if (read.bytes.size() > remaining_total) {
      throw std::runtime_error(
          "the durable pair-support transition prefix exceeds its total byte cap");
    }
    return read;
  }

  [[nodiscard]] ExactPairSupportStreamChunk decode_transition(
      const ReadFile& read,
      std::string_view role) {
    durable_status.maximum_simultaneously_decoded_chunk_count = 1U;
    ExactPairSupportStreamDecodeResult decoded =
        decode_exact_pair_support_stream_chunk(
            read.bytes, config.codec_limits);
    if (!decoded.accepted()) {
      throw std::runtime_error(
          std::string{role} + " has invalid encoding");
    }
    if (decoded.chunk->budget != config.fixed_chunk_budget ||
        !transition_makes_progress(*decoded.chunk)) {
      throw std::runtime_error(
          std::string{role} + " violates the fixed replay contract");
    }
    ExactPairSupportStreamChunk chunk = std::move(*decoded.chunk);
    return chunk;
  }

  void validate_head_temporary(
      const HeadRecord& expected_next_head) const {
    const ReadFile temporary = read_bounded_regular_file(
        directory.get(),
        std::string{head_temporary_file_name},
        head_wire_byte_count,
        1,
        "the uncommitted durable pair-support HEAD temporary");
    if (temporary.bytes.size() == head_wire_byte_count) {
      const HeadRecord decoded = decode_head(temporary.bytes);
      if (decoded != expected_next_head) {
        throw std::runtime_error(
            "the uncommitted durable pair-support HEAD temporary is not the exact successor");
      }
    }
  }

  void validate_and_remove_uncommitted_suffix(
      const DirectoryInventory& inventory,
      std::size_t committed_count,
      std::size_t committed_bytes,
      bool initial_head_link_window) {
    const std::size_t orphan_count = checked_add(
        committed_count,
        1U,
        "the durable pair-support orphan count overflows size_t");
    const bool orphan_present =
        inventory.final_file_count == orphan_count;
    const bool any_uncommitted_entry =
        orphan_present || inventory.temporary_sequence.has_value() ||
        (inventory.head_temporary_present && !initial_head_link_window);
    if (any_uncommitted_entry &&
        verifier.status().terminal_checkpoint_reached) {
      throw std::runtime_error(
          "a terminal durable pair-support HEAD cannot have an uncommitted suffix");
    }
    if (any_uncommitted_entry &&
        committed_count >= config.maximum_committed_transition_count) {
      throw std::runtime_error(
          "a capped durable pair-support HEAD cannot have an uncommitted suffix");
    }

    std::optional<struct stat> transition_temporary_metadata;
    if (inventory.temporary_sequence.has_value()) {
      transition_temporary_metadata = named_metadata(
          directory.get(),
          sequence_file_name(*inventory.temporary_sequence, true),
          "the uncommitted durable pair-support transition temporary");
      const nlink_t expected_links = orphan_present ? 2 : 1;
      require_regular_file_with_links(
          *transition_temporary_metadata,
          expected_links,
          "the uncommitted durable pair-support transition temporary");
      if (transition_temporary_metadata->st_size < 0 ||
          static_cast<std::uintmax_t>(
              transition_temporary_metadata->st_size) >
              config.codec_limits.maximum_encoded_byte_count) {
        throw std::runtime_error(
            "the uncommitted durable pair-support transition temporary exceeds its cap");
      }
    }

    std::optional<ExactPairSupportStreamChunk> orphan_chunk;
    std::size_t orphan_byte_count = 0U;
    if (orphan_present) {
      const nlink_t expected_links =
          inventory.temporary_sequence.has_value() ? 2 : 1;
      const std::uint64_t sequence = checked_u64(
          committed_count,
          "the durable pair-support orphan sequence does not fit uint64");
      const ReadFile read = read_transition(
          sequence,
          committed_bytes,
          config.maximum_total_encoded_byte_count,
          expected_links,
          "the uncommitted durable pair-support transition orphan");
      if (transition_temporary_metadata.has_value() &&
          !same_inode(read.metadata, *transition_temporary_metadata)) {
        throw std::runtime_error(
            "the transition final and temporary are not the same link-window inode");
      }
      orphan_byte_count = read.bytes.size();
      orphan_chunk = decode_transition(
          read,
          "the uncommitted durable pair-support transition orphan");
      auto prepared = verifier.prepare_next(
          config.fixed_chunk_budget, *orphan_chunk);
      if (!prepared.prepared()) {
        throw std::runtime_error(
            "the uncommitted durable pair-support transition orphan fails anchored replay");
      }
      const std::uint64_t next_sequence = checked_u64(
          orphan_count,
          "the prospective durable pair-support count does not fit uint64");
      if (prepared.trusted_next_checkpoint().next_chunk_sequence !=
          next_sequence) {
        throw std::runtime_error(
            "the certified transition orphan does not advance HEAD by one sequence");
      }

      const std::size_t next_total = checked_add(
          committed_bytes,
          orphan_byte_count,
          "the prospective durable pair-support byte count overflows size_t");
      const HeadRecord expected_next_head{
          run_contract_digest,
          next_sequence,
          checked_u64(
              next_total,
              "the prospective durable pair-support byte count does not fit uint64"),
          prepared.trusted_next_checkpoint().checkpoint_digest};
      if (inventory.head_temporary_present) {
        validate_head_temporary(expected_next_head);
      }
    }

    bool mutated = false;
    if (inventory.head_temporary_present) {
      if (::unlinkat(
              directory.get(),
              std::string{head_temporary_file_name}.c_str(),
              0) != 0) {
        throw_last_system_error(
            "cannot remove an uncommitted durable pair-support HEAD temporary");
      }
      durable_status.removed_uncommitted_temporary_file_count = checked_add(
          durable_status.removed_uncommitted_temporary_file_count,
          1U,
          "the durable temporary-removal count overflows size_t");
      mutated = true;
    }
    if (inventory.temporary_sequence.has_value()) {
      const std::string temporary_name = sequence_file_name(
          *inventory.temporary_sequence, true);
      if (::unlinkat(directory.get(), temporary_name.c_str(), 0) != 0) {
        throw_last_system_error(
            "cannot remove an uncommitted durable pair-support transition temporary");
      }
      durable_status.removed_uncommitted_temporary_file_count = checked_add(
          durable_status.removed_uncommitted_temporary_file_count,
          1U,
          "the durable temporary-removal count overflows size_t");
      mutated = true;
    }
    if (orphan_present) {
      const std::string final_name = sequence_file_name(
          checked_u64(
              committed_count,
              "the durable orphan sequence does not fit uint64"),
          false);
      if (::unlinkat(directory.get(), final_name.c_str(), 0) != 0) {
        throw_last_system_error(
            "cannot remove an uncommitted durable pair-support transition orphan");
      }
      durable_status.removed_uncommitted_final_file_count = checked_add(
          durable_status.removed_uncommitted_final_file_count,
          1U,
          "the durable orphan-removal count overflows size_t");
      mutated = true;
    }
    if (mutated && ::fsync(directory.get()) != 0) {
      throw_last_system_error(
          "cannot synchronize durable pair-support recovery cleanup");
    }
  }

  void recover_existing() {
    const std::size_t inventory_cap = checked_add(
        config.maximum_committed_transition_count,
        1U,
        "the durable pair-support inventory cap overflows size_t");
    const DirectoryInventory inventory = inventory_directory(
        directory.get(), inventory_cap);
    if (!inventory.head_present) {
      throw std::runtime_error(
          "open_existing requires an authoritative durable pair-support HEAD");
    }
    bool initial_head_link_window = false;
    if (inventory.head_temporary_present &&
        inventory.final_file_count == 0U &&
        !inventory.temporary_sequence.has_value()) {
      const struct stat named_head = named_metadata(
          directory.get(),
          std::string{head_file_name},
          "the initial durable pair-support HEAD link window");
      const struct stat named_temporary = named_metadata(
          directory.get(),
          std::string{head_temporary_file_name},
          "the initial durable pair-support HEAD temporary link window");
      require_regular_file_with_links(
          named_head,
          2,
          "the initial durable pair-support HEAD link window");
      require_regular_file_with_links(
          named_temporary,
          2,
          "the initial durable pair-support HEAD temporary link window");
      if (!same_inode(named_head, named_temporary)) {
        throw std::runtime_error(
            "the initial durable pair-support HEAD link window spans different inodes");
      }
      initial_head_link_window = true;
    }
    authoritative_head = read_head_file(
        directory.get(), initial_head_link_window ? 2 : 1);
    validate_head_contract_and_caps(authoritative_head);
    const std::size_t head_count = checked_size(
        authoritative_head.committed_transition_count,
        "the durable pair-support HEAD count does not fit size_t");
    const std::size_t head_byte_count = checked_size(
        authoritative_head.total_encoded_byte_count,
        "the durable pair-support HEAD byte count does not fit size_t");
    if (expected_prefix_anchor.has_value() &&
        expected_prefix_anchor->committed_transition_count >
            authoritative_head.committed_transition_count) {
      throw std::runtime_error(
          "the local durable pair-support HEAD predates the external prefix anchor");
    }
    validate_inventory_against_head(
        inventory, head_count, initial_head_link_window);
    verify_external_anchor_at_current_prefix();

    for (std::size_t sequence = 0U; sequence < head_count; ++sequence) {
      const ReadFile read = read_transition(
          checked_u64(
              sequence,
              "a recovered durable pair-support sequence does not fit uint64"),
          total_encoded_byte_count,
          head_byte_count,
          1,
          "a committed durable pair-support transition");
      ExactPairSupportStreamChunk chunk = decode_transition(
          read, "a committed durable pair-support transition");
      auto prepared = verifier.prepare_next(
          config.fixed_chunk_budget, chunk);
      if (!prepared.prepared() ||
          !verifier.commit_prepared(std::move(prepared))) {
        throw std::runtime_error(
            "a committed durable pair-support transition fails anchored replay");
      }
      total_encoded_byte_count = checked_add(
          total_encoded_byte_count,
          read.bytes.size(),
          "the durable pair-support byte count overflows size_t");
      committed_transition_count = checked_add(
          committed_transition_count,
          1U,
          "the durable pair-support transition count overflows size_t");
      durable_status.recovered_transition_count =
          committed_transition_count;
      verify_external_anchor_at_current_prefix();
    }

    if (committed_transition_count != head_count ||
        total_encoded_byte_count != head_byte_count ||
        verifier.trusted_checkpoint().next_chunk_sequence !=
            authoritative_head.committed_transition_count ||
        verifier.trusted_checkpoint().checkpoint_digest !=
            authoritative_head.committed_checkpoint_digest) {
      throw std::runtime_error(
          "anchored replay does not recover the authoritative durable HEAD");
    }
    if (expected_prefix_anchor.has_value() &&
        !durable_status.external_prefix_anchor_verified) {
      throw std::runtime_error(
          "the external durable prefix anchor was not encountered during replay");
    }

    validate_and_remove_uncommitted_suffix(
        inventory,
        head_count,
        head_byte_count,
        initial_head_link_window);
    durable_status.authoritative_head_certified = true;
    refresh_status();
  }

  [[nodiscard]] bool cleanup_failed_publication(
      const std::string& transition_temporary_name,
      const std::string& transition_final_name,
      bool transition_temporary_present,
      bool transition_final_present,
      bool head_temporary_present,
      int& error_number) noexcept {
    bool success = true;
    bool mutation_requested = false;
    if (head_temporary_present) {
      mutation_requested = true;
      success = unlink_if_present(
                    directory.get(),
                    std::string{head_temporary_file_name},
                    error_number) &&
                success;
    }
    if (transition_temporary_present) {
      mutation_requested = true;
      success = unlink_if_present(
                    directory.get(),
                    transition_temporary_name,
                    error_number) &&
                success;
    }
    if (transition_final_present) {
      mutation_requested = true;
      success = unlink_if_present(
                    directory.get(),
                    transition_final_name,
                    error_number) &&
                success;
    }
    if (mutation_requested && ::fsync(directory.get()) != 0) {
      error_number = errno;
      success = false;
    }
    return success;
  }

  [[nodiscard]] ExactPairSupportDurablePublishResult publish(
      const ExactPairSupportStreamChunk& observed,
      ExactPairSupportDurablePublishOptions options) {
    ExactPairSupportDurablePublishResult result;
    result.committed_transition_count = committed_transition_count;
    result.total_encoded_byte_count = total_encoded_byte_count;
    result.current_prefix_anchor = current_anchor();
    if (io_failed_closed || verifier.status().failed_closed) {
      result.decision = ExactPairSupportDurablePublishDecision::
          transition_rejected_failed_closed;
      return result;
    }
    if (verifier.status().terminal_checkpoint_reached) {
      result.decision = ExactPairSupportDurablePublishDecision::
          terminal_checkpoint_already_reached;
      return result;
    }
    if (observed.budget != config.fixed_chunk_budget ||
        !transition_makes_progress(observed)) {
      io_failed_closed = true;
      refresh_status();
      result.decision = ExactPairSupportDurablePublishDecision::
          transition_rejected_failed_closed;
      return result;
    }

    if (committed_transition_count >=
            config.maximum_committed_transition_count ||
        total_encoded_byte_count >
            config.maximum_total_encoded_byte_count) {
      result.decision = ExactPairSupportDurablePublishDecision::
          codec_limit_rejected;
      return result;
    }
    const std::size_t remaining_total_encoded_byte_count =
        config.maximum_total_encoded_byte_count -
        total_encoded_byte_count;
    ExactPairSupportStreamCodecLimits effective_codec_limits =
        config.codec_limits;
    effective_codec_limits.maximum_encoded_byte_count = std::min(
        effective_codec_limits.maximum_encoded_byte_count,
        remaining_total_encoded_byte_count);

    std::vector<std::uint8_t> encoded;
    try {
      encoded = encode_exact_pair_support_stream_chunk(
          observed, effective_codec_limits);
    } catch (const std::invalid_argument&) {
      result.decision = ExactPairSupportDurablePublishDecision::
          codec_limit_rejected;
      return result;
    } catch (const std::length_error&) {
      result.decision = ExactPairSupportDurablePublishDecision::
          codec_limit_rejected;
      return result;
    } catch (const std::overflow_error&) {
      result.decision = ExactPairSupportDurablePublishDecision::
          codec_limit_rejected;
      return result;
    }
    if (encoded.size() > remaining_total_encoded_byte_count) {
      result.decision = ExactPairSupportDurablePublishDecision::
          codec_limit_rejected;
      return result;
    }
    const std::size_t next_committed_transition_count = checked_add(
        committed_transition_count,
        1U,
        "the durable pair-support committed count overflows size_t");
    const std::size_t next_total_encoded_byte_count = checked_add(
        total_encoded_byte_count,
        encoded.size(),
        "the durable pair-support encoded byte count overflows size_t");

    auto prepared = verifier.prepare_next(
        config.fixed_chunk_budget, observed);
    result.transition_verification = prepared.verification();
    if (!prepared.prepared()) {
      refresh_status();
      result.decision = ExactPairSupportDurablePublishDecision::
          transition_rejected_failed_closed;
      return result;
    }

    const std::uint64_t sequence =
        verifier.trusted_checkpoint().next_chunk_sequence;
    const std::uint64_t next_sequence = checked_u64(
        next_committed_transition_count,
        "the durable pair-support successor count does not fit uint64");
    if (sequence != authoritative_head.committed_transition_count ||
        prepared.trusted_next_checkpoint().next_chunk_sequence !=
            next_sequence) {
      io_failed_closed = true;
      refresh_status();
      result.decision = ExactPairSupportDurablePublishDecision::
          transition_rejected_failed_closed;
      return result;
    }
    const std::string final_name = sequence_file_name(sequence, false);
    const std::string temporary_name = sequence_file_name(sequence, true);
    const HeadRecord next_head{
        run_contract_digest,
        next_sequence,
        checked_u64(
            next_total_encoded_byte_count,
            "the durable pair-support successor byte count does not fit uint64"),
        prepared.trusted_next_checkpoint().checkpoint_digest};
    const auto encoded_head = encode_head(next_head);

    UniqueFileDescriptor transition_temporary;
    UniqueFileDescriptor head_temporary;
    bool transition_temporary_present = false;
    bool transition_final_present = false;
    bool head_temporary_present = false;
    bool head_replaced = false;
    bool integrity_failure = false;
    try {
      transition_temporary = create_temporary_file(
          directory.get(),
          temporary_name,
          "a durable pair-support transition temporary");
      transition_temporary_present = true;
      write_all(transition_temporary.get(), encoded);
      notify(
          options,
          ExactPairSupportDurablePublishStage::
              transition_temporary_file_written);
      if (::fdatasync(transition_temporary.get()) != 0) {
        throw_last_system_error(
            "cannot synchronize a durable pair-support transition temporary");
      }
      verify_written_bytes(
          transition_temporary.get(),
          encoded,
          "a durable pair-support transition temporary");
      notify(
          options,
          ExactPairSupportDurablePublishStage::
              transition_temporary_file_synchronized);

      if (::linkat(
              directory.get(),
              temporary_name.c_str(),
              directory.get(),
              final_name.c_str(),
              0) != 0) {
        throw_last_system_error(
            "cannot publish a no-replace durable pair-support transition link");
      }
      transition_final_present = true;
      notify(
          options,
          ExactPairSupportDurablePublishStage::
              transition_final_link_created);
      const struct stat linked_temporary = named_metadata(
          directory.get(),
          temporary_name,
          "the linked durable pair-support transition temporary");
      const struct stat linked_final = named_metadata(
          directory.get(),
          final_name,
          "the linked durable pair-support transition final");
      require_regular_file_with_links(
          linked_temporary,
          2,
          "the linked durable pair-support transition temporary");
      require_regular_file_with_links(
          linked_final,
          2,
          "the linked durable pair-support transition final");
      if (!same_inode(linked_temporary, linked_final)) {
        throw std::runtime_error(
            "the no-replace transition publication did not preserve one inode");
      }
      if (::unlinkat(directory.get(), temporary_name.c_str(), 0) != 0) {
        throw_last_system_error(
            "cannot remove a published durable pair-support transition temporary link");
      }
      transition_temporary_present = false;
      notify(
          options,
          ExactPairSupportDurablePublishStage::
              transition_temporary_link_removed);
      const struct stat final_metadata = named_metadata(
          directory.get(),
          final_name,
          "the durable pair-support transition final");
      require_regular_file_with_links(
          final_metadata,
          1,
          "the durable pair-support transition final");
      if (::fsync(directory.get()) != 0) {
        throw_last_system_error(
            "cannot synchronize a published durable pair-support transition");
      }
      notify(
          options,
          ExactPairSupportDurablePublishStage::
              transition_directory_synchronized);

      head_temporary = create_temporary_file(
          directory.get(),
          std::string{head_temporary_file_name},
          "the durable pair-support HEAD temporary");
      head_temporary_present = true;
      write_all(head_temporary.get(), encoded_head);
      notify(
          options,
          ExactPairSupportDurablePublishStage::
              head_temporary_file_written);
      if (::fdatasync(head_temporary.get()) != 0) {
        throw_last_system_error(
            "cannot synchronize the durable pair-support HEAD temporary");
      }
      verify_written_bytes(
          head_temporary.get(),
          encoded_head,
          "the durable pair-support HEAD temporary");
      notify(
          options,
          ExactPairSupportDurablePublishStage::
              head_temporary_file_synchronized);

      integrity_failure = true;
      if (read_head_file(directory.get()) != authoritative_head) {
        throw std::runtime_error(
            "the authoritative durable pair-support HEAD changed before replacement");
      }
      integrity_failure = false;
      if (::renameat(
              directory.get(),
              std::string{head_temporary_file_name}.c_str(),
              directory.get(),
              std::string{head_file_name}.c_str()) != 0) {
        throw_last_system_error(
            "cannot replace the authoritative durable pair-support HEAD");
      }
      head_temporary_present = false;
      head_replaced = true;
      notify(
          options,
          ExactPairSupportDurablePublishStage::head_replaced);
      if (::fsync(directory.get()) != 0) {
        throw_last_system_error(
            "cannot synchronize the authoritative durable pair-support HEAD");
      }
      notify(
          options,
          ExactPairSupportDurablePublishStage::
              head_directory_synchronized);
    } catch (const std::system_error& error) {
      result.system_error_number = error.code().value();
      transition_temporary = UniqueFileDescriptor{};
      head_temporary = UniqueFileDescriptor{};
      const bool target_collision =
          result.system_error_number == EEXIST;
      if (!head_replaced && !integrity_failure && cleanup_failed_publication(
                                temporary_name,
                                final_name,
                                transition_temporary_present,
                                transition_final_present,
                                head_temporary_present,
                                result.system_error_number) &&
          !target_collision) {
        result.decision = ExactPairSupportDurablePublishDecision::
            retryable_io_failure;
        return result;
      }
      io_failed_closed = true;
      if (head_replaced || integrity_failure) {
        durable_status.authoritative_head_certified = false;
      }
      refresh_status();
      result.decision = ExactPairSupportDurablePublishDecision::
          indeterminate_io_failure_reopen_required;
      return result;
    } catch (const std::exception&) {
      transition_temporary = UniqueFileDescriptor{};
      head_temporary = UniqueFileDescriptor{};
      static_cast<void>(cleanup_failed_publication(
          temporary_name,
          final_name,
          !head_replaced && !integrity_failure &&
              transition_temporary_present,
          !head_replaced && !integrity_failure &&
              transition_final_present,
          !head_replaced && !integrity_failure && head_temporary_present,
          result.system_error_number));
      io_failed_closed = true;
      if (head_replaced || integrity_failure) {
        durable_status.authoritative_head_certified = false;
      }
      refresh_status();
      result.decision = ExactPairSupportDurablePublishDecision::
          indeterminate_io_failure_reopen_required;
      return result;
    }

    if (integrity_failure ||
        !verifier.commit_prepared(std::move(prepared))) {
      io_failed_closed = true;
      durable_status.authoritative_head_certified = false;
      refresh_status();
      result.decision = ExactPairSupportDurablePublishDecision::
          indeterminate_io_failure_reopen_required;
      return result;
    }
    authoritative_head = next_head;
    committed_transition_count = next_committed_transition_count;
    total_encoded_byte_count = next_total_encoded_byte_count;
    refresh_status();
    result.decision =
        ExactPairSupportDurablePublishDecision::durably_published;
    result.committed_transition_count = committed_transition_count;
    result.total_encoded_byte_count = total_encoded_byte_count;
    result.current_prefix_anchor = current_anchor();
    result.trusted_checkpoint_advanced = true;
    return result;
  }

  ExactPairSupportDurableConfig config;
  ExactPairSupportIncrementalVerifier verifier;
  std::optional<ExactPairSupportDurableExternalPrefixAnchor>
      expected_prefix_anchor;
  contract::CanonicalId run_contract_digest{};
  HeadRecord authoritative_head{};
  UniqueFileDescriptor directory;
  UniqueFileDescriptor lock;
  ExactPairSupportDurableStatus durable_status{};
  std::size_t committed_transition_count{};
  std::size_t total_encoded_byte_count{};
  bool io_failed_closed{false};
};

ExactPairSupportDurableSink::ExactPairSupportDurableSink(
    OpenMode mode,
    const std::filesystem::path& dedicated_directory,
    const ExactPairSupportAuthorityContext& authority,
    ExactPairSupportDurableConfig config,
    std::optional<ExactPairSupportDurableExternalPrefixAnchor>
        expected_prefix_anchor)
    : impl_(std::make_unique<Impl>(
          mode,
          dedicated_directory,
          authority,
          std::move(config),
          std::move(expected_prefix_anchor))) {}

ExactPairSupportDurableSink ExactPairSupportDurableSink::create_new(
    const std::filesystem::path& dedicated_directory,
    const ExactPairSupportAuthorityContext& authority,
    ExactPairSupportDurableConfig config) {
  return ExactPairSupportDurableSink{
      OpenMode::create_new,
      dedicated_directory,
      authority,
      std::move(config),
      std::nullopt};
}

ExactPairSupportDurableSink ExactPairSupportDurableSink::open_existing(
    const std::filesystem::path& dedicated_directory,
    const ExactPairSupportAuthorityContext& authority,
    ExactPairSupportDurableConfig config,
    std::optional<ExactPairSupportDurableExternalPrefixAnchor>
        expected_prefix_anchor) {
  return ExactPairSupportDurableSink{
      OpenMode::open_existing,
      dedicated_directory,
      authority,
      std::move(config),
      std::move(expected_prefix_anchor)};
}

ExactPairSupportDurableSink::~ExactPairSupportDurableSink() = default;

ExactPairSupportDurablePublishResult
ExactPairSupportDurableSink::publish_next(
    const ExactPairSupportStreamChunk& observed,
    ExactPairSupportDurablePublishOptions options) {
  return impl_->publish(observed, options);
}

const ExactPairSupportCheckpoint&
ExactPairSupportDurableSink::trusted_checkpoint() const noexcept {
  return impl_->verifier.trusted_checkpoint();
}

const ExactPairSupportDurableStatus& ExactPairSupportDurableSink::status()
    const noexcept {
  return impl_->durable_status;
}

}  // namespace morsehgp3d::hierarchy
