#!/usr/bin/env python3
"""Create-only normal/-O review capture, then separately authorized publication.

Inert unless --execute is passed. Capture requires ROOT's closed receipt pins;
publication requires another explicit call and the newly closed capture pin.
Only the pinned read-only aggregate runs, on CPU0, with 30 seconds per call.
No probe engine, compiler, Git or GCP command is invoked.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import re
import signal
import types

ROOT = Path("/workspaces/E-HGP")
BASE = ROOT / "build/v7_full_lazy_20260905_review_capture"
HEAVY_BASE = ROOT / "build/v7_full_lazy_20260905_probe_controller"
PUBLIC_REL = "morsehgp3D_v7/receipts/full_gabriel_lazy_mono_review_20260905"
HEAVY_PUBLIC = ROOT / "morsehgp3D_v7/receipts/full_gabriel_lazy_mono_20260905"
AGGREGATE = ROOT / "build/v7_full_lazy_20260905_recovery/aggregate.py"
AGGREGATE_SHA = "3cfeead56198035ae3b8b0c0c9bef6248b6aeef960ffbbd0904875dd55a92550"
COMMON = ROOT / "build/v7_full_lazy_20260905_controller/publish.py"
COMMON_SHA = "5c7f18a2577ee388a8f9652c3596ffe9ab9ade6bbc3101ae45f18fc91da6dfba"
CONTROLLER = HEAVY_BASE / "capture.py"
CONTROLLER_SHA = "417ccc3b47bb7591405f3af99bf7591bf2019794aa4535077436ce4889c4adfa"
CHECKER = ROOT / "tools/check_v7_receipt_publication.py"
CHECKER_SHA = "32420385f487260e0706b3e649befca25cc95a9d45f17d22472c333870729580"
JUDGE = ROOT / "morsehgp3D_v7/bench/full_gabriel_lazy_probe_audit.py"
JUDGE_SHA = "8d8a612aa973cb79e60e97a6675f63684ddd8892cfc550716c20620c4d6930ef"
POLICY = ROOT / "morsehgp3D_v7/bench/full_gabriel_cache_policy_audit.py"
POLICY_SHA = "8f8aed03755d9c92775566b21d4fdd9dcba31f171adf4b83e9802a988a450370"
SCHEMA = "mhgp7-full-lazy-readonly-review-capture-v1"


def load(path, pin):
    import hashlib
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != pin:
        raise ValueError("protocol SHA256 mismatch: " + str(path))
    result = types.ModuleType("inert_review_capture_" + path.stem + "_" + pin[:8])
    result.__file__ = str(path)
    exec(compile(raw, str(path), "exec"), result.__dict__)
    return result


def protocols(pin):
    return {"review.py": (Path(__file__), pin), "aggregate.py": (AGGREGATE, AGGREGATE_SHA),
            "capture_frozen.py": (CONTROLLER, CONTROLLER_SHA), "publication_common.py": (COMMON, COMMON_SHA),
            "probe_audit.py": (JUDGE, JUDGE_SHA), "cache_policy_audit.py": (POLICY, POLICY_SHA),
            "check_v7_receipt_publication.py": (CHECKER, CHECKER_SHA)}


def invocation(request, mode):
    argv = ["/usr/bin/taskset", "-c", "0", "/usr/bin/python3", "-B"]
    if mode == "optimized":
        argv.append("-O")
    argv += [str(AGGREGATE), "--execute", "--expected-script-sha256", AGGREGATE_SHA,
             "--paired", request["paired"]]
    for value in request["scales"]:
        argv += ["--scale", value]
    return argv


def reference(argument, H, reader):
    H.require(isinstance(argument, str), "PATH=SHA256 required")
    name, separator, pin = argument.rpartition("=")
    H.require(separator and re.fullmatch("[0-9a-f]{64}", pin), "PATH=SHA256 syntax")
    path = Path(name)
    H.require(path.is_absolute() and path.name == "receipt.json" and
              path.resolve().is_relative_to(HEAVY_BASE) and not path.is_symlink(), "source receipt scope")
    record = H.json_value(reader.pinned(path, pin))
    H.require(record.get("schema") == "mhgp7-full-lazy-probe-controller-v1" and
              record.get("kind") == "heavy" and record.get("status") in ("completed", "failed") and
              record.get("ended"), "source receipt is not closed")
    return {"path": str(path), "sha256": pin, "phase": record["phase"],
            "public_reference": str(HEAVY_PUBLIC / record["phase"] / "receipt.json")}


def capture(args, H, C):
    H.require(re.fullmatch("[a-z0-9][a-z0-9_-]{0,63}", args.id or ""), "capture id required")
    directory = BASE / ("run_" + args.id)
    H.require(not directory.exists() and not directory.is_symlink(), "create-only capture exists")
    reader = H.Reader()
    raw_protocols = {name: reader.pinned(path, pin)
                     for name, (path, pin) in protocols(args.expected_script_sha256).items()}
    references = [reference(value, H, reader) for value in [args.paired] + args.scale]
    H.require(references[0]["phase"] == "paired" and
              references[0]["path"] == str(HEAVY_BASE / "heavy_paired_resume/receipt.json"), "fresh paired receipt required")
    phases = [row["phase"] for row in references]
    H.require(len(phases) == len(set(phases)) and set(phases) <= {"paired", "scale16", "scale32"},
              "phase inventory")
    request = {"paired": args.paired, "scales": args.scale, "references": references,
               "aggregate_sha256": AGGREGATE_SHA, "timeout_seconds_each": 30,
               "child_environment": C.child_environment(), "cpu_affinity": [0]}
    tool_pins = {name: C.sha(Path(name).resolve()) for name in ("/usr/bin/taskset", "/usr/bin/python3")}
    reader.recheck()
    directory.mkdir()
    (directory / "protocol").mkdir()
    for name, raw in raw_protocols.items():
        with (directory / "protocol" / name).open("xb") as stream:
            stream.write(raw)
    C.save(directory / "request.json", request)
    C.save(directory / "tool_pins_before.json", tool_pins)
    result = {"schema": SCHEMA, "status": "failed", "started": C.now(), "commands": [],
              "references": references, "json_equal": False, "stdout_bytes_equal": False,
              "sources_stable": False, "public_status": "not_claimed", "engine_runs": 0, "gcp_used": False,
              "review_script_sha256": args.expected_script_sha256, "aggregate_sha256": AGGREGATE_SHA}
    try:
        for mode in ("normal", "optimized"):
            command = C.command(directory, mode, invocation(request, mode), (0,), 30)
            result["commands"].append(command)
            H.require(command["error"] is None, "interrupted review command:" + mode)
        H.require(all(row["status"] == "completed" and row["exit_code"] == 0 for row in result["commands"]),
                  "a review command failed")
        outputs = [(directory / (mode + ".stdout")).read_bytes() for mode in ("normal", "optimized")]
        values = [H.json_value(raw) for raw in outputs]
        H.require(all((directory / (mode + ".stderr")).stat().st_size == 0 for mode in ("normal", "optimized")),
                  "review stderr is not empty")
        H.require(all(value["schema"] == "mhgp7-full-lazy-mono-readonly-review-v1" and
                      value["status"] == "closed_evidence_reviewed" and value["script_sha256"] == AGGREGATE_SHA
                      for value in values), "review output schema/status")
        H.require(values[0] == values[1] and outputs[0] == outputs[1], "normal/-O review disagreement")
        result.update(status="completed", json_equal=True, stdout_bytes_equal=True,
                      reason="two_readonly_reviews_agree_not_engine_qualification")
    except BaseException as error:
        result.update(status="failed", reason=f"{type(error).__name__}: {error}")
    finally:
        try:
            reader.recheck()
            after = {name: C.sha(Path(name).resolve()) for name in tool_pins}
            C.save(directory / "tool_pins_after.json", after)
            H.require(after == tool_pins, "interpreter/taskset drift")
            result["sources_stable"] = True
        except BaseException as error:
            result.update(status="failed", reason=f"final_binding:{type(error).__name__}: {error}")
        result.update(ended=C.now(), artifacts={name: C.sha(directory / name) for name in sorted(H.files_in(directory))})
        C.save(directory / "receipt.json", result)
    print(H.encode({"status": result["status"], "receipt": str(directory / "receipt.json"),
                    "receipt_sha256": C.sha(directory / "receipt.json"), "publication_executed": False}).decode(), end="")
    return 0 if result["status"] == "completed" else 1


def publish(args, H, C):
    name, separator, pin = (args.capture_receipt or "").rpartition("=")
    H.require(separator and re.fullmatch("[0-9a-f]{64}", pin), "closed capture PATH=SHA256 required")
    path = Path(name)
    H.require(path.is_absolute() and path.name == "receipt.json" and path.resolve().is_relative_to(BASE) and
              not path.is_symlink(), "capture receipt scope")
    public = ROOT / PUBLIC_REL
    H.require(not public.exists() and not public.is_symlink(), "create-only public destination exists")
    reader = H.Reader()
    reader.pinned(Path(__file__), args.expected_script_sha256)
    record = H.json_value(reader.pinned(path, pin))
    H.require(record.get("schema") == SCHEMA and record.get("status") in ("completed", "failed") and
              record.get("ended") and record["review_script_sha256"] == args.expected_script_sha256 and
              record["aggregate_sha256"] == AGGREGATE_SHA and record["public_status"] == "not_claimed" and
              record["engine_runs"] == 0 and record["gcp_used"] is False,
              "capture is unclosed or from another wrapper")
    directory, names = path.parent, H.files_in(path.parent)
    H.require(names == set(record["artifacts"]) | {"receipt.json"}, "capture inventory differs")
    payload = {"capture/receipt.json": reader.get(path)}
    for name, expected in record["artifacts"].items():
        H.require(H.safe_name(name), "unsafe capture member")
        raw = reader.pinned(directory / name, expected)
        H.require(not raw.startswith(b"\x7fELF"), "binary payload forbidden")
        payload["capture/" + name] = raw
    for name, (source, expected) in protocols(args.expected_script_sha256).items():
        H.require(reader.pinned(source, expected) == reader.get(directory / "protocol" / name), "review protocol drift")
    request = H.json_value(reader.get(directory / "request.json"))
    H.require(record["references"] == request["references"], "source reference mirror")
    for row in record["references"]:
        H.require(row == reference(row["path"] + "=" + row["sha256"], H, reader), "source reference changed")
        reader.pinned(Path(row["public_reference"]), row["sha256"])
    good = record["status"] == "completed"
    if good:
        H.require(record["sources_stable"] is True and record["json_equal"] is True and
                  record["stdout_bytes_equal"] is True and len(record["commands"]) == 2, "review completion binding")
        H.require(H.json_value(reader.get(directory / "tool_pins_before.json")) ==
                  H.json_value(reader.get(directory / "tool_pins_after.json")), "review tool pins differ")
        H.require(request["child_environment"] == C.child_environment() and
                  request["timeout_seconds_each"] == 30 and request["cpu_affinity"] == [0], "review environment binding")
        outputs = []
        for mode, command in zip(("normal", "optimized"), record["commands"]):
            H.require(command == H.json_value(reader.get(directory / (mode + ".command.json"))) and
                      command["argv"] == invocation(request, mode) and command["exit_code"] == 0 and
                      command["status"] == "completed" and command["error"] is None and
                      command["outer_timeout_seconds"] == 30 and command["cwd"] == str(ROOT) and
                      command["expected_rc"] == [0] and command["capture"] == "separate_exact_bytes",
                      "review command binding")
            intent = H.json_value(reader.get(directory / (mode + ".intent.json")))
            H.require(all(command[key] == val for key, val in intent.items()), "review command intent mirror")
            for suffix in ("stdout", "stderr"):
                raw = reader.get(directory / (mode + "." + suffix))
                H.require(command["streams"][mode + "." + suffix] == {"bytes": len(raw), "sha256": H.sha(raw)},
                          "review raw binding")
            H.require(reader.get(directory / (mode + ".stderr")) == b"", "review stderr differs")
            outputs.append(reader.get(directory / (mode + ".stdout")))
        H.require(outputs[0] == outputs[1] and H.json_value(outputs[0]) == H.json_value(outputs[1]), "review outputs differ")
    payload["publication.json"] = H.encode({"schema": "mhgp7-full-lazy-mono-review-publication-v1",
        "status": "two_readonly_reviews_agree" if good else "failed_review_capture_preserved",
        "capture_receipt_sha256": pin, "review_script_sha256": args.expected_script_sha256,
        "aggregate_sha256": AGGREGATE_SHA, "references": record["references"],
        "heavy_raw_files_copied": 0, "engine_runs": 0, "public_status": "not_claimed", "gcp_used": False})
    output_links = ", ".join("[Sortie " + mode + " intégrale](capture/" + mode + ".stdout)"
                             for mode in ("normal", "optimized") if mode + ".stdout" in names)
    payload["README.md"] = ("# Contrelecture des observations mono FULL lazy\n\n"
        "5 septembre 2026. public_status=not_claimed ; CPU de référence, entrée u16.\n\n" +
        ("Les deux invocations en lecture seule, Python normal et -O, terminent avec le code 0. "
         "Leurs sorties JSON intégrales sont identiques octet pour octet.\n\n" if good else
         "La capture de revue a échoué ; ses octets et commandes sont conservés sans qualification.\n\n") +
        "La revue compare les reçus clos, les empreintes par ordre, les compteurs et la politique first-C. "
        "Un refus ou une capture interrompue du moteur restent tels quels même si leur revue réussit. "
        "Aucune nouvelle exécution moteur, aucun oracle géométrique ni résultat 50k/1 s/100 ms/G4.\n\n" +
        (output_links + ", " if output_links else "") +
        "[commandes et capture](capture/receipt.json), [protocoles inertes](capture/protocol/). "
        "Les bruts lourds ne sont pas dupliqués : [paquet source](../full_gabriel_lazy_mono_20260905/README.md), "
        "avec références exactes et SHA dans [publication.json](publication.json).\n\n"
        "[Inventaire](manifest.json), [sommes](SHA256SUMS). GCP non utilisé.\n").encode()
    payload["manifest.json"] = H.encode({name: {"bytes": len(raw), "sha256": H.sha(raw)}
                                        for name, raw in sorted(payload.items())})
    payload["SHA256SUMS"] = "".join(H.sha(raw) + "  " + name + "\n"
                                    for name, raw in sorted(payload.items())).encode()
    V = load(CHECKER, CHECKER_SHA)

    def fetch(name):
        H.require(name.startswith(PUBLIC_REL + "/"), "manifest checker escaped packet")
        return payload[name[len(PUBLIC_REL) + 1:]]

    V.verify_manifest(PUBLIC_REL + "/SHA256SUMS", payload["SHA256SUMS"], fetch)
    reader.recheck()
    H.require(H.files_in(directory) == names, "closed capture inventory drift")
    public.mkdir()
    for name, raw in sorted(payload.items()):
        target = public / name
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open("xb") as stream:
            stream.write(raw)
        H.require(target.read_bytes() == raw, "public copy differs")
    reader.recheck()
    H.require(H.files_in(directory) == names and H.files_in(public) == set(payload), "terminal inventory drift")
    print("published", public, "files", len(payload), "SHA256SUMS", H.sha(payload["SHA256SUMS"]),
          "git_index_check_pending_ROOT")
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--execute", action="store_true")
    parser.add_argument("--action", choices=("capture", "publish"))
    parser.add_argument("--expected-script-sha256")
    parser.add_argument("--id")
    parser.add_argument("--paired", metavar="PATH=SHA256")
    parser.add_argument("--scale", action="append", default=[], metavar="PATH=SHA256")
    parser.add_argument("--capture-receipt", metavar="PATH=SHA256")
    args = parser.parse_args(argv)
    if not args.execute:
        print("prepared_not_executed: no capture read, aggregate invocation or publication; ROOT GO required")
        return 0
    H, C = load(COMMON, COMMON_SHA), load(CONTROLLER, CONTROLLER_SHA)
    H.require(re.fullmatch("[0-9a-f]{64}", args.expected_script_sha256 or ""), "explicit wrapper pin required")
    H.require(C.sha(Path(__file__)) == args.expected_script_sha256, "wrapper pin mismatch")
    H.require(args.action in ("capture", "publish"), "explicit action required")
    for sig in C.SIGNALS:
        signal.signal(sig, C.interrupted)
    if args.action == "capture":
        H.require(args.capture_receipt is None, "publication pin not allowed in capture")
        return capture(args, H, C)
    H.require(args.id is None and args.paired is None and not args.scale, "capture options not allowed in publication")
    return publish(args, H, C)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError, KeyError, TypeError, IndexError) as error:
        print("Review wrapper refused:", error)
        raise SystemExit(1)
