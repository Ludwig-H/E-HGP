// Private named primitive and judge; no active product instrumentation.
#define main mhgp7_original_meb_gate_main
#include "source/morsehgp3D_v7/tests/anchor_meb_gate.cpp"
#undef main
#include "anchor_meb_diameter.hpp"

namespace mhgp7 {
AnchorMebResult observed_diameter_meb(std::span<const P3>, AnchorMebWork&);
}
// Test-only observation of genuine MEB calls from the frozen FULL Builder.
#define anchor_meb observed_diameter_meb
#include "source/morsehgp3D_v7/src/forest/full_ball_tower.hpp"
#undef anchor_meb
#include "source/morsehgp3D_v7/src/cloud/families.hpp"
#include "source/morsehgp3D_v7/src/pipeline/generate.hpp"

namespace {
u64 cases = 0, gram_cases = 0, rejected = 0, flow_calls = 0, flow_pairs = 0, flow_towers = 0;
u64 total_pairs = 0, reference_supports = 0, variant_supports = 0;
u64 reference_powers = 0, variant_powers = 0;
std::array<u64, 11> flow_by_k{};
bool observing_flow = false;

void same_result(const AnchorMebResult& a, const AnchorMebResult& b) {
  need(a.status == b.status, "diameter.status");
  need(a.key == b.key && same_exact_level(a.level, b.level), "diameter.geometry");
  need(a.support_size == b.support_size && a.support_slots == b.support_slots, "diameter.canonical");
  need(a.selected_shell_count == b.selected_shell_count, "diameter.shell");
  need(std::string_view(a.reason) == b.reason, "diameter.reason");
}

// Independent rational schedule: no Candidate, form, or product power predicate.
AnchorMebWork gram_schedule(const std::vector<P3>& sites, u64& pairs) {
  AnchorMebWork result; result.calls = 1;
  if (sites.size() == 1) { result.supports_by_size[1] = result.materializations = 1; return result; }
  Rat maximum(-1);
  unsigned ea = 0, eb = 1;
  for (unsigned a = 0; a < sites.size(); ++a) for (unsigned b = a + 1; b < sites.size(); ++b) {
    const auto delta = oracle::detail::difference(oracle::detail::point(sites[a]), oracle::detail::point(sites[b]));
    const Rat distance = oracle::detail::dot(delta, delta); ++pairs;
    if (distance > maximum) { maximum = distance; ea = a; eb = b; }
  }
  std::vector<unsigned> order{ea, eb};
  for (unsigned i = 0; i < sites.size(); ++i) if (i != ea && i != eb) order.push_back(i);
  const u32 diam = (u32{1} << ea) | (u32{1} << eb);
  for (u32 support : supports(sites)) {
    const auto q = std::popcount(support);
    if (q == 2 && support != diam) continue;
    ++result.supports_by_size[q];
    const auto ball = oracle::detail::support_ball(sites, support);
    if (!ball) continue;
    bool inside = true;
    for (unsigned i : order) {
      ++result.power_tests;
      if (distance2(sites[i], *ball) > ball->radius2) { inside = false; break; }
    }
    if (inside) { ++result.materializations; return result; }
  }
  need(false, "diameter.gram_no_ball");
  return result;
}

AnchorMebResult compare_diameter(const std::vector<P3>& sites, bool gram, AnchorMebWork* paid = nullptr,
                                u64* paid_pairs = nullptr) {
  const auto before = sites;
  AnchorMebWork reference, work;
  u64 pairs = 0;
  const auto expected = mhgp7::anchor_meb(sites, reference);
  const auto result = anchor_meb_diameter(sites, work, pairs);
  need(expected.status == AnchorMebStatus::kOk, "diameter.expected_nonvacuous_success");
  same_result(expected, result);
  need(sites == before, "diameter.read_only");
  need(pairs == sites.size() * (sites.size() - 1) / 2, "diameter.paid_pairs");
  if (gram) {
    AnchorMebWork old_schedule;
    u32 first = 0;
    const auto truth = judge(sites, old_schedule, first);
    const auto observed = rational_ball(result.key);
    need(observed.center == truth.center && observed.radius2 == truth.radius2 &&
        rational_level(result.level) == truth.radius2, "diameter.independent_Gram");
    u32 mask = 0;
    for (u8 i = 0; i < result.support_size; ++i) mask |= u32{1} << result.support_slots[i];
    need(mask == first, "diameter.independent_canonical");
    u64 expected_pairs = 0;
    const auto schedule = gram_schedule(sites, expected_pairs);
    need(pairs == expected_pairs && work.calls == schedule.calls &&
        work.supports_by_size == schedule.supports_by_size && work.materializations == schedule.materializations,
        "diameter.paid_schedule");
    need(work.power_tests == schedule.power_tests, "diameter.paid_powers");
    ++gram_cases;
  }
  for (u8 q = 1; q <= 4; ++q) { reference_supports += reference.supports_by_size[q]; variant_supports += work.supports_by_size[q]; }
  total_pairs += pairs; reference_powers += reference.power_tests; variant_powers += work.power_tests;
  if (paid) {
    full_ball_detail::add(paid->calls, work.calls);
    full_ball_detail::add(paid->power_tests, work.power_tests);
    full_ball_detail::add(paid->materializations, work.materializations);
    for (u8 q = 0; q <= 4; ++q) full_ball_detail::add(paid->supports_by_size[q], work.supports_by_size[q]);
  }
  if (paid_pairs) full_ball_detail::add(*paid_pairs, pairs);
  ++cases;
  return result;
}

void refusal(const std::vector<P3>& sites, AnchorMebWork work, u64 pairs,
             AnchorMebStatus status, const char* reason) {
  const auto old_pairs = pairs;
  const auto result = anchor_meb_diameter(sites, work, pairs);
  need(result.status == status && std::string_view(result.reason) == reason, "diameter.rejection_cause");
  need(result.key == BallKey{} && result.level == ExactLevel{} && result.support_size == 0 &&
      result.support_slots == std::array<u8,4>{} && result.selected_shell_count == 0, "diameter.failure_empty");
  need(pairs >= old_pairs, "diameter.failure_paid_not_reset");
  ++rejected;
}

void run_diameter() {
  const std::vector<std::vector<P3>> fixtures{
    {{0,0,0},{2,2,0},{2,0,2},{0,3,2}},  // extrema change containment work
    {{0,0,0},{2,0,0},{0,2,0},{2,2,0}},  // two equal maximum diameters
    {{0,0,0},{1,1,0},{1,0,1}},          // equal maxima, no containing q2
    {{0,0,0},{2,2,0},{2,0,2},{0,2,2}},
    {{0,0,0},{65535,65535,0},{65535,0,65535},{0,65535,65535}},
    {{2,3,2},{2,0,0},{0,2,2},{1,0,0},{2,2,0},{3,0,1},{0,2,3}},
    {{10,5,0},{0,5,0},{5,10,0},{5,0,0},{8,9,0},{2,1,0},{9,8,0}}
  };
  std::mt19937_64 random(20260911);
  for (const auto& input : fixtures) {
    auto sites = input;
    compare_diameter(sites, true);
    for (unsigned i = 0; i < 12; ++i) { std::shuffle(sites.begin(), sites.end(), random); compare_diameter(sites, true); }
  }
  // All permutations, not merely random shuffles, for both tied-diameter cases.
  for (size_t fixture : {size_t{1}, size_t{2}}) {
    std::vector<size_t> order(fixtures[fixture].size()); std::iota(order.begin(), order.end(), 0);
    do {
      std::vector<P3> sites; for (auto i : order) sites.push_back(fixtures[fixture][i]);
      compare_diameter(sites, true);
    } while (std::next_permutation(order.begin(), order.end()));
  }
  for (unsigned n = 1; n <= 10; ++n) for (unsigned repetition = 0; repetition < 4; ++repetition) {
    std::vector<P3> sites;
    while (sites.size() != n) {
      const u64 bound = repetition % 2 ? 16 : 65536;
      const P3 point{static_cast<i64>(random()%bound), static_cast<i64>(random()%bound), static_cast<i64>(random()%bound)};
      if (std::find(sites.begin(), sites.end(), point) == sites.end()) sites.push_back(point);
    }
    compare_diameter(sites, true);
  }
  refusal({}, {}, 0, AnchorMebStatus::kInvalidInput, "anchor_meb_site_count");
  refusal(std::vector<P3>(11), {}, 0, AnchorMebStatus::kInvalidInput, "anchor_meb_site_count");
  refusal({{0,0,0},{0,0,0}}, {}, 0, AnchorMebStatus::kInvalidInput, "anchor_meb_duplicate_position");
  refusal({{-1,0,0}}, {}, 0, AnchorMebStatus::kInvalidInput, "anchor_meb_coordinate_profile");
  refusal({{0,65536,0}}, {}, 0, AnchorMebStatus::kInvalidInput, "anchor_meb_coordinate_profile");
  const auto maximum = std::numeric_limits<u64>::max();
  AnchorMebWork over; over.calls = maximum;
  refusal({{0,0,0}}, over, 0, AnchorMebStatus::kCounterOverflow, "anchor_meb_calls_overflow");
  refusal({{0,0,0},{1,0,0}}, {}, maximum, AnchorMebStatus::kCounterOverflow, "anchor_meb_diameter_pairs_overflow");
  over = {}; over.power_tests = maximum;
  refusal({{0,0,0},{1,0,0}}, over, 0, AnchorMebStatus::kCounterOverflow, "anchor_meb_power_tests_overflow");
  over = {}; over.materializations = maximum;
  refusal({{0,0,0},{1,0,0}}, over, 0, AnchorMebStatus::kCounterOverflow, "anchor_meb_materializations_overflow");
  const std::vector<std::vector<P3>> arities{{},{{0,0,0}},{{0,0,0},{1,0,0}},fixtures[2],fixtures[3]};
  for (unsigned q = 1; q <= 4; ++q) {
    over = {}; over.supports_by_size[q] = maximum;
    refusal(arities[q], over, 0, AnchorMebStatus::kCounterOverflow,
            q == 1 ? "anchor_meb_singleton_work_overflow" : "anchor_meb_supports_overflow");
  }
  AnchorMebWork last_work; u64 last_pairs = maximum - 1;
  need(anchor_meb_diameter(arities[2], last_work, last_pairs).status == AnchorMebStatus::kOk &&
      last_pairs == maximum, "diameter.last_pair_counter");
  AnchorMebWork cumulative; u64 cumulative_pairs = 19;
  need(anchor_meb_diameter(arities[2], cumulative, cumulative_pairs).status == AnchorMebStatus::kOk &&
      anchor_meb_diameter(arities[2], cumulative, cumulative_pairs).status == AnchorMebStatus::kOk &&
      cumulative_pairs == 21 && cumulative.calls == 2 && cumulative.materializations == 2 &&
      cumulative.supports_by_size[2] == 2 && cumulative.power_tests == 4, "diameter.cumulative_work");

  for (int s : {8,10,12}) {
    const auto in = make_family_input(CloudFamily::kUniform, 32, 65536, 3);
    const auto ix = build_cloud_index(in);
    GenerateOptions opt; opt.s = s; opt.smax = 11; opt.threads = 1;
    GenerateStats generation; std::vector<BallCandidate> candidates;
    generate_candidates(ix, opt, &candidates, &generation);
    need(generation.cap_refus == kCapRefusNone && !generation.invariant_jneg, "diameter.flow_generation");
    for (size_t q = 0; q < 3; ++q)
      need(generation.ledger_emitted_mass[q] + generation.ledger_killed_mass[q] == expected_pair_mass(ix), "diameter.flow_pair_mass");
    rle_candidates(&candidates, 1);
    std::vector<Survivor> survivors; std::vector<BallData> balls; ExpandStats census;
    prefilter_balls(ix, candidates, 11, 1, &survivors, &census);
    need(census_balls(ix, candidates, survivors, 11, kBallShellMax, 1, &balls, &census) ==
        PipelineStatus::kCompleteRegular && !balls.empty(), "diameter.flow_real_census");
    const u64 before = flow_calls;
    observing_flow = true;
    auto tower = build_full_ball_tower(ix, balls, 10, 1);
    observing_flow = false;
    need(tower.status == FullBallStatus::kCompleteRelative && tower.orders.size() == 10, "diameter.flow_tower");
    need(flow_calls - before == tower.stats.resolve_work.calls + tower.stats.validation_work.calls,
        "diameter.flow_complete_observation");
    need(tower.stats.resolve_work.calls > 0, "diameter.flow_resolver_nonvacuous");
    ++flow_towers;
  }
  need(gram_cases == 161 && rejected == 13 && flow_towers == 3 && flow_calls > 100 &&
      flow_by_k[9] > 0 && flow_by_k[10] > 0 && variant_supports < reference_supports,
      "diameter.nonvacuity");
  std::printf("{\"status\":\"passed\",\"checks\":%llu,\"cases\":%llu,\"Gram_cases\":%llu,\"rejections\":%llu,"
      "\"real_census_towers\":%llu,\"real_MEB_calls\":%llu,\"distance_pairs\":%llu,"
      "\"reference_supports\":%llu,\"variant_supports\":%llu,\"reference_powers\":%llu,\"variant_powers\":%llu,"
      "\"device_executed\":false,\"benchmark\":false}\n",
      static_cast<unsigned long long>(checks),static_cast<unsigned long long>(cases),static_cast<unsigned long long>(gram_cases),
      static_cast<unsigned long long>(rejected),static_cast<unsigned long long>(flow_towers),static_cast<unsigned long long>(flow_calls),
      static_cast<unsigned long long>(total_pairs),static_cast<unsigned long long>(reference_supports),static_cast<unsigned long long>(variant_supports),
      static_cast<unsigned long long>(reference_powers),static_cast<unsigned long long>(variant_powers));
}
}  // namespace

mhgp7::AnchorMebResult mhgp7::observed_diameter_meb(std::span<const P3> sites, AnchorMebWork& work) {
  if (!observing_flow) return mhgp7::anchor_meb(sites, work);
  ++flow_calls; ++flow_by_k[sites.size()];
  return compare_diameter(std::vector<P3>(sites.begin(), sites.end()), false, &work, &flow_pairs);
}

int main(int argc, char** argv) {
  if (argc != 2) return 2;
  try {
    const std::string_view mode(argv[1]);
    if (mode == "--selftest") run_diameter();
    else if (mode == "--mutant-ties") compare_diameter({{0,0,0},{2,0,0},{0,2,0},{2,2,0}}, true);
    else if (mode == "--mutant-counter") compare_diameter({{0,0,0},{1,0,0}}, true);
    else if (mode == "--mutant-order") compare_diameter({{0,0,0},{2,2,0},{2,0,2},{0,3,2}}, true);
    else if (mode == "--mutant-shell") compare_diameter({{0,0,0},{1,0,0}}, true);
    else return 2;
    return 0;
  }
  catch (const Failure& error) { std::fprintf(stderr, "%s\n", error.why); }
  catch (const full_ball_detail::Failure& error) { std::fprintf(stderr, "%s\n", error.reason); }
  catch (const std::exception& error) { std::fprintf(stderr, "%s\n", error.what()); }
  return 1;
}
