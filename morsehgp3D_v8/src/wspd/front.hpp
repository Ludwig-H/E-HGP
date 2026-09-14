#pragma once

#include "pipeline/q2_census.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

namespace mhgp8 {

enum class WspdFrontMode { Pure, MidpointSamples };

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
// backtracking, and proposes at most Kmax distinct adjacent ranks. It is
// a rejection heuristic only: missing witnesses never remove any pair.
// Neither this front nor a bounded ledger certifies a complete HGP tower.
// Index/consumer must remain alive and valid throughout this synchronous
// call. A callback exception propagates; prior emissions are not undone.
[[nodiscard]] WspdFrontResult run_wspd_front(
    const Q2CensusIndex& index, unsigned kmax, unsigned separation_s,
    WspdFrontMode mode, const WspdRectangleConsumer& consumer,
    std::uint8_t requested_lane_mask = 7);

class WspdFrontJobs;
[[nodiscard]] std::unique_ptr<WspdFrontJobs> make_wspd_front_jobs(
    Q2CensusIndexPtr index, unsigned kmax, unsigned separation_s,
    WspdFrontMode mode, std::size_t target_jobs,
    std::uint8_t requested_lane_mask = 7);

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

 private:
  struct Impl;
  explicit WspdFrontJobs(std::unique_ptr<Impl> implementation);
  std::unique_ptr<Impl> implementation_;
  friend std::unique_ptr<WspdFrontJobs> make_wspd_front_jobs(
      Q2CensusIndexPtr, unsigned, unsigned, WspdFrontMode, std::size_t, std::uint8_t);
};

}  // namespace mhgp8
