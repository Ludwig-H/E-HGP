// Independent fixed-calendar witness. Build against an explicitly captured
// product include root; no constructor gate or private prototype is included.
#if defined(MHGP7_TESTING)
#error "local calendar witness requires the nominal product header"
#endif
#include <array>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "src/forest/meb_proposal.hpp"

using namespace mhgp7;
namespace proposal = mhgp7::meb_proposal_detail;

std::string decimal(i128 value) {
  const bool negative = value < 0;
  u128 rest = negative ? static_cast<u128>(-(value + 1)) + 1 : static_cast<u128>(value);
  std::string result;
  do {
    result.insert(result.begin(), static_cast<char>('0' + rest % 10));
    rest /= 10;
  } while (rest != 0);
  if (negative) result.insert(result.begin(), '-');
  return result;
}

void terminal(bool ok, const SilentIncidenceResult& out,
              const silent_detail::LocalBall& ball, const CloudIndex& ix) {
  std::cout << "{\"ok\":" << (ok ? "true" : "false")
            << ",\"status\":" << static_cast<int>(out.status)
            << ",\"reason\":\"" << out.reason << "\",\"c\":" << out.stats.meb_supports
            << ",\"meb_calls\":" << out.stats.meb_calls
            << ",\"q\":" << static_cast<unsigned>(ball.q) << ",\"support\":[";
  // Full support expressed as original input positions, including support[0].
  for (size_t j = 0; j < 4; ++j) {
    const i32 u = ball.support[j];
    const i64 id = u >= 0 && u < ix.unique_count() ? static_cast<i64>(ix.point_id(u)) : -99;
    std::cout << (j ? "," : "") << id;
  }
  std::cout << "],\"key\":[" << decimal(ball.key.a);
  for (const i128 coefficient : ball.key.b) std::cout << ',' << decimal(coefficient);
  std::cout << ',' << decimal(ball.key.c) << "],\"num\":[" << ball.level.num[0]
            << ',' << ball.level.num[1] << ',' << ball.level.num[2]
            << "],\"den\":" << decimal(ball.level.den) << '}';
}

int main(int argc, char**) {
  if (argc != 1) return 2;
  const std::vector<P3> points{{0,0,0}, {2,2,0}, {2,0,2}, {0,2,2}};
  const CloudIndex ix = build_cloud_index(points);
  if (!ix.valid || ix.has_duplicate_positions() || ix.unique_count() != 4) return 3;
  std::array<i32, 11> sites{};
  for (i32 u = 0; u < ix.unique_count(); ++u) {
    if (ix.point_id(u) >= points.size()) return 3;
    sites[ix.point_id(u)] = u;
  }
  const std::vector<ForestEvent> direct;
  const SilentIncidenceLimits caps{0, 0, 0, 0, 33};
  for (const u64 proposal_cap : {u64{3}, u64{6}, u64{12}}) {
    SilentIncidenceResult reference, proposed;
    reference.reason = proposed.reason = "local_calendar_initial";
    silent_detail::Builder reference_builder(ix, direct, caps, &reference);
    silent_detail::Builder fallback_builder(ix, direct, caps, &proposed);
    proposal::Work work;
    const proposal::Limits limits{proposal_cap};
    proposal::NoObserver observer;
    for (unsigned call = 1; call <= 3; ++call) {
      silent_detail::LocalBall reference_ball, proposed_ball;
      const bool reference_ok = reference_builder.miniball(sites, 4, &reference_ball);
      const bool proposed_ok = proposal::miniball(ix, fallback_builder, caps, &proposed,
          sites, 4, &proposed_ball, limits, &work, &observer);
      std::cout << "{\"P\":" << proposal_cap << ",\"L\":33,\"call\":" << call
                << ",\"accounting\":\"" << proposal::kWorkAccounting << "\",\"reference\":";
      terminal(reference_ok, reference, reference_ball, ix);
      std::cout << ",\"proposed\":";
      terminal(proposed_ok, proposed, proposed_ball, ix);
      std::cout << ",\"work\":{\"p\":" << work.meb_proposal_supports
                << ",\"pivots\":" << work.pivots << ",\"certified\":" << work.certified
                << ",\"fallback\":" << work.fallback << ",\"A\":" << work.reference_supports
                << "}}\n";
    }
  }
  return 0;
}
