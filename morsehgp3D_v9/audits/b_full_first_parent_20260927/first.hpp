// Explicit audit-only specialization of b_full_batch_encoder_20260927.
// Types, helpers and fallback are attributed to that frozen prototype.
#pragma once
#include "../b_full_batch_encoder_20260927/encode.hpp"

namespace mhgp9::audit_first_parent {
using namespace tower;
namespace ab = audit_batch;
enum class Mutant { None, IgnoreDuplicate, AdmitSameBatch, FirstVisitedError, LastOccurrence };
struct Work { bool fast = false, fallback = false; size_t parent_entries = 0; };

inline ab::Result encode(unsigned order, std::shared_ptr<const FullCoveragePopulations> bank,
                         const FullCoverageFlatDraft& d, bool reverse = false,
                         Mutant mutant = Mutant::None, Work* work = nullptr) {
  if (work) *work = {};
  const auto fail = [](const char* reason) { ab::Result r; r.reason = reason; return r; };
  if (!ab::shaped(d)) return fail("coverage_flat_draft_shape");
  if (order < 1 || order > kFacetMaxK || !bank || bank->rows().empty() ||
      bank->domain().size() < order || d.level.empty()) return fail("coverage_invalid_domain");
  const size_t a_count = d.parent_begin.size() - 1;
  for (size_t a = 0; a < a_count; ++a) if (d.parent_begin[a + 1] - d.parent_begin[a] == 1) {
    if (work) work->fallback = true;
    return ab::encode(order, std::move(bank), d, reverse);
  }
  if (work) { work->fast = true; work->parent_entries = d.parent.size(); }
  try {
    if (a_count == kFullCoverageAbsent) throw std::length_error("node ids");
    std::vector<size_t> batch(a_count);
    for (size_t b = 0; b < d.level.size(); ++b)
      for (size_t a = d.batch_begin[b]; a < d.batch_begin[b + 1]; ++a) batch[a] = b;
    const auto visit = [reverse](size_t n, const auto& fn) {
      for (size_t i = 0; i < n; ++i) fn(reverse ? n - 1 - i : i);
    };
    // No continuation => every action creates exactly one node, node ID=a.
    // All parent references belong to fusions, so each may occur only once.
    // This is an integer minimum reduction, not a chronological live replay.
    std::vector<u64> first(a_count, std::numeric_limits<u64>::max());
    visit(d.parent.size(), [&](size_t j) {
      const auto p = d.parent[j];
      if (p < a_count) first[p] = mutant == Mutant::LastOccurrence ? j : std::min(first[p], static_cast<u64>(j));
    });
    ab::Error first_error{};
    const auto note = [&](size_t b, unsigned part, size_t a, unsigned stage,
                          size_t slot, unsigned instruction, const char* reason) {
      ab::Error e{{b, part, a, stage, slot, instruction}, reason};
      if (!first_error.reason || (mutant != Mutant::FirstVisitedError && e.key < first_error.key)) first_error = e;
    };
    // Local checks ported explicitly from the batch prototype. Same stages,
    // same ordinal-first reference priority, same unnormalised ExactLevel.
    visit(a_count, [&](size_t a) {
      const size_t b = batch[a], local = a - d.batch_begin[b];
      const size_t p0 = d.parent_begin[a], pn = d.parent_begin[a + 1] - p0;
      const size_t c0 = d.contribution_begin[a], cn = d.contribution_begin[a + 1] - c0;
      const auto error = [&](unsigned stage, size_t slot, unsigned instruction, const char* reason) {
        note(b, 1, local, stage, slot, instruction, reason);
      };
      for (size_t j = 0; j < pn; ++j) {
        const auto p = d.parent[p0 + j];
        const bool prebatch = p < d.batch_begin[b];
        const bool allowed = mutant == Mutant::AdmitSameBatch ? p < a : prebatch;
        if ((j && d.parent[p0 + j - 1] >= p) || !allowed ||
            (p < a_count && first[p] != p0 + j && mutant != Mutant::IgnoreDuplicate))
          error(0, j, 0, "coverage_parent_not_unique_prebatch_root");
      }
      bool valid_refs = true;
      for (size_t j = 0; j < cn; ++j) {
        const auto& ref = d.contribution[c0 + j];
        if (ref.population >= bank->rows().size()) {
          error(2, j, 0, "coverage_population_reference"); valid_refs = false; continue;
        }
        const auto& row = bank->rows()[ref.population];
        if ((ref.shell_mask & ~ab::shell_mask(row)) || (ref.include_interior && row.interior.empty()) ||
            (!ref.include_interior && ref.shell_mask == 0)) {
          error(2, j, 1, "coverage_empty_or_invalid_mask"); valid_refs = false;
        }
      }
      if (pn == 0) {
        if (cn != 1) error(3, 0, 0, "coverage_birth_population");
        else if (valid_refs) {
          const auto& ref = d.contribution[c0]; const auto& row = bank->rows()[ref.population];
          if (ref.include_interior != !row.interior.empty() || ref.shell_mask != ab::shell_mask(row) ||
              row.interior.size() + row.shell.size() < order) error(3, 0, 0, "coverage_birth_population");
          if (order == 1) {
            if (b || row.interior.size() + row.shell.size() != 1) error(4, 0, 0, "coverage_k1_roots");
            else {
              const auto id = row.interior.empty() ? row.shell.front() : row.interior.front();
              if (local >= bank->domain().size() || id != bank->domain()[local]) error(4, 0, 0, "coverage_k1_roots");
            }
          }
        }
      }
    });
    visit(d.level.size(), [&](size_t b) {
      const auto& level = d.level[b];
      const size_t n = d.batch_begin[b + 1] - d.batch_begin[b];
      const auto header = [&](unsigned instruction, const char* reason) { note(b, 0, 0, 0, 0, instruction, reason); };
      if (level.den <= 0) header(0, "coverage_invalid_level");
      if (b && level.den > 0 && d.level[b - 1].den > 0 && compare_exact_level(d.level[b - 1], level) >= 0)
        header(1, "coverage_nonincreasing_batch");
      if (n == 0) header(2, "coverage_empty_batch");
      if (order > 1 && ab::zero(level)) header(3, "coverage_positive_level_required");
      if (order == 1 && b == 0 && (!ab::zero(level) || n != bank->domain().size())) header(4, "coverage_k1_roots");
    });
    if (first_error.reason) return fail(first_error.reason);
    ab::Result out;
    out.nodes.resize(a_count); out.parents = d.parent;
    out.successors.assign(a_count, kFullCoverageAbsent); out.contributions.resize(d.contribution.size());
    visit(a_count, [&](size_t a) {
      const auto& level = d.level[batch[a]];
      out.nodes[a] = {level, d.parent_begin[a], d.parent_begin[a + 1] - d.parent_begin[a]};
      for (size_t j = d.parent_begin[a]; j < d.parent_begin[a + 1]; ++j) out.successors[d.parent[j]] = a;
      for (size_t j = d.contribution_begin[a]; j < d.contribution_begin[a + 1]; ++j) out.contributions[j] = {level, a, d.contribution[j]};
    });
    out.order = order; out.bank = std::move(bank); out.status = FullCertificateStatus::kOk; out.reason = "structural_only";
    return out;
  } catch (const std::bad_alloc&) {
    auto r = fail("coverage_allocation_failed"); r.status = FullCertificateStatus::kResourceExhausted; return r;
  } catch (const std::length_error&) {
    auto r = fail("coverage_size_overflow"); r.status = FullCertificateStatus::kResourceExhausted; return r;
  }
}
}  // namespace mhgp9::audit_first_parent
