import numpy as np
import pytest

from perg_hgp.backends.power_cover_3d_cuda.api import PowerCoverHierarchy
from perg_hgp.backends.power_cover_3d_cuda.contracts import GridSpec
from perg_hgp.backends.power_cover_3d_cuda.coverage import (
    CONFIRMED,
    EXCLUDED,
    POSSIBLE,
    SiteComponentMembership,
    site_component_membership_cpu,
)
from perg_hgp.backends.power_cover_3d_cuda.cubical_msf import cubical_msf_cpu


def _csr(rows):
    lengths = np.fromiter((len(row) for row in rows), dtype=np.int64)
    indptr = np.zeros(len(rows) + 1, dtype=np.int64)
    np.cumsum(lengths, out=indptr[1:])
    labels = (
        np.concatenate([np.asarray(row, dtype=np.int32) for row in rows])
        if int(indptr[-1])
        else np.empty(0, dtype=np.int32)
    )
    return indptr, labels


def _membership(rows, status):
    indptr, labels = _csr(rows)
    return SiteComponentMembership(
        radius=0.75,
        site_start=0,
        site_stop=len(rows),
        inner_indptr=indptr,
        inner_labels=labels,
        outer_indptr=indptr.copy(),
        outer_labels=labels.copy(),
        status=np.asarray(status, dtype=np.uint8),
    )


class _MembershipHarness:
    labels_at_cut = PowerCoverHierarchy.labels_at_cut

    def __init__(self, membership):
        self.membership = membership
        self.calls = []

    def site_components_at_cut(self, radius, **kwargs):
        self.calls.append((radius, kwargs))
        return self.membership


def test_confirmed_unique_rejects_possible_and_multicomponent_memberships():
    membership = _membership(
        [[10], [10], [3, 10], [3], [], [3], [3], [99]],
        [
            CONFIRMED,
            CONFIRMED,
            CONFIRMED,
            POSSIBLE,
            EXCLUDED,
            CONFIRMED,
            CONFIRMED,
            CONFIRMED,
        ],
    )
    hierarchy = _MembershipHarness(membership)

    labels = hierarchy.labels_at_cut(
        0.75,
        2,
        "confirmed_unique",
        max_sites=17,
        max_candidate_cells_per_site=19,
        max_active_cells=23,
        max_total_cells=29,
    )

    # Component 3 is recoded before component 10 even though component 10 is
    # encountered first.  The confirmed overlap, possible relation, excluded
    # site and singleton component are all strict noise.
    np.testing.assert_array_equal(labels, [1, 1, -1, -1, -1, 0, 0, -1])
    assert labels.dtype == np.int32
    assert hierarchy.calls == [
        (
            0.75,
            {
                "site_start": 0,
                "site_stop": None,
                "max_sites": 17,
                "max_candidate_cells_per_site": 19,
                "max_active_cells": 23,
                "max_total_cells": 29,
            },
        )
    ]


class _BallAABBHarness:
    labels_at_cut = PowerCoverHierarchy.labels_at_cut

    def __init__(self):
        self.sites = np.array(
            [
                [0.50, 0.0, 0.0],
                [0.55, 0.0, 0.0],
                [1.50, 0.0, 0.0],
                [2.50, 0.0, 0.0],
            ]
        )
        self.additive = np.zeros(4)
        self.spec = GridSpec(
            bbox_min=(0.0, 0.0, 0.0),
            bbox_max=(3.0, 0.0, 0.0),
            shape=(3, 1, 1),
            cell_widths=(1.0, 0.0, 0.0),
            half_diagonal=0.5,
        )
        self.forest = cubical_msf_cpu(
            np.array([0.0, 10.0, 0.0]), dims=(3, 1, 1)
        )

    def site_components_at_cut(self, radius, **kwargs):
        return site_component_membership_cpu(
            self.sites,
            self.additive,
            self.spec,
            self.forest,
            self.forest,
            radius=radius,
            **kwargs,
        )


def test_flat_labels_use_ball_aabb_overlap_and_global_minimum_size():
    hierarchy = _BallAABBHarness()

    labels = hierarchy.labels_at_cut(0.6, min_cluster_size=2)

    # The ball at 1.5 intersects both disconnected active cells and must not
    # receive an arbitrary component.  The right component is then too small.
    np.testing.assert_array_equal(labels, [0, 0, -1, -1])


@pytest.mark.parametrize(
    ("kwargs", "error", "message"),
    [
        ({"min_cluster_size": True}, TypeError, "must be an integer"),
        ({"min_cluster_size": 1.5}, TypeError, "must be an integer"),
        ({"min_cluster_size": 0}, ValueError, "must be positive"),
        ({"policy": 1}, TypeError, "must be a string"),
        ({"policy": "nearest"}, ValueError, "confirmed_unique"),
    ],
)
def test_flat_label_options_are_validated_before_coverage(kwargs, error, message):
    hierarchy = _MembershipHarness(_membership([[0]], [CONFIRMED]))

    with pytest.raises(error, match=message):
        hierarchy.labels_at_cut(0.5, **kwargs)

    assert hierarchy.calls == []


def test_flat_labels_delegate_radius_and_cpu_budget_validation():
    hierarchy = _BallAABBHarness()

    with pytest.raises(ValueError, match="radius must be finite"):
        hierarchy.labels_at_cut(np.nan)
    with pytest.raises(RuntimeError, match="above max_sites"):
        hierarchy.labels_at_cut(0.6, max_sites=3)
