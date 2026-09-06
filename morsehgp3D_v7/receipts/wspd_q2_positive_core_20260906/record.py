#!/usr/bin/env python3
"""Direct O2/SAN positive-core gate plus one physical omission mutant."""
import json
import os
from pathlib import Path
import sys
import capture_common as c

c.BASE = Path(__file__).resolve().parent
c.ROOT = c.BASE.parents[1]
c.LOG = c.BASE / 'logs'
GATE = 'morsehgp3D_v7/tests/wspd_terminal_reuse_gate.cpp'
GENERATE = 'morsehgp3D_v7/src/pipeline/generate.hpp'
GATE_SHA = '35d28f2ce548976903b320b472f4c62a94b2bcf394faf11bf84507fb098d04e9'
PRIOR = c.ROOT / 'build/v7_wspd_q2_permanent_20260906_r1/logs/O2.dependencies.json'
PRIOR_SHA = '4453661b4ad0eeaedbaf294e28c22c0ddaf66aa4919fb4ff499e903978c7e710'
ENV = dict(ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
           UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1', LSAN_OPTIONS='exitcode=23')


def copy(path, raw):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('xb') as stream:
        stream.write(raw)


def main():
    c.need(c.sha(c.BASE / 'capture_common.py') ==
           '003e3e6c972d880e2848fd2fb2371e3e650e5397e273970665308903a8cf9ea3', 'capture_helper_pin')
    c.need(c.sha(PRIOR) == PRIOR_SHA, 'prior_dependency_list_pin')
    live = json.loads(PRIOR.read_text())
    live[GATE] = GATE_SHA
    c.need(len(live) == 27 and live[GENERATE] ==
           '345129a775d430a40e151d3b1adb5cd9efeaf77a6ffb6713bd081c74d40bdd9c', 'source_scope')
    c.LOG.mkdir()
    (c.BASE / 'bin').mkdir()
    for name, expected in live.items():
        c.need(c.sha(c.ROOT / name) == expected, 'live_source_pin:' + name)
        raw = (c.ROOT / name).read_bytes()
        copy(c.BASE / 'source_snapshot' / name, raw)
        copy(c.BASE / 'mutant' / name, raw)
    header = c.BASE / 'mutant' / GENERATE
    text = header.read_text()
    old = '          if (m & 0b001) ff.c[0] = fc.c[0];'
    new = '          // Private mutant: omit the q2 core value transfer.'
    c.need(text.count(old) == 1, 'unique_mutation')
    header.write_text(text.replace(old, new))  # Mechanical edit of this new private copy only.
    c.save(c.BASE / 'mutation.json', dict(target=GENERATE, old=old, new=new,
        source_sha256=live[GENERATE], mutant_sha256=c.sha(header), product_modified=False))
    before = c.sources()
    c.save(c.BASE / 'sources_before.json', before)
    result = dict(status='failed', public_status='not_claimed', binaries={},
        authority='permanent_q2_positive_core_fixture_not_geometry_completeness_or_benchmark',
        live_sources=live, sanitizer_environment=ENV, nominal_calls=174, nominal_refusals=6)
    os.environ.update(ENV)
    try:
        c.run('compiler_version', ['g++', '--version'])
        for mode in ('O2', 'san', 'mutant'):
            source = c.BASE / ('mutant' if mode == 'mutant' else 'source_snapshot')
            binary, dep = c.BASE / 'bin' / mode, c.LOG / (mode + '.d')
            flags = ['-O2'] if mode != 'san' else ['-O1', '-g', '-fsanitize=address,undefined',
                '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
            c.run(mode + '_compile', ['g++', '-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                '-pthread', *flags, '-MMD', '-MF', str(dep), str(source / GATE), '-o', str(binary)], compile_step=True)
            result['binaries'][mode] = c.sha(binary)
            deps = [Path(name).resolve() for name in dep.read_text().replace('\\\n', ' ').split(':', 1)[1].split()]
            c.need({str(p.relative_to(source)) for p in deps} == set(live), 'exact_27_compiled_dependencies')
            c.save(c.LOG / (mode + '.dependencies.json'), {str(p): c.sha(p) for p in deps})
            c.run(mode + '_selftest', [str(binary), '--selftest'], 1 if mode == 'mutant' else 0)
            if mode == 'mutant':
                cause = 'wspd q2 front rejected: line.q2_positive_core_value\n'
                c.need((c.LOG / 'mutant_selftest.stderr').read_text() == cause, 'mutant_exact_cause')
                result['mutant_first_cause'] = cause.strip()
            else:
                out = json.loads((c.LOG / (mode + '_selftest.stdout')).read_text())
                c.need(out['status'] == 'passed' and out['calls'] == 174 and out['refusals'] == 6
                       and out['q2_positive_core_checks'] == 1, 'positive_core_nonvacuity')
                c.need((c.LOG / (mode + '_selftest.stderr')).read_bytes() == b'', 'nominal_stderr')
                c.run(mode + '_unknown', [str(binary), '--unknown'], 2)
                c.need((c.LOG / (mode + '_unknown.stdout')).read_bytes() == b''
                       and (c.LOG / (mode + '_unknown.stderr')).read_bytes() == b'', 'unknown_empty')
            c.need(c.sha(binary) == result['binaries'][mode], 'binary_stable')
        c.need((c.LOG / 'O2_selftest.stdout').read_bytes() == (c.LOG / 'san_selftest.stdout').read_bytes(), 'O2_SAN_equal')
        result.update(status='completed', q2_positive_core_checks_per_mode=1, nominal_outputs_equal=True)
    except Exception as error:
        result['error'] = type(error).__name__ + ': ' + str(error)
    finally:
        after = c.sources()
        c.save(c.BASE / 'sources_after.json', after)
        result.update(sources_stable=before == after, commands=len(c.COMMANDS),
            cpp_closed=all(row['closed'] for row in c.COMMANDS),
            live_sources_unchanged=all(c.sha(c.ROOT / name) == expected for name, expected in live.items()))
        if not result['sources_stable'] or not result['live_sources_unchanged']:
            result['status'] = 'failed'
        c.save(c.BASE / 'receipt.json', result)
        print(json.dumps(result, sort_keys=True), flush=True)
    return 0 if result['status'] == 'completed' else 1


if __name__ == '__main__':
    sys.exit(main())
