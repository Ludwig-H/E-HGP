#!/usr/bin/env python3
"""Exact, standalone Fraction oracle for a q3 leaf witness-palette fixture.

This models the published root-cell partition and kd split rule, not timing.
std::partition is unstable in C++, but the count below is independent of
within-child order because all 46 active left-child sites precede w.
"""
from fractions import Fraction as Q

A=(20,20,20); B=(30,20,20)
LINE=[(24,20,20),(25,20,20),(26,20,20)]
W=(25,21,20)
SEEDS=[(25,28,20),(25,28,21)]
DECOYS=[(x,13,z) for x in (20,21,29,30) for z in range(15,26)]
PTS=[A,B,*LINE,W,*SEEDS,*DECOYS]
M=(25,20,20)
T=4 # q3 rejection threshold K-1 at K=5
def require(condition, message):
    if not condition: raise AssertionError(message)

require(len(DECOYS)==44 and len(PTS)==52 and len(set(PTS))==52, "distinct sites")

def d2(a,b): return sum((x-y)**2 for x,y in zip(a,b))
def center(x):
    ry,rz=x[1]-20,x[2]-20
    t=Q(ry*ry+rz*rz-25,2*(ry*ry+rz*rz))
    return (Q(25),Q(20)+t*ry,Q(20)+t*rz)
def power(z,c):
    return sum((Q(z[i])-c[i])**2 for i in range(3))-sum((Q(A[i])-c[i])**2 for i in range(3))
# Root cell [-2,2]^2 in the local basis A=(0,10,0), B=(0,0,10).
CORNERS=[(Q(25),Q(20)+Q(10)*u,Q(20)+Q(10)*v) for u in (-2,2) for v in (-2,2)]
I=[]; O=[]; F=[]
for z in PTS:
    values=[power(z,c) for c in CORNERS]
    if max(values)<0: I.append(z)
    elif min(values)>0: O.append(z)
    else: F.append(z)
require(set(I)==set(LINE) and not O and len(F)==49, "root partition")
require(all(d2(z,M)<=100 for z in PTS), "edge cover")
require(all(max(d2(z,A),d2(z,B))>100 for z in DECOYS), "decoy ownership")
require(d2(A,B)==100, "edge diameter")

def spatial_order(points):
    if len(points)<=1: return points
    lo=[min(p[i] for p in points) for i in range(3)]
    hi=[max(p[i] for p in points) for i in range(3)]
    axis=max(range(3),key=lambda i:hi[i]-lo[i]) # first axis wins ties
    mid=(lo[axis]+hi[axis])//2
    left=[p for p in points if p[axis]<=mid]
    right=[p for p in points if p[axis]>mid]
    require(left and right, "spatial split")
    return spatial_order(left)+spatial_order(right)
ORDER=spatial_order(PTS)
# The root splits on y=20: all decoys and endpoints precede w.
require([max(p[i] for p in PTS)-min(p[i] for p in PTS) for i in range(3)]==[10,15,10],
        "root axis")
require(all(ORDER.index(z)<ORDER.index(W) for z in DECOYS+[A,B]), "left child order")
require(all(ORDER.index(W)<ORDER.index(x) for x in SEEDS), "right child order")

palette=[]
for seed_index,x in enumerate(SEEDS):
    sides=(d2(A,B),d2(A,x),d2(B,x))
    require(sides[0]>=max(sides[1:]) and all(sides[i]+sides[j]>sides[k]
        for i,j,k in ((0,1,2),(0,2,1),(1,2,0))), "acute owned seed")
    c=center(x)
    require(0<=c[1]<=40 and 0<=c[2]<=40, "center root domain")
    require(power(W,c)<0 and min(power(z,c) for z in DECOYS)>0,
            "active witness signs")
    depth=len(I); tests=0; last=None
    for z in ORDER:
        if z not in F: continue
        tests+=1; last=z
        if power(z,c)<0: depth+=1
        if depth>=T: break
    require(depth>=T and tests==47 and last==W, "late rejection")
    palette_tests=0
    if seed_index:
        palette_depth=len(I)
        for z in palette:
            palette_tests+=1
            if power(z,c)<0: palette_depth+=1
            if palette_depth>=T: break
        require(palette_depth>=T and palette_tests==1,
                "learned palette failed to reject early")
    else:
        palette=[last]
    print('seed',x,'sides',sides,'center',c,'w_power',power(W,c),
          'min_decoy_power',min(power(z,c) for z in DECOYS),
          'current_spatial_tests',tests,'palette_reuse_tests',palette_tests)
print('partition: I',len(I),'O',len(O),'F',len(F),
      'root_kd_axis=y root_mid=20; first 46 active sites precede w')
print('PASS exact Fraction oracle')
