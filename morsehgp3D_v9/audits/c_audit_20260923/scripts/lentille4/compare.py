import json, sys
from oracle_catalogue import balls
fx = json.load(open("fixtures.json"))
cache = {name: balls(P) for name, P in fx.items()}
runs = {}
cur = None
for line in open(sys.argv[1]):
    t = line.split()
    if t[0] == "RUN":
        name = t[1]; s = t[2]; w = t[3]; k = int(t[4].split("=")[1])
        cur = (name, s, w, k); runs[cur] = {"status": t[5], "rows": {}}
    elif t[0] == "B":
        key = tuple(int(x) for x in t[1:6]); q = int(t[6].split("=")[1])
        I = t[7][2:]; U = t[8][2:]
        I = [int(x) for x in I.split(",")] if I else []
        U = [int(x) for x in U.split(",")] if U else []
        runs[cur]["rows"][key] = (q, I, U)
bad = 0; checked = 0; rows = 0; by_k = {}
for (name, s, w, k), r in runs.items():
    n = len(fx[name])
    exp = {key: (v["qmin"], v["interior"], v["shell"]) for key, v in cache[name].items()
           if len(v["interior"]) + v["qmin"] <= min(k + 1, n)}
    got = r["rows"]
    checked += 1; rows += len(got)
    by_k.setdefault(k, [0, 0])
    by_k[k][0] += len(exp); by_k[k][1] += len(cache[name]) - len(exp)
    if got != exp:
        bad += 1
        miss = set(exp) - set(got); extra = set(got) - set(exp)
        diff = [kk for kk in set(exp) & set(got) if exp[kk] != got[kk]]
        print("MISMATCH", name, s, w, k, "missing", len(miss), "extra", len(extra), "diff", len(diff))
print("runs", checked, "rows", rows, "mismatches", bad)
for k in sorted(by_k): print("K", k, "expected_rows", by_k[k][0], "positive_balls_outside_window", by_k[k][1])
