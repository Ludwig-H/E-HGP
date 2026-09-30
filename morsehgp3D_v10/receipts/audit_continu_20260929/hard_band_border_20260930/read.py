"""Strict autonomous replay; no dependency on mutable shared worktree sources."""
from copy import deepcopy
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

BASE = Path(__file__).resolve().parent
FILES = {'README.md', 'paires_snapshot.py', 'check.py', 'record.py', 'read.py', 'capture.json'}


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def semantic(result):
    require(set(result) == {'scope', 'snapshot_sha256', 'normal_optimized_invariant', 'cases',
                            'positive_eta_1_8', 'u18_integer_cases', 'mutations_killed', 'total_checks'}, 'result keys')
    require(result['snapshot_sha256'] == sha(BASE / 'paires_snapshot.py'), 'result source pin')
    require(result['normal_optimized_invariant'] is True, 'mode invariant field')
    require(len(result['cases']) == 11 and len(result['positive_eta_1_8']) == 11, 'case count')
    for c in result['cases']:
        outside = c['e'].startswith('-')
        require(c['W'] == (3 if outside else 4), 'band membership semantic check')
        expected = '55' if outside else '105'
        require(c['date_x'] == expected and c['meeting_radii']['0,1'] == expected, 'date/meeting semantics')
        require(c['meeting_radii']['1,3'] == '105', 'C/D stable merge')
        require(c['point_partition_r70scaled'] == ([[0, 1, 2], [3, 4]] if outside else [[0], [1, 2], [3, 4]]),
                'partition semantic check')
    require(all(c['W'] == 3 and c['date_x'] == '55' for c in result['positive_eta_1_8']), 'positive control')
    require(result['mutations_killed'] == {'majority_nonstrict': '55', 'band_open_at_e0': '55'}, 'causal mutations')
    require(result['total_checks'] == 1274, 'comparison count')
    integer = result['u18_integer_cases']
    require(len(integer) == 2 and [c['meeting_radii']['0,1'] for c in integer] == ['56320', '107520'],
            'u18 meeting witness')
    require([c['points'][-1] for c in integer] == ['0', '2'], 'u18 displacement witness')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--manifest-sha256', required=True)
    expected_sha = parser.parse_args().manifest_sha256
    require(len(expected_sha) == 64 and all(c in '0123456789abcdef' for c in expected_sha), 'external SHA syntax')
    require(sha(BASE / 'manifest.json') == expected_sha, 'external manifest SHA')
    require({p.name for p in BASE.iterdir()} == FILES | {'manifest.json'}, 'closed actual inventory')
    require(all(p.is_file() and not p.is_symlink() for p in BASE.iterdir()), 'inventory regular files only')
    manifest = json.loads((BASE / 'manifest.json').read_text())
    require(set(manifest) == {'schema', 'capture_valid', 'files'} and manifest['schema'] == 1 and
            manifest['capture_valid'] is True and set(manifest['files']) == FILES, 'manifest strict schema')
    for name, pin in manifest['files'].items():
        require(sha(BASE / name) == pin, 'file pin: ' + name)
    capture = json.loads((BASE / 'capture.json').read_text())
    require(set(capture) == {'schema', 'scope', 'started_utc', 'shared_before', 'source_before', 'runs',
                             'shared_after', 'source_after', 'finished_utc'} and capture['schema'] == 1, 'capture schema')
    require(capture['shared_before'] == capture['shared_after'], 'shared before/after')
    require(capture['source_before'] == capture['source_after'], 'private before/after')
    require(set(capture['source_before']) == FILES - {'capture.json'}, 'private pin set')
    require(all(capture['source_before'][name] == manifest['files'][name] for name in FILES - {'capture.json'}),
            'private source pins agree')
    runs = capture['runs']
    require(len(runs) == 2 and [r['mode'] for r in runs] == ['normal', 'optimized'], 'run modes')
    for run, flags in zip(runs, [['-B'], ['-B', '-O']]):
        require(set(run) == {'mode', 'argv', 'returncode', 'stdout', 'stderr'}, 'run schema')
        require(run['returncode'] == 0 and run['stderr'] == '', 'run success')
        require(run['argv'][1:-1] == flags and Path(run['argv'][-1]).name == 'check.py', 'command/receipt match')
        semantic(json.loads(run['stdout']))
        replay = subprocess.run([sys.executable] + flags + [str(BASE / 'check.py')], text=True,
                                capture_output=True, timeout=20)
        require(replay.returncode == 0 and replay.stderr == '' and replay.stdout == run['stdout'], 'causal exact replay')
    require(runs[0]['stdout'] == runs[1]['stdout'], 'normal/-O agree')
    bad = deepcopy(json.loads(runs[0]['stdout']))
    bad['cases'][0]['date_x'] = '55'
    rejected = False
    try:
        semantic(bad)
    except RuntimeError:
        rejected = True
    require(rejected, 'semantic mutant not killed')
    require({p.name for p in BASE.iterdir()} == FILES | {'manifest.json'}, 'closed final inventory')
    require(all(p.is_file() and not p.is_symlink() for p in BASE.iterdir()), 'final regular files only')
    require(sha(BASE / 'manifest.json') == expected_sha, 'manifest unchanged after replay')
    for name, pin in manifest['files'].items():
        require(sha(BASE / name) == pin, 'file unchanged after replay: ' + name)
    print(json.dumps({'status': 'PASS', 'checks': 1274, 'cases': 24,
                      'mutants': 2, 'reader_semantic_mutant_killed': True,
                      'manifest_sha256': sha(BASE / 'manifest.json')}, sort_keys=True))


if __name__ == '__main__':
    main()
