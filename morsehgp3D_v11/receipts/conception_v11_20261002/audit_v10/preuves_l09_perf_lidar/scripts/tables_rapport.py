#!/usr/bin/env python3
"""Blocs Markdown du rapport L09, regeneres depuis les sorties brutes des recus G4 (lecture seule)."""
import json, os, sys
ROOT = "/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10/receipts"
SESS = {"s1": "g4_session1_20260929", "s2": "g4_session2_perf_20260929", "s3": "g4_session3_j2_20260929", "s4": "g4_session4_j2c_20260929"}
def load(sess, cmd):
    for line in open(os.path.join(ROOT, SESS[sess], "results", "cmd", cmd, "stdout")):
        line = line.strip()
        if line.startswith("{"): return json.loads(line)
def ms(x, d=1): return f"{1000*x:.{d}f}".replace(".", ",")
def fr(x, d=1): return f"{x:.{d}f}".replace(".", ",")
def sp(n): return f"{n:,}".replace(",", " ")
F = (("00", "008_tower_lidar00_k5_w48", "009_tower_lidar00_k10_w48"), ("01", "010_tower_lidar01_k5_w48", "011_tower_lidar01_k10_w48"), ("02", "012_tower_lidar02_k5_w48", "013_tower_lidar02_k10_w48"))
print("### T1 catalogue")
print("| Trame | Sites | K | Boules | Frontière | Boîtes | Ordre | Assemblage | Hors étages | **Catalogue** |")
print("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for K in (0, 1):
    for f, c5, c10 in F:
        j = load("s4", (c5, c10)[K]); cs = j["catalogue_stages"]
        hs = j["catalogue_s"] - (cs["t_frontier"] + cs["t_boxes"] + cs["t_order"] + cs["t_assemble"])
        print(f"| {f} | {sp(j['n'])} | {j['K']} | {sp(j['balls'])} | {ms(cs['t_frontier'])} | {ms(cs['t_boxes'])} | {ms(cs['t_order'])} | {ms(cs['t_assemble'])} | {ms(hs)} | **{ms(j['catalogue_s'])}** |")
print()
print("### T2 tour")
print("| Trame | K | Index des supports | Atlas | Semis | Descentes | Kruskal | Verticales | Non ventilé | **Tour** | **Catalogue + tour** | Préparation (hors total) |")
print("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for K in (0, 1):
    for f, c5, c10 in F:
        j = load("s4", (c5, c10)[K]); st = j["stages"]
        s = sum(st[k] for k in ("t_prepare", "t_local", "t_seeds", "t_resolve", "t_kruskal", "t_points", "t_vertical"))
        print(f"| {f} | {j['K']} | {ms(st['t_prepare'])} | {ms(st['t_local'])} | {ms(st['t_seeds'])} | {ms(st['t_resolve'])} | {ms(st['t_kruskal'])} | {ms(st['t_vertical'])} | {ms(j['tower_s']-s)} | **{ms(j['tower_s'])}** | **{ms(j['catalogue_s']+j['tower_s'])}** | {ms(j['prepare_s'])} |")
print()
print("### T3 plancher")
print("| Trame | K | Catalogue + tour | Boîtes + descentes | Reste | Part du reste | Kruskal, ordre K seul | Fusions verticales, ordre K seul | Somme des Kruskal des K ordres | Rapport au contrat de 100 ms |")
print("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for K in (0, 1):
    for f, c5, c10 in F:
        j = load("s4", (c5, c10)[K]); cs = j["catalogue_stages"]; st = j["stages"]
        tot = j["catalogue_s"] + j["tower_s"]; par = cs["t_boxes"] + st["t_resolve"]; o = j["orders"][-1]
        print(f"| {f} | {j['K']} | {ms(tot)} | {ms(par)} | {ms(tot-par)} | {fr(100*(tot-par)/tot,0)} % | {ms(o['t_kruskal'])} | {ms(o['t_vertical'])} | {ms(st['sum_order_kruskal'])} | ×{fr(tot/0.1,1)} |")
print()
print("### T4 compteurs tour (trame, K) -- session 4, 48 fils (compteurs de travail, dependants des fils pour le memo)")
print("| Trame | K | Boules | Cellules | Nœuds de la tour (tous ordres) | Naissances | Jonctions | Fusions | Descentes | Arrêts au semis | MEB | Boules fermées (arbre) | Pas de remontée |")
print("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")
for K in (0, 1):
    for f, c5, c10 in F:
        j = load("s4", (c5, c10)[K]); st = j["stages"]; jn = st["join"]
        nodes = sum(o["nodes"] for o in j["orders"]); births = sum(o["births"] for o in j["orders"]); joins = sum(o["joins"] for o in j["orders"]); merges = sum(o["merges"] for o in j["orders"])
        print(f"| {f} | {j['K']} | {sp(j['balls'])} | {sp(st['local_cells'])} | {sp(nodes)} | {sp(births)} | {sp(joins)} | {sp(merges)} | {sp(jn['resolves'])} | {sp(jn['seed_hits'])} | {sp(jn['meb'])} | {sp(jn['closed_balls'])} | {sp(st['walk_steps'])} |")
