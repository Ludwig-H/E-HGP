#!/usr/bin/env python3
import csv, sys, math
p = "/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10/receipts/g4_session5_scale_20260929/scale.csv"
rows = [r for r in csv.DictReader(open(p)) if r["status"] == "ok"]
print("lignes ok:", len(rows))
def f(r, k): return float(r[k])
# octets par boule (pic RSS du processus tour --no-points, une passe)
print("\n== octets par boule au pic (max_rss_kb*1024/balls), par famille et K, min/max et plus grande entree")
import collections
g = collections.defaultdict(list)
for r in rows:
    key = (r["kind"], r["family"] or "lidar", r["k"])
    g[key].append((int(r["sites"]), int(r["balls"]), f(r, "max_rss_kb") * 1024 / int(r["balls"]), f(r,"catalogue_s"), f(r,"tower_s"), int(r["balls"])/int(r["sites"]), r["file"]))
for key in sorted(g):
    v = sorted(g[key])
    bpb = [x[2] for x in v]
    bps = [x[5] for x in v]
    big = v[-1]
    print(f"{key}: n={len(v)} o/boule min={min(bpb):.1f} max={max(bpb):.1f} | boules/site min={min(bps):.1f} max={max(bps):.1f} | plus grande: sites={big[0]} boules={big[1]} o/boule={big[2]:.1f} cat={big[3]:.2f}s tour={big[4]:.2f}s ({big[6]})")
# LiDAR boules par site
print("\n== LiDAR : boules par site par secteur")
for r in sorted(rows, key=lambda r: (r["k"], int(r["sites"]))):
    if r["kind"] != "lidar": continue
    b = int(r["balls"]); n = int(r["sites"])
    print(f"K={r['k']:>2} {r['frame']} {r['sector']:28s} sites={n:6d} boules={b:8d} boules/site={b/n:6.1f} o/boule={f(r,'max_rss_kb')*1024/b:6.1f} cat={f(r,'catalogue_s')*1000:7.1f}ms tour={f(r,'tower_s')*1000:7.1f}ms ns/boule cat={f(r,'catalogue_s')*1e9/b:6.1f} tour={f(r,'tower_s')*1e9/b:6.1f}")
# grandes entrees
print("\n== entrees >= 256000 sites")
for r in sorted(rows, key=lambda r: (int(r["sites"]), r["k"])):
    n = int(r["sites"])
    if n < 256000: continue
    b = int(r["balls"])
    print(f"{r['file']:40s} K={r['k']:>2} sites={n} boules={b} b/site={b/n:.1f} cat={f(r,'catalogue_s'):.2f} tour={f(r,'tower_s'):.2f} rss={f(r,'max_rss_kb')/1048576:.2f}Gio o/boule={f(r,'max_rss_kb')*1024/b:.1f} ns/boule cat={f(r,'catalogue_s')*1e9/b:.1f} tour={f(r,'tower_s')*1e9/b:.1f}")
