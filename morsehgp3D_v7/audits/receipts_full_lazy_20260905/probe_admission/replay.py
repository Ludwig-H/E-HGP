#!/usr/bin/env python3
"""Replay only Python judges over the closed n=8 admission package."""
from __future__ import annotations

import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
from typing import Any


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
PACKAGE = ROOT / "morsehgp3D_v7/receipts/full_gabriel_lazy_probe_20260905"
JUDGE = PACKAGE / "protocol/probe_audit.py"
JUDGE_SHA = "8d8a612aa973cb79e60e97a6675f63684ddd8892cfc550716c20620c4d6930ef"
Json = dict[str, Any]


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path: Path) -> Any:
    return json.loads(path.read_text())


def verify_package() -> Json:
    manifest = read(PACKAGE / "manifest.json")
    require(len(manifest) == 467, "manifest_inventory_floor")
    expected = set(manifest) | {"manifest.json", "SHA256SUMS"}
    actual = {str(p.relative_to(PACKAGE)) for p in PACKAGE.rglob("*") if p.is_file()}
    require(expected == actual, "package_inventory")
    for name, info in manifest.items():
        path = PACKAGE / name
        require(path.stat().st_size == info["bytes"] and sha(path) == info["sha256"], "manifest_binding:" + name)
    sums: dict[str, str] = {}
    for line in (PACKAGE / "SHA256SUMS").read_text().splitlines():
        value, name = line.split("  ", 1)
        require(name not in sums, "duplicate_checksum")
        sums[name] = value
    require(set(sums) == set(manifest) | {"manifest.json"}, "checksum_inventory")
    require(all(sha(PACKAGE / name) == value for name, value in sums.items()), "checksum_binding")
    publication = read(PACKAGE / "publication.json")
    micro = read(PACKAGE / "micro/receipt.json")
    build = read(PACKAGE / "build/receipt.json")
    require(publication["status"] == "closed_captures_published_not_SLO", "publication_closed")
    require(micro["status"] == build["status"] == "completed", "receipts_closed")
    require(publication["micro_receipt_sha256"] == sha(PACKAGE / "micro/receipt.json"), "micro_receipt_pin")
    require(publication["build_receipt_sha256"] == sha(PACKAGE / "build/receipt.json"), "build_receipt_pin")
    sources = read(PACKAGE / "build/sources_before.json")["files"]
    require(len(sources) == 54, "source_inventory_floor")
    require(sources == read(PACKAGE / "build/sources_after.json")["files"], "build_source_stability")
    for path in (PACKAGE / "micro").rglob("*.sources_before.json"):
        before = read(path)
        after = read(path.with_name(path.name.replace(".sources_before.json", ".sources_after.json")))
        require(before == after and before["files"] == sources, "attempt_source_stability")
    for name in ("micro/sources_before.json", "micro/sources_after.json"):
        require(read(PACKAGE / name)["files"] == sources, "micro_source_stability")
    dependencies = read(PACKAGE / "build/dependencies.json")
    require(len(dependencies) == build["dependency_count"] == 40, "dependency_count")
    require(all(sources.get(name) == value for name, value in dependencies.items()), "dependency_source_binding")
    require(sha(JUDGE) == JUDGE_SHA, "published_judge_pin")
    return {"manifest_entries": len(manifest), "checksum_entries": len(sums),
            "published_files": len(actual), "source_map_entries": len(sources),
            "dependency_map_entries": len(dependencies),
            "micro_ended": micro["ended"], "build_ended": build["ended"],
            "binary_sha256_declared": micro["binary_sha256"],
            "binary_reexecuted": False, "binary_omission": publication["omissions"],
            "envelope_pins": {name: sha(PACKAGE / name) for name in
                              ("manifest.json", "SHA256SUMS", "publication.json", "micro/receipt.json", "build/receipt.json")}}


def check_command(path: Path, expected_code: int) -> Json:
    value = read(path)
    require(value["status"] == "completed" and value["exit_code"] == expected_code
            and value["error"] is None, "recorded_command_status:" + path.name)
    for name, info in value["streams"].items():
        stream = path.parent / name
        require(stream.stat().st_size == info["bytes"] and sha(stream) == info["sha256"], "command_stream_binding")
    return value


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--result", type=Path, required=True)
    args = parser.parse_args()
    package = verify_package()
    python = [sys.executable, "-B"] + (["-O"] if sys.flags.optimize else [])
    commands: list[Json] = []
    orders = lazy_rows = eager_rows = positive_portal_rows = 0
    groups: dict[int, list[tuple[str, tuple[str, ...], str]]] = {}
    paths = sorted((PACKAGE / "micro").glob("k*/n8_*.receipt.json"))
    require(len(paths) == 24, "attempt_floor")
    expected_ids = {f"n8_s{s}_k{k}_{policy}_c{cap}" for s in (8, 10, 12)
                    for k in (5, 10) for policy, cap in
                    (("eager", 0), ("lazy", 0), ("lazy", 1), ("lazy", 1000000))}
    require({path.name.removesuffix(".receipt.json") for path in paths} == expected_ids, "attempt_population")
    for path in paths:
        argv = python + [str(JUDGE), str(path)]
        process = subprocess.run(argv, capture_output=True, text=True, timeout=20, check=False)
        require(process.returncode == 0 and process.stderr == "", "judge_replay:" + path.name)
        verdict = read(path.with_name(path.name.replace(".receipt.json", ".verdict.json")))
        result = json.loads(process.stdout)
        require(result["audit_status"] == "valid" and result["attempt_success"] is True, "attempt_success")
        require(result["certificate_digest"] == verdict["certificate_digest"], "stored_verdict_digest")
        commands.append({"argv": argv, "exit_code": process.returncode, "stdout": result, "stderr": ""})
        receipt = read(path)
        command = check_command(path.with_name(path.name.replace(".receipt.json", ".command.json")), 0)
        require(all(receipt[key] == command[key] for key in ("id", "started", "ended", "command", "exit_code", "streams")), "command_receipt_binding")
        terminal = receipt["terminal"]
        effective = min(8, terminal["kmax_requested"])
        require(len(receipt["orders"]) == effective, "effective_k")
        groups.setdefault(terminal["kmax_requested"], []).append((terminal["input_digest"], tuple(row["certificate_digest"] for row in receipt["orders"]), terminal["certificate_digest"]))
        for row in receipt["orders"]:
            orders += 1
            if result["alias_policy"] == "lazy":
                lazy_rows += 1
                positive_portal_rows += int(row["portal_requests"] > 0)
                require(row["cache_inserts"] == min(result["cache_entries"], row["portal_requests"]), "nominal_first_c")
            else:
                eager_rows += 1
    require((orders, lazy_rows, eager_rows) == (156, 117, 39), "order_population")
    require(positive_portal_rows > 0, "first_c_nonvacuum")
    require(set(groups) == {5, 10} and all(len(v) == 12 and len(set(v)) == 1 for v in groups.values()), "semantic_digest_groups")
    for rid in ("n8_s8_k10_eager_c0", "n8_s8_k10_lazy_c1"):
        path = PACKAGE / "micro/k10" / (rid + ".receipt.json")
        argv = python + [str(JUDGE), "--selftest", str(path)]
        process = subprocess.run(argv, capture_output=True, text=True, timeout=20, check=False)
        require(process.returncode == 0 and process.stderr == "", "selftest:" + rid)
        result = json.loads(process.stdout)
        require(len(result["mutants_killed"]) == 19, "nineteen_selftests")
        require(all(result[key] == 1 for key in ("real_positive", "synthetic_refusal", "synthetic_unemitted_read_refusal")), "selftest_floors")
        commands.append({"argv": argv, "exit_code": process.returncode, "stdout": result, "stderr": ""})
    parser_rejections: list[str] = []
    for path in sorted((PACKAGE / "micro").glob("reject_*.command.json")):
        command = check_command(path, 2)
        rid = command["id"]
        terminal = read(path.with_name(rid + ".stdout"))
        require(terminal["type"] == "terminal" and terminal["reason"] == "probe_arguments"
                and terminal["outcome"] == "invalid_input" and terminal["exit_code"] == 2,
                "parser_refusal:" + rid)
        require(terminal["certificate_digest"] == "" and terminal["complete_requested_horizontal_orders"] is False, "parser_no_publication")
        parser_rejections.append(rid)
    require(len(parser_rejections) == 11, "parser_floor")
    check_command(PACKAGE / "micro/digest_selftest.command.json", 0)
    digest_selftest = read(PACKAGE / "micro/digest_selftest.stdout")
    require(digest_selftest["checks"] == 24 and digest_selftest["failures"] == 0 and digest_selftest["passed"] is True, "recorded_cpp_digest_selftest")

    # The old helper counterexample can now be based on a real, closed receipt.
    spec = importlib.util.spec_from_file_location("admission_judge", JUDGE)
    require(spec is not None and spec.loader is not None, "import_spec")
    judge = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(judge)
    path = PACKAGE / "micro/k10/n8_s8_k10_lazy_c1000000.receipt.json"
    receipt = read(path)
    raw = path.with_name(path.name.replace(".receipt.json", ".raw.txt")).read_text()
    lines = raw.splitlines()
    split = next(i for i, line in enumerate(lines) if not line.startswith("{"))
    rows = [json.loads(line) for line in lines[:split]]
    index = next(i for i, row in enumerate(rows) if row.get("k") == 2)
    old = {key: rows[index][key] for key in ("cache_inserts", "cache_skips", "portal_requests")}
    require(old["cache_inserts"] > 0 and old["cache_skips"] == 0, "real_first_c_fixture_floor")
    rows[index]["cache_inserts"] -= 1
    rows[index]["cache_skips"] += 1
    changed = copy.deepcopy(receipt)
    changed.update(orders=rows[1:-1], terminal=rows[-1])
    changed_raw = "\n".join(json.dumps(row) for row in rows) + "\n" + "\n".join(lines[split:]) + "\n"
    result = judge.judge(changed_raw, changed,
                         read(path.with_name(path.name.replace(".receipt.json", ".intent.json"))),
                         read(path.parent / "protocol.json"))
    require(result["audit_status"] == "valid" and result["attempt_success"] is True, "first_c_gap_remains")
    require(rows[index]["cache_inserts"] != min(1000000, rows[index]["portal_requests"]), "first_c_independent_refutation")
    counterfixture = {"kind": "joint_raw_receipt_data_corruption_not_product_mutant",
                      "source_receipt": str(path.relative_to(ROOT)), "zero_based_raw_row": index,
                      "old_values": old, "new_values": {key: rows[index][key] for key in old},
                      "changed_raw_sha256": hashlib.sha256(changed_raw.encode()).hexdigest(),
                      "judge_result": result, "necessary_identity_refutes": True,
                      "historical_package_hashes_preserved": False}
    require(verify_package() == package, "package_changed_during_replay")
    result = {
        "schema": "mhgp7-lazy-probe-admission-independent-replay-v1", "status": "passed",
        "public_status": "not_claimed", "python_optimized": bool(sys.flags.optimize),
        "scope": "published_closed_n8_admission_only", "package": package,
        "script_sha256": sha(Path(__file__)), "judge_sha256": JUDGE_SHA,
        "counts": {"successful_receipts": 24, "completed_order_rows": orders,
                   "lazy_order_rows_first_c_checked": lazy_rows, "eager_order_rows": eager_rows,
                   "lazy_rows_with_positive_portals": positive_portal_rows,
                   "canonical_python_cli_calls": len(commands), "selftest_fixtures": 2,
                   "distinct_selftest_mutations_per_fixture": 19, "parser_refusals_read": 11,
                   "digest_selftest_checks_read": 24, "aggregate_groups_of_twelve": 2},
        "canonical_commands": commands, "parser_rejections_read": parser_rejections,
        "recorded_cpp_digest_selftest": digest_selftest, "first_c_counterfixture": counterfixture,
        "semantic_digest_groups": {str(k): {"input_digest": values[0][0], "order_digests": values[0][1], "aggregate_digest": values[0][2], "records": len(values)} for k, values in groups.items()},
        "engine_or_build_reexecuted": False, "running_or_heavy_campaign_followed": False,
    }
    args.result.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"status": "passed", "counts": result["counts"]}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
