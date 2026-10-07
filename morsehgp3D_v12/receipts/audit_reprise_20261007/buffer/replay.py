#!/usr/bin/env python3
"""Reconstitue le patch capture sur le commit de base dans /tmp, puis rejoue le temoin borne."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    here = Path(__file__).resolve().parent
    repo = here.parents[3]
    capture = json.loads((here / "capture.json").read_text())
    base = capture["base"]
    core = subprocess.check_output(["git", "ls-tree", "-r", "--name-only", base, "--",
                                    "morsehgp3D_v12/src/core"], cwd=repo, text=True).splitlines()
    patched = {"morsehgp3D_v12/src/core/buffer.hpp", "morsehgp3D_v12/src/core/buffer.cpp"}
    tracked = set(core)
    with tempfile.TemporaryDirectory(prefix="mhgp12-buffer-audit-") as td:
        temp = Path(td)
        for rel in sorted(tracked):
            destination = temp / rel
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(subprocess.check_output(["git", "show", base + ":" + rel], cwd=repo))
        subprocess.run(["git", "apply", str(here / "buffer_capture.patch")], cwd=temp, check=True,
                       capture_output=True, text=True)
        for rel in patched:
            require(hashlib.sha256((temp / rel).read_bytes()).hexdigest() == capture["sha256"][rel],
                    "empreinte " + rel)
        source = temp / "morsehgp3D_v12/src"
        exe = temp / "eviction_race"
        command = ["g++", "-std=c++20", "-O2", "-pthread", "-DMHGP12_COORD_BITS=21", "-I", str(source),
                   str(here / "eviction_race.cpp"), str(source / "core/buffer.cpp"), "-o", str(exe)]
        subprocess.run(command, check=True, capture_output=True, text=True)
        witness = subprocess.run([str(exe)], check=True, capture_output=True, text=True, timeout=10)
        observed = json.loads(witness.stdout)
        require(observed == json.loads((here / "eviction_race.json").read_text()), "temoin different")
        body = (source / "core/buffer.cpp").read_text()
        values = [int(x.strip()) for x in re.search(r"kStepNumerator\{([^}]+)\}", body).group(1).split(",")]
        steps = int(re.search(r"kCacheSteps = (\d+)", body).group(1))
        groups = int(re.search(r"kCacheClasses = kCacheSteps \* (\d+)", body).group(1))
        minimum = 1 << int(re.search(r"kCacheMinBytes = u64\{1\} << (\d+)", body).group(1))
        capacities = [((((minimum // 1024) * values[k % steps]) << (k // steps)) + 4095) // 4096 * 4096
                      for k in range(steps * groups)]
        require(all(a < b for a, b in zip(capacities, capacities[1:])), "classes non croissantes")
        for i, capacity in enumerate(capacities):
            request = minimum if i == 0 else capacities[i - 1] + 1
            require(capacity <= request + request // 8, "marge insuffisante")
        print(json.dumps({"status": "counterexample_reproduced", "compiled_patch_hashes": len(patched),
                          "classes_margin_verified": len(capacities), "observation": observed}))


if __name__ == "__main__":
    main()
