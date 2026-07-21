from __future__ import annotations

import struct
import unittest
from collections import Counter
from dataclasses import replace
from fractions import Fraction

from reference.morsehgp3d_oracle.ordinary_diagram import (
    OrdinaryDiagramContactKind,
    OrdinaryDiagramOracleAudit,
    OrdinaryDiagramOracleBudget,
    OrdinaryDiagramOracleDecision,
    build_bounded_ordinary_subset_oracle,
)


SIGN_MASK = 1 << 63
POSITIVE_MAXIMUM_FINITE_BITS = 0x7FEFFFFFFFFFFFFF
NEGATIVE_MAXIMUM_FINITE_BITS = 0xFFEFFFFFFFFFFFFF


def word(value: int | float | str) -> str:
    if isinstance(value, str):
        if len(value) != 16:
            raise AssertionError("a fixture word must contain sixteen hex digits")
        return value
    return struct.pack(">d", float(value)).hex()


def point(
    x: int | float | str,
    y: int | float | str,
    z: int | float | str,
) -> tuple[str, str, str]:
    return word(x), word(y), word(z)


def canonical_word(value: str) -> str:
    return "0000000000000000" if int(value, 16) == SIGN_MASK else value


def total_order_key(value: str) -> int:
    bits = int(canonical_word(value), 16)
    return (~bits) & ((1 << 64) - 1) if bits & SIGN_MASK else bits ^ SIGN_MASK


def canonical_manifest(
    source: tuple[tuple[str, str, str], ...],
) -> tuple[tuple[str, str, str], ...]:
    normalized = [tuple(canonical_word(value) for value in item) for item in source]
    normalized.sort(key=lambda item: tuple(total_order_key(value) for value in item))
    if len(set(normalized)) != len(normalized):
        raise AssertionError("an oracle fixture contains duplicate sites")
    return tuple(normalized)  # type: ignore[return-value]


def predecessor(value: str) -> str:
    bits = int(canonical_word(value), 16)
    if bits == NEGATIVE_MAXIMUM_FINITE_BITS:
        raise AssertionError("fixture predecessor would be infinite")
    if bits == 0:
        return f"{SIGN_MASK | 1:016x}"
    return canonical_word(f"{bits + 1 if bits & SIGN_MASK else bits - 1:016x}")


def successor(value: str) -> str:
    bits = int(canonical_word(value), 16)
    if bits == POSITIVE_MAXIMUM_FINITE_BITS:
        raise AssertionError("fixture successor would be infinite")
    return canonical_word(f"{bits - 1 if bits & SIGN_MASK else bits + 1:016x}")


def omega(
    manifest: tuple[tuple[str, str, str], ...],
) -> tuple[tuple[str, str, str], tuple[str, str, str]]:
    lower: list[str] = []
    upper: list[str] = []
    for axis in range(3):
        values = tuple(item[axis] for item in manifest)
        lower.append(predecessor(min(values, key=total_order_key)))
        upper.append(successor(max(values, key=total_order_key)))
    return tuple(lower), tuple(upper)  # type: ignore[return-value]


def build(
    source: tuple[tuple[str, str, str], ...],
    budget: OrdinaryDiagramOracleBudget | None = None,
):
    manifest = canonical_manifest(source)
    return build_bounded_ordinary_subset_oracle(
        manifest,
        omega(manifest),
        budget,
    )


def cube_source() -> tuple[tuple[str, str, str], ...]:
    return tuple(
        point(x, y, z)
        for x in (-1, 1)
        for y in (-1, 1)
        for z in (-1, 1)
    )


class OrdinaryDiagramOracleTests(unittest.TestCase):
    def test_singleton_is_its_strictly_padded_box(self) -> None:
        source = (
            point(
                "8000000000000000",
                "0000000000000001",
                "8000000000000001",
            ),
        )
        result = build(source)
        self.assertIs(result.decision, OrdinaryDiagramOracleDecision.COMPLETE)
        self.assertEqual(len(result.cells), 1)
        self.assertEqual(len(result.cells[0].vertices), 8)
        self.assertEqual(len(result.global_vertices), 8)
        self.assertEqual(result.contacts, ())
        self.assertTrue(
            all(vertex.shell_ids == (0,) and vertex.owner_ids == (0,)
                for vertex in result.global_vertices)
        )
        self.assertEqual(result.audit.cell_vertex_reference_count, 8)
        self.assertEqual(result.semantic_projection()["contacts"], ())

    def test_collinear_cloud_has_only_adjacent_faces(self) -> None:
        result = build(tuple(point(value, 0, 0) for value in (0, 2, 4, 6)))
        self.assertEqual(
            tuple(contact.query_ids for contact in result.contacts),
            ((0, 1), (1, 2), (2, 3)),
        )
        self.assertTrue(
            all(
                contact.kind is OrdinaryDiagramContactKind.NATURAL_FACE
                and contact.affine_dimension == 2
                and contact.site_affine_rank == 1
                for contact in result.contacts
            )
        )
        self.assertEqual(len(result.cells), 4)
        self.assertTrue(
            all(vertex.shell_ids == vertex.owner_ids for vertex in result.global_vertices)
        )

    def test_cocircular_square_keeps_quotients_out_of_natural_faces(self) -> None:
        result = build(
            (
                point(-1, -1, 0),
                point(1, -1, 0),
                point(-1, 1, 0),
                point(1, 1, 0),
            )
        )
        kinds = Counter(contact.kind for contact in result.contacts)
        self.assertEqual(len(result.contacts), 11)
        self.assertEqual(kinds[OrdinaryDiagramContactKind.NATURAL_FACE], 4)
        self.assertEqual(kinds[OrdinaryDiagramContactKind.NATURAL_EDGE], 1)
        self.assertEqual(
            kinds[OrdinaryDiagramContactKind.NONCANONICAL_QUOTIENT_CONTACT], 6
        )
        contacts = {contact.query_ids: contact for contact in result.contacts}
        diagonal = contacts[(0, 3)]
        self.assertIs(
            diagonal.kind,
            OrdinaryDiagramContactKind.NONCANONICAL_QUOTIENT_CONTACT,
        )
        self.assertEqual(diagonal.carrier_ids, (0, 1, 2, 3))
        carrier = contacts[(0, 1, 2, 3)]
        self.assertIs(carrier.kind, OrdinaryDiagramContactKind.NATURAL_EDGE)
        self.assertEqual(carrier.vertex_positions, diagonal.vertex_positions)

    def test_cube_has_one_shell_eight_vertex_and_exact_caps(self) -> None:
        result = build(cube_source())
        self.assertIs(result.decision, OrdinaryDiagramOracleDecision.COMPLETE)
        self.assertEqual(
            (
                result.requirements.conservative_subset_count,
                result.requirements.conservative_equality_count,
                result.requirements.conservative_inequality_count,
                result.requirements.conservative_candidate_system_count,
                result.requirements.conservative_form_evaluation_count,
                result.requirements.conservative_distance_evaluation_count,
                result.requirements.conservative_cell_vertex_reference_count,
                result.requirements.conservative_contact_vertex_reference_count,
                result.requirements.conservative_witness_count,
            ),
            (255, 769, 2546, 13349, 142982, 108768, 2288, 11061, 247),
        )
        self.assertEqual(len(result.contacts), 247)
        kinds = Counter(contact.kind for contact in result.contacts)
        self.assertEqual(kinds[OrdinaryDiagramContactKind.NATURAL_FACE], 12)
        self.assertEqual(kinds[OrdinaryDiagramContactKind.NATURAL_EDGE], 6)
        self.assertEqual(kinds[OrdinaryDiagramContactKind.NATURAL_VERTEX], 1)
        self.assertEqual(
            kinds[OrdinaryDiagramContactKind.NONCANONICAL_QUOTIENT_CONTACT], 228
        )
        center = next(
            vertex
            for vertex in result.global_vertices
            if vertex.position == (Fraction(), Fraction(), Fraction())
        )
        self.assertEqual(center.shell_ids, tuple(range(8)))
        self.assertEqual(center.owner_ids, tuple(range(8)))
        self.assertEqual(center.box_mask, 0)

    def test_contact_entirely_on_box_face_is_never_natural(self) -> None:
        base = 1 << 52
        result = build(
            (
                point(-2, base + 1, 0),
                point(2, base + 1, 0),
                point(-1, base + 2, 0),
            )
        )
        contact = next(item for item in result.contacts if len(item.query_ids) == 3)
        self.assertEqual(contact.query_ids, (0, 1, 2))
        self.assertEqual(contact.carrier_ids, (0, 1, 2))
        self.assertIs(
            contact.kind,
            OrdinaryDiagramContactKind.BOX_SUPPORTED_CONTACT,
        )
        self.assertEqual(contact.box_mask, 4)
        self.assertEqual(contact.affine_dimension, 1)
        self.assertEqual(contact.site_affine_rank, 2)

    def test_nine_caps_fail_atomically_below_and_reject_above_trust(self) -> None:
        source = cube_source()
        manifest = canonical_manifest(source)
        clipping_box = omega(manifest)
        probe = build_bounded_ordinary_subset_oracle(
            manifest,
            clipping_box,
            OrdinaryDiagramOracleBudget(maximum_subset_count=0),
        )
        self.assertIs(
            probe.decision,
            OrdinaryDiagramOracleDecision.INSUFFICIENT_BUDGET,
        )
        requirements = probe.requirements
        cases = (
            ("maximum_subset_count", "conservative_subset_count"),
            ("maximum_equality_count", "conservative_equality_count"),
            ("maximum_inequality_count", "conservative_inequality_count"),
            ("maximum_candidate_system_count", "conservative_candidate_system_count"),
            ("maximum_form_evaluation_count", "conservative_form_evaluation_count"),
            ("maximum_distance_evaluation_count", "conservative_distance_evaluation_count"),
            (
                "maximum_cell_vertex_reference_count",
                "conservative_cell_vertex_reference_count",
            ),
            (
                "maximum_contact_vertex_reference_count",
                "conservative_contact_vertex_reference_count",
            ),
            ("maximum_witness_count", "conservative_witness_count"),
        )
        defaults = OrdinaryDiagramOracleBudget()
        exact_values: dict[str, int] = {}
        for budget_name, requirement_name in cases:
            requirement = getattr(requirements, requirement_name)
            exact_values[budget_name] = requirement
            insufficient = build_bounded_ordinary_subset_oracle(
                manifest,
                clipping_box,
                replace(defaults, **{budget_name: requirement - 1}),
            )
            self.assertIs(
                insufficient.decision,
                OrdinaryDiagramOracleDecision.INSUFFICIENT_BUDGET,
                budget_name,
            )
            self.assertEqual(insufficient.canonical_point_words, manifest)
            self.assertEqual(insufficient.omega_words, clipping_box)
            self.assertEqual(insufficient.requirements, requirements)
            self.assertEqual(insufficient.audit, OrdinaryDiagramOracleAudit())
            self.assertEqual(insufficient.cells, ())
            self.assertEqual(insufficient.global_vertices, ())
            self.assertEqual(insufficient.contacts, ())

            trusted = getattr(
                OrdinaryDiagramOracleBudget,
                "trusted_" + budget_name,
            )
            with self.assertRaises(ValueError, msg=budget_name):
                build_bounded_ordinary_subset_oracle(
                    manifest,
                    clipping_box,
                    replace(defaults, **{budget_name: trusted + 1}),
                )

        exact = build_bounded_ordinary_subset_oracle(
            manifest,
            clipping_box,
            OrdinaryDiagramOracleBudget(**exact_values),
        )
        self.assertIs(exact.decision, OrdinaryDiagramOracleDecision.COMPLETE)

        same_box_source = cube_source()[:-1] + (point(0, 0, 0),)
        same_box_manifest = canonical_manifest(same_box_source)
        same_box = omega(same_box_manifest)
        self.assertEqual(same_box, clipping_box)
        self.assertNotEqual(same_box_manifest, manifest)
        same_box_receipt = build_bounded_ordinary_subset_oracle(
            same_box_manifest,
            same_box,
            OrdinaryDiagramOracleBudget(maximum_subset_count=0),
        )
        self.assertIs(
            same_box_receipt.decision,
            OrdinaryDiagramOracleDecision.INSUFFICIENT_BUDGET,
        )
        self.assertEqual(same_box_receipt.canonical_point_words, same_box_manifest)
        self.assertNotEqual(same_box_receipt.canonical_point_words, manifest)
        self.assertEqual(same_box_receipt.omega_words, clipping_box)
        self.assertEqual(same_box_receipt.requirements, requirements)
        self.assertEqual(same_box_receipt.audit, OrdinaryDiagramOracleAudit())
        self.assertEqual(same_box_receipt.cells, ())
        self.assertEqual(same_box_receipt.global_vertices, ())
        self.assertEqual(same_box_receipt.contacts, ())


if __name__ == "__main__":
    unittest.main()
