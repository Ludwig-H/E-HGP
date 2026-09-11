#pragma once

#include <array>
#include "terminal_owner.hpp"

namespace mhgp7::gpu_terminal_private {
// Immutable owner, built from the same full census as the catalogue. Validation
// below binds identities/populations, not geometric catalogue completeness.
class SeedOwner {
 public:
  SeedOwner(const CloudIndex& index, std::span<const BallData> balls) : owner_(index, balls) {
    for (size_t id = 0; id < balls.size(); ++id) {
      const auto& ball = balls[id];
      const size_t k = static_cast<size_t>(ball.n_interior) + ball.n_shell;
      if (k < 2 || k > 10) continue;
      Seed seed;
      seed.target = static_cast<BallId>(id);
      size_t cursor = 0;
      for (i32 site : ball.interior()) seed.selected[cursor++] = site;
      for (i32 site : ball.shell()) seed.selected[cursor++] = site;
      std::sort(seed.selected, seed.selected + k);
      for (size_t j = 0; j < k; ++j)
        if (seed.selected[j] < 0 || static_cast<size_t>(seed.selected[j]) >= index.upos.size() ||
            (j && seed.selected[j - 1] >= seed.selected[j]))
          throw std::invalid_argument("seed.population_indices");
      seeds_[k].push_back(seed);
    }
    for (auto& seeds : seeds_) {
      std::sort(seeds.begin(), seeds.end(), [](const Seed& a, const Seed& b) {
        return compare_seed(a.selected, b.selected) < 0;
      });
      for (size_t j = 1; j < seeds.size(); ++j)
        if (compare_seed(seeds[j - 1].selected, seeds[j].selected) == 0)
          throw std::invalid_argument("seed.duplicate_population");
    }
  }
  SeedOwner(const SeedOwner&) = delete;
  SeedOwner& operator=(const SeedOwner&) = delete;
  View view() const { return owner_.view(); }
  SeedView seeds(u32 k) const {
    if (k < 2 || k > 10) throw std::invalid_argument("seed.order");
    return {seeds_[k].data(), seeds_[k].size(), view().index.snapshot, k};
  }
 private:
  HostOwner owner_;
  std::array<std::vector<Seed>, 11> seeds_;
};
}  // namespace mhgp7::gpu_terminal_private
