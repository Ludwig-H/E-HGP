#!/usr/bin/env python3
import json, os
B = "/workspaces/E-HGP/build/v9-audit-c-publish/morsehgp3D_v9"
rows = json.load(open(os.path.join(os.path.dirname(__file__), "rows.json")))
P = json.load(open(os.path.join(B, "receipts/g4_tower_r21_20260925/PACKAGE.json")))
ng = json.load(open(os.path.join(B, "audits/c_catalogue_digest_20260923/results/PINS.json")))["pins"]
raw = json.load(open(os.path.join(B, "audits/c_raw_pins_20260924/PINS_RAW.json")))["pins"]

inputs = P["inputs"]
file_of_scene = {}
for f, v in inputs.items():
    file_of_scene[f.split("/")[-1]] = v

pins = {}
for p in ng:
    # map commit path to data file
    for f, v in inputs.items():
        if v["commit_path"] == p["file"]:
            pins[(f.split("/")[-1], p["K"])] = dict(src="PINS.json", tower=p["tower_digest"], cat=p["catalogue_digest"],
                                                  balls=p["balls"], sites=p["sites"], ihash=p["input_hash"], frame=p["frame"])
for p in raw:
    for f, v in inputs.items():
        if v["sha256"] == p["input_sha256"]:
            pins[(f.split("/")[-1], p["K"])] = dict(src="PINS_RAW.json", tower=p["tower_digest"], cat=p["catalogue_digest"],
                                                  balls=p["balls"], sites=p["sites"], ihash=p["input_fnv"], frame=p["frame"],
                                                  euler=p.get("euler"))
print("pins found:", len(pins))
for k in sorted(pins):
    print(" ", k, pins[k])

bad = []
covered = set()
unchecked = []
for r in rows:
    key = (r["file"], r["K"])
    p = pins.get(key)
    if not p:
        unchecked.append(r["case"])
        continue
    covered.add(key)
    for a, b in (("digest", "tower"), ("cat", "cat"), ("balls", "balls"), ("sites", "sites"), ("ihash", "ihash")):
        if r[a] != p[b]:
            bad.append((r["case"], key, a, r[a], p[b]))
    if p["frame"] != r["frame"]:
        bad.append((r["case"], key, "frame", r["frame"], p["frame"]))
print("unchecked cases:", unchecked)
print("pins not exercised:", sorted(set(pins) - covered))
print("mismatches:", bad)
from collections import Counter
print("cases per pin:", sorted(Counter((r['file'], r['K']) for r in rows).items()))

# presentation digest consistency within a pin group
grp = {}
for r in rows:
    grp.setdefault((r["file"], r["K"]), set()).add(r["pres"])
print("presentation digests per group:", {k: len(v) for k, v in grp.items()})

# input sha of committed files
import hashlib, subprocess
root = "/workspaces/E-HGP/build/v9-audit-c-publish"
for f, v in inputs.items():
    path = os.path.join(root, v["commit_path"])
    if os.path.exists(path):
        h = hashlib.sha256(open(path, "rb").read()).hexdigest()
        n = os.path.getsize(path) // 12
        print(f, "sha ok" if h == v["sha256"] else f"SHA MISMATCH {h}", "n?", n, v["n"])
    else:
        print(f, "commit path missing", path)
