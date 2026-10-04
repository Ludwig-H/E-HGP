// Harnais de la nouvelle option ; les compteurs de structure restent distincts du travail des memos.
#pragma once
#include "forest_parallel_support.hpp"
#include "tower/forest_vertical_seed.hpp"

namespace forest_vertical_test {
using namespace forest_parallel_test;
inline bool vertical_inactive(const OrderTimings& t) {
  return t.vertical_batches == 0 && t.vertical_resolutions == 0 && t.max_vertical_batch == 0 &&
         t.vertical_dispatch_ns == 0 && t.vertical_task_sum_ns == 0 && t.vertical_task_max_ns == 0 &&
         t.vertical_sweep_ns == 0;
}
inline bool vertical_bounds(const OrderTimings& t, u32 w, u32 l, u32 q) {
  return t.max_vertical_batch <= q && t.vertical_task_max_ns <= t.vertical_dispatch_ns &&
         t.vertical_task_sum_ns <= u64{std::min({w,l,q})} * t.vertical_dispatch_ns &&
         t.vertical_dispatch_ns + t.vertical_sweep_ns <= t.verticals_ns;
}
inline FullTimings marked() {
  auto t = sentinel(); t.parallel_verticals = true;
  for (auto& o : t.orders) {
    o.vertical_batches = 97; o.vertical_resolutions = 101; o.max_vertical_batch = 103;
    o.vertical_dispatch_ns = 107; o.vertical_task_sum_ns = 109; o.vertical_task_max_ns = 113;
    o.vertical_sweep_ns = 127;
  }
  return t;
}
}  // namespace forest_vertical_test
