#!/usr/bin/env python3
"""Relit les trois assembleurs captures ; ne compile et n'execute rien."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def run(args, **kw):
    p = subprocess.run(args, capture_output=True, **kw)
    require(p.returncode == 0, f"commande refusee : {args}: {p.stderr!r}")
    return p.stdout


def body(text, name):
    m = re.search(r"^" + name + r":\n(.*?)^\s*\.size\s+" + name + r",", text, re.M | re.S)
    require(m is not None, f"fonction absente : {name}")
    return m.group(1)


def counts(text):
    return {
        "software_popcount_call_sites": len(re.findall(r"^\s*(?:call|jmp)\s+__popcountdi2(?:@PLT)?\s*$", text, re.M)),
        "popcnt_instructions": len(re.findall(r"^\s*popcnt[lq]\s+", text, re.M)),
        "all_call_sites": len(re.findall(r"^\s*call\s+", text, re.M)),
    }


def review(root, repo):
    cap = json.loads((root / "capture.json").read_text())
    require(cap["format"] == "mhgp12_audit_cpu_popcount_asm_v1", "format")
    require(sha((root / "wrappers.cpp").read_bytes()) == cap["wrapper_sha256"], "wrapper altere")
    head = cap["repository_head"]
    source = {}
    for rel, expected in cap["source_pins"].items():
        data = run(["git", "show", f"{head}:{rel}"], cwd=repo)
        require(sha(data) == expected, f"source Git alteree : {rel}")
        source[rel] = data
    patch_rel = "morsehgp3D_v12/receipts/audit_reponses_20261008/cpu_popcount/proposition.patch"
    header = "morsehgp3D_v12/src/catalogue/simt.hpp"
    # Reconstruction epinglee, sans lire de source produit mutable.
    with tempfile.TemporaryDirectory(prefix="audit-popcount-read-") as td:
        temp = Path(td)
        for rel, data in source.items():
            out = temp / rel
            out.parent.mkdir(parents=True, exist_ok=True)
            out.write_bytes(data)
        patch = temp / patch_rel
        run(["git", "apply", "--check", str(patch)], cwd=temp)
        run(["git", "apply", str(patch)], cwd=temp)
        require(sha((temp / header).read_bytes()) == cap["candidate_header_sha256"], "candidat altere")
    expected_names = ["base_generic", "candidate_generic", "base_popcnt", "candidate_popcnt"]
    require([r["arm"] for r in cap["runs"]] == expected_names, "cohorte assembleur")
    result = {"arms": {}, "unique_assemblies": 0, "popcnt_arms_byte_identical": False}
    assembly_hashes = []
    flags = ["-O3", "-DNDEBUG", "-std=c++20", "-DMHGP12_COORD_BITS=21", "-S"]
    for record in cap["runs"]:
        arm = record["arm"]
        popcnt = arm.endswith("_popcnt")
        require(record["flags"] == flags + (["-mpopcnt"] if popcnt else []), "options")
        require(record["code"] == 0 and record["stdout"] == record["stderr"] == "", "compilation non close")
        require(set(record["repository_headers"]) == set(source) - {patch_rel}, "dependances")
        expected_file = "popcnt.s" if popcnt else arm + ".s"
        require(record["stored_assembly"] == expected_file, "fichier assembleur")
        data = (root / expected_file).read_bytes()
        digest = sha(data)
        require(digest == record["assembly_sha256"], f"assembleur altere : {arm}")
        assembly_hashes.append(digest)
        text = data.decode("ascii")
        funcs = {name: counts(body(text, name)) for name in ("audit_popc32", "audit_popc256")}
        totals = counts(text)
        expected = [0, 0, 0] if arm == "candidate_generic" else ([0, 5, 0] if popcnt else [5, 0, 5])
        require(list(totals.values()) == expected, f"instructions : {arm}")
        for i, name in enumerate(funcs):
            count = 1 if i == 0 else 4
            expected_fun = [0, 0, 0] if arm == "candidate_generic" else ([0, count, 0] if popcnt else [count, 0, count])
            require(list(funcs[name].values()) == expected_fun, f"instructions : {arm}/{name}")
        result["arms"][arm] = {"assembly_sha256": digest, **totals, "functions": funcs}
    result["unique_assemblies"] = len(set(assembly_hashes))
    result["popcnt_arms_byte_identical"] = assembly_hashes[2] == assembly_hashes[3]
    require(result["unique_assemblies"] == 3 and result["popcnt_arms_byte_identical"], "identite des bras POPCNT")
    require(result == cap["result"], "resultat different de la capture")
    return result


def main():
    root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=root.parents[3])
    args = parser.parse_args()
    print(json.dumps(review(root, args.repo), ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == "__main__":
    main()
