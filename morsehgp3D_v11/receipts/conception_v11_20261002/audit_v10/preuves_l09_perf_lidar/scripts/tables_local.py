#!/usr/bin/env python3
"""Blocs Markdown des mesures locales L09 (lots 1 a 3)."""
import json, os, re, sys
R1 = "/tmp/v11-audit/l09_perf_lidar/runs"; R2 = "/tmp/v11-audit/l09_perf_lidar/runs2"; R3 = "/tmp/v11-audit/l09_perf_lidar/runs3"
def load(d, name):
    p = os.path.join(d, name + ".json")
    js = []
    if os.path.exists(p):
        for line in open(p):
            line = line.strip()
            if line.startswith("{"):
                try: js.append(json.loads(line))
                except Exception: pass
    t = {}
    tp = os.path.join(d, name + ".time")
    if os.path.exists(tp):
        for line in open(tp):
            m = re.match(r"\s*(.+?): (.+)$", line)
            if m: t[m.group(1).strip()] = m.group(2).strip()
    return js, t
def fr(x, d=1): return f"{x:.{d}f}".replace(".", ",")
def sp(n): return f"{int(n):,}".replace(",", " ")
def cpu(t): return float(t["User time (seconds)"]) + float(t["System time (seconds)"])
def rss(t): return int(t["Maximum resident set size (kbytes)"])
print("### L1 compteurs catalogue (local, identiques a 1 et 4 fils)")
print("| Trame | K | Boules | Niveaux | Nœuds | Feuilles | Σm | Tests du filtre | Paires | Triplets | Quadruplets | Jugements | Identiques à 1 et 4 fils | Identiques au reçu G4 |")
print("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |")
G4 = "/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10/receipts/g4_session5_scale_20260929/scale.csv"
import csv
g4 = {(r["file"], r["k"]): r for r in csv.DictReader(open(G4)) if r["status"] == "ok"}
keys = ["balls","levels","nodes","leaves","sum_m","max_m","skipped_bbox","filter_tests","leaf_dominance_tests","pair_tests","triple_tests","line_hits","quad_tests","judged","extended","max_shell","stalled_leaves"]
for f in ("00", "01", "02"):
    for k in (5, 10):
        (a,), ta = load(R1, f"cat_l{f}_k{k}_w1")[0][:1], None
        b = load(R1, f"cat_l{f}_k{k}_w4")[0][0]
        same = all(a[x] == b[x] for x in keys) and a["by_q_p"] == b["by_q_p"]
        g = g4[(f"lidar{f}_full.u32le", str(k))]
        sameg = (int(g["balls"]) == a["balls"] and int(g["cat_nodes"]) == a["nodes"] and int(g["cat_leaves"]) == a["leaves"] and int(g["cat_sum_m"]) == a["sum_m"] and int(g["cat_judged"]) == a["judged"] and int(g["cat_quad_tests"]) == a["quad_tests"] and int(g["cat_triple_tests"]) == a["triple_tests"])
        print(f"| {f} | {k} | {sp(a['balls'])} | {sp(a['levels'])} | {sp(a['nodes'])} | {sp(a['leaves'])} | {sp(a['sum_m'])} | {sp(a['filter_tests'])} | {sp(a['pair_tests'])} | {sp(a['triple_tests'])} | {sp(a['quad_tests'])} | {sp(a['judged'])} | {'oui' if same else 'NON'} | {'oui' if sameg else 'NON'} |")
print()
print("### L2 temps CPU par boule (local, /usr/bin/time, user+sys)")
print("| Trame | K | Catalogue seul, 1 fil : CPU (s) | µs CPU par boule | Catalogue seul, 4 fils : CPU (s) | Catalogue + tour FULL, 2 passes : CPU à 1 / 2 / 4 fils (s) | Rapport tour / catalogue (mur du même processus, 1 fil, indicatif) |")
print("| --- | ---: | ---: | ---: | ---: | --- | ---: |")
for f in ("00", "01", "02"):
    for k in (5, 10):
        ja, ta = load(R1, f"cat_l{f}_k{k}_w1"); jb, tb = load(R1, f"cat_l{f}_k{k}_w4")
        b = ja[0]["balls"]; c1 = cpu(ta); c4 = cpu(tb)
        ct = []
        for w in (1, 2, 4):
            jt, tt = load(R1, f"tow_l{f}_k{k}_w{w}_r2"); ct.append(cpu(tt))
            if w == 1: ratio = jt[0]["tower_s"] / jt[0]["catalogue_s"]
        print(f"| {f} | {k} | {fr(c1,2)} | {fr(1e6*c1/b,2)} | {fr(c4,2)} | {fr(ct[0],1)} / {fr(ct[1],1)} / {fr(ct[2],1)} | {fr(ratio,2)} |")
print()
print("### L3 memoire (local, une passe, 4 fils sauf mention)")
print("| Trame | K | Boules | Pic, catalogue seul, 1 fil (o/boule) | Pic, catalogue seul, 4 fils (o/boule) | Pic, catalogue + tour FULL (Kio) | idem (o/boule) | RSS après catalogue (o/boule) | RSS après tour (o/boule) | Tour résidente, par différence (o/boule) | Défauts de page mineurs |")
print("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for f in ("00", "01", "02"):
    for k in (5, 10):
        ja, ta = load(R1, f"cat_l{f}_k{k}_w1"); jb, tb = load(R1, f"cat_l{f}_k{k}_w4"); jt, tt = load(R1, f"tow1_l{f}_k{k}_w4")
        j = jt[0]; b = j["balls"]; r = j["rss_kib"]
        print(f"| {f} | {k} | {sp(b)} | {fr(rss(ta)*1024/b)} | {fr(rss(tb)*1024/b)} | {sp(rss(tt))} | {fr(rss(tt)*1024/b)} | {fr(r['after_catalogue']*1024/b)} | {fr(r['after_tower']*1024/b)} | {fr((r['after_tower']-r['after_catalogue'])*1024/b)} | {sp(tt['Minor (reclaiming a frame) page faults'])} |")
print()
print("### L4 attaches (local, 4 fils, compteurs)")
print("| Trame | K | Entrée | Descentes d'attache | MEB | Requêtes k-NN | Boules fermées | Part de `t_points` dans la tour (mur local) |")
print("| --- | ---: | --- | ---: | ---: | ---: | ---: | ---: |")
for f in ("00", "01", "02"):
    for k in (5, 10):
        for name, lab in (("towcover", "cover"), ("towcore", "core")):
            jt, tt = load(R1, f"{name}_l{f}_k{k}_w4"); j = jt[0]; st = j["stages"]; pt = st["point"]
            print(f"| {f} | {k} | {lab} | {sp(pt['resolves'])} | {sp(pt['meb'])} | {sp(pt['knn_queries'])} | {sp(pt['closed_balls'])} | {fr(100*st['t_points']/j['tower_s'],0)} % |")
