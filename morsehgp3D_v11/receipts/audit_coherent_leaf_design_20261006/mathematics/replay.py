#!/usr/bin/env python3
"""First-event helpers and discriminating bounded traces, no product execution."""
import argparse
import hashlib
import itertools
import json
from pathlib import Path
import subprocess

PIN = "ee3eabe5e7aae4d95515f63935e8f9a84adbd468"
CHECKS = 0

def require(condition, message):
    global CHECKS
    if not condition:
        raise RuntimeError(message)
    CHECKS += 1

def first_bit(mask, default):
    return (mask & -mask).bit_length() - 1 if mask else default

def select_bit(mask, ordinal, default):
    """One-based ordinal, returning the site index of that set bit."""
    if ordinal < 1:
        raise ValueError("ordinal must be positive")
    for _ in range(ordinal - 1):
        if not mask:
            return default
        mask &= mask - 1
    return first_bit(mask, default)

def census_serial(states, threshold):
    """I=strict inside, C=contact, O=outside, U=uncertified guarded side."""
    interior, shell = [], []
    for i, state in enumerate(states):
        if state == "U":
            return dict(status="unresolved", census_tests=i+1, stop=i)
        if state == "I":
            if len(interior) == threshold:
                return dict(status="rejected_prefix", census_tests=i+1, stop=i)
            interior.append(i)
        elif state == "C":
            shell.append(i)
    return dict(status="complete", census_tests=len(states), stop=None, interior=interior, shell=shell)

def census_masks(states, threshold):
    size = len(states)
    inside = sum(1 << i for i, state in enumerate(states) if state == "I")
    unknown = sum(1 << i for i, state in enumerate(states) if state == "U")
    contact = sum(1 << i for i, state in enumerate(states) if state == "C")
    reject = select_bit(inside, threshold+1, size)
    refuse = first_bit(unknown, size)
    stop = min(reject, refuse)
    if stop < size:
        return dict(status="unresolved" if refuse < reject else "rejected_prefix", census_tests=stop+1, stop=stop)
    return dict(status="complete", census_tests=size, stop=None,
                interior=[i for i in range(size) if inside >> i & 1],
                shell=[i for i in range(size) if contact >> i & 1])

def guarded_states(raw, generators, inside, outside):
    """Matches generator -> inside mask -> outside mask -> side precedence."""
    if inside & outside:
        return None  # leaf invariant refusal before scan, even on generator sites
    return ["C" if i in generators else "I" if inside >> i & 1 else
            "O" if outside >> i & 1 else state for i,state in enumerate(raw)]

def canonical_serial(candidates):
    # Logical order: all pairs, then all triangles, then all tetrahedra, lexicographic within arity.
    for key, event in sorted(candidates, key=lambda item: (len(item[0]), item[0])):
        if event != "N":
            return dict(event=event, candidate=list(key))
    return dict(event="U", candidate=None)  # catalogue invariant: no support found

def canonical_masks(candidates):
    ordered = sorted(candidates, key=lambda item: (len(item[0]), item[0]))
    success = sum(1 << rank for rank, (_, event) in enumerate(ordered) if event == "S")
    unknown = sum(1 << rank for rank, (_, event) in enumerate(ordered) if event == "U")
    rank = first_bit(success | unknown, len(ordered))
    return dict(event=ordered[rank][1], candidate=list(ordered[rank][0])) if rank < len(ordered) else dict(event="U", candidate=None)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", default="/workspaces/E-HGP")
    args = parser.parse_args()
    manifest = json.loads(Path(__file__).with_name("sources.json").read_text())
    require(manifest["pin"] == PIN, "pin changed")
    for path, info in manifest["files"].items():
        data = subprocess.check_output(["git","-C",args.repo,"show",PIN+":"+path])
        require(hashlib.sha256(data).hexdigest() == info["sha256"], "source changed: "+path)

    traces = [
        ("threshold_zero", "OICU", 0),
        ("unknown_before_extra_inside", "CUIOI", 1),
        ("unknown_after_extra_inside", "CIOIU", 1),
        ("exactly_threshold_then_unknown", "CIOU", 1),
        ("exactly_threshold_full", "CIOCO", 1),
        ("not_first_contact", "OCICOI", 1),
        ("first_unknown", "UICI", 1),
        ("all_shell", "CCCC", 0),
        ("full_warp_last_unknown_ignored", "I"*17+"C"*14+"U", 16),
        ("full_warp_unknown_before_limit", "I"*16+"U"+"I"*15, 16),
    ]
    results = []
    for label, states, threshold in traces:
        serial = census_serial(states, threshold)
        parallel = census_masks(states, threshold)
        require(serial == parallel, "census first-event mismatch: "+label)
        results.append(dict(label=label,states=states,threshold=threshold,expected=serial))
    require(census_masks("CIOU",1)["status"] == "unresolved", "theta-th interior is not the rejection")
    require(census_masks("CIOIU",1)["census_tests"] == 4, "theta+1 interior is counted")
    require(census_masks("CUIOI",1)["status"] == "unresolved", "later known interiors cannot hide earlier unknown")
    # Raw uncertified side at a generator or dominated site must not become a refusal.
    guarded = guarded_states("UUUUU", {0,3}, 1<<1, 1<<2)
    require(guarded == ["C","I","O","C","U"], "precedence of guarded census predicates")
    require(census_masks(guarded,0)["status"] == "rejected_prefix", "unvisited raw unknown ignored")
    require(guarded_states("UU",{0},1,1) is None, "overlap invariant precedes scan")

    cases = [
        ("unknown_before_success", [((0,1,2),"U"),((0,1,3),"S")]),
        ("unknown_after_success", [((0,1,2),"S"),((0,1,3),"U")]),
        ("pair_phase_before_triangle", [((0,1,2),"S"),((3,4),"S")]),
        ("triangle_phase_before_tetra", [((0,1,2,3),"S"),((1,2,3),"U")]),
        ("lex_not_colex", [((0,1,4),"S"),((0,2,3),"S")]),
        ("all_candidates_miss", [((0,1),"N"),((0,1,2),"N"),((0,1,2,3),"N")]),
    ]
    canonical_results = []
    for label,candidates in cases:
        serial = canonical_serial(candidates)
        require(serial == canonical_masks(candidates), "canonical first-event mismatch: "+label)
        canonical_results.append(dict(label=label,expected=serial))
    # J2 combinadic rank is colexicographic, whereas canonical source loops are lexicographic.
    j2_rank = lambda i,j,k: k*(k-1)*(k-2)//6+j*(j-1)//2+i
    require((0,1,4) < (0,2,3) and j2_rank(0,1,4) == 4 and j2_rank(0,2,3) == 2, "lex/colex witness")
    require(canonical_masks(cases[0][1])["event"] == "U", "minimum success alone is unsound")
    require(canonical_masks(cases[1][1])["event"] == "S", "unknown after success must be ignored")
    # Candidate-internal guard/early return must suppress subsequent uncertainty.
    triangle_event = lambda acute,cert,plane_zero: "N" if not acute else "U" if not cert else "S" if plane_zero else "N"
    tetra_event = lambda faces: next(("U" if x == "U" else "N" for x in faces if x != "PASS"),"S")
    require(triangle_event(False,False,True) == "N", "acute guard must precede uncertified orientation")
    require(tetra_event(["MISS","U","PASS","PASS"]) == "N", "unvisited face refusal suppressed")
    require(tetra_event(["U","MISS","PASS","PASS"]) == "U", "early face refusal preserved")
    # Only one logical J2 event per uniform warp predicate, not 32 repeated cache test-and-sets.
    seen = False
    evaluations = hits = 0
    for _ in range(32):
        if seen: hits += 1
        else: evaluations += 1
        seen = True
    require((evaluations,hits) == (1,31), "redundant atomicOr would fabricate hits")

    print(json.dumps(dict(pin=PIN,status="PASS_portable_event_model_only",checks=CHECKS,
        native_executed=False,census_traces=results,canonical_traces=canonical_results,
        census_formula="theta=K+1-q; t=select(theta+1,I); u=first(U); e=min(t,u,m); visited=min(e+1,m)",
        canonical_formula="first event in phase/lex order among success or refusal; miss creates no event",
        limits="Symbolic predicate-event traces, including injected uncertainty, not geometric fixtures, native tests, or CUDA concurrency simulation. Speculative evaluation is admissible only for guarded, numerically safe paths."),sort_keys=True,indent=2))

if __name__ == "__main__":
    main()
