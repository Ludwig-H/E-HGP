#!/usr/bin/env python3
"""Audit exact level ordering and canonical equal-level batches independently."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import subprocess
import sys
from dataclasses import dataclass
from fractions import Fraction
from pathlib import Path
from typing import Iterable, Sequence

SEED = 0x4C4556454C324137
UINT64_MASK = (1 << 64) - 1
MAX_CANONICAL_ID = (1 << 53) - 1

EXPECTED_DEFAULT_COMPARISON_COUNT = 1536
EXPECTED_DEFAULT_BATCH_COUNT = 256
EXPECTED_DEFAULT_CASE_COUNT = 1792
EXPECTED_DEFAULT_BATCH_EMISSION_COUNT = 5120
EXPECTED_DEFAULT_CORPUS_SHA256 = (
    "59fc92204073609b622d86eea14f92efcb704e6bcba3daba7eb6c9dbf53a7e8b"
)
EXPECTED_DEFAULT_ORACLE_SHA256 = (
    "3cb912b811f649d8bf96be79047234b7f5a1a59e6ac846be3e1efc8bd512bbec"
)
EXPECTED_DEFAULT_DISTINCT_BATCH_LEVEL_COUNT = 1536
EXPECTED_DEFAULT_DUPLICATE_EMISSION_EXCESS = 768
EXPECTED_DEFAULT_MAXIMUM_EMISSION_COUNT = 2
EXPECTED_DEFAULT_MAXIMUM_EQUAL_LEVEL_ITEM_COUNT = 3
EXPECTED_DEFAULT_MAXIMUM_PROVENANCE_COUNT = 3
EXPECTED_DEFAULT_SIGN_HISTOGRAM = {
    "negative": 640,
    "positive": 640,
    "zero": 256,
}
EXPECTED_DEFAULT_COMPARISON_FAMILY_HISTOGRAM = {
    "cross_product_minus_one": 256,
    "cross_product_plus_one": 256,
    "exact_equality": 256,
    "exponent_extremes": 256,
    "general_rational": 256,
    "geometric_ulp": 256,
}
EXPECTED_DEFAULT_BATCH_METAMORPHISM_HISTOGRAM = {
    "chunk_reverse": 32,
    "identity": 32,
    "resume_prefix_13": 32,
    "resume_prefix_7": 32,
    "reverse": 32,
    "round_robin_chunks": 32,
    "splitmix_shuffle_a": 32,
    "support_and_arrival_permutation": 32,
}
EXPECTED_DEFAULT_MINIMAL_SUPPORT_SIZE_HISTOGRAM = {
    "1": 768,
    "2": 2304,
    "3": 1280,
    "4": 768,
}
EXPECTED_DEFAULT_SOURCE_SUPPORT_SIZE_HISTOGRAM = {
    "1": 768,
    "2": 1536,
    "3": 1536,
    "4": 1280,
}


class StableGenerator:
    """Versioned SplitMix64 source with platform-independent selection."""

    def __init__(self, seed: int) -> None:
        self.state = seed & UINT64_MASK

    def next_u64(self) -> int:
        self.state = (self.state + 0x9E3779B97F4A7C15) & UINT64_MASK
        value = self.state
        value = ((value ^ (value >> 30)) * 0xBF58476D1CE4E5B9) & UINT64_MASK
        value = ((value ^ (value >> 27)) * 0x94D049BB133111EB) & UINT64_MASK
        return (value ^ (value >> 31)) & UINT64_MASK

    def randbelow(self, bound: int) -> int:
        if bound <= 0:
            raise ValueError("StableGenerator bound must be positive")
        limit = (1 << 64) - ((1 << 64) % bound)
        while True:
            value = self.next_u64()
            if value < limit:
                return value % bound

    def randint(self, lower: int, upper: int) -> int:
        if lower > upper:
            raise ValueError("StableGenerator integer range is empty")
        return lower + self.randbelow(upper - lower + 1)


def canonical_json(value: object) -> str:
    return json.dumps(
        value,
        allow_nan=False,
        ensure_ascii=False,
        separators=(",", ":"),
        sort_keys=True,
    )


def counters(is_zero: bool) -> dict[str, int]:
    return {
        "cpu_multiprecision_certified": 1,
        "exact_zeros": int(is_zero),
        "expansion_certified": 0,
        "fp32_proposals": 0,
        "fp64_filtered_certified": 0,
        "remaining_unknown": 0,
    }


def level_record(level: Fraction) -> dict[str, str]:
    if level < 0:
        raise AssertionError("an ExactLevel cannot be negative")
    return {
        "denominator": str(level.denominator),
        "numerator": str(level.numerator),
        "schema_version": "2.0.0",
        "unit": "input_coordinate_unit_squared",
    }


def comparison_command(left: Fraction, right: Fraction) -> str:
    return (
        "compare_exact_levels "
        f"{left.numerator} {left.denominator} "
        f"{right.numerator} {right.denominator}"
    )


def comparison_expected(left: Fraction, right: Fraction) -> dict[str, object]:
    difference = left.numerator * right.denominator - right.numerator * left.denominator
    if difference < 0:
        sign = "negative"
        ordering = "less"
    elif difference > 0:
        sign = "positive"
        ordering = "greater"
    else:
        sign = "zero"
        ordering = "equal"
    return {
        "certification_stage": "cpu_multiprecision",
        "counters": counters(difference == 0),
        "cross_product_difference_exact": str(difference),
        "equal": difference == 0,
        "ordering": ordering,
        "predicate": "compare_exact_levels",
        "sign": sign,
    }


@dataclass(frozen=True)
class Emission:
    level: Fraction
    minimal_support_ids: tuple[int, ...]
    source_support_ids: tuple[int, ...]

    def __post_init__(self) -> None:
        require_support(self.minimal_support_ids, "minimal")
        require_support(self.source_support_ids, "source")
        if not set(self.minimal_support_ids).issubset(self.source_support_ids):
            raise ValueError("a source support must contain its minimal support")


def require_support(ids: Sequence[int], label: str) -> None:
    if not 1 <= len(ids) <= 4:
        raise ValueError(f"a {label} support must contain one through four IDs")
    if len(set(ids)) != len(ids):
        raise ValueError(f"a {label} support must contain unique IDs")
    if any(identifier < 0 or identifier > MAX_CANONICAL_ID for identifier in ids):
        raise ValueError(f"a {label} support ID is outside the canonical range")


def batch_command(emissions: Sequence[Emission]) -> str:
    tokens = ["canonical_level_batches", str(len(emissions))]
    for emission in emissions:
        tokens.extend(
            (
                str(emission.level.numerator),
                str(emission.level.denominator),
                str(len(emission.minimal_support_ids)),
                *(str(value) for value in emission.minimal_support_ids),
                str(len(emission.source_support_ids)),
                *(str(value) for value in emission.source_support_ids),
            )
        )
    return " ".join(tokens)


def batch_expected(emissions: Sequence[Emission]) -> dict[str, object]:
    levels_by_support: dict[tuple[int, ...], Fraction] = {}
    grouped: dict[Fraction, dict[tuple[int, ...], dict[tuple[int, ...], int]]] = {}
    for emission in emissions:
        minimal_support_ids = tuple(sorted(emission.minimal_support_ids))
        source_support_ids = tuple(sorted(emission.source_support_ids))
        previous = levels_by_support.setdefault(minimal_support_ids, emission.level)
        if previous != emission.level:
            raise AssertionError(
                "one minimal support cannot be emitted at two exact levels"
            )
        provenance = grouped.setdefault(emission.level, {}).setdefault(
            minimal_support_ids, {}
        )
        provenance[source_support_ids] = provenance.get(source_support_ids, 0) + 1

    batches: list[dict[str, object]] = []
    unique_emission_count = 0
    for level in sorted(grouped):
        supports_output: list[dict[str, object]] = []
        batch_emission_count = 0
        supports = sorted(grouped[level], key=lambda value: (len(value), value))
        for support in supports:
            source_counts = grouped[level][support]
            source_supports = sorted(
                source_counts, key=lambda value: (len(value), value)
            )
            support_emission_count = sum(source_counts.values())
            unique_emission_count += len(source_counts)
            batch_emission_count += support_emission_count
            supports_output.append(
                {
                    "emission_count": support_emission_count,
                    "minimal_support_ids": list(support),
                    "source_provenance": [
                        {
                            "emission_count": source_counts[source],
                            "source_support_ids": list(source),
                        }
                        for source in source_supports
                    ],
                    "squared_level_exact": level_record(level),
                }
            )
        batches.append(
            {
                "emission_count": batch_emission_count,
                "squared_level_exact": level_record(level),
                "supports": supports_output,
            }
        )
    return {
        "duplicate_emission_count": len(emissions) - unique_emission_count,
        "emission_count": len(emissions),
        "equal_level_batches": batches,
        "predicate": "canonical_level_batches",
        "unique_emission_count": unique_emission_count,
    }


@dataclass(frozen=True)
class Case:
    command: str
    expected: dict[str, object]
    predicate: str
    family: str
    metamorphism: str
    emissions: tuple[Emission, ...] = ()


def comparison_case(left: Fraction, right: Fraction, family: str) -> Case:
    return Case(
        comparison_command(left, right),
        comparison_expected(left, right),
        "compare_exact_levels",
        family,
        "direct",
    )


def batch_case(emissions: Sequence[Emission], family: str, metamorphism: str) -> Case:
    immutable = tuple(emissions)
    return Case(
        batch_command(immutable),
        batch_expected(immutable),
        "canonical_level_batches",
        family,
        metamorphism,
        immutable,
    )


def random_bigint(generator: StableGenerator, bit_count: int) -> int:
    if bit_count <= 0:
        raise ValueError("a random bigint must have at least one bit")
    word_count = (bit_count + 63) // 64
    value = 0
    for word_index in range(word_count):
        value |= generator.next_u64() << (64 * word_index)
    value &= (1 << bit_count) - 1
    value |= 1 << (bit_count - 1)
    return value


def random_level(generator: StableGenerator, maximum_bits: int = 1024) -> Fraction:
    numerator_bits = generator.randint(1, maximum_bits)
    denominator_bits = generator.randint(1, maximum_bits)
    numerator = random_bigint(generator, numerator_bits)
    denominator = random_bigint(generator, denominator_bits)
    if generator.randbelow(11) == 0:
        numerator = 0
    return Fraction(numerator, denominator)


def shuffled(values: Sequence[Emission], generator: StableGenerator) -> list[Emission]:
    output = list(values)
    for index in range(len(output) - 1, 0, -1):
        selected = generator.randbelow(index + 1)
        output[index], output[selected] = output[selected], output[index]
    return output


def permute_support_and_arrival(
    emissions: Sequence[Emission], generator: StableGenerator
) -> list[Emission]:
    output = shuffled(emissions, generator)
    return [
        (
            Emission(
                emission.level,
                tuple(reversed(emission.minimal_support_ids)),
                tuple(reversed(emission.source_support_ids)),
            )
            if index % 3 != 0
            else emission
        )
        for index, emission in enumerate(output)
    ]


def build_comparison_cases(generator: StableGenerator) -> list[Case]:
    cases: list[Case] = []
    known_equal_levels = (
        Fraction(),
        Fraction(1, 1 << 2150),
        Fraction(25, 64),
        Fraction(25, 9),
        Fraction(3),
        Fraction(139, 36),
        Fraction.from_float(sys.float_info.max) ** 2,
    )
    for index in range(256):
        level = (
            known_equal_levels[index % len(known_equal_levels)]
            if index % 3 == 0
            else random_level(generator)
        )
        cases.append(comparison_case(level, level, "exact_equality"))

    close_pairs: list[tuple[Fraction, Fraction]] = []
    for index in range(256):
        exponent = 8 + ((index * 4051) % 4089)
        integer = (1 << exponent) + 2 * index
        lower = Fraction(integer, integer + 1)
        upper = Fraction(integer + 1, integer + 2)
        if (
            lower.numerator * upper.denominator - upper.numerator * lower.denominator
            != -1
        ):
            raise AssertionError("the close-level construction lost its unit gap")
        close_pairs.append((lower, upper))
        cases.append(comparison_case(lower, upper, "cross_product_minus_one"))
    for lower, upper in close_pairs:
        cases.append(comparison_case(upper, lower, "cross_product_plus_one"))

    for _ in range(128):
        exponent = generator.randint(-1000, 1000)
        base = math.ldexp(1.0, exponent)
        below = math.nextafter(base, 0.0)
        above = math.nextafter(base, math.inf)
        cases.append(
            comparison_case(
                Fraction.from_float(below) ** 2,
                Fraction.from_float(base) ** 2,
                "geometric_ulp",
            )
        )
        cases.append(
            comparison_case(
                Fraction.from_float(above) ** 2,
                Fraction.from_float(base) ** 2,
                "geometric_ulp",
            )
        )

    for index in range(64):
        minimum_pair_level = Fraction(1, 1 << (2150 - index))
        cases.append(
            comparison_case(Fraction(), minimum_pair_level, "exponent_extremes")
        )
        cases.append(
            comparison_case(minimum_pair_level, Fraction(), "exponent_extremes")
        )
    maximum_level = Fraction.from_float(sys.float_info.max) ** 2
    for index in range(64):
        huge = maximum_level * (1 << (index % 17))
        neighbour = huge + Fraction(1, 1 << (index % 128))
        cases.append(comparison_case(huge, neighbour, "exponent_extremes"))
        cases.append(comparison_case(neighbour, huge, "exponent_extremes"))

    for _ in range(128):
        left = random_level(generator)
        right = random_level(generator)
        while left == right:
            right = random_level(generator)
        lower, upper = sorted((left, right))
        cases.append(comparison_case(lower, upper, "general_rational"))
        cases.append(comparison_case(upper, lower, "general_rational"))

    if len(cases) != EXPECTED_DEFAULT_COMPARISON_COUNT:
        raise AssertionError("the primitive level corpus has the wrong size")
    return cases


def make_emission(
    level: Fraction, minimal: Iterable[int], source: Iterable[int] | None = None
) -> Emission:
    minimal_ids = tuple(sorted(minimal))
    source_ids = minimal_ids if source is None else tuple(sorted(source))
    return Emission(level, minimal_ids, source_ids)


def base_chain(index: int) -> tuple[Emission, ...]:
    # These IDs exercise canonical provenance only. The independent levels do
    # not claim that this helper enumerated or certified ambient miniballs.
    if index == 0:
        base = 0
    elif index == 31:
        base = MAX_CANONICAL_ID - 100
    else:
        base = 1_000_000 + index * 100
    zero = Fraction()
    below_one = Fraction.from_float(math.nextafter(1.0, 0.0)) ** 2
    unit = Fraction(1)
    scaled_level = Fraction(25, 64)
    nondyadic_level = Fraction(25, 9)
    high_level = Fraction(1000 + 2 * index, 3)

    singleton_a = (base,)
    singleton_b = (base + 1,)
    below_pair = (base + 2, base + 3)
    unit_pair = (base + 4, base + 5)
    unit_triangle = (base + 8, base + 9, base + 10)
    unit_tetrahedron = (base + 12, base + 13, base + 14, base + 15)
    scaled_pair = (base + 16, base + 17)
    scaled_triangle = (base + 18, base + 19, base + 20)
    scaled_tetrahedron = (base + 21, base + 22, base + 23, base + 24)
    nondyadic_triangle = (base + 25, base + 26, base + 27)
    translated_triangle = (base + 28, base + 29, base + 30)
    high_pair = (base + 31, base + 32)
    high_tetrahedron = (base + 33, base + 34, base + 35, base + 36)

    return (
        make_emission(zero, singleton_a),
        make_emission(zero, singleton_a),
        make_emission(zero, singleton_b),
        make_emission(below_one, below_pair),
        make_emission(unit, unit_pair),
        make_emission(unit, unit_pair),
        make_emission(unit, unit_pair, (*unit_pair, base + 6)),
        make_emission(unit, unit_pair, (*unit_pair, base + 6, base + 7)),
        make_emission(unit, unit_triangle),
        make_emission(unit, unit_triangle, (*unit_triangle, base + 11)),
        make_emission(unit, unit_tetrahedron),
        make_emission(scaled_level, scaled_pair),
        make_emission(scaled_level, scaled_triangle),
        make_emission(scaled_level, scaled_tetrahedron),
        make_emission(nondyadic_level, nondyadic_triangle),
        make_emission(nondyadic_level, translated_triangle),
        make_emission(high_level, high_pair),
        make_emission(high_level, high_pair),
        make_emission(high_level, high_pair, (*high_pair, base + 37)),
        make_emission(high_level, high_tetrahedron),
    )


def chunk_reverse(emissions: Sequence[Emission]) -> list[Emission]:
    boundaries = (0, 3, 8, 12, len(emissions))
    chunks = [
        list(emissions[boundaries[index] : boundaries[index + 1]])
        for index in range(len(boundaries) - 1)
    ]
    return [emission for chunk in reversed(chunks) for emission in chunk]


def round_robin_chunks(emissions: Sequence[Emission]) -> list[Emission]:
    chunks = [[], [], []]
    for index, emission in enumerate(emissions):
        chunks[index % len(chunks)].append(emission)
    return [emission for chunk in chunks for emission in chunk]


def canonical_emission_order(emissions: Sequence[Emission]) -> list[Emission]:
    return sorted(
        emissions,
        key=lambda emission: (
            emission.level,
            len(emission.minimal_support_ids),
            emission.minimal_support_ids,
            len(emission.source_support_ids),
            emission.source_support_ids,
        ),
    )


def resumed(emissions: Sequence[Emission], prefix_size: int) -> list[Emission]:
    prefix = canonical_emission_order(emissions[:prefix_size])
    return [*prefix, *emissions[prefix_size:]]


def build_batch_cases(generator: StableGenerator) -> list[Case]:
    cases: list[Case] = []
    for chain_index in range(32):
        chain = base_chain(chain_index)
        variants = (
            ("identity", list(chain)),
            ("reverse", list(reversed(chain))),
            ("splitmix_shuffle_a", shuffled(chain, generator)),
            (
                "support_and_arrival_permutation",
                permute_support_and_arrival(chain, generator),
            ),
            ("chunk_reverse", chunk_reverse(chain)),
            ("round_robin_chunks", round_robin_chunks(chain)),
            ("resume_prefix_7", resumed(chain, 7)),
            ("resume_prefix_13", resumed(chain, 13)),
        )
        expected = batch_expected(chain)
        for metamorphism, variant in variants:
            case = batch_case(variant, "mixed_support_chain", metamorphism)
            if case.expected != expected:
                raise AssertionError(
                    f"batch metamorphism {metamorphism} changed canonical output"
                )
            cases.append(case)
    if len(cases) != EXPECTED_DEFAULT_BATCH_COUNT:
        raise AssertionError("the canonical batch corpus has the wrong size")
    return cases


def build_cases() -> list[Case]:
    generator = StableGenerator(SEED)
    cases = [
        *build_comparison_cases(generator),
        *build_batch_cases(generator),
    ]
    if len(cases) != EXPECTED_DEFAULT_CASE_COUNT:
        raise AssertionError("the complete level-order corpus has the wrong size")
    return cases


def run_batch(
    executable: Path, corpus: str, arguments: Sequence[str], timeout: int
) -> str:
    completed = subprocess.run(
        [str(executable), *arguments, "--batch"],
        input=corpus,
        capture_output=True,
        encoding="utf-8",
        timeout=timeout,
    )
    if completed.returncode != 0:
        mode = " ".join(arguments) or "normal"
        raise AssertionError(
            f"native level batch {mode} failed closed: {completed.stderr.strip()}"
        )
    if completed.stderr:
        raise AssertionError("native level batch unexpectedly wrote to stderr")
    return completed.stdout


def audit_output(output: str, cases: Sequence[Case]) -> None:
    lines = output.splitlines(keepends=True)
    if len(lines) != len(cases):
        raise AssertionError(
            f"native level batch returned {len(lines)} lines for {len(cases)} cases"
        )
    for index, (line, case) in enumerate(zip(lines, cases)):
        expected = canonical_json(case.expected) + "\n"
        if line == expected:
            continue
        try:
            observed = json.loads(line)
        except json.JSONDecodeError as error:
            raise AssertionError(
                f"level-order case {index} returned invalid JSON: {error}"
            ) from error
        raise AssertionError(
            f"level-order case {index} ({case.predicate}/{case.family}/"
            f"{case.metamorphism}) differs from the Fraction oracle: "
            f"expected={expected.strip()}, observed={canonical_json(observed)}"
        )


def increment(histogram: dict[str, int], key: str, amount: int = 1) -> None:
    histogram[key] = histogram.get(key, 0) + amount


def corpus_statistics(cases: Sequence[Case]) -> dict[str, object]:
    sign_histogram: dict[str, int] = {}
    family_histogram: dict[str, int] = {}
    metamorphism_histogram: dict[str, int] = {}
    minimal_size_histogram = {str(size): 0 for size in range(1, 5)}
    source_size_histogram = {str(size): 0 for size in range(1, 5)}
    batch_emission_count = 0
    distinct_batch_level_count = 0
    duplicate_emission_excess = 0
    maximum_equal_level_item_count = 0
    maximum_provenance_count = 0
    maximum_emission_count = 0

    for case in cases:
        if case.predicate == "compare_exact_levels":
            increment(sign_histogram, str(case.expected["sign"]))
            increment(family_histogram, case.family)
            continue
        increment(metamorphism_histogram, case.metamorphism)
        batch_emission_count += len(case.emissions)
        for emission in case.emissions:
            minimal_size_histogram[str(len(emission.minimal_support_ids))] += 1
            source_size_histogram[str(len(emission.source_support_ids))] += 1
        batches = case.expected["equal_level_batches"]
        assert isinstance(batches, list)
        distinct_batch_level_count += len(batches)
        provenance_record_count = 0
        for batch in batches:
            assert isinstance(batch, dict)
            supports = batch["supports"]
            assert isinstance(supports, list)
            maximum_equal_level_item_count = max(
                maximum_equal_level_item_count, len(supports)
            )
            for support in supports:
                assert isinstance(support, dict)
                provenance = support["source_provenance"]
                assert isinstance(provenance, list)
                provenance_record_count += len(provenance)
                maximum_provenance_count = max(
                    maximum_provenance_count, len(provenance)
                )
                for record in provenance:
                    assert isinstance(record, dict)
                    maximum_emission_count = max(
                        maximum_emission_count, int(record["emission_count"])
                    )
        if provenance_record_count != case.expected["unique_emission_count"]:
            raise AssertionError(
                "the oracle unique-emission accounting is inconsistent"
            )
        duplicate_emission_excess += int(case.expected["duplicate_emission_count"])

    return {
        "batch_emission_count": batch_emission_count,
        "batch_metamorphism_histogram": metamorphism_histogram,
        "comparison_family_histogram": family_histogram,
        "distinct_batch_level_count": distinct_batch_level_count,
        "duplicate_emission_excess": duplicate_emission_excess,
        "maximum_emission_count": maximum_emission_count,
        "maximum_equal_level_item_count": maximum_equal_level_item_count,
        "maximum_provenance_count": maximum_provenance_count,
        "minimal_support_size_histogram": minimal_size_histogram,
        "sign_histogram": sign_histogram,
        "source_support_size_histogram": source_size_histogram,
    }


def require_frozen_statistics(statistics: dict[str, object]) -> None:
    expected_pairs = (
        ("sign_histogram", EXPECTED_DEFAULT_SIGN_HISTOGRAM),
        (
            "comparison_family_histogram",
            EXPECTED_DEFAULT_COMPARISON_FAMILY_HISTOGRAM,
        ),
        (
            "batch_metamorphism_histogram",
            EXPECTED_DEFAULT_BATCH_METAMORPHISM_HISTOGRAM,
        ),
        (
            "minimal_support_size_histogram",
            EXPECTED_DEFAULT_MINIMAL_SUPPORT_SIZE_HISTOGRAM,
        ),
        (
            "source_support_size_histogram",
            EXPECTED_DEFAULT_SOURCE_SUPPORT_SIZE_HISTOGRAM,
        ),
    )
    for name, expected in expected_pairs:
        if statistics[name] != expected:
            raise AssertionError(
                f"the default {name} changed without a generator-version update"
            )
    if statistics["batch_emission_count"] != EXPECTED_DEFAULT_BATCH_EMISSION_COUNT:
        raise AssertionError(
            "the default batch emission count changed without a "
            "generator-version update"
        )
    expected_scalars = (
        (
            "distinct_batch_level_count",
            EXPECTED_DEFAULT_DISTINCT_BATCH_LEVEL_COUNT,
        ),
        (
            "duplicate_emission_excess",
            EXPECTED_DEFAULT_DUPLICATE_EMISSION_EXCESS,
        ),
        ("maximum_emission_count", EXPECTED_DEFAULT_MAXIMUM_EMISSION_COUNT),
        (
            "maximum_equal_level_item_count",
            EXPECTED_DEFAULT_MAXIMUM_EQUAL_LEVEL_ITEM_COUNT,
        ),
        (
            "maximum_provenance_count",
            EXPECTED_DEFAULT_MAXIMUM_PROVENANCE_COUNT,
        ),
    )
    for name, expected in expected_scalars:
        if statistics[name] != expected:
            raise AssertionError(
                f"the default {name} changed without a generator-version update"
            )


def positive_count(value: str) -> int:
    parsed = int(value)
    if parsed <= 0:
        raise argparse.ArgumentTypeError("the timeout must be positive")
    return parsed


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native_replay", type=Path)
    parser.add_argument("--timeout-seconds", type=positive_count, default=180)
    arguments = parser.parse_args()

    cases = build_cases()
    statistics = corpus_statistics(cases)
    require_frozen_statistics(statistics)
    corpus = "".join(case.command + "\n" for case in cases)
    oracle = "".join(canonical_json(case.expected) + "\n" for case in cases)
    corpus_hash = hashlib.sha256(corpus.encode("ascii")).hexdigest()
    oracle_hash = hashlib.sha256(oracle.encode("utf-8")).hexdigest()
    if EXPECTED_DEFAULT_CORPUS_SHA256 and corpus_hash != EXPECTED_DEFAULT_CORPUS_SHA256:
        raise AssertionError(
            "the default level corpus changed without a generator-version update"
        )
    if EXPECTED_DEFAULT_ORACLE_SHA256 and oracle_hash != EXPECTED_DEFAULT_ORACLE_SHA256:
        raise AssertionError(
            "the default level oracle changed without a generator-version update"
        )

    normal_output = run_batch(
        arguments.native_replay, corpus, (), arguments.timeout_seconds
    )
    multiprecision_output = run_batch(
        arguments.native_replay,
        corpus,
        ("--multiprecision-only",),
        arguments.timeout_seconds,
    )
    if normal_output != multiprecision_output:
        raise AssertionError("exact level outputs differ under --multiprecision-only")
    audit_output(normal_output, cases)

    print(
        canonical_json(
            {
                "batch_case_count": EXPECTED_DEFAULT_BATCH_COUNT,
                "case_count": len(cases),
                "command_corpus_sha256": corpus_hash,
                "comparison_case_count": EXPECTED_DEFAULT_COMPARISON_COUNT,
                "generator": "level-order-dyadic-splitmix64-v1",
                "multiprecision_only_byte_identical": True,
                "oracle_sha256": oracle_hash,
                "seed": f"0x{SEED:016x}",
                **statistics,
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
