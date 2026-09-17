import json, os, subprocess, sys
configs = [("ref 2K/16", {}), ("restart", dict(PROTO_RESTART="1")), ("inherit", dict(PROTO_INHERIT="1")), ("minmass2", dict(PROTO_MINMASS="2")), ("restart+inherit", dict(PROTO_RESTART="1", PROTO_INHERIT="1")), ("all three", dict(PROTO_RESTART="1", PROTO_INHERIT="1", PROTO_MINMASS="2"))]
for n, kmax, fam in [a.split(":") for a in sys.argv[1:]]:
    base = None
    for name, cfg in configs:
        best = None
        for _ in range(2):
            out = subprocess.run(["build_base/mhgp8_wspd_q2_proposals_probe", n, fam, kmax, "8", "3", "1", "16", "64", "2", "16"], env=dict(os.environ, **cfg), capture_output=True, text=True)
            if out.returncode: print(fam, n, kmax, name, "FAILED", out.stderr[-300:], flush=True); break
            o = json.loads(out.stdout); ms = o["timings"]["pipeline_wall_ms"]; best = ms if best is None else min(best, ms)
        else:
            fw = o["front_work"]; dig = (o["digest"]["sum"], o["digest"]["xor"]); row = (o["candidate_pairs"], fw["product_visits"], fw["witness_descent_steps"], fw["h_bound_tests"], o["census_work"]["count_node_visits"])
            if base is None: base = (best, dig, row)
            print(f"{fam:9s} n={n:6s} K={kmax:3s} {name:16s} ms {best:8.0f} x{best/base[0]:.3f} cand x{row[0]/base[2][0]:.3f} prod x{row[1]/base[2][1]:.3f} desc x{row[2]/base[2][2]:.3f} h x{row[3]/base[2][3]:.3f} cnv x{row[4]/base[2][4]:.3f}", "=" if dig == base[1] else "DIGEST DIFF", flush=True)
