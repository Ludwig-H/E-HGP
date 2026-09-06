#!/usr/bin/env python3
"""Portable, bounded replay of three pinned Python-only mathematical gates."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import subprocess
import sys

BASE = Path(__file__).resolve().parent
GATES = {
    'quotient': ('full_gabriel_minima_quotient_gate.py', 'bee615b5f8b937e11104597fd674d868828d6b850616582f5163b44454ab9434',
                 '24210dfeb475dfa75ecef65dc13aadc2a6d8336cd76e663b3cec79c352a98c2a'),
    'descent': ('full_gabriel_descent_comparison_gate.py', '5e357ead0d626121cf66e15d17f0817475520663db7fe5237d9cbd7f25448a16',
                '7e2c6c99f5c7bc31d9baef7244ad7ba6e3a46e0f64605bb68fa002838f241318'),
    'lower_bound': ('full_output_lower_bound_gate.py', '01fd40103f89d878e1d89bb08fcc7592f4c684f3f5db9ef70443a2660f7dcb04',
                    '3efe537e423e4f14c59ae5d8d8a2190a49c5c381658f69f33bd266b3d0d88a43'),
}


def require(ok, why):
    if not ok:
        raise ValueError(why)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def json_read(path):
    def pairs(items):
        result = {}
        for key, value in items:
            require(key not in result, 'duplicate_json_key')
            result[key] = value
        return result
    return json.loads(path.read_bytes(), object_pairs_hook=pairs,
                      parse_constant=lambda value: (_ for _ in ()).throw(ValueError('nonfinite_json')))


def path_under(base, name):
    value = PurePosixPath(name)
    require(not value.is_absolute() and '..' not in value.parts and str(value) == name, 'relative_path')
    result = base / name
    require(not any(p.is_symlink() for p in (result, *result.parents)) and result.is_file(), 'regular_file')
    return result


def verify():
    expected = {}
    for line in (BASE / 'PAYLOAD_SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        require(len(digest) == 64 and all(c in '0123456789abcdef' for c in digest)
                and name not in expected, 'manifest_line')
        path = path_under(BASE, name)
        require(sha(path) == digest and not path.read_bytes().startswith(b'\x7fELF'), 'payload_hash_or_ELF')
        expected[name] = digest
    observed = {str(p.relative_to(BASE)) for p in BASE.rglob('*') if p.is_file()
                and p != BASE / 'PAYLOAD_SHA256SUMS'}
    require(observed == set(expected), 'closed_payload_inventory')
    for kind, (filename, pin, receipt_pin) in GATES.items():
        require(sha(BASE / 'sources/morsehgp3D_v7/tests' / filename) == pin, 'gate_pin:' + kind)
        history = BASE / 'history' / kind
        require(sha(history / 'receipt.json') == receipt_pin, 'historical_receipt_pin:' + kind)
        receipt = json_read(history / 'receipt.json')
        require(receipt['status'] == 'completed' and receipt['engine_invoked'] is False, 'history_scope')
        if kind == 'lower_bound':
            require(receipt['gate_sha256'] == pin and receipt['gate_before_after_stable'] is True, 'lower_source')
            for name, digest in receipt['artifacts'].items():
                require(sha(path_under(history, name)) == digest, 'historical_artifact')
            commands = list(receipt['commands'].values())
        else:
            commands = receipt['commands']
        require(len(commands) == 4, 'historical_four_commands')
        seen = set()
        for row in commands:
            name = row['id'] if kind == 'lower_bound' else row['name']
            require(name in ('normal_selftest', 'normal_unknown', 'optimized_selftest', 'optimized_unknown')
                    and name not in seen, 'historical_command_name')
            seen.add(name)
            require(type(row['exit_code']) is int and row['exit_code'] == (0 if name.endswith('selftest') else 2),
                    'historical_code')
            require((history / (name + '.stderr')).read_bytes() == b'', 'historical_stderr')
            for stream in ('stdout', 'stderr'):
                stream_pin = (row['streams'][name + '.' + stream]['sha256'] if kind == 'lower_bound'
                              else row[stream + '_sha256'])
                require(sha(history / (name + '.' + stream)) == stream_pin, 'historical_stream_hash')
            if kind == 'lower_bound':
                require(row['process_group_closed'] is True and row['closure_errors'] == []
                        and row['status'] == 'completed', 'historical_process_closure')
    return expected


def run(output):
    before = verify()
    require(not output.exists() and not output.is_relative_to(BASE), 'fresh_output_outside_payload')
    output.mkdir(parents=True)
    errors, commands = [], []
    for kind, (filename, _, _) in GATES.items():
        for optimized in (False, True):
            mode = 'optimized' if optimized else 'normal'
            for argument, expected in (('--selftest', 0), ('--unknown', 2)):
                name = kind + '_' + mode + '_' + argument[2:]
                argv = [sys.executable, '-I', '-B'] + (['-O'] if optimized else [])
                argv += ['sources/morsehgp3D_v7/tests/' + filename, argument]
                intent = dict(argv=argv, cwd=str(BASE), expected=expected, timeout_seconds=10,
                              engine_invoked=False, historical_mode=mode)
                (output / (name + '.intent.json')).write_text(json.dumps(intent, sort_keys=True, indent=2) + '\n')
                try:
                    result = subprocess.run(argv, cwd=BASE, capture_output=True, timeout=10, check=False)
                    stdout, stderr, code = result.stdout, result.stderr, result.returncode
                except subprocess.TimeoutExpired as error:
                    stdout, stderr, code = error.stdout or b'', error.stderr or b'', None
                    errors.append(name + ':timeout')
                (output / (name + '.stdout')).write_bytes(stdout)
                (output / (name + '.stderr')).write_bytes(stderr)
                historical = BASE / 'history' / kind / (mode + '_' + argument[2:] + '.stdout')
                same = stdout == historical.read_bytes()
                commands.append(dict(name=name, **intent, exit_code=code, historical_stdout_equal=same,
                                     stdout_sha256=sha(output / (name + '.stdout')),
                                     stderr_sha256=sha(output / (name + '.stderr'))))
                if code != expected or stderr or not same:
                    errors.append(name + ':code_stderr_or_historical_output')
    try:
        require(verify() == before, 'payload_drift')
    except (ValueError, OSError) as error:
        errors.append(str(error))
    receipt = dict(schema='mhgp7-portable-minima-math-replay-v1', status='completed' if not errors else 'failed',
                   public_status='not_claimed', engine_invoked=False, latency_qualified=False,
                   payload_manifest_sha256=sha(BASE / 'PAYLOAD_SHA256SUMS'),
                   files_before=before, commands=commands, errors=errors)
    (output / 'receipt.json').write_text(json.dumps(receipt, sort_keys=True, indent=2) + '\n')
    (output / 'SHA256SUMS').write_text(''.join(sha(p) + '  ' + p.name + '\n'
        for p in sorted(output.iterdir()) if p.is_file()))
    require(not errors, 'replay_failed')
    return dict(status='completed', commands=len(commands), engine_invoked=False,
                receipt_sha256=sha(output / 'receipt.json'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if not args.execute:
        require(args.output is None, 'output_requires_execute')
        return dict(status='verified', files=len(verify()), engine_invoked=False)
    require(args.output is not None, 'execute_requires_output')
    return run(args.output.resolve())


if __name__ == '__main__':
    try:
        print(json.dumps(main(), sort_keys=True))
    except (ValueError, KeyError, OSError) as error:
        print(json.dumps(dict(status='failed', reason=str(error)), sort_keys=True))
        sys.exit(1)
