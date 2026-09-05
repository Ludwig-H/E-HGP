#!/usr/bin/env python3
"""Read-only independent replay of captured D/E logs; never runs an engine.

Default checks the portable captured evidence. --live also checks the local
private originals, binaries and current source bytes. No constructor parser is
imported. Exactness, full object equality and statistical gain are not inferred.
"""
import argparse
import datetime as dt
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys


HERE = Path(__file__).resolve().parent
REPO = HERE.parents[3]


def require(condition, message):
    if not condition:
        raise ValueError(message)


def unique(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, f"duplicate key: {key}")
        result[key] = value
    return result


def decode(data):
    return json.loads(data, object_pairs_hook=unique)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def single(text, pattern):
    matches = re.findall(pattern, text, re.MULTILINE)
    require(len(matches) == 1, f"expected one match for {pattern}: {len(matches)}")
    return matches[0]


def numbers(text):
    return unique((key, int(value)) for key, value in
                  re.findall(r"([A-Za-z_]+)=(\d+)(?= |$)", text))


def timestamp(value):
    require(value.endswith("Z"), "UTC timestamp required")
    return dt.datetime.fromisoformat(value.replace("Z", "+00:00"))


def parse_raw(text):
    for line in (
        "payload=mhgp7-forests-horizontal-v1 authority=status_terminal callbacks=provisional vertical_maps=none",
        "forest_layout=csr forest_storage_kind=csr_facet_keys_v1 csr_fallback=0 ordres_publies=10 ordres_storage_conformes=10",
        "backend=cpu_reference",
        "forest_semantics=normalized_horizontal_h0_candidate public_status=not_claimed require_exact=false",
        "tower_scope=profile_complete_k10 smax_requested=11 smax_effective=11",
    ):
        single(text, "^" + re.escape(line) + "$")
    digests = unique(re.findall(r"^(digest_forest_K\d+|digest_all)=([0-9a-f]{64})$", text, re.M))
    require(set(digests) == {"digest_all"} | {f"digest_forest_K{k}" for k in range(1, 11)}, "digest nonvacuum")
    chain = sha(b"mhgp4-digest-v1:all" + "".join(digests[f"digest_forest_K{k}"] for k in range(1, 11)).encode())
    require(chain == digests["digest_all"], "digest chain")
    cardinalities = unique((k, numbers(line)) for k, line in re.findall(r"^cardinalites K=(\d+) (.*)$", text, re.M))
    require(set(cardinalities) == {str(k) for k in range(1, 11)}, "cardinality nonvacuum")
    for values in cardinalities.values():
        require(set(values) == {"evenements", "facettes", "deltas", "attachements", "fusions", "noeuds"}, "cardinality keys")
        require(min(values.values()) >= 0 and values["facettes"] > 0 and values["deltas"] > 0, "cardinality floor")
        require(values["fusions"] == values["facettes"] - 1, "one component per K in observed fixture")
    counts = numbers(single(text, r"^famille=uniform (.*)$"))
    for field in ("evenements", "facettes", "deltas", "fusions", "noeuds"):
        require(sum(v[field] for v in cardinalities.values()) == counts[field], "total " + field)
    require(counts["emis"] == counts["boules_uniques"] == counts["survivantes"] + counts["mortes_profondeur"], "ball partition")
    silent = unique((k, numbers(line)) for k, line in re.findall(r"^silent_K(\d+) (.*)$", text, re.M))
    require(set(silent) == {str(k) for k in range(2, 11)}, "silent nonvacuum")
    for values in silent.values():
        require(set(values) == {"core", "with_two_intruders", "steps", "added", "max_chain", "query_nodes", "meb_supports"}, "silent keys")
        require(min(values.values()) > 0 and values["steps"] == values["added"], "silent floor and additions")
    limits = numbers(single(text, r"^silent_limits (.*)$"))
    caps = numbers(single(text, r"^memory_budget_scope=partial_named_payload_proxy_v1 (.*)$"))
    workers = numbers(single(text, r"^ouvriers (.*)$"))
    require(workers == dict.fromkeys(("wspd", "rects", "rle", "prefiltre", "census", "expansion", "fold"), 1), "serialized stages")
    single(text, r"^temps_fold_mur_ms=\d+\.\d+ \(etages A et B, fold_inflight=1, fold_join=1, pic_mesure_en_vol=1\)$")
    stores = unique((k, line) for k, line in re.findall(r"^stockage_foret K=(\d+) (.*)$", text, re.M))
    require(set(stores) == set(cardinalities), "storage order coverage")
    for k, line in stores.items():
        fields = numbers(line)
        require(line.startswith("kind=csr_facet_keys_v1 "), "CSR per order")
        require(fields["exact"] == 1 and fields["deltas"] == cardinalities[k]["deltas"], "CSR accounting")
        require(fields["offset_dernier_parents"] == fields["cles_parents"] and fields["offset_dernier_nes"] == fields["cles_nes"], "CSR final offsets")
    stage = single(text, r"^temps_ms (.*)$")
    stage_ms = unique((key, float(value)) for key, value in re.findall(r"([A-Za-z_]+)[= ](\d+\.\d+)", stage))
    stage_ms["silent_incidence"] = float(single(text, r"^silent_incidence_ms=(\d+\.\d+) regularity=rank_window_and_local_descent vertical_maps=none$"))
    stage_ms["fold_wall"] = float(single(text, r"^temps_fold_mur_ms=(\d+\.\d+) .*"))
    stage_ms["cli_e2e"] = float(single(text, r"^cli_e2e_ms=(\d+\.\d+) includes_input_and_export=true archive_committed=false$"))
    return {"digests": digests, "cardinalities": cardinalities, "counts": counts,
            "silent": silent, "silent_limits": limits, "payload_caps": caps,
            "stage_workers": workers, "stage_ms": stage_ms,
            "pipeline_ms": float(single(text, r"^temps_mur_ms=(\d+\.\d+) .*")),
            "max_rss_kb": int(single(text, r"^rss_max_kb=(\d+)$")),
            "candidate_digest": single(text, r"^digest_candidates_v5_compat=([0-9a-f]{64})$"),
            "postprefilter_digest": single(text, r"^digest_postprefilter=([0-9a-f]{64})$")}


def inspect(live):
    capture = decode((HERE / "capture_manifest.json").read_bytes())
    entries = capture["files"]
    require(len(entries) == 66, "capture inventory floor")

    def raw(name):
        relative = Path(entries[name]["snapshot_path"])
        require(not relative.is_absolute() and ".." not in relative.parts, "unsafe snapshot path")
        path = HERE / relative
        require(path.resolve().is_relative_to(HERE), "snapshot symlink escape")
        data = path.read_bytes()
        require(len(data) == entries[name]["bytes"] and sha(data) == entries[name]["sha256"], "capture integrity: " + name)
        return data

    def obj(name):
        return decode(raw(name))

    for name in entries:
        raw(name)
    manifest = obj("manifest.json")
    require(set(manifest) == set(entries) - {"manifest.json", "SHA256SUMS", "SHA256SUMS.root"}, "source manifest exhaustiveness")
    for name, pin in manifest.items():
        require(pin == {"bytes": len(raw(name)), "sha256": sha(raw(name))}, "source manifest pin " + name)
    for name, prefix in (("SHA256SUMS", ""), ("SHA256SUMS.root", capture["source_root"] + "/")):
        sums = unique((path.removeprefix(prefix), digest) for digest, path in re.findall(r"^([0-9a-f]{64})  (.+)$", raw(name).decode(), re.M))
        require(set(sums) == set(entries) - {"SHA256SUMS", "SHA256SUMS.root"}, "sum inventory")
        require(all(sha(raw(path)) == digest for path, digest in sums.items()), "sum hashes")
    provenance = obj("provenance.json")
    require(len(provenance["mappings"]) == provenance["byte_exact_files"] == provenance["copied_files"] == 58, "copy nonvacuum")
    require(len({item["public_path"] for item in provenance["mappings"]}) == 58, "copy uniqueness")
    for item in provenance["mappings"]:
        data = raw(item["public_path"])
        require(item["source_sha256"] == item["public_sha256"] == sha(data), "copy hashes")
        require(item["source_bytes"] == item["public_bytes"] == len(data), "copy length")
        require(item["transform"] == "none_byte_exact", "copy transform")
    results = []
    previous_end = None
    build_e = obj("build_E/build_D.json")
    require(build_e == obj("build_E/build_record.provisional.json"), "provisional and closed E build")
    build_runs = obj("build_E/runs.json")
    require(len(build_runs) == 4, "build command count")
    for run in build_runs:
        require(run["returncode"] == 0 and run["status"] == "completed" and run["timed_out"] is False, "build return")
    for s in (8, 10, 12):
        base = f"pairs/s{s}/"
        meta, runs, summary = (obj(base + name) for name in ("metadata.json", "runs.json", "summary.json"))
        require(meta["snapshot_before"] == obj(base + "snapshot_after.json"), "pair source stability")
        require(len(meta["snapshot_before"]["repository_sources"]) == 99, "repository pin floor")
        require(meta["snapshot_before"]["candidate_sources"] == build_e["sources_before"], "candidate source snapshot")
        require(not any(meta["snapshot_before"]["loader_injection_nonempty"].values()), "loader injection")
        require(meta["run_order"] == ["D", "E"] and len(runs) == 2, "pair cardinality/order")
        require(meta["candidate"]["record"] == build_e, "candidate build reference")
        require(meta["candidate"]["receipt_sha256"] == sha(raw("build_E/build_D.json")), "candidate build receipt hash")
        require(meta["host_before"]["host_shared"] is True and 6 in meta["host_before"]["affinity_cpus"], "host scope")
        require(meta["timeout_per_run_seconds"] == 600 and meta["rlimit_as_gib"] == 26 and meta["payload_proxy_bytes"] == 17179869184, "execution limits")
        projected = []
        for role, run in zip(("D", "E"), runs):
            record = meta["baseline" if role == "D" else "candidate"]["record"]
            require(len(record["sources_before"]) == 50 and record["sources_before"] == record["sources_after"], "build source stability")
            require(record["status"] == "completed" and record["build_exit_code"] == 0, "build status")
            require(run["role"] == role and run["separation"] == s, "run identity")
            require(type(run["returncode"]) is int and run["returncode"] == 0 and run["timed_out"] is False and run["status"] == "engine_completed", "engine return")
            require(raw(base + role + ".err") == b"", "engine stderr")
            require(run["command"][0] == record["binary"]["path"], "command binary")
            require(run["wrapped_command"][:3] == ["/usr/bin/taskset", "--cpu-list", "6"] and run["wrapped_command"][8:] == run["command"], "wrapper identity")
            required_flags = {"--family=uniform", "--n=8000", "--coord=65536", "--seed=3", f"--s={s}", "--smax=11", "--threads=1", "--fold-inflight=1", "--fold-join=1", "--layout=csr", "--digest", "--complete-incidences", "--mem-budget=17179869184", "--silent-core-records=8000000", "--silent-chain-steps=2000000", "--silent-cofaces=2000000", "--silent-query-nodes=1000000000", "--silent-meb-supports=1000000000"}
            require(len(run["command"][1:]) == len(required_flags) and set(run["command"][1:]) == required_flags, "exact invocation flags")
            parsed = parse_raw(raw(base + role + ".out").decode())
            for key, value in parsed.items():
                if key in run:
                    require(run[key] == value, "raw/run disagreement: " + key)
            require(parsed["counts"]["s"] == s, "raw separation")
            usage = raw(base + role + ".time").decode()
            require(single(usage, r'^\s*Command being timed: "(.*)"$') == " ".join(run["command"]), "GNU command")
            require(single(usage, r"^\s*Exit status: (\d+)$") == "0", "GNU exit")
            require(int(single(usage, r"^\s*Maximum resident set size \(kbytes\): (\d+)$")) == parsed["max_rss_kb"], "RSS raw/GNU")
            clock_text = single(usage, r"^\s*Elapsed \(wall clock\) time \(h:mm:ss or m:ss\): ([\d:.]+)$")
            wall = 0.0
            for component in clock_text.split(":"):
                wall = wall * 60 + float(component)
            require(abs(wall - run["wall_seconds"]) < 0.1, "GNU/outer wall")
            start, end = timestamp(run["started_utc"]), timestamp(run["ended_utc"])
            require(end > start and abs((end - start).total_seconds() - run["wall_seconds"]) < 0.1, "UTC/monotonic wall")
            require(previous_end is None or start > previous_end, "run chronology")
            previous_end = end
            parsed.update({"role": role, "separation": s, "started_utc": run["started_utc"], "ended_utc": run["ended_utc"], "outer_wall_seconds": run["wall_seconds"], "gnu_wall_seconds": wall, "returncode": 0, "binary": record["binary"]})
            projected.append(parsed)
        equal_fields = ("digests", "cardinalities", "counts", "silent", "silent_limits", "payload_caps", "candidate_digest", "postprefilter_digest")
        require(all(projected[0][field] == projected[1][field] for field in equal_fields), "within-pair projections")
        require(summary["status"] == "paired_equal" and summary["source_stable"] is True and summary["error"] is None and summary["expected_runs"] == summary["runs"] == 2, "summary agreement")
        require(timestamp(summary["started_utc"]) <= timestamp(projected[0]["started_utc"]) and timestamp(summary["ended_utc"]) >= previous_end, "pair containment")
        results.append({"separation": s, "runs": projected, "host": meta["host_before"], "repository_before": meta["repository_before"], "repository_after": obj(base + "repository_after.json"), "observed_wall_reduction_percent": 100 * (1 - projected[1]["outer_wall_seconds"] / projected[0]["outer_wall_seconds"]), "within_pair_equal_fields": list(equal_fields)})
    flat = [run for pair in results for run in pair["runs"]]
    invariant = ("digests", "cardinalities", "silent", "silent_limits", "payload_caps", "postprefilter_digest")
    require(all(run[field] == flat[0][field] for run in flat for field in invariant), "inter-separation projections")
    separation_dependent = {"s", "emis", "boules_uniques", "mortes_profondeur"}
    require(all({k: v for k, v in run["counts"].items() if k not in separation_dependent} == {k: v for k, v in flat[0]["counts"].items() if k not in separation_dependent} for run in flat), "inter-separation counts after prefilter")
    live_checks = None
    if live:
        checks = {}

        def local_pin(path, expected):
            file = Path(path)
            file = file if file.is_absolute() else REPO / file
            require(file.is_file() and sha(file.read_bytes()) == expected, "live pin " + str(path))
            checks[str(path)] = expected

        for item in provenance["mappings"]:
            local_pin(item["source_path"], item["source_sha256"])
        for item in obj("dependencies.json")["existing_public_files"]:
            local_pin(item["repository_relative_path"], item["sha256"])
        meta = obj("pairs/s12/metadata.json")
        for group in ("files", "repository_sources"):
            for path, pin in meta["snapshot_before"][group].items():
                local_pin(path, pin)
        for role in ("baseline", "candidate"):
            record = meta[role]["record"]
            for kind in ("binary", "compile_database", "cmake_cache"):
                local_pin(record[kind]["path"], record[kind]["sha256"])
            database = decode(Path(record["compile_database"]["path"]).read_bytes())
            require(record["compile_command"] in database, "CLI actual compile entry")
        d_sources = meta["baseline"]["record"]["sources_before"]
        d_commit = capture["head"]
        for name, pin in d_sources.items():
            data = subprocess.run(["git", "show", f"{d_commit}:morsehgp3D_v7/{name}"], cwd=REPO, check=True, capture_output=True).stdout
            require(sha(data) == pin, "D committed byte binding " + name)
        live_checks = {"unique_local_pins_verified": len(checks), "pins": checks, "D_committed_sources": len(d_sources), "D_commit": d_commit, "E_current_sources": len(build_e["sources_before"]), "private_copy_checks": 58, "dependency_checks": 14, "current_binary_compile_database_cache_checks": 6, "build_binding": "recorded_commands_and_source_boundaries_not_hermetic_attestation"}
    return {"schema": "mhgp7-q2-independent-replay-v1", "phase": "exploration_v7_hors_registre", "backend": "cpu_reference", "profile": "quantized_u16_input_only", "mode": "audit_independant_math_and_architecture", "public_status": "not_claimed", "gcp": "not_used", "status": "verified_bounded_raw_projections", "engine_runs_executed_by_this_audit": 0, "capture_sha256": sha((HERE / "capture_manifest.json").read_bytes()), "source_files_captured": 66, "source_manifest_files": 63, "each_checksum_list_files": 64, "source_origin_byte_exact_files": 58, "raw_engine_logs": 6, "inter_separation_equal_fields": list(invariant) + ["counts_after_prefilter_and_input_identity"], "inter_separation_differences": ["s", "candidate_digest", "emis", "boules_uniques", "mortes_profondeur"], "pairs": results, "live_checks": live_checks, "limits": ["One D_then_E pair per separation on one shared host and one uniform cloud; no statistical gain or SLO.", "Candidate completed horizontal K1..10 tower, not a vertical map or a certification of full HGP.", "Only printed cardinalities and digests agree; no archive was exported, so no bytewise forest comparison or independent full object reconstruction.", "Worker flags and affinity observed; actual thread creation not measured by this runner.", "Mono E executions do not transfer D's 323-test full qualification to E.", "GNU time RSS and partial payload proxy are distinct; RLIMIT_AS is not a physical RSS budget."]}


def self_test():
    capture = decode((HERE / "capture_manifest.json").read_bytes())
    text = (HERE / capture["files"]["pairs/s8/D.out"]["snapshot_path"]).read_text()
    parse_raw(text)
    mutations = {"empty": "", "duplicate_digest": text + "\n" + single(text, r"^(digest_all=.*)$") + "\n", "missing_k10": re.sub(r"^cardinalites K=10 .*\n", "", text, flags=re.M), "wrong_total": text.replace("evenements=4384229", "evenements=4384230"), "wrong_scope": text.replace("normalized_horizontal_h0_candidate", "verified_events_only"), "zero_facets": text.replace("facettes=8000 deltas=7999", "facettes=0 deltas=7999"), "bad_chain": text.replace("digest_all=4", "digest_all=5"), "workers": text.replace("ouvriers wspd=1", "ouvriers wspd=2")}
    for name, mutant in mutations.items():
        try:
            parse_raw(mutant)
        except (ValueError, KeyError):
            continue
        raise ValueError("mutant survived: " + name)
    try:
        decode('{"key":1,"key":2}')
    except ValueError:
        pass
    else:
        raise ValueError("duplicate JSON survived")
    return {"status": "passed", "positive": 1, "raw_rejections": sorted(mutations), "json_duplicate_rejection": True, "exitcode": 0}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--live", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    try:
        result = self_test() if args.self_test else inspect(args.live)
    except (ValueError, KeyError, OSError, TypeError, subprocess.CalledProcessError) as error:
        print(json.dumps({"status": "rejected", "error": str(error)}))
        return 1
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main())
