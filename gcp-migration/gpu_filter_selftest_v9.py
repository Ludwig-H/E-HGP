#!/usr/bin/env python3
"""Selftest hors ligne du protocole G4 de la sonde S1 GPU v9 ; aucune commande GCP.

Reprend le faux cloud du selftest de la tour (tower_selftest_v9.FakeCloud :
faux gcloud, ssh et scp, VM simulee par un repertoire local ; les scripts SSH
du controleur y sont rejoues par bash) et y execute le VRAI worker GPU en
processus, avec de faux cmake, g++, nvcc, nvidia-smi et une fausse sonde S1.
GNU time, uptime, lscpu et nproc sont les vrais outils locaux.

Ces tests qualifient les predicats, le cycle de vie et la fermeture du
protocole, pas une mesure G4. Ils passent sous python3 -B et python3 -B -O :
aucune instruction assert, uniquement need().
"""
import atexit
from contextlib import ExitStack, contextmanager, redirect_stdout
from copy import deepcopy
import io
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import gpu_filter_session_v9 as session
import gpu_filter_snapshot_v9 as snapshot
import gpu_filter_worker_v9 as worker
import tower_selftest_v9 as tower_selftest
import tower_worker_v9 as base

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
need = worker.need
refused = tower_selftest.refused
NEVER_RUN_GCLOUD = tower_selftest.NEVER_RUN_GCLOUD
DEVICE = worker.DEVICE_NAME

FAKE_PROBE = r'''
import hashlib
import json
import pathlib
import sys


def fnv_u32le(raw):
    h = 14695981039346656037
    def word(value):
        nonlocal h
        for byte in value.to_bytes(8, 'little'):
            h = ((h ^ byte) * 1099511628211) & ((1 << 64) - 1)
    word(len(raw) // 12)
    for i in range(0, len(raw), 4):
        word(int.from_bytes(raw[i:i + 4], 'little'))
    return '%016x' % h


def probe_value(n, fnv, k, workers, repeats, mismatch=False, inject=''):
    rect_visits, pair_visits = 1000 + k, 5000 + 7 * k
    gpu = dict(available=True, device=DEVICE, error='', stack_failure=False, pairs=400, rect_visits=rect_visits,
               pair_visits=pair_visits, upload_ms=1.25, rect_ms=2.5, scan_ms=0.25, pair_ms=10.0, download_ms=0.5,
               total_ms=14.5, first_total_ms=90.0, rect_mismatches=0,
               pair_mismatches=3 if mismatch else 1 if inject else 0, visits_equal=True)
    return dict(schema='mhgp9_gpu_filter_probe_v2', input=dict(sites=n, hash=fnv),
                options=dict(K=k, s=8, workers=workers, repeats=repeats, cpu_only=False, inject=inject),
                population=dict(rectangles=100, rectangle_survivors=40, pairs=400, pair_survivors=30,
                                q3_rejected=300, q3_open=50, q4_rejected=320, q4_open=40, front_product_visits=900),
                cpu=dict(index_ms=1.0, front_ms=2.0, rect_ms=3.0, pair_nocache_ms=4.0, pair_cache_ms=2.0,
                         rect_visits=rect_visits, pair_visits=pair_visits, cache_searches=200, cache_mismatches=0),
                gpu=gpu, peak_rss_kb=4096)


def main():
    config = json.loads(pathlib.Path(CONFIG).read_text())
    path, k, workers = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
    options = sys.argv[4:]
    inject = ''
    if len(options) == 3 and options[2] == '--inject=pair_mask' and not config.get('mutant_survives'):
        inject = 'pair_mask'
    elif len(options) == 3 and options[2] == '--inject=pair_mask':
        options = options[:2]   # a broken judge that ignores the injection
    if len(options) not in (2, 3) or options[0] != '--s=8' or not options[1].startswith('--repeats='):
        print('argument refusal: selftest', file=sys.stderr)
        return 2
    repeats = int(options[1][len('--repeats='):])
    raw = pathlib.Path(path).read_bytes()
    name = pathlib.Path(path).name
    scene = None if name == 'preflight.u32le' else name[len('scene_'):-len('.u32le')]
    if scene is not None and hashlib.sha256(raw).hexdigest() != config['sha256'][scene]:
        print('input refusal: selftest', file=sys.stderr)
        return 2
    mismatch = any(rule['scene'] == scene and rule['k'] == k for rule in config.get('mismatch', []))
    value = probe_value(len(raw) // 12, fnv_u32le(raw), k, workers, repeats, mismatch, inject)
    for kind in ('unavailable', 'fault'):
        if any(rule['scene'] == scene and rule['k'] == k for rule in config.get(kind, [])):
            value['gpu'].update(available=kind == 'fault', error='cudaErrorIllegalAddress' if kind == 'fault' else
                                'no CUDA device', total_ms=0.0, first_total_ms=0.0, upload_ms=0.0, rect_ms=0.0,
                                scan_ms=0.0, pair_ms=0.0, download_ms=0.0)
            print(json.dumps(value, separators=(',', ':')))
            return 3
    if scene is None and config.get('fail_preflight'):
        value['gpu']['device'] = 'other device'
    if any(rule['scene'] == scene and rule['k'] == k for rule in config.get('malform', [])):
        value['gpu']['selftest_unknown'] = 1
    print(json.dumps(value, separators=(',', ':')))
    return 1 if mismatch or inject else 0


if __name__ == '__main__':
    raise SystemExit(main())
'''

FAKE_CMAKE = r'''
import json
import pathlib
import shutil
import sys

HERE = pathlib.Path(__file__).resolve().parent


def main():
    config = json.loads((HERE / 'config.json').read_text())
    args = sys.argv[1:]
    if args == ['--version']:
        print('cmake version 3.22.1 (selftest fake; never a real build)')
        return 0
    if len(args) == 9 and args[0] == '-S' and args[2] == '-B':
        source, build = pathlib.Path(args[1]), pathlib.Path(args[3])
        if (args[4:6] != ['-DCMAKE_BUILD_TYPE=Release', '-DBOOST_ROOT=/usr'] or
                not args[6].startswith('-DCMAKE_CXX_COMPILER=') or args[7] != '-DMHGP9_ENABLE_CUDA=ON' or
                not args[8].startswith('-DCMAKE_CUDA_COMPILER=') or any('Wno-error' in item for item in args)):
            return 64
        if not (source / 'CMakeLists.txt').is_file() or config.get('fail_configure'):
            print('CMake Error: selftest configure failure', file=sys.stderr)
            return 1
        build.mkdir(parents=True)
        (build / 'source.txt').write_text(str(source))
        return 0
    if len(args) == 6 and args[0] == '--build' and args[2:] == ['--target', 'mhgp9_gpu_filter_probe', '--parallel', '48']:
        build = pathlib.Path(args[1])
        source = pathlib.Path((build / 'source.txt').read_text())
        if config.get('fail_build'):
            print('error: selftest strict compilation failure', file=sys.stderr)
            return 2
        probe = build / 'mhgp9_gpu_filter_probe'
        shutil.copyfile(HERE / 'fake_probe.py', probe)
        probe.chmod(0o755)
        for target, relative in (('mhgp9_gpu_filter_probe', 'bench/gpu_filter_probe.cpp'),
                                 ('mhgp9_gpu', 'src/gpu/filter_runner.cu')):
            depfile = build / 'CMakeFiles' / (target + '.dir') / (relative + '.o.d')
            depfile.parent.mkdir(parents=True, exist_ok=True)
            names = [source / relative, source / 'src/gpu/witness_filter.hpp', HERE / 'system_header.hpp']
            depfile.write_text('CMakeFiles/' + target + '.dir/' + relative + '.o: \\\n ' +
                               ' \\\n '.join(map(str, names)) + '\n')
        return 0
    return 64


if __name__ == '__main__':
    raise SystemExit(main())
'''

FAKE_NVCC = r'''
import sys
if sys.argv[1:] != ['--version']:
    raise SystemExit(64)
print('Cuda compilation tools, release 12.9, V12.9.86 (selftest fake)')
'''

FAKE_SMI = r'''
import sys
if sys.argv[1:] != ['--query-gpu=name,driver_version,memory.total,compute_cap', '--format=csv,noheader']:
    raise SystemExit(64)
print('NVIDIA RTX PRO 6000 Blackwell Server Edition, 580.126.09, 97887 MiB, 12.0')
'''


def fake_probe_value(n, fnv, k, workers=48, repeats=3, mismatch=False, inject=''):
    namespace = {'__name__': 'mhgp9_fake_gpu_probe', 'DEVICE': DEVICE, 'CONFIG': ''}
    exec(compile(FAKE_PROBE, 'mhgp9_fake_gpu_probe', 'exec'), namespace)
    return namespace['probe_value'](n, fnv, k, workers, repeats, mismatch, inject)


def fake_tools(directory, **config):
    fakebin = directory / 'fakebin'
    fakebin.mkdir()
    settings = dict(sha256={scene: data['sha256'] for scene, data in worker.INPUTS.items()})
    settings.update(config)
    (fakebin / 'config.json').write_text(json.dumps(settings))
    (fakebin / 'system_header.hpp').write_text('// selftest system header outside the snapshot\n')
    (directory / 'boost_cpp_int.hpp').write_text('// selftest stand-in for boost/multiprecision/cpp_int.hpp\n')
    shebang = '#!' + sys.executable + ' -B\n'
    probe = 'CONFIG = ' + repr(str(fakebin / 'config.json')) + '\nDEVICE = ' + repr(DEVICE) + '\n' + FAKE_PROBE
    for name, text in (('cmake', FAKE_CMAKE), ('g++', tower_selftest.FAKE_GXX), ('nvcc', FAKE_NVCC),
                       ('nvidia-smi', FAKE_SMI), ('fake_probe.py', probe)):
        (fakebin / name).write_text(shebang + text)
        (fakebin / name).chmod(0o755)
    return fakebin


@contextmanager
def guest(fakebin, generation, schedule_text):
    original_load = base.load_helper

    def load(root):
        module = original_load(root)
        module.metadata = lambda: dict(worker.TARGET, machine='g4-standard-48')
        module.scheduled_text = lambda: schedule_text
        return module
    with ExitStack() as stack:
        stack.enter_context(patch.dict(os.environ, {'PATH': str(fakebin) + os.pathsep + os.environ.get('PATH', '')}))
        stack.enter_context(patch.object(base, 'load_helper', load))
        stack.enter_context(patch.object(worker, 'BOOST_HEADER', str(fakebin.parent / 'boost_cpp_int.hpp')))
        stack.enter_context(patch.object(worker, 'CUDA_PATHS', (str(fakebin / 'nvcc'),)))
        stack.enter_context(patch.object(base, 'available_cpus', lambda: list(range(48))))
        stack.enter_context(patch.object(base, 'boot_epoch', lambda: session.epoch(generation)))
        yield


class FakeCloud(tower_selftest.FakeCloud):
    """Le faux cloud de la tour ; seule la commande worker lance le worker GPU."""

    def run(self, host, name, argv):
        if name != 'worker':
            return super().run(host, name, argv)
        self.calls.append((name, argv))
        script = argv[-1][len('--command='):]
        need(script.startswith('exec '), 'worker exec')
        argv_worker = [self.to_local(item) for item in shlex.split(script[len('exec '):])]
        need(argv_worker[:4] == ['env', 'PYTHONPATH=' + str(self.local / 'source/gcp-migration'), 'python3',
                                 str(self.local / 'worker.py')] and
             worker.sha(argv_worker[3]) == worker.sha(worker.__file__) and
             worker.sha(self.local / 'source' / worker.BASE_WORKER) == worker.sha(base.__file__),
             'uploaded worker and its extracted library are the pinned files')
        stream = io.StringIO()
        with guest(self.fakebin, self.generation, self.schedule), redirect_stdout(stream):
            code = worker.main(argv_worker[4:])
        return code, stream.getvalue(), ''


_PACKAGE = {}


def protocol_committed():
    full, _ = tower_selftest.snapshot.resolve_commit('HEAD')
    entries = tower_selftest.snapshot.tree_entries(full, sorted(worker.PROTOCOL_NAMES))
    if set(entries) != set(worker.PROTOCOL_NAMES):
        return False
    blobs = tower_selftest.snapshot.read_blobs(entries.values())
    return all(blobs[entries[name]] == (ROOT / name).read_bytes() for name in worker.PROTOCOL_NAMES)


def package():
    if not _PACKAGE:
        directory = Path(tempfile.mkdtemp(prefix='mhgp9-gpu-selftest-'))
        atexit.register(shutil.rmtree, directory, True)
        committed = protocol_committed()
        record = snapshot.build('HEAD', directory / 'package', allow_uncommitted_protocol=not committed)
        manifest_path = directory / 'package/source_manifest.json'
        _PACKAGE.update(directory=directory, record=record, committed=committed,
                        archive=directory / 'package/snapshot.tar.gz', manifest_path=manifest_path,
                        manifest=worker.strict_json(manifest_path.read_bytes()))
    return _PACKAGE


def session_args(directory):
    private = directory / 'session'
    private.mkdir(mode=0o700)
    key = directory / 'key'
    key.write_text('fixture-key-never-used')
    key.chmod(0o600)
    Path(str(key) + '.pub').write_text('fixture-public-key')
    pkg = package()
    return SimpleNamespace(session_dir=private, ssh_key=key, expected_controller_sha256=worker.sha(session.__file__),
                           worker=Path(worker.__file__), worker_sha256=worker.sha(worker.__file__),
                           snapshot=pkg['archive'], snapshot_sha256=worker.sha(pkg['archive']),
                           manifest=pkg['manifest_path'], manifest_sha256=worker.sha(pkg['manifest_path']),
                           gcloud=NEVER_RUN_GCLOUD)


def run_scenario(directory, tools=None, patches=()):
    fakebin = fake_tools(directory, **(tools or {}))
    fake = FakeCloud(directory, fakebin)
    args = session_args(directory)
    with ExitStack() as stack:
        stack.enter_context(patch.object(session, 'Commands', fake.commands_class()))
        # The host reception checks the recorded nvcc against CUDA_PATHS too.
        stack.enter_context(patch.object(worker, 'CUDA_PATHS', (str(fakebin / 'nvcc'),)))
        for owner, name, value in patches:
            stack.enter_context(patch.object(owner, name, value))
        stack.enter_context(redirect_stdout(io.StringIO()))
        code = session.run_session(args)
    host = args.session_dir / 'gpu_filter_v9_host'
    return code, worker.strict_json((host / 'receipt.json').read_bytes()), fake, host


def expect_certified_stop(receipt, fake):
    names = [name for name, _ in fake.calls]
    need(receipt['targeted_shutdown_certified'] is True and receipt['generation'] == fake.generation and
         fake.stops == [fake.generation] and names.count('guarded_stop') == 1 and names[-1] == 'guarded_stop',
         'exactly one certified targeted stop, last')


class Protocol(unittest.TestCase):
    def test_pins_and_shared_lifecycle(self):
        for name, pin in session.GUARDS.items():
            need(worker.sha(HERE / name) == pin, 'guard pin ' + name)
        need(worker.sha(HERE / 'full_probe_worker_v7.py') == worker.HELPER_SHA, 'worker helper pin')
        need(session.TARGET == base.TARGET == worker.TARGET, 'unchanged fixed target')
        need(worker.MAX_RUN_SECONDS == '3600' and worker.GUEST_SHUTDOWN_MINUTES == '40' and
             worker.USEFUL_BUDGET_SECONDS == base.USEFUL_BUDGET_SECONDS and
             worker.CASE_CAP_SECONDS == base.CASE_CAP_SECONDS, 'same guard durations and budgets as the tower')
        need(session.validate_target is session.lifecycle.validate_target and
             session.guard_deadline is session.lifecycle.guard_deadline, 'lifecycle predicates are the tower ones')

    def test_inert_without_cloud(self):
        with patch('subprocess.Popen', side_effect=RuntimeError('no subprocess permitted')):
            for module in (session, worker):
                stream = io.StringIO()
                with redirect_stdout(stream):
                    need(module.main([]) == 0, 'inert exit code')
                value = json.loads(stream.getvalue())
                need(value['status'] == 'inert' and value['GPU_executed'] is False and
                     value['FULL_executed'] is False and value['backend'] == 'cuda_g4', 'inert report')

    def test_probe_reader(self):
        data = worker.INPUTS['00']
        case = dict(scene='00', file=data['file'], n=data['n'], k=5, s=8, workers=48, repeats=3, repeat=0)
        good = fake_probe_value(data['n'], data['fnv'], 5)
        need(worker.validate_probe(good, case, 0) == 'complete', 'good probe')
        need(worker.validate_probe(fake_probe_value(data['n'], data['fnv'], 5, mismatch=True), case, 1) ==
             'gpu_mismatch', 'measured mismatch')
        need(refused(worker.validate_probe, fake_probe_value(data['n'], data['fnv'], 5, mismatch=True), case, 0),
             'mismatch with exit 0')
        need(refused(worker.validate_probe, good, case, 1), 'exit 1 with agreeing masks')
        mutant = fake_probe_value(data['n'], data['fnv'], 5, inject='pair_mask')
        need(worker.validate_probe(mutant, case, 1, inject='pair_mask') == 'gpu_mismatch', 'mutant killed once')
        need(refused(worker.validate_probe, mutant, case, 1), 'mutant output read as a normal case')
        twice = deepcopy(mutant)
        twice['gpu']['pair_mismatches'] = 2
        need(refused(worker.validate_probe, twice, case, 1, inject='pair_mask'), 'mutant must differ exactly once')
        need(refused(worker.validate_probe, good, case, 0, inject='pair_mask'), 'surviving mutant refused')
        stack = deepcopy(good)
        stack['gpu']['stack_failure'] = True
        need(worker.validate_probe(stack, case, 1) == 'gpu_mismatch', 'device stack violation is a divergence')
        need(refused(worker.validate_probe, stack, case, 3), 'stack violation alone is not unavailability')
        fault = deepcopy(good)
        fault['gpu']['error'] = 'cudaErrorIllegalAddress'
        need(worker.validate_probe(fault, case, 3) == 'gpu_fault', 'CUDA error on a present device is a fault')
        unavailable = deepcopy(good)
        unavailable['gpu'].update(available=False, error='no CUDA device')
        need(worker.validate_probe(unavailable, case, 3) == 'gpu_unavailable', 'unavailable GPU')
        need(refused(worker.validate_probe, good, case, 3), 'exit 3 without a GPU failure')
        mutations = [
            ('schema', lambda v: v.update(schema='mhgp9_gpu_filter_probe_v0')),
            ('extra key', lambda v: v.update(extra=1)),
            ('hash', lambda v: v['input'].update(hash='0' * 16)),
            ('sites', lambda v: v['input'].update(sites=v['input']['sites'] - 1)),
            ('K', lambda v: v['options'].update(K=10)),
            ('workers', lambda v: v['options'].update(workers=24)),
            ('repeats', lambda v: v['options'].update(repeats=1)),
            ('cpu only', lambda v: v['options'].update(cpu_only=True)),
            ('negative count', lambda v: v['population'].update(pairs=-1)),
            ('bool count', lambda v: v['population'].update(rectangles=True)),
            ('survivors', lambda v: v['population'].update(pair_survivors=401)),
            ('lanes', lambda v: v['population'].update(q3_open=101)),
            ('cache mismatch', lambda v: v['cpu'].update(cache_mismatches=1)),
            ('nan time', lambda v: v['cpu'].update(rect_ms=float('nan'))),
            ('device', lambda v: v['gpu'].update(device='NVIDIA other')),
            ('error', lambda v: v['gpu'].update(error='late failure')),
            ('stack', lambda v: v['gpu'].update(stack_failure=True)),
            ('gpu pairs', lambda v: v['gpu'].update(pairs=399)),
            ('rect visits', lambda v: v['gpu'].update(rect_visits=v['gpu']['rect_visits'] + 1)),
            ('pair visits', lambda v: v['gpu'].update(pair_visits=v['gpu']['pair_visits'] - 1)),
            ('visits flag', lambda v: v['gpu'].update(visits_equal=False)),
            ('event sum', lambda v: v['gpu'].update(pair_ms=v['gpu']['pair_ms'] + 1.0)),
            ('best over first', lambda v: v['gpu'].update(first_total_ms=1.0)),
            ('rss', lambda v: v.update(peak_rss_kb=0)),
        ]
        for label, mutate in mutations:
            bad = deepcopy(good)
            mutate(bad)
            need(refused(worker.validate_probe, bad, case, 0), 'probe mutation ' + label)

    def test_plan_and_snapshot(self):
        pkg = package()
        manifest = pkg['manifest']
        cases, provenance = session.validate_snapshot(pkg['archive'], manifest)
        need([(c['scene'], c['k'], c['workers'], c['repeats']) for c in cases] ==
             [(scene, k, 48, 3) for scene in ('00', '01', '02') for k in (5, 10)], 'default S1 plan')
        need(pkg['record']['status'] == 'prepared_not_executed' and
             pkg['record']['real_session_allowed'] is pkg['committed'], 'package record')
        need(worker.PROBE_SOURCE in manifest and worker.RUNNER_SOURCE in manifest and
             worker.BASE_WORKER in manifest, 'GPU sources and library transported')
        plan = snapshot.default_plan()
        for key, value in [('k', 7), ('s', 10), ('workers', 0), ('repeats', 0), ('repeats', 11), ('n', 39884),
                           ('scene', '03'), ('file', 'data/scene_01.u32le'), ('extra', 1)]:
            bad = deepcopy(plan)
            bad['cases'][0][key] = value
            need(refused(worker.validate_plan, bad, manifest), 'plan mutation ' + key)
        bad = deepcopy(plan)
        bad['cases'].append(deepcopy(bad['cases'][0]))
        need(refused(worker.validate_plan, bad, manifest), 'duplicate case')
        again = snapshot.build('HEAD', pkg['directory'] / 'again', allow_uncommitted_protocol=not pkg['committed'])
        need(again['snapshot_sha256'] == pkg['record']['snapshot_sha256'], 'deterministic snapshot')
        if not pkg['committed']:
            need(refused(snapshot.build, 'HEAD', pkg['directory'] / 'strict'), 'uncommitted protocol refused')
            stream = io.StringIO()
            with redirect_stdout(stream):
                code = session.main(['--execute', '--snapshot', str(pkg['archive']), '--manifest',
                                     str(pkg['manifest_path']), '--worker', str(worker.__file__), '--session-dir', '/x',
                                     '--ssh-key', '/x', '--snapshot-sha256', worker.sha(pkg['archive']),
                                     '--manifest-sha256', worker.sha(pkg['manifest_path']), '--worker-sha256', 'x',
                                     '--expected-controller-sha256', 'x', '--gcloud', str(NEVER_RUN_GCLOUD)])
            need(code == 2 and json.loads(stream.getvalue())['status'] == 'refused', 'real session refused')
        del provenance

    def test_completed_session(self):
        with tempfile.TemporaryDirectory() as tmp:
            code, receipt, fake, host = run_scenario(Path(tmp))
            need(code == 0 and receipt['status'] == 'completed' and receipt['GPU_executed'] is True and
                 receipt['FULL_executed'] is False and receipt['backend'] == 'cuda_g4', 'completed receipt')
            expect_certified_stop(receipt, fake)
            output = host / 'received/output'
            value = worker.strict_json((output / 'receipt.json').read_bytes())
            need([entry['outcome'] for entry in value['case_outcomes']] == ['complete'] * 6, 'six complete cases')
            # Tampering with a raw probe output after the fact is refused.
            probe = output / 'probe_0.stdout'
            text = probe.read_text().replace('"pair_ms":10.0', '"pair_ms":11.0')
            probe.write_text(text)
            pkg = package()
            cases, provenance = session.validate_snapshot(pkg['archive'], pkg['manifest'])
            with patch.object(worker, 'CUDA_PATHS', (str(Path(tmp) / 'fakebin/nvcc'),)):
                need(refused(session.validate_received, output, pkg['manifest'], worker.sha(worker.__file__), cases,
                             receipt['generation'], provenance, receipt['verified_guard']), 'tampered output refused')

    def test_mismatch_is_received(self):
        with tempfile.TemporaryDirectory() as tmp:
            code, receipt, fake, _ = run_scenario(Path(tmp), tools=dict(mismatch=[dict(scene='01', k=10)]))
            need(code == 0 and receipt['status'] == 'gpu_mismatch' and receipt['GPU_executed'] is True,
                 'measured mismatch received and judged')
            expect_certified_stop(receipt, fake)

    def test_gate_case_stops_the_plan(self):
        with tempfile.TemporaryDirectory() as tmp:
            code, receipt, fake, host = run_scenario(Path(tmp), tools=dict(mismatch=[dict(scene='00', k=5)]))
            value = worker.strict_json((host / 'received/output/receipt.json').read_bytes())
            need(code == 0 and receipt['status'] == 'gpu_mismatch' and
                 [e['outcome'] for e in value['case_outcomes']] == ['gpu_mismatch'] + ['skipped_s1_gate'] * 5 and
                 not (host / 'received/output/probe_1.command.json').exists(), 'no case paid after a failed gate')
            expect_certified_stop(receipt, fake)

    def test_closed_gate_and_faults_are_received(self):
        for tools, status, first in ((dict(unavailable=[dict(scene='00', k=5)]), 's1_gate_failed', 'gpu_unavailable'),
                                     (dict(fault=[dict(scene='00', k=5)]), 'gpu_fault', 'gpu_fault'),
                                     (dict(fault=[dict(scene='02', k=10)]), 'gpu_fault', 'complete')):
            with tempfile.TemporaryDirectory() as tmp:
                code, receipt, fake, host = run_scenario(Path(tmp), tools=tools)
                value = worker.strict_json((host / 'received/output/receipt.json').read_bytes())
                need(code == 0 and receipt['status'] == status and value['case_outcomes'][0]['outcome'] == first,
                     'received negative result ' + status)
                expect_certified_stop(receipt, fake)

    def test_nothing_measured_is_a_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            code, receipt, fake, _ = run_scenario(Path(tmp), patches=((worker, 'MIN_CASE_START_SECONDS', 10 ** 9),))
            need(code == 1 and receipt['status'] == 'worker_failed' and receipt['worker_status'] == 'no_gpu_measurement',
                 'no measurement is never received')
            expect_certified_stop(receipt, fake)

    def test_failures_still_stop(self):
        for tools, expected in ((dict(fail_build=True), 'worker_failed'), (dict(fail_preflight=True), 'worker_failed'),
                                (dict(mutant_survives=True), 'worker_failed'),
                                (dict(malform=[dict(scene='00', k=5)]), 'worker_failed')):
            with tempfile.TemporaryDirectory() as tmp:
                code, receipt, fake, _ = run_scenario(Path(tmp), tools=tools)
                need(code == 1 and receipt['status'] == expected and receipt['GPU_executed'] is False,
                     'failure receipt ' + json.dumps(tools))
                expect_certified_stop(receipt, fake)


if __name__ == '__main__':
    unittest.main()
