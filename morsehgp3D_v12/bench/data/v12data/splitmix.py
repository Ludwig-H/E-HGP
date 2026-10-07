"""SplitMix64 vectorise (numpy uint64) : flux reproductible a l'octet sur toute plateforme et toute version de numpy
(aucune loi de numpy.random, aucune fonction transcendante : entiers seulement)."""
from __future__ import annotations

import numpy as np

_GOLDEN = np.uint64(0x9E3779B97F4A7C15)
_M1 = np.uint64(0xBF58476D1CE4E5B9)
_M2 = np.uint64(0x94D049BB133111EB)


def stream(seed: int, start: int, count: int) -> np.ndarray:
    """Sorties start .. start+count-1 de SplitMix64(seed)."""
    with np.errstate(over='ignore'):
        idx = np.arange(start + 1, start + count + 1, dtype=np.uint64)
        z = np.uint64(seed & 0xFFFFFFFFFFFFFFFF) + idx * _GOLDEN
        z = (z ^ (z >> np.uint64(30))) * _M1
        z = (z ^ (z >> np.uint64(27))) * _M2
        return z ^ (z >> np.uint64(31))


def bounded(words: np.ndarray, bound: int) -> np.ndarray:
    """Entiers dans [0, bound) par produit haut 64x32 (biais < 2^-32, deterministe), bound < 2^32."""
    if not 0 < bound < (1 << 32):
        raise ValueError('borne hors [1, 2^32)')
    hi = (words >> np.uint64(32)).astype(np.uint64)
    return ((hi * np.uint64(bound)) >> np.uint64(32)).astype(np.int64)


class Rng:
    """Tirages successifs sur un flux SplitMix64."""

    def __init__(self, seed: int):
        self.seed, self.pos = seed, 0

    def words(self, count: int) -> np.ndarray:
        out = stream(self.seed, self.pos, count)
        self.pos += count
        return out

    def integers(self, bound: int, count: int) -> np.ndarray:
        return bounded(self.words(count), bound)

    def gaussian_int(self, sigma_num: int, sigma_den: int, count: int) -> np.ndarray:
        """Irwin-Hall entier : somme de 12 uniformes sur [0, 2^16) moins 6 2^16 (variance 2^32/12 * 12 = 2^32 :
        ecart-type 2^16), puis mise a l'echelle exacte sigma = sigma_num / sigma_den mm (arrondi par plancher)."""
        u = (self.words(12 * count) >> np.uint64(48)).astype(np.int64).reshape(count, 12)
        g = u.sum(axis=1) - 6 * (1 << 16)
        return np.floor_divide(g * sigma_num, sigma_den << 16)
