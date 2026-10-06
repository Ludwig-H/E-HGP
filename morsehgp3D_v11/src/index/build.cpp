// Arbre radix de Morton (Karras) : chaque plage est coupee au bit de Morton le plus haut qui differe entre son premier
// et son dernier site (levier V3, 6 octobre 2026). Boites exactes reunies de bas en haut. Pas de tri ni copie de
// coordonnees : le Cloud est deja trie par cle croissante, sans site repete.
#include "index/index.hpp"

#include <algorithm>
#include <array>

namespace mhgp11 {
namespace {

int highest_bit(MortonKey bits) noexcept {  // bits != 0
  if constexpr (sizeof(MortonKey) > sizeof(u64)) {
    const u64 high = static_cast<u64>(bits >> 64);
    if (high != 0) return 127 - __builtin_clzll(high);
  }
  return 63 - __builtin_clzll(static_cast<u64>(bits));
}

// Coupe radix de [begin,end), end-begin>=2. Les cles sont croissantes et distinctes (un site par position) : celles
// de la plage partagent les bits au-dessus du plus haut bit qui differe entre les extremites, et les sites dont ce bit
// vaut 1 forment un suffixe non vide qui ne contient pas begin. Ce bit est le bit level de la coordonnee axis
// (morton.hpp : bit i de x en 3i, de y en 3i+1, de z en 3i+2) : la recherche lit cette coordonnee, sans cle.
u32 radix_split(const Cloud& cloud, u32 begin, u32 end) noexcept {
  const u32 last = end - 1;
  const MortonKey first_key = morton_key(cloud.x()[begin], cloud.y()[begin], cloud.z()[begin]);
  const MortonKey last_key = morton_key(cloud.x()[last], cloud.y()[last], cloud.z()[last]);
  const int bit = highest_bit(first_key ^ last_key);
  const int axis = bit % 3, level = bit / 3;
  const auto coordinates = axis == 0 ? cloud.x() : axis == 1 ? cloud.y() : cloud.z();
  u32 lo = begin + 1, hi = last;  // bit nul en begin, bit a 1 en last
  while (lo < hi) {
    const u32 middle = lo + (hi - lo) / 2;
    if ((coordinates[middle] >> level) & 1u) hi = middle; else lo = middle + 1;
  }
  return lo;
}

// Premier passage du protocole compter, reserver, remplir : memes coupes que Builder::visit. Chaque niveau fixe au
// moins un bit de plus du prefixe commun, donc la recursion a au plus kMortonBits+1 cadres.
u64 node_count(const Cloud& cloud, u32 begin, u32 end, u32 leaf_size) noexcept {
  if (end - begin <= leaf_size) return 1;
  const u32 split = radix_split(cloud, begin, end);
  return 1 + node_count(cloud, begin, split, leaf_size) + node_count(cloud, split, end, leaf_size);
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
    // Memes coupes que node_count : aucun noeud au-dela du compte reserve, garde defensive avant toute ecriture.
    if (here >= nodes.size()) return fail(Reason::arithmetic_invariant);
    depth = std::max(depth, current_depth);
    if (end - begin <= leaf_size) {
      auto box = leaf_box(cloud, begin, end);
      if (!box.ok()) return box.outcome();
      nodes[here] = {box.value(), begin, end, here + 1};
      return here + 1;
    }
    const u32 middle = radix_split(cloud, begin, end);
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
  // n<2^32-1, nodes<=2n-1 (deux enfants non vides par noeud interne) : le produit tient dans u64 ; sizeof(Node) est
  // une constante de cette unite.
  static_assert(sizeof(index_detail::Node) <= 128);
  const u64 count = node_count(cloud, 0, cloud.sites(), params.leaf_size);
  MHGP11_TRY(budget.admit(count * sizeof(index_detail::Node)));
  Buffer<index_detail::Node> nodes;
  MHGP11_TRY(nodes.allocate(count, budget));
  Builder builder{cloud, nodes, params.leaf_size};
  auto built = builder.visit(0, 0, cloud.sites(), 1);
  if (!built.ok()) return built.outcome();
  if (built.value() != count) return fail(Reason::arithmetic_invariant);
  return GlobalIndex(std::move(cloud), std::move(nodes), params.leaf_size, builder.depth);
}

}  // namespace mhgp11
