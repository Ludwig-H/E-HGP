"""Assemble a fresh audit receipt, preserving all already-closed bytes."""
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import sys

sys.dont_write_bytecode = True
root = Path(__file__).resolve().parent
destination = Path('/workspaces/E-HGP/build/v9-open-worktree/morsehgp3D_v10/receipts/audit_continu_20260929/relative_filter_20260930')
destination.mkdir()
origins = {
    'prototype': Path('/tmp/mhgp10-relative-sphere-filter-0930.gBKSarn2'),
    'judge_adversarial_r1': Path('/tmp/mhgp10-relative-judge-adversarial-20260930.s0huDoJ1'),
    'independent': root,
}
copied = {}
for label, origin in origins.items():
    folder = destination/label
    folder.mkdir()
    copied[label] = {}
    for path in sorted(origin.iterdir()):
        if not path.is_file() or path.is_symlink():
            raise ValueError('unexpected nonfile in source packet: '+str(path))
        shutil.copy2(path, folder/path.name)
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        if digest != hashlib.sha256((folder/path.name).read_bytes()).hexdigest():
            raise ValueError('copy changed bytes')
        copied[label][path.name] = digest
for version in ('inputs_v1', 'inputs'):
    spec = importlib.util.spec_from_file_location(version, root/(version+'.py'))
    inputs = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(inputs)
    panel = inputs.panel()
    outside = [dict(id=r['id'], label=r['label'],
                    coefficients=[str(n) for n in r['n'] if abs(n) > inputs.MAX192])
               for r in panel if r['op'] != 'C' and
               any(abs(n) > inputs.MAX192 for n in r['n'])]
    if version == 'inputs' and outside:
        raise ValueError('final input panel outside prototype domain')
    preflight = dict(schema='mhgp10_relative_filter_input_domain_review_v1',
                     version=version, requests=len(panel), outside=outside,
                     native_executions=0,
                     historical_note='Reread diagnostic now, not a newly found native failure; initial v1 panel was never transmitted to a native filter.')
    (destination/'independent'/(version+'_domain_review.json')).write_text(
        json.dumps(preflight, sort_keys=True, indent=2)+'\n')
(destination/'assembly.json').write_text(json.dumps(dict(
    schema='mhgp10_relative_filter_assembly_v1', origins={k:str(v) for k,v in origins.items()},
    copied_sha256=copied, external_binaries_not_copied=True,
    GCP_used=False, engine_source_changes=0), sort_keys=True, indent=2)+'\n')
files = sorted((destination/'independent').iterdir())
(destination/'independent'/'SHA256SUMS').write_text(''.join(
    hashlib.sha256(path.read_bytes()).hexdigest()+'  '+path.name+'\n' for path in files))
print(json.dumps(dict(status='ASSEMBLED', destination=str(destination),
                      copied_files=sum(map(len,copied.values()))), sort_keys=True))
