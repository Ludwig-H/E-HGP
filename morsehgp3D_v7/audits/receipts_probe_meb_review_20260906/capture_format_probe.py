#!/usr/bin/env python3
"""Reproduce metadata incompatibilities on one closed capture, without engines."""

from __future__ import annotations

import ast
import hashlib
import json
from pathlib import Path
import types

BASE = Path(__file__).resolve().parent
STEM = "n8_s8_k5_lazy_c0_p0"
COMPARATOR = "be4b8712f55324af025d1ed69a2aa3748de45a8f2ba73519bf575243226fd7c6"
CONTROLLER = "ee9d4640452c81bc4bd4d872630da2a334cf49da078c84dd74c82c8b4c20058d"


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise RuntimeError(reason)


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def encode(value) -> bytes:
    return (json.dumps(value, sort_keys=True, indent=2) + "\n").encode()


class PrimaryCheckpoint(Exception):
    pass


class StopBeforeJudgment:
    def judge(self, *unused):
        raise PrimaryCheckpoint("primary_judge_reached_not_executed")


class MemoryReader:
    """Private format adapters change only in-memory bytes, never captures."""
    def __init__(self, values):
        self.values = values

    def read(self, path, pin=None):
        raw = self.values[path]
        require(pin is None or sha(raw) == pin, "memory overlay pin")
        return raw

    def argument(self, argument):
        written, pin = argument.rsplit("=", 1)
        path = Path(written)
        return path, self.read(path, pin)


def main() -> None:
    manifest = json.loads((BASE / "captured_inputs.json").read_text())
    for item in manifest["files"]:
        raw = (BASE / item["copy"]).read_bytes()
        require(sha(raw) == item["sha256"] and len(raw) == item["bytes"], "immutable input pin")
    compare_raw = (BASE / "inputs/compare.py").read_bytes()
    controller_raw = (BASE / "inputs/controller.py").read_bytes()
    require(sha(compare_raw) == COMPARATOR and sha(controller_raw) == CONTROLLER, "reviewed source pair")
    tree = ast.parse(controller_raw)
    probe = next(node for node in ast.walk(tree) if isinstance(node, ast.FunctionDef) and node.name == "probe")
    calls = [node for node in ast.walk(probe) if isinstance(node, ast.Call)
             and isinstance(node.func, ast.Attribute) and node.func.attr == "command"]
    require(len(calls) == 1 and isinstance(calls[0].func.value, ast.Name)
            and calls[0].func.value.id == "self"
            and any(k.arg == "merged" and isinstance(k.value, ast.Constant) and k.value.value is True
                    for k in calls[0].keywords), "actual probe path uses current merged command")
    module = types.ModuleType("captured_pair_comparator")
    module.__file__ = str(BASE / "inputs/compare.py")
    exec(compile(compare_raw, module.__file__, "exec"), module.__dict__)
    attempt = BASE / "inputs/attempt"
    values = {path: path.read_bytes() for path in attempt.iterdir() if path.is_file()}
    get = lambda suffix: json.loads(values[attempt / (STEM + suffix)])
    source, after, receipt, command, intent = (get(x) for x in (
        ".sources_before.json", ".sources_after.json", ".receipt.json", ".command.json", ".intent.json"))
    protocol = json.loads(values[attempt / "protocol.json"])
    require(source == after and source["source_map_sha256"] == sha(encode(source["files"])), "real source map closes")
    require(set(source) == {"files", "source_map_sha256", "binary", "binary_sha256"}, "actual snapshot inventory")
    require(receipt["status"] == "completed" and receipt["error"] is None and receipt["exit_code"] == 0
            and receipt["process_group_closed"] is True, "successful closed capture before comparison")
    require(command == {k: v for k, v in receipt.items() if k not in {"orders", "terminal"}}, "command/receipt identity")
    require(all(command[k] == v for k, v in intent.items()), "intent retained exactly")
    require(set(receipt["streams"]) == {STEM + ".raw.txt", STEM + ".stderr"}, "actual two-stream inventory")
    for name, entry in receipt["streams"].items():
        raw = values[attempt / name]
        require(entry == {"sha256": sha(raw), "bytes": len(raw)}, "real stream hashes")
    require(values[attempt / (STEM + ".stderr")] == b"", "merged capture has an explicitly sealed empty stderr")
    verdict = get(".verdict.json")
    primary, supplement = get(".judge_normal.stdout"), get(".first_c_normal.stdout")
    require(verdict["status"] == "completed" and verdict["capture_valid"] is True
            and verdict["attempt_success"] is True and primary["audit_status"] == "valid"
            and supplement["supplement_status"] == "valid", "captured primary admissions are positive")

    expected = [(0, "KeyError", "source_sha256"), (1, "KeyError", "probe_schema"),
                (2, "KeyError", "successor_accounting"), (3, "ValueError", "stream_inventory"),
                (4, "ValueError", "intent_inventory"),
                (5, "PrimaryCheckpoint", "primary_judge_reached_not_executed")]
    results = []
    extra = {"sanitizer_virtual_address_reservation", "cpu_limit_seconds", "file_size_limit_bytes"}
    for stage, kind, reason in expected:
        data = dict(values)
        s, rec, cmd, start = (json.loads(encode(x)) for x in (source, receipt, command, intent))
        adapters = []
        if stage >= 1:
            s["source_sha256"] = s["source_map_sha256"]
            adapters.append("supply comparator source_sha256 from actual source_map_sha256")
        if stage >= 2:
            s["probe_schema"] = protocol["probe_schema"]
            adapters.append("supply probe_schema from actual protocol")
        if stage >= 3:
            s["successor_accounting"] = protocol["successor_accounting"]
            adapters.append("supply successor_accounting from actual protocol")
        if stage >= 4:
            del rec["streams"][STEM + ".stderr"]
            del cmd["streams"][STEM + ".stderr"]
            adapters.append("remove the already-checked empty stderr only in memory")
        if stage >= 5:
            require(extra <= start.keys(), "three actual intent extensions")
            start = {k: v for k, v in start.items() if k not in extra}
            adapters.append("remove the three current intent guards only in memory")
        for suffix, value in ((".sources_before.json", s), (".sources_after.json", s),
                              (".receipt.json", rec), (".command.json", cmd), (".intent.json", start)):
            data[attempt / (STEM + suffix)] = encode(value)
        reader = MemoryReader(data)
        args = [str(path) + "=" + sha(data[path]) for path in (
            attempt / (STEM + ".receipt.json"), attempt / "protocol.json", attempt / (STEM + ".sources_before.json"))]
        try:
            module.load_arm(reader, *args, StopBeforeJudgment(), StopBeforeJudgment())
        except (KeyError, ValueError, PrimaryCheckpoint) as error:
            actual_reason = error.args[0] if isinstance(error, KeyError) else str(error)
            require(type(error).__name__ == kind and actual_reason == reason, "expected first boundary")
            results.append({"stage": stage, "exception": kind, "reason": actual_reason,
                            "prior_obstacles_adapted_only_in_memory": adapters})
        else:
            raise RuntimeError("unexpected full admission without reaching pinned primary judge")
    print(json.dumps({"status": "demonstrated", "kind": "five_capture_format_incompatibilities",
        "comparator_sha256": COMPARATOR, "controller_sha256": CONTROLLER,
        "capture": STEM, "capture_closed_utc": receipt["ended"],
        "capture_verdict_closed_utc": verdict["ended"], "original_capture_admitted_by_its_two_readers": True,
        "results": results, "scope": "transport-format counterfixture only, no scientific refusal or forged success",
        "source_map_sha256": source["source_map_sha256"], "intent_additional_fields": sorted(extra),
        "fix_direction": "consume declared controller snapshot/stream/intent formats and validate guard values explicitly",
        "no_ELF_read": True, "no_engine": True, "no_primary_judge_executed": True,
        "script_sha256": sha(Path(__file__).read_bytes()), "public_status": "not_claimed", "gcp": "not_used"},
        indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
