#include <cstdio>
#include <string_view>
#include "rank_atlas.hpp"
#include "source/morsehgp3D_v7/src/forest/full_ball_tower.hpp"
#include "source/morsehgp3D_v7/src/cloud/families.hpp"
#include "source/morsehgp3D_v7/src/pipeline/generate.hpp"

namespace mhgp7::full_ball_detail {
// Test-only friend. Executes the actual private catalogue validation and
// visit_block; no duplicate resolver/calendar, no source-body substitution.
struct RankAtlasReference {
  struct Row { u32 ball; u16 contribution; bool interior; std::vector<std::vector<i32>> keys; };
  using Programs = std::vector<std::vector<Row>>;
  static Programs capture(const CloudIndex& ix, std::span<const BallData> balls, unsigned requested) {
    FullBallStats stats;
    Builder builder(ix, balls, requested, stats);
    builder.validate_catalogue();
    Programs out(builder.kmax + 1);
    for (unsigned k = 1; k <= builder.kmax; ++k) {
      builder.current_k = k;
      for (BallId b : builder.programs[k]) {
        Row row{b, 0, false, {}};
        builder.visit_block(b, row.contribution, row.interior, [&](std::vector<i32> key) {
          std::sort(key.begin(), key.end()); row.keys.push_back(std::move(key));
        });
        out[k].push_back(std::move(row));
      }
    }
    return out;
  }
};
}
namespace gate {
using namespace mhgp7;
namespace atlas = rank_atlas_private;
using Reference = full_ball_detail::RankAtlasReference;
struct Totals {
  u64 checks = 0, fixtures = 0, balls = 0, cells = 0, reps = 0, births = 0, points = 0;
  u64 q3 = 0, q4 = 0, extras = 0, extra_tables = 0, extra_ranks = 0, regular_cells = 0;
  u64 k_cells[11]{}, k_reps[11]{}, k_births[11]{};
  u64 logical_max = 0, capacity_max = 0, borrowed_max = 0, equal_pairs = 0, unequal_raw_pairs = 0;
  u64 nonidentity_shells = 0, scaled_levels = 0, terminal_births = 0, empty_catalogues = 0;
} total;
void need(bool condition, const char* why) {
  ++total.checks;
  if (!condition) throw std::runtime_error(why);
}
void compare(const atlas::Atlas& a, const Reference::Programs& expected) {
  a.validate_shape();
  need(expected.size() == a.kmax + 1, "gate.program_count");
  for (unsigned k = 1; k <= a.kmax; ++k) {
    const auto p = a.program(k);
    need(p.size() == expected[k].size(), "gate.program_size");
    for (size_t j = 0; j < p.size(); ++j) {
      const auto b = p[j]; const auto& row = expected[k][j];
      need(b == row.ball, "gate.program_order");
      const u64 cell = a.cell(b, k);
      need(a.contributions[cell].shell == row.contribution &&
          a.contributions[cell].interior == row.interior, "gate.contribution_exact");
      const auto masks = a.masks(b, k);
      need(masks.size() == row.keys.size(), "gate.representative_count");
      for (size_t r = 0; r < masks.size(); ++r)
        need(a.expand(b, masks[r]) == row.keys[r], "gate.representative_identity_order");
      // Raw level remains borrowed, not replaced by the common rank's level.
      need(&a.raw_level(b) == &a.balls[b].level, "gate.raw_level_preserved");
    }
  }
  for (size_t b = 0; b < a.balls.size(); ++b) for (unsigned k = 0; k <= 11; ++k) {
    bool found = false;
    if (k < expected.size()) for (const auto& row : expected[k]) if (row.ball == b) found = true;
    need(a.block_id(static_cast<u32>(b), k).has_value() == found, "gate.admission_exact");
  }
}
std::vector<InputPoint> points(std::initializer_list<P3> values) {
  std::vector<InputPoint> result;
  PointId id = 1009;
  for (P3 p : values) { result.push_back({id, p}); id += 37; }
  return result;
}
void double_raw(ExactLevel& level) {
  // Only used on the small square census, preserving the exact rational value.
  need(level.den < (i128{1} << 110) && level.num[2] < (u64{1} << 40), "gate.scale_domain");
  const u64 carry0 = level.num[0] >> 63, carry1 = level.num[1] >> 63;
  level.num[0] <<= 1; level.num[1] = (level.num[1] << 1) | carry0;
  level.num[2] = (level.num[2] << 1) | carry1; level.den *= 2;
  ++total.scaled_levels;
}
void mutate(atlas::Atlas& a, std::string_view fault) {
  if (fault == "--date") {
    need(a.level_balls.size() > 1, "fault.date_nonvacuous");
    a.slots[a.level_balls[0]].rank = 2;
  } else if (fault == "--offset") {
    need(a.rep_offsets.size() > 1, "fault.offset_nonvacuous");
    a.rep_offsets[1] = a.rep_masks.size() + 1;
  } else if (fault == "--mask") {
    bool changed = false;
    for (size_t b = 0; b < a.balls.size() && !changed; ++b) {
      const auto& s = a.slots[b];
      for (unsigned k = s.lo; k <= s.hi && !changed; ++k) {
        const u64 c = a.cell(static_cast<u32>(b), k);
        if (a.rep_offsets[c] == a.rep_offsets[c + 1]) continue;
        auto& mask = a.rep_masks[a.rep_offsets[c]];
        for (unsigned i = 0; i < a.balls[b].n_shell && !changed; ++i)
          for (unsigned j = 0; j < a.balls[b].n_shell && !changed; ++j)
            if ((mask & (1u << i)) && !(mask & (1u << j))) {
              mask ^= static_cast<u16>((1u << i) | (1u << j)); changed = true;
            }
      }
    }
    need(changed, "fault.mask_nonvacuous");
    a.validate_shape();  // Valid cardinality/domain alone must NOT promote it.
  } else if (fault == "--contribution") {
    need(!a.contributions.empty(), "fault.contribution_nonvacuous");
    a.contributions[0].shell ^= u16{1}; a.validate_shape();
  } else if (fault == "--admission") {
    need(!a.slots.empty(), "fault.admission_nonvacuous");
    ++a.slots[0].lo;
  } else if (fault == "--permutation") {
    bool changed = false;
    for (unsigned k = 1; k <= a.kmax && !changed; ++k) if (a.program(k).size() > 1) {
      std::swap(a.program_balls[a.k_offsets[k]], a.program_balls[a.k_offsets[k] + 1]); changed = true;
    }
    need(changed, "fault.permutation_nonvacuous");
  }
}
bool fixture(const char* name, std::vector<InputPoint> input, unsigned requested, int s,
    bool scramble, bool scaled, std::string_view fault = {}) {
  std::fprintf(stderr, "fixture_start=%s n=%zu K=%u s=%d scramble=%d scale=%d\n",
      name, input.size(), requested, s, scramble, scaled);
  const auto index = build_cloud_index(input);
  need(index.valid && !index.has_duplicate_positions(), "gate.index");
  const auto smax = std::min<u64>(requested + 1, index.input_count);
  GenerateOptions options; options.s = s; options.smax = smax; options.threads = 1;
  GenerateStats generated;
  std::vector<BallCandidate> candidates;
  generate_candidates(index, options, &candidates, &generated);
  need(generated.cap_refus == kCapRefusNone && !generated.invariant_jneg, "gate.generation");
  const u128 mass = expected_pair_mass(index);
  for (size_t q = 0; q < 3; ++q)
    need(generated.ledger_emitted_mass[q] + generated.ledger_killed_mass[q] == mass, "gate.pair_ledger");
  rle_candidates(&candidates, 1);
  need(candidates_capacity_ok(candidates.size()), "gate.candidate_representation");
  ExpandStats census; std::vector<Survivor> survivors;
  prefilter_balls(index, candidates, smax, 1, &survivors, &census);
  std::vector<BallData> balls;
  need(census_balls(index, candidates, survivors, smax, kBallShellMax, 1, &balls, &census) ==
      PipelineStatus::kCompleteRegular, "gate.real_census");
  if (scramble) {
    std::reverse(balls.begin(), balls.end());
    for (auto& b : balls) {
      std::reverse(b.interior_ids, b.interior_ids + b.n_interior);
      std::reverse(b.shell_ids, b.shell_ids + b.n_shell);
    }
  }
  if (scaled) for (size_t b = 0; b < balls.size(); b += 2) double_raw(balls[b].level);
  // Run the actual catalogue validator first: any fixture rejection is fatal.
  const auto expected = Reference::capture(index, balls, requested);
  auto a = atlas::Atlas::build(index, balls, requested);
  need(a.index == &index && a.balls.data() == balls.data(), "gate.borrowed_identity");
  compare(a, expected);
  if (!fault.empty()) {
    mutate(a, fault);
    try { compare(a, expected); }
    catch (const std::exception& error) {
      const std::string_view why(error.what());
      const auto required = fault == "--date" ? "atlas.rank_date" : fault == "--offset" ? "atlas.rep_interval" :
          fault == "--mask" ? "gate.representative_identity_order" : fault == "--contribution" ? "gate.contribution_exact" :
          fault == "--admission" ? "atlas.admission_base" : "atlas.program_order";
      need(why == required, "gate.exact_mutant_cause");
      std::printf("{\"status\":\"causal_rejection\",\"fault\":\"%.*s\",\"cause\":\"%s\",\"checks\":%llu}\n",
          static_cast<int>(fault.size()), fault.data(), error.what(), static_cast<unsigned long long>(total.checks));
      return true;
    }
    throw std::runtime_error("gate.mutant_survived");
  }
  for (size_t b = 0; b < balls.size(); ++b) {
    const auto& ball = balls[b];
    total.q3 += ball.arity == 3; total.q4 += ball.arity == 4; total.extras += ball.n_shell > ball.arity;
    if (ball.n_shell > ball.arity) {
      bool identity = true;
      for (unsigned j = 1; j < ball.n_shell; ++j)
        identity = identity && index.point_id(ball.shell_ids[j - 1]) < index.point_id(ball.shell_ids[j]);
      total.nonidentity_shells += !identity;
    }
    for (size_t c = 0; c < b; ++c) {
      const auto cmp = compare_exact_level(ball.level, balls[c].level);
      need((cmp == 0) == (a.slots[b].rank == a.slots[c].rank) &&
          (cmp < 0) == (a.slots[b].rank < a.slots[c].rank), "gate.exact_rank_relation");
      if (cmp == 0) {
        ++total.equal_pairs;
        total.unequal_raw_pairs += ball.level.den != balls[c].level.den ||
            !std::equal(std::begin(ball.level.num), std::end(ball.level.num), std::begin(balls[c].level.num));
      }
    }
  }
  u64 birth_count = 0;
  for (unsigned k = 1; k <= a.kmax; ++k) for (u32 b : a.program(k)) {
    const auto masks = a.masks(b, k); ++total.k_cells[k]; total.k_reps[k] += masks.size();
    if (masks.empty()) {
      ++birth_count; ++total.k_births[k];
      if (k > 1) need(a.block_id(b, k - 1).has_value(), "gate.birth_lower_same_ball_admitted");
      if (k == index.input_count) ++total.terminal_births;
    }
  }
  for (size_t j = 0; j < index.upos.size(); ++j)
    need(a.point_rank(static_cast<i32>(j)) == 0, "gate.point_zero_rank");
  if (index.input_count <= requested) {
    if (index.input_count == 1) need(balls.empty(), "gate.singleton_no_ball");
    else {
      need(a.program(a.kmax).size() == 1 && a.masks(a.program(a.kmax)[0], a.kmax).empty(), "gate.terminal_unique_birth");
    }
  }
  need(a.work.regular_cells + a.work.extra_ranks == a.contributions.size(), "gate.one_extraction_per_cell");
  need(a.work.extra_tables == static_cast<u64>(std::count_if(balls.begin(), balls.end(),
      [](const BallData& b) { return b.n_shell > b.arity; })), "gate.one_table_per_extra_ball");
  ++total.fixtures; total.balls += balls.size(); total.cells += a.contributions.size(); total.reps += a.rep_masks.size();
  total.births += birth_count; total.points += index.input_count; total.empty_catalogues += balls.empty();
  total.extra_tables += a.work.extra_tables; total.extra_ranks += a.work.extra_ranks; total.regular_cells += a.work.regular_cells;
  total.logical_max = std::max(total.logical_max, a.logical_bytes());
  total.capacity_max = std::max(total.capacity_max, a.capacity_bytes());
  total.borrowed_max = std::max<u64>(total.borrowed_max, balls.size() * sizeof(BallData));
  std::fprintf(stderr, "fixture_complete=%s C=%zu A=%zu R=%zu births=%llu logical_bytes=%llu capacity_bytes=%llu\n",
      name, balls.size(), a.contributions.size(), a.rep_masks.size(), static_cast<unsigned long long>(birth_count),
      static_cast<unsigned long long>(a.logical_bytes()), static_cast<unsigned long long>(a.capacity_bytes()));
  return false;
}
}
int main(int argc, char** argv) {
  if (argc != 2) return 2;
  const std::string_view mode(argv[1]);
  if (mode != "--selftest" && mode != "--date" && mode != "--offset" && mode != "--mask" &&
      mode != "--contribution" && mode != "--admission" && mode != "--permutation") return 2;
  try {
    using namespace gate;
    auto uniform = make_family_input(CloudFamily::kUniform, 32, 65536, 3);
    if (mode != "--selftest") return fixture("causal_uniform32", uniform, 10, 8, false, false, mode) ? 4 : 1;
    for (int s : {8, 10, 12}) {
      fixture("uniform32", uniform, 10, s, false, false);
      auto permutation = uniform; std::reverse(permutation.begin(), permutation.end());
      fixture("uniform32_permuted", permutation, 10, s, true, false);
    }
    const auto square = points({{0,0,0},{10,0,0},{0,10,0},{10,10,0},
        {4,4,0},{5,4,0},{4,5,0},{5,5,0}});
    for (int s : {8, 10, 12}) fixture("extra_square8", square, 8, s, true, true);
    fixture("ABCZ", points({{1,8,0},{5,10,0},{9,8,0},{5,0,0}}), 4, 8, true, false);
    fixture("line3", points({{0,0,0},{2,0,0},{4,0,0}}), 3, 8, true, false);
    fixture("terminal_pair", points({{0,0,0},{2,0,0}}), 2, 8, false, false);
    fixture("terminal_singleton", points({{0,0,0}}), 1, 8, false, false);
    need(total.fixtures == 13 && total.empty_catalogues == 1 && total.q3 > 0 && total.q4 > 0 &&
        total.extras > 0 && total.nonidentity_shells > 0 && total.extra_ranks > total.extra_tables &&
        total.k_cells[9] > 0 && total.k_cells[10] > 0 && total.k_reps[10] > 0 &&
        total.births > 0 && total.terminal_births >= 6 && total.equal_pairs > 0 && total.unequal_raw_pairs > 0,
        "gate.nonvacuity");
    std::printf("{\"status\":\"passed\",\"scope\":\"rank_admission_masks_against_actual_Builder_visit\","
        "\"public_status\":\"not_claimed\",\"backend\":\"cpu_reference\",\"device_executed\":false,"
        "\"checks\":%llu,\"fixtures\":%llu,\"balls\":%llu,\"cells\":%llu,\"representatives\":%llu,"
        "\"births\":%llu,\"point_births\":%llu,\"q3\":%llu,\"q4\":%llu,\"extras\":%llu,"
        "\"extra_tables\":%llu,\"extra_rank_calls\":%llu,\"regular_cells\":%llu,"
        "\"equal_level_pairs\":%llu,\"unequal_raw_equal_pairs\":%llu,\"nonidentity_shells\":%llu,"
        "\"terminal_births\":%llu,\"memory\":{\"logical_max\":%llu,\"capacity_max\":%llu,"
        "\"borrowed_ball_bytes_max\":%llu,\"owned_BallData_copies\":0,\"sizeof_ball_slot\":%zu,"
        "\"sizeof_contribution\":%zu,\"sizeof_mask\":%zu,\"peak_RSS_measured\":false},\"per_k\":[",
        static_cast<unsigned long long>(total.checks), static_cast<unsigned long long>(total.fixtures),
        static_cast<unsigned long long>(total.balls), static_cast<unsigned long long>(total.cells),
        static_cast<unsigned long long>(total.reps), static_cast<unsigned long long>(total.births),
        static_cast<unsigned long long>(total.points), static_cast<unsigned long long>(total.q3),
        static_cast<unsigned long long>(total.q4), static_cast<unsigned long long>(total.extras),
        static_cast<unsigned long long>(total.extra_tables), static_cast<unsigned long long>(total.extra_ranks),
        static_cast<unsigned long long>(total.regular_cells), static_cast<unsigned long long>(total.equal_pairs),
        static_cast<unsigned long long>(total.unequal_raw_pairs), static_cast<unsigned long long>(total.nonidentity_shells),
        static_cast<unsigned long long>(total.terminal_births), static_cast<unsigned long long>(total.logical_max),
        static_cast<unsigned long long>(total.capacity_max), static_cast<unsigned long long>(total.borrowed_max),
        sizeof(atlas::BallSlot), sizeof(atlas::Contribution), sizeof(atlas::Mask));
    for (unsigned k = 1; k <= 10; ++k) std::printf("%s{\"K\":%u,\"A\":%llu,\"R\":%llu,\"births\":%llu}",
        k == 1 ? "" : ",", k, static_cast<unsigned long long>(total.k_cells[k]),
        static_cast<unsigned long long>(total.k_reps[k]), static_cast<unsigned long long>(total.k_births[k]));
    std::printf("]}\n"); return 0;
  } catch (const mhgp7::full_ball_detail::Failure& error) {
    std::fprintf(stderr, "reference_abort=%s\n", error.reason);
  } catch (const std::exception& error) { std::fprintf(stderr, "error=%s\n", error.what()); }
  return 1;
}
