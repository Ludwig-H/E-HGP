#!/usr/bin/env python3
"""Lecture seule : empreintes et traces du prototype T2d-C ; aucun moteur execute."""
import argparse
import hashlib
import json
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
SOURCES = [
    'src/catalogue/assemble.cpp', 'src/catalogue/catalogue.hpp',
    'src/catalogue/device_cuda.cu', 'src/catalogue/device_pipeline.hpp',
    'src/catalogue/exec_host.hpp', 'src/catalogue/finish_driver.hpp',
    'src/catalogue/finish_outputs.hpp', 'src/catalogue/finish_repair.hpp',
    'src/catalogue/internal.hpp', 'src/catalogue/module.cmake',
    'src/catalogue/outputs.cpp', 'src/catalogue/transfer_meter.hpp',
    'src/catalogue/finish_level.hpp', 'src/catalogue/finish_kernels.hpp',
    'src/core/buffer.hpp', 'src/core/buffer.cpp', 'src/sched/pool.cpp',
    'tests/catalogue/device_finish_test.cpp', 'tests/catalogue/device_pipeline_test.cpp',
    'tests/catalogue/device_support.hpp', 'tests/catalogue/device_unit.cpp',
    'tests/catalogue/tests.cmake', 'cmake/run_expect.cmake',
]
TRACES = ['b21/Testing/Temporary/LastTest.log', 'b21/CMakeCache.txt',
          'bcu21/CMakeCache.txt', 'bcu21.build.log']


def require(ok, why):
    if not ok:
        raise ValueError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def collect(root):
    files = ['repo/morsehgp3D_v12/' + s for s in SOURCES] + TRACES
    raw = {name: (root / name).read_bytes() for name in files}
    hashes = {name: {'bytes': len(data), 'sha256': sha(data)} for name, data in raw.items()}
    src = {s: raw['repo/morsehgp3D_v12/' + s].decode('utf-8') for s in SOURCES}
    cuda = src['src/catalogue/device_cuda.cu']
    meter = src['src/catalogue/transfer_meter.hpp']
    outputs = src['src/catalogue/finish_outputs.hpp']
    repair = src['src/catalogue/finish_repair.hpp']
    checks = {
        'staging_requested_16_mib': 'kStagingSlots = 2' in meter and
            'kStreamChunk = u64{8} << 20' in meter and
            'stage(kStagingSlots * kStreamChunk)' in cuda,
        'staging_reserved_before_allocation': cuda.index('staging_reservation.reserve(bytes, budget)') <
            cuda.index('cudaMallocHost('),
        'drain_on_stream_failure': 'if (!published.ok()) {\n      (void)cudaStreamSynchronize(stream);' in cuda,
        'event_after_copy': cuda.index('cudaMemcpyAsync(slot(c)') < cuda.index('cudaEventRecord(slot_events[c]'),
        'wait_consume_then_reissue': meter.index('x.wait(slot)') < meter.index('s.consume(s.context') <
            meter.index('if (next(p))'),
        'net_partition_is_wall_minus_publication':
            'meter.ns += wall > published.value() ? wall - published.value() : 0;' in cuda and
            'meter.publish_ns += published.value();' in cuda,
        'prefault_switch_does_not_disable_early_allocation':
            'MHGP12_TRY(out.allocate(n, budget));\n  if constexpr (kPrefaultOutputs)' in outputs,
        'repair_expands_at_non_global_boundary':
            'w = 2 * w < n ? 2 * w : n' in repair and
            'if ((s > lo || lo == 0) && (e < hi || hi == n)) return {};' in repair,
        'repair_covered_skips_duplicate_markers': 'if (i < covered) continue;' in repair and
            'covered = e;' in repair,
    }
    require(all(checks.values()), 'garde textuelle source absente')
    log = raw[TRACES[0]].decode('utf-8')
    tests = []
    for match in re.finditer(r'^\d+/\d+ Testing: (\S+)\n(.*?)(?=^\d+/\d+ Testing: |\Z)', log, re.M | re.S):
        name, block = match.groups()
        require('Test Passed.' in block and 'run_expect_verdict conforme' in block, 'test non conforme: ' + name)
        counts = re.search(r'^test (\S+) controles=(\d+) echecs=(\d+) plancher=(\d+)$', block, re.M)
        row = {'name': name, 'ctest': 'Passed', 'wrapper': 'conforme'}
        if counts:
            require(int(counts[3]) == 0, 'echec de groupe')
            row.update(group=counts[1], controls=int(counts[2]), failures=int(counts[3]), minimum=int(counts[4]))
        else:
            require('inventaire_ok tests=12' in block, 'groupe absent')
            row['inventory_groups'] = 12
        tests.append(row)
    require('End testing:' in log and len(tests) == 13 and len({t['name'] for t in tests}) == 13,
            'cohorte CTest incomplete')
    require('device_open : appareil indisponible (device_unavailable), voie appareil non jouee' in log,
            'portee device_open changee')
    profiles = {}
    for build in ['b21', 'bcu21']:
        cache = raw[build + '/CMakeCache.txt'].decode('utf-8')
        values = {}
        for key in ['CMAKE_BUILD_TYPE', 'MHGP12_COORD_BITS', 'MHGP12_ENABLE_CUDA', 'MHGP12_MARCH', 'MHGP12_MODULES']:
            matches = re.findall(r'^' + key + r':[^=]*=(.*)$', cache, re.M)
            require(len(matches) == 1, 'cache: ' + key)
            values[key] = matches[0]
        home = re.findall(r'^CMAKE_HOME_DIRECTORY:INTERNAL=(.*)$', cache, re.M)
        require(len(home) == 1 and Path(home[0]) == root / 'repo/morsehgp3D_v12', 'source declaree cache differente')
        values['source_directory_matches_prototype'] = True
        profiles[build] = values
    require('[100%] Built target mhgp12' in raw['bcu21.build.log'].decode('utf-8'), 'construction CUDA non terminee')
    for name in files:
        require((root / name).read_bytes() == raw[name], 'artefact modifie pendant lecture: ' + name)
    return {'pins': hashes, 'result': {
        'source_text_checks': checks, 'tests': sorted(tests, key=lambda t: t['name']),
        'ctest_passed': len(tests), 'groups': 12,
        'controls': sum(t.get('controls', 0) for t in tests),
        'cuda_execution': False, 'cuda_library_build_log_complete': True,
        'declared_build_configuration': profiles,
        'audit_executed_native': False, 'audit_compiled': False,
    }}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prototype', type=Path, required=True, help='racine du chantier T2d-C')
    parser.add_argument('--capture', action='store_true', help='capture initiale, avant publication seulement')
    args = parser.parse_args()
    got = collect(args.prototype.resolve())
    target = HERE / 'capture.json'
    if args.capture:
        require(not target.exists(), 'capture deja presente: ne jamais ecraser un recu historique')
        got['scope'] = {'base_declared': '8dc5d6b16', 'product_delivery': False,
            'kind': 'prelecture statique et lecture de traces du developpeur',
            'not_proven': ['chaine de compilation complete', 'execution CUDA', 'gain chronometrique',
                           'integration des budgets separes de main', 'requalification native independante']}
        target.write_text(json.dumps(got, ensure_ascii=False, indent=2, sort_keys=True) + '\n')
    else:
        expected = json.loads(target.read_text())
        require(got['pins'] == expected['pins'], 'empreintes differentes de la capture')
        require(got['result'] == expected['result'], 'lecture differente de la capture')
    print(json.dumps(got['result'], ensure_ascii=False, sort_keys=True))


if __name__ == '__main__':
    main()
