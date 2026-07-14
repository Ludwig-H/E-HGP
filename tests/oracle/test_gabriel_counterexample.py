from __future__ import annotations

import json
import unittest
from fractions import Fraction
from pathlib import Path

from reference.morsehgp3d_oracle.catalog import build_critical_catalog
from reference.morsehgp3d_oracle.exact import affine_dimension
from reference.morsehgp3d_oracle.gamma import build_gamma_filtration
from reference.morsehgp3d_oracle.oracle import run_oracle


FIXTURE_ROOT = Path(__file__).parents[1] / "fixtures"
REPOSITORY_ROOT = Path(__file__).parents[2]
CURATED_FIXTURE = (
    FIXTURE_ROOT / "regressions" / "gabriel_point_set_counterexample.json"
)


def _load(path: Path) -> dict[str, object]:
    with path.open(encoding="utf-8") as fixture_file:
        return json.load(fixture_file)


def _fraction(raw_value: object) -> Fraction:
    if isinstance(raw_value, dict):
        return Fraction(raw_value["numerator"], raw_value["denominator"])
    return Fraction(raw_value)


def _component_records(cut) -> list[dict[str, object]]:
    return [
        {
            "facet_point_ids": [list(facet) for facet in component.facet_point_ids],
            "covered_point_ids": list(component.covered_point_ids),
        }
        for component in cut.components
    ]


class GabrielPointSetCounterexampleTests(unittest.TestCase):
    def setUp(self) -> None:
        self.fixture = _load(CURATED_FIXTURE)
        self.points = self.fixture["points"]
        self.counterexample = self.fixture["counterexample"]
        self.witness = self.fixture["lost_incidence_witness"]
        assert isinstance(self.points, list)
        assert isinstance(self.counterexample, dict)
        assert isinstance(self.witness, dict)

    def test_pruned_gabriel_point_sets_differ_from_gamma(self) -> None:
        source = self.fixture["source_failure_fixture"]
        domain = self.fixture["domain_checks"]
        assert isinstance(source, dict)
        assert isinstance(domain, dict)
        source_fixture = _load(REPOSITORY_ROOT / source["path"])
        self.assertEqual(source_fixture["fixture_id"], source["fixture_id"])
        self.assertEqual(source_fixture["minimized_points"], self.points)

        self.assertEqual(affine_dimension(self.points), domain["affine_dimension"])
        catalog = build_critical_catalog(self.points, domain["k_max"])
        self.assertIs(catalog.relevant_gp_complete, domain["relevant_gp_complete"])
        self.assertEqual(
            len(catalog.degenerate_candidates), domain["degenerate_candidate_count"]
        )
        self.assertEqual(len(catalog.events), domain["critical_event_count"])

        filtration = build_gamma_filtration(
            self.points, self.counterexample["order"]
        )
        level = _fraction(self.counterexample["squared_level"])
        gamma_cut = filtration.cut(level, graph_kind="gamma")
        gabriel_cut = filtration.cut(level, graph_kind="gabriel")

        self.assertEqual(
            _component_records(gamma_cut), self.counterexample["gamma_components"]
        )
        self.assertEqual(
            _component_records(gabriel_cut), self.counterexample["gabriel_components"]
        )
        gamma_point_sets = {
            component.covered_point_ids for component in gamma_cut.components
        }
        gabriel_point_sets = {
            component.covered_point_ids for component in gabriel_cut.components
        }
        self.assertNotEqual(gamma_point_sets, gabriel_point_sets)
        self.assertIs(
            gamma_point_sets == gabriel_point_sets,
            self.counterexample["point_set_collections_equal"],
        )

    def test_silent_incidence_is_needed_by_a_later_gabriel_coface(self) -> None:
        filtration = build_gamma_filtration(
            self.points, self.counterexample["order"]
        )
        earlier_level = _fraction(self.witness["earlier_squared_level"])
        later_level = _fraction(self.witness["later_gabriel_squared_level"])
        delayed_level = _fraction(
            self.witness["first_delayed_gabriel_connection_level"]
        )
        lost_facet = tuple(self.witness["facet_point_ids"])
        earlier_simplices = {
            tuple(simplex) for simplex in self.witness["earlier_non_gabriel_cofaces"]
        }
        later_simplex = tuple(self.witness["later_gabriel_coface"])

        cofaces = {coface.point_ids: coface for coface in filtration.cofaces}
        hyperedges = {
            edge.simplex_point_ids: edge for edge in filtration.gabriel_hyperedges
        }
        self.assertEqual(
            {cofaces[simplex].squared_level for simplex in earlier_simplices},
            {earlier_level},
        )
        self.assertTrue(earlier_simplices.isdisjoint(hyperedges))
        self.assertEqual(hyperedges[later_simplex].squared_level, later_level)
        self.assertIn(lost_facet, hyperedges[later_simplex].facet_point_ids)

        gamma_open = filtration.cut(later_level, closed=False, graph_kind="gamma")
        gabriel_open = filtration.cut(
            later_level, closed=False, graph_kind="gabriel"
        )
        gamma_container = gamma_open.component_by_facet[lost_facet]
        self.assertEqual(gamma_container.covered_point_ids, (0, 2, 3, 4))
        self.assertNotIn(lost_facet, gabriel_open.component_by_facet)

        delayed_open = filtration.cut(
            delayed_level, closed=False, graph_kind="gabriel"
        )
        delayed_closed = filtration.cut(
            delayed_level, closed=True, graph_kind="gabriel"
        )
        self.assertEqual(len(delayed_open.components), 2)
        self.assertEqual(len(delayed_closed.components), 1)
        self.assertEqual(
            delayed_closed.components[0].covered_point_ids, (0, 1, 2, 3, 4)
        )

    def test_integrated_oracle_rejects_the_false_reduced_contract(self) -> None:
        full_result = run_oracle(self.points, 5, profile="full_pi0")
        self.assertEqual(full_result.k_eff, 5)
        self.assertEqual(len(full_result.vertical_maps), 4)

        with self.assertRaisesRegex(
            ValueError,
            "profile component and its Gamma container cover different points",
        ):
            run_oracle(self.points, 5, profile="hgp_reduced")


if __name__ == "__main__":
    unittest.main()
