// Independent, bounded audit of the existing v7 index; never a product path.
// Normal: 0 pass, 1 failed check; mutant: 3 rejected, 1 survived; bad CLI: 2.
#include <algorithm>
#include <array>
#include <cstdio>
#include <limits>
#include <map>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "../src/core/caps.hpp"
#include "../src/tree/cloud_index.hpp"

using namespace mhgp7;

namespace {

u64 clouds = 0, internal_nodes = 0, leaves = 0, duplicate_clouds = 0;
u64 permutations = 0, rejected_inputs = 0, max_height = 0;

void require(bool ok, const char* message) {
  if (!ok) throw std::runtime_error(message);
}

// One bit at a time: no product masks, inverse, clz or Karras helpers.
u64 reference_key(const P3& p) {
  const i64 c[3] = {p.x, p.y, p.z};
  u64 key = 0;
  for (int bit = 0; bit < 16; ++bit)
    for (int axis = 0; axis < 3; ++axis)
      if ((c[axis] >> bit) & 1) key += u64{1} << (3 * bit + axis);
  return key;
}

P3 reference_position(u64 key) {
  i64 c[3] = {0, 0, 0};
  for (int bit = 0; bit < 48; ++bit)
    if ((key >> bit) & 1) c[bit % 3] += i64{1} << (bit / 3);
  return {c[0], c[1], c[2]};
}

struct Record {
  P3 position;
  std::vector<PointId> ids;
};

void validate(const CloudIndex& ix, const std::vector<InputPoint>& input) {
  std::map<u64, Record> oracle;
  std::map<PointId, P3> identities;
  for (const auto& p : input) {
    require(identities.emplace(p.id, p.position).second, "fixture duplicate id");
    auto& record = oracle[reference_key(p.position)];
    record.position = p.position;
    record.ids.push_back(p.id);
  }
  for (auto& [key, record] : oracle) {
    (void)key;
    std::sort(record.ids.begin(), record.ids.end());
  }
  const size_t m = oracle.size();
  require(ix.valid, "index validity");
  require(ix.input_count == input.size(), "input cardinal");
  require(ix.keys.size() == m && ix.upos.size() == m, "unique cardinal");
  require(ix.nodes.size() == (m ? m - 1 : 0), "internal cardinal");
  require(ix.bucket_start.size() == m + 1 && ix.wsum.size() == m + 1,
          "CSR cardinal");
  require(ix.bucket_ids.size() == input.size(), "identity cardinal");
  require(ix.bucket_start[0] == 0 && ix.wsum[0] == 0, "CSR origin");
  require(ix.bucket_start.back() == input.size() && ix.wsum.back() == input.size(),
          "CSR terminal");
  require(ix.has_duplicate_positions() == (m != input.size()), "duplicate status");
  size_t u = 0, offset = 0;
  for (const auto& [key, record] : oracle) {
    require(ix.keys[u] == key, "Morton key");
    require(ix.upos[u] == record.position, "position association");
    require(ix.bucket_start[u] == offset && ix.wsum[u] == offset, "CSR boundary");
    require(ix.multiplicity(static_cast<i32>(u)) == record.ids.size(), "multiplicity");
    require(ix.point_id(static_cast<i32>(u)) == record.ids.front(), "representative");
    for (PointId id : record.ids) require(ix.bucket_ids[offset++] == id, "identity membership");
    ++u;
  }
  ++clouds;
  duplicate_clouds += m != input.size();
  if (m == 0) return;  // root() is deliberately outside its nonempty precondition.
  std::vector<u8> seen_nodes(ix.nodes.size()), seen_leaves(m);
  struct Task { NodeRef ref; int first, last, parent; u64 height; };
  std::vector<Task> pending{{ix.root(), 0, static_cast<int>(m) - 1, -1, 0}};
  while (!pending.empty()) {
    const auto task = pending.back();
    pending.pop_back();
    max_height = std::max(max_height, task.height);
    require(task.height <= 48, "radix height");
    if (task.first == task.last) {
      require(task.ref < 0, "leaf reference sign");
      const i64 index = -i64{1} - task.ref;  // do not use product inverse.
      require(index == task.first && index >= 0 && index < static_cast<i64>(m), "leaf identity");
      require(seen_leaves[static_cast<size_t>(index)]++ == 0, "unique leaf reachability");
      ++leaves;
      continue;
    }
    require(task.ref >= 0 && static_cast<size_t>(task.ref) < ix.nodes.size(), "internal reference");
    require(seen_nodes[static_cast<size_t>(task.ref)]++ == 0, "unique internal reachability");
    const auto& node = ix.nodes[static_cast<size_t>(task.ref)];
    require(node.first == task.first && node.last == task.last, "exact parent range");
    require(node.parent == task.parent, "parent link");
    const u64 lo_key = ix.keys[static_cast<size_t>(task.first)];
    const u64 hi_key = ix.keys[static_cast<size_t>(task.last)];
    int bit = 47;
    while (bit >= 0 && ((lo_key >> bit) & 1) == ((hi_key >> bit) & 1)) --bit;
    require(bit >= 0, "distinct endpoint keys");
    int split = task.first;
    while (split + 1 < task.last && ((ix.keys[static_cast<size_t>(split + 1)] >> bit) & 1) == 0) ++split;
    require(((ix.keys[static_cast<size_t>(split)] >> bit) & 1) == 0 &&
            ((ix.keys[static_cast<size_t>(split + 1)] >> bit) & 1) == 1, "oracle binary split");
    pending.push_back({node.left, task.first, split, task.ref, task.height + 1});
    pending.push_back({node.right, split + 1, task.last, task.ref, task.height + 1});
    i64 low[3] = {65535, 65535, 65535}, high[3] = {0, 0, 0};
    for (int point = task.first; point <= task.last; ++point) {
      const auto& p = ix.upos[static_cast<size_t>(point)];
      const i64 c[3] = {p.x, p.y, p.z};
      for (int axis = 0; axis < 3; ++axis) {
        low[axis] = std::min(low[axis], c[axis]);
        high[axis] = std::max(high[axis], c[axis]);
      }
    }
    for (int axis = 0; axis < 3; ++axis) {
      require(node.tlo[axis] == low[axis] && node.thi[axis] == high[axis], "tight box equality");
      require(node.clo[axis] <= low[axis] && high[axis] <= node.chi[axis], "Morton cell containment");
      const int side_bits = 16 - (47 - bit) / 3;
      const i64 side = i64{1} << side_bits;
      require(node.clo[axis] == (low[axis] / side) * side &&
              node.chi[axis] == node.clo[axis] + side - 1, "Morton cell equality");
    }
    require(ix.node_weight(task.ref) == ix.bucket_start[static_cast<size_t>(task.last) + 1] -
                                      ix.bucket_start[static_cast<size_t>(task.first)], "range weight");
    ++internal_nodes;
  }
  require(std::all_of(seen_nodes.begin(), seen_nodes.end(), [](u8 n) { return n == 1; }), "all internals reached");
  require(std::all_of(seen_leaves.begin(), seen_leaves.end(), [](u8 n) { return n == 1; }), "all leaves reached");
  require(ix.root() == (m == 1 ? -1 : 0), "canonical root");
}

std::vector<InputPoint> make_cloud(const std::vector<u64>& keys) {
  std::vector<InputPoint> result;
  for (size_t i = 0; i < keys.size(); ++i)
    result.push_back({static_cast<PointId>(0xFFFFFFFFull - 37 * i), reference_position(keys[i])});
  return result;
}

void audit_cloud(std::vector<InputPoint> input, bool permute = true) {
  const auto original = build_cloud_index(input);
  validate(original, input);
  if (!permute || input.size() < 2) return;
  std::reverse(input.begin(), input.end());
  for (int pass = 0; pass < 2; ++pass) {
    if (pass) {
      std::mt19937_64 random(20260905 + input.size());
      std::shuffle(input.begin(), input.end(), random);
    }
    const auto changed = build_cloud_index(input);
    validate(changed, input);
    require(original.keys == changed.keys && original.upos == changed.upos &&
            original.bucket_start == changed.bucket_start && original.bucket_ids == changed.bucket_ids &&
            original.wsum == changed.wsum, "physical permutation canonical CSR");
    for (size_t i = 0; i < original.nodes.size(); ++i) {
      const auto& a = original.nodes[i];
      const auto& b = changed.nodes[i];
      require(a.left == b.left && a.right == b.right && a.parent == b.parent &&
              a.first == b.first && a.last == b.last, "physical permutation canonical topology");
    }
    ++permutations;
  }
}

int mutant(const std::string& kind) {
  const auto input = make_cloud({0, 1, 2, 3, 4, 5, 6, 7});
  auto ix = build_cloud_index(input);
  validate(ix, input);
  if (kind == "alias-child") ix.nodes[0].right = ix.nodes[0].left;
  else if (kind == "parent-link") ix.nodes[0].parent = 1;
  else if (kind == "omit-leaf") ix.nodes[0].last -= 1;
  else if (kind == "tight-box") ix.nodes[0].tlo[0] += 1;
  else if (kind == "morton-key") ix.keys[0] ^= 1;
  else if (kind == "identity-swap") std::swap(ix.bucket_ids[0], ix.bucket_ids[1]);
  else if (kind == "csr-boundary") ix.bucket_start[1] += 1;
  else return 2;
  try { validate(ix, input); }
  catch (const std::exception& error) {
    std::printf("mutant=%s rejected=%s\n", kind.c_str(), error.what());
    return 3;
  }
  std::fprintf(stderr, "surviving mutant=%s\n", kind.c_str());
  return 1;
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 3 && std::string(argv[1]) == "--mutant") return mutant(argv[2]);
    if (argc != 1) return 2;
    require(std::numeric_limits<int>::digits == 31 && sizeof(NodeRef) == 4 && sizeof(u64) == 8,
            "qualified ABI");
    require(kMaxTreePositions == (u64{1} << 30) - 1, "tree cap");
    u64 morton_checks = 0;
    for (u64 value = 0; value < 65536; ++value) {
      for (int axis = 0; axis < 3; ++axis) {
        i64 c[3] = {0, 0, 0};
        c[axis] = static_cast<i64>(value);
        const P3 p{c[0], c[1], c[2]};
        const u64 key = morton48(p);
        require(key == reference_key(p) && key < (u64{1} << 48), "exhaustive axis Morton");
        for (int j = 0; j < 3; ++j)
          require(morton_axis16(key, j) == static_cast<u64>(c[j]), "exhaustive axis inverse");
        ++morton_checks;
      }
    }
    // Every subset of a 4-bit key universe, including empty and singleton.
    for (u64 mask = 0; mask < (u64{1} << 16); ++mask) {
      std::vector<u64> keys;
      for (int bit = 0; bit < 16; ++bit) if ((mask >> bit) & 1) keys.push_back(static_cast<u64>(bit));
      audit_cloud(make_cloud(keys));
    }
    // Every permutation of eight records; stable IDs remain attached to positions.
    auto small = make_cloud({0, 1, 2, 3, 4, 5, 6, 7});
    std::sort(small.begin(), small.end(), [](const InputPoint& a, const InputPoint& b) { return a.id < b.id; });
    do {
      audit_cloud(small, false);
      ++permutations;
    } while (std::next_permutation(small.begin(), small.end(),
                                  [](const InputPoint& a, const InputPoint& b) { return a.id < b.id; }));
    const u64 all = (u64{1} << 48) - 1;
    for (int bit = 0; bit < 48; ++bit) {
      audit_cloud(make_cloud({0, u64{1} << bit}));
      audit_cloud(make_cloud({all ^ (u64{1} << bit), all}));
    }
    std::vector<u64> comb{0};
    for (int bit = 0; bit < 48; ++bit) comb.push_back(u64{1} << bit);
    audit_cloud(make_cloud(comb));
    for (auto& key : comb) key = all ^ key;
    audit_cloud(make_cloud(comb));
    for (int axis = 0; axis < 3; ++axis) {
      std::vector<u64> line;
      for (u64 value = 0; value < 65536; value += 17) {
        i64 c[3] = {0, 0, 0};
        c[axis] = static_cast<i64>(value);
        line.push_back(reference_key({c[0], c[1], c[2]}));
      }
      audit_cloud(make_cloud(line));
    }
    for (u64 seed : {3ull, 4ull, 20260905ull}) {
      std::mt19937_64 random(seed);
      std::vector<u64> keys;
      for (int i = 0; i < 4096; ++i) keys.push_back(random() & all);
      audit_cloud(make_cloud(keys));
    }
    audit_cloud(make_cloud({0, 0, 0, 1, 1, 2, all, all}));
    audit_cloud(make_cloud({all, all, all, all}));
    for (int axis = 0; axis < 3; ++axis) {
      for (i64 invalid : {-1ll, 65536ll}) {
        i64 c[3] = {0, 0, 0};
        c[axis] = invalid;
        const auto bad = build_cloud_index(std::vector<InputPoint>{{7, {c[0], c[1], c[2]}}});
        require(!bad.valid && bad.keys.empty() && bad.nodes.empty(), "invalid coordinate rejected");
        ++rejected_inputs;
      }
    }
    for (bool same_position : {false, true}) {
      const auto bad = build_cloud_index(std::vector<InputPoint>{{7, {0, 0, 0}}, {7, {same_position ? 0 : 1, 0, 0}}});
      require(!bad.valid && bad.keys.empty() && bad.nodes.empty(), "duplicate identity rejected");
      ++rejected_inputs;
    }
    require(clouds >= 230000 && internal_nodes >= 1500000 && leaves >= 1800000 &&
            permutations >= 170000 && duplicate_clouds >= 6 && rejected_inputs == 8 && max_height == 48,
            "non-vacuity floors");
    std::printf("morton_axis_cases=%llu clouds=%llu internals=%llu leaves=%llu permutations=%llu "
                "duplicate_clouds=%llu rejected_inputs=%llu max_height=%llu\n",
                (unsigned long long)morton_checks, (unsigned long long)clouds, (unsigned long long)internal_nodes,
                (unsigned long long)leaves, (unsigned long long)permutations, (unsigned long long)duplicate_clouds,
                (unsigned long long)rejected_inputs, (unsigned long long)max_height);
    return 0;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "failure=%s clouds=%llu internals=%llu leaves=%llu permutations=%llu max_height=%llu\n",
                 error.what(), (unsigned long long)clouds, (unsigned long long)internal_nodes,
                 (unsigned long long)leaves, (unsigned long long)permutations, (unsigned long long)max_height);
    return 1;
  }
}
