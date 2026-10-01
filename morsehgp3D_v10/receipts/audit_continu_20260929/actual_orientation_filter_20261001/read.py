#!/usr/bin/env python3
"""Read-only TEXT archive judge: hashes first; no compilation/native replay."""
import hashlib
import json
import math
import re
import stat
import sys
from pathlib import Path

SCHEMA = 'orientation_filter_causal_v1'
ORIGIN = '/workspaces/E-HGP/build/v10-b21/src/morsehgp3D_v10/src'
PINS = {
    'tower/orient_filter.hpp': '227ea998f4e14e2210395015740f44e4d62763136838a82abbb02ae27adb6d5f',
    'core/types.hpp': 'd84de8e5376aa41e5ebc4b8b8b5b242401f304e9be11547bb127f55ea6964894',
    'arith/geometry.hpp': '34f7190f70c0e8f875491ff4b23c3dff821f78fe32320fe3af7967d3ed5903cf',
    'arith/wide.hpp': '4af30c625baf144c1d2d30ab7ae787f9008e2b1ee3cf2d54de7ad8e6560d5634',
    'tower/tower.cpp': '3ce0a14a0d6413ca8f7171e6414099334cacdd2ebdf8d13845e7a01020ff9f06',
}
VECTORS = [[202317, -171083, 81913], [261103, 119999, -134873],
           [-130177, 80171, 224033], [157339, -209999, 125003]]
CENTER = [1048576] * 3
FLAGS = ['-std=c++20', '-O2', '-frounding-math', '-ffp-contract=off',
         '-Wall', '-Wextra', '-Wpedantic', '-Werror']
CASES = [{'name': 'baseline', 'mutant': 0, 'ubsan': False},
         {'name': 'mutant', 'mutant': 1, 'ubsan': False},
         {'name': 'baseline_ubsan', 'mutant': 0, 'ubsan': True}]
UBFLAGS = ['-fsanitize=undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
FILES = sorted(['baseline/' + x for x in PINS if x != 'tower/tower.cpp'] +
               ['mutant/tower/orient_filter.hpp', 'probe.cpp', 'record.py', 'read.py',
                'README.md', 'protocol.json', 'preflight.json', 'captures.json', 'source_close.json'])
SEED_FILES = [x for x in FILES if x not in ('captures.json', 'source_close.json')]
DIRS = {'baseline', 'baseline/tower', 'baseline/core', 'baseline/arith', 'mutant', 'mutant/tower'}

def need(ok, why):
    if not ok:
        raise ValueError(why)

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def pairs(items):
    result = {}
    for key, value in items:
        need(key not in result, 'duplicate JSON key: ' + key)
        result[key] = value
    return result

def decode(data):
    def invalid(token):
        raise ValueError('nonfinite JSON token: ' + token)
    return json.loads(data, object_pairs_hook=pairs, parse_constant=invalid)

def protocol(p):
    need(p['schema'] == SCHEMA and p['compiler'] == '/usr/bin/g++', 'protocol/compiler')
    need(p['common_flags'] == FLAGS and p['cases'] == CASES, 'flags/case inventory')
    need(all(set(c) == {'name', 'mutant', 'ubsan'} and type(c['name']) is str and
             type(c['mutant']) is int and type(c['ubsan']) is bool for c in p['cases']), 'case types')
    need(p['vectors'] == VECTORS and p['center'] == CENTER, 'fixed fixtures')
    need(p['rounding_modes'] == ['FE_TONEAREST', 'FE_UPWARD', 'FE_DOWNWARD', 'FE_TOWARDZERO'], 'rounding modes')
    need(type(p['compile_timeout_s']) is int and p['compile_timeout_s'] == 30, 'compile timeout')
    need(type(p['run_timeout_s']) is int and p['run_timeout_s'] == 10, 'run timeout')
    need(type(p['production_rounding_index']) is int and p['production_rounding_index'] == 0, 'production mode')
    need(p['public_status'] == 'not_claimed', 'public status')

def sub(a, b):
    return [x - y for x, y in zip(a, b)]

def cross(a, b):
    return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]

def dot(a, b):
    return sum(x*y for x, y in zip(a, b))

def observation(data, mutant):
    need(set(data) == {'variant', 'rows', 'zero_faces', 'other_faces', 'wrong', 'nearest_wrong', 'expected_observation'}, 'observation fields')
    need(data['variant'] == ('bound_2pow_minus80' if mutant else 'baseline'), 'variant')
    need(type(data['rows']) is list and len(data['rows']) == 64, '64 rows')
    inventory, zeros, wrong, nearest = set(), 0, 0, 0
    for row in data['rows']:
        need(set(row) == {'rounding_index', 'fixture', 'face', 'exact', 'filter', 'wrong'}, 'row fields')
        key = tuple(row[x] for x in ('rounding_index', 'fixture', 'face'))
        need(all(type(x) is int and 0 <= x < 4 for x in key) and key not in inventory, 'row inventory/types')
        inventory.add(key)
        mi, vi, f = key
        u = VECTORS[vi]; v = [u[2], u[0], -u[1]]; t = [-u[1], -u[2], u[0]]
        offsets = [u, [-x for x in u], v, t]
        pts = [[CENTER[i] + z[i] for i in range(3)] for z in offsets]
        need(all(0 <= x < 2**21 for z in pts for x in z), 'B21 fixture domain')
        need(len({dot(z, z) for z in offsets}) == 1, 'exact circumcenter equal distances')
        need(dot(cross(sub(pts[1], pts[0]), sub(pts[2], pts[0])), sub(pts[3], pts[0])) != 0, 'unique circumcenter')
        a, b, c = [pts[(f + i) % 4] for i in (1, 2, 3)]
        value = dot(cross(sub(b, a), sub(c, a)), sub(CENTER, a))
        exact = (value > 0) - (value < 0)
        need(type(row['exact']) is int and row['exact'] == exact, 'Python-int exact orientation')
        need(type(row['filter']) is int and row['filter'] in (-1, 0, 1), 'filter sign')
        bad = row['filter'] != 0 and row['filter'] != exact
        need(type(row['wrong']) is bool and row['wrong'] == bad, 'wrong bit')
        zeros += exact == 0; wrong += bad; nearest += mi == 0 and bad
    need(inventory == {(i, j, k) for i in range(4) for j in range(4) for k in range(4)}, 'closed row product')
    for name, value in [('zero_faces', zeros), ('other_faces', 64-zeros), ('wrong', wrong), ('nearest_wrong', nearest)]:
        need(type(data[name]) is int and data[name] == value, 'summary: ' + name)
    need(zeros == 32 and (nearest >= 1 if mutant else wrong == 0), 'causal observations')
    need(data['expected_observation'] is True, 'expected observation')

def terminal(event, argv, timeout, env):
    need(event['argv'] == argv and event['timeout_s'] == timeout and event['env_overrides'] == env, 'argv/timeout/environment binding')
    need(event['timed_out'] is False and type(event['returncode']) is int and event['returncode'] == 0, 'successful terminal required')
    need(type(event['stdout']) is str and type(event['stderr']) is str and event['stderr'] == '', 'complete clean outputs')
    need(type(event['elapsed_s']) in (int, float) and math.isfinite(event['elapsed_s']) and event['elapsed_s'] >= 0, 'elapsed time')

def check(root, expected_sha):
    need(re.fullmatch('[0-9a-f]{64}', expected_sha) is not None, 'external manifest SHA')
    need(root.is_dir() and not root.is_symlink(), 'regular archive directory')
    allowed = set(FILES + ['manifest.json'])
    found = set()
    for path in root.rglob('*'):
        mode = path.lstat().st_mode
        need(not stat.S_ISLNK(mode), 'archive symlink')
        if stat.S_ISREG(mode):
            found.add(path.relative_to(root).as_posix())
        else:
            need(stat.S_ISDIR(mode) and path.relative_to(root).as_posix() in DIRS, 'archive directory/special file')
    need(found == allowed, 'closed payload inventory')
    need(sha(root/'manifest.json') == expected_sha, 'manifest SHA before parsing')
    manifest = decode((root/'manifest.json').read_text())
    need(set(manifest) == {'schema', 'status', 'files'} and manifest['schema'] == SCHEMA and manifest['status'] == 'closed', 'closed manifest')
    need(type(manifest['files']) is dict and set(manifest['files']) == set(FILES), 'manifest inventory')
    for name, digest in manifest['files'].items():
        need(type(digest) is str and re.fullmatch('[0-9a-f]{64}', digest) is not None and sha(root/name) == digest, 'payload hash: '+name)
    p = decode((root/'protocol.json').read_text()); protocol(p)
    for name, digest in PINS.items():
        if name != 'tower/tower.cpp':
            need(sha(root/'baseline'/name) == digest, 'actor snapshot pin')
    base = (root/'baseline/tower/orient_filter.hpp').read_bytes()
    need(base.count(b'mag * 0x1p-49') == 1, 'unique baseline bound')
    need((root/'mutant/tower/orient_filter.hpp').read_bytes() == base.replace(b'mag * 0x1p-49', b'mag * 0x1p-80', 1), 'unique bound mutation')
    close = decode((root/'source_close.json').read_text())
    refs = {ORIGIN+'/'+x: y for x, y in PINS.items()}
    seed = {x: manifest['files'][x] for x in SEED_FILES}
    need(close == {'schema': SCHEMA, 'status': 'closed', 'before': refs, 'after': refs, 'archive_before': seed, 'archive_after': seed}, 'source before/after closure')
    cap = decode((root/'captures.json').read_text())
    need(cap['schema'] == SCHEMA and cap['status'] == 'closed', 'capture status')
    old, runtime = cap['archive_path'], cap['runtime_path']
    need(type(old) is str and Path(old).is_absolute(), 'recorded archive path')
    need(type(runtime) is str and re.fullmatch('/tmp/mhgp10-orient-runtime-[A-Za-z0-9_-]+', runtime) is not None, 'private runtime path')
    terminal(cap['compiler']['version'], ['/usr/bin/g++', '--version'], 10, {'LC_ALL': 'C'})
    need('Free Software Foundation' in cap['compiler']['version']['stdout'], 'GNU compiler version')
    need(re.fullmatch('[0-9a-f]{64}', cap['compiler']['sha256']) is not None, 'compiler binary metadata')
    need(type(cap['cases']) is list and [x['name'] for x in cap['cases']] == [x['name'] for x in CASES], 'three cases')
    for item, case in zip(cap['cases'], CASES):
        exe = runtime+'/'+case['name']; inc = old+'/mutant' if case['mutant'] else old+'/baseline'
        argv = ['/usr/bin/g++']+FLAGS+(UBFLAGS if case['ubsan'] else [])+['-DORIENT_MUTANT='+str(case['mutant']), '-I'+inc, '-I'+old+'/baseline', old+'/probe.cpp', '-o', exe]
        terminal(item['compile'], argv, 30, {'LC_ALL': 'C'})
        env = {'LC_ALL': 'C'}
        if case['ubsan']:
            env['UBSAN_OPTIONS'] = 'halt_on_error=1:print_stacktrace=1'
        terminal(item['run'], [exe], 10, env)
        need(re.fullmatch('[0-9a-f]{64}', item['binary_sha256']) is not None, 'binary SHA metadata, no archived binary')
        observation(decode(item['run']['stdout']), case['mutant'])
    need(cap['cases'][0]['binary_sha256'] != cap['cases'][1]['binary_sha256'], 'distinct mutant binary')
    for name, digest in manifest['files'].items():
        need(sha(root/name) == digest, 'after-read payload hash')
    need(sha(root/'manifest.json') == expected_sha, 'after-read manifest hash')
    return cap

if __name__ == '__main__':
    try:
        need(len(sys.argv) == 3, 'usage: read.py ARCHIVE EXTERNAL_MANIFEST_SHA256')
        result = check(Path(sys.argv[1]), sys.argv[2])
        print('archive_ok: 3 captures, 192 rows, exact Python-int orientations; no native replay; not_claimed')
    except (OSError, ValueError, TypeError, KeyError) as exc:
        print('REFUS: '+str(exc), file=sys.stderr)
        sys.exit(1)
