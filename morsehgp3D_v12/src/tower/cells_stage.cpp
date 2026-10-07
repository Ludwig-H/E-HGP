// Cellules de fenetre de l'etage G, par blocs de boules sur le Pool : comptage exact (naissances, cellules, inertes,
// etendues, representants par ordre ; tailles de fenetre), puis, apres la reservation, remplissage aux places fixees
// par les sommes prefixes des blocs (aucun atomique ; memes octets a tout nombre de fils). Chaque bloc rejoue la meme
// classification aux deux passes ; le remplissage controle que ses places finales egalent celles du comptage.
#include <algorithm>

#include "tower/stage.hpp"

namespace mhgp12::tower_detail {
namespace {

std::size_t field(u64 block, u32 k, int what) noexcept {
  return static_cast<std::size_t>((block * kMaxPart + (k - 1)) * kCountFields + static_cast<u64>(what));
}

struct CountPass {
  const Domain& domain;
  CellPlan& plan;
  std::span<u64> scratch_words;
  std::span<u32> scratch_parent;
  static Outcome body(void* raw, u64 begin, u64 end, u32 worker) noexcept {
    auto& self = *static_cast<CountPass*>(raw);
    CellScratch scratch = self.plan.scratch(self.scratch_words, self.scratch_parent, worker);
    const u32 balls = self.domain.catalogue.balls();
    for (u64 block = begin; block < end; ++block) {
      u64 windows = 0;
      for (u64 b = block * kBallBlock; b < std::min<u64>(balls, (block + 1) * kBallBlock); ++b) {
        const auto w = ball_window(self.domain.catalogue.balls_data()[b], self.plan.orders);
        if (w.lo <= w.hi) windows += w.hi - w.lo + 1;
        auto visit = [&](u32 k, const CellShape& shape, std::span<const u64>) noexcept -> Outcome {
          if (shape.birth) {
            ++self.plan.counts[field(block, k, kBirths)];
            return {};
          }
          ++self.plan.counts[field(block, k, kCells)];
          self.plan.counts[field(block, k, kInert)] += (shape.flags & kCellInert) != 0;
          self.plan.counts[field(block, k, kExtended)] += (shape.flags & kCellExtended) != 0;
          self.plan.counts[field(block, k, kReps)] += shape.reps;
          return {};
        };
        if (const Outcome o = visit_cells(self.domain, static_cast<u32>(b), self.plan.orders, scratch, visit); !o.ok())
          return fail(o.reason, o.order);
      }
      self.plan.windows[block] = windows;
    }
    return {};
  }
};

struct FillPass {
  const Domain& domain;
  const CellPlan& plan;
  Resolution& out;
  std::span<u64> scratch_words;
  std::span<u32> scratch_parent;

  Outcome fill_block(u64 block, CellScratch& scratch) noexcept {
    std::array<std::array<u64, kCountFields>, kMaxPart> at{};
    for (u32 k = 1; k <= plan.orders; ++k)
      for (int what = 0; what < kCountFields; ++what) at[k - 1][what] = plan.starts[field(block, k, what)];
    u64 window = plan.window_starts[block];
    auto& offsets = StageAccess::window_offsets(out);
    auto& targets = StageAccess::window_targets(out);
    auto& lows = StageAccess::window_lo(out);
    const auto& cat = domain.catalogue;
    for (u64 b = block * kBallBlock; b < std::min<u64>(cat.balls(), (block + 1) * kBallBlock); ++b) {
      const auto& data = cat.balls_data()[b];
      const auto w = ball_window(data, plan.orders);
      offsets[b] = window;
      lows[b] = static_cast<u8>(w.lo);
      auto visit = [&](u32 k, const CellShape& shape, std::span<const u64> masks) noexcept -> Outcome {
        ResolvedOrder& order = StageAccess::order(out, static_cast<Order>(k));
        auto& slot = at[k - 1];
        if (shape.birth) {
          const u64 i = slot[kBirths]++;
          StageAccess::birth_keys(order)[i] = static_cast<u32>(b);
          StageAccess::birth_ranks(order)[i] = data.rank;
          targets[window + (k - w.lo)] = birth_target(static_cast<u32>(i));
          return {};
        }
        const u64 c = slot[kCells]++;
        StageAccess::cell_balls(order)[c] = make_id<BallIdx>(static_cast<u32>(b));
        StageAccess::cell_ranks(order)[c] = data.rank;
        StageAccess::cell_flags(order)[c] = shape.flags;
        StageAccess::cell_offsets(order)[c] = slot[kReps];
        auto& traces = StageAccess::trace_masks(order);
        for (u64 r = 0; r < shape.reps; ++r) traces[slot[kReps] + r] = masks[r];
        slot[kReps] += shape.reps;
        slot[kInert] += (shape.flags & kCellInert) != 0;
        slot[kExtended] += (shape.flags & kCellExtended) != 0;
        targets[window + (k - w.lo)] = cell_target(static_cast<u32>(c));
        return {};
      };
      MHGP12_TRY(visit_cells(domain, static_cast<u32>(b), plan.orders, scratch, visit));
      if (w.lo <= w.hi) window += w.hi - w.lo + 1;
    }
    // Le remplissage doit retrouver exactement les places du comptage (classification rejouee a l'identique).
    if (window != plan.window_starts[block] + plan.windows[block]) return fail(Reason::tower_invariant);
    for (u32 k = 1; k <= plan.orders; ++k)
      for (int what = 0; what < kCountFields; ++what)
        if (at[k - 1][what] != plan.starts[field(block, k, what)] + plan.counts[field(block, k, what)])
          return fail(Reason::tower_invariant, static_cast<Order>(k));
    return {};
  }
  static Outcome body(void* raw, u64 begin, u64 end, u32 worker) noexcept {
    auto& self = *static_cast<FillPass*>(raw);
    CellScratch scratch = self.plan.scratch(self.scratch_words, self.scratch_parent, worker);
    for (u64 block = begin; block < end; ++block) MHGP12_TRY(self.fill_block(block, scratch));
    return {};
  }
};

}  // namespace

CellScratch CellPlan::scratch(std::span<u64> words, std::span<u32> parent, u32 worker) const noexcept {
  const auto mine = words.subspan(u64{worker} * kScratchWordsPerWorker, kScratchWordsPerWorker);
  CellScratch s;
  s.witnesses = mine.first(kMaxShellWitnesses);
  s.masks = mine.subspan(kMaxShellWitnesses, kMaxCellCombinations);
  s.sorted = mine.subspan(kMaxShellWitnesses + kMaxCellCombinations, kMaxCellCombinations);
  s.parent = parent.subspan(u64{worker} * kMaxCellCombinations, kMaxCellCombinations);
  return s;
}

Outcome count_cells(const Domain& d, CellPlan& plan, std::span<u64> words, std::span<u32> parent,
                    sched::Pool& pool) noexcept {
  for (u64 i = 0; i < plan.counts.size(); ++i) plan.counts[i] = 0;
  CountPass pass{d, plan, words, parent};
  MHGP12_TRY(pool.parallel_for(plan.blocks, 1, &pass, &CountPass::body));
  // Sommes prefixes par ordre et par champ, dans l'ordre des blocs ; totaux par ordre.
  for (u32 k = 1; k <= plan.orders; ++k)
    for (int what = 0; what < kCountFields; ++what) {
      u64 sum = 0;
      for (u64 block = 0; block < plan.blocks; ++block) {
        plan.starts[field(block, k, what)] = sum;
        sum += plan.counts[field(block, k, what)];
      }
      plan.totals[k - 1][what] = sum;
    }
  u64 windows = 0;
  for (u64 block = 0; block < plan.blocks; ++block) {
    plan.window_starts[block] = windows;
    windows += plan.windows[block];
  }
  plan.total_windows = windows;
  return {};
}

Outcome fill_cells(const Domain& d, const CellPlan& plan, Resolution& out, std::span<u64> words,
                   std::span<u32> parent, sched::Pool& pool) noexcept {
  FillPass pass{d, plan, out, words, parent};
  MHGP12_TRY(pool.parallel_for(plan.blocks, 1, &pass, &FillPass::body));
  StageAccess::window_offsets(out)[d.catalogue.balls()] = plan.total_windows;
  for (u32 k = 1; k <= plan.orders; ++k) {
    ResolvedOrder& order = StageAccess::order(out, static_cast<Order>(k));
    StageAccess::cell_offsets(order)[plan.totals[k - 1][kCells]] = plan.totals[k - 1][kReps];
  }
  return {};
}

}  // namespace mhgp12::tower_detail
