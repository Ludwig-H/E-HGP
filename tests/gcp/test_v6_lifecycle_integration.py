"""Fixture d'INTEGRATION du cycle de vie v6 (audit GCP v6, troisieme tour) :
compose le VRAI `start_and_verify.sh` (via le faux gcloud du harnais de
surete, scenario guest-success), le VRAI `v6_session_lifecycle.sh` et un
FAUX `stop_and_verify.sh` compteur d'appels. Aucun contact GCP.

Prouve : le vrai garde publie l'enregistrement de cycle de vie
(`targeted_running` + generation) via --lifecycle-state-file ; le cleanup
EXTERIEUR possede l'arret nominal, publie `targeted_stopping` puis
`targeted_stopped`, appelle le faux stop EXACTEMENT une fois avec la
generation exacte, et le reçu durable grave l'etat terminal coherent avec
stop_rc=0.
"""

from __future__ import annotations

import hashlib
import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests" / "gcp"))
import test_gcp_safety  # noqa: E402  (harnais du faux gcloud, scenario guest-success)

PROTOCOL_FILES = [
    "session_campagne_v6_g4.sh",
    "v6_session_lifecycle.sh",
    "v6_campaign_pin.sh",
    "v6_campaign_remote.sh",
    "validate_v6_campaign.py",
    "profils/decision_v1.env",
    "profils/smoke_v1.env",
    "profils/g4_mesure_v1.env",
    "set_max_run_duration_and_verify.sh",
    "start_and_verify.sh",
    "stop_and_verify.sh",
]

FAKE_SET_MAX = """#!/usr/bin/env bash
echo "SETMAX $*" >> "${INTEGRATION_CALLS}"
exit 0
"""

FAKE_STOP = """#!/usr/bin/env bash
echo "STOP $*" >> "${INTEGRATION_CALLS}"
exit 0
"""


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    digest.update(path.read_bytes())
    return digest.hexdigest()


class V6LifecycleIntegrationTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tempdir = tempfile.TemporaryDirectory()
        self.addCleanup(self.tempdir.cleanup)
        self.tmp = Path(self.tempdir.name)
        harness = self.tmp / "gcloud-harnais"
        harness.write_text(test_gcp_safety.FAKE_GCLOUD, encoding="utf-8")
        harness.chmod(0o755)
        # Enveloppe : les commandes de session du cycle de vie (config privee,
        # os-login) reussissent localement ; tout le reste est DELEGUE au faux
        # gcloud du harnais de surete (scenario guest-success).
        wrapper = self.tmp / "gcloud"
        wrapper.write_text(
            "#!/usr/bin/env bash\n"
            'case "$*" in\n'
            '  "config set project"*) exit 0 ;;\n'
            '  *"os-login ssh-keys add"*) exit 0 ;;\n'
            f'  *) exec "{harness}" "$@" ;;\n'
            "esac\n",
            encoding="utf-8",
        )
        wrapper.chmod(0o755)
        fake_timeout = self.tmp / "timeout"
        fake_timeout.write_text(test_gcp_safety.FAKE_TIMEOUT, encoding="utf-8")
        fake_timeout.chmod(0o755)
        from datetime import datetime, timedelta, timezone
        now = datetime.now(timezone.utc)
        self.pre_start_timestamp = (now - timedelta(seconds=20)).isoformat().replace("+00:00", "Z")
        self.last_start_timestamp = (now - timedelta(seconds=10)).isoformat().replace("+00:00", "Z")

    def test_real_start_then_outer_cleanup_reaches_terminal_state(self) -> None:
        work = self.tmp / "work"
        pinned = work / "pinned" / "gcp-migration" / "profils"
        pinned.mkdir(parents=True)
        for name in PROTOCOL_FILES:
            destination = work / "pinned" / "gcp-migration" / name
            shutil.copy(ROOT / "gcp-migration" / name, destination)
            destination.chmod(0o755)
        guards = self.tmp / "guards"
        guards.mkdir()
        # COMPOSITION exigee : VRAI start, faux set_max, faux stop compteur.
        shutil.copy(ROOT / "gcp-migration" / "start_and_verify.sh", guards / "start_and_verify.sh")
        (guards / "set_max_run_duration_and_verify.sh").write_text(FAKE_SET_MAX, encoding="utf-8")
        (guards / "stop_and_verify.sh").write_text(FAKE_STOP, encoding="utf-8")
        for script in guards.iterdir():
            script.chmod(0o755)
        (work / "bundle.tgz").write_bytes(b"bundle factice")
        payload_sha = sha256_file(work / "bundle.tgz")
        commit = "0" * 40
        manifest_lines = ["schema=e-hgp.protocol-manifest.v1", f"commit={commit}"]
        for name in PROTOCOL_FILES:
            pinned_file = work / "pinned" / "gcp-migration" / name
            manifest_lines.append(
                f"{sha256_file(pinned_file)}\t{pinned_file.stat().st_size}\tgcp-migration/{name}"
            )
        manifest = self.tmp / "manifest.txt"
        manifest.write_text("\n".join(manifest_lines) + "\n", encoding="utf-8")
        calls = self.tmp / "calls.log"
        calls.write_text("", encoding="utf-8")
        receipts = self.tmp / "recu"
        env = os.environ.copy()
        env.update(
            {
                "PATH": f"{self.tmp}:{env['PATH']}",
                "FAKE_GCLOUD_SCENARIO": "guest-success",
                "FAKE_GCLOUD_LOG": str(self.tmp / "gcloud.log"),
                "FAKE_TIMEOUT_LOG": str(self.tmp / "timeout.log"),
                # Generations FIGEES pour la duree du test, recentes comme
                # dans le harnais (sans quoi chaque describe fabrique un
                # nouveau lastStartTimestamp — defense anti-generation
                # concurrente — ou la garde post-demarrage refuse l'ecart
                # avec terminationTimestamp).
                "FAKE_PRE_START_TIMESTAMP": self.pre_start_timestamp,
                "FAKE_LAST_START_TIMESTAMP": self.last_start_timestamp,
                "INTEGRATION_CALLS": str(calls),
                "GCP_SSH_KEY_FILE": str(self.tmp / "integration-ed25519"),
                "GCP_SSH_KEY_EXPIRATION_UTC": "2027-01-01T00:00:00Z",
                "MHGP6_LIFECYCLE_WORK": str(work),
                "MHGP6_LIFECYCLE_GUARDS_DIR": str(guards),
                "MHGP6_LIFECYCLE_SOURCE_COMMIT": commit,
                "MHGP6_LIFECYCLE_PAYLOAD_SHA256": payload_sha,
                "MHGP6_LIFECYCLE_MANIFEST_SHA256": sha256_file(manifest),
                "DURABLE_RECEIPT_BASE": str(receipts),
                "DURABLE_RECEIPT_PREFIX": "integration",
                "GCP_PROJECT_ID": "devpod-gpu-exploration",
                "GCP_ZONE": "europe-west4-a",
                "GCP_INSTANCE_NAME": "ehgp-blackwell-spot",
            }
        )
        subprocess.run(
            [
                "ssh-keygen", "-q", "-t", "ed25519", "-N", "", "-C", "integration",
                "-f", str(self.tmp / "integration-ed25519"),
            ],
            check=True,
        )
        result = subprocess.run(
            ["bash", str(ROOT / "gcp-migration" / "v6_session_lifecycle.sh")],
            cwd=ROOT,
            env=env,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        # Le VRAI garde a certifie le demarrage (handoff + registre avec
        # generation), puis une etape ulterieure echoue (le faux gcloud ne
        # simule pas le handshake du cycle de vie) : le cleanup EXTERIEUR
        # doit conduire l'arret nominal jusqu'au TERMINAL.
        self.assertNotEqual(result.returncode, 0)
        handoff = work / "handoff.json"
        self.assertTrue(handoff.exists(), result.stdout)
        state = (work / "etat_cycle_vie").read_text(encoding="utf-8")
        self.assertIn("schema=e-hgp.lifecycle-state.v1", state)
        self.assertIn("state=targeted_stopped", state)
        self.assertIn("generation=", state)
        generation = next(
            line.split("=", 1)[1] for line in state.splitlines() if line.startswith("generation=")
        )
        self.assertTrue(generation)
        commands = calls.read_text(encoding="utf-8")
        stop_calls = [line for line in commands.splitlines() if line.startswith("STOP ")]
        self.assertEqual(len(stop_calls), 1, commands)
        self.assertIn(f"--expected-last-start-timestamp {generation}", stop_calls[0])
        published = sorted(receipts.glob("integration_*/RECU_SESSION.txt"))
        self.assertEqual(len(published), 1, result.stdout)
        receipt = published[0].read_text(encoding="utf-8")
        self.assertIn("stop_rc=0", receipt)
        self.assertIn("etat_cycle_vie=targeted_stopped", receipt)


if __name__ == "__main__":
    unittest.main()
