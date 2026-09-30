"""Strict externally pinned autonomous reader; no mutable shared dependencies."""
import argparse
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import subprocess
import sys

BASE = Path(__file__).resolve().parent
FILES = {'README.md', 'sitegrid_snapshot.py', 'check.py', 'record.py', 'read.py', 'capture.json'}


def need(ok, msg):
    if not ok:
        raise RuntimeError(msg)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory():
    need({p.name for p in BASE.iterdir()} == FILES | {'manifest.json'}, 'closed actual seven-file inventory')
    need(all(p.is_file() and not p.is_symlink() for p in BASE.iterdir()), 'regular files only')


def semantic(result):
    need(set(result) == {'scope', 'snapshot_sha256', 'grid_cases', 'packing_cases', 'packing_cases_count',
                         'mutation_candidates_killed', 'mutation_cap_one_killed'}, 'result schema')
    need(result['snapshot_sha256'] == sha(BASE / 'sitegrid_snapshot.py'), 'AST snapshot pin')
    need(result['packing_cases_count'] == 66 and len(result['packing_cases']) == 66, 'packing case count')
    need([r['n'] for r in result['grid_cases']] == [8000, 16000, 32000], 'grid sizes')
    need([r['h'] for r in result['grid_cases']] == [32768, 16384, 16384], 'actual global cell sizes')
    for r in result['grid_cases']:
        m = r['n'] - 2
        need(r['bulk_m'] == m and r['bulk_cell_load'] == m and r['occupied_cells'] == 3, 'dense cell')
        need(r['total_votes_exact_derived'] == 3 * m, 'linear actual votes')
        need(r['bulk_candidate_ids_total_derived'] == m * m, 'quadratic derived candidate count')
        need(r['bulk_lignes_distance_tests_total_derived'] == m * (m - 1), 'quadratic distance tests')
        need(r['mate_census_tests_no_warmup_derived'] == (m // 2) * (m - 2), 'mate census cost')
        need(r['mate_census_tests_after_300_births_lower_bound'] == (m // 2 - 300) * (m - 2), 'warm-cache lower bound')
        need(r['positive_h'] == 4 and all(2 <= v <= 16 for v in r['positive_candidate_samples']), 'positive control')
        need(r['positive_candidate_total_upper_bound_derived'] == 16 * m, 'positive total bound')
        for q in r['sample_queries']:
            need(q['candidate_ids'] == m and q['retained_votes'] == 1 and q['mate_census_ids'] == m and
                 q['mate_census_sphere_tests'] == m - 2, 'sample AST behavior')
    for r in result['packing_cases']:
        need(r['max_incoming_bucket_load'] <= r['K'] - 1 and r['directed_pairs'] <= r['global_bound'],
             'incoming cap and global bound')
    need(result['mutation_candidates_killed'] == 'actual candidates returns whole dense cell' and
         result['mutation_cap_one_killed'] == 2, 'causal mutations')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--manifest-sha256', required=True)
    expected = ap.parse_args().manifest_sha256
    need(len(expected) == 64 and all(c in '0123456789abcdef' for c in expected), 'external SHA format')
    need(sha(BASE / 'manifest.json') == expected, 'external manifest SHA before loading/replay')
    inventory()
    manifest = json.loads((BASE / 'manifest.json').read_text())
    need(set(manifest) == {'schema', 'capture_valid', 'files'} and manifest['schema'] == 1 and
         manifest['capture_valid'] is True and set(manifest['files']) == FILES, 'manifest strict schema')
    for name, pin in manifest['files'].items():
        need(sha(BASE / name) == pin, 'input file pin: ' + name)
    capture = json.loads((BASE / 'capture.json').read_text())
    need(set(capture) == {'schema', 'scope', 'started_utc', 'shared_before', 'source_before', 'runs',
                          'shared_after', 'source_after', 'finished_utc'} and capture['schema'] == 1, 'capture schema')
    need(capture['shared_before'] == capture['shared_after'] and capture['source_before'] == capture['source_after'],
         'all source pins stable before/after capture')
    need(set(capture['source_before']) == FILES - {'capture.json'}, 'source pin inventory')
    need(all(capture['source_before'][n] == manifest['files'][n] for n in FILES - {'capture.json'}),
         'source pins match manifest')
    runs = capture['runs']
    need(len(runs) == 2 and [r['mode'] for r in runs] == ['normal', 'optimized'], 'run modes')
    for run, flags in zip(runs, [['-B'], ['-B', '-O']]):
        need(set(run) == {'mode', 'argv', 'returncode', 'stdout', 'stderr'}, 'run schema')
        need(run['returncode'] == 0 and run['stderr'] == '', 'run success')
        need(run['argv'][1:-1] == flags and Path(run['argv'][-1]).name == 'check.py', 'command/receipt')
        semantic(json.loads(run['stdout']))
        replay = subprocess.run([sys.executable] + flags + [str(BASE / 'check.py')], text=True,
                                capture_output=True, timeout=20)
        need(replay.returncode == 0 and replay.stderr == '' and replay.stdout == run['stdout'], 'causal exact replay')
    need(runs[0]['stdout'] == runs[1]['stdout'], 'normal/-O identical')
    mutated = deepcopy(json.loads(runs[0]['stdout']))
    mutated['grid_cases'][0]['bulk_candidate_ids_total_derived'] = 16 * 7998
    rejected = False
    try:
        semantic(mutated)
    except RuntimeError:
        rejected = True
    need(rejected, 'reader semantic mutation killed')
    inventory()
    need(sha(BASE / 'manifest.json') == expected, 'manifest unchanged after replay')
    for name, pin in manifest['files'].items():
        need(sha(BASE / name) == pin, 'file unchanged after replay: ' + name)
    print(json.dumps({'status': 'PASS', 'grid_sizes': [8000, 16000, 32000], 'packing_cases': 66,
                      'mutants': 2, 'reader_semantic_mutant_killed': True, 'manifest_sha256': expected}, sort_keys=True))


if __name__ == '__main__':
    main()
