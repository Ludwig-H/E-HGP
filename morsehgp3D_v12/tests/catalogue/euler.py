#!/usr/bin/env python3
"""Filets du catalogue (CONTRAT_CATALOGUE.md, paragraphe 6.4) : identite d'Euler a K+2 (JUG-EULER) et restriction J1,
sur des vidages MHGP12DP de Cat_K et Cat_{K+2} rendus par la sonde. Port des regles du juge de la v11
(morsehgp3D_v11/bench/catalogue_euler.hpp, ac081a06f ; MATHEMATIQUES.md de la v11, paragraphe 8), arithmetique Python
exacte, juge NECESSAIRE et jamais certificat (deux omissions de contributions opposees se compensent : temoins de
tests/catalogue/euler_limits.py de la v11).

  Boule b de centre c, p interieurs stricts, coquille U de m sites ; ordre k, t = k - p. Si 1 <= t <= m :
    e_k(b) = somme, sur les parties A de U avec c dans conv(A) et |A| >= t, de (-1)^(|A|-t) C(|A|-1, t-1) ;
  coquille reguliere (m = q_min) : e_k = (-1)^(q-t) C(q-1, t-1). Identite : n [k = 1] + somme_b e_k(b) = 1 pour
  1 <= k <= min(K, n) (Cat_{K+2} contient toute boule de contribution non nulle a ces ordres).
  Restriction J1 : Cat_K egale le filtre p + q_min <= K + 1 de Cat_{K+2} (S*, p, m, q_min, I, U, rangs recalcules).
  Option --census : chaque site liste est juge par le signe exact de sa puissance (I strictement interieur, U sur la
  sphere refaite depuis S*), et chaque S* est un support de sa boule (centre dans l'interieur relatif).

    euler.py <sonde> (--uniform=N,GRAINE,BITS | --data=<nom>) --k=K --leaf=L [--threads=W] [--census]

Codes : 0 conforme ; 1 ecart ; 2 refus (usage, donnee absente, sonde en refus, coquille etendue au-dela de 20 sites) ;
3 plancher. Ligne finale si conforme :
  catalogue_euler_ok k=K n=N ordres=O boules_k=B boules_k2=B2 etendues=E
Python 3.10 nu, aucun assert.
"""
import collections
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', '..', 'reference'))
import catalogue_dump as dump  # noqa: E402
from hgp12_ref import intgeom as G  # noqa: E402

EXTENDED_LIMIT = 20


def binomial(a, b):
    if b < 0 or b > a:
        return 0
    out = 1
    for i in range(b):
        out = out * (a - i) // (i + 1)
    return out


def regular_contribution(p, q, k):
    t = k - p
    if t < 1 or t > q:
        return 0
    return (-1) ** (q - t) * binomial(q - 1, t - 1)


def extended_contributions(cat, b, orders):
    """Contributions e_k d'une coquille etendue : comptes N_j des parties qui contiennent un temoin d'enveloppe."""
    shell = list(cat.population(b))[cat.p[b]:]
    points = [cat.pos[s] for s in shell]
    anchor = cat.pos[cat.sstar[b][0]]
    center = G.through([cat.pos[s] for s in cat.sstar[b]])
    witnesses = G.hull_witnesses(points, anchor, center)
    m = len(shell)
    counts = [0] * (m + 1)
    for mask in range(1 << m):
        if any((w & ~mask) == 0 for w in witnesses):
            counts[bin(mask).count('1')] += 1
    out = {}
    for k in orders:
        t = k - cat.p[b]
        out[k] = 0 if t < 1 or t > m else sum((-1) ** (j - t) * binomial(j - 1, t - 1) * counts[j] for j in range(t, m + 1))
    return out


def positive_support(pts, center):
    """S* est un support de sa boule : centre dans l'interieur relatif (q2 : toujours ; q3 : triangle strictement aigu ;
    q4 : tetraedre non degenere contenant strictement le centre), predicats entiers de la reference."""
    if len(pts) == 2:
        return pts[0] != pts[1]
    if len(pts) == 3:
        return G.acute(pts[0], pts[1], pts[2])
    return G.orient(pts[0], pts[1], pts[2], pts[3]) != 0 and G.tetra_position(tuple(pts), pts[0], center) == 1


def census_errors(cat, errors, limit=20):
    """Signes exacts des sites listes et positivite de S* (option --census), en entiers."""
    for b in range(cat.count):
        pts = [cat.pos[s] for s in cat.sstar[b]]
        center = G.through(pts)
        if center is None:
            errors.append('boule %d : S* degenere' % b)
            continue
        population = list(cat.population(b))
        inner, shell = population[:cat.p[b]], population[cat.p[b]:]
        if any(G.side_key(pts[0], center, cat.pos[s]) >= 0 for s in inner) or \
           any(G.side_key(pts[0], center, cat.pos[s]) != 0 for s in shell) or \
           not set(cat.sstar[b]) <= set(shell) or not positive_support(pts, center):
            errors.append('boule %d : recensement ou support faux' % b)
        if len(errors) >= limit:
            return


def euler(cat_k2, n, kmax, errors):
    orders = range(1, min(kmax, n) + 1)
    totals = {k: (n if k == 1 else 0) for k in orders}
    regular = collections.Counter()
    extended = 0
    for b in range(cat_k2.count):
        if cat_k2.m[b] == cat_k2.q[b]:
            regular[(cat_k2.p[b], cat_k2.q[b])] += 1
            continue
        if cat_k2.m[b] > EXTENDED_LIMIT:
            raise ValueError('coquille etendue de %d sites au-dela du juge' % cat_k2.m[b])
        extended += 1
        for k, e in extended_contributions(cat_k2, b, orders).items():
            totals[k] += e
    for (p, q), count in regular.items():
        for k in orders:
            totals[k] += count * regular_contribution(p, q, k)
    for k in orders:
        if totals[k] != 1:
            errors.append('Euler a l\'ordre %d : %d' % (k, totals[k]))
    return len(orders), extended


def restriction(cat_k, cat_k2, kmax, errors):
    """J1 : Cat_K = filtre p + q_min <= K + 1 de Cat_{K+2}, dans le meme ordre, rangs recalcules."""
    kept = [b for b in range(cat_k2.count) if cat_k2.p[b] + cat_k2.q[b] <= kmax + 1]
    if len(kept) != cat_k.count:
        errors.append('J1 : %d boules filtrees, Cat_K en a %d' % (len(kept), cat_k.count))
        return
    rank, previous = 0, None
    for a, b in enumerate(kept):
        if cat_k2.rank[b] != previous:
            rank += 1
            previous = cat_k2.rank[b]
        if (cat_k.sstar[a], cat_k.p[a], cat_k.m[a], cat_k.q[a], cat_k.rank[a]) != \
           (cat_k2.sstar[b], cat_k2.p[b], cat_k2.m[b], cat_k2.q[b], rank) or cat_k.population(a) != cat_k2.population(b):
            errors.append('J1 : boule %d de Cat_K differente de la boule %d de Cat_K+2' % (a, b))
            return


def run_probe(argv_input, k, leaf, threads, folder, tag):
    out = os.path.join(folder, tag)
    cmd = [sys.argv[1]] + argv_input + ['--k=%d' % k, '--leaf=%d' % leaf, '--threads=%d' % threads, '--out=' + out]
    done = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if done.returncode != 0:
        return None
    return dump.Catalogue(dump.read_dump(os.path.join(out, 'cat.bin')))


def materialise(item, folder):
    """--uniform : la sonde exporte seulement depuis des fichiers ; le nuage synthetique est ecrit par le miroir du
    generateur de la sonde (SplitMix64, BITS bits, tri lexicographique, doublons elimines)."""
    n, seed, bits = (int(v) for v in item.split('=', 1)[1].split(','))
    state, mask, raw = seed, (1 << bits) - 1, []
    for _ in range(3 * n):
        state = (state + 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
        z = state
        z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & 0xFFFFFFFFFFFFFFFF
        z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & 0xFFFFFFFFFFFFFFFF
        raw.append((z ^ (z >> 31)) & mask)
    points = sorted(set(tuple(raw[3 * i:3 * i + 3]) for i in range(n)))
    xyz, ids = os.path.join(folder, 'in.u32le'), os.path.join(folder, 'in.ids.u32le')
    import struct
    with open(xyz, 'wb') as handle:
        handle.write(b''.join(struct.pack('<3I', *p) for p in points))
    with open(ids, 'wb') as handle:
        handle.write(struct.pack('<%dI' % len(points), *range(len(points))))
    return [xyz, ids]


def main(argv):
    options = {'k': None, 'leaf': None, 'threads': 4, 'census': False, 'input': None}
    for item in argv[2:]:
        key, _, value = item.partition('=')
        if key in ('--k', '--leaf', '--threads') and value.isdigit():
            options[key[2:]] = int(value)
        elif key == '--census' and not value:
            options['census'] = True
        elif key in ('--uniform', '--data'):
            options['input'] = item
        else:
            options['input'] = None
            break
    if len(argv) < 2 or not os.path.isfile(argv[1]) or options['input'] is None or options['k'] is None or \
       options['leaf'] is None or options['k'] + 2 > 12:
        print('catalogue_euler_refus usage')
        return 2
    with tempfile.TemporaryDirectory() as folder:
        if options['input'].startswith('--data='):
            stem = os.path.join(os.environ.get('MHGP12_DATA_DIR', ''), options['input'].split('=', 1)[1])
            source = [stem + '.u32le', stem + '.ids.u32le']
        else:
            source = materialise(options['input'], folder)
        cat_k = run_probe(source, options['k'], options['leaf'], options['threads'], folder, 'k')
        cat_k2 = run_probe(source, options['k'] + 2, options['leaf'], options['threads'], folder, 'k2')
    if cat_k is None or cat_k2 is None:
        print('catalogue_euler_refus sonde en refus')
        return 2
    errors = []
    try:
        orders, extended = euler(cat_k2, cat_k2.sites, options['k'], errors)
    except ValueError as refusal:
        print('catalogue_euler_refus %s' % refusal)
        return 2
    restriction(cat_k, cat_k2, options['k'], errors)
    if options['census']:
        census_errors(cat_k2, errors)
    for error in errors[:20]:
        print('ecart %s' % error)
    if errors:
        print('catalogue_euler_ecart ecarts=%d' % len(errors))
        return 1
    if cat_k.count == 0 or orders == 0:
        print('catalogue_euler_plancher')
        return 3
    print('catalogue_euler_ok k=%d n=%d ordres=%d boules_k=%d boules_k2=%d etendues=%d'
          % (options['k'], cat_k2.sites, orders, cat_k.count, cat_k2.count, extended))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
