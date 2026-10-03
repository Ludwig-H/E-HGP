#!/usr/bin/env python3
"""Verification adverse : les 125 jugements ancres, avec MON FULL (indep_full.py) et MES regles.

Juge : deux juges en parallele, qui doivent concorder sur chaque jugement :
  - V10 : ver.juger de la v10 (semantique condensee, CIBLES_REVISEES.md section 1.1), inchange ;
  - MOI : reecriture de la section 1.1 depuis son texte (mon_conforme), memes points de controle.
Cellules ancrees recopiees de cellules.py v10 (T0@2-3 x 4, Q1@3, Q1bis@2 'triangles', Q2@2, Q3@2-6, Q-Pi2@9 'aucun',
Q4@2-3 'q4strict'). Usage : python3 -B cells.py [regles] ; ecrit recus/cells_<regles>.json.
"""
import sys
sys.dont_write_bytecode = True
import json  # noqa: E402
import time  # noqa: E402
import hashlib  # noqa: E402
from fractions import Fraction as Fr  # noqa: E402

import indep_full as I  # noqa: E402
from indep_full import R  # noqa: E402

VER = '/workspaces/E-HGP/build/v10-verrou-points/juge_final/verif_echelle_relative'
if VER not in sys.path:
    sys.path.insert(0, VER)
import ver  # noqa: E402  (juge v10, lecture seule ; n'importe que fractions et vrad)

V2 = '/workspaces/E-HGP/build/v10-verrou-points/juge_final/cibles/fixtures_catalogue_v2.json'

CELLS = [('T0', 'T0_P1_aretes_courtes__mcs2-3', [2, 3], None),
         ('T0', 'T0_P2_pont_court__mcs2-3', [2, 3], None),
         ('T0', 'T0_S_plan_egalites__mcs2-3', [2, 3], None),
         ('T0', 'T0_S_3D_equilateral__mcs2-3', [2, 3], None),
         ('Q1', 'Q1_T1_1700_K2__mcs3', [3], None),
         ('Q1bis', 'Q1_T1_1700_K2__mcs3', [2], 'triangles'),
         ('Q2', 'Q2_S17_K2__mcs2', [2], None),
         ('Q3', 'Q3_filament_K2__mcs2-6', [2, 3, 4, 5, 6], None),
         ('QPi2', 'Q3_filament_K2__mcs9', [9], 'aucun'),
         ('Q4', 'Q4_T6_chaine_K3__mcs2-3', [2, 3], 'q4strict')]


def build(rule, T, mcs):
    K = T.K
    if rule == 'core':
        return I.ultra_sq(T, I.rule_core(T)), 'sq'
    if rule == 'cover':
        return I.ultra_sq(T, I.rule_first(T, 1)), 'sq'
    if rule == 'first':
        return I.ultra_sq(T, I.rule_first(T, K + 1)), 'sq'
    if rule == 'H1':
        return I.ultra_sq(T, I.rule_margin_sq(T, 1)), 'sq'
    if rule == 'Hk1':
        return I.ultra_sq(T, I.rule_margin_sq(T, K + 1)), 'sq'
    if rule == 'Hmcs':
        return I.ultra_sq(T, I.rule_margin_sq(T, max(K + 1, mcs))), 'sq'
    if rule == 'H1r':
        return I.ultra_r(T, I.rule_margin_r(T, 1)), 'r'
    if rule == 'Hk1r':
        return I.ultra_r(T, I.rule_margin_r(T, K + 1)), 'r'
    if rule == 'P2r':
        return I.ultra_r(T, I.rule_margin_r(T, 1, 2)), 'r'
    if rule == 'P2sq':
        return I.ultra_sq(T, I.rule_margin_sq(T, 1, 2)), 'sq'
    raise I.VerifErreur('regle inconnue ' + rule)


DEPEND_MCS = {'Hmcs'}


def naissance_groupe(T, grp):
    best = None
    for w in range(len(T.h)):
        if all(T.cov[y][w] == T.h[w] for y in grp) and (best is None or T.h[w] < best):
            best = T.h[w]
    return best


def mon_conforme(part, spec, ix, mcs):
    """Section 1.1 de CIBLES_REVISEES.md, reecrite depuis le texte."""
    blocks = [[ix[y] for y in b] for b in spec['blocks']]
    tol = set(ix[y] for y in spec.get('tolerance', []))
    free = set(ix[y] for y in spec.get('libres_vrais', []))
    clusters = [set(b) for b in part if len(b) >= mcs]
    lab = {}
    for j, b in enumerate(blocks):
        for y in b:
            lab[y] = j
    for cl in clusters:
        labs = set()
        for y in cl:
            if y in free:
                continue
            if y not in lab:
                return False
            labs.add(lab[y])
        if len(labs) > 1:
            return False
    where = {}
    for i, cl in enumerate(clusters):
        for y in cl:
            where[y] = i
    for b in blocks:
        bb = [y for y in b if y not in free]
        req = [y for y in bb if y not in tol]
        homes = set(where[y] for y in bb if y in where)
        if len(homes) > 1:
            return False
        if len(req) >= mcs:
            if any(y not in where for y in req):
                return False
        elif homes and req:
            if any(y not in where for y in req):
                return False
    return True


def mon_juger(h, spec, ix, mcs):
    a = R.rac(Fr(spec['r2'][0]))
    b2 = str(spec['r2'][1]).strip()
    b = None if b2 in ('inf', '+inf') else R.rac(Fr(b2))
    closed_right = spec.get('bounds', '[]')[1] == ']'
    pts = [a]
    for e in h.rayons_changement():
        if e.cmp(a) <= 0:
            continue
        if b is not None:
            c = e.cmp(b)
            if c > 0 or (c == 0 and not closed_right):
                break
        pts.append(e)
    for r in pts:
        if not mon_conforme(h.partition(r), spec, ix, mcs):
            return False, float(r)
    return True, None


def run(rules):
    v2 = json.load(open(V2))
    by = {e['name']: e for e in v2['fixtures']}
    score = {r: {} for r in rules}
    disagreements = []
    detail = {r: [] for r in rules}
    fulls = {}
    t0 = time.time()
    for code, name, mcss, mode in CELLS:
        e = by[name]
        variants = [('base', e['points'], e['target'])] + [(v['name'], v['points'], v['target']) for v in e.get('variants', [])]
        for vn, pts, targets in variants:
            noms = list(pts)
            P = [tuple(pts[x]) for x in noms]
            key = (tuple(P), e['K'])
            if key not in fulls:
                fulls[key] = I.Full(P, e['K'])
            T = fulls[key]
            ix = {x: i for i, x in enumerate(noms)}
            for rule in rules:
                cache = {}
                for mcs in mcss:
                    k = mcs if rule in DEPEND_MCS else 0
                    if k not in cache:
                        U, kind = build(rule, T, mcs)
                        cache[k] = I.Hier(U, kind)
                    h = cache[k]
                    if mode == 'triangles':
                        bt = max(naissance_groupe(T, [ix[y] for y in 'ABC']), naissance_groupe(T, [ix[y] for y in 'DEF']))
                        cs = []
                        for t in targets:
                            t2 = dict(t)
                            if bt > Fr(t['r2'][0]):
                                t2['r2'] = [str(bt), t['r2'][1]]
                            hi = t2['r2'][1]
                            if hi not in ('inf', '+inf') and Fr(hi) <= Fr(t2['r2'][0]):
                                continue
                            cs.append(t2)
                    elif mode == 'aucun':
                        cs = [{'r2': t['r2'], 'bounds': t.get('bounds', '[]'), 'blocks': [], 'tolerance': [],
                               'libres_vrais': [], 'bruit': list(noms)} for t in targets]
                    else:
                        cs = targets
                    res = []
                    if mode == 'q4strict' and mcs <= 3:
                        bt = max(naissance_groupe(T, [ix[y] for y in ('C', 'P', 'Q', 'R')]),
                                 naissance_groupe(T, [ix[y] for y in ('D', 'P2', 'Q2', 'R2')]))
                        part = h.partition(R.rac(bt))
                        cmd = {ix['C'], ix['m'], ix['D']}
                        ok = not any(len(set(c) & cmd) >= 2 for c in part if len(c) >= mcs)
                        res.append(('q4strict', ok, ok))
                    for ic, t in enumerate(cs):
                        ok10, _v = ver.juger(h, t, ix, mcs)
                        okm, _r = mon_juger(h, t, ix, mcs)
                        if ok10 != okm:
                            disagreements.append((rule, code, vn, mcs, ic))
                        res.append((ic, ok10, okm))
                    for item in res:
                        s = score[rule].setdefault(code, [0, 0])
                        s[0] += 1 if item[1] else 0
                        s[1] += 1
                        detail[rule].append((code, name, vn, mcs, item[0], item[1]))
    tot = {r: [sum(v[0] for v in score[r].values()), sum(v[1] for v in score[r].values())] for r in rules}
    fp = {r: hashlib.sha256(json.dumps(detail[r]).encode()).hexdigest()[:16] for r in rules}
    return {'score': score, 'total': tot, 'desaccords_juges': disagreements, 'empreintes_miennes': fp,
            'detail': detail, 'secondes': round(time.time() - t0, 1)}


if __name__ == '__main__':
    rules = (sys.argv[1] if len(sys.argv) > 1 else 'core,cover,first,H1,Hk1,Hmcs').split(',')
    out = run(rules)
    with open('recus/cells_%s.json' % '_'.join(rules), 'w') as f:
        json.dump(out, f, indent=1, default=str)
    for r in rules:
        print(r, out['total'][r], {c: '%d/%d' % tuple(v) for c, v in out['score'][r].items()})
    print('desaccords juge v10 / mon juge :', len(out['desaccords_juges']), out['desaccords_juges'][:5])
    print('secondes', out['secondes'])
