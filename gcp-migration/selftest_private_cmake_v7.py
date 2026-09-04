#!/usr/bin/env python3
"""Synthetic wheels only: no network, package installation, compilation or GCP."""
from __future__ import annotations

from contextlib import ExitStack
import copy
import hashlib
import importlib.util
import io
from pathlib import Path
import stat
import tempfile
import time
import unittest
from unittest import mock
import warnings
import zipfile


HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("private_cmake_overlay", HERE / "private_cmake_v7.py")
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("overlay helper unavailable")
M = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(M)


def wheel(extra=()) -> bytes:
    stream = io.BytesIO()
    files = [(f"cmake/data/bin/{name}", f"#!/bin/sh\nprintf '{name} version 3.31.6\\n'\n".encode(), stat.S_IFREG | 0o755)
             for name in ("cmake", "ctest")]
    files.append((M.CUDA_MODULE, b"fixture CUDA20 flag, not a real CMake module", stat.S_IFREG | 0o644))
    with warnings.catch_warnings(), zipfile.ZipFile(stream, "w", zipfile.ZIP_DEFLATED) as archive:
        warnings.simplefilter("ignore", UserWarning)
        for name, data, mode in [*files, *extra]:
            info = zipfile.ZipInfo(name)
            info.external_attr = mode << 16
            archive.writestr(info, data)
    return stream.getvalue()


def synthetic_pin(data: bytes) -> ExitStack:
    stack = ExitStack()
    stack.enter_context(mock.patch.object(M, "WHEEL_SIZE", len(data)))
    stack.enter_context(mock.patch.object(M, "WHEEL_SHA256", hashlib.sha256(data).hexdigest()))
    return stack


class ToolingTests(unittest.TestCase):
    def test_selection_and_deadlines(self):
        self.assertTrue(M.installation_needed("cmake version 3.22.1\n", gpu_requested=True, nvcc_present=True, remaining_seconds=920))
        for text, gpu, nvcc, remaining in (("cmake version 3.26.0\n", True, True, 920),
                                          ("cmake version 3.22.1\n", False, True, 920),
                                          ("cmake version 3.22.1\n", True, False, 920),
                                          ("cmake version 3.22.1\n", True, True, 919)):
            self.assertFalse(M.installation_needed(text, gpu_requested=gpu, nvcc_present=nvcc, remaining_seconds=remaining))
        with self.assertRaises(ValueError):
            M.system_version("cmake unknown")
        for seconds in (0, 121):
            with self.assertRaises(ValueError): M.Budget(time.time() + 10, seconds)
        with self.assertRaises(TimeoutError): M.Budget(time.time() - 1, 1)

    def test_pinned_download(self):
        data = wheel()
        class Response(io.BytesIO):
            status = 200
            headers = {"Content-Length": str(len(data))}
            def geturl(self): return M.WHEEL_URL
        with synthetic_pin(data):
            with mock.patch.object(M.urllib.request, "urlopen", return_value=Response(data)):
                self.assertEqual(M.download(M.Budget(time.time() + 3, 3)), data)
            for bad in (data[:-1], data + b"x", b"x" * len(data)):
                with mock.patch.object(M.urllib.request, "urlopen", return_value=Response(bad)), self.assertRaises(ValueError):
                    M.download(M.Budget(time.time() + 3, 3))
            redirected = Response(data)
            redirected.geturl = lambda: "https://untrusted.example/wheel"
            with mock.patch.object(M.urllib.request, "urlopen", return_value=redirected), self.assertRaisesRegex(ValueError, "redirect"):
                M.download(M.Budget(time.time() + 3, 3))

    def test_unsafe_wheel_paths_and_types(self):
        cases = [("../escape", b"bad", stat.S_IFREG | 0o644),
                 ("/absolute", b"bad", stat.S_IFREG | 0o644),
                 ("cmake/data/a/../../escape", b"bad", stat.S_IFREG | 0o644),
                 ("cmake/data/a\\escape", b"bad", stat.S_IFREG | 0o644),
                 ("cmake/data/link", b"/outside", stat.S_IFLNK | 0o777),
                 ("cmake/data/fifo", b"", stat.S_IFIFO | 0o600),
                 ("cmake/data/bin/cmake", b"duplicate", stat.S_IFREG | 0o644)]
        for extra in cases:
            data = wheel([extra])
            with self.subTest(path=extra[0]), synthetic_pin(data), tempfile.TemporaryDirectory() as directory:
                target = Path(directory) / "extract"
                with self.assertRaises(ValueError):
                    M.unpack(data, target, M.Budget(time.time() + 3, 3))
                self.assertFalse(target.exists(), "ZIP must be fully validated before extraction")
        data = wheel()
        with synthetic_pin(data), tempfile.TemporaryDirectory() as directory, mock.patch.object(M, "MAX_UNPACKED", 1):
            with self.assertRaisesRegex(ValueError, "uncompressed"):
                M.unpack(data, Path(directory) / "extract", M.Budget(time.time() + 3, 3))

    def test_install_probes_and_receipt_mutants(self):
        data = wheel()
        with synthetic_pin(data), tempfile.TemporaryDirectory() as directory, mock.patch.object(M, "download", return_value=data):
            root = Path(directory)
            record = M.install(root, deadline_epoch=time.time() + 5, seconds=5)
            M.validate_receipt(record)
            self.assertFalse((root / "out").exists())
            self.assertEqual((root / "tooling").stat().st_mode & 0o777, 0o700)
            self.assertEqual((root / M.BINARIES["cmake"]).stat().st_mode & 0o777, 0o555)
            with self.assertRaisesRegex(ValueError, "already attempted"):
                M.install(root, deadline_epoch=time.time() + 5, seconds=5)
            for field, value in (("status", "failed"), ("version", "3.22.1"), ("wheel_sha256", "0" * 64),
                                 ("wheel_bytes", 0), ("url", "https://elsewhere"), ("elapsed_seconds", 121),
                                 ("elapsed_seconds", float("nan")), ("paths", {"cmake": "/usr/bin/cmake"})):
                changed = copy.deepcopy(record)
                changed[field] = value
                with self.subTest(field=field), self.assertRaises(ValueError): M.validate_receipt(changed)
            for field, value in (("exit_code", 1), ("stdout", "cmake version 3.22.1\n"), ("stderr", "warning"),
                                 ("argv", ["cmake", "--version"]), ("binary_sha256", "missing")):
                changed = copy.deepcopy(record)
                changed["probes"]["cmake"][field] = value
                with self.subTest(probe=field), self.assertRaises(ValueError): M.validate_receipt(changed)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "tooling").symlink_to(root)
            with self.assertRaisesRegex(ValueError, "symlink"):
                M.install(root, deadline_epoch=time.time() + 5, seconds=5)


if __name__ == "__main__":
    unittest.main()
