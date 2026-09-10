"""Fresh frozen O2/SAN qualification of the parent-arena regression gate."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import time

TEST = Path('morsehgp3D_v7/tests/full_coverage_certificate_gate.cpp')
HEADER = Path('morsehgp3D_v7/src/forest/full_coverage_certificate.hpp')


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root, output = args.root.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    boost = root / 'build/v7_boost_gate/extracted/usr/include'
    flags = ['-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-isystem', str(boost)]
    rows = []

    def command(name, argv, cwd=root, expected=0, env=None):
        print(name, flush=True)
        start = time.time_ns()
        result = subprocess.run(argv, cwd=cwd, env=env, capture_output=True, check=False)
        stdout, stderr = output / (name + '.stdout'), output / (name + '.stderr')
        stdout.write_bytes(result.stdout)
        stderr.write_bytes(result.stderr)
        row = dict(name=name, argv=list(map(str, argv)), cwd=str(cwd),
                   started_ns=start, finished_ns=time.time_ns(), exit_code=result.returncode,
                   expected_exit_code=expected, stdout_sha256=sha(stdout), stderr_sha256=sha(stderr),
                   environment_overrides={key: env[key] for key in ('ASAN_OPTIONS', 'UBSAN_OPTIONS')} if env else {})
        rows.append(row)
        (output / 'commands.json').write_text(json.dumps(rows, indent=2, sort_keys=True) + '\n')
        if result.returncode != expected:
            raise RuntimeError(name + ': ' + result.stderr.decode(errors='replace'))

    command('compiler', ['g++', '--version'])
    command('head', ['git', 'rev-parse', 'HEAD'])
    command('dependencies', ['g++', *flags, '-MM', str(TEST)])
    names = shlex.split((output / 'dependencies.stdout').read_text().replace('\\\n', ' ').split(':', 1)[1])
    before = {}
    snapshot = output / 'snapshot'
    for name in names:
        source = (root / name).resolve()
        relative = source.relative_to(root)
        destination = snapshot / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, destination)
        before[str(relative)] = sha(source)
    (output / 'sources_before.json').write_text(json.dumps(before, indent=2, sort_keys=True) + '\n')
    bins = {}
    for kind, options in (('o2', ['-O2']), ('san', ['-O1', '-g', '-fno-omit-frame-pointer',
                                               '-fsanitize=address,undefined', '-fno-pie', '-no-pie'])):
        binary = output / ('gate_' + kind)
        command('compile_' + kind, ['g++', *flags, *options, '-MMD', '-MF', str(output / (kind + '.d')),
                                    str(TEST), '-o', str(binary)], snapshot)
        env = None
        if kind == 'san':
            env = os.environ.copy()
            env.update(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                       UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
        command('selftest_' + kind, [str(binary), '--selftest'], expected=0, env=env)
        command('argument_' + kind, [str(binary), '--unknown'], expected=2, env=env)
        bins[kind] = sha(binary)
        consumed = {}
        for spelling in shlex.split((output / (kind + '.d')).read_text().replace('\\\n', ' ').split(':', 1)[1]):
            path = (snapshot / spelling).resolve()
            relative = path.relative_to(snapshot)
            consumed[str(relative)] = sha(path)
        if consumed != before:
            raise RuntimeError('unexpected compiled dependency closure: ' + kind)
    if (output / 'selftest_o2.stdout').read_bytes() != (output / 'selftest_san.stdout').read_bytes():
        raise RuntimeError('O2/SAN stdout differs')
    if (output / 'selftest_o2.stderr').read_bytes() or (output / 'selftest_san.stderr').read_bytes():
        raise RuntimeError('successful run emitted diagnostics')
    private = output / 'parent_zero_mutant'
    shutil.copytree(snapshot, private)
    path = private / HEADER
    source = path.read_text()
    old, new = 'out.parents_.push_back(parent);', 'out.parents_.push_back(0);'
    if source.count(old) != 1:
        raise RuntimeError('parent mutant substitution must be unique')
    path.write_text(source.replace(old, new))
    binary = output / 'gate_parent_zero_mutant'
    command('compile_parent_zero_mutant', ['g++', *flags, '-O2', str(TEST), '-o', str(binary)], private)
    command('selftest_parent_zero_mutant', [str(binary), '--selftest'], expected=1)
    if (output / 'selftest_parent_zero_mutant.stderr').read_text() != 'FAIL arena.parent_value\n':
        raise RuntimeError('unexpected mutant failure cause')
    bins['parent_zero_mutant'] = sha(binary)
    after = {name: sha(root / name) for name in before}
    (output / 'sources_after.json').write_text(json.dumps(after, indent=2, sort_keys=True) + '\n')
    if before != after:
        raise RuntimeError('active source changed during capture')
    shutil.copyfile(__file__, output / 'record.py')
    receipt = dict(status='passed', scope='structural_parent_arena_regression_not_geometry_or_FULL_product',
                   commands=len(rows), sources_stable=True, sources=before, binaries_sha256=bins,
                   recorder_sha256=sha(__file__), mutant_header_sha256=sha(path),
                   expected_mutant_diagnostic='FAIL arena.parent_value\n',
                   GCP_used=False, CMake_used=False, ELF_in_portable_packet=False)
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2, sort_keys=True) + '\n')
    print(json.dumps(receipt, sort_keys=True), flush=True)


if __name__ == '__main__':
    main()
