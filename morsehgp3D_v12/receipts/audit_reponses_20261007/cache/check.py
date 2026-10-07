#!/usr/bin/env python3
"""Porte officielle eviction_admise et son seul mutant ; six petites sondes ASan/UBSan."""
import hashlib
import json
import os
from pathlib import Path
import resource
import subprocess
import tempfile


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def call(args, **kwargs):
    r = subprocess.run(args, capture_output=True, text=True, timeout=45, **kwargs)
    need(r.returncode == 0, 'commande refusee : ' + r.stderr[-1500:])
    return r.stdout


def main():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    here = Path(__file__).resolve().parent
    repo = here.parents[3]
    capture = json.loads((here / 'capture.json').read_text())
    base, closure = capture['base'], capture['previous_receipt_commit']
    tests = 'morsehgp3D_v12/tests/'
    core = 'morsehgp3D_v12/src/core/'
    records, compilers = [], {}
    with tempfile.TemporaryDirectory(prefix='mhgp12-cache-response-') as td:
        temp = Path(td)
        paths = call(['git', 'ls-tree', '-r', '--name-only', base, '--', core], cwd=repo).splitlines()
        paths += [tests + 'core/alloc_fault.cpp', tests + 'support/test.hpp']
        for rel in paths:
            target = temp / rel; target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(call(['git', 'show', base + ':' + rel], cwd=repo))
        for relative in ['audit_reprise_20261007/buffer/buffer_capture.patch',
                         'audit_cache_concurrence_20261007/delta.patch',
                         'audit_cache_poison_20261007/probe.patch']:
            patch = call(['git', 'show', closure + ':morsehgp3D_v12/receipts/' + relative], cwd=repo)
            call(['git', 'apply', '-'], input=patch, cwd=temp)
        call(['git', 'apply', str(here / 'official_test.patch')], cwd=temp)
        compiled = [core + 'buffer.cpp', core + 'buffer.hpp', core + 'ledger.cpp', tests + 'core/alloc_fault.cpp',
                    tests + 'core/cache_poison_probe.cpp', tests + 'support/test.hpp']
        for rel in compiled:
            need(hashlib.sha256((temp / rel).read_bytes()).hexdigest() == capture['source_sha256'][rel], 'hash ' + rel)
        src = temp / 'morsehgp3D_v12/src'
        support = temp / tests / 'support'
        cpp, ledger = temp / core / 'buffer.cpp', temp / core / 'ledger.cpp'
        test = temp / tests / 'core/alloc_fault.cpp'
        mutant = capture['mutant']
        need(mutant['fichier'] == 'src/core/buffer.cpp', 'mutant file')
        body = cpp.read_text(); need(body.count(mutant['cherche']) == 1, 'unique mutant anchor')
        mutation = temp / 'no_reread.cpp'; mutation.write_text(body.replace(mutant['cherche'], mutant['remplace']))
        compilers['g++'] = call(['g++', '--version']).splitlines()[0]
        for label, code in [('official', cpp), ('refus_sans_relecture', mutation)]:
            exe = temp / label
            call(['g++', '-std=c++20', '-O2', '-pthread', '-DMHGP12_COORD_BITS=21', '-I', str(src), '-I', str(support),
                  str(test), str(code), str(ledger), '-o', str(exe)])
            result = subprocess.run([str(exe), 'eviction_admise'], capture_output=True, text=True, timeout=10)
            need(not result.stderr and result.returncode == (0 if label == 'official' else 1), 'official issue')
            if label == 'official':
                need('mhgp12_test_ok tests=1 controles=12' in result.stdout, 'official controls')
            else:
                need('small_arm.second_ok' in result.stdout and 'cached_arm.second_ok' not in result.stdout,
                     'mutant killed by small arm')
            records.append(dict(kind='unit', label=label, returncode=result.returncode, stdout=result.stdout))
        probe = temp / tests / 'core/cache_poison_probe.cpp'
        for compiler in ['clang++', 'g++']:
            compilers[compiler] = call([compiler, '--version']).splitlines()[0]
            flags = ['-std=c++20', '-O1', '-g', '-fno-omit-frame-pointer', '-fsanitize=address,undefined',
                     '-DMHGP12_COORD_BITS=21']
            env = os.environ.copy(); env['ASAN_OPTIONS'] = 'detect_leaks=0:abort_on_error=0'
            env['UBSAN_OPTIONS'] = 'halt_on_error=1'
            if compiler == 'clang++':
                runtime = call([compiler, '-print-runtime-dir']).strip()
                flags += ['-shared-libasan']
                env['LD_LIBRARY_PATH'] = runtime + ':' + env.get('LD_LIBRARY_PATH', '')
            exe = temp / ('poison_' + compiler)
            call([compiler, *flags, '-I', str(src), str(cpp), str(probe), '-o', str(exe)])
            for mode in ['garde', 'lecture_apres_restitution', 'lecture_hors_taille']:
                result = subprocess.run([str(exe), mode], capture_output=True, text=True, timeout=10, env=env)
                expected = mode != 'garde'
                poison = 'ERROR: AddressSanitizer: use-after-poison' in result.stderr
                need(result.returncode == (-6 if expected else 0) and poison == expected, 'poison result')
                if not expected:
                    need(result.stdout.strip() == 'sonde_cache_ok garde' and not result.stderr, 'poison control')
                records.append(dict(kind='poison', compiler=compiler, mode=mode, returncode=result.returncode,
                                    use_after_poison=poison))
    print(json.dumps(dict(source_sha256=capture['source_sha256'], observations=records, compilers=compilers,
                          scope='one_official_gate_one_mutant_six_poison_probes_no_pipeline_no_GCP'),
                     indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
