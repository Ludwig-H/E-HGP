#!/usr/bin/env python3
"""Trois portes officielles et observation des enfants des deux runners de mutants.
Sources extraites de Git ; faux probes Python seulement, aucun moteur/compilation.
"""
import ast
import argparse
from contextlib import redirect_stderr, redirect_stdout
import hashlib
import io
import json
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys

ROOT = Path(__file__).resolve().parent


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def compact(text):
    return re.sub(r'File "[^"]*/([^/"]+)"', r'File "<temp>/\1"', text).strip()


def main():
    global ROOT
    parser = argparse.ArgumentParser()
    parser.add_argument('--evidence', type=Path, required=True)
    ROOT = parser.parse_args().evidence
    pins = json.loads((ROOT / 'pins.json').read_text())
    for rel, pin in pins['files'].items():
        raw = (ROOT / rel).read_bytes()
        need(hashlib.sha256(raw).hexdigest() == pin['sha256'], 'source différente : ' + rel)
    env = dict(os.environ, PYTHONDONTWRITEBYTECODE='1')
    gates = []
    for rel in ('microbancs/outils/test_lecteur_full.py', 'microbancs/mes_b_scenes/test_pilote_b.py',
                'microbancs/mes_full/test_pilote_full.py'):
        for option in ([], ['-O']):
            done = subprocess.run([sys.executable, *option, str(ROOT / rel)], capture_output=True, text=True,
                                  env=env, timeout=90)
            gates.append(dict(test=rel, optimized=bool(option), code=done.returncode,
                              stdout=compact(done.stdout), stderr=compact(done.stderr)))
            need(done.returncode == 0 and not done.stderr, 'porte officielle en échec : ' + rel)
    runners = []
    for rel, source, expected in (
            ('microbancs/outils/mutants_lecteur_full.py', 'microbancs/outils/lecteur_full.py', 16),
            ('microbancs/mes_b_scenes/mutants_pilote_b.py', 'microbancs/mes_b_scenes/pilote_b.py', 12)):
        ns = runpy.run_path(str(ROOT / rel), run_name='audit_import')
        original = (ROOT / source).read_text()
        names = list(ns['MUTANTS'])
        need(len(names) == expected, 'nombre de mutants')
        for name, (old, new) in ns['MUTANTS'].items():
            need(original.count(old) == 1, 'motif mutant invalide : ' + name)
            ast.parse(original.replace(old, new), filename=source)
        calls, run = [], subprocess.run
        def observed(argv, **kw):
            kw.update(stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env)
            done = run(argv, **kw)
            calls.append(dict(mutant=names[len(calls)], code=done.returncode,
                              stdout=compact(done.stdout), stderr=compact(done.stderr)))
            return done
        out, err = io.StringIO(), io.StringIO()
        try:
            subprocess.run = observed
            with redirect_stdout(out), redirect_stderr(err):
                code = ns['main']()
        finally:
            subprocess.run = run
        need(code == 0 and len(calls) == expected, 'runner en échec')
        for call in calls:
            need(call['code'] == 1 and call['stderr'], 'mort sans diagnostic')
            need(not any(t in call['stderr'] for t in ('SyntaxError', 'ImportError', 'ModuleNotFoundError')),
                 'mort par syntaxe/import')
        runners.append(dict(runner=rel, code=code, stdout=out.getvalue().strip(), stderr=err.getvalue().strip(),
                            children=calls))
    print(json.dumps(dict(commit=pins['commit'], gates=gates, runners=runners), ensure_ascii=False,
                     sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
