// Harnais : geometrie/structure distinctes du travail paye des memos prives par lane.
#pragma once
#include "memo_support.hpp"
#include "tower/forest_parallel.hpp"

namespace forest_parallel_test {
using namespace memo_test;
inline ForestLedger structural(ForestLedger value) { value.descent = {}; return value; }
inline FullTimings sentinel() {
  FullTimings t;
  t.memo_capacity = 7; t.memo_slot_bytes = 11; t.memo_reserved_bytes = 13;
  t.regular_batch_capacity = 17; t.descent_lanes = 19;
  t.lane_memo_capacity = 23; t.lane_memo_reserved_bytes = 29;
  for (auto& order : t.orders) order = {31,37,41,43,47,53,59,61,67,71,73,79,83,89};
  return t;
}
inline bool time_bounds(const OrderTimings& t, u32 workers, u32 lanes, u32 capacity) {
  return t.max_regular_batch <= capacity && t.regular_task_max_ns <= t.regular_dispatch_ns &&
         t.regular_task_sum_ns <= u64{std::min({workers, lanes, capacity})} * t.regular_dispatch_ns &&
         t.regular_dispatch_ns + t.regular_publish_ns + t.extended_ns <= t.plateaus_ns;
}
inline bool inactive(const FullTimings& t) {
  if (t.regular_batch_capacity || t.descent_lanes || t.lane_memo_capacity || t.lane_memo_reserved_bytes) return false;
  for (const auto& o : t.orders)
    if (o.regular_batches || o.regular_cells || o.regular_traces || o.extended_cells || o.max_regular_batch ||
        o.regular_dispatch_ns || o.regular_task_sum_ns || o.regular_task_max_ns || o.regular_publish_ns ||
        o.extended_ns) return false;
  return true;
}
}  // namespace forest_parallel_test
