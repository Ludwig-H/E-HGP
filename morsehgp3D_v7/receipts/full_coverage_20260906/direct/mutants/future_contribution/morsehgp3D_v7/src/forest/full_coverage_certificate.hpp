// Dated FULL coverage journal, structural authority ONLY. This is not the
// regular minima certificate v1, a geometric producer, or a completeness proof.
#pragma once

#include <memory>

#include "/workspaces/E-HGP/morsehgp3D_v7/src/forest/full_certificate.hpp"

namespace mhgp7 {

inline constexpr const char* kFullCoverageSchema = "full_dated_coverage_forest_v2";
inline constexpr FullNodeId kFullCoverageAbsent = std::numeric_limits<FullNodeId>::max();

// Stored once across the orders sharing this bank. Point identifiers, not
// coordinates: association with a certified ball/census belongs to the producer.
struct FullCoveragePopulation {
  std::vector<PointId> interior, shell;
};
class FullCoveragePopulations;
struct FullCoveragePopulationResult {
  FullCertificateStatus status = FullCertificateStatus::kInvalidInput;
  const char* reason = "coverage_invalid_population";
  std::shared_ptr<const FullCoveragePopulations> value;
};
FullCoveragePopulationResult build_full_coverage_populations(
    std::span<const PointId>, std::span<const FullCoveragePopulation>);

class FullCoveragePopulations {
 public:
  FullCoveragePopulations() = default;
  FullCoveragePopulations(const FullCoveragePopulations&) = delete;
  FullCoveragePopulations& operator=(const FullCoveragePopulations&) = delete;
  FullCoveragePopulations(FullCoveragePopulations&&) = delete;
  FullCoveragePopulations& operator=(FullCoveragePopulations&&) = delete;
  const std::vector<PointId>& domain() const { return domain_; }
  const std::vector<FullCoveragePopulation>& rows() const { return rows_; }
 private:
  friend FullCoveragePopulationResult build_full_coverage_populations(
      std::span<const PointId>, std::span<const FullCoveragePopulation>);
  std::vector<PointId> domain_;
  std::vector<FullCoveragePopulation> rows_;
};

inline FullCoveragePopulationResult build_full_coverage_populations(
    std::span<const PointId> domain, std::span<const FullCoveragePopulation> rows) {
  FullCoveragePopulationResult result;
  if (domain.empty() || rows.empty()) return result;
  const auto ordered = [](const auto& points) {
    for (size_t i = 1; i < points.size(); ++i)
      if (points[i - 1] >= points[i]) return false;
    return true;
  };
  if (!ordered(domain)) return result;
  for (const auto& row : rows) {
    // A representation bound of the mask, NOT a cloud/work/time ceiling.
    if (row.shell.size() > std::numeric_limits<u16>::digits ||
        (row.interior.empty() && row.shell.empty()) ||
        !ordered(row.interior) || !ordered(row.shell)) return result;
    for (const auto* points : {&row.interior, &row.shell})
      for (PointId p : *points)
        if (!std::binary_search(domain.begin(), domain.end(), p)) return result;
    for (PointId p : row.shell)
      if (std::binary_search(row.interior.begin(), row.interior.end(), p)) return result;
  }
  try {
    auto bank = std::make_shared<FullCoveragePopulations>();
    bank->domain_.assign(domain.begin(), domain.end());
    bank->rows_.assign(rows.begin(), rows.end());
    result.value = std::move(bank);
    result.status = FullCertificateStatus::kOk;
    result.reason = "structural_only";
  } catch (const std::bad_alloc&) {
    result.status = FullCertificateStatus::kResourceExhausted;
    result.reason = "coverage_allocation_failed";
  } catch (const std::length_error&) {
    result.status = FullCertificateStatus::kResourceExhausted;
    result.reason = "coverage_size_overflow";
  }
  return result;
}

struct FullCoverageRef {
  u64 population = 0;
  u16 shell_mask = 0;
  bool include_interior = false;
};
struct FullCoverageAction {
  // Empty: birth. One: continuation, NO new node. Two or more: multifusion.
  std::vector<FullNodeId> parents;
  std::vector<FullCoverageRef> contributions;
};
struct FullCoverageBatch {
  ExactLevel level{{0, 0, 0}, 1};
  std::vector<FullCoverageAction> actions;
};
struct FullDatedContribution {
  ExactLevel level;
  FullNodeId segment;
  FullCoverageRef ref;
};
class FullCoverageCertificate;
struct FullCoverageBuildResult;
FullCoverageBuildResult build_full_coverage_certificate(unsigned,
    std::shared_ptr<const FullCoveragePopulations>, std::span<const FullCoverageBatch>);

class FullCoverageCertificate {
 public:
  FullCoverageCertificate() = default;
  FullCoverageCertificate(const FullCoverageCertificate&) = delete;
  FullCoverageCertificate& operator=(const FullCoverageCertificate&) = delete;
  FullCoverageCertificate(FullCoverageCertificate&& other) noexcept { swap(other); }
  FullCoverageCertificate& operator=(FullCoverageCertificate&& other) noexcept {
    if (this != &other) { FullCoverageCertificate fresh(std::move(other)); swap(fresh); }
    return *this;
  }
  unsigned order() const { return order_; }
  const auto& populations() const { return populations_; }
  const auto& nodes() const { return nodes_; }
  const auto& parents() const { return parents_; }
  const auto& successors() const { return successors_; }
  const auto& contributions() const { return contributions_; }
 private:
  friend FullCoverageBuildResult build_full_coverage_certificate(unsigned,
      std::shared_ptr<const FullCoveragePopulations>, std::span<const FullCoverageBatch>);
  void swap(FullCoverageCertificate& other) noexcept {
    std::swap(order_, other.order_);
    populations_.swap(other.populations_);
    nodes_.swap(other.nodes_); parents_.swap(other.parents_);
    successors_.swap(other.successors_); contributions_.swap(other.contributions_);
  }
  unsigned order_ = 0;
  std::shared_ptr<const FullCoveragePopulations> populations_;
  std::vector<FullNode> nodes_;  // first is always a CSR offset, including births.
  std::vector<FullNodeId> parents_, successors_;
  std::vector<FullDatedContribution> contributions_;
};
struct FullCoverageBuildResult {
  FullCertificateStatus status = FullCertificateStatus::kInvalidInput;
  const char* reason = "coverage_invalid_input";
  FullCoverageCertificate value;
};

namespace full_coverage_detail {
inline u16 all_shell(const FullCoveragePopulation& row) {
  return static_cast<u16>((u32{1} << row.shell.size()) - 1);
}
inline bool admitted(const ExactLevel& level, const ExactLevel& cut, bool closed) {
  const int cmp = compare_exact_level(level, cut);
  return cmp < 0 || (closed && cmp == 0);
}
}  // namespace full_coverage_detail

// Input actions are already grouped across all balls of an exact level by the
// producer. Distinct empty-parent actions remain distinct births. IDs follow
// action order, skipping continuations. No root point-set is materialized here.
inline FullCoverageBuildResult build_full_coverage_certificate(unsigned order,
    std::shared_ptr<const FullCoveragePopulations> bank,
    std::span<const FullCoverageBatch> batches) {
  const auto invalid = [](const char* reason) {
    FullCoverageBuildResult result; result.reason = reason; return result;
  };
  if (order < 1 || order > kFacetMaxK || !bank || bank->rows().empty() ||
      bank->domain().size() < order || batches.empty()) return invalid("coverage_invalid_domain");
  try {
    FullCoverageBuildResult result;
    auto& out = result.value;
    std::vector<u8> live;
    for (size_t b = 0; b < batches.size(); ++b) {
      const auto& batch = batches[b];
      if (batch.level.den <= 0) return invalid("coverage_invalid_level");
      if (b && compare_exact_level(batches[b - 1].level, batch.level) >= 0)
        return invalid("coverage_nonincreasing_batch");
      if (batch.actions.empty()) return invalid("coverage_empty_batch");
      if (order > 1 && full_certificate_detail::zero(batch.level))
        return invalid("coverage_positive_level_required");
      if (order == 1 && b == 0 && (!full_certificate_detail::zero(batch.level) ||
          batch.actions.size() != bank->domain().size())) return invalid("coverage_k1_roots");
      const size_t prior_count = out.nodes_.size();
      // Consume/check every prelot root BEFORE creating any postlot root. Also
      // rejects two ungrouped actions that share a parent at the same level.
      for (size_t a = 0; a < batch.actions.size(); ++a) {
        const auto& action = batch.actions[a];
        for (size_t j = 0; j < action.parents.size(); ++j) {
          const auto parent = action.parents[j];
          if ((j && action.parents[j - 1] >= parent) || parent >= prior_count || !live[parent])
            return invalid("coverage_parent_not_unique_prebatch_root");
          live[parent] = 0;
        }
        if (action.parents.size() == 1 && action.contributions.empty())
          return invalid("coverage_empty_continuation");
        for (const auto& ref : action.contributions) {
          if (ref.population >= bank->rows().size()) return invalid("coverage_population_reference");
          const auto& row = bank->rows()[ref.population];
          if ((ref.shell_mask & ~full_coverage_detail::all_shell(row)) ||
              (ref.include_interior && row.interior.empty()) ||
              (!ref.include_interior && ref.shell_mask == 0)) return invalid("coverage_empty_or_invalid_mask");
        }
        if (action.parents.empty()) {
          if (action.contributions.size() != 1) return invalid("coverage_birth_population");
          const auto& ref = action.contributions.front();
          const auto& row = bank->rows()[ref.population];
          if (ref.include_interior != !row.interior.empty() ||
              ref.shell_mask != full_coverage_detail::all_shell(row) ||
              row.interior.size() + row.shell.size() < order) return invalid("coverage_birth_population");
          if (order == 1) {
            if (b || row.interior.size() + row.shell.size() != 1)
              return invalid("coverage_k1_roots");
            const auto id = row.interior.empty() ? row.shell.front() : row.interior.front();
            if (id != bank->domain()[a]) return invalid("coverage_k1_roots");
          }
        }
      }
      for (const auto& action : batch.actions) {
        FullNodeId segment;
        if (action.parents.size() == 1) {
          segment = action.parents.front();
          live[segment] = 1;  // continuation retains component identity
        } else {
          if (out.nodes_.size() == kFullCoverageAbsent)
            throw std::length_error("coverage node identifiers exhausted");
          segment = out.nodes_.size();
          out.nodes_.push_back({batch.level, static_cast<u64>(out.parents_.size()),
                                static_cast<u64>(action.parents.size())});
          out.successors_.push_back(kFullCoverageAbsent); live.push_back(1);
          for (auto parent : action.parents) {
            out.parents_.push_back(parent);
            out.successors_[parent] = segment;
          }
        }
        for (const auto& ref : action.contributions)
          out.contributions_.push_back({batch.level, segment, ref});
      }
    }
    out.order_ = order; out.populations_ = std::move(bank);
    result.status = FullCertificateStatus::kOk; result.reason = "structural_only";
    return result;
  } catch (const std::bad_alloc&) {
    auto result = invalid("coverage_allocation_failed");
    result.status = FullCertificateStatus::kResourceExhausted; return result;
  } catch (const std::length_error&) {
    auto result = invalid("coverage_size_overflow");
    result.status = FullCertificateStatus::kResourceExhausted; return result;
  }
}

// Immutable successor history. In particular, NOT a path-compressed final root.
inline FullNodeId full_coverage_root_at(const FullCoverageCertificate& forest,
    FullNodeId segment, const ExactLevel& cut, bool closed) {
  if (!forest.order() || cut.den <= 0 || segment >= forest.nodes().size() ||
      !full_coverage_detail::admitted(forest.nodes()[segment].level, cut, closed))
    return kFullCoverageAbsent;
  while (forest.successors()[segment] != kFullCoverageAbsent) {
    const auto next = forest.successors()[segment];
    if (!full_coverage_detail::admitted(forest.nodes()[next].level, cut, closed)) break;
    segment = next;
  }
  return segment;
}

// Explicit point-set reconstruction is a READER cost, never a constructor cost.
// root must be live at the requested cut. Overlapping roots stay distinct;
// repeated contributions within the requested root use UNION, not addition.
inline FullReadResult<PointId> full_coverage_at(const FullCoverageCertificate& forest,
    FullNodeId root, const ExactLevel& cut, bool closed) {
  FullReadResult<PointId> out;
  if (root == kFullCoverageAbsent || full_coverage_root_at(forest, root, cut, closed) != root)
    return out;
  try {
    std::vector<FullNodeId> roots(forest.nodes().size(), kFullCoverageAbsent);
    for (size_t i = roots.size(); i-- > 0;) {
      if (!full_coverage_detail::admitted(forest.nodes()[i].level, cut, closed)) continue;
      const auto next = forest.successors()[i];
      roots[i] = next == kFullCoverageAbsent || roots[next] == kFullCoverageAbsent ? i : roots[next];
    }
    for (const auto& record : forest.contributions()) {
      (void)record.level; // mutant ignores contribution date
      if (roots[record.segment] != root) continue;
      const auto& row = forest.populations()->rows()[record.ref.population];
      if (record.ref.include_interior)
        out.values.insert(out.values.end(), row.interior.begin(), row.interior.end());
      for (size_t j = 0; j < row.shell.size(); ++j)
        if (record.ref.shell_mask & (u16{1} << j)) out.values.push_back(row.shell[j]);
    }
    std::sort(out.values.begin(), out.values.end());
    out.values.erase(std::unique(out.values.begin(), out.values.end()), out.values.end());
    out.status = FullCertificateStatus::kOk; out.reason = "structural_only";
  } catch (const std::bad_alloc&) {
    out.status = FullCertificateStatus::kResourceExhausted;
    out.reason = "coverage_read_allocation_failed"; out.values.clear();
  } catch (const std::length_error&) {
    out.status = FullCertificateStatus::kResourceExhausted;
    out.reason = "coverage_read_size_overflow"; out.values.clear();
  }
  return out;
}

}  // namespace mhgp7
