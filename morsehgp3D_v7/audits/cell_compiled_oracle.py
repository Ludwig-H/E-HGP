"""Bounded compiled cell audit: squared-distance and rational center judges.

All output is under audits/. No product oracle or exhaustive pipeline is used.
The standalone bridge accepts only this trusted bounded protocol, not public data.
"""

from __future__ import annotations

from datetime import datetime, timezone
from fractions import Fraction as Q
import hashlib
import itertools
import json
import math
import os
from pathlib import Path
import subprocess
import sys
from typing import Any

from meb_rational_oracle_20260905 import circumball

AUDIT = Path(__file__).resolve().parent
ROOT = AUDIT.parent.parent
WORK = AUDIT / ".work_cell_compiled_20260905"
RECEIPTS = AUDIT / "receipts_front_compiled_20260905/cell"


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def dot(a: Any, b: Any) -> Any:
    return sum(x * y for x, y in zip(a, b))


def sub(a: Any, b: Any) -> tuple[Any, ...]:
    return tuple(x - y for x, y in zip(a, b))


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write_json(path: Path, value: Any) -> None:
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + "\n")


def cases() -> list[dict[str, Any]]:
    low = [(0, 4, 4), (8, 4, 4), (4, 9, 4), (4, 4, 4),
           (4, 4, 8), (4, 4, 0), (3, 4, 4), (5, 4, 4), (4, 7, 4)]
    clouds = [low, [tuple(1000 + 6000 * x for x in p) for p in low],
              [(0, 0, 0), (65535, 65535, 0), (65535, 0, 65535),
               (0, 65535, 65535), (32767, 32768, 32767), (1, 1, 1)],
              [(0, 0, 0), (65535, 65535, 65535)]
              + [p for p in itertools.product((0, 65535), repeat=3)
                 if p not in ((0, 0, 0), (65535, 65535, 65535))]]
    result = []
    for cloud, g, rho, h in itertools.product(clouds, (8, 16), (8, 12), (1, 3)):
        require(len(set(cloud)) == len(cloud), "fixture.duplicate")
        result.append(dict(kind="cloud", g=g, rho=rho, h=h, points=cloud))
    for g, width in itertools.product((8, 16), (64, 128)):
        scale = ((1 << 62) - 1) if width == 64 else 10**30
        step = scale // (8 * g)
        for du, dv, rhs in [(step, step, scale), (-step, step, -scale),
                            (step, -step, 0), (-step, -step, 0),
                            (0, step, 0), (0, 0, 0), (0, 0, -scale)]:
            if width == 64:
                require(4 * g * (abs(du) + abs(dv)) < 1 << 62,
                        "fixture.fast_guard")
            result.append(dict(kind="synthetic", g=g, width=width,
                               du=du, dv=dv, rhs=rhs))
    return result


def command(case: dict[str, Any]) -> str:
    if case["kind"] == "synthetic":
        return "S {g} {width} {du} {dv} {rhs}".format(**case)
    points = case["points"]
    return (f"C {case['g']} {case['rho']} {case['h']} {len(points)} "
            + " ".join(str(x) for p in points for x in p))


def exact_counts(case: dict[str, Any], observed: Any) -> tuple[list[int], int]:
    g = case["g"]
    counts, shell = [], 0
    for j, i in itertools.product(range(-g, g), repeat=2):
        if case["kind"] == "synthetic":
            signs = [4 * (ii * case["du"] + jj * case["dv"]) - case["rhs"]
                     for ii, jj in itertools.product((i, i + 1), (j, j + 1))]
            counts.append(int(all(x > 0 for x in signs)))
            shell += signs.count(0)
            continue
        a, b = case["points"][:2]
        u, v = observed["u"], observed["v"]
        centers = [tuple(g * (aa + bb) + 2 * (ii * uk + jj * vk)
                         for aa, bb, uk, vk in zip(a, b, u, v))
                   for ii, jj in itertools.product((i, i + 1), (j, j + 1))]
        count = 0
        for z in case["points"][2:]:
            signs = []
            for center in centers:
                za = sub(tuple(2 * g * x for x in a), center)
                zz = sub(tuple(2 * g * x for x in z), center)
                signs.append(dot(za, za) - dot(zz, zz))
            shell += signs.count(0)
            count += all(s > 0 for s in signs)
        counts.append(count)
    return counts, shell


def contains(box: list[int], coords: list[tuple[Q, Q]]) -> bool:
    if not box[0]:
        return False
    return all(box[1] < a < box[2] + 1 and box[3] < b < box[4] + 1
               for a, b in coords)


def exact_coordinates(pu: int, pv: int, den: int, u: Any, v: Any,
                      g: int) -> tuple[Q, Q]:
    uu, vv, uv = dot(u, u), dot(v, v), dot(u, v)
    determinant = uu * vv - uv * uv
    return (Q(g * (pu * vv - pv * uv), den * determinant),
            Q(g * (pv * uu - pu * uv), den * determinant))


def judge(case: dict[str, Any], output: Any) -> dict[str, int]:
    reference, shell = exact_counts(case, output)
    g = case["g"]
    stats = dict(cells=len(reference), shell_contacts=shell, seeds=0,
                 center_boxes=0, chord_boxes=0, environment_rejects=0,
                 domain_rejects=0, live_cells=0, dead_cells=0,
                 max_coordinate_bits=0)
    if case["kind"] == "synthetic":
        require(output == reference, "synthetic.counts")
        return stats
    require(output["counts"] == reference, "grid.counts")
    needed = [abs(Q(2 * i + 1, 2)) + abs(Q(2 * j + 1, 2)) - 1 <= g
              for j, i in itertools.product(range(-g, g), repeat=2)]
    dead = sum(n and c >= case["h"] for n, c in zip(needed, reference))
    require(output["metadata"] == [case["h"], sum(needed), dead,
                                    int(dead == sum(needed))], "grid.metadata")
    stats["live_cells"], stats["dead_cells"] = sum(needed) - dead, dead
    require(contains(output["origin"], [(Q(0), Q(0))]), "locate.origin_closed")
    require(output["environment_rejects"] == [1, 1, 1], "guard.environment")
    require(output["domain_rejects"] == [1] * 6, "guard.domain")
    stats["environment_rejects"], stats["domain_rejects"] = 3, 6
    a, b = case["points"][:2]
    d = sub(b, a)
    D2 = dot(d, d)
    expected_seeds = []
    for k, x in enumerate(case["points"][2:], 2):
        ax, bx = sub(x, a), sub(x, b)
        ea, eb = dot(ax, ax), dot(bx, bx)
        if max(ea, eb) <= D2 and ea + eb > D2:
            expected_seeds.append(k)
    require([s["index"] for s in output["seeds"]] == expected_seeds,
            "seed.completeness")
    u, v = output["u"], output["v"]
    for seed in output["seeds"]:
        x = case["points"][seed["index"]]
        ball = circumball([a, b, x])
        require(ball is not None, "oracle.center_rank")
        center = ball["center"]
        offset = tuple(cc - Q(aa + bb, 2) for cc, aa, bb in zip(center, a, b))
        pu, pv, den = seed["center"]
        require(den > 0 and Q(pu, den) == dot(offset, u)
                and Q(pv, den) == dot(offset, v), "seed.center")
        coords = exact_coordinates(pu, pv, den, u, v, g)
        require(contains(seed["center_box"], [coords]), "locate.center_closed")
        stats["seeds"] += 1
        stats["center_boxes"] += 1
        stats["max_coordinate_bits"] = max(stats["max_coordinate_bits"],
                                             abs(pu).bit_length(), abs(pv).bit_length())
        ax = sub(x, a)
        normal = (d[1] * ax[2] - d[2] * ax[1], d[2] * ax[0] - d[0] * ax[2],
                  d[0] * ax[1] - d[1] * ax[0])
        gram = dot(normal, normal)
        # At radius²=3D²/8, displacement along the normal has parameter
        # mu²=(2Gram)²*(3D²/8-R²)/|normal|², a rational geometric identity.
        mu2 = 4 * gram * (Q(3 * D2, 8) - ball["radius"])
        require(mu2 >= 0, "oracle.chord_domain")
        mu = math.isqrt(mu2.numerator // mu2.denominator) + 1
        expected = [1, pu + mu * dot(normal, u), pv + mu * dot(normal, v),
                    pu - mu * dot(normal, u), pv - mu * dot(normal, v), den]
        require(seed["chord"] == expected, "seed.chord")
        ends = [exact_coordinates(expected[i], expected[i + 1], den, u, v, g)
                for i in (1, 3)]
        require(contains(seed["chord_box"], ends), "locate.chord_closed")
        stats["chord_boxes"] += 1
        stats["max_coordinate_bits"] = max(stats["max_coordinate_bits"],
                                             *(abs(t).bit_length() for t in expected[1:5]))
    return stats


def main() -> int:
    WORK.mkdir(exist_ok=True)
    RECEIPTS.mkdir(parents=True, exist_ok=True)
    snapshot_paths = [Path(__file__), AUDIT / "cell_compiled_bridge.cpp",
                      AUDIT / "meb_rational_oracle_20260905.py"]
    snapshot_paths += sorted((AUDIT.parent / "src").rglob("*.hpp"))
    before = {str(p.relative_to(ROOT)): sha(p) for p in snapshot_paths}
    dataset = cases()
    data = "\n".join(command(case) for case in dataset) + "\n"
    (RECEIPTS / "input.txt").write_text(data)
    started = datetime.now(timezone.utc).isoformat()
    builds = []
    environment = dict(os.environ, TMPDIR=str(WORK), PYTHONDONTWRITEBYTECODE="1")
    for name, options in (("o2", ["-O2"]),
                          ("ubsan", ["-O1", "-g", "-fsanitize=undefined",
                                     "-fno-sanitize-recover=all"])):
        directory = RECEIPTS / name
        directory.mkdir(exist_ok=True)
        binary = WORK / ("cell_" + name)
        argv = ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                "-DMHGP7_TESTING", *options, str(AUDIT / "cell_compiled_bridge.cpp"),
                "-pthread", "-o", str(binary)]
        record: dict[str, Any] = dict(command=argv, runs=[], started_utc=datetime.now(timezone.utc).isoformat())
        build = subprocess.run(argv, capture_output=True, text=True, env=environment,
                               check=False, timeout=180)
        (directory / "compile.stdout").write_text(build.stdout)
        (directory / "compile.stderr").write_text(build.stderr)
        record["compile_exit"] = build.returncode
        write_json(directory / "receipt.json", record)
        require(build.returncode == 0 and not build.stderr, "compile." + name)
        record["binary_sha256"] = sha(binary)
        for mutant in ("", "cell-kill-nonstrict", "cell-kill-h-minus-one",
                       "cell-locate-eps-zero"):
            label = mutant or "nominal"
            run = subprocess.run([str(binary)] + ([mutant] if mutant else []), input=data,
                                 capture_output=True, text=True, env=environment,
                                 check=False, timeout=90)
            (directory / (label + ".stdout")).write_text(run.stdout)
            (directory / (label + ".stderr")).write_text(run.stderr)
            require(run.returncode == 0 and not run.stderr, "execute." + label)
            outputs = [json.loads(line) for line in run.stdout.splitlines()]
            require(len(outputs) == len(dataset), "transport.cardinality")
            totals: dict[str, int] = {}
            failures = []
            for k, (case, output) in enumerate(zip(dataset, outputs)):
                try:
                    stats = judge(case, output)
                    for key, value in stats.items():
                        totals[key] = (max(totals.get(key, 0), value) if key.startswith("max_")
                                       else totals.get(key, 0) + value)
                except ValueError as exc:
                    failures.append(dict(case=k, reason=str(exc)))
            if mutant:
                reason = {"cell-kill-nonstrict": "grid.counts",
                          "cell-kill-h-minus-one": "grid.metadata",
                          "cell-locate-eps-zero": "locate.origin_closed"}[mutant]
                require(any(f["reason"] == reason for f in failures), "mutant.survived." + mutant)
            else:
                require(not failures, "nominal.divergences: " + str(failures[:3]))
                for key in ("shell_contacts", "seeds", "center_boxes", "chord_boxes",
                            "live_cells", "dead_cells", "environment_rejects", "domain_rejects"):
                    require(totals[key] > 0, "nonvacuity." + key)
                require(totals["max_coordinate_bits"] >= 90, "nonvacuity.wide_coordinates")
            record["runs"].append(dict(mutant=mutant, bridge_exit=run.returncode,
                                       totals=totals, divergences=failures,
                                       verdict="detected" if mutant else "pass"))
            write_json(directory / "receipt.json", record)
        record["ended_utc"] = datetime.now(timezone.utc).isoformat()
        write_json(directory / "receipt.json", record)
        builds.append(record)
    after = {str(p.relative_to(ROOT)): sha(p) for p in snapshot_paths}
    require(before == after, "sources.changed")
    summary = dict(schema="mhgp7-audit-compiled-cell-v1", started_utc=started,
                   ended_utc=datetime.now(timezone.utc).isoformat(), source_pins=before,
                   sources_stable=True, builds=builds, cases=len(dataset),
                   synthetic_cases=sum(c["kind"] == "synthetic" for c in dataset),
                   environment=dict(compiler=subprocess.check_output(["g++", "--version"], text=True),
                                    python=sys.version, python_optimized=bool(sys.flags.optimize)),
                   public_status="not_claimed", gcp_used=False)
    write_json(RECEIPTS / "summary.json", summary)
    print(json.dumps(dict(verdict="pass", cases=len(dataset), builds=len(builds),
                          totals=builds[0]["runs"][0]["totals"]), sort_keys=True))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (ValueError, subprocess.TimeoutExpired) as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
