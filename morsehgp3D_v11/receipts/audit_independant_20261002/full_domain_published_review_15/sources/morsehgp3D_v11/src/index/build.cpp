// Mediane des rangs Morton, boites exactes reunies de bas en haut. Pas de tri ni copie de coordonnees.
#include "index/index.hpp"

#include <algorithm>
#include <array>

namespace mhgp11 {
namespace {

u64 node_count(u32 n, u32 leaf_size) noexcept {
  // Au premier niveau de quotient <=leaf, les plages ont q ou q+1 elements. Si q==leaf,
  // les r plages excedentaires se divisent encore une fois. Un arbre binaire plein a 2L-1 noeuds.
  u64 width = 1;
  while (n / width > leaf_size) width *= 2;
  const u64 extra = n / width == leaf_size ? n % width : 0;
  return 2 * (width + extra) - 1;
}

Result<num::Box> make_box(const std::array<u32, 3>& lo, const std::array<u32, 3>& hi) noexcept {
  auto a = num::Point::make(lo[0], lo[1], lo[2]);
  if (!a.ok()) return a.outcome();
  auto b = num::Point::make(hi[0], hi[1], hi[2]);
  if (!b.ok()) return b.outcome();
  return num::Box::make(a.value(), b.value());
}

Result<num::Box> leaf_box(const Cloud& cloud, u32 begin, u32 end) noexcept {
  std::array<u32, 3> lo{cloud.x()[begin], cloud.y()[begin], cloud.z()[begin]}, hi = lo;
  for (u32 i = begin + 1; i < end; ++i) {
    const std::array<u32, 3> p{cloud.x()[i], cloud.y()[i], cloud.z()[i]};
    for (int j = 0; j < 3; ++j) {
      lo[j] = std::min(lo[j], p[j]);
      hi[j] = std::max(hi[j], p[j]);
    }
  }
  return make_box(lo, hi);
}

Result<num::Box> unite(const num::Box& a, const num::Box& b) noexcept {
  std::array<u32, 3> lo{}, hi{};
  for (int j = 0; j < 3; ++j) {
    lo[j] = std::min(a.lo().coordinates()[j], b.lo().coordinates()[j]);
    hi[j] = std::max(a.hi().coordinates()[j], b.hi().coordinates()[j]);
  }
  return make_box(lo, hi);
}

struct Builder {
  const Cloud& cloud;
  Buffer<index_detail::Node>& nodes;
  u32 leaf_size;
  u64 depth = 0;

  Result<u64> visit(u64 here, u32 begin, u32 end, u64 current_depth) noexcept {
    depth = std::max(depth, current_depth);
    if (end - begin <= leaf_size) {
      auto box = leaf_box(cloud, begin, end);
      if (!box.ok()) return box.outcome();
      nodes[here] = {box.value(), begin, end, here + 1};
      return here + 1;
    }
    const u32 middle = begin + (end - begin) / 2;
    auto right = visit(here + 1, begin, middle, current_depth + 1);
    if (!right.ok()) return right.outcome();
    auto escape = visit(right.value(), middle, end, current_depth + 1);
    if (!escape.ok()) return escape.outcome();
    auto box = unite(nodes[here + 1].box, nodes[right.value()].box);
    if (!box.ok()) return box.outcome();
    nodes[here] = {box.value(), begin, end, escape.value()};
    return escape.value();
  }
};

}  // namespace

Result<GlobalIndex> build_index(Cloud&& cloud, const IndexParams& params, MemoryBudget& budget) noexcept {
  if (params.leaf_size == 0 || params.leaf_size > 256) return fail(Reason::parameter_out_of_range);
  if (cloud.sites() == 0) return fail(Reason::empty_input);
  // n<2^32-1, nodes<=2n-1 : le produit tient dans u64 ; sizeof(Node) est une constante de cette unite.
  static_assert(sizeof(index_detail::Node) <= 128);
  const u64 count = node_count(cloud.sites(), params.leaf_size);
  MHGP11_TRY(budget.admit(count * sizeof(index_detail::Node)));
  Buffer<index_detail::Node> nodes;
  MHGP11_TRY(nodes.allocate(count, budget));
  Builder builder{cloud, nodes, params.leaf_size};
  auto built = builder.visit(0, 0, cloud.sites(), 1);
  if (!built.ok()) return built.outcome();
  return GlobalIndex(std::move(cloud), std::move(nodes), params.leaf_size, builder.depth);
}

}  // namespace mhgp11
