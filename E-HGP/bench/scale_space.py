"""Espace d'echelle entropique : combien de points critiques selon epsilon ?

Cette sonde mesure le fait qui decide si E-HGP est possible. En dimension
grande, le catalogue critique exact de l'ordre `k` compte presque
`C(n, k)` spheres (cf. `docs/OBSTRUCTION_GRANDE_DIMENSION.md`) : la tour
FULL est alors un objet de taille exponentielle, qu'aucun algorithme ne
peut publier. La regularisation entropique lisse `a_k` en le niveau de
Fermi `mu_k^epsilon`, dont les minima locaux fusionnent quand `epsilon`
croit. La sonde compte les minima distincts trouves par descente, pour une
grille de `epsilon`, et les compare au catalogue exact enumere.

Usage :

    python3 bench/scale_space.py --n 11 --k 3 --dims 2,3,5,20,50 --seed 31

Sortie : une ligne par (dimension, epsilon) avec le nombre de minima
distincts, le niveau minimal atteint, et l'ecart au niveau exact minimal.
Aucun flottant n'intervient dans le catalogue exact ; les flottants ne
servent qu'a la recherche, et le niveau publie d'un candidat est
recalcule exactement.
"""

import argparse
import random
import sys
from itertools import combinations
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "src"))

from ehgp.exact.meb import minimum_enclosing_ball, squared_distance, to_rational_cloud
from ehgp.soft.fermi import energies_at, occupancy_slope, ramp_band, ramp_level


def soft_descent(cloud, start, mass, epsilon, family="ramp", steps=120, tolerance=1e-11):
    """Descente de gradient a recherche lineaire sur le niveau de Fermi.

    Le gradient vaut `2 (y - m(y))` ou `m(y)` est le barycentre pondere de
    la coquille ; le pas `1/2` donne l'iteration `y <- m(y)`, qui SURTIRE
    quand les poids se concentrent. On fait donc une recherche lineaire par
    rebroussement, seule facon d'atteindre les minima fins quand `epsilon`
    est petit.
    """
    position = np.asarray(start, dtype=float).copy()
    level = ramp_level(energies_at(cloud, position), mass, epsilon)
    for _step in range(steps):
        energies = energies_at(cloud, position)
        slopes = ramp_band(energies, level, epsilon).astype(float)
        total = slopes.sum()
        if total <= 0.0:
            slopes = occupancy_slope((level - energies) / epsilon, "ramp").astype(float)
            total = slopes.sum()
        if total <= 0.0:
            break
        barycentre = (slopes @ cloud) / total
        direction = barycentre - position
        norm = float(np.linalg.norm(direction))
        if norm <= tolerance * (1.0 + float(np.linalg.norm(position))):
            break
        factor = 1.0
        improved = False
        for _trial in range(16):
            candidate = position + factor * direction
            candidate_level = ramp_level(energies_at(cloud, candidate), mass, epsilon)
            if candidate_level < level:
                position = candidate
                level = candidate_level
                improved = True
                break
            factor *= 0.5
        if not improved:
            break
    return position, level


def exact_catalogue(points, mass):
    """Catalogue exact des spheres critiques de rang ferme `mass`."""
    count = len(points)
    catalogue = {}
    for subset in combinations(range(count), mass):
        center, radius, _support = minimum_enclosing_ball([points[index] for index in subset])
        closed = 0
        for index in range(count):
            distance = squared_distance(center, points[index])
            if distance <= radius:
                closed += 1
        if closed == mass:
            catalogue[(center, radius)] = subset
    return catalogue


def main(argv=None):
    parser = argparse.ArgumentParser(description="espace d'echelle entropique")
    parser.add_argument("--n", type=int, default=11)
    parser.add_argument("--k", type=int, default=3)
    parser.add_argument("--dims", type=str, default="2,3,5,20,50")
    parser.add_argument("--seed", type=int, default=31)
    parser.add_argument("--extent", type=int, default=1000)
    parser.add_argument("--family", type=str, default="ramp")
    parser.add_argument("--min-starts", type=int, default=20)
    options = parser.parse_args(argv)

    dimensions = [int(value) for value in options.dims.split(",") if value]
    generator = random.Random(options.seed)
    print(
        "n={} k={} graine={} famille={}".format(
            options.n, options.k, options.seed, options.family
        )
    )
    print(
        "{:>4} {:>12} {:>8} {:>8} {:>14} {:>14}".format(
            "d", "epsilon/var", "exact", "minima", "niveau min", "exact min"
        )
    )
    for dimension in dimensions:
        cloud = [
            tuple(generator.randint(0, options.extent) for _ in range(dimension))
            for _ in range(options.n)
        ]
        points = to_rational_cloud(cloud)
        catalogue = exact_catalogue(points, options.k)
        exact_minimum = min(float(level) for _center, level in catalogue) if catalogue else 0.0
        array = np.array(cloud, dtype=float)
        variance = float(np.mean(np.var(array, axis=0)))
        starts = [array[index] for index in range(options.n)]
        for left in range(options.n):
            for right in range(left + 1, options.n):
                starts.append(0.5 * (array[left] + array[right]))
        if len(starts) < options.min_starts:
            return 3
        for ratio in (1e-4, 1e-3, 1e-2, 1e-1, 1.0):
            epsilon = ratio * variance
            minima = {}
            for start in starts:
                position, level = soft_descent(
                    array, start, float(options.k) - 0.5, epsilon, options.family
                )
                minima[round(level, 6)] = position
            print(
                "{:>4} {:>12.0e} {:>8} {:>8} {:>14.1f} {:>14.1f}".format(
                    dimension,
                    ratio,
                    len(catalogue),
                    len(minima),
                    min(minima) if minima else float("nan"),
                    exact_minimum,
                )
            )
    return 0


if __name__ == "__main__":
    sys.exit(main())
