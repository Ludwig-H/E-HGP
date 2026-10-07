"""Les memes dumps FAUX que juge_tour_aveugle.py, presentes cette fois au juge de la tour de l'arbre integre R2
(hors depot : build/v10-integration-r2/src, commit 865f5e6 ; dump lu avec --dump-births, temoins de naissance).
Question : le juge R2 ferme-t-il les angles morts du juge de HEAD ?
Usage : python3 juge_r2_contre_faux.py BUILD_R2 SRC_R2_V10 [nb_nuages] [K]
"""
import copy
import json
import os
import random
import sys
import tempfile
from collections import Counter

build, src = sys.argv[1], sys.argv[2]
NCLOUDS = int(sys.argv[3]) if len(sys.argv) > 3 else 6
K = int(sys.argv[4]) if len(sys.argv) > 4 else 5
sys.path.insert(0, os.path.join(src, 'reference'))
sys.path.insert(0, os.path.join(src, 'tests', 'oracle'))
import test_tower_oracle as T  # noqa: E402  (juge R2)
from test_catalogue_oracle import clouds  # noqa: E402


def canon(o):
    nodes = o['nodes']
    kids = {}
    for v, (par, _lv, _low) in enumerate(nodes):
        kids.setdefault(par, []).append(v)
    att = {}
    for pt, v, e in o['points']:
        att.setdefault(v, []).append((pt, e))
    def rec(v):
        return (nodes[v][1], tuple(sorted(att.get(v, []))), tuple(sorted(rec(c) for c in kids.get(v, []))))
    return tuple(sorted(rec(r) for r in kids.get(-1, [])))


def alive_at(nodes, a):
    return [v for v, (par, lv, _l) in enumerate(nodes) if lv <= a and (par < 0 or nodes[par][1] > a)]


def mutations(orders, Kc, n):
    for k in range(1, min(Kc, n) + 1):
        o = orders[k]
        nodes = o['nodes']
        kids = {}
        for v, (par, _lv, _low) in enumerate(nodes):
            kids.setdefault(par, []).append(v)
        attached = Counter(v for _pt, v, _e in o['points'])
        if k >= 2:
            down = orders[k - 1]['nodes']
            dkids = {}
            for v, (par, _lv, _low) in enumerate(down):
                dkids.setdefault(par, []).append(v)
            for u, (par, lv, low) in enumerate(nodes):
                if low < 0:
                    continue
                for c in dkids.get(low, [])[:1]:
                    m = copy.deepcopy(orders)
                    m[k]['nodes'][u] = (par, lv, c)
                    yield 'V_desc', m
                others = [w for w in alive_at(down, lv) if w != low]
                if others:
                    m = copy.deepcopy(orders)
                    m[k]['nodes'][u] = (par, lv, others[0])
                    yield 'V_autre', m
        for v, ch in kids.items():
            if v < 0 or len(ch) < 3:
                continue
            m = copy.deepcopy(orders)
            nn = len(nodes)
            par, lv, low = nodes[v]
            m[k]['nodes'].append((v, lv, low))
            m[k]['ids'].append(nn)
            m[k]['head'] = (m[k]['head'][0] + 1, m[k]['head'][1])
            for c in ch[:2]:
                cp, cl, clow = m[k]['nodes'][c]
                m[k]['nodes'][c] = (nn, cl, clow)
            yield 'BIN', m
        for i, (pt, v, e) in enumerate(o['points']):
            for c in kids.get(v, []):
                if nodes[v][1] == e and nodes[c][1] <= e:
                    m = copy.deepcopy(orders)
                    m[k]['points'][i] = (pt, c, e)
                    yield 'ATT', m
                    break
        leaves = [v for v in range(len(nodes)) if v not in kids and attached[v] == 0 and nodes[v][0] >= 0]
        base = canon(o)
        done = 0
        for i in range(len(leaves)):
            for j in range(i + 1, len(leaves)):
                b1, b2 = leaves[i], leaves[j]
                m1, m2 = nodes[b1][0], nodes[b2][0]
                if m1 == m2 or nodes[b1][1] > nodes[m2][1] or nodes[b2][1] > nodes[m1][1]:
                    continue
                m = copy.deepcopy(orders)
                m[k]['nodes'][b1] = (m2, nodes[b1][1], nodes[b1][2])
                m[k]['nodes'][b2] = (m1, nodes[b2][1], nodes[b2][2])
                if canon(m[k]) == base:
                    continue
                yield 'SWAP', m
                done += 1
                if done >= 40:
                    break
            if done >= 40:
                break


def main():
    rnd = random.Random(20260929)
    total, accepted, reasons = Counter(), Counter(), {}
    with tempfile.TemporaryDirectory() as tmp:
        for t, P in enumerate(clouds(NCLOUDS, rnd)):
            P = P[:12]
            err, orders = T.run_tower(os.path.join(build, 'mhgp10_tower'), P, K, tmp)
            if err:
                print('refus', t, err)
                continue
            ctx = T.Ctx(P)
            base = T.judge(P, orders, K, ctx)
            if base is not None:
                print('TEMOIN REFUSE nuage %d : %s' % (t, base))
                return 3
            for op, m in mutations(orders, K, len(P)):
                total[op] += 1
                verdict = T.judge(P, m, K, ctx)
                if verdict is None:
                    accepted[op] += 1
                else:
                    reasons.setdefault(op, Counter())[verdict.split(':')[0][:60]] += 1
            print('nuage %d n=%d : cumul %s' % (t, len(P), {op: '%d/%d acceptes' % (accepted[op], total[op]) for op in sorted(total)}), flush=True)
    print(json.dumps({'K': K, 'nuages': NCLOUDS, 'dumps_faux': dict(total), 'acceptes_par_le_juge_R2': {op: accepted[op] for op in total},
                      'premiers_motifs_de_refus': {op: c.most_common(3) for op, c in reasons.items()}}, indent=1, sort_keys=True))
    return 0


sys.exit(main())
