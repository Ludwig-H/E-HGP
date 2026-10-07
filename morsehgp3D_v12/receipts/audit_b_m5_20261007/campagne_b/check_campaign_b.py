#!/usr/bin/env python3
"""Recalcule le reçu M3/M4 B épinglé, sans build, GPU, ni lecture de données/binaires.

Par défaut exige les petites archives locales originales. --published-only indique
explicitement une contre-lecture limitée aux objets Git publiés. Aucun assert.
"""
import argparse
import hashlib
import io
import json
import math
from pathlib import Path
import re
import shlex
import statistics
import subprocess
import tarfile

PIN = "e30000dec1027c5f0ade3093a94563ee412409d3"
SNAPSHOT = "5bd963078161cff1de8abe17f0595ab657bdc5e7"
ROOT = Path(__file__).resolve().parents[4]
RECEIPT = "morsehgp3D_v12/receipts/g4_t0b_20261007"
SOURCE = "morsehgp3D_v12/microbancs/mes_m3_m4_tour/"
HASHES = {}
FRAMES = ("ng00", "ng01", "ng02")


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def pinned(path, commit=PIN):
    data = subprocess.check_output(["git", "show", f"{commit}:{path}"], cwd=ROOT)
    if commit == PIN:
        need((ROOT / path).read_bytes() == data, "worktree differs from pin: " + path)
        HASHES[path] = sha(data)
    return data


def load(path):
    return json.loads(pinned(path))


def jsonl(path):
    return [json.loads(line) for line in pinned(path).decode().splitlines() if line.startswith("{")]


def clean(data):
    return re.sub(r"/home/[A-Za-z0-9_.-]+", "$HOME", data.decode()).encode()


def rows_of(rows, phase, expected):
    items = [row for row in rows if row.get("phase") == phase]
    need(sorted(row["k"] for row in items) == list(expected), "missing/duplicate order: " + phase)
    return {row["k"]: row for row in items}


def checked_times(values):
    need(all(type(v) in (int, float) and math.isfinite(v) and v > 0 for v in values), "invalid time")
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--published-only", action="store_true")
    args = parser.parse_args()
    session = load(RECEIPT + "/receipt.json")
    need(session["source"]["head_commit"] == SNAPSHOT, "snapshot identity")
    need(session["status"] == "completed" and session["results_verified"] and
         session["data_verified_remote"] and session["targeted_shutdown_certified"], "session record incomplete")
    manifest = pinned(RECEIPT + "/SHA256SUMS").decode().splitlines()
    for line in manifest:
        expected, name = line.split(None, 1)
        need(sha(pinned(RECEIPT + "/" + name)) == expected, "public hash: " + name)
    need(len(manifest) == 79, "manifest size")
    source_files = [row for row in session["source"]["manifest"] if row["path"].startswith(SOURCE)]
    need(len(source_files) == 9, "M3/M4 source set")
    for row in source_files:
        need(sha(pinned(row["path"])) == row["sha256"] == sha(pinned(row["path"], SNAPSHOT)),
             "measured source does not match source pin")
    rule = pinned("morsehgp3D_v12/docs/PLAN.md", SNAPSHOT).decode()
    need("binaire non haché" in rule and "contraction ≤ 3 ms" in rule and "40 % à un fil" in rule,
         "premeasurement rule changed")
    archive_paths = ["CMakeLists.txt", "cmake", "src", "bench", "tests", "reference", "tools", "cli"]
    archive = subprocess.check_output(["git", "archive", "--format=tar.gz", "ac081a06f"] +
                                     ["morsehgp3D_v11/" + x for x in archive_paths], cwd=ROOT)
    archive_ref = next(row for row in session["data_files"] if row["name"] == "v11_src_ac081a06f.tar.gz")
    need(sha(archive) == archive_ref["sha256"] and len(archive) == archive_ref["size"], "v11 source archive")
    source_receipt = load(RECEIPT + "/resultats/cmd/000_source_v11/files/source_v11.json")
    need(source_receipt["archive_sha256"] == sha(archive), "source extraction mismatch")
    all_cases = {}; reports = {}; directories = {}; binary_hash_keys = []
    counts = dict(m3_processes=0, m4_processes=0, m3_order_rows=0, m4_order_rows=0,
                  parts_per_case_once=0, t6_births_per_case_once=0, dump_files=0)
    for kmax, process_count, command_number in ((5, 5, 2), (10, 3, 4)):
        directory = RECEIPT + f"/resultats/cmd/{command_number:03d}_m34_k{kmax}_publier/files/m34_k{kmax}"
        directories[kmax] = directory
        report = load(directory + "/rapport_mes_m3_m4.json")
        reports[kmax] = report
        execution = report["executions"]
        need(len(execution) == 1 and execution[0]["code"] == 0, "execution history missing/merged/failed")
        plan = execution[0]["arguments"]
        need(plan["processus"] == process_count and plan["fils"] == plan["fils_contraction"] == 48 and
             plan["repetitions_m4"] == 5 and plan["chrono_k10"] == 1 and plan["chrono_k5"] == 3,
             "measurement replication plan")
        need(set(report["vidages"]) == {f"{f}_k{kmax}" for f in FRAMES}, "missing/extra dump case")
        if kmax == 5:
            need(report["construction"]["code"] == 0 and
                 report["construction"]["libmhgp11_sha256"] == source_receipt["libmhgp11_sha256"],
                 "library/build source mismatch")
            for name, gate in report["portes"].items():
                mutant = "mutant" in name
                need(gate["code"] == gate["code_attendu"] == int(mutant), "gate return code")
                lines = jsonl(directory + "/porte_" + name + ".jsonl")
                need(lines == gate["lignes"] and bool(lines), "gate lines missing/differ")
                need((lines[-1]["ecarts"] > 0) == mutant, "gate is vacuous")
            for label, code in (("normal", 0), ("mutant_sans_s_dans_f", 1)):
                variant = jsonl(directory + "/ng00_k5/mes_m3_mere_" + label + ".jsonl")
                need(variant[-1] == {"phase": "fin", "code": code}, "M3 variant exit")
                order_rows = rows_of(variant, "ordre", range(2, 6))
                need(all(r["identite"]["identiques"] for r in order_rows.values()) == (code == 0),
                     "M3 real mutant did not cause an identity discrepancy")
        for frame in FRAMES:
            case = f"{frame}_k{kmax}"
            dump = report["vidages"][case]
            lines = jsonl(directory + f"/{case}/vidage.jsonl")
            need(dump["code"] == 0 and dump["ful1_identique"] and
                 dump["ful1_sha256"] == dump["ful1_reference"] and lines == dump["lignes"], "dump proof")
            need(dump["ful1_sha256"] in pinned("morsehgp3D_v12/docs/MESURE.md").decode(), "frozen FULL hash")
            need(lines[-1]["phase"] == "exit" and lines[-1]["status"] == "ok", "dump did not complete")
            need(lines[0]["trame"] == frame and lines[0]["K"] == kmax and lines[0]["coord_bits"] == 21,
                 "dump input identity")
            seeds = rows_of(lines, "graines", range(1, kmax + 1))
            need(all(r["identiques"] and r["journal_v11_compare"] for r in seeds.values()), "seed identity")
            need(any(r.get("forets_serie_identiques") is True for r in lines), "serial forest identity missing")
            resolution = rows_of(lines, "resolution_un_fil", range(2, kmax + 1))
            need(all(r["graines_identiques"] for r in resolution.values()), "resolution identity")
            sec = {arm: sum(checked_times([r["secondes"][arm] for r in resolution.values()]))
                   for arm in ("v11", "replique_v11", "replique_v12")}
            files = dump["fichiers"]
            need(set(files) == {"cat.bin"} | {f"{kind}_{i}.bin" for kind in ("ordre", "foret")
                                             for i in range(1, kmax + 1)}, "dump inventory incomplete")
            counts["dump_files"] += len(files)
            for name, item in files.items():
                inv = item["inventaire"]
                need(inv["coord_bits"] == 21 and inv["kmax"] == kmax and inv["trame"] == frame and
                     re.fullmatch(r"[0-9a-f]{64}", item["sha256"]), "dump metadata mismatch")
                if name.startswith("foret_"):
                    required = {"FNODES", "FEDGES", "FMETA"} | ({"FLOWER"} if inv["ordre"] > 1 else set())
                    need(set(inv["sections"]) == required and (inv["ordre"] == 1 or
                         inv["sections"]["FLOWER"]["nombre"] == inv["sections"]["FNODES"]["nombre"]),
                         "vertical sections absent/incomplete")
            m3 = []; m4 = []
            for measure, container, orders in ((3, m3, range(2, kmax + 1)), (4, m4, range(1, kmax + 1))):
                block = report[f"mes_m{measure}"][case]
                need(block["code"] == 0 and len(block["processus"]) == process_count, "process coverage")
                for p, process in enumerate(block["processus"]):
                    raw = jsonl(directory + f"/{case}/mes_m{measure}_{p}.jsonl")
                    need(raw == process["lignes"] and process["code"] == 0 and
                         raw[-1] == {"phase": "fin", "code": 0}, "process result incomplete/differs")
                    need(raw[0]["K"] == kmax and raw[0]["trame"] == frame, "process input mismatch")
                    need(raw[0]["repetitions"] == (5 if measure == 4 else 3 if kmax == 5 else 1),
                         "native repetitions mismatch")
                    rows = rows_of(raw, "ordre", orders)
                    for order, row in rows.items():
                        identity = row["identite"]
                        need(identity["identiques"] and all(v == 0 for n, v in identity.items() if n.endswith("ecarts")),
                             "native identity discrepancy")
                        inv = files[f"ordre_{order}.bin"]["inventaire"]["sections"]
                        if measure == 3:
                            need(row["parties"] == inv["PARTINF"]["nombre"] == sum(row["routes"].values()),
                                 "M3 population/routes mismatch")
                        else:
                            need(row["naissances"] == inv["BIRTHS"]["nombre"] and row["racine_unique"] and
                                 row["lem_t6"]["ecarts"] == 0 and row["lem_t6"]["naissances_jugees"] ==
                                 (row["naissances"] if order > 1 else 0), "M4/T6 identity vacuous")
                            need(row["contraction_parallele"]["identique"] and row["contraction_parallele"]["fils"] == 48,
                                 "parallel contraction identity")
                    container.append(rows)
                    counts[f"m{measure}_processes"] += 1
                    counts[f"m{measure}_order_rows"] += len(rows)
            signature = [{o: (r["routes"], r["identite"], r["parties"]) for o, r in process.items()} for process in m3]
            need(all(s == signature[0] for s in signature) and report["mes_m3"][case]["routes_identiques_entre_processus"],
                 "M3 route determinism")
            ref = sum(statistics.median(checked_times([p[o]["temps_un_fil_s"]["reference"] for p in m3])) for o in m3[0])
            new = sum(statistics.median(checked_times([p[o]["temps_un_fil_s"]["voie_nouvelle"] for p in m3])) for o in m3[0])
            parts = sum(r["parties"] for r in m3[0].values())
            t1 = sum(r["routes"]["t1"] for r in m3[0].values())
            counts["parts_per_case_once"] += parts
            counts["t6_births_per_case_once"] += sum(r["lem_t6"]["naissances_jugees"] for r in m4[0].values())
            orders_out = {}
            for o in m4[0]:
                metrics = {n: [p[o]["temps_un_fil_s"][n] * 1000 for p in m4]
                           for n in ("naissances", "noyau", "contraction")}
                metrics["parallel_48"] = [p[o]["contraction_parallele"]["secondes"] * 1000 for p in m4]
                for values in metrics.values():
                    checked_times(values)
                median = {n: statistics.median(values) for n, values in metrics.items()}
                summary = report["synthese"]["mes_m4"][case][str(o)]
                need(all(abs(round(median[n], 3) - summary["ms_" + n]) < 1e-10
                         for n in ("naissances", "noyau", "contraction")), "M4 published median differs")
                orders_out[o] = dict(births=m4[0][o]["naissances"], median_min5_ms=median,
                                     interprocess_min_max_ms={n: [min(v), max(v)] for n, v in metrics.items()})
            all_cases[case] = dict(parts=parts, t1=t1, t1_fraction=t1 / parts,
                                  meb_seconds=dict(reference=ref, candidate=new), meb_ratio=new / ref,
                                  resolution_processes=1, resolution_passes=3 if kmax == 5 else 1,
                                  resolution_seconds=sec, resolution_ratio=sec["replique_v12"] / sec["replique_v11"],
                                  resolution_order_k_ratio=resolution[kmax]["secondes"]["replique_v12"] /
                                                          resolution[kmax]["secondes"]["replique_v11"],
                                  m4_order_count=len(orders_out), m4_last_order=orders_out[kmax],
                                  m4_largest_order_kernel_ms=max(r["median_min5_ms"]["noyau"] for r in orders_out.values()),
                                  m4_largest_parallel_contraction_ms=max(r["median_min5_ms"]["parallel_48"] for r in orders_out.values()),
                                  m4_sum_order_kernel_ms=sum(r["median_min5_ms"]["noyau"] for r in orders_out.values()),
                                  m4_kernel_threshold_on_orders=all(r["median_min5_ms"]["noyau"] <= (10 if kmax == 5 else 35)
                                                                  for r in orders_out.values()),
                                  m4_parallel_contraction_threshold_on_orders=all(r["median_min5_ms"]["parallel_48"] <= 3
                                                                                for r in orders_out.values()))
            tables = pinned(directory + "/tableaux.md").decode()
            need(f"| {case} | {parts} | {t1/parts:.4f} |" in tables and
                 f"| {case} | tous |  | {sec['v11']:.3f} | {sec['replique_v11']:.3f} | {sec['replique_v12']:.3f} | "
                 f"{sec['replique_v12']/sec['replique_v11']:.3f} |" in tables, "published totals differ")
        mutant = jsonl(directory + f"/ng00_k{kmax}/mes_m4_mutant.jsonl")
        need(mutant[-1] == {"phase": "fin", "code": 1} and
             any(not r["identite"]["identiques"] for r in mutant if r.get("phase") == "ordre"), "M4 mutant vacuous")
    # These are facts about this historical capture, not a generic adoption judge.
    need(all(v["m4_kernel_threshold_on_orders"] for v in all_cases.values()), "kernel threshold observation")
    need(all(v["m4_parallel_contraction_threshold_on_orders"] == (case.endswith("_k5"))
             for case, v in all_cases.items()), "contraction threshold observation")
    need(all(v["resolution_ratio"] < .6 for case, v in all_cases.items() if case.endswith("_k10")), "M3 ratio observation")
    def find_hash_keys(value, prefix=""):
        if isinstance(value, dict):
            for key, child in value.items():
                if "sha256" in key:
                    binary_hash_keys.append(prefix + key)
                find_hash_keys(child, prefix + key + ".")
        elif isinstance(value, list):
            for child in value:
                find_hash_keys(child, prefix)
    for report in reports.values():
        find_hash_keys(report)
    need(all(k == "construction.libmhgp11_sha256" or k.startswith("vidages.") for k in binary_hash_keys),
         "historical hash inventory changed")
    raw_checks = dict(available=False, reason="published-only requested")
    if not args.published_only:
        location = Path(session["receipt_path"]).parent
        need(location.is_dir(), "local archives absent; use --published-only for limited verification")
        results = (location / "results/results.tar.gz").read_bytes()
        package = (location / "package/package.tar.gz").read_bytes()
        plan_bytes = (location / "package/plan.json").read_bytes()
        need(sha(results) == session["results_sha256"] and sha(package) == session["package_sha256"] and
             sha(plan_bytes) == session["plan_sha256"] and
             sha((location / "package/plan.sh").read_bytes()) == session["worker_plan_sha256"], "raw archive/plan mismatch")
        with tarfile.open(fileobj=io.BytesIO(package), mode="r:gz") as tar:
            names = {m.name: m for m in tar.getmembers() if m.isfile()}
            for row in source_files:
                need(tar.extractfile(names[row["path"]]).read() == pinned(row["path"]), "packaged source differs")
            need(not any(n.startswith(SOURCE) and any(p in n.split("/") for p in ("build", "out")) for n in names),
                 "preexisting M3/M4 outputs in source package")
        with tarfile.open(fileobj=io.BytesIO(results), mode="r:gz") as tar:
            names = {m.name: m for m in tar.getmembers() if m.isfile()}
            copies = 0
            for path in (ROOT / RECEIPT / "resultats").rglob("*"):
                if path.is_file():
                    relative = path.relative_to(ROOT / RECEIPT / "resultats").as_posix()
                    need(clean(tar.extractfile(names["results/" + relative]).read()) == pinned(RECEIPT + "/resultats/" + relative),
                         "published/raw content differs: " + relative)
                    copies += 1
            native_calls = 0
            for kmax, command_number, process_count in ((5, 1, 5), (10, 3, 3)):
                prefix = f"results/cmd/{command_number:03d}_m34_k{kmax}/"
                stdout = clean(tar.extractfile(names[prefix + "stdout"]).read()).decode()
                observed = re.findall(r"MES-M([34]) (ng0[012]) K(5|10) \(processus (\d+)/(\d+)\)", stdout)
                expected = {(str(m), f, str(kmax), str(p), str(process_count))
                            for m in (3, 4) for f in FRAMES for p in range(1, process_count + 1)}
                need(len(observed) == len(expected) and set(observed) == expected, "process command trace incomplete")
                need(len(re.findall(r"vidage ng0[012] K", stdout)) == 3, "dump process trace")
                argv = shlex.split(clean(tar.extractfile(names[prefix + "argv.txt"]).read()).decode())
                args_report = reports[kmax]["executions"][0]["arguments"]
                need(argv[argv.index("--processus") + 1] == str(process_count) and
                     argv[argv.index("--cas") + 1] == args_report["cas"] and
                     argv[argv.index("--construction") + 1] == args_report["construction"], "session argv/report mismatch")
                native_calls += len(observed)
            raw_checks = dict(available=True, results_sha256=sha(results), source_package_sha256=sha(package),
                              plan_sha256=sha(plan_bytes), public_files_equal_after_redaction=copies,
                              process_calls_in_stdout=native_calls, dump_calls=6,
                              results_archive_bytes=len(results), source_archive_bytes=len(package),
                              executable_files_retained=0, individual_executable_hashes_present=False)
    # Pin closure catches any change during the audit itself.
    for path, expected in HASHES.items():
        need(sha((ROOT / path).read_bytes()) == expected, "input changed during verification: " + path)
    result = dict(schema="ehgp.v12.audit_session_b.v1", pin=PIN, source_snapshot_head=SNAPSHOT,
                  verdict="observations_confirmed_formal_adoption_incomplete", cases=all_cases, counts=counts,
                  raw_evidence=raw_checks, public_manifest_files=len(manifest), sources_verified=source_files,
                  v11_archive_reproduced=archive_ref["sha256"], source_lib_sha256=source_receipt["libmhgp11_sha256"],
                  generic_cst0018_closed=False, cst0021_executable_hashes_missing=True,
                  cst0213_resolution_replicated=False, cst0214_campaign_t6_vacuous=False,
                  formal_adoption_complete=False, gcp_used_by_audit=False,
                  pinned_file_count=len(HASHES), input_hashes_rechecked_at_close=True,
                  input_hash_manifest_sha256=sha(json.dumps(HASHES, sort_keys=True).encode()),
                  key_inputs_sha256={p: v for p, v in HASHES.items()
                                     if p.endswith(("/receipt.json", "/SHA256SUMS", "/rapport_mes_m3_m4.json"))})
    print(json.dumps(result, ensure_ascii=False, indent=2, sort_keys=True, allow_nan=False))


if __name__ == "__main__":
    main()
