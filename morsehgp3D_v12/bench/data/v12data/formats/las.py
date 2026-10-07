"""LAS 1.0 a 1.4 NON compresse, en numpy pur (formats de point 0 a 10).

Rend les entiers bruts X, Y, Z (int32), l'echelle et le decalage de l'en-tete, la classification et les drapeaux
utiles. La conversion en millimetres exacts se fait dans `common.quantize_scaled_int_mm` (echelle lue en decimal).
Un fichier LAZ (bit 7 ou 6 du format de point, ou extension .laz) est refuse ici : voir `formats.laz`.
"""
from __future__ import annotations

import struct
from pathlib import Path

import numpy as np

# taille minimale de chaque format de point (octets) ; les octets supplementaires (extra bytes) sont ignores
_BASE = {0: 20, 1: 28, 2: 26, 3: 34, 4: 57, 5: 63, 6: 30, 7: 36, 8: 38, 9: 59, 10: 67}


def read_header(path: Path) -> dict:
    with open(path, 'rb') as handle:
        head = handle.read(375)
    if head[:4] != b'LASF':
        raise ValueError('%s : pas un fichier LAS' % path)
    major, minor = head[24], head[25]
    header_size = struct.unpack_from('<H', head, 94)[0]
    offset_to_points = struct.unpack_from('<I', head, 96)[0]
    fmt_raw = head[104]
    record_length = struct.unpack_from('<H', head, 105)[0]
    legacy_count = struct.unpack_from('<I', head, 107)[0]
    scale = struct.unpack_from('<3d', head, 131)
    offset = struct.unpack_from('<3d', head, 155)
    maxima_minima = struct.unpack_from('<6d', head, 179)
    count = legacy_count
    if (major, minor) >= (1, 4) and header_size >= 375:
        count = struct.unpack_from('<Q', head, 247)[0] or legacy_count
    return dict(version='%d.%d' % (major, minor), header_size=header_size, offset_to_points=offset_to_points,
                point_format=fmt_raw & 0x3F, compressed=bool(fmt_raw & 0xC0), record_length=record_length,
                count=count, scale=scale, offset=offset,
                bounds=dict(max_x=maxima_minima[0], min_x=maxima_minima[1], max_y=maxima_minima[2],
                            min_y=maxima_minima[3], max_z=maxima_minima[4], min_z=maxima_minima[5]))


def read_points(path: Path) -> dict:
    header = read_header(path)
    if header['compressed'] or str(path).lower().endswith('.laz'):
        raise ValueError('%s : LAZ compresse, utiliser formats.laz' % path)
    fmt, length, n = header['point_format'], header['record_length'], header['count']
    if fmt not in _BASE or length < _BASE[fmt]:
        raise ValueError('format de point %d (longueur %d) non pris en charge' % (fmt, length))
    raw = np.memmap(path, dtype=np.uint8, mode='r', offset=header['offset_to_points'], shape=(n * length,))
    rec = raw.reshape(n, length)
    xyz = np.ascontiguousarray(rec[:, 0:12]).view('<i4').reshape(n, 3).astype(np.int32)
    if fmt <= 5:
        classification = (rec[:, 15] & 0x1F).astype(np.uint8)
        flags = rec[:, 14]
        returns = dict(return_number=(flags & 0x07).astype(np.uint8),
                       number_of_returns=((flags >> 3) & 0x07).astype(np.uint8))
        withheld = ((rec[:, 15] >> 7) & 1).astype(bool)
        overlap = classification == 12
    else:
        classification = rec[:, 16].astype(np.uint8)
        returns = dict(return_number=(rec[:, 14] & 0x0F).astype(np.uint8),
                       number_of_returns=((rec[:, 14] >> 4) & 0x0F).astype(np.uint8))
        withheld = ((rec[:, 15] >> 2) & 1).astype(bool)
        overlap = ((rec[:, 15] >> 3) & 1).astype(bool)
    return dict(header=header, X=xyz[:, 0], Y=xyz[:, 1], Z=xyz[:, 2], classification=classification,
                withheld=withheld, overlap=overlap, **returns)
