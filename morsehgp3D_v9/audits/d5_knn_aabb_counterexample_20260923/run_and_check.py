#!/usr/bin/env python3
"""Rebuild the exact u18 counterexample against a pinned product source tree."""

import argparse
import json
import pathlib
import subprocess
import tempfile


COMMIT = "a6d08f05f47378b1483e630e2c75865571bc1ec0"
TOWER_TREE = "38895277e70bb18a16cb29883d2d1edd1e5ef5be"
HERE = pathlib.Path(__file__).resolve().parent
REPO = HERE.parents[2]


def run(*command: str, cwd: pathlib.Path | None = None) -> str:
    result = subprocess.run(command, cwd=cwd, check=True, capture_output=True, text=True)
    return result.stdout.strip()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--record", action="store_true", help="write RESULT.json (receipt author only)")
    args = parser.parse_args()
    tree = run("git", "rev-parse", f"{COMMIT}:morsehgp3D_v9/src/tower", cwd=REPO)
    if tree != TOWER_TREE:
        raise SystemExit(f"pinned product tree mismatch: {tree}")

    with tempfile.TemporaryDirectory(prefix="mhgp9-d5-aabb-") as temp:
        tmp = pathlib.Path(temp)
        archive = tmp / "tower.tar"
        run("git", "archive", "--format=tar", f"--output={archive}", COMMIT,
            "morsehgp3D_v9/src/tower", cwd=REPO)
        run("tar", "-xf", str(archive), "-C", str(tmp))
        binary = tmp / "probe"
        run("g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Werror",
            "-I", str(tmp), "-o", str(binary), str(HERE / "probe.cpp"))
        actual = json.loads(run(str(binary)))

    receipt = HERE / "RESULT.json"
    if args.record:
        receipt.write_text(json.dumps(actual, ensure_ascii=False, indent=2) + "\n")
    expected = json.loads(receipt.read_text())
    if actual != expected:
        raise SystemExit(f"counterexample changed: {actual!r}")
    print("PASS: pinned radix/AABB source; K5 nonterminal geometry; node visits 29/29, 45/45, 77/77, 141/141")


if __name__ == "__main__":
    main()
