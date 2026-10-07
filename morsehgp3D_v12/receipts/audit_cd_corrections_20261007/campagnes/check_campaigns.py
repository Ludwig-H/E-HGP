#!/usr/bin/env python3
"""Contre-lecture C/D épinglée ; aucun build, GPU, nuage ou dump binaire.

--published-only omet explicitement les petites archives locales originales.
Les refus utilisent des exceptions et restent actifs sous python -O.
"""
import argparse
import hashlib
import io
import itertools
import json
import math
from pathlib import Path
import random
import re
import shlex
import statistics as st
import subprocess
import tarfile

PIN = "07ee13ef6bebc0b6b85da90207f3f067ffff1755"
ROOT = Path(__file__).resolve().parents[4]
HASHES = {}
FRAMES = ("ng00", "ng01", "ng02")
M5 = "morsehgp3D_v12/microbancs/mes_m5_parcours/"
M34 = "morsehgp3D_v12/microbancs/mes_m3_m4_tour/"


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def pinned(path, commit=PIN):
    data = subprocess.check_output(["git", "show", commit + ":" + path], cwd=ROOT)
    if commit == PIN:
        need((ROOT / path).read_bytes() == data, "worktree differs from pin: " + path)
        HASHES[path] = sha(data)
    return data


def load(path):
    return json.loads(pinned(path))


def jsonlines(data):
    return [json.loads(line) for line in data.decode().splitlines() if line.startswith("{")]


def clean(data):
    return re.sub(r"/home/[A-Za-z0-9_.-]+", "$HOME", data.decode()).encode()


def positive(values):
    need(bool(values) and all(type(x) in (int, float) and math.isfinite(x) and x > 0 for x in values), "invalid time")
    return values


def bootstrap(ratios, rng):
    logs = [math.log(x) for x in positive(ratios)]
    draws = sorted(sum(rng.choice(logs) for _ in logs) / len(logs) for _ in range(10000))
    exact = sorted(sum(x) / len(logs) for x in itertools.product(logs, repeat=len(logs)))
    return dict(gm=math.exp(sum(logs) / len(logs)), ci95=[math.exp(draws[250]), math.exp(draws[9749])],
                exhaustive_bootstrap_hi=math.exp(exact[math.ceil(.975 * len(exact)) - 1]))


def orders(rows, phase, expected):
    rows = [r for r in rows if r.get("phase") == phase]
    need(sorted(r["k"] for r in rows) == list(expected), "missing/duplicate order: " + phase)
    return {r["k"]: r for r in rows}


def receipt(letter, public_only):
    base = f"morsehgp3D_v12/receipts/g4_t0{letter}_20261007"
    doc = load(base + "/receipt.json")
    need(doc["status"] == "completed" and doc["results_verified"] and doc["data_verified_remote"] and
         doc["targeted_shutdown_certified"] and not doc["errors"], "session incomplete")
    lines = pinned(base + "/SHA256SUMS").decode().splitlines()
    for line in lines:
        digest, name = line.split(None, 1)
        need(sha(pinned(base + "/" + name)) == digest, "public checksum mismatch: " + name)
    need(len(lines) == (158 if letter == "c" else 165), "public manifest count")
    raw = {}; raw_info = dict(available=False, reason="published-only requested")
    if not public_only:
        location = Path(doc["receipt_path"]).parent
        need(location.is_dir(), "local archives unavailable; --published-only gives limited replay")
        result_bytes = (location / "results/results.tar.gz").read_bytes()
        package_bytes = (location / "package/package.tar.gz").read_bytes()
        plan_bytes = (location / "package/plan.json").read_bytes()
        need(sha(result_bytes) == doc["results_sha256"] and sha(package_bytes) == doc["package_sha256"] and
             sha(plan_bytes) == doc["plan_sha256"] and sha((location / "package/plan.sh").read_bytes()) ==
             doc["worker_plan_sha256"], "raw archive/plan checksum")
        with tarfile.open(fileobj=io.BytesIO(result_bytes)) as tar:
            raw = {m.name: tar.extractfile(m).read() for m in tar.getmembers() if m.isfile() and
                   (m.name.startswith("results/cmd/") or m.name in ("results/commands.tsv", "results/data_check.txt"))}
        copies = 0
        for path in (ROOT / base / "resultats").rglob("*"):
            if path.is_file():
                name = path.relative_to(ROOT / base / "resultats").as_posix()
                if "results/" + name not in raw:  # harmless VM-facts file is read only when explicitly needed
                    with tarfile.open(fileobj=io.BytesIO(result_bytes)) as tar:
                        raw["results/" + name] = tar.extractfile("results/" + name).read()
                need(clean(raw["results/" + name]) == pinned(base + "/resultats/" + name), "raw/public discrepancy: " + name)
                copies += 1
        source_prefix = M5 if letter == "c" else M34
        sources = [r for r in doc["source"]["manifest"] if r["path"].startswith(source_prefix)]
        with tarfile.open(fileobj=io.BytesIO(package_bytes)) as tar:
            for row in sources:
                data = tar.extractfile(row["path"]).read()
                need(sha(data) == row["sha256"] and data == pinned(row["path"]), "source package differs")
        raw_info = dict(available=True, results_sha256=sha(result_bytes), package_sha256=sha(package_bytes),
                        plan_sha256=sha(plan_bytes), raw_public_copies=copies, source_files=len(sources),
                        results_bytes=len(result_bytes), package_bytes=len(package_bytes))
    return base, doc, raw, raw_info


def audit_c(base, session, raw):
    directory = base + "/resultats/cmd/001_m5/files/m5"
    report = load(directory + "/report.json")
    cases = [f"{f}_k{k}_l{l}" for f in FRAMES for k, l in ((5, 16), (5, 24), (10, 24))] + [
        f"uniform_u18_n{n}_k5_l24" for n in (8000, 16000, 32000)]
    deciding = [c for c in cases if c.startswith("ng") and c.endswith("l24")]
    need(report["cases"] == cases and report["evidence"]["deciding"] == deciding and
         report["verdict"] == "adopte" and not report["refused"] and not report["rejected"], "C case grid/verdict")
    need(report["args"]["processes"] == 5 and report["args"]["v11_passes"] == 10 and
         report["args"]["reps"] == 15 and report["args"]["warmup"] == 3 and
         report["gpu_isolation"] and report["rule"]["threshold"] == .25, "C protocol")
    need(len(report["steps"]) == 171 and all(r["code"] == 0 for r in report["steps"]), "C execution step")
    for name, digest in report["hashes"].items():
        if name.startswith(("m2/", "m5/")):
            prefix = M5 if name.startswith("m5/") else "morsehgp3D_v12/microbancs/mes_m2_feuille/"
            need(sha(pinned(prefix + name[3:])) == digest == sha(pinned(prefix + name[3:], session["source"]["head_commit"])),
                 "C measured source hash")
    host = report["identity_host"]["results"]
    need(report["identity_host"]["code"] == 0 and host == jsonlines(pinned(directory + "/identity_host.json")), "C host raw proof")
    fixtures = {r["name"]: r for r in report["fixtures"]["fixtures"]}
    expected_fixtures = {"coquille24_k2_l16", "coquille48_k5_l24", "coquille48_k5_l8_m8", "coquille48_u32_k5_l24",
                         "bord_u32_k2_l5", "uniforme_u32_k3_l8"}
    need(set(fixtures) == expected_fixtures and all(x["present"] and x["controls_ok"] for x in fixtures.values()), "C fixture coverage")
    host_cases = {Path(r["dump"]).stem: r for r in host if r.get("phase") == "identity"}
    need(set(host_cases) == set(cases) | expected_fixtures | {"ng00_k5_l24_crop4000"}, "C host case coverage")
    need(host[0]["ok"] and all(r["identity"] and
                              (r["nodes_equal"] or (name == "coquille48_k5_l8_m8" and r["status"] == r["reference_status"] == 1)) and
                              r["missing"] == r["extra"] == r["list_mismatch"] == r["meta_mismatch"] == 0
                              for name, r in host_cases.items()), "C host identity")
    device = load(directory + "/device_fixtures.json")
    need(report["device_fixtures"]["code"] == 0 and device == report["device_fixtures"]["result"] and device["identity"], "C device fixtures")
    fx_device = {Path(r["dump"]).stem: r for r in device["cases"]}
    need(set(fx_device) == expected_fixtures and all(r["identity"] for r in fx_device.values()), "C GPU fixture grid")
    need(all(r["missing"] == r["extra"] == r["list_mismatch"] == r["meta_mismatch"] == 0 and
             r["leaves"] == r["reference_leaves"] and r["digest"] == r["reference_digest"] and
             r["status"] == host_cases[name]["reference_status"] for name, r in fx_device.items()),
         "C GPU fixture discrepancy/domain")
    mutant_names = set(next(iter(host_cases.values()))["mutants"])
    need(len(mutant_names) == 6 and all(any(r["mutants"][m]["killed"] for r in host_cases.values()) for m in mutant_names), "C host mutants")
    need(host_cases["coquille48_u32_k5_l24"]["mutants"]["repere_enfant"]["killed"] and
         fx_device["coquille48_u32_k5_l24"]["mutants"]["repere_enfant"]["killed"], "C required u32 mutant")
    for tool in ("memcheck", "racecheck", "synccheck"):
        san = load(directory + "/sanitizer_" + tool + ".json")
        need(report["sanitizer"][tool]["code"] == 0 and san["identity"] and
             {Path(x["dump"]).stem for x in san["cases"]} ==
             {"ng00_k5_l24_crop4000", "coquille48_u32_k5_l24", "uniforme_u32_k3_l8"}, "C sanitizer domain")
        need(all(r["identity"] and r["status"] == 0 and r["missing"] == r["extra"] == r["list_mismatch"] ==
                 r["meta_mismatch"] == 0 and r["leaves"] == r["reference_leaves"] and
                 r["digest"] == r["reference_digest"] == host_cases[Path(r["dump"]).stem]["reference_digest"]
                 for r in san["cases"]), "C sanitizer geometric discrepancies")
        marker = "0 errors, 0 warnings" if tool == "racecheck" else "ERROR SUMMARY: 0 errors"
        need(marker in report["sanitizer"][tool]["tail"], "C sanitizer result")
        if raw:
            log = clean(raw["results/cmd/001_m5/files/m5/logs/sanitizer_" + tool + ".log"]).decode()
            need(marker in log and "--error-exitcode 9" in log, "C raw sanitizer log")
    rng = random.Random(20261007); stats = {}; raw_commands = 0
    for case in cases:
        ratios = []; resident_ratios = []; gpu_times = []; cpu_times = []
        dump = report["dumps"][case]["summary"]
        need(report["dumps"][case]["code"] == 0, "C dump failed")
        for p in range(6):
            gpu = load(directory + f"/runs/{case}_p{p}_gpu.json")
            cpu = load(directory + f"/runs/{case}_p{p}_v11.json")
            need(gpu["identity"] and gpu["reps"] == 15 and gpu["warmup"] == 3 and len(gpu["cases"]) == 1, "C GPU process")
            g = gpu["cases"][0]
            need(Path(g["dump"]).stem == case and g["identity"] and g["status"] == g["timed_allocations"] == 0 and
                 g["missing"] == g["extra"] == g["list_mismatch"] == g["meta_mismatch"] == 0 and
                 g["digest"] == g["reference_digest"] == host_cases[case]["digest"], "C GPU input/identity")
            need(g["leaves"] == g["reference_leaves"] == dump["leaves"] and g["sites"] == dump["sites"] and
                 (g["kmax"], g["leaf_size"]) == (dump["kmax"], dump["leaf_size"]), "C GPU dump cardinalities")
            need(cpu["passes"] == 10 and cpu["workers"] == 48 and len(cpu["traversal_ns"]) == 10 and
                 cpu["catalogue_nodes"] == g["ledger"]["nodes"] and
                 cpu["catalogue_leaves"] == g["leaves"] and
                 cpu["catalogue_filter_tests"] == g["ledger"]["filter_tests"], "C CPU reference identity")
            need(all(cpu["walk_" + key] == (g["ledger"][key] if p == 0 else 0)
                     for key in ("nodes", "leaves", "filter_tests")), "C separate walk diagnostic scope")
            need(cpu["traversal_ns"] == [a + b for a, b in zip(cpu["prefix_ns"], cpu["single_pass_ns"])], "C CPU metric scope")
            need(len(g["total_ms"]) == len(g["resident_ms"]) == 15, "C GPU repetitions")
            t = st.median(positive(g["total_ms"])); tr = st.median(positive(g["resident_ms"]))
            v = st.median(positive(cpu["traversal_ns"][1:])) / 1e6
            need(t == g["median_ms"]["total"] and tr == g["median_ms"]["resident"], "C GPU declared median")
            if p:
                rr = report["rounds"][case][p - 1]
                need(rr["round"] == p and rr["gpu_ms"] == t and rr["gpu_resident_ms"] == tr and
                     rr["v11_ms"] == v and rr["gpu_identity"] and rr["v11_ledger_ok"], "C round differs from raw times")
                ratios.append(t / v); resident_ratios.append(tr / v); gpu_times.append(t); cpu_times.append(v)
            if raw:
                for arm in ("gpu", "v11"):
                    log = clean(raw[f"results/cmd/001_m5/files/m5/logs/{arm}_{case}_p{p}.log"]).decode()
                    command = shlex.split(log.splitlines()[0][2:])
                    if arm == "gpu":
                        need(command[-1].endswith(f"/{case}_p{p}_{arm}.json") and
                             command[command.index("--dump") + 1] == g["dump"] and
                             command[command.index("--reps") + 1] == "15" and
                             command[command.index("--warmup") + 1] == "3", "C GPU command attachment")
                    else:
                        need(command[3:7] == [str(g["kmax"]), str(g["leaf_size"]), "48", "10"] and
                             jsonlines(log.encode()) == [cpu], "C CPU command/stdout attachment")
                    raw_commands += 1
        result = bootstrap(ratios, rng); reference = report["stats"][case]
        need(abs(result["gm"] - reference["ratio_gm"]) < 1e-12 and
             all(abs(a-b) < 1e-12 for a,b in zip(result["ci95"], reference["ci95"])), "C stats mismatch")
        if case in deciding:
            need(result["ci95"][1] <= .25 and result["exhaustive_bootstrap_hi"] <= .25, "C adoption threshold")
        result.update(deciding=case in deciding, gpu_ms=st.median(gpu_times), cpu_ms=st.median(cpu_times),
                      resident_gm=math.exp(sum(math.log(r) for r in resident_ratios)/5))
        stats[case] = result
    return dict(verdict="local_M5_adoption_confirmed", cases=12, deciding_cases=6, retained_rounds=60,
                discarded_rounds=12, gpu_retained_total_times=900, cpu_retained_timed_passes=540,
                host_identity_cases=19, fixtures=6, source_hashes=len(report["hashes"]),
                binary_hashes={k:v for k,v in report["hashes"].items() if k.startswith("bin/")},
                raw_process_commands_checked=raw_commands, statistics=stats,
                generic_cst0018_closed=False, sanitizer_scope="crop4000 + two u32 fixtures")


def audit_d(base, session, raw):
    directories = {10: base + "/resultats/cmd/007_m34_k10_publier/files/m34_k10",
                   5: base + "/resultats/cmd/009_m34_k5_publier/files/m34_k5"}
    final = {k: load(path + "/rapport_mes_m3_m4.json") for k, path in directories.items()}
    intermediate = load(base + "/resultats/cmd/005_m34_k10_publier_resolution/files/m34_k10/rapport_mes_m3_m4.json")
    need(intermediate["verdicts"]["mes_m3"]["verdict"] == "refuse" and
         final[10]["verdicts"]["mes_m3"]["verdict"] == "adopte" and
         final[5]["verdicts"]["mes_m3"]["verdict"] == "refuse", "D intermediate/final judgment history")
    need(intermediate["resolution"] == final[10]["resolution"], "D resolution rewritten after interim publication")
    binaries = final[10]["construction"]["binaires"]
    need(len(binaries) == 5 and binaries == final[5]["construction"]["binaires"], "D executable set changed")
    all_proofs = {}; redacted_only = set(); execution_ids = set(); cases = {}; m4_cases = {}
    counts = dict(resolution_processes=0, m3_processes=0, m4_processes=0, m3_order_rows=0,
                  m4_order_rows=0, t6_births_once_per_case=0, resolution_orders=0)
    for kmax, report in final.items():
        directory = directories[kmax]
        construction = report["construction"]
        need(construction["code"] == 0 and report["verdicts"]["mes_m4"]["verdict"] == "conforme", "D construction/conformity")
        for execution in report["executions"]:
            need(execution["code"] == 0 and execution["campagne"] not in execution_ids, "D campaign reused/failed")
            execution_ids.add(execution["campagne"])
            for name, digest in execution["sources_sha256"].items():
                need(sha(pinned(M34 + name)) == digest == sha(pinned(M34 + name, session["source"]["head_commit"])), "D source pin")
        for name, digest in construction["dependances"]["fichiers"].items():
            prefix, relative = name.split(":", 1)
            path = M34 + relative if prefix == "0" else "morsehgp3D_v11/" + relative
            need(sha(pinned(path, PIN if prefix == "0" else "ac081a06f")) == digest, "D compiled dependency")

        def proof(block, binary):
            prov = block["binaire"]
            need(prov["binaire"] == binary and prov["sha256"] == binaries[binary] and
                 prov["construit"] and prov["inchange"] and Path(prov["commande"][0]).name == binary,
                 "D executable provenance")
            need(block["journal"] == prov["journal"] and block["journal_sha256"] == prov["journal_sha256"], "D journal provenance")
            path = directory + "/" + block["journal"]
            public = pinned(path)
            if raw:
                original = raw["results/" + path.split("/resultats/", 1)[1]]
                need(sha(original) == block["journal_sha256"] and clean(original) == public, "D raw journal hash")
            elif sha(public) != block["journal_sha256"]:
                redacted_only.add(path)
            all_proofs[path] = block["journal_sha256"]
            lines = jsonlines(public)
            if "lignes" in block:
                need(lines == block["lignes"], "D raw journal/report differs")
            return lines

        for name, gate in report["portes"].items():
            binary = "mhgp12_" + name
            lines = proof(gate, binary)
            mutant = "mutant" in name
            need(gate["conforme"] and gate["code"] == gate["code_attendu"] == int(mutant) and
                 bool(lines) and (lines[-1]["ecarts"] > 0) == mutant, "D gate or mutant vacuous")
        rng = random.Random(20261007)
        for frame in FRAMES:
            case = f"{frame}_k{kmax}"
            dump = report["vidages"][case]
            lines = proof(dump, "mhgp12_vidage")
            need(dump["conforme"] and dump["code"] == 0 and dump["ful1_identique"] and
                 dump["ful1_sha256"] == dump["ful1_reference"] and
                 lines[-1]["phase"] == "exit" and lines[-1]["status"] == "ok", "D reference dump")
            ref = {name: r["sha256"] for name, r in dump["fichiers"].items()}
            seed_rows = orders(lines, "graines", range(1, kmax + 1))
            need(all(r["identiques"] and r["journal_v11_compare"] for r in seed_rows.values()), "D seed reference")
            camp_ids = report["resolution"][case]["campagnes"]
            need(len(camp_ids) == 1, "D ambiguous resolution campaign")
            camp_id, campaign = next(iter(camp_ids.items()))
            need(camp_id in execution_ids and campaign["processus_demandes"] == campaign["prises_valides"] == 5 and
                 len(campaign["prises"]) == 5 and not campaign["refus"] and
                 campaign["vidages_reference"] == ref and campaign["passes_par_processus"] == (1 if kmax == 10 else 3),
                 "D resolution grid/provenance")
            ratios = []; process_journals = set()
            for index, take in enumerate(campaign["prises"]):
                need(take["processus"] == index and take["valide"] and take["code"] == 0 and
                     take["vidages_identiques_a_la_reference"] and not take["raisons"] and
                     f"/resolution/{camp_id}/p{index}/" in take["journal"], "D fresh resolution slot")
                process_journals.add(take["journal"])
                rows = proof(take, "mhgp12_vidage")
                o = orders(rows, "resolution_un_fil", range(2, kmax + 1))
                need(rows[-1]["phase"] == "exit" and rows[-1]["status"] == "ok" and
                     all(r["graines_identiques"] for r in o.values()), "D resolution identity/incomplete")
                command = take["binaire"]["commande"]
                need(command[3:6] == [frame, str(kmax), str(24 if kmax == 10 else 16)] and
                     command[command.index("--chrono-resolution") + 1] == str(campaign["passes_par_processus"]) and
                     command[7].endswith(f"/{case}/resolution/{camp_id}/p{index}"), "D resolution command domain")
                seconds = {arm: sum(positive([r["secondes"][arm] for r in o.values()]))
                           for arm in ("v11", "replique_v11", "replique_v12")}
                need(all(abs(seconds[arm] - take["totaux"][arm]) < 1e-12 for arm in seconds), "D resolution total differs from raw")
                ratio = seconds["replique_v12"] / seconds["replique_v11"]
                need(abs(ratio - take["rapport_total"]) < 1e-14, "D raw resolution ratio")
                ratios.append(ratio); counts["resolution_processes"] += 1; counts["resolution_orders"] += len(o)
            need(len(process_journals) == 5, "D repeated journal")
            statistic = bootstrap(ratios, rng)
            published = report["verdicts"]["mes_m3"]["cas" if kmax == 10 else "cas_publies_sans_decision"][case]
            need(abs(statistic["gm"] - published["moyenne_geometrique"]) < 1e-12 and
                 all(abs(a-b) < 1e-12 for a,b in zip(statistic["ci95"], published["ic95"])), "D M3 statistic mismatch")
            if kmax == 10:
                need(statistic["ci95"][1] <= .6 and statistic["exhaustive_bootstrap_hi"] <= .6, "D M3 threshold")
            statistic.update(processes=5, passes=campaign["passes_par_processus"], deciding=kmax == 10, ratios=ratios)
            cases[case] = statistic
            m4_rows = []
            for measure, expected_count in ((3, 3 if kmax == 10 else 5), (4, 5)):
                block = report[f"mes_m{measure}"][case]
                need(block["complet"] and block["identique"] and block["code"] == 0 and not block["refus"] and
                     block["vidages_sha256"] == ref and len(block["processus"]) == expected_count, "D M3/M4 proof coverage")
                journal_names = set()
                for process in block["processus"]:
                    lines = proof(process, f"mhgp12_mes_m{measure}")
                    need(process["code"] == 0 and lines[-1] == {"phase": "fin", "code": 0} and
                         lines[0]["trame"] == frame and lines[0]["K"] == kmax, "D measured process domain")
                    o = orders(lines, "ordre", range(2 if measure == 3 else 1, kmax + 1))
                    need(all(r["identite"]["identiques"] and
                             all(v == 0 for key,v in r["identite"].items() if key.endswith("ecarts")) for r in o.values()), "D identity discrepancy")
                    for order, row in o.items():
                        inv = dump["fichiers"][f"ordre_{order}.bin"]["inventaire"]["sections"]
                        if measure == 3:
                            need(row["parties"] == inv["PARTINF"]["nombre"] == sum(row["routes"].values()), "D M3 population")
                        else:
                            need(row["naissances"] == inv["BIRTHS"]["nombre"] and
                                 row["cellules"] == inv["CELLS"]["nombre"] and
                                 row["representants"] == inv["SEEDS"]["nombre"] and row["noeuds_v11"] ==
                                 dump["fichiers"][f"foret_{order}.bin"]["inventaire"]["sections"]["FNODES"]["nombre"],
                                 "D M4 population")
                    if measure == 4:
                        need(all(r["lem_t6"]["ecarts"] == 0 and r["lem_t6"]["naissances_jugees"] ==
                                 (r["naissances"] if k > 1 else 0) and r["contraction_parallele"]["identique"] and
                                 r["contraction_parallele"]["fils"] == 48 for k,r in o.items()), "D T6/parallel identity")
                        m4_rows.append(o)
                    journal_names.add(process["journal"])
                    counts[f"m{measure}_processes"] += 1; counts[f"m{measure}_order_rows"] += len(o)
                need(len(journal_names) == expected_count, "D duplicate process log")
            krows = [p[kmax] for p in m4_rows]
            t_kernel = st.median(positive([r["temps_un_fil_s"]["noyau"] * 1000 for r in krows]))
            t_contract = st.median(positive([r["contraction_parallele"]["secondes"] * 1000 for r in krows]))
            published = report["verdicts"]["mes_m4"]["cas"][case]["temps"]
            need(abs(t_kernel - published["noyau_ms_mediane"]) < 1e-12 and
                 abs(t_contract - published["contraction_parallele_ms_mediane"]) < 1e-12, "D M4 median mismatch")
            counts["t6_births_once_per_case"] += sum(r["lem_t6"]["naissances_jugees"] for r in m4_rows[0].values())
            m4_cases[case] = dict(processes=5, kernel_ms=t_kernel, parallel_48_ms=t_contract,
                                 kernel_threshold_met=t_kernel <= (35 if kmax == 10 else 10),
                                 contraction_threshold_met=t_contract <= 3)
        for name, mutant in report["mes_m4"].items():
            if name.startswith("mutant"):
                rows = proof(mutant, "mhgp12_mes_m4_mutant_sans_contraction")
                need(rows[0] == {"phase": "entree", "trame": "ng00", "K": kmax, "repetitions": 1,
                                 "mutant_sans_contraction": True}, "D real mutant header/domain")
                mutant_orders = orders(rows, "ordre", range(1, kmax + 1))
                reference_files = report["vidages"][f"ng00_k{kmax}"]["fichiers"]
                need(mutant["vidages_sha256"] == {n: x["sha256"] for n,x in reference_files.items()}, "D real mutant dump attachment")
                for order, row in mutant_orders.items():
                    inv = reference_files[f"ordre_{order}.bin"]["inventaire"]["sections"]
                    need(row["naissances"] == inv["BIRTHS"]["nombre"] and row["cellules"] == inv["CELLS"]["nombre"] and
                         row["representants"] == inv["SEEDS"]["nombre"] and row["noeuds_v11"] ==
                         reference_files[f"foret_{order}.bin"]["inventaire"]["sections"]["FNODES"]["nombre"],
                         "D real mutant counters from another population")
                need(mutant["tue"] and mutant["code"] == 1 and rows[-1] == {"phase": "fin", "code": 1} and
                     any(not r["identite"]["identiques"] for r in rows if r.get("phase") == "ordre"), "D real mutant vacuous")
    need(counts["resolution_processes"] == 30 and counts["m4_processes"] == 30 and counts["m3_processes"] == 24,
         "D whole campaign replication")
    need(all(c["kernel_threshold_met"] and c["contraction_threshold_met"] == case.endswith("_k5")
             for case,c in m4_cases.items()), "D performance observations changed")
    return dict(verdict="M3_K10_adoption_confirmed_M4_conformity_confirmed", counts=counts, resolution=cases,
                m4=m4_cases, campaign_nonces=len(execution_ids), journal_proofs=len(all_proofs),
                journal_hashes_need_unredacted_archive=len(redacted_only), binaries_sha256=binaries,
                interim_M3="refuse_missing_M3_identity", final_K5_M3="refuse_outside_deciding_population",
                cst0213_session_D_resolution_gap_closed=True, cst0021_session_D_binary_attachment_present=True,
                historical_session_B_not_requalified=True, real_M4_mutant_full_domain_checked=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--published-only", action="store_true")
    args = parser.parse_args()
    result = dict(schema="ehgp.v12.audit_sessions_cd.v1", pin=PIN, gcp_used_by_audit=False)
    for letter, audit in (("c", audit_c), ("d", audit_d)):
        base, session, raw, raw_info = receipt(letter, args.published_only)
        result[letter] = audit(base, session, raw)
        result[letter]["source_snapshot_head"] = session["source"]["head_commit"]
        result[letter]["archives"] = raw_info
    for path, digest in HASHES.items():
        need(sha((ROOT / path).read_bytes()) == digest, "input changed during audit: " + path)
    result["pin_closure"] = dict(files_rechecked=len(HASHES),
                                 manifest_sha256=sha(json.dumps(HASHES, sort_keys=True).encode()))
    print(json.dumps(result, ensure_ascii=False, sort_keys=True, indent=2, allow_nan=False))


if __name__ == "__main__":
    main()
