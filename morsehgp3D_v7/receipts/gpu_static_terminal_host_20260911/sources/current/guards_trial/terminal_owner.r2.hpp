#pragma once

#include <numeric>
#include "terminal.cuh"
#include "owner.hpp"
#include "source/morsehgp3D_v7/src/pipeline/expand.hpp"

namespace mhgp7::gpu_terminal_private {
// Host owner for helper/stub qualification only. It validates metadata and
// sorted identity, NOT the geometric completeness of supplied ball censuses.
class HostOwner {
 public:
  HostOwner(const CloudIndex& index, std::span<const BallData> balls) : index_(index) {
    if (balls.size() > static_cast<size_t>(kAbsentBall)) throw std::invalid_argument("terminal.catalogue_size");
    entries_.reserve(balls.size());
    for (const auto& ball : balls) {
      if (ball.arity < 2 || ball.arity > 4 || ball.n_shell < ball.arity || ball.n_shell > 12 ||
          ball.n_interior > 9 || ball.level.den <= 0 || !spatial::key_admitted(ball.key))
        throw std::invalid_argument("terminal.catalogue_profile");
      entries_.push_back({meb_key::encode_key(ball.key), encode_level(ball.level),
          ball.arity, ball.n_interior, ball.n_shell, 0});
    }
    by_key_.resize(entries_.size());
    std::iota(by_key_.begin(), by_key_.end(), BallId{0});
    std::sort(by_key_.begin(), by_key_.end(), [&](BallId a, BallId b) {
      return compare_keys(entries_[a].key, entries_[b].key) < 0;
    });
    for (size_t i = 1; i < by_key_.size(); ++i)
      if (compare_keys(entries_[by_key_[i - 1]].key, entries_[by_key_[i]].key) == 0)
        throw std::invalid_argument("terminal.duplicate_ball_key");
    snapshot_ = spatial::owner_detail::acquire_snapshot(next_snapshot_);
  }
  HostOwner(const HostOwner&) = delete;
  HostOwner& operator=(const HostOwner&) = delete;
  HostOwner(HostOwner&&) = delete;
  HostOwner& operator=(HostOwner&&) = delete;
  View view() const {
    auto index = index_.view();
    index.snapshot = snapshot_;  // NEW common owner identity, never two borrowed tokens.
    return {index, {entries_.data(), by_key_.data(), snapshot_, static_cast<u32>(entries_.size())}};
  }
 private:
  spatial::IndexOwner index_;
  std::vector<CatalogEntry> entries_;
  std::vector<BallId> by_key_;
  inline static std::atomic<u64> next_snapshot_{1};
  u64 snapshot_ = 0;
};
}  // namespace mhgp7::gpu_terminal_private
