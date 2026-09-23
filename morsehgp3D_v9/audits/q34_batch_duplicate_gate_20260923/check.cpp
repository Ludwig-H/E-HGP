// Audit sidecar for the unpublished S2 batch path. No product source is included.
#include "pipeline/prepared_cloud.hpp"
#include "pipeline/q2_census.hpp"
#include "pipeline/wspd_q34.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <set>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace {
using namespace mhgp9::gen;

struct Record {
  unsigned arity{};
  std::array<i128, 5> key{};
  std::array<std::size_t, 4> support{};
  std::size_t depth{};
  std::vector<std::size_t> shell;
  bool operator<(const Record& r) const {
    return std::tie(arity, key, support, depth, shell) <
           std::tie(r.arity, r.key, r.support, r.depth, r.shell);
  }
  bool operator==(const Record&) const = default;
};

Record copy(const Q34SeedCandidate& c) {
  Record r;
  r.arity = c.arity;
  r.key = c.ball.coefficients();
  std::copy_n(c.support_ids.begin(), c.arity, r.support.begin());
  std::sort(r.support.begin(), r.support.begin() + c.arity);
  r.depth = c.depth;
  r.shell.insert(r.shell.end(), c.shell_first.begin(), c.shell_first.end());
  r.shell.insert(r.shell.end(), c.shell_second.begin(), c.shell_second.end());
  std::sort(r.shell.begin(), r.shell.end());
  return r;
}

std::set<std::array<i128, 5>> keys(const std::vector<Record>& rows, unsigned arity) {
  std::set<std::array<i128, 5>> out;
  for (const auto& r : rows) if (r.arity == arity) out.insert(r.key);
  return out;
}

struct Observed {
  std::vector<Record> records;
  WspdQ34WitnessWork witness;
  u64 expanded_pairs{}, q3_edges{}, q4_edges{};
};

struct MutationInfo {
  std::size_t drop_rectangle{}, duplicate_rectangle{};
  Q34SurvivingEdge dropped{}, copied{};
};

std::vector<Record> run_engine(Q2CensusIndexPtr index, unsigned k, WspdQ34Options options) {
  std::vector<Record> out;
  const auto result = run_wspd_q34_parallel(index, k, 8, options, 1,
      [&](std::size_t, const Q34SeedCandidate& c) { out.push_back(copy(c)); }, 1);
  (void)result;
  std::sort(out.begin(), out.end());
  return out;
}

Observed run_batch(Q2CensusIndexPtr index, unsigned k, WspdQ34Options options,
                   std::size_t drop, std::size_t duplicate,
                   bool mutate, std::size_t* n_survivors, MutationInfo* mutation = nullptr) {
  Observed out;
  const Q34BatchFilter filter = [=](const Q2CensusIndex& ix, unsigned kk,
                                     std::span<const WspdRectangle> rectangles) {
    auto batch = run_q34_filter_batch_cpu(ix, kk, rectangles, 1);
    *n_survivors = batch.survivors.size();
    if (mutate) {
      if (drop >= batch.survivors.size() || duplicate >= batch.survivors.size() || drop == duplicate)
        throw std::logic_error("invalid audit mutation index");
      // Preserve length, lane mask, rectangle masks, counts and visit totals.
      if (batch.survivors[drop].mask != batch.survivors[duplicate].mask)
        throw std::logic_error("audit mutation requires matching lane masks");
      const auto nodes = ix.spatial_nodes();
      const auto owner = [&](const Q34SurvivingEdge& edge) {
        std::size_t found = rectangles.size();
        for (std::size_t i = 0; i < rectangles.size(); ++i) {
          const auto a = nodes[rectangles[i].a_node].range;
          const auto b = nodes[rectangles[i].b_node].range;
          if (a.first <= edge.a_rank && edge.a_rank < a.last &&
              b.first <= edge.b_rank && edge.b_rank < b.last) {
            if (found != rectangles.size()) throw std::logic_error("audit edge belongs to two rectangles");
            found = i;
          }
        }
        if (found == rectangles.size()) throw std::logic_error("audit edge belongs to no rectangle");
        return found;
      };
      if (mutation) {
        mutation->dropped = batch.survivors[drop];
        mutation->copied = batch.survivors[duplicate];
        mutation->drop_rectangle = owner(mutation->dropped);
        mutation->duplicate_rectangle = owner(mutation->copied);
      }
      batch.survivors[drop] = batch.survivors[duplicate];
    }
    return batch;
  };
  WspdQ34BatchTiming timing;
  const auto result = run_wspd_q34_batched(index, k, 8, options, 1,
      [&](std::size_t, const Q34SeedCandidate& c) { out.records.push_back(copy(c)); }, 1,
      filter, &timing);
  // Returning means validate_completion accepted the batch.
  out.witness = result.pipeline.work.witness;
  out.expanded_pairs = result.pipeline.work.expanded_pairs;
  out.q3_edges = result.pipeline.work.q3_edges;
  out.q4_edges = result.pipeline.work.q4_edges;
  std::sort(out.records.begin(), out.records.end());
  return out;
}
}  // namespace

int main() {
  try {
    // Strictly acute triangle: the longest edge owns its q3 circumcircle.
    const std::array<Point3, 3> points{{{0, 0, 0}, {100, 0, 0}, {30, 60, 0}}};
    const auto index = make_q2_cloud_index(prepare_cloud(points));
    WspdQ34Options options;
    options.requested_lane_mask = 2;
    options.front_mode = WspdFrontMode::Pure;
    options.witness_mode = WspdQ34WitnessMode::RectanglePair;
    options.witness_bounds_mode = Q34WitnessBoundsMode::Affine;
    const auto engine = run_engine(index, 3, options);
    std::size_t survivors = 0;
    const auto honest = run_batch(index, 3, options, 0, 0, false, &survivors);
    if (engine != honest.records || keys(honest.records, 3).empty() || survivors < 2) {
      std::fprintf(stderr, "honest batch and engine disagree or fixture has no q3/two survivors\n");
      return 1;
    }
    for (std::size_t drop = 0; drop < survivors; ++drop) {
      for (std::size_t duplicate = 0; duplicate < survivors; ++duplicate) {
        if (drop == duplicate) continue;
        try {
          std::size_t count = 0;
          MutationInfo mutation;
          const auto altered = run_batch(index, 3, options, drop, duplicate, true, &count, &mutation);
          const auto expected = keys(honest.records, 3), actual = keys(altered.records, 3);
          std::size_t missing = 0;
          for (const auto& key : expected) missing += !actual.contains(key);
          if (missing != 0 && count == survivors && mutation.drop_rectangle != mutation.duplicate_rectangle &&
              honest.witness == altered.witness && honest.expanded_pairs == altered.expanded_pairs &&
              honest.q3_edges == altered.q3_edges && honest.q4_edges == altered.q4_edges) {
            std::printf("PASS sites=%zu K=3 survivors=%zu drop=%zu duplicate=%zu engine_records=%zu "
                        "honest_records=%zu altered_records=%zu honest_q3_keys=%zu altered_q3_keys=%zu missing_q3_keys=%zu "
                        "drop_rectangle=%zu duplicate_rectangle=%zu dropped_ranks=%u,%u copied_ranks=%u,%u "
                        "witness_counters_equal=1 expanded_pairs=%llu q3_edges=%llu validate_completion=accepted\n",
                        points.size(), survivors, drop, duplicate, engine.size(), honest.records.size(),
                        altered.records.size(), expected.size(), actual.size(), missing,
                        mutation.drop_rectangle, mutation.duplicate_rectangle,
                        mutation.dropped.a_rank, mutation.dropped.b_rank,
                        mutation.copied.a_rank, mutation.copied.b_rank,
                        static_cast<unsigned long long>(honest.expanded_pairs),
                        static_cast<unsigned long long>(honest.q3_edges));
            return 0;
          }
        } catch (const std::logic_error&) {
          // Different lane masks are ineligible for this same-mask mutation.
        }
      }
    }
    std::fprintf(stderr, "no same-mask duplicate/drop erased a q3 key\n");
    return 1;
  } catch (const std::exception& e) {
    std::fprintf(stderr, "audit error: %s\n", e.what());
    return 2;
  }
}
