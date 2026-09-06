"""Reject polluted signatures while retaining explicitly named prefixes."""
from __future__ import annotations

import base64
import shlex
import subprocess
import sys


def main() -> int:
    script = sys.argv[1]
    cases = [
        ("signature", "EXPECT_LINE", "signature", True),
        ("before\nsignature\nafter", "EXPECT_LINE", "signature", True),
        ("signature-suffix", "EXPECT_LINE", "signature", False),
        ("prefix-signature", "EXPECT_LINE", "signature", False),
        ("signature suffix", "EXPECT_PREFIX", "signature", True),
        ("prefix signature", "EXPECT_PREFIX", "signature", False),
    ]
    for output, field, value, accepted in cases:
        encoded = base64.b64encode(output.encode()).decode()
        source = f"import base64;print(base64.b64decode('{encoded}').decode())"
        command = [
            "cmake", "-DEXPECTED=0", f"-DCMD={sys.executable}",
            "-DARGS=" + shlex.join(["-c", source]),
            f"-D{field}={value}", "-P", script,
        ]
        result = subprocess.run(command, capture_output=True, text=True, check=False)
        if (result.returncode == 0) != accepted:
            print(f"unexpected signature verdict: {output!r} {field}", file=sys.stderr)
            print(result.stdout + result.stderr, file=sys.stderr)
            return 1
    print(f"exact_line_cases={len(cases)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
