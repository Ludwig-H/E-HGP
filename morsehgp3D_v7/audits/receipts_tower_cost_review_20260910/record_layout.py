"""Record one bounded ABI probe; refuse to overwrite any previous receipt."""
import hashlib
import json
from pathlib import Path
import subprocess
import time

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    output = BASE / "layout_capture"
    output.mkdir()
    work = BASE / ".work_layout"
    work.mkdir(exist_ok=True)
    files = sorted((ROOT / "morsehgp3D_v7/src").rglob("*.hpp"))
    files.append(BASE / "layout.cpp")
    before = {str(p.relative_to(ROOT)): sha(p) for p in files}
    commands = []
    calls = [
        ("compiler", ["g++", "--version"]),
        ("compile", ["g++", "-std=c++20", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                     "-MMD", "-MF", str(output / "layout.d"), str(BASE / "layout.cpp"),
                     "-o", str(work / "layout")]),
        ("run", [str(work / "layout")]),
    ]
    for name, argv in calls:
        start = time.time_ns()
        result = subprocess.run(argv, cwd=ROOT, capture_output=True, check=False)
        commands.append(dict(name=name, argv=argv, started_ns=start, ended_ns=time.time_ns(),
                             exit_code=result.returncode))
        (output / (name + ".stdout")).write_bytes(result.stdout)
        (output / (name + ".stderr")).write_bytes(result.stderr)
        (output / "commands.json").write_text(json.dumps(commands, indent=2) + "\n")
        if result.returncode:
            raise RuntimeError("ABI probe failed: " + name)
    deps = (output / "layout.d").read_text().replace("\\\n", " ").split(":", 1)[1].split()
    pins = {str(Path(p).resolve().relative_to(ROOT)): before[str(Path(p).resolve().relative_to(ROOT))]
            for p in deps}
    if any(sha(ROOT / name) != pin for name, pin in pins.items()):
        raise RuntimeError("ABI source changed during compile/run")
    receipt = dict(status="passed", source_pins_before_after=pins,
                   executable_sha256=sha(work / "layout"), engine_executed=False, gcp_used=False,
                   system_headers_archived=False)
    (output / "receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    print((output / "run.stdout").read_text(), end="")


if __name__ == "__main__":
    main()
