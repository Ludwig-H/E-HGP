#!/usr/bin/env python3
"""Auditeur C -- protocole << Kmax+2 >> contre les mutants compiles du generateur v9.

Pour chaque mutant (objet deja compile par run_euler_mutants.py, ou recompile ici), execute la
chaine mutee a Kmax=5 et a Kmax=7 sur la meme coupe, puis :
  - E_K = 1 pour K <= 3 sur le catalogue K5 (invariant simple) ;
  - E_K = 1 pour K <= 5 sur le catalogue K7 ;
  - condense commutatif du catalogue K5 == condense de la restriction p + q_min <= 6 du catalogue K7.
Verdicts : killed_euler5, killed_euler7, killed_restriction, killed_chain, survived_identical
(catalogue K5 egal a la reference saine : mutant non exprime sur cette entree), survived (catalogue
different mais aucune alarme), inert, error.
"""
import json
import os
import subprocess
import sys

S = os.environ.get("EULER_SCRATCH", "/tmp/claude-1000/-workspaces-E-HGP/71988aca-a49a-4d33-96db-f05468331005/scratchpad")
SRC = os.environ.get("EULER_SRC", f"{S}/src_93066733/morsehgp3D_v9")
B = os.environ.get("EULER_BUILD", f"{S}/build_93066733")
H = os.environ.get("EULER_HARNESS", f"{S}/harness")
OUT = os.environ.get("EULER_OUT", f"{H}/mutants")


def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def euler(out, deg, kmax):
    e = run(["python3", f"{H}/euler_degenerate.py", deg, str(kmax), json.dumps(out["generic_sum"]), str(out["n"])])
    return json.loads(e.stdout)


def chain(exe, data, kmax, deg):
    r = run(["nice", "-n", "19", exe, data, str(kmax), "2", deg], timeout=7200)
    return json.loads(r.stdout.strip().splitlines()[-1])


def main():
    data = sys.argv[1] if len(sys.argv) > 1 else f"{S}/data/s00_k5_s8_w8_r0_nested_8000.u32le"
    only = set(sys.argv[2].split(",")) if len(sys.argv) > 2 and sys.argv[2] else None
    ref = json.load(open(sys.argv[3])) if len(sys.argv) > 3 else None  # sortie saine K5 (v2)
    muts = json.load(open(f"{SRC}/tests/gen/mutants.json"))["mutants"]
    flags = ["-O3", "-DNDEBUG", "-std=c++20", "-w", f"-I{SRC}", f"-I{SRC}/src/gen"]
    results = []
    for m in muts:
        name = m["name"]
        if only and name not in only:
            continue
        rec = {"mutant": name, "family": m["family"], "judgment": m["judgment"]}
        mdir = f"{OUT}/{name}"
        os.makedirs(mdir, exist_ok=True)
        obj = f"{mdir}/mut.o"
        if not os.path.exists(obj):
            src_path = f"{SRC}/src/gen/{m['source']}"
            text = open(src_path).read()
            if any(old not in text for old, _ in m["replacements"]):
                rec["verdict"] = "inert"
                results.append(rec)
                print(json.dumps(rec), flush=True)
                continue
            for old, new in m["replacements"]:
                text = text.replace(old, new, 1)
            mut_src = os.path.join(os.path.dirname(src_path), f".audit_c_mut_{name}.cpp")
            open(mut_src, "w").write(text)
            c = run(["nice", "-n", "19", "g++", *flags, "-c", mut_src, "-o", obj])
            os.remove(mut_src)
            if c.returncode:
                rec["verdict"] = "error"
                results.append(rec)
                print(json.dumps(rec), flush=True)
                continue
        exe = f"{mdir}/euler_check_v2"
        c = run(["nice", "-n", "19", "g++", *flags, f"{H}/euler_check.cpp", obj, f"{B}/libmhgp9_chain.a", f"{B}/libmhgp9_gen.a",
                 "-lpthread", "-o", exe])
        if c.returncode:
            rec["verdict"] = "error"
            rec["stderr"] = c.stderr[-300:]
            results.append(rec)
            print(json.dumps(rec), flush=True)
            continue
        try:
            o5 = chain(exe, data, 5, f"{mdir}/deg5.jsonl")
            o7 = chain(exe, data, 7, f"{mdir}/deg7.jsonl")
        except Exception as exc:  # noqa: BLE001 -- verdict d'erreur explicite
            rec["verdict"] = "error"
            rec["stderr"] = str(exc)[-300:]
            results.append(rec)
            print(json.dumps(rec), flush=True)
            continue
        if o5.get("status") != "complete" or o7.get("status") != "complete":
            rec["verdict"] = "killed_chain"
            rec["chain"] = {"k5": o5.get("status"), "k7": o7.get("status"), "reason5": o5.get("reason"), "reason7": o7.get("reason")}
        else:
            e5, e7 = euler(o5, f"{mdir}/deg5.jsonl", 5), euler(o7, f"{mdir}/deg7.jsonl", 7)
            same = o5["restricted"]["6"] == o7["restricted"]["6"]
            rec.update({"balls5": o5["balls"], "euler5": e5["euler_by_k"], "euler7": e7["euler_by_k"], "restriction_equal": same})
            if not e5["pass"]:
                rec["verdict"] = "killed_euler5"
            elif not e7["pass"]:
                rec["verdict"] = "killed_euler7"
            elif not same:
                rec["verdict"] = "killed_restriction"
            elif ref and o5["restricted"]["6"] == ref["restricted"]["6"]:
                rec["verdict"] = "survived_identical"
            else:
                rec["verdict"] = "survived"
        results.append(rec)
        print(json.dumps(rec), flush=True)
    json.dump(results, open(f"{OUT}/results_protocol_k5_k7.json", "w"), indent=1)


if __name__ == "__main__":
    main()
