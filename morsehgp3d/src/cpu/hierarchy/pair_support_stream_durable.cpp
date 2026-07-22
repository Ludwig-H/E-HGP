#include "morsehgp3d/hierarchy/pair_support_stream_durable.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
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
constexpr std::string_view final_prefix = "pair-support-";
constexpr std::string_view final_suffix = ".p9c";
constexpr std::string_view temporary_prefix = ".pair-support-";
constexpr std::string_view temporary_suffix = ".tmp";
constexpr std::size_t sequence_digit_count = 20U;

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
  [[nodiscard]] int release() noexcept {
    return std::exchange(descriptor_, -1);
  }

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
         name.starts_with(temporary_prefix) || name == lock_file_name;
}

struct DirectoryInventory {
  std::size_t final_file_count{};
  std::optional<std::uint64_t> maximum_final_sequence;
  std::optional<std::uint64_t> temporary_sequence;
};

[[nodiscard]] DirectoryInventory inventory_directory(
    int directory_fd,
    std::size_t maximum_final_file_count) {
  const int duplicate = ::dup(directory_fd);
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
      if (const auto sequence = parse_sequence_file_name(name, false)) {
        inventory.final_file_count = checked_add(
            inventory.final_file_count,
            1U,
            "the durable pair-support file count overflows size_t");
        if (inventory.final_file_count > maximum_final_file_count) {
          throw std::runtime_error(
              "the durable pair-support transition count exceeds its cap");
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
              "the durable pair-support directory contains multiple temporary files");
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

void require_regular_single_link_file(int descriptor, std::string_view role) {
  struct stat metadata {};
  if (::fstat(descriptor, &metadata) != 0) {
    throw_last_system_error(std::string{"cannot inspect "} + std::string{role});
  }
  if (!S_ISREG(metadata.st_mode) || metadata.st_nlink != 1) {
    throw std::runtime_error(
        std::string{role} + " must be a singly linked regular file");
  }
}

[[nodiscard]] std::vector<std::uint8_t> read_transition_file(
    int directory_fd,
    const std::string& name,
    std::size_t maximum_file_byte_count,
    std::size_t current_total_byte_count,
    std::size_t maximum_total_byte_count) {
  UniqueFileDescriptor descriptor{
      ::openat(
          directory_fd,
          name.c_str(),
          O_RDONLY | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK)};
  if (!descriptor.valid()) {
    throw_last_system_error(
        "cannot open a committed durable pair-support transition");
  }
  require_regular_single_link_file(
      descriptor.get(), "a committed durable pair-support transition");
  struct stat metadata {};
  if (::fstat(descriptor.get(), &metadata) != 0) {
    throw_last_system_error(
        "cannot size a committed durable pair-support transition");
  }
  if (metadata.st_size < 0) {
    throw std::runtime_error(
        "a committed durable pair-support transition has negative size");
  }
  const std::uintmax_t unsigned_size =
      static_cast<std::uintmax_t>(metadata.st_size);
  if (unsigned_size > maximum_file_byte_count ||
      unsigned_size > std::numeric_limits<std::size_t>::max()) {
    throw std::runtime_error(
        "a committed durable pair-support transition exceeds its byte cap");
  }
  const std::size_t size = static_cast<std::size_t>(unsigned_size);
  if (size > maximum_total_byte_count - current_total_byte_count) {
    throw std::runtime_error(
        "the committed durable pair-support prefix exceeds its total byte cap");
  }

  std::vector<std::uint8_t> bytes(size);
  std::size_t offset = 0U;
  while (offset < bytes.size()) {
    const ssize_t count = ::read(
        descriptor.get(),
        bytes.data() + offset,
        bytes.size() - offset);
    if (count < 0) {
      if (errno == EINTR) {
        continue;
      }
      throw_last_system_error(
          "cannot read a committed durable pair-support transition");
    }
    if (count == 0) {
      throw std::runtime_error(
          "a committed durable pair-support transition was truncated during reading");
    }
    offset = checked_add(
        offset,
        static_cast<std::size_t>(count),
        "a durable pair-support read offset overflows size_t");
  }
  return bytes;
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
    std::span<const std::uint8_t> expected) {
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

}  // namespace

struct ExactPairSupportDurableSink::Impl {
  Impl(
      const std::filesystem::path& dedicated_directory,
      const ExactPairSupportAuthorityContext& authority,
      ExactPairSupportDurableConfig supplied_config)
      : config(std::move(supplied_config)), verifier(authority) {
    if (config.maximum_committed_transition_count == 0U ||
        config.maximum_total_encoded_byte_count == 0U ||
        config.fixed_chunk_budget.maximum_work_unit_count == 0U ||
        config_has_unbounded_limit(config) ||
        config.codec_limits.maximum_encoded_byte_count >
            static_cast<std::size_t>(
                std::numeric_limits<off_t>::max())) {
      throw std::invalid_argument(
          "durable pair-support caps must be positive, finite and addressable");
    }

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
            O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW,
            S_IRUSR | S_IWUSR)};
    if (!lock.valid()) {
      throw_last_system_error(
          "cannot open the durable pair-support writer lock");
    }
    require_regular_single_link_file(
        lock.get(), "the durable pair-support writer lock");
    if (::flock(lock.get(), LOCK_EX | LOCK_NB) != 0) {
      throw_last_system_error(
          "cannot acquire the durable pair-support writer lock");
    }
    durable_status.writer_lock_acquired = true;
    recover();
  }

  void refresh_status() noexcept {
    const ExactPairSupportIncrementalVerifierStatus& verifier_status =
        verifier.status();
    durable_status.committed_transition_count = committed_transition_count;
    durable_status.total_encoded_byte_count = total_encoded_byte_count;
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

  void recover() {
    const DirectoryInventory inventory = inventory_directory(
        directory.get(), config.maximum_committed_transition_count);
    if (inventory.maximum_final_sequence.has_value()) {
      if (*inventory.maximum_final_sequence ==
          std::numeric_limits<std::uint64_t>::max()) {
        throw std::runtime_error(
            "the durable pair-support transition sequence is exhausted");
      }
      const std::uint64_t expected_count =
          *inventory.maximum_final_sequence + 1U;
      if (expected_count > std::numeric_limits<std::size_t>::max() ||
          static_cast<std::size_t>(expected_count) !=
              inventory.final_file_count) {
        throw std::runtime_error(
            "the durable pair-support transition files are not contiguous from zero");
      }
    } else if (inventory.final_file_count != 0U) {
      throw std::logic_error(
          "a durable pair-support directory inventory is inconsistent");
    }
    if (inventory.final_file_count >
        config.maximum_committed_transition_count) {
      throw std::runtime_error(
          "the durable pair-support transition count exceeds its cap");
    }

    for (std::size_t sequence = 0U;
         sequence < inventory.final_file_count;
         ++sequence) {
      const std::string name = sequence_file_name(
          static_cast<std::uint64_t>(sequence), false);
      std::vector<std::uint8_t> bytes = read_transition_file(
          directory.get(),
          name,
          config.codec_limits.maximum_encoded_byte_count,
          total_encoded_byte_count,
          config.maximum_total_encoded_byte_count);
      durable_status.maximum_simultaneously_decoded_chunk_count = 1U;
      const ExactPairSupportStreamDecodeResult decoded =
          decode_exact_pair_support_stream_chunk(bytes, config.codec_limits);
      if (!decoded.accepted()) {
        throw std::runtime_error(
            "a committed durable pair-support transition has invalid encoding");
      }
      if (decoded.chunk->budget != config.fixed_chunk_budget ||
          !transition_makes_progress(*decoded.chunk)) {
        throw std::runtime_error(
            "a committed durable pair-support transition violates the fixed replay contract");
      }
      auto prepared = verifier.prepare_next(
          config.fixed_chunk_budget, *decoded.chunk);
      if (!prepared.prepared() ||
          !verifier.commit_prepared(std::move(prepared))) {
        throw std::runtime_error(
            "a committed durable pair-support transition fails anchored replay");
      }
      total_encoded_byte_count = checked_add(
          total_encoded_byte_count,
          bytes.size(),
          "the durable pair-support byte count overflows size_t");
      committed_transition_count = checked_add(
          committed_transition_count,
          1U,
          "the durable pair-support transition count overflows size_t");
      durable_status.recovered_transition_count = committed_transition_count;
    }

    const std::uint64_t next_sequence =
        static_cast<std::uint64_t>(inventory.final_file_count);
    if (inventory.temporary_sequence.has_value()) {
      if (*inventory.temporary_sequence != next_sequence) {
        throw std::runtime_error(
            "a durable pair-support temporary file does not match the next sequence");
      }
      const std::string temporary_name =
          sequence_file_name(next_sequence, true);
      if (::unlinkat(directory.get(), temporary_name.c_str(), 0) != 0) {
        throw_last_system_error(
            "cannot remove an uncommitted durable pair-support temporary file");
      }
      if (::fsync(directory.get()) != 0) {
        throw_last_system_error(
            "cannot synchronize removal of a durable pair-support temporary file");
      }
      durable_status.removed_uncommitted_temporary_file_count = 1U;
    }
    if (::fsync(directory.get()) != 0) {
      throw_last_system_error(
          "cannot synchronize the recovered durable pair-support directory");
    }
    refresh_status();
  }

  [[nodiscard]] bool clean_temporary_after_failure(
      const std::string& temporary_name,
      int& error_number) noexcept {
    if (::unlinkat(directory.get(), temporary_name.c_str(), 0) != 0 &&
        errno != ENOENT) {
      error_number = errno;
      return false;
    }
    if (::fsync(directory.get()) != 0) {
      error_number = errno;
      return false;
    }
    return true;
  }

  [[nodiscard]] ExactPairSupportDurablePublishResult publish(
      const ExactPairSupportStreamChunk& observed,
      ExactPairSupportDurablePublishOptions options) {
    ExactPairSupportDurablePublishResult result;
    result.committed_transition_count = committed_transition_count;
    result.total_encoded_byte_count = total_encoded_byte_count;
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

    std::vector<std::uint8_t> encoded;
    try {
      encoded = encode_exact_pair_support_stream_chunk(
          observed, config.codec_limits);
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
    if (committed_transition_count >=
            config.maximum_committed_transition_count ||
        encoded.size() >
            config.maximum_total_encoded_byte_count -
                total_encoded_byte_count) {
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
    const std::string final_name = sequence_file_name(sequence, false);
    const std::string temporary_name = sequence_file_name(sequence, true);
    struct stat existing {};
    errno = 0;
    const int final_presence = ::fstatat(
        directory.get(),
        final_name.c_str(),
        &existing,
        AT_SYMLINK_NOFOLLOW);
    const int final_error = errno;
    errno = 0;
    const int temporary_presence = ::fstatat(
        directory.get(),
        temporary_name.c_str(),
        &existing,
        AT_SYMLINK_NOFOLLOW);
    const int temporary_error = errno;
    if (final_presence == 0 || temporary_presence == 0 ||
        (final_presence != 0 && final_error != ENOENT) ||
        (temporary_presence != 0 && temporary_error != ENOENT)) {
      result.system_error_number =
          final_presence != 0 && final_error != ENOENT
              ? final_error
              : (temporary_presence != 0 && temporary_error != ENOENT
                     ? temporary_error
                     : EEXIST);
      io_failed_closed = true;
      refresh_status();
      result.decision = ExactPairSupportDurablePublishDecision::
          indeterminate_io_failure_reopen_required;
      return result;
    }
    UniqueFileDescriptor temporary;
    bool renamed = false;
    try {
      temporary = UniqueFileDescriptor{
          ::openat(
              directory.get(),
              temporary_name.c_str(),
              O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
              S_IRUSR | S_IWUSR)};
      if (!temporary.valid()) {
        throw_last_system_error(
            "cannot create a durable pair-support temporary file");
      }
      require_regular_single_link_file(
          temporary.get(), "a durable pair-support temporary file");
      write_all(temporary.get(), encoded);
      notify(
          options,
          ExactPairSupportDurablePublishStage::temporary_file_written);
      if (::fdatasync(temporary.get()) != 0) {
        throw_last_system_error(
            "cannot synchronize a durable pair-support temporary file");
      }
      verify_written_bytes(temporary.get(), encoded);
      notify(
          options,
          ExactPairSupportDurablePublishStage::
              temporary_file_synchronized);
      if (::renameat(
              directory.get(),
              temporary_name.c_str(),
              directory.get(),
              final_name.c_str()) != 0) {
        throw_last_system_error(
            "cannot publish a durable pair-support transition");
      }
      renamed = true;
      notify(
          options,
          ExactPairSupportDurablePublishStage::transition_renamed);
      if (::fsync(directory.get()) != 0) {
        throw_last_system_error(
            "cannot synchronize a published durable pair-support transition");
      }
      notify(
          options,
          ExactPairSupportDurablePublishStage::directory_synchronized);
    } catch (const std::system_error& error) {
      result.system_error_number = error.code().value();
      if (!renamed) {
        temporary = UniqueFileDescriptor{};
        if (clean_temporary_after_failure(
                temporary_name, result.system_error_number)) {
          result.decision = ExactPairSupportDurablePublishDecision::
              retryable_io_failure;
          return result;
        }
      }
      io_failed_closed = true;
      refresh_status();
      result.decision = ExactPairSupportDurablePublishDecision::
          indeterminate_io_failure_reopen_required;
      return result;
    } catch (const std::exception&) {
      if (!renamed) {
        temporary = UniqueFileDescriptor{};
        if (clean_temporary_after_failure(
                temporary_name, result.system_error_number)) {
          result.decision = ExactPairSupportDurablePublishDecision::
              retryable_io_failure;
          return result;
        }
      }
      io_failed_closed = true;
      refresh_status();
      result.decision = ExactPairSupportDurablePublishDecision::
          indeterminate_io_failure_reopen_required;
      return result;
    }

    if (!verifier.commit_prepared(std::move(prepared))) {
      io_failed_closed = true;
      refresh_status();
      result.decision = ExactPairSupportDurablePublishDecision::
          indeterminate_io_failure_reopen_required;
      return result;
    }
    committed_transition_count = next_committed_transition_count;
    total_encoded_byte_count = next_total_encoded_byte_count;
    refresh_status();
    result.decision =
        ExactPairSupportDurablePublishDecision::durably_published;
    result.committed_transition_count = committed_transition_count;
    result.total_encoded_byte_count = total_encoded_byte_count;
    result.trusted_checkpoint_advanced = true;
    return result;
  }

  ExactPairSupportDurableConfig config;
  ExactPairSupportIncrementalVerifier verifier;
  UniqueFileDescriptor directory;
  UniqueFileDescriptor lock;
  ExactPairSupportDurableStatus durable_status{};
  std::size_t committed_transition_count{};
  std::size_t total_encoded_byte_count{};
  bool io_failed_closed{false};
};

ExactPairSupportDurableSink::ExactPairSupportDurableSink(
    const std::filesystem::path& dedicated_directory,
    const ExactPairSupportAuthorityContext& authority,
    ExactPairSupportDurableConfig config)
    : impl_(std::make_unique<Impl>(
          dedicated_directory, authority, std::move(config))) {}

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
