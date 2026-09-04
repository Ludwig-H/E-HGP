"""Serial paired v6/v7 diagnostics with source and executable hash evidence.

Hashes establish stability at observation boundaries, not filesystem immutability
or a proof that the binaries were built from those sources. No SLO is qualified.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import re
import resource
import signal
import subprocess
import sys
import time


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def source_snapshot(root: Path) -> dict[str, str]:
    files = []
    for version in ("morsehgp3D_v6", "morsehgp3D_v7"):
        for directory in ("src", "cli", "oracle"):
            files.extend(p for p in (root / version / directory).rglob("*") if p.is_file())
        files.append(root / version / "CMakeLists.txt")
    files.append(root / "morsehgp3D_v7" / "bench" / "compare_v6_v7.py")
    return {str(p.relative_to(root)): sha256(p) for p in sorted(files)}


def object_digest(text: str, kmax: int = 10) -> dict[str, str]:
    require(kmax in (5, 10), "digest order must be 5 or 10")
    fields = {}
    for line in text.splitlines():
        if not line.startswith(("digest_all", "digest_forest_K")):
            continue
        match = re.fullmatch(r"(digest_all|digest_forest_K(?:[1-9]|10))=([0-9a-f]{64})", line)
        require(match is not None, "malformed object digest field")
        key, value = match.groups()
        require(key not in fields, "duplicate object digest field")
        fields[key] = value
    require(set(fields) == {"digest_all"} | {f"digest_forest_K{k}" for k in range(1, kmax + 1)},
            f"missing exact K1..{kmax} digest fields")
    chained = hashlib.sha256(b"mhgp4-digest-v1:all" +
                             "".join(fields[f"digest_forest_K{k}"] for k in range(1, kmax + 1)).encode()).hexdigest()
    require(fields["digest_all"] == chained, "forest digest chain mismatch")
    return fields


def one_line(text: str, prefix: str) -> str:
    lines = [line for line in text.splitlines() if line.startswith(prefix)]
    require(len(lines) == 1, f"missing or repeated {prefix}")
    return lines[0]


def key_values(line: str, expected: set[str]) -> dict[str, str]:
    result = {}
    for token in line.split():
        require(token.count("=") == 1, "malformed key/value record")
        key, value = token.split("=")
        require(key not in result, "duplicate key/value field")
        result[key] = value
    require(set(result) == expected, "key/value record schema")
    return result


def positive_metric(text: str, prefix: str) -> float:
    line = one_line(text, prefix)
    value = line[len(prefix):].split(" ", 1)[0]
    require(re.fullmatch(r"[0-9]+(?:\.[0-9]+)?", value) is not None, f"malformed {prefix}")
    number = float(value)
    require(math.isfinite(number) and number > 0, f"nonpositive {prefix}")
    return number


def parse_success(text: str, stderr: str, usage: str, *, label: str, family: str,
                  n: int, seed: int, threads: int, wall_seconds: float,
                  kmax: int = 10, serial_stages_requested: bool = False, s: int = 8) -> dict:
    require(label in ("v6", "v7") and kmax in (5, 10) and s in (8, 10, 12), "wire version/order/separation")
    require(not stderr.strip(), "successful run has stderr")
    require(one_line(text, "payload=") ==
            f"payload=mhgp{label[1:]}-forests-horizontal-v1 authority=status_terminal callbacks=provisional vertical_maps=none",
            "payload authority/version")
    require(one_line(text, "backend=") == "backend=cpu_reference", "backend")
    tower = ("tower_scope=profile_complete_k10 smax_requested=11 smax_effective=11" if kmax == 10 else
             "tower_scope=prefix_k5 smax_requested=6 smax_effective=6 (K = 1..5, prefixe exact de l'objet complet)")
    require(one_line(text, "tower_scope=") == tower, "order scope")
    require(one_line(text, "forest_layout=") ==
            f"forest_layout=csr forest_storage_kind=csr_facet_keys_v1 csr_fallback=0 ordres_publies={kmax} ordres_storage_conformes={kmax}",
            "layout/fallback/complete orders")
    if label == "v7":
        require(one_line(text, "forest_semantics=") ==
                "forest_semantics=verified_events_only public_status=not_claimed require_exact=false", "candidate semantics")
    require(not any(line.startswith(("REFUS", "profil_kind=", "memory_budget_scope=", "silent_K"))
                    for line in text.splitlines()), "unexpected refusal/profile/budget/semantic route")
    fields = key_values(one_line(text, "famille="), {
        "famille", "n", "coord", "s", "smax", "seed", "threads", "emis", "boules_uniques",
        "mortes_profondeur", "survivantes", "census_int", "census_shell", "evenements", "facettes",
        "fusions", "deltas", "noeuds",
    })
    require(fields["famille"] == family, "family does not match command")
    for key, value in fields.items():
        if key == "famille":
            continue
        require(re.fullmatch(r"-?[0-9]+" if key == "seed" else r"[0-9]+", value) is not None, f"integer {key}")
    counts = {key: int(value) for key, value in fields.items() if key != "famille"}
    require((counts["n"], counts["seed"], counts["threads"], counts["s"], counts["smax"]) ==
            (n, seed, threads, s, kmax + 1), "run identity does not match command")
    if serial_stages_requested:
        require(threads == 1, "serialized stages require one requested worker")
        require(re.fullmatch(r"temps_fold_mur_ms=[0-9]+(?:\.[0-9]+)? \(etages A et B, "
                             r"fold_inflight=1, fold_join=1, pic_mesure_en_vol=[01]\)",
                             one_line(text, "temps_fold_mur_ms=")) is not None,
                "serialized stage flags or measured in-flight bound")
    require(1 <= counts["coord"] <= 65536 and counts["evenements"] > 0 and counts["facettes"] > 0,
            "non-vacuity/coordinate domain")
    require(counts["emis"] >= counts["boules_uniques"] == counts["mortes_profondeur"] + counts["survivantes"],
            "candidate count identity")
    cards = {}
    for line in text.splitlines():
        if not line.startswith("cardinalites"):
            continue
        require(line.startswith("cardinalites "), "malformed cardinality record")
        values = key_values(line[len("cardinalites "):], {
            "K", "evenements", "facettes", "deltas", "attachements", "fusions", "noeuds",
        })
        require(all(re.fullmatch(r"[0-9]+", value) is not None for value in values.values()), "cardinality integer")
        values = {key: int(value) for key, value in values.items()}
        k = values.pop("K")
        require(1 <= k <= kmax and k not in cards, "duplicate/outside cardinality order")
        cards[k] = values
    require(set(cards) == set(range(1, kmax + 1)), "cardinality order coverage")
    require(cards[1]["facettes"] == n, "K1 point coverage")
    for key in ("evenements", "facettes", "deltas", "fusions", "noeuds"):
        require(sum(card[key] for card in cards.values()) == counts[key], f"cardinality total {key}")
    pipeline_ms = positive_metric(text, "temps_mur_ms=")
    require(pipeline_ms <= wall_seconds * 1000 + 50, "pipeline time outside process envelope")
    rss = re.findall(r"^\s*Maximum resident set size \(kbytes\): ([0-9]+)$", usage, re.MULTILINE)
    require(len(rss) == 1 and int(rss[0]) > 0, "missing/invalid external RSS")
    require(re.findall(r"^\s*Exit status: ([0-9]+)$", usage, re.MULTILINE) == ["0"], "external time exit status")
    return {"digests": object_digest(text, kmax), "cardinalities": cards, "counts": counts,
            "pipeline_ms": pipeline_ms, "max_rss_kb": int(rss[0])}


def atomic_json(path: Path, value: object) -> None:
    temporary = path.with_name("." + path.name + ".tmp")
    with temporary.open("w", encoding="utf-8") as stream:
        json.dump(value, stream, indent=2, allow_nan=False)
        stream.write("\n")
        stream.flush()
        os.fsync(stream.fileno())
    temporary.replace(path)


def run_process(command: list[str], stdout: Path, stderr: Path, *, timeout: int,
                address_limit_gib: int) -> tuple[int, bool, float]:
    def limits() -> None:
        size = address_limit_gib * 1024**3
        resource.setrlimit(resource.RLIMIT_AS, (size, size))
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))

    environment = dict(os.environ, LC_ALL="C", LANG="C")
    started = time.monotonic()
    timed_out = False
    with stdout.open("wb") as out, stderr.open("wb") as err:
        process = subprocess.Popen(command, stdout=out, stderr=err, start_new_session=True,
                                   preexec_fn=limits, env=environment)
        try:
            rc = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            timed_out = True
            rc = -signal.SIGKILL
        finally:
            # Drain exactly the process group we created, also on interruption
            # and when the top-level child exits leaving descendants behind.
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            final_rc = process.wait()
        if timed_out:
            rc = final_rc
    return rc, timed_out, time.monotonic() - started


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--sizes", type=int, nargs="+", default=[8000, 16000, 32000])
    parser.add_argument("--seeds", type=int, nargs="+", default=[3, 17, 29])
    parser.add_argument("--families", nargs="+", default=["uniform", "terrain"])
    parser.add_argument("--threads", type=int, default=8)
    parser.add_argument("--kmax", type=int, choices=(10, 5), default=10)
    parser.add_argument("--serial-stages", action="store_true",
                        help="request threads=1/inflight=1/join=1; does not certify absence of auxiliary threads")
    parser.add_argument("--reference-version", choices=("v6", "v7"), default="v6",
                        help="actual wire version of the reference binary, independent of its comparison role")
    parser.add_argument("--separations", type=int, nargs="+", choices=(8, 10, 12), default=[8])
    parser.add_argument("--timeout", type=int, default=600)
    parser.add_argument("--address-limit-gib", type=int, default=26)
    args = parser.parse_args()
    requested_threads = args.threads
    require(1 <= args.threads <= 1024 and args.timeout > 0 and args.address_limit_gib > 0, "positive resource limits required")
    if args.serial_stages:
        args.threads = 1
    require(all(args.kmax + 1 <= n <= 1_000_000_000 for n in args.sizes),
            f"K1..{args.kmax} requires n >= {args.kmax + 1}")
    require(all(-(2**63) <= seed < 2**63 for seed in args.seeds), "seed outside i64")
    require(all(re.fullmatch(r"[a-z][a-z0-9_]*", family) is not None for family in args.families), "family name")
    for values in (args.sizes, args.seeds, args.families, args.separations):
        require(len(values) == len(set(values)), "duplicate matrix coordinates")
    root = Path(__file__).resolve().parents[2]
    reference, candidate = args.reference.resolve(), args.candidate.resolve()
    require(reference != candidate, "paired executables must have distinct paths")
    require(all(path.is_file() and os.access(path, os.X_OK) for path in (reference, candidate)), "executable unavailable")
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    before = source_snapshot(root)
    binary_hashes = {str(p): sha256(p) for p in (reference, candidate)}
    metadata = {
        "schema": "mhgp7-paired-cpu-v2", "public_status": "not_claimed",
        "purpose": "compatibility_and_exploratory_cost", "backend": "cpu_reference",
        "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip(),
        "worktree": subprocess.check_output(["git", "status", "--porcelain=v1"], cwd=root, text=True),
        "source_sha256": before, "binaries": binary_hashes,
        "binary_roles": {"reference": {"path": str(reference), "wire_version": args.reference_version},
                         "candidate": {"path": str(candidate), "wire_version": "v7"}},
        "serial_stages_requested": args.serial_stages, "threads_option_before_serial_override": requested_threads,
        "strict_single_thread_qualification": "not_claimed",
        "host": platform.platform(), "cpu_affinity": sorted(os.sched_getaffinity(0)),
        "options": {k: str(v) if isinstance(v, Path) else v for k, v in vars(args).items()},
        "compiler": subprocess.check_output(["c++", "--version"], text=True).splitlines()[0],
        "python": sys.version, "interpreter": str(Path(sys.executable).resolve()),
        "time_sha256": sha256(Path("/usr/bin/time")),
        "source_binary_binding": "source_hashes_and_binary_hashes_only_build_not_attested",
        "time_scope": "external_process_including_digest_not_warm_e2e",
    }
    atomic_json(args.output / "metadata.json", metadata)
    (args.output / "runner.py").write_bytes(Path(__file__).read_bytes())
    records = []
    expected_pairs = len(args.families) * len(args.seeds) * len(args.sizes) * len(args.separations)
    sequence = 0
    terminal_error = None
    stable = True
    cross_separation = {}
    cross_separation_comparisons = 0

    def evidence_stable() -> bool:
        return (before == source_snapshot(root) and
                all(sha256(Path(path)) == digest for path, digest in binary_hashes.items()) and
                sha256(Path("/usr/bin/time")) == metadata["time_sha256"])

    try:
        for family, seed, n, separation in ((family, seed, n, s) for family in args.families
                                           for seed in args.seeds for n in args.sizes for s in args.separations):
            pair = {}
            roles = ("reference", "candidate") if sequence % 2 == 0 else ("candidate", "reference")
            for role in roles:
                binary = reference if role == "reference" else candidate
                label = args.reference_version if role == "reference" else "v7"
                stable = evidence_stable()
                require(stable, "source or executable changed before run")
                suffix = role if args.reference_version == "v7" else label
                separation_suffix = f"_wspd_s{separation}" if args.separations != [8] else ""
                name = f"{sequence:03d}_{family}_{n}_s{seed}{separation_suffix}_{suffix}"
                command = [str(binary), f"--family={family}", f"--n={n}", f"--seed={seed}",
                           f"--threads={args.threads}", f"--s={separation}", f"--smax={args.kmax + 1}", "--layout=csr",
                           "--fold-inflight=1" if args.serial_stages else "--fold-inflight=2",
                           "--fold-join=1" if args.serial_stages else "--fold-join=0", "--digest"]
                usage = args.output / f"{name}.time"
                stdout = args.output / f"{name}.out"
                stderr = args.output / f"{name}.err"
                wrapped = ["/usr/bin/time", "-v", "-o", str(usage), "--", *command]
                rc, timed_out, elapsed = run_process(wrapped, stdout, stderr, timeout=args.timeout,
                                                    address_limit_gib=args.address_limit_gib)
                record = {"sequence": sequence, "family": family, "n": n, "seed": seed, "version": label,
                          "role": role, "kmax": args.kmax, "s": separation,
                          "serial_stages_requested": args.serial_stages,
                          "command": command, "wrapped_command": wrapped, "returncode": rc, "timed_out": timed_out,
                          "wall_seconds": elapsed, "status": "failed" if rc or timed_out else "completed"}
                if rc == 0 and not timed_out:
                    try:
                        parsed = parse_success(stdout.read_text(), stderr.read_text(), usage.read_text(),
                                               label=label, family=family, n=n, seed=seed,
                                               threads=args.threads, wall_seconds=elapsed, kmax=args.kmax,
                                               serial_stages_requested=args.serial_stages, s=separation)
                        record.update(parsed)
                        pair[role] = parsed
                    except (RuntimeError, OSError, UnicodeError, ValueError) as error:
                        record["status"] = "invalid"
                        record["error"] = str(error)
                if rc in (2, 3) and stdout.read_bytes():
                    record["status"] = "invalid"
                    record["error"] = "refusal_published_stdout"
                records.append(record)
                stable = evidence_stable()
                if not stable:
                    record["status"] = "invalid"
                    record["error"] = "source_or_executable_changed_during_run"
                atomic_json(args.output / "runs.json", records)
                print(json.dumps({key: record[key] for key in ("sequence", "role", "version", "family", "n", "seed", "s", "kmax", "status", "wall_seconds")}), flush=True)
                require(stable, "source or executable changed during run")
            if len(pair) == 2:
                # Internal work counts are diagnostics: an optimization may
                # legitimately change them without changing a published object.
                if (any(pair["reference"][key] != pair["candidate"][key] for key in ("digests", "cardinalities")) or
                        pair["reference"]["counts"]["coord"] != pair["candidate"]["counts"]["coord"]):
                    for record in records[-2:]:
                        record["status"] = "invalid"
                        record["error"] = "paired_object_divergence"
                    atomic_json(args.output / "runs.json", records)
                else:
                    # Pair equality is checked first, so a shared projection
                    # also compares both actual wire versions across s.
                    key = (family, seed, n)
                    projection = {field: pair["reference"][field] for field in ("digests", "cardinalities")}
                    projection["coord"] = pair["reference"]["counts"]["coord"]
                    if key in cross_separation:
                        cross_separation_comparisons += 1
                        if cross_separation[key] != projection:
                            for record in records:
                                if (record["family"], record["seed"], record["n"]) == key and record["status"] == "completed":
                                    record["status"] = "invalid"
                                    record["error"] = "cross_separation_object_divergence"
                            atomic_json(args.output / "runs.json", records)
                    else:
                        cross_separation[key] = projection
            sequence += 1
    except (Exception, KeyboardInterrupt) as error:
        terminal_error = f"{type(error).__name__}: {error}"
    try:
        stable = stable and evidence_stable()
    except (OSError, RuntimeError) as error:
        stable = False
        terminal_error = terminal_error or str(error)
    completed = (not terminal_error and stable and sequence == expected_pairs and
                 len(records) == expected_pairs * 2 and all(r["status"] == "completed" for r in records))
    if not stable:
        for record in records:
            record["status"] = "invalid"
            record["error"] = "campaign_source_or_executable_unstable"
    atomic_json(args.output / "runs.json", records)
    summary = {"status": "completed" if completed else "invalid", "source_stable": stable,
               "kmax": args.kmax, "serial_stages_requested": args.serial_stages,
               "strict_single_thread_qualification": "not_claimed",
               "separations": args.separations, "cross_separation_comparisons": cross_separation_comparisons,
               "cross_separation_objects_equal": completed if len(args.separations) > 1 else None,
               "runs": len(records), "pairs": sequence, "expected_pairs": expected_pairs,
               "error": terminal_error, "all_objects_equal": completed,
               "performance_qualification": "not_claimed", "exact_hgp_qualification": "not_claimed"}
    atomic_json(args.output / "summary.json", summary)
    hashes = {p.name: sha256(p) for p in sorted(args.output.iterdir()) if p.is_file()}
    atomic_json(args.output / "hashes.json", hashes)
    print(json.dumps(summary), flush=True)
    require(completed, "campaign did not complete with stable source and equal objects")


if __name__ == "__main__":
    main()
