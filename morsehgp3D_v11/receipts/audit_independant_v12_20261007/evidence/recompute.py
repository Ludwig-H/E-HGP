#!/usr/bin/env python3
"""Recompte borne des preuves archivees, sans donnees LiDAR ni GCP.

Ne recopie que des agregats, empreintes, identifiants de commits et chemins
relatifs. Les sources sont lues par git show au snapshot fixe, pas au HEAD.
"""
import argparse
import collections
import hashlib
import io
import json
from pathlib import Path
import statistics
import subprocess
import tarfile

PIN = "33c2ae3c8b66d12ef5b2405f1b0f807d7e57aeae"
V = "morsehgp3D_v11/"
R = V + "receipts/"
PERF = R + "developpement_20261007/filtre_g1_avx2/claudeg1/"


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[4])
    ap.add_argument("--out", type=Path, default=Path(__file__).with_name("recomputed.json"))
    args = ap.parse_args()
    used = {}

    def git(*cmd):
        return subprocess.check_output(["git", "-C", str(args.repo), *cmd])

    def blob(path):
        raw = git("show", PIN + ":" + path)
        used[path] = hashlib.sha256(raw).hexdigest()
        return raw

    def obj(path):
        return json.loads(blob(path))

    files = git("ls-tree", "-r", "--name-only", PIN, R).decode().splitlines()
    result = {"schema": "audit.v11.evidence.v1", "snapshot": PIN,
              "gcp_used": False, "raw_clouds_read": False}
    timings = []
    for suffix in ("ab_k5_16_cpu", "ab_k5_24_gpu", "ab_k10_24_gpu"):
        path = PERF + "gpu_ab_report_" + suffix + ".json"
        d = obj(path)
        for frame in sorted(d["identity"]):
            mode = "base:" + ("cpu" if "cpu" in suffix else "gpu")
            c = [r for r in d["cold"] if r["frame"] == frame and r["mode"] == mode]
            w = [r for r in d["warm"] if r["frame"] == frame and r["mode"] == mode]
            if len(w) != 1 or any(r["code"] != 0 for r in c + w):
                raise ValueError("performance rows incomplete: " + suffix)
            passes = [p for p in w[0]["passes"] if p["pass"] >= 2]
            cold_values = [r["summary"]["wall_ms"] for r in c]
            times = {"source": path, "frame": frame, "k": d["kmax"], "leaf": d["leaf"],
                     "mode": d["modes"][mode], "cold_processes": len(c),
                     "cold_upper_median_ms": sorted(cold_values)[len(cold_values) // 2],
                     "cold_median_ms": statistics.median(cold_values),
                     "warm_processes": len(w), "warm_timed_passes": len(passes),
                     "digest": w[0]["dump_sha256"],
                     "cold_external_process_median_ms": statistics.median(r["seconds"] * 1000 for r in c)}
            for stage in ("wall", "domain", "forest"):
                times["warm_" + stage + "_median_ms"] = statistics.median(p[stage + "_ns"] / 1e6 for p in passes)
            key = "cold|%s|w48|%s" % (frame, mode)
            wkey = "warm|%s|w48|%s" % (frame, mode)
            if times["cold_upper_median_ms"] != d["cold_medians_ms"][key]["wall_ms"]:
                raise ValueError("cold aggregate mismatch")
            if abs(times["warm_wall_median_ms"] - d["warm_medians_ms"][wkey]["wall_ms"]) > 1e-9:
                raise ValueError("warm aggregate mismatch")
            timings.append(times)
    result["final_base_timings"] = timings

    counts, by_frame, digests, invalid, inspected = collections.Counter(), collections.Counter(), {}, [], []
    for path in files:
        if not any(path.startswith(R + "developpement_2026100%d/" % day) for day in range(4, 8)):
            continue
        if "report" not in Path(path).name or not path.endswith(".json"):
            continue
        d = obj(path)
        if not isinstance(d, dict) or d.get("schema") != "ehgp.v11.gpu_ab.v1":
            continue
        inspected.append(path)
        for regime in ("cold", "warm"):
            for row in d[regime]:
                key = str(d["kmax"]) + "|" + row["frame"]
                digests.setdefault(key, set()).add(row.get("dump_sha256"))
                mode = d["modes"][row["mode"]].split("@")[0].split(":")[0]
                route = "gpu" if int(mode) & 65536 else "cpu_or_host_batch"
                counts[regime] += 1
                counts[route + "_" + regime] += 1
                if row.get("dump_sha256"):
                    counts["dump_" + regime] += 1
                    counts["dump_" + route + "_" + regime] += 1
                    by_frame[key] += 1
                if row.get("code") != 0 or not row.get("dump_sha256"):
                    invalid.append({"source": path, "regime": regime, "frame": row["frame"],
                                    "code": row.get("code"), "exit": row["summary"].get("exit"),
                                    "report_verdict": d.get("verdict")})
    result["dump_inventory"] = {"report_count": len(inspected), "reports": inspected,
                                "counts": dict(counts), "invalid_rows": invalid,
                                "available_dumps_by_k_frame": dict(by_frame),
                                "digests_by_k_frame": {k: sorted(v, key=lambda x: x or "") for k, v in sorted(digests.items())}}

    scope = R + "developpement_20261005/qualification_finale/"
    final = obj(scope + "summary.json")
    qualification = []
    for session in final:
        qualification.append({k: session[k] for k in ("session", "commit", "status", "certified", "configs")})
    result["qualification_union"] = qualification
    result["release_poison_3695_recount"] = sum(
        c["tests"]["passed"] for s in final if s["session"] in ("clauderepriser1", "clauderepriser2")
        for c in s["configs"])
    result["qualification_v3"] = []
    for path in files:
        if path.startswith(R + "developpement_20261006/v3_qualification/") and Path(path).name.startswith("result_"):
            d = obj(path)
            result["qualification_v3"].append({"source": path, **{k: d[k] for k in ("name", "status", "cmake_options", "tests")}})

    result["source_diffs"] = []
    for a, b in (("38b76701b", "98a009550"), ("733912e65", "ac081a06f"), ("ac081a06f", PIN)):
        paths = [V + p for p in ("src", "cli", "bench", "tests", "tools", "cmake", "CMakeLists.txt")]
        changed = git("diff", "--name-only", a, b, "--", *paths).decode().splitlines()
        result["source_diffs"].append({"from": a, "to": b, "changed": changed,
                                      "engine_changed": [p for p in changed if p.startswith(V + "src/")]})
    mutants = git("ls-tree", "-r", "--name-only", PIN, V + "tests/mutants").decode().splitlines()
    result["mutant_manifest_counts"] = {Path(p).stem: len(obj(p)["mutants"])
                                       for p in mutants if p.endswith(".json")}

    manifests = []
    for folder in (R + "developpement_20261005/qualification_finale/",
                   R + "developpement_20261006/v3_qualification/",
                   R + "developpement_20261007/filtre_g1_avx2/",
                   R + "audit_geant_v11_20261007/"):
        good, bad = 0, []
        for line in blob(folder + "SHA256SUMS").decode().splitlines():
            expected, name = line.split(maxsplit=1)
            name = name.lstrip("*")
            path = folder + name.removeprefix("./")
            # Some manifests carry repository-relative paths.
            if name.startswith(V):
                path = name
            if hashlib.sha256(blob(path)).hexdigest() == expected:
                good += 1
            else:
                bad.append(path)
        manifests.append({"folder": folder, "verified": good, "mismatches": bad})
    result["sha256_manifests"] = manifests

    # The following file contains evaluation aggregates, never raw point coordinates.
    table = obj(R + "developpement_20261004/e1_sortie_plate/etude/etude_table.json")
    result["hdbscan_flat_conditional_table"] = {
        k: {line: table["%s_mcs20|k=%s" % (line, k)]["all_found"]
            for line in ("T_eom1", "A_eom1", "R0_eom", "T_eom2")}
        for k in ("2", "3", "5", "10", "all")}
    synthetic = []
    for session in ("claudepts4", "claudepts6"):
        path = R + "developpement_20261003/points_g4/sessions/" + session + "/results.tar.gz"
        groups = collections.defaultdict(list)
        with tarfile.open(fileobj=io.BytesIO(blob(path))) as archive:
            for member in archive.getmembers():
                parts = member.name.split("/")
                if not member.isfile() or len(parts) < 2 or parts[-2] != "synthetic" or not member.name.endswith(".json"):
                    continue
                scene = json.load(archive.extractfile(member))
                if scene.get("status") != "ok":
                    continue
                n = scene["meta"]["spec"]["n"]
                for k, order in scene["orders"].items():
                    if "hdbscan" not in order:
                        continue
                    baseline = statistics.mean(order["hdbscan"]["best"])
                    for rule in ("margin_r", "cover", "first", "core"):
                        if rule in order:
                            groups[(n, int(k), rule)].append(statistics.mean(order[rule]["best"]) - baseline)
        for (n, k, rule), values in sorted(groups.items()):
            synthetic.append({"session": session, "n": n, "k": k, "rule": rule,
                              "scenes": len(values), "mean_best_iou_difference": statistics.mean(values),
                              "scenes_at_least_as_good": sum(x >= 0 for x in values)})
    result["hdbscan_synthetic_level_b"] = synthetic
    result["inputs_sha256"] = used
    args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(json.dumps({"snapshot": PIN, "performance_rows": len(timings),
                      "reports": len(inspected), "dump_counts": dict(counts),
                      "qualification_recount": result["release_poison_3695_recount"],
                      "manifests": manifests}, indent=2))


if __name__ == "__main__":
    main()
