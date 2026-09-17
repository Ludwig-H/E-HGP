#pragma once

#include "pipeline/q2_census.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>

namespace mhgp8 {

enum class WspdFrontMode { Pure, MidpointSamples };

// Options of the MidpointSamples proposer of the q2 lane. The rejection
// threshold and the predicate never change: only which ranks are proposed and
// which already certified identifiers a product receives. The default
// reproduces the historical front exactly, counter for counter. A window
// factor other than 1, or inherited witnesses, require the active lane mask to
// be q2 alone and the MidpointSamples mode: no independent judge covers them
// on q3/q4, so they are refused. A finite small_factor_limit alone is inert.
struct WspdFrontProposals {
  // L = window_factor*Kmax ranks around the SAME pivot, truncated to n.
  // Accepted values: 1 (historical window only), 2 and 4.
  unsigned window_factor{1};
  // The extra ranks are proposed only when max(|A|,|B|) <= this limit.
  // The default admits every product; it never removes or caps any pair.
  std::size_t small_factor_limit{std::numeric_limits<std::size_t>::max()};
  // Certified witness IDENTIFIERS (ranks), never bare counts, passed by a
  // surviving product to both children: at most Kmax-1 ranks. h_minimum is an
  // exact minimum over the boxes and child boxes are included in parent boxes,
  // so an inherited rank stays a strict universal witness, outside A' and B'.
  // The child starts from that count, skips without a test any proposed rank
  // already in its list, and appends its new credits. A rejection is still
  // certified by Kmax DISTINCT ranks, all universal for the product's boxes.
  bool inherit_witnesses{false};
  bool operator==(const WspdFrontProposals&) const = default;
};

// A task stores its received ranks on 32 bits: the option is usable only when
// every rank fits, i.e. for at most 2^32 sites. Refused otherwise.
[[nodiscard]] constexpr bool wspd_proposals_fit(const WspdFrontProposals& proposals, std::uint64_t sites) noexcept {
  return !proposals.inherit_witnesses || sites <= (std::uint64_t{1} << 32);
}

// The centered, clamped rank window of MidpointSamples: `count` adjacent
// ranks (count <= n required) around `pivot` (< n). Exposed so that a gate
// can judge the window arithmetic exhaustively. For count <= wider <= n the
// window of `wider` ranks contains the window of `count` ranks.
struct WspdProposalWindow {
  std::size_t first{};
  std::size_t last{};
  bool operator==(const WspdProposalWindow&) const = default;
};
[[nodiscard]] constexpr WspdProposalWindow wspd_proposal_window(
    std::size_t pivot, std::size_t count, std::size_t n) noexcept {
  const auto first = std::min(pivot > count / 2 ? pivot - count / 2 : std::size_t{0}, n - count);
  return {first, first + count};
}

// Borrowed index-relative node IDs. This is a producer event, not a public
// adoptable geometric certificate. Its ranges use this index's order only.
// It may be retained as data if the caller also retains the exact index.
struct WspdRectangle {
  std::size_t a_node{};
  std::size_t b_node{};
  std::uint8_t lane_mask{};  // Bit 0=q2, bit 1=q3, bit 2=q4. No tower output.
};

struct WspdFrontWork {
  u64 product_visits{};
  u64 diagonal_splits{};
  u64 diagonal_leaves{};
  u64 disjoint_splits{};
  u64 separation_tests{};
  u64 witness_searches{};
  u64 witness_descent_steps{};
  u64 witness_box_distance_tests{};
  u64 proposed_sites{};
  u64 proposals_in_factors{};
  u64 h_bound_tests{};
  u64 xi_bound_tests{};
  u64 witness_lane_credits{};
  u64 fully_rejected_products{};
  u64 emitted_rectangles{};
  u64 emitted_factor_sites{};
  u64 max_factor_size{};
  u64 leaf_pair_rectangles{};
  u64 max_stack_size{};  // Canonical mono DFS high-water, not job storage or worker RAM.
  u64 max_product_depth{};
  // Classes of max(|A|,|B|): 1, 2..7, 8..63, 64..1023, >=1024.
  std::array<u64, 5> size_class_rectangles{};
  std::array<u64, 5> size_class_pair_mass{};
  std::array<u64, 3> rejected_pair_mass{};
  std::array<u64, 3> residual_pair_mass{};
  std::array<u64, 3> lane_rectangles{};
  // Window extension of the q2 lane (WspdFrontProposals). All zero when
  // window_factor is 1. These counters are INCLUDED in proposed_sites,
  // proposals_in_factors, h_bound_tests and witness_lane_credits: subtracting
  // them yields exactly the historical-window work on the visited products.
  u64 extended_products{};   // Products that proposed at least one extra rank.
  u64 extended_proposals{};  // Extra proposed ranks beyond the historical window.
  u64 extended_proposals_in_factors{};  // Extra ranks skipped because they belong to A or B.
  u64 extended_credits{};    // q2 credits granted by extra ranks.
  u64 extended_rejections{};  // Products whose Kmax-th q2 credit came from an extra rank.
  // Inherited witnesses. All zero unless inherit_witnesses. Inherited credits are
  // NOT in witness_lane_credits, which counts the credits granted by tests of
  // this run. A duplicate is a proposed rank already in the received list: it
  // is counted in proposed_sites (and in extended_proposals inside an extension
  // interval), then skipped without a test. Hence, for every option,
  // proposed_sites = proposals_in_factors + h_bound_tests + inherited_duplicates.
  // The extension part has no H-test counter of its own: on a receipt only
  // extended_credits + extended_inherited_duplicates <=
  // extended_proposals - extended_proposals_in_factors is observable.
  u64 inherited_credits{};              // Sum over searches of the received list length: even.
  u64 inherited_duplicates{};           // Proposed ranks skipped because already received.
  u64 extended_inherited_duplicates{};  // Those of the extension intervals, INCLUDED above.
  // Rejections pronounced while the new credits of the search plus the
  // received ranks it proposed again stay below Kmax: the ranks seen by this
  // search did not suffice. An UPPER bound of the rejections that the same
  // product would not have obtained alone, since the rest of its window was
  // not scanned: the true count needs the tests that inheritance saves, and
  // belongs to an independent replay.
  u64 inherited_rejections{};
  // Final q2 credits (received and new) of the searched products that were
  // emitted. Credit ledger of the inheritance, every searched product being
  // rejected with Kmax credits, split (its credits are received twice) or
  // emitted: witness_lane_credits + inherited_credits/2 =
  // Kmax*fully_rejected_products + emitted_witness_credits.
  u64 emitted_witness_credits{};
  bool operator==(const WspdFrontWork&) const = default;
};

struct WspdFrontResult {
  u64 total_unordered_pairs{};
  std::uint8_t active_lane_mask{};
  WspdFrontWork work;
};

using WspdRectangleConsumer = std::function<void(const WspdRectangle&)>;

// Exactly the existing v8 convention: squared box gap >= s^2 times the
// largest squared box diagonal. Not the v4 center/radius convention.
// Stream a separated residual cover of every active lane, after safe
// optional early rejection. No copies/validation/scans of n sites or of
// factors per product; no stored complete frontier or rectangle catalogue.
// Kmax is in 1..10; lane q is active iff requested AND q<=Kmax+1, threshold
// Kmax+2-q. requested_lane_mask uses bits 0..2; no other bit, zero mask, or
// empty intersection with the supported lanes is accepted. Default=all.
// MidpointSamples descends one spatial path, with no nearest-neighbor
// backtracking, and proposes the historical window of min(Kmax,n) distinct
// adjacent ranks around its pivot. With proposals.window_factor > 1 (q2
// lane alone), a product that survives that window and whose factors
// satisfy small_factor_limit then proposes the two disjoint intervals
// completing the window of min(window_factor*Kmax,n) ranks around the
// SAME pivot: left interval first, then right, no second descent. Credits
// acquired by the historical window remain valid during this one filter
// call and the threshold stays Kmax. Ranks of A or B are counted as
// proposed, then skipped without a test or a credit; H=0 never credits.
// A bare partial COUNT is never inherited by descendants or by any census:
// it could count one site twice. With proposals.inherit_witnesses (q2 lane
// alone) a surviving product passes the RANKS of its certified witnesses to
// its children, which count each rank once; the census still starts from
// zero. The received list travels in the task: the work of a product depends
// on its task alone, and counters are identical for the mono front, any job
// plan and any dispatch, with any worker count.
// Pure proposes nothing: it requires window_factor 1 and no inheritance. The
// front is a rejection heuristic only: missing witnesses never remove any pair.
// Neither this front nor a bounded ledger certifies a complete HGP tower.
// Index/consumer must remain alive and valid throughout this synchronous
// call. A callback exception propagates; prior emissions are not undone.
[[nodiscard]] WspdFrontResult run_wspd_front(
    const Q2CensusIndex& index, unsigned kmax, unsigned separation_s,
    WspdFrontMode mode, const WspdRectangleConsumer& consumer,
    std::uint8_t requested_lane_mask = 7, WspdFrontProposals proposals = {});

class WspdFrontJobs;
class WspdFrontDispatch;

struct WspdFrontDispatchWork {
  u64 seeds_started{};
  u64 seeds_completed{};
  u64 donations{};
  u64 donor_checks{};
  u64 offer_attempts{};
  u64 offer_full{};
  u64 offer_busy{};
  u64 offer_no_demand{};
  u64 stolen_started{};
  u64 stolen_completed{};
  u64 waits{};
  u64 wakes{};
  u64 max_queue_size{};
  u64 max_local_stack_size{};
  bool operator==(const WspdFrontDispatchWork&) const = default;
};

struct WspdFrontDispatchResult {
  WspdFrontResult front;
  WspdFrontDispatchWork work;
};
[[nodiscard]] std::unique_ptr<WspdFrontJobs> make_wspd_front_jobs(
    Q2CensusIndexPtr index, unsigned kmax, unsigned separation_s,
    WspdFrontMode mode, std::size_t target_jobs,
    std::uint8_t requested_lane_mask = 7, WspdFrontProposals proposals = {});

// An immutable, owning partition of ONE front traversal, not a catalogue
// of its complete WSPD. The breadth-first preparation stops when pending
// products plus stored terminal rectangles reach the requested granularity
// (at most target_jobs+2 states). Fully rejected/diagonal leaves need no job.
// Every parent is tested before subdivision; its counters belong only to
// prefix_result(). A stored terminal has already been counted there, so its
// run_job merely invokes the consumer. Other jobs resume an unvisited product
// with the inherited lane mask, depth and canonical DFS pending-sibling count.
//
// Sum prefix and job work counters, but combine max_factor_size,
// max_stack_size and max_product_depth by MAX. The stack maximum denotes
// the equivalent mono DFS, not an actual worker stack or retained storage.
// Every partial result repeats the GLOBAL pair-count/mask metadata; do not
// add these metadata. Their lane mass ledger is complete only after all
// jobs have been consumed exactly once. Different job orders are allowed.
//
// Jobs are reusable and const; concurrent calls need independent consumers
// (or caller synchronization). No callback is retained. A callback exception
// propagates without undoing prior emissions; retrying repeats that job and
// requires discarding/distinguishing the earlier partial output. The plan
// retains its exact immutable index even after the caller resets its handle.
// There is no interruption point inside a running job in this API.
class WspdFrontJobs final {
 public:
  ~WspdFrontJobs();
  WspdFrontJobs(const WspdFrontJobs&) = delete;
  WspdFrontJobs& operator=(const WspdFrontJobs&) = delete;
  WspdFrontJobs(WspdFrontJobs&&) = delete;
  WspdFrontJobs& operator=(WspdFrontJobs&&) = delete;
  [[nodiscard]] std::size_t job_count() const noexcept;
  [[nodiscard]] std::size_t terminal_job_count() const noexcept;
  [[nodiscard]] const WspdFrontResult& prefix_result() const noexcept;
  [[nodiscard]] const Q2CensusIndex& index() const noexcept;
  [[nodiscard]] WspdFrontResult run_job(std::size_t id, const WspdRectangleConsumer& consumer) const;
  // Retained job-vector capacity only; excludes the owned shared index,
  // plan metadata, preparation queue and independent worker stacks.
  [[nodiscard]] std::size_t retained_bytes() const;
  // A new, single-use dispatcher over these immutable seeds. Its owning
  // context survives destruction of this plan and of the external index.
  // All three arguments must be positive; they never truncate exploration.
  [[nodiscard]] std::unique_ptr<WspdFrontDispatch> make_dispatch(
      std::size_t queue_capacity, std::size_t donation_interval,
      std::size_t worker_count) const;

 private:
  struct Impl;
  explicit WspdFrontJobs(std::shared_ptr<const Impl> implementation);
  std::shared_ptr<const Impl> implementation_;
  friend class WspdFrontDispatch;
  friend std::unique_ptr<WspdFrontJobs> make_wspd_front_jobs(
      Q2CensusIndexPtr, unsigned, unsigned, WspdFrontMode, std::size_t, std::uint8_t,
      WspdFrontProposals);
};

// Cooperative redistribution of UNVISITED products only. A terminal seed
// merely invokes its callback; no filter/emission counter is paid twice.
// Each worker has one private DFS stack; it acquires a seed or donation
// only when that stack is empty. An offer never waits for queue space or
// its mutex: failure retains the product locally. The queue is allocated
// before any worker starts. No census/callback continuation is transferred.
//
// Call run_worker at most worker_count times, with independent consumers.
// The declared count is also the number of available slots used to request
// donations: not-yet-started slots are included. Fewer actual callers remain
// correct but may make a worker consume its own donations. One declared
// worker bypasses all per-product donation/cancellation polling and locks.
// During multi-worker traversal cancellation is polled between bounded
// batches of whole products, never within a callback. Setting an external
// cancellation flag must be accompanied by cancel() to wake sleepers.
// cancel() itself is sufficient, idempotent, and also called on exceptions.
// A nested synchronous call must use a NEW dispatcher (or run_job/mono),
// not this same single-use dispatcher: otherwise it could await its own
// outer fragment. No worker synchronizes the caller's mutable captures.
// All run_worker calls must finish before destroying this dispatcher.
//
// Normal completion: sums seeds_started=seeds_completed=job_count;
// donations=stolen_started=stolen_completed. Completed seeds/fragments mean
// their LOCAL remainder is complete; donated descendants close separately.
// donor_checks=offer_no_demand+offer_attempts; offer_attempts=donations+
// offer_full+offer_busy. Combine the two dispatch maxima by MAX, the other
// counters by sum. Prefix/front work and metadata follow the Jobs contract.
// Cancellation/exception does not roll back output or imply completion.
class WspdFrontDispatch final {
 public:
  ~WspdFrontDispatch();
  WspdFrontDispatch(const WspdFrontDispatch&) = delete;
  WspdFrontDispatch& operator=(const WspdFrontDispatch&) = delete;
  WspdFrontDispatch(WspdFrontDispatch&&) = delete;
  WspdFrontDispatch& operator=(WspdFrontDispatch&&) = delete;
  [[nodiscard]] WspdFrontDispatchResult run_worker(
      const WspdRectangleConsumer& consumer, const std::atomic<bool>& cancellation);
  void cancel() noexcept;
  [[nodiscard]] std::size_t queue_capacity() const noexcept;
  [[nodiscard]] std::size_t worker_count() const noexcept;
  // Read-only observation, including the interval immediately before a
  // receiver enters its condition wait. Not a completion certificate.
  [[nodiscard]] std::size_t waiting_workers() const noexcept;
  // Read-only observation for gates: total number of received witness ranks
  // carried by the tasks published to the donation queue so far. Zero unless
  // proposals.inherit_witnesses. Not a work counter and not a ledger term.
  [[nodiscard]] u64 donated_witness_ranks() const noexcept;
  // Queue-vector capacity only. Excludes shared initial jobs/index, local
  // stacks, callbacks, object metadata and allocations made by consumers.
  [[nodiscard]] std::size_t retained_bytes() const;

 private:
  struct Impl;
  explicit WspdFrontDispatch(std::unique_ptr<Impl> implementation);
  std::unique_ptr<Impl> implementation_;
  friend class WspdFrontJobs;
};

}  // namespace mhgp8
