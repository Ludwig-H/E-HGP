"""Sonde de l'obstruction de taille : part des parties a boule englobante vide.

Une partie `F` de cardinal `k` dont la boule englobante minimale ne contient
aucune autre observation engendre une NAISSANCE de composante de `Gamma_k`
(son centre est un point critique d'indice 0 de `D_k`). Le nombre de telles
parties minore donc le nombre de naissances de la tour, c'est-a-dire la
taille de la SORTIE.

La sonde enumere toutes les parties de cardinal `k`, calcule la boule
englobante minimale EXACTE en rationnels, et compte celles dont la boule
fermee ne contient aucune autre observation. Aucun flottant n'intervient.

Usage :

    python3 bench/empty_ball_fraction.py --n 11 --ks 2,3,4,5 \
        --dims 2,3,5,10,20,50,100 --seed 5 --extent 1000

Code de sortie : 0 conforme, 2 refus avant calcul, 3 plancher de couverture
viole (trop peu de parties examinees).
"""

import argparse
import random
import sys
from itertools import combinations
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "src"))

from ehgp.exact.meb import minimum_enclosing_ball, squared_distance, to_rational_cloud

EXIT_OK = 0
EXIT_REFUSED = 2
EXIT_FLOOR = 3


def empty_ball_count(points, order):
    """Compte les parties de cardinal `order` a boule englobante vide."""
    count = len(points)
    empty = 0
    total = 0
    for subset in combinations(range(count), order):
        center, radius, _support = minimum_enclosing_ball([points[index] for index in subset])
        total += 1
        outside = True
        for index in range(count):
            if index in subset:
                continue
            if squared_distance(center, points[index]) <= radius:
                outside = False
                break
        if outside:
            empty += 1
    return empty, total


def main(argv=None):
    parser = argparse.ArgumentParser(description="part des parties a boule vide")
    parser.add_argument("--n", type=int, default=11)
    parser.add_argument("--ks", type=str, default="2,3,4,5")
    parser.add_argument("--dims", type=str, default="2,3,5,10,20,50,100")
    parser.add_argument("--seed", type=int, default=5)
    parser.add_argument("--extent", type=int, default=1000)
    parser.add_argument("--min-subsets", type=int, default=100)
    options = parser.parse_args(argv)

    orders = [int(value) for value in options.ks.split(",") if value]
    dimensions = [int(value) for value in options.dims.split(",") if value]
    if not orders or not dimensions or options.n < 2:
        return EXIT_REFUSED

    generator = random.Random(options.seed)
    print(
        "n={} graine={} etendue={} : part des parties de cardinal k a boule englobante VIDE".format(
            options.n, options.seed, options.extent
        )
    )
    header = "{:>5} | ".format("d") + " | ".join("k={:<2}".format(order) for order in orders)
    print(header)
    examined = 0
    for dimension in dimensions:
        cloud = [
            tuple(generator.randint(0, options.extent) for _ in range(dimension))
            for _ in range(options.n)
        ]
        points = to_rational_cloud(cloud)
        cells = []
        for order in orders:
            empty, total = empty_ball_count(points, order)
            examined += total
            cells.append("{:>4}/{:<4}".format(empty, total))
        print("{:>5} | ".format(dimension) + " | ".join(cells))
    if examined < options.min_subsets:
        return EXIT_FLOOR
    return EXIT_OK


if __name__ == "__main__":
    sys.exit(main())
