import json
R="/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/g4_tower_r22_20260926"
V=[json.loads(open(f"{R}/vm/probe_{i}.stdout").read()) for i in range(36)]
c=lambda i:V[i]['times_ms']['chain_total']
for K,g,r in ((5,(0,13),(12,16)),(10,(2,15),(14,17))):
    for a in g:
        for b in r:
            print(f"K{K} gpu#{a} - r21#{b} = {c(a)-c(b):.2f} ms  adjacent_interleaved={abs(a-b)<=3}")
