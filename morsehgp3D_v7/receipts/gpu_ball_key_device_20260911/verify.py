#!/usr/bin/env python3
"""Verify every portable object, then run the same pinned read-only proof reader."""
import hashlib
import json
from pathlib import Path, PurePosixPath
import sys

HERE = Path(__file__).resolve().parent


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    need(len(sys.argv) == 1, "no_partial_publication_reader")
    manifest = json.loads((HERE / "manifest.json").read_text())
    need(manifest["schema"] == "mhgp7-gpu-ball-key-private-local-v1"
         and manifest["device_executed"] is False and manifest["GCP_used"] is False
         and manifest["public_status"] == "not_claimed", "bounded_manifest_authority")
    entries = {row["path"]: row for row in manifest["files"]}
    need(len(entries) == len(manifest["files"]) and len(entries) > 500, "manifest_unique_nonvacuity")

    def data(name):
        need(name in entries, "manifest_missing:" + name)
        return (HERE / entries[name]["object"]).read_bytes()

    for name, row in entries.items():
        path = PurePosixPath(name)
        need(not path.is_absolute() and ".." not in path.parts, "manifest_relative_path")
        need(len(row["sha256"]) == 64 and all(character in "0123456789abcdef" for character in row["sha256"]),
             "manifest_digest_shape")
        need(row["object"] == "objects/" + row["sha256"], "manifest_object_path")
        payload = data(name)
        need(len(payload) == row["bytes"] and sha(payload) == row["sha256"], "object_integrity:" + name)
        need(not payload.startswith(b"\x7fELF"), "ELF_forbidden")
    need(data("portable_verify.py") == Path(__file__).read_bytes(), "reader_self_pin")
    need(data("PUBLIC_README.md") == (HERE / "README.md").read_bytes(), "public_readme_pin")
    need(data("NOTE_MATH_ET_DEVICE.md") == (HERE / "NOTE_MATH_ET_DEVICE.md").read_bytes(), "public_note_pin")

    class LogicalPath:
        """Read-only path view over objects; no materialization or writes."""
        def __init__(self, name=""):
            self.name = name

        def __truediv__(self, suffix):
            return LogicalPath(self.name + ("/" if self.name else "") + str(suffix))

        def read_bytes(self):
            return data(self.name)

        def read_text(self):
            return self.read_bytes().decode()

        def exists(self):
            return self.name in entries

    # Reuse the exact source of the locally executed proof reader. Only HERE
    # changes to the read-only object view. No C++, kernel, cloud or subprocess
    # is executed by that reader, whose full source is itself a pinned object.
    namespace = {"__name__": "mhgp7_pinned_private_reader", "__file__": str(HERE / "verify.py")}
    exec(compile(data("verify.py"), "pinned/verify.py", "exec"), namespace)
    namespace["HERE"] = LogicalPath()
    namespace["main"]()

    checks = json.loads(data("reader_checks/receipt.json"))
    need(checks["status"] == "passed", "closed_normal_optimized_readers")
    need([row["name"] for row in checks["commands"]] == ["normal", "optimized"], "reader_modes")
    for row in checks["commands"]:
        need(row["exit_code"] == 0 and ("-O" in row["argv"]) == (row["name"] == "optimized"),
             "reader_command_exit_and_mode")
        for stream in ("stdout", "stderr"):
            need(sha(data("reader_checks/" + row["name"] + "." + stream)) == row[stream + "_sha256"],
                 "reader_command_stream_pin")
        result = json.loads(data("reader_checks/" + row["name"] + ".stdout"))
        need(result["status"] == "verified_closed" and "mutants_r1" in result["captures"]
             and "host_san_root_r1" in result["captures"], "reader_closed_qualification_domain")
    print(json.dumps(dict(status="verified_portable_ball_key_local", logical_files=len(entries),
        manifest_sha256=sha((HERE / "manifest.json").read_bytes()),
        device_executed=False, GCP_used=False, public_status="not_claimed")))


if __name__ == "__main__":
    main()
