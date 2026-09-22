#!/usr/bin/env python3
"""Rejoue les bornes du contre-audit A sur les sources Git épinglées.

Standard library only. No assertion: python3 and python3 -O have identical checks.
No build, cloud data, network, or working-tree source is consulted.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
V8 = "a74e90f22167105cdba90b6850f0597f1a01a329"
V7 = "dc57ffd5fec5b73aff9bc7f79f280fb8bc92a6d1"
M = (1 << 18) - 1

# Full SHA-256, on the bytes returned by `git show REV:PATH`.
HASHES = {
    (V8, "morsehgp3D_v8/src/lanes/q4_local.cpp"):
        "ebe0087c79d8f6bd137063e14156eb34fecc01d5a74aaf550e1bbe0333546ef3",
    (V8, "morsehgp3D_v8/src/lanes/q4_local_partition.cpp"):
        "210caaa63bd247e35e5f745833aba7b6e5686982ce6124b56840d1dc851db8e2",
    (V8, "morsehgp3D_v8/src/lanes/q4_local_partition.hpp"):
        "6835f77123635bf49e731651f34cc02bff79fab2b497b152f011f1bb69f522be",
    (V8, "morsehgp3D_v8/src/lanes/q3_ball_census.hpp"):
        "d80558a46bf38414198fb3f26a3fb8993f3cf9ba9c6a7f94a6f2210bb37dec0f",
    (V8, "morsehgp3D_v8/src/pipeline/wspd_q34.cpp"):
        "79ae04fe505671ab2ebbb15d7b2b546a9beb4a3fd427f8df0af40670b318084e",
    (V8, "morsehgp3D_v8/src/core/types.hpp"):
        "dbe746853b3af64f2a82b70cd030ccea05004d5611c4fc7922f0536e392fa578",
    (V8, "morsehgp3D_v8/docs/ELARGISSEMENT_18_BITS_20260922.md"):
        "4b783c2bf8392f0fd28c5da85cdf213ffb8e5bbedddd753d26c7fd533d2c6511",
    (V7, "morsehgp3D_v7/src/forest/full_ball_tower.hpp"):
        "83f1c78e0656f08cd42522e4cd36d153ce283a6082246a36fe5225b3790c6366",
    (V7, "morsehgp3D_v7/src/lanes/q3.hpp"):
        "4155a1c39193b68c47504e247a36e1bbf28b2c9ecbeeb50d6285d974519563fe",
    (V7, "morsehgp3D_v7/src/lanes/q4.hpp"):
        "58aac9bd57ac1a9b19ad156f6397941f67df1379e29215c50fcf268268491c4a",
    (V7, "morsehgp3D_v7/src/lanes/level.hpp"):
        "acd6641e0616c926f6ce8afb6e294ae9982dcf9c518fa807cc9cfd713da7f34c",
    (V7, "morsehgp3D_v7/src/core/wide.hpp"):
        "a4ac26dd4968e0d45a882bd8210060784d34361fd5d60ec2261d639507833a85",
}


def git(*args: str) -> bytes:
    result = subprocess.run(
        ["git", "-C", str(ROOT), *args], capture_output=True, check=False
    )
    if result.returncode:
        raise RuntimeError(f"git {' '.join(args)} failed: {result.stderr.decode(errors='replace').strip()}")
    return result.stdout


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def bound(name: str, coefficient: int, degree: int, bits: int) -> None:
    value = coefficient * M**degree
    require(value < (1 << bits), f"{name}: {value.bit_length()} bits, expected <2^{bits}")


def center_fixture() -> tuple[int, int, int]:
    """Mirror only the exact formulas at q4_local_partition.cpp:52-65,118-138."""
    a, b, x = (0, 0, 0), (0, M, M), (M, 0, M)
    dot = lambda v, w: sum(v[i] * w[i] for i in range(3))
    v = [b[i] - a[i] for i in range(3)]
    axis = max(range(3), key=lambda i: abs(v[i]))
    i, j = (axis + 1) % 3, (axis + 2) % 3
    h, sign = abs(v[axis]), 1 if v[axis] > 0 else -1
    basis_a, basis_b = [0] * 3, [0] * 3
    basis_a[i], basis_a[axis] = h, -sign * v[i]
    basis_b[j], basis_b[axis] = h, -sign * v[j]
    w = [2 * x[t] - a[t] - b[t] for t in range(3)]
    constant = dot(w, w) - dot(v, v)
    fx, fy = -2 * dot(w, basis_a), -2 * dot(w, basis_b)
    aa, ab, bb = dot(basis_a, basis_a), dot(basis_a, basis_b), dot(basis_b, basis_b)
    p, q = aa * fy - ab * fx, ab * fy - bb * fx
    determinant = fx * q - fy * p
    require(determinant != 0, "q3 fixture became collinear")
    return (abs(constant * q).bit_length(), abs(constant * p).bit_length(),
            abs(determinant).bit_length())


def main() -> dict[str, object]:
    for (revision, path), expected in HASHES.items():
        actual = hashlib.sha256(git("show", f"{revision}:{path}")).hexdigest()
        require(actual == expected, f"source SHA-256 mismatch: {revision[:8]}:{path}")

    # Exact simple comment syntax. Other bounds are checked explicitly below.
    pattern = re.compile(r"(?<![A-Za-z0-9])([0-9]+)\s*\*?\s*M\s*\^\s*([0-9]+)\s*<\s*2\s*\^\s*([0-9]+)")
    paths = git("ls-tree", "-r", "--name-only", V8, "--", "morsehgp3D_v8/src/").decode().splitlines()
    comments = 0
    for path in paths:
        for line_number, line in enumerate(git("show", f"{V8}:{path}").decode().splitlines(), 1):
            if "//" not in line:
                continue
            for coefficient, degree, bits in pattern.findall(line.split("//", 1)[1]):
                bound(f"{path}:{line_number}", int(coefficient), int(degree), int(bits))
                comments += 1
    require(comments == 42, f"expected 42 simple source comments, found {comments}")

    # v8 q3 coefficients, global power, q3 center and local disk.
    for name, c, d, bits in [
        ("v8 q3 A", 12, 4, 76), ("v8 q3 B", 60, 5, 96),
        ("v8 q3 C", 144, 6, 116), ("v8 q3 power", 360, 6, 117),
        ("v8 center numerator", 480, 6, 117),
        ("v8 center determinant", 512, 6, 117),
        ("v8 Gram", 2, 2, 37), ("v8 local disk", 96 * (1 << 40), 2, 83),
        # v7 formulas: q3 DEX/(4G), q4 Cramer with 2*coordinate differences.
        ("v7 q3 numerator", 27, 6, 113),
        ("v7 q3 denominator", 36, 4, 78),
        ("v7 q3 cross", 972, 10, 190),
        ("v7 q4 determinant", 48, 3, 60),
        ("v7 q4 numerator component", 72, 4, 79),
        ("v7 q4 radius numerator", 15552, 8, 158),
        ("v7 q4 radius denominator", 2304, 6, 120),
        ("v7 q4 cross", 15552 * 2304, 14, 278),
    ]:
        bound(name, c, d, bits)

    fixture_bits = center_fixture()
    require(fixture_bits == (112, 113, 114), f"q3 center fixture changed: {fixture_bits}")
    return {"status": "PASS", "v8_pin": V8[:8], "v7_pin": V7[:8],
            "source_hashes": len(HASHES), "simple_comment_bounds": comments,
            "explicit_u18_bounds": 16, "q3_center_fixture_bits": fixture_bits}


if __name__ == "__main__":
    try:
        print(json.dumps(main(), sort_keys=True, separators=(",", ":")))
    except (OSError, RuntimeError, UnicodeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        sys.exit(1)
