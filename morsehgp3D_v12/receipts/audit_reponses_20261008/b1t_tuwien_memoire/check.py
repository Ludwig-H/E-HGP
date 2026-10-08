#!/usr/bin/env python3
"""Pins, journal public, arithmetique et application en copie ; jamais de sonde."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent


def need(ok, label):
    if not ok:
        raise ValueError(label)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def uint(value):
    return type(value) is int and 0 <= value < 2**64


def main(repo, returned):
    cap = json.loads((HERE / "capture.json").read_text())
    sources = {}
    for path, expected in cap["source_sha256"].items():
        data = subprocess.check_output(["git", "-C", str(repo), "show", cap["pin"] + ":" + path])
        need(sha(data) == expected, "source " + path)
        sources[path] = data
    need(not subprocess.check_output([
        "git", "-C", str(repo), "diff", "47feedc96", cap["pin"], "--", "morsehgp3D_v12/src/"
    ]), "source R1")
    raw = (returned / cap["raw_path"]).read_bytes()
    report_bytes = (returned / cap["report_path"]).read_bytes()
    need(sha(raw) == cap["raw_sha256"], "journal")
    need(sha(report_bytes) == cap["report_sha256"], "rapport")
    rows = [json.loads(line) for line in raw.splitlines()]
    need([r.get("phase") for r in rows] == ["open", "full", "liberation", "exit"], "sequence")
    op, full, freed, end = rows
    need(op["status"] == "ok" and op["reason"] == "none" and op["budget_appareil"] == "separe", "open")
    need(full["status"] == "ok" and full["voie"] == "device" and full["etapes_schema"] == "recouvert", "full")
    for key, value in {"pass": 0, "coord_bits": 21, "kmax": 5, "threads": 48, "sites": 5199758}.items():
        need(uint(full[key]) and full[key] == value, "full " + key)
    need(uint(freed["pass"]) and freed["pass"] == 0 and uint(freed["liberation_ns"]), "liberation")
    need(end == {"phase": "exit", "status": "resource_exhausted", "reason": "memory_budget"}, "exit")
    need("full_sha256" not in full and full["hors_mur_ns"]["empreinte"] == 0, "sans empreinte FULL")
    report = json.loads(report_bytes)
    cases = [c for c in report["cas"] if c["nom"] == "forinst_tuwien_train_sans_sol"]
    need(len(cases) == 1, "cohorte cible")
    case = cases[0]
    need(type(case["code"]) is int and case["code"] == 2 and case["etat"] == "refus", "code")
    need(case["empreinte"] is False and case["raison"] == "resource_exhausted/memory_budget", "refus")
    need(case["passes"] == [dict(full, liberation_ns=freed["liberation_ns"])], "rapport/prefixe")
    pars = report["parametres"]  # whitelist ci-dessous : jamais exporter argv/chemins de compte
    host, device = pars["budget_octets"], pars["budget_appareil_octets"]
    need(uint(host) and host == 160 * 2**30, "budget hote")
    need(uint(device) and device == 88 * 2**30, "budget appareil")
    capacity = full["appareil_octets"]
    need(uint(capacity) and capacity == full["pic_appareil_octets"] == 85886193388, "capacite/pic")
    headroom = device - capacity
    residual = 12 * full["sites"] + 1024 * 4
    need(headroom == 8603087124 and residual == 62401192 and capacity > residual, "arithmetique")
    # Illustration de coexistence seulement : aucune allocation TU Wien n'est reconstruite ici.
    need(4 + 3 <= 10 and 4 + 4 <= 10 and 4 + 4 + 3 > 10, "exemple abstrait")
    path = "morsehgp3D_v12/bench/full_probe.cpp"
    before = sources[path]
    with tempfile.TemporaryDirectory(prefix="audit-b1t-patch-") as tmp:
        target = Path(tmp) / path
        target.parent.mkdir(parents=True)
        target.write_bytes(before)
        patch = (HERE / "instrumentation.patch").resolve()
        for args in (["--check"], []):
            subprocess.run(["git", "apply", *args, str(patch)], cwd=tmp, check=True, capture_output=True)
        after = target.read_bytes()
    need(sha(after) == cap["instrumented_probe_sha256"], "postimage proposee")
    # Toutes les emissions contractuelles demeurent textuellement identiques.
    def output_block(data):
        return data.split(b"void print_pass(", 1)[1].split(b"\nOutcome passes(", 1)[0].split(
            b"\n// Diagnostic stderr seulement", 1)[0]
    need(output_block(before) == output_block(after), "bloc stdout")
    need(after.count(b"trace_failure(pass, t, budget, device_budget);") == 2, "deux frontieres")
    need((returned / cap["raw_path"]).read_bytes() == raw, "journal stable")
    need((returned / cap["report_path"]).read_bytes() == report_bytes, "rapport stable")
    result = {
        "pin": cap["pin"], "source_pins_verified": len(sources), "source_R1_identical": True,
        "raw_sha256": sha(raw), "report_sha256": sha(report_bytes),
        "observed": {"sites": full["sites"], "successful_passes": [0], "exit_code": case["code"],
                     "reason": end["reason"], "wall_ns": full["wall_ns"],
                     "liberation_ns": freed["liberation_ns"], "host_limit": host, "device_limit": device,
                     "device_capacity_and_peak": capacity, "host_peak": full["pic_octets"],
                     "rss_max_bytes": full["rss_max_octets"], "full_digest": False},
        "arithmetic": {"next_pass_device_headroom": headroom, "sliced_endpoint_if_taken": residual},
        "source_inference": "first_finish_complete_resident; front_retention_unknown",
        "failure_stage": "unknown", "causal_fix_qualified": False,
        "patch_applies_in_copy": True, "stdout_block_unchanged": True,
        "postimage_sha256": sha(after), "native_execution": False,
    }
    expected = HERE / "results.json"
    if expected.exists():
        need(result == json.loads(expected.read_text()), "resultat fige")
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: check.py DEPOT DOSSIER_B1T/b")
    main(Path(sys.argv[1]), Path(sys.argv[2]))
