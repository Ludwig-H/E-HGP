#!/usr/bin/env python3
"""Trois binaires, neuf petits processus ASan/UBSan ; aucun moteur ni code vivant compile."""
import hashlib
import json
import os
from pathlib import Path
import resource
import subprocess
import tempfile


def need(value, message):
    if not value:
        raise RuntimeError(message)


def run(command, **kwargs):
    result = subprocess.run(command, capture_output=True, text=True, timeout=45, **kwargs)
    need(result.returncode == 0, 'commande refusee : ' + result.stderr[-1500:])
    return result.stdout


def main():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    here = Path(__file__).resolve().parent
    root = here.parents[2]
    capture = json.loads((here / 'capture.json').read_text())
    base = capture['base']
    closure = capture['memory_snapshot_receipt_commit']
    patch_path = 'morsehgp3D_v12/receipts/audit_reprise_20261007/buffer/buffer_capture.patch'
    memory_patch = run(['git', 'show', closure + ':' + patch_path], cwd=root)
    records = []
    compilers = {}
    with tempfile.TemporaryDirectory(prefix='mhgp12-cache-poison-') as td:
        temp = Path(td)
        core = run(['git', 'ls-tree', '-r', '--name-only', base, '--', 'morsehgp3D_v12/src/core'], cwd=root)
        for rel in core.splitlines():
            target = temp / rel
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(run(['git', 'show', base + ':' + rel], cwd=root))
        run(['git', 'apply', '-'], cwd=temp, input=memory_patch)
        run(['git', 'apply', str(here / 'probe.patch')], cwd=temp)
        for rel, expected in capture['source_sha256'].items():
            need(hashlib.sha256((temp / rel).read_bytes()).hexdigest() == expected, 'empreinte source ' + rel)
        source = temp / 'morsehgp3D_v12/src'
        probe = temp / 'morsehgp3D_v12/tests/core/cache_poison_probe.cpp'
        good = source / 'core/buffer.cpp'
        old_guard = '''#if defined(__SANITIZE_ADDRESS__)
#define MHGP12_ASAN_POISON 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define MHGP12_ASAN_POISON 1
#endif
#endif'''
        body = good.read_text()
        need(body.count(old_guard) == 1, 'garde ASan ambigue')
        mutant = temp / 'buffer_gcc_guard_only.cpp'
        mutant.write_text(body.replace(old_guard, '#if defined(__SANITIZE_ADDRESS__)\n#define MHGP12_ASAN_POISON 1\n#endif'))
        for compiler, label, cpp in [('clang++', 'clang_corrected', good),
                                     ('clang++', 'clang_gcc_guard_only', mutant),
                                     ('g++', 'gcc_corrected', good)]:
            compilers[compiler] = run([compiler, '--version']).splitlines()[0]
            flags = ['-std=c++20', '-O1', '-g', '-fno-omit-frame-pointer', '-fsanitize=address,undefined',
                     '-DMHGP12_COORD_BITS=21']
            environment = os.environ.copy()
            environment['ASAN_OPTIONS'] = 'detect_leaks=0:abort_on_error=0'
            environment['UBSAN_OPTIONS'] = 'halt_on_error=1'
            if compiler == 'clang++':
                runtime = run([compiler, '-print-runtime-dir']).strip()
                need((Path(runtime) / 'libclang_rt.asan-x86_64.so').is_file(), 'runtime ASan Clang absent')
                flags.append('-shared-libasan')
                environment['LD_LIBRARY_PATH'] = runtime + ':' + environment.get('LD_LIBRARY_PATH', '')
            exe = temp / label
            run([compiler, *flags, '-I', str(source), str(cpp), str(probe), '-o', str(exe)])
            for mode in ['garde', 'lecture_apres_restitution', 'lecture_hors_taille']:
                result = subprocess.run([str(exe), mode], capture_output=True, text=True,
                                        timeout=10, env=environment)
                poison = 'ERROR: AddressSanitizer: use-after-poison' in result.stderr
                expected_poison = label != 'clang_gcc_guard_only' and mode != 'garde'
                need(result.returncode == (-6 if expected_poison else 0), 'issue inattendue ' + label + '/' + mode)
                need(poison == expected_poison, 'diagnostic inattendu ' + label + '/' + mode)
                if not expected_poison:
                    need('sonde_cache_ok ' + mode in result.stdout and not result.stderr, 'controle sain/ancien')
                records.append(dict(variant=label, mode=mode, returncode=result.returncode,
                                    use_after_poison=poison, normal_completion=not expected_poison))
    print(json.dumps(dict(scope='three_small_binaries_only_no_pipeline_no_GCP',
                          source_sha256=capture['source_sha256'], compilers=compilers,
                          memory_patch_sha256=hashlib.sha256(memory_patch.encode()).hexdigest(),
                          sanitizers='address,undefined', clang_runtime='shared', leak_detection=False,
                          core_dumps=False, observations=records), indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
