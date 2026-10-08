#!/usr/bin/env python3
"""Pins source et contre-exemple de partition de candidats ; aucun moteur."""
import argparse
from fractions import Fraction as F
import hashlib
from itertools import combinations
import json
from pathlib import Path
import subprocess


def need(ok, message):
    if not ok:
        raise ValueError(message)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, required=True)
    a = ap.parse_args()
    cap = json.loads(Path(__file__).with_name('capture.json').read_text())
    for path, digest in cap['sources'].items():
        bodies = [subprocess.check_output(['git','-C',str(a.repo),'show',pin+':'+path])
                  for pin in cap['pins']]
        need(bodies[0]==bodies[1], 'source a2/83 différente: '+path)
        need(hashlib.sha256(bodies[0]).hexdigest()==digest, 'source modifiée: '+path)
    points = [(1,1,1),(1,3,1),(3,1,1),(3,3,1)]
    center = (F(2),F(2),F(1)); radius2=F(2)
    square = lambda v: sum(x*x for x in v)
    powers = [square(tuple(F(x)-c for x,c in zip(p,center)))-radius2 for p in points]
    need(powers==[0]*4, 'coquille exacte')
    def diameter(i,j):
        c = tuple(F(x+y,2) for x,y in zip(points[i],points[j]))
        return c, square(tuple(F(x)-z for x,z in zip(points[i],c)))
    supports = [pair for pair in combinations(range(4),2) if diameter(*pair)==(center,radius2)]
    need(supports==[(0,3),(1,2)], 'deux supports minimaux')
    need(min(supports)==(0,3), 'S* global par positions')
    blocks = [(0,1),(2,3)]
    local = [diameter(*pair) for block in blocks for pair in combinations(block,2)]
    need((center,radius2) not in local, 'centre perdu par partition')
    # Implication CloseKernel avec paramètres L2 : candidat>256>24 et feuille => largeur max<=1.
    # Tous les intervalles demi-ouverts survivants satisfont lo<hi à bornes entières.
    for widths in ((1,1,1),(1,1,2),(1,2,1),(2,1,1)):
        is_leaf = 257<=24 or max(widths)<=1
        need(is_leaf == (widths==(1,1,1)), 'témoin règle de fermeture')
    print(json.dumps(dict(source_files=len(cap['sources']), source_a2_83_identical=True,
        integer_square=True, interior=0, shell=4, qmin=2, global_support=list(min(supports)),
        local_blocks_lose_ball=True, belongs_to_Cat1_mathematically=True, native_execution=False),sort_keys=True))


if __name__=='__main__':
    main()
