#!/usr/bin/env python3
"""Publish a CLOSED local G4 receipt; never makes a GCP/SSH call.

No snapshot or input payload is copied into v9. Host evidence is allowlisted;
only received/output is copied recursively, after protocol revalidation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import sys
import tarfile

import analyze_receipt as analysis

ROOT = Path(__file__).resolve().parents[3]


def need(ok, why):
    if not ok:
        raise ValueError(why)


def read(path):
    def unique(items):
        result = {}
        for key, value in items:
            need(key not in result, "duplicate JSON key: " + key)
            result[key] = value
        return result
    return json.loads(path.read_text(), object_pairs_hook=unique)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, sort_keys=True, indent=2) + "\n")


def redact(raw):
    text = raw.decode("utf-8")
    text = re.sub(r"[A-Za-z0-9_.+%-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}", "<account redacted>", text)
    text = re.sub(r"(?:ssh-ed25519|ssh-rsa|ecdsa-sha2-\S+) [A-Za-z0-9+/=]+(?: [^\r\n]*)?",
                  "<public SSH key redacted>", text)
    need("PRIVATE KEY" not in text, "private key marker in publication")
    return text.encode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", type=Path, required=True)
    parser.add_argument("--package", type=Path, required=True)
    parser.add_argument("--after-stop", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    host, output = args.host.resolve(), args.output.absolute()
    need(host.name == "gpu_filter_v9_host" and not args.host.is_symlink(), "exact host directory only")
    need(not output.exists() and not output.is_symlink(), "fresh publication directory required")
    closed = read(host / "receipt.json")
    need(closed.get("targeted_shutdown_certified") is True and closed.get("status") == "completed",
         "publication requires the closed completed host receipt")
    after = read(args.after_stop)
    sys.path.insert(0, str(ROOT / "gcp-migration"))
    import gpu_filter_session_v9 as session
    import gpu_filter_worker_v9 as worker
    session.validate_target(after, "TERMINATED", closed["generation"])
    package = read(args.package)
    manifest = read(host / "source_manifest.json")
    need(sha(host / "snapshot.tar.gz") == closed["snapshot_sha256"] == package["snapshot_sha256"], "snapshot pin")
    need(sha(host / "source_manifest.json") == closed["manifest_sha256"] == package["manifest_sha256"], "manifest pin")
    session.validate_protocol_runtime(manifest)
    need(sha(Path(worker.__file__)) == closed["worker_sha256"] == package["worker_sha256"], "worker pin")
    cases, provenance = session.validate_snapshot(host / "snapshot.tar.gz", manifest)
    need(session.validate_received(host / "received/output", manifest, closed["worker_sha256"], cases,
         closed["generation"], provenance, closed["verified_guard"]) == "completed", "closed raw receipt rejudge")
    original_analysis = analysis.build(host)
    inputs = host / "received/output"
    for path in inputs.rglob("*"):
        need(not path.is_symlink(), "symlink in captured output")
        if path.is_file():
            need(path.suffix not in (".u16le", ".u32le", ".f32le", ".bin") or path.name == worker.PREFLIGHT_FILE,
                 "input payload unexpectedly present in output")
    output.mkdir(parents=True)
    shutil.copytree(inputs, output / "vm")
    need({str(p.relative_to(inputs)): sha(p) for p in inputs.rglob("*") if p.is_file()} ==
         {str(p.relative_to(output / "vm")): sha(p) for p in (output / "vm").rglob("*") if p.is_file()},
         "raw VM capture changed during copying")
    evidence = output / "host"
    evidence.mkdir()
    for path in sorted(host.iterdir()):
        if path.is_file() and (path.name.endswith(".command.json") or path.name.endswith(".intent.json") or
                               path.name in ("receipt.json", "handoff.json", "lifecycle.txt")):
            need(not path.is_symlink(), "host evidence symlink")
            need(b"PRIVATE KEY" not in path.read_bytes(), "private key marker in host evidence")
            shutil.copyfile(path, evidence / path.name)
            need(sha(path) == sha(evidence / path.name), "host evidence copy hash")
    for name in ("double_guard_verified", "guest_guard_pending"):
        path = host / "guardmarks" / name
        if path.exists():
            need(path.is_file() and not path.is_symlink(), "guard mark path")
            shutil.copyfile(path, evidence / name)
    redactions = []
    for stem in ("guarded_start", "guarded_stop"):
        for suffix in ("stdout", "stderr"):
            path = host / (stem + "." + suffix)
            if path.exists():
                raw = path.read_bytes()
                target = evidence / (stem + ".redacted." + suffix)
                target.write_bytes(redact(raw))
                redactions.append(dict(source_name=path.name, source_sha256=hashlib.sha256(raw).hexdigest(),
                                       published_name=target.name, published_sha256=sha(target)))
    projection = {key: after[key] for key in ("name", "selfLink", "status", "lastStartTimestamp", "lastStopTimestamp",
                  "zone", "machineType", "labels", "scheduling") if key in after}
    save(evidence / "after_stop.json", projection)
    save(evidence / "PUBLICATION_REDACTIONS.json", dict(redactions=redactions,
         after_stop_source_sha256=sha(args.after_stop), after_stop_policy="GCE state projection; metadata and account fields omitted",
         omitted="All other host stdout/stderr, especially OS Login/account/public-key dumps; session parent never traversed"))
    shutil.copyfile(args.package, output / "PACKAGE.json")
    shutil.copyfile(host / "source_manifest.json", output / "source_manifest.json")
    with tarfile.open(host / "snapshot.tar.gz", "r:gz") as archive:
        plan_raw = archive.extractfile(worker.PLAN).read()
    need(hashlib.sha256(plan_raw).hexdigest() == manifest[worker.PLAN], "plan hash")
    (output / "plan.json").write_bytes(plan_raw)
    published_analysis = analysis.build(output, snapshot_path=host / "snapshot.tar.gz")
    expected_analysis = dict(original_analysis, shutdown_raw_hashes_verified=False)
    need(published_analysis == expected_analysis, "published/raw analysis differs beyond redacted stop logs")
    save(output / "SUMMARY.json", published_analysis)
    inventory = sorted(path for path in output.rglob("*") if path.is_file())
    (output / "SHA256SUMS").write_text("".join(sha(path) + "  " + str(path.relative_to(output)) + "\n"
                                              for path in inventory))
    print(json.dumps(dict(status="published_closed_capture", path=str(output), files=len(inventory),
                          generation=closed["generation"], GCP_calls=False), sort_keys=True))


if __name__ == "__main__":
    main()
