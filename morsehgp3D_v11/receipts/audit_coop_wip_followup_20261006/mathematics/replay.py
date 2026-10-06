#!/usr/bin/env python3
"""Small exact control-flow witnesses; no native execution."""
from fractions import Fraction as F
from itertools import combinations
import hashlib
import json
from pathlib import Path
import control_model as m

checks = 0
def require(ok, message):
    global checks
    if not ok:
        raise RuntimeError(message)
    checks += 1

def main():
    base = Path(__file__).parent
    manifest = json.loads((base / "sources.json").read_text())
    for rel, info in manifest["files"].items():
        require(hashlib.sha256((base / "source" / rel).read_bytes()).hexdigest() == info["sha256"], "source changed")
    require(hashlib.sha256((base / "control_model.py").read_bytes()).hexdigest() ==
            manifest["control_model_origin"]["sha256"], "model changed")

    p = [(0,3,0),(1,3,0),(3,1,4),(3,0,6),(5,6,6)]
    lo, hi, k = (2,2,2), (5,5,5), 3
    require(p == sorted(p, key=m.morton), "Morton order")
    reference = m.Traversal(p, lo, hi, k)
    tasks = reference.tasks()
    ordered = reference.play(tasks, range(len(tasks)))
    mutant = m.Traversal(p, lo, hi, k)
    changed = mutant.tasks()
    for i,j,suffix,logical,mask in changed:
        mutant.node((i,),j,suffix,logical,0)  # proposed masks[1]=0 mutation
    require(reference.dom == [2,0,0,4,0], "strict-box dominance")
    require(reference.counts == dict(prefixes=17,tests=3,evaluations=3,hits=0,rejects=0), "reference counters")
    require(mutant.counts == dict(prefixes=17,tests=4,evaluations=4,hits=0,rejects=0), "mutant counters")
    require([x for x in mutant.events if x not in reference.events] == [(0,3,4)], "extra prefix")
    require(m.line_meets(p[0],p[3],p[4],lo,hi), "extra J2 actually intersects")
    require((reference.dom[0] | reference.dom[3]).bit_count() == 2, "correct pair union")
    require(reference.dom[3].bit_count() == 1, "forgotten first mask")
    require((reference.live[1][0] & reference.live[1][3]) & (1 << 4), "live q3 descendant")
    serial = m.Traversal(p,lo,hi,k)
    serial.walk((),31,31,0)
    require(serial.counts == reference.counts and serial.events == ordered, "pair/DFS equality")

    # u18 near_max: enumerate every strict-acute q3, including ones the traversal may prune.
    M = (1 << 18) - 1
    cases = [[(0,0,0),(M,M,0),(M,0,M),(0,M,M)],
             [(0,0,0),(1,0,0),(M,M,0),(M,0,M),(0,M,M),(M-1,M,M)],
             [(0,0,0),(M//2,0,0),(0,M//2,0),(0,0,M//2),(M,M,M)]]
    q3_summary = []
    cross = lambda a,b: (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
    for pts in cases:
        acute, extended = 0, 0
        for ix in combinations(range(len(pts)),3):
            a,b,c = [pts[i] for i in ix]
            if min(m.dot(m.sub(b,a),m.sub(c,a)),m.dot(m.sub(a,b),m.sub(c,b)),m.dot(m.sub(a,c),m.sub(b,c))) <= 0:
                continue
            acute += 1
            u,v = m.sub(b,a),m.sub(c,a)
            w = cross(u,v)
            d = 2*m.dot(w,w)
            t = tuple(m.dot(u,u)*v[j] - m.dot(v,v)*u[j] for j in range(3))
            n = cross(t,w)
            center = tuple(F(a[j]) + F(n[j],d) for j in range(3))
            radius = m.dot(m.sub(center,a),m.sub(center,a))
            shell = [i for i,x in enumerate(pts) if m.dot(m.sub(center,x),m.sub(center,x)) == radius]
            require(len(shell) == 3, "u18 q3 has extended shell requiring uncertified orientation")
            extended += len(shell) > 3
        q3_summary.append(dict(strict_acute_q3=acute,extended_shell_q3=extended))
    require([x["strict_acute_q3"] for x in q3_summary] == [4,13,4], "fixture enumeration")
    require(6*18+8 <= 127 and M < 1 << 20, "u18 side/q4 width certified")
    # q4 raw |d| <= 12M^3; each |n_j| <= 18M^4, strict enough for global_orientation.
    require(12*M**3 < 1 << (124-3*18), "u18 q4 d globally certified")
    require(18*M**4 < 1 << (124-2*18), "u18 q4 n globally certified")
    print(json.dumps(dict(status="PASS_portable_only",checks=checks,native_executed=False,
        wip_head=manifest["head"],
        mask_fixture=dict(sites=p,morton_keys=[m.morton(x) for x in p],lo=lo,hi=hi,k=k,
                          dom=reference.dom,reference=reference.counts,mask_zero=mutant.counts,
                          extra_J2_prefix=[0,3,4]),
        near_max_u18=dict(coord_max=M,q3=q3_summary,partial_refusal_expected=False),
        limits="Control-flow model checks prefixes/J2, not all native counters or GPU concurrency; u18 near_max is a predicate-bound argument, not an executed test."),sort_keys=True,indent=2))

if __name__ == "__main__":
    main()
