import json
R="/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/g4_tower_r21_20260925"
S=json.load(open(R+"/SUMMARY.json"))
KEYS=('expanded_pairs', 'witness_rejected_pairs', 'cover_builds', 'cover_sites','cover_node_visits', 'q3_edges', 'q4_edges', 'both_edges', 'dead_loads', 'dead_form_sites','dead_cells', 'dead_outside_cells', 'dead_deep_cells', 'dead_failed_cells','dead_uniform_tests', 'dead_point_tests', 'dead_q3_proved', 'dead_q3_open','dead_q4_proved', 'dead_q4_open', 'core_builds', 'core_sites', 'core_closed_edges','dead_core_loads', 'dead_core_form_sites', 'dead_core_cells', 'dead_core_uniform_tests','dead_core_point_tests', 'dead_core_q3_proved', 'dead_core_q3_open', 'dead_core_q4_proved','dead_core_q4_open', 'core_cover_node_visits', 'core_cover_bound_tests','core_cover_point_tests', 'dead_core_outside_cells', 'dead_core_deep_cells','dead_core_failed_cells')
V=[json.loads(open(f"{R}/vm/probe_{i}.stdout").read()) for i in range(34)]
def logical(v):
    g,c=v['generator'],v['catalogue']
    return (v['input']['hash'],v['input']['sites'],v['options']['K_effective'],c['unique_keys'],c['balls'],json.dumps(c['euler'],sort_keys=True),json.dumps(v['orders'],sort_keys=True),v['tower_digest'],v['catalogue_digest'],v['presentation_digest'],(g['q2_accepted_pairs'],g['q3_emitted'],g['q4_emitted'],c['q2_presentations'],c['q3_presentations'],c['q4_presentations']))
bad=0
for c in S['cross_worker_comparisons']:
    a,b=V[c['reference']],V[c['other']]
    l=logical(a)==logical(b)
    w=all(a['ledger'][k]==b['ledger'][k] for k in KEYS)
    diffk=[k for k in KEYS if a['ledger'][k]!=b['ledger'][k]]
    # tower_work differs?
    tw=[k for k in a['tower_work'] if a['tower_work'][k]!=b['tower_work'].get(k)]
    print(c['reference'],c['other'],'logical',l,'cert_work',w,diffk[:5],'tower_work_diff',tw[:6])
    bad+= (not l) or (not w)
print('bad',bad)
# Euler details
for i in (0,2,22,24,32):
    print(i, json.dumps(V[i]['catalogue']['euler'])[:600])
