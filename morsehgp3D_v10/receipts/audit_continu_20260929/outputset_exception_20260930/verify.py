"""Portable archive rejudge: hashes before judge import; no binaries/builds/origin required."""
import datetime
import hashlib
import importlib.util
import json
from pathlib import Path, PurePosixPath
import sys

ROOT = Path(__file__).resolve().parent

def require(condition, message):
    if not condition:
        raise ValueError(message)

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    entries = {}
    for line in (ROOT / 'SHA256SUMS').read_text().splitlines():
        digest, path = line.split('  ', 1)
        p = PurePosixPath(path)
        require(len(digest) == 64 and all(c in '0123456789abcdef' for c in digest), 'hash syntax')
        require(not p.is_absolute() and '..' not in p.parts and str(p) == path and path not in entries, 'manifest path')
        require(sha(ROOT / path) == digest, 'hash mismatch: ' + path)
        entries[path] = digest
    expected = {'README.md', 'probe.cpp', 'record.py', 'judge.py', 'verify.py', 'receipt.json'}
    expected |= {'source/core/' + p for p in ['cli_output.hpp', 'status.hpp', 'types.hpp', 'reasons.def']}
    expected |= {p + '.' + s for p in ['compiler', 'compile_normal', 'run_normal', 'compile_ubsan', 'run_ubsan']
                 for s in ['stdout', 'stderr']}
    require(set(entries) == expected, 'exact manifest inventory')
    receipt = json.loads((ROOT / 'receipt.json').read_text())
    require(receipt['kind'] == 'isolated_outputset_exception_audit', 'receipt identity')
    require(receipt['origin_head'] == 'bf704f9eb30d97c61cbdc62cade6b8e22450adbe', 'origin revision')
    require(receipt['source_before'] == receipt['source_after'], 'source stability')
    require(receipt['origin_before'] == receipt['origin_after'], 'origin observation stability')
    for p, digest in receipt['source_before'].items():
        require(p in entries and entries[p] == digest, 'source pin: ' + p)
    require(set(receipt['source_before']) == {'probe.cpp', 'record.py'} | {p for p in entries if p.startswith('source/')}, 'source pin inventory')
    for p, digest in receipt['origin_before'].items():
        require(entries.get('source/' + p) == digest, 'copied origin identity: ' + p)
    require(set(receipt['origin_before']) == {'core/' + p for p in ['cli_output.hpp', 'status.hpp', 'types.hpp', 'reasons.def']}, 'origin inventory')
    commands = receipt['commands']
    require([c['name'] for c in commands] == ['compiler', 'compile_normal', 'run_normal', 'compile_ubsan', 'run_ubsan'], 'command inventory')
    captured_root = commands[0]['cwd']
    require(commands[0]['argv'] == ['g++', '--version'], 'compiler command')
    flags = {'normal': ['-O2'], 'ubsan': ['-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=undefined']}
    for mode, compile_record, run_record in [('normal', commands[1], commands[2]), ('ubsan', commands[3], commands[4])]:
        binary = captured_root + '/probe_' + mode
        require(compile_record['argv'] == ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror'] + flags[mode] +
                ['-Isource', 'probe.cpp', '-o', binary], 'compile argv: ' + mode)
        require(run_record['argv'] == [binary, captured_root + '/' + mode], 'run argv: ' + mode)
    for c in commands:
        require(type(c['rc']) is int and c['rc'] == 0 and c['cwd'] == captured_root, 'command status/cwd')
        require(datetime.datetime.fromisoformat(c['finished_utc']) >= datetime.datetime.fromisoformat(c['started_utc']), 'command timestamps')
        for stream in ['stdout', 'stderr']:
            require(c[stream + '_sha256'] == entries[c['name'] + '.' + stream], 'command stream linkage')
        require((ROOT / (c['name'] + '.stderr')).read_bytes() == b'', 'captured stderr nonempty')
    require(set(receipt['binary_sha256']) == {'probe_normal', 'probe_ubsan'} and
            all(len(v) == 64 and all(c in '0123456789abcdef' for c in v) for v in receipt['binary_sha256'].values()), 'binary hash records')
    spec = importlib.util.spec_from_file_location('closed_outputset_judge', ROOT / 'judge.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    observed = [module.parse(ROOT / ('run_' + m + '.stdout')) for m in ['normal', 'ubsan']]
    require(observed[0] == observed[1], 'normal/UBSan observations differ')
    count = module.adverse_controls(observed[0])
    print(json.dumps({'status': 'ARCHIVE_OBSERVED_FAULTS', 'manifest_files': len(entries), 'native_captures': 2,
                      'adverse_refusals': count, 'scope': 'helper_only_bf704f9_no_live_binary_no_engine'}, sort_keys=True))

if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, KeyError, TypeError) as e:
        print('REFUS: ' + str(e), file=sys.stderr)
        sys.exit(2)
