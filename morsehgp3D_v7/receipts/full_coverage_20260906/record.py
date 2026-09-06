#!/usr/bin/env python3
"""Create-only local qualification captures. No remote session or work ceilings."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path('/workspaces/E-HGP')
BASE = Path(__file__).resolve().parent
RUN = BASE / sys.argv[1]
RUN.mkdir(exist_ok=False)
commands = []


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(name, value):
    (RUN / name).write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def command(name, argv, expected=0):
    start = time.monotonic()
    with (RUN / (name + '.stdout')).open('xb') as out, (RUN / (name + '.stderr')).open('xb') as err:
        process = subprocess.Popen(argv, cwd=ROOT, stdout=out, stderr=err)
        code = process.wait()
    row = dict(name=name, argv=argv, expected=expected, exit_code=code,
               elapsed_seconds=time.monotonic() - start, closed=True)
    commands.append(row)
    save('commands.json', commands)
    print(name, code, flush=True)
    if code != expected:
        raise RuntimeError(name + ': unexpected exit')


def capture_sources():
    paths = ['morsehgp3D_v7/src/forest/full_coverage_certificate.hpp',
             'morsehgp3D_v7/tests/full_coverage_certificate_gate.cpp',
             'morsehgp3D_v7/src/forest/local_plateau.hpp',
             'morsehgp3D_v7/tests/local_plateau_gate.cpp', 'morsehgp3D_v7/CMakeLists.txt']
    result = {}
    for path in paths:
        target = RUN / 'sources' / path
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(ROOT / path, target)
        result[path] = sha(target)
    save('source_pins.json', result)
    return result


result = dict(status='failed', schema='mhgp7-coverage-local-qualification-v1',
              public_status='not_claimed', authority='structural_only', gcp_used=False)
try:
    pins = capture_sources()
    command('git_head', ['git', 'rev-parse', 'HEAD'])
    command('git_status', ['git', 'status', '--porcelain=v1'])
    command('compiler_version', ['g++', '--version'])
    command('uname', ['uname', '-a'])
    base_flags = ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                  '-isystem', str(ROOT / 'build/v7_boost_gate/extracted/usr/include')]
    gate = ROOT / 'morsehgp3D_v7/tests/full_coverage_certificate_gate.cpp'
    for name, flags in [('O2', ['-O2']), ('SAN', ['-O1', '-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer'])]:
        binary = RUN / name
        command('compile_' + name, base_flags + flags + ['-MMD', '-MF', str(RUN / (name + '.d')), str(gate), '-o', str(binary)])
        command(name + '_selftest', [str(binary), '--selftest'])
        command(name + '_unknown', [str(binary), '--unknown'], 2)
        result[name + '_elf_sha256'] = sha(binary)
    if (RUN / 'O2_selftest.stdout').read_bytes() != (RUN / 'SAN_selftest.stdout').read_bytes():
        raise RuntimeError('O2/SAN output mismatch')
    # Mutants only in private include trees, nominal product/gate frozen above.
    mutations = {
        'drop_continuation': ('out.contributions_.push_back({batch.level, segment, ref});',
                              'if (action.parents.size() != 1) out.contributions_.push_back({batch.level, segment, ref});'),
        'future_contribution': ('if (!full_coverage_detail::admitted(record.level, cut, closed)) break;',
                                '(void)record.level; // mutant ignores contribution date'),
        'final_root': ('if (!full_coverage_detail::admitted(forest.nodes()[next].level, cut, closed)) break;',
                       '// mutant follows future successor unconditionally'),
    }
    expected_failures = {'drop_continuation': 'growth.no_fake_node',
                         'future_contribution': 'growth.no_future_leak',
                         'final_root': 'replay.live_identity'}
    header = ROOT / 'morsehgp3D_v7/src/forest/full_coverage_certificate.hpp'
    for name, (before, after) in mutations.items():
        tree = RUN / 'mutants' / name / 'morsehgp3D_v7'
        (tree / 'src/forest').mkdir(parents=True)
        (tree / 'tests').mkdir()
        # Mechanical include redirection keeps the other exact product bytes.
        original = header.read_text()
        if original.count(before) != 1:
            raise RuntimeError('mutant replacement cardinality')
        mutated = original.replace(before, after).replace('"full_certificate.hpp"',
                    '"' + str(ROOT / 'morsehgp3D_v7/src/forest/full_certificate.hpp') + '"')
        (tree / 'src/forest/full_coverage_certificate.hpp').write_text(mutated)
        mutant_gate = tree / 'tests/full_coverage_certificate_gate.cpp'
        mutant_gate.write_text(gate.read_text().replace('"../oracle/local_plateau_oracle.hpp"',
                    '"' + str(ROOT / 'morsehgp3D_v7/oracle/local_plateau_oracle.hpp') + '"'))
        binary = RUN / name
        command('compile_' + name, base_flags + ['-O2', str(mutant_gate), '-o', str(binary)])
        command(name, [str(binary), '--selftest'], 1)
        if (RUN / (name + '.stderr')).read_text().strip() != 'FAIL ' + expected_failures[name]:
            raise RuntimeError(name + ': wrong causal failure')
    observed = {}
    for line in (RUN / 'O2.d').read_text().replace('\\\n', ' ').split()[1:]:
        path = Path(line)
        if path.is_file() and path.is_relative_to(ROOT):
            observed[str(path.relative_to(ROOT))] = sha(path)
    save('dependencies.json', observed)
    for path, digest in pins.items():
        if sha(ROOT / path) != digest:
            raise RuntimeError('source changed during qualification: ' + path)
    result['status'] = 'completed'
except BaseException as error:
    result['error'] = type(error).__name__ + ': ' + str(error)
finally:
    result['commands'] = len(commands)
    result['all_cpp_closed'] = all(row['closed'] for row in commands)
    save('receipt.json', result)
    print(json.dumps(result, sort_keys=True), flush=True)
sys.exit(0 if result['status'] == 'completed' else 1)
