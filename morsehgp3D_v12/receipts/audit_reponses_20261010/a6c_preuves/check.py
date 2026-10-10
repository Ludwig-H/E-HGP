#!/usr/bin/env python3
"""Read public receipts/source, optional archived timing metadata; no native run."""
import argparse
import collections
import hashlib
import json
import math
from pathlib import Path
import re
import statistics
import subprocess
import tarfile

REV = "aa6338ee8b3d5daf6a821043c3bdd9d63ce85c66"
B3 = "1879eff9a625f31c9477c0d56d232719c5813514"
BASE = "8a0716e74"
V12 = "morsehgp3D_v12/"
PILOT_SHA = "6a798b312bfb59e14bfc65df209b57ab13ed2bd6fd247ce7d530d4d2953c60f2"


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def fit(rows):
    x = [r[0] / 1000 for r in rows]
    y = [r[1] for r in rows]
    n = len(x)
    xm, ym = statistics.mean(x), statistics.mean(y)
    xx = sum((v - xm) ** 2 for v in x)
    b = sum((v - xm) * (w - ym) for v, w in zip(x, y)) / xx
    a = ym - b * xm
    e = [w - a - b * v for v, w in zip(x, y)]
    loo = [v / (1 - 1 / n - (z - xm) ** 2 / xx) for z, v in zip(x, e)]
    return [a, b, math.sqrt(statistics.mean(v * v for v in e)),
            math.sqrt(statistics.mean(v * v for v in loo))]


def replay_raw(repo, archive, saved):
    require(sha(archive.read_bytes()) == saved["archive_a6b_sha256"], "archive hash")
    root = repo / (V12 + "receipts/g4_a6b_20261008/a6b2/resultats/cmd/"
                   "001_t2da6b_pilote/files/t2da6b/journaux")
    sources = [(str(p.relative_to(root)), p.read_bytes()) for p in root.rglob("*.jsonl")]
    with tarfile.open(archive) as pack:
        for member in pack:
            if member.isfile() and "/t2da6b/journaux/" in member.name and member.name.endswith(".jsonl"):
                sources.append((member.name.split("/t2da6b/journaux/")[1],
                                pack.extractfile(member).read()))
    groups, pins = collections.defaultdict(list), []
    aliases = {"ng00": "kitti_ng_08_000000", "ng01": "kitti_ng_08_000100",
               "ng02": "kitti_ng_08_000200"}
    for path, data in sources:
        selected = (re.fullmatch(r"grandes/avant_t[0-9]+\.jsonl", path)
                    or path == "identite/v12set_k5_avant.jsonl"
                    or re.fullmatch(r"ng/ng0[012]_avant_t[0-9]+\.jsonl", path))
        if not selected:
            continue
        pins.append([path, sha(data)])
        for line in data.splitlines():
            row = json.loads(line)
            if row.get("phase") != "full" or row["pass"] < (21 if path.startswith("grandes/") else 1):
                continue
            require(row["status"] == "ok" and row["threads"] == 48 and row["kmax"] == 5,
                    "calibration row configuration")
            groups[aliases.get(row["trame"], row["trame"])].append(row)
    expected = [{"trame": name, "sites": rows[0]["sites"], "passes": len(rows),
                 "g_ns_median": statistics.median(r["etapes_ns"]["G"] for r in rows),
                 "kernel5_ns_median": statistics.median(r["fins_par_ordre_ns"][4][1] for r in rows)}
                for name, rows in sorted(groups.items())]
    require(expected == saved["groups"], "exact raw calibration aggregates")
    require(len(pins) == saved["selected_logs"], "selected logs count")
    require(sha(json.dumps(sorted(pins), separators=(",", ":")).encode()) ==
            saved["selected_logs_manifest_sha256"], "selected logs hashes")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--a6b-archive", type=Path)
    args = parser.parse_args()
    here = Path(__file__).resolve().parent
    repo = args.repo.resolve()

    def git(*argv):
        return subprocess.check_output(["git", "-C", str(repo), *argv])

    for line in (here / "SHA256SUMS").read_text().splitlines():
        digest, name = line.split(maxsplit=1)
        require(sha((here / name).read_bytes()) == digest, "receipt hash " + name)

    trees = {}
    for name in ("src", "tests"):
        before = git("rev-parse", BASE + ":" + V12 + name).decode().strip()
        after = git("rev-parse", B3 + ":" + V12 + name).decode().strip()
        require(before == after, "B3 withdrawal " + name)
        trees[name] = before

    receipt = repo / (V12 + "receipts/g4_t2db3_20261008")
    manifest = git("show", B3 + ":" + V12 + "receipts/g4_t2db3_20261008/SHA256SUMS")
    require((receipt / "SHA256SUMS").read_bytes() == manifest, "B3 public manifest source")
    checked = 0
    for line in manifest.decode().splitlines():
        digest, name = line.split(maxsplit=1)
        require(sha((receipt / name.lstrip("*")).read_bytes()) == digest, "B3 hash " + name)
        checked += 1
    pilot = git("show", REV + ":" + V12 + "microbancs/mes_t2d_b3/pilote_t2d_b3.py")
    require(sha(pilot) == PILOT_SHA, "B3 pilot is original, not v2 proposal")

    criterion_bytes = git("show", REV + ":" + V12 + "microbancs/mes_t2d_a6c/critere_a6c.json")
    criterion = json.loads(criterion_bytes)
    saved = json.loads((here / "calibration.json").read_text())
    published = {r["trame"]: r for r in criterion["trames"]}
    pairs = []
    for row in saved["groups"]:
        target = published[row["trame"]]
        g, k = row["g_ns_median"] / 1e6, row["kernel5_ns_median"] / 1e6
        delta = (row["kernel5_ns_median"] - row["g_ns_median"]) / 1e6
        require(row["sites"] == target["sites"] and row["passes"] == target["passes_base"], "group counts")
        require(round(g, 2) == target["fin_g_base_ms"] and round(k, 2) == target["fin_noyau_k_base_ms"]
                and round(delta, 2) == target["retard_reel_ms"], "published medians")
        require(target["chaine"] == (row["sites"] >= 43900), "published chain decision")
        pairs.append((row["sites"], delta))
    result = fit(pairs)
    claim = [*criterion["coefficients_a_b"], criterion["rms_echantillon_ms"], criterion["rms_loo_ms"]]
    require(all(abs(a - b) < 1e-12 for a, b in zip(result, claim)), "exact OLS reconstruction")
    require(len(pairs) == len(published) == 37 and "40 trames" in criterion["modele"], "37/40 metadata discrepancy")
    if args.a6b_archive:
        replay_raw(repo, args.a6b_archive, saved)
    threshold = 1000 * (3 - result[0]) / result[1]
    print(json.dumps({"schema": "ehgp.v12.audit.a6c_preuves.v1", "revision": REV,
        "b3_withdrawal_trees": trees, "b3_manifest_entries": checked,
        "b3_public_jsonl": sum(1 for _ in receipt.rglob("*.jsonl")),
        "b3_pilot_sha256": PILOT_SHA, "criterion_sha256": sha(criterion_bytes),
        "calibration_groups": len(pairs), "calibration_passes": sum(r["passes"] for r in saved["groups"]),
        "model_claim_groups": 40, "fit_a_b_rms_loo": result,
        "raw_calibration_replayed": args.a6b_archive is not None,
        "predicted_ms_at_43900": result[0] + result[1] * 43.9,
        "three_ms_threshold_continuous": threshold,
        "three_ms_threshold_integer": math.ceil(threshold)}, indent=2))


if __name__ == "__main__":
    main()
