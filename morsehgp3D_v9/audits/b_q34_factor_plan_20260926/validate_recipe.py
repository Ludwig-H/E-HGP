#!/usr/bin/env python3
"""Small complementary LIVE recipe check. No capture, build, cloud call or mutation.

The in-flight run.py/inputs.py stay byte-for-byte unchanged. This checker
first uses their existing complete readback, then binds compilation commands
to the two distinct qualified binaries and constrains ID-map paths.
"""
import argparse
import copy
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
import run as capture

need = capture.need


def expected_commands(binaries):
    need(set(binaries) == {'release', 'sanitize'}, 'recipe binary roles')
    paths = {kind: Path(path) for kind, path in binaries.items()}
    need(len(set(paths.values())) == 2, 'recipe binaries must be distinct')
    prefixes = []
    expected = {'system': ['uname', '-a'], 'cpu': ['lscpu']}
    for kind, binary in paths.items():
        need(binary.is_absolute() and binary.name == capture.TARGET and
             binary.parent.name.endswith('_' + kind), 'recipe binary path/role')
        prefixes.append(str(binary.parent)[:-len('_' + kind)])
        compiler = 'c++' if kind == 'release' else 'clang++'
        flags = '-Wall -Wextra -Wpedantic -Werror'
        if kind == 'sanitize':
            flags += ' -O1 -g1 -fsanitize=address,undefined -fno-omit-frame-pointer'
        expected['compiler_' + kind] = [compiler, '--version']
        expected['configure_' + kind] = ['cmake', '-S', str(capture.HERE), '-B', str(binary.parent),
            '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_CXX_COMPILER=' + compiler, '-DCMAKE_CXX_FLAGS=' + flags,
            '-DMHGP9_SOURCE_ROOT=' + str(capture.ROOT / 'morsehgp3D_v9'),
            '-DMHGP9_GEN_LIBRARY=' + str(capture.LIBS[kind])]
        expected['build_' + kind] = ['cmake', '--build', str(binary.parent), '--target', capture.TARGET, '-j', '2']
    need(prefixes[0] == prefixes[1], 'recipe build prefixes differ')
    return expected


def check_recipe(state):
    expected = expected_commands(state['binaries'])
    names = [row['name'] for row in state['commands']]
    need(len(names) == len(set(names)), 'recipe duplicate command')
    actual = {row['name']: row['argv'] for row in state['commands']}
    for name, argv in expected.items():
        need(actual.get(name) == argv, 'recipe argv: ' + name)
    return len(expected)


def check_id_paths(directory, manifest):
    count = 0
    for part in capture.inputs.PARTS:
        entry = manifest['datasets'][part]
        for stem in ('site_ids', 'original_site_ids'):
            path = directory / entry[stem + '_file']
            need(path.name == part + '.' + stem + '.u32le' and path.parent == directory and not path.is_symlink(),
                 'recipe ID map must be an immediate child of its dataset')
            count += 1
    return count


def audit(directory):
    summary = capture.readback(directory)
    state = capture.read(directory / 'capture.json')
    recipes = check_recipe(state)
    maps = 0
    for scene in ('00', '01', '02'):
        data = capture.inputs.GROUND / ('scene_' + scene + '_grid')
        maps += check_id_paths(data, capture.read(data / 'MANIFEST.json'))
    return dict(status='PASS', scope=summary['scope'], GCP_used=False,
                base_readback_passed=True, bound_recipe_commands=recipes, immediate_ID_maps=maps,
                capture_sha256=capture.sha(directory / 'capture.json'),
                checker_sha256=capture.sha(Path(__file__)), run_sha256=capture.sha(capture.HERE / 'run.py'),
                inputs_sha256=capture.sha(capture.HERE / 'inputs.py'))


def selftest():
    binaries = {kind: '/tmp/factor_recipe_fixture_' + kind + '/' + capture.TARGET
                for kind in ('release', 'sanitize')}
    state = dict(binaries=binaries, commands=[dict(name=name, argv=argv)
                 for name, argv in expected_commands(binaries).items()])
    need(check_recipe(state) == 8, 'recipe fixture not reached')
    killed = 0

    def reject(mutator):
        nonlocal killed
        wrong = copy.deepcopy(state)
        mutator(wrong)
        try:
            check_recipe(wrong)
        except ValueError:
            killed += 1
            return
        raise ValueError('recipe mutation survived')

    def command(value, name):
        return next(row for row in value['commands'] if row['name'] == name)['argv']

    reject(lambda value: value['binaries'].__setitem__('sanitize', value['binaries']['release']))
    reject(lambda value: command(value, 'compiler_sanitize').__setitem__(0, 'c++'))
    reject(lambda value: command(value, 'configure_sanitize').__setitem__(-1,
           '-DMHGP9_GEN_LIBRARY=' + str(capture.LIBS['release'])))
    reject(lambda value: command(value, 'configure_release').__setitem__(-2, '-DMHGP9_SOURCE_ROOT=/tmp/other'))
    reject(lambda value: command(value, 'configure_sanitize').__setitem__(-3, '-DCMAKE_CXX_FLAGS=-O3'))
    reject(lambda value: command(value, 'build_release').__setitem__(4, 'wrong_target'))
    reject(lambda value: command(value, 'build_sanitize').__setitem__(2,
           str(Path(value['binaries']['release']).parent)))
    reject(lambda value: value['commands'].pop())
    reject(lambda value: value['commands'].append(copy.deepcopy(value['commands'][0])))
    directory = Path('/tmp/factor_recipe_dataset_fixture')
    manifest = {'datasets': {part: {stem + '_file': part + '.' + stem + '.u32le'
                 for stem in ('site_ids', 'original_site_ids')} for part in capture.inputs.PARTS}}
    need(check_id_paths(directory, manifest) == 14, 'ID map fixture not reached')
    manifest['datasets']['full']['site_ids_file'] = 'elsewhere/full.site_ids.u32le'
    try:
        check_id_paths(directory, manifest)
    except ValueError:
        killed += 1
    else:
        raise ValueError('ID map parent mutation survived')
    need(killed == 10, 'recipe coverage floor')
    return dict(status='PASS', scope='offline_reader_tests_not_native_gates', valid_recipe_checks=8,
                valid_ID_paths=14, killed=10, GCP_used=False, builds_executed=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument('--readback', type=Path)
    action.add_argument('--selftest', action='store_true')
    args = parser.parse_args()
    print(json.dumps(selftest() if args.selftest else audit(args.readback.resolve()), sort_keys=True))


if __name__ == '__main__':
    main()
