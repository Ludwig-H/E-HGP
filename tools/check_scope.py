#!/usr/bin/env python3
"""Keep superseded prototypes out of active documentation and product builds."""

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
    ROOT / "morsehgp3d" / "src" / "tools" / "gpu_geogram_low_order_diagnostic.cu",
    ROOT / "morsehgp3d" / "src" / "tools" / "gpu_morton_window_h0_surrogate.cpp",
)
PRODUCT_BUILD_FILES = (
    ROOT / ".github" / "workflows" / "ci.yml",
    ROOT / "morsehgp3d" / "CMakeLists.txt",
    ROOT / "morsehgp3d" / "CMakePresets.json",
    ROOT / "morsehgp3d" / "tests" / "configuration" / "check_phase3_build.py",
    ROOT / "morsehgp3d" / "tests" / "unit" / "CMakeLists.txt",
    ROOT
    / "morsehgp3d"
    / "tests"
    / "profiling"
    / "phase15_pair_first_scale_campaign.py",
    ROOT
    / "morsehgp3d"
    / "tests"
    / "profiling"
    / "phase15_pair_first_scale_campaign_v1.json",
)
BANNED_PRODUCT_IDENTIFIERS = (
    "MORSEHGP3D_ENABLE_GEOGRAM_LOW_ORDER_DIAGNOSTIC",
    "morsehgp3d_gpu_geogram_low_order_diagnostic",
    "morsehgp3d_gpu_morton_window_h0_surrogate",
)


def main() -> int:
    errors: list[str] = []
    for path in REMOVED_PATHS:
        if path.exists():
            errors.append(
                f"{path.relative_to(ROOT)}: superseded path must remain physically absent"
            )
    for path in PRODUCT_BUILD_FILES:
        try:
            content = path.read_text(encoding="utf-8")
        except OSError as exc:
            errors.append(
                f"{path.relative_to(ROOT)}: unreadable product build file: {exc}"
            )
            continue
        for identifier in BANNED_PRODUCT_IDENTIFIERS:
            if identifier in content:
                errors.append(
                    f"{path.relative_to(ROOT)}: superseded product identifier "
                    f"{identifier}"
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
