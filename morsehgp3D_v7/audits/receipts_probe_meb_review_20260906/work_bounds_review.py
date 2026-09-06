"""Prove missing Work consistency checks using inert reader snapshots and old captures.

Only the two pure Work predicates are extracted with AST. No reader main,
constructor runner, C++ engine or benchmark is invoked. The proposed inequalities
are checked on existing captured FULL results, not newly generated geometries.
"""
from __future__ import annotations

import ast
from collections import Counter
import gzip
import hashlib
import json
import math
from pathlib import Path

HERE = Path(__file__).resolve().parent
FULL = HERE.parent / "receipts_full_meb_20260906"
FIELDS = ("meb_proposal_supports", "meb_proposal_pivots", "meb_proposal_certified",
          "meb_proposal_fallback", "meb_reference_supports")
READERS = {
    "primary": ("work_primary.py.txt", "475b92884d4e0aac5f9a2856ab841401ea1681a3477ea53599faa9a5140b3e11", "meb_work"),
    "first_c": ("work_first_c.py.txt", "9f54cb46518390942079379168813da84a4789fae482cf851627122a857799b6", "check_meb")}
HELPER_SHA = "f922544b5cfdc214de96ecd49520e318ea8632d14a8142ef21fd248f9cc38fb3"


def require(ok: bool, message: str) -> None:
    if not ok:
        raise ValueError(message)


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def read_json(path: Path):
    return json.loads(path.read_text())


def predicate(filename: str, pin: str, function: str):
    raw = (HERE / filename).read_bytes()
    require(sha(raw) == pin, "snapshot.pin:" + filename)
    tree = ast.parse(raw)
    fields = next(node.value for node in tree.body if isinstance(node, ast.Assign) and
                  any(isinstance(target, ast.Name) and target.id == "MEB_FIELDS" for target in node.targets))
    require(isinstance(fields, ast.Call) and isinstance(fields.func, ast.Attribute) and
            fields.func.attr == "split" and not fields.args and not fields.keywords,
            "snapshot.field_inventory_expression")
    require(tuple(ast.literal_eval(fields.func.value).split()) == FIELDS, "snapshot.field_inventory")
    selected = [node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name in ("require", function)]
    require(len(selected) == 2, "snapshot.two_pure_functions")
    namespace = {"MEB_FIELDS": FIELDS, "JsonObject": dict, "Json": dict}
    exec(compile(ast.Module(body=selected, type_ignores=[]), filename + ":pure_predicate", "exec"), namespace)
    return namespace[function]


def model(p: int, pivots: int, certified: int, fallback: int,
          actual: int, legacy: int, geometry: int, outer: int) -> dict:
    return dict(zip(FIELDS, (p, pivots, certified, fallback, actual)),
                meb_supports=legacy, geometry_meb_calls=geometry, meb_calls=outer)


def extra_bounds(row: dict) -> None:
    # An external FULL call is paid before any form, including observer throws.
    require(row[FIELDS[0]] <= 146 * row["meb_calls"], "proposal_forms_without_sufficient_FULL_calls")
    # In a fresh order, d is the sum of positive virtual ordinal charges.
    virtual = row["meb_supports"] - row[FIELDS[4]]
    certificates = row[FIELDS[2]]
    require(certificates <= virtual <= 550 * certificates, "virtual_ordinal_charge_interval")


def captured_rows(predicates: dict) -> dict:
    helper = FULL / "source/morsehgp3D_v7/src/forest/meb_proposal.hpp"
    require(sha(helper.read_bytes()) == HELPER_SHA, "helper.stable_certification_transaction")
    pins, counts = {}, Counter()
    maximums = {"forms_per_outer_call": [0, 1], "virtual_charge_per_certificate": [0, 1]}
    for build in ("O2", "sanitized"):
        for corpus in ("legacy", "mixed", "higher"):
            for cap in (0, 1, 1000000):
                name = f"{build}_{corpus}_P{cap}"
                receipt_bytes = (FULL / (name + ".json")).read_bytes()
                receipt = json.loads(receipt_bytes)
                compressed = (FULL / (name + ".stdout.gz")).read_bytes()
                raw = gzip.decompress(compressed)
                require(receipt["exit_code"] == 0 and receipt["stdout_gzip_sha256"] == sha(compressed) and
                        receipt["stdout_sha256"] == sha(raw), "captured.output_binding:" + name)
                pins[name] = {"receipt_sha256": sha(receipt_bytes), "gzip_sha256": sha(compressed), "stdout_sha256": sha(raw)}
                records = json.loads(raw)["records"]
                for record in records:
                    require(record["proposal_limit"] == cap, "captured.P_binding")
                    require(record["meb_accounting"] == "reference_ordinal_plus_native_z_q3_q4_proposal_v2",
                            "captured.accounting")
                    counts["nominal_rows"] += 1
                    if record["order"] == 10:
                        counts["K10_rows"] += 1
                    for kind, result in [("nominal", record)] + [("budget", trial) for trial in record["budget_trials"]]:
                        s = result["stats"]
                        work = s["proposal"]
                        row = model(work["p"], work["pivots"], work["certified"], work["fallback"], work["A"],
                                    s["geometry"]["meb_supports"], s["geometry"]["meb_calls"], s["meb_calls"])
                        complete = result["status"] == 0
                        for reader in predicates.values():
                            reader(row, cap, complete)
                        extra_bounds(row)
                        counts["all_rows"] += 1
                        counts["budget_rows"] += kind == "budget"
                        counts["completed_rows"] += complete
                        counts["refused_prefix_rows"] += not complete
                        counts["zero_certificate_with_physical_F"] += work["certified"] == 0 and work["A"] > 0
                        counts["certification_and_F_rows"] += work["certified"] > 0 and work["fallback"] > 0
                        counts["positive_forms_rows"] += work["p"] > 0
                        for name2, numerator, denominator in (
                                ("forms_per_outer_call", work["p"], s["meb_calls"]),
                                ("virtual_charge_per_certificate", s["geometry"]["meb_supports"] - work["A"], work["certified"])):
                            old = maximums[name2]
                            if denominator and numerator * old[1] > old[0] * denominator:
                                maximums[name2] = [numerator, denominator]
    require(counts["nominal_rows"] == 5568 and counts["budget_rows"] == 1248 and counts["K10_rows"] == 48,
            "captured.exact_inventory")
    require(all(counts[key] > 0 for key in ("refused_prefix_rows", "zero_certificate_with_physical_F",
                                         "certification_and_F_rows", "positive_forms_rows")), "captured.nonvacuum")
    return {"counts": dict(counts), "max_ratios_as_exact_pairs": maximums, "inputs": pins,
            "scope": "Existing nominal O2/SAN FULL captures only; reset-work mutant is excluded."}


def main() -> None:
    predicates = {name: predicate(*spec) for name, spec in READERS.items()}
    # These eight algebraic examples are transcribed from pinned primary's
    # meb_selftest, not claimed as eight physical executions or reachable states.
    positives = [
        (model(0, 0, 0, 0, 0, 0, 0, 0), 0, True),
        (model(0, 0, 0, 2, 7, 7, 2, 2), 0, True),
        (model(2, 0, 2, 0, 0, 7, 2, 2), 9, True),
        (model(1, 0, 1, 1, 3, 7, 2, 2), 1, True),
        (model(1, 0, 1, 0, 0, 7, 1, 2), 1, False),
        (model(0, 0, 0, 0, 7, 7, 1, 1), 0, False),
        (model(3, 1, 0, 0, 0, 0, 0, 1), 3, False),
        (model(0, 0, 0, 0, 0, 0, 0, 0), 584000000, True)]
    for row, cap, complete in positives:
        for reader in predicates.values():
            reader(row, cap, complete)
        extra_bounds(row)
    # Only Work diagnostics are changed: the first is the independently
    # qualified triangle P1 state with A=4 erased. No cloud is regenerated.
    mutations = [
        ("erased_F_work_without_certificate", model(1, 1, 0, 1, 0, 4, 1, 1), 1,
         "virtual_ordinal_charge_interval"),
        ("form_paid_without_FULL_call", model(1, 0, 0, 0, 0, 0, 0, 0), 1,
         "proposal_forms_without_sufficient_FULL_calls"),
        ("one_certificate_charged_551", model(6, 2, 1, 0, 0, 551, 1, 1), 6,
         "virtual_ordinal_charge_interval")]
    observed = []
    for name, row, cap, reason in mutations:
        for reader in predicates.values():
            reader(row, cap, True)
        rejected = None
        try:
            extra_bounds(row)
        except ValueError as error:
            rejected = str(error)
        require(rejected == reason, "mutation.exact_cause:" + name)
        observed.append({"name": name, "row": row, "P": cap,
                         "both_captured_predicates_accept": True, "proposed_bound_rejects": reason})
    ordinal_max = sum(math.comb(11, q) for q in (2, 3, 4))
    require(ordinal_max == 550, "proof.ordinal_max")
    result = {
        "schema": "mhgp7-independent-probe-work-bounds-v1", "status": "passed",
        "public_status": "not_claimed", "gcp": "not_used", "engines_invoked": 0,
        "snapshot_pins": {name: {"file": spec[0], "sha256": spec[1], "extracted_function": spec[2]}
                          for name, spec in READERS.items()},
        "execution_scope": "Only require plus each pure Work predicate are AST-extracted; no reader main or runner is executed.",
        "positive_scalar_models_retained": len(positives), "scalar_models_are_engine_results": False,
        "mutants": observed, "compiled_captures_rechecked": captured_rows(predicates),
        "proof": {
            "captured_helper_sha256": HELPER_SHA,
            "domain": "Fresh per-order Work/counters, immutable caps and input, K<=10, local sites<=11.",
            "forms": "An outer FULL call is paid before any form; at most 146 forms per native attempt. Thus p<=146*FULL_calls even before geometry entry.",
            "virtual_work": "Starting at c=A=0, F increments c and A equally; every certificate increments c by min(R, L-c_before), never A.",
            "ordinal": "For K10 the chain has 11 sites; R<=C(11,2)+C(11,3)+C(11,4)=55+165+330=550.",
            "positive_margin": "The stable dispatcher rejects c>=L before proposing; certification sees immutable L and positive remaining margin. Each virtual increment lies in [1,550].",
            "conclusion": "certified <= c-A <= 550*certified, on completed and normally observable refused prefixes; in particular certified=0 implies A=c even for P>0.",
            "exceptions": "Observer injection occurs before form/certification, and the F mirror runs on unwind. No throwing operation lies between incrementing certified and charging c in this fixed arithmetic path.",
            "excluded_observation": "An artificially exposed state stopped between ++certified and the c charge (certified=1,c=A=0) is not a C++ throw boundary of this code. An asynchronous termination there supplies no closed probe receipt and is outside this claim.",
            "limits": "The new checks reject impossible accounting; they neither certify geometry nor prove all accepted scalar states reachable."}}
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
