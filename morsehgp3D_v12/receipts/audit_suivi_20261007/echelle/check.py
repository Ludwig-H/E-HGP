#!/usr/bin/env python3
"""Scale contracts: constant-size arithmetic, analytic lines, bounded oracle n=8.

No native execution, allocation proportional to large projected cardinalities,
nor experimental complexity claim. The v11 Definition oracle is used explicitly
for two eight-site clouds; all other checks use only Python's exact integers.
"""
import json
from fractions import Fraction
from pathlib import Path
import sys
sys.dont_write_bytecode = True


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def line_counts(n, kmax):
    need(n > kmax >= 1, 'line domain')
    # Sites (i,0,0), i=0..n-1. Each positive critical ball is a diameter.
    # Gap d gives p=d-1, q=m=2; admission p+q<=K+1 means 1<=d<=K.
    catalogue = sum(n-d for d in range(1,kmax+1))
    incidences = sum((n-d)*(d+1) for d in range(1,kmax+1))
    births = [n-k+1 for k in range(1,kmax+1)]
    return dict(n=n,K=kmax,catalogue=catalogue,incidences=incidences,
                births_by_order=births,binary_events_by_order=[b-1 for b in births],
                final_merges_by_order=[1]*kmax,final_nodes_by_order=[b+1 for b in births])


def main():
    root = Path(__file__).resolve().parents[4]
    sys.path.insert(0,str(root/'morsehgp3D_v11/reference'))
    from hgp11_ref.definition import Definition
    line = [(i,0,0) for i in range(8)]
    d = Definition(line)
    counts = line_counts(8,5)
    checked = []
    for k in range(1,6):
        order = d.order(k)
        births = [x for x in order.nodes if not x.children]
        merges = [x for x in order.nodes if x.children]
        need(len(births)==counts['births_by_order'][k-1],'line births')
        need(len(merges)==1 and len(merges[0].children)==len(births),'line plateau')
        need(all(x.level == Fraction((k-1)**2,4) for x in births),'line birth dates')
        need(merges[0].level == Fraction(k*k,4),'line fusion date')
        checked.append(dict(k=k,births=len(births),merges=1,children=len(merges[0].children),
                            birth_level=str(births[0].level),merge_level=str(merges[0].level)))
    cube = [(x,y,z) for x in (0,2) for y in (0,2) for z in (0,2)]
    order = Definition(cube).order(2)
    nb = sum(not node.children for node in order.nodes)
    need(nb == 12 and len(order.nodes)==13,'cube births exceed input sites')

    ev, none = 1<<31,(1<<32)-1
    leaf = ev
    event_zero = 0 | ev
    need(leaf == event_zero,'tag alias')
    event_last_payload = (ev-1) | ev
    need(event_last_payload == none,'tag sentinel alias')
    nb = ev+1
    need(nb+1 < none,'contracted giant plateau fits dense u32')
    projected_inc = 5_000_000 * 1137
    need(projected_inc > none,'64-bit CSR requirement')

    sphere = [(x,y,z) for x in range(-7,8) for y in range(-7,8) for z in range(-7,8)
              if x*x+y*y+z*z==50]
    need(len(sphere)==84 and all(tuple(-c for c in p) in sphere for p in sphere),'sphere50 exact shell')
    result = dict(schema='ehgp.v12.scale_audit.v1',pin='3e6e6a8e721c6dbe8aaa3a0c5cb20c56abfae5ff',
      native_executed=False,
      tag=dict(leaf_id=leaf,event_index=0,event_encoding=event_zero,
               maximum_payload=ev-1,encoding_of_maximum_payload=event_last_payload,kNone=none,
               abstract_plateau_births=nb,abstract_plateau_final_nodes=nb+1,
               geometric_under_10M_realization_claimed=False),
      csr=dict(projected_K10_sites=5_000_000,incidences_per_site=1137,
               projected_incidences=projected_inc,wrapped_u32=projected_inc & none),
      bounded_oracle=dict(max_sites=8,line=checked,cube_K2=dict(sites=8,births=12,merges=1,nodes=13)),
      analytic_lines=[line_counts(n,5) for n in (8,60_000,9_999_999)],
      sphere50=dict(sites=84,p=0,q_min=2,m=84,admitted_K5=True,
                    conditional_refusal_if_shell_cap_is_64=True,native_refusal_claimed=False),
      per_site_claim=dict(main_ns_per_site=100_000_000//60_000,
                          allowed_100_site_total_ns_fraction='500000/3',
                          two_ms_per_100_sites_ns_per_site=20_000),
      universal_combinatorial_bounds='C<=sum(q=2..4,binomial(n,q)); I<=n*C; b1=n, bk<=C; Nk<=2*bk-1')
    print(json.dumps(result,sort_keys=True,indent=2))


if __name__ == '__main__':
    main()
