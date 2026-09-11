#!/usr/bin/env python3
"""Portable read-only O2 worker-failure proof, with explicit failed SAN."""
import hashlib
import json
from pathlib import Path
import sys

BASE = Path(__file__).resolve().parent

def need(ok, why):
    if not ok:
        raise ValueError(why)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def path(name):
    p = Path(name)
    need(not p.is_absolute() and '..' not in p.parts and (BASE/p).resolve().is_relative_to(BASE), 'relative_path')
    return BASE / p

def main():
    need(len(sys.argv) == 1, 'no_arguments')
    manifest = json.loads((BASE/'MANIFEST.json').read_text())
    found = {p.relative_to(BASE).as_posix() for p in BASE.rglob('*') if p.is_file() and p.name != 'MANIFEST.json'}
    need(found == set(manifest['files']), 'physical_file_set')
    for name, digest in manifest['files'].items():
        data = path(name).read_bytes()
        need(sha(data) == digest and not data.startswith(b'\x7fELF'), 'file_hash_no_ELF:' + name)
    mapping = json.loads((BASE/'storage_map.json').read_text())
    def raw(name):
        row = mapping[name]; data = path(row['physical']).read_bytes()
        need(sha(data) == row['sha256'], 'logical_hash:' + name)
        return data
    def js(name):
        return json.loads(raw(name))
    for name in mapping:
        raw(name)
    header = 'morsehgp3D_v7/src/forest/full_ball_tower.hpp'
    original = raw('original_header.hpp.source')
    need(sha(original) == '33e7d05effce908532d21255e5e0efc4f8c7370d8a05a17dd761142d81bd4209', 'prototype_r2')
    needle = b'scratch.clear(); scratch.push_back(ix.root());'
    hook = b'scratch.clear(); ::scratch_fault::arm(work.resolve_work.calls, scratch.capacity()); scratch.push_back(ix.root());'
    need(original.count(needle) == 1, 'one_private_hook_site')
    instrumented = raw('o2/source/' + header)
    need(instrumented == original.replace(needle, hook), 'only_test_hook_delta')
    need(js('o2/receipt.json')['status'] == 'passed', 'O2_passed')
    before = js('o2/sources_before.json'); after = js('o2/sources_after.json')
    need(before == after == js('san/sources_before.json'), 'O2_stable_SAN_same_sources')
    for name, digest in before.items():
        need(sha(raw('o2/source/' + name)) == digest and sha(raw('san/source/' + name)) == digest, 'source_pin')
    o2 = js('o2/commands.json'); san = js('san/commands.json')
    need(len(o2) == 4 and len(san) == 2, 'six_closed_commands')
    for prefix, rows in [('o2', o2), ('san', san)]:
        for row in rows:
            need(row['ended_ns'] >= row['started_ns'], 'command_closed')
            for stream in ('stdout', 'stderr'):
                need(sha(raw(prefix + '/' + row['name'] + '.' + stream)) == row[stream + '_sha256'], 'stream_pin')
    need([r['exit_code'] for r in o2] == [0,0,0,1], 'O2_actual_exits')
    outputs = [json.loads(line) for line in raw('o2/post_admission_and_reuse.stdout').splitlines()]
    need(len(outputs) == 2 and outputs[0]['status'] == 'passed_post_admission_failure' and
         outputs[0]['failed_runs'] == 2 and outputs[0]['retained_MEB_calls'] >= 2, 'paid_after_admission_faults')
    need(outputs[1]['status'] == 'passed' and outputs[1]['rows'] == 2524 and outputs[1]['vertical'] == 1506,
         'nominal_reuse_Gamma')
    need(not raw('o2/post_admission_and_reuse.stderr').strip(), 'O2_clean_stderr')
    need(raw('o2/drop_paid_work.stderr').strip() == b'paid_worker_work_retained_after_failure', 'causal_mutant_reason')
    mb = js('o2/mutant_before.json'); ma = js('o2/mutant_after.json')
    need(mb == ma and set(mb) == set(before), 'mutant_source_stable')
    need({p for p in mb if mb[p] != before[p]} == {header}, 'one_mutant_file')
    for name, digest in mb.items():
        need(sha(raw('o2/mutant_source/' + name)) == digest, 'mutant_pin')
    need([r['exit_code'] for r in san] == [0,1] and js('san/receipt.json')['status'] == 'failed', 'SAN_failed_not_promoted')
    need('-fsanitize=address,undefined' in san[0]['argv'], 'SAN_compile_flag')
    need(b'LeakSanitizer has encountered a fatal error' in raw('san/post_admission_and_reuse.stderr') and
         b'ptrace' in raw('san/post_admission_and_reuse.stderr'), 'preserved_ptrace_failure')
    need('san/sources_after.json' not in mapping and not any(n.startswith('san_replay/') for n in mapping),
         'no_fabricated_SAN_after_or_replay')
    need(sha(raw('o2/expected.txt')) == '578a38d8d52c610bc5fed0a834ab06a46aee351e43927e0c92eb101738f5f432', 'independent_expected_pin')
    print(json.dumps(dict(status='verified_O2_post_admission_failure_with_unqualified_SAN', commands=6,
        O2_fault_runs=2, causal_mutants=1, nominal_Gamma_rows=2524, SAN_status='failed_LSan_ptrace',
        logical_files=len(mapping), public_status='not_claimed', GCP_used=False)))

if __name__ == '__main__':
    main()
