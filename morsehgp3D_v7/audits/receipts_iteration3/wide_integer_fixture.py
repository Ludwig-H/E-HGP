#!/usr/bin/env python3
"""Independent big-integer fixture evaluation; does not execute the C++ product."""

import json
import math

R = 1 << 64
I128_MIN = -(1 << 127)
I128_MAX = (1 << 127) - 1
records = []


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def words(number: int, count: int) -> list[int]:
    require(0 <= number < R ** count, "integer does not fit requested limb count")
    return [(number >> (64 * index)) & (R - 1) for index in range(count)]


def order(left: int, right: int) -> int:
    return (left > right) - (left < right)


# Two literal levels, first nonzero bit of w[4]; no call to a U192 builder.
x_num, x_den, y_num, y_den = 1 << 130, 1, 1, 1 << 126
require(x_num < R ** 3 and y_num < R ** 3, "ExactLevel numerator domain")
require(0 < x_den <= I128_MAX and 0 < y_den <= I128_MAX, "ExactLevel denominator domain")
left, right = x_num * y_den, y_num * x_den
require(words(left, 5) == [0, 0, 0, 0, 1], "minimal high-word literal")
require(words(right, 5) == [1, 0, 0, 0, 0], "comparison right literal")
nominal, truncated = order(left, right), order(left % (1 << 256), right % (1 << 256))
require(nominal == 1 and truncated == -1, "isolated U320 truncation must reverse comparison")
records.append({"name": "first_bit_u320_exact_level", "domain": "generic_numeric_not_geometric",
                "x_num_words": words(x_num, 3), "x_den": x_den,
                "y_num_words": words(y_num, 3), "y_den": y_den,
                "left_words": words(left, 5), "right_words": words(right, 5),
                "nominal_order": nominal, "python_modeled_drop_w4_order": truncated})

full = (R ** 3 - 1) * (R ** 2 - 1)
require(full == R ** 5 - R ** 3 - R ** 2 + 1, "generic maximum product identity")
require(words(full, 5) == [1, 0, R - 1, R - 2, R - 1], "W2 full carry literal")
records.append({"name": "u320_full_carry", "words": words(full, 5)})

u192_site = (1 << 64) * (1 << 64)
require(words(u192_site, 3) == [0, 0, 1], "isolated U192 top-word literal")
records.append({"name": "first_bit_u192", "words": words(u192_site, 3),
                "python_modeled_drop_w2_value": u192_site % (1 << 128)})
for x, y in (((1 << 96) - 1, (1 << 96) - 1), ((1 << 127) - 1, (1 << 64) + 1)):
    product = x * y
    require(product < R ** 3, "W1 valid product fits U192")
    records.append({"name": "u192_valid_product", "x": x, "y": y, "words": words(product, 3)})

valid_square = ((1 << 95) - 1) ** 2
valid_sum = 3 * valid_square
require(valid_sum < R ** 3, "W3 valid sum fits U192")
middle_word = words(valid_square, 3)[1]
require(2 * middle_word >= R, "W3 nonzero middle carry")
records.append({"name": "three_squares_valid", "one_square_words": words(valid_square, 3),
                "sum_words": words(valid_sum, 3), "second_add_middle_carry": (2 * middle_word) // R})
invalid_square = (3 * (1 << 94)) ** 2
require(invalid_square < R ** 3 and 2 * invalid_square >= R ** 3,
        "collective rejection must differ from per-square capacity")
require((1 << 96) ** 2 == R ** 3, "U192 exact capacity boundary")
require((1 << 100) * (1 << 100) >= R ** 3, "generic Rational128 crossing outside U192")
records.append({"name": "rejected_fixture_domains", "u192_boundary_product": R ** 3,
                "each_square_valid": True, "sum_of_two_squares": 2 * invalid_square,
                "rational_crossing": 1 << 200, "cpp_calls": 0})

gcd_cases = [(0, 0, 0), (0, (1 << 128) - 1, (1 << 128) - 1),
             ((1 << 128) - 1, 0, (1 << 128) - 1), (R + 1, R - 1, 1),
             (R - 1, R + 1, 1), (1 << 127, R, R), ((1 << 127) + 1, R, 1)]
for x, y, expected in gcd_cases:
    require(math.gcd(x, y) == expected, "independent GCD fixture")
records.append({"name": "gcd_literals", "cases": gcd_cases, "abs_i128_min": abs(I128_MIN)})
floor_cases = [(-5, 2, -3), (5, -2, -3), (-4, 2, -2), (I128_MIN, 1, I128_MIN)]
for numerator, denominator, expected in floor_cases:
    require(numerator // denominator == expected, "Python floor fixture")
require(I128_MIN // -1 > I128_MAX, "excluded signed quotient is unrepresentable")
records.append({"name": "floor_literals", "cases": floor_cases,
                "rejected_without_cpp_call": [[1, 0], [I128_MIN, -1]]})

# Current selftest's chosen U320 product has a zero fifth limb.
selftest_product = ((1 << 100) + 12345) * ((1 << 90) + 6789) * 3
require(words(selftest_product, 5)[4] == 0, "selftest does not reach U320.w4")
records.append({"name": "existing_selftest_range", "bit_length": selftest_product.bit_length(),
                "words": words(selftest_product, 5)})

print(json.dumps({"status": "fixture_arithmetic_verified", "records": records,
                  "product_execution": False, "mutant_execution": False,
                  "gcp": "not_used"}, ensure_ascii=False, indent=2))
