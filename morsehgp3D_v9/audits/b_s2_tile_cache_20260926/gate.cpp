// Causal fixtures for the independent-tile witness cache, no GPU.
#include "lanes/q34_witness_search.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace mhgp9::gen;
void need(bool b, const char* text) { if (!b) throw std::runtime_error(text); }
int main() {
  try {
    const std::vector<Point3> points{{0,0,0},{10,0,0},{1,0,0},{5,0,0},{6,0,0}};
    const auto index=make_q2_cloud_index(prepare_cloud(points));
    Q34WitnessSearchWork reference, candidate;
    Q34WitnessBoundsWork r_bounds,c_bounds;
    Q34WitnessCacheWork cached;
    std::vector<Q34WitnessNode> trace, spare;
    const auto first=filter_q34_witnesses(*index,points[0],points[1],3,6,candidate,c_bounds,trace);
    need(first==0 && !trace.empty(), "cause=tile_cache.first_nonvacuous");
    const auto self=q34_cached_witness_rejections(*index,points[0],points[1],3,6,trace,cached);
    need(self==6, "cause=tile_cache.self_trace");
    const auto second=filter_q34_witnesses(*index,points[0],points[2],3,6,reference,r_bounds,spare);
    need(second==6, "cause=tile_cache.endpoint_reference");
    auto dead=q34_cached_witness_rejections(*index,points[0],points[2],3,6,trace,cached);
#ifdef MHGP9_TILE_CACHE_MUTANT_NO_RETEST
    dead=self; // wrong: a certificate for (0,10) does not certify (0,1).
#endif
    need(dead==0, "cause=tile_cache.endpoint_retest");
    auto duplicate=trace; duplicate.push_back(trace.front());
    bool refused=false;
    try { static_cast<void>(q34_cached_witness_rejections(*index,points[0],points[1],3,6,duplicate,cached)); }
    catch (const std::invalid_argument& e) {
      refused=std::string(e.what())=="mhgp9 gen q34 witness cache nodes overlap on a lane";
    }
    need(refused,"cause=tile_cache.overlapping_trace");
    Q34WitnessSearchWork empty_path, baseline;
    Q34WitnessBoundsWork empty_bounds,baseline_bounds;
    Q34WitnessCacheWork empty_cache;
    for (std::size_t b=1;b<points.size();++b) {
      const auto expected=filter_q34_witnesses(*index,points[0],points[b],3,6,baseline,baseline_bounds,spare);
      const auto skipped=q34_cached_witness_rejections(*index,points[0],points[b],3,6,{},empty_cache);
      const auto observed=filter_q34_witnesses(*index,points[0],points[b],3,
          static_cast<std::uint8_t>(6&~skipped),empty_path,empty_bounds,spare);
      need(expected==observed && skipped==0,"cause=tile_cache.empty_object");
    }
    need(empty_path==baseline && empty_bounds==baseline_bounds && empty_cache.node_tests==0,
         "cause=tile_cache.empty_work");
    std::cout << "{\"status\":\"pass\",\"endpoint_retest\":1,\"self_trace\":1,"
                 "\"overlap_refused\":1,\"empty_cache_queries\":4}\n";
    return 0;
  } catch (const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
