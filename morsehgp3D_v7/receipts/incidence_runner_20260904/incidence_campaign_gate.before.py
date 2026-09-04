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

    def test_main_retains_failed_and_interrupted_attempts(self) -> None:
        for mode in ("crash", "malformed", "interrupted", "refused", "censored", "completed"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory(prefix="mhgp7-incidence-main-") as directory:
                output = Path(directory) / "receipt"

                def run(_command, out, err, **_kwargs):
                    out.write_text(completed_text() if mode == "completed" else "partial" if mode == "malformed" else "")
                    err.write_text(REFUSAL if mode == "refused" else "")
                    out.with_suffix(".time").write_text(USAGE)
                    if mode == "interrupted":
                        raise KeyboardInterrupt("fixture interruption")
                    return (-11 if mode == "crash" else 2 if mode == "refused" else -9 if mode == "censored" else 0,
                            mode == "censored", 1.0)

                argv = ["incidence_campaign.py", "--binary", sys.executable, "--output", str(output),
                        "--sizes", "11", "--families", "uniform", "--coords", "65536", "--threads", "2"]
                with patch.object(sys, "argv", argv), patch.object(campaign, "run_process", side_effect=run), \
                        patch.object(campaign, "source_snapshot", return_value={}), \
                        patch.object(campaign.subprocess, "check_output", return_value="fixture-head\n"), \
                        contextlib.redirect_stdout(io.StringIO()):
                    if mode in ("crash", "malformed", "interrupted"):
                        with self.assertRaises(RuntimeError):
                            campaign.main()
                    else:
                        campaign.main()
                records = json.loads((output / "runs.json").read_text())
                summary = json.loads((output / "summary.json").read_text())
                self.assertEqual(len(records), 1)
                self.assertEqual(summary["runs"], 1)
                self.assertEqual(summary["status"], "invalid" if mode in ("crash", "malformed", "interrupted") else "observations_completed")
                if mode in ("crash", "malformed", "interrupted"):
                    self.assertEqual(records[0]["status"], "invalid")
                self.assertTrue((output / "hashes.json").is_file())


if __name__ == "__main__":
    unittest.main()
