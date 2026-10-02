#!/usr/bin/env python3
"""Capsule reader only: no native/product/GCP calls."""
import hashlib
import json
from pathlib import Path
import tarfile
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
checks = 0

def require(ok, message):
    global checks
    checks += 1
    if not ok:
        raise RuntimeError(message)

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

before = json.loads((HERE / "SOURCE_BEFORE.json").read_text())
supplement = json.loads((HERE / "SUPPLEMENT_BEFORE.json").read_text())
require(before["commit"] == supplement["commit"] == "6a22a9118a9dc692d3b5a34b8fd97673746697c4", "commit scope")
for item in before["sources"] + supplement["sources"]:
    require(digest(HERE / "sources" / item["path"]) == item["sha256"], "snapshot hash " + item["path"])
evidence = json.loads((HERE / "G4_CLOUD_EVIDENCE.json").read_text())
root = HERE / "sources/morsehgp3D_v11/receipts/developpement_20261002/reprise3"
receipt = json.loads((root / "receipt.json").read_text())
require(evidence["qualified_commit"] == receipt["commit"] == "a971806679a1c68519249bb28c5dac9533a43f59", "qualified source")
require(digest(root / "results.tar.gz") == receipt["results_sha256"] == evidence["archive_sha256"], "G4 archive")
require(len(evidence["source_matches"]) == 19, "source match count")
for match in evidence["source_matches"]:
    require(match["identical"] and match["qualified_sha256"] == match["reviewed_sha256"] == digest(HERE / "sources" / match["path"]), "matched source " + match["path"])
expected = {"gcc_release", "gcc_asan_ubsan", "gcc_tsan", "bits21", "bits24", "poison"}
with tarfile.open(root / "results.tar.gz") as tar:
    for row in evidence["configurations"]:
        name = row["configuration"]
        if name in expected:
            require(row["matrix_status"] == "ok" and len(row["cloud_tests"]) == 20, "G4 configuration " + name)
            require(all(t["status"] == "run" and not t["failure"] and not t["skipped"] for t in row["cloud_tests"]), "cloud outcomes " + name)
            data = tar.extractfile(row["junit_member"]).read()
            require(hashlib.sha256(data).hexdigest() == row["junit_sha256"] == digest(HERE / "g4_logs" / name / "junit.xml"), "JUnit hash " + name)
            cases = [c for c in ET.fromstring(data).iter("testcase") if "cloud" in c.attrib.get("name", "")]
            require([c.attrib["name"] for c in cases] == [t["name"] for t in row["cloud_tests"]], "JUnit names " + name)
            for group, count in (("ownership", 27), ("shared_budget", 11)):
                case = next(t for t in row["cloud_tests"] if t["name"] == "mhgp11_cloud_unit_" + group)
                require("controles=" + str(count) + " echecs=0" in case["stdout"], "native controls " + name + " " + group)
        elif name == "clang_release":
            require(row["optional_unavailable"] and not row["cloud_tests"], "no Clang qualification")
require(expected <= {r["configuration"] for r in evidence["configurations"]}, "all selected configurations")
after = json.loads((HERE / "SOURCE_AFTER.json").read_text())
require(after["snapshot_intact"], "snapshot closure")
manifest = HERE / "SHA256SUMS"
if manifest.exists():
    for line in manifest.read_text().splitlines():
        sha, name = line.split("  ", 1)
        if digest(HERE / name) != sha:
            raise RuntimeError("closed artifact hash " + name)
print(json.dumps({"status": "PASS", "checks": checks, "scope": "Read frozen Cloud source and committed G4 archive only; no new native execution or FULL qualification"}, sort_keys=True))
