#!/usr/bin/env python3
"""Validate the versioned MorseHGP3D JSON contract without third-party code."""

from __future__ import annotations

import hashlib
import json
import math
import re
import sys
from collections import Counter
from fractions import Fraction
from itertools import combinations
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
SCHEMA_PATH = ROOT / "schemas" / "morsehgp3d-contract-v2.schema.json"
LEGACY_SCHEMA_PATH = ROOT / "schemas" / "morsehgp3d-contract-v1.schema.json"
FIXTURE_DIR = ROOT / "tests" / "fixtures" / "contracts"
CONTRACT_DOMAIN = "MorseHGP3D/v2"

REQUIRED_DEFINITIONS = {
    "InputSemantics",
    "CertifiedPoint3",
    "ExactRational3",
    "ExactLevel",
    "VertexWitness",
    "CriticalEvent",
    "GammaCoface",
    "GabrielHyperedge",
    "Attachment",
    "EqualLevelBatch",
    "MergeForest",
    "VerticalMap",
    "MorseHGP3DResult",
    "RunCertificate",
    "PartialScope",
    "FragmentHint",
    "CanonicalCellCertificate",
    "BudgetPolicy",
    "BudgetSnapshot",
    "CheckpointManifest",
    "BenchmarkRecord",
}


class ContractError(ValueError):
    """Raised when a schema or contract instance violates the active v2 rules."""


def _object_without_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ContractError(f"duplicate JSON key {key!r}")
        result[key] = value
    return result


def load_json(path: Path) -> Any:
    try:
        return json.loads(
            path.read_text(encoding="utf-8"),
            object_pairs_hook=_object_without_duplicate_keys,
        )
    except (OSError, json.JSONDecodeError, ContractError) as exc:
        raise ContractError(f"{path.relative_to(ROOT)}: {exc}") from exc


def canonical_json(value: Any) -> str:
    """Return the byte-independent canonical JSON text used by phase-0 tests."""

    return json.dumps(
        value,
        ensure_ascii=False,
        allow_nan=False,
        sort_keys=True,
        separators=(",", ":"),
    )


def _without_schema_versions(value: Any) -> Any:
    """Remove record-version metadata from a scientific identity projection."""

    if isinstance(value, dict):
        return {
            key: _without_schema_versions(child)
            for key, child in value.items()
            if key != "schema_version"
        }
    if isinstance(value, (list, tuple)):
        return [_without_schema_versions(child) for child in value]
    return value


def canonical_contract_id(record_type: str, identity_projection: Any) -> str:
    """Return a v2 domain-separated identifier for an identity projection."""

    if not record_type or "/" in record_type:
        raise ContractError("record_type must be a non-empty path component")
    digest = hashlib.sha256()
    digest.update(f"{CONTRACT_DOMAIN}/{record_type}/".encode("ascii"))
    digest.update(
        canonical_json(_without_schema_versions(identity_projection)).encode("utf-8")
    )
    return digest.hexdigest()


def _exact_level_key(level: dict[str, Any]) -> Fraction:
    """Return the mathematical value of an exact squared-level record."""

    return Fraction(int(level["numerator"]), int(level["denominator"]))


def _binary64_total_order_key(bits: str) -> int:
    """Return the unsigned key frozen for canonical finite binary64 inputs."""

    if re.fullmatch(r"(?![7f]ff)(?!8000000000000000$)[0-9a-f]{16}", bits) is None:
        raise ContractError("an embedded coordinate is not canonical finite binary64")
    word = int(bits, 16)
    if word >> 63:
        return (~word) & ((1 << 64) - 1)
    return word ^ (1 << 63)


def _canonical_integer(value: Any, *, positive: bool, path: str) -> int:
    """Parse one canonical arbitrary-precision decimal integer string."""

    if not isinstance(value, str):
        raise ContractError(f"{path}: expected a canonical decimal string")
    pattern = r"^[1-9][0-9]*$" if positive else r"^(0|-?[1-9][0-9]*)$"
    if re.fullmatch(pattern, value) is None:
        raise ContractError(f"{path}: integer string is not canonical")
    parsed = int(value)
    if positive and parsed <= 0:
        raise ContractError(f"{path}: denominator must be positive")
    return parsed


def validate_canonical_rationals(value: Any, path: str = "$") -> None:
    """Reject non-reduced exact rational records anywhere in a contract."""

    if isinstance(value, dict):
        keys = set(value)
        if {
            "x_numerator",
            "y_numerator",
            "z_numerator",
            "denominator",
            "unit",
        } <= keys:
            numerators = [
                _canonical_integer(
                    value[field], positive=False, path=f"{path}.{field}"
                )
                for field in ("x_numerator", "y_numerator", "z_numerator")
            ]
            denominator = _canonical_integer(
                value["denominator"],
                positive=True,
                path=f"{path}.denominator",
            )
            divisor = math.gcd(
                math.gcd(abs(numerators[0]), abs(numerators[1])),
                math.gcd(abs(numerators[2]), denominator),
            )
            if divisor != 1:
                raise ContractError(f"{path}: ExactRational3 is not reduced")
        elif {"numerator", "denominator", "unit"} <= keys:
            numerator = _canonical_integer(
                value["numerator"], positive=False, path=f"{path}.numerator"
            )
            if numerator < 0:
                raise ContractError(f"{path}: ExactLevel numerator must be non-negative")
            denominator = _canonical_integer(
                value["denominator"],
                positive=True,
                path=f"{path}.denominator",
            )
            if math.gcd(numerator, denominator) != 1:
                raise ContractError(f"{path}: ExactLevel is not reduced")
        elif keys == {"schema_version", "numerator", "denominator"}:
            numerator = _canonical_integer(
                value["numerator"], positive=True, path=f"{path}.numerator"
            )
            denominator = _canonical_integer(
                value["denominator"],
                positive=True,
                path=f"{path}.denominator",
            )
            if math.gcd(numerator, denominator) != 1:
                raise ContractError(f"{path}: ExactPositiveRational is not reduced")
        for key, child in value.items():
            validate_canonical_rationals(child, f"{path}.{key}")
    elif isinstance(value, list):
        for index, child in enumerate(value):
            validate_canonical_rationals(child, f"{path}[{index}]")


def _canonical_id(
    actual: Any, record_type: str, projection: Any, context: str
) -> None:
    expected = canonical_contract_id(record_type, projection)
    if actual != expected:
        raise ContractError(
            f"{context} is not its canonical MorseHGP3D/v2 identifier"
        )


def _index_records(
    records: list[dict[str, Any]], id_field: str, context: str
) -> dict[str, dict[str, Any]]:
    indexed: dict[str, dict[str, Any]] = {}
    for record in records:
        record_id = record[id_field]
        if record_id in indexed:
            raise ContractError(f"{context} must have unique {id_field} values")
        indexed[record_id] = record
    return indexed


def _require_references(
    references: list[str], indexed: dict[str, Any], context: str
) -> None:
    unknown = [reference for reference in references if reference not in indexed]
    if unknown:
        raise ContractError(f"{context} references unknown identifiers {unknown!r}")


def _is_type(value: Any, expected: str) -> bool:
    if expected == "null":
        return value is None
    if expected == "boolean":
        return isinstance(value, bool)
    if expected == "integer":
        return isinstance(value, int) and not isinstance(value, bool)
    if expected == "number":
        return (
            isinstance(value, (int, float))
            and not isinstance(value, bool)
            and math.isfinite(value)
        )
    if expected == "string":
        return isinstance(value, str)
    if expected == "array":
        return isinstance(value, list)
    if expected == "object":
        return isinstance(value, dict)
    raise ContractError(f"unsupported schema type {expected!r}")


class SchemaValidator:
    """Small Draft 2020-12 subset sufficient for the frozen local contract."""

    def __init__(self, schema: dict[str, Any]) -> None:
        self.schema = schema

    def resolve_ref(self, ref: str) -> Any:
        if not ref.startswith("#/"):
            raise ContractError(f"external schema reference is forbidden: {ref!r}")
        target: Any = self.schema
        for raw_part in ref[2:].split("/"):
            part = raw_part.replace("~1", "/").replace("~0", "~")
            if not isinstance(target, dict) or part not in target:
                raise ContractError(f"unresolved schema reference {ref!r}")
            target = target[part]
        return target

    def check(self, instance: Any, schema: Any | None = None, path: str = "$") -> None:
        current = self.schema if schema is None else schema
        if current is True:
            return
        if current is False:
            raise ContractError(f"{path}: value rejected by false schema")
        if not isinstance(current, dict):
            raise ContractError(f"{path}: schema node is not an object or boolean")

        if "$ref" in current:
            self.check(instance, self.resolve_ref(current["$ref"]), path)

        if "allOf" in current:
            for index, branch in enumerate(current["allOf"]):
                self.check(instance, branch, f"{path}.allOf[{index}]")
        if "anyOf" in current:
            matches = self._matching_branches(instance, current["anyOf"], path)
            if not matches:
                raise ContractError(f"{path}: value matches no anyOf branch")
        if "oneOf" in current:
            matches = self._matching_branches(instance, current["oneOf"], path)
            if len(matches) != 1:
                raise ContractError(
                    f"{path}: value must match exactly one oneOf branch, got {len(matches)}"
                )
        if "not" in current:
            try:
                self.check(instance, current["not"], path)
            except ContractError:
                pass
            else:
                raise ContractError(f"{path}: value matches forbidden schema")
        if "if" in current:
            try:
                self.check(instance, current["if"], path)
            except ContractError:
                branch = current.get("else")
            else:
                branch = current.get("then")
            if branch is not None:
                self.check(instance, branch, path)

        if "const" in current and instance != current["const"]:
            raise ContractError(f"{path}: expected constant {current['const']!r}")
        if "enum" in current and instance not in current["enum"]:
            raise ContractError(f"{path}: {instance!r} is not in {current['enum']!r}")

        declared_type = current.get("type")
        if declared_type is not None:
            expected_types = (
                declared_type if isinstance(declared_type, list) else [declared_type]
            )
            if not any(_is_type(instance, expected) for expected in expected_types):
                raise ContractError(
                    f"{path}: expected type {expected_types!r}, got {type(instance).__name__}"
                )

        if isinstance(instance, dict):
            self._check_object(instance, current, path)
        elif isinstance(instance, list):
            self._check_array(instance, current, path)
        elif isinstance(instance, str):
            self._check_string(instance, current, path)
        elif isinstance(instance, (int, float)) and not isinstance(instance, bool):
            self._check_number(instance, current, path)

    def _matching_branches(
        self, instance: Any, branches: list[Any], path: str
    ) -> list[int]:
        matches: list[int] = []
        for index, branch in enumerate(branches):
            try:
                self.check(instance, branch, path)
            except ContractError:
                continue
            matches.append(index)
        return matches

    def _check_object(
        self, instance: dict[str, Any], schema: dict[str, Any], path: str
    ) -> None:
        required = schema.get("required", [])
        missing = [key for key in required if key not in instance]
        if missing:
            raise ContractError(f"{path}: missing required properties {missing!r}")

        properties = schema.get("properties", {})
        pattern_properties = schema.get("patternProperties", {})
        for key, value in instance.items():
            matched = False
            if key in properties:
                self.check(value, properties[key], f"{path}.{key}")
                matched = True
            for pattern, subschema in pattern_properties.items():
                if re.search(pattern, key):
                    self.check(value, subschema, f"{path}.{key}")
                    matched = True
            if matched:
                continue
            additional = schema.get("additionalProperties", True)
            if additional is False:
                raise ContractError(f"{path}: unknown property {key!r}")
            if isinstance(additional, dict):
                self.check(value, additional, f"{path}.{key}")

        if len(instance) < schema.get("minProperties", 0):
            raise ContractError(f"{path}: too few properties")
        if "maxProperties" in schema and len(instance) > schema["maxProperties"]:
            raise ContractError(f"{path}: too many properties")

    def _check_array(
        self, instance: list[Any], schema: dict[str, Any], path: str
    ) -> None:
        if len(instance) < schema.get("minItems", 0):
            raise ContractError(f"{path}: too few items")
        if "maxItems" in schema and len(instance) > schema["maxItems"]:
            raise ContractError(f"{path}: too many items")
        if schema.get("uniqueItems"):
            encoded = [canonical_json(value) for value in instance]
            if len(encoded) != len(set(encoded)):
                raise ContractError(f"{path}: array items must be unique")
        canonical_order = schema.get("x-canonical-order")
        if canonical_order is not None:
            self._check_canonical_order(instance, canonical_order, path)

        prefix_items = schema.get("prefixItems", [])
        for index, subschema in enumerate(prefix_items):
            if index < len(instance):
                self.check(instance[index], subschema, f"{path}[{index}]")
        items = schema.get("items")
        if items is not None:
            start = len(prefix_items) if prefix_items else 0
            for index in range(start, len(instance)):
                self.check(instance[index], items, f"{path}[{index}]")

    @staticmethod
    def _check_canonical_order(
        instance: list[Any], rule: str, path: str
    ) -> None:
        if rule == "ascending":
            keys = instance
        elif rule == "lexicographic":
            keys = [canonical_json(value) for value in instance]
        elif isinstance(rule, str) and rule.startswith("by:"):
            field = rule[3:]
            if not field or any(
                not isinstance(value, dict) or field not in value for value in instance
            ):
                raise ContractError(
                    f"{path}: canonical order {rule!r} requires field {field!r}"
                )
            keys = [value[field] for value in instance]
        else:
            raise ContractError(f"{path}: unsupported x-canonical-order {rule!r}")
        try:
            ordered = all(left <= right for left, right in zip(keys, keys[1:]))
        except TypeError as exc:
            raise ContractError(
                f"{path}: values cannot be compared for canonical order {rule!r}"
            ) from exc
        if not ordered:
            raise ContractError(
                f"{path}: items do not follow canonical order {rule!r}"
            )

    @staticmethod
    def _check_string(instance: str, schema: dict[str, Any], path: str) -> None:
        if len(instance) < schema.get("minLength", 0):
            raise ContractError(f"{path}: string is too short")
        if "maxLength" in schema and len(instance) > schema["maxLength"]:
            raise ContractError(f"{path}: string is too long")
        if "pattern" in schema and re.search(schema["pattern"], instance) is None:
            raise ContractError(
                f"{path}: string {instance!r} does not match {schema['pattern']!r}"
            )

    @staticmethod
    def _check_number(
        instance: int | float, schema: dict[str, Any], path: str
    ) -> None:
        if "minimum" in schema and instance < schema["minimum"]:
            raise ContractError(f"{path}: value is below minimum")
        if "maximum" in schema and instance > schema["maximum"]:
            raise ContractError(f"{path}: value is above maximum")
        if "exclusiveMinimum" in schema and instance <= schema["exclusiveMinimum"]:
            raise ContractError(f"{path}: value is not above exclusiveMinimum")
        if "exclusiveMaximum" in schema and instance >= schema["exclusiveMaximum"]:
            raise ContractError(f"{path}: value is not below exclusiveMaximum")
        if "multipleOf" in schema:
            quotient = instance / schema["multipleOf"]
            if not math.isclose(quotient, round(quotient), rel_tol=0.0, abs_tol=1e-12):
                raise ContractError(f"{path}: value is not a multipleOf")


def validate_round_trip(value: Any, label: str) -> None:
    encoded = canonical_json(value)
    decoded = json.loads(encoded, object_pairs_hook=_object_without_duplicate_keys)
    if decoded != value:
        raise ContractError(f"{label}: canonical JSON round-trip changed the value")
    if canonical_json(decoded) != encoded:
        raise ContractError(f"{label}: canonical JSON serialization is unstable")


def validate_certificate_semantics(
    certificate: dict[str, Any], *, check_canonical_ids: bool = True
) -> None:
    """Check cross-field guarantees that JSON Schema cannot express readably."""

    validate_canonical_rationals(certificate, "run_certificate")

    n = certificate["input_point_count"]
    k_eff = certificate["k_eff"]
    expected_k_eff = min(certificate["k_max"], n)
    if k_eff != expected_k_eff:
        raise ContractError(
            f"run_certificate.k_eff={k_eff} but min(k_max,n)={expected_k_eff}"
        )
    expected_s_max = min(k_eff + 1, n)
    if certificate["s_max"] != expected_s_max:
        raise ContractError(
            "run_certificate.s_max does not equal min(k_eff + 1, input_point_count)"
        )
    expected_m_star = 0 if n == 1 else min(k_eff - 1, n - 2)
    if certificate["m_star"] != expected_m_star:
        raise ContractError(
            "run_certificate.m_star does not match the frozen shallow-depth rule"
        )

    expected_lengths = {
        "catalog_complete_by_rank": expected_s_max,
        "attachments_complete_by_order": k_eff,
        "gamma_complete_by_order": k_eff,
        "batches_complete_by_order": k_eff,
    }
    for field, expected in expected_lengths.items():
        if len(certificate[field]) != expected:
            raise ContractError(
                f"run_certificate.{field} must contain {expected} positional entries"
            )

    mode = certificate["requested_mode"]
    status = certificate["public_status"]
    semantics = certificate.get("forest_semantics")
    proof_basis = certificate["proof_basis"]
    profile = certificate["effective_profile"]

    exact_markers = {
        "forest_semantics=exact": semantics == "exact",
        "public_status=exact": status == "exact",
        "result_kind=hierarchy": certificate["result_kind"] == "hierarchy",
    }
    if len(set(exact_markers.values())) != 1:
        raise ContractError(
            "exact forest semantics, exact public status and hierarchy result kind "
            "must be equivalent"
        )
    if any(exact_markers.values()) and not certificate["require_exact"]:
        raise ContractError("exact hierarchy publication requires require_exact=true")

    if certificate["budget_policy"]["mode"] != mode:
        raise ContractError("budget_policy.mode must match requested_mode")
    if certificate["budget_policy"]["require_exact"] != certificate["require_exact"]:
        raise ContractError("budget_policy.require_exact must match run certificate")
    if mode == "budgeted" and status == "exact":
        raise ContractError("a budgeted run can never publish public_status=exact")
    if status == "exact":
        if certificate["partial_guarantees"] != ["none"]:
            raise ContractError("public_status=exact requires partial_guarantees=[none]")
    elif status in {"conditional", "budget_exhausted"} and "none" in certificate[
        "partial_guarantees"
    ]:
        raise ContractError(
            "a conditional or budget result cannot claim partial_guarantees=none"
        )
    if semantics == "partial_refinement" and certificate["require_exact"]:
        raise ContractError("partial_refinement is incompatible with require_exact=true")
    if semantics == "partial_refinement" and status not in {
        "conditional",
        "budget_exhausted",
    }:
        raise ContractError("partial_refinement requires a conditional or budget status")

    if proof_basis == "gabriel_positive_connectivity":
        if profile != "hgp_reduced":
            raise ContractError(
                "gabriel_positive_connectivity is reserved for hgp_reduced"
            )
        if certificate["result_kind"] != "partial_hierarchy":
            raise ContractError(
                "gabriel_positive_connectivity requires result_kind=partial_hierarchy"
            )
        if semantics != "partial_refinement":
            raise ContractError(
                "gabriel_positive_connectivity requires partial_refinement semantics"
            )
        if certificate["require_exact"]:
            raise ContractError(
                "gabriel_positive_connectivity is incompatible with require_exact=true"
            )
        if status not in {"conditional", "budget_exhausted"}:
            raise ContractError(
                "gabriel_positive_connectivity permits only conditional or budget status"
            )
        if "positive_connectivity" not in certificate["partial_guarantees"]:
            raise ContractError(
                "gabriel_positive_connectivity requires its positive guarantee"
            )

    if profile == "full_pi0" and semantics == "exact":
        raise ContractError("the active v2 contract cannot publish full_pi0 exact")
    if status == "exact":
        if certificate["result_kind"] != "hierarchy":
            raise ContractError("public_status=exact requires a hierarchy result")
        if semantics != "exact":
            raise ContractError("public_status=exact requires forest_semantics=exact")
        if certificate["stop_reason"] != "completed":
            raise ContractError("public_status=exact requires stop_reason=completed")
        if certificate["general_position_status"] not in {
            "verified_relevant",
            "not_applicable",
        }:
            raise ContractError("public_status=exact requires a closed input domain")
        scalar_gates = (
            "relevant_gp_complete",
            "canonical_children_complete",
            "active_cross_incidences_complete",
            "vertical_maps_complete",
        )
        if not all(certificate[field] for field in scalar_gates):
            raise ContractError("public_status=exact has a false completeness gate")
        vector_gates = (
            "catalog_complete_by_rank",
            "attachments_complete_by_order",
            "gamma_complete_by_order",
            "batches_complete_by_order",
        )
        if not all(all(certificate[field]) for field in vector_gates):
            raise ContractError("public_status=exact has an incomplete positional gate")
        if certificate["fallback_counts"]["unsupported_degeneracies"] != 0:
            raise ContractError("public_status=exact has unsupported degeneracies")
        if certificate["fallback_counts"]["numeric_failures"] != 0:
            raise ContractError("public_status=exact has numeric failures")
        if certificate["exact_predicate_counts"]["remaining_unknown"] != 0:
            raise ContractError("public_status=exact has unresolved exact predicates")
        if certificate["unresolved_locus_ids"]:
            raise ContractError("public_status=exact has unresolved loci")
        if profile != "hgp_reduced":
            raise ContractError("the active v2 contract cannot publish full_pi0 exact")
        expected_basis = "gamma_exhaustive_reference"
        if proof_basis != expected_basis:
            raise ContractError(
                f"public_status=exact for {profile} requires proof_basis={expected_basis}"
            )
        if profile == "hgp_reduced" and certificate["effective_backend"] != "reference_cpu":
            raise ContractError(
                "exact hgp_reduced with gamma_exhaustive_reference is restricted to reference_cpu"
            )

    expected_contract = {
        "hgp_reduced": "hgp-reduced-v2",
        "full_pi0": "M1-reconstruction-v1",
    }[profile]
    if certificate["reconstruction_contract_id"] != expected_contract:
        raise ContractError(
            f"{profile} requires reconstruction_contract_id={expected_contract}"
        )
    allowed_bases = {
        "hgp_reduced": {
            "gamma_exhaustive_reference",
            "gabriel_positive_connectivity",
        },
        "full_pi0": {"m1_conditional_contract"},
    }[profile]
    if proof_basis not in allowed_bases:
        raise ContractError(f"proof_basis={proof_basis} is invalid for {profile}")

    budget_reasons = {
        "device_budget",
        "host_budget",
        "scratch_budget",
        "output_budget",
        "time_budget",
        "preempted",
        "user_cancelled",
    }
    if status == "budget_exhausted" and certificate["stop_reason"] not in budget_reasons:
        raise ContractError("budget_exhausted requires an explicit budget-like stop reason")
    if status == "unsupported_degeneracy" and certificate["stop_reason"] != status:
        raise ContractError("unsupported_degeneracy status and stop reason must agree")
    if status == "numeric_failure" and certificate["stop_reason"] != status:
        raise ContractError("numeric_failure status and stop reason must agree")

    _validate_budget_snapshot(
        certificate["budget_policy"], certificate["final_budget_snapshot"]
    )

    if check_canonical_ids:
        snapshot = certificate["final_budget_snapshot"]
        _canonical_id(
            snapshot["snapshot_id"],
            "BudgetSnapshot",
            {key: value for key, value in snapshot.items() if key != "snapshot_id"},
            "run_certificate.final_budget_snapshot.snapshot_id",
        )
        run_projection = {
            "input_sha256": certificate["input_semantics"]["input_sha256"],
            "k_max": certificate["k_max"],
            "profile": certificate["effective_profile"],
            "backend": certificate["effective_backend"],
            "mode": certificate["requested_mode"],
        }
        _canonical_id(
            certificate["run_id"],
            "Run",
            run_projection,
            "run_certificate.run_id",
        )
        _canonical_id(
            certificate["certificate_id"],
            "RunCertificate",
            {
                key: value
                for key, value in certificate.items()
                if key != "certificate_id"
            },
            "run_certificate.certificate_id",
        )


def _validate_budget_snapshot(
    policy: dict[str, Any], snapshot: dict[str, Any]
) -> None:
    """Validate one closed transactional resource-accounting snapshot."""

    if snapshot["boundary"] != "final":
        raise ContractError("final_budget_snapshot must use boundary=final")
    if snapshot["byte_unit"] != policy["byte_unit"]:
        raise ContractError("budget snapshot byte unit disagrees with its policy")
    if snapshot["time_unit"] != policy["time_unit"]:
        raise ContractError("budget snapshot time unit disagrees with its policy")

    for resource in ("device", "host", "scratch", "output"):
        policy_limit = policy[f"{resource}_budget_bytes"]
        limit = snapshot[f"{resource}_limit_bytes"]
        used = snapshot[f"{resource}_used_bytes"]
        reserved = snapshot[f"{resource}_reserved_bytes"]
        remaining = snapshot[f"{resource}_remaining_bytes"]
        if limit != policy_limit:
            raise ContractError(
                f"budget snapshot {resource} limit disagrees with its policy"
            )
        if limit is None:
            if remaining is not None:
                raise ContractError(
                    f"an unlimited {resource} budget must have null remaining bytes"
                )
            continue
        expected_remaining = limit - used - reserved
        if expected_remaining < 0 or remaining != expected_remaining:
            raise ContractError(
                f"budget snapshot {resource} accounting is not closed"
            )

    time_limit = snapshot["time_limit_s"]
    if time_limit != policy["time_budget_s"]:
        raise ContractError("budget snapshot time limit disagrees with its policy")
    if time_limit is None:
        if snapshot["remaining_s"] is not None:
            raise ContractError("an unlimited time budget must have null remaining time")
    else:
        expected_remaining = max(0.0, time_limit - snapshot["elapsed_s"])
        remaining = snapshot["remaining_s"]
        if remaining is None or not math.isclose(
            remaining, expected_remaining, rel_tol=0.0, abs_tol=1e-12
        ):
            raise ContractError("budget snapshot time accounting is not closed")


def validate_result_semantics(
    result: dict[str, Any], *, check_canonical_ids: bool = True
) -> None:
    certificate = result["run_certificate"]
    validate_canonical_rationals(result)
    validate_certificate_semantics(
        certificate, check_canonical_ids=check_canonical_ids
    )

    equal_fields = {
        "run_id": "run_id",
        "result_kind": "result_kind",
        "backend": "effective_backend",
        "profile": "effective_profile",
        "mode": "requested_mode",
        "k_eff": "k_eff",
    }
    for result_field, certificate_field in equal_fields.items():
        if result[result_field] != certificate[certificate_field]:
            raise ContractError(
                f"result.{result_field} must match run_certificate.{certificate_field}"
            )
    if result.get("forest_semantics") != certificate.get("forest_semantics"):
        raise ContractError("result and certificate forest_semantics must agree")

    n = certificate["input_point_count"]
    k_eff = certificate["k_eff"]
    cofaces = result["gamma_cofaces"]
    coface_by_id: dict[str, dict[str, Any]] = {}
    coface_labels: set[tuple[int, tuple[int, ...]]] = set()
    for coface in cofaces:
        coface_id = coface["coface_id"]
        if coface_id in coface_by_id:
            raise ContractError("gamma_cofaces must have unique coface_id values")

        order = coface["order"]
        if not isinstance(order, int) or isinstance(order, bool) or not 1 <= order <= k_eff:
            raise ContractError("a Gamma coface order is outside 1..k_eff")
        simplex = tuple(coface["simplex_point_ids"])
        if len(simplex) != order + 1:
            raise ContractError("a Gamma coface simplex has the wrong cardinality")
        if any(
            not isinstance(point_id, int)
            or isinstance(point_id, bool)
            or point_id < 0
            or point_id >= n
            for point_id in simplex
        ):
            raise ContractError("a Gamma coface simplex has an out-of-range point ID")
        if list(simplex) != sorted(set(simplex)):
            raise ContractError("a Gamma coface simplex is not a canonical point label")

        identity = (order, simplex)
        if identity in coface_labels:
            raise ContractError("Gamma cofaces must have unique (order, simplex) labels")
        coface_labels.add(identity)

        expected_facets = sorted(
            [list(facet) for facet in combinations(simplex, order)],
            key=canonical_json,
        )
        if coface["facet_point_ids"] != expected_facets:
            raise ContractError("a Gamma coface does not list all canonical facets")

        identity_projection = {
            "schema_version": coface["schema_version"],
            "order": order,
            "simplex_point_ids": list(simplex),
            "facet_point_ids": coface["facet_point_ids"],
            "squared_level_exact": coface["squared_level_exact"],
        }
        expected_coface_id = canonical_contract_id(
            "GammaCoface", identity_projection
        )
        if coface_id != expected_coface_id:
            raise ContractError("a Gamma coface_id is not its canonical v2 identifier")
        coface_by_id[coface_id] = coface

    if certificate["work_and_memory_counters"]["gamma_cofaces_emitted"] != len(
        cofaces
    ):
        raise ContractError("gamma_cofaces_emitted must equal the serialized count")

    batch_keys: set[tuple[int, Fraction]] = set()
    assigned_gamma_ids: list[str] = []
    for batch in result["equal_level_batches"]:
        batch_order = batch["order"]
        if (
            not isinstance(batch_order, int)
            or isinstance(batch_order, bool)
            or not 1 <= batch_order <= k_eff
        ):
            raise ContractError("an equal-level batch order is outside 1..k_eff")
        batch_level = batch["squared_level_exact"]
        batch_key = (batch_order, _exact_level_key(batch_level))
        if batch_key in batch_keys:
            raise ContractError("equal-level batches must have unique (order, level) keys")
        batch_keys.add(batch_key)
        for coface_id in batch["gamma_coface_ids"]:
            coface = coface_by_id.get(coface_id)
            if coface is None:
                raise ContractError("a batch references an unknown Gamma coface")
            if coface["order"] != batch_order:
                raise ContractError("a Gamma coface is assigned to the wrong order")
            if _exact_level_key(coface["squared_level_exact"]) != batch_key[1]:
                raise ContractError("a Gamma coface is assigned to the wrong exact level")
            assigned_gamma_ids.append(coface_id)

    counts = Counter(assigned_gamma_ids)
    if any(count > 1 for count in counts.values()):
        raise ContractError("a Gamma coface may occur in at most one batch")
    for order in range(1, certificate["k_eff"] + 1):
        if certificate["gamma_complete_by_order"][order - 1]:
            observed_labels = {
                simplex
                for coface_order, simplex in coface_labels
                if coface_order == order
            }
            expected_count = math.comb(n, order + 1) if order < n else 0
            if len(observed_labels) != expected_count:
                raise ContractError(
                    f"complete Gamma order {order} is missing or inventing cofaces"
                )
            if any(
                counts[coface["coface_id"]] != 1
                for coface in cofaces
                if coface["order"] == order
            ):
                raise ContractError(
                    "a complete Gamma coface must occur in exactly one batch"
                )

    result_kind = result["result_kind"]
    if result_kind == "hierarchy":
        if "forest_semantics" not in result:
            raise ContractError("a hierarchy result must declare forest_semantics")
        if len(result["forests"]) != result["k_eff"]:
            raise ContractError("a hierarchy result must contain one forest per order")
        if len(result["vertical_maps"]) != max(0, result["k_eff"] - 1):
            raise ContractError("a hierarchy result must contain one adjacent vertical map")
    elif result_kind == "partial_hierarchy":
        if result.get("forest_semantics") != "partial_refinement":
            raise ContractError("partial_hierarchy requires partial_refinement semantics")
        if "partial_scope" not in result:
            raise ContractError("partial_hierarchy requires partial_scope")
    elif result_kind == "verified_events_only":
        if "forest_semantics" in result:
            raise ContractError("verified_events_only must omit forest_semantics")
        if "partial_scope" not in result:
            raise ContractError("verified_events_only requires partial_scope")

    forest_orders = [forest["order"] for forest in result["forests"]]
    if result_kind == "hierarchy":
        expected_orders = list(range(1, result["k_eff"] + 1))
        if forest_orders != expected_orders:
            raise ContractError("forests must be canonically ordered from 1 through k_eff")
    elif forest_orders != sorted(set(forest_orders)) or any(
        order < 1 or order > result["k_eff"] for order in forest_orders
    ):
        raise ContractError("partial forests must have sorted unique valid orders")
    for forest in result["forests"]:
        if forest["profile"] != result["profile"]:
            raise ContractError("every forest profile must match the result profile")
        if forest["forest_semantics"] != result.get("forest_semantics"):
            raise ContractError("every forest semantic must match the result semantic")

    source_orders = [vertical_map["source_order"] for vertical_map in result["vertical_maps"]]
    if result_kind == "hierarchy":
        expected_sources = list(range(2, result["k_eff"] + 1))
        if source_orders != expected_sources:
            raise ContractError("vertical maps must be ordered by source order 2 through k_eff")
    elif source_orders != sorted(set(source_orders)):
        raise ContractError("partial vertical maps must have sorted unique source orders")
    for vertical_map in result["vertical_maps"]:
        if vertical_map["target_order"] != vertical_map["source_order"] - 1:
            raise ContractError("vertical maps must target the adjacent lower order")

    if certificate["public_status"] == "exact":
        if "partial_scope" in result:
            raise ContractError("an exact hierarchy must not carry partial_scope")
        if any(not forest["complete"] for forest in result["forests"]):
            raise ContractError("an exact hierarchy contains an incomplete forest")
        for batch in result["equal_level_batches"]:
            if (
                not batch["events_complete"]
                or not batch["attachments_complete"]
                or batch["batch_status"] != "closed"
            ):
                raise ContractError("an exact hierarchy contains an open batch")
        for vertical_map in result["vertical_maps"]:
            if not vertical_map["complete"]:
                raise ContractError("an exact hierarchy contains an incomplete vertical map")
            if vertical_map["naturality_failures"] != 0:
                raise ContractError("an exact hierarchy has a vertical naturality failure")

    _validate_result_identifiers_and_references(
        result, check_canonical_ids=check_canonical_ids
    )


def _validate_result_identifiers_and_references(
    result: dict[str, Any], *, check_canonical_ids: bool
) -> None:
    """Validate the content-addressed result graph and every typed reference."""

    certificate = result["run_certificate"]
    n = certificate["input_point_count"]
    input_semantics = certificate["input_semantics"]

    points = result.get("embedded_input_points")
    if points is not None:
        if len(points) != n:
            raise ContractError(
                "embedded_input_points length must equal input_point_count"
            )
        point_ids = [point["point_id"] for point in points]
        if point_ids != list(range(n)):
            raise ContractError("embedded point IDs must be contiguous from zero")
        coordinate_keys: set[tuple[str, str, str]] = set()
        source_indices: set[int] = set()
        point_order_keys: list[tuple[int, int, int, int]] = []
        for point in points:
            if point["multiplicity"] != len(point["source_indices"]):
                raise ContractError(
                    "an embedded point multiplicity disagrees with source_indices"
                )
            coordinate_key = tuple(point["coordinate_bits"])
            if coordinate_key in coordinate_keys:
                raise ContractError("embedded canonical points repeat coordinates")
            coordinate_keys.add(coordinate_key)
            if point["source_indices"] != sorted(set(point["source_indices"])):
                raise ContractError(
                    "embedded point source_indices must be canonical and unique"
                )
            point_order_keys.append(
                (
                    *(
                        _binary64_total_order_key(bits)
                        for bits in point["coordinate_bits"]
                    ),
                    point["source_indices"][0],
                )
            )
            overlap = source_indices.intersection(point["source_indices"])
            if overlap:
                raise ContractError("embedded points repeat source indices")
            source_indices.update(point["source_indices"])
            if (
                input_semantics["duplicate_policy"] == "reject"
                and point["multiplicity"] != 1
            ):
                raise ContractError(
                    "duplicate_policy=reject requires unit point multiplicities"
                )
        if point_order_keys != sorted(point_order_keys):
            raise ContractError(
                "embedded points do not follow the canonical binary64 total order"
            )
        if source_indices != set(range(sum(point["multiplicity"] for point in points))):
            raise ContractError(
                "embedded source indices must be contiguous from zero"
            )
        input_projection = [
            {
                "point_id": point["point_id"],
                "source_indices": point["source_indices"],
                "multiplicity": point["multiplicity"],
                "coordinate_bits": point["coordinate_bits"],
            }
            for point in points
        ]
        _canonical_id(
            certificate["input_semantics"]["input_sha256"],
            "Input",
            input_projection,
            "run_certificate.input_semantics.input_sha256",
        )

    similarity = input_semantics["similarity_transform"]
    if not similarity["applied"]:
        if similarity["scale"] != {
            "schema_version": similarity["scale"]["schema_version"],
            "numerator": "1",
            "denominator": "1",
        }:
            raise ContractError("an unapplied similarity must have unit scale")
        translation = similarity["translation"]
        if any(
            translation[field] != "0"
            for field in ("x_numerator", "y_numerator", "z_numerator")
        ) or translation["denominator"] != "1":
            raise ContractError("an unapplied similarity must have zero translation")
    if (
        certificate["public_status"] == "exact"
        and input_semantics["perturbation_policy"] != "none"
    ):
        raise ContractError("an exact result cannot use symbolic perturbation")

    events = _index_records(result["critical_catalog"], "event_id", "critical_catalog")
    cofaces = _index_records(result["gamma_cofaces"], "coface_id", "gamma_cofaces")
    hyperedges = _index_records(
        result["gabriel_hyperedges"], "hyperedge_id", "gabriel_hyperedges"
    )
    attachments = _index_records(result["attachments"], "attachment_id", "attachments")
    batches = _index_records(
        result["equal_level_batches"], "batch_id", "equal_level_batches"
    )
    forests = _index_records(result["forests"], "forest_id", "forests")
    vertical_maps = _index_records(result["vertical_maps"], "map_id", "vertical_maps")

    exact_result = certificate["public_status"] == "exact"
    for event in events.values():
        _validate_critical_event(event, n, certificate["k_eff"], exact_result)
        if certificate["catalog_complete_by_rank"][event["closed_rank"] - 1] and (
            event["predicate_status"] != "certified_exact"
            or event["degeneracy_class"] == "unsupported"
        ):
            raise ContractError(
                "a complete catalog rank contains an uncertified critical event"
            )
    if certificate["catalog_complete_by_rank"][0]:
        rank_one_shells = sorted(
            tuple(event["shell_ids"])
            for event in events.values()
            if event["closed_rank"] == 1
        )
        if rank_one_shells != [(point_id,) for point_id in range(n)]:
            raise ContractError(
                "a complete critical catalog must contain one rank-one birth per point"
            )

    for hyperedge in hyperedges.values():
        _validate_gabriel_hyperedge(
            hyperedge,
            events[hyperedge["event_id"]]
            if hyperedge["event_id"] in events
            else None,
            n,
            certificate["k_eff"],
        )
    hyperedges_by_event = Counter(
        hyperedge["event_id"] for hyperedge in hyperedges.values()
    )
    if any(
        hyperedges_by_event[event_id] != 1
        for event_id, event in events.items()
        if event["saddle_order"] is not None
        and certificate["catalog_complete_by_rank"][event["closed_rank"] - 1]
    ):
        raise ContractError(
            "a complete saddle-event catalog must emit one Gabriel hyperedge per event"
        )

    counters = certificate["work_and_memory_counters"]
    if counters["accepted_events"] != len(events):
        raise ContractError("accepted_events must equal the critical_catalog size")
    if counters["candidate_events"] != (
        counters["accepted_events"] + counters["rejected_events"]
    ):
        raise ContractError("candidate event accounting is not closed")
    if counters["gamma_cofaces_emitted"] != len(cofaces):
        raise ContractError("gamma_cofaces_emitted must equal the serialized count")
    if counters["hyperedges_emitted"] != len(hyperedges):
        raise ContractError("hyperedges_emitted must equal the serialized count")

    if check_canonical_ids:
        for event in events.values():
            _canonical_id(
                event["event_id"],
                "CriticalEvent",
                {
                    "interior_ids": event["interior_ids"],
                    "shell_ids": event["shell_ids"],
                    "minimal_support_ids": event["minimal_support_ids"],
                    "center_witness_homogeneous": event[
                        "center_witness_homogeneous"
                    ],
                    "squared_level_exact": event["squared_level_exact"],
                },
                "critical_catalog.event_id",
            )
        for hyperedge in hyperedges.values():
            _canonical_id(
                hyperedge["hyperedge_id"],
                "GabrielHyperedge",
                {
                    "event_id": hyperedge["event_id"],
                    "order": hyperedge["order"],
                    "simplex_point_ids": hyperedge["simplex_point_ids"],
                    "facet_point_ids": hyperedge["facet_point_ids"],
                    "strict_arm_point_ids": hyperedge["strict_arm_point_ids"],
                    "squared_level_exact": hyperedge["squared_level_exact"],
                },
                "gabriel_hyperedges.hyperedge_id",
            )
        for attachment in attachments.values():
            _canonical_id(
                attachment["attachment_id"],
                "Attachment",
                {
                    "event_id": attachment["event_id"],
                    "order": attachment["order"],
                    "removed_shell_id": attachment["removed_shell_id"],
                },
                "attachments.attachment_id",
            )
        for batch in batches.values():
            _canonical_id(
                batch["batch_id"],
                "EqualLevelBatch",
                {
                    "order": batch["order"],
                    "squared_level_exact": batch["squared_level_exact"],
                    "event_ids": batch["event_ids"],
                    "gamma_coface_ids": batch["gamma_coface_ids"],
                    "hyperedge_ids": batch["hyperedge_ids"],
                    "attachment_ids": batch["attachment_ids"],
                },
                "equal_level_batches.batch_id",
            )

    forest_by_order: dict[int, dict[str, Any]] = {}
    nodes: dict[str, dict[str, Any]] = {}
    node_order: dict[str, int] = {}
    nodes_by_order: dict[int, dict[str, dict[str, Any]]] = {}
    for forest in result["forests"]:
        order = forest["order"]
        if order in forest_by_order:
            raise ContractError("forests must have unique order values")
        forest_by_order[order] = forest
        local_nodes = _index_records(
            forest["nodes"], "node_id", f"forest order {order} nodes"
        )
        nodes_by_order[order] = local_nodes
        for node_id, node in local_nodes.items():
            if node_id in nodes:
                raise ContractError("merge node IDs must be globally unique")
            nodes[node_id] = node
            node_order[node_id] = order

    for hyperedge in hyperedges.values():
        _require_references(
            [hyperedge["event_id"]], events, "a Gabriel hyperedge"
        )
    for attachment in attachments.values():
        _require_references([attachment["event_id"]], events, "an attachment")
        _require_references(
            [attachment["target_node_id"]], nodes, "an attachment target"
        )
        if node_order[attachment["target_node_id"]] != attachment["order"]:
            raise ContractError("an attachment target belongs to the wrong order")
        event = events[attachment["event_id"]]
        target = nodes[attachment["target_node_id"]]
        owner_batches = [
            batch
            for batch in batches.values()
            if attachment["attachment_id"] in batch["attachment_ids"]
        ]
        if len(owner_batches) != 1:
            raise ContractError(
                "an attachment must occur in exactly one equal-level batch"
            )
        target_components = [
            component
            for component in owner_batches[0]["pre_lot_components"]
            if component["component_id"] == attachment["target_node_id"]
        ]
        if len(target_components) != 1:
            raise ContractError(
                "an attachment target is absent from its strict pre-lot state"
            )
        target_covered_at_attachment = set(
            target_components[0]["covered_point_ids"]
        )
        expected_arm = sorted(
            (
                set(event["interior_ids"])
                | set(event["shell_ids"])
            )
            - {attachment["removed_shell_id"]}
        )
        if (
            event["saddle_order"] != attachment["order"]
            or attachment["removed_shell_id"] not in event["shell_ids"]
            or attachment["arm_point_ids"] != expected_arm
            or _exact_level_key(attachment["event_squared_level"])
            != _exact_level_key(event["squared_level_exact"])
        ):
            raise ContractError("an attachment disagrees with its critical-event arm")
        if not set(attachment["arm_point_ids"]) <= target_covered_at_attachment:
            raise ContractError("an attachment target does not cover its arm")
        if _exact_level_key(target["squared_level"]) >= _exact_level_key(
            attachment["event_squared_level"]
        ):
            raise ContractError("an attachment target is not at a strict lower level")
        previous_label = attachment["arm_point_ids"]
        previous_level = attachment["event_squared_level"]
        for expected_index, step in enumerate(attachment["descent_path"]):
            if step["step_index"] != expected_index:
                raise ContractError("attachment descent step indices must be contiguous")
            if step["from_point_ids"] != previous_label or _exact_level_key(
                step["from_squared_level"]
            ) != _exact_level_key(previous_level):
                raise ContractError("an attachment descent path is not causally chained")
            if len(step["from_point_ids"]) != attachment["order"] or len(
                step["to_point_ids"]
            ) != attachment["order"]:
                raise ContractError("an attachment descent step has the wrong order")
            expected_shared = sorted(
                set(step["from_point_ids"]).intersection(step["to_point_ids"])
            )
            if (
                step["shared_facet_point_ids"] != expected_shared
                or len(expected_shared) != max(1, attachment["order"] - 1)
            ):
                raise ContractError("an attachment descent step has the wrong shared facet")
            if _exact_level_key(step["to_squared_level"]) >= _exact_level_key(
                step["from_squared_level"]
            ):
                raise ContractError("an attachment descent step is not strictly decreasing")
            previous_label = step["to_point_ids"]
            previous_level = step["to_squared_level"]
        if not set(previous_label) <= target_covered_at_attachment:
            raise ContractError("an attachment descent does not terminate in its target")
        if _exact_level_key(target["squared_level"]) > _exact_level_key(
            previous_level
        ):
            raise ContractError("an attachment descent terminates below its target birth")

    assigned_event_ids: list[str] = []
    assigned_hyperedge_ids: list[str] = []
    assigned_attachment_ids: list[str] = []
    for batch in batches.values():
        _require_references(batch["event_ids"], events, "a batch event list")
        _require_references(
            batch["gamma_coface_ids"], cofaces, "a batch Gamma list"
        )
        _require_references(
            batch["hyperedge_ids"], hyperedges, "a batch hyperedge list"
        )
        _require_references(
            batch["attachment_ids"], attachments, "a batch attachment list"
        )
        assigned_event_ids.extend(batch["event_ids"])
        assigned_hyperedge_ids.extend(batch["hyperedge_ids"])
        assigned_attachment_ids.extend(batch["attachment_ids"])
        order = batch["order"]
        level = _exact_level_key(batch["squared_level_exact"])
        for event_id in batch["event_ids"]:
            event = events[event_id]
            if _exact_level_key(event["squared_level_exact"]) != level or order not in {
                event["birth_order"],
                event["saddle_order"],
            }:
                raise ContractError("a batch contains an event at the wrong order or level")
        for hyperedge_id in batch["hyperedge_ids"]:
            hyperedge = hyperedges[hyperedge_id]
            if hyperedge["order"] != order or _exact_level_key(
                hyperedge["squared_level_exact"]
            ) != level:
                raise ContractError("a batch contains a hyperedge at the wrong order or level")
        for attachment_id in batch["attachment_ids"]:
            attachment = attachments[attachment_id]
            if attachment["order"] != order or _exact_level_key(
                attachment["event_squared_level"]
            ) != level:
                raise ContractError("a batch contains an attachment at the wrong order or level")
        for field in ("pre_lot_components", "post_lot_components"):
            component_ids: set[str] = set()
            active_facets: set[tuple[int, ...]] = set()
            for component in batch[field]:
                component_id = component["component_id"]
                if component_id in component_ids:
                    raise ContractError("a batch repeats a component identity")
                component_ids.add(component_id)
                _require_references([component_id], nodes, f"batch.{field}")
                if node_order[component_id] != order:
                    raise ContractError("a batch component belongs to the wrong order")
                component_facets = [
                    tuple(facet) for facet in component["active_facet_point_ids"]
                ]
                if any(
                    len(facet) != order
                    or tuple(sorted(set(facet))) != facet
                    or any(point_id < 0 or point_id >= n for point_id in facet)
                    for facet in component_facets
                ):
                    raise ContractError(
                        "a batch component contains a noncanonical active facet"
                    )
                if active_facets.intersection(component_facets):
                    raise ContractError(
                        "a batch snapshot assigns one facet to multiple components"
                    )
                active_facets.update(component_facets)
                expected_covered = sorted(
                    {point_id for facet in component_facets for point_id in facet}
                )
                if component["covered_point_ids"] != expected_covered:
                    raise ContractError(
                        "a batch component coverage disagrees with its active facets"
                    )
        for delta in batch["coverage_deltas"]:
            if delta["batch_id"] != batch["batch_id"] or delta["order"] != order:
                raise ContractError("a batch coverage delta has inconsistent ownership")
            _require_references(
                [delta["root_node_id"]], nodes, "a batch coverage delta"
            )
            if node_order[delta["root_node_id"]] != order:
                raise ContractError("a coverage delta root belongs to the wrong order")

    if exact_result:
        for assigned_ids, records, label in (
            (assigned_event_ids, events, "critical event"),
            (assigned_hyperedge_ids, hyperedges, "Gabriel hyperedge"),
            (assigned_attachment_ids, attachments, "attachment"),
        ):
            assignment_counts = Counter(assigned_ids)
            if set(assignment_counts) != set(records) or any(
                count != 1 for count in assignment_counts.values()
            ):
                raise ContractError(
                    f"every exact {label} must occur in exactly one batch"
                )

    for order in range(1, certificate["k_eff"] + 1):
        order_batches = [
            batch for batch in batches.values() if batch["order"] == order
        ]
        if certificate["batches_complete_by_order"][order - 1]:
            if any(
                batch["batch_status"] != "closed"
                or not batch["events_complete"]
                for batch in order_batches
            ):
                raise ContractError(
                    "a complete batch order contains an open or event-incomplete batch"
                )
            hyperedge_counts = Counter(
                hyperedge_id
                for batch in order_batches
                for hyperedge_id in batch["hyperedge_ids"]
            )
            if any(
                hyperedge_counts[hyperedge_id] != 1
                for hyperedge_id, hyperedge in hyperedges.items()
                if hyperedge["order"] == order
            ):
                raise ContractError(
                    "a complete batch order must assign every Gabriel hyperedge once"
                )
            if result["profile"] == "full_pi0":
                event_counts = Counter(
                    event_id
                    for batch in order_batches
                    for event_id in batch["event_ids"]
                )
                if any(
                    event_counts[event_id] != 1
                    for event_id, event in events.items()
                    if order in {event["birth_order"], event["saddle_order"]}
                ):
                    raise ContractError(
                        "a complete full_pi0 order must assign every active event once"
                    )
        if certificate["attachments_complete_by_order"][order - 1]:
            if any(not batch["attachments_complete"] for batch in order_batches):
                raise ContractError(
                    "a complete attachment order contains an incomplete batch"
                )
            attachment_counts = Counter(
                attachment_id
                for batch in order_batches
                for attachment_id in batch["attachment_ids"]
            )
            if any(
                attachment_counts[attachment_id] != 1
                for attachment_id, attachment in attachments.items()
                if attachment["order"] == order
            ):
                raise ContractError(
                    "a complete attachment order must assign every attachment once"
                )
            if result["profile"] == "full_pi0":
                expected_attachment_keys = {
                    (event_id, order, shell_id)
                    for event_id, event in events.items()
                    if event["saddle_order"] == order
                    for shell_id in event["shell_ids"]
                }
                actual_attachment_keys = {
                    (
                        attachment["event_id"],
                        attachment["order"],
                        attachment["removed_shell_id"],
                    )
                    for attachment in attachments.values()
                    if attachment["order"] == order
                }
                if actual_attachment_keys != expected_attachment_keys:
                    raise ContractError(
                        "a complete full_pi0 order must attach every saddle arm"
                    )

    for order, forest in forest_by_order.items():
        local_nodes = nodes_by_order[order]
        child_ids: set[str] = set()
        child_parent_counts: Counter[str] = Counter()
        for node in local_nodes.values():
            _require_references(node["child_ids"], local_nodes, "a merge node child list")
            _require_references(node["event_ids"], events, "a merge node event list")
            if node["kind"] == "birth" and node["child_ids"]:
                raise ContractError("a birth node cannot have children")
            if node["kind"] == "merge" and len(node["child_ids"]) < 2:
                raise ContractError("a merge node must have at least two children")
            if node["kind"] == "merge" and node["batch_id"] is None:
                raise ContractError("a merge node must reference its equal-level batch")
            for child_id in node["child_ids"]:
                if _exact_level_key(local_nodes[child_id]["squared_level"]) >= (
                    _exact_level_key(node["squared_level"])
                ):
                    raise ContractError("a merge child must be at a strictly lower level")
                child_ids.add(child_id)
                child_parent_counts[child_id] += 1
            if node["kind"] == "merge":
                expected_covered = sorted(
                    {
                        point_id
                        for child_id in node["child_ids"]
                        for point_id in local_nodes[child_id]["covered_point_ids"]
                    }
                )
            else:
                expected_covered = sorted(
                    {
                        point_id
                        for event_id in node["event_ids"]
                        for point_id in (
                            events[event_id]["interior_ids"]
                            + events[event_id]["shell_ids"]
                        )
                    }
                )
            if node["covered_point_ids"] != expected_covered:
                raise ContractError(
                    "a merge node coverage disagrees with its children or birth events"
                )
            node_level = _exact_level_key(node["squared_level"])
            for event_id in node["event_ids"]:
                event = events[event_id]
                if order not in {event["birth_order"], event["saddle_order"]} or (
                    _exact_level_key(event["squared_level_exact"]) != node_level
                ):
                    raise ContractError(
                        "a merge node cites an event at the wrong order or level"
                    )
                if not any(
                    candidate["order"] == order
                    and _exact_level_key(candidate["squared_level_exact"])
                    == node_level
                    and event_id in candidate["event_ids"]
                    for candidate in batches.values()
                ):
                    raise ContractError(
                        "a merge node event is absent from its equal-level batch"
                    )
            batch_id = node["batch_id"]
            if batch_id is not None:
                _require_references([batch_id], batches, "a merge node batch")
                batch = batches[batch_id]
                if batch["order"] != order or _exact_level_key(
                    batch["squared_level_exact"]
                ) != _exact_level_key(node["squared_level"]):
                    raise ContractError("a merge node batch has the wrong order or level")
            if check_canonical_ids:
                _canonical_id(
                    node["node_id"],
                    "MergeNode",
                    {
                        "order": order,
                        "profile": result["profile"],
                        "kind": node["kind"],
                        "squared_level": node["squared_level"],
                        "child_ids": node["child_ids"],
                        "event_ids": node["event_ids"],
                        "batch_id": node["batch_id"],
                    },
                    "forests.nodes.node_id",
                )
        if any(count != 1 for count in child_parent_counts.values()):
            raise ContractError("a forest node must have at most one parent")
        _require_references(forest["root_ids"], local_nodes, "forest.root_ids")
        expected_roots = set(local_nodes) - child_ids
        if set(forest["root_ids"]) != expected_roots:
            raise ContractError("forest.root_ids do not equal the DAG roots")
        for delta in forest["coverage_log"]:
            _validate_coverage_reference(delta, batches, nodes, node_order, order)
        if check_canonical_ids:
            _canonical_id(
                forest["forest_id"],
                "MergeForest",
                {
                    "order": order,
                    "profile": result["profile"],
                    "forest_semantics": result["forest_semantics"],
                    "node_ids": [node["node_id"] for node in forest["nodes"]],
                    "root_ids": forest["root_ids"],
                    "coverage_log": forest["coverage_log"],
                },
                "forests.forest_id",
            )

    for delta in result["coverage_log"]:
        _validate_coverage_reference(delta, batches, nodes, node_order, delta["order"])

    result_coverage = Counter(canonical_json(delta) for delta in result["coverage_log"])
    forest_coverage = Counter(
        canonical_json(delta)
        for forest in result["forests"]
        for delta in forest["coverage_log"]
    )
    batch_coverage = Counter(
        canonical_json(delta)
        for batch in result["equal_level_batches"]
        for delta in batch["coverage_deltas"]
    )
    if result_coverage != forest_coverage or result_coverage != batch_coverage:
        raise ContractError(
            "result, forest, and batch coverage logs must encode the same deltas"
        )

    materialized_keys: set[tuple[int, str]] = set()
    for point_set in result["materialized_point_sets"]:
        _require_references(
            [point_set["node_id"]], nodes, "a materialized point set"
        )
        if node_order[point_set["node_id"]] != point_set["order"]:
            raise ContractError("a materialized point set belongs to the wrong order")
        materialized_key = (point_set["order"], point_set["node_id"])
        if materialized_key in materialized_keys:
            raise ContractError("a merge node has multiple materialized point sets")
        materialized_keys.add(materialized_key)
        if point_set["point_ids"] != nodes[point_set["node_id"]][
            "covered_point_ids"
        ]:
            raise ContractError(
                "a materialized point set disagrees with its merge node coverage"
            )

    for vertical_map in vertical_maps.values():
        source_order = vertical_map["source_order"]
        target_order = vertical_map["target_order"]
        source_nodes = nodes_by_order.get(source_order, {})
        target_nodes = nodes_by_order.get(target_order, {})
        assigned_source_ids: set[str] = set()
        for assignment in vertical_map["assignments"]:
            _require_references(
                [assignment["source_node_id"]],
                source_nodes,
                "a vertical assignment source",
            )
            _require_references(
                [assignment["target_node_id"]],
                target_nodes,
                "a vertical assignment target",
            )
            _require_references(
                assignment["proof_reference_ids"],
                events,
                "a vertical assignment proof list",
            )
            if assignment["source_node_id"] in assigned_source_ids:
                raise ContractError(
                    "a vertical map repeats an assignment source node"
                )
            assigned_source_ids.add(assignment["source_node_id"])
            source_node = source_nodes[assignment["source_node_id"]]
            target_node = target_nodes[assignment["target_node_id"]]
            if _exact_level_key(assignment["at_squared_level"]) != _exact_level_key(
                source_node["squared_level"]
            ):
                raise ContractError(
                    "a vertical assignment must be recorded at its source-node level"
                )
            if _exact_level_key(target_node["squared_level"]) > _exact_level_key(
                assignment["at_squared_level"]
            ):
                raise ContractError(
                    "a vertical assignment targets a node born above its level"
                )
            assignment_level = _exact_level_key(
                assignment["at_squared_level"]
            )
            source_component = _closed_component_at_level(
                batches,
                source_order,
                assignment["source_node_id"],
                assignment_level,
            )
            target_component = _closed_component_at_level(
                batches,
                target_order,
                assignment["target_node_id"],
                assignment_level,
            )
            if not set(source_component["covered_point_ids"]) <= set(
                target_component["covered_point_ids"]
            ):
                raise ContractError(
                    "a vertical assignment target does not cover its source"
                )
            if set(assignment["proof_reference_ids"]) != set(
                source_node["event_ids"]
            ):
                raise ContractError(
                    "a vertical assignment proof does not identify its source events"
                )
        if vertical_map["complete"] and assigned_source_ids != set(source_nodes):
            raise ContractError(
                "a complete vertical map must assign every source forest node"
            )
        if vertical_map["naturality_failures"] > vertical_map[
            "naturality_squares_checked"
        ]:
            raise ContractError("vertical naturality failures exceed checked squares")
        if check_canonical_ids:
            _canonical_id(
                vertical_map["map_id"],
                "VerticalMap",
                {
                    "source_order": source_order,
                    "target_order": target_order,
                    "assignments": vertical_map["assignments"],
                },
                "vertical_maps.map_id",
            )

    if certificate["vertical_maps_complete"]:
        expected_sources = set(range(2, certificate["k_eff"] + 1))
        if set(vertical_map["source_order"] for vertical_map in vertical_maps.values()) != (
            expected_sources
        ) or any(not vertical_map["complete"] for vertical_map in vertical_maps.values()):
            raise ContractError(
                "vertical_maps_complete requires every adjacent complete vertical map"
            )

    partial_scope = result.get("partial_scope")
    if partial_scope is not None:
        _require_references(
            partial_scope["verified_event_ids"],
            events,
            "partial_scope.verified_event_ids",
        )
        locus_ids = [locus["locus_id"] for locus in partial_scope["unresolved_loci"]]
        if len(locus_ids) != len(set(locus_ids)):
            raise ContractError("partial_scope unresolved locus IDs must be unique")
        if set(certificate["unresolved_locus_ids"]) != set(locus_ids):
            raise ContractError(
                "run_certificate unresolved_locus_ids must match partial_scope"
            )
        _validate_partial_scope(
            partial_scope,
            certificate,
            events,
            n,
            check_canonical_ids=check_canonical_ids,
        )
    elif certificate["unresolved_locus_ids"]:
        raise ContractError("unresolved_locus_ids require a partial_scope")

    closed_gamma_replay = exact_result or (
        result["profile"] == "full_pi0"
        and all(certificate["catalog_complete_by_rank"])
        and all(certificate["gamma_complete_by_order"])
        and all(certificate["batches_complete_by_order"])
        and all(certificate["attachments_complete_by_order"])
    )
    if closed_gamma_replay:
        if any(not forest["complete"] for forest in result["forests"]):
            raise ContractError("a closed Gamma replay contains an incomplete forest")
        _validate_closed_gamma_replay(
            result,
            events,
            cofaces,
            batches,
            forest_by_order,
            nodes_by_order,
        )

    if check_canonical_ids:
        scientific_projection = {
            "input_sha256": certificate["input_semantics"]["input_sha256"],
            "profile": result["profile"],
            "forest_semantics": result.get("forest_semantics"),
            "event_ids": [event["event_id"] for event in result["critical_catalog"]],
            "hyperedge_ids": [
                hyperedge["hyperedge_id"] for hyperedge in result["gabriel_hyperedges"]
            ],
            "gamma_coface_ids": [
                coface["coface_id"] for coface in result["gamma_cofaces"]
            ],
            "attachment_ids": [
                attachment["attachment_id"] for attachment in result["attachments"]
            ],
            "batch_ids": [batch["batch_id"] for batch in result["equal_level_batches"]],
            "forest_ids": [forest["forest_id"] for forest in result["forests"]],
            "vertical_map_ids": [
                vertical_map["map_id"] for vertical_map in result["vertical_maps"]
            ],
        }
        _canonical_id(
            result["result_id"],
            "MorseHGP3DResult",
            scientific_projection,
            "result.result_id",
        )


def _validate_critical_event(
    event: dict[str, Any], n: int, k_eff: int, exact_result: bool
) -> None:
    interior = event["interior_ids"]
    shell = event["shell_ids"]
    support = event["minimal_support_ids"]
    for field, point_ids in (
        ("interior_ids", interior),
        ("shell_ids", shell),
        ("minimal_support_ids", support),
    ):
        if any(point_id < 0 or point_id >= n for point_id in point_ids):
            raise ContractError(f"a critical event has an out-of-range {field}")
    if set(interior).intersection(shell):
        raise ContractError("critical-event interior and shell must be disjoint")
    if not set(support) <= set(shell):
        raise ContractError("critical-event minimal support must lie on its shell")
    if event["closed_rank"] != len(interior) + len(shell):
        raise ContractError("critical-event closed_rank disagrees with interior and shell")
    if len(event["barycentric_signs"]) != len(support):
        raise ContractError("critical-event barycentric signs do not match its support")
    if event["predicate_status"] == "certified_exact" and any(
        sign != 1 for sign in event["barycentric_signs"]
    ):
        raise ContractError("a certified critical support is not relatively interior")

    rank = event["closed_rank"]
    expected_birth = rank if rank <= k_eff else None
    expected_saddle = rank - 1 if 2 <= rank <= k_eff + 1 else None
    if event["birth_order"] != expected_birth:
        raise ContractError("critical-event birth_order disagrees with closed_rank")
    if event["saddle_order"] != expected_saddle:
        raise ContractError("critical-event saddle_order disagrees with closed_rank")

    roles_by_order: dict[int, dict[str, Any]] = {}
    for role in event["morse_roles"]:
        if role["order"] in roles_by_order:
            raise ContractError("critical-event Morse roles repeat an order")
        roles_by_order[role["order"]] = role
    expected_orders = {
        order for order in (expected_birth, expected_saddle) if order is not None
    }
    if set(roles_by_order) != expected_orders:
        raise ContractError("critical-event Morse roles do not match its active orders")
    if expected_birth is not None:
        role = roles_by_order[expected_birth]
        if (
            role["morse_index"] != 0
            or role["local_multiplicity"] != 1
            or role["arm_count"] != 0
        ):
            raise ContractError("a critical birth has an incoherent Morse role")
    if expected_saddle is not None:
        role = roles_by_order[expected_saddle]
        if (
            role["morse_index"] != 1
            or role["arm_count"] != len(shell)
            or role["local_multiplicity"] != len(shell) - 1
        ):
            raise ContractError("a critical saddle has an incoherent Morse role")

    if exact_result:
        if event["predicate_status"] != "certified_exact":
            raise ContractError("an exact result contains an uncertified critical event")
        if event["degeneracy_class"] == "unsupported":
            raise ContractError("an exact result contains an unsupported critical event")


def _validate_gabriel_hyperedge(
    hyperedge: dict[str, Any],
    event: dict[str, Any] | None,
    n: int,
    k_eff: int,
) -> None:
    """Validate the complete geometric projection of one Gabriel hyperedge."""

    order = hyperedge["order"]
    simplex = hyperedge["simplex_point_ids"]
    if order < 1 or order > k_eff:
        raise ContractError("a Gabriel hyperedge order is outside 1..k_eff")
    if (
        len(simplex) != order + 1
        or simplex != sorted(set(simplex))
        or any(point_id < 0 or point_id >= n for point_id in simplex)
    ):
        raise ContractError(
            "a Gabriel hyperedge simplex is not a canonical order+1 point label"
        )
    expected_facets = sorted(
        [list(facet) for facet in combinations(simplex, order)], key=canonical_json
    )
    if hyperedge["facet_point_ids"] != expected_facets:
        raise ContractError("a Gabriel hyperedge does not list all canonical facets")
    if hyperedge["covered_point_ids"] != simplex:
        raise ContractError("a Gabriel hyperedge coverage must equal its simplex")
    if not 0 <= hyperedge["star_pivot_facet_index"] < len(expected_facets):
        raise ContractError("a Gabriel hyperedge pivot facet index is out of range")
    if event is None:
        raise ContractError("a Gabriel hyperedge references an unknown critical event")
    event_label = sorted(event["interior_ids"] + event["shell_ids"])
    if (
        event_label != simplex
        or event["saddle_order"] != order
        or _exact_level_key(event["squared_level_exact"])
        != _exact_level_key(hyperedge["squared_level_exact"])
    ):
        raise ContractError(
            "a Gabriel hyperedge disagrees with its geometric critical event"
        )
    expected_arms = sorted(
        [
            sorted(point_id for point_id in simplex if point_id != shell_id)
            for shell_id in event["shell_ids"]
        ],
        key=canonical_json,
    )
    if hyperedge["strict_arm_point_ids"] != expected_arms:
        raise ContractError("a Gabriel hyperedge strict arms disagree with its shell")


def _component_snapshot(
    components: list[dict[str, Any]],
) -> dict[str, tuple[tuple[tuple[int, ...], ...], tuple[int, ...]]]:
    return {
        component["component_id"]: (
            tuple(tuple(facet) for facet in component["active_facet_point_ids"]),
            tuple(component["covered_point_ids"]),
        )
        for component in components
    }


def _closed_component_at_level(
    batches: dict[str, dict[str, Any]],
    order: int,
    root_id: str,
    level: Fraction,
) -> dict[str, Any]:
    """Return one root's replayed closed component at an exact threshold."""

    eligible = [
        batch
        for batch in batches.values()
        if batch["order"] == order
        and _exact_level_key(batch["squared_level_exact"]) <= level
    ]
    if not eligible:
        raise ContractError(
            "a vertical assignment has no replay state at its exact level"
        )
    latest = max(
        eligible, key=lambda batch: _exact_level_key(batch["squared_level_exact"])
    )
    components = [
        component
        for component in latest["post_lot_components"]
        if component["component_id"] == root_id
    ]
    if len(components) != 1:
        raise ContractError(
            "a vertical assignment root is absent from its replayed exact-level state"
        )
    return components[0]


def _validate_closed_gamma_replay(
    result: dict[str, Any],
    events: dict[str, dict[str, Any]],
    cofaces: dict[str, dict[str, Any]],
    batches: dict[str, dict[str, Any]],
    forests: dict[int, dict[str, Any]],
    nodes_by_order: dict[int, dict[str, dict[str, Any]]],
) -> None:
    """Replay every declared-complete Gamma batch from its strict-sublevel state."""

    gamma_keys = {
        (
            coface["order"],
            tuple(coface["simplex_point_ids"]),
            _exact_level_key(coface["squared_level_exact"]),
        )
        for coface in cofaces.values()
    }
    for hyperedge in result["gabriel_hyperedges"]:
        key = (
            hyperedge["order"],
            tuple(hyperedge["simplex_point_ids"]),
            _exact_level_key(hyperedge["squared_level_exact"]),
        )
        if key not in gamma_keys:
            raise ContractError(
                "a closed Gabriel hyperedge has no matching exhaustive Gamma coface"
            )

    for order in range(1, result["k_eff"] + 1):
        forest = forests[order]
        local_nodes = nodes_by_order[order]
        ordered_batches = sorted(
            (batch for batch in batches.values() if batch["order"] == order),
            key=lambda batch: _exact_level_key(batch["squared_level_exact"]),
        )
        previous_post: dict[
            str, tuple[tuple[tuple[int, ...], ...], tuple[int, ...]]
        ] = {}
        created_node_ids: set[str] = set()

        for batch in ordered_batches:
            pre_snapshot = _component_snapshot(batch["pre_lot_components"])
            if pre_snapshot != previous_post:
                raise ContractError(
                    "a closed batch pre-lot snapshot is not the preceding post-lot state"
                )

            parent: dict[tuple[int, ...], tuple[int, ...]] = {}

            def add(facet: tuple[int, ...]) -> None:
                parent.setdefault(facet, facet)

            def find(facet: tuple[int, ...]) -> tuple[int, ...]:
                root = parent[facet]
                if root != facet:
                    parent[facet] = find(root)
                return parent[facet]

            def union(left: tuple[int, ...], right: tuple[int, ...]) -> None:
                left_root = find(left)
                right_root = find(right)
                if left_root == right_root:
                    return
                if left_root < right_root:
                    parent[right_root] = left_root
                else:
                    parent[left_root] = right_root

            pre_facets_by_root: dict[str, set[tuple[int, ...]]] = {}
            pre_covered_by_root: dict[str, set[int]] = {}
            for component in batch["pre_lot_components"]:
                root_id = component["component_id"]
                facets = {
                    tuple(facet) for facet in component["active_facet_point_ids"]
                }
                pre_facets_by_root[root_id] = facets
                pre_covered_by_root[root_id] = set(component["covered_point_ids"])
                for facet in facets:
                    add(facet)
                for facet in sorted(facets)[1:]:
                    union(sorted(facets)[0], facet)

            for coface_id in batch["gamma_coface_ids"]:
                facets = tuple(
                    tuple(facet) for facet in cofaces[coface_id]["facet_point_ids"]
                )
                for facet in facets:
                    add(facet)
                for facet in facets[1:]:
                    union(facets[0], facet)

            if result["profile"] == "full_pi0":
                for event_id in batch["event_ids"]:
                    event = events[event_id]
                    if event["birth_order"] == order:
                        add(tuple(sorted(event["interior_ids"] + event["shell_ids"])))
            elif order == 1:
                for event_id in batch["event_ids"]:
                    event = events[event_id]
                    if event["closed_rank"] == 1:
                        add(tuple(event["shell_ids"]))

            groups: dict[tuple[int, ...], set[tuple[int, ...]]] = {}
            for facet in parent:
                groups.setdefault(find(facet), set()).add(facet)

            post_components_by_facets: dict[
                frozenset[tuple[int, ...]], dict[str, Any]
            ] = {}
            for component in batch["post_lot_components"]:
                facet_key = frozenset(
                    tuple(facet) for facet in component["active_facet_point_ids"]
                )
                if facet_key in post_components_by_facets:
                    raise ContractError(
                        "a closed post-lot snapshot repeats one component"
                    )
                post_components_by_facets[facet_key] = component
            expected_group_keys = {frozenset(group) for group in groups.values()}
            if set(post_components_by_facets) != expected_group_keys:
                raise ContractError(
                    "a closed post-lot snapshot is not the Gamma batch contraction"
                )

            expected_deltas: list[dict[str, Any]] = []
            for group_key, component in post_components_by_facets.items():
                pre_roots = sorted(
                    root_id
                    for root_id, facets in pre_facets_by_root.items()
                    if facets.intersection(group_key)
                )
                root_id = component["component_id"]
                if not pre_roots:
                    node = local_nodes[root_id]
                    if (
                        node["kind"] != "birth"
                        or _exact_level_key(node["squared_level"])
                        != _exact_level_key(batch["squared_level_exact"])
                        or node["batch_id"]
                        not in (
                            {None, batch["batch_id"]}
                            if order == 1
                            else {batch["batch_id"]}
                        )
                    ):
                        raise ContractError(
                            "a new closed component lacks its equal-level birth node "
                            f"in batch {batch['batch_id']}"
                        )
                    created_node_ids.add(root_id)
                elif len(pre_roots) == 1:
                    if root_id != pre_roots[0]:
                        raise ContractError(
                            "a q=1 closed growth must retain its existing root"
                        )
                else:
                    node = local_nodes[root_id]
                    if (
                        node["kind"] != "merge"
                        or node["batch_id"] != batch["batch_id"]
                        or node["child_ids"] != pre_roots
                    ):
                        raise ContractError(
                            "a closed multifusion node disagrees with its pre-lot roots"
                        )
                    created_node_ids.add(root_id)

                old_facets = {
                    facet for pre_root in pre_roots for facet in pre_facets_by_root[pre_root]
                }
                old_points = {
                    point_id
                    for pre_root in pre_roots
                    for point_id in pre_covered_by_root[pre_root]
                }
                added_facets = sorted(group_key - old_facets)
                added_points = sorted(set(component["covered_point_ids"]) - old_points)
                if added_facets or added_points:
                    expected_deltas.append(
                        {
                            "batch_id": batch["batch_id"],
                            "order": order,
                            "root_node_id": root_id,
                            "added_facet_point_ids": [
                                list(facet) for facet in added_facets
                            ],
                            "added_point_ids": added_points,
                        }
                    )

            actual_deltas = [
                {key: value for key, value in delta.items() if key != "schema_version"}
                for delta in batch["coverage_deltas"]
            ]
            if sorted(actual_deltas, key=canonical_json) != sorted(
                expected_deltas, key=canonical_json
            ):
                raise ContractError(
                    "a closed batch coverage log is not its pre/post state delta "
                    f"for batch {batch['batch_id']}"
                )
            previous_post = _component_snapshot(batch["post_lot_components"])

        if set(forest["root_ids"]) != set(previous_post):
            raise ContractError(
                "a closed forest root set is not its final post-lot component set"
            )
        if set(local_nodes) != created_node_ids:
            raise ContractError(
                "a closed forest contains a node not created by its batch replay"
            )


def _validate_coverage_reference(
    delta: dict[str, Any],
    batches: dict[str, dict[str, Any]],
    nodes: dict[str, dict[str, Any]],
    node_order: dict[str, int],
    expected_order: int,
) -> None:
    _require_references([delta["batch_id"]], batches, "a coverage delta batch")
    _require_references([delta["root_node_id"]], nodes, "a coverage delta root")
    if (
        delta["order"] != expected_order
        or batches[delta["batch_id"]]["order"] != expected_order
        or node_order[delta["root_node_id"]] != expected_order
    ):
        raise ContractError("a coverage delta crosses orders")


def _validate_partial_scope(
    scope: dict[str, Any],
    certificate: dict[str, Any],
    events: dict[str, dict[str, Any]],
    n: int,
    *,
    check_canonical_ids: bool,
) -> None:
    """Validate the positive perimeter of a conditional or partial result."""

    if set(scope["positive_guarantees"]) != set(
        certificate["partial_guarantees"]
    ):
        raise ContractError(
            "partial_scope positive guarantees must match the run certificate"
        )
    if any(
        depth < 0 or depth > certificate["m_star"]
        for depth in scope["closed_parent_depths"]
    ):
        raise ContractError("partial_scope closes a parent depth outside 0..m_star")
    if any(
        order < 1 or order > certificate["k_eff"]
        for order in scope["closed_orders"]
    ):
        raise ContractError("partial_scope closes an order outside 1..k_eff")
    if any(
        rank < 1 or rank > certificate["s_max"]
        for rank in scope["catalog_complete_ranks"]
    ):
        raise ContractError("partial_scope closes a rank outside 1..s_max")
    for rank in scope["catalog_complete_ranks"]:
        if not certificate["catalog_complete_by_rank"][rank - 1]:
            raise ContractError(
                "partial_scope claims a catalog rank whose certificate gate is open"
            )
    for order in scope["closed_orders"]:
        if not certificate["batches_complete_by_order"][order - 1]:
            raise ContractError(
                "partial_scope claims an order whose batch gate is open"
            )
    if scope["closed_parent_depths"] and not certificate[
        "canonical_children_complete"
    ]:
        raise ContractError(
            "partial_scope closes parent depths while canonical children are incomplete"
        )

    if "verified_events" in scope["positive_guarantees"] and not scope[
        "verified_event_ids"
    ]:
        raise ContractError("verified_events requires at least one verified event")
    _require_references(
        scope["verified_event_ids"], events, "partial_scope.verified_event_ids"
    )
    if any(
        events[event_id]["predicate_status"] != "certified_exact"
        or events[event_id]["degeneracy_class"] == "unsupported"
        for event_id in scope["verified_event_ids"]
    ):
        raise ContractError(
            "partial_scope verified_event_ids contains an uncertified event"
        )

    cell_ids: set[str] = set()
    for cell in scope["canonical_cell_certificates"]:
        cell_id = cell["cell_id"]
        if cell_id in cell_ids:
            raise ContractError("partial_scope repeats a canonical cell certificate")
        cell_ids.add(cell_id)
        depth = cell["depth"]
        label = cell["label_point_ids"]
        if depth < 0 or depth > certificate["m_star"] or len(label) != depth:
            raise ContractError(
                "a canonical cell certificate has an invalid depth or label size"
            )
        if label != sorted(set(label)) or any(
            point_id < 0 or point_id >= n for point_id in label
        ):
            raise ContractError("a canonical cell label is outside the input domain")
        vertex_ids: set[str] = set()
        exact_vertex_projections: list[dict[str, Any]] = []
        for witness in cell["vertex_witnesses"]:
            vertex_id = witness["vertex_id"]
            if vertex_id in vertex_ids:
                raise ContractError("a canonical cell repeats a vertex witness")
            vertex_ids.add(vertex_id)
            projection = {
                "position_exact": witness["position_exact"],
                "binding_constraint_ids": witness["binding_constraint_ids"],
                "affine_dimension": witness["affine_dimension"],
                "artificial_boundary": witness["artificial_boundary"],
                "degeneracy_class": witness["degeneracy_class"],
            }
            if check_canonical_ids:
                _canonical_id(
                    vertex_id,
                    "VertexWitness",
                    projection,
                    "partial_scope.canonical_cell_certificates.vertex_id",
                )
            exact_vertex_projections.append(
                {"vertex_id": vertex_id, **projection}
            )
        closed_flags = all(
            cell[field]
            for field in (
                "global_queue_empty",
                "all_vertices_certified",
                "active_cross_incidences_complete",
                "artificial_boundary_only_marked",
            )
        )
        if (cell["certificate_status"] == "closed") != closed_flags:
            raise ContractError(
                "a canonical cell status disagrees with its closure flags"
            )
        if depth in scope["closed_parent_depths"] and not closed_flags:
            raise ContractError(
                "partial_scope contains an open cell at a declared closed depth"
            )
        if closed_flags and any(
            witness["degeneracy_class"] == "unsupported"
            for witness in cell["vertex_witnesses"]
        ):
            raise ContractError("a closed canonical cell has an unsupported vertex")
        if check_canonical_ids:
            _canonical_id(
                cell_id,
                "CanonicalCellCertificate",
                {
                    "depth": depth,
                    "label_point_ids": label,
                    "vertex_witnesses": exact_vertex_projections,
                    "cross_constraint_ids": cell["cross_constraint_ids"],
                    "active_cross_incidence_ids": cell[
                        "active_cross_incidence_ids"
                    ],
                },
                "partial_scope.canonical_cell_certificates.cell_id",
            )

    for locus in scope["unresolved_loci"]:
        point_ids = locus["point_ids"]
        if point_ids != sorted(set(point_ids)) or any(
            point_id < 0 or point_id >= n for point_id in point_ids
        ):
            raise ContractError("a partial locus is outside the input domain")
        order = locus["order"]
        depth = locus["depth"]
        if order is not None and not 1 <= order <= certificate["k_eff"]:
            raise ContractError("a partial locus order is outside 1..k_eff")
        if depth is not None and not 0 <= depth <= certificate["m_star"]:
            raise ContractError("a partial locus depth is outside 0..m_star")
        if locus["kind"] == "open_cell" and depth is None:
            raise ContractError("an open-cell locus must identify its depth")
        if locus["kind"] == "open_attachment" and order is None:
            raise ContractError("an open-attachment locus must identify its order")
        if locus["kind"] == "open_cell" and depth in scope[
            "closed_parent_depths"
        ]:
            raise ContractError("an open-cell locus contradicts a closed parent depth")
        if locus["kind"] == "open_attachment" and order in scope["closed_orders"]:
            raise ContractError("an open attachment contradicts a closed order")
        if check_canonical_ids:
            _canonical_id(
                locus["locus_id"],
                "PartialLocus",
                {
                    key: value
                    for key, value in locus.items()
                    if key not in {"schema_version", "locus_id"}
                },
                "partial_scope.unresolved_loci.locus_id",
            )


def validate_benchmark_semantics(record: dict[str, Any]) -> None:
    safety = record["gcp_safety"]
    target_fields = (
        "project",
        "zone",
        "instance_name",
        "machine_type",
        "provisioning_model",
        "instance_termination_action",
        "max_run_duration_s",
    )
    if safety["used"]:
        if any(safety[field] is None for field in target_fields):
            raise ContractError("a GCP benchmark must identify its exact guarded target")
        expected = {
            "machine_type": "g4-standard-48",
            "provisioning_model": "SPOT",
            "instance_termination_action": "STOP",
            "guest_shutdown_armed": True,
            "final_state": "TERMINATED",
        }
        if any(safety[field] != value for field, value in expected.items()):
            raise ContractError("GCP benchmark safety evidence is not fail-closed")
        if not 30 <= safety["max_run_duration_s"] <= 28_800:
            raise ContractError("GCP max run duration must be between 30s and 8h")
    else:
        if any(safety[field] is not None for field in target_fields):
            raise ContractError("an unused GCP record must not claim a target")
        if safety["guest_shutdown_armed"]:
            raise ContractError("an unused GCP record cannot claim a guest guard")
        if safety["initial_state"] != "NOT_USED" or safety["final_state"] != "NOT_USED":
            raise ContractError("an unused GCP record must use NOT_USED states")


def validate_repository_contract() -> tuple[int, int]:
    schema = load_json(SCHEMA_PATH)
    if not isinstance(schema, dict):
        raise ContractError("root schema must be an object")
    if schema.get("$schema") != "https://json-schema.org/draft/2020-12/schema":
        raise ContractError("contract must declare JSON Schema Draft 2020-12")
    if schema.get("$id") != (
        "https://e-hgp.invalid/schemas/morsehgp3d-contract-v2.schema.json"
    ):
        raise ContractError("the active contract must declare the v2 schema identifier")
    definitions = schema.get("$defs")
    if not isinstance(definitions, dict):
        raise ContractError("contract must contain an object-valued $defs")
    missing = sorted(REQUIRED_DEFINITIONS - definitions.keys())
    if missing:
        raise ContractError(f"contract is missing required definitions: {missing!r}")
    if definitions.get("SchemaVersion", {}).get("const") != "2.0.0":
        raise ContractError("the active contract must require schema_version=2.0.0")

    legacy_schema = load_json(LEGACY_SCHEMA_PATH)
    legacy_definitions = legacy_schema.get("$defs", {})
    if legacy_schema.get("$id") != (
        "https://e-hgp.invalid/schemas/morsehgp3d-contract-v1.schema.json"
    ) or legacy_definitions.get("SchemaVersion", {}).get("const") != "1.0.0":
        raise ContractError("the archived v1 schema must remain identifiable and immutable")

    validator = SchemaValidator(schema)
    sample_count = 0
    for name in sorted(REQUIRED_DEFINITIONS):
        definition = definitions[name]
        examples = definition.get("examples") if isinstance(definition, dict) else None
        if not isinstance(examples, list) or not examples:
            raise ContractError(f"$defs/{name}: at least one example is required")
        for index, example in enumerate(examples):
            validator.check(example, definition, f"$defs.{name}.examples[{index}]")
            validate_round_trip(example, f"$defs/{name}/examples/{index}")
            if name == "RunCertificate":
                validate_certificate_semantics(example)
            elif name == "MorseHGP3DResult":
                validate_result_semantics(example)
            elif name == "BenchmarkRecord":
                validate_benchmark_semantics(example)
            sample_count += 1

    fixture_count = 0
    if FIXTURE_DIR.is_dir():
        for path in sorted(FIXTURE_DIR.glob("*.json")):
            fixture = load_json(path)
            validator.check(fixture, path=f"fixture:{path.name}")
            validate_result_semantics(fixture)
            validate_round_trip(fixture, str(path.relative_to(ROOT)))
            fixture_count += 1
    return sample_count, fixture_count


def main() -> int:
    try:
        samples, fixtures = validate_repository_contract()
    except ContractError as exc:
        print(f"Contract validation failed: {exc}", file=sys.stderr)
        return 1
    print(
        f"Validated {len(REQUIRED_DEFINITIONS)} contract definitions, "
        f"{samples} schema examples and {fixtures} result fixtures."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
