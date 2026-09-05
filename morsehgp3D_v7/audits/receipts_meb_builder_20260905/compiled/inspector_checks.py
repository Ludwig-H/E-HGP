#!/usr/bin/env python3
"""In-memory false-acceptance checks for the Builder receipt inspector."""

import copy
from collections.abc import Callable
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
from unittest import mock

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
AUDITS = HERE.parents[1]
sys.path.insert(0, str(AUDITS))


def require(value: bool, cause: str) -> None:
    if not value:
        raise ValueError(cause)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> dict[str, object]:
    source = AUDITS / "meb_builder_audit.py"
    spec = importlib.util.spec_from_file_location("builder_receipt_inspector_under_test", source)
    driver = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(driver)
    report = json.loads((HERE / "run.json").read_text())
    input_files = {
        "current_judge": source,
        "driver_at_capture": HERE / "driver_at_capture.py",
        "run": HERE / "run.json",
        "include_mapping": HERE.parent / "inputs/include_map.json",
        "inspector_checks": Path(__file__).resolve(),
    }
    input_pins = {k: sha(p.read_bytes()) for k, p in input_files.items()}
    require(input_pins["driver_at_capture"] == report["inputs"]["meb_builder_audit.py"],
            "historical driver binding")
    originals = {p.name: sha(p.read_bytes()) for p in HERE.iterdir()
                 if p.is_file() and not p.name.startswith("inspector_checks")}
    results = []

    def reject(name: str, expected: str, action: Callable[[], object]) -> None:
        try:
            action()
        except ValueError as error:
            require(str(error) == expected, "unexpected rejection for " + name + ": " + str(error))
            results.append(dict(name=name, cause=str(error)))
        else:
            raise ValueError("false acceptance: " + name)

    # None of these checks should ever compile, launch a binary or save a
    # replay result. Mocking these entry points also catches an accidental
    # future expansion of build_provenance()/replay() into execution.
    with mock.patch.object(driver, "execute", side_effect=ValueError("forbidden engine command")), \
            mock.patch.object(driver, "save", side_effect=ValueError("forbidden replay write")) as writer:
        nominal = driver.build_provenance(copy.deepcopy(report))
        require(len(nominal["builds"]) == 7 and
                all(v["source_headers"] == 19 and v["dependencies"] == 20
                    for v in nominal["builds"].values()), "nominal provenance non-vacuity")

        changed = copy.deepcopy(report)
        changed["builds"]["O2"]["sources"]["overlay/silent_incidence.hpp"] = "0" * 64
        reject("transformed_header_pin", "receipt.transformed_source_pins",
               lambda: driver.build_provenance(changed))

        changed = copy.deepcopy(report)
        dependencies = changed["builds"]["O2"]["dependency_pins"]
        selected = next(k for k in dependencies if k.endswith("overlay/meb_proposal.hpp"))
        dependencies[selected + ".foreign"] = dependencies.pop(selected)
        require(len(dependencies) == 20, "replacement retains dependency cardinality")
        reject("foreign_dependency_same_cardinality", "receipt.exact_dependency_pins",
               lambda: driver.build_provenance(changed))

        changed = copy.deepcopy(report)
        next(r for r in changed["commands"] if r["label"] == "O2")["argv"][0] += ".foreign"
        reject("different_binary_argv", "receipt.build_command_binding",
               lambda: driver.build_provenance(changed))

        changed = copy.deepcopy(report)
        changed["builds"]["O2"]["instrumented"] = True
        reject("different_instrumentation_route", "receipt.build_route",
               lambda: driver.build_provenance(changed))

        original_read = Path.read_bytes
        patch_file = HERE / "instrumented_silent_incidence.hpp.patch"

        def changed_patch(path: Path) -> bytes:
            data = original_read(path)
            return data + b"audit_mutation\n" if path == patch_file else data

        with mock.patch.object(Path, "read_bytes", changed_patch):
            reject("changed_patch_bytes", "receipt.patch_bytes",
                   lambda: driver.build_provenance(copy.deepcopy(report)))

        # A malformed mutant output must fail the precise causal-rejection
        # contract, rather than masquerading as a killed semantic mutant.
        original_output = driver.read_output
        with mock.patch.object(driver, "read_output",
                               side_effect=lambda label: "{" if label == "reset_work" else original_output(label)):
            reject("malformed_mutant_JSON_is_not_causal_rejection",
                   "mutant.unexpected_rejection.reset_work", driver.replay)
        require(writer.call_count == 0, "replay attempted a write before rejecting")

    require(originals == {p.name: sha(p.read_bytes()) for p in HERE.iterdir()
                          if p.is_file() and not p.name.startswith("inspector_checks")},
            "preserved receipt bytes changed")
    require(input_pins == {k: sha(p.read_bytes()) for k, p in input_files.items()}, "authority changed")
    return dict(schema="mhgp7-builder-provenance-inspector-checks-v1", status="passed",
                public_status="not_claimed", optimization=sys.flags.optimize,
                positives=1, targeted_rejections=results, input_sha256=input_pins,
                preserved_files_verified_unchanged=len(originals), engine_runs=0,
                build_runs=0, replay_writes=0, GCP="non utilisé")


if __name__ == "__main__":
    try:
        print(json.dumps(main(), indent=2, sort_keys=True))
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(json.dumps(dict(status="failed", error=str(error))))
        sys.exit(1)
