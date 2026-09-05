"""Bounded observations of the candidate incidence route, including refusals.

Coordinate extent is an explicit axis. A successful wide-grid observation
never replaces a refused default-family observation or qualifies exact HGP.
"""

from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import re
import subprocess

from compare_v6_v7 import atomic_json, key_values, object_digest, one_line, positive_metric, require, run_process, sha256, source_snapshot


# Exact untyped messages emitted by build_silent_cofaces/run_pipeline. A new
# reason is invalid until reviewed here; suffixes such as "*_budget" are not
# enough to infer a status. The ordinary typed pipeline route remains separate.
SILENT_RESOURCE_REASONS = frozenset({
    "silent_core_record_budget", "silent_chain_step_budget",
    "silent_added_coface_budget", "silent_query_node_budget",
    "silent_meb_support_budget", "silent_direct_catalogue_budget",
    "silent_allocation_failure",
})
SILENT_UNSUPPORTED_REASONS = frozenset({
    "silent_local_nonessential_shell", "silent_external_shell",
    "silent_nonregular_direct_catalogue",
})


def refusal_kind(line: str) -> tuple[str, int | None]:
    """Recognize typed refusals or the closed, untyped completion vocabulary."""
    typed = re.fullmatch(r"REFUS (unsupported_degeneracy|resource_exhausted) : [^\x00-\x1f\x7f]+", line)
    if typed is not None:
        return typed[1], None
    silent = re.fullmatch(r"REFUS silent incidence K=([2-9]|10) : (silent_[a-z_]+)", line)
    require(silent is not None, "unexpected refusal cause")
    reason = silent[2]
    if reason in SILENT_RESOURCE_REASONS:
        return "resource_exhausted", int(silent[1])
    require(reason in SILENT_UNSUPPORTED_REASONS, "unknown or non-observational silent refusal")
    return "unsupported_degeneracy", int(silent[1])


def parse_completion(text: str, stderr: str, usage: str, *, family: str, n: int,
                     coord: int, seed: int, threads: int, meb_supports: int,
                     wall_seconds: float) -> dict:
    """Validate an observation's identity and structure, never its exactness."""
    require(not stderr, "successful candidate stderr")
    require(one_line(text, "forest_semantics=") ==
            "forest_semantics=normalized_horizontal_h0_candidate public_status=not_claimed require_exact=false",
            "candidate semantics absent")
    require(one_line(text, "payload=") ==
            "payload=mhgp7-forests-horizontal-v1 authority=status_terminal callbacks=provisional vertical_maps=none",
            "payload authority")
    require(one_line(text, "backend=") == "backend=cpu_reference", "backend")
    require(one_line(text, "tower_scope=") ==
            "tower_scope=profile_complete_k10 smax_requested=11 smax_effective=11", "complete order scope")
    require(one_line(text, "forest_layout=") ==
            "forest_layout=csr forest_storage_kind=csr_facet_keys_v1 csr_fallback=0 ordres_publies=10 ordres_storage_conformes=10",
            "layout and order storage")
    require(not any(line.startswith(("REFUS", "profil_kind=")) for line in text.splitlines()), "unexpected result route")
    fields = key_values(one_line(text, "famille="), {
        "famille", "n", "coord", "s", "smax", "seed", "threads", "emis", "boules_uniques",
        "mortes_profondeur", "survivantes", "census_int", "census_shell", "evenements", "facettes",
        "fusions", "deltas", "noeuds",
    })
    require(fields.pop("famille") == family, "family does not match command")
    require(all(re.fullmatch(r"-?[0-9]+" if k == "seed" else r"[0-9]+", v)
                for k, v in fields.items()), "noninteger identity/count")
    counts = {k: int(v) for k, v in fields.items()}
    require((counts["n"], counts["seed"], counts["threads"], counts["s"], counts["smax"]) ==
            (n, seed, threads, 8, 11), "run identity does not match command")
    require(1 <= counts["coord"] <= 65536 and (coord == 0 or coord == counts["coord"]), "coordinate identity")
    require(counts["evenements"] > 0 and counts["facettes"] > 0, "empty completed observation")
    require(counts["emis"] >= counts["boules_uniques"] == counts["mortes_profondeur"] + counts["survivantes"],
            "candidate accounting")
    cards = {}
    for line in text.splitlines():
        if not line.startswith("cardinalites"):
            continue
        require(line.startswith("cardinalites "), "malformed cardinalities")
        values = key_values(line[len("cardinalites "):], {
            "K", "evenements", "facettes", "deltas", "attachements", "fusions", "noeuds",
        })
        require(all(re.fullmatch(r"[0-9]+", v) for v in values.values()), "cardinality integer")
        values = {k: int(v) for k, v in values.items()}
        k = values.pop("K")
        require(1 <= k <= 10 and k not in cards, "cardinality order")
        cards[k] = values
    require(set(cards) == set(range(1, 11)) and cards[1]["facettes"] == n, "cardinality coverage K1..10")
    for key in ("evenements", "facettes", "deltas", "fusions", "noeuds"):
        require(sum(card[key] for card in cards.values()) == counts[key], f"cardinality total {key}")
    limits = key_values(one_line(text, "silent_limits ")[len("silent_limits "):],
                        {"core_records", "chain_steps", "cofaces", "query_nodes", "meb_supports"})
    require(all(re.fullmatch(r"[0-9]+", v) for v in limits.values()), "silent limit integer")
    require(int(limits["meb_supports"]) == meb_supports, "MEB budget identity")
    silent = {}
    for line in text.splitlines():
        if not line.startswith("silent_K"):
            continue
        match = re.fullmatch(r"silent_K([2-9]|10) (.+)", line)
        require(match is not None, "silent order record")
        k = int(match[1])
        require(k not in silent, "duplicate silent order")
        values = key_values(match[2], {"core", "with_two_intruders", "steps", "added", "max_chain", "query_nodes", "meb_supports"})
        require(all(re.fullmatch(r"[0-9]+", v) for v in values.values()), "silent counter integer")
        silent[k] = {field: int(v) for field, v in values.items()}
        require(silent[k]["steps"] == silent[k]["added"] and
                silent[k]["with_two_intruders"] <= silent[k]["core"] and
                silent[k]["max_chain"] <= silent[k]["steps"] and
                silent[k]["meb_supports"] <= meb_supports, "silent counter accounting")
    require(set(silent) == set(range(2, 11)), "silent order coverage")
    budget = key_values(one_line(text, "memory_budget_scope="), {
        "memory_budget_scope", "budget", "cap_brut_demande", "cap_brut_effectif", "cap_fusion_budgetaire",
    })
    require(budget.pop("memory_budget_scope") == "partial_named_payload_proxy_v1", "memory scope")
    require(all(re.fullmatch(r"[0-9]+", v) for v in budget.values()) and int(budget["budget"]) == 17179869184,
            "memory proxy budget identity")
    pipeline_ms = positive_metric(text, "temps_mur_ms=")
    require(math.isfinite(wall_seconds) and pipeline_ms <= wall_seconds * 1000 + 50, "elapsed envelope")
    rss = re.findall(r"^\s*Maximum resident set size \(kbytes\): ([0-9]+)$", usage, re.MULTILINE)
    require(len(rss) == 1 and int(rss[0]) > 0, "external RSS")
    require(re.findall(r"^\s*Exit status: ([0-9]+)$", usage, re.MULTILINE) == ["0"], "external successful exit")
    return {"digests": object_digest(text), "counts": counts, "cardinalities": cards,
            "silent": silent, "pipeline_ms": pipeline_ms, "max_rss_kb": int(rss[0])}


def classify(record: dict, out: Path, err: Path, usage: Path, **identity: object) -> None:
    """Keep the attempt even if validation fails; partial output is not success."""
    if record["timed_out"]:
        record["status"] = "censored"
        return
    if record["returncode"] == 0:
        parsed = parse_completion(out.read_text(), err.read_text(), usage.read_text(),
                                  wall_seconds=record["wall_seconds"], **identity)
        record.update(status="engine_completed", **parsed)
    elif record["returncode"] == 2:
        require(not out.read_bytes(), "refusal published output")
        lines = err.read_text().splitlines()
        require(len(lines) >= 3, "incomplete refusal diagnostics")
        refusal_status, refusal_order = refusal_kind(lines[0])
        stage = re.fullmatch(r"refus_etage=(entree|generation|rle|prefiltre|census|fold|publication) "
                             r"rss_mb apres_generation=[0-9]+ apres_rle=[0-9]+ apres_prefiltre=[0-9]+ "
                             r"apres_census=[0-9]+ max_fold=[0-9]+ "
                             r"\(frontiere de completion : dernier etage atteint\)", lines[1])
        require(stage is not None, "refusal stage record")
        require(refusal_order is None or stage[1] == "fold", "silent refusal outside fold stage")
        require(re.fullmatch(r"silent_refusal_work census_balls=[0-9]+ plateau_balls=[0-9]+ "
                             r"total_ms=[0-9]+\.[0-9]+ completion_ms=[0-9]+\.[0-9]+", lines[2]), "refusal work record")
        previous_order = 1
        for line in lines[3:]:
            match = re.fullmatch(r"silent_refusal_K([2-9]|10) core=[0-9]+ steps=[0-9]+ added_provisional=[0-9]+ "
                                 r"query_nodes=[0-9]+ meb_supports=[0-9]+", line)
            require(match is not None, "unexpected refusal diagnostic")
            order = int(match[1])
            require(previous_order < order and (refusal_order is None or order <= refusal_order),
                    "duplicate, unordered or future refusal diagnostic")
            previous_order = order
        record.update(status="engine_refused", reason=lines[0], refusal_status=refusal_status,
                      refusal_order=refusal_order)
    else:
        raise ValueError(f"unexpected engine returncode {record['returncode']}")


def summarize(records: list[dict], expected: int, failure: str | None, stable: bool) -> dict:
    valid = {"engine_completed", "engine_refused", "censored"}
    complete = not failure and stable and len(records) == expected and all(r["status"] in valid for r in records)
    return {"status": "observations_completed" if complete else "invalid", "error": failure,
            "runs": len(records), "expected_runs": expected, "source_stable": stable,
            "engine_successes": sum(r["status"] == "engine_completed" for r in records),
            "engine_refusals": sum(r["status"] == "engine_refused" for r in records),
            "censored": sum(r["status"] == "censored" for r in records),
            "invalid": sum(r["status"] not in valid for r in records), "public_status": "not_claimed"}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--sizes", type=int, nargs="+", default=[8000, 16000, 32000])
    parser.add_argument("--families", nargs="+", choices=["uniform", "terrain"], default=["uniform", "terrain"])
    parser.add_argument("--coords", type=int, nargs="+", default=[0, 65536])
    parser.add_argument("--seed", type=int, default=3)
    parser.add_argument("--threads", type=int, default=8)
    parser.add_argument("--timeout", type=int, default=180)
    parser.add_argument("--meb-supports", type=int, default=25000000)
    args = parser.parse_args()
    require(all(n >= 11 for n in args.sizes) and len(set(args.sizes)) == len(args.sizes), "sizes")
    require(-(2**63) <= args.seed < 2**63, "seed outside i64")
    require(all(0 <= n <= 65536 for n in args.coords) and len(set(args.coords)) == len(args.coords), "coords")
    require(len(set(args.families)) == len(args.families), "families")
    require(1 <= args.threads <= 1024 and 1 <= args.timeout <= 600 and args.meb_supports >= 1, "budgets")
    root = Path(__file__).resolve().parents[2]
    binary = args.binary.resolve(strict=True)
    args.output.mkdir(parents=True, exist_ok=False)
    sources = source_snapshot(root)
    script_hash = sha256(Path(__file__))
    executable_hash = sha256(binary)
    metadata = {"schema": "mhgp7-incidence-observations-v1", "public_status": "not_claimed",
                "purpose": "success_and_refusal_observations_not_qualification",
                "source_sha256": sources, "binary_sha256": executable_hash,
                "runner_sha256": script_hash,
                "source_binary_binding": "separately_recorded_hashes_not_build_attestation",
                "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(),
                "options": {k: str(v) if isinstance(v, Path) else v for k, v in vars(args).items()},
                "address_limit_gib": 26, "payload_proxy_budget_bytes": 17179869184}
    atomic_json(args.output / "metadata.json", metadata)
    records = []
    failure = None

    def stable() -> bool:
        return sources == source_snapshot(root) and sha256(binary) == executable_hash and sha256(Path(__file__)) == script_hash

    try:
        for family, n, coord in ((family, n, coord) for family in args.families for n in args.sizes for coord in args.coords):
            require(stable(), "source changed before run")
            name = f"{family}_{n}_coord{coord}"
            command = [str(binary), f"--family={family}", f"--n={n}", f"--seed={args.seed}",
                       f"--threads={args.threads}", "--smax=11", "--layout=csr", "--digest",
                       "--complete-incidences", "--mem-budget=17179869184",
                       f"--silent-meb-supports={args.meb_supports}"]
            if coord:
                command.append(f"--coord={coord}")
            out, err, usage = (args.output / (name + suffix) for suffix in (".out", ".err", ".time"))
            wrapped = ["/usr/bin/time", "-v", "-o", str(usage), "--", *command]
            record = {"case": name, "command": command, "returncode": None,
                      "timed_out": False, "wall_seconds": None,
                      "status": "invalid", "stdout": out.name, "stderr": err.name, "usage": usage.name}
            records.append(record)
            # Persist BEFORE parsing: malformed and partially emitted outputs
            # remain identifiable attempts in the terminal receipt.
            atomic_json(args.output / "runs.json", records)
            try:
                code, timed_out, elapsed = run_process(wrapped, out, err, timeout=args.timeout, address_limit_gib=26)
                record.update(returncode=code, timed_out=timed_out, wall_seconds=elapsed)
                classify(record, out, err, usage, family=family, n=n, coord=coord, seed=args.seed,
                         threads=args.threads, meb_supports=args.meb_supports)
            except Exception as error:
                record["validation_error"] = f"{type(error).__name__}: {error}"
                raise
            finally:
                atomic_json(args.output / "runs.json", records)
            print(json.dumps(record), flush=True)
            require(stable(), "source changed during run")
    except (Exception, KeyboardInterrupt) as error:
        failure = f"{type(error).__name__}: {error}"
    source_stable = False
    try:
        source_stable = stable()
    except (Exception, KeyboardInterrupt) as error:
        failure = failure or f"{type(error).__name__}: {error}"
    summary = summarize(records, len(args.families) * len(args.sizes) * len(args.coords), failure, source_stable)
    atomic_json(args.output / "summary.json", summary)
    atomic_json(args.output / "hashes.json", {p.name: sha256(p) for p in sorted(args.output.iterdir()) if p.is_file()})
    print(json.dumps(summary), flush=True)
    require(summary["status"] == "observations_completed", "invalid observations")


if __name__ == "__main__":
    main()
