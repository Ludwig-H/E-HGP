#!/usr/bin/env python3
"""Run the resumable, exact-sign campaign used to close MorseHGP3D phase 2A."""

from __future__ import annotations

import argparse
import fcntl
import hashlib
import json
import math
import os
import platform
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, replace
from pathlib import Path, PurePosixPath
from typing import Any, Callable, Iterable, Iterator, Sequence, TextIO

SCHEMA_VERSION = "1.0.0"
CONFIG_KIND = "morsehgp3d_phase2a_predicate_campaign_config"
MANIFEST_KIND = "morsehgp3d_phase2a_predicate_campaign_checkpoint"
CHUNK_KIND = "morsehgp3d_phase2a_predicate_campaign_chunk"
CERTIFICATE_KIND = "morsehgp3d_phase2a_predicate_campaign_certificate"
REPRO_KIND = "morsehgp3d_phase2a_predicate_campaign_repro"
ORIGINAL_REPRO_KIND = "morsehgp3d_phase2a_predicate_campaign_original_repro"
PRECOMPUTE_KIND = "morsehgp3d_phase2a_predicate_campaign_precomputed_roots"
GENERATOR_ID = "splitmix64-counter-v1"
ORACLE_ID = "independent-integer-dyadic-v1"
NATIVE_INTERFACE = "decision-only-batch-v1"
ORDERED_ROOT_ALGORITHM = "sha256-record-chain-v1"
MANIFEST_NAME = "checkpoint.json"
RESULT_NAME = "result.json"
FAILPOINT_ENVIRONMENT = "MORSEHGP3D_PHASE2A_CAMPAIGN_FAIL_AFTER"
UINT64_MASK = (1 << 64) - 1
SPLITMIX_INCREMENT = 0x9E3779B97F4A7C15
COUNTER_DOMAIN_SIZE = 128
MAX_CHUNK_BASE_CASES = 9_600
MIN_PRODUCTION_BASE_CASES = 10_000_000
SMOKE_MINIMUM = 300
SMOKE_MAXIMUM = 10_000
HASH_FIELDS = {"relative_path", "sha256", "size_bytes"}
PREDICATES = (
    "compare_squared_distances",
    "orientation_3d",
    "power_bisector_side",
)
STRATA = (
    "well_conditioned",
    "cancellation",
    "subnormal",
    "extreme_exponents",
    "large_offsets",
    "near_coplanar",
    "near_cospherical",
    "exact_equalities",
)
STRATUM_PERCENTAGES = (93, 1, 1, 1, 1, 1, 1, 1)
SIGNS = ("negative", "zero", "positive")
STAGES = ("cpu_multiprecision", "expansion", "fp64_filtered")
PROPER_ROTATION = "proper_signed_axis_permutation"
IMPROPER_REFLECTION = "improper_signed_axis_permutation"
DYADIC_TRANSLATION = "nonzero_exact_dyadic_translation"
POWER_OF_TWO_SCALING = "exact_power_of_two_scaling"
PREDICATE_SYMMETRY = "predicate_argument_symmetry"
METAMORPHISMS = (
    PROPER_ROTATION,
    IMPROPER_REFLECTION,
    DYADIC_TRANSLATION,
    POWER_OF_TWO_SCALING,
    PREDICATE_SYMMETRY,
)
RELATIONS = ("same_sign", "opposite_sign")
POWER_WITNESS_DENOMINATORS = (1, 2, 3, 5, 7, 8)
ROOT_DOMAINS = {
    "corpus": b"MorseHGP3D/phase2a-corpus-chain-v1/",
    "oracle": b"MorseHGP3D/phase2a-oracle-chain-v1/",
    "output": b"MorseHGP3D/phase2a-output-chain-v1/",
}
COUNTER_FIELDS = {
    "cpu_multiprecision_certified",
    "exact_zeros",
    "expansion_certified",
    "fp32_proposals",
    "fp64_filtered_certified",
    "remaining_unknown",
}
NATIVE_RESULT_FIELDS = {"certification_stage", "counters", "predicate", "sign"}


PointWords = tuple[str, str, str]
Dyadic = tuple[int, int]  # numerator times two to the exponent


def canonical_json(value: object) -> str:
    return json.dumps(
        value,
        allow_nan=False,
        ensure_ascii=False,
        separators=(",", ":"),
        sort_keys=True,
    )


def canonical_bytes(value: object) -> bytes:
    return (canonical_json(value) + "\n").encode("utf-8")


def strict_json_loads(text: str) -> Any:
    def reject_duplicates(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
        result: dict[str, Any] = {}
        for key, value in pairs:
            if key in result:
                raise ValueError(f"duplicate JSON key: {key}")
            result[key] = value
        return result

    def reject_nonfinite(value: str) -> None:
        raise ValueError(f"non-finite JSON number: {value}")

    return json.loads(
        text,
        object_pairs_hook=reject_duplicates,
        parse_constant=reject_nonfinite,
    )


def require_fields(value: Any, fields: set[str], label: str) -> dict[str, Any]:
    if not isinstance(value, dict) or set(value) != fields:
        raise ValueError(f"{label} has missing or unknown fields")
    return value


def require_integer(
    value: Any, label: str, *, minimum: int = 0, maximum: int | None = None
) -> int:
    if type(value) is not int or value < minimum:
        raise ValueError(f"{label} is not a valid integer")
    if maximum is not None and value > maximum:
        raise ValueError(f"{label} exceeds its closed bound")
    return value


def sha256_bytes(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while block := stream.read(1024 * 1024):
            digest.update(block)
    return digest.hexdigest()


def canonical_hash(value: object) -> str:
    return sha256_bytes(canonical_json(value).encode("utf-8"))


def initial_ordered_roots() -> dict[str, str]:
    return {
        name: sha256_bytes(domain + b"initial") for name, domain in ROOT_DOMAINS.items()
    }


def extend_ordered_root(name: str, root: str, row: bytes) -> str:
    if name not in ROOT_DOMAINS or len(root) != 64:
        raise ValueError("an ordered scientific root has an invalid identity")
    return sha256_bytes(
        ROOT_DOMAINS[name] + bytes.fromhex(root) + hashlib.sha256(row).digest()
    )


def require_regular_file(path: Path, label: str) -> None:
    if path.is_symlink() or not path.is_file():
        raise ValueError(f"{label} is not a regular file")


def fsync_directory(path: Path) -> None:
    descriptor = os.open(path, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
    try:
        os.fsync(descriptor)
    finally:
        os.close(descriptor)


def atomic_publish(path: Path, content: bytes) -> None:
    descriptor, temporary_name = tempfile.mkstemp(
        dir=path.parent, prefix=f".{path.name}.", suffix=".tmp"
    )
    temporary = Path(temporary_name)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(content)
            stream.flush()
            os.fsync(stream.fileno())
        if sha256_file(temporary) != sha256_bytes(content):
            raise OSError(f"temporary artifact verification failed: {path.name}")
        os.replace(temporary, path)
        fsync_directory(path.parent)
    finally:
        if temporary.exists():
            temporary.unlink()


def maybe_fail_after(label: str) -> None:
    if os.environ.get(FAILPOINT_ENVIRONMENT) == label:
        raise RuntimeError(f"injected campaign failure after {label}")


def load_canonical_file(path: Path, label: str) -> tuple[Any, bytes]:
    require_regular_file(path, label)
    content = path.read_bytes()
    try:
        value = strict_json_loads(content.decode("utf-8"))
    except UnicodeDecodeError as error:
        raise ValueError(f"{label} is not UTF-8") from error
    if content != canonical_bytes(value):
        raise ValueError(f"{label} is not canonical JSON")
    return value, content


def splitmix64(seed: int, counter: int) -> int:
    value = (seed + (counter + 1) * SPLITMIX_INCREMENT) & UINT64_MASK
    value = ((value ^ (value >> 30)) * 0xBF58476D1CE4E5B9) & UINT64_MASK
    value = ((value ^ (value >> 27)) * 0x94D049BB133111EB) & UINT64_MASK
    return (value ^ (value >> 31)) & UINT64_MASK


def counter_value(seed: int, base_index: int, lane: int) -> int:
    if not 0 <= lane < COUNTER_DOMAIN_SIZE:
        raise ValueError("a SplitMix64 counter lane escaped its frozen domain")
    return splitmix64(seed, base_index * COUNTER_DOMAIN_SIZE + lane)


def signed_small(seed: int, base_index: int, lane: int, bound: int) -> int:
    value = counter_value(seed, base_index, lane)
    return int(value % (2 * bound + 1)) - bound


def word_from_float(value: float) -> str:
    if not math.isfinite(value):
        raise ValueError("the generator attempted to serialize a non-finite coordinate")
    return struct.pack(">d", value).hex()


def scaled_word(integer: int, exponent: int = 0) -> str:
    value = math.ldexp(float(integer), exponent)
    if not math.isfinite(value):
        raise ValueError("the generator exceeded binary64")
    return word_from_float(value)


ZERO = scaled_word(0)
ONE = scaled_word(1)
NEGATIVE_ONE = scaled_word(-1)
TWO = scaled_word(2)
MINIMUM_SUBNORMAL = "0000000000000001"


def dyadic_from_word(word: str) -> Dyadic:
    if len(word) != 16:
        raise ValueError("a generated binary64 word is not canonical")
    bits = int(word, 16)
    sign = -1 if bits >> 63 else 1
    exponent_bits = (bits >> 52) & 0x7FF
    fraction_bits = bits & ((1 << 52) - 1)
    if exponent_bits == 0x7FF:
        raise ValueError("the exact oracle refuses non-finite binary64")
    if exponent_bits == 0:
        return (sign * fraction_bits, -1074) if fraction_bits else (0, 0)
    return (sign * ((1 << 52) | fraction_bits), exponent_bits - 1023 - 52)


def dyadic_normalize(value: Dyadic) -> Dyadic:
    numerator, exponent = value
    if numerator == 0:
        return (0, 0)
    trailing = (abs(numerator) & -abs(numerator)).bit_length() - 1
    return numerator >> trailing, exponent + trailing


def dyadic_add(left: Dyadic, right: Dyadic) -> Dyadic:
    if left[0] == 0:
        return right
    if right[0] == 0:
        return left
    exponent = min(left[1], right[1])
    return dyadic_normalize(
        (
            (left[0] << (left[1] - exponent)) + (right[0] << (right[1] - exponent)),
            exponent,
        )
    )


def dyadic_negate(value: Dyadic) -> Dyadic:
    return -value[0], value[1]


def dyadic_subtract(left: Dyadic, right: Dyadic) -> Dyadic:
    return dyadic_add(left, dyadic_negate(right))


def dyadic_multiply(left: Dyadic, right: Dyadic) -> Dyadic:
    return dyadic_normalize((left[0] * right[0], left[1] + right[1]))


def dyadic_sum(values: Iterable[Dyadic]) -> Dyadic:
    result = (0, 0)
    for value in values:
        result = dyadic_add(result, value)
    return result


def dyadic_sign(value: Dyadic) -> str:
    return "negative" if value[0] < 0 else "zero" if value[0] == 0 else "positive"


def exact_point(point: PointWords) -> tuple[Dyadic, Dyadic, Dyadic]:
    return tuple(dyadic_from_word(word) for word in point)  # type: ignore[return-value]


def stratum_for_index(base_index: int) -> str:
    slot = base_index % 100
    if slot < 93:
        return STRATA[0]
    return STRATA[slot - 92]


def predicate_for_index(base_index: int) -> str:
    return PREDICATES[base_index % len(PREDICATES)]


def determinant_sign(permutation: tuple[int, int, int]) -> int:
    inversions = sum(
        permutation[left] > permutation[right]
        for left in range(3)
        for right in range(left + 1, 3)
    )
    return -1 if inversions % 2 else 1


ROTATIONS = tuple(
    (permutation, signs)
    for permutation in (
        (0, 1, 2),
        (0, 2, 1),
        (1, 0, 2),
        (1, 2, 0),
        (2, 0, 1),
        (2, 1, 0),
    )
    for signs in (
        (1, 1, 1),
        (1, -1, -1),
        (-1, 1, -1),
        (-1, -1, 1),
        (-1, 1, 1),
        (1, -1, 1),
        (1, 1, -1),
        (-1, -1, -1),
    )
    if determinant_sign(permutation) * math.prod(signs) == 1
    and (permutation, signs) != ((0, 1, 2), (1, 1, 1))
)

REFLECTIONS = tuple(
    (permutation, signs)
    for permutation in (
        (0, 1, 2),
        (0, 2, 1),
        (1, 0, 2),
        (1, 2, 0),
        (2, 0, 1),
        (2, 1, 0),
    )
    for signs in (
        (1, 1, 1),
        (1, -1, -1),
        (-1, 1, -1),
        (-1, -1, 1),
        (-1, 1, 1),
        (1, -1, 1),
        (1, 1, -1),
        (-1, -1, -1),
    )
    if determinant_sign(permutation) * math.prod(signs) == -1
)

METAMORPHISM_CATALOG = (
    {
        "id": PROPER_ROTATION,
        "scope": "all_strata",
        "sign_rule": "same",
    },
    {
        "id": IMPROPER_REFLECTION,
        "scope": "all_strata",
        "sign_rule": "opposite_for_orientation_otherwise_same",
    },
    {
        "id": DYADIC_TRANSLATION,
        "scope": "well_conditioned",
        "sign_rule": "same",
    },
    {
        "id": POWER_OF_TWO_SCALING,
        "scope": "well_conditioned",
        "sign_rule": "same",
    },
    {
        "id": PREDICATE_SYMMETRY,
        "scope": "well_conditioned",
        "sign_rule": "opposite",
    },
)


def negate_word(word: str) -> str:
    return f"{int(word, 16) ^ (1 << 63):016x}"


def transform_point(
    point: PointWords, permutation: tuple[int, int, int], signs: tuple[int, int, int]
) -> PointWords:
    return tuple(
        (
            point[permutation[axis]]
            if signs[axis] > 0
            else negate_word(point[permutation[axis]])
        )
        for axis in range(3)
    )  # type: ignore[return-value]


@dataclass(frozen=True)
class PredicateCase:
    base_index: int
    predicate: str
    stratum: str
    points: tuple[PointWords, ...]
    witness: tuple[int, int, int, int] | None = None
    r_ids: tuple[int, ...] = ()
    q_ids: tuple[int, ...] = ()
    relation: str = "base"

    def command(self) -> str:
        if self.predicate != "power_bisector_side":
            return " ".join(
                [self.predicate, *(word for point in self.points for word in point)]
            )
        if self.witness is None:
            raise ValueError("a generated H_RQ case has no witness")
        return " ".join(
            [
                self.predicate,
                *(str(value) for value in self.witness),
                str(len(self.points)),
                *(word for point in self.points for word in point),
                str(len(self.r_ids)),
                *(str(identifier) for identifier in self.r_ids),
                str(len(self.q_ids)),
                *(str(identifier) for identifier in self.q_ids),
            ]
        )


def opposite_sign(value: str) -> str:
    return (
        "positive"
        if value == "negative"
        else "negative" if value == "positive" else "zero"
    )


def expected_metamorphic_sign(base: PredicateCase, metamorphism: str) -> str:
    base_sign = oracle_sign(base)
    if metamorphism == PREDICATE_SYMMETRY or (
        metamorphism == IMPROPER_REFLECTION and base.predicate == "orientation_3d"
    ):
        return opposite_sign(base_sign)
    return base_sign


def canonical_witness(values: tuple[int, int, int, int]) -> tuple[int, int, int, int]:
    denominator = values[3]
    divisor = math.gcd(
        denominator,
        math.gcd(abs(values[0]), math.gcd(abs(values[1]), abs(values[2]))),
    )
    return (
        values[0] // divisor,
        values[1] // divisor,
        values[2] // divisor,
        denominator // divisor,
    )


def axis_transform_case(
    case: PredicateCase,
    transform: tuple[tuple[int, int, int], tuple[int, int, int]],
    relation: str,
) -> PredicateCase:
    permutation, signs = transform
    witness = case.witness
    if witness is not None:
        numerators = witness[:3]
        transformed = tuple(
            numerators[permutation[axis]] * signs[axis] for axis in range(3)
        )
        witness = canonical_witness((*transformed, witness[3]))
    return replace(
        case,
        points=tuple(
            transform_point(point, permutation, signs) for point in case.points
        ),
        witness=witness,
        relation=relation,
    )


def translated_word(word: str, translation: int) -> str:
    value = struct.unpack(">d", bytes.fromhex(word))[0]
    transformed = word_from_float(value + translation)
    if dyadic_normalize(dyadic_from_word(transformed)) != dyadic_normalize(
        dyadic_add(dyadic_from_word(word), (translation, 0))
    ):
        raise ValueError("a generated dyadic translation rounded")
    return transformed


def scaled_binary64_word(word: str, exponent: int) -> str:
    value = struct.unpack(">d", bytes.fromhex(word))[0]
    transformed = word_from_float(math.ldexp(value, exponent))
    if dyadic_normalize(dyadic_from_word(transformed)) != dyadic_normalize(
        dyadic_multiply(dyadic_from_word(word), (1, exponent))
    ):
        raise ValueError("a generated power-of-two homothety rounded")
    return transformed


def metamorphic_case(
    base: PredicateCase, seed: int, metamorphism: str
) -> PredicateCase:
    if metamorphism == PROPER_ROTATION:
        transform = ROTATIONS[
            counter_value(seed, base.base_index, 126) % len(ROTATIONS)
        ]
        result = axis_transform_case(base, transform, metamorphism)
    elif metamorphism == IMPROPER_REFLECTION:
        transform = REFLECTIONS[
            counter_value(seed, base.base_index, 125) % len(REFLECTIONS)
        ]
        result = axis_transform_case(base, transform, metamorphism)
    elif metamorphism == DYADIC_TRANSLATION:
        if base.stratum != "well_conditioned":
            raise ValueError("dyadic translation escaped its frozen stratum")
        translations = tuple(
            (1 if counter_value(seed, base.base_index, 121 + axis) & 1 else -1)
            * (1 << (12 + axis))
            for axis in range(3)
        )
        witness = base.witness
        if witness is not None:
            witness = canonical_witness(
                (
                    witness[0] + translations[0] * witness[3],
                    witness[1] + translations[1] * witness[3],
                    witness[2] + translations[2] * witness[3],
                    witness[3],
                )
            )
        result = replace(
            base,
            points=tuple(
                tuple(
                    translated_word(word, translations[axis])
                    for axis, word in enumerate(point)
                )
                for point in base.points
            ),
            witness=witness,
            relation=metamorphism,
        )
    elif metamorphism == POWER_OF_TWO_SCALING:
        if base.stratum != "well_conditioned":
            raise ValueError("power-of-two scaling escaped its frozen stratum")
        exponent = 1 + counter_value(seed, base.base_index, 124) % 3
        witness = base.witness
        if witness is not None:
            witness = canonical_witness(
                (
                    witness[0] << exponent,
                    witness[1] << exponent,
                    witness[2] << exponent,
                    witness[3],
                )
            )
        result = replace(
            base,
            points=tuple(
                tuple(scaled_binary64_word(word, exponent) for word in point)
                for point in base.points
            ),
            witness=witness,
            relation=metamorphism,
        )
    elif metamorphism == PREDICATE_SYMMETRY:
        if base.stratum != "well_conditioned":
            raise ValueError("predicate symmetry escaped its frozen stratum")
        if base.predicate == "compare_squared_distances":
            result = replace(
                base,
                points=(base.points[0], base.points[2], base.points[1]),
                relation=metamorphism,
            )
        elif base.predicate == "orientation_3d":
            result = replace(
                base,
                points=(base.points[0], base.points[2], base.points[1], base.points[3]),
                relation=metamorphism,
            )
        else:
            result = replace(
                base,
                r_ids=base.q_ids,
                q_ids=base.r_ids,
                relation=metamorphism,
            )
    else:
        raise ValueError("the requested metamorphism is outside the frozen catalog")
    if oracle_sign(result) != expected_metamorphic_sign(base, metamorphism):
        raise ValueError("the generated metamorphism violated its exact sign relation")
    return result


def point_from_integers(x: int, y: int, z: int, exponent: int = 0) -> PointWords:
    return scaled_word(x, exponent), scaled_word(y, exponent), scaled_word(z, exponent)


def randomized_point(
    seed: int, base_index: int, lane: int, bound: int = 1024
) -> PointWords:
    return point_from_integers(
        signed_small(seed, base_index, lane, bound),
        signed_small(seed, base_index, lane + 1, bound),
        signed_small(seed, base_index, lane + 2, bound),
    )


def distance_points(seed: int, base_index: int, stratum: str) -> tuple[PointWords, ...]:
    if stratum == "well_conditioned":
        witness = randomized_point(seed, base_index, 0, 32)
        left = randomized_point(seed, base_index, 3, 1024)
        right = randomized_point(seed, base_index, 6, 1024)
        return witness, left, right
    if stratum == "cancellation":
        return (
            point_from_integers(0, 0, 0),
            ("3fefffffffffffff", ZERO, ZERO),
            point_from_integers(1, 0, 0),
        )
    if stratum == "subnormal":
        return (
            point_from_integers(0, 0, 0),
            (MINIMUM_SUBNORMAL, ZERO, ZERO),
            point_from_integers(0, 0, 0),
        )
    if stratum == "extreme_exponents":
        return (
            point_from_integers(0, 0, 0),
            point_from_integers(1, 0, 0, 500),
            point_from_integers(1, 0, 0, 499),
        )
    if stratum == "large_offsets":
        base = 1 << 40
        return (
            point_from_integers(base, base, base),
            point_from_integers(base + 1, base, base),
            point_from_integers(base + 2, base, base),
        )
    if stratum == "near_coplanar":
        return (
            point_from_integers(0, 0, 0),
            point_from_integers(1, 1, 0),
            (ONE, ONE, scaled_word(1, -500)),
        )
    if stratum == "near_cospherical":
        return (
            point_from_integers(0, 0, 0),
            point_from_integers(1, 0, 0),
            (ONE, scaled_word(1, -52), ZERO),
        )
    return (
        point_from_integers(0, 0, 0),
        point_from_integers(-1, 0, 0),
        point_from_integers(1, 0, 0),
    )


def orientation_points(
    seed: int, base_index: int, stratum: str
) -> tuple[PointWords, ...]:
    if stratum == "well_conditioned":
        while True:
            points = tuple(
                randomized_point(seed, base_index, lane, 32) for lane in (0, 3, 6, 9)
            )
            candidate = PredicateCase(base_index, "orientation_3d", stratum, points)
            if oracle_sign(candidate) != "zero":
                return points
            base_index += 1 << 32
    origin = point_from_integers(0, 0, 0)
    if stratum == "cancellation":
        return (
            origin,
            point_from_integers(1, 0, 0),
            point_from_integers(0, 1, 0),
            (ONE, ONE, scaled_word(1, -52)),
        )
    if stratum == "subnormal":
        return (
            origin,
            (MINIMUM_SUBNORMAL, ZERO, ZERO),
            (ZERO, MINIMUM_SUBNORMAL, ZERO),
            (ZERO, ZERO, MINIMUM_SUBNORMAL),
        )
    if stratum == "extreme_exponents":
        return (
            origin,
            point_from_integers(1, 0, 0, 500),
            point_from_integers(0, 1, 0, -500),
            point_from_integers(0, 0, 1, 10),
        )
    if stratum == "large_offsets":
        base = 1 << 40
        a = point_from_integers(base, base, base)
        return (
            a,
            point_from_integers(base + 1, base, base),
            point_from_integers(base, base + 2, base),
            point_from_integers(base, base, base + 4),
        )
    if stratum == "near_coplanar":
        return (
            origin,
            point_from_integers(1, 0, 0),
            point_from_integers(0, 1, 0),
            (ONE, ONE, scaled_word(1, -500)),
        )
    if stratum == "near_cospherical":
        return (
            point_from_integers(1, 0, 0),
            point_from_integers(0, 1, 0),
            point_from_integers(-1, 0, 0),
            (ZERO, NEGATIVE_ONE, scaled_word(1, -52)),
        )
    return (
        origin,
        point_from_integers(1, 0, 0),
        point_from_integers(0, 1, 0),
        point_from_integers(1, 1, 0),
    )


def power_case(seed: int, base_index: int, stratum: str) -> PredicateCase:
    cardinality = 1 + ((base_index // 3) % 10)
    if stratum == "exact_equalities":
        overlap = cardinality
    elif stratum == "well_conditioned":
        overlap = (base_index // 30) % (cardinality + 1)
    else:
        overlap = cardinality - 1
    r_ids = tuple(range(cardinality))
    q_ids = tuple(range(overlap)) + tuple(
        range(cardinality, cardinality + cardinality - overlap)
    )
    point_count = max((*r_ids, *q_ids), default=0) + 1
    points = [
        randomized_point(seed, base_index, 10 + 3 * index, 32)
        for index in range(point_count)
    ]
    denominator = POWER_WITNESS_DENOMINATORS[
        (base_index // 3) % len(POWER_WITNESS_DENOMINATORS)
    ]
    witness_numerators = [
        signed_small(seed, base_index, lane, 16) for lane in (1, 2, 3)
    ]
    while (
        math.gcd(
            denominator,
            math.gcd(
                abs(witness_numerators[0]),
                math.gcd(abs(witness_numerators[1]), abs(witness_numerators[2])),
            ),
        )
        != 1
    ):
        witness_numerators[0] += 1
    witness = canonical_witness((*witness_numerators, denominator))
    if stratum != "well_conditioned" and overlap < cardinality:
        r_special = overlap
        q_special = cardinality
        if stratum == "cancellation":
            points[r_special] = ("3fefffffffffffff", ZERO, ZERO)
            points[q_special] = point_from_integers(1, 0, 0)
            witness = (0, 0, 0, 1)
        elif stratum == "subnormal":
            points[r_special] = (MINIMUM_SUBNORMAL, ZERO, ZERO)
            points[q_special] = point_from_integers(0, 0, 0)
            witness = (0, 0, 0, 1)
        elif stratum == "extreme_exponents":
            points[r_special] = point_from_integers(1, 0, 0, 500)
            points[q_special] = point_from_integers(1, 0, 0, 499)
            witness = (0, 0, 0, 1)
        elif stratum == "large_offsets":
            base = 1 << 40
            points[r_special] = point_from_integers(base + 2, base, base)
            points[q_special] = point_from_integers(base + 1, base, base)
            witness = (base, base, base, 1)
        elif stratum == "near_coplanar":
            points[r_special] = (ONE, ONE, scaled_word(1, -500))
            points[q_special] = point_from_integers(1, 1, 0)
            witness = (0, 0, 0, 1)
        elif stratum == "near_cospherical":
            points[r_special] = (ONE, scaled_word(1, -52), ZERO)
            points[q_special] = point_from_integers(1, 0, 0)
            witness = (0, 0, 0, 1)
    return PredicateCase(
        base_index,
        "power_bisector_side",
        stratum,
        tuple(points),
        witness,
        r_ids,
        q_ids,
    )


def generate_base_case(seed: int, base_index: int) -> PredicateCase:
    predicate = predicate_for_index(base_index)
    stratum = stratum_for_index(base_index)
    if predicate == "compare_squared_distances":
        case = PredicateCase(
            base_index, predicate, stratum, distance_points(seed, base_index, stratum)
        )
    elif predicate == "orientation_3d":
        case = PredicateCase(
            base_index,
            predicate,
            stratum,
            orientation_points(seed, base_index, stratum),
        )
    else:
        case = power_case(seed, base_index, stratum)
    if stratum == "well_conditioned":
        return case
    # Hard recipes keep their algebraic stratum, but their axes and signs are
    # selected from the global counter.  They are therefore not ten million
    # copies of seven hand-written fixtures.  A second independent rotation is
    # reserved for the explicit metamorphic derivative.
    case = replace(
        metamorphic_case(case, seed ^ 0xA5A5A5A5A5A5A5A5, PROPER_ROTATION),
        relation="base",
    )
    if counter_value(seed, base_index, 61) & 1:
        if predicate == "compare_squared_distances":
            case = replace(
                case, points=(case.points[0], case.points[2], case.points[1])
            )
        elif predicate == "orientation_3d":
            case = replace(
                case,
                points=(case.points[0], case.points[2], case.points[1], case.points[3]),
            )
        else:
            case = replace(case, r_ids=case.q_ids, q_ids=case.r_ids)
    return case


def should_metamorph(base_index: int, stratum: str, stride: int) -> bool:
    return base_index % stride == 0 or stratum != "well_conditioned"


def metamorphism_for_base(base_index: int, stratum: str, stride: int) -> str:
    if stratum != "well_conditioned":
        return (PROPER_ROTATION, IMPROPER_REFLECTION)[base_index % 2]
    if base_index % stride:
        raise ValueError("a well-conditioned metamorphism escaped its frozen stride")
    return METAMORPHISMS[(base_index // stride) % len(METAMORPHISMS)]


def oracle_value(case: PredicateCase) -> Dyadic:
    points = tuple(
        tuple(dyadic_from_word(word) for word in point) for point in case.points
    )
    if case.predicate == "compare_squared_distances":
        witness, left, right = points
        left_distance = dyadic_sum(
            dyadic_multiply(dyadic_subtract(a, b), dyadic_subtract(a, b))
            for a, b in zip(witness, left)
        )
        right_distance = dyadic_sum(
            dyadic_multiply(dyadic_subtract(a, b), dyadic_subtract(a, b))
            for a, b in zip(witness, right)
        )
        return dyadic_subtract(left_distance, right_distance)
    if case.predicate == "orientation_3d":
        a, b, c, d = points
        u = tuple(dyadic_subtract(bi, ai) for ai, bi in zip(a, b))
        v = tuple(dyadic_subtract(ci, ai) for ai, ci in zip(a, c))
        w = tuple(dyadic_subtract(di, ai) for ai, di in zip(a, d))
        first = dyadic_multiply(
            u[0],
            dyadic_subtract(dyadic_multiply(v[1], w[2]), dyadic_multiply(v[2], w[1])),
        )
        second = dyadic_multiply(
            u[1],
            dyadic_subtract(dyadic_multiply(v[0], w[2]), dyadic_multiply(v[2], w[0])),
        )
        third = dyadic_multiply(
            u[2],
            dyadic_subtract(dyadic_multiply(v[0], w[1]), dyadic_multiply(v[1], w[0])),
        )
        return dyadic_add(dyadic_subtract(first, second), third)
    if case.witness is None:
        raise ValueError("the exact H_RQ oracle has no witness")
    denominator = case.witness[3]
    if denominator <= 0:
        raise ValueError("the long-campaign H_RQ witness denominator must be positive")
    witness_numerators = tuple((numerator, 0) for numerator in case.witness[:3])
    r_points = tuple(points[index] for index in case.r_ids)
    q_points = tuple(points[index] for index in case.q_ids)
    coordinate_delta = tuple(
        dyadic_subtract(
            dyadic_sum(point[axis] for point in r_points),
            dyadic_sum(point[axis] for point in q_points),
        )
        for axis in range(3)
    )
    norm_delta = dyadic_subtract(
        dyadic_sum(
            dyadic_sum(dyadic_multiply(value, value) for value in point)
            for point in r_points
        ),
        dyadic_sum(
            dyadic_sum(dyadic_multiply(value, value) for value in point)
            for point in q_points
        ),
    )
    dot = dyadic_sum(
        dyadic_multiply(value, delta)
        for value, delta in zip(witness_numerators, coordinate_delta)
    )
    return dyadic_subtract(
        dyadic_multiply((denominator, 0), norm_delta),
        dyadic_multiply((2, 0), dot),
    )


def oracle_sign(case: PredicateCase) -> str:
    return dyadic_sign(oracle_value(case))


def empty_counts() -> dict[str, Any]:
    return {
        "base_case_count": 0,
        "by_metamorphic_relation": {relation: 0 for relation in RELATIONS},
        "by_metamorphism": {name: 0 for name in METAMORPHISMS},
        "by_predicate": {predicate: 0 for predicate in PREDICATES},
        "by_sign": {value: 0 for value in SIGNS},
        "by_stage": {stage: 0 for stage in STAGES},
        "by_stratum": {stratum: 0 for stratum in STRATA},
        "metamorphic_sign_count": 0,
        "metamorphisms_verified": 0,
        "predicate_strata": {
            predicate: {stratum: 0 for stratum in STRATA} for predicate in PREDICATES
        },
        "remaining_unknown": 0,
        "total_sign_count": 0,
        "wrong_sign": 0,
    }


def validate_counts(value: Any, label: str) -> dict[str, Any]:
    fields = {
        "base_case_count",
        "by_metamorphic_relation",
        "by_metamorphism",
        "by_predicate",
        "by_sign",
        "by_stage",
        "by_stratum",
        "metamorphic_sign_count",
        "metamorphisms_verified",
        "predicate_strata",
        "remaining_unknown",
        "total_sign_count",
        "wrong_sign",
    }
    counts = require_fields(value, fields, label)
    for scalar in fields - {
        "by_predicate",
        "by_metamorphic_relation",
        "by_metamorphism",
        "by_sign",
        "by_stage",
        "by_stratum",
        "predicate_strata",
    }:
        require_integer(counts[scalar], f"{label} {scalar}")
    for field, keys in (
        ("by_metamorphic_relation", RELATIONS),
        ("by_metamorphism", METAMORPHISMS),
        ("by_predicate", PREDICATES),
        ("by_sign", SIGNS),
        ("by_stage", STAGES),
        ("by_stratum", STRATA),
    ):
        mapping = require_fields(counts[field], set(keys), f"{label} {field}")
        for key in keys:
            require_integer(mapping[key], f"{label} {field} {key}")
    matrix = require_fields(
        counts["predicate_strata"], set(PREDICATES), f"{label} matrix"
    )
    for predicate in PREDICATES:
        row = require_fields(
            matrix[predicate], set(STRATA), f"{label} matrix {predicate}"
        )
        for stratum in STRATA:
            require_integer(row[stratum], f"{label} matrix cell")
        if sum(row.values()) != counts["by_predicate"][predicate]:
            raise ValueError(f"{label} predicate matrix row disagrees")
    for stratum in STRATA:
        if (
            sum(matrix[predicate][stratum] for predicate in PREDICATES)
            != counts["by_stratum"][stratum]
        ):
            raise ValueError(f"{label} stratum matrix column disagrees")
    if counts["total_sign_count"] != sum(counts["by_predicate"].values()):
        raise ValueError(f"{label} predicate totals disagree")
    if counts["total_sign_count"] != sum(counts["by_sign"].values()):
        raise ValueError(f"{label} sign totals disagree")
    if counts["total_sign_count"] != sum(counts["by_stage"].values()):
        raise ValueError(f"{label} stage totals disagree")
    if counts["total_sign_count"] != sum(counts["by_stratum"].values()):
        raise ValueError(f"{label} stratum totals disagree")
    if (
        counts["total_sign_count"]
        != counts["base_case_count"] + counts["metamorphic_sign_count"]
    ):
        raise ValueError(f"{label} base and metamorphic totals disagree")
    if counts["metamorphisms_verified"] != counts["metamorphic_sign_count"]:
        raise ValueError(f"{label} metamorphism verification total disagrees")
    if sum(counts["by_metamorphism"].values()) != counts["metamorphic_sign_count"]:
        raise ValueError(f"{label} metamorphism catalog total disagrees")
    if (
        sum(counts["by_metamorphic_relation"].values())
        != counts["metamorphic_sign_count"]
    ):
        raise ValueError(f"{label} metamorphic relation total disagrees")
    return counts


def add_counts(target: dict[str, Any], addition: dict[str, Any]) -> None:
    validate_counts(target, "aggregate counts")
    validate_counts(addition, "chunk counts")
    for field in (
        "base_case_count",
        "metamorphic_sign_count",
        "metamorphisms_verified",
        "remaining_unknown",
        "total_sign_count",
        "wrong_sign",
    ):
        target[field] += addition[field]
    for field in ("by_predicate", "by_sign", "by_stage", "by_stratum"):
        for key in target[field]:
            target[field][key] += addition[field][key]
    for field in ("by_metamorphic_relation", "by_metamorphism"):
        for key in target[field]:
            target[field][key] += addition[field][key]
    for predicate in PREDICATES:
        for stratum in STRATA:
            target["predicate_strata"][predicate][stratum] += addition[
                "predicate_strata"
            ][predicate][stratum]


CONFIG_FIELDS = {
    "base_case_count",
    "chunk_size_base_cases",
    "expected_corpus_sha256",
    "expected_oracle_sha256",
    "generator",
    "kind",
    "metamorphisms",
    "native_interface",
    "oracle",
    "ordered_root_algorithm",
    "power_witness_denominators",
    "predicate_cycle",
    "schema_version",
    "strata",
}


def validate_config(
    value: Any, *, require_expected_roots: bool = False
) -> dict[str, Any]:
    config = require_fields(value, CONFIG_FIELDS, "campaign config")
    if config["kind"] != CONFIG_KIND or config["schema_version"] != SCHEMA_VERSION:
        raise ValueError("the campaign config changed its closed identity")
    base_count = require_integer(
        config["base_case_count"], "base case count", minimum=MIN_PRODUCTION_BASE_CASES
    )
    chunk_size = require_integer(
        config["chunk_size_base_cases"],
        "chunk size",
        minimum=1,
        maximum=MAX_CHUNK_BASE_CASES,
    )
    if base_count % 100:
        raise ValueError("the production base count must close the percentage cycle")
    generator = require_fields(
        config["generator"], {"algorithm", "seed"}, "generator config"
    )
    if (
        generator["algorithm"] != GENERATOR_ID
        or not isinstance(generator["seed"], str)
        or len(generator["seed"]) != 16
    ):
        raise ValueError("the generator config is unsupported")
    int(generator["seed"], 16)
    metamorphisms = require_fields(
        config["metamorphisms"],
        {"catalog", "non_well_conditioned_ids", "stride"},
        "metamorphism catalog config",
    )
    if metamorphisms["catalog"] != list(METAMORPHISM_CATALOG) or metamorphisms[
        "non_well_conditioned_ids"
    ] != [PROPER_ROTATION, IMPROPER_REFLECTION]:
        raise ValueError("the metamorphism catalog is unsupported")
    require_integer(metamorphisms["stride"], "metamorphism stride", minimum=1)
    oracle = require_fields(config["oracle"], {"id"}, "oracle config")
    if oracle["id"] != ORACLE_ID or config["native_interface"] != NATIVE_INTERFACE:
        raise ValueError("the campaign oracle or native interface is unsupported")
    if config["ordered_root_algorithm"] != ORDERED_ROOT_ALGORITHM:
        raise ValueError("the ordered scientific root algorithm changed")
    if config["power_witness_denominators"] != list(POWER_WITNESS_DENOMINATORS):
        raise ValueError("the frozen H_RQ witness denominators changed")
    for field in ("expected_corpus_sha256", "expected_oracle_sha256"):
        root = config[field]
        if root is None and not require_expected_roots:
            continue
        if not isinstance(root, str) or len(root) != 64:
            raise ValueError(f"{field} must be precomputed before a campaign run")
        int(root, 16)
    if config["predicate_cycle"] != list(PREDICATES):
        raise ValueError("the predicate cycle changed")
    strata = config["strata"]
    if not isinstance(strata, list) or len(strata) != len(STRATA):
        raise ValueError("the stratum catalog is incomplete")
    expected = [
        {"id": name, "percent": percent}
        for name, percent in zip(STRATA, STRATUM_PERCENTAGES)
    ]
    if strata != expected:
        raise ValueError("the frozen stratum percentages changed")
    if (
        chunk_size
        + math.ceil(chunk_size / metamorphisms["stride"])
        + math.ceil(chunk_size * 7 / 100)
        > 10_000
    ):
        raise ValueError("the configured chunk can exceed 10,000 native output rows")
    return config


def repository_snapshot(repository: Path) -> dict[str, Any]:
    def git(*arguments: str) -> str:
        completed = subprocess.run(
            ["git", "-C", str(repository), *arguments],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
            encoding="utf-8",
        )
        if completed.returncode != 0 or completed.stderr:
            raise ValueError(
                f"Git provenance failed closed: {completed.stderr.strip()}"
            )
        return completed.stdout

    root = Path(git("rev-parse", "--show-toplevel").strip()).resolve()
    if root != repository.resolve():
        raise ValueError("--repository must name the Git worktree root")
    head = git("rev-parse", "HEAD").strip()
    status = git("status", "--porcelain=v1", "--untracked-files=all")
    return {
        "git_head": head,
        "git_status_sha256": sha256_bytes(status.encode("utf-8")),
        "repository_clean": status == "",
    }


def verify_repository(
    repository: Path, expected: dict[str, Any], allow_dirty_smoke: bool
) -> None:
    observed = repository_snapshot(repository)
    if observed != expected:
        raise ValueError("the repository HEAD or status changed during the campaign")
    if not allow_dirty_smoke and not observed["repository_clean"]:
        raise ValueError("the long campaign refuses a dirty Git worktree")


def verify_head_blob(repository: Path, path: Path, label: str) -> None:
    try:
        relative = path.resolve().relative_to(repository.resolve())
    except ValueError as error:
        raise ValueError(f"production {label} must be inside --repository") from error
    relative_text = relative.as_posix()
    tracked = subprocess.run(
        [
            "git",
            "-C",
            str(repository),
            "ls-files",
            "--error-unmatch",
            "--",
            relative_text,
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if tracked.returncode != 0:
        raise ValueError(f"production {label} is not tracked by Git")
    blob = subprocess.run(
        ["git", "-C", str(repository), "show", f"HEAD:{relative_text}"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if blob.returncode != 0 or blob.stdout != path.read_bytes():
        raise ValueError(f"production {label} differs from its blob at HEAD")


def discover_cmake_cache(native: Path, explicit: Path | None) -> Path:
    if explicit is not None:
        cache = explicit.resolve()
        require_regular_file(cache, "CMake cache")
        return cache
    for directory in (native.parent, *native.parents):
        candidate = directory / "CMakeCache.txt"
        if candidate.is_file() and not candidate.is_symlink():
            return candidate.resolve()
    raise ValueError("the native predicate replay has no discoverable CMakeCache.txt")


def cmake_cache_values(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith(("#", "//")) or "=" not in line:
            continue
        typed_key, value = line.split("=", 1)
        key = typed_key.split(":", 1)[0]
        values[key] = value
    return values


def discover_target_flags(
    cache: Path, native: Path, cache_values: dict[str, str]
) -> tuple[Path, str]:
    build_directories: list[Path] = []
    configured_build_directory = cache_values.get("CMAKE_CACHEFILE_DIR")
    if configured_build_directory:
        build_directories.append(Path(configured_build_directory).resolve())
    if cache.parent.resolve() not in build_directories:
        build_directories.append(cache.parent.resolve())
    relative = Path("CMakeFiles") / f"{native.name}.dir" / "flags.make"
    for build_directory in build_directories:
        flags = build_directory / relative
        if flags.is_file() and not flags.is_symlink():
            content = flags.read_bytes().decode("utf-8")
            if "CXX_FLAGS" not in content:
                raise ValueError("the native target flags omit CXX_FLAGS")
            return flags.resolve(), content
    raise ValueError(
        "the native predicate replay has no effective target flags.make provenance"
    )


def discover_build_metadata(cache: Path, native: Path) -> dict[str, str]:
    require_regular_file(cache, "CMake cache")
    values = cmake_cache_values(cache)
    compiler_text = values.get("CMAKE_CXX_COMPILER")
    if not compiler_text:
        raise ValueError("CMake cache omits CMAKE_CXX_COMPILER")
    compiler = Path(compiler_text)
    if not compiler.is_absolute():
        compiler = Path(shutil.which(compiler_text) or compiler_text)
    if not compiler.is_file():
        raise ValueError("the configured C++ compiler is unavailable")
    completed = subprocess.run(
        [str(compiler), "--version"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        check=False,
    )
    if completed.returncode != 0 or not completed.stdout:
        raise ValueError("the configured C++ compiler version is unavailable")
    version_text = completed.stdout.decode("utf-8", errors="replace")
    build_type = values.get("CMAKE_BUILD_TYPE", "")
    target_flags_path, target_flags = discover_target_flags(cache, native, values)
    typed_flags = (
        values.get(f"CMAKE_CXX_FLAGS_{build_type.upper()}", "") if build_type else ""
    )
    return {
        "build_type": build_type,
        "cmake_cache_path": str(cache),
        "cmake_cache_sha256": sha256_file(cache),
        "cmake_generator": values.get("CMAKE_GENERATOR", ""),
        "compiler_path": str(compiler.resolve()),
        "compiler_version": version_text.splitlines()[0],
        "compiler_version_sha256": sha256_bytes(completed.stdout),
        "cxx_flags": values.get("CMAKE_CXX_FLAGS", ""),
        "cxx_flags_for_build_type": typed_flags,
        "target_flags": target_flags,
        "target_flags_path": str(target_flags_path),
        "target_flags_sha256": sha256_file(target_flags_path),
    }


def verify_runtime_inputs(
    *,
    native: Path,
    tool: Path,
    config_path: Path,
    cache: Path,
    identities: dict[str, str],
    expected_build_metadata: dict[str, str],
    repository: Path,
    scope: str,
) -> None:
    observed_hashes = {
        "native_executable_sha256": sha256_file(native),
        "tool_sha256": sha256_file(tool),
        "frozen_config_sha256": sha256_file(config_path),
    }
    for field, observed in observed_hashes.items():
        if identities[field] != observed:
            raise ValueError(f"runtime provenance changed: {field}")
    if discover_build_metadata(cache, native) != expected_build_metadata:
        raise ValueError("runtime build metadata changed")
    if scope == "production":
        verify_head_blob(repository, tool, "campaign tool")
        verify_head_blob(repository, config_path, "campaign config")


def artifact_reference(relative_path: str, content: bytes) -> dict[str, Any]:
    return {
        "relative_path": relative_path,
        "sha256": sha256_bytes(content),
        "size_bytes": len(content),
    }


def validate_reference(value: Any, label: str) -> dict[str, Any]:
    reference = require_fields(value, HASH_FIELDS, label)
    relative_path = reference["relative_path"]
    if not isinstance(relative_path, str) or "\\" in relative_path:
        raise ValueError(f"{label} path is unsafe")
    pure_path = PurePosixPath(relative_path)
    if (
        pure_path.is_absolute()
        or not pure_path.parts
        or any(part in ("", ".", "..") for part in pure_path.parts)
    ):
        raise ValueError(f"{label} path is unsafe")
    if not isinstance(reference["sha256"], str) or len(reference["sha256"]) != 64:
        raise ValueError(f"{label} hash is invalid")
    int(reference["sha256"], 16)
    require_integer(reference["size_bytes"], f"{label} size", minimum=1)
    return reference


def validate_artifact(
    checkpoint_dir: Path, reference: dict[str, Any], label: str
) -> tuple[Any, bytes]:
    validate_reference(reference, label)
    path = checkpoint_dir / reference["relative_path"]
    value, content = load_canonical_file(path, label)
    if (
        len(content) != reference["size_bytes"]
        or sha256_bytes(content) != reference["sha256"]
    ):
        raise ValueError(f"{label} differs from its checkpoint hash")
    return value, content


def run_native_one(
    native: Path, case: PredicateCase, timeout_seconds: int
) -> dict[str, Any]:
    completed = subprocess.run(
        [str(native), "--decision-only", "--batch"],
        input=case.command() + "\n",
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        encoding="utf-8",
        timeout=timeout_seconds,
    )
    if completed.returncode != 0 or completed.stderr:
        raise RuntimeError(
            f"native minimizer probe failed closed: {completed.stderr!r}"
        )
    lines = completed.stdout.splitlines()
    if len(lines) != 1:
        raise ValueError("native minimizer probe returned the wrong row count")
    return validate_native_row(lines[0], case)


def validate_native_row(line: str, case: PredicateCase) -> dict[str, Any]:
    value = strict_json_loads(line)
    if canonical_json(value) != line:
        raise ValueError("the native decision row is not canonical JSON")
    result = require_fields(value, NATIVE_RESULT_FIELDS, "native decision row")
    if (
        result["predicate"] != case.predicate
        or result["sign"] not in SIGNS
        or result["certification_stage"] not in STAGES
    ):
        raise ValueError("the native decision row changed its closed enums")
    counters = require_fields(result["counters"], COUNTER_FIELDS, "native counters")
    for field, count in counters.items():
        require_integer(count, f"native counter {field}")
    stage_counter = {
        "cpu_multiprecision": "cpu_multiprecision_certified",
        "expansion": "expansion_certified",
        "fp64_filtered": "fp64_filtered_certified",
    }[result["certification_stage"]]
    if (
        counters[stage_counter] != 1
        or sum(
            counters[field]
            for field in (
                "cpu_multiprecision_certified",
                "expansion_certified",
                "fp64_filtered_certified",
            )
        )
        != 1
    ):
        raise ValueError("the native decision stage counters are inconsistent")
    if counters["remaining_unknown"] != 0 or counters["exact_zeros"] != (
        result["sign"] == "zero"
    ):
        raise ValueError(
            "the native decision counters contain an unknown or bad zero count"
        )
    return result


def greedy_minimize_words(
    words: Sequence[str], diverges: Callable[[tuple[str, ...]], bool]
) -> tuple[str, ...]:
    minimized = tuple(words)
    for index in range(len(minimized)):
        for replacement in word_shrink_candidates(minimized[index]):
            candidate = minimized[:index] + (replacement,) + minimized[index + 1 :]
            if diverges(candidate):
                minimized = candidate
                break
    return minimized


def word_shrink_candidates(word: str) -> tuple[str, ...]:
    bits = int(word, 16)
    sign_bit = bits & (1 << 63)
    exponent_bits = (bits >> 52) & 0x7FF
    fraction_bits = bits & ((1 << 52) - 1)
    candidates = [ZERO, f"{sign_bit:016x}", ONE, NEGATIVE_ONE, MINIMUM_SUBNORMAL]
    if exponent_bits not in (0, 0x7FF):
        candidates.extend(
            [
                f"{sign_bit | (1023 << 52):016x}",
                f"{sign_bit | (exponent_bits << 52):016x}",
            ]
        )
        for retained_bits in (1, 4, 8, 16, 26, 40):
            mask = ((1 << retained_bits) - 1) << (52 - retained_bits)
            candidates.append(
                f"{sign_bit | (exponent_bits << 52) | (fraction_bits & mask):016x}"
            )
    unique: list[str] = []
    for candidate in candidates:
        if candidate != word and candidate not in unique:
            unique.append(candidate)
    return tuple(unique)


def integer_shrink_candidates(value: int) -> tuple[int, ...]:
    candidates = (0, 1 if value >= 0 else -1, value // 2)
    return tuple(
        candidate for candidate in dict.fromkeys(candidates) if candidate != value
    )


def minimize_case(
    native: Path, case: PredicateCase, timeout_seconds: int
) -> tuple[PredicateCase, dict[str, Any]]:
    shape = tuple(len(point) for point in case.points)
    flat = tuple(word for point in case.points for word in point)

    def rebuilt(words: tuple[str, ...]) -> PredicateCase:
        cursor = 0
        points: list[PointWords] = []
        for width in shape:
            points.append(tuple(words[cursor : cursor + width]))  # type: ignore[arg-type]
            cursor += width
        return replace(case, points=tuple(points))

    last_observed: dict[str, Any] = {}

    def diverges(words: tuple[str, ...]) -> bool:
        nonlocal last_observed
        candidate = rebuilt(words)
        last_observed = run_native_one(native, candidate, timeout_seconds)
        return last_observed["sign"] != oracle_sign(candidate)

    minimized_words = greedy_minimize_words(flat, diverges)
    minimized = rebuilt(minimized_words)
    if minimized.witness is not None:
        for index in range(3):
            for replacement_value in integer_shrink_candidates(
                minimized.witness[index]
            ):
                witness = list(minimized.witness)
                witness[index] = replacement_value
                candidate = replace(
                    minimized,
                    witness=canonical_witness(tuple(witness)),  # type: ignore[arg-type]
                )
                candidate_observed = run_native_one(native, candidate, timeout_seconds)
                if candidate_observed["sign"] != oracle_sign(candidate):
                    minimized = candidate
                    break
        for denominator in POWER_WITNESS_DENOMINATORS:
            witness = canonical_witness((*minimized.witness[:3], denominator))
            candidate = replace(minimized, witness=witness)
            candidate_observed = run_native_one(native, candidate, timeout_seconds)
            if candidate_observed["sign"] != oracle_sign(candidate):
                minimized = candidate
                break
        while len(minimized.r_ids) > 1:
            candidate = replace(
                minimized,
                r_ids=minimized.r_ids[:-1],
                q_ids=minimized.q_ids[:-1],
            )
            candidate_observed = run_native_one(native, candidate, timeout_seconds)
            if candidate_observed["sign"] == oracle_sign(candidate):
                break
            minimized = candidate
    observed = run_native_one(native, minimized, timeout_seconds)
    if observed["sign"] == oracle_sign(minimized):
        raise RuntimeError("the divergence vanished during automatic minimization")
    return minimized, observed


def publish_repro(
    checkpoint_dir: Path,
    native: Path,
    original: PredicateCase,
    observed: dict[str, Any],
    timeout_seconds: int,
    identities: dict[str, str],
) -> Path:
    original_record = {
        "base_index": original.base_index,
        "command": original.command(),
        "config_sha256": identities["config_sha256"],
        "expected_sign": oracle_sign(original),
        "kind": ORIGINAL_REPRO_KIND,
        "native_executable_sha256": identities["native_executable_sha256"],
        "observed": observed,
        "oracle_descriptor_sha256": identities["oracle_descriptor_sha256"],
        "relation": original.relation,
        "repository_phase_exit_claimed": False,
        "schema_version": SCHEMA_VERSION,
        "stratum": original.stratum,
        "tool_sha256": identities["tool_sha256"],
    }
    original_content = canonical_bytes(original_record)
    original_relative_path = f"repros/original-{sha256_bytes(original_content)}.json"
    original_path = checkpoint_dir / original_relative_path
    atomic_publish(original_path, original_content)
    try:
        minimized, minimized_observed = minimize_case(native, original, timeout_seconds)
    except Exception as error:
        raise RuntimeError(
            f"original divergence preserved at {original_path}; minimization failed: {error}"
        ) from error
    repro = {
        "base_index": original.base_index,
        "config_sha256": identities["config_sha256"],
        "kind": REPRO_KIND,
        "minimized": {
            "command": minimized.command(),
            "expected_sign": oracle_sign(minimized),
            "observed": minimized_observed,
            "relation": minimized.relation,
            "stratum": minimized.stratum,
        },
        "native_executable_sha256": identities["native_executable_sha256"],
        "oracle_descriptor_sha256": identities["oracle_descriptor_sha256"],
        "original_artifact": artifact_reference(
            original_relative_path, original_content
        ),
        "repository_phase_exit_claimed": False,
        "schema_version": SCHEMA_VERSION,
        "tool_sha256": identities["tool_sha256"],
    }
    content = canonical_bytes(repro)
    path = checkpoint_dir / "repros" / f"repro-{sha256_bytes(content)}.json"
    atomic_publish(path, content)
    return path


def cases_for_range(
    seed: int, begin: int, end: int, stride: int
) -> Iterator[PredicateCase]:
    for base_index in range(begin, end):
        base = generate_base_case(seed, base_index)
        yield base
        if should_metamorph(base_index, base.stratum, stride):
            yield metamorphic_case(
                base,
                seed,
                metamorphism_for_base(base_index, base.stratum, stride),
            )


def update_case_count(
    counts: dict[str, Any],
    case: PredicateCase,
    result: dict[str, Any],
    expected: str,
) -> None:
    counts["total_sign_count"] += 1
    counts["by_predicate"][case.predicate] += 1
    counts["by_stratum"][case.stratum] += 1
    counts["predicate_strata"][case.predicate][case.stratum] += 1
    counts["by_sign"][expected] += 1
    counts["by_stage"][result["certification_stage"]] += 1
    counts["remaining_unknown"] += result["counters"]["remaining_unknown"]
    if case.relation == "base":
        counts["base_case_count"] += 1
    else:
        counts["metamorphic_sign_count"] += 1
        counts["metamorphisms_verified"] += 1
        counts["by_metamorphism"][case.relation] += 1
        relation = (
            "opposite_sign"
            if case.relation == PREDICATE_SYMMETRY
            or (
                case.relation == IMPROPER_REFLECTION
                and case.predicate == "orientation_3d"
            )
            else "same_sign"
        )
        counts["by_metamorphic_relation"][relation] += 1
    if result["sign"] != expected:
        counts["wrong_sign"] += 1


def execute_chunk(
    native: Path,
    checkpoint_dir: Path,
    seed: int,
    stride: int,
    begin: int,
    end: int,
    timeout_seconds: int,
    identities: dict[str, str],
    ordered_roots_before: dict[str, str],
) -> tuple[dict[str, Any], bytes]:
    counts = empty_counts()
    corpus_digest = hashlib.sha256()
    oracle_digest = hashlib.sha256()
    output_digest = hashlib.sha256()
    ordered_roots_after = dict(ordered_roots_before)
    with tempfile.NamedTemporaryFile(
        "w+", encoding="utf-8", dir=checkpoint_dir, delete=False
    ) as input_stream:
        input_path = Path(input_stream.name)
        for case in cases_for_range(seed, begin, end, stride):
            line = case.command() + "\n"
            input_stream.write(line)
            encoded = line.encode("utf-8")
            corpus_digest.update(encoded)
            ordered_roots_after["corpus"] = extend_ordered_root(
                "corpus", ordered_roots_after["corpus"], encoded
            )
        input_stream.flush()
        os.fsync(input_stream.fileno())
    output_descriptor, output_name = tempfile.mkstemp(
        dir=checkpoint_dir, prefix=".native-output-", suffix=".jsonl"
    )
    output_path = Path(output_name)
    try:
        with input_path.open("rb") as input_stream, os.fdopen(
            output_descriptor, "wb"
        ) as output_stream:
            completed = subprocess.run(
                [str(native), "--decision-only", "--batch"],
                stdin=input_stream,
                stdout=output_stream,
                stderr=subprocess.PIPE,
                check=False,
                timeout=timeout_seconds,
            )
            output_stream.flush()
            os.fsync(output_stream.fileno())
        stderr = completed.stderr.decode("utf-8", errors="replace")
        if completed.returncode != 0 or stderr:
            raise RuntimeError(
                "the native campaign chunk failed closed: "
                f"returncode={completed.returncode}, stderr={stderr!r}"
            )
        with output_path.open("r", encoding="utf-8", newline="") as stream:
            for case in cases_for_range(seed, begin, end, stride):
                line = stream.readline()
                if not line or not line.endswith("\n"):
                    raise ValueError(
                        "the native campaign chunk ended before its expected row count"
                    )
                encoded_output = line.encode("utf-8")
                output_digest.update(encoded_output)
                ordered_roots_after["output"] = extend_ordered_root(
                    "output", ordered_roots_after["output"], encoded_output
                )
                result = validate_native_row(line[:-1], case)
                expected_sign = oracle_sign(case)
                oracle_row = canonical_bytes(
                    {
                        "base_index": case.base_index,
                        "predicate": case.predicate,
                        "relation": case.relation,
                        "sign": expected_sign,
                        "stratum": case.stratum,
                    }
                )
                oracle_digest.update(oracle_row)
                ordered_roots_after["oracle"] = extend_ordered_root(
                    "oracle", ordered_roots_after["oracle"], oracle_row
                )
                update_case_count(counts, case, result, expected_sign)
                if result["sign"] != expected_sign:
                    repro = publish_repro(
                        checkpoint_dir,
                        native,
                        case,
                        result,
                        timeout_seconds,
                        identities,
                    )
                    raise RuntimeError(
                        f"wrong predicate sign; minimized repro: {repro}"
                    )
            if stream.read(1):
                raise ValueError("the native campaign chunk returned extra rows")
    finally:
        input_path.unlink(missing_ok=True)
        output_path.unlink(missing_ok=True)
    validate_counts(counts, "completed chunk counts")
    chunk = {
        "base_begin": begin,
        "base_end": end,
        "corpus_sha256": corpus_digest.hexdigest(),
        "counts": counts,
        "kind": CHUNK_KIND,
        "native_output_sha256": output_digest.hexdigest(),
        "oracle_signs_sha256": oracle_digest.hexdigest(),
        "ordered_roots_after": ordered_roots_after,
        "ordered_roots_before": ordered_roots_before,
        "schema_version": SCHEMA_VERSION,
    }
    return chunk, canonical_bytes(chunk)


MANIFEST_FIELDS = {
    "base_case_count",
    "build_metadata",
    "build_metadata_sha256",
    "catalog_sha256",
    "certificate_artifact",
    "chunk_size_base_cases",
    "chunks",
    "config_sha256",
    "corpus_descriptor_sha256",
    "counts",
    "effective_smoke",
    "expected_corpus_sha256",
    "expected_oracle_sha256",
    "frozen_config_sha256",
    "gcp_used",
    "git_head",
    "git_status_sha256",
    "kind",
    "native_executable_sha256",
    "next_base_index",
    "oracle_descriptor_sha256",
    "ordered_roots",
    "ordered_root_algorithm",
    "repository_clean",
    "repository_phase_exit_claimed",
    "runtime_environment",
    "runtime_milliseconds",
    "scope",
    "schema_version",
    "sequence_number",
    "state",
    "tool_sha256",
}


def require_ordered_roots(value: Any, label: str) -> dict[str, str]:
    roots = require_fields(value, set(ROOT_DOMAINS), label)
    for name, root in roots.items():
        if not isinstance(root, str) or len(root) != 64:
            raise ValueError(f"{label} {name} is not a SHA-256 chain root")
        int(root, 16)
    return roots


def current_runtime_environment() -> dict[str, str]:
    return {
        "platform": platform.platform(),
        "python": platform.python_version(),
    }


def validate_manifest(value: Any, label: str) -> dict[str, Any]:
    manifest = require_fields(value, MANIFEST_FIELDS, label)
    if (
        manifest["kind"] != MANIFEST_KIND
        or manifest["schema_version"] != SCHEMA_VERSION
    ):
        raise ValueError(f"{label} changed identity")
    for field in (
        "config_sha256",
        "catalog_sha256",
        "corpus_descriptor_sha256",
        "frozen_config_sha256",
        "oracle_descriptor_sha256",
        "tool_sha256",
        "native_executable_sha256",
        "git_status_sha256",
        "expected_corpus_sha256",
        "expected_oracle_sha256",
        "build_metadata_sha256",
    ):
        if not isinstance(manifest[field], str) or len(manifest[field]) != 64:
            raise ValueError(f"{label} has an invalid {field}")
        int(manifest[field], 16)
    if not isinstance(manifest["git_head"], str) or len(manifest["git_head"]) != 40:
        raise ValueError(f"{label} has an invalid Git HEAD")
    if (
        manifest["repository_phase_exit_claimed"] is not False
        or manifest["gcp_used"] is not False
    ):
        raise ValueError(f"{label} makes a forbidden phase or GCP claim")
    if (
        type(manifest["repository_clean"]) is not bool
        or type(manifest["effective_smoke"]) is not bool
    ):
        raise ValueError(f"{label} has invalid repository booleans")
    base_count = require_integer(
        manifest["base_case_count"], f"{label} base count", minimum=1
    )
    chunk_size = require_integer(
        manifest["chunk_size_base_cases"],
        f"{label} chunk size",
        minimum=1,
        maximum=MAX_CHUNK_BASE_CASES,
    )
    next_index = require_integer(
        manifest["next_base_index"], f"{label} cursor", maximum=base_count
    )
    require_integer(manifest["sequence_number"], f"{label} sequence")
    require_integer(manifest["runtime_milliseconds"], f"{label} runtime")
    require_ordered_roots(manifest["ordered_roots"], f"{label} ordered roots")
    if manifest["ordered_root_algorithm"] != ORDERED_ROOT_ALGORITHM:
        raise ValueError(f"{label} ordered root algorithm changed")
    if manifest["scope"] not in ("smoke", "production"):
        raise ValueError(f"{label} scope is invalid")
    if manifest["effective_smoke"] != (manifest["scope"] == "smoke"):
        raise ValueError(f"{label} scope and smoke flag disagree")
    runtime_environment = require_fields(
        manifest["runtime_environment"],
        {"platform", "python"},
        f"{label} runtime environment",
    )
    if not all(
        isinstance(value, str) and value for value in runtime_environment.values()
    ):
        raise ValueError(f"{label} runtime environment is invalid")
    build_metadata = require_fields(
        manifest["build_metadata"],
        {
            "build_type",
            "cmake_cache_path",
            "cmake_cache_sha256",
            "cmake_generator",
            "compiler_path",
            "compiler_version",
            "compiler_version_sha256",
            "cxx_flags",
            "cxx_flags_for_build_type",
            "target_flags",
            "target_flags_path",
            "target_flags_sha256",
        },
        f"{label} build metadata",
    )
    if canonical_hash(build_metadata) != manifest["build_metadata_sha256"]:
        raise ValueError(f"{label} build metadata hash disagrees")
    if manifest["state"] not in ("running", "complete"):
        raise ValueError(f"{label} has an invalid state")
    chunks = manifest["chunks"]
    if not isinstance(chunks, list):
        raise ValueError(f"{label} chunk catalog is invalid")
    for index, reference in enumerate(chunks):
        validate_reference(reference, f"{label} chunk {index}")
    validate_counts(manifest["counts"], f"{label} counts")
    if manifest["counts"]["base_case_count"] != next_index:
        raise ValueError(f"{label} counts disagree with its cursor")
    certificate = manifest["certificate_artifact"]
    if manifest["state"] == "complete":
        validate_reference(certificate, f"{label} certificate")
        if next_index != base_count:
            raise ValueError(f"{label} completed before its base corpus")
    elif certificate is not None:
        raise ValueError(f"{label} running state references a certificate")
    if chunk_size > base_count and not manifest["effective_smoke"]:
        raise ValueError(f"{label} has an impossible production chunk")
    return manifest


def chunk_relative_path(index: int) -> str:
    return f"chunks/chunk-{index:08d}.json"


def publish_manifest(checkpoint_dir: Path, manifest: dict[str, Any]) -> None:
    validate_manifest(manifest, "manifest to publish")
    atomic_publish(checkpoint_dir / MANIFEST_NAME, canonical_bytes(manifest))


def validate_committed_chunks(checkpoint_dir: Path, manifest: dict[str, Any]) -> None:
    aggregate = empty_counts()
    expected_begin = 0
    expected_roots = initial_ordered_roots()
    for index, reference in enumerate(manifest["chunks"]):
        value, _ = validate_artifact(
            checkpoint_dir, reference, f"committed chunk {index}"
        )
        chunk = require_fields(
            value,
            {
                "base_begin",
                "base_end",
                "corpus_sha256",
                "counts",
                "kind",
                "native_output_sha256",
                "oracle_signs_sha256",
                "ordered_roots_after",
                "ordered_roots_before",
                "schema_version",
            },
            f"committed chunk {index}",
        )
        if chunk["kind"] != CHUNK_KIND or chunk["schema_version"] != SCHEMA_VERSION:
            raise ValueError("a committed chunk changed identity")
        begin = require_integer(chunk["base_begin"], "chunk begin")
        end = require_integer(chunk["base_end"], "chunk end", minimum=begin + 1)
        if begin != expected_begin or end - begin > manifest["chunk_size_base_cases"]:
            raise ValueError("the committed chunks are not a contiguous bounded prefix")
        for field in (
            "corpus_sha256",
            "native_output_sha256",
            "oracle_signs_sha256",
        ):
            if not isinstance(chunk[field], str) or len(chunk[field]) != 64:
                raise ValueError(f"a committed chunk has an invalid {field}")
            int(chunk[field], 16)
        before = require_ordered_roots(
            chunk["ordered_roots_before"], f"committed chunk {index} roots before"
        )
        after = require_ordered_roots(
            chunk["ordered_roots_after"], f"committed chunk {index} roots after"
        )
        if before != expected_roots or after == before:
            raise ValueError("the committed chunk ordered roots are discontinuous")
        expected_roots = dict(after)
        counts = validate_counts(chunk["counts"], f"committed chunk {index} counts")
        if (
            counts["base_case_count"] != end - begin
            or counts["wrong_sign"]
            or counts["remaining_unknown"]
        ):
            raise ValueError("a committed chunk contains an invalid scientific verdict")
        add_counts(aggregate, counts)
        expected_begin = end
    if (
        expected_begin != manifest["next_base_index"]
        or aggregate != manifest["counts"]
        or expected_roots != manifest["ordered_roots"]
    ):
        raise ValueError("the checkpoint aggregate differs from its committed chunks")


def identities_for(
    config: dict[str, Any], config_content: bytes, tool: Path, native: Path
) -> dict[str, str]:
    catalog = {
        "metamorphisms": config["metamorphisms"],
        "ordered_root_algorithm": config["ordered_root_algorithm"],
        "power_witness_denominators": config["power_witness_denominators"],
        "predicate_cycle": config["predicate_cycle"],
        "strata": config["strata"],
    }
    oracle_descriptor = {
        "arithmetic": "integer-times-power-of-two",
        "formulae": list(PREDICATES),
        "id": ORACLE_ID,
    }
    corpus_descriptor = {
        "base_case_count": config["base_case_count"],
        "catalog_sha256": canonical_hash(catalog),
        "generator": config["generator"],
    }
    return {
        "catalog_sha256": canonical_hash(catalog),
        "config_sha256": sha256_bytes(config_content),
        "corpus_descriptor_sha256": canonical_hash(corpus_descriptor),
        "frozen_config_sha256": sha256_bytes(config_content),
        "native_executable_sha256": sha256_file(native),
        "oracle_descriptor_sha256": canonical_hash(oracle_descriptor),
        "tool_sha256": sha256_file(tool),
    }


def initialize_checkpoint(
    checkpoint_dir: Path,
    identities: dict[str, str],
    snapshot: dict[str, Any],
    base_count: int,
    chunk_size: int,
    effective_smoke: bool,
    expected_corpus_sha256: str,
    expected_oracle_sha256: str,
    build_metadata: dict[str, str],
) -> dict[str, Any]:
    if checkpoint_dir.exists():
        raise ValueError("the checkpoint path exists without a canonical manifest")
    parent = checkpoint_dir.parent
    parent.mkdir(parents=True, exist_ok=True)
    temporary = Path(
        tempfile.mkdtemp(dir=parent, prefix=f".{checkpoint_dir.name}.", suffix=".init")
    )
    try:
        for directory in ("chunks", "certificates", "repros"):
            (temporary / directory).mkdir()
        manifest = {
            "base_case_count": base_count,
            "build_metadata": build_metadata,
            "build_metadata_sha256": canonical_hash(build_metadata),
            "catalog_sha256": identities["catalog_sha256"],
            "certificate_artifact": None,
            "chunk_size_base_cases": chunk_size,
            "chunks": [],
            "config_sha256": identities["config_sha256"],
            "corpus_descriptor_sha256": identities["corpus_descriptor_sha256"],
            "counts": empty_counts(),
            "effective_smoke": effective_smoke,
            "expected_corpus_sha256": expected_corpus_sha256,
            "expected_oracle_sha256": expected_oracle_sha256,
            "frozen_config_sha256": identities["frozen_config_sha256"],
            "gcp_used": False,
            "git_head": snapshot["git_head"],
            "git_status_sha256": snapshot["git_status_sha256"],
            "kind": MANIFEST_KIND,
            "native_executable_sha256": identities["native_executable_sha256"],
            "next_base_index": 0,
            "oracle_descriptor_sha256": identities["oracle_descriptor_sha256"],
            "ordered_roots": initial_ordered_roots(),
            "ordered_root_algorithm": ORDERED_ROOT_ALGORITHM,
            "repository_clean": snapshot["repository_clean"],
            "repository_phase_exit_claimed": False,
            "runtime_environment": current_runtime_environment(),
            "runtime_milliseconds": 0,
            "scope": "smoke" if effective_smoke else "production",
            "schema_version": SCHEMA_VERSION,
            "sequence_number": 0,
            "state": "running",
            "tool_sha256": identities["tool_sha256"],
        }
        atomic_publish(temporary / MANIFEST_NAME, canonical_bytes(manifest))
        for directory in (
            temporary,
            *(temporary / name for name in ("chunks", "certificates", "repros")),
        ):
            fsync_directory(directory)
        os.replace(temporary, checkpoint_dir)
        fsync_directory(parent)
        return manifest
    finally:
        if temporary.exists():
            shutil.rmtree(temporary)


def load_checkpoint(checkpoint_dir: Path) -> dict[str, Any]:
    value, _ = load_canonical_file(
        checkpoint_dir / MANIFEST_NAME, "campaign checkpoint"
    )
    manifest = validate_manifest(value, "campaign checkpoint")
    validate_committed_chunks(checkpoint_dir, manifest)
    if manifest["state"] == "complete":
        _, certificate_content = validate_artifact(
            checkpoint_dir, manifest["certificate_artifact"], "campaign certificate"
        )
        if certificate_content != canonical_bytes(certificate_for(manifest)):
            raise ValueError(
                "the campaign certificate differs from its closed scientific state"
            )
        alias = checkpoint_dir / RESULT_NAME
        if not alias.exists():
            atomic_publish(alias, certificate_content)
        else:
            require_regular_file(alias, "campaign result alias")
            if alias.read_bytes() != certificate_content:
                raise ValueError(
                    "the campaign result alias differs from its content-addressed certificate"
                )
    return manifest


def certificate_for(manifest: dict[str, Any]) -> dict[str, Any]:
    counts = validate_counts(manifest["counts"], "final counts")
    if (
        counts["base_case_count"] != manifest["base_case_count"]
        or counts["wrong_sign"] != 0
        or counts["remaining_unknown"] != 0
    ):
        raise ValueError(
            "the phase 2A campaign cannot certify an incomplete or wrong corpus"
        )
    if any(counts["by_stratum"][stratum] == 0 for stratum in STRATA):
        raise ValueError("the phase 2A campaign omitted a frozen stratum")
    if any(
        counts["predicate_strata"][predicate][stratum] == 0
        for predicate in PREDICATES
        for stratum in STRATA
    ):
        raise ValueError("the phase 2A campaign omitted a predicate/stratum cell")
    if any(counts["by_metamorphism"][name] == 0 for name in METAMORPHISMS):
        raise ValueError("the phase 2A campaign omitted a frozen metamorphism")
    if (
        manifest["ordered_roots"]["corpus"] != manifest["expected_corpus_sha256"]
        or manifest["ordered_roots"]["oracle"] != manifest["expected_oracle_sha256"]
    ):
        raise ValueError("the completed corpus or oracle root drifted from its config")
    if manifest["scope"] == "production" and (
        manifest["base_case_count"] < MIN_PRODUCTION_BASE_CASES
        or not manifest["repository_clean"]
        or manifest["effective_smoke"]
    ):
        raise ValueError("a production certificate requires >=10M clean base signs")
    output = {
        "counts": counts,
        "wrong_sign": 0,
        "remaining_unknown": 0,
    }
    return {
        "base_case_count": manifest["base_case_count"],
        "build_metadata": manifest["build_metadata"],
        "build_metadata_sha256": manifest["build_metadata_sha256"],
        "catalog_sha256": manifest["catalog_sha256"],
        "config_sha256": manifest["config_sha256"],
        "corpus_descriptor_sha256": manifest["corpus_descriptor_sha256"],
        "corpus_sha256": manifest["ordered_roots"]["corpus"],
        "counts": counts,
        "diagnostics": {
            **manifest["runtime_environment"],
            "runtime_milliseconds": manifest["runtime_milliseconds"],
        },
        "expected_corpus_sha256": manifest["expected_corpus_sha256"],
        "expected_oracle_sha256": manifest["expected_oracle_sha256"],
        "frozen_config_sha256": manifest["frozen_config_sha256"],
        "gcp_used": False,
        "git_head": manifest["git_head"],
        "kind": CERTIFICATE_KIND,
        "native_executable_sha256": manifest["native_executable_sha256"],
        "oracle_descriptor_sha256": manifest["oracle_descriptor_sha256"],
        "oracle_sha256": manifest["ordered_roots"]["oracle"],
        "ordered_root_algorithm": manifest["ordered_root_algorithm"],
        "output_sha256": manifest["ordered_roots"]["output"],
        "repository_clean": manifest["repository_clean"],
        "repository_end_verified": True,
        "repository_phase_exit_claimed": False,
        "schema_version": SCHEMA_VERSION,
        "scientific_summary_sha256": canonical_hash(output),
        "scope": manifest["scope"],
        "tool_sha256": manifest["tool_sha256"],
        "total_sign_count": counts["total_sign_count"],
    }


def finalize(checkpoint_dir: Path, manifest: dict[str, Any]) -> dict[str, Any]:
    certificate = certificate_for(manifest)
    content = canonical_bytes(certificate)
    digest = sha256_bytes(content)
    relative_path = f"certificates/certificate-{digest}.json"
    path = checkpoint_dir / relative_path
    atomic_publish(path, content)
    maybe_fail_after("certificate_artifact")
    reference = artifact_reference(relative_path, content)
    manifest["certificate_artifact"] = reference
    manifest["sequence_number"] += 1
    manifest["state"] = "complete"
    publish_manifest(checkpoint_dir, manifest)
    maybe_fail_after("complete_manifest")
    atomic_publish(checkpoint_dir / RESULT_NAME, content)
    return manifest


def nonnegative(value: str) -> int:
    parsed = int(value)
    if parsed < 0:
        raise argparse.ArgumentTypeError("the value must be nonnegative")
    return parsed


def positive(value: str) -> int:
    parsed = int(value)
    if parsed <= 0:
        raise argparse.ArgumentTypeError("the value must be positive")
    return parsed


def precompute_ordered_roots(
    config: dict[str, Any], base_count: int
) -> tuple[dict[str, str], int]:
    roots = initial_ordered_roots()
    seed = int(config["generator"]["seed"], 16)
    stride = config["metamorphisms"]["stride"]
    metamorphic_count = 0
    for case in cases_for_range(seed, 0, base_count, stride):
        command = (case.command() + "\n").encode("utf-8")
        expected_sign = oracle_sign(case)
        oracle_row = canonical_bytes(
            {
                "base_index": case.base_index,
                "predicate": case.predicate,
                "relation": case.relation,
                "sign": expected_sign,
                "stratum": case.stratum,
            }
        )
        roots["corpus"] = extend_ordered_root("corpus", roots["corpus"], command)
        roots["oracle"] = extend_ordered_root("oracle", roots["oracle"], oracle_row)
        metamorphic_count += case.relation != "base"
    return roots, metamorphic_count


def precompute_mode(arguments: argparse.Namespace) -> dict[str, Any]:
    config_path = arguments.config.resolve()
    repository = arguments.repository.resolve()
    output_path = arguments.precompute_roots.resolve()
    tool = Path(__file__).resolve()
    try:
        output_path.relative_to(repository)
    except ValueError:
        pass
    else:
        raise ValueError(
            "precomputed roots must be written outside the measured worktree"
        )
    config_value, config_content = load_canonical_file(config_path, "campaign config")
    config = validate_config(config_value)
    effective_smoke = arguments.smoke_base_case_count is not None
    if arguments.allow_dirty_repository and not effective_smoke:
        raise ValueError(
            "--allow-dirty-repository is restricted to an explicit smoke corpus"
        )
    base_count = config["base_case_count"]
    if effective_smoke:
        base_count = require_integer(
            arguments.smoke_base_case_count,
            "smoke base case count",
            minimum=SMOKE_MINIMUM,
            maximum=SMOKE_MAXIMUM,
        )
    snapshot = repository_snapshot(repository)
    verify_repository(repository, snapshot, arguments.allow_dirty_repository)
    scope = "smoke" if effective_smoke else "production"
    if scope == "production":
        verify_head_blob(repository, tool, "campaign tool")
        verify_head_blob(repository, config_path, "campaign config")
    tool_sha256 = sha256_file(tool)
    config_sha256 = sha256_bytes(config_content)
    roots, metamorphic_count = precompute_ordered_roots(config, base_count)
    verify_repository(repository, snapshot, arguments.allow_dirty_repository)
    if sha256_file(tool) != tool_sha256 or sha256_file(config_path) != config_sha256:
        raise ValueError("precompute inputs changed while roots were generated")
    artifact = {
        "base_case_count": base_count,
        "config_source_sha256": config_sha256,
        "corpus_sha256": roots["corpus"],
        "gcp_used": False,
        "git_head": snapshot["git_head"],
        "kind": PRECOMPUTE_KIND,
        "metamorphic_sign_count": metamorphic_count,
        "oracle_sha256": roots["oracle"],
        "ordered_root_algorithm": ORDERED_ROOT_ALGORITHM,
        "repository_clean": snapshot["repository_clean"],
        "repository_phase_exit_claimed": False,
        "schema_version": SCHEMA_VERSION,
        "scope": scope,
        "tool_sha256": tool_sha256,
    }
    output_path.parent.mkdir(parents=True, exist_ok=True)
    atomic_publish(output_path, canonical_bytes(artifact))
    return artifact


def campaign(arguments: argparse.Namespace) -> dict[str, Any]:
    invocation_accounted_at = time.monotonic()
    native = arguments.native.resolve()
    config_path = arguments.config.resolve()
    checkpoint_dir = arguments.checkpoint_dir.resolve()
    repository = arguments.repository.resolve()
    try:
        checkpoint_dir.relative_to(repository)
    except ValueError:
        pass
    else:
        raise ValueError(
            "the checkpoint directory must stay outside the measured Git worktree"
        )
    tool = Path(__file__).resolve()
    require_regular_file(native, "native predicate replay")
    if not os.access(native, os.X_OK):
        raise ValueError("native predicate replay is not executable")
    config_value, config_content = load_canonical_file(config_path, "campaign config")
    config = validate_config(config_value, require_expected_roots=True)
    effective_smoke = arguments.smoke_base_case_count is not None
    scope = "smoke" if effective_smoke else "production"
    if arguments.allow_dirty_repository and not effective_smoke:
        raise ValueError(
            "--allow-dirty-repository is restricted to an explicit smoke corpus"
        )
    base_count = config["base_case_count"]
    chunk_size = config["chunk_size_base_cases"]
    if effective_smoke:
        base_count = require_integer(
            arguments.smoke_base_case_count,
            "smoke base case count",
            minimum=SMOKE_MINIMUM,
            maximum=SMOKE_MAXIMUM,
        )
        chunk_size = min(
            chunk_size, arguments.smoke_chunk_size or chunk_size, base_count
        )
        require_integer(
            chunk_size, "smoke chunk size", minimum=1, maximum=MAX_CHUNK_BASE_CASES
        )
    snapshot = repository_snapshot(repository)
    if not arguments.allow_dirty_repository and not snapshot["repository_clean"]:
        raise ValueError("the long campaign refuses a dirty Git worktree")
    effective_config = dict(config)
    effective_config["base_case_count"] = base_count
    effective_config["chunk_size_base_cases"] = chunk_size
    identities = identities_for(effective_config, config_content, tool, native)
    identities["config_sha256"] = canonical_hash(
        {
            "frozen_config_sha256": sha256_bytes(config_content),
            "smoke_base_case_count": base_count if effective_smoke else None,
            "smoke_chunk_size": chunk_size if effective_smoke else None,
        }
    )
    cache = discover_cmake_cache(native, arguments.cmake_cache)
    build_metadata = discover_build_metadata(cache, native)
    identities["build_metadata_sha256"] = canonical_hash(build_metadata)
    verify_runtime_inputs(
        native=native,
        tool=tool,
        config_path=config_path,
        cache=cache,
        identities=identities,
        expected_build_metadata=build_metadata,
        repository=repository,
        scope=scope,
    )
    lock_path = checkpoint_dir.parent / f".{checkpoint_dir.name}.lock"
    checkpoint_dir.parent.mkdir(parents=True, exist_ok=True)
    lock_descriptor = os.open(lock_path, os.O_CREAT | os.O_RDWR, 0o600)
    try:
        fcntl.flock(lock_descriptor, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError as error:
        os.close(lock_descriptor)
        raise ValueError("another writer holds the POSIX campaign lock") from error
    try:
        verify_repository(repository, snapshot, arguments.allow_dirty_repository)
        verify_runtime_inputs(
            native=native,
            tool=tool,
            config_path=config_path,
            cache=cache,
            identities=identities,
            expected_build_metadata=build_metadata,
            repository=repository,
            scope=scope,
        )
        if (checkpoint_dir / MANIFEST_NAME).is_file():
            manifest = load_checkpoint(checkpoint_dir)
            expected_identity = {
                **identities,
                "base_case_count": base_count,
                "chunk_size_base_cases": chunk_size,
                "effective_smoke": effective_smoke,
                "expected_corpus_sha256": config["expected_corpus_sha256"],
                "expected_oracle_sha256": config["expected_oracle_sha256"],
                "ordered_root_algorithm": ORDERED_ROOT_ALGORITHM,
                "scope": scope,
            }
            for field, expected in expected_identity.items():
                if manifest.get(field) != expected:
                    raise ValueError(f"campaign checkpoint mismatch: {field}")
            if manifest["runtime_environment"] != current_runtime_environment():
                raise ValueError("campaign checkpoint mismatch: runtime_environment")
            if manifest["build_metadata"] != build_metadata:
                raise ValueError("campaign checkpoint mismatch: build_metadata")
            expected_snapshot = {
                "git_head": manifest["git_head"],
                "git_status_sha256": manifest["git_status_sha256"],
                "repository_clean": manifest["repository_clean"],
            }
            verify_repository(
                repository, expected_snapshot, arguments.allow_dirty_repository
            )
        else:
            manifest = initialize_checkpoint(
                checkpoint_dir,
                identities,
                snapshot,
                base_count,
                chunk_size,
                effective_smoke,
                config["expected_corpus_sha256"],
                config["expected_oracle_sha256"],
                build_metadata,
            )
        if manifest["state"] == "complete":
            verify_repository(repository, snapshot, arguments.allow_dirty_repository)
            return manifest
        seed = int(config["generator"]["seed"], 16)
        stride = config["metamorphisms"]["stride"]
        transactions = 0
        while (
            manifest["next_base_index"] < base_count
            and transactions < arguments.max_chunks
        ):
            verify_repository(repository, snapshot, arguments.allow_dirty_repository)
            verify_runtime_inputs(
                native=native,
                tool=tool,
                config_path=config_path,
                cache=cache,
                identities=identities,
                expected_build_metadata=build_metadata,
                repository=repository,
                scope=scope,
            )
            begin = manifest["next_base_index"]
            end = min(begin + chunk_size, base_count)
            chunk, content = execute_chunk(
                native,
                checkpoint_dir,
                seed,
                stride,
                begin,
                end,
                arguments.timeout_seconds,
                identities,
                manifest["ordered_roots"],
            )
            verify_repository(repository, snapshot, arguments.allow_dirty_repository)
            verify_runtime_inputs(
                native=native,
                tool=tool,
                config_path=config_path,
                cache=cache,
                identities=identities,
                expected_build_metadata=build_metadata,
                repository=repository,
                scope=scope,
            )
            relative_path = chunk_relative_path(len(manifest["chunks"]))
            atomic_publish(checkpoint_dir / relative_path, content)
            maybe_fail_after("chunk_artifact")
            manifest["chunks"].append(artifact_reference(relative_path, content))
            add_counts(manifest["counts"], chunk["counts"])
            manifest["next_base_index"] = end
            manifest["ordered_roots"] = chunk["ordered_roots_after"]
            now = time.monotonic()
            manifest["runtime_milliseconds"] += max(
                0, int((now - invocation_accounted_at) * 1000)
            )
            invocation_accounted_at = now
            manifest["sequence_number"] += 1
            publish_manifest(checkpoint_dir, manifest)
            maybe_fail_after("chunk_manifest")
            transactions += 1
        if manifest["next_base_index"] == base_count:
            verify_repository(repository, snapshot, arguments.allow_dirty_repository)
            verify_runtime_inputs(
                native=native,
                tool=tool,
                config_path=config_path,
                cache=cache,
                identities=identities,
                expected_build_metadata=build_metadata,
                repository=repository,
                scope=scope,
            )
            now = time.monotonic()
            manifest["runtime_milliseconds"] += max(
                0, int((now - invocation_accounted_at) * 1000)
            )
            manifest = finalize(checkpoint_dir, manifest)
        verify_repository(repository, snapshot, arguments.allow_dirty_repository)
        verify_runtime_inputs(
            native=native,
            tool=tool,
            config_path=config_path,
            cache=cache,
            identities=identities,
            expected_build_metadata=build_metadata,
            repository=repository,
            scope=scope,
        )
        return manifest
    finally:
        fcntl.flock(lock_descriptor, fcntl.LOCK_UN)
        os.close(lock_descriptor)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", type=Path)
    parser.add_argument("--config", required=True, type=Path)
    parser.add_argument("--checkpoint-dir", type=Path)
    parser.add_argument("--repository", required=True, type=Path)
    parser.add_argument("--max-chunks", type=nonnegative)
    parser.add_argument("--cmake-cache", type=Path)
    parser.add_argument("--precompute-roots", type=Path)
    parser.add_argument("--timeout-seconds", type=positive, default=300)
    parser.add_argument("--smoke-base-case-count", type=positive)
    parser.add_argument("--smoke-chunk-size", type=positive)
    parser.add_argument("--allow-dirty-repository", action="store_true")
    arguments = parser.parse_args()
    if (
        arguments.smoke_chunk_size is not None
        and arguments.smoke_base_case_count is None
    ):
        parser.error("--smoke-chunk-size requires --smoke-base-case-count")
    if arguments.precompute_roots is not None:
        if any(
            value is not None
            for value in (
                arguments.native,
                arguments.checkpoint_dir,
                arguments.max_chunks,
                arguments.cmake_cache,
                arguments.smoke_chunk_size,
            )
        ):
            parser.error(
                "--precompute-roots does not accept native/checkpoint/chunk/build options"
            )
    elif any(
        value is None
        for value in (
            arguments.native,
            arguments.checkpoint_dir,
            arguments.max_chunks,
        )
    ):
        parser.error(
            "campaign mode requires --native, --checkpoint-dir, and --max-chunks"
        )
    try:
        if arguments.precompute_roots is not None:
            result = precompute_mode(arguments)
        else:
            result = campaign(arguments)
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"predicate campaign failed closed: {error}", file=sys.stderr)
        return 2
    if arguments.precompute_roots is not None:
        summary = {
            "base_case_count": result["base_case_count"],
            "corpus_sha256": result["corpus_sha256"],
            "metamorphic_sign_count": result["metamorphic_sign_count"],
            "oracle_sha256": result["oracle_sha256"],
            "scope": result["scope"],
        }
    else:
        summary = {
            "base_cases_committed": result["counts"]["base_case_count"],
            "complete": result["state"] == "complete",
            "metamorphic_signs_committed": result["counts"]["metamorphic_sign_count"],
            "total_signs_committed": result["counts"]["total_sign_count"],
        }
    print(canonical_json(summary))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
