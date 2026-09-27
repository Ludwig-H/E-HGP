#!/usr/bin/env python3
"""Fresh attachment build through the frozen builder library, not old objects.

One checked rename of the frozen weighted export's own main is generated in
a private sibling directory. The native AObservedBuilder is included intact.
CPLUS_INCLUDE_PATH supplies these explicit adapter include directories; the
frozen builder's full compiler dependency discovery pins their contents.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path

HERE = Path(__file__).resolve().parent
V9 = HERE.parents[1]
EXPECTED = {
    'native_weighted_export.cpp': '7036fc56bfa6c7b092c6d640f69875226a8a6365f2b5083a4de58def1700a71a',
    'build_native.py': '25f281101aef3bf0259302c5204cd2a7f1424df32a4c8c1c5c8c6f84559ef6bf',
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    with path.open('x') as stream:
        json.dump(value, stream, indent=2, sort_keys=True, allow_nan=False)
        stream.write('\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--compiler', default='g++')
    parser.add_argument('--jobs', type=int, choices=range(1, 5), default=2)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    args.build = args.build.resolve()
    adapter = args.build.with_name(args.build.name + '-adapter')
    if args.build.exists() or adapter.exists() or not args.build.parent.is_dir():
        raise RuntimeError('fresh build and adapter sibling required')
    if args.build.is_relative_to(V9):
        raise RuntimeError('private build outside v9 source required')
    for name, expected in EXPECTED.items():
        if sha(HERE/name) != expected:
            raise RuntimeError('frozen dependency changed: ' + name)
    pins = {str(path): sha(path) for path in [Path(__file__), HERE/'native_attachment_export.cpp',
            *(HERE/name for name in EXPECTED), V9/'src/tower/forest/full_ball_tower.hpp',
            V9/'audits/b_full_a_manifest_20260927/native_a.hpp',
            V9/'audits/b_full_a_manifest_20260927/capture.hpp']}
    expected_header = '124d3e9b52b1e6155e5a2ffd2a32cda7e5b403dda21155da2949b9ca8c90c0d0'
    if pins[str(V9/'src/tower/forest/full_ball_tower.hpp')] != expected_header:
        raise RuntimeError('observed Builder no longer matches native header')
    original = (HERE/'native_weighted_export.cpp').read_text()
    before = 'int main(int argc,char** argv) {'
    after = 'int mhgp9_frozen_weighted_export_main(int argc,char** argv) {'
    if original.count(before) != 1:
        raise RuntimeError('main rename substitution cardinality')
    adapter.mkdir()
    generated = adapter/'native_weighted_export_renamed.hpp'
    with generated.open('x') as stream:
        stream.write(original.replace(before, after))
    unit = adapter/'native_weighted_export.cpp'
    with unit.open('x') as stream:
        stream.write('#include "native_attachment_export.cpp"\n')
    previous = os.environ.get('CPLUS_INCLUDE_PATH')
    # Deliberately replace, do not inherit hidden extra include search paths.
    includes = os.pathsep.join(map(str, [adapter, HERE, V9/'src']))
    os.environ['CPLUS_INCLUDE_PATH'] = includes
    receipt = dict(schema='mhgp9_attachment_build_wrapper_v1', status='started', pins_before=pins,
                   generated_main_rename=dict(before=before, after=after, count=1),
                   generated_sha256={str(p):sha(p) for p in (generated, unit)},
                   CPLUS_INCLUDE_PATH=includes, inherited_include_path_ignored=previous is not None)
    save(adapter/'inputs.json', receipt)
    try:
        spec = importlib.util.spec_from_file_location('frozen_weighted_build', HERE/'build_native.py')
        library = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(library)
        # Only the entry unit directory changes. ROOT/V9, compiler flags,
        # dependency checks and exclusive build/receipt rules stay frozen.
        library.HERE = adapter
        library.build(args)
        receipt.update(status='completed', binary=str(args.build/'native_weighted_export'),
                       binary_sha256=sha(args.build/'native_weighted_export'))
    except BaseException as error:
        receipt.update(status='failed', error=repr(error))
        raise
    finally:
        if previous is None:
            os.environ.pop('CPLUS_INCLUDE_PATH', None)
        else:
            os.environ['CPLUS_INCLUDE_PATH'] = previous
        receipt['pins_after'] = {path:sha(Path(path)) for path in pins}
        receipt['pins_match'] = receipt['pins_after'] == pins
        if not receipt['pins_match']:
            receipt['status'] = 'failed'
        save(adapter/'receipt.json', receipt)
        if not receipt['pins_match']:
            raise RuntimeError('attachment inputs changed during build')


if __name__ == '__main__':
    main()
