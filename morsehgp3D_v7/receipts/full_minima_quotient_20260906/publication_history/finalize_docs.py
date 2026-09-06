#!/usr/bin/env python3
"""Authorized suffix-only documentation fix; preserve the initial closure."""
import hashlib
import json
from pathlib import Path
import shutil
import sys

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
DEST = ROOT / 'morsehgp3D_v7/receipts/full_minima_quotient_20260906'


def require(ok, why):
    if not ok:
        raise ValueError(why)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def manifest(directory, name):
    (directory / name).write_text(''.join(sha(p) + '  ' + str(p.relative_to(directory)) + '\n'
        for p in sorted(directory.rglob('*')) if p.is_file() and p != directory / name))


def main():
    require(sys.argv[1:] == ['--execute'], 'inert_without_execute')
    expected = {'SHA256SUMS': 'f22d594a86a24ddb3dfe051b7eb9a6cd28347005ff88c59386b337f70e1fa16b',
                'payload/PAYLOAD_SHA256SUMS': '26be4445fb571a72ac8ed30a340359f0f8a453b0bf46674ffa84b37e544ee275',
                'publication.json': '6461da075ab7e6977218a69bcee6e37ac3af91f9f0a2043982ad8451f13771ed',
                'README.md': 'e003dbb115e84c2335194e4c4e8db6990736cba2816e80439767d2ac1a60dd95'}
    require(all(sha(DEST / name) == pin for name, pin in expected.items()), 'initial_closure_pins')
    require(sha(BASE / 'README_final.md') == '857b8a683bec2896ce0daf05bf97f84f7354a1fd06d9c4a83137660df47c0e6f',
            'new_readme_pin')
    history = DEST / 'publication_history/initial'
    require(not history.parent.exists(), 'fresh_publication_history')
    history.mkdir(parents=True)
    for name in expected:
        shutil.copyfile(DEST / name, history / Path(name).name)
    for name, pin in {'quotient': 'e5cad93a310d420c0a466e2b03cf2082850c4f52388dce7a83d4601e2ea33448',
                      'descent': '5439d5f48738340adca6c66dc90a916b929242cb0ee2cc2eccb42aad0f913281',
                      'lower_bound': 'a3350584db1d7e98cc7e5cae6a5ff17d3b13238388ed83b8780b754ea7a2ff67'}.items():
        source = DEST / 'payload/proofs' / (name + '.md')
        target = source.with_suffix('.md.txt')
        require(sha(source) == pin and not target.exists(), 'historical_proof_pin')
        source.rename(target)
        require(sha(target) == pin, 'unchanged_proof_bytes')
    shutil.copyfile(BASE / 'README_final.md', DEST / 'README.md')
    shutil.copyfile(Path(__file__), history.parent / 'finalize_docs.py')
    manifest(DEST / 'payload', 'PAYLOAD_SHA256SUMS')
    report = json.loads((DEST / 'publication.json').read_bytes())
    report['replayed_payload_manifest_sha256'] = report['portable_payload_manifest_sha256']
    report['portable_payload_manifest_sha256'] = sha(DEST / 'payload/PAYLOAD_SHA256SUMS')
    report['post_replay_change'] = 'three_document_suffixes_only_same_bytes_and_readme_links'
    report['new_test_campaign_after_doc_fix'] = False
    (DEST / 'publication.json').write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    manifest(DEST, 'SHA256SUMS')
    print(json.dumps(dict(status='completed', files=sum(p.is_file() for p in DEST.rglob('*')),
                         root_manifest_sha256=sha(DEST / 'SHA256SUMS'),
                         publication_sha256=sha(DEST / 'publication.json')), sort_keys=True))


if __name__ == '__main__':
    main()
