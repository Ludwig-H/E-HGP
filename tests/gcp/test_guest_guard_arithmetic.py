"""Arithmetique EXACTE de la garde invitee (audit serie C § 5.19.1).

Le texte de la garde est celui que `start_and_verify.sh` transporte par SSH
(`--print-guest-guard-script` l'imprime sans aucun appel GCP) ; il est
execute ICI avec un faux `shutdown` (ecrit le fichier scheduled avec un skew
choisi) et un faux `date` (instant d'armement choisi). Les quatre frontieres
demandees : marge nominale 600 s => armement a 600 s accepte, 601 s refuse
(sans skew) ; avec skew systemd +120 s => 480 s accepte, 481 s refuse. Plus :
skew au-dela de la tolerance refuse, mode non-poweroff refuse, `shutdown -c`
avant `-P`. Aucune assertion Python `assert` (tenir sous python3 -O).
"""
from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
START = ROOT / "gcp-migration" / "start_and_verify.sh"

FAKE_SHUTDOWN = """#!/usr/bin/env bash
# Faux shutdown : journalise, et sur -P +N ecrit le fichier scheduled avec
# USEC = (FAKE_NOW + N*60 + FAKE_SKEW) * 1e6 et MODE=FAKE_MODE.
printf '%s\\n' "$*" >> "${FAKE_SHUTDOWN_LOG}"
case "$1" in
  -c) exit 0 ;;
  -P)
    minutes="${2#+}"
    usec=$(( (FAKE_NOW + minutes * 60 + FAKE_SKEW) * 1000000 ))
    printf 'MODE=%s\\nUSEC=%s\\n' "${FAKE_MODE:-poweroff}" "${usec}" > "${FAKE_SCHEDULED_FILE}"
    exit 0 ;;
esac
exit 9
"""

FAKE_DATE = """#!/usr/bin/env bash
[ "$1" = "+%s" ] || { echo "faux date : format inattendu $*" >&2; exit 9; }
printf '%s\\n' "${FAKE_NOW}"
"""

MINUTES = 405
NOW = 1_800_000_000


class GuestGuardArithmeticTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = Path(tempfile.mkdtemp(prefix="ehgp-guest-guard."))
        fake_bin = self.tmp / "bin"
        fake_bin.mkdir()
        (fake_bin / "shutdown").write_text(FAKE_SHUTDOWN, encoding="utf-8")
        (fake_bin / "shutdown").chmod(0o755)
        (fake_bin / "date").write_text(FAKE_DATE, encoding="utf-8")
        (fake_bin / "date").chmod(0o755)
        printed = subprocess.run(
            ["bash", str(START), "--print-guest-guard-script"],
            cwd=ROOT, text=True, capture_output=True, check=False, timeout=30,
        )
        self.assertEqual(printed.returncode, 0, printed.stderr)
        self.script = printed.stdout.strip()
        self.assertIn('readonly scheduled_file="${3:-/run/systemd/shutdown/scheduled}"', self.script)
        self.assertNotIn("\n", self.script)
        self.fake_bin = fake_bin

    def run_guard(self, *, deadline: int, skew: int, mode: str = "poweroff",
                  minutes: int = MINUTES) -> tuple[int, str, str]:
        scheduled = self.tmp / "scheduled"
        if scheduled.exists():
            scheduled.unlink()
        log = self.tmp / "shutdown.log"
        if log.exists():
            log.unlink()
        env = dict(os.environ)
        env.update({
            "PATH": f"{self.fake_bin}:{env['PATH']}",
            "FAKE_NOW": str(NOW),
            "FAKE_SKEW": str(skew),
            "FAKE_MODE": mode,
            "FAKE_SCHEDULED_FILE": str(scheduled),
            "FAKE_SHUTDOWN_LOG": str(log),
        })
        result = subprocess.run(
            ["bash", "-c", self.script, "--", str(minutes), str(deadline), str(scheduled)],
            env=env, text=True, capture_output=True, check=False, timeout=30,
        )
        journal = log.read_text(encoding="utf-8") if log.exists() else ""
        return result.returncode, result.stdout, journal

    # Marge nominale M = deadline - (NOW + minutes*60) : l'armement « a t
    # secondes » du depart correspond a M = 600 - t.
    def deadline_for_margin(self, margin: int) -> int:
        return NOW + MINUTES * 60 + margin

    def test_arming_at_600s_without_skew_is_accepted(self) -> None:
        rc, out, journal = self.run_guard(deadline=self.deadline_for_margin(0), skew=0)
        self.assertEqual(rc, 0, out)
        self.assertIn("__EHGP_GUEST_GUARD_VERIFIED__", out)
        self.assertIn("MODE=poweroff", out)
        self.assertEqual(journal.splitlines()[0], "-c")
        self.assertEqual(journal.splitlines()[1], f"-P +{MINUTES} Coupe-circuit E-HGP")

    def test_arming_at_601s_without_skew_is_refused(self) -> None:
        rc, out, _journal = self.run_guard(deadline=self.deadline_for_margin(-1), skew=0)
        self.assertNotEqual(rc, 0)
        self.assertNotIn("__EHGP_GUEST_GUARD_VERIFIED__", out)

    def test_arming_at_480s_with_120s_skew_is_accepted(self) -> None:
        rc, out, _journal = self.run_guard(deadline=self.deadline_for_margin(120), skew=120)
        self.assertEqual(rc, 0, out)
        self.assertIn("__EHGP_GUEST_GUARD_VERIFIED__", out)

    def test_arming_at_481s_with_120s_skew_is_refused(self) -> None:
        rc, out, _journal = self.run_guard(deadline=self.deadline_for_margin(119), skew=120)
        self.assertNotEqual(rc, 0)
        self.assertNotIn("__EHGP_GUEST_GUARD_VERIFIED__", out)

    def test_skew_beyond_systemd_tolerance_is_refused_even_with_margin(self) -> None:
        rc, out, _journal = self.run_guard(deadline=self.deadline_for_margin(600), skew=121)
        self.assertNotEqual(rc, 0)
        self.assertNotIn("__EHGP_GUEST_GUARD_VERIFIED__", out)
        rc, out, _journal = self.run_guard(deadline=self.deadline_for_margin(600), skew=-121)
        self.assertNotEqual(rc, 0)
        self.assertNotIn("__EHGP_GUEST_GUARD_VERIFIED__", out)

    def test_non_poweroff_mode_is_refused(self) -> None:
        rc, out, _journal = self.run_guard(deadline=self.deadline_for_margin(600), skew=0, mode="reboot")
        self.assertNotEqual(rc, 0)
        self.assertNotIn("__EHGP_GUEST_GUARD_VERIFIED__", out)

    def test_scheduled_in_the_past_is_refused(self) -> None:
        rc, out, _journal = self.run_guard(deadline=self.deadline_for_margin(600), skew=-(MINUTES * 60 + 1), minutes=MINUTES)
        self.assertNotEqual(rc, 0)
        self.assertNotIn("__EHGP_GUEST_GUARD_VERIFIED__", out)

    def test_print_mode_touches_no_gcloud(self) -> None:
        fake_bin = self.tmp / "bin-gcloud"
        fake_bin.mkdir()
        witness = self.tmp / "gcloud.calls"
        (fake_bin / "gcloud").write_text(
            "#!/usr/bin/env bash\nprintf '%s\\n' \"$*\" >> \"" + str(witness) + "\"\nexit 97\n",
            encoding="utf-8",
        )
        (fake_bin / "gcloud").chmod(0o755)
        env = dict(os.environ)
        env["PATH"] = f"{fake_bin}:{env['PATH']}"
        result = subprocess.run(
            ["bash", str(START), "--print-guest-guard-script"],
            cwd=ROOT, env=env, text=True, capture_output=True, check=False, timeout=30,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(witness.exists(), "le mode d'impression a appele gcloud")


if __name__ == "__main__":
    unittest.main()
