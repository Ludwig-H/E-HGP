#!/usr/bin/env python3
"""Portable source/capture/causal reader; no compiler or engine invocation."""
import hashlib
import json
from pathlib import Path
import re
import sys
import zlib

ROOT = Path(__file__).resolve().parent


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def sha(value):
    return hashlib.sha256(value).hexdigest()


def safe(name):
    p = Path(name)
    return not p.is_absolute() and '..' not in p.parts and p.as_posix() == name


def main():
    manifest = json.loads((ROOT / 'MANIFEST.json').read_text())
    need(manifest['schema'] == 'terminal_batch_probe_local_v1' and manifest['public_status'] == 'not_claimed' and
         manifest['device_executed'] is False and manifest['GCP_used'] is False, 'scope_schema')
    need(set(manifest['files']) | {'MANIFEST.json'} ==
         {p.relative_to(ROOT).as_posix() for p in ROOT.rglob('*') if p.is_file()}, 'exact_file_domain')
    for name, pin in manifest['files'].items():
        need(safe(name) and sha((ROOT / name).read_bytes()) == pin, 'file_pin')
    mapping = json.loads((ROOT / 'storage_map.json').read_text())
    need(len(mapping) == manifest['logical_files'] and
         len({row['storage'] for row in mapping.values()}) == manifest['objects'], 'logical_counts')
    cache = {}

    def raw(name):
        need(safe(name) and name in mapping, 'logical_path:' + name)
        row = mapping[name]
        need(safe(row['storage']), 'safe_object_path')
        if row['storage'] not in cache:
            stored = (ROOT / row['storage']).read_bytes()
            need(sha(stored) == row['compressed_sha256'] and len(stored) == row['compressed_bytes'], 'storage_pin')
            cache[row['storage']] = zlib.decompress(stored)
        value = cache[row['storage']]
        need(sha(value) == row['sha256'] and len(value) == row['bytes'] and value[:4] != b'\x7fELF', 'raw_pin_no_ELF')
        return value

    def read(name):
        return json.loads(raw(name))

    for name in mapping:
        raw(name)
    if len(sys.argv) == 3 and sys.argv[1] == '--extract':
        sys.stdout.buffer.write(raw(sys.argv[2]))
        return
    need(len(sys.argv) == 1, 'reader_arguments')
    origins = sorted(((value + '/', key + '/') for key, value in manifest['origins'].items()), reverse=True)

    def local(path):
        for original, logical in origins:
            if path.startswith(original):
                return logical + path[len(original):]
        return None

    def stable(capture):
        r = read(capture + '/receipt.json')
        need(r['status'] == 'passed' and r['stable'] is True and r['error'] is None and
             r['device_executed'] is False and r['GCP_used'] is False, 'closed_local_capture')
        for kind in ('sources', 'tools', 'dependencies'):
            a, b = read(capture + '/' + kind + '_before.json'), read(capture + '/' + kind + '_after.json')
            need(a and a == b, 'before_after:' + kind)
            for path, pin in a.items():
                name = local(path)
                if name is not None:
                    need(mapping[name]['sha256'] == pin, 'source_octets')
            if kind == 'dependencies':
                need(not any('/boost/' in p or '/oracle/' in p for p in a), 'no_oracle_Boost_used')
        for path, pair in read(capture + '/binaries.json').items():
            need(pair['before'] == pair['after'] == manifest['omitted_ELF'][local(path)]['sha256'], 'ELF_attestation')
        return r

    def commands(capture, sequence, codes, recorded_expected=None):
        rows = read(capture + '/commands.json')
        need([row['name'] for row in rows] == sequence and len(rows) == len(codes), 'command_sequence')
        expectation = codes if recorded_expected is None else recorded_expected
        for row, code, recorded in zip(rows, codes, expectation, strict=True):
            need(row == read(capture + '/' + row['name'] + '.command.json') and
                 type(row['exit_code']) is int and row['exit_code'] == code and row['expected_exit'] == recorded,
                 'command_exit')
            for suffix in ('stdout', 'stderr'):
                need(mapping[capture + '/' + row['name'] + '.' + suffix]['sha256'] == row[suffix + '_sha256'],
                     'raw_command_output')
            need(not row['argv'][0].endswith(('/cuda_probe_compile_only', '/cuda_gate_compile_only')),
                 'CUDA_ELF_never_invoked')
        return {row['name']: row for row in rows}

    ph = stable('probe_host/o2_r1')
    ph_sequence = ['compiler', 'dependencies', 'compile'] + [f'n{n}_k{k}_batch{b}'
        for n, k in ((200, 10), (1, 10), (200, 1)) for b in (0, 1)] + ['invalid_' + str(i) for i in range(10)]
    commands('probe_host/o2_r1', ph_sequence, [0] * 9 + [2] * 10)
    need(ph['commands'] == 19 and ph['backend'] == 'host_emulation_only', 'probe_host_scope')
    # This archived pure comparator was consumed by the recorder. Execute only
    # these captured Python checks, never any C++ binary or compiler.
    comparator = {}
    exec(compile(raw('probe_host/o2_r1/compare.py'), 'captured_probe_compare.py', 'exec'), comparator)
    for n, k in ((200, 10), (1, 10), (200, 1)):
        a, b = [read(f'probe_host/o2_r1/n{n}_k{k}_batch{mode}.stdout') for mode in (0, 1)]
        need(comparator['compare'](a, b) == read(f'probe_host/o2_r1/n{n}_k{k}_comparison.json'),
             '34_fields_digests_RUSQHT_callbacks')
        for field in ('resolver_power_tests', 'resolver_materializations', 'resolver_supports_by_size',
                      'resolver_supports_tested'):
            need(a[field] == b[field], 'same_actual_work')
    lineage = read('probe_host/PROVENANCE.json')
    core = dict(lineage['unchanged_HD_sources'], **lineage['bench_sources'])
    for name, pin in core.items():
        for capture in ('probe_host/o2_r1', 'probe_nvcc/nvcc_r1', 'whole_gate/o2_r2',
                        'whole_gate/san_root_r1', 'whole_gate/nvcc_r1'):
            need(mapping[capture + '/prototype/' + name]['sha256'] == pin, 'same_frozen_HD_probe_sources')
    need(core['source/morsehgp3D_v7/bench/full_ball_tower_probe.cpp'] ==
         '21d0a5dd8e086506c91a8d3c0aec55c009bcbb901885085878f7cf3f5f450216', 'probe21d0')
    for capture, compile_name in (('probe_nvcc/nvcc_r1', 'compile_link'), ('whole_gate/nvcc_r1', 'compile')):
        receipt = stable(capture)
        rows = commands(capture, ['compiler', 'dependencies', compile_name, 'symbols'], [0] * 4)
        need(receipt['commands'] == 4 and receipt['cuda_ELF_invoked'] is False, 'NVCC_compile_only')
        argv = rows[compile_name]['argv']
        need(all(flag in argv for flag in ('--gpu-architecture=sm_120', '--Werror=cross-execution-space-call',
             '-Xcompiler=-pthread,-Wall,-Wextra,-Wpedantic,-Werror')), 'strict_SM120_flags')
        need(not any('diag-suppress' in arg for arg in argv), 'no_warning_suppression')
        need(not re.search(r'\bwarning\b|\berror\s*#', raw(capture + '/' + compile_name + '.stderr').decode(), re.I),
             'zero_NVCC_warning')
        need('gpu_terminal_batch_private::kernel' in raw(capture + '/symbols.stdout').decode(), 'linked_batch_kernel')
    seq = ['compiler', 'dependencies', 'compile', 'selftest', 'wrong_terminal', 'after_prefix', 'unknown', 'missing']
    for capture, mode in (('whole_gate/o2_r2', 'o2'), ('whole_gate/san_root_r1', 'san')):
        receipt = stable(capture)
        rows = commands(capture, seq, [0, 0, 0, 0, 4, 4, 2, 2])
        need(receipt['mode'] == mode and receipt['commands'] == 8 and receipt['source_gate_sha256'] ==
             '98e426f288d52d0331b3480f3a8233ac525814069893296794a46e1a19b30d29', 'gate98e426_scope')
        need(mapping[capture + '/whole_gate.cu']['sha256'] == receipt['source_gate_sha256'], 'gate_source_copy')
        if mode == 'san':
            need('-fsanitize=address,undefined' in rows['compile']['argv'] and
                 receipt['sanitizer_environment'] == dict(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                     UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'), 'ASan_UBSan_LSan')
        for name in ('selftest', 'wrong_terminal', 'after_prefix', 'unknown', 'missing'):
            for suffix in ('stdout', 'stderr'):
                need(raw(capture + '/' + name + '.' + suffix) == raw('whole_gate/o2_r2/' + name + '.' + suffix),
                     'O2_SAN_exact_output')
        for name, cause in (('wrong_terminal', 'causal_direct_terminal_rejected'),
                            ('after_prefix', 'causal_after_prefix_global_empty')):
            need(cause in raw(capture + '/' + name + '.stderr').decode() and
                 raw(capture + '/' + name + '.stdout') == b'', 'causal_rejection_no_payload')
    gate = read('whole_gate/o2_r2/selftest.stdout')
    need(gate['status'] == 'passed' and gate['scope'] == 'bounded_real_census_batch_FULL' and
         gate['backend'] == 'cpu_census_host_stub_terminal_batch' and gate['device_executed'] is False and
         gate['checks'] == 372536 and gate['fixtures'] == 10 and gate['physical_pairs'] == 20 and
         gate['nodes_compared'] == 46732 and gate['contributions_compared'] == 28666 and
         gate['contexts'] == gate['closed_contexts'] == 9 and gate['zero_context_cases'] == 1 and
         gate['batches'] == gate['launches'] == 59 and gate['extra_shell_rows'] == 7 and
         gate['q3_rows'] == 7808 and gate['q4_rows'] == 5976, 'whole_gate_floor')
    need(sum(r['direct_terminals'] for r in gate['per_k']) == 10326 and
         sum(r['Q'] for r in gate['per_k']) == 2911 and sum(r['H'] for r in gate['per_k']) == 715 and
         [r['K'] for r in gate['per_k']] == list(range(1, 11)) and
         gate['per_k'][8]['direct_terminals'] == 1752 and gate['per_k'][9]['direct_terminals'] == 1758,
         'direct_K_and_QH_floors')
    failed = read('whole_gate/o2_r1/receipt.json')
    need(failed['status'] == 'failed' and failed['stable'] is True and failed['commands'] == 3 and
         failed['error'] == 'RuntimeError: unexpected_exit:compile', 'preserved_compile_failure')
    commands('whole_gate/o2_r1', ['compiler', 'dependencies', 'compile'], [0, 0, 1], [0, 0, 0])
    need('narrowing conversion' in raw('whole_gate/o2_r1/compile.stderr').decode(), 'fixture_failure_cause')
    need(len(manifest['omitted_ELF']) == 5, 'five_ELF_omitted')
    print(json.dumps(dict(status='passed_portable_local_probe_and_gate', probe_commands=19,
        probe_equal_fields=34, whole_gate_physical_pairs=20, whole_gate_direct_terminals=10326,
        host_gate_commands_per_mode=8, strict_NVCC_compilations=2, zero_NVCC_warning=True,
        preserved_fixture_compile_failure=True, ELF_and_dependencies_before_after_attested=True,
        omitted_ELF_live_bytes_rechecked=False, device_executed=False, GCP_used=False,
        public_status='not_claimed'), sort_keys=True))


if __name__ == '__main__':
    main()
