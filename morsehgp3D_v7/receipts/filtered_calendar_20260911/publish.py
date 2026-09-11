#!/usr/bin/env python3
"""Close the prepared source-only packet, preserving exact capture bytes."""
import hashlib
import json
from pathlib import Path

BASE = Path(__file__).resolve().parent
ORIGIN = BASE.parent / "v7_filtered_calendar_20260911"
PACKET = BASE / "packet"
RUNS = ("o2_r1", "o2_r2", "san_root_r1", "chains_o2_root_r1", "chains_san_root_r1")
SOURCES = ("filtered_calendar.hpp", "gate.cpp", "historical_chains.hpp",
           "historical_chains_gate.cpp", "README.md", "record.py", "record_chains.py")


def need(ok, message):
    if not ok:
        raise ValueError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    need(not (PACKET / "MANIFEST.json").exists(), "packet already closed")
    logical, physical, by_hash, omitted = {}, {}, {}, []

    def add(name, source):
        need(source.is_file() and not source.is_symlink(), "plain source: " + str(source))
        data = source.read_bytes()
        need(not data.startswith(b"\x7fELF"), "ELF must not be included")
        pin = digest(data)
        location = by_hash.get(pin, name)
        target = PACKET / location
        if pin not in by_hash:
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.exists():
                need(target.is_file() and not target.is_symlink() and target.read_bytes() == data,
                     "prepared source differs: " + name)
            else:
                with target.open("xb") as stream:
                    stream.write(data)
            by_hash[pin] = location
            physical[location] = {"sha256": pin, "size": len(data)}
        logical[name] = {"path": location, "sha256": pin, "size": len(data)}

    for name in SOURCES:
        add("sources/" + name, ORIGIN / name)
    for name in ("README.md", "verify.py", "publish.py"):
        add(name, BASE / name)
    add("history/initial_chains_compile_failure.json", ORIGIN / "initial_chains_compile_failure.json")
    for run in RUNS:
        receipt = json.loads((ORIGIN / run / "receipt.json").read_text())
        need(receipt["status"] == "passed", "capture not closed: " + run)
        for source in sorted((ORIGIN / run).iterdir()):
            need(source.is_file() and not source.is_symlink(), "capture must be flat/plain")
            if source.name == "gate":
                data = source.read_bytes()
                need(data.startswith(b"\x7fELF") and digest(data) == receipt["binary_sha256"], "ELF pin")
                omitted.append({"logical_path": "captures/" + run + "/gate", "reason": "ELF",
                                "sha256": digest(data), "size": len(data)})
            else:
                add("captures/" + run + "/" + source.name, source)
    manifest = {"schema": "filtered_calendar_packet_v1", "logical_files": logical,
                "physical_files": physical, "omitted": omitted, "captures": list(RUNS),
                "geometry_qualified": False, "performance_claim": False, "GCP_used": False}
    actual = {str(path.relative_to(PACKET)) for path in PACKET.rglob("*") if path.is_file()}
    need(actual == set(physical), "unmapped prepared file")
    with (PACKET / "MANIFEST.json").open("x") as stream:
        json.dump(manifest, stream, indent=2, sort_keys=True)
        stream.write("\n")
    print(json.dumps({"status": "closed", "logical_files": len(logical), "physical_files": len(physical),
                      "manifest_sha256": digest((PACKET / "MANIFEST.json").read_bytes())}))


if __name__ == "__main__":
    main()
