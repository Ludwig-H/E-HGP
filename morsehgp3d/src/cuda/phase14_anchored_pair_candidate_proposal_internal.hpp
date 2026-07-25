#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace morsehgp3d::gpu::detail {

inline constexpr std::size_t
    phase14_anchored_pair_candidate_maximum_witness_count = 64U;
inline constexpr std::uint64_t
    phase14_anchored_pair_candidate_invalid_node_index =
        std::numeric_limits<std::uint64_t>::max();
inline constexpr std::uint64_t
    phase14_anchored_pair_candidate_sentinel =
        std::numeric_limits<std::uint64_t>::max();
inline constexpr std::size_t
    phase14_anchored_pair_candidate_maximum_cuda_kernel_launch_count = 8U;

enum class Phase14AnchoredPairCandidateCursorKind : std::uint64_t {
  initial = 0U,
  active = 1U,
  complete = 2U,
};

enum class Phase14AnchoredPairCandidateSegmentStatus : std::uint64_t {
  complete = 0U,
  budget_exhausted = 1U,
  capacity_exhausted = 2U,
};

enum class Phase14AnchoredPairCandidateStopReason : std::uint64_t {
  none = 0U,
  node_visit_limit = 1U,
  transcript_record_limit = 2U,
};

enum class Phase14AnchoredPairCandidateFailureCode : std::uint64_t {
  none = 0U,
  malformed_query = 1U,
  node_out_of_range = 2U,
  traversal_failure = 3U,
};

enum class Phase14AnchoredPairCandidateExecutionKind : std::uint64_t {
  host_fake = 0U,
  cuda = 1U,
};

// CUDA may discover records in warp/atomic order, but it must publish them in
// query order.  A bounded scan/compaction pipeline avoids requiring one
// globally ordered traversal kernel and keeps the resident arena fixed.
enum class Phase14AnchoredPairCandidateCompactionKind : std::uint64_t {
  host_fake = 0U,
  device_scan = 1U,
};

struct Phase14AnchoredPairCandidateQueryInputRecord {
  std::uint64_t query_index{phase14_anchored_pair_candidate_sentinel};
  std::uint64_t query_digest_fnv1a{
      phase14_anchored_pair_candidate_sentinel};
  std::uint64_t anchor_point_id{
      phase14_anchored_pair_candidate_sentinel};
  std::uint64_t maximum_closed_rank{};
  std::uint64_t witness_count{};
  std::uint64_t cursor_kind{};
  std::uint64_t cursor_node_index{
      phase14_anchored_pair_candidate_invalid_node_index};
  std::uint64_t cursor_query_digest_fnv1a{};
  std::uint64_t cursor_source_snapshot_epoch{};
  std::uint64_t maximum_node_visit_count{};
  std::uint64_t maximum_transcript_record_count{};
  std::uint64_t witnesses[
      phase14_anchored_pair_candidate_maximum_witness_count]{};
};

static_assert(
    std::is_standard_layout_v<
        Phase14AnchoredPairCandidateQueryInputRecord>);
static_assert(
    std::is_trivially_copyable_v<
        Phase14AnchoredPairCandidateQueryInputRecord>);
static_assert(
    sizeof(Phase14AnchoredPairCandidateQueryInputRecord) ==
    75U * sizeof(std::uint64_t));

struct Phase14AnchoredPairCandidateTranscriptRecord {
  std::uint64_t node_index{
      phase14_anchored_pair_candidate_invalid_node_index};
  std::uint64_t witness_mask{};
};

static_assert(
    std::is_standard_layout_v<
        Phase14AnchoredPairCandidateTranscriptRecord>);
static_assert(
    std::is_trivially_copyable_v<
        Phase14AnchoredPairCandidateTranscriptRecord>);
static_assert(
    sizeof(Phase14AnchoredPairCandidateTranscriptRecord) == 16U);

struct Phase14AnchoredPairCandidateSegmentRecord {
  std::uint64_t query_index{phase14_anchored_pair_candidate_sentinel};
  std::uint64_t query_digest_fnv1a{
      phase14_anchored_pair_candidate_sentinel};
  std::uint64_t buffer_epoch{phase14_anchored_pair_candidate_sentinel};
  std::uint64_t source_snapshot_epoch{
      phase14_anchored_pair_candidate_sentinel};
  std::uint64_t input_cursor_kind{
      phase14_anchored_pair_candidate_sentinel};
  std::uint64_t input_cursor_node_index{
      phase14_anchored_pair_candidate_invalid_node_index};
  std::uint64_t next_cursor_kind{
      phase14_anchored_pair_candidate_sentinel};
  std::uint64_t next_cursor_node_index{
      phase14_anchored_pair_candidate_invalid_node_index};
  std::uint64_t status{phase14_anchored_pair_candidate_sentinel};
  std::uint64_t stop_reason{phase14_anchored_pair_candidate_sentinel};
  std::uint64_t node_visit_count{
      phase14_anchored_pair_candidate_sentinel};
  std::uint64_t record_begin{phase14_anchored_pair_candidate_sentinel};
  std::uint64_t record_count{phase14_anchored_pair_candidate_sentinel};
  std::uint64_t failure_code{phase14_anchored_pair_candidate_sentinel};
};

static_assert(
    std::is_standard_layout_v<
        Phase14AnchoredPairCandidateSegmentRecord>);
static_assert(
    std::is_trivially_copyable_v<
        Phase14AnchoredPairCandidateSegmentRecord>);
static_assert(
    sizeof(Phase14AnchoredPairCandidateSegmentRecord) ==
    14U * sizeof(std::uint64_t));

struct Phase14AnchoredPairCandidateAdoptedTraversal {
  std::shared_ptr<void> owner;
  const std::uint64_t* device_coordinate_bits{};
  const std::uint64_t* device_morton_point_ids{};
  const void* device_nodes{};
  std::size_t coordinate_word_capacity{};
  std::size_t morton_point_id_capacity{};
  std::size_t node_capacity{};
  std::size_t point_count{};
  std::size_t node_count{};
  std::uint64_t source_snapshot_epoch{};
  int cuda_device{-1};
  bool cuda_resident{false};
  bool host_fake{false};
};

struct Phase14AnchoredPairCandidateDeviceBatch {
  // records is resized to the authenticated compact prefix after the sole
  // synchronization.  The initialized and copied transcript byte counts name
  // the full active limit, since the host cannot know record_count before that
  // synchronization.  The inactive suffix has no authority and is omitted
  // from records and the digest.
  std::vector<Phase14AnchoredPairCandidateSegmentRecord> segments;
  std::vector<Phase14AnchoredPairCandidateTranscriptRecord> records;
  std::size_t segment_count{};
  std::size_t record_count{};
  std::size_t host_to_device_query_byte_count{};
  std::size_t initialized_segment_byte_count{};
  std::size_t initialized_transcript_byte_count{};
  std::size_t device_to_host_segment_byte_count{};
  std::size_t device_to_host_transcript_byte_count{};
  std::size_t kernel_launch_count{};
  std::size_t synchronization_count{};
  std::size_t compaction_kernel_launch_count{};
  std::size_t physical_query_capacity{};
  std::size_t physical_transcript_record_capacity{};
  std::size_t active_total_transcript_record_limit{};
  std::uint64_t buffer_epoch{};
  std::uint64_t source_snapshot_epoch{};
  std::uint64_t proposal_digest_fnv1a{};
  int cuda_device{-1};
  bool cuda_path_qualified{false};
  Phase14AnchoredPairCandidateExecutionKind execution_kind{
      Phase14AnchoredPairCandidateExecutionKind::host_fake};
  Phase14AnchoredPairCandidateCompactionKind compaction_kind{
      Phase14AnchoredPairCandidateCompactionKind::host_fake};
};

[[nodiscard]] constexpr std::uint64_t
phase14_anchored_pair_candidate_digest_word(
    std::uint64_t digest,
    std::uint64_t word) noexcept {
  constexpr std::uint64_t kFnvPrime = UINT64_C(1099511628211);
  for (unsigned int shift = 0U; shift < 64U; shift += 8U) {
    digest ^= (word >> shift) & UINT64_C(0xff);
    digest *= kFnvPrime;
  }
  return digest;
}

[[nodiscard]] inline std::uint64_t
phase14_anchored_pair_candidate_batch_digest(
    const Phase14AnchoredPairCandidateDeviceBatch& batch) noexcept {
  std::uint64_t digest = UINT64_C(14695981039346656037);
  digest = phase14_anchored_pair_candidate_digest_word(
      digest, UINT64_C(0x5038645452414e31));
  digest = phase14_anchored_pair_candidate_digest_word(
      digest, batch.buffer_epoch);
  digest = phase14_anchored_pair_candidate_digest_word(
      digest, batch.source_snapshot_epoch);
  digest = phase14_anchored_pair_candidate_digest_word(
      digest, static_cast<std::uint64_t>(batch.physical_query_capacity));
  digest = phase14_anchored_pair_candidate_digest_word(
      digest,
      static_cast<std::uint64_t>(
          batch.physical_transcript_record_capacity));
  digest = phase14_anchored_pair_candidate_digest_word(
      digest,
      static_cast<std::uint64_t>(
          batch.active_total_transcript_record_limit));
  digest = phase14_anchored_pair_candidate_digest_word(
      digest, static_cast<std::uint64_t>(batch.kernel_launch_count));
  digest = phase14_anchored_pair_candidate_digest_word(
      digest,
      static_cast<std::uint64_t>(
          batch.compaction_kernel_launch_count));
  digest = phase14_anchored_pair_candidate_digest_word(
      digest, static_cast<std::uint64_t>(batch.execution_kind));
  digest = phase14_anchored_pair_candidate_digest_word(
      digest, static_cast<std::uint64_t>(batch.compaction_kind));
  digest = phase14_anchored_pair_candidate_digest_word(
      digest, static_cast<std::uint64_t>(batch.segments.size()));
  digest = phase14_anchored_pair_candidate_digest_word(
      digest, static_cast<std::uint64_t>(batch.records.size()));
  for (const Phase14AnchoredPairCandidateSegmentRecord& segment :
       batch.segments) {
    digest = phase14_anchored_pair_candidate_digest_word(
        digest, segment.query_index);
    digest = phase14_anchored_pair_candidate_digest_word(
        digest, segment.query_digest_fnv1a);
    digest = phase14_anchored_pair_candidate_digest_word(
        digest, segment.buffer_epoch);
    digest = phase14_anchored_pair_candidate_digest_word(
        digest, segment.source_snapshot_epoch);
    digest = phase14_anchored_pair_candidate_digest_word(
        digest, segment.input_cursor_kind);
    digest = phase14_anchored_pair_candidate_digest_word(
        digest, segment.input_cursor_node_index);
    digest = phase14_anchored_pair_candidate_digest_word(
        digest, segment.next_cursor_kind);
    digest = phase14_anchored_pair_candidate_digest_word(
        digest, segment.next_cursor_node_index);
    digest = phase14_anchored_pair_candidate_digest_word(
        digest, segment.status);
    digest = phase14_anchored_pair_candidate_digest_word(
        digest, segment.stop_reason);
    digest = phase14_anchored_pair_candidate_digest_word(
        digest, segment.node_visit_count);
    digest = phase14_anchored_pair_candidate_digest_word(
        digest, segment.record_begin);
    digest = phase14_anchored_pair_candidate_digest_word(
        digest, segment.record_count);
    digest = phase14_anchored_pair_candidate_digest_word(
        digest, segment.failure_code);
  }
  for (const Phase14AnchoredPairCandidateTranscriptRecord& record :
       batch.records) {
    digest = phase14_anchored_pair_candidate_digest_word(
        digest, record.node_index);
    digest = phase14_anchored_pair_candidate_digest_word(
        digest, record.witness_mask);
  }
  return digest;
}

class Phase14AnchoredPairCandidateProposalContextState final {
 public:
  Phase14AnchoredPairCandidateProposalContextState(
      std::size_t maximum_query_count,
      std::size_t physical_transcript_record_capacity)
      : maximum_query_count_(maximum_query_count),
        physical_transcript_record_capacity_(
            physical_transcript_record_capacity) {}
  ~Phase14AnchoredPairCandidateProposalContextState() = default;

  Phase14AnchoredPairCandidateProposalContextState(
      const Phase14AnchoredPairCandidateProposalContextState&) = delete;
  Phase14AnchoredPairCandidateProposalContextState& operator=(
      const Phase14AnchoredPairCandidateProposalContextState&) = delete;

  template <typename Operation>
  decltype(auto) with_gpu_section(Operation&& operation) {
    std::lock_guard<std::mutex> lock{mutex_};
    if (poisoned_.load(std::memory_order_acquire)) {
      throw std::runtime_error(
          "the Phase 14 anchored-pair proposal context is poisoned by a "
          "prior launcher or transcript validation failure");
    }
    try {
      return std::forward<Operation>(operation)();
    } catch (...) {
      poisoned_.store(true, std::memory_order_release);
      throw;
    }
  }

  [[nodiscard]] std::shared_ptr<void>& cuda_resources() noexcept {
    return cuda_resources_;
  }

  [[nodiscard]] std::uint64_t advance_epoch() {
    if (epoch_ == std::numeric_limits<std::uint64_t>::max()) {
      throw std::overflow_error(
          "the Phase 14 anchored-pair proposal epoch overflowed");
    }
    return ++epoch_;
  }

  [[nodiscard]] std::uint64_t current_epoch() const noexcept {
    return epoch_;
  }

  [[nodiscard]] bool fixed_capacities_match(
      std::size_t maximum_query_count,
      std::size_t physical_transcript_record_capacity) const noexcept {
    return maximum_query_count == maximum_query_count_ &&
           physical_transcript_record_capacity ==
               physical_transcript_record_capacity_;
  }

  [[nodiscard]] std::size_t maximum_query_count() const noexcept {
    return maximum_query_count_;
  }

  [[nodiscard]] std::size_t physical_transcript_record_capacity()
      const noexcept {
    return physical_transcript_record_capacity_;
  }

 private:
  std::mutex mutex_;
  std::shared_ptr<void> cuda_resources_;
  std::atomic<bool> poisoned_{false};
  std::uint64_t epoch_{};
  // Immutable constructor capacities.  A CUDA implementation may allocate
  // lazily, but only to these extents; an active call limit must never size or
  // resize the resident arenas.
  const std::size_t maximum_query_count_{};
  const std::size_t physical_transcript_record_capacity_{};
};

[[nodiscard]] Phase14AnchoredPairCandidateDeviceBatch
propose_phase14_anchored_pair_candidates_on_gpu(
    Phase14AnchoredPairCandidateProposalContextState& context,
    const Phase14AnchoredPairCandidateAdoptedTraversal& traversal,
    std::span<const Phase14AnchoredPairCandidateQueryInputRecord> queries,
    std::size_t maximum_query_count,
    std::size_t physical_transcript_record_capacity,
    std::size_t active_total_transcript_record_limit);

}  // namespace morsehgp3d::gpu::detail
