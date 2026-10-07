"""KITTI / SemanticKITTI : velodyne .bin = float32 petit-boutiste (x, y, z, reflectance) par retour ;
.label = uint32 par retour (16 bits bas = classe semantique, 16 bits hauts = instance)."""
from __future__ import annotations

from pathlib import Path

import numpy as np


def read_velodyne(path: Path) -> np.ndarray:
    raw = np.fromfile(path, dtype='<f4')
    if raw.size % 4:
        raise ValueError('taille de %s non multiple de 16 octets' % path)
    return raw.reshape(-1, 4)


def read_labels(path: Path, n: int) -> np.ndarray:
    labels = np.fromfile(path, dtype='<u4')
    if labels.size != n:
        raise ValueError('%s : %d etiquettes pour %d retours' % (path, labels.size, n))
    return labels
