// Pannes de creation partielle et d'execution : aucun terminate, aucun
// callback admis avant la creation complete, aucune barriere orpheline.
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "../src/parallel/sort.hpp"
#include "../src/cloud/families.hpp"
#include "../src/pipeline/run.hpp"

using namespace mhgp7;

struct Wide {
  u32 key = 0;
  u64 payload[8]{};
  bool operator==(const Wide&) const = default;
};

int main(int argc, char** argv) {
  bool mutant = false;
  if (argc == 2 && std::string(argv[1]) == "--inject=parallel-admit-partial-launch") {
    if (!mutants_enable("parallel-admit-partial-launch")) return 2;
    mutant = true;
  } else if (argc != 1) {
    return 2;
  }
  size_t failures = 0, partial_cases = 0, runtime_cases = 0, admitted = 0;
  const size_t n = 65536;
  for (int mode = 0; mode < (mutant ? 2 : 4); ++mode) {
    std::atomic<size_t> calls{0};
    parallel_detail::launch_started.store(0);
    parallel_detail::launch_fail_after = 2;
    bool caught = false;
    try {
      if (mode == 0) parallel_items(n, 4, [&](size_t, size_t) { ++calls; });
      if (mode == 1) parallel_ranges(n, 4, [&](size_t, size_t, size_t) { ++calls; });
      if (mode == 2) {
        std::vector<u32> v(n, 7);
        parallel_stable_sort_vector(&v, [&](u32 a, u32 b) { ++calls; return a < b; }, 4);
      }
      if (mode == 3) {
        std::vector<Wide> v(n);
        parallel_stable_sort_vector(&v, [&](const Wide& a, const Wide& b) {
          ++calls;
          return a.key < b.key;
        }, 4);
      }
    } catch (const std::system_error& e) {
      caught = e.code() == std::errc::resource_unavailable_try_again;
    }
    parallel_detail::launch_fail_after = (size_t)-1;
    admitted += calls.load();
    if (!caught || parallel_detail::launch_started.load() != 2 ||
        parallel_detail::launch_active.load() != 0) ++failures;
    ++partial_cases;
  }
  if (mutant) {
    std::printf("partial_launch cases=%zu admitted=%zu active=%zu failures=%zu\n",
                partial_cases, admitted, parallel_detail::launch_active.load(), failures);
    return failures == 0 && admitted > 0 ? 4 : 1;
  }
  if (admitted != 0) ++failures;
  for (int mode = 0; mode < 4; ++mode) {
    std::atomic<size_t> calls{0};
    bool caught = false;
    try {
      if (mode == 0) parallel_items(n, 4, [&](size_t, size_t) {
        ++calls;
        throw std::runtime_error("worker_failure");
      });
      if (mode == 1) parallel_ranges(n, 4, [&](size_t, size_t, size_t) {
        ++calls;
        throw std::runtime_error("worker_failure");
      });
      if (mode >= 2) {
        std::vector<u32> v(n);
        std::iota(v.begin(), v.end(), 0);
        // mode 3 throws only when merging two distinct original slices.
        parallel_stable_sort_vector(&v, [&](u32 a, u32 b) {
          if (mode == 2 || a / 16384 != b / 16384) {
            ++calls;
            throw std::runtime_error("worker_failure");
          }
          return a < b;
        }, 4);
      }
    } catch (const std::runtime_error& e) {
      caught = std::string(e.what()) == "worker_failure";
    }
    if (!caught || calls.load() == 0 || parallel_detail::launch_active.load() != 0) ++failures;
    ++runtime_cases;
  }
  // A successful operation following failures must still admit all workers.
  std::vector<u32> values(n);
  std::iota(values.rbegin(), values.rend(), 0);
  const size_t workers = parallel_stable_sort_vector(&values, std::less<u32>{}, 4);
  if (workers != 4 || !std::is_sorted(values.begin(), values.end()) ||
      parallel_detail::launch_active.load() != 0) ++failures;
  const auto input = make_family_input(CloudFamily::kUniform, 400,
                                      cloud_family_default_coord(CloudFamily::kUniform, 400), 3);
  RunOptions options;
  options.threads = 4;
  options.smax = 2;
  options.digest = true;
  const RunResult witness = run_pipeline(input, options);
  if (witness.status != PipelineStatus::kCompleteRegular || witness.digest_all.empty()) ++failures;
  std::atomic<size_t> callbacks{0};
  options.on_forest = [&](u64, const std::vector<ForestEvent>&, const ForestResult&) { ++callbacks; };
  parallel_detail::launch_started.store(0);
  parallel_detail::launch_fail_after = 2;
  const RunResult refused = run_pipeline(input, options);
  parallel_detail::launch_fail_after = (size_t)-1;
  const bool no_payload = refused.digest_raw_candidates.empty() && refused.digest_balls.empty() &&
                          refused.digest_postprefilter.empty() && refused.digest_all.empty() &&
                          refused.digest_forest.empty() && refused.cards.empty() &&
                          refused.total_events == 0 && refused.total_facets == 0 &&
                          refused.total_fusions == 0 && refused.total_deltas == 0 &&
                          refused.total_nodes == 0 && refused.forest_storage.empty() &&
                          refused.sum_parents_by_k.empty() && refused.sum_parents_total == 0;
  if (refused.status != PipelineStatus::kResourceExhausted || !no_payload || callbacks.load() != 0 ||
      parallel_detail::launch_started.load() != 2 || parallel_detail::launch_active.load() != 0) ++failures;
  std::printf("thread_failure partial=%zu runtime=%zu workers=%zu failures=%zu\n",
              partial_cases, runtime_cases, workers, failures);
  return failures == 0 && partial_cases == 4 && runtime_cases == 4 ? 0 : 1;
}
