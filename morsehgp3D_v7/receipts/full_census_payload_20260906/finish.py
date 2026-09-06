#!/usr/bin/env python3
"""Targeted CTest closure and lightweight publication; no benchmark or GCP."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import time
import xml.etree.ElementTree as ET

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
RECEIPTS = ROOT / "morsehgp3D_v7/receipts"
RUN = BASE / "run_r1"
DEST = RECEIPTS / "full_census_payload_20260906"
RUN_PIN = "f6101d22321ba23691293b4e979e26949baeb675a3c1c8ebe8939657b24550ab"
NAMES = {"mhgp7_full_gabriel_census_payload", "mhgp7_full_gabriel_census_payload_bad_argument"}
NEW = {"morsehgp3D_v7/bench/full_gabriel_lazy_probe.cpp", "morsehgp3D_v7/bench/full_gabriel_probe_limits.hpp",
       "morsehgp3D_v7/tests/full_gabriel_census_payload_gate.cpp"}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def write(path, value):
    path.write_text(json.dumps(value, sort_keys=True, indent=2) + "\n")


def verify(packet):
    for line in (packet / "SHA256SUMS").read_text().splitlines():
        pin, name = line.split("  ", 1)
        need(not name.startswith(("/", "./")) and ".." not in Path(name).parts, "manifest path")
        need(sha(packet / name) == pin, "manifest " + name)
    publication = json.loads((packet / "publication.json").read_text())
    for name, ref in publication["source_references"].items():
        need(sha(packet / ref["relative_path"]) == ref["sha256"], "external source " + name)
    print(json.dumps({"status": "verified", "files": len(list(packet.rglob("*"))),
                      "external_source_references": len(publication["source_references"]), "ELF_published": False}, sort_keys=True))


def execute():
    need(sha(RUN / "receipt.json") == RUN_PIN, "closed qualification pin")
    original = json.loads((RUN / "receipt.json").read_text())
    for name, pin in original["artifacts"].items():
        need(sha(RUN / name) == pin, "closed artifact " + name)
    logs = BASE / "ctest_r1"
    build = BASE / "cmake_r1"
    logs.mkdir(exist_ok=False)
    need(not build.exists() and not DEST.exists(), "create only")
    cmake_before = sha(ROOT / "morsehgp3D_v7/CMakeLists.txt")
    commands = []

    def command(name, argv):
        item = {"argv": list(map(str, argv)), "cwd": str(ROOT), "started_ns": time.time_ns(), "timeout": None}
        write(logs / (name + ".intent.json"), item)
        with (logs / (name + ".stdout")).open("wb") as out, (logs / (name + ".stderr")).open("wb") as err:
            p = subprocess.Popen(item["argv"], cwd=ROOT, stdout=out, stderr=err, start_new_session=True)
            item["pid"] = p.pid
            write(logs / (name + ".spawn.json"), item)
            try:
                item["exit_code"] = p.wait()
            except BaseException:
                os.killpg(p.pid, signal.SIGKILL)
                item["exit_code"] = p.wait()
                raise
            finally:
                item["ended_ns"] = time.time_ns()
                commands.append(item)
                write(logs / (name + ".command.json"), item)
        print(name, item["exit_code"], flush=True)
        need(item["exit_code"] == 0, name)

    command("configure", ["cmake", "-S", ROOT / "morsehgp3D_v7", "-B", build, "-DCMAKE_BUILD_TYPE=Release",
                          "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", "-DMHGP7_DIGEST_BOOST_INCLUDE_DIR=/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include"])
    command("build", ["cmake", "--build", build, "--target", "mhgp7_full_gabriel_census_payload_gate", "--parallel", "2"])
    regex = "^mhgp7_full_gabriel_census_payload(_bad_argument)?$"
    command("inventory", ["ctest", "--test-dir", build, "-R", regex, "--show-only=json-v1"])
    inventory = json.loads((logs / "inventory.stdout").read_text())
    need({test["name"] for test in inventory["tests"]} == NAMES, "exact two tests")
    need(all(prop["name"] != "TIMEOUT" for test in inventory["tests"] for prop in test["properties"]), "no new timeout")
    command("ctest", ["ctest", "--test-dir", build, "-R", regex, "-V", "--output-on-failure", "--output-junit", logs / "junit.xml"])
    cases = ET.parse(logs / "junit.xml").getroot().findall(".//testcase")
    need(len(cases) == 2 and {case.attrib["name"] for case in cases} == NAMES and
         all(case.find("failure") is None and case.find("skipped") is None for case in cases), "CTest results")
    need(sha(ROOT / "morsehgp3D_v7/CMakeLists.txt") == cmake_before, "CMake stable")
    for name, pin in json.loads((RUN / "sources_before.json").read_text()).items():
        need(sha(ROOT / name) == pin, "source stable " + name)
    binding = {"CMakeLists_sha256": cmake_before, "binary_sha256": sha(build / "mhgp7_full_gabriel_census_payload_gate"),
               "commands": commands, "status": "completed", "tests": sorted(NAMES), "explicit_timeout": None}
    for filename in ("compile_commands.json", "CTestTestfile.cmake", "CMakeCache.txt", "Testing/Temporary/LastTest.log",
                     "CMakeFiles/mhgp7_full_gabriel_census_payload_gate.dir/flags.make",
                     "CMakeFiles/mhgp7_full_gabriel_census_payload_gate.dir/tests/full_gabriel_census_payload_gate.cpp.o.d"):
        target = logs / filename
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(build / filename, target)
    write(logs / "receipt.json", binding)
    refs = {}
    for name, pin in json.loads((RUN / "sources_before.json").read_text()).items():
        if name in NEW:
            continue
        candidates = [RECEIPTS / "full_pipeline_threads_micro_20260906/sources" / name,
                      RECEIPTS / "full_probe_no_quotas_20260906/source_snapshot" / name,
                      RECEIPTS / "wspd_terminal_once_negative_20260906/payload/reference" / Path(name).relative_to("morsehgp3D_v7")]
        target = next((p for p in candidates if p.is_file() and sha(p) == pin), None)
        need(target is not None, "sealed source reference " + name)
        refs["run_r1/sources/" + name] = {"relative_path": "../" + str(target.relative_to(RECEIPTS)), "sha256": pin}
    DEST.mkdir(exist_ok=False)
    for path in RUN.iterdir():
        if path.is_file():
            dest = DEST / "run_r1" / path.name
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, dest)
    for name in NEW:
        dest = DEST / "run_r1/sources" / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(RUN / "sources" / name, dest)
    shutil.copytree(logs, DEST / "ctest")
    shutil.copyfile(ROOT / "morsehgp3D_v7/CMakeLists.txt", DEST / "CMakeLists.txt")
    shutil.copyfile(__file__, DEST / "finish.py")
    shutil.copyfile(BASE / "PUBLIC_README.md", DEST / "README.md")
    write(DEST / "publication.json", {"schema": "mhgp7-full-census-payload-publication-v1", "status": "completed",
          "public_status": "not_claimed", "qualification_receipt_sha256": RUN_PIN, "source_references": refs,
          "omitted_binary_hashes": {name: pin for name, pin in original["artifacts"].items() if name.startswith("bin/")},
          "CTest_receipt_sha256": sha(logs / "receipt.json"), "historical_paths_in_captures_preserved": True})
    for path in DEST.rglob("*"):
        if path.is_file():
            need(path.read_bytes()[:4] != b"\x7fELF", "no ELF")
    files = sorted(path for path in DEST.rglob("*") if path.is_file())
    (DEST / "SHA256SUMS").write_text("".join(f"{sha(path)}  {path.relative_to(DEST)}\n" for path in files))
    verify(DEST)


if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1] == "--execute":
        execute()
    elif len(sys.argv) == 3 and sys.argv[1] == "--verify":
        verify(Path(sys.argv[2]).resolve())
    else:
        sys.exit(2)
