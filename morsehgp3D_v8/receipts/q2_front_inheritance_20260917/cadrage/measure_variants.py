import json, os, subprocess
for fam, n, k in [("uniform","8000","10"), ("clusters","8000","10"), ("terrain","8000","10"), ("rows","8000","10"), ("uniform","32000","10")]:
    base = None
    for name, cfg in [("ref 2K/16", {}), ("pivot always", dict(PROTO_PIVOT="1")), ("inherit+pivot c>=1", dict(PROTO_INHERIT="1", PROTO_PIVOT="2")), ("last-path", dict(PROTO_RESTART="1")), ("climb", dict(PROTO_RESTART="2", PROTO_CLIMB_REPORT="1")), ("inherit", dict(PROTO_INHERIT="1")), ("climb+inherit", dict(PROTO_RESTART="2", PROTO_INHERIT="1", PROTO_CLIMB_REPORT="1"))]:
        if name.startswith("pivot") and n != "8000": continue
        best = None
        for _ in range(2):
            out = subprocess.run(["build_base/mhgp8_wspd_q2_proposals_probe", n, fam, k, "8", "3", "1", "16", "64", "2", "16"], env=dict(os.environ, **cfg), capture_output=True, text=True)
            if out.returncode: print(fam, n, name, "FAILED", out.stderr[-300:], flush=True); break
            o = json.loads(out.stdout); ms = o["timings"]["pipeline_wall_ms"]; best = ms if best is None else min(best, ms)
        else:
            fw = o["front_work"]; dig = (o["digest"]["sum"], o["digest"]["xor"], o["digest"]["supports"])
            row = dict(candidates=o["candidate_pairs"], products=fw["product_visits"], descent_steps=fw["witness_descent_steps"], h_tests=fw["h_bound_tests"], count_node_visits=o["census_work"]["count_node_visits"])
            if base is None: base = (best, dig, row)
            climb = [l for l in out.stderr.splitlines() if l.startswith("CLIMB")]
            print(json.dumps(dict(family=fam, n=int(n), kmax=int(k), s=8, seed=3, variant=name, env=cfg, pipeline_wall_ms_min_of_2=best, time_over_ref=round(best / base[0], 3), work=row, climb=climb, digest=dig, digest_equals_ref=dig == base[1])), flush=True)
