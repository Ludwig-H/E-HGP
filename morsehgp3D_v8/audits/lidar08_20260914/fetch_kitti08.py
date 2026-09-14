#!/usr/bin/env python3
"""Fetch seven KITTI-08 scans by bounded HTTP Range, plus calibration/poses.

No labels, no full 80 GB download, and no implicit quantification. The poses
are KITTI odometry ground truth, not the SuMa poses supplied by SemanticKITTI.
Raw files are local evidence under data/, never product source fixtures.
"""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import io
import json
from pathlib import Path
import sys
import urllib.request
import zipfile
import zlib


ROOT = Path(__file__).resolve().parent
BASE = "https://s3.eu-central-1.amazonaws.com/avg-kitti/"
MAX_REQUEST = 8 * 1024 * 1024
MAX_TRANSFER = 32 * 1024 * 1024
MAX_MEMBER = 4 * 1024 * 1024
FRAMES = (0, 1, 2, 3, 4, 100, 200)
SELECTED = (
    ("data_odometry_velodyne.zip", tuple(
        f"dataset/sequences/08/velodyne/{frame:06d}.bin" for frame in FRAMES)),
    ("data_odometry_calib.zip", ("dataset/sequences/08/calib.txt",)),
    ("data_odometry_poses.zip", ("dataset/poses/08.txt",)),
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def stamp() -> str:
    return datetime.now(timezone.utc).isoformat()


def sha256(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


class RangeArchive(io.RawIOBase):
    """Read-only seek adapter; validates 206/length/ETag before reading bytes."""

    def __init__(self, url: str, receipt: dict, budget: dict):
        self.url = url
        self.receipt = receipt
        self.budget = budget
        self.position = 0
        receipt.update(url=url, requests=[])
        request = urllib.request.Request(url, method="HEAD", headers={"Accept-Encoding": "identity"})
        with urllib.request.urlopen(request, timeout=30) as response:
            require(response.status == 200 and response.url == url, "unexpected HEAD status/redirect")
            self.size = int(response.headers["Content-Length"])
            self.etag = response.headers["ETag"]
            require(self.size > 0 and bool(self.etag), "missing archive identity")
            require(response.headers.get("Accept-Ranges") == "bytes", "byte ranges unsupported")
            receipt.update(size_bytes=self.size, etag=self.etag,
                           last_modified=response.headers.get("Last-Modified"),
                           head_status=response.status, head_received_utc=stamp())

    def readable(self) -> bool:
        return True

    def seekable(self) -> bool:
        return True

    def tell(self) -> int:
        return self.position

    def seek(self, offset: int, whence: int = 0) -> int:
        if whence == 0:
            target = offset
        elif whence == 1:
            target = self.position + offset
        elif whence == 2:
            target = self.size + offset
        else:
            raise ValueError("unsupported seek mode")
        require(0 <= target <= self.size, "seek outside archive")
        self.position = target
        return target

    def read(self, size: int = -1) -> bytes:
        amount = self.size - self.position if size < 0 else min(size, self.size - self.position)
        require(0 <= amount <= MAX_REQUEST, "request exceeds bounded Range budget")
        if not amount:
            return b""
        require(self.budget["requested_bytes"] + amount <= MAX_TRANSFER, "transfer budget exceeded")
        self.budget["requested_bytes"] += amount
        start, end = self.position, self.position + amount - 1
        item = dict(start=start, end_inclusive=end, requested_bytes=amount,
                    started_utc=stamp(), status="failed")
        self.receipt["requests"].append(item)
        request = urllib.request.Request(self.url, headers={
            "Range": f"bytes={start}-{end}", "If-Match": self.etag,
            "Accept-Encoding": "identity"})
        try:
            with urllib.request.urlopen(request, timeout=30) as response:
                item["http_status"] = response.status
                require(response.status == 206 and response.url == self.url,
                        "server ignored Range or redirected request")
                require(response.headers.get("ETag") == self.etag, "archive changed during extraction")
                require(response.headers.get("Content-Range") == f"bytes {start}-{end}/{self.size}",
                        "incorrect Content-Range")
                require(int(response.headers["Content-Length"]) == amount, "incorrect range length")
                raw = response.read(amount + 1)
                item["received_bytes"] = len(raw)
                self.budget["received_bytes"] += len(raw)
                require(len(raw) == amount, "truncated or overlong range body")
            self.position += amount
            item.update(status="completed", sha256=sha256(raw))
            return raw
        except Exception as error:
            item["error"] = f"{type(error).__name__}: {error}"
            raise
        finally:
            item["finished_utc"] = stamp()


def extract(archive_name: str, members: tuple[str, ...], manifest: dict) -> None:
    archive = {}
    manifest["archives"].append(archive)
    source = RangeArchive(BASE + archive_name, archive, manifest["transport"])
    with zipfile.ZipFile(source) as zipped:
        infos = zipped.infolist()
        require(len({item.filename for item in infos}) == len(infos), "duplicate ZIP entry names")
        archive["central_directory_entries"] = len(infos)
        for name in members:
            info = zipped.getinfo(name)
            require(not info.is_dir() and not info.flag_bits & 1, "encrypted/directory member")
            require(info.compress_type in (zipfile.ZIP_STORED, zipfile.ZIP_DEFLATED),
                    "unsupported compression")
            require(0 < info.file_size <= MAX_MEMBER and 0 < info.compress_size <= MAX_MEMBER,
                    "member exceeds explicit size cap")
            target = ROOT / "data" / name
            require(target.resolve().is_relative_to((ROOT / "data").resolve()), "unsafe member path")
            reused = target.exists()
            if reused:
                raw = target.read_bytes()
            else:
                # ZipFile verifies the member CRC after decompression.
                raw = zipped.read(info)
            require(len(raw) == info.file_size and zlib.crc32(raw) & 0xffffffff == info.CRC,
                    "uncompressed member size/CRC mismatch")
            if not reused:
                target.parent.mkdir(parents=True, exist_ok=True)
                with target.open("xb") as output:
                    output.write(raw)
            if name.endswith(".bin"):
                require(len(raw) % 16 == 0, "scan length is not a float32 xyzi array")
            manifest["files"].append(dict(
                path=str(target.relative_to(ROOT)), archive_url=source.url, archive_etag=source.etag,
                zip_member=name, zip_header_offset=info.header_offset,
                zip_method=info.compress_type, compressed_bytes=info.compress_size,
                bytes=len(raw), zip_crc32=f"{info.CRC:08x}", sha256=sha256(raw),
                points=len(raw) // 16 if name.endswith(".bin") else None,
                reused_verified_local_file=reused))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", default="FETCH_MANIFEST.json")
    args = parser.parse_args()
    require(Path(args.manifest).name == args.manifest and args.manifest.endswith(".json"),
            "manifest must be a local JSON filename")
    manifest = dict(
        schema="mhgp8_kitti08_range_fetch_v1", status="failed", started_utc=stamp(),
        command=[sys.executable, *sys.argv], script_sha256=sha256(Path(__file__).read_bytes()),
        purpose="bounded_real_outdoor_lidar_audit_not_product_qualification",
        primary_pages=["https://www.cvlibs.net/datasets/kitti/eval_odometry.php",
                       "https://www.semantic-kitti.org/dataset.html"],
        sequence="08", frames=list(FRAMES), labels_downloaded=False,
        pose_kind="kitti_odometry_ground_truth_not_semantickitti_suma",
        raw_format="little_endian_float32_x_y_z_remission",
        quantification_applied=False, raw_data_git_tracked=False,
        archive_hash_scope="ETag identity and requested-range hashes; no whole-archive SHA256",
        limits=dict(max_request_bytes=MAX_REQUEST, max_transfer_bytes=MAX_TRANSFER,
                    max_uncompressed_member_bytes=MAX_MEMBER),
        transport=dict(requested_bytes=0, received_bytes=0), archives=[], files=[])
    code = 1
    # Never overwrite a failed capture. A retry must select another manifest.
    with (ROOT / args.manifest).open("x") as output:
        try:
            for archive_name, members in SELECTED:
                extract(archive_name, members, manifest)
            require(len(manifest["files"]) == 9, "incomplete requested extraction")
            require(manifest["script_sha256"] == sha256(Path(__file__).read_bytes()),
                    "fetch source changed during execution")
            manifest["status"] = "completed"
            code = 0
        except BaseException as error:
            manifest["error"] = f"{type(error).__name__}: {error}"
        finally:
            manifest["finished_utc"] = stamp()
            json.dump(manifest, output, indent=2, allow_nan=False)
            output.write("\n")
    print(json.dumps(dict(status=manifest["status"], files=len(manifest["files"]),
                          transport=manifest["transport"], error=manifest.get("error"))))
    return code


if __name__ == "__main__":
    raise SystemExit(main())
