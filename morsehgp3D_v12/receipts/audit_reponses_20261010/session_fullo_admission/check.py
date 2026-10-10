#!/usr/bin/env python3
"""FULL O : adaptation explicite du lecteur FULL N, sans moteur ni appel cloud."""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import subprocess
import types

HERE = Path(__file__).resolve().parent
READER_PIN = "589e3b752"
READER_PATH = "morsehgp3D_v12/receipts/audit_reponses_20261008/session_fulln_admission/replay.py"


def need(ok, why):
    if not ok:
        raise ValueError(why)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def run(repo, session, capture):
    def inputs():
        for path, expected in capture["inputs"].items():
            data = (session / path).read_bytes()
            need(dict(bytes=len(data), sha256=digest(data)) == expected, "input " + path)
    inputs()
    raw = subprocess.check_output(["git", "-C", str(repo), "show", READER_PIN + ":" + READER_PATH])
    need(digest(raw) == capture["reader_sha256"], "historical reader")
    changes = {
        "8a0716e7470197c95953b38d79f26b8d8f2379fc": "aa6338ee8b3d5daf6a821043c3bdd9d63ce85c66",
        "a7044e68015f8c827d3f73a5d8feb6f562d56cbfbc9f4c96f3347516ecb0998c":
            "3f50106b6ac41da81d445032b5efce540e136614ba3d49693b2ed3a08be62b09",
        488: 496,
        747: 755,
        "488 source files exact": "496 source files exact",
    }
    counts = {k: 0 for k in changes}

    class Adapt(ast.NodeTransformer):
        def visit_Constant(self, node):
            if type(node.value) in (int, str) and node.value in changes:
                counts[node.value] += 1
                node.value = changes[node.value]
            return node

    tree = Adapt().visit(ast.parse(raw))
    need(all(counts.values()), "adaptation missing")
    module = types.ModuleType("fullo_metadata_replay")
    exec(compile(tree, READER_PATH + " [O explicit pins]", "exec"), module.__dict__)
    result = module.run(repo, session)
    inputs()
    compact = {k: result[k] for k in result if k not in ("full", "mes_c")}
    compact["full"] = {k: v for k, v in result["full"].items() if k != "absolute"}
    compact["mes_c"] = {k: v for k, v in result["mes_c"].items() if k != "configuration_statistics"}
    compact["reader_adaptation"] = [{"from": k, "to": changes[k], "count": n} for k, n in counts.items()]
    return compact


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", required=True, type=Path)
    parser.add_argument("--session", required=True, type=Path)
    parser.add_argument("--record", action="store_true", help="capture initiale seulement")
    args = parser.parse_args()
    capture = json.loads((HERE / "capture.json").read_text())
    result = run(args.repo, args.session, capture)
    if args.record:
        (HERE / "results.json").write_text(json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True) + "\n")
    else:
        need(result == json.loads((HERE / "results.json").read_text()), "results differ")
        for line in (HERE / "SHA256SUMS").read_text().splitlines():
            sha, name = line.split("  ", 1)
            need(Path(name).name == name and digest((HERE / name).read_bytes()) == sha, "receipt " + name)
    print(json.dumps(dict(source=result["source_git"], sources=result["source_files"],
                          socle_passed=result["socle_passed"], full_processes=result["full"]["processes"],
                          full_warm=result["full"]["warm_passes"],
                          mes_c_processes=result["mes_c"]["processes"],
                          evidence="conditional replay; individual codes/stderr and final pilot ELF absent",
                          native_executed=False, payload_read=False)))
