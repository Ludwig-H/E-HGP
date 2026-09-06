#!/usr/bin/env python3
"""Portable, read-only verification of one closed capture; never reads ELF files.

Usage: python3 -B verify_mutations.py CAPTURE_DIRECTORY
Boost pins are checked against depfiles; system libraries are not certified.
"""
import hashlib
import json
from pathlib import Path, PurePosixPath
import posixpath
import shlex
import sys
from datetime import datetime

RUN_SHA = 'dbbe577e7ca392b680844c098b5a4fb122122ea2ccb10cd3629694af4c71044f'
P = 'morsehgp3D_v7'
H, F, FULL = (P + '/src/forest/' + n + '.hpp' for n in ('meb_proposal', 'silent_incidence', 'full_gabriel'))
DRIVER = 'build/v7_meb_product_mutation_qualification_20260906/record.py'
FAULT = 'build/v7_meb_product_fault_20260906'
PINS = {H: 'f922544b5cfdc214de96ecd49520e318ea8632d14a8142ef21fd248f9cc38fb3',
        F: 'f75a136a320ddd1ace025436874585c2226e6150b8f2ebc37920b8dfc7e36c76',
        FULL: 'a946e31dde8fbd8ec528d6f5e94f9c727998acc172b4dd29c084dd522c730d1d',
        DRIVER: 'd0975cdd3eb8b33567848b148ddcaf52952b41521939f09dba4682a9741c1cc4'}
CAUSES = {'charge_after': 'observer.prospective_P_charge',
          'drop_A': 'physical_F.only_real_fallback_supports_not_virtual_ordinal',
          'reset_Work': 'P=0 retains only F work with A=c',
          'drop_FULL_P_mirror': 'P=0 retains only F work with A=c'}
INTENT = 'argv cpu cpu_seconds cwd executable_sha256 expected_exit_code file_size_limit_bytes process_wall_seconds started_utc stdin wall_wait_seconds'.split()


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def encode(value):
    return (json.dumps(value, sort_keys=True, indent=2) + '\n').encode()


def parse(raw):
    def pairs(items):
        out = {}
        for key, value in items:
            need(key not in out, 'json.duplicate_key')
            out[key] = value
        return out
    return json.loads(raw, object_pairs_hook=pairs, parse_constant=lambda _: need(False, 'json.nonfinite'))


def verify(directory):
    base = Path(directory).resolve()
    observed = {}
    def read(name):
        parts = PurePosixPath(name).parts
        need(parts and not PurePosixPath(name).is_absolute() and '..' not in parts, 'path.unsafe')
        path = base.joinpath(*parts)
        need(all(not base.joinpath(*parts[:i]).is_symlink() for i in range(1, len(parts)+1)), 'path.symlink')
        raw = path.read_bytes()
        need(name not in observed or observed[name] == sha(raw), 'read.changed:' + name)
        observed[name] = sha(raw)
        return raw
    def obj(name):
        return parse(read(name))
    def tree(prefix, expected):
        paths = list((base / prefix).rglob('*'))
        need(all(not p.is_symlink() for p in paths), 'tree.symlink')
        need({str(p.relative_to(base / prefix)) for p in paths if p.is_file()} == set(expected), 'tree.inventory')
        actual = {str(p.relative_to(base / prefix)): sha(read(str(p.relative_to(base)))) for p in paths if p.is_file()}
        need(actual == expected, 'tree.bytes:' + prefix)
    need(sha(read('run.json')) == RUN_SHA, 'run.external_pin')
    r = obj('run.json')
    need(r['schema'] == 'mhgp7-product-meb-mutation-run-v1' and r['status'] == 'completed' and
         r['sources_stable'] is True and r['F_unchanged'] is True and r['nominal_product_mutated'] is False and
         r['public_status'] == 'not_claimed' and r['gcp'] == 'not_used', 'run.scope')
    need(r['caps'] == dict(campaign_wall_seconds=1800, compiler_file_bytes=512 << 20,
         cpu_seconds=120, execution_file_bytes=64 << 20, process_wall_seconds=300) and
         0 <= r['elapsed_seconds'] <= 1800, 'run.bounds')
    before = read('inputs_before.json')
    need(before == read('inputs_after.json') and sha(before) == r['source_map_sha256'], 'inputs.binding')
    pins = parse(before)
    need(all(pins['sources'][n] == p for n, p in PINS.items()), 'original.authorities')
    tree('snapshot', pins['sources'])
    old = str(PurePosixPath(r['commands']['compiler_version']['cwd']).parent)
    compiler = pins['compiler_path']
    boost = '/workspaces/E-HGP/build/v7_boost_gate/extracted/usr/include'
    env = obj('environment.json')
    need(env['ASAN_OPTIONS'] == 'detect_leaks=1:halt_on_error=1' and env['LSAN_OPTIONS'] == 'exitcode=23' and
         env['UBSAN_OPTIONS'] == 'halt_on_error=1:print_stacktrace=1', 'sanitizers.enabled')
    need(obj('host.json')['recorder_sha256'] == PINS[DRIVER], 'host.driver')
    variants = {}
    need(set(r['variants']) == set(CAUSES) | {'form_fault'} and set(r['mutations']) == set(CAUSES), 'variants.inventory')
    for label, record in r['variants'].items():
        patch = obj('variants/' + label + '.json')
        target = FULL if label in ('reset_Work', 'drop_FULL_P_mirror') else H
        raw = read('snapshot/' + target)
        need(patch['target'] == target and raw.decode().count(patch['old']) == 1 and patch['old'] != patch['new'], 'mutation.unique')
        changed = raw.decode().replace(patch['old'], patch['new']).encode()
        expected = dict(pins['sources']); expected[target] = sha(changed)
        need(patch['before_sha256'] == sha(raw) and patch['after_sha256'] == sha(changed) and
             patch['source_pins'] == expected and sha(encode(expected)) == record['source_map_sha256'] and
             record['root'] == 'variants/' + label and expected[F] == PINS[F], 'mutation.recomputed')
        tree(record['root'], expected)
        variants[label] = expected
    labels = list(CAUSES) + ['form_fault_o2', 'form_fault_san']
    need(set(r['binaries']) == set(labels), 'binaries.inventory')
    plan = {'compiler_version': ([compiler, '--version'], old + '/snapshot', 0, pins['compiler_sha256'])}
    for label in labels:
        fault = label.startswith('form_fault_')
        variant = 'form_fault' if fault else label
        root = old + '/variants/' + variant
        gate = FAULT + '/full_fault_gate.cpp' if fault else P + '/tests/' + (
            'meb_proposal_local_gate.cpp' if label in ('charge_after', 'drop_A') else 'full_gabriel_meb_gate.cpp')
        binary = old + '/bin/' + label
        pin = r['binaries'][label]
        need(pin['path'] == 'bin/' + label and len(pin['sha256']) == 64 and
             all(c in '0123456789abcdef' for c in pin['sha256']), 'binary.recorded_pin')
        flags = ['-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                 '-fno-omit-frame-pointer', '-fno-pie', '-no-pie'] if label == 'form_fault_san' else ['-O2']
        argv = [compiler, '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread', *flags,
                '-I', root + '/' + P, '-I', boost, '-MMD', '-MF', old + '/dependencies/' + label + '.d']
        argv += ['-include', root + '/' + FAULT + '/fault_hook.hpp'] if fault else []
        plan[label + '_compile'] = (argv + [root + '/' + gate, '-o', binary], root, 0, pins['compiler_sha256'])
        for suffix, arg, code in ([('selftest', '--selftest', 0), ('unknown', '--unknown', 2)] if fault else
                                  [('run', '--rejects' if label == 'charge_after' else '--selftest', 1)]):
            plan[label + '_' + suffix] = ([binary, arg], root, code, pin['sha256'])
        deps = obj('dependencies/' + label + '.json')
        target, body = read('dependencies/' + label + '.d').decode().replace('\\\n', ' ').split(':', 1)
        need(shlex.split(target) == [binary], 'depfile.target')
        mapped = {}
        for name in shlex.split(body):
            name = posixpath.normpath(name)
            if name.startswith(root + '/'):
                key = name[len(root)+1:]; mapped[key] = variants[variant][key]
            else:
                need(name.startswith(boost + '/'), 'depfile.outside_authority')
                key = name[len(boost)+1:]; mapped['BOOST/' + key] = pins['boost_headers'][key]
        required = {gate, H, F} | ({FULL} if gate != P + '/tests/meb_proposal_local_gate.cpp' else set())
        need(deps == mapped and required <= set(deps) and (not fault or FAULT + '/fault_hook.hpp' in deps), 'dependencies.binding')
    need(set(r['commands']) == set(plan) and len(plan) == 15, 'commands.inventory')
    for label, (argv, cwd, code, pin) in plan.items():
        row = r['commands'][label]; prefix = 'commands/' + label
        need(encode(obj(prefix + '.json')) == encode(row), 'command.mirror')
        intent = obj(prefix + '.intent.json'); spawn = obj(prefix + '.spawn.json')
        need(set(intent) == set(INTENT) and encode(intent) == encode({k: row[k] for k in INTENT}), 'intent.binding')
        need(set(spawn) == {'pid', 'pgid', 'utc'} and all(type(spawn[k]) is int for k in ('pid', 'pgid')) and
             type(row['pid']) is int and row['pid'] > 0 and
             spawn['pid'] == spawn['pgid'] == row['pid'] == row['pgid'] and
             datetime.fromisoformat(row['started_utc']) <= datetime.fromisoformat(spawn['utc']) <= datetime.fromisoformat(row['ended_utc']), 'spawn.binding')
        need(row['argv'] == argv and row['cwd'] == cwd and row['executable_sha256'] == pin and
             type(row['exit_code']) is int and row['exit_code'] == row['expected_exit_code'] == code and
             row['error'] is None and row['process_group_closed'] is True and 0 <= row['elapsed_seconds'] <= 300 and
             row['cpu_seconds'] == 120 and row['process_wall_seconds'] == 300 and
             row['file_size_limit_bytes'] == (512 if argv[0] == compiler else 64) << 20, 'command.terminal')
        for suffix in ('stdout', 'stderr'):
            raw = read(prefix + '.' + suffix)
            need(sha(raw) == row[suffix + '_sha256'] and len(raw) == row[suffix + '_bytes'], 'stream.binding')
        if code != 1:
            need(read(prefix + '.stderr') == b'', 'command.unexpected_stderr')
        if label.endswith('_unknown'):
            need(read(prefix + '.stdout') == b'', 'unknown.stdout')
    for label, cause in CAUSES.items():
        first = read('commands/' + label + '_run.stderr').decode().splitlines()[0]
        need((first == 'meb proposal local rejected: ' + cause) if label in ('charge_after', 'drop_A') else
             (first.startswith('FAIL [') and first.split('] ', 1)[1] == cause), 'mutant.first_cause')
        need(r['mutations'][label] == dict(cause=cause, exit_code=1, first_diagnostic=first,
             private_only=True, status='rejected_as_expected'), 'mutant.summary')
    fixed = dict(cases=12, public_refusals=4, runtime_propagations=2, builder_propagations=6,
                 mirrors=10, compared_mirrors=8, baselines=2, retries=6, failures=0, paid_at_throw=36, checks=9091)
    fault = dict(fixed, schema='mhgp7-private-full-meb-form-fault-v1', status='passed', public_status='not_claimed',
                 nominal_noobserver_exception_claim=False, F_exception_coverage='not_exercised')
    for mode in ('o2', 'san'):
        rows = read('commands/form_fault_' + mode + '_selftest.stdout').splitlines()
        need(len(rows) == 1 and encode(parse(rows[0])) == encode(fault) == encode(r['fault'][mode]), 'fault.exact_counts_and_scope')
    for name in list(observed):
        read(name)
    return dict(schema='mhgp7-product-meb-mutations-verification-v1', status='passed', public_status='not_claimed',
                run_sha256=RUN_SHA, commands=15, recorded_binary_pins_not_reread=6, mutants=4,
                fault_cases_per_mode=12, files_verified=len(observed), engine_invoked=False, ELF_read=False)


if __name__ == '__main__':
    need(len(sys.argv) == 2, 'usage: verify_mutations.py CAPTURE_DIRECTORY')
    print(json.dumps(verify(sys.argv[1]), sort_keys=True))
