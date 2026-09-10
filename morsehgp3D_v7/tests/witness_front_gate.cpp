// Scalar/batched differential and explicit pair ownership. This qualifies
// the host scheduling seam, not a CUDA backend or global WSPD completeness.
#include <array>
#include <cstdio>
#include <string_view>

#include "../src/cloud/families.hpp"
#include "../src/pipeline/witness_front.hpp"

#ifdef MHGP7_TESTING
#error Nominal primitives required
#endif

namespace {
using namespace mhgp7;
u64 checks = 0, cases = 0, rejections = 0, rows = 0, queries = 0, positive_q2 = 0, singleton_reuses = 0;
void need(bool value, const char* reason) { ++checks; if (!value) throw std::runtime_error(reason); }
std::vector<InputPoint> fixture(unsigned scene) {
  if (scene == 0) return {{91, {0,0,0}}, {7, {20,0,0}}};
  if (scene == 1) return {{1,{0,0,0}}, {8,{10,0,0}}, {2,{20,0,0}}, {99,{30,0,0}}, {3,{40,0,0}}};
  if (scene == 2) return {{1,{0,0,0}}, {8,{65535,0,0}}, {2,{65535,65535,0}}, {99,{0,65535,0}}};
  auto points = make_family_input(CloudFamily::kUniform, 24, 65536, 3);
  if (scene == 4) {
    points.push_back({101, points[2].position});
    points.push_back({102, points[2].position});
    points.push_back({103, points[9].position});
  }
  if (scene == 5) for (auto& point : points) point.position.z = 0;
  return points;
}

void equal(const std::vector<MultiAliveRect>& a, const std::vector<MultiAliveRect>& b,
           const GenerateStats& x, const GenerateStats& y) {
  need(a.size() == b.size(), "ordered.size");
  for (size_t i = 0; i < a.size(); ++i) {
    need(a[i].r.a == b[i].r.a && a[i].r.b == b[i].r.b && a[i].mask == b[i].mask, "ordered.rectangle");
    for (unsigned q = 0; q < 3; ++q) need(a[i].core[q] == b[i].core[q], "ordered.core");
  }
  need(x.rect_visited_fused == y.rect_visited_fused && x.wspd_witness_nodes == y.wspd_witness_nodes &&
       x.wspd_corner_evals == y.wspd_corner_evals && x.wave_peak_tasks == y.wave_peak_tasks &&
       x.alive_peak_rects == y.alive_peak_rects, "same.physical_work");
  for (unsigned q = 0; q < 3; ++q)
    need(x.rect_alive[q] == y.rect_alive[q] && x.ledger_emitted_mass[q] == y.ledger_emitted_mass[q] &&
         x.ledger_killed_mass[q] == y.ledger_killed_mass[q], "same.ledger");
}

void ownership(const CloudIndex& ix, const std::vector<MultiAliveRect>& result,
               const GenerateStats& st, const u64 h[3], u8 mask, i64 s) {
  const size_t n = ix.upos.size();
  std::array<std::vector<unsigned>, 3> owners;
  for (auto& lane : owners) lane.assign(n*n, 0);
  for (const auto& row : result) {
    const auto a = ix.range_of(row.r.a), b = ix.range_of(row.r.b);
    need(wspd_detail::separated(ix.box_of(row.r.a), ix.box_of(row.r.b), s, 1), "geometry.separation");
    for (i32 x = a.first; x <= a.last; ++x) for (i32 y = b.first; y <= b.last; ++y) {
      const size_t key = static_cast<size_t>(std::min(x,y))*n + static_cast<size_t>(std::max(x,y));
      for (unsigned q = 0; q < 3; ++q) if (row.mask & (1u << q)) {
        need(++owners[q][key] == 1 && row.core[q] < h[q], "ownership.no_duplicate");
        if (q == 0 && row.core[q]) ++positive_q2;
      }
    }
  }
  for (unsigned q = 0; q < 3; ++q) need(st.ledger_emitted_mass[q] + st.ledger_killed_mass[q] ==
      ((mask & (1u << q)) ? expected_pair_mass(ix) : 0), "ownership.mass");
}

void corpus() {
  u64 generation = 0;
  for (unsigned scene = 0; scene < 6; ++scene) {
    const auto ix = build_cloud_index(fixture(scene)); need(ix.valid, "fixture.index");
    for (i64 s : {8,10,12}) for (u8 mask = 0; mask < 8; ++mask)
      for (const auto h : {std::array<u64,3>{0,0,0}, std::array<u64,3>{1,1,1}, std::array<u64,3>{10,9,8}}) {
        std::vector<MultiAliveRect> reference; GenerateStats baseline;
        alive_rectangles_fused(ix,s,h.data(),mask,1,&reference,&baseline);
        ownership(ix,reference,baseline,h.data(),mask,s);
        for (size_t lot : {size_t{1},size_t{7},size_t{64}}) for (int threads : {1,4}) {
          if (threads == 4 && lot != 64) continue;
          CpuWitnessBatch cpu(ix,threads);
          std::vector<MultiAliveRect> result(1); GenerateStats actual; WitnessFrontWork work;
          const u64 before = generation;
          alive_rectangles_batched(ix,s,h.data(),mask,&result,&actual,lot,generation,&work,cpu);
          need(actual.cap_refus == kCapRefusNone, "front.complete");
          equal(reference,result,baseline,actual);
          need(work.batches > 0 && generation == before + work.batches && work.peak_batch <= lot &&
               work.requests >= work.batches && work.corner_requests <= work.requests, "work.accounting");
          need(work.peak_wave_tasks == std::max<u64>(ix.nodes.size(), actual.wave_peak_tasks), "work.initial_wave_counted");
          rows += result.size(); queries += work.requests; ++cases;
        }
      }
  }
}

struct CorruptBatch : CpuWitnessBatch {
  using CpuWitnessBatch::CpuWitnessBatch;
  u64 calls = 0;
  unsigned fault = 0;
  size_t operator()(std::span<const WitnessBatchRequest> requests, const u64* thresholds,
                    u64 epoch, std::vector<WitnessBatchResult>* results) {
      const auto workers = CpuWitnessBatch::operator()(requests,thresholds,epoch,results);
      if (++calls != 3) return workers;  // require private progress before the fault
      if (fault == 0) results->clear();
      if (fault == 1) results->front().status = WitnessBatchStatus::kUnwritten;
      if (fault == 2) results->front().request_id = ~u64{0};
      if (fault == 3) --results->front().generation;
      if (fault == 4) results->front().counts.c[0] = thresholds[0]+1;
      if (fault == 5) throw std::runtime_error("backend failed after prefix");
      if (fault == 6) results->push_back(results->front());
      if (fault == 7) results->front().status = WitnessBatchStatus::kStackOverflow;
      return workers;
  }
};

void empty_reuse() {
  const auto pair = build_cloud_index(fixture(0));
  CpuWitnessBatch pair_cpu(pair, 1);
  std::vector<MultiAliveRect> output;
  GenerateStats stats;
  WitnessFrontWork work;
  u64 generation = 0;
  const u64 h[3] = {10,9,8};
  alive_rectangles_batched(pair,8,h,7,&output,&stats,7,generation,&work,pair_cpu);
  need(!output.empty() && work.batches > 0 && work.requests > 0 && work.peak_wave_tasks > 0,
       "work.singleton_prior_nonempty");
  const auto before = stats;
  const u64 previous_generation = generation;
  const auto singleton = build_cloud_index(std::vector<InputPoint>{{45,{3,4,5}}});
  need(singleton.valid && singleton.nodes.empty(), "work.singleton_index");
  CpuWitnessBatch singleton_cpu(singleton, 1);
  alive_rectangles_batched(singleton,8,h,7,&output,&stats,7,generation,&work,singleton_cpu);
  need(output.empty() && generation == previous_generation && work.batches == 0 && work.requests == 0 &&
       work.corner_requests == 0 && work.peak_batch == 0 && work.peak_wave_tasks == 0,
       "work.singleton_clears_per_call_only");
  equal({}, {}, before, stats);  // GenerateStats remains cumulative.
  need(before.workers_wspd == stats.workers_wspd, "work.singleton_preserves_workers");
  ++singleton_reuses;
}

void faults() {
  const auto ix = build_cloud_index(fixture(3)); CpuWitnessBatch cpu(ix,1);
  const u64 h[3] = {10,9,8};
  for (unsigned fault = 0; fault < 8; ++fault) {
    std::vector<MultiAliveRect> output(1); GenerateStats stats; stats.rect_visited_fused = 123;
    WitnessFrontWork work; u64 generation = 70;
    CorruptBatch bad(ix,1); bad.fault = fault;
    bool rejected = false;
    try { alive_rectangles_batched(ix,8,h,7,&output,&stats,7,generation,&work,bad); }
    catch (const std::runtime_error&) { rejected = true; }
    need(rejected && bad.calls == 3 && output.empty() && stats.rect_visited_fused == 123 &&
         stats.wspd_witness_nodes == 0, "failure.atomic"); ++rejections;
  }
  for (unsigned mode = 0; mode < 6; ++mode) {
    std::vector<MultiAliveRect> output(1); GenerateStats stats; WitnessFrontWork work;
    u64 generation = mode >= 4 ? ~u64{0} - (mode - 4) : 0;
    const u64 generation_before = generation;
    bool rejected = false;
    try {
      alive_rectangles_batched(ix,mode == 3 ? 7 : 8,h,7,&output,&stats,mode == 2 ? 0 : 7,
          generation,&work,cpu,mode == 0 ? 0 : kMaxWaveTasks,mode == 1 ? 0 : kMaxAliveRects);
    } catch (const std::exception&) { rejected = true; }
    need(output.empty() && (mode < 2 ? stats.cap_refus == (mode == 0 ? kCapRefusWaveTasks : kCapRefusAliveRects)
                                   : rejected), "failure.option_or_capacity"); ++rejections;
    if (mode >= 4)
      need(generation == generation_before && stats.rect_visited_fused == 0 && work.batches == 0,
           "generation.reserved_before_first_query");
  }
  const auto other = build_cloud_index(fixture(4));
  need(other.nodes.size() == ix.nodes.size(), "binding.same_node_range");
  std::vector<MultiAliveRect> output(1); GenerateStats stats; WitnessFrontWork work; u64 generation = 13;
  bool rejected = false;
  try { alive_rectangles_batched(other,8,h,7,&output,&stats,7,generation,&work,cpu); }
  catch (const std::invalid_argument&) { rejected = true; }
  need(rejected && output.empty() && generation == 13 && stats.rect_visited_fused == 0,
       "binding.wrong_cloud_rejected_before_query"); ++rejections;
}
}

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try {
    corpus(); empty_reuse(); faults();
    need(cases == 1728 && rejections == 15 && rows > 1000 && queries > 10000 && positive_q2 > 0 && singleton_reuses == 1,
         "nonvacuity");
    std::printf("witness_front_gate=passed checks=%llu cases=%llu rows=%llu requests=%llu rejections=%llu singleton_reuses=%llu backend=cpu_only\n",
        static_cast<unsigned long long>(checks),static_cast<unsigned long long>(cases),
        static_cast<unsigned long long>(rows),static_cast<unsigned long long>(queries),
        static_cast<unsigned long long>(rejections),static_cast<unsigned long long>(singleton_reuses));
    return 0;
  } catch (const std::exception& error) { std::fprintf(stderr,"witness_front_gate: %s\n",error.what()); return 1; }
}
