#!/usr/bin/env python3
"""Exercise P0 receipt ingestion with real tiny probes and artificial failures."""

from __future__ import annotations

import argparse
import base64
import hashlib
import json
import os
import shutil
import signal
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "morsehgp3D_v8/bench/run_p0_matrix.py"
sys.path.insert(0, str(RUNNER.parent))
import run_p0_matrix as runner_module  # noqa: E402


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def invoke(probe: Path, output: Path, extra: list[str]) -> tuple[Any, Any, list[Any]]:
    flags = ["-B", "-O"] if sys.flags.optimize else ["-B"]
    command = [sys.executable, *flags, str(RUNNER), "--probe", str(probe),
               "--output", str(output), "--sizes", "8", "--families", "grid",
               "--repeats", "1", "--kmax", "10", "--s", "8", *extra]
    result = subprocess.run(command, capture_output=True, text=True, cwd=ROOT)
    require((output / "COMPLETION.json").is_file(), f"unclosed campaign: {command}")
    completion = json.loads((output / "COMPLETION.json").read_text())
    measurements = output / "MEASURES.jsonl"
    rows = ([json.loads(line) for line in measurements.read_text().splitlines()]
            if measurements.is_file() else [])
    require(completion["attempts"] == len(rows), "lost attempted row")
    require(completion["runs"] == sum(row["status"] == "completed" for row in rows),
            "invalid success accounting")
    for row in rows:
        require(base64.b64decode(row["stdout_base64"]).decode("utf-8", errors="replace") ==
                row["stdout"], "raw stdout was not preserved")
        require(base64.b64decode(row["stderr_base64"]).decode("utf-8", errors="replace") ==
                row["stderr"], "raw stderr was not preserved")
    return result, completion, rows


def spawn_signal_checks() -> int:
    """Force the child signal before Popen returns, without timing assumptions."""
    real_popen = subprocess.Popen
    previous = {signum: signal.signal(signum, runner_module.on_signal)
                for signum in (signal.SIGINT, signal.SIGTERM)}
    children: list[Any] = []
    checks = 0
    try:
        for new_session, signum in ((False, signal.SIGINT), (False, signal.SIGTERM),
                                    (True, signal.SIGINT), (True, signal.SIGTERM)):
            def delayed_popen(*args: Any, **kwargs: Any) -> Any:
                child = real_popen(*args, **kwargs)
                children.append(child)
                require(os.getpgid(child.pid) == (child.pid if new_session else os.getpgrp()),
                        "invoke did not preserve the requested process-group contract")
                # Waiting for this terminating child forces its already-flushed
                # signal into invoke's Popen window. It does not read its pipes.
                child.wait()
                return child

            subprocess.Popen = delayed_popen
            body = ("import os,signal,sys; "
                    "print('spawn stdout',flush=True); "
                    "print('spawn stderr',file=sys.stderr,flush=True); "
                    f"os.kill(os.getppid(),{int(signum)})")
            record: dict[str, Any] = {}
            try:
                runner_module.invoke([sys.executable, "-B", "-c", body], dict(os.environ), ROOT, record,
                                     new_session=new_session)
            except runner_module.CampaignInterrupted as error:
                require(error.signum == signum, "deferred spawn signal changed identity")
            else:
                raise RuntimeError("deferred spawn signal was not replayed")
            require(record.get("exit_code") == 0 and record.get("stdout") == "spawn stdout\n" and
                    record.get("stderr") == "spawn stderr\n" and
                    base64.b64decode(record.get("stdout_base64", "")) == b"spawn stdout\n" and
                    base64.b64decode(record.get("stderr_base64", "")) == b"spawn stderr\n",
                    "signal during Popen lost flushed child output")
            require(all(signal.getsignal(value) is runner_module.on_signal for value in previous),
                    "successful spawn interruption leaked deferred handlers")
            checks += 1

        # A separate exact-session child ignores TERM. Verify both group
        # cancellation signals, their exact target, and the final KILL status.
        def tracked_popen(*args: Any, **kwargs: Any) -> Any:
            child = real_popen(*args, **kwargs)
            children.append(child)
            return child

        subprocess.Popen = tracked_popen
        real_killpg = os.killpg
        group_signals: list[int] = []

        def checked_killpg(group: int, value: int) -> None:
            require(group == children[-1].pid and group != os.getpgrp(),
                    "group cancellation escaped its exact owned session")
            group_signals.append(value)
            real_killpg(group, value)

        os.killpg = checked_killpg
        try:
            body = ("import os,signal,sys\n"
                    "signal.signal(signal.SIGTERM,signal.SIG_IGN)\n"
                    "print('group stdout',flush=True)\n"
                    "print('group stderr',file=sys.stderr,flush=True)\n"
                    "os.kill(os.getppid(),signal.SIGINT)\n"
                    "while True: signal.pause()\n")
            record = {}
            try:
                runner_module.invoke([sys.executable, "-B", "-c", body], dict(os.environ), ROOT, record,
                                     new_session=True)
            except runner_module.CampaignInterrupted as error:
                require(error.signum == signal.SIGINT, "group cancellation changed the original signal")
            else:
                raise RuntimeError("group cancellation signal was swallowed")
            require(group_signals == [signal.SIGTERM, signal.SIGKILL] and
                    record.get("exit_code") == -signal.SIGKILL and
                    record.get("stdout") == "group stdout\n" and record.get("stderr") == "group stderr\n",
                    "group cancellation lost its escalation, return code or output")
            checks += 1
        finally:
            os.killpg = real_killpg

        def failing_popen(*_args: Any, **_kwargs: Any) -> Any:
            raise FileNotFoundError("forced Popen failure before a process is returned")

        subprocess.Popen = failing_popen
        try:
            runner_module.invoke(["nonexistent-fixture"], dict(os.environ), ROOT, {})
        except FileNotFoundError:
            pass
        else:
            raise RuntimeError("forced Popen failure was swallowed")
        require(all(signal.getsignal(value) is runner_module.on_signal for value in previous),
                "Popen failure leaked deferred handlers")
        checks += 1
    finally:
        subprocess.Popen = real_popen
        for signum, handler in previous.items():
            signal.signal(signum, handler)
        # Exact owned handles only; cleanup also works against the unfixed
        # implementation, so this regression test cannot orphan a fake child.
        for child in children:
            if child.poll() is None:
                child.kill()
            child.communicate()
    return checks


def read_signal_checks() -> int:
    """Signal exactly after an owned pipe read, before communicate saves it."""
    real_popen, real_read, real_killpg = subprocess.Popen, os.read, os.killpg
    children: list[Any] = []
    delivered: list[int] = []
    group_signals: list[int] = []

    def raising_handler(signum: int, _frame: Any) -> None:
        delivered.append(signum)
        raise runner_module.CampaignInterrupted(signum)

    previous = {signum: signal.signal(signum, raising_handler)
                for signum in (signal.SIGINT, signal.SIGTERM)}

    def tracked_popen(*args: Any, **kwargs: Any) -> Any:
        child = real_popen(*args, **kwargs)
        children.append(child)
        require(os.getpgid(child.pid) == child.pid and child.pid != os.getpgrp(),
                "read-signal child did not own its exact process group")
        return child

    def checked_killpg(group: int, signum: int) -> None:
        require(group == children[-1].pid and group != os.getpgrp(),
                "read-signal cleanup targeted an unowned process group")
        group_signals.append(signum)
        real_killpg(group, signum)

    def check_record(record: dict[str, Any], stdout: bytes, stderr: bytes, code: int) -> None:
        require(record.get("exit_code") == code and record.get("stdout") == stdout.decode() and
                record.get("stderr") == stderr.decode() and
                base64.b64decode(record.get("stdout_base64", "")) == stdout and
                base64.b64decode(record.get("stderr_base64", "")) == stderr,
                "read interruption lost output bytes, base64 or the final return code")

    checks = 0
    try:
        subprocess.Popen, os.killpg = tracked_popen, checked_killpg
        record: dict[str, Any] = {}
        runner_module.invoke([sys.executable, "-B", "-c",
                              "import sys; print('plain stdout'); print('plain stderr',file=sys.stderr)"],
                             dict(os.environ), ROOT, record, new_session=True)
        check_record(record, b"plain stdout\n", b"plain stderr\n", 0)
        require(not delivered and not group_signals, "uninterrupted capture cancelled its child")
        require(all(signal.getsignal(value) is raising_handler for value in previous),
                "uninterrupted capture did not restore handlers")
        checks += 1

        for stream in ("stdout", "stderr"):
            # Reading the targeted marker must prove BOTH initial writes
            # completed, regardless of child scheduling or the poll interval.
            output_order = ("stderr", "stdout") if stream == "stdout" else ("stdout", "stderr")

            def messages(prefix: str, indent: str = "") -> str:
                return "".join(f"{indent}print({(prefix + ' ' + channel)!r},file=sys.{channel},flush=True)\n"
                               for channel in output_order)

            body = ("import signal,sys\n"
                    "def term(_signum,_frame):\n" + messages("cancel", " ") +
                    "signal.signal(signal.SIGTERM,term)\n" + messages("read") +
                    "while True: signal.pause()\n")
            for signum in (signal.SIGINT, signal.SIGTERM):
                delivered.clear()
                group_signals.clear()
                injected: list[int] = []
                second_signal = signal.SIGTERM if signum == signal.SIGINT else signal.SIGINT
                # Two cases isolate the original byte-loss window; two
                # additionally interrupt cancellation with the other signal.
                during_cancel = (stream == "stdout") == (signum == signal.SIGTERM)

                def read_then_signal(fd: int, count: int) -> bytes:
                    data = real_read(fd, count)
                    pipe = getattr(children[-1], stream) if children else None
                    if pipe is not None and not pipe.closed and fd == pipe.fileno():
                        if not injected and ("read " + stream + "\n").encode() in data:
                            injected.append(signum)
                            os.kill(os.getpid(), signum)
                        elif during_cancel and len(injected) == 1 and ("cancel " + stream + "\n").encode() in data:
                            require(group_signals == [signal.SIGTERM],
                                    "second read signal did not occur during TERM cancellation")
                            injected.append(second_signal)
                            os.kill(os.getpid(), second_signal)
                    return data

                os.read = read_then_signal
                record = {}
                try:
                    runner_module.invoke([sys.executable, "-B", "-c", body], dict(os.environ), ROOT,
                                         record, new_session=True)
                except runner_module.CampaignInterrupted as error:
                    require(error.signum == signum, "cleanup replaced the first interruption")
                else:
                    raise RuntimeError("read interruption was not replayed")
                finally:
                    os.read = real_read
                expected_signals = [signum, second_signal] if during_cancel else [signum]
                require(injected == delivered == expected_signals,
                        "read/cancellation signals were not injected and replayed exactly once")
                require(group_signals == [signal.SIGTERM, signal.SIGKILL] and children[-1].poll() == -signal.SIGKILL,
                        "read-signal cancellation lost escalation or left an owned child alive")
                check_record(record, b"read stdout\ncancel stdout\n", b"read stderr\ncancel stderr\n",
                             -signal.SIGKILL)
                require(all(signal.getsignal(value) is raising_handler for value in previous),
                        "read interruption leaked deferred handlers")
                checks += 1

        delivered.clear()
        group_signals.clear()

        def returning_handler(signum: int, _frame: Any) -> None:
            delivered.append(signum)

        signal.signal(signal.SIGINT, returning_handler)
        record = {}
        runner_module.invoke([sys.executable, "-B", "-c",
                              "import os,signal; print('return stdout',flush=True); os.kill(os.getppid(),signal.SIGINT)"],
                             dict(os.environ), ROOT, record, new_session=True)
        check_record(record, b"return stdout\n", b"", 0)
        require(delivered == [signal.SIGINT] and not group_signals and
                signal.getsignal(signal.SIGINT) is returning_handler and
                signal.getsignal(signal.SIGTERM) is raising_handler,
                "non-raising handler was changed, lost or turned into cancellation")
        checks += 1
    finally:
        subprocess.Popen, os.read, os.killpg = real_popen, real_read, real_killpg
        for signum, handler in previous.items():
            signal.signal(signum, handler)
        for child in children:
            if child.poll() is None:
                child.kill()
            child.communicate()
    return checks


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--selftest", required=True, action="store_true")
    args = parser.parse_args()
    source_probe = args.probe.resolve()
    original_hash = digest(source_probe)
    runner_hash = digest(RUNNER)
    checks = 0
    spawn_checks = spawn_signal_checks()
    require(spawn_checks == 6, "spawn-signal regression floor")
    read_checks = read_signal_checks()
    require(read_checks == 6, "read-signal regression floor")
    with tempfile.TemporaryDirectory(prefix="mhgp8_campaign_gate_") as temporary_name:
        temporary = Path(temporary_name)
        actual = temporary / "actual_probe"
        shutil.copy2(source_probe, actual)
        shutil.copy2(source_probe.parent / "CMakeCache.txt", temporary / "CMakeCache.txt")
        captured = subprocess.run([str(actual), "8", "pool", "2", "grid", "10", "8"],
                                  capture_output=True, check=True)
        require(not captured.stderr, "genuine fixture emitted stderr")
        (temporary / "captured.json").write_bytes(captured.stdout)
        result, completion, rows = invoke(actual, temporary / "genuine", [])
        require(result.returncode == 0 and completion["status"] == "completed" and
                len(rows) == completion["runs"] == 9, "real nine-case matrix failed")
        require(completion["source_hashes_unchanged"] is True and
                completion["probe_hash_unchanged"] is True, "positive closing pins failed")
        manifest = json.loads((temporary / "genuine/MANIFEST.json").read_text())
        require(manifest["receipt_validation_version"] == 2 and
                manifest["source_sha256"] == completion["source_sha256_closing"] and
                manifest["probe_sha256"] == completion["probe_sha256_closing"],
                "closing pins not recorded")
        require(all(row["probe_sha256_before"] == row["probe_sha256_after"] == original_hash
                    for row in rows), "per-invocation binary pins missing")
        checks += 1

        # A compiler selected with -DCMAKE_CXX_COMPILER=clang++ is commonly
        # recorded as UNINITIALIZED, not FILEPATH. Metadata ingestion must
        # not be tied to the cache type chosen by CMake's invocation.
        cache_path = temporary / "CMakeCache.txt"
        cache = cache_path.read_text()
        alternate = "\n".join(
            "CMAKE_CXX_COMPILER:UNINITIALIZED=" + line.split("=", 1)[1]
            if line.startswith("CMAKE_CXX_COMPILER:") else line
            for line in cache.splitlines()) + "\n"
        cache_path.write_text(alternate)
        result, completion, rows = invoke(actual, temporary / "alternate_cache_type",
                                           ["--strategies", "pool", "--lanes", "2"])
        require(result.returncode == 0 and completion["status"] == "completed" and
                len(rows) == 1, "non-FILEPATH compiler cache was rejected")
        cache_path.write_text(cache)
        checks += 1

        prelude = (f"#!{sys.executable}\n"
                   "import json, os, signal, subprocess, sys\n"
                   "from pathlib import Path\n"
                   f"real = {str(actual)!r}\n")
        passthrough = (
            "result = subprocess.run([real, *sys.argv[1:]], capture_output=True, check=True)\n"
            "row = json.loads(result.stdout)\n")
        mutants = {
            "wrong_n": "row['n'] += 1\n",
            "wrong_strategy": "row['strategy'] = 'dual'\n",
            "wrong_lane": "row['lane'] = 3\n",
            "wrong_family": "row['family'] = 'sheet'\n",
            "wrong_kmax": "row['kmax'] = 5\n",
            "wrong_s": "row['separation_s'] = 12\n",
            "wrong_schema": "row['schema'] = 'another_schema'\n",
            "wrong_status": "row['status'] = 'failed'\n",
            "wrong_scope": "row['scope'] = 'FULL'\n",
            "false_claim": "row['public_status'] = 'exact'\n",
            "false_downstream": "row['downstream_measured'] = True\n",
            "boolean_integer": "row['threads'] = True\n",
            "wrong_core": "row['core_credit'] = 1\n",
            "wrong_threshold": "row['threshold'] += 1\n",
            "wrong_factor": "row['n_a'] += 1\n",
            "wrong_count": "row['candidate_pairs'] += 1\n",
            "wrong_fraction": "row['rejected_fraction'] = 0.125\n",
            "wrong_descriptors": "row['candidate_descriptors'] = 10000\n",
            "negative_time": "row['plan_ms'] = -1\n",
            "wrong_time_sum": "row['total_component_ms'] += 1\n",
            "nonfinite_time": "row['plan_ms'] = float('nan')\n",
            "overflow_time": "row['plan_ms'] = float('inf')\n",
            "negative_work": "row['plan_work']['pool_selection_tests'] = -1\n",
            "overflow_work": "row['plan_work']['pool_selection_tests'] = 2**64\n",
            "wrong_validation": "row['preparation_work']['validation_points'] -= 1\n",
            "wrong_strategy_work": "row['plan_work']['tube_records'] = 1\n",
            "wrong_identity": "row['input_fnv1a64_le_u16_xyz'] = 'not_hex'\n",
        }
        for name, mutation in mutants.items():
            fake = temporary / name
            fake.write_text(prelude + passthrough + mutation + "print(json.dumps(row))\n")
            fake.chmod(0o755)
            result, completion, rows = invoke(
                fake, temporary / f"receipt_{name}", ["--strategies", "pool", "--lanes", "2"])
            require(result.returncode == 1 and completion["status"] == "invalid" and
                    completion["runs"] == 0 and len(rows) == 1 and
                    rows[0]["status"] == "invalid" and bool(rows[0]["stdout"]),
                    f"mutant accepted or evidence lost: {name}: {result.stderr}")
            checks += 1

        failures = {
            "replay": (
                "sys.stdout.buffer.write(Path(__file__).with_name('captured.json').read_bytes())\n",
                [], "invalid", 1, 2),
            "malformed": ("sys.stdout.write('{\"schema\":')\n",
                          [], "invalid", 0, 1),
            "bad_utf8": ("sys.stdout.buffer.write(b'\\xff\\x80')\n",
                         [], "invalid", 0, 1),
            "duplicate_json_key": (
                "data = Path(__file__).with_name('captured.json').read_text().strip()\n"
                "sys.stdout.write(data[:-1] + ',\"n\":8}')\n",
                [], "invalid", 0, 1),
            "overflow_json_exponent": (
                "data = Path(__file__).with_name('captured.json').read_text().strip()\n"
                "sys.stdout.write(data[:-1] + ',\"overflow\":1e999}')\n",
                [], "invalid", 0, 1),
            "process_failure": (
                "print('partial stdout'); print('failure marker', file=sys.stderr); sys.exit(3)\n",
                [], "failed", 0, 1),
            "binary_changes_last_run": (
                passthrough + "print(json.dumps(row))\n"
                "with Path(__file__).open('a') as stream: stream.write('# changed\\n')\n",
                ["--strategies", "pool", "--lanes", "2"], "invalid", 0, 1),
            "changed_point_identities": (
                passthrough + "if sys.argv[2] == 'dual': row['input_fnv1a64_le_u16_xyz'] = '1'\n"
                "print(json.dumps(row))\n", [], "invalid", 1, 2),
            "changed_repeat_work": (
                passthrough + "state = Path(__file__).with_suffix('.state')\n"
                "if state.exists(): row['preparation_work']['uniqueness_comparisons'] += 1\n"
                "state.write_text('visited')\nprint(json.dumps(row))\n",
                ["--strategies", "pool", "--lanes", "2", "--repeats", "2"],
                "invalid", 1, 2),
            "interrupted": (
                "print('interrupted stdout', flush=True)\n"
                "print('interrupted stderr', file=sys.stderr, flush=True)\n"
                "os.kill(os.getppid(), signal.SIGINT)\n",
                [], "interrupted", 0, 1),
            "interrupted_ignores_term": (
                "signal.signal(signal.SIGTERM, signal.SIG_IGN)\n"
                "print('interrupted stdout', flush=True)\n"
                "os.kill(os.getppid(), signal.SIGINT)\n"
                "while True: signal.pause()\n",
                [], "interrupted", 0, 1),
            "terminated": (
                "print('terminated stdout', flush=True)\n"
                "os.kill(os.getppid(), signal.SIGTERM)\n",
                [], "interrupted", 0, 1),
        }
        for name, (body, extra, status, successes, attempts) in failures.items():
            fake = temporary / name
            fake.write_text(prelude + body)
            fake.chmod(0o755)
            result, completion, rows = invoke(fake, temporary / f"receipt_{name}", extra)
            expected_exit = (143 if name == "terminated" else
                             130 if status == "interrupted" else 1)
            require(result.returncode == expected_exit and completion["status"] == status and
                    completion["runs"] == successes and len(rows) == attempts,
                    f"wrong failure closure: {name}: {completion}: {result.stderr}")
            require(rows[-1]["status"] == status and bool(rows[-1]["stdout_base64"]),
                    f"failed attempt stdout lost: {name}")
            if name == "binary_changes_last_run":
                require(completion["probe_hash_unchanged"] is False and
                        rows[-1]["probe_sha256_before"] != rows[-1]["probe_sha256_after"],
                        "last-invocation mutation was not detected")
            if name in ("process_failure", "interrupted"):
                require(bool(rows[-1]["stderr_base64"]), f"stderr lost: {name}")
            if name == "interrupted_ignores_term":
                require(rows[-1]["exit_code"] == -9, "cancelled process was not killed")
            checks += 1
        require(checks == 41, "non-vacuity floor")
        require(digest(actual) == digest(source_probe) == original_hash and
                digest(RUNNER) == runner_hash, "gate changed its genuine inputs")
    print(json.dumps({"status": "passed", "checks": checks,
                      "spawn_signal_checks": spawn_checks,
                      "read_signal_checks": read_checks,
                      "python_optimized": bool(sys.flags.optimize),
                      "scope": "p0_campaign_receipt_ingestion_not_geometry"}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
