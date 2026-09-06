// Private local quotient only. No global resolver, event producer or census.
// Authority: audits/receipts_plateaux_full_20260906/README.md (ceb163f9).
#pragma once
#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>
#include "src/forest/plateau.hpp"

namespace mhgp7::local_plateau_private {
using Mask = u16;
struct LocalCensus {
  BallKey ball;
  std::vector<InputPoint> interior, shell;
};
struct Component {
  // Representative: first interior_prefix sites of the shared interior list,
  // plus representative_shell. Coverage: ALL shared interiors + shell_cover.
  std::size_t interior_prefix = 0;
  Mask representative_shell = 0, shell_cover = 0;
  std::vector<Mask> reduced_members;  // empty for the analytic K<=p case
};
struct LocalRank {
  std::size_t k = 0;
  bool present = false, analytic_interior_hub = false;
  bool inert_sufficient = false, no_strict_local_component = false;
  Mask closed_shell_cover = 0;
  std::vector<Component> strict_components;
  u64 reduced_vertices = 0, strict_cofaces = 0, union_attempts = 0;
};
class ShellTable {
 public:
  // Completeness of I/U relative to an external cloud is an ASSUMPTION.
  // This validates only the supplied local census and BallKey domain.
  static ShellTable prepare(LocalCensus census) {
    check(census.shell.size() >= 2 && census.shell.size() <= 12, "local.shell_size_2_to_12");
    check(census.interior.size() <= std::numeric_limits<PointId>::max() - census.shell.size(),
          "local.identity_representation");
    const auto& key = census.ball;
    check(key.a > 0 && key.a < (static_cast<i128>(1) << 68), "local.BallKey_A_bound");
    check(uabs128(key.c) < (static_cast<u128>(1) << 104), "local.BallKey_C_bound");
    for (unsigned axis = 0; axis < 3; ++axis)
      check(uabs128(key.b[axis]) < (static_cast<u128>(1) << 86), "local.BallKey_B_bound");
    std::vector<PointId> ids;
    std::vector<P3> positions;
    ids.reserve(census.interior.size() + census.shell.size()); positions.reserve(ids.capacity());
    const auto inspect = [&](std::vector<InputPoint>& sites, bool on_shell) {
      std::sort(sites.begin(), sites.end(), [](const auto& a, const auto& b) { return a.id < b.id; });
      for (const auto& site : sites) {
        const auto& p = site.position;
        check(p.x >= 0 && p.x <= 65535 && p.y >= 0 && p.y <= 65535 && p.z >= 0 && p.z <= 65535,
              "local.u16_positions");
        const i128 power = key.power(p);
        check(on_shell ? power == 0 : power < 0, "local.census_classification");
        ids.push_back(site.id); positions.push_back(p);
      }
    };
    inspect(census.interior, false); inspect(census.shell, true);
    std::sort(ids.begin(), ids.end());
    check(std::adjacent_find(ids.begin(), ids.end()) == ids.end(), "local.distinct_ids");
    const auto less = [](const P3& a, const P3& b) {
      if (a.x != b.x) return a.x < b.x;
      return a.y != b.y ? a.y < b.y : a.z < b.z;
    };
    std::sort(positions.begin(), positions.end(), less);
    check(std::adjacent_find(positions.begin(), positions.end(), [&](const P3& a, const P3& b) {
      return !less(a, b) && !less(b, a);
    }) == positions.end(), "local.distinct_positions");
    ShellTable table(std::move(census));
    table.prepare_contains();
    return table;
  }
  const LocalCensus& census() const { return census_; }
  const std::vector<u8>& contains_center() const { return contains_; }
  const std::vector<Mask>& minimal_supports() const { return supports_; }
  unsigned q_min() const { return q_min_; }
  unsigned max_strict_cardinality() const { return h_; }
  const std::array<u64, 5>& support_predicates() const { return predicates_; }
  u64 upward_steps() const { return upward_steps_; }

  LocalRank rank(std::size_t k) const {
    check(k > 0, "local.K_positive");
    LocalRank result;
    result.k = k;
    const std::size_t p = census_.interior.size(), u = census_.shell.size();
    if (k > p + u) return result;  // Empty block, never a birth.
    result.present = true;
    result.closed_shell_cover = static_cast<Mask>((1u << u) - 1);
    result.inert_sufficient = k <= p || k - p <= q_min_ - 2;
    result.no_strict_local_component = k > p && k - p > h_;
    if (k <= p) {
      result.analytic_interior_hub = true;
      result.strict_components.push_back(Component{k, 0, result.closed_shell_cover, {}});
      return result;
    }
    const unsigned t = static_cast<unsigned>(k - p);
    std::vector<int> parent(contains_.size(), -1);
    const auto root = [&](int x) {
      while (parent[static_cast<std::size_t>(x)] != x) {
        parent[static_cast<std::size_t>(x)] = parent[static_cast<std::size_t>(parent[static_cast<std::size_t>(x)])];
        x = parent[static_cast<std::size_t>(x)];
      }
      return x;
    };
    for (unsigned mask = 0; mask < contains_.size(); ++mask)
      if (!contains_[mask] && std::popcount(mask) == static_cast<int>(t)) {
        parent[mask] = static_cast<int>(mask); ++result.reduced_vertices;
      }
    // Each strict (t+1)-mask connects its t-faces by ONE STAR, not a clique.
    for (unsigned mask = 0; mask < contains_.size(); ++mask) {
      if (contains_[mask] || std::popcount(mask) != static_cast<int>(t + 1)) continue;
      ++result.strict_cofaces;
      int first = -1;
      for (unsigned bit = 0; bit < u; ++bit) if (mask & (1u << bit)) {
        const int face = static_cast<int>(mask ^ (1u << bit));
        check(parent[static_cast<std::size_t>(face)] >= 0, "local.strict_face_exists");
        if (first < 0) { first = face; continue; }
        ++result.union_attempts;
        const int a = root(first), b = root(face);
        if (a != b) parent[static_cast<std::size_t>(std::max(a, b))] = std::min(a, b);
      }
    }
    std::vector<int> component_index(contains_.size(), -1);
    for (unsigned mask = 0; mask < contains_.size(); ++mask) {
      if (parent[mask] < 0) continue;
      const int representative = root(static_cast<int>(mask));
      int& index = component_index[static_cast<std::size_t>(representative)];
      if (index < 0) {
        index = static_cast<int>(result.strict_components.size());
        result.strict_components.push_back(Component{p, static_cast<Mask>(mask), 0, {}});
      }
      auto& component = result.strict_components[static_cast<std::size_t>(index)];
      component.reduced_members.push_back(static_cast<Mask>(mask));
      component.shell_cover |= static_cast<Mask>(mask);
    }
    check(result.strict_components.empty() == result.no_strict_local_component, "local.hemisphere_identity");
    return result;
  }

 private:
  LocalCensus census_;
  std::vector<u8> contains_;
  std::vector<Mask> supports_;
  unsigned q_min_ = 0, h_ = 0;
  std::array<u64, 5> predicates_{};
  u64 upward_steps_ = 0;
  explicit ShellTable(LocalCensus c) : census_(std::move(c)) {}
  static void check(bool ok, const char* why) { if (!ok) throw std::invalid_argument(why); }
  void prepare_contains() {
    const unsigned u = static_cast<unsigned>(census_.shell.size());
    contains_.assign(1u << u, 0);
    const BallRat center = ball_center(census_.ball);
    for (unsigned q = 2; q <= 4; ++q) for (unsigned mask = 1; mask < contains_.size(); ++mask) {
      if (std::popcount(mask) != static_cast<int>(q)) continue;
      bool proper = false;
      std::array<P3, 4> points{};
      unsigned at = 0;
      for (unsigned bit = 0; bit < u; ++bit) if (mask & (1u << bit)) {
        proper = proper || contains_[mask ^ (1u << bit)];
        points[at++] = census_.shell[bit].position;
      }
      if (proper) { contains_[mask] = 1; continue; }
      ++predicates_[q];
      const bool closed = q == 2 ? plateau_detail::pair_diametral(center, points[0], points[1])
        : q == 3 ? plateau_detail::triangle_closed(center, points[0], points[1], points[2])
        : plateau_detail::tetra_closed(center, points[0], points[1], points[2], points[3]);
      if (closed) { contains_[mask] = 1; supports_.push_back(static_cast<Mask>(mask)); }
    }
    for (unsigned bit = 0; bit < u; ++bit) for (unsigned mask = 0; mask < contains_.size(); ++mask)
      if (mask & (1u << bit)) { contains_[mask] |= contains_[mask ^ (1u << bit)]; ++upward_steps_; }
    check(contains_.back() && !supports_.empty(), "local.shell_defines_miniball");
    q_min_ = 4;
    for (Mask mask : supports_) q_min_ = std::min(q_min_, static_cast<unsigned>(std::popcount(mask)));
    for (unsigned mask = 0; mask < contains_.size(); ++mask)
      if (!contains_[mask]) h_ = std::max(h_, static_cast<unsigned>(std::popcount(mask)));
  }
};
}  // namespace mhgp7::local_plateau_private
