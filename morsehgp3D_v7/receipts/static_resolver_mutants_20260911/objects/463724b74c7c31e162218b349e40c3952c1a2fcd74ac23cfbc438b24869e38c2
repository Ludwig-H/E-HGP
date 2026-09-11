// Compact FULL forest, structural authority only. No geometry or Gamma builder.
// A successful build validates this encoded forest, NOT Gabriel completeness.
#pragma once

#include <algorithm>
#include <cstddef>
#include <limits>
#include <new>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../lanes/keys.hpp"
#include "../lanes/level.hpp"

namespace mhgp7 {

using FullNodeId = u64;
inline constexpr const char* kFullCertificateAuthority = "structural_only";
inline constexpr const char* kFullCertificateSchema = "full_minima_merge_forest_v1";

enum class FullCertificateStatus { kOk, kInvalidInput, kResourceExhausted };

struct FullCertificateLimits {
  u64 max_batches = 0;
  u64 max_nodes = 0;
  u64 max_parent_refs = 0;
};

struct FullBatch {
  ExactLevel level{{0, 0, 0}, 1};
  std::vector<FacetKey> births;  // Strict lexicographic order, zero padding.
  std::vector<std::vector<FullNodeId>> merges;  // Sorted lists, sorted parents.
};

struct FullNode {
  ExactLevel level{{0, 0, 0}, 1};
  u64 first = 0;  // Minimum index for a leaf; CSR parent offset otherwise.
  u64 parent_count = 0;  // Zero for leaves; at least two for merges.
};

class FullCertificate;
struct FullBuildResult;
FullBuildResult build_full_certificate(unsigned order, std::span<const PointId> points,
                                      std::span<const FullBatch> batches,
                                      const FullCertificateLimits& limits);

class FullCertificate {
 public:
  FullCertificate() = default;
  FullCertificate(const FullCertificate&) = delete;
  FullCertificate& operator=(const FullCertificate&) = delete;
  FullCertificate(FullCertificate&& other) noexcept { swap(other); }
  FullCertificate& operator=(FullCertificate&& other) noexcept {
    if (this != &other) {
      FullCertificate fresh(std::move(other));
      swap(fresh);
    }
    return *this;
  }
  unsigned order() const { return order_; }
  const std::vector<FullNode>& nodes() const { return nodes_; }
  const std::vector<FacetKey>& minima() const { return minima_; }
  const std::vector<FullNodeId>& parents() const { return parents_; }

 private:
  friend FullBuildResult build_full_certificate(unsigned, std::span<const PointId>,
      std::span<const FullBatch>, const FullCertificateLimits&);
  void swap(FullCertificate& other) noexcept {
    std::swap(order_, other.order_);
    nodes_.swap(other.nodes_);
    minima_.swap(other.minima_);
    parents_.swap(other.parents_);
  }
  unsigned order_ = 0;  // A default/failed value is not a readable certificate.
  std::vector<FullNode> nodes_;
  std::vector<FacetKey> minima_;
  std::vector<FullNodeId> parents_;
};

struct FullBuildResult {
  FullCertificateStatus status = FullCertificateStatus::kInvalidInput;
  const char* reason = "full_uninitialized";
  FullCertificate value;
};

namespace full_certificate_detail {
inline bool zero(const ExactLevel& level) {
  return level.num[0] == 0 && level.num[1] == 0 && level.num[2] == 0;
}
inline bool add(u64& total, size_t count, u64 cap) {
  if (total > cap || count > cap - total) return false;
  total += static_cast<u64>(count);
  return true;
}
}  // namespace full_certificate_detail

// Canonical order within a batch is an input contract. Node IDs are assigned
// densely: births first, then merges. References always address an older batch.
// All mutable state is private until success; every refusal returns empty arenas.
inline FullBuildResult build_full_certificate(unsigned order, std::span<const PointId> points,
                                             std::span<const FullBatch> batches,
                                             const FullCertificateLimits& limits) {
  const auto reject = [](FullCertificateStatus status, const char* reason) {
    FullBuildResult r;
    r.status = status;
    r.reason = reason;
    return r;
  };
  const auto invalid = [&](const char* reason) {
    return reject(FullCertificateStatus::kInvalidInput, reason);
  };
  const auto exhausted = [&](const char* reason) {
    return reject(FullCertificateStatus::kResourceExhausted, reason);
  };
  if (order < 1 || order > kFacetMaxK || points.size() < order || batches.empty())
    return invalid("full_invalid_domain");
  for (size_t i = 1; i < points.size(); ++i)
    if (points[i - 1] >= points[i]) return invalid("full_point_order");
  if (batches.size() > limits.max_batches) return exhausted("full_batch_budget");

  u64 total_nodes = 0, total_parents = 0, total_minima = 0;
  for (size_t b = 0; b < batches.size(); ++b) {
    const auto& batch = batches[b];
    if (batch.level.den <= 0) return invalid("full_invalid_level");
    if (b != 0 && compare_exact_level(batches[b - 1].level, batch.level) >= 0)
      return invalid("full_nonincreasing_batch");
    if (batch.births.empty() && batch.merges.empty()) return invalid("full_empty_batch");
    if (!full_certificate_detail::add(total_nodes, batch.births.size(), limits.max_nodes) ||
        !full_certificate_detail::add(total_nodes, batch.merges.size(), limits.max_nodes))
      return exhausted("full_node_budget");
    if (!full_certificate_detail::add(total_minima, batch.births.size(), limits.max_nodes))
      return exhausted("full_node_budget");
    for (size_t i = 0; i < batch.births.size(); ++i) {
      const FacetKey& f = batch.births[i];
      if (f.k != order) return invalid("full_minimum_order");
      for (size_t j = 0; j < order; ++j) {
        if ((j != 0 && f.p[j - 1] >= f.p[j]) ||
            !std::binary_search(points.begin(), points.end(), f.p[j]))
          return invalid("full_minimum_points");
      }
      for (size_t j = order; j < f.p.size(); ++j)
        if (f.p[j] != 0) return invalid("full_minimum_padding");
      if (i != 0 && !(batch.births[i - 1] < f)) return invalid("full_minimum_sort");
    }
    for (size_t i = 0; i < batch.merges.size(); ++i) {
      const auto& parents = batch.merges[i];
      if (parents.size() < 2) return invalid("full_not_multifusion");
      if (!full_certificate_detail::add(total_parents, parents.size(), limits.max_parent_refs))
        return exhausted("full_parent_budget");
      for (size_t j = 1; j < parents.size(); ++j)
        if (parents[j - 1] >= parents[j]) return invalid("full_parent_order");
      if (i != 0 && !(batch.merges[i - 1] < parents)) return invalid("full_merge_sort");
    }
    if (order == 1) {
      if (b == 0) {
        if (!full_certificate_detail::zero(batch.level) || !batch.merges.empty() ||
            batch.births.size() != points.size()) return invalid("full_k1_roots");
        for (size_t i = 0; i < points.size(); ++i)
          if (batch.births[i].p[0] != points[i]) return invalid("full_k1_roots");
      } else if (!batch.births.empty()) return invalid("full_k1_late_birth");
    } else if (full_certificate_detail::zero(batch.level)) {
      return invalid("full_positive_level_required");
    }
  }
  if (total_minima == 0) return invalid("full_no_minimum");
  if (total_nodes > std::numeric_limits<size_t>::max() ||
      total_parents > std::numeric_limits<size_t>::max()) return exhausted("full_size_overflow");

  try {
    FullBuildResult result;
    auto& out = result.value;
    out.nodes_.reserve(static_cast<size_t>(total_nodes));
    out.minima_.reserve(static_cast<size_t>(total_minima));
    out.parents_.reserve(static_cast<size_t>(total_parents));
    std::vector<u8> live(static_cast<size_t>(total_nodes), 0);
    for (const auto& batch : batches) {
      const u64 prior_count = out.nodes_.size();
      // Consume only old roots. Marking here also rejects a parent shared by
      // distinct groups of the SAME plateau, before installing any new root.
      for (const auto& parents : batch.merges) {
        for (FullNodeId p : parents) {
          if (p >= prior_count || !live[static_cast<size_t>(p)])
            return invalid("full_parent_not_prebatch_root");
          live[static_cast<size_t>(p)] = 0;
        }
      }
      for (const FacetKey& f : batch.births) {
        live[out.nodes_.size()] = 1;
        out.nodes_.push_back({batch.level, static_cast<u64>(out.minima_.size()), 0});
        out.minima_.push_back(f);
      }
      for (const auto& parents : batch.merges) {
        live[out.nodes_.size()] = 1;
        out.nodes_.push_back({batch.level, static_cast<u64>(out.parents_.size()),
                              static_cast<u64>(parents.size())});
        out.parents_.insert(out.parents_.end(), parents.begin(), parents.end());
      }
    }
    // Duplicate minima across different levels are invalid as well. This
    // temporary index is discarded; the result keeps no redundant facet map.
    auto sorted = out.minima_;
    std::sort(sorted.begin(), sorted.end());
    for (size_t i = 1; i < sorted.size(); ++i)
      if (sorted[i - 1] == sorted[i]) return invalid("full_duplicate_minimum");
    out.order_ = order;
    result.status = FullCertificateStatus::kOk;
    result.reason = "structural_only";
    return result;
  } catch (const std::bad_alloc&) {
    return exhausted("full_allocation_failed");
  } catch (const std::length_error&) {
    return exhausted("full_size_overflow");
  }
}

template <typename T> struct FullReadResult {
  FullCertificateStatus status = FullCertificateStatus::kInvalidInput;
  const char* reason = "full_invalid_read";
  std::vector<T> values;
};

// Queries replay only the encoded forest, not a geometric completeness claim.
inline FullReadResult<FullNodeId> full_certificate_roots_at(
    const FullCertificate& forest, const ExactLevel& cut, bool closed, u64 max_nodes) {
  FullReadResult<FullNodeId> out;
  if (forest.order() == 0 || cut.den <= 0) return out;
  if (forest.nodes().size() > max_nodes) {
    out.status = FullCertificateStatus::kResourceExhausted;
    out.reason = "full_read_node_budget";
    return out;
  }
  try {
    std::vector<u8> live(forest.nodes().size(), 0);
    for (size_t id = 0; id < forest.nodes().size(); ++id) {
      const auto& node = forest.nodes()[id];
      const int cmp = compare_exact_level(node.level, cut);
      if (cmp > 0 || (cmp == 0 && !closed)) break;
      for (u64 j = 0; j < node.parent_count; ++j)
        live[static_cast<size_t>(forest.parents()[static_cast<size_t>(node.first + j)])] = 0;
      live[id] = 1;
    }
    for (size_t id = 0; id < live.size(); ++id)
      if (live[id]) out.values.push_back(static_cast<FullNodeId>(id));
    out.status = FullCertificateStatus::kOk;
    out.reason = "structural_only";
  } catch (const std::bad_alloc&) {
    out.status = FullCertificateStatus::kResourceExhausted;
    out.reason = "full_read_allocation_failed";
    out.values.clear();
  } catch (const std::length_error&) {
    out.status = FullCertificateStatus::kResourceExhausted;
    out.reason = "full_read_size_overflow";
    out.values.clear();
  }
  return out;
}

inline FullReadResult<PointId> full_certificate_coverage(
    const FullCertificate& forest, FullNodeId root, u64 max_nodes, u64 max_point_refs) {
  FullReadResult<PointId> out;
  if (forest.order() == 0 || root >= forest.nodes().size()) return out;
  try {
    std::vector<FullNodeId> pending;
    if (max_nodes == 0) {
      out.status = FullCertificateStatus::kResourceExhausted;
      out.reason = "full_read_node_budget";
      return out;
    }
    pending.push_back(root);
    u64 visited = 0, point_refs = 0;
    while (!pending.empty()) {
      const FullNodeId id = pending.back();
      pending.pop_back();
      ++visited;
      const auto& node = forest.nodes()[static_cast<size_t>(id)];
      if (node.parent_count == 0) {
        const auto& f = forest.minima()[static_cast<size_t>(node.first)];
        if (!full_certificate_detail::add(point_refs, f.k, max_point_refs)) {
          out.status = FullCertificateStatus::kResourceExhausted;
          out.reason = "full_read_point_budget";
          out.values.clear();
          return out;
        }
        out.values.insert(out.values.end(), f.p.begin(), f.p.begin() + f.k);
      } else {
        if (node.parent_count > max_nodes - visited - pending.size()) {
          out.status = FullCertificateStatus::kResourceExhausted;
          out.reason = "full_read_node_budget";
          out.values.clear();
          return out;
        }
        for (u64 j = 0; j < node.parent_count; ++j)
          pending.push_back(forest.parents()[static_cast<size_t>(node.first + j)]);
      }
    }
    std::sort(out.values.begin(), out.values.end());
    out.values.erase(std::unique(out.values.begin(), out.values.end()), out.values.end());
    out.status = FullCertificateStatus::kOk;
    out.reason = "structural_only";
  } catch (const std::bad_alloc&) {
    out.status = FullCertificateStatus::kResourceExhausted;
    out.reason = "full_read_allocation_failed";
    out.values.clear();
  } catch (const std::length_error&) {
    out.status = FullCertificateStatus::kResourceExhausted;
    out.reason = "full_read_size_overflow";
    out.values.clear();
  }
  return out;
}

}  // namespace mhgp7
