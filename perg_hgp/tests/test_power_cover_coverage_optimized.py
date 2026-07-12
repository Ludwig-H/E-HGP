import math

import numpy as np
import pytest

from perg_hgp.backends.power_cover_3d_cuda.contracts import GridSpec
from perg_hgp.backends.power_cover_3d_cuda.coverage import (
    CONFIRMED,
    EXCLUDED,
    POSSIBLE,
    _cell_relation,
    site_component_membership_cpu,
)
from perg_hgp.backends.power_cover_3d_cuda.cubical_msf import cubical_msf_cpu
from perg_hgp.backends.power_cover_3d_cuda.spatial_core import grid_centers


def _brute_cell_relation(centers, additive, radius, labels, spec):
    """Independent all-active-cell reference for the closed ball--AABB test."""

    active_ids = np.flatnonzero(labels >= 0)
    active_centers = grid_centers(spec, active_ids, xp=np, dtype=np.float64)
    half_widths = 0.5 * np.asarray(spec.cell_widths, dtype=np.float64)
    half_diagonal = float(np.linalg.norm(half_widths))
    radius_squared = float(radius) ** 2
    arithmetic_margin = (
        128.0
        * np.finfo(np.float64).eps
        * max(1.0, radius_squared, float(np.max(np.abs(centers))) ** 2)
    )
    rows = []
    for site, weight in zip(centers, additive):
        remaining = radius_squared - float(weight)
        if remaining < -arithmetic_margin or active_ids.size == 0:
            rows.append(np.empty(0, dtype=np.int32))
            continue
        broad_radius = math.sqrt(max(remaining, 0.0)) + half_diagonal
        candidates = (
            np.sum((active_centers - site) ** 2, axis=1) <= broad_radius**2
        )
        delta = np.maximum(np.abs(active_centers - site) - half_widths, 0.0)
        intersects = candidates & (
            np.sum(delta * delta, axis=1) <= remaining + arithmetic_margin
        )
        rows.append(
            np.unique(labels[active_ids[intersects]]).astype(np.int32, copy=False)
        )
    return rows


def _assert_rows_equal(observed, expected):
    assert len(observed) == len(expected)
    for actual_row, expected_row in zip(observed, expected):
        np.testing.assert_array_equal(actual_row, expected_row)


@pytest.mark.parametrize("single_component", [False, True])
def test_cell_relation_optimized_paths_match_brute_ball_aabb(single_component):
    rng = np.random.default_rng(20260712 + int(single_component))
    spec = GridSpec(
        bbox_min=(-1.0, -2.0, 0.25),
        bbox_max=(3.0, 1.0, 2.25),
        shape=(5, 4, 3),
        cell_widths=(0.8, 0.75, 2.0 / 3.0),
        half_diagonal=float(np.linalg.norm([0.4, 0.375, 1.0 / 3.0])),
    )
    labels = rng.integers(-1, 7, size=np.prod(spec.shape), dtype=np.int32)
    labels[labels < 1] = -1
    if single_component:
        labels[labels >= 0] = 17
    sites = rng.uniform([-2.0, -3.0, -0.5], [4.0, 2.0, 3.0], size=(80, 3))
    # Include empty balls, exact zero-radius balls and ordinary positive balls.
    additive = rng.uniform(0.0, 2.0, size=sites.shape[0])
    additive[:3] = [0.0, 1.0, np.nextafter(1.0, 2.0)]
    radius = 1.0

    observed, returned_labels = _cell_relation(
        sites,
        additive,
        radius,
        labels,
        spec,
        max_candidate_cells_per_site=int(np.prod(spec.shape)),
        max_active_cells=int(np.prod(spec.shape)),
    )
    expected = _brute_cell_relation(sites, additive, radius, labels, spec)

    assert returned_labels is labels
    _assert_rows_equal(observed, expected)


def test_single_component_witness_cannot_bypass_empty_legacy_broad_phase():
    spec = GridSpec(
        bbox_min=(0.0, 0.0, 0.0),
        bbox_max=(1.0, 0.0, 0.0),
        shape=(1, 1, 1),
        cell_widths=(1.0, 0.0, 0.0),
        half_diagonal=0.5,
    )
    sites = np.asarray([[1.0 + 1.0e-8, 0.0, 0.0]])
    labels = np.zeros(1, dtype=np.int32)

    observed, _ = _cell_relation(
        sites,
        np.zeros(1),
        0.0,
        labels,
        spec,
        max_candidate_cells_per_site=1,
        max_active_cells=1,
    )

    # The arithmetic-margin exact predicate alone would accept this site, but
    # the established broad phase has no candidate.  The optimization must not
    # silently alter that existing output contract.
    assert observed[0].size == 0


def test_single_component_fast_path_preserves_candidate_budget_guard():
    spec = GridSpec(
        bbox_min=(0.0, 0.0, 0.0),
        bbox_max=(4.0, 4.0, 4.0),
        shape=(4, 4, 4),
        cell_widths=(1.0, 1.0, 1.0),
        half_diagonal=math.sqrt(3.0) / 2.0,
    )
    labels = np.zeros(64, dtype=np.int32)

    with pytest.raises(RuntimeError, match="too many candidate cells"):
        _cell_relation(
            np.asarray([[2.0, 2.0, 2.0]]),
            np.zeros(1),
            10.0,
            labels,
            spec,
            max_candidate_cells_per_site=63,
            max_active_cells=64,
        )


def _reference_status(inner_rows, outer_rows, inner_labels, outer_labels):
    inner_to_outer = {}
    outer_preimages = {}
    for cell_id in np.flatnonzero(inner_labels >= 0):
        inner_label = int(inner_labels[cell_id])
        outer_label = int(outer_labels[cell_id])
        inner_to_outer.setdefault(inner_label, outer_label)
        outer_preimages.setdefault(outer_label, set()).add(inner_label)
    status = np.empty(len(inner_rows), dtype=np.uint8)
    for row_id, (inner, outer) in enumerate(zip(inner_rows, outer_rows)):
        if outer.size == 0:
            status[row_id] = EXCLUDED
            continue
        mapped = np.unique([inner_to_outer[int(label)] for label in inner])
        one_to_one = all(
            len(outer_preimages.get(int(label), set())) == 1 for label in outer
        )
        status[row_id] = (
            CONFIRMED if one_to_one and np.array_equal(mapped, outer) else POSSIBLE
        )
    return status


def test_full_membership_matches_brute_relations_and_status_for_nested_cuts():
    rng = np.random.default_rng(1948)
    spec = GridSpec(
        bbox_min=(-1.0, -1.5, -2.0),
        bbox_max=(2.0, 2.5, 2.0),
        shape=(4, 5, 3),
        cell_widths=(0.75, 0.8, 4.0 / 3.0),
        half_diagonal=float(np.linalg.norm([0.375, 0.4, 2.0 / 3.0])),
    )
    births = rng.uniform(0.0, 2.0, size=np.prod(spec.shape))
    base = cubical_msf_cpu(births, dims=spec.shape)
    # A constant shift has the production inner/outer nesting invariant while
    # yielding different active masks and, at intermediate cuts, components.
    inner = base.shifted(0.25)
    outer = base.shifted(-0.25)
    sites = rng.uniform([-2.0, -2.0, -3.0], [3.0, 3.0, 3.0], size=(100, 3))
    additive = rng.uniform(0.0, 1.5, size=sites.shape[0])
    radius = 1.1

    membership = site_component_membership_cpu(
        sites,
        additive,
        spec,
        inner,
        outer,
        radius=radius,
        max_candidate_cells_per_site=int(np.prod(spec.shape)),
        max_active_cells=int(np.prod(spec.shape)),
    )
    inner_labels = inner.components_at_cut(radius)
    outer_labels = outer.components_at_cut(radius)
    expected_inner = _brute_cell_relation(
        sites, additive, radius, inner_labels, spec
    )
    expected_outer = _brute_cell_relation(
        sites, additive, radius, outer_labels, spec
    )

    _assert_rows_equal(
        [membership.inner_for_site(index) for index in range(membership.n_sites)],
        expected_inner,
    )
    _assert_rows_equal(
        [membership.outer_for_site(index) for index in range(membership.n_sites)],
        expected_outer,
    )
    np.testing.assert_array_equal(
        membership.status,
        _reference_status(
            expected_inner, expected_outer, inner_labels, outer_labels
        ),
    )
