"""Independent wire reader, digest replay, and transactional CLI fixtures."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def validate_archive(directory: Path) -> dict:
    def unique_object(pairs: list[tuple[str, object]]) -> dict:
        result = {}
        for key, value in pairs:
            require(key not in result, "duplicate JSON field")
            result[key] = value
        return result

    manifest_path = directory / "manifest.json"
    require(manifest_path.is_file() and not manifest_path.is_symlink(), "manifest regular file")
    manifest = json.loads(manifest_path.read_text(), object_pairs_hook=unique_object)
    require(type(manifest) is dict and set(manifest) == {
        "schema", "status", "public_status", "require_exact", "vertical_maps",
        "forest_semantics", "points", "kmax", "digest_all", "files",
    }, "manifest schema fields")
    require(manifest["schema"] == "mhgp7-archive-v1", "schema")
    require(manifest["status"] == "completed", "terminal status")
    require(manifest["public_status"] == "not_claimed", "public status")
    require(manifest["require_exact"] is False, "unqualified exact promotion")
    require(manifest["vertical_maps"] == "none", "unsupported vertical maps")
    require(manifest["forest_semantics"] in {
        "verified_events_only", "normalized_horizontal_h0_candidate",
    }, "unknown forest semantics")
    require(type(manifest["points"]) is int and 2 <= manifest["points"] <= 2**30 - 1, "points")
    kmax = manifest["kmax"]
    require(type(kmax) is int and 1 <= kmax <= 10, "kmax")
    require(kmax < manifest["points"], "orders exceed point count")
    require(type(manifest["digest_all"]) is str and len(manifest["digest_all"]) == 64 and
            set(manifest["digest_all"]) <= set("0123456789abcdef"), "digest syntax")
    expected_names = {"input.u16"} | {f"forest_K{k}.bin" for k in range(1, kmax + 1)}
    files = manifest["files"]
    require(type(files) is list, "files list")
    for entry in files:
        require(type(entry) is dict and set(entry) == {"name", "sha256", "bytes"}, "file entry fields")
        require(type(entry["name"]) is str and entry["name"] in expected_names, "file name")
        require(type(entry["bytes"]) is int and entry["bytes"] >= 0, "file byte count")
        require(type(entry["sha256"]) is str and len(entry["sha256"]) == 64 and
                set(entry["sha256"]) <= set("0123456789abcdef"), "file digest syntax")
    require(len(files) == len(expected_names), "duplicate/missing file")
    require({entry["name"] for entry in files} == expected_names, "file inventory")
    require({p.name for p in directory.iterdir()} == expected_names | {"manifest.json"}, "physical inventory")
    for entry in files:
        file_path = directory / entry["name"]
        require(file_path.is_file() and not file_path.is_symlink(), "payload regular file")
        data = file_path.read_bytes()
        require(len(data) == entry["bytes"], "file length")
        require(hashlib.sha256(data).hexdigest() == entry["sha256"], "file hash")
    source = (directory / "input.u16").read_bytes()
    magic = b"mhgp7-input-u16-v1\n"
    require(source.startswith(magic), "input magic")
    count = struct.unpack_from("<Q", source, len(magic))[0]
    require(count == manifest["points"] and len(source) == len(magic) + 8 + 10 * count, "input count")
    points = list(struct.iter_unpack("<IHHH", source[len(magic) + 8:]))
    ids = {point[0] for point in points}
    require(len(ids) == count, "input identities")
    digests = []
    wire_offsets = []
    for k in range(1, kmax + 1):
        data = (directory / f"forest_K{k}.bin").read_bytes()
        magic = b"mhgp7-forest-file-v1\n"
        require(data.startswith(magic), "forest magic")
        position = len(magic)

        def take(size: int) -> bytes:
            nonlocal position
            require(0 <= size <= len(data) - position, "truncated forest")
            value = data[position:position + size]
            position += size
            return value

        def uint(size: int) -> int:
            return int.from_bytes(take(size), "little")

        def facet() -> tuple[int, ...]:
            require(uint(1) == k, "facet order")
            values = tuple(uint(4) for _ in range(10))
            require(all(x == 0 for x in values[k:]), "facet padding")
            key = values[:k]
            require(tuple(sorted(set(key))) == key and set(key) <= ids, "facet identities")
            return key

        require(uint(4) == k, "file order")
        nfacets = uint(8)
        require(nfacets <= len(data) // 41, "facet count bound")
        keys = [facet() for _ in range(nfacets)]
        require(keys == sorted(set(keys)), "facet canonical order")
        if k == 1:
            require(keys == sorted((point_id,) for point_id in ids), "K1 complete point identities")
        require(uint(8) == nfacets, "partition count")
        canon_offset = position
        canon = [uint(4) for _ in range(nfacets)]
        require(all(fid <= i and canon[fid] == fid for i, fid in enumerate(canon)), "partition idempotence")
        ndeltas = uint(8)
        require(ndeltas <= len(data) // 105, "delta count bound")
        previous_level = (0, 1)
        previous_batch = -1
        known_keys = set(keys)
        key_ids = {key: i for i, key in enumerate(keys)}
        parent = list(range(nfacets))
        seen = [k == 1] * nfacets
        reduced = k >= 2 and manifest["forest_semantics"] == "normalized_horizontal_h0_candidate"
        pending: list[tuple[int, list[int], list[int]]] = []
        delta_offsets = []

        def find(fid: int) -> int:
            while parent[fid] != fid:
                parent[fid] = parent[parent[fid]]
                fid = parent[fid]
            return fid

        def apply_batch() -> None:
            # Judge only exported deltas, never source cofaces or product DSU.
            # Read every parent in the same pre-batch snapshot before union.
            consumed: set[int] = set()
            for output, parents, born in pending:
                require(parents or born, "empty delta")
                require(len(parents) != 1 or bool(born), "unreduced empty continuation")
                for fid in parents:
                    require(find(fid) == fid, "parent is not a pre-batch canonical root")
                    require(not reduced or seen[fid], "latent facet used as a reduced parent")
                for fid in born:
                    require(not seen[fid] and find(fid) == fid, "repeated or already incident materialization")
                references = parents + born
                require(len(set(references)) == len(references), "parent and birth overlap")
                require(not consumed.intersection(references), "pre-batch component consumed twice")
                consumed.update(references)
                require(output == min(references), "delta output is not canonical")
            for output, parents, born in pending:
                for fid in parents + born:
                    parent[fid] = output
                    seen[fid] = True
                seen[output] = True
            pending.clear()

        for _ in range(ndeltas):
            batch_offset = position
            batch = uint(8)
            numerator = uint(24)
            denominator = uint(16)
            require(0 < denominator < 2**127, "positive level denominator")
            require(numerator * previous_level[1] >= previous_level[0] * denominator, "level monotonicity")
            require(batch >= previous_batch, "batch order")
            same_level = numerator * previous_level[1] == previous_level[0] * denominator
            if previous_batch >= 0:
                require((batch == previous_batch) == same_level, "batch and exact level disagree")
            if batch != previous_batch:
                apply_batch()
            previous_level = numerator, denominator
            previous_batch = batch
            output_offset = position
            output = facet()
            delta_offsets.append({"batch": batch_offset, "output": output_offset})
            require(output in known_keys, "delta output")
            lists = []
            for _ in range(2):
                size = uint(8)
                require(size <= nfacets, "delta list bound")
                values = [facet() for _ in range(size)]
                require(values == sorted(set(values)) and set(values) <= known_keys, "delta membership")
                lists.append([key_ids[key] for key in values])
            pending.append((key_ids[output], lists[0], lists[1]))
        apply_batch()
        require(all(seen), "exported facet never materialized")
        require([find(fid) for fid in range(nfacets)] == canon, "terminal partition differs from delta replay")
        require(position == len(data), "trailing forest bytes")
        # The archived fields deliberately equal the historical canonical digest wire.
        digests.append(hashlib.sha256(b"mhgp4-digest-v1:forest" + data[len(magic):]).hexdigest())
        wire_offsets.append({"canon": canon_offset, "keys": keys, "deltas": delta_offsets})
    # This exact chaining contract is tested against the product's terminal digest.
    actual_digest = hashlib.sha256(b"mhgp4-digest-v1:all" + "".join(digests).encode()).hexdigest()
    require(manifest["digest_all"] == actual_digest, "terminal forest digest replay")
    return {"points": points, "digests": digests, "manifest": manifest, "wire_offsets": wire_offsets}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--binary", type=Path, required=True)
    args = parser.parse_args()
    binary = args.binary.resolve()
    scenes = 0
    with tempfile.TemporaryDirectory(prefix="mhgp7-archive-test-") as name:
        root = Path(name)
        source = root / "points.txt"
        original = [(99, 0, 0, 7), (7, 0, 9, 6), (123, 1, 4, 0), (0, 0, 0, 1), (42, 4, 1, 2)]
        source.write_text("# id x y z\n" + "\n".join(" ".join(map(str, p)) for p in original))

        def run(arguments: list[str], expected: int) -> subprocess.CompletedProcess:
            nonlocal scenes
            result = subprocess.run([str(binary), *arguments], text=True, capture_output=True, timeout=120)
            require(result.returncode == expected, f"rc {result.returncode} != {expected}: {result.stderr}")
            scenes += 1
            return result

        run([f"--input={source}", f"--output={root / 'one'}", "--threads=1", "--smax=4"], 0)
        one = validate_archive(root / "one")
        require(one["points"] == original, "original point order lost")
        run([f"--input={source}", f"--output={root / 'many'}", "--threads=4", "--smax=4", "--layout=csr"], 0)
        many = validate_archive(root / "many")
        require(one["digests"] == many["digests"], "thread/layout export mismatch")
        normalized = []
        for layout, threads in (("classic", 1), ("csr", 4)):
            destination = root / ("normalized_" + layout)
            run([f"--input={source}", f"--output={destination}", f"--threads={threads}",
                 "--smax=4", f"--layout={layout}", "--complete-incidences"], 0)
            checked = validate_archive(destination)
            require(checked["manifest"]["forest_semantics"] == "normalized_horizontal_h0_candidate",
                    "normalization semantics missing")
            normalized.append(checked["digests"])
        require(normalized[0] == normalized[1], "normalized layout/thread mismatch")
        require(normalized[0] != one["digests"], "normalized E5 fixture vacuous")
        for cap in ("core-records", "chain-steps", "cofaces", "query-nodes", "meb-supports"):
            destination = root / ("cap_" + cap)
            refused = run([f"--input={source}", f"--output={destination}", "--complete-incidences",
                           "--smax=4", "--fold-join=1", f"--silent-{cap}=0"], 2)
            require(not refused.stdout and "silent_" in refused.stderr and "budget" in refused.stderr,
                    "silent cap cause or provisional payload")
            require(not destination.exists(), "silent cap published a K1 prefix")
        for invalid in ("--silent-core-records=-1", "--silent-core-records=1x", "--silent-core-records=", "--silent-unknown=1"):
            run([f"--input={source}", "--complete-incidences", invalid], 2)
        run([f"--input={source}", "--silent-core-records=100"], 2)
        # Permanent self-signed corruptions from the independent audit. All
        # hashes are recomputed so these exercise semantics, not SHA mismatch.
        for mutation, expected in (
            ("partition", "terminal partition differs from delta replay"),
            ("batches", "batch and exact level disagree"),
            ("output", "delta output is not canonical"),
        ):
            destination = root / ("corrupt_" + mutation)
            shutil.copytree(root / "one", destination)
            path = destination / "forest_K1.bin"
            data = bytearray(path.read_bytes())
            offsets = one["wire_offsets"][0]
            require(len(offsets["keys"]) == 5 and len(offsets["deltas"]) >= 2, "structural mutant floor")
            if mutation == "partition":
                start = offsets["canon"]
                data[start:start + 20] = struct.pack("<5I", *range(5))
            elif mutation == "batches":
                for delta in offsets["deltas"]:
                    start = delta["batch"]
                    data[start:start + 8] = bytes(8)
            else:
                start = offsets["deltas"][0]["output"] + 1
                old = struct.unpack_from("<I", data, start)[0]
                changed = next(key[0] for key in offsets["keys"] if key[0] != old)
                data[start:start + 4] = struct.pack("<I", changed)
            path.write_bytes(data)
            manifest = json.loads((destination / "manifest.json").read_text())
            for entry in manifest["files"]:
                content = (destination / entry["name"]).read_bytes()
                entry.update(sha256=hashlib.sha256(content).hexdigest(), bytes=len(content))
            magic = b"mhgp7-forest-file-v1\n"
            changed_digests = [hashlib.sha256(b"mhgp4-digest-v1:forest" +
                (destination / f"forest_K{k}.bin").read_bytes()[len(magic):]).hexdigest()
                for k in range(1, manifest["kmax"] + 1)]
            manifest["digest_all"] = hashlib.sha256(b"mhgp4-digest-v1:all" + "".join(changed_digests).encode()).hexdigest()
            (destination / "manifest.json").write_text(json.dumps(manifest))
            try:
                validate_archive(destination)
            except RuntimeError as error:
                require(str(error) == expected, f"structural mutant caught for wrong reason: {error}")
            else:
                raise RuntimeError("self-signed structural mutation accepted")
        before = (root / "one" / "manifest.json").read_bytes()
        manifest_path = root / "one" / "manifest.json"
        for mutate in (
            lambda m: m.pop("vertical_maps"),
            lambda m: m.update(vertical_maps="complete"),
            lambda m: m.update(forest_semantics="exact"),
            lambda m: m.update(points=True),
            lambda m: m.update(unknown_field=1),
            lambda m: m["files"][0].update(bytes=True),
            lambda m: m["files"][0].update(unknown_field=1),
        ):
            changed = json.loads(before)
            mutate(changed)
            manifest_path.write_text(json.dumps(changed))
            rejected = False
            try:
                validate_archive(root / "one")
            except RuntimeError:
                rejected = True
            finally:
                manifest_path.write_bytes(before)
            require(rejected, "invalid manifest schema accepted")
        manifest_path.write_bytes(before.replace(b'{', b'{"schema":"duplicate",', 1))
        try:
            rejected = False
            try:
                validate_archive(root / "one")
            except RuntimeError:
                rejected = True
            require(rejected, "duplicate manifest key accepted")
        finally:
            manifest_path.write_bytes(before)
        run([f"--input={source}", f"--output={root / 'one'}"], 2)
        require((root / "one" / "manifest.json").read_bytes() == before, "existing output overwritten")
        run([f"--input={source}", f"--output={root / 'exact'}", "--require-exact"], 2)
        require(not (root / "exact").exists(), "unqualified exact output")
        run([f"--input={source}", "--n=5"], 2)
        # A late pipeline refusal must remove the previously written input file too.
        duplicate = root / "duplicate.txt"
        duplicate.write_text("1 0 0 0\n2 0 0 0\n")
        run([f"--input={duplicate}", f"--output={root / 'refused'}"], 2)
        require(not (root / "refused").exists(), "refused payload published")
        for bad in (b"0 0 0 0\n1 1 2 65536\n", b"0 0 0 0\n1 1 2 3suffix\n", b"0 0 0 0\n1 1 2 3 4\n",
                    b"0 0 0 0\n1 1 2 -1\n", b"0 0 0 0\n1 1 2 3\0garbage\n", b"0 0 0 0\n" + b"9" * 300):
            malformed = root / "bad.txt"
            malformed.write_bytes(bad)
            run([f"--input={malformed}"], 2)
        require(not list(root.glob(".mhgp7-provisional-*")), "provisional directory leaked")
        corrupt = root / "many" / "forest_K2.bin"
        data = corrupt.read_bytes()
        corrupt.write_bytes(data[:-1] + bytes([data[-1] ^ 1]))
        rejected = False
        try:
            validate_archive(root / "many")
        except RuntimeError:
            rejected = True
        require(rejected, "corruption undetected")
    require(scenes >= 24, "non-vacuity")
    print(f"archive_gate=passed scenes={scenes} digest_replay=independent delta_replay=independent corruption=rejected")


if __name__ == "__main__":
    main()
