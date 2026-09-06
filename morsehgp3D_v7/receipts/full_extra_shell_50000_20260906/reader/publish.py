#!/usr/bin/env python3
"""Create only reader/; leave the base package and final SHA256SUMS to ROOT."""
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path('/workspaces/E-HGP')
BASE = Path(__file__).resolve().parent
PREP = ROOT/'build/v7_extra_shell_20260906'
TARGET = ROOT/'morsehgp3D_v7/receipts/full_extra_shell_50000_20260906/reader'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    files = {name: BASE/name for name in ('capture.py', 'compare_gcp.py', 'comparison.json', 'comparison.command.json', 'verify.py')}
    files.update({'README.md': BASE/'README.public.md', 'publish.py': Path(__file__),
                  'preparation_README.md': PREP/'README.md',
                  'read_extra_shell.py': PREP/'read_extra_shell.py', 'selftest_reader.py': PREP/'selftest_reader.py'})
    files.update({'synthetic/'+name: PREP/name for name in ('model_checks.json', 'development_count_failure.json')})
    files.update({'checks/'+path.name: path for path in sorted((BASE/'checks').iterdir()) if path.is_file()})
    pins = {name: sha(path) for name, path in files.items()}
    references = {'../run_r3/'+name: sha(PREP/'run_r3'/name) for name in
                  ('n50000_k10.stderr', 'n50000_k10.stdout', 'n50000_k10.command.json')}
    gcp = ROOT/'morsehgp3D_v7/receipts/full_g4_spot_50000_20260906/guest'
    references.update({'../../full_g4_spot_50000_20260906/guest/'+name: sha(gcp/name)
        for name in ('n50000_k10.stdout', 'n50000_k10.stderr', 'n50000_k10.command.json',
                     'n50000_k5.stdout', 'n50000_k5.stderr', 'n50000_k5.command.json')})
    TARGET.mkdir(parents=True, exist_ok=False)
    for name, source in files.items():
        destination = TARGET/name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, destination)
        if sha(source) != pins[name] or sha(destination) != pins[name]:
            raise ValueError('copy/source changed: '+name)
    with (TARGET/'FILES.json').open('x') as stream:
        json.dump(dict(files=pins, references=references, diagnostic_only=True, global_parents_reconstructed=False),
                  stream, sort_keys=True, indent=2)
        stream.write('\n')
    print(json.dumps(dict(status='copied', files=len(pins), references=len(references),
        target=str(TARGET), manifest_sha256=sha(TARGET/'FILES.json')), sort_keys=True))


if __name__ == '__main__':
    main()
