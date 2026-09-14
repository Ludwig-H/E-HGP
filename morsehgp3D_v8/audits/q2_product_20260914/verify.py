#!/usr/bin/env python3
"""Replay a closed joint model, including a separate joint-admission fixture."""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import zipfile

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[2]


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def record(name, selected):
    require(name.isidentifier() and selected.isidentifier(), 'invalid receipt name')
    output = BASE / (name + '_VALIDATION.json')
    require(not output.exists(), 'refuse overwrite')
    report = dict(schema='mhgp8_q2_product_validation_v1', status='running', model_run=selected,
                  started_utc=datetime.now(timezone.utc).isoformat(), commands=[], pins={},
                  public_status='not_claimed', gcp_used=False, verifier_sha256=sha(Path(__file__)))

    def pin(path):
        report['pins'][str(path.relative_to(ROOT))] = sha(path)

    def save():
        output.write_text(json.dumps(report, indent=2, sort_keys=True, allow_nan=False) + '\n')

    def run(command):
        step = dict(command=command)
        report['commands'].append(step)
        save()
        r = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=180)
        step.update(returncode=r.returncode, stderr=r.stderr,
                    stdout_sha256=hashlib.sha256(r.stdout.encode()).hexdigest())
        require(r.returncode == 0, 'model replay failed: ' + r.stderr)
        return json.loads(r.stdout)

    save()
    try:
        for run_name in dict.fromkeys(('r1', selected)):
            path = BASE / (run_name + '_RUN.json')
            receipt = json.loads(path.read_text())
            result_path = BASE / (run_name + '_RESULT.json')
            archive = ROOT / receipt['source_archive']
            for p in (path, result_path, archive):
                pin(p)
            require(sha(archive) == receipt['source_archive_sha256'] and
                    sha(result_path) == receipt['result_sha256'], 'changed model capture')
            with zipfile.ZipFile(archive) as z:
                require(set(z.namelist()) == set(receipt['sources']), 'wrong source archive')
                for p, h in receipt['sources'].items():
                    require(hashlib.sha256(z.read(p)).hexdigest() == h, 'changed archived source')
                    if run_name == selected:
                        require(sha(ROOT / p) == h, 'changed active model source')
                        pin(ROOT / p)
            require(receipt['status'] == ('passed' if run_name == selected else 'failed'),
                    'unexpected capture status')
        static = json.loads((BASE / 'STATIC_REVIEW.json').read_text())
        archive = ROOT / static['archive']
        require(sha(archive) == static['archive_sha256'], 'changed static review')
        with zipfile.ZipFile(archive) as z:
            require(set(z.namelist()) == set(static['sources']), 'wrong static source paths')
            for p, h in static['sources'].items():
                require(hashlib.sha256(z.read(p)).hexdigest() == h, 'changed static source')
        for p in (BASE / 'STATIC_REVIEW.json', archive, Path(__file__), BASE / 'README.md'):
            pin(p)
        expected = json.loads((BASE / (selected + '_RESULT.json')).read_text())
        normal = run([sys.executable, '-B', str(BASE / 'model.py')])
        optimized = run([sys.executable, '-B', '-O', str(BASE / 'model.py')])
        require(normal == optimized == expected, 'normal/optimized/captured model differs')
        report['normal_optimized_and_captured_identical'] = True
        require(len(expected['rows']) == 36 and len(expected['mutants']) == 6, 'missing model case')
        report['configuration_count'] = 36
        report['model_calls'] = 222
        report['mutants'] = {k: dict(fixture=v['fixture'], detected_by=v['detected_by'])
                             for k, v in expected['mutants'].items()}
        require(all(row['modes']['joint_then_anchor']['work']['joint_accepted_pairs'] == 0
                    and row['modes']['joint_then_anchor']['work']['handoffs_after_anchor_zero_consumed'] == 0
                    and not any(event['kind'] == 'joint_transition'
                                for event in row['modes']['joint_then_anchor']['events'])
                    for row in expected['rows']), 'strict-policy reachable-state invariant failed')
        report['strict_policy_reachable_state_checks'] = 36
        sys.path.insert(0, str(BASE))
        import model
        cube = next(c for c in model.cases() if c[0] == 'cube_shells')
        _, points, a, b = cube
        extra = model.compare(points, a, b, 1, refinement='witness_first')
        joint = extra['joint_then_anchor']
        require(joint['work']['joint_accepted_pairs'] == 16
                and joint['work']['handoff_pairs'] == 0
                and sum(len(row[2][2]) for row in joint['accepted']) == 72,
                'joint admission/complete shell fixture failed')
        report['joint_admission_extra'] = dict(scope='witness_first_policy_not_current_product',
                                               calls=6, results=extra)
        local = json.loads((BASE / 'self_node_r1_RUN.json').read_text())
        result_path = BASE / 'self_node_r1_RESULT.json'
        archive = ROOT / local['source_archive']
        require(local['status'] == 'passed' and sha(result_path) == local['result_sha256']
                and sha(archive) == local['source_archive_sha256'], 'changed self-node capture')
        for p in (BASE / 'self_node_r1_RUN.json', result_path, archive):
            pin(p)
        with zipfile.ZipFile(archive) as z:
            require(set(z.namelist()) == set(local['sources']), 'wrong self-node source paths')
            for p, h in local['sources'].items():
                require(hashlib.sha256(z.read(p)).hexdigest() == h == sha(ROOT / p),
                        'changed self-node source')
                pin(ROOT / p)
        self_normal = run([sys.executable, '-B', str(BASE / 'self_node.py')])
        self_optimized = run([sys.executable, '-B', '-O', str(BASE / 'self_node.py')])
        require(self_normal == self_optimized == json.loads(result_path.read_text()),
                'self-node normal/optimized/captured results differ')
        report['self_node'] = dict(normal_optimized_and_captured_identical=True,
                                  configurations=self_normal['configurations'],
                                  model_calls=self_normal['model_calls'],
                                  comparison_summary=self_normal['comparison_summary'])
        sys.path.insert(0, str(ROOT / 'tools'))
        import check_docs
        require(not check_docs.validate(BASE / 'README.md'), 'product audit docs failed')
        require(all(sha(ROOT / p) == h for p, h in report['pins'].items()), 'source changed during checks')
        report['status'] = 'passed'
    except BaseException as error:
        report.update(status='failed', error_type=type(error).__name__, error=str(error))
        raise
    finally:
        report['finished_utc'] = datetime.now(timezone.utc).isoformat()
        save()
    print(json.dumps(dict(status=report['status'], report=str(output), base_calls=228, policy_calls=228)))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--name', default='r1')
    parser.add_argument('--run', default='r2')
    args = parser.parse_args()
    record(args.name, args.run)
