#!/usr/bin/env python3
"""Validate the Phase 3 nanobind module on the authorized CUDA G4 target."""

from __future__ import annotations

import argparse
import ctypes
import gc
import importlib
import json
import pathlib
import sys
from collections.abc import Mapping
from typing import NoReturn

MODULE_NAME = "morsehgp3d_phase3"
PROBE_BYTES = 4096
DLPACK_COMMIT = "84d107bf416c6bab9ae68ad285876600d230490d"
NANOBIND_COMMIT = "2a61ad2494d09fecb2e13322c1383342c299900d"
DLPACK_CAPSULE_NAME = b"dltensor_versioned"
DLPACK_CONSUMED_CAPSULE_NAME = b"used_dltensor_versioned"


class DLPackVersion(ctypes.Structure):
    _fields_ = [("major", ctypes.c_uint32), ("minor", ctypes.c_uint32)]


class DLDevice(ctypes.Structure):
    _fields_ = [("device_type", ctypes.c_int32), ("device_id", ctypes.c_int32)]


class DLDataType(ctypes.Structure):
    _fields_ = [
        ("code", ctypes.c_uint8),
        ("bits", ctypes.c_uint8),
        ("lanes", ctypes.c_uint16),
    ]


class DLTensor(ctypes.Structure):
    _fields_ = [
        ("data", ctypes.c_void_p),
        ("device", DLDevice),
        ("ndim", ctypes.c_int32),
        ("dtype", DLDataType),
        ("shape", ctypes.POINTER(ctypes.c_int64)),
        ("strides", ctypes.POINTER(ctypes.c_int64)),
        ("byte_offset", ctypes.c_uint64),
    ]


class DLManagedTensorVersioned(ctypes.Structure):
    pass


DLManagedTensorDeleter = ctypes.CFUNCTYPE(
    None, ctypes.POINTER(DLManagedTensorVersioned)
)
DLManagedTensorVersioned._fields_ = [
    ("version", DLPackVersion),
    ("manager_ctx", ctypes.c_void_p),
    ("deleter", DLManagedTensorDeleter),
    ("flags", ctypes.c_uint64),
    ("dl_tensor", DLTensor),
]

capsule_new = ctypes.pythonapi.PyCapsule_New
capsule_new.restype = ctypes.py_object
capsule_new.argtypes = (ctypes.c_void_p, ctypes.c_char_p, ctypes.c_void_p)
capsule_get_pointer = ctypes.pythonapi.PyCapsule_GetPointer
capsule_get_pointer.restype = ctypes.c_void_p
capsule_get_pointer.argtypes = (ctypes.py_object, ctypes.c_char_p)
capsule_set_name = ctypes.pythonapi.PyCapsule_SetName
capsule_set_name.restype = ctypes.c_int
capsule_set_name.argtypes = (ctypes.py_object, ctypes.c_char_p)
capsule_get_context = ctypes.pythonapi.PyCapsule_GetContext
capsule_get_context.restype = ctypes.c_void_p
capsule_get_context.argtypes = (ctypes.py_object,)


def fail(message: str) -> NoReturn:
    raise AssertionError(message)


def require_mapping(value: object, context: str) -> Mapping[str, object]:
    if not isinstance(value, Mapping):
        fail(f"{context} must be a mapping, got {type(value).__name__}")
    return value


def require_exact_int(value: object, context: str) -> int:
    if type(value) is not int:
        fail(f"{context} must be an exact int, got {type(value).__name__}")
    return value


def require_exact_bool(value: object, context: str) -> bool:
    if type(value) is not bool:
        fail(f"{context} must be an exact bool, got {type(value).__name__}")
    return value


def require_keys(
    record: Mapping[str, object],
    expected: set[str],
    context: str,
) -> None:
    missing = expected.difference(record)
    if missing:
        fail(f"{context} is missing keys: {sorted(missing)}")
    unexpected = set(record).difference(expected)
    if unexpected:
        fail(f"{context} has unexpected keys: {sorted(unexpected)}")


def expect_invalid_call(module: object, value: object) -> None:
    try:
        module.dlpack_zero_copy_probe(value)
    except (TypeError, ValueError, module.Phase3CudaError) as error:
        if not str(error):
            fail(f"invalid input {value!r} produced an empty error")
    else:
        fail(f"invalid input {value!r} was accepted")


def expect_invalid_capsule_size(module: object, value: object) -> None:
    try:
        module.make_dlpack_capsule(value)
    except (TypeError, ValueError, module.Phase3CudaError) as error:
        if not str(error):
            fail(f"invalid capsule input {value!r} produced an empty error")
    else:
        fail(f"invalid capsule input {value!r} was accepted")


def validate_environment(
    module: object,
    *,
    expected_live_bytes: int = 0,
) -> Mapping[str, object]:
    environment = require_mapping(module.environment(), "environment")
    require_keys(
        environment,
        {
            "schema_version",
            "phase",
            "backend",
            "profile",
            "mode",
            "purpose",
            "scientific_result_claimed",
            "scientific_public_status",
            "allocation_limit_bytes",
            "live_device_bytes",
            "dlpack_cleanup_failures",
            "versions",
            "gpu",
        },
        "environment",
    )
    if require_exact_int(environment["schema_version"], "schema_version") != 1:
        fail("unsupported environment schema version")
    if require_exact_int(environment["phase"], "phase") != 3:
        fail("binding did not report Phase 3")
    if environment["backend"] != "cuda_g4":
        fail("binding did not report backend=cuda_g4")
    if environment["profile"] != "hgp_reduced":
        fail("binding did not report the hgp_reduced profile priority")
    if environment["mode"] != "certified":
        fail("binding did not report mode=certified")
    if environment["purpose"] != "reproducibility_infrastructure_qualification":
        fail("binding purpose is not limited to infrastructure qualification")
    if require_exact_bool(
        environment["scientific_result_claimed"],
        "scientific_result_claimed",
    ):
        fail("Phase 3 binding must not claim a scientific result")
    if environment["scientific_public_status"] is not None:
        fail("Phase 3 binding exposed a scientific public status")

    allocation_limit = require_exact_int(
        environment["allocation_limit_bytes"],
        "allocation_limit_bytes",
    )
    if allocation_limit < PROBE_BYTES:
        fail("configured allocation limit is smaller than the modest probe")
    if (
        require_exact_int(environment["live_device_bytes"], "live_device_bytes")
        != expected_live_bytes
    ):
        fail(
            "device-memory counter does not match the expected ownership: "
            f"expected {expected_live_bytes}, got {environment['live_device_bytes']}"
        )
    if (
        require_exact_int(
            environment["dlpack_cleanup_failures"],
            "dlpack_cleanup_failures",
        )
        != 0
    ):
        fail("a prior DLPack cleanup failed")

    versions = require_mapping(environment["versions"], "versions")
    require_keys(
        versions, {"dlpack", "nanobind", "cuda", "cub_compile_encoded"}, "versions"
    )
    dlpack = require_mapping(versions["dlpack"], "versions.dlpack")
    nanobind = require_mapping(versions["nanobind"], "versions.nanobind")
    cuda = require_mapping(versions["cuda"], "versions.cuda")
    for record, expected, expected_commit, context in (
        (dlpack, (1, 3, 0), DLPACK_COMMIT, "versions.dlpack"),
        (nanobind, (2, 12, 0), NANOBIND_COMMIT, "versions.nanobind"),
    ):
        require_keys(record, {"major", "minor", "patch", "commit"}, context)
        actual = tuple(
            require_exact_int(record[key], f"{context}.{key}")
            for key in ("major", "minor", "patch")
        )
        if actual != expected:
            fail(f"{context} expected version {expected}, got {actual}")
        if record["commit"] != expected_commit:
            fail(
                f"{context}.commit expected {expected_commit}, "
                f"got {record['commit']!r}"
            )

    require_keys(
        cuda,
        {
            "compile_encoded",
            "compile",
            "runtime_encoded",
            "runtime",
            "driver_encoded",
            "driver",
        },
        "versions.cuda",
    )
    compile_version = require_exact_int(
        cuda["compile_encoded"],
        "versions.cuda.compile_encoded",
    )
    if not 12090 <= compile_version < 12100:
        fail(f"binding was not compiled with CUDA 12.9.x: {compile_version}")
    if require_exact_int(cuda["runtime_encoded"], "versions.cuda.runtime_encoded") <= 0:
        fail("CUDA runtime version is unavailable")
    if require_exact_int(cuda["driver_encoded"], "versions.cuda.driver_encoded") <= 0:
        fail("CUDA driver version is unavailable")
    if require_exact_int(versions["cub_compile_encoded"], "cub_compile_encoded") <= 0:
        fail("Toolkit-provided CUB version is unavailable")

    gpu = require_mapping(environment["gpu"], "gpu")
    require_keys(
        gpu,
        {
            "device_count",
            "device_id",
            "name",
            "compute_capability",
            "total_memory_bytes",
            "free_memory_bytes",
            "async_allocator_supported",
        },
        "gpu",
    )
    if require_exact_int(gpu["device_count"], "gpu.device_count") < 1:
        fail("no CUDA device is visible")
    if not isinstance(gpu["name"], str) or not gpu["name"]:
        fail("GPU name is empty")
    if gpu["compute_capability"] != [12, 0]:
        fail(
            f"G4 qualification requires compute capability 12.0, got {gpu['compute_capability']!r}"
        )
    if not require_exact_bool(
        gpu["async_allocator_supported"],
        "gpu.async_allocator_supported",
    ):
        fail("CUDA asynchronous allocation is not supported")
    if require_exact_int(gpu["total_memory_bytes"], "gpu.total_memory_bytes") <= 0:
        fail("GPU total memory is unavailable")
    return environment


def validate_probe(module: object) -> Mapping[str, object]:
    capsule = module.make_dlpack_capsule(PROBE_BYTES)
    if type(capsule).__name__ != "PyCapsule":
        fail(f"DLPack producer returned {type(capsule).__name__}, not PyCapsule")
    managed_address = capsule_get_pointer(capsule, DLPACK_CAPSULE_NAME)
    ownership_context = capsule_get_context(capsule)
    if ownership_context in (None, 0):
        fail("DLPack producer did not attach a private ownership context")
    alias_capsule = capsule_new(managed_address, DLPACK_CAPSULE_NAME, None)
    if capsule_get_context(alias_capsule) not in (None, 0):
        fail("foreign alias unexpectedly inherited the private ownership context")
    try:
        module.consume_dlpack_capsule(alias_capsule)
    except ValueError as error:
        if "not the owning" not in str(error):
            fail(f"aliased DLPack capsule failed ambiguously: {error}")
    else:
        fail("an aliased DLPack capsule consumed another capsule's export")
    probe = require_mapping(
        module.consume_dlpack_capsule(capsule),
        "consume_dlpack_capsule",
    )
    require_keys(
        probe,
        {
            "schema_version",
            "operation",
            "exchange_protocol",
            "capsule_name_before",
            "capsule_name_after",
            "requested_bytes",
            "allocation_limit_bytes",
            "allocation_address",
            "dlpack_data_address",
            "pointer_identity",
            "zero_copy",
            "copy_operations",
            "live_device_bytes_before",
            "peak_live_device_bytes",
            "live_device_bytes_after",
            "dlpack",
            "scientific_result_claimed",
            "scientific_public_status",
        },
        "probe",
    )
    if require_exact_int(probe["schema_version"], "probe.schema_version") != 1:
        fail("unsupported probe schema version")
    if probe["operation"] != "dlpack_versioned_capsule_exchange":
        fail("unexpected probe operation")
    if probe["exchange_protocol"] != "python_array_api_dlpack_versioned_capsule":
        fail("probe did not traverse the versioned Python DLPack capsule protocol")
    if probe["capsule_name_before"] != "dltensor_versioned":
        fail("producer did not export the standard versioned DLPack capsule name")
    if probe["capsule_name_after"] != "used_dltensor_versioned":
        fail("consumer did not mark the DLPack capsule as consumed")
    for key in ("requested_bytes", "peak_live_device_bytes"):
        if require_exact_int(probe[key], f"probe.{key}") != PROBE_BYTES:
            fail(f"probe.{key} does not match the request")
    allocation_address = require_exact_int(
        probe["allocation_address"],
        "probe.allocation_address",
    )
    dlpack_address = require_exact_int(
        probe["dlpack_data_address"],
        "probe.dlpack_data_address",
    )
    if allocation_address <= 0 or allocation_address != dlpack_address:
        fail("DLPack data does not retain the exact CUDA allocation pointer")
    if not require_exact_bool(probe["pointer_identity"], "probe.pointer_identity"):
        fail("pointer identity was not certified")
    if not require_exact_bool(probe["zero_copy"], "probe.zero_copy"):
        fail("probe did not certify zero-copy identity")
    if require_exact_int(probe["copy_operations"], "probe.copy_operations") != 0:
        fail("DLPack exchange reported a copy")
    for key in ("live_device_bytes_before", "live_device_bytes_after"):
        if require_exact_int(probe[key], f"probe.{key}") != 0:
            fail(f"probe.{key} must be zero")
    if require_exact_bool(
        probe["scientific_result_claimed"],
        "probe.scientific_result_claimed",
    ):
        fail("probe must not claim a scientific result")
    if probe["scientific_public_status"] is not None:
        fail("probe must not expose a scientific public status")

    dlpack = require_mapping(probe["dlpack"], "probe.dlpack")
    require_keys(
        dlpack,
        {
            "device_type",
            "device_type_code",
            "device_id",
            "version_major",
            "version_minor",
            "flags",
            "ndim",
            "shape",
            "strides",
            "dtype",
            "byte_offset",
        },
        "probe.dlpack",
    )
    if dlpack["device_type"] != "kDLCUDA":
        fail("DLTensor is not a CUDA tensor")
    if (
        require_exact_int(dlpack["version_major"], "version_major"),
        require_exact_int(dlpack["version_minor"], "version_minor"),
    ) != (1, 3):
        fail("DLPack capsule does not carry the pinned 1.3 ABI version")
    if require_exact_int(dlpack["flags"], "flags") != 0:
        fail("DLPack producer declared a copy or an unsupported flag")
    if require_exact_int(dlpack["device_type_code"], "device_type_code") != 2:
        fail("DLTensor uses an unexpected device type code")
    if require_exact_int(dlpack["ndim"], "ndim") != 1:
        fail("DLTensor must be one-dimensional")
    if dlpack["shape"] != [PROBE_BYTES] or dlpack["strides"] != [1]:
        fail("DLTensor shape or strides are invalid")
    if require_exact_int(dlpack["byte_offset"], "byte_offset") != 0:
        fail("DLTensor byte_offset must be zero for pointer identity")
    dtype = require_mapping(dlpack["dtype"], "probe.dlpack.dtype")
    expected_dtype = {"code": 1, "bits": 8, "lanes": 1}
    if dict(dtype) != expected_dtype:
        fail(f"DLTensor dtype mismatch: expected {expected_dtype}, got {dict(dtype)}")
    try:
        module.consume_dlpack_capsule(capsule)
    except ValueError as error:
        if "unconsumed" not in str(error):
            fail(f"second DLPack consumption failed ambiguously: {error}")
    else:
        fail("a consumed DLPack capsule was accepted twice")

    foreign_capsule = capsule_new(1, DLPACK_CAPSULE_NAME, None)
    try:
        module.consume_dlpack_capsule(foreign_capsule)
    except ValueError as error:
        if "not the owning" not in str(error):
            fail(f"foreign DLPack capsule failed ambiguously: {error}")
    else:
        fail("a foreign DLPack capsule was accepted as a private Phase 3 export")
    return probe


def validate_external_consumer_lifetime(module: object) -> None:
    capsule = module.make_dlpack_capsule(PROBE_BYTES)
    managed_address = capsule_get_pointer(capsule, DLPACK_CAPSULE_NAME)
    managed = ctypes.cast(
        managed_address,
        ctypes.POINTER(DLManagedTensorVersioned),
    )
    if capsule_set_name(capsule, DLPACK_CONSUMED_CAPSULE_NAME) != 0:
        fail("external consumer could not mark the capsule as consumed")
    del capsule
    gc.collect()
    validate_environment(module, expected_live_bytes=PROBE_BYTES)
    managed.contents.deleter(managed)
    final_environment = validate_environment(module)
    if final_environment["live_device_bytes"] != 0:
        fail("external DLPack consumer deleter did not release device memory")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "module_dir",
        type=pathlib.Path,
        help="Directory containing the compiled morsehgp3d_phase3 extension",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    module_dir = args.module_dir.resolve()
    if not module_dir.is_dir():
        fail(f"module directory does not exist: {module_dir}")
    sys.path.insert(0, str(module_dir))
    module = importlib.import_module(MODULE_NAME)

    environment = validate_environment(module)
    probe = validate_probe(module)
    validate_external_consumer_lifetime(module)
    allocation_limit = require_exact_int(
        environment["allocation_limit_bytes"],
        "allocation_limit_bytes",
    )
    for invalid in (0, -1, allocation_limit + 1, "4096", None):
        expect_invalid_call(module, invalid)
        expect_invalid_capsule_size(module, invalid)
    direct_probe = require_mapping(
        module.dlpack_zero_copy_probe(PROBE_BYTES),
        "dlpack_zero_copy_probe",
    )
    require_keys(
        direct_probe,
        {
            "schema_version",
            "operation",
            "requested_bytes",
            "allocation_limit_bytes",
            "allocation_address",
            "dlpack_data_address",
            "pointer_identity",
            "zero_copy",
            "live_device_bytes_before",
            "peak_live_device_bytes",
            "live_device_bytes_after",
            "dlpack",
            "scientific_result_claimed",
            "scientific_public_status",
        },
        "dlpack_zero_copy_probe",
    )
    if direct_probe.get("zero_copy") is not True:
        fail("direct DLPack probe did not certify zero-copy pointer identity")
    if direct_probe.get("live_device_bytes_after") != 0:
        fail("direct DLPack probe did not release its device allocation")
    if direct_probe.get("scientific_result_claimed") is not False:
        fail("direct DLPack probe claimed a scientific result")
    if direct_probe.get("scientific_public_status") is not None:
        fail("direct DLPack probe exposed a scientific public status")
    orphan = module.make_dlpack_capsule(PROBE_BYTES)
    del orphan
    gc.collect()
    final_environment = validate_environment(module)
    if final_environment["live_device_bytes"] != 0:
        fail("device-memory counter did not return to zero")

    print(
        json.dumps(
            {
                "schema_version": 1,
                "test": "phase3_binding",
                "status": "passed",
                "probe_bytes": probe["requested_bytes"],
                "gpu": final_environment["gpu"]["name"],
                "scientific_result_claimed": False,
            },
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
