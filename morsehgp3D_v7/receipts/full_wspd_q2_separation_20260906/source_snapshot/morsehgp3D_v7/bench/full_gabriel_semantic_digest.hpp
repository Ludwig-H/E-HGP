// Bench-only semantic fingerprints, not an oracle or a completeness certificate.
// FULLv1 wire: all scalars are u64 little-endian except positive denominators
// (u128 LE) and numerators (three u64 LE limbs). Tags are length-prefixed by
// Sha256::tag. Input records are sorted by external PointId. Forest roots and
// children are sorted by their lexicographically least descendant leaf label;
// preorder with node kind/arity encodes topology, not allocation IDs or padding.
// ExactLevel is reduced here: its historical representation is NOT canonical.
#pragma once

#include <algorithm>
#include <array>
#include <limits>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "../src/core/sha256.hpp"
#include "../src/forest/full_certificate.hpp"

namespace mhgp7::full_probe_digest {

inline constexpr const char* kKind = "sha256_FULLv1_semantic_labelled_horizontal_forest";
inline constexpr u64 kScratchBytesPerNode = 3 * sizeof(FullNodeId) + sizeof(u8);

// Binary divmod uses exactly 192 iterations. Before each step r < d and
// d <= 2^127-1, hence 2*r+bit fits u128. No signed shift or truncation.
inline std::array<u64, 3> divmod(const std::array<u64, 3>& n, u128 d, u128& remainder) {
  if (d == 0 || d > static_cast<u128>(std::numeric_limits<i128>::max()))
    throw std::invalid_argument("digest_divisor");
  std::array<u64, 3> q{};
  remainder = 0;
  for (unsigned bit = 192; bit-- > 0;) {
    remainder = (remainder << 1) | ((n[bit / 64] >> (bit % 64)) & 1);
    if (remainder >= d) {
      remainder -= d;
      q[bit / 64] |= u64{1} << (bit % 64);
    }
  }
  return q;
}

inline ExactLevel normalized(const ExactLevel& level) {
  if (level.den <= 0) throw std::invalid_argument("digest_level_denominator");
  const u128 d = static_cast<u128>(level.den);
  ExactLevel result = level;
  if (level.num[2] == 0) {
    const u128 n = (static_cast<u128>(level.num[1]) << 64) | level.num[0];
    const u128 g = ugcd128(n, d);
    const u128 q = n / g;
    result.num[0] = static_cast<u64>(q);
    result.num[1] = static_cast<u64>(q >> 64);
    result.den = static_cast<i128>(d / g);
    return result;
  }
  u128 remainder = 0;
  const std::array<u64, 3> n{level.num[0], level.num[1], level.num[2]};
  (void)divmod(n, d, remainder);
  const u128 g = ugcd128(remainder, d);
  const auto q = divmod(n, g, remainder);
  if (remainder != 0) throw std::logic_error("digest_inexact_reduction");
  for (size_t i = 0; i < 3; ++i) result.num[i] = q[i];
  result.den = static_cast<i128>(d / g);
  return result;
}

inline void level_wire(Sha256& hash, const ExactLevel& level) {
  const ExactLevel r = normalized(level);
  for (u64 limb : r.num) hash.u64le(limb);
  hash.u128le(static_cast<u128>(r.den));
}

inline std::string input(std::span<const InputPoint> points) {
  std::vector<size_t> sorted(points.size());
  std::iota(sorted.begin(), sorted.end(), size_t{0});
  std::sort(sorted.begin(), sorted.end(), [&](size_t a, size_t b) { return points[a].id < points[b].id; });
  Sha256 hash;
  hash.tag("mhgp7-full-semantic-v1:input");
  hash.u64le(points.size());
  for (size_t i = 0; i < sorted.size(); ++i) {
    const InputPoint& p = points[sorted[i]];
    if (!p3_in_profile(p.position) || (i != 0 && points[sorted[i - 1]].id == p.id))
      throw std::invalid_argument("digest_input_domain");
    hash.u64le(p.id);
    hash.u64le(static_cast<u64>(p.position.x));
    hash.u64le(static_cast<u64>(p.position.y));
    hash.u64le(static_cast<u64>(p.position.z));
  }
  return hash.hex();
}

// This is a serializer view, NOT a public untrusted-forest parser. The actual
// probe uses only the opaque factory's successful FullCertificate. Synthetic
// selftests vary allocation IDs, minima-arena order and parent-list order on
// the same valid labelled trees, without bypassing the product factory.
struct View {
  unsigned order;
  std::span<const FullNode> nodes;
  std::span<const FacetKey> minima;
  std::span<const FullNodeId> parents;
};

inline View view(const FullCertificate& forest) {
  return {forest.order(), forest.nodes(), forest.minima(), forest.parents()};
}

inline std::string forest(const View& f, u64 max_nodes, u64 max_parents,
                          const std::string& input_hash) {
  if (f.order < 1 || f.order > kFacetMaxK || f.nodes.empty() || f.minima.empty())
    throw std::invalid_argument("digest_forest_domain");
  if (f.nodes.size() > max_nodes || f.parents.size() > max_parents)
    throw std::length_error("digest_scratch_budget");
  // Exact logical scratch bound: (3*sizeof(ID)+1)*N + sizeof(ID)*P.
  // No capacity/RSS bound is inferred; RLIMIT_AS remains the process guard.
  std::vector<FullNodeId> least(f.nodes.size());
  std::vector<u8> consumed(f.nodes.size(), 0);
  std::vector<FullNodeId> children(f.parents.begin(), f.parents.end());
  std::vector<FullNodeId> roots, stack;
  roots.reserve(f.nodes.size());
  stack.reserve(f.nodes.size());
  for (size_t i = 0; i < f.nodes.size(); ++i) {
    const FullNode& node = f.nodes[i];
    if (node.parent_count == 0) {
      if (node.first >= f.minima.size() || f.minima[node.first].k != f.order)
        throw std::invalid_argument("digest_leaf");
      least[i] = node.first;
    } else {
      if (node.parent_count < 2 || node.first > children.size() ||
          node.parent_count > children.size() - node.first)
        throw std::invalid_argument("digest_parent_range");
      for (u64 j = 0; j < node.parent_count; ++j) {
        const FullNodeId p = children[node.first + j];
        if (p >= i || consumed[p] != 0) throw std::invalid_argument("digest_parent_topology");
        consumed[p] = 1;
      }
      const auto begin = children.begin() + static_cast<std::ptrdiff_t>(node.first);
      const auto end = begin + static_cast<std::ptrdiff_t>(node.parent_count);
      std::sort(begin, end, [&](FullNodeId a, FullNodeId b) { return f.minima[least[a]] < f.minima[least[b]]; });
      least[i] = least[*begin];
    }
  }
  for (size_t i = 0; i < f.nodes.size(); ++i)
    if (consumed[i] == 0) roots.push_back(i);
  std::sort(roots.begin(), roots.end(), [&](FullNodeId a, FullNodeId b) { return f.minima[least[a]] < f.minima[least[b]]; });
  for (auto p = roots.rbegin(); p != roots.rend(); ++p) stack.push_back(*p);
  Sha256 hash;
  hash.tag("mhgp7-full-semantic-v1:forest");
  hash.tag(input_hash.c_str());
  hash.u64le(f.order);
  hash.u64le(f.nodes.size());
  hash.u64le(roots.size());
  u64 visited = 0;
  while (!stack.empty()) {
    const FullNodeId id = stack.back();
    stack.pop_back();
    const FullNode& node = f.nodes[id];
    hash.u64le(node.parent_count == 0 ? 0 : 1);
    level_wire(hash, node.level);
    if (node.parent_count == 0) {
      const FacetKey& leaf = f.minima[node.first];
      hash.u64le(leaf.k);
      for (size_t j = 0; j < leaf.k; ++j) hash.u64le(leaf.p[j]);
    } else {
      hash.u64le(node.parent_count);
      for (u64 j = node.parent_count; j-- > 0;) stack.push_back(children[node.first + j]);
    }
    ++visited;
  }
  if (visited != f.nodes.size()) throw std::logic_error("digest_visit_count");
  return hash.hex();
}

inline std::string orders(const std::string& input_hash, std::span<const std::string> hashes) {
  Sha256 hash;
  hash.tag("mhgp7-full-semantic-v1:horizontal-orders");
  hash.tag(input_hash.c_str());
  hash.u64le(hashes.size());
  for (size_t i = 0; i < hashes.size(); ++i) {
    hash.u64le(i + 1);
    hash.tag(hashes[i].c_str());
  }
  return hash.hex();
}

struct SelftestResult { u64 checks = 0, failures = 0; };

// Strict non-vacuity checked by the caller; no NDEBUG-sensitive assertions.
inline SelftestResult selftest() {
  SelftestResult result;
  const auto check = [&](bool ok) { ++result.checks; if (!ok) ++result.failures; };
  const auto level_hash = [](const ExactLevel& level) { Sha256 h; level_wire(h, level); return h.hex(); };
  check(level_hash({{1, 0, 0}, 2}) == level_hash({{2, 0, 0}, 4}));
  check(level_hash({{0, 0, 0}, 1}) == level_hash({{0, 0, 0}, std::numeric_limits<i128>::max()}));
  check(level_hash({{1, 0, 0}, 2}) != level_hash({{1, 0, 0}, 3}));
  const ExactLevel high{{2, 0, 2}, static_cast<i128>(u128{1} << 126)};
  const ExactLevel high_reduced{{1, 0, 1}, static_cast<i128>(u128{1} << 125)};
  check(level_hash(high) == level_hash(high_reduced));
  const ExactLevel high_den{{0, 0, 1}, std::numeric_limits<i128>::max()};
  check(normalized(high_den) == high_den);
  const ExactLevel maximum{{~u64{0}, ~u64{0}, ~u64{0}}, std::numeric_limits<i128>::max()};
  check(normalized(maximum) == maximum);  // gcd(2^192-1,2^127-1)=2^1-1=1.
  u128 remainder = 0;
  const auto divided = divmod({0, 0, 1}, u128{1} << 126, remainder);
  check(divided == std::array<u64, 3>{4, 0, 0} && remainder == 0);
  const auto unit = divmod({~u64{0}, ~u64{0}, ~u64{0}}, 1, remainder);
  check(unit == std::array<u64, 3>{~u64{0}, ~u64{0}, ~u64{0}} && remainder == 0);
  const auto quotient = divmod({0, 0, 1}, static_cast<u128>(std::numeric_limits<i128>::max()), remainder);
  check(quotient == std::array<u64, 3>{2, 0, 0} && remainder == 2);

  std::vector<InputPoint> points{{10, {1, 2, 3}}, {20, {4, 5, 6}}, {30, {7, 8, 9}}, {40, {10, 11, 12}}};
  const std::string original_input = input(points);
  std::reverse(points.begin(), points.end());
  check(input(points) == original_input);
  ++points[0].position.x;
  check(input(points) != original_input);
  --points[0].position.x;
  ++points[0].id;
  check(input(points) != original_input);
  --points[0].id;

  std::vector<FacetKey> leaves(4);
  for (size_t i = 0; i < 4; ++i) { leaves[i].k = 1; leaves[i].p[0] = static_cast<PointId>((i + 1) * 10); }
  const ExactLevel zero{{0, 0, 0}, 1}, one{{1, 0, 0}, 1}, two{{2, 0, 0}, 1};
  std::vector<FullNode> nodes{{zero, 0, 0}, {zero, 1, 0}, {zero, 2, 0}, {zero, 3, 0},
                            {one, 0, 2}, {one, 2, 2}, {two, 4, 2}};
  std::vector<FullNodeId> parents{0, 1, 2, 3, 4, 5};
  const auto digest = [&](const std::vector<FullNode>& n, const std::vector<FacetKey>& f,
                          const std::vector<FullNodeId>& p) { return forest({1, n, f, p}, 7, 6, original_input); };
  const std::string base = digest(nodes, leaves, parents);
  std::vector<FullNodeId> permuted_parents{1, 0, 3, 2, 5, 4};
  check(digest(nodes, leaves, permuted_parents) == base);
  // Old IDs [0,1,2,3,4,5,6] become [2,3,0,1,5,4,6]. Children remain older.
  std::vector<FullNode> permuted_nodes{{zero, 2, 0}, {zero, 3, 0}, {zero, 0, 0}, {zero, 1, 0},
                                     {one, 0, 2}, {one, 2, 2}, {two, 4, 2}};
  check(digest(permuted_nodes, leaves, parents) == base);
  auto reversed_leaves = leaves;
  std::reverse(reversed_leaves.begin(), reversed_leaves.end());
  auto reversed_nodes = nodes;
  for (size_t i = 0; i < 4; ++i) reversed_nodes[i].first = 3 - i;
  check(digest(reversed_nodes, reversed_leaves, parents) == base);
  auto equivalent = nodes;
  equivalent[4].level = {{2, 0, 0}, 2};
  equivalent[6].level = {{10, 0, 0}, 5};
  check(digest(equivalent, leaves, parents) == base);
  auto changed_leaves = leaves;
  ++changed_leaves[0].p[0];
  check(digest(nodes, changed_leaves, parents) != base);
  auto changed_nodes = nodes;
  changed_nodes[6].level = {{3, 0, 0}, 1};
  check(digest(changed_nodes, leaves, parents) != base);
  const std::vector<FullNodeId> crossed{0, 2, 1, 3, 4, 5};
  check(digest(nodes, leaves, crossed) != base);  // Same counts, U, levels; different hierarchy.
  check(forest({1, nodes, leaves, parents}, 7, 6, "different-input") != base);
  check(orders(original_input, std::array<std::string, 1>{base}) != base);
  check(orders(original_input, std::array<std::string, 2>{base, base}) !=
        orders(original_input, std::array<std::string, 1>{base}));
  // Multi-root serialization must also be independent of arena order.
  check(forest({1, std::span(nodes).first(6), leaves, std::span(parents).first(4)}, 7, 6, original_input) ==
        forest({1, std::span(permuted_nodes).first(6), leaves, std::span(parents).first(4)}, 7, 6, original_input));
  const std::vector<PointId> ids{10, 20, 30, 40};
  const std::vector<FullBatch> batches{{zero, leaves, {}}, {one, {}, {{0, 1}, {2, 3}}}, {two, {}, {{4, 5}}}};
  const FullBuildResult product = build_full_certificate(1, ids, batches, {3, 7, 6});
  check(product.status == FullCertificateStatus::kOk && forest(view(product.value), 7, 6, original_input) == base);
  return result;
}

}  // namespace mhgp7::full_probe_digest
