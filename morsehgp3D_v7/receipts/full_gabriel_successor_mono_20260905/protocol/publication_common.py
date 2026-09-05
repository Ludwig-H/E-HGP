#!/usr/bin/env python3
"""Create-only publication of a CLOSED FULL lazy targeted qualification.

Default invocation is inert: no capture read, authority import or publication.
Separate ROOT GO and three SHA256 pins are required by --execute. No subprocess,
engine, compilation, Git or GCP call is made by this publisher.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import math
import os
from pathlib import Path, PurePosixPath
import re
import stat
import types

ROOT = Path("/workspaces/E-HGP")
BASE = ROOT / "build/v7_full_lazy_20260905_controller"
RUNS = BASE / "runs"
PUBLIC_REL = "morsehgp3D_v7/receipts/full_gabriel_lazy_20260905"
PUBLIC = ROOT / PUBLIC_REL
CONTROLLER = BASE / "capture.py"
CONTROLLER_SHA = "528175a4fae239aa62630c32c27355be34db1092bef7f8cdb98e589022663bb4"
CHECKER = ROOT / "tools/check_v7_receipt_publication.py"
CHECKER_SHA = "32420385f487260e0706b3e649befca25cc95a9d45f17d22472c333870729580"
MAX_FILES, MAX_FILE_BYTES, MAX_TOTAL_BYTES = 1024, 16 << 20, 128 << 20


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise RuntimeError(reason)


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def encode(value) -> bytes:
    return (json.dumps(value, sort_keys=True, indent=2) + "\n").encode()


def unique(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, "duplicate JSON key")
        result[key] = value
    return result


def json_value(raw: bytes):
    return json.loads(raw, object_pairs_hook=unique)


def meta(value) -> tuple:
    return value.st_dev, value.st_ino, value.st_size, value.st_mtime_ns, value.st_ctime_ns


class Reader:
    """Retain exact bytes and metadata; recheck every file before publication."""
    def __init__(self):
        self.files = {}
        self.total = 0

    def get(self, path: Path) -> bytes:
        if path in self.files:
            return self.files[path][0]
        before = path.lstat()
        require(stat.S_ISREG(before.st_mode), "not a regular nonsymlink file: " + str(path))
        require(before.st_size <= MAX_FILE_BYTES and self.total + before.st_size <= MAX_TOTAL_BYTES,
                "bounded publication byte inventory exceeded")
        with path.open("rb") as stream:
            require(meta(before) == meta(os.fstat(stream.fileno())), "file changed at open")
            raw = stream.read()
            require(meta(before) == meta(os.fstat(stream.fileno())), "file changed during read")
        require(meta(before) == meta(path.lstat()) and len(raw) == before.st_size, "file changed after read")
        self.files[path] = raw, meta(before)
        self.total += len(raw)
        require(len(self.files) <= MAX_FILES, "bounded publication file inventory exceeded")
        return raw

    def pinned(self, path: Path, pin: str) -> bytes:
        raw = self.get(path)
        require(sha(raw) == pin, "SHA256 mismatch: " + str(path))
        return raw

    def value(self, name: str):
        return json_value(self.get(RUNS / name))

    def recheck(self) -> None:
        for path, (raw, observed) in self.files.items():
            require(meta(path.lstat()) == observed and path.read_bytes() == raw and
                    meta(path.lstat()) == observed, "terminal byte/metadata drift: " + str(path))


def module(name: str, path: Path, raw: bytes):
    result = types.ModuleType(name)
    result.__file__ = str(path)
    exec(compile(raw, str(path), "exec"), result.__dict__)
    return result


def safe_name(name: str) -> bool:
    path = PurePosixPath(name)
    return bool(re.fullmatch("[A-Za-z0-9_./-]+", name)) and not path.is_absolute() and \
        ".." not in path.parts and str(path) == name


def files_in(directory: Path) -> set[str]:
    require(directory.is_dir() and not directory.is_symlink(), "directory missing or symlink")
    found = set()
    for path in directory.rglob("*"):
        require(not path.is_symlink(), "symlink in sealed capture")
        if path.is_file():
            name = str(path.relative_to(directory))
            require(safe_name(name), "unsafe receipt member name")
            found.add(name)
    require(0 < len(found) <= MAX_FILES, "capture inventory outside bound")
    return found


def sealed_capture(reader: Reader, receipt_pin: str, sums_pin: str) -> tuple[dict, set[str]]:
    receipt = json_value(reader.pinned(RUNS / "receipt.json", receipt_pin))
    require(receipt.get("schema") == "mhgp7-private-full-lazy-qualification-v1" and
            receipt.get("status") in ("completed", "failed") and receipt.get("ended_utc"),
            "capture is not terminal")
    require(datetime.fromisoformat(receipt["started_utc"]) <= datetime.fromisoformat(receipt["ended_utc"]),
            "capture chronology malformed")
    raw_sums = reader.pinned(RUNS / "SHA256SUMS", sums_pin)
    names = files_in(RUNS)
    manifest = reader.value("manifest.json")
    require(isinstance(manifest, dict) and set(manifest) | {"manifest.json", "SHA256SUMS"} == names,
            "capture manifest does not cover its exact terminal inventory")
    seen = {}
    for line in raw_sums.decode("utf-8").splitlines():
        match = re.fullmatch(r"([0-9a-f]{64})  ([A-Za-z0-9_./-]+)", line)
        require(match is not None, "capture checksum syntax")
        pin, name = match.groups()
        require(safe_name(name) and name != "SHA256SUMS" and name not in seen, "capture checksum path")
        seen[name] = pin
        reader.pinned(RUNS / name, pin)
    require(set(seen) == names - {"SHA256SUMS"}, "capture checksums miss/add files")
    for name, row in manifest.items():
        raw = reader.get(RUNS / name)
        require(row.get("path") == str(RUNS / name) and row.get("bytes") == len(raw) and
                row.get("sha256") == sha(raw), "raw manifest binding mismatch: " + name)
    require(receipt.get("public_status") == "not_claimed" and receipt.get("gcp") == "not_used" and
            receipt.get("hermetic") is False and receipt.get("permission_override") == "none" and
            receipt.get("fresh_tests_only") is True and receipt.get("historical_results_reused") is False,
            "capture scope/policy changed")
    return receipt, names


def raw_binding(reader: Reader, record: dict) -> None:
    for kind in ("stdout", "stderr"):
        row = record["raw"][kind]
        path = Path(row["path"])
        require(path.is_relative_to(RUNS), "command raw path escapes capture")
        raw = reader.get(path)
        require(row["sha256"] == sha(raw) and row["bytes"] == len(raw), "command raw byte binding")
    duration = record["elapsed_seconds"]
    require(type(record["exit_code"]) is int and type(duration) in (int, float) and
            math.isfinite(duration) and duration >= 0 and record["ended_utc"], "command not terminal")


def completed_phase(reader: Reader, C, mode: str, reported: dict, before: dict) -> dict:
    prefix = mode + "/"
    summary = reader.value(prefix + "summary.json")
    require(summary == reported and summary.get("mode") == mode and summary.get("status") == "completed" and
            summary.get("errors") == [], "completed phase summary mismatch")
    for key in ("sources_stable", "binaries_stable", "compile_binding_stable"):
        require(summary.get(key) is True, "phase stability not true")
    for name in ("sources_before.json", "sources_before_ctest.json", "sources_after.json"):
        require(reader.value(prefix + name) == before, "phase source snapshot differs")
    build = C.BUILDS[mode]
    binaries = reader.value(prefix + "binaries_after_build.json")
    require(set(binaries) == set(C.TARGETS) and
            binaries == reader.value(prefix + "binaries_before_ctest.json") ==
            reader.value(prefix + "binaries_after.json") == C.binary_snapshot(build), "six-binary drift")
    binding = reader.value(prefix + "compile_binding.json")
    require(binding == reader.value(prefix + "compile_binding_before_ctest.json") ==
            reader.value(prefix + "compile_binding_after.json") == C.compile_binding(build, mode, before),
            "compiled source/options/MDD drift")
    commands, records = C.commands(mode), summary["commands"]
    require(len(records) == len(commands) == 4, "phase command inventory")
    for (label, argv, limit), record in zip(commands, records):
        require(record == reader.value(prefix + label + ".command.json") and
                record["label"] == label and record["argv"] == argv and record["cwd"] == str(ROOT) and
                record["timeout_seconds"] == limit and record["expected_code"] == 0 and
                record["environment"] == C.environment(mode) and record["exit_code"] == 0 and
                record["status"] == "completed" and record["error"] is None, "exact phase command/status mismatch")
        raw_binding(reader, record)
    require(C.inventory(reader.get(RUNS / prefix / "inventory.stdout"), build) == summary["inventory"],
            "inventory replay differs")
    log = reader.get(RUNS / prefix / "LastTest.stdout")
    junit = reader.get(RUNS / prefix / "JUnit.stdout")
    require(junit == reader.get(RUNS / prefix / "ctest.junit.xml"), "JUnit archive differs")
    outputs = reader.value(prefix + "test_outputs.json")
    require(outputs == summary["test_outputs"] and
            outputs["last_test"] == C.judge_log(log, sorted(C.TESTS)) and
            outputs["junit"] == C.judge_junit(junit) and outputs["fresh"] is True, "test-output replay differs")
    fence = reader.value(prefix + "ctest.fence.json")
    prior = reader.value(prefix + "LastTest.prestate.json")
    for name, expected_raw in (("LastTest.stdout", log), ("JUnit.stdout", junit)):
        copied = outputs["copies"][name]
        source, archived = copied["source"], copied["archive"]
        require(source["device"] == fence["source"]["device"] and
                source["mtime_ns"] >= fence["source"]["mtime_ns"] and
                source["ctime_ns"] >= fence["source"]["ctime_ns"] and
                source["sha256"] == archived["sha256"] == sha(expected_raw), "test-output freshness/binding")
        require(C.read_stable(Path(source["path"]))[1] == source and
                C.read_stable(Path(archived["path"]))[1] == archived, "terminal live log/archive changed")
    require(C.read_stable(Path(fence["source"]["path"]))[1] == fence["source"] and
            reader.get(RUNS / prefix / "ctest.fence") == reader.get(Path(fence["source"]["path"])),
            "owned CTest fence changed")
    if prior["present"]:
        require(prior["binding"]["source"]["sha256"] != sha(log), "LastTest unchanged from prestate")
    require(reader.value(prefix + "environment.json") == C.environment(mode), "phase environment mismatch")
    require(reader.value(prefix + "host_before.json")["process_status"]["TracerPid"] == "0",
            "traced successful context")
    for state in ("before", "after"):
        require(reader.value("toolchain_" + state + ".json") == summary["toolchain_after"], "toolchain differs")
    gate_lines = {}
    for row in C.ET.fromstring(junit).findall("testcase"):
        gate_lines[row.get("name")] = [line for line in (row.findtext("system-out") or "").splitlines()
                                      if line.startswith("full_")]
    return {"mode": mode, "status": "completed_revalidated", "tests": 14, "binary_count": 6,
            "ctest_elapsed_seconds": records[-1]["elapsed_seconds"],
            "build_elapsed_seconds": records[1]["elapsed_seconds"], "gate_summary_lines": gate_lines}


def preflight(reader: Reader, C, args) -> tuple[dict, dict[str, bytes], set[str]]:
    receipt, names = sealed_capture(reader, args.expected_receipt_sha256, args.expected_capture_sums_sha256)
    outcomes = []
    modes = [row.get("mode") for row in receipt["phases"]]
    require(modes in ([], ["release"], ["release", "san"]), "phase order/duplicate/foreign mode")
    if receipt["status"] == "completed":
        require(modes == ["release", "san"] and receipt["errors"] == [], "global closure incomplete")
    before = reader.value("sources_before.json") if "sources_before.json" in names else None
    if before is not None and "controller_binding.json" in names:
        binding = reader.value("controller_binding.json")
        require(binding["controller_sha256"] == CONTROLLER_SHA and
                binding["source_sha256"] == sha(C.encoded(before)) and
                binding["expected_tests"] == json_value(C.encoded(C.TESTS)) and
                binding["targets"] == C.TARGETS, "controller/source/inventory admission mismatch")
    for row in receipt["phases"]:
        mode = row["mode"]
        if row["status"] == "completed":
            require(before is not None, "complete phase lacks initial sources")
            outcomes.append(completed_phase(reader, C, mode, row, before))
        else:
            require(row["status"] == "failed" and receipt["status"] == "failed" and row.get("errors"),
                    "unclosed or unacknowledged failed phase")
            require(row == reader.value(mode + "/summary.json"), "failed phase summary differs")
            outcomes.append({"mode": mode, "status": "failed_preserved", "tests": None, "errors": row["errors"]})
    if receipt["status"] == "completed":
        require("controller_binding.json" in names, "completed capture lacks admission")
        require(before == reader.value("sources_after.json") == C.source_snapshot(), "terminal source drift")
        require(reader.value("toolchain_before.json") == reader.value("toolchain_after.json") ==
                C.tool_snapshot(), "terminal toolchain drift")
        require(reader.value("host_before.json")["process_status"]["TracerPid"] == "0", "traced admission")
        boost = reader.value("boost_dependency_binding.json")
        require(boost["header_count"] == 521 and boost["canonical_header_map_sha256"] == C.BOOST_MAP_SHA and
                sha(reader.get(RUNS / "boost_precheck_dependencies.stdout")) == C.BOOST_DEPFILE_SHA,
                "pre-build Boost dependency admission mismatch")
        require(len(outcomes) == 2 and all(row["tests"] == 14 for row in outcomes), "14/14 x2 not established")
    classification = {"schema": "mhgp7-full-lazy-targeted-publication-v1",
                      "status": "qualified_targeted" if receipt["status"] == "completed" else "failed_capture_preserved",
                      "capture_status": receipt["status"], "phases": outcomes,
                      "public_status": "not_claimed", "whole_suite_qualified": False,
                      "binary_payload_exported": False, "boost_code_exported": False,
                      "performance_claim": False, "gcp": "not_used"}
    return classification, {"capture/" + name: reader.get(RUNS / name) for name in names}, names


def recheck_live(reader: Reader, C, classification: dict) -> None:
    for phase in classification["phases"]:
        if phase["status"] != "completed_revalidated":
            continue
        mode = phase["mode"]
        before = reader.value("sources_before.json")
        require(C.source_snapshot() == before and
                C.binary_snapshot(C.BUILDS[mode]) == reader.value(mode + "/binaries_after.json") and
                C.compile_binding(C.BUILDS[mode], mode, before) == reader.value(mode + "/compile_binding_after.json"),
                "source/binary/compile binding changed at publication boundary")


def readme(classification: dict) -> bytes:
    good = classification["status"] == "qualified_targeted"
    lines = ["# Qualification ciblée du producteur FULL à cache facultatif", "",
             "5 septembre 2026. public_status=not_claimed, CPU de référence, entrée u16.", "",
             ("Les deux builds neufs ferment **14/14 CTests Release et 14/14 ASan/UBSan**, "
              "après relecture des captures et des liaisons." if good else
              "**Capture échouée conservée. Aucune qualification globale 14/14 × 2 n’est revendiquée.**"), "",
             "Les six exécutables couvrent le certificat structurel, le producteur eager, ses injections "
             "mémoire, le producteur lazy, ses injections mémoire et le digest sémantique.", "",
             "Les commandes, stdout, stderr, codes, délais, JUnit et LastTest sont copiés byte pour byte "
             "dans [capture/](capture/). Les échecs éventuels ne sont ni effacés ni réécrits.", "",
             "Protocole prévu — Release : O3/NDEBUG. Instrumenté : O1/g/NDEBUG, AddressSanitizer et UndefinedBehaviorSanitizer, "
             "sans PIE, avec ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 et "
             "UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1. Aucune escalade ni désactivation LSAN.", "",
             "Construction CPU0, parallélisme 2 et plafond 600 s ; CTest CPU6, parallélisme 1, "
             "60 s par porte et 120 s pour l’appel complet. Ce sont des limites de qualification, pas un benchmark.", "",
             ("Les cartes de sources, les six binaires et leurs options/dépendances sont liés avant/après. "
             "Les 521 headers Boost consommés sont pré-épinglés et recontrôlés ; seul leur inventaire "
             "inerte et leurs SHA sont exportés, pas leur code ni les binaires. "
             "Les headers système ne sont pas tous pré-épinglés : **build frais, non hermétique**." if good else
              "Les captures disponibles restent archivées, mais les liaisons sources/binaires et les 521 pins Boost "
              "ne sont pas tous déclarés validés dans une capture échouée. Aucun code Boost ni binaire n’est exporté."), "",
             "HEAD et le statut v7 sont observés sans exiger la stabilité globale du dépôt concurrent ; "
             "la stabilité des sources effectivement qualifiées reste obligatoire.", "",
             "## Observations des exécutions closes", ""]
    for phase in classification["phases"]:
        if phase["status"] != "completed_revalidated":
            lines.extend([f"- {phase['mode']} : échec conservé, aucun nombre de tests promu.", ""])
            continue
        lines.extend([f"- {phase['mode']} : 14/14 ; construction {phase['build_elapsed_seconds']:.6f} s ; "
                      f"CTest {phase['ctest_elapsed_seconds']:.6f} s.", "",
                      "Compteurs repris littéralement des mêmes sorties JUnit :", "", chr(96) * 3 + "text"])
        for name, summaries in phase["gate_summary_lines"].items():
            for line in summaries:
                lines.append(name + ": " + line)
        lines.extend([chr(96) * 3, ""])
    lines.extend(["## Portée et limites", "",
                  "Qualification ciblée de ce lot seulement : ni suite historique 339 ni suite v7 complète. "
                  "Les positifs n’authentifient pas des catalogues arbitraires ; l’autorité demeure relative "
                  "aux catalogues Gabriel complets, exacts et réguliers fournis.", "",
                  "Aucune qualification inter-K/CLI/archive, aucun gain de latence, aucun contrat 50k "
                  "en une seconde ou 100 ms, aucun résultat massif G4 n’en découle.", "",
                  "Les préchecks et anciennes campagnes ne deviennent pas des preuves fraîches. "
                  "Le fichier de dépendances du précheck Boost sert uniquement à fermer l’inventaire de headers.", "",
                  "[Reclassification](qualification.json), [manifest des octets](manifest.json), "
                  "[sommes complètes](SHA256SUMS). Les protocoles sous protocol/ sont des copies inertes ; "
                  "leur présence n’exécute aucune commande. GCP non utilisé.", ""])
    return "\n".join(lines).encode()


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", action="store_true")
    parser.add_argument("--expected-publisher-sha256")
    parser.add_argument("--expected-receipt-sha256")
    parser.add_argument("--expected-capture-sums-sha256")
    args = parser.parse_args(argv)
    if not args.execute:
        print(json.dumps({"status": "prepared_not_executed", "source": str(RUNS), "destination": str(PUBLIC),
                          "requires_closed_capture_and_ROOT_GO": True, "capture_files_read": 0,
                          "controller_sha256": CONTROLLER_SHA, "engine_runs": 0, "git_calls": 0}))
        return 0
    reader = Reader()
    for value in (args.expected_publisher_sha256, args.expected_receipt_sha256, args.expected_capture_sums_sha256):
        require(re.fullmatch("[0-9a-f]{64}", value or "") is not None, "three explicit SHA256 pins required")
    publisher = reader.pinned(Path(__file__), args.expected_publisher_sha256)
    controller = reader.pinned(CONTROLLER, CONTROLLER_SHA)
    checker = reader.pinned(CHECKER, CHECKER_SHA)
    C = module("full_lazy_capture_read_only", CONTROLLER, controller)
    V = module("receipt_publication_read_only", CHECKER, checker)
    require(not PUBLIC.exists() and not PUBLIC.is_symlink(), "public destination exists; no overwrite")
    classification, payload, capture_names = preflight(reader, C, args)
    payload.update({"protocol/capture.py": controller, "protocol/publish.py": publisher,
                    "protocol/check_v7_receipt_publication.py": checker,
                    "qualification.json": encode(classification), "README.md": readme(classification)})
    payload["provenance.json"] = encode({
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "capture_receipt_sha256": args.expected_receipt_sha256,
        "capture_sums_sha256": args.expected_capture_sums_sha256,
        "controller_sha256": CONTROLLER_SHA, "publisher_sha256": args.expected_publisher_sha256,
        "checker_sha256": CHECKER_SHA, "copies_are_byte_exact": True,
        "sources_and_binaries": "before/after inventories only; no executable binary payload",
        "boost": "521 header hashes plus inert precheck dependency file; no Boost code",
        "old_prechecks_or_other_campaign_results_imported": False})
    require(all(safe_name(name) for name in payload), "unsafe public filename")
    require(not any(data.startswith(b"\x7fELF") for data in payload.values()), "binary payload forbidden")
    manifest = {name: {"bytes": len(data), "sha256": sha(data)} for name, data in sorted(payload.items())}
    payload["manifest.json"] = encode(manifest)
    payload["SHA256SUMS"] = "".join(sha(data) + "  " + name + "\n"
                                    for name, data in sorted(payload.items())).encode()
    # Use the exact index checker's pure verifier, NEVER its Git entrypoint.
    def fetch(path):
        prefix = PUBLIC_REL + "/"
        require(path.startswith(prefix), "verifier escaped the new receipt")
        name = path[len(prefix):]
        require(name in payload, "sealed member missing from payload")
        return payload[name]
    for name, data in payload.items():
        if name.endswith("SHA256SUMS"):
            V.verify_manifest(PUBLIC_REL + "/" + name, data, fetch)
    reader.recheck()
    recheck_live(reader, C, classification)
    require(files_in(RUNS) == capture_names, "capture gained/lost members at publication boundary")
    # No destination exists until ALL preflight checks and reclassification passed.
    PUBLIC.mkdir(mode=0o755)
    for name, raw in sorted(payload.items()):
        path = PUBLIC / name
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("xb") as stream:
            stream.write(raw)
        require(path.read_bytes() == raw, "public byte-exact copy failed")
    require(files_in(PUBLIC) == set(payload), "public file inventory differs")
    for name, raw in payload.items():
        require((PUBLIC / name).read_bytes() == raw, "terminal public drift")
    reader.recheck()
    recheck_live(reader, C, classification)
    print(json.dumps({"status": classification["status"], "published_files": len(payload),
                      "destination": str(PUBLIC), "sums_sha256": sha(payload["SHA256SUMS"]),
                      "git_index_verified": False, "git_index_check_is_ROOT_followup": True}))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError, KeyError, TypeError) as error:
        print("Publication refused: " + str(error))
        raise SystemExit(1)
