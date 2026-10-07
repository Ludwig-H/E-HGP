#!/usr/bin/env python3
"""Grandeurs derivees des recus G4 (sessions 1 a 5) : couts par boule, parts, extrapolations. Lecture seule."""
import json, os, csv
ROOT = "/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10/receipts"
def load(sess, cmd):
    p = os.path.join(ROOT, sess, "results", "cmd", cmd, "stdout")
    for line in open(p):
        line = line.strip()
        if line.startswith("{"): return json.loads(line)
def time_of(sess, cmd):
    t = {}
    for line in open(os.path.join(ROOT, sess, "results", "cmd", cmd, "time.txt")):
        if ": " in line:
            k, v = line.strip().rsplit(": ", 1); t[k.strip()] = v
    return t
S4 = "g4_session4_j2c_20260929"; S1 = "g4_session1_20260929"
print("== couts par boule a 1 fil (G4)")
j5 = load(S4, "001_cat_lidar02_k5_w1"); j10 = load(S4, "002_cat_lidar02_k10_w1"); t1 = load(S1, "008_c0_lidar02_k5_w1")
for j in (j5, j10):
    b = j["balls"]; cs = j["catalogue_stages"]
    print(f"K={j['K']}: catalogue {j['catalogue_s']*1e6/b:.2f} us/boule dont boites {cs['t_boxes']*1e6/b:.2f}, ordre {cs['t_order']*1e6/b:.3f}, assemblage {cs['t_assemble']*1e6/b:.3f}, frontiere {cs['t_frontier']*1e6/b:.3f}")
print(f"K=5 tour 1 fil (s1): {t1['tower_s']*1e6/t1['balls']:.2f} us/boule ; resolve {t1['stages']['t_resolve']*1e6/t1['balls']:.2f}")
print()
print("== 48 fils, session 4 : ns de mur par boule et par site, parts")
rows = []
for f, c5, c10 in (("00", "008_tower_lidar00_k5_w48", "009_tower_lidar00_k10_w48"), ("01", "010_tower_lidar01_k5_w48", "011_tower_lidar01_k10_w48"), ("02", "012_tower_lidar02_k5_w48", "013_tower_lidar02_k10_w48")):
    for c in (c5, c10):
        j = load(S4, c); t = time_of(S4, c); b = j["balls"]; n = j["n"]; tot = j["catalogue_s"] + j["tower_s"]
        cs = j["catalogue_stages"]; st = j["stages"]
        nodes = sum(o["nodes"] for o in j["orders"])
        print(f"l{f} K={j['K']}: total {1000*tot:.1f} ms = {tot*1e9/b:.1f} ns/boule = {tot*1e6/n:.2f} us/site = {tot*1e9/nodes:.1f} ns/noeud de tour ({nodes} noeuds, {nodes/b:.2f}/boule, {nodes/n:.1f}/site) ; boules/site {b/n:.1f}")
        print(f"      parts: boites {100*cs['t_boxes']/tot:.1f}% frontiere {100*cs['t_frontier']/tot:.1f}% ordre {100*cs['t_order']/tot:.1f}% assemblage {100*cs['t_assemble']/tot:.1f}% hors_etages_cat {100*(j['catalogue_s']-cs['t_frontier']-cs['t_boxes']-cs['t_order']-cs['t_assemble'])/tot:.1f}% | tour: index {100*st['t_prepare']/tot:.1f}% atlas {100*st['t_local']/tot:.1f}% semis {100*st['t_seeds']/tot:.1f}% descentes {100*st['t_resolve']/tot:.1f}% kruskal {100*st['t_kruskal']/tot:.1f}% verticales {100*st['t_vertical']/tot:.1f}%")
        u = float(t["User time (seconds)"]); s = float(t["System time (seconds)"]); pf = int(t["Minor (reclaiming a frame) page faults"])
        print(f"      processus (3 passes): user {u:.2f}s sys {s:.2f}s -> {1e6*(u+s)/3/b:.2f} us CPU/boule/passe ; defauts de page mineurs {pf} ({pf/3:.0f}/passe, {pf*4096/3/b:.0f} o/boule/passe) ; sys/(user+sys) {100*s/(u+s):.1f}%")
        rows.append((f, j["K"], n, b, tot))
print()
print("== extrapolation lineaire en sites (us/site constants) aux tailles de trames de la sequence 08 (criblage 1 sur 8)")
import statistics
for K in (5, 10):
    us = [tot*1e6/n for (f, k, n, b, tot) in rows if k == K]
    lo, hi = min(us), max(us)
    for name, n in (("mediane", 62342), ("p75", 76948), ("p90", 84609), ("p95", 89995), ("max", 101472)):
        print(f"K={K} {name:8s} n={n}: {lo*n/1e3:.0f} a {hi*n/1e3:.0f} ms (x{lo*n/1e5:.1f} a x{hi*n/1e5:.1f} le contrat de 100 ms)")
print()
print("== Amdahl catalogue (trame 02, session 4)")
for K, cmds in ((5, ("001_cat_lidar02_k5_w1", "003_cat_lidar02_k5_w12", "004_cat_lidar02_k5_w24", "005_cat_lidar02_k5_w48")), (10, ("002_cat_lidar02_k10_w1", "006_cat_lidar02_k10_w24", "007_cat_lidar02_k10_w48"))):
    base = load(S4, cmds[0])
    for c in cmds[1:]:
        j = load(S4, c); P = j["threads"]
        r = j["catalogue_s"] / base["catalogue_s"]
        s = (r - 1.0 / P) / (1 - 1.0 / P)
        cs = j["catalogue_stages"]; bs = base["catalogue_stages"]
        print(f"K={K} P={P}: acceleration x{1/r:.1f} ; fraction serie equivalente (Amdahl a P fils) {100*s:.2f}% du temps a 1 fil = {1000*s*base['catalogue_s']:.0f} ms ; par etage: boites x{bs['t_boxes']/cs['t_boxes']:.1f} frontiere x{bs['t_frontier']/cs['t_frontier']:.1f} ordre x{bs['t_order']/cs['t_order']:.1f} assemblage x{bs['t_assemble']/cs['t_assemble']:.1f}")
print()
print("== SMT : 24 -> 48 fils")
a = load(S4, "004_cat_lidar02_k5_w24"); b_ = load(S4, "005_cat_lidar02_k5_w48")
print(f"catalogue K5: boites {a['catalogue_stages']['t_boxes']/b_['catalogue_stages']['t_boxes']:.2f}")
a = load(S4, "006_cat_lidar02_k10_w24"); b_ = load(S4, "007_cat_lidar02_k10_w48")
print(f"catalogue K10: boites {a['catalogue_stages']['t_boxes']/b_['catalogue_stages']['t_boxes']:.2f}")
a = load(S4, "020_tower_lidar02_k5_w24"); b_ = load(S4, "012_tower_lidar02_k5_w48")
print(f"tour K5: descentes {a['stages']['t_resolve']/b_['stages']['t_resolve']:.2f} ; catalogue+tour {(a['catalogue_s']+a['tower_s'])/(b_['catalogue_s']+b_['tower_s']):.2f}")
