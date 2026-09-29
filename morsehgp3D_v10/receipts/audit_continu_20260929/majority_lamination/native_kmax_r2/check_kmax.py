"""Audit-only majority head on exports of the pinned native FULL engine.

Writes a NEW output directory; never run on an existing closed capture.
Uniform rational weights, K=2, six collinear sites. This is not a native head,
an EOM experiment, a statistical benchmark or a performance qualification.
"""
from fractions import Fraction as Q
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import time

NONE = 2**32 - 1
EXPECTED_PROBE_SHA = "1c2ce0d73c5871783f2fc3e04a3940a53250af7a0d60a1689ec8a4c3a9787a85"


def check(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def order2(data):
    return next(order for order in data["orders"] if order["k"] == 2)


def levels(data):
    return [Q(int(num), int(den)) for num, den in data["levels"]]


def node_date(data, node):
    rank = order2(data)["rank"][node]
    return Q(0) if rank == 0 else levels(data)[rank - 1]


def ancestor(data, node, beta):
    order = order2(data)
    check(node != NONE, "missing component for a covering ball")
    check(node_date(data, node) <= beta, "component not yet born")
    while order["parent"][node] != NONE:
        parent = order["parent"][node]
        if node_date(data, parent) > beta:
            break
        node = parent
    return node


def ball_id(data, b):
    ball = data["balls"][b]
    return (str(levels(data)[ball["rank"]]), ball["q"], ball["p"],
            tuple(ball["I"]), tuple(ball["U"]))


def forest_signature(data):
    """Compare the FULL K2 trees independently of ranks and array node IDs."""
    order = order2(data)
    children = [[] for _ in order["parent"]]
    roots = []
    for node, parent in enumerate(order["parent"]):
        (roots if parent == NONE else children[parent]).append(node)

    def signature(node):
        birth = order["birth"][node]
        label = None if birth == NONE else ball_id(data, birth)
        return (str(node_date(data, node)), label,
                tuple(sorted((signature(c) for c in children[node]), key=repr)))
    return tuple(sorted((signature(root) for root in roots), key=repr))


def atoms(data, strong):
    result = [[] for _ in data["points"]]
    order = order2(data)
    for b, ball in enumerate(data["balls"]):
        if ball["p"] + ball["u"] < 2:
            continue
        if strong and ball["p"] + ball["q"] > 2:
            continue
        beta = levels(data)[ball["rank"]]
        node = ancestor(data, order["ball_node"][b], beta)
        for point in ball["I"] + ball["U"]:
            result[point].append((b, node, beta))
    check(all(result), "a point has no witness")
    return result


def majority_partition(data, witnesses, beta):
    labels = []
    for point, rows in enumerate(witnesses):
        masses = {}
        for _ball, node, birth in rows:
            if birth <= beta:
                component = ancestor(data, node, beta)
                masses[component] = masses.get(component, 0) + 1
        owners = [c for c, mass in masses.items() if 2 * mass > len(rows)]
        check(len(owners) <= 1, "two majority owners")
        labels.append(("component", owners[0]) if owners else ("singleton", point))
    return labels


def examine(data, strong):
    witnesses = atoms(data, strong)
    dates = sorted({Q(0)} | {node_date(data, i) for i in range(len(order2(data)["rank"]))}
                   | {birth for rows in witnesses for _b, _n, birth in rows})
    cuts = sorted(set(dates) | {(a+b)/2 for a, b in zip(dates, dates[1:])}
                  | {dates[-1] + 1})
    previous = None
    first_pair = None
    nesting_checks = 0
    for beta in cuts:
        labels = majority_partition(data, witnesses, beta)
        if previous is not None:
            for x in range(len(labels)):
                for y in range(x + 1, len(labels)):
                    check(previous[x] != previous[y] or labels[x] == labels[y],
                          "point partition split")
                    nesting_checks += 1
        if first_pair is None and labels[0] == labels[2]:
            first_pair = beta
        previous = labels
    check(len(set(previous)) == 1, "points do not join at the final root")
    return {
        "witness_counts": [len(rows) for rows in witnesses],
        "first_x0_x2_merge_beta": str(first_pair),
        "nesting_checks": nesting_checks,
        "witness_ids": [[ball_id(data, b) for b, _n, _t in rows] for rows in witnesses],
    }


def main():
    check(len(sys.argv) == 3, "usage: check_kmax.py PINNED_PROBE NEW_OUTPUT_DIR")
    probe = Path(sys.argv[1]).resolve()
    dest = Path(sys.argv[2]).resolve()
    check(not dest.exists(), "closed output directory must not be overwritten")
    fixture = Path(__file__).with_name("line6.txt")
    before = {str(p): sha(p) for p in (probe, fixture, Path(__file__).resolve())}
    check(before[str(probe)] == EXPECTED_PROBE_SHA, "probe version mismatch")
    dest.mkdir(parents=True)
    data = {}
    commands = []
    for kmax in (2, 4):
        for workers in (1, 2):
            cmd = [str(probe), str(fixture), str(kmax), str(workers), "1"]
            start = time.monotonic()
            result = subprocess.run(cmd, capture_output=True, timeout=10, check=False)
            stem = f"kmax{kmax}_w{workers}"
            (dest / f"{stem}.stdout").write_bytes(result.stdout)
            (dest / f"{stem}.stderr").write_bytes(result.stderr)
            commands.append({"argv": cmd, "exit_code": result.returncode,
                             "wall_seconds": time.monotonic() - start})
            check(result.returncode == 0 and not result.stderr, "native call failed")
            data[kmax, workers] = json.loads(result.stdout)
        check(data[kmax, 1] == data[kmax, 2], "worker output differs")
    check(forest_signature(data[2, 1]) == forest_signature(data[4, 1]),
          "underlying FULL K2 trees differ")
    results = {str(k): {"all": examine(data[k, 1], False),
                        "strong": examine(data[k, 1], True)} for k in (2, 4)}
    check(results["2"]["all"]["first_x0_x2_merge_beta"] == "1", "unexpected Kmax2 date")
    check(results["4"]["all"]["first_x0_x2_merge_beta"] == "9/4", "unexpected Kmax4 date")
    for k in ("2", "4"):
        check(results[k]["strong"]["first_x0_x2_merge_beta"] == "1", "strong date differs")
    check(results["2"]["strong"]["witness_ids"] == results["4"]["strong"]["witness_ids"],
          "strong universe depends on Kmax")
    after = {str(p): sha(p) for p in (probe, fixture, Path(__file__).resolve())}
    check(before == after, "source or binary changed")
    receipt = {"status": "PASS", "scope": "native_FULL_K2_export_with_audit_only_unit_mass_head",
               "profile": "six_collinear_u18_sites", "native_calls": len(commands),
               "commands": commands, "hashes_before": before, "hashes_after": after,
               "underlying_FULL_K2_same": True, "results": results,
               "engine_modified": False, "native_head_implemented": False,
               "statistical_or_GPU_qualification": False, "GCP_used": False}
    (dest / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    for k in results:
        for mode in results[k]:
            del results[k][mode]["witness_ids"]
    print(json.dumps({"status": "PASS", "native_calls": len(commands),
                      "underlying_FULL_K2_same": True, "results": results}, sort_keys=True))


if __name__ == "__main__":
    main()
