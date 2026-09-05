#!/usr/bin/env python3
"""Create-only read-only-judge captures. No engine, Git or GCP invocation.

First run qualify/attempt into a fresh build directory. ROOT reviews its closed
receipt before invoking publish, which copies the exact sealed packet unchanged.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys
import time

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_first_c_20260905_controller'
MICRO = ROOT / 'build/v7_full_lazy_20260905_probe_controller/micro_admission'
MICRO_SHA = '9ce369e2d6085e1e7ac0b95c03a84f1793f42d0107a2b2a15474644d880ce1b2'
SUPPLEMENT = ROOT / 'morsehgp3D_v7/bench/full_gabriel_cache_policy_audit.py'
SUPPLEMENT_SHA = '8f8aed03755d9c92775566b21d4fdd9dcba31f171adf4b83e9802a988a450370'
JUDGE = ROOT / 'morsehgp3D_v7/bench/full_gabriel_lazy_probe_audit.py'
JUDGE_SHA = '8d8a612aa973cb79e60e97a6675f63684ddd8892cfc550716c20620c4d6930ef'
SCHEMA = 'mhgp7-first-c-supplement-capture-v1'
ENV = {'PATH': '/usr/bin:/bin', 'LC_ALL': 'C', 'LANG': 'C', 'TZ': 'UTC',
       'PYTHONHASHSEED': '0', 'OMP_NUM_THREADS': '1', 'OPENBLAS_NUM_THREADS': '1'}


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def encoded(value: object) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def now() -> str:
    return datetime.now(timezone.utc).isoformat()


def write(path: Path, raw: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('xb') as stream:
        stream.write(raw)
    require(path.read_bytes() == raw, 'created bytes differ: ' + str(path))


def names(directory: Path) -> set[str]:
    result = set()
    for path in directory.rglob('*'):
        require(not path.is_symlink(), 'symlink artifact')
        if path.is_file():
            result.add(str(path.relative_to(directory)))
        else:
            require(path.is_dir(), 'nonregular artifact')
    return result


class Capture:
    def __init__(self, directory: Path, own_sha: str):
        require(not directory.exists(), 'create-only destination exists')
        directory.mkdir()
        self.directory = directory
        self.watches: dict[Path, bytes] = {}
        self.commands: list[dict] = []
        self.started = now()
        for label, path, pin in (
            ('check.py', Path(__file__).resolve(), own_sha),
            ('first_c_audit.py', SUPPLEMENT, SUPPLEMENT_SHA),
            ('v2_audit.py', JUDGE, JUDGE_SHA),
        ):
            write(directory / 'protocol' / label, self.read(path, pin))
        write(directory / 'intent.json', encoded({
            'schema': SCHEMA, 'started': self.started, 'argv': sys.argv,
            'cwd': str(ROOT), 'child_environment': ENV,
            'public_status': 'not_claimed', 'engine_executed': False,
            'gcp_used': False, 'controller_sha256': own_sha,
        }))

    def read(self, path: Path, pin: str | None = None) -> bytes:
        require(path.is_file() and not path.is_symlink(), 'nonregular input')
        raw = path.read_bytes()
        require(len(raw) <= 8 * 1024 * 1024, 'input size cap')
        require(pin is None or sha(raw) == pin, 'input pin differs: ' + str(path))
        require(path not in self.watches or self.watches[path] == raw, 'input drift')
        self.watches[path] = raw
        return raw

    def command(self, label: str, script: Path, args: list[str], mode: str,
                expected: int) -> dict:
        argv = ['/usr/bin/taskset', '-c', '0', '/usr/bin/python3', '-B']
        argv += ['-O'] if mode == 'optimized' else []
        argv += [str(script), *args]
        start = now()
        clock = time.monotonic()
        intent = {'argv': argv, 'command': shlex.join(argv), 'cwd': str(ROOT),
                  'environment': ENV, 'expected_exit_code': expected,
                  'started': start, 'timeout_seconds': 20}
        write(self.directory / 'commands' / (label + '.intent.json'), encoded(intent))
        error = None
        try:
            run = subprocess.run(argv, cwd=ROOT, env=ENV, capture_output=True,
                                 timeout=20, check=False)
            stdout, stderr, code = run.stdout, run.stderr, run.returncode
        except subprocess.TimeoutExpired as failure:
            stdout, stderr, code = failure.stdout or b'', failure.stderr or b'', None
            error = 'timeout_not_a_completed_exit'
        write(self.directory / 'commands' / (label + '.stdout'), stdout)
        write(self.directory / 'commands' / (label + '.stderr'), stderr)
        row = dict(intent, ended=now(), elapsed_seconds=time.monotonic() - clock,
                   exit_code=code, error=error, label=label,
                   stdout_sha256=sha(stdout), stderr_sha256=sha(stderr),
                   passed=code == expected and not stderr and error is None)
        write(self.directory / 'commands' / (label + '.command.json'), encoded(row))
        self.commands.append(row)
        require(len(stdout) <= 65536 and len(stderr) <= 65536, 'output size cap')
        require(row['passed'], 'command failed: ' + label)
        result = json.loads(stdout)
        return result

    def input(self, path: Path, pin: str, key: str) -> dict:
        record = self.read(path, pin)
        raw_path = path.with_name(path.name.removesuffix('.receipt.json') + '.raw.txt')
        receipt = json.loads(record)
        raw = self.read(raw_path, receipt['streams'][raw_path.name]['sha256'])
        require(len(raw) == receipt['streams'][raw_path.name]['bytes'], 'raw bytes binding')
        write(self.directory / 'inputs' / key / path.name, record)
        write(self.directory / 'inputs' / key / raw_path.name, raw)
        return receipt

    def seal(self, status: str, kind: str, summary: dict, error: str | None) -> dict:
        for path, raw in self.watches.items():
            require(path.read_bytes() == raw and not path.is_symlink(), 'terminal input drift')
        write(self.directory / 'inputs.json', encoded({str(path): {
            'bytes': len(raw), 'sha256': sha(raw)}
            for path, raw in sorted(self.watches.items())}))
        receipt = {'schema': SCHEMA, 'kind': kind, 'status': status,
                   'started': self.started, 'ended': now(), 'error': error,
                   'sources_stable': True, 'summary': summary,
                   'commands': self.commands, 'public_status': 'not_claimed',
                   'engine_executed': False, 'gcp_used': False,
                   'scope': 'first_C_counters_not_geometry_or_catalogue_completeness',
                   'requires_frozen_v2_judge_sha256': JUDGE_SHA}
        write(self.directory / 'receipt.json', encoded(receipt))
        lines = [
            '# Supplément first-C — contrôles en lecture seule', '',
            '5 septembre 2026. CPU de référence, entrée u16 ; public_status=not_claimed.', '',
            'Ce paquet ne lance aucun moteur. Il contrôle des captures existantes avec le',
            'supplément first-C épinglé et conserve leurs octets, les commandes et sorties.',
            'Le juge FULLv2 reste obligatoire : ce supplément ne le remplace pas.', '',
            'Pour chaque ordre lazy réussi : inserts=min(C,portals),',
            'skips=max(0,portals-C). Les refus ne reçoivent que des bornes de préfixe.',
            'Les modèles de selftest ne sont pas des captures du moteur.', '',
            '[Statut et comptages](receipt.json), [sources et entrées](inputs.json),',
            '[sommes des octets](SHA256SUMS). Aucun résultat 50k/1 seconde/100 ms,',
            'aucune complétude géométrique ni qualification de tour inter-K intégrée.',
            'GCP non utilisé.', '',
        ]
        write(self.directory / 'README.md', '\n'.join(lines).encode())
        inventory = {name: {'bytes': (self.directory / name).stat().st_size,
                           'sha256': sha((self.directory / name).read_bytes())}
                     for name in sorted(names(self.directory))}
        write(self.directory / 'manifest.json', encoded(inventory))
        sums = ''.join(sha((self.directory / name).read_bytes()) + '  ' + name + '\n'
                       for name in sorted(names(self.directory)))
        write(self.directory / 'SHA256SUMS', sums.encode())
        return receipt


def qualify(capture: Capture) -> dict:
    micro_bytes = capture.read(MICRO / 'receipt.json', MICRO_SHA)
    micro = json.loads(micro_bytes)
    require(micro['status'] == 'completed' and micro['sources_stable'] is True,
            'micro not admitted')
    write(capture.directory / 'micro_admission_receipt.json', micro_bytes)
    choices = [('eager', 0), ('lazy', 0), ('lazy', 1), ('lazy', 1000000)]
    outcomes = []
    order_count = lazy_count = 0
    for kmax in (5, 10):
        for sep in (8, 10, 12):
            for policy, cap in choices:
                stem = f'n8_s{sep}_k{kmax}_{policy}_c{cap}'
                key = f'k{kmax}/{stem}'
                relative = f'k{kmax}/{stem}.receipt.json'
                path = MICRO / relative
                receipt = capture.input(path, micro['artifacts'][relative], key)
                require(receipt['exit_code'] == 0, 'micro engine not successful')
                order_count += len(receipt['orders'])
                lazy_count += len(receipt['orders']) if policy == 'lazy' else 0
                for mode in ('normal', 'optimized'):
                    for extension in ('stdout', 'stderr', 'command.json'):
                        saved = f'k{kmax}/{stem}.judge_{mode}.{extension}'
                        content = capture.read(MICRO / saved, micro['artifacts'][saved])
                        write(capture.directory / 'inherited_v2' / saved, content)
                    saved_result = json.loads(capture.read(
                        MICRO / f'k{kmax}/{stem}.judge_{mode}.stdout'))
                    require(saved_result['audit_status'] == 'valid', 'inherited v2 failed')
                    saved_command = json.loads(capture.read(
                        MICRO / f'k{kmax}/{stem}.judge_{mode}.command.json'))
                    require(saved_command['exit_code'] == 0, 'inherited v2 exit')
                    result = capture.command(f'{stem}_{mode}', SUPPLEMENT,
                                             [str(path)], mode, 0)
                    require(result['supplement_status'] == 'valid'
                            and result['attempt_success'] is True, 'micro supplement failed')
                    require(result['successful_lazy_orders_checked'] == (
                        len(receipt['orders']) if policy == 'lazy' else 0), 'micro checked count')
                    outcomes.append(result)
    path = MICRO / 'k10/n8_s8_k10_lazy_c1.receipt.json'
    selftests = []
    for mode in ('normal', 'optimized'):
        result = capture.command('selftest_' + mode, SUPPLEMENT,
                                 ['--selftest', str(path)], mode, 0)
        require(result['supplement_status'] == 'selftests_passed'
                and len(set(result['mutants_killed'])) == 12
                and result['real_positive'] == 1
                and result['scalar_success_models'] == 9
                and result['scalar_refusal_prefix_models'] == 3
                and result['models_are_engine_receipts'] is False, 'selftest counts')
        selftests.append(result)
        for label, args in (
            ('empty', []), ('selftest_without_path', ['--selftest']),
            ('unknown', ['--unknown']), ('unknown_with_path', ['--unknown', str(path)]),
        ):
            result = capture.command('argv_' + label + '_' + mode,
                                     SUPPLEMENT, args, mode, 2)
            require(result == {'supplement_status': 'invalid_arguments'}, 'argv rejection reason')
    require(len(outcomes) == 48 and order_count == 156 and lazy_count == 117
            and len(capture.commands) == 58, 'qualification nonvacuum')
    return {'real_micro_receipts': 24, 'real_horizontal_orders': order_count,
            'real_lazy_orders_per_mode': lazy_count, 'supplement_commands': 48,
            'inherited_v2_successful_judgments': 48,
            'selftest_commands': 2, 'mutants_per_mode': 12,
            'invalid_argv_commands': 8, 'total_commands': 58,
            'normal_and_optimized_results_equal': outcomes[::2] == outcomes[1::2],
            'selftests_equal': selftests[0] == selftests[1],
            'micro_receipt_sha256': MICRO_SHA}


def attempt(capture: Capture, path: Path, pin: str) -> dict:
    require(path.is_absolute() and path.name.endswith('.receipt.json')
            and path.resolve().is_relative_to(ROOT / 'build'), 'attempt scope')
    receipt = capture.input(path, pin, 'attempt')
    results = []
    for mode in ('normal', 'optimized'):
        base = capture.command('v2_' + mode, JUDGE, [str(path)], mode, 0)
        require(base['audit_status'] == 'valid', 'v2 judgment failed')
        result = capture.command('first_c_' + mode, SUPPLEMENT, [str(path)], mode, 0)
        require(result['supplement_status'] == 'valid', 'supplement failed')
        results.append(result)
    require(results[0] == results[1], 'normal/-O supplement differs')
    return {'real_attempt_receipts': 1, 'source_receipt_sha256': pin,
            'engine_exit_code': receipt['exit_code'], 'total_commands': 4,
            'supplement': results[0], 'normal_and_optimized_results_equal': True}


def publish(source: Path, expected: str, label: str) -> None:
    require(source.resolve().is_relative_to(BASE), 'private packet scope')
    require(re.fullmatch(r'full_gabriel_first_c_[a-z0-9_]+', label) is not None,
            'publication name scope')
    original = {name: (source / name).read_bytes() for name in names(source)}
    require(sha(original['SHA256SUMS']) == expected, 'closed packet SHA differs')
    indexed = set()
    for line in original['SHA256SUMS'].decode().splitlines():
        match = re.fullmatch(r'([0-9a-f]{64})  ([a-zA-Z0-9_./-]+)', line)
        require(match is not None, 'invalid checksum line')
        pin, name = match.groups()
        require(name in original and '..' not in Path(name).parts
                and name not in indexed and name != 'SHA256SUMS'
                and sha(original[name]) == pin, 'checksum inventory differs')
        indexed.add(name)
    require(indexed | {'SHA256SUMS'} == set(original), 'unsealed artifact')
    receipt = json.loads(original['receipt.json'])
    require(receipt['schema'] == SCHEMA and receipt['status'] in ('completed', 'failed')
            and receipt['ended'], 'unclosed receipt')
    destination = ROOT / 'morsehgp3D_v7/receipts' / label
    require(not destination.exists(), 'create-only public destination exists')
    destination.mkdir()
    for name, raw in sorted(original.items()):
        write(destination / name, raw)
    require(names(destination) == set(original), 'publication inventory differs')
    require({name: (source / name).read_bytes() for name in names(source)} == original,
            'private packet changed during publication')
    print(json.dumps({'published': str(destination), 'files': len(original),
                      'SHA256SUMS': expected, 'git_index_check_pending_ROOT': True}))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    parser.add_argument('--expected-controller-sha256')
    sub = parser.add_subparsers(dest='action', required=True)
    for kind in ('qualify', 'attempt'):
        command = sub.add_parser(kind)
        command.add_argument('--id', required=True)
        if kind == 'attempt':
            command.add_argument('--receipt', type=Path, required=True)
            command.add_argument('--receipt-sha256', required=True)
    public = sub.add_parser('publish')
    public.add_argument('--source', type=Path, required=True)
    public.add_argument('--sha256sums', required=True)
    public.add_argument('--name', required=True)
    args = parser.parse_args()
    if not args.execute:
        print('prepared_not_executed; no input read or artifact created')
        return 0
    require(sha(Path(__file__).read_bytes()) == args.expected_controller_sha256,
            'controller pin required or differs')
    if args.action == 'publish':
        publish(args.source, args.sha256sums, args.name)
        return 0
    require(re.fullmatch(r'[a-z0-9_]+', args.id) is not None, 'private id scope')
    capture = Capture(BASE / args.id, args.expected_controller_sha256)
    try:
        summary = qualify(capture) if args.action == 'qualify' else attempt(
            capture, args.receipt, args.receipt_sha256)
        require(summary.get('normal_and_optimized_results_equal') is True,
                'normal/-O result divergence')
        if args.action == 'qualify':
            require(summary['selftests_equal'] is True, 'normal/-O selftests differ')
        receipt = capture.seal('completed', args.action, summary, None)
    except (ValueError, OSError, KeyError, TypeError) as failure:
        capture.seal('failed', args.action, {}, str(failure))
        raise
    print(json.dumps({'status': receipt['status'], 'directory': str(capture.directory),
                      'summary': summary,
                      'SHA256SUMS': sha((capture.directory / 'SHA256SUMS').read_bytes())},
                     sort_keys=True))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, OSError, KeyError, TypeError) as error:
        print(json.dumps({'status': 'failed', 'reason': str(error)}, sort_keys=True))
        sys.exit(1)
