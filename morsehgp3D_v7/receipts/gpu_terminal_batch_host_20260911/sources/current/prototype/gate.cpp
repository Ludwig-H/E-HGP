// Private composed helper/FULL host-stub gate; no CUDA execution here.
#define MHGP7_PRIVATE_TERMINAL_STUB 1
#define MHGP7_MEB_KEY_FORBID_LEGACY_HOST_MATERIALIZE 1
#define main mhgp7_historical_full_gate_main
#include "source/morsehgp3D_v7/tests/full_ball_tower_gate.cpp"
#undef main

namespace terminal_gate {
using namespace mhgp7;
namespace terminal = gpu_terminal_private;
struct Case {
  u32 count; P3 points[10]; u32 q, shell, slots[4];
  u64 supports[5], powers, key_words[10], level_words[5];
};
const Case cases[]{
#include "source/morsehgp3D_v7/tests/anchor_meb_fixtures.inc"
};
u64 checks = 0, compared = 0, arities[5]{}, extras = 0, rejected = 0, raw_levels_differ = 0, large_ordinals = 0;
void need(bool condition, const char* reason) {
  ++checks;
  if (!condition) throw std::runtime_error(reason);
}

void unit() {
  for (const auto& fixture : cases) {
    const auto ix = build_cloud_index(std::vector<P3>(fixture.points, fixture.points + fixture.count));
    AnchorMebWork paid;
    const auto cpu = anchor_meb(ix.upos, paid);
    need(cpu.status == AnchorMebStatus::kOk && cpu.support_size == fixture.q &&
        cpu.selected_shell_count == fixture.shell, "unit.cpu_sorted_meb");
    const auto cpu_key = terminal::meb_key::encode_key(cpu.key);
    for (u8 i = 0; i < 10; ++i) need(cpu_key.words[i] == fixture.key_words[i], "unit.Gram_key");
    terminal::LevelWords expected_level;
    for (u8 i = 0; i < 5; ++i) expected_level.words[i] = fixture.level_words[i];
    need(same_exact_level(cpu.level, terminal::decode_level(expected_level)), "unit.Gram_level");
    std::vector<BallData> balls;
    if (fixture.q > 1) {
      BallData ball;
      ball.key = cpu.key; ball.level = cpu.level; ball.arity = cpu.support_size;
      ball.n_shell = cpu.selected_shell_count; ball.n_interior = fixture.count - ball.n_shell;
      u8 interior = 0, shell = 0;
      for (u32 i = 0; i < fixture.count; ++i) {
        const auto power = ball.key.power(ix.upos[i]);
        need(power <= 0, "unit.fixture_contained");
        if (power < 0) ball.interior_ids[interior++] = static_cast<i32>(i);
        else ball.shell_ids[shell++] = static_cast<i32>(i);
      }
      need(interior == ball.n_interior && shell == ball.n_shell, "unit.fixture_census_counts");
      balls.push_back(ball);
    }
    terminal::HostOwner owner(ix, balls);
    terminal::Request request;
    request.snapshot = owner.view().index.snapshot; request.k = fixture.count;
    request.before = terminal::encode_level(level(rational(cpu.level) + Rat(1)));
    for (u32 i = 0; i < request.k; ++i) request.selected[i] = static_cast<i32>(i);
    terminal::TraceRow observed;
    terminal::TraceBuffer trace{&observed, 1, 0, false};
    const auto result = terminal::resolve(owner.view(), request, &trace);
    if (fixture.q == 1) {
      need(result.status == terminal::Status::kInvalidRequest && result.target == terminal::kAbsentBall &&
          result.work.calls == 0 && trace.written == 0, "unit.K1_separate_no_catalogue_singleton");
      ++rejected; continue;
    }
    need(result.status == terminal::Status::kOk && result.target == 0 && trace.written == 1 && !trace.overflow,
        "unit.lookup_terminal_before_intruder");
    need(result.work.key_lookups == 1 && result.work.anchor_hits == 1 && result.work.intruder_queries == 0 &&
        result.work.intruder_nodes == 0 && result.work.same_radius_steps == 0 && result.work.descending_steps == 0,
        "unit.no_spatial_query_on_hit");
    need(result.work.calls == paid.calls && result.work.powers == paid.power_tests &&
        result.work.materializations == paid.materializations && result.work.level_materializations == 1,
        "unit.same_MEB_work");
    for (u8 q = 0; q < 5; ++q) need(result.work.supports[q] == paid.supports_by_size[q], "unit.same_support_work");
    std::vector<terminal::TraceRow> expected;
    std::vector<i32> sites(fixture.count); std::iota(sites.begin(), sites.end(), i32{0});
    terminal::note_cpu(&expected, sites, cpu, paid, -1, 0);
    need(terminal::same_trace(observed, expected.front()), "unit.full_trace");
    if (terminal::decode_level(observed.level) != cpu.level) ++raw_levels_differ;
    for (u8 word = 0; word < 10; ++word) need(observed.key.words[word] == fixture.key_words[word], "unit.device_Gram_key");
    need(same_exact_level(terminal::decode_level(observed.level), terminal::decode_level(expected_level)),
        "unit.raw_level_Gram_semantic_equality");
    if (!compared) for (u64 ordinal : {(u64{1} << 32) + 7, ~u64{0}}) {
      auto large = request; large.ordinal = ordinal;
      terminal::TraceRow large_row;
      terminal::TraceBuffer large_trace{&large_row, 1, 0, false};
      const auto large_result = terminal::resolve(owner.view(), large, &large_trace);
      need(large_result.status == terminal::Status::kOk && large_result.target == result.target &&
          large_result.ordinal == ordinal && large_trace.written == 1 &&
          terminal::same_trace(large_row, observed), "unit.u64_ordinal_no_narrowing");
      need(large_result.work.calls == result.work.calls && large_result.work.powers == result.work.powers,
          "unit.u64_ordinal_same_work");
      ++large_ordinals;
    }
    auto equal_before = request; equal_before.before = observed.level;
    const auto not_strict = terminal::resolve(owner.view(), equal_before);
    need(not_strict.status == terminal::Status::kNotStrict && not_strict.target == terminal::kAbsentBall &&
        not_strict.work.calls == 1 && not_strict.work.powers == paid.power_tests && not_strict.work.key_lookups == 0,
        "unit.equal_before_refused_with_paid_work");
    ++rejected; ++compared; ++arities[fixture.q]; if (fixture.shell > fixture.q) ++extras;
  }
  need(compared == 523 && arities[2] == 393 && arities[3] == 110 && arities[4] == 20 &&
      extras == 197 && raw_levels_differ > 0 && rejected == 605 && large_ordinals == 2, "unit.nonvacuity");
}
}  // namespace terminal_gate

int main(int argc, char** argv) {
  if (argc != 2 || (std::string_view(argv[1]) != "--selftest" && std::string_view(argv[1]) != "--static-4")) return 2;
  try {
    terminal_gate::unit();
    char name[] = "private_terminal_gate";
    char one[] = "--static-1";
    char four[] = "--static-4";
    char* original_args[]{name, std::string_view(argv[1]) == "--static-4" ? four : one};
    const int code = mhgp7_historical_full_gate_main(2, original_args);
    if (code) return code;
    namespace terminal = mhgp7::gpu_terminal_private;
    terminal_gate::need(terminal::proof.calls > 0 && terminal::proof.traces > terminal::proof.calls &&
        terminal::proof.same_steps > 0 && terminal::proof.strict_steps > 0 && terminal::proof.extra_shells > 0 &&
        terminal::proof.empty_trace_checks == terminal::proof.calls, "full.terminal_nonvacuity");
    std::printf("{\"status\":\"passed\",\"scope\":\"private_terminal_FULL_host_stub\",\"unit_checks\":%llu,"
        "\"unit_compared\":%llu,\"unit_rejections\":%llu,\"q2\":%llu,\"q3\":%llu,\"q4\":%llu,"
        "\"extra_shells\":%llu,\"raw_level_repr_differences\":%llu,\"large_ordinals\":%llu,\"paired_terminals\":%llu,"
        "\"paired_trace_rows\":%llu,\"paired_key_words\":%llu,\"paired_raw_levels\":%llu,"
        "\"strict_steps\":%llu,\"same_radius_steps\":%llu,\"zero_trace_capacity_checks\":%llu,"
        "\"device_executed\":false,\"gcp_used\":false}\n",
        static_cast<unsigned long long>(terminal_gate::checks), static_cast<unsigned long long>(terminal_gate::compared),
        static_cast<unsigned long long>(terminal_gate::rejected), static_cast<unsigned long long>(terminal_gate::arities[2]),
        static_cast<unsigned long long>(terminal_gate::arities[3]), static_cast<unsigned long long>(terminal_gate::arities[4]),
        static_cast<unsigned long long>(terminal_gate::extras), static_cast<unsigned long long>(terminal_gate::raw_levels_differ),
        static_cast<unsigned long long>(terminal_gate::large_ordinals),
        static_cast<unsigned long long>(terminal::proof.calls.load()), static_cast<unsigned long long>(terminal::proof.traces.load()),
        static_cast<unsigned long long>(terminal::proof.key_words.load()),
        static_cast<unsigned long long>(terminal::proof.semantically_equal_levels.load()),
        static_cast<unsigned long long>(terminal::proof.strict_steps.load()),
        static_cast<unsigned long long>(terminal::proof.same_steps.load()),
        static_cast<unsigned long long>(terminal::proof.empty_trace_checks.load()));
    return 0;
  } catch (const std::exception& error) {
    std::fprintf(stderr, "TERMINAL FAIL %s\n", error.what()); return 1;
  }
}
