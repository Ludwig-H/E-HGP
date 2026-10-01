#!/usr/bin/env python3
"""Strict autonomous replay: external manifest authority, closed inventory."""
import hashlib
import json
import pathlib
import stat
import subprocess
import sys

ROOT = pathlib.Path(__file__).absolute().parent
EXPECTED_FILES = {"README.md", "check.py", "read.py", "record.py", "capture_normal.json",
                  "capture_optimized.json", "manifest.json"}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(ok, what):
    if not ok:
        raise RuntimeError(what)


def sha(value):
    return type(value) is str and len(value) == 64 and all(c in "0123456789abcdef" for c in value)


def strict_json(text):
    def reject(value):
        raise RuntimeError("floating/nonfinite JSON value: " + value)

    def pairs(items):
        result = {}
        for key, value in items:
            require(key not in result, "duplicate JSON key: " + key)
            result[key] = value
        return result

    return json.loads(text, object_pairs_hook=pairs, parse_float=reject, parse_constant=reject)


def inventory():
    # No resolve(): a symlinked root or ancestor is refused rather than laundered.
    for directory in (ROOT,) + tuple(ROOT.parents):
        require(stat.S_ISDIR(directory.lstat().st_mode), "root/ancestor is not a real directory")
    require(set(p.name for p in ROOT.iterdir()) == EXPECTED_FILES, "closed inventory mismatch")
    for name in EXPECTED_FILES:
        require(stat.S_ISREG((ROOT / name).lstat().st_mode), "payload is not a regular nonsymlink file")


def main():
    require(len(sys.argv) == 2, "expected external manifest SHA argument")
    expected = sys.argv[1]
    require(sha(expected), "SHA format")
    inventory()
    require(digest(ROOT / "manifest.json") == expected, "external manifest SHA mismatch")
    manifest = strict_json((ROOT / "manifest.json").read_text())
    require(type(manifest) is dict and set(manifest) == {"format", "files"}, "manifest schema")
    require(manifest.get("format") == "meb-shell-euler-private-v2", "manifest format")
    hashes = manifest["files"]
    require(type(hashes) is dict and set(hashes) == EXPECTED_FILES - {"manifest.json"}, "manifest inventory")
    require(all(sha(h) for h in hashes.values()), "manifest hash type/format")
    require(all(digest(ROOT / n) == h for n, h in hashes.items()), "file hash mismatch")
    stdout = None
    for optimized, name in ((False, "capture_normal.json"), (True, "capture_optimized.json")):
        capture = strict_json((ROOT / name).read_text())
        require(type(capture) is dict and set(capture) == {"optimized", "command", "returncode", "stdout", "stderr",
                "files_before", "files_after", "shared_before", "shared_after"}, "capture schema")
        require(capture["optimized"] is optimized and type(capture["returncode"]) is int
                and capture["returncode"] == 0 and type(capture["stdout"]) is str and capture["stderr"] == "",
                "capture status")
        captured_command = capture["command"]
        require(type(captured_command) is list and all(type(v) is str for v in captured_command), "command type")
        require(len(captured_command) == (4 if optimized else 3)
                and captured_command[1:-1] == (["-B", "-O"] if optimized else ["-B"])
                and pathlib.Path(captured_command[-1]).is_absolute()
                and pathlib.Path(captured_command[-1]).name == "check.py", "captured command")
        for field in ("files_before", "files_after", "shared_before", "shared_after"):
            require(type(capture[field]) is dict and all(type(n) is str and sha(h) for n, h in capture[field].items()),
                    "capture hashes schema")
        require(set(capture["shared_before"]) == {
            "/workspaces/E-HGP/build/v10-verrou-points/revision_cible/majorites_continues/MEMO.md",
            "/workspaces/E-HGP/build/v10-verrou-points/revision_cible/majorites_continues/mmc.py"}, "shared pin keys")
        require(capture["files_before"] == capture["files_after"], "captured source mutation")
        require(capture["shared_before"] == capture["shared_after"], "captured shared source mutation")
        require(capture["files_before"] == {n: hashes[n] for n in ("README.md", "check.py", "read.py", "record.py")},
                "capture does not pin frozen sources")
        command = [sys.executable, "-B"] + (["-O"] if optimized else []) + [str(ROOT / "check.py")]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=30)
        require(result.returncode == 0 and not result.stderr and result.stdout == capture["stdout"], "replay mismatch")
        if stdout is None:
            stdout = result.stdout
        require(stdout == result.stdout, "normal/-O divergence")
    result = strict_json(stdout)
    require(result["status"] == "PASS" and len(result["fixtures"]) == 10, "fixture census")
    require(result["caratheodory_subsets"] == 628 and len(result["meb_cases"]) == 4, "oracle census")
    require(result["fixed_k_case_count"] == 480 and result["fixed_k_unique_memoized_subsets"] == 20505
            and result["fixed_k_exhaustive_subsets"] == 41808, "fixed-K oracle census")
    require(len(result["fixed_k_checks"]) == 10 and all(len(c["cases"]) == 48 for c in result["fixed_k_checks"]),
            "fixed-K fixture inventory")
    require(set(result["mutants_killed"]) == {"zeros_positive", "no_alternation", "canonical_support_only",
            "transpose_abs_sign", "fixed_k_missing_empty_correction"}, "causal mutant census")
    inventory()
    require(all(digest(ROOT / n) == h for n, h in hashes.items()), "files changed after replay")
    require(digest(ROOT / "manifest.json") == expected, "manifest changed after replay")
    print("PASS closed seven-file packet; exact Euler + sparse transpose + independent oracles; normal/-O")


if __name__ == "__main__":
    main()
