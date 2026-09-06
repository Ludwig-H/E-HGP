// Private one-shot component timing; no FULL or industrial timing claim.
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <string_view>
#include "src/cloud/families.hpp"
#include "src/pipeline/generate.hpp"
#ifdef MHGP7_TESTING
#error This component measurement requires nominal headers
#endif
int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--measure") return 2;
  using namespace mhgp7;
  const auto input = make_family_input(CloudFamily::kUniform, 8000, 65536, 3);
  const auto ix = build_cloud_index(input);
  if (!ix.valid || ix.has_duplicate_positions()) return 1;
  const u64 h[3] = {10, 9, 8};
  std::vector<MultiAliveRect> out;
  GenerateStats st;
  const auto start = std::chrono::steady_clock::now();
  alive_rectangles_fused(ix, 8, h, 7, 1, &out, &st);
  const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
  if (st.cap_refus != kCapRefusNone) return 1;
  const auto num = [](u64 x) { return static_cast<unsigned long long>(x); };
  std::printf("{\"schema\":\"mhgp7-private-terminal-once-front-timing-v1\",\"status\":\"completed\","
              "\"public_status\":\"not_claimed\",\"scope\":\"front_component_only\","
              "\"family\":\"uniform\",\"seed\":3,\"coord\":65536,\"n\":8000,\"s\":8,"
              "\"h\":[10,9,8],\"mask\":7,\"threads\":1,\"front_ms\":%.9f,"
              "\"work\":{\"witness_nodes\":%llu,\"corner_evals\":%llu},"
              "\"semantic\":{\"cap_refus\":%llu,\"wave_peak\":%llu,\"alive_peak\":%llu,"
              "\"visited\":%llu,\"workers\":%llu,\"ledgers\":[",
              ms, num(st.wspd_witness_nodes), num(st.wspd_corner_evals), num(st.cap_refus),
              num(st.wave_peak_tasks), num(st.alive_peak_rects), num(st.rect_visited_fused), num(st.workers_wspd));
  for (unsigned q = 0; q < 3; ++q) {
    if (st.ledger_emitted_mass[q] + st.ledger_killed_mass[q] != u128{8000} * 7999 / 2) return 1;
    if (q) std::printf(",");
    std::printf("[%llu,%llu,%llu]", num(st.rect_alive[q]), num(static_cast<u64>(st.ledger_emitted_mass[q])),
                num(static_cast<u64>(st.ledger_killed_mass[q])));
  }
  std::printf("],\"rectangles\":[");
  for (std::size_t i = 0; i < out.size(); ++i) {
    const auto& r = out[i];
    if (i) std::printf(",");
    std::printf("[%d,%d,%u,%llu,%llu,%llu]", r.r.a, r.r.b, static_cast<unsigned>(r.mask),
                num(r.core[0]), num(r.core[1]), num(r.core[2]));
  }
  std::printf("]}}\n");
}
