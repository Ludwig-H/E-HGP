// Permanent nominal front regression, not a geometric oracle or a benchmark.
// Explicit port of private front_gate.cpp SHA256
// 45c4736806e103b930c8ac7da1982d069d7ae115a9b310a62f4c847158fc6b2e.
// The old/new O2 and SAN differential is retained separately in receipts/
// wspd_terminal_q2_reuse_20260906. This gate now requires the q2 reuse path.
#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <string_view>
#include <vector>

#include "../src/pipeline/generate.hpp"

#ifdef MHGP7_TESTING
#error Compile this gate against nominal product sources
#endif

namespace {
using namespace mhgp7;
struct Failure { const char* why; };
void require(bool ok, const char* why) { if (!ok) throw Failure{why}; }
unsigned long long number(u64 value) { return static_cast<unsigned long long>(value); }
u64 calls = 0, refused = 0, rows = 0, q2_terminals = 0, masses_killed = 0;
u64 q2_positive_core_checks = 0;
bool comma = false;

std::vector<InputPoint> cloud(unsigned scene, i64 separation) {
  std::vector<P3> p;
  if (scene == 0) p = {{0, 0, 0}, {20, 0, 0}};
  if (scene == 1) p = {{0, 0, 0}, {10, 0, 0}, {20, 0, 0}, {30, 0, 0}, {40, 0, 0}};
  if (scene == 2) p = {{15, 0, 0}, {15, 10, 0}, {16, 0, 0}, {15 + 5 * (separation + 2), 5, 0}};
  if (scene == 3) p = {{0, 0, 7}, {0, 9, 6}, {1, 4, 0}, {0, 0, 1},
                        {4, 1, 2}, {17, 23, 8}, {65535, 65535, 65535}, {62000, 61000, 63000}};
  std::vector<InputPoint> out;
  for (std::size_t i = 0; i < p.size(); ++i)
    out.push_back(InputPoint{static_cast<PointId>(101 + 17 * i), p[i]});
  return out;
}

void one(unsigned scene, i64 separation, u8 mask, unsigned threshold,
         u64 wave_cap, u64 alive_cap, u64 expected_refusal) {
  const auto input = cloud(scene, separation);
  const auto ix = build_cloud_index(input);
  require(ix.valid && !ix.has_duplicate_positions(), "fixture.index");
  const std::array<u64, 3> h = threshold == 0 ? std::array<u64, 3>{1, 1, 1}
                                            : std::array<u64, 3>{10, 9, 8};
  std::vector<MultiAliveRect> alive;
  GenerateStats st;
  alive_rectangles_fused(ix, separation, h.data(), mask, 1, &alive, &st, false,
                         wave_cap, alive_cap);
  require(st.cap_refus == expected_refusal, "front.refusal_code");
  ++calls;
  if (st.cap_refus != kCapRefusNone) ++refused;
  const u128 expected = static_cast<u128>(input.size()) * (input.size() - 1) / 2;
  std::array<u128, 3> observed{};
  std::array<u64, 3> rectangle_counts{};
  for (const auto& r : alive) {
    require(r.mask != 0 && (r.mask & mask) == r.mask, "front.mask");
    require(wspd_detail::separated(ix.box_of(r.r.a), ix.box_of(r.r.b), separation, 1),
            "front.separation");
    const u128 mass = static_cast<u128>(ix.node_weight(r.r.a)) * ix.node_weight(r.r.b);
    for (unsigned q = 0; q < 3; ++q) {
      if (r.mask & (1u << q)) {
        require(r.core[q] < h[q], "front.dead_lane_emitted");
        observed[q] += mass;
        ++rectangle_counts[q];
      } else require(r.core[q] == 0, "front.inactive_core");
    }
  }
  if (expected_refusal == kCapRefusNone) {
    for (unsigned q = 0; q < 3; ++q) {
      require(observed[q] == st.ledger_emitted_mass[q], "front.emitted_binding");
      require(rectangle_counts[q] == st.rect_alive[q], "front.rectangle_binding");
      require(st.ledger_emitted_mass[q] + st.ledger_killed_mass[q] ==
                  ((mask & (1u << q)) ? expected : 0), "front.mass_closure");
      if (st.ledger_killed_mass[q] != 0) ++masses_killed;
    }
    rows += alive.size();
    q2_terminals += st.rect_alive[0];
    if (scene == 1 && separation == 8 && mask == 1 && threshold == 1) {
      // Leaves (0,0,0) and (20,0,0) have exactly one strict diametral
      // witness, (10,0,0). This detects a lost fc.c[0] -> ff.c[0] copy.
      size_t found = 0;
      for (const auto& r : alive) {
        if (r.r.a == -1 && r.r.b == -3) {
          ++found;
          require(r.core[0] == 1, "line.q2_positive_core_value");
        }
      }
      require(found == 1, "line.q2_positive_core_unique_rectangle");
      ++q2_positive_core_checks;
    }
    if (scene == 0 && mask == 1) {
      require(alive.size() == 1 && alive[0].core[0] == 0, "pair.q2_terminal");
      // The historical duplicate pass visited six nodes. Three is required:
      // unchanged geometry alone cannot qualify an inactive optimization.
      require(st.wspd_witness_nodes == 3, "pair.exact_reused_q2_nodes");
    }
  }
  if (comma) std::printf(",");
  comma = true;
  std::printf("{\"scene\":%u,\"s\":%lld,\"mask\":%u,\"threshold\":%u,"
              "\"wave_cap\":%llu,\"alive_cap\":%llu,\"semantic\":{"
              "\"cap_refus\":%llu,\"wave_peak\":%llu,\"alive_peak\":%llu,"
              "\"visited\":%llu,\"workers\":%llu,\"ledgers\":[",
              scene, static_cast<long long>(separation), static_cast<unsigned>(mask), threshold,
              number(wave_cap), number(alive_cap), number(st.cap_refus), number(st.wave_peak_tasks),
              number(st.alive_peak_rects), number(st.rect_visited_fused), number(st.workers_wspd));
  for (unsigned q = 0; q < 3; ++q) {
    if (q) std::printf(",");
    require(st.ledger_emitted_mass[q] <= expected && st.ledger_killed_mass[q] <= expected,
            "front.small_fixture_integer_domain");
    std::printf("[%llu,%llu,%llu]", number(st.rect_alive[q]),
                number(static_cast<u64>(st.ledger_emitted_mass[q])),
                number(static_cast<u64>(st.ledger_killed_mass[q])));
  }
  std::printf("],\"rectangles\":[");
  for (std::size_t i = 0; i < alive.size(); ++i) {
    if (i) std::printf(",");
    const auto& r = alive[i];
    std::printf("[%d,%d,%u,%llu,%llu,%llu]", r.r.a, r.r.b, static_cast<unsigned>(r.mask),
                number(r.core[0]), number(r.core[1]), number(r.core[2]));
  }
  std::printf("]},\"work\":{\"witness_nodes\":%llu,\"corner_evals\":%llu}}",
              number(st.wspd_witness_nodes), number(st.wspd_corner_evals));
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try {
    std::printf("{\"schema\":\"mhgp7-wspd-terminal-reuse-gate-v1\","
                "\"public_status\":\"not_claimed\",\"scope\":\"nominal_front_regression_only\",\"cases\":[");
    for (i64 s : {8, 10, 12}) {
      for (unsigned scene = 0; scene < 4; ++scene)
        for (u8 mask = 1; mask < 8; ++mask)
          for (unsigned threshold = 0; threshold < 2; ++threshold)
            one(scene, s, mask, threshold, kMaxWaveTasks, kMaxAliveRects, kCapRefusNone);
      one(0, s, 1, 0, 0, kMaxAliveRects, kCapRefusWaveTasks);
      one(0, s, 1, 0, kMaxWaveTasks, 0, kCapRefusAliveRects);
    }
    require(calls == 174 && refused == 6 && rows > 0 && q2_terminals > 0 && masses_killed > 0 &&
                q2_positive_core_checks == 1,
            "front.nonvacuity");
    std::printf("],\"status\":\"passed\",\"calls\":%llu,\"refusals\":%llu,"
                "\"rectangles\":%llu,\"q2_terminals\":%llu,\"nonzero_killed_ledgers\":%llu,"
                "\"q2_positive_core_checks\":%llu}\n",
                number(calls), number(refused), number(rows), number(q2_terminals), number(masses_killed),
                number(q2_positive_core_checks));
    return 0;
  } catch (const Failure& failure) {
    std::fprintf(stderr, "wspd q2 front rejected: %s\n", failure.why);
  } catch (const std::exception& error) {
    std::fprintf(stderr, "wspd q2 front exception: %s\n", error.what());
  }
  return 1;
}
