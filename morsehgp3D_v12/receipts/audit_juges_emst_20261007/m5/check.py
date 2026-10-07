#!/usr/bin/env python3
"""Small native checks of the M5 reader, task arithmetic and admission order."""
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
PIN = '1f7642e105aebd76632c58c63fdfd5b5c0824779'
M5 = ROOT/'morsehgp3D_v12/microbancs/mes_m5_parcours'
M2 = ROOT/'morsehgp3D_v12/microbancs/mes_m2_feuille'
OLD = ROOT/'morsehgp3D_v12/receipts/audit_b_m5_20261007/juge_format/format_probe.cpp'


def need(value, why):
    if not value:
        raise RuntimeError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def hashes(paths):
    return {str(p.relative_to(ROOT)): sha(p.read_bytes()) for p in paths}


def main():
    paths = sorted(set(M5.glob('include/**/*.hpp')) | set(M2.glob('include/**/*.hpp')) |
                   {M5/'host/format_selftest.cpp', M5/'README.md', M5/'CMakeLists.txt',
                    ROOT/'morsehgp3D_v11/src/catalogue/boxes.cpp', OLD})
    before = hashes(paths)
    for p in paths:
        committed = subprocess.check_output(['git', 'show', PIN+':'+str(p.relative_to(ROOT))], cwd=ROOT)
        need(sha(committed) == before[str(p.relative_to(ROOT))], 'source differs from pin')
    own = {p.name: sha(p.read_bytes()) for p in [HERE/'check.py', HERE/'probe.cpp']}
    # Reuse the immutable old witness; change only four old expected admissions to refusals.
    old = OLD.read_text()
    for name in ['impossible_test_count', 'nonterminal_leaf', 'wrong_root_envelope', 'wrapped_test_sum']:
        text = 'read_case(folder, "'+name+'", d, true);'
        need(old.count(text) == 1, 'old witness shape')
        old = old.replace(text, text.replace('true', 'false'))
    deps, binaries, outputs = {}, {}, {}
    flags = ['-std=c++20', '-O2', '-DNDEBUG', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
             '-DMHGP12_COORD_BITS=21']
    with tempfile.TemporaryDirectory(prefix='ehgp-m5-corrections-') as temporary:
        tmp = Path(temporary)
        old_tu = tmp/'old_format.cpp'
        old_tu.write_text(old)
        for name, source, args in [('old_format', old_tu, [str(tmp)]),
                                   ('delivered_format', M5/'host/format_selftest.cpp', [str(tmp)]),
                                   ('capacity_guard', HERE/'probe.cpp', [])]:
            exe, dep = tmp/name, tmp/(name+'.d')
            command = ['c++', *flags, '-I', str(M5/'include'), '-I', str(M2/'include'),
                       '-MMD', '-MF', str(dep), str(source), '-o', str(exe)]
            built = subprocess.run(command, capture_output=True, text=True, timeout=60)
            need(built.returncode == 0 and not built.stderr, 'compile '+name+': '+built.stderr)
            dependencies = dep.read_text().replace('\\\n', ' ').split(':', 1)[1].split()
            for raw in dependencies:
                p = Path(raw).resolve()
                if p == old_tu:
                    continue
                rel = str(p.relative_to(ROOT))
                h = sha(p.read_bytes())
                need((rel in before and before[rel] == h) or p == HERE/'probe.cpp', 'unclosed compiler dependency '+rel)
                deps[rel] = h
            result = subprocess.run([str(exe), *args], capture_output=True, text=True, timeout=10)
            need(result.returncode == 0 and not result.stderr, 'native witness '+name+': '+result.stdout+result.stderr)
            outputs[name] = (result.stdout.splitlines() if name == 'old_format'
                             else [json.loads(line) for line in result.stdout.splitlines()])
            binaries[name] = sha(exe.read_bytes())
    need(len(outputs['old_format']) == 18, 'old format coverage')
    for name in ['impossible_test_count', 'nonterminal_leaf', 'wrong_root_envelope', 'wrapped_test_sum']:
        need('read '+name+' 0' in outputs['old_format'], 'old invalid admitted')
    need(outputs['capacity_guard'][0]['emit_records'] == 520, 'capacity coverage')
    need(len(outputs['capacity_guard']) == 7, 'guard coverage')
    need(hashes(paths) == before, 'sources changed')
    need(own == {p.name: sha(p.read_bytes()) for p in [HERE/'check.py', HERE/'probe.cpp']}, 'witness changed')
    print(json.dumps({'schema': 'audit.v12.juges_emst.m5.v1', 'pin': PIN,
                      'source_sha256': before, 'compiler_dependencies_sha256': deps,
                      'witness_sha256': own, 'transformed_old_witness_sha256': sha(old.encode()),
                      'old_witness_transformation': 'Only the four expected admissions change to refusals.',
                      'flags': flags, 'compiler': subprocess.check_output(['c++', '--version'], text=True).splitlines()[0],
                      'binary_sha256': binaries, 'results': outputs, 'sources_before_after_equal': True,
                      'gcp_used': False, 'cuda_executed': False, 'large_allocations': False,
                      'scope': 'Reader and shared host arithmetic; driver reservations/kernels mocked after fabricated totals. No GPU or product catalogue qualification.'},
                     ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
