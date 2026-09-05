#!/usr/bin/env python3
"""NEW-only FULLv3 mono capture, inert by default; ROOT GO and pins required.

The old 417ccc controller is imported byte-for-byte as an isolated module. Its
bounded process machinery, build, probe_attempt(selftests=False), command and
seal functions are reused. Its v2 micro and qualification routines are not.
No historical timing pair is made. Heavy actions execute one planned job each.
"""
from __future__ import annotations

import argparse
import fcntl
import hashlib
import json
from pathlib import Path
import re
import shlex
import signal
import sys
import types

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_successor_20260905_mono_controller'
IMPORTED = ROOT / 'build/v7_full_lazy_20260905_probe_controller/capture.py'
IMPORTED_SHA = '417ccc3b47bb7591405f3af99bf7591bf2019794aa4535077436ce4889c4adfa'
PROBE = 'morsehgp3D_v7/bench/full_gabriel_lazy_probe.cpp'
DIGEST = 'morsehgp3D_v7/bench/full_gabriel_semantic_digest.hpp'
JUDGE = 'morsehgp3D_v7/bench/full_gabriel_lazy_probe_audit.py'
FIRST_C = 'morsehgp3D_v7/bench/full_gabriel_cache_policy_audit.py'
PRODUCER = 'morsehgp3D_v7/src/forest/full_gabriel.hpp'
OBSOLETE = 'morsehgp3D_v7/bench/full_gabriel_probe.cpp'
OBSOLETE_SHA = '1fb757ed5afc18bb207657c709b6e8fe37b5e38f9e444d66cc7c38449c932ba0'
JUDGE_SHA = '5de5b8d20d6073d5521a4217b8d504a59d255fad514eb8f4ddc257553ee0f032'
QUALIFICATION_CONTROLLER_SHA = '265be9e2425078e18c288642610ad4d4fae9a5e18224a10f1651d57186281e8a'
SCHEMA = 'mhgp7-successor-mono-controller-v1'
PROBE_SCHEMA = 'mhgp7-full-gabriel-probe-v3'
ACCOUNTING = 'full_successor_reads_writes_no_last_pair_v2'
SEQUENCE = [[8000, 8, 'lazy', 1000000], [8000, 10, 'lazy', 1000000],
            [8000, 12, 'lazy', 1000000], [16000, 8, 'lazy', 1000000],
            [32000, 8, 'lazy', 1000000]]


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def encoded(value: object) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def same_typed(left: object, right: object, reason: str) -> None:
    require(encoded(left) == encoded(right), reason)


def terminal_binding(extended: dict, base: dict, receipt: dict, rows: list[dict]) -> None:
    same_typed(extended['base_verdict'], base, 'extended/base verdict mirror')
    require(extended['status'] == base['status'] == 'completed' and base['capture_valid'] is True,
            'noncompleted captured verdict')
    require(len(rows) >= 2 and rows[0]['type'] == 'configuration' and rows[-1]['type'] == 'terminal'
            and [row['type'] for row in rows[1:-1]] == ['order'] * (len(rows) - 2), 'terminal row inventory')
    same_typed(receipt['orders'], rows[1:-1], 'receipt/raw order mirror')
    same_typed(receipt['terminal'], rows[-1], 'receipt/raw terminal mirror')
    terminal = rows[-1]
    code = {'complete_relative': 0, 'invalid_input': 2, 'unsupported_degeneracy': 2,
            'resource_exhausted': 2, 'invariant_violated': 3}[terminal['outcome']]
    require(type(receipt['exit_code']) is int and receipt['exit_code'] == terminal['exit_code'] == code
            and base['attempt_success'] is (code == 0) and extended['attempt_success'] is (code == 0),
            'terminal/exit/success binding')
    require(terminal['terminal_status'] == ('completed' if code == 0 else 'failed')
            and terminal['complete_requested_horizontal_orders'] is (code == 0), 'terminal completion binding')
    for key in ('outcome', 'input_digest', 'certificate_digest'):
        same_typed(base[key], terminal[key], 'base/raw ' + key)
        same_typed(extended[key], terminal[key], 'extended/raw ' + key)


def metadata_selftest() -> dict:
    # These explicitly synthetic metadata models are not engine captures.
    plan = {'binary': 'new_only', 'kmax': 10, 'cap': 128000000, 'enabled': True}
    same_typed(plan, dict(plan), 'plan positive')
    terminal = {'type': 'terminal', 'outcome': 'complete_relative', 'exit_code': 0,
                'terminal_status': 'completed', 'complete_requested_horizontal_orders': True,
                'input_digest': 'input_model', 'certificate_digest': 'forest_model'}
    rows = [{'type': 'configuration'}, {'type': 'order'}, terminal]
    receipt = {'orders': rows[1:-1], 'terminal': terminal, 'exit_code': 0}
    base = {'status': 'completed', 'capture_valid': True, 'attempt_success': True,
            'outcome': terminal['outcome'], 'input_digest': terminal['input_digest'],
            'certificate_digest': terminal['certificate_digest']}
    extended = dict(base, base_verdict=base)
    terminal_binding(extended, base, receipt, rows)
    killed = []
    for key, bad in (('kmax', 5), ('binary', 'other_binary'), ('cap', 256000000),
                     ('enabled', 1), ('kmax', 10.0)):
        altered = dict(plan, **{key: bad})
        try:
            same_typed(altered, plan, 'model plan differs')
        except ValueError:
            killed.append('plan_' + key + '_' + repr(bad))
    for name in ('false_success', 'false_exit', 'false_terminal', 'false_base', 'false_orders'):
        ext, actual, rec, raw_rows = (json.loads(json.dumps(value)) for value in (extended, base, receipt, rows))
        if name == 'false_success': ext['attempt_success'] = False
        elif name == 'false_exit': rec['exit_code'] = 2
        elif name == 'false_terminal': rec['terminal']['certificate_digest'] = 'other'
        elif name == 'false_base': ext['base_verdict']['capture_valid'] = False
        else: rec['orders'] = []
        try:
            terminal_binding(ext, actual, rec, raw_rows)
        except ValueError:
            killed.append(name)
    require(len(killed) == 10, 'metadata selftest nonvacuum')
    return {'status': 'selftests_passed', 'positive_models': 2, 'mutants_killed': killed,
            'models_are_engine_receipts': False, 'engine_runs': 0}


def source_files() -> dict[str, str]:
    paths = {path for path in (ROOT / 'morsehgp3D_v7/src').rglob('*') if path.is_file()}
    paths.update(ROOT / name for name in (PROBE, DIGEST, JUDGE, FIRST_C, OBSOLETE,
                                          'morsehgp3D_v7/CMakeLists.txt'))
    paths.update((Path(__file__).resolve(), IMPORTED))
    require(len(paths) < 2048 and all(not path.is_symlink() for path in paths), 'source inventory')
    return {str(path.relative_to(ROOT)): sha(path.read_bytes()) for path in sorted(paths)}


class Controller:
    def __init__(self, own_pin: str, source_pin: str, closed_sources: Path | None = None):
        own = Path(__file__).resolve()
        require(re.fullmatch('[0-9a-f]{64}', source_pin or '') is not None
                and sha(own.read_bytes()) == own_pin, 'reviewed controller/source pins required')
        imported = IMPORTED.read_bytes()
        require(sha(imported) == IMPORTED_SHA, 'imported controller pin differs')
        c = types.ModuleType('inert_successor_probe_controller')
        c.__file__ = str(IMPORTED)
        exec(compile(imported, str(IMPORTED), 'exec'), c.__dict__)
        self.c, self.own_pin, self.source_pin = c, own_pin, source_pin
        reviewed = (c.read_json(closed_sources)['files'] if closed_sources is not None else source_files())
        require(sha(encoded(reviewed)) == source_pin and reviewed[JUDGE] == JUDGE_SHA
                and reviewed[OBSOLETE] == OBSOLETE_SHA
                and reviewed[str(own.relative_to(ROOT))] == own_pin,
                'source admission fingerprint or fixed authority differs')
        self.reviewed = reviewed
        c.BASE, c.SCHEMA = BASE, SCHEMA
        c.PINS = {name: reviewed[name] for name in (PROBE, DIGEST, JUDGE, PRODUCER)}
        c.snapshot = self.snapshot
        original_protocol, original_metadata = c.protocol, c.metadata

        def protocol(binary: Path, k: int, sequence: list) -> dict:
            value = original_protocol(binary, k, sequence)
            value.update(schema='mhgp7-full-gabriel-mono-observation-v3', probe_schema=PROBE_SCHEMA,
                         successor_accounting=ACCOUNTING, first_c_sha256=reviewed[FIRST_C],
                         measurement_kind='new_only_no_historical_timing_pair')
            return value

        def metadata(directory: Path) -> None:
            original_metadata(directory)
            saved = directory / 'protocol_sources'
            saved.mkdir()
            for label, path, pin in (
                ('capture.py', own, own_pin), ('imported_controller.py', IMPORTED, IMPORTED_SHA),
                ('probe.cpp', ROOT / PROBE, reviewed[PROBE]),
                ('semantic_digest.hpp', ROOT / DIGEST, reviewed[DIGEST]),
                ('full_gabriel.hpp', ROOT / PRODUCER, reviewed[PRODUCER]),
                ('primary_judge.py', ROOT / JUDGE, reviewed[JUDGE]),
                ('first_c_judge.py', ROOT / FIRST_C, reviewed[FIRST_C]),
                ('obsolete_probe.cpp', ROOT / OBSOLETE, reviewed[OBSOLETE]),
            ):
                raw = path.read_bytes()
                require(sha(raw) == pin, 'protocol copy drift')
                with (saved / label).open('xb') as stream:
                    stream.write(raw)

        c.protocol, c.metadata = protocol, metadata

    def snapshot(self, binary: Path | None = None) -> dict:
        current = source_files()
        require(current == self.reviewed and sha(encoded(current)) == self.source_pin,
                'reviewed source drift')
        value = {'files': current, 'source_sha256': self.source_pin,
                 'probe_schema': PROBE_SCHEMA, 'successor_accounting': ACCOUNTING}
        if binary is not None:
            require(binary.resolve().is_relative_to(BASE), 'NEW-only binary scope')
            value.update(binary=str(binary.relative_to(ROOT)), binary_sha256=self.c.sha(binary))
        return value

    def build(self, name: str) -> None:
        c, ordinary = self.c, self.c.checked

        def checked(directory, label, argv, expected=0, timeout=30):
            result = ordinary(directory, label, argv, expected, timeout)
            if label == 'compile':
                rejected = directory / 'obsolete_full_gabriel_probe'
                require(not rejected.exists(), 'obsolete rejection output must be absent')
                command = ['/usr/bin/taskset', '-c', '0', '/usr/bin/g++', '-std=c++20',
                           '-O3', '-DNDEBUG', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                           '-pthread', str(ROOT / OBSOLETE), '-o', str(rejected)]
                refusal = c.command(directory, 'obsolete_probe_compile', command, (1,), 600)
                stderr = (directory / 'obsolete_probe_compile.stderr').read_bytes()
                require(refusal['status'] == 'completed' and refusal['exit_code'] == 1
                        and b'mhgp7_obsolete_full_probe_calendar' in stderr and not rejected.exists(),
                        'obsolete probe must fail compilation with its exact calendar tag and no ELF')
                c.save(directory / 'obsolete_probe_rejection.json', {
                    'status': 'expected_compile_rejection', 'exit_code': 1,
                    'tag': 'mhgp7_obsolete_full_probe_calendar', 'output_absent': True,
                    'source_sha256': OBSOLETE_SHA,
                })
            return result

        c.checked = checked
        try:
            c.build(name)
        finally:
            c.checked = ordinary

    def admission(self, argument: str, kind: str) -> tuple[Path, dict]:
        written, sep, pin = argument.rpartition('=')
        require(sep and re.fullmatch('[0-9a-f]{64}', pin) is not None, 'PATH=SHA256 required')
        path, value = self.c.verify_receipt(written, pin, kind)
        require(value['schema'] == SCHEMA, 'NEW-only admission schema')
        if kind == 'build':
            require(value['dependency_count'] == 40, 'exact forty compiled user dependencies')
            rejection = self.c.read_json(path.parent / 'obsolete_probe_rejection.json')
            require(rejection == {'status': 'expected_compile_rejection', 'exit_code': 1,
                    'tag': 'mhgp7_obsolete_full_probe_calendar', 'output_absent': True,
                    'source_sha256': OBSOLETE_SHA}, 'obsolete compile rejection admission')
            require(not (path.parent / 'obsolete_full_gabriel_probe').exists(), 'obsolete ELF appeared')
        return path, value

    def version(self, value: dict) -> None:
        require(value.get('probe_schema', value.get('schema')) == PROBE_SCHEMA
                and value.get('successor_accounting') == ACCOUNTING, 'explicit v3/work-v2 binding')

    def probe(self, directory: Path, index: int, selftests: bool = False) -> dict:
        c = self.c
        value = c.probe_attempt(directory, index, selftests=False)
        result = {'status': 'failed', 'started': c.now(), 'base_verdict': value,
                  'attempt_success': False, 'attempt_id': value['attempt_id']}
        try:
            require(value['status'] == 'completed', 'missing/invalid/censored probe terminal')
            label = value['attempt_id']
            receipt = directory / (label + '.receipt.json')
            for optimized in (False, True):
                tag = 'optimized' if optimized else 'normal'
                self.version(c.read_json(directory / (label + '.judge_' + tag + '.stdout')))
                python = ['/usr/bin/taskset', '-c', '0', sys.executable, '-B']
                if optimized:
                    python.append('-O')
                c.checked(directory, label + '.first_c_' + tag,
                          python + [str(ROOT / FIRST_C), str(receipt)])
                first_c = c.read_json(directory / (label + '.first_c_' + tag + '.stdout'))
                self.version(first_c)
                require(first_c['supplement_status'] == 'valid'
                        and first_c['attempt_success'] is value['attempt_success']
                        and first_c['requires_frozen_judge_sha256'] == JUDGE_SHA,
                        'first-C versioned judgment')
                if selftests:
                    require(value['attempt_success'] is True, 'selftests need a real successful capture')
                    for prefix, script, count, status_key in (
                        ('primary', JUDGE, 35, 'audit_status'),
                        ('first_c', FIRST_C, 27, 'supplement_status'),
                    ):
                        target = label + '.' + prefix + '_selftest_' + tag
                        c.checked(directory, target, python + [str(ROOT / script), '--selftest', str(receipt)])
                        model = c.read_json(directory / (target + '.stdout'))
                        self.version(model)
                        require(model[status_key] == 'selftests_passed'
                                and len(model['mutants_killed']) == len(set(model['mutants_killed'])) == count,
                                'versioned judge selftest nonvacuum')
            result.update(status='completed', attempt_success=value['attempt_success'],
                          outcome=value['outcome'], input_digest=value['input_digest'],
                          certificate_digest=value['certificate_digest'])
        except BaseException as error:
            result['reason'] = type(error).__name__ + ': ' + str(error)
        finally:
            try:
                binary = ROOT / c.read_json(directory / 'protocol.json')['binary']
                require(self.snapshot(binary) == c.read_json(directory / 'sources_before.json'),
                        'post-extended-judge source drift')
            except BaseException as error:
                result.update(status='failed', reason=type(error).__name__ + ': ' + str(error))
            result['ended'] = c.now()
            c.save(directory / (value['attempt_id'] + '.extended_verdict.json'), result)
        return result

    def micro(self, name: str, build_argument: str) -> None:
        c = self.c
        build_path, build = self.admission(build_argument, 'build')
        binary = ROOT / build['binary']
        directory = c.new_directory('micro', name)
        before = self.snapshot(binary)
        c.save(directory / 'sources_before.json', before)
        result = {'kind': 'micro', 'status': 'failed', 'started': c.now(), 'attempts': [],
                  'build_argument': build_argument, 'binary': build['binary'],
                  'binary_sha256': build['binary_sha256'], 'parser_rejects': 0}
        try:
            c.metadata(directory)
            metadata_models = []
            for optimized in (False, True):
                tag = 'optimized' if optimized else 'normal'
                python = ['/usr/bin/taskset', '-c', '0', sys.executable, '-B']
                if optimized:
                    python.append('-O')
                c.checked(directory, 'controller_selftest_' + tag,
                          python + [str(Path(__file__).resolve()), '--selftest'])
                model = c.read_json(directory / ('controller_selftest_' + tag + '.stdout'))
                require(model['status'] == 'selftests_passed' and model['positive_models'] == 2
                        and len(model['mutants_killed']) == 10 and model['engine_runs'] == 0
                        and model['models_are_engine_receipts'] is False, 'controller metadata model admission')
                metadata_models.append(model)
            same_typed(metadata_models[0], metadata_models[1], 'metadata normal/-O differ')
            c.checked(directory, 'digest_selftest', ['/usr/bin/taskset', '-c', '0', str(binary), '--digest-selftest'])
            model = c.read_json(directory / 'digest_selftest.stdout')
            self.version(model)
            require(model['type'] == 'digest_selftest' and model['passed'] is True
                    and model['checks'] == model['expected_checks'] == 24 and model['failures'] == 0,
                    'digest selftest nonvacuum')
            base, eager = ['--n=8', '--s=8', '--kmax=10'], ['--alias-policy=eager']
            negatives = {
                'missing': ['--n=8', '--s=8'] + eager,
                'duplicate': ['--n=8', '--n=8', '--s=8', '--kmax=10'] + eager,
                'n9': ['--n=9', '--s=8', '--kmax=10'] + eager,
                's9': ['--n=8', '--s=9', '--kmax=10'] + eager,
                'k0': ['--n=8', '--s=8', '--kmax=0'] + eager,
                'unknown': base + eager + ['--unknown'], 'policy_missing': base,
                'policy_duplicate': base + eager + eager,
                'cache_eager': base + eager + ['--cache-entries=0'],
                'lazy_no_cache': base + ['--alias-policy=lazy'],
                'cache_overflow': base + ['--alias-policy=lazy', '--cache-entries=1000001'],
            }
            for label, options in negatives.items():
                target = 'reject_' + label
                c.checked(directory, target, ['/usr/bin/taskset', '-c', '0', str(binary)] + options, expected=2)
                row = c.read_json(directory / (target + '.stdout'))
                self.version(row)
                require((directory / (target + '.stderr')).stat().st_size == 0 and row['type'] == 'terminal'
                        and row['terminal_status'] == 'failed' and row['outcome'] == 'invalid_input'
                        and row['reason'] == 'probe_arguments' and row['exit_code'] == 2
                        and row['completed_orders_diagnostic'] == 0 and row['certificate_digest'] == ''
                        and row['complete_requested_horizontal_orders'] is False, 'parser rejection')
            result['parser_rejects'] = len(negatives)
            orders = 0
            for k in (5, 10):
                section = directory / f'k{k}'
                section.mkdir()
                sequence = [[8, s, policy, cap] for s in (8, 10, 12)
                            for policy, cap in (('eager', 0), ('lazy', 0), ('lazy', 1), ('lazy', 1000000))]
                c.prepare_directory(section, c.protocol(binary, k, sequence), binary)
                values = []
                for index in range(12):
                    value = self.probe(section, index, selftests=k == 10 and index == 0)
                    result['attempts'].append(value)
                    require(value['status'] == 'completed' and value['attempt_success'], 'micro attempt failed')
                    values.append(value)
                    orders += len(c.read_json(section / (value['attempt_id'] + '.receipt.json'))['orders'])
                require(len({row['input_digest'] for row in values}) == 1
                        and len({row['certificate_digest'] for row in values}) == 1, 'micro digest mismatch')
            require(len(result['attempts']) == 24 and orders == 156 and result['parser_rejects'] == 11,
                    'micro nonvacuum')
            result.update(status='completed', reason='24_v3_micro_35_primary_27_firstC_mutants',
                          horizontal_orders=orders, primary_mutants_per_mode=35, first_c_mutants_per_mode=27)
        except BaseException as error:
            result['reason'] = type(error).__name__ + ': ' + str(error)
        finally:
            c.seal(directory, result, before, binary)
        print(json.dumps(result, sort_keys=True))
        require(result['status'] == 'completed', 'micro admission failed')

    def qualification(self, argument: str) -> dict:
        c = self.c
        written, sep, pin = argument.rpartition('=')
        require(sep and re.fullmatch('[0-9a-f]{64}', pin) is not None, 'qualification PATH=SHA256')
        path = Path(written)
        require(path.is_absolute() and path.name == 'receipt.json'
                and path.resolve().is_relative_to(ROOT / 'build/v7_successor_20260905_controller')
                and c.sha(path) == pin, 'qualification source pin')
        value = c.read_json(path)
        require(value['schema'] == 'mhgp7-successor-qualification-v1' and value['kind'] == 'qualification'
                and value['status'] == 'completed' and value['errors'] == [] and value['development'] is False
                and value['public_status'] == 'not_claimed' and value['gcp_used'] is False
                and value['historical_results_reused'] is False
                and value['controller_sha256'] == QUALIFICATION_CONTROLLER_SHA
                and value['producer_sha256'] == self.reviewed[PRODUCER], 'twenty-test qualification admission')
        for name, expected in value['artifacts'].items():
            target = (path.parent / name).resolve()
            require(target.is_relative_to(path.parent) and c.sha(target) == expected, 'qualification artifact drift')
        require({str(p.relative_to(path.parent)) for p in path.parent.rglob('*') if p.is_file()}
                == set(value['artifacts']) | {'receipt.json'}, 'qualification exact inventory')
        before = c.read_json(path.parent / 'sources_before.json')
        require(sha(encoded(before)) == value['source_sha256']
                and c.read_json(path.parent / 'sources_after.json') == before, 'qualification source binding')
        for name, expected in self.reviewed.items():
            if name in before:
                require(before[name] == expected, 'qualified/live source mismatch:' + name)
        require(before[PRODUCER] == self.reviewed[PRODUCER] and before[DIGEST] == self.reviewed[DIGEST]
                and c.sha(path.parent / 'protocol/capture.py') == QUALIFICATION_CONTROLLER_SHA,
                'qualification producer/digest/protocol binding')
        expected_tests = sorted(c.QUALIFIED_TESTS + [
            'mhgp7_full_gabriel_singleton', 'mhgp7_full_gabriel_singleton_rejects',
            'mhgp7_full_gabriel_singleton_bad_argument', 'mhgp7_full_gabriel_successor',
            'mhgp7_full_gabriel_successor_rejects', 'mhgp7_full_gabriel_successor_bad_argument'])
        require(sorted(value['tests']) == expected_tests and len(value['targets']) == 8
                and [p['mode'] for p in value['phases']] == ['release', 'san'], 'twenty-test phase inventory')
        for phase in value['phases']:
            require(phase['status'] == 'completed' and phase['errors'] == []
                    and all(phase[key] is True for key in ('sources_stable', 'binaries_stable',
                            'compile_binding_stable', 'toolchain_stable'))
                    and phase['source_sha256'] == value['source_sha256'], 'qualification stability')
            inventory, outputs = phase['inventory'], phase['test_outputs']
            require(inventory['tests'] == 20 and inventory['names'] == expected_tests
                    and inventory['commands_codes_and_timeouts_exact'] is True, 'qualification CTest inventory')
            require(outputs['fresh'] is True and outputs['junit']['all_run'] is True
                    and outputs['last_test']['all_blocks_passed'] is True
                    and outputs['last_test']['terminal_footer'] is True, 'qualification fresh raw terminal')
            for key in ('junit', 'last_test'):
                require(outputs[key]['tests'] == 20 and outputs[key]['names'] == expected_tests,
                        'qualification raw test count')
            require(all(row['status'] == 'completed' and row['exit_code'] == 0 for row in phase['commands']),
                    'qualification command refusal')
            for copy in outputs['copies'].values():
                target = Path(copy['archive']['path'])
                require(target.resolve().is_relative_to(path.parent)
                        and c.sha(target) == copy['archive']['sha256'] == copy['source']['sha256'],
                        'qualification byte copy binding')
        return {'path': str(path), 'sha256': pin, 'source_sha256': value['source_sha256'],
                'producer_sha256': value['producer_sha256'], 'tests_per_mode': 20}

    def prepare(self, name: str, micro: str, qualification: str) -> None:
        c = self.c
        micro_path, admitted = self.admission(micro, 'micro')
        self.admission(admitted['build_argument'], 'build')
        require(len(admitted['attempts']) == 24 and admitted['horizontal_orders'] == 156
                and admitted['primary_mutants_per_mode'] == 35 and admitted['first_c_mutants_per_mode'] == 27,
                'micro gate counts')
        qualified = self.qualification(qualification)
        directory = c.new_directory('heavy', name)
        binary = ROOT / admitted['binary']
        c.prepare_directory(directory, c.protocol(binary, 10, SEQUENCE), binary)
        c.metadata(directory)
        c.save(directory / 'admission.json', {'kind': 'heavy', 'phase': 'new_only', 'started': c.now(),
            'micro_argument': micro, 'qualification_argument': qualification,
            'qualification': qualified, 'no_probe_launched': True})
        with (directory / 'session.lock').open('xb'):
            pass
        print(json.dumps({'prepared': str(directory), 'jobs': 5, 'no_probe_launched': True}))

    def attempt(self, directory: Path, index: int, go: bool) -> None:
        c = self.c
        require(go, 'explicit heavy GO required')
        directory = c.owned(directory)
        with (directory / 'session.lock').open('rb') as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            require(not (directory / 'receipt.json').exists(), 'heavy already closed')
            binary, plan = self.heavy_binding(directory)
            require(0 <= index < len(SEQUENCE), 'planned heavy index')
            for previous_index in range(index):
                previous = self.read_attempt(directory, previous_index, binary, plan)
                require(previous['attempt_success'] is True, 'refused/censored predecessor blocks next run')
            result = self.probe(directory, index)
            print(json.dumps(result, sort_keys=True))
            require(result['status'] == 'completed', 'heavy capture invalid')

    def heavy_binding(self, directory: Path) -> tuple[Path, dict]:
        c = self.c
        admission = c.read_json(directory / 'admission.json')
        _, micro = self.admission(admission['micro_argument'], 'micro')
        _, build = self.admission(micro['build_argument'], 'build')
        require(micro['binary'] == build['binary'] and micro['binary_sha256'] == build['binary_sha256'],
                'micro/build binary mirror')
        binary = ROOT / build['binary']
        plan = c.read_json(directory / 'protocol.json')
        same_typed(plan, c.protocol(binary, 10, SEQUENCE), 'entire prepared protocol differs from admitted binary/plan')
        same_typed(c.read_json(directory / 'sources_before.json'), self.snapshot(binary),
                   'prepared source/binary binding differs')
        same_typed(self.qualification(admission['qualification_argument']), admission['qualification'],
                   'heavy qualification drift')
        return binary, plan

    def read_attempt(self, directory: Path, index: int, binary: Path, plan: dict) -> dict:
        c = self.c
        n, separation, policy, cache = SEQUENCE[index]
        label = f'n{n}_s{separation}_k10_{policy}_c{cache}'
        extended = c.read_json(directory / (label + '.extended_verdict.json'))
        base = c.read_json(directory / (label + '.verdict.json'))
        receipt = c.read_json(directory / (label + '.receipt.json'))
        command = c.read_json(directory / (label + '.command.json'))
        intent = c.read_json(directory / (label + '.intent.json'))
        for row in (extended, base):
            require(row['attempt_id'] == label, 'captured attempt identity')
        for key, value in intent.items():
            same_typed(command[key], value, 'command/intent mirror:' + key)
        for key, value in command.items():
            same_typed(receipt[key], value, 'receipt/command mirror:' + key)
        argv = ['timeout', '--signal=TERM', '--kill-after=10s', '600s', 'taskset', '-c', '6',
                '/usr/bin/time', '-v', plan['binary'], f'--n={n}', f'--s={separation}',
                '--kmax=10', f'--alias-policy={policy}', f'--cache-entries={cache}']
        same_typed(command['argv'], argv, 'actual command differs from planned attempt')
        require(command['command'] == shlex.join(argv) and command['status'] == 'completed'
                and command['error'] is None, 'captured command not completed')
        raw_path = directory / (label + '.raw.txt')
        require(raw_path.stat().st_size <= 1 << 20, 'captured raw stream bound')
        raw = raw_path.read_bytes()
        stream = command['streams'][raw_path.name]
        require(stream['bytes'] == len(raw) and stream['sha256'] == sha(raw), 'captured stream byte/hash mirror')
        rows = [c.parse(line) for line in raw.decode().splitlines() if line.startswith('{')]
        terminal_binding(extended, base, receipt, rows)
        for row in rows:
            self.version(row)
        expected_sources = self.snapshot(binary)
        for suffix in ('sources_before.json', 'sources_after.json'):
            same_typed(c.read_json(directory / (label + '.' + suffix)), expected_sources,
                       'attempt source/binary snapshot mirror:' + suffix)
        return extended

    def close(self, directory: Path) -> None:
        c = self.c
        directory = c.owned(directory)
        with (directory / 'session.lock').open('rb') as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            require(not (directory / 'receipt.json').exists(), 'already closed')
            admission, plan = c.read_json(directory / 'admission.json'), c.read_json(directory / 'protocol.json')
            before = c.read_json(directory / 'sources_before.json')
            result = {'kind': 'heavy', 'phase': 'new_only', 'status': 'failed', 'started': admission['started'],
                      'binary': plan['binary'], 'binary_sha256': plan['binary_sha256'],
                      'attempts': [], 'all_successful': False}
            binary = None
            try:
                binary, plan = self.heavy_binding(directory)
                for index in range(len(SEQUENCE)):
                    value = self.read_attempt(directory, index, binary, plan)
                    result['attempts'].append(value)
                    require(value['status'] == 'completed', 'invalid or interrupted capture')
                result['all_successful'] = all(row['attempt_success'] for row in result['attempts'])
                result.update(status='completed', reason='closed_new_only_captures_not_SLO_or_historical_pair')
            except BaseException as error:
                result['reason'] = type(error).__name__ + ': ' + str(error)
            c.seal(directory, result, before, binary)
            print(json.dumps(result, sort_keys=True))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    parser.add_argument('--snapshot', action='store_true')
    parser.add_argument('--selftest', action='store_true')
    parser.add_argument('--expected-controller-sha256')
    parser.add_argument('--expected-source-sha256')
    sub = parser.add_subparsers(dest='action')
    build = sub.add_parser('build'); build.add_argument('--id', required=True)
    micro = sub.add_parser('micro'); micro.add_argument('--id', required=True); micro.add_argument('--build', required=True)
    prepare = sub.add_parser('prepare-heavy'); prepare.add_argument('--id', required=True)
    prepare.add_argument('--micro', required=True); prepare.add_argument('--qualification', required=True)
    attempt = sub.add_parser('attempt'); attempt.add_argument('--directory', type=Path, required=True)
    attempt.add_argument('--index', type=int, required=True); attempt.add_argument('--go-reviewed-heavy', action='store_true')
    close = sub.add_parser('close-heavy'); close.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    require(sum((args.execute, args.snapshot, args.selftest)) <= 1, 'snapshot, selftest and execution are separate')
    if args.selftest:
        print(json.dumps(metadata_selftest(), sort_keys=True))
        return 0
    if not args.execute:
        result = {'status': 'prepared_not_executed', 'requires_ROOT_GO': True,
                  'controller_sha256': sha(Path(__file__).read_bytes())}
        if args.snapshot:
            files = source_files()
            result.update(sources=files, source_sha256=sha(encoded(files)), producer_sha256=files[PRODUCER])
        print(json.dumps(result, sort_keys=True))
        return 0
    require(args.action is not None, 'execution action required')
    saved = args.directory / 'sources_before.json' if args.action == 'close-heavy' else None
    controller = Controller(args.expected_controller_sha256, args.expected_source_sha256, saved)
    for sig in controller.c.SIGNALS:
        signal.signal(sig, controller.c.interrupted)
    if args.action == 'build': controller.build(args.id)
    elif args.action == 'micro': controller.micro(args.id, args.build)
    elif args.action == 'prepare-heavy': controller.prepare(args.id, args.micro, args.qualification)
    elif args.action == 'attempt': controller.attempt(args.directory, args.index, args.go_reviewed_heavy)
    else: controller.close(args.directory)
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except BaseException as error:
        if isinstance(error, SystemExit):
            raise
        print(json.dumps({'status': 'failed', 'reason': type(error).__name__ + ': ' + str(error)}))
        raise SystemExit(1)
