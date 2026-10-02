"""Familles de nuages gravees : fixtures aux coordonnees exactes et familles a graine fixe.

Toutes les coordonnees sont des entiers de [0, 2^18) : chaque nuage peut etre soumis au binaire fige de la v10.
Le generateur pseudo-aleatoire est ecrit ici (SplitMix64) : la suite des nuages ne depend ni de la version de
Python ni du module random.

Un nuage est un Cloud(name, points, kmax). Les doublons sont ecrits comme points repetes.
"""
from collections import namedtuple

Cloud = namedtuple('Cloud', 'name points kmax')

E18 = 262143  # plus grande coordonnee u18


class Rng(object):
    """SplitMix64 (Steele, Lea, Flood 2014)."""

    def __init__(self, seed):
        self.state = seed & 0xFFFFFFFFFFFFFFFF

    def next(self):
        self.state = (self.state + 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
        z = self.state
        z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & 0xFFFFFFFFFFFFFFFF
        z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & 0xFFFFFFFFFFFFFFFF
        return z ^ (z >> 31)

    def between(self, lo, hi):
        return lo + self.next() % (hi - lo + 1)

    def choice(self, seq):
        return seq[self.next() % len(seq)]

    def shuffle(self, items):
        for i in range(len(items) - 1, 0, -1):
            j = self.next() % (i + 1)
            items[i], items[j] = items[j], items[i]


# ---------------------------------------------------------------- fixtures gravees

def _weighted(sites, weights):
    return [p for p, w in zip(sites, weights) for _ in range(w)]


def _line(xs):
    return [(x, 0, 0) for x in xs]


SQUARE = [(0, 0, 0), (2, 0, 0), (0, 2, 0), (2, 2, 0)]
CUBE = [(x, y, z) for x in (0, 2) for y in (0, 2) for z in (0, 2)]
OCTA = [(15, 10, 10), (5, 10, 10), (10, 15, 10), (10, 5, 10), (10, 10, 15), (10, 10, 5)]
OCTA_W = [(8, 10, 10), (12, 10, 10), (10, 8, 10), (10, 12, 10), (10, 10, 8), (10, 10, 12), (10, 10, 10)]

# (nom, points, K de la suite rapide, ce que la fixture grave)
FIXTURES = (
    ('e5', [(0, 0, 7), (0, 9, 6), (1, 4, 0), (0, 0, 1), (4, 1, 2)], 4,
     'fixture E5 de la v10 (contre-exemple de l\'audit de la v9 au graphe de Gabriel)'),
    ('audit3', _line((0, 2, 5)), 3, 'verticale d\'une naissance avant l\'entree des points (audit du 29 sept.)'),
    ('pair', _line((0, 2)), 2, 'entrees posterieures au dernier niveau critique'),
    ('line4', _line((0, 1, 4, 7)), 4, 'idem a K = 4'),
    ('line5', _line((0, 2, 4, 10, 12)), 3, 'trois naissances d\'ordre 2 au niveau 1 ; entree exactement a une fusion'),
    ('line024', _line((0, 2, 4)), 3, 'F1 : le site 2 couvert au meme niveau par deux composantes (egalite cover)'),
    ('line01269', _line((0, 1, 2, 6, 9)), 3, 'cover non verticale entre K = 2 et K = 3'),
    ('line_f3', _line((0, 20, 22, 50, 52)), 3, 'F3 : la reunion libre des ordres n\'est pas laminaire'),
    ('triangle_far', [(1, 0, 0), (0, 1, 0), (0, 0, 1), (2, 2, 0), (50, 50, 50), (51, 50, 50)], 3,
     'fusion ternaire d\'ordre 2 au niveau 2/3 sans aucun point entre'),
    ('square', SQUARE, 4, 'quatre sites cocycliques : coquille etendue, naissance unique a K = 3'),
    ('square_plus', SQUARE + [(7, 7, 1)], 5, 'carre et un site hors plan'),
    ('octa', OCTA, 6, 'octaedre : trois paires antipodales sur une sphere'),
    ('cube', CUBE, 5, 'cube : huit sites cospheriques, tetraedres inscrits'),
    ('tetra_center', [(0, 0, 0), (2, 2, 0), (2, 0, 2), (0, 2, 2), (1, 1, 1)], 5,
     'tetraedre regulier et son centre : le centre est interieur a la boule circonscrite'),
    ('right_triangle', [(0, 0, 0), (3, 0, 0), (0, 4, 0)], 3, 'triangle rectangle : q_min = 2, coquille de trois sites'),
    ('generic6', [(0, 0, 0), (9, 1, 0), (2, 8, 1), (5, 5, 7), (1, 3, 9), (8, 8, 8)], 6,
     'boules a au moins deux sites interieurs'),
    ('firstcov_k3_n6', [(2, 4, 4), (2, 8, 5), (2, 9, 1), (3, 7, 0), (6, 9, 6), (8, 10, 7)], 4,
     'premiere couverture distincte de C n X a K = 3'),
    ('neighbour_q2', [(1000, 1000, 1000), (1100, 1000, 1000), (1010, 1120, 1000), (1010, 1119, 1016)], 3,
     'voisin a 100 contre paire serree a 120'),
    # deux triangles equilateraux face a face (these, section 6.1) : pont de 2000, de 1998 et de 1700
    ('two_triangles', [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (4000, 2000, 0), (5732, 3000, 0),
                       (5732, 1000, 0)], 3, 'these section 6.1 : sept paires, puis ABC | CD | DEF, puis tout'),
    ('two_triangles_1998', [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (3998, 2000, 0), (5730, 3000, 0),
                            (5730, 1000, 0)], 3, 'these section 6.1, pont plus court de 0,1 %'),
    ('two_triangles_1700', [(268, 3000, 0), (268, 1000, 0), (2000, 2000, 0), (3700, 2000, 0), (5432, 3000, 0),
                            (5432, 1000, 0)], 3, 'these section 6.1, pont de 1700'),
    # extremes du domaine u18
    ('pair_boundary', [(0, 0, 0), (E18, E18, E18)], 2, 'paire aux deux coins du domaine'),
    ('square_top', [(0, 0, E18), (2, 0, E18), (0, 2, E18), (2, 2, E18)], 4, 'carre au bord superieur'),
    ('cube_max', [(x, y, z) for x in (0, E18) for y in (0, E18) for z in (0, E18)], 5, 'cube du domaine entier'),
    # collisions de niveaux
    ('circle25_pair', [(15, 10, 3), (7, 14, 3), (7, 6, 3), (40, 40, 40), (50, 40, 40)], 3,
     'niveau 25 ecrit 100/4 par une paire et 409600/16384 par un triangle : meme rang, deux ecritures'),
    ('double_collision', [(0, 0, 0), (78404, 0, 0), (39202, 55440, 0), (150000, 150000, 150000),
                          (198046, 200290, 195586)], 3,
     'niveaux exacts distincts 1728896403 et 1728896403 + 1/768398400 : meme double'),
    ('q3_tetra_form', [(7, 8, 4), (3, 6, 6), (4, 5, 4), (8, 5, 8), (7, 4, 8)], 5,
     'boule q_min = 3 a coquille etendue dont le niveau s\'ecrit dans la forme d\'un tetraedre'),
    # multiplicites (doublons)
    ('pair_weighted', _weighted([(12, 12, 12), (14, 12, 12)], [3, 1]), 4, 'paire dont un site pese 3'),
    ('square_weighted', _weighted(SQUARE + [(1, 1, 5)], [1, 2, 1, 1, 3]), 5, 'carre a sites ponderes'),
    ('octa_weighted', _weighted(OCTA_W, [3, 1, 2, 1, 4, 1, 2]), 5, 'octaedre pondere et son centre'),
    ('triangle_weighted', _weighted([(0, 0, 0), (6, 0, 0), (3, 5, 0)], [2, 1, 1]), 4,
     'triangle aigu dont un sommet est double : la jonction passe de p + 2 a p + 3'),
    ('triangle_weighted_311', _weighted([(0, 0, 0), (6, 0, 0), (3, 5, 0)], [3, 1, 1]), 5,
     'triangle aigu de poids (3, 1, 1)'),
    ('all_equal', [(5, 5, 5)] * 4, 4, 'un seul site de poids 4 : naissances de niveau nul a chaque ordre'),
)


# ---------------------------------------------------------------- familles a graine fixe

def _integer_sphere(r2, center):
    lim = int(r2 ** 0.5) + 1
    return [(x + center, y + center, z + center) for x in range(-lim, lim + 1) for y in range(-lim, lim + 1)
            for z in range(-lim, lim + 1) if x * x + y * y + z * z == r2]


def _integer_circle(r2, center, z):
    lim = int(r2 ** 0.5) + 1
    return [(x + center, y + center, z) for x in range(-lim, lim + 1) for y in range(-lim, lim + 1)
            if x * x + y * y == r2]


def _distinct(rng, n, draw):
    """n positions distinctes tirees par draw ; au plus 1000 n tirages, sinon la famille est mal definie."""
    pts, seen = [], set()
    for _ in range(1000 * n):
        if len(pts) == n:
            break
        p = draw(rng)
        if p not in seen:
            seen.add(p)
            pts.append(p)
    if len(pts) != n:
        raise ValueError('famille incapable de rendre %d positions distinctes' % n)
    return pts


def _subset(rng, pool, n):
    pool = list(pool)
    rng.shuffle(pool)
    return pool[:n]


def _generic(rng, n):
    return _distinct(rng, n, lambda r: (r.between(0, 1000), r.between(0, 1000), r.between(0, 1000)))


def _generic_u18(rng, n):
    return _distinct(rng, n, lambda r: (r.between(0, E18), r.between(0, E18), r.between(0, E18)))


def _corner_u18(rng, n):
    return _distinct(rng, n, lambda r: (r.between(E18 - 500, E18), r.between(E18 - 500, E18), r.between(E18 - 500, E18)))


def _grid3(rng, n):
    return _distinct(rng, n, lambda r: (r.between(0, 2), r.between(0, 2), r.between(0, 2)))


def _grid4(rng, n):
    return _distinct(rng, n, lambda r: (r.between(0, 3), r.between(0, 3), r.between(0, 3)))


def _coplanar(rng, n):
    return _distinct(rng, n, lambda r: (r.between(0, 6), r.between(0, 6), 4))


def _collinear(rng, n):
    d = rng.choice(((1, 0, 0), (0, 1, 0), (0, 0, 1), (1, 1, 0), (1, 2, 3), (2, 0, 1)))
    span = max(14, 2 * n)  # au moins 2 n + 1 positions sur la droite : le tirage de n positions distinctes termine
    return _distinct(rng, n, lambda r: tuple(c * t for c, t in zip(d, [r.between(0, span)] * 3)))


def _clusters(rng, n):
    return _distinct(rng, n, lambda r: tuple(r.choice((0, 2, 4)) + r.between(0, 1) for _ in range(3)))


def _cocircular(rng, n):
    circle = _integer_circle(rng.choice((25, 65)), 10, 3)
    extra = rng.between(0, max(0, min(2, n - 3)))
    pts = _subset(rng, circle, n - extra)
    return pts + _distinct(rng, extra, lambda r: (r.between(0, 20), r.between(0, 20), r.between(0, 6)))


def _cospherical(rng, n):
    sphere = _integer_sphere(rng.choice((9, 14, 26)), 8)
    extra = rng.between(0, max(0, min(2, n - 4)))
    pts = _subset(rng, sphere, n - extra)
    for p in _distinct(rng, 3 * extra, lambda r: (r.between(3, 13), r.between(3, 13), r.between(3, 13))):
        if p not in pts and len(pts) < n:
            pts.append(p)
    return pts


def _duplicates(rng, n):
    """Doublons : positions tirees dans une famille degeneree ou generique, multiplicites 1, 1, 2 ou 3."""
    base = rng.choice((_generic, _grid3, _coplanar, _collinear, _cospherical))
    sites = base(rng, max(2, n - rng.between(1, max(1, n // 2))))
    pts = list(sites)
    while len(pts) < n:
        pts.append(rng.choice(sites))
    rng.shuffle(pts)
    return pts


FAMILIES = (('generic', _generic), ('generic_u18', _generic_u18), ('corner_u18', _corner_u18), ('grid3', _grid3),
            ('grid4', _grid4), ('coplanar', _coplanar), ('collinear', _collinear), ('clusters', _clusters),
            ('cocircular', _cocircular), ('cospherical', _cospherical), ('duplicates', _duplicates))


def family(name, count, nmin, nmax, kmax, seed):
    """count nuages de la famille : n tire dans [nmin, nmax], K = min(kmax, n)."""
    draw = dict(FAMILIES)[name]
    out = []
    for i in range(count):
        rng = Rng(seed * 1000003 + i)
        n = rng.between(nmin, nmax)
        pts = draw(rng, n)
        if name != 'duplicates':
            rng.shuffle(pts)
        out.append(Cloud('%s_%d_%d' % (name, seed, i), pts, min(kmax, len(pts))))
    return out


def fixtures(kmax=None):
    """Fixtures gravees ; kmax : ordre maximal impose (sinon celui de la suite rapide), borne par n."""
    return [Cloud(name, list(pts), min(len(pts), kmax if kmax is not None else k)) for name, pts, k, _why in FIXTURES]


def fast_suite():
    """Suite rapide : toutes les fixtures, puis 28 nuages de 4 a 8 points par famille, K <= 4."""
    out = fixtures()
    for name, _draw in FAMILIES:
        out += family(name, 28, 4, 8, 4, 11)
    return out


def full_suite():
    """Suite complete : fixtures a leur ordre grave puis jusqu'a K = 10 ; par famille, 300 nuages de 6 a 11 points
    a K <= 6, 150 de 9 a 12 points a K <= 10, 48 nuages larges de 13 a 16 points a K <= 3 (sauts de la descente) et
    8 nuages de 13 ou 14 points a K <= 10. Les nuages sont entrelaces par famille : une tranche i mod N est
    equilibree."""
    engraved = fixtures()
    blocks = [engraved + [c for c in fixtures(10) if c not in engraved]]
    for name, _draw in FAMILIES:
        blocks.append(family(name, 300, 6, 11, 6, 21) + family(name, 150, 9, 12, 10, 22) +
                      family(name, 48, 13, 16, 3, 23) + family(name, 8, 13, 14, 10, 24))
    out = list(blocks[0])
    for i in range(max(len(b) for b in blocks[1:])):
        out.extend(b[i] for b in blocks[1:] if i < len(b))
    return out


SUITES = {'fast': fast_suite, 'full': full_suite}
