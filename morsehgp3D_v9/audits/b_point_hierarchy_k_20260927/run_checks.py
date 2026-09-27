#!/usr/bin/env python3
"""Archive normal/-O local gates without overwriting earlier captures."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, data):
    Path(path).write_text(json.dumps(data, sort_keys=True, indent=2, allow_nan=False) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--native", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    sources = sorted(HERE.glob("*.py")) + [HERE / "native_export.cpp"]
    receipt = dict(schema="mhgp9_point_hierarchy_checks_v1", status="running", commands=[],
                   native_binary_sha256=sha(args.native),
                   sources_before={str(p): sha(p) for p in sources}, GCP_used=False, GPU_used=False)
    save(args.output / "receipt.json", receipt)
    try:
        commands = [("native", [str(args.native.resolve()), "--gate"])]
        for opt in (False, True):
            for stem in ("test_datasets", "test_eom", "test_projection", "test_benchmark"):
                command = [sys.executable, "-B"] + (["-O"] if opt else []) + [str(HERE / (stem + ".py"))]
                if stem == "test_projection":
                    command.extend(["--native", str(args.native.resolve())])
                commands.append((stem + ("_optimized" if opt else "_normal"), command))
        for name, command in commands:
            start = time.perf_counter()
            with (args.output / (name + ".stdout")).open("wb") as out, (args.output / (name + ".stderr")).open("wb") as err:
                result = subprocess.run(command, stdout=out, stderr=err, check=False)
            receipt["commands"].append(dict(name=name, argv=command, returncode=result.returncode,
                seconds=time.perf_counter() - start, stdout_sha256=sha(args.output / (name + ".stdout")),
                stderr_sha256=sha(args.output / (name + ".stderr"))))
            save(args.output / "receipt.json", receipt)
            print(name, result.returncode, flush=True)
        receipt["sources_after"] = {str(p): sha(p) for p in sources}
        receipt["native_binary_sha256_after"] = sha(args.native)
        receipt["status"] = "passed" if (receipt["sources_before"] == receipt["sources_after"] and
            receipt["native_binary_sha256"] == receipt["native_binary_sha256_after"] and
            all(c["returncode"] == 0 for c in receipt["commands"])) else "failed"
        for stem in ("test_eom", "test_projection"):
            if (args.output / (stem + "_normal.stdout")).read_bytes() != (args.output / (stem + "_optimized.stdout")).read_bytes():
                receipt["status"] = "failed"
                receipt.setdefault("differing_normal_optimized", []).append(stem)
    except BaseException as error:
        receipt["status"], receipt["error"] = "failed", repr(error)
        save(args.output / "receipt.json", receipt)
        raise
    save(args.output / "receipt.json", receipt)
    return 0 if receipt["status"] == "passed" else 1


if __name__ == "__main__":
    sys.exit(main())
