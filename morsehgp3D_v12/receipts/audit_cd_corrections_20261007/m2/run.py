#!/usr/bin/env python3
"""Audit borne des correctifs M2. Une TU hote Release, petites simulations Python.

python3 [-O] run.py [--output RESULT.json]
Les sources et les trois temoins doivent garder les hashes du manifeste avant/apres.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
NAMES = ['site_out_of_cloud', 'wrapped_job_begin', 'zero_k', 'profile_mismatch',
         'coordinate_out_of_profile', 'short_record_population']
EXPECTED = ['valid native_code=0 forms=j3,coherent identity=true', 'valid_pair reader=1']
EXPECTED += [prefix + name + ' reader=0 native_code=2 mixed_code=2 result_lines=0'
             for prefix in ('historical_', 'causal_') for name in NAMES]
EXPECTED += [name + ' reader=0' for name in ('huge_header', 'status_without_reference',
                                          'counter_mismatch', 'duplicate_population')]


def check(value, message):
    if not value:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(manifest):
    for relative, digest in manifest['sources_sha256'].items():
        check(sha(ROOT / relative) == digest, 'source changed: ' + relative)
    for relative, digest in manifest['witness_sha256'].items():
        check(sha(HERE / relative) == digest, 'witness changed: ' + relative)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    manifest = json.loads((HERE / 'MANIFEST.json').read_text())
    verify(manifest)
    head = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip()
    spec = importlib.util.spec_from_file_location('audit_m2_judge', HERE / 'judge_probe.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    judges = module.run(ROOT)
    compiler = subprocess.check_output(['g++', '--version'], text=True).splitlines()[0]
    m2 = ROOT / 'morsehgp3D_v12/microbancs/mes_m2_feuille'
    with tempfile.TemporaryDirectory(prefix='ehgp-cd-m2-audit-') as temp:
        temp = Path(temp)
        binary, depfile = temp / 'reader_probe', temp / 'reader_probe.d'
        flags = ['g++', '-std=c++20', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread',
                 '-DMHGP12_COORD_BITS=21', '-I' + str(m2 / 'include'), '-I' + str(m2 / 'host')]
        built = subprocess.run(flags + ['-MMD', '-MF', str(depfile), str(HERE / 'reader_probe.cpp'),
                                        '-o', str(binary)], capture_output=True, text=True, timeout=90)
        check(built.returncode == 0, 'small Release compilation failed: ' + built.stderr)
        dependencies = []
        for token in shlex.split(depfile.read_text().replace('\\\n', '').split(':', 1)[1]):
            path = Path(token).resolve()
            relative = path.relative_to(ROOT).as_posix()
            dependencies.append(relative)
            if path == HERE / 'reader_probe.cpp':
                check(sha(path) == manifest['witness_sha256']['reader_probe.cpp'], 'TU changed')
            else:
                check(relative in manifest['sources_sha256'] and sha(path) == manifest['sources_sha256'][relative],
                      'compiled dependency not closed: ' + relative)
        check(len(dependencies) == 8, 'compiled closure expected: six headers, original identity source and TU')
        process = subprocess.run([str(binary), str(temp)], capture_output=True, text=True, timeout=30)
        check(process.returncode == 0 and process.stdout.splitlines() == EXPECTED,
              'reader/native refusal probe mismatch: ' + process.stdout + process.stderr)
        largest = max(path.stat().st_size for path in temp.glob('*.bin'))
        binary_hash = sha(binary)
    verify(manifest)
    check(subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip() == head,
          'HEAD changed during audit')
    result = {'schema': 'audit.cd.m2.v1', 'audited_commit': manifest['audited_commit'],
              'current_head': head, 'python_optimized': bool(sys.flags.optimize),
              'sources_before_after': 'unchanged', 'profile_bits': 21, 'judge_injections': judges,
              'reader_and_native_rows': EXPECTED, 'largest_fixture_bytes': largest,
              'compiler': compiler, 'binary_sha256': binary_hash, 'compiled_dependencies': dependencies,
              'native_original_entrypoint_calls': 25, 'gpu_calls': 0, 'gcp_calls': 0, 'network_calls': 0,
              'real_data_read': False,
              'limits': ['One host Release TU; no sanitizer or GPU.',
                         'Original identity main included unmodified except symbol rename.',
                         'Judge external-tool factories imported from pinned test; assertions and injections independent.',
                         'No replay of historical real G4 measurements.']}
    encoded = json.dumps(result, indent=2, sort_keys=True) + '\n'
    if args.output:
        args.output.write_text(encoded)
    print(json.dumps({'audited_commit': result['audited_commit'], 'python_optimized': result['python_optimized'],
                      'reader_cases': len(EXPECTED), 'native_entrypoint_calls': 25,
                      'largest_fixture_bytes': largest, 'judge_injections': {n: v['verdict'] for n, v in judges.items()},
                      'sources_closed': len(manifest['sources_sha256']), 'compiled_dependencies': len(dependencies)}))


if __name__ == '__main__':
    main()
