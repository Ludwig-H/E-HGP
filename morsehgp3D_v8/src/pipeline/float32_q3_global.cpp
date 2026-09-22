#include "pipeline/float32_q3_global.hpp"

#include <algorithm>
#include <stdexcept>

namespace mhgp8 {
namespace {
using float32_predicate_detail::count;

bool reject_edge(const Float32CloudIndex& index, std::size_t a, std::size_t b,
                 std::size_t threshold, Float32Q3GlobalWork& work) {
  count(work.edge_filter_calls);
  // Both endpoints have H=0, so at most n-2 distinct sites can certify the
  // strict citron. This is an impossibility proof, never a search quota.
  if (threshold > index.points().size() - 2) return false;
  const auto geometry = Float32EdgeGeometry::make(index.points()[a], a, index.points()[b], b,
      Float32PredicateMode::Filtered, work.edge_filter);
  std::size_t cursor = 0, credit = 0;
  while (cursor != index.nodes().size()) {
    const auto& node = index.nodes()[cursor];
    count(work.witness_node_visits);
    const int sign = geometry.citron(node.box, work.edge_filter);
    if (sign < 0) {
      count(work.witness_inside_nodes);
      const auto gained = std::min(threshold - credit, node.last - node.first);
      count(work.witness_inside_sites, gained);
      credit += gained;
      if (credit == threshold) { count(work.rejected_edges); return true; }
      cursor = node.escape;
    } else if (sign > 0 || node.leaf()) {
      // A point UNKNOWN would provide no credit, which remains safe. The
      // prepared implementation in fact resolves every singleton exactly.
      count(work.witness_outside_nodes);
      cursor = node.escape;
    } else {
      count(work.witness_splits);
      cursor = node.left;
    }
  }
  return false;
}
} // namespace

void validate_float32_q3_global_options(const Float32Q3GlobalOptions& options) {
  if (options.kmax == 0 || options.separation == 0 || options.relay_sites == 0 ||
      (options.mode != Float32Q3OwnedMode::Individual && options.mode != Float32Q3OwnedMode::SharedPrefix))
    throw std::invalid_argument("mhgp8 invalid float32 q3 global options");
}

void run_float32_q3_global(Float32IndexPtr index, const Float32Q3GlobalOptions& requested,
                           const Float32Q3GlobalConsumer& requested_emit, Float32Q3GlobalWork& work) {
  validate_float32_q3_global_options(requested);
  if (!index || !requested_emit) throw std::invalid_argument("mhgp8 q3 global requires index and callback");
  // Copy callable/options before entry. A caller changing its handles during
  // a callback cannot change this synchronous traversal's context.
  const auto options = requested;
  const auto emit = requested_emit;
  count(work.calls);
  if (options.kmax == 1 || index->points().size() < 3) return;
  Float32Q3OwnedWorkspace workspace(index);
  const Float32Q3OwnedOptions owned_options{options.kmax, options.mode, options.relay_sites};
  const Float32FrontOptions front_options{options.filter ? Float32FrontWitnessMode::MidpointSamples
                                                        : Float32FrontWitnessMode::Disabled};
  std::array<std::size_t, 2> active_edge{};
  // Allocate/type-erase callbacks once for the whole run, not once per edge.
  const std::function<void(const Float32Q3OwnedEmission&)> owned_emit = [&](const Float32Q3OwnedEmission& item) {
    const auto key = Float32BallKey::from_support(item.ball, work.keys);
    work.peak_key_bytes = std::max(work.peak_key_bytes, static_cast<std::uint64_t>(key.capacity_bytes()));
    std::array<std::size_t, 3> support{active_edge[0], active_edge[1], item.seed};
    std::sort(support.begin(), support.end());
    count(work.emitted);
    count(work.shell_ids, item.shell.size());
    emit(Float32Q3GlobalEmission{support, {std::min(active_edge[0], active_edge[1]),
        std::max(active_edge[0], active_edge[1])}, item.depth, key, item.shell});
  };
  const Float32FrontConsumer rectangle = [&](const Float32FrontRectangle& product) {
    count(work.rectangles);
    const auto& left = index->nodes()[product.node_a];
    const auto& right = index->nodes()[product.node_b];
    for (auto i = left.first; i != left.last; ++i) {
      for (auto j = right.first; j != right.last; ++j) {
        const auto a = index->permutation()[i], b = index->permutation()[j];
        count(work.edges);
        if (options.filter && reject_edge(*index, a, b, options.kmax - 1, work)) continue;
        active_edge = {a, b};
        run_float32_q3_owned_edge(workspace, a, b, owned_options, owned_emit, work.owned);
      }
    }
  };
  run_float32_front(index, options.kmax, options.separation, front_options, rectangle, work.front);
}
} // namespace mhgp8
