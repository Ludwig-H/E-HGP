from __future__ import annotations

import unittest
from dataclasses import FrozenInstanceError, fields, replace
from fractions import Fraction
from unittest.mock import patch

from reference.morsehgp3d_oracle import (
    direct_gateway_differential as differential_module,
)
from reference.morsehgp3d_oracle.direct_gateway_differential import (
    DifferentialBoundary,
    DifferentialDecision,
    DifferentialEvidenceStatus,
    DifferentialMismatch,
    DifferentialPathElementKind,
    DifferentialProvenance,
    DifferentialStage,
    DifferentialVerdict,
    build_direct_gateway_gamma_differential,
    verify_direct_gateway_gamma_differential,
)
from reference.morsehgp3d_oracle.gamma import GammaFiltration, build_gamma_filtration
from reference.morsehgp3d_oracle.generators import generate_affine_cloud
from tools.check_oracle_independence import audit_oracle_imports

Point3 = tuple[int, int, int]

E5_POINTS: tuple[Point3, ...] = (
    (0, 0, 7),  # A
    (0, 9, 6),  # B
    (1, 4, 0),  # C
    (0, 0, 1),  # D
    (4, 1, 2),  # E
)
E5_LABELS = ("A", "B", "C", "D", "E")
E5_SILENT_LEVEL = Fraction(33, 2)
E5_CONTINUATION_LEVEL = Fraction(83886, 3563)


def _canonical_ids(*labels: str) -> tuple[int, ...]:
    """Translate fixture letters to the oracle's lexicographic point IDs."""

    by_label = dict(zip(E5_LABELS, E5_POINTS, strict=True))
    by_point = {point: point_id for point_id, point in enumerate(sorted(E5_POINTS))}
    return tuple(sorted(by_point[by_label[label]] for label in labels))


E5_AC = _canonical_ids("A", "C")
E5_ABC = _canonical_ids("A", "B", "C")
E5_ACD = _canonical_ids("A", "C", "D")
E5_ACE = _canonical_ids("A", "C", "E")


class _AlwaysEqual:
    """Hostile payload that makes ordinary dataclass equality unsound."""

    def __eq__(self, other: object) -> bool:
        return True


def _checkpoint(result, level: Fraction, boundary: DifferentialBoundary):
    return next(
        checkpoint
        for checkpoint in result.cut_results
        if checkpoint.squared_level == level and checkpoint.boundary is boundary
    )


def _all_facets(components) -> set[tuple[int, ...]]:
    return {facet for component in components for facet in component}


def _component_by_facet(components, facet: tuple[int, ...]):
    return next(component for component in components if facet in component)


def _reversed_coface_filtration(points, order: int) -> GammaFiltration:
    """Bypass the storage-order guard solely to stress equal-lot invariance."""

    filtration = build_gamma_filtration(points, order)
    reordered = object.__new__(GammaFiltration)
    for field in fields(GammaFiltration):
        value = getattr(filtration, field.name)
        if field.name == "cofaces":
            value = tuple(reversed(value))
        object.__setattr__(reordered, field.name, value)
    return reordered


def _amputated_filtration(
    points,
    order: int,
    *,
    removed_facet: tuple[int, ...] | None = None,
    removed_coface: tuple[int, ...] | None = None,
) -> GammaFiltration:
    """Build a structurally valid-looking but scientifically incomplete Gamma."""

    filtration = build_gamma_filtration(points, order)
    amputated = object.__new__(GammaFiltration)
    for field in fields(GammaFiltration):
        value = getattr(filtration, field.name)
        if field.name == "facets" and removed_facet is not None:
            value = tuple(item for item in value if item.point_ids != removed_facet)
        if field.name == "cofaces" and removed_coface is not None:
            value = tuple(item for item in value if item.point_ids != removed_coface)
        object.__setattr__(amputated, field.name, value)
    return amputated


class DirectGatewayGammaDifferentialDomainTests(unittest.TestCase):
    def test_structural_domain_is_rejected_before_exhaustive_enumeration(self) -> None:
        valid = E5_POINTS
        invalid_calls = (
            (valid[:2], 2),
            (valid, 1),
            (valid, 5),
            (tuple((index, index * index, index**3) for index in range(15)), 2),
        )
        with (
            patch.object(
                differential_module,
                "build_critical_catalog",
                side_effect=AssertionError("catalogue reached before domain guard"),
            ) as catalog_builder,
            patch.object(
                differential_module,
                "build_gamma_filtration",
                side_effect=AssertionError("Gamma reached before domain guard"),
            ) as gamma_builder,
        ):
            for points, order in invalid_calls:
                with self.subTest(point_count=len(points), order=order):
                    with self.assertRaises(ValueError):
                        build_direct_gateway_gamma_differential(points, order)

            with self.assertRaises(TypeError):
                build_direct_gateway_gamma_differential(valid, True)
            catalog_builder.assert_not_called()
            gamma_builder.assert_not_called()

    def test_relevant_gp_violation_is_an_explicit_unsupported_result(self) -> None:
        right_triangle = ((0, 0, 0), (2, 0, 0), (0, 2, 0))
        result = build_direct_gateway_gamma_differential(right_triangle, 2)

        self.assertEqual(result.decision, DifferentialDecision.UNSUPPORTED_DEGENERACY)
        self.assertEqual(
            result.evidence_status,
            DifferentialEvidenceStatus.UNSUPPORTED_DEGENERACY,
        )
        self.assertEqual(
            result.direct_alphabet_verdict,
            DifferentialVerdict.UNSUPPORTED_DEGENERACY,
        )
        self.assertEqual(
            result.gateway_generator_verdict,
            DifferentialVerdict.UNSUPPORTED_DEGENERACY,
        )
        self.assertFalse(result.relevant_gp)
        self.assertTrue(result.degeneracy_witnesses)
        self.assertEqual(result.cut_results, ())
        self.assertIsNone(result.first_failure)
        self.assertEqual(result.scope, "oracle_only")
        self.assertEqual(result.public_status, "not_claimed")
        self.assertFalse(result.diagnostic_ablation)

        excluded = ((0, 1, 2),)
        diagnostic = build_direct_gateway_gamma_differential(
            right_triangle,
            2,
            excluded_gateway_cofaces=excluded,
        )
        self.assertEqual(
            diagnostic.evidence_status,
            DifferentialEvidenceStatus.UNSUPPORTED_DEGENERACY,
        )
        self.assertEqual(
            diagnostic.decision,
            DifferentialDecision.UNSUPPORTED_DEGENERACY,
        )
        self.assertTrue(diagnostic.diagnostic_ablation)
        self.assertFalse(diagnostic.bounded_equivalent)
        self.assertEqual(diagnostic.excluded_gateway_cofaces, excluded)
        self.assertEqual(diagnostic.cut_results, ())

    def test_reference_module_remains_independent_from_production(self) -> None:
        self.assertEqual(audit_oracle_imports(), ())


class DirectGatewayGammaDifferentialE5Tests(unittest.TestCase):
    def test_e5_passes_both_projected_hypotheses_without_ablation(self) -> None:
        result = build_direct_gateway_gamma_differential(E5_POINTS, 2)

        self.assertEqual(
            result.decision, DifferentialDecision.OPEN_BOUNDED_EVIDENCE_ONLY
        )
        self.assertEqual(
            result.evidence_status,
            DifferentialEvidenceStatus.OPEN_BOUNDED_EVIDENCE_ONLY,
        )
        self.assertEqual(result.scope, "oracle_only")
        self.assertEqual(result.public_status, "not_claimed")
        self.assertFalse(result.diagnostic_ablation)
        self.assertEqual(result.direct_alphabet_verdict, DifferentialVerdict.EQUIVALENT)
        self.assertEqual(
            result.gateway_generator_verdict, DifferentialVerdict.EQUIVALENT
        )
        self.assertIs(result.facet_partition_equivalent, True)
        self.assertIs(
            result.root_genealogy_and_covered_points_equivalent,
            True,
        )
        self.assertIsNone(result.first_failure)
        self.assertIsNone(result.direct_alphabet_witness)
        self.assertIsNone(result.gateway_generator_witness)

        self.assertEqual(result.points, tuple(sorted(E5_POINTS)))
        self.assertIn(E5_AC, result.direct_facets)
        self.assertIn(E5_AC, result.direct_alphabet)
        self.assertIn(E5_ABC, result.direct_saddle_cofaces)
        self.assertIn(E5_ACD, result.first_incidence_cofaces)
        self.assertIn(E5_ACE, result.first_incidence_cofaces)
        self.assertIn(E5_ACD, result.generated_cofaces)
        self.assertIn(E5_ACE, result.generated_cofaces)

    def test_every_gamma_level_is_checked_open_then_closed(self) -> None:
        result = build_direct_gateway_gamma_differential(E5_POINTS, 2)
        levels = build_gamma_filtration(result.points, 2).critical_levels

        self.assertEqual(
            tuple(
                (checkpoint.squared_level, checkpoint.boundary)
                for checkpoint in result.cut_results
            ),
            tuple(
                (level, boundary)
                for level in levels
                for boundary in (
                    DifferentialBoundary.OPEN,
                    DifferentialBoundary.CLOSED,
                )
            ),
        )

    def test_active_birth_facets_are_retained_as_singletons_before_any_coface(
        self,
    ) -> None:
        result = build_direct_gateway_gamma_differential(E5_POINTS, 2)
        checkpoint = _checkpoint(result, Fraction(9, 2), DifferentialBoundary.CLOSED)
        expected_singletons = (((0, 3),), ((0, 4),))

        self.assertEqual(
            checkpoint.direct_alphabet_reference_components,
            expected_singletons,
        )
        self.assertEqual(
            checkpoint.direct_alphabet_candidate_components,
            expected_singletons,
        )
        self.assertEqual(checkpoint.gateway_reference_components, expected_singletons)
        self.assertEqual(checkpoint.gateway_candidate_components, expected_singletons)
        self.assertTrue(checkpoint.direct_alphabet_partition_equivalent)
        self.assertTrue(checkpoint.facet_partition_equivalent)

    def test_full_gamma_facets_outside_v_remain_diagnostic_only(self) -> None:
        points = generate_affine_cloud(3, 5, 0, coordinate_bound=19)
        result = build_direct_gateway_gamma_differential(points, 2)
        diagnostic = next(
            checkpoint
            for checkpoint in result.cut_results
            if not checkpoint.full_nontrivial_facet_coverage_equivalent_diagnostic
        )

        self.assertEqual(
            result.decision, DifferentialDecision.OPEN_BOUNDED_EVIDENCE_ONLY
        )
        self.assertTrue(diagnostic.facet_partition_equivalent)
        self.assertTrue(diagnostic.root_genealogy_and_covered_points_equivalent)
        self.assertEqual(
            diagnostic.gateway_reference_components,
            diagnostic.gateway_candidate_components,
        )
        self.assertEqual(diagnostic.full_reference_nontrivial_component_count, 1)
        self.assertEqual(diagnostic.full_candidate_nontrivial_component_count, 1)
        missing_full_facet = diagnostic.first_missing_full_facet_diagnostic
        self.assertIsNotNone(missing_full_facet)
        self.assertNotIn(missing_full_facet, result.direct_alphabet)

    def test_e5_ablation_loses_ac_then_its_root_genealogy(self) -> None:
        result = build_direct_gateway_gamma_differential(
            E5_POINTS,
            2,
            excluded_gateway_cofaces=(E5_ACD, E5_ACE),
        )

        self.assertEqual(
            result.decision,
            DifferentialDecision.GATEWAY_GENERATOR_INSUFFICIENT,
        )
        self.assertEqual(
            result.evidence_status,
            DifferentialEvidenceStatus.DIAGNOSTIC_ABLATION,
        )
        self.assertTrue(result.diagnostic_ablation)
        self.assertFalse(result.bounded_equivalent)
        self.assertEqual(result.direct_alphabet_verdict, DifferentialVerdict.EQUIVALENT)
        self.assertEqual(
            result.gateway_generator_verdict, DifferentialVerdict.INSUFFICIENT
        )
        self.assertIs(result.facet_partition_equivalent, False)
        self.assertIs(
            result.root_genealogy_and_covered_points_equivalent,
            False,
        )
        self.assertEqual(result.excluded_gateway_cofaces, (E5_ACD, E5_ACE))
        self.assertNotIn(E5_ACD, result.generated_cofaces)
        self.assertNotIn(E5_ACE, result.generated_cofaces)

        silent_cut = _checkpoint(result, E5_SILENT_LEVEL, DifferentialBoundary.CLOSED)
        self.assertTrue(silent_cut.direct_alphabet_partition_equivalent)
        self.assertFalse(silent_cut.facet_partition_equivalent)
        self.assertIn(E5_AC, _all_facets(silent_cut.gateway_reference_components))
        self.assertIn(E5_AC, _all_facets(silent_cut.gateway_candidate_components))
        self.assertEqual(
            _component_by_facet(silent_cut.gateway_candidate_components, E5_AC),
            (E5_AC,),
        )

        witness = result.gateway_generator_witness
        self.assertIsNotNone(witness)
        assert witness is not None
        self.assertEqual(result.first_failure, witness)
        self.assertEqual(witness.squared_level, E5_SILENT_LEVEL)
        self.assertEqual(witness.boundary, DifferentialBoundary.CLOSED)
        self.assertEqual(witness.stage, DifferentialStage.GATEWAY_GENERATOR)
        self.assertEqual(
            witness.mismatch,
            DifferentialMismatch.REFERENCE_CONNECTION_MISSING,
        )
        self.assertIsNone(witness.first_absent_facet)
        self.assertEqual(witness.separated_facets, ((0, 1), E5_AC))
        self.assertEqual(witness.first_ungenerated_coface, E5_ACD)
        self.assertEqual(
            tuple(element.kind for element in witness.gamma_path),
            (
                DifferentialPathElementKind.FACET,
                DifferentialPathElementKind.COFACE,
                DifferentialPathElementKind.FACET,
            ),
        )
        self.assertEqual(witness.gamma_path[1].point_ids, E5_ACD)
        self.assertIn(
            DifferentialProvenance.FIRST_INCIDENCE,
            witness.gamma_path[1].provenance,
        )

        continuation = _checkpoint(
            result, E5_CONTINUATION_LEVEL, DifferentialBoundary.CLOSED
        )
        self.assertFalse(continuation.root_genealogy_and_covered_points_equivalent)
        reconverged = _checkpoint(result, Fraction(24), DifferentialBoundary.CLOSED)
        self.assertTrue(reconverged.facet_partition_equivalent)
        self.assertEqual(
            reconverged.reference_covered_point_partitions,
            reconverged.candidate_covered_point_partitions,
        )
        self.assertFalse(reconverged.root_genealogy_and_covered_points_equivalent)
        self.assertTrue(witness.gamma_path)
        self.assertIsNotNone(witness.first_ungenerated_coface)

    def test_permutations_exclusion_canonicalization_and_frozen_witnesses(
        self,
    ) -> None:
        nominal = build_direct_gateway_gamma_differential(E5_POINTS, 2)
        self.assertEqual(
            build_direct_gateway_gamma_differential(tuple(reversed(E5_POINTS)), 2),
            nominal,
        )
        self.assertEqual(
            build_direct_gateway_gamma_differential(E5_POINTS[2:] + E5_POINTS[:2], 2),
            nominal,
        )

        ablated = build_direct_gateway_gamma_differential(
            E5_POINTS,
            2,
            excluded_gateway_cofaces=(E5_ACD, E5_ACE),
        )
        noncanonical_exclusions = (
            tuple(reversed(E5_ACE)),
            tuple(reversed(E5_ACD)),
            E5_ACD,
        )
        self.assertEqual(
            build_direct_gateway_gamma_differential(
                tuple(reversed(E5_POINTS)),
                2,
                excluded_gateway_cofaces=noncanonical_exclusions,
            ),
            ablated,
        )

        checkpoint = ablated.cut_results[0]
        witness = ablated.gateway_generator_witness
        self.assertIsNotNone(witness)
        assert witness is not None
        path_element = witness.gamma_path[0]
        for frozen, attribute, value in (
            (
                ablated,
                "decision",
                DifferentialDecision.DIRECT_ALPHABET_INSUFFICIENT,
            ),
            (checkpoint, "boundary", DifferentialBoundary.CLOSED),
            (witness, "mismatch", DifferentialMismatch.FACET_ABSENT),
            (path_element, "point_ids", (99,)),
        ):
            with self.subTest(type=type(frozen).__name__, attribute=attribute):
                with self.assertRaises(FrozenInstanceError):
                    setattr(frozen, attribute, value)

        exclusions = (E5_ACD, E5_ACE)
        self.assertTrue(
            verify_direct_gateway_gamma_differential(
                E5_POINTS,
                2,
                ablated,
                excluded_gateway_cofaces=exclusions,
            )
        )
        self.assertFalse(
            verify_direct_gateway_gamma_differential(
                E5_POINTS,
                2,
                None,
                excluded_gateway_cofaces=exclusions,
            )
        )

        forged_checkpoint = replace(
            checkpoint,
            boundary=DifferentialBoundary.CLOSED,
        )
        forged_checkpoints = (forged_checkpoint,) + ablated.cut_results[1:]
        forged_witness = replace(
            witness,
            first_ungenerated_coface=E5_ACE,
        )
        for forged in (
            replace(ablated, cut_results=forged_checkpoints),
            replace(ablated, gateway_generator_witness=forged_witness),
        ):
            with self.subTest(forgery=type(forged).__name__):
                self.assertFalse(
                    verify_direct_gateway_gamma_differential(
                        E5_POINTS,
                        2,
                        forged,
                        excluded_gateway_cofaces=exclusions,
                    )
                )

    def test_strict_verifier_rejects_equal_hostile_values_and_incomplete_object(
        self,
    ) -> None:
        nominal = build_direct_gateway_gamma_differential(E5_POINTS, 2)
        closed_twenty_four_index = next(
            index
            for index, checkpoint in enumerate(nominal.cut_results)
            if checkpoint.squared_level == 24 and checkpoint.closed
        )
        checkpoint = nominal.cut_results[closed_twenty_four_index]
        integer_level_checkpoint = replace(checkpoint, squared_level=24)
        integer_level_checks = list(nominal.cut_results)
        integer_level_checks[closed_twenty_four_index] = integer_level_checkpoint

        enum_as_string = replace(nominal, decision=nominal.decision.value)
        always_equal = replace(nominal, decision=_AlwaysEqual())
        fraction_as_integer = replace(
            nominal,
            cut_results=tuple(integer_level_checks),
        )
        incomplete = object.__new__(type(nominal))

        self.assertEqual(nominal, enum_as_string)
        self.assertEqual(nominal, always_equal)
        self.assertEqual(nominal, fraction_as_integer)
        for hostile in (
            enum_as_string,
            always_equal,
            fraction_as_integer,
            incomplete,
        ):
            with self.subTest(hostile=type(hostile).__name__):
                self.assertFalse(
                    verify_direct_gateway_gamma_differential(
                        E5_POINTS,
                        2,
                        hostile,
                    )
                )

    def test_even_equivalent_single_ablation_is_not_bounded_evidence(self) -> None:
        ablated = build_direct_gateway_gamma_differential(
            E5_POINTS,
            2,
            excluded_gateway_cofaces=(E5_ACD,),
        )

        self.assertEqual(
            ablated.decision,
            DifferentialDecision.OPEN_BOUNDED_EVIDENCE_ONLY,
        )
        self.assertEqual(
            ablated.evidence_status,
            DifferentialEvidenceStatus.DIAGNOSTIC_ABLATION,
        )
        self.assertTrue(ablated.diagnostic_ablation)
        self.assertTrue(ablated.facet_partition_equivalent)
        self.assertTrue(ablated.root_genealogy_and_covered_points_equivalent)
        self.assertFalse(ablated.bounded_equivalent)

    def test_reversing_stored_cofaces_preserves_equal_level_batches_and_witness(
        self,
    ) -> None:
        nominal = build_direct_gateway_gamma_differential(E5_POINTS, 2)
        exclusions = (E5_ACD, E5_ACE)
        ablated = build_direct_gateway_gamma_differential(
            E5_POINTS,
            2,
            excluded_gateway_cofaces=exclusions,
        )

        with patch.object(
            differential_module,
            "build_gamma_filtration",
            side_effect=_reversed_coface_filtration,
        ):
            reordered_nominal = build_direct_gateway_gamma_differential(E5_POINTS, 2)
            reordered_ablated = build_direct_gateway_gamma_differential(
                E5_POINTS,
                2,
                excluded_gateway_cofaces=tuple(reversed(exclusions)),
            )

        self.assertEqual(reordered_nominal, nominal)
        self.assertEqual(reordered_ablated, ablated)

    def test_catalogue_to_gamma_guard_rejects_amputated_births_and_direct_cofaces(
        self,
    ) -> None:
        amputations = (
            (
                "birth_facet",
                lambda points, order: _amputated_filtration(
                    points,
                    order,
                    removed_facet=(0, 1),
                ),
            ),
            (
                "direct_coface",
                lambda points, order: _amputated_filtration(
                    points,
                    order,
                    removed_coface=E5_ABC,
                ),
            ),
        )
        for name, builder in amputations:
            with self.subTest(amputation=name):
                with patch.object(
                    differential_module,
                    "build_gamma_filtration",
                    side_effect=builder,
                ):
                    with self.assertRaises(AssertionError):
                        build_direct_gateway_gamma_differential(E5_POINTS, 2)


class DirectGatewayGammaDifferentialCampaignTests(unittest.TestCase):
    def _assert_unablated_bounded_evidence(self, points, order: int) -> None:
        result = build_direct_gateway_gamma_differential(points, order)
        if (
            result.evidence_status
            is not DifferentialEvidenceStatus.OPEN_BOUNDED_EVIDENCE_ONLY
        ):
            witness = result.first_failure
            witness_payload = (
                None
                if witness is None
                else {
                    "level": str(witness.squared_level),
                    "boundary": witness.boundary.value,
                    "stage": witness.stage.value,
                    "mismatch": witness.mismatch.value,
                    "path": tuple(
                        (item.kind.value, item.point_ids) for item in witness.gamma_path
                    ),
                }
            )
            self.fail(
                "unregistered scientific direct-gateway counterexample: "
                f"points={points!r}, order={order}, "
                f"decision={result.decision.value}, witness={witness_payload!r}"
            )
        self.assertTrue(result.relevant_gp)
        self.assertEqual(result.scope, "oracle_only")
        self.assertEqual(result.public_status, "not_claimed")
        self.assertFalse(result.diagnostic_ablation)
        self.assertEqual(
            result.decision,
            DifferentialDecision.OPEN_BOUNDED_EVIDENCE_ONLY,
        )
        self.assertTrue(result.bounded_equivalent)
        self.assertIsNone(result.first_failure)
        self.assertTrue(result.facet_partition_equivalent)
        self.assertTrue(result.root_genealogy_and_covered_points_equivalent)

    def test_short_deterministic_campaign_classifies_every_scientific_outcome(
        self,
    ) -> None:
        for point_count in range(5, 9):
            for seed in (0, 2):
                points = generate_affine_cloud(
                    3,
                    point_count,
                    seed,
                    coordinate_bound=19,
                )
                with self.subTest(point_count=point_count, seed=seed):
                    self._assert_unablated_bounded_evidence(points, 2)

    def test_short_order_three_campaign_and_order_ten_boundary(self) -> None:
        for point_count in range(5, 9):
            points = generate_affine_cloud(
                3,
                point_count,
                0,
                coordinate_bound=19,
            )
            with self.subTest(order=3, point_count=point_count):
                self._assert_unablated_bounded_evidence(points, 3)

        boundary_points = generate_affine_cloud(
            3,
            11,
            0,
            coordinate_bound=31,
        )
        with self.subTest(order=10, point_count=11):
            self._assert_unablated_bounded_evidence(boundary_points, 10)


if __name__ == "__main__":
    unittest.main()
