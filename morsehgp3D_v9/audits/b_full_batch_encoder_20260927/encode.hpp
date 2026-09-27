// Audit-only batch encoder. Shared product types/comparator, independent
// structural decision: no call to build_from and no chronological live array.
#pragma once

#include <tuple>

#include "tower/forest/full_coverage_certificate.hpp"

namespace mhgp9::audit_batch {
using namespace tower;

enum class Mutant { None, IgnoreDead, FirstVisitedError, NormalizeLevel };

struct Result {
  FullCertificateStatus status = FullCertificateStatus::kInvalidInput;
  const char* reason = "coverage_invalid_input";
  unsigned order = 0;
  std::shared_ptr<const FullCoveragePopulations> bank;
  std::vector<FullNode> nodes;
  std::vector<FullNodeId> parents, successors;
  std::vector<FullDatedContribution> contributions;
};

inline bool shaped(const FullCoverageFlatDraft& d) {
  const auto csr = [](const std::vector<u64>& x, size_t rows, size_t count) {
    if (rows == std::numeric_limits<size_t>::max() || x.size() != rows + 1 ||
        x.empty() || x.front() != 0 || x.back() != count) return false;
    for (size_t i = 1; i < x.size(); ++i) if (x[i] < x[i - 1]) return false;
    return true;
  };
  if (d.parent_begin.empty()) return false;
  const size_t n = d.parent_begin.size() - 1;
  return csr(d.batch_begin, d.level.size(), n) && csr(d.parent_begin, n, d.parent.size()) &&
         csr(d.contribution_begin, n, d.contribution.size());
}

struct Error {
  // The header precedes every action in its batch. Local stages are ordered
  // exactly like the native validator. The tuple, not arrival, owns priority.
  std::tuple<size_t, unsigned, size_t, unsigned, size_t, unsigned> key;
  const char* reason = nullptr;
};
inline bool zero(const ExactLevel& x) { return x.num[0] == 0 && x.num[1] == 0 && x.num[2] == 0; }
inline u16 shell_mask(const FullCoveragePopulation& p) {
  // A genuine immutable, copying-factory bank is required at this boundary.
  return static_cast<u16>((u32{1} << p.shell.size()) - 1);
}

inline Result encode(unsigned order, std::shared_ptr<const FullCoveragePopulations> bank,
                     const FullCoverageFlatDraft& d, bool reverse = false,
                     Mutant mutant = Mutant::None) {
  const auto fail = [](const char* reason) { Result r; r.reason = reason; return r; };
  if (!shaped(d)) return fail("coverage_flat_draft_shape");
  if (order < 1 || order > kFacetMaxK || !bank || bank->rows().empty() ||
      bank->domain().size() < order || d.level.empty()) return fail("coverage_invalid_domain");
  try {
    const size_t a_count = d.parent_begin.size() - 1, b_count = d.level.size();
    std::vector<size_t> batch(a_count), node_prefix(a_count + 1, 0), parent_prefix(a_count + 1, 0);
    for (size_t b = 0; b < b_count; ++b)
      for (size_t a = d.batch_begin[b]; a < d.batch_begin[b + 1]; ++a) batch[a] = b;
    for (size_t a = 0; a < a_count; ++a) {
      const size_t n = d.parent_begin[a + 1] - d.parent_begin[a];
      node_prefix[a + 1] = node_prefix[a] + (n != 1);
      parent_prefix[a + 1] = parent_prefix[a] + (n != 1 ? n : 0);
    }
    if (node_prefix.back() == kFullCoverageAbsent) throw std::length_error("node ids");
    Error first{};
    const auto note = [&](size_t b, unsigned part, size_t a, unsigned stage,
                          size_t slot, unsigned instruction, const char* reason) {
      Error e{{b, part, a, stage, slot, instruction}, reason};
      if (!first.reason || (mutant != Mutant::FirstVisitedError && e.key < first.key)) first = e;
    };
    const auto visit = [reverse](size_t n, const auto& fn) {
      for (size_t i = 0; i < n; ++i) fn(reverse ? n - 1 - i : i);
    };
    struct Incidence {
      FullNodeId parent;
      size_t b, a, slot;
      bool merge;
      auto key() const { return std::tie(parent, b, a, slot); }
    };
    std::vector<Incidence> incidences;
    incidences.reserve(d.parent.size());
    // Deliberately actions BEFORE headers. Canonical error reduction must
    // still reproduce header-first semantics, even in the reverse schedule.
    visit(a_count, [&](size_t a) {
      const size_t b = batch[a], local = a - d.batch_begin[b];
      const size_t p0 = d.parent_begin[a], pn = d.parent_begin[a + 1] - p0;
      const size_t c0 = d.contribution_begin[a], cn = d.contribution_begin[a + 1] - c0;
      const auto action_error = [&](unsigned stage, size_t slot, unsigned instruction, const char* reason) {
        note(b, 1, local, stage, slot, instruction, reason);
      };
      for (size_t j = 0; j < pn; ++j) {
        const auto p = d.parent[p0 + j];
        if ((j && d.parent[p0 + j - 1] >= p) || p >= node_prefix[d.batch_begin[b]])
          action_error(0, j, 0, "coverage_parent_not_unique_prebatch_root");
        // No dereference through p. Invalid references still form harmless
        // integer incidences; a local range error always has earlier priority.
        incidences.push_back({p, b, local, j, pn != 1});
      }
      if (pn == 1 && cn == 0) action_error(1, 0, 0, "coverage_empty_continuation");
      bool valid_refs = true;
      for (size_t j = 0; j < cn; ++j) {
        const auto& ref = d.contribution[c0 + j];
        if (ref.population >= bank->rows().size()) {
          action_error(2, j, 0, "coverage_population_reference"); valid_refs = false; continue;
        }
        const auto& row = bank->rows()[ref.population];
        if ((ref.shell_mask & ~shell_mask(row)) || (ref.include_interior && row.interior.empty()) ||
            (!ref.include_interior && ref.shell_mask == 0)) {
          action_error(2, j, 1, "coverage_empty_or_invalid_mask"); valid_refs = false;
        }
      }
      if (pn == 0) {
        if (cn != 1) action_error(3, 0, 0, "coverage_birth_population");
        else if (valid_refs) {
          const auto& ref = d.contribution[c0];
          const auto& row = bank->rows()[ref.population];
          if (ref.include_interior != !row.interior.empty() || ref.shell_mask != shell_mask(row) ||
              row.interior.size() + row.shell.size() < order)
            action_error(3, 0, 0, "coverage_birth_population");
          if (order == 1) {
            if (b || row.interior.size() + row.shell.size() != 1)
              action_error(4, 0, 0, "coverage_k1_roots");
            else {
              const auto id = row.interior.empty() ? row.shell.front() : row.interior.front();
              if (local >= bank->domain().size() || id != bank->domain()[local])
                action_error(4, 0, 0, "coverage_k1_roots");
            }
          }
        }
      }
    });
    visit(b_count, [&](size_t b) {
      const auto& l = d.level[b];
      const size_t n = d.batch_begin[b + 1] - d.batch_begin[b];
      const auto header = [&](unsigned instruction, const char* reason) { note(b, 0, 0, 0, 0, instruction, reason); };
      if (l.den <= 0) header(0, "coverage_invalid_level");
      if (b && l.den > 0 && d.level[b - 1].den > 0 && compare_exact_level(d.level[b - 1], l) >= 0)
        header(1, "coverage_nonincreasing_batch");
      if (n == 0) header(2, "coverage_empty_batch");
      if (order > 1 && zero(l)) header(3, "coverage_positive_level_required");
      if (order == 1 && b == 0 && (!zero(l) || n != bank->domain().size()))
        header(4, "coverage_k1_roots");
    });
    std::sort(incidences.begin(), incidences.end(), [](const auto& x, const auto& y) { return x.key() < y.key(); });
    // Segmented prefix OR of merge flags. The scalar loop is a prototype of
    // that scan, not a chronological DSU/live-state replay or threaded claim.
    bool dead = false;
    for (size_t i = 0; i < incidences.size(); ++i) {
      const auto& x = incidences[i];
      const bool same_parent = i && incidences[i - 1].parent == x.parent;
      if (!same_parent) dead = false;
      const bool duplicate_batch = same_parent && incidences[i - 1].b == x.b;
      if (duplicate_batch || (dead && mutant != Mutant::IgnoreDead))
        note(x.b, 1, x.a, 0, x.slot, 0, "coverage_parent_not_unique_prebatch_root");
      dead = dead || x.merge;
    }
    if (first.reason) return fail(first.reason);
    Result out;
    out.nodes.resize(node_prefix.back()); out.parents.resize(parent_prefix.back());
    out.successors.assign(node_prefix.back(), kFullCoverageAbsent);
    out.contributions.resize(d.contribution.size());
    visit(a_count, [&](size_t a) {
      const size_t b = batch[a], p0 = d.parent_begin[a], pn = d.parent_begin[a + 1] - p0;
      auto level = d.level[b];
      if (mutant == Mutant::NormalizeLevel && level.num[1] == 0 && level.num[2] == 0 &&
          level.num[0] % 2 == 0 && level.den % 2 == 0) { level.num[0] /= 2; level.den /= 2; }
      const FullNodeId segment = pn == 1 ? d.parent[p0] : node_prefix[a];
      if (pn != 1) {
        out.nodes[segment] = {level, static_cast<u64>(parent_prefix[a]), static_cast<u64>(pn)};
        for (size_t j = 0; j < pn; ++j) {
          out.parents[parent_prefix[a] + j] = d.parent[p0 + j];
          out.successors[d.parent[p0 + j]] = segment;
        }
      }
      for (size_t j = d.contribution_begin[a]; j < d.contribution_begin[a + 1]; ++j)
        out.contributions[j] = {level, segment, d.contribution[j]};
    });
    out.order = order; out.bank = std::move(bank);
    out.status = FullCertificateStatus::kOk; out.reason = "structural_only";
    return out;
  } catch (const std::bad_alloc&) {
    auto r = fail("coverage_allocation_failed"); r.status = FullCertificateStatus::kResourceExhausted; return r;
  } catch (const std::length_error&) {
    auto r = fail("coverage_size_overflow"); r.status = FullCertificateStatus::kResourceExhausted; return r;
  }
}
}  // namespace mhgp9::audit_batch
