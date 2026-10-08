#!/usr/bin/env python3
"""Modèle entier borné : rangs C -> naissances K1 ; aucune donnée réelle."""
import hashlib
import itertools
import json
from pathlib import Path
import subprocess
import argparse


def need(value, why):
    if not value:
        raise ValueError(why)


def morton(p, bits):
    return sum(((v >> i) & 1) << (3*i+a) for a,v in enumerate(p) for i in range(bits))


def catalogue_ranks(cloud):
    # Même encodage entier que SiteKeyKernel, oracle T ci-dessous par tuples.
    order=sorted(range(len(cloud)),key=lambda i:(cloud[i][0]<<64)|(cloud[i][1]<<32)|cloud[i][2])
    ranks=[0]*len(cloud)
    for j,i in enumerate(order):ranks[i]=j+1
    return ranks


def transfer(cloud,ranks,birth_key,birth_rank):
    n=len(cloud)
    if birth_key!=list(range(n)) or birth_rank!=[0]*n:
        return None  # contrat générique : garder la voie actuelle
    need(len(ranks)==n,'taille')
    order=[None]*n
    for site,r in enumerate(ranks):
        need(type(r) is int and 1<=r<=n,'domaine rang')
        need(order[r-1] is None,'rang répété')
        order[r-1]=site
    need(all(cloud[a]<cloud[b] for a,b in zip(order,order[1:])),'ordre du Cloud courant')
    nodes=[r-1 for r in ranks]
    return order,nodes


def one(points,bits):
    cloud=sorted(points,key=lambda p:morton(p,bits))
    ranks=catalogue_ranks(cloud)
    order,nodes=transfer(cloud,ranks,list(range(len(cloud))),[0]*len(cloud))
    expected=sorted(range(len(cloud)),key=lambda i:cloud[i])
    need(order==expected,'ordre canonique')
    need(all(order[nodes[i]]==i for i in range(len(cloud))),'inverses')
    # Ces IDs identiques alimentent ensuite le même noyau, aucune nouvelle règle d'union.
    return len(cloud)


def run():
    cube=list(itertools.product(range(2),repeat=3))
    subsets=0
    for mask in range(1,1<<len(cube)):
        one([p for i,p in enumerate(cube) if mask>>i&1],1)
        subsets+=1
    high=0
    for bits in (21,24,32):
        hi=(1<<bits)-1
        points=list(itertools.product((0,hi,hi-1),repeat=3))
        one(points,bits);high+=1
    cloud=[(1,0,0),(0,1,0)]  # Morton, qui n'est pas l'ordre XYZ
    result=transfer(cloud,[2,1],[0,1],[0,0])
    need(result==([1,0],[1,0]),'témoin Morton/XYZ')
    need(transfer(cloud,[2,1],[1],[0]) is None,'sous-ensemble hors raccourci')
    need(transfer(cloud,[2,1],[0,1],[0,1]) is None,'plusieurs rangs hors raccourci')
    corruptions=[[0,1],[1,1],[True,2],[3,1],[2],[1,2]]
    rejected=0
    for ranks in corruptions:
        try:transfer(cloud,ranks,[0,1],[0,0])
        except ValueError:rejected+=1
    need(rejected==len(corruptions),'rang invalide/stale admis')
    return dict(nonempty_cube_subsets=subsets,high_bit_clouds=high,rejected_rank_tables=rejected,
        fallback_domains=2,native_execution=False,performance_measurement=False)


def main():
    ap=argparse.ArgumentParser();ap.add_argument('--repo',type=Path,required=True);args=ap.parse_args()
    pins=json.loads(Path(__file__).with_name('capture.json').read_text())
    for f,digest in pins['sources'].items():
        raw=subprocess.check_output(['git','-C',str(args.repo),'show',pins['source_pin']+':'+f])
        need(hashlib.sha256(raw).hexdigest()==digest,'source changée')
    print(json.dumps(run(),sort_keys=True))


if __name__=='__main__':main()
