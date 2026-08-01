#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace morsehgp3d::tests::fixtures {

inline constexpr std::string_view
    moment_curve_k3_one_step_star_incompleteness_n9_name =
        "moment_curve_k3_one_step_star_incompleteness_n9";

using MomentCurveSourcePoint = std::array<std::int64_t, 3U>;
using MomentCurveSourceTriangle = std::array<std::size_t, 3U>;
using MomentCurveSourceCoface = std::array<std::size_t, 4U>;

inline constexpr std::array<MomentCurveSourcePoint, 9U>
    moment_curve_k3_one_step_star_incompleteness_n9_points{{
        {0, 0, 0},
        {10000, 1, 1},
        {20000, 4, 8},
        {30000, 9, 27},
        {40000, 16, 64},
        {50000, 25, 125},
        {60000, 36, 216},
        {70000, 49, 343},
        {80000, 64, 512},
    }};

inline constexpr std::array<MomentCurveSourceCoface, 6U>
    moment_curve_k3_one_step_star_incompleteness_n9_direct_families{{
        {0U, 1U, 2U, 3U},
        {1U, 2U, 3U, 4U},
        {2U, 3U, 4U, 5U},
        {3U, 4U, 5U, 6U},
        {4U, 5U, 6U, 7U},
        {5U, 6U, 7U, 8U},
    }};

inline constexpr std::array<MomentCurveSourceTriangle, 19U>
    moment_curve_k3_one_step_star_incompleteness_n9_direct_deletions{{
        {0U, 1U, 2U},
        {0U, 1U, 3U},
        {0U, 2U, 3U},
        {1U, 2U, 3U},
        {1U, 2U, 4U},
        {1U, 3U, 4U},
        {2U, 3U, 4U},
        {2U, 3U, 5U},
        {2U, 4U, 5U},
        {3U, 4U, 5U},
        {3U, 4U, 6U},
        {3U, 5U, 6U},
        {4U, 5U, 6U},
        {4U, 5U, 7U},
        {4U, 6U, 7U},
        {5U, 6U, 7U},
        {5U, 6U, 8U},
        {5U, 7U, 8U},
        {6U, 7U, 8U},
    }};

inline constexpr MomentCurveSourceTriangle
    moment_curve_k3_one_step_star_incompleteness_n9_target_facet{
        0U, 4U, 8U};

inline constexpr std::array<std::size_t, 6U>
    moment_curve_k3_one_step_star_incompleteness_n9_silent_added_points{
        1U, 2U, 3U, 5U, 6U, 7U};

inline constexpr std::array<std::size_t, 2U>
    moment_curve_k3_one_step_star_incompleteness_n9_essential_support{
        0U, 8U};

inline constexpr std::int64_t
    moment_curve_k3_one_step_star_incompleteness_n9_squared_level =
        1600066560;

inline constexpr std::size_t
    moment_curve_k3_one_step_star_incompleteness_n9_affine_quadruplet_count =
        126U;

inline constexpr std::int64_t
    moment_curve_k3_one_step_star_incompleteness_n9_minimum_absolute_affine_determinant =
        120000;

}  // namespace morsehgp3d::tests::fixtures
