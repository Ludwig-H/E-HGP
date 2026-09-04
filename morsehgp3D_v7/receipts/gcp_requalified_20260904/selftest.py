#!/usr/bin/env python3
"""Public export gates use private-shaped temporary fixtures, never a real session."""
from __future__ import annotations

import copy
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import unittest
from unittest import mock

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError("module missing")
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


E = module("public_export", HERE / "export_public.py")
F = module("fake_g4_towers", ROOT / "gcp-migration/selftest_cpu_towers_v7.py")
SECRET = "private-account@example.invalid SECRET_NEVER_PUBLIC 10.23.45.67 /home/private-user/key.pem"
GENERATION = "2026-09-04T22:55:00.000Z"
ENDED = "2026-09-04T23:10:00+00:00"
TARGET = {"project": "fake-project", "zone": "fake-zone-a", "instance": "fake-instance"}
STAGES = (
    "temps_ms index=1.0 gen=2.0 (wspd 1.0 rects 1.0) rle=1.0 prefiltre=1.0 census=2.0 comptage=1.0 "
    "expansion=1.0 fold=2.0 (tri 1.0 intern 1.0 fusion 1.0 reduce 1.0) digest=1.0\n"
    "temps_fold_mur_ms=2.0 (etages A et B, fold_inflight=2, fold_join=0, pic_mesure_en_vol=2)\n"
    "ouvriers wspd=48 rects=48 rle=48 prefiltre=48 census=48 expansion=48 fold=48\n"
    "vcensus prefiltre_nœuds=1 prefiltre_feuilles=2 range_add=3 census_nœuds=4 census_feuilles=5\n"
    "rss_mb apres_generation=1 apres_rle=2 apres_prefiltre=3 apres_census=4 max_fold=5 fin=6 (fixture)\n"
    "residence_hwm_mb apres_generation=1 apres_rle=2 apres_prefiltre=3 apres_census=4 max_fold=5 fin=6 (fixture)\n"
)


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(E.encode(value))


def fixture(root, scenario="slow"):
    rc, _ = F.fake_worker(root, scenario)
    for path in (root / "out").glob("cpu_v*.stdout"):
        if path.read_text().startswith("payload="):
            path.write_text(path.read_text() + STAGES + SECRET + "\n")
    tools = {"packages": {"build-essential": "12.9ubuntu3", "cmake": "3.22.1-1ubuntu1.22.04.2",
                           "libboost-dev": "1.74.0.3ubuntu7", "time": "1.9-0.1build2"},
             "probe": "cplusplus=202002 boost=107400\n", "versions": {"unselected": SECRET}}
    save(root / "out/cpu_toolchain.json", tools)
    gpu_stdout = "\n".join(f"{i}/12 Test #{i}: mhgp7_device_witness_fixture{i} .......... Passed 0.10 sec" for i in range(1, 13))
    (root / "out/gpu_primitives.stdout").write_text(gpu_stdout + "\n100% tests passed, 0 tests failed out of 12\n")
    binary = json.loads((root / "out/cpu_binaries.json").read_text())
    for item in binary.values():
        item["path"] = SECRET
    save(root / "out/cpu_binaries.json", binary)
    save(root / "out/never-selected.json", {"secret": SECRET})
    result_sha = F.C.seal(root / "out")
    (root / "received").mkdir()
    shutil.move(root / "out", root / "received/out")
    source = {"schema": E.SCHEMA, "head": "1" * 40, "public_status": "not_claimed", "worktree_status": SECRET,
              "files": [{"path": "gcp-migration/v7_g4_session.py", "sha256": E.CONTROLLER_SHA, "size": 123, "mode": 0o444},
                        {"path": "morsehgp3D_v7/src/example.hpp", "sha256": "2" * 64, "size": 456, "mode": 0o444}]}
    save(root / "source/source_manifest.json", source)
    source_sha = E.digest((root / "source/source_manifest.json").read_bytes())
    save(root / "prepared.json", {"schema": E.SCHEMA, "manifest_sha256": source_sha, "archive_sha256": "3" * 64,
                                   "head": "1" * 40, "created": GENERATION, "private_extra": SECRET})
    state = {"schema": E.SCHEMA, "target": TARGET, "generation": GENERATION, "started": GENERATION, "ended": ENDED,
             "stopped_verified": True, "status": "completed" if rc == 0 else "failed", "exit_code": rc,
             "remote_exit_code": rc, "source_manifest_sha256": source_sha, "result_manifest_sha256": result_sha,
             "private_extra": SECRET}
    save(root / "session.json", state)
    stop = {"name": TARGET["instance"], "zone": "projects/fake/zones/" + TARGET["zone"],
            "selfLink": f"https://example.invalid/compute/v1/projects/{TARGET['project']}/zones/{TARGET['zone']}/instances/{TARGET['instance']}",
            "id": "123456789", "status": "TERMINATED", "lastStartTimestamp": GENERATION,
            "machineType": "zones/fake/machineTypes/g4-standard-48", "labels": {"project": "e-hgp"},
            "scheduling": {"provisioningModel": "SPOT", "instanceTerminationAction": "STOP", "maxRunDuration": {"seconds": "3600"}},
            "networkInterfaces": [{"private": SECRET}], "serviceAccounts": [{"email": SECRET}]}
    save(root / "control/post_stop.stdout", stop)
    save(root / "control/post_stop.json", {"exit_code": 0, "ended": ENDED, "argv": [SECRET]})
    save(root / "control/stop.json", {"exit_code": 0, "ended": ENDED, "stop_and_check_seconds": 5.0, "argv": [SECRET]})
    marker = dict(TARGET, schema="e-hgp.guard-mark.v1", mark="double_guard_verified", generation=GENERATION,
                  max_run_seconds="3600", guest_shutdown_minutes="45", private_extra=SECRET)
    (root / "guard-marks").mkdir()
    (root / "guard-marks/double_guard_verified").write_text("".join(f"{key}={value}\n" for key, value in marker.items()))
    save(root / "control/start.json", {"must_never_read": SECRET})
    return state


class ExportTests(unittest.TestCase):
    def test_whitelist_receipt_stage_metrics_and_create_only_publication(self):
        with tempfile.TemporaryDirectory() as temp:
            session = Path(temp) / "session"
            session.mkdir()
            fixture(session)
            reads = []
            original = E.SelectedReader.raw
            def tracked(reader, relative, expected=None):
                reads.append(relative)
                return original(reader, relative, expected)
            with mock.patch.object(E.SelectedReader, "raw", tracked):
                values = E.build_public(session)
            raw = E.encode(values)
            for secret in SECRET.split():
                self.assertNotIn(secret.encode(), raw)
            self.assertNotIn("control/start.json", reads)
            self.assertNotIn("received/out/never-selected.json", reads)
            self.assertNotIn("source.tgz", reads)
            receipt = values["receipt.json"]
            self.assertEqual(receipt["stop"]["target"], dict(TARGET, id="123456789"))
            self.assertEqual(len(receipt["results"]["cpu"]["observations"]), 6)
            self.assertEqual(receipt["results"]["cpu"]["observations"][0]["counts"]["n"], 50000)
            self.assertIn("digest_cumulative", receipt["results"]["cpu"]["observations"][0]["stages"]["stage_times_ms"])
            destination = Path(temp) / "published"
            self.assertEqual(E.publish(values, destination)["status"], "published")
            self.assertEqual({path.name for path in destination.iterdir()}, {"receipt.json", "source_pins.json", "manifest.json"})
            with self.assertRaises(ValueError):
                E.publish(values, destination)

    def test_censure_is_retained_and_partial_validation_withholds_objects(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            fixture(root, "censored")
            receipt = E.build_public(root)["receipt.json"]
            self.assertEqual(receipt["controller_status"], "failed")
            self.assertEqual(receipt["controller_exit_code"], 1)
            self.assertEqual(receipt["results"]["cpu"]["pairs"]["uniform_k10"], {"status": "not_comparable"})
            censored = [row for row in receipt["results"]["cpu"]["observations"] if row["status"] == "censored"]
            self.assertEqual(len(censored), 1)
            self.assertNotIn("digests", censored[0])
            terminal_path = root / "received/out/worker_terminal.json"
            terminal = json.loads(terminal_path.read_text())
            terminal["diagnostics_completed"] = False
            save(terminal_path, terminal)
            state = json.loads((root / "session.json").read_text())
            state["result_manifest_sha256"] = F.C.seal(root / "received/out")
            save(root / "session.json", state)
            partial = E.build_public(root)["receipt.json"]["results"]
            self.assertEqual(partial["semantic_validation"], "raw_manifest_and_terminal_only")
            self.assertTrue(all("digests" not in row for row in partial["cpu"]["observations"]))

    def test_completed_candidate_costs_and_gpu_nonvacuity(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            fixture(root)
            fixture_module = module("incidence_export_fixture", ROOT / "morsehgp3D_v7/tests/incidence_campaign_gate.py")
            text = fixture_module.completed_text().replace("n=11 coord=", "n=8000 coord=").replace(
                "facettes=74 ", "facettes=8063 ").replace("K=1 evenements=10 facettes=11 ", "K=1 evenements=10 facettes=8000 ").replace(
                "threads=2", "threads=48").replace("meb_supports=25000000", "meb_supports=1000000000")
            parsed = F.C.incidence_module(ROOT).parse_completion(text, "", fixture_module.USAGE, family="uniform",
                n=8000, coord=65536, seed=3, threads=48, meb_supports=1000000000, wall_seconds=2.0)
            save(root / "received/out/candidate_wide_status.json", dict(parsed, status="engine_completed", exit_code=0,
                public_status="not_claimed", mathematical_refusal=False))
            (root / "received/out/candidate_wide.stdout").write_text(text + STAGES)
            state = json.loads((root / "session.json").read_text())
            state["result_manifest_sha256"] = F.C.seal(root / "received/out")
            save(root / "session.json", state)
            public = E.build_public(root)["receipt.json"]["results"]
            candidate = public["candidate_wide"]
            self.assertEqual(candidate["semantics"], "normalized_horizontal_h0_candidate")
            self.assertEqual(candidate["counts"]["n"], 8000)
            self.assertEqual(len(candidate["silent"]), 9)
            self.assertEqual(len(candidate["digests"]), 11)
            self.assertEqual(public["gpu"]["ctest"]["tests"], 12)
            raw = (root / "received/out/gpu_primitives.stdout").read_text()
            for invalid in (raw.replace("out of 12", "out of 0"), raw.replace("Passed", "Failed", 1),
                            raw.replace("fixture12", "fixture11")):
                with self.assertRaises(ValueError): E.gpu_test_projection(invalid)

    def test_terminal_target_marker_source_and_result_corruptions(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            fixture(root)
            originals = {relative: (root / relative).read_bytes() for relative in
                         ("session.json", "control/post_stop.stdout", "guard-marks/double_guard_verified",
                          "source/source_manifest.json", "received/out/cpu_campaign.json")}
            for label in ("running", "unstopped", "different_generation", "different_target", "nonspot", "marker", "source", "result"):
                relative = "session.json"
                data = json.loads(originals[relative])
                if label == "running": data["status"] = "running"
                elif label == "unstopped": data["stopped_verified"] = False
                elif label in ("different_generation", "different_target", "nonspot"):
                    relative = "control/post_stop.stdout"
                    data = json.loads(originals[relative])
                    if label == "different_generation": data["lastStartTimestamp"] = "2026-09-04T01:00:00Z"
                    elif label == "different_target": data["name"] = "other-instance"
                    else: data["scheduling"]["provisioningModel"] = "STANDARD"
                elif label == "marker":
                    relative = "guard-marks/double_guard_verified"
                    (root / relative).write_bytes(originals[relative].replace(b"guest_shutdown_minutes=45", b"guest_shutdown_minutes=99"))
                elif label == "source":
                    relative = "source/source_manifest.json"
                    data = json.loads(originals[relative])
                    data["head"] = "f" * 40
                elif label == "result":
                    relative = "received/out/cpu_campaign.json"
                    data = json.loads(originals[relative])
                    data["status"] = "changed"
                if label != "marker": save(root / relative, data)
                with self.subTest(label=label), self.assertRaises(ValueError):
                    E.build_public(root)
                (root / relative).write_bytes(originals[relative])

    def test_stage_mutants_and_duplicate_json_or_symlink(self):
        self.assertEqual(E.stage_projection(STAGES)["workers"]["fold"], 48)
        for invalid in (STAGES + STAGES, STAGES.replace("gen=2.0", "gen=nan"), STAGES.replace("digest=1.0", "digest=" + SECRET)):
            with self.assertRaises(ValueError): E.stage_projection(invalid)
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / "duplicate.json").write_text('{"status": 0, "status": 1}')
            with self.assertRaises(ValueError): E.SelectedReader(root).read("duplicate.json")
            (root / "link.json").symlink_to(root / "duplicate.json")
            with self.assertRaises(ValueError): E.SelectedReader(root).read("link.json")


if __name__ == "__main__":
    unittest.main()
