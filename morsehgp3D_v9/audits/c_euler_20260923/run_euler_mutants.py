#!/usr/bin/env python3
"""Auditeur C -- les mutants compiles du generateur v9 sont-ils tues par l'invariant d'Euler ?

Pour chaque mutant de tests/gen/mutants.json : copie mutee de l'unite source, compilee en objet,
liee AVANT libmhgp9_gen.a avec euler_check.cpp ; execution sur une coupe LiDAR 8k sans sol.
Verdicts : killed_euler (chaine complete mais Euler != 1), killed_chain (chaine refuse),
survived (Euler = 1 sur les ordres verifiables), inert (remplacement introuvable), error.
Priorite basse (nice 19), 2 fils.
"""
import json
import os
import subprocess
import sys

# chemins : variables d'environnement (valeurs par defaut = poste de l'auditeur C)
S = os.environ.get("EULER_SCRATCH", "/tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad")
SRC = os.environ.get("EULER_SRC", f"{S}/src_93066733/morsehgp3D_v9")  # archive du commit, jamais le worktree
B = os.environ.get("EULER_BUILD", f"{S}/build_93066733")  # contient libmhgp9_chain.a et libmhgp9_gen.a
H = os.environ.get("EULER_HARNESS", f"{S}/harness")  # euler_check.cpp et euler_degenerate.py
OUT = os.environ.get("EULER_OUT", f"{H}/mutants")
os.makedirs(OUT, exist_ok=True)


def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def main():
    kmax = int(sys.argv[1]) if len(sys.argv) > 1 else 5
    data = sys.argv[2] if len(sys.argv) > 2 else f"{S}/data/s00_k5_s8_w8_r0_nested_8000.u32le"
    only = set(sys.argv[3].split(",")) if len(sys.argv) > 3 and sys.argv[3] else None
    muts = json.load(open(f"{SRC}/tests/gen/mutants.json"))["mutants"]
    flags = ["-O3", "-DNDEBUG", "-std=c++20", "-w", f"-I{SRC}", f"-I{SRC}/src/gen"]
    results = []
    for m in muts:
        name = m["name"]
        if only and name not in only:
            continue
        src_path = f"{SRC}/src/gen/{m['source']}"
        text = open(src_path).read()
        inert = False
        for old, new in m["replacements"]:
            if old not in text:
                inert = True
                break
            text = text.replace(old, new, 1)
        rec = {"mutant": name, "family": m["family"], "judgment": m["judgment"], "kmax": kmax}
        if inert:
            rec["verdict"] = "inert"
            results.append(rec)
            print(json.dumps(rec), flush=True)
            continue
        mdir = f"{OUT}/{name}"
        os.makedirs(mdir, exist_ok=True)
        # la copie mutee doit resoudre ses includes relatifs comme l'original
        mut_src = os.path.join(os.path.dirname(src_path), f".audit_c_mut_{name}.cpp")
        open(mut_src, "w").write(text)
        obj = f"{mdir}/mut.o"
        exe = f"{mdir}/euler_check"
        c = run(["nice", "-n", "19", "g++", *flags, "-c", mut_src, "-o", obj])
        os.remove(mut_src)
        if c.returncode:
            rec["verdict"] = "error"
            rec["stderr"] = c.stderr[-400:]
            results.append(rec)
            print(json.dumps(rec), flush=True)
            continue
        c = run(["nice", "-n", "19", "g++", *flags, f"{H}/euler_check.cpp", obj, f"{B}/libmhgp9_chain.a", f"{B}/libmhgp9_gen.a",
                 "-lpthread", "-o", exe])
        if c.returncode:
            rec["verdict"] = "error"
            rec["stderr"] = c.stderr[-400:]
            results.append(rec)
            print(json.dumps(rec), flush=True)
            continue
        r = run(["nice", "-n", "19", exe, data, str(kmax), "2", f"{mdir}/deg.jsonl"], timeout=3600)
        try:
            out = json.loads(r.stdout.strip().splitlines()[-1])
        except Exception:
            rec["verdict"] = "error"
            rec["stderr"] = (r.stderr or "")[-400:]
            results.append(rec)
            print(json.dumps(rec), flush=True)
            continue
        if out.get("status") != "complete":
            rec["verdict"] = "killed_chain"
            rec["chain"] = out
        else:
            e = run(["python3", f"{H}/euler_degenerate.py", f"{mdir}/deg.jsonl", str(kmax), json.dumps(out["generic_sum"]), str(out["n"])])
            ev = json.loads(e.stdout)
            rec["euler_by_k"] = ev["euler_by_k"]
            rec["balls"] = out["balls"]
            rec["verdict"] = "survived" if ev["pass"] else "killed_euler"
        results.append(rec)
        print(json.dumps(rec), flush=True)
    json.dump(results, open(f"{OUT}/results_k{kmax}.json", "w"), indent=1)


if __name__ == "__main__":
    main()
