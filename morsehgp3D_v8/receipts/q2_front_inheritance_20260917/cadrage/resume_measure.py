import json, subprocess, sys
S = "/tmp/claude-1000/-workspaces-E-HGP/b64b3f68-b0cf-4b1f-9ec0-8ffa4358295a/scratchpad"
variants = ["2 16 0 0", "2 16 1 0", "2 16 0 1", "2 16 1 1"]
out = open(S + "/resume_measure.jsonl", "w")
for fam, n, k in [("uniform", "8000", "10"), ("uniform", "32000", "10"), ("clusters", "32000", "10"), ("terrain", "32000", "10"), ("rows", "32000", "10")]:
    for rep in range(3):
        for v in variants:
            o = json.loads(subprocess.run([S + "/resume_snapshot/probe_with_resume", n, fam, k, "8", "3", "1", "16", "64", *v.split()], capture_output=True, text=True, check=True).stdout)
            out.write(json.dumps(dict(family=fam, n=int(n), kmax=int(k), s=8, seed=3, repeat=rep, window_factor=2, small_factor_limit=16, resume_descent=o["resume_descent"], inherit_witnesses=o["inherit_witnesses"], pipeline_wall_ms=o["timings"]["pipeline_wall_ms"], candidate_pairs=o["candidate_pairs"], witness_descent_steps=o["front_work"]["witness_descent_steps"], h_bound_tests=o["front_work"]["h_bound_tests"], resume_work=o["resume_work"], inheritance_work=o["inheritance_work"], digest=o["digest"])) + "\n"); out.flush()
for n in ("8000", "32000"):
    for v in variants:
        r = subprocess.run([S + "/timing_t21/build/mhgp8_wspd_q2_inheritance_probe", n, "uniform", "10", "8", "3", "1", "16", "64", *v.split()], capture_output=True, text=True, check=True)
        line = [l for l in r.stderr.splitlines() if l.startswith("TSC")][0]
        out.write(json.dumps(dict(kind="cycle_breakdown_rdtsc", family="uniform", n=int(n), kmax=10, variant=v, line=line)) + "\n"); out.flush()
out.close(); open(S + "/resume_measure.done", "w").write("done\n")
