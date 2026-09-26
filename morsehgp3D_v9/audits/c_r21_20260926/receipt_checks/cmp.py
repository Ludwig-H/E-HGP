import json
R="/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/g4_tower_r21_20260925"
S=json.load(open(R+"/SUMMARY.json"))
rows={r["idx"]:r for r in json.load(open("rows.json"))}
print("keys of comparison entries:", {tuple(sorted(c.keys())) for c in S["cross_worker_comparisons"]})
refs=set()
for c in S["cross_worker_comparisons"]:
    a,b=rows[c["reference"]],rows[c["other"]]
    same=(a["file"],a["K"])==(b["file"],b["K"])
    eq=all(a[k]==b[k] for k in ("digest","cat","pres","balls"))
    refs.add(c["reference"])
    print(f'{c["reference"]:2d}({a["arm"]},{a["file"][6:-6]},K{a["K"]}) vs {c["other"]:2d}({b["arm"]},K{b["K"]}) equal={c["equal"]} same_input={same} recomputed={eq}')
print("n=",len(S["cross_worker_comparisons"]))
covered=set()
for c in S["cross_worker_comparisons"]: covered|={c["reference"],c["other"]}
print("cases not in any comparison:", sorted(set(rows)-covered))
# other top-level fields
for k,v in S.items():
    if k not in ("rows","cross_worker_comparisons"): print(k, v if not isinstance(v,dict) else json.dumps(v))
