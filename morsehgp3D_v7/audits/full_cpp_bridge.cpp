// Audit adapter: no expected topology, geometry or partition in this process.
// Header resolution is pinned by the compiler dependency file for every build.
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "src/forest/full_certificate.hpp"

namespace {
using namespace mhgp7;

template <typename T> T read() {
  T value{};
  if (!(std::cin >> value)) throw std::runtime_error("audit_truncated_input");
  return value;
}

ExactLevel read_level() {
  ExactLevel value{};
  for (auto& limb : value.num) limb = read<u64>();
  value.den = static_cast<i128>(read<u64>());
  return value;
}

template <typename T> void array(const std::vector<T>& values) {
  std::cout << '[';
  bool comma = false;
  for (const auto value : values) {
    if (comma) std::cout << ',';
    comma = true;
    std::cout << value;
  }
  std::cout << ']';
}

void status(FullCertificateStatus value, const char* reason) {
  std::cout << "\"status\":" << static_cast<int>(value)
            << ",\"reason\":\"" << reason << '\"';
}

template <typename T> void refusal(const FullReadResult<T>& value) {
  std::cout << '{';
  status(value.status, value.reason);
  std::cout << ",\"empty\":" << (value.values.empty() ? "true" : "false") << '}';
}

void one_record() {
  const u64 id = read<u64>();
  const unsigned order = read<unsigned>();
  const size_t point_count = read<size_t>();
  const size_t batch_count = read<size_t>();
  const size_t cut_count = read<size_t>();
  if (point_count > 32 || batch_count > 128 || cut_count > 1024 || order > 10)
    throw std::runtime_error("audit_input_bound");
  std::vector<PointId> points;
  for (size_t i = 0; i < point_count; ++i) points.push_back(read<PointId>());
  std::vector<FullBatch> batches;
  u64 node_count = 0, parent_count = 0;
  for (size_t b = 0; b < batch_count; ++b) {
    FullBatch batch;
    batch.level = read_level();
    const size_t births = read<size_t>(), merges = read<size_t>();
    if (births + merges > 128) throw std::runtime_error("audit_batch_bound");
    node_count += births + merges;
    for (size_t j = 0; j < births; ++j) {
      FacetKey facet;
      facet.k = static_cast<u8>(order);
      for (unsigned i = 0; i < order; ++i) facet.p[i] = read<PointId>();
      batch.births.push_back(facet);
    }
    for (size_t j = 0; j < merges; ++j) {
      const size_t count = read<size_t>();
      if (count > 128) throw std::runtime_error("audit_parents_bound");
      std::vector<FullNodeId> parents;
      for (size_t i = 0; i < count; ++i) parents.push_back(read<FullNodeId>());
      parent_count += count;
      batch.merges.push_back(std::move(parents));
    }
    batches.push_back(std::move(batch));
  }
  if (node_count == 0 || node_count > 256)
    throw std::runtime_error("audit_nodes_bound");
  std::vector<std::pair<u64, u64>> read_caps;
  for (u64 i = 0; i < node_count; ++i) {
    const u64 nodes = read<u64>(), refs = read<u64>();
    if (nodes == 0 || refs == 0) throw std::runtime_error("audit_caps_bound");
    read_caps.emplace_back(nodes, refs);
  }
  const FullCertificateLimits limits{batch_count, node_count, parent_count};
  const auto built = build_full_certificate(order, points, batches, limits);
  std::cout << "{\"id\":" << id << ",\"order\":" << order << ',';
  status(built.status, built.reason);
  const auto& forest = built.value;
  std::cout << ",\"nodes\":[";
  for (size_t i = 0; i < forest.nodes().size(); ++i) {
    if (i) std::cout << ',';
    const auto& node = forest.nodes()[i];
    std::cout << "{\"num\":[" << node.level.num[0] << ',' << node.level.num[1]
              << ',' << node.level.num[2] << "],\"den\":" << static_cast<u64>(node.level.den)
              << ",\"first\":" << node.first << ",\"parent_count\":" << node.parent_count << '}';
  }
  std::cout << "],\"minima\":[";
  for (size_t i = 0; i < forest.minima().size(); ++i) {
    if (i) std::cout << ',';
    const auto& f = forest.minima()[i];
    array(std::vector<PointId>(f.p.begin(), f.p.begin() + f.k));
  }
  std::cout << "],\"parents\":";
  array(forest.parents());
  std::cout << ",\"coverage\":[";
  for (u64 i = 0; i < node_count; ++i) {
    if (i) std::cout << ',';
    const auto [nodes, refs] = read_caps[static_cast<size_t>(i)];
    const auto covered = full_certificate_coverage(forest, i, nodes, refs);
    std::cout << "{\"node\":" << i << ',';
    status(covered.status, covered.reason);
    std::cout << ",\"values\":";
    array(covered.values);
    std::cout << ",\"under_nodes\":";
    refusal(full_certificate_coverage(forest, i, nodes - 1, refs));
    std::cout << ",\"under_points\":";
    refusal(full_certificate_coverage(forest, i, nodes, refs - 1));
    std::cout << '}';
  }
  std::cout << "],\"cuts\":[";
  for (size_t i = 0; i < cut_count; ++i) {
    const ExactLevel cut = read_level();
    const bool closed = read<unsigned>() != 0;
    const auto roots = full_certificate_roots_at(forest, cut, closed, node_count);
    if (i) std::cout << ',';
    std::cout << '{';
    status(roots.status, roots.reason);
    std::cout << ",\"roots\":";
    array(roots.values);
    std::cout << '}';
  }
  const auto rejected = build_full_certificate(
      order, points, batches, {batch_count, node_count - 1, parent_count});
  const auto& rejected_value = rejected.value;
  std::cout << "],\"build_under_nodes\":{";
  status(rejected.status, rejected.reason);
  std::cout << ",\"empty\":"
            << (rejected_value.order() == 0 && rejected_value.nodes().empty() &&
                rejected_value.minima().empty() && rejected_value.parents().empty() ? "true" : "false")
            << "}}";
}
}  // namespace

int main() {
  try {
    if (read<std::string>() != "FULLCPP1") throw std::runtime_error("audit_magic");
    const size_t count = read<size_t>();
    if (count > 256) throw std::runtime_error("audit_records_bound");
    std::cout << "{\"records\":[";
    for (size_t i = 0; i < count; ++i) {
      if (i) std::cout << ',';
      one_record();
    }
    std::string trailing;
    if (std::cin >> trailing) throw std::runtime_error("audit_trailing_input");
    std::cout << "]}\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 2;
  }
}
