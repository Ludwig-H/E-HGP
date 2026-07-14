#!/usr/bin/env python3
"""Audit the reference oracle's imports without importing the oracle itself."""

from __future__ import annotations

import ast
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ORACLE_ROOT = ROOT / "reference" / "morsehgp3d_oracle"

# A checked-in allowlist makes the decision independent of installed packages
# and of environment-specific import resolution. Adding a new standard-library
# dependency therefore requires an explicit review of this file.
APPROVED_STDLIB_ROOTS = frozenset(
    {
        "__future__",
        "dataclasses",
        "enum",
        "fractions",
        "hashlib",
        "itertools",
        "json",
        "math",
        "pathlib",
        "platform",
        "random",
        "re",
        "struct",
        "typing",
    }
)
DYNAMIC_IMPORT_CALLS = frozenset({"__import__", "import_module", "load_module"})


@dataclass(frozen=True, order=True)
class IndependenceViolation:
    """One deterministic source location that breaks oracle independence."""

    path: str
    line: int
    column: int
    reason: str

    def render(self) -> str:
        return f"{self.path}:{self.line}:{self.column}: {self.reason}"


def _display_path(path: Path, package_root: Path) -> str:
    try:
        return path.resolve().relative_to(ROOT.resolve()).as_posix()
    except ValueError:
        return path.resolve().relative_to(package_root.resolve().parent).as_posix()


def _relative_import_escapes(
    path: Path, package_root: Path, node: ast.ImportFrom
) -> bool:
    relative = path.relative_to(package_root)
    package_depth = len(relative.parent.parts)
    return node.level > package_depth + 1


def audit_oracle_imports(
    package_root: Path = ORACLE_ROOT,
) -> tuple[IndependenceViolation, ...]:
    """Return every forbidden or non-static import in canonical source order."""

    package_root = package_root.resolve()
    if not package_root.is_dir():
        return (
            IndependenceViolation(
                package_root.as_posix(),
                0,
                0,
                "oracle package directory is missing",
            ),
        )

    source_paths = tuple(sorted(package_root.rglob("*.py")))
    if not source_paths:
        return (
            IndependenceViolation(
                package_root.as_posix(),
                0,
                0,
                "oracle package contains no Python source",
            ),
        )

    violations: list[IndependenceViolation] = []
    for path in source_paths:
        display_path = _display_path(path, package_root)
        if path.is_symlink():
            violations.append(
                IndependenceViolation(
                    display_path,
                    0,
                    0,
                    "symbolic-link sources are forbidden in the reference oracle",
                )
            )
            continue
        try:
            source = path.read_text(encoding="utf-8")
            tree = ast.parse(source, filename=display_path, type_comments=True)
        except (OSError, SyntaxError, UnicodeError) as error:
            violations.append(
                IndependenceViolation(
                    display_path,
                    getattr(error, "lineno", 0) or 0,
                    getattr(error, "offset", 0) or 0,
                    f"source cannot be audited: {type(error).__name__}: {error}",
                )
            )
            continue

        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                for alias in node.names:
                    root = alias.name.partition(".")[0]
                    if root not in APPROVED_STDLIB_ROOTS:
                        violations.append(
                            IndependenceViolation(
                                display_path,
                                node.lineno,
                                node.col_offset + 1,
                                f"absolute import {alias.name!r} is not approved stdlib; "
                                "oracle modules must use relative imports internally",
                            )
                        )
            elif isinstance(node, ast.ImportFrom):
                if node.level:
                    if _relative_import_escapes(path, package_root, node):
                        violations.append(
                            IndependenceViolation(
                                display_path,
                                node.lineno,
                                node.col_offset + 1,
                                "relative import escapes reference/morsehgp3d_oracle",
                            )
                        )
                else:
                    module = node.module or ""
                    root = module.partition(".")[0]
                    if root not in APPROVED_STDLIB_ROOTS:
                        violations.append(
                            IndependenceViolation(
                                display_path,
                                node.lineno,
                                node.col_offset + 1,
                                f"absolute import from {module!r} is not approved stdlib; "
                                "oracle modules must use relative imports internally",
                            )
                        )
            elif isinstance(node, ast.Call):
                called_name: str | None = None
                if isinstance(node.func, ast.Name):
                    called_name = node.func.id
                elif isinstance(node.func, ast.Attribute):
                    called_name = node.func.attr
                if called_name in DYNAMIC_IMPORT_CALLS:
                    violations.append(
                        IndependenceViolation(
                            display_path,
                            node.lineno,
                            node.col_offset + 1,
                            f"dynamic import call {called_name!r} cannot be audited statically",
                        )
                    )

    return tuple(sorted(set(violations)))


def main() -> int:
    violations = audit_oracle_imports()
    if violations:
        print("Reference-oracle independence validation failed:", file=sys.stderr)
        for violation in violations:
            print(f"- {violation.render()}", file=sys.stderr)
        return 1

    source_count = len(tuple(sorted(ORACLE_ROOT.rglob("*.py"))))
    print(
        f"Validated {source_count} reference-oracle modules: only approved stdlib "
        "and in-package relative imports are used."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
