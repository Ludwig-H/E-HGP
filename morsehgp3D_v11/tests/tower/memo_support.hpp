// Tests seulement : egalite des representations de niveaux et equations du travail reel.
#pragma once
#include "forest_support.hpp"
#include "tower/descent_memo.hpp"

namespace memo_test {
using namespace forest_test;
inline bool level_bytes(const num::Level& a, const num::Level& b) {
  return num::compare(num::to_wide(a.numerator()), num::to_wide(b.numerator())) == 0 &&
         num::compare(num::to_wide(a.denominator()), num::to_wide(b.denominator())) == 0;
}
inline bool answer(const DescentResult& a, const DescentResult& b) {
  return a.seed() == b.seed() && level_bytes(a.initial_level(), b.initial_level()) &&
         level_bytes(a.terminal_level(), b.terminal_level());
}
inline bool counts(const DescentLedger& w) {
  const auto& m = w.memo;
  return m.lookups == m.hits + m.misses && m.misses == w.steps && m.hits <= m.queries &&
         m.suffix_hits <= m.hits && m.collisions <= m.misses && m.evictions <= m.insertions &&
         m.queries == m.insertions + m.hits - m.suffix_hits &&
         w.steps - w.interior_steps - w.trace_steps == m.queries - m.hits;
}
inline DescentLedger without_memo(DescentLedger value) { value.memo = {}; return value; }
inline Input line() { return Input({{0,0,0},{2,0,0},{4,0,0},{6,0,0}}); }
inline std::vector<std::vector<SiteIdx>> parts(u32 n) {
  std::vector<std::vector<SiteIdx>> out;
  for (u32 mask = 1; mask < (1u << n); ++mask) {
    std::vector<SiteIdx> part;
    for (u32 i = 0; i < n; ++i) if ((mask >> i) & 1u) part.push_back(SiteIdx{i});
    if (part.size() <= 4) out.push_back(std::move(part));
  }
  return out;
}
}  // namespace memo_test
