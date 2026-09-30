"""Exact bounded K2 counterexample for fixed-mass majority.

Run directly, without arguments, from any working directory. Uses the
repository Fraction reference for MEB and DSU; this dependency is hashed in
receipt.json. No native build, random seed, selection, or performance claim.
"""
from fractions import Fraction as F
from itertools import combinations
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'reference'))
import hgp10_ref as R


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


# Translated nonnegative u18 coordinates; point 0 is the audited frontier.
P = [(1,1,0), (2,1,0), (0,2,0), (0,0,0), (0,1,1)]
pairs = list(combinations(range(len(P)), 2))
pair_beta = {a:R.meb(P,a)[0] for a in pairs}
tri_beta = {a:R.meb(P,a)[0] for a in combinations(range(len(P)),3)}

# For K2, p+q_min<=2 and positive radius require p=0,q_min=2.
# Enumerate ALL diameter spheres, deduplicate exact geometry, retain every
# boundary incidence, including points outside the canonical support pair.
by_geometry = {}
for a in pairs:
    c = tuple(F(P[a[0]][j]+P[a[1]][j],2) for j in range(3))
    b = pair_beta[a]
    powers = [R.d2(c,p)-b for p in P]
    if any(v<0 for v in powers):
        continue
    pop = tuple(i for i,v in enumerate(powers) if v==0)
    by_geometry[(c,b)] = (b,pop)
atoms = sorted(by_geometry.values())

# Counter-check the universe against the independent exhaustive critical
# sphere reference, including larger supports with the same geometry.
expected = {(b.center,b.level): (b.level,tuple(sorted(b.I+b.U)))
            for b in R.critical_balls(P)
            if b.level>0 and b.p+b.qmin<=2 and len(b.I)+len(b.U)>=2}
require(by_geometry==expected, 'incomplete proper K2 witness universe')
for kmax in (2,3,4,5):
    admitted = {(b.center,b.level): (b.level,tuple(sorted(b.I+b.U)))
                for b in R.catalogue(P,kmax)
                if b.level>0 and b.p+b.qmin<=2 and len(b.I)+len(b.U)>=2}
    require(admitted==by_geometry, 'Kmax-dependent proper witness universe')

levels = sorted(set(pair_beta.values()) | set(tri_beta.values()))
W = {mode:[sum((F(1) if mode=='uniform' else 1/b)
               for b,pop in atoms if x in pop) for x in range(len(P))]
     for mode in ('uniform','inverse_beta')}
first_cover = [min(pair_beta[a] for a in pairs if x in a) for x in range(len(P))]
first_pairs = [tuple(a for a in pairs if x in a and pair_beta[a]==first_cover[x])
               for x in range(len(P))]
modes = ('uniform','inverse_beta','hybrid_unique_first','k2_lca_eta0')
first_owner = {mode:{} for mode in modes}
first_join = {mode:None for mode in modes}
previous = {mode:{} for mode in modes}
trace = []

for beta in levels:
    # The exact nerve Gamma2: vertices are active pairs; their witness
    # regions meet when the union triple has MEB radius squared <= beta.
    # Elementary triple links suffice; all simultaneous events precede votes.
    d = R.DSU()
    for a in pairs:
        if pair_beta[a]<=beta:
            d.find(a)
    for a,b in tri_beta.items():
        if b<=beta:
            facets = list(combinations(a,2))
            for q in facets[1:]:
                d.union(facets[0],q)
    comps = {}
    for a in pairs:
        if pair_beta[a]<=beta:
            comps.setdefault(d.find(a),set()).update(a)
    covered = {x:{d.find(a) for a in pairs if x in a and pair_beta[a]<=beta}
               for x in range(len(P))}
    scores = {mode:{} for mode in W}
    for b,pop in atoms:
        if b>beta:
            continue
        # All K-parts contained in a ball see its center in their convex
        # witness regions. Check their common component before any voting.
        roots = {d.find(a) for a in combinations(pop,2)}
        require(len(roots)==1, 'one ball resolved to several components')
        comp = next(iter(roots))
        for mode in W:
            val = F(1) if mode=='uniform' else 1/b
            for x in pop:
                scores[mode][(x,comp)] = scores[mode].get((x,comp),F(0))+val
    owners = {mode:{x:None for x in range(len(P))} for mode in modes}
    for mode,w in W.items():
        for (x,comp),v in scores[mode].items():
            if v>w[x]/2:  # STRICT majority; beta is the SQUARED radius.
                require(owners[mode][x] is None, 'two strict majorities')
                owners[mode][x] = comp
        for x,old in previous[mode].items():
            require(owners[mode][x]==d.find(old), 'majority owner withdrew or switched')
    for x in range(len(P)):
        old = previous['hybrid_unique_first'].get(x)
        if old is not None:
            owners['hybrid_unique_first'][x] = d.find(old)
        elif beta==first_cover[x] and len(covered[x])==1:
            owners['hybrid_unique_first'][x] = next(iter(covered[x]))
        else:
            owners['hybrid_unique_first'][x] = owners['inverse_beta'][x]
        if beta>=first_cover[x]:
            roots = {d.find(a) for a in first_pairs[x]}
            if len(roots)==1:
                owners['k2_lca_eta0'][x] = next(iter(roots))
    for mode in modes:
        for x,c in owners[mode].items():
            if c is not None:
                require(c in covered[x], 'owner does not cover the point')
                first_owner[mode].setdefault(x,(beta,tuple(sorted(comps[c]))))
                previous[mode][x] = c
        if owners[mode][0] is not None and owners[mode][0]==owners[mode][1]:
            if first_join[mode] is None:
                first_join[mode] = beta
    trace.append((beta,comps,owners,scores))

require(atoms==[(F(1,4),(0,1)), (F(1,2),(0,2)), (F(1,2),(0,3)),
               (F(1,2),(0,4)), (F(1,2),(2,4)), (F(1,2),(3,4)),
               (F(1),(0,2,3,4))], 'unexpected atoms')
require(W['inverse_beta'][0]==11, 'unexpected fixed mass')
require(first_owner['inverse_beta'][0]==(F(2,3),(0,2,3,4)), 'unexpected remote assignment')
require(first_join['inverse_beta']==F(5,4), 'unexpected delayed nearest pair')
require(first_join['hybrid_unique_first']==F(1,4), 'hybrid lost unique first coverage')
require(first_join['k2_lca_eta0']==F(1,4), 'LCA eta0 lost unique minimizer')
require(R.rank([R.sub(P[i],P[0]) for i in range(1,5)])==3, 'fixture not genuinely 3D')
laminar_checks = 0
for i,(_,_,owners_i,_) in enumerate(trace):
    for _,_,owners_j,_ in trace[i+1:]:
        for mode in modes:
            for x,y in pairs:
                joined_i = owners_i[mode][x] is not None and owners_i[mode][x]==owners_i[mode][y]
                joined_j = owners_j[mode][x] is not None and owners_j[mode][x]==owners_j[mode][y]
                require(not joined_i or joined_j, 'point block split at a later cut')
                laminar_checks += 1

print('coordinates',P)
print('unit: beta=squared radius; threshold M>W/2; cover_extra=0')
print('proper atoms (beta, closed population)',[(str(b),pop) for b,pop in atoms])
print('fixed W', {mode:list(map(str,w)) for mode,w in W.items()})
for beta,comps,owners,scores in trace:
    print('cut',str(beta),'covered components',sorted(sorted(v) for v in comps.values()))
    for mode in modes:
        print(' ',mode,'owners', [None if owners[mode][x] is None
                                  else sorted(comps[owners[mode][x]]) for x in range(len(P))])
    print(' inverse_beta mass0',[(sorted(comps[c]),str(v))
                                 for (x,c),v in scores['inverse_beta'].items() if x==0])
print('first_owner', {mode:{x:(str(b),pop) for x,(b,pop) in z.items()}
                      for mode,z in first_owner.items()})
print('first_join_0_1', {mode:str(b) for mode,b in first_join.items()})
print('status ok; proper universe equal for Kmax=2,3,4,5; exact Fraction only;',
      laminar_checks,'pairwise laminar checks')
