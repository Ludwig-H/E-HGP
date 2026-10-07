#!/usr/bin/env python3
"""Lecteur de transition du catalogue (tranche T1 de la v12) : juge un vidage MHGP12DP de genre catalogue, le
CANDIDAT (la v12), contre un vidage de REFERENCE (la v11 gelee), par la regle du paragraphe 6.1 de
docs/CONTRAT_CATALOGUE.md (format et lecteur : paragraphe 8 bis). Bibliotheque standard seule ; entiers exacts
(centres rationnels a denominateur commun, reduits par pgcd) : aucun flottant dans une decision, aucun assert, memes
codes et memes lignes sous python3 -S -O. Temoins, cas et mutants : test_transition_catalogue.py.

Format lu (microbancs/mes_m3_m4_tour/common/format.hpp, version 1, genre 1 << catalogue >>) : en-tete de 64 octets
(MHGP12DP, version, genre, bits de coordonnees, K, ordre 0, nombre de sections, nombre de sites, trame), puis les
sections SITEXYZ (u32 x, y, z par SiteIdx), BALLS (u32 rank, p, m, q, sstar[4]), POPOFF (u64, B + 1 decalages),
POPVAL (u32 : I puis U, par boule) et NLEVELS (u64). Une section inattendue, absente, en double, de taille d'element
fausse ou tronquee, des octets en trop : refus.

Ce que le lecteur verifie.
  Par vidage, chacun dans SA convention (v11 : SiteIdx, rangs de Morton ; v12 : positions) :
    format       en-tete, sections, domaines d'indices, q dans 2..4, S* strictement croissant puis 0xFFFFFFFF,
                 decalages coherents (p + m par boule), I et U strictement croissants, positions des sites deux a
                 deux distinctes (l'identite d'un site est sa position) : refus, code 2 ;
    support      S* est un support minimal : centre circonscrit exact de ses 2, 3 ou 4 sites, coordonnees
                 barycentriques du centre strictement positives (paire ; triangle strictement aigu ; centre
                 strictement interieur au tetraedre non plat) ; sa sphere circonscrite est alors sa plus petite boule ;
    populations  tout site de I strictement dedans, tout site de U sur la sphere, S* inclus dans U. Seule la
                 correction se juge ici ; la completude (aucun site oublie) se juge contre l'autre vidage ;
    admission    p + q <= K + 1 (catalogue positif Cat_K, q = q_min >= 2) ;
    rangs        les niveaux (rayons carres exacts) croissent dans l'ordre publie, et le rang publie est le rang
                 dense du niveau recalcule (premier niveau : rang 1) ;
    niveaux      NLEVELS = nombre de niveaux distincts + 1 (le niveau nul des sites) ;
    ordre        a niveau egal, S* croissant (au sens large ; l'egalite est un doublon, juge par la bijection) selon
                 la convention : v11, suite des SiteIdx ; v12, liste triee des positions, ordre lexicographique des
                 coordonnees ; bourrage par un element plus grand que tout, sans effet : a niveau egal, un S* n'est
                 jamais prefixe d'un autre (une sphere qui porte un support porte sa boule) ;
    qmin         coquille etendue (m > q) : aucun support de cardinal < q dans U ;
    convention   coquille etendue : S* est le premier support de cardinal q de U dans l'ordre de la convention
                 (v11 : suites croissantes de SiteIdx ; v12 : listes triees des positions). Regle appliquee a TOUTE
                 coquille etendue, pas seulement aux S* qui different : un candidat qui garde le departage de la v11
                 sous la convention v12 est un desaccord meme si son S* egale celui de la reference.
  Entre les deux vidages :
    sites        meme ensemble de positions (jamais l'egalite des indices) ;
    bijection    boules identifiees par (centre exact, rayon carre) reduits, jamais par S* ; une boule de
                 reference sans image (absente), une boule candidate sans antecedent (en_trop), une cle repetee
                 dans un vidage (doublon) sont des desaccords ;
    populations  I et U egaux comme ensembles de positions (donc p et m) ;
    rangs        rangs egaux par boule appariee (rangs_croises) : consequence de la bijection et des rangs
                 recalcules, garde defensive ;
    supports     S* differents comme ensembles de positions : meme cardinal (cardinal_support : consequence de qmin
                 et de U egaux, garde defensive) ; chacun minimal et premier de SA convention (regles par vidage).
                 Les S* differents et les boules renumerotees (indice change) sont comptes, jamais des desaccords.
  La comparaison croisee avance niveau par niveau (les deux vidages sont ordonnes par niveaux) : la memoire de
  travail est celle d'un niveau. Si un vidage n'est pas ordonne par niveaux croissants, elle s'arrete sur un
  desaccord. Comptes publies par vidage : boules par q, coquilles etendues, coquilles a plusieurs supports de
  cardinal minimal (supports_multiples), coquilles dont le premier support differe entre les deux conventions
  (sstar_selon_convention : les S* que le passage a la v12 change).

Cout : lineaire en boules et en incidences ; une coquille etendue enumere ses parties de cardinal au plus q (en
pratique m <= 24). Juge exhaustif du vidage, pas d'echantillon (temps mesures : RAPPORT du lecteur).

Usage : transition_catalogue.py REFERENCE CANDIDAT [--convention-reference {v11,v12}]
        [--convention-candidat {v11,v12}] [--rapport FICHIER] [--ecarts N]
Defauts : reference v11, candidat v12 ; --convention-candidat v11 juge un candidat qui garde la convention de la v11
(auto-differentiel v11 contre v11). K et trame doivent etre egaux dans les deux en-tetes ; les profils (bits de
coordonnees) peuvent differer.
Codes : 0 conforme (ligne transition_catalogue_conforme) ; 1 desaccord (ligne transition_catalogue_desaccord, puis
les premiers ecarts) ; 2 refus d'entree ou d'usage ; 3 invariant interne viole (toute exception inattendue aussi :
jamais un code 1 par accident).
"""
import argparse
import array
import itertools
import json
import math
import struct
import sys
import time

CONFORME, DESACCORD, REFUS, INVARIANT = 0, 1, 2, 3
MAGIC = b'MHGP12DP'
VERSION = 1
KIND_CATALOGUE = 1
NONE = 0xFFFFFFFF
SECTIONS = {'SITEXYZ': 12, 'BALLS': 32, 'POPOFF': 8, 'POPVAL': 4, 'NLEVELS': 8}
CONVENTIONS = ('v11', 'v12')
PAD_V12 = (1 << 33, 0, 0)  # plus grand que toute position (coordonnees < 2^32)
ECARTS_DEFAUT = 20
SCHEMA = 'ehgp.v12.transition_catalogue.v1'


class Refus(Exception):
    """Entree ou usage refuses avant jugement (code 2)."""


class Invariant(Exception):
    """Invariant interne du lecteur viole (code 3)."""


class Ecarts(object):
    """Desaccords : nombre par categorie, et les premiers en detail."""

    def __init__(self, keep):
        self.counts = {}
        self.details = []
        self.keep = keep

    def add(self, category, where, message):
        self.counts[category] = self.counts.get(category, 0) + 1
        if len(self.details) < self.keep:
            self.details.append({'categorie': category, 'ou': where, 'detail': message})

    def total(self):
        return sum(self.counts.values())


# ---------------------------------------------------------------- arithmetique exacte (entiers Python)

def _sub(p, q):
    return (p[0] - q[0], p[1] - q[1], p[2] - q[2])


def _dot(u, v):
    return u[0] * v[0] + u[1] * v[1] + u[2] * v[2]


def _cross(u, v):
    return (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])


def _acute(u, v):
    """Triangle (0, u, v) strictement aigu : u.v > 0, (-u).(v - u) > 0, (-v).(u - v) > 0."""
    uu, vv, uv = _dot(u, u), _dot(v, v), _dot(u, v)
    return uv > 0 and uu > uv and vv > uv


def _inside_tetra(u, v, s, det, n, d):
    """Centre c' = n / d (d > 0) strictement interieur au tetraedre (0, u, v, s), det = u.(v x s) non nul :
    coordonnees barycentriques l1 = n.(v x s) / (d det), l2 = n.(s x u) / (d det), l3 = n.(u x v) / (d det) et
    l0 = 1 - l1 - l2 - l3, toutes strictement positives."""
    a1, a2, a3 = _dot(n, _cross(v, s)), _dot(n, _cross(s, u)), _dot(n, _cross(u, v))
    rest = d * det - a1 - a2 - a3
    if det > 0:
        return a1 > 0 and a2 > 0 and a3 > 0 and rest > 0
    return a1 < 0 and a2 < 0 and a3 < 0 and rest < 0


def circumsphere(points):
    """Sphere circonscrite de 2, 3 ou 4 sites distincts, dans le repere du premier : (n, d, minimal), centre
    c = points[0] + n / d, d > 0 ; minimal : centre dans l'interieur relatif de l'enveloppe des sites (support
    minimal ; la sphere est alors leur plus petite boule). (None, 0, False) si les sites sont affinement dependants."""
    a = points[0]
    u = _sub(points[1], a)
    if len(points) == 2:
        return u, 2, True
    v = _sub(points[2], a)
    if len(points) == 3:
        w = _cross(u, v)
        ww = _dot(w, w)
        if ww == 0:
            return None, 0, False
        uu, vv = _dot(u, u), _dot(v, v)
        t = (uu * v[0] - vv * u[0], uu * v[1] - vv * u[1], uu * v[2] - vv * u[2])
        return _cross(t, w), 2 * ww, _acute(u, v)
    s = _sub(points[3], a)
    vs, su, uv = _cross(v, s), _cross(s, u), _cross(u, v)
    det = _dot(u, vs)
    if det == 0:
        return None, 0, False
    uu, vv, ss = _dot(u, u), _dot(v, v), _dot(s, s)
    n = (uu * vs[0] + vv * su[0] + ss * uv[0], uu * vs[1] + vv * su[1] + ss * uv[1],
         uu * vs[2] + vv * su[2] + ss * uv[2])
    d = 2 * det
    if d < 0:
        n, d = (-n[0], -n[1], -n[2]), -d
    return n, d, _inside_tetra(u, v, s, det, n, d)


def ball_key(a, n, d):
    """Identite exacte d'une boule de centre a + n / d : (centre absolu (X, Y, Z, D) reduit par le pgcd commun,
    rayon carre |n|^2 / d^2 reduit (num, den))."""
    cx, cy, cz = d * a[0] + n[0], d * a[1] + n[1], d * a[2] + n[2]
    g = math.gcd(math.gcd(math.gcd(cx, cy), cz), d)
    num, den = _dot(n, n), d * d
    h = math.gcd(num, den)
    return (cx // g, cy // g, cz // g, d // g), (num // h, den // h)


def compare_levels(x, y):
    """Signe de x - y pour deux niveaux (num, den), den > 0."""
    left, right = x[0] * y[1], y[0] * x[1]
    return (left > right) - (left < right)


class Sphere(object):
    """Sphere d'une boule : ancre a (premier site de S*), centre a + n / d, d > 0 ; predicats de support exacts
    pour des sites de la sphere."""
    __slots__ = ('a', 'n', 'd')

    def __init__(self, a, n, d):
        self.a, self.n, self.d = a, n, d

    def offset(self, p):
        """d (c - p) : numerateur entier du centre vu du site p."""
        a, n, d = self.a, self.n, self.d
        return (d * (a[0] - p[0]) + n[0], d * (a[1] - p[1]) + n[1], d * (a[2] - p[2]) + n[2])

    def support(self, points):
        """Les sites (2, 3 ou 4, sur la sphere) sont un support minimal de la boule : centre dans l'interieur
        relatif de leur enveloppe."""
        p = points[0]
        if len(points) == 2:
            q = points[1]
            a, n, d = self.a, self.n, self.d
            return all(d * (p[i] + q[i] - 2 * a[i]) == 2 * n[i] for i in range(3))
        u, v = _sub(points[1], p), _sub(points[2], p)
        if len(points) == 3:
            return _acute(u, v) and _dot(_cross(u, v), self.offset(p)) == 0
        s = _sub(points[3], p)
        det = _dot(u, _cross(v, s))
        return det != 0 and _inside_tetra(u, v, s, det, self.offset(p), self.d)


def first_support(sphere, ordered, size, positions):
    """Premiere partie de taille size de ordered (indices de sites dans l'ordre de la convention), dans l'ordre
    lexicographique, qui est un support minimal de la sphere ; None s'il n'y en a pas."""
    for part in itertools.combinations(ordered, size):
        if sphere.support([positions[i] for i in part]):
            return part
    return None


# ---------------------------------------------------------------- lecture stricte

def _u32_view(data, start, count):
    """u32 little-endian : vue sans copie sur une machine little-endian, copie (array) sinon."""
    if sys.byteorder == 'little':
        return memoryview(data)[start:start + 4 * count].cast('I')
    values = array.array('I')
    values.frombytes(data[start:start + 4 * count])
    values.byteswap()
    return values


class Catalogue(object):
    """Vidage MHGP12DP de genre catalogue, lu en entier et controle dans sa structure (refus : Refus)."""

    def __init__(self, path, role, convention):
        if convention not in CONVENTIONS:
            raise Refus('convention inconnue %r' % (convention,))
        if struct.calcsize('I') != 4 or array.array('I').itemsize != 4:
            raise Invariant('entiers non signes de 32 bits indisponibles')
        self.path, self.role, self.convention = path, role, convention
        try:
            with open(path, 'rb') as handle:
                data = handle.read()
        except OSError as error:
            raise Refus('%s : lecture impossible de %s (%s)' % (role, path, error))
        self.data = data
        if len(data) < 64 or data[:8] != MAGIC:
            raise Refus('%s : magie inconnue (%s)' % (role, path))
        version, kind, bits, kmax, order, nsec = struct.unpack_from('<6I', data, 8)
        sites, = struct.unpack_from('<Q', data, 32)
        if version != VERSION or kind != KIND_CATALOGUE:
            raise Refus('%s : version %d, genre %d (attendus %d, %d)' % (role, version, kind, VERSION, KIND_CATALOGUE))
        if not 1 <= bits <= 32 or kmax < 1 or order != 0:
            raise Refus('%s : en-tete hors domaine (bits %d, K %d, ordre %d)' % (role, bits, kmax, order))
        self.bits, self.kmax = bits, kmax
        name = data[40:64].rstrip(b'\0')
        if any(not 0x20 <= byte <= 0x7e for byte in name):
            # CST-0227 : un decodage avec remplacement confondait deux identites distinctes (audit+FF, audit+FE)
            raise Refus('%s : nom de trame hors ASCII imprimable (%r)' % (role, name))
        self.frame = name.decode('ascii')
        sections, at = {}, 64
        for _ in range(nsec):
            if at + 24 > len(data):
                raise Refus('%s : en-tete de section tronque' % role)
            tag = data[at:at + 8].rstrip(b'\0').decode('ascii', 'replace')
            elem, _reserved, count = struct.unpack_from('<IIQ', data, at + 8)
            at += 24
            if tag not in SECTIONS:
                raise Refus('%s : section inattendue %r' % (role, tag))
            if tag in sections:
                raise Refus('%s : section en double %s' % (role, tag))
            if elem != SECTIONS[tag]:
                raise Refus('%s : taille d\'element %d pour %s' % (role, elem, tag))
            size = elem * count
            if at + size > len(data):
                raise Refus('%s : section %s tronquee' % (role, tag))
            sections[tag] = (at, count)
            at += size + (-size) % 8
        if at != len(data):
            raise Refus('%s : octets en trop ou manquants (%d octets lus sur %d)' % (role, at, len(data)))
        missing = sorted(set(SECTIONS) - set(sections))
        if missing:
            raise Refus('%s : sections absentes %s' % (role, ','.join(missing)))
        start, count = sections['SITEXYZ']
        if count != sites:
            raise Refus('%s : %d sites annonces, %d dans SITEXYZ' % (role, sites, count))
        flat = _u32_view(data, start, 3 * count)
        if count and max(flat) >= (1 << bits):
            raise Refus('%s : coordonnee hors de [0, 2^%d)' % (role, bits))
        self.positions = list(zip(flat[0::3], flat[1::3], flat[2::3]))
        if len(set(self.positions)) != count:
            raise Refus('%s : positions de sites en double (l\'identite d\'un site est sa position)' % role)
        self.n = count
        self.balls_at, self.balls = sections['BALLS']
        self.offsets_at, offsets = sections['POPOFF']
        values_at, self.incidences = sections['POPVAL']
        if offsets != self.balls + 1:
            raise Refus('%s : POPOFF a %d decalages pour %d boules' % (role, offsets, self.balls))
        first, = struct.unpack_from('<Q', data, self.offsets_at)
        last, = struct.unpack_from('<Q', data, self.offsets_at + 8 * self.balls)
        if first != 0 or last != self.incidences:
            raise Refus('%s : decalages extremes %d et %d (attendus 0 et %d)' % (role, first, last, self.incidences))
        self.values = _u32_view(data, values_at, self.incidences)
        start, count = sections['NLEVELS']
        if count != 1:
            raise Refus('%s : NLEVELS a %d elements' % (role, count))
        self.nlevels, = struct.unpack_from('<Q', data, start)

    def records(self):
        """(indice, rang, p, m, q, S*, I, U) dans l'ordre publie ; la structure de chaque boule est controlee au
        passage (Refus)."""
        role, n, values, view = self.role, self.n, self.values, memoryview(self.data)
        offsets = struct.iter_unpack('<Q', view[self.offsets_at + 8:self.offsets_at + 8 * (self.balls + 1)])
        balls = struct.iter_unpack('<8I', view[self.balls_at:self.balls_at + 32 * self.balls])
        previous = 0
        for index, (record, (offset,)) in enumerate(zip(balls, offsets)):
            rank, p, m, q = record[0], record[1], record[2], record[3]
            if q < 2 or q > 4:
                raise Refus('%s : boule %d, q = %d hors de 2..4' % (role, index, q))
            sstar = record[4:4 + q]
            if any(x != NONE for x in record[4 + q:]):
                raise Refus('%s : boule %d, S* sans bourrage 0xFFFFFFFF au-dela de q' % (role, index))
            if sstar[-1] >= n or any(sstar[i] >= sstar[i + 1] for i in range(q - 1)):
                raise Refus('%s : boule %d, S* non strictement croissant ou hors des sites' % (role, index))
            if offset < previous or offset - previous != p + m:
                raise Refus('%s : boule %d, decalages %d..%d pour p + m = %d' % (role, index, previous, offset, p + m))
            population = values[previous:offset].tolist()
            previous = offset
            inner, shell = population[:p], population[p:]
            for part in (inner, shell):
                if part and (part[-1] >= n or any(part[i] >= part[i + 1] for i in range(len(part) - 1))):
                    raise Refus('%s : boule %d, population non strictement croissante ou hors des sites'
                                % (role, index))
            yield index, rank, p, m, q, sstar, inner, shell


# ---------------------------------------------------------------- jugement d'un vidage

class Judge(object):
    """Juge d'un vidage : controles par boule et par niveau ; rend ses niveaux dans l'ordre publie."""

    def __init__(self, catalogue, ecarts):
        self.cat, self.ecarts = catalogue, ecarts
        self.stats = {'sites': catalogue.n, 'boules': catalogue.balls, 'incidences': catalogue.incidences,
                      'niveaux': catalogue.nlevels, 'bits': catalogue.bits, 'convention': catalogue.convention,
                      'par_q': {'2': 0, '3': 0, '4': 0}, 'coquilles_etendues': 0, 'coquille_max': 0,
                      'supports_multiples': 0, 'sstar_selon_convention': 0, 'niveaux_lus': 0}
        self.sorted_levels = True

    def _shell(self, where, sphere, q, sstar, shell):
        """Coquille etendue (sites tous sur la sphere, S* support minimal inclus dans U) : qmin, convention, comptes."""
        positions, convention, ecarts = self.cat.positions, self.cat.convention, self.ecarts
        for size in range(2, q):
            if first_support(sphere, shell, size, positions) is not None:
                ecarts.add('qmin', where, 'support de cardinal %d dans la coquille, q = %d' % (size, q))
                return
        by_index = first_support(sphere, shell, q, positions)
        by_position = first_support(sphere, sorted(shell, key=positions.__getitem__), q, positions)
        if by_index is None or by_position is None:
            raise Invariant('%s : S* support minimal dans U, mais aucun support de cardinal %d trouve' % (where, q))
        first = by_index if convention == 'v11' else by_position
        if set(first) != set(sstar):
            ecarts.add('convention', where, 'S* %s, premier support de la convention %s : %s'
                       % (list(sstar), convention, list(first)))
        if set(by_index) != set(by_position):
            self.stats['sstar_selon_convention'] += 1
            self.stats['supports_multiples'] += 1
            return
        for part in itertools.combinations(shell, q):
            if set(part) != set(by_index) and sphere.support([positions[i] for i in part]):
                self.stats['supports_multiples'] += 1
                return

    def levels(self):
        """Niveaux dans l'ordre publie : (niveau, [(indice, cle, rang, q, S* en positions, I, U)]) ; controles
        par vidage au passage."""
        cat, ecarts, stats = self.cat, self.ecarts, self.stats
        positions, role, kmax, convention = cat.positions, cat.role, cat.kmax, cat.convention
        group, level, dense, previous_key = [], None, 0, None
        for index, rank, p, m, q, sstar, inner, shell in cat.records():
            where = '%s:%d' % (role, index)
            stats['par_q'][str(q)] += 1
            points = [positions[i] for i in sstar]
            n, d, minimal = circumsphere(points)
            if n is None:
                ecarts.add('support_non_minimal', where, 'S* %s affinement dependant' % list(sstar))
                continue
            if not minimal:
                ecarts.add('support_non_minimal', where, 'S* %s : centre hors de l\'interieur relatif' % list(sstar))
            a = points[0]
            key, ball_level = ball_key(a, n, d)
            a0, a1, a2 = a
            n0, n1, n2 = n
            for i in inner:
                x = positions[i]
                e0, e1, e2 = x[0] - a0, x[1] - a1, x[2] - a2
                if d * (e0 * e0 + e1 * e1 + e2 * e2) - 2 * (e0 * n0 + e1 * n1 + e2 * n2) >= 0:
                    ecarts.add('interieur_non_strict', where, 'site %d de I hors de la boule ouverte' % i)
            on_sphere = True
            for i in shell:
                x = positions[i]
                e0, e1, e2 = x[0] - a0, x[1] - a1, x[2] - a2
                if d * (e0 * e0 + e1 * e1 + e2 * e2) - 2 * (e0 * n0 + e1 * n1 + e2 * n2) != 0:
                    on_sphere = False
                    ecarts.add('coquille_hors_sphere', where, 'site %d de U hors de la sphere' % i)
            in_shell = set(shell)
            if not all(i in in_shell for i in sstar):
                ecarts.add('support_hors_coquille', where, 'S* %s non inclus dans U' % list(sstar))
            elif m > q:
                stats['coquilles_etendues'] += 1
                stats['coquille_max'] = max(stats['coquille_max'], m)
                if minimal and on_sphere:
                    self._shell(where, Sphere(a, n, d), q, sstar, shell)
            if p + q > kmax + 1:
                ecarts.add('admission', where, 'p + q = %d > K + 1 = %d' % (p + q, kmax + 1))
            if level is None or compare_levels(ball_level, level) != 0:
                if level is not None and compare_levels(ball_level, level) < 0:
                    self.sorted_levels = False
                    ecarts.add('rangs', where, 'niveau inferieur a celui de la boule precedente')
                if group:
                    yield level, group
                group, level, previous_key = [], ball_level, None
                dense += 1
            if rank != dense:
                ecarts.add('rangs', where, 'rang publie %d, rang dense recalcule %d' % (rank, dense))
            spos = tuple(sorted(points))
            order_key = sstar + (NONE,) * (4 - q) if convention == 'v11' else spos + (PAD_V12,) * (4 - q)
            if previous_key is not None and order_key < previous_key:
                ecarts.add('ordre', where, 'S* avant celui de la boule precedente de meme niveau (convention %s)'
                           % convention)
            previous_key = order_key
            group.append((index, key, rank, q, spos, inner, shell))
        if group:
            yield level, group
        stats['niveaux_lus'] = dense
        if cat.nlevels != dense + 1:
            ecarts.add('niveaux', role, 'NLEVELS = %d, niveaux distincts + 1 = %d' % (cat.nlevels, dense + 1))


# ---------------------------------------------------------------- comparaison croisee

class Transition(object):
    """Comparaison de deux vidages, niveau par niveau."""

    def __init__(self, reference, candidate, ecarts):
        self.ref, self.cand, self.ecarts = reference, candidate, ecarts
        self.same_sites = reference.positions == candidate.positions
        self.to_ref = None
        if not self.same_sites:
            index = {position: i for i, position in enumerate(reference.positions)}
            self.to_ref = [index.get(position, -1) for position in candidate.positions]
        self.stats = {'appariees': 0, 'sstar_differents': 0, 'renumerotees': 0,
                      'ordre_des_sites_identique': self.same_sites}

    def sites(self):
        left, right = set(self.ref.positions), set(self.cand.positions)
        if left != right:
            self.ecarts.add('sites', 'croise', '%d sites de reference absents du candidat, %d sites en trop'
                            % (len(left - right), len(right - left)))

    def _population(self, values):
        """Population candidate en indices de la reference (ensemble de positions, trie)."""
        if self.same_sites:
            return values
        return sorted(self.to_ref[i] for i in values)

    def _pair(self, r, c):
        ecarts, stats = self.ecarts, self.stats
        where = 'reference:%d/candidat:%d' % (r[0], c[0])
        stats['appariees'] += 1
        if r[0] != c[0]:
            stats['renumerotees'] += 1
        if r[5] != self._population(c[5]):
            ecarts.add('interieur', where, 'I differents (p = %d contre %d)' % (len(r[5]), len(c[5])))
        if r[6] != self._population(c[6]):
            ecarts.add('coquille', where, 'U differents (m = %d contre %d)' % (len(r[6]), len(c[6])))
        if r[2] != c[2]:
            ecarts.add('rangs_croises', where, 'rangs %d contre %d' % (r[2], c[2]))
        if r[4] != c[4]:
            stats['sstar_differents'] += 1
            if r[3] != c[3]:
                ecarts.add('cardinal_support', where, 'S* de cardinaux %d contre %d' % (r[3], c[3]))

    def _level(self, ref_group, cand_group):
        ecarts = self.ecarts
        by_key = {}
        for c in cand_group:
            by_key.setdefault(c[1], []).append(c)
        seen = {}
        for r in ref_group:
            if r[1] in seen:
                ecarts.add('doublon', 'reference:%d' % r[0], 'meme boule que reference:%d' % seen[r[1]])
                continue
            seen[r[1]] = r[0]
            matches = by_key.pop(r[1], None)
            if matches is None:
                ecarts.add('absente', 'reference:%d' % r[0], 'boule de reference sans image dans le candidat')
                continue
            for extra in matches[1:]:
                ecarts.add('doublon', 'candidat:%d' % extra[0], 'meme boule que candidat:%d' % matches[0][0])
            self._pair(r, matches[0])
        for matches in by_key.values():
            for c in matches:
                ecarts.add('en_trop', 'candidat:%d' % c[0], 'boule candidate sans antecedent dans la reference')

    def run(self, ref_judge, cand_judge):
        ecarts = self.ecarts
        ref_levels, cand_levels = ref_judge.levels(), cand_judge.levels()
        r, c = next(ref_levels, None), next(cand_levels, None)
        while r is not None or c is not None:
            if not (ref_judge.sorted_levels and cand_judge.sorted_levels):
                ecarts.add('rangs', 'croise', 'niveaux non croissants : comparaison croisee interrompue')
                break
            side = 0 if r is None or c is None else compare_levels(r[0], c[0])
            if c is None or (r is not None and side < 0):
                for ball in r[1]:
                    ecarts.add('absente', 'reference:%d' % ball[0], 'niveau absent du candidat')
                r = next(ref_levels, None)
            elif r is None or side > 0:
                for ball in c[1]:
                    ecarts.add('en_trop', 'candidat:%d' % ball[0], 'niveau absent de la reference')
                c = next(cand_levels, None)
            else:
                self._level(r[1], c[1])
                r, c = next(ref_levels, None), next(cand_levels, None)
        for rest in (ref_levels, cand_levels):  # controles par vidage jusqu'au bout, meme apres une interruption
            for _ in rest:
                pass


# ---------------------------------------------------------------- point d'entree

def judge_files(reference_path, candidate_path, convention_reference='v11', convention_candidate='v12',
                keep=ECARTS_DEFAUT):
    """Juge deux vidages ; rend (code, rapport). Leve Refus (entree) ou Invariant (lecteur)."""
    reference = Catalogue(reference_path, 'reference', convention_reference)
    candidate = Catalogue(candidate_path, 'candidat', convention_candidate)
    if reference.kmax != candidate.kmax:
        raise Refus('K differents : %d contre %d' % (reference.kmax, candidate.kmax))
    if reference.frame != candidate.frame:
        raise Refus('trames differentes : %r contre %r' % (reference.frame, candidate.frame))
    ecarts = Ecarts(keep)
    transition = Transition(reference, candidate, ecarts)
    transition.sites()
    ref_judge, cand_judge = Judge(reference, ecarts), Judge(candidate, ecarts)
    transition.run(ref_judge, cand_judge)
    code = CONFORME if ecarts.total() == 0 else DESACCORD
    report = {'schema': SCHEMA, 'code': code, 'K': reference.kmax, 'trame': reference.frame,
              'reference': ref_judge.stats, 'candidat': cand_judge.stats, 'croise': transition.stats,
              'ecarts': dict(sorted(ecarts.counts.items())), 'premiers_ecarts': ecarts.details}
    return code, report


def summary(code, report):
    """Lignes de sortie : verdict, puis les premiers ecarts."""
    ref, cross = report['reference'], report['croise']
    if code == CONFORME:
        return ['transition_catalogue_conforme boules=%d incidences=%d niveaux=%d coquilles_etendues=%d '
                'supports_multiples=%d sstar_selon_convention=%d sstar_differents=%d renumerotees=%d'
                % (cross['appariees'], ref['incidences'], ref['niveaux'], ref['coquilles_etendues'],
                   ref['supports_multiples'], ref['sstar_selon_convention'], cross['sstar_differents'],
                   cross['renumerotees'])]
    counts = report['ecarts']
    lines = ['transition_catalogue_desaccord ecarts=%d categories=%s'
             % (sum(counts.values()), ','.join('%s:%d' % item for item in sorted(counts.items())))]
    for item in report['premiers_ecarts']:
        lines.append('ecart %s %s : %s' % (item['categorie'], item['ou'], item['detail']))
    return lines


def main(argv):
    parser = argparse.ArgumentParser(prog='transition_catalogue.py',
                                     description='Lecteur de transition du catalogue (MHGP12DP, genre 1).')
    parser.add_argument('reference')
    parser.add_argument('candidat')
    parser.add_argument('--convention-reference', choices=CONVENTIONS, default='v11')
    parser.add_argument('--convention-candidat', choices=CONVENTIONS, default='v12')
    parser.add_argument('--rapport')
    parser.add_argument('--ecarts', type=int, default=ECARTS_DEFAUT)
    try:
        args = parser.parse_args(argv[1:])
    except SystemExit as stop:
        return REFUS if stop.code else CONFORME
    if args.ecarts < 0:
        print('transition_catalogue_refus : --ecarts negatif', file=sys.stderr)
        return REFUS
    started = time.time()
    try:
        code, report = judge_files(args.reference, args.candidat, args.convention_reference,
                                   args.convention_candidat, args.ecarts)
    except Refus as refusal:
        print('transition_catalogue_refus : %s' % refusal, file=sys.stderr)
        return REFUS
    except Invariant as violation:
        print('transition_catalogue_invariant : %s' % violation, file=sys.stderr)
        return INVARIANT
    except Exception as error:  # jamais un code 1 par accident : une exception inattendue rend 3
        print('transition_catalogue_invariant : exception %s : %s' % (type(error).__name__, error), file=sys.stderr)
        return INVARIANT
    for line in summary(code, report):
        print(line)
    print('transition_catalogue_temps secondes=%.1f' % (time.time() - started), file=sys.stderr)
    if args.rapport:
        try:
            with open(args.rapport, 'w', encoding='utf-8') as out:
                json.dump(report, out, indent=1, sort_keys=True)
                out.write('\n')
        except OSError as error:
            print('transition_catalogue_refus : rapport %s (%s)' % (args.rapport, error), file=sys.stderr)
            return REFUS
    return code


if __name__ == '__main__':
    sys.exit(main(sys.argv))
