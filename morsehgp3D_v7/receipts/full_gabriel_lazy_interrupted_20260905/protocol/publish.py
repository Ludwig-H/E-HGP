#!/usr/bin/env python3
"""Create-only archive of the interrupted first FULL lazy paired campaign.

Default invocation is inert. ROOT must review this file and pass --execute with
the exact publisher pin. No subprocess, engine, build, Git or GCP call is made.
The closed frozen controller receipt is copied, never repaired or rewritten.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import types

ROOT = Path("/workspaces/E-HGP")
BASE = ROOT / "build/v7_full_lazy_20260905_probe_controller"
CAPTURE = BASE / "heavy_paired"
PUBLIC_REL = "morsehgp3D_v7/receipts/full_gabriel_lazy_interrupted_20260905"
PUBLIC = ROOT / PUBLIC_REL
RECEIPT_SHA = "3b240de86626bc7824ba4d19c099304c25a1f57be97f8b95fc85efaf810a7a2a"
COMMON = ROOT / "build/v7_full_lazy_20260905_controller/publish.py"
COMMON_SHA = "5c7f18a2577ee388a8f9652c3596ffe9ab9ade6bbc3101ae45f18fc91da6dfba"
CONTROLLER = BASE / "capture.py"
CONTROLLER_SHA = "417ccc3b47bb7591405f3af99bf7591bf2019794aa4535077436ce4889c4adfa"
CHECKER = ROOT / "tools/check_v7_receipt_publication.py"
CHECKER_SHA = "32420385f487260e0706b3e649befca25cc95a9d45f17d22472c333870729580"
JUDGE = ROOT / "morsehgp3D_v7/bench/full_gabriel_lazy_probe_audit.py"
JUDGE_SHA = "8d8a612aa973cb79e60e97a6675f63684ddd8892cfc550716c20620c4d6930ef"
BUILD_SHA = "da11c743e63fb63acd00c25ce02080671a34a43fa2fa9cd473bb3068df2712a3"
MICRO_SHA = "9ce369e2d6085e1e7ac0b95c03a84f1793f42d0107a2b2a15474644d880ce1b2"
EAGER = "n8000_s8_k10_eager_c0"
LAZY = "n8000_s8_k10_lazy_c1000000"


def load(path, pin):
    import hashlib
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != pin:
        raise ValueError("protocol SHA256 mismatch: " + str(path))
    result = types.ModuleType("inert_recovery_" + path.stem + "_" + pin[:8])
    result.__file__ = str(path)
    exec(compile(raw, str(path), "exec"), result.__dict__)
    return result


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", action="store_true")
    parser.add_argument("--expected-publisher-sha256")
    args = parser.parse_args(argv)
    if not args.execute:
        print("prepared_not_executed: no capture read or publication; ROOT GO required")
        return 0
    H = load(COMMON, COMMON_SHA)
    C = load(CONTROLLER, CONTROLLER_SHA)
    J = load(JUDGE, JUDGE_SHA)
    V = load(CHECKER, CHECKER_SHA)
    require, reader = H.require, H.Reader()
    require(re.fullmatch("[0-9a-f]{64}", args.expected_publisher_sha256 or "") is not None,
            "explicit publisher SHA256 required")
    require(not PUBLIC.exists() and not PUBLIC.is_symlink(), "create-only destination exists")
    payload = {}
    for name, path, pin in (
        ("publish.py", Path(__file__), args.expected_publisher_sha256),
        ("capture.py", CONTROLLER, CONTROLLER_SHA),
        ("publication_common.py", COMMON, COMMON_SHA),
        ("probe_audit.py", JUDGE, JUDGE_SHA),
        ("check_v7_receipt_publication.py", CHECKER, CHECKER_SHA),
    ):
        payload["protocol/" + name] = reader.pinned(path, pin)

    receipt = H.json_value(reader.pinned(CAPTURE / "receipt.json", RECEIPT_SHA))
    names = H.files_in(CAPTURE)
    require(names == set(receipt["artifacts"]) | {"receipt.json"}, "closed inventory differs")
    require(receipt.get("schema") == C.SCHEMA and receipt.get("kind") == "heavy" and
            receipt.get("phase") == "paired" and receipt.get("status") == "failed" and
            receipt.get("all_successful") is False and receipt.get("semantic_digests_equal") is False and
            receipt.get("sources_stable") is True and receipt.get("ended"), "unexpected closure status")
    require(receipt.get("reason", "").startswith("FileNotFoundError:") and
            LAZY + ".verdict.json" in receipt["reason"], "closure reason differs")
    payload["capture/receipt.json"] = reader.get(CAPTURE / "receipt.json")
    for name, pin in receipt["artifacts"].items():
        require(H.safe_name(name), "unsafe capture member")
        raw = reader.pinned(CAPTURE / name, pin)
        require(not raw.startswith(b"\x7fELF"), "binary payload forbidden")
        payload["capture/" + name] = raw

    def value(name):
        return H.json_value(reader.get(CAPTURE / name))

    plan = value("protocol.json")
    require(plan["planned_sequence"] == C.PAIRED and plan["kmax"] == 10 and
            plan["binary"] == receipt["binary"] and
            plan["binary_sha256"] == receipt["binary_sha256"], "paired protocol binding")
    baseline = value("sources_before.json")
    require(value("sources_after.json") == baseline and
            value(EAGER + ".sources_before.json") == baseline and
            value(EAGER + ".sources_after.json") == baseline and
            value(LAZY + ".sources_before.json") == baseline, "available source snapshots differ")
    admission = value("admission.json")
    require(admission["phase"] == "paired" and admission["paired"] is None and
            admission["micro_receipt"] == str(BASE / "micro_admission/receipt.json") and
            admission["micro_sha256"] == MICRO_SHA, "historical admission binding")

    eager = value(EAGER + ".receipt.json")
    verdict = value(EAGER + ".verdict.json")
    require(receipt["attempts"] == [verdict] and verdict["status"] == "completed" and
            verdict["attempt_success"] is True and eager["exit_code"] == 0, "eager completion differs")
    command = value(EAGER + ".command.json")
    require(all(eager[key] == val for key, val in command.items()), "eager command mirror differs")
    raw_eager = reader.get(CAPTURE / (EAGER + ".raw.txt"))
    require(command["streams"] == {EAGER + ".raw.txt": {
        "bytes": len(raw_eager), "sha256": H.sha(raw_eager)}}, "eager raw stream binding")
    judged = J.judge(raw_eager.decode(), eager, value(EAGER + ".intent.json"), plan)
    judged["raw_sha256"] = H.sha(raw_eager)
    require(judged["attempt_success"] is True and judged["orders_complete"] == 10,
            "eager receipt revalidation failed")
    for mode in ("normal", "optimized"):
        require(value(EAGER + ".judge_" + mode + ".stdout") == judged,
                "recorded eager judge differs from read-only replay")
    lazy_names = {name for name in names if name.startswith(LAZY + ".")}
    require(lazy_names == {LAZY + suffix for suffix in
                          (".intent.json", ".raw.txt", ".sources_before.json")},
            "interrupted lazy inventory is no longer exactly the known three files")
    raw_lazy = reader.get(CAPTURE / (LAZY + ".raw.txt"))
    lines = raw_lazy.decode().splitlines()
    require(len(lines) == 1, "interrupted raw contains more than configuration")
    config = H.json_value(lines[0].encode())
    require(config.get("type") == "configuration" and config.get("n") == 8000 and
            config.get("s") == 8 and config.get("kmax_requested") == 10 and
            config.get("alias_policy") == "lazy_first_c_strict_resolutions_v1" and
            config.get("cache_entries") == 1000000, "interrupted configuration identity")
    lazy_intent = value(LAZY + ".intent.json")
    require(lazy_intent["id"] == LAZY and lazy_intent["argv"] ==
            value(EAGER + ".intent.json")["argv"][:-1] +
            ["--alias-policy=lazy", "--cache-entries=1000000"], "lazy command identity")
    require(not any(key in lazy_intent for key in
                    ("ended", "elapsed_seconds", "exit_code", "status", "terminal", "orders")),
            "intent unexpectedly contains a terminal result")
    unattempted = []
    for n, s, policy, cache in C.PAIRED[2:]:
        label = f"n{n}_s{s}_k10_{policy}_c{cache}"
        require(not any(name.startswith(label + ".") for name in names), "later attempt exists")
        unattempted.append(label)
    fields = dict(line.strip().split(": ", 1) for line in raw_eager.decode().splitlines()
                  if not line.startswith("{"))
    summary = {
        "schema": "mhgp7-full-lazy-interrupted-publication-v1",
        "status": "failed_paired_campaign_preserved_without_repair",
        "capture_receipt_sha256": RECEIPT_SHA,
        "controller_closed_at": receipt["ended"],
        "closure_timestamp_is_not_lazy_process_end": True,
        "capture_status": "failed", "public_status": "not_claimed",
        "scope": "horizontal_relative_orders_not_integrated_inter_k_tower",
        "eager": {"attempt_id": EAGER, "status": "completed_unpaired_observation",
                  "exit_code": 0, "orders_complete_relative": 10,
                  "elapsed_before_terminal_ms": eager["terminal"]["elapsed_before_terminal_ms"],
                  "max_rss_kib": int(fields["Maximum resident set size (kbytes)"]),
                  "input_digest": eager["terminal"]["input_digest"],
                  "certificate_digest": eager["terminal"]["certificate_digest"],
                  "receipt_revalidated_readonly": True},
        "lazy": {"attempt_id": LAZY, "status": "interrupted_capture_nonterminal",
                 "cause": "unknown", "started_from_intent": lazy_intent["started"],
                 "ended": None, "exit_code": None, "elapsed_seconds": None,
                 "terminal_status": None, "timeout_observed": None,
                 "raw_bytes": len(raw_lazy), "raw_sha256": H.sha(raw_lazy),
                 "captured_configuration_rows": 1, "captured_order_rows": 0,
                 "captured_terminal_rows": 0,
                 "missing_artifacts": [LAZY + suffix for suffix in
                                       (".command.json", ".receipt.json", ".verdict.json",
                                        ".sources_after.json")],
                 "closure_snapshot_is_not_missing_attempt_after_snapshot": True},
        "not_attempted": unattempted,
        "paired_comparison_available": False, "speedup_ratio": None,
        "resume_policy": "new_campaign_repeat_eager_same_binary_and_source_pins_no_cross_campaign_ratio",
        "engine_reexecuted": False, "capture_bytes_modified": False,
        "slo_claim": False, "gcp_used": False,
    }
    payload["publication.json"] = H.encode(summary)
    payload["README.md"] = ("# Première comparaison FULL lazy interrompue\n\n"
        "5 septembre 2026. public_status=not_claimed ; CPU de référence, entrée u16.\n\n"
        "La campagne initiale est close avec le statut failed : une réussite eager à n=8 000/s=8 "
        "est conservée, suivie d’une capture lazy non terminale. Le fichier lazy ne contient que "
        "la configuration ; son code de sortie, sa durée, sa fin et la cause de l’interruption "
        "restent inconnus. Le plafond prévu de 600 s n’est pas un timeout observé.\n\n"
        "La date de clôture du contrôleur est administrative : elle ne date pas la fin du processus "
        "lazy. La carte de sources prise à cette clôture ne remplace pas sa carte après tentative absente. "
        "Aucun artefact terminal manquant n’est reconstruit. Les quatre tentatives suivantes "
        "n’ont aucun artefact de lancement dans cette campagne.\n\n"
        "Le passage eager reste une observation réussie non appariée : dix ordres horizontaux "
        "complets relativement aux catalogues fournis, 149 951,700395 ms jusqu’au terminal "
        "et 1 833 004 KiB de pic RSS GNU time. Le juge de reçus est rejoué en lecture seule ; "
        "aucun moteur n’est relancé par la publication.\n\n"
        "La reprise exige une campagne neuve répétant aussi eager, avec les mêmes pins de binaire "
        "et de sources. Aucun ratio entre l’ancien eager et un nouveau lazy n’est autorisé. "
        "Ni accélération lazy, ni contrat 50k/1 s/100 ms, ni résultat massif G4 ne sont établis ici.\n\n"
        "Tous les fichiers du répertoire clos sont copiés octet pour octet sous [capture/](capture/), "
        "sans omission, correction ou écrasement ; les protocoles sous protocol/ sont inertes. "
        "[Qualification de la capture](publication.json), [inventaire](manifest.json), "
        "[sommes](SHA256SUMS). GCP non utilisé.\n").encode()
    payload["manifest.json"] = H.encode({name: {"bytes": len(raw), "sha256": H.sha(raw)}
                                        for name, raw in sorted(payload.items())})
    payload["SHA256SUMS"] = "".join(H.sha(raw) + "  " + name + "\n"
                                    for name, raw in sorted(payload.items())).encode()

    def fetch(name):
        require(name.startswith(PUBLIC_REL + "/"), "manifest checker escaped packet")
        return payload[name[len(PUBLIC_REL) + 1:]]

    V.verify_manifest(PUBLIC_REL + "/SHA256SUMS", payload["SHA256SUMS"], fetch)

    def terminal_check():
        reader.recheck()
        require(H.files_in(CAPTURE) == names, "sealed capture inventory drift")
        C.verify_receipt(BASE / "build_admission/receipt.json", BUILD_SHA, "build")
        C.verify_receipt(BASE / "micro_admission/receipt.json", MICRO_SHA, "micro")
        require(C.snapshot(ROOT / plan["binary"]) == baseline, "current source/binary drift")

    terminal_check()
    PUBLIC.mkdir()
    for name, raw in sorted(payload.items()):
        target = PUBLIC / name
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open("xb") as stream:
            stream.write(raw)
        require(target.read_bytes() == raw, "public copy differs")
    require(H.files_in(PUBLIC) == set(payload), "public inventory differs")
    terminal_check()
    print("published", PUBLIC, "files", len(payload), "SHA256SUMS", H.sha(payload["SHA256SUMS"]),
          "git_index_check_pending_ROOT")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError, KeyError, TypeError) as error:
        print("Publication refused:", error)
        raise SystemExit(1)
