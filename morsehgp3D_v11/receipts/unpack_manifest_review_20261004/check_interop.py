#!/usr/bin/env python3
"""Bounded interoperation check on frozen AST and 16 synthetic bytes, no tar extraction."""
import ast
import hashlib
import json
from pathlib import Path
from types import SimpleNamespace


BASE = Path(__file__).resolve().parent
PIN = 'f1a53fe1ccc121bc80736473753ca156b0efee46'
HASHES = {
    'points_unpack.py': '7f683f36bf8453deb6a292ecdeda84c9da1d8d4fb202f8a498bcd4ddf7bfac55',
    'points_lidar_prepare.py': '5911b13b85e97821603e613927cc7b82229bfd5c44375108cef1a76d602edbe9',
}
checks = 0


def guard(value, reason):
    global checks
    checks += 1
    if not value:
        raise RuntimeError(reason)


def tree(name):
    path = BASE / 'source' / name
    raw = path.read_bytes()
    guard(hashlib.sha256(raw).hexdigest() == HASHES[name], 'changed frozen source ' + name)
    return ast.parse(raw.decode())


def run(nodes, namespace, filename):
    exec(compile(ast.Module(body=nodes, type_ignores=[]), filename, 'exec'), namespace)


def main():
    unpack, prepare = tree('points_unpack.py'), tree('points_lidar_prepare.py')
    sha = [n for n in unpack.body if isinstance(n, ast.FunctionDef) and n.name == 'sha']
    guard(len(sha) == 1, 'one actual digest function')
    ns = {'Path': Path, 'hashlib': hashlib,
          'args': SimpleNamespace(out=BASE / 'synthetic', scenes=BASE / 'synthetic'),
          'receipt': {'sites': 1}, 'name': 'tiny', 'scenes': []}
    run(sha, ns, 'source/points_unpack.py')
    # Execute only the actual scene-manifest append expression, without prepare_frame/native work.
    appends = []
    for n in ast.walk(prepare):
        if not isinstance(n, ast.Expr) or not isinstance(n.value, ast.Call):
            continue
        c = n.value
        if isinstance(c.func, ast.Attribute) and isinstance(c.func.value, ast.Name) and \
                c.func.value.id == 'scenes' and c.func.attr == 'append' and len(c.args) == 1:
            d = c.args[0]
            if isinstance(d, ast.Call) and isinstance(d.func, ast.Name) and d.func.id == 'dict' and \
                    any(k.arg == 'kind' and isinstance(k.value, ast.Constant) and
                        k.value.value == 'criblage_voisin' for k in d.keywords):
                appends.append(n)
    guard(len(appends) == 1, 'one actual new-neighbour manifest append')
    run(appends, ns, 'source/points_lidar_prepare.py')
    scene = ns['scenes'][0]
    guard('sites_sha256' in scene and 'labels_sha256' not in scene, 'producer omits label digest')
    loops = [n for n in ast.walk(unpack) if isinstance(n, ast.For) and
             isinstance(n.target, ast.Name) and n.target.id == 'scene' and
             isinstance(n.iter, ast.Subscript) and isinstance(n.iter.value, ast.Name) and
             n.iter.value.id == 'manifest']
    guard(len(loops) == 1, 'one actual manifest checker loop')
    def validate(s):
        local = dict(ns, manifest={'scenes': [s]}, bad=[])
        run(loops, local, 'source/points_unpack.py')
        return local['bad']
    initial = validate(scene)
    guard(initial == ['tiny_labels.u32le'], 'valid synthetic payload rejected for missing label digest')
    proposed = dict(scene, labels_sha256=ns['sha'](BASE / 'synthetic/tiny_labels.u32le'))
    accepted = validate(proposed)
    guard(accepted == [], 'adding actual label digest closes the interoperation mismatch')
    wrong = validate(dict(proposed, labels_sha256='0' * 64))
    guard(wrong == ['tiny_labels.u32le'], 'strict consumer still rejects a wrong label digest')
    historical = json.loads((BASE / 'source/data_manifest_pts3.json').read_text())['scenes']
    names = [s['name'] for s in historical]
    guard(len(names) == len(set(names)), 'historical names distinct')
    guard(all(isinstance(s.get(key), str) and len(s[key]) == 64 and
              all(c in '0123456789abcdef' for c in s[key]) for s in historical
              for key in ('sites_sha256', 'labels_sha256')), 'historical digest fields available')
    print(json.dumps({'status': 'PASS', 'pin': PIN, 'guards': checks,
                     'producer_fields': sorted(scene), 'missing_field': 'labels_sha256',
                     'consumer_bad_before': initial, 'consumer_bad_after_proposed_field': accepted,
                     'consumer_bad_wrong_label_hash': wrong, 'historical_manifest_scenes': len(historical),
                     'synthetic_payload_bytes': 16, 'archive_extractions': 0, 'native_runs': 0,
                     'gcp_actions': 0, 'scope': 'actual AST fragments; synthetic files; no historical data rehashed'},
                    sort_keys=True, indent=1))


if __name__ == '__main__':
    main()
