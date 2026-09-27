#pragma once

// Audit representation only. Explicitly borrows the frozen factor-plan
// representation from ddf4776d7; no geometric certificate is introduced.
#include "../b_q34_factor_plan_20260926/plan.hpp"

namespace mhgp9::audit::bands {
namespace fp = factor_plan;
using namespace mhgp9::gen;
enum class Mutant { None, Overlap, Mask6 };
struct Band { std::uint32_t a_class, b_first, b_last; };
static_assert(sizeof(Band) == 12);
struct Work {
  u64 validation_ranks{}, validation_groups{}, histogram_slots{}, prefix_steps{};
  u64 a_classes{}, row_tests{}, empty_bands{}, mass_queries{};
};

// Both factors must remain alive AND immutable until this object is no longer
// used. The plan does not own their ranks/credits; it owns only descriptors.
// Per-rectangle local offsets are u32 after checked conversion. Collective
// GPU-arena offsets would be separate u64 fields, not covered by this audit.
class Plan final {
 public:
  Plan(const fp::Factor& a, const fp::Factor& b, unsigned k, unsigned mask,
       Mutant mutant = Mutant::None) : a_(&a), b_(&b), k_(k), mutant_(mutant) {
    if (k == 0 || k > 10 || (mask & ~6U) != 0)
      throw std::invalid_argument("bands.invalid_K_mask");
    mask_ = mask & (k < 2 ? 0U : k < 3 ? 2U : 6U);
    validate(a); validate(b);
    std::array<std::array<u64, 11>, 11> prefix{};
    std::array<std::array<std::uint32_t, 11>, 10> counts{}, offsets{};
    work.histogram_slots = 121 + 110 + 110;
    for (const auto& g : b.groups)
      counts[g.credit.q3][g.credit.q4] = narrow(g.ranks.size());
    std::array<std::uint32_t, 11> row_starts{};
    std::uint32_t at = 0;
    for (unsigned x = 0; x != 10; ++x) {
      row_starts[x] = at;
      for (unsigned y = 0; y != 10; ++y) {
        offsets[x][y] = at;
        at += counts[x][y]; // Validated total size fits u32.
        prefix[x+1][y+1] = counts[x][y] + prefix[x][y+1] +
            prefix[x+1][y] - prefix[x][y];
        ++work.prefix_steps;
      }
      offsets[x][10] = at;
    }
    row_starts[10] = at;
    for (std::size_t ai = 0; ai != a.groups.size(); ++ai) {
      ++work.a_classes;
      const auto c = a.groups[ai].credit;
      const unsigned r3 = (mask_ & 2U) != 0 ? k - 1U - c.q3 : 0;
      const unsigned r4 = (mask_ & 4U) != 0 ? k - 2U - c.q4 : 0;
      const auto n3 = prefix[r3][10], n4 = prefix[10][r4];
      const auto both = prefix[r3][r4];
      const auto as = a.groups[ai].ranks.size();
      counter_add(q3_mass, fp::product(as, n3));
      counter_add(q4_mass, fp::product(as, n4));
      counter_add(union_mass, fp::product(as, n3+n4-both));
      ++work.mass_queries;
      append(narrow(ai), 0, row_starts[r3]);
      // q3 prefix and remaining q4 row prefixes are disjoint.
      for (unsigned x = r3; x != 10; ++x) {
        ++work.row_tests;
        append(narrow(ai), row_starts[x], offsets[x][r4]);
      }
    }
    if (mutant_ == Mutant::Overlap && !bands.empty()) bands.push_back(bands.front());
  }
  [[nodiscard]] unsigned pair_mask(std::size_t ai, std::size_t bi) const {
    const auto ac = a_->groups.at(ai).credit;
    const auto rank = b_->grouped.at(bi);
    const auto bc = b_->credits.at(rank - b_->original_ranks.first);
    unsigned out = 0;
    if ((mask_ & 2U) != 0 && static_cast<unsigned>(ac.q3) + bc.q3 < k_-1U) out |= 2U;
    if ((mask_ & 4U) != 0 && static_cast<unsigned>(ac.q4) + bc.q4 < k_-2U) out |= 4U;
    return mutant_ == Mutant::Mask6 && out != 0 ? 6U : out;
  }
  [[nodiscard]] std::size_t retained_bytes() const { return sizeof(*this) + bands.capacity()*sizeof(Band); }
  std::vector<Band> bands;
  Work work;
  u64 q3_mass{}, q4_mass{}, union_mass{};
  std::size_t validation_scratch_peak{};
  static constexpr std::size_t histogram_scratch_bytes = 121*sizeof(u64) + 231*sizeof(std::uint32_t);
 private:
  static std::uint32_t narrow(std::size_t value) {
    if (value > std::numeric_limits<std::uint32_t>::max())
      throw std::invalid_argument("bands.local_offset_overflow");
    return static_cast<std::uint32_t>(value);
  }
  void append(std::uint32_t ai, std::uint32_t first, std::uint32_t last) {
    if (first == last) { ++work.empty_bands; return; }
    bands.push_back({ai, first, last});
  }
  void validate(const fp::Factor& f) {
    if (f.original_ranks.last < f.original_ranks.first ||
        f.credits.size() != f.original_ranks.size() || f.grouped.size() != f.credits.size())
      throw std::invalid_argument("bands.factor_shape");
    static_cast<void>(narrow(f.grouped.size()));
    std::vector<unsigned char> seen(f.grouped.size(), 0);
    validation_scratch_peak = std::max(validation_scratch_peak, seen.capacity());
    std::size_t cursor = 0;
    unsigned previous = 0;
    bool first = true;
    for (const auto& g : f.groups) {
      ++work.validation_groups;
      const unsigned key = 10U*g.credit.q3 + g.credit.q4;
      if (g.credit.q3 > (k_ >= 2 ? k_-1 : 0) || g.credit.q4 > (k_ >= 3 ? k_-2 : 0) ||
          (!first && key <= previous) || g.ranks.first != cursor ||
          g.ranks.last <= cursor || g.ranks.last > f.grouped.size())
        throw std::invalid_argument("bands.factor_groups");
      previous = key; first = false;
      for (auto i = g.ranks.first; i != g.ranks.last; ++i) {
        ++work.validation_ranks;
        const auto rank = f.grouped[i];
        if (rank < f.original_ranks.first || rank >= f.original_ranks.last)
          throw std::invalid_argument("bands.factor_rank");
        const auto offset = rank - f.original_ranks.first;
        if (seen[offset]++ != 0 || f.credits[offset] != g.credit)
          throw std::invalid_argument("bands.factor_permutation_credit");
      }
      cursor = g.ranks.last;
    }
    if (cursor != f.grouped.size()) throw std::invalid_argument("bands.factor_coverage");
  }
  const fp::Factor *a_, *b_;
  unsigned k_, mask_{};
  Mutant mutant_;
};
}  // namespace mhgp9::audit::bands
