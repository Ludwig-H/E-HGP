#!/usr/bin/env python3
"""Judge tiny q2 probe payload digests with direct integer all-site censuses."""

from __future__ import annotations

import argparse
import itertools
import json
import math
from pathlib import Path
import subprocess
import sys
from typing import Any

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
from run_q2_census_matrix import cross_check, parse_result, validate_result  # noqa: E402


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def hash_words(words: list[int]) -> int:
    result = 14695981039346656037
    for word in words:
        for value in word.to_bytes(8, "little"):
            result = ((result ^ value) * 1099511628211) & ((1 << 64) - 1)
    return result


def points(n: int, family: str) -> tuple[list[tuple[int, int, int]], int]:
    b = max(1, n // 16) if family == "skew" else n - n // 2
    answer = []
    for count, base_x in ((n - b, 1000), (b, 60000)):
        if family == "sheet_full":
            side = max(d for d in range(1, math.isqrt(count) + 1) if count % d == 0)
        else:
            side = 1
            while side ** (2 if family == "sheet" else 3) < count:
                side += 1
        for index in range(count):
            if family == "tube":
                answer.append((base_x + index, 1000, 1000))
            elif family in ("sheet", "sheet_full"):
                answer.append((base_x, 1000 + index % side, 1000 + index // side))
            else:
                answer.append((base_x + index % side, 1000 + (index // side) % side,
                               1000 + index // (side * side)))
    return answer, n - b


def oracle(n: int, family: str, kmax: int) -> dict[str, Any]:
    cloud, split = points(n, family)
    result: dict[str, Any] = dict(supports=0, interior_ids=0, shell_ids=0, sum=0, xor=0)
    for a in range(split):
        for b in range(split, n):
            values = [sum((z[i] - cloud[a][i]) * (cloud[b][i] - z[i]) for i in range(3))
                      for z in cloud]
            interior = [i for i, h in enumerate(values) if h > 0]
            if len(interior) >= kmax:
                continue
            shell = [i for i, h in enumerate(values) if h == 0]
            words = [1, a, b, *(cloud[a][i] + cloud[b][i] for i in range(3)),
                     sum((cloud[a][i] - cloud[b][i]) ** 2 for i in range(3))]
            for ids in (interior, shell):
                hashes = [hash_words([1, identity]) for identity in ids]
                xor = 0
                for value in hashes:
                    xor ^= value
                words.extend((len(ids), sum(hashes) & ((1 << 64) - 1), xor))
            value = hash_words(words)
            result["supports"] += 1
            result["interior_ids"] += len(interior)
            result["shell_ids"] += len(shell)
            result["sum"] = (result["sum"] + value) & ((1 << 64) - 1)
            result["xor"] ^= value
    result["sum"] = format(result["sum"], "x")
    result["xor"] = format(result["xor"], "x")
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--selftest", action="store_true", required=True)
    args = parser.parse_args()
    probe = str(args.probe.resolve())
    valid = ["8", "grid", "5", "8", "additive", "pairwise-first"]
    invalid = [[], ["--help"], [*valid, "extra"]]
    for index, value in ((0, "-1"), (0, "1"), (0, "8x"), (1, "unknown"),
                         (2, "0"), (2, "11"), (3, "0"), (4, "pool"), (5, "baseline-first")):
        mutant = list(valid)
        mutant[index] = value
        invalid.append(mutant)
    invalid.extend((["7", "sheet_full", *valid[2:]], ["131074", "sheet_full", *valid[2:]],
                    ["32000", "tube", *valid[2:]], ["70000", "grid", *valid[2:]],
                    ["8", "rails", *valid[2:]]))
    for arguments in invalid:
        process = subprocess.run([probe, *arguments], capture_output=True)
        require(process.returncode == 2 and not process.stdout and bool(process.stderr),
                f"bad CLI rejection/partial output: {arguments}")
    cases = [(n, family, kmax) for n, family, kmax in itertools.product(
        (8, 18, 32), ("grid", "sheet", "sheet_full", "skew", "tube"), (1, 5, 10))]
    cases.extend((128, "sheet_full", kmax) for kmax in (5, 10))
    identities, work, outputs = {}, {}, {}
    rows = interior_ids = shell_extras = rejected = query_splits = 0
    for n, family, kmax in cases:
        expected = oracle(n, family, kmax)
        for separation, prefilter, order in itertools.product(
                (8, 10, 12), ("independent", "additive", "intersection_pool"),
                ("pairwise-first", "shared-first")):
            command = [probe, str(n), family, str(kmax), str(separation), prefilter, order]
            process = subprocess.run(command, capture_output=True)
            require(process.returncode == 0 and not process.stderr,
                    f"q2 probe failed: {command}: {process.stderr!r}")
            row = parse_result(process.stdout)
            validate_result(row, command)
            cross_check(row, identities, work, outputs)
            for arm in row["arms"]:
                require(arm["digest"] == expected,
                        "q2 materialized support/key/interior/shell digest differs from direct all-site oracle")
                interior_ids += arm["digest"]["interior_ids"]
                shell_extras += arm["digest"]["shell_ids"] - 2 * arm["accepted_pairs"]
                rejected += arm["rejected_pairs"]
            query_splits += row["arms"][1]["work"]["query_splits"]
            rows += 1
    require(len(invalid) == 17 and len(cases) == 47 and rows == 846 and
            interior_ids > 0 and shell_extras > 0 and rejected > 0 and query_splits > 0,
            "q2 CLI gate was vacuous")
    print(json.dumps(dict(status="passed", invalid_cli=17, oracle_cases=47, probe_rows=rows,
                         python_optimized=bool(sys.flags.optimize),
                         scope="tiny_q2_payload_digest_oracle_not_full_hgp")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
