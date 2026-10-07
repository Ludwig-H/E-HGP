// Petits fichiers synthetiques seulement : aucune donnee reelle ni parcours GPU.
#include <cstdio>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include "mhgp12/traversal/compare.hpp"

namespace f = mhgp12::traversal::format;
namespace t = mhgp12::traversal;

void check(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}

f::Dump one(f::u32 n = 1) {
  f::Dump d;
  auto& h = d.header;
  h.producer = f::kProducerOracle;
  h.coord_bits = 21;
  h.kmax = 1;
  h.leaf_size = 4;
  h.max_leaf = 8;
  h.n_sites = n;
  h.n_nodes = h.nodes = 1;
  h.n_leaves = h.leaves = 1;
  h.n_leaf_sites = n;
  h.filter_tests = n;
  h.max_leaf_seen = n;
  for (f::u32 i = 0; i < n; ++i) {
    d.x.push_back(i); d.y.push_back(0); d.z.push_back(0); d.sites.push_back(i);
  }
  f::Node node;
  node.hi[0] = n; node.hi[1] = node.hi[2] = 1;
  node.candidates = node.count = n;
  node.kind = f::kKindLeaf;
  node.tests = n;
  node.list_fnv = f::list_fnv(d.sites.data(), n);
  d.nodes.push_back(node);
  f::Leaf leaf;
  leaf.m = n;
  leaf.hi[0] = n; leaf.hi[1] = leaf.hi[2] = 1;
  d.leaves.push_back(leaf);
  return d;
}

void read_case(const std::string& folder, const std::string& name, const f::Dump& d, bool accepted) {
  const std::string path = folder + "/" + name + ".bin";
  std::string error;
  check(f::write(path, d, error), "fixture write failed");
  f::Dump read;
  const bool ok = f::read(path, read, error);
  check(ok == accepted, (name + ": unexpected reader outcome").c_str());
  std::cout << "read " << name << " " << ok << "\n";
}

int main(int argc, char** argv) {
  check(argc == 2, "temporary directory required");
  const std::string folder = argv[1];
  const auto valid = one();
  read_case(folder, "valid_one", valid, true);
  auto d = valid;
  d.sites[0] = 1;
  d.nodes[0].list_fnv = f::list_fnv(d.sites.data(), 1);
  read_case(folder, "site_oob", d, false);
  d = valid; d.header.kmax = 0;
  read_case(folder, "k_zero", d, false);
  d = valid; d.x[0] = 1u << 21;
  read_case(folder, "coordinate_oob", d, false);
  d = valid; d.leaves[0].begin = std::numeric_limits<f::u64>::max();
  read_case(folder, "begin_wrap", d, false);
  d = valid; d.header.filter_tests = 2;
  read_case(folder, "ledger_mismatch", d, false);
  d = valid; d.header.status = f::kStatusWideLeaf;
  read_case(folder, "refusal_with_prefix", d, false);
  // Semantic gaps: bound of a node's test count, leaf stopping predicate, root envelope.
  d = valid; d.nodes[0].tests = d.header.filter_tests = std::numeric_limits<f::u64>::max();
  read_case(folder, "impossible_test_count", d, true);
  d = one(5);
  read_case(folder, "nonterminal_leaf", d, true);
  d = valid; d.nodes[0].hi[0] = 2;
  read_case(folder, "wrong_root_envelope", d, true);
  // A well-formed three-node topology whose exact sum of tests exceeds u64.
  d = one(5);
  d.nodes[0].kind = f::kKindSplit;
  d.nodes[0].tests = std::numeric_limits<f::u64>::max();
  auto left = d.nodes[0], right = left;
  left.kind = right.kind = f::kKindLeaf;
  left.depth = right.depth = 1;
  left.count = 2; right.count = 3;
  left.hi[0] = right.lo[0] = 2;
  right.path[0] = f::u64{1} << 63;
  left.tests = right.tests = 2;
  left.list_fnv = f::list_fnv(d.sites.data(), 2);
  right.list_fnv = f::list_fnv(d.sites.data() + 2, 3);
  d.nodes.push_back(left); d.nodes.push_back(right);
  auto a = d.leaves[0], b = a;
  a.depth = b.depth = 1;
  a.m = 2; b.m = 3; b.begin = 2;
  a.hi[0] = b.lo[0] = 2;
  b.path[0] = f::u64{1} << 63;
  d.leaves = {a, b};
  d.header.nodes = d.header.n_nodes = 3;
  d.header.leaves = d.header.n_leaves = 2;
  d.header.max_depth = 1; d.header.max_leaf_seen = 3;
  d.header.filter_tests = 3;
  read_case(folder, "wrapped_test_sum", d, true);
  // Huge announced sections, tiny real file: refusal before vector allocation.
  const std::string short_path = folder + "/short_count.bin";
  auto header = valid.header;
  header.n_nodes = f::u64{1} << 40;
  std::FILE* file = std::fopen(short_path.c_str(), "wb");
  check(file != nullptr, "open short fixture");
  check(std::fwrite(&header, sizeof(header), 1, file) == 1, "write header");
  f::u64 tail = 0;
  check(std::fwrite(&tail, sizeof(tail), 1, file) == 1 && std::fclose(file) == 0, "write tail");
  f::Dump small; std::string error;
  check(!f::read(short_path, small, error) && small.x.empty() && small.nodes.empty(), "size must refuse before vectors");
  std::cout << "read short_count 0\n";
  // Exact comparison independently of digest: range, list, metadata, ledger, duplicate boxes.
  t::Ledger ledger{1, 1, 1, 0, 1};
  auto mine = valid;
  check(t::compare(valid, 0, ledger, t::leaf_set(mine)).identity(), "valid comparison");
  std::cout << "compare valid 1\n";
  mine.sites[0] = 7;
  check(!t::compare(valid, 0, ledger, t::leaf_set(mine)).identity(), "list mismatch");
  std::cout << "compare list 0\n";
  mine = valid; mine.leaves[0].begin = std::numeric_limits<f::u64>::max();
  check(!t::compare(valid, 0, ledger, t::leaf_set(mine)).identity(), "range mismatch");
  std::cout << "compare range 0\n";
  mine = valid; mine.leaves[0].path[0] = 1;
  check(!t::compare(valid, 0, ledger, t::leaf_set(mine)).identity(), "metadata mismatch");
  std::cout << "compare metadata 0\n";
  mine = valid; mine.leaves.push_back(mine.leaves[0]);
  check(!t::compare(valid, 0, ledger, t::leaf_set(mine)).identity(), "duplicate box");
  std::cout << "compare duplicate_box 0\n";
  mine = valid; ++ledger.filter_tests;
  check(!t::compare(valid, 0, ledger, t::leaf_set(mine)).identity(), "ledger mismatch");
  std::cout << "compare ledger 0\n";
}
