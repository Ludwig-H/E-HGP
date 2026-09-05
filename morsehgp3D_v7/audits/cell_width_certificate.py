"""Check integer bounds for the u16 cell path, without compiling the product."""

from __future__ import annotations

import hashlib
import json
from math import isqrt
from pathlib import Path
import sys


AUDITS = Path(__file__).resolve().parent
LINEAGE = AUDITS.parent


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def main() -> int:
    m = 65535
    # Basis bound applies to the first, always successful iteration for den 8/12.
    bounds = {
        "w_coordinate": 2 * m,
        "w_norm2": 12 * m**2,
        "gram_entry_abs": 3 * m**2,
        "gram_product": 9 * m**4,
        "du_dv_abs": 6 * m**2,
        "rhs_abs_G16": 192 * m**2,
        "mag_G16": 768 * m**2,
        "Nk_abs": 45 * m**5,
        "center_pu_pv_abs": 135 * m**6,
        "J_upper": 81 * m**6,
        "normal_basis_dot_abs": 6 * m**3,
    }
    mu = isqrt(bounds["J_upper"] // 2) + 1
    bounds["mu_upper"] = mu
    bounds["endpoint_pu_pv_abs"] = 135 * m**6 + mu * 6 * m**3
    require(max(bounds[k] for k in ("rhs_abs_G16", "mag_G16")) < 2**46,
            "cell fast path bound")
    require(bounds["gram_product"] < 2**68, "Gram products")
    require(bounds["endpoint_pu_pv_abs"] < 2**105, "endpoint width")
    require(bounds["Nk_abs"] < 2**86, "Nk width")
    require(mu < 2**51, "mu width")
    # Generic guarded i64 branch: each operand is below 2^62, the difference
    # below 2^63; these synthetic extrema are not claimed to be u16 geometry.
    limit = 2**62
    guarded = []
    for grid in (8, 16):
        du = (limit - 1) // (4 * grid)
        for sign in (-1, 1):
            rhs = sign * (limit - 1)
            for j in (-grid, grid):
                value = rhs - 4 * j * du
                require(-2**63 < value < 2**63, "guarded subtraction")
                guarded.append(value)
    require(any(abs(v) > 2**62 for v in guarded), "guard nonvacuity")
    # Strict overestimate, including exact-square boundaries.
    square_checks = 0
    for k in (0, 1, 2, 65535, 2**32, isqrt(bounds["J_upper"] // 2)):
        for delta in (-1, 0, 1):
            value = 2 * k * k + delta
            if value < 0:
                continue
            root = isqrt(value // 2) + 1
            require(2 * root * root > value, "strict chord enclosure")
            square_checks += 1
    require(square_checks == 17, "square check floor")
    # Width mutants fail against genuine integer bounds, not sampled outputs.
    mutants = {
        "rhs_narrow_i32": bounds["rhs_abs_G16"] >= 2**31,
        "endpoint_narrow_i64": bounds["endpoint_pu_pv_abs"] >= 2**63,
        "sqrt_without_plus_one": 2 * isqrt(2 // 2)**2 <= 2,
    }
    require(all(mutants.values()), "width/enclosure mutant survived")
    paths = [
        "src/lanes/cell_grid.hpp", "src/lanes/sector_kill.hpp",
        "src/pipeline/generate.hpp", "src/lanes/q3.hpp",
        "src/core/types.hpp", "src/pipeline/float_filter.hpp",
    ]
    record = {
        "status": "passed", "public_status": "not_claimed",
        "authority": "integer bound certificate; no product execution",
        "bounds": {k: {"value": v, "bits": v.bit_length()}
                   for k, v in bounds.items()},
        "guarded_extrema": guarded, "square_checks": square_checks,
        "bound_mutants_detected": mutants,
        "pinned_sources": {p: hashlib.sha256((LINEAGE / p).read_bytes()).hexdigest()
                           for p in paths},
        "script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "python_optimized": not __debug__, "gcp": "not_used",
    }
    out = AUDITS / "receipts_front_20260905"
    out.mkdir(exist_ok=True)
    name = "cell_width_optimized.json" if not __debug__ else "cell_width.json"
    (out / name).write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps({"cell_width": "passed", "square_checks": square_checks,
                      "bound_mutants": len(mutants)}))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, ValueError) as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1) from error
