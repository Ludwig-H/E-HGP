#!/usr/bin/env python3
"""Recreate the one-site LiDAR ablation from pinned v8 quarter inputs."""

import argparse
import hashlib
import json
from pathlib import Path
import struct


POINTS_SHA = "6924df0523a457925a2398585fd170eeb02d6efacf9e673739d2c40b14fd1f68"
IDS_SHA = "815ce101e675fdea99541265aef73730ad5d2aac16550e4283245b8d88c2dda6"
OUTPUT_SHA = "40aa7bd17d429316493a276b6738041a20bd0d41b3f22840f7618cfb36669923"
REMOVED_ID = 122516
REMOVED_XYZ = (113499, 18184, 0)


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--points", type=Path, required=True)
    parser.add_argument("--original-site-ids", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()

    points = args.points.read_bytes()
    ids = args.original_site_ids.read_bytes()
    if sha256(points) != POINTS_SHA or sha256(ids) != IDS_SHA:
        raise ValueError("source SHA-256 mismatch")
    if len(points) != 12 * 14829 or len(ids) != 4 * 14829:
        raise ValueError("source length mismatch")

    positions = [i for i, (site_id,) in enumerate(struct.iter_unpack("<I", ids))
                 if site_id == REMOVED_ID]
    if len(positions) != 1:
        raise ValueError("removed site is not unique")
    index = positions[0]
    xyz = struct.unpack_from("<III", points, 12 * index)
    if xyz != REMOVED_XYZ or index != 13848:
        raise ValueError("removed site position or coordinates differ")

    result = points[:12 * index] + points[12 * (index + 1):]
    if sha256(result) != OUTPUT_SHA:
        raise ValueError("ablation SHA-256 mismatch")
    args.out.write_bytes(result)
    z = [xyz[2] for xyz in struct.iter_unpack("<III", result)]
    print(json.dumps({"removed_original_id": REMOVED_ID, "new_sites": len(z),
                      "new_z_min_mm": min(z), "new_z_max_mm": max(z),
                      "output_sha256": sha256(result)}, sort_keys=True))


if __name__ == "__main__":
    main()
