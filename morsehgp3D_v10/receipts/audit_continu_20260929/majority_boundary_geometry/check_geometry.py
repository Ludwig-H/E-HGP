"""Audit-only, exact small FULL-export experiment; not a production head.

Eight fixed four-site fixtures. Strong K2 witnesses, fixed masses, strict
majority. Uniform vs inverse squared-radius weights; all arithmetic Fraction.
No engine build, no benchmark seed, no score optimization, no GCP.
"""
from fractions import Fraction as Q
from pathlib import Path
import hashlib
import importlib.util
import json
import subprocess
import sys
import time

NONE = 2**32 - 1
PROBE_SHA = "1c2ce0d73c5871783f2fc3e04a3940a53250af7a0d60a1689ec8a4c3a9787a85"


def check(value, message):
    if not value:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def native_helpers(data):
    order = next(o for o in data["orders"] if o["k"] == 2)
    levels = [Q(int(a), int(b)) for a, b in data["levels"]]

    def date(node):
        rank = order["rank"][node]
        return Q(0) if rank == 0 else levels[rank - 1]

    def ancestor(node, beta):
        check(node != NONE and date(node) <= beta, "inactive/missing anchor")
        while order["parent"][node] != NONE:
            parent = order["parent"][node]
            if date(parent) > beta:
                break
            node = parent
        return node

    witnesses = [[] for _ in data["points"]]
    for b, ball in enumerate(data["balls"]):
        if ball["p"] + ball["u"] < 2 or ball["p"] + ball["q"] > 2:
            continue
        beta = levels[ball["rank"]]
        check(beta > 0, "unexpected zero K2 witness")
        node = ancestor(order["ball_node"][b], beta)
        for x in ball["I"] + ball["U"]:
            witnesses[x].append((b, node, beta))
    return order, levels, date, ancestor, witnesses


def partition(data, witnesses, ancestor, beta, inverse):
    result = []
    for x, atoms in enumerate(witnesses):
        total = sum((1 / t if inverse else Q(1) for _, _, t in atoms), Q(0))
        check(total > 0, "point without finite witnesses")
        masses = {}
        for _b, node, birth in atoms:
            if birth <= beta:
                c = ancestor(node, beta)
                masses[c] = masses.get(c, Q(0)) + (1 / birth if inverse else Q(1))
        owners = [c for c, mass in masses.items() if 2 * mass > total]
        check(len(owners) <= 1, "majority not exclusive")
        result.append(("component", owners[0]) if owners else ("singleton", x))
    return result


def point_blocks(labels):
    groups = {}
    for x, label in enumerate(labels):
        groups.setdefault(label, []).append(x)
    return sorted(groups.values())


def analyze(data, ref):
    P = [tuple(p) for p in data["points"]]
    order, _levels, date, ancestor, witnesses = native_helpers(data)
    check(sorted(len(w) for w in witnesses) == [1, 1, 2, 2], "unexpected witness incidence")
    # Independent rational geometry, not reconstruction from native ball IDs.
    exact = [b for b in ref.critical_balls(P) if b.p + b.m >= 2 and b.p + b.qmin <= 2]
    exact_keys = sorted((str(b.level), tuple(sorted(b.I)), tuple(sorted(b.U))) for b in exact)
    native_keys = sorted((str(t), tuple(data["balls"][b]["I"]), tuple(data["balls"][b]["U"]))
                         for b in {b for rows in witnesses for b, _, _ in rows}
                         for t in [Q(*map(int, data["levels"][data["balls"][b]["rank"]]))])
    check(exact_keys == native_keys and len(exact) == 3, "witness geometry mismatch")
    gamma_levels, gamma_closed, _ = ref.gamma_cuts(P, 2)
    check(max(p[0] for p in P) - min(p[0] for p in P) >= 5, "bad fixture spacing")
    expected = sorted([sorted(i for i, p in enumerate(P) if p[0] <= 1),
                       sorted(i for i, p in enumerate(P) if p[0] > 1)])
    local_beta = max(ref.d2(P[g[0]], P[g[1]]) / 4 for g in expected)
    # Every point has its paired nearest neighbour. At this later diagnostic
    # cut each is core, but FULL has not yet suffered a cross-group fusion.
    core_beta = max(ref.d2(P[g[0]], P[g[1]]) for g in expected)
    first_fusion = min(date(i) for i, born in enumerate(order["birth"]) if born == NONE)
    check(core_beta < first_fusion, "core comparison is after a parasite fusion")
    all_dates = sorted({Q(0), local_beta, core_beta} | set(gamma_levels)
                       | {date(i) for i in range(len(order["rank"]))}
                       | {t for rows in witnesses for _b, _n, t in rows})
    cuts = sorted(set(all_dates) | {(a + b) / 2 for a, b in zip(all_dates, all_dates[1:])}
                  | {all_dates[-1] + 1})
    modes = {}
    comparisons = 0
    for inverse in (False, True):
        previous = None
        first_pairs = [None, None]
        pre_fusion_recovered = set()
        trace = []
        for beta in cuts:
            labels = partition(data, witnesses, ancestor, beta, inverse)
            if previous is not None:
                for x in range(len(P)):
                    for y in range(x + 1, len(P)):
                        check(previous[x] != previous[y] or labels[x] == labels[y], "point split")
                        comparisons += 1
            blocks = point_blocks(labels)
            for j, group in enumerate(expected):
                if group in blocks:
                    if first_pairs[j] is None:
                        first_pairs[j] = beta
                    if beta < first_fusion:
                        pre_fusion_recovered.add(j)
            trace.append({"beta": str(beta), "blocks": blocks})
            previous = labels
        check(len(set(previous)) == 1, "final root incomplete")
        before = point_blocks(partition(data, witnesses, ancestor, local_beta, inverse))
        at_core = point_blocks(partition(data, witnesses, ancestor, core_beta, inverse))
        check(before == (expected if inverse else [[i] for i in range(len(P))]), "early result mismatch")
        check(at_core == before, "unexpected recovery before core comparison")
        check(len(pre_fusion_recovered) == (2 if inverse else 0), "recall difference absent")
        modes["inverse_beta" if inverse else "uniform"] = {
            "weights": [[str(1 / t if inverse else Q(1)) for _b, _n, t in rows] for rows in witnesses],
            "first_pair_beta": [None if t is None else str(t) for t in first_pairs],
            "blocks_at_local_beta": before, "blocks_at_core_beta": at_core,
            "recovered_groups_before_first_FULL_fusion": len(pre_fusion_recovered),
            "cut_trace": trace,
        }
    # Rational Gamma independently confirms both covered groups at the early cut.
    gi = max(i for i, t in enumerate(gamma_levels) if t <= local_beta)
    gamma_groups = sorted(sorted(g) for g in gamma_closed[gi])
    check(gamma_groups == expected, "FULL coverage was not the expected two groups")
    return {"points": P, "expected_groups": expected, "native_witness_keys": native_keys,
            "witness_counts": [len(w) for w in witnesses], "gamma_at_local_beta": gamma_groups,
            "local_beta": str(local_beta), "core_beta": str(core_beta),
            "first_FULL_fusion_beta": str(first_fusion), "nesting_pair_checks": comparisons,
            "affine_dimension": ref.rank([ref.sub(p, P[0]) for p in P[1:]]), "modes": modes}


def main():
    check(len(sys.argv) == 4, "usage: check_geometry.py PINNED_PROBE FROZEN_REFERENCE NEW_OUT")
    script = Path(__file__).resolve()
    probe, ref_path, dest = (Path(p).resolve() for p in sys.argv[1:])
    fixtures = sorted(script.with_name("fixtures").glob("*.txt"))
    check(len(fixtures) == 8 and not dest.exists(), "wrong fixtures or existing capture")
    inputs = [probe, script, ref_path] + fixtures
    before = {str(p): sha(p) for p in inputs}
    check(before[str(probe)] == PROBE_SHA, "native exporter version mismatch")
    spec = importlib.util.spec_from_file_location("frozen_hgp10_reference", ref_path)
    ref = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(ref)
    dest.mkdir(parents=True)
    rows, commands = {}, []
    for fixture in fixtures:
        exports = []
        for workers in (1, 2):
            argv = [str(probe), str(fixture), "2", str(workers), "1"]
            start = time.monotonic()
            result = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=10)
            prefix = fixture.stem + "_w" + str(workers)
            (dest / (prefix + ".stdout")).write_bytes(result.stdout)
            (dest / (prefix + ".stderr")).write_bytes(result.stderr)
            commands.append({"argv": argv, "exit_code": result.returncode,
                             "wall_seconds": time.monotonic() - start,
                             "stdout_sha256": hashlib.sha256(result.stdout).hexdigest()})
            check(result.returncode == 0 and not result.stderr, "native exporter failed")
            exports.append(json.loads(result.stdout))
        check(exports[0] == exports[1], "worker result differs")
        rows[fixture.stem] = analyze(exports[0], ref)
    check(sum(row["affine_dimension"] == 3 for row in rows.values()) == 4, "no full-dimensional cases")
    after = {str(p): sha(p) for p in inputs}
    check(before == after, "source or binary changed during capture")
    receipt = {"status": "PASS_COUNTEREXAMPLE_REPRODUCED", "scope": "four_site_K2_audit_only_majority_head",
               "native_engine_source_commit": "6206d1d11", "GCP_used": False, "engine_modified": False,
               "production_head_implemented": False, "statistical_benchmark": False,
               "native_commands": commands, "hashes_before": before, "hashes_after": after,
               "rows": rows, "nesting_pair_checks": sum(r["nesting_pair_checks"] for r in rows.values())}
    (dest / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({"status": receipt["status"], "native_calls": len(commands), "fixtures": len(rows),
                      "full_dimensional": 4, "nesting_pair_checks": receipt["nesting_pair_checks"],
                      "uniform_recovered_groups_before_first_fusion": 0,
                      "inverse_beta_recovered_groups_before_first_fusion": 16}, sort_keys=True))


if __name__ == "__main__":
    main()
