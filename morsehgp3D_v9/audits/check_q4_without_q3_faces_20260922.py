#!/usr/bin/env python3
"""Exact twelve-site counterexample: q4 accepted, every q3 face rejected.

Audit-only oracle. Optional --probe exercises a built v9 public pipeline on
the same u18 sites, but its aggregate JSON cannot by itself prove the target
BallKey was emitted; the Fraction checks below establish the geometry.
"""

from __future__ import annotations

import argparse
from fractions import Fraction
import json
from pathlib import Path
import struct
import subprocess
import tempfile


Point = tuple[int, int, int]
O: Point = (20, 20, 20)
V: tuple[Point, ...] = (
    (30, 30, 30),
    (30, 10, 10),
    (10, 30, 10),
    (10, 10, 30),
)


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def signs(v: Point) -> Point:
    return tuple((v[j] - O[j]) // 10 for j in range(3))  # type: ignore[return-value]


def witnesses() -> tuple[Point, ...]:
    return tuple(
        tuple(O[j] - r * signs(v)[j] for j in range(3))  # type: ignore[misc]
        for v in V
        for r in (11, 12)
    )


def norm2(p: tuple[Fraction | int, ...], c: tuple[Fraction | int, ...]) -> Fraction:
    return sum((Fraction(p[j]) - Fraction(c[j])) ** 2 for j in range(3))


def check_exact() -> list[Point]:
    z = witnesses()
    sites = list(V + z)
    need(len(sites) == 12 and len(set(sites)) == 12, "distinct sites")
    need(all(0 <= x < 1 << 18 for p in sites for x in p), "u18 domain")
    need(all(norm2(V[i], V[j]) == 800 for i in range(4) for j in range(i)),
         "equal longest edges")
    need(tuple(sum(Fraction(v[j], 4) for v in V) for j in range(3)) == O,
         "positive quarter weights")
    need(all(norm2(v, O) == 300 for v in V), "q4 shell")
    q4_witness_powers = [norm2(p, O) - 300 for p in z]
    need(q4_witness_powers == [Fraction(v) for v in (63, 132) * 4],
         "q4 witnesses outside")

    for omitted in range(4):
        s = signs(V[omitted])
        center = tuple(Fraction(O[j]) - Fraction(10, 3) * s[j] for j in range(3))
        radius2 = Fraction(800, 3)
        need(all(norm2(V[j], center) == radius2 for j in range(4) if j != omitted),
             f"q3 face {omitted} shell")
        need(norm2(V[omitted], center) > radius2, f"q3 omitted support {omitted}")
        powers = [norm2(p, center) - radius2 for p in z]
        need(powers[2 * omitted:2 * omitted + 2] ==
             [Fraction(-271, 3), Fraction(-124, 3)], f"q3 inner pair {omitted}")
        need(all(power > 0 for j, power in enumerate(powers)
                 if j not in (2 * omitted, 2 * omitted + 1)),
             f"q3 other witnesses outside {omitted}")
        need(sum(power < 0 for power in powers) == 2, f"q3 depth {omitted}")
    need(0 < 3 - 2 and 2 >= 3 - 1, "K3 acceptance thresholds")
    return sites


def probe_pipeline(binary: Path, sites: list[Point]) -> dict[str, object]:
    permutations = {
        "identity": list(range(12)),
        "reverse_supports": [3, 2, 1, 0] + list(range(4, 12)),
        "witnesses_first": list(range(4, 12)) + list(range(4)),
        "reverse_all": list(reversed(range(12))),
    }
    results: dict[str, object] = {}
    with tempfile.TemporaryDirectory(prefix="mhgp9-q4-no-q3-") as directory:
        for name, order in permutations.items():
            payload = Path(directory) / f"{name}.u32le"
            payload.write_bytes(b"".join(struct.pack("<III", *sites[j]) for j in order))
            reference: dict[str, object] | None = None
            for s in (8, 10, 12):
                for workers in (1, 4):
                    run = subprocess.run(
                        [str(binary.resolve()), str(payload), "3", str(workers),
                         f"--s={s}", "--grid=diagnostic_fixture"],
                        check=False, capture_output=True, text=True, timeout=60,
                    )
                    label = f"{name}/s{s}/W{workers}"
                    need(run.returncode == 0, f"v9 probe refused ({label}): {run.stderr} {run.stdout}")
                    data = json.loads(run.stdout)
                    need(data["status"] == "complete_relative" and data["options"]["run_tower"],
                         f"v9 FULL status ({label})")
                    need(len(data["orders"]) == 3 and data["generator"]["q4_emitted"] > 0,
                         f"v9 q4/full route ({label})")
                    summary = {
                        "q4_emitted": data["generator"]["q4_emitted"],
                        "balls": data["catalogue"]["balls"],
                        "tower_digest": data["tower_digest"],
                    }
                    if reference is None:
                        reference = summary
                    else:
                        need(summary == reference, f"s/workers differential ({label})")
            need(reference is not None, f"missing v9 route ({name})")
            results[name] = reference
    need(len({item["q4_emitted"] for item in results.values()}) == 1,
         "q4 presentation count under permutations")
    need(len({item["balls"] for item in results.values()}) == 1,
         "catalogue size under permutations")
    return {"configurations_per_permutation": 6, "permutations": results}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe", type=Path, help="optional built mhgp9_tower_probe")
    args = parser.parse_args()
    sites = check_exact()
    result: dict[str, object] = {
        "status": "PASS",
        "sites": len(sites),
        "K": 3,
        "q4_depth": 0,
        "q3_face_depths": [2, 2, 2, 2],
        "owner_edge_ids": [0, 1],
    }
    if args.probe:
        result["probe_diagnostic"] = probe_pipeline(args.probe, sites)
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
