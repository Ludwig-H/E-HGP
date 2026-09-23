#!/usr/bin/env python3
"""Auditeur C -- comparaison cle par cle des catalogues K5 mutes contre le catalogue sain
(reponse a l'erratum B, point 3). Un fil, nice 19. Le mutant desactive
`admitted_lane_recounted_in_children` est recompile sur son site exact a deux lignes."""
import json
import os
import subprocess
import sys

S = os.environ.get("EULER_SCRATCH", "/tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad")
SRC = f"{S}/src_93066733/morsehgp3D_v9"
B = f"{S}/build_93066733"
H = f"{S}/harness"
OUT = f"{H}/mutants"
DATA = f"{S}/data/s00_k5_s8_w8_r0_nested_8000.u32le"
FLAGS = ["-O3", "-DNDEBUG", "-std=c++20", "-w", f"-I{SRC}", f"-I{SRC}/src/gen"]
EXACT_SITE = ("        admitted_mask |= bit;\n        frame.mask &= static_cast<std::uint8_t>(~bit);",
              "        admitted_mask |= bit;\n        /* mutant: leave admitted lane in children */")


def run(cmd, env=None, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, env=env, **kw)


def build(name, obj):
    exe = f"{OUT}/{name}/euler_check_v3"
    c = run(["nice", "-n", "19", "g++", *FLAGS, f"{H}/euler_check.cpp", obj, f"{B}/libmhgp9_chain.a", f"{B}/libmhgp9_gen.a",
             "-lpthread", "-o", exe])
    if c.returncode:
        raise RuntimeError(c.stderr[-300:])
    return exe


def dump(exe, path):
    env = dict(os.environ, EULER_DUMP=path)
    r = run(["nice", "-n", "19", exe, DATA, "5", "1", path + ".deg"], env=env, timeout=7200)
    return json.loads(r.stdout.strip().splitlines()[-1])


def main():
    names = sys.argv[1].split(",")
    ref = f"{OUT}/healthy_k5.keys"
    if not os.path.exists(ref):
        dump(f"{H}/euler_check_v3", ref)
    muts = {m["name"]: m for m in json.load(open(f"{SRC}/tests/gen/mutants.json"))["mutants"]}
    results = []
    for name in names:
        m = muts[name]
        os.makedirs(f"{OUT}/{name}", exist_ok=True)
        obj = f"{OUT}/{name}/mut_exact.o" if name == "admitted_lane_recounted_in_children" else f"{OUT}/{name}/mut.o"
        if name == "admitted_lane_recounted_in_children" and not os.path.exists(obj):
            src_path = f"{SRC}/src/gen/{m['source']}"
            text = open(src_path).read()
            if text.count(EXACT_SITE[0]) != 1:
                raise SystemExit("exact site not unique")
            text = text.replace(EXACT_SITE[0], EXACT_SITE[1], 1)
            mut_src = os.path.join(os.path.dirname(src_path), ".audit_c_mut_exact.cpp")
            open(mut_src, "w").write(text)
            c = run(["nice", "-n", "19", "g++", *FLAGS, "-c", mut_src, "-o", obj])
            os.remove(mut_src)
            if c.returncode:
                raise SystemExit(c.stderr[-300:])
        exe = build(name, obj)
        path = f"{OUT}/{name}/k5.keys"
        out = dump(exe, path)
        rec = {"mutant": name, "status": out.get("status")}
        if out.get("status") == "complete":
            cmp = json.loads(run(["python3", f"{H}/compare_dumps.py", ref, path, "5"]).stdout)
            rec.update({"compare": cmp})
            e = run(["python3", f"{H}/euler_degenerate.py", path + ".deg", "5", json.dumps(out["generic_sum"]), str(out["n"])])
            rec["euler5"] = json.loads(e.stdout)["euler_by_k"]
        else:
            rec["reason"] = out.get("reason")
        results.append(rec)
        print(json.dumps(rec), flush=True)
    json.dump(results, open(f"{OUT}/results_key_compare_k5.json", "w"), indent=1)


if __name__ == "__main__":
    main()
