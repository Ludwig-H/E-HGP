#pragma once

// Host-only: flat copy of an immutable generator index for the portable
// witness filter (gpu/witness_filter.hpp). Ranks, boxes and children are
// copied verbatim; the escape links are not needed by the filter.

#include "witness_filter.hpp"

#include "../gen/pipeline/q2_census.hpp"

#include <limits>
#include <stdexcept>
#include <vector>

namespace mhgp9::gpu {

inline FlatBox flat_box(const gen::Box3& box) {
  return FlatBox{{box.low.x, box.low.y, box.low.z}, {box.high.x, box.high.y, box.high.z}};
}

inline FlatBox flat_point(const gen::Point3& point) { return flat_box(gen::Box3{point, point}); }

inline std::vector<FlatNode> flatten_nodes(const gen::Q2CensusIndex& index) {
  const auto nodes = index.spatial_nodes();
  if (nodes.size() >= absent32 || index.spatial_order().size() >= absent32)
    throw std::overflow_error("mhgp9 gpu flat index exceeds u32 node or rank numbers");
  std::vector<FlatNode> flat(nodes.size());
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    const auto& node = nodes[i];
    const bool leaf = node.left == gen::Q2SpatialNode::absent;
    if (leaf != (node.right == gen::Q2SpatialNode::absent))
      throw std::logic_error("mhgp9 gpu flat index found a node with one child");
    flat[i] = FlatNode{flat_box(node.box), leaf ? absent32 : static_cast<u32>(node.left),
                       leaf ? absent32 : static_cast<u32>(node.right), static_cast<u32>(node.range.first),
                       static_cast<u32>(node.range.last)};
  }
  return flat;
}

}  // namespace mhgp9::gpu
