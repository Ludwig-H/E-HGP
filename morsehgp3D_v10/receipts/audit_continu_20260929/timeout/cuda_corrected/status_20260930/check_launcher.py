"""Audit-only envelope tests and control-flow simulations. No CUDA process.

Requires a frozen cuda_probe.py and a NEW output directory. Simulated paths
do not certify an actual GPU launch, timeout, signal, or compile failure.
"""
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
from unittest.mock import Mock, mock_open, patch
import hashlib
import importlib.util
import json
import subprocess
import sys


def require(value, message):
    if not value:
        raise RuntimeError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    require(len(sys.argv) == 3, "usage: check_launcher.py FROZEN_READER NEW_OUTPUT")
    source, dest = (Path(p).resolve() for p in sys.argv[1:])
    require(not dest.exists(), "do not overwrite a closed capture")
    script = Path(__file__).resolve()
    before = {str(p): digest(p) for p in (source, script)}
    spec = importlib.util.spec_from_file_location("frozen_cuda_reader", source)
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    cases = [
        ("status_only", 0, '{"status":"ok"}', 0),
        ("timing_false", 0, '{"status":"ok","timing_ok":false}', 0),
        ("mismatch_positive", 0, '{"status":"ok","i128_mismatches":1}', 0),
        ("nan_rate", 0, '{"status":"ok","rate":NaN}', 0),
        ("negative_rate", 0, '{"status":"ok","rate":-1}', 0),
        ("duplicate_status", 0, '{"status":"cuda_error","status":"ok"}', 0),
        ("timing_invalid_with_rate", 5, '{"status":"timing_invalid","rate":1000}', 5),
        ("wrong_status", 0, '{"status":"cuda_error"}', 8),
        ("wrong_code", 3, '{"status":"ok"}', 8),
        ("two_lines", 0, '{"status":"ok"}\n{"status":"ok"}', 8),
    ]
    results = []
    for name, code, raw, expected in cases:
        got = reader.check_output(code, raw)
        require(got == expected, "reader behaviour changed: " + name)
        results.append({"case": name, "process_code": code, "raw_stdout": raw,
                        "reader_code": got, "expected_envelope_code": expected})

    simulations = []
    for name, expected in (("no_nvcc", 4), ("compile_failure", 6), ("timeout", 7), ("signal", 7)):
        compile_result = subprocess.CompletedProcess(["SIMULATED_NVCC"],
                                                    1 if name == "compile_failure" else 0,
                                                    "SIMULATED_COMPILE_STDOUT", "SIMULATED_COMPILE_STDERR")
        process_result = subprocess.CompletedProcess(["SIMULATED_PROBE"], -15,
                                                    "SIMULATED_SIGNAL_PARTIAL_STDOUT", "SIMULATED_SIGNAL_STDERR")
        timeout = subprocess.TimeoutExpired(["SIMULATED_PROBE"], 600,
                                            output="SIMULATED_TIMEOUT_PARTIAL_STDOUT",
                                            stderr="SIMULATED_TIMEOUT_PARTIAL_STDERR")
        mocked_run = Mock(side_effect=[compile_result, timeout if name == "timeout" else process_result])
        opened = mock_open()
        stdout = StringIO()
        with patch.object(sys, "argv", [str(source), "--out", "SIMULATED_OUT", "--work", "SIMULATED_WORK"]), \
                patch.object(reader.os, "makedirs"), \
                patch.object(reader.os.path, "exists", return_value=name != "no_nvcc"), \
                patch.object(reader.glob, "glob", return_value=[]), \
                patch.object(reader.subprocess, "run", mocked_run), \
                patch("builtins.open", opened), redirect_stdout(stdout):
            got = reader.main()
        require(got == expected, "wrong simulated launcher exit: " + name)
        opened_paths = [str(call.args[0]) for call in opened.call_args_list]
        json_written = any(p.endswith("cuda_probe.json") for p in opened_paths)
        require(json_written == (name == "signal"), "unexpected JSON write")
        emitted = stdout.getvalue()
        require(name != "timeout" or "SIMULATED_TIMEOUT_PARTIAL" not in emitted,
                "timeout partial output is no longer lost")
        simulations.append({"case": name, "launcher_code": got, "opened_paths": opened_paths,
                            "cuda_json_opened_for_write": json_written,
                            "printed_stdout": emitted,
                            "subprocess_calls_simulated": mocked_run.call_count,
                            "actual_subprocesses_launched": 0})
    after = {str(p): digest(p) for p in (source, script)}
    require(before == after, "source changed during test")
    dest.mkdir(parents=True)
    receipt = {"status": "ENVELOPE_AND_FAILURE_PATH_LIMITS_REPRODUCED",
               "date": "2026-09-30", "reader_calls": len(results), "simulations": simulations,
               "cases": results, "hashes_before": before, "hashes_after": after,
               "actual_CUDA_processes": 0, "actual_timeout_or_signal_test": False,
               "GPU_qualification": False, "engine_modified": False, "GCP_used": False}
    (dest / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"status": receipt["status"], "reader_calls": len(results),
                      "control_flow_simulations": len(simulations), "actual_CUDA_processes": 0}, sort_keys=True))


if __name__ == "__main__":
    main()
