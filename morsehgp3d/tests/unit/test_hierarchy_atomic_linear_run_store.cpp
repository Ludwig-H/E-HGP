#include "morsehgp3d/hierarchy/atomic_linear_run_store.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

using morsehgp3d::contract::CanonicalId;
using morsehgp3d::contract::CanonicalSha256Builder;
using morsehgp3d::hierarchy::AtomicLinearRunChunkProposal;
using morsehgp3d::hierarchy::AtomicLinearRunContract;
using morsehgp3d::hierarchy::AtomicLinearRunExternalAnchor;
using morsehgp3d::hierarchy::AtomicLinearRunPublishDecision;
using morsehgp3d::hierarchy::AtomicLinearRunPublishOptions;
using morsehgp3d::hierarchy::AtomicLinearRunPublishStage;
using morsehgp3d::hierarchy::AtomicLinearRunRecertification;
using morsehgp3d::hierarchy::AtomicLinearRunRecertificationPhase;
using morsehgp3d::hierarchy::AtomicLinearRunRecertifier;
using morsehgp3d::hierarchy::AtomicLinearRunStore;
using morsehgp3d::hierarchy::AtomicLinearRunStoreLimits;
using morsehgp3d::hierarchy::AtomicLinearRunTransition;
using morsehgp3d::hierarchy::
    atomic_linear_run_transition_fixed_wire_byte_count;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

template <typename Function>
void check_throws(Function&& function, const std::string& message) {
  try {
    std::forward<Function>(function)();
  } catch (const std::exception&) {
    return;
  }
  ++failures;
  std::cerr << "FAIL: " << message << " (no exception)\n";
}

class TemporaryWorkspace {
 public:
  TemporaryWorkspace() {
    temporary_parent_ = std::filesystem::temp_directory_path();
    std::string pattern =
        (temporary_parent_ / "morsehgp3d-linear-run-XXXXXX").string();
    std::vector<char> writable(pattern.begin(), pattern.end());
    writable.push_back('\0');
    char* const created = ::mkdtemp(writable.data());
    if (created == nullptr) {
      throw std::system_error(
          errno, std::generic_category(), "mkdtemp failed");
    }
    root_ = std::filesystem::path{created};
  }

  ~TemporaryWorkspace() {
    if (!root_.empty() && root_.parent_path() == temporary_parent_ &&
        root_.filename().string().starts_with(
            "morsehgp3d-linear-run-")) {
      std::error_code ignored;
      static_cast<void>(std::filesystem::remove_all(root_, ignored));
    }
  }

  TemporaryWorkspace(const TemporaryWorkspace&) = delete;
  TemporaryWorkspace& operator=(const TemporaryWorkspace&) = delete;

  [[nodiscard]] std::filesystem::path make_directory(
      std::string_view name) const {
    const std::filesystem::path directory = root_ / name;
    if (!std::filesystem::create_directory(directory)) {
      throw std::runtime_error(
          "cannot create an atomic linear run test directory");
    }
    return directory;
  }

 private:
  std::filesystem::path temporary_parent_;
  std::filesystem::path root_;
};

[[nodiscard]] CanonicalId digest(std::string_view text) {
  CanonicalSha256Builder builder;
  builder.update("MorseHGP3D/atomic-linear-run/test/");
  builder.update(text);
  return builder.finalize();
}

[[nodiscard]] AtomicLinearRunContract contract_fixture() {
  return {
      digest("application-contract"),
      digest("initial-checkpoint"),
      digest("initial-output-chain"),
      7U,
      10U};
}

[[nodiscard]] AtomicLinearRunStoreLimits generous_limits() {
  return {
      8U,
      64U,
      atomic_linear_run_transition_fixed_wire_byte_count + 64U,
      8U *
          (atomic_linear_run_transition_fixed_wire_byte_count + 64U),
      16U};
}

struct RecertificationCounts {
  std::size_t publication{};
  std::size_t recovery{};
  std::size_t uncommitted_cleanup{};
  bool reject_marker{false};
};

[[nodiscard]] AtomicLinearRunRecertifier recertifier(
    RecertificationCounts& counts) {
  return [&counts](
             const AtomicLinearRunTransition& transition,
             AtomicLinearRunRecertificationPhase phase) {
    if (phase ==
        AtomicLinearRunRecertificationPhase::publication) {
      ++counts.publication;
    } else if (
        phase == AtomicLinearRunRecertificationPhase::recovery) {
      ++counts.recovery;
    } else {
      ++counts.uncommitted_cleanup;
    }
    const bool marker_rejected =
        counts.reject_marker && !transition.payload.empty() &&
        transition.payload.front() == 0xffU;
    return AtomicLinearRunRecertification{
        !marker_rejected,
        !marker_rejected,
        !marker_rejected};
  };
}

[[nodiscard]] AtomicLinearRunChunkProposal proposal(
    const AtomicLinearRunStore& store,
    std::uint64_t batch_end_index,
    std::uint8_t marker,
    std::size_t payload_size = 3U) {
  const auto& state = store.trusted_state();
  std::vector<std::uint8_t> payload(payload_size, marker);
  return {
      state.next_chunk_index,
      state.next_batch_index,
      batch_end_index,
      digest(
          "checkpoint-" +
          std::to_string(state.next_sequence + 1U)),
      digest(
          "budget-" +
          std::to_string(state.next_sequence)),
      std::move(payload)};
}

[[nodiscard]] std::uint64_t load_u64_big_endian(
    std::span<const std::uint8_t> bytes,
    std::size_t offset) {
  if (offset > bytes.size() || bytes.size() - offset < 8U) {
    throw std::runtime_error("big-endian test read is out of bounds");
  }
  std::uint64_t value = 0U;
  for (std::size_t index = 0U; index < 8U; ++index) {
    value = (value << 8U) | bytes[offset + index];
  }
  return value;
}

[[nodiscard]] std::vector<std::uint8_t> read_file_bytes(
    const std::filesystem::path& path) {
  const int descriptor = ::open(
      path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
  if (descriptor < 0) {
    throw std::system_error(
        errno, std::generic_category(), "cannot open test file");
  }
  struct stat metadata {};
  if (::fstat(descriptor, &metadata) != 0 ||
      !S_ISREG(metadata.st_mode) || metadata.st_size < 0) {
    const int error_number = errno;
    static_cast<void>(::close(descriptor));
    throw std::system_error(
        error_number,
        std::generic_category(),
        "cannot inspect test file");
  }
  const std::uintmax_t size =
      static_cast<std::uintmax_t>(metadata.st_size);
  if (size > std::numeric_limits<std::size_t>::max()) {
    static_cast<void>(::close(descriptor));
    throw std::runtime_error("test file is not addressable");
  }
  std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
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
      const int error_number = count < 0 ? errno : EIO;
      static_cast<void>(::close(descriptor));
      throw std::system_error(
          error_number,
          std::generic_category(),
          "cannot read test file");
    }
    offset += static_cast<std::size_t>(count);
  }
  if (::close(descriptor) != 0) {
    throw std::system_error(
        errno, std::generic_category(), "cannot close test file");
  }
  return bytes;
}

void corrupt_byte(
    const std::filesystem::path& path,
    std::size_t offset) {
  const int descriptor = ::open(
      path.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW);
  if (descriptor < 0) {
    throw std::system_error(
        errno, std::generic_category(), "cannot open corruption target");
  }
  std::uint8_t byte = 0U;
  if (::pread(
          descriptor,
          &byte,
          1U,
          static_cast<off_t>(offset)) != 1) {
    const int error_number = errno;
    static_cast<void>(::close(descriptor));
    throw std::system_error(
        error_number,
        std::generic_category(),
        "cannot read corruption target");
  }
  byte ^= 0x5aU;
  if (::pwrite(
          descriptor,
          &byte,
          1U,
          static_cast<off_t>(offset)) != 1 ||
      ::fsync(descriptor) != 0) {
    const int error_number = errno;
    static_cast<void>(::close(descriptor));
    throw std::system_error(
        error_number,
        std::generic_category(),
        "cannot corrupt test file");
  }
  if (::close(descriptor) != 0) {
    throw std::system_error(
        errno,
        std::generic_category(),
        "cannot close corruption target");
  }
}

struct StageObserverState {
  std::array<AtomicLinearRunPublishStage, 9U> stages{};
  std::array<std::uint64_t, 9U> observed_next_sequences{};
  std::array<std::size_t, 9U> observed_committed_counts{};
  const AtomicLinearRunStore* store{};
  std::size_t count{};
};

void collect_stage(
    AtomicLinearRunPublishStage stage,
    void* opaque) noexcept {
  auto* const state = static_cast<StageObserverState*>(opaque);
  if (state->count >= state->stages.size() || state->store == nullptr) {
    return;
  }
  state->stages[state->count] = stage;
  state->observed_next_sequences[state->count] =
      state->store->trusted_state().next_sequence;
  state->observed_committed_counts[state->count] =
      state->store->status().committed_transition_count;
  ++state->count;
}

struct UnlinkAfterHeadState {
  std::string final_path;
  bool unlink_succeeded{false};
};

void unlink_after_head_replace(
    AtomicLinearRunPublishStage stage,
    void* opaque) noexcept {
  if (stage != AtomicLinearRunPublishStage::head_replaced) {
    return;
  }
  auto* const state = static_cast<UnlinkAfterHeadState*>(opaque);
  state->unlink_succeeded = ::unlink(state->final_path.c_str()) == 0;
}

struct CorruptHeadTemporaryState {
  std::string temporary_path;
  bool corruption_succeeded{false};
};

void corrupt_head_temporary(
    AtomicLinearRunPublishStage stage,
    void* opaque) noexcept {
  if (stage !=
      AtomicLinearRunPublishStage::head_temporary_file_written) {
    return;
  }
  auto* const state =
      static_cast<CorruptHeadTemporaryState*>(opaque);
  const int descriptor = ::open(
      state->temporary_path.c_str(),
      O_WRONLY | O_CLOEXEC | O_NOFOLLOW);
  if (descriptor < 0) {
    return;
  }
  constexpr std::uint8_t corrupt_magic = 0xffU;
  const bool write_succeeded =
      ::pwrite(descriptor, &corrupt_magic, 1U, 0) == 1;
  const bool close_succeeded = ::close(descriptor) == 0;
  state->corruption_succeeded =
      write_succeeded && close_succeeded;
}

struct UnlinkHeadBeforeRenameState {
  std::string head_path;
  bool unlink_succeeded{false};
};

void unlink_head_before_rename(
    AtomicLinearRunPublishStage stage,
    void* opaque) noexcept {
  if (stage != AtomicLinearRunPublishStage::
                   head_temporary_file_synchronized_and_reread) {
    return;
  }
  auto* const state =
      static_cast<UnlinkHeadBeforeRenameState*>(opaque);
  state->unlink_succeeded = ::unlink(state->head_path.c_str()) == 0;
}

[[nodiscard]] std::filesystem::path first_final(
    const std::filesystem::path& directory) {
  return directory / "linear-run-00000000000000000000.run";
}

void test_two_chunks_reopen_and_wire() {
  TemporaryWorkspace workspace;
  const std::filesystem::path directory =
      workspace.make_directory("success");
  const AtomicLinearRunContract run_contract = contract_fixture();
  const AtomicLinearRunStoreLimits limits = generous_limits();
  AtomicLinearRunExternalAnchor first_anchor;

  {
    RecertificationCounts counts;
    AtomicLinearRunStore store = AtomicLinearRunStore::create_new(
        directory, run_contract, limits, recertifier(counts));
    check(
        store.status().writer_lock_acquired &&
            store.status().authoritative_head_certified,
        "create_new must certify its lock and initial HEAD");
    check(
        !store.status().process_local_ticket_serialized &&
            store.status().retained_transition_history_count == 0U &&
            store.status().global_gamma_cell_count == 0U &&
            store.status().higher_order_delaunay_cell_count == 0U,
        "the store must retain no ticket, history, or global geometry");

    RecertificationCounts locked_counts;
    check_throws(
        [&]() {
          AtomicLinearRunStore locked =
              AtomicLinearRunStore::open_existing(
                  directory,
                  run_contract,
                  limits,
                  recertifier(locked_counts));
          static_cast<void>(locked.status());
        },
        "a second writer must not acquire the live run");

    StageObserverState observer;
    observer.store = &store;
    const auto first = store.publish_next(
        proposal(store, 13U, 0x11U),
        AtomicLinearRunPublishOptions{collect_stage, &observer});
    check(
        first.decision ==
                AtomicLinearRunPublishDecision::durably_published &&
            first.trusted_state_advanced,
        "the first chunk must publish durably");
    const std::array<AtomicLinearRunPublishStage, 9U> expected_stages{
        AtomicLinearRunPublishStage::
            transition_temporary_file_written,
        AtomicLinearRunPublishStage::
            transition_temporary_file_synchronized_and_reread,
        AtomicLinearRunPublishStage::
            transition_immutable_link_created,
        AtomicLinearRunPublishStage::
            transition_temporary_link_removed,
        AtomicLinearRunPublishStage::
            transition_directory_synchronized,
        AtomicLinearRunPublishStage::head_temporary_file_written,
        AtomicLinearRunPublishStage::
            head_temporary_file_synchronized_and_reread,
        AtomicLinearRunPublishStage::head_replaced,
        AtomicLinearRunPublishStage::head_directory_synchronized};
    check(
        observer.count == expected_stages.size() &&
            observer.stages == expected_stages,
        "the observer must see every durability boundary in order");
    check(
        std::all_of(
            observer.observed_next_sequences.begin(),
            observer.observed_next_sequences.end(),
            [](std::uint64_t sequence) { return sequence == 0U; }) &&
            std::all_of(
                observer.observed_committed_counts.begin(),
                observer.observed_committed_counts.end(),
                [](std::size_t count) { return count == 0U; }) &&
            store.trusted_state().next_sequence == 1U &&
            store.status().committed_transition_count == 1U,
        "all nine observer boundaries expose the old in-memory state, "
        "then success exposes the new state");
    first_anchor = first.current_anchor;

    const auto second =
        store.publish_next(proposal(store, 17U, 0x22U));
    check(
        second.decision ==
                AtomicLinearRunPublishDecision::durably_published &&
            second.committed_transition_count == 2U &&
            second.current_anchor.next_chunk_index == 9U &&
            second.current_anchor.next_batch_index == 17U,
        "the second chunk must advance the exact linear cursor");
    check(
        counts.publication == 2U && counts.recovery == 0U,
        "publication must invoke the mandatory recertifier once per chunk");

    const std::vector<std::uint8_t> encoded =
        read_file_bytes(first_final(directory));
    const std::array<std::uint8_t, 16U> expected_magic{
        'M', 'H', 'G', 'P', '3', 'D', '-', 'L',
        'I', 'N', 'E', 'A', 'R', '-', 'V', '1'};
    check(
        encoded.size() ==
            atomic_linear_run_transition_fixed_wire_byte_count + 3U,
        "the v1 wire size must be fixed metadata plus bounded payload");
    check(
        encoded.size() >= expected_magic.size() &&
            std::equal(
                expected_magic.begin(),
                expected_magic.end(),
                encoded.begin()),
        "the transition must carry the new v1 magic");
    check(
        load_u64_big_endian(encoded, 64U) == 0U &&
            load_u64_big_endian(encoded, 72U) == 7U &&
            load_u64_big_endian(encoded, 80U) == 10U &&
            load_u64_big_endian(encoded, 88U) == 13U &&
            load_u64_big_endian(encoded, 224U) == 3U,
        "sequence, chunk, batch, and payload length must be big-endian");
  }

  {
    RecertificationCounts replay_counts;
    AtomicLinearRunStore reopened =
        AtomicLinearRunStore::open_existing(
            directory,
            run_contract,
            limits,
            recertifier(replay_counts),
            first_anchor);
    check(
        replay_counts.publication == 0U &&
            replay_counts.recovery == 2U &&
            reopened.status().recovery_recertification_count == 2U &&
            reopened.status().
                    uncommitted_cleanup_recertification_count ==
                0U &&
            reopened.status().recovered_transition_count == 2U,
        "reopen must recertify and replay both committed transitions");
    check(
        reopened.status().external_anchor_supplied &&
            reopened.status().external_anchor_verified &&
            reopened.status().linear_prefix_replayed,
        "reopen must cross and verify an older external prefix anchor");
    check(
        reopened.trusted_state().next_sequence == 2U &&
            reopened.trusted_state().next_chunk_index == 9U &&
            reopened.trusted_state().next_batch_index == 17U &&
            reopened.status().current_anchor.committed_transition_count ==
                2U,
        "reopen must reconstruct the exact terminal trusted state");
  }
}

void test_corruption_contract_and_anchors() {
  TemporaryWorkspace workspace;
  const AtomicLinearRunContract run_contract = contract_fixture();
  const AtomicLinearRunStoreLimits limits = generous_limits();

  const std::filesystem::path corrupt_directory =
      workspace.make_directory("corrupt");
  {
    RecertificationCounts counts;
    AtomicLinearRunStore store = AtomicLinearRunStore::create_new(
        corrupt_directory,
        run_contract,
        limits,
        recertifier(counts));
    check(
        store.publish_next(proposal(store, 12U, 0x33U)).decision ==
            AtomicLinearRunPublishDecision::durably_published,
        "corruption fixture must seed one committed transition");
  }
  corrupt_byte(first_final(corrupt_directory), 232U);
  RecertificationCounts corrupt_counts;
  check_throws(
      [&]() {
        AtomicLinearRunStore reopened =
            AtomicLinearRunStore::open_existing(
                corrupt_directory,
                run_contract,
                limits,
                recertifier(corrupt_counts));
        static_cast<void>(reopened.status());
      },
      "recovery must reject a transition with a bad SHA-256");

  const std::filesystem::path contract_directory =
      workspace.make_directory("contract");
  {
    RecertificationCounts counts;
    AtomicLinearRunStore store = AtomicLinearRunStore::create_new(
        contract_directory,
        run_contract,
        limits,
        recertifier(counts));
    static_cast<void>(store.status());
  }
  AtomicLinearRunContract wrong_contract = run_contract;
  wrong_contract.application_contract_digest =
      digest("another-application-contract");
  RecertificationCounts contract_counts;
  check_throws(
      [&]() {
        AtomicLinearRunStore reopened =
            AtomicLinearRunStore::open_existing(
                contract_directory,
                wrong_contract,
                limits,
                recertifier(contract_counts));
        static_cast<void>(reopened.status());
      },
      "recovery must reject a mismatched run contract digest");

  AtomicLinearRunStoreLimits different_limits = limits;
  --different_limits.maximum_batch_span;
  RecertificationCounts limit_contract_counts;
  check_throws(
      [&]() {
        AtomicLinearRunStore reopened =
            AtomicLinearRunStore::open_existing(
                contract_directory,
                run_contract,
                different_limits,
                recertifier(limit_contract_counts));
        static_cast<void>(reopened.status());
      },
      "recovery must reject different limits bound into the run contract");

  const std::filesystem::path anchor_directory =
      workspace.make_directory("anchors");
  AtomicLinearRunExternalAnchor committed_anchor;
  {
    RecertificationCounts counts;
    AtomicLinearRunStore store = AtomicLinearRunStore::create_new(
        anchor_directory,
        run_contract,
        limits,
        recertifier(counts));
    const auto published =
        store.publish_next(proposal(store, 12U, 0x44U));
    check(
        published.decision ==
            AtomicLinearRunPublishDecision::durably_published,
        "anchor fixture must seed one committed transition");
    committed_anchor = published.current_anchor;
  }
  AtomicLinearRunExternalAnchor wrong_anchor = committed_anchor;
  wrong_anchor.checkpoint_digest = digest("wrong-anchor-checkpoint");
  RecertificationCounts wrong_anchor_counts;
  check_throws(
      [&]() {
        AtomicLinearRunStore reopened =
            AtomicLinearRunStore::open_existing(
                anchor_directory,
                run_contract,
                limits,
                recertifier(wrong_anchor_counts),
                wrong_anchor);
        static_cast<void>(reopened.status());
      },
      "recovery must reject a disagreeing external anchor");

  AtomicLinearRunExternalAnchor future_anchor = committed_anchor;
  future_anchor.committed_transition_count = 2U;
  RecertificationCounts future_anchor_counts;
  check_throws(
      [&]() {
        AtomicLinearRunStore reopened =
            AtomicLinearRunStore::open_existing(
                anchor_directory,
                run_contract,
                limits,
                recertifier(future_anchor_counts),
                future_anchor);
        static_cast<void>(reopened.status());
      },
      "recovery must reject a directory older than its external anchor");
}

void test_limits_shapes_and_recertification() {
  TemporaryWorkspace workspace;
  const std::filesystem::path directory =
      workspace.make_directory("limits");
  const AtomicLinearRunContract run_contract = contract_fixture();
  AtomicLinearRunStoreLimits limits{
      3U,
      3U,
      atomic_linear_run_transition_fixed_wire_byte_count + 3U,
      atomic_linear_run_transition_fixed_wire_byte_count + 3U,
      4U};
  RecertificationCounts counts;
  counts.reject_marker = true;
  AtomicLinearRunStore store = AtomicLinearRunStore::create_new(
      directory, run_contract, limits, recertifier(counts));

  check(
      store.publish_next(proposal(store, 12U, 0x55U, 4U)).decision ==
          AtomicLinearRunPublishDecision::store_limit_rejected,
      "an oversized payload must be rejected before recertification");

  AtomicLinearRunChunkProposal wrong_chunk =
      proposal(store, 12U, 0x66U);
  ++wrong_chunk.chunk_index;
  check(
      store.publish_next(std::move(wrong_chunk)).decision ==
          AtomicLinearRunPublishDecision::transition_shape_rejected,
      "a discontinuous chunk index must be rejected");

  check(
      store.publish_next(proposal(store, 12U, 0xffU)).decision ==
          AtomicLinearRunPublishDecision::recertification_rejected,
      "a caller-rejected transition must never reach durable I/O");
  check(
      store.trusted_state().next_sequence == 0U &&
          store.status().committed_transition_count == 0U,
      "all rejected proposals must leave trusted state unchanged");

  AtomicLinearRunChunkProposal zero_budget =
      proposal(store, 12U, 0x88U);
  zero_budget.budget_snapshot_digest = CanonicalId{};
  check(
      store.publish_next(std::move(zero_budget)).decision ==
          AtomicLinearRunPublishDecision::durably_published,
      "an all-zero content digest must remain a valid "
      "recertifier-controlled value");
  check(
      store.publish_next(proposal(store, 14U, 0x99U)).decision ==
          AtomicLinearRunPublishDecision::store_limit_rejected,
      "the total encoded-byte cap must stop the next transition");
  check(
      counts.publication == 2U,
      "only bounded, structurally valid proposals reach recertification");

  AtomicLinearRunStoreLimits invalid = limits;
  invalid.maximum_payload_byte_count = 0U;
  const std::filesystem::path invalid_directory =
      workspace.make_directory("invalid-limits");
  RecertificationCounts invalid_counts;
  check_throws(
      [&]() {
        AtomicLinearRunStore invalid_store =
            AtomicLinearRunStore::create_new(
                invalid_directory,
                run_contract,
                invalid,
                recertifier(invalid_counts));
        static_cast<void>(invalid_store.status());
      },
      "zero-valued store limits must fail closed");

  const std::filesystem::path callback_directory =
      workspace.make_directory("missing-callback");
  check_throws(
      [&]() {
        AtomicLinearRunStore missing =
            AtomicLinearRunStore::create_new(
                callback_directory,
                run_contract,
                generous_limits(),
                AtomicLinearRunRecertifier{});
        static_cast<void>(missing.status());
      },
      "the publication and recovery recertifier must be mandatory");
}

void test_post_head_indeterminate_requires_reopen() {
  TemporaryWorkspace workspace;
  const std::filesystem::path pre_head_directory =
      workspace.make_directory("pre-head-recovery");
  const AtomicLinearRunContract run_contract = contract_fixture();
  const AtomicLinearRunStoreLimits limits = generous_limits();
  {
    RecertificationCounts counts;
    AtomicLinearRunStore store = AtomicLinearRunStore::create_new(
        pre_head_directory,
        run_contract,
        limits,
        recertifier(counts));
    CorruptHeadTemporaryState sabotage{
        (pre_head_directory / ".HEAD.tmp").string(), false};
    const auto result = store.publish_next(
        proposal(store, 12U, 0xa0U),
        AtomicLinearRunPublishOptions{
            corrupt_head_temporary, &sabotage});
    check(
        sabotage.corruption_succeeded &&
            result.decision ==
                AtomicLinearRunPublishDecision::
                    indeterminate_io_failure_reopen_required &&
            store.status().failed_closed_reopen_required,
        "hostile pre-rename temporary damage must fail the writer closed");
  }
  {
    RecertificationCounts replay_counts;
    AtomicLinearRunStore recovered =
        AtomicLinearRunStore::open_existing(
            pre_head_directory,
            run_contract,
            limits,
            recertifier(replay_counts));
    check(
        recovered.trusted_state().next_sequence == 0U &&
            recovered.status()
                    .removed_uncommitted_temporary_file_count ==
                1U &&
            recovered.status()
                    .removed_uncommitted_final_file_count ==
                1U &&
            replay_counts.recovery == 0U &&
            replay_counts.uncommitted_cleanup == 1U &&
            recovered.status()
                    .uncommitted_cleanup_recertification_count ==
                1U,
        "reopen must recertify then remove a pre-HEAD orphan and "
        "full-size torn HEAD temporary");
    check(
        recovered.publish_next(
                     proposal(recovered, 12U, 0xa1U))
                .decision ==
            AtomicLinearRunPublishDecision::durably_published,
        "a certified cleanup must permit the sequence to be retried");
  }

  const std::filesystem::path lost_head_directory =
      workspace.make_directory("pre-rename-head-loss");
  {
    RecertificationCounts counts;
    AtomicLinearRunStore store = AtomicLinearRunStore::create_new(
        lost_head_directory,
        run_contract,
        limits,
        recertifier(counts));
    UnlinkHeadBeforeRenameState sabotage{
        (lost_head_directory / "HEAD").string(), false};
    const auto result = store.publish_next(
        proposal(store, 12U, 0xa2U),
        AtomicLinearRunPublishOptions{
            unlink_head_before_rename, &sabotage});
    check(
        sabotage.unlink_succeeded &&
            result.decision ==
                AtomicLinearRunPublishDecision::
                    indeterminate_io_failure_reopen_required &&
            store.status().failed_closed_reopen_required &&
            !store.status().authoritative_head_certified,
        "loss of the old HEAD before rename must never be retryable");
  }
  RecertificationCounts lost_head_counts;
  check_throws(
      [&]() {
        AtomicLinearRunStore reopened =
            AtomicLinearRunStore::open_existing(
                lost_head_directory,
                run_contract,
                limits,
                recertifier(lost_head_counts));
        static_cast<void>(reopened.status());
      },
      "reopen must expose a missing pre-rename authoritative HEAD");

  const std::filesystem::path directory =
      workspace.make_directory("indeterminate");
  {
    RecertificationCounts counts;
    AtomicLinearRunStore store = AtomicLinearRunStore::create_new(
        directory, run_contract, limits, recertifier(counts));
    UnlinkAfterHeadState sabotage{
        first_final(directory).string(), false};
    const auto result = store.publish_next(
        proposal(store, 12U, 0xaaU),
        AtomicLinearRunPublishOptions{
            unlink_after_head_replace, &sabotage});
    check(
        sabotage.unlink_succeeded &&
            result.decision ==
                AtomicLinearRunPublishDecision::
                    indeterminate_io_failure_reopen_required &&
            !result.trusted_state_advanced,
        "failure after HEAD rename must be reported as indeterminate");
    check(
        store.status().failed_closed_reopen_required &&
            !store.status().authoritative_head_certified &&
            store.trusted_state().next_sequence == 0U,
        "post-rename uncertainty must fail the live writer closed");
    check(
        store.publish_next(proposal(store, 12U, 0xbbU)).decision ==
            AtomicLinearRunPublishDecision::
                indeterminate_io_failure_reopen_required,
        "a failed-closed writer must require reopen before more work");
  }

  RecertificationCounts replay_counts;
  check_throws(
      [&]() {
        AtomicLinearRunStore reopened =
            AtomicLinearRunStore::open_existing(
                directory,
                run_contract,
                limits,
                recertifier(replay_counts));
        static_cast<void>(reopened.status());
      },
      "reopen must detect a hostile missing final referenced by HEAD");
}

}  // namespace

int main() {
  try {
    test_two_chunks_reopen_and_wire();
    test_corruption_contract_and_anchors();
    test_limits_shapes_and_recertification();
    test_post_head_indeterminate_requires_reopen();
  } catch (const std::exception& error) {
    ++failures;
    std::cerr << "UNEXPECTED: " << error.what() << '\n';
  }
  if (failures != 0) {
    std::cerr << failures
              << " atomic linear run store test(s) failed\n";
    return 1;
  }
  std::cout << "atomic linear run store tests passed\n";
  return 0;
}
