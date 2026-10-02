#!/usr/bin/env python3
"""Read existing committed G4 artifacts; does not execute any product or test."""
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
ROOT = HERE / "sources/morsehgp3D_v11/receipts/developpement_20261002/reprise3"
receipt = json.loads((ROOT / "receipt.json").read_text())
matrix = json.loads((ROOT / "matrix.json").read_text())
archive = ROOT / "results.tar.gz"
sha = hashlib.sha256(archive.read_bytes()).hexdigest()
if sha != receipt["results_sha256"]:
    raise RuntimeError("G4 archive hash")
rows = []
with tarfile.open(archive) as tar:
    for config in matrix["configurations"]:
        name = config["name"]
        member_name = "results/cmd/000_matrice/files/matrix/" + name + "/junit.xml"
        member = tar.getmember(member_name)
        data = tar.extractfile(member).read()
        dest = HERE / "g4_logs" / name / "junit.xml"
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(data)
        root = ET.fromstring(data)
        tests = []
        for case in root.iter("testcase"):
            if "cloud" not in case.attrib.get("name", ""):
                continue
            tests.append({"name": case.attrib["name"], "status": case.attrib.get("status"),
                          "failure": case.find("failure") is not None or case.find("error") is not None,
                          "skipped": case.find("skipped") is not None,
                          "stdout": case.findtext("system-out", "")})
        rows.append({"configuration": name, "matrix_status": config["status"], "cloud_tests": tests,
                     "junit_member": member_name, "junit_sha256": hashlib.sha256(data).hexdigest()})

before = json.loads((HERE / "SOURCE_BEFORE.json").read_text())
matches = []
for item in before["sources"]:
    name = item["path"]
    if not (name.startswith("morsehgp3D_v11/src/") or name.startswith("morsehgp3D_v11/tests/cloud/")):
        continue
    qualified = subprocess.run(["git", "show", receipt["commit"] + ":" + name], cwd="/workspaces/E-HGP/build/v11-development-20261002", capture_output=True, check=True).stdout
    matches.append({"path": name, "reviewed_sha256": item["sha256"], "qualified_sha256": hashlib.sha256(qualified).hexdigest(),
                    "identical": item["sha256"] == hashlib.sha256(qualified).hexdigest()})
if not all(m["identical"] for m in matches):
    raise RuntimeError("reviewed source differs from qualified commit")
result = {"scope": "Read committed archives only; no native/product execution, no new GCP session, no LiDAR/FULL performance qualification",
          "reviewed_commit": before["commit"], "qualified_commit": receipt["commit"], "archive_sha256": sha,
          "source_matches": matches, "configurations": rows}
(HERE / "G4_CLOUD_EVIDENCE.json").write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n")
print(json.dumps({"matching_source_files": len(matches), "configurations": [{"name": row["configuration"], "cloud_tests": len(row["cloud_tests"]), "failures": sum(t["failure"] for t in row["cloud_tests"]), "skipped": sum(t["skipped"] for t in row["cloud_tests"])} for row in rows]}))
