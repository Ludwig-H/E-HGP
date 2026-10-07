"""Texte a colonnes (Semantic3D : `x y z intensite r g b`, une ligne par point ; etiquettes : un entier par ligne).

Lecture des trois premieres colonnes en float64 (numpy.loadtxt, par blocs), puis grille exacte
`common.quantize_float_mm`. C'est EXACT pour des decimaux d'au plus 3 chiffres apres la virgule et |x| < 4e12 m :
si x = d/1000 et f est le double le plus proche, |1000 f - d| <= 1000 |x| 2^-53 < 1/2, donc floor(1000 f + 1/2) = d.
Le nombre maximal de decimales des colonnes x, y, z est mesure sur tout le fichier (expression reguliere) et publie ;
au-dela de 3, l'arrondi est celui du double lu (il ne differe de l'arrondi decimal qu'aux egalites exactes).
"""
from __future__ import annotations

import re
from pathlib import Path

import numpy as np

_DECIMALS = re.compile(rb'(?m)^\s*([-+]?\d*\.?(\d*))\s+([-+]?\d*\.?(\d*))\s+([-+]?\d*\.?(\d*))')


def max_decimals(path: Path, block: int = 1 << 26) -> int:
    """Plus grand nombre de chiffres apres la virgule dans les colonnes 1-3 (fichier entier, par blocs)."""
    best = 0
    tail = b''
    with open(path, 'rb') as handle:
        while True:
            chunk = handle.read(block)
            if not chunk:
                break
            data = tail + chunk
            cut = data.rfind(b'\n')
            if cut < 0:
                tail = data
                continue
            body, tail = data[:cut + 1], data[cut + 1:]
            for m in _DECIMALS.finditer(body):
                best = max(best, len(m.group(2)), len(m.group(4)), len(m.group(6)))
    if tail.strip():
        m = _DECIMALS.match(tail)
        if m:
            best = max(best, len(m.group(2)), len(m.group(4)), len(m.group(6)))
    return best


def read_xyz_float64(path: Path, rows_per_block: int = 4_000_000) -> np.ndarray:
    """Colonnes x, y, z en float64 (n, 3), lecture par blocs."""
    parts = []
    with open(path, 'rb') as handle:
        while True:
            block = np.loadtxt(handle, dtype=np.float64, usecols=(0, 1, 2), max_rows=rows_per_block, ndmin=2)
            if block.size == 0:
                break
            parts.append(block)
            if len(block) < rows_per_block:
                break
    return np.concatenate(parts) if parts else np.empty((0, 3))


def read_int_column(path: Path) -> np.ndarray:
    return np.loadtxt(path, dtype=np.int64, ndmin=1)
