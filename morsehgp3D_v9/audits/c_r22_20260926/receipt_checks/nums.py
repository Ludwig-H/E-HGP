import json
R="/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9/receipts/"
rows={r["idx"]:r for r in json.load(open("rows.json"))}
V=[json.loads(open(f"{R}g4_tower_r22_20260926/vm/probe_{i}.stdout").read()) for i in range(36)]
S21=json.load(open(R+"g4_tower_r21_20260925/SUMMARY.json"))
V21=[json.loads(open(f"{R}g4_tower_r21_20260925/vm/probe_{i}.stdout").read()) for i in range(34)]
c=lambda i: round(V[i]['times_ms']['chain_total']/1000,3)
print("README chain table (s):")
tab=[("000100",5,5,4,None,None,None,None),("000000",5,1,(0,13),(12,16),18,20,22),("000200",5,9,8,None,None,None,None),
     ("000100",10,7,6,None,None,None,None),("000000",10,3,(2,15),(14,17),19,21,23),("000200",10,11,10,None,None,None,None),
     ("b00",5,25,24,None,None,None,None),("b01",5,29,28,None,None,None,None),("b02",5,33,32,None,None,None,None),
     ("b00",10,27,26,None,None,None,None),("b01",10,31,30,None,None,None,None),("b02",10,35,34,None,None,None,None)]
f=lambda x: None if x is None else (c(x) if isinstance(x,int) else tuple(c(y) for y in x))
for t in tab:
    print(t[0],t[1],"engine",f(t[2]),"gpu",f(t[3]),"gpu_r21",f(t[4]),"no_seal",f(t[5]),"no_early",f(t[6]),"no_pinned",f(t[7]))
# R21 column: identify R21 gpu arm rows by file/K
print("R21 gpu-arm rows (chain s):")
P21=json.load(open(R+"g4_tower_r21_20260925/PACKAGE.json"))
GPU=["q34_batch_filter","q34_gpu_filter","q34_batch_certificates","q34_gpu_certificates","q34_batch_q3","q34_gpu_q3","q34_batch_q4","q2_during_device","device_session","tower_pipelined_tail","tower_hash_grouping","tower_persistent_pool"]
for i,v in enumerate(V21):
    lv=v['options']['levers']
    if all(lv[k] for k in GPU):
        print(" ",i, P21['cases'][i]['file'].split('/')[-1], v['options']['K'], round(v['times_ms']['chain_total']/1000,3))
