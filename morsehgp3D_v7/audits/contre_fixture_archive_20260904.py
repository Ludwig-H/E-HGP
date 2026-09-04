"""Bounded audit: valid CLI archives and self-consistent corruptions of their wire.

Only the supplied audits work directory is written. The independent replay is
structural: it neither recomputes geometry nor certifies Gamma completeness.
"""

from __future__ import annotations

import argparse
from fractions import Fraction
import hashlib
import importlib.util
from itertools import groupby
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
MAGIC = b"mhgp7-forest-file-v1\n"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def decode(data: bytes) -> dict:
    require(data.startswith(MAGIC), "forest magic")
    offset = len(MAGIC)

    def uint(size: int) -> int:
        nonlocal offset
        require(offset + size <= len(data), "truncated field")
        value = int.from_bytes(data[offset:offset + size], "little")
        offset += size
        return value

    k = uint(4)

    def facet() -> tuple[int, ...]:
        require(uint(1) == k, "facet order")
        values = tuple(uint(4) for _ in range(10))
        return values[:k]

    count = uint(8)
    require(count <= len(data) // 41, "facet bound")
    keys = [facet() for _ in range(count)]
    require(uint(8) == count, "partition count")
    canon_offset = offset
    canon = [uint(4) for _ in range(count)]
    delta_count = uint(8)
    require(delta_count <= len(data) // 105, "delta bound")
    deltas = []
    for _ in range(delta_count):
        batch_offset = offset
        batch = uint(8)
        numerator, denominator = uint(24), uint(16)
        require(denominator > 0, "level denominator")
        output_offset = offset
        output = facet()
        parent_count = uint(8)
        require(parent_count <= count, "parent bound")
        parents = [facet() for _ in range(parent_count)]
        born_count = uint(8)
        require(born_count <= count, "birth bound")
        born = [facet() for _ in range(born_count)]
        deltas.append({"batch": batch, "level": Fraction(numerator, denominator),
                       "output": output, "parents": parents, "born": born,
                       "batch_offset": batch_offset, "output_offset": output_offset})
    require(offset == len(data), "trailing bytes")
    return {"k": k, "keys": keys, "canon": canon,
            "canon_offset": canon_offset, "deltas": deltas}


def replay_partition(forest: dict, normalized: bool = False) -> None:
    """Replay sets, independent of the C++ fold/DSU and its digest functions."""
    keys = forest["keys"]
    components = {key: frozenset([key]) for key in keys}
    owner = {key: key for key in keys}
    seen = set(keys) if forest["k"] == 1 else set()
    for _, batch_items in groupby(forest["deltas"], key=lambda delta: delta["batch"]):
        batch = list(batch_items)
        require(len({delta["level"] for delta in batch}) == 1,
                "one batch has different exact levels")
        consumed = set()
        updates = []
        for delta in batch:
            parents, born = delta["parents"], delta["born"]
            references = parents + born
            require(bool(references), "empty component delta")
            require(len(set(references)) == len(references), "repeated delta reference")
            require(all(owner[key] == key for key in references), "noncanonical reference")
            require(not normalized or all(key in seen for key in parents),
                    "parent not materialized before batch")
            require(all(key not in seen for key in born), "repeated materialization")
            roots = {owner[key] for key in references}
            require(not consumed.intersection(roots), "component consumed twice in batch")
            consumed.update(roots)
            members = frozenset().union(*(components[root] for root in roots))
            canonical = min(members)
            require(delta["output"] == canonical,
                    "delta output is not the canonical component")
            updates.append((canonical, members, roots, references))
        for canonical, members, roots, references in updates:
            for root in roots:
                del components[root]
            components[canonical] = members
            seen.update(references)
            for key in members:
                owner[key] = canonical
    require(seen == set(keys), "facet never materialized")
    key_to_id = {key: index for index, key in enumerate(keys)}
    actual = [key_to_id[owner[key]] for key in keys]
    require(actual == forest["canon"], "terminal partition differs from delta replay")


def refresh_hashes(directory: Path) -> None:
    manifest_path = directory / "manifest.json"
    manifest = json.loads(manifest_path.read_text())
    for entry in manifest["files"]:
        data = (directory / entry["name"]).read_bytes()
        entry.update(sha256=hashlib.sha256(data).hexdigest(), bytes=len(data))
    digests = []
    for k in range(1, manifest["kmax"] + 1):
        data = (directory / f"forest_K{k}.bin").read_bytes()
        digests.append(hashlib.sha256(b"mhgp4-digest-v1:forest" + data[len(MAGIC):]).hexdigest())
    manifest["digest_all"] = hashlib.sha256(
        b"mhgp4-digest-v1:all" + "".join(digests).encode()).hexdigest()
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, required=True)
    parser.add_argument("--validator", type=Path, required=True)
    parser.add_argument("--work-dir", type=Path, required=True)
    parser.add_argument("--receipt", type=Path)
    parser.add_argument("--require-product-rejections", action="store_true")
    args = parser.parse_args()
    audits = Path(__file__).resolve().parent
    work = args.work_dir.resolve()
    require(work.is_relative_to(audits), "work directory must be inside audits")
    if args.receipt:
        require(args.receipt.resolve().is_relative_to(audits), "receipt must be inside audits")
    work.mkdir(parents=True, exist_ok=True)
    module_spec = importlib.util.spec_from_file_location("archive_reader_under_audit", args.validator)
    require(module_spec is not None and module_spec.loader is not None, "validator module")
    validator = importlib.util.module_from_spec(module_spec)
    module_spec.loader.exec_module(validator)
    receipt = {"schema": "mhgp7-audit-interfaces-v1", "public_status": "not_claimed",
               "binary_sha256": hashlib.sha256(args.binary.read_bytes()).hexdigest(),
               "validator_sha256": hashlib.sha256(args.validator.read_bytes()).hexdigest(),
               "cli_cases": [], "mutants": [], "normalized_archives": []}
    with tempfile.TemporaryDirectory(prefix="interface-", dir=work) as temporary:
        root = Path(temporary)
        source = root / "input.txt"
        original = [(99, 0, 0, 7), (7, 0, 9, 6), (123, 1, 4, 0),
                    (0, 0, 0, 1), (42, 4, 1, 2)]
        source.write_text("\n".join(" ".join(map(str, point)) for point in original))

        def run(name: str, options: list[str], expected: int) -> subprocess.CompletedProcess:
            result = subprocess.run([str(args.binary.resolve()), *options], capture_output=True,
                                    text=True, timeout=30)
            require(result.returncode == expected,
                    f"{name}: rc={result.returncode} expected={expected}: {result.stderr}")
            if expected != 0:
                require(not result.stdout, f"{name}: refused payload on stdout")
            receipt["cli_cases"].append({"case": name, "exit_code": result.returncode})
            return result

        baseline = root / "baseline"
        run("valid_classic", [f"--input={source}", f"--output={baseline}", "--smax=4"], 0)
        parsed = validator.validate_archive(baseline)
        require(parsed["points"] == original, "original point order")
        for k in range(1, 4):
            replay_partition(decode((baseline / f"forest_K{k}.bin").read_bytes()))
        alternate = root / "alternate"
        run("valid_csr_parallel_wide_s", [f"--input={source}", f"--output={alternate}",
            "--smax=4", "--layout=csr", "--threads=4", "--s=9223372036854775807"], 0)
        require(validator.validate_archive(alternate)["digests"] == parsed["digests"],
                "wide separation/layout/thread object mismatch")
        normalized_directories = []
        for layout, threads in (("classic", 1), ("csr", 4)):
            destination = root / f"normalized_{layout}"
            run(f"valid_normalized_{layout}_{threads}_threads",
                [f"--input={source}", f"--output={destination}", "--smax=4",
                 f"--layout={layout}", f"--threads={threads}", "--complete-incidences"], 0)
            normalized = validator.validate_archive(destination)
            require(normalized["points"] == original, "normalized original point order")
            require(normalized["manifest"]["forest_semantics"] ==
                    "normalized_horizontal_h0_candidate", "normalized semantics")
            require(normalized["digests"] != parsed["digests"], "normalized E5 non-vacuity")
            stats = []
            for k in range(1, 4):
                forest = decode((destination / f"forest_K{k}.bin").read_bytes())
                replay_partition(forest, normalized=True)
                stats.append({"k": k, "facets": len(forest["keys"]),
                              "deltas": len(forest["deltas"]),
                              "parents": sum(len(delta["parents"]) for delta in forest["deltas"]),
                              "born": sum(len(delta["born"]) for delta in forest["deltas"])})
            require(stats[1]["parents"] > 0 and stats[1]["born"] > 0,
                    "normalized K2 parents/materializations non-vacuity")
            receipt["normalized_archives"].append({"layout": layout, "threads": threads,
                "forest_semantics": normalized["manifest"]["forest_semantics"],
                "digests": normalized["digests"], "stats": stats})
            normalized_directories.append(destination)
        for k in range(1, 4):
            require((normalized_directories[0] / f"forest_K{k}.bin").read_bytes() ==
                    (normalized_directories[1] / f"forest_K{k}.bin").read_bytes(),
                    "normalized born/parents/output wire differs by layout or threads")
        receipt["normalized_wire_equal"] = True
        before = (baseline / "manifest.json").read_bytes()
        run("create_only", [f"--input={source}", f"--output={baseline}"], 2)
        require((baseline / "manifest.json").read_bytes() == before, "existing archive changed")
        for name, options in [
            ("exact_unqualified", ["--require-exact"]),
            ("input_generation_exclusive", ["--n=5"]),
            ("budget_below_candidate", ["--mem-budget=1"]),
            ("invalid_s", ["--s=7"]),
            ("invalid_s_suffix", ["--s=8x"]),
            ("invalid_thread_count", ["--threads=0"]),
            ("unknown_layout", ["--layout=bogus"]),
            ("product_mutant_rejected", ["--inject=render-active-only"]),
        ]:
            destination = root / name
            run(name, [f"--input={source}", f"--output={destination}", *options], 2)
            require(not destination.exists(), f"{name}: output published")
        invalid_inputs = {
            "duplicate_id": b"1 0 0 0\n1 1 2 3\n",
            "duplicate_position": b"1 0 0 0\n2 0 0 0\n",
            "id_overflow": b"1 0 0 0\n4294967296 1 2 3\n",
            "coordinate_overflow": b"1 0 0 0\n2 1 2 65536\n",
            "negative_coordinate": b"1 0 0 0\n2 1 2 -1\n",
            "signed_id": b"1 0 0 0\n+2 1 2 3\n",
            "decimal_coordinate": b"1 0 0 0\n2 1 2 3.0\n",
            "exponent_coordinate": b"1 0 0 0\n2 1 2 3e0\n",
            "missing_field": b"1 0 0 0\n2 1 2\n",
            "extra_field": b"1 0 0 0\n2 1 2 3 4\n",
            "nul_byte": b"1 0 0 0\n2 1 2 3\x00\n",
            "line_limit": b"1 0 0 0\n" + b"7" * 300,
            "one_point": b"1 0 0 0\n",
        }
        for name, data in invalid_inputs.items():
            malformed = root / f"{name}.txt"
            malformed.write_bytes(data)
            destination = root / name
            run(name, [f"--input={malformed}", f"--output={destination}"], 2)
            require(not destination.exists(), f"{name}: output published")
        require(not list(root.glob(".mhgp7-provisional-*")), "provisional directory leak")
        mutation_cases = [(baseline, 1, False), (normalized_directories[0], 2, True)]
        for archive_source, k, normalized_mode, mutant in (
                (archive_source, k, normalized_mode, mutant)
                for archive_source, k, normalized_mode in mutation_cases
                for mutant in ("reset_final_partition", "collapse_batches", "wrong_delta_output")):
            semantics = ("normalized_horizontal_h0_candidate" if normalized_mode
                         else "verified_events_only")
            directory = root / f"{semantics}_{mutant}"
            shutil.copytree(archive_source, directory)
            path = directory / f"forest_K{k}.bin"
            data = bytearray(path.read_bytes())
            forest = decode(data)
            count = len(forest["keys"])
            require(count >= 2 and len(forest["deltas"]) >= 2,
                    "mutant non-vacuity")
            require(forest["canon"] == [0] * count, "connected baseline")
            if mutant == "reset_final_partition":
                offset = forest["canon_offset"]
                for i in range(count):
                    data[offset + i * 4:offset + (i + 1) * 4] = i.to_bytes(4, "little")
            elif mutant == "collapse_batches":
                require(len({d["level"] for d in forest["deltas"]}) >= 2, "distinct levels")
                for delta in forest["deltas"]:
                    offset = delta["batch_offset"]
                    data[offset:offset + 8] = bytes(8)
            else:
                offset = forest["deltas"][0]["output_offset"]
                previous = forest["deltas"][0]["output"]
                chosen = next(key for key in forest["keys"] if key != previous)
                data[offset:offset + 41] = (bytes([k]) +
                    b"".join(point_id.to_bytes(4, "little") for point_id in chosen) +
                    bytes((10 - k) * 4))
            path.write_bytes(data)
            refresh_hashes(directory)
            product_rejected = False
            try:
                validator.validate_archive(directory)
            except (RuntimeError, ValueError, KeyError, IndexError):
                product_rejected = True
            independent_error = ""
            try:
                replay_partition(decode(data), normalized=normalized_mode)
            except RuntimeError as error:
                independent_error = str(error)
            require(bool(independent_error), f"{mutant}: independent audit missed corruption")
            receipt["mutants"].append({"name": mutant, "forest_semantics": semantics, "k": k,
                                       "product_reader_rejected": product_rejected,
                                       "independent_rejection": independent_error})
    require(len(receipt["cli_cases"]) == 26 and len(receipt["mutants"]) == 6, "non-vacuity")
    if args.receipt:
        args.receipt.parent.mkdir(parents=True, exist_ok=True)
        args.receipt.write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))
    if args.require_product_rejections and not all(
            mutant["product_reader_rejected"] for mutant in receipt["mutants"]):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
