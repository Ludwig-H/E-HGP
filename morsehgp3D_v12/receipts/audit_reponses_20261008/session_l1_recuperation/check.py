#!/usr/bin/env python3
"""Relecture locale hachee ; aucune commande cloud, aucun moteur, sortie filtree."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import tarfile

HERE = Path(__file__).resolve().parent

def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def same_hash(raw, spec):
    need(len(raw) == spec['bytes'] and hashlib.sha256(raw).hexdigest() == spec['sha256'], 'empreinte differente')


def files(raw):
    with tarfile.open(fileobj=io.BytesIO(raw)) as archive:
        result = {}
        for member in archive.getmembers():
            path = Path(member.name)
            need(not path.is_absolute() and '..' not in path.parts and not member.issym() and not member.islnk(),
                 'membre interdit')
            if member.isfile():
                need(member.name not in result, 'membre repete')
                result[member.name] = archive.extractfile(member).read()
        return result


def role(fs, name):
    found = [raw for path, raw in fs.items() if Path(path).name == name]
    need(len(found) == 1, 'role absent ou ambigu')
    return found[0]


def manifest(fs, count):
    covered = set()
    for line in role(fs, 'MANIFEST.sha256').decode('ascii').splitlines():
        digest, name = line.split(None, 1)
        name = name.lstrip('*').strip()
        found = set(n for n in (name, 'results/' + name, name.removeprefix('./'),
                               'results/' + name.removeprefix('./')) if n in fs)
        need(len(found) == 1, 'entree du manifeste absente ou ambigue')
        name = found.pop()
        need(name not in covered and hashlib.sha256(fs[name]).hexdigest() == digest, 'manifeste divergent')
        covered.add(name)
    need(len(covered) == count and len(set(fs) - covered) == 1 and
         Path(next(iter(set(fs) - covered))).name == 'MANIFEST.sha256', 'manifeste incomplet')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original-session', type=Path, required=True)
    parser.add_argument('--recovery-session', type=Path, required=True)
    args = parser.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    for folder, key in ((args.original_session, 'original'), (args.recovery_session, 'recovery_and_new_run')):
        for name, field in (('receipt.json', 'receipt'), ('package/package.tar.gz', 'package'),
                            ('package/plan.json', 'plan')):
            same_hash((folder / name).read_bytes(), cap[key][field])
    outer = (args.recovery_session / 'results/results.tar.gz').read_bytes()
    same_hash(outer, cap['recovery_and_new_run']['archive'])
    outer_files = files(outer)
    inner = role(outer_files, 'results.tar.gz')
    same_hash(inner, cap['original']['archive'])
    for fs, key in ((files(inner), 'original'), (outer_files, 'recovery_and_new_run')):
        spec = cap[key]
        manifest(fs, spec['manifest_entries_verified'])
        same_hash(role(fs, 'rapport_b.json'), spec['report'])
        same_hash(role(fs, 'plan.sh'), spec['plan_script'])
        worker = dict(line.split('=', 1) for line in role(fs, 'worker.txt').decode().splitlines() if '=' in line)
        need(all(worker.get(k) == v for k, v in spec['worker'].items()), 'source ou cloture worker divergente')
    receipt = json.loads((args.recovery_session / 'receipt.json').read_text())
    close = cap['recovery_and_new_run']['closure']
    for key in ('status', 'worker_exit_code', 'results_verified', 'results_sha256', 'retrieval', 'closure',
                'targeted_shutdown_certified', 'stop_exit_code', 'stop_attempts', 'reserve_released'):
        need(type(receipt[key]) is type(close[key]) and receipt[key] == close[key], 'cloture differente')
    need(len(receipt['errors']) == close['errors_count'] == 0 and
         int((args.recovery_session / 'DONE').read_text()) == close['done'] == 0, 'issue recuperation differente')
    need(receipt['observed_before_stop']['status'] == close['before_status'] == 'RUNNING' and
         receipt['observed_after']['status'] == close['after_status'] == 'TERMINATED', 'arret different')
    print('verified: original 3497c745, envelope 34eebfbf, manifests 72+81, sources distinctes, arret certifie local')


if __name__ == '__main__':
    main()
