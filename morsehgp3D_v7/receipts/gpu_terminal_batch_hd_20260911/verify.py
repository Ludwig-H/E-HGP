#!/usr/bin/env python3
"""Portable proof reader. No compiler, engine, vendor or ELF execution."""
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
    path = Path(name)
    return not path.is_absolute() and '..' not in path.parts and path.as_posix() == name


def main():
    manifest = json.loads((ROOT / 'MANIFEST.json').read_text())
    need(manifest['schema'] == 'private_HD_restore_compile_host_v1', 'schema')
    need(manifest['public_status'] == 'not_claimed' and manifest['device_executed'] is False and
         manifest['GCP_used'] is False, 'packet_scope')
    need(set(manifest['files']) | {'MANIFEST.json'} ==
         {p.relative_to(ROOT).as_posix() for p in ROOT.rglob('*') if p.is_file()}, 'exact_packet_file_domain')
    for name, pin in manifest['files'].items():
        need(safe(name) and sha((ROOT / name).read_bytes()) == pin, 'packet_hash:' + name)
    mapping = json.loads((ROOT / 'storage_map.json').read_text())
    need(len(mapping) == manifest['logical_files'] and
         len({row['storage'] for row in mapping.values()}) == manifest['objects'], 'logical_counts')
    cache = {}

    def raw(name):
        need(safe(name) and name in mapping, 'logical_path:' + name)
        row = mapping[name]
        need(safe(row['storage']), 'object_path')
        if row['storage'] not in cache:
            stored = (ROOT / row['storage']).read_bytes()
            need(sha(stored) == row['compressed_sha256'] and len(stored) == row['compressed_bytes'],
                 'compressed_pin')
            cache[row['storage']] = zlib.decompress(stored)
        value = cache[row['storage']]
        need(sha(value) == row['sha256'] and len(value) == row['bytes'] and value[:4] != b'\x7fELF',
             'uncompressed_pin_no_ELF')
        return value

    def read(name):
        return json.loads(raw(name))

    for name in mapping:
        raw(name)
    if len(sys.argv) == 3 and sys.argv[1] == '--extract':
        sys.stdout.buffer.write(raw(sys.argv[2]))
        return
    need(len(sys.argv) == 1, 'reader_arguments')
    root = manifest['original_root'] + '/'

    def local_name(absolute):
        need(absolute.startswith(root), 'local_source_prefix')
        return absolute[len(root):]

    def stable(capture):
        receipt = read(capture + '/receipt.json')
        need(receipt['status'] == 'passed' and receipt['stable'] is True and receipt['error'] is None,
             'closed:' + capture)
        for kind in ('sources', 'toolchain', 'dependencies'):
            before, after = read(capture + '/' + kind + '_before.json'), read(capture + '/' + kind + '_after.json')
            need(before and before == after, 'stable_capture:' + kind)
            for path, pin in before.items():
                if path.startswith(root):
                    need(mapping[local_name(path)]['sha256'] == pin, 'captured_source_octets')
        for path, pair in read(capture + '/binaries.json').items():
            name = local_name(path)
            need(pair['before'] == pair['after'] == manifest['omitted_ELF'][name]['sha256'],
                 'ELF_attestation_not_live_bytes')
        return receipt

    def commands(capture, sequence, expected_codes):
        rows = read(capture + '/commands.json')
        need([r['name'] for r in rows] == sequence, 'command_sequence:' + capture)
        for row, expected in zip(rows, expected_codes, strict=True):
            need(row == read(capture + '/' + row['name'] + '.command.json'), 'command_copy')
            need(type(row['exit_code']) is int and row['exit_code'] == row['expected_exit'] == expected,
                 'command_exit:' + row['name'])
            for suffix in ('stdout', 'stderr'):
                need(mapping[capture + '/' + row['name'] + '.' + suffix]['sha256'] == row[suffix + '_sha256'],
                     'command_stream')
            need(not row['argv'][0].endswith('/cuda_compile_only'), 'CUDA_ELF_never_executed')
        return {row['name']: row for row in rows}

    old = read('historical_warning_capture/nvcc_compile_link.command.json')
    old_stderr = raw('historical_warning_capture/nvcc_compile_link.stderr').decode()
    need(old['exit_code'] == 0 and old['stderr_sha256'] == sha(old_stderr.encode()) and
         len(re.findall(r'warning #200(?:11|14)-D', old_stderr)) == 6 and
         '--Werror=cross-execution-space-call' not in old['argv'], 'old_six_warnings_not_promoted')
    strict = stable('strict_r2')
    need(strict['cross_execution_space_call_errors_fatal'] is True and strict['compiler_target'] == 'sm_120',
         'strict_target')
    for flag in ('device_executed', 'cuda_ELF_invoked', 'geometry_executed', 'complete_tower_executed', 'GCP_used'):
        need(strict[flag] is False, 'strict_scope')
    seq = ['host_version', 'nvcc_version', 'historical_nvcc_deps', 'historical_nvcc_compile_link',
           'corrected_nvcc_deps', 'corrected_nvcc_compile_link', 'corrected_symbols',
           'historical_host_deps', 'historical_native_expansion', 'corrected_host_deps',
           'corrected_native_expansion', 'corrected_host_compile', 'corrected_host_abi',
           'corrected_unknown', 'corrected_missing']
    rows = commands('strict_r2', seq, [0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2])
    need(strict['commands'] == 15, 'strict_commands')
    for name in ('historical_nvcc_compile_link', 'corrected_nvcc_compile_link'):
        argv = rows[name]['argv']
        need(all(flag in argv for flag in ('--Werror=cross-execution-space-call',
             '--gpu-architecture=sm_120', '-Xcompiler=-pthread,-Wall,-Wextra,-Wpedantic,-Werror')),
             'cross_space_fatal_both_arms')
        need(not any('diag-suppress' in flag for flag in argv), 'no_diagnostic_suppression')
    negative = raw('strict_r2/historical_nvcc_compile_link.stderr').decode()
    need('error:' in negative and 'calling a __host__ function' in negative and 'ball_key_reduce' in negative,
         'negative_compile_cause')
    positive = raw('strict_r2/corrected_nvcc_compile_link.stderr').decode()
    need(not re.search(r'\bwarning\b|\berror\s*#', positive, re.I), 'corrected_zero_warning')
    expected = read('strict_r2/abi_expected.json')
    explicit = {name: int(value) for name, value in re.findall(
        r'X\("([^"]+)",[^\n]*, (\d+)\)', raw('strict_r2/corrected/abi_gate.cu').decode())}
    need(expected == explicit and len(expected) == 63, 'explicit_63_ABI')
    need(read('strict_r2/corrected_host_abi.stdout') == dict(status='host_abi_layout_only',
         device_executed=False, GCP_used=False, fields=expected), 'host_ABI')
    native = [raw('strict_r2/' + mode + '_native_expansion.stdout').decode().splitlines()
              for mode in ('historical', 'corrected')]
    need([x.strip() for x in native[0]] == [x.strip() for x in native[1]], 'native_expansion_equal')
    delta = read('strict_r2/annotation_delta.json')
    need(len(delta) == 3, 'exact_three_source_deltas')
    declarations = ('inline u64 ugcd64(', 'inline u128 ugcd128(',
                    'inline bool q4_center_strictly_inside(', 'inline BallKey ball_key_reduce(')
    count = 0
    prefix = 'strict_r2/historical/source_snapshot/'
    domain = [name[len(prefix):] for name in mapping if name.startswith(prefix)]
    for name in domain:
        a, b = prefix + name, 'strict_r2/corrected/source_snapshot/' + name
        wanted = raw(a).decode()
        if name in delta:
            need(mapping[a]['sha256'] == delta[name]['before'] and mapping[b]['sha256'] == delta[name]['after'],
                 'annotation_source_pins')
            for declaration in declarations:
                if '\n' + declaration in wanted:
                    need(wanted.count('\n' + declaration) == 1, 'unique_declaration')
                    wanted = wanted.replace('\n' + declaration, '\nMHGP7_HD ' + declaration)
                    count += 1
        need(raw(b).decode() == wanted, 'only_four_annotation_changes')
    need(count == 4, 'four_annotations')
    failed = read('strict_r1/receipt.json')
    need(failed['status'] == 'failed' and failed['stable'] is True and
         failed['error'] == 'RuntimeError: negative_cross_space_cause' and failed['commands'] == 4,
         'historical_reader_failure_preserved')
    failed_host = read('host_o2_r1/receipt.json')
    need(failed_host['status'] == 'failed' and failed_host['stable'] is True and
         failed_host['error'] == 'RuntimeError: empty_success_stderr' and failed_host['commands'] == 9,
         'historical_stderr_harness_failure_preserved')
    host_seq = ['compiler_version', 'batch_dependencies', 'batch_compile', 'selftest',
                'batch_unknown', 'batch_missing', 'T2_dependencies', 'T2_compile',
                'line12', 'shell14', 'spatial12', 'T2_unknown', 'T2_missing']
    codes = [0, 0, 0, 0, 2, 2, 0, 0, 0, 0, 0, 2, 2]
    for capture, mode in (('host_o2_r2', 'o2'), ('host_san_root_r1', 'san')):
        receipt = stable(capture)
        need(receipt['mode'] == mode and receipt['commands'] == 13 and receipt['annotation_variant'] is True and
             receipt['new_host_execution'] is True and receipt['inherited_results'] is False and
             receipt['device_executed'] is False and receipt['GCP_used'] is False, 'new_host_scope')
        host_rows = commands(capture, host_seq, codes)
        for name in ('batch_compile', 'T2_compile'):
            argv = host_rows[name]['argv']
            need(all(flag in argv for flag in ('-Wall', '-Wextra', '-Wpedantic', '-Werror', '-DMHGP7_FAKE_DEVICE')),
                 'strict_host_flags')
            if mode == 'san':
                need('-fsanitize=address,undefined' in argv, 'SAN_flags')
        if mode == 'san':
            need(receipt['sanitizer_environment'] == dict(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
                 UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1'), 'LSan_not_disabled')
        im = capture + '/source_snapshot/MANIFEST.json'
        need(receipt['input_manifest_sha256'] == mapping[im]['sha256'], 'host_input_bound')
        for name, pin in read(im)['files'].items():
            need(mapping[capture + '/source_snapshot/' + name]['sha256'] == pin, 'host_source_snapshot_pin')
        for fixture in ('selftest', 'line12', 'shell14', 'spatial12'):
            for suffix in ('stdout', 'stderr'):
                need(raw(capture + '/' + fixture + '.' + suffix) == raw(capture + '/source_snapshot/reference_' +
                     fixture + '.' + suffix), 'new_execution_equals_reference')
                need(raw(capture + '/' + fixture + '.' + suffix) == raw('host_o2_r2/' + fixture + '.' + suffix),
                     'paired_O2_SAN_outputs')
    full = read('host_o2_r2/selftest.stdout')
    need(full['checks'] == 28044 and full['direct_terminals'] == 176 and full['seed_hits'] == 8 and
         full['transaction_rejections'] == 41 and full['synthetic_aggregate_rejections'] == 1,
         'FULL_nonvacuity')
    t2 = [json.loads(line) for fixture in ('line12', 'shell14', 'spatial12')
          for line in raw('host_o2_r2/' + fixture + '.stdout').decode().splitlines()]
    need(len(t2) == 9 and all(row['status'] == 'passed' for row in t2), 'T2_nine_verdicts')
    for scope, expected_count in (('real_census_seeded_batch_T2', 1872),
         ('separate_all_highK_subsets_upper_cut', 3575)):
        need(sum(k['direct_terminals'] for row in t2 if row['scope'] == scope
             for k in row['per_k']) == expected_count, 'direct_BallId_scope_floor')
    print(json.dumps(dict(status='passed_portable_HD_compile_and_host', ABI_fields=63,
        strict_commands=15, host_commands_per_mode=13, old_warnings=6, old_compile_exit=1,
        corrected_compile_exit=0, corrected_warnings=0, source_annotation_changes=4,
        direct_T2_terminals=1872, separate_highK_terminals=3575,
        ELF_and_dependencies_before_after_attested=True, omitted_ELF_live_bytes_rechecked=False,
        device_executed=False, GCP_used=False, public_status='not_claimed'), sort_keys=True))


if __name__ == '__main__':
    main()
