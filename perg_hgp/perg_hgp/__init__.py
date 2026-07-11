"""Public PERG-HGP API with the optional historical atlas loaded lazily."""

from __future__ import annotations

from importlib import import_module
from typing import Any

from .backends.power_cover_3d_cuda.api import (
    PowerCover3D,
    PowerCoverHierarchy,
    pilot_kappa_calibration,
)
from .backends.power_cover_3d_cuda.contracts import PowerCoverConfig

_LAZY_EXPORTS = {
    "PERGHGPClusterer": (".estimator", "PERGHGPClusterer"),
    "PERGHGPConfig": (".config", "PERGHGPConfig"),
    "SpatialGrid3D": (".grid", "SpatialGrid3D"),
    "solve_local_gibbs_entropy": (".local_gibbs", "solve_local_gibbs_entropy"),
    "compute_regularized_sites": (".local_gibbs", "compute_regularized_sites"),
    "compute_rank_soft_dp": (".rank_field", "compute_rank_soft_dp"),
    "compute_rank_shell_weights": (".rank_field", "compute_rank_shell_weights"),
    "compute_rank_soft_dp_batched": (".rank_field", "compute_rank_soft_dp_batched"),
    "WitnessPool": (".witnesses", "WitnessPool"),
    "refine_witness_pool": (".witnesses", "refine_witness_pool"),
    "lift_witness_pool": (".witnesses", "lift_witness_pool"),
    "extract_top_cofaces": (".cofaces", "extract_top_cofaces"),
    "solve_weighted_miniball_active_set_3d": (
        ".cofaces",
        "solve_weighted_miniball_active_set_3d",
    ),
    "local_gabriel_filter": (".gabriel", "local_gabriel_filter"),
    "global_gabriel_grid_test": (".gabriel", "global_gabriel_grid_test"),
    "compute_facet_ids": (".dual_graph", "compute_facet_ids"),
    "build_dual_edges": (".dual_graph", "build_dual_edges"),
    "dual_graph_mst": (".hierarchy", "dual_graph_mst"),
    "condense_tree": (".hierarchy", "condense_tree"),
    "extract_labels": (".hierarchy", "extract_labels"),
}


def __getattr__(name: str) -> Any:
    try:
        module_name, attribute = _LAZY_EXPORTS[name]
    except KeyError as error:
        raise AttributeError(
            f"module {__name__!r} has no attribute {name!r}"
        ) from error
    value = getattr(import_module(module_name, __name__), attribute)
    globals()[name] = value
    return value


def __dir__() -> list[str]:
    return sorted(set(globals()) | set(_LAZY_EXPORTS))


__all__ = [
    *sorted(_LAZY_EXPORTS),
    "PowerCover3D",
    "PowerCoverHierarchy",
    "PowerCoverConfig",
    "pilot_kappa_calibration",
]
