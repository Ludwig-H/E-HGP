"""CUDA-oriented building blocks for the enveloped 3-D power-cover backend."""

from .cubical_msf import (
    CubicalMSFResult,
    build_cubical_msf,
    cubical_msf_cpu,
    cubical_msf_gpu,
    estimate_cubical_msf_memory,
)
from .api import PowerCover3D, PowerCoverHierarchy, pilot_kappa_calibration
from .contracts import (
    AccuracyReport,
    GridSpec,
    PowerCoverConfig,
    SpatialTransform,
    estimate_memory,
)
from .spatial_core import (
    CertifiedPowerKNN,
    ExactKDTreeIndex,
    PowerKNNCertificationError,
    brute_force_power_topk,
    build_grid_spec,
    evaluate_cubical_field,
    regularize_sites_streaming,
    solve_entropy_gibbs,
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
    "GridSpec",
    "PowerCover3D",
    "PowerCoverConfig",
    "PowerCoverHierarchy",
    "PowerKNNCertificationError",
    "SpatialTransform",
    "brute_force_power_topk",
    "build_grid_spec",
    "estimate_memory",
    "evaluate_cubical_field",
    "pilot_kappa_calibration",
    "regularize_sites_streaming",
    "solve_entropy_gibbs",
]
