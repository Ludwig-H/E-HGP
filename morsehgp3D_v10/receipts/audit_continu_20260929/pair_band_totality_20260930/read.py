"""Externally pinned seven-file reader of a small exact semantics proof."""
import argparse
from copy import deepcopy
from fractions import Fraction as F
import hashlib
import json
from pathlib import Path
import subprocess
import sys

BASE = Path(__file__).resolve().parent
FILES = {'README.md', 'functions_snapshot.py', 'check.py', 'record.py', 'read.py', 'capture.json'}


def need(ok, msg):
    if not ok:
        raise RuntimeError(msg)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory():
    need({p.name for p in BASE.iterdir()} == FILES | {'manifest.json'}, 'closed actual seven-file inventory')
    need(all(p.is_file() and not p.is_symlink() for p in BASE.iterdir()), 'regular files only')


def semantic(result):
    need(set(result) == {'scope', 'snapshot_sha256', 'cases', 'K2_controls', 'threshold_controls',
                         'scale_bounds_all_points', 'mutants_killed'}, 'result strict schema')
    need(result['snapshot_sha256'] == sha(BASE / 'functions_snapshot.py'), 'function snapshot pin')
    cases = result['cases']
    need(len(cases) == 3 and [c['K'] for c in cases] == [3, 5, 5], 'K3/K5/translation cases')
    need([c['alpha2'] for c in cases] == ['9', '3', '3'] and
         [c['actual_A'] for c in cases] == ['16', '19/4', '19/4'], 'actual distinct scales')
    need(all(c['hard_eta_1_4_y'] == [] for c in cases), 'empty hard-alpha bands')
    need([r['ell2'] for r in cases[0]['raw_pairs']] == ['25', '16', '81/4', '81/4'], 'K3 exhaustive pair levels')
    need(all([r['ell2'] for r in c['raw_pairs']] == ['19/4'] * 4 for c in cases[1:]), 'K5 exhaustive pair levels')
    need(cases[0]['hard_eta_1_4_cutoff2'] == '225/16' and
         all(c['hard_eta_1_4_cutoff2'] == '75/16' for c in cases[1:]), 'cutoff levels')
    need(cases[1]['raw_pairs'] == cases[2]['raw_pairs'], 'translation exact invariance')
    need(all(0 <= F(v) < 2 ** 18 and F(v).denominator == 1 for p in cases[2]['points'] for v in p), 'u18 translation')
    controls = result['K2_controls']
    need(len(controls) == 2 and all(c['K'] == 2 and c['hard_eta_1_4_y'] for c in controls), 'K2 nonempty controls')
    need(result['threshold_controls'] == {'K3_eta_1_3': [1], 'K3_eta_1_2': [1, 2, 4],
                                         'K5_factor_squared_19_12': [1, 2, 3, 4],
                                         'K5_eta_13_50': [1, 2, 3, 4]}, 'threshold positive controls')
    need(len(result['scale_bounds_all_points']) == 20 and
         all(F(b['alpha2']) <= F(b['A']) <= 4 * F(b['alpha2']) for b in result['scale_bounds_all_points']),
         'all finite scale bounds')
    need(result['mutants_killed'] == {'open_shell_K3_K5': True, 'A_equals_alpha2': ['9', '3']}, 'causal mutations')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--manifest-sha256', required=True)
    expected = ap.parse_args().manifest_sha256
    need(len(expected) == 64 and all(c in '0123456789abcdef' for c in expected), 'external SHA format')
    need(sha(BASE / 'manifest.json') == expected, 'external manifest SHA before loading/replay')
    inventory()
    manifest = json.loads((BASE / 'manifest.json').read_text())
    need(set(manifest) == {'schema', 'capture_valid', 'files'} and manifest['schema'] == 1 and
         manifest['capture_valid'] is True and set(manifest['files']) == FILES, 'strict manifest')
    for name, pin in manifest['files'].items():
        need(sha(BASE / name) == pin, 'input file pin: ' + name)
    capture = json.loads((BASE / 'capture.json').read_text())
    need(set(capture) == {'schema', 'scope', 'started_utc', 'shared_before', 'source_before', 'runs',
                          'shared_after', 'source_after', 'finished_utc'} and capture['schema'] == 1, 'strict capture')
    need(capture['shared_before'] == capture['shared_after'] and capture['source_before'] == capture['source_after'],
         'stable source pins before/after')
    need(set(capture['source_before']) == FILES - {'capture.json'} and
         all(capture['source_before'][n] == manifest['files'][n] for n in FILES - {'capture.json'}), 'source pin set')
    runs = capture['runs']
    need(len(runs) == 2 and [r['mode'] for r in runs] == ['normal', 'optimized'], 'two execution modes')
    for run, flags in zip(runs, [['-B'], ['-B', '-O']]):
        need(set(run) == {'mode', 'argv', 'returncode', 'stdout', 'stderr'}, 'strict run')
        need(run['returncode'] == 0 and run['stderr'] == '' and run['argv'][1:-1] == flags and
             Path(run['argv'][-1]).name == 'check.py', 'command/receipt success')
        semantic(json.loads(run['stdout']))
        replay = subprocess.run([sys.executable] + flags + [str(BASE / 'check.py')], text=True,
                                capture_output=True, timeout=20)
        need(replay.returncode == 0 and replay.stderr == '' and replay.stdout == run['stdout'], 'causal exact replay')
    need(runs[0]['stdout'] == runs[1]['stdout'], 'normal/-O identical')
    bad = deepcopy(json.loads(runs[0]['stdout']))
    bad['cases'][0]['actual_A'] = '9'
    rejected = False
    try:
        semantic(bad)
    except RuntimeError:
        rejected = True
    need(rejected, 'reader semantic mutant rejected')
    inventory()
    need(sha(BASE / 'manifest.json') == expected, 'manifest unchanged after replay')
    for name, pin in manifest['files'].items():
        need(sha(BASE / name) == pin, 'files unchanged after replay: ' + name)
    print(json.dumps({'status': 'PASS', 'empty_cases': 3, 'K2_controls': 2, 'scale_checks': 20,
                      'mutants': 2, 'reader_semantic_mutant_killed': True, 'manifest_sha256': expected}, sort_keys=True))


if __name__ == '__main__':
    main()
