#!/usr/bin/env python3
"""Offline refusal/redaction checks; no cloud commands or real publication."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import publish_closed as publish


class PublicationGuards(unittest.TestCase):
    def test_redaction(self):
        raw = (b"compte : person@example.test\n"
               b"ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIExample comment\n"
               b"generation=2026-09-26T12:20:06.956-07:00\nTERMINATED\n")
        result = publish.redact(raw)
        self.assertNotIn(b"person@example.test", result)
        self.assertNotIn(b"AAAAC3NzaC1lZDI1NTE5AAAAIExample", result)
        self.assertIn(b"generation=2026-09-26T12:20:06.956-07:00\nTERMINATED", result)

    def test_private_key_marker_refused(self):
        with self.assertRaisesRegex(ValueError, "private key"):
            publish.redact(b"-----BEGIN OPENSSH PRIVATE KEY-----\nfixture\n")

    def test_live_receipt_refused_without_output(self):
        for receipt in (dict(status="completed", targeted_shutdown_certified=False),
                        dict(status="worker_returned", targeted_shutdown_certified=True)):
            with tempfile.TemporaryDirectory(prefix="mhgp9-publish-refusal-") as tmp:
                root = Path(tmp)
                host = root / "gpu_filter_v9_host"
                host.mkdir()
                (host / "receipt.json").write_text(json.dumps(receipt))
                out = root / "publication"
                argv = ["publish_closed.py", "--host", str(host), "--package", str(root / "absent-package"),
                        "--after-stop", str(root / "absent-stop"), "--output", str(out)]
                with patch("sys.argv", argv), self.assertRaisesRegex(ValueError, "closed completed"):
                    publish.main()
                self.assertFalse(out.exists())

    def test_session_parent_refused(self):
        with tempfile.TemporaryDirectory(prefix="mhgp9-publish-parent-") as tmp:
            root = Path(tmp)
            out = root / "publication"
            argv = ["publish_closed.py", "--host", str(root), "--package", str(root / "absent-package"),
                    "--after-stop", str(root / "absent-stop"), "--output", str(out)]
            with patch("sys.argv", argv), self.assertRaisesRegex(ValueError, "exact host"):
                publish.main()
            self.assertFalse(out.exists())


if __name__ == "__main__":
    unittest.main()
