#include "certified_support.hpp"

#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <string>

using namespace certified_support_audit;

static void require(bool condition, const char* reason) {
  if (!condition) {
    std::fprintf(stderr, "FAIL %s\n", reason);
    std::exit(1);
  }
}

struct Fixture {
  std::string name;
  std::vector<P3> sites;
  std::vector<std::vector<u8>> supports;
  u8 canonical_q;
  u8 shell_size;
};

static unsigned long long number(u64 value) {
  return static_cast<unsigned long long>(value);
}

int main() {
  const std::vector<Fixture> fixtures{
      {"diameter", {{0, 4, 4}, {8, 4, 4}, {1, 4, 4}, {4, 4, 4}, {5, 5, 4}},
       {{0, 1}}, 2, 2},
      {"acute_triangle", {{1, 1, 3}, {7, 1, 3}, {4, 6, 3}, {4, 2, 3}, {4, 3, 3}},
       {{0, 1, 2}}, 3, 3},
      {"cube", {{1, 1, 1}, {1, 1, 5}, {1, 5, 1}, {1, 5, 5}, {5, 1, 1},
                 {5, 1, 5}, {5, 5, 1}, {5, 5, 5}, {3, 3, 3}, {3, 3, 4}},
       {{0, 3, 5, 6}, {1, 2, 4, 7}, {1, 6}, {2, 5}}, 2, 8},
      {"welzl_k7", {{2, 3, 2}, {2, 0, 0}, {0, 2, 2}, {1, 0, 0}, {2, 2, 0},
                      {3, 0, 1}, {0, 2, 3}},
       {{0, 1, 5, 6}}, 4, 4},
  };
  u64 cases = 0;
  u64 certificate_powers = 0;
  u64 all_powers = 0;
  u64 reduced_powers = 0;
  u64 different_supports = 0;
  u64 different_arities = 0;
  u64 direct_returns = 0;
  for (const auto& fixture : fixtures) {
    std::vector<u8> order(fixture.sites.size());
    std::iota(order.begin(), order.end(), u8{0});
    for (unsigned permutation = 0; permutation < 3; ++permutation) {
      if (permutation == 1) std::reverse(order.begin(), order.end());
      if (permutation == 2) std::rotate(order.begin(), order.begin() + 2, order.end());
      std::vector<P3> sites;
      std::vector<u8> inverse(order.size());
      for (u8 i = 0; i < order.size(); ++i) {
        sites.push_back(fixture.sites[order[i]]);
        inverse[order[i]] = i;
      }
      for (std::size_t support_index = 0; support_index < fixture.supports.size();
           ++support_index) {
        auto proposed = fixture.supports[support_index];
        for (auto& slot : proposed) slot = inverse[slot];
        std::sort(proposed.begin(), proposed.end());
        std::array<u8, 4> support{};
        std::copy(proposed.begin(), proposed.end(), support.begin());
        const auto q = static_cast<u8>(proposed.size());
        Work full_work, reduced_work;
        const auto full = run(sites, support, q, Mode::kAllSites, full_work);
        const auto reduced = run(sites, support, q, Mode::kCertifiedSupport, reduced_work);
        AnchorMebWork nominal_work;
        const auto nominal = anchor_meb(sites, nominal_work);
        require(full.state == State::kOk && reduced.state == State::kOk,
                "both_canonicalizations_must_succeed");
        require(nominal.status == AnchorMebStatus::kOk && same(full.result, nominal) &&
                    same(reduced.result, nominal), "six_result_fields_differ");
        require(nominal.support_size == fixture.canonical_q &&
                    nominal.selected_shell_count == fixture.shell_size,
                "expected_support_or_shell_size");
        require(full_work.certification.attempts == 1 &&
                    full_work.certification.positive_forms == 1 &&
                    full_work.certification.powers == sites.size() &&
                    reduced_work.certification.attempts == 1 &&
                    reduced_work.certification.positive_forms == 1 &&
                    reduced_work.certification.powers == sites.size(),
                "certification_cost_must_remain_visible");
        if (fixture.shell_size == q) {
          require(reduced_work.direct_returns == 1 &&
                      reduced_work.canonicalization.attempts == 0 &&
                      reduced_work.canonicalization.positive_forms == 0 &&
                      reduced_work.canonicalization.powers == 0,
                  "equal_shell_must_return_certified_support_directly");
        } else {
          require(reduced_work.direct_returns == 0 &&
                      full_work.canonicalization.attempts == reduced_work.canonicalization.attempts &&
                      full_work.canonicalization.positive_forms ==
                          reduced_work.canonicalization.positive_forms,
                  "canonical_attempt_order_changed");
        }
        require(reduced_work.canonicalization.powers < full_work.canonicalization.powers,
                "fixture_must_reduce_power_tests");
        const bool support_differs = q != nominal.support_size || support != nominal.support_slots;
        const bool arity_differs = q != nominal.support_size;
        ++cases;
        different_supports += support_differs;
        different_arities += arity_differs;
        direct_returns += reduced_work.direct_returns;
        certificate_powers += reduced_work.certification.powers;
        all_powers += full_work.canonicalization.powers;
        reduced_powers += reduced_work.canonicalization.powers;
        std::printf("CASE %s permutation=%u support=%zu n=%zu proposed_q=%u canonical_q=%u "
                    "shell=%u certification_attempts=1 certification_positive_forms=1 "
                    "certification_powers=%llu full_canonical_attempts=%llu "
                    "full_canonical_positive_forms=%llu canonical_attempts=%llu canonical_positive_forms=%llu "
                    "full_powers=%llu support_powers=%llu skipped=%llu different_support=%d "
                    "different_arity=%d direct_return=%llu six_fields_equal=1\n",
                    fixture.name.c_str(), permutation, support_index, sites.size(), q,
                    nominal.support_size, nominal.selected_shell_count,
                    number(reduced_work.certification.powers),
                    number(full_work.canonicalization.attempts),
                    number(full_work.canonicalization.positive_forms),
                    number(reduced_work.canonicalization.attempts),
                    number(reduced_work.canonicalization.positive_forms),
                    number(full_work.canonicalization.powers),
                    number(reduced_work.canonicalization.powers),
                    number(reduced_work.skipped_support_intersections), support_differs, arity_differs,
                    number(reduced_work.direct_returns));
      }
    }
  }

  // An enclosing circum-ball alone does not certify minimality. The first
  // three points are on its shell, but their circumcenter is outside their hull.
  const std::vector<P3> invalid_certificate{{0, 5, 0}, {2, 9, 0}, {5, 10, 0}, {5, 1, 0}};
  const auto circum = q3_form(invalid_certificate[0], invalid_certificate[1],
                             invalid_certificate[2]);
  require(circum.g > 0, "negative_fixture_circum_ball_must_exist");
  for (const auto& point : invalid_certificate)
    require(q3_power(circum, point) <= 0, "negative_fixture_must_enclose_all_sites");
  Work rejected_work;
  const auto rejected = run(invalid_certificate, {0, 1, 2, 0}, 3,
                            Mode::kCertifiedSupport, rejected_work);
  require(rejected.state == State::kNotCertified &&
              std::string(rejected.reason) == "proposal_support_not_positive" &&
              rejected_work.certification.attempts == 1 &&
              rejected_work.certification.positive_forms == 0 &&
              rejected_work.certification.powers == 0 &&
              rejected_work.canonicalization.attempts == 0,
          "nonpositive_certificate_must_decline_before_shortcut");
  AnchorMebWork fallback_work;
  const auto fallback = anchor_meb(invalid_certificate, fallback_work);
  require(fallback.status == AnchorMebStatus::kOk && fallback_work.calls == 1,
          "complete_fallback_must_succeed");
  AnchorMebWork shell_work;
  const auto shell_only = anchor_meb(std::span<const P3>(invalid_certificate.data(), 3), shell_work);
  Candidate shell_candidate;
  require(shell_only.status == AnchorMebStatus::kOk &&
              anchor_meb_detail::form(invalid_certificate, shell_only.support_slots,
                                      shell_only.support_size, shell_candidate) &&
              shell_candidate.power(invalid_certificate[3]) > 0,
          "nonpositive_shell_only_must_be_an_actual_counterexample");
  std::printf("REJECT nonpositive_enclosing_ball certification_attempts=1 "
              "certification_positive_forms=0 certification_powers=0 canonical_attempts=0 "
              "fallback_calls=1 fallback_powers=%llu shell_only_excludes_fourth=1\n",
              number(fallback_work.power_tests));

  // Positive support alone is also insufficient: preserve the K=7 incomplete
  // proposal from the preceding sealed audit and decline on its outside site.
  Work incomplete_work;
  const auto incomplete = run(fixtures[3].sites, {0, 2, 3, 5}, 4,
                              Mode::kCertifiedSupport, incomplete_work);
  require(incomplete.state == State::kNotCertified &&
              std::string(incomplete.reason) == "proposal_not_enclosing" &&
              incomplete_work.certification.positive_forms == 1 &&
              incomplete_work.certification.powers == 7 &&
              incomplete_work.canonicalization.attempts == 0,
          "positive_but_incomplete_certificate_must_decline");
  std::puts("REJECT incomplete_positive_ball certification_attempts=1 "
            "certification_positive_forms=1 certification_powers=7 canonical_attempts=0");
  require(cases == 21 && different_supports > 0 && different_arities == 6 && direct_returns == 9 &&
              reduced_powers < all_powers, "nonvacuity_floor");
  std::printf("PASS cases=%llu rejections=2 different_supports=%llu different_arities=%llu "
              "direct_returns=%llu "
              "certification_powers=%llu full_canonical_powers=%llu support_canonical_powers=%llu "
              "full_total_powers=%llu support_total_powers=%llu\n",
              number(cases), number(different_supports), number(different_arities),
              number(direct_returns),
              number(certificate_powers), number(all_powers), number(reduced_powers),
              number(certificate_powers + all_powers), number(certificate_powers + reduced_powers));
  return 0;
}
