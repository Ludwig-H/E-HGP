import json
R="/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/g4_tower_r22_20260926"
S=json.load(open(R+"/SUMMARY.json"))
rows={r["idx"]:r for r in json.load(open("rows.json"))}
V=[json.loads(open(f"{R}/vm/probe_{i}.stdout").read()) for i in range(36)]
print("keys of comparison entries:", {tuple(sorted(c.keys())) for c in S["cross_worker_comparisons"]})
def logical(v):
    g,c=v['generator'],v['catalogue']
    return (v['input']['hash'],v['input']['sites'],v['options']['K_effective'],c['unique_keys'],c['balls'],json.dumps(c['euler'],sort_keys=True),json.dumps(v['orders'],sort_keys=True),v['tower_digest'],v['catalogue_digest'],v['presentation_digest'],(g['q2_accepted_pairs'],g['q3_emitted'],g['q4_emitted'],c['q2_presentations'],c['q3_presentations'],c['q4_presentations']),json.dumps(c['by_qmin']),json.dumps(c['by_shell']),c['extra_shell_balls'],c['max_shell'],c['max_interior'],json.dumps(c.get('regular_supports')))
bad=0; covered=set()
for c in S["cross_worker_comparisons"]:
    a,b=rows[c["reference"]],rows[c["other"]]
    same=(a["file"],a["K"])==(b["file"],b["K"])
    eq=all(a[k]==b[k] for k in ("digest","cat","pres","balls"))
    L=logical(V[c["reference"]])==logical(V[c["other"]])
    diff=[i for i,(x,y) in enumerate(zip(logical(V[c["reference"]]),logical(V[c["other"]]))) if x!=y]
    covered|={c["reference"],c["other"]}
    # tower_work compare
    tw=[k for k in V[c['reference']]['tower_work'] if V[c['reference']]['tower_work'][k]!=V[c['other']]['tower_work'].get(k)]
    print(f'{c["reference"]:2d}({a["arm"]},{a["file"][6:-6]},K{a["K"]}) vs {c["other"]:2d}({b["arm"]},K{b["K"]}) equal={c["equal"]} same_input={same} recomputed={eq} logical={L} {diff} tower_work_diff={tw}')
    bad += (not (c["equal"] and same and eq and L))
print("n=",len(S["cross_worker_comparisons"]), "bad", bad)
print("cases not in any comparison:", sorted(set(rows)-covered))
