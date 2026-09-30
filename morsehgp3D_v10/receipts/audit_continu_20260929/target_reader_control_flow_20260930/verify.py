"""Hash-first portable reader of the two observed control-flow gaps; no engine/native calls."""
import datetime as dt
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parent
OLD = "/tmp/mhgp10-target-reader-audit-20260930.9ZbrpwSQ"
PYTHON = "/home/codespace/.python/current/bin/python3"
SOURCE = {
    "source/run_target.py": "41e92016a967180812549e5c3851b79baf510110404e5b333f600973193d50cf",
    "source/valide_lib.py": "94a96b1f0fcdd5032e13323aa065e44bb16f5e27abd397f5c1e16fcbebd2e6ad",
}
FILES = set(SOURCE) | {
    "README.md", "PROTOCOL.txt", "micro.py", "record.py", "verify.py", "receipt.json", "provenance.json",
    "preflight/micro.py", "preflight/receipt.json", "preflight/stdout", "preflight/stderr",
    "preflight2/micro.py", "preflight2/receipt.json",
}
FILES |= {f"logs/{m}.{s}" for m in ("normal", "optimized") for s in ("stdout", "stderr")}
FILES |= {f"preflight2/{m}.{s}" for m in ("normal", "optimized") for s in ("stdout", "stderr")}


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(name):
    return json.loads((ROOT / name).read_text())


def inventory():
    pins = {}
    for line in (ROOT / "SHA256SUMS").read_text().splitlines():
        found = re.fullmatch(r"([0-9a-f]{64})  ([A-Za-z0-9_./-]+)", line)
        require(found is not None, "manifest line")
        value, name = found.groups()
        require(name in FILES and name not in pins, "manifest inventory")
        pins[name] = value
    require(set(pins) == FILES, "manifest missing file")
    actual = {str(x.relative_to(ROOT)) for x in ROOT.rglob("*") if x.is_file()}
    require(actual == FILES | {"SHA256SUMS"}, "archive inventory")
    for name, value in pins.items():
        require(not (ROOT / name).is_symlink() and sha(ROOT / name) == value, "SHA " + name)
    return pins


def observation(out):
    require(out["status"] == "OBSERVED_CONTROL_FLOW_GAPS" and out["source_pins"] == SOURCE, "observation pins")
    for field in ("native_calls", "Gamma_calls", "sklearn_calls", "shared_source_mutations", "filesystem_outputs"):
        require(type(out[field]) is int and out[field] == 0, "zero scope " + field)
    require(out["scope"] == "AST-selected pinned functions and explicit unit stubs, not validation of geometric results",
            "stub scope")
    rows = out["source_change"]
    require([r["mode"] for r in rows] == ["stable", "changed", "test_failure"], "source cases")
    for row in rows:
        mode = row["mode"]
        code = 1 if mode == "test_failure" else 0
        require(type(row["returned_code"]) is int and row["returned_code"] == code
                and type(row["receipt_code"]) is int and row["receipt_code"] == code, "observed main code")
        require(row["sources_stables"] is (mode != "changed")
                and row["controls"] == 9 and row["snapshot_calls"] == 2
                and row["failures"] == (1 if mode == "test_failure" else 0), "source non-vacuity")
        require(row["before"] == {"frozen_unit": "0" * 64}
                and row["after"] == {"frozen_unit": ("1" if mode == "changed" else "0") * 64},
                "synthetic fingerprints")
    case = out["empty_variant"]
    require(case["base_empty_refused"] is True
            and case["stub_calls"] == {"Scene_stubs": 6, "compat_stubs": 5, "judge_stubs": 5}, "target controls")
    require([r["case"] for r in case["reports"]] == ["positive_nonempty", "negative_nonempty", "empty_target"],
            "target cases")
    for row in case["reports"]:
        positive = row["case"] != "negative_nonempty"
        empty = row["case"] == "empty_target"
        target_count = 0 if empty else 1
        require(row["variant_passed"] is positive and row["overall_passed"] is positive
                and row["variant_targets"] == target_count
                and row["normalized_variant_targets"] == target_count
                and row["judgments_including_base"] == 1 + target_count
                and len(row["variant_verdicts"]) == target_count, "vacuity and positive/negative controls")


def stamps(command, previous=-1):
    require(type(command["start_ns"]) is int and type(command["end_ns"]) is int
            and previous <= command["start_ns"] <= command["end_ns"], "monotonic stamps")
    start = dt.datetime.fromisoformat(command["start_utc"])
    end = dt.datetime.fromisoformat(command["end_utc"])
    require(start.utcoffset() == dt.timedelta(0) and end.utcoffset() == dt.timedelta(0) and start <= end,
            "UTC stamps")
    return command["end_ns"]


def main():
    pins = inventory()  # Before ANY receipt/probe read or subprocess replay.
    for name, value in SOURCE.items():
        require(pins[name] == value, "fixed original source identity")
    prov = load("provenance.json")
    require(prov["origin"] == "/workspaces/E-HGP/build/v10-verrou-points/fixtures_cibles/lib"
            and prov["shared_sources_modified"] is False and prov["regles_not_executed"] is True
            and prov["native_invocations"] == 0 and prov["GCP_used"] is False, "provenance scope")
    expected = {name.split("/")[-1]: value for name, value in SOURCE.items()}
    expected["regles.py"] = "517957e4e48d0f5b8f6e129955c2fc63b0d23613fa472a990d788b7e8d18bfd6"
    require([r["observed_utc"] for r in prov["observations"]] ==
            ["2026-09-30T14:08:18Z", "2026-09-30T14:14:35Z"]
            and all(r["hashes"] == expected for r in prov["observations"]), "source observation bracket")
    pre = load("preflight/receipt.json")
    require(pre["argv"] == ["python3", "-B", OLD + "/micro.py"] and type(pre["code"]) is int and pre["code"] == 1
            and pre["exact_utc_not_recorded"] is True and pre["stdout"] == ""
            and (ROOT / "preflight/stdout").read_bytes() == b""
            and "NameError: name 'ALIAS' is not defined" in (ROOT / "preflight/stderr").read_text(),
            "retained first harness failure")
    pre2 = load("preflight2/receipt.json")
    require([c["name"] for c in pre2["commands"]] == ["normal", "optimized"], "preflight2 inventory")
    for c in pre2["commands"]:
        opt = c["name"] == "optimized"
        require(c["argv"] == ["python3", "-B"] + (["-O"] if opt else []) + [OLD + "/micro.py"]
                and type(c["code"]) is int and c["code"] == 1 and c["stdout"] == ""
                and "AttributeError: 'list' object has no attribute 'items'" in c["stderr"],
                "retained second harness failure")
        for stream in ("stdout", "stderr"):
            require((ROOT / ("preflight2/" + c["name"] + "." + stream)).read_text() == c[stream],
                    "preflight2 log linkage")
    receipt = load("receipt.json")
    require(receipt["schema"] == "mhgp10.target_reader_control_flow.v1"
            and receipt["status"] == "CAPTURED" and receipt["native_invocations"] == 0
            and receipt["Gamma_invocations"] == 0 and receipt["GCP_used"] is False, "final receipt scope")
    expected_names = set(SOURCE) | {"micro.py", "record.py"}
    require(set(receipt["sources_before"]) == expected_names
            and receipt["sources_before"] == receipt["sources_after"]
            and all(pins[n] == v for n, v in receipt["sources_before"].items()), "capture source pins")
    require([c["name"] for c in receipt["commands"]] == ["normal", "optimized"], "final commands")
    previous = -1
    outputs = []
    for command in receipt["commands"]:
        opt = command["name"] == "optimized"
        require(command["argv"] == [PYTHON, "-B"] + (["-O"] if opt else []) + [OLD + "/micro.py"]
                and type(command["code"]) is int and command["code"] == 0 and command["stderr"] == "",
                "final historical command")
        previous = stamps(command, previous)
        for stream in ("stdout", "stderr"):
            require((ROOT / ("logs/" + command["name"] + "." + stream)).read_text() == command[stream],
                    "final log linkage")
        observation(json.loads(command["stdout"]))
        outputs.append(command["stdout"])
        replay = subprocess.run([sys.executable, "-B"] + (["-O"] if opt else []) + [str(ROOT / "micro.py")],
                                cwd=ROOT, text=True, capture_output=True, check=False, timeout=10)
        require(replay.returncode == 0 and replay.stderr == "" and replay.stdout == command["stdout"],
                "portable AST probe replay")
    require(len(set(outputs)) == 1 and inventory() == pins, "normal/O or closure mismatch")
    print(json.dumps({"status": "ARCHIVE_OBSERVED_CONTROL_FLOW_GAPS", "files": len(FILES),
                      "AST_stub_replays": 2, "native_replays": 0, "GCP_used": False}, sort_keys=True))


if __name__ == "__main__":
    main()
