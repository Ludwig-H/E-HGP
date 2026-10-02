"""Petits nuages globaux et supports externes : aucune liste de feuille n'est un oracle."""
import itertools
import random
from dataclasses import dataclass

from fraction_model import sphere


@dataclass(frozen=True)
class Fixture:
    name: str
    records: tuple
    support: tuple


def make(name, points, support):
    points = tuple(dict.fromkeys(tuple(point) for point in points))
    # IDs volontairement opposes a l'ordre d'entree et sans rapport avec SiteIdx.
    records = tuple(tuple(point) + (10007 - 17 * i,) for i, point in enumerate(points))
    return Fixture(name, records, tuple(tuple(point) for point in support))


def fixtures(bits):
    maximum = (1 << bits) - 1
    corners = list(itertools.product((0, maximum), repeat=3))
    octa = [(4 + x, 4 + y, 4 + z) for x, y, z in
            ((-2, 0, 0), (2, 0, 0), (0, -2, 0), (0, 2, 0), (0, 0, -2), (0, 0, 2))]
    inside = [(4, 4, 4), (4, 5, 4), (4, 4, 5)]
    circle = [(6 + x, 6 + y, 0) for x in range(-5, 6) for y in range(-5, 6) if x*x + y*y == 25]
    thin = [(0, 0, 0), (maximum, 1, 0), (maximum - 1, 1, 0), (maximum, 1, 1)]
    thin_queries = [(maximum // 2, 0, 0), (maximum // 2, 1, 0), (1, 0, 1), (1, 1, 1)]
    regular = [corners[i] for i in (0, 3, 5, 6)]
    out = [
        make('point_contact', octa + inside, [(4, 4, 4)]),
        make('point_absent', octa + inside, [(0, 0, 0)]),
        make('seuil_trois', octa + inside + [(0, 0, 0), (8, 8, 8)], octa[:2]),
        make('coquille_douze', circle + [(6, 6, 0)], [(1, 6, 0), (11, 6, 0)]),
        make('cube_fractionnaire', itertools.product((0, 1), repeat=3), [(0, 0, 0), (1, 1, 1)]),
        make('triangle_rationnel', itertools.product(range(5), range(4), range(2)), [(0, 0, 0), (4, 0, 0), (2, 3, 0)]),
        make('tetra_droit', itertools.product((0, 2, 4), repeat=3), [(0, 0, 0), (4, 0, 0), (0, 4, 0), (0, 0, 4)]),
        make('triangle_exterieur', thin + thin_queries + corners, thin[:3]),
        make('tetra_exterieur', thin + thin_queries + corners, thin),
        make('triangle_grands_bits', corners + [(maximum // 2,) * 3], regular[:3]),
        make('tetra_grands_bits', corners + [(maximum // 2,) * 3], regular),
        make('poids_nul', itertools.product(range(5), range(4), range(3)),
             [(0, 0, 0), (4, 0, 0), (2, 3, 0), (2, 0, 2)]),
        make('bloc_interieur', inside, octa[:2]),
        make('support_hors_nuage', [(4, 4, 4), (4, 5, 4), (0, 0, 0)], octa[:2]),
        make('singleton', [(maximum, 0, maximum)], [(maximum, 0, maximum)]),
    ]
    for i, support in enumerate(itertools.permutations([(0, 0, 0), (6, 6, 0), (6, 0, 6), (0, 6, 6)])):
        out.append(make('permutation_q4_%02d' % i, itertools.product((0, 3, 6), repeat=3), support))
    rng = random.Random(113107)
    for q in range(1, 5):
        for scale in (7, maximum):
            points = [tuple(rng.randrange(scale + 1) for _ in range(3)) for _ in range(37)]
            while True:
                support = tuple(tuple(rng.randrange(scale + 1) for _ in range(3)) for _ in range(q))
                try:
                    sphere(support)
                    break
                except ValueError:
                    pass
            out.append(make('aleatoire_q%d_%d' % (q, scale), points + list(support), support))
    return out
