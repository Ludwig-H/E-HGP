#!/usr/bin/env python3
"""Contrelecture stdlib d'une exigence de label impossible ; aucun test natif/cloud."""
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


def select(config, inventory):
    args = config['ctest_args']
    opts = dict(zip(args[::2], args[1::2]))
    return [test for test in inventory if not test['disabled'] and
            ('-E' not in opts or not re.search(opts['-E'], test['name'])) and
            ('-R' not in opts or re.search(opts['-R'], test['name'])) and
            ('-L' not in opts or any(re.search(opts['-L'], label) for label in test['labels'])) and
            ('-LE' not in opts or not any(re.search(opts['-LE'], label) for label in test['labels']))]


def main():
    here = Path(__file__).resolve().parent
    proof = json.loads((here / 'proof.json').read_text())
    repo = Path('/workspaces/E-HGP')
    raw = (here / 'matrix_snapshot.json').read_bytes()
    need(sha(raw) == proof['matrix_sha256_before'] == proof['matrix_sha256_after_capture'], 'matrice changee')
    code = subprocess.check_output(['git', '-C', str(repo), 'show',
                                   proof['source_worktree_head'] + ':' + proof['judge_source_path']])
    need(sha(code) == proof['judge_source_sha256'], 'juge change')
    ns = {'__name__': 'audit_judge_only', '__file__': str(repo / proof['judge_source_path'])}
    exec(compile(code, ns['__file__'], 'exec'), ns)
    matrix = ns['load_matrix'](here / 'matrix_snapshot.json')
    config = next(c for c in matrix['configurations'] if c['name'] == 'gcc_release')
    archive = Path('/workspaces/.ehgp-sessions/v11.20261005.claudefina2/results/results.tar.gz')
    need(sha(archive.read_bytes()) == proof['fina2_archive_sha256'], 'archive changee')
    with tarfile.open(archive) as tar:
        inventory_raw = tar.extractfile(proof['fina2_inventory_member']).read()
    need(sha(inventory_raw) == proof['fina2_inventory_sha256'], 'inventaire change')
    inventory = json.loads(inventory_raw)
    need(len({t['name'] for t in inventory}) == len(inventory), 'inventaire duplique')
    selected = select(config, inventory)
    labels = sorted({label for t in selected for label in t['labels']})
    need(len(selected) == 873 and labels == ['fast', 'oracle', 'unit'] and
         config['require_labels_if_data'] == ['lidar'], 'cause de disjonction differente')
    # Temoignage de logique du juge uniquement : supposer toutes les portes PASS est le cas le plus favorable.
    # Cette sortie construite n'est pas un journal d'execution ni une qualification du WIP.
    stdout = '\n'.join('%d/%d Test #%d: %s .... Passed 0.00 sec' % (i, len(selected), i, t['name'])
                       for i, t in enumerate(selected, 1))
    stdout += '\n100%% tests passed, 0 tests failed out of %d\n' % len(selected)
    step = {'status': 'ok', 'exit_code': 0}
    before = ns['judge_tests'](selected, step, stdout, None, '', matrix['limits'], config, True)
    corrected = copy.deepcopy(config)
    corrected['require_labels_if_data'] = []
    after = ns['judge_tests'](selected, step, stdout, None, '', matrix['limits'], corrected, True)
    need(before['status'] == 'floor_violated' and before['reason'].endswith('lidar') and
         before['tests']['passed'] == 873 and before['tests']['failed'] == before['tests']['not_run'] == 0,
         'refus impossible non reproduit')
    need(after['status'] == 'ok', 'suppression seule ne corrige pas la cause')
    result = dict(selected_count=len(selected), selected_labels=labels, conditional_required=['lidar'],
                  data_given=True, fixture_assumes_all_selected_pass=True,
                  actual_judge_before=dict(status=before['status'], reason=before['reason'], tests=before['tests']),
                  actual_judge_after_removing_only_conditional_label=dict(status=after['status'], reason=after['reason']),
                  native_or_cloud_runs=0)
    need(result == proof['result'], 'preuve divergente')
    print(json.dumps(result, ensure_ascii=False, sort_keys=True))


if __name__ == '__main__':
    main()
