#!/usr/bin/env python3
"""Read existing captures only; no timing/performance or global-parent claim."""
import hashlib
import json
from pathlib import Path
import sys

MEASURES = {'stage_ms', 'generation_wspd_ms', 'generation_rects_ms', 'elapsed_before_terminal_ms',
            'provisional_output_ms', 'digest_ms', 'compute_read_release_ms_subtracted_diagnostic',
            'rss_mib_sample', 'hwm_mib_sample'}
WORKERS = {'pipeline_threads', 'threads', 'workers_wspd', 'workers_rects', 'workers_sort',
           'workers_prefilter', 'workers_census', 'workers_expand'}
ADDED = {'extra_shell_diagnostics', 'extra_shell_diagnostic_records'}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def load(path):
    rows = [json.loads(line) for line in path.read_text().splitlines() if line.strip()]
    need(len(rows) == 2 and [row['type'] for row in rows] == ['configuration', 'terminal'], 'configuration + refusal only')
    config, terminal = rows
    need(terminal['exit_code'] == 2 and terminal['outcome'] == 'unsupported_degeneracy' and
         terminal['reason'] == 'probe_rank_relevant_extra_shell' and terminal['last_stage'] == 'regularity' and
         terminal['completed_orders_diagnostic'] == 0 and terminal['complete_requested_horizontal_orders'] is False,
         'refusal not promoted')
    command_path = path.with_suffix('.command.json')
    command = json.loads(command_path.read_text())
    need(command['exit_code'] == 2 and command['stdout_sha256'] == sha(path) and
         command['stderr_sha256'] == sha(path.with_suffix('.stderr')), 'command code and stream hashes')
    return rows, dict(stdout_sha256=sha(path), stderr_sha256=sha(path.with_suffix('.stderr')),
                     command_sha256=sha(command_path), command=command)


def compare(old, new):
    excluded = MEASURES | WORKERS | ADDED
    left = {key: value for key, value in old.items() if key not in excluded}
    right = {key: value for key, value in new.items() if key not in excluded}
    # JSON text preserves bool/int and int/float distinctions recursively.
    equal = json.dumps(left, sort_keys=True) == json.dumps(right, sort_keys=True)
    need(equal, 'common non-measurement semantics/work changed')
    difference = {key: dict(old_gcp=old.get(key), new_local=new.get(key)) for key in sorted(set(old) | set(new))
                  if json.dumps(old.get(key), sort_keys=True) != json.dumps(new.get(key), sort_keys=True)}
    return dict(equal_nonexcluded_fields=len(left), nonexcluded_fields=sorted(left), differences=difference)


def main():
    need(len(sys.argv) == 3, 'arguments: LOCAL_RUN_DIRECTORY GCP_GUEST_DIRECTORY')
    local, gcp = map(Path, sys.argv[1:])
    fresh, fresh_meta = load(local/'n50000_k10.stdout')
    old, old_meta = load(gcp/'n50000_k10.stdout')
    old5, old5_meta = load(gcp/'n50000_k5.stdout')
    need(fresh[1]['input_digest'] == old[1]['input_digest'] == old5[1]['input_digest'], 'all input digests equal')
    result = dict(status='same_K10_nonmeasurement_capture_fields', diagnostic_only=True, public_status='not_claimed',
        local=fresh_meta, gcp_k10=old_meta, gcp_k5=old5_meta,
        configuration=compare(old[0], fresh[0]), terminal=compare(old[1], fresh[1]),
        input_digest=fresh[1]['input_digest'], gcp_k10_extra_shell=old[1]['rank_relevant_extra_shell'],
        gcp_k5_extra_shell=old5[1]['rank_relevant_extra_shell'],
        ball_key_identity_with_gcp_proven=False,
        limitation='GCP captures contain aggregate counts, not ball keys or a catalogue digest. Matching counters do not prove identity of its individual balls.',
        censoring='Both processes return explicit unsupported_degeneracy code 2 before any FULL order; neither is a timeout-censored FULL run.')
    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
