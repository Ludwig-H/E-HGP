"""sin et cos deterministes (aucune libm) : reduction a [-pi, pi] par une constante double fixe, puis serie de Taylor
de 30 termes evaluee dans un ordre fixe en flottants IEEE (+, *, / correctement arrondis). Meme resultat a l'octet
sur toute machine IEEE 754 binaire64 (CPython n'emploie pas de FMA pour ces operations). Erreur < 1e-15 pour |x| <=
quelques dizaines de radians : largement suffisant pour des angles d'attitude.
"""
from __future__ import annotations

PI = 3.141592653589793
TWO_PI = 6.283185307179586


def _reduce(x: float) -> float:
    k = round(x / TWO_PI)
    return x - k * TWO_PI


def sin(x: float) -> float:
    x = _reduce(float(x))
    term, total, x2 = x, x, x * x
    for n in range(1, 30):
        term = -term * x2 / ((2 * n) * (2 * n + 1))
        total += term
    return total


def cos(x: float) -> float:
    x = _reduce(float(x))
    term, total, x2 = 1.0, 1.0, x * x
    for n in range(1, 30):
        term = -term * x2 / ((2 * n - 1) * (2 * n))
        total += term
    return total


def matmul3(a, b):
    """Produit 3x3 en flottants Python, ordre fixe ((a0 b0 + a1 b1) + a2 b2)."""
    return [[(a[i][0] * b[0][j] + a[i][1] * b[1][j]) + a[i][2] * b[2][j] for j in range(3)] for i in range(3)]
