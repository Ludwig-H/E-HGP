#!/usr/bin/env python3
"""Juge d'echantillon du module supports (sortie parametree, tranche S6a) : Fraction et bibliotheque standard.

    python3 sample_judge.py <mhgp11_supports_probe> <dossier de travail> <entree> --k=<K> <selection>
            [--workers=<W>] [--min-balls=<N>] [--min-extended=<N>] [--min-brute=<N>] [--min-multiple=<N>]
            [--min-tetra=<N>]
    entree     --uniform18=<n>,<graine>        random.Random(graine).getrandbits(18) pour x, y puis z de chaque point,
                                               PointId 0..n-1 (famille des bancs du catalogue), ecrite en u32le
               --grid=<n>,<cote>,<pas>,<graine> n cases distinctes de la grille cote^3 (random.Random(graine).sample),
                                               coordonnees case * pas : coquilles cospheriques nombreuses
               --data=<nom>                    trame du dossier MHGP11_DATA_DIR (jamais recopiee)
               --small=<graine>,<nuages>       petits nuages de grille (12 a 40 points, cote 3 a 5) : toutes leurs
                                               boules sont jugees (sonde --all)
    selection  --sample=<N>,<graine> [--extended=<E>]   (transmis a la sonde ; sans objet pour --small)
               --registers   aucune boule jugee : la sonde parcourt W_K (--window --forest) et confronte les six
                             registres de la foret d'ordre K aux comptes (classified_cells, deux sommes de C(m,t),
                             replayed_cells, trace_resolutions = somme des strict_traces, naissances) ; le juge exige
                             le code 0, six paires egales, et recopie la ligne de verdict de la sonde (gravable)

La sonde rend, pour chaque boule choisie dans Cat_K, S* et Q_b en PointId et en SiteIdx, le niveau, p, m, qmin, la
fermeture N_j et les comptes du lemme G. Le juge les recalcule sans code du produit :
  - la sphere de S* (centre dans l'enveloppe affine, Gram) et son niveau, egal au niveau publie ;
  - I_b et U_b par force brute sur tous les sites (distances entieres apres mise au denominateur commun, elagage
    par une fenetre en x exacte) : p et m publies, S* inclus dans U_b ;
  - les rangs de Morton des sites : SiteIdx publies, ordre (arite, rangs) de Q_b ;
  - Q_b : parties de 2 a 4 sites de U_b dont les poids barycentriques du centre sont tous strictement positifs ;
    egal a la liste publiee, dans l'ordre, S* en tete, qmin = plus petite arite ;
  - N_j : parties de U_b de cardinal j qui contiennent un support (enumeration des parties) ; recoupe partie par
    partie avec l'enveloppe FAIBLE (c dans conv(A) : poids positifs ou nuls d'au plus quatre sites, M1) ;
  - les comptes par leurs formules (math.comb), puis, si C(p+m, K+1) <= 20000, par denombrement brut des
    (K+1)-parties G de P_b avec c dans conv(G inter U_b), de celles qui contiennent I_b, des incidences de chaque
    support, et des t-parties de U_b separables (traces strictes).
Codes : 0 conforme ; 1 desaccord ; 2 refus (usage, donnees absentes) ; 3 plancher. Dernieres lignes :
    supports_sample_judge_couverture boules=<b> etendues=<e> multiples=<x> tetraedres=<t> brutes=<d>
    supports_sample_judge_ok controles=<c>
Python 3.10 nu, aucun assert.
"""
import bisect
import json
import math
import os
import random
import struct
import sys
from fractions import Fraction as F
from itertools import combinations

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'support'))
import mhgp11_gate  # noqa: E402

BRUTE_LIMIT = 20000   # C(p+m, K+1) au-dela duquel le denombrement brut des cofaces n'est pas fait
CLOSURE_LIMIT = 16    # coquilles jugees partie par partie (2^m parties)


class Usage(Exception):
    pass


# ---------------------------------------------------------------- geometrie exacte
def solve(rows, rhs):
    """Systeme lineaire carre en Fraction (Gauss-Jordan) ; None s'il est singulier."""
    size = len(rhs)
    m = [[F(v) for v in rows[i]] + [F(rhs[i])] for i in range(size)]
    for col in range(size):
        pivot = next((r for r in range(col, size) if m[r][col] != 0), None)
        if pivot is None:
            return None
        m[col], m[pivot] = m[pivot], m[col]
        for r in range(size):
            if r != col and m[r][col] != 0:
                f = m[r][col] / m[col][col]
                m[r] = [x - f * y for x, y in zip(m[r], m[col])]
    return [m[i][size] / m[i][i] for i in range(size)]


def circumsphere(points):
    """Centre (dans l'enveloppe affine) et rayon carre de la sphere circonscrite, ou None si dependance affine."""
    p0 = points[0]
    d = [tuple(a - b for a, b in zip(p, p0)) for p in points[1:]]
    lam = solve([[2 * sum(x * y for x, y in zip(a, b)) for b in d] for a in d], [sum(x * x for x in a) for a in d])
    if lam is None:
        return None
    center = tuple(F(p0[j]) + sum(w * v[j] for w, v in zip(lam, d)) for j in range(3))
    return center, sum((center[j] - p0[j]) ** 2 for j in range(3))


def weights(points, target):
    """Poids barycentriques de target dans l'enveloppe affine de points (independants), ou None."""
    p0 = points[0]
    d = [tuple(a - b for a, b in zip(p, p0)) for p in points[1:]]
    lam = solve([[sum(x * y for x, y in zip(a, b)) for b in d] for a in d],
                [sum(x * (t - c) for x, t, c in zip(a, target, p0)) for a in d])
    if lam is None:
        return None
    w = [F(1) - sum(lam)] + lam
    if tuple(sum(wi * p[j] for wi, p in zip(w, points)) for j in range(3)) != target:
        return None
    return w


def morton(p):
    key = 0
    for i in range(24):
        key |= (((p[0] >> i) & 1) << (3 * i)) | (((p[1] >> i) & 1) << (3 * i + 1)) | (((p[2] >> i) & 1) << (3 * i + 2))
    return key


# ---------------------------------------------------------------- nuages
class Cloud:
    def __init__(self, points, ids):
        self.points, self.ids = points, ids
        self.by_id = {pid: p for pid, p in zip(ids, points)}
        order = sorted(points, key=morton)
        self.rank = {p: i for i, p in enumerate(order)}
        self.by_x = sorted(points)
        self.xs = [p[0] for p in self.by_x]

    def fnv(self):
        h = 0xcbf29ce484222325
        for word in [c for p in self.points for c in p] + list(self.ids):
            for b in range(4):
                h = ((h ^ ((word >> (8 * b)) & 255)) * 0x100000001b3) & 0xFFFFFFFFFFFFFFFF
        return '%016x' % h

    def census(self, center, level):
        """(I_b, U_b) par force brute exacte : centre C/D, comparaison de |D x - C|^2 a D^2 level."""
        den = 1
        for c in center:
            den = den * c.denominator // math.gcd(den, c.denominator)
        big = [int(c * den) for c in center]
        radius = level * den * den
        if radius.denominator != 1:
            return None
        radius = radius.numerator
        reach = math.isqrt(radius) + 1
        lo = bisect.bisect_left(self.xs, (big[0] - reach) // den)
        hi = bisect.bisect_right(self.xs, (big[0] + reach) // den + 1)
        inner, shell = [], []
        for p in self.by_x[lo:hi]:
            d2 = sum((den * p[j] - big[j]) ** 2 for j in range(3))
            if d2 < radius:
                inner.append(p)
            elif d2 == radius:
                shell.append(p)
        return inner, shell


def write_cloud(folder, name, points, ids):
    xyz, idf = (os.path.join(folder, name + suffix) for suffix in ('.u32le', '.ids.u32le'))
    with open(xyz, 'wb') as handle:
        handle.write(b''.join(struct.pack('<III', *p) for p in points))
    with open(idf, 'wb') as handle:
        handle.write(b''.join(struct.pack('<I', i) for i in ids))
    return '--input=%s,%s' % (xyz, idf)


def read_cloud(base):
    with open(base + '.u32le', 'rb') as handle:
        raw = handle.read()
    with open(base + '.ids.u32le', 'rb') as handle:
        rawid = handle.read()
    if len(raw) % 12 != 0 or len(rawid) * 3 != len(raw):
        raise Usage('fichiers %s incoherents' % base)
    points = [struct.unpack_from('<III', raw, 12 * i) for i in range(len(raw) // 12)]
    ids = [struct.unpack_from('<I', rawid, 4 * i)[0] for i in range(len(rawid) // 4)]
    return points, ids


def grid_points(n, side, step, seed):
    cells = random.Random(seed).sample(range(side ** 3), n)
    return [((c // (side * side)) * step, ((c // side) % side) * step, (c % side) * step) for c in cells]


# ---------------------------------------------------------------- jugement d'une boule
class Judge:
    def __init__(self, gate, cloud, k):
        self.gate, self.cloud, self.k = gate, cloud, k
        self.balls = self.extended = self.multiple = self.tetra = self.brute = 0

    def check(self, ok, what):
        return self.gate.check(ok, what)

    def ball(self, record):
        gate, cloud, k = self.gate, self.cloud, self.k
        name = 'boule %s' % record.get('ball')
        star = [cloud.by_id.get(pid) for pid in record['star']]
        if not self.check(None not in star and len(star) == record['qmin'], name + ' : S* connu'):
            return
        sphere = circumsphere(star)
        if not self.check(sphere is not None, name + ' : S* affinement independant'):
            return
        center, level = sphere
        published = F(int(record['level'][0], 16), int(record['level'][1], 16))
        self.check(level == published, name + ' : niveau de S* %s, publie %s' % (level, published))
        census = cloud.census(center, level)
        if not self.check(census is not None, name + ' : rayon entier apres mise au denominateur'):
            return
        inner, shell = census
        shell.sort(key=lambda p: cloud.rank[p])
        p, m = len(inner), len(shell)
        gate.check_eq((p, m), (record['p'], record['m']), name + ' : (p, m) par force brute')
        self.check(all(s in shell for s in star), name + ' : S* dans U_b')
        if (p, m) != (record['p'], record['m']) or m > 24:
            return
        self.balls += 1
        self.extended += m > record['qmin']
        strict, weak, supports = self.shell_parts(center, shell)
        published = [[cloud.by_id.get(pid) for pid in support] for support in record['supports']]
        expected = [[shell[i] for i in range(m) if mask >> i & 1] for mask in supports]
        self.check(published == expected, name + ' : Q_b (ordre (arite, rangs de Morton))')
        ranks = [[cloud.rank.get(s) for s in support] for support in published]
        self.check(ranks == record['sites'], name + ' : SiteIdx = rangs de Morton')
        self.check(expected[:1] == [star] and min(len(s) for s in expected) == record['qmin'],
                   name + ' : S* en tete, qmin = plus petite arite')
        self.multiple += len(expected) > 1
        self.tetra += sum(len(s) == 4 for s in expected)
        closure = list(record['closure'])  # au-dela de CLOSURE_LIMIT : formules recoupees sur les N_j publies
        if strict is not None:
            closure = [0] * (m + 1)
            for mask in range(1 << m):
                closure[bin(mask).count('1')] += strict[mask]
            self.check(closure == record['closure'], name + ' : N_j %r, publie %r' % (closure, record['closure']))
            self.check(strict == weak, name + ' : contient un support <=> c dans conv(A) (enveloppe faible)')
        self.counts(name, record, p, m, supports, closure, weak)

    def shell_parts(self, center, shell):
        """Supports stricts (masques, ordre publie) ; si m <= CLOSURE_LIMIT, pour chaque partie A de U_b : contient
        un support (strict) et c dans conv(A) (faible, Caratheodory : au plus quatre sites, poids >= 0)."""
        m = len(shell)
        supports, independent = [], {}
        for size in (2, 3, 4):
            for subset in combinations(range(m), size):
                w = weights([shell[i] for i in subset], center)
                mask = sum(1 << i for i in subset)
                independent[mask] = w
                if w is not None and all(x > 0 for x in w):
                    supports.append(mask)
        supports.sort(key=lambda mask: (bin(mask).count('1'), [i for i in range(m) if mask >> i & 1]))
        if m > CLOSURE_LIMIT:
            return None, None, supports
        strict = [0] * (1 << m)
        weak = [0] * (1 << m)
        for mask in range(1, 1 << m):
            strict[mask] = int(any(mask & s == s for s in supports))
            count = bin(mask).count('1')
            if count <= 4:
                w = independent.get(mask)
                inside = w is not None and all(x >= 0 for x in w)
            else:
                inside = False
            weak[mask] = int(inside or any(weak[mask & ~(1 << i)] for i in range(m) if mask >> i & 1))
        return strict, weak, supports

    def counts(self, name, record, p, m, supports, closure, closed):
        gate, k = self.gate, self.k
        t = k - p
        n = closure + [0] * 40
        formula = {'kparties_reliees': math.comb(p + m, k),
                   'compressed_parts': math.comb(m, t) if 0 <= t <= m else 0,
                   'strict_traces': (math.comb(m, t) - n[t]) if 0 <= t <= m else 0,
                   'cofaces': sum(math.comb(p, k + 1 - j) * n[j] for j in range(m + 1) if 0 <= k + 1 - j <= p),
                   'gabriel_cofaces': n[t + 1] if t + 1 >= 0 else 0}
        gate.check_eq(record['counts'], formula, name + ' : comptes du lemme G')
        arities = [bin(mask).count('1') for mask in supports]
        gate.check_eq(record['cofaces_support'],
                      [math.comb(p + m - a, k + 1 - a) if k + 1 >= a else 0 for a in arities],
                      name + ' : cofaces par support')
        gate.check_eq(record['gabriel_cofaces_support'],
                      [math.comb(m - a, t + 1 - a) if t + 1 >= a else 0 for a in arities],
                      name + ' : cofaces de Gabriel par support')
        if closed is None or math.comb(p + m, k + 1) > BRUTE_LIMIT:
            return
        self.brute += 1
        cofaces = gabriel = 0
        incidences = [0] * len(supports)
        gabriel_incidences = [0] * len(supports)
        for g in combinations(range(p + m), k + 1):
            mask = sum(1 << (i - p) for i in g if i >= p)
            if not closed[mask]:
                continue
            cofaces += 1
            with_inner = all(i in g for i in range(p))
            gabriel += with_inner
            for s, support in enumerate(supports):
                if mask & support == support:
                    incidences[s] += 1
                    gabriel_incidences[s] += with_inner
        separable = sum(1 for a in combinations(range(m), t) if not closed[sum(1 << i for i in a)]) if 1 <= t <= m \
            else 0
        brute = {'kparties_reliees': sum(1 for _ in combinations(range(p + m), k)), 'compressed_parts':
                 sum(1 for f in combinations(range(p + m), k) if all(i in f for i in range(p))),
                 'strict_traces': separable, 'cofaces': cofaces, 'gabriel_cofaces': gabriel}
        gate.check_eq(record['counts'], brute, name + ' : comptes par denombrement brut')
        gate.check_eq(record['cofaces_support'], incidences, name + ' : incidences par denombrement brut')
        gate.check_eq(record['gabriel_cofaces_support'], gabriel_incidences, name + ' : incidences de Gabriel brutes')


# ---------------------------------------------------------------- sonde
def run_probe(gate, judge, probe, argv, timeout):
    done = mhgp11_gate.run([probe] + argv, timeout=timeout)
    gate.check(done.code == 0, 'sonde %s : %s, attendu code 0' % (' '.join(argv), done.describe()))
    records, verdicts, domain, forest, aggregates = [], [], {}, None, []
    for line in (done.stdout or '').splitlines():
        if line.startswith('{'):
            record = json.loads(line)
            if record.get('phase') == 'ball':
                records.append(record)
            elif record.get('phase') == 'domain':
                domain = record
            elif record.get('phase') == 'forest':
                forest = record
            if record.get('phase') in ('measure', 'supports', 'forest'):
                aggregates.append(line)  # agregats et temps de la sonde, recopies pour les recus (aucune coordonnee)
        elif line.startswith('supports_probe_verdict'):
            verdicts.append(line)
    if '--forest' in argv:
        for line in aggregates:
            print(line)
        pairs = [value for key, value in sorted((forest or {}).items()) if isinstance(value, list)]
        gate.check_eq(len(pairs), 6, 'registres de la foret : six paires')
        for pair in pairs:
            gate.check(len(pair) == 2 and pair[0] == pair[1], 'registre de la foret %r' % (pair,))
        if verdicts:
            print(verdicts[0])
    gate.check_eq(len(verdicts), 1, 'sonde : une ligne de verdict')
    if verdicts:  # --window ne publie aucune ligne de boule : choisies = |W_K|
        chosen = '--forest' in argv or (' choisies=%d ' % len(records)) in verdicts[0]
        gate.check(verdicts[0].startswith('supports_probe_verdict conforme ') and chosen, 'verdict : ' + verdicts[0])
    gate.check_eq(domain.get('input_fnv1a64'), judge.cloud.fnv(), 'empreinte de l entree lue par la sonde')
    for record in records:
        judge.ball(record)
    return len(records)


def options(argv):
    if len(argv) < 4:
        raise Usage('arguments manquants')
    out = {'probe': argv[1], 'work': argv[2], 'input': None, 'k': 0, 'probe_args': [], 'floors': {}}
    for arg in argv[3:]:
        key, _, value = arg.partition('=')
        if key in ('--uniform18', '--grid', '--data', '--small'):
            if out['input'] is not None:
                raise Usage('une seule entree')
            out['input'] = (key[2:], value)
        elif key == '--k':
            out['k'] = int(value)
        elif key in ('--sample', '--extended', '--workers'):
            out['probe_args'].append(arg)
        elif arg == '--registers':
            out['probe_args'] += ['--window', '--forest']
            out['registers'] = True
        elif key in ('--min-balls', '--min-extended', '--min-brute', '--min-multiple', '--min-tetra'):
            out['floors'][key[6:]] = int(value)
        else:
            raise Usage('option inconnue ' + arg)
    if out['input'] is None or not 1 <= out['k'] <= 12:
        raise Usage('entree ou K invalide')
    return out


def main():
    gate = mhgp11_gate.Gate('supports_sample_judge')
    try:
        o = options(sys.argv)
    except (Usage, ValueError) as error:
        print('usage : %s' % error)
        return mhgp11_gate.REFUSAL
    kind, value = o['input']
    os.makedirs(o['work'], exist_ok=True)
    k, probe = o['k'], o['probe']
    common = ['--k=%d' % k] + o['probe_args']
    judges = []
    if kind == 'small':
        seed, count = (int(v) for v in value.split(','))
        generator = random.Random(seed)
        for number in range(count):
            n, side = generator.randint(12, 40), generator.randint(3, 5)
            points = grid_points(min(n, side ** 3), side, generator.choice((1, 2, 3)), generator.getrandbits(32))
            ids = [generator.getrandbits(32) | (number << 24) for _ in points]
            if len(set(ids)) != len(ids):
                ids = list(range(len(points)))
            judge = Judge(gate, Cloud(points, ids), k)
            run_probe(gate, judge, probe, [write_cloud(o['work'], 'small%d' % number, points, ids), '--all',
                                           '--k=%d' % k], 120)
            judges.append(judge)
    else:
        if kind == 'data':
            folder = mhgp11_gate.require_data_dir()
            points, ids = read_cloud(os.path.join(folder, value))
            source = '--data=' + value
        elif kind == 'uniform18':
            n, seed = (int(v) for v in value.split(','))
            generator = random.Random(seed)
            points = []
            for _ in range(n):
                x = generator.getrandbits(18)
                y = generator.getrandbits(18)
                points.append((x, y, generator.getrandbits(18)))
            ids = list(range(n))
            source = write_cloud(o['work'], 'uniform18', points, ids)
        else:
            n, side, step, seed = (int(v) for v in value.split(','))
            points = grid_points(n, side, step, seed)
            ids = list(range(n))
            source = write_cloud(o['work'], 'grid', points, ids)
        judge = Judge(gate, Cloud(points, ids), k)
        run_probe(gate, judge, probe, [source] + common, 3000)
        judges.append(judge)
    totals = {name: sum(getattr(j, name) for j in judges)
              for name in ('balls', 'extended', 'multiple', 'tetra', 'brute')}
    floors = {'balls': 'balls', 'extended': 'extended', 'brute': 'brute', 'multiple': 'multiple', 'tetra': 'tetra'}
    below = [name for name, field in floors.items() if totals[field] < o['floors'].get(name, 0)]
    print('supports_sample_judge_couverture boules=%d etendues=%d multiples=%d tetraedres=%d brutes=%d'
          % (totals['balls'], totals['extended'], totals['multiple'], totals['tetra'], totals['brute']))
    if gate.failures == 0 and below:
        print('PLANCHER supports_sample_judge : %s sous %s' % (', '.join(below), json.dumps(o['floors'], sort_keys=True)))
        return mhgp11_gate.FLOOR
    return gate.finish(floor=1)


if __name__ == '__main__':
    sys.exit(main())
