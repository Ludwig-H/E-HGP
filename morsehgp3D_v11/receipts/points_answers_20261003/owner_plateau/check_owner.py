#!/usr/bin/env python3
"""Bounded replay of frozen actual Python helpers at an exact closed plateau.

No native executable, fit or GCP. AST extraction keeps actual function bodies;
the dictionary of requested definitions is strict and their SHA pins are checked.
"""
import ast
import decimal
from fractions import Fraction
import hashlib
import json
import math
from pathlib import Path
import sys
import numpy as np

BASE = Path(__file__).resolve().parent
sys.dont_write_bytecode = True
sys.path.insert(0, str(BASE/'reference'))
from hgp11_ref import Definition  # noqa: E402

checks=0


def need(ok, message):
    global checks
    checks+=1
    if not ok:raise RuntimeError(message)


def extract(path, names, namespace):
    source=path.read_bytes()
    before=json.loads((BASE/'before.json').read_text())
    pin=next(e['sha256'] for e in before['entries'] if e['snapshot']==str(path.relative_to(BASE)))
    need(hashlib.sha256(source).hexdigest()==pin,'source hash '+str(path))
    tree=ast.parse(source,filename=str(path))
    selected=[n for n in tree.body if isinstance(n,(ast.FunctionDef,ast.ClassDef)) and n.name in names]
    need({n.name for n in selected}==set(names),'requested AST definitions')
    module=ast.Module(body=selected,type_ignores=[])
    exec(compile(ast.fix_missing_locations(module),str(path),'exec'),namespace)


PR={'Fraction':Fraction}
extract(BASE/'source/points_reference.py',
        ['parents_of','popcount','Tree','reference_radius_rules','reference_owner_signature'],PR)
RH={'Fraction':Fraction,'math':math,'ZERO':Fraction(0)}
extract(BASE/'source/points_radius.py',
        ['sign','sqrt_diff_cmp','sqrt_cmp2','sqrt_bounds','Refusal','square_ratio','radical_classes',
         'sign_of_radicals','RValue','ancestor_at_radius'],RH)
LH={'np':np,'need':need,'Fraction':Fraction}
extract(BASE/'source/points_hierarchy.py',['Levels'],LH)


def exact_owner(res,tree,start,value):
    owner=start
    while tree.parent[owner]>=0 and value.cmp_sqrt(res.nodes[tree.parent[owner]].level)>=0:
        owner=tree.parent[owner]
    return owner


def actual_public_ancestor(res,tree,start,value):
    values=sorted(set(Fraction(0) for _ in [0])|{n.level for n in res.nodes})
    levels=LH['Levels'].from_fractions(values)
    class Order:
        def lifting(self):
            # Valid first binary-lifting table; the actual helper's terminal loop closes the ancestor chain.
            return None,[[p if p>=0 else i for i,p in enumerate(self.parent)]]
    order=Order();order.parent=tree.parent;order.rank=[values.index(n.level) for n in res.nodes];order.levels=levels
    parent=tree.parent[start]
    need(parent>=0,'first node has parent')
    need(value.cmp_level(levels,order.rank[parent])==0,'actual cmp_level certifies exact plateau')
    return RH['ancestor_at_radius'](order,start,value)


rows=[]
for shift in (0,4):
    points=[(0+shift,0+shift,shift),(2+shift,2+shift,shift),(-4+shift,-4+shift,shift),(4+shift,4+shift,shift)]
    res=Definition(points).order(2)
    tree=PR['Tree'](res.nodes)
    need(sorted(n.level for n in res.nodes)==[Fraction(2),Fraction(2),Fraction(8),Fraction(8),Fraction(18)],'exact Gamma FULL levels')
    reference,tree=PR['reference_radius_rules'](res,4,1,precision=120)
    e,wrong,triple=reference['margin_r'][0]
    need(triple==(Fraction(2),Fraction(18),Fraction(8)),'actual selected date triple')
    value=RH['RValue'](*triple)
    need(value.cmp_sqrt(Fraction(8))==0,'exact radical equality sqrt2+sqrt18-sqrt8=sqrt8')
    # Determine the starting component from the first closed coverage, without asking the oracle rule for it.
    first=next(cut for cut in res.cuts if any(coverage&1 for _,coverage,_ in cut.closed))
    primary=next(v for v,coverage,_ in first.closed if coverage&1)
    correct=exact_owner(res,tree,primary,value)
    need(correct==tree.parent[primary],'parent is alive at exact date')
    need(wrong==primary and wrong!=correct,'actual Decimal oracle returns child dead at exact date')
    exact_sig=PR['reference_owner_signature'](res,4,correct)
    wrong_sig=PR['reference_owner_signature'](res,4,wrong)
    need(exact_sig==(Fraction(8),frozenset([0,1,3])),'correct closed owner signature')
    need(wrong_sig==(Fraction(2),frozenset([0,1])),'wrong dead child signature')
    helper_owner=actual_public_ancestor(res,tree,primary,value)
    need(helper_owner==correct,'actual exact owner helper chooses parent')
    with decimal.localcontext(decimal.Context(prec=120)):
        root8=decimal.Decimal(8).sqrt()
        discrepancy=e-root8
        need(discrepancy==decimal.Decimal('-1E-119'),'actual rounding lies below exact closed plateau')
    # Retained Pi3 has a different start and does not expose this witness, even though its rule computes raw too.
    qualified,_=PR['reference_radius_rules'](res,4,3,precision=120)
    eq,oq,tq=qualified['margin_r'][0]
    need(tq==(Fraction(8),Fraction(0),Fraction(0)),'qualified start itself is at plateau')
    need(oq==correct,'current retained Pi3 control')
    rows.append({'shift':shift,'coordinates':points,'bits21_admissible':all(0<=c<1<<21 for p in points for c in p),
                 'actual_reference_SHA256':'ab200babb0aa066139725a830e4c95c9eba5d05bbc904fe08d03c0907c8c0333',
                 'date_triple':[str(x) for x in triple],'exact_date_squared':'8',
                 'Decimal120_date':str(e),'Decimal120_gap_from_root8':str(discrepancy),
                 'oracle_owner':wrong,'exact_owner':correct,'actual_exact_helper_owner':helper_owner,
                 'oracle_owner_signature':[str(wrong_sig[0]),sorted(wrong_sig[1])],
                 'exact_owner_signature':[str(exact_sig[0]),sorted(exact_sig[1])],
                 'Pi3_control_owner':oq,'Pi3_control_date_triple':[str(x) for x in tq],
                 'FULL_nodes':[{'level':str(n.level),'children':list(n.children)} for n in res.nodes]})

print(json.dumps({'status':'PASS_EXPECTED_COUNTEREXAMPLE','checks':checks,'cases':rows,
                 'scope':'actual frozen Python oracle/helper AST; Fraction Gamma; no native/GCP or fit',
                 'conclusion':'Decimal localcontext fixes precision leakage, but finite precision is not a closed-owner certificate; exact symbolic equality chooses the living parent.'},indent=2,sort_keys=True))
