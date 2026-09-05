#!/usr/bin/env python3
"""Tiny CLI domain checks on the already qualified E binary; no build."""
import datetime
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
BINARY = ROOT / "build/v7_next_q2_qualification/mhgp7"
EXPECTED_BINARY = "df75153326f7bbf4ce0a412031a365205559cb68155d4304adc9301461f505f6"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_checks() -> dict:
    require(digest(BINARY) == EXPECTED_BINARY, "unqualified E binary")
    pins_file = HERE.parents[1] / "receipts_front_compiled_20260905/qualification/e_tests_snapshot/full/inputs_before.json"
    pins = json.loads(pins_file.read_text())["sources"]
    wanted = {
        "cli/mhgp7.cpp", "src/pipeline/run.hpp", "src/pipeline/expand.hpp",
        "src/forest/plateau.hpp", "src/forest/fold.hpp",
        "src/forest/silent_incidence.hpp", "src/io/archive.hpp",
    }
    source_pins = {
        entry["path"]: entry["sha256"] for entry in pins
        if entry["path"].removeprefix("morsehgp3D_v7/") in wanted
    }
    require(len(source_pins) == len(wanted), "source pin nonvacuum")
    for path, expected in source_pins.items():
        require(digest(ROOT / path) == expected, "source drift: " + path)

    two = "7 0 0 0\n42 6 0 0\n"
    acute = two + "99 2 3 0\n"
    zero_caps = [f"--silent-{cap}=0" for cap in (
        "core-records", "chain-steps", "cofaces", "query-nodes", "meb-supports"
    )]
    cases = [
        ("two_default", two, [], 0, 1),
        ("two_zero_unused_caps", two, zero_caps, 0, 1),
        ("two_smax2", two, ["--smax=2"], 0, 1),
        ("acute_three", acute, [], 0, 2),
        ("acute_three_prefix", acute, ["--smax=2"], 0, 1),
        ("collinear_three", "7 0 0 0\n42 2 0 0\n99 5 0 0\n", [], 0, 2),
        ("empty", "", [], 2, 0),
        ("singleton", "7 0 0 0\n", [], 2, 0),
        ("duplicate_id", "7 0 0 0\n7 6 0 0\n", [], 2, 0),
        ("duplicate_position", "7 0 0 0\n42 0 0 0\n", [], 2, 0),
        ("coordinate_high", "7 0 0 0\n42 65536 0 0\n", [], 2, 0),
        ("coordinate_negative", "7 0 0 0\n42 -1 0 0\n", [], 2, 0),
        ("smax_low", two, ["--smax=1"], 2, 0),
        ("smax_high", two, ["--smax=12"], 2, 0),
        ("s_low", two, ["--s=7"], 2, 0),
        ("square_extra_shell", "0 0 0 0\n1 8 0 0\n2 8 8 0\n3 0 8 0\n", [], 2, 0),
        ("require_exact", two, ["--require-exact"], 2, 0),
        ("bad_cap", acute, ["--silent-chain-steps=-1"], 2, 0),
    ]
    observations = []
    require(len(cases) == len({case[0] for case in cases}) == 18,
            "domain case nonvacuum and uniqueness")
    for name, fixture, flags, expected_code, expected_kmax in cases:
        input_path = HERE / (name + ".input.txt")
        input_path.write_text(fixture)
        with tempfile.TemporaryDirectory(prefix=".archive_", dir=HERE) as work:
            output_path = Path(work) / "result"
            command = [str(BINARY), f"--input={input_path}",
                       f"--output={output_path}", "--threads=1",
                       "--fold-join=1", "--layout=csr", "--digest",
                       "--complete-incidences"] + flags
            started = datetime.datetime.now(datetime.timezone.utc).isoformat()
            result = subprocess.run(command, capture_output=True, timeout=10)
            (HERE / (name + ".stdout")).write_bytes(result.stdout)
            (HERE / (name + ".stderr")).write_bytes(result.stderr)
            require(result.returncode == expected_code, f"exitcode {name}: {result.returncode}")
            manifest = None
            if expected_code == 0:
                require(not result.stderr, "successful stderr: " + name)
                text = result.stdout.decode()
                require("forest_semantics=normalized_horizontal_h0_candidate public_status=not_claimed require_exact=false\n" in text, "CLI semantic type")
                require("authority=status_terminal callbacks=provisional vertical_maps=none\n" in text, "CLI authority type")
                cards = re.findall(r"^cardinalites K=(\d+) (.*)$", text, re.M)
                require([int(k) for k, _ in cards] == list(range(1, expected_kmax + 1)), "all effective orders")
                manifest = json.loads((output_path / "manifest.json").read_text())
                require(manifest["status"] == "completed" and manifest["kmax"] == expected_kmax, "archive terminal scope")
                require(manifest["public_status"] == "not_claimed" and manifest["require_exact"] is False and manifest["vertical_maps"] == "none", "archive public type")
                require(manifest["forest_semantics"] == "normalized_horizontal_h0_candidate", "archive semantic type")
                require(len(manifest["files"]) == expected_kmax + 1, "input and all forests")
                for entry in manifest["files"]:
                    file = output_path / entry["name"]
                    require(file.stat().st_size == entry["bytes"] and digest(file) == entry["sha256"], "archive file pin")
                    (HERE / (name + "." + entry["name"])).write_bytes(file.read_bytes())
                (HERE / (name + ".manifest.json")).write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
            else:
                require(not result.stdout and b"REFUS" in result.stderr, "refusal authority: " + name)
                require(not output_path.exists() and not list(Path(work).glob(".mhgp7-provisional-*")), "refused archive cleanup")
            observations.append({"case": name, "command": command,
                                 "started_utc": started,
                                 "exitcode": result.returncode,
                                 "expected_kmax": expected_kmax,
                                 "input_sha256": digest(input_path),
                                 "stdout_sha256": hashlib.sha256(result.stdout).hexdigest(),
                                 "stderr_sha256": hashlib.sha256(result.stderr).hexdigest(),
                                 "manifest": manifest})
    require((HERE / "acute_three.forest_K1.bin").read_bytes() ==
            (HERE / "acute_three_prefix.forest_K1.bin").read_bytes(),
            "completed K1 prefix archive byte equality on acute triangle")
    successes = sum(item["exitcode"] == 0 for item in observations)
    rejections = sum(item["exitcode"] == 2 for item in observations)
    require(successes == 6 and rejections == 12, "result nonvacuum")
    require(digest(BINARY) == EXPECTED_BINARY, "binary changed during checks")
    for path, expected in source_pins.items():
        require(digest(ROOT / path) == expected, "source changed during checks")
    return {"schema": "mhgp7-horizontal-cli-domain-v1", "status": "passed",
            "phase": "exploration_v7_hors_registre", "backend": "cpu_reference",
            "profile": "quantized_u16_input_only",
            "mode": "audit_independant_math_and_architecture",
            "public_status": "not_claimed", "gcp": "not_used",
            "script_sha256": digest(Path(__file__)),
            "qualified_E_binary_sha256": EXPECTED_BINARY,
            "qualified_sources": source_pins, "cases": observations,
            "successes": successes, "rejections": rejections,
            "completed_prefix_byte_equality_fixture": "acute_three_K1",
            "limits": ["Tiny CLI domain fixtures, not a benchmark or full suite.",
                       "Archive fields and file hashes checked; semantic delta replay remains attributed to closed E archive gates.",
                       "No private pipeline callback observer used by this CLI guard."]}


if __name__ == "__main__":
    try:
        print(json.dumps(run_checks(), indent=2, sort_keys=True))
    except (OSError, ValueError, KeyError, subprocess.TimeoutExpired) as error:
        print(json.dumps({"status": "failed", "error": str(error)}))
        sys.exit(1)
