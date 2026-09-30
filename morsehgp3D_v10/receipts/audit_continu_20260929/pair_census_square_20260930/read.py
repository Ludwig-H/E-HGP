"""Strict externally pinned seven-file reader; never loads before manifest pin."""
import argparse
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import subprocess
import sys

BASE = Path(__file__).resolve().parent
FILES = {'README.md', 'resolver_snapshot.py', 'check.py', 'record.py', 'read.py', 'capture.json'}
SHARED_PATH = '/workspaces/E-HGP/build/v10-frontiere/work/bench/frontier/frontier_core.py'
SHARED_SHA = '86ba984ff7986bdd44a5d53e9b2c7d3f2c0eb847764938cd28259d18439850b7'
SNAPSHOT_SHA = '5f234ded085eb3d3fca7d072a268265a02605559f7c37793220cec65aba1d63b'


def need(ok, msg):
    if not ok:
        raise RuntimeError(msg)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inventory():
    need({p.name for p in BASE.iterdir()} == FILES | {'manifest.json'}, 'closed actual seven-file inventory')
    need(all(p.is_file() and not p.is_symlink() for p in BASE.iterdir()), 'regular files only')


def semantic(result):
    need(set(result) == {'scope', 'snapshot_sha256', 'small_cases', 'streaming_controls',
                         'large_analytic', 'memo_scope', 'mutants_killed'}, 'strict result schema')
    need(result['scope'] == 'pinned actual census/argmin with ideal strict-interior generator and analytic toy forest; no native/GCP',
         'explicit perfect mock scope')
    need(result['snapshot_sha256'] == SNAPSHOT_SHA == sha(BASE / 'resolver_snapshot.py'), 'real method snapshot pin')
    small = result['small_cases']
    need(len(small) == 20, 'small cases only')
    actual_configs = set()
    for c in small:
        need(set(c) == {'m', 'directed_votes', 'fresh_states', 'sphere_tests', 'argmin_distances',
                        'peak_materialized_list', 'query_reverse', 'candidate_reverse',
                        'owners_checked', 'replay_no_new_work'}, 'strict small result')
        m = c['m']
        need(m in (2, 3, 5, 8, 16) and type(c['query_reverse']) is bool and type(c['candidate_reverse']) is bool,
             'small m and two boolean orders')
        actual_configs.add((m, c['query_reverse'], c['candidate_reverse']))
        need(c['directed_votes'] == c['owners_checked'] == 3 * m - 2, 'all directed-band owners at own levels')
        need(c['fresh_states'] == 2 * m - 1 <= c['directed_votes'], 'memo universe count')
        need(c['sphere_tests'] == c['argmin_distances'] == m * (m - 1) // 2 and
             c['peak_materialized_list'] == m - 1 and c['replay_no_new_work'] is True, 'actual quadratic paid census')
    need(actual_configs == {(m, q, c) for m in (2, 3, 5, 8, 16) for q in (False, True) for c in (False, True)},
         'closed unique configuration inventory')
    large = result['large_analytic']
    need(len(large) == 3 and [c['m'] for c in large] == [8000, 16000, 32000], 'analytic large sizes')
    for c in large:
        need(set(c) == {'m', 'n', 'directed_votes', 'fresh_keep0_states', 'sphere_tests', 'argmin_distances',
                        'sphere_plus_argmin', 'peak_materialized_list', 'measurement'}, 'strict analytical schema')
        m = c['m']
        need(c['measurement'] == 'analytic_only' and c['n'] == m + 1 and c['directed_votes'] == 3 * m - 2,
             'large counts not execution/timing')
        need(c['fresh_keep0_states'] == 2 * m - 1 and c['sphere_tests'] == c['argmin_distances'] == m * (m - 1) // 2 and
             c['sphere_plus_argmin'] == m * (m - 1) and c['peak_materialized_list'] == m - 1, 'exact analytical formulas')
    controls = result['streaming_controls']
    need(len(controls) == 4 and [c['m'] for c in controls] == [3, 5, 8, 16], 'small streaming and mutation controls')
    for c in controls:
        need(set(c) == {'m', 'keep1_trace_first_two', 'keep1_owner', 'midpoint_mutant_next_endpoint',
                        'midpoint_mutant_owner', 'streaming_visits', 'streaming_peak_selected_records',
                        'stopfirst_different_selections', 'keep1_repeat_uncached', 'memo_read_mutant_killed'},
             'strict causal control schema')
        m = c['m']
        need(c['keep1_trace_first_two'] == [[m, 0, m - 1], [m, m - 1, 0]], 'actual held-anchor argmin trace')
        need(c['midpoint_mutant_next_endpoint'] == 1 and c['keep1_owner'] == c['midpoint_mutant_owner'] == 'R',
             'different selection does not imply wrong owner')
        need(c['streaming_visits'] == m * (m - 1) // 2 and c['streaming_peak_selected_records'] == 1 and
             c['stopfirst_different_selections'] == m - 2, 'streaming only reduces selected storage')
        need(c['keep1_repeat_uncached'] is True and c['memo_read_mutant_killed'] is True, 'cache paid-work mutation')
    need(result['memo_scope'] == 'synchronous keep0 accepted-band universe; no eviction; not keep1/workers/arbitrary probes',
         'memo theorem explicit scope')
    need(result['mutants_killed'] == {'midpoint_not_anchor': True, 'memo_read_disabled': True}, 'two causal mutants')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--manifest-sha256', required=True)
    expected = ap.parse_args().manifest_sha256
    need(len(expected) == 64 and all(c in '0123456789abcdef' for c in expected), 'external SHA format')
    need(sha(BASE / 'manifest.json') == expected, 'external manifest SHA before JSON or replay')
    inventory()
    manifest = json.loads((BASE / 'manifest.json').read_text())
    need(set(manifest) == {'schema', 'capture_valid', 'files'} and manifest['schema'] == 1 and
         manifest['capture_valid'] is True and set(manifest['files']) == FILES, 'strict manifest')
    for name, pin in manifest['files'].items():
        need(sha(BASE / name) == pin, 'input file pin: ' + name)
    capture = json.loads((BASE / 'capture.json').read_text())
    need(set(capture) == {'schema', 'scope', 'started_utc', 'shared_before', 'source_before', 'runs',
                          'shared_after', 'source_after', 'finished_utc'} and capture['schema'] == 1, 'strict capture')
    need(capture['scope'] == 'actual third_sites square under ideal index; analytic large counts; no native/GCP', 'capture scope')
    need(capture['shared_before'] == capture['shared_after'] == {SHARED_PATH: SHARED_SHA}, 'pinned source stable before/after')
    need(capture['source_before'] == capture['source_after'] and set(capture['source_before']) == FILES - {'capture.json'} and
         all(capture['source_before'][n] == manifest['files'][n] for n in FILES - {'capture.json'}), 'source pin set stable')
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
    bad['large_analytic'][0]['sphere_tests'] = bad['large_analytic'][0]['m']
    rejected = False
    try:
        semantic(bad)
    except RuntimeError:
        rejected = True
    need(rejected, 'linear-census reader semantic mutant rejected')
    inventory()
    need(sha(BASE / 'manifest.json') == expected, 'manifest unchanged after replay')
    for name, pin in manifest['files'].items():
        need(sha(BASE / name) == pin, 'all files unchanged after replay: ' + name)
    print(json.dumps({'status': 'PASS', 'small_cases': 20, 'owners_checked': 368,
                      'large_sizes_analytic_only': [8000, 16000, 32000], 'causal_mutants': 2,
                      'reader_semantic_mutant_killed': True, 'manifest_sha256': expected}, sort_keys=True))


if __name__ == '__main__':
    main()
