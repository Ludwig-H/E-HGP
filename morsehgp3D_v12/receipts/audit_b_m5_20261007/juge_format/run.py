#!/usr/bin/env python3
"""Rejoue les petites preuves MES-M5. Aucun GPU/reseau/GCP ni donnees reelles.

Sources et temoins exiges identiques au MANIFEST avant/apres ; fermeture des
headers effectivement compiles par -MMD ; exceptions actives avec python -O.
"""
import argparse
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
EXPECTED = ['read valid_one 1', 'read site_oob 0', 'read k_zero 0', 'read coordinate_oob 0',
            'read begin_wrap 0', 'read ledger_mismatch 0', 'read refusal_with_prefix 0',
            'read impossible_test_count 1', 'read nonterminal_leaf 1', 'read wrong_root_envelope 1',
            'read wrapped_test_sum 1', 'read short_count 0', 'compare valid 1', 'compare list 0',
            'compare range 0', 'compare metadata 0', 'compare duplicate_box 0', 'compare ledger 0']


def check(value, message):
    if not value:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(manifest):
    for relative, expected in manifest['sources_sha256'].items():
        check(sha(ROOT / relative) == expected, 'source changed: ' + relative)
    for relative, expected in manifest['witness_sha256'].items():
        check(sha(HERE / relative) == expected, 'witness changed: ' + relative)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    manifest = json.loads((HERE / 'MANIFEST.json').read_text())
    verify(manifest)
    head = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip()
    spec = importlib.util.spec_from_file_location('runner_probe', HERE / 'runner_probe.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    runner = module.load(ROOT)
    captured = io.StringIO()
    with contextlib.redirect_stdout(captured):
        code = runner.selftest_judge()
    selftest = json.loads(captured.getvalue())
    check(code == 0 and selftest['selftest_judge'] == 'conforme' and len(selftest['scenarios']) == 16 and
          all(s['ok'] for s in selftest['scenarios']), 'existing selftest must pass its 16 checks')
    probes = module.run(ROOT)
    compiler = subprocess.check_output(['g++', '--version'], text=True).splitlines()[0]
    with tempfile.TemporaryDirectory(prefix='ehgp-m5-format-audit-') as temp:
        temp = Path(temp)
        binary, depfile = temp / 'format_probe', temp / 'format_probe.d'
        argv = ['g++', '-std=c++20', '-O1', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                '-I' + str(ROOT / 'morsehgp3D_v12/microbancs/mes_m5_parcours/include'),
                '-I' + str(ROOT / 'morsehgp3D_v12/microbancs/mes_m2_feuille/include'),
                '-MMD', '-MF', str(depfile), str(HERE / 'format_probe.cpp'), '-o', str(binary)]
        build = subprocess.run(argv, capture_output=True, text=True, timeout=90)
        check(build.returncode == 0, 'small TU compilation failed: ' + build.stderr)
        dependencies = []
        for name in shlex.split(depfile.read_text().replace('\\\n', '').split(':', 1)[1]):
            path = Path(name).resolve()
            relative = path.relative_to(ROOT).as_posix()
            dependencies.append(relative)
            if path == HERE / 'format_probe.cpp':
                check(sha(path) == manifest['witness_sha256']['format_probe.cpp'], 'TU changed')
            else:
                check(relative in manifest['sources_sha256'] and sha(path) == manifest['sources_sha256'][relative],
                      'unclosed compiled header: ' + relative)
        check(len(dependencies) == 8, 'compiled closure expected: seven headers plus TU')
        process = subprocess.run([str(binary), str(temp)], capture_output=True, text=True, timeout=20)
        check(process.returncode == 0 and process.stdout.splitlines() == EXPECTED,
              'format/comparison probe mismatch: ' + process.stdout + process.stderr)
        binary_hash = sha(binary)
        largest = max(p.stat().st_size for p in temp.glob('*.bin'))
    verify(manifest)
    check(subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip() == head,
          'HEAD changed during audit')
    result = {'schema': 'audit.b_m5.judge_format.v1', 'audited_commit': manifest['audited_commit'],
              'current_head': head, 'python_optimized': bool(sys.flags.optimize),
              'source_hashes_before_after': 'identical', 'existing_selftest_cases': 16,
              'main_injections': probes, 'format_and_compare': EXPECTED, 'largest_synthetic_dump_bytes': largest,
              'compiler': compiler, 'compiled_dependencies': dependencies, 'binary_sha256': binary_hash,
              'cloud_calls': 0, 'gpu_calls': 0, 'real_data_read': False,
              'limitations': ['External MES-M5 programs mocked; no native/GPU adoption demonstrated.',
                              'C++ probe executes only reader and comparator on tiny synthetic dumps.',
                              'System headers not pinned; local compilation dependency closure pinned.']}
    encoded = json.dumps(result, indent=2, sort_keys=True) + '\n'
    if args.output:
        args.output.write_text(encoded)
    print(json.dumps({'audited_commit': result['audited_commit'], 'python_optimized': result['python_optimized'],
                      'selftest': 16, 'main_injections': {k: v['verdict'] for k, v in probes.items()},
                      'format_and_compare_cases': len(EXPECTED), 'largest_dump_bytes': largest,
                      'sources_closed': len(manifest['sources_sha256']), 'compiled_dependencies': len(dependencies)}))


if __name__ == '__main__':
    main()
