#pragma once

// Explicit adaptation of q2_pool_bridge_20260914/bridge.hpp, SHA256
// 88a5cbef024e4a27d1d8ff6c6fe015c5f1023ddcd22dbe5ce855d82d097fdebd.
// Only unselected initial singleton roots vary; Pool survivors are unchanged.

// Audit-only integration seam. The builder extracts e3af11a7 and includes
// its UNMODIFIED census translation unit once. This accesses its internal
// engine without adding an API to, or editing, the shared product sources.
#include "pipeline/q2_census.cpp"
#include "node_pool.hpp"

namespace mhgp8::audit_pool {

enum class ResidualMode { Pairwise, Shared };
enum class SmallRootMode { Complement, GlobalIterative, GlobalPairwise };

struct BridgeWork {
  u64 selected_rectangles{}, selected_pairs{}, residual_pairs{}, filtered_pairs{};
  u64 factor_sites{}, selection_tests{}, witness_attempts{}, universal_queries{};
  u64 pool_selected{}, pool_insertions{}, pool_shifted_entries{}, prefix_class_visits{}, q2_axis_terms{};
  u64 factor_read_visits{}, grouping_visits{}, local_b_sites{}, local_b_nodes{};
  u64 cover_visits{}, cover_nodes{}, local_roots{}, selected_anchors{}, original_selected_anchors{};
  u64 plan_peak_bytes{}, query_peak_bytes{};
  double preparation_ms{}, query_build_ms{}, selected_total_ms{};
};

struct BridgeResult {
  WspdQ2CensusResult pipeline;
  BridgeWork pool;
  u64 small_roots{};
};

// Cover a prefix in the balanced tree built over this exact grouped B order.
// Cache once per nonempty credit class, not once per anchor. Population
// ranges partition the prefix; they are never interpreted in global Z order.
inline void prefix_cover(const Q2CensusEngine& engine, std::size_t query,
                         std::size_t end, std::vector<std::size_t>& out,
                         BridgeWork& work) {
  counter_add(work.cover_visits);
  const auto& node = engine.queries.at(query);
  if (node.range.first >= end) return;
  if (node.range.last <= end) {
    out.push_back(query);
    counter_add(work.cover_nodes);
    return;
  }
  if (node.left == Q2SpatialNode::absent)
    throw std::logic_error("audit partial prefix intersects a singleton");
  prefix_cover(engine, node.left, end, out, work);
  prefix_cover(engine, node.right, end, out, work);
}

inline BridgeResult run_bridge(const Q2CensusIndex& index, unsigned k, unsigned s,
                               std::size_t cutoff, ResidualMode mode,
                               const Q2CensusConsumer& consumer,
                               WspdFrontMode front_mode = WspdFrontMode::MidpointSamples,
                               SmallRootMode small_mode = SmallRootMode::Complement) {
  if (k == 0 || k > 10 || s == 0 || !consumer ||
      (mode != ResidualMode::Pairwise && mode != ResidualMode::Shared) ||
      (small_mode != SmallRootMode::Complement && small_mode != SmallRootMode::GlobalIterative &&
       small_mode != SmallRootMode::GlobalPairwise))
    throw std::invalid_argument("invalid audit Pool bridge parameters");
  BridgeResult output;
  if (cutoff == 0) {
    output.pipeline = run_wspd_q2_census(index, k, s, front_mode,
        Q2CensusMode::SharedBlocks, consumer, Q2SiblingMode::Saturating,
        Q2WitnessOrder::ComplementFirst);
    return output;
  }
  const auto started = std::chrono::steady_clock::now();
  {
    Q2CensusEngine engine(index, k, consumer);
    const auto nodes = index.spatial_nodes();
    const auto order = index.spatial_order();
    auto& result = output.pipeline;
    auto& work = output.pool;
    result.front = run_wspd_front(index, k, s, front_mode,
      [&](const WspdRectangle& rectangle) {
        if (rectangle.lane_mask != 1)
          throw std::logic_error("audit Pool bridge requires q2-only front");
        auto a_node = rectangle.a_node, b_node = rectangle.b_node;
        if (nodes[a_node].range.size() > nodes[b_node].range.size())
          std::swap(a_node, b_node);
        const auto a = nodes[a_node].range, b = nodes[b_node].range;
        const auto wide_mass = static_cast<i128>(a.size()) * b.size();
        if (wide_mass > std::numeric_limits<u64>::max())
          throw std::overflow_error("audit product mass exceeds u64");
        const auto mass = static_cast<u64>(wide_mass);
        counter_add(result.input_rectangles);
        counter_add(result.anchor_queries, a.size());
        counter_add(engine.result.work.input_descriptors);
        if (b.size() < cutoff) {
          counter_add(engine.result.candidate_pairs, mass);
          if (b.size() == 1) {
            // A is no larger than B; both are initial singleton factors.
            // No witness has been consumed. Global starts at zero, and
            // includes both endpoints (whose exact H is zero).
            counter_add(output.small_roots);
            if (small_mode == SmallRootMode::GlobalPairwise) {
              engine.pair_task(order[a.first], b.first);  // starts its own root
              return;
            }
            if (small_mode == SmallRootMode::GlobalIterative) {
              engine.root_start(0);
              engine.shared_task<false, false>(order[a.first], b_node, 0, 0);
              return;
            }
          }
          for (auto rank = a.first; rank < a.last; ++rank) {
            engine.root_start(0);
            const Q2CensusEngine::OrderContext context{b_node, nodes[b_node].escape, rank};
            engine.shared_task<true, true>(order[rank], b_node, 0, 0,
                                          Q2SpatialNode::absent, &context);
          }
          return;
        }
        const auto selected_started = std::chrono::steady_clock::now();
        counter_add(work.selected_rectangles);
        counter_add(work.selected_pairs, mass);
        counter_add(work.original_selected_anchors, a.size());
        counter_add(work.factor_sites, a.size() + b.size());
        {
          const audit::NodePoolPlan plan(index, a_node, b_node, k);
          const auto prepared = std::chrono::steady_clock::now();
          work.preparation_ms += milliseconds(selected_started, prepared);
          counter_add(work.residual_pairs, plan.candidate_pairs());
          counter_add(work.filtered_pairs, mass - plan.candidate_pairs());
          counter_add(engine.result.candidate_pairs, plan.candidate_pairs());
          const auto& pw = plan.work();
          counter_add(work.selection_tests, pw.selection_tests);
          counter_add(work.witness_attempts, pw.witness_attempts);
          counter_add(work.universal_queries, pw.predicates.universal_queries);
          counter_add(work.q2_axis_terms, pw.predicates.q2_axis_terms);
          counter_add(work.pool_selected, pw.pool_selected);
          counter_add(work.pool_insertions, pw.pool_insertions);
          counter_add(work.pool_shifted_entries, pw.pool_shifted_entries);
          counter_add(work.prefix_class_visits, pw.prefix_class_visits);
          counter_add(work.factor_read_visits, pw.selection_point_visits + pw.certification_anchor_visits);
          counter_add(work.grouping_visits, pw.group_credit_visits + pw.group_scatter_visits + pw.prefix_anchor_visits);
          work.plan_peak_bytes = std::max(work.plan_peak_bytes, static_cast<u64>(plan.retained_bytes()));
          if (plan.candidate_pairs() != 0) {
            // A local query tree may only use its own b_order. Z remains
            // the complete original global index; no factor is a new cloud.
            engine.b_order = plan.b_order().first(plan.max_prefix());
            engine.shared_queries = {};
            std::array<std::vector<std::size_t>, 10> covers;
            if (mode == ResidualMode::Shared) {
              const auto build_started = std::chrono::steady_clock::now();
              static_cast<void>(engine.build_queries({0, engine.b_order.size()}, 0));
              counter_add(work.local_b_sites, engine.b_order.size());
              counter_add(work.local_b_nodes, engine.queries.size());
              for (unsigned credit = 0; credit < k; ++credit)
                if (plan.a_groups()[credit].size() != 0 && plan.prefix_for_credit(credit) != 0)
                  prefix_cover(engine, 0, plan.prefix_for_credit(credit), covers[credit], work);
              work.query_build_ms += milliseconds(build_started, std::chrono::steady_clock::now());
              u64 bytes = static_cast<u64>(engine.queries.capacity()) * sizeof(Q2CensusEngine::QueryNode);
              for (const auto& cover : covers) counter_add(bytes, cover.capacity() * sizeof(std::size_t));
              work.query_peak_bytes = std::max(work.query_peak_bytes, bytes);
            }
            for (unsigned credit = 0; credit < k; ++credit) {
              const auto prefix = plan.prefix_for_credit(credit);
              if (prefix == 0) continue;
              const auto group = plan.a_groups()[credit];
              for (auto position = group.first; position < group.last; ++position) {
                const auto rank = plan.a_ranks()[position];
                counter_add(work.selected_anchors);
                if (mode == ResidualMode::Pairwise) {
                  for (std::size_t j = 0; j < prefix; ++j) {
                    counter_add(work.local_roots);
                    engine.pair_task(order[rank], j);
                  }
                } else {
                  // B0 is a GLOBAL node, never a local prefix/node ID.
                  // Local siblings must not enter the global sibling lookup.
                  const Q2CensusEngine::OrderContext context{b_node, nodes[b_node].escape, rank};
                  for (const auto query : covers[credit]) {
                    counter_add(work.local_roots);
                    engine.root_start(0);
                    engine.shared_task<false, true>(order[rank], query, 0, 0,
                                                    Q2SpatialNode::absent, &context);
                  }
                }
              }
            }
            engine.b_order = order;
            engine.shared_queries = nodes;
            std::vector<Q2CensusEngine::QueryNode>().swap(engine.queries);
          }
        }
        work.selected_total_ms += milliseconds(selected_started, std::chrono::steady_clock::now());
      }, 1);
    result.census = engine.result;
    result.sibling_work = engine.sibling_work;
    result.order_work = engine.order_work;
    if (result.census.candidate_pairs + work.filtered_pairs != result.front.work.residual_pair_mass[0] ||
        result.census.accepted_pairs > result.census.candidate_pairs ||
        result.census.rejected_pairs != result.census.candidate_pairs - result.census.accepted_pairs)
      throw std::logic_error("audit Pool/census/front masses do not partition");
  }
  output.pipeline.total_ms = milliseconds(started, std::chrono::steady_clock::now());
  output.pipeline.census.total_ms = output.pipeline.total_ms;
  output.pipeline.census.query_index_ms = output.pool.query_build_ms;
  output.pipeline.census.count_ms = output.pipeline.total_ms - output.pool.query_build_ms - output.pipeline.census.payload_ms;
  return output;
}

}  // namespace mhgp8::audit_pool
