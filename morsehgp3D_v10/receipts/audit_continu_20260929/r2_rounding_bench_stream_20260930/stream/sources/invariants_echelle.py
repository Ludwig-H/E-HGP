"""Invariants globaux des regles du second tour, a l'echelle, sur les sorties du binaire (aucun juge O(n^3), lecture
en flux, memoire lineaire en noeuds).

    python3 invariants_echelle.py SRC_R2 BUILD IN.u32le K THREADS [tour]

Avec « tour », seul le dump de la tour est juge (le catalogue a l'echelle a deja ete juge par les invariants globaux du
verificateur du premier tour, scale_judge.py, sur le meme binaire).

Tour (mhgp10_tower --dump-births, entree core), par ordre : identifiants consecutifs, une racine, parent pas plus bas ;
naissance = feuille a temoin de k sites distincts, fusion = au moins deux enfants sans temoin ; AUCUNE fusion enfant
d'une fusion de meme niveau exact (plateau atomique) ; naissances absorbees dans leur plateau comptees (admises) ; bloc
d'attaches complet (chaque site une fois) ; chaque attache au noeud vivant a la coupe fermee de son entree :
niveau(v) <= e < niveau(parent(v)) ; verticales vivantes a la coupe fermee de leur niveau.
Catalogue (mhgp10_catalogue --dump-levels) : la fonction published_check du juge r2 (tests/oracle/
test_catalogue_oracle.py), telle quelle, sur des tranches de 20 000 lignes lues par parse_dump du juge, chaque tranche
reprenant la derniere ligne de la precedente (rangs translates pour qu'elle vaille 0 : la regle du rang dense est
invariante par translation ; la premiere tranche n'est pas translatee). Multiplicites lues dans l'entree.
Temoins (beta) et S* canoniques ne sont pas rejuges ici (juges d'echantillon du premier tour).
Sortie : une ligne JSON de compteurs. Code 0 si aucun ecart, 1 sinon. Python nu, sans assert.
"""
import importlib.util
import json
import os
import struct
import subprocess
import sys
import tempfile
from collections import Counter
from fractions import Fraction

CHUNK = 20000


def load(name, root, base):
    spec = importlib.util.spec_from_file_location(name, os.path.join(root, 'tests', 'oracle', base))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


class Sites:
    def __init__(self, sites):
        self.sites = sites


def check_order(k, par, lev, wit, low, pts, n, down, cnt):
    nn = len(par)
    kids = [0] * nn
    roots = 0
    for v in range(nn):
        p = par[v]
        if p < 0:
            roots += 1
            continue
        if p >= nn or lev[p] < lev[v]:
            return 'k=%d noeud %d : parent' % (k, v)
        kids[p] += 1
    if roots != 1:
        return 'k=%d : %d racines' % (k, roots)
    for v in range(nn):
        p = par[v]
        if kids[v]:
            if wit[v] or kids[v] < 2:
                return 'k=%d fusion %d mal typee' % (k, v)
            if p >= 0 and lev[p] == lev[v]:
                return 'k=%d fusion %d enfant de la fusion %d au meme niveau (plateau binarise)' % (k, v, p)
            cnt['fusions'] += 1
            cnt['fusion_links'] += p >= 0
        else:
            if wit[v] != k:
                return 'k=%d naissance %d : temoin de %d sites' % (k, v, wit[v])
            cnt['births'] += 1
            cnt['plateau_births'] += p >= 0 and lev[p] == lev[v]
    if len(pts) != n or len({s for s, _v, _e in pts}) != n:
        return 'k=%d : bloc d\'attaches de %d lignes pour %d sites' % (k, len(pts), n)
    for s, v, e in pts:
        if not 0 <= v < nn or lev[v] > e:
            return 'k=%d site %s : noeud %d ne apres l\'entree' % (k, s, v)
        p = par[v]
        if p >= 0 and lev[p] <= e:
            return 'k=%d site %s : attache %d non vivante a la coupe fermee %s' % (k, s, v, e)
        cnt['attaches'] += 1
        cnt['attaches_parent_a_l_entree'] += p >= 0 and lev[p] == e
    if down is not None:
        dpar, dlev = down
        for u in range(nn):
            w, a = low[u], lev[u]
            if not 0 <= w < len(dpar) or dlev[w] > a or (dpar[w] >= 0 and dlev[dpar[w]] <= a):
                return 'k=%d verticale %d non vivante a la coupe fermee' % (k, u)
            cnt['verticals'] += 1
            cnt['verticals_same_level'] += dlev[w] == a
    cnt['orders'] += 1
    return None


def tower_stream(path, n, cnt):
    down = None
    cur = None

    def finish(c):
        return check_order(c['k'], c['par'], c['lev'], c['wit'], c['low'], c['pts'], n, down, cnt)
    with open(path) as f:
        for line in f:
            t = line.split()
            if t[0] == 'order':
                if cur is not None:
                    err = finish(cur)
                    if err:
                        return err
                    down = (cur['par'], cur['lev'])
                cur = dict(k=int(t[1]), par=[], lev=[], wit=[], low=[], pts=[])
            elif t[0] == 'node':
                if int(t[1]) != len(cur['par']):
                    return 'k=%d : identifiants non consecutifs' % cur['k']
                cur['par'].append(int(t[2]))
                cur['lev'].append(Fraction(int(t[3]), int(t[4])))
                cur['low'].append(int(t[5]))
                w = t[6:]
                cur['wit'].append(len(w) if len(set(w)) == len(w) else -1)
            else:
                cur['pts'].append(((int(t[1]), int(t[2]), int(t[3])), int(t[4]), Fraction(int(t[5]))))
    return finish(cur) if cur is not None else 'dump vide'


def catalogue_stream(c, path, mult, tmp, cnt):
    sites = list(mult)
    weights = [mult[s] for s in sites]
    ref = Sites(sites)
    ccnt = c.new_counters()
    lines, first, prev, balls = [], True, None, 0

    def flush(chunk_lines, first):
        part = os.path.join(tmp, 'tranche.txt')
        with open(part, 'w') as f:
            f.writelines(chunk_lines)
        got = c.parse_dump(part)
        if not first:
            base = got[0]['rank']
            for b in got:
                b['rank'] -= base
        return c.published_check(ref, weights, got, ccnt)
    with open(path) as f:
        for line in f:
            balls += 1
            lines.append(line)
            if len(lines) == CHUNK:
                err = flush(lines, first)
                if err:
                    return err
                prev = lines[-1]
                lines = [prev]
                first = False
    if len(lines) > (0 if first else 1):
        err = flush(lines, first)
        if err:
            return err
    cnt['balls'] = balls
    cnt['same_level_neighbours'] = ccnt['order_ties']
    return None


def main():
    src, build, inp, K, threads = sys.argv[1], sys.argv[2], sys.argv[3], int(sys.argv[4]), int(sys.argv[5])
    tower_only = len(sys.argv) > 6 and sys.argv[6] == 'tour'
    sys.dont_write_bytecode = True
    c = load('catalogue_r2', src, 'test_catalogue_oracle.py')
    mult = Counter(struct.iter_unpack('<III', open(inp, 'rb').read()))
    cnt = Counter()
    out = dict(entree=os.path.basename(inp), points=sum(mult.values()), sites=len(mult), K=K, threads=threads,
               catalogue=not tower_only)
    err = None
    with tempfile.TemporaryDirectory() as tmp:
        dump = os.path.join(tmp, 'tower.txt')
        r = subprocess.run([os.path.join(build, 'mhgp10_tower'), inp, '--k=%d' % K, '--threads=%d' % threads,
                            '--dump=' + dump, '--dump-births'], capture_output=True, text=True)
        if r.returncode != 0:
            err = 'tour code %d' % r.returncode
        else:
            err = tower_stream(dump, len(mult), cnt)
        os.remove(dump) if os.path.exists(dump) else None
        if err is None and not tower_only:
            dump = os.path.join(tmp, 'cat.txt')
            r = subprocess.run([os.path.join(build, 'mhgp10_catalogue'), inp, '--k=%d' % K, '--threads=%d' % threads,
                                '--dump=' + dump, '--dump-levels'], capture_output=True, text=True)
            err = 'catalogue code %d' % r.returncode if r.returncode else catalogue_stream(c, dump, mult, tmp, cnt)
    out.update(cnt)
    out['ecart'] = err
    print(json.dumps(out, ensure_ascii=False))
    return 0 if err is None else 1


if __name__ == '__main__':
    sys.exit(main())
