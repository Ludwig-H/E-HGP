"""Replay sealed observations against the independent oracle; never run C++."""
from __future__ import annotations

import gzip
import hashlib
import json
from pathlib import Path

from corpus import build_corpus
from runner import differences, wire

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def data(name: str) -> dict:
    return json.loads((HERE / name).read_text())


def raw(name: str) -> bytes:
    return gzip.decompress((HERE / (name + '.stdout.gz')).read_bytes())


def main() -> None:
    pins = {}
    for line in (HERE / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        path = HERE / name
        need(path.parent == HERE and name not in pins, 'unique_local_capture_path')
        need(sha(path) == digest and path.read_bytes()[:4] != b'\x7fELF', 'capture_hash:' + name)
        pins[name] = digest
    need(set(pins) == {p.name for p in HERE.iterdir() if p.is_file() and p.name != 'SHA256SUMS'},
         'complete_packet_manifest')
    refs = data('source_refs.json')
    for row in refs.values():
        path = (ROOT / row['archived_path']).resolve()
        need(path.is_relative_to(ROOT) and sha(path) == row['sha256'], 'archived_source_binding')
    before = data('project_sources_before.json')
    need(before == data('project_sources_after_build.json'), 'stable_project_closure')
    for filename in ('project_sources_before.json', 'constructor_gate_sources_before.json'):
        for name, digest in data(filename).items():
            need(refs[name]['sha256'] == digest, 'compiled_dependency_is_archived')

    commands = list(HERE.glob('*.command.json'))
    need(len(commands) == 13, 'all_commands_preserved')
    for path in commands:
        row = json.loads(path.read_text())
        observed = raw(row['name'])
        stderr = (HERE / (row['name'] + '.stderr')).read_bytes()
        need(hashlib.sha256(observed).hexdigest() == row['stdout_sha256'] and
             hashlib.sha256(stderr).hexdigest() == row['stderr_sha256'], 'command_capture_binding')
        if row['name'] == 'r1_san':
            need(row['status'] == 'failed' and row['exit_code'] == 1 and
                 b'LeakSanitizer does not work under ptrace' in stderr,
                 'failed_sanitizer_attempt_retained')
        else:
            need(row['status'] == 'completed' and row['exit_code'] == 0 and not stderr,
                 'closed_successful_command:' + row['name'])

    cases, statistics = build_corpus()
    expected_input = wire(cases)
    for attempt in ('r1', 'r2'):
        stored = gzip.decompress((HERE / (attempt + '_input.txt.gz')).read_bytes())
        need(stored == expected_input, 'same_independent_input:' + attempt)
        recorded = json.loads(gzip.decompress((HERE / (attempt + '_corpus.json.gz')).read_bytes()))
        need(recorded == cases, 'same_independent_expectations:' + attempt)
    nominal = raw('r2_o2')
    need(raw('r1_o2') == nominal == raw('r2_san'), 'closed_o2_san_identical')
    failures, actual = differences(cases, nominal)
    need(not failures, 'independent_nominal_structure_and_cut_judge')
    mutant_failures, mutant = differences(cases, raw('r2_parent_mutant'))
    need(len(mutant_failures) == 124 and {row['field'] for row in mutant_failures} == {'parents'},
         'causal_parent_array_mutant_refuted')
    need(all(a['queries'] == b['queries'] for a, b in zip(actual, mutant)),
         'corrupt_parent_array_is_invisible_to_readers')
    need(raw('constructor_gate_mutant') ==
         b'full_coverage_certificate checks=710 rejects=30 replay_cuts=30 gamma_cuts=34 allocation_rejects=34 authority=structural_only\n',
         'constructor_gate_blind_spot_reproduced')
    qualification = data('r2_qualification.json')
    need(qualification['status'] == 'passed' and qualification['cases'] == len(cases) and
         qualification['corpus'] == statistics and qualification['mutant_failures'] == len(mutant_failures),
         'qualification_summary_from_observations')
    for name, digest in qualification['source_sha256'].items():
        need(sha(HERE / name) == digest, 'qualification_driver_pin')
    square = cases.index(next(c for c in cases if c['name'] == 'square/K2/gap/factor1'))
    need(actual[square]['parents'] == [0, 1, 2, 3] and
         actual[square]['nodes'][-1][-2:] == [0, 4] and
         len(actual[square]['contributions']) == 4, 'four_parent_geometric_merge')
    print(json.dumps(dict(status='passed', mode='capture_replay_and_rational_oracle_only',
        files=len(pins), source_bindings=len(refs), cases=len(cases),
        cuts=statistics['counters']['cpp_queries'],
        root_queries=statistics['counters']['cpp_root_queries'],
        coverage_queries=statistics['counters']['cpp_coverage_queries'],
        parent_mutant_failures=len(mutant_failures),
        output_sha256=hashlib.sha256(nominal).hexdigest(),
        preserved_failed_attempts=['r1_san'], public_status='not_claimed', gcp_used=False),
        sort_keys=True))


if __name__ == '__main__':
    main()
