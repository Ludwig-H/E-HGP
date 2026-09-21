#include "lanes/float32_q3_census.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <vector>

namespace mhgp8 {
namespace {
using float32_predicate_detail::count;

struct Frame {
  std::size_t node{}, depth{}, cursor{};
};

class Engine final {
 public:
  Engine(const Float32CloudIndex& index, std::size_t a, std::size_t b,
         const Float32Q3CensusOptions& options,
         const std::function<void(const Float32Q3Emission&)>& emit,
         Float32Q3CensusWork& work)
      : index_(index), a_(a), b_(b), options_(options), threshold_(options.kmax - 1),
        emit_(emit), work_(work) {}

  void run(std::size_t seed_node) {
    const auto& root = index_.nodes()[seed_node];
    count(work_.input_seed_slots, root.last - root.first);
    if (options_.mode == Float32Q3CensusMode::Individual) {
      relay({seed_node, 0, 0});
      return;
    }
    // A binary DFS of a median-split tree has at most h+1 pending frames.
    // This bound follows this owner's constructed depth, not u16 geometry.
    std::vector<Frame> pending;
    pending.reserve(index_.max_depth() + 1);
    work_.stack_capacity_bytes = std::max(work_.stack_capacity_bytes,
        static_cast<std::uint64_t>(pending.capacity() * sizeof(Frame)));
    const auto push = [&](Frame next) {
      if (pending.size() == pending.capacity())
        throw std::logic_error("mhgp8 float32 q3 median DFS depth invariant failed");
      pending.push_back(next);
      work_.peak_pending_frames = std::max(work_.peak_pending_frames,
          static_cast<std::uint64_t>(pending.size()));
    };
    push({seed_node, 0, 0});
    while (!pending.empty()) {
      auto frame = pending.back();
      pending.pop_back();
      count(work_.shared_frames);
      const auto& x = index_.nodes()[frame.node];
      if (x.last - x.first <= options_.relay_sites) {
        relay(frame); // No envelope prepared for a small sharing terminal.
        continue;
      }
      const auto prepared = Float32Q3Block::make(
          index_.points()[a_], index_.points()[b_], x.box, work_.shared_bounds);
      while (frame.cursor != index_.nodes().size()) {
        const auto& z = index_.nodes()[frame.cursor];
        count(work_.shared_witness_visits);
        if (z.leaf()) {
          const auto id = index_.permutation()[z.first];
          if (id == a_ || id == b_) {
            // These two sites lie on EVERY positive support sphere. Other
            // candidate seeds are NOT common boundary sites and stay in Z.
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
          // This witness remains UNCONSUMED. Each child gets exactly the
          // frozen certified prefix. Never exclude all of X from witnesses.
          if (x.leaf()) throw std::logic_error("mhgp8 float32 q3 singleton was not relayed");
          count(work_.shared_splits);
          if (frame.depth != 0) count(work_.shared_children_with_credit, 2);
          push({x.right, frame.depth, frame.cursor});
          push({x.left, frame.depth, frame.cursor});
          break;
        }
      }
      if (frame.cursor == index_.nodes().size() && frame.depth < threshold_)
        relay(frame);
    }
  }

 private:
  void relay(const Frame& ticket) {
    count(work_.relay_blocks);
    const auto& x = index_.nodes()[ticket.node];
    count(work_.relayed_seed_slots, x.last - x.first);
    for (auto rank = x.first; rank != x.last; ++rank) {
      const auto seed = index_.permutation()[rank];
      if (seed == a_ || seed == b_) { count(work_.endpoint_seeds); continue; }
      count(work_.support_candidates);
      const auto ball = Float32Ball::make_q3(
          {index_.points()[a_], index_.points()[b_], index_.points()[seed]},
          Float32PredicateMode::Filtered, work_.supports);
      if (!ball) { count(work_.invalid_supports); continue; }
      count(work_.valid_supports);
      if (ticket.depth != 0) count(work_.relays_with_credit);
      if (ticket.cursor == index_.nodes().size()) count(work_.relays_at_eof);
      const auto bits = index_.points()[seed].bits();
      const auto prepared = Float32Q3Block::make(index_.points()[a_], index_.points()[b_],
          Float32Box3::from_corners(bits, bits), work_.individual_bounds);
      // These copies are essential: one seed's suffix never modifies the
      // ticket used by its siblings, whether it accepts or saturates.
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
          // An exact singleton zero is a contact, not a strict interior.
          count(work_.count_outside_nodes);
          cursor = z.escape;
        } else {
          count(work_.count_splits);
          cursor = z.left;
        }
      }
      if (depth == threshold_) { count(work_.saturated_supports); continue; }
      collect_shell(*ball, prepared, ticket.cursor);
      count(work_.accepted_supports);
      count(work_.callbacks);
      emit_(Float32Q3Emission{seed, depth, *ball, shell_});
    }
  }

  void collect_shell(const Float32Ball& ball, const Float32Q3Block& prepared, std::size_t cursor) {
    // Starts globally even if the count cursor was already EOF. All contacts
    // skipped by the prefix (including a,b) must be recovered here.
    shell_.clear();
    // MUTANT: omit all contacts preceding the count cursor.
    while (cursor != index_.nodes().size()) {
      const auto& node = index_.nodes()[cursor];
      count(work_.shell_node_visits);
      if (node.leaf()) {
        count(work_.shell_point_tests);
        const auto id = index_.permutation()[node.first];
        if (ball.power_sign(index_.points()[id], work_.power) == 0) {
          shell_.push_back(id);
          count(work_.shell_ids);
          work_.peak_shell_capacity_bytes = std::max(work_.peak_shell_capacity_bytes,
              static_cast<std::uint64_t>(shell_.capacity() * sizeof(std::size_t)));
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

  const Float32CloudIndex& index_;
  const std::size_t a_, b_;
  const Float32Q3CensusOptions options_;
  const std::size_t threshold_;
  const std::function<void(const Float32Q3Emission&)>& emit_;
  Float32Q3CensusWork& work_;
  std::vector<std::size_t> shell_;
};

}  // namespace

void run_float32_q3_edge_census(
    Float32IndexPtr index, std::size_t a, std::size_t b, std::size_t seed_node,
    const Float32Q3CensusOptions& options,
    const std::function<void(const Float32Q3Emission&)>& emit,
    Float32Q3CensusWork& work) {
  if (!index || a >= index->points().size() || b >= index->points().size() || a == b ||
      seed_node >= index->nodes().size() || options.kmax < 2 || options.relay_sites == 0 || !emit ||
      (options.mode != Float32Q3CensusMode::Individual && options.mode != Float32Q3CensusMode::SharedPrefix))
    throw std::invalid_argument("mhgp8 invalid float32 q3 edge census arguments");
  count(work.calls);
  Engine(*index, a, b, options, emit, work).run(seed_node);
}

}  // namespace mhgp8
