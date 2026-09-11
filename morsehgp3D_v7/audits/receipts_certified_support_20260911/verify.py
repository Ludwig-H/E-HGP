#!/usr/bin/env python3
"""Read-only integrity and bounded result checks; never compile or run geometry."""

import json

from reproduce import MODES, PACKET, SAN_ENV, STRICT, fail, read_sources, sha


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def numeric_fields(tokens: list[str]) -> dict[str, int]:
    result = {}
    for token in tokens:
        key, separator, value = token.partition("=")
        require(separator == "=" and key not in result and value.isdecimal(), "malformed field")
        result[key] = int(value)
    return result


def check_stdout(stdout: str) -> dict[str, int]:
    lines = stdout.splitlines()
    require(len(lines) == 24, "expected 21 cases, 2 rejections and 1 summary")
    expected_keys = {
        "permutation", "support", "n", "proposed_q", "canonical_q", "shell",
        "certification_attempts", "certification_positive_forms", "certification_powers",
        "full_canonical_attempts", "full_canonical_positive_forms", "canonical_attempts",
        "canonical_positive_forms", "full_powers", "support_powers", "skipped",
        "different_support", "different_arity", "direct_return", "six_fields_equal",
    }
    expected_cases = {
        (family, permutation, support)
        for family in ("diameter", "acute_triangle", "cube", "welzl_k7")
        for permutation in range(3)
        for support in range(4 if family == "cube" else 1)
    }
    seen = set()
    totals = {key: 0 for key in (
        "certification_powers", "full_powers", "support_powers", "different_support",
        "different_arity", "direct_return",
    )}
    dimensions = {"diameter": (5, 2, 2), "acute_triangle": (5, 3, 3),
                  "cube": (10, 2, 8), "welzl_k7": (7, 4, 4)}
    for line in lines[:21]:
        tokens = line.split()
        require(tokens[0] == "CASE" and tokens[1] in dimensions, "unexpected case")
        family = tokens[1]
        fields = numeric_fields(tokens[2:])
        require(set(fields) == expected_keys, "case fields differ")
        identity = (family, fields["permutation"], fields["support"])
        require(identity in expected_cases and identity not in seen, "case missing or duplicated")
        seen.add(identity)
        n, q, shell = dimensions[family]
        require((fields["n"], fields["canonical_q"], fields["shell"]) == (n, q, shell),
                "fixture dimensions differ")
        proposed_q = 4 if family == "cube" and fields["support"] < 2 else q
        require(fields["proposed_q"] == proposed_q, "proposed arity differs")
        require(fields["certification_attempts"] == 1 and
                fields["certification_positive_forms"] == 1 and
                fields["certification_powers"] == n, "certification cost differs")
        require(fields["six_fields_equal"] == 1 and fields["full_powers"] > fields["support_powers"],
                "differential equality or reduction failed")
        if family == "cube":
            require(fields["direct_return"] == 0 and fields["different_support"] == 1 and
                    fields["different_arity"] == int(proposed_q == 4) and
                    fields["canonical_attempts"] == fields["full_canonical_attempts"] == 7 and
                    fields["canonical_positive_forms"] == fields["full_canonical_positive_forms"] == 7 and
                    fields["skipped"] > 0, "wide shell path differs")
        else:
            require(fields["direct_return"] == 1 and fields["different_support"] == 0 and
                    fields["different_arity"] == 0 and fields["canonical_attempts"] == 0 and
                    fields["canonical_positive_forms"] == 0 and fields["support_powers"] == 0 and
                    fields["skipped"] == 0, "direct return differs")
        for key in totals:
            totals[key] += fields[key]
    require(seen == expected_cases, "case coverage differs")
    require(lines[21] == "REJECT nonpositive_enclosing_ball certification_attempts=1 "
            "certification_positive_forms=0 certification_powers=0 canonical_attempts=0 "
            "fallback_calls=1 fallback_powers=18 shell_only_excludes_fourth=1", "nonpositive rejection differs")
    require(lines[22] == "REJECT incomplete_positive_ball certification_attempts=1 "
            "certification_positive_forms=1 certification_powers=7 canonical_attempts=0", "incomplete rejection differs")
    require(lines[23].startswith("PASS "), "missing summary")
    summary = numeric_fields(lines[23].split()[1:])
    expected = {"cases": 21, "rejections": 2, "different_supports": 12, "different_arities": 6,
                "direct_returns": 9, "certification_powers": 171, "full_canonical_powers": 460,
                "support_canonical_powers": 123, "full_total_powers": 631, "support_total_powers": 294}
    require(summary == expected, "summary differs")
    require(totals == {"certification_powers": 171, "full_powers": 460, "support_powers": 123,
                       "different_support": 12, "different_arity": 6, "direct_return": 9},
            "case totals differ from summary")
    return summary


def check_capture(name: str, header_name: str, probe_name: str, pins: dict,
                  sources: dict[str, bytes]) -> tuple[dict, str]:
    expected_sources = {name: sha(content) for name, content in sources.items()}
    expected_sources["certified_support.hpp"] = sha((PACKET / header_name).read_bytes())
    expected_sources["probe.cpp"] = sha((PACKET / probe_name).read_bytes())
    capture = json.loads((PACKET / name).read_text())
    require(capture["source_pins"] == pins and
            capture["source_hashes_before"] == expected_sources and
            capture["source_hashes_after"] == expected_sources, "consumed source pins differ")
    require(capture["device_executed"] is False and capture["proposal_generation_executed"] is False and
            capture["gcp_used"] is False and capture["public_status"] == "not_claimed", "scope differs")
    commands = capture["commands"]
    require(len(commands) == 4, "expected exactly four commands")
    summary = {}
    outputs = []
    for index, (mode, flags) in enumerate(MODES.items()):
        compile_cell, run_cell = commands[index * 2:index * 2 + 2]
        expected_argv = ["g++", *STRICT, *flags, "-I", "morsehgp3D_v7", "probe.cpp", "-o", mode]
        require(compile_cell == {"mode": mode, "kind": "compile", "argv": expected_argv,
                                 "env": {}, "returncode": 0, "stdout": "", "stderr": ""},
                "compile cell differs")
        require(run_cell["mode"] == mode and run_cell["kind"] == "run" and
                run_cell["argv"] == ["./" + mode] and run_cell["returncode"] == 0 and
                run_cell["stderr"] == "" and run_cell["env"] == (SAN_ENV if mode == "SAN" else {}),
                "run cell differs")
        summary = check_stdout(run_cell["stdout"])
        outputs.append(run_cell["stdout"])
    require(outputs[0] == outputs[1], "O2 and SAN outputs differ")
    return summary, outputs[0]


def main() -> None:
    expected_files = {"README.md", "certified_support.hpp", "initial_certified_support.hpp",
                      "probe.cpp", "initial_probe.cpp", "source_pins.json", "commands.json", "initial_commands.json",
                      "reproduce.py", "verify.py"}
    sealed = {}
    for line in (PACKET / "SHA256SUMS").read_text().splitlines():
        digest, separator, name = line.partition("  ")
        require(separator == "  " and name in expected_files and name not in sealed,
                "malformed seal")
        require(sha((PACKET / name).read_bytes()) == digest, "receipt hash differs: " + name)
        sealed[name] = digest
    require(set(sealed) == expected_files, "receipt closure differs")
    pins, sources = read_sources()
    original_summary, original_stdout = check_capture(
        "initial_commands.json", "initial_certified_support.hpp", "initial_probe.cpp", pins, sources)
    summary, stdout = check_capture("commands.json", "certified_support.hpp", "probe.cpp", pins, sources)
    require(summary == original_summary and stdout == original_stdout,
            "initial and normalized-slot results differ")
    print(json.dumps({"status": "passed_certified_support_canonicalization", "commands": 8,
                      "source_files": len(sources), **summary, "device_executed": False,
                      "gcp_used": False, "public_status": "not_claimed"}, sort_keys=True))


if __name__ == "__main__":
    main()
