#!/usr/bin/env python3
"""Read-only portable verification preserving historical failure statuses."""
import hashlib
import json
from pathlib import Path, PurePosixPath
import posixpath
import re
import shlex

BASE = Path(__file__).resolve().parent
CAP = BASE / "capture"
HEADER = "morsehgp3D_v7/src/forest/full_ball_tower.hpp"
REASONS = {
    "drop_growth_grouped": "FAIL [growth_ABCZ_doubled_lot/variant0] gamma.coverage_multiset_including_multiplicity\n",
    "drop_inert_grouped": "product refusal [growth_ABCZ_doubled_lot/variant0]: full_ball_vertical_birth_anchor\nFAIL [growth_ABCZ_doubled_lot/variant0] producer.complete_relative_orders\n",
    "drop_growth_singleton": "FAIL [growth_ABCZ/variant0] gamma.coverage_multiset_including_multiplicity\n",
    "drop_inert_singleton": "product refusal [E5/variant0]: full_ball_vertical_birth_anchor\nFAIL [E5/variant0] producer.complete_relative_orders\n",
    "strict_radius": "product refusal [actual_equal_radius_descent/variant0]: full_ball_radius_increased\nFAIL [actual_equal_radius_descent/variant0] producer.complete_relative_orders\n",
}
MUTATIONS = {
    "drop_growth_grouped": ("if (action.parents.size() != 1 || !action.contributions.empty()) batch.actions.push_back(std::move(action));", "if (action.parents.size() != 1) batch.actions.push_back(std::move(action));"),
    "drop_inert_grouped": ("for (size_t b = 0; b < blocks.size(); ++b) {\n      require(anchors[blocks[b].ball]", "for (size_t b = 0; b < blocks.size(); ++b) {\n      if (blocks[b].roots.size() == 1 && !blocks[b].contribution && !blocks[b].interior) continue;\n      require(anchors[blocks[b].ball]"),
    "drop_growth_singleton": ("if (action.parents.size() != 1 || !action.contributions.empty()) {", "if (action.parents.size() != 1) {"),
    "drop_inert_singleton": ("anchors[block.ball] = target;  // All representatives and the whole lot are closed.", "if (block.roots.size() != 1 || block.contribution || block.interior) anchors[block.ball] = target;"),
    "strict_radius": ('require(cmp <= 0, "full_ball_radius_increased");', 'require(cmp < 0, "full_ball_radius_increased");'),
    "wrong_vertical_cut": ("upper.lower_nodes[root], cut, closed);", "upper.lower_nodes[root], ExactLevel{U192{}, 1}, true);"),
    "wrong_vertical_cut_typed_retry": ("upper.lower_nodes[root], cut, closed);", "upper.lower_nodes[root], ExactLevel{{0, 0, 0}, 1}, true);"),
}


def need(value, reason):
    if not value:
        raise SystemExit("grouped_lot_receipt: " + reason)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(path.read_text())


manifest = read(BASE / "manifest.json")
need({str(path.relative_to(BASE)) for path in BASE.rglob("*") if path.is_file() and path.name != "manifest.json"} == set(manifest), "manifest inventory")
for name, digest in manifest.items():
    relative = PurePosixPath(name)
    need(not relative.is_absolute() and ".." not in relative.parts, "manifest path")
    need(sha(BASE / name) == digest, "manifest pin " + name)
pins = read(CAP / "run_r1/sources_before.json")
need(pins == read(CAP / "run_r2/sources_before.json") and len(pins) == 60, "shared original baseline")
need(pins[HEADER] == "0b72b4e9cb3858f7026d6b5d2b55f8a7b191903fc37aa24c140dfb8b557657e8", "historical FULL source")
need(pins["morsehgp3D_v7/tests/full_ball_tower_gate.cpp"] == "bf1a28242dd2d6897f3dc02edff8c6077dbec1caa3f9b9a67813dcc748e3d75a", "28-cloud gate source")
for name, digest in pins.items():
    need(sha(BASE / "source_snapshot" / name) == digest, "archived nominal source")
original_header = (BASE / "source_snapshot" / HEADER).read_text()
for name, (old, new) in MUTATIONS.items():
    need(original_header.count(old) == 1, "unique mutant site " + name)
    need((BASE / "mutants" / name / "full_ball_tower.hpp").read_text() == original_header.replace(old, new), "single mutation " + name)
archive = read(CAP / "archive_at_publication.json")
need(archive["scope"] == "archived_sources_checked_at_publication_not_historical_after_run", "archive audit scope")
need(archive["before"] == archive["after"], "archive publication drift")
for tree, kind in archive["trees"].items():
    expected = dict(pins)
    if kind != "baseline":
        expected[HEADER] = sha(BASE / "mutants" / kind / "full_ball_tower.hpp")
    need(archive["before"][tree] == expected, "archive variant closure " + tree)
closures = read(CAP / "compiled_project_closures.json")
need(len(closures) == 13, "compiler dependency records")
for name, expected in closures.items():
    run = name.split("/", 1)[0]
    observed = {}
    for spelling in shlex.split((CAP / name).read_text().replace("\\\n", " ").split(":", 1)[1]):
        normalized = posixpath.normpath(spelling)
        prefix = "/workspaces/E-HGP/build/v7_grouped_front_20260910/" + run + "/"
        need(normalized.startswith(prefix), "compiler project dependency outside archived tree")
        tree_name, relative = normalized[len(prefix):].split("/", 1)
        tree = run + "/" + tree_name
        need(relative in archive["before"][tree], "unexpected compiled dependency")
        observed[relative] = archive["before"][tree][relative]
    need(observed == expected and len(observed) >= 20, "compiled project closure " + name)
receipts = {run: read(CAP / run / "receipt.json") for run in ("run_r1", "run_r2", "run_r3_vertical")}
need([receipts[run]["status"] for run in receipts] == ["failed", "failed", "passed"], "preserve original statuses")
for run in ("run_r1", "run_r2"):
    receipt = receipts[run]
    need(receipt["GCP_used"] is False and receipt["CUDA_executed"] is False, "historical CPU scope")
    bad_compile = "drop_growth_grouped_compile" if run == "run_r1" else "wrong_vertical_cut_compile"
    need(receipt["error"] == "RuntimeError: " + bad_compile + " unexpected code", "historical compile failure")
    for row in receipt["commands"]:
        name = row["name"]
        need(row["closed"] and row == read(CAP / run / (name + ".command.json")), "closed command identity")
        intent = read(CAP / run / (name + ".intent.json"))
        need(all(row[key] == value for key, value in intent.items()), "command matches original intent")
        for suffix in ("stdout", "stderr"):
            need(sha(CAP / run / (name + "." + suffix)) == row[suffix + "_sha256"], "command output hash")
        if name == bad_compile:
            need(row["expected_code"] == 0 and row["exit_code"] == 1, "compile failure is not a killed mutant")
        else:
            need(row["exit_code"] == row["expected_code"], "command exit code")
        if name.endswith("_compile"):
            need(all(flag in row["argv"] for flag in ("-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-pthread", "-MMD")), "strict compile")
            if name != bad_compile:
                need(not (CAP / run / (name + ".stderr")).read_bytes(), "successful compile diagnostics")
    need(not (CAP / run / (bad_compile.removesuffix("_compile") + "_run.command.json")).exists(), "failed compile has no mutant run")
need("Is a directory" in (CAP / "run_r1/drop_growth_grouped_compile.stderr").read_text(), "r1 linker collision")
need("cannot convert" in (CAP / "run_r2/wrong_vertical_cut_compile.stderr").read_text() and "mhgp7::U192" in (CAP / "run_r2/wrong_vertical_cut_compile.stderr").read_text(), "r2 invalid mutant type")
need(len(receipts["run_r1"]["nominal"]) == 6 and receipts["run_r1"]["mutants"] == {}, "six nominal runs only in r1")
need(receipts["run_r2"]["nominal"] == {} and set(receipts["run_r2"]["mutants"]) == set(REASONS), "five causal r2 mutants")
for label in ("tower", "work", "front"):
    outputs = [(CAP / "run_r1" / (label + "_" + mode + "_run.stdout")).read_bytes() for mode in ("o2", "san")]
    need(outputs[0] == outputs[1] and outputs[0], "nominal paired nonvacuity " + label)
    for mode in ("o2", "san"):
        need(not (CAP / "run_r1" / (label + "_" + mode + "_run.stderr")).read_bytes(), "nominal diagnostics")
        need(re.fullmatch("[0-9a-f]{64}", receipts["run_r1"]["nominal"][label + "_" + mode]), "nominal binary pin")
    need(read(CAP / "run_r1" / (label + "_argument.command.json"))["exit_code"] == 2, "invalid argument code")
tower = (CAP / "run_r1/tower_o2_run.stdout").read_text()
need(all(value in tower for value in ("170320", "28", "45948")), "tower nonvacuity floor")
work = [json.loads(line) for line in (CAP / "run_r1/work_o2_run.stdout").read_text().splitlines()]
need(work[-1]["status"] == "passed" and work[-1]["clouds"] == 28 and work[-1]["grouped_lots"] == 176, "work grouped nonvacuity")
for name, reason in REASONS.items():
    row = receipts["run_r2"]["mutants"][name]
    need(row["header_sha256"] == sha(BASE / "mutants" / name / "full_ball_tower.hpp"), "causal mutant header")
    need(re.fullmatch("[0-9a-f]{64}", row["binary_sha256"]), "mutant binary pin")
    need(read(CAP / "run_r2" / (name + "_compile.command.json"))["exit_code"] == 0, "mutant compiled")
    need(read(CAP / "run_r2" / (name + "_run.command.json"))["exit_code"] == 1, "mutant rejected at runtime")
    need((CAP / "run_r2" / (name + "_run.stderr")).read_text() == reason, "causal rejection reason")
retry = receipts["run_r3_vertical"]
need(retry["frozen_source_inventory"] == sha(CAP / "run_r2/sources_before.json"), "r3 source provenance")
need(retry["live_sources_consumed"] is False and retry["GCP_used"] is False, "r3 frozen-only CPU scope")
need(retry["mutant_header_sha256"] == sha(BASE / "mutants/wrong_vertical_cut_typed_retry/full_ball_tower.hpp"), "r3 typed header")
need(len(retry["commands"]) == 2, "r3 compile then run")
for name, row, code in zip(("compile", "run"), retry["commands"], (0, 1)):
    need(row["exit_code"] == row["expected"] == code, "r3 runtime causal rejection")
    for suffix in ("stdout", "stderr"):
        need(sha(CAP / "run_r3_vertical" / (name + "." + suffix)) == row[suffix + "_sha256"], "r3 output hash")
need((CAP / "run_r3_vertical/run.stderr").read_text() == "FAIL [pair/variant0] vertical.all_subfacets_at_same_open_or_closed_cut\n", "r3 exact vertical failure")
need(not list((CAP / "run_r3_vertical").glob("*.d")), "do not invent r3 compiler dependency evidence")
need(not any((CAP / run / "sources_after.json").exists() for run in receipts), "do not invent historical after-run pins")
print("grouped_lot_receipt=passed historical_statuses=failed,failed,passed nominal_runs=6 invalid_args=3 causal_mutants=6 compile_failures_not_kills=2 GCP=not_used")
