#!/usr/bin/env python3
"""Audit léger des corrections M6 ; aucun appel CUDA valide ni mesure GPU."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

PIN = "95247cf4b"
REL = "morsehgp3D_v12/microbancs/mes_m6_session/mes_m6_session_cost.cu"


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def run(args):
    result = subprocess.run(args, text=True, capture_output=True, timeout=90)
    need(result.returncode == 0, f"{args}: {result.returncode}: {result.stderr}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[4])
    parser.add_argument("--cuda-cli", action="store_true", help="compile sm_120 et vérifie seulement les refus CLI")
    args = parser.parse_args()
    source = args.root / REL
    data = source.read_bytes()
    pinned = subprocess.check_output(["git", "-C", str(args.root), "show", f"{PIN}:{REL}"])
    need(data == pinned, "source M6 différente du pin examiné")
    code = data.decode()
    extracted = code[code.index("struct Quantiles {"):code.index("void emit_once(")]
    host = "#include <algorithm>\n#include <cstdio>\n#include <string>\n#include <vector>\n" + extracted + r'''
int main() {
  if (!quantiles_selftest()) return 1;
  const std::vector<std::vector<double>> cases = {
    {10,9,8,7,6,5,4,3,2,1}, {3,1,2}, {7}, {2,2,2,2}, {0,100},
    {20,1,19,2,18,3,17,4,16,5,15,6,14,7,13,8,12,9,11,10}
  };
  for (size_t i=0; i<cases.size(); ++i) {
    const auto q = quantiles(cases[i]);
    std::printf("{\"case\":%zu,\"p05\":%.17g,\"p50\":%.17g,\"p95\":%.17g,\"max\":%.17g}\n",
                i, q.p05, q.p50, q.p95, q.max);
  }
  int calls = 0;
  first_then_warm("synthetic", "", 10, [&] { return ++calls == 1 ? 100.0 : 1.0; });
  if (calls != 11) return 2;
}
'''
    with tempfile.TemporaryDirectory(prefix="v12-audit-m6-") as tmp:
        folder = Path(tmp)
        (folder / "host.cpp").write_text(host)
        run(["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Werror",
             str(folder / "host.cpp"), "-o", str(folder / "host")])
        rows = [json.loads(line) for line in run([str(folder / "host")]).splitlines()]
        expected = [(1.45,5.5,9.55,10), (1.1,2,2.9,3), (7,7,7,7),
                    (2,2,2,2), (5,50,95,100), (1.95,10.5,19.05,20)]
        need(len(rows) == 8, "nombre de lignes du témoin")
        for row, values in zip(rows[:6], expected):
            for key, value in zip(("p05", "p50", "p95", "max"), values):
                need(abs(row[key]-value) < 1e-12, f"quantile incorrect : {row}")
        need(rows[6]["name"] == "synthetic_first" and rows[6]["us"] == 100
             and rows[6]["n"] == 1, "premier usage non séparé")
        need(rows[7]["name"] == "synthetic" and rows[7]["n"] == 10
             and all(rows[7][key] == 1 for key in ("p05_us","p50_us","p95_us","max_us")),
             "le premier usage contamine encore les répétitions")
        names = ["pool_alloc_free_256mib", "empty_kernel_launch_sync", "ten_launches_sync",
                 "graph_of_ten_sync", "copy", "touch_256mib"]
        need(all(f'first_then_warm("{name}"' in code for name in names), "appel répété non couvert")
        need(code.index('emit_once("graph_of_ten_prepare"') < code.index('first_then_warm("graph_of_ten_sync"'),
             "préparation du graphe non publiée avant le premier usage")
        need(code.index("if (!quantiles_selftest())") < code.index("return run(flag, name, reps)"),
             "auto-test après CUDA")
        cli = []
        compiler = None
        if args.cuda_cli:
            nvcc = "/usr/local/cuda-12.9/bin/nvcc"
            compiler = run([nvcc, "--version"]).strip().splitlines()[-2:]
            run([nvcc, "-std=c++20", "-O3", "-arch=sm_120", "-Xcompiler=-Wall,-Wextra,-Werror",
                 str(source), "-o", str(folder / "m6")])
            for arg in ["--reps=9", "--reps=100001", "--reps=", "--sync=invalid", "--reps="+"9"*42]:
                result = subprocess.run([str(folder / "m6"), arg], text=True, capture_output=True, timeout=10)
                need(result.returncode == 2 and not result.stdout, "refus CLI manquant")
                cli.append({"argument":arg, "exit":result.returncode, "empty_stdout":True})
    print(json.dumps({"pin":PIN, "source":REL, "source_sha256":hashlib.sha256(data).hexdigest(),
        "extracted_sha256":hashlib.sha256(extracted.encode()).hexdigest(), "rows":rows,
        "repeated_call_sites":names, "cuda_compiler":compiler, "invalid_cli":cli,
        "scope":"synthetic host values; CUDA compilation and invalid CLI only; no GPU timing",
        "verdict":"CST-0209 and CST-0210 corrected in M6 at this pin"}, indent=2))


if __name__ == "__main__":
    main()
