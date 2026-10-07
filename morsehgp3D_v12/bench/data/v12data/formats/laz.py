"""LAZ / COPC (LAS compresse par LASzip) : delegue a laspy + lazrs (paquets optionnels).

La decompression LASzip n'est pas reimplementee : sur la VM G4 (Python 3.10 sans pip), envoyer un Python portable
contenant numpy, laspy et lazrs comme donnee, ou decompresser localement et n'envoyer que les fichiers convertis.
Rend les memes champs que `formats.las.read_points` (entiers bruts X, Y, Z, echelle et decalage de l'en-tete).
"""
from __future__ import annotations

from pathlib import Path

import numpy as np


def available() -> bool:
    try:
        import laspy  # noqa: F401
        import lazrs  # noqa: F401
    except ImportError:
        return False
    return True


def read_points(path: Path) -> dict:
    try:
        import laspy
    except ImportError as error:
        raise RuntimeError('lecture LAZ impossible : laspy + lazrs absents (pip install "laspy[lazrs]")') from error
    with laspy.open(str(path), laz_backend=laspy.LazBackend.Lazrs) as reader:
        las = reader.read()
    header = las.header
    pts = las.points
    fmt = header.point_format.id
    n = len(pts)
    out = dict(header=dict(version=str(header.version), point_format=fmt, count=n,
                           scale=tuple(float(v) for v in header.scales),
                           offset=tuple(float(v) for v in header.offsets), compressed=True,
                           bounds=dict(min=[float(v) for v in header.mins], max=[float(v) for v in header.maxs])),
               X=np.asarray(pts.X, dtype=np.int32), Y=np.asarray(pts.Y, dtype=np.int32),
               Z=np.asarray(pts.Z, dtype=np.int32),
               classification=np.asarray(pts.classification, dtype=np.uint8),
               return_number=np.asarray(pts.return_number, dtype=np.uint8),
               number_of_returns=np.asarray(pts.number_of_returns, dtype=np.uint8),
               withheld=np.asarray(pts.withheld, dtype=bool))
    names = set(pts.point_format.dimension_names)
    out['overlap'] = np.asarray(pts.overlap, dtype=bool) if 'overlap' in names else (out['classification'] == 12)
    return out
