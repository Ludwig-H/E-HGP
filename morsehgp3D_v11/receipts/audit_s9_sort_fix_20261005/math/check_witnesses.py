#!/usr/bin/env python3
"""Two new S9 fixture expectations checked from Definition only; no native or unchanged-model replay."""
import json
import pathlib
import sys
from fractions import Fraction

sys.dont_write_bytecode=True
HERE=pathlib.Path(__file__).resolve().parent
sys.path.insert(0,str(HERE/'snapshot/morsehgp3D_v11/tests/points'))
import points_oracle_stdlib as O

def need(ok,message):
    if not ok:raise RuntimeError(message)

res=O.Definition(O.EQUILATERAL).order(2)
entries,tree=O.reference_radius(res,len(O.EQUILATERAL),1)
groups=O.oracle_blocks(entries,tree,(Fraction(1),Fraction(0),Fraction(0)))
need(groups==[[0,1],[4,5]],'F2 m1 radius groups differ from exact Definition')

res=O.Definition(O.PLATEAU).order(2)
entries,tree=O.reference_radius(res,len(O.PLATEAU),1)
owner,date=entries[0]
signature=O.owner_signature(res,len(O.PLATEAU),owner)
need(O.two_roots_sign(date[0],date[1],date[2],Fraction(8))==0,'F6 date differs from sqrt8')
need(signature==(Fraction(8),frozenset({0,1,3})),'F6 closed-cut owner differs')
up=tree.parent[owner]
need(up<0 or O.two_roots_sign(date[0],date[1],date[2],res.nodes[up].level)<0,'F6 owner not alive')
print(json.dumps(dict(pin='451301787c7e02b8f5e1a4c08064275b6af6de1e',native_executed=False,
                     scope='two newly graved m1 fixture expectations from exact Definition',
                     F2_groups_at_level1=groups,F6_date=[str(v) for v in date],
                     F6_owner_birth=str(signature[0]),F6_owner_cover=sorted(signature[1]),
                     F6_closed_cut=True),sort_keys=True,separators=(',',':')))
