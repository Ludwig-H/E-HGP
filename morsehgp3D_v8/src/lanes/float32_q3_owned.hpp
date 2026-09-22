#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

#include "core/float32_ball.hpp"
#include "core/float32_edge_geometry.hpp"
#include "core/float32_q3_owned_block.hpp"

namespace mhgp8 {

enum class Float32Q3OwnedMode { Individual, SharedPrefix };
struct Float32Q3OwnedOptions {
  std::size_t kmax{10};
  Float32Q3OwnedMode mode{Float32Q3OwnedMode::Individual};
  std::size_t relay_sites{8}; // Sharing grain, never a population/work quota.
};

struct Float32Q3OwnedWork {
  Float32EdgeWork selection{};
  Float32Q3OwnedBlockWork shared_bounds{}, individual_bounds{};
  Float32BallWork supports{}, power{};
  // SUM except the final four explicit MAX fields. Logical input mass need
  // not be physically scanned. On success input_seed_slots is partitioned
  // into seed_rejected_slots + shared_rejected_seed_slots + relayed_seed_slots.
  std::uint64_t calls{}, input_seed_slots{}, seed_node_visits{}, seed_rejected_nodes{}, seed_rejected_slots{};
  std::uint64_t seed_splits{}, shared_frames{}, shared_witness_visits{}, shared_endpoint_skips{};
  std::uint64_t shared_inside_nodes{}, shared_inside_sites{}, shared_outside_nodes{}, shared_witness_splits{};
  std::uint64_t shared_saturated_blocks{}, shared_rejected_seed_slots{}, shared_children_with_credit{};
  std::uint64_t relay_blocks{}, relayed_seed_slots{}, endpoint_seeds{}, owner_candidates{}, owner_rejections{};
  std::uint64_t owned_seeds{}, invalid_supports{}, valid_supports{}, relays_with_credit{}, relays_at_eof{};
  std::uint64_t count_node_visits{}, count_point_tests{}, count_inside_nodes{}, count_inside_sites{};
  std::uint64_t count_outside_nodes{}, count_splits{}, saturated_supports{}, accepted_supports{};
  std::uint64_t shell_node_visits{}, shell_point_tests{}, shell_excluded_nodes{}, shell_splits{}, shell_ids{}, callbacks{};
  std::uint64_t stack_reserves{}, shell_growths{};
  std::uint64_t peak_pending_frames{}, stack_capacity_bytes{}, peak_shell_capacity_bytes{}, peak_workspace_bytes{}; // MAX.
  bool operator==(const Float32Q3OwnedWork&) const = default;
};

struct Float32Q3OwnedEmission {
  std::size_t seed{}, depth{};
  const Float32Ball& ball;
  std::span<const std::size_t> shell; // Complete original IDs, unsorted, no repeats.
};
using Float32Q3OwnedConsumer = std::function<void(const Float32Q3OwnedEmission&)>;

// One privately mutable workspace per simultaneous caller/worker. Owns the
// immutable global index; neither cloud nor index is copied between edges.
// Stack/shell allocations are retained and reused. Same-workspace recursion
// is rejected; simultaneous access is forbidden, not protected by a lock.
class Float32Q3OwnedWorkspace final {
 public:
  explicit Float32Q3OwnedWorkspace(Float32IndexPtr index);
  Float32Q3OwnedWorkspace(const Float32Q3OwnedWorkspace&) = delete;
  Float32Q3OwnedWorkspace& operator=(const Float32Q3OwnedWorkspace&) = delete;
  Float32Q3OwnedWorkspace(Float32Q3OwnedWorkspace&&) = delete;
  Float32Q3OwnedWorkspace& operator=(Float32Q3OwnedWorkspace&&) = delete;
  [[nodiscard]] const Float32IndexPtr& index() const noexcept { return index_; }
  [[nodiscard]] std::size_t retained_bytes() const;

 private:
  struct Frame { std::size_t node{}, depth{}, cursor{}; };
  friend class Float32Q3OwnedEngine;
  friend void run_float32_q3_owned_edge(Float32Q3OwnedWorkspace&, std::size_t, std::size_t,
      const Float32Q3OwnedOptions&, const Float32Q3OwnedConsumer&, Float32Q3OwnedWork&);
  Float32IndexPtr index_;
  std::vector<Frame> pending_;
  std::vector<std::size_t> shell_;
  bool busy_{};
};

// ONE edge, all eligible X nodes of the same index. Each strictly acute
// triangle belongs to its longest edge, ties by SMALLEST sorted original-ID
// pair. Ownership is checked BEFORE individual census, not in its callback.
// All sites remain global witnesses, including obtuse/non-owned seeds.
// Shared children and relays inherit their own frozen (count,cursor) ticket;
// shell starts globally. No arbitrary credits, q4 gating, key or catalogue.
// Invalid arguments/reentrant calls leave work unchanged. Later exceptions
// preserve partial work/emissions, but clear traversal state so this workspace
// is reusable. Borrowed emission/ball/shell live only through the callback.
void run_float32_q3_owned_edge(Float32Q3OwnedWorkspace&, std::size_t a, std::size_t b,
                              const Float32Q3OwnedOptions&, const Float32Q3OwnedConsumer&,
                              Float32Q3OwnedWork&);

}  // namespace mhgp8
