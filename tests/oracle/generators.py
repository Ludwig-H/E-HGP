"""Compatibility imports for the shared deterministic oracle generators."""

from reference.morsehgp3d_oracle.generators import (
    Point3,
    affine_dimension,
    generate_affine_cloud,
)


__all__ = ["Point3", "affine_dimension", "generate_affine_cloud"]
