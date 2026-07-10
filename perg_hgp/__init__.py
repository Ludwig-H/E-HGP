"""Repository-root import shim using the canonical ``perg_hgp.*`` names.

The installable package lives in ``perg_hgp/perg_hgp``.  Extending this
package's search path avoids the former ``perg_hgp.inner`` loader, which
created a second copy of every class when the repository root was on
``sys.path``.
"""

from pathlib import Path

_INNER = str(Path(__file__).with_name("perg_hgp"))
__path__ = [_INNER]

from .estimator import PERGHGPClusterer
from .config import PERGHGPConfig
from .grid import SpatialGrid3D
from .local_gibbs import solve_local_gibbs_entropy, compute_regularized_sites
from .rank_field import compute_rank_soft_dp, compute_rank_shell_weights, compute_rank_soft_dp_batched
from .witnesses import WitnessPool, refine_witness_pool, lift_witness_pool
from .cofaces import extract_top_cofaces, solve_weighted_miniball_active_set_3d
from .gabriel import local_gabriel_filter, global_gabriel_grid_test
from .dual_graph import compute_facet_ids, build_dual_edges
from .hierarchy import dual_graph_mst, condense_tree, extract_labels
from .backends.power_cover_3d_cuda.api import (
    PowerCover3D,
    PowerCoverHierarchy,
    pilot_kappa_calibration,
)
from .backends.power_cover_3d_cuda.contracts import PowerCoverConfig

__all__ = [
    "PERGHGPClusterer",
    "PERGHGPConfig",
    "SpatialGrid3D",
    "solve_local_gibbs_entropy",
    "compute_regularized_sites",
    "compute_rank_soft_dp",
    "compute_rank_shell_weights",
    "compute_rank_soft_dp_batched",
    "WitnessPool",
    "refine_witness_pool",
    "lift_witness_pool",
    "extract_top_cofaces",
    "solve_weighted_miniball_active_set_3d",
    "local_gabriel_filter",
    "global_gabriel_grid_test",
    "compute_facet_ids",
    "build_dual_edges",
    "dual_graph_mst",
    "condense_tree",
    "extract_labels",
    "PowerCover3D",
    "PowerCoverHierarchy",
    "PowerCoverConfig",
    "pilot_kappa_calibration",
]
