// Explicit audit-only parallel port of first.hpp at fd1a2c7ee.
// Local checks/types/fallback are attributed to that frozen scalar prototype.
// Draft and authentic bank must remain immutable for this synchronous call.
#pragma once
#include "../b_full_first_parent_20260927/first.hpp"
#include <atomic>
#include <exception>
#include <thread>
#include <system_error>

namespace mhgp9::audit_parallel_parent {
using namespace tower;
namespace ab = audit_batch;
using Mutant = audit_first_parent::Mutant;
struct Options { size_t workers = 1, grain = 64; bool reverse = false; };
struct Work {
  bool fast = false, fallback = false;
  size_t parent_entries = 0, phases = 0, threads_started = 0, max_workers = 0;
  size_t max_workers_active = 0, parent_workers_active = 0;
};
struct PhaseStats { size_t workers = 0, active = 0; };
// Audit-only deterministic failure injection at the actual launch seam.
struct LaunchFault { size_t after = std::numeric_limits<size_t>::max(); };

template<class Fn>
size_t parallel_for(size_t n, Options o, const Fn& fn, LaunchFault fault = {}, PhaseStats* stats = nullptr) {
  if (o.workers == 0 || o.grain == 0) throw std::invalid_argument("parallel options");
  if (stats) *stats = {};
  if (n == 0) return 0;
  const size_t jobs = n / o.grain + (n % o.grain != 0);
  const size_t count = std::min(o.workers, jobs);
  std::atomic<bool> cancel{false};
  std::vector<std::exception_ptr> errors(count);
  std::vector<unsigned char> active(count, 0);
  std::vector<std::thread> threads;
  threads.reserve(count - 1);
  const auto body = [&](size_t worker) {
    try {
      // Cyclic chunks: each worker owns at least one nonempty job. This
      // avoids vacuous thread launches and any overflowing queue counter.
      for (size_t job = worker; job < jobs && !cancel.load(std::memory_order_relaxed);) {
        active[worker] = 1;
        const size_t begin = job * o.grain; // job<ceil(n/grain), so begin<n.
        const size_t end = begin + std::min(o.grain, n - begin);
        for (size_t i = begin; i < end; ++i) fn(o.reverse ? n - 1 - i : i, worker);
        if (jobs - job <= count) break;
        job += count;
      }
    } catch (...) {
      errors[worker] = std::current_exception();
      cancel.store(true, std::memory_order_relaxed);
    }
  };
  const auto join = [&] { for (auto& thread : threads) if (thread.joinable()) thread.join(); };
  try {
    for (size_t worker = 1; worker < count; ++worker) {
      if (threads.size() == fault.after) throw std::system_error(std::make_error_code(std::errc::resource_unavailable_try_again));
      threads.emplace_back(body, worker);
    }
    body(0);
  } catch (...) {
    cancel.store(true, std::memory_order_relaxed); join(); throw;
  }
  join(); // Publication and ordinary post-CAS reads happen only after joins.
  for (const auto& error : errors) if (error) std::rethrow_exception(error);
  if (stats) { stats->workers = count; for (auto x : active) stats->active += x; }
  return count;
}

inline ab::Result encode(unsigned order, std::shared_ptr<const FullCoveragePopulations> bank,
                         const FullCoverageFlatDraft& d, Options options = {},
                         Mutant mutant = Mutant::None, Work* work = nullptr) {
  if (work) *work = {};
  const auto fail = [](const char* reason) { ab::Result r; r.reason = reason; return r; };
  if (!ab::shaped(d)) return fail("coverage_flat_draft_shape");
  if (order < 1 || order > kFacetMaxK || !bank || bank->rows().empty() ||
      bank->domain().size() < order || d.level.empty()) return fail("coverage_invalid_domain");
  if (options.workers == 0 || options.grain == 0) throw std::invalid_argument("parallel options");
  if (mutant == Mutant::IgnoreDuplicate && options.workers > 1)
    throw std::invalid_argument("duplicate-admission mutant requires W1");
  const size_t a_count = d.parent_begin.size() - 1;
  for (size_t a = 0; a < a_count; ++a) if (d.parent_begin[a + 1] - d.parent_begin[a] == 1) {
    if (work) work->fallback = true;
    return audit_first_parent::encode(order, std::move(bank), d, options.reverse);
  }
  if (work) { work->fast = true; work->parent_entries = d.parent.size(); }
  try {
    if (a_count == kFullCoverageAbsent) throw std::length_error("node ids");
    const auto visit = [&](size_t n, const auto& fn) {
      PhaseStats stats;
      const auto used = parallel_for(n, options, fn, {}, &stats);
      if (work) {
        const auto extra = used > 0 ? used - 1 : 0;
        if (work->threads_started > std::numeric_limits<size_t>::max() - extra) throw std::length_error("thread counter");
        ++work->phases; work->threads_started += extra;
        work->max_workers = std::max(work->max_workers, used);
        work->max_workers_active = std::max(work->max_workers_active, stats.active);
      }
      return stats.active;
    };
    std::vector<size_t> batch(a_count);
    visit(d.level.size(), [&](size_t b, size_t) {
      for (size_t a = d.batch_begin[b]; a < d.batch_begin[b + 1]; ++a) batch[a] = b;
    });
    // No continuation => every action creates exactly one node, node ID=a.
    // All parent references belong to fusions, so each may occur only once.
    // This is an integer minimum reduction, not a chronological live replay.
    // Ordinary initialization and later reads do not overlap atomic accesses.
    // Every atomic phase joins before proceeding; relaxed CAS is sufficient.
    static_assert(std::atomic_ref<u64>::required_alignment <= alignof(u64));
    std::vector<u64> first(a_count);
    visit(a_count, [&](size_t a, size_t) { first[a] = std::numeric_limits<u64>::max(); });
    const auto parent_active = visit(d.parent.size(), [&](size_t j, size_t) {
      const auto p = d.parent[j];
      if (p >= a_count) return;  // No invalid index, even on a rejected draft.
      std::atomic_ref<u64> atom(first[p]);
      auto old = atom.load(std::memory_order_relaxed);
      const auto value = static_cast<u64>(j);
      if (mutant == Mutant::LastOccurrence) {
        // Deliberately wrong but race-free maximum, only for semantic testing.
        while ((old == std::numeric_limits<u64>::max() || old < value) &&
               !atom.compare_exchange_weak(old, value, std::memory_order_relaxed)) {}
      } else {
        while (value < old && !atom.compare_exchange_weak(old, value, std::memory_order_relaxed)) {}
      }
    });
    if (work) work->parent_workers_active = parent_active;
    std::vector<ab::Error> errors(std::min(options.workers, std::max(a_count, d.level.size())));
    const auto note = [&](size_t worker, size_t b, unsigned part, size_t a, unsigned stage,
                          size_t slot, unsigned instruction, const char* reason) {
      ab::Error e{{b, part, a, stage, slot, instruction}, reason};
      auto& local_error = errors[worker]; // Exactly one writer per worker slot.
      if (!local_error.reason || (mutant != Mutant::FirstVisitedError && e.key < local_error.key)) local_error = e;
    };
    // Local checks ported explicitly from the batch prototype. Same stages,
    // same ordinal-first reference priority, same unnormalised ExactLevel.
    visit(a_count, [&](size_t a, size_t worker) {
      const size_t b = batch[a], local = a - d.batch_begin[b];
      const size_t p0 = d.parent_begin[a], pn = d.parent_begin[a + 1] - p0;
      const size_t c0 = d.contribution_begin[a], cn = d.contribution_begin[a + 1] - c0;
      const auto error = [&](unsigned stage, size_t slot, unsigned instruction, const char* reason) {
        note(worker, b, 1, local, stage, slot, instruction, reason);
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
    visit(d.level.size(), [&](size_t b, size_t worker) {
      const auto& level = d.level[b];
      const size_t n = d.batch_begin[b + 1] - d.batch_begin[b];
      const auto header = [&](unsigned instruction, const char* reason) { note(worker, b, 0, 0, 0, 0, instruction, reason); };
      if (level.den <= 0) header(0, "coverage_invalid_level");
      if (b && level.den > 0 && d.level[b - 1].den > 0 && compare_exact_level(d.level[b - 1], level) >= 0)
        header(1, "coverage_nonincreasing_batch");
      if (n == 0) header(2, "coverage_empty_batch");
      if (order > 1 && ab::zero(level)) header(3, "coverage_positive_level_required");
      if (order == 1 && b == 0 && (!ab::zero(level) || n != bank->domain().size())) header(4, "coverage_k1_roots");
    });
    ab::Error first_error{};
    // All validation workers are joined. No arrival-order winner.
    for (const auto& error : errors)
      if (error.reason && (!first_error.reason || error.key < first_error.key)) first_error = error;
    if (first_error.reason) return fail(first_error.reason);
    ab::Result out;
    out.nodes.resize(a_count); out.parents = d.parent;
    out.successors.assign(a_count, kFullCoverageAbsent); out.contributions.resize(d.contribution.size());
    visit(a_count, [&](size_t a, size_t) {
      const auto& level = d.level[batch[a]];
      out.nodes[a] = {level, d.parent_begin[a], d.parent_begin[a + 1] - d.parent_begin[a]};
      for (size_t j = d.parent_begin[a]; j < d.parent_begin[a + 1]; ++j) out.successors[d.parent[j]] = a;
      for (size_t j = d.contribution_begin[a]; j < d.contribution_begin[a + 1]; ++j) out.contributions[j] = {level, a, d.contribution[j]};
    });
    out.order = order; out.bank = std::move(bank); out.status = FullCertificateStatus::kOk; out.reason = "structural_only";
    return out;
  } catch (const std::system_error&) {
    auto r = fail("coverage_worker_start_failed"); r.status = FullCertificateStatus::kResourceExhausted; return r;
  } catch (const std::bad_alloc&) {
    auto r = fail("coverage_allocation_failed"); r.status = FullCertificateStatus::kResourceExhausted; return r;
  } catch (const std::length_error&) {
    auto r = fail("coverage_size_overflow"); r.status = FullCertificateStatus::kResourceExhausted; return r;
  }
}
}  // namespace mhgp9::audit_parallel_parent
