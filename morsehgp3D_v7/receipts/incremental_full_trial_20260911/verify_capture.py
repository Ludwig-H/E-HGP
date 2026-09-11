#!/usr/bin/env python3
"""Read-only closed evidence; no engine/compiler invocation, valid under -O."""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
BASELINE = '33e7d05effce908532d21255e5e0efc4f8c7370d8a05a17dd761142d81bd4209'
CANDIDATE = 'a2c6ab390540fd57d2e593d6d78097f0cee477810e1e3b0dcd35fe4c3afbbd50'
PROTOTYPE = {
    'full_coverage_certificate.hpp': 'b526b895238d454c7b5df6b8d40a7af8682551e71fb2582bcc8e016f0018650b',
    'full_coverage_incremental.hpp': '76885ecd317eaa6a4277a3545e18efd739209ca53cdf9f7c08f9007b33b57a12'}

def need(ok: bool, reason: str) -> None:
    if not ok: raise RuntimeError(reason)

def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

def js(path: Path):
    return json.loads(path.read_text())

def capture(name: str, expected_status: str, commands: int):
    folder = HERE / name
    receipt = js(folder / 'receipt.json')
    need(receipt['status'] == expected_status and receipt['sources_stable'] and
         receipt['public_status'] == 'not_claimed' and receipt['GCP_used'] is False, name + ':status')
    before, after = js(folder / 'sources_before.json'), js(folder / 'sources_after.json')
    actual = {p.relative_to(folder / 'source').as_posix(): sha(p)
              for p in sorted((folder / 'source').rglob('*')) if p.is_file()}
    need(before == after == actual, name + ':source_closure')
    calls = js(folder / 'commands.json')
    need(len(calls) == commands == receipt['commands'], name + ':command_count')
    for call in calls:
        need(call['ended_ns'] >= call['started_ns'], name + ':closed_command')
        for stream in ('stdout', 'stderr'):
            need(sha(folder / (call['name'] + '.' + stream)) == call[stream + '_sha256'], name + ':raw_' + stream)
        if call['name'].startswith('compile'):
            for flag in ('-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread'):
                need(flag in call['argv'], name + ':strict_compile:' + flag)
            need(call['exit_code'] == 0, name + ':compile_success')
        if receipt.get('sanitizer'):
            need(call['sanitizer_environment'] == {
                'ASAN_OPTIONS': 'detect_leaks=1:halt_on_error=1',
                'UBSAN_OPTIONS': 'halt_on_error=1:print_stacktrace=1'}, name + ':SAN_options_unchanged')
            if call['name'].startswith('compile'):
                need('-fsanitize=address,undefined' in call['argv'], name + ':SAN_compilation')
        if expected_status == 'passed':
            expected = 2 if 'arguments' in call['name'] else 0
            need(call['exit_code'] == expected, name + ':expected_exit')
    return folder

def main() -> None:
    need(len(sys.argv) == 1, 'no_arguments')
    for kind, digest in (('baseline', BASELINE), ('candidate', CANDIDATE)):
        need(sha(HERE / kind / 'morsehgp3D_v7/src/forest/full_ball_tower.hpp') == digest, 'pure_header:' + kind)
    for name, digest in PROTOTYPE.items():
        need(sha(HERE / 'candidate/morsehgp3D_v7/src/forest' / name) == digest, 'prototype_unchanged:' + name)
    o2 = capture('o2_r1', 'passed', 12)
    san = capture('san_root_r2', 'passed', 12)
    physical_totals = dict(calls=0, complete=0, rejected=0, nodes=0, parents=0, contributions=0)
    for mode in ('cache', 'no_cache', 'static1', 'static4'):
        for suffix in ('physical', 'stdout', 'stderr'):
            streams = [(folder / (kind + '_' + mode + '.' + suffix)).read_bytes()
                       for folder in (o2, san) for kind in ('baseline', 'candidate')]
            need(all(stream == streams[0] for stream in streams), 'full_physical_O2_SAN:' + mode + ':' + suffix)
        lines = (o2 / ('candidate_' + mode + '.stdout')).read_text().splitlines()
        need(len(lines) == 2, 'two_gate_results')
        oracle, physical = map(json.loads, lines)
        static = mode.startswith('static')
        need(oracle['status'] == 'passed' and oracle['clouds'] == (30 if static else 28) and
             oracle['orders'] == (124 if static else 112) and oracle['vertical_checks'] == (75136 if static else 45948) and
             oracle['growth_snapshots'] > 0 and oracle['extra_blocks'] > 0 and oracle['rejections'] == 8,
             'nonvacuous_Gram_Gamma:' + mode)
        need(physical['status'] == 'physical_recorded' and physical['complete'] == (61 if static else 29) and
             physical['rejected'] == 9 and physical['nodes'] > 700, 'nonvacuous_physical:' + mode)
        for field in physical_totals: physical_totals[field] += physical[field]
    failure_o2 = capture('failure_o2_v2', 'passed', 3)
    failure_san = capture('failure_san_root_v2', 'passed', 3)
    need((failure_o2 / 'run.stdout').read_bytes() == (failure_san / 'run.stdout').read_bytes(), 'failure_O2_SAN')
    failures = js(failure_o2 / 'run.stdout')
    need(failures == dict(status='passed', allocation_rejections=1402, after_prefix=1300, at_seal=102,
        semantic_rejections=3, poison_checks=708, modes=[0,1,4], public_status='not_claimed'), 'failure_causal_floors')
    # Historical v1 positive O2 and the two failed SAN captures are kept,
    # independently labelled; neither failed attempt becomes a positive run.
    capture('failure_o2_r1', 'passed', 3)
    ptrace = capture('san_r1', 'failed', 2)
    need('does not work under ptrace' in (ptrace / 'baseline_cache.stderr').read_text(), 'ptrace_failure_preserved')
    interposer = capture('failure_san_root_r2', 'failed', 2)
    text = (interposer / 'run.stderr').read_text()
    need('alloc-dealloc-mismatch' in text and 'nothrow_t' in text and 'failure_gate.cpp' in text,
         'interposer_v1_failure_preserved')
    micro = capture('micro_o2_r1', 'passed', 4)
    comparison = js(micro / 'comparison.json')
    need([r['n'] for r in comparison] == [200,400,800], 'micro_sizes')
    for row in comparison:
        need(row['all_matched'] and row['shared_host'] and row['speedup_claim'] is False, 'micro_scope')
        a, b = row['baseline'], row['candidate']
        for field in row['matched_fields']: need(a[field] == b[field], 'micro_physical:' + field)
        for values in (a, b):
            for field in ('new_calls', 'requested_total', 'requested_peak', 'retained', 'arene_capacity'):
                need(int(values[field]) > 0, 'micro_nonzero:' + field)
        for kind, expected in (('baseline', a), ('candidate', b)):
            lines = (micro / ('run_' + kind + '.stdout')).read_text().splitlines()
            derived = [dict(field.split('=', 1) for field in line.split()) for line in lines if line.startswith('n=')]
            need(next(r for r in derived if r['n'] == str(row['n'])) == expected, 'micro_raw_bound')
    print(json.dumps(dict(status='verified_private_incremental_full', modes=['cache','no_cache','static1','static4'],
        qualification_commands=24, physical_totals_per_kind_per_build=physical_totals, failure_gate=failures,
        micro=comparison, preserved_negative_attempts=['san_r1','failure_san_root_r2'],
        public_status='not_claimed', contract_qualified=False, GCP_used=False)))

if __name__ == '__main__': main()
