#!/usr/bin/env python3
"""Build an isolated direct-H judge, UBSan run and real C++ census mutants."""
from __future__ import annotations

from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PROBE = 'audits/morsehgp3D_v8_complementaire/q2_census_independent_probe.cpp'
CENSUS = 'morsehgp3D_v8/src/pipeline/q2_census.cpp'
SOURCES = ['core/types.hpp', 'pipeline/local_credits.cpp', 'pipeline/local_credits.hpp',
           'pipeline/tube_credits.hpp', 'pipeline/axis_q2.cpp', 'pipeline/axis_q2.hpp',
           'pipeline/q2_census.cpp', 'pipeline/q2_census.hpp', 'spindle/predicates.hpp']
MUTANTS = [
    ('closed_strict_point',
     'if (point_power4(key, points[index.order_[z.range.first]]) > 0)',
     'if (point_power4(key, points[index.order_[z.range.first]]) >= 0)'),
    ('drop_tangent_shell', 'if (bounds.maximum4 < 0) {', 'if (bounds.maximum4 <= 0) {'),
    ('restart_right_query_after_prefix', 'shared_task(a_id, b.right, count, cursor);',
     'shared_task(a_id, b.right, count, 0);'),
    ('drop_right_query_prefix_count', 'shared_task(a_id, b.right, count, cursor);',
     'shared_task(a_id, b.right, 0, cursor);'),
    ('diameter_accumulator_u32',
     'key.diameter_squared += static_cast<u64>(difference * difference);',
     'key.diameter_squared = static_cast<std::uint32_t>(key.diameter_squared + static_cast<u64>(difference * difference));'),
    ('payload_position_as_original_id', 'const auto id = index.order_[z.range.first];',
     'const auto id = z.range.first;'),
    ('maximum_at_lower_endpoint_only',
     'const i64 summit2 = std::clamp(av + value, 2 * zl, 2 * zh);',
     'const i64 summit2 = 2 * zl;'),
]


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise RuntimeError(reason)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def command(args: list[str], timeout: int = 60) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, capture_output=True, text=True, timeout=timeout)


def evaluate(contents: dict[str, str], expected: dict[str, object] | None) -> dict[str, object]:
    require(set(contents) == {PROBE} | {'morsehgp3D_v8/src/' + p for p in SOURCES},
            'snapshot source coverage differs')
    with tempfile.TemporaryDirectory(prefix='mhgp8_census_checked_') as temporary:
        snapshot = Path(temporary)
        for name, text in contents.items():
            target = snapshot / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(text.encode('utf-8'))
        result: dict[str, object] = {}
        for mode in ('normal', 'ubsan'):
            flags = ['-std=c++20', '-O2' if mode == 'normal' else '-O1',
                     '-Wall', '-Wextra', '-Wpedantic', '-Werror']
            if mode == 'ubsan':
                flags += ['-fsanitize=undefined', '-fno-sanitize-recover=all']
            binary = snapshot / mode
            argv = ['g++', *flags, '-I', str(snapshot / 'morsehgp3D_v8/src'),
                    str(snapshot / PROBE), str(snapshot / 'morsehgp3D_v8/src/pipeline/local_credits.cpp'),
                    str(snapshot / 'morsehgp3D_v8/src/pipeline/axis_q2.cpp'), '-o', str(binary)]
            build = command(argv)
            require(build.returncode == 0, mode + ' build failed: ' + build.stderr)
            run = command([str(binary), '--selftest'], 20)
            require(run.returncode == 0, mode + ' judge failed: ' + run.stdout + run.stderr)
            decoded = json.loads(run.stdout)
            if mode == 'normal':
                result['results'] = decoded
                if expected is not None:
                    require(decoded == expected, 'replayed census results differ')
            else:
                require(decoded == result['results'], 'UBSan and normal results differ')
            invalid = command([str(binary), '--invalid'], 10)
            require(invalid.returncode == 2, 'invalid CLI must exit 2')
            result[mode] = {'compile_flags': flags, 'build_exit': build.returncode,
                            'run_exit': run.returncode, 'stdout': run.stdout, 'stderr': run.stderr,
                            'invalid_cli_exit': invalid.returncode}
            print('completed ' + mode, file=sys.stderr)
        mutations = []
        original = contents[CENSUS]
        for name, before, after in MUTANTS:
            require(original.count(before) == 1, 'mutant source occurrence count: ' + name)
            altered = original.replace(before, after)
            (snapshot / CENSUS).write_bytes(altered.encode('utf-8'))
            binary = snapshot / name
            flags = ['-std=c++20', '-O1', '-Wall', '-Wextra', '-Wpedantic', '-Werror']
            argv = ['g++', *flags, '-I', str(snapshot / 'morsehgp3D_v8/src'),
                    str(snapshot / PROBE), str(snapshot / 'morsehgp3D_v8/src/pipeline/local_credits.cpp'),
                    str(snapshot / 'morsehgp3D_v8/src/pipeline/axis_q2.cpp'), '-o', str(binary)]
            build = command(argv)
            require(build.returncode == 0, 'mutant build failed: ' + name + ': ' + build.stderr)
            run = command([str(binary), '--selftest'], 20)
            require(run.returncode == 1, 'mutant was not rejected with exit 1: ' + name + ': ' + run.stdout)
            mutations.append({'name': name, 'source_sha256': sha(altered.encode('utf-8')),
                              'before': before, 'after': after, 'compile_flags': flags,
                              'build_exit': build.returncode, 'run_exit': run.returncode,
                              'stdout': run.stdout, 'stderr': run.stderr})
            print('rejected mutant ' + name, file=sys.stderr)
        result['executed_cpp_mutants'] = mutations
        return result


def main() -> int:
    args = sys.argv[1:]
    expected = None
    if args == ['--selftest'] or (len(args) == 3 and args[:2] == ['--selftest', '--snapshot']):
        origin = ROOT if len(args) == 1 else Path(args[2])
        contents = {'morsehgp3D_v8/src/' + p: (origin / 'morsehgp3D_v8/src' / p).read_bytes().decode('utf-8') for p in SOURCES}
        contents[PROBE] = (ROOT / PROBE).read_bytes().decode('utf-8')
        state = 'working_tree_snapshot'
    elif len(args) == 2 and args[0] == '--replay':
        prior = json.loads(Path(args[1]).read_text())
        contents = prior['snapshot_utf8']
        expected = prior['results']
        for path, digest in prior['source_sha256'].items():
            require(sha(contents[path].encode('utf-8')) == digest, 'replay source hash differs: ' + path)
        require(sha(contents[PROBE].encode('utf-8')) == prior['auditor_source_sha256'], 'replay judge hash differs')
        state = 'receipt_snapshot_replay'
    else:
        print(json.dumps({'status': 'invalid_cli'}))
        return 2
    hashes = {path: sha(text.encode('utf-8')) for path, text in contents.items() if path != PROBE}
    receipt = evaluate(contents, expected)
    receipt.update({'schema': 'mhgp8_cpp_q2_census_independent_checks_v1',
                    'status': 'passed', 'public_status': 'not_claimed',
                    'created_utc': datetime.now(timezone.utc).isoformat(),
                    'source_state': state, 'source_sha256': hashes,
                    'auditor_source': PROBE, 'auditor_source_sha256': sha(contents[PROBE].encode('utf-8')),
                    'runner_source_sha256': sha(Path(__file__).read_bytes()),
                    'snapshot_utf8': contents,
                    'source_hashes_still_match_worktree': all((ROOT / p).is_file() and sha((ROOT / p).read_bytes()) == digest for p, digest in hashes.items()),
                    'gcp_used': False,
                    'scope': 'bounded_direct_H_census_and_rational_box_extrema_no_performance_or_full'})
    print(json.dumps(receipt, ensure_ascii=False, sort_keys=True, indent=2))
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (RuntimeError, OSError, ValueError, subprocess.TimeoutExpired) as error:
        print(json.dumps({'status': 'failed', 'error': str(error)}, sort_keys=True))
        raise SystemExit(1) from error
