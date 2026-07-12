"""CUDA-oriented building blocks for the enveloped 3-D power-cover backend."""

from .cubical_msf import (
    CubicalMSFResult,
    build_cubical_msf,
    cubical_msf_cpu,
    cubical_msf_gpu,
    estimate_cubical_msf_memory,
)
from .api import (
    NeighborAuditError,
    PowerCover3D,
    PowerCoverHierarchy,
    pilot_kappa_calibration,
)
from .coverage import (
    SiteComponentMembership,
    site_component_membership_cpu,
)
from .contracts import (
    AccuracyReport,
    GridSpec,
    PowerCoverConfig,
    SpatialTransform,
    estimate_memory,
)
from .progress import ProgressCallback, ProgressEvent
from .spatial_core import (
    CertifiedPowerKNN,
    ExactKDTreeIndex,
    ExactLiftedPowerKNN,
    GridCoordinateResolutionError,
    GridResolutionBudgetError,
    PowerKNNCertificationError,
    brute_force_power_topk,
    build_grid_spec,
    build_grid_spec_for_local_scale,
    evaluate_cubical_field,
    regularize_sites_streaming,
    solve_entropy_gibbs,
    solve_entropy_gibbs_budget,
)

__all__ = [
    "CubicalMSFResult",
    "build_cubical_msf",
    "cubical_msf_cpu",
    "cubical_msf_gpu",
    "estimate_cubical_msf_memory",
    "AccuracyReport",
    "CertifiedPowerKNN",
    "ExactKDTreeIndex",
    "ExactLiftedPowerKNN",
    "GridCoordinateResolutionError",
    "GridResolutionBudgetError",
    "GridSpec",
    "NeighborAuditError",
    "PowerCover3D",
    "PowerCoverConfig",
    "PowerCoverHierarchy",
    "SiteComponentMembership",
    "ProgressCallback",
    "ProgressEvent",
    "PowerKNNCertificationError",
    "SpatialTransform",
    "brute_force_power_topk",
    "build_grid_spec",
    "build_grid_spec_for_local_scale",
    "estimate_memory",
    "evaluate_cubical_field",
    "pilot_kappa_calibration",
    "regularize_sites_streaming",
    "site_component_membership_cpu",
    "solve_entropy_gibbs",
    "solve_entropy_gibbs_budget",
]
