#!/usr/bin/env python3
"""Read-only portable reader; accepts raw capture or content-addressed packet."""
import hashlib
import json
from pathlib import Path
import sys
from typing import Any


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise RuntimeError(reason)


def main() -> None:
    root = Path(__file__).resolve().parent
    manifest_path = root / "logical_manifest.json"
    manifest = json.loads(manifest_path.read_text()) if manifest_path.exists() else None

    def raw(name: str) -> bytes:
        if manifest is None:
            return (root / name).read_bytes()
        return (root / "objects" / (manifest["files"][name] + ".source")).read_bytes()

    def js(name: str) -> Any:
        return json.loads(raw(name))

    def lines(name: str) -> list[dict[str, Any]]:
        return [json.loads(line) for line in raw(name).decode().splitlines() if line.startswith("{")]

    if manifest is not None:
        for name, expected in manifest["files"].items():
            need(hashlib.sha256(raw(name)).hexdigest() == expected, f"file hash: {name}")
        names = sorted(manifest["files"])
    else:
        names = [str(p.relative_to(root)) for p in sorted((root / "runs").rglob("receipt.json"))]
    commands = [js(name) for name in names if name.startswith("runs/") and name.endswith("receipt.json")]
    command_names = {command["name"] for command in commands}
    required = {"compile_baseline_gate", "baseline_gate", "observed_gate", "observed_gate_nocache",
                "baseline_n400", "observed_n400", "observed_n400_nocache", "compile_observed_gate_san",
                "observed_gate_san", "compiler_version", "uname", "cpu",
                "dependencies_gate", "dependencies_probe", "observed_uniform_n8000_s8",
                "observed_scanline_overlap_multiecho_n2000_s8"}
    required.update("compile_r2_" + name for name in ["baseline_gate", "observed_gate", "baseline_probe", "observed_probe"])
    required.update(prefix + name for name in ["after_cache", "count_seeds", "count_descent", "hash_only"]
                    for prefix in ["compile_mutant_", "mutant_"])
    san_replayed = "observed_gate_san_r2" in command_names
    san_names = (["observed_gate_san_r2", "observed_gate_san_nocache_r2"] if san_replayed
                 else ["observed_gate_san", "observed_gate_san_nocache"])
    required.update(san_names)
    need(required <= command_names and len(command_names) == len(commands), "required unique commands")
    expected_failed = {"compile_baseline_gate": 1, **{f"mutant_{name}": 1 for name in
                       ["after_cache", "count_seeds", "count_descent", "hash_only"]}}
    if san_replayed:
        expected_failed["observed_gate_san"] = 1
        refused = raw("runs/observed_gate_san/stderr").decode()
        need("LeakSanitizer" in refused and "ptrace" in refused, "preserved sanitizer environment refusal")
        original = js("runs/observed_gate_san/receipt.json")
        for name in san_names:
            replay = js(f"runs/{name}/receipt.json")
            need(original["executables"] == replay["executables"] and
                 replay["environment_overlay"]["ASAN_OPTIONS"] == "detect_leaks=1:halt_on_error=1",
                 "sanitizer replay preserves binary and leak detection")
    for command in commands:
        name = command["name"]
        need(command["exit_code"] == expected_failed.get(name, 0), f"exit: {name}")
        need(command["sources_stable"] and command["sources_before"] == command["sources_after"], f"source drift: {name}")
        for source, expected in command["sources_before"].items():
            need(hashlib.sha256(raw(source)).hexdigest() == expected, f"source pin: {name}/{source}")
        for stream in ["stdout", "stderr"]:
            need(hashlib.sha256(raw(f"runs/{name}/{stream}")).hexdigest() == command[f"{stream}_sha256"], f"stream: {name}")
    pins = js("published_source_manifest.json")
    need(pins["revision"] == "ad7ffd28b35e153a20bd8cf42534d1cd29160bcd", "revision")
    for source, expected in pins["files"].items():
        need(hashlib.sha256(raw("baseline/" + source)).hexdigest() == expected, "baseline publication pin")
    nominal = lines("runs/baseline_gate/stdout")[0]
    observed = lines("runs/observed_gate/stdout")
    need(nominal == observed[0], "observer changed independent gate aggregate")
    need(nominal["checks"] == 170320 and nominal["clouds"] == 28 and nominal["orders"] == 112,
         "Gram/Gamma floor")
    gate_names = ["observed_gate", "observed_gate_nocache"] + san_names
    traces = []
    for name in gate_names:
        row = lines(f"runs/{name}/stdout")[-1]
        need(row["observer_gate"] == "passed" and row["completed"] == 28 and row["rows"] == 112,
             "observer gate completion")
        need(min(row[k] for k in ["duplicate_rows", "distinct_rows", "zero_rows", "pairwise_checks"]) > 0,
             "observer nonvacuity")
        traces.append(row["transcript_digest"])
    need(len(set(traces)) == 1, "cache or sanitizer changed initial-key sequence")
    for name in ["after_cache", "count_seeds", "count_descent", "hash_only"]:
        text = raw(f"runs/mutant_{name}/stderr").decode()
        diagnostic = "observer.pairwise_unique" if name == "hash_only" else "observer.occurrence_delta"
        need(diagnostic in text, f"causal mutant diagnostic: {name}")

    def comparable(row: dict[str, Any]) -> dict[str, Any]:
        return {k: v for k, v in row.items() if not k.endswith("_s") and k not in ["observer", "family"]}

    a = lines("runs/baseline_n400/stdout")[0]
    b = lines("runs/observed_n400/stdout")[0]
    c = lines("runs/observed_n400_nocache/stdout")[0]
    need(comparable(a) == comparable(b), "observer changed nominal geometry/counters/payload")
    def identities(row: dict[str, Any]) -> list[tuple[int, int, int, str]]:
        return [(r["k"], r["representative_occurrences"], r["unique_representative_keys"], r["unique_digest"])
                for r in row["observer"]["rows"]]
    need(identities(b) == identities(c) and a["payload_digest"] == c["payload_digest"], "cache changed initial keys/payload")
    measured = ["observed_n400", "observed_n400_nocache", "observed_uniform_n8000_s8", "observed_scanline_overlap_multiecho_n2000_s8"]
    measured += [f"observed_uniform_n8000_s{s}" for s in [10, 12]
                 if f"observed_uniform_n8000_s{s}" in command_names]
    scale_rows = []
    for name in measured:
        row = lines(f"runs/{name}/stdout")[0]
        need(row["status"] == "completed_relative" and row["public_status"] == "not_claimed" and
             row["contract_qualified"] is False and row["orders"] == row["kmax"] == 10, "tower completion")
        obs = row["observer"]
        need(obs["instrumented_not_benchmark"] is True, "instrumented authority")
        need([r["k"] for r in obs["rows"]] == list(range(1, 11)), "all orders including K1")
        need(obs["rows"][0]["unique_representative_keys"] == row["n"] and
             obs["rows"][0]["resolver_meb_calls"] == obs["rows"][0]["cache_queries"] == 0,
             "measured K1 covers all sites without MEB/cache")
        for r in obs["rows"]:
            need(0 <= r["unique_representative_keys"] <= r["representative_occurrences"], "U<=R")
            need(r["duplicate_occurrences"] == r["representative_occurrences"] - r["unique_representative_keys"], "duplicates")
            need(r["key_logical_bytes"] == 40 * r["representative_occurrences"] <= r["key_capacity_bytes"], "memory accounting")
        for source, dest in [("representatives", "representative_occurrences"), ("resolver_meb_calls", "resolver_meb_calls"),
                             ("resolver_cache_hits", "cache_hits"), ("resolver_cache_seed_stores", "cache_seed_stores"),
                             ("resolver_cache_queries", "cache_queries")]:
            need(row[source] == sum(r[dest] for r in obs["rows"]), f"order sum: {source}")
        if name.startswith("observed_uniform_n8000"):
            scale_rows.append(row)
    if len(scale_rows) > 1:
        need(len({row["payload_digest"] for row in scale_rows}) == 1, "s payload equality")
        need(all(identities(row) == identities(scale_rows[0]) for row in scale_rows), "s exact initial-key equality")
    scan = lines("runs/observed_scanline_overlap_multiecho_n2000_s8/stdout")[0]
    need(scan["extra_records"] > 0 and scan["same_radius_steps"] > 0, "structured nonregular floor")
    historical_pin = js("historical_reference/pin.json")
    need(historical_pin["not_a_new_measurement"] is True and
         hashlib.sha256(raw("historical_reference/uniform_n8000_s8.stdout")).hexdigest() == historical_pin["sha256"],
         "historical reference pin")
    need(comparable(lines("historical_reference/uniform_n8000_s8.stdout")[0]) == comparable(scale_rows[0]),
         "observer changed historical same-source 8k geometry/counters/payload")
    print(json.dumps({"status": "passed", "commands": len(commands), "measured_towers": len(measured),
                      "mutants_rejected": 4, "uniform_8k_s_values": sorted(row["s"] for row in scale_rows),
                      "public_status": "not_claimed", "instrumented_not_benchmark": True}))


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"VERIFY FAILED: {e}", file=sys.stderr)
        raise SystemExit(1)
