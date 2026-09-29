import csv,json,sys
V="/workspaces/E-HGP/build/v10-perf/tour-verif"
groups=[
("replay","chaine_k5",["instr-c","p1c-c"]),
("replay","chaine_k10",["instr-c","p1c-c"]),
("replay_tour","tour_k5",["instr-c","p1c-c"]),
("wall_ab","chaine_k5",["base","p1c","p2c"])
]
w=csv.writer(sys.stdout,lineterminator="\n")
w.writerow(["group","series","variant","round","threads","n","K","only","points","verticals","wall","cpu","t_resolve","t_kruskal","kr_cpu_max_order","digest"])
for group,series,variants in groups:
 for variant in variants:
  for l in open(f"{V}/out/{group}/{series}.{variant}.jsonl"):
   if not l.startswith("{"):continue
   r=json.loads(l)
   orders=r.get("orders",[])
   kr=max(orders,key=lambda o:o["k"]).get("kr_cpu","") if orders else ""
   w.writerow([group,series,variant,r.get("round",""),r["threads"],r["n"],r["K"],r["only"],r["points"],r["verticals"],r["wall"],r["cpu"],r["t_resolve"],r["t_kruskal"],kr,r["digest"]])
