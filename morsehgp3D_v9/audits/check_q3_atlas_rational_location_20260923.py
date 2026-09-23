#!/usr/bin/env python3
"""Differential oracle for v9 Local28 closed-cell rational location (read-only)."""
import random

SCALE = 1 << 20
LIMIT = 1 << 117
I128_MAX = (1 << 127) - 1


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def old_location(x, y, den, depth):
    if x < -2 * den or x > 2 * den or y < -2 * den or y > 2 * den:
        return None

    def scaled(value):
        # Identical integer result to the existing 20-bit long division.
        quotient, remainder = divmod(value, den)
        numerator = quotient * SCALE + (remainder * SCALE) // den
        exact = (remainder * SCALE) % den == 0
        return numerator, exact

    sx, ex = scaled(x)
    sy, ey = scaled(y)
    cell = (-2 * SCALE, 2 * SCALE, -2 * SCALE, 2 * SCALE)
    path = []
    for _ in range(depth):
        left, right, bottom, top = cell
        mx, my = (left + right) // 2, (bottom + top) // 2
        children = (
            (left, mx, bottom, my),
            (mx, right, bottom, my),
            (left, mx, my, top),
            (mx, right, my, top),
        )
        for q, candidate in enumerate(children):
            low_x, high_x, low_y, high_y = candidate
            if (low_x <= sx and (sx < high_x or (sx == high_x and ex))
                    and low_y <= sy and (sy < high_y or (sy == high_y and ey))):
                path.append(q)
                cell = candidate
                break
        else:
            raise AssertionError((x, y, den, depth, cell))
    return tuple(path)


def dyadic_location(x, y, den, depth):
    if x < -2 * den or x > 2 * den or y < -2 * den or y > 2 * den:
        return None
    cell = (-2 * SCALE, 2 * SCALE, -2 * SCALE, 2 * SCALE)
    path = []
    for d in range(depth):
        left, right, bottom, top = cell
        mx, my = (left + right) // 2, (bottom + top) // 2
        denominator = 1 if d == 0 else 1 << (d - 1)
        unit = SCALE // denominator
        require(mx % unit == 0 and my % unit == 0, "non-dyadic midpoint")
        jx, jy = mx // unit, my // unit
        lhs_x, rhs_x = x * denominator, jx * den
        lhs_y, rhs_y = y * denominator, jy * den
        require(all(abs(z) <= I128_MAX for z in (lhs_x, rhs_x, lhs_y, rhs_y)),
                "i128 overflow")
        # q=0 is checked first by the current closed-cell descent: on an
        # exact internal boundary choose left/bottom, not right/top.
        right_child = lhs_x > rhs_x
        top_child = lhs_y > rhs_y
        q = int(right_child) + 2 * int(top_child)
        path.append(q)
        cell = (
            mx if right_child else left, right if right_child else mx,
            my if top_child else bottom, top if top_child else my,
        )
    return tuple(path)


def check(x, y, den):
    require(0 < den < LIMIT and abs(x) < LIMIT and abs(y) < LIMIT,
            "centre outside public arithmetic domain")
    # Equality of the complete paths also checks every terminal prefix.
    slow = old_location(x, y, den, 10)
    fast = dyadic_location(x, y, den, 10)
    if slow != fast:
        raise AssertionError((x, y, den, slow, fast))


rng = random.Random(0x230923)
count = 0
for _ in range(100_000):
    den = rng.randrange(1, LIMIT)
    edge = min(2 * den, LIMIT - 1)
    if rng.randrange(4) == 0:
        x = rng.randrange(-LIMIT + 1, LIMIT)
        y = rng.randrange(-LIMIT + 1, LIMIT)
    else:
        x = rng.randrange(-edge, edge + 1)
        y = rng.randrange(-edge, edge + 1)
    check(x, y, den)
    count += 1

# Every split boundary from depth 0..10 is on this 1/512 lattice. Use a
# denominator divisible by 512, including a value just below the u18 proof
# limit. Perturb each numerator by one integer to test the two sides.
for den in (512, 512 * ((1 << 107) + 19), LIMIT - 512):
    for j in range(-1024, 1025):
        numerator = j * den // 512
        for delta in (-1, 0, 1):
            x = numerator + delta
            if abs(x) >= LIMIT:
                continue
            for y in (0, den // 3, -den // 3):
                if abs(y) >= LIMIT:
                    continue
                check(x, y, den)
                check(y, x, den)
                count += 2

# Signed-i128 extremes and root boundary equality/adjacency.
for den in (1, 2, 3, (1 << 116) - 1, 1 << 116, LIMIT - 1):
    for x in (-LIMIT + 1, -2 * den - 1, -2 * den, -den, -1, 0, 1,
              den, 2 * den, 2 * den + 1, LIMIT - 1):
        if abs(x) >= LIMIT:
            continue
        for y in (0, x, -x):
            if abs(y) < LIMIT:
                check(x, y, den)
                count += 1

max_left = (LIMIT - 1) * (1 << 9)
max_right = ((1 << 10) - 1) * (LIMIT - 1)
require(max_left < 1 << 126 and max_right < 1 << 127,
        "dyadic comparison exceeds i128 proof")
print(f"PASS {count} centers x 11 depths; all dyadic split boundaries ±2 and neighbors; "
      f"i128 maxima lhs={max_left.bit_length()} bits, rhs={max_right.bit_length()} bits")
