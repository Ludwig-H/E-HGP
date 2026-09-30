#!/usr/bin/env python3
"""Bounded exact audit, n<=7; no engine/native export/large campaign."""
from fractions import Fraction as F
from itertools import combinations
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

REF = Path('/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10/reference/hgp10_ref.py')
JUDGE = Path('/workspaces/E-HGP/build/v10-frontiere/work/bench/frontier/juge_bras_ref.py')

def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod

def require(ok, message):
    if not ok:
        raise RuntimeError(message)

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

hashes = {'reference': sha(REF), 'judge': sha(JUDGE)}
R, J = load('audit_exact_ref', REF), load('audit_exact_judge', JUDGE)
counts = {'first_cover_incidence': 0, 'rejected_ball_incidence_strict_smaller_MEB': 0,
          'fractional_mass_identity': 0, 'gamma_distinct_birth_components': 0,
          'strong_universe_complete_component_coverage':0}
out = []

def check_firstcover(P, K):
    cache = {}
    def meb(idx):
        idx = tuple(sorted(idx))
        if idx not in cache:
            cache[idx] = R.meb(P, idx)[0]
        return cache[idx]
    alpha = [min(meb(t) for t in combinations(range(len(P)), K) if x in t) for x in range(len(P))]
    for b in R.critical_balls(P):
        pop = list(b.I + b.U)
        if len(pop) < K:
            continue
        for x in pop:
            if b.level == alpha[x]:
                require(b.p + b.qmin <= K, 'first cover incidence not strong')
                counts['first_cover_incidence'] += 1
            if b.p + b.qmin <= K:
                continue
            # Construct K-part F containing x with fewer than q_min boundary sites.
            if x in b.I and b.p >= K:
                t = [x] + [y for y in b.I if y != x][:K-1]
            elif x in b.U and b.p >= K-1:
                t = [x] + list(b.I[:K-1])
            else:
                t = list(b.I)
                if x in b.U:
                    t += [x]
                t += [y for y in b.U if y not in t][:K-len(t)]
            require(len(t) == K and x in t and len(set(t)) == K, 'bad smaller subset')
            require(len(set(t) & set(b.U)) < b.qmin, 'too many boundary sites')
            require(meb(t) < b.level, 'claimed strict smaller MEB not true')
            counts['rejected_ball_incidence_strict_smaller_MEB'] += 1
    return alpha

def check_all_covering_components(judge):
    cuts = [F(0)] + list(judge.levels) + [max(judge.levels)+1]
    cuts = sorted(set(cuts + [(a+b)/2 for a,b in zip(sorted(set(cuts)),sorted(set(cuts))[1:])]))
    for beta in cuts:
        labels = judge.labels(beta)
        for x in range(judge.n):
            intrinsic = {lab for vertex,lab in labels.items() if x in vertex}
            strong = {labels[vertex] for lvl,vertex,_ball in judge.wit[x] if lvl<=beta}
            require(intrinsic == strong, 'strong universe incomplete component coverage')
            counts['strong_universe_complete_component_coverage'] += 1

for S in (1, 1024, 2048):
    for jitter in (0, 1):
        P = [(0,0,0), (-4*S,5*S-jitter,0), (-4*S,-5*S+jitter,0),
             (4*S,0,5*S), (4*S,0,-5*S)]
        P = [tuple(c+5*S for c in p) for p in P]
        require(all(0 <= c <= 262143 for p in P for c in p), 'u18 domain')
        require(R.rank([R.sub(p, P[0]) for p in P[1:]]) == 3, 'not genuine 3D')
        alpha = check_firstcover(P, 3)
        judge = J.ReferenceArms(R, P, 3)
        check_all_covering_components(judge)
        a2, reps = judge.covering_at_alpha(0)
        a5, conflict5 = judge.unique_else_lca()
        a6, conflict6 = judge.unique_else_maj_invbeta()
        require(a2 == (5*S-jitter)**2 and alpha[0] == a2, 'first cover radius')
        require(conflict5[0] == (not jitter) and conflict6 == conflict5, 'conflict rule')
        expected = F(3249*S*S, 89) if not jitter else F((5*S-1)**2)
        for arm in (a5,a6):
            require(arm[0][0] == expected, 'unexpected x attachment date')
            require(judge.height(arm, 0, 1) == expected, 'unexpected projected reunion')
        if not jitter:
            require(len(reps)==2 and len(set(judge.labels(25*S*S).values()))==2, 'Gamma birth components')
            counts['gamma_distinct_birth_components'] += 1
        for mode in ('uniform', 'inverse_beta'):
            totals = judge.W(mode)
            for beta in sorted(set(judge.levels + [F(0), a2])):
                labels = judge.labels(beta)
                mass, reserve = {}, F(0)
                for x, wl in enumerate(judge.wit):
                    active = F(0)
                    for level, vertex, _ball in wl:
                        if level <= beta:
                            share = judge.weight(mode,level)/totals[x]
                            lab = labels[vertex]
                            mass[lab] = mass.get(lab,F(0))+share
                            active += share
                    reserve += 1-active
                require(sum(mass.values(),F(0))+reserve == len(P), 'mass conservation')
                counts['fractional_mass_identity'] += 1
        balls = [{'beta': str(b.level), 'p': b.p, 'qmin': b.qmin,
                  'strict_contains_x': 0 in b.I, 'population': list(b.I+b.U)}
                 for b in R.critical_balls(P)
                 if b.level == a2 and 0 in b.I+b.U and b.p+b.qmin<=3]
        if S >= 1024:
            require(all(b['strict_contains_x'] and b['p']==1 and b['qmin']==2 for b in balls),
                    'prioritary cases must remain strict interior, p1/q2')
        cut = 26*S*S
        partition5 = judge.partition(a5,cut)
        partition6 = judge.partition(a6,cut)
        expected_part = [[0,1,2],[3,4]] if jitter else [[0],[1,2],[3,4]]
        require(partition5 == expected_part and partition6 == expected_part, 'common-cut partition')
        same_labels = {(0,1,2):(0,1,2), (0,3,4):(0,3,4)}
        require(judge.labels(cut) == same_labels, 'intrinsic Gamma differs at common cut')
        active_cover_x = {judge.label(vertex,cut) for level,vertex,_ball in judge.wit[0] if level<=cut}
        require(active_cover_x == set(same_labels), 'x intrinsic covering branches differ')
        fractional_at_common_cut = {}
        for mode in ('uniform','inverse_beta'):
            totals = judge.W(mode)
            mass = {}
            reserve = F(0)
            for x, wl in enumerate(judge.wit):
                active = F(0)
                for level, vertex, _ball in wl:
                    if level <= cut:
                        share = judge.weight(mode,level)/totals[x]
                        lab = judge.label(vertex,cut)
                        mass[lab] = mass.get(lab,F(0))+share
                        active += share
                reserve += 1-active
            require(sum(mass.values(),F(0))+reserve == 5, 'common-cut mass identity')
            fractional_at_common_cut[mode] = {'component_mass':{str(t):str(m) for t,m in mass.items()},
                                              'reserve':str(reserve)}
        out.append({'S':S,'jitter':jitter,'points':P, 'alpha_x':str(a2),
                    'covering_components_at_alpha':len(reps), 'strong_first_balls':balls,
                    'Gamma_3_levels':[str(v) for v in judge.levels],
                    'A5_date_x':str(a5[0][0]),'A6_date_x':str(a6[0][0]),
                    'A5_height_x_a':str(judge.height(a5,0,1)),
                    'A6_height_x_a':str(judge.height(a6,0,1)),
                    'common_cut_beta':str(cut),'A5_partition':partition5,'A6_partition':partition6,
                    'intrinsic_Gamma_common_cut':{str(t):str(v) for t,v in same_labels.items()},
                    'fractional_at_common_cut':fractional_at_common_cut})

more = [([(0,0,0),(2,0,0),(4,0,0)],2),
        ([(0,0,0),(4,0,0),(0,5,0),(0,0,100)],2),
        ([(15,4,0),(5,4,0),(7,8,0),(7,0,0),(1,4,0),(0,4,1)],3),
        ([(325,325,650),(520,325,65),(200,325,25),(325,416,13),
          (325,130,65),(442,481,65),(250,225,25)],5)]
for P,K in more:
    check_firstcover(P,K)
    check_all_covering_components(J.ReferenceArms(R,P,K))
require(hashes == {'reference':sha(REF),'judge':sha(JUDGE)}, 'source changed')
print(json.dumps({'status':'PASS','scope':'bounded exact audit; no native/GCP',
                  'sources':hashes,'counts':counts,'K3_interior_tie_jitter':out},sort_keys=True,indent=1))
