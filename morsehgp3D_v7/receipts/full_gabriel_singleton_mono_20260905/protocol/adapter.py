#!/usr/bin/env python3
"""Inert singleton-lot comparison adapter; explicit ROOT GO for every run.

The pinned old controller is loaded as an isolated module, never edited. Its
process, parser and FULLv2 judge machinery is reused. Snapshot semantics are
explicitly replaced: compiled dependencies and live dependencies are separate.
No build or execution occurs without --execute and a reviewed adapter pin.
The 17-test heavy qualification receipt must be supplied and admitted explicitly.
"""
from __future__ import annotations

import argparse
import copy
import fcntl
import hashlib
import json
from pathlib import Path
import re
import signal
import sys
import types

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_singleton_20260905_mono_controller'
OLD_BASE = ROOT / 'build/v7_full_lazy_20260905_probe_controller'
OLD_CONTROLLER = OLD_BASE / 'capture.py'
OLD_CONTROLLER_SHA = '417ccc3b47bb7591405f3af99bf7591bf2019794aa4535077436ce4889c4adfa'
OLD_BUILD = OLD_BASE / 'build_admission/receipt.json'
OLD_BUILD_SHA = 'da11c743e63fb63acd00c25ce02080671a34a43fa2fa9cd473bb3068df2712a3'
OLD_BINARY = OLD_BASE / 'build_admission/full_gabriel_lazy_probe'
OLD_BINARY_SHA = '1d5a38cea99555fd2db474ee43aff6ba1ee708208508cfa97c540774d0bb7e78'
OLD_PRODUCER_SHA = '13c6cc72ab5065d498827bf89c6bc2a321b5e896c93a60263de52b9d800a2627'
FIRST_C = ROOT / 'morsehgp3D_v7/bench/full_gabriel_cache_policy_audit.py'
FIRST_C_SHA = '8f8aed03755d9c92775566b21d4fdd9dcba31f171adf4b83e9802a988a450370'
QUALIFICATION_CONTROLLER = ROOT / 'build/v7_singleton_20260905_controller/capture.py'
QUALIFICATION_CONTROLLER_SHA = 'c829ab897fa962766e6e71773a9b11d4389e9f419660f35e0885a5b00dd4116e'
SCHEMA = 'mhgp7-singleton-mono-adapter-v1'
ORDER_MEASURES = {'build_ms', 'digest_ms', 'expand_ms', 'read_ms', 'release_ms',
                  'rss_mib_sample', 'hwm_mib_sample'}
TERMINAL_MEASURES = {
    'compute_read_release_ms_subtracted_diagnostic', 'digest_ms',
    'elapsed_before_terminal_ms', 'generation_rects_ms', 'generation_wspd_ms',
    'provisional_output_ms', 'rss_mib_sample', 'hwm_mib_sample', 'stage_ms',
}
SEQUENCE = [('old', 8), ('new', 8), ('new', 10), ('old', 10),
            ('old', 12), ('new', 12)]


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def digest(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def projected(config: dict, receipt: dict) -> str:
    require(ORDER_MEASURES <= set(receipt['orders'][0]), 'order measure inventory')
    require(TERMINAL_MEASURES <= set(receipt['terminal']), 'terminal measure inventory')
    # Canonical JSON retains scalar types: Python dict equality alone would
    # silently identify 1 with 1.0 or True in an otherwise unguarded counter.
    return json.dumps({
        'configuration': config,
        'orders': [{key: value for key, value in row.items() if key not in ORDER_MEASURES}
                   for row in receipt['orders']],
        'terminal': {key: value for key, value in receipt['terminal'].items()
                     if key not in TERMINAL_MEASURES},
        'engine_exit_code': receipt['exit_code'],
    }, sort_keys=True, separators=(',', ':'), allow_nan=False)


def source_binding(binary_sha: str, compiled_producer: str, old_arm: bool,
                   new_producer: str) -> None:
    if old_arm:
        require(binary_sha == OLD_BINARY_SHA and compiled_producer == OLD_PRODUCER_SHA,
                'old binary falsely bound to nonhistorical compiled producer')
    else:
        require(compiled_producer == new_producer, 'new binary compiled producer mismatch')


class Adapter:
    def __init__(self, own_sha: str, producer_sha: str, validate_live: bool = True):
        require(re.fullmatch('[0-9a-f]{64}', producer_sha) is not None
                and producer_sha != OLD_PRODUCER_SHA, 'new producer pin required')
        own = Path(__file__).resolve()
        require(digest(own.read_bytes()) == own_sha, 'adapter pin differs')
        source = OLD_CONTROLLER.read_bytes()
        require(digest(source) == OLD_CONTROLLER_SHA, 'reused controller pin differs')
        module = types.ModuleType('pinned_singleton_adapter_controller')
        module.__file__ = str(OLD_CONTROLLER)
        exec(compile(source, str(OLD_CONTROLLER), 'exec'), module.__dict__)
        self.c = module
        self.own_sha, self.producer_sha = own_sha, producer_sha
        require(module.sha(OLD_BUILD) == OLD_BUILD_SHA, 'old build pin differs')
        old = module.read_json(OLD_BUILD)
        require(old['kind'] == 'build' and old['status'] == 'completed'
                and old['sources_stable'] is True and old['binary_sha256'] == OLD_BINARY_SHA,
                'old compilation not admitted')
        for name, pin in old['artifacts'].items():
            path = (OLD_BUILD.parent / name).resolve()
            require(path.is_relative_to(OLD_BUILD.parent) and module.sha(path) == pin,
                    'old build artifact drift')
        self.dependencies = module.read_json(OLD_BUILD.parent / 'dependencies.json')
        require(len(self.dependencies) == 40
                and self.dependencies[module.PRODUCER] == OLD_PRODUCER_SHA,
                'old dependency inventory differs')
        self.protocols = {str(own.relative_to(ROOT)): own_sha,
                          str(OLD_CONTROLLER.relative_to(ROOT)): OLD_CONTROLLER_SHA,
                          str(FIRST_C.relative_to(ROOT)): FIRST_C_SHA,
                          module.JUDGE: module.PINS[module.JUDGE]}
        self.old_tool_pins = module.read_json(OLD_BUILD.parent / 'host.json')['tools']
        # Deliberate isolated-module configuration, never an edit of old files.
        module.BASE = BASE
        module.PINS = dict(module.PINS, **{module.PRODUCER: producer_sha})
        module.snapshot = self.snapshot
        original_metadata = module.metadata

        def metadata(directory: Path) -> None:
            original_metadata(directory)
            protocol = directory / 'adapter_protocol'
            protocol.mkdir()
            for label, path, pin in (
                ('adapter.py', own, own_sha),
                ('imported_controller.py', OLD_CONTROLLER, OLD_CONTROLLER_SHA),
                ('first_c.py', FIRST_C, FIRST_C_SHA),
                ('v2_judge.py', ROOT / module.JUDGE, module.PINS[module.JUDGE]),
                ('old_build_receipt.json', OLD_BUILD, OLD_BUILD_SHA),
            ):
                raw = path.read_bytes()
                require(digest(raw) == pin, 'protocol copy pin differs')
                with (protocol / label).open('xb') as stream:
                    stream.write(raw)
            with (protocol / 'old_dependencies.json').open('xb') as stream:
                stream.write((OLD_BUILD.parent / 'dependencies.json').read_bytes())

        module.metadata = metadata
        if validate_live:
            self.snapshot()

    def snapshot(self, binary: Path | None = None) -> dict:
        c = self.c
        live = {name: c.sha(ROOT / name) for name in self.dependencies}
        expected = dict(self.dependencies, **{c.PRODUCER: self.producer_sha})
        require(live == expected, 'live dependencies differ beyond reviewed producer')
        require(all(c.sha(ROOT / name) == pin for name, pin in self.protocols.items()),
                'comparison protocol drift')
        require(c.sha(OLD_BUILD) == OLD_BUILD_SHA and c.sha(OLD_BINARY) == OLD_BINARY_SHA,
                'old compiled witness drift')
        require(all(c.sha(Path(name).resolve()) == pin for name, pin in self.old_tool_pins.items()),
                'compiler or timing tool drift from old build')
        old_arm = binary is not None and binary.resolve() == OLD_BINARY
        require(binary is None or old_arm or binary.resolve().is_relative_to(BASE),
                'unreviewed binary scope')
        value = {
            'files': self.dependencies if old_arm else live,
            'live_dependency_files': live,
            'adapter_protocols': self.protocols,
            'compiled_dependency_scope': '40_MMD_user_dependencies_not_system_headers',
            'source_binding': 'old_binary_from_original_build_receipt' if old_arm
                              else 'new_binary_from_live_single_header_delta',
            'old_build_receipt_sha256': OLD_BUILD_SHA,
        }
        if binary is not None:
            value.update(binary=str(binary.relative_to(ROOT)), binary_sha256=c.sha(binary))
            source_binding(value['binary_sha256'], value['files'][c.PRODUCER],
                           old_arm, self.producer_sha)
        return value

    def seal(self, directory: Path, record: dict, before: dict) -> None:
        c = self.c
        try:
            after = self.snapshot()
            c.save(directory / 'live_after.json', after)
            record['sources_stable'] = before == after
            if before != after:
                record.update(status='failed', reason='source drift')
        except BaseException as error:
            record.update(status='failed', sources_stable=False,
                          source_error=type(error).__name__ + ': ' + str(error))
        record.update(schema=SCHEMA, ended=c.now(), public_status='not_claimed', gcp_used=False)
        record['artifacts'] = {str(path.relative_to(directory)): c.sha(path)
                               for path in sorted(directory.rglob('*')) if path.is_file()}
        c.save(directory / 'receipt.json', record)

    def read_closed(self, argument: str, kind: str) -> tuple[Path, dict]:
        written, separator, pin = argument.rpartition('=')
        require(separator and re.fullmatch('[0-9a-f]{64}', pin) is not None,
                'PATH=SHA256 required')
        path = self.c.owned(written)
        require(path.name == 'receipt.json' and self.c.sha(path) == pin, 'receipt pin differs')
        record = self.c.read_json(path)
        require(record['status'] == 'completed' and record['kind'] == kind
                and record['sources_stable'] is True, 'admission not completed')
        for name, expected in record['artifacts'].items():
            target = (path.parent / name).resolve()
            require(target.is_relative_to(path.parent) and self.c.sha(target) == expected,
                    'closed artifact drift')
        return path, record

    def supplement(self, source: Path, directory: Path, label: str) -> None:
        c = self.c
        for optimized in (False, True):
            tag = 'optimized' if optimized else 'normal'
            argv = ['/usr/bin/taskset', '-c', '0', sys.executable, '-B']
            if optimized:
                argv.append('-O')
            c.checked(directory, label + '.first_c_' + tag,
                      argv + [str(FIRST_C), str(source)])
            output = c.read_json(directory / (label + '.first_c_' + tag + '.stdout'))
            require(output['supplement_status'] == 'valid', 'first-C failed')

    def compare(self, left: Path, right: Path) -> dict:
        c = self.c
        a, b = c.read_json(left), c.read_json(right)
        raw_a = left.with_name(left.name.removesuffix('.receipt.json') + '.raw.txt')
        raw_b = right.with_name(right.name.removesuffix('.receipt.json') + '.raw.txt')
        config_a = c.parse(raw_a.read_text().splitlines()[0])
        config_b = c.parse(raw_b.read_text().splitlines()[0])
        require(projected(config_a, a) == projected(config_b, b),
                'old/new non-measure field mismatch')
        require(a['exit_code'] == b['exit_code'] == 0, 'refusal is not a completed comparison')
        return {'old_receipt_sha256': c.sha(left), 'new_receipt_sha256': c.sha(right),
                'orders_equal': len(a['orders']), 'all_non_measure_fields_equal': True,
                'old_elapsed_ms': a['terminal']['elapsed_before_terminal_ms'],
                'new_elapsed_ms': b['terminal']['elapsed_before_terminal_ms'],
                'old_full_ms': a['terminal']['stage_ms']['full'],
                'new_full_ms': b['terminal']['stage_ms']['full'],
                'input_digest': a['terminal']['input_digest'],
                'certificate_digest': a['terminal']['certificate_digest'],
                'scope': 'same_policy_same_caps_all_reported_algorithm_fields_not_geometry_proof'}

    def micro(self, name: str, build_argument: str) -> None:
        c = self.c
        build_path, build = self.read_closed(build_argument, 'build')
        c.verify_receipt(build_path, c.sha(build_path), 'build')
        new_binary = ROOT / build['binary']
        directory = c.new_directory('admission', name)
        before = self.snapshot()
        c.save(directory / 'live_before.json', before)
        result = {'kind': 'admission', 'status': 'failed', 'started': c.now(),
                  'new_build_argument': build_argument, 'comparisons': []}
        try:
            # The complete old admission machinery is reexecuted for the new
            # binary: 24 profiles, 11 parser rejects, 24 digest checks and both
            # modes of its 19-mutant FULLv2 selftest. No prior result is inherited.
            new_id = name + '_new'
            c.micro(new_id, str(build_path), c.sha(build_path))
            new_receipt = BASE / ('micro_' + new_id) / 'receipt.json'
            c.verify_receipt(new_receipt, c.sha(new_receipt), 'micro')
            result['new_micro_argument'] = str(new_receipt) + '=' + c.sha(new_receipt)
            c.metadata(directory)
            comparator_models = []
            for optimized in (False, True):
                tag = 'optimized' if optimized else 'normal'
                argv = ['/usr/bin/taskset', '-c', '0', sys.executable, '-B']
                if optimized:
                    argv.append('-O')
                argv += [str(Path(__file__).resolve()), '--execute',
                         '--expected-adapter-sha256', self.own_sha,
                         '--producer-sha256', self.producer_sha, 'comparator-selftest',
                         '--fixture', str(new_receipt.parent / 'k10/n8_s8_k10_lazy_c1.receipt.json')]
                c.checked(directory, 'comparator_selftest_' + tag, argv)
                checked = c.read_json(directory / ('comparator_selftest_' + tag + '.stdout'))
                require(checked['status'] == 'selftests_passed' and len(checked['field_mutants']) == 14
                        and checked['fingerprint_mutants'] == 3
                        and checked['models_are_engine_receipts'] is False,
                        'comparator selftest nonvacuum')
                comparator_models.append(checked)
            require(comparator_models[0] == comparator_models[1], 'comparator normal/-O mismatch')
            result['comparator_models'] = comparator_models
            for k in (5, 10):
                section = directory / f'old_k{k}'
                section.mkdir()
                sequence = [[8, s, policy, cap] for s in (8, 10, 12)
                            for policy, cap in (('eager', 0), ('lazy', 0),
                                                ('lazy', 1), ('lazy', 1000000))]
                plan = c.protocol(OLD_BINARY, k, sequence)
                plan['producer_sha256'] = OLD_PRODUCER_SHA
                c.prepare_directory(section, plan, OLD_BINARY)
                for index, (_, s, policy, cap) in enumerate(sequence):
                    verdict = c.probe_attempt(section, index,
                                              selftests=k == 10 and index == 0)
                    require(verdict['status'] == 'completed' and verdict['attempt_success'],
                            'old micro failed')
                    stem = f'n8_s{s}_k{k}_{policy}_c{cap}'
                    old_path = section / (stem + '.receipt.json')
                    new_path = new_receipt.parent / f'k{k}' / (stem + '.receipt.json')
                    self.supplement(old_path, section, stem)
                    self.supplement(new_path, directory, stem + '_new')
                    result['comparisons'].append(self.compare(old_path, new_path))
            require(len(result['comparisons']) == 24
                    and sum(row['orders_equal'] for row in result['comparisons']) == 156,
                    'paired micro nonvacuum')
            result.update(status='completed', reason='48_micro_same_policy_counter_and_digest_pairs')
        except BaseException as error:
            result['reason'] = type(error).__name__ + ': ' + str(error)
        finally:
            self.seal(directory, result, before)
        require(result['status'] == 'completed', 'micro admission failed')
        print(json.dumps(result, sort_keys=True))

    def qualification(self, argument: str) -> dict:
        c = self.c
        written, separator, pin = argument.rpartition('=')
        require(separator and re.fullmatch('[0-9a-f]{64}', pin) is not None,
                'qualification PATH=SHA256 required')
        path = Path(written)
        require(path.is_absolute() and path.name == 'receipt.json' and path.resolve().is_relative_to(
            ROOT / 'build/v7_singleton_20260905_controller'), 'qualification scope')
        require(c.sha(path) == pin, 'qualification receipt pin differs')
        value = c.read_json(path)
        require(value['schema'] == 'mhgp7-singleton-qualification-v1'
                and value['kind'] == 'qualification' and value['status'] == 'completed'
                and value['errors'] == [] and value['development'] is False
                and value['public_status'] == 'not_claimed' and value['gcp_used'] is False
                and value['historical_results_reused'] is False
                and value['producer_sha256'] == self.producer_sha
                and value['controller_sha256'] == QUALIFICATION_CONTROLLER_SHA,
                'singleton qualification not admitted')
        actual = {str(item.relative_to(path.parent)) for item in path.parent.rglob('*') if item.is_file()}
        require(actual == set(value['artifacts']) | {'receipt.json'}, 'qualification exact inventory')
        for name, expected in value['artifacts'].items():
            item = (path.parent / name).resolve()
            require(item.is_relative_to(path.parent) and c.sha(item) == expected,
                    'qualification artifact drift')
        require(c.sha(path.parent / 'protocol/capture.py') == QUALIFICATION_CONTROLLER_SHA,
                'archived qualification controller pin differs')
        before = c.read_json(path.parent / 'sources_before.json')
        canonical = (json.dumps(before, sort_keys=True, indent=2) + '\n').encode()
        require(digest(canonical) == value['source_sha256']
                and c.read_json(path.parent / 'sources_after.json') == before,
                'qualification source-map binding')
        live = self.snapshot()['live_dependency_files']
        require(set(live) - set(before) == {c.PROBE}
                and all(before[name] == expected for name, expected in live.items() if name in before),
                'qualified probe dependencies differ')
        names = sorted(c.QUALIFIED_TESTS + ['mhgp7_full_gabriel_singleton',
                       'mhgp7_full_gabriel_singleton_rejects',
                       'mhgp7_full_gabriel_singleton_bad_argument'])
        require(sorted(value['tests']) == names and len(value['targets']) == 7
                and [phase['mode'] for phase in value['phases']] == ['release', 'san'],
                'qualification phase/target/test inventory')
        for phase in value['phases']:
            require(phase['status'] == 'completed' and phase['errors'] == []
                    and all(phase[key] is True for key in ('sources_stable', 'binaries_stable',
                            'compile_binding_stable', 'toolchain_stable'))
                    and phase['source_sha256'] == value['source_sha256'],
                    'qualification phase stability')
            inventory = phase['inventory']
            require(inventory['tests'] == 17 and inventory['names'] == names
                    and inventory['commands_codes_and_timeouts_exact'] is True,
                    'qualification CTest inventory')
            outputs = phase['test_outputs']
            require(outputs['fresh'] is True and outputs['junit']['all_run'] is True
                    and outputs['last_test']['all_blocks_passed'] is True
                    and outputs['last_test']['terminal_footer'] is True,
                    'qualification fresh terminal outputs')
            for key in ('junit', 'last_test'):
                require(outputs[key]['tests'] == 17 and outputs[key]['names'] == names,
                        'qualification raw output test inventory')
            require(all(row['status'] == 'completed' and row['exit_code'] == 0
                        for row in phase['commands']), 'qualification command failure')
            for copied in outputs['copies'].values():
                archive = Path(copied['archive']['path'])
                require(archive.resolve().is_relative_to(path.parent)
                        and c.sha(archive) == copied['archive']['sha256'] == copied['source']['sha256'],
                        'qualification raw copy binding')
        return {'path': str(path), 'sha256': pin, 'producer_sha256': self.producer_sha,
                'controller_sha256': QUALIFICATION_CONTROLLER_SHA,
                'source_sha256': value['source_sha256'], 'tests_per_mode': 17}

    def prepare_heavy(self, name: str, micro: str, qualification: str) -> None:
        c = self.c
        micro_path, admitted = self.read_closed(micro, 'admission')
        require(len(admitted['comparisons']) == 24 and all(
            item['all_non_measure_fields_equal'] is True for item in admitted['comparisons']),
            'micro comparisons not admitted')
        build_path, build = self.read_closed(admitted['new_build_argument'], 'build')
        c.verify_receipt(build_path, c.sha(build_path), 'build')
        qualified = self.qualification(qualification)
        directory = c.new_directory('paired', name)
        before = self.snapshot()
        c.save(directory / 'live_before.json', before)
        c.metadata(directory)
        sequence = []
        for index, (arm, separation) in enumerate(SEQUENCE):
            label = f'{index:02d}_{arm}_s{separation}'
            child = directory / label
            child.mkdir()
            binary = OLD_BINARY if arm == 'old' else ROOT / build['binary']
            plan = c.protocol(binary, 10, [[8000, separation, 'lazy', 1000000]])
            plan['producer_sha256'] = OLD_PRODUCER_SHA if arm == 'old' else self.producer_sha
            plan['comparison_kind'] = 'single_header_delta_same_lazy_policy_and_all_caps'
            c.prepare_directory(child, plan, binary)
            sequence.append({'label': label, 'arm': arm, 's': separation,
                             'binary_sha256': c.sha(binary)})
        c.save(directory / 'admission.json', {
            'schema': SCHEMA, 'kind': 'paired', 'started': c.now(),
            'micro_argument': micro, 'qualification_argument': qualification,
            'qualification': qualified, 'sequence': sequence,
            'order_measure_exclusions': sorted(ORDER_MEASURES),
            'terminal_measure_exclusions': sorted(TERMINAL_MEASURES),
            'reference_scope': 'reported_horizontal_orders_not_integrated_inter_K_tower',
            'no_engine_launched': True,
        })
        with (directory / 'session.lock').open('xb'):
            pass
        c.save(directory / 'prepared.json', {
            'artifacts': {str(path.relative_to(directory)): c.sha(path)
                          for path in sorted(directory.rglob('*')) if path.is_file()},
            'sources_stable': self.snapshot() == before,
        })
        print(json.dumps({'prepared': str(directory), 'planned_attempts': 6,
                          'no_engine_launched': True, 'explicit_GO_per_attempt': True}))

    def heavy_context(self, directory: Path) -> dict:
        c = self.c
        directory = c.owned(directory)
        require(not (directory / 'receipt.json').exists(), 'paired capture already closed')
        prepared = c.read_json(directory / 'prepared.json')
        require(prepared['sources_stable'] is True, 'preparation source drift')
        for name, pin in prepared['artifacts'].items():
            require(c.sha(directory / name) == pin, 'prepared artifact drift')
        require(c.read_json(directory / 'live_before.json') == self.snapshot(), 'paired live source drift')
        admission = c.read_json(directory / 'admission.json')
        _, micro = self.read_closed(admission['micro_argument'], 'admission')
        build_path, _ = self.read_closed(micro['new_build_argument'], 'build')
        c.verify_receipt(build_path, c.sha(build_path), 'build')
        require(self.qualification(admission['qualification_argument']) == admission['qualification'],
                'paired qualification drift')
        require([(row['arm'], row['s']) for row in admission['sequence']] == SEQUENCE,
                'paired sequence differs')
        return admission

    def heavy_attempt(self, directory: Path, index: int, go: bool) -> None:
        c = self.c
        require(go, 'explicit per-attempt heavy GO required')
        directory = c.owned(directory)
        with (directory / 'session.lock').open('rb') as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            admission = self.heavy_context(directory)
            require(0 <= index < len(SEQUENCE), 'heavy attempt index')
            for previous in admission['sequence'][:index]:
                old = c.read_json(directory / (previous['label'] + '.adapter_verdict.json'))
                require(old['status'] == 'completed' and old['attempt_success'] is True,
                        'unsuccessful predecessor blocks continuation')
            cell = admission['sequence'][index]
            child = directory / cell['label']
            result = {'status': 'failed', 'attempt_success': False, 'started': c.now()}
            try:
                value = c.probe_attempt(child, 0)
                require(value['status'] == 'completed', 'nonterminal or invalid heavy capture')
                stem = value['attempt_id']
                path = child / (stem + '.receipt.json')
                self.supplement(path, child, stem)
                result.update(status='completed', attempt_success=value['attempt_success'],
                              attempt_receipt=str(path), receipt_sha256=c.sha(path),
                              base_verdict=value, index=index, arm=cell['arm'], s=cell['s'])
                if value['attempt_success'] and index % 2:
                    previous_cell = admission['sequence'][index - 1]
                    previous = c.read_json(directory / (previous_cell['label'] + '.adapter_verdict.json'))
                    first, second = Path(previous['attempt_receipt']), path
                    if cell['arm'] == 'old':
                        first, second = second, first
                    result['comparison'] = self.compare(first, second)
            except BaseException as error:
                result.update(status='failed', reason=type(error).__name__ + ': ' + str(error))
            result['ended'] = c.now()
            c.save(directory / (cell['label'] + '.adapter_verdict.json'), result)
            print(json.dumps(result, sort_keys=True))
            require(result['status'] == 'completed', 'heavy capture or comparison failed')

    def close_heavy(self, directory: Path) -> None:
        c = self.c
        directory = c.owned(directory)
        with (directory / 'session.lock').open('rb') as lock:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
            require(not (directory / 'receipt.json').exists(), 'already closed')
            admission = c.read_json(directory / 'admission.json')
            before = c.read_json(directory / 'live_before.json')
            record = {'kind': 'paired', 'status': 'failed', 'started': admission['started'],
                      'attempts': [], 'all_successful': False, 'comparisons': []}
            try:
                self.heavy_context(directory)
                for cell in admission['sequence']:
                    value = c.read_json(directory / (cell['label'] + '.adapter_verdict.json'))
                    record['attempts'].append(value)
                    require(value['status'] == 'completed', 'invalid or interrupted capture')
                    if 'comparison' in value:
                        record['comparisons'].append(value['comparison'])
                record['all_successful'] = all(row['attempt_success'] for row in record['attempts'])
                if record['all_successful']:
                    require(len(record['comparisons']) == 3
                            and sum(row['orders_equal'] for row in record['comparisons']) == 30
                            and len({row['certificate_digest'] for row in record['comparisons']}) == 1
                            and len({row['input_digest'] for row in record['comparisons']}) == 1,
                            'paired terminal nonvacuum or cross-s digest mismatch')
                record.update(status='completed', reason='closed_captures_not_SLO_or_allocation_measurement')
            except BaseException as error:
                record['reason'] = type(error).__name__ + ': ' + str(error)
            self.seal(directory, record, before)
            print(json.dumps(record, sort_keys=True))

    def comparator_selftest(self, fixture: Path) -> dict:
        c = self.c
        record = c.read_json(fixture)
        raw_path = fixture.with_name(fixture.name.removesuffix('.receipt.json') + '.raw.txt')
        raw = raw_path.read_bytes()
        require(record['streams'][raw_path.name]['sha256'] == digest(raw)
                and record['exit_code'] == 0 and record['orders'], 'real comparator fixture binding')
        config = c.parse(raw.decode().splitlines()[0])
        baseline = projected(config, record)
        noisy = copy.deepcopy(record)
        for row in noisy['orders']:
            for name in ORDER_MEASURES:
                row[name] += 17
        for name in TERMINAL_MEASURES:
            if name == 'stage_ms':
                noisy['terminal'][name] = {key: value + 17
                                          for key, value in noisy['terminal'][name].items()}
            else:
                noisy['terminal'][name] += 17
        require(projected(config, noisy) == baseline, 'measurement-only positive rejected')
        killed = []
        for key in ('input_records', 'face_visits', 'successor_steps', 'chain_steps',
                    'minimum_hits', 'cache_inserts', 'direct_lookups', 'certificate_digest'):
            altered = copy.deepcopy(record)
            row = altered['orders'][0]
            row[key] = ('0' * 64 if row[key] != '0' * 64 else '1' * 64) if isinstance(
                row[key], str) else row[key] + 1
            require(projected(config, altered) != baseline, 'counter or digest mutant survived')
            killed.append('order_' + key)
        for key in ('raw_candidates', 'certificate_digest', 'input_digest'):
            altered = copy.deepcopy(record)
            value = altered['terminal'][key]
            altered['terminal'][key] = ('0' * 64 if value != '0' * 64 else '1' * 64) if isinstance(
                value, str) else value + 1
            require(projected(config, altered) != baseline, 'terminal mutant survived')
            killed.append('terminal_' + key)
        altered_config = copy.deepcopy(config)
        altered_config['cache_entries'] += 1
        require(projected(altered_config, record) != baseline, 'configuration mutant survived')
        killed.append('configuration_cache_entries')
        for name in ('counter_float_type', 'scope_boolean_type'):
            altered = copy.deepcopy(record)
            if name == 'counter_float_type':
                altered['orders'][0]['input_records'] = float(altered['orders'][0]['input_records'])
            else:
                altered['orders'][0]['whole_tower_authority'] = 0
            require(projected(config, altered) != baseline, 'scalar type mutant survived')
            killed.append(name)
        source_binding(OLD_BINARY_SHA, OLD_PRODUCER_SHA, True, self.producer_sha)
        source_binding('1' * 64, self.producer_sha, False, self.producer_sha)
        fingerprint_killed = 0
        for binary_sha, header_sha, old_arm in (
            (OLD_BINARY_SHA, self.producer_sha, True),
            ('1' * 64, OLD_PRODUCER_SHA, True),
            ('1' * 64, OLD_PRODUCER_SHA, False),
        ):
            try:
                source_binding(binary_sha, header_sha, old_arm, self.producer_sha)
            except ValueError:
                fingerprint_killed += 1
        require(len(killed) == 14 and fingerprint_killed == 3, 'comparator selftest nonvacuum')
        return {'status': 'selftests_passed', 'measurement_only_positive': 1,
                'field_mutants': killed, 'fingerprint_positives': 2,
                'fingerprint_mutants': fingerprint_killed,
                'models_are_engine_receipts': False, 'raw_sha256': digest(raw),
                'receipt_sha256': c.sha(fixture)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    parser.add_argument('--expected-adapter-sha256')
    parser.add_argument('--producer-sha256')
    sub = parser.add_subparsers(dest='action', required=True)
    for action in ('build', 'micro'):
        cli = sub.add_parser(action)
        cli.add_argument('--id', required=True)
        if action == 'micro':
            cli.add_argument('--build', required=True, metavar='PATH=SHA256')
    heavy = sub.add_parser('prepare-heavy')
    heavy.add_argument('--id', required=True)
    heavy.add_argument('--micro', required=True, metavar='PATH=SHA256')
    heavy.add_argument('--qualification', required=True, metavar='PATH=SHA256')
    attempt = sub.add_parser('attempt')
    attempt.add_argument('--directory', type=Path, required=True)
    attempt.add_argument('--index', type=int, required=True)
    attempt.add_argument('--go-reviewed-heavy', action='store_true')
    close = sub.add_parser('close-heavy')
    close.add_argument('--directory', type=Path, required=True)
    selftest = sub.add_parser('comparator-selftest')
    selftest.add_argument('--fixture', required=True, type=Path)
    args = parser.parse_args()
    if not args.execute:
        print('prepared_not_executed; no source read, artifact, build or engine')
        return 0
    adapter = Adapter(args.expected_adapter_sha256, args.producer_sha256,
                      validate_live=args.action != 'close-heavy')
    for sig in adapter.c.SIGNALS:
        signal.signal(sig, adapter.c.interrupted)
    if args.action == 'build':
        adapter.c.build(args.id)
    elif args.action == 'micro':
        adapter.micro(args.id, args.build)
    elif args.action == 'comparator-selftest':
        print(json.dumps(adapter.comparator_selftest(args.fixture), sort_keys=True))
    elif args.action == 'attempt':
        adapter.heavy_attempt(args.directory, args.index, args.go_reviewed_heavy)
    elif args.action == 'close-heavy':
        adapter.close_heavy(args.directory)
    else:
        adapter.prepare_heavy(args.id, args.micro, args.qualification)
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except BaseException as error:
        if isinstance(error, SystemExit):
            raise
        print(json.dumps({'status': 'failed', 'reason': type(error).__name__ + ': ' + str(error)}))
        raise SystemExit(1)
