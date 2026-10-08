#!/usr/bin/env python3
"""Lecture seulement des sources et traces Gc ; aucun moteur, build ou CTest."""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tarfile

HERE = Path(__file__).resolve().parent
PIN = '45976be8ddc489f14494937b13e43d0ca5fa0a55'
TOPS = ('CMakeLists.txt', 'cmake', 'src', 'cli', 'bench', 'tests', 'tools', 'reference', 'docs')


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def current_tree(source):
    h = hashlib.sha256()
    n = 0
    for top in TOPS:
        origin = source / top
        paths = [origin] if origin.is_file() else []
        for folder, folders, files in os.walk(origin):
            folders[:] = sorted(f for f in folders if f != '__pycache__')
            paths.extend(Path(folder) / f for f in files if not f.endswith('.pyc'))
        for path in sorted(paths):
            h.update(str(path.relative_to(source)).encode() + b'\0')
            h.update(sha(path.read_bytes()).encode() + b'\n')
            n += 1
    return n, h.hexdigest()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--scratch', required=True, type=Path, help='v12_tour_Gc')
    args = p.parse_args()
    root = args.scratch.resolve()
    capture = json.loads((HERE / 'capture.json').read_text())
    def pins():
        for rel, h in capture['artifact_hashes'].items():
            need(sha((root / rel).read_bytes()) == h, 'artifact changed: ' + rel)
        prefix = '\n'.join((root / 'runs/final_checks.log').read_text().splitlines()[:6]) + '\n'
        need(sha(prefix.encode()) == capture['driver_prefix_sha256'], 'driver prefix changed')
    pins()
    source = root / 'repo3/morsehgp3D_v12'
    archived = subprocess.check_output(['git', 'archive', PIN, 'morsehgp3D_v12'], cwd=root / 'git3')
    with tarfile.open(fileobj=io.BytesIO(archived)) as tar:
        files = {m.name.removeprefix('morsehgp3D_v12/'): tar.extractfile(m).read() for m in tar if m.isfile()}
    for rel, data in files.items():
        if rel.split('/')[0] in TOPS and '/__pycache__/' not in rel and not rel.endswith('.pyc'):
            need((source / rel).read_bytes() == data, 'source changed: ' + rel)
    need(current_tree(source) == (359, capture['source_tree_sha256']), 'source tree')
    cache = (root / 'build_final/CMakeCache.txt').read_text()
    for token in ('CMAKE_BUILD_TYPE:STRING=Release', 'MHGP12_COORD_BITS:STRING=21',
                  'MHGP12_ENABLE_CUDA:BOOL=OFF', 'MHGP12_TSAN:BOOL=OFF',
                  'CMAKE_HOME_DIRECTORY:INTERNAL=' + str(source)):
        need(token in cache, 'configuration: ' + token)
    need('warning:' not in (root / 'build_final.build.log').read_text(), 'build warning')
    cohorts = {}
    for phase, total in (('fast', 675), ('lidar', 6)):
        log = (root / 'runs' / ('final_' + phase + '.log')).read_text()
        cases = re.findall(r'^[ \t]*\d+/\d+ Test\s+#\d+: (\S+)\s+\.+\s*(Passed|\*\*\*Skipped|\*\*\*Failed)\s+',
                           log, re.M)
        need(len(cases) == len(set(n for n, _ in cases)) == total, 'cohort cardinality')
        need(all(status == 'Passed' for _, status in cases), 'cohort failure or skip')
        cohorts[phase] = set(n for n, _ in cases)
    need(cohorts['fast'] & cohorts['lidar'] == {'mhgp12_support_lidar_sentinel'}, 'cohort overlap')
    for name in ('radix_stable', 'index_reference', 'weak_key_resolution', 'support_table', 'duplicate_population'):
        need('mhgp12_tower_index_' + name in cohorts['fast'], 'index gate missing')
    for name in ('oracle', 'scale8000', 'scale16000', 'scale32000'):
        need(all('mhgp12_tower_' + name + opt in cohorts['fast'] for opt in ('', '_opt')), 'scale/oracle missing')
    last = (root / 'build_final/Testing/Temporary/LastTest.log').read_text()
    for line in ('g_determinism_ok cas=lidar_ng00_k5 fils=1,8 empreinte=e5a81154fb1b15f1',
                 'End testing: Oct 08 00:21 UTC'):
        need(line in last, 'LiDAR detailed terminal')
    table = files['src/catalogue/table.cpp'].decode()
    pilot = files['microbancs/mes_t2c_g/pilote_t2c.py'].decode()
    need('if (u64{first} + 1 >= table_.off.size())' in table, 'lookup state changed')
    need('p.add_argument("--processus", type=int, default=8)' in pilot, 'CLI default state changed')
    need('"processus_min": 10' in pilot, 'admission minimum')
    pins()
    need(current_tree(source) == (359, capture['source_tree_sha256']), 'source changed after reading')
    print(json.dumps({'status': 'ok', 'prototype_pin': PIN, 'source_tree_sha256': capture['source_tree_sha256'],
        'source_files': 359, 'bits': 21, 'cuda_enabled': False,
        'fast_passed': 675, 'lidar_passed': 6, 'skipped': 0, 'unique_gates': len(cohorts['fast'] | cohorts['lidar']),
        'artifact_hashes_verified_before_after': len(capture['artifact_hashes']),
        'last_id_guard_adopted': False, 'cli_default_processes': 8, 'admission_minimum_processes': 10,
        'current_mutant_campaign_qualified': False, 'old_24_32_tsan_transferred': False,
        'native_executed_by_auditor': False}, indent=2, sort_keys=True))


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
