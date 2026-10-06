#!/usr/bin/env python3
"""Exact bounded examples for an architecture proposal; no native/cloud run."""
import hashlib
import itertools
import json
import subprocess
from fractions import Fraction as F
from pathlib import Path

HERE = Path(__file__).resolve().parent
PIN = "87f1621053635e76f21908146e53b790feda0af1"

def need(value, message):
    if not value:
        raise RuntimeError(message)

def sqdist(a, b):
    return sum((F(x)-F(y))**2 for x, y in zip(a,b))

def bary(points):
    return tuple(sum(F(p[j]) for p in points)/len(points) for j in range(3))

def scalar(x):
    x=F(x)
    return str(x.numerator) if x.denominator==1 else str(x)

def main():
    manifest=json.loads((HERE/"sources.json").read_text())
    need(manifest["pin"]==PIN,"pin")
    sources={}
    for path, digest in manifest["files"].items():
        raw=subprocess.check_output(["git","show",PIN+":"+path])
        need(hashlib.sha256(raw).hexdigest()==digest,"source "+path)
        sources[path]=raw.decode()
    need("if (p + qmin > static_cast<u32>(run.params.kmax) + 1) return {};" in
         sources["morsehgp3D_v11/src/catalogue/leaf.cpp"],"catalogue admission")

    tetra=((2,2,2),(2,0,0),(0,2,0),(0,0,2))
    c=(1,1,1)
    need(bary(tetra)==c,"tetra center")
    u,v,w=[tuple(p[j]-tetra[0][j] for j in range(3)) for p in tetra[1:]]
    det=u[0]*(v[1]*w[2]-v[2]*w[1])-u[1]*(v[0]*w[2]-v[2]*w[0])+u[2]*(v[0]*w[1]-v[1]*w[0])
    need(det==-16,"affine independence and unique positive center weights")
    need(all(sqdist(c,p)==3 for p in tetra),"tetra sphere")
    mids=sorted({bary(pair) for pair in itertools.combinations(tetra,2)})
    expected=sorted((tuple(F(1+(sign if j==axis else 0)) for j in range(3))
                     for axis in range(3) for sign in (-1,1)))
    need(mids==expected,"order-2 octahedron vertices")
    # A regular tetrahedron is affinely independent and has positive weights 1/4:
    # its centered sphere has q_min=4, p=0.
    need(0+4>1+1 and 0+4>2+1 and 0+4<=3+1,"catalogue window")

    tri=((0,0,0),(4,0,0),(1,1,0))
    fourth=(0,0,10)
    circum=(2,-1,0)
    need(all(sqdist(circum,p)==5 for p in tri),"triangle circumradius")
    need(sqdist(circum,fourth)==105,"empty sphere in 3D")
    weights=(F(5,4),F(3,4),F(-1))
    reconstructed=tuple(sum(w*p[j] for w,p in zip(weights,tri)) for j in range(3))
    need(sum(weights)==1 and reconstructed==circum and min(weights)<0,
         "circumcenter outside convex hull")
    meb=(2,0,0)
    need(sqdist(meb,tri[0])==sqdist(meb,tri[1])==4 and sqdist(meb,tri[2])==2,
         "different minimum enclosing ball")

    q=(F(0),F(2)); y=F(0)
    mean=sum((y-x)**2 for x in q)/len(q)
    kth=max((y-x)**2 for x in q)
    need(mean==2 and kth==4 and mean<=3<kth,"mean is not kth distance")

    # Cospherical degeneracy: six 2-subsets, only four extreme barycenters.
    square=((0,0,0),(2,0,0),(2,2,0),(0,2,0))
    all_mids=[bary(pair) for pair in itertools.combinations(square,2)]
    need(len(all_mids)==6 and len(set(all_mids))==5 and all_mids.count((F(1),F(1),F(0)))==2,
         "coincident non-extreme barycenters")
    need(set(all_mids)-{(F(1),F(1),F(0))}=={(0,1,0),(1,0,0),(1,2,0),(2,1,0)},
         "four diamond extreme vertices")

    result={
        "pin":PIN,"native_runs":0,"cloud_actions":0,
        "sources_verified":len(sources),
        "tetrahedron":{"radius_squared":3,"order_2_vertices":[[scalar(x) for x in p] for p in mids],
                      "q_min":4,"admitted_Cat1":False,"admitted_Cat2":False,"admitted_Cat3":True},
        "obtuse_face":{"sphere_radius_squared":5,"fourth_site_distance_squared":105,
                       "center_barycentric":[scalar(w) for w in weights],"meb_radius_squared":4},
        "distance_filter":{"mean_squared":scalar(mean),"kth_squared":scalar(kth),"threshold_squared":3},
        "degenerate_square":{"two_subsets":6,"distinct_barycenters":5,"extreme_vertices":4},
        "scope":"Exact examples and pinned source identities; no mosaic constructor or surface validation."
    }
    need(result==json.loads((HERE/"proof.json").read_text()),"frozen proof")
    print(json.dumps(result,indent=2,sort_keys=True))

if __name__=="__main__":
    main()
