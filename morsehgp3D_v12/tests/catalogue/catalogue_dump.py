#!/usr/bin/env python3
"""Lecteur MHGP12DP (version 1, genre << catalogue >>, CONTRAT_CATALOGUE.md, paragraphe 8 bis) des portes du
catalogue : oracle.py et euler.py y lisent les exports de la sonde (sites, boules, populations, nombre de niveaux) ;
oracle.py y prend aussi la geometrie exacte de S* (centre circonscrit et rayon carre, rationnels exacts) et le controle
de l'ordre publie de la v12 (rang, puis S* par liste triee des positions). Le differentiel contre la v11 n'est pas
juge ici mais par le lecteur de transition reference/transition_catalogue.py (porte diff_case.py). Un nom de trame
hors ASCII imprimable est refuse, comme par ce lecteur (CST-0227). Python 3.10 nu, aucun assert.
"""
import struct
import sys
from array import array
from fractions import Fraction

NONE = 0xFFFFFFFF


class Refusal(Exception):
    pass


def read_dump(path):
    try:
        with open(path, 'rb') as handle:
            data = handle.read()
    except OSError as error:
        raise Refusal('illisible %s : %s' % (path, error))
    if len(data) < 64 or data[:8] != b'MHGP12DP':
        raise Refusal('magie absente : %s' % path)
    version, kind, bits, kmax, order, sections, sites = struct.unpack_from('<6IQ', data, 8)
    if version != 1 or kind != 1 or order != 0:
        raise Refusal('en-tete hors format (version %d, genre %d, ordre %d) : %s' % (version, kind, order, path))
    name = data[40:64].rstrip(b'\0')
    if any(not 0x20 <= byte <= 0x7e for byte in name):
        raise Refusal('nom de trame hors ASCII imprimable (%r) : %s' % (name, path))
    out = {'bits': bits, 'kmax': kmax, 'sites': sites, 'frame': name.decode('ascii')}
    at = 64
    for _ in range(sections):
        if at + 24 > len(data):
            raise Refusal('section tronquee : %s' % path)
        tag = data[at:at + 8].rstrip(b'\0').decode('ascii', 'replace')
        size, _reserved, count = struct.unpack_from('<IIQ', data, at + 8)
        at += 24
        nbytes = size * count
        if at + nbytes > len(data):
            raise Refusal('donnees tronquees (%s) : %s' % (tag, path))
        out[tag] = (size, data[at:at + nbytes])
        at += nbytes + (-nbytes) % 8
    if at != len(data):
        raise Refusal('octets en trop : %s' % path)
    for tag, size in (('SITEXYZ', 12), ('BALLS', 32), ('POPOFF', 8), ('POPVAL', 4), ('NLEVELS', 8)):
        if tag not in out or out[tag][0] != size:
            raise Refusal('section %s absente ou de taille inattendue : %s' % (tag, path))
    return out


def words(blob, code):
    values = array(code)
    values.frombytes(blob)
    if sys.byteorder != 'little':
        values.byteswap()
    return values


class Catalogue(object):
    def __init__(self, dump):
        self.sites = dump['sites']
        xyz = words(dump['SITEXYZ'][1], 'I')
        self.pos = [(xyz[3 * i], xyz[3 * i + 1], xyz[3 * i + 2]) for i in range(len(xyz) // 3)]
        raw = words(dump['BALLS'][1], 'I')
        self.count = len(raw) // 8
        self.rank = raw[0::8]
        self.p = raw[1::8]
        self.m = raw[2::8]
        self.q = raw[3::8]
        self.sstar = [tuple(s for s in raw[8 * b + 4:8 * b + 8] if s != NONE) for b in range(self.count)]
        self.off = words(dump['POPOFF'][1], 'Q')
        self.val = words(dump['POPVAL'][1], 'I')
        self.levels = struct.unpack('<Q', dump['NLEVELS'][1])[0]

    def population(self, b):
        return self.val[self.off[b]:self.off[b + 1]]


# ------------------------------------------------------------------------------------------- geometrie exacte
def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def solve3(rows, rhs):
    """Elimination de Gauss exacte ; None si singulier."""
    m = [[Fraction(v) for v in row] + [Fraction(r)] for row, r in zip(rows, rhs)]
    n = len(rhs)
    for col in range(n):
        pivot = next((i for i in range(col, n) if m[i][col] != 0), None)
        if pivot is None:
            return None
        m[col], m[pivot] = m[pivot], m[col]
        for i in range(n):
            if i != col and m[i][col] != 0:
                f = m[i][col] / m[col][col]
                m[i] = [x - f * y for x, y in zip(m[i], m[col])]
    return [m[i][n] / m[i][i] for i in range(n)]


def circumcenter(points):
    """Centre du cercle ou de la sphere circonscrit aux 2, 3 ou 4 points (dans leur sous-espace affine)."""
    a = points[0]
    if len(points) == 2:
        return tuple(Fraction(x + y, 2) for x, y in zip(a, points[1]))
    vectors = [sub(p, a) for p in points[1:]]
    if len(points) == 3:
        u, v = vectors
        # c = a + s u + t v, |c-a|^2 = |c-b|^2 = |c-c'|^2 : systeme de Gram 2x2
        coefficients = solve3([[dot(u, u), dot(u, v)], [dot(u, v), dot(v, v)]],
                              [Fraction(dot(u, u), 2), Fraction(dot(v, v), 2)])
        if coefficients is None:
            return None
        s, t = coefficients
        return tuple(x + s * p + t * q for x, p, q in zip(a, u, v))
    coefficients = solve3([list(vec) for vec in vectors], [Fraction(dot(vec, vec), 2) for vec in vectors])
    if coefficients is None:
        return None
    return tuple(x + c for x, c in zip(a, coefficients))


def ball_key(cat, sstar):
    points = [cat.pos[s] for s in sstar]
    center = circumcenter(points)
    if center is None:
        return None
    radius = sum((c - x) ** 2 for c, x in zip(center, points[0]))
    return center, radius


def check_order(v12, errors):
    previous = None
    for b in range(v12.count):
        current = (v12.rank[b], sorted(v12.pos[s] for s in v12.sstar[b]))
        if previous is not None and not previous < current:
            errors.append('ordre publie de la v12 rompu a la boule %d' % b)
            return
        previous = current
