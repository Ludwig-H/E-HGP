#!/usr/bin/env python3
"""Read-only review of sealed constructor captures; no imported judge or engine."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import shlex

AUDIT = Path(__file__).resolve().parents[1]
PACK = AUDIT.parent / "receipts/full_meb_product_20260906"
SEAL = "bbdbc40dfb737b3dc78b3fd6a146e6f11324f2e892f43e54307ddf7e8c451a2b"
HEADER = "a946e31dde8fbd8ec528d6f5e94f9c727998acc172b4dd29c084dd522c730d1d"
HELPER = "f922544b5cfdc214de96ecd49520e318ea8632d14a8142ef21fd248f9cc38fb3"
FAULT_GATE = "079ee371a04eabd78dead3413b004e9d4511766bcc5cdccf47a63b1666b3673f"
FAULT_HOOK = "61f9e5413ba8a6fc41ae506e4c24c601803fda9877113b099a6adeff8f923d6d"
CHECKS = 0


def require(value: bool, reason: str) -> None:
    global CHECKS
    CHECKS += 1
    if not value:
        raise RuntimeError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path: Path):
    return json.loads(path.read_text())


def main() -> None:
    require(sha(PACK / "SHA256SUMS") == SEAL, "publication seal pin")
    sealed = {}
    for line in (PACK / "SHA256SUMS").read_text().splitlines():
        digest, name = line.split("  ", 1)
        require(name not in sealed and not Path(name).is_absolute()
                and ".." not in Path(name).parts, "unique safe sealed relative path")
        require(sha(PACK / name) == digest, "sealed bytes: " + name)
        sealed[name] = digest
    actual = {str(p.relative_to(PACK)) for p in PACK.rglob("*") if p.is_file()}
    require(len(sealed) == 1250 and actual == set(sealed) | {"SHA256SUMS"},
            "closed inventory: 1250 payload files and one seal")
    publication = read(PACK / "publication.json")
    for group, digest in publication["admitted_runs"].items():
        require(sha(PACK / group / "run.json") == digest, "admitted run pin")
    core = PACK / "core"
    run = read(core / "run.json")
    require(run["status"] == "passed" and run["sources_stable"], "core completion")
    before, after = read(core / "sources_before.json"), read(core / "sources_after.json")
    require(before == after and len(before) == 120, "120 unchanged core source pins")
    for path, digest in before.items():
        require(sha(core / "source_snapshot" / path) == digest, "captured source: " + path)
    require(before["morsehgp3D_v7/src/forest/full_gabriel.hpp"] == HEADER
            and before["morsehgp3D_v7/src/forest/meb_proposal.hpp"] == HELPER,
            "reviewed nominal product bytes")
    require(run["environment_overrides"]["ASAN_OPTIONS"] == "detect_leaks=1:halt_on_error=1"
            and run["environment_overrides"]["UBSAN_OPTIONS"] == "halt_on_error=1:print_stacktrace=1",
            "recorded sanitizer environment")
    for command in run["commands"]:
        require(command["exit_code"] == 0 and command["status"] == "completed"
                and command["process_group_closed"], "closed successful core command")
        for stream in ("stdout", "stderr"):
            require(sha(core / (command["name"] + "." + stream)) == command[stream + "_sha256"],
                    "core command stream binding")
    wrapper = core / "source_snapshot/morsehgp3D_v7/cmake/run_expect.cmake"
    require(sha(wrapper) == "7bb9af4eddbea7e7a66619c92491dc11c22a49d7b3b46bddd6b2c25b20b2d605",
            "reviewed exact-code wrapper; signals cannot pass")
    expected_names = {
        "certificate", "certificate_rejects", "gabriel", "gabriel_rejects", "gabriel_bad_argument",
        "gabriel_allocation", "gabriel_allocation_bad_argument", "gabriel_lazy", "gabriel_lazy_rejects",
        "gabriel_lazy_bad_argument", "gabriel_lazy_allocation", "gabriel_lazy_allocation_bad_argument",
        "gabriel_singleton", "gabriel_singleton_rejects", "gabriel_singleton_bad_argument",
        "gabriel_successor", "gabriel_successor_rejects", "gabriel_successor_bad_argument",
        "gabriel_meb_selftest", "gabriel_meb_rejects", "gabriel_meb_bad_argument",
        "gabriel_meb_allocation_low", "gabriel_meb_lazy_allocation_low",
        "gabriel_meb_allocation_large", "gabriel_meb_lazy_allocation_large",
        "gabriel_digest", "gabriel_digest_bad_argument",
    }
    expected_names = {"mhgp7_full_" + name for name in expected_names}
    expected_names |= {"mhgp7_meb_proposal_local_" + x for x in ("selftest", "rejects", "bad_argument")}
    core_results = {}
    for build in ("release", "san"):
        log = (core / (build + "_ctest.stdout")).read_text()
        starts = re.findall(r"Start\s+(\d+): (\S+)", log)
        passed = re.findall(r"\d+/30 Test\s+#(\d+): (\S+)\s+\.+\s+Passed", log)
        require(len(starts) == 30 and starts == passed
                and {x[1] for x in starts} == expected_names, "30 exact CTest identities passed")
        require("100% tests passed, 0 tests failed out of 30" in log, "CTest aggregate closes")
        commands = dict(re.findall(r"^(\d+): Test command: (.+)$", log, re.M))
        expected_codes = {}
        for number, name in starts:
            args = shlex.split(commands[number])
            code = 2 if name.endswith("_bad_argument") else 0
            require("-DEXPECTED=" + str(code) in args and args[-1].endswith("/cmake/run_expect.cmake"),
                    "expected numeric exit per CTest")
            binary = next(x.split("=", 1)[1].split("/")[-1] for x in args if x.startswith("-DCMD="))
            require(binary in run["binaries"][build], "CTest executable has captured ELF identity")
            expected_codes[name] = code
        summaries = [json.loads(x) for x in re.findall(r"^\d+: (\{.+\})$", log, re.M)]
        require(len(summaries) == 4, "four local/FULL semantic summaries")
        local = [x for x in summaries if x["schema"] == "mhgp7-meb-proposal-local-v1"]
        require(len(local) == 2 and all(x["status"] == "passed" for x in local)
                and {x["test_mode"]: x["calls"] for x in local} == {"selftest": 109, "rejects": 45},
                "local counter-F nominal/rejection recorded summaries")
        full = [x for x in summaries if x["schema"] == "mhgp7-full-gabriel-meb-gate-v1"]
        for item in full:
            require(item["status"] == "passed" and item["failures"] == 0
                    and item["orders"] == 93 and item["matrix_calls"] == 1488
                    and item["gamma_runs"] == 1488 and item["cuts"] == 33792,
                    "bounded FULL/Gamma recorded counts")
            require(item["persistent_cases"] == 42 and item["saved_reference_cases"] == 267,
                    "persistent and nonzero physical-F saving witnesses")
        rejects = next(x for x in full if x["test_mode"] == "--rejects")
        require(rejects["budgets"] == [{"exact": 32, "one_short": 32}] * 5
                and rejects["metadata_rejects"] == 28 and rejects["retained_proposal_refusals"] == 120,
                "five physical caps and retained-work refusals")
        eager = re.findall(r"^\d+: full_gabriel_allocation [^\n]+", log, re.M)
        lazy = re.findall(r"^\d+: full_gabriel_lazy_allocation [^\n]+", log, re.M)
        require(len(eager) == 3 and len(lazy) == 3, "P0/P1/large sweeps")
        for row in eager:
            require("allocations=49 fault_runs=49 denied=49 escaped=0" in row
                    and "failures=0" in row, "49 eager failures per P")
        for row in lazy:
            require("allocations=209 fault_runs=209 denied=209 escaped=0" in row
                    and "failures=0" in row, "209 lazy failures per P")
        compile_rows = read(core / "compilation" / build / "compile_commands.json")
        selected = [x for x in compile_rows if any("CMakeFiles/" + target + ".dir/" in x["output"]
                                                 for target in run["binaries"][build])]
        require(len(selected) == 10, "ten actual selected targets, not all configured targets")
        macros = []
        for entry in selected:
            args = shlex.split(entry["command"])
            require(all(x in args for x in ("-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror")),
                    "strict compiled C++ flags")
            if build == "san":
                require("-fsanitize=address,undefined" in args and "-fno-sanitize-recover=all" in args,
                        "ASan/UBSan nominal compile flags")
            if any("MHGP7_TESTING" in x for x in args):
                macros.append(Path(entry["file"]).name)
        require(sorted(macros) == ["full_gabriel_singleton_gate.cpp", "full_gabriel_successor_gate.cpp"],
                "only two historical differential targets use testing macro")
        core_results[build] = {"tests": 30, "expected_codes": expected_codes,
                               "semantic_summaries": summaries, "compiled_targets": 10,
                               "product_targets_without_testing_macro": 8}
    require(core_results["release"] == core_results["san"], "same nominal semantic summaries in both builds")

    mutation = PACK / "mutations"
    mr = read(mutation / "run.json")
    require(mr["status"] == "completed" and mr["sources_stable"] and len(mr["commands"]) == 15,
            "15 closed mutation/injection commands")
    require(read(mutation / "inputs_before.json") == read(mutation / "inputs_after.json"),
            "mutation runner input identities unchanged")
    environment = read(mutation / "environment.json")
    require(environment["ASAN_OPTIONS"] == "detect_leaks=1:halt_on_error=1"
            and environment["UBSAN_OPTIONS"] == "halt_on_error=1:print_stacktrace=1"
            and environment["LSAN_OPTIONS"] == "exitcode=23", "private injection sanitizer environment")
    for name, command in mr["commands"].items():
        require(command == read(mutation / "commands" / (name + ".json")), "command duplicated record agrees")
        intent = read(mutation / "commands" / (name + ".intent.json"))
        require(all(command.get(key) == value for key, value in intent.items()), "recorded intent preserved")
        spawn = read(mutation / "commands" / (name + ".spawn.json"))
        require(spawn["pid"] == command["pid"] and spawn["pgid"] == command["pgid"]
                and command["process_group_closed"] and command["error"] is None,
                "spawn/process closure binding")
        require(command["exit_code"] == command["expected_exit_code"], "exact mutation/injection exit")
        for stream in ("stdout", "stderr"):
            path = mutation / "commands" / (name + "." + stream)
            require(sha(path) == command[stream + "_sha256"]
                    and path.stat().st_size == command[stream + "_bytes"], "mutation output binding")
        if name.endswith("_compile"):
            args = command["argv"]
            require(all(x in args for x in ("-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror"))
                    and not any("MHGP7_TESTING" in x for x in args), "strict private variant flags")
        elif name != "compiler_version":
            binary = Path(command["argv"][0]).name
            require(command["executable_sha256"] == mr["binaries"][binary]["sha256"],
                    "executed binary identity recorded")
    variants = {}
    for name in mr["variants"]:
        description = read(mutation / "variants" / (name + ".json"))
        original = mutation / "snapshot" / description["target"]
        changed = mutation / "variants" / name / description["target"]
        require(sha(original) == description["before_sha256"] and sha(changed) == description["after_sha256"],
                "variant before/after identities")
        old = original.read_text()
        require(old.count(description["old"]) == 1
                and old.replace(description["old"], description["new"]) == changed.read_text(),
                "exactly one declared causal replacement")
        for path, digest in description["source_pins"].items():
            require(sha(mutation / "variants" / name / path) == digest, "complete variant source binding")
        variants[name] = {"target": description["target"], "before": description["before_sha256"],
                          "after": description["after_sha256"]}
    for name, expected_cause in {
        "charge_after": "observer.prospective_P_charge",
        "drop_A": "physical_F.only_real_fallback_supports_not_virtual_ordinal",
        "reset_Work": "P=0 retains only F work with A=c",
        "drop_FULL_P_mirror": "P=0 retains only F work with A=c",
    }.items():
        first = (mutation / "commands" / (name + "_run.stderr")).read_text().splitlines()[0]
        require(expected_cause in first and first == mr["mutations"][name]["first_diagnostic"]
                and mr["commands"][name + "_run"]["exit_code"] == 1, "causal first mutant diagnostic")
    fault_root = mutation / "variants/form_fault/build/v7_meb_product_fault_20260906"
    require(sha(fault_root / "full_fault_gate.cpp") == FAULT_GATE
            and sha(fault_root / "fault_hook.hpp") == FAULT_HOOK, "statically reviewed injection protocol")
    fault_summary = read(mutation / "commands/form_fault_o2_selftest.stdout")
    require(fault_summary == read(mutation / "commands/form_fault_san_selftest.stdout")
            == mr["fault"]["o2"] == mr["fault"]["san"], "identical injection summaries")
    expected = {"cases": 12, "public_refusals": 4, "runtime_propagations": 2,
                "builder_propagations": 6, "mirrors": 10, "compared_mirrors": 8,
                "baselines": 2, "retries": 6, "paid_at_throw": 36, "failures": 0,
                "nominal_noobserver_exception_claim": False, "F_exception_coverage": "not_exercised"}
    require(all(fault_summary[key] == value for key, value in expected.items()), "12 late injections with scoped claims")
    for build in ("o2", "san"):
        cc = mr["commands"]["form_fault_" + build + "_compile"]
        require("-include" in cc["argv"] and any(x.endswith("/fault_hook.hpp") for x in cc["argv"]),
                "explicit force-included private hook")
        if build == "san":
            require("-fsanitize=address,undefined" in cc["argv"]
                    and "-fno-sanitize-recover=all" in cc["argv"], "private ASan/UBSan compile")
        require(mr["commands"]["form_fault_" + build + "_selftest"]["exit_code"] == 0
                and mr["commands"]["form_fault_" + build + "_unknown"]["exit_code"] == 2,
                "fault gate success and exact bad argument")
    failures = []
    for path in sorted((PACK / "failed_attempts").rglob("run.json")):
        record = read(path)
        require(record["status"] == "failed", "failed attempt remains failed")
        failed = [(x["name"], x["exit_code"]) for x in record["commands"] if x["exit_code"] != 0]
        failures.append({"path": str(path.relative_to(PACK)), "failed_commands": failed})
    require([x["failed_commands"] for x in failures] == [[("release_configure", 1)], [("san_build", 2)]],
            "Boost configure and temporary-file-limit incidents preserved")
    result = {"status": "passed", "review_kind": "independent_read_only_no_imported_judge_no_engine",
              "packet_seal_sha256": SEAL, "sealed_payload_files": len(sealed), "checks": CHECKS,
              "product_full_sha256": HEADER, "product_meb_sha256": HELPER,
              "core": core_results, "mutations": variants, "late_injections_per_build": fault_summary,
              "late_injection_evidence": "compiled check code plus aggregate stdout; individual states not published",
              "late_prefix_scope": "p=3, certified=2, outer_calls=3, geometry_calls=2; F fallback and A remain zero",
              "runtime_wrapper_private_result_observable": False,
              "other_limits": ["no C++ rerun", "ELF hashes bound to receipts, binaries not distributed",
                               "system dependencies recorded after run, not hermetic",
                               "extra packet sealed but its scientific judgments not independently rerun here",
                               "FULL fast-q4 dispatch not demonstrated by q4 catalogue counts",
                               "no K9/K10 FULL oracle, CLI, archive, vertical or performance qualification"],
              "failed_attempts": failures, "cpp_closed_utc": run["ended"],
              "review_script_sha256": sha(Path(__file__)), "public_status": "not_claimed", "gcp": "not_used"}
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
