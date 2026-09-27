"""Retrait du sol par Patchwork++, avec exactement les paramètres de la v8.

Même contrat que ``morsehgp3D_v8/bench/patchwork_ground_probe.cpp`` : seuls
x, y, z sont passés, la portée ``min_range < r <= max_range`` est filtrée
avant l'appel, et le masque vaut 0 (indécis, conservé), 1 (sol, retiré) ou
2 (non-sol). Sur 08/000000 le masque obtenu est identique octet pour octet
au masque versionné par la v8 (sha256 ``9db3fe5c…``), donc les trames
« sans sol » des démos sont exactement l'entrée de la tour v9.
"""
from __future__ import annotations

import contextlib
import os

import numpy as np

V8_PARAMS = {
    'RNR_intensity_thr': 0.2, 'RNR_ver_angle_thr': -15, 'adaptive_seed_selection_margin': -1.2,
    'elevation_thr': [0, 0, 0, 0], 'enable_RNR': False, 'enable_RVPF': True, 'enable_TGR': True,
    'flatness_thr': [0, 0, 0, 0], 'intensity_thr': 0, 'max_elevation_storage': 1000,
    'max_flatness_storage': 1000, 'max_range': 80, 'min_range': 2.7, 'num_iter': 3, 'num_lpr': 20,
    'num_min_pts': 10, 'num_rings_each_zone': [2, 4, 4, 4], 'num_rings_of_interest': 4,
    'num_sectors_each_zone': [16, 32, 54, 32], 'num_zones': 4, 'sensor_height': 1.723,
    'th_dist': 0.125, 'th_dist_v': 0.1, 'th_seeds': 0.125, 'th_seeds_v': 0.25,
    'uprightness_thr': 0.707, 'verbose': False,
}
V8_MASK_SHA256_08_000000 = '9db3fe5c3f5c4c5d955dde9fcaf61f52ac304f4c23aa68076fd934b589caaf8f'


@contextlib.contextmanager
def _quiet_stdout():
    # Patchwork++ imprime une ligne d'initialisation sur la sortie C.
    fd = os.dup(1)
    try:
        with open(os.devnull, 'wb') as null:
            os.dup2(null.fileno(), 1)
            yield
    finally:
        os.dup2(fd, 1)
        os.close(fd)


def ground_mask(xyzi: np.ndarray) -> np.ndarray:
    """Masque u8 : 0 indécis (conservé), 1 sol (retiré), 2 non-sol."""
    import pypatchworkpp

    xyz = np.ascontiguousarray(xyzi[:, :3], dtype=np.float32)
    mask = np.zeros(len(xyz), np.uint8)
    r = np.hypot(xyz[:, 0].astype(np.float64), xyz[:, 1].astype(np.float64))
    ok = (r > V8_PARAMS['min_range']) & (r <= V8_PARAMS['max_range'])
    ok &= np.ascontiguousarray(xyzi[:, 2]).view(np.uint32) != 0x00800000
    ids = np.flatnonzero(ok)
    params = pypatchworkpp.Parameters()
    for key, value in V8_PARAMS.items():
        setattr(params, key, value)
    with _quiet_stdout():
        pw = pypatchworkpp.patchworkpp(params)
        pw.estimateGround(np.ascontiguousarray(xyz[ids].astype(np.float64)))
    ground = np.asarray(pw.getGroundIndices(), dtype=np.int64)
    nonground = np.asarray(pw.getNongroundIndices(), dtype=np.int64)
    mask[ids[ground]] = 1
    mask[ids[nonground]] = 2
    return mask
