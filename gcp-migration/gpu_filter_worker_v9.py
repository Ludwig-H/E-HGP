#!/usr/bin/env python3
"""Worker invite de la sonde S1 GPU v9 (filtre temoin exact q3/q4), inerte sans --execute.

Port du worker de la tour v9 (tower_worker_v9.py), importe comme bibliotheque
et epingle par SHA256 dans le manifeste : cible fixe, garde double, commandes,
trames LiDAR, preflight deterministe et empreintes sont les siens, inchanges.
Ce qui change :
- construction CMake de morsehgp3D_v9 avec -DMHGP9_ENABLE_CUDA=ON (nvcc et
  nvidia-smi existants, aucune installation), cible unique
  mhgp9_gpu_filter_probe ; les unites C++ gardent -Werror ;
- chaque cas lance la sonde S1 (front WSPD q3/q4, rectangles, paires, CPU
  puis GPU) ; sa sortie n'est acceptee que si chaque masque GPU egale le
  masque CPU exhaustif et le cache de ligne CPU donne les memes masques.
  Sans cache GPU, les totaux de visites sont egaux ; avec cache tuile, les
  visites globales et les tests de cache/trace sont publies separement ;
- backend=cuda_g4, GPU_executed vrai des qu'un cas a tourne ; aucune tour
  FULL n'est calculee.

public_status reste not_claimed : un debit mesure ne certifie ni la tour ni
le contrat 1 s / 100 ms.
"""
import argparse
import json
import math
import os
from pathlib import Path
import re
import shlex
import shutil
import signal
import sys
import time

import tower_worker_v9 as base

TARGET = base.TARGET
HELPER, HELPER_SHA = base.HELPER, base.HELPER_SHA
PLAN, PROVENANCE = base.PLAN, base.PROVENANCE
PROVENANCE_SCHEMA = base.PROVENANCE_SCHEMA
PLAN_SCHEMA = 'mhgp9_gpu_filter_plan_v2'
LEGACY_PLAN_SCHEMA = 'mhgp9_gpu_filter_plan_v1'
PROBE_SCHEMA = 'mhgp9_gpu_filter_probe_v3'
LEGACY_PROBE_SCHEMA = 'mhgp9_gpu_filter_probe_v2'
# Le protocole GPU et les modules v9 qu'il importe comme bibliotheques.
PROTOCOL_NAMES = frozenset('gcp-migration/' + name for name in (
    'gpu_filter_worker_v9.py', 'gpu_filter_session_v9.py', 'gpu_filter_snapshot_v9.py',
    'gpu_filter_selftest_v9.py', 'tower_worker_v9.py', 'tower_session_v9.py', 'tower_snapshot_v9.py'))
BASE_WORKER = 'gcp-migration/tower_worker_v9.py'
SOURCE_ROOT = base.SOURCE_ROOT
CMAKE_LISTS = base.CMAKE_LISTS
SOURCE_PREFIXES = base.SOURCE_PREFIXES
PROBE_SOURCE = SOURCE_ROOT + '/bench/gpu_filter_probe.cpp'
KERNEL_SOURCE = SOURCE_ROOT + '/src/gpu/witness_filter.hpp'
RUNNER_SOURCE = SOURCE_ROOT + '/src/gpu/filter_runner.cu'
REQUIRED_SOURCES = frozenset({CMAKE_LISTS, SOURCE_ROOT + '/cmake/run_expect.cmake', PROBE_SOURCE, KERNEL_SOURCE,
                              RUNNER_SOURCE, SOURCE_ROOT + '/src/gpu/filter_runner.hpp',
                              SOURCE_ROOT + '/src/gpu/flat_index.hpp'})
PROBE_TARGET = 'mhgp9_gpu_filter_probe'
INPUTS = base.INPUTS
TIME, BOOST_HEADER, BOOST_ROOT, BUILD_PARALLEL = base.TIME, base.BOOST_HEADER, base.BOOST_ROOT, base.BUILD_PARALLEL
CUDA_PATHS = ('/usr/local/cuda/bin/nvcc', '/usr/local/cuda-12.9/bin/nvcc')
DEVICE_NAME = 'NVIDIA RTX PRO 6000 Blackwell Server Edition'
MAX_RUN_SECONDS, GUEST_SHUTDOWN_MINUTES = base.MAX_RUN_SECONDS, base.GUEST_SHUTDOWN_MINUTES
USEFUL_BUDGET_SECONDS = 1500
CASE_CAP_SECONDS = 600
MIN_CASE_START_SECONDS = 15
OUTCOMES = ('complete', 'gpu_mismatch', 'gpu_fault', 'gpu_unavailable', 'killed_case_cap', 'killed_budget',
            'skipped_budget', 'probe_failed', 'skipped_protocol_defect', 'skipped_s1_gate')
# Worker statuses the host receives and judges (measured results, negative
# ones included); anything else is a worker failure.
RECEIVED_STATUSES = ('completed', 'partial', 'gpu_mismatch', 'gpu_fault', 's1_gate_failed')
# Go/no-go (contre-audit B, 13 h 10) : le premier cas est 08/000000/K5 ; s'il
# n'est pas complet (masques divergents, GPU indisponible, cas tue), les
# suivants ne sont pas payes et sont archives skipped_s1_gate. Un cas complet
# au-dela du seuil laisse mesurer les autres : quelques secondes chacun, et
# les ordres K10 et les autres trames disent quel morceau porter ensuite.
GATE_CASE = dict(scene='00', k=5)
CASE_KEYS = frozenset({'scene', 'file', 'n', 'k', 's', 'workers', 'repeats', 'repeat'})
TILE_CASE_KEYS = CASE_KEYS | {'tile_cache'}
SCOPE = 'S1_exact_q34_witness_filter_cpu_vs_gpu'
# Seuil fixe d'avance (coordination du 23 septembre, 13 h 00 UTC) : toute la
# population de filtrage de 08/000000 a K5, sans cache, en 0,1 s au plus sur
# le GPU, transferts compris (meilleur des passages).
S1_THRESHOLD_MS = 100.0
PREFLIGHT_FILE = base.PREFLIGHT_FILE

TOP_KEYS = frozenset({'schema', 'input', 'options', 'population', 'cpu', 'gpu', 'peak_rss_kb'})
INPUT_KEYS = frozenset({'sites', 'hash'})
OPTION_KEYS = frozenset({'K', 's', 'workers', 'repeats', 'cpu_only', 'inject'})
TILE_OPTION_KEYS = OPTION_KEYS | {'tile_cache'}
INJECT = 'pair_mask'  # mutant causal du juge de la sonde (preflight_mutant)
POPULATION_KEYS = ('rectangles', 'rectangle_survivors', 'pairs', 'pair_survivors', 'q3_rejected', 'q3_open',
                   'q4_rejected', 'q4_open', 'front_product_visits')
CPU_TIMES = ('index_ms', 'front_ms', 'rect_ms', 'pair_nocache_ms', 'pair_cache_ms')
CPU_COUNTS = ('rect_visits', 'pair_visits', 'cache_searches', 'cache_mismatches')
GPU_TIMES = ('upload_ms', 'rect_ms', 'scan_ms', 'pair_ms', 'download_ms', 'total_ms', 'first_total_ms')
GPU_COUNTS = ('pairs', 'rect_visits', 'pair_visits', 'rect_mismatches', 'pair_mismatches')
GPU_TILE_COUNTS = ('cache_node_tests', 'trace_node_tests', 'representatives', 'tiles')
GPU_V3_COUNTS = GPU_TILE_COUNTS + ('repeat_mismatches',)
GPU_PASS_TIMES = tuple(key for key in GPU_TIMES if key != 'first_total_ms')
GPU_OTHER = ('available', 'device', 'error', 'stack_failure', 'visits_equal')
# Rapprochement des chronos d'evenements CUDA (resolution ~0,5 us).
EVENT_TOLERANCE_MS = 0.05

need, sha, save, strict_json, canonical_json = base.need, base.sha, base.save, base.strict_json, base.canonical_json


class BuildFailed(Exception):
    pass


class PreflightFailed(Exception):
    pass


def safe_name(name):
    return base.safe_name(name) or (type(name) is str and name in PROTOCOL_NAMES)


def validate_manifest(manifest):
    need(type(manifest) is dict and manifest, 'payload manifest object')
    for name, pin in manifest.items():
        need(safe_name(name) and type(pin) is str and re.fullmatch('[0-9a-f]{64}', pin), 'unsafe payload path/hash')
    need(REQUIRED_SOURCES | PROTOCOL_NAMES | {HELPER, PLAN, PROVENANCE} |
         {data['file'] for data in INPUTS.values()} <= set(manifest), 'required payload absent')
    need(manifest[HELPER] == HELPER_SHA, 'legacy helper pin')
    need(all(manifest[data['file']] == data['sha256'] for data in INPUTS.values()), 'pinned LiDAR frame hash')


def validate_sources(read_bytes, tile_cache=False):
    cmake = read_bytes(CMAKE_LISTS)
    probe = read_bytes(PROBE_SOURCE)
    need(b'mhgp9_product_executable(' + PROBE_TARGET.encode() + b' bench/gpu_filter_probe.cpp)' in cmake and
         b'option(MHGP9_ENABLE_CUDA' in cmake and b'src/gpu/filter_runner.cu' in cmake,
         'CMake GPU probe target/option absent')
    need(any(schema.encode() in probe for schema in (PROBE_SCHEMA, LEGACY_PROBE_SCHEMA)) and
         all(token in probe for token in (b'"--s="', b'"--repeats="', b'"--cpu-only"',
                                          b'"--inject=' + INJECT.encode() + b'"', b'\\"inject\\"')),
         'GPU probe schema/CLI differs from the S1 protocol')
    need(not tile_cache or all(token in probe for token in (PROBE_SCHEMA.encode(), b'"--tile-cache"',
                                                           b'\\"tile_cache\\"')),
         'tile plan requires the v3 probe and its explicit tile-cache option')


def _integer(value, low, high):
    return type(value) is int and low <= value <= high


def validate_plan(plan, manifest):
    need(type(plan) is dict and set(plan) == {'schema', 'cases'} and
         plan['schema'] in (PLAN_SCHEMA, LEGACY_PLAN_SCHEMA) and
         type(plan['cases']) is list and 0 < len(plan['cases']) <= 16, 'GPU plan schema')
    tile_plan = plan['schema'] == PLAN_SCHEMA
    need(type(plan['cases'][0]) is dict and all(plan['cases'][0].get(key) == value for key, value in GATE_CASE.items()),
         'the first case is the S1 gate case 08/000000 (scene 00) at K5')
    seen = set()
    for case in plan['cases']:
        need(type(case) is dict and set(case) == (TILE_CASE_KEYS if tile_plan else CASE_KEYS), 'GPU case fields')
        need(not tile_plan or type(case['tile_cache']) is bool, 'GPU tile_cache flag')
        need(type(case['scene']) is str and case['scene'] in INPUTS and
             case['file'] == INPUTS[case['scene']]['file'] and case['file'] in manifest, 'GPU scene/file identity')
        need(case['n'] == INPUTS[case['scene']]['n'] and type(case['n']) is int, 'whole-frame size; prefixes forbidden')
        need(type(case['k']) is int and case['k'] in (5, 10) and type(case['s']) is int and
             case['s'] in ((8, 10, 12) if tile_plan else (8,)) and
             _integer(case['workers'], 1, 48) and _integer(case['repeats'], 1, 10) and
             _integer(case['repeat'], 0, (1 << 32) - 1), 'GPU case domain')
        identity = tuple(case[key] for key in ('scene', 'k', 's', 'workers', 'repeats', 'repeat')) + (case.get('tile_cache', False),)
        need(identity not in seen, 'duplicate case needs an explicit distinct repetition')
        seen.add(identity)
    return plan['cases']


def validate_runtime(manifest):
    # Les deux executables reels (worker.py et sa bibliotheque) sont
    # televerses hors de source/ : les epingler contre leur copie archivee.
    need(sha(__file__) == manifest.get('gcp-migration/gpu_filter_worker_v9.py') and
         sha(base.__file__) == manifest.get(BASE_WORKER), 'executing worker/library differs from transported sources')


def probe_command(build, root, case, inject=''):
    return [str(build / PROBE_TARGET), str(root / case['file']), *expected_probe_tail(case, inject)]


def expected_probe_tail(case, inject=''):
    tail = [str(case['k']), str(case['workers']), '--s=' + str(case['s']), '--repeats=' + str(case['repeats'])]
    if case.get('tile_cache', False):
        tail.append('--tile-cache')
    return tail + (['--inject=' + inject] if inject else [])


def preflight_case(raw, tile_cache=False):
    """Preflight : la sonde reelle, CPU puis GPU, nuage deterministe, K5, deux fils, un passage."""
    return dict(scene='preflight', file=PREFLIGHT_FILE, n=len(raw) // 12, k=5, s=8, workers=2, repeats=1, repeat=0,
                tile_cache=tile_cache)


def _count(value):
    return type(value) is int and 0 <= value < (1 << 63)


def _number(value):
    return type(value) in (int, float) and math.isfinite(value) and value >= 0


def validate_probe(value, case, exit_code, inputs=None, inject=''):
    """Lit la sortie S1 ; rend complete, gpu_mismatch ou gpu_unavailable.

    Toute sortie malformee, incoherente ou d'un domaine different est
    refusee (ValueError) : c'est un defaut de protocole, pas une mesure."""
    inputs = INPUTS if inputs is None else inputs
    need(type(value) is dict and set(value) == TOP_KEYS and
         value['schema'] in (PROBE_SCHEMA, LEGACY_PROBE_SCHEMA), 'S1 probe top-level schema')
    tile_schema = value['schema'] == PROBE_SCHEMA
    tile_cache = case.get('tile_cache', False)
    need(not tile_cache or tile_schema, 'tile cache requires probe v3')
    need(type(value['input']) is dict and set(value['input']) == INPUT_KEYS and
         type(value['input']['sites']) is int and value['input']['sites'] == case['n'] and
         value['input']['hash'] == inputs[case['scene']]['fnv'],
         'S1 probe input identity')
    options = value['options']
    need(type(options) is dict and set(options) == (TILE_OPTION_KEYS if tile_schema else OPTION_KEYS) and
         all(type(options[key]) is int for key in ('K', 's', 'workers', 'repeats')) and
         options['K'] == case['k'] and
         options['s'] == case['s'] and options['workers'] == case['workers'] and
         options['repeats'] == case['repeats'] and options['cpu_only'] is False and options['inject'] == inject,
         'S1 probe options')
    need(not tile_schema or type(options['tile_cache']) is bool and options['tile_cache'] is tile_cache,
         'S1 tile cache option identity')
    population = value['population']
    need(type(population) is dict and set(population) == set(POPULATION_KEYS) and
         all(_count(population[key]) for key in POPULATION_KEYS), 'S1 population fields')
    need(population['rectangle_survivors'] <= population['rectangles'] and
         population['pair_survivors'] <= population['pairs'] and
         population['q3_rejected'] + population['q3_open'] <= population['pairs'] and
         population['q4_rejected'] + population['q4_open'] <= population['pairs'] and
         population['rectangles'] > 0 and population['pairs'] > 0, 'S1 population identities')
    cpu = value['cpu']
    need(type(cpu) is dict and set(cpu) == set(CPU_TIMES) | set(CPU_COUNTS) and
         all(_number(cpu[key]) for key in CPU_TIMES) and all(_count(cpu[key]) for key in CPU_COUNTS), 'S1 CPU fields')
    need(cpu['cache_mismatches'] == 0 and cpu['cache_searches'] <= population['pairs'] and
         cpu['pair_visits'] >= population['pairs'], 'S1 CPU reference consistency')
    gpu = value['gpu']
    counts = GPU_COUNTS + (GPU_V3_COUNTS if tile_schema else ())
    other = set(GPU_OTHER) | ({'passes'} if tile_schema else set())
    need(type(gpu) is dict and set(gpu) == set(GPU_TIMES) | set(counts) | other and
         all(_number(gpu[key]) for key in GPU_TIMES) and all(_count(gpu[key]) for key in counts) and
         type(gpu['available']) is bool and type(gpu['device']) is str and type(gpu['error']) is str and
         type(gpu['stack_failure']) is bool and type(gpu['visits_equal']) is bool, 'S1 GPU fields')
    need(not tile_schema or type(gpu['passes']) is list, 'S1 GPU passes array')
    need(_count(value['peak_rss_kb']) and value['peak_rss_kb'] > 0, 'S1 peak RSS')
    if exit_code == 3:
        # No device at all is unavailability; a CUDA or host error on a
        # present device is a fault of the pass under test.
        need(not gpu['available'] or gpu['error'] != '', 'exit 3 without a GPU failure')
        return 'gpu_unavailable' if not gpu['available'] else 'gpu_fault'
    need(gpu['available'] and gpu['error'] == '' and gpu['device'] == DEVICE_NAME, 'S1 GPU identity/availability')
    if tile_schema:
        need((tile_cache or all(gpu[key] == 0 for key in GPU_TILE_COUNTS)) and
             gpu['representatives'] == gpu['tiles'] <= gpu['pairs'] and
             gpu['representatives'] <= gpu['trace_node_tests'] <= gpu['pair_visits'], 'S1 tile work counters')
        need(not tile_cache or case['scene'] == 'preflight' or gpu['tiles'] > 0,
             'whole-frame tile cache experiment must be non-vacuous')
    visits_equal = gpu['rect_visits'] == cpu['rect_visits'] and gpu['pair_visits'] == cpu['pair_visits']
    need(gpu['visits_equal'] is visits_equal, 'S1 visits_equal must report actual equality')
    parts = gpu['upload_ms'] + gpu['rect_ms'] + gpu['scan_ms'] + gpu['pair_ms'] + gpu['download_ms']
    need(abs(parts - gpu['total_ms']) <= EVENT_TOLERANCE_MS and gpu['total_ms'] <= gpu['first_total_ms'] + 1e-9 and
         gpu['total_ms'] > 0, 'S1 GPU event timings')
    if tile_schema:
        need(len(gpu['passes']) == case['repeats'], 'S1 all GPU repetitions must be recorded')
        for row in gpu['passes']:
            need(type(row) is dict and set(row) == set(GPU_PASS_TIMES) and
                 all(_number(row[key]) for key in GPU_PASS_TIMES) and row['total_ms'] > 0 and
                 abs(sum(row[key] for key in GPU_PASS_TIMES if key != 'total_ms') - row['total_ms']) <=
                 EVENT_TOLERANCE_MS, 'S1 pass event timings')
        need(gpu['first_total_ms'] == gpu['passes'][0]['total_ms'] and
             gpu['total_ms'] == min(row['total_ms'] for row in gpu['passes']) and
             any(all(gpu[key] == row[key] for key in GPU_PASS_TIMES) for row in gpu['passes']),
             'S1 selected timing must be one actual minimum-total pass')
    # A stack-bound violation on the device is a divergence: the CPU passed
    # the same queries under the same proved bound.
    agree = (not gpu['stack_failure'] and gpu['pairs'] == population['pairs'] and gpu['rect_mismatches'] == 0 and
             gpu['pair_mismatches'] == 0 and
             gpu['rect_visits'] == cpu['rect_visits'] and (tile_cache or visits_equal) and
             (not tile_schema or gpu['repeat_mismatches'] == 0))
    if inject:
        # The flipped mask of the causal mutant: exactly one pair differs.
        need(exit_code == 1 and not gpu['stack_failure'] and gpu['pairs'] == population['pairs'] and
             gpu['rect_mismatches'] == 0 and
             gpu['pair_mismatches'] == 1 and gpu['rect_visits'] == cpu['rect_visits'] and
             (tile_cache or visits_equal) and (not tile_schema or gpu['repeat_mismatches'] == 0),
             'the pair_mask mutant was not detected exactly once')
        return 'gpu_mismatch'
    if exit_code == 1:
        need(not agree, 'exit 1 while masks and required counters agree')
        return 'gpu_mismatch'
    need(exit_code == 0 and agree, 'S1 exit 0 requires identical masks and required counters')
    return 'complete'


def compiled_dependencies(build, root, before):
    consumed, relative_seen = {}, set()
    depfiles = sorted(build.glob('CMakeFiles/*.dir/**/*.o.d'))
    need(depfiles, 'no compiler depfile under the CMake build')
    for dep in depfiles:
        text = dep.read_text().replace('\\\n', ' ')
        need(':' in text, 'malformed depfile')
        for name in shlex.split(text.split(':', 1)[1]):
            path = Path(name) if Path(name).is_absolute() else build / name
            path = path.resolve()
            if path.is_relative_to(root):
                relative = str(path.relative_to(root))
                need(relative in before and sha(path) == before[relative], 'unmanifested compilation dependency')
                relative_seen.add(relative)
            consumed[str(path)] = sha(path)
    need({PROBE_SOURCE, KERNEL_SOURCE} <= relative_seen, 'depfiles lack the GPU probe/kernel sources')
    return consumed


def execute(args):
    root, output = args.source_root.resolve(), args.output.absolute()
    need(1 <= args.useful_budget_seconds <= USEFUL_BUDGET_SECONDS and
         1 <= args.case_cap_seconds <= int(MAX_RUN_SECONDS) and args.closing_margin_seconds >= 300, 'cost/closing budget')
    need(not output.exists() and not output.is_symlink() and not output.resolve().is_relative_to(root), 'fresh output')
    output.mkdir(mode=0o700)
    result = dict(status='failed', backend='cuda_g4', scope=SCOPE, public_status='not_claimed',
                  GPU_attempted=False, GPU_executed=False, FULL_executed=False, contract_certified=False,
                  targeted_GCP_stop_required_by_ROOT=True, worker_argv=list(args.argv), worker_sha256=sha(__file__),
                  base_worker_sha256=sha(base.__file__), useful_budget_seconds=args.useful_budget_seconds,
                  case_cap_seconds=args.case_cap_seconds, s1_threshold_ms=S1_THRESHOLD_MS)
    worker, manifest, before, consumed = None, None, None, {}
    tools, binary, binary_sha = {}, None, None
    began = time.time()

    def interrupted(signum, _frame):
        raise InterruptedError('guest signal ' + str(signum))
    previous = {signum: signal.signal(signum, interrupted) for signum in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP)}
    try:
        helper = base.load_helper(root)
        need(sha(args.guard_mark) == args.guard_mark_sha256, 'guard pin')
        mark, schedule = helper.fields(args.guard_mark.read_text()), helper.fields(helper.scheduled_text())
        target = {key: getattr(args, key) for key in TARGET}
        need(target == TARGET and mark.get('max_run_seconds') == MAX_RUN_SECONDS and
             mark.get('guest_shutdown_minutes') == GUEST_SHUTDOWN_MINUTES, 'fixed target and dual-guard duration')
        guards = helper.guard_values(mark, schedule, target, args.generation, args.session_deadline_epoch,
                                     args.closing_margin_seconds, time.time())
        observed = helper.metadata()
        need(all(observed[key] == value for key, value in TARGET.items()) and
             observed['machine'] == 'g4-standard-48', 'guest target/type')
        need(abs(base.boot_epoch() - helper.epoch(args.generation)) <= 300, 'guest boot/generation')
        cpus = base.available_cpus()
        need(len(cpus) == 48, '48 available vCPUs required')
        useful_deadline = min(began + args.useful_budget_seconds, guards['work_deadline_epoch'])
        worker = helper.Worker(output, useful_deadline, schedule)
        result.update(target=target, generation=args.generation, available_cpus=cpus, guards=guards,
                      useful_deadline_epoch=useful_deadline)
        save(output / 'guard_evidence.json', dict(mark=mark, schedule=schedule, metadata=observed))
        need(sha(args.source_manifest) == args.source_manifest_sha256, 'manifest pin')
        manifest = strict_json(args.source_manifest.read_bytes())
        validate_manifest(manifest)
        validate_runtime(manifest)
        result['source_manifest_sha256'] = args.source_manifest_sha256
        before = source_map(root, manifest)
        save(output / 'sources_before.json', before)
        read = lambda name: (root / name).read_bytes()
        validate_sources(read)
        plan = strict_json(read(PLAN))
        cases = validate_plan(plan, manifest)
        validate_sources(read, tile_cache=any(case.get('tile_cache', False) for case in cases))
        base.validate_data(read)
        result['provenance'] = base.validate_provenance(strict_json(read(PROVENANCE)), manifest)
        result.update(plan_schema=plan['schema'], cases=cases, case_outcomes=[], completed_case_indices=[])
        nvcc = next((p for p in CUDA_PATHS if Path(p).is_file() and os.access(p, os.X_OK)), None)
        tools = {'g++': shutil.which('g++'), 'cmake': shutil.which('cmake'), 'nvcc': nvcc,
                 'nvidia-smi': shutil.which('nvidia-smi')}
        need(all(tools.values()) and Path(TIME).is_file() and Path(BOOST_HEADER).is_file(),
             'g++, cmake, nvcc, nvidia-smi, GNU time and system Boost required; no automatic installation')
        result['CUDA_installation_attempted'] = False
        result['tool_paths'] = dict(tools, time=TIME, boost_header=BOOST_HEADER)
        result['tool_sha256'] = {name: sha(path) for name, path in tools.items()}
        for name, command in [('compiler', [tools['g++'], '--version']), ('cmake_version', [tools['cmake'], '--version']),
                              ('nvcc_version', [tools['nvcc'], '--version']), ('time_version', [TIME, '--version']),
                              ('gpu_inventory', [tools['nvidia-smi'],
                                                 '--query-gpu=name,driver_version,memory.total,compute_cap',
                                                 '--format=csv,noheader']),
                              ('lscpu', ['lscpu']), ('nproc', ['nproc'])]:
            need(worker.command(name, command)['exit_code'] == 0, name)
        need(DEVICE_NAME in (output / 'gpu_inventory.stdout').read_text(), 'G4 GPU inventory')
        for filename in ('/etc/os-release', '/proc/meminfo'):
            with (output / (Path(filename).name + '.txt')).open('x') as stream:
                stream.write(Path(filename).read_text())
        build = output / 'build'
        configure = worker.command('configure', configure_command(tools, root, build))
        if configure['exit_code'] != 0:
            raise BuildFailed('cmake configure failed; logs in configure.stdout/stderr')
        compiled = worker.command('build', [tools['cmake'], '--build', str(build), '--target', PROBE_TARGET,
                                            '--parallel', BUILD_PARALLEL])
        if compiled['exit_code'] != 0:
            raise BuildFailed('strict build of mhgp9_gpu_filter_probe failed; logs in build.stdout/stderr')
        binary = build / PROBE_TARGET
        need(binary.is_file() and not binary.is_symlink(), 'probe binary absent after build')
        binary_sha = sha(binary)
        consumed = compiled_dependencies(build, root, before)
        save(output / 'compiled_dependencies.json', consumed)
        result.update(binary_sha256=binary_sha,
                      compiled_dependency_manifest_sha256=sha(output / 'compiled_dependencies.json'))
        pre_raw = base.preflight_cloud()
        with (output / PREFLIGHT_FILE).open('xb') as stream:
            stream.write(pre_raw)
        pre_case = preflight_case(pre_raw)
        # Attempted as soon as the device probe runs, validated or not.
        result['GPU_attempted'] = True
        pre_row = worker.command('preflight', [TIME, '-v', str(binary), str(output / PREFLIGHT_FILE),
                                               *expected_probe_tail(pre_case)])
        mutant_row = worker.command('preflight_mutant', [TIME, '-v', str(binary), str(output / PREFLIGHT_FILE),
                                                         *expected_probe_tail(pre_case, INJECT)])
        try:
            pre_value = strict_json((output / 'preflight.stdout').read_bytes())
            need(validate_probe(pre_value, pre_case, pre_row['exit_code'],
                                inputs=base.preflight_inputs(pre_raw)) == 'complete', 'preflight probe not complete')
            base.validate_gnu_time((output / 'preflight.stderr').read_text(errors='replace'), 0)
            mutant_value = strict_json((output / 'preflight_mutant.stdout').read_bytes())
            need(validate_probe(mutant_value, pre_case, mutant_row['exit_code'], inputs=base.preflight_inputs(pre_raw),
                                inject=INJECT) == 'gpu_mismatch', 'causal mutant not killed')
            base.validate_gnu_time((output / 'preflight_mutant.stderr').read_text(errors='replace'), 1)
        except (ValueError, KeyError, TypeError, UnicodeError) as error:
            raise PreflightFailed(type(error).__name__ + ': ' + str(error)) from error
        result['GPU_executed'] = True
        result['preflight'] = dict(sites=pre_case['n'], pairs=pre_value['population']['pairs'],
                                   gpu_total_ms=pre_value['gpu']['total_ms'])
        if any(case.get('tile_cache', False) for case in cases):
            tile_case = preflight_case(pre_raw, tile_cache=True)
            tile_row = worker.command('preflight_tile', [TIME, '-v', str(binary), str(output / PREFLIGHT_FILE),
                                                         *expected_probe_tail(tile_case)])
            tile_mutant_row = worker.command('preflight_tile_mutant',
                                            [TIME, '-v', str(binary), str(output / PREFLIGHT_FILE),
                                             *expected_probe_tail(tile_case, INJECT)])
            try:
                tile_value = strict_json((output / 'preflight_tile.stdout').read_bytes())
                need(validate_probe(tile_value, tile_case, tile_row['exit_code'],
                                    inputs=base.preflight_inputs(pre_raw)) == 'complete',
                     'tile preflight probe not complete')
                base.validate_gnu_time((output / 'preflight_tile.stderr').read_text(errors='replace'), 0)
                tile_mutant = strict_json((output / 'preflight_tile_mutant.stdout').read_bytes())
                need(validate_probe(tile_mutant, tile_case, tile_mutant_row['exit_code'],
                                    inputs=base.preflight_inputs(pre_raw), inject=INJECT) == 'gpu_mismatch',
                     'tile causal mutant not killed')
                base.validate_gnu_time((output / 'preflight_tile_mutant.stderr').read_text(errors='replace'), 1)
                need(tile_value['population'] == pre_value['population'], 'preflight tile population differs')
            except (ValueError, KeyError, TypeError, UnicodeError) as error:
                raise PreflightFailed(type(error).__name__ + ': ' + str(error)) from error
            result['preflight_tile'] = dict(sites=tile_case['n'], pairs=tile_value['population']['pairs'],
                                            gpu_total_ms=tile_value['gpu']['total_ms'])

        def left():
            try:
                return worker.remaining()
            except helper.SessionDeadline:
                return 0.0
        outcomes, exhausted, defect, gated = result['case_outcomes'], False, False, False
        for index, case in enumerate(cases):
            suffix = str(index)
            if defect:
                outcomes.append(dict(index=index, outcome='skipped_protocol_defect'))
                continue
            # Precedence, mirrored by the host reception: an exhausted budget,
            # then the S1 gate, then too little time left for a new case.
            if exhausted:
                outcomes.append(dict(index=index, outcome='skipped_budget'))
                continue
            if gated:
                outcomes.append(dict(index=index, outcome='skipped_s1_gate'))
                continue
            if left() < MIN_CASE_START_SECONDS:
                exhausted = True
                outcomes.append(dict(index=index, outcome='skipped_budget'))
                continue
            need(worker.command('uptime_before_' + suffix, ['uptime'])['exit_code'] == 0, 'uptime')
            started = time.time()
            capped = started + args.case_cap_seconds < useful_deadline
            case_worker = helper.Worker(output, min(started + args.case_cap_seconds, useful_deadline), schedule)
            name = 'probe_' + suffix
            killed, row = False, None
            try:
                row = case_worker.command(name, [TIME, '-v', *probe_command(build, root, case)])
            except helper.SessionDeadline:
                killed = True
            finally:
                worker.commands.extend(case_worker.commands)
            if killed:
                if not case_worker.commands:
                    exhausted = True
                    outcomes.append(dict(index=index, outcome='skipped_budget'))
                    continue
                row = case_worker.commands[-1]
                need(row.get('group_closed') is True, 'killed case process group not certified closed')
                outcomes.append(dict(index=index, outcome='killed_case_cap' if capped else 'killed_budget',
                                     exit_code=row['exit_code'], elapsed_seconds=row['elapsed_seconds']))
                exhausted = not capped
            else:
                entry = dict(index=index, exit_code=row['exit_code'], elapsed_seconds=row['elapsed_seconds'])
                try:
                    need(row['exit_code'] in (0, 1, 3), 'probe exit code outside {0, 1, 3}')
                    value = strict_json((output / (name + '.stdout')).read_bytes())
                    outcome = validate_probe(value, case, row['exit_code'])
                    rss = base.validate_gnu_time((output / (name + '.stderr')).read_text(errors='replace'),
                                                 row['exit_code'])
                    entry.update(outcome=outcome, gnu_time_max_rss_kb=rss, pairs=value['population']['pairs'],
                                 gpu_total_ms=value['gpu']['total_ms'],
                                 cpu_filter_cache_ms=value['cpu']['rect_ms'] + value['cpu']['pair_cache_ms'])
                except (ValueError, KeyError, TypeError, UnicodeError) as error:
                    entry.update(outcome='probe_failed', reason=type(error).__name__ + ': ' + str(error))
                    defect = True
                outcomes.append(entry)
                need(sha(binary) == binary_sha, 'binary changed')
            save(output / (name + '.summary.json'), dict(case=case, input_file_sha256=manifest[case['file']],
                                                         **outcomes[-1]))
            gated = index == 0 and outcomes[-1]['outcome'] != 'complete'
            if not exhausted and left() >= 5:
                need(worker.command('uptime_after_' + suffix, ['uptime'])['exit_code'] == 0, 'uptime')
        result['completed_case_indices'] = [e['index'] for e in outcomes if e['outcome'] == 'complete']
        if any(e['outcome'] == 'probe_failed' for e in outcomes):
            result['status'] = 'probe_failed'
        else:
            result['status'] = campaign_status(outcomes)
            if result['status'] == 'no_gpu_measurement':
                result['error'] = 'no GPU case measured within the useful budget'
    except BuildFailed as error:
        result.update(status='build_failed', error=str(error))
    except PreflightFailed as error:
        result.update(status='preflight_failed', error=str(error))
    except BaseException as error:
        result.update(status='failed', error=type(error).__name__ + ': ' + str(error))
    finally:
        result['commands'] = worker.commands if worker else []
        result['elapsed_before_closure_seconds'] = time.time() - began
        try:
            if before is not None:
                after = source_map(root, manifest)
                save(output / 'sources_after.json', after)
                result['sources_stable'] = before == after
            if binary_sha is not None:
                result['binary_stable'] = sha(binary) == binary_sha
                need(result['binary_stable'], 'binary changed at closure')
            for name, pin in result.get('tool_sha256', {}).items():
                need(sha(tools[name]) == pin, 'tool changed: ' + name)
            for name, pin in consumed.items():
                need(sha(name) == pin, 'compiled dependency changed at closure')
            result['compiled_dependencies_stable'] = bool(consumed)
        except Exception as error:
            result.update(status='failed', closure_error=type(error).__name__ + ': ' + str(error))
        save(output / 'receipt.json', result)
        print(json.dumps(dict(status=result['status'], targeted_GCP_stop_required_by_ROOT=True)), flush=True)
        for signum, handler in previous.items():
            signal.signal(signum, handler)
    # Negative measured results (mismatch, fault, closed S1 gate) are received and judged by the host.
    return 0 if result['status'] in RECEIVED_STATUSES else 1


def campaign_status(outcomes):
    """Status of a campaign without protocol defects, recomputed identically by the host.

    Precedence: a device fault, then a divergence, then a closed S1 gate (the
    gate case killed at its cap or without a device), then nothing measured,
    then completed/partial."""
    names = [entry['outcome'] for entry in outcomes]
    if 'gpu_fault' in names:
        return 'gpu_fault'
    if 'gpu_mismatch' in names:
        return 'gpu_mismatch'
    if names and names[0] in ('killed_case_cap', 'gpu_unavailable'):
        return 's1_gate_failed'
    if 'complete' not in names:
        return 'no_gpu_measurement'
    return 'completed' if all(name == 'complete' for name in names) else 'partial'


def configure_command(tools, root, build):
    """Construction stricte : -Werror des unites C++ intact, CUDA active, nvcc explicite."""
    return [tools['cmake'], '-S', str(root / SOURCE_ROOT), '-B', str(build), '-DCMAKE_BUILD_TYPE=Release',
            '-DBOOST_ROOT=' + BOOST_ROOT, '-DCMAKE_CXX_COMPILER=' + tools['g++'], '-DMHGP9_ENABLE_CUDA=ON',
            '-DCMAKE_CUDA_COMPILER=' + tools['nvcc']]


def source_map(root, manifest):
    validate_manifest(manifest)
    out = {}
    for name, pin in manifest.items():
        path = root / name
        need(path.is_file() and not path.is_symlink() and path.resolve().is_relative_to(root), 'payload file type')
        out[name] = sha(path)
        need(out[name] == pin, 'payload changed: ' + name)
    return out


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('source-root', 'source-manifest', 'guard-mark', 'output'):
        parser.add_argument('--' + key, type=Path)
    for key in ('source-manifest-sha256', 'guard-mark-sha256', 'project', 'zone', 'instance', 'generation'):
        parser.add_argument('--' + key)
    parser.add_argument('--session-deadline-epoch', type=int)
    parser.add_argument('--closing-margin-seconds', type=int, default=300)
    parser.add_argument('--useful-budget-seconds', type=int, default=USEFUL_BUDGET_SECONDS)
    parser.add_argument('--case-cap-seconds', type=int, default=CASE_CAP_SECONDS)
    parser.add_argument('--execute', action='store_true')
    args = parser.parse_args(argv)
    if not args.execute:
        print(json.dumps(dict(status='inert', target=TARGET, backend='cuda_g4', scope=SCOPE,
                              useful_budget_seconds=USEFUL_BUDGET_SECONDS, case_cap_seconds=CASE_CAP_SECONDS,
                              GPU_executed=False, FULL_executed=False), sort_keys=True))
        return 0
    need(all(getattr(args, key.replace('-', '_')) is not None for key in (
        'source-root', 'source-manifest', 'guard-mark', 'output', 'source-manifest-sha256',
        'guard-mark-sha256', 'project', 'zone', 'instance', 'generation', 'session-deadline-epoch')), 'missing arguments')
    args.argv = ['worker.py', *argv]
    return execute(args)


if __name__ == '__main__':
    raise SystemExit(main())
