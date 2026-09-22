#include "lanes/float32_q3_owned.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

namespace mhgp8 {
namespace {
using float32_predicate_detail::count;

std::size_t capacity_bytes(std::size_t capacity, std::size_t element) {
  if (capacity > std::numeric_limits<std::size_t>::max() / element)
    throw std::overflow_error("mhgp8 float32 owned workspace size overflow");
  return capacity * element;
}
}  // namespace

Float32Q3OwnedWorkspace::Float32Q3OwnedWorkspace(Float32IndexPtr index) : index_(std::move(index)) {
  if (!index_) throw std::invalid_argument("mhgp8 null float32 owned workspace index");
}

std::size_t Float32Q3OwnedWorkspace::retained_bytes() const {
  const auto stack = capacity_bytes(pending_.capacity(), sizeof(Frame));
  const auto shell = capacity_bytes(shell_.capacity(), sizeof(std::size_t));
  if (stack > std::numeric_limits<std::size_t>::max() - shell)
    throw std::overflow_error("mhgp8 float32 owned workspace total size overflow");
  return stack + shell;
}

// Explicit port of the old census's frozen (count,cursor) traversal, NOT a
// call through that API: ownership must be established before point census,
// and all buffers must survive between edges. Neither old product is edited.
class Float32Q3OwnedEngine final {
 public:
  Float32Q3OwnedEngine(Float32Q3OwnedWorkspace& workspace, std::size_t a, std::size_t b,
      const Float32Q3OwnedOptions& options, const Float32Q3OwnedConsumer& emit,
      Float32Q3OwnedWork& work)
      : workspace_(workspace), index_(*workspace.index_), a_(a), b_(b), options_(options),
        threshold_(options.kmax - 1), emit_(emit), work_(work),
        edge_(Float32EdgeGeometry::make(index_.points()[a], a, index_.points()[b], b,
                                       Float32PredicateMode::Filtered, work.selection)) {}

  void run() {
    count(work_.input_seed_slots, index_.points().size());
    // This median owner's depth proves the bound. No u16 depth, population
    // quota or per-edge frontier allocation is substituted for it.
    const auto required = index_.max_depth() + 1;
    if (workspace_.pending_.capacity() < required) {
      workspace_.pending_.reserve(required);
      count(work_.stack_reserves);
    }
    observe();
    push({0, 0, 0});
    while (!workspace_.pending_.empty()) {
      auto frame = workspace_.pending_.back();
      workspace_.pending_.pop_back();
      const auto& x = index_.nodes()[frame.node];
      count(work_.seed_node_visits);
      // A false result certifies absence of an acute owned support, not
      // absence of witness points. Z remains the ENTIRE original index.
      if (!edge_.may_own(x.box, work_.selection)) {
        count(work_.seed_rejected_nodes);
        count(work_.seed_rejected_slots, x.last - x.first);
        continue;
      }
      if (x.last - x.first <= options_.relay_sites) {
        relay(frame);
        continue;
      }
      if (options_.mode == Float32Q3OwnedMode::Individual) {
        split(frame);
        continue;
      }
      count(work_.shared_frames);
      const auto prepared = prepare(x.box, work_.shared_bounds);
      while (frame.cursor != index_.nodes().size()) {
        const auto& z = index_.nodes()[frame.cursor];
        count(work_.shared_witness_visits);
        if (z.leaf()) {
          const auto id = index_.permutation()[z.first];
          if (id == a_ || id == b_) {
            count(work_.shared_endpoint_skips);
            frame.cursor = z.escape;
            continue;
          }
        }
        const int sign = prepared.classify(z.box, work_.shared_bounds);
        if (sign < 0) {
          count(work_.shared_inside_nodes);
          const auto credit = std::min(threshold_ - frame.depth, z.last - z.first);
          frame.depth += credit;
          count(work_.shared_inside_sites, credit);
          frame.cursor = z.escape;
          if (frame.depth == threshold_) {
            count(work_.shared_saturated_blocks);
            count(work_.shared_rejected_seed_slots, x.last - x.first);
            break;
          }
        } else if (sign > 0) {
          count(work_.shared_outside_nodes);
          frame.cursor = z.escape;
        } else if (!z.leaf()) {
          count(work_.shared_witness_splits);
          frame.cursor = z.left;
        } else {
          // This witness is still UNCONSUMED. Descendants receive their
          // own frozen prefix, never the later census of an older sibling.
          split(frame);
          break;
        }
      }
      if (frame.cursor == index_.nodes().size() && frame.depth < threshold_)
        relay(frame);
    }
    observe();
  }

 private:
  using Frame = Float32Q3OwnedWorkspace::Frame;

  void observe() {
    work_.stack_capacity_bytes = std::max(work_.stack_capacity_bytes,
        static_cast<std::uint64_t>(capacity_bytes(workspace_.pending_.capacity(), sizeof(Frame))));
    work_.peak_shell_capacity_bytes = std::max(work_.peak_shell_capacity_bytes,
        static_cast<std::uint64_t>(capacity_bytes(workspace_.shell_.capacity(), sizeof(std::size_t))));
    work_.peak_workspace_bytes = std::max(work_.peak_workspace_bytes,
        static_cast<std::uint64_t>(workspace_.retained_bytes()));
  }

  void push(Frame frame) {
    if (workspace_.pending_.size() == workspace_.pending_.capacity())
      throw std::logic_error("mhgp8 float32 owned median DFS depth invariant failed");
    workspace_.pending_.push_back(frame);
    work_.peak_pending_frames = std::max(work_.peak_pending_frames,
        static_cast<std::uint64_t>(workspace_.pending_.size()));
  }

  void split(const Frame& frame) {
    const auto& x = index_.nodes()[frame.node];
    if (x.leaf()) throw std::logic_error("mhgp8 float32 owned singleton was not relayed");
    count(work_.seed_splits);
    if (frame.depth != 0) count(work_.shared_children_with_credit, 2);
    push({x.right, frame.depth, frame.cursor});
    push({x.left, frame.depth, frame.cursor});
  }

  Float32Q3OwnedBlock prepare(const Float32Box3& box, Float32Q3OwnedBlockWork& work) {
    // Prepared once lazily for this edge; stored inline, no heap. Preparation
    // is charged to the first caller's ledger, not hidden or paid per seed.
    if (!prepared_edge_)
      prepared_edge_.emplace(Float32Q3OwnedBlock::prepare_edge(index_.points()[a_], index_.points()[b_], work));
    return Float32Q3OwnedBlock::make(*prepared_edge_, box, work);
  }

  void relay(const Frame& ticket) {
    count(work_.relay_blocks);
    const auto& x = index_.nodes()[ticket.node];
    count(work_.relayed_seed_slots, x.last - x.first);
    for (auto rank = x.first; rank != x.last; ++rank) {
      const auto seed = index_.permutation()[rank];
      if (seed == a_ || seed == b_) { count(work_.endpoint_seeds); continue; }
      count(work_.owner_candidates);
      if (!edge_.owns(index_.points()[seed], seed, work_.selection)) {
        count(work_.owner_rejections);
        continue;
      }
      count(work_.owned_seeds);
      const auto ball = Float32Ball::make_q3(
          {index_.points()[a_], index_.points()[b_], index_.points()[seed]},
          Float32PredicateMode::Filtered, work_.supports);
      if (!ball) { count(work_.invalid_supports); continue; }
      count(work_.valid_supports);
      if (ticket.depth != 0) count(work_.relays_with_credit);
      if (ticket.cursor == index_.nodes().size()) count(work_.relays_at_eof);
      const auto bits = index_.points()[seed].bits();
      const auto prepared = prepare(Float32Box3::from_corners(bits, bits), work_.individual_bounds);
      auto depth = ticket.depth;
      auto cursor = ticket.cursor;
      while (cursor != index_.nodes().size() && depth != threshold_) {
        const auto& z = index_.nodes()[cursor];
        count(work_.count_node_visits);
        int sign;
        if (z.leaf()) {
          count(work_.count_point_tests);
          sign = ball->power_sign(index_.points()[index_.permutation()[z.first]], work_.power);
        } else {
          sign = prepared.classify(z.box, work_.individual_bounds);
        }
        if (sign < 0) {
          count(work_.count_inside_nodes);
          const auto credit = std::min(threshold_ - depth, z.last - z.first);
          depth += credit;
          count(work_.count_inside_sites, credit);
          cursor = z.escape;
        } else if (sign > 0 || z.leaf()) {
          count(work_.count_outside_nodes);
          cursor = z.escape;
        } else {
          count(work_.count_splits);
          cursor = z.left;
        }
      }
      if (depth == threshold_) { count(work_.saturated_supports); continue; }
      collect_shell(*ball, prepared);
      count(work_.accepted_supports);
      count(work_.callbacks);
      emit_(Float32Q3OwnedEmission{seed, depth, *ball, workspace_.shell_});
    }
  }

  void collect_shell(const Float32Ball& ball, const Float32Q3OwnedBlock& prepared) {
    // Revisit global Z for contacts, including a/b and the certified prefix.
    // A strict inside or outside block cannot contain a contact.
    workspace_.shell_.clear();
    std::size_t cursor = 0;
    while (cursor != index_.nodes().size()) {
      const auto& node = index_.nodes()[cursor];
      count(work_.shell_node_visits);
      if (node.leaf()) {
        count(work_.shell_point_tests);
        const auto id = index_.permutation()[node.first];
        if (ball.power_sign(index_.points()[id], work_.power) == 0) {
          const auto old_capacity = workspace_.shell_.capacity();
          workspace_.shell_.push_back(id);
          if (workspace_.shell_.capacity() != old_capacity) count(work_.shell_growths);
          count(work_.shell_ids);
          observe();
        } else {
          count(work_.shell_excluded_nodes);
        }
        cursor = node.escape;
      } else if (prepared.classify(node.box, work_.individual_bounds) != 0) {
        count(work_.shell_excluded_nodes);
        cursor = node.escape;
      } else {
        count(work_.shell_splits);
        cursor = node.left;
      }
    }
  }

  Float32Q3OwnedWorkspace& workspace_;
  const Float32CloudIndex& index_;
  const std::size_t a_, b_;
  const Float32Q3OwnedOptions options_;
  const std::size_t threshold_;
  const Float32Q3OwnedConsumer& emit_; // Synchronous borrow, never copied per edge.
  Float32Q3OwnedWork& work_;
  const Float32EdgeGeometry edge_;
  std::optional<Float32Q3OwnedBlock::PreparedEdge> prepared_edge_;
};

void run_float32_q3_owned_edge(Float32Q3OwnedWorkspace& workspace, std::size_t a, std::size_t b,
    const Float32Q3OwnedOptions& options, const Float32Q3OwnedConsumer& emit,
    Float32Q3OwnedWork& work) {
  if (workspace.busy_ || a >= workspace.index_->points().size() || b >= workspace.index_->points().size() ||
      a == b || options.kmax < 2 || options.relay_sites == 0 || !emit ||
      (options.mode != Float32Q3OwnedMode::Individual && options.mode != Float32Q3OwnedMode::SharedPrefix))
    throw std::invalid_argument("mhgp8 invalid float32 q3 owned census arguments");
  struct BusyGuard {
    Float32Q3OwnedWorkspace& workspace;
    explicit BusyGuard(Float32Q3OwnedWorkspace& value) : workspace(value) {
      workspace.busy_ = true;
      workspace.pending_.clear();
      workspace.shell_.clear();
    }
    ~BusyGuard() {
      workspace.pending_.clear();
      workspace.shell_.clear();
      workspace.busy_ = false;
    }
  } guard(workspace);
  count(work.calls);
  Float32Q3OwnedEngine(workspace, a, b, options, emit, work).run();
}
}  // namespace mhgp8
