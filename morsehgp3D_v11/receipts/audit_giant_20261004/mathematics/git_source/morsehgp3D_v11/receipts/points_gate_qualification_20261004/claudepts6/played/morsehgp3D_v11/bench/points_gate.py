#!/usr/bin/env python3
"""Porte de la pendaison FULL -> points : export natif + consommateur contre l'oracle de la definition.

    python3 bench/points_gate.py --export BUILD/mhgp11_points_export --work DIR --out FICHIER.json [--seconds 240]

Pour chaque nuage (fixtures gravees puis nuages aleatoires a ex aequo frequents, au plus neuf sites, k <= 4),
l'export natif est lu par bench/points_hierarchy.py ; les cinq regles (core, cover, first, margin1, margin, avec
m = 1, k + 1, k + 2) rendent des ultrametriques exactes comparees a celles de bench/points_reference.py (force
brute sur les coupes de Gamma_k, etage A) ; la marge en rayon (bench/points_radius.py, decisions exactes) est
comparee a l'oracle decimal a 120 chiffres, a 1e-9 relatif. Les sites natifs sont reindexes par leurs PointId.
Codes : 0 conforme ; 1 desaccord ; 3 plancher non atteint ou fixture fausse ; 2 refus d'entree.
"""
import argparse
from fractions import Fraction
import json
import os
from pathlib import Path
import random
import subprocess
import sys
import time

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import points_hierarchy as ph  # noqa: E402
import points_reference as pr  # noqa: E402
import points_radius as prad  # noqa: E402
from hgp11_ref import Definition  # noqa: E402

EQUILATERAL = [tuple(c + 2 for c in p) for p in ((-1, -1, 0), (-1, 0, -1), (0, 0, 0), (1, 1, 0), (2, 2, 0), (2, 1, 1))]
FIVE = [(6, 2, 0), (0, 0, 0), (0, 4, 0), (12, 0, 0), (12, 4, 0)]
# Temoin de l'auditeur (hm_followup_20261003) : x = site 0, point de coeur d'un amas qualifie, entre apres F = 12.
EIGHT = [(20, 20, 0), (30, 20, 0), (40, 20, 0), (50, 20, 0), (64, 20, 0), (74, 20, 0), (0, 30, 0), (0, 10, 0)]
# Temoin de l'auditeur (points_answers_20261003/owner_plateau) : a k = 2, m = 1, la date du site 0 vaut
# sqrt(2) + sqrt(18) - sqrt(8) = sqrt(8), exactement la naissance du parent {0, 1, 3} ; la coupe fermee le prend.
PLATEAU = [(4, 4, 4), (6, 6, 4), (0, 0, 4), (8, 8, 4)]
FLOORS = dict(clouds=150, comparisons=4000, delayed=500, fixtures=12, mutants=4)
MUTANT_CLOUDS = 24


def run_export(binary, points, work, name, kmax):
    work.mkdir(parents=True, exist_ok=True)  # les mutants exportent dans leur propre sous-dossier
    xyz, ids = work / (name + '.xyz'), work / (name + '.ids')
    np.asarray(points, dtype='<u4').tofile(xyz)
    np.arange(len(points), dtype='<u4').tofile(ids)
    out = work / (name + '.ph')
    orders = ','.join(str(k) for k in range(1, kmax + 1))
    done = subprocess.run([str(binary), str(xyz), str(ids), str(out), str(kmax), orders, '2', str(1 << 30)],
                          capture_output=True, text=True, timeout=60)
    if done.returncode != 0:
        raise RuntimeError('export %s code %d : %s' % (name, done.returncode, done.stdout + done.stderr))
    data = ph.read_export(out)
    for path in (xyz, ids, out):
        path.unlink()
    return data


def native_ultrametrics(data, k, m):
    order = data['orders'][k]
    ids = data['ids'].tolist()
    out = {}
    for rule, hanging in pr.bench_hangings(order, m).items():
        u = ph.ultrametric(hanging)
        n = len(u)
        back = [[None] * n for _ in range(n)]
        for a in range(n):
            for b in range(n):
                back[ids[a]][ids[b]] = u[a][b]
        out[rule] = back
    return out


def bench_owner_signature(order, node, ids):
    """Identite du noeud cote banc : (niveau de naissance exact, sites couverts a la naissance en indices d'entree)."""
    level = Fraction(*order.levels.exact(int(order.rank[node])))
    birth = int(order.rank[node])
    inside = set()
    for j in range(len(order.inc_site)):
        if int(order.inc_rank[j]) <= birth:
            v = int(order.inc_node[j])
            if int(order.lca(np.array([node]), np.array([v]))[0]) == node:
                inside.add(ids[int(order.inc_site[j])])
    return (level, frozenset(inside))


def radius_blocks(order, hang, ids, level):
    """Blocs exacts de la pendaison en rayon au rayon sqrt(level), coupe fermee, en indices d'entree."""
    target = prad.RValue(Fraction(level))
    groups = {}
    for s in range(order.n):
        if hang.values[s].cmp(target) <= 0:
            top = prad.ancestor_at_radius(order, int(hang.owner[s]), target)
            groups.setdefault(top, []).append(ids[s])
    return sorted(sorted(b) for b in groups.values())


def compare_cloud(binary, points, work, name, stats):
    n = len(points)
    kmax = min(4, n)
    data = run_export(binary, points, work, name, kmax)
    definition = Definition(points)
    delayed = 0
    for k in range(1, kmax + 1):
        res = definition.order(k)
        for m in sorted(set([1, k + 1, k + 2])):
            if m > n:
                continue
            ref, tree = pr.reference_rules(res, n, m)
            native = native_ultrametrics(data, k, m)
            for rule in pr.RULES:
                stats['comparisons'] += 1
                if pr.reference_ultrametric(ref[rule], tree) != native[rule]:
                    stats['disagreements'].append(dict(cloud=name, points=points, k=k, m=m, rule=rule))
            order = data['orders'][k]
            delayed += ph.hang_margin(order, m).extra['delayed']
            # Marge en rayon : regle exacte du banc contre l'oracle de la definition (Decimal, 120 chiffres dans un
            # seul contexte). Comparaison EXACTE par site : date (somme de radicaux, egalite certifiee par classes de
            # carres) et proprietaire (niveau de naissance, sites couverts a la naissance), donc ultrametrique exacte.
            refr, _treer = pr.reference_radius_rules(res, n, m)
            hang = prad.hang_margin_radius(order, m)
            ids = data['ids'].tolist()
            stats['comparisons'] += 1
            for s_native in range(n):
                orig = ids[s_native]
                _e, owner, (t, meet, h) = refr['margin_r'][orig]
                same_date = hang.values[s_native].cmp(prad.RValue(t, meet, h)) == 0
                same_owner = bench_owner_signature(order, int(hang.owner[s_native]), ids) == \
                    pr.reference_owner_signature(res, n, owner)
                stats['radius_sites'] = stats.get('radius_sites', 0) + 1
                if not (same_date and same_owner):
                    stats['disagreements'].append(dict(cloud=name, points=points, k=k, m=m, rule='margin_r',
                                                       site=orig, date=same_date, owner=same_owner))
                    break
    stats['clouds'] += 1
    stats['delayed'] += delayed


def fixtures(binary, work, stats):
    """Faits exacts graves : deux triangles a k = 2, cinq points sans percolation (regles carree et en rayon),
    continuite {0,2,4}, proprietaire au plateau exact (quatre sites), temoin des huit sites."""
    facts = []
    data = run_export(binary, EQUILATERAL, work, 'equilateral', 2)
    for m, expected in ((3, [[0, 1, 2], [3, 4, 5]]), (1, [[0, 1], [4, 5]])):
        u = native_ultrametrics(data, 2, m)['margin' if m > 1 else 'margin1']
        got = [sorted(b) for b in pr.blocks_at(u, Fraction(1))]
        facts.append(dict(fixture='deux_triangles_k2_m%d' % m, blocks=got, ok=got == expected))
    data = run_export(binary, FIVE, work, 'five', 2)
    u = native_ultrametrics(data, 2, 3)['margin']
    got = [sorted(b) for b in pr.blocks_at(u, Fraction(35))]
    facts.append(dict(fixture='cinq_points_k2_m3', blocks=got, entry0=str(u[0][0]),
                      ok=got == [[1, 2], [3, 4]] and u[0][0] == 36))
    # Regle retenue H^r_3 (rayon), memes faits : ABC | DEF au rayon 1 ; cinq points : {1,2}, {3,4} au rayon
    # sqrt(35), le point 0 entre exactement au rayon 6.
    data = run_export(binary, EQUILATERAL, work, 'equilateral_r', 2)
    order = data['orders'][2]
    hang = prad.hang_margin_radius(order, 3)
    got = radius_blocks(order, hang, data['ids'].tolist(), 1)
    facts.append(dict(fixture='deux_triangles_rayon_k2_m3', blocks=got, ok=got == [[0, 1, 2], [3, 4, 5]]))
    data = run_export(binary, FIVE, work, 'five_r', 2)
    order = data['orders'][2]
    hang = prad.hang_margin_radius(order, 3)
    ids = data['ids'].tolist()
    got = radius_blocks(order, hang, ids, 35)
    entry0 = hang.values[ids.index(0)]
    facts.append(dict(fixture='cinq_points_rayon_k2_m3', blocks=got, entry0=str(entry0),
                      ok=got == [[1, 2], [3, 4]] and entry0.cmp_sqrt(Fraction(36)) == 0))
    medians = []
    for d in (0, 1):
        data = run_export(binary, [(0, 0, 0), (2000, 0, 0), (4000 + d, 0, 0)], work, 'line%d' % d, 2)
        medians.append(native_ultrametrics(data, 2, 1)['margin1'][1][1])
    facts.append(dict(fixture='continuite_024', entries=[str(x) for x in medians],
                      ok=medians == [Fraction(4000000), Fraction(4001000)]))
    # Temoin de l'auditeur (hm_followup_20261003) : un point de coeur d'un amas qualifie entre apres la fusion
    # parasite F = 12 ; H^r_3 le fait entrer a sqrt(250) - 5/2, borne correcte e <= t' + d_k / 2 avec t' = 10.
    data = run_export(binary, PLATEAU, work, 'plateau', 2)
    order = data['orders'][2]
    hang = prad.hang_margin_radius(order, 1)
    ids = data['ids'].tolist()
    x = ids.index(0)
    owner = bench_owner_signature(order, int(hang.owner[x]), ids)
    facts.append(dict(fixture='plateau_proprietaire_k2_m1', entry=str(hang.values[x]), owner=[str(owner[0]), sorted(owner[1])],
                      ok=hang.values[x].cmp_sqrt(Fraction(8)) == 0 and
                      owner == (Fraction(8), frozenset([0, 1, 3]))))
    data = run_export(binary, EIGHT, work, 'eight', 2)
    order = data['orders'][2]
    hang = prad.hang_margin_radius(order, 3)
    x = data['ids'].tolist().index(0)
    expected = prad.RValue(Fraction(100), Fraction(250), Fraction(625, 4))
    facts.append(dict(fixture='huit_sites_retard_qualifie', entry=str(hang.values[x]),
                      ok=hang.values[x].cmp(expected) == 0))
    stats['fixtures'] = facts
    return all(f['ok'] for f in facts)


def arithmetic_fixtures(stats):
    """Contre-gardes exactes de l'auditeur (recu hm_review_20261003) sur la comparaison des dates en rayon :
    egalite 5 sqrt 2 a six radicandes differents, filtre flottant a annulation ; puis 3 000 comparaisons
    3 contre 3 contre decimal a 200 chiffres."""
    import decimal
    facts = []
    a = prad.RValue(Fraction(2), Fraction(162), Fraction(50))
    b = prad.RValue(Fraction(8), Fraction(98), Fraction(32))
    facts.append(dict(fixture='egalite_5_racine_2', ok=a.cmp(b) == 0 and b.cmp(a) == 0))
    target = Fraction(1, 2) + Fraction(6, 997) + Fraction(1, 2 ** 70)
    witness = prad.RValue(Fraction(1, 4), Fraction(14000000) ** 2, (Fraction(14000000) - Fraction(6, 997)) ** 2)
    levels = ph.Levels.from_fractions([Fraction(0), target * target])
    facts.append(dict(fixture='filtre_annulation', ok=witness.cmp_level(levels, 1) == -1))
    ctx = decimal.Context(prec=200)

    def dec(v):
        with decimal.localcontext(ctx):  # racines ET sommes a 200 chiffres
            root = lambda f: (decimal.Decimal(f.numerator) / decimal.Decimal(f.denominator)).sqrt()
            return root(v.t) + root(v.m) - root(v.q)
    rng = random.Random(20261003)
    wrong = 0
    for _ in range(3000):
        vals = []
        for _ in range(2):
            base = Fraction(rng.randint(1, 50), rng.randint(1, 9))
            pick = lambda: base * Fraction(rng.randint(1, 12)) ** 2 if rng.random() < 0.5 else \
                Fraction(rng.randint(1, 10 ** rng.randint(1, 30)), rng.randint(1, 10 ** rng.randint(1, 12)))
            vals.append(prad.RValue(pick(), pick(), pick()))
        with decimal.localcontext(ctx):
            gap = dec(vals[0]) - dec(vals[1])
        expect = 0 if abs(gap) < decimal.Decimal(10) ** -150 else (1 if gap > 0 else -1)
        wrong += vals[0].cmp(vals[1]) != expect
    facts.append(dict(fixture='trois_contre_trois_decimal', comparisons=3000, wrong=wrong, ok=wrong == 0))
    # Deux routines exactes ecrites separement (oracle : pr.two_roots_sign ; banc : prad.sqrt_cmp2) sur 4 000
    # comparaisons deux racines contre deux, dont des egalites construites dans une meme classe de carres.
    wrong = equal = 0
    for _ in range(4000):
        base = Fraction(rng.randint(1, 30), rng.randint(1, 7))
        if rng.random() < 0.5:
            u, v, w = (Fraction(rng.randint(0, 9)) for _ in range(3))
            if u + v < w:
                u, w = w, u
            x = u + v - w  # sqrt(b u^2) + sqrt(b v^2) = sqrt(b w^2) + sqrt(b x^2)
            args = (base * u * u, base * v * v, base * w * w, base * x * x)
        else:
            args = tuple(base * Fraction(rng.randint(0, 12)) ** 2 if rng.random() < 0.5 else
                         Fraction(rng.randint(0, 10 ** rng.randint(1, 20)), rng.randint(1, 10 ** rng.randint(1, 9)))
                         for _ in range(4))
        a = pr.two_roots_sign(*args)
        equal += a == 0
        wrong += a != prad.sqrt_cmp2(*args)
    facts.append(dict(fixture='oracle_contre_banc_deux_racines', comparisons=4000, equalities=equal, wrong=wrong,
                      ok=wrong == 0 and equal >= 1000))
    stats['fixtures'].extend(facts)
    return all(f['ok'] for f in facts)


class Mutant(object):
    """Mutant causal de la regle retenue, applique en place puis retire (porte en Python, sans --inject natif)."""

    def __init__(self, name):
        self.name = name
        self.saved = []

    def patch(self, owner, attr, value):
        self.saved.append((owner, attr, getattr(owner, attr)))
        setattr(owner, attr, value)

    def __enter__(self):
        hang = prad.hang_margin_radius
        if self.name == 'qualification_decalee':  # seuil m - 1 au lieu de m
            self.patch(prad, 'hang_margin_radius', lambda order, m, rule='margin_r': hang(order, max(1, m - 1), rule))
        elif self.name == 'marge_carree':  # regression Q_1 : marge mesuree en niveau carre
            def squared(order, m, rule='margin_r'):
                out = hang(order, m, rule)
                sq = ph.hang_margin(order, m)
                for i in range(order.n):
                    out.values[i] = prad.RValue(Fraction(sq.value(i)))
                return out
            self.patch(prad, 'hang_margin_radius', squared)
        elif self.name == 'sans_marge':  # premiere couverture qualifiee (regle first, discontinue)
            def first(order, m, rule='margin_r'):
                out = hang(order, m, rule)
                p1 = ph.first_points(order, *ph.qualified_starts(order, ph.qualify(order, m)))[1]
                for i in range(order.n):
                    out.values[i] = prad.RValue(out.values[i].t)
                    out.owner[i] = p1[i]
                return out
            self.patch(prad, 'hang_margin_radius', first)
        elif self.name == 'coupe_ouverte':  # egalite date = naissance tranchee comme stricte
            cmp_level = prad.RValue.cmp_level
            self.patch(prad.RValue, 'cmp_level',
                       lambda value, levels, rank: cmp_level(value, levels, rank) or -1)
        else:
            raise ValueError(self.name)
        return self

    def __exit__(self, *exc):
        for owner, attr, value in reversed(self.saved):
            setattr(owner, attr, value)
        return False


MUTANTS = ('qualification_decalee', 'marge_carree', 'sans_marge', 'coupe_ouverte')


def run_mutants(binary, work, stats, seed):
    """Chaque mutant doit etre tue : desaccord avec l'oracle, fixture fausse, ou invariant interne leve."""
    out = {}
    for name in MUTANTS:
        local = dict(clouds=0, comparisons=0, delayed=0, disagreements=[], fixtures=[])
        how = []
        try:
            with Mutant(name):
                if not fixtures(binary, work / name, local):
                    how.append('fixtures:' + ','.join(f['fixture'] for f in local['fixtures'] if not f['ok']))
                rng = random.Random(seed)
                clouds = [EQUILATERAL, FIVE, EIGHT, PLATEAU] + [random_cloud(rng) for _ in range(MUTANT_CLOUDS)]
                for index, cloud in enumerate(clouds):
                    compare_cloud(binary, cloud, work / name, 'm%03d' % index, local)
                    if local['disagreements']:
                        how.append('desaccord:%s' % local['disagreements'][0]['cloud'])
                        break
        except ph.ExportError as error:
            how.append('invariant:%s' % error)
        out[name] = dict(killed=bool(how), how=how[:2])
    stats['mutants'] = out
    return sum(v['killed'] for v in out.values())


def random_cloud(rng):
    n = rng.randint(4, 9)
    side = rng.choice([3, 4, 6, 10, 40])
    scale = rng.choice([1, 7, 1000])
    pts = set()
    while len(pts) < n:
        pts.add((rng.randrange(side) * scale, rng.randrange(side) * scale, rng.choice([0, rng.randrange(side) * scale])))
    pts = sorted(pts)
    rng.shuffle(pts)
    return pts


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--export', required=True, type=Path)
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--seconds', type=float, default=240.0)
    parser.add_argument('--seed', type=int, default=20261003)
    parser.add_argument('--mirror', type=Path, help='seconde copie du verdict (collectee avec la session)')
    args = parser.parse_args()
    if not args.export.is_file():
        print('refus : exportateur absent', file=sys.stderr)
        return 2
    args.work.mkdir(parents=True, exist_ok=True)
    stats = dict(clouds=0, comparisons=0, delayed=0, disagreements=[], fixtures=[])
    started = time.monotonic()
    fixtures_ok = fixtures(args.export, args.work, stats)
    fixtures_ok = arithmetic_fixtures(stats) and fixtures_ok
    # Nuages a egalites exactes (triangles equilateraux, cinq points, huit sites) : dates et proprietaires compares
    # site par site, ordres 1 a 4, comme tout nuage aleatoire (recoupe e8 du workflow aux egalites exactes).
    for name, cloud in (('fixe_triangles', EQUILATERAL), ('fixe_cinq', FIVE), ('fixe_huit', EIGHT),
                        ('fixe_plateau', PLATEAU)):
        compare_cloud(args.export, cloud, args.work, name, stats)
    rng = random.Random(args.seed)
    index = 0
    while time.monotonic() - started < args.seconds:
        compare_cloud(args.export, random_cloud(rng), args.work, 'c%05d' % index, stats)
        index += 1
    killed = run_mutants(args.export, args.work / 'mutants', stats, args.seed + 1)
    floors_ok = all(stats[key] >= value for key, value in FLOORS.items() if key not in ('fixtures', 'mutants')) and \
        len(stats['fixtures']) >= FLOORS['fixtures'] and killed >= FLOORS['mutants']
    verdict = 'desaccord' if stats['disagreements'] else ('conforme' if fixtures_ok and floors_ok else 'plancher')
    stats.update(verdict=verdict, floors=FLOORS, seconds=time.monotonic() - started, seed=args.seed,
                 disagreements=stats['disagreements'][:20])
    text = json.dumps(stats, indent=1, sort_keys=True, default=str) + '\n'
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(text)
    if args.mirror:
        args.mirror.parent.mkdir(parents=True, exist_ok=True)
        args.mirror.write_text(text)
    print('points_gate_verdict %s nuages%d comparaisons%d retardes%d fixtures%d/%d mutants%d/%d' % (
        verdict, stats['clouds'], stats['comparisons'], stats['delayed'],
        sum(f['ok'] for f in stats['fixtures']), len(stats['fixtures']), killed, len(MUTANTS)))
    return {'conforme': 0, 'desaccord': 1}.get(verdict, 3)


if __name__ == '__main__':
    raise SystemExit(main())
