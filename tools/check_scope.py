#!/usr/bin/env python3
"""Keep superseded prototypes out of the maintained documentation corpus."""

from __future__ import annotations

import re
import sys
from pathlib import Path

from check_docs import ROOT, active_markdown


HISTORY = ROOT / "docs" / "HISTORIQUE.md"
BANNED = {
    "Perg-HGP": re.compile(r"\bperg(?:_|-)hgp\b", re.IGNORECASE),
    "PowerCover3D": re.compile(r"\bPowerCover3D\b"),
    "HomogeneousLensTower": re.compile(r"\bHomogeneousLensTower\b"),
}
REMOVED_PATHS = (
    ROOT / "perg_hgp",
    ROOT / "experiments" / "powercover3d",
)


def main() -> int:
    errors: list[str] = []
    for path in REMOVED_PATHS:
        if path.exists():
            errors.append(
                f"{path.relative_to(ROOT)}: superseded path must remain physically absent"
            )
    for path in active_markdown():
        if not path.is_file() or path.resolve() == HISTORY.resolve():
            continue
        for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            for name, pattern in BANNED.items():
                if pattern.search(line):
                    errors.append(
                        f"{path.relative_to(ROOT)}:{number}: superseded scope {name}"
                    )

    if errors:
        print("Active-scope validation failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1

    print("Validated active documentation scope.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
