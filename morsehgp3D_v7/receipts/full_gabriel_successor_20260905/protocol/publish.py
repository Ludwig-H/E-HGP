#!/usr/bin/env python3
"""Create-only successor qualification publication; separate ROOT GO required.

Default is inert. All qualification/development bytes are copied and hashed.
Completed primary evidence is revalidated in read-only mode; failed development
captures remain separate, never repaired or reused as successful qualification.
No engine, compilation, subprocess, Git or GCP call is made.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import types

ROOT = Path("/workspaces/E-HGP")
BASE = ROOT / "build/v7_successor_20260905_controller"
PUBLIC_REL = "morsehgp3D_v7/receipts/full_gabriel_successor_20260905"


def identity(argument):
    name, separator, pin = (argument or "").rpartition("=")
    path = Path(name)
    if not (separator and re.fullmatch("[0-9a-f]{64}", pin) and path.is_absolute() and
            path.name == "receipt.json" and path.resolve().is_relative_to(BASE) and not path.is_symlink()):
        raise ValueError("closed controller receipt PATH=SHA256 required")
    return path, pin


def load(path, pin):
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != pin:
        raise ValueError("protocol SHA256 mismatch: " + str(path))
    module = types.ModuleType("inert_successor_publication_" + pin[:8])
    module.__file__ = str(path)
    exec(compile(raw, str(path), "exec"), module.__dict__)
    return module


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", action="store_true")
    parser.add_argument("--expected-publisher-sha256")
    parser.add_argument("--expected-controller-sha256")
    parser.add_argument("--qualification", metavar="PATH=SHA256")
    parser.add_argument("--development", action="append", default=[], metavar="PATH=SHA256")
    parser.add_argument("--failed-qualification", action="append", default=[], metavar="PATH=SHA256")
    args = parser.parse_args(argv)
    if not args.execute:
        print("prepared_not_executed: no capture read or publication; separate ROOT GO required")
        return 0
    for pin in (args.expected_publisher_sha256, args.expected_controller_sha256):
        if re.fullmatch("[0-9a-f]{64}", pin or "") is None:
            raise ValueError("publisher/controller reviewed SHA256 pins required")
    primary, primary_pin = identity(args.qualification)
    # The captured controller finds its three pinned modules next to itself.
    # Reproduction does not require the earlier private controller directories.
    N = load(primary.parent / "protocol/capture.py", args.expected_controller_sha256)
    H, _, common_raw = N.imported("publication_common.py", N.COMMON, N.COMMON_SHA)
    V, _, checker_raw = N.imported("check_v7_receipt_publication.py", N.CHECKER, N.CHECKER_SHA)
    require, reader, payload, watches = H.require, H.Reader(), {}, []
    public = ROOT / PUBLIC_REL
    require(not public.exists() and not public.is_symlink(), "create-only publication exists")
    payload["protocol/publish.py"] = reader.pinned(Path(__file__), args.expected_publisher_sha256)

    def collect(path, pin, prefix, development):
        record = H.json_value(reader.pinned(path, pin))
        require(record.get("schema") == N.SCHEMA and record.get("kind") == "qualification" and
                record.get("status") in ("completed", "failed") and record.get("ended_utc") and
                record.get("public_status") == "not_claimed" and record.get("gcp_used") is False and
                record.get("historical_results_reused") is False and record.get("hermetic") is False,
                "capture not closed or scope differs")
        require(record.get("development") is development, "development/qualification lane mismatch")
        names = H.files_in(path.parent)
        require(names == set(record["artifacts"]) | {"receipt.json"}, "exact capture inventory")
        payload[prefix + "/receipt.json"] = reader.get(path)
        for name, expected in record["artifacts"].items():
            require(H.safe_name(name), "unsafe capture path")
            raw = reader.pinned(path.parent / name, expected)
            require(not raw.startswith(b"\x7fELF"), "ELF/object payload forbidden")
            payload[prefix + "/" + name] = raw
        watches.append((path.parent, names))
        return record

    record = collect(primary, primary_pin, "qualification", False)
    require(record["controller_sha256"] == args.expected_controller_sha256, "primary controller binding")
    require(record.get("build_tag") == "" or re.fullmatch("[a-z0-9][a-z0-9_-]{0,63}", record.get("build_tag", "")),
            "build tag syntax")
    C = N.context(primary.parent, record["build_tag"])
    require(record["tests"] == H.json_value(C.encoded(C.TESTS)) and record["targets"] == N.targets(C) and
            len(C.TESTS) == 20 and len(N.targets(C)) == 8, "primary target/test inventory")
    require(reader.pinned(primary.parent / "protocol/capture.py", args.expected_controller_sha256) and
            reader.pinned(primary.parent / "protocol/legacy_capture.py", N.LEGACY_SHA) and
            reader.pinned(primary.parent / "protocol/publication_common.py", N.COMMON_SHA) == common_raw and
            reader.pinned(primary.parent / "protocol/check_v7_receipt_publication.py", N.CHECKER_SHA) == checker_raw,
            "co-located protocol binding")
    phase_summaries = []

    def value(name):
        return H.json_value(reader.get(primary.parent / name))

    before = value("sources_before.json") if "sources_before.json" in record["artifacts"] else None
    successful = record["status"] == "completed"
    if successful:
        require(before is not None and C.sha(C.encoded(before)) == record["source_sha256"], "source-map admission SHA")
        require(record["producer_sha256"] == before["morsehgp3D_v7/src/forest/full_gabriel.hpp"], "producer pin binding")
        require(record["errors"] == [] and [row["mode"] for row in record["phases"]] == ["release", "san"],
                "primary phase closure")
        require(value("sources_after.json") == before == N.sources(C), "primary source drift")
        require(value("toolchain_before.json") == value("toolchain_after.json") == C.tool_snapshot(), "toolchain drift")
        require(value("host_before.json")["process_status"]["TracerPid"] == "0", "traced successful capture")
        require(value("freshness.json")["builds"] == {str(path): "absent" for path in C.BUILDS.values()}, "fresh build admission")
        for reported in record["phases"]:
            mode, build = reported["mode"], C.BUILDS[reported["mode"]]
            require(value(mode + "/summary.json") == reported and reported["status"] == "completed" and reported["errors"] == [],
                    "completed phase summary")
            require(all(reported[key] is True for key in ("sources_stable", "binaries_stable", "compile_binding_stable", "toolchain_stable")),
                    "phase stability gates")
            require(value(mode + "/sources_after.json") == before and reported["source_sha256"] == record["source_sha256"],
                    "phase source binding")
            binaries = value(mode + "/binaries.json")
            binding = value(mode + "/compile_binding.json")
            require(binaries == value(mode + "/binaries_after.json") == N.binaries(build, C), "eight binaries changed")
            require(binding == N.binding(build, mode, before, C) and value(mode + "/compile_binding_after_sha256.json") ==
                    {"sha256": C.sha(C.encoded(binding))}, "flags/dependency/object binding changed")
            require(value(mode + "/pre_ctest_binding.json") == {"source_sha256": record["source_sha256"],
                    "binary_map_sha256": C.sha(C.encoded(binaries)), "compile_binding_sha256": C.sha(C.encoded(binding))},
                    "pre-CTest binding")
            commands = N.commands(C, mode)
            require(len(commands) == len(reported["commands"]) == 4, "phase command inventory")
            for (label, argv, limit), command in zip(commands, reported["commands"]):
                require(command == value(mode + "/" + label + ".command.json") and command["argv"] == argv and
                        command["cwd"] == str(ROOT) and command["environment"] == C.environment(mode) and
                        command["exit_code"] == command["expected_code"] == 0 and command["status"] == "completed" and
                        command["error"] is None and command["timeout_seconds"] == limit, "exact command/exit/environment")
                intent = value(mode + "/" + label + ".intent.json")
                require(all(command[key] == val for key, val in intent.items()), "command intent mirror")
                for stream in ("stdout", "stderr"):
                    raw = reader.get(primary.parent / mode / (label + "." + stream))
                    require(command["raw"][stream]["sha256"] == H.sha(raw) and command["raw"][stream]["bytes"] == len(raw),
                            "command stdout/stderr exact bytes")
            inventory = N.inventory(reader.get(primary.parent / mode / "inventory.stdout"), build, C)
            require(inventory == reported["inventory"], "twenty-test inventory replay")
            outputs = value(mode + "/test_outputs.json")
            raw_log = reader.get(primary.parent / mode / "LastTest.stdout")
            raw_junit = reader.get(primary.parent / mode / "JUnit.stdout")
            require(outputs == reported["test_outputs"] and outputs["fresh"] is True and
                    outputs["last_test"] == C.judge_log(raw_log, sorted(C.TESTS)) and
                    outputs["junit"] == N.junit(raw_junit, C), "fresh JUnit/LastTest replay")
            require(raw_junit == reader.get(primary.parent / mode / "ctest.junit.xml"), "JUnit copy differs")
            fence = value(mode + "/ctest.fence.json")["source"]
            require(C.read_stable(Path(fence["path"]))[1] == fence, "owned CTest freshness fence changed")
            for copy in outputs["copies"].values():
                source, archived = copy["source"], copy["archive"]
                raw = reader.get(Path(archived["path"]))
                require(source["sha256"] == archived["sha256"] == H.sha(raw) and source["device"] == fence["device"] and
                        source["mtime_ns"] >= fence["mtime_ns"] and source["ctime_ns"] >= fence["ctime_ns"] and
                        C.read_stable(Path(source["path"]))[1] == source and C.read_stable(Path(archived["path"]))[1] == archived,
                        "test output byte/freshness metadata changed")
            phase_summaries.append({"mode": mode, "status": "revalidated_20_targeted_tests", "tests": 20, "binaries": 8,
                                    "build_seconds": reported["commands"][1]["elapsed_seconds"],
                                    "ctest_seconds": reported["commands"][3]["elapsed_seconds"]})
        for mode in ("normal", "optimized"):
            observed = value("metadata_selftest_" + mode + ".stdout")
            require(observed == N.selftest(C), "metadata model selftests differ from pure replay")
    else:
        phase_summaries = [{"mode": row["mode"], "status": "failed_capture_not_requalified"} for row in record["phases"]]
    development = []
    seen = {primary}
    for index, argument in enumerate(args.development):
        path, pin = identity(argument)
        require(path not in seen, "duplicate development capture")
        seen.add(path)
        item = collect(path, pin, f"development/{index:02d}_{path.parent.name}", True)
        development.append({"path": str(path), "sha256": pin, "captured_status": item["status"],
                            "not_imported_as_qualification": True})
    failed_qualification = []
    for index, argument in enumerate(args.failed_qualification):
        path, pin = identity(argument)
        require(path not in seen, "duplicate failed qualification capture")
        seen.add(path)
        item = collect(path, pin, f"failed_qualification/{index:02d}_{path.parent.name}", False)
        require(item["status"] == "failed", "failed qualification cannot promote a capture")
        failed_qualification.append({"path": str(path), "sha256": pin, "captured_status": "failed",
                                     "not_imported_as_qualification": True})
    summary = {"schema": "mhgp7-successor-publication-v1", "status": "qualified_targeted" if successful else "failed_primary_preserved",
               "capture_receipt_sha256": primary_pin, "controller_sha256": args.expected_controller_sha256,
               "publisher_sha256": args.expected_publisher_sha256, "producer_sha256": record["producer_sha256"],
               "phases": phase_summaries, "development": development,
               "failed_qualification": failed_qualification, "public_status": "not_claimed",
               "historical_results_reused": False, "whole_suite_qualified": False, "gcp_used": False}
    payload["publication.json"] = H.encode(summary)
    payload["README.md"] = ("# Qualification ciblée FULL de la normalisation des successeurs\n\n"
        "5 septembre 2026. public_status=not_claimed ; CPU de référence, entrée u16.\n\n" +
        ("Deux constructions neuves : 20/20 CTests Release et 20/20 ASan/UBSan, huit binaires par build. "
         "LeakSanitizer reste actif (detect_leaks=1), sans override ni désactivation.\n\n" if successful else
         "Capture principale échouée conservée ; aucune qualification globale 20/20 ×2 n’est revendiquée.\n\n") +
        "Les six binaires produits restent sans macro testing ; les deux portes singleton et successor portent "
        "MHGP7_TESTING=1. Filtre de labels exact, argv et codes attendus contrôlés ; pas de suite F importée. "
        "Les deux juges de métadonnées ont des modèles distincts : deux positifs et douze mutants, "
        "sans les confondre avec des résultats moteur.\n\n"
        "Les bruts complets, JUnit, LastTest, commandes, options, dépendances, sources et pins des binaires "
        "figurent sous [qualification/](qualification/). Les éventuelles captures de développement restent "
        "séparées et ne sont jamais promues en qualification fraîche. Les qualifications initiales échouées "
        "sont conservées sous failed_qualification/, avec leurs champs et sceaux originaux inchangés. "
        "Aucun ELF, objet compilé ni code Boost "
        "n’est exporté ; les modules Python importés sont copiés octet pour octet et co-localisés sous "
        "qualification/protocol/. Le publisher est sous protocol/.\n\n"
        "Provenance bornée : HEAD est éventuellement déclaré par ROOT, pas authentifié ici ; le worktree "
        "est identifié par la carte des sources effectivement qualifiées, pas par un statut Git global. "
        "Build frais mais non hermétique : headers système listés, non tous pré-épinglés. L’ancien depfile "
        "Boost sert seulement à pré-épingler les 521 headers consommés, jamais un ancien résultat.\n\n"
        "Aucun gain de performance, contrat 50k/1 s/100 ms, verticale inter-K ou résultat massif G4 ne découle "
        "de ces tests. [Statut et références](publication.json), [inventaire](manifest.json), "
        "[sommes](SHA256SUMS). GCP non utilisé.\n").encode()
    payload["manifest.json"] = H.encode({name: {"bytes": len(raw), "sha256": H.sha(raw)} for name, raw in sorted(payload.items())})
    payload["SHA256SUMS"] = "".join(H.sha(raw) + "  " + name + "\n" for name, raw in sorted(payload.items())).encode()

    def fetch(path):
        require(path.startswith(PUBLIC_REL + "/"), "manifest checker escaped packet")
        return payload[path[len(PUBLIC_REL) + 1:]]

    V.verify_manifest(PUBLIC_REL + "/SHA256SUMS", payload["SHA256SUMS"], fetch)
    reader.recheck()
    for directory, names in watches:
        require(H.files_in(directory) == names, "closed capture inventory drift")
    if successful:
        require(N.sources(C) == before and C.tool_snapshot() == value("toolchain_before.json"), "publication source/tool drift")
        for mode in ("release", "san"):
            require(N.binaries(C.BUILDS[mode], C) == value(mode + "/binaries.json") and
                    N.binding(C.BUILDS[mode], mode, before, C) == value(mode + "/compile_binding.json"), "publication binary/binding drift")
    public.mkdir()
    for name, raw in sorted(payload.items()):
        path = public / name
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open("xb") as stream:
            stream.write(raw)
        require(path.read_bytes() == raw, "public copy differs")
    reader.recheck()
    require(H.files_in(public) == set(payload), "public inventory differs")
    print("published", public, "files", len(payload), "SHA256SUMS", H.sha(payload["SHA256SUMS"]), "git_index_check_pending_ROOT")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError, KeyError, TypeError, IndexError) as error:
        print("Successor publication refused:", error)
        raise SystemExit(1)
