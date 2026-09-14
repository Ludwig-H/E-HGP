#!/usr/bin/env python3
"""Reproduce a sibling-certificate audit from the closed f7edd646 snapshot."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import difflib
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import zipfile

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]
ARCHIVE = BASE.parent / 'q2_front_20260914/r1_sources.zip'
ARCHIVE_SHA = '29bfa1b40eed6e6fa7f8167c9868067ceec5ebd4e6b856f9ef26cfa928e96ef3'
SOURCE_COMMIT = 'f7edd6463adfeba559f305d14361bf85a8c25703'
PREFIX = 'morsehgp3D_v8/'
FLAGS = ['-std=c++20', '-O2', '-DNDEBUG', '-Wall', '-Wextra', '-Wpedantic', '-Werror']


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def stamp():
    return datetime.now(timezone.utc).isoformat()


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True,
                               allow_nan=False) + '\n')


def replace_once(source, before, after):
    require(source.count(before) == 1, 'patch anchor missing or ambiguous: ' + before[:100])
    return source.replace(before, after, 1)


SIBLING_TEST = '''    // Audit-only early rejection. The child and sibling IDs belong to the
    // same query tree and its b_order, not necessarily the witness Z tree.
    // The exact inherited prefix counted only points outside their parent B:
    // otherwise evaluating b=z would contradict the strict uniform H>0 test.
    // Thus Remaining may combine the disjoint prefix with the sibling ONLY
    // for immediate saturation. Failed tests change neither count nor cursor.
    if constexpr (Sibling != 0) {
      if (sibling != absent) {
        counter_add(sibling_work->child_entries);
        const auto sibling_node = query_node(sibling);
        const auto needed = Sibling == 2 ? threshold - count : threshold;
        if (sibling_node.range.size() >= needed) {
          counter_add(sibling_work->population_eligible);
          counter_add(sibling_work->bound_tests);
          const auto sibling_bounds = singleton ? pair_bounds(key, sibling_node.box)
              : prepared.bounds_unchecked(sibling_node.box);
          if (sibling_bounds.minimum4 > 0) {
            counter_add(sibling_work->rejected_children);
            counter_add(sibling_work->rejected_pairs, static_cast<u64>(b.range.size()));
            reject(b.range);
            return;
          }
        }
      }
    }
'''

DISPATCH = '''namespace audit {

WspdQ2CensusResult run_sibling_census(
    const Q2CensusIndex& index, unsigned kmax, unsigned separation_s,
    WspdFrontMode front_mode, Q2CensusMode census_mode,
    const Q2CensusConsumer& consumer, SiblingMode mode, SiblingWork& work) {
  work = {};
  if (census_mode != Q2CensusMode::SharedBlocks) {
    throw std::invalid_argument("audit sibling entry supports SharedBlocks only");
  }
  switch (mode) {
    case SiblingMode::Baseline:
      return run_wspd_q2_census(index, kmax, separation_s, front_mode, census_mode, consumer);
    case SiblingMode::Autonomous:
      return run_audit_sibling<1>(index, kmax, separation_s, front_mode, census_mode, consumer, work);
    case SiblingMode::Remaining:
      return run_audit_sibling<2>(index, kmax, separation_s, front_mode, census_mode, consumer, work);
  }
  throw std::invalid_argument("invalid audit sibling mode");
}

Q2CensusResult sibling_group_fixture(
    const Q2CensusIndex& index, std::size_t a_id,
    std::span<const std::size_t> b_ids, unsigned kmax,
    const Q2CensusConsumer& consumer, SiblingMode mode, SiblingWork& work) {
  work = {};
  if (a_id >= index.cloud().points().size() || b_ids.empty() ||
      kmax == 0 || kmax > 10 || !consumer) {
    throw std::invalid_argument("invalid audit group fixture");
  }
  // Fixture-only validation/copy, outside every measured WSPD path.
  std::vector<std::size_t> ids(b_ids.begin(), b_ids.end());
  std::sort(ids.begin(), ids.end());
  if (ids.back() >= index.cloud().points().size() ||
      std::binary_search(ids.begin(), ids.end(), a_id) ||
      std::adjacent_find(ids.begin(), ids.end()) != ids.end()) {
    throw std::invalid_argument("audit group has invalid, duplicate or overlapping IDs");
  }
  Q2CensusEngine engine(index, kmax, consumer);
  engine.b_order = b_ids;
  engine.shared_queries = {};
  static_cast<void>(engine.build_queries({0, b_ids.size()}, 0));
  engine.result.candidate_pairs = static_cast<u64>(b_ids.size());
  engine.result.work.input_descriptors = 1;
  engine.root_start(0);
  switch (mode) {
    case SiblingMode::Baseline: engine.shared_task(a_id, 0, 0, 0); break;
    case SiblingMode::Autonomous: engine.shared_task<1>(a_id, 0, 0, 0, &work); break;
    case SiblingMode::Remaining: engine.shared_task<2>(a_id, 0, 0, 0, &work); break;
    default: throw std::invalid_argument("invalid audit sibling mode");
  }
  return engine.result;
}

}  // namespace audit

'''

GATE_WRAPPER = '''#include "audit_sibling.hpp"
#include <cstdlib>

// Explicitly adapted from the pinned production gate; Pairwise is unchanged.
// The environment selects one audit specialization for every SharedBlocks run.
mhgp8::WspdQ2CensusResult audit_gate_run(
    const mhgp8::Q2CensusIndex& index, unsigned k, unsigned s,
    mhgp8::WspdFrontMode front, mhgp8::Q2CensusMode census,
    const mhgp8::Q2CensusConsumer& consumer) {
  const char* selected = std::getenv("MHGP8_AUDIT_SIBLING_MODE");
  const std::string_view mode = selected == nullptr ? "baseline" : selected;
  if (mode == "baseline" || census != mhgp8::Q2CensusMode::SharedBlocks) {
    return mhgp8::run_wspd_q2_census(index, k, s, front, census, consumer);
  }
  if (mode != "sibling" && mode != "sibling_remaining") {
    throw std::invalid_argument("invalid audit gate mode");
  }
  mhgp8::audit::SiblingWork work;
  return mhgp8::audit::run_sibling_census(index, k, s, front, census, consumer,
      mode == "sibling" ? mhgp8::audit::SiblingMode::Autonomous
                        : mhgp8::audit::SiblingMode::Remaining, work);
}

'''


def patch_census(original):
    source = replace_once(original, '#include "q2_census.hpp"',
                          '#include "q2_census.hpp"\n#include "audit_sibling.hpp"')
    source = replace_once(source,
        '  void shared_task(std::size_t a_id, std::size_t query, unsigned count,\n'
        '                    std::size_t cursor) {',
        '  template <unsigned Sibling = 0>\n'
        '  void shared_task(std::size_t a_id, std::size_t query, unsigned count,\n'
        '                    std::size_t cursor, audit::SiblingWork* sibling_work = nullptr,\n'
        '                    std::size_t sibling = absent) {')
    source = replace_once(source,
        '      prepared = Q2PreparedBounds(points[a_id], b.box);\n    }\n    while (cursor',
        '      prepared = Q2PreparedBounds(points[a_id], b.box);\n    }\n'
        + SIBLING_TEST + '    while (cursor')
    source = replace_once(source,
        '        shared_task(a_id, b.left, count, cursor);\n'
        '        shared_task(a_id, b.right, count, cursor);',
        '        if constexpr (Sibling == 0) {\n'
        '          shared_task(a_id, b.left, count, cursor);\n'
        '          shared_task(a_id, b.right, count, cursor);\n'
        '        } else {\n'
        '          shared_task<Sibling>(a_id, b.left, count, cursor, sibling_work, b.right);\n'
        '          shared_task<Sibling>(a_id, b.right, count, cursor, sibling_work, b.left);\n'
        '        }')
    begin = original.index('WspdQ2CensusResult run_wspd_q2_census(')
    end = original.index('\n}  // namespace mhgp8', begin)
    # Explicitly clone only this pinned adapter; the original remains intact.
    extension = original[begin:end]
    extension = replace_once(extension, 'WspdQ2CensusResult run_wspd_q2_census(',
                             'template <unsigned Sibling>\nWspdQ2CensusResult run_audit_sibling(')
    extension = replace_once(extension, 'const Q2CensusConsumer& consumer) {',
                             'const Q2CensusConsumer& consumer, audit::SiblingWork& work) {')
    extension = replace_once(extension, 'engine.shared_task(a_id, b_node, 0, 0);',
                             'engine.shared_task<Sibling>(a_id, b_node, 0, 0, &work);')
    return replace_once(source, '}  // namespace mhgp8', extension + DISPATCH + '}  // namespace mhgp8')


def patch_probe(source):
    source = '// Explicit adaptation of archive-pinned lidar_q2_probe.cpp, SHA256\n' \
        '// 217b2bca2956e75df52a4ad1877959a5bd3e2078384b4c06faabf7c0123e8f99.\n' \
        '// Digest/checks unchanged. Only isolated SharedBlocks sibling modes added.\n' + source
    source = replace_once(source, '#include "pipeline/wspd_q2_census.hpp"',
                          '#include "pipeline/wspd_q2_census.hpp"\n#include "audit_sibling.hpp"')
    source = replace_once(source,
        'require(argc == 6, "usage: lidar_q2_probe input.u16le Kmax s pure|samples pairwise|shared");',
        'require(argc == 7, "usage: lidar_sibling_probe input.u16le Kmax s pure|samples shared baseline|sibling|sibling_remaining");')
    source = replace_once(source,
        'const std::string_view front_mode(argv[4]), census_mode(argv[5]);',
        'const std::string_view front_mode(argv[4]), census_mode(argv[5]), sibling_mode(argv[6]);\n'
        '  require(census_mode == "shared" && (sibling_mode == "baseline" || sibling_mode == "sibling" ||\n'
        '          sibling_mode == "sibling_remaining"), "unsupported sibling parameters");')
    source = replace_once(source, 'const auto result = run_wspd_q2_census(*index, k, s,',
        'audit::SiblingWork sibling_work;\n'
        '  const auto audit_mode = sibling_mode == "baseline" ? audit::SiblingMode::Baseline\n'
        '      : sibling_mode == "sibling" ? audit::SiblingMode::Autonomous : audit::SiblingMode::Remaining;\n'
        '  const auto result = audit::run_sibling_census(*index, k, s,')
    source = replace_once(source,
        '[&](const Q2Support& support) { digest.consume(support, cloud->points(), k); });',
        '[&](const Q2Support& support) { digest.consume(support, cloud->points(), k); }, audit_mode, sibling_work);')
    source = replace_once(source, '#undef EMIT',
        '  std::cout << "},\\\"sibling_mode\\\":\\\"" << sibling_mode << "\\\",\\\"sibling_work\\\":{";\n'
        '  {\n    Fields fields;\n'
        '    EMIT(sibling_work, child_entries); EMIT(sibling_work, population_eligible); EMIT(sibling_work, bound_tests);\n'
        '    EMIT(sibling_work, rejected_children); EMIT(sibling_work, rejected_pairs);\n  }\n#undef EMIT')
    return source


def build(name, compiler):
    require(name and all(c.isalnum() or c == '_' for c in name), 'invalid capture name')
    receipt = BASE / (name + '_BUILD.json')
    snapshot = BASE / '.snapshot' / name
    output = BASE / '.build' / name
    require(not receipt.exists() and not snapshot.exists() and not output.exists(),
            'refuse to overwrite a previous attempt')
    require(sha(ARCHIVE) == ARCHIVE_SHA, 'source archive changed')
    snapshot.mkdir(parents=True)
    output.mkdir(parents=True)
    record = dict(schema='mhgp8_q2_sibling_build_v1', status='running', name=name,
                  source_commit=SOURCE_COMMIT, archive=str(ARCHIVE.relative_to(ROOT)),
                  archive_sha256=ARCHIVE_SHA, command=sys.argv, compiler=compiler,
                  flags=FLAGS, platform=platform.platform(), started_utc=stamp(),
                  modes=['baseline', 'sibling', 'sibling_remaining'], steps=[], binaries={})
    inputs = [Path(__file__), BASE / 'audit_sibling.hpp', BASE / 'sibling_fixture.cpp']
    record['audit_inputs'] = {str(p.relative_to(ROOT)): sha(p) for p in inputs}
    write(receipt, record)

    def execute(label, command, expected=0):
        step = dict(label=label, command=command, started_utc=stamp(), expected_returncode=expected)
        record['steps'].append(step)
        write(receipt, record)
        try:
            run = subprocess.run(command, capture_output=True, text=True, timeout=180)
            step.update(returncode=run.returncode, stdout=run.stdout, stderr=run.stderr)
        except subprocess.TimeoutExpired as error:
            step.update(returncode=124, stdout=(error.stdout or b'').decode(errors='replace'),
                        stderr=(error.stderr or b'').decode(errors='replace'), timeout=True)
        finally:
            step['finished_utc'] = stamp()
            write(receipt, record)
        require(step['returncode'] == expected, 'step failed: ' + label)
        print(label + ': passed', flush=True)

    try:
        execute('compiler_version', [compiler, '--version'])
        originals = {}
        with zipfile.ZipFile(ARCHIVE) as archive:
            for member in archive.namelist():
                require(member.startswith(PREFIX) and '..' not in Path(member).parts,
                        'unsafe archive member')
                data = archive.read(member)
                originals[member] = data
                target = snapshot / member
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(data)
        record['original_sources'] = {p: hashlib.sha256(data).hexdigest()
                                      for p, data in sorted(originals.items())}
        census_path = snapshot / PREFIX / 'src/pipeline/q2_census.cpp'
        census_path.write_text(patch_census(census_path.read_text()))
        header_path = snapshot / 'audit_sibling.hpp'
        header_path.write_bytes((BASE / 'audit_sibling.hpp').read_bytes())
        fixture_path = snapshot / 'sibling_fixture.cpp'
        fixture_path.write_bytes((BASE / 'sibling_fixture.cpp').read_bytes())
        probe_path = snapshot / 'lidar_sibling_probe.cpp'
        probe_path.write_text(patch_probe(originals[PREFIX + 'audits/q2_front_20260914/lidar_q2_probe.cpp'].decode()))
        gate_path = snapshot / 'sibling_product_gate.cpp'
        gate = originals[PREFIX + 'tests/wspd_q2_census_gate.cpp'].decode()
        gate = gate.replace('mhgp8::run_wspd_q2_census(', 'audit_gate_run(')
        gate = replace_once(gate, 'namespace {', GATE_WRAPPER + 'namespace {')
        gate_path.write_text(gate)
        files = sorted(p for p in snapshot.rglob('*') if p.is_file())
        record['adapted_sources'] = {str(p.relative_to(snapshot)): sha(p) for p in files}
        difference = ''.join(difflib.unified_diff(
            originals[PREFIX + 'src/pipeline/q2_census.cpp'].decode().splitlines(True),
            census_path.read_text().splitlines(True), fromfile='pinned/q2_census.cpp',
            tofile='audit/q2_census.cpp'))
        (BASE / (name + '_census.patch')).write_text(difference)
        source_zip = BASE / (name + '_sources.zip')
        with zipfile.ZipFile(source_zip, 'x', compression=zipfile.ZIP_DEFLATED) as archive:
            for p in files:
                archive.write(p, str(p.relative_to(snapshot)))
        record['adapted_archive'] = str(source_zip.relative_to(ROOT))
        record['adapted_archive_sha256'] = sha(source_zip)
        src = snapshot / PREFIX / 'src'
        common = [str(src / p) for p in ('pipeline/axis_q2.cpp', 'pipeline/local_credits.cpp',
                  'pipeline/prepared_cloud.cpp', 'pipeline/q2_census.cpp', 'wspd/front.cpp')]
        # Compile shared sources once, with identical flags for every audit mode.
        objects = []
        for source in common:
            obj = output / (Path(source).stem + '.o')
            execute('compile_' + obj.stem, [compiler, *FLAGS, '-I', str(src), '-I', str(snapshot),
                                           '-c', source, '-o', str(obj)])
            objects.append(str(obj))
        for label, source in (('probe', probe_path), ('gate', gate_path), ('fixture', fixture_path)):
            binary = output / ('lidar_sibling_probe' if label == 'probe' else label)
            execute('link_' + label, [compiler, *FLAGS, '-I', str(src), '-I', str(snapshot),
                                      str(source), *objects, '-o', str(binary)])
            record['binaries'][label] = dict(path=str(binary.relative_to(ROOT)), sha256=sha(binary))
            write(receipt, record)
        for mode in record['modes']:
            execute('product_gate_' + mode, ['env', 'MHGP8_AUDIT_SIBLING_MODE=' + mode,
                    str(output / 'gate'), '--selftest'])
        execute('sibling_fixture', [str(output / 'fixture')])
        # A tiny physical LE input verifies all new CLI modes and unchanged digests.
        tiny = output / 'tiny.u16le'
        tiny.write_bytes(b''.join(int(c).to_bytes(2, 'little')
                         for x in range(16) for c in (x, 0, 0)))
        digests = []
        for mode in record['modes']:
            execute('probe_' + mode, [str(output / 'lidar_sibling_probe'), str(tiny),
                                     '10', '8', 'samples', 'shared', mode])
            result = json.loads(record['steps'][-1]['stdout'])
            require(result['sibling_mode'] == mode, 'probe mode mismatch')
            digests.append(result['digest'])
        require(digests[0] == digests[1] == digests[2], 'probe digests differ')
        require(all(sha(ROOT / p) == pin for p, pin in record['audit_inputs'].items()),
                'audit input changed during build')
        require(all(sha(snapshot / p) == pin for p, pin in record['adapted_sources'].items()),
                'adapted source changed during build')
        record['status'] = 'passed'
    except BaseException as error:
        record.update(status='interrupted' if isinstance(error, KeyboardInterrupt) else 'failed',
                      error_type=type(error).__name__, error=str(error))
        raise
    finally:
        record['finished_utc'] = stamp()
        write(receipt, record)
    print(json.dumps(dict(status=record['status'], receipt=str(receipt),
                          binaries=record['binaries'])), flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name', default='r1')
    parser.add_argument('--compiler', default='g++')
    arguments = parser.parse_args()
    build(arguments.name, arguments.compiler)
