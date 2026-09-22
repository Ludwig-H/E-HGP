#pragma once

#include "wspd_q2_census.hpp"

#include <cstddef>
#include <memory>

namespace mhgp9::gen {

enum class Q2CensusContinuationStatus { Ready, Done, Failed };
enum class Q2CensusResumeStage { None, Entry, Witness, Emit };

struct Q2CensusAnchorResult {
  Q2CensusResult census;
  Q2SiblingWork sibling_work;
  Q2OrderWork order_work;
};

struct Q2CensusResumeWork {
  u64 advance_calls{};
  u64 transitions{};
  u64 entry_steps{};
  u64 witness_steps{};
  u64 admission_steps{};
  u64 payload_steps{};
  u64 pauses{};
  u64 pauses_after_credit{};
  u64 pauses_inside_deferred{};
  u64 pauses_during_emission{};
  u64 max_pending_tasks{};
  bool operator==(const Q2CensusResumeWork&) const = default;
};

struct Q2CensusDetachWork {
  // Successfully completed Ready calls, including those without a sibling;
  // failed allocations/checks and Done no-ops do not increase this field.
  u64 attempts{};
  u64 detached_frames{};
  u64 imported_frames{};
  u64 transferred_pairs{};
  u64 moved_frames{};
  bool operator==(const Q2CensusDetachWork&) const = default;
};

struct Q2CensusResumeSnapshot {
  Q2CensusResult census;
  Q2SiblingWork sibling_work;
  Q2OrderWork order_work;
  Q2CensusResumeWork resume_work;
  Q2CensusContinuationStatus status = Q2CensusContinuationStatus::Ready;
  Q2CensusDetachWork detach_work;
};

// Read-only description, not an adoptable continuation or a certificate
// for another index. Ranks use the retained index's global spatial order.
struct Q2CensusResumePending {
  std::size_t task_count{};
  Q2CensusResumeStage stage = Q2CensusResumeStage::None;
  std::size_t query_node = Q2SpatialNode::absent;
  std::size_t cursor = Q2SpatialNode::absent;
  std::size_t sibling_node = Q2SpatialNode::absent;
  std::size_t original_b_node = Q2SpatialNode::absent;
  unsigned acquired_count{};
  bool inside_deferred{};
  std::size_t emit_next{};
  std::size_t emit_end{};
};

struct Q2CensusResumeMemory {
  std::size_t stack_capacity{};
  std::size_t stack_bytes{};
  std::size_t interior_capacity{};
  std::size_t shell_capacity{};
  std::size_t payload_bytes{};
  std::size_t retained_bytes{};
};

class Q2CensusContinuation;

// A single spatial anchor against ONE disjoint global B node, K in1..10.
// Owns the exact immutable index; no cloud, B tree, or factor scan is made.
// The rank and B handle are validated against this index before work starts.
// No Pool, joint query, Pairwise mode or WSPD/tower qualification is implied.
[[nodiscard]] std::unique_ptr<Q2CensusContinuation> make_q2_census_continuation(
    Q2CensusIndexPtr index, std::size_t anchor_rank, std::size_t b_node,
    unsigned kmax, Q2SiblingMode sibling_mode = Q2SiblingMode::Disabled,
    Q2WitnessOrder witness_order = Q2WitnessOrder::GlobalDfs);

// Thin non-resumable reference: calls the EXISTING shared_task path with
// exactly the same validated anchor/B/K/options and initial metadata.
// census.total_ms is one enclosing interval for this reference call.
[[nodiscard]] Q2CensusAnchorResult run_q2_anchor_reference(
    Q2CensusIndexPtr index, std::size_t anchor_rank, std::size_t b_node,
    unsigned kmax, const Q2CensusConsumer& consumer,
    Q2SiblingMode sibling_mode = Q2SiblingMode::Disabled,
    Q2WitnessOrder witness_order = Q2WitnessOrder::GlobalDfs);

// Owned, single-owner continuation. Index, original B, acquired count,
// witness cursor/phase, sibling, entry state, prepared bounds, accepted
// range and pending query siblings all survive pauses. No user callback
// or support view is retained between advances. It can be passed to another
// thread after the previous call ends under the caller's synchronization.
// advance/detach_pending/snapshot/pending/memory require exclusive ownership; overlapping
// calls and synchronous reentrance are detected and rejected with logic_error.
// These observers are intended BETWEEN advances, not from a running callback.
// index() is the exception: its immutable view is safe during an advance.
// Destroy only after all attempted calls have returned.
//
// advance executes at most budget positive transitions: one task entry
// (including its optional sibling certificate), one structural/geometric
// witness action or phase change, one admission of an accepted range, OR
// collection and emission of ONE support. Collection plus callback is
// atomic here and can still cost O(n + shell size); this is not a bound on
// wall time, instructions, or payload memory. No counter/root is replayed.
//
// An accepted range increments its accepted/uniform masses once, before
// its per-pair emissions. Intermediate snapshots may therefore count more
// accepted pairs than payload_supports; they are not completed outputs.
// Without detachment the geometric discrete counters equal the reference
// only at completion. With detachment, sum the complete lineage's geometric
// histories and masses: an individual fragment is not a fresh root census.
// Resume counters are separate; transitions is the sum of its four step
// kinds. Pauses are counted when an advance exhausts its budget unfinished.
//
// Positive budget and nonempty consumer are always validated first, without
// poisoning the object. A valid advance after Done is a no-op returning true;
// after Failed it throws logic_error. Other exceptions escaping an acquired
// advance (including collection/callback errors) irreversibly mark Failed;
// earlier emissions remain visible and this object cannot be retried.
// A rejected nested call caught INSIDE the consumer does not poison its outer
// advance. New independent continuations are allowed inside a callback.
//
// census.total_ms sums ACTIVE advance intervals, excluding time between
// pauses; payload_ms sums atomic collection/callback intervals. count_ms
// is their difference, not the historical enclosing wall-clock semantics.
// Factory, detachment, observer and destruction costs are not included in these clocks.
// query_index_ms remains zero. Memory reports vector capacities only,
// excluding shared index/cloud, object metadata, temporaries and thread stack.
class Q2CensusContinuation final {
 public:
  ~Q2CensusContinuation();
  Q2CensusContinuation(const Q2CensusContinuation&) = delete;
  Q2CensusContinuation& operator=(const Q2CensusContinuation&) = delete;
  Q2CensusContinuation(Q2CensusContinuation&&) = delete;
  Q2CensusContinuation& operator=(Q2CensusContinuation&&) = delete;
  [[nodiscard]] bool advance(std::size_t budget, const Q2CensusConsumer& consumer);
  // Detach the OLDEST unvisited sibling (stack.front), never the current
  // frame (stack.back), and only when at least two frames remain. The child
  // owns its index, engine, sentinel and buffers; original B/escape, anchor,
  // count/cursor/phase/sibling stay unchanged. No root_start, descriptor or
  // historical geometric counter is copied or replayed. Its candidates are
  // exactly the detached B population m; the donor's candidates decrease
  // by m while every already paid history remains in the donor.
  //
  // A Ready call without a sibling returns nullptr and increments attempts.
  // Done returns nullptr as an exact no-op; Failed/overlapping calls throw
  // logic_error. All fallible construction (including the public child)
  // and counter/ledger checks precede the nonthrowing transfer commit. An
  // allocation failure leaves the donor EXACTLY unchanged, attempts included,
  // and does not poison it. The returned fragment is independently resumable.
  //
  // Donor stats: attempts+1; on success detached_frames+1, transferred_pairs
  // +=m, moved_frames+=old_stack_size-1 (value shifts from erasing the first
  // frame). Child stats: imported_frames=1, everything else zero. Its resume
  // history is zero except max_pending_tasks=1, describing its initial stack.
  // Each child currently reserves index_stack_frames (55) frames and starts with empty payload
  // buffers; detachment is not a promise of a constant total memory footprint.
  // Across all descendants detached_frames=imported_frames; transferred_pairs
  // is cumulative traffic and may exceed the initial mass after repeat moves.
  [[nodiscard]] std::unique_ptr<Q2CensusContinuation> detach_pending();
  [[nodiscard]] Q2CensusResumeSnapshot snapshot() const;
  [[nodiscard]] Q2CensusResumePending pending() const;
  [[nodiscard]] Q2CensusResumeMemory memory() const;
  // Immutable owner view, without acquiring the continuation's busy flag.
  [[nodiscard]] const Q2CensusIndex& index() const noexcept;

 private:
  struct Impl;
  explicit Q2CensusContinuation(std::unique_ptr<Impl> implementation);
  std::unique_ptr<Impl> implementation_;
  friend std::unique_ptr<Q2CensusContinuation> make_q2_census_continuation(
      Q2CensusIndexPtr, std::size_t, std::size_t, unsigned, Q2SiblingMode, Q2WitnessOrder);
};

}  // namespace mhgp9::gen
