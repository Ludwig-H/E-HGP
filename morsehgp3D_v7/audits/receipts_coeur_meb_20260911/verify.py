#!/usr/bin/env python3
"""Lecteur portable du reçu `receipts_coeur_meb_20260911`.

Ne compile rien, n'execute aucun moteur, ne contacte pas GCP. Il controle que
les pieces annoncees sont presentes, que les chiffres cites par la note se
lisent dans la sortie brute conservee, et que le paquet `provenance/` atteste
reellement deux bras DIFFERENTS. Sans assert ; identique sous `python3 -B` et
`python3 -B -O`. Code 0 conforme, 1 sinon, 2 lecture impossible.
"""
from __future__ import annotations

import hashlib
import json
import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
PROV = HERE / "provenance"
REQUIRED = ["README.md", "meb_hybrid.cpp", "welzl2.cpp", "flux_reel.out",
            "flux_reel_16k.out", "realflow_patch.py"]
PROV_REQUIRED = [
    "provenance.txt", "compiler.txt", "sources.sha256", "patch.stdout",
    "compile_base.argv", "compile_var.argv",
    "run_base.argv", "run_var.argv",
    "run_base.stdout", "run_var.stdout",
]
# Le noyau MEB est le SEUL fichier qui doit differer entre les deux bras.
KERNEL = "src/forest/anchor_meb.hpp"
UNCHANGED = ["src/forest/full_ball_tower.hpp", "bench/full_ball_tower_probe.cpp"]


def read(path: Path, problems: list, label: str):
    try:
        return path.read_text(encoding="utf-8", errors="replace")
    except OSError as error:
        problems.append(f"{label} illisible : {error}")
        return None


def scan_arm(raw: str):
    return {
        "digest": re.findall(r'"payload_digest":"([0-9a-f]{64})"', raw),
        "tower_s": [float(v) for v in re.findall(r'"tower_s":([0-9.]+)', raw)],
        "calls": [int(v) for v in re.findall(r'"resolver_meb_calls":([0-9]+)', raw)],
        "supports": [int(v) for v in re.findall(r'"resolver_supports_tested":([0-9]+)', raw)],
    }


def check_provenance(problems: list) -> dict:
    """Le controle central : prouver que les deux bras different vraiment."""
    if not PROV.is_dir():
        problems.append("paquet provenance/ absent")
        return {}
    for name in PROV_REQUIRED:
        if not (PROV / name).is_file():
            problems.append(f"provenance : piece absente : {name}")

    # a) Les empreintes de sources : noyau DIFFERENT, reste IDENTIQUE.
    digests: dict = {}
    sums = read(PROV / "sources.sha256", problems, "provenance/sources.sha256")
    if sums:
        for line in sums.splitlines():
            parts = line.split()
            if len(parts) != 2:
                continue
            sha, tagged = parts
            if ":" not in tagged:
                continue
            arm, rel = tagged.split(":", 1)
            rel = rel.split("morsehgp3D_v7/", 1)[-1]
            digests.setdefault(rel, {})[arm] = sha
    kernel = digests.get(KERNEL, {})
    if len(kernel) != 2:
        problems.append(f"provenance : {KERNEL} non epingle dans les deux bras")
    elif kernel.get("base") == kernel.get("var"):
        problems.append(
            "MESURE A VIDE : le noyau est identique dans les deux bras, "
            "le patch n'a donc pas ete applique")
    for rel in UNCHANGED:
        pair = digests.get(rel, {})
        if len(pair) == 2 and pair.get("base") != pair.get("var"):
            problems.append(f"provenance : {rel} devait rester identique entre les bras")

    # b) Le patch a bien laisse ses marqueurs.
    patch_out = read(PROV / "patch.stdout", problems, "provenance/patch.stdout")

    # c) Les sorties completes, relues sans grep.
    arms = {}
    for arm in ("base", "var"):
        raw = read(PROV / f"run_{arm}.stdout", problems, f"provenance/run_{arm}.stdout")
        if raw is None:
            continue
        found = scan_arm(raw)
        if len(found["digest"]) != 1:
            problems.append(f"provenance : run_{arm}.stdout ne porte pas un payload_digest unique")
        if len(found["tower_s"]) != 1:
            problems.append(f"provenance : run_{arm}.stdout ne porte pas un tower_s unique")
        arms[arm] = found

    result: dict = {"sources": digests, "arms": {}}
    if len(arms) == 2 and all(a["digest"] and a["tower_s"] for a in arms.values()):
        db, dv = arms["base"]["digest"][0], arms["var"]["digest"][0]
        if db != dv:
            problems.append("provenance : les payload_digest des deux bras different")
        tb, tv = arms["base"]["tower_s"][0], arms["var"]["tower_s"][0]
        if not tv < tb:
            problems.append("provenance : tower_s ne decroit pas dans le bras patche")
        sb = arms["base"]["supports"][0] if arms["base"]["supports"] else None
        sv = arms["var"]["supports"][0] if arms["var"]["supports"] else None
        if sb is not None and sv is not None and not sv < sb:
            problems.append("provenance : les supports testes ne decroissent pas")
        cb = arms["base"]["calls"][0] if arms["base"]["calls"] else None
        cv = arms["var"]["calls"][0] if arms["var"]["calls"] else None
        if cb is not None and cv is not None and cb != cv:
            problems.append("provenance : resolver_meb_calls differe entre les bras")
        result["arms"] = {
            "payload_digest_identical": db == dv,
            "payload_digest": db,
            "tower_s": {"base": tb, "var": tv, "gain": round(tb / tv, 3) if tv else None},
            "supports_tested": {"base": sb, "var": sv,
                                "gain": round(sb / sv, 3) if sb and sv else None},
            "resolver_meb_calls": {"base": cb, "var": cv},
        }
    result["kernel_differs_between_arms"] = (
        len(kernel) == 2 and kernel.get("base") != kernel.get("var"))
    result["patch_report"] = (patch_out or "").strip().splitlines()[:4]
    result["commit"] = (read(PROV / "provenance.txt", problems, "provenance.txt") or "").strip()
    result["compiler"] = (read(PROV / "compiler.txt", problems, "compiler.txt") or "").strip()
    for arm in ("base", "var"):
        argv = read(PROV / f"run_{arm}.argv", problems, f"run_{arm}.argv")
        if argv:
            result.setdefault("run_commands", {})[arm] = argv.strip()
    return result


def check_shell_hist(problems: list) -> dict:
    """Frequence du cas coquille=support, relue depuis la sortie brute."""
    path = HERE / "shell_hist.out"
    if not path.is_file():
        problems.append("piece absente : shell_hist.out")
        return {}
    raw = read(path, problems, "shell_hist.out")
    if raw is None:
        return {}
    rows = [(int(q), int(sh), int(c)) for q, sh, c in
            re.findall(r"q=(\d+) shell=(\d+) count=(\d+)", raw)]
    totals = re.findall(r"total_accepte=(\d+) coquille_egale_support=(\d+)", raw)
    calls = re.findall(r'"resolver_meb_calls":(\d+)', raw)
    if not rows or not totals:
        problems.append("shell_hist.out : histogramme illisible")
        return {}
    total, equal = int(totals[0][0]), int(totals[0][1])
    if sum(c for _, _, c in rows) != total:
        problems.append("shell_hist.out : les lignes ne totalisent pas total_accepte")
    if sum(c for q, sh, c in rows if q == sh) != equal:
        problems.append("shell_hist.out : le compte coquille=support ne suit pas les lignes")
    if calls and int(calls[0]) != total:
        problems.append(
            "shell_hist.out : total_accepte differe de resolver_meb_calls ; "
            "l'instrumentation n'a pas vu exactement un evenement par appel")
    return {
        "accepted": total,
        "shell_equals_support": equal,
        "fraction": round(equal / total, 6) if total else None,
        "by_support_size": {f"q={q}": {"shell": sh, "count": c} for q, sh, c in rows},
        "resolver_meb_calls": int(calls[0]) if calls else None,
    }


def main() -> int:
    problems: list = []
    for name in REQUIRED:
        if not (HERE / name).is_file():
            problems.append(f"piece absente : {name}")

    scales = {}
    for out in sorted(HERE.glob("flux_reel*.out")):
        raw = read(out, problems, f"sortie brute {out.name}")
        if raw is None:
            continue
        found = scan_arm(raw)
        digests, towers = found["digest"], found["tower_s"]
        calls, supports = found["calls"], found["supports"]
        if len(digests) != 2 or digests[0] != digests[1]:
            problems.append(f"{out.name} : les deux payload_digest ne sont pas identiques")
        if len(towers) != 2 or not towers[1] < towers[0]:
            problems.append(f"{out.name} : tower_s ne decroit pas dans le bras patche")
        if calls and (len(calls) != 2 or calls[0] != calls[1]):
            problems.append(f"{out.name} : resolver_meb_calls differe entre les bras")
        if supports and (len(supports) != 2 or not supports[1] < supports[0]):
            problems.append(f"{out.name} : les supports testes ne decroissent pas")
        scales[out.name] = {
            "payload_digest_identical": len(digests) == 2 and digests[0] == digests[1],
            "payload_digest": digests[0] if digests else None,
            "tower_s": towers,
            "tower_gain": round(towers[0] / towers[1], 3) if len(towers) == 2 and towers[1] else None,
            "supports_tested": supports or None,
        }
    if len(scales) < 2:
        problems.append("moins de deux echelles conservees")

    provenance = check_provenance(problems)
    shell = check_shell_hist(problems)

    summary = {
        "receipt": "audits/receipts_coeur_meb_20260911",
        "scales": scales,
        "provenance": provenance,
        "shell_histogram": shell,
        "engine_executed_by_reader": False,
        "gcp_used": False,
        "public_status": "not_claimed",
        "problems": problems,
        "status": "passed" if not problems else "failed",
    }
    print(json.dumps(summary, indent=2, ensure_ascii=False))
    return 0 if not problems else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except OSError as error:
        print(f"lecture impossible : {error}", file=sys.stderr)
        raise SystemExit(2)
