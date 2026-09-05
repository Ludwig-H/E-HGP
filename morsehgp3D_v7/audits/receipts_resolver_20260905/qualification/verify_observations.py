#!/usr/bin/env python3
"""Independent parser of closed E/F pairs and F scale receipts; no engine calls."""
import argparse
from datetime import datetime
import hashlib
import json
from pathlib import Path
import re
import sys

HERE = Path(__file__).resolve().parent
AUDITS = HERE.parents[1]
ROOT = AUDITS.parents[1]
F_SHA = "ee29d3d5cfb49a728fa9dfa44fdb85a5a6043c941b1f61d4a6d9531ea4671f85"
E_SHA = "df75153326f7bbf4ce0a412031a365205559cb68155d4304adc9301461f505f6"


def require(value, reason):
    if not value:
        raise ValueError(reason)


def unique(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, "duplicate key " + key)
        result[key] = value
    return result


def decode(data):
    return json.loads(data, object_pairs_hook=unique)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def one(pattern, data):
    values = re.findall(pattern, data, re.M)
    require(len(values) == 1, "exactly one " + pattern)
    return values[0]


def integer_fields(data):
    return unique([(key, int(value)) for key, value in re.findall(r"(\w+)=(\d+)(?= |$)", data)])


def parse_success(raw, n, separation):
    for line in (
        "payload=mhgp7-forests-horizontal-v1 authority=status_terminal callbacks=provisional vertical_maps=none",
        "backend=cpu_reference",
        "forest_semantics=normalized_horizontal_h0_candidate public_status=not_claimed require_exact=false",
        "tower_scope=profile_complete_k10 smax_requested=11 smax_effective=11",
        "forest_layout=csr forest_storage_kind=csr_facet_keys_v1 csr_fallback=0 ordres_publies=10 ordres_storage_conformes=10",
    ):
        require(raw.splitlines().count(line) == 1, "success scope " + line)
    digests = unique(re.findall(r"^(digest_forest_K\d+|digest_all)=([0-9a-f]{64})$", raw, re.M))
    require(set(digests) == {"digest_all"} | {"digest_forest_K" + str(k) for k in range(1, 11)}, "eleven digests")
    cards = unique([(k, integer_fields(values)) for k, values in re.findall(r"^cardinalites K=(\d+) (.+)$", raw, re.M)])
    require(set(cards) == {str(k) for k in range(1, 11)} and cards["1"]["facettes"] == n, "ten nonempty orders")
    require(all(set(card) == {"evenements", "facettes", "deltas", "attachements", "fusions", "noeuds"} and card["deltas"] > 0 for card in cards.values()), "cardinality nonvacuum")
    counts = integer_fields(one(r"^famille=uniform (.+)$", raw))
    require(all(counts[k] == v for k, v in {"n": n, "coord": 65536, "seed": 3, "s": separation, "smax": 11, "threads": 1}.items()), "raw input identity")
    for field in ("evenements", "facettes", "deltas", "fusions", "noeuds"):
        require(sum(card[field] for card in cards.values()) == counts[field], "cardinality sum " + field)
    silent = unique([(k, integer_fields(values)) for k, values in re.findall(r"^silent_K(\d+) (.+)$", raw, re.M)])
    require(set(silent) == {str(k) for k in range(2, 11)}, "nine silent orders")
    limits = integer_fields(one(r"^silent_limits (.+)$", raw))
    require(limits == {"core_records": 8000000, "chain_steps": 2000000, "cofaces": 2000000, "query_nodes": 1000000000, "meb_supports": 1000000000}, "silent budgets")
    caps = integer_fields(one(r"^memory_budget_scope=partial_named_payload_proxy_v1 (.+)$", raw))
    require(caps["budget"] == 17179869184, "16 GiB partial payload proxy")
    for values in silent.values():
        require(values["added"] == values["steps"] <= limits["chain_steps"] and values["query_nodes"] <= limits["query_nodes"] and values["meb_supports"] <= limits["meb_supports"], "observed silent work within corresponding budgets")
    times_line = one(r"^temps_ms (.+)$", raw)
    times = {key: float(one(r"\b" + key + r"[= ]([0-9.]+)", times_line)) for key in ("index", "gen", "wspd", "rects", "rle", "prefiltre", "census", "comptage", "expansion", "fold", "tri", "intern", "fusion", "reduce", "digest")}
    for key, prefix in (("silent_incidence", "silent_incidence_ms"), ("fold_wall", "temps_fold_mur_ms"), ("cli_e2e", "cli_e2e_ms")):
        times[key] = float(one(r"^" + prefix + r"=([0-9.]+)", raw))
    pipeline = float(one(r"^temps_mur_ms=([0-9.]+)", raw))
    rss = int(one(r"^rss_max_kb=(\d+)$", raw))
    require(pipeline <= times["cli_e2e"] and rss > 0, "positive consistent timer/RSS")
    workers = integer_fields(one(r"^ouvriers (.+)$", raw))
    require(len(workers) == 7 and set(workers.values()) == {1}, "seven serialized stages reported")
    return {"digests": digests, "cardinalities": cards, "counts": counts, "silent": silent, "silent_limits": limits, "payload_caps": caps, "stage_ms": times, "pipeline_ms": pipeline, "max_rss_kb": rss, "stage_workers": workers}


def usage_values(raw):
    status = int(one(r"^\s*Exit status: (\d+)$", raw))
    rss = int(one(r"^\s*Maximum resident set size \(kbytes\): (\d+)$", raw))
    clock = one(r"^\s*Elapsed \(wall clock\) time \(h:mm:ss or m:ss\): ([0-9:.]+)$", raw)
    wall = 0.0
    for value in clock.split(":"):
        wall = wall * 60 + float(value)
    require(rss > 0 and wall > 0, "positive GNU time counters")
    return {"exit_code": status, "max_rss_kb": rss, "GNU_time_wall_seconds": wall}


def inspect(live=False):
    capture = decode((HERE / "capture_manifest.json").read_bytes())
    scopes = capture["scopes"]

    def raw(scope, name):
        entry = scopes[scope][name]
        path = AUDITS / entry["audit_path"]
        require(path.resolve().is_relative_to(AUDITS), "capture stays in audits")
        data = path.read_bytes()
        require(sha(data) == entry["sha256"] and len(data) == entry["bytes"], "capture integrity " + scope + "/" + name)
        return data

    def obj(scope, name):
        return decode(raw(scope, name))

    for scope, files in scopes.items():
        for name in files:
            raw(scope, name)
    published = set(scopes["published"])
    sums = {}
    for line in raw("published", "SHA256SUMS").decode().splitlines():
        digest, path = line.split("  ", 1)
        require(path not in sums and sha(raw("published", path)) == digest, "public checksum")
        sums[path] = digest
    require(set(sums) == published - {"SHA256SUMS", "SHA256SUMS.root"}, "checksum exhaustiveness")
    root_sums = raw("published", "SHA256SUMS.root").decode()
    require(root_sums == "".join(digest + "  morsehgp3D_v7/receipts/witness_stack_mono_20260905/" + path + "\n" for path, digest in sums.items()), "root checksum projection")
    manifest = obj("published", "manifest.json")
    require(set(manifest) == published - {"manifest.json", "SHA256SUMS", "SHA256SUMS.root"}, "public manifest exhaustiveness")
    for name, pin in manifest.items():
        require(sha(raw("published", name)) == pin["sha256"] and len(raw("published", name)) == pin["bytes"], "public manifest pin")
    provenance = obj("published", "provenance.json")
    require(len(provenance["mappings"]) == 61, "61 exported originals")
    for mapping in provenance["mappings"]:
        data = raw("published", mapping["public_path"])
        require(mapping["source_sha256"] == mapping["public_sha256"] == sha(data) and mapping["source_bytes"] == mapping["public_bytes"] == len(data) and mapping["transform"] == "none_byte_exact", "byte-exact export")
    build = obj("published", "build_F/build_D.json")
    require(build["sources_before"] == build["sources_after"] and len(build["sources_before"]) == 51 and build["binary"]["sha256"] == F_SHA and build["build_exit_code"] == 0, "own F CLI binding")
    require(sha(raw("source", "silent_incidence.hpp")) == build["sources_before"]["src/forest/silent_incidence.hpp"], "source guard is F source")
    for name, pin in obj("published", "build_F/SOURCE_HASHES.json").items():
        require(sha(raw("published", "build_F/" + name)) == pin, "private F build checksum")
    build_summary = obj("published", "build_F/summary.json")
    require(build_summary["status"] == "completed" and build_summary["error"] is None and build_summary["sources_helpers_C_stable"] is True, "closed build receipt")
    build_runs = obj("published", "build_F/runs.json")
    require(len(build_runs) == 4 and {run["name"] for run in build_runs} == {"compiler_version", "cmake_version", "configure", "build"}, "four build commands")
    for run in build_runs:
        require(run["status"] == "completed" and run["returncode"] == 0 and run["timed_out"] is False and not raw("published", "build_F/" + run["name"] + ".err"), "build command completed")
    parsed_pairs = {}
    measurements = []
    live_pins = {}

    def check_snapshot(metadata, after):
        before = metadata["snapshot_before"]
        require(before == after, "before/after source binary identity")
        require(len(before["candidate_sources"]) == 51 and before["candidate_sources"] == build["sources_before"], "51 F source binding")
        require(not any(before["loader_injection_nonempty"].values()), "no recorded loader injection")
        require(metadata["rlimit_as_gib"] == 26 and metadata["rlimit_as_is_not_physical_RSS_limit"] is True and metadata["payload_proxy_bytes"] == 17179869184, "distinct virtual/payload budgets")
        require(metadata["silent_limits"] == {"core_records": 8000000, "chain_steps": 2000000, "cofaces": 2000000, "query_nodes": 1000000000, "meb_supports": 1000000000}, "metadata completion caps")
        require(metadata["public_status"] == "not_claimed" and metadata["archive"] is False and metadata["digest"] is True, "metadata output scope")
        candidate = metadata["candidate"]["record"]["binary"]
        require(candidate["sha256"] == F_SHA and before["files"][candidate["path"]] == F_SHA, "F identity")
        live_pins.update(before["files"])
        live_pins.update(before["repository_sources"])
        live_pins.update({"morsehgp3D_v7/" + name: pin for name, pin in before["candidate_sources"].items()})
        live_pins.update({"build/v7_f_scale_20260905/" + name: pin for name, pin in before.get("scale_protocol_files", {}).items()})

    def check_run(scope, prefix, record, n, separation, refused=False):
        stdout = raw(scope, prefix + record["stdout"]).decode()
        stderr = raw(scope, prefix + record["stderr"]).decode()
        usage_raw = raw(scope, prefix + record["usage"]).decode()
        usage = usage_values(usage_raw)
        require(one(r'^\s*Command being timed: "(.+)"$', usage_raw) == " ".join(record["command"]), "actual GNU time command")
        expected_binary = "/workspaces/E-HGP/build/" + ("v7_next_q2_qualification" if record["role"] == "E" else "v7_f_qualification") + "/mhgp7"
        require(record["command"][0] == expected_binary, "actual role-specific engine")
        require(record["wrapped_command"][:3] == ["/usr/bin/taskset", "--cpu-list", "6"] and record["wrapped_command"][-len(record["command"]):] == record["command"], "recorded wrapper command/affinity")
        require(record["timed_out"] is False and type(record["returncode"]) is int, "terminal non-timeout engine")
        require(record["returncode"] == usage["exit_code"] == (2 if refused else 0), "record/raw exit code")
        require(abs(record["wall_seconds"] - usage["GNU_time_wall_seconds"]) < 0.15, "process clock agreement")
        elapsed = (datetime.fromisoformat(record["ended_utc"]) - datetime.fromisoformat(record["started_utc"])).total_seconds()
        require(abs(elapsed - record["wall_seconds"]) < 0.15 and record["wall_seconds"] < 600, "timestamp/clock/bound")
        for flag in ("--n=" + str(n), "--s=" + str(separation), "--seed=3", "--coord=65536", "--smax=11", "--complete-incidences", "--digest", "--threads=1", "--fold-inflight=1", "--fold-join=1", "--mem-budget=17179869184", "--silent-core-records=8000000", "--silent-chain-steps=2000000", "--silent-cofaces=2000000", "--silent-query-nodes=1000000000", "--silent-meb-supports=1000000000"):
            require(record["command"].count(flag) == 1, "actual command flag " + flag)
        if refused:
            require(record["status"] == "engine_refused" and stdout == "", "closed refusal, no published payload")
            reason = one(r"^(REFUS silent incidence K=9 : silent_core_record_budget)$", stderr)
            require(record["reason"] == reason and record["refusal_status"] == "resource_exhausted" and record["refusal_order"] == 9, "refusal classification")
            require("refus_etage=fold" in stderr and "silent_refusal_K9 core=0 steps=0 added_provisional=0 query_nodes=0 meb_supports=0" in stderr, "provisional refusal diagnostics")
            diagnostics = integer_fields(one(r"^silent_refusal_work (.+)$", stderr))
            total = float(one(r"^silent_refusal_work .*total_ms=([0-9.]+)", stderr))
            completion = float(one(r"^silent_refusal_work .*completion_ms=([0-9.]+)", stderr))
            require(0 < completion < total < record["wall_seconds"] * 1000, "refusal diagnostic timing")
            return {"status": "engine_refused", "reason": reason, "diagnostic_total_ms": total, "diagnostic_completion_ms": completion, "diagnostic_counts": diagnostics, **usage}
        require(record["status"] == "engine_completed" and stderr == "", "successful raw engine")
        parsed = parse_success(stdout, n, separation)
        for field, value in parsed.items():
            require(value == record[field], "independent extraction matches " + field)
        require(parsed["max_rss_kb"] == usage["max_rss_kb"] and parsed["stage_ms"]["cli_e2e"] <= record["wall_seconds"] * 1000 + 1, "raw/output RSS and timer agreement")
        measurements.append({"n": n, "s": separation, "role": record["role"], "wall_seconds": record["wall_seconds"], "started_utc": record["started_utc"], "ended_utc": record["ended_utc"], "pipeline_ms": parsed["pipeline_ms"], "stage_ms": parsed["stage_ms"], "max_rss_kb": parsed["max_rss_kb"]})
        return parsed

    for separation in (8, 10, 12):
        prefix = "pairs/s" + str(separation) + "/"
        metadata = obj("published", prefix + "metadata.json")
        check_snapshot(metadata, obj("published", prefix + "snapshot_after.json"))
        require(metadata["timeout_per_run_seconds"] == 600, "pair process deadline")
        baseline = metadata["baseline"]["record"]["binary"]
        require(baseline["sha256"] == E_SHA and metadata["snapshot_before"]["files"][baseline["path"]] == E_SHA, "E retains own binary identity")
        source_hashes = obj("published", prefix + "SOURCE_HASHES.json")
        for name, pin in source_hashes.items():
            require(sha(raw("published", prefix + name)) == pin, "private pair hashes")
        runs = obj("published", prefix + "runs.json")
        summary = obj("published", prefix + "summary.json")
        require(len(runs) == 2 and [run["role"] for run in runs] == ["E", "F"], "fresh E then F arms")
        require(summary["status"] == "paired_equal" and summary["error"] is None and summary["runs"] == 2 and summary["source_stable"] is True, "closed successful pair")
        require(runs[0]["ended_utc"] < runs[1]["started_utc"], "sequential pair intervals")
        parsed = [check_run("published", prefix, run, 8000, separation) for run in runs]
        for field in ("digests", "cardinalities", "counts", "silent", "silent_limits", "payload_caps"):
            require(parsed[0][field] == parsed[1][field], "intra-s equality " + field)
        parsed_pairs[str(separation)] = {field: parsed[0][field] for field in ("digests", "cardinalities")}
    require(parsed_pairs["8"] == parsed_pairs["10"] == parsed_pairs["12"], "inter-s objects, not workload, equal")
    scale = {}
    for n in (16000, 32000):
        scope = "n" + str(n)
        metadata = obj(scope, "metadata.json")
        check_snapshot(metadata, obj(scope, "snapshot_after.json"))
        require(metadata["timeout_seconds"] == 600 and metadata["comparison"] == "none_F_only", "scale bounds and attribution")
        hashes = obj(scope, "hashes.json")
        require(set(hashes) == set(scopes[scope]) - {"hashes.json"}, "scale manifest exhaustive")
        for name, pin in hashes.items():
            require(sha(raw(scope, name)) == pin, "scale manifest hash")
        runs = obj(scope, "runs.json")
        summary = obj(scope, "summary.json")
        require(len(runs) == 1 and summary["status"] == "observations_completed" and summary["error"] is None and summary["source_stable"] is True, "closed scale receipt")
        refused = n == 32000
        require(summary["engine_successes"] == (0 if refused else 1) and summary["engine_refusals"] == (1 if refused else 0) and summary["censored"] == summary["invalid"] == 0, "receipt completion differs from engine success")
        scale[str(n)] = check_run(scope, "", runs[0], n, 8, refused)
    live_result = {"performed": live}
    if live:
        stale = []
        for name, pin in live_pins.items():
            path = Path(name)
            path = path if path.is_absolute() else ROOT / path
            if sha(path.read_bytes()) != pin:
                stale.append(name)
        live_result.update({"pins": len(live_pins), "mismatches": stale, "historical_receipt_validity_not_reassigned_from_worktree": True})
    return {"status": "three_pairs_equal_F16k_success_F32k_closed_budget_refusal", "public_status": "not_claimed", "phase": "exploration_v7_hors_registre", "backend": "cpu_reference", "profile": "quantized_u16_input_only", "mode": "audit_independant_math_and_architecture", "engine_build_runs": 0, "gcp": "not_used", "capture_sha256": sha((HERE / "capture_manifest.json").read_bytes()), "public_files_verified": len(published), "previous_bytes_referenced": sum(entry["preservation"] == "existing_identical_audit_bytes" for files in scopes.values() for entry in files.values()), "cross_s_reference": parsed_pairs["8"], "measurements": measurements, "scale": scale, "live": live_result, "limits": ["One cold ordered pair per s and one unpaired F observation per larger n; no statistical gain/SLO or exactness claim.", "Raw scope is normalized horizontal candidate, terminal authority, provisional callbacks, no vertical maps or archive.", "RSS, virtual-address RLIMIT and partial named payload proxy are distinct quantities.", "F32k refusal has no published forest/digest: diagnostic work is provisional, duration is time to refusal.", "Recorded source/build binding is not hermetic attestation."]}


def self_test():
    capture = decode((HERE / "capture_manifest.json").read_bytes())
    entry = capture["scopes"]["published"]["pairs/s8/F.out"]
    raw = (AUDITS / entry["audit_path"]).read_text()
    parse_success(raw, 8000, 8)
    mutants = {
        "empty_output": "",
        "refusal_promoted_to_success": "REFUS silent incidence K=9 : silent_core_record_budget\n",
        "exact_claim": raw.replace("public_status=not_claimed", "public_status=exact"),
        "vertical_maps_claim": raw.replace("vertical_maps=none", "vertical_maps=complete"),
        "missing_digest": re.sub(r"^digest_all=.*\n", "", raw, flags=re.M),
        "duplicate_digest": raw + "digest_all=4c3ceb0498990bafa41a9e43d0bffe25a3fee579b12b5d34365f3578f526a0e7\n",
        "bad_cardinality_sum": raw.replace("cardinalites K=2 evenements=67539", "cardinalites K=2 evenements=67540"),
        "RSS_zero": re.sub(r"^rss_max_kb=\d+", "rss_max_kb=0", raw, flags=re.M),
        "over_meb_cap": raw.replace("meb_supports=394768070", "meb_supports=1000000001"),
        "workers_two": raw.replace("ouvriers wspd=1", "ouvriers wspd=2"),
    }
    for name, mutated in mutants.items():
        try:
            parse_success(mutated, 8000, 8)
        except ValueError:
            continue
        raise ValueError("parser mutant survived " + name)
    try:
        decode('{"status":0,"status":1}')
    except ValueError:
        pass
    else:
        raise ValueError("duplicate JSON survived")
    return {"status": "passed", "positive": 1, "rejections": sorted(list(mutants) + ["duplicate_JSON"])}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--live", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    try:
        print(json.dumps(self_test() if args.self_test else inspect(args.live), indent=2, sort_keys=True))
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(json.dumps({"status": "rejected", "error": str(error)}))
        sys.exit(1)
