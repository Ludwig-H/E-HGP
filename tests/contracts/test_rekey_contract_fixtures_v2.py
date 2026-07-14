from __future__ import annotations

import io
import sys
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import rekey_contract_fixtures_v2 as rekey  # noqa: E402


FIXTURE = ROOT / "tests" / "fixtures" / "contracts" / "k1-emst.json"


class RekeyContractFixturesV2Tests(unittest.TestCase):
    def _run_main_with_fixtures(
        self,
        paths: list[Path],
        *,
        load_side_effect,
        rekey_side_effect,
        validate_side_effect,
        write_side_effect,
    ) -> tuple[int, str, str]:
        stdout = io.StringIO()
        stderr = io.StringIO()
        with (
            patch.object(rekey, "fixture_paths", return_value=paths),
            patch.object(rekey, "load_json", side_effect=load_side_effect),
            patch.object(rekey, "rekey_fixture", side_effect=rekey_side_effect),
            patch.object(
                rekey, "validate_fixed_point", side_effect=validate_side_effect
            ),
            patch.object(rekey, "atomic_write", side_effect=write_side_effect),
            redirect_stdout(stdout),
            redirect_stderr(stderr),
        ):
            status = rekey.main(["--write"])
        return status, stdout.getvalue(), stderr.getvalue()

    def test_late_validation_failure_prevents_every_write(self) -> None:
        paths = [
            rekey.FIXTURE_DIR / "transaction-first.json",
            rekey.FIXTURE_DIR / "transaction-second.json",
        ]
        calls: list[tuple[str, str]] = []

        def load(path: Path) -> dict[str, str]:
            calls.append(("load", path.name))
            return {"fixture": path.name, "state": "source"}

        def migrate(source: dict[str, str]) -> dict[str, str]:
            calls.append(("migrate", source["fixture"]))
            return {**source, "state": "migrated"}

        def validate(_value: dict[str, str], path: Path) -> None:
            calls.append(("validate", path.name))
            if path == paths[1]:
                raise rekey.RekeyError("late semantic validation failure")

        def write(path: Path, _value: dict[str, str]) -> None:
            calls.append(("write", path.name))

        status, stdout, stderr = self._run_main_with_fixtures(
            paths,
            load_side_effect=load,
            rekey_side_effect=migrate,
            validate_side_effect=validate,
            write_side_effect=write,
        )

        self.assertEqual(status, 1)
        self.assertNotIn(("write", paths[0].name), calls)
        self.assertNotIn(("write", paths[1].name), calls)
        self.assertEqual(
            calls,
            [
                ("load", paths[0].name),
                ("migrate", paths[0].name),
                ("validate", paths[0].name),
                ("load", paths[1].name),
                ("migrate", paths[1].name),
                ("validate", paths[1].name),
            ],
        )
        self.assertEqual(stdout, "")
        self.assertIn("late semantic validation failure", stderr)
        self.assertIn("no file was written", stderr)
        self.assertNotIn("UPDATED", stderr)

    def test_publication_error_reports_completed_paths_without_false_rollback(
        self,
    ) -> None:
        paths = [
            rekey.FIXTURE_DIR / "transaction-first.json",
            rekey.FIXTURE_DIR / "transaction-second.json",
        ]
        calls: list[tuple[str, str]] = []

        def load(path: Path) -> dict[str, str]:
            calls.append(("load", path.name))
            return {"fixture": path.name, "state": "source"}

        def migrate(source: dict[str, str]) -> dict[str, str]:
            calls.append(("migrate", source["fixture"]))
            return {**source, "state": "migrated"}

        def validate(_value: dict[str, str], path: Path) -> None:
            calls.append(("validate", path.name))

        def write(path: Path, _value: dict[str, str]) -> None:
            calls.append(("write", path.name))
            if path == paths[1]:
                raise OSError("simulated disk failure")

        status, stdout, stderr = self._run_main_with_fixtures(
            paths,
            load_side_effect=load,
            rekey_side_effect=migrate,
            validate_side_effect=validate,
            write_side_effect=write,
        )

        self.assertEqual(status, 1)
        self.assertLess(
            calls.index(("validate", paths[1].name)),
            calls.index(("write", paths[0].name)),
        )
        self.assertEqual(
            calls[-2:],
            [("write", paths[0].name), ("write", paths[1].name)],
        )
        self.assertEqual(stdout, "")
        self.assertIn("1/2 changed fixture(s) completed", stderr)
        self.assertIn(
            "Published before failure: "
            "tests/fixtures/contracts/transaction-first.json.",
            stderr,
        )
        self.assertIn(
            "Failing publication target: "
            "tests/fixtures/contracts/transaction-second.json",
            stderr,
        )
        self.assertNotIn("no file was written", stderr)
        self.assertNotIn("UPDATED", stderr)

    def test_final_validation_rejects_semantically_incomplete_gamma(self) -> None:
        source = rekey.load_json(FIXTURE)
        removed = source["gamma_cofaces"].pop()
        for batch in source["equal_level_batches"]:
            batch["gamma_coface_ids"] = [
                coface_id
                for coface_id in batch["gamma_coface_ids"]
                if coface_id != removed["coface_id"]
            ]
        counters = source["run_certificate"]["work_and_memory_counters"]
        counters["gamma_cofaces_emitted"] -= 1

        migrated = rekey.rekey_fixture(source)
        with self.assertRaisesRegex(
            rekey.RekeyError, "complete Gamma order 1 is missing or inventing cofaces"
        ):
            rekey.validate_fixed_point(migrated, FIXTURE)

    def test_final_validation_rejects_every_noncanonical_rational_shape(self) -> None:
        for rational_kind in ("exact_level", "homogeneous", "positive_rational"):
            with self.subTest(rational_kind=rational_kind):
                source = rekey.load_json(FIXTURE)
                if rational_kind == "exact_level":
                    record = source["critical_catalog"][0]["squared_level_exact"]
                    record["numerator"] = str(2 * int(record["numerator"]))
                    record["denominator"] = str(2 * int(record["denominator"]))
                elif rational_kind == "homogeneous":
                    record = source["critical_catalog"][0][
                        "center_witness_homogeneous"
                    ]
                    for field in ("x_numerator", "y_numerator", "z_numerator"):
                        record[field] = str(2 * int(record[field]))
                    record["denominator"] = str(2 * int(record["denominator"]))
                else:
                    record = source["run_certificate"]["input_semantics"][
                        "similarity_transform"
                    ]["scale"]
                    record["numerator"] = str(2 * int(record["numerator"]))
                    record["denominator"] = str(2 * int(record["denominator"]))

                migrated = rekey.rekey_fixture(source)
                with self.assertRaisesRegex(
                    rekey.RekeyError, "not reduced"
                ):
                    rekey.validate_fixed_point(migrated, FIXTURE)


if __name__ == "__main__":
    unittest.main()
