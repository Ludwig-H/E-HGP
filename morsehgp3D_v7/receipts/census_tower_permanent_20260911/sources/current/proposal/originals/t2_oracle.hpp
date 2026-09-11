// Independent bounded test oracle. Never include from product code.
#pragma once
#include "source/morsehgp3D_v7/oracle/local_plateau_oracle.hpp"

namespace mhgp7::t2_plateau_oracle {
using local_plateau_oracle::Int;
using local_plateau_oracle::Rat;
using local_plateau_oracle::Vec;
using local_plateau_oracle::Ball;
namespace detail = local_plateau_oracle::detail;
enum class Mutation { none, skip_full_assignment, close_open_cut, omit_adjacency };

class Model {
 public:
  explicit Model(std::vector<P3> points, Mutation mutation = Mutation::none)
      : points_(std::move(points)), mutation_(mutation) {
    if (points_.empty() || points_.size() > 14)
      throw std::invalid_argument("T2 oracle requires 1..14 points");
    for (size_t i = 0; i < points_.size(); ++i) {
      const auto& p = points_[i];
      if (p.x < 0 || p.x > 65535 || p.y < 0 || p.y > 65535 || p.z < 0 || p.z > 65535)
        throw std::invalid_argument("T2 u16 profile");
      for (size_t j = 0; j < i; ++j)
        if (p == points_[j]) throw std::invalid_argument("T2 distinct positions");
    }
    domain_ = (u32{1} << points_.size()) - 1;
    ball_ids_.assign(domain_ + 1, missing);
    for (u32 support = 1; support <= domain_; ++support) {
      if (std::popcount(support) > 4) continue;
      ++gram_calls;
      const auto ball = detail::support_ball(points_, support);
      if (!ball) continue;
      ++positive_supports;
      u32 enclosed = 0;
      for (size_t i = 0; i < points_.size(); ++i) {
        const auto delta = detail::difference(detail::point(points_[i]), ball->center);
        if (detail::dot(delta, delta) <= ball->radius2) enclosed |= u32{1} << i;
      }
      if ((enclosed & support) != support) throw std::logic_error("T2 support not enclosed");
      u32 id = missing;
      for (u32 i = 0; i < unique_.size(); ++i)
        if (unique_[i].center == ball->center && unique_[i].radius2 == ball->radius2) { id = i; break; }
      if (id == missing) { id = static_cast<u32>(unique_.size()); unique_.push_back(*ball); }
      const u32 remaining = enclosed & ~support;
      for (u32 subset = remaining;; subset = (subset - 1) & remaining) {
        const u32 mask = support | subset;
        if (mutation_ != Mutation::skip_full_assignment || mask != domain_) {
          if (ball_ids_[mask] != missing && ball_ids_[mask] != id)
            throw std::logic_error("T2 incompatible positive enclosing supports");
          ball_ids_[mask] = id;
          ++assignments;
        }
        if (subset == 0) break;
      }
    }
    for (u32 mask = 1; mask <= domain_; ++mask)
      if (ball_ids_[mask] == missing) throw std::logic_error("T2 unassigned subset");
  }

  Ball meb(u32 mask) const {
    if (!mask || (mask & ~domain_)) throw std::invalid_argument("T2 MEB mask");
    return unique_[ball_ids_[mask]];
  }

  std::vector<std::vector<u32>> components(unsigned k, const Rat& radius2,
                                         bool closed, u32 allowed_mask) const {
    if (!k || (allowed_mask & ~domain_)) throw std::invalid_argument("T2 component query");
    if (mutation_ == Mutation::close_open_cut) closed = true;
    std::vector<bool> active(unique_.size());
    for (size_t i = 0; i < unique_.size(); ++i)
      active[i] = closed ? unique_[i].radius2 <= radius2 : unique_[i].radius2 < radius2;
    const auto present = [&](u32 mask) { return active[ball_ids_[mask]]; };
    std::vector<bool> seen(domain_ + 1, false);
    std::vector<std::vector<u32>> result;
    for (u32 mask = 1; mask <= domain_; ++mask) {
      if ((mask & ~allowed_mask) || static_cast<unsigned>(std::popcount(mask)) != k ||
          seen[mask] || !present(mask)) continue;
      std::vector<u32> component{mask};
      seen[mask] = true;
      for (size_t next = 0; next < component.size(); ++next) {
        const u32 facet = component[next];
        if (mutation_ == Mutation::omit_adjacency) continue;
        for (u32 outside = allowed_mask & ~facet; outside; outside &= outside - 1) {
          const u32 coface = facet | (u32{1} << std::countr_zero(outside));
          if (!present(coface)) continue;
          for (u32 bits = coface; bits; bits &= bits - 1) {
            const u32 neighbor = coface & ~(u32{1} << std::countr_zero(bits));
            if (!present(neighbor)) throw std::logic_error("T2 MEB monotonicity");
            if (!seen[neighbor]) { seen[neighbor] = true; component.push_back(neighbor); }
          }
        }
      }
      std::sort(component.begin(), component.end());
      result.push_back(std::move(component));
    }
    std::sort(result.begin(), result.end());
    return result;
  }

  u64 gram_calls = 0, positive_supports = 0, assignments = 0;
 private:
  static constexpr u32 missing = ~u32{0};
  std::vector<P3> points_;
  u32 domain_ = 0;
  std::vector<Ball> unique_;
  std::vector<u32> ball_ids_;
  Mutation mutation_;
};
}  // namespace mhgp7::t2_plateau_oracle
