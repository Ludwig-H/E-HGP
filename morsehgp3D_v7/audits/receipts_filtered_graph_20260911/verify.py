"""Read sealed evidence and rerun only the independent bounded graph model."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

import graph_model


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def compute() -> dict[str, object]:
    context = json.loads((HERE / "context_pins.json").read_text())
    for path, expected in context["source_pins"].items():
        result = subprocess.run(
            ["git", "show", context["reviewed_commit"] + ":" + path],
            cwd=ROOT, capture_output=True, check=False,
        )
        require(result.returncode == 0 and digest(result.stdout) == expected,
                "historical source changed: " + path)
    receipts = []
    for item in context["receipt_packages"]:
        for key in ("reader", "manifest"):
            path = ROOT / item[key]
            require(digest(path.read_bytes()) == item[key + "_sha256"], "pin: " + str(path))
        command = [sys.executable, "-B"] + (["-O"] if sys.flags.optimize else [])
        result = subprocess.run(command + [str(ROOT / item["reader"])],
                                cwd=ROOT, capture_output=True, check=False)
        require(result.returncode == 0 and not result.stderr,
                "receipt reader failed: " + item["reader"] + ": " + result.stderr.decode())
        receipts.append({"reader": item["reader"], "exit_code": result.returncode,
                         "stdout_sha256": digest(result.stdout),
                         "result": json.loads(result.stdout)})
    return {"schema": "mhgp7-filtered-graph-audit-v1",
            "reviewed_commit": context["reviewed_commit"], "model": graph_model.run(),
            "constructor_receipts": receipts, "new_global_variant": False}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", action="store_true", help="Create a new result once, before sealing")
    args = parser.parse_args()
    if not args.record:
        pins = {}
        for line in (HERE / "SHA256SUMS").read_text().splitlines():
            expected, name = line.split("  ", 1)
            require(name not in pins and Path(name).name == name, "invalid packet name")
            pins[name] = expected
        require(set(pins) == {"README.md", "context_pins.json", "graph_model.py", "review.json", "verify.py"},
                "incomplete packet seal")
        for name, expected in pins.items():
            require(digest((HERE / name).read_bytes()) == expected, "packet pin: " + name)
    review = compute()
    if args.record:
        with (HERE / "review.json").open("x") as stream:
            stream.write(json.dumps(review, ensure_ascii=False, indent=2) + "\n")
    else:
        require(review == json.loads((HERE / "review.json").read_text()), "record differs")
    print(json.dumps({**review["model"], "status": "passed_filtered_graph_audit",
                      "readers": len(review["constructor_receipts"])}, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError, TypeError) as error:
        print("filtered graph audit failed: " + str(error), file=sys.stderr)
        raise SystemExit(1)
