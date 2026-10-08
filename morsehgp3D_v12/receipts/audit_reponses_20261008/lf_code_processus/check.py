#!/usr/bin/env python3
"""Strict process-code proposal; Python fixtures only, no native process launched."""
import argparse
import contextlib
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import tempfile
import types

HERE = Path(__file__).resolve().parent


def need(ok, why):
    if not ok:
        raise ValueError(why)


def digest(body):
    return hashlib.sha256(body).hexdigest()


def module(body, name):
    value = types.ModuleType(name)
    exec(compile(body, name, 'exec'), value.__dict__)
    return value


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', type=Path, required=True)
    args = parser.parse_args()
    capture = json.loads((HERE / 'capture.json').read_bytes())
    bodies = {}
    for path, expected in capture['files'].items():
        body = subprocess.check_output(['git', '-C', str(args.repo), 'show', capture['source'] + ':' + path])
        need(digest(body) == expected, 'source pin')
        bodies[path] = body
    patch = HERE / 'proposition.patch'
    need(digest(patch.read_bytes()) == capture['patch_sha256'], 'patch pin')
    with tempfile.TemporaryDirectory(prefix='audit-lf-codes-') as tmp:
        for path, body in bodies.items():
            dest = Path(tmp) / path
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(body)
        for check in (True, False):
            subprocess.run(['git', 'apply'] + (['--check'] if check else []) + [str(patch)],
                           cwd=tmp, check=True, capture_output=True)
        folder = Path(tmp) / 'morsehgp3D_v12/microbancs/outils'
        candidate = (folder / 'lecteur_full.py').read_bytes()
        need(digest(candidate) == capture['postimage_sha256'], 'postimage')
        spec = importlib.util.spec_from_file_location('fixture', folder / 'test_lecteur_full.py')
        fixture = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(fixture)
        new = fixture.lf
        old = module(bodies['morsehgp3D_v12/microbancs/outils/lecteur_full.py'], 'old')
        guard = ("    if type(code) is not int:\n"
                 "        return dict(etat='illisible', raison='code processus non entier', passes=[])\n")
        need(candidate.decode().count(guard) == 1, 'unique guard')
        mutant_body = candidate.decode().replace(guard, '')
        need(mutant_body.encode() == bodies['morsehgp3D_v12/microbancs/outils/lecteur_full.py'], 'causal mutant')
        mutant = module(mutant_body, 'mutant')
        good = fixture.device_output()
        refused = fixture.device_output(passes=1, end=dict(phase='exit', status='resource_exhausted', reason='memory_budget'))
        examples = [('boolean_false', False, good, 'ok'), ('float_zero', 0.0, good, 'ok'),
                    ('float_negative_zero', -0.0, good, 'ok'), ('float_two', 2.0, refused, 'refus')]
        results = []
        for name, code, text, previous in examples:
            states = [reader.parse_output(code, text, fixture.ATTENDU)['etat'] for reader in (old, new, mutant)]
            need(states == [previous, 'illisible', previous], 'non-integer code not causally rejected')
            results.append(dict(case=name, before=states[0], after=states[1], mutant=states[2]))
        positives = []
        for code, text in ((0, good), (2, refused), (-9, ''), ('expire', '')):
            states = [reader.parse_output(code, text, fixture.ATTENDU) for reader in (old, new, mutant)]
            need(states[0] == states[1] == states[2], 'valid code changed')
            positives.append(dict(code=code, state=states[0]['etat']))
        outputs = {}
        for name, reader in (('before', old), ('after', new)):
            fixture.lf = reader
            stream = io.StringIO()
            with contextlib.redirect_stdout(stream):
                code = fixture.main()
            need(code == 0, 'official Python gate failed')
            outputs[name] = stream.getvalue().strip()
        need(outputs['before'] == outputs['after'], 'official nominal gate changed')
        print(json.dumps(dict(counterexamples=results, positives=positives, official_gate=outputs,
                              source=capture['source'], native_executed=False), indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
