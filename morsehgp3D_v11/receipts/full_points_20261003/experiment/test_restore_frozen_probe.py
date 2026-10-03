"""Read pinned artifacts and reject mutations; never execute the native ELF."""
import argparse
import json
from pathlib import Path
import tempfile

import restore_frozen_probe as restore


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data", required=True, type=Path)
    args = parser.parse_args()
    binary = (args.data / "points_probe.c40").read_bytes()
    manifest = (args.data / "points_probe.c40.build.json").read_bytes()
    result = restore.verify_frozen(binary, manifest)
    checks = 1
    mutations = [(binary[:-1], manifest), (binary + b"x", manifest),
                 (b"bad!" + binary[4:], manifest),
                 (binary[:100] + bytes([binary[100] ^ 1]) + binary[101:], manifest),
                 (binary, manifest[:-1]), (binary, manifest + b"x"),
                 (binary, manifest[:100] + bytes([manifest[100] ^ 1]) + manifest[101:])]
    for left, right in mutations:
        try:
            restore.verify_frozen(left, right)
        except restore.BuildFailure:
            checks += 1
        else:
            raise ValueError("corrupt frozen artifact admitted")
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        plain = root / "bytes"
        plain.write_bytes(binary)
        if restore.read_bounded(plain, len(binary)) != binary:
            raise ValueError("bounded copy changed bytes")
        checks += 1
        link = root / "alias"
        link.symlink_to(plain)
        for path, size in ((link, len(binary)), (plain, len(binary) - 1),
                           (plain, len(binary) + 1), (root, len(binary))):
            try:
                restore.read_bounded(path, size)
            except restore.BuildFailure:
                checks += 1
            else:
                raise ValueError("alias or invalid payload size admitted")
    print(json.dumps(dict(status="pass", checks=checks, native_executed=False,
                          source_commit=result["source_commit"],
                          binary_sha256=restore.BINARY_SHA,
                          manifest_sha256=restore.MANIFEST_SHA,
                          scope="pinned artifact reads and mutation refusals only"), sort_keys=True))


if __name__ == "__main__":
    main()
