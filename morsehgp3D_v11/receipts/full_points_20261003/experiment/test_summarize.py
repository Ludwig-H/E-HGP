#!/usr/bin/env python3
"""Pure exact/JSON regression; no native, sklearn fit, real data, or GCP."""
import ast
import copy
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import sys
import tempfile

sys.dont_write_bytecode = True
import native_gate
from qualified import HierarchyScorer, analyse, load
from summarize import collect, qualified_m3_identity
from test_qualified import Oracle

HERE = Path(__file__).resolve().parent
CHECKS = 0


def check(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise RuntimeError(message)


def rejected(action, message):
    try:
        action()
    except ValueError:
        check(True, message)
    else:
        check(False, message)


def actual_compact():
    # Run the runner's actual compaction body without importing numpy or its
    # campaign entrypoint. No substitute hashing convention is introduced.
    path = HERE / "run_experiment.py"
    module = ast.parse(path.read_text(), filename=str(path))
    body = [node for node in module.body if isinstance(node, ast.FunctionDef) and node.name == "compact"]
    check(len(body) == 1, "one actual runner compact implementation")
    environment = dict(json=json, hashlib=hashlib)
    exec(compile(ast.Module(body=body, type_ignores=[]), str(path), "exec"), environment)
    return environment["compact"]


def compact_projection(value, compact):
    result = copy.deepcopy(value)
    for order in result["orders"].values():
        for name, body in list(order.items()):
            order[name] = {m: compact(item) for m, item in body.items()} if name == "qualified" else compact(body)
    return result


def main():
    compact = actual_compact()
    accepted = []
    for fixture in native_gate.cases():
        if len(set(fixture.points)) != len(fixture.points) or fixture.kmax < 3:
            continue
        oracle = Oracle(fixture.points, fixture.kmax)
        data = load(oracle.encode())
        try:
            result = compact_projection(analyse(data, [i % 3 for i in range(data.sites)],
                                               orders=[2, 3], thresholds=[3]), compact)
            identity = qualified_m3_identity(result, fixture.name)
            check(identity["status"] == "pass", "canonical identity on exact Gamma encoder " + fixture.name)
            accepted.append(fixture.name)
        finally:
            data.close()
    check(len(accepted) == 29, "all29unitfixtures with orders2/3 including K10 fixture")

    # Atomic numbering is independent of union/activation order and DSU roots.
    scorers = [HierarchyScorer([0, 0, 1, 1]) for _ in range(2)]
    for scorer, activation, pairs in ((scorers[0], [0, 1, 2, 3], [(0, 1), (2, 3)]),
                                       (scorers[1], [3, 2, 1, 0], [(3, 2), (1, 0)])):
        for site in activation:
            scorer.activate(site, Fraction(1))
        for a, b in pairs:
            scorer.union(a, b)
        scorer.observe(Fraction(1))
        scorer.union(0, 2)
        scorer.observe(Fraction(4))
    first, second = map(lambda scorer: compact(scorer.finish()), scorers)
    check(first["tree_sha256"] == second["tree_sha256"] and
          first["entry_dates_sha256"] == second["entry_dates_sha256"], "canonical atomic numbering")

    baseline = copy.deepcopy(result)
    for field in ("tree_sha256", "entry_dates_sha256"):
        mutant = copy.deepcopy(baseline)
        current = mutant["orders"]["3"]["qualified"]["3"][field]
        mutant["orders"]["3"]["qualified"]["3"][field] = ("0" if current[0] != "0" else "1") + current[1:]
        rejected(lambda: qualified_m3_identity(mutant, field), "unequal " + field + " rejected")
        for malformed in (None, 17, "", "z" * 64, "A" * 64):
            mutant = copy.deepcopy(baseline)
            for k in ("2", "3"):
                mutant["orders"][k]["qualified"]["3"][field] = malformed
            rejected(lambda: qualified_m3_identity(mutant, field), "matching malformed digest rejected")
        mutant = copy.deepcopy(baseline)
        del mutant["orders"]["2"]["qualified"]["3"][field]
        rejected(lambda: qualified_m3_identity(mutant, field), "missing digest rejected")
    for field in ("3", "2"):
        mutant = copy.deepcopy(baseline)
        del mutant["orders"][field]
        rejected(lambda: qualified_m3_identity(mutant, field), "missing order rejected")
    mutant = copy.deepcopy(baseline)
    mutant["height_units"] = "radius"
    rejected(lambda: qualified_m3_identity(mutant, "units"), "incompatible units rejected")
    mutant = copy.deepcopy(baseline)
    mutant["orders"]["3"]["qualified"]["3"]["work"] = {"different": 999}
    check(qualified_m3_identity(mutant, "work")["status"] == "pass", "work need not agree")

    reference = baseline["orders"]["2"]["qualified"]["3"]["best_iou"]
    case = dict(schema="ehgp.audit.full_points.case.v1", status="ok", name="pure_case", sites=baseline["sites"],
                projection=baseline, hdbscan={k: dict(best_iou=reference) for k in ("2", "3")},
                jitter=dict(alternate=dict(projection=copy.deepcopy(baseline))))
    with tempfile.TemporaryDirectory(prefix="ehgp-summary-pure-") as temporary:
        directory = Path(temporary)
        path = directory / "case.json"
        path.write_text(json.dumps(case))
        cases, rows = collect([directory])
        check(bool(rows) and len(cases) == 1, "actual collect reads completed case JSON")
        check(set(cases[0]["qualified_m3_identity"]) == {"primary", "jitter_alternate"},
              "both completed projection results checked")
        mutant = copy.deepcopy(case)
        mutant["jitter"]["alternate"]["projection"]["orders"]["3"]["qualified"]["3"]["tree_sha256"] = "f" * 64
        path.write_text(json.dumps(mutant))
        rejected(lambda: collect([directory]), "alternate identity mismatch rejected")
        mutant = copy.deepcopy(case)
        mutant["status"] = "failed"
        path.write_text(json.dumps(mutant))
        rejected(lambda: collect([directory]), "failed campaign case rejected")
    print(json.dumps(dict(status="pass", checks=CHECKS, unit_cases=len(accepted),
                          scope="pure exact encoder/consumer and collected JSON; no native, fit, real data or GCP"),
                     sort_keys=True))


if __name__ == "__main__":
    main()
