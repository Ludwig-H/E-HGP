#!/usr/bin/env python3
"""Explicit parallel port of the frozen FULL weighted Gaussian pilot.

Only scheduling and gzip encoding change. Each isolated worker computes one
whole (case,K), with the frozen model and 2x2 selection grid. Interrupted R2
units are reusable only with all 14 rows and all six payloads hash-verified.
The original failed receipt is never edited or promoted to completed.
"""
from __future__ import annotations

import argparse
from collections import Counter
import gzip
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time
import traceback

HERE = Path(__file__).resolve().parent
V9 = HERE.parents[1]
PREVIOUS = V9 / "audits/b_gaussian_point_clustering_20260927"
FIRST = V9 / "audits/b_point_hierarchy_k_20260927"
sys.path.extend((str(PREVIOUS), str(FIRST)))
from benchmark import clean, metrics
from evaluation import evaluate_labels
from weighted_model import build_facet_model, vote_points
from weighted_eom import weighted_condense_eom
from full_weighted_tree import build_full_weighted_tree, eom_input

CASES = tuple(f"spherical_g{g}_d{delta}_s{seed}"
             for g, delta in ((2, 8), (8, 4), (16, 2)) for seed in (1, 2, 3)) + tuple(
    f"{regime}_g8_d4_s{seed}" for regime in ("anisotropic", "unbalanced") for seed in (1, 2))
KS, SIZES, EXPONENTS = (5, 10), (20, 50), (1, 2)
PLAN = dict(cases=list(CASES), k=list(KS), sizes=list(SIZES), exp_z=list(EXPONENTS))
PREVIOUS_RECEIPT_SHA = "8f9aba99396b6341f7a6872956e66f50a7776cc1535d9a3f9137907d2945157a"
MANIFEST_SHA = "1cab6404055aebf0dfa31b76f16f8a4c84e4acf9678b2b910dabd9f367f46c67"
SERIAL_SOURCE_SHA = "30bab0dc0d3009ac68eb8434e01888d56d8ce83cfcacf0bab636a8a422d18d9f"
ROW_KEYS = ("case", "regime", "communities", "separation", "seed", "n", "k",
            "min_cluster_size", "exp_z", "method", "metrics", "extra")
THREAD_ENV = {key: "1" for key in ("OPENBLAS_NUM_THREADS", "OMP_NUM_THREADS", "MKL_NUM_THREADS",
                                  "NUMEXPR_NUM_THREADS", "VECLIB_MAXIMUM_THREADS")}


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(path):
    result = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def save(path, value):
    with Path(path).open("x") as stream:
        json.dump(clean(value), stream, sort_keys=True, indent=2, allow_nan=False)
        stream.write("\n")


def save_gzip(path, value):
    # Same strict sorted JSON bytes as the serial encoder, one bulk compression.
    encoded = json.dumps(clean(value), sort_keys=True, allow_nan=False,
                         separators=(",", ":")).encode()
    with Path(path).open("xb") as raw:
        with gzip.GzipFile(filename="", mode="wb", fileobj=raw, mtime=0, compresslevel=1) as zipped:
            zipped.write(encoded)


def verify(pins, label):
    for path, expected in pins.items():
        need(sha(path) == expected, label + ": " + path)


def comparator_rows(baseline, case_id, k):
    return [{key: row[key] for key in ROW_KEYS} for row in baseline["rows"]
            if row["case"] == case_id and row["k"] == k and row["min_cluster_size"] in SIZES]


def check_rows(rows, baseline, case_id, k):
    need(len(rows) == 14 and all(row["case"] == case_id and row["k"] == k and row["n"] == 1200
                               for row in rows), "whole 14-row unit")
    weighted = [r for r in rows if r["method"] == "hgp_weighted_full_vote"]
    need(len(weighted) == 4 and {(r["min_cluster_size"], r["exp_z"]) for r in weighted} ==
         {(m, z) for m in SIZES for z in EXPONENTS}, "whole weighted selection grid")
    other = [{key: row[key] for key in ROW_KEYS} for row in rows if row["method"] != "hgp_weighted_full_vote"]
    need(other == comparator_rows(baseline, case_id, k) and len(other) == 10, "immutable comparator rows")


def payload_names():
    return [f"measure_z{z}.json.gz" for z in EXPONENTS] + [
        f"weighted_m{m}_z{z}.json.gz" for z in EXPONENTS for m in SIZES]


def native_argv(context, case, k):
    return [context["native_binary"], "--input", case["points_u32le"], "--k", str(k), "--workers", "1"]


def worker(spec_path):
    spec = json.loads(spec_path.read_text())
    context, case, k = spec["context"], spec["case"], spec["k"]
    directory = Path(spec["directory"])
    directory.mkdir()
    result = dict(status="running", rows=[], commands=[], artifacts={}, worker_pid=os.getpid())
    started = time.monotonic()
    try:
        verify(context["sources_before"], "worker source preflight")
        verify(context["fixed_pins"], "worker fixed preflight")
        verify({case[key]: value for key, value in case["prepared_sha256"].items()}, "worker input")
        truth = json.loads(Path(case["labels_json"]).read_text())
        need(len(truth) == case["n"] == 1200 and k in KS, "whole Gaussian scene")
        baseline = json.loads(Path(context["baseline_receipt"]).read_text())
        baseline_commands = {(c["case"], c["k"]): c for c in baseline["commands"]}
        argv = native_argv(context, case, k)
        save(directory / "intent.json", dict(argv=argv, binary_sha256=context["native_binary_sha256"],
             input_sha256=sha(case["points_u32le"])))
        print("START", case["id"], k, flush=True)
        start = time.monotonic()
        with (directory / "native.json").open("xb") as out, (directory / "native.stderr").open("xb") as err:
            command = subprocess.run(argv, stdout=out, stderr=err, check=False)
        result["commands"].append(dict(case=case["id"], k=k, argv=argv, returncode=command.returncode,
            elapsed_seconds=time.monotonic()-start, stdout_sha256=sha(directory/"native.json"),
            stderr_sha256=sha(directory/"native.stderr")))
        save(directory / "command.json", result["commands"][-1])
        need(command.returncode == 0, "native export failed")
        attachment_export = json.loads((directory / "native.json").read_text())
        need(attachment_export["schema"] == "mhgp9_weighted_full_attachment_export_v1" and
             attachment_export["status"] == "completed", "FULL attachment status/schema")
        payload = attachment_export["weighted"]
        need(payload["schema"] == "mhgp9_weighted_catalogue_export_v1" and
             payload["status"] == "completed" and payload["catalog_universe"] == "gabriel_complete_boundary",
             "weighted catalogue status/schema")
        native = payload["native"]
        need(native["status"] == "completed" and native["point_count"] == 1200 and native["k"] == k,
             "native request binding")
        old_native_path = Path(context["baseline_receipt"]).parent / f"{case['id']}_k{k}" / "native.json"
        need(sha(old_native_path) == baseline_commands[(case["id"], k)]["stdout_sha256"], "baseline native pin")
        old_native = json.loads(old_native_path.read_text())
        for key in ("points", "nodes", "roots", "populations", "contributions", "tower_digest", "catalogue_digest"):
            need(native[key] == old_native[key], "changed fixed-K object: " + key)
        result["artifacts"][str(old_native_path)] = sha(old_native_path)
        cofaces = [dict(vertices=row["vertices"], beta=row["beta"]) for row in payload["cofaces"]]
        for z in EXPONENTS:
            start = time.monotonic()
            model = build_facet_model(1200, k, cofaces, exp_z=z)
            naive_ms = 1000*(time.monotonic()-start)
            need([list(face) for face in model["facets"]] == [r["vertices"] for r in attachment_export["attachments"]],
                 "facet order attachment binding")
            start = time.monotonic()
            tree = build_full_weighted_tree(native["nodes"], native["roots"], model["masses"],
                [dict(node=row["node"], beta=row["beta"]) for row in attachment_export["attachments"]])
            tree_ms = 1000*(time.monotonic()-start)
            need(len(tree["tree"]["roots"]) == 1, "connected FULL weighted facet restriction")
            path = directory / f"measure_z{z}.json.gz"
            save_gzip(path, dict(facets=model["facets"], scores=model["scores"], point_totals=model["point_totals"],
                masses=model["masses"], children=tree["tree"]["children"], squared_levels=tree["tree"]["squared_levels"],
                leaf_birth_betas=tree["leaf_birth_betas"], attachments=tree["attachments"]))
            result["artifacts"][str(path)] = sha(path)
            for minimum in SIZES:
                start = time.monotonic()
                selected = weighted_condense_eom(**eom_input(tree, min_cluster_size=minimum), exp_z=z)
                selection_ms = 1000*(time.monotonic()-start)
                start = time.monotonic()
                vote = vote_points(model, selected["labels"])
                vote_ms = 1000*(time.monotonic()-start)
                counts = Counter(x for x in vote["labels"] if x >= 0)
                result["rows"].append(dict(case=case["id"], regime=case["regime"], communities=case["communities"],
                    separation=case["separation"], seed=case["seed"], n=1200, k=k, min_cluster_size=minimum,
                    exp_z=z, method="hgp_weighted_full_vote", metrics=metrics(truth, vote["labels"]),
                    extra=evaluate_labels(truth, vote["labels"]), model_statistics=model["statistics"],
                    full_weighted_statistics=tree["statistics"], naive_gabriel_components=len(model["roots"]),
                    native_statistics=payload["stats"], attachment_statistics=attachment_export["stats"],
                    times_ms=dict(model=naive_ms+tree_ms, naive_mass_and_graph=naive_ms,
                        full_attachment_tree=tree_ms, selection=selection_ms, vote=vote_ms),
                    final_point_sizes=sorted(counts.values()),
                    final_point_clusters_below_mass_threshold=sum(v < minimum for v in counts.values()),
                    near_threshold_nodes=len(selected["near_threshold_nodes"]),
                    vote_ties=sum(x == 0 and label >= 0 for x, label in zip(vote["raw_margins"], vote["labels"]))))
                path = directory / f"weighted_m{minimum}_z{z}.json.gz"
                save_gzip(path, dict(selection=selected, vote=vote))
                result["artifacts"][str(path)] = sha(path)
        result["rows"].extend(comparator_rows(baseline, case["id"], k))
        check_rows(result["rows"], baseline, case["id"], k)
        verify(context["sources_before"], "worker source closure")
        verify(context["fixed_pins"], "worker fixed closure")
        result["status"] = "completed"
        print("DONE", case["id"], k, "cofaces", len(cofaces), "facets", len(model["facets"]), flush=True)
    except BaseException as error:
        result.update(status="failed", error=repr(error), traceback=traceback.format_exc())
        raise
    finally:
        result["elapsed_seconds"] = time.monotonic()-started
        save(directory / "worker_receipt.json", result)


def prepare(args):
    baseline_path = args.baseline.resolve() / "receipt.json"
    need(sha(baseline_path) == PREVIOUS_RECEIPT_SHA, "baseline receipt pin")
    baseline = json.loads(baseline_path.read_text())
    qualification = json.loads(args.qualification.read_text())
    for inherited in (baseline, qualification):
        need(inherited["sources_before"] == inherited["sources_after"], "inherited source closure")
        verify(inherited["sources_after"], "inherited sources")
    need(qualification["status"] == "passed" and sha(args.native) == qualification["native_binary_sha256"],
         "qualified native required")
    manifest_path = Path(baseline["manifest_path"])
    need(sha(manifest_path) == MANIFEST_SHA, "manifest pin")
    by_case = {c["id"]: c for c in json.loads(manifest_path.read_text())["cases"]}
    need(len(CASES) == len(set(CASES)) == 13 and all(c in by_case for c in CASES), "fixed cases")
    dependencies = [HERE / name for name in (Path(__file__).name, "benchmark_full_weighted.py", "weighted_model.py",
        "weighted_eom.py", "full_weighted_tree.py", "qualify_full_attachments.py")]
    dependencies += [PREVIOUS / name for name in ("evaluation.py", "condensed.py")]
    dependencies += [PREVIOUS / "benchmark.py"] if (PREVIOUS / "benchmark.py").exists() else []
    dependencies += [FIRST / name for name in ("benchmark.py", "eom.py", "projection.py", "datasets.py", "native_export.cpp")]
    need(sha(HERE / "benchmark_full_weighted.py") == SERIAL_SOURCE_SHA, "serial source provenance")
    context = dict(sources_before={str(p.resolve()): sha(p) for p in dependencies},
        native_binary=str(args.native.resolve()), native_binary_sha256=sha(args.native),
        qualification=str(args.qualification.resolve()), qualification_sha256=sha(args.qualification),
        baseline_receipt=str(baseline_path), baseline_receipt_sha256=sha(baseline_path),
        manifest=str(manifest_path), manifest_sha256=sha(manifest_path), input_hashes={})
    context["fixed_pins"] = {context["native_binary"]: context["native_binary_sha256"],
        context["qualification"]: context["qualification_sha256"], str(baseline_path): PREVIOUS_RECEIPT_SHA,
        str(manifest_path): MANIFEST_SHA}
    for case_id in CASES:
        case = by_case[case_id]
        need(case["n"] == 1200, "whole scene size")
        context["input_hashes"].update({case[key]: value for key, value in case["prepared_sha256"].items()})
    verify(context["input_hashes"], "input preflight")
    return context, baseline, by_case


def reuse_units(args, context, baseline, by_case):
    if args.reuse is None:
        return {}, None
    source = args.reuse.resolve()
    old_path = source / "receipt.json"
    old = json.loads(old_path.read_text())
    need(old["schema"] == "mhgp9_weighted_full_gaussian_pilot_v2" and old["status"] == "failed"
         and old.get("error") == "KeyboardInterrupt()", "actual interrupted R2 receipt required")
    need(old["plan"] == PLAN, "identical interrupted plan")
    for key in ("native_binary", "native_binary_sha256", "qualification", "qualification_sha256",
                "baseline_receipt", "baseline_receipt_sha256", "manifest", "manifest_sha256"):
        need(old[key] == context[key], "reuse binding " + key)
    expected_sources = {p: h for p, h in context["sources_before"].items() if p != str(Path(__file__).resolve())}
    need(old["sources_before"] == expected_sources, "exact serial source inventory")
    verify(old["sources_before"], "new post-interruption source verification")
    verify(old["input_hashes"], "interrupted inputs")
    verify(old["artifacts"], "all interrupted recorded artifacts")
    commands = {}
    for command in old["commands"]:
        key = (command["case"], command["k"])
        need(key not in commands, "duplicate interrupted command")
        commands[key] = command
        directory = source / f"{key[0]}_k{key[1]}"
        need(json.loads((directory / "command.json").read_text()) == command, "command receipt binding")
        for suffix, name in (("stdout", "native.json"), ("stderr", "native.stderr")):
            need(sha(directory / name) == command[suffix + "_sha256"], "interrupted command stream")
    units = {}
    for case_id in CASES:
        for k in KS:
            rows = [r for r in old["rows"] if r["case"] == case_id and r["k"] == k]
            if len(rows) != 14:
                continue  # Recompute the WHOLE unit, never keep partial selections.
            check_rows(rows, baseline, case_id, k)
            command = commands[(case_id, k)]
            case = by_case[case_id]
            need(command["returncode"] == 0 and command["argv"] == native_argv(context, case, k), "reused native command")
            directory = source / f"{case_id}_k{k}"
            intent = json.loads((directory / "intent.json").read_text())
            need(intent == dict(argv=command["argv"], binary_sha256=context["native_binary_sha256"],
                 input_sha256=sha(case["points_u32le"])), "reused native intent")
            artifacts = {}
            for name in payload_names():
                original = str(directory / name)
                need(original in old["artifacts"], "complete recorded reused payloads")
                artifacts[str(args.output / directory.name / name)] = old["artifacts"][original]
            old_native = str(args.baseline.resolve() / directory.name / "native.json")
            need(old_native in old["artifacts"], "reused baseline object pin")
            artifacts[old_native] = old["artifacts"][old_native]
            tag = dict(receipt=str(old_path), receipt_sha256=sha(old_path), directory=str(directory.resolve()))
            (args.output / directory.name).symlink_to(directory.resolve(), target_is_directory=True)
            units[(case_id, k)] = dict(status="completed", rows=[dict(r, reused_from=tag) for r in rows],
                commands=[dict(command, reused_from=tag)], artifacts=artifacts, reused_from=tag)
    proof = dict(receipt=str(old_path), receipt_sha256=sha(old_path), units=len(units),
        missing_original_failure_source_closure="sources_after" not in old,
        resume_verification_currentpins={p: sha(p) for p in old["sources_before"]},
        note="New read-only verification after interruption; not an original successful source closure")
    return units, proof


def run(args):
    args.output = args.output.absolute()
    need(args.output.parent.resolve().is_relative_to(Path("/tmp")), "parallel capture must be under /tmp")
    need(not args.output.exists(), "fresh output required")
    args.output.mkdir(parents=True)
    started = time.monotonic()
    receipt = dict(schema="mhgp9_weighted_full_gaussian_parallel_pilot_v1", status="running", plan=PLAN,
        rows=[], commands=[], artifacts={}, worker_commands=[], workers=args.workers, worker_environment=THREAD_ENV,
        GCP_used=False, GPU_used=False, engine_modified=False, root_policy="excluded_for_both",
        scope="weighted_Gabriel_measure_FULL_connectivity_not_nested_point_decoder", approximate_postprocessing=True,
        selected_subset_after_previous_scores=True, gzip_compresslevel=1,
        note="Same diagnostic grid; parallel and reused elapsed times are not a serial performance comparison")
    active, units = {}, {}
    try:
        context, baseline, by_case = prepare(args)
        receipt.update(context)
        units, proof = reuse_units(args, context, baseline, by_case)
        receipt["reuse_verification"] = proof
        save(args.output / "intent.json", receipt)
        pending = [(case_id, k) for case_id in CASES for k in KS if (case_id, k) not in units]
        with (args.output / "ledger.jsonl").open("x") as ledger:
            while pending or active:
                while pending and len(active) < args.workers:
                    key = pending.pop(0)
                    name = f"{key[0]}_k{key[1]}"
                    spec = args.output / (name + ".spec.json")
                    save(spec, dict(context=context, case=by_case[key[0]], k=key[1], directory=str(args.output / name)))
                    receipt["artifacts"][str(spec)] = sha(spec)
                    argv = [sys.executable, "-B", str(Path(__file__).resolve()), "--worker-spec", str(spec)]
                    out = (args.output / (name + ".worker.stdout")).open("xb")
                    err = (args.output / (name + ".worker.stderr")).open("xb")
                    try:
                        process = subprocess.Popen(argv, stdout=out, stderr=err, start_new_session=True,
                                                   env=dict(os.environ, **THREAD_ENV))
                    finally:
                        out.close(); err.close()
                    active[key] = (process, argv, time.monotonic())
                    ledger.write(json.dumps(dict(event="start", case=key[0], k=key[1], pid=process.pid, argv=argv)) + "\n")
                    ledger.flush()
                    print("START", name, "pid", process.pid, flush=True)
                for key, (process, argv, start) in list(active.items()):
                    if process.poll() is None:
                        continue
                    process.wait()
                    name = f"{key[0]}_k{key[1]}"
                    command = dict(case=key[0], k=key[1], argv=argv, pid=process.pid, returncode=process.returncode,
                        elapsed_seconds=time.monotonic()-start,
                        stdout_sha256=sha(args.output / (name + ".worker.stdout")),
                        stderr_sha256=sha(args.output / (name + ".worker.stderr")))
                    receipt["worker_commands"].append(command)
                    ledger.write(json.dumps(dict(event="joined", **command)) + "\n"); ledger.flush()
                    del active[key]
                    need(process.returncode == 0, "worker failed: " + name)
                    worker_path = args.output / name / "worker_receipt.json"
                    result = json.loads(worker_path.read_text())
                    need(result["status"] == "completed" and len(result["commands"]) == 1, "whole worker receipt")
                    check_rows(result["rows"], baseline, *key)
                    receipt["artifacts"][str(worker_path)] = sha(worker_path)
                    units[key] = result
                    print("DONE", name, "units", len(units), "/26", flush=True)
                if active:
                    time.sleep(0.2)
        for key in ((c, k) for c in CASES for k in KS):
            unit = units[key]
            receipt["rows"].extend(unit["rows"])
            receipt["commands"].extend(unit["commands"])
            receipt["artifacts"].update(unit["artifacts"])
        receipt["artifacts"][str(args.output / "ledger.jsonl")] = sha(args.output / "ledger.jsonl")
        for command in receipt["worker_commands"]:
            name = f"{command['case']}_k{command['k']}"
            for suffix in ("stdout", "stderr"):
                path = args.output / (name + ".worker." + suffix)
                receipt["artifacts"][str(path)] = command[suffix + "_sha256"]
        receipt["sources_after"] = {p: sha(p) for p in receipt["sources_before"]}
        need(receipt["sources_after"] == receipt["sources_before"], "source closure")
        for label in ("fixed_pins", "input_hashes", "artifacts"):
            verify(receipt[label], label + " closure")
        for command in receipt["commands"]:
            directory = args.output / f"{command['case']}_k{command['k']}"
            for suffix, name in (("stdout", "native.json"), ("stderr", "native.stderr")):
                need(sha(directory / name) == command[suffix + "_sha256"], "native capture closure")
        if proof:
            need(sha(proof["receipt"]) == proof["receipt_sha256"], "interrupted receipt closure")
        need(len(receipt["rows"]) == 364 and len(receipt["commands"]) == 26, "complete unchanged grid")
        receipt["status"] = "completed"
    except BaseException as error:
        receipt.update(status="failed", error=repr(error), traceback=traceback.format_exc())
        raise
    finally:
        # No orphan process groups after an interrupted/failed orchestration.
        for process, _, _ in active.values():
            if process.poll() is None:
                try:
                    os.killpg(process.pid, signal.SIGINT)
                except ProcessLookupError:
                    pass
        for key, (process, argv, start) in active.items():
            try:
                process.wait(timeout=30)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL); process.wait()
            name = f"{key[0]}_k{key[1]}"
            receipt["worker_commands"].append(dict(case=key[0], k=key[1], argv=argv, pid=process.pid,
                returncode=process.returncode, elapsed_seconds=time.monotonic()-start, interrupted=True,
                stdout_sha256=sha(args.output / (name + ".worker.stdout")),
                stderr_sha256=sha(args.output / (name + ".worker.stderr"))))
        if receipt["status"] != "completed":
            receipt["rows"], receipt["commands"] = [], []
            for key in ((c, k) for c in CASES for k in KS):
                if key in units:
                    receipt["rows"].extend(units[key]["rows"])
                    receipt["commands"].extend(units[key]["commands"])
                    receipt["artifacts"].update(units[key]["artifacts"])
        receipt["elapsed_seconds"] = time.monotonic()-started
        save(args.output / "receipt.json", receipt)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--worker-spec", type=Path, help=argparse.SUPPRESS)
    parser.add_argument("--native", type=Path)
    parser.add_argument("--qualification", type=Path)
    parser.add_argument("--baseline", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--reuse", type=Path)
    parser.add_argument("--workers", type=int, choices=(1, 2, 3), default=2)
    args = parser.parse_args()
    if args.worker_spec:
        worker(args.worker_spec)
    else:
        need(all(getattr(args, key) is not None for key in ("native", "qualification", "baseline", "output")),
             "native/qualification/baseline/output required")
        run(args)
