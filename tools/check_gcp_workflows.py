#!/usr/bin/env python3
"""Enforce a small, explicit read-only allowlist for GCP GitHub workflows."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
WORKFLOWS = ROOT / ".github" / "workflows"

# Folded scalars turn physical newlines into spaces. Reject them everywhere in
# workflows so aliases or anchors cannot hide a folded command from the audit.
FOLDED_SCALAR = re.compile(
    r"^[ \t]*(?:-[ \t]*)?[^#\n]+:[^#\n]*>[+-]?[ \t]*(?:#.*)?$",
    re.MULTILINE,
)
MUTATING_REPOSITORY_SCRIPT = re.compile(
    r"(?:^|[\s/])(?:deploy|start_and_verify|stop_and_verify)\.sh\b",
    re.IGNORECASE,
)
GCLOUD_WORD = re.compile(r"(?<![A-Za-z0-9_-])gcloud(?![A-Za-z0-9_-])", re.IGNORECASE)
ALLOWED_GCLOUD_COMMANDS = (
    re.compile(r"^gcloud\s+auth\s+list(?:\s|$)", re.IGNORECASE),
    re.compile(
        r"^gcloud\s+compute\s+instances\s+(?:list|describe)(?:\s|$)",
        re.IGNORECASE,
    ),
)
FORBIDDEN_INDIRECTION = re.compile(
    r"(?:^|[;&|]\s*)(?:eval|source)\b|\b(?:bash|sh)\s+-c\b",
    re.IGNORECASE,
)


def _logical_lines(text: str) -> list[str]:
    """Join shell backslash continuations without applying YAML folding."""

    normalized = re.sub(r"\\\r?\n[ \t]*", " ", text)
    return normalized.splitlines()


def validate_workflow(path: Path, text: str) -> list[str]:
    """Return policy violations for one workflow."""

    errors: list[str] = []
    display_path = path.relative_to(ROOT) if path.is_relative_to(ROOT) else path

    if FOLDED_SCALAR.search(text):
        errors.append(
            f"{display_path}: folded YAML scalar is forbidden; use literal run: | blocks"
        )

    joined = "\n".join(_logical_lines(text))
    if MUTATING_REPOSITORY_SCRIPT.search(joined):
        errors.append(f"{display_path}: billable repository script reference")
    if FORBIDDEN_INDIRECTION.search(joined):
        errors.append(f"{display_path}: shell command indirection is forbidden")

    for line_number, line in enumerate(_logical_lines(text), start=1):
        for occurrence in GCLOUD_WORD.finditer(line):
            command = line[occurrence.start() :].strip()
            if not any(pattern.match(command) for pattern in ALLOWED_GCLOUD_COMMANDS):
                errors.append(
                    f"{display_path}:{line_number}: gcloud command outside read-only allowlist"
                )

    if path.name == "gcp.yml":
        if "GCP_VIEWER_SERVICE_ACCOUNT" not in text:
            errors.append(
                f"{display_path}: missing dedicated GCP_VIEWER_SERVICE_ACCOUNT"
            )
        if re.search(r"github-deployer|instanceAdmin", text, re.IGNORECASE):
            errors.append(f"{display_path}: privileged deployment principal forbidden")
        if not re.search(
            r"service_account:\s*\$\{\{\s*env\.GCP_VIEWER_SERVICE_ACCOUNT\s*\}\}",
            text,
        ):
            errors.append(
                f"{display_path}: WIF must use env.GCP_VIEWER_SERVICE_ACCOUNT"
            )

    return errors


def main() -> int:
    errors: list[str] = []
    for path in sorted((*WORKFLOWS.glob("*.yml"), *WORKFLOWS.glob("*.yaml"))):
        errors.extend(validate_workflow(path, path.read_text(encoding="utf-8")))

    if errors:
        print("GCP workflow safety validation failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1

    print("Validated read-only GCP workflow allowlist.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
