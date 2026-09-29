"""Small owned-process reproduction of scale_run.run_json's descendant timeout.

Only writes to a fresh /tmp directory; no cloud calls, geometry, or engine edits.
Linux subreaper lets this process explicitly reap the controlled orphan.
"""
import ctypes
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time

SOURCE = Path(__file__).resolve().parents[3] / "bench/scaling/scale_run.py"


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def child(marker):
    Path(marker).write_text(json.dumps({"pid": os.getpid(), "pgid": os.getpgrp()}))
    fd = os.open(os.devnull, os.O_RDWR)
    for target in (0, 1, 2):
        os.dup2(fd, target)
    os.close(fd)
    time.sleep(10)


def worker(root):
    spec = importlib.util.spec_from_file_location("audited_scale_run", SOURCE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.TIMEOUT = 1.0
    # run_json places its timing file beside cmd[0], so use an owned symlink.
    command = [str(root / "python_owned"), "-B", str(Path(__file__).resolve()), "--child", str(root / "child.json")]
    started = time.monotonic()
    result = module.run_json(command)
    print(json.dumps({"result": result, "elapsed_seconds": time.monotonic() - started}), flush=True)


def reproduce():
    libc = ctypes.CDLL(None, use_errno=True)
    need(libc.prctl(36, 1, 0, 0, 0) == 0, "cannot enable subreaper for owned cleanup")
    root = Path(tempfile.mkdtemp(prefix="mhgp10-scale-timeout-proof-"))
    (root / "python_owned").symlink_to(Path(sys.executable).resolve())
    process = subprocess.Popen([sys.executable, "-B", str(Path(__file__).resolve()), "--worker", str(root)],
                               start_new_session=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    record = {"schema": "mhgp10.audit_timeout_descendant.v1", "source": str(SOURCE),
              "source_sha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
              "reproducer_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              "worker_pid": process.pid, "configured_timeout_seconds": 1.0,
              "historical_session5_timeouts": 0, "engine_modified": False, "gcp_used": False}
    controlled = None
    try:
        stdout, stderr = process.communicate(timeout=4)
        record.update(worker_returncode=process.returncode, worker_stdout=stdout, worker_stderr=stderr)
        controlled = json.loads((root / "child.json").read_text())
        record["child"] = controlled
        need(controlled["pgid"] == process.pid, "unexpected child process group")
        child_proc = Path("/proc") / str(controlled["pid"])
        record["child_alive_after_timeout_return"] = child_proc.exists()
        if child_proc.exists():
            record["child_proc_state_after_timeout"] = (child_proc / "stat").read_text().split()[2]
        result = json.loads(stdout)
        need(result["result"][0] == -9, "timeout did not return -9")
        need(record["child_proc_state_after_timeout"] == "S", "controlled sleeping descendant not alive")
        record["regression_reproduced"] = True
    except BaseException as error:
        record["error"] = repr(error)
        raise
    finally:
        try:
            os.killpg(process.pid, signal.SIGTERM)
            record["owned_group_sigterm"] = True
        except ProcessLookupError:
            record["owned_group_already_gone"] = True
        process.wait(timeout=3)
        record["reaped_owned_descendants"] = []
        while True:
            try:
                pid, status = os.waitpid(-process.pid, 0)
                record["reaped_owned_descendants"].append({"pid": pid, "status": status})
            except ChildProcessError:
                break
        if controlled is not None:
            record["child_absent_after_cleanup"] = not (Path("/proc") / str(controlled["pid"])).exists()
        (root / "receipt.json").write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps({"receipt": str(root / "receipt.json"), **record}, indent=2))


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--child":
        child(sys.argv[2])
    elif len(sys.argv) == 3 and sys.argv[1] == "--worker":
        worker(Path(sys.argv[2]))
    else:
        need(len(sys.argv) == 1, "unexpected arguments")
        reproduce()
