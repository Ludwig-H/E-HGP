#include "lanes/q34_pruning.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace mhgp8 {

Q34WitnessPoolPtr Q34WitnessPool::make(Q34EdgeCoverPtr cover, std::size_t budget) {
  if (!cover) throw std::invalid_argument("mhgp8 witness pool requires an immutable edge cover");
  return Q34WitnessPoolPtr(new Q34WitnessPool(std::move(cover), budget));
}

Q34WitnessPool::Q34WitnessPool(Q34EdgeCoverPtr cover, std::size_t budget)
    : cover_(std::move(cover)) {
  const auto population = cover_->site_count();
  const auto count = std::min(budget, population);
  if (count == 0) return;
  ids_.reserve(count);
  // Positions floor(i*m/count), i=0..count-1, via quotient/remainder
  // increments. No i*m product or remainder+remainder overflow in size_t.
  const auto step = population / count, remainder = population % count;
  std::size_t target = 0, error = 0, consumed = 0, range_id = 0;
  const auto ranges = cover_->ranges();
  const auto order = cover_->index()->spatial_order();
  counter_add(work_.range_visits);
  for (std::size_t i = 0; i < count; ++i) {
    while (target - consumed >= ranges[range_id].size()) {
      consumed += ranges[range_id].size();
      ++range_id;
      counter_add(work_.range_visits);
    }
    ids_.push_back(order[ranges[range_id].first + (target - consumed)]);
    counter_add(work_.selected_sites);
    if (i + 1 == count) break;
    target += step;
    if (remainder != 0 && error >= count - remainder) {
      ++target;
      error -= count - remainder;
    } else error += remainder;
  }
}

std::size_t Q34WitnessPool::retained_bytes() const {
  if (ids_.capacity() > std::numeric_limits<std::size_t>::max() / sizeof(std::size_t))
    throw std::overflow_error("mhgp8 witness pool storage exceeds size_t");
  return ids_.capacity() * sizeof(std::size_t);
}

}  // namespace mhgp8
