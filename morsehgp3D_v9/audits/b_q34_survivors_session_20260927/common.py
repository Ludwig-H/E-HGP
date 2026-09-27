#!/usr/bin/env python3
# Explicit protocol port of b_q34_resident_session_20260927 at 03decc16c.
# The pinned lifecycle helper, guards, fixed target and cooperative join are unchanged.
"""Pure artifact contracts for the isolated S2 CUDA experiment."""
import hashlib
import importlib.util
import json
from pathlib import Path, PurePosixPath
import re
import sys
import tarfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PREFIX = 'morsehgp3D_v9/audits/b_q34_survivors_session_20260927'
PROTOTYPE = 'morsehgp3D_v9/audits/b_q34_survivors_compare_20260927'
SURVIVORS = 'morsehgp3D_v9/audits/b_q34_resident_survivors_20260927'
DIRECT_GATE = 'morsehgp3D_v9/audits/b_q34_survivors_device_gate_20260927'
HELPER = 'gcp-migration/full_probe_session_v7.py'
HELPER_PIN = '177b25a0d72150dc331661fdf8da1ccde77ea17fb694d9c6af5b0929755160d8'
TARGET = dict(project='devpod-gpu-exploration', zone='us-central1-b',
              instance='ehgp-v7-4fa0e0789a7d5bb06b787d35')
DATA_SOURCE = 'morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/full.u32le'
DATA = 'data/scene_00.u32le'
DATA_PIN = '0baa4de14c95838ef7bd18d5a98551ca513ed830ec1eeee84f649fa97c95abaf'
PROVENANCE = 'data/provenance.json'
SCHEMA = 'mhgp9_q34_survivors_session_v1'
SOURCE_PATHS = [
    'morsehgp3D_v9/src', 'morsehgp3D_v9/tests/gen', PROTOTYPE, PREFIX,
    SURVIVORS, DIRECT_GATE,
    *['morsehgp3D_v9/audits/' + name for name in (
        'b_q34_arena_waves_20260927', 'b_q34_collective_arena_20260927',
        'b_q34_cuda_waves_20260927',
        'b_q34_filtered_resident_20260927',
        'b_q34_direct_bands_20260927', 'b_q34_bands_20260927',
        'b_q34_factor_plan_20260926')],
    HELPER,
]


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def unique(pairs):
    out = {}
    for key, value in pairs:
        need(key not in out, 'duplicate JSON key')
        out[key] = value
    return out


def read(path):
    return json.loads(Path(path).read_bytes(), object_pairs_hook=unique)


def save(path, value):
    with Path(path).open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')


def load_legacy(root=ROOT):
    path = root / HELPER
    need(sha(path) == HELPER_PIN, 'lifecycle helper pin')
    spec = importlib.util.spec_from_file_location('cuda_waves_pinned_lifecycle', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    need(module.TARGET == TARGET, 'same fixed target')
    return module


def safe_source(name):
    return any(name == prefix or name.startswith(prefix + '/') for prefix in SOURCE_PATHS)


def unpack_readonly(path, manifest):
    """Inspect every member; never extract links, unknown paths, or data blindly."""
    need(type(manifest) is dict and manifest and all(type(k) is str and
         type(v) is str and re.fullmatch('[0-9a-f]{64}', v) for k, v in manifest.items()), 'manifest form')
    seen, files = set(), {}
    with tarfile.open(path, 'r:*') as archive:
        for item in archive.getmembers():
            name = item.name
            p = PurePosixPath(name)
            need(item.isfile() and name and len(name) <= 4096 and str(p) == name and
                 not p.is_absolute() and all(x not in ('.', '..') for x in p.parts) and
                 name not in seen and (safe_source(name) or name in (DATA, PROVENANCE)), 'unsafe member')
            seen.add(name)
            raw = archive.extractfile(item).read()
            need(manifest.get(name) == hashlib.sha256(raw).hexdigest(), 'member hash')
            files[name] = raw
    need(set(files) == set(manifest), 'exhaustive manifest')
    need(all(name in files for name in (DATA, PROVENANCE, HELPER,
         PREFIX + '/worker.py', PREFIX + '/common.py', PREFIX + '/session.py',
         PREFIX + '/compile_contract.py', PROTOTYPE + '/CMakeLists.txt',
         DIRECT_GATE + '/device_gate.cu', SURVIVORS + '/device_cuda.cu')), 'required members')
    need(len(files[DATA]) == 39885 * 12 and hashlib.sha256(files[DATA]).hexdigest() == DATA_PIN,
         'entire pinned ng00 1mm input')
    need(hashlib.sha256(files[HELPER]).hexdigest() == HELPER_PIN, 'transported lifecycle pin')
    provenance = json.loads(files[PROVENANCE], object_pairs_hook=unique)
    need(type(provenance) is dict and provenance.get('schema') == SCHEMA and
         provenance.get('scope') == 'S2_only_no_FULL' and provenance.get('protocol_source') == 'commit' and
         re.fullmatch('[0-9a-f]{40}', provenance.get('commit', '')) and
         re.fullmatch('[0-9a-f]{40}', provenance.get('tree', '')), 'committed provenance')
    return files, provenance
