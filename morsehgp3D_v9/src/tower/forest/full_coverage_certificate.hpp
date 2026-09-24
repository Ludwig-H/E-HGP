// Dated FULL coverage journal, structural authority ONLY. This is not the
// regular minima certificate v1, a geometric producer, or a completeness proof.
#pragma once

#include <atomic>
#include <memory>
#include <system_error>

#include "full_certificate.hpp"
#include "../parallel/pool.hpp"

namespace mhgp9::tower {

inline constexpr const char* kFullCoverageSchema = "full_dated_coverage_forest_v2";
inline constexpr const char* kFullCoverageArenaAccounting = "exact_structural_sizes_reserved_v1";
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
// Same bank; the rows are MOVED in (no copy of their two vectors) and are
// validated on up to `threads` threads. The first invalid row in index order
// decides, exactly as the copying overload.
FullCoveragePopulationResult build_full_coverage_populations(
    std::span<const PointId>, std::vector<FullCoveragePopulation>&&, int threads);

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
  friend FullCoveragePopulationResult build_full_coverage_populations(
      std::span<const PointId>, std::vector<FullCoveragePopulation>&&, int);
  std::vector<PointId> domain_;
  std::vector<FullCoveragePopulation> rows_;
};

namespace full_coverage_detail {
template <typename Points>
inline bool strictly_ordered(const Points& points) {
  for (size_t i = 1; i < points.size(); ++i)
    if (points[i - 1] >= points[i]) return false;
  return true;
}
// v9 E4: exact membership in a STRICTLY increasing domain. A domain of n ids
// whose first is 0 and last is n-1 is exactly {0, ..., n-1} (n distinct
// increasing integers in [0, n-1]), so p is a member iff p < n: the same
// answer as the binary search, in O(1). Any other domain keeps the search.
struct DomainMembership {
  std::span<const PointId> domain;
  bool dense = false;
  explicit DomainMembership(std::span<const PointId> ordered)
      : domain(ordered), dense(!ordered.empty() && ordered.front() == 0 &&
                               static_cast<u64>(ordered.back()) + 1 == ordered.size()) {}
  bool contains(PointId p) const {
#if defined(MHGP9_BANK_MUTANT_DENSE_INCLUSIVE)
    if (dense) return static_cast<u64>(p) <= domain.size();  // mutant: the id n is admitted
#else
    if (dense) return static_cast<u64>(p) < domain.size();
#endif
    return std::binary_search(domain.begin(), domain.end(), p);
  }
};
inline bool valid_population_row(const DomainMembership& domain, const FullCoveragePopulation& row) {
  // A representation bound of the mask, NOT a cloud/work/time ceiling.
  if (row.shell.size() > std::numeric_limits<u16>::digits ||
      (row.interior.empty() && row.shell.empty()) ||
      !strictly_ordered(row.interior) || !strictly_ordered(row.shell)) return false;
  for (const auto* points : {&row.interior, &row.shell})
    for (PointId p : *points)
      if (!domain.contains(p)) return false;
  for (PointId p : row.shell)
    if (std::binary_search(row.interior.begin(), row.interior.end(), p)) return false;
  return true;
}
}  // namespace full_coverage_detail

inline FullCoveragePopulationResult build_full_coverage_populations(
    std::span<const PointId> domain, std::vector<FullCoveragePopulation>&& rows, int threads) {
  FullCoveragePopulationResult result;
  if (domain.empty() || rows.empty() || !full_coverage_detail::strictly_ordered(domain)) return result;
  try {
    // Thread launch or allocation failures are statuses, as in the copying
    // overload, never an exception escaping this public entry point.
    std::atomic<bool> valid{true};
    const full_coverage_detail::DomainMembership members(domain);  // domain strictly ordered (checked above)
    parallel_ranges(rows.size(), threads, [&](size_t begin, size_t end, size_t) {
      for (size_t i = begin; i < end && valid.load(std::memory_order_relaxed); ++i)
        if (!full_coverage_detail::valid_population_row(members, rows[i])) valid.store(false);
    });
    if (!valid.load()) return result;
    auto bank = std::make_shared<FullCoveragePopulations>();
    bank->domain_.assign(domain.begin(), domain.end());
    bank->rows_ = std::move(rows);
    result.value = std::move(bank);
    result.status = FullCertificateStatus::kOk;
    result.reason = "structural_only";
  } catch (const std::bad_alloc&) {
    result.status = FullCertificateStatus::kResourceExhausted;
    result.reason = "coverage_allocation_failed";
  } catch (const std::length_error&) {
    result.status = FullCertificateStatus::kResourceExhausted;
    result.reason = "coverage_size_overflow";
  } catch (const std::system_error&) {
    result.status = FullCertificateStatus::kResourceExhausted;
    result.reason = "coverage_thread_launch_failed";
  }
  return result;
}

inline FullCoveragePopulationResult build_full_coverage_populations(
    std::span<const PointId> domain, std::span<const FullCoveragePopulation> rows) {
  FullCoveragePopulationResult result;
  if (domain.empty() || rows.empty()) return result;
  if (!full_coverage_detail::strictly_ordered(domain)) return result;
  const full_coverage_detail::DomainMembership members(domain);
  for (const auto& row : rows)
    if (!full_coverage_detail::valid_population_row(members, row)) return result;
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
// The same batches in flat form: levels per batch and, for every action, a
// CSR range of parents and one of contributions, in batch then action order.
// Filled without any allocation per action (amortized appends only).
struct FullCoverageFlatDraft {
  std::vector<ExactLevel> level;            // per batch
  std::vector<u64> batch_begin{0};          // actions of batch b: [batch_begin[b], batch_begin[b + 1])
  std::vector<u64> parent_begin{0};         // parents of action a: [parent_begin[a], parent_begin[a + 1])
  std::vector<FullNodeId> parent;
  std::vector<u64> contribution_begin{0};   // contributions of action a, likewise
  std::vector<FullCoverageRef> contribution;
  void open_batch(const ExactLevel& l) { level.push_back(l); batch_begin.push_back(batch_begin.back()); }
  void add_action(std::span<const FullNodeId> parents, std::span<const FullCoverageRef> contributions) {
    parent.insert(parent.end(), parents.begin(), parents.end());
    parent_begin.push_back(parent.size());
    contribution.insert(contribution.end(), contributions.begin(), contributions.end());
    contribution_begin.push_back(contribution.size());
    ++batch_begin.back();
  }
  size_t actions() const { return parent_begin.size() - 1; }
  std::span<const FullNodeId> parents_of(size_t a) const {
    return {parent.data() + parent_begin[a], parent_begin[a + 1] - parent_begin[a]};
  }
  std::span<const FullCoverageRef> contributions_of(size_t a) const {
    return {contribution.data() + contribution_begin[a], contribution_begin[a + 1] - contribution_begin[a]};
  }
};

namespace full_coverage_detail {
// A draft read as batches of actions: the vector form or the flat form.
struct BatchVectorSource {
  std::span<const FullCoverageBatch> v;
  size_t batches() const { return v.size(); }
  const ExactLevel& level(size_t b) const { return v[b].level; }
  size_t actions(size_t b) const { return v[b].actions.size(); }
  std::span<const FullNodeId> parents(size_t b, size_t a) const { return v[b].actions[a].parents; }
  std::span<const FullCoverageRef> contributions(size_t b, size_t a) const { return v[b].actions[a].contributions; }
};
struct FlatDraftSource {
  const FullCoverageFlatDraft& d;
  size_t batches() const { return d.level.size(); }
  const ExactLevel& level(size_t b) const { return d.level[b]; }
  size_t actions(size_t b) const { return d.batch_begin[b + 1] - d.batch_begin[b]; }
  std::span<const FullNodeId> parents(size_t b, size_t a) const { return d.parents_of(d.batch_begin[b] + a); }
  std::span<const FullCoverageRef> contributions(size_t b, size_t a) const {
    return d.contributions_of(d.batch_begin[b] + a);
  }
};
}  // namespace full_coverage_detail

class FullCoverageCertificate;
struct FullCoverageBuildResult;
FullCoverageBuildResult build_full_coverage_certificate(unsigned,
    std::shared_ptr<const FullCoveragePopulations>, std::span<const FullCoverageBatch>);
FullCoverageBuildResult build_full_coverage_certificate(unsigned,
    std::shared_ptr<const FullCoveragePopulations>, const FullCoverageFlatDraft&);
namespace full_coverage_detail {
template <class Source>
FullCoverageBuildResult build_from(unsigned, std::shared_ptr<const FullCoveragePopulations>, const Source&);
}

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
  template <class Source>
  friend FullCoverageBuildResult full_coverage_detail::build_from(unsigned,
      std::shared_ptr<const FullCoveragePopulations>, const Source&);
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
// One body for both draft forms (vector or flat): the same checks, the same
// certificate.
namespace full_coverage_detail {
template <class Source>
FullCoverageBuildResult build_from(unsigned order, std::shared_ptr<const FullCoveragePopulations> bank,
                                   const Source& batches) {
  const auto invalid = [](const char* reason) {
    FullCoverageBuildResult result; result.reason = reason; return result;
  };
  if (order < 1 || order > kFacetMaxK || !bank || bank->rows().empty() ||
      bank->domain().size() < order || batches.batches() == 0) return invalid("coverage_invalid_domain");
  try {
    FullCoverageBuildResult result;
    auto& out = result.value;
    std::vector<u8> live;
    // Count exact structural sizes without changing actions, identities,
    // population references or the validator's atomic pre-lot ordering.
    size_t node_count = 0, parent_count = 0, contribution_count = 0;
    const auto count = [](size_t& total, size_t amount) {
      if (amount > std::numeric_limits<size_t>::max() - total)
        throw std::length_error("coverage arena size overflow");
      total += amount;
    };
    for (size_t b = 0; b < batches.batches(); ++b)
      for (size_t a = 0; a < batches.actions(b); ++a) {
        const auto parents = batches.parents(b, a);
        if (parents.size() != 1) {
          count(node_count, 1);
          count(parent_count, parents.size());
        }
        count(contribution_count, batches.contributions(b, a).size());
      }
    out.nodes_.reserve(node_count);
    out.parents_.reserve(parent_count);
    out.successors_.reserve(node_count);
    out.contributions_.reserve(contribution_count);
    live.reserve(node_count);
    for (size_t b = 0; b < batches.batches(); ++b) {
      const auto& level = batches.level(b);
      const size_t action_count = batches.actions(b);
      if (level.den <= 0) return invalid("coverage_invalid_level");
      if (b && compare_exact_level(batches.level(b - 1), level) >= 0)
        return invalid("coverage_nonincreasing_batch");
      if (action_count == 0) return invalid("coverage_empty_batch");
      if (order > 1 && full_certificate_detail::zero(level))
        return invalid("coverage_positive_level_required");
      if (order == 1 && b == 0 && (!full_certificate_detail::zero(level) ||
          action_count != bank->domain().size())) return invalid("coverage_k1_roots");
      const size_t prior_count = out.nodes_.size();
      // Consume/check every prelot root BEFORE creating any postlot root. Also
      // rejects two ungrouped actions that share a parent at the same level.
      for (size_t a = 0; a < action_count; ++a) {
        const auto parents = batches.parents(b, a);
        const auto contributions = batches.contributions(b, a);
        for (size_t j = 0; j < parents.size(); ++j) {
          const auto parent = parents[j];
          if ((j && parents[j - 1] >= parent) || parent >= prior_count || !live[parent])
            return invalid("coverage_parent_not_unique_prebatch_root");
          live[parent] = 0;
        }
        if (parents.size() == 1 && contributions.empty())
          return invalid("coverage_empty_continuation");
        for (const auto& ref : contributions) {
          if (ref.population >= bank->rows().size()) return invalid("coverage_population_reference");
          const auto& row = bank->rows()[ref.population];
          if ((ref.shell_mask & ~full_coverage_detail::all_shell(row)) ||
              (ref.include_interior && row.interior.empty()) ||
              (!ref.include_interior && ref.shell_mask == 0)) return invalid("coverage_empty_or_invalid_mask");
        }
        if (parents.empty()) {
          if (contributions.size() != 1) return invalid("coverage_birth_population");
          const auto& ref = contributions.front();
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
      for (size_t a = 0; a < action_count; ++a) {
        const auto parents = batches.parents(b, a);
        FullNodeId segment;
        if (parents.size() == 1) {
          segment = parents.front();
          live[segment] = 1;  // continuation retains component identity
        } else {
          if (out.nodes_.size() == kFullCoverageAbsent)
            throw std::length_error("coverage node identifiers exhausted");
          segment = out.nodes_.size();
          out.nodes_.push_back({level, static_cast<u64>(out.parents_.size()), static_cast<u64>(parents.size())});
          out.successors_.push_back(kFullCoverageAbsent); live.push_back(1);
          for (auto parent : parents) {
            out.parents_.push_back(parent);
            out.successors_[parent] = segment;
          }
        }
        for (const auto& ref : batches.contributions(b, a)) out.contributions_.push_back({level, segment, ref});
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
}  // namespace full_coverage_detail

inline FullCoverageBuildResult build_full_coverage_certificate(unsigned order,
    std::shared_ptr<const FullCoveragePopulations> bank, std::span<const FullCoverageBatch> batches) {
  return full_coverage_detail::build_from(order, std::move(bank), full_coverage_detail::BatchVectorSource{batches});
}

// The public flat form is checked as a whole before its first read (auditor
// B, heap overflow under ASan on a short batch_begin): each CSR has one more
// offset than rows, starts at 0, never decreases and ends at its array's size.
inline bool full_coverage_flat_draft_shaped(const FullCoverageFlatDraft& draft) {
  const auto csr = [](const std::vector<u64>& begin, size_t rows, size_t items) {
    if (begin.size() != rows + 1 || begin.front() != 0 || begin.back() != items) return false;
    for (size_t i = 0; i + 1 < begin.size(); ++i)
      if (begin[i] > begin[i + 1]) return false;
    return true;
  };
  if (draft.parent_begin.empty()) return false;
  const size_t actions = draft.parent_begin.size() - 1;
  return csr(draft.batch_begin, draft.level.size(), actions) && csr(draft.parent_begin, actions, draft.parent.size()) &&
         csr(draft.contribution_begin, actions, draft.contribution.size());
}

inline FullCoverageBuildResult build_full_coverage_certificate(unsigned order,
    std::shared_ptr<const FullCoveragePopulations> bank, const FullCoverageFlatDraft& draft) {
  if (!full_coverage_flat_draft_shaped(draft)) {
    FullCoverageBuildResult result;
    result.reason = "coverage_flat_draft_shape";
    return result;
  }
  return full_coverage_detail::build_from(order, std::move(bank), full_coverage_detail::FlatDraftSource{draft});
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
      if (!full_coverage_detail::admitted(record.level, cut, closed)) break;
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

}  // namespace mhgp9::tower
