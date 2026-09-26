import json, math
R="/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/g4_tower_r22_20260926"
rows=json.load(open("rows.json"))
V=[json.loads(open(f"{R}/vm/probe_{i}.stdout").read()) for i in range(36)]
print("idx file K arm balls sampled ceil(b/64) floor(b/64) decl extra regsup_sum b-extra early_keys q2_acc q2_pres")
for r in rows:
    c=V[r["idx"]]["catalogue"]
    print(r["idx"], r["file"][6:-6], r["K"], r["arm"][:28], r["balls"], r["sampled"], math.ceil(r["balls"]/64), r["balls"]//64, r["decl"], c["extra_shell_balls"], sum(c["regular_supports"]), r["balls"]-c["extra_shell_balls"], r["early_keys"], r["q2_acc"], r["q2_pres"], c.get("early_census_extra_shell_balls"))
