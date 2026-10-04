#!/usr/bin/env python3
"""Fixtures gravees du juge d'Euler a K+2 : ses LIMITES, ses detections et ses refus.

    python3 euler_limits.py <mhgp11_catalogue_euler> <dossier de travail>

Bibliotheque standard seulement (Python 3.10 nu), aucun assert. Le juge n'est pas un certificat : deux omissions de
contributions opposees, retirees ensemble par le harnais (--omit, a la fois de Cat_K et de Cat_{K+2}), passent Euler
ET la restriction J1. Les fixtures gravent ces limites aux coordonnees exactes, et la detection des omissions voisines.
  - compensation5 (docs/MATHEMATIQUES.md, paragraphe 8) : au niveau 25, un triangle aigu (+1) et une paire (-1) a
    l'ordre 1 ; les retirer ensemble passe a K = 1 (Cat_3), echoue a K = 2 (ordre 2) ; un seul retrait echoue.
  - dt13 (v9, CONTRE_EXEMPLE_EULER_KPLUS2_20260923.md) : paire D et triangle T a quatre interieurs, polynomes
    -t^4 + t^5 et t^4 - 2t^5 + t^6 ; retrait commun invisible a K = 5 (Cat_7), vu a K = 6 (ordre 6).
  - dt23 : meme construction a neuf interieurs, invisible a K = 10 (Cat_12), vu par un retrait seul a l'ordre 10.
  - restriction J1 seule : D retiree de Cat_K seulement (--omit-k) passe Euler mais est absente de Cat_K ; retiree
    de Cat_{K+2} seulement (--omit-k2), Euler echoue et Cat_K a une boule en trop.
  - borne des coquilles etendues : 24 sites cocycliques acceptes, 25 refuses (code 2, aucun calcul).
  - refus d'usage, d'entree, de multiplicite, d'omission introuvable, et planchers (code 3).
Ligne finale : euler_limits_ok controles=N.
"""
import json
import math
import os
import shutil
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'support'))
import mhgp11_gate  # noqa: E402

COMPENSATION5 = ((0, 5, 0), (8, 9, 0), (8, 1, 0), (35, 5, 0), (45, 5, 0))
D13 = ((0, 0, 0), (20, 0, 0), (8, 1, 1), (9, 2, 2), (11, 1, 3), (12, 3, 1))
T13 = ((100, 0, 0), (120, 0, 0), (110, 16, 0), (108, 4, 1), (110, 4, 2), (112, 5, 1), (109, 6, 2))
D23 = D13 + ((7, 1, 1), (13, 1, 1), (10, 1, 1), (10, 2, 1), (10, 3, 1))
T23 = T13 + ((108, 5, 1), (109, 4, 1), (110, 5, 1), (111, 4, 1), (112, 4, 1))
FIRST_ID = 100  # PointId du premier site ; les suivants dans l'ordre des listes ci-dessus


def cocircular(count):
    """`count` sites entiers du cercle x^2 + y^2 = 65^2 (36 en tout), paire (65, 0), (-65, 0) comprise."""
    circle = sorted(((x, y) for x in range(-65, 66) for y in range(-65, 66) if x * x + y * y == 4225),
                    key=lambda p: math.atan2(p[1], p[0]))
    chosen = [(65, 0), (-65, 0)]
    chosen += [p for p in circle if p not in chosen][:count - 2]
    return tuple((x + 66, y + 66, 7) for x, y in chosen)


def ids(points, *sites):
    return ','.join(str(FIRST_ID + points.index(site)) for site in sites)


class Probe:
    def __init__(self, gate, probe, folder):
        self.gate, self.probe, self.folder = gate, probe, folder

    def write(self, name, points, identifiers=None):
        xyz, idf = (os.path.join(self.folder, name + suffix) for suffix in ('.u32le', '.ids.u32le'))
        identifiers = identifiers or [FIRST_ID + i for i in range(len(points))]
        with open(xyz, 'wb') as handle:
            handle.write(b''.join(struct.pack('<III', *point) for point in points))
        with open(idf, 'wb') as handle:
            handle.write(b''.join(struct.pack('<I', value) for value in identifiers))
        return '--input=%s,%s' % (xyz, idf)

    def run(self, what, argv, code):
        done = mhgp11_gate.run([self.probe] + argv, timeout=120)
        self.gate.check(done.code == code, '%s : %s, attendu code %d' % (what, done.describe(), code))
        phases, verdicts = {}, []
        for line in (done.stdout or '').splitlines():
            if line.startswith('{'):
                record = json.loads(line)
                phases[record.get('phase')] = record
            elif line.startswith('catalogue_euler_verdict'):
                verdicts.append(line)
        self.gate.check_eq(len(verdicts), 1, what + ' : une ligne de verdict')
        return phases, (verdicts[0] if verdicts else '')

    def euler(self, what, argv, code, failing, euler_by_k=None):
        phases, verdict = self.run(what, argv, code)
        record = phases.get('euler', {})
        self.gate.check_eq(record.get('failing_orders'), failing, what + ' : ordres verifiables en ecart')
        if euler_by_k is not None:
            self.gate.check_eq(record.get('euler_by_k'), euler_by_k, what + ' : euler_by_k')
        word = 'conforme' if code == 0 else 'ecart'
        self.gate.check(verdict.startswith('catalogue_euler_verdict %s ' % word), what + ' : verdict ' + word)
        return phases


def omitted(gate, phases, what, expected):
    """expected : liste de (in_k, in_k2, p, qmin, m) dans l'ordre des options."""
    got = [(b.get('in_k'), b.get('in_k2'), b.get('p'), b.get('qmin'), b.get('m'))
           for b in phases.get('omissions', {}).get('balls', [])]
    gate.check_eq(got, expected, what + ' : boules retirees (in_k, in_k2, p, qmin, m)')


def compensation(probe):
    gate = probe.gate
    cloud = probe.write('compensation5', COMPENSATION5)
    triangle = '--omit=' + ids(COMPENSATION5, *COMPENSATION5[:3])
    pair = '--omit=' + ids(COMPENSATION5, *COMPENSATION5[3:])
    probe.euler('compensation5 K1', [cloud, '--k=1'], 0, [], [1, 1, 1])
    phases = probe.euler('compensation5 K1 LIMITE triangle+paire', [cloud, '--k=1', triangle, pair], 0, [], [1, 2, 0])
    omitted(gate, phases, 'compensation5 K1', [(False, True, 0, 3, 3), (True, True, 0, 2, 2)])
    gate.check_eq(phases.get('restriction', {}).get('missing'), 0, 'compensation5 K1 : J1 aveugle au retrait commun')
    probe.euler('compensation5 K1 triangle seul', [cloud, '--k=1', triangle], 1, [1])
    probe.euler('compensation5 K1 paire seule', [cloud, '--k=1', pair], 1, [1])
    probe.euler('compensation5 K2 triangle+paire', [cloud, '--k=2', triangle, pair], 1, [2])


def dt(probe, name, d, t, kmax):
    gate = probe.gate
    points = d + t
    cloud = probe.write(name, points)
    omit_d, omit_t = '--omit=' + ids(points, d[0], d[1]), '--omit=' + ids(points, *t[:3])
    p = len(d) - 2
    probe.euler('%s K%d' % (name, kmax), [cloud, '--k=%d' % kmax], 0, [])
    phases = probe.euler('%s K%d LIMITE D+T' % (name, kmax), [cloud, '--k=%d' % kmax, omit_d, omit_t], 0, [])
    omitted(gate, phases, '%s K%d' % (name, kmax), [(True, True, p, 2, 2), (False, True, p, 3, 3)])
    restriction = phases.get('restriction', {})
    gate.check_eq((restriction.get('missing'), restriction.get('extra'), restriction.get('fields')), (0, 0, 0),
                  '%s K%d : restriction cle par cle egale malgre le retrait commun' % (name, kmax))
    probe.euler('%s K%d D seule' % (name, kmax), [cloud, '--k=%d' % kmax, omit_d], 1, [kmax])
    return cloud, omit_d, omit_t


def restriction_only(probe, cloud, omit_d):
    gate = probe.gate
    keep_k = omit_d.replace('--omit=', '--omit-k=')
    keep_k2 = omit_d.replace('--omit=', '--omit-k2=')
    phases = probe.euler('dt13 K5 D hors de Cat_K', [cloud, '--k=5', keep_k], 1, [], [1] * 7)
    restriction = phases.get('restriction', {})
    gate.check_eq((restriction.get('missing'), restriction.get('extra')), (1, 0), 'dt13 K5 : J1 voit D absente de Cat_K')
    gate.check_eq(restriction.get('first', {}).get('kind'), 'absente_de_cat_k', 'dt13 K5 : premier ecart de J1')
    phases = probe.euler('dt13 K5 D hors de Cat_K+2', [cloud, '--k=5', keep_k2], 1, [5])
    restriction = phases.get('restriction', {})
    gate.check_eq((restriction.get('missing'), restriction.get('extra')), (0, 1), 'dt13 K5 : J1 voit D en trop dans Cat_K')


def bounds(probe):
    gate = probe.gate
    phases = probe.euler('cercle24 K1', [probe.write('cercle24', cocircular(24)), '--k=1'], 0, [], [1, 1, 1])
    record = phases.get('euler', {})
    gate.check_eq((record.get('max_shell'), record.get('extended'), record.get('extended_by_shell')),
                  (24, 1, {'24': 1}), 'cercle24 : coquille de 24 sites jugee')
    phases, verdict = probe.run('cercle25 K1', [probe.write('cercle25', cocircular(25)), '--k=1'], 2)
    gate.check_eq((phases.get('euler', {}).get('refused_shell'), phases.get('euler', {}).get('euler_by_k')), (25, None),
                  'cercle25 : refus avant tout calcul, aucune somme publiee')
    gate.check_eq(verdict, 'catalogue_euler_verdict refus coquille_etendue m=25 borne=24', 'cercle25 : verdict')


def refusals(probe, dt13):
    gate = probe.gate
    pair = probe.write('paire', ((0, 0, 0), (3, 0, 0)))
    for argv, what in (([pair, '--k=0'], 'K nul'), ([pair, '--k=11'], 'K+2 au-dela de 12'), ([pair], 'K absent'),
                       ([pair, '--k=2', '--adaptive-frontier'], 'frontiere adaptative sans Pool'),
                       ([pair, '--k=2', '--omit=100'], 'omission d un seul site'),
                       ([pair, '--k=2', '--inconnue'], 'option inconnue'),
                       (['--k=2'], 'entree absente'),
                       ([pair, '--uniform18=10,1', '--k=2'], 'deux entrees')):
        _phases, verdict = probe.run('refus : ' + what, argv, 2)
        gate.check_eq(verdict, 'catalogue_euler_verdict refus usage', 'refus : %s : verdict' % what)
    missing = '--input=%s,%s' % (os.path.join(probe.folder, 'absent.u32le'), os.path.join(probe.folder, 'absent.ids'))
    _phases, verdict = probe.run('refus : fichier absent', [missing, '--k=2'], 2)
    gate.check_eq(verdict, 'catalogue_euler_verdict refus input_unreadable', 'refus : fichier absent : verdict')
    double = probe.write('doublon', ((0, 0, 0), (3, 0, 0), (3, 0, 0)), [7, 8, 9])
    _phases, verdict = probe.run('refus : multiplicite', [double, '--k=2'], 2)
    gate.check_eq(verdict, 'catalogue_euler_verdict refus multiplicity_unsupported', 'refus : multiplicite : verdict')
    # Paire (0,0,0)-(120,0,0) : sa boule diametrale a onze interieurs, hors de Cat_7.
    _phases, verdict = probe.run('refus : omission introuvable', [dt13, '--k=5', '--omit=100,107'], 2)
    gate.check_eq(verdict, 'catalogue_euler_verdict refus omission_introuvable', 'refus : omission : verdict')
    for option, code in (('--min-balls=70', 0), ('--min-balls=71', 3), ('--min-orders=5', 0), ('--min-orders=6', 3),
                         ('--min-extended=2', 0), ('--min-extended=3', 3), ('--min-compared=57', 0),
                         ('--min-compared=58', 3)):
        _phases, verdict = probe.run('plancher ' + option, [dt13, '--k=5', option], code)
        gate.check(verdict.startswith('catalogue_euler_verdict %s ' % ('conforme' if code == 0 else 'plancher')),
                   'plancher %s : verdict' % option)


def main():
    if len(sys.argv) != 3:
        print('usage : euler_limits.py <sonde> <dossier de travail>')
        return mhgp11_gate.REFUSAL
    gate = mhgp11_gate.Gate('euler_limits')
    folder = os.path.join(sys.argv[2], 'euler_limits_%d' % os.getpid())
    os.makedirs(folder)
    probe = Probe(gate, sys.argv[1], folder)
    try:
        compensation(probe)
        dt13, omit_d, omit_t = dt(probe, 'dt13', D13, T13, 5)
        probe.euler('dt13 K6 D+T', [dt13, '--k=6', omit_d, omit_t], 1, [6])
        probe.euler('dt13 K5 T seul', [dt13, '--k=5', omit_t], 1, [5])
        dt(probe, 'dt23', D23, T23, 10)
        restriction_only(probe, dt13, omit_d)
        bounds(probe)
        refusals(probe, dt13)
    finally:
        shutil.rmtree(folder, ignore_errors=True)
    return gate.finish(floor=120)


if __name__ == '__main__':
    sys.exit(main())
