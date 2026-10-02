"""Petits nuages graves pour le catalogue ; toutes les coordonnees suivent le profil compile."""
from dataclasses import dataclass
from itertools import product


@dataclass(frozen=True)
class Fixture:
    name: str
    points: tuple
    strata: tuple
    split: bool = False


def fixtures(bits):
    maximum = (1 << bits) - 1
    a = maximum // 2
    rows = [
        ('singleton', [(0, 0, 0)], ('dim0',)),
        ('pair', [(0, 0, 0), (3, 0, 0)], ('dim1', 'q2')),
        ('line3', [(0, 0, 0), (2, 0, 0), (5, 0, 0)], ('dim1', 'admission')),
        ('line12', [(i, 0, 0) for i in range(12)], ('dim1', 'all_k')),
        ('right_triangle', [(0, 0, 0), (2, 0, 0), (0, 2, 0)], ('dim2', 'qmin', 'shell')),
        ('acute_triangle', [(0, 0, 0), (4, 0, 0), (2, 3, 0)], ('dim2', 'q3')),
        ('obtuse_triangle', [(0, 0, 0), (4, 0, 0), (1, 1, 0)], ('dim2', 'obtuse')),
        ('regular_tetra', [(0, 0, 0), (2, 2, 0), (2, 0, 2), (0, 2, 2)], ('dim3', 'q4')),
        ('right_tetra', [(0, 0, 0), (2, 0, 0), (0, 2, 0), (0, 0, 2)], ('dim3', 'negative_weight')),
        ('zero_weight', [(0, 0, 0), (4, 0, 0), (2, 3, 0), (2, 0, 2)], ('dim3', 'zero_weight', 'qmin')),
        # Source : test num permanent. En Morton : C,A,B,D ; C,A,B est obtus mais le tetraedre est strict.
        ('obtuse_prefix', [(10, 5, 5), (9, 8, 5), (5, 2, 1), (1, 5, 8)], ('dim3', 'obtuse_q4')),
        # Coquille5 de rayon5 sans paire antipodale ni triangle-support ; premier tetra strict Morton (0,1,3,4).
        ('extended_q4', [(10, 5, 5), (9, 8, 5), (5, 2, 1), (1, 5, 8), (9, 2, 5)],
         ('dim3', 'q4', 'shell', 'canonical', 'extended_q4')),
        ('octa', [(4, 2, 2), (0, 2, 2), (2, 4, 2), (2, 0, 2), (2, 2, 4), (2, 2, 0)], ('dim3', 'shell', 'canonical')),
        ('octa_center', [(4, 2, 2), (0, 2, 2), (2, 4, 2), (2, 0, 2), (2, 2, 4), (2, 2, 0), (2, 2, 2)],
         ('dim3', 'shell', 'admission')),
        ('shared_witness', [(10, 5, 5), (0, 5, 5), (5, 10, 5), (5, 0, 5), (5, 5, 10), (5, 5, 0), (5, 5, 5)],
         ('dim3', 'shell', 'shared_witness')),
        ('cube', list(product((0, 4), repeat=3)), ('dim3', 'shell', 'canonical', 'plateau')),
        ('circle', [(0, 5, 0), (2, 1, 0), (2, 9, 0), (5, 0, 0), (5, 10, 0), (8, 1, 0), (8, 9, 0), (10, 5, 0)],
         ('dim2', 'shell', 'canonical')),
        ('extended_q3', [(10, 5, 5), (8, 9, 5), (2, 9, 5), (1, 2, 5), (9, 2, 5), (5, 5, 10), (8, 5, 1)],
         ('dim3', 'shell', 'qmin', 'canonical')),
        ('grid12', list(product(range(3), range(2), range(2))), ('dim3', 'all_k', 'plateau')),
        ('maximum_face', [(maximum, 0, 0), (maximum, maximum - 1, 0), (maximum, a, maximum)], ('dim2', 'extreme')),
        ('maximum_tetra', [(0, 0, 0), (maximum, maximum, 0), (maximum, 0, maximum), (0, maximum, maximum)],
         ('dim3', 'extreme', 'q4')),
        # R_triangle^2 = a^2 + 1/(4(a^2+1)) > R_pair^2=a^2, mais leur conversion binary64 est identique.
        ('close_levels', [(0, 0, 0), (2 * a, 0, 0), (a, a, 1)], ('dim2', 'extreme', 'close_levels')),
    ]
    out = [Fixture(name, tuple(points), strata, name in ('line12', 'octa', 'cube', 'grid12', 'shared_witness'))
           for name, points, strata in rows]
    # Generateur entier local deterministe, sans le generateur du moteur ni celui de la reference FULL.
    state = 1100211
    for number in range(6):
        points = set()
        while len(points) < 6 + number % 3:
            point = []
            for _axis in range(3):
                state = (1664525 * state + 1013904223) & 0xffffffff
                point.append((state >> 12) % 13)
            points.add(tuple(point))
        out.append(Fixture('random%d' % number, tuple(sorted(points)), ('dim3', 'random')))
    return tuple(out)


def records(points):
    """PointId ne coincide pas avec SiteIdx ; inclut deliberement le PointId maximal valide."""
    return tuple(tuple(p) + ((2**32 - 1) if i == 0 else 1009 + 17 * i,) for i, p in enumerate(points))
