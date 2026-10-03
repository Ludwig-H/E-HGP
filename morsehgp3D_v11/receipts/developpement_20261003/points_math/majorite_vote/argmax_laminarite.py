#!/usr/bin/env python3
"""Le vote de la these applique a chaque niveau (argmax par coupe) n'est pas laminaire : recherche d'un temoin exact
minimal. Un temoin est un site x et deux niveaux s1 < s2 tels que le gagnant UNIQUE de l'argmax a s2 n'est pas
l'ancetre vivant a s2 du gagnant unique a s1 : les blocs « sites dont l'argmax est C » ne sont alors pas emboites.

On cherche sur des nuages aleatoires (n <= 7, K = 2 et 3), pour p = 0 et 2, faces 'all' et 'gabriel' ; on garde les
plus petits temoins (n minimal, puis coordonnees les plus petites), puis on verifie la violation de laminarite au
niveau des BLOCS (pas seulement du site) : un bloc a s1 coupe en deux a s2.

Usage : python3 -B argmax_laminarite.py [essais] [graine] > recus/argmax.json
"""
import json
import random
import sys
import time

sys.dont_write_bytecode = True
import arbre  # noqa: E402
import regles as RG  # noqa: E402


def blocks_argmax(d, tree, k, data, s):
    """Blocs de la hierarchie 'argmax a chaque niveau' au niveau s : sites groupes par gagnant unique."""
    groups = {}
    for x in range(tree.n):
        cr, _r = RG.vote_credits(d, k, x, data)
        m = {}
        for o, v, w in cr:
            if o <= s:
                C = tree.anc(v, s)
                m[C] = m.get(C, 0) + w
        if not m:
            continue
        best = max(m.values())
        win = [C for C, mm in m.items() if mm == best]
        if len(win) == 1:
            groups.setdefault(win[0], []).append(x)
    return dict((C, sorted(g)) for C, g in groups.items())


def search(trials, seed):
    rng = random.Random(seed)
    found = []
    t0 = time.time()
    for it in range(trials):
        n = rng.randint(4, 7)
        side = rng.choice([4, 6, 10])
        pts = set()
        while len(pts) < n:
            pts.add((rng.randrange(side), rng.randrange(side), rng.choice([0, 0, rng.randrange(side)])))
        pts = sorted(pts)
        for k in (2, 3):
            if k >= n:
                continue
            d, res, tree = arbre.v11_tree(pts, k)
            for p in (0, 2):
                for faces in ('all', 'gabriel'):
                    data = RG.face_data(d, k, p, faces)
                    for x in range(n):
                        tr = RG.argmax_trace(d, tree, k, x, data)
                        v = RG.argmax_violation(tree, tr)
                        if v is None:
                            continue
                        s1, c1, s2, c2 = v
                        b1 = blocks_argmax(d, tree, k, data, s1)
                        b2 = blocks_argmax(d, tree, k, data, s2)
                        # violation au niveau des blocs : un bloc de s1 rencontre deux blocs de s2
                        where = {}
                        for C, g in b2.items():
                            for y in g:
                                where[y] = C
                        split = [g for g in b1.values() if len(set(where.get(y) for y in g if y in where)) > 1]
                        found.append(dict(n=n, k=k, p=p, faces=faces, points=pts, site=x, s1=str(s1), s2=str(s2),
                                          gagnant_s1=c1, gagnant_s2=c2, ancetre_de_c1_a_s2=tree.anc(c1, s2),
                                          blocs_s1=sorted(b1.values()), blocs_s2=sorted(b2.values()),
                                          bloc_coupe=split, size=sum(sum(abs(c) for c in q) for q in pts)))
        if len(found) > 400:
            break
    found.sort(key=lambda f: (0 if f['bloc_coupe'] else 1, f['n'], f['size']))
    return found, time.time() - t0


def main():
    trials = int(sys.argv[1]) if len(sys.argv) > 1 else 300
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 3
    found, secs = search(trials, seed)
    by = {}
    for f in found:
        key = '%s_p%d_k%d' % (f['faces'], f['p'], f['k'])
        by[key] = by.get(key, 0) + 1
    print(json.dumps(dict(essais=trials, graine=seed, secondes=round(secs, 1), temoins=len(found), par_variante=by,
                          minimaux=found[:6]), indent=1, default=str))


if __name__ == '__main__':
    main()
