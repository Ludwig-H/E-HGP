"""Reproduit deux limites du cache sans executer de suite sanitizer.

Usage: python3 check_cache.py --source /path/to/morsehgp3D_v11 --work /tmp/cache-audit
Le verdict valide le temoin, il ne qualifie pas le moteur.
"""
import argparse
import hashlib
import json
import pathlib
import re
import subprocess


def run(command):
    result = subprocess.run(command, text=True, capture_output=True, check=False)
    if result.returncode:
        raise RuntimeError(f"{command}: {result.returncode}\n{result.stderr}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=pathlib.Path, required=True)
    parser.add_argument("--work", type=pathlib.Path, required=True)
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=True)
    src = args.source / "src"
    buffer = src / "core/buffer.cpp"
    probe = pathlib.Path(__file__).with_name("cache_probe.cpp")
    binary = args.work / "cache_probe"
    compile_command = ["g++", "-std=c++20", "-DMHGP11_COORD_BITS=21", "-O2", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
                       "-I", str(src), str(probe), str(buffer), "-pthread",
                       "-Wl,--wrap=_ZnwmRKSt9nothrow_t", "-o", str(binary)]
    run(compile_command)
    witness = json.loads(run([str(binary)]))
    compilers = {}
    for compiler in ("g++", "clang++"):
        command = [compiler, "-std=c++20", "-DMHGP11_COORD_BITS=21", "-E", "-P", "-fsanitize=address", "-I", str(src), str(buffer)]
        preprocessed = run(command)
        match = re.search(r"void poison\(void\* block, u64 bytes\) noexcept \{(.*?)\n\}",
                          preprocessed, re.DOTALL)
        if match is None:
            raise RuntimeError(f"poison absent pour {compiler}")
        body = match.group(1).strip()
        compilers[compiler] = {
            "version": run([compiler, "--version"]).splitlines()[0],
            "command": command, "poison_body": body,
            "asan_poison_called": "__asan_poison_memory_region" in body,
        }
    valid = (witness["allocated"] > witness["used"] + witness["cache_limit"]
             and witness["released"] and compilers["g++"]["asan_poison_called"]
             and not compilers["clang++"]["asan_poison_called"])
    output = {"schema": "ehgp.audit.v12.cache.v1", "witnesses_reproduced": valid,
              "source_sha256": hashlib.sha256(buffer.read_bytes()).hexdigest(),
              "probe_sha256": hashlib.sha256(probe.read_bytes()).hexdigest(),
              "compile_command": compile_command, "allocation": witness, "preprocessing": compilers,
              "scope": "Release witness and compiler preprocessing only; no ASan runtime qualification"}
    print(json.dumps(output, indent=2, ensure_ascii=False))
    return 0 if valid else 1


if __name__ == "__main__":
    raise SystemExit(main())
