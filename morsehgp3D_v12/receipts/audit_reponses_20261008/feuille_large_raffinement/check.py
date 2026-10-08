#!/usr/bin/env python3
"""Modèle Fraction borné ; aucune primitive du moteur importée."""
from fractions import Fraction as F
from itertools import product
from pathlib import Path
import hashlib
import json
import subprocess
import sys


def need(ok, why):
    if not ok:
        raise ValueError(why)


def d2(x, c):
    return sum((a-b)**2 for a, b in zip(x, c))


def corners(e):
    return list(product((F(0), e), repeat=3))


def lower(x, y, e):
    # Minimum d'une fonction affine, dérivé sans appeler d2 aux coins.
    return sum(a*a-b*b-2*e*max(a-b, 0) for a, b in zip(x, y))


def kept(points, k, e, weak=False):
    mid = (e/2,)*3
    witnesses = sorted(range(len(points)), key=lambda i: (d2(points[i], mid), i))[:3*k]
    out = []
    for i, x in enumerate(points):
        count = sum(i != j and (lower(x, points[j], e) >= 0 if weak else
                                lower(x, points[j], e) > 0) for j in witnesses)
        if count < k:
            out.append(i)
    return out


def main():
    here = Path(__file__).resolve().parent
    cap = json.loads((here/'capture.json').read_text())
    for path, wanted in cap['sources'].items():
        b = subprocess.check_output(['git', '-C', sys.argv[1], 'show', cap['pin']+':'+path])
        need(hashlib.sha256(b).hexdigest() == wanted, path)
    fixtures = [
        [(-1000,0,0),(1000,0,0),(0,1001,0),(0,-1001,0)],
        [(-1,0,0),(1,0,0),(0,1,0),(0,-1,0)],
        [(-1,0,0),(1,0,0),(0,1,0),(0,-1,0),(0,0,1),(0,0,-1)],
        [(-1,0,0),(1,0,0),(-2,0,0),(2,0,0),(0,3,0),(0,-3,0)],
    ]
    tested = comparisons = grids = 0
    for points in fixtures:
        for k in range(1, min(5, len(points))+1):
            distances = [d2(x, (F(0),)*3) for x in points]
            cutoff = sorted(distances)[k-1]
            prefix = [i for i, d in enumerate(distances) if d <= cutoff]
            margins = [F(distances[i]-distances[j], 4*sum(abs(a-b) for a,b in zip(points[i],points[j])))
                       for i in range(len(points)) if i not in prefix for j in prefix]
            fine = min(margins) if margins else F(1,16)
            need(kept(points, k, fine) == prefix, 'préfixe local exact')
            for e in (F(1),F(1,4),fine):
                out = kept(points, k, e)
                for x in points:
                    for y in points:
                        exact = min(d2(x,c)-d2(y,c) for c in corners(e))
                        need(exact == lower(x,y,e), 'minimum affine')
                        delta = d2(x,(F(0),)*3)-d2(y,(F(0),)*3)
                        need(exact >= delta-2*e*sum(abs(a-b) for a,b in zip(x,y)), 'borne L1')
                        comparisons += 1
                for c in product((F(0),e/2,e),repeat=3):
                    for i,x in enumerate(points):
                        if sum(d2(y,c)<d2(x,c) for y in points) < k:
                            need(i in out, 'site de rang strict <K perdu')
                    grids += 1
                tested += 1
    rectangle = fixtures[0]
    shift = (1001,1001,1)
    translated = [tuple(a+b for a,b in zip(x,shift)) for x in rectangle]
    need(all(0 <= a < 2**21 for x in translated for a in x), 'translation u21')
    for c in corners(F(1)):
        for x,y in zip(rectangle,translated):
            need(d2(x,c) == d2(y,tuple(a+b for a,b in zip(c,shift))), 'translation exacte')
    need(len(kept(rectangle,1,F(1))) == 3, 'boîte unité')
    need(len(kept(rectangle,1,F(1,4))) == 2, 'petite boîte')
    square = fixtures[1]
    for e in (F(1),F(1,4),F(1,1024)):
        need(len(kept(square,1,e)) == 4, 'contacts égaux')
    need(len(kept(square,1,F(1,4),weak=True)) != 4, 'mutant dominance')
    need(len(sorted(range(4),key=lambda i:d2(square[i],(0,0,0)))[:1]) != 4, 'mutant coupe égalités')
    result = {'cas_filtre':tested,'comparaisons_affines':comparisons,'grilles':grids,
              'rectangle_candidats':[3,2], 'carre_contacts':4,'mutations_refusees':2,
              'pins':len(cap['sources']),'natif_execute':False,'mesure_mesc2':False}
    if (here/'results.json').exists():
        need(json.loads((here/'results.json').read_text()) == result, 'résultat figé')
    print(json.dumps(result,ensure_ascii=False,sort_keys=True))


if __name__ == '__main__':
    main()
