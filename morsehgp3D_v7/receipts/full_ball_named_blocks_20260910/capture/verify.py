"""Read-only verifier of preparation, never a 50k/GCP runner. Also works -O."""
import copy
import hashlib
import json
from pathlib import Path

BASE = Path(__file__).resolve().parent
AUDIT = BASE / "authorities/morsehgp3D_v7/audits/receipts_plateaux_full_20260906"
RAW = BASE / "authorities/morsehgp3D_v7/receipts/full_extra_shell_50000_20260906/run_r3/n50000_k10.stderr"


def need(good, why):
    if not good: raise RuntimeError(why)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check_expectations(expected):
    parents = json.loads((AUDIT / "real_parent_certificates_normal.json").read_text())
    census = json.loads((AUDIT / "real_shell_census_normal.json").read_text())
    raw = {row["ball_index"]: row for line in RAW.read_text().splitlines()
           if line.startswith('{"schema":"mhgp7-extra-shell-diagnostic-v1"')
           for row in [json.loads(line)]}
    rows = {row["ball_index"]: row for row in census["records"]}
    need(parents["status"] == "passed" and parents["exact_global_parent_relation_for_these_three_blocks"], "authority global parents")
    need(expected["input"]["digest"] == parents["input_digest"] == census["input"]["digest"], "input digest pin")
    need([expected["input"][k] for k in ("n","seed","coord","s","kmax","smax")] == [50000,3,65536,8,10,11], "input parameters")
    need(len(expected["blocks"]) == 4 and {row["capture_label"] for row in expected["blocks"]} == set(raw), "four named labels")
    for row in expected["blocks"]:
        label = row["capture_label"]; source = raw[label]; independent = rows[label]
        need(source["input_digest"] == expected["input"]["digest"], "raw input pin")
        need(row["ball_key"] == source["ball_key"] == independent["local_analysis"]["ball_key"], "exact BallKey pin")
        level = source["squared_radius"]
        need([row["level_num"],0,0] == level["numerator_u64_le"] and str(row["level_den"]) == level["denominator"], "exact level pin")
        for kind in ("interior","shell"):
            need(row[kind] == sorted(site["point_id"] for site in source[kind]) == independent["census"][kind+"_point_ids"], "census identity pin")
        local = independent["local_analysis"]
        local_rank = next((r for r in local["orders"] if r["k"] == row["k"]),None)
        need(local_rank is not None and local_rank["local_uncovered_point_ids"] == [], "empty local contribution pin")
        need(row["expected_contribution_mask"] == 0 and row["expected_contribution_interior"] is False, "empty contribution pin")
        if str(label) in parents["cases"]:
            case = parents["cases"][str(label)]
            need(row["k"] == case["k"], "parent order pin")
            wanted = 1 if case["verdict"] == "same_global_parent" else 2
            need(local_rank["strict_component_count"] == 2 and row["expected_roots"] == wanted, "parent cardinality pin")
        else:
            need(label == 1251653 and row["k"] == 10 and local["anchor_interval"] == [10,10], "inert K10 anchor pin")
            need(local_rank["strict_component_count"] == 1 and row["expected_roots"] == 1, "inert K10 root pin")


def check():
    expected = json.loads((BASE / "expectations.json").read_text())
    check_expectations(expected)
    mutations = [lambda x: x["input"].update(digest="0"*64),
                 lambda x: x["blocks"][0].update(expected_roots=2),
                 lambda x: x["blocks"][0]["ball_key"].update(a="2"),
                 lambda x: x["blocks"][0].update(interior=[]),
                 lambda x: x["blocks"][3].update(k=9),
                 lambda x: x["blocks"].pop()]
    for mutate in mutations:
        changed = copy.deepcopy(expected); mutate(changed)
        try: check_expectations(changed)
        except RuntimeError: continue
        raise RuntimeError("expectation mutation survived")
    for mode in ("o2_r2","san_r2"):
        description = json.loads((BASE / f"logs/describe_{mode}.stdout").read_text())
        need(description == expected["blocks"], "compiled expectations differ from pinned JSON")
        result = json.loads((BASE / f"logs/selftest_{mode}.stdout").read_text())
        need(result["status"] == "passed" and result["cases"] == 2 and result["matched_blocks"] == 4, "small selftest scope")
        need(result["observer_rejections"] == 8 and result["strict_root_checks"] == 68 and result["grouped_lots"] == 17, "observer nonvacuity")
        need(result["physical_outputs_equal"] and result["mutant_inconclusive_rejections"] == 1, "transactional observer checks")
        need(result["raw_anchor_mutant_killed"] and result["raw_anchor_mutant_reason"] == "named.pre_root_not_live", "causal mutant reason")
        need(result["real_50000_executed"] is False and result["benchmark"] is False, "prepared scope")
    need((BASE / "logs/selftest_o2_r2.stdout").read_bytes() == (BASE / "logs/selftest_san_r2.stdout").read_bytes(), "O2/SAN stdout mismatch")
    for path in sorted((BASE / "logs").glob("*.json")):
        row = json.loads(path.read_text())
        need("--pinned-50000" not in row["argv"], "unexpected heavy local execution")
        need(row["exit_code"] == row["expected_exit_code"], "command code")
        for suffix in ("stdout","stderr"):
            need(sha(path.with_suffix("."+suffix)) == row[suffix+"_sha256"], "command log pin")
        if path.stem.startswith(("compile_","selftest_","describe_")):
            need(not path.with_suffix(".stderr").read_bytes(), "unexpected successful diagnostics")
    for name,digest in json.loads((BASE / "source_before.json").read_text()).items():
        need(sha(BASE / "baseline" / name) == digest,"baseline source pin")
    for name,digest in json.loads((BASE / "authority_pins.json").read_text()).items():
        need(sha(BASE / "authorities" / name) == digest,"authority source pin")
    if (BASE / "overlay_after.json").exists():
        for name,digest in json.loads((BASE / "overlay_after.json").read_text()).items():
            need(sha(BASE / "overlay" / name) == digest,"final overlay source pin")
    return {"status":"NOT_EXECUTED_50K","small_observer_tests":"passed_O2_ASAN_UBSAN",
            "expectation_mutants_rejected":len(mutations),"GCP_used":False,"public_status":"not_claimed"}


if __name__ == "__main__":
    for name,digest in json.loads((BASE / "manifest.json").read_text()).items():
        relative = Path(name)
        need(not relative.is_absolute() and ".." not in relative.parts,"unsafe manifest path")
        need(sha(BASE / relative) == digest,"manifest file pin: "+name)
    print(json.dumps(check(),sort_keys=True))
