#pragma once

// Audit-only explicit port, base 92c709bc8 (26 September 2026).
// Projection selection: src/gen/pipeline/q2_node_pool.hpp and the generic
// q3/q4 Pool in local_credits.cpp. Certification: spindle/predicates.hpp.
// Unlike the old PreparedRectangle path, factors below are node ranges of
// ONE existing Q2CensusIndex. No cloud copy, local tree or per-job plan.
// Tube credits (tube_credits.hpp) are NOT ported or qualified here.
// Shared K-proposal pool for both lanes; this may propose one extra site
// for q4 versus its historical independent (threshold+1)-proposal pool.
// No historical proof receipts, timing or FULL/GPU qualification inherited.

#include "gen/pipeline/q2_census.hpp"
#include "gen/spindle/predicates.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace mhgp9::audit::factor_plan {
using namespace mhgp9::gen;

enum class Mutant { None, Equality, Overlap, Ids };

inline u64 product(std::size_t a, std::size_t b) {
  if (a != 0 && b > std::numeric_limits<u64>::max() / a)
    throw std::overflow_error("factor product overflows u64");
  return static_cast<u64>(a) * static_cast<u64>(b);
}

struct Credit {
  std::uint8_t q3{}, q4{};
  bool operator==(const Credit&) const = default;
};
struct Work {
  u64 factor_sites{}, selection_visits{}, selection_tests{}, selection_shifts{};
  u64 selected_sites{}, anchor_visits{}, witness_attempts{}, self_skips{};
  u64 q3_credits{}, q4_credits{}, grouping_visits{}, grouping_sort_tests{};
  u64 occupied_classes{}, class_slots{}, cells_tested{}, descriptors{};
  PredicateWork predicates{};
};
struct Group { Credit credit; Range ranks; };
struct Factor {
  Range original_ranks;
  std::vector<std::size_t> proposals;  // ORIGINAL IDs, not spatial ranks.
  std::vector<Credit> credits;        // Original spatial rank order.
  std::vector<std::size_t> grouped;   // Spatial ranks, stable within a class.
  std::vector<Group> groups;
  std::size_t retained_bytes() const {
    return proposals.capacity() * sizeof(std::size_t) +
        credits.capacity() * sizeof(Credit) + grouped.capacity() * sizeof(std::size_t) +
        groups.capacity() * sizeof(Group);
  }
};
struct Block {
  Range a, b;  // Ranges into the two factors' grouped spatial rank arrays.
  std::uint8_t mask{};
};

// Normal path always calls the existing exact eight-corner predicate.
// The equality variant is reachable ONLY from the gate's explicit mutant.
inline bool certify(Lane lane, Point3 a, const Box3& b, Point3 z,
                    PredicateWork& work, Mutant mutant) {
  if (mutant != Mutant::Equality) return universal_witness(lane, a, b, z, work);
  counter_add(work.universal_queries);
  for (unsigned corner = 0; corner != 8; ++corner) {
    counter_add(work.corner_tests);
    counter_add(work.point_tests);
    const auto p = box_corner(b, corner);
    std::array<i64, 3> u{}, v{};
    i64 h = 0;
    for (std::size_t d = 0; d != 3; ++d) {
      u[d] = static_cast<i64>(z[d]) - a[d];
      v[d] = static_cast<i64>(p[d]) - z[d];
      h += u[d] * v[d];
    }
    i128 xi = 0;
    for (std::size_t d = 0; d != 3; ++d) {
      const auto cross = u[(d + 1) % 3] * v[(d + 2) % 3] -
          u[(d + 2) % 3] * v[(d + 1) % 3];
      xi += static_cast<i128>(cross) * cross;
    }
    if (h < 0 || static_cast<i128>(lane == Lane::Q3 ? 3 : 2) * h * h < xi)
      return false;  // Deliberately admits equality.
  }
  return true;
}

// Borrows an immutable index for this synchronous plan's lifetime. The plan
// owns all permutations, classes and descriptors; no borrowed mutable buffer.
// h_ext=0. Within each lane, A\{a} and B\{b} are disjoint populations.
// Credits prove ONLY rejection; no count may seed a later census.
// Cost O(K F + C_A C_B), memory O(F + C_A C_B), C<=K(K-1).
// The global sum F and the occupied-class products are measured, NOT bounded
// subquadratically here. No rejected pair is enumerated or stored.
class Plan final {
 public:
  Plan(const Q2CensusIndex& index, std::size_t an, std::size_t bn,
       unsigned k, std::uint8_t mask, Mutant mutant = Mutant::None)
      : index_(&index), k_(k), mask_(mask), mutant_(mutant) {
    const auto nodes = index.spatial_nodes();
    if (k < 2 || k > 10 || mask == 0 || (mask & ~6U) != 0 ||
        (k == 2 && (mask & 4U) != 0) || an >= nodes.size() || bn >= nodes.size())
      throw std::invalid_argument("invalid factor-plan K/mask/node");
    const auto ar = nodes[an].range, br = nodes[bn].range;
    if (ar.size() == 0 || br.size() == 0 || !(ar.last <= br.first || br.last <= ar.first))
      throw std::invalid_argument("factor-plan factors must be nonempty and disjoint");
    counter_add(work.factor_sites, static_cast<u64>(ar.size()));
    counter_add(work.factor_sites, static_cast<u64>(br.size()));
    a = prepare(ar, nodes[an].box, nodes[bn].box);
    b = prepare(br, nodes[bn].box, nodes[an].box);
    for (const auto& ag : a.groups) for (const auto& bg : b.groups) {
      counter_add(work.cells_tested);
      const auto surviving = surviving_mask(ag.credit, bg.credit);
      if (surviving == 0) continue;
      blocks.push_back({ag.ranks, bg.ranks, surviving});
      counter_add(work.descriptors);
      const auto mass = product(ag.ranks.size(), bg.ranks.size());
      counter_add(union_mass, mass);
      if ((surviving & 2U) != 0) counter_add(q3_mass, mass);
      if ((surviving & 4U) != 0) counter_add(q4_mass, mass);
    }
  }
  Plan(const Plan&) = delete;
  Plan& operator=(const Plan&) = delete;
  Plan(Plan&&) = delete;
  Plan& operator=(Plan&&) = delete;
  [[nodiscard]] std::uint8_t surviving_mask(Credit ac, Credit bc) const {
    const unsigned multiple = mutant_ == Mutant::Overlap ? 2U : 1U;
    std::uint8_t out = 0;
    if ((mask_ & 2U) != 0 && multiple * ac.q3 + bc.q3 < k_ - 1U) out |= 2U;
    if ((mask_ & 4U) != 0 && multiple * ac.q4 + bc.q4 < k_ - 2U) out |= 4U;
    return out;
  }
  [[nodiscard]] std::size_t retained_bytes() const {
    return sizeof(*this) + a.retained_bytes() + b.retained_bytes() + blocks.capacity() * sizeof(Block);
  }
  static constexpr std::size_t preparation_scratch_bytes =
      4 * 100 * sizeof(std::size_t) + 10 * (sizeof(i64) + sizeof(std::size_t));
  Factor a, b;
  std::vector<Block> blocks;
  Work work;
  u64 q3_mass{}, q4_mass{}, union_mass{};

 private:
  struct Proposal { i64 score; std::size_t id; };
  Factor prepare(Range ranks, const Box3& own, const Box3& opposite) {
    Factor f;
    f.original_ranks = ranks;
    const auto points = index_->cloud().points();
    const auto order = index_->spatial_order();
    std::array<i64, 3> direction{};
    for (std::size_t d = 0; d != 3; ++d)
      direction[d] = static_cast<i64>(opposite.low[d]) + opposite.high[d] - own.low[d] - own.high[d];
    const auto capacity = std::min<std::size_t>(ranks.size(), k_);
    std::array<Proposal, 10> pool{};
    std::size_t used = 0;
    for (std::size_t rank = ranks.first; rank != ranks.last; ++rank) {
      counter_add(work.selection_visits);
      const auto id = mutant_ == Mutant::Ids ? rank : order[rank];
      i64 score = 0;  // |score|<=6M^2<2^39 in the certified u18 domain.
      for (std::size_t d = 0; d != 3; ++d) score += direction[d] * points[id][d];
      std::size_t at = 0;
      while (at != used) {
        counter_add(work.selection_tests);
        if (score > pool[at].score || (score == pool[at].score && id < pool[at].id)) break;
        ++at;
      }
      if (at == capacity) continue;
      if (used < capacity) ++used;
      for (auto j = used - 1; j != at; --j) {
        pool[j] = pool[j - 1];
        counter_add(work.selection_shifts);
      }
      pool[at] = {score, id};
    }
    f.proposals.reserve(used);
    for (std::size_t i = 0; i != used; ++i) f.proposals.push_back(pool[i].id);
    counter_add(work.selected_sites, static_cast<u64>(used));
    f.credits.resize(ranks.size());
    for (std::size_t rank = ranks.first; rank != ranks.last; ++rank) {
      counter_add(work.anchor_visits);
      const auto id = order[rank];
      auto& c = f.credits[rank - ranks.first];
      for (const auto witness : f.proposals) {
        if (witness == id) { counter_add(work.self_skips); continue; }
        if ((mask_ & 2U) != 0 && c.q3 < k_ - 1U) {
          counter_add(work.witness_attempts);
          if (certify(Lane::Q3, points[id], opposite, points[witness], work.predicates, mutant_)) {
            ++c.q3;
            counter_add(work.q3_credits);
          }
        }
        if ((mask_ & 4U) != 0 && c.q4 < k_ - 2U) {
          counter_add(work.witness_attempts);
          if (certify(Lane::Q4, points[id], opposite, points[witness], work.predicates, mutant_)) {
            ++c.q4;
            counter_add(work.q4_credits);
          }
        }
      }
    }
    std::array<std::size_t, 100> counts{}, offsets{}, cursor{}, occupied{};
    std::size_t occupied_count = 0;
    for (const auto c : f.credits) {
      counter_add(work.grouping_visits);
      const auto key = static_cast<std::size_t>(c.q3) * 10 + c.q4;
      if (counts[key]++ == 0) occupied[occupied_count++] = key;
    }
    std::sort(occupied.begin(), occupied.begin() + static_cast<std::ptrdiff_t>(occupied_count), [&](std::size_t x, std::size_t y) {
      counter_add(work.grouping_sort_tests);
      return x < y;
    });
    counter_add(work.class_slots, 400);  // Four zeroed 100-slot tables.
    std::size_t prefix = 0;
    for (std::size_t i = 0; i != occupied_count; ++i) {
      const auto key = occupied[i];
      offsets[key] = cursor[key] = prefix;
      prefix += counts[key];
      f.groups.push_back({{static_cast<std::uint8_t>(key / 10), static_cast<std::uint8_t>(key % 10)},
                          {offsets[key], prefix}});
    }
    counter_add(work.occupied_classes, static_cast<u64>(occupied_count));
    f.grouped.resize(ranks.size());
    for (std::size_t i = 0; i != f.credits.size(); ++i) {
      counter_add(work.grouping_visits);
      const auto c = f.credits[i];
      f.grouped[cursor[static_cast<std::size_t>(c.q3) * 10 + c.q4]++] = ranks.first + i;
    }
    return f;
  }
  const Q2CensusIndex* index_;
  unsigned k_;
  std::uint8_t mask_;
  Mutant mutant_;
};
}  // namespace mhgp9::audit::factor_plan
