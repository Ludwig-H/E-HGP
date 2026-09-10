// Private requested-byte/new-call instrumentation, not process RSS.
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>

void private_residence_marker(const char*, unsigned);

#include "morsehgp3D_v7/src/forest/full_ball_tower.hpp"
#include "morsehgp3D_v7/src/pipeline/generate.hpp"
#include "morsehgp3D_v7/src/cloud/families.hpp"
#include "morsehgp3D_v7/bench/full_gabriel_semantic_digest.hpp"

namespace measured {
struct alignas(std::max_align_t) Header { std::size_t size; unsigned epoch; };
unsigned epoch = 0;
std::size_t live = 0, peak = 0, calls = 0, total = 0;
[[gnu::noinline]] void* allocate(std::size_t size) {
  if (size > static_cast<std::size_t>(-1) - sizeof(Header)) throw std::bad_alloc();
  auto* p = static_cast<Header*>(std::malloc(sizeof(Header) + (size ? size : 1)));
  if (!p) throw std::bad_alloc();
  p->size = size; p->epoch = epoch;
  if (epoch) { live += size; total += size; ++calls; if (live > peak) peak = live; }
  return p + 1;
}
[[gnu::noinline]] void release(void* pointer) noexcept {
  if (!pointer) return;
  auto* p = static_cast<Header*>(pointer) - 1;
  if (epoch && p->epoch == epoch) live -= p->size;
  std::free(p);
}
void begin() { ++epoch; live = peak = calls = total = 0; }
}
void* operator new(std::size_t n) { return measured::allocate(n); }
void* operator new[](std::size_t n) { return measured::allocate(n); }
void operator delete(void* p) noexcept { measured::release(p); }
void operator delete[](void* p) noexcept { measured::release(p); }
void operator delete(void* p, std::size_t) noexcept { measured::release(p); }
void operator delete[](void* p, std::size_t) noexcept { measured::release(p); }

void private_residence_marker(const char* stage, unsigned k) {
  static auto prior = std::chrono::steady_clock::now();
  const auto now = std::chrono::steady_clock::now();
  std::printf("stage=%s K=%u requested_live=%zu requested_peak=%zu new_calls=%zu interval_seconds=%.9f\n",
      stage, k, measured::live, measured::peak, measured::calls,
      std::chrono::duration<double>(now - prior).count());
  prior = now;
}

int main() {
  using namespace mhgp7;
  std::printf("sizeof ExactLevel=%zu FullNode=%zu FullDatedContribution=%zu Population=%zu Ref=%zu Action=%zu Batch=%zu Draft=%zu History=%zu Block=%zu Header=%zu\n",
      sizeof(ExactLevel), sizeof(FullNode), sizeof(FullDatedContribution), sizeof(FullCoveragePopulation),
      sizeof(FullCoverageRef), sizeof(FullCoverageAction), sizeof(FullCoverageBatch), sizeof(full_ball_detail::Draft),
      sizeof(full_ball_detail::History), sizeof(full_ball_detail::Block), sizeof(measured::Header));
  for (int n : {800}) {
    measured::epoch = 0;
    const auto input = make_family_input(CloudFamily::kUniform, n, 65536, 3);
    const auto ix = build_cloud_index(input);
    std::vector<BallCandidate> candidates;
    GenerateOptions options; options.s = 8; options.smax = 11; options.threads = 1;
    GenerateStats gen;
    generate_candidates(ix, options, &candidates, &gen);
    if (gen.cap_refus != kCapRefusNone || gen.invariant_jneg) return 2;
    rle_candidates(&candidates, 1);
    std::vector<Survivor> survivors;
    std::vector<BallData> balls;
    ExpandStats stats;
    prefilter_balls(ix, candidates, 11, 1, &survivors, &stats);
    if (census_balls(ix, candidates, survivors, 11, 12, 1, &balls, &stats) != PipelineStatus::kCompleteRegular) return 2;
    std::vector<BallCandidate>().swap(candidates);
    std::vector<Survivor>().swap(survivors);
    measured::begin();
    const auto start = std::chrono::steady_clock::now();
    const auto tower = build_full_ball_tower(ix, balls, 10);
    const auto end = std::chrono::steady_clock::now();
    const auto live = measured::live, peak = measured::peak, calls = measured::calls, total = measured::total;
    measured::epoch = 0;
    if (tower.status != FullBallStatus::kCompleteRelative) { std::fprintf(stderr, "%s\n", tower.reason); return 1; }
    Sha256 digest;
    std::size_t nodes = 0, refs = 0, contributions = 0, arene_capacity = 0;
    for (const auto& order : tower.orders) {
      const auto& f = order.forest;
      nodes += f.nodes().size(); refs += f.parents().size(); contributions += f.contributions().size();
      arene_capacity += f.nodes().capacity() * sizeof(FullNode) + f.parents().capacity() * sizeof(u64) +
          f.successors().capacity() * sizeof(u64) + f.contributions().capacity() * sizeof(FullDatedContribution) +
          order.lower_nodes.capacity() * sizeof(u64);
      digest.u64le(f.order());
      for (const auto& node : f.nodes()) { full_probe_digest::level_wire(digest, node.level); digest.u64le(node.first); digest.u64le(node.parent_count); }
      for (auto parent : f.parents()) digest.u64le(parent);
      for (auto next : f.successors()) digest.u64le(next);
      for (const auto& c : f.contributions()) {
        full_probe_digest::level_wire(digest, c.level); digest.u64le(c.segment);
        digest.u64le(c.ref.population); digest.u64le(c.ref.shell_mask); digest.u64le(c.ref.include_interior);
      }
      for (auto lower : order.lower_nodes) digest.u64le(lower);
    }
    const auto& bank = *tower.orders.front().forest.populations();
    for (auto p : bank.domain()) digest.u64le(p);
    for (const auto& row : bank.rows()) {
      digest.u64le(row.interior.size()); for (auto p : row.interior) digest.u64le(p);
      digest.u64le(row.shell.size()); for (auto p : row.shell) digest.u64le(p);
    }
    std::printf("n=%d balls=%zu nodes=%zu parents=%zu contributions=%zu populations=%zu new_calls=%zu requested_total=%zu requested_peak=%zu retained=%zu arene_capacity=%zu tower_seconds=%.9f physical_digest=%s\n",
        n, balls.size(), nodes, refs, contributions, bank.rows().size(), calls, total, peak, live,
        arene_capacity, std::chrono::duration<double>(end-start).count(), digest.hex().c_str());
    std::fflush(stdout);
  }
}
