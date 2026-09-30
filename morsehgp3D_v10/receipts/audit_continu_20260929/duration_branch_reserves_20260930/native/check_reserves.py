"""Exact small geometry checks for durationleaf reserves, audit only."""
from fractions import Fraction as Q
from itertools import combinations
from pathlib import Path
import hashlib
import importlib.util
import json
import sys

if len(sys.argv) != 2:
    raise SystemExit('usage: check_reserves.py FROZEN_FRACTION_REFERENCE')
ref_path = Path(sys.argv[1]).resolve()
inputs = (Path(__file__).resolve(),ref_path)
before = {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
spec = importlib.util.spec_from_file_location('frozen_reference',ref_path)
ref = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ref)


def check(value,message):
    if not value:
        raise RuntimeError(message)


def gamma_tree(P,k):
    vertices = {F:ref.meb(P,F)[0] for F in combinations(range(len(P)),k)}
    edges = {G:ref.meb(P,G)[0] for G in combinations(range(len(P)),k+1)}
    levels = sorted(set(vertices.values()) | set(edges.values()))
    dsu,alive,branch,nodes,trace = ref.DSU(),set(),{},[],[]
    exact_levels,closed,_opened = ref.gamma_cuts(P,k)
    check(levels == exact_levels,'Gamma levels differ')
    for step,beta in enumerate(levels):
        prior = branch.copy()
        for F,born in vertices.items():
            if born == beta:
                dsu.find(F)
                alive.add(F)
        for G,joined in edges.items():
            if joined == beta:
                Fs = list(combinations(G,k))
                for F in Fs[1:]:
                    dsu.union(Fs[0],F)
        comps = {}
        for F in alive:
            comps.setdefault(dsu.find(F),[]).append(F)
        branch,snapshot = {},[]
        for root in sorted(comps):
            Fs = comps[root]
            previous = sorted({prior[F] for F in Fs if F in prior})
            if len(previous) == 1:
                v = previous[0]
            else:
                v = len(nodes)
                nodes.append({'id':v,'birth':beta,'death':None,'parent':None,
                              'leaf':not previous,'children':previous,'first':{}})
                for c in previous:
                    check(nodes[c]['death'] is None,'branch dies twice')
                    nodes[c]['death'],nodes[c]['parent'] = beta,v
            points = sorted({x for F in Fs for x in F})
            for F in Fs:
                branch[F] = v
            for x in points:
                nodes[v]['first'].setdefault(x,beta)
            snapshot.append({'node':v,'points':points})
        check(sorted(s['points'] for s in snapshot) == sorted(sorted(c) for c in closed[step]),'coverage oracle differs')
        trace.append({'beta':beta,'components':snapshot})
    check(sum(n['parent'] is None for n in nodes) == 1,'final tree not rooted')
    return nodes,trace


def serialize_nodes(nodes):
    return [{**{k:v for k,v in n.items() if k not in ('birth','death','first')},
             'birth':str(n['birth']), 'death':None if n['death'] is None else str(n['death']),
             'first':{str(x):str(t) for x,t in sorted(n['first'].items())}} for n in nodes]


def serialize_trace(trace):
    return [{'beta':str(row['beta']),'components':row['components']} for row in trace]


def proof_missing_leaf(P,k):
    check(all(min(p)>=0 and max(p)<=262143 for p in P),'not u18')
    check(ref.rank([ref.sub(p,P[0]) for p in P[1:]]) == 3,'not full dimensional')
    nodes,trace = gamma_tree(P,k)
    leaf_x = [n['id'] for n in nodes if n['leaf'] and 0 in n['first']]
    internal_x = [n['id'] for n in nodes if not n['leaf'] and 0 in n['first']]
    check(not leaf_x and internal_x,'point x does have a leaf or lacks internal coverage')
    balls = [b for b in ref.critical_balls(P)
             if b.p+b.m>=k and b.p+b.qmin<=k and 0 in b.I+b.U]
    check(len(balls) == 1,'unexpected covering atom count')
    local = ref.local_structure(P,balls[0],k)
    check(local[0] == 'join','covering ball is a birth')
    return {'points':P,'k':k,'affine_dimension':3,'leaf_x':leaf_x,'internal_x':internal_x,
            'nodes':serialize_nodes(nodes),'trace':serialize_trace(trace),
            'unique_strong_covering_ball':{'beta':str(balls[0].level),'I':list(balls[0].I),
                                         'U':list(balls[0].U),'qmin':balls[0].qmin,
                                         'local_kind':local[0],'local_pieces':[list(F) for F in local[1]]}}


def nearest_K2_proof_checks(P):
    nodes,_trace = gamma_tree(P,2)
    rows = []
    for x in range(len(P)):
        a = min((j for j in range(len(P)) if j != x),key=lambda j:(ref.d2(P[x],P[j]),j))
        F = tuple(sorted((x,a)))
        beta,center = ref.meb(P,F)
        contacts = [j for j in range(len(P)) if ref.d2(center,P[j])<=beta]
        check(contacts == list(F),'nearest diameter not empty outside endpoints')
        for z in range(len(P)):
            if z not in F:
                check(ref.meb(P,tuple(sorted(F+(z,))))[0] > beta,'nearest pair has simultaneous incident edge')
        birth_nodes = [n['id'] for n in nodes if n['leaf'] and n['birth']==beta and n['first'].get(x)==beta]
        check(birth_nodes,'nearest pair did not give a birth leaf')
        rows.append({'x':x,'nearest':a,'beta':str(beta),'birth_leaf_nodes':birth_nodes})
    return rows


def ghost_fixture(base,N):
    P = [tuple(N*a+325*N for a in p) for p in base]
    shifted = list(P)
    shifted[0] = (P[0][0],P[0][1],P[0][2]-1)
    check(all(min(p)>=0 and max(p)<=262143 for cloud in (P,shifted) for p in cloud),'ghost fixture not u18')
    original,original_trace = gamma_tree(P,5)
    moved,moved_trace = gamma_tree(shifted,5)
    root0 = next(n for n in original if n['parent'] is None)
    continuation = [n for n in moved if n['birth']==root0['birth'] and sorted(n['first'])==[1,2,3,4,5,6]]
    check(len(continuation)==1,'old cap arc was not preserved before its new death')
    oldarc = continuation[0]
    check(oldarc['death'] is not None,'cap arc was not segmented')
    ghost_leaves = [n for n in moved if n['leaf'] and 0 in n['first']]
    check(ghost_leaves and not any(n['leaf'] and 0 in n['first'] for n in original),'no new ghost incidences')
    check(all(n['death'] is not None for n in ghost_leaves),'new leaves never die')
    # Weights in normalized geometry, phi=1/beta, for transparent rationals.
    original_arc_weight = N*N/root0['birth']
    cut_arc_weight = N*N*(1/oldarc['birth']-1/oldarc['death'])
    ghost_weights = [N*N*(1/n['birth']-1/n['death']) for n in ghost_leaves]
    check(all(w>0 for w in ghost_weights),'ghost weight nonpositive')
    return {'N':N,'point_displacement_grid':1,'normalized_displacement':str(Q(1,N)),
            'original_nodes':serialize_nodes(original),'original_trace':serialize_trace(original_trace),
            'moved_nodes':serialize_nodes(moved),'moved_trace':serialize_trace(moved_trace),
            'long_arc_birth_normalized':str(root0['birth']/(N*N)),
            'new_long_arc_death_normalized':str(oldarc['death']/(N*N)),
            'original_arc_weight':str(original_arc_weight),'segmented_arc_weight':str(cut_arc_weight),
            'lost_long_arc_weight':str(original_arc_weight-cut_arc_weight),
            'new_ghost_leaf_weights':list(map(str,ghost_weights)),
            'total_ghost_leaf_weight':str(sum(ghost_weights,Q(0)))}


def main():
    k3 = [(15,4,0),(5,4,0),(7,8,0),(7,0,0),(1,4,0),(0,4,1)]
    cap = [(0,0,325),(195,0,-260),(-125,0,-300),(0,91,-312),
           (0,-195,-260),(117,156,-260),(-75,-100,-300)]
    k5 = [tuple(a+325 for a in p) for p in cap]
    missing = {'k3_six_sites':proof_missing_leaf(k3,3), 'k5_seven_sites':proof_missing_leaf(k5,5)}
    check(missing['k3_six_sites']['unique_strong_covering_ball']['beta']=='25','wrong K3 entry')
    check(missing['k5_seven_sites']['unique_strong_covering_ball']['beta']=='105625','wrong K5 entry')
    nearest_checks = {'k3_fixture_as_K2':nearest_K2_proof_checks(k3),
                      'k5_fixture_as_K2':nearest_K2_proof_checks(k5),
                      'symmetric_ties':nearest_K2_proof_checks([(0,0,0),(1,0,0),(0,1,0),(0,0,1),(-1,0,0)])}
    ghosts = [ghost_fixture(cap,N) for N in (64,128,256)]
    ghost_totals = [Q(g['total_ghost_leaf_weight']) for g in ghosts]
    check(ghost_totals[0]>ghost_totals[1]>ghost_totals[2]>0,'ghost weight did not diminish')
    lost = [Q(g['lost_long_arc_weight']) for g in ghosts]
    check(all(Q(9,1000000)<w<Q(1,100000) for w in lost),'long arc loss did not stay nonzero')
    after = {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
    check(before == after,'source changed during audit')
    print(json.dumps({'status':'DURATIONLEAF_RESERVES_GEOMETRICALLY_RESOLVED',
                      'scope':'K2_theorem_controls_K3_K5_missing_leaf_and_K5_ghost_small_exact_Gamma',
                      'native_calls':0,'GCP_used':False,'engine_modified':False,
                      'argv':sys.argv,'optimized':sys.flags.optimize,
                      'hashes_before':before,'hashes_after':after,
                      'missing_leaf':missing,'nearest_K2_controls':nearest_checks,'ghosts':ghosts},indent=2))


if __name__ == '__main__':
    main()
