"""Fixtures MEB locales : la partie et la population globale sont deux objets distincts."""
import itertools
import random
from dataclasses import dataclass


@dataclass(frozen=True)
class Fixture:
    name: str
    records: tuple
    part_points: tuple


def make(name, points, part=None):
    points = tuple(tuple(p) for p in points)
    part = points if part is None else tuple(tuple(p) for p in part)
    records = tuple(p + (30001 - 17*i,) for i, p in enumerate(points))
    return Fixture(name, records, part)


def fixtures(bits):
    maximum = (1 << bits) - 1
    corners = tuple(itertools.product((0, maximum), repeat=3))
    regular = tuple(corners[i] for i in (0, 3, 5, 6))
    negative = ((1, 2, 0), (0, 5, 0), (8, 1, 0), (8, 9, 0))
    square = ((0, 0, 0), (4, 0, 0), (4, 4, 0), (0, 4, 0))
    obtuse_prefix = ((10, 5, 5), (9, 8, 5), (5, 2, 1), (1, 5, 8))
    circle = tuple((6+x, 6+y, 0) for x in range(-5, 6) for y in range(-5, 6) if x*x+y*y == 25)
    line = tuple((maximum*i//11, 0, 0) for i in range(12))
    thin = ((0, 0, 0), (maximum, 1, 0), (maximum-1, 1, 0), (maximum, 1, 1))
    out = [
        make('singleton', [(0, 0, 0)]),
        make('singleton_global', [(0, 0, 0), (3, 2, 1), (7, 6, 5)], [(3, 2, 1)]),
        make('demi_entier', [(0, 0, 0), (1, 1, 1)]),
        make('ligne_descente', [(0, 0, 0), (4, 0, 0), (5, 0, 0), (11, 0, 0)], [(0, 0, 0), (11, 0, 0)]),
        make('ligne_complete', [(0, 0, 0), (4, 0, 0), (5, 0, 0), (11, 0, 0)]),
        make('triangle_droit', [(0, 0, 0), (4, 0, 0), (0, 4, 0)]),
        make('triangle_obtus', [(0, 0, 0), (4, 0, 0), (1, 1, 0)]),
        make('triangle_aigu', [(0, 0, 0), (4, 0, 0), (2, 3, 0)]),
        make('carre_centre', square + ((2, 2, 0),)),
        make('carre_coface', square + ((2, 2, 0),), square),
        make('support_negatif', negative),
        make('support_local_global', negative + ((9, 8, 0),), negative),
        make('meme_boule_globale', negative + ((9, 8, 0),)),
        make('prefixe_obtus_q4', obtuse_prefix),
        make('poids_nul_q4', [(0, 0, 0), (4, 0, 0), (2, 3, 0), (2, 0, 2)]),
        make('coquille_douze', circle + ((6, 6, 0),), [(1, 6, 0), (11, 6, 0)]),
        make('multiplicites', [(0, 0, 0), (4, 0, 0), (0, 0, 0), (2, 0, 0), (2, 0, 0)],
             [(0, 0, 0), (4, 0, 0), (2, 0, 0)]),
        make('cube_huit', corners),
        make('tetra_extreme', regular),
        make('tetra_local_cube_global', corners, regular),
        make('triangle_extreme', corners, regular[:3]),
        make('ligne_douze_extreme', line),
        make('tetra_mince_extreme', thin),
    ]
    rng = random.Random(1112026)
    for size in (3, 5, 8, 12):
        for scale in (7, maximum):
            points = []
            while len(points) < size:
                p = tuple(rng.randrange(scale+1) for _ in range(3))
                if p not in points:
                    points.append(p)
            out.append(make('aleatoire_n%d_s%d' % (size, scale), points))
    return out
