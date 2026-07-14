#!/usr/bin/env python3
"""Validate the machine-readable MorseHGP3D phase register."""

from __future__ import annotations

import re
import sys
import tomllib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REGISTER = ROOT / "docs" / "implementation_status.toml"
ROADMAP = ROOT / "docs" / "ROADMAP_IMPLEMENTATION_MORSEHGP3D.md"
PHASE_HEADING = re.compile(r"^## Phase ([0-9]+[A-Z]?)\b", re.MULTILINE)


def main() -> int:
    errors: list[str] = []
    try:
        data = tomllib.loads(REGISTER.read_text(encoding="utf-8"))
    except (OSError, tomllib.TOMLDecodeError) as exc:
        print(f"Implementation register is unreadable: {exc}", file=sys.stderr)
        return 1

    if data.get("schema_version") != 1:
        errors.append("schema_version must be 1")

    conventions = data.get("conventions", {})
    allowed = conventions.get("allowed_status", [])
    if not isinstance(allowed, list) or not allowed or not all(
        isinstance(status, str) and status for status in allowed
    ):
        errors.append("conventions.allowed_status must be a non-empty string array")
        allowed = []

    phases = data.get("phases")
    if not isinstance(phases, list) or not phases:
        errors.append("at least one [[phases]] table is required")
        phases = []

    phase_by_id: dict[str, dict[str, object]] = {}
    in_progress = 0
    for index, phase in enumerate(phases, start=1):
        label = f"phase entry {index}"
        if not isinstance(phase, dict):
            errors.append(f"{label} must be a table")
            continue
        phase_id = phase.get("id")
        if not isinstance(phase_id, str) or not phase_id:
            errors.append(f"{label} has no non-empty id")
            continue
        label = f"phase {phase_id}"
        if phase_id in phase_by_id:
            errors.append(f"duplicate {label}")
        phase_by_id[phase_id] = phase

        for field in ("title", "track", "status", "blocking_reason"):
            if not isinstance(phase.get(field), str):
                errors.append(f"{label}.{field} must be a string")
        for field in ("entry_gate_satisfied", "exit_gate_satisfied"):
            if not isinstance(phase.get(field), bool):
                errors.append(f"{label}.{field} must be a boolean")
        for field in ("depends_on", "evidence"):
            value = phase.get(field)
            if not isinstance(value, list) or not all(
                isinstance(item, str) and item for item in value
            ):
                errors.append(f"{label}.{field} must be a string array")

        status = phase.get("status")
        if status not in allowed:
            errors.append(f"{label}.status is outside conventions.allowed_status")
        if status == "in_progress":
            in_progress += 1
        if status in {"ready", "in_progress", "completed"} and not phase.get(
            "entry_gate_satisfied"
        ):
            errors.append(f"{label} is {status} without a satisfied entry gate")
        if status == "completed":
            if not phase.get("exit_gate_satisfied"):
                errors.append(f"{label} is completed without a satisfied exit gate")
            if not phase.get("evidence"):
                errors.append(f"{label} is completed without evidence")
        if status == "blocked" and not phase.get("blocking_reason"):
            errors.append(f"{label} is blocked without a reason")

    if in_progress > 1:
        errors.append("at most one phase may be in_progress")

    known_ids = set(phase_by_id)
    try:
        roadmap_ids = set(PHASE_HEADING.findall(ROADMAP.read_text(encoding="utf-8")))
    except OSError as exc:
        errors.append(f"roadmap is unreadable: {exc}")
        roadmap_ids = set()
    if known_ids != roadmap_ids:
        missing = sorted(roadmap_ids - known_ids)
        extra = sorted(known_ids - roadmap_ids)
        if missing:
            errors.append(f"register misses roadmap phases: {', '.join(missing)}")
        if extra:
            errors.append(f"register has phases absent from roadmap: {', '.join(extra)}")

    for phase_id, phase in phase_by_id.items():
        for dependency in phase.get("depends_on", []):
            if dependency != "milestone-specific" and dependency not in known_ids:
                errors.append(f"phase {phase_id} has unknown dependency {dependency}")

    project = data.get("project")
    if not isinstance(project, dict):
        errors.append("[project] table is required")
    else:
        current = project.get("current_phase")
        if current not in known_ids:
            errors.append("project.current_phase must reference a declared phase")
        started = project.get("implementation_started")
        if not isinstance(started, bool):
            errors.append("project.implementation_started must be a boolean")
        if started is False and any(
            phase.get("status") in {"in_progress", "completed"}
            for phase in phase_by_id.values()
        ):
            errors.append(
                "implementation_started=false conflicts with an in-progress or completed phase"
            )

    if errors:
        print("Implementation status validation failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1

    print(f"Validated {len(phases)} implementation phases and their gates.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
