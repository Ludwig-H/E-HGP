#!/usr/bin/env python3
"""Extraction brute des sorties JSON des sessions G4 v10 (lecture seule des recus)."""
import json, os, sys, glob

ROOT = "/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10/receipts"
SESS = [("s1", "g4_session1_20260929"), ("s2", "g4_session2_perf_20260929"),
        ("s3", "g4_session3_j2_20260929"), ("s4", "g4_session4_j2c_20260929")]
rows = []
for tag, d in SESS:
    base = os.path.join(ROOT, d, "results", "cmd")
    for c in sorted(os.listdir(base)):
        out = os.path.join(base, c, "stdout")
        meta = os.path.join(base, c, "meta.txt")
        argv = open(os.path.join(base, c, "argv.txt")).read().strip()
        m = {}
        for line in open(meta):
            if "=" in line:
                k, v = line.strip().split("=", 1)
                m[k] = v
        txt = open(out).read().strip()
        js = None
        for line in txt.splitlines():
            line = line.strip()
            if line.startswith("{"):
                try:
                    js = json.loads(line)
                except Exception as e:
                    js = {"_parse_error": str(e)}
        rows.append({"session": tag, "cmd": c, "argv": argv, "meta": m, "json": js})
json.dump(rows, open(sys.argv[1], "w"))
print(len(rows), "commandes")
for r in rows:
    j = r["json"] or {}
    print(r["session"], r["cmd"], "wall=", r["meta"].get("wall_seconds"), "rss_kb=", r["meta"].get("max_rss_kb"),
          "keys=", sorted(j.keys())[:14] if isinstance(j, dict) else None)
