#define main old_gate_main
#include "check.cpp"
#undef main

#include <string>

int main() {
  const std::array<Point3, 3> points{{{0, 0, 0}, {100, 0, 0}, {30, 60, 0}}};
  const auto index = make_q2_cloud_index(prepare_cloud(points));
  WspdQ34Options options;
  options.requested_lane_mask = 2;
  options.front_mode = WspdFrontMode::Pure;
  options.witness_mode = WspdQ34WitnessMode::RectanglePair;
  options.witness_bounds_mode = Q34WitnessBoundsMode::Affine;

  const auto engine = run_engine(index, 3, options);
  std::size_t honest_survivors = 0;
  const auto honest = run_batch(index, 3, options, 0, 0, false, &honest_survivors);
  if (engine != honest.records || keys(honest.records, 3).empty() || honest_survivors != 3) return 1;

  std::size_t altered_survivors = 0;
  MutationInfo mutation;
  try {
    static_cast<void>(run_batch(index, 3, options, 1, 0, true, &altered_survivors, &mutation));
    return 2;
  } catch (const std::logic_error& error) {
    if (altered_survivors != honest_survivors || mutation.drop_rectangle == mutation.duplicate_rectangle ||
        std::string(error.what()) !=
            "mhgp9 gen batched q34 filter returned a duplicate, unordered or widened pair") return 3;
    std::printf("duplicate_replacement=refused survivors=%zu->%zu drop_rect=%zu copy_rect=%zu reason=%s\n",
                honest_survivors, altered_survivors, mutation.drop_rectangle, mutation.duplicate_rectangle,
                error.what());
  }

  std::size_t foreign_survivors = 0;
  u64 expanded_before = 0, expanded_after = 0;
  std::uint32_t original_b = 0, corrupted_b = 0;
  Q34BatchFilter foreign = [&](const Q2CensusIndex& ix, unsigned k, std::span<const WspdRectangle> rectangles) {
    auto batch = run_q34_filter_batch_cpu(ix, k, rectangles, 1);
    foreign_survivors = batch.survivors.size();
    if (foreign_survivors == 0) throw std::logic_error("empty test batch");
    expanded_before = batch.expanded_pairs;
    auto& edge = batch.survivors.back();
    original_b = edge.b_rank;
    edge.b_rank = edge.a_rank;
    corrupted_b = edge.b_rank;
    expanded_after = batch.expanded_pairs;
    return batch;
  };
  try {
    static_cast<void>(run_wspd_q34_batched(index, 3, 8, options, 1,
        [](std::size_t, const Q34SeedCandidate&) {}, 1, foreign, nullptr));
    return 4;
  } catch (const std::logic_error& error) {
    if (expanded_before != expanded_after || original_b == corrupted_b ||
        std::string(error.what()) !=
            "mhgp9 gen batched q34 filter returned a pair outside its surviving rectangles") return 5;
    std::printf("foreign_pair=refused survivors=%zu expanded=%llu->%llu b_rank=%u->%u reason=%s\n",
                foreign_survivors, static_cast<unsigned long long>(expanded_before),
                static_cast<unsigned long long>(expanded_after), original_b, corrupted_b, error.what());
  }
  return 0;
}
