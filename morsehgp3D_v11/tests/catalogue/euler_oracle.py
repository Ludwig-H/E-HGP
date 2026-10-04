#!/usr/bin/env python3
"""Juge d'Euler a K+2 et restriction J1 contre un juge Fraction independant, sur petits nuages (n <= 14).

    python3 euler_oracle.py <mhgp11_catalogue_euler> <bits du profil> <dossier de travail>

Bibliotheque standard seulement (Python 3.10 nu), aucun assert. Pour chaque nuage et chaque K, la sonde construit
Cat_K et Cat_{K+2} par la voie sequentielle de reference (et, pour une partie des cas, par la voie production a deux
fils, dont la sortie doit etre identique). Le juge :
  - enumere toutes les boules critiques en Fraction (fraction_model.all_balls, sans code du produit) ;
  - calcule e_k(b) par ses propres supports minimaux (Gram/Fraction : meme centre, poids strictement positifs) et la
    fermeture des parties de la coquille ; pour m <= 8, il recoupe cette fermeture par le test d'enveloppe faible de
    chaque partie (poids positifs ou nuls), autre formulation de c dans conv(A) ;
  - verifie l'identite J3 elle-meme sur TOUTES les boules critiques, aux ordres 1..n (role d'oracle borne) ;
  - compare a la sonde : euler_by_k complet (ordres verifiables et non verifiables), parts reguliere et etendue,
    nombres de boules de Cat_K et Cat_{K+2}, coquilles etendues par taille, qmin, supports minimaux, sites recenses,
    compteurs de la restriction J1 (niveaux bruts recalcules compris), ligne de verdict et code.
Ligne finale : euler_oracle_ok controles=N ; planchers : cas, coquilles etendues jugees, ordres non verifiables vus.
"""
import json
import os
import shutil
import struct
import sys
from fractions import Fraction as F
from itertools import combinations
from math import comb

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'support'))
import mhgp11_gate  # noqa: E402
from fixtures import fixtures, records  # noqa: E402
from fraction_model import all_balls, circumsphere, prepared  # noqa: E402

ORDERS = (1, 2, 3, 5, 8, 10)
PRODUCTION_ORDERS = (3, 10)
# Planchers sous les valeurs mesurees le 4 octobre 2026 (258 cas, 1721 coquilles etendues, 555 ordres non
# verifiables differents de 1, 5720 parties recoupees, 7270 controles) : une couverture qui fond fait echouer.
MIN_CASES = 250
MIN_EXTENDED = 1500
MIN_UNCHECKABLE = 500
MIN_WEAK = 5000
MIN_CHECKS = 7000


def lcg_clouds():
    """Nuages a petite etendue (coordonnees 0..3) : cospheres et cocycliques frequents, coquilles etendues."""
    state = 20261004
    out = []
    for number in range(10):
        points = set()
        while len(points) < 7 + number % 6:
            point = []
            for _axis in range(3):
                state = (1103515245 * state + 12345) & 0x7fffffff
                point.append((state >> 16) % 4)
            points.add(tuple(point))
        out.append(('petite_grille%d' % number, tuple(sorted(points))))
    return out


def extra_clouds():
    sphere = [(5, 0, 0), (-5, 0, 0), (0, 5, 0), (0, -5, 0), (0, 0, 5), (0, 0, -5),
              (3, 4, 0), (-3, -4, 0), (0, 3, 4), (0, -3, -4), (4, 0, 3), (-4, 0, -3)]
    circle = [(5, 0), (-5, 0), (0, 5), (0, -5), (3, 4), (-3, 4), (3, -4), (-3, -4), (4, 3), (-4, 3), (4, -3), (-4, -3)]
    return [
        ('compensation5', ((0, 5, 0), (8, 9, 0), (8, 1, 0), (35, 5, 0), (45, 5, 0))),
        ('dt13', ((0, 0, 0), (20, 0, 0), (8, 1, 1), (9, 2, 2), (11, 1, 3), (12, 3, 1), (100, 0, 0), (120, 0, 0),
                  (110, 16, 0), (108, 4, 1), (110, 4, 2), (112, 5, 1), (109, 6, 2))),
        ('sphere12', tuple((x + 20, y + 20, z + 20) for x, y, z in sphere)),
        ('cercle12', tuple((x + 10, y + 10, 3) for x, y in circle)),
        ('sphere12_centre', tuple((x + 20, y + 20, z + 20) for x, y, z in sphere[:10]) + ((20, 20, 20), (21, 20, 20))),
    ]


def shell_counts(ball, points):
    """(counts[s] pour s = 0..m, supports minimaux, controles d'enveloppe faible) d'une boule du modele Fraction."""
    shell = [points[i] for i in ball.shell]
    m = len(shell)
    supports = []
    for size in (2, 3, 4):
        for chosen in combinations(range(m), size):
            sphere = circumsphere([shell[i] for i in chosen])
            if sphere is not None and sphere[0] == ball.center and all(w > 0 for w in sphere[2]):
                supports.append(sum(1 << i for i in chosen))
    inside = bytearray(1 << m)
    for mask in supports:
        inside[mask] = 1
    for i in range(m):
        bit = 1 << i
        for mask in range(1 << m):
            if mask & bit and inside[mask ^ bit]:
                inside[mask] = 1
    weak = 0
    if 2 < m <= 8 and len(ball.support) < m:
        for mask in range(1 << m):
            members = [shell[i] for i in range(m) if mask >> i & 1]
            found = False
            for size in range(2, min(4, len(members)) + 1):
                for chosen in combinations(members, size):
                    sphere = circumsphere(list(chosen))
                    if sphere is not None and sphere[0] == ball.center and all(w >= 0 for w in sphere[2]):
                        found = True
                        break
                if found:
                    break
            if found != bool(inside[mask]):
                raise ValueError('enveloppe faible et fermeture des supports divergent : %r masque %d' % (shell, mask))
            weak += 1
    counts = [0] * (m + 1)
    for mask in range(1 << m):
        if inside[mask]:
            counts[bin(mask).count('1')] += 1
    return counts, len(supports), weak


def contribution(ball, counts, k):
    p, m = len(ball.inner), len(ball.shell)
    t = k - p
    if t < 1 or t > m:
        return 0
    return sum((-1) ** (s - t) * comb(s - 1, t - 1) * counts[s] for s in range(t, m + 1))


def fnv1a(blobs):
    value = 0xcbf29ce484222325
    for blob in blobs:
        for byte in blob:
            value = ((value ^ byte) * 0x100000001b3) & 0xFFFFFFFFFFFFFFFF
    return '%016x' % value


class Cloud:
    """Verite Fraction d'un nuage : boules critiques, comptes par cardinal, identite J3 aux ordres 1..n."""

    def __init__(self, gate, name, recs):
        self.name = name
        self.records = recs
        self.points, _ids = prepared(recs)
        self.n = len(self.points)
        self.balls = all_balls(self.points) if self.n > 1 else ()
        self.counts, self.supports, self.weak = {}, {}, 0
        for ball in self.balls:
            counts, supports, weak = shell_counts(ball, self.points)
            self.counts[ball] = counts
            self.supports[ball] = supports
            self.weak += weak
        for k in range(1, self.n + 1):
            total = (self.n if k == 1 else 0) + sum(contribution(b, self.counts[b], k) for b in self.balls)
            gate.check_eq(total, 1, '%s : identite J3 sur toutes les boules critiques, ordre %d' % (name, k))
        xyz = b''.join(struct.pack('<III', x, y, z) for x, y, z, _i in recs)
        ids = b''.join(struct.pack('<I', i) for _x, _y, _z, i in recs)
        self.blobs = (xyz, ids)
        self.fnv = fnv1a(self.blobs)

    def expected(self, kmax):
        large = [b for b in self.balls if len(b.inner) + len(b.support) <= kmax + 3]
        small = [b for b in self.balls if len(b.inner) + len(b.support) <= kmax + 1]
        orders = kmax + 2
        regular = [sum(contribution(b, self.counts[b], k) for b in large if len(b.shell) == len(b.support))
                   for k in range(1, orders + 1)]
        extended = [sum(contribution(b, self.counts[b], k) for b in large if len(b.shell) > len(b.support))
                    for k in range(1, orders + 1)]
        totals = [r + e + (self.n if k == 0 else 0) for k, (r, e) in enumerate(zip(regular, extended))]
        wide = [b for b in large if len(b.shell) > len(b.support)]
        by_shell = {}
        for b in wide:
            by_shell[str(len(b.shell))] = by_shell.get(str(len(b.shell)), 0) + 1
        small_set = set(small)
        levels = []
        recomputed = 0
        for b in small:
            if not levels or levels[-1] != b.level:
                levels.append(b.level)
                first = next(c for c in large if c.level == b.level)
                recomputed += 0 if first in small_set else 1
        checkable = min(kmax, self.n)
        return {
            'euler': {'orders': orders, 'checkable': checkable, 'euler_by_k': totals, 'regular_by_k': regular,
                      'extended_by_k': extended, 'failing_orders': [], 'balls': len(large), 'omitted': 0,
                      'regular': len(large) - len(wide), 'extended': len(wide),
                      'max_shell': max((len(b.shell) for b in large), default=0), 'extended_by_shell': by_shell,
                      'by_qmin': [sum(1 for b in large if len(b.support) == q) for q in (2, 3, 4)],
                      'supports': sum(self.supports[b] for b in wide),
                      'census_sites': sum(len(b.inner) + len(b.shell) for b in large), 'faults': 0},
            'restriction': {'k': kmax, 'filtered': len(small), 'compared': len(small), 'missing': 0, 'extra': 0,
                            'fields': 0, 'raw_checks': len(levels), 'raw_recomputed': recomputed},
            'verdict': 'catalogue_euler_verdict conforme k=%d n=%d ordres=%d boules_k=%d boules_k2=%d etendues=%d '
                       'coquille_max=%d entree=%s' % (kmax, self.n, checkable, len(small), len(large), len(wide),
                                                      max((len(b.shell) for b in large), default=0), self.fnv),
            'uncheckable': sum(1 for k in range(checkable + 1, orders + 1) if totals[k - 1] != 1),
        }


def run_probe(probe, folder, cloud, kmax, extra):
    xyz, ids = (os.path.join(folder, cloud.name + suffix) for suffix in ('.u32le', '.ids.u32le'))
    with open(xyz, 'wb') as handle:
        handle.write(cloud.blobs[0])
    with open(ids, 'wb') as handle:
        handle.write(cloud.blobs[1])
    done = mhgp11_gate.run([probe, '--input=%s,%s' % (xyz, ids), '--k=%d' % kmax] + extra, timeout=120)
    phases, verdicts = {}, []
    for line in (done.stdout or '').splitlines():
        if line.startswith('{'):
            record = json.loads(line)
            phases.setdefault(record.get('phase'), []).append(record)
        elif line.startswith('catalogue_euler_verdict'):
            verdicts.append(line)
    return done, phases, verdicts


def stable(phases):
    """Sortie canonique d'une execution : phases euler et restriction sans durees."""
    out = {}
    for name in ('euler', 'restriction'):
        for record in phases.get(name, []):
            out[name] = {key: value for key, value in record.items() if key != 'wall_ns'}
    return out


def judge_case(gate, probe, folder, cloud, kmax):
    want = cloud.expected(kmax)
    what = '%s/K%d' % (cloud.name, kmax)
    done, phases, verdicts = run_probe(probe, folder, cloud, kmax, [])
    gate.check_eq(done.code, 0, what + ' : code de la sonde')
    gate.check_eq(verdicts, [want['verdict']], what + ' : ligne de verdict')
    got = stable(phases)
    for phase in ('euler', 'restriction'):
        record = got.get(phase, {})
        for key, value in want[phase].items():
            gate.check_eq(record.get(key), value, '%s : %s.%s' % (what, phase, key))
    if kmax in PRODUCTION_ORDERS:
        done2, phases2, verdicts2 = run_probe(probe, folder, cloud, kmax, ['--production', '--workers=2'])
        gate.check_eq(done2.code, 0, what + ' : code de la voie production')
        gate.check_eq(verdicts2, verdicts, what + ' : verdict identique en voie production')
        gate.check_eq(stable(phases2), got, what + ' : euler et restriction identiques en voie production')
    return want


def main():
    if len(sys.argv) != 4:
        print('usage : euler_oracle.py <sonde> <bits> <dossier de travail>')
        return mhgp11_gate.REFUSAL
    probe, bits, work = sys.argv[1], int(sys.argv[2]), sys.argv[3]
    gate = mhgp11_gate.Gate('euler_oracle')
    folder = os.path.join(work, 'euler_oracle_%d' % os.getpid())
    os.makedirs(folder)
    clouds = [(f.name, records(f.points)) for f in fixtures(bits)]
    clouds += [(name, records(points)) for name, points in lcg_clouds() + extra_clouds()]
    cases = extended = uncheckable = weak = 0
    try:
        for name, recs in clouds:
            cloud = Cloud(gate, name, recs)
            weak += cloud.weak
            for kmax in ORDERS:
                want = judge_case(gate, probe, folder, cloud, kmax)
                cases += 1
                extended += want['euler']['extended']
                uncheckable += want['uncheckable']
    finally:
        shutil.rmtree(folder, ignore_errors=True)
    print('euler_oracle couverture : nuages=%d cas=%d coquilles_etendues=%d ordres_non_verifiables_differents_de_1=%d '
          'parties_enveloppe_faible=%d' % (len(clouds), cases, extended, uncheckable, weak))
    gate.check(cases >= MIN_CASES, 'plancher : %d cas, au moins %d' % (cases, MIN_CASES))
    gate.check(extended >= MIN_EXTENDED, 'plancher : %d coquilles etendues jugees, au moins %d' % (extended, MIN_EXTENDED))
    gate.check(uncheckable >= MIN_UNCHECKABLE,
               'plancher : %d ordres non verifiables differents de 1, au moins %d' % (uncheckable, MIN_UNCHECKABLE))
    gate.check(weak >= MIN_WEAK, 'plancher : %d parties recoupees par l enveloppe faible, au moins %d' % (weak, MIN_WEAK))
    return gate.finish(floor=MIN_CHECKS)


if __name__ == '__main__':
    sys.exit(main())
