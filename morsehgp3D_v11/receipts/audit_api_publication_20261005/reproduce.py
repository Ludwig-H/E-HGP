#!/usr/bin/env python3
"""Bounded manifest compatibility proof; no native run or cloud call.

Imports the frozen official reader without alteration. Builds a real one-site
MHGP11FUL1 payload in Python, accepted by that reader. Tests only declared
Provenance metadata mutations against the exact same payload and counts.
The payload is a schema fixture, not output of a native publish() invocation.
"""
import copy
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / 'source' / 'morsehgp3D_v11'
sys.path.insert(0, str(SOURCE / 'bench'))
import mhgp11_formats as formats


def require(value, message):
    if not value:
        raise RuntimeError(message)


def encode(manifest):
    return (json.dumps(manifest, separators=(',', ':'), ensure_ascii=True) + '\n').encode('ascii')


def words(*values):
    return struct.pack('<%dQ' % len(values), *values)


def integer(value):
    return words(int(value < 0), 1, abs(value))


def main():
    captured = json.loads((ROOT / 'source_manifest.json').read_text())
    for item in captured['files']:
        require(hashlib.sha256((ROOT / 'source' / item['path']).read_bytes()).hexdigest() == item['sha256'],
                'captured source changed: ' + item['path'])

    # A unit-weight site at (0, 0, 0), PointId 0, order 1: one root birth.
    payload = b'MHGP11FUL1' + words(21, 1, 1, 1) + words(0, 0, 0, 1, 0) + words(1, 1, 1, 0, 0)
    payload += words((1 << 32) - 1, 0, 0) + integer(0) + integer(1)
    payload += integer(0) + integer(0) + integer(0) + integer(1)
    point_input, id_input = struct.pack('<3I', 0, 0, 0), struct.pack('<I', 0)
    sha = lambda raw: hashlib.sha256(raw).hexdigest()
    fixture = dict(
        schema=formats.SCHEMA, output='full', status='complete', public_status='not_claimed', coord_bits=21, k=1,
        parameters=dict(budget_bytes=None, grid_step=None, origin=None),
        inputs=[dict(name='points', bytes=len(point_input), sha256=sha(point_input)),
                dict(name='ids', bytes=len(id_input), sha256=sha(id_input))],
        files=[dict(name=formats.FULL_NAME, format='MHGP11FUL1', version=1, bytes=len(payload), sha256=sha(payload))],
        # This syntactically valid field is not recomputed by this manifest reader.
        tree_k_sha256=sha(b'one-point manifest compatibility schema fixture'),
        counts=dict(sites=1, points=1, orders=[dict(k=1, births=1, nodes=1, edges=0, root=0)]))

    cases = [('valid_provenance', fixture, None)]
    variant = copy.deepcopy(fixture)
    variant['inputs'][0]['bytes'] = 0
    variant['inputs'][1]['bytes'] = 0
    cases.append(('default_provenance_zero_sizes', variant, "manifeste : points et octets d'entree"))
    variant = copy.deepcopy(fixture)
    variant['inputs'][0]['bytes'] = 11
    cases.append(('inconsistent_input_ratio', variant, 'manifeste : 12 et 4 octets par point'))
    variant = copy.deepcopy(fixture)
    variant['inputs'][0]['bytes'] = 24
    variant['inputs'][1]['bytes'] = 8
    cases.append(('consistent_ratio_wrong_point_count', variant, "manifeste : points et octets d'entree"))
    variant = copy.deepcopy(fixture)
    variant['parameters']['budget_bytes'] = 0
    cases.append(('declared_zero_budget', variant, 'manifeste : budget_bytes'))

    results = []
    with tempfile.TemporaryDirectory(prefix='v11-s5-reader-fixture-') as temp:
        directory = Path(temp) / 'output'
        directory.mkdir()
        (directory / formats.FULL_NAME).write_bytes(payload)
        for name, manifest, expected in cases:
            raw = encode(manifest)
            (ROOT / ('fixture_' + name + '.json')).write_bytes(raw)
            (directory / formats.MANIFEST).write_bytes(raw)
            try:
                checked = formats.check_directory(str(directory), 21)
            except ValueError as error:
                observed = str(error)
                require(expected is not None and observed == expected, 'unexpected refusal for ' + name)
                results.append(dict(case=name, official_reader='refused', reason=observed,
                                    manifest_sha256=sha(raw)))
            else:
                require(expected is None, 'mutation accepted: ' + name)
                require(checked['decoded']['sites'] == 1 and checked['decoded']['bytes'] == len(payload),
                        'fixture binary unexpectedly decoded')
                results.append(dict(case=name, official_reader='accepted', decoded_sites=1,
                                    manifest_sha256=sha(raw)))

    # Checks the relevant native source condition is unchanged, without claiming
    # to execute its C++ body. All mutations keep grid_step and origin empty.
    source = (SOURCE / 'src/api/manifest.cpp').read_text()
    begin = source.index('Outcome check_provenance(const Provenance& provenance) noexcept {')
    end = source.index('\n}\n', begin) + 2
    check_body = source[begin:end]
    expected_body = '''Outcome check_provenance(const Provenance& provenance) noexcept {
  if (!provenance.grid_step.empty() && !api::valid_grid_step(provenance.grid_step))
    return fail(Reason::parameter_out_of_range);
  const bool declared = !provenance.origin[0].empty();
  for (const std::string_view axis : provenance.origin)
    if (declared ? !api::valid_origin_coordinate(axis) : !axis.empty()) return fail(Reason::parameter_out_of_range);
  return {};
}'''
    require(check_body == expected_body, 'native pre-I/O provenance validation changed')
    print(json.dumps(dict(
        schema='ehgp.v11.audit.s5_provenance_reader.v1', source_kind=captured['source_kind'], head=captured['head'],
        native_runs=0, cloud_calls=0, fixture_kind='python_one_site_full_schema_fixture',
        fixture_full_bytes=len(payload), fixture_full_sha256=sha(payload),
        frozen_check_provenance_sha256=sha(check_body.encode()),
        source_trace='pre-I/O check validates decimal declarations only; all tested variants reach its success return',
        results=results), sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
