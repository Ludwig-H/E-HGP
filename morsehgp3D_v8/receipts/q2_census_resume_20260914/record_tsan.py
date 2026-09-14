#!/usr/bin/env python3
"""Local TSan capture of the owned continuation gate; no build/provisioning."""
import json
import os
from pathlib import Path
import signal
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "morsehgp3D_v8/bench"))
from run_q2_resume_checks import digest, require, source_pins, write_json  # noqa: E402
from run_p0_matrix import invoke, on_signal, utc_stamp  # noqa: E402


def main():
    require(len(sys.argv) == 1, "this capture accepts no arguments")
    build = ROOT / "build/v8_census_resume_tsan_clang_20260914"
    binary = build / "mhgp8_q2_census_resume_gate"
    cache = (build / "CMakeCache.txt").read_text()
    require("-fsanitize=thread" in cache and "MHGP8_SANITIZE:BOOL=OFF" in cache, "wrong TSan configuration")
    source = source_pins()
    artifacts = {str(p.relative_to(ROOT)): digest(p) for p in (binary, build / "CMakeCache.txt", Path(__file__).resolve())}
    output = Path(tempfile.mkdtemp(prefix="tsan_", dir=Path(__file__).resolve().parent))
    command = ["timeout", "60s", str(binary), "--selftest"]
    record = dict(command=command, cwd=str(ROOT), started_utc=utc_stamp(), status="failed", exit_code=None,
                  stdout="", stderr="", stdout_base64="", stderr_base64="", source_sha256=source,
                  artifact_sha256=artifacts, full_contract_qualified=False, gcp_used=False)
    previous = {sig: signal.signal(sig, on_signal) for sig in (signal.SIGINT, signal.SIGTERM)}
    status, error = "failed", None
    try:
        invoke(command, dict(os.environ, TSAN_OPTIONS="halt_on_error=1"), ROOT, record, new_session=True)
        require(record["exit_code"] == 0, "TSan gate failed")
        row = json.loads(record["stdout"])
        require(row["status"] == "pass" and row["resumptions"] == 768 and
                row["thread_transfers"] > 0 and row["overlap_rejections"] > 0, "TSan gate coverage missing")
        require(source_pins() == source and all(digest(ROOT / p) == h for p, h in artifacts.items()), "TSan inputs changed")
        status = record["status"] = "passed"
    except BaseException as cause:
        error = f"{type(cause).__name__}: {cause}"
        raise
    finally:
        record["finished_utc"] = utc_stamp()
        write_json(output / "attempt.json", record)
        write_json(output / "completion.json", dict(status=status, error=error,
            attempt_sha256=digest(output / "attempt.json"), source_sha256_after=source_pins(),
            artifact_sha256_after={p: digest(ROOT / p) for p in artifacts}))
        for sig, handler in previous.items():
            signal.signal(sig, handler)
        print(json.dumps(dict(path=str(output), status=status, error=error)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
