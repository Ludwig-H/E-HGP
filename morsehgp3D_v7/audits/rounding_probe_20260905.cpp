// Independent audit of rounding guards and bounded pipeline equivalence.
#include <atomic>
#include <cfenv>
#include <cfloat>
#include <cstdio>
#include <limits>
#include <vector>

#include "../src/pipeline/run.hpp"

namespace {
using namespace mhgp7;
int failures = 0;
void check(bool valid, const char* message) {
  if (!valid) { ++failures; std::fprintf(stderr, "FAIL %s\n", message); }
}

std::vector<InputPoint> fixture(unsigned kind) {
  std::vector<InputPoint> points;
  if (kind == 0) {
    // Nondegenerate full-window fixture already pinned by the mono gate.
    const P3 coordinates[] = {
        {31052,37054,53791}, {63099,62295,5489}, {45851,18621,10092},
        {32290,41054,26270}, {35795,23044,15792}, {22475,26532,25195},
        {55919,55323,7531}, {60817,37898,64418}, {48853,14056,27781},
        {26341,59313,45083}, {7417,12277,35399}};
    for (const auto& p : coordinates)
      points.push_back({static_cast<PointId>(points.size() * 17 + 2), p});
  } else if (kind == 1) {
    // An integer regular tetrahedron exercises equal geometric levels.
    points = {{97,{0,0,0}}, {12,{4,4,0}}, {701,{4,0,4}}, {31,{0,4,4}}};
  } else {
    u64 state = 0x19d704e6029ULL + kind;
    const auto next = [&]() {
      state = state * 6364136223846793005ULL + 1442695040888963407ULL;
      return static_cast<i64>((state >> 32) & 65535);
    };
    for (unsigned i = 0; i < 19; ++i) points.push_back({i * 37 + 1, {next(), next(), next()}});
  }
  return points;
}
}  // namespace

int main() {
  using namespace mhgp7;
  const int saved = std::fegetround();
  struct Restore { int mode; ~Restore() { std::fesetround(mode); } } restore{saved};
  check(std::numeric_limits<double>::is_iec559 && FLT_RADIX == 2 &&
        DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024 && FLT_EVAL_METHOD == 0,
        "binary64 evaluation domain");
  check(kFloatFilterCompileEnabled, "default Release enables floating filters");
  const int modes[] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
  const char* names[] = {"nearest", "downward", "upward", "towardzero"};
  u64 runs = 0, positive_certificates = 0, built_grids = 0, exact_fallbacks = 0;
  for (unsigned kind = 0; kind < 5; ++kind) {
    std::string expected;
    for (int threads : {1, 2}) {
      for (unsigned mode = 0; mode < 4; ++mode) {
        check(std::fesetround(modes[mode]) == 0, "rounding mode supported");
        check(float_filter_runtime_enabled() == (mode == 0), "runtime rounding guard");
        std::atomic<unsigned> wrong_worker_mode{0};
        // Each actual worker is visited once, independently of task scheduling.
        parallel_detail::run_threads(2, [&](size_t) {
          if (std::fegetround() != modes[mode]) ++wrong_worker_mode;
        });
        check(wrong_worker_mode.load() == 0, "new worker inherits rounding mode");
        RunOptions options;
        options.digest = true;
        options.threads = threads;
        options.fold_join_before_next_k = true;
        options.complete_silent_incidence = (kind == 0);
        options.cell_grid_min_sites = 0;
        const auto result = run_pipeline(fixture(kind), options);
        check(result.status == PipelineStatus::kCompleteRegular, "pipeline completes");
        check(!result.digest_all.empty() && !result.digest_postprefilter.empty() &&
              result.total_facets > 0 && result.total_deltas > 0, "nonempty scientific object");
        const std::string object = result.digest_postprefilter + ":" + result.digest_all;
        if (expected.empty()) expected = object;
        check(object == expected, "postfilter catalogue and forests invariant across rounding/threads");
        const auto& gen = result.gen;
        const u64 cert = gen.float_cert_neg + gen.float_cert_pos +
                         gen.jung_cert_kill + gen.jung_cert_skip;
        const u64 grids = gen.grids_built[0] + gen.grids_built[1] + gen.grids_built[2];
        if (mode == 0) {
          positive_certificates += cert;
          built_grids += grids;
        } else {
          check(cert == 0 && grids == 0, "disabled filter/grid never claims a certificate");
          exact_fallbacks += gen.float_fallback + gen.jung_fallback;
        }
        check(std::fegetround() == modes[mode], "pipeline preserves caller rounding");
        std::printf("fixture=%u threads=%d rounding=%s complete=%u cert=%llu grids=%llu fallback=%llu object=%s\n",
                    kind, threads, names[mode], kind == 0,
                    static_cast<unsigned long long>(cert), static_cast<unsigned long long>(grids),
                    static_cast<unsigned long long>(gen.float_fallback + gen.jung_fallback), object.c_str());
        ++runs;
      }
    }
  }
  check(runs == 40 && positive_certificates > 0 && built_grids > 0 && exact_fallbacks > 0,
        "non-vacuity: enabled filters and grids, disabled exact fallbacks");
  std::printf("runs=%llu certificates=%llu grids=%llu fallbacks=%llu failures=%d\n",
              static_cast<unsigned long long>(runs), static_cast<unsigned long long>(positive_certificates),
              static_cast<unsigned long long>(built_grids), static_cast<unsigned long long>(exact_fallbacks), failures);
  return failures ? 1 : 0;
}
