"""Pouvoir de discrimination du juge T2 de HEAD (tests/oracle/test_tower_oracle.py, afb081774).

On ne mute PAS le moteur : on prend des dumps vrais de mhgp10_tower (acceptes par le juge), on les rend FAUX par
des operateurs systematiques, et on rejoue EXACTEMENT les comparaisons du juge (fonctions parse, gamma_sweep, top,
vertical_check importees du fichier de HEAD ; corps de check() recopie a l'identique, sauf l'appel du binaire).
Un dump faux accepte = angle mort du juge.

Operateurs :
  V_desc  : lower[u] remplace par un enfant de lower[u] (noeud mort au niveau de u : contraire au contrat tower.hpp:108)
  V_autre : lower[u] remplace par un autre noeud vivant de l'ordre k-1 au niveau de u (autre composante)
  BIN     : multifusion de >= 3 enfants binarisee au meme niveau exact (contraire au contrat « plateau atomique »)
  ATT     : attache deplacee vers un enfant absorbe exactement au niveau d'entree (convention fermee, AT1)
  SWAP    : deux feuilles sans point de parents differents echangees (arbre de fusion NON isomorphe a l'original)
Usage : python3 juge_tour_aveugle.py BUILD SRC_V10 [nb_nuages] [K]
"""
import copy
import json
import os
import random
import subprocess
import sys
import tempfile
from collections import Counter

build, src = sys.argv[1], sys.argv[2]
NCLOUDS = int(sys.argv[3]) if len(sys.argv) > 3 else 6
K = int(sys.argv[4]) if len(sys.argv) > 4 else 5
sys.path.insert(0, os.path.join(src, 'reference'))
sys.path.insert(0, os.path.join(src, 'tests', 'oracle'))
import hgp10_ref as R  # noqa: E402
import test_tower_oracle as T  # noqa: E402
from test_catalogue_oracle import clouds  # noqa: E402


def judge(orders, P, Kc, sweeps):
    """Corps de test_tower_oracle.check() (HEAD, lignes 127-154), a l'identique, sur un dump deja lu."""
    idx = {p: i for i, p in enumerate(P)}
    for k in range(1, min(Kc, len(P)) + 1):
        o = orders[k]
        nodes = o['nodes']
        for a, op, cl in sweeps[k]:
            for is_closed, (want_n, want_part) in ((True, cl), (False, op)):
                ok = (lambda l: l <= a) if is_closed else (lambda l: l < a)
                alive = sum(1 for v, (par, lv, _low) in enumerate(nodes)
                            if ok(lv) and (par < 0 or not ok(nodes[par][1])))
                if alive != want_n:
                    return 'composantes k=%d' % k
                groups = {}
                for pt, v, e in o['points']:
                    if ok(e):
                        groups.setdefault(T.top(nodes, v, a, is_closed), set()).add(idx[pt])
                mine = sorted((frozenset(g) for g in groups.values()), key=lambda s: sorted(s))
                if mine != want_part:
                    return 'partition k=%d' % k
    for k in range(2, min(Kc, len(P)) + 1):
        levels = sorted({lv for _, lv, _ in orders[k]['nodes']} | {e for _, _, e in orders[k]['points']})
        err, _c = T.vertical_check(orders, k, P, levels)
        if err:
            return err
    return None


def canon(o):
    """Forme canonique de l'arbre annote (niveaux exacts, points attaches avec leur entree), a isomorphisme pres."""
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
    """Engendre (operateur, description, dump mute)."""
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
                    yield 'V_desc', 'k=%d u=%d' % (k, u), m
                others = [w for w in alive_at(down, lv) if w != low]
                if others:
                    m = copy.deepcopy(orders)
                    m[k]['nodes'][u] = (par, lv, others[0])
                    yield 'V_autre', 'k=%d u=%d' % (k, u), m
        for v, ch in kids.items():
            if v < 0 or len(ch) < 3:
                continue
            m = copy.deepcopy(orders)
            nn = len(nodes)
            par, lv, low = nodes[v]
            m[k]['nodes'].append((v, lv, low))  # noeud intermediaire de meme niveau exact, enfant de la fusion
            for c in ch[:2]:
                cp, cl, clow = m[k]['nodes'][c]
                m[k]['nodes'][c] = (nn, cl, clow)
            yield 'BIN', 'k=%d fusion=%d' % (k, v), m
        for i, (pt, v, e) in enumerate(o['points']):
            for c in kids.get(v, []):
                if nodes[v][1] == e and nodes[c][1] <= e:  # c absorbe exactement au niveau d'entree
                    m = copy.deepcopy(orders)
                    m[k]['points'][i] = (pt, c, e)
                    yield 'ATT', 'k=%d point=%d' % (k, i), m
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
                    continue  # reetiquetage isomorphe : pas un faux
                yield 'SWAP', 'k=%d feuilles=%d,%d' % (k, b1, b2), m
                done += 1
                if done >= 40:
                    break
            if done >= 40:
                break


def main():
    rnd = random.Random(20260929)
    total, accepted = Counter(), Counter()
    examples = {}
    with tempfile.TemporaryDirectory() as tmp:
        for t, P in enumerate(clouds(NCLOUDS, rnd)):
            P = P[:12]
            srcf = os.path.join(tmp, 'in.u32le')
            with open(srcf, 'wb') as f:
                for p in P:
                    for v in p:
                        f.write(int(v).to_bytes(4, 'little'))
            dump = os.path.join(tmp, 'tower.txt')
            r = subprocess.run([os.path.join(build, 'mhgp10_tower'), srcf, '--k=%d' % K, '--threads=2',
                                '--dump=' + dump], capture_output=True, text=True)
            if r.returncode != 0:
                print('refus', t, r.stdout.strip())
                continue
            orders = T.parse(dump)
            sweeps = {k: T.gamma_sweep(P, k) for k in range(1, min(K, len(P)) + 1)}
            base = judge(orders, P, K, sweeps)
            if base is not None:
                print('TEMOIN REFUSE nuage %d : %s' % (t, base))
                return 3
            for op, desc, m in mutations(orders, K, len(P)):
                total[op] += 1
                verdict = judge(m, P, K, sweeps)
                if verdict is None:
                    accepted[op] += 1
                    examples.setdefault(op, 'nuage %d (%s, n=%d) %s' % (t, ['generique', 'grille 0..3', 'coplanaire', 'grille 2'][t % 4], len(P), desc))
            print('nuage %d n=%d : cumul %s' % (t, len(P), {op: '%d/%d acceptes' % (accepted[op], total[op]) for op in sorted(total)}), flush=True)
    print(json.dumps({'K': K, 'nuages': NCLOUDS,
                      'dumps_faux': dict(total), 'acceptes_par_le_juge_HEAD': dict(accepted),
                      'exemples_acceptes': examples}, indent=1, sort_keys=True))
    return 0


sys.exit(main())
