"""Juge independant de la tour : echantillonnage direct de `L_k(a)`.

Ce module ne connait pas `Gamma_k`. Il decide, pour chaque point d'une
grille rationnelle, le predicat ENTIER `#{i : ||y - x_i||^2 <= a} >= k`,
puis calcule les composantes connexes de l'ensemble marque par voisinage de
grille, et remonte pour chaque composante l'union des identifiants des
boules qui la couvrent.

C'est donc un juge d'une autre nature que `tower.py` : il approche
`pi_0(L_k(a))` par discretisation, sans aucune theorie de nerf, et sert a
falsifier l'usage du theoreme 2 en dimension quelconque. Il ne remplace
aucune preuve : un desaccord est un signal a expliquer, un accord n'est pas
un certificat. Il reste borne aux dimensions 1 a 3 (cout `M^d`).

Les comparaisons de distance sont exactes : la grille vit sur des entiers
apres mise au denominateur commun, et le predicat est reecrit en entiers
(`total * D <= N * echelle^2`). Aucun flottant n'intervient.
"""

from fractions import Fraction
from math import gcd

import numpy as np


def _integerise(cloud):
    """Met le nuage sur un denominateur commun, renvoie (entiers, denominateur)."""
    denominator = 1
    for point in cloud:
        for coordinate in point:
            other = Fraction(coordinate).denominator
            denominator = denominator * other // gcd(denominator, other)
    scaled = [
        tuple(int(Fraction(coordinate) * denominator) for coordinate in point) for point in cloud
    ]
    return scaled, denominator


def judge_level(cloud, order, level, steps=8, margin=Fraction(21, 20), connectivity=None):
    """Composantes de `L_order(level)` par grille entiere.

    Renvoie `(nombre_de_composantes, unions_triees)`, ou chaque union est le
    tuple trie des identifiants des observations dont la boule fermee
    rencontre la composante.
    """
    level = Fraction(level)
    if level < 0:
        raise ValueError("niveau negatif")
    scaled, denominator = _integerise(cloud)
    dimension = len(scaled[0])
    if dimension > 3:
        raise ValueError("juge de grille borne a d <= 3")
    scale = denominator * steps
    points = np.array(scaled, dtype=np.int64) * steps
    reach = int(Fraction(margin) * _sqrt_ceiling(level) * denominator * steps) + steps
    axes = []
    for axis in range(dimension):
        low = int(points[:, axis].min()) - reach
        high = int(points[:, axis].max()) + reach
        axes.append(np.arange(low, high + 1, dtype=np.int64))
    mesh = np.meshgrid(*axes, indexing="ij")
    counts = np.zeros(mesh[0].shape, dtype=np.int32)
    membership = []
    threshold_numerator = level.numerator * scale * scale
    threshold_denominator = level.denominator
    for index in range(points.shape[0]):
        total = np.zeros(mesh[0].shape, dtype=np.int64)
        for axis in range(dimension):
            difference = mesh[axis] - points[index, axis]
            total += difference * difference
        inside = total * threshold_denominator <= threshold_numerator
        membership.append(inside)
        counts += inside
    marked = counts >= order
    if not marked.any():
        return 0, ()
    labels, group_count = _label(marked, connectivity)
    unions = []
    for group in range(1, group_count + 1):
        selector = labels == group
        union = tuple(
            index for index in range(points.shape[0]) if bool((membership[index] & selector).any())
        )
        unions.append(union)
    return group_count, tuple(sorted(unions))


def _label(marked, connectivity=None):
    """Etiquetage des composantes connexes de la grille marquee.

    `connectivity = 1` ne relie que les voisins d'axe : cette variante
    SURESTIME le nombre de composantes, car la pointe d'une lentille fine
    oblique ne touche ses voisines que par la diagonale. `connectivity =
    ndim` (defaut) relie tous les voisins du cube : cette variante
    SOUS-ESTIME. Le juge honnete compare les deux (cf. `judge_bracket`).
    """
    from scipy import ndimage

    if connectivity is None:
        connectivity = marked.ndim
    structure = ndimage.generate_binary_structure(marked.ndim, connectivity)
    labels, count = ndimage.label(marked, structure=structure)
    return labels, int(count)


def judge_bracket(cloud, order, level, steps=8, margin=Fraction(21, 20)):
    """Encadrement du juge : (plein, axes) pour la meme grille.

    Renvoie `((bas, unions_bas), (haut, unions_haut))` ou `bas` utilise la
    connexite pleine et `haut` la connexite d'axe. La verite discrete est
    encadree ; si les deux coincident, la grille est jugee assez fine.
    """
    low = judge_level(cloud, order, level, steps=steps, margin=margin, connectivity=None)
    high = judge_level(cloud, order, level, steps=steps, margin=margin, connectivity=1)
    return low, high


def _sqrt_ceiling(value):
    """Plafond entier de la racine carree d'un rationnel positif."""
    value = Fraction(value)
    root = 0
    while (root + 1) * (root + 1) * value.denominator <= value.numerator:
        root += 1
    if root * root * value.denominator < value.numerator:
        root += 1
    return root
