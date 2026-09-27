// Explicit audit port of b_full_real_drafts_20260927/probe.cpp at b9fcc3d63.
// Changes: first-parent encoder, measured scratch formula, distinct schema.
// Audit instrumentation by a narrowly scoped call-name substitution.
// The native certificate header is included FIRST: its definitions are not
// renamed. FullBall's two calls are intercepted, not its geometry or decisions.
#include "../b_full_first_parent_20260927/first.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sys/resource.h>

namespace real_drafts {
using namespace mhgp9::tower;
using Clock = std::chrono::steady_clock;
double elapsed(Clock::time_point t) { return std::chrono::duration<double, std::milli>(Clock::now() - t).count(); }
void need(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct Slot {
  FullCoverageFlatDraft draft;
  std::shared_ptr<const FullCoveragePopulations> bank;
  double hook_native_ms{}, copy_flatten_ms{};
  unsigned calls{};
  bool flat{};
};
// Sized before the chain creates workers. Exactly one writer per K; all reads
// and resets follow the chain's join. No thread-local capture that misses K.
inline std::array<Slot, 11> slots;
inline bool enabled = false;
FullCoverageBuildResult hook(unsigned k, std::shared_ptr<const FullCoveragePopulations> bank,
                             const FullCoverageFlatDraft& draft) {
  const auto t = Clock::now();
  auto out = build_full_coverage_certificate(k, bank, draft);
  const auto native_ms = elapsed(t);
  if (enabled) {
    need(k > 0 && k <= 10 && out.status == FullCertificateStatus::kOk, "hook.flat_domain");
    auto& s = slots[k]; need(s.calls == 0, "hook.once_per_order");
    const auto begin = Clock::now(); s.draft = draft; s.bank = bank;
    s.copy_flatten_ms = elapsed(begin); s.hook_native_ms = native_ms; s.flat = true; ++s.calls;
  }
  return out;
}
FullCoverageBuildResult hook(unsigned k, std::shared_ptr<const FullCoveragePopulations> bank,
                             std::span<const FullCoverageBatch> batches) {
  const auto t = Clock::now();
  auto out = build_full_coverage_certificate(k, bank, batches);
  const auto native_ms = elapsed(t);
  if (enabled) {
    need(k > 0 && k <= 10 && out.status == FullCertificateStatus::kOk, "hook.vector_domain");
    auto& s = slots[k]; need(s.calls == 0, "hook.once_per_order");
    const auto begin = Clock::now();
    for (const auto& b : batches) {
      s.draft.open_batch(b.level);
      for (const auto& a : b.actions) s.draft.add_action(a.parents, a.contributions);
    }
    s.bank = bank; s.copy_flatten_ms = elapsed(begin); s.hook_native_ms = native_ms; ++s.calls;
  }
  return out;
}
}  // namespace real_drafts

namespace mhgp9::tower {
inline FullCoverageBuildResult audit_capture_certificate(unsigned k,
    std::shared_ptr<const FullCoveragePopulations> bank, const FullCoverageFlatDraft& draft) {
  return real_drafts::hook(k, std::move(bank), draft);
}
inline FullCoverageBuildResult audit_capture_certificate(unsigned k,
    std::shared_ptr<const FullCoveragePopulations> bank, std::span<const FullCoverageBatch> draft) {
  return real_drafts::hook(k, std::move(bank), draft);
}
}  // namespace mhgp9::tower

#define build_full_coverage_certificate audit_capture_certificate
#include "tower/forest/full_ball_tower.hpp"
#undef build_full_coverage_certificate
// ONE translation unit owns all definitions of the instrumented inline
// Builder. Never link libmhgp9_chain.a alongside this source.
#include "../../src/chain/tower_chain.cpp"
#include "../../src/gpu/filter_runner_stub.cpp"
#include "front_fixtures.hpp"

namespace real_drafts {
namespace ab = mhgp9::audit_batch;
using mhgp9::gen::Point3;
bool same_ref(const FullCoverageRef& a, const FullCoverageRef& b) {
  return a.population == b.population && a.shell_mask == b.shell_mask && a.include_interior == b.include_interior;
}
void equal(const FullCoverageCertificate& x, const ab::Result& y) {
  need(y.status == FullCertificateStatus::kOk && std::string(y.reason) == "structural_only", "compare.status");
  need(x.order() == y.order && x.populations() == y.bank && x.nodes().size() == y.nodes.size() &&
       x.parents() == y.parents && x.successors() == y.successors && x.contributions().size() == y.contributions.size(), "compare.structure");
  for (size_t i = 0; i < y.nodes.size(); ++i) {
    const auto& a = x.nodes()[i]; const auto& b = y.nodes[i];
    need(a.level == b.level && a.first == b.first && a.parent_count == b.parent_count, "compare.node");
  }
  for (size_t i = 0; i < y.contributions.size(); ++i) {
    const auto& a = x.contributions()[i]; const auto& b = y.contributions[i];
    need(a.level == b.level && a.segment == b.segment && same_ref(a.ref, b.ref), "compare.contribution");
  }
}
template <typename T> u64 bytes(const std::vector<T>& x) { return x.capacity() * sizeof(T); }
u64 draft_bytes(const FullCoverageFlatDraft& d) {
  return bytes(d.level) + bytes(d.batch_begin) + bytes(d.parent_begin) + bytes(d.parent) + bytes(d.contribution_begin) + bytes(d.contribution);
}
u64 output_bytes(const FullCoverageCertificate& x) {
  return bytes(x.nodes()) + bytes(x.parents()) + bytes(x.successors()) + bytes(x.contributions());
}
long rss() { rusage r{}; need(getrusage(RUSAGE_SELF, &r) == 0, "getrusage"); return r.ru_maxrss; }
struct Row {
  unsigned k{}; size_t batches{}, actions{}, parent_refs{}, contributions{}, nodes{}, continuations{};
  u64 draft_capacity{}, output_capacity{}, prototype_workspace_requested{};
  double native_hook_ms{}, capture_ms{}, check_ms{};
  bool flat{};
  std::array<double, 3> native_ms{}, prototype_ms{};
};
struct Measurement {
  std::vector<Row> rows;
  mhgp9::ChainResult chain;
  double external_ms{}, copy_ms_sum{};
  u64 bank_bytes{}, draft_bytes_sum{}, output_bytes_sum{};
};
Measurement run(const std::vector<Point3>& points, unsigned k, size_t workers, int static_threads, bool baseline) {
  for (auto& s : slots) s = {};
  mhgp9::ChainOptions o;
  o.kmax = k; o.workers = workers; o.tower_static_threads = static_threads;
  o.separation_s = 8; o.catalogue_digest = true;
  // The defaults are pinned source; all GPU levers remain false. Normal
  // CPU generator and real static FULL construction, not a synthetic draft.
  Measurement result;
  enabled = true;
  const auto t = Clock::now(); result.chain = mhgp9::run_tower_chain(points, o); result.external_ms = elapsed(t);
  enabled = false;
  need(result.chain.status == mhgp9::ChainStatus::kComplete, result.chain.reason.c_str());
  need(result.chain.kmax_effective == k && result.chain.tower.orders.size() == k, "capture.all_orders");
  const auto bank = slots[1].bank; need(static_cast<bool>(bank), "capture.bank");
  result.bank_bytes = bytes(bank->domain()) + bytes(bank->rows());
  for (const auto& p : bank->rows()) result.bank_bytes += bytes(p.interior) + bytes(p.shell);
  for (unsigned order = 1; order <= k; ++order) {
    const auto& s = slots[order]; need(s.calls == 1 && s.bank == bank, "capture.bank_identity");
    const auto& d = s.draft;
    const auto& forest = result.chain.tower.orders[order - 1].forest;
    Row row;
    row.k = order; row.batches = d.level.size(); row.actions = d.actions();
    row.parent_refs = d.parent.size(); row.contributions = d.contribution.size(); row.nodes = forest.nodes().size();
    row.draft_capacity = draft_bytes(d); row.output_capacity = output_bytes(forest);
    row.native_hook_ms = s.hook_native_ms; row.capture_ms = s.copy_flatten_ms; row.flat = s.flat;
    for (size_t a = 0; a < row.actions; ++a) row.continuations += d.parent_begin[a + 1] - d.parent_begin[a] == 1;
    // Fast path: batch[A] and first[V], V=A. No incidence sort or prefixes.
    // Allocator metadata, output, and stack excluded. Distinct from peak RSS.
    need(row.continuations == 0, "measurement.requires_fast_path");
    row.prototype_workspace_requested = 2 * row.actions * sizeof(size_t);
    for (size_t rep = 0; rep < 3; ++rep) {
      FullCoverageBuildResult native;
      ab::Result trial;
      const auto native_call = [&] {
        const auto start = Clock::now(); native = build_full_coverage_certificate(order, bank, d); row.native_ms[rep] = elapsed(start);
      };
      const auto trial_call = [&] {
        const auto start = Clock::now(); trial = mhgp9::audit_first_parent::encode(order, bank, d, false); row.prototype_ms[rep] = elapsed(start);
      };
      if (rep % 2 == 0) { native_call(); trial_call(); } else { trial_call(); native_call(); }
      need(native.status == FullCertificateStatus::kOk, "reencode.native_status");
      const auto check_start = Clock::now(); equal(native.value, trial); equal(forest, trial); row.check_ms += elapsed(check_start);
    }
    result.copy_ms_sum += row.capture_ms; result.draft_bytes_sum += row.draft_capacity;
    result.output_bytes_sum += row.output_capacity; result.rows.push_back(row);
  }
  if (baseline) {
    // Same compiled geometry with hooks pass-through (no copies), not a
    // separately compiled stock-build performance comparison.
    auto plain = mhgp9::run_tower_chain(points, o);
    need(plain.status == mhgp9::ChainStatus::kComplete && plain.tower_digest == result.chain.tower_digest &&
         plain.catalogue_digest == result.chain.catalogue_digest && plain.presentation_digest == result.chain.presentation_digest,
         "hook.pass_through_three_digests");
  }
  return result;
}
std::vector<Point3> read_frame(const std::string& name) {
  std::ifstream f(name, std::ios::binary); need(static_cast<bool>(f), "frame.open");
  std::vector<unsigned char> data((std::istreambuf_iterator<char>(f)), {});
  need(data.size() % 12 == 0 && !data.empty(), "frame.shape");
  std::vector<Point3> points(data.size() / 12);
  for (size_t i = 0; i < points.size(); ++i) {
    std::array<u32, 3> xyz{};
    for (size_t d = 0; d < 3; ++d) {
      for (size_t j = 0; j < 4; ++j) xyz[d] |= static_cast<u32>(data[12*i + 4*d + j]) << (8*j);
      need(xyz[d] <= 262143, "frame.u18");
    }
    // Coordinate is signed; u18 was checked before each narrowing.
    points[i] = Point3{static_cast<mhgp9::gen::Coordinate>(xyz[0]),
                      static_cast<mhgp9::gen::Coordinate>(xyz[1]),
                      static_cast<mhgp9::gen::Coordinate>(xyz[2])};
  }
  return points;
}
u64 hash_points(const std::vector<Point3>& points) {
  u64 h = 14695981039346656037ULL;
  mhgp9::gen::bench::front_hash_word(h, 1); mhgp9::gen::bench::front_hash_word(h, points.size());
  for (const auto p : points) for (const auto x : {p.x, p.y, p.z}) mhgp9::gen::bench::front_hash_word(h, x);
  return h;
}
void report(const Measurement& x, const std::vector<Point3>& points, const char* mode, const std::string& family) {
  std::cout << std::setprecision(12) << "{\"schema\":\"mhgp9_full_first_real_drafts_v1\",\"status\":\"passed\",\"scope\":\"instrumented_real_cpu_chain_flat_encoder_only\",\"GCP_used\":false,\"mode\":\"" << mode
    << "\",\"family\":\"" << family << "\",\"n\":" << points.size() << ",\"input_hash_u64\":" << hash_points(points)
    << ",\"k\":" << x.chain.kmax_effective << ",\"s\":8,\"seed\":3,\"workers\":4,\"static_threads\":4"
    << ",\"chain_instrumented_ms\":" << x.chain.times.total_ms << ",\"process_chain_external_ms\":" << x.external_ms
    << ",\"chain_instrumented_tower_ms\":" << x.chain.times.tower_ms << ",\"chain_generation_q2_ms\":" << x.chain.times.q2_ms
    << ",\"chain_generation_q34_ms\":" << x.chain.times.q34_ms << ",\"chain_encode_instrumented_ms\":" << x.chain.tower_times.encode_ms
    << ",\"capture_copy_ms_sum\":" << x.copy_ms_sum << ",\"bank_capacity_bytes\":" << x.bank_bytes
    << ",\"draft_capacity_bytes_sum\":" << x.draft_bytes_sum << ",\"output_capacity_bytes_sum\":" << x.output_bytes_sum
    << ",\"peak_rss_kib\":" << rss() << ",\"tower_digest\":" << x.chain.tower_digest << ",\"catalogue_digest\":" << x.chain.catalogue_digest
    << ",\"presentation_digest\":" << x.chain.presentation_digest << ",\"rows\":[";
  bool first = true;
  for (const auto& r : x.rows) {
    if (!first) std::cout << ',';
    first = false;
    std::cout << "{\"k\":" << r.k << ",\"flat_source\":" << (r.flat ? "true" : "false") << ",\"batches\":" << r.batches
      << ",\"actions\":" << r.actions << ",\"parents\":" << r.parent_refs << ",\"contributions\":" << r.contributions
      << ",\"nodes\":" << r.nodes << ",\"continuations\":" << r.continuations << ",\"draft_capacity_bytes\":" << r.draft_capacity
      << ",\"output_capacity_bytes\":" << r.output_capacity << ",\"prototype_workspace_requested_bytes\":" << r.prototype_workspace_requested
      << ",\"native_hook_ms\":" << r.native_hook_ms << ",\"capture_copy_ms\":" << r.capture_ms << ",\"checks_ms_sum\":" << r.check_ms
      << ",\"native_ms\":[" << r.native_ms[0] << ',' << r.native_ms[1] << ',' << r.native_ms[2]
      << "],\"prototype_ms\":[" << r.prototype_ms[0] << ',' << r.prototype_ms[1] << ',' << r.prototype_ms[2] << "]}";
  }
  std::cout << "]}\n";
}
}  // namespace real_drafts
int main(int argc, char** argv) {
  try {
    using namespace real_drafts;
    if (argc == 2 && std::string(argv[1]) == "--gate") {
      for (int threads : {0, 2}) {
        const auto points = mhgp9::gen::bench::make_front_fixture(12, "uniform", 3).points;
        auto result = run(points, 5, threads == 0 ? 1 : 2, threads, true);
        need(result.rows.size() == 5 && result.draft_bytes_sum > 0, "gate.nonvacuity");
        for (const auto& row : result.rows) need(row.flat == (threads == 2), "gate.two_overloads");
      }
      std::cout << "{\"schema\":\"mhgp9_full_first_real_drafts_gate_v1\",\"status\":\"passed\",\"chains\":4,\"orders_reencoded\":10,\"pairs\":30,\"GCP_used\":false}\n";
      return 0;
    }
    need(argc == 3 || argc == 4, "usage: --frame path | --synthetic family n");
    std::vector<Point3> points;
    std::string family = "none";
    const char* mode = "frame";
    if (std::string(argv[1]) == "--frame" && argc == 3) points = read_frame(argv[2]);
    else {
      need(std::string(argv[1]) == "--synthetic" && argc == 4, "synthetic.args");
      family = argv[2]; mode = "synthetic";
      points = mhgp9::gen::bench::make_front_fixture(std::stoull(argv[3]), family, 3).points;
    }
    auto result = run(points, 5, 4, 4, false);
    report(result, points, mode, family);
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 2; }
}

