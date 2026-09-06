"""Refute false-positive and lost-attempt observation receipts; no engine claim."""

from __future__ import annotations

import contextlib
import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "bench"))
import incidence_campaign as campaign  # noqa: E402


IDENTITY = {"family": "uniform", "n": 11, "coord": 65536, "seed": 3,
            "threads": 2, "meb_supports": 25000000, "wall_seconds": 1.0}
USAGE = "Maximum resident set size (kbytes): 1000\nExit status: 0\n"
REFUSAL = (
    "REFUS unsupported_degeneracy : rank-relevant extra-shell\n"
    "refus_etage=census rss_mb apres_generation=1 apres_rle=2 apres_prefiltre=3 "
    "apres_census=4 max_fold=0 (frontiere de completion : dernier etage atteint)\n"
    "silent_refusal_work census_balls=2 plateau_balls=1 total_ms=1.000 completion_ms=0.000\n"
)


def silent_refusal(reason: str = "silent_meb_support_budget", k: int = 2) -> str:
    # Shape and counts taken from the real n=11/coord=65536/seed=3 CLI
    # refusal with --silent-meb-supports=1, after the K1 callback stage.
    return (
        f"REFUS silent incidence K={k} : {reason}\n"
        "refus_etage=fold rss_mb apres_generation=4 apres_rle=4 apres_prefiltre=4 "
        "apres_census=4 max_fold=5 (frontiere de completion : dernier etage atteint)\n"
        "silent_refusal_work census_balls=172 plateau_balls=0 total_ms=7.197 completion_ms=0.050\n"
        f"silent_refusal_K{k} core=24 steps=0 added_provisional=0 query_nodes=5 meb_supports=1\n"
    )


def completed_text() -> str:
    lines = [
        "payload=mhgp7-forests-horizontal-v1 authority=status_terminal callbacks=provisional vertical_maps=none",
        "backend=cpu_reference",
        "forest_semantics=normalized_horizontal_h0_candidate public_status=not_claimed require_exact=false",
        "tower_scope=profile_complete_k10 smax_requested=11 smax_effective=11",
        "forest_layout=csr forest_storage_kind=csr_facet_keys_v1 csr_fallback=0 ordres_publies=10 ordres_storage_conformes=10",
        "famille=uniform n=11 coord=65536 s=8 smax=11 seed=3 threads=2 emis=20 boules_uniques=19 "
        "mortes_profondeur=0 survivantes=19 census_int=1 census_shell=2 evenements=19 facettes=74 "
        "fusions=10 deltas=19 noeuds=10",
        "silent_limits core_records=8000000 chain_steps=2000000 cofaces=2000000 query_nodes=1000000000 meb_supports=25000000",
        "memory_budget_scope=partial_named_payload_proxy_v1 budget=17179869184 cap_brut_demande=100 cap_brut_effectif=90 cap_fusion_budgetaire=90",
        "temps_mur_ms=1.000",
        "cardinalites K=1 evenements=10 facettes=11 deltas=10 attachements=0 fusions=10 noeuds=10",
    ]
    for k in range(2, 11):
        lines.append(f"cardinalites K={k} evenements=1 facettes={k + 1} deltas=1 attachements=0 fusions=0 noeuds=0")
        lines.append(f"silent_K{k} core=1 with_two_intruders=0 steps=0 added=0 max_chain=0 query_nodes=1 meb_supports=1")
    digests = [hashlib.sha256(f"fixture-{k}".encode()).hexdigest() for k in range(1, 11)]
    for k, digest in enumerate(digests, 1):
        lines.append(f"digest_forest_K{k}={digest}")
    total = hashlib.sha256(b"mhgp4-digest-v1:all" + "".join(digests).encode()).hexdigest()
    lines.append(f"digest_all={total}")
    return "\n".join(lines) + "\n"


class IncidenceCampaignGate(unittest.TestCase):
    def classify_refusal(self, text: str, *, code: int = 2, stdout: str = "", timed_out: bool = False) -> dict:
        with tempfile.TemporaryDirectory(prefix="mhgp7-silent-refusal-") as directory:
            out, err, usage = (Path(directory) / name for name in ("out", "err", "time"))
            out.write_text(stdout)
            err.write_text(text)
            usage.write_text(USAGE.replace("Exit status: 0", f"Exit status: {code}"))
            record = {"returncode": code, "timed_out": timed_out, "wall_seconds": 1.0, "status": "invalid"}
            try:
                campaign.classify(record, out, err, usage,
                                  **{key: value for key, value in IDENTITY.items() if key != "wall_seconds"})
            except (RuntimeError, ValueError):
                self.assertEqual(record["status"], "invalid")
                raise
            return record

    def test_positive_is_only_engine_completion(self) -> None:
        result = campaign.parse_completion(completed_text(), "", USAGE, **IDENTITY)
        self.assertEqual(result["counts"]["coord"], 65536)
        self.assertEqual(set(result["silent"]), set(range(2, 11)))
        summary = campaign.summarize([{"status": "engine_completed"}], 1, None, True)
        self.assertEqual(summary["status"], "observations_completed")
        self.assertEqual(summary["public_status"], "not_claimed")

    def test_identity_and_payload_corruptions(self) -> None:
        text = completed_text()
        changes = [
            ("famille=uniform", "famille=terrain"), ("n=11 coord=", "n=12 coord="),
            ("coord=65536", "coord=200"), ("threads=2", "threads=3"),
            ("backend=cpu_reference", "backend=cuda"), ("public_status=not_claimed", "public_status=exact"),
            ("ordres_publies=10", "ordres_publies=9"), ("facettes=74", "facettes=75"),
            ("budget=17179869184", "budget=10"), ("meb_supports=25000000", "meb_supports=2"),
            ("silent_K2 ", "silent_K3 "), ("cardinalites K=2 ", "cardinalites K=3 "),
            ("digest_forest_K10=", "missing_digest_K10="),
        ]
        for before, after in changes:
            with self.subTest(change=before), self.assertRaises(RuntimeError):
                campaign.parse_completion(text.replace(before, after), "", USAGE, **IDENTITY)
        with self.assertRaises(RuntimeError):
            campaign.parse_completion(text, "unexpected stderr", USAGE, **IDENTITY)
        with self.assertRaises(RuntimeError):
            campaign.parse_completion(text, "", USAGE.replace("Exit status: 0", "Exit status: 1"), **IDENTITY)

    def test_partial_or_invalid_summary_never_completed(self) -> None:
        for records, expected, failure, stable in [
            ([{"status": "invalid"}], 1, None, True),
            ([{"status": "engine_completed"}], 2, None, True),
            ([{"status": "engine_completed"}], 1, "changed", True),
            ([{"status": "engine_completed"}], 1, None, False),
        ]:
            self.assertEqual(campaign.summarize(records, expected, failure, stable)["status"], "invalid")
        mixed = campaign.summarize([{"status": "engine_refused"}, {"status": "censored"}], 2, None, True)
        self.assertEqual((mixed["status"], mixed["engine_successes"], mixed["engine_refusals"], mixed["censored"]),
                         ("observations_completed", 0, 1, 1))

    def test_refusal_and_censor_are_not_payloads(self) -> None:
        with tempfile.TemporaryDirectory(prefix="mhgp7-incidence-gate-") as directory:
            out, err, usage = (Path(directory) / name for name in ("out", "err", "time"))
            out.write_text("")
            err.write_text(REFUSAL)
            record = {"returncode": 2, "timed_out": False, "wall_seconds": 1.0, "status": "invalid"}
            campaign.classify(record, out, err, usage)
            self.assertEqual(record["status"], "engine_refused")
            for stderr in ("", REFUSAL + "digest_all=unexpected\n", REFUSAL.replace("unsupported_degeneracy", "invariant_violated")):
                err.write_text(stderr)
                record["status"] = "invalid"
                with self.assertRaises(RuntimeError):
                    campaign.classify(record, out, err, usage)
                self.assertEqual(record["status"], "invalid")
            err.write_text(REFUSAL)
            out.write_text("partial payload\n")
            with self.assertRaises(RuntimeError):
                campaign.classify(record, out, err, usage)
            record.update(returncode=-9, timed_out=True, status="invalid")
            campaign.classify(record, out, err, usage)
            self.assertEqual(record["status"], "censored")

    def test_closed_silent_refusal_vocabulary(self) -> None:
        resource = (
            "silent_core_record_budget", "silent_chain_step_budget", "silent_added_coface_budget",
            "silent_query_node_budget", "silent_meb_support_budget", "silent_direct_catalogue_budget",
            "silent_allocation_failure",
        )
        unsupported = ("silent_local_nonessential_shell", "silent_external_shell", "silent_nonregular_direct_catalogue")
        self.assertEqual(campaign.SILENT_RESOURCE_REASONS, set(resource))
        self.assertEqual(campaign.SILENT_UNSUPPORTED_REASONS, set(unsupported))
        for reasons, expected in ((resource, "resource_exhausted"), (unsupported, "unsupported_degeneracy")):
            for reason in reasons:
                for k in (2, 10):
                    with self.subTest(reason=reason, k=k):
                        record = self.classify_refusal(silent_refusal(reason, k))
                        self.assertEqual((record["status"], record["refusal_status"], record["refusal_order"]),
                                         ("engine_refused", expected, k))
                        summary = campaign.summarize([record], 1, None, True)
                        self.assertEqual((summary["engine_successes"], summary["engine_refusals"], summary["censored"]),
                                         (0, 1, 0))
        # Early catalogue refusal may have no per-order work counters at all.
        early = "\n".join(silent_refusal("silent_direct_catalogue_budget").splitlines()[:3]) + "\n"
        self.assertEqual(self.classify_refusal(early)["status"], "engine_refused")
        self.assertEqual(self.classify_refusal(silent_refusal(), code=124, timed_out=True)["status"], "censored")

    def test_invariant_unknown_and_malformed_silent_refusals_stay_invalid(self) -> None:
        bad_reasons = (
            "silent_unknown_point_id", "silent_invalid_index", "silent_invalid_order", "silent_duplicate_direct_coface",
            "silent_no_local_miniball", "silent_descent_not_strict", "silent_terminal_level_mismatch",
            "silent_terminal_missing_from_catalogue", "silent_support_not_in_coface",
            "silent_unknown_budget", "silent_meb_support_budget_extra", "silent_external_shell_extra",
            "silent_", "silent_meb_support_budget garbage", "silent_meb_support_budget\x00", "",
        )
        malformed = [silent_refusal(reason) for reason in bad_reasons]
        malformed.extend(silent_refusal(k=k) for k in (-1, 0, 1, 11))
        valid = silent_refusal()
        malformed.extend(valid.replace(before, after, 1) for before, after in (
            ("K=2", "K=02"), ("K=2 :", "K=2:"), ("REFUS ", " REFUS "),
            ("silent incidence ", "silent incidence fold capacity "),
            ("refus_etage=fold", "refus_etage=census"), ("refus_etage=fold", "refus_etage=unknown"),
            ("silent_refusal_K2", "silent_refusal_K3"), ("core=24", "core=-24"),
            ("total_ms=7.197", "total_ms=nan"),
        ))
        diagnostic = valid.splitlines()[-1] + "\n"
        malformed.extend((valid + diagnostic, silent_refusal(k=10) + diagnostic,
                          valid + "digest_all=unexpected\n", valid + "garbled\n",
                          "\n".join(valid.splitlines()[:2]) + "\n"))
        for text in malformed:
            with self.subTest(text=text), self.assertRaises(RuntimeError):
                self.classify_refusal(text)
        with self.assertRaises(RuntimeError):
            self.classify_refusal(valid, stdout="partial payload\n")
        with self.assertRaises(RuntimeError):
            self.classify_refusal(valid, code=0)
        with self.assertRaises(ValueError):
            self.classify_refusal(valid, code=3)

    def test_main_retains_failed_and_interrupted_attempts(self) -> None:
        invalid_modes = ("crash", "malformed", "interrupted", "unknown_refusal")
        silent_modes = {"cap_refused": "silent_meb_support_budget", "unsupported_refused": "silent_external_shell",
                        "unknown_refusal": "silent_unknown_budget"}
        for mode in (*invalid_modes, "refused", "cap_refused", "unsupported_refused", "censored", "completed"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory(prefix="mhgp7-incidence-main-") as directory:
                output = Path(directory) / "receipt"

                def run(_command, out, err, **_kwargs):
                    out.write_text(completed_text() if mode == "completed" else "partial" if mode == "malformed" else "")
                    err.write_text(REFUSAL if mode == "refused" else
                                   silent_refusal(silent_modes[mode]) if mode in silent_modes else "")
                    out.with_suffix(".time").write_text(USAGE)
                    if mode == "interrupted":
                        raise KeyboardInterrupt("fixture interruption")
                    return (-11 if mode == "crash" else 2 if mode == "refused" or mode in silent_modes else
                            -9 if mode == "censored" else 0,
                            mode == "censored", 1.0)

                argv = ["incidence_campaign.py", "--binary", sys.executable, "--output", str(output),
                        "--sizes", "11", "--families", "uniform", "--coords", "65536", "--threads", "2"]
                with patch.object(sys, "argv", argv), patch.object(campaign, "run_process", side_effect=run), \
                        patch.object(campaign, "source_snapshot", return_value={}), \
                        patch.object(campaign.subprocess, "check_output", return_value="fixture-head\n"), \
                        contextlib.redirect_stdout(io.StringIO()):
                    if mode in invalid_modes:
                        with self.assertRaises(RuntimeError):
                            campaign.main()
                    else:
                        campaign.main()
                records = json.loads((output / "runs.json").read_text())
                summary = json.loads((output / "summary.json").read_text())
                self.assertEqual(len(records), 1)
                self.assertEqual(summary["runs"], 1)
                self.assertEqual(summary["status"], "invalid" if mode in invalid_modes else "observations_completed")
                if mode in invalid_modes:
                    self.assertEqual(records[0]["status"], "invalid")
                if mode in ("refused", "cap_refused", "unsupported_refused"):
                    self.assertEqual((summary["engine_successes"], summary["engine_refusals"]), (0, 1))
                self.assertTrue((output / "hashes.json").is_file())


if __name__ == "__main__":
    unittest.main()
