from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from tools.check_oracle_independence import audit_oracle_imports


class OracleIndependenceTests(unittest.TestCase):
    def test_repository_oracle_uses_only_reviewed_imports(self) -> None:
        self.assertEqual(audit_oracle_imports(), ())

    def test_external_absolute_imports_are_rejected_deterministically(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            package = Path(directory) / "oracle"
            package.mkdir()
            (package / "__init__.py").write_text(
                "from .geometry import value\n", encoding="utf-8"
            )
            (package / "geometry.py").write_text(
                "from fractions import Fraction\n"
                "import numpy\n"
                "from hgp_old.engine import run\n",
                encoding="utf-8",
            )

            violations = audit_oracle_imports(package)
            self.assertEqual(
                tuple((item.path, item.line) for item in violations),
                (("oracle/geometry.py", 2), ("oracle/geometry.py", 3)),
            )
            self.assertIn("numpy", violations[0].reason)
            self.assertIn("hgp_old.engine", violations[1].reason)

    def test_relative_escape_and_dynamic_import_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            package = Path(directory) / "oracle"
            subpackage = package / "nested"
            subpackage.mkdir(parents=True)
            (package / "__init__.py").write_text("", encoding="utf-8")
            (subpackage / "module.py").write_text(
                "from ...production import kernel\n"
                "module = __import__('production')\n",
                encoding="utf-8",
            )

            violations = audit_oracle_imports(package)
            self.assertEqual(len(violations), 2)
            self.assertIn("escapes", violations[0].reason)
            self.assertIn("dynamic import", violations[1].reason)


if __name__ == "__main__":
    unittest.main()
