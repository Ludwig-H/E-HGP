// Private CPU atlas: prevalidated borrowed census, no terminal or calendar.
#pragma once
#include <bit>
#include <numeric>
#include <optional>
#include "source/morsehgp3D_v7/src/forest/local_plateau.hpp"
#include "source/morsehgp3D_v7/src/pipeline/expand.hpp"

namespace mhgp7::rank_atlas_private {
using BallId = u32;
using BlockId = u64;
using Mask = u16;
inline void require(bool value, const char* why) {
  if (!value) throw std::invalid_argument(why);
}
struct BallSlot {
  u64 base = 0;  // Cell index, excluding implicit point births.
  u32 rank = 0;  // Common exact level rank; zero is reserved for point births.
  u8 lo = 0, hi = 0;
};
struct Contribution {
  // PointId-sorted shell order, as in FullPopulationBank (NOT BallData order).
  Mask shell = 0;
  bool interior = false;
};
struct Work {
  u64 regular_cells = 0, extra_tables = 0, extra_ranks = 0;
  u64 reduced_vertices = 0, strict_cofaces = 0, union_attempts = 0, dsu_mask_slots = 0;
};
class Atlas {
 public:
  // PRECONDITION: authentic immutable index and complete exact census accepted
  // by the FULL catalogue contract, including q_min == arity and unique keys.
  // This helper does NOT establish completeness or requalify support geometry.
  // All storage is built transactionally in this returned object. Borrowed
  // index/census must keep the same identity, contents and lifetime afterwards.
  static Atlas build(const CloudIndex& index, std::span<const BallData> balls, unsigned requested) {
    Atlas result(index, balls, requested);
    result.prepare();
    result.validate_shape();
    return result;
  }
  const CloudIndex* index;
  std::span<const BallData> balls;
  unsigned kmax;
  std::vector<BallSlot> slots;
  std::vector<BallId> level_balls;  // One borrowed raw level per positive rank.
  std::vector<BallId> program_balls;  // Flat per-K permutation; no K*C census.
  std::vector<u64> k_offsets, rep_offsets;
  std::vector<Contribution> contributions;
  std::vector<Mask> rep_masks;  // Each mask is in ORIGINAL BallData shell order.
  Work work;

  std::optional<BlockId> block_id(BallId b, unsigned k) const {
    require(b < slots.size(), "atlas.ball_id");
    const auto& row = slots[b];
    if (k < row.lo || k > row.hi) return std::nullopt;
    return index->input_count + row.base + k - row.lo;
  }
  u64 cell(BallId b, unsigned k) const {
    const auto id = block_id(b, k);
    require(id.has_value(), "atlas.absent_block");
    return *id - index->input_count;
  }
  std::span<const BallId> program(unsigned k) const {
    require(k >= 1 && k <= kmax, "atlas.K");
    return std::span<const BallId>(program_balls).subspan(k_offsets[k], k_offsets[k + 1] - k_offsets[k]);
  }
  std::span<const Mask> masks(BallId b, unsigned k) const {
    const u64 c = cell(b, k);
    return std::span<const Mask>(rep_masks).subspan(rep_offsets[c], rep_offsets[c + 1] - rep_offsets[c]);
  }
  // Temporary expansion only. The atlas never retains a 40-byte facet key.
  std::vector<i32> expand(BallId b, Mask mask) const {
    const auto& ball = balls[b];
    require((mask >> ball.n_shell) == 0, "atlas.mask_domain");
    std::vector<i32> key(ball.interior().begin(), ball.interior().end());
    for (unsigned j = 0; j < ball.n_shell; ++j) if (mask & (Mask{1} << j)) key.push_back(ball.shell_ids[j]);
    std::sort(key.begin(), key.end());
    return key;
  }
  // Block IDs 0..n-1 are K1 point births in geometric-index order, rank zero.
  // These are internal atlas IDs, NOT exported forest/population IDs.
  u32 point_rank(i32 geometry_id) const {
    require(geometry_id >= 0 && static_cast<size_t>(geometry_id) < index->upos.size(), "atlas.point_id");
    return 0;
  }
  const ExactLevel& raw_level(BallId b) const { return balls[b].level; }
  u64 logical_bytes() const {
    return slots.size() * sizeof(BallSlot) + level_balls.size() * sizeof(BallId) +
        program_balls.size() * sizeof(BallId) + k_offsets.size() * sizeof(u64) +
        rep_offsets.size() * sizeof(u64) + contributions.size() * sizeof(Contribution) + rep_masks.size() * sizeof(Mask);
  }
  u64 capacity_bytes() const {
    return slots.capacity() * sizeof(BallSlot) + level_balls.capacity() * sizeof(BallId) +
        program_balls.capacity() * sizeof(BallId) + k_offsets.capacity() * sizeof(u64) +
        rep_offsets.capacity() * sizeof(u64) + contributions.capacity() * sizeof(Contribution) + rep_masks.capacity() * sizeof(Mask);
  }

  // Structural/date checks only. Mask semantics are compared to visit_block by
  // the separate gate, not "certified" by cardinality or a plausible graph.
  void validate_shape() const {
    require(slots.size() == balls.size() && k_offsets.size() == kmax + 2 &&
        k_offsets[0] == 0 && k_offsets[1] == 0 && k_offsets.back() == program_balls.size(), "atlas.program_offsets");
    require(rep_offsets.size() == contributions.size() + 1 && rep_offsets.front() == 0 &&
        rep_offsets.back() == rep_masks.size(), "atlas.rep_offsets");
    const ExactLevel zero{{0, 0, 0}, 1};
    for (size_t j = 0; j < level_balls.size(); ++j) {
      require(level_balls[j] < balls.size(), "atlas.rank_ball_id");
      require(compare_exact_level(j ? balls[level_balls[j - 1]].level : zero,
          balls[level_balls[j]].level) < 0, "atlas.rank_order");
    }
    u64 count = 0;
    for (size_t b = 0; b < balls.size(); ++b) {
      const auto& row = slots[b]; const auto& ball = balls[b];
      require(row.lo == ball.n_interior + ball.arity - 1 &&
          row.hi == std::min<unsigned>(kmax, ball.n_interior + ball.n_shell) && row.lo <= row.hi &&
          row.base == count, "atlas.admission_base");
      require(row.rank > 0 && row.rank <= level_balls.size() &&
          same_exact_level(ball.level, balls[level_balls[row.rank - 1]].level), "atlas.rank_date");
      for (unsigned k = row.lo; k <= row.hi; ++k) {
        require(count < contributions.size() && rep_offsets[count] <= rep_offsets[count + 1] &&
            rep_offsets[count + 1] <= rep_masks.size(), "atlas.rep_interval");
        const auto c = contributions[count];
        require((c.shell >> ball.n_shell) == 0 && (!c.interior || ball.n_interior), "atlas.contribution_domain");
        for (u64 r = rep_offsets[count]; r < rep_offsets[count + 1]; ++r)
          require((rep_masks[r] >> ball.n_shell) == 0 &&
              std::popcount(rep_masks[r]) + ball.n_interior == static_cast<int>(k), "atlas.mask_cardinality");
        ++count;
      }
    }
    require(count == contributions.size() && count == program_balls.size(), "atlas.cell_count");
    std::vector<u8> seen(count, 0);
    for (unsigned k = 1; k <= kmax; ++k) {
      require(k_offsets[k] <= k_offsets[k + 1] && k_offsets[k + 1] <= program_balls.size(), "atlas.program_interval");
      bool first = true; BallId previous = 0;
      for (BallId b : program(k)) {
        require(b < balls.size(), "atlas.program_ball_id");
        const u64 c = cell(b, k);
        require(!seen[c]++, "atlas.program_unique");
        if (!first) {
          const int comparison = compare_exact_level(balls[previous].level, balls[b].level);
          require(comparison < 0 || (comparison == 0 && balls[previous].key < balls[b].key), "atlas.program_order");
        }
        first = false; previous = b;
      }
    }
    require(std::all_of(seen.begin(), seen.end(), [](u8 value) { return value == 1; }), "atlas.program_complete");
  }

 private:
  Atlas(const CloudIndex& ix, std::span<const BallData> census, unsigned requested)
      : index(&ix), balls(census), kmax(static_cast<unsigned>(std::min<u64>(requested, ix.input_count))) {
    require(requested > 0 && requested <= 10 && ix.valid && !ix.upos.empty() && !ix.has_duplicate_positions() &&
        ix.input_count <= static_cast<u64>(std::numeric_limits<i32>::max()) &&
        census.size() <= std::numeric_limits<BallId>::max(), "atlas.precondition_shape");
  }
  void prepare() {
    slots.resize(balls.size());
    std::vector<BallId> sorted(balls.size());
    std::iota(sorted.begin(), sorted.end(), BallId{0});
    std::sort(sorted.begin(), sorted.end(), [&](BallId a, BallId b) { return balls[a].key < balls[b].key; });
    std::stable_sort(sorted.begin(), sorted.end(), [&](BallId a, BallId b) {
      return compare_exact_level(balls[a].level, balls[b].level) < 0;
    });
    for (BallId b : sorted) {
      if (level_balls.empty() || !same_exact_level(balls[level_balls.back()].level, balls[b].level)) level_balls.push_back(b);
      slots[b].rank = static_cast<u32>(level_balls.size());
    }
    k_offsets.assign(kmax + 2, 0);
    u64 cells = 0;
    for (size_t b = 0; b < balls.size(); ++b) {
      const auto& ball = balls[b]; auto& row = slots[b];
      require(ball.arity >= 2 && ball.arity <= 4 && ball.n_shell >= ball.arity &&
          ball.n_shell <= kBallShellMax && ball.n_interior <= kBallInteriorMax && ball.level.den > 0,
          "atlas.census_shape");
      row.lo = static_cast<u8>(ball.n_interior + ball.arity - 1);
      row.hi = static_cast<u8>(std::min<unsigned>(kmax, ball.n_interior + ball.n_shell));
      require(row.lo <= row.hi && ball.n_interior + ball.arity <= std::min<u64>(kmax + 1, index->input_count),
          "atlas.rank_window");
      row.base = cells; cells += row.hi - row.lo + 1;
      for (unsigned k = row.lo; k <= row.hi; ++k) ++k_offsets[k + 1];
    }
    for (unsigned k = 1; k <= kmax; ++k) k_offsets[k + 1] += k_offsets[k];
    require(cells <= std::numeric_limits<size_t>::max() - 1 - index->input_count, "atlas.cell_representation");
    program_balls.resize(cells);
    auto next = k_offsets;
    for (BallId b : sorted) for (unsigned k = slots[b].lo; k <= slots[b].hi; ++k) program_balls[next[k]++] = b;
    contributions.resize(cells);
    rep_offsets.reserve(cells + 1); rep_offsets.push_back(0);
    for (size_t bi = 0; bi < balls.size(); ++bi) {
      const auto& b = balls[bi]; const auto& row = slots[bi];
      const auto all = static_cast<Mask>((1u << b.n_shell) - 1);
      std::optional<local_plateau::ShellTable> table;
      std::array<Mask, kBallShellMax> to_original{};
      if (b.n_shell != b.arity) {
        local_plateau::LocalCensus local{b.key, {}, {}};
        for (i32 site : b.interior()) local.interior.push_back({index->point_id(site), index->upos[site]});
        for (i32 site : b.shell()) local.shell.push_back({index->point_id(site), index->upos[site]});
        table.emplace(local_plateau::ShellTable::prepare(std::move(local))); ++work.extra_tables;
        require(table->q_min() == b.arity, "atlas.minimum_arity");
        for (unsigned j = 0; j < b.n_shell; ++j) {
          for (unsigned t = 0; t < b.n_shell; ++t)
            if (table->census().shell[j].id == index->point_id(b.shell_ids[t])) to_original[j] = static_cast<Mask>(Mask{1} << t);
          require(to_original[j] != 0, "atlas.shell_identity");
        }
      }
      for (unsigned k = row.lo; k <= row.hi; ++k) {
        auto& contribution = contributions[row.base + k - row.lo];
        if (!table) {
          ++work.regular_cells;
          if (k == b.n_interior + b.n_shell) contribution = {all, b.n_interior != 0};
          else {
            require(k + 1 == b.n_interior + b.n_shell, "atlas.regular_rank");
            for (unsigned omit = 0; omit < b.n_shell; ++omit) rep_masks.push_back(static_cast<Mask>(all ^ (Mask{1} << omit)));
          }
        } else {
          const auto rank = table->rank(k); ++work.extra_ranks;
          require(rank.present, "atlas.local_rank_present");
          work.reduced_vertices += rank.reduced_vertices; work.strict_cofaces += rank.strict_cofaces;
          work.union_attempts += rank.union_attempts; work.dsu_mask_slots += rank.dsu_mask_slots;
          contribution = {rank.contribution_shell, rank.contribution_interior};
          for (const auto& component : rank.strict_components) {
            require(component.interior_prefix == b.n_interior, "atlas.complete_interior");
            Mask translated = 0;
            for (unsigned j = 0; j < b.n_shell; ++j)
              if (component.representative_shell & (Mask{1} << j)) translated |= to_original[j];
            rep_masks.push_back(translated);
          }
        }
        rep_offsets.push_back(rep_masks.size());
      }
    }
  }
};
}  // namespace mhgp7::rank_atlas_private
