#!/usr/bin/env python3
"""Fresh compiler closure, including indirect options. No cloud operations.

Explicit design port of the high-u64 gate prepin contract. This file records
its own remote dependencies; no old CUDA receipt is a compilation authority.
"""
import argparse
import json
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import common as c

SCHEMA = 'mhgp9_survivors_compile_closure_v1'
TARGET = 'mhgp9_survivors_compare'
GATE = 'mhgp9_survivors_device_gate'
BUILT_TARGETS = {TARGET: 5, GATE: 1, 'resident_gen': 25}
EXCLUDED_TARGETS = {'mhgp9_q34_filtered_resident': 3}


def binaries(build):
    return {'compare': build / TARGET, 'high_keys': build / 'device_gate' / GATE}


def dependencies(text, cwd):
    text = text.replace('\\\n', ' ')
    c.need(': ' in text, 'dependency target syntax')
    names = shlex.split(text.split(': ', 1)[1])
    c.need(names, 'nonempty dependency inventory')
    return {str((Path(p) if Path(p).is_absolute() else cwd / p).resolve()) for p in names}


def jobs(build):
    return jobs_from_entries(c.read(build / 'compile_commands.json'))


def jobs_from_entries(entries):
    result = []
    counts = {name: 0 for name in (*BUILT_TARGETS, *EXCLUDED_TARGETS)}
    for entry in entries:
        argv = entry.get('arguments') or shlex.split(entry['command'])
        c.need(argv.count('-o') == 1 and argv.index('-o') + 1 < len(argv), 'explicit object output')
        cwd = Path(entry['directory']).resolve()
        obj = (cwd / argv[argv.index('-o') + 1]).resolve()
        parts = obj.parts
        c.need('CMakeFiles' in parts and parts.index('CMakeFiles') + 1 < len(parts), 'CMake object target')
        folder = parts[parts.index('CMakeFiles') + 1]
        c.need(folder.endswith('.dir') and folder[:-4] in counts, 'fixed declared compilation targets')
        target = folder[:-4]
        counts[target] += 1
        if target in EXCLUDED_TARGETS:
            continue
        command, skip = [], False
        for token in argv:
            if skip:
                skip = False
                continue
            if token in ('-o', '-MF', '-MT', '-MQ'):
                skip = True
            elif token not in ('-c', '-MD', '-MMD', '-MP', '-fsyntax-only'):
                command.append(token)
        c.need(not skip and command, 'complete compilation arguments')
        result.append(dict(cwd=str(cwd), argv=command + ['-M'], source=str(Path(entry['file']).resolve()),
                           object=str(obj), target=target))
    c.need(counts == BUILT_TARGETS | EXCLUDED_TARGETS, '31 built TU and three explicitly excluded baseline TU')
    return result


def generated(build):
    paths = {build / 'compile_commands.json', build / 'CMakeCache.txt'}
    for pattern in ('*.rsp', 'flags.make', 'link.txt'):
        paths.update(build.rglob(pattern))
    c.need(any(p.suffix == '.rsp' for p in paths), 'indirect CUDA flags expected')
    return {str(p.resolve()): p.read_text() for p in sorted(paths)}


def expand(tokens, cwd, stack=()):
    result = []
    for token in tokens:
        if token.startswith('@'):
            path = (cwd / token[1:]).resolve()
            c.need(path not in stack and len(stack) < 16, 'cyclic/deep response arguments')
            result.extend(expand(shlex.split(path.read_text()), cwd, (*stack, path)))
        else:
            result.append(token)
    return result


def archives(build):
    paths = set()
    for link in build.rglob('link.txt'):
        if link.parent.name not in {name + '.dir' for name in BUILT_TARGETS}:
            continue
        cwd = link.parents[2]
        argv = expand(shlex.split(link.read_text()), cwd)
        dirs, names = [], []
        for index, token in enumerate(argv):
            if token == '-L':
                c.need(index + 1 < len(argv), 'link search argument')
                dirs.append((cwd / argv[index + 1]).resolve())
            elif token.startswith('-L'):
                dirs.append((cwd / token[2:]).resolve())
            elif token in ('-lcudart_static', '-lcudadevrt'):
                names.append('lib' + token[2:])
            elif token.endswith('.a'):
                path = (cwd / token).resolve()
                if not path.is_relative_to(build):
                    c.need(path.is_file(), 'external link archive exists')
                    paths.add(path)
        for name in names:
            # GNU -l search prefers a shared library to its archive within the
            # first matching search directory. Never silently pin the .a instead.
            found = next((directory / (name + suffix) for directory in dirs for suffix in ('.so', '.a')
                          if (directory / (name + suffix)).is_file()), None)
            c.need(found is not None, 'actual CUDA archive search resolution')
            c.need(found.suffix == '.a', 'unexpected dynamic CUDA link target')
            paths.add(found.resolve())
    c.need({'libcudart_static.a', 'libcudadevrt.a', 'librt.a'} <= {p.name for p in paths},
           'CUDA and rt archives explicitly closed')
    return {str(p): c.sha(p) for p in sorted(paths)}


def toolchain(compilation_jobs):
    paths, cuda_roots = set(), set()
    for job in compilation_jobs:
        executable = shutil.which(job['argv'][0])
        c.need(executable is not None, 'actual compiler executable')
        compiler = Path(executable).resolve()
        paths.add(compiler)
        if compiler.name == 'nvcc':
            c.need(compiler.parent.name == 'bin', 'installed CUDA bin directory')
            cuda_roots.add(compiler.parent.parent)
    for name in ('c++', 'g++', 'cmake'):
        executable = shutil.which(name)
        c.need(executable is not None, 'toolchain executable')
        paths.add(Path(executable).resolve())
    c.need(len(cuda_roots) == 1, 'one effective CUDA compiler root')
    cuda = next(iter(cuda_roots))
    c.need((cuda / 'bin/nvcc').is_file(), 'installed CUDA required; no installation')
    for folder in ('bin', 'nvvm'):
        paths.update(p.resolve() for p in (cuda / folder).rglob('*') if p.is_file())
    return {str(p): c.sha(p) for p in sorted(paths)}


def source_binding(headers, root, manifest):
    local = set()
    for name, digest in headers.items():
        path = Path(name)
        if path.is_relative_to(root):
            relative = str(path.relative_to(root))
            c.need(manifest.get(relative) == digest, 'compiled source bound to committed manifest')
            local.add(relative)
    required = {c.PROTOTYPE + '/probe.cpp', c.SURVIVORS + '/device_cuda.cu',
                c.DIRECT_GATE + '/device_gate.cu',
                'morsehgp3D_v9/audits/b_q34_filtered_resident_20260927/device_cuda.cu',
                'morsehgp3D_v9/src/gpu/witness_filter.hpp'}
    c.need(required <= local, 'real comparator/candidate/reference/device dependencies')


def prepin(root, build, manifest):
    texts = generated(build)
    compilation_jobs = jobs(build)
    tools, libraries = toolchain(compilation_jobs), archives(build)
    headers, commands = {}, []
    for job in compilation_jobs:
        result = subprocess.run(job['argv'], cwd=job['cwd'], text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, check=False)
        commands.append(job | dict(exit_code=result.returncode, stdout=result.stdout, stderr=result.stderr))
        c.need(result.returncode == 0, 'dependency compilation failed: ' + result.stderr)
        for name in dependencies(result.stdout, Path(job['cwd'])):
            digest = c.sha(name)
            c.need(name not in headers or headers[name] == digest, 'header changed during discovery')
            headers[name] = digest
    source_binding(headers, root, manifest)
    c.need(texts == generated(build) and tools == toolchain(compilation_jobs) and libraries == archives(build) and
           all(c.sha(p) == h for p, h in headers.items()), 'prebuild discovery stable')
    return dict(schema=SCHEMA, stage='before_build', root=str(root), build=str(build),
                generated=texts, tools=tools, archives=libraries, headers=headers, commands=commands)


def close(root, build, manifest, before):
    c.need(before['schema'] == SCHEMA and before['stage'] == 'before_build' and
           before['root'] == str(root) and before['build'] == str(build), 'prebuild identity')
    c.need(before['generated'] == generated(build), 'all indirect options/configurations unchanged')
    c.need(before['tools'] == toolchain(jobs(build)) and before['archives'] == archives(build),
           'compiler and linked archives unchanged')
    c.need(all(c.sha(path) == digest for path, digest in before['headers'].items()), 'headers unchanged')
    used, objects, depfiles = {}, {}, {}
    for path in sorted(build.rglob('*.o.d')):
        text = path.read_text()
        depfiles[str(path)] = text
        # Compiler invocations below top-level run in their sub-build directory.
        target_part = next(i for i, part in enumerate(path.parts) if part == 'CMakeFiles')
        cwd = Path(*path.parts[:target_part])
        for name in dependencies(text, cwd):
            c.need(name in before['headers'] and c.sha(name) == before['headers'][name],
                   'actual compiled dependency was pinned before build')
            used[name] = before['headers'][name]
        obj = path.with_suffix('')
        c.need(obj.is_file(), 'compiled object exists')
        objects[str(obj)] = c.sha(obj)
    expected_objects = {job['object'] for job in jobs(build)}
    c.need(set(objects) == expected_objects, 'all 31 expected compiled objects and dependencies present')
    source_binding(used, root, manifest)
    result_binaries = {key: {'path': str(path), 'sha256': c.sha(path)} for key, path in binaries(build).items()}
    built_archives = {str(p): c.sha(p) for p in sorted(build.rglob('*.a'))}
    c.need(set(built_archives) == {str(build / 'baseline/libresident_gen.a')}, 'fresh generated GEN archive')
    return dict(schema=SCHEMA, stage='after_build', root=str(root), build=str(build),
                generated=generated(build), tools=before['tools'], archives=before['archives'],
                headers=before['headers'], compiled_dependencies=used, dependency_files=depfiles,
                objects=objects, built_archives=built_archives, binaries=result_binaries)


def validate_closed(before, after, root, build, manifest):
    """Read received evidence only; never tries to open remote absolute paths."""
    for value, stage in ((before, 'before_build'), (after, 'after_build')):
        c.need(type(value) is dict and value.get('schema') == SCHEMA and value.get('stage') == stage and
               value.get('root') == str(root) and value.get('build') == str(build), 'compile closure identity')
    for field in ('generated', 'tools', 'archives', 'headers'):
        c.need(type(before.get(field)) is dict and before[field] and before[field] == after.get(field),
               'compile closure unchanged ' + field)
    for field in ('tools', 'archives', 'headers', 'compiled_dependencies', 'objects', 'built_archives'):
        values = after.get(field)
        c.need(type(values) is dict and (values or field == 'built_archives'), 'compile pin map ' + field)
        for path, digest in values.items():
            c.need(type(path) is str and Path(path).is_absolute() and '..' not in Path(path).parts and
                   type(digest) is str and re.fullmatch('[0-9a-f]{64}', digest), 'compile pin form')
    texts = after['generated']
    for name, text in texts.items():
        c.need(type(name) is str and type(text) is str and Path(name).is_relative_to(build) and
               '..' not in Path(name).parts and
               (name.endswith(('.rsp', '/flags.make', '/link.txt', '/compile_commands.json', '/CMakeCache.txt'))),
               'generated configuration belongs to build')
    c.need(any(name.endswith('.rsp') for name in texts), 'response files closed')
    c.need({'libcudart_static.a', 'libcudadevrt.a', 'librt.a'} <= {Path(p).name for p in after['archives']},
           'received CUDA archives complete')
    raw_commands = json.loads(texts[str(build / 'compile_commands.json')], object_pairs_hook=c.unique)
    planned = jobs_from_entries(raw_commands)
    commands = before.get('commands')
    c.need(type(commands) is list and len(commands) == len(planned), 'all prebuild dependency commands')
    discovered = set()
    for row, expected in zip(commands, planned):
        c.need(all(row.get(key) == val for key, val in expected.items()) and
               type(row.get('exit_code')) is int and row['exit_code'] == 0 and
               type(row.get('stdout')) is str and type(row.get('stderr')) is str, 'bound dependency command')
        discovered.update(dependencies(row['stdout'], Path(row['cwd'])))
    c.need(discovered == set(before['headers']), 'before header inventory matches dependency streams')
    source_binding(before['headers'], root, manifest)
    source_binding(after['compiled_dependencies'], root, manifest)
    c.need(all(before['headers'].get(p) == h for p, h in after['compiled_dependencies'].items()),
           'compiled dependencies included before build')
    depfiles = after.get('dependency_files')
    c.need(type(depfiles) is dict and depfiles, 'actual dependency files retained')
    used, objects = set(), set()
    for name, text in depfiles.items():
        path = Path(name)
        c.need(path.is_relative_to(build) and '..' not in path.parts and name.endswith('.o.d') and
               type(text) is str and 'CMakeFiles' in path.parts, 'dependency file binding')
        target_part = path.parts.index('CMakeFiles')
        used.update(dependencies(text, Path(*path.parts[:target_part])))
        objects.add(str(path.with_suffix('')))
    c.need(used == set(after['compiled_dependencies']) and objects == set(after['objects']),
           'objects and compiled dependency inventory complete')
    c.need(objects == {job['object'] for job in planned}, 'all expected object jobs closed')
    c.need(set(after['built_archives']) == {str(build / 'baseline/libresident_gen.a')}, 'received fresh GEN archive')
    expected_binaries = binaries(build)
    c.need(type(after.get('binaries')) is dict and set(after['binaries']) == set(expected_binaries),
           'two actual compiled binaries')
    for key, path in expected_binaries.items():
        item = after['binaries'][key]
        c.need(type(item) is dict and set(item) == {'path', 'sha256'} and item['path'] == str(path) and
               type(item['sha256']) is str and re.fullmatch('[0-9a-f]{64}', item['sha256']), 'binary path/hash binding')
    for path in (*after['objects'], *after['built_archives']):
        c.need(Path(path).is_relative_to(build), 'compiled artifact belongs to fresh build')
    return after


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('source-root', 'build', 'manifest', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    result = prepin(args.source_root.resolve(), args.build.resolve(), c.read(args.manifest))
    c.save(args.output, result)
    print(json.dumps(dict(schema=SCHEMA, status='pinned_before_build', jobs=len(result['commands']),
                         headers=len(result['headers']), output_sha256=c.sha(args.output)), sort_keys=True))
