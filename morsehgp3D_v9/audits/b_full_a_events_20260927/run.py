#!/usr/bin/env python3
"""Serial event-A gate; explicit port of frozen manifest runner R2 dd36210c.
No engine/cloud; LIVE readers never rebuild or rerun.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shlex
import signal
import sys

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SOURCE = ROOT / 'morsehgp3D_v9/src'
BOOST = Path('/workspaces/E-HGP/build/v7_boost_gate/extracted/usr')
HELPER = ROOT / 'gcp-migration/full_probe_session_v7.py'
HELPER_SHA = '177b25a0d72150dc331661fdf8da1ccde77ea17fb694d9c6af5b0929755160d8'
TARGET = 'mhgp9_full_a_events'
SCHEMA = 'mhgp9_full_a_events_capture_v1'
MANIFEST = HERE.parent / 'b_full_a_manifest_20260927'
PORT_RUN = MANIFEST / 'run.py'
PORT_SHA = 'dd36210c8f9a42aeb8a4b597d137745b1ae1f8e9bf6079c7a6bd27fc262d5c61'
MODEL = HERE.parent / 'b_full_phase_a_work_20260926/event_model.py'
MODEL_SHA = 'b59d908368c92a4e0305aa0ea55936014ea0aade2e2273683135c272bdf47085'
KINDS = ('release', 'sanitize')
EXPECTED_REASONS = ['event.predecessor_missing', 'replay.occurrence_roots', 'replay.occurrence_roots']
EXPECTED_COUNTS = dict(fixtures=22, captures=44, event_replays=376, vertices=4632, edges=5416,
                       groups=3784, parent_event_incidences=3408, parent_draft_incidences=3024,
                       parent_forest_incidences=2992, silent_groups=384, continuations=32,
                       max_parents=32, max_combined_capacity_observed_bytes=13004,
                       ancestor_queries=10048, ancestor_steps=54032, boundary_replays=2, abstract_replays=64)


def need(ok, reason):
    if not ok:
        raise RuntimeError(reason)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def read(path):
    return json.loads(Path(path).read_text())


def save(path, value):
    Path(path).write_text(json.dumps(value, sort_keys=True, indent=2, allow_nan=False) + '\n')


def sources():
    need(sha(PORT_RUN) == PORT_SHA and sha(MODEL) == MODEL_SHA, 'explicit port source pins')
    paths = [p for p in SOURCE.rglob('*') if p.suffix in ('.hpp', '.cpp', '.h')]
    paths += [HERE / n for n in ('events.hpp', 'probe.cpp', 'CMakeLists.txt', 'run.py', 'selftest.py')]
    paths += [MANIFEST / n for n in ('capture.hpp', 'manifest.hpp', 'fixtures.hpp',
              'native_a.hpp', 'instrument.py', 'instrument.diff', 'instrument.json')]
    paths += [ROOT / 'morsehgp3D_v9/oracle/tower/local_plateau_oracle.hpp',
              ROOT / 'morsehgp3D_v9/tests/tower/full_ball_tower_gate.cpp', HELPER, PORT_RUN, MODEL]
    return {str(p.resolve()): sha(p) for p in sorted(set(paths))}


def dependencies(text):
    logical = text.replace('\\\n', ' ')
    need(': ' in logical, 'dependency target missing')
    names = shlex.split(logical.split(': ', 1)[1])
    need(names and len(names) == len(set(names)), 'empty or duplicate dependency inventory')
    result = set()
    for name in names:
        p = Path(name)
        p = (ROOT / p).resolve() if not p.is_absolute() else p.resolve()
        need(p.is_file(), 'missing dependency: ' + str(p))
        result.add(str(p))
    return result


def recipe(state):
    rows = [('system', ['uname', '-a'], 0),
            ('instrument', ['python3', '-B', str(MANIFEST / 'instrument.py'), '--check'], 0),
            ('instrument_optimized', ['python3', '-B', '-O', str(MANIFEST / 'instrument.py'), '--check'], 0)]
    for kind in KINDS:
        compiler = 'c++' if kind == 'release' else 'clang++'
        # Optimization changes glibc include paths; sanitizer adds its ignorelist
        # to actual compiler dependencies. Match all compilation-affecting flags.
        flags = ['-O3', '-DNDEBUG', '-Wall', '-Wextra', '-Wpedantic', '-Werror']
        if kind == 'sanitize':
            flags += ['-O1', '-g1', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-sanitize-recover=all']
        rows += [('compiler_' + kind, [compiler, '--version'], 0),
                 ('dependencies_' + kind, [compiler, '-std=c++20', *flags,
                   '-I' + str(SOURCE), '-isystem', str(BOOST / 'include'), '-M', str(HERE / 'probe.cpp')], 0)]
    for kind in KINDS:
        compiler = 'c++' if kind == 'release' else 'clang++'
        build = state['builds'][kind]
        binary = state['binaries'][kind]
        rows += [('configure_' + kind, ['cmake', '-S', str(HERE), '-B', build,
                   '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_CXX_COMPILER=' + compiler,
                   '-DBOOST_ROOT=' + str(BOOST), '-DMHGP9_EVENT_SANITIZE=' + ('ON' if kind == 'sanitize' else 'OFF')], 0),
                 ('build_' + kind, ['cmake', '--build', build, '--target', TARGET, '-j', '2'], 0),
                 ('gate_' + kind, [binary, '--gate'], 0),
                 ('cli_no_arg_' + kind, [binary], 2),
                 ('cli_unknown_' + kind, [binary, '--unknown'], 2)]
    return rows


def build_pins(state):
    out = {}
    for path in state['binaries'].values():
        p = Path(path)
        files = [p, p.parent / 'CMakeCache.txt']
        folder = p.parent / 'CMakeFiles' / (TARGET + '.dir')
        files += [folder / n for n in ('flags.make', 'link.txt', 'probe.cpp.o', 'probe.cpp.o.d')]
        for f in files:
            out[str(f)] = sha(f)
    return out


def validate_gate(value):
    need(value.get('schema') == 'mhgp9_full_a_events_v1' and value.get('status') == 'passed'
         and value.get('GCP_used') is False and value.get('full_gate') is True
         and value.get('parallel') is False and value.get('FULL_executed_by_candidate') is False, 'serial A-only gate scope')
    fields = ('fixtures', 'captures', 'event_replays', 'vertices', 'edges', 'groups',
              'parent_event_incidences', 'parent_draft_incidences', 'parent_forest_incidences',
              'silent_groups', 'continuations', 'max_parents', 'max_combined_capacity_observed_bytes',
              'ancestor_queries', 'ancestor_steps', 'boundary_replays', 'abstract_replays')
    need(set(value) == set(fields) | {'schema', 'status', 'GCP_used', 'full_gate', 'parallel',
                                   'FULL_executed_by_candidate', 'mutant_reasons'}, 'gate exact fields')
    need(all(type(value[k]) is int and 0 < value[k] <= (1 << 64)-1 for k in fields), 'non-vacuous strict u64 counters')
    need({k: value[k] for k in fields} == EXPECTED_COUNTS, 'deterministic bounded corpus counters')
    need(value['fixtures'] == 22 and value['captures'] == 44 and value['event_replays'] == 376 and
         value['max_parents'] == 32 and value['boundary_replays'] == 2 and value['abstract_replays'] == 64,
         'fixed bounded geometric and abstract corpus')
    need(value['groups'] <= value['vertices'] and
         value['parent_forest_incidences'] <= value['parent_draft_incidences'] <= value['parent_event_incidences'] <= value['edges']
         and value['ancestor_queries'] == value['vertices'] + value['edges'] and
         value['ancestor_steps'] >= value['ancestor_queries'], 'work-domain identities')
    reasons = value['mutant_reasons']
    need(type(reasons) is list and len(reasons) == 3 and all(type(x) is str for x in reasons), 'three causal branch reasons')
    need(reasons == EXPECTED_REASONS, 'fixed causal branch refusals')


def readback(directory):
    state = read(directory / 'capture.json')
    need(state['schema'] == SCHEMA and state['status'] == 'completed' and state['GCP_used'] is False, 'closed local capture')
    need(set(state['builds']) == set(state['binaries']) == set(KINDS) and
         all(state['binaries'][k] == str(Path(state['builds'][k]) / TARGET) for k in KINDS), 'binary/build binding')
    current = sources()
    need(state['source_pins_before'] == state['source_pins_after'] == current, 'LIVE source closure')
    need(state['dependency_pins_before'] == state['dependency_pins_after'], 'compiled dependency closure')
    need(all(sha(p) == h for p, h in state['dependency_pins_before'].items()), 'LIVE system/Boost/include pins')
    need(state['build_pins'] == build_pins(state), 'LIVE build closure')
    need(sha(HELPER) == HELPER_SHA, 'collector identity')
    planned = recipe(state)
    need(len(state['commands']) == len(planned), 'complete command inventory')
    gates, inventory, last_end = [], set(), 0
    for command, (name, argv, code) in zip(state['commands'], planned):
        need(command['name'] == name and command['argv'] == argv and command['exit_code'] == code and
             command['group_closed'] and command['ended_epoch'] >= command['started_epoch'], 'command recipe/closure')
        need(command['started_epoch'] >= last_end, 'sequential command chronology')
        last_end = command['ended_epoch']
        need(read(directory / (name + '.command.json')) == command, 'command binding')
        need(all(command.get(k) == v for k, v in read(directory / (name + '.intent.json')).items()), 'intent binding')
        for stream in ('stdout', 'stderr'):
            need(sha(directory / (name + '.' + stream)) == command[stream + '_sha256'], 'stream hash')
        if name.startswith('dependencies_'):
            inventory |= dependencies((directory / (name + '.stdout')).read_text())
        if name.startswith('gate_'):
            value = read(directory / (name + '.stdout'))
            validate_gate(value)
            need((directory / (name + '.stderr')).read_text() == '', 'gate stderr clean')
            gates.append(value)
        if name.startswith('cli_'):
            need((directory / (name + '.stdout')).read_text() == '' and
                 (directory / (name + '.stderr')).read_text() == 'event.usage\n', 'exact CLI refusal')
    need(set(state['dependency_pins_before']) == inventory, 'dependency command inventory binding')
    for kind in KINDS:
        dep = Path(state['builds'][kind]) / 'CMakeFiles' / (TARGET + '.dir') / 'probe.cpp.o.d'
        built = dependencies(dep.read_text())
        need(built <= inventory and str(MANIFEST / 'native_a.hpp') in built and str(MANIFEST / 'manifest.hpp') in built and str(HERE / 'events.hpp') in built,
             'actual compiler dependency inclusion')
    need(len(gates) == 2 and gates[0] == gates[1], 'Release/sanitizer exact equality')
    return dict(status='PASS', commands=len(planned), gate=gates[0], dependencies=len(inventory),
                scope='event_a_C++_serial', parallel=False, FULL_executed_by_candidate=False, GCP_used=False)


def capture(args):
    directory = args.capture.resolve()
    builds = {k: str(args.build_prefix.resolve()) + '_' + k for k in KINDS}
    need(not directory.exists() and all(not Path(p).exists() for p in builds.values()), 'fresh receipt/builds required')
    need(sha(HELPER) == HELPER_SHA, 'collector pin before import')
    spec = importlib.util.spec_from_file_location('event_process_collector', HELPER)
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    directory.mkdir(parents=True)
    state = dict(schema=SCHEMA, status='failed', GCP_used=False, builds=builds,
                 binaries={k: str(Path(p) / TARGET) for k, p in builds.items()},
                 source_pins_before=sources(), dependency_pins_before={}, build_pins={})
    env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1', ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    commands = helper.Commands(directory, env)

    def interrupted(signum, _frame):
        raise InterruptedError('signal ' + str(signum))

    old_handlers = {sig: signal.signal(sig, interrupted) for sig in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP)}
    try:
        for name, argv, expected in recipe(state):
            code, _, _ = commands.run(name, argv, timeout=None)
            need(code == expected, name + ': unexpected exit ' + str(code))
            if name.startswith('dependencies_'):
                for p in dependencies((directory / (name + '.stdout')).read_text()):
                    digest = sha(p)
                    need(p not in state['dependency_pins_before'] or state['dependency_pins_before'][p] == digest,
                         'dependency changed during discovery')
                    state['dependency_pins_before'][p] = digest
            if name.startswith('gate_'):
                validate_gate(read(directory / (name + '.stdout')))
            print(name + ': closed ' + str(code), flush=True)
        state['build_pins'] = build_pins(state)
        state['status'] = 'completed'
    except BaseException as error:
        state['error'] = type(error).__name__ + ': ' + str(error)
        raise
    finally:
        state['commands'] = commands.rows
        state['source_pins_after'] = {p: sha(p) for p in state['source_pins_before']}
        state['dependency_pins_after'] = {p: sha(p) for p in state['dependency_pins_before']}
        if state['source_pins_before'] != state['source_pins_after'] or state['dependency_pins_before'] != state['dependency_pins_after']:
            state['status'] = 'failed'
        save(directory / 'capture.json', state)
        for sig, handler in old_handlers.items():
            signal.signal(sig, handler)
    try:
        result = readback(directory)
    except BaseException as error:
        save(directory / 'readback_failure.json', dict(status='failed', error=type(error).__name__ + ': ' + str(error),
                                                      commands_completed=state['status'] == 'completed'))
        raise
    save(directory / 'summary.json', result)
    print(json.dumps(result, sort_keys=True), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--capture', type=Path)
    mode.add_argument('--readback', type=Path)
    parser.add_argument('--build-prefix', type=Path)
    args = parser.parse_args()
    if args.readback:
        print(json.dumps(readback(args.readback.resolve()), sort_keys=True))
    else:
        need(args.build_prefix is not None, 'build prefix required')
        capture(args)


if __name__ == '__main__':
    main()
