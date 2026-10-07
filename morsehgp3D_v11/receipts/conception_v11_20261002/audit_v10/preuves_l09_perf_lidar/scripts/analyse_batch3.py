#!/usr/bin/env python3
"""Lot 3 : composition du juge (B) contre base (A), chemin FULL sans attaches ; ISA v3 contre defaut."""
import json, os, re, statistics
O = "/tmp/v11-audit/l09_perf_lidar/runs3"
def load(name):
    p = os.path.join(O, name + ".json")
    js = []
    if os.path.exists(p):
        for line in open(p):
            line = line.strip()
            if line.startswith("{"):
                try: js.append(json.loads(line))
                except Exception: pass
    t = {}
    tp = os.path.join(O, name + ".time")
    if os.path.exists(tp):
        for line in open(tp):
            m = re.match(r"\s*(.+?): (.+)$", line)
            if m: t[m.group(1).strip()] = m.group(2).strip()
    return (js[0] if js else None), t
def cpu(t): return float(t["User time (seconds)"]) + float(t["System time (seconds)"])
out = []; P = out.append
P("== composition du juge (B) contre base (A) : mhgp10_tower --no-points, une passe, temps CPU user+sys")
for f, k, reps in (("01", 5, (1, 2)), ("02", 5, (1, 2)), ("01", 10, (1,))):
    A = []; B = []
    for r in reps:
        for tag, lst in (("A", A), ("A2", A), ("B", B), ("B2", B)):
            j, t = load(f"{tag}_l{f}_k{k}_r{r}")
            if j and t: lst.append((cpu(t), j, int(t["Maximum resident set size (kbytes)"])))
    if not A or not B: P(f"l{f} K={k}: manquant"); continue
    ca = [x[0] for x in A]; cb = [x[0] for x in B]
    ja = A[0][1]; jb = B[0][1]
    same_nodes = [o["nodes"] for o in ja["orders"]] == [o["nodes"] for o in jb["orders"]] and ja["balls"] == jb["balls"]
    same_merges = [(o["births"], o["merges"], o["joins"]) for o in ja["orders"]] == [(o["births"], o["merges"], o["joins"]) for o in jb["orders"]]
    P(f"l{f} K={k}: CPU base {['%.2f' % x for x in ca]} mediane {statistics.median(ca):.2f} s ; composition {['%.2f' % x for x in cb]} mediane {statistics.median(cb):.2f} s ; rapport x{statistics.median(ca)/statistics.median(cb):.2f} [min x{min(ca)/max(cb):.2f} ; max x{max(ca)/min(cb):.2f}] ; boules et noeuds par ordre identiques: {same_nodes} ; (naissances, fusions, jonctions) identiques: {same_merges} ; pic RSS base {A[0][2]} kB composition {B[0][2]} kB")
    # etages (mur, indicatif)
    def st(j):
        cs = j["catalogue_stages"]; s = j["stages"]
        return dict(front=cs["t_frontier"], boites=cs["t_boxes"], ordre=cs["t_order"], ass=cs["t_assemble"], cat=j["catalogue_s"], tour=j["tower_s"], resolve=s["t_resolve"], kruskal=s["t_kruskal"], vert=s["t_vertical"])
    sa = [st(x[1]) for x in A]; sb = [st(x[1]) for x in B]
    keys = list(sa[0].keys())
    P("      mediane des etages (mur local, indicatif) base -> composition : " + " ; ".join(f"{k_} {statistics.median(x[k_] for x in sa):.3f} -> {statistics.median(x[k_] for x in sb):.3f} (x{statistics.median(x[k_] for x in sa)/max(1e-9, statistics.median(x[k_] for x in sb)):.2f})" for k_ in keys))
P("")
P("== ISA : -march=x86-64-v3 (B) contre defaut (A), mhgp10_catalogue, 1 fil")
for f, k, tags in (("01", 5, ("isaA", "isaA2", "isaB", "isaB2")), ("01", 10, ("isaA", "isaB"))):
    A = []; B = []
    for tag in tags:
        j, t = load(f"{tag}_l{f}_k{k}")
        if j and t: (A if "A" in tag else B).append((cpu(t), j))
    if not A or not B: P(f"l{f} K={k}: manquant"); continue
    ja = A[0][1]; jb = B[0][1]
    keys = ["balls","levels","nodes","leaves","sum_m","filter_tests","pair_tests","triple_tests","quad_tests","judged"]
    same = all(ja[x] == jb[x] for x in keys) and ja["by_q_p"] == jb["by_q_p"]
    ca = statistics.median(x[0] for x in A); cb = statistics.median(x[0] for x in B)
    ba = statistics.median(x[1]["catalogue_stages"]["t_boxes"] for x in A); bb = statistics.median(x[1]["catalogue_stages"]["t_boxes"] for x in B)
    P(f"l{f} K={k}: CPU defaut {ca:.2f} s ; v3 {cb:.2f} s ; rapport x{ca/cb:.2f} ; t_boxes (mur) {ba:.2f} -> {bb:.2f} (x{ba/bb:.2f}) ; compteurs et boules identiques: {same}")
open(os.path.join(O, "ANALYSE3.txt"), "w").write("\n".join(out) + "\n")
print("\n".join(out))
