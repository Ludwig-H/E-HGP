#!/usr/bin/env python3
"""Rejoue des portes Python avec sondes fictives, jamais un moteur HGP."""
import argparse
import ast
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent


def need(ok, why):
    if not ok:
        raise ValueError(why)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', required=True, type=Path)
    args = ap.parse_args()
    pins = json.loads((HERE / 'pins.json').read_text())
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        for rel, digest in pins['sources'].items():
            raw = subprocess.check_output(['git', '-C', str(args.repo), 'show', pins['commit'] + ':morsehgp3D_v12/' + rel])
            need(hashlib.sha256(raw).hexdigest() == digest, rel)
            path = root / 'morsehgp3D_v12' / rel
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(raw)
        folder = root / 'morsehgp3D_v12/microbancs/mes_c_petits'
        gate = folder / 'test_pilote_c.py'
        pilot = folder / 'pilote_c.py'

        def run(optimized=False):
            argv = [sys.executable, '-B'] + (['-O'] if optimized else []) + [str(gate)]
            return subprocess.run(argv, capture_output=True, text=True, timeout=120)

        baseline = [run(False), run(True)]
        need(all(r.returncode == 0 for r in baseline), 'porte officielle')
        need(baseline[0].stdout == baseline[1].stdout, 'porte officielle normal/-O')
        subprocess.run(['git', 'apply', str(HERE / 'cohorte_et_porte.patch')], cwd=root, check=True)
        for rel, digest in pins['postimages'].items():
            need(hashlib.sha256((root / rel).read_bytes()).hexdigest() == digest, 'postimage ' + rel)
        proposed = [run(False), run(True)]
        need(all(r.returncode == 0 for r in proposed), 'porte adaptee')
        need(proposed[0].stdout == proposed[1].stdout, 'porte adaptee normal/-O')
        spec = importlib.util.spec_from_file_location('mutants_mes_c', folder / 'mutants_pilote_c.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)  # definitions seules ; main non appele
        original = pilot.read_text()
        mutants = {}
        for name, (old, new) in module.MUTANTS.items():
            need(original.count(old) == 1, 'substitution unique ' + name)
            mutated = original.replace(old, new)
            ast.parse(mutated)
            pilot.write_text(mutated)
            done = run()
            need(done.returncode == 1, 'mutant non tue causalement ' + name)
            diagnostic = done.stderr.strip()
            need(bool(diagnostic) and 'SyntaxError' not in diagnostic and 'ImportError' not in diagnostic,
                 'diagnostic absent/invalide ' + name)
            mutants[name] = {'code': done.returncode, 'diagnostic': diagnostic}
        pilot.write_text(original)
        print(json.dumps({'native_execution': False, 'baseline_normal_optimized': [r.returncode for r in baseline],
                         'proposed_normal_optimized': [r.returncode for r in proposed],
                         'mutants_after_patch': mutants}, sort_keys=True))


if __name__ == '__main__':
    main()
