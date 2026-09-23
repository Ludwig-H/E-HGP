#!/usr/bin/env python3
"""Worker invite de la tour FULL v9 (reference_cpu), inerte sans --execute.

Port explicite de q34_spatial_worker_v8.py (SHA 39596c7607370e27...,
commit 70de84f2, lui-meme port de cpu_probe_worker_v8.py) : memes primitives
de garde et de commande, reprises du worker v7 epingle par SHA256
(gcp-migration/full_probe_worker_v7.py, da967163...). Son producteur FULL v7, sa recette de compilation et son
lecteur ne sont jamais appeles ; le fichier v7 n'est pas modifie.

Ce qui change pour la v9 :
- la construction passe par le CMake de morsehgp3D_v9 (cible unique
  mhgp9_tower_probe), sans modifier le CMakeLists ni affaiblir -Werror ;
- les entrees sont les trois trames LiDAR sans sol a 1 mm, fichiers entiers
  epingles (sha256, taille, empreinte FNV-1a de la sonde) ;
- chaque cas du plan a un plafond propre ; un cas qui l'atteint est tue et
  consigne, les suivants sont sautes quand le budget utile est epuise.

Aucune installation de paquet, aucun reboot, aucune mutation cloud, aucun
CUDA, aucun ELF local transporte. public_status reste not_claimed : une
mesure, meme complete, ne certifie ni la tour ni le contrat 1 s / 100 ms.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path, PurePosixPath
import re
import shlex
import shutil
import signal
import struct
import sys
import time
import types

TARGET = dict(project='devpod-gpu-exploration', zone='us-central1-b',
              instance='ehgp-v7-4fa0e0789a7d5bb06b787d35')
HELPER = 'gcp-migration/full_probe_worker_v7.py'
HELPER_SHA = 'da967163bdb7247bc6aad4df0c294cda1071076a0127cd5bd9f59bc0e4788439'
PLAN = 'data/session_plan.json'
PROVENANCE = 'data/provenance.json'
PLAN_SCHEMA = 'mhgp9_tower_plan_v1'
PROVENANCE_SCHEMA = 'mhgp9_tower_provenance_v1'
PROBE_SCHEMA = 'mhgp9_tower_probe_v3'
PROTOCOL_NAMES = frozenset('gcp-migration/tower_' + name + '_v9.py' for name in
                           ('worker', 'session', 'snapshot', 'selftest'))
SOURCE_ROOT = 'morsehgp3D_v9'
CMAKE_LISTS = SOURCE_ROOT + '/CMakeLists.txt'
PROBE_SOURCE = SOURCE_ROOT + '/bench/tower_probe.cpp'
CHAIN_SOURCE = SOURCE_ROOT + '/src/chain/tower_chain.cpp'
# Le CMake v9 enregistre aussi les portes (tests/, oracle/) : leurs sources
# doivent exister a la configuration, meme si seule la sonde est construite.
SOURCE_PREFIXES = tuple(SOURCE_ROOT + '/' + name + '/' for name in ('cmake', 'src', 'bench', 'tests', 'oracle'))
REQUIRED_SOURCES = frozenset({CMAKE_LISTS, SOURCE_ROOT + '/cmake/run_expect.cmake', PROBE_SOURCE,
                              CHAIN_SOURCE, SOURCE_ROOT + '/src/chain/tower_chain.hpp'})
PROBE_TARGET = 'mhgp9_tower_probe'
INPUT_ROOT = 'morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6'
# Trames entieres (jamais un prefixe). fnv = empreinte FNV-1a 64 imprimee par
# la sonde (n puis x, y, z en u64 LE) ; celle de la scene 00 est recoupee
# avec la sortie C++ de mhgp9_tower_probe a d2700314 (5c785760053d17ce).
INPUTS = {
    '00': dict(file='data/scene_00.u32le', source=INPUT_ROOT + '/scene_00_grid/full.u32le', n=39885,
               sha256='0baa4de14c95838ef7bd18d5a98551ca513ed830ec1eeee84f649fa97c95abaf', fnv='5c785760053d17ce'),
    '01': dict(file='data/scene_01.u32le', source=INPUT_ROOT + '/scene_01_grid/full.u32le', n=35551,
               sha256='ba15adc6907d58e50bf28bca92305210c1efdde6efdf46c782aa1eec2318036f', fnv='4210173194931f58'),
    '02': dict(file='data/scene_02.u32le', source=INPUT_ROOT + '/scene_02_grid/full.u32le', n=45845,
               sha256='a4bbc86d00f92627b869fdc34aa260353bf1b821eff7c992ad93beb2a13308af', fnv='1c41bd0d1d689300'),
}
TIME = '/usr/bin/time'
BOOST_HEADER = '/usr/include/boost/multiprecision/cpp_int.hpp'
BOOST_ROOT = '/usr'
BUILD_PARALLEL = '48'
# Garde fixe : maxRunDuration 3600 s (valide sur la cible, jamais modifie).
# Arret invite 40 min et non 30 comme en v8 : avec 30 min, l'arret invite
# tombe a ~1800 s du depart et la marge de fermeture de 300 s ne laisse que
# ~1300-1400 s apres transfert, donc 1500 s utiles seraient une promesse
# fausse. 40 min = 2400 s couvrent 1500 s utiles + 300 s de fermeture + les
# ~150-300 s de certification/transfert ; et 40*60 + 300 (reserve GCE) +
# 120 (tolerance systemd) + 480 (armement) = 3300 <= 3600, l'inegalite
# imposee par start_and_verify.sh (et guest*60 + 900 <= 3600 exigee par la
# primitive v7 guard_values) tient avec 300 s de marge.
MAX_RUN_SECONDS = '3600'
GUEST_SHUTDOWN_MINUTES = '40'
USEFUL_BUDGET_SECONDS = 1500   # defaut et plafond dur du worker
CASE_CAP_SECONDS = 600         # plafond par cas : un K10 qui explose laisse vivre les autres cas
MIN_CASE_START_SECONDS = 15    # aucun cas n'est lance avec moins de temps utile restant
PROBE_STATUSES = ('complete_relative', 'unsupported_degeneracy', 'invalid_input',
                  'resource_exhausted', 'invariant_violated')
OUTCOMES = ('complete_relative', 'explicit_refusal', 'killed_case_cap', 'killed_budget',
            'skipped_budget', 'probe_failed')
CASE_KEYS = frozenset({'scene', 'file', 'n', 'k', 's', 'workers', 'static_threads', 'repeat'})
TOP_KEYS = frozenset({'schema', 'status', 'reason', 'input', 'options', 'times_ms', 'chain_cpu_s', 'generator',
                      'ledger', 'catalogue', 'tower_work', 'orders', 'tower_digest', 'peak_rss_kb'})
INPUT_KEYS = frozenset({'format', 'grid', 'sites', 'hash'})
OPTION_KEYS = frozenset({'K', 'K_effective', 's', 'workers', 'tower_static_threads', 'run_tower', 'atlas_saturate_deep'})
TIME_KEYS = frozenset({'read', 'prepare', 'gen_index', 'q2', 'q34', 'merge', 'tower_index', 'census', 'tower',
                       'chain_total'})
ORDER_KEYS = frozenset({'K', 'nodes', 'births', 'merges', 'parents', 'contributions'})
SCOPE = 'FULL_tower_chain_relative_to_cross_checked_catalogue'


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    with Path(path).open('x') as stream:
        json.dump(value, stream, sort_keys=True, indent=2)
        stream.write('\n')


def canonical_json(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), allow_nan=False).encode('ascii') + b'\n'


def strict_json(raw):
    def pairs(items):
        out = {}
        for key, value in items:
            need(key not in out, 'duplicate JSON key')
            out[key] = value
        return out

    def invalid(_):
        raise ValueError('nonfinite JSON number')

    def finite(text):
        value = float(text)
        need(math.isfinite(value), 'nonfinite JSON number')
        return value
    return json.loads(raw, object_pairs_hook=pairs, parse_constant=invalid, parse_float=finite)


def safe_name(name):
    if type(name) is not str or not name:
        return False
    path = PurePosixPath(name)
    if str(path) != name or path.is_absolute() or any(part in ('.', '..') for part in path.parts):
        return False
    return (name == CMAKE_LISTS or name.startswith(SOURCE_PREFIXES) or name == HELPER or name in PROTOCOL_NAMES or
            name in (PLAN, PROVENANCE) or name in {data['file'] for data in INPUTS.values()})


def validate_manifest(manifest):
    need(type(manifest) is dict and manifest, 'payload manifest object')
    for name, pin in manifest.items():
        need(safe_name(name) and type(pin) is str and re.fullmatch('[0-9a-f]{64}', pin), 'unsafe payload path/hash')
    need(REQUIRED_SOURCES | PROTOCOL_NAMES | {HELPER, PLAN, PROVENANCE} |
         {data['file'] for data in INPUTS.values()} <= set(manifest), 'required payload absent')
    need(manifest[HELPER] == HELPER_SHA, 'legacy helper pin')
    need(all(manifest[data['file']] == data['sha256'] for data in INPUTS.values()), 'pinned LiDAR frame hash')


def validate_sources(read_bytes):
    """Controle statique bon marche : cible et interface CLI attendues au commit."""
    cmake = read_bytes(CMAKE_LISTS)
    probe = read_bytes(PROBE_SOURCE)
    need(b'mhgp9_product_executable(' + PROBE_TARGET.encode() + b' bench/tower_probe.cpp)' in cmake,
         'CMake target mhgp9_tower_probe absent')
    need(all(token in probe for token in (PROBE_SCHEMA.encode(), b'"--s="', b'"--static="', b'"--grid="')),
         'tower probe schema/CLI differs from the v9 protocol')


def _integer(value, low, high):
    return type(value) is int and low <= value <= high


def validate_plan(plan, manifest):
    need(type(plan) is dict and set(plan) == {'schema', 'cases'} and plan['schema'] == PLAN_SCHEMA and
         type(plan['cases']) is list and 0 < len(plan['cases']) <= 64, 'tower plan schema')
    seen = set()
    for case in plan['cases']:
        need(type(case) is dict and set(case) == CASE_KEYS, 'tower case fields')
        need(type(case['scene']) is str and case['scene'] in INPUTS and
             case['file'] == INPUTS[case['scene']]['file'] and case['file'] in manifest, 'tower scene/file identity')
        need(case['n'] == INPUTS[case['scene']]['n'] and type(case['n']) is int, 'whole-frame size; prefixes forbidden')
        need(type(case['k']) is int and case['k'] in (5, 10) and type(case['s']) is int and case['s'] in (8, 10, 12) and
             _integer(case['workers'], 1, 48) and _integer(case['static_threads'], 0, 48) and
             _integer(case['repeat'], 0, (1 << 32) - 1), 'tower case domain')
        identity = tuple(case[key] for key in ('scene', 'k', 's', 'workers', 'static_threads', 'repeat'))
        need(identity not in seen, 'duplicate case needs an explicit distinct repetition')
        seen.add(identity)
    return plan['cases']


_FNV_PRIME = 1099511628211
_FNV_MASK = (1 << 64) - 1
_FNV_ZERO5 = pow(_FNV_PRIME, 5, 1 << 64)


def input_fnv(raw):
    """FNV-1a 64 de mhgp9_tower_probe : n puis chaque coordonnee en u64 LE."""
    need(len(raw) % 12 == 0, 'u32le incomplete site record')
    h = 14695981039346656037
    for byte in (len(raw) // 12).to_bytes(8, 'little'):
        h = ((h ^ byte) * _FNV_PRIME) & _FNV_MASK
    for (value,) in struct.iter_unpack('<I', raw):
        need(value < (1 << 18), 'coordinate outside [0, 2^18)')
        h = ((h ^ (value & 255)) * _FNV_PRIME) & _FNV_MASK
        h = ((h ^ ((value >> 8) & 255)) * _FNV_PRIME) & _FNV_MASK
        h = ((h ^ (value >> 16)) * _FNV_PRIME) & _FNV_MASK
        h = (h * _FNV_ZERO5) & _FNV_MASK   # cinq octets nuls de poids fort
    return '%016x' % h


def validate_data(read_bytes):
    for data in INPUTS.values():
        raw = read_bytes(data['file'])
        need(hashlib.sha256(raw).hexdigest() == data['sha256'] and len(raw) == 12 * data['n'] and
             input_fnv(raw) == data['fnv'], 'pinned whole LiDAR frame differs')


def validate_provenance(value, manifest):
    need(type(value) is dict and set(value) == {'schema', 'commit', 'tree', 'protocol_source', 'inputs', 'helper'} and
         value['schema'] == PROVENANCE_SCHEMA, 'provenance schema')
    need(all(type(value[key]) is str and re.fullmatch('[0-9a-f]{40}|[0-9a-f]{64}', value[key])
             for key in ('commit', 'tree')), 'provenance commit/tree')
    need(value['protocol_source'] in ('commit', 'worktree_uncommitted'), 'protocol source')
    need(value['inputs'] == {data['file']: dict(commit_path=data['source'], sha256=data['sha256'])
                             for data in INPUTS.values()}, 'provenance inputs')
    need(value['helper'] == dict(path=HELPER, sha256=HELPER_SHA) and manifest.get(HELPER) == HELPER_SHA,
         'provenance helper')
    return value


def validate_runtime(manifest):
    # L'executable est televerse comme remote/worker.py, hors de source/ :
    # epingler ce code reel en plus de sa copie archivee.
    need(sha(__file__) == manifest.get('gcp-migration/tower_worker_v9.py'),
         'executing worker differs from transported worker source')


def source_map(root, manifest):
    validate_manifest(manifest)
    out = {}
    for name, pin in manifest.items():
        path = root / name
        need(path.is_file() and not path.is_symlink() and path.resolve().is_relative_to(root), 'payload file type')
        out[name] = sha(path)
        need(out[name] == pin, 'payload changed: ' + name)
    return out


def load_helper(root):
    path = root / HELPER
    raw = path.read_bytes()
    need(hashlib.sha256(raw).hexdigest() == HELPER_SHA, 'worker helper changed')
    module = types.ModuleType('mhgp9_pinned_worker_primitives')
    module.__file__ = str(path)
    exec(compile(raw, str(path), 'exec'), module.__dict__)
    return module


def available_cpus():
    return sorted(os.sched_getaffinity(0))


def boot_epoch():
    return time.time() - float(Path('/proc/uptime').read_text().split()[0])


def probe_command(build, root, case):
    return [str(build / PROBE_TARGET), str(root / case['file']), str(case['k']), str(case['workers']),
            '--s=' + str(case['s']), '--static=' + str(case['static_threads']), '--grid=1mm']


def expected_probe_tail(case):
    return [str(case['k']), str(case['workers']), '--s=' + str(case['s']),
            '--static=' + str(case['static_threads']), '--grid=1mm']


def _count(value):
    return type(value) is int and 0 <= value < (1 << 64)


def _number(value):
    return type(value) in (int, float) and math.isfinite(value) and value >= 0


def validate_probe(value, case, exit_code):
    """Lit la sortie de la sonde ; rend complete_relative ou explicit_refusal.

    Un statut non complet n'est accepte que s'il est explicite (code 3) ; une
    sortie partielle, un code inattendu ou un domaine different est refuse."""
    need(type(value) is dict and set(value) == TOP_KEYS, 'probe JSON fields')
    need(value['schema'] == PROBE_SCHEMA and value['status'] in PROBE_STATUSES and type(value['reason']) is str,
         'probe schema/status')
    need(type(value['ledger']) is dict and value['ledger'] and
         all(type(k) is str and type(v) is int and v >= 0 for k, v in value['ledger'].items()), 'probe ledger')
    data = INPUTS[case['scene']]
    source = value['input']
    need(type(source) is dict and set(source) == INPUT_KEYS and source['format'] == 'u32le' and
         source['grid'] == '1mm' and source['sites'] == case['n'] == data['n'] and source['hash'] == data['fnv'],
         'probe input identity')
    options = value['options']
    need(type(options) is dict and set(options) == OPTION_KEYS and options['K'] == case['k'] and
         options['s'] == case['s'] and options['workers'] == case['workers'] and
         options['tower_static_threads'] == case['static_threads'] and options['run_tower'] is True and
         type(options['atlas_saturate_deep']) is bool and type(options['K_effective']) is int, 'probe options')
    times = value['times_ms']
    need(type(times) is dict and set(times) == TIME_KEYS and all(_number(item) for item in times.values()) and
         _number(value['chain_cpu_s']), 'probe times')
    for key in ('generator', 'tower_work'):
        need(type(value[key]) is dict and value[key] and
             all(type(name) is str and _count(item) for name, item in value[key].items()), 'probe counters ' + key)
    catalogue = value['catalogue']
    need(type(catalogue) is dict and {'unique_keys', 'balls', 'by_qmin', 'by_shell'} <= set(catalogue) and
         all(all(_count(x) for x in item) if type(item) is list else _count(item) for item in catalogue.values()),
         'probe catalogue')
    orders = value['orders']
    need(type(orders) is list and all(type(order) is dict and set(order) == ORDER_KEYS and
                                      all(_count(item) for item in order.values()) for order in orders), 'probe orders')
    need(type(value['tower_digest']) is str and re.fullmatch('[0-9a-f]{16}', value['tower_digest']) and
         type(value['peak_rss_kb']) is int and value['peak_rss_kb'] >= -1, 'probe digest/RSS')
    effective = min(case['k'], case['n'])
    if value['status'] == 'complete_relative':
        need(exit_code == 0 and options['K_effective'] == effective and
             [order['K'] for order in orders] == list(range(1, effective + 1)), 'complete tower: code 0, orders 1..K')
        return 'complete_relative'
    need(exit_code == 3 and options['K_effective'] in (0, effective), 'explicit refusal must exit with code 3')
    return 'explicit_refusal'


def validate_gnu_time(text, exit_code):
    need('Command being timed: "' in text and 'Elapsed (wall clock) time' in text, 'GNU time -v report')
    rss = re.findall(r'^\s*Maximum resident set size \(kbytes\): (\d+)$', text, re.M)
    status = re.findall(r'^\s*Exit status: (\d+)$', text, re.M)
    need(len(rss) == 1 and status == [str(exit_code)], 'GNU time exit status/RSS')
    return int(rss[0])


def logical_result(value):
    """Projection comparee entre nombres d'ouvriers : l'objet, pas les couts."""
    return dict(hash=value['input']['hash'], sites=value['input']['sites'],
                K_effective=value['options']['K_effective'], unique_keys=value['catalogue']['unique_keys'],
                balls=value['catalogue']['balls'], orders=value['orders'], tower_digest=value['tower_digest'])


def compare_cases(cases, outcomes, values):
    groups, out = {}, []
    for index, (case, entry) in enumerate(zip(cases, outcomes)):
        if entry.get('outcome') != 'complete_relative':
            continue
        key = (case['file'], case['k'], case['s'])
        if key in groups:
            reference = groups[key]
            out.append(dict(reference=reference, other=index,
                            equal=logical_result(values[reference]) == logical_result(values[index])))
        else:
            groups[key] = index
    return out


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
    need({PROBE_SOURCE, CHAIN_SOURCE} <= relative_seen, 'depfiles lack the probe/chain sources')
    return consumed


class BuildFailed(Exception):
    pass


def execute(args):
    root, output = args.source_root.resolve(), args.output.absolute()
    # Un plafond par cas superieur au budget est licite : le budget gouverne.
    need(1 <= args.useful_budget_seconds <= USEFUL_BUDGET_SECONDS and
         1 <= args.case_cap_seconds <= int(MAX_RUN_SECONDS) and args.closing_margin_seconds >= 300, 'cost/closing budget')
    need(not output.exists() and not output.is_symlink() and not output.resolve().is_relative_to(root), 'fresh output')
    output.mkdir(mode=0o700)
    result = dict(status='failed', backend='reference_cpu', scope=SCOPE, public_status='not_claimed',
                  GPU_executed=False, FULL_executed=False, contract_certified=False,
                  targeted_GCP_stop_required_by_ROOT=True, worker_argv=list(args.argv), worker_sha256=sha(__file__),
                  useful_budget_seconds=args.useful_budget_seconds, case_cap_seconds=args.case_cap_seconds)
    worker, manifest, before, consumed = None, None, None, {}
    tools, binary, binary_sha = {}, None, None
    began = time.time()

    def interrupted(signum, _frame):
        raise InterruptedError('guest signal ' + str(signum))
    previous = {signum: signal.signal(signum, interrupted) for signum in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP)}
    try:
        helper = load_helper(root)
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
        need(abs(boot_epoch() - helper.epoch(args.generation)) <= 300, 'guest boot/generation')
        cpus = available_cpus()
        need(len(cpus) == 48, '48 available vCPUs required')
        useful_deadline = min(began + args.useful_budget_seconds, guards['work_deadline_epoch'])
        worker = helper.Worker(output, useful_deadline, schedule)
        result.update(target=target, generation=args.generation, available_cpus=cpus, guards=guards,
                      useful_deadline_epoch=useful_deadline)
        save(output / 'guard_evidence.json', dict(mark=mark, schedule=schedule, metadata=observed))
        need(sha(args.source_manifest) == args.source_manifest_sha256, 'manifest pin')
        manifest = strict_json(args.source_manifest.read_bytes())
        validate_runtime(manifest)
        result['source_manifest_sha256'] = args.source_manifest_sha256
        before = source_map(root, manifest)
        save(output / 'sources_before.json', before)
        read = lambda name: (root / name).read_bytes()
        validate_sources(read)
        cases = validate_plan(strict_json(read(PLAN)), manifest)
        validate_data(read)
        result['provenance'] = validate_provenance(strict_json(read(PROVENANCE)), manifest)
        result.update(plan_schema=PLAN_SCHEMA, cases=cases, case_outcomes=[], completed_case_indices=[],
                      cross_worker_comparisons=[])
        tools = {'g++': shutil.which('g++'), 'cmake': shutil.which('cmake')}
        need(tools['g++'] and tools['cmake'] and Path(TIME).is_file() and Path(BOOST_HEADER).is_file(),
             'g++, cmake, GNU time and system Boost required; no automatic installation')
        result['tool_paths'] = dict(tools, time=TIME, boost_header=BOOST_HEADER)
        result['tool_sha256'] = {name: sha(path) for name, path in tools.items()}
        for name, command in [('compiler', [tools['g++'], '--version']), ('cmake_version', [tools['cmake'], '--version']),
                              ('time_version', [TIME, '--version']), ('lscpu', ['lscpu']), ('nproc', ['nproc'])]:
            need(worker.command(name, command)['exit_code'] == 0, name)
        for filename in ('/etc/os-release', '/proc/meminfo'):
            with (output / (Path(filename).name + '.txt')).open('x') as stream:
                stream.write(Path(filename).read_text())
        build = output / 'build'
        # Build strict tel quel : -DCMAKE_CXX_FLAGS=-Wno-error serait place
        # AVANT le -Werror de add_compile_options (CMAKE_CXX_FLAGS precede les
        # options de compilation), donc sans effet ; le CMake ne le permet
        # pas et le worker ne le passe pas. Un echec est consigne, sans relance.
        configure = worker.command('configure', [tools['cmake'], '-S', str(root / SOURCE_ROOT), '-B', str(build),
                                                 '-DCMAKE_BUILD_TYPE=Release', '-DBOOST_ROOT=' + BOOST_ROOT,
                                                 '-DCMAKE_CXX_COMPILER=' + tools['g++']])
        if configure['exit_code'] != 0:
            raise BuildFailed('cmake configure failed; logs in configure.stdout/stderr')
        compiled = worker.command('build', [tools['cmake'], '--build', str(build), '--target', PROBE_TARGET,
                                            '--parallel', BUILD_PARALLEL])
        if compiled['exit_code'] != 0:
            raise BuildFailed('strict build of mhgp9_tower_probe failed; logs in build.stdout/stderr')
        binary = build / PROBE_TARGET
        need(binary.is_file() and not binary.is_symlink(), 'probe binary absent after build')
        binary_sha = sha(binary)
        consumed = compiled_dependencies(build, root, before)
        save(output / 'compiled_dependencies.json', consumed)
        result.update(binary_sha256=binary_sha,
                      compiled_dependency_manifest_sha256=sha(output / 'compiled_dependencies.json'))

        def left():
            try:
                return worker.remaining()
            except helper.SessionDeadline:
                return 0.0
        outcomes, values, exhausted = result['case_outcomes'], {}, False
        for index, case in enumerate(cases):
            suffix = str(index)
            if exhausted or left() < MIN_CASE_START_SECONDS:
                exhausted = True
                outcomes.append(dict(index=index, outcome='skipped_budget'))
                continue
            need(worker.command('uptime_before_' + suffix, ['uptime'])['exit_code'] == 0, 'uptime')
            started = time.time()
            capped = started + args.case_cap_seconds < useful_deadline
            case_worker = helper.Worker(output, min(started + args.case_cap_seconds, useful_deadline), schedule)
            name = 'probe_' + suffix
            result['FULL_executed'] = True
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
                outcomes.append(dict(index=index, outcome='killed_case_cap' if capped else 'killed_budget',
                                     exit_code=row['exit_code'], elapsed_seconds=row['elapsed_seconds']))
                exhausted = not capped
            else:
                entry = dict(index=index, exit_code=row['exit_code'], elapsed_seconds=row['elapsed_seconds'])
                try:
                    need(row['exit_code'] in (0, 3), 'probe exit code outside {0, 3}')
                    value = strict_json((output / (name + '.stdout')).read_bytes())
                    outcome = validate_probe(value, case, row['exit_code'])
                    rss = validate_gnu_time((output / (name + '.stderr')).read_text(errors='replace'), row['exit_code'])
                    values[index] = value
                    entry.update(outcome=outcome, probe_status=value['status'], tower_digest=value['tower_digest'],
                                 gnu_time_max_rss_kb=rss, chain_total_ms=value['times_ms']['chain_total'])
                except (ValueError, KeyError, TypeError, UnicodeError) as error:
                    entry.update(outcome='probe_failed', reason=type(error).__name__ + ': ' + str(error))
                outcomes.append(entry)
                need(sha(binary) == binary_sha, 'binary changed')
            save(output / (name + '.summary.json'), dict(case=case, input_file_sha256=manifest[case['file']],
                                                         **outcomes[-1]))
            if not exhausted and left() >= 5:
                need(worker.command('uptime_after_' + suffix, ['uptime'])['exit_code'] == 0, 'uptime')
        need(result['FULL_executed'], 'no case started within the useful budget')
        result['completed_case_indices'] = [e['index'] for e in outcomes if e['outcome'] == 'complete_relative']
        result['cross_worker_comparisons'] = compare_cases(cases, outcomes, values)
        if any(e['outcome'] == 'probe_failed' for e in outcomes):
            result['status'] = 'probe_failed'
        elif not all(item['equal'] for item in result['cross_worker_comparisons']):
            result['status'] = 'cross_worker_mismatch'
        elif len(result['completed_case_indices']) == len(cases):
            result['status'] = 'completed'
        else:
            result['status'] = 'partial'
    except BuildFailed as error:
        result.update(status='build_failed', error=str(error))
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
    return 0 if result['status'] in ('completed', 'partial') else 1


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
        print(json.dumps(dict(status='inert', target=TARGET, backend='reference_cpu', scope=SCOPE,
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
