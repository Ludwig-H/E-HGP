"""Source-tree shim for the historical PowerCover3D reference package.

The installable package lives in ``perg_hgp/perg_hgp``. Extending the package
search path keeps imports canonical when tests run directly from the repository
root, without loading a second copy of the public classes.
"""

from pathlib import Path

_INNER = str(Path(__file__).with_name("perg_hgp"))
__path__ = [_INNER]

from .backends.power_cover_3d_cuda.api import (
    PowerCover3D,
    PowerCoverHierarchy,
    pilot_kappa_calibration,
)
from .backends.power_cover_3d_cuda.contracts import PowerCoverConfig

__all__ = [
    "PowerCover3D",
    "PowerCoverHierarchy",
    "PowerCoverConfig",
    "pilot_kappa_calibration",
]
