#!/usr/bin/env python3
"""Vérifie le correctif documentaire dans un répertoire temporaire uniquement."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent


def need(ok, why):
    if not ok:
        raise ValueError(why)


def pin(raw):
    return dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args])


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--repo', required=True, type=Path)
    ap.add_argument('--live', action='store_true', help='exige les deux bases encore inchangées sur disque')
    args = ap.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    evidence_path = HERE / cap['evidence']['relative']
    evidence_raw = evidence_path.read_bytes()
    need(pin(evidence_raw) == {k: cap['evidence'][k] for k in ('bytes', 'sha256')},
         'evidence changed')
    measure = json.loads(evidence_raw)['second']
    stats = measure['judgment']['cas']['statistiques']
    selected = dict(large_gm=stats['grandes']['moyenne_geometrique'],
        ng_upper={n: stats[n]['ic95'][1] for n in ['ng00', 'ng01']},
        large={a: {k: measure['large'][a][k] for k in ['median_frame_medians_ns',
            'max_frame_median_ns', 'max_warm_ns', 'frames_median_over_100ms',
            'passes_over_100ms']} for a in ['avant', 'apres']})
    need(selected == cap['statistics'], 'selected admitted statistics')
    for arm in ['avant', 'apres']:
        values = measure['large'][arm]['per_frame_median_ns']
        need(values['kitti_ng_00_001896'] == max(values.values()), 'worst frame median')
    need(not git(args.repo, 'diff', '--name-only', cap['withdrawal_reference'],
                 cap['source_git'], '--', *cap['withdrawal_scopes']), 'committed withdrawal')
    current = {}
    with tempfile.TemporaryDirectory(prefix='a6b-doc-review-') as tmp:
        root = Path(tmp)
        for path, expected in cap['paths'].items():
            raw = git(args.repo, 'show', cap['source_git'] + ':' + path)
            need(pin(raw) == expected['before'], 'pinned source ' + path)
            p = root / path
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(raw)
            live = args.repo / path
            current[path] = ('before' if live.exists() and pin(live.read_bytes()) == expected['before']
                             else 'after' if live.exists() and pin(live.read_bytes()) == expected['after']
                             else 'different')
        if args.live:
            need(all(v == 'before' for v in current.values()), 'live bases changed; rebase proposal')
        patch = (HERE / 'proposition.patch').read_bytes()
        subprocess.run(['git', 'apply', '--check', '-'], input=patch, cwd=root, check=True)
        subprocess.run(['git', 'apply', '-'], input=patch, cwd=root, check=True)
        for path, expected in cap['paths'].items():
            need(pin((root / path).read_bytes()) == expected['after'], 'postimage ' + path)
    print(json.dumps(dict(source=cap['source_git'], patched_documents=2,
        live=current, source_and_postimages_verified=True, product_changed=False)))


if __name__ == '__main__':
    main()
