from __future__ import annotations

import copy
import unittest
from pathlib import Path

from tools.check_phase15_pair_rank_k2_benchmark import (
    ContractError,
    check_raw,
    check_summary,
    load_json,
)


ROOT = Path(__file__).resolve().parents[2]
VALIDATION = ROOT / "docs" / "validation"
SUMMARY_PATH = (
    VALIDATION / "phase15_pair_rank_k2_vs_k10_g4_17f7b04.json"
)
K2_PATH = VALIDATION / "phase15_pair_rank_n12500_k2_q3_g4_17f7b04.json"
K10_PATH = VALIDATION / "phase15_pair_rank_n12500_k10_q3_g4_17f7b04.json"
MEM_JSON_PATH = (
    VALIDATION / "phase15_pair_rank_n4096_k2_memcheck_g4_17f7b04.json"
)
MEMCHECK_PATH = (
    VALIDATION / "phase15_pair_rank_n4096_k2_memcheck_g4_17f7b04.txt"
)


class Phase15PairRankK2BenchmarkCheckerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.summary = load_json(SUMMARY_PATH)
        cls.k2 = load_json(K2_PATH)
        cls.k10 = load_json(K10_PATH)

    def check_summary(self, summary: dict[str, object]) -> None:
        check_summary(
            summary,
            self.k2,
            self.k10,
            K2_PATH,
            K10_PATH,
            MEM_JSON_PATH,
            MEMCHECK_PATH,
        )

    def test_sealed_artifacts_pass(self) -> None:
        check_raw(self.k2, 2)
        check_raw(self.k10, 10)
        self.check_summary(self.summary)

    def test_changed_main_budget_fails_closed(self) -> None:
        changed = copy.deepcopy(self.k2)
        changed["budgets"]["maximum_closed_ball_node_visit_count"] = 8_388_608
        with self.assertRaisesRegex(ContractError, "wrong budgets"):
            check_raw(changed, 2)

    def test_unexhausted_frontier_fails_closed(self) -> None:
        changed = copy.deepcopy(self.k2)
        changed["assisted_first"]["gpu_audit"][
            "every_device_frontier_exhausted"
        ] = False
        with self.assertRaisesRegex(
            ContractError, "every_device_frontier_exhausted"
        ):
            check_raw(changed, 2)

    def test_summary_measurement_drift_fails_closed(self) -> None:
        changed = copy.deepcopy(self.summary)
        changed["measurements"]["k2"]["callback_count"] += 1
        with self.assertRaisesRegex(ContractError, "K=2 summary drift"):
            self.check_summary(changed)

    def test_unterminated_spot_session_fails_closed(self) -> None:
        changed = copy.deepcopy(self.summary)
        changed["gcp_session"]["final_status"] = "RUNNING"
        with self.assertRaisesRegex(ContractError, "GCP final status"):
            self.check_summary(changed)


if __name__ == "__main__":
    unittest.main()
