#!/usr/bin/env python3
"""Cross-check C++ exact records against Python's independent Fraction path."""

from __future__ import annotations

import json
import subprocess
import sys
from fractions import Fraction
from pathlib import Path


def canonical_json(value: object) -> str:
    return json.dumps(value, ensure_ascii=False, separators=(",", ":"), sort_keys=True)


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: check_exact_types.py EXACT_TYPES_DUMP")
    executable = Path(sys.argv[1])
    completed = subprocess.run(
        [str(executable)],
        check=True,
        capture_output=True,
        encoding="utf-8",
    )
    lines = completed.stdout.splitlines()
    expected = [
        canonical_json(
            {
                "schema_version": "2.0.0",
                "numerator": "3",
                "denominator": "4",
                "unit": "input_coordinate_unit_squared",
            }
        ),
        canonical_json(
            {
                "schema_version": "2.0.0",
                "x_numerator": "3",
                "y_numerator": "-2",
                "z_numerator": "0",
                "denominator": "6",
                "unit": "input_coordinate_unit",
            }
        ),
        (
            f"{Fraction.from_float(0.1).numerator}/"
            f"{Fraction.from_float(0.1).denominator}"
        ),
    ]
    if lines != expected:
        print("C++ exact records differ from the independent Python corpus", file=sys.stderr)
        print(f"expected={expected!r}", file=sys.stderr)
        print(f"observed={lines!r}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
