#!/usr/bin/env python3
"""Fresh CPU-only build; explicit port of the frozen fixed-K build adapter.

Origin: audits/b_point_hierarchy_k_20260927/build_native.py,
SHA256 073f7bc97bf93b0d6c690fdb4e1dcfc91af845c04a78432afd20e57c782e31fb.
Changes: new adapter/unit and artifact/schema names, exclusive receipt writes.
No old object or build directory is reused. Not a new engine qualification.
"""
import argparse
import concurrent.futures
import hashlib
import json
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
V9 = ROOT / 'morsehgp3D_v9'


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    with path.open('x') as stream:
        stream.write(json.dumps(value, indent=2, sort_keys=True, allow_nan=False) + '\n')


def dependencies(text, directory):
    body = text.replace('\\\n', ' ')
    need(': ' in body, 'dependency syntax')
    paths = sorted({str((directory / name).resolve()) for name in shlex.split(body.split(': ', 1)[1])})
    need(paths and all(Path(p).is_file() for p in paths), 'dependency files')
    return paths


def build(args):
    directory = args.build.resolve()
    need(not directory.exists(), 'build directory must be NEW')
    need(directory.parent.is_dir() and directory not in (ROOT, V9, HERE), 'build parent')
    need(not directory.is_relative_to(V9), 'build must be outside versioned v9 source')
    directory.mkdir()
    started = time.monotonic()
    receipt = dict(schema='mhgp9_weighted_native_build_v1', status='started', GCP_used=False,
                   engine_modified=False, build=str(directory), commands=[])

    def run(name, argv, timeout=180):
        start = time.monotonic()
        with (directory / (name + '.stdout')).open('xb') as out, (directory / (name + '.stderr')).open('xb') as err:
            result = subprocess.run(argv, cwd=directory, stdout=out, stderr=err, timeout=timeout, check=False)
        command = dict(name=name, argv=argv, returncode=result.returncode, elapsed_seconds=time.monotonic()-start,
                       stdout_sha256=sha(directory/(name+'.stdout')), stderr_sha256=sha(directory/(name+'.stderr')))
        receipt['commands'].append(command)
        save(directory/(name+'.command.json'), command)
        need(result.returncode == 0, name + ' failed; inspect ' + str(directory/(name+'.stderr')))
        return (directory / (name+'.stdout')).read_text()

    try:
        compiler = shutil.which(args.compiler)
        need(compiler is not None, 'compiler available')
        compiler = str(Path(compiler).resolve())
        cmake = V9 / 'CMakeLists.txt'
        match = re.search(r'add_library\(mhgp9_gen STATIC\s+(.*?)\)', cmake.read_text(), re.S)
        need(match is not None, 'GEN source inventory')
        gen = [V9 / name for name in match.group(1).split()]
        need(len(gen) == len(set(gen)) == 25, '25 GEN units')
        units = gen + [V9/'src/chain/tower_chain.cpp', V9/'src/gpu/filter_runner_stub.cpp', HERE/'native_weighted_export.cpp']
        flags = ['-std=c++20', '-O3', '-DNDEBUG', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-pthread',
                 '-I'+str(V9/'src/gen')]
        if args.sanitize:
            flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
        pins = {str(p):sha(p) for p in [*units, cmake, Path(__file__), Path(compiler)]}
        run('compiler', [compiler,'--version'])

        def discover(pair):
            i, unit = pair
            text = run(f'dependencies_{i:02d}', [compiler,*flags,'-M',str(unit)], 120)
            return dependencies(text, directory)

        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
            discovered = list(pool.map(discover, enumerate(units)))
        for names in discovered:
            pins.update({name:sha(name) for name in names})
        receipt.update(compiler=compiler, flags=flags, units=list(map(str,units)), pins_before=pins)
        save(directory/'inputs.json', dict(pins=pins, units=list(map(str,units)), flags=flags))
        objects = [directory / f'unit_{i:02d}.o' for i in range(len(units))]

        def compile_one(i):
            run(f'compile_{i:02d}', [compiler,*flags,'-MD','-MF',str(objects[i])+'.d','-c',str(units[i]),'-o',str(objects[i])], 600)
            actual = dependencies(Path(str(objects[i])+'.d').read_text(), directory)
            need(actual == discovered[i], 'actual dependencies differ from pre-build discovery')

        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
            list(pool.map(compile_one, range(len(units))))
        binary = directory/'native_weighted_export'
        run('link', [compiler,*flags,*map(str,objects),'-o',str(binary)], 120)
        after = {path:sha(path) for path in pins}
        need(after == pins, 'compiled inputs changed during build')
        artifacts = [binary,*objects,*(Path(str(p)+'.d') for p in objects)]
        receipt.update(status='completed', binary=str(binary), binary_sha256=sha(binary), pins_after=after,
                       artifacts_sha256={str(p):sha(p) for p in artifacts})
        print(json.dumps(dict(status='completed', binary=str(binary), binary_sha256=sha(binary))))
    except BaseException as exc:
        receipt.update(status='failed', error=str(exc))
        raise
    finally:
        receipt['elapsed_seconds'] = time.monotonic()-started
        receipt['commands'].sort(key=lambda c: c['name'])
        save(directory/'receipt.json', receipt)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--compiler', default='c++')
    parser.add_argument('--jobs', type=int, default=2, choices=range(1,5))
    parser.add_argument('--sanitize', action='store_true')
    build(parser.parse_args())
