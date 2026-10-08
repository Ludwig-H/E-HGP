#!/usr/bin/env python3
"""Rejoue la proposition B3 dans un répertoire temporaire, sans exécution native."""
from pathlib import Path
import argparse
import ast
import hashlib
import importlib.util
import json
import subprocess
import sys
import tempfile
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent

def need(ok, why):
    if not ok:
        raise ValueError(why)

def pin(raw):
    return dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())

def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args])

def functions(text):
    return {n.name: ast.get_source_segment(text, n) for n in ast.parse(text).body
            if isinstance(n, ast.FunctionDef)}

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo', required=True, type=Path)
    ap.add_argument('--live', action='store_true', help='exige les bases encore identiques sur disque')
    args = ap.parse_args()
    for line in (HERE / 'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        need('/' not in name and pin((HERE / name).read_bytes())['sha256'] == digest, 'receipt ' + name)
    cap = json.loads((HERE / 'capture.json').read_text())
    before = None
    with tempfile.TemporaryDirectory(prefix='b3-patch-replay-') as tmp:
        root = Path(tmp)
        snapshot, proposed = root / 'snapshot', root / 'proposal'
        source = snapshot / 'source'
        for name, expected in {**cap['sources'], **cap['native_schema_references']}.items():
            relative = 'morsehgp3D_v12/' + name
            raw = git(args.repo, 'show', cap['source_git'] + ':' + relative)
            need(pin(raw) == expected, 'committed source ' + name)
            if args.live:
                need(pin((args.repo / relative).read_bytes()) == expected, 'live base changed ' + name)
            if name in cap['sources']:
                for p in (source / name, proposed / relative):
                    p.parent.mkdir(parents=True, exist_ok=True)
                    p.write_bytes(raw)
            if relative == cap['changed_path']:
                before = raw
        (snapshot / 'capture.json').write_text(json.dumps({'sources': cap['sources']}))
        patch = (HERE / 'proposition.patch').read_bytes()
        subprocess.run(['git', 'apply', '--check', '-'], input=patch, cwd=proposed, check=True)
        subprocess.run(['git', 'apply', '-'], input=patch, cwd=proposed, check=True)
        after = (proposed / cap['changed_path']).read_bytes()
        need(pin(before) == cap['before'] and pin(after) == cap['after'], 'pre/postimages')
        a, b = functions(before.decode()), functions(after.decode())
        need([n for n in a if a[n] != b.get(n)] == cap['changed_functions'], 'changed functions')
        need([n for n in b if n not in a] == cap['added_functions'], 'added functions')
        # Les seuils, le bootstrap et toute la decision apres admission demeurent octet pour octet.
        for name in ['mediane', 'bootstrap_gm', 'juger_identite', 'juger_resolution', '_juger_admis',
                     'resume', 'campagne_synthetique', 'etape_auto_test']:
            need(a[name] == b[name], 'unchanged decision ' + name)
        ignored = set(cap['changed_functions']) | set(cap['added_functions'])
        def other_nodes(raw):
            return [ast.dump(n) for n in ast.parse(raw).body
                    if not isinstance(n, ast.FunctionDef) or n.name not in ignored]
        need(other_nodes(before.decode()) == other_nodes(after.decode()), 'other module code unchanged')
        spec = importlib.util.spec_from_file_location('b3_receipt_compare', HERE / 'compare.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        actual = module.run(snapshot, proposed / 'morsehgp3D_v12')
        need(actual == json.loads((HERE / 'results.json').read_text()), 'comparative outcomes changed')
    print(json.dumps(dict(source=cap['source_git'], after=cap['after']['sha256'],
        cases=len(actual['cases']), normal_or_optimized_replay='ok', native_executed=False,
        patch_applied_to_product=False, gate_integration=cap['test_integration'])))

if __name__ == '__main__':
    main()
