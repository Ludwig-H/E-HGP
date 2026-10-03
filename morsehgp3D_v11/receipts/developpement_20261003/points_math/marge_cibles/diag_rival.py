#!/usr/bin/env python3
"""Diagnostic de H_m : pour chaque site, premiere couverture qualifiee t, rival realisant la marge D, date e."""
import json
import sys
sys.dont_write_bytecode = True
import adaptateur as AD  # noqa: E402
CL, ver = AD.CL, AD.ver
from fractions import Fraction  # noqa: E402


def diag(T, m, noms):
    adm = ver.admissibilite(T, m)
    out = []
    for x in range(T.n):
        s = {}
        for v, c in T.cov[x].items():
            a = adm[v]
            if a is None:
                continue
            o = max(c, a)
            if T.death[v] is not None and o >= T.death[v]:
                continue
            s[v] = o
        t = min(s.values())
        o = min(v for v in s if s[v] == t)
        anc_o = set(T.ancetres(o))
        best = (Fraction(0), None)
        for v, sv in s.items():
            if v in anc_o:
                continue
            w = AD._lca(T, o, v)
            if w in (o, v):
                continue
            term = T.birth[w] - sv
            if term > best[0]:
                cov_v = sorted(noms[y] for y in range(T.n) if v in T.cov[y] and T.cov[y][v] <= sv)
                best = (term, {'rival_naissance_r': round(float(T.birth[v]) ** 0.5, 3),
                               'rival_couvre_x_des_r': round(float(sv) ** 0.5, 3),
                               'rejoint_lignee_r': round(float(T.birth[w]) ** 0.5, 3), 'rival_couvre': cov_v})
        e = t + best[0]
        cov_o = sorted(noms[y] for y in range(T.n) if o in T.cov[y] and T.cov[y][o] <= t)
        out.append({'site': noms[x], 't_r': round(float(t) ** 0.5, 3), 'premier_couvre': cov_o,
                    'e_r': round(float(e) ** 0.5, 3), 'rival': best[1]})
    return out


if __name__ == '__main__':
    v2 = json.load(open(CL.V2))
    by = {e['name']: e for e in v2['fixtures']}
    nom, var, mcode = sys.argv[1], sys.argv[2], sys.argv[3]
    e = by[nom]
    pts = e['points'] if var == 'base' else next(v['points'] for v in e['variants'] if v['name'] == var)
    noms = list(pts)
    T, info = CL.full([tuple(pts[x]) for x in noms], e['K'])
    m = 1 if mcode == 'un' else e['K'] + 1
    print('fixture', nom, 'K', e['K'], 'variante', var, 'm', m)
    print('cibles', json.dumps([{k: t[k] for k in ('r2', 'blocks', 'tolerance')} for t in e['target']]))
    for row in diag(T, m, noms):
        print(json.dumps(row))
