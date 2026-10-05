#!/usr/bin/env python3
"""Rejeu stdlib du correctif de label et ancrage du commit worker, sans calcul natif/cloud."""
import copy
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tarfile


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def derive():
    here = Path(__file__).resolve().parent
    proof = json.loads((here / 'matrix_correction.json').read_text())
    repo = Path('/workspaces/E-HGP')
    def git(pin, path, wanted):
        raw = subprocess.check_output(['git', '-C', str(repo), 'show', pin + ':' + path])
        need(sha(raw) == wanted, 'source changee : ' + path)
        return raw
    pin = proof['source_commit']
    raw = git(pin, proof['matrix_path'], proof['corrected_matrix_sha256'])
    old_raw = git(proof['old_evidence_commit'], proof['old_snapshot_path'], proof['old_matrix_sha256'])
    actual, old = json.loads(raw), json.loads(old_raw)
    corrected_only = copy.deepcopy(old)
    prior = next(c for c in corrected_only['configurations'] if c['name'] == 'gcc_release')
    need(prior.pop('require_labels_if_data') == ['lidar'], 'cause initiale differente')
    need(corrected_only == actual, 'delta autre que suppression conditionnelle')
    code = git(pin, proof['judge_path'], proof['judge_sha256'])
    ns = {'__name__': 'audit_judge_only', '__file__': str(repo / proof['judge_path'])}
    exec(compile(code, ns['__file__'], 'exec'), ns)
    config = ns['validate_configuration'](next(c for c in actual['configurations'] if c['name'] == 'gcc_release'))
    original = ns['validate_configuration'](next(c for c in old['configurations'] if c['name'] == 'gcc_release'))
    archive = Path('/workspaces/.ehgp-sessions/v11.20261005.claudefina2/results/results.tar.gz')
    need(sha(archive.read_bytes()) == proof['fina2_archive_sha256'], 'archive changee')
    with tarfile.open(archive) as tar:
        inventory_raw = tar.extractfile(proof['inventory_member']).read()
    need(sha(inventory_raw) == proof['inventory_sha256'], 'inventaire change')
    inventory = json.loads(inventory_raw)
    need(len({t['name'] for t in inventory}) == len(inventory), 'inventaire duplique')
    args = config['ctest_args']
    opts = dict(zip(args[::2], args[1::2]))
    need(set(opts) == {'-LE'}, 'selection differente')
    selected = [t for t in inventory if not t['disabled'] and
                not any(re.search(opts['-LE'], label) for label in t['labels'])]
    labels = sorted({label for t in selected for label in t['labels']})
    need(len(selected) == 873 and labels == ['fast', 'oracle', 'unit'] and
         config['require_labels_if_data'] == [], 'correctif different')
    # Sortie construite pour la logique du juge : cas favorable suppose873PASS, aucun calcul natif execute.
    stdout = '\n'.join('%d/%d Test #%d: %s .... Passed 0.00 sec' % (i, len(selected), i, t['name'])
                       for i, t in enumerate(selected, 1))
    stdout += '\n100%% tests passed, 0 tests failed out of %d\n' % len(selected)
    limits = dict(ns['LIMITS'], **actual['limits'])
    step = {'status': 'ok', 'exit_code': 0}
    before = ns['judge_tests'](selected, step, stdout, None, '', limits, original, True)
    after = ns['judge_tests'](selected, step, stdout, None, '', limits, config, True)
    need(before['status'] == 'floor_violated' and before['reason'].endswith('lidar') and after['status'] == 'ok',
         'effet causal non reproduit')
    marking = proof['future_measurement_marking']
    worker = git(pin, marking['worker']['repo_path'], marking['worker']['sha256']).decode()
    controller = git(pin, marking['controller']['repo_path'], marking['controller']['sha256']).decode()
    need('export V11_SOURCE_PIN="${SOURCE}"' in worker and '--source) SOURCE="$2"' in worker and
         '^(commit|worktree_snapshot):[0-9a-f]{40,64}$' in worker,
         'transmission source worker differente')
    need("'--source', context['worker_source']" in controller, 'raccord controller different')
    return dict(selected_count=873, selected_labels=labels, data_given=True,
        fixture_assumes_all_selected_pass=True, sole_matrix_delta='remove gcc_release.require_labels_if_data',
        actual_judge_before=dict(status=before['status'], reason=before['reason']),
        actual_judge_after=dict(status=after['status'], reason=after['reason']),
        source_corrected=True, native_requalified=False,
        packaged_commit_already_exported_by_worker=True, bench_environment_read_change_not_claimed_implemented=True,
        closed_receipts_modified=False, native_or_cloud_runs=0)


def main():
    result = derive()
    proof = json.loads(Path(__file__).with_name('matrix_correction.json').read_text())
    need(result == proof['result'], 'preuve divergente')
    print(json.dumps(result, ensure_ascii=False, sort_keys=True))


if __name__ == '__main__':
    main()
