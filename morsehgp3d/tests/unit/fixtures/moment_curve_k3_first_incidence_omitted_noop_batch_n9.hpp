#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace morsehgp3d::tests::fixtures {

inline constexpr std::string_view
    moment_curve_k3_first_incidence_omitted_noop_batch_n9_name =
        "moment_curve_k3_first_incidence_omitted_noop_batch_n9";

using MomentCurveFirstIncidenceSourcePair = std::array<std::size_t, 2U>;
using MomentCurveFirstIncidenceSourceCoface =
    std::array<std::size_t, 4U>;

// Permanent counterexample to a one-to-one batch matcher. The direct plus
// first-incidence subflow contains 0123 and 1234 before this level, so the
// three 04-diametral cofaces below are one atomic qR=1 continuation with no
// core-facet or PointId delta. Omitting the whole group preserves both strict
// and closed H0 partitions; requiring a physical candidate group is wrong.
inline constexpr MomentCurveFirstIncidenceSourcePair
    moment_curve_k3_first_incidence_omitted_noop_batch_n9_support{0U, 4U};

inline constexpr std::int64_t
    moment_curve_k3_first_incidence_omitted_noop_batch_n9_squared_level =
        400001088;

inline constexpr std::array<MomentCurveFirstIncidenceSourceCoface, 3U>
    moment_curve_k3_first_incidence_omitted_noop_batch_n9_cofaces{{
        {0U, 1U, 2U, 4U},
        {0U, 1U, 3U, 4U},
        {0U, 2U, 3U, 4U},
    }};

inline constexpr std::size_t
    moment_curve_k3_first_incidence_omitted_noop_batch_n9_q_r = 1U;

inline constexpr std::string_view
    moment_curve_k3_first_incidence_omitted_noop_batch_n9_parent_digest =
        "42595fde9d4aee1cb6ae472d66ee6b945264a7dd128880f60c9153c4dd2c1925";

}  // namespace morsehgp3d::tests::fixtures
