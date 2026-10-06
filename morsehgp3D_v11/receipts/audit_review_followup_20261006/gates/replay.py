#!/usr/bin/env python3
"""Relecture stdlib de deux raccords WIP ; ne compile ni ne joue de porte native."""
import argparse
import ast
import difflib
import hashlib
import io
import json
import re
import subprocess
import tarfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--repo', default=str(Path.cwd()))
parser.add_argument('--capture', action='store_true')
args = parser.parse_args()
meta = json.loads((ROOT / 'sources.json').read_text())
captured = {}
for record in meta['wip_captures']:
    raw = (ROOT / record['capture_path']).read_bytes()
    sha = hashlib.sha256(raw).hexdigest()
    if sha != record['sha256'] or sha != record['second_capture_sha256']:
        raise ValueError('WIP snapshot changed')
    captured[record['repo_path']] = raw.decode()
pinned = {}
for pin in sorted({item['pin'] for item in meta['git_dependencies']}):
    records = [item for item in meta['git_dependencies'] if item['pin'] == pin]
    raw = subprocess.check_output(['git', 'archive', pin, '--'] + [item['repo_path'] for item in records], cwd=args.repo)
    with tarfile.open(fileobj=io.BytesIO(raw), mode='r:') as tar:
        for item in records:
            data = tar.extractfile(item['repo_path']).read()
            if hashlib.sha256(data).hexdigest() != item['sha256']:
                raise ValueError('Git dependency changed')
            pinned[(pin, item['repo_path'])] = data.decode()
prefix = 'morsehgp3D_v11/'
cmake = pinned[(meta['dependency_pin'], prefix + 'CMakeLists.txt')]
runner = pinned[(meta['dependency_pin'], prefix + 'tests/mutants/run_mutants.py')]
if 'set(MHGP11_WARNING_OPTIONS -Wall -Wextra -Wpedantic -Werror)' not in cmake:
    raise ValueError('warning flags changed')
if 'add_compile_options(${MHGP11_WARNING_OPTIONS})' not in cmake:
    raise ValueError('warning flag use changed')
if "expected = mutant.setdefault('attendu', 'porte')" not in runner:
    raise ValueError('mutant expected default changed')
if "if failed is not None:\n        return 'INVALIDE', 'le mutant ne passe pas la %s : il doit etre tue par sa porte' % failed, output" not in runner:
    raise ValueError('build-refusal classification changed')
manifest_source = captured[prefix + 'tests/mutants/cli.json']
manifest = json.loads(manifest_source)
mutants = [m for m in manifest['mutants'] if m['id'] == 'sp_internes_gardees']
if len(mutants) != 1:
    raise ValueError('mutant absent or duplicate')
mutant = mutants[0]
source = captured[prefix + mutant['fichier']]
if source.count(mutant['cherche']) != 1 or mutant.get('attendu', 'porte') != 'porte':
    raise ValueError('mutant anchor or expectation changed')
modified = source.replace(mutant['cherche'], mutant['remplace'])
function = re.search(r'bool kept\(u64 i\) const noexcept\s*\{([^{}]*)\}', modified)
if function is None or re.search(r'\bi\b', function.group(1)):
    raise ValueError('unused parameter not reproduced')
fixed_manifest_source = manifest_source.replace(meta['patch_needle'], meta['patch_replacement'])
if hashlib.sha256(fixed_manifest_source.encode()).hexdigest() != meta['proposed_manifest_sha256']:
    raise ValueError('proposed manifest hash')
patch = ''.join(difflib.unified_diff(manifest_source.splitlines(keepends=True), fixed_manifest_source.splitlines(keepends=True),
    fromfile='a/' + prefix + 'tests/mutants/cli.json', tofile='b/' + prefix + 'tests/mutants/cli.json'))
if patch != (ROOT / 'mutant_parameter_use.patch').read_text() or hashlib.sha256(patch.encode()).hexdigest() != meta['patch_sha256']:
    raise ValueError('patch mismatch')
fixed_manifest = json.loads(fixed_manifest_source)
fixed_mutant = next(m for m in fixed_manifest['mutants'] if m['id'] == mutant['id'])
fixed_function = re.search(r'bool kept\(u64 i\) const noexcept\s*\{([^{}]*)\}', source.replace(mutant['cherche'], fixed_mutant['remplace']))
if fixed_function is None or 'static_cast<void>(i);' not in fixed_function.group(1) or 'return true;' not in fixed_function.group(1):
    raise ValueError('minimal proposed fix')


def u18_table(text):
    block = re.search(r'if\(MHGP11_COORD_BITS EQUAL 18\)\n((?:(?!\nendif\(\)).)*?)\nelseif\(MHGP11_COORD_BITS EQUAL 21\)', text, re.S)
    if block is None:
        raise ValueError('u18 profile block')
    rows = re.findall(r'set\(mhgp11_route_(scale8000|scale16000|scale32000|ng00|ng01|ng02)_(file|manifest) ([0-9a-f]{16})\)', block.group(1))
    if len(rows) != 12:
        raise ValueError('u18 golden count')
    return {name + '_' + kind: value for name, kind, value in rows}


api_cmake = captured[prefix + 'tests/api/tests.cmake']
old_cmake = pinned[(meta['old_goldens_pin'], prefix + 'tests/api/tests.cmake')]
current, old = u18_table(api_cmake), u18_table(old_cmake)
if current != old:
    raise ValueError('u18 references no longer match historical v1')
writer = captured[prefix + 'src/api/write_supports.cpp']
published = captured[prefix + 'src/api/manifest.cpp']
if 'const std::array<u64, 16> words = {2,' not in writer or '\\"format\\":\\"MHGP11SP\\",\\"version\\":2' not in published:
    raise ValueError('SPv2 publication source changed')
old_writer = pinned[(meta['old_goldens_pin'], prefix + 'src/api/write_supports.cpp')]
old_manifest = pinned[(meta['old_goldens_pin'], prefix + 'src/api/manifest.cpp')]
if 'const std::array<u64, 16> words = {1,' not in old_writer or 'MHGP11SP\\",\\"version\\":1' not in old_manifest:
    raise ValueError('historical SPv1 source')
cases = []
for text in re.findall(r'"([^"\n]+)"', api_cmake):
    words = text.split(';')
    if len(words) == 6 and words[0] in ('scale8000', 'scale16000', 'scale32000', 'lidar'):
        cases.append(words)
if len(cases) != 6:
    raise ValueError('six diagnostic cases')
commands = []
for label, n, input_option, counts, balls, cells in cases:
    name = input_option.removeprefix('data=lidar_') if label == 'lidar' else label
    commands.append(dict(case=name, sites=int(n),
        argv=['{build}/mhgp11_api_supports_route_probe', '--work={out}/u18_spv2_' + name,
              '--k=5', '--workers=1,4', '--' + input_option, '--min-balls=1', '--min-cells=1'],
        scope='Diagnostic collection after guarded G4 build of final corrected pushed source; not an execution or qualification in this audit.'))
result = dict(schema='audit_sp_v2_gate_followup_replay_v1', base_pin=meta['base_pin'], double_capture_equal=True,
    mutant=dict(id=mutant['id'], gate=mutant['porte'], manifest_mutants=len(manifest['mutants']), manifest_floor=manifest['plancher'],
        original_mutated_body=function.group(1).strip(), corrected_mutated_body=fixed_function.group(1).strip(),
        unused_parameter='i', compiler_outcome='Expected -Wunused-parameter under -Wextra/-Werror; not compiled by auditor.',
        runner_build_refusal='INVALIDE, never a semantic oracle kill',
        rebase_condition='Applies only while kept(u64 i) and its current search needle exist. Rebase after actual Kruskal selector integration if it changes.'),
    u18=dict(historical_pin=meta['old_goldens_pin'], profile=18, pairs=6, fields=12,
        current_exactly_historical=True, values=current, binary_version_now=2, manifest_version_now=2,
        outcome='References still historical SPv1; six SPv2 references require guarded native regeneration. No SPv2 digest predicted or native failure observed.'),
    diagnostic_commands=commands, native_runs=0, cloud_actions=0)
output = json.dumps(result, indent=2, sort_keys=True) + '\n'
if args.capture:
    (ROOT / 'summary.json').write_text(output)
elif (ROOT / 'summary.json').read_text() != output:
    raise ValueError('summary changed')
print(output, end='')
