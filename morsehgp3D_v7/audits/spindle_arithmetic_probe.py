"""Lightweight integer ledger and mathematical fixtures, not C++ execution."""

from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import subprocess
import time


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
RECEIPTS = HERE / "receipts_front_20260905"
SOURCE_PATHS = [
    "morsehgp3D_v7/src/spindle/spindle.hpp",
    "morsehgp3D_v7/src/spindle/witness_count.hpp",
    "morsehgp3D_v7/src/core/intmath.hpp",
    "morsehgp3D_v7/src/core/types.hpp",
    "morsehgp3D_v7/src/core/caps.hpp",
    "morsehgp3D_v7/src/tree/cloud_index.hpp",
    "morsehgp3D_v7/src/wspd/wavefront.hpp",
    "morsehgp3D_v7/src/pipeline/generate.hpp",
    "morsehgp3D_v7/src/pipeline/run.hpp",
    "morsehgp3D_v7/audits/spindle_arithmetic_probe.py",
]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def hashes() -> dict[str, str]:
    return {
        path: hashlib.sha256((ROOT / path).read_bytes()).hexdigest()
        for path in SOURCE_PATHS
    }


def ceil_root(value: int) -> int:
    root = math.isqrt(value)
    return root + (root * root != value)


def corrected_root_model(value: int, seed: int) -> int:
    """Mathematical model of the two loops; Python integers cannot overflow."""
    if value <= 0:
        return 0
    root = seed
    while root > 0 and root * root > value:
        root -= 1
    while (root + 1) * (root + 1) <= value:
        root += 1
    return root


def run() -> dict[str, object]:
    start = time.monotonic()
    before = hashes()
    spindle = (ROOT / SOURCE_PATHS[0]).read_text(encoding="utf-8")
    declarations = [
        "kSpindleD = 1ll << 30;",
        "kA2 = kSpindleD;",
        "kA3 = 619000000;",
        "kA4 = 555000000;",
        "kCoupE = 1ll << 20;",
        "kC2 = 2 * kCoupE;",
        "kC3 = (4 * kCoupE + 2) / 3;",
        "kC4 = 1329545;",
    ]
    for declaration in declarations:
        require(declaration in spindle, f"source constant changed: {declaration}")
    maximum = 65535
    scale_a, scale_c = 1 << 30, 1 << 20
    a3, a4 = 619000000, 555000000
    c2, c3, c4 = 2 * scale_c, (4 * scale_c + 2) // 3, 1329545
    radius_2 = ceil_root(3 * maximum * maximum)
    distance_2 = math.isqrt(12 * maximum * maximum)
    s2 = 2 * radius_2 * radius_2
    numerator = 2 * c2 * s2 + scale_c - 1
    sub2 = numerator // scale_c
    require(3 * a3 * a3 < scale_a * scale_a, "q3 lower coefficient")
    discriminant = 2 * scale_a * scale_a - a4 * a4
    require(discriminant > 0 and discriminant * discriminant > 3 * scale_a**4,
            "q4 lower coefficient")
    require(3 * c3 >= 4 * scale_c, "q3 upper subtraction")
    require((3 * scale_c - c4) ** 2 < 3 * scale_c * scale_c, "q4 upper subtraction")
    bounds = {
        "d2q": (12 * maximum * maximum, 64),
        "w2a_w2b": (3 * maximum * maximum, 64),
        "ra2u_rb2u": (radius_2, 64),
        "d2u": (distance_2, 64),
        "r2u": (2 * radius_2, 64),
        "s2": (s2, 64),
        "sub2_numerator": (numerator, 64),
        "sub2": (sub2, 64),
        "ceil_sqrt_sub2": (ceil_root(sub2), 64),
        "aq_times_d2u": (scale_a * distance_2, 128),
        "center4": (4 * maximum, 64),
        "ball_distance_squared": (48 * maximum * maximum, 128),
        "ball_radius_squared": (distance_2 * distance_2, 128),
        "cross_square_sum_loose": (12 * maximum**4, 128),
        "three_h_square_loose": (27 * maximum**4, 128),
        "hmax4_absolute_loose": (12 * maximum * maximum, 64),
        "q4_static_assert_square": (discriminant * discriminant, 128),
        "q4_static_assert_rhs": (3 * scale_a**4, 128),
    }
    ledger: dict[str, object] = {}
    for name, (value, storage_bits) in bounds.items():
        require(value < 1 << (storage_bits - 1), f"signed bound failed: {name}")
        ledger[name] = {
            "upper_bound": value,
            "magnitude_bits": value.bit_length(),
            "signed_storage_bits": storage_bits,
        }

    # Boundary samples exercise the integer correction model, never a C++ libm.
    root_inputs = {0, 1, 2, sub2, 3 * maximum * maximum, 12 * maximum * maximum}
    for root in [1, 2, 3, 7, 113509, 113510, 227019, 321054]:
        for delta in [-1, 0, 1, 2 * root]:
            value = root * root + delta
            if 0 <= value <= sub2:
                root_inputs.add(value)
    root_cases = 0
    for value in sorted(root_inputs):
        expected = math.isqrt(value)
        require(int(float(value)) == value, "bounded integer-to-double conversion")
        for seed in {int(math.sqrt(value)), max(0, expected - 2), expected, expected + 2}:
            actual = corrected_root_model(value, seed)
            require(actual == expected, "integer root correction model")
            require(actual * actual <= value < (actual + 1) ** 2, "root enclosure")
            root_cases += 1

    axis_cases = 0
    for a in [0, 1, maximum // 2, maximum - 1, maximum]:
        for b in [0, 1, maximum // 2, maximum - 1, maximum]:
            for z in [0, 1, maximum // 2, maximum - 1, maximum]:
                expanded = z * (a + b) - a * b - z * z
                factored = (z - a) * (b - z)
                require(expanded == factored, "H axis identity")
                require(-maximum * maximum <= expanded and 4 * expanded <= maximum * maximum,
                        "H axis mathematical bound")
                axis_cases += 1

    # A=(0,0,0), B=(1,1,0), Z=(0,1,0): exact diametral shell.
    # Rounding the center distance upwards produces a false strict inclusion.
    normal_radius4 = math.isqrt(8)
    mutant_radius4 = ceil_root(8)
    witness_distance4_squared = 8
    witness_h = 0
    require(not witness_distance4_squared < normal_radius4**2, "normal core direction")
    require(witness_distance4_squared < mutant_radius4**2 and witness_h == 0,
            "ceil-distance mutant mathematical counter-fixture")
    require(root_cases >= 90 and axis_cases == 125, "non-vacuity")
    after = hashes()
    require(before == after, "sources changed during lightweight audit")
    return {
        "status": "completed",
        "scope": "static_bound_ledger_and_python_mathematical_models",
        "cpp_executed": False,
        "compilation": False,
        "phase": "exploration_v7_hors_registre",
        "backend": "cpu_reference",
        "profile": "quantized_u16_input_only",
        "mode": "audit_independant_math_and_architecture",
        "public_status": "not_claimed",
        "gcp": "not_used",
        "utc": datetime.now(timezone.utc).isoformat(),
        "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "worktree": subprocess.check_output(["git", "status", "--short"], cwd=ROOT, text=True).strip(),
        "command": "python3 -O morsehgp3D_v7/audits/spindle_arithmetic_probe.py",
        "sources_before": before,
        "sources_after": after,
        "ledger": ledger,
        "root_model_cases": root_cases,
        "axis_identity_cases": axis_cases,
        "ceil_distance_counter_fixture": {
            "a": [0, 0, 0], "b": [1, 1, 0], "z": [0, 1, 0],
            "normal_radius4": normal_radius4,
            "mutant_radius4": mutant_radius4,
            "witness_distance4_squared": witness_distance4_squared,
            "witness_h": witness_h,
            "authority": "integer_equations_not_cpp_run",
        },
        "elapsed_seconds": time.monotonic() - start,
    }


if __name__ == "__main__":
    result = run()
    RECEIPTS.mkdir(exist_ok=True)
    (RECEIPTS / "spindle_bounds.json").write_text(
        json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )
    print(f"status={result['status']} root_models={result['root_model_cases']} "
          f"axis_identities={result['axis_identity_cases']} cpp_executed=false "
          f"elapsed_seconds={result['elapsed_seconds']:.6f}")
