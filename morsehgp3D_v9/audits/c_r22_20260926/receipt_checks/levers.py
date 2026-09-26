import json
R="/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/"
rows=json.load(open("rows.json"))
base=rows[0]["opts"]["levers"]
for r in rows:
    lv=r["opts"]["levers"]
    d={k:v for k,v in lv.items() if base.get(k)!=v}
    print(r["idx"], r["file"][6:-6], r["K"], r["arm"], "diff vs case0:", d)
# R21 gpu arm levers
P21=json.load(open(R+"g4_tower_r21_20260925/PACKAGE.json"))
S21=json.load(open(R+"g4_tower_r21_20260925/SUMMARY.json"))
l21=P21["cases"][0]["levers"]
print("R21 case0 levers:", l21)
g=rows[12]["opts"]["levers"]
print("R22 gpu_r21 vs R21 case0 diff:", {k:(g.get(k),l21.get(k)) for k in set(g)|set(l21) if g.get(k)!=l21.get(k)})
e21=P21["cases"][1]["levers"]; e22=rows[1]["opts"]["levers"]
print("R22 engine vs R21 case1 diff:", {k:(e22.get(k),e21.get(k)) for k in set(e22)|set(e21) if e22.get(k)!=e21.get(k)})
# other options
for r in rows:
    o={k:v for k,v in r["opts"].items() if k!="levers"}
    if r["idx"]==0: o0=o
    if o!=o0 and not (o["K"]!=o0["K"]): print("opts differ", r["idx"], o)
print({(r["opts"]["K"],r["opts"]["K_effective"],r["opts"]["s"],r["opts"]["workers"],r["opts"]["tower_static_threads"],r["opts"]["certificate_capacity"],r["opts"]["lanes_capacity"],r["opts"]["certificate_judge"],r["opts"]["lanes_judge"]) for r in rows})
