#!/usr/bin/env python3
"""Compile un petit appel du vrai EmitKernel ; ni CUDA, ni gros nuage, ni mutation."""
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
PIN = "e30000dec1027c5f0ade3093a94563ee412409d3"
M5 = "morsehgp3D_v12/microbancs/mes_m5_parcours"
M2 = "morsehgp3D_v12/microbancs/mes_m2_feuille"
SOURCES = [M5+"/include/mhgp12/traversal/"+n for n in
           ("bfs.hpp", "driver.hpp", "warp.hpp", "format.hpp")]
SOURCES += [M5+"/cuda/traversal_bench.cu", M5+"/README.md", M2+"/include/mhgp12/leaf/simt.hpp"]


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_hashes():
    out = {}
    for name in SOURCES:
        data = subprocess.check_output(["git", "-C", str(ROOT), "show", PIN+":"+name])
        need((ROOT/name).read_bytes() == data, "source différente du pin : "+name)
        out[name] = sha(ROOT/name)
    return out


def main():
    before = source_hashes()
    witnesses = {n: sha(HERE/n) for n in ("probe.cpp", "check.py")}
    with tempfile.TemporaryDirectory(prefix="ehgp-m5-capacity-") as tmp:
        exe = Path(tmp)/"probe"
        cmd = ["c++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
               "-I", str(ROOT/M5/"include"), "-I", str(ROOT/M2/"include"),
               str(HERE/"probe.cpp"), "-o", str(exe)]
        built = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        need(built.returncode == 0, "compilation refusée : "+built.stderr)
        got = subprocess.run([str(exe)], capture_output=True, text=True, timeout=5)
        need(got.returncode == 0 and got.stderr == "", "témoin refusé")
        rows = [json.loads(line) for line in got.stdout.splitlines()]
        need(len(rows) == 7 and sum(r["correct"] for r in rows) == 5, "bornes modifiées")
        for row in rows:
            expected = (row["count"]+255)//256
            need(row["exact_tasks"] == expected and row["scan_child_tasks"] == 2*expected,
                 "oracle exact différent")
        binary_hash = sha(exe)
    need(source_hashes() == before, "source modifiée pendant contrôle")
    need({n: sha(HERE/n) for n in witnesses} == witnesses, "témoin modifié pendant contrôle")
    compiler = subprocess.check_output(["c++", "--version"], text=True).splitlines()[0]
    print(json.dumps({"schema":"ehgp.v12.m5.capacity.audit.v1", "pin":PIN,
        "sources_sha256":before, "witnesses_sha256":witnesses, "compiler":compiler,
        "binary_sha256":binary_hash, "compile_exit":0, "probe_exit":0, "cases":rows,
        "source_hashes_before_after_equal":True, "cuda_executed":False,
        "scope":"seven tiny EmitKernel records; no large input, allocation or whole-traversal crash"},
        ensure_ascii=False, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
