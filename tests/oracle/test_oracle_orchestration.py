from __future__ import annotations

import unittest
from fractions import Fraction

from reference.morsehgp3d_oracle.oracle import (
    OracleOrderResult,
    UnsupportedDegeneracyError,
    canonicalize_points,
    run_oracle,
)


class RecordingEngine:
    def __init__(self) -> None:
        self.catalog_calls: list[tuple[object, ...]] = []
        self.order_calls: list[int] = []
        self.vertical_calls: list[tuple[int, int]] = []

    def build_catalog(self, points, k_eff):
        self.catalog_calls.append((points, k_eff))
        return ("event",)

    def build_order(self, points, order, profile, catalog):
        self.order_calls.append(order)
        level = Fraction(order)
        return OracleOrderResult(
            order=order,
            filtration=("filtration", order),
            critical_levels=(level,),
            cuts=(("cut", order, level),),
            forest=("forest", profile, order),
            hyperedges=(("edge", order),),
            attachments=(("attachment", order),),
            equal_level_batches=(("batch", order),),
            coverage_log=(("coverage", order),),
        )

    def build_vertical_map(self, source, target, profile):
        self.vertical_calls.append((source.order, target.order))
        return ("vertical", profile, source.order, target.order)


class OracleOrchestrationTests(unittest.TestCase):
    def test_all_effective_orders_and_vertical_maps_are_built(self) -> None:
        engine = RecordingEngine()
        result = run_oracle(
            [(2, 0, 0), (0, 0, 0), (1, 1, 0)],
            10,
            profile="hgp_reduced",
            engine=engine,
        )

        self.assertEqual(result.k_eff, 3)
        self.assertEqual(result.s_max, 3)
        self.assertEqual(engine.order_calls, [1, 2, 3])
        self.assertEqual(engine.vertical_calls, [(2, 1), (3, 2)])
        self.assertEqual(tuple(point.point_id for point in result.input_points), (0, 1, 2))
        self.assertEqual(result.points[0], (Fraction(0), Fraction(0), Fraction(0)))
        self.assertEqual(result.gabriel_hyperedges, (("edge", 1), ("edge", 2), ("edge", 3)))
        self.assertEqual(len(result.forests), result.k_eff)
        self.assertEqual(len(result.vertical_maps), result.k_eff - 1)

    def test_canonicalization_is_permutation_independent(self) -> None:
        points = [(2, 0, -1), (-1, 3, 0), (0, 0, 0)]
        forward = canonicalize_points(points)
        reverse = canonicalize_points(reversed(points))
        self.assertEqual(
            tuple(point.coordinates for point in forward),
            tuple(point.coordinates for point in reverse),
        )

    def test_invalid_inputs_fail_before_engine_calls(self) -> None:
        engine = RecordingEngine()
        bad_cases = (
            ([], 1),
            ([(0, 0, 0), (0, 0, 0)], 1),
            ([(0, 0)], 1),
            ([(0, 0, float("nan"))], 1),
            ([(0, 0, 0)], 0),
            ([(0, 0, 0)], 11),
        )
        for points, k_max in bad_cases:
            with self.subTest(points=points, k_max=k_max):
                with self.assertRaises((TypeError, ValueError)):
                    run_oracle(points, k_max, engine=engine)
        self.assertFalse(engine.catalog_calls)

    def test_default_engine_fails_closed_on_exact_shell_degeneracy(self) -> None:
        with self.assertRaises(UnsupportedDegeneracyError):
            run_oracle([(0, 0, 0), (2, 0, 0), (0, 2, 0)], 2)


if __name__ == "__main__":
    unittest.main()
