#!/usr/bin/env python3
"""Bounded, read-only gate for the constructor-owned v8 Markdown scope."""

from __future__ import annotations

from collections.abc import Iterator
import importlib.util
import json
from pathlib import Path
import sys
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[2]
V8 = ROOT / "morsehgp3D_v8"
REPORTS = (
    "ETAT_COURANT.md",
    "CONTRATS_ET_MESURES.md",
    "IMPLEMENTATION_PARALLELISATION.md",
    "WSPD_Q2_Q3_Q4.md",
    "FONDEMENTS_ET_OBJET.md",
    "PERIMETRE_ET_PREUVES.md",
)


def run() -> dict[str, object]:
    spec = importlib.util.spec_from_file_location(
        "mhgp8_documentation_checker", ROOT / "tools" / "check_docs.py"
    )
    if spec is None or spec.loader is None:
        raise RuntimeError("checker import unavailable")
    checker = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(checker)
    checks = 0

    def require(condition: bool, cause: str) -> None:
        nonlocal checks
        checks += 1
        if not condition:
            raise RuntimeError(cause)

    expected = {V8 / "README.md", V8 / "PASSATION.md"}
    expected.update(V8 / "audits" / name for name in REPORTS)
    actual = checker.active_markdown()
    require(len(expected) == 8, "scope.expected_nonvacuity")
    for path in sorted(expected):
        require(path in actual, f"scope.required_missing:{path.name}")
    require(len(actual) == len(set(actual)), "scope.duplicate")
    actual_v8 = [path for path in actual if path.is_relative_to(V8)]
    require(len(actual_v8) >= 8, "scope.actual_nonvacuity")

    # Virtual Markdown candidates exercise every recursive include without
    # creating files. A future independent report must remain out of scope.
    virtual = {
        V8 / directory: V8 / directory / "nested" / "scope_probe.md"
        for directory in ("docs", "receipts", "bench")
    }
    independent = V8 / "audits" / "FUTURE_INDEPENDENT_AUDITOR.md"
    original_rglob = Path.rglob
    discovery_calls: list[Path] = []

    def discover(path: Path, pattern: str) -> Iterator[Path]:
        discovery_calls.append(path)
        if path in virtual:
            return iter((virtual[path],))
        if path == V8 / "audits":
            return iter((independent,))
        return original_rglob(path, pattern)

    with patch.object(Path, "rglob", discover):
        discovered = checker.active_markdown()
    for directory, candidate in virtual.items():
        require(directory in discovery_calls, "scope.directory_not_scanned")
        require(candidate in discovered, "scope.recursive_candidate_lost")
    require(independent not in discovered, "scope.independent_auditor_included")
    require(V8 / "audits" not in discovery_calls, "scope.audits_globbed")

    source = V8 / "README.md"
    require(source.is_file(), "validation.positive_target_missing")
    positive = "# Documentation gate\n\n[Entry](README.md)\n"
    missing_name = "__mhgp8_docs_scope_missing_target__.md"
    require(not (V8 / missing_name).exists(), "validation.mutant_target_exists")
    mutant = positive.replace("(README.md)", f"({missing_name})")
    require(positive != mutant, "validation.mutant_not_changed")
    with patch.object(Path, "read_text", return_value=positive) as reader:
        positive_errors = checker.validate(source)
        require(reader.call_count == 1, "validation.positive_not_read")
    require(positive_errors == [], "validation.positive_refused")
    with patch.object(Path, "read_text", return_value=mutant) as reader:
        mutant_errors = checker.validate(source)
        require(reader.call_count == 1, "validation.mutant_not_read")
    expected_error = (
        "morsehgp3D_v8/README.md:3: missing local link "
        f"{missing_name!r}"
    )
    require(mutant_errors == [expected_error], "validation.missing_link_not_refuted")
    require(checks >= 25, "gate.check_nonvacuity")
    return {
        "status": "passed",
        "checks": checks,
        "required_entries": len(expected),
        "recursive_directories": len(virtual),
        "positive_validations": 1,
        "missing_link_mutants_refuted": 1,
        "independent_audits_excluded": True,
        "writes": 0,
        "scope": "documentation_only_not_an_engine_test",
    }


def main() -> int:
    if sys.argv[1:] != ["--selftest"]:
        print("usage: docs_scope_gate.py --selftest", file=sys.stderr)
        return 2
    try:
        result = run()
    except (OSError, RuntimeError, ValueError) as error:
        print(f"documentation scope gate failed: {error}", file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
