"""Fresh bounded C++ qualification; writes only in this audit packet.

Compile the transport bridge and one private parent-array mutant. The old
constructor gate is also executed on that mutant to test the reported blind
spot. Geometry and expectations are supplied by the independent corpus.
"""
from __future__ import annotations

import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SOURCE = ROOT / 'morsehgp3D_v7'
WORK = HERE / '.work_build'
ABSENT = (1 << 64) - 1


def need(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def dump(path: Path, data: object) -> None:
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + '\n')


def compressed(path: Path, data: bytes) -> None:
    path.write_bytes(gzip.compress(data, mtime=0))


def run(name: str, args: list[str], stdin: bytes | None = None,
        environment: dict[str, str] | None = None, expected: int = 0) -> subprocess.CompletedProcess:
    destination = HERE / (name + '.command.json')
    need(not destination.exists(), 'retain_previous_attempt:' + name)
    row = dict(name=name, args=args, cwd=str(ROOT), expected_exit=expected,
               started_unix=time.time(), status='running', environment=environment or {})
    dump(destination, row)
    env = os.environ.copy()
    env.update(environment or {})
    try:
        result = subprocess.run(args, input=stdin, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, cwd=ROOT, env=env, timeout=180)
    except Exception as error:
        row.update(status='failed', error=repr(error), finished_unix=time.time())
        dump(destination, row)
        raise
    compressed(HERE / (name + '.stdout.gz'), result.stdout)
    (HERE / (name + '.stderr')).write_bytes(result.stderr)
    row.update(status='completed' if result.returncode == expected else 'failed',
               exit_code=result.returncode, finished_unix=time.time(),
               stdout_sha256=hashlib.sha256(result.stdout).hexdigest(),
               stderr_sha256=hashlib.sha256(result.stderr).hexdigest(),
               stdin_sha256=hashlib.sha256(stdin).hexdigest() if stdin is not None else None)
    dump(destination, row)
    need(result.returncode == expected, 'unexpected_exit:' + name + ':' + str(result.returncode))
    return result


def build() -> None:
    need(not WORK.exists(), 'fresh_build_directory_required')
    WORK.mkdir()
    compiler = '/usr/bin/g++'
    affinity = ['taskset', '-c', '1']
    flags = ['-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror']
    includes = ['-I', str(SOURCE / 'src')]
    bridge = HERE / 'bridge.cpp'
    depfile = WORK / 'project.d'
    run('dependencies', affinity + [compiler, *flags, *includes,
                                    '-MM', '-MF', str(depfile), str(bridge)])
    dependencies = shlex.split(depfile.read_text().replace('\\\n', ' ').split(':', 1)[1])
    project = {str(Path(name).resolve().relative_to(ROOT)): sha(Path(name))
               for name in dependencies if Path(name).resolve().is_relative_to(ROOT)}
    need(str(bridge.relative_to(ROOT)) in project and
         'morsehgp3D_v7/src/forest/full_coverage_certificate.hpp' in project,
         'actual_project_dependency_closure')
    (HERE / 'project.d').write_bytes(depfile.read_bytes())
    dump(HERE / 'project_sources_before.json', project)
    run('compiler_version', [compiler, '--version'])
    context = dict(git_head=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                   git_status=subprocess.check_output(['git', 'status', '--short'], cwd=ROOT, text=True),
                   cpu=1, compiler_sha256=sha(Path(compiler)), build_dir=str(WORK),
                   public_status='not_claimed', gcp_used=False,
                   external_headers='System headers not archived or pinned before compilation; compiler identity recorded.')
    dump(HERE / 'context.json', context)
    run('compile_o2', affinity + [compiler, *flags, '-O2', *includes, str(bridge), '-o', str(WORK / 'o2')])
    run('compile_san', affinity + [compiler, *flags, '-O1', '-g', '-fno-omit-frame-pointer',
                                 '-fsanitize=address,undefined', *includes, str(bridge), '-o', str(WORK / 'san')])
    mutant = WORK / 'mutant' / 'forest'
    mutant.mkdir(parents=True)
    source = SOURCE / 'src/forest/full_coverage_certificate.hpp'
    original = source.read_text()
    before = 'out.parents_.push_back(parent);'
    after = 'out.parents_.push_back(0);'
    need(original.count(before) == 1, 'one_parent_storage_mutation')
    mutated = original.replace(before, after)
    (mutant / source.name).write_text(mutated)
    # Retain the exact private mutant source as evidence, separate from product.
    (HERE / 'mutant_parent_array.hpp').write_text(mutated)
    mutation_includes = ['-I', str(mutant.parent), *includes, '-I', str(SOURCE / 'src/forest')]
    run('compile_parent_mutant', affinity + [compiler, *flags, '-O2', *mutation_includes,
                                           str(bridge), '-o', str(WORK / 'parent_mutant')])
    gate_source = SOURCE / 'tests/full_coverage_certificate_gate.cpp'
    gate = gate_source.read_text()
    edits = {'#include "../src/forest/full_coverage_certificate.hpp"':
             '#include "forest/full_coverage_certificate.hpp"',
             '#include "../oracle/local_plateau_oracle.hpp"':
             '#include "oracle/local_plateau_oracle.hpp"'}
    for before_include, after_include in edits.items():
        need(gate.count(before_include) == 1, 'one_gate_include_adaptation')
        gate = gate.replace(before_include, after_include)
    adapted = WORK / 'constructor_gate.cpp'
    adapted.write_text(gate)
    (HERE / 'constructor_gate_adapter.cpp').write_text(gate)
    boost = ROOT / 'build/v7_boost_gate/extracted/usr/include'
    need((boost / 'boost/multiprecision/cpp_int.hpp').is_file(), 'existing_boost_headers')
    gate_includes = [*mutation_includes, '-I', str(SOURCE), '-isystem', str(boost)]
    gate_depfile = WORK / 'constructor_gate.d'
    run('constructor_gate_dependencies', affinity + [compiler, *flags, *gate_includes,
        '-MM', '-MF', str(gate_depfile), str(adapted)])
    gate_dependencies = shlex.split(gate_depfile.read_text().replace('\\\n', ' ').split(':', 1)[1])
    gate_pins = {str(Path(name).resolve().relative_to(ROOT)): sha(Path(name))
                 for name in gate_dependencies if Path(name).resolve().is_relative_to(ROOT)}
    gate_pins[str(gate_source.relative_to(ROOT))] = sha(gate_source)
    dump(HERE / 'constructor_gate_sources_before.json', gate_pins)
    run('compile_constructor_gate_mutant', affinity + [compiler, *flags, '-O2',
        *gate_includes, str(adapted), '-o', str(WORK / 'constructor_gate_mutant')])
    witness = run('constructor_gate_mutant', affinity + [str(WORK / 'constructor_gate_mutant'), '--selftest'])
    expected = b'full_coverage_certificate checks=710 rejects=30 replay_cuts=30 gamma_cuts=34 allocation_rejects=34 authority=structural_only\n'
    need(witness.stdout == expected and not witness.stderr, 'constructor_gate_does_not_detect_wrong_parent_storage')
    need(all(sha(ROOT / name) == pin for name, pin in gate_pins.items()),
         'constructor_gate_dependencies_stable')
    after_pins = {name: sha(ROOT / name) for name in project}
    need(after_pins == project, 'project_sources_stable_during_compilation')
    dump(HERE / 'project_sources_after_build.json', after_pins)
    dump(HERE / 'build.json', dict(status='passed', binaries={name:sha(WORK / name) for name in
        ('o2','san','parent_mutant','constructor_gate_mutant')}, constructor_gate_source_sha256=sha(gate_source),
        private_gate_adaptation='Only two include paths changed to select the isolated parent-array mutant.',
        mutation='out.parents_.push_back(parent) -> out.parents_.push_back(0)',
        nominal_source_sha256=sha(source), mutant_sha256=sha(HERE/'mutant_parent_array.hpp')))
    print('Fresh O2/SAN and mutant builds closed; original gate misses the parent-array mutant.', flush=True)


def wire(cases: list[dict]) -> bytes:
    lines = [str(len(cases))]
    for case in cases:
        domain, rows, batches, queries = (case[key] for key in ('domain','rows','batches','queries'))
        lines.append(f"{len(domain)} {len(rows)} {case['order']} {len(batches)} {len(queries)}")
        lines.append(' '.join(map(str, domain)))
        for row in rows:
            lines.append(' '.join(map(str, [len(row['interior']), len(row['shell']),
                                           *row['interior'], *row['shell']])))
        for batch in batches:
            lines.append(' '.join(map(str, [*batch['level'], len(batch['actions'])])))
            for action in batch['actions']:
                parts = [len(action['parents']), *action['parents'], len(action['refs'])]
                for pop, mask, include in action['refs']:
                    parts.extend([pop, mask, int(include)])
                lines.append(' '.join(map(str, parts)))
        for query in queries:
            lines.append(' '.join(map(str, [*query['level'], int(query['closed'])])))
    return ('\n'.join(lines) + '\n').encode()


def differences(cases: list[dict], output: bytes) -> tuple[list[dict], list[dict]]:
    actual = [json.loads(line) for line in output.splitlines()]
    need(len(actual) == len(cases), 'complete_case_output')
    failures = []
    for index, (case, row) in enumerate(zip(cases, actual)):
        for field, expected in case['expected'].items():
            if row.get(field) != expected:
                failures.append(dict(case=index, name=case['name'], field=field))
        for field in ('ownership_ok','moved_construct_source_unreadable',
                      'moved_assign_source_unreadable','bank_shared_ok'):
            if row.get(field) is not True:
                failures.append(dict(case=index, name=case['name'], field=field))
    return failures, actual


def test(attempt: str) -> None:
    from corpus import build_corpus
    cases, stats = build_corpus()
    need(bool(cases), 'nonempty_corpus')
    source_pins = json.loads((HERE / 'project_sources_before.json').read_text())
    need(all(sha(ROOT / name) == pin for name, pin in source_pins.items()), 'compiled_sources_still_current')
    encoded = wire(cases)
    compressed(HERE / (attempt + '_input.txt.gz'), encoded)
    compressed(HERE / (attempt + '_corpus.json.gz'), json.dumps(cases, sort_keys=True, separators=(',',':')).encode())
    outputs = {}
    for name in ('o2','san','parent_mutant'):
        env = {'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1',
               'UBSAN_OPTIONS':'halt_on_error=1:print_stacktrace=1'} if name=='san' else {}
        result = run(attempt + '_' + name, ['taskset','-c','1',str(WORK/name)], encoded, env)
        need(not result.stderr, 'empty_execution_stderr:' + name)
        failures, actual = differences(cases, result.stdout)
        dump(HERE/(attempt+'_'+name+'_judgment.json'), dict(cases=len(cases), failures=failures))
        if name != 'parent_mutant':
            need(not failures, 'independent_comparison_failed:' + name + ':' + repr(failures[:4]))
        else:
            need(bool(failures) and {item['field'] for item in failures} == {'parents'},
                 'isolated_parent_array_mutant_is_detected_only_by_new_structural_judge')
        outputs[name] = (result.stdout, actual, failures)
    need(outputs['o2'][0] == outputs['san'][0], 'o2_san_identical_outputs')
    need(all(a['queries'] == b['queries'] for a,b in zip(outputs['o2'][1],outputs['parent_mutant'][1])),
         'corrupt_parent_array_is_invisible_to_the_existing_readers')
    need(all(sha(ROOT / name) == pin for name, pin in source_pins.items()), 'sources_unchanged_after_execution')
    result = dict(status='passed', cases=len(cases), corpus=stats, output_sha256=hashlib.sha256(outputs['o2'][0]).hexdigest(),
                  mutant_failures=len(outputs['parent_mutant'][2]), mutant_fields=['parents'],
                  nominal_and_sanitized_identical=True, constructor_gate_passes_same_mutant=True,
                  source_sha256={'runner.py':sha(Path(__file__)),'corpus.py':sha(HERE/'corpus.py'),
                                 'bridge.cpp':sha(HERE/'bridge.cpp')},
                  engine_executed=False, global_producer_qualified=False, gcp_used=False,
                  public_status='not_claimed')
    dump(HERE/(attempt+'_qualification.json'),result)
    print(json.dumps(result, sort_keys=True), flush=True)


def main() -> None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('phase', choices=['build','test'])
    parser.add_argument('--attempt',default='r1')
    args=parser.parse_args()
    if args.phase=='build': build()
    else:
        need(args.attempt.isalnum(), 'alphanumeric_attempt')
        test(args.attempt)


if __name__=='__main__': main()
