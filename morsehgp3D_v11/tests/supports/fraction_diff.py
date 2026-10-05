#!/usr/bin/env python3
"""Porte mhgp11_supports_fraction (tranche S6a ; apports des auditeurs du 5 octobre 2026) : differentiel de la sonde
native du module supports contre l'oracle borne S1 (reference/hgp11_ref/supports.py : Fraction, etage A seul).

    python3 fraction_diff.py <mhgp11_supports_probe> <dossier de travail> --bits=<18|21|24>
            [--min-clouds=N] [--min-orders=N] [--min-balls=N] [--min-supports=N] [--min-extended=N]
            [--min-multiple=N] [--min-tetra=N]

Nuages : ceux de la suite de l'oracle (reference/test_supports.py : fixtures de la specification et des audits, nuages
d'Euler, fixtures historiques a positions distinctes, puis 16 nuages de 5 a 10 points par famille, graine 31), a leurs
ordres (ceux de la specification, et 1 a min(5, n - 1)). Un nuage hors du domaine du profil (une coordonnee >= 2^bits)
est exclu et compte ; les autres sont tous compares. Pour chaque (nuage, K) :
  - la sonde (--all : une ligne par boule de Cat_K, domaine prepare a K) est filtree a W_K (p + m >= K) ;
  - l'oracle rend la sortie canonique de l'ordre K (canonical) : W_K par la definition, Q_b par Gram, comptes par
    enumeration brute des parties de P_b et lemmes A a H controles ;
  - les boules sont appariees par (niveau exact, ensemble des supports en coordonnees) : memes boules de W_K, ni plus
    ni moins ;
  - pour chaque boule : p, m, qmin ; les cinq comptes (kparties_reliees, compressed_parts, strict_traces, cofaces,
    gabriel_cofaces) ; cofaces et cofaces de Gabriel par support, l'oracle rangeant les supports par (arite,
    coordonnees) et la sonde par (arite, SiteIdx) ; l'ordre natif (arite, rangs de Morton recalcules), S* en tete.
Codes : 0 conforme ; 1 ecart ; 2 refus d'usage ; 3 plancher. Dernieres lignes :
    supports_fraction_couverture bits=<b> nuages=<c> exclus=<x> ordres=<o> boules=<w> supports=<s> etendues=<e>
        multiples=<m> tetraedres=<t>
    supports_fraction_ok controles=<n>
Python 3.10 nu, bibliotheque standard seule, aucun assert. L'oracle est charge sans le paquet hgp11_ref
(ref_mutants.load_private, comme reference/test_supports.py) ; la porte ne cree aucun bytecode
(PYTHONDONTWRITEBYTECODE).
"""
import json
import os
import sys
from fractions import Fraction

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'reference'))
import sample_judge  # noqa: E402  (meme dossier : write_cloud, morton)
import test_supports  # noqa: E402  (suite de l'oracle S1 : fixtures, familles, chargement prive)

mhgp11_gate = sample_judge.mhgp11_gate
COUNTS = ('kparties_reliees', 'compressed_parts', 'strict_traces', 'cofaces', 'gabriel_cofaces')
FLOORS = ('clouds', 'orders', 'balls', 'supports', 'extended', 'multiple', 'tetra')


class Usage(Exception):
    pass


def options(argv):
    if len(argv) < 4:
        raise Usage('arguments manquants')
    out = {'probe': argv[1], 'work': argv[2], 'bits': 0, 'floors': {}}
    for arg in argv[3:]:
        key, _, value = arg.partition('=')
        if key == '--bits' and value in ('18', '21', '24'):
            out['bits'] = int(value)
        elif key.startswith('--min-') and key[6:] in FLOORS and value.isdigit():
            out['floors'][key[6:]] = int(value)
        else:
            raise Usage('option inconnue ou invalide %s' % arg)
    if out['bits'] == 0:
        raise Usage('--bits=18|21|24 obligatoire')
    return out


def native_balls(gate, probe, source, cloud, k, name):
    """Boules de W_K de la sonde : cle (niveau, ensemble des supports en coordonnees) -> enregistrement."""
    done = mhgp11_gate.run([probe, source, '--k=%d' % k, '--all'], timeout=120)
    if not gate.check(done.code == 0, '%s K%d : sonde %s' % (name, k, done.describe())):
        return None
    balls, verdicts = {}, 0
    for line in (done.stdout or '').splitlines():
        if line.startswith('supports_probe_verdict'):
            verdicts += 1
            gate.check(line.startswith('supports_probe_verdict conforme '), '%s K%d : %s' % (name, k, line))
        if not line.startswith('{'):
            continue
        record = json.loads(line)
        if record.get('phase') != 'ball' or record['p'] + record['m'] < k:
            continue
        sites = [[cloud.by_id.get(pid) for pid in support] for support in record['supports']]
        record['_sites'] = sites
        level = Fraction(int(record['level'][0], 16), int(record['level'][1], 16))
        key = (level, frozenset(frozenset(s) for s in sites))
        gate.check(key not in balls, '%s K%d : boule native en double %s' % (name, k, level))
        balls[key] = record
    gate.check_eq(verdicts, 1, '%s K%d : une ligne de verdict' % (name, k))
    return balls


def compare_ball(gate, cloud, name, k, record, ball, totals):
    """Une boule appariee : forme, comptes, comptes par support (reordonnes), ordre natif."""
    where = '%s K%d boule %s' % (name, k, ball['level'])
    gate.check_eq([record['p'], record['m'], record['qmin']], [ball['p'], ball['m'], ball['qmin']],
                  where + ' : (p, m, qmin)')
    gate.check_eq([record['counts'][c] for c in COUNTS], [ball[c] for c in COUNTS], where + ' : comptes du lemme G')
    oracle_supports = [frozenset(tuple(p) for p in s) for s in ball['supports']]
    native = [frozenset(s) for s in record['_sites']]
    order = [oracle_supports.index(s) if s in oracle_supports else -1 for s in native]
    if gate.check(-1 not in order and len(order) == len(oracle_supports), where + ' : supports apparies'):
        gate.check_eq(record['cofaces_support'], [ball['cofaces_support'][i] for i in order],
                      where + ' : cofaces par support')
        gate.check_eq(record['gabriel_cofaces_support'], [ball['gabriel_cofaces_support'][i] for i in order],
                      where + ' : cofaces de Gabriel par support')
    keys = [(len(s), sorted(cloud.rank[p] for p in s)) for s in record['_sites']]
    gate.check(keys == sorted(keys) and all(len(s) >= 2 for s in record['_sites']),
               where + ' : ordre natif (arite, rangs de Morton)')
    gate.check(record['_sites'][:1] == [[cloud.by_id.get(pid) for pid in record['star']]], where + ' : S* en tete')
    totals['balls'] += 1
    totals['supports'] += len(native)
    totals['extended'] += record['m'] > record['qmin']
    totals['multiple'] += len(native) > 1
    totals['tetra'] += sum(len(s) == 4 for s in native)


def compare_cloud(gate, o, oracle_module, name, points, ks, totals):
    ids = list(range(len(points)))
    cloud = sample_judge.Cloud(points, ids)
    source = sample_judge.write_cloud(o['work'], 'nuage', points, ids)
    try:
        oracle = oracle_module.Supports(points)
        docs = [oracle.canonical(k) for k in ks]
    except Exception as exc:  # noqa: BLE001 -- un lemme viole par l'oracle est un ecart, pas une trace Python
        gate.check(False, '%s : oracle : %s : %s' % (name, type(exc).__name__, exc))
        return
    for k, doc in zip(ks, docs):
        native = native_balls(gate, o['probe'], source, cloud, k, name)
        if native is None:
            continue
        totals['orders'] += 1
        reference = {}
        for ball in doc['balls']:
            key = (Fraction(ball['level']), frozenset(frozenset(tuple(p) for p in s) for s in ball['supports']))
            reference[key] = ball
        missing = [str(key[0]) for key in reference if key not in native]
        extra = [str(key[0]) for key in native if key not in reference]
        if not gate.check(not missing and not extra, '%s K%d : boules de W_K, absentes de la sonde %r, en trop %r'
                          % (name, k, missing[:5], extra[:5])):
            continue
        for key, ball in sorted(reference.items(), key=lambda item: (item[0][0], sorted(map(sorted, item[0][1])))):
            compare_ball(gate, cloud, name, k, native[key], ball, totals)


def main():
    gate = mhgp11_gate.Gate('supports_fraction')
    try:
        o = options(sys.argv)
    except Usage as error:
        print('usage : %s' % error)
        return mhgp11_gate.REFUSAL
    os.makedirs(o['work'], exist_ok=True)
    oracle_module = test_supports.STAGE['supports']
    totals = dict((name, 0) for name in FLOORS)
    excluded = 0
    for name, points, ks in test_supports.fixtures() + test_supports.family_clouds():
        if max(c for p in points for c in p) >= 1 << o['bits'] or min(c for p in points for c in p) < 0:
            excluded += 1
            continue
        totals['clouds'] += 1
        compare_cloud(gate, o, oracle_module, name, points, ks, totals)
    print('supports_fraction_couverture bits=%d nuages=%d exclus=%d ordres=%d boules=%d supports=%d etendues=%d '
          'multiples=%d tetraedres=%d' % (o['bits'], totals['clouds'], excluded, totals['orders'], totals['balls'],
                                          totals['supports'], totals['extended'], totals['multiple'],
                                          totals['tetra']))
    below = [name for name in FLOORS if totals[name] < o['floors'].get(name, 0)]
    if gate.failures == 0 and below:
        print('PLANCHER supports_fraction : %s sous %s' % (', '.join(below), json.dumps(o['floors'], sort_keys=True)))
        return mhgp11_gate.FLOOR
    return gate.finish(floor=1)


if __name__ == '__main__':
    sys.exit(main())
