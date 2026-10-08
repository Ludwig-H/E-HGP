#!/usr/bin/env python3
"""CPU census reduction model: metadata/Python only; never run the engine."""
import hashlib
import itertools
import json
from pathlib import Path
import subprocess
import sys


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def masks_to_result(inside, shell, fault, visited, theta, m):
    # Existing precedence: any prefix fault before census_tests or rejection.
    if fault:
        verdict, tests = "fault", 0
    elif inside.bit_count() > theta:
        verdict, tests = "rejected", visited
    else:
        verdict, tests = "complete", m
    return dict(I=inside, U=shell, fault=fault, visited=visited,
                verdict=verdict, census_tests_delta=tests)


def reference(cats, theta, width):
    """Host lanes initialized through N, then three ballots, as pinned code."""
    inside, shell, fault = [False] * width, [False] * width, [False] * width
    seen, visited = 0, 0
    for rank in range(width):
        if rank < len(cats) and seen <= theta:
            category = cats[rank]
            visited += 1
            inside[rank], shell[rank], fault[rank] = category == "I", category == "U", category == "F"
            seen += inside[rank]
    ballots = [sum(1 << i for i, flag in enumerate(lanes) if flag) for lanes in (inside, shell, fault)]
    return masks_to_result(*ballots, visited, theta, len(cats))


def reduced(cats, theta, width, mutation=None):
    """Proposed abstract fold: same classified prefix, including after faults."""
    seen = visited = 0
    word_bits, word_count = (32, 1) if width == 32 else (64, 4)
    words = [[0] * word_count for _ in range(3)]
    limit = theta - 1 if mutation == "cutoff_one_early" else theta
    sequence = cats[::-1] if mutation == "reverse_ranks" else cats
    for rank, category in enumerate(sequence):
        if seen > limit and mutation != "read_after_cutoff":
            break
        if mutation == "early_success" and seen == theta:
            break
        index = rank % 32 if mutation == "truncate_mask32" else rank
        word, bit = index // word_bits, 1 << (index % word_bits)
        visited += 1
        if category == "I":
            words[0][word] |= bit
            seen += 1
        elif category == "U":
            words[1][word] |= bit
        elif category == "F":
            words[2][word] |= bit
            if mutation == "return_at_first_fault":
                break
    inside, shell, fault = [sum(w << (word_bits * i) for i, w in enumerate(mask)) for mask in words]
    if mutation == "padding_is_shell":
        shell |= ((1 << width) - 1) ^ ((1 << len(cats)) - 1)
    if mutation == "rejection_before_fault" and inside.bit_count() > theta:
        fault = 0
    return masks_to_result(inside, shell, fault, visited, theta, len(cats))


def check_case(cats, theta, width):
    old, new = reference(cats, theta, width), reduced(cats, theta, width)
    need(old == new, "fold differs")
    # Independent set/prefix specification, not just two matching implementations.
    interiors = [i for i, c in enumerate(cats) if c == "I"]
    end = interiors[theta] + 1 if len(interiors) > theta else len(cats)
    for key, category in (("I", "I"), ("U", "U"), ("fault", "F")):
        need(new[key] == sum(1 << i for i in range(end) if cats[i] == category), "prefix mask")
    need(new["visited"] == end, "classified prefix")
    if new["verdict"] == "complete":
        need(end == len(cats), "incomplete admitted shell")


def model():
    exhaustive = 0
    # All 4-category words through six sites; each distinct cutoff through m.
    # theta >= m cannot stop, so theta=m represents all larger thresholds here.
    for m in range(7):
        for cats in itertools.product("IOUF", repeat=m):
            for theta in range(m + 1):
                for width in (32, 256):
                    check_case(cats, theta, width)
                    exhaustive += 1
    directed = 0
    parameters = [(k, q, k + 1 - q) for k in range(1, 13) for q in (2, 3, 4) if q <= k + 1]
    for width in (32, 256):
        for k, q, theta in parameters:
            for m in sorted({q, min(width, q + theta + 2), width}):
                # Every rank individually carries each category, including 31/32, 63/64, 255.
                for rank in range(m):
                    for category in "IUF":
                        cats = "O" * rank + category + "O" * (m - rank - 1)
                        check_case(cats, theta, width)
                        directed += 1
            suffix = width - theta - 1
            for cats in ("I" * (theta + 1) + "F" * suffix,
                         "F" + "I" * (theta + 1) + "U" * (suffix - 1),
                         "I" * theta + "U" * (width - theta)):
                check_case(cats, theta, width)
                directed += 1
    witnesses = {
        "cutoff_one_early": ("UI", 0, 32),
        "read_after_cutoff": ("IF", 0, 32),
        "rejection_before_fault": ("FI", 0, 32),
        "return_at_first_fault": ("FUO", 2, 32),
        "early_success": ("IU", 1, 32),
        "truncate_mask32": ("O" * 255 + "U", 0, 256),
        "padding_is_shell": ("O", 0, 32),
        "reverse_ranks": ("IF", 0, 32),
    }
    killed = []
    for mutation, (cats, theta, width) in witnesses.items():
        need(reference(cats, theta, width) != reduced(cats, theta, width, mutation), "surviving mutant " + mutation)
        killed.append(mutation)
    # Pure vote helper cannot move the external canonical/shell-capacity checks.
    shell = reduced("U" * 65, 4, 256)
    need(shell["verdict"] == "complete" and shell["U"].bit_count() == 65, "premature shell limit")
    return dict(exhaustive_cases=exhaustive, directed_cases=directed, valid_K_q_pairs=len(parameters),
                widths=[32, 256], alphabet="I interior / O exterior / U shell / F arithmetic fault",
                killed_model_mutants=killed, shell_65_preserved_for_unchanged_downstream=True)


def main():
    need(len(sys.argv) == 2, "usage: check.py DEPOT_GIT")
    repo, here = Path(sys.argv[1]).resolve(), Path(__file__).resolve().parent
    cap = json.loads((here / "capture.json").read_text())
    for commit in cap["commits"].values():
        for path, sha in cap["sources_equal_at_three_pins"].items():
            data = subprocess.check_output(["git", "show", commit + ":" + path], cwd=repo)
            need(hashlib.sha256(data).hexdigest() == sha, "source pin " + path)
    report = cap["fulln_report"]
    data = subprocess.check_output(["git", "show", report["git"] + ":" + report["path"]], cwd=repo)
    need(hashlib.sha256(data).hexdigest() == report["sha256"], "FULLN public report")
    row = json.loads(data)["statistiques"]["k5_cpu"]["ng00"]
    need(dict(wall=row["mediane_ns"], C=row["etapes_ns"]["C"], count=row["c_ns"]["feuilles"],
              fill=row["c_ns"]["emission"]) == report["ng00_cpu_k5_w48_published_ns"], "published stage reference")
    out = dict(schema="cpu-census-reduction-result-v1", source_files=len(cap["sources_equal_at_three_pins"]),
               immutable_commits=cap["commits"], model=model(), native_runs=0,
               geometry_qualified=False, speedup_qualified=False, engine_patch=False)
    text = json.dumps(out, ensure_ascii=False, indent=2) + "\n"
    saved = here / "results.json"
    if saved.exists():
        need(saved.read_text() == text, "result changed")
    print(text, end="")


if __name__ == "__main__":
    main()
