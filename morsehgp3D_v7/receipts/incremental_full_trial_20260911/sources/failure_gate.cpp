// Test-only hooks at transactional boundaries. A real global new throws after
// a successful K1 and non-empty K2 prefix, or at global bank sealing.
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <new>
#include <stdexcept>
#include <string>

namespace incremental_failure_test {
std::atomic<bool> counting{false};
std::atomic<std::size_t> attempts{0}, fail_at{0}, failures{0};
std::size_t committed = 0, armed_after = 0, poison_checks = 0;
bool armed = false, seal_stage = false, semantic = false, semantic_injected = false;
template<class Batch> void before_append(unsigned, std::uint64_t, const Batch&);
template<class Owner, class Token, class Batch, class Result>
void after_attempt(Owner&, const Token&, const Batch&, const Result&);
void after_append() { ++committed; }
void before_seal();
[[gnu::noinline]] void* allocate(std::size_t n) {
  if (counting.load(std::memory_order_relaxed)) {
    const auto call = attempts.fetch_add(1, std::memory_order_relaxed) + 1;
    if (call == fail_at.load(std::memory_order_relaxed)) {
      failures.fetch_add(1, std::memory_order_relaxed); throw std::bad_alloc();
    }
  }
  if (void* result = std::malloc(n ? n : 1)) return result;
  throw std::bad_alloc();
}
}
void* operator new(std::size_t n) { return incremental_failure_test::allocate(n); }
void* operator new[](std::size_t n) { return incremental_failure_test::allocate(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

#include MHGP7_FULL_HEADER

namespace incremental_failure_test {
using namespace mhgp7;
ExactLevel previous{};
void need(bool value, const char* reason) { if (!value) throw std::runtime_error(reason); }
void arm() {
  armed = true; armed_after = committed;
  counting.store(!semantic, std::memory_order_relaxed);
}
template<class Batch> void before_append(unsigned k, std::uint64_t prior, const Batch& batch) {
  if (!armed && !seal_stage && k == 2 && prior > 0) {
    need(committed > 2, "prefix_before_failure_nonempty"); arm();
    if (semantic) {
      // The actual object is a non-const local batch, borrowed by const ref.
      // Test-only corruption of the later level, never of the census or prefix.
      const_cast<Batch&>(batch).level = previous;
      semantic_injected = true;
    }
  }
  previous = batch.level;
}
template<class Owner, class Token, class Batch, class Result>
void after_attempt(Owner& owner, const Token& token, const Batch& batch, const Result& result) {
  if (result.status == FullCertificateStatus::kOk) return;
  counting.store(false, std::memory_order_relaxed);
  if (!semantic_injected && !failures.load()) return;
  const auto closed = owner.seal();
  need(closed.status == result.status && closed.orders.empty() && !closed.populations,
       "failed_owner_seal_publishes_nothing");
  need(owner.append_batch(token, batch).status == result.status &&
       owner.end_order(token).status == result.status &&
       owner.begin_order(1).status == result.status,
       "failed_owner_cannot_resume");
  poison_checks += 4;
}
void before_seal() {
  if (!armed && seal_stage) {
    need(committed > 5, "whole_tower_prefix_before_seal"); arm();
  }
}
void reset(std::size_t fail, bool at_seal, bool invalid) {
  counting.store(false); attempts.store(0); fail_at.store(fail); failures.store(0);
  committed = armed_after = 0; armed = false; seal_stage = at_seal;
  semantic = invalid; semantic_injected = false; previous = {};
}
}

namespace {
using namespace mhgp7;
using incremental_failure_test::need;
i128 integer(std::istream& in) {
  std::string s; in >> s; need(!s.empty(), "fixture_integer");
  const bool negative = s[0] == '-'; u128 value = 0;
  for (size_t j = negative ? 1 : 0; j < s.size(); ++j) {
    need(s[j] >= '0' && s[j] <= '9' && value < (u128{1} << 120), "integer_domain");
    value = 10 * value + static_cast<unsigned>(s[j] - '0');
  }
  return negative ? -static_cast<i128>(value) : static_cast<i128>(value);
}
struct Fixture { CloudIndex index; std::vector<BallData> balls; unsigned kmax; };
Fixture load(const char* path) {
  std::ifstream in(path); need(in.good(), "fixture_open");
  unsigned count = 0, n = 0, kmax = 0, nb = 0, nc = 0; std::string name;
  in >> count >> name >> n >> kmax >> nb >> nc;
  need(count == 6 && name == "E5" && n == 5 && kmax == 5 && nb == 17 && nc == 17, "pinned_E5_shape");
  std::vector<P3> points(n);
  for (auto& p : points) in >> p.x >> p.y >> p.z;
  auto index = build_cloud_index(points);
  std::vector<BallData> balls(nb);
  for (auto& b : balls) {
    b.key.a = integer(in); for (auto& x : b.key.b) x = integer(in); b.key.c = integer(in);
    const auto num = integer(in), den = integer(in);
    need(num >= 0 && num <= std::numeric_limits<u64>::max() && den > 0, "level_bound");
    b.level = {{static_cast<u64>(num), 0, 0}, den};
    unsigned arity = 0, ni = 0, ns = 0; in >> arity >> ni >> ns;
    need(arity >= 2 && arity <= 4 && ni <= kBallInteriorMax && ns <= kBallShellMax, "ball_shape");
    b.arity = static_cast<u8>(arity); b.n_interior = static_cast<u8>(ni); b.n_shell = static_cast<u8>(ns);
    for (auto ids : {std::span<i32>(b.interior_ids, ni), std::span<i32>(b.shell_ids, ns)}) {
      for (auto& id : ids) {
        unsigned original; in >> original; need(original < n, "site_range");
        auto found = std::find(index.upos.begin(), index.upos.end(), points[original]);
        need(found != index.upos.end(), "morton_remap"); id = static_cast<i32>(found - index.upos.begin());
      }
      std::sort(ids.begin(), ids.end());
    }
  }
  need(in.good(), "fixture_complete");
  return {std::move(index), std::move(balls), kmax};
}
}

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  try {
    const auto f = load(argv[1]);
    std::size_t rejected = 0, after_prefix = 0, at_seal = 0, semantic_rejections = 0;
    for (int workers : {0, 1, 4}) {
      for (bool sealing : {false, true}) {
        incremental_failure_test::reset(0, sealing, false);
        const auto nominal = build_full_ball_tower(f.index, f.balls, f.kmax, workers);
        incremental_failure_test::counting.store(false);
        const auto allocations = incremental_failure_test::attempts.load();
        need(nominal.status == FullBallStatus::kCompleteRelative && nominal.orders.size() == 5 &&
             incremental_failure_test::armed && allocations > 10, "measured_nominal_prefix_or_seal");
        for (std::size_t fail = 1; fail <= allocations; ++fail) {
          incremental_failure_test::reset(fail, sealing, false);
          const auto result = build_full_ball_tower(f.index, f.balls, f.kmax, workers);
          incremental_failure_test::counting.store(false);
          need(incremental_failure_test::failures.load() == 1, "actual_new_failure_hit");
          need(result.status == FullBallStatus::kResourceExhausted && result.orders.empty(), "failure_no_public_prefix");
          need(incremental_failure_test::armed_after > 2 && result.stats.births > f.index.input_count,
               "failure_after_nontrivial_paid_prefix");
          ++rejected; if (sealing) ++at_seal; else ++after_prefix;
        }
      }
      incremental_failure_test::reset(0, false, true);
      const auto result = build_full_ball_tower(f.index, f.balls, f.kmax, workers);
      need(incremental_failure_test::semantic_injected && result.status == FullBallStatus::kInvariantViolated &&
           result.orders.empty() && std::string_view(result.reason) == "full_ball_structural_certificate",
           "late_invalid_lot_global_rejection");
      ++semantic_rejections;
    }
    incremental_failure_test::reset(0, false, false);
    const auto recovered = build_full_ball_tower(f.index, f.balls, f.kmax);
    incremental_failure_test::counting.store(false);
    need(recovered.status == FullBallStatus::kCompleteRelative && recovered.orders.size() == 5,
         "fresh_builder_after_failed_predecessors");
    need(after_prefix > 100 && at_seal > 30 && semantic_rejections == 3 &&
         incremental_failure_test::poison_checks >= 12, "failure_nonvacuity");
    std::printf("{\"status\":\"passed\",\"allocation_rejections\":%zu,\"after_prefix\":%zu,"
        "\"at_seal\":%zu,\"semantic_rejections\":%zu,\"poison_checks\":%zu,"
        "\"modes\":[0,1,4],\"public_status\":\"not_claimed\"}\n",
        rejected, after_prefix, at_seal, semantic_rejections, incremental_failure_test::poison_checks);
    return 0;
  } catch (const std::exception& error) {
    incremental_failure_test::counting.store(false);
    std::fprintf(stderr, "FAIL %s\n", error.what()); return 1;
  }
}
