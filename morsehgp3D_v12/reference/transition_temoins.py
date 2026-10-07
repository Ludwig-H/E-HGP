#!/usr/bin/env python3
"""Generateur des temoins du lecteur de transition du catalogue (transition_catalogue.py) : vidages MHGP12DP de genre
catalogue (version 1, microbancs/mes_m3_m4_tour/common/format.hpp) ecrits depuis le catalogue de l'oracle borne
(hgp12_ref, etage B : Cat_K par force brute, S* de la v11 = premier support en rangs de Morton), dans la convention
v11 ou v12, avec des alterations declarees. Temoins, cas et attendus graves : fixtures/transition_catalogue.json ;
juge : test_transition_catalogue.py.

Le generateur ne partage aucun code avec le lecteur : sites dans l'ordre de Morton de l'oracle (intgeom.morton, x au
bit 0) ; S* de chaque convention = premier support de cardinal q_min de la coquille dans l'ordre de la convention
(v11 : suites croissantes de SiteIdx ; v12 : listes triees de positions), decide par les predicats de l'oracle
(sphere circonscrite et sa cle exacte, triangle aigu, position dans le tetraedre). En convention v11 et dans l'ordre
de Morton, ce S* doit egaler celui de l'oracle (sinon InvariantError). Ordre publie : (niveau, S* dans la convention,
bourre par un element plus grand que tout) ; rangs denses dans l'ordre publie ; NLEVELS = niveaux distincts + 1.

Alterations (listes JSON, appliquees dans l'ordre) :
  sur les sites        ["sites_inverses"] ordre des sites renverse ; ["site_en_trop", [x, y, z]] site ajoute en fin,
                       dans aucune boule
  sur les boules       ["retirer", b] ; ["dupliquer", b] ; ["ajouter", K2, b] (boule b du catalogue de l'ordre K2) ;
                       ["interieur_retirer" | "interieur_ajouter" | "coquille_retirer" | "coquille_ajouter", b, site] ;
                       ["support", b, [sites]] (S* impose, q = son cardinal)
  sur l'ordre publie   ["ordre", convention] (ordre publie de cette convention) ; ["niveaux_inverses"] (niveaux
                       decroissants, rangs denses dans l'ordre publie) ; ["rang", b, r] ; ["niveaux", N]
  sur les octets       ["magie"] ; ["genre", g] ; ["K", k] ; ["trame", nom] ; ["tronquer", octets] ;
                       ["octets_en_trop"] ; ["section_absente", etiquette] ; ["section_inattendue"] ;
                       ["sstar_non_croissant", b] ; ["index_hors_domaine", b] ; ["positions_en_double", s1, s2] ;
                       ["decalage", b] (population de b privee de sa derniere valeur, decalages coherents avec
                       POPVAL) ; ["q_hors_domaine", b] ; ["bourrage", b] ; ["coordonnee_hors_domaine"]
Une boule est nommee par les etiquettes de son S* de la v11, triees (une sphere a un seul S* par convention).

Python 3.10 nu, aucun assert.
Usage : transition_temoins.py --ecrire DOSSIER [--cas NOM]   ecrit DOSSIER/<cas>/reference.bin et candidat.bin
        transition_temoins.py --catalogue TEMOIN             catalogue du temoin dans les deux conventions (JSON)
"""
import json
import os
import struct
import sys
from itertools import combinations

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
from hgp12_ref import intgeom as G  # noqa: E402
from hgp12_ref.constructive import Reference  # noqa: E402
from hgp12_ref.model import InvariantError  # noqa: E402

FIXTURE = os.path.join(HERE, 'fixtures', 'transition_catalogue.json')
NONE = 0xFFFFFFFF
BITS = G.MORTON_BITS
FRAME = 'temoin'
SITE_LEVEL = ('sites_inverses', 'site_en_trop')
BALL_LEVEL = ('retirer', 'dupliquer', 'ajouter', 'interieur_retirer', 'interieur_ajouter', 'coquille_retirer',
              'coquille_ajouter', 'support')
ORDER_LEVEL = ('ordre', 'niveaux_inverses', 'rang', 'niveaux')
BYTE_LEVEL = ('magie', 'genre', 'K', 'trame', 'tronquer', 'octets_en_trop', 'section_absente', 'section_inattendue',
              'sstar_non_croissant', 'index_hors_domaine', 'positions_en_double', 'decalage', 'q_hors_domaine',
              'bourrage', 'coordonnee_hors_domaine')


class Ball(object):
    """Boule du catalogue : nom, niveau et centre exacts (Fraction), q_min, S* de la v11 et populations, en
    etiquettes de sites ; forced : S* impose par une alteration."""
    __slots__ = ('name', 'level', 'center', 'q', 'v11', 'inner', 'shell', 'base_shell', 'forced')

    def copy(self):
        other = Ball()
        for slot in Ball.__slots__:
            setattr(other, slot, getattr(self, slot))
        return other


def load_fixture(path=FIXTURE):
    with open(path, encoding='utf-8') as handle:
        fixture = json.load(handle)
    if fixture.get('kind') != 'mhgp12_transition_witnesses' or fixture.get('schema_version') != 1:
        raise ValueError('fixture inattendue : %s' % path)
    return fixture


def points_of(witness):
    return {label: tuple(xyz) for label, xyz in witness['points'].items()}


def catalogue(witness, k=None):
    """Cat_K du temoin par l'oracle (etage B), boules de q_min >= 2 dans l'ordre de l'oracle."""
    points = points_of(witness)
    labels = sorted(points)
    ref = Reference([points[label] for label in labels], witness['K'] if k is None else k)
    label_of = {points[label]: label for label in labels}
    balls = []
    for b in ref.balls:
        if b.qmin < 2:
            continue
        ball = Ball()
        ball.level, ball.center, ball.q = b.level, b.center, b.qmin
        ball.v11 = tuple(sorted(label_of[ref.sites[s]] for s in b.support))
        ball.name = ''.join(ball.v11)
        ball.inner = tuple(sorted(label_of[ref.sites[s]] for s in b.inner_sites))
        ball.shell = tuple(sorted(label_of[ref.sites[s]] for s in b.shell_sites))
        ball.base_shell, ball.forced = ball.shell, None
        if ball.v11 != canonical_support(ball, points, lambda label: G.morton(points[label])):
            raise InvariantError('boule %s : S* de la v11 du generateur different de celui de l\'oracle' % ball.name)
        balls.append(ball)
    return balls


def is_support(ball, positions):
    """Les positions (sur la sphere de la boule) en sont un support minimal : predicats de l'oracle."""
    sphere = G.through(positions)
    if sphere is None or G.sphere_key(positions[0], sphere) != (ball.center, ball.level):
        return False
    if len(positions) == 3:
        return G.acute(positions[0], positions[1], positions[2])
    if len(positions) == 4:
        return G.tetra_position(positions, positions[0], sphere) == 1
    return True


def canonical_support(ball, points, order):
    """Premier support de cardinal q de la coquille d'origine (avant alteration) dans l'ordre lexicographique des
    suites de cles order."""
    shell = sorted(ball.base_shell, key=order)
    for part in combinations(shell, ball.q):
        if is_support(ball, [points[label] for label in part]):
            return tuple(sorted(part))
    raise InvariantError('boule %s : aucun support de cardinal %d dans sa coquille' % (ball.name, ball.q))


def morton_order(points):
    return sorted(points, key=lambda label: G.morton(points[label]))


def _find(balls, name):
    for ball in balls:
        if ball.name == name:
            return ball
    raise ValueError('boule %s absente du temoin' % name)


def _alter_balls(witness, balls, alteration):
    op, args = alteration[0], alteration[1:]
    if op == 'ajouter':
        balls.append(_find(catalogue(witness, args[0]), args[1]).copy())
        return
    ball = _find(balls, args[0])
    if op == 'retirer':
        balls.remove(ball)
    elif op == 'dupliquer':
        balls.insert(balls.index(ball) + 1, ball.copy())
    elif op in ('interieur_retirer', 'coquille_retirer'):
        field = 'inner' if op == 'interieur_retirer' else 'shell'
        values = list(getattr(ball, field))
        values.remove(args[1])
        setattr(ball, field, tuple(values))
    elif op in ('interieur_ajouter', 'coquille_ajouter'):
        field = 'inner' if op == 'interieur_ajouter' else 'shell'
        setattr(ball, field, tuple(sorted(getattr(ball, field) + (args[1],))))
    elif op == 'support':
        ball.forced = tuple(sorted(args[1]))
    else:
        raise ValueError('alteration inconnue %r' % (op,))


def build(witness, convention, alterations=()):
    """Octets du vidage du temoin dans la convention, alterations comprises."""
    for alteration in alterations:
        if alteration[0] not in SITE_LEVEL + BALL_LEVEL + ORDER_LEVEL + BYTE_LEVEL:
            raise ValueError('alteration inconnue %r' % (alteration[0],))
    points = points_of(witness)
    sites = morton_order(points)
    balls = [ball.copy() for ball in catalogue(witness)]
    for alteration in alterations:
        if alteration[0] == 'sites_inverses':
            sites.reverse()
        elif alteration[0] == 'site_en_trop':
            points['#'] = tuple(alteration[1])
            sites.append('#')
        elif alteration[0] in BALL_LEVEL:
            _alter_balls(witness, balls, alteration)
    index = {label: i for i, label in enumerate(sites)}
    keys = {'v11': lambda label: index[label], 'v12': lambda label: points[label]}
    rows = []
    for ball in balls:
        sstar = ball.forced if ball.forced is not None else canonical_support(ball, points, keys[convention])
        rows.append({'ball': ball, 'sstar': sorted(index[label] for label in sstar),
                     'inner': sorted(index[label] for label in ball.inner),
                     'shell': sorted(index[label] for label in ball.shell)})
    order = convention
    descending = False
    for alteration in alterations:
        if alteration[0] == 'ordre':
            order = alteration[1]
        elif alteration[0] == 'niveaux_inverses':
            descending = True

    def order_key(row):
        if order == 'v11':
            tail = tuple(row['sstar']) + (NONE,) * (4 - len(row['sstar']))
        else:
            tail = tuple(sorted(points[sites[i]] for i in row['sstar']))
            tail += ((1 << 40, 0, 0),) * (4 - len(row['sstar']))
        return tail

    rows.sort(key=order_key)
    rows.sort(key=lambda row: row['ball'].level, reverse=descending)
    dense, previous = 0, None
    for row in rows:
        if previous is None or row['ball'].level != previous:
            dense += 1
            previous = row['ball'].level
        row['rank'] = dense
    nlevels = dense + 1
    for alteration in alterations:
        if alteration[0] == 'rang':
            for row in rows:
                if row['ball'].name == alteration[1]:
                    row['rank'] = alteration[2]
        elif alteration[0] == 'niveaux':
            nlevels = alteration[1]
    return serialize(witness, [points[label] for label in sites], rows, nlevels,
                     [a for a in alterations if a[0] in BYTE_LEVEL])


def _section(tag, elem, data, count):
    return [tag, elem, data, count]


def serialize(witness, positions, rows, nlevels, alterations):
    """Format MHGP12DP version 1, genre catalogue ; alterations d'octets declarees."""
    edits = {}
    for alteration in alterations:
        edits.setdefault(alteration[0], []).append(alteration[1:])
    names = [row['ball'].name for row in rows]

    def target(op):
        return [names.index(args[0]) for args in edits.get(op, [])]

    positions = list(positions)
    points = points_of(witness)
    for first, second in edits.get('positions_en_double', []):
        positions[positions.index(points[second])] = points[first]
    if 'coordonnee_hors_domaine' in edits:
        positions[0] = (1 << BITS, positions[0][1], positions[0][2])
    sites = b''.join(struct.pack('<3I', *p) for p in positions)
    balls, offsets, values = [], [0], []
    for i, row in enumerate(rows):
        sstar = list(row['sstar'])
        q = len(sstar)
        if i in target('sstar_non_croissant'):
            sstar.reverse()
        padded = sstar + [NONE] * (4 - q)
        if i in target('bourrage'):
            padded[q] = 0
        if i in target('q_hors_domaine'):
            q = 5
        shell = list(row['shell'])
        if i in target('index_hors_domaine'):
            shell[-1] = len(positions)
        population = list(row['inner']) + shell
        balls.append(struct.pack('<8I', row['rank'], len(row['inner']), len(shell), q, *padded))
        if i in target('decalage'):
            population.pop()  # decalages coherents avec POPVAL, mais une valeur de moins que p + m pour cette boule
        values.extend(population)
        offsets.append(offsets[-1] + len(population))
    sections = [_section('SITEXYZ', 12, sites, len(positions)),
                _section('BALLS', 32, b''.join(balls), len(rows)),
                _section('POPOFF', 8, b''.join(struct.pack('<Q', x) for x in offsets), len(offsets)),
                _section('POPVAL', 4, b''.join(struct.pack('<I', x) for x in values), len(values)),
                _section('NLEVELS', 8, struct.pack('<Q', nlevels), 1)]
    for (tag,) in edits.get('section_absente', []):
        sections = [s for s in sections if s[0] != tag]
    if 'section_inattendue' in edits:
        sections.append(_section('EXTRA', 4, struct.pack('<I', 0), 1))
    magic = b'MHGP12DQ' if 'magie' in edits else b'MHGP12DP'
    kind = edits['genre'][0][0] if 'genre' in edits else 1
    kmax = edits['K'][0][0] if 'K' in edits else witness['K']
    frame = (edits['trame'][0][0] if 'trame' in edits else FRAME).encode('ascii')
    out = bytearray(struct.pack('<8s6IQ24s', magic, 1, kind, BITS, kmax, 0, len(sections), len(positions), frame))
    for tag, elem, data, count in sections:
        out += struct.pack('<8sIIQ', tag.encode('ascii'), elem, 0, count) + data + b'\0' * (-len(data) % 8)
    if 'octets_en_trop' in edits:
        out += b'\0' * 8
    for (size,) in edits.get('tronquer', []):
        out = out[:len(out) - size]
    return bytes(out)


def case_files(fixture, case, directory):
    """Ecrit reference.bin et candidat.bin du cas dans directory ; rend leurs chemins."""
    witness = fixture['witnesses'][case['witness']]
    paths = []
    for side, convention in (('reference', case.get('convention_reference', 'v11')),
                             ('candidat', case.get('convention_candidat', 'v12'))):
        path = os.path.join(directory, side + '.bin')
        with open(path, 'wb') as out:
            out.write(build(witness, convention, case.get(side, [])))
        paths.append(path)
    return paths


def describe(witness):
    """Catalogue du temoin dans les deux conventions : ordre publie (noms), S* par convention, niveaux."""
    points = points_of(witness)
    balls = catalogue(witness)
    sites = morton_order(points)
    index = {label: i for i, label in enumerate(sites)}
    out = {'sites_morton': sites, 'boules': len(balls), 'niveaux': len({b.level for b in balls}) + 1, 'ordre': {},
           'sstar': {}, 'ordre_oracle': [b.name for b in balls]}
    for convention, key in (('v11', lambda label: index[label]), ('v12', lambda label: points[label])):
        support = {b.name: canonical_support(b, points, key) for b in balls}
        if convention == 'v11':
            pad = lambda s: tuple(sorted(index[x] for x in s)) + (NONE,) * (4 - len(s))  # noqa: E731
        else:
            pad = lambda s: tuple(sorted(points[x] for x in s)) + ((1 << 40, 0, 0),) * (4 - len(s))  # noqa: E731
        ordered = sorted(balls, key=lambda b: (b.level, pad(support[b.name])))
        out['ordre'][convention] = [b.name for b in ordered]
        out['sstar'][convention] = {b.name: ''.join(support[b.name]) for b in balls}
    out['boules_detail'] = {b.name: {'niveau': str(b.level), 'centre': [str(c) for c in b.center], 'q': b.q,
                                     'interieur': ''.join(b.inner), 'coquille': ''.join(b.shell)} for b in balls}
    return out


def main(argv):
    fixture = load_fixture()
    if len(argv) >= 3 and argv[1] == '--catalogue':
        print(json.dumps(describe(fixture['witnesses'][argv[2]]), indent=1, sort_keys=True))
        return 0
    if len(argv) in (3, 5) and argv[1] == '--ecrire' and (len(argv) == 3 or argv[3] == '--cas'):
        wanted = argv[4] if len(argv) == 5 else None
        for case in fixture['cases']:
            if wanted is None or case['name'] == wanted:
                directory = os.path.join(argv[2], case['name'])
                os.makedirs(directory, exist_ok=True)
                case_files(fixture, case, directory)
        return 0
    print('usage : transition_temoins.py --ecrire DOSSIER [--cas NOM] | --catalogue TEMOIN', file=sys.stderr)
    return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv))
