"""Six sites ; Γ3 collinéaire et intégrales exactes indépendantes, puis fonctions de sélection figées."""
from pathlib import Path
from fractions import Fraction as F
from itertools import combinations
import ast, json, hashlib

HERE = Path(__file__).resolve().parent

def require(ok, message):
    if not ok:
        raise RuntimeError(message)

P = [0, 1, 2, 10, 11, 12]
K, MCS = 3, F(2)
vertices = list(combinations(range(len(P)), K))
def meb(face):
    return F((max(P[x] for x in face) - min(P[x] for x in face)) ** 2, 4)

# Oracle Γ3 : vertices K-parties, edges (K+1)-parties, union complète à la coupe.
def gamma_supports(beta):
    active = [v for v in vertices if meb(v) <= beta]
    parent = {v:v for v in active}
    def root(v):
        while parent[v] != v:
            v = parent[v]
        return v
    for face in combinations(range(len(P)), K+1):
        if meb(face) <= beta:
            facets = list(combinations(face, K))
            for v in facets[1:]:
                parent[root(v)] = root(facets[0])
    groups = {}
    for v in active:
        groups.setdefault(root(v), set()).update(v)
    return sorted(sorted(v) for v in groups.values())

cuts = {str(b):gamma_supports(b) for b in [F(1), F(81,4), F(25)]}
require(cuts['1']==[[0,1,2],[3,4,5]], 'Γ birth components')
require(cuts['81/4']==[[0,1,2],[1,2,3],[2,3,4],[3,4,5]], 'Γ bridge components')
require(cuts['25']==[[0,1,2,3,4,5]], 'Γ simultaneous four-way fusion')

# Exact branch durations λ=1/β (z=2). Each of the six sites has leaf+root duration 1;
# x=1,11 also one bridge; x=2,10 two bridges. The contribution 19/2025 is exact.
delta = F(4,81)-F(1,25)
W = [F(1),1+delta,1+2*delta,1+2*delta,1+delta,F(1)]
a = sum(1/W[x] for x in [0,1,2])
ab = sum(1/W[x] for x in [1,2,3])
require(a==F(12533447,4216772), 'independent left coefficient')
ld = F(1,25)
leaf_end = a*(1-ld)
bridge_end = ab*delta
root_initial = 2*leaf_end+2*bridge_end
require(root_initial==6-2*a*ld, 'conservation at four-way split')
leaf_full_score = a*(1-ld)**2/2
bridge_score = ab*delta**2/2
root_score = root_initial*ld+(2*a)*ld**2/2
stop_lambda = 1-MCS/a
require(ld<stop_lambda<F(1,2)<1, 'interior threshold then below mcs')
leaf_mass_half = a*F(1,2)
require(leaf_end>MCS and leaf_mass_half<MCS, 'mass crossing within a geometry segment')
extra_score = MCS*MCS/(2*a)
clipped_score = leaf_full_score-extra_score
require(extra_score==F(8433544,12533447), 'tail area under mcs')
require(leaf_full_score==F(902408184,658870625), 'full score')

# Consume exact recorded source bodies. Replace no body: only Fraction cmp and tiny data adapters.
source = (HERE/'selection.source.txt').read_text()
mod = ast.parse(source)
names = {'NodeStats','condense','eom','depth_of','unselect','ancestor_selected'}
body = [n for n in mod.body if isinstance(n,(ast.ClassDef,ast.FunctionDef)) and n.name in names]
require({n.name for n in body}==names, 'recorded source functions')
ns = {'Fraction':F,'cmp':lambda x,y,soft=False:(x>y)-(x<y)}
exec(compile(ast.fix_missing_locations(ast.Module(body=body,type_ignores=[])), 'selection.source.txt', 'exec'),ns)
class Tree:
    root=4
    birth=[F(1),F(1),F(81,4),F(81,4),F(25)]
    death=[F(25),F(25),F(25),F(25),None]
    parent=[4,4,4,4,-1]
    children=[[],[],[],[],[0,1,2,3]]
T=Tree()
stats=ns['NodeStats']([leaf_end,leaf_end,bridge_end,bridge_end,F(6)],
                       [leaf_full_score,leaf_full_score,bridge_score,bridge_score,root_score])
clusters=ns['condense'](T,stats,MCS)
chosen,scores=ns['eom'](T,stats,clusters)
require([clusters[i]['top'] for i in chosen]==[0,1], 'selected leaves')
require(scores[chosen[0]]==leaf_full_score, 'consumer integrates full geometric leaf life')

result={
 'scope':'private proposal11 semantics; no HEAD defect and no statistical result',
 'points_xyz':[[x,0,0] for x in P], 'K':K,'z':2,'mcs':str(MCS),
 'gamma_supports_at_cuts':cuts,
 'site_total_weights':[str(w) for w in W], 'leaf_coefficient':str(a),
 'leaf_mass_function':'a*(1-lambda), lambda in [1/25,1]',
 'leaf_end_mass_at_split':str(leaf_end), 'leaf_mass_at_lambda_1_2':str(leaf_mass_half),
 'leaf_mass_at_geometric_birth':'0', 'threshold_lambda':str(stop_lambda),
 'root_progressive_mass_at_birth':str(root_initial),
 'sum_child_end_mass':str(2*leaf_end+2*bridge_end),
 'prototype_condensed_clusters':clusters, 'prototype_chosen_tops':[clusters[i]['top'] for i in chosen],
 'prototype_leaf_score':str(leaf_full_score),
 'score_if_end_at_first_mass_lt_mcs':str(clipped_score),
 'extra_score_from_below_mcs_tail':str(extra_score),
 'step_leaf_mass':'constant leaf_end_mass until geometric birth; no same interior stop',
 'invariants':{'at_split_mass_not_anticipatory':True,'parent_birth_equals_sum_child_ends_progressive':True,
               'end_masses_do_not_determine_chronological_terminal_dates':True,
               'antichain_property_not_refuted':True,'label_change_not_claimed':True},
 'recorded_selection_source_sha256':hashlib.sha256(source.encode()).hexdigest(),
}
print(json.dumps(result,ensure_ascii=False,indent=2,sort_keys=True))
