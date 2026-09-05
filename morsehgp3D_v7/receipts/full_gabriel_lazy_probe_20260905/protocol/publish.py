#!/usr/bin/env python3
"""Compact create-only FULL lazy probe publication; ROOT GO only.

Default is inert. No engine, compiler, Git/GCP call or mutation of captures.
The established Reader and index verifier are reused as pure pinned modules.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import types

ROOT = Path("/workspaces/E-HGP")
BASE = ROOT / "build/v7_full_lazy_20260905_probe_controller"
CONTROLLER = BASE / "capture.py"
CONTROLLER_SHA = "417ccc3b47bb7591405f3af99bf7591bf2019794aa4535077436ce4889c4adfa"
COMMON = ROOT / "build/v7_full_lazy_20260905_controller/publish.py"
COMMON_SHA = "5c7f18a2577ee388a8f9652c3596ffe9ab9ade6bbc3101ae45f18fc91da6dfba"
CHECKER = ROOT / "tools/check_v7_receipt_publication.py"
CHECKER_SHA = "32420385f487260e0706b3e649befca25cc95a9d45f17d22472c333870729580"
BUILD = BASE / "build_admission/receipt.json"
BUILD_SHA = "da11c743e63fb63acd00c25ce02080671a34a43fa2fa9cd473bb3068df2712a3"
MICRO = BASE / "micro_admission/receipt.json"
MICRO_SHA = "9ce369e2d6085e1e7ac0b95c03a84f1793f42d0107a2b2a15474644d880ce1b2"


def load(path, expected):
    import hashlib
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != expected:
        raise ValueError("protocol pin changed: " + str(path))
    module = types.ModuleType("inert_" + path.stem + "_" + expected[:8])
    module.__file__ = str(path)
    exec(compile(raw, str(path), "exec"), module.__dict__)
    return module


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", action="store_true")
    parser.add_argument("--kind", choices=("admission", "heavy"), required=True)
    parser.add_argument("--expected-publisher-sha256")
    parser.add_argument("--heavy-receipt", action="append", default=[], metavar="PATH=SHA256")
    args = parser.parse_args(argv)
    if not args.execute:
        print("prepared_not_executed: no capture read, no export, ROOT GO required")
        return 0
    H = load(COMMON, COMMON_SHA)
    C = load(CONTROLLER, CONTROLLER_SHA)
    V = load(CHECKER, CHECKER_SHA)
    require = H.require
    require(re.fullmatch("[0-9a-f]{64}", args.expected_publisher_sha256 or "") is not None, "publisher pin required")
    require((args.kind == "heavy") == bool(args.heavy_receipt), "heavy receipts required only for heavy mode")
    reader, payload, watches, omissions = H.Reader(), {}, [], []
    protocols = {"publish.py": (Path(__file__), args.expected_publisher_sha256),
                 "capture.py": (CONTROLLER, CONTROLLER_SHA), "publication_common.py": (COMMON, COMMON_SHA),
                 "check_v7_receipt_publication.py": (CHECKER, CHECKER_SHA),
                 "source_probe.cpp": (ROOT / C.PROBE, C.PINS[C.PROBE]),
                 "semantic_digest.hpp": (ROOT / C.DIGEST, C.PINS[C.DIGEST]),
                 "probe_audit.py": (ROOT / C.JUDGE, C.PINS[C.JUDGE])}
    for name, (path, pin) in protocols.items():
        payload["protocol/" + name] = reader.pinned(path, pin)
    public_rel = "morsehgp3D_v7/receipts/" + (
        "full_gabriel_lazy_probe_20260905" if args.kind == "admission" else "full_gabriel_lazy_mono_20260905")
    public = ROOT / public_rel
    require(not public.exists() and not public.is_symlink(), "create-only destination exists")

    def collect(path, pin, kind, prefix):
        path = Path(path)
        require(path.is_absolute() and path.name == "receipt.json" and
                path.resolve().is_relative_to(BASE) and not path.is_symlink(), "receipt scope")
        receipt = H.json_value(reader.pinned(path, pin))
        require(receipt.get("schema") == C.SCHEMA and receipt.get("kind") == kind and
                receipt.get("status") in ("completed", "failed") and receipt.get("ended"), "unclosed receipt")
        if receipt["status"] == "completed":
            C.verify_receipt(path, pin, kind)  # Exact closed readonly controller, no probe.
        else:
            require(kind == "heavy", "failed admission cannot be published as admitted")
        names = H.files_in(path.parent)
        require(names == set(receipt["artifacts"]) | {"receipt.json"}, "exact closed artifact inventory")
        watches.append((path, pin, kind, receipt["status"], names))
        payload[prefix + "/receipt.json"] = reader.get(path)
        for name, expected in receipt["artifacts"].items():
            require(H.safe_name(name), "unsafe artifact path")
            source = path.parent / name
            if "binary" in receipt and source == ROOT / receipt["binary"]:
                require(C.sha(source) == expected == receipt["binary_sha256"], "omitted binary drift")
                omissions.append({"path": str(source), "sha256": expected, "bytes": source.stat().st_size,
                                  "reason": "executable_not_exported_pin_retained_in_original_receipt"})
                continue
            raw = reader.pinned(source, expected)
            require(not raw.startswith(b"\x7fELF"), "unexpected binary artifact")
            payload[prefix + "/" + name] = raw
        return receipt

    # Admissions are always rechecked, but are only copied in their dedicated
    # packet. Heavy packets refer to these public admissions without duplication.
    C.verify_receipt(BUILD, BUILD_SHA, "build")
    C.verify_receipt(MICRO, MICRO_SHA, "micro")
    observations = []
    if args.kind == "admission":
        build = collect(BUILD, BUILD_SHA, "build", "build")
        micro = collect(MICRO, MICRO_SHA, "micro", "micro")
        require(micro["build_receipt"] == str(BUILD) and micro["build_receipt_sha256"] == BUILD_SHA and
                micro["binary_sha256"] == build["binary_sha256"] and micro["parser_rejects"] == 11 and
                len(micro["attempts"]) == 24 and all(a["status"] == "completed" and
                a["attempt_success"] is True for a in micro["attempts"]), "micro admission counts/binding")
        orders = 0
        for verdict in micro["attempts"]:
            label = verdict["attempt_id"]
            match = re.fullmatch(r"n8_s(8|10|12)_k(5|10)_(eager|lazy)_c(0|1|1000000)", label)
            require(match is not None, "micro attempt identity")
            section = "k" + match[2]
            recorded = H.json_value(reader.get(MICRO.parent / section / (label + ".receipt.json")))
            require(verdict == H.json_value(reader.get(MICRO.parent / section / (label + ".verdict.json"))),
                    "micro verdict differs from same attempt")
            orders += len(recorded["orders"])
        require(orders == 156, "micro horizontal order count")
        for mode in ("normal", "optimized"):
            result = H.json_value(reader.get(MICRO.parent / "k10" /
                ("n8_s8_k10_eager_c0.selftest_" + mode + ".stdout")))
            require(result["audit_status"] == "selftests_passed" and
                    len(result["mutants_killed"]) == len(set(result["mutants_killed"])) == 19, "19 mutants per judge mode")
        observations.append({"kind": "micro", "status": "completed", "successful_attempts": 24,
                             "horizontal_orders": orders, "parser_rejects": 11, "mutants_per_mode": 19,
                             "modes": ["normal", "optimized"], "binary_sha256": build["binary_sha256"]})
    else:
        phases = set()
        for argument in args.heavy_receipt:
            path, separator, pin = argument.rpartition("=")
            require(separator and re.fullmatch("[0-9a-f]{64}", pin), "heavy PATH=SHA256 required")
            record = H.json_value(reader.pinned(Path(path), pin))
            phase = record.get("phase")
            require(phase in ("paired", "scale16", "scale32") and phase not in phases, "heavy phase inventory")
            phases.add(phase)
            record = collect(path, pin, "heavy", phase)
            observations.append({key: record.get(key) for key in
                ("kind", "phase", "status", "reason", "all_successful", "semantic_digests_equal", "attempts")})
        require("paired" in phases, "mono packet must include the closed paired observation")
    summary = {"kind": args.kind, "status": "closed_captures_published_not_SLO",
               "observations": observations, "omissions": omissions,
               "build_receipt_sha256": BUILD_SHA, "micro_receipt_sha256": MICRO_SHA,
               "controller_sha256": CONTROLLER_SHA, "public_status": "not_claimed",
               "scope": "horizontal_relative_orders_not_integrated_inter_k_tower",
               "semantic_digest_is_not_geometry_or_catalogue_completeness_proof": True,
               "engine_reexecuted": False, "gcp_used": False}
    payload["publication.json"] = H.encode(summary)
    lines = ["# Sonde FULL lazy — " + ("admission locale" if args.kind == "admission" else "observations mono"),
             "", "5 septembre 2026. public_status=not_claimed ; CPU de référence, entrée u16.", "",
             ("Admission close : 24 petites exécutions réussies, 156 ordres horizontaux, 11 refus parser ; "
              "19 mutants du juge tués en mode normal et sous Python -O." if args.kind == "admission" else
              "Captures lourdes closes, y compris les refus, censures ou échecs présents. "
              "Un statut completed du contrôleur ne signifie pas que tous les moteurs ont réussi."), "",
             "Octets copiés intégralement, sauf l’exécutable explicitement omis : son SHA reste dans le reçu "
             "de compilation original et dans publication.json. Aucun ELF ni code Boost exporté.", "",
             "Les protocoles sous protocol/ sont inertes et épinglés. Les cartes de sources avant/après, "
             "MDD et commandes sont conservées ; le build reste local et non hermétique.", "",
             "Les digests comparent des forêts horizontales étiquetées, pas la complétude géométrique "
             "des catalogues. Cette admission n’est ni la tour inter-K intégrée, ni un résultat 50k "
             "en une seconde/100 ms, ni un benchmark massif G4.", "",
             "[Captures et statuts](publication.json), [inventaire](manifest.json), [sommes](SHA256SUMS). "
             "GCP non utilisé.", ""]
    payload["README.md"] = "\n".join(lines).encode()
    payload["manifest.json"] = H.encode({name: {"bytes": len(raw), "sha256": H.sha(raw)}
                                        for name, raw in sorted(payload.items())})
    payload["SHA256SUMS"] = "".join(H.sha(raw) + "  " + name + "\n"
                                    for name, raw in sorted(payload.items())).encode()
    def fetch(name):
        require(name.startswith(public_rel + "/"), "checker escaped packet")
        return payload[name[len(public_rel) + 1:]]
    V.verify_manifest(public_rel + "/SHA256SUMS", payload["SHA256SUMS"], fetch)
    def terminal_check():
        reader.recheck()
        C.verify_receipt(BUILD, BUILD_SHA, "build")
        C.verify_receipt(MICRO, MICRO_SHA, "micro")
        for path, pin, kind, status, names in watches:
            require(H.files_in(path.parent) == names, "capture inventory drift")
            if status == "completed":
                C.verify_receipt(path, pin, kind)
    terminal_check()
    public.mkdir()
    for name, raw in sorted(payload.items()):
        target = public / name
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open("xb") as stream:
            stream.write(raw)
        require(target.read_bytes() == raw, "copy changed bytes")
    require(H.files_in(public) == set(payload), "public inventory differs")
    terminal_check()
    print("published", public, "files", len(payload), "SHA256SUMS", H.sha(payload["SHA256SUMS"]),
          "git_index_check_pending_ROOT")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError, RuntimeError, KeyError, TypeError) as error:
        print("Publication refused:", error)
        raise SystemExit(1)

