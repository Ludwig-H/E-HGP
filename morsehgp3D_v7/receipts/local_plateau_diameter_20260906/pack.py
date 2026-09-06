#!/usr/bin/env python3
"""Create-only archival packet in build; no compiler, engine, Git or publisher."""
import hashlib
import json
from pathlib import Path
import shutil

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
OUT = BASE / 'packet'
OLD = ROOT / 'morsehgp3D_v7/receipts/local_plateau_20260906'
MUTANTS = ('drop_square_diameter', 'disable_diameter_hub', 'admit_u2', 'drop_star_unions')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(ok, why):
    if not ok:
        raise ValueError(why)


def copy(path, relative):
    blob = path.read_bytes()
    require(not blob.startswith(b'\x7fELF'), 'no_ELF')
    target = OUT / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    with target.open('xb') as out:
        out.write(blob)
    require(sha(path) == sha(target), 'byte_exact_copy')


prepare = json.loads((BASE / 'prepare.receipt.json').read_text())
failed = json.loads((BASE / 'san.receipt.json').read_text())
resume = json.loads((BASE / 'san_resume/receipt.json').read_text())
require(prepare['status'] == resume['status'] == 'completed' and failed['status'] == 'failed', 'separate_statuses')
require(all(r['cpp_closed'] is True for r in (prepare, failed, resume)), 'all_commands_closed')
require(sha(BASE / 'prepare.receipt.json') == 'e9fb6d2a0dbd14fb79eae5145b074c23d43519313cc6f8e64c10932e4ea43afd', 'prepare_pin')
require(sha(BASE / 'san_resume/receipt.json') == 'b801e66ef00f3df9a978112da83d79bb7f49964b8899ac6d7a4e107dac1f89a6', 'resume_pin')
OUT.mkdir()
for path in BASE.iterdir():
    if path.is_file() and (path.suffix in ('.json', '.stdout', '.stderr', '.d') or path.name in ('README.md', 'record.py', 'pack.py')):
        copy(path, path.name)
for path in (BASE / 'san_resume').iterdir():
    if path.is_file():
        copy(path, Path('san_resume') / path.name)
for path in (BASE / 'source_snapshot').rglob('*'):
    if path.is_file():
        copy(path, path.relative_to(BASE))
for mutant in MUTANTS:
    copy(BASE / mutant / 'src/forest/local_plateau.hpp', Path('mutants') / mutant / 'local_plateau.hpp')
before = json.loads((BASE / 'O2.sources_before.json').read_text())
old = json.loads((OLD / 'publication.json').read_text())['sources']
refs = {}
for path, digest in before.items():
    source = Path(path)
    require(sha(source) == digest, 'current_dependency_stable')
    if source.is_relative_to(ROOT / 'morsehgp3D_v7'):
        relative = source.relative_to(ROOT / 'morsehgp3D_v7')
        if (OUT / 'source_snapshot' / relative).exists():
            continue
        historical = old[str(source.relative_to(ROOT))]
        target = (OLD / historical['relative_path']).resolve()
        require(historical['sha256'] == digest == sha(target), 'historical_dependency_pin')
        refs[str(source.relative_to(ROOT))] = {'repo_path': str(target.relative_to(ROOT)), 'sha256': digest}
metadata = dict(schema='mhgp7-local-plateau-diameter-packet-v1', status='prepared_in_build',
                public_status='not_claimed', FULL_integrated=False, GCP_used=False,
                omitted_ELF=prepare['binaries'], historical_product_dependencies=refs,
                external_dependency_hashes_in='O2.sources_before.json',
                reused_test_sources='mutants use nominal gate/oracle from source_snapshot; only changed helper copied')
with (OUT / 'packet.json').open('x') as stream:
    json.dump(metadata, stream, sort_keys=True, indent=2)
    stream.write('\n')
files = sorted(path for path in OUT.rglob('*') if path.is_file())
with (OUT / 'SHA256SUMS').open('x') as stream:
    for path in files:
        stream.write(sha(path) + '  ' + path.relative_to(OUT).as_posix() + '\n')
print(json.dumps({'status': 'prepared_in_build', 'files': len(files), 'manifest_sha256': sha(OUT / 'SHA256SUMS')}, sort_keys=True))
