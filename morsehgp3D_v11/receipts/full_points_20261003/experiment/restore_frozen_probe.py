"""Reuse the exact c40 audit ELF already compiled and gated on G4.

The current worker source supplies orchestration only. This does not build or
qualify its newer production engine. Each resumed session must gate this exact
binary again before any whole-scene experiment. No native execution here.
"""
import argparse
import hashlib
import json
from pathlib import Path

from build_probe import BuildFailure, FLAGS, PROFILE, fresh_directory, require
from prepare_python_host import worker_context

SOURCE = "a12f7f5425974d398e87271cac0a40b79393f0d2"
BINARY_SHA = "5881224aeae7cd110a6e935cd3490ec9dcbef9ecf616ca4b78b85114c0b0ba36"
MANIFEST_SHA = "8ee49235e8a557faffef4dcde3b3895cf56f5c5b831055c309953128ee98edb5"
BINARY_BYTES = 561400
MANIFEST_BYTES = 41872


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def verify_frozen(binary, manifest):
    require(len(binary) == BINARY_BYTES and binary[:4] == b"\x7fELF" and
            sha(binary) == BINARY_SHA, "frozen_binary_pin")
    require(len(manifest) == MANIFEST_BYTES and sha(manifest) == MANIFEST_SHA,
            "frozen_build_manifest_pin")
    data = json.loads(manifest)
    require(data["status"] == "built" and data["exit_code"] == 0 and
            data["sources_unchanged"] is True and
            data["sources_before"] == data["sources_after"] and
            data["source_pin"] == "commit:" + SOURCE and
            data["source_commit"] == SOURCE and data["source_kind"] == "commit" and
            data["coord_bits"] == 21 and data["profile"] == PROFILE and
            data["flags"] == FLAGS and
            data["binary"]["sha256"] == BINARY_SHA and
            data["binary"]["bytes"] == BINARY_BYTES,
            "frozen_build_closure")
    return data


def read_bounded(path, size):
    require(path.is_file() and not path.is_symlink() and path.stat().st_size == size,
            "regular_pinned_payload_required")
    with path.open("rb") as stream:
        raw = stream.read(size + 1)
    require(len(raw) == size, "payload_size_changed")
    return raw


def main():
    import os
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    wrapper = Path(__file__).resolve()
    context = worker_context(os.environ, wrapper)
    binary = read_bounded(args.binary, BINARY_BYTES)
    manifest = read_bounded(args.manifest, MANIFEST_BYTES)
    frozen = verify_frozen(binary, manifest)
    build = fresh_directory(args.build, Path(context["repository"]))
    out = fresh_directory(args.out, Path(context["repository"]))
    target = build / "points_probe"
    target.write_bytes(binary)
    target.chmod(0o700)
    require(sha(read_bounded(target, BINARY_BYTES)) == BINARY_SHA,
            "restored_binary_closure")
    (out / "frozen_build.json").write_bytes(manifest)
    report = dict(schema="ehgp.audit.frozen_probe_reuse.v1", status="restored",
                  worker=context, built=False, native_executed=False,
                  backend="cpu_reference", profile=PROFILE,
                  engine_reference="c40f40798375a0fc37917499401f16876cccbd2a",
                  binary_source_commit=SOURCE, binary_sha256=BINARY_SHA,
                  binary_bytes=BINARY_BYTES, binary_path=str(target),
                  frozen_build_sha256=MANIFEST_SHA,
                  original_package_sha256=frozen["source_package_sha256"],
                  origin_session="v11.20261003.points-synth3",
                  wrapper_sha256=sha(wrapper.read_bytes()),
                  public_status="not_claimed",
                  scope="exact prior G4 audit ELF; newer engine not executed; fresh native gate required")
    (out / "reuse.json").write_text(json.dumps(report, sort_keys=True, indent=2) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
